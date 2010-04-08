/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : rdrcmsg.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of a Big Text message
**
** History:
**
** 26-Jun-2000 (uk$so01)
**    created
**
**/

#if !defined(AFX_RDRCMSG_H__E516B346_4748_11D4_A363_00C04F1F754A__INCLUDED_)
#define AFX_RDRCMSG_H__E516B346_4748_11D4_A363_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxDlgMessage : public CDialog
{
public:
	CxDlgMessage(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgMessage)
	enum { IDD = IDD_MESSAGE };
	CString	m_strText;
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
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RDRCMSG_H__E516B346_4748_11D4_A363_00C04F1F754A__INCLUDED_)
