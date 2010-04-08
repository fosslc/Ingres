// DgDpInt.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpint.h"

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
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIntegrity dialog


CuDlgDomPropIntegrity::CuDlgDomPropIntegrity(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropIntegrity::IDD, pParent)
{
  //{{AFX_DATA_INIT(CuDlgDomPropIntegrity)
  //}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_INTEGRITY);
}

void CuDlgDomPropIntegrity::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropIntegrity)
  DDX_Control(pDX, IDC_DOMPROP_INTEG_TXT, m_edText);
  DDX_Control(pDX, IDC_DOMPROP_INTEG_NUM, m_edNumber);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIntegrity, CDialog)
  //{{AFX_MSG_MAP(CuDlgDomPropIntegrity)
    ON_WM_SIZE()
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIntegrity message handlers

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIntegrity message handlers

void CuDlgDomPropIntegrity::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropIntegrity::OnInitDialog() 
{
  CDialog::OnInitDialog();

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropIntegrity::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

  if (!IsWindow (m_edNumber.m_hWnd))
    return;

  CRect rDlg, r;
  GetClientRect (rDlg);

  // adjust width of integrity number
  m_edNumber.GetWindowRect (r);
  ScreenToClient (r);
  r.right = rDlg.right - r.left;
  m_edNumber.MoveWindow (r);

  // adjust width and height of statement
  m_edText.GetWindowRect (r);
  ScreenToClient (r);
  r.bottom = rDlg.bottom - r.left;
  r.right = rDlg.right - r.left;
  m_edText.MoveWindow (r);
}

LONG CuDlgDomPropIntegrity::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIntegrity::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  INTEGRITYPARAMS IntegrityParams;
  memset (&IntegrityParams, 0, sizeof (IntegrityParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get Detail Info
  //
  IntegrityParams.number = lpRecord->complimValue;
  x_strcpy ((char *)IntegrityParams.DBName, (const char *)lpRecord->extra);
  x_strcpy ((char *)IntegrityParams.TableName, RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->extra2)));
  x_strcpy ((char *)IntegrityParams.TableOwner, (const char *)lpRecord->tableOwner);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_INTEGRITY,
                               &IntegrityParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIntegrity tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_lNumber = IntegrityParams.number;
  m_Data.m_csText  = IntegrityParams.lpIntegrityText;

  // liberate detail structure
  FreeAttachedPointers (&IntegrityParams, OT_INTEGRITY);


  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIntegrity::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIntegrity") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIntegrity* pData = (CuDomPropDataIntegrity*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIntegrity)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIntegrity::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIntegrity* pData = new CuDomPropDataIntegrity(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIntegrity::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  CString csNumber;
  csNumber.Format("%ld", m_Data.m_lNumber);
  m_edNumber.SetWindowText(csNumber);
  m_edText.SetWindowText(m_Data.m_csText);
}

void CuDlgDomPropIntegrity::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edNumber.SetWindowText(_T(""));
  m_edText.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}