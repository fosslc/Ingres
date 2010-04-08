/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : nodeinfo.h, Header File 
** 
**    Project  : Virtual Node Manager.
**    Author   : UK Sotheavut, 
**    Purpose  : Information about a node of Tree View of Node Data 
**               (image, expanded, ...) 
**
** History:
** xx-Mar-1999 (uk$so01)
**    created.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 09-Feb-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Now ingnet uses ingnet.chm instead of vdba.chm 
*/

#if !defined(AFX_NODEINFO_H__2D0C26D8_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_NODEINFO_H__2D0C26D8_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CaVNodeTreeItemData;
class CaTreeNodeInformation  
{
public:
	CaTreeNodeInformation();
	virtual ~CaTreeNodeInformation();

	HTREEITEM GetTreeItem(){return m_hTreeItem;}
	BOOL IsExpanded(){return m_bExpanded;}
	BOOL IsQueried(){return m_bQueried;}

	void SetTreeItem(HTREEITEM hTree){m_hTreeItem = hTree;}
	void SetExpanded (BOOL bExpand){m_bExpanded = bExpand;}
	void SetQueried (BOOL bQueried){m_bQueried = bQueried;}
	void SetImage(int nImage){m_nImage = nImage;}
	int  GetImage(){return m_nImage;}

protected:
	HTREEITEM m_hTreeItem;
	BOOL m_bExpanded;  // TRUE if expanded, otherwise FALSE
	BOOL m_bQueried;   // TRUE if data has been queried
	int  m_nImage;

};


#include "vnode.h"

//
// Query the list of Virtual Nodes:
BOOL VNODE_QueryNode (
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& oldList, 
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& listNode);

BOOL VNODE_QueryLogin      (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem);
BOOL VNODE_QueryConnection (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem);
BOOL VNODE_QueryAttribute  (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem);
BOOL VNODE_QueryServer     (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem, BOOL bLocal = FALSE);

//
// Start the VDBA on Context:
BOOL VNODE_RunVdbaSql(LPCTSTR lpszNode);
//
// Start the VDBA on Context:
BOOL VNODE_RunVdbaDom(LPCTSTR lpszNode);
//
// Start the VDBA on Context:
BOOL VNODE_RunVdbaIpm(LPCTSTR lpszNode);
//
// Add a tree item:
HTREEITEM VNODE_TreeAddItem (LPCTSTR lpszItem, CTreeCtrl* pTree, HTREEITEM hParent, HTREEITEM hInsertAfter, int nImage, DWORD_PTR dwData);
//
// Start the INGNET on Context:
BOOL VNODE_RunIngNetLocalVnode(LPCTSTR lpszNode);
//

#if defined (EDBC)
#define HELPFILE TEXT ("edbcnet.chm")
#define IPMEXE TEXT ("???") // not available

#if defined (MAINWIN)
#define DOMEXE TEXT ("edbcvdba")
#define SQLEXE TEXT ("edbquery")
#else
#define DOMEXE TEXT ("edbcvdba.exe")
#define SQLEXE TEXT ("edbquery.exe")
#endif // #if defined (MAINWIN)

#else  // NOT EDBC
#define HELPFILE TEXT ("ingnet.chm")

#if defined (MAINWIN)
#define DOMEXE TEXT ("vdba")
#define SQLEXE TEXT ("vdbasql")
#define IPMEXE TEXT ("vdbamon")
#else
#define DOMEXE TEXT ("vdba.exe")
#define SQLEXE TEXT ("vdbasql.exe")
#define IPMEXE TEXT ("vdbamon.exe")
#endif

#endif // #if defined (EDBC)


#endif // !defined(AFX_NODEINFO_H__2D0C26D8_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
