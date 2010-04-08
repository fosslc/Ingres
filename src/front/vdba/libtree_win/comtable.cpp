/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : comtable.cpp: implementation of the CmtTable, CmtFolderTable class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Table object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "comaccnt.h"
#include "comdbase.h"
#include "comtable.h"
#include "ingobdml.h"
#include "a2stream.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// ************************************************************************************************
//                                 LOCAL FUNCTIONS
// ************************************************************************************************
static HRESULT CopyTables2Stream(CTypedPtrList< CObList, CmtTable* >& listObject, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	IStream* pStreamBuffer = NULL;
	//
	// Prepare to transfer data through the ISream interface:
	try
	{
		CMemFile file;
		CArchive ar(&file, CArchive::store);

		//
		// List of objects to store to the Archive;
		CTypedPtrList <CObList, CaTable*> listObject2Store;
		//
		// Write the individual object:
		CaTable* pObj = NULL;
		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CmtTable* pMtObj = listObject.GetNext(pos);
			pMtObj->GetTable (pObj);
			listObject2Store.AddTail (pObj);
		}

		//
		// Serialize the list of objects and cleanup the list.
		listObject2Store.Serialize (ar);
		while (!listObject2Store.IsEmpty())
			delete listObject2Store.RemoveHead();
		//
		// Force everything to be written to File (memory file)
		ar.Flush();
		CArchiveToIStream (ar, &pStreamBuffer);
	}
	catch (...)
	{
		hError = E_FAIL;
		if (pStreamBuffer)
			pStreamBuffer->Release();
		pStreamBuffer = NULL;
	}

	pStream = pStreamBuffer;
	return hError;
}



//
// Object: Table
// **************
IMPLEMENT_SERIAL (CmtTable, CObject, 1)
CmtSessionManager* CmtTable::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}
//
// Folder of Table:
// ****************
CmtFolderTable::~CmtFolderTable()
{
	CmtMutexReadWrite mAccess(this);
	CmtWaitUntilNoAccess mWait(this);

	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

CmtSessionManager* CmtFolderTable::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}

CmtTable* CmtFolderTable::SearchObject(CaLLQueryInfo* pInfo, CaTable* pObj, BOOL b2QueryIfNotDone)
{
	if (b2QueryIfNotDone && GetLastQuery() == -1)
	{
		IStream* pStream = NULL;
		BOOL bCopy2Stream = FALSE;
		HRESULT hError = GetListObject (pInfo, pStream, bCopy2Stream);
		ASSERT (pStream == NULL);
		if FAILED(hError)
			return NULL;
	}

	//
	// Block the threads that want to modify (update) the list of objects:
	CmtAccess access(this);

	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CmtTable* pMtObj = m_listObject.GetNext (pos);
		CaTable& obj = pMtObj->GetTable();
		if (obj.Matched(pObj))
		{
			pMtObj->Access();
			return pMtObj;
		}
	}

	return NULL;
}


HRESULT CmtFolderTable::GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream)
{
	HRESULT hError = NOERROR;
	pStream = NULL;
	//
	// Wait until another thread has finised updating the list of nodes:
	CmtMutexReadWrite mReadWrite(this);
	//
	// At this point, All threads want to read or update are blocked by 'mReadWrite' mutex.
	// All entries to the cache are blocked !

	if (GetLastQuery() != -1)
	{
		CmtAccess access(this);
		//
		// Release all blockeing threads that wait for 'mReadWrite' mutex:
		mReadWrite.UnLock();
		if (bCopy2Stream)
			hError = CopyTables2Stream (m_listObject, pStream);
		return hError;
	}

	// ALL ACCESSES TO THE CACHE HAVE BEEN BLOCKED BUT
	// Wait until all threads have finished referenced to this object,
	// because this object would be destroyed while the thread might have access to 
	// the descendants of this object.
	// Ex: Another thread might be querying the list of users of a given node
	//     that would be removed.
	CmtWaitUntilNoAccess mWait(this);

	//
	// CmtDatabase:
	ASSERT (m_pBackParent);
	if (!m_pBackParent)
		return E_FAIL;
	CaDatabase& database = m_pBackParent->GetDatabase();
	//
	// CmtFolderDatabase:
	CmtFolderDatabase* pParent2 = m_pBackParent->GetBackParent();
	ASSERT (pParent2);
	if (!pParent2)
		return E_FAIL;
	//
	// CmtNode:
	CmtNode* pParent3 = pParent2->GetBackParent();
	ASSERT (pParent3);
	if (!pParent3)
		return E_FAIL;
	CaNode& node = pParent3->GetNode();

	//
	// Recursive until reaching the Account Folder and return the 
	// &m_sessionMager of that account:
	CmtSessionManager* pOpaqueSessionManager = GetSessionManager();
	CTypedPtrList< CObList, CaDBObject* > lNew;
	pInfo->SetObjectType(OBT_TABLE);
	pInfo->SetNode (node.GetName());
	pInfo->SetDatabase (database.GetName());

	BOOL bOk = INGRESII_llQueryObject (pInfo, lNew, pOpaqueSessionManager);
	if (!bOk)
	{
		//
		// TODO:HANDLE ERRROR

		return E_FAIL;
	}

	//
	// Mark all old node as being deleted:
	CmtTable* pMtObj = NULL;
	POSITION p = NULL, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		pMtObj = m_listObject.GetNext (pos);
		pMtObj->SetState (CmtItemData::ITEM_DELETE);
	}
	
	while (!lNew.IsEmpty())
	{
		CaTable* pObj = (CaTable*)lNew.RemoveHead();
		CmtTable* pObjExist = SearchObject(pInfo, pObj, FALSE);

		//
		// The new queried object already exists in the old list, we destroy it !
		if (pObjExist != NULL)
		{
			pObjExist->SetState (CmtItemData::ITEM_EXIST);
			delete pObj;
			continue;
		}

		//
		// New object that is not in the old list, add it to the list:
		CmtTable* pNewMtObject = new CmtTable (pObj);

		pNewMtObject->SetBackParent (this);
		m_listObject.AddTail (pNewMtObject);

		delete pObj;
	}

	//
	// Delete all items that have the flag = ITEM_DELETE.
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pMtObj = m_listObject.GetNext (pos);
		if (pMtObj && pMtObj->GetState () == CmtItemData::ITEM_DELETE)
		{
			m_listObject.RemoveAt (p);
			delete pMtObj;
		}
	}

	if (bCopy2Stream)
		hError = CopyTables2Stream (m_listObject, pStream);
	SetLastQuery();

	//
	// Release all blockeing threads that wait for 'mWrite' mutex:
	mReadWrite.UnLock();

	return hError;
}

