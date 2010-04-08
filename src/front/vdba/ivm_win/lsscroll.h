/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : lsscroll.cpp, implementation file
**    Project  : Ingres Visual Manager.
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Owner draw LISTCTRL to have a custom V Scroll
**
** History:
**
** 23-Aug-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 21-Jul-2000 (uk$so01)
**    BUG #102152. Replace the CScrollBar* by the custom CuVerticalScroll*
**
**/


#if !defined(AFX_LSSCROLL_H__23DD3B71_A12A_11D1_A24A_00609725DDAF__INCLUDED_)
#define AFX_LSSCROLL_H__23DD3B71_A12A_11D1_A24A_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"
#include "ivmsgdml.h"
#include "uvscroll.h"

class CaLoggedEvent;
class CuListCtrlVScrollEvent : public CuListCtrl
{
public:
	CuListCtrlVScrollEvent();

	void SetCtrlType(BOOL bOwnerDraw){m_bOwnerDraw = bOwnerDraw;}
	BOOL IsOwnerDraw(){return m_bOwnerDraw;}
	void InitializeItems(int nHeader = -1);
	BOOL NextItems();
	BOOL BackItems();
	BOOL MoveAt(int nPos = 0);
	void Sort(LPARAM lParamSort, PFNEVENTCOMPARE compare);

	void SetListEvent(CTypedPtrList<CObList, CaLoggedEvent*>* pListEvent);
	void RemoveEvent (CaLoggedEvent* pEvent);

	void SetCustopmVScroll(CuVerticalScroll* pVcroll){m_pCustomVScroll = pVcroll;}
	CObArray& GetArrayEvent() {return m_arrayEvent;}

	BOOL IsListCtrlScrollManagement(){return m_bScrollManagement;}
	void SetListCtrlScrollManagement(BOOL bScroll){m_bScrollManagement = bScroll;}
	void DrawThumbTrackInfo (UINT nPos, BOOL bErase = FALSE);
	void SetControlMaxEvent (BOOL bControl) {m_bControlMaxEvent = bControl;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlVScrollEvent)
	//}}AFX_VIRTUAL
	virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void AddEvent (CaLoggedEvent* pEvent, int nIndex = -1);
	LONG FindText(WPARAM wParam, LPARAM lParam);

	//
	// Implementation
public:
	virtual ~CuListCtrlVScrollEvent();

	// Generated message map functions
protected:
	BOOL m_bDisableGetItems;
	CObArray m_arrayEvent;

	BOOL m_bOwnerDraw;
	BOOL m_bControlMaxEvent;
	CuVerticalScroll* m_pCustomVScroll;
	BOOL m_bScrollManagement;
	POINT m_pointInfo;
	BOOL m_bDrawInfo;

	//{{AFX_MSG(CuListCtrlVScrollEvent)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSSCROLL_H__23DD3B71_A12A_11D1_A24A_00609725DDAF__INCLUDED_)
