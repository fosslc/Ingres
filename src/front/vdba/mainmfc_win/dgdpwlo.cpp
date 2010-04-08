// DgDpWlo.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwlo.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropIceLocation, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceLocation dialog


CuDlgDomPropIceLocation::CuDlgDomPropIceLocation()
	: CFormView(CuDlgDomPropIceLocation::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceLocation)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceLocation::~CuDlgDomPropIceLocation()
{
}

void CuDlgDomPropIceLocation::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceLocation)
	DDX_Control(pDX, IDC_PATH, m_edPath);
	DDX_Control(pDX, IDC_PUBLIC, m_chkPublic);
	DDX_Control(pDX, IDC_EXTENSION, m_edExtension);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceLocation, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceLocation)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceLocation diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceLocation::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceLocation::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceLocation message handlers

void CuDlgDomPropIceLocation::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceLocation::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceLocation::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  ICELOCATIONDATA IceLocationParams;
  memset (&IceLocationParams, 0, sizeof (IceLocationParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get ICE Detail Info
  //
  int objType = lpRecord->recType;
  ASSERT (objType == OT_ICE_SERVER_LOCATION
       || objType == OT_ICE_BUNIT_LOCATION);
  lstrcpy ((LPTSTR)IceLocationParams.LocationName, (LPCTSTR)lpRecord->objName);
  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            OT_ICE_SERVER_LOCATION,
                            &IceLocationParams);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceLocation tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_bIce         = IceLocationParams.bIce;
  m_Data.m_bPublic      = IceLocationParams.bPublic;
  m_Data.m_csPath       = IceLocationParams.path;
  m_Data.m_csExtensions = IceLocationParams.extensions;
  m_Data.m_csComment = _T("");

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceLocation::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceLocation") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceLocation* pData = (CuDomPropDataIceLocation*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceLocation)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceLocation::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceLocation* pData = new CuDomPropDataIceLocation(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceLocation::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edPath.SetWindowText      (m_Data.m_csPath);
  m_edExtension.SetWindowText (m_Data.m_csExtensions);
  m_chkPublic.SetCheck        (m_Data.m_bPublic? 1: 0);
  CheckRadioButton(IDC_RADIO_HTTP, IDC_RADIO_ICE, m_Data.m_bIce? IDC_RADIO_ICE: IDC_RADIO_HTTP);
}

void CuDlgDomPropIceLocation::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_edPath.SetWindowText      (m_Data.m_csPath);
  m_edExtension.SetWindowText (m_Data.m_csExtensions);
  m_chkPublic.SetCheck        (0);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);

}
