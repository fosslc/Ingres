/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   
*/

/*
**    Source   : calsmain.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : List of differences for the main parameters
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 22-Apr-2005 (komve01)
**    BUG #114371/Issue#13864404, Added a handler for WM_RBUTTONDOWN to prevent 
**    the parameters being edited and avoid the default behavior of the parent class.
*/

#if !defined(AFX_CALSMAIN_H__B09E5334_DABB_11D6_87EB_00C04F1F754A__INCLUDED_)
#define AFX_CALSMAIN_H__B09E5334_DABB_11D6_87EB_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "editlsct.h"


class CuEditableListCtrlMain : public CuEditableListCtrl
{
public:
	CuEditableListCtrlMain();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlMain)
	//}}AFX_VIRTUAL
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);

	// Implementation
public:
	virtual ~CuEditableListCtrlMain();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlMain)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALSMAIN_H__B09E5334_DABB_11D6_87EB_00C04F1F754A__INCLUDED_)
