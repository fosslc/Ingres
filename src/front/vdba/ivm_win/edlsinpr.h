/*
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : edlsinpr.h, Header File
**
**
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Editable List control to provide to Change the parameter of
**               Installation Branch (System and User)
**
**  History:
**	17-Dec-1998 (uk$so01)
**	    Created.
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
*/

#if !defined(EDLS_PARAMETER_INSTALLATION_HEADER)
#define EDLS_PARAMETER_INSTALLATION_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "editlsct.h"


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlInstallationParameter window

class CuEditableListCtrlInstallationParameter : public CuEditableListCtrl
{
public:
	enum {C_PARAMETER, C_VALUE, C_DESCRIPTION};
	CuEditableListCtrlInstallationParameter();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlInstallationParameter)
	//}}AFX_VIRTUAL

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam) { return OnEditDlgOK(wParam, lParam);}
	virtual LONG ManageEditNumberDlgOK (UINT wParam, LONG lParam){ return OnEditNumberDlgOK(wParam, lParam);}
	virtual LONG ManageEditSpinDlgOK (UINT wParam, LONG lParam){return OnEditSpinDlgOK(wParam, lParam);}
	virtual LONG ManageComboDlgOK (UINT wParam, LONG lParam){return OnComboDlgOK(wParam, lParam);}

	void DisplayParameter(int nItem = -1);
	void SetAllowEditing(BOOL bAllow = TRUE){m_bAllowEditing = bAllow;}
	BOOL IsAllowEditing(){return m_bAllowEditing;}

	//
	// Implementation
public:
	virtual ~CuEditableListCtrlInstallationParameter();

	//
	// Generated message map functions
protected:
	BOOL m_bAllowEditing;

	//{{AFX_MSG(CuEditableListCtrlInstallationParameter)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditAssistant (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEditNumberDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgCancel (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgCancel (UINT wParam, LONG lParam);
	afx_msg LONG OnComboDlgOK (UINT wParam, LONG lParam);
	
	
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif 
