/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : wnfixedw.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Ruler control that allow user to click on the ruler to divide
**               the columns:
**
** History:
**
** 29-Dec-2000 (uk$so01)
**    Created
**/


#if !defined(AFX_WNFIXEDW_H__8D741561_DBD9_11D4_8739_00C04F1F754A__INCLUDED_)
#define AFX_WNFIXEDW_H__8D741561_DBD9_11D4_8739_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "hdfixedw.h"
#include "lbfixedw.h"
#include "staruler.h"


class CuWndFixedWidth : public CWnd
{
	DECLARE_DYNCREATE(CuWndFixedWidth)
public:
	CuWndFixedWidth();
	void SetData(CStringArray& arrayLine, int nStartIndex = 0);
	void Display(int nPos = 0);
	int  GetMinStringLength(){return m_nMinLen;}
	int  GetMaxStringLength(){return m_nMaxLen;}

	CuListBoxFixedWidth& GetListBox(){return m_listbox;}
	CScrollBar& GetHorzScrollBar(){return m_horzScrollBar;}
	void AddDivider (CPoint p);
	void Cleanup();
	void ResizeControls();

	void SetReadOnly(BOOL bSet){m_bReadOnly = bSet;}
	BOOL GetReadOnly(){return m_bReadOnly;}
	int  GetColumnDividerCount();
	void GetColumnPositions(int*& pArrayPosition, int& nSize);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuWndFixedWidth)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuWndFixedWidth();
protected:
	int m_nMaxLen;
	int m_nMinLen;
	int m_nItemsPerPage;
	int m_nStartIndex;
	CStringArray* m_pArrayLine;
	BOOL m_bReadOnly;
	CFont m_font;
	int m_nCurrentHScrollPos;
	int m_nCurrentVScrollPos;
	CSize m_sizeChar;
	CString m_strMsgScrollTo;

	CuHeaderFixedWidth m_header;
	CuStaticRuler m_ruler;
	CuListBoxFixedWidth m_listbox;
	CScrollBar m_horzScrollBar;
	CScrollBar m_vertScrollBar;
	SCROLLINFO m_scrollInfo;
	BOOL m_bDrawInfo;
	BOOL m_bTrackPos;
	CPoint m_pointInfo;

	//
	// Drag & Drop divider:
	BOOL m_bDragging;
	CaHeaderMarker* m_pHeaderMarker; // Header marker to be moved:
	CPen*  m_pXorPen;
	CPoint m_pMove1;
	CPoint m_pMove2;
	CRect  m_rectListBox;

	void DrawThumbTrackInfo (int nPos, BOOL bErase = FALSE);
	void ScrollLineUp();
	void ScrollLineDown();
	void ScrollPageUp();
	void ScrollPageDown();


	// Generated message map functions
protected:
	//{{AFX_MSG(CuWndFixedWidth)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WNFIXEDW_H__8D741561_DBD9_11D4_8739_00C04F1F754A__INCLUDED_)
