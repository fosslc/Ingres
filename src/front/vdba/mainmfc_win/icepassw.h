#if !defined(AFX_ICEPASSW_H__F78A9A84_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_ICEPASSW_H__F78A9A84_6D9A_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icepassw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgIcePassword dialog

class CxDlgIcePassword : public CDialog
{
// Construction
public:
	CxDlgIcePassword(CWnd* pParent = NULL);   // standard constructor
	CString m_csVnodeName;

// Dialog Data
	//{{AFX_DATA(CxDlgIcePassword)
	enum { IDD = IDD_ICE_PASSWORD };
	CEdit	m_ctrledPassword;
	CEdit	m_ctrledUserName;
	CButton	m_ctrlOK;
	CString	m_csPassword;
	CString	m_csUserName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIcePassword)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableDisableOK();
	// Generated message map functions
	//{{AFX_MSG(CxDlgIcePassword)
	afx_msg void OnChangePassword();
	afx_msg void OnChangeUser();
	virtual void OnOK();
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEPASSW_H__F78A9A84_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
