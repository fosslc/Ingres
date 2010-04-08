/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpVi.cpp
**
**  Description:
**
**  History:
**   24-Oct-2000 (schph01)
**     sir 102821 Comment on table and columns.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpvi.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "resource.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgDomPropView::CuDlgDomPropView(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropView::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropView)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_VIEW);
}


void CuDlgDomPropView::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropView)
      // NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_EDIT_COMMENT, m_ctrlEditComment);
	DDX_Control(pDX, IDC_DOMPROP_VIEW_TXT, m_edText);
	DDX_Control(pDX, IDC_MFC_LIST1, m_clistCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropView, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropView)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropView message handlers

void CuDlgDomPropView::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropView::OnInitDialog() 
{
  CDialog::OnInitDialog();

  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_VIEWTABLE);
  m_ImageList.AddIcon(IDI_TREE_STATIC_TABLE);
  m_ImageList.AddIcon(IDI_TREE_STATIC_VIEW);
  m_clistCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);

  #define VIEWCOMP_NUMBER  2
  CHECKMARK_COL_DESCRIBE  aViewComps[VIEWCOMP_NUMBER] = {
    { _T(""), FALSE, 125 + 16 },      // + 16 for image
    { _T(""), FALSE, 125 },
  };
  lstrcpy(aViewComps[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME)); // _T("Name")
  lstrcpy(aViewComps[1].szCaption, VDBA_MfcResourceString(IDS_TC_OWNER));

  InitializeColumns(m_clistCtrl, aViewComps, VIEWCOMP_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropView::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

	if (!IsWindow (m_clistCtrl.m_hWnd))
		return;

	CRect rDlg, r,r2;
	GetClientRect (rDlg);

  // adjust width of view components list
	m_clistCtrl.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - r.left;
	m_clistCtrl.MoveWindow (r);

  // adjust width of edit comment
  // keep the size of m_clistCtrl because there is a static before the
  // edit comment
	r2 = r;
	m_ctrlEditComment.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - r2.left;
	m_ctrlEditComment.MoveWindow (r);

  // adjust width and height of statement
	m_edText.GetWindowRect (r);
	ScreenToClient (r);
	r.bottom = rDlg.bottom - r.left;
	r.right = rDlg.right - r.left;
	m_edText.MoveWindow (r);
}


LONG CuDlgDomPropView::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropView::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
        VDBA_OnGeneralSettingChange(&m_clistCtrl);
        return 0L;
    default:
      return 0L;    // nothing to change on the display
  }

  // Get info on the object
  VIEWPARAMS ViewParams;
  memset (&ViewParams, 0, sizeof (ViewParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get Detail Info
  //
  x_strcpy ((char *)ViewParams.objectname, RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName)));
  x_strcpy ((char *)ViewParams.szSchema, (const char *)lpRecord->ownerName);
  x_strcpy ((char *)ViewParams.DBName, (const char *)lpRecord->extra);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_VIEW,
                               &ViewParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataView tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csText = ViewParams.lpViewText;

  // liberate detail structure
  FreeAttachedPointers (&ViewParams, OT_VIEW);

  //
  // Get list of view components
  //
  m_Data.m_acsViewComponents.RemoveAll();
  m_Data.m_acsSchema.RemoveAll();
  m_Data.m_awCompType.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->extra;  // DBName

  UCHAR bufParent1[MAXOBJECTNAME];
  aparentsTemp[1] = bufParent1;
  x_strcpy((char *)buf, (const char *)lpRecord->objName);
  StringWithOwner(buf, lpRecord->ownerName, aparentsTemp[1]); // schema.name

  aparentsTemp[2] = NULL;

  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_VIEWTABLE,
                            2,                           // level,
                            aparentsTemp,                // Temp mandatory!
                            pUps->pSFilter->bWithSystem, // bwithsystem
                            NULL,                        // owner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CString csUnavail;
    csUnavail.LoadString(IDS_TM_ERR_ERR);// = "< Data Unavailable >";
    m_Data.m_acsViewComponents.Add(csUnavail);
    m_Data.m_acsSchema.Add(_T(""));
    m_Data.m_awCompType.Add(OT_VIEWTABLE);    // unresolved view component
  }
  else {
    while (iret == RES_SUCCESS) {
      m_Data.m_acsViewComponents.Add((LPCTSTR)buf);
      m_Data.m_acsSchema.Add((LPCTSTR)bufOwner);

      // Solve type on the fly
      int viewCompType;
      UCHAR resBuf[MAXOBJECTNAME];
      UCHAR resBuf2[MAXOBJECTNAME];
      UCHAR resBuf3[MAXOBJECTNAME];
      LPUCHAR aparentsSolve[MAXPLEVEL];
      aparentsSolve[0] = lpRecord->extra;  // DBName
      aparentsSolve[1] = aparentsSolve[2] = NULL;
      int res = DOMGetObjectLimitedInfo(nNodeHandle,
                                        buf,
                                        bufOwner,
                                        OT_VIEWTABLE,
                                        1,     // level
                                        aparentsSolve,  // parentstrings,
                                        TRUE,
                                        &viewCompType,
                                        resBuf,
                                        resBuf2,
                                        resBuf3);
      if (res != RES_SUCCESS)
        viewCompType = OT_VIEWTABLE;

      m_Data.m_awCompType.Add(viewCompType);    // OT_TABLE or OT_VIEW, or OT_VIEWTABLE if error

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_acsViewComponents.GetUpperBound() == -1) {
    CString csNoViewComp;
	csNoViewComp.LoadString(IDS_VIEW_COMPONENTS);// = "< No View Components>";
    m_Data.m_acsViewComponents.Add(csNoViewComp);
    m_Data.m_acsSchema.Add(_T(""));
    m_Data.m_awCompType.Add(OT_VIEWTABLE);    // unresolved view component
  }

  // Get comment
  TCHAR *lpObjComment = NULL;
  LPOBJECTLIST lpObjCol = NULL;

  iret = VDBAGetCommentInfo ( (LPTSTR)(LPUCHAR)GetVirtNodeName(nNodeHandle),
                       (LPTSTR)ViewParams.DBName, (LPTSTR)ViewParams.objectname,
                       (LPTSTR)ViewParams.szSchema, &lpObjComment, lpObjCol);

  if (iret!=RES_SUCCESS)
  {
    CString csMsg;
    csMsg.LoadString(IDS_NOT_AVAILABLE);
    m_ctrlEditComment.SetWindowText(csMsg);
    m_Data.m_csComment = csMsg;
  }
  else if (lpObjComment)
  {
      m_ctrlEditComment.SetWindowText(lpObjComment);
      m_Data.m_csComment = lpObjComment;
      ESL_FreeMem(lpObjComment);
  }
  else
  {
    m_ctrlEditComment.SetWindowText(_T(""));
    m_Data.m_csComment.Empty();
  }

  //
  // Refresh display
  //
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropView::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataView") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataView* pData = (CuDomPropDataView*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataView)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropView::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataView* pData = new CuDomPropDataView(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropView::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_edText.SetWindowText(m_Data.m_csText);
  m_ctrlEditComment.SetWindowText(m_Data.m_csComment);

  int cnt;
  int size;
  m_clistCtrl.DeleteAllItems();
  size = m_Data.m_acsViewComponents.GetSize();
  ASSERT (size > 0);
  ASSERT (m_Data.m_acsSchema.GetSize() == size);
  ASSERT (m_Data.m_awCompType.GetSize() == size);

  for (cnt = 0; cnt < size; cnt++) {
    int imageIndex = GetImageIndexFromType(m_Data.m_awCompType[cnt]);
    int nCount = m_clistCtrl.GetItemCount ();
    int index  = m_clistCtrl.InsertItem (nCount, m_Data.m_acsViewComponents[cnt], imageIndex);
    m_clistCtrl.SetItemText (index, 1, m_Data.m_acsSchema[cnt]);
  }
}

int CuDlgDomPropView::GetImageIndexFromType(int componentType)
{
  int imageIndex;

  switch(componentType) {
    case OT_VIEWTABLE:
      imageIndex = 0;
      break;
    case OT_TABLE:
      imageIndex = 1;
      break;
    case OT_VIEW:
      imageIndex = 2;
      break;
    default:
      ASSERT (FALSE);
      imageIndex = 0;
  }
  return imageIndex;
}

void CuDlgDomPropView::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_clistCtrl.DeleteAllItems();
  m_edText.SetWindowText(_T(""));
  m_ctrlEditComment.SetWindowText(_T(""));
  VDBA_OnGeneralSettingChange(&m_clistCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
