/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlguser.h , Header File
**    Project  : Ingres II / VDBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog box for Create/Alter User
**
** History:
**
** 27-Oct-1999 (uk$so01)
**    created, Rewrite the dialog code, in C++/MFC.
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Missing Pop help ID when destroying the dialog
**  07-Apr-2005 (srisu02)
**   Add functionality for 'Delete password' checkbox in 'Alter User' dialog box
** 09-Mar-2010 (drivi01) SIR #123397
**    Update the dialog to be more user friendly.  Minimize the amount
**    of controls exposed to the user when dialog is initially displayed.
**    Add "Show/Hide Advanced" button to see more/less options in the 
**    dialog. Put advanced options into a tab control.
**
*/
#if !defined(AFX_XDLGUSER_H__CCC5A422_8B7B_11D3_A316_00609725DDAF__INCLUDED_)
#define AFX_XDLGUSER_H__CCC5A422_8B7B_11D3_A316_00609725DDAF__INCLUDED_
#include "urpectrl.h" // User, Role, Profile list control of privileges
#include "dmlurp.h"
#include "xDlgUserPriv.h"
#include "xDlgUserSecurity.h"
#include "xDlgUserAlterSec.h"


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CxDlgUser : public CDialog
{
	friend class CxDlgUserPriv;
	friend class CxDlgUserSecurity;
	friend class CxDlgUserAlterSec;
public:
	CxDlgUser(CWnd* pParent = NULL);
	CxDlgUser(CWnd* pParent , BOOL bAlter = TRUE);
	void SetParam (CaUser* pParam)
	{
		m_pParam = pParam;
		InitClassMember();
	}
	void InitClassMember(BOOL bInitMember = TRUE);
	CaUser* m_pParam;

	// Dialog Data
	//{{AFX_DATA(CxDlgUser)
	enum { IDD = IDD_XUSER };
	CButton m_ctrlMoreOptions;
	CButton	m_cButtonOK;
	CEdit	m_cEditUserName;
	CComboBox	m_cComboProfile;
	CComboBox	m_cComboGroup;
	CString	m_strGroup;
	CString	m_strProfile;
	CString	m_strUserName;
	CString	m_strExpireDate;
	BOOL	bExpanded;
	CTabCtrl m_cTabUser;
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CxDlgUserPriv *m_cDlgUserPriv;
	CxDlgUserSecurity *m_cDlgUserSecurity;
	CxDlgUserAlterSec *m_cDlgUserAlterSec;

	void FillGroups();
	void FillProfiles();


	// Generated message map functions
	//{{AFX_MSG(CxDlgUser)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnEditChangeUserName();
	afx_msg void OnSelchangeTabLocation(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedMoreOptions();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGUSER_H__CCC5A422_8B7B_11D3_A316_00609725DDAF__INCLUDED_)
