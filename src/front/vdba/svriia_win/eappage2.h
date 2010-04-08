/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eappage2.h : header file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 2 of Export assistant
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**    Created
**/

#if !defined(AFX_EAPPAGE2_H__9DAD9542_C17F_11D5_8784_00C04F1F754A__INCLUDED_)
#define AFX_EAPPAGE2_H__9DAD9542_C17F_11D5_8784_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "256bmp.h"
#include "schkmark.h"
#include "exportdt.h"
#include "eapstep2.h"


class CuIeaPPage2 : public CPropertyPage, public CuIeaPPage2CommonData
{
	DECLARE_DYNCREATE(CuIeaPPage2)
public:
	CuIeaPPage2(); 
	~CuIeaPPage2(); 

	//{{AFX_DATA(CuIeaPPage2)
	enum { IDD = IDD_IEAPAGE2_CSV };
	CButton	m_cCheckReadLock;
	CButton	m_cCheckExportHeader;
	CButton	m_cCheckQuoteIfNeeded;
	CComboBox	m_cComboQuote;
	CComboBox	m_cComboDelimiter;
	CButton	m_cButtonSave;
	CStatic	m_cStatic1;
	CuStaticCheckMark m_cStaticCheckMark;
	C256bmp m_bitmap;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuIeaPPage2)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	HICON m_hIcon;
	CString m_strPageTitle;

	void CollectData(CaIEAInfo* pData);
	// Generated message map functions
	//{{AFX_MSG(CuIeaPPage2)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonSaveParameters();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EAPPAGE2_H__9DAD9542_C17F_11D5_8784_00C04F1F754A__INCLUDED_)
