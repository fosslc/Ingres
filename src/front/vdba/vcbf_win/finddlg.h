#if !defined(AFX_FINDDLG_H__50989AD1_6D34_11D2_9FE1_006008924264__INCLUDED_)
#define AFX_FINDDLG_H__50989AD1_6D34_11D2_9FE1_006008924264__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// FindDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFindDlg dialog

class CFindDlg : public CDialog
{
// Construction
public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor
    CEdit *m_EditText;
    
// Dialog Data
	//{{AFX_DATA(CFindDlg)
	enum { IDD = IDD_FIND_DIALOG };
	CButton	m_cbMatchCase;
	CButton	m_RadioUp;
	CButton	m_RadioDown;
	CEdit	m_FindText;
	CButton	m_FindNext;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFindDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFindDlg)
	afx_msg void OnFindNext();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINDDLG_H__50989AD1_6D34_11D2_9FE1_006008924264__INCLUDED_)
