/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : comborgb.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Owner draw combo for the RGB Item

** History:
**
** 29-Mar-2000 (uk$so01)
**    Created
**
**/


#if !defined(AFX_COMBORGB_H__44C9BC03_056E_11D4_A352_00C04F1F754A__INCLUDED_)
#define AFX_COMBORGB_H__44C9BC03_056E_11D4_A352_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CuComboColor : public CComboBox
{
public:
	CuComboColor();
	int AddRgb (COLORREF rgb);
	COLORREF GetRgb ();
	void SelectItem (COLORREF rgb);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuComboColor)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuComboColor();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuComboColor)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMBORGB_H__44C9BC03_056E_11D4_A352_00C04F1F754A__INCLUDED_)
