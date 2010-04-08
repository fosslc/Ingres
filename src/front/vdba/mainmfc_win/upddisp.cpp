/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : upddisp.cpp, Implementation file 
**    Project  : CA-OpenIngres/Monitoring.
**
** HISTORY (after 26-Feb-2002):
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "mainfrm.h"
#include "extccall.h"

static void UpdateMonDisplay(int hnodestruct, HWND hwnd)
{
/*UKS
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwnd);
  ASSERT (pWnd);
  ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CIpmFrame)));

  CIpmFrame *pIpmFrame = (CIpmFrame *)pWnd;
  CIpmDoc *pDoc = (CIpmDoc *)pIpmFrame->GetActiveDocument();
  ASSERT (pDoc);

  CTreeGlobalData *pGD =  pDoc->GetPTreeGD();
  ASSERT (pGD);

  // refresh left pane
  pGD->RefreshAllTreeBranches();

  CTreeItem* pItem = NULL;
  try
  {
    // Special management for replication
    CuDlgIpmTabCtrl* pTabDlg  = (CuDlgIpmTabCtrl*)pIpmFrame->GetTabDialog();
    ASSERT (pTabDlg);
    IPMUPDATEPARAMS ups;
    memset (&ups, 0, sizeof (ups));

    CTreeCtrl *pTree = pGD->GetPTree();
    ASSERT (pTree);
    HTREEITEM hSelectedItem = pTree->GetSelectedItem();
    pItem = (CTreeItem*)pTree->GetItemData(hSelectedItem);
    ASSERT (pItem);
    if (pItem->IsNoItem() || pItem->IsErrorItem())
    {
        pTabDlg->DisplayPage (NULL);
    }
    else if (pItem->ItemDisplaysNoPage()) {
        CString caption = pItem->ItemNoPageCaption();
        pTabDlg->DisplayPage (NULL, caption);
    }
    else {
        int nImage = -1, nSelectedImage = -1;
        CImageList* pImageList = pTree->GetImageList (TVSIL_NORMAL);
        HICON hIcon = NULL;
        int nImageCount = pImageList? pImageList->GetImageCount(): 0;
        if (pImageList && pTree->GetItemImage(hSelectedItem, nImage, nSelectedImage))
        {
            if (nImage < nImageCount)
                hIcon = pImageList->ExtractIcon(nImage);
        }
        CuPageInformation* pPageInfo = pItem->GetPageInformation();
        CString strItem = pItem->GetRightPaneTitle();

        ups.nType   = pItem->GetType();
        ups.pStruct = pItem->GetPTreeItemData()? pItem->GetPTreeItemData()->GetDataPtr(): NULL;
        ups.pSFilter= pDoc->GetPTreeGD()->GetPSFilter();
        pPageInfo->SetUpdateParam (&ups);
        pPageInfo->SetTitle ((LPCTSTR)strItem, pItem, pDoc->m_hNode);

        pPageInfo->SetImage  (hIcon);
        pTabDlg->DisplayPage (pPageInfo);
    }
  }
  catch(CMemoryException* e)
  {
      VDBA_OutOfMemoryMessage();
      e->Delete();
      return;
  }
  catch(CResourceException* e)
  {
      //_T("Cannot load dialog box");
      BfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_DLG));
      e->Delete();
      return;
  }
  catch(...)
  {
      //_T("Cannot construct the property pane");
      BfxMessageBox (VDBA_MfcResourceString(IDS_E_CONSTRUCT_PROPERTY));
      return;
  }

  // Replicator Monitor special management
  // ASSUMES we did not change the current item!
  CIpmView1 *pView1 = (CIpmView1 *)pIpmFrame->GetLeftPane();
  ASSERT (pView1);
  int* pReplMonHandle = pView1->GetPReplMonHandle();
  BOOL bReplMonWasOn = (*pReplMonHandle != -1) ? TRUE: FALSE;
  BOOL bReplMonToBeOn = pItem->HasReplicMonitor();
  // 3 cases :
  //    - on to off ---> replicator has been uninstalled
  //    - off to on ---> replicator has been installed
  //    - on to on, or off to off: state has not changed
  if (bReplMonWasOn && !bReplMonToBeOn) {
    int res = DBEReplMonTerminate(*pReplMonHandle);
    ASSERT (res == RES_SUCCESS);
    *pReplMonHandle = -1;
  }
  else if (!bReplMonWasOn && bReplMonToBeOn) {
    CString csDbName = pItem->GetDBName();
    int hNode = pGD->GetHNode();
    ASSERT (hNode != -1);
    int res = DBEReplMonInit(hNode,
                             (LPUCHAR)(LPCTSTR)csDbName,
                             pReplMonHandle);
    ASSERT (res == RES_SUCCESS);
  }

  // refresh right pane
  pDoc->UpdateAllViews((CView *)pIpmFrame->GetLeftPane());
*/
}
void UpdateMonDisplay(int   hnodestruct,
                      int   iobjecttypeParent,
                      void *pstructParent,
                      int   iobjecttypeReq)
{
/*UKS
	UPDMONDISPLAYPARAMS params;
	params.hnodestruct=hnodestruct;
	params.iobjecttypeParent=iobjecttypeParent;
	params.pstructParent=pstructParent;
	params.iobjecttypeReq=iobjecttypeReq;
	if (CanBeInSecondaryThead())
		Notify_MT_Action(ACTION_UPDATEMONDISPLAY, (LPARAM) &params);
	else
		UpdateMonDisplay_MT(hnodestruct,iobjecttypeParent,pstructParent,iobjecttypeReq);
*/
}

/*ukS
void UpdateMonDisplay_MT(int   hnodestruct,
                      int   iobjecttypeParent,
                      void *pstructParent,
                      int   iobjecttypeReq)
{
  ASSERT (iobjecttypeReq == OT_MON_ALL);
  if (iobjecttypeReq != OT_MON_ALL)
    return;

  // scan ipm docs on requested node
  HWND  hwndCurDoc;

  // get first document handle (with loop to skip the icon title windows)
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);

  while (hwndCurDoc) {
    // test monitor on requested node
    if (QueryDocType(hwndCurDoc) == TYPEDOC_MONITOR)
      if (GetMDINodeHandle(hwndCurDoc) == hnodestruct)
        UpdateMonDisplay(hnodestruct, hwndCurDoc);

    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }
}
*/
void UpdateNodeDisplay_MT()
{
  CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
  ASSERT (pMainFrame);

  CuResizableDlgBar *pNodeBar = pMainFrame->GetVNodeDlgBar();
  ASSERT (pNodeBar);

  CTreeGlobalData *pGD = pNodeBar->GetPTreeGD();
  ASSERT (pGD);

  pGD->RefreshAllTreeBranches();
}

void UpdateNodeDisplay()
{
	if (CanBeInSecondaryThead())
		Notify_MT_Action(ACTION_UPDATENODEDISPLAY, (LPARAM) 0);
	else
		UpdateNodeDisplay_MT();
}

////////////////////////////////////////////////////////////////////
// new code for font preferences management - started april 23, 97
extern "C" void UpdateIpmDocsFont(HFONT hFontIpmTree)
{
/*UKS
  HWND hwndCurDoc;

  // get first document handle (with loop to skip the icon title windows)
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);

  while (hwndCurDoc) {
    // test monitor on requested node
    if (QueryDocType(hwndCurDoc) == TYPEDOC_MONITOR) {
      CWnd *pWnd = CWnd::FromHandlePermanent(hwndCurDoc);
      ASSERT (pWnd);
      if (pWnd) {
        CIpmFrame *pIpmFrm = (CIpmFrame *)pWnd;
        CIpmDoc *pIpmDoc = (CIpmDoc *)pIpmFrm->GetActiveDocument();
        ASSERT (pIpmDoc);
        if (pIpmDoc) {
          CTreeGlobalData *pGD = pIpmDoc->GetPTreeGD();
          ASSERT (pGD);
          CTreeCtrl *pTree = pGD->GetPTree();
          pTree->SetFont (CFont::FromHandle(hFontIpmTree));
          //
          // Update font of some right panes:
          CuDlgIpmTabCtrl* pTabDlg = pIpmFrm->GetTabDialog ();
          if (pTabDlg && IsWindow (pTabDlg->m_hWnd))
          {
              if (pTabDlg->m_pCurrentPage && IsWindow (pTabDlg->m_pCurrentPage->m_hWnd))
                  pTabDlg->m_pCurrentPage->SendMessage (WM_CHANGE_FONT, 0, (LPARAM)hFontIpmTree);
          }
        }
      }
    }

    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }
*/
}

extern "C" void UpdateNodeFont(HFONT hFontNodeTree)
{
  CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
  ASSERT (pMainFrame);

  CuResizableDlgBar *pNodeBar = pMainFrame->GetVNodeDlgBar();
  ASSERT (pNodeBar);
  CTreeGlobalData *pGD = pNodeBar->GetPTreeGD();
  ASSERT (pGD);
  CTreeCtrl *pTree = pGD->GetPTree();
  pTree->SetFont (CFont::FromHandle(hFontNodeTree));
}
