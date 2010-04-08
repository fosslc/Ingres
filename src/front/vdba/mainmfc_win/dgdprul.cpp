// DgDpRul.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdprul.h"

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


/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRule dialog


CuDlgDomPropRule::CuDlgDomPropRule(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropRule::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropRule)
        // NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_RULE);
}

void CuDlgDomPropRule::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropRule)
	DDX_Control(pDX, IDC_DOMPROP_RULE_TXT, m_edText);
	DDX_Control(pDX, IDC_DOMPROP_RULE_PROC, m_edProc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropRule, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropRule)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRule message handlers

void CuDlgDomPropRule::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropRule::OnInitDialog() 
{
  CDialog::OnInitDialog();

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropRule::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

	if (!IsWindow (m_edProc.m_hWnd))
		return;

	CRect rDlg, r;
	GetClientRect (rDlg);

  // adjust width of procedure name
	m_edProc.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - r.left;
	m_edProc.MoveWindow (r);

  // adjust width and height of statement
	m_edText.GetWindowRect (r);
	ScreenToClient (r);
	r.bottom = rDlg.bottom - r.left;
	r.right = rDlg.right - r.left;
	m_edText.MoveWindow (r);
}

LONG CuDlgDomPropRule::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropRule::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_DOM_OTHEROWNER:
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
  RULEPARAMS RuleParams;
  memset (&RuleParams, 0, sizeof (RuleParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get Detail Info
  //
  x_strcpy ((char *)RuleParams.RuleName,   RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName)));
  x_strcpy ((char *)RuleParams.RuleOwner,  (const char *)lpRecord->ownerName);
  x_strcpy ((char *)RuleParams.DBName,     (const char *)lpRecord->extra);
  x_strcpy ((char *)RuleParams.TableName,  RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->extra2)));
  x_strcpy ((char *)RuleParams.TableOwner, (const char *)lpRecord->tableOwner);

  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_RULE,
                               &RuleParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataRule tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csText  = RuleParams.lpRuleText;

  // liberate detail structure
  FreeAttachedPointers (&RuleParams, OT_RULE);

  //
  // Get name of procedure executed by the rule
  //
  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // Note: parenthoods with schemas
  aparentsTemp[0] = lpRecord->extra;    // database
  aparentsTemp[1] = lpRecord->extra2;   // schema.table
  UCHAR bufParent2[MAXOBJECTNAME];
  aparentsTemp[2] = bufParent2;
  x_strcpy((char *)buf, (const char *)lpRecord->objName);
  StringWithOwner(buf, lpRecord->ownerName, aparentsTemp[2]); // schema.rule

  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_RULEPROC,
                            3,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            (LPUCHAR)pUps->pSFilter->lpOtherOwner, // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    m_Data.m_csProc = VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE);//"<Data Unavailable>"
  }
  else {
    m_Data.m_csProc = buf;
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropRule::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataRule") == 0);
 ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataRule* pData = (CuDomPropDataRule*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataRule)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropRule::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataRule* pData = new CuDomPropDataRule(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropRule::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edProc.SetWindowText(m_Data.m_csProc);
  m_edText.SetWindowText(m_Data.m_csText);
}

void CuDlgDomPropRule::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edProc.SetWindowText(_T(""));
  m_edText.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}