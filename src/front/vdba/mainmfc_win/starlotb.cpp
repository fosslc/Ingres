// StarLoTb.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "starlotb.h"
#include "staritem.h"

extern "C" {
#include "monitor.h"
#include "dbaset.h"       // ExecSQLImm
#include "dba.h"
#include "domdata.h"      // DOMGetFirst/Next
#include "dbaginfo.h"     // GetSession
#include "msghandl.h"     // IDS_I_LOCALNODE
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CString GetCDBName(int nNodeHandle, LPUCHAR DBName)
{
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
  aparentsTemp[0] = DBName;
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
    return _T("");
  }
  return CString(buf);
}


// C interface to "create table" old code in creattbl.c (dbadlg1)

extern "C" int DlgStarLocalTable(HWND hwnd, LPTABLEPARAMS lptbl)
{
  CuDlgStarLocalTable dlg;

  // preliminary checks
  ASSERT (lptbl->bDDB);
  if (!lptbl->bDDB)
    return IDCANCEL;

  // current node/database info - Must remove server class
  char nodeNameNoGW[MAXOBJECTNAME];
  x_strcpy (nodeNameNoGW, GetVirtNodeName(lptbl->hCurNode));
  RemoveGWNameFromString((LPUCHAR)nodeNameNoGW);
  dlg.m_CurrentNodeName = nodeNameNoGW;
  dlg.m_CurrentDbName   = lptbl->DBName;

  CString currentCDBName = GetCDBName(lptbl->hCurNode, lptbl->DBName);
  dlg.m_CurrentCDBName  = currentCDBName;

  // where do we create the table ?
  dlg.m_bCoordinator = lptbl->bCoordinator;
  if (lptbl->szLocalNodeName[0])
    dlg.m_LocalNodeName  = lptbl->szLocalNodeName;
  if (lptbl->szLocalDBName[0])
    dlg.m_LocalDbName     = lptbl->szLocalDBName;
  dlg.m_bLocalNodeIsLocal = lptbl->bLocalNodeIsLocal;

  // Which name for the table ?
  dlg.m_bDefaultName      = lptbl->bDefaultName;
  if (lptbl->bDefaultName)
    dlg.m_LocalTableName = lptbl->szCurrentName;
  else {
    ASSERT (lptbl->szLocalTblName[0]);
    dlg.m_LocalTableName = lptbl->szLocalTblName;
  }
  dlg.m_CurrentTblName = lptbl->szCurrentName;   // table name as entered in main dialog

  // call box
  int iret = dlg.DoModal();

  // update connection state on icons in mfc list of nodes
  UpdateNodeDisplay();

  // use data
  if (iret == IDOK) {

    // infos on node/database
    lptbl->bCoordinator = dlg.m_bCoordinator;
    if (lptbl->bCoordinator) {
      lptbl->szLocalNodeName[0] = '\0';
      lptbl->szLocalDBName[0] = '\0';
    }
    else {
      ASSERT (!dlg.m_LocalNodeName.IsEmpty());
      x_strcpy((char *)lptbl->szLocalNodeName, (LPCTSTR)dlg.m_LocalNodeName);
      lptbl->bLocalNodeIsLocal = dlg.m_bLocalNodeIsLocal;
      ASSERT (!dlg.m_LocalDbName.IsEmpty());
      x_strcpy((char *)lptbl->szLocalDBName, (LPCTSTR)dlg.m_LocalDbName);
    }

    // infos on name
    lptbl->bDefaultName = dlg.m_bDefaultName;
    if (lptbl->bDefaultName)
      lptbl->szLocalTblName[0] = '\0';
    else {
      ASSERT (!dlg.m_LocalTableName.IsEmpty());
      x_strcpy((char *)lptbl->szLocalTblName, (LPCTSTR)dlg.m_LocalTableName);
    }
  }

  return iret;
}


/////////////////////////////////////////////////////////////////////////////
// CuDlgStarLocalTable dialog


CuDlgStarLocalTable::CuDlgStarLocalTable(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStarLocalTable::IDD, pParent)
{
  m_CurrentNodeName = "";
  m_CurrentDbName   = "";
  m_CurrentCDBName  = "";
  m_CurrentTblName  = "";

  m_LocalNodeName   = "";
  m_LocalDbName     = "";
  m_LocalTableName  = "";
  m_bLocalNodeIsLocal = FALSE;

  m_pTreeGlobalData = NULL;

	m_bDefaultName    = TRUE;
	m_bCoordinator    = TRUE;

  m_bStateDisabled  = FALSE;

  m_bIgnorePreventTreeOperation = FALSE;

	//{{AFX_DATA_INIT(CuDlgStarLocalTable)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CuDlgStarLocalTable::~CuDlgStarLocalTable()
{
  if (m_pTreeGlobalData)
    delete m_pTreeGlobalData;
}



void CuDlgStarLocalTable::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStarLocalTable)
	DDX_Control(pDX, IDC_CHECK_USEDEFAULTNAME, m_CheckDefaultName);
	DDX_Control(pDX, IDC_RADIO_COORDINATOR, m_RadCoordinator);
	DDX_Control(pDX, IDC_TREE, m_Tree);
	DDX_Control(pDX, IDC_LOCALNAME, m_EditName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStarLocalTable, CDialog)
	//{{AFX_MSG_MAP(CuDlgStarLocalTable)
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE, OnItemexpandingTree)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnSelchangedTree)
	ON_BN_CLICKED(IDC_RADIO_COORDINATOR, OnRadioCoordinator)
	ON_BN_CLICKED(IDC_RADIO_CUSTOM, OnRadioCustom)
	ON_BN_CLICKED(IDC_CHECK_USEDEFAULTNAME, OnCheckUsedefaultname)
	ON_EN_CHANGE(IDC_LOCALNAME, OnChangeLocalname)
	ON_NOTIFY(TVN_SELCHANGING, IDC_TREE, OnSelchangingTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarLocalTable message handlers

BOOL CuDlgStarLocalTable::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // check input parameters
  ASSERT (m_CurrentNodeName != "");
  ASSERT (m_CurrentDbName   != "");
  ASSERT (m_CurrentCDBName  != "");

  // initialize local table options related to node/database
  if (!m_bCoordinator) {
    ASSERT (!m_LocalNodeName.IsEmpty());
    ASSERT (!m_LocalDbName.IsEmpty());
  }
  if (m_bCoordinator)
    CheckRadioButton(IDC_RADIO_COORDINATOR, IDC_RADIO_CUSTOM, IDC_RADIO_COORDINATOR);
  else
    CheckRadioButton(IDC_RADIO_COORDINATOR, IDC_RADIO_CUSTOM, IDC_RADIO_CUSTOM);

  // initialize local table options related to name
	if (m_bDefaultName)
  	m_CheckDefaultName.SetCheck(1);
  else
  	m_CheckDefaultName.SetCheck(0);
  if (!m_bDefaultName)
    ASSERT (!m_LocalTableName.IsEmpty());
  m_EditName.SetWindowText(m_LocalTableName);

  // disable controls that need to
  ManageControlsState();

  //
  // Create root branches for source
  //
  int hNode = 0;    // null node handle for nodes list pick
  m_pTreeGlobalData = new CTreeGlobalData (&m_Tree);
  // prepare variables for call
  int   iobjecttypeParent   = NULL;
  void *pstructParent = NULL;
  int   iobjecttypeReq      = OT_NODE;
  int   reqSize             = GetMonInfoStructSize(iobjecttypeReq);
	CString csItemName;
  
  // allocate one requested structure
  void *pStructReq          = new char[reqSize];
  memset (pStructReq, 0, reqSize);
  
  // call GetFirstMonInfo
  int res = GetFirstMonInfo(hNode,              // hNode
                            iobjecttypeParent,
                            pstructParent,
                            iobjecttypeReq,
                            pStructReq,
                            NULL);                // no filter
  ASSERT (res == RES_SUCCESS);

  // loop
  while (res != RES_ENDOFDATA) {
    // Generate display name
    char	name[MAXMONINFONAME];
    name[0] = '\0';
    GetMonInfoName(hNode, iobjecttypeReq, pStructReq, name, sizeof(name));
    ASSERT (name[0]);
    csItemName = name;

    // arrange node name
    if (IsLocalNodeName(csItemName, FALSE)) {
      CString HostName = GetLocalHostName();
      csItemName = HostName + " " + csItemName;
    }
      
    // Insert an item with the brand new name
    CTreeItemData ItemData(pStructReq, reqSize, hNode, csItemName);   // can be local since will be duplicated by AllocateChildItem
    CTreeItemStar *pNewItem = NULL;
    pNewItem = new CuTreeStar4LocalTblServer (m_pTreeGlobalData, &ItemData);
    delete pStructReq;                                    // free structure that has been duplicated.
    pStructReq = NULL;    // check not used anymore
    if (!pNewItem)
      return FALSE;

    ASSERT (iobjecttypeReq == pNewItem->GetType());       // Check types compatibility
    HTREEITEM hNewChild = pNewItem->CreateTreeLine();     // At root level, at last position
    ASSERT (hNewChild);
    if (!hNewChild)
      return FALSE;
    pNewItem->ChangeImageId(pNewItem->GetCustomImageId(), hNewChild);        // update image if necessary

    // call GetNextMonInfo()
    pStructReq = new char[reqSize];
    memset (pStructReq, 0, reqSize);
    res = GetNextMonInfo(pStructReq);
  }

  // free last allocated structure, not used
  delete pStructReq;

  PreselectTreeItem();
  PushHelpId (IDHELP_IDD_STAR_TABLECREATE);
  return TRUE;    // we let the default procedure set the focus
}

void CuDlgStarLocalTable::OnOK() 
{
  //
  // Pick data about the table/view to be registered as link
  //

  // infos on node/database
  m_bCoordinator = m_RadCoordinator.GetCheck();
  if (!m_bCoordinator)
    UpdateCustomInfo(TRUE);

  // infos on object name
  m_bDefaultName = m_CheckDefaultName.GetCheck();
  if (!m_bDefaultName)
    m_EditName.GetWindowText(m_LocalTableName);

	CDialog::OnOK();
}

void CuDlgStarLocalTable::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
  if (m_pTreeGlobalData)
    m_pTreeGlobalData->FreeAllTreeItemsData();
}

void CuDlgStarLocalTable::OnItemexpandingTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
  *pResult = 0;     // default to allow expanding

  // Prevent expansion if necessary
  if (MustPreventTreeOperation()) {
    MessageBeep(MB_ICONEXCLAMATION);
    *pResult = 1;
    return;
  }

  NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

  if (pNMTreeView->action == TVE_EXPAND) {
    HTREEITEM hItem = pNMTreeView->itemNew.hItem;
    CTreeItem *pItem;
    pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
    if (pItem) {
      if (!pItem->IsAlreadyExpanded()) {
        if (pItem->CreateSubBranches(hItem))
          pItem->SetAlreadyExpanded(TRUE);
        else
          *pResult = 1;     // prevent expanding
      }
    }
  }
}

void CuDlgStarLocalTable::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	*pResult = 0;   // accept selchange

  ManageControlsState();
}

void CuDlgStarLocalTable::ManageControlsState()
{
  // anti-reentrance code
  if (IsControlsStateManagementDisabled())
    return;
  
  // state of edit control for default name
  if (m_CheckDefaultName.GetCheck()) {
    m_EditName.EnableWindow(FALSE);               // not editable
    DisableControlsState();
    m_EditName.SetWindowText(m_CurrentTblName);   // reset to table name as a reminder
    EnableControlsState();
  }
  else
    m_EditName.EnableWindow(TRUE);                // editable

  // State of ok button
  BOOL bOk = TRUE;
  if (!m_CheckDefaultName.GetCheck())
    if (m_EditName.GetWindowTextLength() == 0)
      bOk = FALSE;
  if (!m_RadCoordinator.GetCheck()) {
    // selection on tree must be on a database
    HTREEITEM hItem = m_Tree.GetSelectedItem();
    if (hItem) {
      CTreeItem *pItem;
      pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
      ASSERT (pItem);
      if (pItem) {
        // check runtime class
        if (!pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4LocalTblDatabase)))
          bOk = FALSE;
        // check item is not "< error >" or "< No xyz >"
        if (pItem->IsSpecialItem())
          bOk = FALSE;
      }
      else
        bOk = FALSE;   // no item data (asserted)
    }
    else
      bOk = FALSE;    // no selection
  }
  GetDlgItem(IDOK)->EnableWindow(bOk);
}

void CuDlgStarLocalTable::OnRadioCoordinator() 
{
  UpdateCustomInfo(FALSE);
  m_bCoordinator = m_RadCoordinator.GetCheck();
  PreselectTreeItem();
  ManageControlsState();
}

void CuDlgStarLocalTable::OnRadioCustom() 
{
  m_bCoordinator = m_RadCoordinator.GetCheck();
  PreselectTreeItem();
  ManageControlsState();
}

void CuDlgStarLocalTable::OnCheckUsedefaultname() 
{
  ManageControlsState();
}

void CuDlgStarLocalTable::OnChangeLocalname() 
{
  ManageControlsState();
}

void CuDlgStarLocalTable::OnSelchangingTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

  // Prevent expansion if necessary
  if (MustPreventTreeOperation()) {
    MessageBeep(MB_ICONEXCLAMATION);
    *pResult = 1;
    return;
  }

	*pResult = 0;
}

BOOL CuDlgStarLocalTable::MustPreventTreeOperation()
{
  if (m_bIgnorePreventTreeOperation)
    return FALSE;

  if (m_bCoordinator)
    return TRUE;

  // defaults to no prevention
  return FALSE;
}

void CuDlgStarLocalTable::PreselectTreeItem()
{
  // expand tree up to the "good" object (cdb, or node/Db according to m_bCoordinator)
  IgnorePreventTreeOperation();

  // 1) find node branch
  HTREEITEM hTreeItemDDNode;
  for (hTreeItemDDNode = m_Tree.GetRootItem();
       hTreeItemDDNode;
       hTreeItemDDNode = m_Tree.GetNextSiblingItem(hTreeItemDDNode)) {
    CTreeItemStar* pItem = (CTreeItemStar*)m_Tree.GetItemData(hTreeItemDDNode);
    ASSERT (pItem);
    void *pStructReq = pItem->GetPTreeItemData()->GetDataPtr();
    ASSERT (pStructReq);

    char	name[MAXMONINFONAME];
    name[0] = '\0';
    GetMonInfoName(0, OT_NODE, pStructReq, name, sizeof(name));
    ASSERT (name[0]);

    if (m_bCoordinator) {
      if (m_CurrentNodeName.CompareNoCase((LPCTSTR)name) == 0)
        break;    // found
    }
    else {
      CString localNodeName;
      if (m_bLocalNodeIsLocal)
        localNodeName.LoadString(IDS_I_LOCALNODE);
      else
        localNodeName = m_LocalNodeName;
      if (localNodeName.CompareNoCase((LPCTSTR)name) == 0)
        break;    // found
    }
  }

  // no selection acceptable if custom was not complete
  if(m_bCoordinator)
    ASSERT (hTreeItemDDNode);
  if (!hTreeItemDDNode) {
    m_Tree.SelectItem(m_Tree.GetRootItem());
    return;
  }
  m_Tree.SelectItem(hTreeItemDDNode);
  m_Tree.Expand(hTreeItemDDNode, TVE_EXPAND);

  // 2) find database branch
  HTREEITEM hTreeItemDB = m_Tree.GetChildItem(hTreeItemDDNode);
  ASSERT (hTreeItemDB);
  do {
    CTreeItemStar* pItem = (CTreeItemStar*)m_Tree.GetItemData(hTreeItemDB);
    ASSERT (pItem);
    LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
    ASSERT (pStar);
    int compare;
    if (m_bCoordinator)
      compare = m_CurrentCDBName.CompareNoCase((LPCTSTR)pStar->objName);
    else
      compare = m_LocalDbName.CompareNoCase((LPCTSTR)pStar->objName);
    if (compare == 0) {
      m_Tree.SelectItem(hTreeItemDB);
      break;
    }
  }
  while ( (hTreeItemDB = m_Tree.GetNextSiblingItem(hTreeItemDB)) != NULL );

  // no selection acceptable if custom was not complete
  if(m_bCoordinator)
    ASSERT (hTreeItemDB);
  if (!hTreeItemDB)
    return;

  // check expansion has been successful
  HTREEITEM hSelectedItem = m_Tree.GetSelectedItem();
  ASSERT (hSelectedItem);
  CTreeItem *pItem;
  pItem = (CTreeItem *)m_Tree.GetItemData(hSelectedItem);
  ASSERT (pItem);
  if (pItem)
    ASSERT (pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4LocalTblDatabase)));

  AuthorizePreventTreeOperation();
}

void CuDlgStarLocalTable::UpdateCustomInfo(BOOL bMustFit)
{
  HTREEITEM hItem = m_Tree.GetSelectedItem();
  ASSERT (hItem);
  CTreeItem *pItem;
  pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
  ASSERT (pItem);
  if (!pItem)
    return;

  BOOL bFits = FALSE;
  bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4LocalTblDatabase));
  if (bMustFit)
    ASSERT (bFits);
  if (!bFits) {
    m_bLocalNodeIsLocal = FALSE;
    m_LocalNodeName = "";
    m_LocalDbName   = "";
    return;
  }
  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);
  if (IsLocalNodeName((LPCTSTR)pStar->NodeName, FALSE)) {
    m_bLocalNodeIsLocal = TRUE;
    m_LocalNodeName = GetLocalHostName();
  }
  else {
    m_bLocalNodeIsLocal = FALSE;
    m_LocalNodeName = pStar->NodeName;
  }
  m_LocalDbName   = pStar->objName;
}
