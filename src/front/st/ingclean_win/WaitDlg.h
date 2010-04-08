/*
**  Copyright (c) 2003 Ingres Corporation
*/

/*
**  Name: WaitDlg.h : header file
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/

#if !defined(AFX_WAITDLG_H__B3378FC1_36C8_4859_B60F_BCAF242A4E7C__INCLUDED_)
#define AFX_WAITDLG_H__B3378FC1_36C8_4859_B60F_BCAF242A4E7C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

class CWaitDlg : public CDialog
{
// Construction
public:
	HANDLE m_hThread;
	CString m_csText;
	UINT m_nAVI_Res;

	CWaitDlg(CString csText, UINT nAVI_Res = IDR_CLOCK_AVI, UINT nDialogID = IDD_WAIT_DIALOG, CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CWaitDlg)
	enum { IDD = IDD_WAIT_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWaitDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITDLG_H__B3378FC1_36C8_4859_B60F_BCAF242A4E7C__INCLUDED_)
