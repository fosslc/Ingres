/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dlgmsg.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : dialog modeless for message
**
** History:
**
** 16-Sep-1999 (uk$so01)
**    created
** 17-Jun-2002 (uk$so01)
**    BUG #107930, fix bug of displaying the message box when the number of
**    events reached the limit of setting.
*/

#include "stdafx.h"
#include "ivm.h"
#include "dlgmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgShowMessage dialog


CuDlgShowMessage::CuDlgShowMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgShowMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgShowMessage)
	m_bShowLater = TRUE;
	m_strStatic2 = _T("");
	//}}AFX_DATA_INIT
	m_pParam = NULL;
	m_bShowCheckBox = TRUE;
	m_strStatic2 = _T("You have reached the max events limit of the settings.");
}

void CuDlgShowMessage::SetParam(CaMessageBoxParam* pParam)
{
	m_pParam = pParam;
	if (!m_pParam)
		return;
	m_strStatic2 = m_pParam->GetMessage();
	m_bShowLater = m_pParam->IsChecked();
	m_bShowCheckBox = m_pParam->IsShowCheckbox();
}


void CuDlgShowMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgShowMessage)
	DDX_Control(pDX, IDC_CHECK1, m_cCheck1);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Check(pDX, IDC_CHECK1, m_bShowLater);
	DDX_Text(pDX, IDC_STATIC2, m_strStatic2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgShowMessage, CDialog)
	//{{AFX_MSG_MAP(CuDlgShowMessage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgShowMessage message handlers

BOOL CuDlgShowMessage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (m_bShowCheckBox)
		m_cCheck1.ShowWindow (SW_SHOW);
	else
		m_cCheck1.ShowWindow (SW_HIDE);
	SetWindowPos (&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgShowMessage::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgShowMessage::OnOK() 
{
	CDialog::OnOK();
	if (m_pParam)
	{
		m_pParam->SetChecked(m_bShowLater);
		m_pParam->EndMessage();
		delete m_pParam;
		m_pParam = NULL;
	}
}

void CuDlgShowMessage::OnCancel() 
{
	CDialog::OnCancel();
}
