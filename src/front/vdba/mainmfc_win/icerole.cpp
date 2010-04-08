/******************************************************************************
**                                                                           
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**                                                                           
**    Source   : icerole.cpp : implementation file                           
**                                                                           
**                                                                           
**    Project  : Ingres II/Vdba.                                             
**    Author   : Schalk Philippe                                             
**                                                                           
**    Purpose  : Popup Dialog Box for create Ice Role. 
** 
** History:
**
** 19-Mar-2009 (drivi01)
**    Add a return type for MfcDlgCreateIceRole to clean up warnings.                      
******************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "icerole.h"
#include "dgerrh.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateIceRole ( LPICEROLEDATA lpcreateIRole , int nHnode)
{
	CxDlgIceRole dlg;
	ASSERT (lpcreateIRole);
	if (!lpcreateIRole)
		return FALSE;
	dlg.m_pStructRoleInfo = lpcreateIRole;
	dlg.m_csVirtNodeName = GetVirtNodeName (nHnode);
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceRole dialog


CxDlgIceRole::CxDlgIceRole(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceRole::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceRole)
	//}}AFX_DATA_INIT
}


void CxDlgIceRole::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceRole)
	DDX_Control(pDX, IDC_ROLE_COMMENT, m_edRoleComment);
	DDX_Control(pDX, IDC_ICE_ROLE_NAME, m_ctrlIceRoleName);
	DDX_Control(pDX, IDOK, m_ctrlOK);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceRole, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceRole)
	ON_EN_CHANGE(IDC_ICE_ROLE_NAME, OnChangeIceRoleName)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceRole message handlers

BOOL CxDlgIceRole::OnInitDialog() 
{
	CString csTitle;
	CDialog::OnInitDialog();
	m_ctrlIceRoleName.LimitText(sizeof(m_pStructRoleInfo->RoleName)-1);
	m_edRoleComment.LimitText(sizeof(m_pStructRoleInfo->Comment)-1);
	if (m_pStructRoleInfo->bAlter)
	{	//"Alter Ice Role %s on %s"
		csTitle.Format(IDS_ICE_ALTER_ROLE_TITLE,
			(LPCTSTR)m_pStructRoleInfo->RoleName, 
			(LPCTSTR)m_csVirtNodeName);
		m_ctrlIceRoleName.SetWindowText((char *)m_pStructRoleInfo->RoleName);
		m_ctrlIceRoleName.EnableWindow(FALSE);
		m_edRoleComment.SetWindowText((char *)m_pStructRoleInfo->Comment);
		PushHelpId (HELPID_ALTER_IDD_ICE_ROLE);
	}
	else
	{
		CString csFormat;
		GetWindowText(csFormat);
		csTitle.Format(csFormat,m_csVirtNodeName);
		PushHelpId (HELPID_IDD_ICE_ROLE);
	}
	SetWindowText(csTitle);
	EnableDisableOKbutton();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceRole::OnChangeIceRoleName() 
{
	EnableDisableOKbutton();
}
void CxDlgIceRole::EnableDisableOKbutton()
{
	if (m_ctrlIceRoleName.LineLength() == 0)
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}

void CxDlgIceRole::OnOK() 
{
	CString csMsg;
	ICEROLEDATA StructRoleInfo;
	memset(&StructRoleInfo,0,sizeof(StructRoleInfo));

	m_ctrlIceRoleName.GetWindowText((char *)StructRoleInfo.RoleName,
									sizeof(StructRoleInfo.RoleName));
	m_edRoleComment.GetWindowText(	(char *)StructRoleInfo.Comment,
									sizeof(StructRoleInfo.Comment));
	if (!m_pStructRoleInfo->bAlter)
	{
		if (ICEAddobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName ,OT_ICE_ROLE,&StructRoleInfo ) == RES_ERR)
		{
			csMsg.LoadString(IDS_E_ICE_ADD_ROLE);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName,OT_ICE_ROLE, m_pStructRoleInfo, &StructRoleInfo ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_ROLE);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	CDialog::OnOK();
}

void CxDlgIceRole::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
