/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**    Source   : cleftdlg.cpp, Implementation file 
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**               Blattes Emmanuel (Custom implementations)
**    Purpose  : Modeless dialog displayed at the left-pane of the
**		 page Configure 
**
**    History:
**	22-nov-98 (cucjo01)
**	    Added wait dialog AVI's
**	    On duplicate item, select the new item and put it in edit mode
**	    Added focus on count option for properties page
**	09-dec-98 (cucjo01)
**	    Added ICE Server Page support
**	    Added Gateway Pages for Oracle, Informix, Sybase, MSSQL, and ODBC
**	    * These pages specifically get put under the "Gateway" folder
**	    Added "double-click" -> properties on tree view items
**	    Added check on properties page to update only if info has changed
**	    Sized tree control to entire window since buttons are gone
**	    Added maximum text limit of text fields in tree control
**	22-dec-98 (cucjo01)
**	    Updated tree control of non-duplicatable items to be at same level
**	    as duplicatable items
**	15-mar-99 (cucjo01)
**	    Added typecast to CString.Format() calls for MAINWIN to behave
**	    properly
**	30-mar-1999 (mcgem01)
**	    Added the product name dynamically.
**	21-may-1999 (cucjo01)
**	    Added tooltip on tree control to notify user of r-button pop-up menu
**	    Pass pItem to VALIDATE messages so Startup Count can be updated 
**	    on the right panes.
**	    Update right window if user brings up "Properties" dialog for change
**	07-aug-1999 (cucjo01)
**	    VCBF now asks user if they want to save changes if they close the
**	    window after making a change.  Previously, the user would only be
**	    asked to save the changes if they switched between components.
**	16-may-2000 (cucjo01)
**	    Added JDBC Server page support.
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	16-Oct-2001 (uk$so01)
**	    BUG #106053, free the unused memory
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 	31-Oct-2001 (hanje04)
**    	    Move declaration of cpt outside for loops to stop non-ANSI compiler 
**          errors on Linux.
**  02-Oct-2002 (noifr01)
**    (SIR 107587) have VCBF manage DB2UDB gateway (mapping it to
**    oracle gateway code until this is moved to generic management)
**  04-Oct-2002 (noifr01)
**    (bug 108868) display "gateways" instead of "gateway" for the gateways
**    parent branch.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
**  07-Jul-2003 (schph01)
**    (bug 106858) consistently with CBF change 455479
**    used the new constructor for CuSECUREItemData class when it is not
**    necessary to display all tabs for "Security" branch.
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
** 06-May-2007 (drivi01)
**    SIR #118462
**    Separate COMP_TYPE_GW_ORACLE and COMP_TYPE_GW_DB2UDB to
**    use a separate GUI.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "lvleft.h"
#include "cleftdlg.h"
#include "confvr.h"
#include "conffrm.h"
#include "cbfitem.h"
#include "ConfigLeftProperties.h"
#include "WaitDlg.h"
#include <compat.h>
#include <gl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define g_nClosed    0
#define g_nOpen      1
#define g_nBoxOpen   2
#define g_nBoxClosed 2
#define g_nGear      2

/////////////////////////////////////////////////////////////////////////////
// CConfLeftDlg dialog


CConfLeftDlg::CConfLeftDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfLeftDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfLeftDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CConfLeftDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfLeftDlg)
	DDX_Control(pDX, IDC_TREE1, m_tree_ctrl);
	DDX_Control(pDX, IDC_LIST1, m_clistctrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfLeftDlg, CDialog)
	//{{AFX_MSG_MAP(CConfLeftDlg)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_BN_CLICKED(IDC_BUTTON1, OnEditName)
	ON_BN_CLICKED(IDC_BUTTON2, OnEditCount)
	ON_BN_CLICKED(IDC_BUTTON3, OnDuplicate)
	ON_BN_CLICKED(IDC_BUTTON4, OnDelete)
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_TREE1, OnItemexpandedTree)
	ON_NOTIFY(NM_RCLICK, IDC_TREE1, OnRclickTree1)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1, OnDblclickTree1)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, OnSelchangedTree1)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TREE1, OnBeginlabeleditTree1)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE1, OnEndlabeleditTree1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_COMPONENT_EXIT,  OnComponentExit)
	ON_MESSAGE (WM_COMPONENT_ENTER, OnComponentEnter)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfLeftDlg message handlers

void CConfLeftDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	CRect rDlg;
	CWnd* pWndListCtrl = GetDlgItem (IDC_LIST1);

	CWnd* pWndTreeCtrl = GetDlgItem (IDC_TREE1);

	GetClientRect (rDlg);

	if (pWndListCtrl && IsWindow(pWndListCtrl->m_hWnd)) 
	{
		//
		// Resize the 3 buttons.
		// The buttons will be placed at the bottom of the dialog.
		CButton* pB1 = (CButton*)GetDlgItem (IDC_BUTTON1);
		CButton* pB2 = (CButton*)GetDlgItem (IDC_BUTTON2);
		CButton* pB3 = (CButton*)GetDlgItem (IDC_BUTTON3);
		CButton* pB4 = (CButton*)GetDlgItem (IDC_BUTTON4);
		
		CRect r, newRect;
		pB1->GetWindowRect (r);
		ScreenToClient (r);
		newRect = r;
		newRect.bottom  = rDlg.bottom -2;
		newRect.top     = newRect.bottom - r.Height();
		pB1->MoveWindow (newRect);

		pB2->GetWindowRect (r);
		ScreenToClient (r);
		newRect.left    = r.left;
		newRect.right   = r.right;
		pB2->MoveWindow (newRect);

		pB3->GetWindowRect (r);
		ScreenToClient (r);
		newRect.left    = r.left;
		newRect.right   = r.right;
		pB3->MoveWindow (newRect);

		pB4->GetWindowRect (r);
		ScreenToClient (r);
		newRect.left    = r.left;
		newRect.right   = r.right;
		pB4->MoveWindow (newRect);

		//
		// Resize the list Control.
		newRect = rDlg;
		newRect.bottom  = rDlg.bottom -10 -2 - r.Height();
		pWndListCtrl->MoveWindow(newRect);

		// Resize the tree Control.
		newRect = rDlg;
		pWndTreeCtrl->MoveWindow(newRect);
	}
}


CuCbfListViewItem* CConfLeftDlg::CreateItemFromComponentInfo(LPCOMPONENTINFO lpcomp, int &imageIndex)
{
  CuCbfListViewItem*  pItem = NULL;
  imageIndex = -1;

  switch (lpcomp->itype) {
    case COMP_TYPE_NAME             :
      // Name Server
      pItem = new CuNAMEItemData();
      imageIndex = INDEX_IMGLIST_NAME;
      break;
    case COMP_TYPE_DBMS             :
      // DBMS Server
      pItem = new CuDBMSItemData();
      imageIndex = INDEX_IMGLIST_DBMS;
      break;
    case COMP_TYPE_INTERNET_COM     :
      // ICE Server
      pItem = new CuICEItemData();
      imageIndex = INDEX_IMGLIST_ICE;
      break;
    case COMP_TYPE_NET              :
      // NET Server
      pItem = new CuNETItemData();
      imageIndex = INDEX_IMGLIST_NET;
      break;
    case COMP_TYPE_JDBC             :
      // JDBC Server
      pItem = new CuJDBCItemData();
      imageIndex = INDEX_IMGLIST_JDBC;
      break;
    case COMP_TYPE_DASVR             :
      // DASVR Server
      pItem = new CuDASVRItemData();
      imageIndex = INDEX_IMGLIST_DASVR;
      break;
    case COMP_TYPE_BRIDGE           :
      // BRIDGE Server
      pItem = new CuBRIDGEItemData();
      imageIndex = INDEX_IMGLIST_BRIDGE;
      break;
    case COMP_TYPE_STAR             :
      // STAR Server
      pItem = new CuSTARItemData();
      imageIndex = INDEX_IMGLIST_STAR;
      break;
    case COMP_TYPE_SECURITY         :
      // SECURITY
      if (!lpcomp->bDisplayAllTabs)
         pItem = new CuSECUREItemData(lpcomp->bDisplayAllTabs);
      else
         pItem = new CuSECUREItemData();
      imageIndex = INDEX_IMGLIST_SECURE;
      break;
    case COMP_TYPE_LOCKING_SYSTEM   :
      // LOCKING SYSTEM
      pItem = new CuLOCKItemData();
      imageIndex = INDEX_IMGLIST_LOCK;
      break;
    case COMP_TYPE_LOGGING_SYSTEM   :
      // LOGGING SYSTEM
      pItem = new CuLOGItemData();
      imageIndex = INDEX_IMGLIST_LOG;
      break;
    case COMP_TYPE_TRANSACTION_LOG  :
      // LOGFILE 
      pItem = new CuLOGFILEItemData();
      imageIndex = INDEX_IMGLIST_LOGFILE;
      break;
    case COMP_TYPE_RECOVERY         :
      // RECOVERY Server
      pItem = new CuRECOVERItemData();
      imageIndex = INDEX_IMGLIST_RECOVER;
      break;
    case COMP_TYPE_ARCHIVER         :
      // ARCHIVER
      pItem = new CuARCHIVEItemData();
      imageIndex = INDEX_IMGLIST_ARCHIVE;
      break;
    case COMP_TYPE_RMCMD:
      // RMCMD
      pItem = new CuRMCMDItemData();
      imageIndex = INDEX_IMGLIST_RMCMD;
      break;
    case COMP_TYPE_GW_DB2UDB        : /* same GUI as for Oracle */
      // DB2 UDB Gateway
      pItem = new CuGW_DB2UDBItemData();
      imageIndex = INDEX_IMGLIST_GW_ORACLE;
      break;
    case COMP_TYPE_GW_ORACLE        : 
      // Oracle Gateway
      pItem = new CuGW_ORACLEItemData();
      imageIndex = INDEX_IMGLIST_GW_ORACLE;
      break;
    case COMP_TYPE_GW_INFORMIX      :
      // Informix Gateway
      pItem = new CuGW_INFORMIXItemData();
      imageIndex = INDEX_IMGLIST_GW_INFORMIX;
      break;
    case COMP_TYPE_GW_SYBASE        :
      // Sybase Gateway
      pItem = new CuGW_SYBASEItemData();
      imageIndex = INDEX_IMGLIST_GW_SYBASE;
      break;
    case COMP_TYPE_GW_MSSQL         :
      // MSSQL Gateway
      pItem = new CuGW_MSSQLItemData();
      imageIndex = INDEX_IMGLIST_GW_MSSQL;
      break;
    case COMP_TYPE_GW_ODBC          :
      // ODBC Gateway
      pItem = new CuGW_ODBCItemData();
      imageIndex = INDEX_IMGLIST_GW_ODBC;
      break;

  }

  ASSERT (pItem);
  ASSERT (imageIndex != -1);
  return pItem;
}


BOOL CConfLeftDlg::AddSystemComponent(LPCOMPONENTINFO lpcomp)
{ TCHAR szBuffer[1024];
	
	CuCbfListViewItem*  pItem = NULL;
	int imageIndex;
	pItem = CreateItemFromComponentInfo(lpcomp, imageIndex);
	ASSERT (pItem);
	if (!pItem)
		return FALSE;
	ASSERT (imageIndex != -1);
	if (imageIndex == -1) {
		delete pItem;
		return FALSE;
	}

	pItem->SetComponentInfo(lpcomp);
	pItem->SetConfLeftDlg(this);
	int nCount, index;
	nCount = m_clistctrl.GetItemCount ();
	index  = m_clistctrl.InsertItem (nCount, (LPCTSTR)lpcomp->sztype, imageIndex);
	m_clistctrl.SetItemText (index, 1, (LPCTSTR)lpcomp->szname);
	m_clistctrl.SetItemText (index, 2, (LPCTSTR)lpcomp->szcount);
	m_clistctrl.SetItemData (index, (DWORD_PTR)pItem);

	TCHAR szDisplayString[512];
	if (VCBFllCanCompBeDuplicated (&pItem->m_componentInfo))
	{ _tcscpy(szDisplayString, (LPCTSTR)lpcomp->szname);
	}
	else
	{ _stprintf(szDisplayString, _T("%s"), lpcomp->sztype);
	  if ( (!_tcscmp((LPCTSTR)lpcomp->sztype, GetTransLogCompName())) && (!_tcscmp((LPCTSTR)lpcomp->szname, _T("II_LOG_FILE"))) )
		  _stprintf(szDisplayString, _T("Primary Transaction Log"));
	  if ( (!_tcscmp((LPCTSTR)lpcomp->sztype, GetTransLogCompName())) && (!_tcscmp((LPCTSTR)lpcomp->szname, _T("II_DUAL_LOG"))) )
		  _stprintf(szDisplayString, _T("Dual Transaction Log"));
	}

    // Add Items To TreeView, If the parent exists, use it
	HTREEITEM hFindItem=NULL, hNewItem=NULL;
	hFindItem = FindInTree(m_tree_ctrl.GetRootItem(), (LPCTSTR)lpcomp->sztype);
	
	if (!hFindItem) 
    { if (pItem && (!_tcscmp((LPCTSTR)lpcomp->sztype, _T("Oracle Gateway")))     ||   /// Use Constants Here
                   (!_tcscmp((LPCTSTR)lpcomp->sztype, _T("Informix Gateway")))   ||
                   (!_tcscmp((LPCTSTR)lpcomp->sztype, _T("Sybase Gateway")))     || 
                   (!_tcscmp((LPCTSTR)lpcomp->sztype, _T("SQL Server Gateway"))) ||
                   (!_tcscmp((LPCTSTR)lpcomp->sztype, _T("DB2 UDB Gateway"))) ||
                   (!_tcscmp((LPCTSTR)lpcomp->sztype, _T("ODBC Gateway")))     )
	  { _tcscpy(szDisplayString, (LPCTSTR)lpcomp->sztype);
        hFindItem = FindInTree(m_tree_ctrl.GetRootItem(), _T("Gateways"));
        if (!hFindItem)
           hFindItem = AddItemToTree(_T("Gateways"), m_tree_ctrl.GetRootItem(), TRUE, 0, NULL);
      }
      else if (pItem && VCBFllCanCompBeDuplicated (&pItem->m_componentInfo))
	  { _stprintf(szBuffer, _T("%s"), (LPCTSTR)lpcomp->sztype);
	    hFindItem = AddItemToTree(szBuffer, m_tree_ctrl.GetRootItem(), TRUE, 0, NULL);
      }
    }

	hNewItem = AddItemToTree(szDisplayString, hFindItem, FALSE, imageIndex+3, pItem);
	
	return TRUE;  // successful
}

BOOL CConfLeftDlg::OnInitDialog() 
{
	const int LAYOUT_NUMBER = 3;
	int rc = m_ToolTip.Create(this);
	rc = m_ToolTip.AddTool(GetDlgItem(IDC_TREE1), IDS_TOOLSTIP_RIGHT_CLICK);
	m_ToolTip.Activate(TRUE);

	// CDialog::OnInitDialog();
	VERIFY (m_clistctrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_clistctrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN|LVS_SINGLESEL|LVS_SHOWSELALWAYS;
	SetWindowLong (m_clistctrl.m_hWnd, GWL_STYLE, style);

	LV_COLUMN lvcolumn;
	m_Imagelist1.Create(IDB_BITMAP1, 16, 1, RGB(255, 0, 255));
	m_clistctrl.SetImageList(&m_Imagelist1, LVSIL_SMALL);

	/// JC
	
	
	VERIFY (m_tree_ctrl.SubclassDlgItem (IDC_TREE1, this));
	style = GetWindowLong (m_tree_ctrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN|LVS_SINGLESEL|LVS_SHOWSELALWAYS;
	SetWindowLong (m_tree_ctrl.m_hWnd, GWL_STYLE, style);

	m_Imagelist2.Create(IDB_BITMAP2, 16, 1, RGB(255, 0, 255));
	m_tree_ctrl.SetImageList(&m_Imagelist2, TVSIL_NORMAL);

	CString strTitle;
	strTitle.Format(_T("%s Configuration"), SYSTEM_PRODUCT_NAME);
	AddItemToTree(strTitle.GetBuffer(255), TVI_ROOT, TRUE, 2, NULL);  // Init the treeview control with a root item

	UINT strHeaderID [LAYOUT_NUMBER] = { IDS_COL_COMPONENT, IDS_COL_CONFIG, IDS_COL_STARTUP};
	int i, hWidth    [LAYOUT_NUMBER] = {140 , 100, 100};
	//
	// Set the number of columns: LAYOUT_NUMBER
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER; i++)
	{
		lvcolumn.fmt      = LVCFMT_LEFT;
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = hWidth[i];
		/// m_clistctrl.InsertColumn (i, &lvcolumn);  /// Causes RtlFreeHeap() Errors
	}

	// create the lines
	try
	{
		BOOL bSuccess = FALSE;
		
		COMPONENTINFO comp;
		BOOL bFirstPass = TRUE;
		while (1) {
			BOOL bOk;
			if (bFirstPass) {
				bOk = VCBFllGetFirstComp(&comp);
				bFirstPass = FALSE;
			}
			else
				bOk = VCBFllGetNextComp (&comp) ;
			if (!bOk)
				break;    // last item encountered
			if (!AddSystemComponent(&comp)) {
				AfxMessageBox(IDS_ERR_CREATE_LIST, MB_ICONSTOP | MB_OK);
				EndDialog(IDCANCEL);    // leave initdialog
				return TRUE;
			}
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CConfLeftDlg::OnInitDialog() has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}

	m_tree_ctrl.Expand(m_tree_ctrl.GetRootItem(), TVE_EXPAND);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfLeftDlg::OnDestroy() 
{
	int nCount;
	CuCbfListViewItem* pData = NULL;
	
	nCount = m_clistctrl.GetItemCount();
	
	// delete all items
	for (int i=0; i<nCount; i++)
	{
		pData = (CuCbfListViewItem*)m_clistctrl.GetItemData (i);
		ASSERT (pData);
		if (pData)
			delete pData;
	}

	CDialog::OnDestroy();
}

void CConfLeftDlg::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CConfLeftDlg::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CConfListViewLeft* pView = (CConfListViewLeft*)GetParent();
	ASSERT (pView);
	CSplitterWnd* pSplitter = (CSplitterWnd*)pView->GetParent();
	ASSERT (pSplitter);
	CConfigFrame* pFrame = (CConfigFrame*)pSplitter->GetParent();
	ASSERT (pFrame);
	if (!pFrame->IsAllViewsCreated())
		return;
	CConfViewRight* pViewRight = (CConfViewRight*)pSplitter->GetPane (0,1);
	if (!pViewRight && !IsWindow (pViewRight->m_hWnd))
		return;

	// Manage Activation/Deactivations ---> calls to low level
	BOOL bOldStateActive = (BOOL)(pNMListView->uOldState & LVIS_SELECTED);
	BOOL bNewStateActive = (BOOL)(pNMListView->uNewState & LVIS_SELECTED);
	
	BOOL bActivated   = !bOldStateActive && bNewStateActive;
	BOOL bDeactivated = bOldStateActive && !bNewStateActive;
	
	try
	{
		// Manage Deactivation on low level side
		if (bDeactivated) {
			TRACE0 ("OnItemchangedList1: item has been deactivated...\n");
			ASSERT (pNMListView->iItem != -1);
			CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_clistctrl.GetItemData (pNMListView->iItem);
			ASSERT (pItem);
			CWaitCursor GlassHour;
			//
			// Try to validate the old active page
			(pViewRight->GetCConfRightDlg())->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, 0);
			BOOL bModified = pItem->LowLevelDeactivationWithCheck();
		}
		
		// Manage Activation on low level side
		if (bActivated) {
			TRACE0 ("OnItemchangedList1: item has been activated...\n");
			ASSERT (pNMListView->iItem != -1);
			CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_clistctrl.GetItemData (pNMListView->iItem);
			ASSERT (pItem);
			CWaitCursor GlassHour;
			BOOL bSuccess = pItem->LowLevelActivationWithCheck();
			ASSERT (bSuccess);
		}
		
		// Disable all buttons when deactivated
		if (bDeactivated)
		{
			GetDlgItem (IDC_BUTTON1)->EnableWindow (FALSE);
			GetDlgItem (IDC_BUTTON2)->EnableWindow (FALSE);
			GetDlgItem (IDC_BUTTON3)->EnableWindow (FALSE);
			GetDlgItem (IDC_BUTTON4)->EnableWindow (FALSE);
		}
		
		// Enable the "Good" buttons when activated
		if (bActivated) // pNMListView->uNewState != 0 && (pNMListView->uNewState & LVIS_SELECTED))
		{
			ASSERT (pNMListView->iItem != -1);
			CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_clistctrl.GetItemData (pNMListView->iItem);
		
			GetDlgItem (IDC_BUTTON1)->EnableWindow (VCBFllCanNameBeEdited    (&pItem->m_componentInfo));
			GetDlgItem (IDC_BUTTON2)->EnableWindow (VCBFllCanCountBeEdited   (&pItem->m_componentInfo));
			GetDlgItem (IDC_BUTTON3)->EnableWindow (VCBFllCanCompBeDuplicated(&pItem->m_componentInfo));
			GetDlgItem (IDC_BUTTON4)->EnableWindow (VCBFllCanCompBeDeleted   (&pItem->m_componentInfo));
			CuPageInformation* pPageInfo = pItem->GetPageInformation();
			ASSERT (pPageInfo);
			pPageInfo->SetCbfItem (pItem);
			pViewRight->DisplayPage(pPageInfo);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CConfLeftDlg::OnItemchangedList1 has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	*pResult = 0;
}

void CConfLeftDlg::OnEditName() 
{
	CRect r, rCell;
	UINT nState = 0;
	int iNameCol = 1;
	int iIndex = -1;
	int i, nCount = m_clistctrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_clistctrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return;
	m_clistctrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_clistctrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_clistctrl.SetEditText (iIndex, iNameCol, rCell);
	}
}

void CConfLeftDlg::OnEditCount() 
{
	CRect r, rCell;
	UINT nState  = 0;
	int iNameCol = 2;
	int iIndex   = -1;
	int i, nCount= m_clistctrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_clistctrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return;
	m_clistctrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_clistctrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_clistctrl.SetEditSpin (iIndex, iNameCol, rCell, 0, 400);
	}
}

	
void CConfLeftDlg::GetMaxInternaleName (LPCTSTR lpszTemplateName, long& lnMaxCount)
{
	lnMaxCount= 0L;
	int nLen = (lpszTemplateName)? _tcslen (lpszTemplateName): 0;
	int i, nCount = m_clistctrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		CString strItem = m_clistctrl.GetItemText (i, 0);
		CString strTemplate = strItem;
		int iSearch = strTemplate.Find (_T(' '));
		if (iSearch > 0)
			strTemplate = strTemplate.Left (iSearch);
		if (strTemplate.CompareNoCase (lpszTemplateName) == 0)
		{
			CString strItem1 = m_clistctrl.GetItemText (i, 1);
			int len = strItem1.GetLength();

			//
			// DBCS Compliant
			int cpt = 0;
			for (cpt = len-1; cpt>=0; cpt--)
			{
				if (!_istdigit(strItem1[cpt]))
					break;
			}

			// drop if only digits in name
			if (cpt < 0)
				continue;

			// compare left part of string (without the trailing digits), to template
			CString sLeft = strItem1.Left(cpt+1);
			if (sLeft.CompareNoCase (strTemplate) != 0)
				continue;

			// pick trailing number and keep maximum value
			ASSERT (len-cpt-1 > 0 );
			CString sNum = strItem1.Right(len-cpt-1);
			long lVal = 0L;
			lVal = _ttol ((LPCTSTR)sNum);
			lnMaxCount  = max (lnMaxCount, lVal);
		}
	}
}


void CConfLeftDlg::OnDuplicate() 
{
	CRect r, rCell;
	UINT nState  = 0;
	int iNameCol = 2;
	int iIndex   = -1;
	int i, nCount= m_clistctrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_clistctrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return;
	
	try
	{
		// Emb Sept 24, 97: modify for preliminary test at low level side
		CuCbfListViewItem* pItemData1 = (CuCbfListViewItem*)m_clistctrl.GetItemData(iIndex);
		LPCOMPONENTINFO lpComponentInfo = &pItemData1->m_componentInfo;
		COMPONENTINFO   dupComponentInfo;
		memcpy(&dupComponentInfo, lpComponentInfo, sizeof(COMPONENTINFO));
		int iSearch = -1;
		long lnMaxInternalName;
		CString strTemplate = lpComponentInfo->sztype;
		iSearch = strTemplate.Find (_T(' '));
		if (iSearch > 0)
			strTemplate = strTemplate.Left (iSearch);
		
		GetMaxInternaleName ((LPCTSTR)strTemplate, lnMaxInternalName);
		CString strGenerateName;
		strGenerateName.Format (_T("%s%ld"), (LPCTSTR)strTemplate, lnMaxInternalName+1);   // Emb 22/10/97: no underscore
		_tcscpy((LPTSTR)dupComponentInfo.szname, (LPTSTR)(LPCTSTR)strGenerateName);


		AfxGetApp()->DoWaitCursor(1);

		BOOL bSuccess = VCBFllValidDuplicate(lpComponentInfo, &dupComponentInfo, dupComponentInfo.szname);
		AfxGetApp()->DoWaitCursor(-1);
		if (!bSuccess)
			return;
		_tcscpy((LPTSTR)dupComponentInfo.szname, (LPTSTR)(LPCTSTR)strGenerateName);  // Must copy again!
		
		// here we can create a duplicate line
		CuCbfListViewItem* pItemData2 = (CuCbfListViewItem*)pItemData1->Copy();
		_tcscpy ((LPTSTR)dupComponentInfo.szcount, _T("0"));  // Reset to Zero when duplicating
		pItemData2->SetComponentInfo(&dupComponentInfo);
		pItemData2->SetConfLeftDlg(this);
		LV_ITEM lvitem;
		int     nImage = -1;
		memset (&lvitem, 0, sizeof(lvitem));
		lvitem.mask  = LVIF_IMAGE;
		lvitem.iItem = iIndex;
		if (m_clistctrl.GetItem (&lvitem))
			nImage = lvitem.iImage;
		CString strMainItem = m_clistctrl.GetItemText (iIndex, 0);
		iIndex = m_clistctrl.InsertItem (iIndex+1, (LPTSTR)dupComponentInfo.sztype, nImage);
		if (iIndex != -1)
		{
			m_clistctrl.SetItemData (iIndex, (DWORD_PTR)pItemData2);
			m_clistctrl.SetItemText (iIndex, 1, (LPTSTR)dupComponentInfo.szname);
			m_clistctrl.SetItemText (iIndex, 2, (LPTSTR)dupComponentInfo.szcount);
			m_clistctrl.SetItemState(iIndex, LVIS_SELECTED, LVIS_SELECTED);
			OnEditName();
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CConfLeftDlg::OnDuplicate() has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}

void CConfLeftDlg::OnDelete() 
{
	//
	// Delete the current selected Item.
	CConfListViewLeft* pView = (CConfListViewLeft*)GetParent();
	ASSERT (pView);
	CSplitterWnd* pSplitter  = (CSplitterWnd*)pView->GetParent ();
	ASSERT (pSplitter);
	CConfigFrame* pFrame = (CConfigFrame*)pSplitter->GetParent();
	ASSERT (pFrame);
	if (!pFrame->IsAllViewsCreated())
		return;
	CConfViewRight* pViewRight = (CConfViewRight*)pSplitter->GetPane (0,1);

	UINT nState  = 0;
	int iIndex   = -1;
	int i, nCount= m_clistctrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_clistctrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return;

	try
	{
		// Emb Sept 24, 97: modify for preliminary test at low level side
		CuCbfListViewItem* pItemData1 = (CuCbfListViewItem*)m_clistctrl.GetItemData(iIndex);
		LPCOMPONENTINFO lpComponentInfo = &pItemData1->m_componentInfo;
		AfxGetApp()->DoWaitCursor(1);
		BOOL bSuccess = VCBFllValidDelete(lpComponentInfo);
		AfxGetApp()->DoWaitCursor(-1);
		if (!bSuccess)
			return;
			
		m_clistctrl.HideProperty ();
		int nNewSelectedItem = -1;
		if (nCount == 1)
		{
			pViewRight->DisplayPage(NULL);
		}
		else
		{
			nNewSelectedItem = (iIndex == 0)? nNewSelectedItem = iIndex +1: nNewSelectedItem = iIndex -1;
			m_clistctrl.SetItemState(nNewSelectedItem, LVIS_SELECTED, LVIS_SELECTED);
		}
		
		delete pItemData1;
		m_clistctrl.DeleteItem (iIndex);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CConfLeftDlg::OnDelete() has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}


LONG CConfLeftDlg::OnComponentExit (UINT wParam, LONG lParam)
{
	int nCount;
	CuCbfListViewItem* pData = NULL;
	
	nCount = m_clistctrl.GetItemCount();
	
	// deactivate the currently selected line, if any
	for (int i=0; i < nCount; i++)
	{
		// call low level for final termination
		UINT nState = m_clistctrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED) {
			pData = (CuCbfListViewItem*)m_clistctrl.GetItemData (i);
			ASSERT (pData);
			BOOL bModified = pData->LowLevelDeactivationWithCheck();
			break;
		}
	}


	return 0;
}

LONG CConfLeftDlg::OnComponentEnter (UINT wParam, LONG lParam)
{
	int nCount;
	CuCbfListViewItem* pData = NULL;
	
	nCount = m_clistctrl.GetItemCount();
	
	// deactivate the currently selected line, if any
	for (int i=0; i<nCount; i++)
	{
		// call low level for final termination
		UINT nState = m_clistctrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED) {
			pData = (CuCbfListViewItem*)m_clistctrl.GetItemData (i);
			ASSERT (pData);
			BOOL bModified = pData->LowLevelActivationWithCheck();
			break;
		}
	}
	return 0;
}

void CConfLeftDlg::OnMainFrameClose()
{
	int nCount;
	CuCbfListViewItem* pData = NULL;
	
	nCount = m_clistctrl.GetItemCount();

	// loop for low level deactivation before window has disappeared
	for (int i=0; i<nCount; i++)
	{
		// call low level for final termination
		UINT nState = m_clistctrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED) {
			pData = (CuCbfListViewItem*)m_clistctrl.GetItemData (i);
			ASSERT (pData);
			BOOL bModified = pData->LowLevelDeactivationWithCheck();
			break;
		}
	}

	HTREEITEM hti = m_tree_ctrl.GetSelectedItem();
	if (!hti)
	   return;

	CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_tree_ctrl.GetItemData (hti);
	if (!pItem)  // Ignore if there is no data
	   return;

	CConfListViewLeft* pView = (CConfListViewLeft*)GetParent();
	CSplitterWnd* pSplitter = (CSplitterWnd*)pView->GetParent();
	CConfigFrame* pFrame = (CConfigFrame*)pSplitter->GetParent();
	if (!pFrame->IsAllViewsCreated())
	   return;
	CConfViewRight* pViewRight = (CConfViewRight*)pSplitter->GetPane (0,1);
	if (!pViewRight && !IsWindow (pViewRight->m_hWnd))
	   return;
	(pViewRight->GetCConfRightDlg())->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, (LPARAM)pItem);

	BOOL bModified = pItem->LowLevelDeactivationWithCheck();

}

LONG CConfLeftDlg::OnUpdateData (UINT wParam, LONG lParam)
{
	CConfListViewLeft* pView = (CConfListViewLeft*)GetParent();
	ASSERT (pView);
	CSplitterWnd* pSplitter  = (CSplitterWnd*)pView->GetParent ();
	ASSERT (pSplitter);
	CConfigFrame* pFrame = (CConfigFrame*)pSplitter->GetParent();
	ASSERT (pFrame);
	if (!pFrame->IsAllViewsCreated())
		return 0L;
	CConfViewRight* pViewRight = (CConfViewRight*)pSplitter->GetPane (0,1);
	OnComponentEnter(wParam, lParam);
	(pViewRight->GetCConfRightDlg())->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);
	return 0L;
}

LONG CConfLeftDlg::OnValidateData (UINT wParam, LONG lParam)
{
	m_clistctrl.HideProperty();
	return 0L;
}

// AddItemToTree - adds items to a tree-view control. 
// Returns the handle of the newly added item. 
// lpszItem - text of the item to add 
// hParent - Handle to parent leaf
// bDir - if this item is a directory (use "folder" instead of "box" bitmap)
//
// Returns the newly inserted HTREEITEM (if the caller cares)
HTREEITEM CConfLeftDlg::AddItemToTree( LPCTSTR lpszItem, HTREEITEM hParent, int bDir, int iIconIndex, CuCbfListViewItem *pItem)
{ 
	TVITEM tvi; 
	TV_INSERTSTRUCT tvins; 
	static HTREEITEM hPrev = (HTREEITEM) TVI_FIRST; 
	
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
 
	// Set the text of the item. 
	tvi.pszText = (LPTSTR)lpszItem; 
	tvi.cchTextMax = lstrlen(lpszItem); 
 
	// Use folder if it is a directory, else use box
	if (bDir) {
		tvi.iImage = g_nClosed; 		// Closed folder
		tvi.iSelectedImage = g_nClosed; // Still closed, expanding it opens it, not just selecting
	}
	else {
		tvi.iImage = iIconIndex;		// Closed box of software
		tvi.iSelectedImage = iIconIndex;// Open box (just by selecting it)
	}

	tvins.item = tvi; 

	if (bDir)
	   tvins.hInsertAfter = TVI_FIRST;
	else if (!VCBFllCanCompBeDuplicated (&pItem->m_componentInfo))
	   tvins.hInsertAfter = TVI_LAST;
	else 
	   tvins.hInsertAfter = TVI_SORT;

	if (!hParent)
	   tvins.hParent = m_tree_ctrl.GetRootItem();
	else
	   tvins.hParent = hParent;
	
	hPrev = m_tree_ctrl.InsertItem(&tvins);

	m_tree_ctrl.SetItemData (hPrev, (DWORD_PTR)pItem);

	return hPrev; 
}

// They are expanding the tree. Adjust the image of the folder accordingly
void CConfLeftDlg::OnItemexpandedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_ITEM 	 tvi; 
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	// If it's collapsed, the folder is closed whether selected or not
	if (pNMTreeView->action == TVE_COLLAPSE) { 
		tvi.iImage = g_nClosed;
		tvi.iSelectedImage = g_nClosed;
	}
	// If it's expanded, the folder is open whether selected or not
	else if (pNMTreeView->action == TVE_EXPAND) {
		tvi.iImage = g_nOpen;
		tvi.iSelectedImage = g_nOpen;
	}
	else return;

	// Override Icon with "Gear" if root
	if ( (pNMTreeView->itemNew).hItem == m_tree_ctrl.GetRootItem() )
	{ tvi.iImage = g_nGear;
	  tvi.iSelectedImage = g_nGear;
	}

	// Show the open or closed folder icon for the node
	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
	tvi.hItem = (pNMTreeView->itemNew).hItem;
	m_tree_ctrl.SetItem(&tvi);
	
	*pResult = 0;
}

// Return HTREEITEM if true, NULL if click was not within tree
HTREEITEM CConfLeftDlg::ClickWithinTree()
{
	DWORD			dwpos;
	POINT			point;
	HTREEITEM		hti = NULL;
	TV_HITTESTINFO	tvhi;

	// Find out where the cursor was
	dwpos = GetMessagePos();
	point.x = LOWORD(dwpos);
	point.y = HIWORD(dwpos);

	::MapWindowPoints(HWND_DESKTOP, ::GetDlgItem(m_hWnd, IDC_TREE1), &point, 1);

	tvhi.pt = point;
	hti = m_tree_ctrl.HitTest( &tvhi );

	return(hti);
}

// They right-clicked the tree, make sure whatever they right-clicked
// on becomes selected, if they right-clicked on a tree item.
// PreTranslateMessage() will make sure the context menu is popped-up
void CConfLeftDlg::OnRclickTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	HTREEITEM hti = ClickWithinTree();
	
	if (hti == NULL) return;

	// Make sure whatever the user right-clicks on becomes selected
	if (m_tree_ctrl.GetSelectedItem() != hti)
		m_tree_ctrl.Select(hti,TVGN_CARET);

	*pResult = 0;

}

void CConfLeftDlg::OnDblclickTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// If user double-clicks on an item in the tree, go to "edit count" property
	
	HTREEITEM hti = ClickWithinTree();
	
	if (hti == NULL) return;

	// Make sure whatever the user right-clicks on becomes selected
	if (m_tree_ctrl.GetSelectedItem() != hti)
		m_tree_ctrl.Select(hti,TVGN_CARET);

	OnMenuProperties(TRUE);

	*pResult = 0;

}

// Context menu coming up (right-click), enable/disable the appropriate items
void CConfLeftDlg::OnContextMenu(CWnd*, CPoint point)
{  HTREEITEM hti;
   BOOL bEditName;
   BOOL bEditCount;
   BOOL bDuplicate;
   BOOL bDelete;
   
   if (!(hti = ClickWithinTree())) return;  // Right click, but not on the tree, go away

   CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_tree_ctrl.GetItemData (hti);
   if (!pItem)  // Ignore if there is no data
	   return;

   CMenu menu;

	VERIFY(menu.LoadMenu(IDR_TREE_POPUP));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	CWnd* pWndPopupOwner = this;

   bEditName = VCBFllCanNameBeEdited (&pItem->m_componentInfo);
   bEditCount = VCBFllCanCountBeEdited (&pItem->m_componentInfo);
   bDuplicate = VCBFllCanCompBeDuplicated (&pItem->m_componentInfo);
   bDelete = VCBFllCanCompBeDeleted (&pItem->m_componentInfo);
   
   CString strItemText, strParentText, strMenuText;
   strItemText = m_tree_ctrl.GetItemText(hti);
   strParentText = m_tree_ctrl.GetItemText(m_tree_ctrl.GetParentItem(hti));

   if (bDuplicate)
     strMenuText.Format(IDS_MENU_DUPLICATE_DEFAULT,(LPCTSTR)strParentText, (LPCTSTR)GetLocDefConfName());
   else
     strMenuText.Format(IDS_MENU_DUPLICATE_DEFAULT,(LPCTSTR)strItemText, (LPCTSTR)GetLocDefConfName());

   pPopup->ModifyMenu(ID_MENU_DUPLICATE_DEFAULT, MF_BYCOMMAND | MF_STRING, ID_MENU_DUPLICATE_DEFAULT, strMenuText);
 
   if (bDuplicate)
     strMenuText.Format(IDS_MENU_DUPLICATE_BASED,(LPCTSTR)strParentText,(LPCTSTR)strItemText);
   else
     strMenuText.Format(IDS_MENU_DUPLICATE,(LPCTSTR)strItemText);


   pPopup->ModifyMenu(ID_MENU_DUPLICATE, MF_BYCOMMAND | MF_STRING, ID_MENU_DUPLICATE, strMenuText);

   strMenuText.Format(IDS_MENU_DELETE,(LPCTSTR)strItemText);   //= _T("&Delete \"") + strItemText + _T("\"");

   pPopup->ModifyMenu(ID_MENU_DELETE, MF_BYCOMMAND | MF_STRING, ID_MENU_DELETE, strMenuText);
 
   if (bEditName)
      pPopup->EnableMenuItem(ID_MENU_EDITNAME, MF_BYCOMMAND | MF_ENABLED);
   else
  	  pPopup->EnableMenuItem(ID_MENU_EDITNAME, MF_BYCOMMAND | MF_GRAYED);

   if (bEditCount)
      pPopup->EnableMenuItem(ID_MENU_EDITCOUNT, MF_BYCOMMAND | MF_ENABLED);
   else
      pPopup->EnableMenuItem(ID_MENU_EDITCOUNT, MF_BYCOMMAND | MF_GRAYED);

   if (bDuplicate)
      pPopup->EnableMenuItem(ID_MENU_DUPLICATE_DEFAULT, MF_BYCOMMAND | MF_ENABLED);
   else
      pPopup->EnableMenuItem(ID_MENU_DUPLICATE_DEFAULT, MF_BYCOMMAND | MF_GRAYED);

   if (bDuplicate)
      pPopup->EnableMenuItem(ID_MENU_DUPLICATE, MF_BYCOMMAND | MF_ENABLED);
   else
      pPopup->EnableMenuItem(ID_MENU_DUPLICATE, MF_BYCOMMAND | MF_GRAYED);
 
   if (strItemText.CompareNoCase(GetLocDefConfName()) == 0)
      pPopup->RemoveMenu(ID_MENU_DUPLICATE, MF_BYCOMMAND);

   if (bDelete)
      pPopup->EnableMenuItem(ID_MENU_DELETE, MF_BYCOMMAND | MF_ENABLED);
   else
      pPopup->EnableMenuItem(ID_MENU_DELETE, MF_BYCOMMAND | MF_GRAYED);

   if (!VCBFllCanCompBeDuplicated (&pItem->m_componentInfo))
      pPopup->RemoveMenu(ID_MENU_DELETE, MF_BYCOMMAND);
	
//	while (pWndPopupOwner->GetStyle() & WS_CHILD)
//		pWndPopupOwner = pWndPopupOwner->GetParent();

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RIGHTBUTTON,
	   point.x, point.y, pWndPopupOwner);
}


BOOL CConfLeftDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);

	// CG: This block was added by the Pop-up Menu component
	{
		// Shift+F10: show pop-up menu.
		if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
			(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||	// it's Shift+F10 OR
			(pMsg->message == WM_CONTEXTMENU)) 									// Natural keyboard key
		{
			CRect rect;
			GetClientRect(rect);
			ClientToScreen(rect);

			CPoint point = rect.TopLeft();
			point.Offset(5, 5);
			OnContextMenu(NULL, point);
			return TRUE;
		}
	}

    if( pMsg->message == WM_KEYDOWN )
    {
       if (m_tree_ctrl.GetEditControl() && 
          (pMsg->wParam == VK_RETURN
           || pMsg->wParam == VK_DELETE
           || pMsg->wParam == VK_ESCAPE
           || GetKeyState(VK_CONTROL)) )
       { ::TranslateMessage(pMsg);
         ::DispatchMessage(pMsg);
         return TRUE;  // Do Not Process Further
       }
    }

    if( pMsg->message == WM_KEYDOWN )
    {
       if (!(m_tree_ctrl.GetEditControl()) && 
          (pMsg->wParam == VK_DELETE))
       { OnMenuDelete();
         ::TranslateMessage(pMsg);
         ::DispatchMessage(pMsg);
         return TRUE;  // Do Not Process Further
       }
    }

	return CDialog::PreTranslateMessage(pMsg);
}

HTREEITEM CConfLeftDlg::FindInTree(HTREEITEM hRootItem, LPCTSTR szFindText)
{ HTREEITEM hNewItem;

  if ( (!hRootItem) || (!szFindText) )
	 return(NULL);

  hNewItem = m_tree_ctrl.GetChildItem(hRootItem);
  while (hNewItem)
  { CString strItem; 
    strItem = m_tree_ctrl.GetItemText(hNewItem);
	
	if (strItem.CompareNoCase(szFindText) == 0)
      return hNewItem;
	
    hNewItem = m_tree_ctrl.GetNextSiblingItem(hNewItem);
  }

  return NULL;  // Not Found - returning NULL

}

void CConfLeftDlg::OnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{ 
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CConfListViewLeft* pView = (CConfListViewLeft*)GetParent();
	ASSERT (pView);
	CSplitterWnd* pSplitter = (CSplitterWnd*)pView->GetParent();
	ASSERT (pSplitter);
	CConfigFrame* pFrame = (CConfigFrame*)pSplitter->GetParent();
	ASSERT (pFrame);
	if (!pFrame->IsAllViewsCreated())
	  return;
	CConfViewRight* pViewRight = (CConfViewRight*)pSplitter->GetPane (0,1);
	if (!pViewRight && !IsWindow (pViewRight->m_hWnd))
	  return;

	if ((pNMTreeView->itemOld).hItem)
   	{ CuCbfListViewItem* pItemOld = (CuCbfListViewItem*)m_tree_ctrl.GetItemData ((pNMTreeView->itemOld).hItem);
      if(pItemOld)  // Try to validate the old active page
	  { (pViewRight->GetCConfRightDlg())->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, (LPARAM)pItemOld);
        BOOL bModified = pItemOld->LowLevelDeactivationWithCheck();
      }
	}
     
   // CString x; 
   // x = m_tree_ctrl.GetItemText((pNMTreeView->itemNew).hItem);
   // AfxMessageBox(x);
   // Activate The Page
   CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_tree_ctrl.GetItemData ((pNMTreeView->itemNew).hItem);
   if (!pItem)
   { pViewRight->DisplayPage(NULL);
     // Set Pane To Blank
     return;
   }

   CWaitCursor GlassHour;
   BOOL bSuccess = pItem->LowLevelActivationWithCheck();
   ASSERT (bSuccess);


   CuPageInformation* pPageInfo = pItem->GetPageInformation();
   ASSERT (pPageInfo);
   pPageInfo->SetCbfItem (pItem);
   pViewRight->DisplayPage(pPageInfo);

   *pResult = 0;
}


BOOL CConfLeftDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{ HTREEITEM	hti = NULL;

	switch (LOWORD(wParam)) {
		case ID_MENU_EDITNAME:
		  hti = m_tree_ctrl.GetSelectedItem();
          if (hti)
             m_tree_ctrl.EditLabel(hti);
		  break;

		case ID_MENU_EDITCOUNT:
		  OnMenuProperties(TRUE);
		  break;

        case ID_MENU_DUPLICATE_DEFAULT:
		  hti = FindInTree(m_tree_ctrl.GetParentItem(m_tree_ctrl.GetSelectedItem()), GetLocDefConfName());
          if (hti)
             OnMenuDuplicate(hti);
		  break;

		case ID_MENU_DUPLICATE:
		  hti = m_tree_ctrl.GetSelectedItem();
          if (hti)
             OnMenuDuplicate(hti);
		  break;

		case ID_MENU_DELETE:
		  OnMenuDelete();
		  break;

        case ID_MENU_PROPERTIES:
          OnMenuProperties();
          break;
         
		default: break;
	}
	
	return CDialog::OnCommand(wParam, lParam);
}

void CConfLeftDlg::OnBeginlabeleditTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
  // TODO: Add your control notification handler code here
  
  HTREEITEM hti = m_tree_ctrl.GetSelectedItem();
  if (!hti) return;  // Ignore clicks that are not on valid items

  CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_tree_ctrl.GetItemData (hti);
  if (!pItem)
  { *pResult = 1;
    return;
  }

  // Do not allow direct editing of labels in certain cases
  if (VCBFllCanNameBeEdited (&pItem->m_componentInfo))
  { CEdit *pEdit=m_tree_ctrl.GetEditControl();
    if (pEdit)
       pEdit->SetLimitText(79);  /// Use constant
    *pResult = 0;   
  }
  else
    *pResult = 1;  

}

void CConfLeftDlg::OnMenuDuplicate(HTREEITEM hDupItem)
{ 
  CuCbfListViewItem* pItemData1 = (CuCbfListViewItem*)m_tree_ctrl.GetItemData(hDupItem);
  if (!pItemData1)
  { MessageBeep(MB_ICONEXCLAMATION);
    return;
  }

  int iImageIndex, iSelectedImage;
  BOOL bOK = m_tree_ctrl.GetItemImage(hDupItem, iImageIndex, iSelectedImage);

  LPCOMPONENTINFO lpComponentInfo = &pItemData1->m_componentInfo;
  COMPONENTINFO   dupComponentInfo;
  memcpy(&dupComponentInfo, lpComponentInfo, sizeof(COMPONENTINFO));
  int iSearch = -1;
  long lnMaxInternalName;
  CString strTemplate = lpComponentInfo->sztype;
  iSearch = strTemplate.Find (_T(' '));
  if (iSearch > 0)
  	strTemplate = strTemplate.Left (iSearch);

  GetMaxName((LPCTSTR)strTemplate, lnMaxInternalName, hDupItem);
  CString strGenerateName;
  strTemplate.MakeLower();
  strGenerateName.Format (_T("%s%ld"), (LPCTSTR)strTemplate, lnMaxInternalName+1);   // Emb 22/10/97: no underscore
  _tcscpy((LPTSTR)dupComponentInfo.szname, (LPCTSTR)strGenerateName);

  // AfxGetApp()->DoWaitCursor(1);
  
  CString strText;
  strText.Format(IDS_WAIT_CREATING, dupComponentInfo.szname, (LPCTSTR)m_tree_ctrl.GetItemText(hDupItem));
  CWaitDlg dlgWait(strText, IDR_COPY_AVI, IDD_WAIT_DIALOG);
  
  BOOL bSuccess = VCBFllValidDuplicate(lpComponentInfo, &dupComponentInfo, dupComponentInfo.szname);
  // AfxGetApp()->DoWaitCursor(-1);

  if (!bSuccess)
  {	dlgWait.DestroyWindow();
    return;
  }
 
  _tcscpy((LPTSTR)dupComponentInfo.szname, (LPTSTR)(LPCTSTR)strGenerateName);  // Must copy again!
		
  // here we can create a duplicate line
  CuCbfListViewItem* pItemData2 = (CuCbfListViewItem*)pItemData1->Copy();
  _tcscpy ((LPTSTR)dupComponentInfo.szcount, _T("0"));  // Reset to Zero when duplicating
  pItemData2->SetComponentInfo(&dupComponentInfo);
  pItemData2->SetConfLeftDlg(this);

  TCHAR szDisplayString[512], szBuffer[512];
  if (VCBFllCanCompBeDuplicated (&pItemData2->m_componentInfo))
  { _tcscpy(szDisplayString, (LPTSTR)dupComponentInfo.szname);
  }
  else
  { // RETURN!  sprintf(szDisplayString, "%s : \"%s\"", (LPTSTR)dupComponentInfo.sztype, (LPTSTR)dupComponentInfo.szname);
  }
    
  // Add Items To TreeView, If the parent exists, use it
  HTREEITEM hFindItem, hNewItem;
  hFindItem = FindInTree(m_tree_ctrl.GetRootItem(), (LPCTSTR)dupComponentInfo.sztype);
	
  // This should never happen... comment out later  RETURN!
  if ( (!hFindItem) && (VCBFllCanCompBeDuplicated (&pItemData2->m_componentInfo)) )
  { _stprintf(szBuffer, _T("%s"), (LPTSTR)dupComponentInfo.sztype);
    AddItemToTree(szBuffer, m_tree_ctrl.GetRootItem(), TRUE, 0, NULL);
  }

  hNewItem = AddItemToTree(szDisplayString, hFindItem, FALSE, iImageIndex, pItemData2);
  m_tree_ctrl.Select(hNewItem,TVGN_CARET);
  
  dlgWait.DestroyWindow();
  
  hNewItem = m_tree_ctrl.GetSelectedItem();
  if (hNewItem)
     m_tree_ctrl.EditLabel(hNewItem);
  
}

void CConfLeftDlg::OnEndlabeleditTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
   TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;

   HTREEITEM hti = m_tree_ctrl.GetSelectedItem();
   if (!hti)
     *pResult = FALSE;  // Returning True Allows The Change

   CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_tree_ctrl.GetItemData (hti);
   if (!pItem)
   { MessageBeep(MB_ICONEXCLAMATION);
     return;
   }
    
   CString s = pTVDispInfo->item.pszText;
	
   AfxGetApp()->DoWaitCursor(1);
   BOOL bSuccess = VCBFllValidRename(&(pItem->m_componentInfo), (LPTSTR)(LPCTSTR)s);
   AfxGetApp()->DoWaitCursor(-1);

   if (bSuccess)
     *pResult = TRUE;  // Returning True Allows The Change
   else
   { *pResult = FALSE;
     //AfxMessageBox("Error: Failed on rename");
   }

}

void CConfLeftDlg::OnMenuDelete()
{ HTREEITEM hti;
  CString strMessage, strLabel;
  BOOL bSuccess;
  int rc;

  hti = m_tree_ctrl.GetSelectedItem();
  if (!hti) 
     return;

  strLabel=m_tree_ctrl.GetItemText(hti);

  CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_tree_ctrl.GetItemData (hti);
  if (!pItem)
  { MessageBeep(MB_ICONEXCLAMATION);
    return;
  }

  BOOL bDelete = VCBFllCanCompBeDeleted (&pItem->m_componentInfo);
  if (!bDelete)
  {
    AfxFormatString1(strMessage,IDS_ERR_DELETE_ITEM,strLabel);
    MessageBeep(MB_ICONEXCLAMATION);
    AfxMessageBox(strMessage);
    return;
  }
 
  AfxFormatString1(strMessage,IDS_CONFIRM_DELETE,strLabel);
  MessageBeep(MB_ICONEXCLAMATION);
  rc=AfxMessageBox(strMessage, MB_YESNO);
  if (rc == IDNO)
     return;
  
  CString strText;
  strText.Format(IDS_WAIT_DELETE, (LPCTSTR)strLabel);
  CWaitDlg dlgWait(strText, IDR_DELETE_AVI, IDD_WAIT_DIALOG);

  LPCOMPONENTINFO lpComponentInfo = &pItem->m_componentInfo;
  AfxGetApp()->DoWaitCursor(1);
  bSuccess = VCBFllValidDelete(lpComponentInfo);
  AfxGetApp()->DoWaitCursor(-1);

  if (!bSuccess)
  { CString strMessage;
    AfxFormatString1(strMessage,IDS_CHANGE_STARTUP_COUNT,lpComponentInfo->szcount);
    int rc=AfxMessageBox(strMessage, MB_YESNO | MB_ICONQUESTION);

    if (rc == IDNO)
    { dlgWait.DestroyWindow();
      return;
    }

    Sleep(3000);
    
    CString s=_T("0");
    bSuccess = VCBFllValidCount(lpComponentInfo, (LPTSTR)(LPCTSTR)s);
    if (!bSuccess)
      return;

    bSuccess = VCBFllValidDelete(lpComponentInfo);
    if (!bSuccess)
      return;

  }

  m_tree_ctrl.DeleteItem(hti);  

  dlgWait.DestroyWindow();
    
}

void CConfLeftDlg::GetMaxName(LPCTSTR lpszTemplateName, long& lnMaxCount, HTREEITEM hti)
{ HTREEITEM hParentItem, hNewItem;
  int iSearch;

  lnMaxCount= 0L;
  CString strTemplate = lpszTemplateName;
		
  iSearch = strTemplate.Find (_T(' '));
  if (iSearch > 0)
	strTemplate = strTemplate.Left (iSearch);
		
  int nLen = (lpszTemplateName) ? lstrlen (lpszTemplateName): 0;

  hParentItem = m_tree_ctrl.GetParentItem(hti);
   	
  hNewItem = m_tree_ctrl.GetChildItem(hParentItem);
  while (hNewItem)
  { CString strItem; 
    strItem = m_tree_ctrl.GetItemText(hNewItem);

	int iLen = strItem.GetLength();

    // count how many digits from the tail of the string
	//
	// DBCS Compliant
	int cpt = 0;
	for (cpt = iLen-1; cpt>=0; cpt--)
	   if (!_istdigit(strItem[cpt]))
	     break;
			
	if (cpt >= 0)  // drop if only digits in name

    { // compare left part of string (without the trailing digits), to template
	  CString sLeft = strItem.Left(cpt+1);
	  if (sLeft.CompareNoCase (strTemplate) == 0)
	  {
	    // pick trailing number and keep maximum value
	    ASSERT (iLen-cpt-1 > 0 );
	    CString sNum = strItem.Right(iLen-cpt-1);
	    long lVal = 0L;
	    lVal = _ttol ((LPCTSTR)sNum);
	    lnMaxCount  = max (lnMaxCount, lVal);
      }
  	}

    hNewItem = m_tree_ctrl.GetNextSiblingItem(hNewItem);
  }

}

void CConfLeftDlg::OnMenuProperties(BOOL bCount)
{ CString strText;
  CString strOrigType, strOrigName, strOrigCount;
  CString strNewType, strNewName, strNewCount;
  bool bChangeName=TRUE;
  bool bChangeCount=TRUE;

  HTREEITEM hti = m_tree_ctrl.GetSelectedItem();
  if (!hti)
    return;

  CuCbfListViewItem* pItem = (CuCbfListViewItem*)m_tree_ctrl.GetItemData (hti);
  if (!pItem)  // Ignore if there is no data
    return;
	      
  strOrigType=pItem->m_componentInfo.sztype;
  strOrigName=pItem->m_componentInfo.szname;
  strOrigCount=pItem->m_componentInfo.szcount;

///setlimittext &validatedata

  CConfigLeftProperties dlg;
  dlg.m_ItemData=pItem;
  dlg.m_bFocusCount = bCount;
     
  if (dlg.DoModal() == IDOK)
  { 
	strNewType  = dlg.m_Component;
    strNewName  = dlg.m_Name;
    strNewCount = dlg.m_StartupCount;
    
    if (strOrigName.CompareNoCase(strNewName) == 0)
       bChangeName=FALSE;

    if (strOrigCount.CompareNoCase(strNewCount) == 0)
       bChangeCount=FALSE;
  
	// optimization: if text not changed or if text empty, behave as if cancelled
	if (bChangeCount && strNewCount.IsEmpty())
	  return;

	int len = strNewCount.GetLength();
	if (len > 1) {
		int cpt = 0;
		for (cpt=0; cpt<len; cpt++)
			if (strNewCount[cpt] != _T('0'))
				break;
		if (cpt == len)
			strNewCount = _T("0");  // Only zeros found
		else
		strNewCount = strNewCount.Right(len-cpt);  // Start from first non-zero digit
	}

    if (bChangeName || bChangeCount)
    { strText.Format(IDS_WAIT_UPDATING, (LPCTSTR)strNewName);
      CWaitDlg dlgWait(strText, IDR_CLOCK_AVI, IDD_WAIT_DIALOG1);

      LPCOMPONENTINFO lpComponentInfo = &pItem->m_componentInfo;
      try
      {
	     if (bChangeCount)
         { AfxGetApp()->DoWaitCursor(1);
	       BOOL bSuccess = VCBFllValidCount(lpComponentInfo, (LPTSTR)(LPCTSTR)strNewCount);
	       AfxGetApp()->DoWaitCursor(-1);
	       if (!bSuccess)
	          MessageBeep(MB_ICONEXCLAMATION);
         }
      }
      catch (CeVcbfException e)
      {
	      //
	      // Catch critical error 
	      TRACE1 ("CuEditableListCtrlComponent::OnEditSpinDlgOK has caught exception: %s\n", e.m_strReason);
	      CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
	      pMain->CloseApplication (FALSE);
      }
      catch (CMemoryException* e)
      {
	      VCBF_OutOfMemoryMessage ();
	      e->Delete();
      }
      catch (...)
      {
	      TRACE0 ("Other error occured ...\n");
      }

      
      if (bChangeName)
      { BOOL bSuccess = VCBFllValidRename(&(pItem->m_componentInfo), (LPTSTR)(LPCTSTR)strNewName);	

        if (bSuccess)  // Change tree control if successful
          m_tree_ctrl.SetItemText(hti, strNewName);
      }

      dlgWait.DestroyWindow();
  
      // Notify right pane of the change made in this dialog
      CConfListViewLeft* pView = (CConfListViewLeft*)GetParent();
      ASSERT (pView);
      CSplitterWnd* pSplitter = (CSplitterWnd*)pView->GetParent();
      ASSERT (pSplitter);
      CConfigFrame* pFrame = (CConfigFrame*)pSplitter->GetParent();
      ASSERT (pFrame);
      if (!pFrame->IsAllViewsCreated())
	     return;
      CConfViewRight* pViewRight = (CConfViewRight*)pSplitter->GetPane (0,1);
      if (!pViewRight && !IsWindow (pViewRight->m_hWnd))
         return;
      (pViewRight->GetCConfRightDlg())->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, (LPARAM)pItem);

    } // if (bChangeName || bChangeCount)

 
  }

}
