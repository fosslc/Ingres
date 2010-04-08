/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgRolePriv.h
** 
** History:
**	08-Feb-2010 (drivi01)
**		Created.
**/


#if !defined(AFX_XDLGROLEPRIV__INCLUDED_)
#define AFX_XDLGROLEPRIV__INCLUDED_

#pragma once
#include "dmlurp.h"
#include "resmfc.h"
#include "urpectrl.h" // User, Role, Profile list control of privileges

// CxDlgRolePriv dialog

class CxDlgRolePriv : public CDialog
{
	DECLARE_DYNAMIC(CxDlgRolePriv)

public:
	CxDlgRolePriv(CWnd* pParent = NULL);   // standard constructor
	virtual ~CxDlgRolePriv();
	void InitClassMembers(BOOL bInitMember = TRUE);

// Dialog Data
	enum { IDD = IDD_XROLE_PRIV };
	//}}AFX_DATA

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CaRole* m_pParam;
	CImageList m_ImageCheck;
	CuEditableListCtrlURPPrivileges m_cListPrivilege;
	void FillPrivileges();
	afx_msg BOOL OnInitDialog();
	afx_msg void OnOK();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

#endif