/******************************************************************************
**                                                                           
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**                                                                           
**    Source   : icesvrdt.cpp : implementation file                          
**                                                                           
**                                                                           
**    Project  : Ingres II/Vdba.                                             
**    Author   : Schalk Philippe                                             
**                                                                           
**    Purpose  : Popup Dialog Box for create Ice Server Variable.            
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
******************************************************************************/
//  : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "icesvrdt.h"
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
extern "C" BOOL MfcDlgIceServerVariable( LPICESERVERVARDATA lpIceVar ,int nHnode)
{
	CxDlgIceServerVariable dlg;
	dlg.m_pStructVariableInfo = lpIceVar;
	dlg.m_csVnodeName = GetVirtNodeName (nHnode);
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceServerVariable dialog


CxDlgIceServerVariable::CxDlgIceServerVariable(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceServerVariable::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceServerVariable)
	m_csName = _T("");
	m_csValue = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceServerVariable::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceServerVariable)
	DDX_Control(pDX, IDC_ICE_VALUE, m_ctrledValue);
	DDX_Control(pDX, IDC_ICE_NAME, m_ctrledName);
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Text(pDX, IDC_ICE_NAME, m_csName);
	DDX_Text(pDX, IDC_ICE_VALUE, m_csValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceServerVariable, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceServerVariable)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_ICE_NAME, OnChangeIceName)
	ON_EN_CHANGE(IDC_ICE_VALUE, OnChangeIceValue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceServerVariable message handlers

void CxDlgIceServerVariable::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

BOOL CxDlgIceServerVariable::OnInitDialog() 
{
	CString csTempo,csTitle;
	CDialog::OnInitDialog();

	if( m_pStructVariableInfo->bAlter)
	{
		m_csName  = m_pStructVariableInfo->VariableName;
		m_csValue = m_pStructVariableInfo->VariableValue;
		m_ctrledName.EnableWindow(FALSE);
		//"Alter Ice Server Variable %s"
		csTitle.Format(IDS_ICE_ALTER_SVR_VARIABLE_TITLE,
				(LPCTSTR)m_csName);
		UpdateData(FALSE);
		PushHelpId (HELPID_ALTER_IDD_ICE_SVR_VARIABLE);
	}
	else
	{
		GetWindowText(csTempo);
		csTitle.Format(csTempo,m_csVnodeName);
		PushHelpId (HELPID_IDD_ICE_SVR_VARIABLE);
	}
	SetWindowText(csTitle);

	m_ctrledName.LimitText(sizeof(m_pStructVariableInfo->VariableName)-1);
	m_ctrledValue.LimitText(sizeof(m_pStructVariableInfo->VariableValue)-1);

	EnableDisableOK();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceServerVariable::OnChangeIceName() 
{
	EnableDisableOK();
}

void CxDlgIceServerVariable::OnChangeIceValue() 
{
	EnableDisableOK();
}

void CxDlgIceServerVariable::EnableDisableOK ()
{
	if (m_ctrledName.LineLength() == 0 ||
		m_ctrledValue.LineLength() == 0 )
		m_ctrlOK.EnableWindow(FALSE);
	else
		m_ctrlOK.EnableWindow(TRUE);
}

void CxDlgIceServerVariable::OnOK() 
{
	CString csMsg;
	ICESERVERVARDATA StructIceVar;
	memset(&StructIceVar,0,sizeof(ICESERVERVARDATA));
	UpdateData(TRUE);

	lstrcpy((char *)StructIceVar.VariableName,m_csName);
	lstrcpy((char *)StructIceVar.VariableValue,m_csValue);

	if (!m_pStructVariableInfo->bAlter)
	{
		if (ICEAddobject( (LPUCHAR)(LPCTSTR)m_csVnodeName,OT_ICE_SERVER_VARIABLE,&StructIceVar ) == RES_ERR)
		{
			csMsg.LoadString(IDS_E_ICE_ADD_SVR_VARIABLE);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}
	else
	{
		if ( ICEAlterobject( (LPUCHAR)(LPCTSTR)m_csVnodeName,OT_ICE_SERVER_VARIABLE,m_pStructVariableInfo , &StructIceVar ) == RES_ERR )
		{
			csMsg.LoadString(IDS_E_ICE_ALTER_SVR_VARIABLE);
			MessageWithHistoryButton(m_hWnd,csMsg);
			return;
		}
	}

	CDialog::OnOK();
}
