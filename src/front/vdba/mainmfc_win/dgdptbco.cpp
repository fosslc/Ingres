/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpTbCo.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    into two statements to avoid compiler error.
**  24-Feb-2000 (uk$so01)
**     Bug #100562, Add "nKeySequence" to the COLUMNPARAMS to indicate if the columns
**     is part of primary key or unique key.
**  24-Oct-2000 (schph01)
**     Sir 102821 Comment on table and columns.
**  08-Dec-2000 (schph01)
**     Sir 102823 When columns type is Object Key or Table Key, add System
**     Maintained information.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdptbco.h"
#include "vtree.h"
#include "dgerrh.h"     // MessageWithHistoryButton

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "dbadlg1.h"
  #include "resource.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CuDlgDomPropTableColumns::AddTblColumn(CuTblColumn* pTblColumn)
{
  ASSERT (pTblColumn);
  BOOL  bSpecial = pTblColumn->IsSpecialItem();

  CString csTmp;

  int nCount, index;
  nCount = m_cListColumns.GetItemCount ();

  // Name
  index = m_cListColumns.InsertItem (nCount, (LPCSTR)pTblColumn->GetStrName(), 0);

  // Other columns of container not to be filled if special
  if (!bSpecial) {
    // type
    m_cListColumns.SetItemText (index, 1, (LPCSTR)pTblColumn->GetType());
    
    // Rank in primary key
    int primKeyRank = pTblColumn->GetPrimKeyRank();
    if (primKeyRank > 0) {
      csTmp.Format(_T("%d"), primKeyRank);
      m_cListColumns.SetItemText (index, 2, (LPCSTR)csTmp);
    }
    
    // Nullable - Default
    if (pTblColumn->IsNullable())
	  	m_cListColumns.SetCheck (index, 3, TRUE);
	  else
	  	m_cListColumns.SetCheck (index, 3, FALSE);
    if (pTblColumn->HasDefault())
	  	m_cListColumns.SetCheck (index, 4, TRUE);
	  else
	  	m_cListColumns.SetCheck (index, 4, FALSE);

    // Default specification
    if (pTblColumn->HasDefault())
      m_cListColumns.SetItemText (index, 5, (LPCSTR)pTblColumn->GetDefSpec());

    // Comment on column
    m_cListColumns.SetItemText (index, 6, (LPCSTR)pTblColumn->GetColumnComment());
  }

  // Attach pointer on column, for future sort purposes
  m_cListColumns.SetItemData(index, (DWORD)pTblColumn);
}

CuDlgDomPropTableColumns::CuDlgDomPropTableColumns(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropTableColumns::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropTableColumns)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropTableColumns::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropTableColumns)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_TABLE);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTableColumns, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropTableColumns)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableColumns message handlers

void CuDlgDomPropTableColumns::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropTableColumns::OnInitDialog() 
{
  CDialog::OnInitDialog();

  //
  // subclass columns list control and connect image list
  //
  VERIFY (m_cListColumns.SubclassDlgItem (IDC_MFC_LIST1, this));
	m_ImageList.Create(1, 18, TRUE, 1, 0);
	m_cListColumns.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListColumns.SetCheckImageList(&m_ImageCheck);

  #define COLLIST_NUMBER  7
  CHECKMARK_COL_DESCRIBE  aColColumns[COLLIST_NUMBER] = {
    /* { _T("Order #"),                FALSE, 60 }, */
    { _T(""),                       FALSE, 80 },
    { _T(""),                       FALSE, 100},
    { _T(""),                       FALSE, -1 },
    { _T("With Null"),              TRUE,  -1 },
    { _T("With Default"),           TRUE,  -1 },
    { _T(""),                       FALSE, -1 },
    { _T(""),                       FALSE, -1 },
  };
  lstrcpy(aColColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_COL_NAME));
  lstrcpy(aColColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_TYPE));
  lstrcpy(aColColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_PRIM_KEY));
  lstrcpy(aColColumns[5].szCaption, VDBA_MfcResourceString(IDS_TC_DEFAULT_SPEC));
  lstrcpy(aColColumns[6].szCaption, VDBA_MfcResourceString(IDS_TC_COLUMN_COMMENT));

  InitializeColumns(m_cListColumns, aColColumns, COLLIST_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropTableColumns::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListColumns.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListColumns.MoveWindow (r);
}


LONG CuDlgDomPropTableColumns::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropTableColumns::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
  // cast received parameters
  int nNodeHandle = (int)wParam;
  LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
  ASSERT (nNodeHandle != -1);
  ASSERT (pUps);

  // ignore selected actions on filters
  switch (pUps->nIpmHint)
  {
    case 0:
    //case FILTER_DOM_SYSTEMOBJECTS:
    //case FILTER_DOM_BASEOWNER:
    //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH_DETAIL:
      if (m_Data.m_refreshParams.MustRefresh(pUps->pSFilter->bOnLoad, pUps->pSFilter->refreshtime))
        break;    // need to update
      else
        return 0; // no need to update
      break;
    case FILTER_SETTING_CHANGE:
        VDBA_OnGeneralSettingChange(&m_cListColumns);
        return 0L;
    default:
      return 0L;    // nothing to change on the display
  }

  // Get info on the object
  TABLEPARAMS tableparams;
  memset (&tableparams, 0, sizeof (tableparams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  x_strcpy ((char *)tableparams.DBName,     (const char *)lpRecord->extra);
  x_strcpy ((char *)tableparams.objectname, RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName)));
  x_strcpy ((char *)tableparams.szSchema,   (const char *)lpRecord->ownerName);
  tableparams.detailSubset = TABLE_SUBSET_COLUMNSONLY;  // minimize query duration

  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_TABLE,
                               &tableparams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    // Table may not exist if propagate too fast
    if (iResult != RES_ENDOFDATA)
      ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataTableColumns tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Create error item and that's it!
    /* "<Data Unavailable>" */
    CuTblColumn tblColumn1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaColumns.Add(tblColumn1);

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();
  TCHAR *lpObjComment = NULL;
  int iret = RES_SUCCESS;
  LPOBJECTLIST ls ,lpColTemp,lpObjCol = NULL;
  LPCOMMENTCOLUMN lpCommentCol;
  ls = tableparams.lpColumns;
  while (ls)
  {
    lpColTemp = AddListObjectTail(&lpObjCol, sizeof(COMMENTCOLUMN));
    if (!lpColTemp)
    {
      // Need to free previously allocated objects.
      FreeObjectList(lpObjCol);
      lpObjCol = NULL;
      break;
    }
    LPCOLUMNPARAMS lstbl=(LPCOLUMNPARAMS)ls->lpObject;
    lpCommentCol = (LPCOMMENTCOLUMN)lpColTemp->lpObject;
    lstrcpy((LPTSTR)lpCommentCol->szColumnName,(LPCTSTR)(LPUCHAR)lstbl->szColumn);
    lpCommentCol->lpszComment = NULL;
    ls=(LPOBJECTLIST)ls->lpNext;
  }

  if (lpObjCol)
    iret = VDBAGetCommentInfo ( (LPTSTR)(LPUCHAR)GetVirtNodeName(nNodeHandle),
                                (LPTSTR)tableparams.DBName, (LPTSTR)tableparams.objectname,
                                (LPTSTR)tableparams.szSchema, &lpObjComment, lpObjCol);
  else
    ASSERT (FALSE);

  // update member variables, for display/load/save purpose

  // list of columns
  m_Data.m_uaColumns.RemoveAll();
  ls = tableparams.lpColumns;
  ASSERT (ls);    // No columns is unacceptable
  while (ls) {
    LPCOLUMNPARAMS lpCol = (LPCOLUMNPARAMS)ls->lpObject;
    ASSERT (lpCol);

    // Column name
    CString csName = lpCol->szColumn;

    // Format "column type"
    CString csType = "";
    if (lstrcmpi(lpCol->tchszInternalDataType, lpCol->tchszDataType) != 0)  {
      ASSERT (lpCol->tchszInternalDataType[0]);
      csType = lpCol->tchszInternalDataType;  // UDTs
      if (lstrcmpi(lpCol->tchszInternalDataType,VDBA_MfcResourceString(IDS_OBJECT_KEY)) == 0 ||
          lstrcmpi(lpCol->tchszInternalDataType,VDBA_MfcResourceString(IDS_TABLE_KEY) ) == 0) {
          if (lpCol->bSystemMaintained)
              csType += VDBA_MfcResourceString(IDS_SYSTEM_MAINTAINED);
          else
              csType += VDBA_MfcResourceString(IDS_NO_SYSTEM_MAINTAINED);
      }
    }
    else {
      LPUCHAR lpType;
      lpType = GetColTypeStr(lpCol);
      if (lpType) {
        csType = lpType;
        ESL_FreeMem(lpType);
      }
      else {
        // Unknown type: put type name "as is" - length will not be displayed
        csType = lpCol->tchszDataType;
	  }
    }

    // Format "default specification"
    CString csDefSpec = "";
    if (lpCol->lpszDefSpec)
      csDefSpec = lpCol->lpszDefSpec;

    // Format "Comment"
    CString csTblComment = "";
    LPOBJECTLIST LsObj = lpObjCol;
    LPCOMMENTCOLUMN lpCommentCol;

    while (LsObj)
    {
      lpCommentCol = (LPCOMMENTCOLUMN)LsObj->lpObject;
      if (strcmp((LPTSTR)lpCommentCol->szColumnName,(LPTSTR)lpCol->szColumn) == 0)
      {
        if (iret!=RES_SUCCESS && !lpCommentCol->lpszComment)
          csTblComment.LoadString(IDS_NOT_AVAILABLE);
        else
        {
            if (lpCommentCol->lpszComment)
              csTblComment = lpCommentCol->lpszComment;
        }
      }
      LsObj = (LPOBJECTLIST)LsObj->lpNext;
    }

    // item on the stack
    CuTblColumn tblColumn(csName,             // LPCTSTR name,
                          csType,             // LPCTSTR type, ??? Code complexe
                          lpCol->nKeySequence,// int primKeyRank,
                          lpCol->bNullable,   // BOOL bNullable,
                          lpCol->bDefault,    // BOOL bWithDefault,
                          csDefSpec,          // LPCTSTR defSpec
                          csTblComment );     // LPCTSTR comment on column
    CuMultFlag *pRefCol = m_Data.m_uaColumns.Find(&tblColumn);
    ASSERT (!pRefCol);
    m_Data.m_uaColumns.Add(tblColumn);

    // Loop on next column
    ls = (LPOBJECTLIST)ls->lpNext;
  }

  // liberate detail structure
  FreeAttachedPointers (&tableparams,  OT_TABLE);
  // liberate column comment
  ls = lpObjCol;
  while (ls)
  {
    lpCommentCol = (LPCOMMENTCOLUMN)ls->lpObject;
    if (lpCommentCol->lpszComment)
      ESL_FreeMem (lpCommentCol->lpszComment);
    ls = (LPOBJECTLIST)ls->lpNext;
  }
  FreeObjectList(lpObjCol);
  if (lpObjComment)
    ESL_FreeMem (lpObjComment);
  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTableColumns::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataTableColumns") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataTableColumns* pData = (CuDomPropDataTableColumns*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataTableColumns)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropTableColumns::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataTableColumns* pData = new CuDomPropDataTableColumns(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropTableColumns::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  // List of columns
  int size, cnt;
  m_cListColumns.DeleteAllItems();
  size = m_Data.m_uaColumns.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuTblColumn* pTblColumn = m_Data.m_uaColumns[cnt];
    AddTblColumn(pTblColumn);
  }
}

void CuDlgDomPropTableColumns::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListColumns.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListColumns);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
