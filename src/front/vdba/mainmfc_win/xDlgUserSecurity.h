/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgUserSecurity.h
** 
** History:
**	08-Feb-2010 (drivi01)
**		Created.
**/

#if !defined(AFX_XDLGUSERSECURITY__INCLUDED_)
#define AFX_XDLGUSERSECURITY__INCLUDED_

#pragma once

#include "resmfc.h"
#include "uchklbox.h" // Check list box.
#include "dmlurp.h"

// CxDlgUserSecurity dialog

class CxDlgUserSecurity : public CDialogEx
{
	DECLARE_DYNAMIC(CxDlgUserSecurity)

public:
	CxDlgUserSecurity(CWnd* pParent = NULL);   // standard constructor
	virtual ~CxDlgUserSecurity();
	void InitClassMember(BOOL bInitMember = TRUE);
	CuCheckListBox m_cCheckListBoxDatabase;

// Dialog Data
	enum { IDD = IDD_XUSER_SECURITY };
	CButton	m_RmcmdTitle;
	CButton	m_ctrlCheckGrantRMCMD;
	CButton	m_ctrlRemovePwd;
	CButton m_AllEvents;
	CButton m_QueryText;
	CStatic	m_ctrlStaticPartial;
	CStatic	m_ctrlIconPartial;
	CEdit	m_cEditConfirmPassword;
	CEdit	m_cEditPassword;
	CEdit   m_cOldPassword;
	CButton	m_cCheckExternalPassword;
	BOOL	m_bAllEvents;
	BOOL	m_bQueryText;
	BOOL	m_bExternalPassword;
	CString	m_strLimitSecurityLabel;
	CString	m_strPassword;
	CString	m_strConfirmPassword;
	BOOL	m_bGrantRmcmd;
	BOOL	m_bRemovePwd;
	BOOL	bExpanded;
	CTabCtrl m_cTabUser;
	CString m_strOldPassword;

protected:
	CaUser *pParam2;
	void FillDatabases();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg BOOL OnInitDialog();
	afx_msg void OnCheckExternalPassword();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	DECLARE_MESSAGE_MAP()
};

#endif