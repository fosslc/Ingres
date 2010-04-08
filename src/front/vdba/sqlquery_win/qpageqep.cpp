/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageqep.cpp, Implementation file    (Modeless Dialog) 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a view for drawing the binary tree of boxes of QEP
**               It contains a document which itself contains a list of qep's trees.
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 07-Jan-2000 (uk$so01)
**    Bug #99940, 
**    The non-selected statements will not be exected any more in the QEP mode.
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "qframe.h"
#include "qpageqep.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgSqlQueryPageQep::CuDlgSqlQueryPageQep(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryPageQep::IDD, pParent)
{
	m_bShowStatement = FALSE;
	//{{AFX_DATA_INIT(CuDlgSqlQueryPageQep)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
	m_strDatabase = _T("");
	m_nStart = 0L;
	m_nEnd   = 0L;
	m_pDoc = NULL;
	m_pCurrentPage = NULL;
}


void CuDlgSqlQueryPageQep::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryPageQep)
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Control(pDX, IDC_STATIC1, m_cStat1);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EDIT2,   m_cEdit2);
	DDX_Control(pDX, IDC_STATIC3, m_cStat3);
	DDX_Text(pDX, IDC_EDIT2, m_strDatabase);


}

void CuDlgSqlQueryPageQep::NotifyLoad (CaQueryQepPageData* pData)
{
	m_pDoc = pData->m_pQepDoc;

	m_bShowStatement = pData->m_bShowStatement;
	m_nStart         = pData->m_nStart;
	m_nEnd           = pData->m_nEnd;
	m_strEdit1       = pData->m_strStatement;
	m_strDatabase    = pData->m_strDatabase;
	UpdateData (FALSE);
	ResizeControls();
	OnUpdateData (0, 0);
}

void CuDlgSqlQueryPageQep::ResizeControls()
{
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	if (!IsWindow (m_cTab1.m_hWnd))
		return;
	if (m_bShowStatement)
	{
		m_cEdit1.ShowWindow(SW_SHOW); // Statement
		m_cStat1.ShowWindow(SW_SHOW); // Statement
		m_cEdit2.ShowWindow(SW_SHOW); // Database
		m_cStat3.ShowWindow(SW_SHOW); // Database
	}
	else
	{
		m_cEdit1.ShowWindow(SW_HIDE); // Statement
		m_cStat1.ShowWindow(SW_HIDE); // Statement
		m_cEdit2.ShowWindow(SW_HIDE); // Database
		m_cStat3.ShowWindow(SW_HIDE); // Database
	}
	CRect rDlg, r;
	GetClientRect (rDlg);

	m_cEdit1.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right;
	m_cEdit1.MoveWindow (r);

	r.top    = m_bShowStatement? r.bottom + 4: rDlg.top;
	r.left   = rDlg.left;
	r.bottom = rDlg.bottom;
	m_cTab1.MoveWindow (r);
	if (m_pCurrentPage)
	{
		m_cTab1.GetClientRect (r);
		m_cTab1.AdjustRect (FALSE, r);
		m_pCurrentPage->MoveWindow (r);
	}
}


BEGIN_MESSAGE_MAP(CuDlgSqlQueryPageQep, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryPageQep)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGING, IDC_TAB1, OnSelchangingTab1)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_UPADATE_DATA,    OnUpdateData)
	ON_MESSAGE (WM_SQL_QUERY_PAGE_SHOWHEADER, OnDisplayHeader)
	ON_MESSAGE (WM_SQL_QUERY_PAGE_HIGHLIGHT,  OnHighlightStatement)
	ON_MESSAGE (WM_SQL_GETPAGE_DATA,          OnGetData)
	ON_MESSAGE (WM_SQL_TAB_BITMAP,            OnGetTabBitmap)
END_MESSAGE_MAP()


LONG CuDlgSqlQueryPageQep::OnDisplayHeader (WPARAM wParam, LPARAM lParam)
{
	m_bShowStatement = TRUE;
	CRect r;
	GetClientRect (r);
	OnSize (SIZE_RESTORED, r.Width(), r.Height());
	return 0L;
}


LONG CuDlgSqlQueryPageQep::OnHighlightStatement (WPARAM wParam, LPARAM lParam)
{
	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return 0;
	CvSqlQueryRichEditView* pRichEditView = pFrame->GetRichEditView();
	ASSERT (pRichEditView);
	if (!pRichEditView)
		return 0;
	CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)pRichEditView->GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return 0L;
	pRichEditView->SetColor (-1,  0, pDoc->m_crColorText);
	if (m_bShowStatement)
		return 0L;
	pRichEditView->SetColor (m_nStart, m_nEnd, pDoc->m_crColorSelect);
	return 0L;
}


LONG CuDlgSqlQueryPageQep::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaQueryQepPageData* pData = NULL;
	try
	{
		pData = new CaQueryQepPageData();
		pData->m_nID            = IDD;
		pData->m_bShowStatement = m_bShowStatement;
		pData->m_strStatement   = m_strEdit1;
		pData->m_nStart         = m_nStart;
		pData->m_nEnd           = m_nEnd;
		pData->m_strDatabase    = m_strDatabase;
		pData->m_pQepDoc        = m_pDoc;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return (LONG)NULL;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_GETDATA_FOR_SAVE, MB_ICONEXCLAMATION|MB_OK);
		if (pData)
			delete pData;
		return (LONG)NULL;
	}
	return (LONG)pData;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgSqlQueryPageQep message handlers

void CuDlgSqlQueryPageQep::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgSqlQueryPageQep::OnInitDialog() 
{
	CDialog::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryPageQep::OnDestroy() 
{
	try
	{
		if (m_pDoc)
			delete m_pDoc;
		CDialog::OnDestroy();
	}
	catch (...)
	{
	}
}

void CuDlgSqlQueryPageQep::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

LONG CuDlgSqlQueryPageQep::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	TC_ITEM tcitem;
	memset (&tcitem, 0, sizeof (tcitem));
	tcitem.mask       = TCIF_TEXT|TCIF_PARAM;
	tcitem.cchTextMax = 64;
	//
	// Create the individual page for each Qep's tree.
	ASSERT (m_pDoc != NULL);
	if (m_pDoc == NULL)
		return 0L;
	int  nTab = 0;
	BOOL bCreateOK = FALSE;
	CaSqlQueryExecutionPlanData* pData = NULL;
	POSITION pos = m_pDoc->m_listQepData.GetHeadPosition();
	while (pos != NULL)
	{
		CuDlgSqlQueryPageQepIndividual* pQepPage = new CuDlgSqlQueryPageQepIndividual (&m_cTab1);
		pQepPage->m_nQepSequence          = pos;
		pData = m_pDoc->m_listQepData.GetNext(pos);
		pQepPage->m_pDoc                  = m_pDoc;
		pQepPage->m_strTable              = pData->m_strGenerateTable;
		pQepPage->m_bTimeOut              = pData->m_bTimeOut;
		pQepPage->m_bLargeTemporaries     = pData->m_bLargeTemporaries;
		pQepPage->m_bFloatIntegerException= pData->m_bFloatIntegerException;
		pQepPage->m_pListAggregate        = &(pData->m_strlistAggregate);
		pQepPage->m_pListExpression       = &(pData->m_strlistExpression);
		pQepPage->m_bPreviewMode          = pData->GetDisplayMode();
		bCreateOK = pQepPage->Create (IDD_SQLQUERY_PAGEQEP_INDIVIDUAL, &m_cTab1);
		tcitem.pszText = (LPTSTR)(LPCTSTR)pData->m_strHeader;
		tcitem.lParam  = (LPARAM)pQepPage;
		m_cTab1.InsertItem (nTab, &tcitem);
		nTab++;
	}
	DisplayQepTree (0);
	return 0L;
}

void CuDlgSqlQueryPageQep::DisplayQepTree (int nTab)
{
	TC_ITEM tcitem;
	memset (&tcitem, 0, sizeof (tcitem));
	tcitem.mask = TCIF_PARAM;
	if (m_pCurrentPage)
		m_pCurrentPage->ShowWindow (SW_HIDE);
	if (m_cTab1.GetItem (nTab, &tcitem))
	{
		CRect r;
		CuDlgSqlQueryPageQepIndividual* pPage = (CuDlgSqlQueryPageQepIndividual*)tcitem.lParam;
		if (!pPage)
			return;
		m_cTab1.GetClientRect (r);
		m_cTab1.AdjustRect (FALSE, r);
		pPage->MoveWindow (r);
		pPage->ShowWindow (SW_SHOW);
		m_pCurrentPage = pPage;
	}
}

void CuDlgSqlQueryPageQep::OnSelchangingTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CuDlgSqlQueryPageQep::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int nSel = m_cTab1.GetCurSel();
	DisplayQepTree (nSel);
	
	*pResult = 0;
}

LONG CuDlgSqlQueryPageQep::OnGetTabBitmap (WPARAM wParam, LPARAM lParam)
{
	return (LONG)BM_QEP;
}