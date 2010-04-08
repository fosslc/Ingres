/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Ingres Visual Manager
**
**  Source   : xdlgmsg.h : header file
**  Author   : Sotheavut UK (uk$so01)
**  Purpose  : Dialog box to be used as a MessageBox
**             with a checkbox "Show this message later"
**
** History:
** 29-Jun-2000 (uk$so01)
**    SIR #105160, created for adding custom message box. 
**
*/

#if !defined(AFX_XDLGMSG_H__5C8063C1_6C64_11D5_875B_00C04F1F754A__INCLUDED_)
#define AFX_XDLGMSG_H__5C8063C1_6C64_11D5_875B_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxDlgMessage : public CDialog
{
public:
	CxDlgMessage(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgMessage)
	enum { IDD = IDD_XMESSAGE };
	CStatic	m_cStaticIcon;
	BOOL	m_bShowItLater;
	CString	m_strMessage;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	HICON m_hIcon;
	// Generated message map functions
	//{{AFX_MSG(CxDlgMessage)
	virtual void OnOK();
	afx_msg void OnNo();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGMSG_H__5C8063C1_6C64_11D5_875B_00C04F1F754A__INCLUDED_)
