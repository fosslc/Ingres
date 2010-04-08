/******************************************************************************
**                                                                           
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**                                                                           
**    Source   : IceDUser.cpp : implementation file                          
**                                                                           
**                                                                           
**    Project  : Ingres II/Vdba.                                             
**    Author   : Schalk Philippe                                             
**                                                                           
**    Purpose  : Popup Dialog Box for create Ice User.                       
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
******************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "iceduser.h"
#include "dgerrh.h"

extern "C"
{
#include "main.h"
#include "domdata.h"
#include "getdata.h"
#include "msghandl.h"
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateIceDbUser( LPICEDBUSERDATA lpCreateDbUser ,int nHnode)
{
	CxDlgIceDbUser dlg;
	dlg.m_pStructDbUser = lpCreateDbUser;
	dlg.m_csVirtNodeName = GetVirtNodeName (nHnode);
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceDbUser dialog


CxDlgIceDbUser::CxDlgIceDbUser(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceDbUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceDbUser)
	m_csAlias = _T("");
	m_csComment = _T("");
	m_csConfirm = _T("");
	m_csName = _T("");
	m_csPassword = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceDbUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceDbUser)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_DBU_PASSWORD, m_ctrlPassWord);
	DDX_Control(pDX, IDC_DBU_NAME, m_ctrlName);
	DDX_Control(pDX, IDC_DBU_CONFIRM, m_ctrlConfirm);
	DDX_Control(pDX, IDC_DBU_COMMENT, m_ctrlComment);
	DDX_Control(pDX, IDC_DBU_ALIAS, m_ctrlAlias);
	DDX_Text(pDX, IDC_DBU_ALIAS, m_csAlias);
	DDX_Text(pDX, IDC_DBU_COMMENT, m_csComment);
	DDX_Text(pDX, IDC_DBU_CONFIRM, m_csConfirm);
	DDX_Text(pDX, IDC_DBU_NAME, m_csName);
	DDX_Text(pDX, IDC_DBU_PASSWORD, m_csPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceDbUser, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceDbUser)
	ON_EN_CHANGE(IDC_DBU_ALIAS, OnChangeDbuAlias)
	ON_EN_CHANGE(IDC_DBU_NAME, OnChangeDbuName)
	ON_EN_CHANGE(IDC_DBU_PASSWORD, OnChangeDbuPassword)
	ON_EN_CHANGE(IDC_DBU_CONFIRM, OnChangeDbuConfirm)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceDbUser message handlers

BOOL CxDlgIceDbUser::OnInitDialog() 
{
	CString csTitle;
	CDialog::OnInitDialog();

	m_ctrlAlias.LimitText(sizeof(m_pStructDbUser->UserAlias)-1);
	m_ctrlName.LimitText(sizeof(m_pStructDbUser->User_Name)-1);
	m_ctrlPassWord.LimitText(sizeof(m_pStructDbUser->Password)-1);
	m_ctrlConfirm.LimitText(sizeof(m_pStructDbUser->Password)-1);
	m_ctrlComment.LimitText(sizeof(m_pStructDbUser->Comment)-1);
	if (m_pStructDbUser->bAlter)
	{
		// "Alter Ice Database User %s on %s"
		csTitle.Format(IDS_ICE_ALTER_DATABASE_USER_TITLE,
			(LPCTSTR)m_pStructDbUser->UserAlias, 
			(LPCTSTR)m_csVirtNodeName);
		m_ctrlAlias.SetWindowText((char *)m_pStructDbUser->UserAlias);
		m_ctrlName.SetWindowText((char *)m_pStructDbUser->User_Name);
		m_ctrlAlias.EnableWindow(FALSE);
		m_ctrlPassWord.SetWindowText((char *)m_pStructDbUser->Password);
		m_ctrlConfirm.SetWindowText((char *)m_pStructDbUser->Password);
		m_ctrlComment.SetWindowText((char *)m_pStructDbUser->Comment);
		PushHelpId (HELPID_ALTER_IDD_ICE_DATABASE_USER);
	}
	else
	{
		CString csFormat;
		GetWindowText(csFormat);
		csTitle.Format(csFormat,m_csVirtNodeName);
		PushHelpId (HELPID_IDD_ICE_DATABASE_USER);
	}
	SetWindowText(csTitle);
	EnableDisableOK ();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceDbUser::OnOK() 
{
	CString csMsg;
	ICEDBUSERDATA StructDbUserNew;

	memset(&StructDbUserNew,0,sizeof(ICEDBUSERDATA));
	UpdateData(TRUE);

	if ( m_csPassword != m_csConfirm )
	{
		AfxMessageBox (IDS_E_PLEASE_RETYPE_PASSWORD);
		m_csPassword = _T("");
		m_csConfirm = _T("");
		UpdateData(FALSE);
		return;
	}

	if ( FillstructureFromCtrl(&StructDbUserNew) == FALSE )
		return;
	if (!m_pStructDbUser->bAlter)
	{
		if ( ICEAddobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName ,OT_ICE_DBUSER,&StructDbUserNew ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ADD_DATABASE_USER_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName,OT_ICE_DBUSER, m_pStructDbUser, &StructDbUserNew ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_DATABASE_USER_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	CDialog::OnOK();
}

BOOL CxDlgIceDbUser::FillstructureFromCtrl(LPICEDBUSERDATA pStrucDbUserNew)
{
	lstrcpy((LPSTR)pStrucDbUserNew->User_Name	,m_csName);
	lstrcpy((LPSTR)pStrucDbUserNew->UserAlias	,m_csAlias);
	lstrcpy((LPSTR)pStrucDbUserNew->Password	,m_csPassword);
	lstrcpy((LPSTR)pStrucDbUserNew->Comment		,m_csComment);
	return TRUE;
}

void CxDlgIceDbUser::EnableDisableOK ()
{
	if (m_ctrlAlias.LineLength() == 0 || 
		m_ctrlPassWord.LineLength() == 0 ||
		m_ctrlConfirm.LineLength() == 0 ||
		m_ctrlName.LineLength() == 0)
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);

}

void CxDlgIceDbUser::OnChangeDbuAlias() 
{
	EnableDisableOK ();
}

void CxDlgIceDbUser::OnChangeDbuName() 
{
	EnableDisableOK ();
}

void CxDlgIceDbUser::OnChangeDbuPassword() 
{
	EnableDisableOK ();
}

void CxDlgIceDbUser::OnChangeDbuConfirm() 
{
	EnableDisableOK ();
}

void CxDlgIceDbUser::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
