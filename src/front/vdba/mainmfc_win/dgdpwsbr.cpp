// dgdpwsbr.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwsbr.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropIceSecRole, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceSecRole dialog


CuDlgDomPropIceSecRole::CuDlgDomPropIceSecRole()
	: CFormView(CuDlgDomPropIceSecRole::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceSecRole)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceSecRole::~CuDlgDomPropIceSecRole()
{
}

void CuDlgDomPropIceSecRole::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceSecRole)
	DDX_Control(pDX, IDC_READDOC, m_chkReadDoc);
	DDX_Control(pDX, IDC_EXECDOC, m_chkExecDoc);
	DDX_Control(pDX, IDC_CREATEDOC, m_chkCreateDoc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceSecRole, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceSecRole)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceSecRole diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceSecRole::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceSecRole::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceSecRole message handlers

void CuDlgDomPropIceSecRole::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceSecRole::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceSecRole::OnUpdateData (WPARAM wParam, LPARAM lParam)
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


  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  // double check type
  int objType = lpRecord->recType;
  ASSERT (objType == OT_ICE_BUNIT_SEC_ROLE);

  // Get Info
  ICEBUSUNITROLEDATA  iceStruct;
  memset (&iceStruct, 0, sizeof(iceStruct));
  x_strcpy((LPSTR)iceStruct.icerole.RoleName, (LPCSTR)lpRecord->objName);
  x_strcpy((LPSTR)iceStruct.icebusunit.Name, (LPCSTR)lpRecord->extra);
  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            objType,
                            &iceStruct);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceSecRole tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  //
  // update member variables, for display/load/save purpose
  //
  m_Data.m_bExecDoc   = iceStruct.bExecDoc;
  m_Data.m_bCreateDoc = iceStruct.bCreateDoc;
  m_Data.m_bReadDoc   = iceStruct.bReadDoc;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceSecRole::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceSecRole") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceSecRole* pData = (CuDomPropDataIceSecRole*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceSecRole)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceSecRole::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceSecRole* pData = new CuDomPropDataIceSecRole(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceSecRole::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_chkExecDoc.SetCheck(m_Data.m_bExecDoc);
  m_chkCreateDoc.SetCheck(m_Data.m_bCreateDoc);
  m_chkReadDoc.SetCheck(m_Data.m_bReadDoc);
}

void CuDlgDomPropIceSecRole::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_chkExecDoc.SetCheck(0);
  m_chkCreateDoc.SetCheck(0);
  m_chkReadDoc.SetCheck(0);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}