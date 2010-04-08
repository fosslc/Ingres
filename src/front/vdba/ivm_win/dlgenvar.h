/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dlgenvar.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Add extra Variable Environment 
**
** History:
**
** 01-Sep-1999 (uk$so01)
**    Created
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**
**/

#if !defined(AFX_DLGENVAR_H__ECCC0012_5EB2_11D3_A305_00609725DDAF__INCLUDED_)
#define AFX_DLGENVAR_H__ECCC0012_5EB2_11D3_A305_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CxEnvironmentVariable : public CDialog
{
public:
	CxEnvironmentVariable(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CxEnvironmentVariable)
	enum { IDD = IDD_XENVIRONMENT_VARIABLE };
	CButton	m_cButtonOK;
	CEdit	m_cEditValue;
	CEdit	m_cEditName;
	CString	m_strName;
	CString	m_strValue;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxEnvironmentVariable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void EnableControls();

	// Generated message map functions
	//{{AFX_MSG(CxEnvironmentVariable)
	afx_msg void OnChangeEditName();
	afx_msg void OnChangeEditValue();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGENVAR_H__ECCC0012_5EB2_11D3_A305_00609725DDAF__INCLUDED_)
