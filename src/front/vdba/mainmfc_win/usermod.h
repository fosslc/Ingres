/******************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Source   : usermod.h, Implementation file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : Schalk Philippe
**
**  Purpose  : defined Class to manage the dialog box for "usermod" utility.
**
**
**  11-Apr-2001 (schph01)
**      SIR 103292 Created
**	01-Sept-2004 (kodse01)
**		Changed #include "..\HDR\DBASET.H" to #include "DBASET.H"
******************************************************************************/
#if !defined(AFX_USERMOD_H__852AD8B3_2E56_11D5_B5FC_00C04F1790C3__INCLUDED_)
#define AFX_USERMOD_H__852AD8B3_2E56_11D5_B5FC_00C04F1790C3__INCLUDED_

#include "DBASET.H"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// usermod.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CxDlgUserMod dialog

class CxDlgUserMod : public CDialog
{
// Construction
public:
	int m_nNodeHandle;
	CString m_csDBName;
	CString m_csTableOwner;
	CString m_csTableName;
	CxDlgUserMod(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CxDlgUserMod)
	enum { IDD = IDD_USERMOD };
	CEdit	m_ctrlTblName;
	CButton	m_ctrlButtonTables;
	CButton	m_CheckSpecifTbl;
	CStatic	m_ctrlStaticTblName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgUserMod)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	AUDITDBTPARAMS m_lpTblParam;
	BOOL bChooseTableEnable;
	// Generated message map functions
	//{{AFX_MSG(CxDlgUserMod)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonTables();
	afx_msg void OnCheckSpecifTables();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL GenerateUsermodSyntax(CString& csCommandSyntax);
	void ExecuteRemoteCommand(LPCTSTR csCommandLine);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERMOD_H__852AD8B3_2E56_11D5_B5FC_00C04F1790C3__INCLUDED_)
