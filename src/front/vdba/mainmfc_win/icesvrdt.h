#if !defined(AFX_ICESVRDT_H__F78A9A85_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_ICESVRDT_H__F78A9A85_6D9A_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icesvrdt.h : header file
//
extern "C"
{
#include "ice.h"
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceServerVariable dialog

class CxDlgIceServerVariable : public CDialog
{
// Construction
public:
	CxDlgIceServerVariable(CWnd* pParent = NULL);   // standard constructor
	LPICESERVERVARDATA m_pStructVariableInfo;
	CString m_csVnodeName;
// Dialog Data
	//{{AFX_DATA(CxDlgIceServerVariable)
	enum { IDD = IDD_ICE_SVR_VARIABLE };
	CEdit	m_ctrledValue;
	CEdit	m_ctrledName;
	CButton	m_ctrlOK;
	CString	m_csName;
	CString	m_csValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceServerVariable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void EnableDisableOK ();
	// Generated message map functions
	//{{AFX_MSG(CxDlgIceServerVariable)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeIceName();
	afx_msg void OnChangeIceValue();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICESVRDT_H__F78A9A85_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
