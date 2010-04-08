/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parinsys.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : System Parameter Page for Installation branch 
**
** History:
**
** 17-Dec-1998 (uk$so01)
**    created
** 29-Feb-2000 (uk$so01)
**    SIR # 100634
**    The "Add" button of Parameter Tab can be used to set 
**    Parameters for System and User Tab.
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/


#if !defined(AFX_PARINSYS_H__5D002664_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)
#define AFX_PARINSYS_H__5D002664_94F4_11D2_A2B7_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "edlsinpr.h"
#include "compdata.h"

class CuDlgParameterInstallationSystem : public CDialog
{
public:
	CuDlgParameterInstallationSystem(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}
	void RefreshParameter(BOOL bShowUnset);

	// Dialog Data
	//{{AFX_DATA(CuDlgParameterInstallationSystem)
	enum { IDD = IDD_PARAMETER_INSTALLATION_SYSTEM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgParameterInstallationSystem)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuEditableListCtrlInstallationParameter m_cListCtrl;
	CTypedPtrList<CObList, CaEnvironmentParameter*> m_listParameter;
	CImageList m_ImageList;
	BOOL m_bShowUnset;
	SORTPARAMS m_sortparam;

	void SetUnsetVariable (BOOL bUnset = FALSE);

	// Generated message map functions
	//{{AFX_MSG(CuDlgParameterInstallationSystem)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	afx_msg LONG OnListCtrlEditing (WPARAM w, LPARAM l);
	afx_msg LONG OnListCtrlEditOK(WPARAM w, LPARAM l);
	afx_msg LONG OnUnsetVariable(WPARAM w, LPARAM l);
	afx_msg LONG OnEnableControl(WPARAM w, LPARAM l);
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnAddParameter (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARINSYS_H__5D002664_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)
