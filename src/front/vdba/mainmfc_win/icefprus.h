#if !defined(AFX_ICEFPRUS_H__3ED6E302_7E20_11D2_973A_00609725DBF9__INCLUDED_)
#define AFX_ICEFPRUS_H__3ED6E302_7E20_11D2_973A_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icefprus.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBuDefRoleUserForFacetPage dialog

class CxDlgIceBuDefRoleUserForFacetPage : public CDialog
{
// Construction
public:
	int m_nTypeObj;
	UINT m_nHelpId;
	CString m_csTitle;
	CString m_csStaticValue;
	UINT m_nMessageID;
	int m_nHnode;
	CString m_csCurrentVnodeName;
	LPVOID m_pStructObj;
	CxDlgIceBuDefRoleUserForFacetPage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CxDlgIceBuDefRoleUserForFacetPage)
	enum { IDD = IDD_ICE_BU_FACETPAGE_ROLEUSER };
	CButton	m_ctrlOK;
	CComboBox	m_ctrlCombo;
	CStatic	m_ctrlStaticType;
	BOOL	m_bDelete;
	BOOL	m_bExecute;
	BOOL	m_bRead;
	BOOL	m_bUpdate;
	CString	m_csCurrentSelected;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceBuDefRoleUserForFacetPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void FillCombo();
	void EnableDisableOk();

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceBuDefRoleUserForFacetPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeComboRoleUser();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnCheckDelete();
	afx_msg void OnCheckExecute();
	afx_msg void OnCheckRead();
	afx_msg void OnCheckUpdate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEFPRUS_H__3ED6E302_7E20_11D2_973A_00609725DBF9__INCLUDED_)
