// DgDpWwu.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwwu.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropIceWebuser, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceWebuser dialog


CuDlgDomPropIceWebuser::CuDlgDomPropIceWebuser()
  : CFormView(CuDlgDomPropIceWebuser::IDD)
{
  //{{AFX_DATA_INIT(CuDlgDomPropIceWebuser)
  //}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceWebuser::~CuDlgDomPropIceWebuser()
{
}

void CuDlgDomPropIceWebuser::DoDataExchange(CDataExchange* pDX)
{
  CFormView::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropIceWebuser)
  DDX_Control(pDX, IDC_UNIT_MANAGER, m_chkUnitMgrPriv);
  DDX_Control(pDX, IDC_TIMEOUT, m_edtimeoutms);
  DDX_Control(pDX, IDC_SECUR_ADMIN, m_chkSecurityPriv);
  DDX_Control(pDX, IDC_MONITORING, m_chkMonitorPriv);
  DDX_Control(pDX, IDC_COMMENT, m_edComment);
  DDX_Control(pDX, IDC_DEFDBUSER, m_edDefDBUser);
  DDX_Control(pDX, IDC_ADMIN, m_chkAdminPriv);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceWebuser, CFormView)
  //{{AFX_MSG_MAP(CuDlgDomPropIceWebuser)
    ON_WM_SIZE()
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceWebuser diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceWebuser::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceWebuser::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceWebuser message handlers

void CuDlgDomPropIceWebuser::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceWebuser::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceWebuser::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  ICEWEBUSERDATA IceWebuserParams;
  memset (&IceWebuserParams, 0, sizeof (IceWebuserParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get ICE Detail Info
  //
  lstrcpy ((LPTSTR)IceWebuserParams.UserName, (LPCTSTR)lpRecord->objName);
  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            OT_ICE_WEBUSER,
                            &IceWebuserParams);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceWebuser tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csDefDBUser   = IceWebuserParams.DefDBUsr.UserAlias;
  m_Data.m_csComment     = IceWebuserParams.Comment      ;
  m_Data.m_bAdminPriv    = IceWebuserParams.bAdminPriv   ;
  m_Data.m_bSecurityPriv = IceWebuserParams.bSecurityPriv;
  m_Data.m_bUnitMgrPriv  = IceWebuserParams.bUnitMgrPriv ;
  m_Data.m_bMonitorPriv  = IceWebuserParams.bMonitorPriv ;
  m_Data.m_ltimeoutms    = IceWebuserParams.ltimeoutms   ;
  m_Data.m_bRepositoryPsw = IceWebuserParams.bICEpwd;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceWebuser::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceWebuser") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceWebuser* pData = (CuDomPropDataIceWebuser*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceWebuser)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceWebuser::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceWebuser* pData = new CuDomPropDataIceWebuser(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceWebuser::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_edDefDBUser.SetWindowText(m_Data.m_csDefDBUser);
  m_edComment.SetWindowText  (m_Data.m_csComment);
  m_chkAdminPriv.SetCheck    (m_Data.m_bAdminPriv    ? 1: 0);
  m_chkSecurityPriv.SetCheck (m_Data.m_bSecurityPriv ? 1: 0);
  m_chkUnitMgrPriv.SetCheck  (m_Data.m_bUnitMgrPriv  ? 1: 0);
  m_chkMonitorPriv.SetCheck  (m_Data.m_bMonitorPriv  ? 1: 0);
  CString csTimeOut;
  csTimeOut.Format(_T("%ld"), m_Data.m_ltimeoutms);
  m_edtimeoutms.SetWindowText(csTimeOut);

  int idcRad = -1;
  if (m_Data.m_bRepositoryPsw)
    idcRad = IDC_RADIO_REPOSITORY;
  else
    idcRad = IDC_RADIO_OPERASYSTEM;
  CheckRadioButton(IDC_RADIO_REPOSITORY, IDC_RADIO_OPERASYSTEM, idcRad);
}

void CuDlgDomPropIceWebuser::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_edDefDBUser.SetWindowText(_T(""));
  m_edComment.SetWindowText  (_T(""));
  m_chkAdminPriv.SetCheck    (0);
  m_chkSecurityPriv.SetCheck (0);
  m_chkUnitMgrPriv.SetCheck  (0);
  m_chkMonitorPriv.SetCheck  (0);
  m_edtimeoutms.SetWindowText(_T(""));
  CheckRadioButton(IDC_RADIO_REPOSITORY, IDC_RADIO_OPERASYSTEM, -1);
}
