/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdpasswd.cpp : implementation file
**    Project  : Ingres r3
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Prompting for password (modal dialog)
**
** History:
**
** 04-Oct-2004 (uk$so01)
**    created
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
**/


#include "stdafx.h"
#include "rctools.h"
#include "xdpasswd.h"


// CxDlgEnterPassword dialog

IMPLEMENT_DYNAMIC(CxDlgEnterPassword, CDialog)
CxDlgEnterPassword::CxDlgEnterPassword(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgEnterPassword::IDD, pParent)
{
	m_strPassword = _T("");
}

CxDlgEnterPassword::~CxDlgEnterPassword()
{
}

void CxDlgEnterPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_RCT_EDIT1, m_strPassword);
}


BEGIN_MESSAGE_MAP(CxDlgEnterPassword, CDialog)
END_MESSAGE_MAP()

extern "C" BOOL Prompt4Password(LPTSTR lpszBuffer, int nBufferSize)
{
	CxDlgEnterPassword dlg;
	if (dlg.DoModal() == IDOK)
	{
		_tcsncpy (lpszBuffer, dlg.m_strPassword, nBufferSize-1);
		return TRUE;
	}
	_tcscpy (lpszBuffer, _T(""));
	return FALSE;
}

// CxDlgEnterPassword message handlers
