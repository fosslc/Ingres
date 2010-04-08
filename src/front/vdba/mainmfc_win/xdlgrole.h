/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgrole.h , Header File
**    Project  : Ingres II / VDBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog box for Create/Alter Role
**
** History:
**
** 27-Oct-1999 (uk$so01)
**    created, Rewrite the dialog code, in C++/MFC.
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Missing Pop help ID when destroying the dialog
**  09-Mar-2010 (drivi01) SIR #123397
**    Update the dialog to be more user friendly.  Minimize the amount
**    of controls exposed to the user when dialog is initially displayed.
**    Add "Show/Hide Advanced" button to see more/less options in the 
**    dialog. Put advanced options into a tab control.
**
*/
#if !defined(AFX_XDLGROLE_H__28A94563_8BC1_11D3_A317_00609725DDAF__INCLUDED_)
#define AFX_XDLGROLE_H__28A94563_8BC1_11D3_A317_00609725DDAF__INCLUDED_
#include "urpectrl.h" // User, Role, Profile list control of privileges
#include "uchklbox.h" // Check list box.
#include "dmlurp.h"
#include "xDlgRolePriv.h"
#include "xDlgRoleSec.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgRole : public CDialogEx
{
		friend class CxDlgRolePriv;
		friend class CxDlgRoleSec;
public:
	CxDlgRole(CWnd* pParent = NULL);
	CxDlgRole(CWnd* pParent /*=NULL*/, BOOL bAlter);
	void SetParam (CaRole* pParam)
	{
		m_pParam = pParam;
		InitClassMembers();
	}
	CaRole* m_pParam;
	void InitClassMembers(BOOL bInitMember = TRUE);
	virtual void OnOK();

	// Dialog Data
	//{{AFX_DATA(CxDlgRole)
	enum { IDD = IDD_XROLE };
	CButton	m_cButtonOK;
	CEdit	m_cEditRoleName;
	CString	m_strRoleName;
	CTabCtrl	m_usrTab;
	CButton		m_cAdvanced;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgRole)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CxDlgRolePriv* m_cDlgRolePriv;
	CxDlgRoleSec* m_cDlgRoleSec;

	// Generated message map functions
	//{{AFX_MSG(CxDlgRole)
	afx_msg void OnChangeEditRoleName();
	virtual BOOL OnInitDialog();
	virtual void OnSelchangeTabLocation(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	virtual void OnShowHideAdvanced();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGROLE_H__28A94563_8BC1_11D3_A317_00609725DDAF__INCLUDED_)
