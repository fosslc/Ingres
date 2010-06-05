/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijadml.cpp , Implementation File
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
** 02-Feb-2004 (noifr01)
**    removed test code that was causing problem with new versions of the
**    compiler (with iostream libraries)
** 05-Mar-2010 (drivi01)
**    Temporarily use SubType field of CaLLQueryInfo to set VectorWise
**    installation field.
**    VectorWise is not journaled and therefore shouldn't allow users
**    to view journals.
**/

#include "stdafx.h"
#include "ija.h"
#include "ijadml.h"
#include "dmldbase.h"
#include "vnodedml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



BOOL IJA_QueryNode (CTypedPtrList<CObList, CaIjaTreeItemData*>& listNode)
{
	//
	// Real List of Nodes:
	CTypedPtrList<CObList, CaDBObject*> list;

	HRESULT hErr = VNODE_QueryNode(list);
	if (hErr == NOERROR)
	{
		POSITION pos = list.GetHeadPosition();
		while (pos != NULL)
		{
			CaNode* pNode = (CaNode*)list.GetNext(pos);
			CaIjaNode* pNewNode = new CaIjaNode(pNode->GetName());
			if (pNode->IsLocalNode())
				pNewNode->LocalNode();
			listNode.AddTail (pNewNode);
			delete pNode;
		}
	}
	return (hErr == NOERROR)? TRUE: FALSE;
}

BOOL IJA_QueryDatabase (LPCTSTR lpszNode, CTypedPtrList<CObList, CaIjaTreeItemData*>& listDatabase)
{
	CaIjaDatabase* pItem = NULL;

	CaSessionManager& smgr = theApp.GetSessionManager();
	CTypedPtrList< CObList, CaDBObject* > listObj;
	CaLLQueryInfo info (OBT_DATABASE, lpszNode);
	info.SetIndependent(TRUE);
	info.SetFetchObjects(CaLLQueryInfo::FETCH_USER);
	BOOL bOk = INGRESII_llQueryObject (&info, listObj, (void*)&smgr);
	if (bOk)
	{
		POSITION pos = listObj.GetHeadPosition();
		while (pos != NULL)
		{
			CaDatabase* pDb = (CaDatabase*)listObj.GetNext(pos);
			CaIjaDatabase* pObj = new CaIjaDatabase(pDb->GetName(), pDb->GetOwner());
			pObj->SetStar(pDb->GetStar());
			if (info.GetSubObjectType() == DBMS_INGRESVW)
			{
				pObj->SetVW(1);
			}
			delete pDb;
			listDatabase.AddTail(pObj);
		}
	}

	return TRUE;
}


BOOL IJA_QueryTable (LPCTSTR lpszNode, LPCTSTR lpszDatabase, CTypedPtrList<CObList, CaIjaTreeItemData*>& listTable, int nStar)
{
	CaIjaTable* pItem = NULL;
	//
	// Real List of Tables:
	CaSessionManager& smgr = theApp.GetSessionManager();
	CTypedPtrList< CObList, CaDBObject* > listObj;
	CaLLQueryInfo info (OBT_TABLE, lpszNode, lpszDatabase);
	info.SetIndependent(TRUE);
	info.SetFetchObjects(CaLLQueryInfo::FETCH_USER);
	if (nStar != OBJTYPE_NOTSTAR)
		info.SetFlag(DBFLAG_STARNATIVE);
	BOOL bOk = INGRESII_llQueryObject (&info, listObj, (void*)&smgr);
	if (bOk)
	{
		POSITION pos = listObj.GetHeadPosition();
		while (pos != NULL)
		{
			CaTable* pTable = (CaTable*)listObj.GetNext(pos);
			CaIjaTable* pNewTable = new CaIjaTable(pTable->GetName(), pTable->GetOwner());
			listTable.AddTail (pNewTable);
			delete pTable;
		}
	}

	return bOk;
}


BOOL IJA_QueryUser (LPCTSTR lpszNode, CTypedPtrList<CObList, CaUser*>& listUser)
{
	try
	{
		CaSessionManager& smgr = theApp.GetSessionManager();
		CTypedPtrList< CObList, CaDBObject* > listObj;
		CaLLQueryInfo info (OBT_USER, lpszNode);
		info.SetIndependent(TRUE);
		BOOL bOk = INGRESII_llQueryObject (&info, listObj, (void*)&smgr);
		if (bOk)
		{
			POSITION pos = listObj.GetHeadPosition();
			while (pos != NULL)
			{
				CaUser* pUser = (CaUser*)listObj.GetNext(pos);
				listUser.AddTail (pUser);
			}
		}

		return bOk;
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
		return FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error::IJA_QueryUser, failed to query users."));
		return FALSE;
	}
	return TRUE;
}


