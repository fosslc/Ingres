// DgDpWDbc.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwdbc.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropIceDbconnection, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceDbconnection dialog


CuDlgDomPropIceDbconnection::CuDlgDomPropIceDbconnection()
	: CFormView(CuDlgDomPropIceDbconnection::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceDbconnection)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceDbconnection::~CuDlgDomPropIceDbconnection()
{
}

void CuDlgDomPropIceDbconnection::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceDbconnection)
	DDX_Control(pDX, IDC_DBUSER, m_edDBUser);
	DDX_Control(pDX, IDC_COMMENT, m_edComment);
	DDX_Control(pDX, 1003, m_edDBName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceDbconnection, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceDbconnection)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceDbconnection diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceDbconnection::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceDbconnection::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceDbconnection message handlers

void CuDlgDomPropIceDbconnection::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceDbconnection::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceDbconnection::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  DBCONNECTIONDATA IceDbconnectionParams;
  memset (&IceDbconnectionParams, 0, sizeof (IceDbconnectionParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get ICE Detail Info
  //
  int objType = lpRecord->recType;
  ASSERT (objType == OT_ICE_DBCONNECTION
       || objType == OT_ICE_WEBUSER_CONNECTION
       || objType == OT_ICE_PROFILE_CONNECTION);
  lstrcpy ((LPTSTR)IceDbconnectionParams.ConnectionName, (LPCTSTR)lpRecord->objName);
  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            OT_ICE_DBCONNECTION,
                            &IceDbconnectionParams);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceDbconnection tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csDBName  = IceDbconnectionParams.DBName;
  m_Data.m_csDBUser  = IceDbconnectionParams.DBUsr.UserAlias;
  m_Data.m_csComment = IceDbconnectionParams.Comment;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceDbconnection::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceDbconnection") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceDbconnection* pData = (CuDomPropDataIceDbconnection*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceDbconnection)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceDbconnection::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceDbconnection* pData = new CuDomPropDataIceDbconnection(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceDbconnection::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edDBName.SetWindowText(m_Data.m_csDBName);
  m_edDBUser.SetWindowText(m_Data.m_csDBUser);
  m_edComment.SetWindowText(m_Data.m_csComment);
}

void CuDlgDomPropIceDbconnection::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edDBName.SetWindowText(_T(""));
  m_edDBUser.SetWindowText(_T(""));
  m_edComment.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
