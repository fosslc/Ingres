/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: WaitDlg.h : header file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
*/

#if !defined(AFX_WAITDLG_H__C084473E_A9B0_4AC2_970F_2DD3DF95067B__INCLUDED_)
#define AFX_WAITDLG_H__C084473E_A9B0_4AC2_970F_2DD3DF95067B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

class CWaitDlg : public CDialog
{
// Construction
public:
	HANDLE m_hThreadWait;
	CString m_strText;
	UINT m_uiAVI_Resource;
	
	CWaitDlg(UINT uiStrText, UINT m_uiAVI_Resource=IDR_CLOCK_AVI, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWaitDlg)
	enum { IDD = IDD_WAIT_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWaitDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITDLG_H__C084473E_A9B0_4AC2_970F_2DD3DF95067B__INCLUDED_)
