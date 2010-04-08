/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : addvnode.h : header file
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Add VNode (in VNode Page of Bridge)
** 
** History:
**
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf, Created 
** 30-Apr-2004 (uk$so01)
**    SIR #111701, New Help MAP ID is available.
*/

#if !defined(AFX_ADDVNODE_H__15B176F5_58DD_4585_AE86_C088D08965A5__INCLUDED_)
#define AFX_ADDVNODE_H__15B176F5_58DD_4585_AE86_C088D08965A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxDlgAddVNode : public CDialog
{
public:
	CxDlgAddVNode(CWnd* pParent = NULL); 

	// Dialog Data
	//{{AFX_DATA(CxDlgAddVNode)
	enum { IDD = IDD_XVNODE };
	CEdit	m_cListenAddress;
	CEdit	m_cVnode;
	CComboBox	m_cComboProtocol;
	int		m_nIndex;
	CString	m_strVNode;
	CString	m_strListenAddress;
	//}}AFX_DATA
	CString m_strProtocol;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgAddVNode)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgAddVNode)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDVNODE_H__15B176F5_58DD_4585_AE86_C088D08965A5__INCLUDED_)
