/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : treestat.h : header file
**    Project  : libwctrl.lib 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Tree Control that supports the state image list.
**               the CCheckListBox.
**
** History:
**
** 17-Jul-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/


#if !defined(AFX_TREESTAT_H__EA544EF4_7A89_11D5_875F_00C04F1F754A__INCLUDED_)
#define AFX_TREESTAT_H__EA544EF4_7A89_11D5_875F_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuTreeStateIcon : public CTreeCtrl
{
public:
	CuTreeStateIcon();
	BOOL IsItemChecked(HTREEITEM hItem);
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuTreeStateIcon)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuTreeStateIcon();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuTreeStateIcon)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TREESTAT_H__EA544EF4_7A89_11D5_875F_00C04F1F754A__INCLUDED_)
