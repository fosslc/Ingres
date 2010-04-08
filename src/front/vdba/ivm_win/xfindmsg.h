/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xfindmsg.h : header file
**    Project  : Ingres II/IVM
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Popup dialog for asking the input (message) for searching.
**
** History:
**
** 06-Mar-2003 (uk$so01)
**    Created
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#if !defined(AFX_XFINDMSG_H__514FE9C3_4D5D_11D7_880A_00C04F1F754A__INCLUDED_)
#define AFX_XFINDMSG_H__514FE9C3_4D5D_11D7_880A_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxDlgFindMessage : public CDialog
{
public:
	CxDlgFindMessage(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgFindMessage)
	enum { IDD = IDD_XFINDMESSAGE };
	CString	m_strMessage;
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgFindMessage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgFindMessage)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XFINDMSG_H__514FE9C3_4D5D_11D7_880A_00C04F1F754A__INCLUDED_)
