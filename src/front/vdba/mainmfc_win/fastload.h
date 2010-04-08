#if !defined(AFX_FASTLOAD_H__67239682_5C28_11D2_9727_00609725DBF9__INCLUDED_)
#define AFX_FASTLOAD_H__67239682_5C28_11D2_9727_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// fastload.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgFastLoad dialog

class CxDlgFastLoad : public CDialog
{
// Construction
public:
	CString m_csTableName;
	CString m_csOwnerTbl;
	CString m_csDatabaseName;
	CString m_csNodeName;
	CxDlgFastLoad(CWnd* pParent = NULL);   // standard constructor
	void EnableDisableOKbutton();

// Dialog Data
	//{{AFX_DATA(CxDlgFastLoad)
	enum { IDD = IDD_FASTLOAD };
	CButton	m_crtlOKButton;
	CEdit	m_ctrledFileName;
	CButton	m_ctrlbtBrowseFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgFastLoad)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgFastLoad)
	afx_msg void OnBrowseFile();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeEditFileName();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FASTLOAD_H__67239682_5C28_11D2_9727_00609725DBF9__INCLUDED_)
