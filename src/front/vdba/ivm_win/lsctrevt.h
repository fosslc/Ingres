/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : lsctrevt.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : List Control for Message setting (bottom pane)                        //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-June-1999 created)
*
*
*/

#if !defined(AFX_LSCTREVT_H__62D046D2_2323_11D3_A2F3_00609725DDAF__INCLUDED_)
#define AFX_LSCTREVT_H__62D046D2_2323_11D3_A2F3_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "lsscroll.h"

class CaMessageNode;
class CfEventSetting;
class CaLoggedEvent;
class CaMessageNodeTree;
class CuListCtrlLoggedEvent : public CuListCtrlVScrollEvent
{
public:
	CuListCtrlLoggedEvent();
	void SetIngresgenegicMessage(){m_bIngresGenericMessage = TRUE;}
//	void QuickSort(LPARAM lParamSort);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlLoggedEvent)
	//}}AFX_VIRTUAL

	//
	// Specific for the list of events (full description)
	virtual void AddEvent (CaLoggedEvent* pEvent, int nIndex = -1);

	// Implementation
public:
	virtual ~CuListCtrlLoggedEvent();
protected:
	BOOL m_bIngresGenericMessage;
	BOOL m_bDragging;
	int  m_iItemDrag;
	int  m_iItemDrop;
	CPoint m_ptHotSpot;
	CPoint m_ptOrigin;
	CSize  m_sizeDelta;
	CImageList* m_pImageListDrag;
	CfEventSetting* m_pEventSettingFrame;

	void OnButtonUp(CPoint point);

protected:
	void NewUserMessage (
		int nPane, 
		UINT nAction, 
		CTreeCtrl* pTree, 
		CaMessageNode* pNodeTarget, 
		CListCtrl* pListCtrl);

	BOOL NewMessageRetainState (CaMessageNode* pNodeTarget, CaLoggedEvent* pMsg);
	void NewMessageNewState    (CaMessageNode* pNodeTarget, CaLoggedEvent* pMsg, CTreeCtrl* pTree);
	CaMessageNodeTree* CreateMessageNode(CaMessageManager* pMsgEntry, CaLoggedEvent* pMsg);

	// Generated message map functions
protected:
	//{{AFX_MSG(CuListCtrlLoggedEvent)
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSCTREVT_H__62D046D2_2323_11D3_A2F3_00609725DDAF__INCLUDED_)
