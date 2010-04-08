/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   
*/

/*
**    Source   : pagediff.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of tab control to display the list of differences
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
*/

#if !defined(AFX_PAGEDIFF_H__EAF345FD_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_PAGEDIFF_H__EAF345FD_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "calsctrl.h"

class CuDlgPageDifference : public CDialog
{
public:
	CuDlgPageDifference(CWnd* pParent = NULL);
	CuDlgPageDifference(BOOL bVirtualNodePage, CWnd* pParent = NULL);
	CListCtrl* GetListCtrl(){return &m_cListCtrl;}

	// Dialog Data
	//{{AFX_DATA(CuDlgPageDifference)
	enum { IDD = IDD_PAGE_DIFFERENCE };
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPageDifference)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageList;
	BOOL       m_bVirtualNodePage; // TRUE, this page is used for vnode.

	void Init();
	// Generated message map functions
	//{{AFX_MSG(CuDlgPageDifference)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg long OnUpdateData(WPARAM w, LPARAM l);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEDIFF_H__EAF345FD_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
