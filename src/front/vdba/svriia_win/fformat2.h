/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fformat2.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Page of Tab (Fixed Width format)
**
** History:
**
** 26-Dec-2000 (uk$so01)
**    Created
**/


#if !defined(AFX_FFORMAT2_H__AE456E27_B3C5_11D4_8727_00C04F1F754A__INCLUDED_)
#define AFX_FFORMAT2_H__AE456E27_B3C5_11D4_8727_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "wnfixedw.h"


class CuDlgPageFixedWidth : public CDialog
{
	DECLARE_DYNCREATE(CuDlgPageFixedWidth)
public:
	CuDlgPageFixedWidth(CWnd* pParent = NULL);
	CuDlgPageFixedWidth(CWnd* pParent, BOOL bReadOnly);
	void OnOK() {return;}
	void OnCancel() {return;}
	void SetArrayPos(TCHAR* pArrayPos) {m_pArrayPos = pArrayPos;}
	
	CuWndFixedWidth& GetFixedWidthCtrl() {return m_wndFixedWidth;}

	// Dialog Data
	//{{AFX_DATA(CuDlgPageFixedWidth)
	enum { IDD = IDD_PROPPAGE_FORMAT_FIX };
	CButton	m_cCheckConfirmChoice;
	CStatic	m_cStatic1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPageFixedWidth)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL
protected:
	CuWndFixedWidth m_wndFixedWidth;
	TCHAR* m_pArrayPos;
	BOOL m_bReadOnly;

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgPageFixedWidth)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnCheckConfirmChoice();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnCleanData  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FFORMAT2_H__AE456E27_B3C5_11D4_8727_00C04F1F754A__INCLUDED_)
