/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : secmech.h : header file
**    Project  : Configuration Manager 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog, Page (Mechanism) of Security
**
** History:
**
** 20-jun-2003 (uk$so01)
**    created.
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
*/

#if !defined(AFX_SECMECH_H__17EC0062_94D9_11D7_8839_00C04F1F754A__INCLUDED_)
#define AFX_SECMECH_H__17EC0062_94D9_11D7_8839_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "editlsct.h"
class CuEditableListCtrlMechanism : public CuEditableListCtrl
{
public:
	CuEditableListCtrlMechanism();

	// Overrides
	//
	virtual ~CuEditableListCtrlMechanism();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlNet)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlNet)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	DECLARE_MESSAGE_MAP()
};

class CuDlgSecurityMechanism : public CDialog
{
public:
	CuDlgSecurityMechanism(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgSecurityMechanism)
	enum { IDD = IDD_SECURITY_PAGE_MECHANISM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSecurityMechanism)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CPtrList m_listMechanism;
	CImageList m_ImageListCheck;
	CImageList m_ImageList;
	CuEditableListCtrlMechanism m_ListCtrl;

	void Cleanup();
	void Display();
	// Generated message map functions
	//{{AFX_MSG(CuDlgSecurityMechanism)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECMECH_H__17EC0062_94D9_11D7_8839_00C04F1F754A__INCLUDED_)
