/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dlgenvar.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Add extra Variable Environment 
**
** History:
**
** 01-Sep-1999 (uk$so01)
**    Created
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dlgenvar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxEnvironmentVariable dialog


CxEnvironmentVariable::CxEnvironmentVariable(CWnd* pParent /*=NULL*/)
	: CDialog(CxEnvironmentVariable::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxEnvironmentVariable)
	m_strName = _T("");
	m_strValue = _T("");
	//}}AFX_DATA_INIT
}


void CxEnvironmentVariable::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxEnvironmentVariable)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_EDIT2, m_cEditValue);
	DDX_Control(pDX, IDC_EDIT1, m_cEditName);
	DDX_Text(pDX, IDC_EDIT1, m_strName);
	DDX_Text(pDX, IDC_EDIT2, m_strValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxEnvironmentVariable, CDialog)
	//{{AFX_MSG_MAP(CxEnvironmentVariable)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT2, OnChangeEditValue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxEnvironmentVariable message handlers

void CxEnvironmentVariable::EnableControls()
{
	CString strText;

	m_cEditName.GetWindowText (strText);
	strText.TrimLeft();
	strText.TrimRight();
	if (strText.IsEmpty())
	{
		m_cButtonOK.EnableWindow (FALSE);
		return;
	}

	m_cEditValue.GetWindowText (strText);
	strText.TrimLeft();
	strText.TrimRight();
	if (strText.IsEmpty())
	{
		m_cButtonOK.EnableWindow (FALSE);
		return;
	}

	m_cButtonOK.EnableWindow (TRUE);
}


void CxEnvironmentVariable::OnChangeEditName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	EnableControls();
	
}

void CxEnvironmentVariable::OnChangeEditValue() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	EnableControls();
}


BOOL CxEnvironmentVariable::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	EnableControls();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CxEnvironmentVariable::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nHelpID = IDD;
	return APP_HelpInfo(nHelpID);
}
