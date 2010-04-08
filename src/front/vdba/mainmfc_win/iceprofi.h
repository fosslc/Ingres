#if !defined(AFX_ICEPROFI_H__516E09E6_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
#define AFX_ICEPROFI_H__516E09E6_5CEF_11D2_9727_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IceProfi.h : header file
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
// CxDlgIceProfile dialog

class CxDlgIceProfile : public CDialog
{
// Construction
public:
	int m_nHnodeHandle;
	CxDlgIceProfile(CWnd* pParent = NULL);   // standard constructor
	LPICEPROFILEDATA m_pStructProfileInfo;
	CString m_csVnodeName;
// Dialog Data
	//{{AFX_DATA(CxDlgIceProfile)
	enum { IDD = IDD_ICE_PROFILE };
	CEdit	m_ctrledTimeOut;
	CButton	m_ctrlOK;
	CEdit	m_ctrledName;
	CComboBox	m_cbDbUser;
	long	m_lTimeout;
	BOOL	m_bAdmin;
	BOOL	m_bMonitoring;
	CString	m_csName;
	BOOL	m_bSecurityAdmin;
	BOOL	m_bUnitManager;
	CString	m_csDefUser;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceProfile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void FillDatabasesUsers ();
	void EnableDisableOK ();
	BOOL FillstructureFromCtrl( LPICEPROFILEDATA pProfileDta );

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceProfile)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeCpName();
	afx_msg void OnSelchangeCpDefDbUser();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEPROFI_H__516E09E6_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
