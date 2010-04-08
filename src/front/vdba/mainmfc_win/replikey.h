#if !defined(AFX_REPLIKEY_H__E84D3F62_A3AD_11D2_974F_00609725DBF9__INCLUDED_)
#define AFX_REPLIKEY_H__E84D3F62_A3AD_11D2_974F_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// replikey.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationSelectKeyOption dialog

class CuDlgReplicationSelectKeyOption : public CDialog
{
// Construction
public:
	CuDlgReplicationSelectKeyOption(CWnd* pParent = NULL);   // standard constructor
	enum {CREATEKEYSHADOWTABLEONLY,CREATEKEYBOTHQUEUEANDSHADOW};
	int m_nKeyType;
	CString m_strTableName;

// Dialog Data
	//{{AFX_DATA(CuDlgReplicationSelectKeyOption)
	enum { IDD = IDD_REPLICATION_KEY_OPTION };
	CButton	m_ctrlOK;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationSelectKeyOption)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationSelectKeyOption)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPLIKEY_H__E84D3F62_A3AD_11D2_974F_00609725DBF9__INCLUDED_)
