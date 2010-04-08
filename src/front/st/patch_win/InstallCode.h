/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: InstallCode.h : header file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	23-July-2001 (penga03)
**	    Added m_codelist to save all installtion codes.
**	05-nov-2001 (penga03)
**	    Removed m_codelist.
**	01-Mar-2006 (drivi01)
**	    Reused for MSI Patch Installer on Windows.
*/

#if !defined(AFX_INSTALLCODE_H__3CBC09E7_6658_4792_8FB5_B5C806BAF991__INCLUDED_)
#define AFX_INSTALLCODE_H__3CBC09E7_6658_4792_8FB5_B5C806BAF991__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CInstallCode dialog

class CInstallCode : public CPropertyPage
{
	DECLARE_DYNCREATE(CInstallCode)

// Construction
public:
	CInstallCode();
	~CInstallCode();

// Dialog Data
	//{{AFX_DATA(CInstallCode)
	enum { IDD = IDD_INSTALLCODE };
	CEdit	m_code;
	CListCtrl	m_list;
	C256bmp	m_image;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CInstallCode)
	public:
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CInstallCode)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedInstalledlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusInstallationcode();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void SetList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INSTALLCODE_H__3CBC09E7_6658_4792_8FB5_B5C806BAF991__INCLUDED_)
