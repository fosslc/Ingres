/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Visual DBA
**
**  Source : starprln.cpp
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
#include "starprln.h"
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

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarProcRegister dialog


CuDlgStarProcRegister::CuDlgStarProcRegister(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStarProcRegister::IDD, pParent)
{
  m_destNodeName  = "";
  m_destDbName    = "";
  m_bDragDrop     = FALSE;
  m_srcNodeName   = "";
  m_srcDbName     = "";
  m_srcObjName    = "";
  m_srcOwnerName  = "";

  m_pTreeGlobalData = NULL;

  m_bPreventTreeOperation = FALSE;

	//{{AFX_DATA_INIT(CuDlgStarProcRegister)
	//}}AFX_DATA_INIT
}


CuDlgStarProcRegister::~CuDlgStarProcRegister()
{
  if (m_pTreeGlobalData)
    delete m_pTreeGlobalData;
}

void CuDlgStarProcRegister::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStarProcRegister)
	DDX_Control(pDX, IDC_STATIC_SOURCE, m_GroupboxSource);
	DDX_Control(pDX, IDC_TREE, m_Tree);
	DDX_Control(pDX, IDC_STATIC_DISTNAME, m_StaticName);
	DDX_Control(pDX, IDC_DISTNAME, m_EditName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStarProcRegister, CDialog)
	//{{AFX_MSG_MAP(CuDlgStarProcRegister)
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE, OnItemexpandingTree)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnSelchangedTree)
	ON_EN_CHANGE(IDC_DISTNAME, OnChangeDistname)
	ON_NOTIFY(TVN_SELCHANGING, IDC_TREE, OnSelchangingTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarProcRegister message handlers

void CuDlgStarProcRegister::OnOK() 
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
  bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4ProcProc));
  ASSERT (bFits);
  if (!bFits)
    return;
  // end of Triple-check item validity

  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  //
  // Build sql statement
  //
  CString stmt = "register procedure ";

  // object name
  CString objName;
  m_EditName.GetWindowText(objName);
  stmt += QuoteIfNeeded(objName);
  stmt += " ";

  // syntax
  stmt += "as link from ";

  // local owner/ object name
  stmt += QuoteIfNeeded((LPCTSTR)pStar->ownerName);
  stmt += ".";
  stmt += QuoteIfNeeded((LPCTSTR)pStar->objName);
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
    stmt += GetLocalHostName();
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

BOOL CuDlgStarProcRegister::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // check input parameters
  ASSERT (m_destNodeName != "");
  ASSERT (m_destDbName != "");
  if (m_bDragDrop) {
    ASSERT (m_srcNodeName != "");
    ASSERT (m_srcDbName   != "");
    ASSERT (m_srcObjName  != "");
  }

  // disable controls that need to
  m_EditName.EnableWindow(FALSE);
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
    pNewItem = new CuTreeStar4ProcServer (m_pTreeGlobalData, &ItemData);
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
    if (pItem)
      ASSERT (pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4ProcProc)));

    // Cannot Disable Tree, otherwise selection not visible
    // solution: prevent tree expansion and selchange
    // cannot: m_Tree.EnableWindow(FALSE);
    m_bPreventTreeOperation = TRUE;

    // set focus on dbname for rename purposes
    m_EditName.SetFocus();
  }
  PushHelpId (IDHELP_IDD_STAR_PROCREGISTER);
  if (m_bDragDrop)
    return FALSE;   // we have set the focus to a control
  else
    return TRUE;    // we let the default procedure set the focus
}

void CuDlgStarProcRegister::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
  if (m_pTreeGlobalData)
    m_pTreeGlobalData->FreeAllTreeItemsData();
}

void CuDlgStarProcRegister::OnItemexpandingTree(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CuDlgStarProcRegister::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	*pResult = 0;   // accept selchange

  HTREEITEM hItem = pNMTreeView->itemNew.hItem;
  CTreeItem *pItem;
  pItem = (CTreeItem *)m_Tree.GetItemData(hItem);
  ASSERT (pItem);
  if (pItem) {
    BOOL bFits = FALSE;
    bFits = pItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4ProcProc));

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

    }
    else {
      // Selected Item does not fit

      m_EditName.EnableWindow(FALSE);

      m_EditName.SetWindowText("");
    }

  }
}

void CuDlgStarProcRegister::OnChangeDistname() 
{
  if (m_EditName.GetWindowTextLength())
    GetDlgItem(IDOK)->EnableWindow();
  else
    GetDlgItem(IDOK)->EnableWindow(FALSE);
}


void CuDlgStarProcRegister::OnSelchangingTree(NMHDR* pNMHDR, LRESULT* pResult) 
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
