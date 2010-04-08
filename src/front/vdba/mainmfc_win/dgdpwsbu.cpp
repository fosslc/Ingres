// dgdpwsbu.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwsbu.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropIceSecDbuser, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceSecDbuser dialog


CuDlgDomPropIceSecDbuser::CuDlgDomPropIceSecDbuser()
	: CFormView(CuDlgDomPropIceSecDbuser::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceSecDbuser)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceSecDbuser::~CuDlgDomPropIceSecDbuser()
{
}

void CuDlgDomPropIceSecDbuser::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceSecDbuser)
	DDX_Control(pDX, IDC_UNITREAD, m_chkUnitRead);
	DDX_Control(pDX, IDC_READDOC, m_chkReadDoc);
	DDX_Control(pDX, IDC_CREATEDOC, m_chkCreateDoc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceSecDbuser, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceSecDbuser)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceSecDbuser diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceSecDbuser::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceSecDbuser::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceSecDbuser message handlers

void CuDlgDomPropIceSecDbuser::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceSecDbuser::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceSecDbuser::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  ASSERT (objType == OT_ICE_BUNIT_SEC_USER);

  // Get Info
  ICEBUSUNITWEBUSERDATA  iceStruct;
  memset (&iceStruct, 0, sizeof(iceStruct));
  x_strcpy((LPSTR)iceStruct.icewevuser.UserName, (LPCSTR)lpRecord->objName);
  x_strcpy((LPSTR)iceStruct.icebusunit.Name, (LPCSTR)lpRecord->extra);
  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            objType,
                            &iceStruct);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceSecDbuser tempData;
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
  m_Data.m_bUnitRead   = iceStruct.bUnitRead;
  m_Data.m_bCreateDoc = iceStruct.bCreateDoc;
  m_Data.m_bReadDoc   = iceStruct.bReadDoc;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceSecDbuser::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceSecDbuser") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceSecDbuser* pData = (CuDomPropDataIceSecDbuser*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceSecDbuser)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceSecDbuser::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceSecDbuser* pData = new CuDomPropDataIceSecDbuser(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceSecDbuser::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_chkUnitRead.SetCheck(m_Data.m_bUnitRead);
  m_chkCreateDoc.SetCheck(m_Data.m_bCreateDoc);
  m_chkReadDoc.SetCheck(m_Data.m_bReadDoc);
}

void CuDlgDomPropIceSecDbuser::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_chkUnitRead.SetCheck(0);
  m_chkCreateDoc.SetCheck(0);
  m_chkReadDoc.SetCheck(0);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);

}
