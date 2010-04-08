/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : cominteg.cpp: implementation of the CmtIntegrity, CmtFolderIntegrity class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Integrity object and folder
**
** History:
**
** 24-Nov-2000 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "comaccnt.h"
#include "comdbase.h"
#include "cominteg.h"
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
static HRESULT CopyIntegrities2Stream(CTypedPtrList< CObList, CmtIntegrity* >& listObject, IStream*& pStream)
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
		CTypedPtrList <CObList, CaIntegrity*> listObject2Store;
		//
		// Write the individual object:
		CaIntegrity* pObj = NULL;
		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CmtIntegrity* pMtObj = listObject.GetNext(pos);
			pMtObj->GetIntegrity (pObj);
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
// Object: Integrity
// *****************
CmtSessionManager* CmtIntegrity::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}
//
// Folder of Integrity:
// ********************
CmtFolderIntegrity::~CmtFolderIntegrity()
{
	CmtMutexReadWrite mAccess(this);
	CmtWaitUntilNoAccess mWait(this);

	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

CmtSessionManager* CmtFolderIntegrity::GetSessionManager()
{
	return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;
}

CmtIntegrity* CmtFolderIntegrity::SearchObject(CaLLQueryInfo* pInfo, CaIntegrity* pObj, BOOL b2QueryIfNotDone)
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
		CmtIntegrity* pMtObj = m_listObject.GetNext (pos);
		CaIntegrity& obj = pMtObj->GetIntegrity();
		if (obj.Matched(pObj))
		{
			pMtObj->Access();
			return pMtObj;
		}
	}

	return NULL;
}


HRESULT CmtFolderIntegrity::GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream)
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
			hError = CopyIntegrities2Stream (m_listObject, pStream);
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
	// CmtTable:
	ASSERT (m_pBackParent);
	if (!m_pBackParent)
		return E_FAIL;
	CaTable& table =((CmtTable*)m_pBackParent)->GetTable();
	CString strItem;
	CString strItemOwner;
	strItem = table.GetName();
	strItemOwner = table.GetOwner();
	//
	// CmtFolderTable:
	CmtFolderTable* pParent2 = ((CmtTable*)m_pBackParent)->GetBackParent();
	ASSERT (pParent2);
	if (!pParent2)
		return E_FAIL;
	//
	// CmtDatabase:
	CmtDatabase* pmtDatabase = pParent2->GetBackParent();
	ASSERT (pmtDatabase);
	if (!pmtDatabase)
		return E_FAIL;
	CaDatabase& database = pmtDatabase->GetDatabase();
	//
	// CmtFolderDatabase:
	CmtFolderDatabase* pParent3 = pmtDatabase->GetBackParent();
	ASSERT (pParent3);
	if (!pParent3)
		return E_FAIL;
	//
	// CmtNode:
	CmtNode* pParent4 = pParent3->GetBackParent();
	ASSERT (pParent4);
	if (!pParent4)
		return E_FAIL;
	CaNode& node = pParent4->GetNode();

	//
	// Recursive until reaching the Account Folder and return the 
	// &m_sessionMager of that account:
	CmtSessionManager* pOpaqueSessionManager = GetSessionManager();
	CTypedPtrList< CObList, CaDBObject* > lNew;
	pInfo->SetObjectType(OBT_INTEGRITY);
	pInfo->SetNode (node.GetName());
	pInfo->SetDatabase(database.GetName());
	pInfo->SetItem2(strItem, strItemOwner);

	BOOL bOk = INGRESII_llQueryObject (pInfo, lNew, pOpaqueSessionManager);
	if (!bOk)
	{
		//
		// TODO:HANDLE ERRROR

		return E_FAIL;
	}

	//
	// Mark all old node as being deleted:
	CmtIntegrity* pMtObj = NULL;
	POSITION p = NULL, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		pMtObj = m_listObject.GetNext (pos);
		pMtObj->SetState (CmtItemData::ITEM_DELETE);
	}
	
	while (!lNew.IsEmpty())
	{
		CaIntegrity* pObj = (CaIntegrity*)lNew.RemoveHead();
		CmtIntegrity* pObjExist = SearchObject(pInfo, pObj, FALSE);

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
		CmtIntegrity* pNewMtObject = new CmtIntegrity (pObj);

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
		hError = CopyIntegrities2Stream (m_listObject, pStream);
	SetLastQuery();

	//
	// Release all blockeing threads that wait for 'mWrite' mutex:
	mReadWrite.UnLock();

	return hError;
}

