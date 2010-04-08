/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : msgtreec.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Visual Manager                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Define Message Categories and Notification Levels                     //
//               Defenition of Tree Node of Message Category (Visual Design)           //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#if !defined(AFX_MSGTREEC_H__D636D274_14C9_11D3_A2EF_00609725DDAF__INCLUDED_)
#define AFX_MSGTREEC_H__D636D274_14C9_11D3_A2EF_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CfEventSetting;
class CaMessageNode;
class CuMessageCategoryTreeCtrl : public CTreeCtrl
{
public:
	CuMessageCategoryTreeCtrl();

	void SetDropTarget (BOOL bDropTarget){m_bCanbeDropTarget = bDropTarget;}
	void DropItem();


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuMessageCategoryTreeCtrl)
	//}}AFX_VIRTUAL


	// Implementation
public:
	virtual ~CuMessageCategoryTreeCtrl();

	enum {dropAllowed = 100};
//	void SetCategory(BOOL bPredefinedCategory){m_bPredefinedCategory = bPredefinedCategory;}
//	void SetTreeData (CaTreeEventCategory* pTreeData){m_pTreeEventCategoryData = pTreeData;}


	CImageList* m_pImagelist;

protected:
	CfEventSetting* m_pEventSettingFrame;
	BOOL m_bDragging;
	BOOL m_bCanbeDropTarget;
	HTREEITEM m_hitemDrag;
	HTREEITEM m_hitemDrop;

	void OnButtonUp(CPoint point);
	BOOL MoveItems (CaMessageNode* pNodeSource, CaMessageNode* pNodeTarget, CTreeCtrl* pTree);
	BOOL MoveSingleItem (CaMessageNode* pNodeSource, CaMessageNode* pNodeTarget, CTreeCtrl* pTree);
	BOOL MoveMultipleItems (CaMessageNode* pNodeSource, CaMessageNode* pNodeTarget, CTreeCtrl* pTree);
	void EnableDisableMenuItem (CaMessageNode* pItemData, CMenu* pPopup);

	// Generated message map functions
protected:
	//{{AFX_MSG(CuMessageCategoryTreeCtrl)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnAddFolder();
	afx_msg void OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNotifyMessage();
	afx_msg void OnAlertMessage();
	afx_msg void OnDiscardMessage();
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDropFolder();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGTREEC_H__D636D274_14C9_11D3_A2EF_00609725DDAF__INCLUDED_)
