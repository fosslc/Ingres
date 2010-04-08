#if !defined(AFX_ICELOCAT_H__F78A9A86_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_ICELOCAT_H__F78A9A86_6D9A_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icelocat.h : header file
//
extern "C"
{
#include "ice.h"
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceServerLocation dialog

class CxDlgIceServerLocation : public CDialog
{
// Construction
public:
	CxDlgIceServerLocation(CWnd* pParent = NULL);   // standard constructor
	LPICELOCATIONDATA m_pStructLocationInfo;
	CString m_csVnodeName;
// Dialog Data
	//{{AFX_DATA(CxDlgIceServerLocation)
	enum { IDD = IDD_ICE_LOCATION };
	CEdit	m_ctrledExtension;
	CEdit	m_ctrledPath;
	CEdit	m_ctrledNameLoc;
	CButton	m_ctrlOK;
	CString	m_csName;
	BOOL	m_bPublic;
	CString	m_csExtension;
	CString	m_csPath;
	int		m_nType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceServerLocation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableDisableOK();

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceServerLocation)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNameLocation();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICELOCAT_H__F78A9A86_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
