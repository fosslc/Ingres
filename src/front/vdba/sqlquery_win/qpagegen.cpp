/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpagegen.cpp, Implementation file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains an edit to show a non-select statement.
**               This dialog is displayed upon the setting. 
**
** History:
** xx-Oct-1997 (uk$so01)
**    Created
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "qframe.h"
#include "qpagegen.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgSqlQueryPageGeneric::CuDlgSqlQueryPageGeneric(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryPageGeneric::IDD, pParent)
{
	m_bShowStatement = FALSE;
	//{{AFX_DATA_INIT(CuDlgSqlQueryPageGeneric)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
	m_strDatabase = _T("");
}


void CuDlgSqlQueryPageGeneric::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryPageGeneric)
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT2, m_strDatabase);
}

void CuDlgSqlQueryPageGeneric::NotifyLoad (CaQueryNonSelectPageData* pData)
{
	m_bShowStatement = pData->m_bShowStatement;
	m_nStart         = pData->m_nStart;
	m_nEnd           = pData->m_nEnd;
	m_strEdit1       = pData->m_strStatement;
	m_strDatabase    = pData->m_strDatabase;
	UpdateData (FALSE);
	ResizeControls();
}

void CuDlgSqlQueryPageGeneric::ResizeControls()
{
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);
	m_cEdit1.GetWindowRect (r);
	ScreenToClient (r);
	r.left  = rDlg.left;
	r.right = rDlg.right;
	r.bottom= rDlg.bottom;
	m_cEdit1.MoveWindow (r);
}

BEGIN_MESSAGE_MAP(CuDlgSqlQueryPageGeneric, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryPageGeneric)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_PAGE_SHOWHEADER, OnDisplayHeader)
	ON_MESSAGE (WM_SQL_QUERY_PAGE_HIGHLIGHT,  OnHighlightStatement)
	ON_MESSAGE (WM_SQL_GETPAGE_DATA,          OnGetData)
	ON_MESSAGE (WM_SQL_TAB_BITMAP,            OnGetTabBitmap)
END_MESSAGE_MAP()


LONG CuDlgSqlQueryPageGeneric::OnDisplayHeader (WPARAM wParam, LPARAM lParam)
{
	m_bShowStatement = TRUE;
	return 0L;
}

LONG CuDlgSqlQueryPageGeneric::OnHighlightStatement (WPARAM wParam, LPARAM lParam)
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


LONG CuDlgSqlQueryPageGeneric::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaQueryNonSelectPageData* pData = NULL;
	try
	{
		pData = new CaQueryNonSelectPageData();
		pData->m_nID            = IDD;
		pData->m_bShowStatement = m_bShowStatement;
		pData->m_strStatement   = m_strEdit1;
		pData->m_nStart         = m_nStart;
		pData->m_nEnd           = m_nEnd;
		pData->m_strDatabase    = m_strDatabase;
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
// CuDlgSqlQueryPageGeneric message handlers

void CuDlgSqlQueryPageGeneric::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgSqlQueryPageGeneric::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryPageGeneric::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

LONG CuDlgSqlQueryPageGeneric::OnGetTabBitmap (WPARAM wParam, LPARAM lParam)
{
	return (LONG)BM_NON_SELECT;
}