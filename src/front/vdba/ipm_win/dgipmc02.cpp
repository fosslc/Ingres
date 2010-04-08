/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipmc02.cpp, Implementation file 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog Box that will appear at the right pane of the 
**               Ipm Frame.
**               It contains a TabCtrl and maintains the pages (Modeless Dialogs)
**               that will appeare at each tab. It also keeps track of the current 
**               object being displayed. When we get the new object to display,
**               we check if it is the same Class of the Current Object for the 
**               purpose of just updating the data of the current page.
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
** 12-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Show/Hide the control m_staticHeader depending on 
**    CMainMfcApp::GetShowRightPaneHeader()
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include <stdlib.h>
#include "wmusrmsg.h"
#include "ipmdoc.h"
#include "ipmframe.h"
#include "viewleft.h"
#include "dgipmc02.h"
#include "ipmctl.h"

#include "dgipacdb.h"
#include "dgipllis.h"
#include "dgiplock.h"
#include "dgiplogf.h"
#include "dgiplres.h"
#include "dgipltbl.h"
#include "dgipproc.h"
#include "dgipsess.h"
#include "dgiptran.h"
#include "dlgvfrm.h"
#include "ipmdisp.h"
//
// For Replicator.
#include "rsactivi.h"
#include "rscollis.h"
#include "rsdistri.h"
#include "rsserver.h"
#include "rsrevent.h"
#include "rsintegr.h"
#include "rvstatus.h"
#include "rvstartu.h"
#include "rvrevent.h"
#include "rvassign.h"

// Pages on root static items
#include "dgipstsv.h"
#include "dgipstdb.h"
#include "dgipstus.h"
#include "dgipstre.h"

//
// Ice Monitoring:
#include "dgipicht.h"
#include "dgipictr.h"
#include "dgipcusr.h"
#include "dgipiccu.h"
#include "dgipiccp.h"
#include "dgipicac.h"
#include "dgipicdc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline static int compare (const void* e1, const void* e2)
{
	UINT o1 = *((UINT*)e1);
	UINT o2 = *((UINT*)e2);
	return (o1 > o2)? 1: (o1 < o2)? -1: 0;
}

static int Find (UINT nIDD, DLGSTRUCT* pArray, int first, int last)
{
	if (first <= last)
	{
		int mid = (first + last) / 2;
		if (pArray [mid].nIDD == nIDD)
			return mid;
		else
		if (pArray [mid].nIDD > nIDD)
			return Find (nIDD, pArray, first, mid-1);
		else
			return Find (nIDD, pArray, mid+1, last);
	}
	return -1;
}


CuDlgIpmTabCtrl::CuDlgIpmTabCtrl(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmTabCtrl::IDD, pParent)
{
	UINT nArrayIDD [NUMBEROFPAGES] = 
	{
		IDD_IPMDETAIL_LOCK,             // Detail of Lock
		IDD_IPMDETAIL_LOCKLIST,         // Detail of Lock List
		IDD_IPMDETAIL_LOCKINFO,         // Lock Information summary
		IDD_IPMDETAIL_LOGINFO,          // Log Information summary
		IDD_IPMDETAIL_PROCESS,          // Detail of Process
		IDD_IPMDETAIL_RESOURCE,         // Detail of Resource
		IDD_IPMDETAIL_SERVER,           // Detail of Server
		IDD_IPMDETAIL_SESSION,          // Detail of Session
		IDD_IPMDETAIL_TRANSACTION,      // Detail of Transaction
		
		IDD_IPMPAGE_ACTIVEDB,           // Page of Active Database (List)
		IDD_IPMPAGE_CLIENT,             // Page of Client of a session.
		IDD_IPMPAGE_HEADER,             // Page of Header
		IDD_IPMPAGE_LOCKEDRESOURCES,    // Page of Lock Resource (List)
		IDD_IPMPAGE_LOCKEDTABLES,       // Page of Lock Table (List)
		IDD_IPMPAGE_LOCKLISTS,          // Page of Lock List (List)
		IDD_IPMPAGE_LOCKS,              // Page of Lock (List)
		IDD_IPMPAGE_LOGFILE,            // Page of Log File
		IDD_IPMPAGE_PROCESSES,          // Page of Process (List)
		IDD_IPMPAGE_SESSIONS,           // Page of Session (List)
		IDD_IPMPAGE_TRANSACTIONS,       // Page of Transaction (List)

		IDD_REPSTATIC_PAGE_ACTIVITY,    
		IDD_REPSTATIC_PAGE_RAISEEVENT, 
		IDD_REPSTATIC_PAGE_DISTRIB, 
		IDD_REPSTATIC_PAGE_COLLISION,
		IDD_REPSTATIC_PAGE_SERVER,
		IDD_REPSTATIC_PAGE_INTEGRITY,  
		
		IDD_REPSERVER_PAGE_STATUS,
		IDD_REPSERVER_PAGE_STARTSETTING,
		IDD_REPSERVER_PAGE_RAISEEVENT, 
		IDD_REPSERVER_PAGE_ASSIGNMENT,

		// Pages on root static items
		IDD_IPMPAGE_STATIC_SERVERS,
		IDD_IPMPAGE_STATIC_DATABASES,
		IDD_IPMPAGE_STATIC_ACTIVEUSERS,
		IDD_IPMPAGE_STATIC_REPLICATIONS,

		//
		// Ice Monitoring:
		IDD_IPMICEDETAIL_ACTIVEUSER,
		IDD_IPMICEDETAIL_CACHEPAGE,
		IDD_IPMICEDETAIL_CONNECTEDUSER,
		IDD_IPMICEDETAIL_CURSOR,
		IDD_IPMICEDETAIL_DATABASECONNECTION,
		IDD_IPMICEDETAIL_HTTPSERVERCONNECTION,
		IDD_IPMICEDETAIL_TRANSACTION,

		IDD_IPMICEPAGE_ACTIVEUSER,
		IDD_IPMICEPAGE_CACHEPAGE,
		IDD_IPMICEPAGE_CONNECTEDUSER,
		IDD_IPMICEPAGE_CURSOR,
		IDD_IPMICEPAGE_DATABASECONNECTION,
		IDD_IPMICEPAGE_HTTPSERVERCONNECTION,
		IDD_IPMICEPAGE_TRANSACTION
	};

	qsort ((void*)nArrayIDD, (size_t)NUMBEROFPAGES, (size_t)sizeof(UINT), compare);
	for (int i=0; i<NUMBEROFPAGES; i++)
	{
		m_arrayDlg[i].nIDD = nArrayIDD [i];
		m_arrayDlg[i].pPage= NULL;
	}
	m_pCurrentProperty = NULL;
	m_pCurrentPage = NULL;
	m_bIsLoading   = FALSE;
	//{{AFX_DATA_INIT(CuDlgIpmTabCtrl)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmTabCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmTabCtrl)
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	//}}AFX_DATA_MAP
}

CWnd* CuDlgIpmTabCtrl::ConstructPage (UINT nIDD)
{
	switch (nIDD)
	{
	case IDD_IPMDETAIL_LOCKINFO:
	case IDD_IPMDETAIL_LOGINFO:
	case IDD_IPMDETAIL_LOCK:
	case IDD_IPMDETAIL_LOCKLIST:
	case IDD_IPMDETAIL_RESOURCE:
	case IDD_IPMDETAIL_PROCESS:
	case IDD_IPMDETAIL_SERVER:
	case IDD_IPMDETAIL_SESSION:
	case IDD_IPMDETAIL_TRANSACTION:
	case IDD_IPMPAGE_CLIENT:
	case IDD_IPMPAGE_HEADER:
		return CreateFormViewPage(nIDD);

	case IDD_IPMPAGE_ACTIVEDB:          return Page_ActiveDB         (nIDD);
	case IDD_IPMPAGE_LOCKEDRESOURCES:   return Page_LockedResources  (nIDD);
	case IDD_IPMPAGE_LOCKEDTABLES:      return Page_LockedTables     (nIDD);
	case IDD_IPMPAGE_LOCKLISTS:         return Page_LockLists        (nIDD);
	case IDD_IPMPAGE_LOCKS:             return Page_Locks            (nIDD);
	case IDD_IPMPAGE_LOGFILE:           return Page_LogFile          (nIDD);
	case IDD_IPMPAGE_PROCESSES:         return Page_Processes        (nIDD);
	case IDD_IPMPAGE_SESSIONS:          return Page_Sessions         (nIDD);
	case IDD_IPMPAGE_TRANSACTIONS:      return Page_Transactions     (nIDD);

	case IDD_REPSTATIC_PAGE_ACTIVITY:   return Page_RepStaticActivity (nIDD);
	case IDD_REPSTATIC_PAGE_RAISEEVENT: return Page_RepStaticEvent    (nIDD);
	case IDD_REPSTATIC_PAGE_DISTRIB:    return Page_RepStaticDistrib  (nIDD);
	case IDD_REPSTATIC_PAGE_COLLISION:  return Page_RepStaticCollision(nIDD);
	case IDD_REPSTATIC_PAGE_SERVER:     return Page_RepStaticServer   (nIDD);
	case IDD_REPSTATIC_PAGE_INTEGRITY:  return Page_RepStaticIntegerity(nIDD);
	
	case IDD_REPSERVER_PAGE_STATUS:      return Page_RepServerStatus   (nIDD);
	case IDD_REPSERVER_PAGE_STARTSETTING:return Page_RepServerStartupSetting(nIDD);
	case IDD_REPSERVER_PAGE_RAISEEVENT:  return Page_RepServerEvent    (nIDD);
	case IDD_REPSERVER_PAGE_ASSIGNMENT:  return Page_RepServerAssignment(nIDD);

	// Pages on root static items
	case IDD_IPMPAGE_STATIC_SERVERS:        return Page_StaticServers(nIDD);
	case IDD_IPMPAGE_STATIC_DATABASES:      return Page_StaticDatabases(nIDD);
	case IDD_IPMPAGE_STATIC_ACTIVEUSERS:    return Page_StaticActiveusers(nIDD);
	case IDD_IPMPAGE_STATIC_REPLICATIONS:   return Page_StaticReplication(nIDD);
	//
	// Ice Monitoring:
	case IDD_IPMICEDETAIL_ACTIVEUSER:
	case IDD_IPMICEDETAIL_CACHEPAGE:
	case IDD_IPMICEDETAIL_CONNECTEDUSER:
	case IDD_IPMICEDETAIL_CURSOR:
	case IDD_IPMICEDETAIL_DATABASECONNECTION:
	case IDD_IPMICEDETAIL_HTTPSERVERCONNECTION:
	case IDD_IPMICEDETAIL_TRANSACTION:
		return CreateFormViewPage(nIDD);

	case IDD_IPMICEPAGE_ACTIVEUSER:            return IcePage_ActiveUser(nIDD);
	case IDD_IPMICEPAGE_CACHEPAGE:             return IcePage_CachePage(nIDD);
	case IDD_IPMICEPAGE_CONNECTEDUSER:         return IcePage_ConnectedUser(nIDD);
	case IDD_IPMICEPAGE_CURSOR:                return IcePage_Cursor(nIDD);
	case IDD_IPMICEPAGE_DATABASECONNECTION:    return IcePage_DatabaseConnection (nIDD);
	case IDD_IPMICEPAGE_HTTPSERVERCONNECTION:  return IcePage_HttpServerConnection (nIDD);
	case IDD_IPMICEPAGE_TRANSACTION:           return IcePage_Transaction(nIDD);

	default: 
		return NULL;
	}
	
	return NULL;
}

CWnd* CuDlgIpmTabCtrl::GetPage (UINT nIDD)
{
	int index = Find (nIDD, m_arrayDlg, 0, NUMBEROFPAGES -1);
	if (index == -1)
	{
		ASSERT (FALSE); // You must define page !!
		return NULL;
	}
	else
	{
		if (m_arrayDlg[index].pPage)
			return m_arrayDlg[index].pPage;
		else
		{
			m_arrayDlg[index].pPage = ConstructPage (nIDD);
			return m_arrayDlg[index].pPage;
		}
	}
	
	for (int i=0; i<NUMBEROFPAGES; i++)
	{
		if (m_arrayDlg[i].nIDD == nIDD)
		{
			if (m_arrayDlg[i].pPage)
				return m_arrayDlg[i].pPage;
			m_arrayDlg[i].pPage = ConstructPage (nIDD);
			return m_arrayDlg[i].pPage;
		}
	}
	//
	// Call GetPage, you must define some dialog box...
	ASSERT (FALSE);
	return NULL;
}

void CuDlgIpmTabCtrl::DisplayPage (CuPageInformation* pPageInfo, LPCTSTR lpszHeader, int nSel)
{
	CRect   r;
	TCITEM  item;
	int     i, nPages;
	UINT*   nTabID;
	UINT*   nDlgID;
	CString strTab;
	CString strTitle;
	UINT    nIDD; 
	CWnd*   pDlg;
	try
	{
		CView*   pView = (CView*)GetParent();
		ASSERT (pView);
		CdIpmDoc* pDoc  = (CdIpmDoc*)pView->GetDocument();
		ASSERT (pDoc);
		
		if (m_bIsLoading)
		{
			m_bIsLoading = FALSE;
			pDoc->SetLoadDoc(FALSE);
			// Masqued Emb April 9, 97 for bug fix
			// return;
		}
		if (!pPageInfo)
		{
			if (m_pCurrentPage)
				m_pCurrentPage->ShowWindow (SW_HIDE);
			m_cTab1.DeleteAllItems();
			if (m_pCurrentProperty)
			{
				m_pCurrentProperty->SetPageInfo (NULL);
				delete m_pCurrentProperty;
				m_pCurrentProperty = NULL;
				m_pCurrentPage     = NULL;
			}

			if (lpszHeader)
				m_staticHeader.SetWindowText (lpszHeader);
			else
				m_staticHeader.SetWindowText (_T(""));
			UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
			m_staticHeader.ResetBitmap (NULL); // No Image
			m_staticHeader.SetWindowPos (NULL, 0, 0, 0, 0, uFlags);
			return;
		}
		ASSERT (!lpszHeader);
		if (!m_pCurrentProperty)
		{
			m_pCurrentProperty = new CuIpmProperty (0, pPageInfo);
			//
			// Set up the Title:
			pPageInfo->GetTitle (strTitle);
			m_staticHeader.SetWindowText (strTitle);

			UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
			if (!strTitle.IsEmpty())
			{
				switch (pPageInfo->GetImageType())
				{
				case 0:
					m_staticHeader.ResetBitmap (pPageInfo->GetIconImage());
					break;
				case 1:
					m_staticHeader.ResetBitmap (pPageInfo->GetBitmapImage(), pPageInfo->GetImageMask());
					break;
				default:
					m_staticHeader.ResetBitmap (NULL);
				}
			}
			else
				m_staticHeader.ResetBitmap (NULL);
			m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);
			
			nPages = pPageInfo->GetNumberOfPage();
			if (nPages < 1)
			{
				m_pCurrentPage = NULL;
				return;
			}
			//
			// Construct the Tab(s)
			nTabID = pPageInfo->GetTabID();
			nDlgID = pPageInfo->GetDlgID();
			memset (&item, 0, sizeof (item));
			item.mask       = TCIF_TEXT;
			item.cchTextMax = 32;
			m_cTab1.DeleteAllItems();
			for (i=0; i<nPages; i++)
			{
				strTab.LoadString (nTabID [i]);
				item.pszText = (LPTSTR)(LPCTSTR)strTab;
				m_cTab1.InsertItem (i, &item);
			}
			//
			// Display the default (the first) page, except if context-driven specified tab
			int initialSel = (nSel != -1)? nSel: 0;
			if (initialSel >= nPages)
				initialSel = 0;

			nIDD = pPageInfo->GetDlgID (initialSel); 
			pDlg= GetPage (nIDD);
			m_cTab1.SetCurSel (initialSel);
			m_cTab1.GetClientRect (r);
			m_cTab1.AdjustRect (FALSE, r);
			pDlg->MoveWindow (r);
			pDlg->ShowWindow(SW_SHOW);
			m_pCurrentPage = pDlg;
			//
			// Properties:
			UINT nMask = IPMMASK_FONT|IPMMASK_SHOWGRID;
			m_pCurrentPage->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&(pDoc->GetProperty()));
			//
			// Update data:
			m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pDoc, (LPARAM)pPageInfo->GetUpdateParam());
		}
		else // !m_pCurrentProperty
		{
			CuPageInformation* pCurrentPageInfo = m_pCurrentProperty->GetPageInfo();
			pPageInfo->GetTitle (strTitle);
			m_staticHeader.SetWindowText (strTitle);
		
			UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
			if (!strTitle.IsEmpty())
			{
				switch (pPageInfo->GetImageType())
				{
				case 0:
					m_staticHeader.ResetBitmap (pPageInfo->GetIconImage());
					break;
				case 1:
					m_staticHeader.ResetBitmap (pPageInfo->GetBitmapImage(), pPageInfo->GetImageMask());
					break;
				default:
					m_staticHeader.ResetBitmap (NULL);
				}
			}
			else
				m_staticHeader.ResetBitmap (NULL);
			m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);

			// Cloned from dgdomc02.cpp : pCurrentPageInfo can have become NULL
			// example: current selection was system obj, and system checkbox has been unchecked
			BOOL bNoMoreCurrentClassName = FALSE;
			CString CurrentClassName;
			if (pCurrentPageInfo)
				CurrentClassName = pCurrentPageInfo->GetClassName();
			else
			{
				bNoMoreCurrentClassName = TRUE;
				CurrentClassName = _T("No More Current Class Name!!!");
			}
			CString ClassName        = pPageInfo->GetClassName();
			if ((CurrentClassName == ClassName) && (!bNoMoreCurrentClassName) )
			{
				m_pCurrentProperty->SetPageInfo (pPageInfo);
				//
				// wParam, and lParam will contain the information needed
				// by the dialog to refresh itself.
				if (m_pCurrentPage) 
					m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pDoc, (LPARAM)pPageInfo->GetUpdateParam());
				//
				// Special re-initialize the Activity page of a Static Replication Item
				// It is not optimal, but we have to do like this due to no specification for replication.
				int i;
				for (i=0; i<pPageInfo->GetNumberOfPage(); i++)
				{
					nIDD = pPageInfo->GetDlgID (i); 
					if (nIDD == IDD_REPSTATIC_PAGE_ACTIVITY)
					{
						pDlg= GetPage (nIDD);
						//
						// Properties:
						UINT nMask = IPMMASK_FONT|IPMMASK_SHOWGRID;
						pDlg->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&(pDoc->GetProperty()));

						//
						// Update data:
						pDlg->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pDoc, (LPARAM)pPageInfo->GetUpdateParam());
						break;
					}
				}
			}
			else
			{
				int initialSel = (nSel != -1)? nSel: 0;
				if (initialSel >= pPageInfo->GetNumberOfPage())
					initialSel = 0;
				if (m_pCurrentPage) 
					m_pCurrentPage->ShowWindow (SW_HIDE);
				m_cTab1.DeleteAllItems();
				nPages = pPageInfo->GetNumberOfPage();
				m_pCurrentProperty->SetPageInfo (pPageInfo);
				m_pCurrentProperty->SetCurSel (initialSel);
				
				if (nPages < 1)
				{
					m_pCurrentPage = NULL;
					return;
				}
				UINT nIDD; 
				CWnd* pDlg;
				//
				// Construct the Tab(s)
				nTabID = pPageInfo->GetTabID();
				nDlgID = pPageInfo->GetDlgID();
				memset (&item, 0, sizeof (item));
				item.mask       = TCIF_TEXT;
				item.cchTextMax = 32;
				m_cTab1.DeleteAllItems();
				for (i=0; i<nPages; i++)
				{
					strTab.LoadString (nTabID [i]);
					item.pszText = (LPTSTR)(LPCTSTR)strTab;
					m_cTab1.InsertItem (i, &item);
				}
				//
				// Display the default (the first) page.
				nIDD = pPageInfo->GetDlgID (initialSel); 
				pDlg= GetPage (nIDD);
				m_cTab1.SetCurSel (initialSel);
				m_cTab1.GetClientRect (r);
				m_cTab1.AdjustRect (FALSE, r);
				pDlg->MoveWindow (r);
				pDlg->ShowWindow(SW_SHOW);
				m_pCurrentPage = pDlg;
				//
				// Properties:
				UINT nMask = IPMMASK_FONT|IPMMASK_SHOWGRID;
				m_pCurrentPage->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&(pDoc->GetProperty()));
				//
				// Update Data:
				m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pDoc, (LPARAM)pPageInfo->GetUpdateParam());
			}
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		if (m_pCurrentPage)
			m_pCurrentPage->ShowWindow (SW_HIDE);
		if (m_pCurrentProperty)
		{
			m_pCurrentProperty->SetPageInfo (NULL);
			delete m_pCurrentProperty;
		}
		m_pCurrentPage = NULL;
		m_pCurrentProperty = NULL;
		e->Delete();
		return;
	}
	catch (CResourceException* e)
	{
		AfxMessageBox (IDS_E_LOAD_RESOURCE);//"Load resource failed"
		if (m_pCurrentPage)
			m_pCurrentPage->ShowWindow (SW_HIDE);
		if (m_pCurrentProperty)
		{
			m_pCurrentProperty->SetPageInfo (NULL);
			delete m_pCurrentProperty;
		}
		m_pCurrentPage = NULL;
		m_pCurrentProperty = NULL;
		e->Delete();
		return;
	}
}

void CuDlgIpmTabCtrl::NotifyPageSelChanging()
{
	if (m_pCurrentProperty)
	{
		if (m_pCurrentPage)
		{
			CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame();
			CIpmCtrl* pIpmCtrl = (CIpmCtrl*)pFrame->GetParent();
			if (pIpmCtrl)
				pIpmCtrl->ContainerNotifySelChange();
			m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, (WPARAM)LEAVINGPAGE_CHANGELEFTSEL, 0L);
		}
	}
}

void CuDlgIpmTabCtrl::NotifyPageDocClose()
{
	if (m_pCurrentProperty)
		if (m_pCurrentPage)
			m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, (WPARAM)LEAVINGPAGE_CLOSEDOCUMENT, 0L);
}


BEGIN_MESSAGE_MAP(CuDlgIpmTabCtrl, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmTabCtrl)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_WM_DESTROY()
	ON_NOTIFY(TCN_SELCHANGING, IDC_TAB1, OnSelchangingMfcTab1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING,   OnUpdateData)
	ON_MESSAGE (WM_LAYOUTEDIT_HIDE_PROPERTY, OnHideProperty)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING, OnSettingChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmTabCtrl message handlers
LONG CuDlgIpmTabCtrl::OnHideProperty (WPARAM wParam, LPARAM lParam)
{
	if (m_pCurrentPage)
	{
		m_pCurrentPage->SendMessage (WM_LAYOUTEDIT_HIDE_PROPERTY, wParam, lParam);
	}
	return 0L;
}

LONG CuDlgIpmTabCtrl::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	//
	// lParam containts the hint about the change.
	// Its value can be FILTER_RESOURCE_TYPE, FILTER_NULL_RESOURCES, ...
	if (m_pCurrentPage)
	{
		CuPageInformation* pPageInfo = m_pCurrentProperty->GetPageInfo();
		ASSERT (pPageInfo);
		LPIPMUPDATEPARAMS pUps = pPageInfo->GetUpdateParam();
		pUps->nIpmHint = (int)lParam;
		m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, wParam, (LPARAM)pUps);
	}
	return 0L;
}

void CuDlgIpmTabCtrl::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmTabCtrl::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_staticHeader.SubclassDlgItem (IDC_STATIC, this));
	m_staticHeader.SetImage (-1); // No Image for Now

	CWnd* pParent1 = GetParent();    // The view: CImView2
	ASSERT (pParent1);
	CdIpmDoc* pDoc = (CdIpmDoc*)((CView*)pParent1)->GetDocument();
	ASSERT (pDoc);
	if (!pDoc->GetCurrentProperty())
		return TRUE;

	//
	// When the document is Loaded ...
	try
	{
		CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame();
		CvIpmLeft*  pIpmView1= (CvIpmLeft*)pFrame->GetLeftPane();
		CTreeCtrl& treeCtrl = pIpmView1->GetTreeCtrl();
		HTREEITEM  hSelected= treeCtrl.GetSelectedItem ();
		ASSERT (hSelected);
		if (!hSelected)
			return TRUE;
		CTreeItem* pItem = (CTreeItem*)treeCtrl.GetItemData (hSelected);
		CuPageInformation* pPageInfo = pDoc->GetCurrentProperty()->GetPageInfo();
		LPIPMUPDATEPARAMS pUps = pPageInfo->GetUpdateParam();
		pUps->nType   = pItem->GetType();
		pUps->pStruct = pItem->GetPTreeItemData()? pItem->GetPTreeItemData()->GetDataPtr(): NULL;
		pUps->pSFilter= pDoc->GetFilter();

		int nImage = -1, nSelectedImage = -1;
		CImageList* pImageList = treeCtrl.GetImageList (TVSIL_NORMAL);
		HICON hIcon = NULL;
		int nImageCount = pImageList? pImageList->GetImageCount(): 0;
		if (pImageList && treeCtrl.GetItemImage(hSelected, nImage, nSelectedImage))
		{
			if (nImage < nImageCount)
				hIcon = pImageList->ExtractIcon(nImage);
		}
		pPageInfo->SetImage  (hIcon);

		LoadPage (pDoc->GetCurrentProperty());
		m_bIsLoading = TRUE;
		if (m_pCurrentPage)
		{
			UINT nMask = IPMMASK_FONT|IPMMASK_SHOWGRID;
			m_pCurrentPage->SendMessage (WMUSRMSG_CHANGE_SETTING, (UINT)nMask, (LPARAM)&(pDoc->GetProperty()));
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		m_pCurrentPage     = NULL;
		m_pCurrentProperty = NULL;
		e->Delete();
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmTabCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cTab1.m_hWnd) || !IsWindow (m_staticHeader.m_hWnd))
		return;
	CRect r, r2;
	GetClientRect (r);
	BOOL bShowRightPaneHeader = TRUE;
	if (bShowRightPaneHeader)
	{
		m_staticHeader.GetWindowRect (r2);
		ScreenToClient (r2);
		r2.top   = r.top;
		r2.left  = r.left;
		r2.right = r.right;
		m_staticHeader.MoveWindow (r2);
	}
	m_cTab1.GetWindowRect (r2);
	ScreenToClient (r2);
	if (!bShowRightPaneHeader)
	{
		r2.top = r.top;
		m_staticHeader.ShowWindow (SW_HIDE);
	}
	r2.left  = r.left;
	r2.right = r.right;
	r2.bottom= r.bottom;
	m_cTab1.MoveWindow (r2);
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage)
		m_pCurrentPage->MoveWindow (r);
}

BOOL CuDlgIpmTabCtrl::ChangeTab()
{
	int nSel;
	CRect r;
	CWnd* pNewPage;
	CuPageInformation* pPageInfo;
	CWnd* pParent1 = GetParent(); // The view: CImView2
	ASSERT (pParent1);
	CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame(); // The Frame Window
	ASSERT (pFrame);
	CdIpmDoc* pDoc  = (CdIpmDoc*)((CView*)pParent1)->GetDocument();
	ASSERT (pDoc);
	
	if (!pFrame->IsAllViewCreated())
		return FALSE;
	if (!m_pCurrentProperty)
		return FALSE;
	CWaitCursor doWaitCursor;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	nSel = m_cTab1.GetCurSel();
	m_pCurrentProperty->SetCurSel(nSel);
	pPageInfo = m_pCurrentProperty->GetPageInfo();
	try
	{
		pNewPage  = GetPage (pPageInfo->GetDlgID (nSel));
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return FALSE;
	}
	catch (CResourceException* e)
	{
		AfxMessageBox (IDS_E_LOAD_RESOURCE);//"Fail to load resource"
		e->Delete();
		return FALSE;
	}
	if (m_pCurrentPage)
		m_pCurrentPage->ShowWindow (SW_HIDE);
	m_pCurrentPage = pNewPage;
	m_pCurrentPage->MoveWindow (r);
	m_pCurrentPage->ShowWindow(SW_SHOW);
	//
	// Properties:
	UINT nMask = IPMMASK_FONT|IPMMASK_SHOWGRID;
	m_pCurrentPage->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&(pDoc->GetProperty()));
	//
	// Update Data:
	m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pDoc, (LPARAM)pPageInfo->GetUpdateParam());
	return TRUE;
}


void CuDlgIpmTabCtrl::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	if (!ChangeTab())
		*pResult = 1;
}


CWnd* CuDlgIpmTabCtrl::Page_ActiveDB(UINT nIDD)
{
	CuDlgIpmPageActiveDatabases* pDld = new CuDlgIpmPageActiveDatabases (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}


CWnd* CuDlgIpmTabCtrl::Page_LockedResources(UINT nIDD)
{
	CuDlgIpmPageLockResources* pDld = new CuDlgIpmPageLockResources (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_LockedTables(UINT nIDD)
{
	CuDlgIpmPageLockTables* pDld = new CuDlgIpmPageLockTables (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_LockLists(UINT nIDD)
{
	CuDlgIpmPageLockLists* pDld = new CuDlgIpmPageLockLists (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_Locks(UINT nIDD)
{
	CuDlgIpmPageLocks* pDld = new CuDlgIpmPageLocks (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_LogFile(UINT nIDD)
{
	CuDlgIpmPageLogFile* pDld = new CuDlgIpmPageLogFile (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}


CWnd* CuDlgIpmTabCtrl::Page_Processes(UINT nIDD)
{
	CuDlgIpmPageProcesses* pDld = new CuDlgIpmPageProcesses (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_Sessions(UINT nIDD)
{
	CuDlgIpmPageSessions* pDld = new CuDlgIpmPageSessions (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_Transactions(UINT nIDD)    
{
	CuDlgIpmPageTransactions* pDld = new CuDlgIpmPageTransactions (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}



CWnd* CuDlgIpmTabCtrl::Page_RepStaticActivity (UINT nIDD)
{
	CuDlgReplicationStaticPageActivity* pDld = new CuDlgReplicationStaticPageActivity (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepStaticEvent    (UINT nIDD)
{
	CuDlgReplicationStaticPageRaiseEvent* pDld = new CuDlgReplicationStaticPageRaiseEvent (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepStaticDistrib  (UINT nIDD)
{
	CuDlgReplicationStaticPageDistrib* pDld = new CuDlgReplicationStaticPageDistrib (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepStaticCollision(UINT nIDD)
{
	CuDlgReplicationStaticPageCollision* pDld = new CuDlgReplicationStaticPageCollision (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepStaticServer   (UINT nIDD)
{
	CuDlgReplicationStaticPageServer* pDld = new CuDlgReplicationStaticPageServer (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepStaticIntegerity(UINT nIDD)
{
	CuDlgReplicationStaticPageIntegrity* pDld = new CuDlgReplicationStaticPageIntegrity (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepServerStatus   (UINT nIDD)
{
	CuDlgReplicationServerPageStatus* pDld = new CuDlgReplicationServerPageStatus (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepServerStartupSetting(UINT nIDD)
{
	CuDlgReplicationServerPageStartupSetting* pDld = new CuDlgReplicationServerPageStartupSetting (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepServerEvent    (UINT nIDD)
{
	CuDlgReplicationServerPageRaiseEvent* pDld = new CuDlgReplicationServerPageRaiseEvent (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_RepServerAssignment(UINT nIDD)  
{
	CuDlgReplicationServerPageAssignment* pDld = new CuDlgReplicationServerPageAssignment (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

//
// Pages on root static items
//
CWnd* CuDlgIpmTabCtrl::Page_StaticServers(UINT nIDD)    
{
	CuDlgIpmPageStaticServers* pDld = new CuDlgIpmPageStaticServers(&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_StaticDatabases(UINT nIDD)    
{
	CuDlgIpmPageStaticDatabases* pDld = new CuDlgIpmPageStaticDatabases(&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_StaticActiveusers(UINT nIDD)    
{
	CuDlgIpmPageStaticActiveusers* pDld = new CuDlgIpmPageStaticActiveusers(&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::Page_StaticReplication(UINT nIDD)    
{
	CuDlgIpmPageStaticReplications* pDld = new CuDlgIpmPageStaticReplications(&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}


//
// Ice (Pages + Detail):
CWnd* CuDlgIpmTabCtrl::CreateFormViewPage (UINT nIDD)
{
	CRect r;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	
	CuDlgViewFrame* pDlgFrame = new CuDlgViewFrame (nIDD);
	BOOL bCreated = pDlgFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		&m_cTab1
		);
	if (!bCreated)
	{
		AfxThrowResourceException();
		return NULL;
	}
	pDlgFrame->InitialUpdateFrame (NULL, FALSE);
	return (CWnd*)pDlgFrame;
}


CWnd* CuDlgIpmTabCtrl::IcePage_ActiveUser(UINT nIDD)
{
	CuDlgIpmIcePageActiveUser* pDld = new CuDlgIpmIcePageActiveUser (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::IcePage_CachePage(UINT nIDD)
{
	CuDlgIpmIcePageCachePage* pDld = new CuDlgIpmIcePageCachePage (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::IcePage_ConnectedUser(UINT nIDD)
{
	CuDlgIpmIcePageConnectedUser* pDld = new CuDlgIpmIcePageConnectedUser (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}


CWnd* CuDlgIpmTabCtrl::IcePage_Cursor(UINT nIDD)
{
	CuDlgIpmIcePageCursor* pDld = new CuDlgIpmIcePageCursor (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::IcePage_DatabaseConnection(UINT nIDD)
{
	CuDlgIpmIcePageDatabaseConnection* pDld = new CuDlgIpmIcePageDatabaseConnection (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::IcePage_HttpServerConnection(UINT nIDD)
{
	CuDlgIpmIcePageHttpServerConnection* pDld = new CuDlgIpmIcePageHttpServerConnection (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}

CWnd* CuDlgIpmTabCtrl::IcePage_Transaction(UINT nIDD)
{
	CuDlgIpmIcePageTransaction* pDld = new CuDlgIpmIcePageTransaction (&m_cTab1);
	if (!pDld->Create (nIDD, &m_cTab1))
		AfxThrowResourceException();
	return (CWnd*)pDld;
}


void CuDlgIpmTabCtrl::OnDestroy() 
{
	if (m_pCurrentProperty)
	{
		m_pCurrentProperty->SetPageInfo (NULL);
		delete m_pCurrentProperty;
	}
	CDialog::OnDestroy();
}


void CuDlgIpmTabCtrl::LoadPage (CuIpmProperty* pCurrentProperty)
{
	CRect   r;
	TC_ITEM item;
	int     i, nPages;
	UINT*   nTabID;
	UINT*   nDlgID;
	CString strTab;
	CString strTitle;
	UINT    nIDD; 
	CWnd*   pDlg;
	CuPageInformation* pPageInfo = NULL;
	
	if (m_pCurrentProperty)
	{
		m_pCurrentProperty->SetPageInfo (NULL);
		delete m_pCurrentProperty;
	}
	pPageInfo = pCurrentProperty->GetPageInfo();
	m_pCurrentProperty = new CuIpmProperty (pCurrentProperty->GetCurSel(), pPageInfo);
	
	//
	// Set up the Title:
	pPageInfo->GetTitle (strTitle);
	m_staticHeader.SetWindowText (strTitle);

	UINT uFlags = SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER;
	if (!strTitle.IsEmpty())
	{
		switch (pPageInfo->GetImageType())
		{
		case 0:
			m_staticHeader.ResetBitmap (pPageInfo->GetIconImage());
			break;
		case 1:
			m_staticHeader.ResetBitmap (pPageInfo->GetBitmapImage(), pPageInfo->GetImageMask());
			break;
		default:
			m_staticHeader.ResetBitmap (NULL);
		}
	}
	else
		m_staticHeader.ResetBitmap (NULL);
	m_staticHeader.SetWindowPos(NULL, 0, 0, 0, 0, uFlags);

	nPages = pPageInfo->GetNumberOfPage();
	if (nPages < 1)
	{
		m_pCurrentPage = NULL;
		return;
	}
	//
	// Construct the Tab(s)
	nTabID = pPageInfo->GetTabID();
	nDlgID = pPageInfo->GetDlgID();
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT;
	item.cchTextMax = 32;
	m_cTab1.DeleteAllItems();
	for (i=0; i<nPages; i++)
	{
		strTab.LoadString (nTabID [i]);
		item.pszText = (LPTSTR)(LPCTSTR)strTab;
		m_cTab1.InsertItem (i, &item);
	}
	//
	// Display the selected page.
	nIDD = pPageInfo->GetDlgID (pCurrentProperty->GetCurSel()); 
	pDlg = GetPage (nIDD);
	m_cTab1.SetCurSel (pCurrentProperty->GetCurSel());
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	pDlg->MoveWindow (r);
	pDlg->ShowWindow(SW_SHOW);
	m_pCurrentPage = pDlg;
	//
	// Display the content (data) of this page.
	CuIpmPropertyData* pData = pCurrentProperty->GetPropertyData();
	pData->NotifyLoad (m_pCurrentPage);

	//
	// Properties:
	CView*   pView = (CView*)GetParent();
	ASSERT (pView);
	CdIpmDoc* pDoc  = (CdIpmDoc*)pView->GetDocument();
	ASSERT (pDoc);
	if (pDoc)
	{
		UINT nMask = IPMMASK_FONT|IPMMASK_SHOWGRID;
		m_pCurrentPage->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&(pDoc->GetProperty()));
	}
}

CuIpmPropertyData* CuDlgIpmTabCtrl::GetDialogData()
{
	if (m_pCurrentPage)
		return (CuIpmPropertyData*)(LRESULT)m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_GETDATA, 0, 0L);
	return NULL;
}

UINT CuDlgIpmTabCtrl::GetCurrentPageID()
{
	if (!m_pCurrentProperty)
		return 0;
	CuPageInformation* pPageInfo = m_pCurrentProperty->GetPageInfo();
	if (!pPageInfo)
		return 0;
	if (pPageInfo->GetNumberOfPage() == 0)
		return 0;
	UINT* pArrayID = pPageInfo->GetDlgID();
	if (!pArrayID)
		return 0;
	int index = m_pCurrentProperty->GetCurSel();
	if (index >= 0 && index < pPageInfo->GetNumberOfPage())
		return pArrayID [index];
	return 0;
}

void CuDlgIpmTabCtrl::OnSelchangingMfcTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// Notify current page BEFORE hiding it
	if (m_pCurrentProperty)
		if (m_pCurrentPage)
			m_pCurrentPage->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, (WPARAM)LEAVINGPAGE_CHANGEPROPPAGE, 0L);

	OnHideProperty (0, 0);
	*pResult = 0;
}

long CuDlgIpmTabCtrl::OnSettingChange(WPARAM wParam, LPARAM lParam)
{
	UINT nMask = (UINT)wParam;
	if (m_pCurrentPage && ((nMask & IPMMASK_FONT) || (nMask & IPMMASK_SHOWGRID)))
	{
		m_pCurrentPage->SendMessage (WMUSRMSG_CHANGE_SETTING, wParam, lParam);
	}
	return 0;
}

void CuDlgIpmTabCtrl::SelectPage (int nPage)
{
	int nSel = m_cTab1.SetCurSel(nPage);
	if (nSel != -1)
		ChangeTab();
}

