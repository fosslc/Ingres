#if !defined(AFX_CUDLGLOGFILEADD_H__4E6F44BB_6929_11D2_B0F4_00805FE6FB5C__INCLUDED_)
#define AFX_CUDLGLOGFILEADD_H__4E6F44BB_6929_11D2_B0F4_00805FE6FB5C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CuDlgLogFileAdd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCuDlgLogFileAdd dialog

class CCuDlgLogFileAdd : public CDialog
{
// Construction
public:
	CCuDlgLogFileAdd(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCuDlgLogFileAdd)
	enum { IDD = IDD_LOGFILE_ADDFILE };
	CString	m_LogFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCuDlgLogFileAdd)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCuDlgLogFileAdd)
	afx_msg void OnBrowse();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CUDLGLOGFILEADD_H__4E6F44BB_6929_11D2_B0F4_00805FE6FB5C__INCLUDED_)
