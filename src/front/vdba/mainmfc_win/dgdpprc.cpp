// DgDpPrc.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpprc.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropProcedure, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcedure dialog


CuDlgDomPropProcedure::CuDlgDomPropProcedure()
	: CFormView(CuDlgDomPropProcedure::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropProcedure)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_PROCEDURE);
}

CuDlgDomPropProcedure::~CuDlgDomPropProcedure()
{
}

void CuDlgDomPropProcedure::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropProcedure)
	DDX_Control(pDX, IDC_DOMPROP_PROC_TXT, m_edText);
	DDX_Control(pDX, IDC_DOMPROP_PROC_RULESLIST, m_lbRulesExecutingProcedure);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropProcedure, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropProcedure)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcedure diagnostics

#ifdef _DEBUG
void CuDlgDomPropProcedure::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropProcedure::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcedure message handlers

void CuDlgDomPropProcedure::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropProcedure::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropProcedure::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  x_strcpy ((char *)ProcedureParams.objectname, RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName)));
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
  // Get list of Rules executing procedure
  //
  m_Data.m_acsRulesExecutingProcedure.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  LPUCHAR aparentsResult[MAXPLEVEL];
  UCHAR   bufParResult0[MAXOBJECTNAME];
  UCHAR   bufParResult1[MAXOBJECTNAME];
  UCHAR   bufParResult2[MAXOBJECTNAME];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->extra;  // DBName

  UCHAR bufParent1[MAXOBJECTNAME];
  aparentsTemp[1] = bufParent1;
  x_strcpy((char *)buf, (const char *)lpRecord->objName);
  StringWithOwner(buf, lpRecord->ownerName, aparentsTemp[1]); // schema.name

  aparentsTemp[2] = NULL;

  iret =  DOMGetFirstRelObject(nNodeHandle,
                               OTR_PROC_RULE,
                               2,                           // level,
                               aparentsTemp,                // Temp mandatory!
                               pUps->pSFilter->bWithSystem, // bwithsystem
                               NULL,                        // base owner
                               NULL,                        // other owner
                               aparentsResult,
                               buf,
                               bufOwner,
                               bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CString csUnavail;
    csUnavail.LoadString(IDS_TM_ERR_ERR);// = "< Data Unavailable >";
    m_Data.m_acsRulesExecutingProcedure.Add(csUnavail);
  }
  else {
    while (iret == RES_SUCCESS) {
      m_Data.m_acsRulesExecutingProcedure.Add((LPCTSTR)buf);
      iret = DOMGetNextRelObject(aparentsResult, buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_acsRulesExecutingProcedure.GetUpperBound() == -1) {
    CString csNoGrp;
    csNoGrp.LoadString(IDS_TREE_RULE_NONE);// = "< No Rules >";
    m_Data.m_acsRulesExecutingProcedure.Add(csNoGrp);
  }

  //
  // Refresh display
  //
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropProcedure::OnLoad (WPARAM wParam, LPARAM lParam)
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

LONG CuDlgDomPropProcedure::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataProcedure* pData = new CuDomPropDataProcedure(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropProcedure::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edText.SetWindowText(m_Data.m_csText);
  int cnt;
  int size;

  m_lbRulesExecutingProcedure.ResetContent();
  size = m_Data.m_acsRulesExecutingProcedure.GetSize();
  for (cnt = 0; cnt < size; cnt++)
    m_lbRulesExecutingProcedure.AddString(m_Data.m_acsRulesExecutingProcedure[cnt]);

}

void CuDlgDomPropProcedure::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edText.SetWindowText(_T(""));
  m_lbRulesExecutingProcedure.ResetContent();
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}