/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eappagc2.h : Header file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 2 of Export assistant (DBF, XML, FIXED WIDTH, COPY STATEMENT)
**
** History:
**
** 16-Jan-2002 (uk$so01)
**    Created
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**/

#if !defined(AFX_EAPPAGC2_H__DEAFC711_0A5D_11D6_879E_00C04F1F754A__INCLUDED_)
#define AFX_EAPPAGC2_H__DEAFC711_0A5D_11D6_879E_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "256bmp.h"
#include "schkmark.h"
#include "exportdt.h"
#include "eapstep2.h"

class CuIeaPPage2Common : public CPropertyPage, public CuIeaPPage2CommonData
{
	DECLARE_DYNCREATE(CuIeaPPage2Common)
public:
	CuIeaPPage2Common(); 
	~CuIeaPPage2Common(); 

	//{{AFX_DATA(CuIeaPPage2Common)
	enum { IDD = IDD_IEAPAGE2_COMMON };
	CButton m_cCheckExportHeader;
	CButton	m_cCheckReadLock;
	CStatic	m_cExportInfo;
	CButton	m_cButtonSave;
	CStatic	m_cStatic1;
	CuStaticCheckMark m_cStaticCheckMark;
	C256bmp m_bitmap;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuIeaPPage2Common)
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
	//{{AFX_MSG(CuIeaPPage2Common)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonSaveParameters();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EAPPAGC2_H__DEAFC711_0A5D_11D6_879E_00C04F1F754A__INCLUDED_)
