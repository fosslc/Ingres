#if !defined(AFX_ICECOEDT_H__F78A9A83_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_ICECOEDT_H__F78A9A83_6D9A_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icecoedt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceCommonEdit dialog

class CxDlgIceCommonEdit : public CDialog
{
// Construction
public:
	CxDlgIceCommonEdit(CWnd* pParent = NULL);   // standard constructor
	int m_nCurrentObject;
	int m_nParentObject;
	int m_nHnodeHandle;
	int m_nHelpId;
	UINT m_nMessageID;
	CString m_csTitle;

// Dialog Data
	//{{AFX_DATA(CxDlgIceCommonEdit)
	enum { IDD = IDD_ICE_COMMON_EDIT };
	CButton	m_ctrlOK;
	CStatic	m_ctrlCommonStatic;
	CEdit	m_ctrlCommonEdit;
	CString	m_csEditValue;
	CString	m_csStaticValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceCommonEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void EnableDisableOK();

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceCommonEdit)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnChangeCommonEdit();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICECOEDT_H__F78A9A83_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
