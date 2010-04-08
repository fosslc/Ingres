/******************************************************************************
**                                                                           
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**                                                                           
**    Source   : IceWebU.cpp : implementation file                           
**                                                                           
**                                                                           
**    Project  : Ingres II/Vdba.                                             
**    Author   : Schalk Philippe                                             
**                                                                           
**    Purpose  : Popup Dialog Box for create Ice Web User.                   
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
******************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "icewebu.h"
#include "dgerrh.h"

extern "C"
{
#include "msghandl.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateIceWebUser( LPICEWEBUSERDATA lpcreateWebUser, int nHnode )
{
	CxDlgIceWebUser dlg;
	dlg.m_pStructWebUsrInfo = lpcreateWebUser;
	dlg.m_csVnodeName =  GetVirtNodeName (nHnode);
	dlg.m_nHnodeHandle = nHnode;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceWebUser dialog


CxDlgIceWebUser::CxDlgIceWebUser(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceWebUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceWebUser)
	m_lTimeOut = 0;
	m_bSecurAdmin = FALSE;
	m_bUnitManager = FALSE;
	m_bMonitoring = FALSE;
	m_bAdmin = FALSE;
	m_csDefUser = _T("");
	m_csPassword = _T("");
	m_csConfirmPassword = _T("");
	m_csUserName = _T("");
	m_csComment = _T("");
	m_nTypePassword = -1;
	//}}AFX_DATA_INIT
}


void CxDlgIceWebUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceWebUser)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_ICE_USR_NAME, m_ctrledUserName);
	DDX_Control(pDX, IDC_ICE_TIMEOUT, m_ctrledTimeOut);
	DDX_Control(pDX, IDC_ICE_DEF_DB_USER, m_ctrlcbUser);
	DDX_Control(pDX, IDC_ICE_CWU_PASSWORD, m_ctrledPassword);
	DDX_Control(pDX, IDC_ICE_CWU_CONFIRM, m_ctrledConfirm);
	DDX_Control(pDX, IDC_ICE_CWU_COMMENT, m_ctrledComment);
	DDX_Text(pDX, IDC_ICE_TIMEOUT, m_lTimeOut);
	DDV_MinMaxLong(pDX, m_lTimeOut, 0, 2147483646);
	DDX_Check(pDX, IDC_SECUR_ADMIN, m_bSecurAdmin);
	DDX_Check(pDX, IDC_ICE_UNIT_MANAGER, m_bUnitManager);
	DDX_Check(pDX, IDC_ICE_MONITORING, m_bMonitoring);
	DDX_Check(pDX, IDC_ICE_ADMIN, m_bAdmin);
	DDX_CBString(pDX, IDC_ICE_DEF_DB_USER, m_csDefUser);
	DDX_Text(pDX, IDC_ICE_CWU_PASSWORD, m_csPassword);
	DDX_Text(pDX, IDC_ICE_CWU_CONFIRM, m_csConfirmPassword);
	DDX_Text(pDX, IDC_ICE_USR_NAME, m_csUserName);
	DDX_Text(pDX, IDC_ICE_CWU_COMMENT, m_csComment);
	DDX_Radio(pDX, IDC_RADIO_REPOSITORY, m_nTypePassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceWebUser, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceWebUser)
	ON_EN_CHANGE(IDC_ICE_CWU_CONFIRM, OnChangeIceCwuConfirm)
	ON_EN_CHANGE(IDC_ICE_CWU_PASSWORD, OnChangeIceCwuPassword)
	ON_CBN_SELCHANGE(IDC_ICE_DEF_DB_USER, OnSelchangeIceDefDbUser)
	ON_EN_CHANGE(IDC_ICE_USR_NAME, OnChangeIceUsrName)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_RADIO_OPERASYSTEM, OnRadioOperasystem)
	ON_BN_CLICKED(IDC_RADIO_REPOSITORY, OnRadioRepository)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceWebUser message handlers

void CxDlgIceWebUser::OnChangeIceCwuConfirm() 
{
	EnableDisableOK ();
}

void CxDlgIceWebUser::OnChangeIceCwuPassword() 
{
	EnableDisableOK ();
}

void CxDlgIceWebUser::OnSelchangeIceDefDbUser() 
{
	EnableDisableOK ();
}

void CxDlgIceWebUser::OnChangeIceUsrName() 
{
	EnableDisableOK ();
}

void CxDlgIceWebUser::OnOK() 
{
	CString csMsg;
	ICEWEBUSERDATA StructWebUsrInfoNew;
	memset(&StructWebUsrInfoNew,0,sizeof(ICEWEBUSERDATA));

	if (!UpdateData(TRUE)) // Verify validation for m_lTimeOut value.
		return;

	if (m_csConfirmPassword != m_csPassword )
	{
		AfxMessageBox (IDS_E_PLEASE_RETYPE_PASSWORD);
		m_csConfirmPassword = _T("");
		m_csPassword = _T("");
		UpdateData(FALSE);
		return;
	}

	if ( FillstructureFromCtrl(&StructWebUsrInfoNew) == FALSE)
		return;

	if (!m_pStructWebUsrInfo->bAlter)
	{
		if (ICEAddobject((LPUCHAR)(LPCTSTR)m_csVnodeName ,OT_ICE_WEBUSER, &StructWebUsrInfoNew) == RES_ERR)
		{
			csMsg.LoadString(IDS_E_ICE_ADD_WEB_USER);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csVnodeName,OT_ICE_WEBUSER,m_pStructWebUsrInfo , &StructWebUsrInfoNew ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_WEB_USER);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	CDialog::OnOK();
}

void CxDlgIceWebUser::EnableDisableOK ()
{
	if (GetCheckedRadioButton( IDC_RADIO_REPOSITORY, IDC_RADIO_OPERASYSTEM ) == IDC_RADIO_REPOSITORY)
	{
		if (m_ctrledUserName.LineLength()    == 0 ||
			m_ctrlcbUser.GetCurSel()   == CB_ERR ||
			m_ctrledPassword.LineLength()     == 0 ||
			m_ctrledConfirm.LineLength() == 0 )
			m_ctrlOK.EnableWindow(FALSE);
		else
			m_ctrlOK.EnableWindow(TRUE);
	}
	else
	{
		if (m_ctrledUserName.LineLength()    == 0 ||
			m_ctrlcbUser.GetCurSel()   == CB_ERR )
			m_ctrlOK.EnableWindow(FALSE);
		else
			m_ctrlOK.EnableWindow(TRUE);
	}
}

BOOL CxDlgIceWebUser::OnInitDialog()
{
	CString csTempo,csTitle;
	CDialog::OnInitDialog();
	if (m_pStructWebUsrInfo->bAlter)
	{
		//"Alter Ice Web User %s on %s"
		csTitle.Format(IDS_ICE_ALTER_WEB_USER_TITLE,
		(LPCTSTR)m_pStructWebUsrInfo->UserName, 
		(LPCTSTR)m_csVnodeName);
		PushHelpId (HELPID_ALTER_IDD_ICE_WEB_USER);
	}
	else
	{
		GetWindowText(csTempo);
		csTitle.Format(csTempo,m_csVnodeName);
		PushHelpId (HELPID_IDD_ICE_WEB_USER);
	}
	SetWindowText(csTitle);

	FillDatabasesUsers();
	m_bAdmin			= m_pStructWebUsrInfo->bAdminPriv;
	m_bSecurAdmin		= m_pStructWebUsrInfo->bSecurityPriv;
	m_bUnitManager		= m_pStructWebUsrInfo->bUnitMgrPriv;
	m_bMonitoring		= m_pStructWebUsrInfo->bMonitorPriv;
	m_csDefUser			= m_pStructWebUsrInfo->DefDBUsr.UserAlias;
	m_csUserName		= m_pStructWebUsrInfo->UserName;
	m_csPassword		= m_pStructWebUsrInfo->Password;
	m_csConfirmPassword = m_pStructWebUsrInfo->Password;
	m_csComment			= m_pStructWebUsrInfo->Comment;
	m_lTimeOut			= m_pStructWebUsrInfo->ltimeoutms;
	if (m_pStructWebUsrInfo->bAlter)
		m_nTypePassword		= (m_pStructWebUsrInfo->bICEpwd? 0:1);   // FALSE: OS password. TRUE: ICE (repository) password;
	else
		m_nTypePassword=0;
	UpdateData(FALSE);

	m_ctrledUserName.LimitText(sizeof(m_pStructWebUsrInfo->UserName)-1);
	m_ctrledPassword.LimitText(sizeof(m_pStructWebUsrInfo->Password)-1);
	m_ctrledConfirm.LimitText(sizeof(m_pStructWebUsrInfo->Password)-1);
	m_ctrledComment.LimitText(sizeof(m_pStructWebUsrInfo->Comment)-1);
	char buffer[20];
	_ltot(LONG_MAX,buffer,10);
	m_ctrledTimeOut.LimitText(lstrlen(buffer));
	if (m_pStructWebUsrInfo->bAlter)
	{
		m_ctrledUserName.EnableWindow(FALSE);
	}
	EnableDisablePassword();
	EnableDisableOK ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CxDlgIceWebUser::FillstructureFromCtrl( LPICEWEBUSERDATA pWebUserNew)
{
	pWebUserNew->ltimeoutms		= m_lTimeOut;
	pWebUserNew->bAdminPriv		= m_bAdmin;
	pWebUserNew->bSecurityPriv	= m_bSecurAdmin;
	pWebUserNew->bUnitMgrPriv	= m_bUnitManager;
	pWebUserNew->bMonitorPriv	= m_bMonitoring;
	lstrcpy((char *)pWebUserNew->UserName,(LPTSTR)(LPCTSTR)m_csUserName);
	lstrcpy((char *)pWebUserNew->Comment,(LPTSTR)(LPCTSTR)m_csComment);
	lstrcpy((char *)pWebUserNew->Password,(LPTSTR)(LPCTSTR)m_csPassword );
	lstrcpy((char *)pWebUserNew->DefDBUsr.UserAlias,(LPTSTR)(LPCTSTR)m_csDefUser);
	if (m_nTypePassword)
		pWebUserNew->bICEpwd = FALSE;
	else
		pWebUserNew->bICEpwd = TRUE;

	return TRUE;
}
void CxDlgIceWebUser::FillDatabasesUsers ()
{
	CString csMsg;
	int ires;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR bufOwner[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];

	m_ctrlcbUser.ResetContent();

	ires = DOMGetFirstObject (	m_nHnodeHandle,
								OT_ICE_DBUSER, 0, NULL,
								FALSE, NULL, buf, bufOwner, extradata);
	while (ires == RES_SUCCESS)
	{
		m_ctrlcbUser.AddString ((char *)buf);
		ires  = DOMGetNextObject (buf, NULL, extradata);
	}

	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		csMsg.LoadString(IDS_E_ICE_FILL_COMBO_DB_USER);
		MessageWithHistoryButton(m_hWnd,csMsg);
	}
}

void CxDlgIceWebUser::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
void CxDlgIceWebUser::EnableDisablePassword()
{
	if (GetCheckedRadioButton( IDC_RADIO_REPOSITORY, IDC_RADIO_OPERASYSTEM ) == IDC_RADIO_OPERASYSTEM)
	{
		GetDlgItem(IDC_STATIC_PWD)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_CONF_PWD)->ShowWindow(SW_HIDE);
		m_ctrledPassword.ShowWindow(SW_HIDE);
		m_ctrledConfirm.ShowWindow(SW_HIDE);
	}
	else
	{
		GetDlgItem(IDC_STATIC_PWD)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_CONF_PWD)->ShowWindow(SW_SHOW);
		m_ctrledPassword.ShowWindow(SW_SHOW);
		m_ctrledConfirm.ShowWindow(SW_SHOW);
	}
}

void CxDlgIceWebUser::OnRadioOperasystem() 
{
	EnableDisablePassword();
	EnableDisableOK ();
}

void CxDlgIceWebUser::OnRadioRepository() 
{
	EnableDisablePassword();
	EnableDisableOK ();
}
