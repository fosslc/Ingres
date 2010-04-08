/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : treenode.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Virtual Node Manager.                                                 //
//    Author   : UK Sotheavut,                                                         //
//                                                                                     //
//    Purpose  : Root node of Tree Node Data (Implement as Template list of Objects)   //
****************************************************************************************/
#if !defined(AFX_TREENODE_H__2D0C26D6_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_TREENODE_H__2D0C26D6_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "vnode.h"

class CaTreeNodeFolder : public CaVNodeTreeItemData  
{
public:
	CaTreeNodeFolder();
	virtual ~CaTreeNodeFolder();
	virtual BOOL IsAddEnabled(){return TRUE;}
	BOOL Refresh();
	void Display (CTreeCtrl* pTree);
	BOOL OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand);

	virtual BOOL Add();
	CaVNodeTreeItemData* FindNode (LPCTSTR lpszNodeName);

protected:

	CString m_strRoot;
	CTypedPtrList<CObList, CaVNodeTreeItemData*> m_listNode;
	CaTreeNodeInformation m_nodeInfo;
	CaNodeDummy m_DummyItem;

};

#endif // !defined(AFX_TREENODE_H__2D0C26D6_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
