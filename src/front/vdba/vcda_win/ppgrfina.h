/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppgrfina.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: Final step of restore operation
**
** History:
**
** 18-Mar-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
*/

#if !defined(AFX_PPGRFINA_H__1F989B72_5854_11D7_8810_00C04F1F754A__INCLUDED_)
#define AFX_PPGRFINA_H__1F989B72_5854_11D7_8810_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "256bmp.h"
#include "schkmark.h"
#include "calsctrl.h"

class CuPropertyPageRestoreFinal : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPropertyPageRestoreFinal)
public:
	CuPropertyPageRestoreFinal();
	~CuPropertyPageRestoreFinal();
	void SetSnapshot(LPCTSTR lpszFile){m_strSnapshot = lpszFile;}
	// Dialog Data
	//{{AFX_DATA(CuPropertyPageRestoreFinal)
	enum { IDD = IDD_PROPPAGE_RESTORE_FINAL };
	CListBox	m_cListBox;
	CuStaticCheckMark	m_cStaticCheckMark2;
	CuStaticCheckMark	m_cStaticCheckMark1;
	C256bmp	m_bitmap;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPropertyPageRestoreFinal)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuListCtrl m_cListCtrl;
	CString m_strSnapshot;

	// Generated message map functions
	//{{AFX_MSG(CuPropertyPageRestoreFinal)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPGRFINA_H__1F989B72_5854_11D7_8810_00C04F1F754A__INCLUDED_)
