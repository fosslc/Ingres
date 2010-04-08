/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

#if !defined(AFX_ICEROLE_H__516E09E2_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
#define AFX_ICEROLE_H__516E09E2_5CEF_11D2_9727_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/*
**
**  Name: icerole.h
**
**  Description:
**	This file contains the definitions for the CxDlgIceRole class.
**
**  History:
**	16-feb-2000 (somsa01)
**	    Properly declared EnableDisableOKbutton().
*/

extern "C"
{
#include "main.h"
#include "monitor.h"
#include "domdata.h"
#include "getdata.h"
#include "ice.h"
#include "msghandl.h"

}
/////////////////////////////////////////////////////////////////////////////
// CxDlgIceRole dialog

class CxDlgIceRole : public CDialog
{
// Construction
public:
	CxDlgIceRole(CWnd* pParent = NULL);   // standard constructor
	LPICEROLEDATA m_pStructRoleInfo;
	CString m_csVirtNodeName;
// Dialog Data
	//{{AFX_DATA(CxDlgIceRole)
	enum { IDD = IDD_ICE_ROLE };
	CEdit	m_edRoleComment;
	CEdit	m_ctrlIceRoleName;
	CButton	m_ctrlOK;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceRole)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceRole)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeIceRoleName();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void EnableDisableOKbutton();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEROLE_H__516E09E2_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
