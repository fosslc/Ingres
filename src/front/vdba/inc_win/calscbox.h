/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : calscbox.h : header file
**    Project  : Owner Drawn window control 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Make the CListCtrl to have a check box brhaviour like 
**               the CCheckListBox.
**
** History:
**
** 27-Apr-2001 (uk$so01)
**    Created for SIR #102678
**    Support the composite histogram of base table and secondary index.
**/


#if !defined(AFX_CALSCBOX_H__04B6E086_3988_11D5_874F_00C04F1F754A__INCLUDED_)
#define AFX_CALSCBOX_H__04B6E086_3988_11D5_874F_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "calsctrl.h"

class CuListCtrlCheckBox : public CuListCtrl
{
public:
	CuListCtrlCheckBox();
	void CheckItem(int nNewCheckedItem); // Check = !Check
	void CheckItem(int nNewCheckedItem, BOOL bCheck);
	BOOL GetItemChecked(int nItem);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlCheckBox)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuListCtrlCheckBox();

	// Generated message map functions
protected:
	BOOL m_bStateIcons;

	//{{AFX_MSG(CuListCtrlCheckBox)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALSCBOX_H__04B6E086_3988_11D5_874F_00C04F1F754A__INCLUDED_)
