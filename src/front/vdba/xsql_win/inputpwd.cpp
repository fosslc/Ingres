/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : inputpwd.cpp
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Request the role's password.
**
** History:
**
** 12-Aug-2003 (uk$so01)
**    Created
**    SIR #106648
*/

#include "stdafx.h"
#include "xsql.h"
#include "inputpwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgInputPassword::CxDlgInputPassword(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgInputPassword::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgInputPassword)
	m_strPassword = _T("");
	//}}AFX_DATA_INIT
	m_strRole = _T("");
}


void CxDlgInputPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgInputPassword)
	DDX_Text(pDX, IDC_EDIT1, m_strPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgInputPassword, CDialog)
	//{{AFX_MSG_MAP(CxDlgInputPassword)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgInputPassword message handlers

BOOL CxDlgInputPassword::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString strTitleTemplate, strTitle;
	GetWindowText (strTitleTemplate);
	strTitle.Format(strTitleTemplate, (LPCTSTR)m_strRole);
	SetWindowText(strTitle);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
