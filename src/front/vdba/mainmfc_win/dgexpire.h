#if !defined(AFX_DGEXPIRE_H__F6928432_733C_11D2_A3C2_00609725DBFA__INCLUDED_)
#define AFX_DGEXPIRE_H__F6928432_733C_11D2_A3C2_00609725DBFA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dgexpire.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgExpireDate dialog

class CuDlgExpireDate : public CDialog
{
// Construction
public:
	CuDlgExpireDate(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CuDlgExpireDate)
	enum { IDD = IDD_EXPIREDATE };
	CButton	m_btnOk;
	CEdit	m_edExpireDate;
	CString	m_csExpireDate;
	int		m_iRad;
	//}}AFX_DATA
  CString m_csTableName;
  CString m_csOwnerName;
  CString m_csVirtNodeName;
  CString m_csParentDbName;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgExpireDate)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgExpireDate)
	afx_msg void OnRadExpireNo();
	afx_msg void OnRadExpireYes();
	afx_msg void OnChangeEditExpiredate();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
  BOOL ChangeExpirationDate();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGEXPIRE_H__F6928432_733C_11D2_A3C2_00609725DBFA__INCLUDED_)
