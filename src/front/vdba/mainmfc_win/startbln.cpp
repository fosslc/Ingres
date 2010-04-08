/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Visual DBA
**
**  Source : StarTbLn.cpp
**
**  History :
**   06-jun-2001 (noifr01)
**     (bug 104861) embedded the nodename in single quotes in the
**     "register as link" syntax
**   02-Oct-2002 (schph01)
**     bug 108841 Verify SQL error and add warning when the "register as link"
**     command failed.
**   02-feb-2004 (somsa01)
**     Updated to "typecast" UCHAR into CString before concatenation.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "starutil.h"   // LINK_XYZ
#include "startbln.h"
#include "staritem.h"
#include "dgerrh.h"     // MessageWithHistoryButton

extern "C" {
#include "monitor.h"
#include "dbaset.h"       // ExecSQLImm
#include "dba.h"
#include "domdata.h"      // DOMGetFirst/Next
#include "dbaginfo.h"     // GetSession

};


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LAYOUT_NUMBER   2

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarTblRegister dialog


CuDlgStarTblRegister::CuDlgStarTblRegister(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStarTblRegister::IDD, pParent)
{
  m_destNodeName  = "";
  m_destDbName    = "";
  m_bDragDrop     = FALSE;
  m_linkType      = 0;
  m_srcNodeName   = "";
  m_srcDbName     = "";
  m_srcObjName    = "";
  m_srcOwnerName  = "";

  m_pTreeGlobalData = NULL;

  m_bPreventTreeOperation = FALSE;

	//{{AFX_DATA_INIT(CuDlgStarTblRegister)
	//}}AFX_DATA_INIT
}


CuDlgStarTblRegister::~CuDlgStarTblRegister()
{
  if (m_pTreeGlobalData)
    delete m_pTreeGlobalData;
}

void CuDlgStarTblRegister::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStarTblRegister)
	DDX_Control(pDX, IDC_BTN_RESET, m_BtnReset);
	DDX_Control(pDX, IDC_STATIC_SOURCE, m_GroupboxSource);
	DDX_Control(pDX, IDC_TREE, m_Tree);
	DDX_Control(pDX, IDC_COLUMNSNAMES, m_Columns);
	DDX_Control(pDX, IDC_STATIC_DISTNAME, m_StaticName);
	DDX_Control(pDX, IDC_DISTNAME, m_EditName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStarTblRegister, CDialog)
	//{{AFX_MSG_MAP(CuDlgStarTblRegister)
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE, OnItemexpandingTree)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnSelchangedTree)
	ON_EN_CHANGE(IDC_DISTNAME, OnChangeDistname)
	ON_BN_CLICKED(IDC_BTN_RESET, OnBtnReset)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_COLUMNSNAMES, OnBeginEditColumnName)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_COLUMNSNAMES, OnEndEditColumnName)
	ON_NOTIFY(TVN_SELCHANGING, IDC_TREE, OnSelchangingTree)
	ON_BN_CLICKED(IDC_BTN_NAME, OnBtnName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarTblRegister message handlers

void CuDlgStarTblRegister::OnOK() 
{
  //
  // Pick data about the table/view to be registered as link
  //
  HTREEITEM hItem = m_Tree.GetSelectedItem();
  ASSERT (hItem);
  CTreeItem *pItem;
  pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
  ASSERT (pItem);
  if (!pItem)
    return;

  // Triple-check item validity - could be removed since OK grayed when necessary
  BOOL bFits = FALSE;
  switch (m_linkType) {
    case LINK_TABLE:
      bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4TblTable));
      break;
    case LINK_VIEW:
      bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4ViewView));
      break;
    default:
      ASSERT (FALSE);
      return;
  }
  ASSERT (bFits);
  if (!bFits)
    return;
  // end of Triple-check item validity

  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  //
  // Build sql statement
  //
  CString stmt = "register ";

  // object name
  CString objName;
  m_EditName.GetWindowText(objName);
  stmt += QuoteIfNeeded(objName);
  stmt += " ";

  // columns names
  // a) check whether at least one column has been renamed
  BOOL bRenameCol = FALSE;
  int count = m_Columns.GetItemCount();
  for (int index = 0; index < count; index++) {
    if (m_Columns.GetItemText(index, 0) != m_Columns.GetItemText(index, 1) ) {
      bRenameCol = TRUE;
      break;
    }
  }
  // b) if necessary, list columns names
  if (bRenameCol) {
    stmt += "( ";
    for (int index = 0; index < count; index++) {
      stmt += QuoteIfNeeded(m_Columns.GetItemText(index, 0));
      if (index < count - 1)
        stmt += " , ";
      else
        stmt += " ) ";
    }
  }

  // syntax
  stmt += "as link from ";

  // local owner/ object name
  stmt += QuoteIfNeeded((LPCTSTR)pStar->ownerName);
  stmt += ".";
  stmt += QuoteIfNeeded(RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(pStar->objName)));
  stmt += " ";

  // server class
  UCHAR gwClassName[MAXOBJECTNAME];
  BOOL bHasGW = GetGWClassNameFromString((LPUCHAR)(LPCTSTR)pStar->NodeName, gwClassName);
  UCHAR nodeNameNoGW[MAXOBJECTNAME];
  x_strcpy ((char*)nodeNameNoGW, (LPCTSTR)pStar->NodeName);
  if (bHasGW)
    RemoveGWNameFromString(nodeNameNoGW);

  // node + database  ( syntax says both )
  stmt += "with node = '";
  if (IsLocalNodeName((const char*)nodeNameNoGW, FALSE))
    stmt += CString(GetLocalHostName());
  else
    stmt += CString(nodeNameNoGW);
  stmt += "', database = '";
  stmt += CString(pStar->parent0);
  stmt += "'";

  // Server Class:
  if (bHasGW) {
    stmt += " ,dbms = ";
    stmt += CString(gwClassName);
  }

  // Get a session on the ddb
  int ilocsession;
  CString connectname;
  // Getsession accepts (local) as a node name
  connectname.Format("%s::%s", (LPCTSTR)m_destNodeName, (LPCTSTR)m_destDbName);
  CWaitCursor glassHour;
  int iret = Getsession((LPUCHAR)(LPCSTR)connectname, SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS) {
    MessageWithHistoryButton(m_hWnd, VDBA_MfcResourceString (IDS_E_GET_SESSION));//_T("Cannot get a Session")
    return;
  }

  // execute the statement
  iret=ExecSQLImm((LPUCHAR)(LPCSTR)stmt, FALSE, NULL, NULL, NULL);

  // release the session
  ReleaseSession(ilocsession, iret==RES_SUCCESS ? RELEASE_COMMIT : RELEASE_ROLLBACK);
  if (iret !=RES_SUCCESS) {
    CString csMessError;
    csMessError = VDBA_MfcResourceString (IDS_E_REGISTER_AS_LINK);
    csMessError += VerifyStarSqlError(m_destNodeName,(const char*)nodeNameNoGW);
    MessageWithHistoryButton(m_hWnd,csMessError );//_T("Cannot Register as Link")
    return;
  }

  CDialog::OnOK();
}

BOOL CuDlgStarTblRegister::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // check input parameters
  ASSERT (m_destNodeName != "");
  ASSERT (m_destDbName != "");
  ASSERT (m_linkType == LINK_TABLE || m_linkType == LINK_VIEW);
  if (m_bDragDrop) {
    ASSERT (m_srcNodeName != "");
    ASSERT (m_srcDbName   != "");
    ASSERT (m_srcObjName  != "");
  }

  // initialize captions
  switch (m_linkType) {
    case LINK_TABLE:
      SetWindowText("Register Table as Link");
      m_StaticName.SetWindowText("Distributed Table Name");
      m_GroupboxSource.SetWindowText("&Source [Node][Database]Table");
      PushHelpId (IDHELP_IDD_STAR_TABLEREGISTER);
      break;
    case LINK_VIEW:
      SetWindowText("Register View as Link");
      m_StaticName.SetWindowText("Distributed View Name");
      m_GroupboxSource.SetWindowText("&Source [Node][Database]View");
      PushHelpId (IDHELP_IDD_STAR_VIEWREGISTER);
      break;
    default:
      ASSERT (FALSE);
      return TRUE;
  }

  // initialize columns control
  LV_COLUMN lvcolumn;
  TCHAR strHeader[LAYOUT_NUMBER][32];
//  = 
//  {
//      _T("Distributed Name"),
//      _T("Local Name")
//  };
  lstrcpy(strHeader[0], VDBA_MfcResourceString(IDS_TC_DISTRIB_NAME));
  lstrcpy(strHeader[1], VDBA_MfcResourceString(IDS_TC_LOCAL_NAME));
  int i, hWidth   [LAYOUT_NUMBER] = {100, 70};
  //
  // Set the number of columns: LAYOUT_NUMBER
  //
  memset (&lvcolumn, 0, sizeof (LV_COLUMN));
  lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
  for (i=0; i<LAYOUT_NUMBER; i++)
  {
    //CString strHeader;
    //strHeader.LoadString (strHeaderID[i]);
    lvcolumn.fmt = LVCFMT_LEFT;
    lvcolumn.pszText = (LPTSTR)(LPCTSTR)strHeader[i];
    lvcolumn.iSubItem = i;
    lvcolumn.cx = hWidth[i];
    m_Columns.InsertColumn (i, &lvcolumn); 
  }

  // disable controls that need to
  m_EditName.EnableWindow(FALSE);
  m_BtnReset.EnableWindow(FALSE);
  GetDlgItem(IDOK)->EnableWindow(FALSE);

  //
  // Prepare source server class info if drag-dropping
  //
  // server class
  UCHAR gwClassName[MAXOBJECTNAME];
  UCHAR nodeNameNoGW[MAXOBJECTNAME];
  BOOL bHasGW;
  if (m_bDragDrop) {
    // First, remove user name from source node name.
    UCHAR nodeNameNoUser[MAXOBJECTNAME];
    x_strcpy((char*)nodeNameNoUser, (LPCTSTR)m_srcNodeName);
    RemoveConnectUserFromString(nodeNameNoUser);
    m_srcNodeName = nodeNameNoUser;

    bHasGW = GetGWClassNameFromString((LPUCHAR)(LPCTSTR)m_srcNodeName, gwClassName);
    x_strcpy ((char*)nodeNameNoGW, (LPCTSTR)m_srcNodeName);
    if (bHasGW)
      RemoveGWNameFromString(nodeNameNoGW);
  }
  else {
    gwClassName[0] = '\0';
    nodeNameNoGW[0] = '\0';
    bHasGW = FALSE;
  }

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
  HTREEITEM hTreeItemDDNode = NULL; // dragdrop
  
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

    // Insert an item with the brand new name
    CTreeItemData ItemData(pStructReq, reqSize, hNode, csItemName);   // can be local since will be duplicated by AllocateChildItem
    CTreeItemStar *pNewItem = NULL;
    switch (m_linkType) {
      case LINK_TABLE:
        pNewItem = new CuTreeStar4TblServer (m_pTreeGlobalData, &ItemData);
        break;
      case LINK_VIEW:
        pNewItem = new CuTreeStar4ViewServer (m_pTreeGlobalData, &ItemData);
        break;
      default:
        ASSERT(FALSE);
        return TRUE;
    }
    if (!pNewItem) {
      delete pStructReq;
      return FALSE;
    }

    ASSERT (iobjecttypeReq == pNewItem->GetType());       // Check types compatibility
    HTREEITEM hNewChild = pNewItem->CreateTreeLine();     // At root level, at last position
    ASSERT (hNewChild);
    if (!hNewChild) {
      delete pStructReq;
      return FALSE;
    }
    pNewItem->ChangeImageId(pNewItem->GetCustomImageId(), hNewChild);        // update image if necessary

    // flag src node name for drag drop
    if (m_bDragDrop) {
      if (bHasGW) {
        ASSERT (nodeNameNoGW[0]);
        if (x_strcmp (name, (LPCTSTR)nodeNameNoGW) == 0)
          hTreeItemDDNode = hNewChild;
      }
      else {
        if (name == m_srcNodeName)
          hTreeItemDDNode = hNewChild;
      }
    }

    // call GetNextMonInfo()
    delete pStructReq;  // free structure that has been duplicated.
    pStructReq = new char[reqSize];
    memset (pStructReq, 0, reqSize);
    res = GetNextMonInfo(pStructReq);
  }

  // free last allocated structure, not used
  delete pStructReq;

  //
  // Special for drag-drop operation, if source with gateway specified
  //
  if (m_bDragDrop && bHasGW) {
    // 1) expand node branch
    ASSERT (hTreeItemDDNode);
    m_Tree.SelectItem(hTreeItemDDNode);
    m_Tree.Expand(hTreeItemDDNode, TVE_EXPAND);

    // 2) skipping list of databases, find the "Servers" trailing static item
    HTREEITEM hTreeSubItem = m_Tree.GetChildItem(hTreeItemDDNode);
    ASSERT (hTreeSubItem);
    HTREEITEM hTreeItemStaticSV = NULL;
    for ( ; hTreeSubItem; hTreeSubItem = m_Tree.GetNextSiblingItem(hTreeSubItem)) {
      CTreeItemStar* pItem = (CTreeItemStar*)m_Tree.GetItemData(hTreeSubItem);
      ASSERT (pItem);
      if (pItem->IsStatic()) {
        hTreeItemStaticSV = hTreeSubItem;
        break;
      }
    }
    ASSERT (hTreeItemStaticSV);

    // 3) expand servers branch
    ASSERT (hTreeItemStaticSV);
    m_Tree.SelectItem(hTreeItemStaticSV);
    m_Tree.Expand(hTreeItemStaticSV, TVE_EXPAND);

    // 4) find the right server branch and store it's handle in hTreeItemDDNode
    HTREEITEM hTreeItemSvr = m_Tree.GetChildItem(hTreeItemStaticSV);
    ASSERT (hTreeItemSvr);
    do {
      CTreeItemStar* pItem = (CTreeItemStar*)m_Tree.GetItemData(hTreeItemSvr);
      ASSERT (pItem);

      LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)pItem->GetPTreeItemData()->GetDataPtr();
      ASSERT (pServerDataMin);
      if (x_stricmp ((LPCSTR)gwClassName, (LPCSTR)pServerDataMin->ServerClass) == 0) {
        hTreeItemDDNode = hTreeItemSvr;
        break;
      }
    }
    while ( (hTreeItemSvr = m_Tree.GetNextSiblingItem(hTreeItemSvr)) != NULL );
  }

  //
  // Special for drag-drop operation:
  //
  if (m_bDragDrop) {
    // expand tree up to the "good" object
    ASSERT (hTreeItemDDNode);
    m_Tree.SelectItem(hTreeItemDDNode);
    m_Tree.Expand(hTreeItemDDNode, TVE_EXPAND);
    HTREEITEM hTreeItemDB = m_Tree.GetChildItem(hTreeItemDDNode);
    ASSERT (hTreeItemDB);
    do {
      CTreeItemStar* pItem = (CTreeItemStar*)m_Tree.GetItemData(hTreeItemDB);
      ASSERT (pItem);
      LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
      ASSERT (pStar);
      if (m_srcDbName.CompareNoCase((LPCTSTR)pStar->objName) == 0) {
        m_Tree.SelectItem(hTreeItemDB);
        m_Tree.Expand(hTreeItemDB, TVE_EXPAND);
        HTREEITEM hTreeItemTbl = m_Tree.GetChildItem(hTreeItemDB);
        ASSERT (hTreeItemTbl);
        do {
          CTreeItemStar* pItem = (CTreeItemStar*)m_Tree.GetItemData(hTreeItemTbl);
          ASSERT (pItem);
          LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
          ASSERT (pStar);
          if (m_srcObjName.CompareNoCase((LPCTSTR)pStar->objName) == 0 &&
              m_srcOwnerName.CompareNoCase((LPCTSTR)pStar->ownerName) == 0 ) {
            m_Tree.SelectItem(hTreeItemTbl);
            m_Tree.EnsureVisible(hTreeItemTbl);
            break;
          }
        }
        while ( (hTreeItemTbl = m_Tree.GetNextSiblingItem(hTreeItemTbl)) != NULL );
        break;
      }
    }
    while ( (hTreeItemDB = m_Tree.GetNextSiblingItem(hTreeItemDB)) != NULL );

    // check expansion has been successful
    HTREEITEM hSelectedItem = m_Tree.GetSelectedItem();
    ASSERT (hSelectedItem);
    CTreeItem *pItem;
    pItem = (CTreeItem *)m_Tree.GetItemData(hSelectedItem);
    ASSERT (pItem);
    if (pItem) {
      switch (m_linkType) {
        case LINK_TABLE:
          ASSERT (pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4TblTable)));
          break;
        case LINK_VIEW:
          ASSERT (pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4ViewView)));
          break;
        default:
          ASSERT (FALSE);
          break;
      }
    }

    // Cannot Disable Tree, otherwise selection not visible
    // solution: prevent tree expansion and selchange
    // cannot: m_Tree.EnableWindow(FALSE);
    m_bPreventTreeOperation = TRUE;

    // set focus on dbname for rename purposes
    m_EditName.SetFocus();
  }
  if (m_bDragDrop)
    return FALSE;   // we have set the focus to a control
  else
    return TRUE;    // we let the default procedure set the focus
}

void CuDlgStarTblRegister::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
  if (m_pTreeGlobalData)
    m_pTreeGlobalData->FreeAllTreeItemsData();
}

void CuDlgStarTblRegister::OnItemexpandingTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
  *pResult = 0;     // default to allow expanding

  // Prevent expansion if drag-drop mode
  if (m_bPreventTreeOperation) {
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

void CuDlgStarTblRegister::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	*pResult = 0;   // accept selchange

  HTREEITEM hItem = pNMTreeView->itemNew.hItem;
  CTreeItem *pItem;
  pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
  ASSERT (pItem);
  if (pItem) {
    BOOL bFits = FALSE;
    int  colType = OT_UNKNOWN;
    switch (m_linkType) {
      case LINK_TABLE:
        bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4TblTable));
        colType = OT_COLUMN;
        break;
      case LINK_VIEW:
        bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4ViewView));
        colType = OT_VIEWCOLUMN;
        break;
      default:
        ASSERT (FALSE);
        return;
    }

    // check item is not "< error >" or "< No xyz >"
    if (pItem->IsSpecialItem())
      bFits = FALSE;

    if (bFits) {
      // Enable edit
      m_EditName.EnableWindow();

      // Fill edit with object name, removing schema if necessary
      LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
      ASSERT (pStar);
      m_EditName.SetWindowText((LPCSTR)StringWithoutOwner(pStar->objName));

      // Enable reset button on columns list
      m_BtnReset.EnableWindow();

      // fill columns list
      FillColumnsList(pItem);
    }
    else {
      // Selected Item does not fit

      m_EditName.EnableWindow(FALSE);
      m_BtnReset.EnableWindow(FALSE);

      m_EditName.SetWindowText("");
      m_Columns.DeleteAllItems();
    }

  }
}

void CuDlgStarTblRegister::OnChangeDistname() 
{
  if (m_EditName.GetWindowTextLength())
    GetDlgItem(IDOK)->EnableWindow();
  else
    GetDlgItem(IDOK)->EnableWindow(FALSE);
}

void CuDlgStarTblRegister::FillColumnsList(CTreeItem *pItem)
{
  //
  // fills columnslist with list of columns
  //

  CWaitCursor glassHour;

  LPUCHAR aParents[3];
  char bufColName[MAXOBJECTNAME];
  char bufOwnName[MAXOBJECTNAME];
  char bufComplim[MAXOBJECTNAME];

  // extract structure describing object
  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  // Empty list
  m_Columns.DeleteAllItems();

  int  colType = OT_UNKNOWN;
  int  objType = OT_UNKNOWN;
  switch (m_linkType) {
    case LINK_TABLE:
      colType = OT_COLUMN;
      objType = OT_TABLE;
      break;
    case LINK_VIEW:
      colType = OT_VIEWCOLUMN;
      objType = OT_VIEW;
      break;
    default:
      ASSERT (FALSE);
      return;
  }

  // Open Node
  int hNode = OpenNodeStruct(pStar->NodeName);
  ASSERT (hNode != -1);

  // Reload list of databases in cache
  aParents[0] = aParents[1] = aParents[2] = '\0';
  int retval = DOMGetFirstObject(hNode,
                                 OT_DATABASE,
                                 0,
                                 aParents,
                                 NULL,       // No filter on owner
                                 FALSE,      // Not with system - does not care here
                                 (LPUCHAR)bufColName,
                                 (LPUCHAR)bufOwnName,
                                 (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);

  // Reload list of tables in cache
  ASSERT (objType != OT_UNKNOWN);
  aParents[0] = pStar->parent0;
  retval = DOMGetFirstObject(hNode,
                             objType,    // Table or view
                             1,
                             aParents,
                             NULL,       // No filter on owner
                             FALSE,      // Not with system - does not care here
                             (LPUCHAR)bufColName,
                             (LPUCHAR)bufOwnName,
                             (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);

  // Obtain list of columns
  ASSERT (colType != OT_UNKNOWN);
  aParents[0] = pStar->parent0;   // database name
  char bufParent1[MAXOBJECTNAME];
  StringWithOwner(pStar->objName, pStar->ownerName, (LPUCHAR)bufParent1);
  aParents[1] = (LPUCHAR)bufParent1;
  aParents[2] = NULL;

  // loop on getfirst/next
  retval = DOMGetFirstObject(hNode,
                             colType,
                             2,           // level
                             aParents,
                             NULL,       // No filter on owner
                             FALSE,      // Not with system - IMPORTANT
                             (LPUCHAR)bufColName,
                             (LPUCHAR)bufOwnName,
                             (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);
  while (retval != RES_ENDOFDATA) {
    int index = m_Columns.InsertItem(1, (LPCSTR)bufColName);
    ASSERT (index != -1);
    m_Columns.SetItemText(index, 1, (LPTSTR)bufColName);
    retval = DOMGetNextObject((LPUCHAR)bufColName, (LPUCHAR)bufOwnName, (LPUCHAR)bufComplim);
  }

  // terminate against the OpenNodeStruct
  CloseNodeStruct(hNode, TRUE);   // keep entry
}

void CuDlgStarTblRegister::OnBtnReset() 
{
  HTREEITEM hItem = m_Tree.GetSelectedItem();
  ASSERT (hItem);
  CTreeItem *pItem;
  pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
  ASSERT (pItem);
  if (pItem) {
    BOOL bFits = FALSE;
    switch (m_linkType) {
      case LINK_TABLE:
        bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4TblTable));
        break;
      case LINK_VIEW:
        bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4ViewView));
        break;
      default:
        ASSERT (FALSE);
        return;
    }
    ASSERT (bFits);
    if (bFits)
      FillColumnsList(pItem);
  }
}

void CuDlgStarTblRegister::OnBeginEditColumnName(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

  // Always accept column name edition
	*pResult = 0;
}

void CuDlgStarTblRegister::OnEndEditColumnName(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

  // NOTE: since the text we set with SetItemText() may be
  // different from the text in item.pszText, due to spaces trim,
  // we must ALWAYS set *pResult to non-acceptation value

  // default to non-acceptation
	*pResult = 0;

  // null pointer means cancelled operation
  if (pDispInfo->item.pszText) {
    // spaces management
    CString csText(pDispInfo->item.pszText);
    csText.TrimLeft();
    csText.TrimRight();
    if (csText.GetLength() > 0 )
      m_Columns.SetItemText(pDispInfo->item.iItem, 0, csText);
  }
}

void CuDlgStarTblRegister::OnSelchangingTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

  // Prevent selchange if drag-drop mode
  if (m_bPreventTreeOperation) {
    MessageBeep(MB_ICONEXCLAMATION);
    *pResult = 1;
    return;
  }
	
	*pResult = 0;
}

void CuDlgStarTblRegister::OnBtnName()
{
  int nCurSel = m_Columns.GetNextItem(-1, LVNI_SELECTED);
  if (nCurSel != -1) {
    m_Columns.SetFocus();
    CEdit* pEdit = m_Columns.EditLabel(nCurSel);
    ASSERT (pEdit);
  }
}
