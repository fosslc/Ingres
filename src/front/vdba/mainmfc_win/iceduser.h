#if !defined(AFX_ICEDUSER_H__516E09E3_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
#define AFX_ICEDUSER_H__516E09E3_5CEF_11D2_9727_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IceDUser.h : header file
//
extern "C"
{
#include "ice.h"
}
/////////////////////////////////////////////////////////////////////////////
// CxDlgIceDbUser dialog

class CxDlgIceDbUser : public CDialog
{
// Construction
public:
	CxDlgIceDbUser(CWnd* pParent = NULL);   // standard constructor
	LPICEDBUSERDATA m_pStructDbUser;
	CString m_csVirtNodeName;
// Dialog Data
	//{{AFX_DATA(CxDlgIceDbUser)
	enum { IDD = IDD_ICE_DATABASE_USER };
	CButton	m_ctrlOK;
	CEdit	m_ctrlPassWord;
	CEdit	m_ctrlName;
	CEdit	m_ctrlConfirm;
	CEdit	m_ctrlComment;
	CEdit	m_ctrlAlias;
	CString	m_csAlias;
	CString	m_csComment;
	CString	m_csConfirm;
	CString	m_csName;
	CString	m_csPassword;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceDbUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void EnableDisableOK ();
	// Generated message map functions
	//{{AFX_MSG(CxDlgIceDbUser)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeDbuAlias();
	afx_msg void OnChangeDbuName();
	afx_msg void OnChangeDbuPassword();
	afx_msg void OnChangeDbuConfirm();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
	BOOL FillstructureFromCtrl(LPICEDBUSERDATA pStrucDbUserNew);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEDUSER_H__516E09E3_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
