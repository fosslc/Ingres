// DgDpDtL.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpdtl.h"
#include "vtree.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER   2


CuDlgDomPropTableLink::CuDlgDomPropTableLink(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropTableLink::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropTableLink)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_TABLE);
}


void CuDlgDomPropTableLink::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropTableLink)
  DDX_Control(pDX, IDC_EDIT_LDB_OWNER, m_edLdbObjOwner);
  DDX_Control(pDX, IDC_EDIT_LDB_NODE, m_edLdbNode);
  DDX_Control(pDX, IDC_EDIT_LDB_NAME, m_edLdbObjName);
  DDX_Control(pDX, IDC_EDIT_LDB_DBMSTYPE, m_edLdbDbmsType);
  DDX_Control(pDX, IDC_EDIT_LDB_DB, m_edLdbDatabase);
  //}}AFX_DATA_MAP
}

void CuDlgDomPropTableLink::AddTblColumn(CuStarTblColumn* pTblColumn)
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
    // ldb name
    m_cListColumns.SetItemText (index, 1, (LPCSTR)pTblColumn->GetLdbColName());

    // type
    m_cListColumns.SetItemText (index, 2, (LPCSTR)pTblColumn->GetType());
    
    // Nullable - Default
    if (pTblColumn->IsNullable())
      m_cListColumns.SetCheck (index, 3, TRUE);
    else
      m_cListColumns.SetCheck (index, 3, FALSE);
    if (pTblColumn->HasDefault())
      m_cListColumns.SetCheck (index, 4, TRUE);
    else
      m_cListColumns.SetCheck (index, 4, FALSE);
  }

  // Attach pointer on column, for future sort purposes
  m_cListColumns.SetItemData(index, (DWORD)pTblColumn);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTableLink, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropTableLink)
    ON_WM_SIZE()
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableLink message handlers

void CuDlgDomPropTableLink::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropTableLink::OnInitDialog() 
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

  #define COLLIST_NUMBER  5
  CHECKMARK_COL_DESCRIBE  aColColumns[COLLIST_NUMBER] = {
    /* { _T("Order #"),                FALSE, 60 }, */
    { _T(""),             FALSE, 100 },
    { _T(""),             FALSE, 100 },
    { _T(""),             FALSE, 100},
    { _T("With Null"),    TRUE,  -1 },
    { _T("With Default"), TRUE,  -1 },
  };
  lstrcpy(aColColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_COL_NAME));
  lstrcpy(aColColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_LBD_COL_NAME));
  lstrcpy(aColColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_TYPE));
  InitializeColumns(m_cListColumns, aColColumns, COLLIST_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropTableLink::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

  if (!IsWindow (m_cListColumns.m_hWnd))
      return;

  CRect rDlg, r;
  GetClientRect (rDlg);

  // adjust width and height of columns list
  m_cListColumns.GetWindowRect (r);
  ScreenToClient (r);
  r.bottom = rDlg.bottom - r.left;
  r.right = rDlg.right - r.left;
  m_cListColumns.MoveWindow (r);
}


LONG CuDlgDomPropTableLink::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropTableLink::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataStarTableLink tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csLdbObjName   = tableparams.szLdbObjName  ;
  m_Data.m_csLdbObjOwner  = tableparams.szLdbObjOwner ;
  m_Data.m_csLdbDatabase  = tableparams.szLdbDatabase ;
  m_Data.m_csLdbNode      = tableparams.szLdbNode     ;
  m_Data.m_csLdbDbmsType  = tableparams.szLdbDbmsType ;

  LPOBJECTLIST ls;

  // list of columns
  m_Data.m_uaColumns.RemoveAll();
  ls = tableparams.lpColumns;
  ASSERT (ls);    // No columns is unacceptable
  while (ls) {
    LPCOLUMNPARAMS lpCol = (LPCOLUMNPARAMS)ls->lpObject;
    ASSERT (lpCol);

    // Column name / ldb name
    CString csName = lpCol->szColumn;
    CString csLdbColName = lpCol->szLdbColumn;

    // Format "column type"
    CString csType = "";
    if (lstrcmpi(lpCol->tchszInternalDataType, lpCol->tchszDataType) != 0)  {
      ASSERT (lpCol->tchszInternalDataType[0]);
      csType = lpCol->tchszInternalDataType;  // UDTs
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

    // item on the stack
    CuStarTblColumn tblColumn(csName,             // LPCTSTR name,
                              csType,             // LPCTSTR type
                              lpCol->bNullable,   // BOOL bNullable,
                              lpCol->bDefault,    // BOOL bWithDefault,
                              csLdbColName);      // LPCTSTR ldbColName

    CuMultFlag *pRefCol = m_Data.m_uaColumns.Find(&tblColumn);
    ASSERT (!pRefCol);
    m_Data.m_uaColumns.Add(tblColumn);

    // Loop on next column
    ls = (LPOBJECTLIST)ls->lpNext;
  }

  // liberate detail structure
  FreeAttachedPointers (&tableparams,  OT_TABLE);

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTableLink::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataStarTableLink") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataStarTableLink* pData = (CuDomPropDataStarTableLink*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataStarTableLink)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropTableLink::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataStarTableLink* pData = new CuDomPropDataStarTableLink(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropTableLink::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  //
  // Exclusively use member variables of m_Data for display refresh
  //
  m_edLdbObjName.SetWindowText  (m_Data.m_csLdbObjName ) ;
  m_edLdbObjOwner.SetWindowText (m_Data.m_csLdbObjOwner) ;
  m_edLdbDatabase.SetWindowText (m_Data.m_csLdbDatabase) ;
  m_edLdbNode.SetWindowText     (m_Data.m_csLdbNode    ) ;
  m_edLdbDbmsType.SetWindowText (m_Data.m_csLdbDbmsType) ;

  int cnt;
  int size;

  m_cListColumns.DeleteAllItems();
  size = m_Data.m_uaColumns.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddTblColumn(m_Data.m_uaColumns[cnt]);
}


void CuDlgDomPropTableLink::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListColumns.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListColumns);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}