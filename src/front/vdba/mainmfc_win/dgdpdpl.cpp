// DgDpDPL.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpdpl.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropProcLink, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcLink dialog


CuDlgDomPropProcLink::CuDlgDomPropProcLink()
	: CFormView(CuDlgDomPropProcLink::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropProcLink)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_PROCEDURE);
}

CuDlgDomPropProcLink::~CuDlgDomPropProcLink()
{
}

void CuDlgDomPropProcLink::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropProcLink)
	DDX_Control(pDX, IDC_DOMPROP_PROC_TXT, m_edText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropProcLink, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropProcLink)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcLink diagnostics

#ifdef _DEBUG
void CuDlgDomPropProcLink::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropProcLink::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcLink message handlers

void CuDlgDomPropProcLink::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropProcLink::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropProcLink::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  PROCEDUREPARAMS ProcedureParams;
  memset (&ProcedureParams, 0, sizeof (ProcedureParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();

  //
  // Get Detail Info
  //
  x_strcpy ((char *)ProcedureParams.objectname, (const char *)lpRecord->objName);
  x_strcpy ((char *)ProcedureParams.objectowner, (const char *)lpRecord->ownerName);
  x_strcpy ((char *)ProcedureParams.DBName, (const char *)lpRecord->extra);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_PROCEDURE,
                               &ProcedureParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataProcedure tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csText = ProcedureParams.lpProcedureText;


  // liberate detail structure
  FreeAttachedPointers (&ProcedureParams, OT_PROCEDURE);

  //
  // No list of Rules executing procedure
  //
  m_Data.m_acsRulesExecutingProcedure.RemoveAll();    // For serialization only

  //
  // Refresh display
  //
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropProcLink::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataProcedure") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataProcedure* pData = (CuDomPropDataProcedure*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataProcedure)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropProcLink::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataProcedure* pData = new CuDomPropDataProcedure(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropProcLink::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edText.SetWindowText(m_Data.m_csText);
}

void CuDlgDomPropProcLink::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edText.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}