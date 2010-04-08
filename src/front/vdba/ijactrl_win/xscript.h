/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : xscript.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of Redo Dialog Box
**
** History:
**
** 28-Mar-1999 (uk$so01)
**    created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Remove the class CuStaticResizeMark and the bitmap sizemark.bmp.bmp
**/

#if !defined(AFX_XSCRIPT_H__D3246333_03F5_11D4_A352_00C04F1F754A__INCLUDED_)
#define AFX_XSCRIPT_H__D3246333_03F5_11D4_A352_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxScript : public CDialog
{
public:
	CxScript(CWnd* pParent = NULL);
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CxScript)
	enum { IDD = IDD_XEDITTEXT };
	CButton	m_cButtonSave;
	CButton	m_cCheckChangeScript;
	CButton	m_cButtonCancel;
	CButton	m_cButtonRun;
	CEdit	m_cEdit1;
	CString	m_strEdit1;
	//}}AFX_DATA
	CString m_strTitle;
	BOOL m_bReadOnly;
	CString m_strNode;
	CString m_strDatabase;
	BOOL m_bCall4Redo;
	UINT m_nHelpID;
	//
	// This boolean inherited from the Recover or Redo Dialog box.
	// The caller (Redo/Recover) must set this parameter:
	BOOL m_bErrorOnAffectedRowCount;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxScript)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	HICON m_hIcon;

	void ResizeControls();
	// Generated message map functions
	//{{AFX_MSG(CxScript)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnCheckChangeScript();
	afx_msg void OnButtonSave();
	afx_msg void OnButtonRun();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XSCRIPT_H__D3246333_03F5_11D4_A352_00C04F1F754A__INCLUDED_)
