/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lbfixedw.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Listbox control (owner drawn that allows to paint the divider
**               of column on it (fixed width control associated with ruler)
**
** History:
**
** 19-Dec-2000 (uk$so01)
**    Created
**/

#if !defined(AFX_LBFIXEDW_H__8D74155F_DBD9_11D4_8739_00C04F1F754A__INCLUDED_)
#define AFX_LBFIXEDW_H__8D74155F_DBD9_11D4_8739_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "staruler.h"


class CuListBoxFixedWidth : public CListBox
{
public:
	CuListBoxFixedWidth();
	void SetRuler(CuStaticRuler* pRuler){m_pRuler = pRuler;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListBoxFixedWidth)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuListBoxFixedWidth();
protected:
	int  CalcMinimumItemHeight();
	void PreDrawItem   (LPDRAWITEMSTRUCT    lpDrawItemStruct);
	void PreMeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	CuStaticRuler* m_pRuler;

	// Generated message map functions
protected:
	//{{AFX_MSG(CuListBoxFixedWidth)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LBFIXEDW_H__8D74155F_DBD9_11D4_8739_00C04F1F754A__INCLUDED_)
