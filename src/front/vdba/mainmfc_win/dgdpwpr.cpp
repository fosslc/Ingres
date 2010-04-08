// DgDpWPr.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwpr.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  //#include "monitor.h"
  #include "ice.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgDomPropIceProfile, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceProfile dialog


CuDlgDomPropIceProfile::CuDlgDomPropIceProfile()
	: CFormView(CuDlgDomPropIceProfile::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceProfile)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceProfile::~CuDlgDomPropIceProfile()
{
}

void CuDlgDomPropIceProfile::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceProfile)
	DDX_Control(pDX, IDC_DEFDBUSER, m_edDefProfile);
	DDX_Control(pDX, IDC_UNIT_MANAGER, m_chkUnitMgrPriv);
	DDX_Control(pDX, IDC_TIMEOUT, m_edtimeoutms);
	DDX_Control(pDX, IDC_SECUR_ADMIN, m_chkSecurityPriv);
	DDX_Control(pDX, IDC_MONITORING, m_chkMonitorPriv);
	DDX_Control(pDX, IDC_ADMIN, m_chkAdminPriv);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceProfile, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceProfile)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceProfile diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceProfile::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceProfile::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceProfile message handlers

void CuDlgDomPropIceProfile::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceProfile::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceProfile::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  ResetDisplay();
  // Get info on the object
  ICEPROFILEDATA IceProfileParams;
  memset (&IceProfileParams, 0, sizeof (IceProfileParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get ICE Detail Info
  //
  lstrcpy ((LPTSTR)IceProfileParams.ProfileName, (LPCTSTR)lpRecord->objName);
  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            OT_ICE_PROFILE,
                            &IceProfileParams);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceProfile tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csDefProfile  = IceProfileParams.DefDBUsr.UserAlias   ;
  m_Data.m_bAdminPriv    = IceProfileParams.bAdminPriv   ;
  m_Data.m_bSecurityPriv = IceProfileParams.bSecurityPriv;
  m_Data.m_bUnitMgrPriv  = IceProfileParams.bUnitMgrPriv ;
  m_Data.m_bMonitorPriv  = IceProfileParams.bMonitorPriv ;
  m_Data.m_ltimeoutms    = IceProfileParams.ltimeoutms   ;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceProfile::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceProfile") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceProfile* pData = (CuDomPropDataIceProfile*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceProfile)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceProfile::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceProfile* pData = new CuDomPropDataIceProfile(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceProfile::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_edDefProfile.SetWindowText(m_Data.m_csDefProfile);
  m_chkAdminPriv.SetCheck    (m_Data.m_bAdminPriv    ? 1: 0);
  m_chkSecurityPriv.SetCheck (m_Data.m_bSecurityPriv ? 1: 0);
  m_chkUnitMgrPriv.SetCheck  (m_Data.m_bUnitMgrPriv  ? 1: 0);
  m_chkMonitorPriv.SetCheck  (m_Data.m_bMonitorPriv  ? 1: 0);
  CString csTimeOut;
  csTimeOut.Format(_T("%ld"), m_Data.m_ltimeoutms);
  m_edtimeoutms.SetWindowText(csTimeOut);
}

void CuDlgDomPropIceProfile::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_edDefProfile.SetWindowText(_T(""));
  m_chkAdminPriv.SetCheck    (0);
  m_chkSecurityPriv.SetCheck (0);
  m_chkUnitMgrPriv.SetCheck  (0);
  m_chkMonitorPriv.SetCheck  (0);
  m_edtimeoutms.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}