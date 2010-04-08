/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qboxleaf.cpp, Implementation file 
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree. (Leaf)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "qboxleaf.h"
#include "qepbtree.h"
#include "transbmp.h"   // DrawTransparentBitmap

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgQueryExecutionPlanBoxLeaf::CuDlgQueryExecutionPlanBoxLeaf(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgQueryExecutionPlanBoxLeaf::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgQueryExecutionPlanBoxLeaf)
	m_strHeader = _T("");
	m_strPage = _T("");
	m_strTuple = _T("");
	//}}AFX_DATA_INIT
	m_bPopupInfo = FALSE;
}

CuDlgQueryExecutionPlanBoxLeaf::CuDlgQueryExecutionPlanBoxLeaf(CWnd* pParent, BOOL bPopupInfo)
	: CDialog(CuDlgQueryExecutionPlanBoxLeaf::IDD_POPUPINFO, pParent)
{
	ASSERT (bPopupInfo == TRUE);
	m_bPopupInfo = TRUE;
	//{{AFX_DATA_INIT(CuDlgQueryExecutionPlanBoxLeaf)
	m_strHeader = _T("");
	m_strPage = _T("");
	m_strTuple = _T("");
	//}}AFX_DATA_INIT
}

void CuDlgQueryExecutionPlanBoxLeaf::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgQueryExecutionPlanBoxLeaf)
	DDX_Text(pDX, IDC_STATIC_HEADER, m_strHeader);
	DDX_Text(pDX, IDC_STATIC_PAGE, m_strPage);
	DDX_Text(pDX, IDC_STATIC_TUPLE, m_strTuple);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_BITMAP_PAGE, m_cBmpPage);
	DDX_Control(pDX, IDC_BITMAP_TUPLE, m_cBmpTuple);
}

void CuDlgQueryExecutionPlanBoxLeaf::SetupData(CaQepNodeInformation* pData)
{
	m_strHeader = pData->m_strNodeHeader;
	m_strPage   = pData->m_strPage;
	m_strTuple  = pData->m_strTuple;
}

BEGIN_MESSAGE_MAP(CuDlgQueryExecutionPlanBoxLeaf, CDialog)
	//{{AFX_MSG_MAP(CuDlgQueryExecutionPlanBoxLeaf)
	ON_WM_MOVE()
	ON_WM_NCPAINT()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_UPADATE_DATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgQueryExecutionPlanBoxLeaf message handlers

LONG CuDlgQueryExecutionPlanBoxLeaf::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CaQepNodeInformation* pData = (CaQepNodeInformation*)lParam;
	SetupData (pData);
	UpdateData (FALSE);
	return 0L;
}

void CuDlgQueryExecutionPlanBoxLeaf::PostNcDestroy() 
{
	m_bmpPage.DeleteObject();
	m_bmpTuple.DeleteObject();
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgQueryExecutionPlanBoxLeaf::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_bmpPage.LoadBitmap(IDB_QEP_PAGE);
	m_bmpTuple.LoadBitmap(IDB_QEP_TUPLE);

	m_cBmpPage.SetBitmap((HBITMAP)m_bmpPage);
	m_cBmpTuple.SetBitmap((HBITMAP)m_bmpTuple);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgQueryExecutionPlanBoxLeaf::OnMove(int x, int y) 
{
	CDialog::OnMove(x, y);
	if (!m_bPopupInfo)
		GetParent()->SendMessage (WM_QEP_REDRAWLINKS, (WPARAM)0, (LPARAM)0);
}

void CuDlgQueryExecutionPlanBoxLeaf::OnNcPaint() 
{
	// Default method
	CDialog::OnNcPaint();

	// Draw our bitmap in transparent mode
	CWindowDC captDC(this);
	// captDC.SetMapMode(MM_TEXT);
	LONG lIconId = GetWindowLong(m_hWnd, GWL_USERDATA);
	UINT iconId = (UINT)lIconId;

	// iconId can be NULL: preview mode
	if (iconId) {
		CBitmap bmp;
		BOOL bSuccess = bmp.LoadBitmap(iconId);
		ASSERT (bSuccess);
		BITMAP bm;
		bmp.GetBitmap(&bm);
		short dx = GetSystemMetrics(SM_CXDLGFRAME)+1;
		short dy = GetSystemMetrics(SM_CYDLGFRAME)+1;

		COLORREF transparent = RGB(255, 0, 255);
		DrawTransparentBitmap(captDC.m_hDC, (HBITMAP)bmp, dx, dy, transparent);
	}
}
