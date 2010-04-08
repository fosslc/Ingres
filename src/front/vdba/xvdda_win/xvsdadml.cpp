/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xvsdadml.cpp, implementation file 
**    Project  : Schema Diff
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Stand alone Visual SDA.
**
** History:
**
** 24-Sep-2002 (uk$so01)
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 18-Oct-2004 (noifr01)
**    (bug 113254) don't include $ingres in the schema comboboxes
*/

#include "stdafx.h"
#include "vsda.h"
#include "vnodedml.h"
#include "ingobdml.h"
#include "constdef.h"
#include "dmldbase.h"
#include "usrexcep.h"
#include "xvsdadml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL DML_QueryNode(CdSda* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = (VNODE_QueryNode (listObj) == NOERROR)? TRUE: FALSE;
	return bOK;
}


BOOL DML_QueryServer(CdSda* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = (VNODE_QueryServer (pNode, listObj) == NOERROR)? TRUE: FALSE;
	return bOK;
}

BOOL DML_QueryUser(CdSda* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = FALSE;
	CaLLQueryInfo queryInfo (OBT_USER, pNode->GetName());
	queryInfo.SetFetchObjects (CaLLQueryInfo::FETCH_USER);
	queryInfo.SetIndependent(TRUE);

	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	bOK = INGRESII_llQueryObject (&queryInfo, listObj, (void*)&sessionMgr);

	return bOK;
}

BOOL DML_QueryDatabase(CdSda* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = FALSE;
	CaLLQueryInfo queryInfo (OBT_DATABASE, pNode->GetName());
	queryInfo.SetFetchObjects (CaLLQueryInfo::FETCH_USER);
	queryInfo.SetIndependent(TRUE);

	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	bOK = INGRESII_llQueryObject (&queryInfo, listObj, (void*)&sessionMgr);
#if defined (_IGNORE_STAR_DATABASE)
	if (bOK)
	{
		POSITION p = NULL, pos = listObj.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaDatabase* pDb = (CaDatabase*)listObj.GetNext(pos);
			if (pDb->GetStar() != OBJTYPE_NOTSTAR)
			{
				listObj.RemoveAt(p);
				delete pDb;
			}
		}
	}
#endif
	return bOK;
}
