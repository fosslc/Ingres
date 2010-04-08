#if !defined(AFX_ICEWEBU_H__516E09E5_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
#define AFX_ICEWEBU_H__516E09E5_5CEF_11D2_9727_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IceWebU.h : header file
//

extern "C"
{
#include "main.h"
#include "monitor.h"
#include "domdata.h"
#include "getdata.h"
#include "ice.h"
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceWebUser dialog

class CxDlgIceWebUser : public CDialog
{
// Construction
public:
	int m_nHnodeHandle;
	CxDlgIceWebUser(CWnd* pParent = NULL);   // standard constructor
	LPICEWEBUSERDATA m_pStructWebUsrInfo;
	CString m_csVnodeName;
// Dialog Data
	//{{AFX_DATA(CxDlgIceWebUser)
	enum { IDD = IDD_ICE_WEB_USER };
	CButton	m_ctrlOK;
	CEdit	m_ctrledUserName;
	CEdit	m_ctrledTimeOut;
	CComboBox	m_ctrlcbUser;
	CEdit	m_ctrledPassword;
	CEdit	m_ctrledConfirm;
	CEdit	m_ctrledComment;
	long	m_lTimeOut;
	BOOL	m_bSecurAdmin;
	BOOL	m_bUnitManager;
	BOOL	m_bMonitoring;
	BOOL	m_bAdmin;
	CString	m_csDefUser;
	CString	m_csPassword;
	CString	m_csConfirmPassword;
	CString	m_csUserName;
	CString	m_csComment;
	int		m_nTypePassword;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceWebUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void EnableDisableOK ();
	BOOL FillstructureFromCtrl(LPICEWEBUSERDATA pWebUsr);
	void FillDatabasesUsers ();
	void EnableDisablePassword();
	// Generated message map functions
	//{{AFX_MSG(CxDlgIceWebUser)
	afx_msg void OnChangeIceCwuConfirm();
	afx_msg void OnChangeIceCwuPassword();
	afx_msg void OnSelchangeIceDefDbUser();
	afx_msg void OnChangeIceUsrName();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnRadioOperasystem();
	afx_msg void OnRadioRepository();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEWEBU_H__516E09E5_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
