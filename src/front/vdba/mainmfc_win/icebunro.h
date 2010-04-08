#if !defined(AFX_ICEBUNRO_H__A1070612_6813_11D2_972F_00609725DBF9__INCLUDED_)
#define AFX_ICEBUNRO_H__A1070612_6813_11D2_972F_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icebunro.h : header file
//
extern "C"
{
#include "ice.h"
}
/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBusinessRole dialog

class CxDlgIceBusinessRole : public CDialog
{
// Construction
public:
	LPICEBUSUNITROLEDATA m_StructBunitRole;
	CxDlgIceBusinessRole(CWnd* pParent = NULL);   // standard constructor
	int m_nHnodeHandle;
// Dialog Data
	//{{AFX_DATA(CxDlgIceBusinessRole)
	enum { IDD = IDD_ICE_BUSINESS_ROLE };
	CButton	m_ctrlOK;
	CComboBox	m_cbRole;
	BOOL	m_bCreateDoc;
	BOOL	m_bExecDoc;
	BOOL	m_bReadDoc;
	CString	m_csRoleName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceBusinessRole)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void EnableDisableOK();
	void FillRolesCombobox();

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceBusinessRole)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeRole();
	afx_msg void OnCreateDoc();
	afx_msg void OnExecDoc();
	afx_msg void OnReadDoc();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEBUNRO_H__A1070612_6813_11D2_972F_00609725DBF9__INCLUDED_)
