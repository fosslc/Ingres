// DgDpVi.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpdvl.h"

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


CuDlgDomPropViewLink::CuDlgDomPropViewLink(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropViewLink::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropViewLink)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_VIEW);
}


void CuDlgDomPropViewLink::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropViewLink)
      // NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_DOMPROP_VIEW_TXT, m_edText);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropViewLink, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropViewLink)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropViewLink message handlers

void CuDlgDomPropViewLink::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropViewLink::OnInitDialog() 
{
  CDialog::OnInitDialog();

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropViewLink::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

	if (!IsWindow (m_edText.m_hWnd))
		return;

	CRect rDlg, r;
	GetClientRect (rDlg);

  // adjust width and height of statement
	m_edText.GetWindowRect (r);
	ScreenToClient (r);
	r.bottom = rDlg.bottom - r.left;
	r.right = rDlg.right - r.left;
	m_edText.MoveWindow (r);
}


LONG CuDlgDomPropViewLink::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropViewLink::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  x_strcpy ((char *)ViewParams.objectname, (const char *)lpRecord->objName);
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

  // No list of view components!
  //
  m_Data.m_acsViewComponents.RemoveAll();   // For serialization only
  m_Data.m_acsSchema.RemoveAll();           // For serialization only

  //
  // Refresh display
  //
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropViewLink::OnLoad (WPARAM wParam, LPARAM lParam)
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

LONG CuDlgDomPropViewLink::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataView* pData = new CuDomPropDataView(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropViewLink::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_edText.SetWindowText(m_Data.m_csText);

}


void CuDlgDomPropViewLink::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edText.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}