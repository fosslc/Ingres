/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppgropti.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: Option od restore (can be final step) of restore operation
**
** History:
**
** 18-Mar-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 03-May-2005 (komve01)
**    Issue#13921873/Bug#114438, Added message boxes for VNode Definitions 
**    and Restore Parameters and Variables with paths.
** 06-Jun-2005 (lakvi01)
**    If none of the check boxes are selected (in the screen 1 of the Restore wizard)
**	  disable the 'Next' button. For this purpose added OnBtnCheckConfigParams(),
**	  OnBtnCheckSysVariables(), OnBtnCheckUserVariables(), EnableNextButton() functions.
*/

#if !defined(AFX_PPGROPTI_H__1F989B71_5854_11D7_8810_00C04F1F754A__INCLUDED_)
#define AFX_PPGROPTI_H__1F989B71_5854_11D7_8810_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "256bmp.h"

class CuPropertyPageRestoreOption : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPropertyPageRestoreOption)

public:
	CuPropertyPageRestoreOption();
	~CuPropertyPageRestoreOption();
	void SetFinalStep(BOOL bSet){m_bFinalStep = bSet;}
	void SetBackStep(BOOL bSet){m_bNoPrevious = bSet;}

	// Dialog Data
	//{{AFX_DATA(CuPropertyPageRestoreOption)
	enum { IDD = IDD_PROPPAGE_RESTORE_OPTION };
	CStatic	m_cStaticNonEditVariableInfo;
	CListBox	m_cListROVariable;
	CEdit	m_cEditBackup;
	CButton	m_cCheckNotRestorePath;
	CButton	m_cCheckUserVariable;
	CButton	m_cCheckVNode;
	CButton	m_cCheckSystemVariable;
	CButton	m_cCheckParameter;
	CStatic	m_cIconWarning;
	C256bmp	m_bitmap;
	BOOL	m_bRestoreConfig;
	BOOL	m_bRestoreSystemVariable;
	BOOL	m_bRestoreVirtualNode;
	BOOL	m_bRestoreUserVariable;
	BOOL	m_bNotResorePath;
	CString	m_strBackupFile;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPropertyPageRestoreOption)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bFinalStep;
	BOOL m_bNoPrevious;

	BOOL CheckOverwriteBackupFile();
	// Generated message map functions
	//{{AFX_MSG(CuPropertyPageRestoreOption)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBtnCheckVNodeDefs();
	afx_msg void OnBtnCheckParams();
	afx_msg void OnBtnCheckConfigParams();
	afx_msg void OnBtnCheckSysVariables();
	afx_msg void OnBtnCheckUserVariables();
	void EnableNextButton();


};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPGROPTI_H__1F989B71_5854_11D7_8810_00C04F1F754A__INCLUDED_)
