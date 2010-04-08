/******************************************************************************
**                                                                           
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**                                                                           
**    Source   : IceProfi.cpp : implementation file                          
**                                                                           
**                                                                           
**    Project  : Ingres II/Vdba.                                             
**    Author   : Schalk Philippe                                             
**                                                                           
**    Purpose  : Popup Dialog Box for create Ice Profile.                    
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
******************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "iceprofi.h"
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
extern "C" BOOL MfcDlgCreateIceProfile( LPICEPROFILEDATA lpcreateIceProf, int nHnode )
{
	CxDlgIceProfile dlg;
	dlg.m_pStructProfileInfo = lpcreateIceProf;
	dlg.m_csVnodeName = GetVirtNodeName (nHnode);
	dlg.m_nHnodeHandle = nHnode;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceProfile dialog


CxDlgIceProfile::CxDlgIceProfile(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceProfile::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceProfile)
	m_lTimeout = 0;
	m_bAdmin = FALSE;
	m_bMonitoring = FALSE;
	m_csName = _T("");
	m_bSecurityAdmin = FALSE;
	m_bUnitManager = FALSE;
	m_csDefUser = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceProfile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceProfile)
	DDX_Control(pDX, IDC_CP_TIMEOUT, m_ctrledTimeOut);
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_CP_NAME, m_ctrledName);
	DDX_Control(pDX, IDC_CP_DEF_DB_USER, m_cbDbUser);
	DDX_Text(pDX, IDC_CP_TIMEOUT, m_lTimeout);
	DDV_MinMaxLong(pDX, m_lTimeout, 0, 2147483646);
	DDX_Check(pDX, IDC_CP_ADMIN, m_bAdmin);
	DDX_Check(pDX, IDC_CP_MONITORING, m_bMonitoring);
	DDX_Text(pDX, IDC_CP_NAME, m_csName);
	DDX_Check(pDX, IDC_CP_SECUR_ADMIN, m_bSecurityAdmin);
	DDX_Check(pDX, IDC_CP_UNIT_MANAGER, m_bUnitManager);
	DDX_CBString(pDX, IDC_CP_DEF_DB_USER, m_csDefUser);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceProfile, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceProfile)
	ON_EN_CHANGE(IDC_CP_NAME, OnChangeCpName)
	ON_CBN_SELCHANGE(IDC_CP_DEF_DB_USER, OnSelchangeCpDefDbUser)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceProfile message handlers

BOOL CxDlgIceProfile::OnInitDialog() 
{
	CString csTempo,csTitle;
	CDialog::OnInitDialog();

	if (m_pStructProfileInfo->bAlter)
	{	//"Alter Ice Profile %s on %s"
		csTitle.Format(IDS_ICE_ALTER_PROFILE_TITLE,
		(LPCTSTR)m_pStructProfileInfo->ProfileName, 
		(LPCTSTR)m_csVnodeName);
		m_ctrledName.EnableWindow(FALSE);
		PushHelpId (HELPID_ALTER_IDD_ICE_PROFILE);
	}
	else
	{
		GetWindowText(csTempo);
		csTitle.Format(csTempo,m_csVnodeName);
		PushHelpId (HELPID_IDD_ICE_PROFILE);
	}
	SetWindowText(csTitle);
	FillDatabasesUsers();
	m_bAdmin			=	m_pStructProfileInfo->bAdminPriv; 
	m_bSecurityAdmin	=	m_pStructProfileInfo->bSecurityPriv;
	m_bUnitManager		=	m_pStructProfileInfo->bUnitMgrPriv;
	m_bMonitoring		=	m_pStructProfileInfo->bMonitorPriv;
	m_lTimeout			=	m_pStructProfileInfo->ltimeoutms;
	m_csName			=	m_pStructProfileInfo->ProfileName;
	m_csDefUser			=	m_pStructProfileInfo->DefDBUsr.UserAlias;
	UpdateData(FALSE);
	char buffer[20];
	_ltot(LONG_MAX,buffer,10);
	m_ctrledTimeOut.LimitText(lstrlen(buffer));
	m_ctrledName.LimitText(sizeof(m_pStructProfileInfo->ProfileName)-1);

	EnableDisableOK ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceProfile::OnChangeCpName() 
{
	EnableDisableOK ();
}

void CxDlgIceProfile::OnSelchangeCpDefDbUser() 
{
	EnableDisableOK ();
}

void CxDlgIceProfile::OnOK() 
{
	CString csMsg;
	ICEPROFILEDATA ProfileInfoNew;

	memset(&ProfileInfoNew,0,sizeof(ICEPROFILEDATA));

	if (!UpdateData(TRUE)) // Verify validation for m_lTimeOut value.
		return;

	if ( FillstructureFromCtrl(&ProfileInfoNew) == FALSE)
		return;

	if (!m_pStructProfileInfo->bAlter)
	{
		if (ICEAddobject((LPUCHAR)(LPCTSTR)m_csVnodeName ,OT_ICE_PROFILE,&ProfileInfoNew ) == RES_ERR)
		{
			csMsg.LoadString(IDS_E_ICE_ADD_PROFILE);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csVnodeName,OT_ICE_PROFILE, m_pStructProfileInfo, &ProfileInfoNew ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_PROFILE);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}

	}
	CDialog::OnOK();
}

void CxDlgIceProfile::EnableDisableOK ()
{
	if (m_ctrledName.LineLength() == 0 ||
		m_cbDbUser.GetCurSel()  == CB_ERR )
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}
void CxDlgIceProfile::FillDatabasesUsers ()
{
	int ires;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR bufOwner[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];
	CString csMsg;

	m_cbDbUser.ResetContent();

	ires = DOMGetFirstObject (	m_nHnodeHandle,
								OT_ICE_DBUSER, 0, NULL,
								FALSE, NULL, buf, bufOwner, extradata);
	while (ires == RES_SUCCESS)
	{
		m_cbDbUser.AddString ((char *)buf);
		ires  = DOMGetNextObject (buf, NULL, extradata);
	}

	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		csMsg.LoadString(IDS_E_ICE_FILL_COMBO_DB_USER);
		MessageWithHistoryButton(m_hWnd,csMsg);
	}
}

BOOL CxDlgIceProfile::FillstructureFromCtrl( LPICEPROFILEDATA pProfileDta )
{

	if (!m_csName.IsEmpty())
		lstrcpy((char *)pProfileDta->ProfileName,(LPTSTR)(LPCTSTR)m_csName);
	else
		return FALSE;
	if (!m_csDefUser.IsEmpty())
		lstrcpy((char *)pProfileDta->DefDBUsr.UserAlias,(LPTSTR)(LPCTSTR)m_csDefUser);
	else
		return FALSE;

	pProfileDta->bAdminPriv		= m_bAdmin;
	pProfileDta->bSecurityPriv	= m_bSecurityAdmin;
	pProfileDta->bUnitMgrPriv	= m_bUnitManager;
	pProfileDta->bMonitorPriv	= m_bMonitoring;
	pProfileDta->ltimeoutms		= m_lTimeout;

	return TRUE;
}

void CxDlgIceProfile::OnDestroy() 
{
	CDialog::OnDestroy();

	PopHelpId();
}
