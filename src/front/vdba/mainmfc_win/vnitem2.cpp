/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vnitem2.cpp: New branches for "Nodes" tree 
**               related to "Users" aliasing for a connection
**    Author  : Emmanuel Blattes 
**
** History (after 26-Sep-2000)
** 
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
** 28-nov-2001 (somsa01)
**    When throw'ing a string, throw a local variable which is set to the
**    string. This will prevent a SIGABRT on HP.
** 18-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
** 26-Feb-2002 (uk$so01)
**    SIR #106648, Integrate IPM ActiveX Control
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
*/

/* IMPORTANT NOTE: MENU IDs USED <--> METHODS TO WRITE
    IDM_VNODEBAR01: // Connect
    IDM_VNODEBAR02: // SqlTest
    IDM_VNODEBAR03: // Monitor
    IDM_VNODEBAR04: // Dbevent
    IDM_VNODEBAR05: // Disconnect
    IDM_VNODEBAR06: // CloseWin
    IDM_VNODEBAR07: // ActivateWin
    IDM_VNODEBAR08: // Add
    IDM_VNODEBAR09: // Alter
    IDM_VNODEBAR10: // Drop
    IDM_VNODEBARSC: // Scratchpad
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "mainfrm.h"
#include "vnitem.h"

#include "dbedoc.h"     // dbevent

#include "dgvnode.h"    // Virtual Node Dialog, Convenient
#include "dgvnlogi.h"   // Virtual Node Dialog, Advanced Login
#include "dgvndata.h"   // Virtual Node Dialog, Advanced Connection Data

#include "discon3.h"    // Disconnect dialog box

#include "discon2.h"    // Disconnect Server dialog box

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
  #include "main.h"
 // #include "getvnode.h"
  #include "dba.h"
  #include "domdata.h"
  #include "dbadlg1.h"
  #include "dbaginfo.h"
  #include "dbaset.h"     // Oivers
  #include "domdloca.h"
  #include "domdata.h"
// from main.c
BOOL MainOnSpecialCommand(HWND hwnd, int id, DWORD dwData);


#include "resource.h"   // IDM_NonMfc version

#include "monitor.h"    // low-lever interface structures
};

//
// Classes Serialization declarations
//

IMPLEMENT_SERIAL (CuTreeUserStatic, CTreeItemNodes, 2)
IMPLEMENT_SERIAL (CuTreeUser, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeUsrOpenwinStatic, CTreeItemNodes, 1)
IMPLEMENT_SERIAL (CuTreeUsrOpenwin, CTreeItemNodes, 1)

//
// Classes Implementation
//

CuTreeUserStatic::CuTreeUserStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE, IDS_NODEUSER, SUBBRANCH_KIND_OBJ, IDB_NODEUSER, pItemData, OT_USER, IDS_NODEUSER_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

void CuTreeUserStatic::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  // Need to preset gateway type for cohabitation with desktop
  ASSERT (iobjecttypeParent == OT_NODE);
  LPNODEDATAMIN lpNodeDataMin = (LPNODEDATAMIN)pstructParent;
  ASSERT (lpNodeDataMin);
}

void CuTreeUserStatic::StarCloseNodeStruct(int &hNode)
{
  // Need to restore gateway type for cohabitation with desktop
  // hNode ignored
}

CTreeItem* CuTreeUserStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeUser* pItem = new CuTreeUser (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeUserStatic::CuTreeUserStatic()
{
}

BOOL CuTreeUserStatic::IsEnabled(UINT idMenu)
{
  return CTreeItemNodes::IsEnabled(idMenu);
}

void CuTreeUserStatic::UpdateParentBranchesAfterExpand(HTREEITEM hItem)
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  // Verification : chech hItem data against "this"
  CTreeItem *pTreeCheckItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeCheckItem == this);
  if (pTreeCheckItem != this)
    return;

  // This code takes advantage of the tree organization!
  hItem = pTree->GetParentItem(hItem);
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  pTreeItem->ChangeImageId(pTreeItem->GetCustomImageId(), hItem);
}

CuTreeUser::CuTreeUser (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_USER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODEUSER);
}

BOOL CuTreeUser::CreateStaticSubBranches (HTREEITEM hParentItem)
{
  CTreeGlobalData* pGlobalData = GetPTreeGlobData();

  CuTreeUsrOpenwinStatic* pStat1  = new CuTreeUsrOpenwinStatic (pGlobalData, GetPTreeItemData());
  pStat1->CreateTreeLine (hParentItem);

  return TRUE;
}

CuTreeUser::CuTreeUser()
{

}

BOOL CuTreeUser::IsEnabled(UINT idMenu)
{
  LPNODEUSERDATAMIN pUserDataMin = NULL;

  switch (idMenu) {
    case IDM_VNODEBAR01:    // Connect
    case IDM_VNODEBAR02:    // SqlTest
    case IDM_VNODEBARSC:    // Scratchpad
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      return TRUE;
      break;

    case IDM_VNODEBAR03:    // Monitor Ipm
    case IDM_VNODEBAR04:    // Dbevent
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      // needs to be a pure ingres node
      pUserDataMin = (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr();
      ASSERT (pUserDataMin);
      return TRUE;

    case IDM_VNODEBAR05:    // Disconnect
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      return TRUE;
      break;

    case IDM_VNODEBAR08:    // Add
    case IDM_VNODEBAR09:    // Alter
    case IDM_VNODEBAR10:    // Drop
        return FALSE;
      break;

    case ID_SERVERS_TESTNODE:
        return FALSE;
      break;

    default:
      return CTreeItemNodes::IsEnabled(idMenu);
      break;
  }

  ASSERT (FALSE);
  return FALSE;   // security if return forgotten in switch
}

BOOL CuTreeUser::Connect()
{
  // Create a dom type document

  int oldOIVers = GetOIVers();

  LPNODEUSERDATAMIN pUserDataMin = (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr();

  char szNodeName[200];
  lstrcpy(szNodeName, BuildFullNodeName(pUserDataMin));
  BOOL bOk = MainOnSpecialCommand(AfxGetMainWnd()->m_hWnd,
                                  IDM_CONNECT,
                                  (DWORD)szNodeName);
  if (bOk)
    UpdateOpenedWindowsList();    // update low-level
  else {
    SetOIVers(oldOIVers);
  }
  return bOk;
}

BOOL CuTreeUser::SqlTest(LPCTSTR lpszDatabase)
{
	LPNODEUSERDATAMIN pSvrUserDataMin = (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr();
	CString strNode = pSvrUserDataMin->NodeName;
	CString strServer = pSvrUserDataMin->ServerClass;
	CString strUser = pSvrUserDataMin->User;
	CString strDatabase = lpszDatabase;

	CdSqlQuery* pSqlDoc = CreateSqlTest(strNode, strServer, strUser, strDatabase);
	if (pSqlDoc)
	{
		UpdateOpenedWindowsList();    // Update low-level: AFTER SetTitle()!
		return TRUE;
	}

	return FALSE;
}

BOOL CuTreeUser::Monitor()
{
	LPNODEUSERDATAMIN pUserDataMin = (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr();
	CString strNode = pUserDataMin->NodeName;
	CString strServer = pUserDataMin->ServerClass;
	CString strUser = pUserDataMin->User;
	CdIpm* pIpmDoc = CreateMonitor(strNode, strServer, strUser);
	if (pIpmDoc)
	{
		UpdateOpenedWindowsList();    // Update low-level: AFTER SetTitle()!
		return TRUE;
	}
	return FALSE;
}

BOOL CuTreeUser::Dbevent(LPCTSTR lpszDatabase)
{
  int oldOIVers = GetOIVers();

  // Preset gateway type
  LPNODEUSERDATAMIN pUserDataMin = (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr();

  char szNodeName[200];
  lstrcpy(szNodeName, BuildFullNodeName(pUserDataMin));
  if (!CheckConnection((LPUCHAR)szNodeName,FALSE,TRUE)) {
    SetOIVers(oldOIVers);
    return FALSE;   // ERROR!
  }

  int hNode = OpenNodeStruct((LPUCHAR)szNodeName);
  if (hNode == -1) {
    SetOIVers(oldOIVers);
    CString strMsg =  VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
    strMsg += VDBA_MfcResourceString (IDS_E_DBEVENT_TRACE);         // " - Cannot create DBEvent Trace Window."
    MessageBox(GetFocus(),strMsg, NULL, MB_OK | MB_ICONEXCLAMATION);
    return FALSE;   // ERROR!
  }

  // Added March 3, 98: must be pure open-ingres (not a gateway)
  if (GetOIVers() == OIVERS_NOTOI) {
    CString rString;
    AfxFormatString1(rString, IDS_ERR_DBEVENT_NOTOI, (LPCTSTR)szNodeName);
    BfxMessageBox(rString, MB_OK | MB_ICONEXCLAMATION);
    CloseNodeStruct(hNode, FALSE);
    SetOIVers(oldOIVers);
    return FALSE;   // ERROR!
  }

  int OIVers = GetOIVers();   // new as of check connection

	CWinApp* pWinApp = AfxGetApp();
  ASSERT (pWinApp);
  CString csDbEventTemplateString = GetDbEventTemplateString();
  POSITION pos = pWinApp->GetFirstDocTemplatePosition();
  while (pos) {
    CDocTemplate *pTemplate = pWinApp->GetNextDocTemplate(pos);
    CString docName;
    pTemplate->GetDocString(docName, CDocTemplate::docName);
    if (docName == (LPCTSTR)csDbEventTemplateString) {   // DbEvent
      // prepare detached document
      CDocument *pNewDoc = pTemplate->CreateNewDocument();
      ASSERT(pNewDoc);
      if (pNewDoc) {
        // initialize document member variables
        CDbeventDoc *pDbeventDoc = (CDbeventDoc *)pNewDoc;
        pDbeventDoc->m_hNode  = hNode;
        pDbeventDoc->m_OIVers = OIVers;
        pDbeventDoc->SetInputDatabase(lpszDatabase);
        SetOIVers(OIVers);

        // set document title
        CString capt;
        capt.LoadString(IDS_CAPT_DBEVENT);
        CString buf;
        CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
        int seqnum = pMainFrame->GetNextDbeventSeqNum();
        buf.Format("%s - %s - %d", szNodeName, (LPCTSTR)capt, seqnum);
        pNewDoc->SetTitle(buf);
        pDbeventDoc->SetSeqNum(seqnum);

        // Associate to a new frame
        CFrameWnd *pFrame = pTemplate->CreateNewFrame(pNewDoc, NULL);
        ASSERT (pFrame);
        if (pFrame) {
          pTemplate->InitialUpdateFrame(pFrame, pNewDoc);
          UpdateOpenedWindowsList();    // update low-level
          bSaveRecommended = TRUE;
          return TRUE;
        }
      }
    }
  }

  // Here, we have failed!
  CloseNodeStruct(hNode, FALSE);
  SetOIVers(oldOIVers);
  return FALSE;
}

BOOL CuTreeUser::Scratchpad()
{
  // Create a scratchpad type document

  int oldOIVers = GetOIVers();

  // Preset gateway type
  LPNODEUSERDATAMIN pUserDataMin = (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr();

  char szNodeName[200];
  lstrcpy(szNodeName, BuildFullNodeName(pUserDataMin));
  BOOL bOk = MainOnSpecialCommand(AfxGetMainWnd()->m_hWnd,
                                  IDM_SCRATCHPAD,
                                  (DWORD)szNodeName);
  if (bOk)
    UpdateOpenedWindowsList();    // update low-level
  else {
    SetOIVers(oldOIVers);
  }
  return bOk;
}

BOOL CuTreeUser::AtLeastOneWindowOnNode()
{
  int   hNode             = 0;         // no node
  int   iobjecttypeParent = GetType();
  void *pstructParent     = GetPTreeItemData()->GetDataPtr();
  int   iobjecttypeReq    = OT_NODE_OPENWIN;
  int   reqSize           = GetMonInfoStructSize(iobjecttypeReq);
  LPSFILTER psFilter      = NULL;   // no filter

  ASSERT (iobjecttypeParent == OT_NODE_SERVERCLASS);
  ASSERT (pstructParent);

  // allocate one requested structure
  void *pStructReq          = new char[reqSize];

  // call GetFirstMonInfo
  int res = GetFirstMonInfo(hNode,
                            iobjecttypeParent,
                            pstructParent,
                            iobjecttypeReq,
                            pStructReq,
                            psFilter);

  // immediately free memory
  delete pStructReq;

  if (res == RES_ENDOFDATA)
    return FALSE;
  else
    return TRUE;    // includes error cases
}

void CuTreeUser::UpdateOpenedWindowsList()
{
  // Update list of opened windows on the user
  void *pstructParent = GetPTreeItemData()->GetDataPtr();
  UpdateMonInfo(0,                    // hnodestruct,
                OT_USER,              // parent type
                pstructParent,        // parent structure
                OT_NODE_OPENWIN);     // requested type

  // Also, escalate to CuTreeServer Parent to refresh it's windows list
  // escalate parent branch in tree to get "good" parent structure
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem == this);
  if (pTreeItem != this)
    return;
  hItem = pTree->GetParentItem(hItem);  // svrclassstatic
  hItem = pTree->GetParentItem(hItem);  // node
  CuTreeServer* pTSV = (CuTreeServer*)pTree->GetItemData(hItem);
  ASSERT (pTSV);
  ASSERT (pTSV->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  if (pTSV) {
    void *pstructParent2 = pTSV->GetPTreeItemData()->GetDataPtr();
    UpdateMonInfo(0,                // hnodestruct,
                OT_NODE,          // parent type
                pstructParent2,   // parent structure
                OT_NODE_OPENWIN); // requested type
  }
}

BOOL CuTreeUser::Disconnect()
{
  CDlgDisconnUser dlg;

  dlg.m_pUserDataMin = (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr();
  PushHelpId(HELPID_IDD_DISCONNECT_SVRCLASS);
  PushVDBADlg();
  int zap = dlg.DoModal();
  PopVDBADlg();
  PopHelpId();
  if (zap == IDOK) {
    UpdateOpenedWindowsList();    // update low-level
    return TRUE;
  }
  else
    return FALSE;
}



BOOL CuTreeUser::RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because)
{
  BOOL bSuccess = TRUE;

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  // Verification : chech hItem data against "this"
  CTreeItem *pTreeCheckItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeCheckItem == this);
  if (pTreeCheckItem != this)
    return FALSE;

  // This code takes advantage of the tree organization!
  hItem = pTree->GetParentItem(hItem);
  hItem = pTree->GetParentItem(hItem);
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  pTreeItem->ChangeImageId(pTreeItem->GetCustomImageId(), hItem);

  return TRUE;
}


UINT CuTreeUser::GetCustomImageId()
{
  LPNODEUSERDATAMIN pUserDataMin = GetPTreeItemData()? (LPNODEUSERDATAMIN)GetPTreeItemData()->GetDataPtr(): NULL;
  if (!pUserDataMin)
    return IDB_NODEUSER;
  // connected ?
  int hNode = GetVirtNodeHandle((LPUCHAR)(LPCTSTR)BuildFullNodeName(pUserDataMin));
  BOOL bConnected = ( hNode!=-1 ? TRUE : FALSE );
  if (!bConnected) {
     NODEUSERDATAMIN UserDataMin2 = *pUserDataMin;
	 if (ReplaceLocalWithRealNodeName(UserDataMin2.NodeName)) {
		hNode = GetVirtNodeHandle((LPUCHAR)(LPCTSTR)BuildFullNodeName(&UserDataMin2));
		bConnected = ( hNode!=-1 ? TRUE : FALSE );
	 }
  }
  if (bConnected)
    return IDB_NODEUSER_CONNECTED;
  else
    return IDB_NODEUSER;
}


CuTreeUsrOpenwinStatic::CuTreeUsrOpenwinStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_USER, IDS_USER_OPENWIN, SUBBRANCH_KIND_OBJ, IDB_USER_OPENWIN, pItemData, OT_NODE_OPENWIN, IDS_USER_OPENWIN_NONE)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeUsrOpenwinStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeUsrOpenwin* pItem = new CuTreeUsrOpenwin (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeUsrOpenwinStatic::CuTreeUsrOpenwinStatic()
{


}

CuTreeUsrOpenwin::CuTreeUsrOpenwin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemNodes (pTreeGlobalData, OT_NODE_OPENWIN, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE_OPENWIN);
}

CuTreeUsrOpenwin::CuTreeUsrOpenwin()
{

}

BOOL CuTreeUsrOpenwin::IsEnabled(UINT idMenu)
{
  switch (idMenu) {
    case IDM_VNODEBAR06:    // CloseWin
    case IDM_VNODEBAR07:    // ActivateWin
      if (IsNoItem() || IsErrorItem())
        return FALSE;
      return TRUE;
    default:
      return CTreeItemNodes::IsEnabled(idMenu);
  }
  return FALSE;   // security if return forgotten in switch
}

BOOL CuTreeUsrOpenwin::ActivateWin()
{
  LPWINDOWDATAMIN pWindowDataMin = (LPWINDOWDATAMIN)GetPTreeItemData()->GetDataPtr();

  ASSERT (pWindowDataMin);
  ASSERT (pWindowDataMin->hwnd);
  HWND    hwnd = pWindowDataMin->hwnd;

  CWnd *pWnd;
  pWnd = pWnd->FromHandlePermanent(hwnd);
  ASSERT (pWnd);
  if (pWnd) {
    CMDIChildWnd *pMdiChild = (CMDIChildWnd *)pWnd;
    pMdiChild->MDIActivate();
    if (pMdiChild->IsIconic())
      pMdiChild->MDIRestore();
    return TRUE;
  }
  else
    return FALSE;
}

BOOL CuTreeUsrOpenwin::CloseWin()
{
  LPWINDOWDATAMIN pWindowDataMin = (LPWINDOWDATAMIN)GetPTreeItemData()->GetDataPtr();

  ASSERT (pWindowDataMin);
  ASSERT (pWindowDataMin->hwnd);
  HWND    hwnd = pWindowDataMin->hwnd;

  BOOL bSuccess = TRUE;

  CWnd *pWnd;
  pWnd = pWnd->FromHandlePermanent(hwnd);
  ASSERT (pWnd);
  if (pWnd) {
    CMDIChildWnd *pMdiChild = (CMDIChildWnd *)pWnd;
    ForgetDelayedUpdates();
    pMdiChild->MDIDestroy();
    AcceptDelayedUpdates();
    UpdateOpenedWindowsList();  // update low level
  }
  else
    bSuccess = FALSE;
  return bSuccess;
}

void CuTreeUsrOpenwin::UpdateOpenedWindowsList()
{
  // escalate parent branch in tree to get "good" parent structure
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem == this);
  if (pTreeItem != this)
    return;
  hItem = pTree->GetParentItem(hItem);  // static openwin
  CuTreeUsrOpenwinStatic *pTOS = (CuTreeUsrOpenwinStatic *)pTree->GetItemData(hItem);
  ASSERT (pTOS);
  ASSERT (pTOS->IsKindOf(RUNTIME_CLASS(CuTreeUsrOpenwinStatic)));
  if (pTOS) {
    void *pstructParent = pTOS->GetPTreeItemData()->GetDataPtr();
    UpdateMonInfo(0,                    // hnodestruct,
                  OT_USER,              // parent type
                  pstructParent,        // parent structure
                  OT_NODE_OPENWIN);     // requested type
  }

  // Also, escalate to CuTreeServer Parent to refresh it's windows list
  hItem = pTree->GetParentItem(hItem);  // user
  hItem = pTree->GetParentItem(hItem);  // user static
  hItem = pTree->GetParentItem(hItem);  // node
  CuTreeServer* pTSV = (CuTreeServer*)pTree->GetItemData(hItem);
  ASSERT (pTSV);
  ASSERT (pTSV->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
  if (pTSV) {
    void *pstructParent2 = pTSV->GetPTreeItemData()->GetDataPtr();
    UpdateMonInfo(0,                // hnodestruct,
                OT_NODE,          // parent type
                pstructParent2,   // parent structure
                OT_NODE_OPENWIN); // requested type
  }
}

