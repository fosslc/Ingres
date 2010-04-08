/******************************************************************************
//                                                                           //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.	     //
//                                                                           //
//    Source   : icebunro.cpp : implementation file                          //
//                                                                           //
//                                                                           //
//    Project  : Ingres II/Vdba.                                             //
//    Author   : Schalk Philippe                                             //
//                                                                           //
//    Purpose  : Popup Dialog Box for create Ice Business unit Security Role.//
******************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "icebunro.h"
#include "dgerrh.h"

extern "C"
{
#include "domdata.h"
#include "msghandl.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateIceBusinessRole( LPICEBUSUNITROLEDATA lpBuRole, int nHnode)
{
	CxDlgIceBusinessRole dlg;
	dlg.m_StructBunitRole = lpBuRole;
	dlg.m_nHnodeHandle    = nHnode;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBusinessRole dialog


CxDlgIceBusinessRole::CxDlgIceBusinessRole(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceBusinessRole::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceBusinessRole)
	m_bCreateDoc = FALSE;
	m_bExecDoc = FALSE;
	m_bReadDoc = FALSE;
	m_csRoleName = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceBusinessRole::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceBusinessRole)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_ROLE, m_cbRole);
	DDX_Check(pDX, IDC_CREATE_DOC, m_bCreateDoc);
	DDX_Check(pDX, IDC_EXEC_DOC, m_bExecDoc);
	DDX_Check(pDX, IDC_READ_DOC, m_bReadDoc);
	DDX_CBString(pDX, IDC_ROLE, m_csRoleName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceBusinessRole, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceBusinessRole)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_ROLE, OnSelchangeRole)
	ON_BN_CLICKED(IDC_CREATE_DOC, OnCreateDoc)
	ON_BN_CLICKED(IDC_EXEC_DOC, OnExecDoc)
	ON_BN_CLICKED(IDC_READ_DOC, OnReadDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBusinessRole message handlers

void CxDlgIceBusinessRole::OnOK() 
{
	CString csMsg;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (m_nHnodeHandle);
	ICEBUSUNITROLEDATA busunitrole;
	UpdateData(TRUE);
	busunitrole.bCreateDoc = m_bCreateDoc;
	busunitrole.bReadDoc   = m_bReadDoc;
	busunitrole.bExecDoc   = m_bExecDoc;
	lstrcpy((char *)busunitrole.icebusunit.Name,(char *)m_StructBunitRole->icebusunit.Name);
	lstrcpy((char *)busunitrole.icerole.RoleName,m_csRoleName);

	if (!m_StructBunitRole->bAlter)
	{
		if ( ICEAddobject(vnodeName ,OT_ICE_BUNIT_SEC_ROLE,&busunitrole ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ADD_BUNIT_SEC_ROLE_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject(vnodeName,
							OT_ICE_BUNIT_SEC_ROLE,
							m_StructBunitRole, &busunitrole) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_BUNIT_SEC_ROLE_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}

	CDialog::OnOK();
}

void CxDlgIceBusinessRole::EnableDisableOK()
{
	UpdateData(TRUE);
	if ( ( m_bCreateDoc || m_bReadDoc || m_bExecDoc ) &&
		 m_cbRole.GetCurSel() != CB_ERR)
		m_ctrlOK.EnableWindow(TRUE);
	else
		m_ctrlOK.EnableWindow(FALSE);
}

BOOL CxDlgIceBusinessRole::OnInitDialog() 
{
	CString csTempo,csTitle;
	CDialog::OnInitDialog();

	if (m_StructBunitRole->bAlter)
	{ //"Alter Role Access Definition %s for Business Unit %s"
		csTitle.Format( IDS_ICE_ALTER_BU_ROLE_TITLE,
						(char *)m_StructBunitRole->icerole.RoleName,
						(char *)m_StructBunitRole->icebusunit.Name);
		PushHelpId (HELPID_ALTER_IDD_ICE_BUSINESS_ROLE);
		m_csRoleName = m_StructBunitRole->icerole.RoleName;
		m_bExecDoc   = m_StructBunitRole->bExecDoc;
		m_bReadDoc   = m_StructBunitRole->bReadDoc;
		m_bCreateDoc = m_StructBunitRole->bCreateDoc;
		m_cbRole.EnableWindow(FALSE);
	}
	else
	{
		GetWindowText(csTempo);
		csTitle.Format(csTempo,(char *)m_StructBunitRole->icebusunit.Name);
		PushHelpId (HELPID_IDD_ICE_BUSINESS_ROLE);
	}
	SetWindowText(csTitle);
	FillRolesCombobox ();
	UpdateData(FALSE);
	EnableDisableOK();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceBusinessRole::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgIceBusinessRole::OnSelchangeRole() 
{
	EnableDisableOK();
}

void CxDlgIceBusinessRole::FillRolesCombobox ()
{
	CString csMsg,csType;
	int ires;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR bufOwner[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];

	m_cbRole.ResetContent();

	ires = DOMGetFirstObject (	m_nHnodeHandle,
								OT_ICE_ROLE, 0, NULL,
								FALSE, NULL, buf, bufOwner, extradata);
	while (ires == RES_SUCCESS)
	{
		m_cbRole.AddString ((char *)buf);
		ires  = DOMGetNextObject (buf, NULL, extradata);
	}

	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		csType.LoadString(IDS_E_ICE_FILL_COMBO_ROLE);
		MessageWithHistoryButton(m_hWnd,csType);
	}
}

void CxDlgIceBusinessRole::OnCreateDoc() 
{
	EnableDisableOK();
}

void CxDlgIceBusinessRole::OnExecDoc() 
{
	EnableDisableOK();
}

void CxDlgIceBusinessRole::OnReadDoc() 
{
	EnableDisableOK();
}
