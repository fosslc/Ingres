// icecocom.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "icecocom.h"
#include "dgerrh.h"

extern "C"
{
#include "ice.h"
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
extern "C" BOOL MfcDlgCommonCombobox( int nType ,char *szParentName ,int nHnode)
{
	CxDlgIceCommonCombo dlg;
	dlg.m_nCurrentObject = nType;
	dlg.m_nHnodeHandle = nHnode;
	dlg.m_csParentName = szParentName;

	switch (nType)
	{
	case OT_ICE_WEBUSER_CONNECTION:
		dlg.m_nID4FillCombo=OT_ICE_DBCONNECTION;
		dlg.m_nHelpId = HELPID_IDD_COMMON_CB_WU_CONNECTION;
		dlg.m_nParentObject = OT_ICE_WEBUSER;
		dlg.m_csStatic = _T("&DBConnection:");
		//"Associate DB connection to Web user %s"
		dlg.m_csTitle.Format(IDS_ICE_WEBUSER_CONNECTION_TITLE,szParentName);
		dlg.m_nMessageID = IDS_E_ICE_WEBUSER_CONNECTION_FAILED;
		dlg.m_nMessageErrorFillCombo = IDS_E_ICE_FILL_COMBO_DBCONNECTION;
		break;
	case OT_ICE_WEBUSER_ROLE:
		dlg.m_nID4FillCombo=OT_ICE_ROLE;
		dlg.m_nHelpId = HELPID_IDD_COMMON_CB_WU_ROLE;
		dlg.m_nParentObject = OT_ICE_WEBUSER;
		dlg.m_csStatic = _T("&Role:");
		//"Associate role to Web user %s"
		dlg.m_csTitle.Format(IDS_ICE_WEBUSER_ROLE_TITLE,szParentName);
		dlg.m_nMessageID = IDS_E_ICE_WEBUSER_ROLE_FAILED;
		dlg.m_nMessageErrorFillCombo = IDS_E_ICE_FILL_COMBO_ROLE;
		break;
	case OT_ICE_PROFILE_CONNECTION:
		dlg.m_nID4FillCombo=OT_ICE_DBCONNECTION;
		dlg.m_nHelpId = HELPID_IDD_COMMON_CB_PROF_CONNECTION;
		dlg.m_nParentObject = OT_ICE_PROFILE;
		dlg.m_csStatic = _T("&DBConnection:");
		//"Associate DB Connection to Ice Profile %s"
		dlg.m_csTitle.Format(IDS_ICE_PROFILE_CONNECTION_TITLE,szParentName);
		dlg.m_nMessageID = IDS_E_ICE_PROFILE_CONNECTION_FAILED;
		dlg.m_nMessageErrorFillCombo = IDS_E_ICE_FILL_COMBO_DBCONNECTION;
		break;
	case OT_ICE_PROFILE_ROLE:
		dlg.m_nID4FillCombo=OT_ICE_ROLE;
		dlg.m_nHelpId = HELPID_IDD_COMMON_CB_PROF_ROLE;
		dlg.m_nParentObject = OT_ICE_PROFILE;
		dlg.m_csStatic = _T("&Role:");
		//"Associate role to Ice Profile %s"
		dlg.m_csTitle.Format(IDS_ICE_PROFILE_ROLE_TITLE,szParentName);
		dlg.m_nMessageID = IDS_E_ICE_PROFILE_ROLE_FAILED;
		dlg.m_nMessageErrorFillCombo = IDS_E_ICE_FILL_COMBO_ROLE;
		break;
	case OT_ICE_BUNIT_LOCATION:
		dlg.m_nID4FillCombo=OT_ICE_SERVER_LOCATION;
		dlg.m_nHelpId = HELPID_IDD_COMMON_CB_LOCATION;
		dlg.m_nParentObject = OT_ICE_BUNIT;
		dlg.m_csStatic = _T("&Location:");
		//"Associate a location to Business Unit %s"
		dlg.m_csTitle.Format(IDS_ICE_BUNIT_LOCATION_TITLE,szParentName);
		dlg.m_nMessageID = IDS_E_ICE_BUNIT_LOCATION_FAILED;
		dlg.m_nMessageErrorFillCombo = IDS_E_ICE_FILL_COMBO_SERVER_LOCATION;
		break;
	default:
		ASSERT(FALSE);
		return FALSE;
	}
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceCommonCombo dialog


CxDlgIceCommonCombo::CxDlgIceCommonCombo(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceCommonCombo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceCommonCombo)
	m_csStatic = _T("");
	m_csCommon = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceCommonCombo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceCommonCombo)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_COMMON_STATIC, m_ctrlStatic);
	DDX_Control(pDX, IDC_COMMON_COMBOBOX, m_ctrlCommon);
	DDX_Text(pDX, IDC_COMMON_STATIC, m_csStatic);
	DDX_CBString(pDX, IDC_COMMON_COMBOBOX, m_csCommon);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceCommonCombo, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceCommonCombo)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMMON_COMBOBOX, OnSelchangeCommonCombobox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceCommonCombo message handlers

void CxDlgIceCommonCombo::OnOK() 
{
	ICEBUSUNITLOCATIONDATA Il;
	ICEPROFILEROLEDATA Ip;
	ICEPROFILECONNECTIONDATA Ic;
	ICEWEBUSERROLEDATA Ir;
	ICEWEBUSERCONNECTIONDATA Iw;
	LPVOID pStruct = NULL;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (m_nHnodeHandle);
	UpdateData(TRUE);

	switch (m_nCurrentObject)
	{
	case OT_ICE_WEBUSER_CONNECTION:
		memset(&Iw,0,sizeof(ICEWEBUSERCONNECTIONDATA));
		lstrcpy((LPSTR)Iw.icedbconnection.ConnectionName,m_csCommon);
		lstrcpy((LPSTR)Iw.icewebuser.UserName,(LPSTR)(LPCTSTR)m_csParentName);
		pStruct = &Iw;
		break;
	case OT_ICE_WEBUSER_ROLE:
		memset(&Ir,0,sizeof(ICEWEBUSERROLEDATA));
		lstrcpy((LPSTR)Ir.icerole.RoleName,m_csCommon);
		lstrcpy((LPSTR)Ir.icewebuser.UserName,(LPSTR)(LPCTSTR)m_csParentName);
		pStruct = &Ir;
		break;
	case OT_ICE_PROFILE_CONNECTION:
		memset(&Ic,0,sizeof(ICEPROFILECONNECTIONDATA));
		lstrcpy((LPSTR)Ic.icedbconnection.ConnectionName,m_csCommon);
		lstrcpy((LPSTR)Ic.iceprofile.ProfileName,(LPSTR)(LPCTSTR)m_csParentName);
		pStruct = &Ic;
		break;
	case OT_ICE_PROFILE_ROLE:
		memset(&Ip,0,sizeof(ICEPROFILEROLEDATA));
		lstrcpy((LPSTR)Ip.icerole.RoleName,m_csCommon);
		lstrcpy((LPSTR)Ip.iceprofile.ProfileName,(LPSTR)(LPCTSTR)m_csParentName);
		pStruct = &Ip;
		break;
	case OT_ICE_BUNIT_LOCATION:
		memset(&Il,0,sizeof(ICEBUSUNITLOCATIONDATA));
		lstrcpy((LPSTR)Il.icelocation.LocationName,m_csCommon);
		lstrcpy((LPSTR)Il.icebusunit.Name,(LPSTR)(LPCTSTR)m_csParentName);
		pStruct = &Il;
		break;
	default:
		ASSERT(FALSE);
		return;
	}

	if ( ICEAddobject(vnodeName ,m_nCurrentObject,pStruct ) == RES_ERR )
	{
		CString csMsg;
		csMsg.LoadString(m_nMessageID);
		MessageWithHistoryButton(m_hWnd,csMsg);
		return;
	}

	CDialog::OnOK();
}

BOOL CxDlgIceCommonCombo::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SetWindowText(m_csTitle);
	FillComboBox();
	EnableDisableOK();
	PushHelpId (m_nHelpId);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceCommonCombo::OnDestroy() 
{
	CDialog::OnDestroy();

	PopHelpId();

}
void CxDlgIceCommonCombo::FillComboBox()
{
	int ires,nIndex;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR bufOwner[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];
	CString csCurrentMsg;

	m_ctrlCommon.ResetContent();

	ires = DOMGetFirstObject (	m_nHnodeHandle,
								m_nID4FillCombo, 0, NULL,
								FALSE, NULL, buf, bufOwner, extradata);
	while (ires == RES_SUCCESS)
	{
		nIndex = m_ctrlCommon.AddString ((char *)buf);
		if ( nIndex == CB_ERR || nIndex == CB_ERRSPACE )
			return;
		ires  = DOMGetNextObject (buf, NULL, extradata);
	}

	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		csCurrentMsg.LoadString(m_nMessageErrorFillCombo);
		MessageWithHistoryButton(m_hWnd,csCurrentMsg);
	}
	m_ctrlCommon.SetCurSel(0);
}

void CxDlgIceCommonCombo::OnSelchangeCommonCombobox() 
{
	EnableDisableOK();
}

void CxDlgIceCommonCombo::EnableDisableOK()
{
	if (m_ctrlCommon.GetCurSel() == CB_ERR)
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}