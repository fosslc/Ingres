/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qstmtdlg.cpp, Implementation file    (Modeless Dialog) 
**    Project  : CA-OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains an Edit for SQL statement and other static controls that 
**               hold the information about execute time, elap time, 
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
#include "qstmtdlg.h"
#include "qupview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgSqlQueryStatement::CuDlgSqlQueryStatement(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryStatement::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSqlQueryStatement)
	m_strCom = _T("");
	m_strExec = _T("");
	m_strElap = _T("");
	m_strCost = _T("");
	//}}AFX_DATA_INIT
}


void CuDlgSqlQueryStatement::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryStatement)
	DDX_Text(pDX, IDC_STATIC1, m_strCom);
	DDX_Text(pDX, IDC_STATIC2, m_strExec);
	DDX_Text(pDX, IDC_STATIC3, m_strElap);
	DDX_Text(pDX, IDC_STATIC4, m_strCost);
	//}}AFX_DATA_MAP
}

void CuDlgSqlQueryStatement::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgSqlQueryStatement::DisplayStatistic (CaSqlQueryMeasureData* pData)
{
	if (!pData)
		return;
	m_strCom  = pData->m_strCom;
	m_strExec = pData->m_strExec;
	m_strElap = pData->m_strElap;
	m_strCost = pData->m_strCost;
	UpdateData (FALSE);
}




BEGIN_MESSAGE_MAP(CuDlgSqlQueryStatement, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryStatement)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CuDlgSqlQueryStatement::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CvSqlQueryUpperView* pParent1 = (CvSqlQueryUpperView*)GetParent ();                 // CvSqlQueryUpperView.
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();     // CdSqlQueryRichEditDoc.
	ASSERT (pDoc);
	if (pDoc && pDoc->IsLoadedDoc())
	{
		m_strCom  = pDoc->m_strCom;
		m_strExec = pDoc->m_strExec;
		m_strElap = pDoc->m_strElap;
		m_strCost = pDoc->m_strCost;
		UpdateData (FALSE);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryStatement::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
}



void CuDlgSqlQueryStatement::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CuDlgSqlQueryStatement::LoadDocument() 
{
	CvSqlQueryUpperView* pParent1 = (CvSqlQueryUpperView*)GetParent ();                 // CvSqlQueryUpperView.
	ASSERT (pParent1);
	CdSqlQueryRichEditDoc* pDoc =  (CdSqlQueryRichEditDoc*)pParent1->GetDocument();     // CdSqlQueryRichEditDoc.
	ASSERT (pDoc);
	if (pDoc && pDoc->IsLoadedDoc())
	{
		m_strCom  = pDoc->m_strCom;
		m_strExec = pDoc->m_strExec;
		m_strElap = pDoc->m_strElap;
		m_strCost = pDoc->m_strCost;
		UpdateData (FALSE);
	}
}