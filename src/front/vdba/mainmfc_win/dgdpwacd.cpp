// dgdpwacd.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwacd.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropIceAccessDef, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceAccessDef dialog


CuDlgDomPropIceAccessDef::CuDlgDomPropIceAccessDef()
	: CFormView(CuDlgDomPropIceAccessDef::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceAccessDef)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceAccessDef::~CuDlgDomPropIceAccessDef()
{
}

void CuDlgDomPropIceAccessDef::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceAccessDef)
	DDX_Control(pDX, IDC_UPDATE, m_chkUpdate);
	DDX_Control(pDX, IDC_READ, m_chkRead);
	DDX_Control(pDX, IDC_MFC_DELETE, m_chkDelete);
	DDX_Control(pDX, IDC_EXECUTE, m_chkExecute);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceAccessDef, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceAccessDef)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceAccessDef diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceAccessDef::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceAccessDef::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceAccessDef message handlers

void CuDlgDomPropIceAccessDef::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceAccessDef::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceAccessDef::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  int objType = lpRecord->recType;
  ASSERT (objType == OT_ICE_BUNIT_FACET_ROLE || objType == OT_ICE_BUNIT_FACET_USER
       || objType == OT_ICE_BUNIT_PAGE_ROLE || objType == OT_ICE_BUNIT_PAGE_USER   );

  m_Data.m_objType = objType;   // MANDATORY!

  ICEBUSUNITDOCROLEDATA IceFrole;
  memset(&IceFrole,0,sizeof(ICEBUSUNITDOCROLEDATA));
  ICEBUSUNITDOCUSERDATA IceRuser;
  memset(&IceRuser,0,sizeof(ICEBUSUNITDOCUSERDATA));
  LPVOID lpStruct;

  switch (objType) {
    case OT_ICE_BUNIT_FACET_ROLE:
    case OT_ICE_BUNIT_PAGE_ROLE:
      lpStruct = &IceFrole;
      lstrcpy((LPSTR)IceFrole.icebusunitdoc.icebusunit.Name, (LPCSTR)lpRecord->extra);
      lstrcpy((LPSTR)IceFrole.icebusunitdoc.name, (LPCSTR)lpRecord->extra2);
      lstrcpy((LPSTR)IceFrole.icebusunitdocrole.RoleName, (LPCSTR)lpRecord->objName);
      if (objType == OT_ICE_BUNIT_FACET_ROLE)
        IceFrole.icebusunitdoc.bFacet = TRUE;
      else
        IceFrole.icebusunitdoc.bFacet = FALSE;
      break;

    case OT_ICE_BUNIT_FACET_USER:
    case OT_ICE_BUNIT_PAGE_USER:
      lpStruct = &IceRuser;
      lstrcpy((LPSTR)IceRuser.icebusunitdoc.icebusunit.Name, (LPCSTR)lpRecord->extra);
      lstrcpy((LPSTR)IceRuser.icebusunitdoc.name, (LPCSTR)lpRecord->extra2);
      lstrcpy((LPSTR)IceRuser.user.UserName, (LPCSTR)lpRecord->objName);
      if (objType == OT_ICE_BUNIT_FACET_USER)
        IceRuser.icebusunitdoc.bFacet = TRUE;
      else
        IceRuser.icebusunitdoc.bFacet = FALSE;
      break;

    default:
      ASSERT (FALSE);
      return 0;
  }

  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            objType,
                            lpStruct);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceAccessDef tempData;
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
  switch (objType) {
    case OT_ICE_BUNIT_FACET_ROLE:
    case OT_ICE_BUNIT_PAGE_ROLE:
      m_Data.m_bExecute = IceFrole.bExec;
      m_Data.m_bRead    = IceFrole.bRead;
      m_Data.m_bUpdate  = IceFrole.bUpdate;
      m_Data.m_bDelete  = IceFrole.bDelete;
      break;

    case OT_ICE_BUNIT_FACET_USER:
    case OT_ICE_BUNIT_PAGE_USER:
      m_Data.m_bExecute = IceRuser.bExec;
      m_Data.m_bRead    = IceRuser.bRead;
      m_Data.m_bUpdate  = IceRuser.bUpdate;
      m_Data.m_bDelete  = IceRuser.bDelete;
      break;
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceAccessDef::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceAccessDef") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceAccessDef* pData = (CuDomPropDataIceAccessDef*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceAccessDef)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceAccessDef::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceAccessDef* pData = new CuDomPropDataIceAccessDef(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceAccessDef::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_chkExecute.SetCheck(m_Data.m_bExecute);
  m_chkRead.SetCheck(m_Data.m_bRead);
  m_chkUpdate.SetCheck(m_Data.m_bUpdate);
  m_chkDelete.SetCheck(m_Data.m_bDelete);
}

void CuDlgDomPropIceAccessDef::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  m_chkExecute.SetCheck(0);
  m_chkRead.SetCheck(0);
  m_chkUpdate.SetCheck(0);
  m_chkDelete.SetCheck(0);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
