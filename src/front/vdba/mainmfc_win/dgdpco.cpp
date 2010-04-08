// DgDpCo.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpco.h"

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


IMPLEMENT_DYNCREATE(CuDlgDomPropConnection, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropConnection dialog


CuDlgDomPropConnection::CuDlgDomPropConnection()
	: CFormView(CuDlgDomPropConnection::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropConnection)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_REPLIC_CONNECTION);   // refresh base type for all about replication
}

CuDlgDomPropConnection::~CuDlgDomPropConnection()
{
}

void CuDlgDomPropConnection::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropConnection)
	DDX_Control(pDX, IDC_CHECK_LOCALDB, m_chkLocalDb);
	DDX_Control(pDX, IDC_DOMPROP_CONN_VNODE, m_edNode);
	DDX_Control(pDX, IDC_DOMPROP_CONN_TYPE, m_edType);
	DDX_Control(pDX, IDC_DOMPROP_CONN_NUM, m_edNumber);
	DDX_Control(pDX, IDC_DOMPROP_CONN_DESC, m_edDescription);
	DDX_Control(pDX, IDC_DOMPROP_CONN_DB, m_edDb);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropConnection, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropConnection)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropConnection diagnostics

#ifdef _DEBUG
void CuDlgDomPropConnection::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropConnection::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropConnection message handlers

void CuDlgDomPropConnection::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropConnection::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropConnection::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  REPLCONNECTPARAMS ConnectionParams;
  memset (&ConnectionParams, 0, sizeof (ConnectionParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  // Database name and Number
  x_strcpy ((char *)ConnectionParams.DBName, (const char *)lpRecord->extra);
  ConnectionParams.nNumber = lpRecord->complimValue;

  int dummySesHndl;

  // Get db owner
  int restype;
  UCHAR buf[MAXOBJECTNAME];
  UCHAR bufOwner[MAXOBJECTNAME];
  UCHAR udummy[] = "";
  int res = DOMGetObjectLimitedInfo(nNodeHandle,
                               lpRecord->extra,
                               udummy,
                               OT_DATABASE,
                               0,
                               NULL, // parentstrings,
                               TRUE,
                               &restype,
                               buf,
                               bufOwner,
                               buf);
  if (res != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data, refresh display and return
    CuDomPropDataConnection tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }

  // Get replicator version
  ConnectionParams.nReplicVersion = GetReplicInstallStatus(nNodeHandle, lpRecord->extra, bufOwner);
  int repObjType = GetRepObjectType4ll(OT_REPLIC_CONNECTION, ConnectionParams.nReplicVersion);
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               repObjType,
                               &ConnectionParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataConnection tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csDescription = ConnectionParams.szDescription;
  m_Data.m_nNumber       = ConnectionParams.nNumber;
  m_Data.m_csVnode       = ConnectionParams.szVNode;
  m_Data.m_bLocal        = ConnectionParams.bLocalDb;
  m_Data.m_csDb          = ConnectionParams.szDBName;
  m_Data.m_csType        = ConnectionParams.szShortDescrDBMSType; // New field in structure REPLCONNECTPARAMS PS :03/02/98

  // liberate detail structure
  FreeAttachedPointers (&ConnectionParams, repObjType);

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropConnection::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataConnection") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataConnection* pData = (CuDomPropDataConnection*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataConnection)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropConnection::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataConnection* pData = new CuDomPropDataConnection(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropConnection::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  //
  // Exclusively use member variables of m_Data for display refresh
  //
  CString csTxt;

  m_edDescription.SetWindowText(m_Data.m_csDescription);
  csTxt.Format(_T("%d"), m_Data.m_nNumber);
  m_edNumber.SetWindowText(csTxt);
  m_edNode.SetWindowText(m_Data.m_csVnode);
  m_edDb.SetWindowText(m_Data.m_csDb);
  m_chkLocalDb.SetCheck(m_Data.m_bLocal? 1: 0);
  m_edType.SetWindowText(m_Data.m_csType);
}

void CuDlgDomPropConnection::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edDescription.SetWindowText(_T(""));
  m_edNumber.SetWindowText(_T(""));
  m_edNode.SetWindowText(_T(""));
  m_edDb.SetWindowText(_T(""));
  m_chkLocalDb.SetCheck(0);
  m_edType.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}