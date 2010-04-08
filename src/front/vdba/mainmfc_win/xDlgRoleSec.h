/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgRoleSec.h
** 
** History:
**	08-Feb-2010 (drivi01)
**		Created.
**/

#if !defined(AFX_XDLGROLESEC__INCLUDED_)
#define AFX_XDLGROLESEC__INCLUDED_

#pragma once

#include "urpectrl.h" // User, Role, Profile list control of privileges
#include "uchklbox.h" // Check list box.
#include "dmlurp.h"
#include "resmfc.h"
#include "xdlgrole.h"

// CxDlgRoleSec dialog

class CxDlgRoleSec : public CDialog
{
	DECLARE_DYNAMIC(CxDlgRoleSec)

public:
	CxDlgRoleSec(CWnd* pParent = NULL);   // standard constructor
	virtual ~CxDlgRoleSec();
	void InitClassMembers(BOOL bInitMember = TRUE);

// Dialog Data
	enum { IDD = IDD_XROLE_SEC };
	CEdit	m_cEditConfirmPassword;
	CEdit	m_cEditPassword;
	CButton	m_cCheckExternalPassword;
	CButton	m_bAllEvents;
	CButton	m_bQueryText;
	CString	m_strLimitSecurityLabel;
	CString	m_strPassword;
	CString	m_strConfirmPassword;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CaRole* m_pParam;
	CuCheckListBox m_cCheckListBoxDatabase;

	void FillDatabases();
	// Generated message map functions
	//{{AFX_MSG(CxDlgRole)
	afx_msg void OnCheckExternalPassword();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnDestroy();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedStaticAccessdb();
};

#endif