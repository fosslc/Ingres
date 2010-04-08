/******************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : icepassw.cpp : implementation file
**
**
**    Project  : Visual DBA.
**    Author   : Schalk Philippe
**
**    Purpose  : Popup Dialog Box for fill username and Password.
**
**    28-Feb-2000(schph01)
**    Sir #100559
**    Verify the username and password before to leave the dialog box.
**
******************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "icepassw.h"
extern "C" 
{
#include "ice.h"
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MESSAGE_LEN 100

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgIcePassword( LPICEPASSWORDDATA lpPassworddta , LPUCHAR lpVnode)
{
	CxDlgIcePassword dlg;
	memset(lpPassworddta,0,sizeof(ICEPASSWORDDATA));
	dlg.m_csVnodeName = lpVnode;
	int iret = dlg.DoModal();
	if (iret == IDOK)
	{
		lstrcpy(lpPassworddta->tchUserName,dlg.m_csUserName);
		lstrcpy(lpPassworddta->tchPassword,dlg.m_csPassword);
		return TRUE;
	}
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIcePassword dialog


CxDlgIcePassword::CxDlgIcePassword(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIcePassword::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIcePassword)
	m_csPassword = _T("");
	m_csUserName = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIcePassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIcePassword)
	DDX_Control(pDX, IDC_ICE_PASSWORD, m_ctrledPassword);
	DDX_Control(pDX, IDC_ICE_USER, m_ctrledUserName);
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Text(pDX, IDC_ICE_PASSWORD, m_csPassword);
	DDX_Text(pDX, IDC_ICE_USER, m_csUserName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIcePassword, CDialog)
	//{{AFX_MSG_MAP(CxDlgIcePassword)
	ON_EN_CHANGE(IDC_ICE_PASSWORD, OnChangePassword)
	ON_EN_CHANGE(IDC_ICE_USER, OnChangeUser)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIcePassword message handlers

void CxDlgIcePassword::OnChangePassword() 
{
	EnableDisableOK();
}

void CxDlgIcePassword::OnChangeUser() 
{
	EnableDisableOK();
}

void CxDlgIcePassword::OnOK()
{
	TCHAR tcErrorBuf[MESSAGE_LEN];

	UpdateData(TRUE);
	if (!IsIceLoginPasswordValid((LPTSTR)(LPCTSTR)m_csVnodeName,
								 (LPTSTR)(LPCTSTR)m_csUserName,
								 (LPTSTR)(LPCTSTR)m_csPassword,tcErrorBuf,MESSAGE_LEN))
	{
		CString csMsg;
		csMsg.Format(VDBA_MfcResourceString (IDS_E_ICE_VERIF_CONNECTION_FAILED),tcErrorBuf);
		AfxMessageBox(csMsg, MB_ICONSTOP);
		return;
	}

	CDialog::OnOK();
}

void CxDlgIcePassword::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
	

}

BOOL CxDlgIcePassword::OnInitDialog() 
{
	CDialog::OnInitDialog();
	EnableDisableOK();
	PushHelpId (IDD_ICE_PASSWORD);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIcePassword::EnableDisableOK()
{
	if (m_ctrledPassword.LineLength() == 0 ||
		m_ctrledUserName.LineLength() == 0 )
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);

}
