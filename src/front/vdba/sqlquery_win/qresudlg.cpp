/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qresudlg.cpp, Implementation file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a CTab Control to manage displaying the different pages
**               of the result (history of query result)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created.
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 21-Oct-2002 (uk$so01)
**    BUG/SIR #106156 Manually integrate the change #453850 into ingres30
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 07-Nov-2003 (uk$so01)
**    SIR #106648. Additional fix: eliminate the dirty garbage on the screen
**    after changing the "Max Tab" (decrease). Use the >= instead of =
**    to check if the number of Tabs have reached the limit.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "qframe.h"
#include "qresudlg.h"
#include "qpageqep.h"    // QEP Page.
#include "qpageres.h"    // Result Page (Select statement only)
#include "qpagegen.h"    // The generic page (Non-Select statement)
#include "qpagexml.h"    // The XML page
#include "tlsfunct.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgSqlQueryResult::CuDlgSqlQueryResult(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryResult::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSqlQueryResult)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pRawPage     = NULL;
	m_pCurrentPage = NULL;
	m_bMaxPageReached = FALSE;
	m_bInitialCreated = TRUE;
}


void CuDlgSqlQueryResult::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryResult)
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgSqlQueryResult, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryResult)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_STATEMENT_CHANGE,  OnSQLStatementChange)
	ON_MESSAGE (WM_SQL_STATEMENT_EXECUTE, OnExecuteSQLStatement)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgSqlQueryResult message handlers

void CuDlgSqlQueryResult::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgSqlQueryResult::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent ();                 // CvSqlQueryLowerView.
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();     // CdSqlQueryRichEditDoc.
	ASSERT (pDoc);
	m_ImageList.Create(IDB_SQLACT_TABS, 16, 1, RGB(255, 0, 255));
	m_cTab1.SetImageList (&m_ImageList);

	if (pDoc->IsLoadedDoc())
	{
		LoadDocument();
	}
	else
	{
		//
		// When the dialog is create, it always has
		// one empty Tab called 'Result' with number 1.
		DisplayPage (NULL);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryResult::UpdatePage(int nPage) 
{
	ASSERT (nPage == m_cTab1.GetCurSel());
	CRect r;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	//
	// Hide the old page
	if (m_pCurrentPage)
		m_pCurrentPage->ShowWindow(SW_HIDE);
	m_cTab1.GetItem (nPage, &item);
	CWnd* pPage = (CWnd*)item.lParam;
	m_pCurrentPage = pPage? pPage: NULL;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (!m_pCurrentPage)
		return;
	m_pCurrentPage->MoveWindow(r);
	m_pCurrentPage->ShowWindow(SW_SHOW);
}


void CuDlgSqlQueryResult::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cTab1.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cTab1.MoveWindow (r);

	if (!m_pCurrentPage)
		return;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	m_pCurrentPage->MoveWindow(r);
	m_pCurrentPage->ShowWindow(SW_SHOW);
}

void CuDlgSqlQueryResult::ChangeTab(BOOL bLoaded)
{
	CRect r;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;

	int nSel = m_cTab1.GetCurSel();
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;

	//
	// Hide the old page
	if (m_pCurrentPage)
	{
		m_pCurrentPage->ShowWindow(SW_HIDE);
	}
	m_cTab1.GetItem (nSel, &item);
	CDialog* pPage = (CDialog*)item.lParam;
	m_pCurrentPage = pPage? pPage: NULL;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	//
	// Show new page
	if (m_pCurrentPage)
	{
		m_pCurrentPage->MoveWindow(r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
		if (!bLoaded)
			m_pCurrentPage->SendMessage (WM_SQL_QUERY_PAGE_HIGHLIGHT, 0, 0);
	}
}

void CuDlgSqlQueryResult::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ChangeTab();
	*pResult = 0;
}


void CuDlgSqlQueryResult::DisplayPageHeader (int nStart)
{
	CWnd* pWnd = NULL;  
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	int   nTab = m_cTab1.GetItemCount ();
	for (int i=nStart; i<nTab; i++)
	{
		m_cTab1.GetItem (i, &item);
		pWnd = (CWnd*)item.lParam;
		if (pWnd) 
			pWnd->SendMessage (WM_SQL_QUERY_PAGE_SHOWHEADER, (WPARAM)0, (LPARAM)0);
	}
}

void CuDlgSqlQueryResult::DisplayPage (CWnd* pPage, BOOL bLoaded, int nImage)
{
	CRect r;
	int   nTab = 0;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM|TCIF_IMAGE;
	item.iImage     = -1;
	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent ();
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;
	CaSqlQueryProperty& property = pDoc->GetProperty();
	CString strTabLabel;
	if (bLoaded)
	{
		if (pDoc->m_nResultCounter == 1 && pDoc->IsUsedTracePage())
			strTabLabel = _T("Trace");
		else
		{
			if (pDoc->IsUsedTracePage())
				// Do not count the raw page
				strTabLabel.Format (_T("%d*"), pDoc->m_nResultCounter -1);
			else
				strTabLabel.Format (_T("%d*"), pDoc->m_nResultCounter);
		}
	}
	else
		strTabLabel.Format (_T("%d"), pDoc->m_nResultCounter);
	//
	// Destroy the Tab (before the last one) if need:
	DestroyTab(pDoc, TRUE);

	//
	// Hide old page if any
	if (m_pCurrentPage && pPage && IsWindow(m_pCurrentPage->m_hWnd))
		m_pCurrentPage->ShowWindow(SW_HIDE);

	if (pPage == NULL)
	{
		//
		// Add new Tab (Empty Tab) and Do not select it
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_TEXT|TCIF_IMAGE;
		item.cchTextMax = 32;
		if (bLoaded)
		{
			if (pDoc->m_nResultCounter == 1 && pDoc->IsUsedTracePage())
				item.iImage = BM_TRACE;
			else
				item.iImage = (nImage == -1)? BM_NEWTAB: nImage;
		}
		else
			item.iImage = BM_NEWTAB;
		item.pszText    = (LPTSTR)(LPCTSTR)strTabLabel;
		item.lParam     = 0L;
		m_cTab1.InsertItem (0, &item);
		pDoc->m_nResultCounter++;
		return;
	}

	// pPage != NULL.
	// Test if the first Tab (index 0) has already a page
	// then add a new Tab with new page.
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	item.iImage     = -1;
	m_cTab1.GetItem (0, &item);
	if (item.lParam != 0)
	{
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_TEXT|TCIF_IMAGE;
		item.cchTextMax = 32;
		item.iImage     = (nImage == -1)? (int)pPage->SendMessage (WM_SQL_TAB_BITMAP, 0, 0): nImage;
		item.pszText    = (LPTSTR)(LPCTSTR)strTabLabel;
		m_cTab1.InsertItem (0, &item);
		pDoc->m_nResultCounter++;
	}

	BOOL bCurrentIsTrace = FALSE;
	if (m_pCurrentPage && m_pCurrentPage->SendMessage (WM_SQL_TAB_BITMAP, 0, 0) == BM_TRACE)
		bCurrentIsTrace = TRUE;
	if (bCurrentIsTrace)
	{
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_PARAM;
		item.iImage     = -1;
		item.lParam = (LPARAM)pPage;
		m_cTab1.SetItem (0, &item);
		UpdatePage (m_cTab1.GetCurSel());
	}
	else
	{
		m_pCurrentPage = pPage? pPage: NULL;
		m_cTab1.GetClientRect (r);
		m_cTab1.AdjustRect (FALSE, r);
		if (m_pCurrentPage)
		{
			m_pCurrentPage->MoveWindow(r);
			m_pCurrentPage->ShowWindow(SW_SHOW);
		}
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_PARAM|TCIF_IMAGE;
		item.iImage     = bLoaded? nImage: -1;
		item.lParam = (LPARAM)m_pCurrentPage;
		m_cTab1.SetItem (0, &item);
		
		m_cTab1.SetCurSel (0);
	}
}

void CuDlgSqlQueryResult::DisplayRawPage (BOOL bDisplay)
{
	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent (); 
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;
	CaSqlQueryProperty& property = pDoc->GetProperty();
	int nTabCount = m_cTab1.GetItemCount();
	CRect r;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;
	item.cchTextMax = 32;
	item.iImage     = BM_TRACE;
	item.pszText    = _T("Trace");
	item.lParam     = 0L;

	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);

	if (!m_pRawPage)
	{
		m_pRawPage = new CuDlgSqlQueryPageRaw(&m_cTab1);
		m_pRawPage->Create (IDD_SQLQUERY_PAGERAW, &m_cTab1);
		UINT nMask = SQLMASK_FONT;
		m_pRawPage->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&property);
	}
	item.lParam     = (LPARAM)m_pRawPage;
	if (bDisplay)
	{
		if (!pDoc->IsUsedTracePage())
		{
			//
			// Destroy the Tab (before the last one) if need:
			DestroyTab(pDoc, TRUE);
			m_cTab1.InsertItem (nTabCount, &item);
			if (property.IsTraceToTop())
				m_cTab1.SetCurSel (nTabCount);
			if (m_pCurrentPage)
				m_pCurrentPage->ShowWindow(SW_HIDE);
			m_pCurrentPage = m_pRawPage;
			pDoc->SetUsedTracePage(TRUE);
		}
	}
	else
	if (m_pRawPage && IsWindow (m_pRawPage->m_hWnd) && nTabCount > 0)
	{
		m_pRawPage->ShowWindow (SW_HIDE);
		m_cTab1.DeleteItem (nTabCount-1);
		m_pCurrentPage = NULL;
		m_cTab1.SetCurSel (pDoc->m_nTabLastSel);
		int nCurSel = m_cTab1.GetCurSel();
		if (nCurSel == -1)
		{
			m_cTab1.SetCurSel(0);
			nCurSel = 0;
		}
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_PARAM;
		item.iImage     = -1;
		m_cTab1.GetItem (nCurSel, &item);
		if (item.lParam != 0)
			m_pCurrentPage = (CWnd*)item.lParam;
		pDoc->SetUsedTracePage(FALSE);
	}
	if (m_pCurrentPage)
	{
		m_pCurrentPage->MoveWindow(r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
		ChangeTab(FALSE);
	}
}


LONG CuDlgSqlQueryResult::OnSQLStatementChange (WPARAM wParam, LPARAM lParam)
{
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	m_cTab1.GetItem (0, &item);
	if (item.lParam != 0)
	{
		DisplayPage (NULL);
	}
	DisplayPageHeader (1);
	return 0L;
}

//
// The expected wParam is 
// 0: normal
// 1: qep.
// 2: generate xml.
LONG CuDlgSqlQueryResult::OnExecuteSQLStatement (WPARAM wParam, LPARAM lParam)
{
	int nMode = (int)wParam;
	CaSqlQueryData* pQueryData = (CaSqlQueryData*)lParam;
	ASSERT (pQueryData);
	if (!pQueryData)
		return 0L;
	CaSqlQueryStatement* pQueryInfo = pQueryData->m_pQueryStatement;
	CuDlgSqlQueryPageQep* pPage1;
	CuDlgSqlQueryPageResult* pPage2;
	CuDlgSqlQueryPageGeneric* pPage3 ;
	CuDlgSqlQueryPageXML* pPage4 ;
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return 0;
	try
	{
		CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
		CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pFrame->GetSqlDocument();
		ASSERT(pDoc);
		CaSqlQueryProperty& property = pDoc->GetProperty();

		switch (nMode)
		{
		case 0:
			if (pQueryInfo->m_bSelect)
			{
				//
				// Execute a select statement 'pQueryInfo->m_strStatement'.
				TRACE1 ("CuDlgSqlQueryResult::OnExecuteSQLStatement, Execute a Select Statement: %s...\n", (LPCTSTR)pQueryInfo->m_strStatement);

				pPage2 = new CuDlgSqlQueryPageResult (&m_cTab1);
				pPage2->m_nStart      = pQueryInfo->m_nStart;
				pPage2->m_nEnd        = pQueryInfo->m_nEnd;
				pPage2->m_strEdit1    = pQueryInfo->m_strStatement;
				RCTOOL_EliminateTabs  (pPage2->m_strEdit1);
				pPage2->m_strDatabase = pQueryInfo->m_strDatabase;
				pPage2->Create (IDD_SQLQUERY_PAGERESULT, &m_cTab1);
				UINT nMask = SQLMASK_FONT|SQLMASK_SHOWGRID;
				pPage2->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&property);
				return (LONG)pPage2;
			}
			else
			{
				//
				// Execute a non-select statement 'strStatement'.
				TRACE1 ("CuDlgSqlQueryResult::OnExecuteSQLStatement, Execute a Non-Select Statement: %s...\n", (LPCTSTR)pQueryInfo->m_strStatement);
				pPage3 = new CuDlgSqlQueryPageGeneric (&m_cTab1);
				pPage3->m_nStart      = pQueryInfo->m_nStart;
				pPage3->m_nEnd        = pQueryInfo->m_nEnd;
				pPage3->m_strEdit1    = pQueryInfo->m_strStatement;
				RCTOOL_EliminateTabs (pPage3->m_strEdit1);
				if (pQueryData->m_nAffectedRows != -1)
				{
					CString strRowAffected;
					pPage3->m_strEdit1 +=_T("\r\n");
					strRowAffected.Format (IDS_F_ROWS, pQueryData->m_nAffectedRows);//_T("<%d row(s)>")
					pPage3->m_strEdit1 +=strRowAffected;
				}

				pPage3->m_strDatabase = pQueryInfo->m_strDatabase;
				pPage3->Create (IDD_SQLQUERY_PAGEGENERIC, &m_cTab1);
				return (LONG)pPage3;
			}
			break;
		case 1:
			//
			// For the QEP page, it receives the message WM_SQL_QUERY_UPADATE_DATA
			// and then Call the Frame member InitialUpdateFrame that cause the view
			// to Initialize the Binary Tree Data on its OnInitialUpdate
			//
			//
			// Execute a statement 'strStatement' for QEP
			// The statement must be a select statement:
			ASSERT (pQueryData && pQueryData->m_pQueryStatement && pQueryData->m_pQueryStatement->m_bSelect);
			TRACE1 ("CuDlgSqlQueryResult::OnExecuteSQLStatement, Execute a Statement for QEP: %s...\n", (LPCTSTR)pQueryInfo->m_strStatement);

			pPage1 = new CuDlgSqlQueryPageQep (&m_cTab1);
			pPage1->SetQepDoc (((CaSqlQueryQepData*)pQueryData)->m_pQepDoc);
			pPage1->m_nStart      = pQueryInfo->m_nStart;
			pPage1->m_nEnd        = pQueryInfo->m_nEnd;
			pPage1->m_strEdit1    = pQueryInfo->m_strStatement;
			RCTOOL_EliminateTabs (pPage1->m_strEdit1);
			pPage1->m_strDatabase = pQueryInfo->m_strDatabase;
			pPage1->Create (IDD_SQLQUERY_PAGEQEP, &m_cTab1);
			pPage1->SendMessage (WM_SQL_QUERY_UPADATE_DATA, (WPARAM)0, (LPARAM)0);
			return (LONG)pPage1;
			break;
		case 2: // XML
			if (pQueryInfo->m_bSelect)
			{
				//
				// Execute a select statement 'pQueryInfo->m_strStatement'.
				TRACE1 ("CuDlgSqlQueryResult::OnExecuteSQLStatement, Gen XML::Statement: %s...\n", (LPCTSTR)pQueryInfo->m_strStatement);

				pPage4 = new CuDlgSqlQueryPageXML (&m_cTab1);
				pPage4->m_nStart      = pQueryInfo->m_nStart;
				pPage4->m_nEnd        = pQueryInfo->m_nEnd;
				pPage4->m_strStatement= pQueryInfo->m_strStatement;
				RCTOOL_EliminateTabs (pPage4->m_strStatement);
				pPage4->m_strDatabase = pQueryInfo->m_strDatabase;
				pPage4->Create (IDD_SQLQUERY_PAGEXML, &m_cTab1);
				return (LONG)pPage4;
			}
			break;
		default:
			break;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeSqlQueryException e)
	{
		CString strText = e.GetReason();
		if (!strText.IsEmpty())
			AfxMessageBox (strText);
		else
		{
			AfxMessageBox (_T("CuDlgSqlQueryResult::OnExecuteSQLStatement: Cannot execute the statement")); 
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("CuDlgSqlQueryResult::OnExecuteSQLStatement: Cannot execute the statement")); 
	}
	return 0L;
}

void CuDlgSqlQueryResult::GetData ()
{
	TCITEM item;
	CaQueryPageData* pData = NULL;
	int i, nCount = m_cTab1.GetItemCount();

	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;

	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent ();                 // CvSqlQueryLowerView.
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();     // CdSqlQueryRichEditDoc.
	ASSERT (pDoc);
	if (!pDoc)
		return;

	pDoc->m_nTabCurSel = m_cTab1.GetCurSel();
	while (!pDoc->m_listPageResult.IsEmpty())
		delete pDoc->m_listPageResult.RemoveHead();
	for (i=0; i<nCount; i++)
	{
		m_cTab1.GetItem (i, &item);
		if (item.lParam != 0)
		{
			//
			// The Tab has a page data.
			pData = (CaQueryPageData*)(LRESULT)((CWnd*)item.lParam)->SendMessage (WM_SQL_GETPAGE_DATA, 0, 0);
			if (pData)
				pDoc->m_listPageResult.AddTail (pData);
		}
		else
		{
			// 
			// The Tab does not has the page data, (Result, with empty page)
			if (nCount > 1)
				TRACE0 ("CuDlgSqlQueryResult::GetData: The Tab does not has the page data (Result, with empty page)\n");
		}
	}
}

CaQueryPageData* CuDlgSqlQueryResult::GetCurrentPageData()
{
	TCITEM item;
	CaQueryPageData* pData = NULL;
	int nCount = m_cTab1.GetItemCount();

	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;

	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent ();                 // CvSqlQueryLowerView.
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();     // CdSqlQueryRichEditDoc.
	ASSERT (pDoc);
	if (!pDoc)
		return NULL;

	int nTabCurSel = m_cTab1.GetCurSel();
	ASSERT (nTabCurSel != -1);
	m_cTab1.GetItem (nTabCurSel, &item);
	if (item.lParam != 0) 
	{
		// The Tab has a page data.
		pData = (CaQueryPageData*)(LRESULT)((CWnd*)item.lParam)->SendMessage (WM_SQL_GETPAGE_DATA, 0, 0);
	}
	else {
		// The Tab does not has the page data, (Result, with empty page)
		if (nCount > 1)
			TRACE0 ("CuDlgSqlQueryResult::GetCurrentPageData: UNEXPECTED: No page data\n");
	}
	return pData;
}

void CuDlgSqlQueryResult::CloseCursor()
{
	CRect r;
	TCITEM item;
	TCITEM ite;
	memset (&ite,  0, sizeof (item));
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	int  i, nTab = m_cTab1.GetItemCount ();
	for (i=0; i<nTab; i++)
	{
		m_cTab1.GetItem (i, &item);
		if ((CWnd*)item.lParam)
			((CWnd*)item.lParam)->SendMessage (WM_SQL_CLOSE_CURSOR, 0, 0);
	}
	UpdateTabBitmaps();
}

void CuDlgSqlQueryResult::UpdateTabBitmaps()
{
	CRect r;
	TCITEM item;
	TCITEM ite;
	memset (&ite,  0, sizeof (item));
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	int  i, nTab = m_cTab1.GetItemCount ();
	for (i=0; i<nTab; i++)
	{
		m_cTab1.GetItem (i, &item);
		if ((CWnd*)item.lParam)
		{
			ite.mask   = TCIF_IMAGE;
			ite.iImage = (int)((CWnd*)item.lParam)->SendMessage (WM_SQL_TAB_BITMAP, 0, 0);
			m_cTab1.SetItem (i, &ite);
		}
	}
}

void CuDlgSqlQueryResult::DisplayTraceLine (LPCTSTR strStatement, LPCTSTR lpszTrace, LPCTSTR lpszCursorName)
{
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	if (!IsWindow (m_cTab1.m_hWnd))
		return;
	int  i, nTab = m_cTab1.GetItemCount ();
	for (i=0; i<nTab; i++)
	{
		m_cTab1.GetItem (i, &item);
		if ((CWnd*)item.lParam)
		{
			if ((int)((CWnd*)item.lParam)->SendMessage (WM_SQL_TAB_BITMAP, 0, 0) == BM_TRACE)
			{
				CuDlgSqlQueryPageRaw* pPageRaw = (CuDlgSqlQueryPageRaw*)item.lParam;
				pPageRaw->DisplayTraceLine (strStatement, lpszTrace, lpszCursorName);
			}
		}
	}
}


CuDlgSqlQueryPageRaw* CuDlgSqlQueryResult::GetRawPage()
{
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	int  i, nTab = m_cTab1.GetItemCount ();
	for (i=0; i<nTab; i++)
	{
		m_cTab1.GetItem (i, &item);
		if ((CWnd*)item.lParam)
		{
			if ((int)((CWnd*)item.lParam)->SendMessage (WM_SQL_TAB_BITMAP, 0, 0) == BM_TRACE)
			{
				CuDlgSqlQueryPageRaw* pPageRaw = (CuDlgSqlQueryPageRaw*)item.lParam;
				return pPageRaw;
			}
		}
	}
	return NULL;
}

int CuDlgSqlQueryResult::GetOpenedCursorCount()
{
	int nCursorOpened = 0;
	int iBitmap = -1;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.iImage     = -1;
	int  i, nTab = m_cTab1.GetItemCount ();
	for (i=0; i<nTab; i++)
	{
		m_cTab1.GetItem (i, &item);
		if ((CWnd*)item.lParam)
		{
			iBitmap = (int)((CWnd*)item.lParam)->SendMessage (WM_SQL_TAB_BITMAP, 0, 0);
			if (iBitmap == BM_SELECT_OPEN)
				nCursorOpened++;
		}
	}
	return nCursorOpened;
}

void CuDlgSqlQueryResult::Cleanup()
{
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask  = TCIF_PARAM;
	item.iImage= -1;
	int nTab = m_cTab1.GetItemCount();
	for (int i=0; i<nTab; i++)
	{
		m_cTab1.GetItem (0, &item);
		CDialog* pPage = (CDialog*)item.lParam;
		if (pPage)
		{
			pPage->ShowWindow(SW_HIDE);
			pPage->DestroyWindow();
		}
		m_cTab1.DeleteItem (0);
	}
	m_pCurrentPage = NULL;
	m_pRawPage     = NULL;
	m_pCurrentPage = NULL;
	m_bMaxPageReached = FALSE;

	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent ();                 // CvSqlQueryLowerView.
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();     // CdSqlQueryRichEditDoc.
	ASSERT (pDoc);

	while (pDoc && !pDoc->m_listPageResult.IsEmpty())
		delete pDoc->m_listPageResult.RemoveHead();
}


void CuDlgSqlQueryResult::LoadDocument()
{
	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent ();                 // CvSqlQueryLowerView.
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();     // CdSqlQueryRichEditDoc.
	ASSERT (pDoc);
	if (pDoc->IsLoadedDoc())
	{
		CWnd* pPage = NULL;
		CaQueryPageData* pData = NULL;
		int nSaveTabCount = pDoc->m_nResultCounter;
		pDoc->m_nResultCounter = pDoc->m_nResultCounter - pDoc->m_listPageResult.GetCount();
		if (pDoc->m_nResultCounter < 1)
			pDoc->m_nResultCounter = 1;
		//
		// When the dialog is create, it always has
		// one empty Tab called 'Result' with number 1.
		DisplayPage (NULL, TRUE);
		while (!pDoc->m_listPageResult.IsEmpty())
		{
			pData = pDoc->m_listPageResult.RemoveTail();
			if (pData)
			{
				pPage = pData->LoadPage(&m_cTab1); // Construct page from the serialization
				UINT nMask = SQLMASK_FONT|SQLMASK_SHOWGRID;
				pPage->SendMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&pDoc->GetProperty());
				if (pPage && pData->m_nTabImage == BM_SELECT_OPEN)
				{
					pData->m_nTabImage = BM_SELECT_BROKEN;
					((CuDlgSqlQueryPageResult*)pPage)->m_bBrokenCursor = TRUE;
				}

				DisplayPage (pPage, TRUE, pData->m_nTabImage); // Display the page in the Tab Control
				delete pData;
			}
		}
		if (nSaveTabCount == 2 && pDoc->IsUsedTracePage() && m_cTab1.GetItemCount() == 1)
			DisplayPage (NULL, TRUE);
		pDoc->m_nResultCounter = nSaveTabCount;
		m_cTab1.SetCurSel (pDoc->m_nTabCurSel);
		ChangeTab(TRUE);
	}
}


void CuDlgSqlQueryResult::SettingChange(UINT nMask, CaSqlQueryProperty* pProperty)
{
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask  = TCIF_PARAM;
	item.iImage= -1;

	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	ASSERT(pFrame);
	if (!pFrame)
		return;

	if ((nMask & SQLMASK_AUTOCOMMIT) || (nMask & SQLMASK_READLOCK) || (nMask & SQLMASK_TIMEOUT))
	{
		pFrame->SetSessionOption(nMask, pProperty);
	}

	CdSqlQueryRichEditDoc* pDoc =  pFrame->GetSqlDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;
	if (nMask & SQLMASK_MAXTAB)
	{
		m_bMaxPageReached = FALSE;
		DestroyTab(pDoc, FALSE);
	}

	int i, nTab = m_cTab1.GetItemCount();
	if ((nMask & SQLMASK_SHOWNONSELECTTAB) && !pProperty->IsShowNonSelect())
	{
		//
		// Remove the non-select pages:
		int nNewActiveTab = -1;
		i = 0;
		while (i < m_cTab1.GetItemCount())
		{
			m_cTab1.GetItem (i, &item);
			CDialog* pPage = (CDialog*)item.lParam;
			if (pPage && IsWindow (pPage->m_hWnd))
			{
				long nBmp = (long)pPage->SendMessage(WM_SQL_TAB_BITMAP);
				if (nBmp == BM_NON_SELECT)
				{
					pPage->ShowWindow(SW_HIDE);
					m_cTab1.DeleteItem (i);
					if (m_pCurrentPage == pPage)
					{
						m_cTab1.GetItem (i, &item);
						m_pCurrentPage = (CDialog*)item.lParam;
						nNewActiveTab = i;
					}
					pPage->DestroyWindow();

					continue;
				}
				else
				{
					i++;
				}
			}
			else
			{
				i++;
			}
		}
		if (nNewActiveTab != -1)
		{
			m_cTab1.SetCurSel(nNewActiveTab);
			ChangeTab();
		}
	}

	if ((nMask & SQLMASK_LOAD) && m_bInitialCreated) // Trace Tab Initially Visible
	{
		if (pProperty->IsTraceActivated())
			DisplayRawPage (TRUE);
		else
			DisplayRawPage (FALSE);
		m_bInitialCreated = FALSE;
	}

	if (nMask & SQLMASK_TRACETAB)
	{
		if (pProperty->IsTraceToTop())
			SelectRawPage();
		else
		{
			m_cTab1.SetCurSel(0);
			UpdatePage(0);
		}
	}

	nTab = m_cTab1.GetItemCount();
	for (i=0; i<nTab; i++)
	{
		m_cTab1.GetItem (i, &item);
		CDialog* pPage = (CDialog*)item.lParam;
		if (pPage && IsWindow (pPage->m_hWnd))
		{
			pPage->SendMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)pProperty);
		}
	}
}

void CuDlgSqlQueryResult::DestroyTab(CdSqlQueryRichEditDoc* pDoc, BOOL bAdded)
{
	int nPosDelete = 0;
	CDialog* pPage = NULL;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM|TCIF_IMAGE;
	item.iImage     = -1;
	CaSqlQueryProperty& property = pDoc->GetProperty();
	//
	// Destroy the Tab (before the last one) if need:
	if (bAdded)
	{
		int nTab = m_cTab1.GetItemCount();
		if (nTab >= property.GetMaxTab())
		{
			if (!m_bMaxPageReached)
			{
				CString strMsg;
				//
				// MSG = The max number of Tabs (defined in your preferences) is %d.\nIncoming new Tabs will destroy the oldest ones.")
				strMsg.Format (IDS_MAX_TAB_REACHED, property.GetMaxTab());
				AfxMessageBox (strMsg);
				m_bMaxPageReached = TRUE;
			}

			if (pDoc->IsUsedTracePage())
			{
				nPosDelete = nTab - 2;
				m_cTab1.GetItem (nPosDelete, &item);
				pPage = (CDialog*)item.lParam;
				if (pPage)
					pPage->DestroyWindow();
				m_cTab1.DeleteItem (nPosDelete);
			}
			else
			{
				nPosDelete = nTab - 1;
				m_cTab1.GetItem (nPosDelete, &item);
				pPage = (CDialog*)item.lParam;
				if (pPage)
					pPage->DestroyWindow();
				m_cTab1.DeleteItem (nPosDelete);
			}
		}
	}
	else
	{
		BOOL bDeleted = (m_cTab1.GetItemCount() > property.GetMaxTab());
		//
		// "Some old Tabs will be removed in order to reflect the setting changes.";
		if (bDeleted)
			AfxMessageBox(IDS_MSG_REMOVE_OLD_TAB); 
		while (m_cTab1.GetItemCount() > property.GetMaxTab())
		{
			if (pDoc->IsUsedTracePage())
			{
				nPosDelete = m_cTab1.GetItemCount() - 2;
				m_cTab1.GetItem (nPosDelete, &item);
				pPage = (CDialog*)item.lParam;
				if (pPage)
					pPage->DestroyWindow();
				m_cTab1.DeleteItem (nPosDelete);
			}
			else
			{
				nPosDelete = m_cTab1.GetItemCount() - 1;
				m_cTab1.GetItem (nPosDelete, &item);
				pPage = (CDialog*)item.lParam;
				if (pPage)
					pPage->DestroyWindow();
				m_cTab1.DeleteItem (nPosDelete);
			}
		}

		if (bDeleted)
			ChangeTab(FALSE); // Render the display because of some dirty garbages on the screen !
	}
}

void CuDlgSqlQueryResult::SelectRawPage ()
{
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask  = TCIF_PARAM;
	item.iImage= -1;
	CvSqlQueryLowerView* pParent1 = (CvSqlQueryLowerView*)GetParent (); 
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;

	CaSqlQueryProperty& property = pDoc->GetProperty();
	if (pDoc->IsUsedTracePage() && property.IsTraceToTop())
	{
		int i, nTab = m_cTab1.GetItemCount();
		for (i=0; i<nTab; i++)
		{
			m_cTab1.GetItem (i, &item);
			CDialog* pPage = (CDialog*)item.lParam;
			if (pPage && IsWindow (pPage->m_hWnd))
			{
				if (BM_TRACE == (long)pPage->SendMessage(WM_SQL_TAB_BITMAP))
				{
					if (m_pCurrentPage)
						m_pCurrentPage->ShowWindow(SW_HIDE);
					m_pCurrentPage = pPage;
					m_cTab1.SetCurSel(i);
					ChangeTab(FALSE);
					break;
				}
			}
		}
	}
}
