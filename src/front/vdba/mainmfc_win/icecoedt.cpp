/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : icecoedt.cpp : implementation File.
**    Project  : INGRES II/ ICE- VDBA.
**    Author   : UK Sotheavut (uk$so01)
**
** History: (after 05-July-2005)
**
**	05-July-2005 (komve01)
**	Bug#114787/Issue#14198174
**	Added a limitation for m_csEditValue to limit it to 32 characters.
**	Also added missing copyright statements.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "icecoedt.h"
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
extern "C" BOOL MfcDlgCommonEdit( int nType , int nHnode)
{
	CxDlgIceCommonEdit dlg;
	dlg.m_nCurrentObject = nType;
	dlg.m_nHnodeHandle = nHnode;
	switch (nType)
	{
	case OT_ICE_BUNIT:
		dlg.m_nHelpId       = HELPID_IDD_COMMON_ED_BUSINESS_UNIT;
		dlg.m_nMessageID    = IDS_E_ICE_BUSINESS_UNIT_FAILED;
		dlg.m_csStaticValue = _T("&Business unit:");
		//"Create Business Unit on %s"
		dlg.m_csTitle.Format(IDS_ICE_BUNIT_TITLE,GetVirtNodeName (dlg.m_nHnodeHandle));
	break;
	case OT_ICE_SERVER_APPLICATION:
		dlg.m_nHelpId       = HELPID_IDD_COMMON_ED_SESSION_GROUP;
		dlg.m_nMessageID    = IDS_E_ICE_SESSION_GROUP_FAILED;
		dlg.m_csStaticValue = _T("&Session Group:");
		//"Create Session Group Name on %s"
		dlg.m_csTitle.Format(IDS_ICE_SERVER_APPLICATION_TITLE,GetVirtNodeName (dlg.m_nHnodeHandle));
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
// CxDlgIceCommonEdit dialog


CxDlgIceCommonEdit::CxDlgIceCommonEdit(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceCommonEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceCommonEdit)
	m_csEditValue = _T("");
	m_csStaticValue = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceCommonEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceCommonEdit)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_COMMON_STATIC, m_ctrlCommonStatic);
	DDX_Control(pDX, IDC_COMMON_EDIT, m_ctrlCommonEdit);
	DDX_Text(pDX, IDC_COMMON_EDIT, m_csEditValue);
	DDV_MaxChars(pDX, m_csEditValue, 32);
	DDX_Text(pDX, IDC_COMMON_STATIC, m_csStaticValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceCommonEdit, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceCommonEdit)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_COMMON_EDIT, OnChangeCommonEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceCommonEdit message handlers

BOOL CxDlgIceCommonEdit::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetWindowText(m_csTitle);
	EnableDisableOK();
	PushHelpId (m_nHelpId);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceCommonEdit::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgIceCommonEdit::OnChangeCommonEdit() 
{
	EnableDisableOK();
}
void CxDlgIceCommonEdit::EnableDisableOK()
{
	if (m_ctrlCommonEdit.LineLength() == 0)
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}
void CxDlgIceCommonEdit::OnOK() 
{
	ICESBUSUNITDATA BusUnitDta;
	ICEAPPLICATIONDATA AppDta;
	LPVOID pStruct = NULL;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (m_nHnodeHandle);
	UpdateData(TRUE);
	
	switch (m_nCurrentObject)
	{
	case OT_ICE_BUNIT:
		memset(&BusUnitDta,0,sizeof(ICESBUSUNITDATA));
		lstrcpy((char *)BusUnitDta.Name,m_csEditValue);
		pStruct = &BusUnitDta;
		break;
	case OT_ICE_SERVER_APPLICATION:
		memset(&AppDta,0,sizeof(ICEAPPLICATIONDATA));
		lstrcpy((char *)AppDta.Name,m_csEditValue);
		pStruct = &AppDta;
		break;
	default:
		break;
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
