/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : xdlgmsg.cpp : implementation file
**
**  Source   : xdlgmsg.h : header file
**  Author   : Sotheavut UK (uk$so01)
**  Purpose  : Dialog box to be used as a MessageBox
**             with a checkbox "Show this message later"
**
** History:
** 29-Jun-2000 (uk$so01)
**    SIR #105160, created for adding custom message box. 
**
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "xdlgmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgMessage::CxDlgMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgMessage)
	m_bShowItLater = TRUE;
	m_strMessage = _T("");
	//}}AFX_DATA_INIT
	m_hIcon = NULL;
}


void CxDlgMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgMessage)
	DDX_Control(pDX, IDC_STATIC2, m_cStaticIcon);
	DDX_Check(pDX, IDC_CHECK1, m_bShowItLater);
	DDX_Text(pDX, IDC_STATIC1, m_strMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgMessage, CDialog)
	//{{AFX_MSG_MAP(CxDlgMessage)
	ON_BN_CLICKED(IDNO, OnNo)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CxDlgMessage::OnOK() 
{
	UpdateData(TRUE);
	EndDialog(IDYES);
	CDialog::OnOK();
}

void CxDlgMessage::OnNo() 
{
	UpdateData(TRUE);
	EndDialog(IDNO);
}

void CxDlgMessage::OnCancel() 
{
	EndDialog(IDCANCEL);
	CDialog::OnCancel();
}

BOOL CxDlgMessage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CWinApp* pApp = AfxGetApp();
	ASSERT(pApp);
	if (pApp)
	{
		m_hIcon = theApp.LoadStandardIcon(IDI_QUESTION);
		m_cStaticIcon.SetIcon(m_hIcon);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgMessage::OnDestroy() 
{
	CDialog::OnDestroy();
	if (m_hIcon)
		DestroyIcon(m_hIcon);
}
