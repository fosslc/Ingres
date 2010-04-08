/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : comseq.cpp: implementation of the CmtSequence, CmtFolderSequence class
**    Project  : Meta data library 
**    Author   : Philippe Schalk (schph01)
**    Purpose  : Sequence object and folder
**
** History:
**
** 22-April-2003 (schph01)
**    SIR 107523 Created
**/


#include "stdafx.h"
#include "comaccnt.h"
#include "comdbase.h"
#include "comseq.h"
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
static HRESULT CopySequences2Stream(CTypedPtrList< CObList, CmtSequence* >& listObject, IStream*& pStream)
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
		CTypedPtrList <CObList, CaSequence*> listObject2Store;
		//
		// Write the individual object:
		CaSequence* pObj = NULL;
		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CmtSequence* pMtObj = listObject.GetNext(pos);
			pMtObj->GetSequence (pObj);
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
// Object: Sequence
// *****************
CmtSessionManager* CmtSequence::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}
//
// Folder of Sequence:
// ********************
CmtFolderSequence::~CmtFolderSequence()
{
	CmtMutexReadWrite mAccess(this);
	CmtWaitUntilNoAccess mWait(this);

	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

CmtSessionManager* CmtFolderSequence::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}

CmtSequence* CmtFolderSequence::SearchObject(CaLLQueryInfo* pInfo, CaSequence* pObj, BOOL b2QueryIfNotDone)
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
		CmtSequence* pMtObj = m_listObject.GetNext (pos);
		CaSequence& obj = pMtObj->GetSequence();
		if (obj.Matched(pObj))
		{
			pMtObj->Access();
			return pMtObj;
		}
	}

	return NULL;
}


HRESULT CmtFolderSequence::GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream)
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
			hError = CopySequences2Stream (m_listObject, pStream);
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
	pInfo->SetObjectType(OBT_SEQUENCE);
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
	CmtSequence* pMtObj = NULL;
	POSITION p = NULL, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		pMtObj = m_listObject.GetNext (pos);
		pMtObj->SetState (CmtItemData::ITEM_DELETE);
	}
	
	while (!lNew.IsEmpty())
	{
		CaSequence* pObj = (CaSequence*)lNew.RemoveHead();
		CmtSequence* pObjExist = SearchObject(pInfo, pObj, FALSE);

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
		CmtSequence* pNewMtObject = new CmtSequence (pObj);

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
		hError = CopySequences2Stream (m_listObject, pStream);
	SetLastQuery();

	//
	// Release all blockeing threads that wait for 'mWrite' mutex:
	mReadWrite.UnLock();

	return hError;
}

