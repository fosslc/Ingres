#if !defined(AFX_ICEBUNUS_H__A1070613_6813_11D2_972F_00609725DBF9__INCLUDED_)
#define AFX_ICEBUNUS_H__A1070613_6813_11D2_972F_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icebunus.h : header file
//
extern "C"
{
#include "ice.h"
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBusinessUser dialog

class CxDlgIceBusinessUser : public CDialog
{
// Construction
public:
	LPICEBUSUNITWEBUSERDATA m_lpStructBunitUser;
	CxDlgIceBusinessUser(CWnd* pParent = NULL);   // standard constructor
	int m_nHnodeHandle;
// Dialog Data
	//{{AFX_DATA(CxDlgIceBusinessUser)
	enum { IDD = IDD_ICE_BUSINESS_USER };
	CButton	m_ctrlOK;
	CComboBox	m_ctrlWebUser;
	BOOL	m_bCreateDoc;
	BOOL	m_bReadDoc;
	BOOL	m_bUnitRead;
	CString	m_csWebUser;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceBusinessUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void EnableDisableOK();
	void FillUserCombobox();

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceBusinessUser)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeWebUser();
	afx_msg void OnUserCreateDoc();
	afx_msg void OnUserReadDoc();
	afx_msg void OnUserUnitRead();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEBUNUS_H__A1070613_6813_11D2_972F_00609725DBF9__INCLUDED_)
