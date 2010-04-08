/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edlsinsv.h, Header File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Insert Page 2)
**               Editable List control to provide the Insert Values
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/


#if !defined(EDLSINSV_HEADER)
#define EDLSINSV_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "editlsct.h"


class CuEditableListCtrlSqlWizardInsertValue : public CuEditableListCtrl
{
public:
	enum {COLUMN, VALUE};
	CuEditableListCtrlSqlWizardInsertValue();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlSqlWizardInsertValue)
	//}}AFX_VIRTUAL

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	virtual LONG ManageEditDlgOK       (UINT wParam, LONG lParam) {return OnEditDlgOK(wParam, lParam);}      
	CaLLQueryInfo m_queryInfo;

	//
	// Implementation
public:
	virtual ~CuEditableListCtrlSqlWizardInsertValue();

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlSqlWizardInsertValue)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditAssistant (UINT wParam, LONG lParam);

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif 
