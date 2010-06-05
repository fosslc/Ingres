/*
**  Copyright (c) 2003 Ingres Corporation
*/

/*
**  Name: IngCleanDlg.h : header file 
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
**	06-Apr-2010 (drivi01)
**	    Update OnTimer to take UINT_PTR as a parameter as UINT_PTR
**	    will be unsigned int on x86 and unsigned int64 on x64.
**	    This will cleanup warnings.
*/

#if !defined(AFX_INGCLEANDLG_H__EB908B05_5DD5_4306_BBE9_6BFC1BA34EB7__INCLUDED_)
#define AFX_INGCLEANDLG_H__EB908B05_5DD5_4306_BBE9_6BFC1BA34EB7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CIngCleanDlg dialog

class CIngCleanDlg : public CDialog
{
// Construction
public:
	CIngCleanDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CIngCleanDlg)
	enum { IDD = IDD_INGCLEAN_DIALOG };
	CStatic	m_text;
	CStatic	m_output;
	CStatic	m_copyright;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIngCleanDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CIngCleanDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual void OnCancel();
	afx_msg void OnUninstall();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INGCLEANDLG_H__EB908B05_5DD5_4306_BBE9_6BFC1BA34EB7__INCLUDED_)
