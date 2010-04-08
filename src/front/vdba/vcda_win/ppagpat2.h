/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppagpat2.h : header file
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

#if !defined(AFX_PPAGPAT2_H__E8F3DE21_5922_11D7_8811_00C04F1F754A__INCLUDED_)
#define AFX_PPAGPAT2_H__E8F3DE21_5922_11D7_8811_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "256bmp.h"
// ppagpat2.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageRestorePatch2 dialog

class CuPropertyPageRestorePatch2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPropertyPageRestorePatch2)
public:
	CuPropertyPageRestorePatch2();
	~CuPropertyPageRestorePatch2();
	void SetPatches (LPCTSTR lpszPatch1, LPCTSTR lpszPatch2, LPCTSTR lpszVersion)
	{
		m_strPatch1 = lpszPatch1;
		m_strPatch2 = lpszPatch2;
		m_strVersion= lpszVersion;
	}
	// Dialog Data
	//{{AFX_DATA(CuPropertyPageRestorePatch2)
	enum { IDD = IDD_PROPPAGE_RESTORE_PATCH2 };
	CButton	m_cButtonAccessPatch;
	CButton	m_cButtonUninstallPatch;
	CListBox	m_cListPatch2;
	CListBox	m_cListPatch1;
	CStatic	m_cInfoVersion;
	CButton	m_cButtonRefresh;
	C256bmp	m_bitmap;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuPropertyPageRestorePatch2)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIconRefresh;
	CString m_strPatch1;
	CString m_strPatch2;
	CString m_strVersion;

	void FillPatchListBox (CListBox** pListBox, CString** PatchArr, int nDim);
	void GeneratePatchURL (LPCTSTR lpszPatchNum, CString& strURL);
	// Generated message map functions
	//{{AFX_MSG(CuPropertyPageRestorePatch2)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonUninstallPatch();
	afx_msg void OnButtonRefreshInstalledPatch();
	afx_msg void OnButtonAccessPatch();
	afx_msg void OnSelchangeList1();
	afx_msg void OnSelchangeList2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPAGPAT2_H__E8F3DE21_5922_11D7_8811_00C04F1F754A__INCLUDED_)
