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
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 09-Apr-2003 (uk$so01)
**    SIR #109718, Enhance the library. Add UserDragged(), it will return TRUE
**    if user has dragged the splitter bar.
** 22-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already included in every "stdafx.h"
*/

#if !defined (QSPLITTR_HEADER)
#define QSPLITTR_HEADER


class CuUntrackableSplitterWnd : public CSplitterWnd
{
	DECLARE_DYNCREATE(CuUntrackableSplitterWnd)
public:
	CuUntrackableSplitterWnd();
	void SetTracking (BOOL bCanTrack = TRUE) {m_bCanTrack = bCanTrack;}
	BOOL GetTracking (){return m_bCanTrack;}
	BOOL ReplaceView(int row, int col, CRuntimeClass* pViewClass, SIZE size);
	BOOL UserDragged(){return m_bUserDrags;}
	//
	// Implementation
public:
	virtual ~CuUntrackableSplitterWnd();
protected:
	BOOL m_bCanTrack;
	BOOL m_bUserDrags;

	// Generated message map functions
	//{{AFX_MSG(CuUntrackableSplitterWnd)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // QSPLITTR_HEADER