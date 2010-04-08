/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgprof.h , Header File 
**    Project  : Ingres II / VDBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog box for Create/Alter Profile
**
** History:
**
** 27-Oct-1999 (uk$so01)
**    created, Rewrite the dialog code, in C++/MFC.
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Missing Pop help ID when destroying the dialog
**
*/
#if !defined(AFX_XDLGPROF_H__28A94562_8BC1_11D3_A317_00609725DDAF__INCLUDED_)
#define AFX_XDLGPROF_H__28A94562_8BC1_11D3_A317_00609725DDAF__INCLUDED_
#include "urpectrl.h" // User, Role, Profile list control of privileges
#include "dmlurp.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgProfile : public CDialog
{
public:
	CxDlgProfile(CWnd* pParent = NULL);
	void SetParam (CaProfile* pParam)
	{
		m_pParam = pParam;
		InitClassMembers();
	}
	void InitClassMembers(BOOL bInitMember = TRUE);

	// Dialog Data
	//{{AFX_DATA(CxDlgProfile)
	enum { IDD = IDD_XPROFILE };
	CComboBox	m_cComboGroup;
	CEdit	m_cEditProfileName;
	CButton	m_cButtonOK;
	BOOL	m_bAllEvents;
	BOOL	m_bQueryText;
	CString	m_strGroup;
	CString	m_strProfileName;
	CString	m_strExpireDate;
	CString	m_strLimitSecurityLabel;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgProfile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageCheck;
	CuEditableListCtrlURPPrivileges m_cListPrivilege;
	CaProfile* m_pParam;

	void FillPrivileges();
	void FillGroups();

	// Generated message map functions
	//{{AFX_MSG(CxDlgProfile)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGPROF_H__28A94562_8BC1_11D3_A317_00609725DDAF__INCLUDED_)
