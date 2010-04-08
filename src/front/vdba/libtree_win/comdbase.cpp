/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : comdbase.cpp: implementation of the CmtDatabase, CmtFolderDatabase class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Database object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "comaccnt.h"
#include "comdbase.h"
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
static HRESULT CopyDatabases2Stream(CTypedPtrList< CObList, CmtDatabase* >& listObject, IStream*& pStream)
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
		CTypedPtrList <CObList, CaDatabase*> listObject2Store;

		CaDatabase* pObj = NULL;
		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CmtDatabase* pMtObj = listObject.GetNext(pos);
			pMtObj->GetDatabase (pObj);
			listObject2Store.AddTail(pObj);
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
// Object: Database
// ****************
CmtSessionManager* CmtDatabase::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}
//
// Folder of Database:
// *******************
CmtFolderDatabase::~CmtFolderDatabase()
{
	CmtMutexReadWrite mAccess(this);
	CmtWaitUntilNoAccess mWait(this);

	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

CmtSessionManager* CmtFolderDatabase::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}

CmtDatabase* CmtFolderDatabase::SearchObject(CaLLQueryInfo* pInfo, CaDatabase* pObj, BOOL b2QueryIfNotDone)
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
		CmtDatabase* pMtObj = m_listObject.GetNext (pos);
		CaDatabase& obj = pMtObj->GetDatabase();
		if (obj.Matched(pObj))
		{
			pMtObj->Access();
			return pMtObj;
		}
	}

	return NULL;
}


HRESULT CmtFolderDatabase::GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream)
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
			hError = CopyDatabases2Stream (m_listObject, pStream);
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
	// CmtNode: (parent 1)
	ASSERT (m_pBackParent);
	if (!m_pBackParent)
		return E_FAIL;
	CaNode& node = m_pBackParent->GetNode();

	//
	// Recursive until reaching the Account Folder and return the 
	// &m_sessionMager of that account:
	CmtSessionManager* pOpaqueSessionManager = GetSessionManager();
	CTypedPtrList< CObList, CaDBObject* > lNew;
	pInfo->SetObjectType(OBT_DATABASE);
	pInfo->SetNode (node.GetName());

	BOOL bOk = INGRESII_llQueryObject (pInfo, lNew, pOpaqueSessionManager);
	if (!bOk)
	{
		//
		// TODO:HANDLE ERRROR

		return E_FAIL;
	}

	//
	// Mark all old node as being deleted:
	CmtDatabase* pMtObj = NULL;
	POSITION p = NULL, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		pMtObj = m_listObject.GetNext (pos);
		pMtObj->SetState (CmtItemData::ITEM_DELETE);
	}
	
	while (!lNew.IsEmpty())
	{
		CaDatabase* pObj = (CaDatabase*)lNew.RemoveHead();
		CmtDatabase* pObjExist = SearchObject(pInfo, pObj, FALSE);

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
		CmtDatabase* pNewMtObject = new CmtDatabase (pObj);

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
		hError = CopyDatabases2Stream (m_listObject, pStream);
	SetLastQuery();

	//
	// Release all blockeing threads that wait for 'mWrite' mutex:
	mReadWrite.UnLock();

	return hError;
}

