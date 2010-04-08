/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eappage1.h : header file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 1 of Export assistant
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Date conversion and numeric display format.
**/


#if !defined(AFX_EAPPAGE1_H__9DAD9541_C17F_11D5_8784_00C04F1F754A__INCLUDED_)
#define AFX_EAPPAGE1_H__9DAD9541_C17F_11D5_8784_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "256bmp.h"
#include "ieatree.h"
#include "exportdt.h"


class CuIeaPPage1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuIeaPPage1)
public:
	CuIeaPPage1();
	~CuIeaPPage1();

	//{{AFX_DATA(CuIeaPPage1)
	enum { IDD = IDD_IEAPAGE1 };
	CButton	m_cCheckF8Display;
	CButton	m_cCheckF4Display;
	CSpinButtonCtrl	m_cSpinF8Scale;
	CSpinButtonCtrl	m_cSpinF8Precision;
	CSpinButtonCtrl	m_cSpinF4Scale;
	CSpinButtonCtrl	m_cSpinF4Precision;
	CEdit	m_cEditF8Scale;
	CEdit	m_cEditF8Precision;
	CEdit	m_cEditF4Scale;
	CEdit	m_cEditF4Precision;
	CComboBox	m_cComboFormat;
	CEdit	m_cEditSql;
	CEdit	m_cEditFile2BeCreated;
	CButton	m_cButtonSqlAssistant;
	CButton	m_cButton3Periods;
	C256bmp m_bitmap;
	CTreeCtrl	m_cTreeIngresDatabase;
	CButton	m_cButtonLoad;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuIeaPPage1)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bAllowExpandTree;
	CImageList m_imageList;
	HICON m_hIcon;
	HICON m_hIconAssistant;
	CString m_strPageTitle;
	CaIexportTree m_ieaTree;


	BOOL CanGoNext();
	void FillComboFormat();
	void UpdateExponentialFormat();
	ExportedFileType VerifyFileFormat (CString& strFullName, CString& strExt, int nFilterIndex = -1);
	UINT CollectData(CaIEAInfo* pData);
	void UpdateFloatPreferences(CaFloatFormat* pFloatFormat);
	void Loading(LPCTSTR lpszFile);

	// Generated message map functions
	//{{AFX_MSG(CuIeaPPage1)
	afx_msg void OnButtonLoadParameter();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonFile2BeCreated();
	afx_msg void OnButtonSqlAssistant();
	afx_msg void OnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpandedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboFileFormat();
	afx_msg void OnChangeEditF4Precision();
	afx_msg void OnChangeEditF4Scale();
	afx_msg void OnChangeEditF8Precision();
	afx_msg void OnChangeEditF8Scale();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EAPPAGE1_H__9DAD9541_C17F_11D5_8784_00C04F1F754A__INCLUDED_)
