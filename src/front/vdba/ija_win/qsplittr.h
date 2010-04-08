/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsplittr.h: interface for the CuNoTrackableSplitterWnd class
**    Project  : Generic Control 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The splitter that can prevent user from tracking
**
** History:
**
** xx-Mar-1996 (uk$so01)
**    Created
** 18-Aug-2008 (whiro01)
**    Removed unnecessary include of <afxext.h>
**/

#if !defined (NO_TRACKABLE_SPLITTER)
#define NO_TRACKABLE_SPLITTER



class CuNoTrackableSplitterWnd : public CSplitterWnd
{
	DECLARE_DYNCREATE(CuNoTrackableSplitterWnd)
public:
	CuNoTrackableSplitterWnd();
	void SetTracking (BOOL bCanTrack = TRUE) {m_bCanTrack = bCanTrack;}
	BOOL GetTracking (){return m_bCanTrack;}
	BOOL ReplaceView(int row, int col, CRuntimeClass* pViewClass, SIZE size);
	//
	// Implementation
public:
	virtual ~CuNoTrackableSplitterWnd();
protected:
	BOOL m_bCanTrack;

	// Generated message map functions
	//{{AFX_MSG(CuNoTrackableSplitterWnd)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // NO_TRACKABLE_SPLITTER
