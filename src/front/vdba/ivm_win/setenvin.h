/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : setenvin.h : header file
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Set ingres parameter dialog
**
** History:
**
** 29-Feb-2000 (uk$so01)
**    Created. (SIR # 100634)
**    The "Add" button of Parameter Tab can be used to set 
**    Parameters for System and User Tab. 
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/


#if !defined(AFX_SETENVIN_H__842EE886_E914_11D3_A347_00C04F1F754A__INCLUDED_)
#define AFX_SETENVIN_H__842EE886_E914_11D3_A347_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxDlgSetEnvironmentVariableIngres : public CDialog
{
public:
	enum {EPARAM_SYSTEM, EPARAM_USER};
	CxDlgSetEnvironmentVariableIngres(CWnd* pParent = NULL);
	void SetParameterType(int nType = EPARAM_SYSTEM){m_nParamType = nType;}
	BOOL FillEnvironment();

	// Dialog Data
	//{{AFX_DATA(CxDlgSetEnvironmentVariableIngres)
	enum { IDD = IDD_XSETENVIRONMENT_INGRES };
	CButton	m_cButtonOK;
	CEdit	m_cEdit1;
	CComboBoxEx	m_cComboEx1;
	CString	m_strName;
	CString	m_strValue;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgSetEnvironmentVariableIngres)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_imageList;
	int m_nParamType; // = EPARAM_SYSTEM (default)
	CTypedPtrList<CObList, CaEnvironmentParameter*> m_listEnvirenment;

	void EnableOKButton();

	// Generated message map functions
	//{{AFX_MSG(CxDlgSetEnvironmentVariableIngres)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnChangeEdit1();
	afx_msg void OnSelchangeComboboxex1();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETENVIN_H__842EE886_E914_11D3_A347_00C04F1F754A__INCLUDED_)
