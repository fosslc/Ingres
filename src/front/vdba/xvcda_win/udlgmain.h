/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : udlgmain.h : header file
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main dialog for vcda
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
**/

//{{AFX_INCLUDES()
#include "vcdactrl.h"
//}}AFX_INCLUDES
#if !defined(AFX_UDLGMAIN_H__EAF345E4_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_UDLGMAIN_H__EAF345E4_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuVcdaControl: public CuVcda
{
public:
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	void HideFrame(short nShow);

	DECLARE_MESSAGE_MAP()
};


class CuDlgMain : public CDialog
{
public:
	CuDlgMain(CWnd* pParent = NULL); 
	void OnOK(){return;}
	void OnCancel(){return;}
	void DoCompare();
	void DoSaveInstallation();

	// Dialog Data
	//{{AFX_DATA(CuDlgMain)
	enum { IDD = IDD_MAIN };
	CComboBox	m_cComboFile1;
	CButton	m_cButtonSaveCurrent;
	CButton	m_cCaptionDifference;
	CButton	m_cButtonFile1;
	CButton	m_cButtonFile2;
	CEdit	m_cEditFile2;
	CuVcdaControl	m_cVcda;
	//}}AFX_DATA
	BOOL m_bCompared;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgMain)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:


	HICON m_hIconOpen;
	HICON m_hIconSave;
	HICON m_hIconNode;
	// Generated message map functions
	//{{AFX_MSG(CuDlgMain)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonSaveCurrentInstallation();
	afx_msg void OnButtonFile1();
	afx_msg void OnButtonFile2();
	afx_msg void OnButtonCompare();
	afx_msg void OnSelchangeComboFile1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UDLGMAIN_H__EAF345E4_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
