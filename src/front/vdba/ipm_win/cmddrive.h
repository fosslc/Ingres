/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : cmddrive.h : header file.
**    Project  : INGRES II/ Monitoring (ActiveX Control).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Command line driven (select the specific object in the tree
**               and display its properties on the right pane)
**
** History:
**
** 15-Apr-2002 (uk$so01)
**    Created
*/

#if !defined(CMDDRIVEN_HEADER)
#define CMDDRIVEN_HEADER
#include "ipmfast.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CdIpmDoc;
class CaTreeItemPath: public CuIpmTreeFastItem
{
public:
	CaTreeItemPath(BOOL bStatic, int type, void* pFastStruct = NULL, BOOL bCheckName = FALSE, LPCTSTR lpsCheckName = NULL);
	CaTreeItemPath(void* pFastStruct, int type, BOOL bStatic, BOOL bCheckName = FALSE, LPCTSTR lpsCheckName = NULL);
	virtual ~CaTreeItemPath();

private:
	BOOL  m_bDeleteStruct;
};
BOOL IPM_BuildItemPath (CdIpmDoc* pDoc, CTypedPtrList<CObList, CuIpmTreeFastItem*>& listItemPath);
int  GetDefaultSelectTab (CdIpmDoc* pDoc);

#endif // CMDDRIVEN_HEADER