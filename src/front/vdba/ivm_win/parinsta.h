/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parinsta.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parameter Page for Installation branch
**
** History:
**
** 16-Dec-1998 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 06-Mar-2000 (uk$so01)
**    SIR #100751, Remove the button "Restore" of Parameter Tab.
**
**/

#if !defined(AFX_PARINSTA_H__5D002662_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)
#define AFX_PARINSTA_H__5D002662_94F4_11D2_A2B7_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "parinsys.h"
#include "parinusr.h"
#include "parinext.h"


class CuDlgParameterInstallation : public CDialog
{
public:
	CuDlgParameterInstallation(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgParameterInstallation)
	enum { IDD = IDD_PARAMETER_INSTALLATION };
	CButton	m_cCheckShowUnsetParameter;
	CButton	m_cButtonAdd;
	CButton	m_cButtonUnset;
	CButton	m_cButtonEditValue;
	CTabCtrl	m_cTab1;
	BOOL	m_bCheckShowUnsetParameter;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgParameterInstallation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuDlgParameterInstallationSystem* m_pDlgParameterSystem;
	CuDlgParameterInstallationUser* m_pDlgParameterUser;
	CuDlgParameterInstallationExtra* m_pDlgParameterExtra;
	CWnd* m_pCurrentPage;

	void SelchangeTab1(BOOL bUpdateData = TRUE);

	// Generated message map functions
	//{{AFX_MSG(CuDlgParameterInstallation)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonEditValue();
	afx_msg void OnButtonRestore();
	afx_msg void OnButtonUnset();
	afx_msg void OnButtonAdd();
	afx_msg void OnCheckShowUnsetParameter();
	//}}AFX_MSG
	afx_msg LONG OnEnableControl(WPARAM w, LPARAM l);
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARINSTA_H__5D002662_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)
