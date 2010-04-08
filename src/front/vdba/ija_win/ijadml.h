/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijadml.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data Manipulation
**
** History:
**
** 14-Dec-1999 (uk$so01)
**    Created
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#if !defined(IJADML_HEADER)
#define IJADML_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "ijatree.h"

BOOL IJA_QueryNode (CTypedPtrList<CObList, CaIjaTreeItemData*>& listNode);
BOOL IJA_QueryDatabase (LPCTSTR lpszNode, CTypedPtrList<CObList, CaIjaTreeItemData*>& listDatabase);
BOOL IJA_QueryTable (LPCTSTR lpszNode, LPCTSTR lpszDatabase, CTypedPtrList<CObList, CaIjaTreeItemData*>& listTable, int nStar = 0);
BOOL IJA_QueryUser (LPCTSTR lpszNode, CTypedPtrList<CObList, CaUser*>& listUser);

#endif // IJADML_HEADER