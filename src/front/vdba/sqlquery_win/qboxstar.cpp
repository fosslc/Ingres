/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qboxstar.cpp, Implementation file
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree. (Star)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "qboxstar.h"
#include "qepbtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgQueryExecutionPlanBoxStar::CuDlgQueryExecutionPlanBoxStar(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgQueryExecutionPlanBoxStar::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgQueryExecutionPlanBoxStar)
	m_strC = _T("");
	m_strD = _T("");
	m_strN = _T("");
	m_strDB = _T("");
	m_strHeader = _T("");
	m_strNode = _T("");
	m_strPage = _T("");
	m_strTuple = _T("");
	//}}AFX_DATA_INIT
	m_Indicator1.SetMode(TRUE); // Vertical
	m_Indicator2.SetMode(TRUE); // Vertical
	m_Indicator3.SetMode(TRUE); // Vertical
	m_bPopupInfo = FALSE;
}

CuDlgQueryExecutionPlanBoxStar::CuDlgQueryExecutionPlanBoxStar(CWnd* pParent, BOOL bPopupInfo)
	: CDialog(CuDlgQueryExecutionPlanBoxStar::IDD_POPUPINFO, pParent)
{
	ASSERT (bPopupInfo == TRUE);
	m_bPopupInfo = FALSE;
	//{{AFX_DATA_INIT(CuDlgQueryExecutionPlanBoxStar)
	m_strC = _T("");
	m_strD = _T("");
	m_strN = _T("");
	m_strDB = _T("");
	m_strHeader = _T("");
	m_strNode = _T("");
	m_strPage = _T("");
	m_strTuple = _T("");
	//}}AFX_DATA_INIT
	m_Indicator1.SetMode(TRUE); // Vertical
	m_Indicator2.SetMode(TRUE); // Vertical
	m_Indicator3.SetMode(TRUE); // Vertical
}

void CuDlgQueryExecutionPlanBoxStar::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgQueryExecutionPlanBoxStar)
	DDX_Text(pDX, IDC_STATIC_C, m_strC);
	DDX_Text(pDX, IDC_STATIC_D, m_strD);
	DDX_Text(pDX, IDC_STATIC_N, m_strN);
	DDX_Text(pDX, IDC_STATIC_DB, m_strDB);
	DDX_Text(pDX, IDC_STATIC_HEADER, m_strHeader);
	DDX_Text(pDX, IDC_STATIC_NODE, m_strNode);
	DDX_Text(pDX, IDC_STATIC_PAGE, m_strPage);
	DDX_Text(pDX, IDC_STATIC_TUPLE, m_strTuple);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_BITMAP_DB,    m_cBmpDatabase);
	DDX_Control(pDX, IDC_BITMAP_PAGE,  m_cBmpPage);
	DDX_Control(pDX, IDC_BITMAP_TUPLE, m_cBmpTuple);
	DDX_Control(pDX, IDC_BITMAP_D,     m_cBmpD);
	DDX_Control(pDX, IDC_BITMAP_C,     m_cBmpC);
	DDX_Control(pDX, IDC_BITMAP_N,     m_cBmpN);
}

void CuDlgQueryExecutionPlanBoxStar::SetupData(CaQepNodeInformationStar* pData)
{
	m_strDB     = pData->m_strDatabase;
	m_strNode   = pData->m_strNode;
	m_strHeader = pData->m_strNodeHeader;
	m_strPage   = pData->m_strPage;
	m_strTuple  = pData->m_strTuple;
	m_strC      = pData->m_strC;
	m_strD      = pData->m_strD;
	m_strN      = pData->m_strN;
	m_Indicator1.DeleteIndicator();
	m_Indicator2.DeleteIndicator();
	m_Indicator3.DeleteIndicator();
	POSITION pos = pData->m_listPercent1.GetHeadPosition();
	while (pos != NULL)
	{
		CaStaticIndicatorData* p = pData->m_listPercent1.GetNext (pos);
		m_Indicator1.AddIndicator (p->m_crPercent, p->m_dPercent);
	}

	pos = pData->m_listPercent2.GetHeadPosition();
	while (pos != NULL)
	{
		CaStaticIndicatorData* p = pData->m_listPercent2.GetNext (pos);
		m_Indicator2.AddIndicator (p->m_crPercent, p->m_dPercent);
	}
	pos = pData->m_listPercent3.GetHeadPosition();
	while (pos != NULL)
	{
		CaStaticIndicatorData* p = pData->m_listPercent3.GetNext (pos);
		m_Indicator3.AddIndicator (p->m_crPercent, p->m_dPercent);
	}
}


BEGIN_MESSAGE_MAP(CuDlgQueryExecutionPlanBoxStar, CDialog)
	//{{AFX_MSG_MAP(CuDlgQueryExecutionPlanBoxStar)
	ON_WM_MOVE()
	ON_WM_NCPAINT()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_UPADATE_DATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgQueryExecutionPlanBoxStar message handlers

LONG CuDlgQueryExecutionPlanBoxStar::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CaQepNodeInformationStar* pData = (CaQepNodeInformationStar*)lParam;
	SetupData (pData);
	UpdateData (FALSE);
	return 0L;
}


void CuDlgQueryExecutionPlanBoxStar::PostNcDestroy() 
{
	m_bmpDatabase.DeleteObject();
	m_bmpPage.DeleteObject();
	m_bmpTuple.DeleteObject();
	m_bmpD.DeleteObject();
	m_bmpC.DeleteObject();
	m_bmpN.DeleteObject();
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgQueryExecutionPlanBoxStar::OnInitDialog() 
{
	m_Indicator1.SubclassDlgItem (IDC_STATIC1, this);
	m_Indicator2.SubclassDlgItem (IDC_STATIC2, this);
	m_Indicator3.SubclassDlgItem (IDC_STATIC3, this);

	m_bmpDatabase.LoadBitmap(IDB_QEP_DB);
	m_bmpPage.LoadBitmap(IDB_QEP_PAGE);
	m_bmpTuple.LoadBitmap(IDB_QEP_TUPLE);
	m_bmpD.LoadBitmap(IDB_QEP_D);
	m_bmpC.LoadBitmap(IDB_QEP_C);
	m_bmpN.LoadBitmap(IDB_QEP_N);

	m_cBmpDatabase.SetBitmap((HBITMAP)m_bmpDatabase);
	m_cBmpPage.SetBitmap((HBITMAP)m_bmpPage);
	m_cBmpTuple.SetBitmap((HBITMAP)m_bmpTuple);
	m_cBmpD.SetBitmap((HBITMAP)m_bmpD);
	m_cBmpC.SetBitmap((HBITMAP)m_bmpC);
	m_cBmpN.SetBitmap((HBITMAP)m_bmpN);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgQueryExecutionPlanBoxStar::OnMove(int x, int y) 
{
	CDialog::OnMove(x, y);
	if (!m_bPopupInfo)
		GetParent()->SendMessage (WM_QEP_REDRAWLINKS, (WPARAM)0, (LPARAM)0);
}

void CuDlgQueryExecutionPlanBoxStar::OnNcPaint() 
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


