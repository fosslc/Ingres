// DgDpDd.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpdd.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropDDb, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDDb dialog


CuDlgDomPropDDb::CuDlgDomPropDDb()
	: CFormView(CuDlgDomPropDDb::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropDDb)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_DATABASE);
}

CuDlgDomPropDDb::~CuDlgDomPropDDb()
{
}

void CuDlgDomPropDDb::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropDDb)
	DDX_Control(pDX, IDC_DOMPROP_DB_CDB, m_edCoordinator);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCCKP, m_edLocChk);
	DDX_Control(pDX, IDC_DOMPROP_DB_OWNER, m_edOwner);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCSORT, m_edLocSrt);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCJNL, m_edLocJnl);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCDMP, m_edLocDmp);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCDB, m_edLocDb);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropDDb, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropDDb)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDDb diagnostics

#ifdef _DEBUG
void CuDlgDomPropDDb::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropDDb::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDDb message handlers

void CuDlgDomPropDDb::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropDDb::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropDDb::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  DATABASEPARAMS DbParams;
  memset (&DbParams, 0, sizeof (DbParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get Detail Info
  //
  lstrcpy ((LPTSTR)DbParams.objectname, (LPCTSTR)lpRecord->objName);
  DbParams.DbType = lpRecord->parentDbType;
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_DATABASE,
                               &DbParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataDDb tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csOwner  = DbParams.szOwner ;
  m_Data.m_csLocDb  = DbParams.szDbDev ;
  m_Data.m_csLocChk = DbParams.szChkpDev;
  m_Data.m_csLocJnl = DbParams.szJrnlDev;
  m_Data.m_csLocDmp = DbParams.szDmpDev;
  m_Data.m_csLocSrt = DbParams.szSortDev;

  // liberate detail structure
  FreeAttachedPointers (&DbParams, OT_DATABASE);

  //
  // Get name of Coordinator Database
  //
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
  aparentsTemp[0] = lpRecord->objName;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  iret =  DOMGetFirstRelObject(nNodeHandle,
                               OTR_CDB,
                               1,                           // level,
                               aparentsTemp,                // Temp mandatory!
                               TRUE,                        // bwithsystem
                               NULL,                        // base owner
                               NULL,                        // other owner
                               aparentsResult,
                               buf,
                               bufOwner,
                               bufComplim);
  if (iret != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataDDb tempData;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }
  m_Data.m_csCoordinator = buf;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropDDb::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataDDb") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataDDb* pData = (CuDomPropDataDDb*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataDDb)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropDDb::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataDDb* pData = new CuDomPropDataDDb(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropDDb::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edOwner.SetWindowText(m_Data.m_csOwner);
  m_edCoordinator.SetWindowText(m_Data.m_csCoordinator);

  m_edLocDb.SetWindowText(m_Data.m_csLocDb);
  m_edLocChk.SetWindowText(m_Data.m_csLocChk);
  m_edLocJnl.SetWindowText(m_Data.m_csLocJnl);
  m_edLocDmp.SetWindowText(m_Data.m_csLocDmp);
  m_edLocSrt.SetWindowText(m_Data.m_csLocSrt);
}

void CuDlgDomPropDDb::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edOwner.SetWindowText(_T(""));
  m_edCoordinator.SetWindowText(_T(""));

  m_edLocDb.SetWindowText (_T(""));
  m_edLocChk.SetWindowText(_T(""));
  m_edLocJnl.SetWindowText(_T(""));
  m_edLocDmp.SetWindowText(_T(""));
  m_edLocSrt.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}