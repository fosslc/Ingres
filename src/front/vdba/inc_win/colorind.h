/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : colorind.h, header file 
**    Project  : OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Static Control that paints itself with a given color
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 03-Sep-2001 (uk$so01)
**    Transform code to be an ActiveX Control
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vcbf uses libwctrl.lib).
*/

#if !defined(AFX_COLORIND_H__C1FB7D21_ADE2_11D1_A25B_00609725DDAF__INCLUDED_)
#define AFX_COLORIND_H__C1FB7D21_ADE2_11D1_A25B_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CuStaticColorWnd : public CStatic
{
public:
	CuStaticColorWnd();
	void SetPaletteAware(BOOL bSet){m_bPaletteAware = bSet;}

	//
	// Set the color of the static, by default, m_crColorPaint is NULL
	// and use the GetSysColoor (COLOR_BTNFACE) to draw the background
	void SetColor (COLORREF crColor)
	{
		m_crColorPaint = crColor;
		if (IsWindow (m_hWnd))
			Invalidate();
	}
	//
	// The same as SetColor, but the second parameter is use to draw the 2
	// small rectangles (look like a hatch fill):
	void SetColor (COLORREF crColor, COLORREF crColorHatch)
	{
		m_bHatch = TRUE;
		m_crColorHatch = crColorHatch;
		m_crColorPaint = crColor;
		if (IsWindow (m_hWnd))
			Invalidate();
	}
	void SetPalette(CPalette* p){m_Palette = p;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuStaticColorWnd)
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CuStaticColorWnd();

protected:
	int  m_nXHatchWidth;
	int  m_nXHatchHeight;
	BOOL m_bHatch;
	COLORREF m_crColorHatch;
	COLORREF m_crColorPaint;
	BOOL     m_bPaletteAware;
	CPalette* m_Palette;
	// Generated message map functions
protected:
	//{{AFX_MSG(CuStaticColorWnd)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//
// Special create font:
BOOL UxCreateFont (CFont& font, LPCTSTR lpFaceName, int size, int weight = FW_NORMAL, int bItalic= FALSE);

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORIND_H__C1FB7D21_ADE2_11D1_A25B_00609725DDAF__INCLUDED_)
