/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: InstallCode.h : header file
**
**  History:
**
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
*/

#if !defined(AFX_INSTALLCODE_H__3CBC09E7_6658_4792_8FB5_B5C806BAF991__INCLUDED_)
#define AFX_INSTALLCODE_H__3CBC09E7_6658_4792_8FB5_B5C806BAF991__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// WcInstallCode dialog

class WcInstallCode : public CPropertyPage
{
	DECLARE_DYNCREATE(WcInstallCode)

// Construction
public:
	WcInstallCode();
	~WcInstallCode();

// Dialog Data
	//{{AFX_DATA(WcInstallCode)
	enum { IDD = IDD_INSTALLCODE };
	CEdit	m_code;
	CListCtrl	m_list;
	C256bmp	m_image;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(WcInstallCode)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(WcInstallCode)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedInstalledlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusInstallationcode();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void SetList();
	BOOL m_UserSaidYes;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INSTALLCODE_H__3CBC09E7_6658_4792_8FB5_B5C806BAF991__INCLUDED_)
