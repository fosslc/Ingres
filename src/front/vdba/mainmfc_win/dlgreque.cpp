// dlgreque.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dlgreque.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgRaiseEventQuestion dialog


CuDlgRaiseEventQuestion::CuDlgRaiseEventQuestion(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgRaiseEventQuestion::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgRaiseEventQuestion)
	m_NewValue = 1;
	m_strQuestion = _T("");
	//}}AFX_DATA_INIT
}


void CuDlgRaiseEventQuestion::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CuDlgRaiseEventQuestion)
	DDX_Text(pDX, IDC_REPLICQUESTION, m_NewValue);
	DDX_Text(pDX, IDC_QUESTION, m_strQuestion);
	//}}AFX_DATA_MAP
	DDV_MinMaxInt(pDX, m_NewValue, m_iMin, m_iMax);
}


BEGIN_MESSAGE_MAP(CuDlgRaiseEventQuestion, CDialog)
	//{{AFX_MSG_MAP(CuDlgRaiseEventQuestion)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgRaiseEventQuestion message handlers

BOOL CuDlgRaiseEventQuestion::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetWindowText( m_strCaption );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgRaiseEventQuestion::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}
