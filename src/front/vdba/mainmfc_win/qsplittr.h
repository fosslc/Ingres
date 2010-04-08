/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : qsplittr.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  :                                                                       //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Splitter that can prevent user from tracking.                         //
//                                                                                     //
****************************************************************************************/


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

