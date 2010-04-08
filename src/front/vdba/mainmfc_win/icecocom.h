#if !defined(AFX_ICECOCOM_H__F78A9A82_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_ICECOCOM_H__F78A9A82_6D9A_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icecocom.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceCommonCombo dialog

class CxDlgIceCommonCombo : public CDialog
{
// Construction
public:
	CxDlgIceCommonCombo(CWnd* pParent = NULL);   // standard constructor
	int m_nCurrentObject;
	int m_nParentObject;
	int m_nHnodeHandle;
	int m_nID4FillCombo;
	int m_nHelpId;
	UINT m_nMessageID;
	UINT m_nMessageErrorFillCombo;
	CString m_csParentName;
	CString m_csTitle;
// Dialog Data
	//{{AFX_DATA(CxDlgIceCommonCombo)
	enum { IDD = IDD_ICE_COMMON_COMBOBOX };
	CButton	m_ctrlOK;
	CStatic	m_ctrlStatic;
	CComboBox	m_ctrlCommon;
	CString	m_csStatic;
	CString	m_csCommon;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceCommonCombo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void FillComboBox();
	void EnableDisableOK();

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceCommonCombo)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeCommonCombobox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICECOCOM_H__F78A9A82_6D9A_11D2_9734_00609725DBF9__INCLUDED_)
