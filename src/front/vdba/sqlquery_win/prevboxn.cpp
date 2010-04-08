/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : prevboxn.cpp, Implementation file
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Node of Qep tree.(mode preview)
**
** History:
**
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
#include "prevboxn.h"
#include "qepbtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgQueryExecutionPlanBoxPreview::CuDlgQueryExecutionPlanBoxPreview(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgQueryExecutionPlanBoxPreview::IDD, pParent)
{
	m_bDisplayIndicator = TRUE;
	m_bRoot = FALSE;
	//{{AFX_DATA_INIT(CuDlgQueryExecutionPlanBoxPreview)
	//}}AFX_DATA_INIT
	m_bInitialize = FALSE;
	m_Indicator1.SetMode(TRUE); // Virtical
	m_Indicator2.SetMode(TRUE); // Virtical
	m_Color.SetPaletteAware(FALSE);
}


void CuDlgQueryExecutionPlanBoxPreview::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgQueryExecutionPlanBoxPreview)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgQueryExecutionPlanBoxPreview, CDialog)
	//{{AFX_MSG_MAP(CuDlgQueryExecutionPlanBoxPreview)
	ON_WM_MOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_UPADATE_DATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgQueryExecutionPlanBoxPreview message handlers

void CuDlgQueryExecutionPlanBoxPreview::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CDialog::OnLButtonDown(nFlags, point);
	SetCapture();
	GetParent()->SendMessage (WM_QEP_OPEN_POPUPINFO, 0, (LPARAM)this);
}

void CuDlgQueryExecutionPlanBoxPreview::OnLButtonUp(UINT nFlags, CPoint point)
{
	CDialog::OnLButtonUp(nFlags, point);
	ReleaseCapture();
	GetParent()->SendMessage (WM_QEP_CLOSE_POPUPINFO, 0, (LPARAM)this);
}



LONG CuDlgQueryExecutionPlanBoxPreview::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CaQepNodeInformation* pData = (CaQepNodeInformation*)lParam;
	m_Color.SetColor (pData->m_crColorPreview);
	m_Color.SetWindowText (pData->m_strNodeType);
	if (!m_bDisplayIndicator)
		return 0L;
	m_Indicator1.DeleteIndicator();
	m_Indicator2.DeleteIndicator();
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
	return 0L;
}


void CuDlgQueryExecutionPlanBoxPreview::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


void CuDlgQueryExecutionPlanBoxPreview::OnMove(int x, int y) 
{
	CDialog::OnMove(x, y);
	GetParent()->SendMessage (WM_QEP_REDRAWLINKS, (WPARAM)0, (LPARAM)0);
}

BOOL CuDlgQueryExecutionPlanBoxPreview::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_Indicator1.SubclassDlgItem (IDC_STATIC1, this);
	m_Indicator2.SubclassDlgItem (IDC_STATIC2, this);
	m_Color.SubclassDlgItem (IDC_COLOR, this);
	m_bInitialize = TRUE;
	if (!m_bDisplayIndicator)
	{
		CRect r;
		m_Indicator1.ShowWindow (SW_HIDE);
		m_Indicator2.ShowWindow (SW_HIDE);
		GetClientRect (r);
		r.top    += 2;
		r.left   += 2;
		r.right  -= 2;
		r.bottom -= 2;
		m_Color.MoveWindow (r);
	}

	return TRUE;
}

