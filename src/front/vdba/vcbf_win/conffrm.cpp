/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : conffrm.cpp, Implementation file                                      //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Frame support to provide the splitter in the Page (Configure) of      //
//               the class CMainTabDlg                                                 //
//               See the CONCEPT.DOC for more detail                                   //
//    History:
//        01-apr-99 (cucjo01)
//             Added special context sensitive help id for Remote Command
//        29-jul-99 (cucjo01)
//             Added support for command-line option '/c' for the initial
//             screen by passing /c="<Branch>\<Sub-Branch>", VCBF will show
//             the selected item when it first comes up.
//             Some examples:
//                VCBF.EXE /c="Security"
//                   - Display the Security branch (it doesn't have sub-branches)
//                VCBF.EXE /c="DBMS Servers\dbms1"
//                   - Expand DBMS Servers and select sub-branch dbms1
//                VCBF.EXE /c="DBMS Servers"
//                   - Expand DBMS Servers and select "(default)"
//                VCBF.EXE /c="DBMS Servers\someunknownvalue"
//                   - Only Expand DBMS Servers (nothing is selected)
//                     (ie: it will display as much as it can before it fails)
//        04-aug-99 (cucjo01)
//             When VCBF first appears, the right pane should display a default
//             settings page so that it doesn't look so empty when it first
//             appears.  It should display a default of 'Name Server'
//             if no command-line parameters are given.
//        06-Jun-2000: (uk$so01) 
//            (BUG #99242)
//             Cleanup code for DBCS compliant
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
****************************************************************************************/

#include "stdafx.h"
#include "vcbf.h"
#include "conffrm.h"
#include "confvr.h"
#include "confdoc.h"
#include "lvleft.h"
#include "cachedlg.h" // Help ID

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigFrame


CConfigFrame::CConfigFrame()
{
	m_bAllViewCreated = FALSE;
}

CConfigFrame::~CConfigFrame()
{
}


BEGIN_MESSAGE_MAP(CConfigFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CConfigFrame)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigFrame message handlers
//    History  :
//        03-dec-98 (cucjo01)
//             Re-sized for aesthetic purposes

BOOL CConfigFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1, context2;
	CRuntimeClass* pView1 = RUNTIME_CLASS(CConfListViewLeft);
	CRuntimeClass* pView2 = RUNTIME_CLASS(CConfViewRight);
	CRuntimeClass* pView3 = RUNTIME_CLASS(CConfViewRight);
	context1.m_pNewViewClass = pView1;
	context1.m_pCurrentDoc   = new CConfDoc;
	context2.m_pNewViewClass = pView2;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;
	if (!m_Splitterwnd.CreateStatic (this, 1, 2)) {
		TRACE0 ("Failed to create Splitter\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 0, pView1, CSize (280, 400), &context1)) {
		TRACE0 ("Failed to create first pane\n");
		return FALSE;
	}
	if (!m_Splitterwnd.CreateView (0, 1, pView2, CSize (350, 400), &context2)) {
		TRACE0 (" Failed to create second pane\n");
		return FALSE;
	}
	
    m_Splitterwnd.SetActivePane( 0,0); 
	CWnd* pWnd = m_Splitterwnd.GetPane (0,0);
	pWnd->Invalidate();
	m_Splitterwnd.RecalcLayout();
    m_bAllViewCreated = TRUE;

    DisplayContextBranch();

	return TRUE;
}

void CConfigFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}

CConfLeftDlg* CConfigFrame::GetCConfLeftDlg()
{
	CConfListViewLeft* pViewLeft =(CConfListViewLeft*)m_Splitterwnd.GetPane (0,0);
	ASSERT_VALID(pViewLeft);
	return (CConfLeftDlg*)pViewLeft->GetCConfLeftDlg();
}

CConfRightDlg* CConfigFrame::GetCConfRightDlg()
{
	CConfViewRight* pViewRight =(CConfViewRight*)m_Splitterwnd.GetPane (0,1);
	ASSERT_VALID(pViewRight);
	return (CConfRightDlg*)pViewRight->GetCConfRightDlg();
}

UINT CConfigFrame::GetHelpID()
{
	ASSERT (m_bAllViewCreated == TRUE);
	CConfRightDlg* pConfigRightDlg = GetCConfRightDlg();
	CConfLeftDlg*  pConfigLeftDlg  = GetCConfLeftDlg();

	ASSERT (pConfigLeftDlg);
	if (!pConfigLeftDlg)
		return 0;


    HTREEITEM hTreeItem = pConfigLeftDlg->m_tree_ctrl.GetSelectedItem();

	//
	// Nothing has been selected in the left pane, the help is the one
	// of the left pane:
	if (!hTreeItem)
		return IDD_CONFIG_LEFT;
	//
	// If there is a selected item in the left pane, then check to see 
	// if there is no pane in the right side, if so we must handle the special case:
	CuPageInformation*  pPageInfo = NULL;
	CuCbfListViewItem* pItem = (CuCbfListViewItem*)pConfigLeftDlg->m_tree_ctrl.GetItemData (hTreeItem);
	if (!pItem)
		return IDD_CONFIG_LEFT;

	if (pItem && pItem->GetPageInformation() && pItem->GetPageInformation()->GetNumberOfPage() == 0)
	{
		//
		// No page in the right pane:
		pPageInfo = pItem->GetPageInformation();
		if (pPageInfo->GetClassName().CompareNoCase (_T("RECOVER")) == 0)
			return IDHELP_RECOVER;
		if (pPageInfo->GetClassName().CompareNoCase (_T("ARCHIVE")) == 0)
			return IDHELP_ACHIVER;
		if (pPageInfo->GetClassName().CompareNoCase (_T("RMCMD")) == 0)
			return IDHELP_RMCMD;

	}
	UINT nIDHelp = 0;
	ASSERT (pConfigRightDlg);
	if (!pConfigRightDlg)
		return nIDHelp;
	CuCbfProperty* pProp = pConfigRightDlg->GetCurrentProperty();
	if (!pProp)
		return nIDHelp;
	pPageInfo = pProp->GetPageInfo();
	if (!pPageInfo)
		return nIDHelp;
	int nCurSel = pProp->GetCurSel();
	if (nCurSel>=0 && nCurSel<pPageInfo->GetNumberOfPage() && pPageInfo->GetNumberOfPage()>0)
		nIDHelp = pPageInfo->GetDlgID(nCurSel);
	//
	// For the cache page the are three sub-panes (cache buffer, parameter, derived):
	if (nIDHelp == IDD_DBMS_PAGE_CACHE)
	{
		CuDlgDbmsCache* pCachePage = (CuDlgDbmsCache*)pConfigRightDlg->GetCurrentPage();
		if (pCachePage)
		{
			CvDbmsCacheViewLeft* pCacheBuffer = pCachePage->GetLeftPane();
			if (pCacheBuffer)
			{
				CuCacheCheckListCtrl* pListBuffer = pCacheBuffer->GetCacheListCtrl();
				//
				// Cache buffer:
				if (pListBuffer->GetSelectedCount() == 0)
					nIDHelp = IDD_CACHE_LIST;
				else
				{
					CvDbmsCacheViewRight* pCacheRightPane = pCachePage->GetRightPane();
					if (pCacheRightPane)
					{
						CuDlgCacheTab* pTab = pCacheRightPane->GetCacheTabDlg();
						if (pTab)
							nIDHelp = pTab->GetHelpID();
					}
				}
			}
		}
	}
	return nIDHelp;
}


BOOL CConfigFrame::DisplayContextBranch()
{   TCHAR p;
    int i, iPos, iLen, iLeft, iChars=0;
    CString strCmdLine(_T("")), strContext(_T(""));
    CString strItem(_T("")), strClass(_T("")), strSubItem(_T("")); 
    HTREEITEM hTreeItem=NULL, hChildItem=NULL, hShowItem=NULL;

    // Check for command line option
    CVcbfApp *theApp = (CVcbfApp *)AfxGetApp();
    if (!theApp)
       return FALSE;

    if (theApp->m_lpCmdLine)
      strCmdLine = theApp->m_lpCmdLine;

    strCmdLine.MakeLower();
    //
    // DBCS Compliant:
    iPos = strCmdLine.Find(_T("/c=\""));
    if (iPos != -1)
    { 
      iLen = strCmdLine.GetLength();
      iLeft = iPos + 4;
      for (i=iLeft; i<iLen; i++)
      { 
        p = strCmdLine.GetAt(i);
        if (p == _T('\"'))
        { 
          break;
        }
        iChars++;
      }
      
      strContext=strCmdLine.Mid(iLeft, iChars);
    }
    else
      strContext=_T("Name Server");

    // Take the context string and divide it into Branch and Sub-Branch
    // If no subitem is specified, "(default)" is used
       
    if (strContext.IsEmpty())
       return FALSE;    

    iPos = strContext.Find(_T("\\"));
    if (iPos != -1)
    { strClass = strContext.Left(iPos);
      strSubItem = strContext.Right(strContext.GetLength() - iPos - 1);
    }
    else
    { strClass = strContext;
      strSubItem = GetLocDefConfName();
    }

    CConfLeftDlg* pLeftDlg = GetCConfLeftDlg();
	if (!pLeftDlg) 
       return FALSE;

    // Search through the first level of the tree
    hTreeItem = pLeftDlg->m_tree_ctrl.GetChildItem(pLeftDlg->m_tree_ctrl.GetRootItem());
  
    while (hTreeItem)
    { 
      strItem = pLeftDlg->m_tree_ctrl.GetItemText(hTreeItem);
	
	  if (strItem.CompareNoCase(strClass) == 0)
      { pLeftDlg->m_tree_ctrl.Expand(hTreeItem, TVE_EXPAND);
        hShowItem=hTreeItem;
        break;
      }

      hTreeItem = pLeftDlg->m_tree_ctrl.GetNextSiblingItem(hTreeItem);
    }

    // Search through child items if necessary
    hChildItem = pLeftDlg->m_tree_ctrl.GetChildItem(hTreeItem);
    
    while (hChildItem)
    { 
      strItem = pLeftDlg->m_tree_ctrl.GetItemText(hChildItem);
	
	  if (strItem.CompareNoCase(strSubItem) == 0)
      { hShowItem=hChildItem;
        break;
      }
      
      hChildItem = pLeftDlg->m_tree_ctrl.GetNextSiblingItem(hChildItem);
    }

    // Select the item that was found and display the right pane 
    if (hShowItem)
       pLeftDlg->m_tree_ctrl.Select(hShowItem,TVGN_CARET);
 
    return TRUE;
}