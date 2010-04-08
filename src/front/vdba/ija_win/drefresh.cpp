/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : drefresh.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Purpose  :  Popup Dialog proposing the option of the Refresh data
**
** History:
**
** 23-Dec-1999 (uk$so01)
**    Created
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
**
**/


#include "stdafx.h"
#include "rcdepend.h"
#include "drefresh.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgRefresh::CxDlgRefresh(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRefresh::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgRefresh)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nRadioChecked = IDC_RADIO1;
}


void CxDlgRefresh::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgRefresh)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgRefresh, CDialog)
	//{{AFX_MSG_MAP(CxDlgRefresh)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRefresh message handlers

void CxDlgRefresh::OnOK() 
{
	CDialog::OnOK();
	m_nRadioChecked = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3);
}

BOOL CxDlgRefresh::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CxDlgRefresh::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo(m_hWnd, ID_HELP_REFRESH);
		
	return TRUE;
}
