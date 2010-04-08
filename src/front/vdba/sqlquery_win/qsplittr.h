/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsplittr.h, Header file
**    Project  : VDBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Splitter that can prevent user from tracking.
**
** History:
**
** xx-Mar-1996 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 21-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already in "stdafx.h"
*/

#if !defined (QSPLITTR_HEADER)
#define QSPLITTR_HEADER


class CuQueryMainSplitterWnd : public CSplitterWnd
{
	DECLARE_DYNCREATE(CuQueryMainSplitterWnd)
public:
	CuQueryMainSplitterWnd();
	void SetTracking (BOOL bCanTrack = TRUE) {m_bCanTrack = bCanTrack;}
	BOOL GetTracking (){return m_bCanTrack;}
	BOOL ReplaceView(int row, int col, CRuntimeClass* pViewClass, SIZE size);
	//
	// Implementation
public:
	virtual ~CuQueryMainSplitterWnd();
protected:
	BOOL m_bCanTrack;

	// Generated message map functions
	//{{AFX_MSG(CuQueryMainSplitterWnd)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // QSPLITTR_HEADER