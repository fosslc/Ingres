/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppagpat1.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: Preliminary Step - Uninstall patches
**
** History:
**
** 18-Mar-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
*/


#ifndef __PPAGPAT1_H__
#define __PPAGPAT1_H__
#include "256bmp.h"

class CuPropertyPageRestorePatch1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPropertyPageRestorePatch1)

public:
	CuPropertyPageRestorePatch1();
	~CuPropertyPageRestorePatch1();
	void SetMessage(LPCTSTR lpszText){m_strMsg = lpszText;}
	// Dialog Data
	//{{AFX_DATA(CuPropertyPageRestorePatch1)
	enum { IDD = IDD_PROPPAGE_RESTORE_PATCH1 };
	CStatic	m_cIconWarning;
	C256bmp m_bitmap;
	CString	m_strMsg;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPropertyPageRestorePatch1)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CuPropertyPageRestorePatch1)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



#endif // __PPAGPAT1_H__
