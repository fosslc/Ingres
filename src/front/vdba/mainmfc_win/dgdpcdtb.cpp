// DgDpCdTb.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpcdtb.h"
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

CuDlgDomPropCddsTables::CuDlgDomPropCddsTables(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropCddsTables::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropCddsTables)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
  m_bIgnoreNotif  = FALSE;
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_REPLIC_CONNECTION);   // refresh base type for all about replication
}


void CuDlgDomPropCddsTables::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropCddsTables)
  DDX_Control(pDX, IDC_STATIC_CDDS_TBL_COLUMNS, m_stColumns);
  //}}AFX_DATA_MAP
}

void CuDlgDomPropCddsTables::AddCddsTableCol (CuCddsTableCol* pCddsTableCol)
{
  ASSERT (pCddsTableCol);
  BOOL  bSpecial = pCddsTableCol->IsSpecialItem();

  CString csTxt;
  int nCount, index;
  nCount = m_cListTableCol.GetItemCount ();

  // column name
  index  = m_cListTableCol.InsertItem (nCount, (LPCTSTR)pCddsTableCol->GetStrName(), 0);

  // no other columns if special item
  if (!bSpecial) {
    // Key sequence
    if (pCddsTableCol->GetRepKey()) {
      csTxt.Format(_T("%d"), pCddsTableCol->GetRepKey());
      m_cListTableCol.SetItemText (index, 1, (LPCTSTR)csTxt);
    }

    // Replicated flag
    if (pCddsTableCol->IsReplicated())  // or : GetFlag(FLAG_CDDSTBLCOL_REPLIC))
      m_cListTableCol.SetCheck (index, 2, TRUE);
    else
      m_cListTableCol.SetCheck (index, 2, FALSE);
    // don't disable the item,
    // since derived class CuListCtrlCheckmarksWithSel takes care of it
    //m_cListTableCol.EnableCheck(index, cpt+3, FALSE);

  }

  // Attach pointer for future sort purposes
  m_cListTableCol.SetItemData(index, (DWORD)pCddsTableCol);
}

void CuDlgDomPropCddsTables::AddCddsTable (CuCddsTable* pCddsTable)
{
  ASSERT (pCddsTable);
  BOOL  bSpecial = pCddsTable->IsSpecialItem();

  int nCount, index;
  nCount = m_cListTable.GetItemCount ();

  // table name
  index  = m_cListTable.InsertItem (nCount, (LPCTSTR)pCddsTable->GetStrName(), 0);

  // no other columns if special item
  if (!bSpecial) {
    // schema
    m_cListTable.SetItemText (index, 1, (LPCTSTR)pCddsTable->GetSchema());

    // Support Objects and Activated flags
    if (pCddsTable->IsSupport())     // or : GetFlag(FLAG_CDDSTBL_SUPPORT))
      m_cListTable.SetCheck (index, 2, TRUE);
    else
      m_cListTable.SetCheck (index, 2, FALSE);
    if (pCddsTable->IsActivated())   // or : GetFlag(FLAG_CDDSTBL_ACTIVATED))
      m_cListTable.SetCheck (index, 3, TRUE);
    else
      m_cListTable.SetCheck (index, 3, FALSE);
    // don't disable the items,
    // since derived class CuListCtrlCheckmarksWithSel takes care of it
    //m_cListTable.EnableCheck(index, cpt+3, FALSE);

    // Lookup and Priority lookup tables
    m_cListTable.SetItemText (index, 4, (LPCTSTR)pCddsTable->GetLookup());
    m_cListTable.SetItemText (index, 5, (LPCTSTR)pCddsTable->GetPriority());
  }

  // Attach pointer for future sort purposes
  m_cListTable.SetItemData(index, (DWORD)pCddsTable);
}

BEGIN_MESSAGE_MAP(CuDlgDomPropCddsTables, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropCddsTables)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_MFC_LIST1, OnSelChangedListTables)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropCddsTables message handlers

void CuDlgDomPropCddsTables::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropCddsTables::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // Columns for Tables
  #define NBCOLUMNS_TABLE  (6)
  VERIFY (m_cListTable.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_TableImageList.Create(16, 16, TRUE, 1, 1);
  m_TableImageList.AddIcon(IDI_TREE_STATIC_TABLE);
  m_cListTable.SetImageList (&m_TableImageList, LVSIL_SMALL);
  m_TableImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListTable.SetCheckImageList(&m_TableImageCheck);
  CHECKMARK_COL_DESCRIBE  aListTableColumns[NBCOLUMNS_TABLE] = {
    { _T(""),  FALSE, 100 + 16},   // + 16 for image
    { _T(""),  FALSE, 60 },
    { _T(""),  TRUE,  -1 },
    { _T(""),  TRUE,  -1 },
    { _T(""),  FALSE, -1 },
    { _T(""),  FALSE, -1 },
  };
  lstrcpy(aListTableColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));
  lstrcpy(aListTableColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_SCHEMA));
  lstrcpy(aListTableColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_SUPP_OBJECTS));
  lstrcpy(aListTableColumns[3].szCaption, VDBA_MfcResourceString(IDS_TC_TBL_ACTIVATED));
  lstrcpy(aListTableColumns[4].szCaption, VDBA_MfcResourceString(IDS_TC_LOOKUP_TBL));
  lstrcpy(aListTableColumns[5].szCaption, VDBA_MfcResourceString(IDS_TC_PRIORITY_LOOKUP_TBL));
  InitializeColumns(m_cListTable, aListTableColumns, NBCOLUMNS_TABLE);

  // Columns for propagation path
  #define NBCOLUMNS_TBLCOL  (3)
  VERIFY (m_cListTableCol.SubclassDlgItem (IDC_MFC_LIST2, this));
  m_TableColImageList.Create(1, 18, TRUE, 1, 0);
  m_cListTableCol.SetImageList (&m_TableColImageList, LVSIL_SMALL);
  m_TableColImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListTableCol.SetCheckImageList(&m_TableColImageCheck);
  CHECKMARK_COL_DESCRIBE  aListTblColColumns[NBCOLUMNS_TBLCOL] = {
    { _T(""),  FALSE, 145 },
    { _T(""),  FALSE, -1 },
    { _T(""),  TRUE,  -1 },
  };
  lstrcpy(aListTblColColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));
  lstrcpy(aListTblColColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_KEY_SEQ));
  lstrcpy(aListTblColColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_REGISTERED_REPLIC));
  InitializeColumns(m_cListTableCol, aListTblColColumns, NBCOLUMNS_TBLCOL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropCddsTables::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

  if (!IsWindow (m_cListTable.m_hWnd))
    return;

  CRect rDlg, r;
  GetClientRect (rDlg);

  // adjust width of tables control
  m_cListTable.GetWindowRect (r);
  ScreenToClient (r);
  r.right = rDlg.right - r.left;
  m_cListTable.MoveWindow (r);

  // adjust width and height of columns control
  m_cListTableCol.GetWindowRect (r);
  ScreenToClient (r);
  r.bottom = rDlg.bottom - r.left;
  r.right = rDlg.right - r.left;
  m_cListTableCol.MoveWindow (r);
}

LONG CuDlgDomPropCddsTables::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropCddsTables::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      VDBA_OnGeneralSettingChange(&m_cListTable);
      VDBA_OnGeneralSettingChange(&m_cListTableCol);
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
    CuDomPropDataCddsTables tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }
  int nReplicVersion=GetReplicInstallStatus(nNodeHandle,
                                            cddsparams.DBName,
                                            cddsparams.DBOwnerName);
  int replicCddsTableType = GetRepObjectType4ll(OT_REPLIC_CDDS,nReplicVersion);
  if (replicCddsTableType != OT_REPLIC_CDDS_V11) {
    ASSERT (FALSE);

    // Reset m_Data, refresh display and return
    CuDomPropDataCddsTables tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }
           
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                             replicCddsTableType,
                             &cddsparams,
                             FALSE,
                             &dummySesHndl);
  cddsparams.iReplicType=nReplicVersion;
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data, refresh display and return
    CuDomPropDataCddsTables tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;
    RefreshDisplay();
    return 0L;
  }
  if (nReplicVersion == REPLIC_NOINSTALL) {
    ASSERT (FALSE);

    // liberate detail structure
    FreeAttachedPointers (&cddsparams,  OT_REPLIC_CDDS);    // ??? replicCddsTableType

    // Reset m_Data, refresh display and return
    CuDomPropDataCddsTables tempData;
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
  m_Data.m_uaCddsTables.RemoveAll();

  LPOBJECTLIST ls;

  ls = cddsparams.registered_tables;
  while (ls) {
    // the table
    LPDD_REGISTERED_TABLES lpTbl = (LPDD_REGISTERED_TABLES)ls->lpObject;
    ASSERT (lpTbl);
    // To keep the registred table only for
    // the current selected CDDS number. PS 22/04/99
    if (lpTbl->cdds == cddsparams.cdds) { 
        BOOL bSupportObjects = ( (lpTbl->table_created[0]=='T') ? TRUE: FALSE);
        BOOL bTableActivated = ( (lpTbl->rules_created[0]=='R') ? TRUE: FALSE);
        CuCddsTable  cddsTblInfo((const char *)lpTbl->tablename,
                                 (const char *)lpTbl->tableowner,
                                 bSupportObjects,
                                 bTableActivated,
                                 (const char *)lpTbl->cdds_lookup_table_v11,
                                 (const char *)lpTbl->priority_lookup_table_v11);
        // list of columns for that table
        cddsTblInfo.GetACols().RemoveAll();
        LPOBJECTLIST lpO;
        for (lpO = lpTbl->lpCols; lpO; lpO = (LPOBJECTLIST)lpO->lpNext){
          LPDD_REGISTERED_COLUMNS lpcol = (LPDD_REGISTERED_COLUMNS)lpO->lpObject;
          BOOL  bColumnRegistered = ( (lpcol->rep_colums[0] == 'R') ? TRUE: FALSE);
          CuCddsTableCol cddsColInfo((const char *)lpcol->columnname,
                                     lpcol->key_sequence,
                                     bColumnRegistered);
          cddsTblInfo.AddColumnInfo(cddsColInfo);
        }
        // Add table with columns description in it
        m_Data.m_uaCddsTables.Add(cddsTblInfo);
    }
    // next table
    ls = (LPOBJECTLIST)ls->lpNext;
  }

  // liberate detail structure
  FreeAttachedPointers (&cddsparams,  OT_REPLIC_CDDS);    // ??? replicCddsTableType

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropCddsTables::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataCddsTables") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataCddsTables* pData = (CuDomPropDataCddsTables*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataCddsTables)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropCddsTables::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // update current data instance with cur sel in list of tables
  if (m_cListTable.GetSelectedCount())
    m_Data.m_nCurSel = m_cListTable.GetNextItem(-1, LVNI_SELECTED);
  else
    m_Data.m_nCurSel = -1;

  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor

  CuDomPropDataCddsTables* pData = new CuDomPropDataCddsTables(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropCddsTables::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  CString csTxt;
  int cnt;
  int size;

  // list of tables
  IgnoreTblListNotif();
  m_cListTable.DeleteAllItems();
  size = m_Data.m_uaCddsTables.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuCddsTable* p = m_Data.m_uaCddsTables[cnt];
    AddCddsTable(p);
  }
  // Initial sort if necessary
  // m_cList.SortItems(CuCddsTable::CompareXyz, 0L);
  AcceptTblListNotif();

  // Initial selection if serialization said so
  if (m_Data.m_nCurSel != -1)
    m_cListTable.SetItemState (m_Data.m_nCurSel, LVIS_SELECTED, LVIS_SELECTED);

  // list of columns - OBSOLETE since integrated within each table
  /*
  m_cListTableCol.DeleteAllItems();
  size = m_Data.m_uaCddsTableCols.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuCddsTableCol* pTableCol = m_Data.m_uaCddsTableCols[cnt];
    AddCddsTableCol(pTableCol);
  }
  // Initial sort if necessary
  // m_cListTableCol.SortItems(CuCddsTableCol::CompareXyz, 0L);
  */
}


void CuDlgDomPropCddsTables::OnSelChangedListTables(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
  *pResult = 0;

  if (TblListNotifAcceptable()) {
    CuCddsTable* pCddsTable = (CuCddsTable*)m_cListTable.GetItemData(pNMListView->iItem);
    ASSERT (pCddsTable);
    
    // Update name of table
    CString cs;
    cs.Format("Columns for Table %s:", (LPCTSTR)pCddsTable->GetStrName());
    m_stColumns.SetWindowText(cs);
    
    // list of columns (for the current table)
    int cnt;
    int size;
    
    CuArrayCddsTableCols& rColList = pCddsTable->GetACols();
    m_cListTableCol.DeleteAllItems();
    size = rColList.GetCount();
    ASSERT (size > 0);
    for (cnt = 0; cnt < size; cnt++) {
      CuCddsTableCol* pTableCol = rColList[cnt];
      AddCddsTableCol(pTableCol);
    }
    // Initial sort if necessary
    // m_cListTableCol.SortItems(CuCddsTableCol::CompareXyz, 0L);
  }
}

void CuDlgDomPropCddsTables::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_cListTable.DeleteAllItems();
  m_cListTableCol.DeleteAllItems();

  VDBA_OnGeneralSettingChange(&m_cListTable);
  VDBA_OnGeneralSettingChange(&m_cListTableCol);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}