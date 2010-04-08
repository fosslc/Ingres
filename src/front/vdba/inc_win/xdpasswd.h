/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdpasswd.h, Header File
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

#pragma once
#if defined (__cplusplus)

// CxDlgEnterPassword dialog
class CxDlgEnterPassword : public CDialog
{
	DECLARE_DYNAMIC(CxDlgEnterPassword)

public:
	CxDlgEnterPassword(CWnd* pParent = NULL);   // standard constructor
	virtual ~CxDlgEnterPassword();

// Dialog Data
	enum { IDD = IDD_RCT_INPUTPASSWORD };
	CString	m_strPassword;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
extern "C" BOOL Prompt4Password(LPTSTR lpszBuffer, int nBufferSize);

#else
extern BOOL Prompt4Password(LPTSTR lpszBuffer, int nBufferSize);
#endif
