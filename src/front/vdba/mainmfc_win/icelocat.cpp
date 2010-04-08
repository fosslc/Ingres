/******************************************************************************
**                                                                           
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**                                                                           
**    Source   : icelocat.cpp : implementation file                          
**                                                                           
**                                                                           
**    Project  : Ingres II/Vdba.                                             
**    Author   : Schalk Philippe                                             
**                                                                           
**    Purpose  : Popup Dialog Box for create Ice Server Location.            
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
******************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "icelocat.h"
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
extern "C" BOOL MfcDlgIceServerLocation( LPICELOCATIONDATA lpcreateIceLoc ,int nHnode)
{
	CxDlgIceServerLocation dlg;
	dlg.m_pStructLocationInfo = lpcreateIceLoc;
	dlg.m_csVnodeName = GetVirtNodeName (nHnode);
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CxDlgIceServerLocation dialog


CxDlgIceServerLocation::CxDlgIceServerLocation(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceServerLocation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceServerLocation)
	m_csName = _T("");
	m_bPublic = FALSE;
	m_csExtension = _T("");
	m_csPath = _T("");
	m_nType = -1;
	//}}AFX_DATA_INIT
}


void CxDlgIceServerLocation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceServerLocation)
	DDX_Control(pDX, IDC_EXTENSION_LOCATION, m_ctrledExtension);
	DDX_Control(pDX, IDC_PATH_LOCATION, m_ctrledPath);
	DDX_Control(pDX, IDC_NAME_LOCATION, m_ctrledNameLoc);
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Text(pDX, IDC_NAME_LOCATION, m_csName);
	DDX_Check(pDX, IDC_ICE_PUBLIC, m_bPublic);
	DDX_Text(pDX, IDC_EXTENSION_LOCATION, m_csExtension);
	DDX_Text(pDX, IDC_PATH_LOCATION, m_csPath);
	DDX_Radio(pDX, IDC_HTTP, m_nType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceServerLocation, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceServerLocation)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_NAME_LOCATION, OnChangeNameLocation)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceServerLocation message handlers

void CxDlgIceServerLocation::OnDestroy() 
{
	CDialog::OnDestroy();

	PopHelpId();
}

BOOL CxDlgIceServerLocation::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CString csTempo,csTitle;
	if (m_pStructLocationInfo->bAlter)
	{
		m_csName      = m_pStructLocationInfo->LocationName;
		m_bPublic     = m_pStructLocationInfo->bPublic;
		m_nType       = (m_pStructLocationInfo->bIce? 1:0);
		m_csPath      = m_pStructLocationInfo->path;
		m_csExtension = m_pStructLocationInfo->extensions;
		//m_csComment   = m_pStructLocationInfo->Comment;
		m_ctrledNameLoc.EnableWindow(FALSE);
		//"Alter Ice Location %s"
		csTitle.Format(IDS_ICE_ALTER_BU_LOCATION_TITLE,
						(LPCTSTR)m_csName);
		PushHelpId (HELPID_ALTER_IDD_ICE_LOCATION);
	}
	else
	{
		m_nType = 1;
		GetWindowText(csTempo);
		csTitle.Format(csTempo,m_csVnodeName);
		PushHelpId (HELPID_IDD_ICE_LOCATION);
	}
	SetWindowText(csTitle);


	m_ctrledNameLoc.LimitText(sizeof(m_pStructLocationInfo->LocationName)-1);
	m_ctrledPath.LimitText(sizeof(m_pStructLocationInfo->path)-1);
	m_ctrledExtension.LimitText(sizeof(m_pStructLocationInfo->extensions)-1);
	//m_ctrledComment.LimitText(sizeof(m_pStructLocationInfo->Comment)-1);
	UpdateData(FALSE);
	EnableDisableOK();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceServerLocation::OnChangeNameLocation() 
{
	EnableDisableOK();
}
void CxDlgIceServerLocation::EnableDisableOK()
{
	if (m_ctrledNameLoc.LineLength() == 0)
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}

void CxDlgIceServerLocation::OnOK() 
{
	CString csMsg;
	ICELOCATIONDATA StructIceLoc;
	memset(&StructIceLoc,0,sizeof(ICELOCATIONDATA));
	UpdateData(TRUE);

	lstrcpy((char *)StructIceLoc.LocationName,m_csName);
	StructIceLoc.bPublic = m_bPublic;
	if (m_nType)
		StructIceLoc.bIce = TRUE;
	else
		StructIceLoc.bIce = FALSE;

	lstrcpy((char *)StructIceLoc.path,m_csPath);
	lstrcpy((char *)StructIceLoc.extensions,m_csExtension);
	//lstrcpy((char *)StructIceLoc.Comment,m_csComment);

	if (!m_pStructLocationInfo->bAlter)
	{
		if (ICEAddobject((LPUCHAR)(LPCTSTR)m_csVnodeName ,OT_ICE_SERVER_LOCATION,&StructIceLoc ) == RES_ERR)
		{
			csMsg.LoadString(IDS_E_ICE_ADD_SVR_LOCATION);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csVnodeName,OT_ICE_SERVER_LOCATION,m_pStructLocationInfo , &StructIceLoc ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_SVR_LOCATION);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	CDialog::OnOK();
}
