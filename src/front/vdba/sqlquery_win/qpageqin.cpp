/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageqin.cpp, Implementation file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a a scroll view for drawing the tree of qep.
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
#include "qpageqin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgSqlQueryPageQepIndividual::CuDlgSqlQueryPageQepIndividual(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryPageQepIndividual::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSqlQueryPageQepIndividual)
	m_bTimeOut = FALSE;
	m_bLargeTemporaries = FALSE;
	m_bFloatIntegerException = FALSE;
	m_strTable = _T("");
	//}}AFX_DATA_INIT
	m_pQepFrame = NULL;
	m_pDoc = NULL;
	m_nQepSequence = NULL;
	m_pListAggregate = NULL;
	m_pListExpression = NULL;
	m_bPreviewMode = TRUE; 
}


void CuDlgSqlQueryPageQepIndividual::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryPageQepIndividual)
	DDX_Control(pDX, IDC_STATIC2, m_cStaticFrame);
	DDX_Control(pDX, IDC_STATIC3, m_cStaticExpression);
	DDX_Control(pDX, IDC_STATIC1, m_cStaticAggregate);
	DDX_Control(pDX, IDC_LIST2, m_cList2);
	DDX_Control(pDX, IDC_LIST1, m_cList1);
	DDX_Check(pDX, IDC_CHECK1, m_bTimeOut);
	DDX_Check(pDX, IDC_CHECK3, m_bLargeTemporaries);
	DDX_Check(pDX, IDC_CHECK4, m_bFloatIntegerException);
	DDX_Text(pDX, IDC_EDIT1, m_strTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgSqlQueryPageQepIndividual, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryPageQepIndividual)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON1, OnPreview)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgSqlQueryPageQepIndividual message handlers

void CuDlgSqlQueryPageQepIndividual::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}

void CuDlgSqlQueryPageQepIndividual::OnDestroy() 
{
	try
	{
		if (m_pQepFrame)
		{
			m_pQepFrame->DestroyWindow();
			m_pQepFrame = NULL;
		}
		CDialog::OnDestroy();
	}
	catch (...)
	{
	}
}

BOOL CuDlgSqlQueryPageQepIndividual::OnInitDialog() 
{
	CDialog::OnInitDialog();
	ASSERT (m_pDoc != NULL);
	CaSqlQueryExecutionPlanData* pQepData = m_pDoc->m_listQepData.GetAt(m_nQepSequence);
	m_strNormal.LoadString (IDS_QEP_NORMAL);
	m_strPreview.LoadString (IDS_QEP_PREVIEW);

	m_bPreviewMode = pQepData? pQepData->GetDisplayMode(): FALSE;
	CString strLabel = m_bPreviewMode? m_strNormal: m_strPreview;
	GetDlgItem (IDC_BUTTON1)->SetWindowText (strLabel);

	POSITION pos = m_pListAggregate->GetHeadPosition();
	while (pos != NULL)
	{
		CString& strItem = m_pListAggregate->GetNext (pos);
		m_cList1.AddString (strItem);
	}
	pos = m_pListExpression->GetHeadPosition();
	while (pos != NULL)
	{
		CString& strItem = m_pListExpression->GetNext (pos);
		m_cList2.AddString (strItem);
	}

	CRect    r;
	GetClientRect (r);
	try
	{
		m_pQepFrame = new CfQueryExecutionPlanFrame(m_pDoc);
		m_pQepFrame->Create (
			NULL,
			NULL,
			WS_CHILD,
			r,
			this);
		m_pQepFrame->ShowWindow (SW_SHOW);
		m_pQepFrame->InitialUpdateFrame (m_pDoc, TRUE);
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_CREATE_QEP_RESULT, MB_ICONEXCLAMATION|MB_OK);
		m_pQepFrame = NULL;
	}
	ResizeControls();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryPageQepIndividual::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

void CuDlgSqlQueryPageQepIndividual::ResizeControls()
{
	if (!IsWindow (m_cStaticFrame.m_hWnd))
		return;
	if (!m_pQepFrame && !IsWindow (m_pQepFrame->m_hWnd))
		return;

	CRect rDlg, r;
	GetClientRect (rDlg);
	m_cStaticFrame.GetWindowRect (r);
	ScreenToClient (r);
	r.left   = rDlg.left;
	r.right  = rDlg.right;
	r.bottom = rDlg.bottom;

	BOOL bMaximize = (m_cList1.GetCount() + m_cList2.GetCount()) == 0;
	if (bMaximize)
	{
		CRect r2;
		m_cStaticAggregate.ShowWindow (SW_HIDE); 
		m_cStaticExpression.ShowWindow(SW_HIDE); 
		m_cList1.ShowWindow(SW_HIDE);
		m_cList2.ShowWindow(SW_HIDE); 
		m_cList1.GetWindowRect (r2);
		ScreenToClient (r2);
		r.top = r2.top;
	}
	else
	{
		m_cStaticAggregate.ShowWindow (SW_SHOW); 
		m_cStaticExpression.ShowWindow(SW_SHOW); 
		m_cList1.ShowWindow(SW_SHOW);
		m_cList2.ShowWindow(SW_SHOW); 
	}
	m_pQepFrame->MoveWindow (r);
}

void CuDlgSqlQueryPageQepIndividual::OnPreview() 
{
	m_bPreviewMode = !m_bPreviewMode;
	CString strLabel = m_bPreviewMode? m_strNormal: m_strPreview;
	GetDlgItem (IDC_BUTTON1)->SetWindowText (strLabel);
	m_pQepFrame->SetMode (m_bPreviewMode, m_nQepSequence);
}
