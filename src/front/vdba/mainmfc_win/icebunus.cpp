/******************************************************************************
**                                                                           
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**                                                                           
**    Source   :  icebunus.cpp : implementation file                         
**                                                                           
**                                                                           
**    Project  : Ingres II/Vdba.                                             
**    Author   : Schalk Philippe                                             
**                                                                           
**    Purpose  : Popup Dialog Box for create Ice Business unit Security User.
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
******************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "icebunus.h"
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
extern "C" BOOL MfcDlgCreateIceBusinessUser( LPICEBUSUNITWEBUSERDATA lpBusUnitUser ,int nHnode)
{
	CxDlgIceBusinessUser dlg;
	dlg.m_lpStructBunitUser = lpBusUnitUser;
	dlg.m_nHnodeHandle = nHnode;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBusinessUser dialog


CxDlgIceBusinessUser::CxDlgIceBusinessUser(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceBusinessUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceBusinessUser)
	m_bCreateDoc = FALSE;
	m_bReadDoc = FALSE;
	m_bUnitRead = FALSE;
	m_csWebUser = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceBusinessUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceBusinessUser)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_WEB_USER, m_ctrlWebUser);
	DDX_Check(pDX, IDC_USER_CREATE_DOC, m_bCreateDoc);
	DDX_Check(pDX, IDC_USER_READ_DOC, m_bReadDoc);
	DDX_Check(pDX, IDC_USER_UNIT_READ, m_bUnitRead);
	DDX_CBString(pDX, IDC_WEB_USER, m_csWebUser);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceBusinessUser, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceBusinessUser)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_WEB_USER, OnSelchangeWebUser)
	ON_BN_CLICKED(IDC_USER_CREATE_DOC, OnUserCreateDoc)
	ON_BN_CLICKED(IDC_USER_READ_DOC, OnUserReadDoc)
	ON_BN_CLICKED(IDC_USER_UNIT_READ, OnUserUnitRead)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBusinessUser message handlers

BOOL CxDlgIceBusinessUser::OnInitDialog()
{
	CString csTempo,csTitle;
	CDialog::OnInitDialog();
	if (m_lpStructBunitUser->bAlter)
	{
		//"Alter User Access Definition ""for Business Unit %s"
		csTitle.Format(IDS_ICE_ALTER_BU_USER_TITLE,
			(LPCTSTR)m_lpStructBunitUser->icewevuser.UserName,
			(LPCTSTR)m_lpStructBunitUser->icebusunit.Name);
		PushHelpId (HELPID_ALTER_IDD_ICE_BUSINESS_USER);
		m_csWebUser  = m_lpStructBunitUser->icewevuser.UserName;
		m_bUnitRead  = m_lpStructBunitUser->bUnitRead;
		m_bReadDoc   = m_lpStructBunitUser->bReadDoc;
		m_bCreateDoc = m_lpStructBunitUser->bCreateDoc;
		m_ctrlWebUser.EnableWindow(FALSE);
	}
	else
	{
		GetWindowText(csTempo);
		csTitle.Format(csTempo,m_lpStructBunitUser->icebusunit.Name);
		PushHelpId (HELPID_IDD_ICE_BUSINESS_USER);
	}
	SetWindowText(csTitle);
	FillUserCombobox();
	UpdateData(FALSE);
	EnableDisableOK();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
void CxDlgIceBusinessUser::EnableDisableOK()
{
	UpdateData(TRUE);
	if (( m_bUnitRead || m_bReadDoc || m_bCreateDoc ) &&
		 m_ctrlWebUser.GetCurSel() != CB_ERR )
		m_ctrlOK.EnableWindow(TRUE);
	else
		m_ctrlOK.EnableWindow(FALSE);
}

void CxDlgIceBusinessUser::OnOK() 
{
	CString csMsg;
	ICEBUSUNITWEBUSERDATA BusUnitUser;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (m_nHnodeHandle);
	UpdateData(TRUE);

	BusUnitUser.bCreateDoc = m_bCreateDoc;
	BusUnitUser.bUnitRead  = m_bUnitRead;
	BusUnitUser.bReadDoc   = m_bReadDoc;
	lstrcpy((char *)BusUnitUser.icebusunit.Name,(char *)m_lpStructBunitUser->icebusunit.Name);
	lstrcpy((char *)BusUnitUser.icewevuser.UserName,m_csWebUser);
	if (m_lpStructBunitUser->bAlter)
	{
		if ( ICEAlterobject(vnodeName,
							OT_ICE_BUNIT_SEC_USER,
							m_lpStructBunitUser, &BusUnitUser) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_BUNIT_SEC_USER_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAddobject(vnodeName ,OT_ICE_BUNIT_SEC_USER,&BusUnitUser ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ADD_BUNIT_SEC_USER_FAILED);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}

	}
	CDialog::OnOK();
}

void CxDlgIceBusinessUser::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgIceBusinessUser::OnSelchangeWebUser()
{
	EnableDisableOK();
}

void CxDlgIceBusinessUser::FillUserCombobox()
{
	int ires;
	CString csType;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR bufOwner[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];

	m_ctrlWebUser.ResetContent();

	ires = DOMGetFirstObject (	m_nHnodeHandle,
								OT_ICE_WEBUSER, 0, NULL,
								FALSE, NULL, buf, bufOwner, extradata);
	while (ires == RES_SUCCESS)
	{
		m_ctrlWebUser.AddString ((char *)buf);
		ires  = DOMGetNextObject (buf, NULL, extradata);
	}

	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		csType.LoadString(IDS_E_ICE_FILL_COMBO_WEB_USER);
		MessageWithHistoryButton(m_hWnd,csType);
	}
}

void CxDlgIceBusinessUser::OnUserCreateDoc() 
{
	EnableDisableOK();
}

void CxDlgIceBusinessUser::OnUserReadDoc() 
{
	EnableDisableOK();
}

void CxDlgIceBusinessUser::OnUserUnitRead() 
{
	EnableDisableOK();
}
