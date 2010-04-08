// DgDpCd.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpcd.h"
#include "vtree.h"

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

CuDlgDomPropCdds::CuDlgDomPropCdds(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropCdds::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropCdds)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_REPLIC_CONNECTION);   // refresh base type for all about replication
}


void CuDlgDomPropCdds::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropCdds)
  DDX_Control(pDX, IDC_EDIT_NO, m_edNo);
  DDX_Control(pDX, IDC_EDIT_NAME, m_edName);
  DDX_Control(pDX, IDC_EDIT_ERRORMODE, m_edErrorMode);
  DDX_Control(pDX, IDC_EDIT_COLLISIONMODE, m_edCollisionMode);
  //}}AFX_DATA_MAP
}

void CuDlgDomPropCdds::AddCddsDbInfo (CuCddsDbInfo* pCddsDbInfo)
{
  ASSERT (pCddsDbInfo);
  BOOL  bSpecial = pCddsDbInfo->IsSpecialItem();

  int nCount, index;
  nCount = m_cListDbInfo.GetItemCount ();

  // fill columns
  if (bSpecial) {
    index  = m_cListDbInfo.InsertItem (nCount, "", 0);
    m_cListDbInfo.SetItemText (index, 1, (LPCTSTR)pCddsDbInfo->GetStrName());
  }
  else {
    CString csTxt;

    // db number
    csTxt.Format(_T("%d"), pCddsDbInfo->GetNumber());
    index  = m_cListDbInfo.InsertItem (nCount, (LPCTSTR)csTxt, 0);

    // vnode::database name
    csTxt = pCddsDbInfo->GetStrName() + _T("::") + pCddsDbInfo->GetDbName();
    m_cListDbInfo.SetItemText (index, 1, (LPCTSTR)csTxt);

    // Type
    m_cListDbInfo.SetItemText (index, 2, (LPCTSTR)pCddsDbInfo->GetTypeString());

    // server no
    csTxt.Format(_T("%d"), pCddsDbInfo->GetServerNo());
    m_cListDbInfo.SetItemText (index, 3, (LPCTSTR)csTxt);
  }

  // Attach pointer for future sort purposes
  m_cListDbInfo.SetItemData(index, (DWORD)pCddsDbInfo);
}

void CuDlgDomPropCdds::AddCddsPath (CuCddsPath* pCddsPath)
{
  ASSERT (pCddsPath);
  BOOL  bSpecial = pCddsPath->IsSpecialItem();

  int nCount, index;
  nCount = m_cListPath.GetItemCount ();

  // fill columns
  if (bSpecial)
    index  = m_cListPath.InsertItem (nCount, (LPCTSTR)pCddsPath->GetStrName(), 0);
  else {
    index  = m_cListPath.InsertItem (nCount, (LPCTSTR)pCddsPath->GetStrName(), 0);
    m_cListPath.SetItemText (index, 1, (LPCSTR)pCddsPath->GetLocalName());
    m_cListPath.SetItemText (index, 2, (LPCSTR)pCddsPath->GetTargetName());
  }

  // Attach pointer for future sort purposes
  m_cListPath.SetItemData(index, (DWORD)pCddsPath);
}

BEGIN_MESSAGE_MAP(CuDlgDomPropCdds, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropCdds)
    ON_WM_SIZE()
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropCdds message handlers

void CuDlgDomPropCdds::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropCdds::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // Columns for Database Information
  #define NBCOLUMNS_DBINFO  (4)
  VERIFY (m_cListDbInfo.SubclassDlgItem (IDC_MFC_LIST1, this));
  CHECKMARK_COL_DESCRIBE  aListDbInfoColumns[NBCOLUMNS_DBINFO] = {
    { _T(""),  FALSE, 30 + 16 }, // + 16 for image "DB #"
    { _T(""),  FALSE, 145 },
    { _T(""),  FALSE, 160 },
    { _T(""),  FALSE, -1 },
  };
  lstrcpy(aListDbInfoColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_DB_NO));
  lstrcpy(aListDbInfoColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_VNODE));
  lstrcpy(aListDbInfoColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_TYPE));
  lstrcpy(aListDbInfoColumns[3].szCaption, VDBA_MfcResourceString(IDS_TC_SVR_NO));

  InitializeColumns(m_cListDbInfo, aListDbInfoColumns, NBCOLUMNS_DBINFO);
  m_DbInfoImageList.Create(16, 16, TRUE, 1, 1);
  m_DbInfoImageList.AddIcon(IDI_TREE_STATIC_DATABASE);
  m_cListDbInfo.SetImageList (&m_DbInfoImageList, LVSIL_SMALL);

  // Columns for propagation path
  #define NBCOLUMNS_PATH  (3)
  VERIFY (m_cListPath.SubclassDlgItem (IDC_MFC_LIST2, this));
  CHECKMARK_COL_DESCRIBE  aListPathColumns[NBCOLUMNS_PATH] = {
    { _T(""),      FALSE, 145 + 16},    // + 16 for image
    { _T(""),      FALSE, 145 },
    { _T(""),      FALSE, 145 },
  };
  lstrcpy(aListPathColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_ORIGINATOR));
  lstrcpy(aListPathColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_LOCAL));
  lstrcpy(aListPathColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_TARGET));
  InitializeColumns(m_cListPath, aListPathColumns, NBCOLUMNS_PATH);
  m_PathImageList.Create(16, 16, TRUE, 1, 1);
  m_PathImageList.AddIcon(IDI_TREE_STATIC_REPLIC_CDDS_DETAIL);
  m_cListPath.SetImageList (&m_PathImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropCdds::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

  if (!IsWindow (m_cListPath.m_hWnd))
    return;

  CRect rDlg, r;
  GetClientRect (rDlg);

  // adjust width of db info
  m_cListDbInfo.GetWindowRect (r);
  ScreenToClient (r);
  r.right = rDlg.right - r.left;
  m_cListDbInfo.MoveWindow (r);

  // adjust width and height of propagation paths
  m_cListPath.GetWindowRect (r);
  ScreenToClient (r);
  r.bottom = rDlg.bottom - r.left;
  r.right = rDlg.right - r.left;
  m_cListPath.MoveWindow (r);
}

LONG CuDlgDomPropCdds::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropCdds::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_SETTING_CHANGE:
      VDBA_OnGeneralSettingChange(&m_cListPath);
      VDBA_OnGeneralSettingChange(&m_cListDbInfo);
      return 0L;

    default:
      return 0L;    // nothing to change on the display
  }

  ResetDisplay();
  //
  // Get info on the CDDS
  //
  REPCDDS cddsparams;
  memset (&cddsparams, 0, sizeof (cddsparams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  // Prepare some of the parameters
  x_strcpy ((char *)cddsparams.DBName,     (const char *)lpRecord->extra);      // Database
  cddsparams.cdds = atoi((const char *)lpRecord->objName);

  // Get db owner
  int restype;
  UCHAR buf[MAXOBJECTNAME];
  UCHAR udummy[] = "";
  int res = DOMGetObjectLimitedInfo(nNodeHandle,
                               cddsparams.DBName,
                               udummy,
                               OT_DATABASE,
                               0,
                               NULL, // parentstrings,
                               TRUE,
                               &restype,
                               buf,
                               cddsparams.DBOwnerName,
                               buf);
  if (res != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data, refresh display and return
    CuDomPropDataCdds tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }
  int nReplicVersion=GetReplicInstallStatus(nNodeHandle,
                                            cddsparams.DBName,
                                            cddsparams.DBOwnerName);
  int replicCddsType = GetRepObjectType4ll(OT_REPLIC_CDDS,nReplicVersion);
  if (replicCddsType != OT_REPLIC_CDDS_V11) {
    ASSERT (FALSE);

    // Reset m_Data, refresh display and return
    CuDomPropDataCdds tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }
           
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                             replicCddsType,
                             &cddsparams,
                             FALSE,
                             &dummySesHndl);
  cddsparams.iReplicType=nReplicVersion;
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data, refresh display and return
    CuDomPropDataCdds tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }
  if (nReplicVersion == REPLIC_NOINSTALL) {
    ASSERT (FALSE);

    // liberate detail structure
    FreeAttachedPointers (&cddsparams,  OT_REPLIC_CDDS);    // ??? replicCddsType

    // Reset m_Data, refresh display and return
    CuDomPropDataCdds tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  //
  //  Store useful data
  //
  m_Data.m_no             = cddsparams.cdds;
  m_Data.m_csName         = cddsparams.szCDDSName;
  m_Data.m_collisionMode  = cddsparams.collision_mode;
  m_Data.m_errorMode      = cddsparams.error_mode;

  m_Data.m_uaDbInfos.RemoveAll();
  m_Data.m_uaPropagationPaths.RemoveAll();
  LPOBJECTLIST ls;

  // Db Infos
  ls = cddsparams.connections;
  while (ls) {
    LPDD_CONNECTION lpDbInfo = (LPDD_CONNECTION)ls->lpObject;
    ASSERT (lpDbInfo);
    // Skip uninteresting items - filter on cdds number
    if (lpDbInfo->nServerNo != -1 && lpDbInfo->CDDSNo == (int)cddsparams.cdds ) {
      CuCddsDbInfo dbInfo((const char *)lpDbInfo->szVNode,
                          (const char *)lpDbInfo->szDBName,
                          lpDbInfo->dbNo,
                          lpDbInfo->nTypeRepl,
                          lpDbInfo->nServerNo);
      m_Data.m_uaDbInfos.Add(dbInfo);
    }
    ls = (LPOBJECTLIST)ls->lpNext;
  }

  // Propagation paths
  ls = cddsparams.distribution;
  while (ls) {
    LPDD_DISTRIBUTION lpPath = (LPDD_DISTRIBUTION)ls->lpObject;
    ASSERT (lpPath);
    if (lpPath->CDDSNo == (int)cddsparams.cdds ) {
      CuCddsPath path(NodeDbNameFromNo(lpPath->source, m_Data.m_uaDbInfos),
                      NodeDbNameFromNo(lpPath->localdb, m_Data.m_uaDbInfos),
                      NodeDbNameFromNo(lpPath->target, m_Data.m_uaDbInfos));
      m_Data.m_uaPropagationPaths.Add(path);
    }
    ls = (LPOBJECTLIST)ls->lpNext;
  }

  // liberate detail structure
  FreeAttachedPointers (&cddsparams,  OT_REPLIC_CDDS);    // ??? replicCddsType

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropCdds::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataCdds") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataCdds* pData = (CuDomPropDataCdds*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataCdds)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropCdds::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataCdds* pData = new CuDomPropDataCdds(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropCdds::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  CString csTxt;
  int cnt;
  int size;

  // Edits
  csTxt.Format(_T("%d"), m_Data.m_no);
  m_edNo.SetWindowText(csTxt);
  m_edName.SetWindowText(m_Data.m_csName);
  m_edCollisionMode.SetWindowText(GetCollisionModeString(m_Data.m_collisionMode));
  m_edErrorMode.SetWindowText(GetErrorModeString(m_Data.m_errorMode));

  // list of db infos
  m_cListDbInfo.DeleteAllItems();
  size = m_Data.m_uaDbInfos.GetCount();
  for (cnt = 0; cnt < size; cnt++) {
    CuCddsDbInfo* pDbInfo = m_Data.m_uaDbInfos[cnt];
    AddCddsDbInfo(pDbInfo);
  }
  // Initial sort if necessary
  // m_cListDbInfo.SortItems(CuCddsPath::CompareXyz, 0L);

  // list of paths
  m_cListPath.DeleteAllItems();
  size = m_Data.m_uaPropagationPaths.GetCount();
  for (cnt = 0; cnt < size; cnt++) {
    CuCddsPath* pPath = m_Data.m_uaPropagationPaths[cnt];
    AddCddsPath(pPath);
  }
  // Initial sort if necessary
  // m_cListPath.SortItems(CuCddsPath::CompareXyz, 0L);
}


CString CuDlgDomPropCdds::GetCollisionModeString(int collisionMode)
{
  switch (collisionMode) {
    case REPLIC_COLLISION_PASSIVE:
      return _T("Passive Detection");
    case REPLIC_COLLISION_ACTIVE:
      return _T("Active Detection");
    case REPLIC_COLLISION_BEGNIN:
      return _T("Benign Resolution");
    case REPLIC_COLLISION_PRIORITY:
      return _T("Priority Resolution");
    case REPLIC_COLLISION_LASTWRITE:
      return _T("Last Write Wins");
    default:
      ASSERT (FALSE);
      return _T("Unknown");
  }
}


CString CuDlgDomPropCdds::GetErrorModeString(int errorMode)
{
  switch (errorMode) {
    case REPLIC_ERROR_SKIP_TRANSACTION:
      return _T("Skip Transaction");
    case REPLIC_ERROR_SKIP_ROW:
      return _T("Skip Row");
    case REPLIC_ERROR_QUIET_CDDS:
      return _T("Quiet CDDS");
    case REPLIC_ERROR_QUIET_DATABASE:
      return _T("Quiet Database");
    case REPLIC_ERROR_QUIET_SERVER:
      return _T("Quiet Server");
    default:
      ASSERT (FALSE);
      return _T("Unknown");
  }
}

CString CuDlgDomPropCdds::NodeDbNameFromNo(int dbNo, CuArrayCddsDbInfos& refaDbInfos)
{
  ASSERT (dbNo >= 0);

  CString csTxt;
  CuCddsDbInfo* pCddsDbInfo = refaDbInfos.FindFromDbNo(dbNo);
  ASSERT (pCddsDbInfo);
  if(pCddsDbInfo) {
    csTxt = pCddsDbInfo->GetStrName() + _T("::") + pCddsDbInfo->GetDbName();
    return csTxt;
  }
  else {
    ASSERT (FALSE);
    return _T("Unknown");
  }
}

void CuDlgDomPropCdds::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edNo.SetWindowText(_T(""));
  m_edName.SetWindowText(_T(""));
  m_edErrorMode.SetWindowText(_T(""));
  m_edCollisionMode.SetWindowText(_T(""));

  m_cListPath.DeleteAllItems();
  m_cListDbInfo.DeleteAllItems();

  VDBA_OnGeneralSettingChange(&m_cListPath);
  VDBA_OnGeneralSettingChange(&m_cListDbInfo);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}