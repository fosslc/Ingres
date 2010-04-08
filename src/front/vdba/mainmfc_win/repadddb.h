#if !defined(AFX_REPADDDB_H__E9CAAAC2_7B05_11D3_97CC_0020AF45B3B5__INCLUDED_)
#define AFX_REPADDDB_H__E9CAAAC2_7B05_11D3_97CC_0020AF45B3B5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// repadddb.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgReplicConnectionDB dialog

class CxDlgReplicConnectionDB : public CDialog
{
// Construction
public:
	BOOL FillComboBoxSvrType();
	int m_nNodeHandle;
	LPREPCDDS m_pStructVariableInfo;
	BOOL FillComboBoxDB();
	CxDlgReplicConnectionDB(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CxDlgReplicConnectionDB)
	enum { IDD = IDD_REPLIC_ADD_DATABASE };
	CEdit	m_ctrlEditSvrNum;
	CComboBox	m_ctrlTargetType;
	CSpinButtonCtrl	m_ctrlSpin_SvrNumber;
	CComboBox	m_comboDatabase;
	UINT	m_editsvrNumber;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgReplicConnectionDB)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgReplicConnectionDB)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPADDDB_H__E9CAAAC2_7B05_11D3_97CC_0020AF45B3B5__INCLUDED_)
