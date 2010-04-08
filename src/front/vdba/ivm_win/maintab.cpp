/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : maintab.cpp , Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Tab Control that contain the pages of Right View                      //
****************************************************************************************/
/* History:
* --------
* uk$so01: (15-Dec-1998 created)
*
*
*/

#include "stdafx.h"
#include "ivm.h"
#include "maintab.h"
#include "mgrfrm.h"
#include "wmusrmsg.h"

#include "stainsta.h"    // INSTALLATION Status 
#include "parinsta.h"    // INSTALLATION Parameter 
#include "levgener.h"    // Generic Logged Events

#include "acprcp.h"      // Archiver and Recovery logged event pages 
#include "stadupco.h"    // Status of Duplicatable component 
#include "stasinco.h"    // Status of Single instance component 
#include "staautco.h"    // Status of component that started by others
#include "stainstg.h"    // Status of Instance (Generic)
#include "msgstat.h"     // Statistic Page

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



CuMainTabCtrl::CuMainTabCtrl()
{
	UINT nArrayIDD [NUMBEROFPAGES] = 
	{
		IDD_LOGEVENT_GENERIC,
		IDD_PARAMETER_INSTALLATION,
		IDD_STATUS_INSTALLATION,
		IDD_STATUS_DUPLICATABLE_COMPONENT,
		IDD_STATUS_SINGLEINSTANCE_COMPONENT,
		IDD_STATUS_INSTANCE,
		IDD_STATUS_AUTOSTART_COMPONENT,
		IDD_LOGEVENT_ACPRCP,
		IDD_MESSAGE_STATISTIC
	};

	qsort ((void*)nArrayIDD, (size_t)NUMBEROFPAGES, (size_t)sizeof(UINT), compare);
	for (int i=0; i<NUMBEROFPAGES; i++)
	{
		m_arrayDlg[i].nIDD = nArrayIDD [i];
		m_arrayDlg[i].pPage= NULL;
	}
	m_pCurrentProperty = NULL;
	m_pCurrentPage = NULL;
}

CuMainTabCtrl::~CuMainTabCtrl()
{
	if (m_pCurrentProperty)
	{
		m_pCurrentProperty->SetPageInfo (NULL);
		delete m_pCurrentProperty;
	}
}

UINT CuMainTabCtrl::GetHelpID()
{
	int nsel = GetCurSel();
	UINT nHelpID = 0;
	if (m_pCurrentProperty)
	{
		CaPageInformation* pCurrentPageInfo = m_pCurrentProperty->GetPageInfo();
		nHelpID = pCurrentPageInfo->GetDlgID (nsel);
	}
	return nHelpID;
}


CWnd* CuMainTabCtrl::ConstructPage (UINT nIDD)
{
	CWnd* pPage = NULL;
	switch (nIDD)
	{
	//
	// Modeless Dialog Pages
	case IDD_LOGEVENT_GENERIC:
		pPage = (CWnd*)new CuDlgLogEventGeneric (this);
		if (!((CuDlgLogEventGeneric*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_PARAMETER_INSTALLATION:
		pPage = (CWnd*)new CuDlgParameterInstallation (this);
		if (!((CuDlgParameterInstallation*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_STATUS_INSTALLATION:
		pPage = (CWnd*)new CuDlgStatusInstallation (this);
		if (!((CuDlgStatusInstallation*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_LOGEVENT_ACPRCP:
		pPage = (CWnd*)new CuDlgArchiverRecoverLog (this);
		if (!((CuDlgArchiverRecoverLog*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_MESSAGE_STATISTIC:
		pPage = (CWnd*)new CuDlgMessageStatistic (this);
		if (!((CuDlgMessageStatistic*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_STATUS_DUPLICATABLE_COMPONENT:
		pPage = (CWnd*)new CuDlgStatusDuplicatableComponent (this);
		if (!((CuDlgStatusDuplicatableComponent*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_STATUS_SINGLEINSTANCE_COMPONENT:
		pPage = (CWnd*)new CuDlgStatusSingleInstanceComponent (this);
		if (!((CuDlgStatusSingleInstanceComponent*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_STATUS_INSTANCE:
		pPage = (CWnd*)new CuDlgStatusInstance (this);
		if (!((CuDlgStatusInstance*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;
	case IDD_STATUS_AUTOSTART_COMPONENT:
		pPage = (CWnd*)new CuDlgStatusAutoStartComponent (this);
		if (!((CuDlgStatusAutoStartComponent*)pPage)->Create (nIDD, this))
			AfxThrowResourceException();
		return pPage;

	//
	// Form View Pages.
	default:
		return FormViewPage (nIDD);
	}
}


CWnd* CuMainTabCtrl::FormViewPage(UINT nIDD)
{
	CRect r;
	GetClientRect (r);
	AdjustRect (FALSE, r);
	
	//
	// Actually, there is no form view
	ASSERT (FALSE);
	return NULL;
}

CWnd* CuMainTabCtrl::IsPageCreated (UINT nIDD)
{
	int index = Find (nIDD, m_arrayDlg, 0, NUMBEROFPAGES -1);
	ASSERT (index != -1);
	if (index == -1)
		return NULL;
	return m_arrayDlg[index].pPage;
}

CWnd* CuMainTabCtrl::GetPage (UINT nIDD)
{
	int index = Find (nIDD, m_arrayDlg, 0, NUMBEROFPAGES -1);
	//
	// Call GetPage, Did you forget to define the page in this file ???..
	ASSERT (index != -1);
	if (index == -1)
		return NULL;
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
	// Call GetPage, you must define some dialog box.(Form view)..
	ASSERT (FALSE);
	return NULL;
}

BEGIN_MESSAGE_MAP(CuMainTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CuMainTabCtrl)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_REMOVE_EVENT,  OnRemoveEvent)
	ON_MESSAGE (WMUSRMSG_IVM_COMPONENT_CHANGED,  OnComponentChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuMainTabCtrl message handlers

void CuMainTabCtrl::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	int nSel;
	CRect r;
	CWnd* pNewPage;
	CaPageInformation* pPageInfo;
	CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;

	if (!pFrame->IsAllViewsCreated())
		return;
	if (!m_pCurrentProperty)
		return;
	CWaitCursor waitCursor;
	GetClientRect (r);
	AdjustRect (FALSE, r);
	nSel = GetCurSel();
	m_pCurrentProperty->SetCurSel(nSel);
	pPageInfo = m_pCurrentProperty->GetPageInfo();
	try
	{
		pNewPage  = GetPage (pPageInfo->GetDlgID (nSel));
		if (!pNewPage)
			return;
	}
	catch (CMemoryException* e)
	{
		IVM_OutOfMemoryMessage();
		e->Delete();
		return;
	}
	catch (CResourceException* e)
	{
		AfxMessageBox (_T("System error (CuMainTabCtrl::OnSelchange): Fail to load resource"));
		e->Delete();
		return;
	}
	if (m_pCurrentPage)
	{
		//
		// Try to save (and validate) the data that has been changed
		// in the old page before displaying a new page.
		// m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
		m_pCurrentPage->ShowWindow (SW_HIDE);
	}
	m_pCurrentPage = pNewPage;
	m_pCurrentPage->MoveWindow (r);
	m_pCurrentPage->ShowWindow(SW_SHOW);
	m_pCurrentPage->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, (LPARAM)pPageInfo);
}

void CuMainTabCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CTabCtrl::OnSize(nType, cx, cy);
	CRect r;
	if (!IsWindow (m_hWnd)) 
		return;

	GetClientRect (r);
	AdjustRect (FALSE, r);
	if (m_pCurrentPage)
		m_pCurrentPage->MoveWindow (r);
}

int CuMainTabCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}


void CuMainTabCtrl::DisplayPage (CaPageInformation* pPageInfo)
{
	CRect   r;
	TC_ITEM item;
	int     i, nPages;
	UINT*   nTabID;
	UINT*   nDlgID;
	CString strTab;
	UINT    nIDD; 
	CWnd* pDlg;
	
	if (!pPageInfo)
	{
		if (m_pCurrentPage)
		{
			m_pCurrentPage->ShowWindow (SW_HIDE);
		}
		DeleteAllItems();

		if (m_pCurrentProperty)
		{
			m_pCurrentProperty->SetPageInfo (NULL);
			delete m_pCurrentProperty;
			m_pCurrentProperty = NULL;
			m_pCurrentPage     = NULL;
		}
		return;
	}
	if (!m_pCurrentProperty)
	{
		try
		{
			m_pCurrentProperty = new CaIvmProperty (0, pPageInfo);
		}
		catch (CMemoryException* e)
		{
			IVM_OutOfMemoryMessage();
			e->Delete();
			m_pCurrentPage = NULL;
			m_pCurrentProperty = NULL;
			return;
		}
		
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
		DeleteAllItems();
		for (i=0; i<nPages; i++)
		{
			strTab.LoadString (nTabID [i]);
			item.pszText = (LPTSTR)(LPCTSTR)strTab;
			InsertItem (i, &item);
		}
		//
		// Display the default (the first in general) page,
		int iPage = pPageInfo->GetDefaultPage ();
		nIDD = pPageInfo->GetDlgID (iPage);
		pDlg= GetPage (nIDD);
		if (!pDlg)
			return;

		m_pCurrentProperty->SetCurSel(iPage);
		SetCurSel (iPage);
		GetClientRect (r);
		AdjustRect (FALSE, r);
		pDlg->MoveWindow (r);
		pDlg->ShowWindow(SW_SHOW);
		m_pCurrentPage = pDlg;
		m_pCurrentPage->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)0, (LPARAM)pPageInfo);
	}
	else
	{
		CaPageInformation* pCurrentPageInfo = m_pCurrentProperty->GetPageInfo();
		if (pCurrentPageInfo->GetClassName() == pPageInfo->GetClassName())
		{
			m_pCurrentProperty->SetPageInfo (pPageInfo);
			//
			// In general, when the selection changes on the left-pane, 
			// and if the new selected item if the same class as the old one
			// then we keep the current selected page (Tab) at the right-pane.

			//
			// wParam, and lParam will contain the information needed
			// by the dialog to refresh itself.
			if (m_pCurrentPage) 
				m_pCurrentPage->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)0, (LPARAM)pPageInfo);
		}
		else
		{
			if (m_pCurrentPage) 
			{
				//
				// Try to save (and validate) the data that has been changed
				// in the old page before displaying a new page.
				// m_pCurrentPage->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
				m_pCurrentPage->ShowWindow (SW_HIDE);
			}
			DeleteAllItems();
			nPages = pPageInfo->GetNumberOfPage();
			m_pCurrentProperty->SetPageInfo (pPageInfo);
			
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
			DeleteAllItems();
			for (i=0; i<nPages; i++)
			{
				strTab.LoadString (nTabID [i]);
				item.pszText = (LPTSTR)(LPCTSTR)strTab;
				InsertItem (i, &item);
			}
			//
			// Display the default (the first) page.
			int iPage = pPageInfo->GetDefaultPage ();
			nIDD  = pPageInfo->GetDlgID (iPage);
			pDlg= GetPage (nIDD);
			if (!pDlg)
				return;

			m_pCurrentProperty->SetCurSel (iPage);
			SetCurSel (iPage);
			GetClientRect (r);
			AdjustRect (FALSE, r);
			pDlg->MoveWindow (r);
			pDlg->ShowWindow(SW_SHOW);
			m_pCurrentPage = pDlg;
			m_pCurrentPage->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)0, (LPARAM)pPageInfo);
		}
	}
}



LONG CuMainTabCtrl::OnRemoveEvent(WPARAM wParam, LPARAM lParam)
{
	int index = Find (IDD_LOGEVENT_GENERIC, m_arrayDlg, 0, NUMBEROFPAGES -1);
	ASSERT (index != -1);
	if (index == -1)
		return 0;
	if (m_arrayDlg[index].pPage)
		(m_arrayDlg[index].pPage)->SendMessage (WMUSRMSG_IVM_REMOVE_EVENT, wParam, lParam);
	return 0;
}

LONG CuMainTabCtrl::OnComponentChanged (WPARAM wParam, LPARAM lParam)
{
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
		m_pCurrentPage->SendMessage (WMUSRMSG_IVM_COMPONENT_CHANGED, wParam, lParam);
	return 0;
}
