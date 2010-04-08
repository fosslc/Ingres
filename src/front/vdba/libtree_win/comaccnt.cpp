/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : comaccnt.cpp: Implementation for the CmtAccount, CmtFolderAccount class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : ACCOUNT OBJECT object and folder
**
** History:
**
** 21-Nov-2000 (uk$so01)
**    Created
**/

#include "stdafx.h"
#include "comaccnt.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Folder of Account:
// ******************
CmtFolderAccount::CmtFolderAccount(): CmtItemData(), CmtProtect()
{
}

CmtFolderAccount::~CmtFolderAccount()
{
	CmtMutexReadWrite mAccess(this);
	CmtWaitUntilNoAccess mWait(this);

	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();

}

CmtAccount* CmtFolderAccount::SearchObject(LPCTSTR lpszDomain, LPCTSTR lpszUser)
{
	CmtMutexReadWrite mAccess(this);

	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CmtAccount* pMtObj = m_listObject.GetNext (pos);
		if (pMtObj && pMtObj->GetUser().CompareNoCase (lpszUser) == 0 && pMtObj->GetDomain().CompareNoCase (lpszDomain) == 0)
		{
			pMtObj->Access();
			return pMtObj;
		}
	}
	return NULL;
}


CmtAccount* CmtFolderAccount::AddAccount (LPCTSTR lpszDomain, LPCTSTR lpszUser)
{
	CmtAccount* pExist = SearchObject(lpszDomain, lpszUser);
	if (pExist)
	{
		pExist->UnAccess();
		return pExist;
	}

	CmtMutexReadWrite mAccess(this);
	CmtAccount* pNewObj = new CmtAccount(this, lpszDomain, lpszUser);
	m_listObject.AddTail(pNewObj);
	return pNewObj;
}

void CmtFolderAccount::DelAccount (LPCTSTR lpszDomain, LPCTSTR lpszUser)
{
	CmtMutexReadWrite mAccess(this);

	POSITION p, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CmtAccount* pObj = m_listObject.GetNext (pos);
		if (pObj && pObj->GetUser().CompareNoCase (lpszUser) == 0 && pObj->GetDomain().CompareNoCase (lpszDomain) == 0)
		{
			//
			// Wait until NO THREAD is currently accessing the account object 
			// before destroying it !
			pObj->WaitUntilNoAccess();
			mAccess.UnLock();

			m_listObject.RemoveAt(p);
			delete pObj;
			return;
		}
	}
}



//
// OBJECT Account:
// ***************



HRESULT CmtAccount::GetListObject (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	BOOL bMustQueryObject_IfNotDone = TRUE; // Query the object first if not has been yet.
	BOOL bCopy2Stream = TRUE;               // Copy the data to the ISream Interface

	ASSERT (pInfo);
	if (!pInfo)
		return E_FAIL;
	//
	// Mark the access to this account:
	CmtAccess access1(this);

	switch (pInfo->GetObjectType())
	{
	case OBT_VIRTNODE:
		return m_folderNode.GetListObject (pInfo, pStream, bCopy2Stream);
	default:
		break;
	}

	CString strNode = pInfo->GetNode();
	ASSERT (!strNode.IsEmpty());
	if (strNode.IsEmpty())
		return E_FAIL;
	//
	// Find Node:
	// m_folderNode.SearchObject might update the list of nodes:
	BOOL b2QueryIfNotDone = TRUE;
	CaNode node (strNode);
	CaLLQueryInfo infoNode(*pInfo);
	infoNode.SetObjectType (OBT_VIRTNODE);
	CmtNode* pMtNode = m_folderNode.SearchObject (&infoNode, &node, b2QueryIfNotDone);
	ASSERT(pMtNode);
	if (!pMtNode)
		return E_FAIL;
	//
	// SearchObject must have marked 'pMtNode' as accessed so that another
	// thread wants to remove it must wait !
	ASSERT(pMtNode->IsAccessed());
	//
	// Only unaccess 'pMtNode' on Destructor:
	CmtAccess access2(pMtNode, FALSE);

	//
	// Mark the access to the folder of nodes:
	CmtAccess access3(&m_folderNode);

	switch (pInfo->GetObjectType())
	{
	case OBT_VNODE_LOGINPSW:
		return pMtNode->GetFolderNodeLogin().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_VNODE_CONNECTIONDATA:
		return pMtNode->GetFolderNodeConnectData().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_VNODE_SERVERCLASS:
		return pMtNode->GetFolderNodeServer().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_VNODE_ATTRIBUTE:
		return pMtNode->GetFolderNodeAttribute().GetListObject(pInfo, pStream, bCopy2Stream);

	case OBT_PROFILE:
		return pMtNode->GetFolderProfile().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_USER:
		return pMtNode->GetFolderUser().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_GROUP:
		return pMtNode->GetFolderGroup().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_ROLE:
		return pMtNode->GetFolderRole().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_LOCATION:
		return pMtNode->GetFolderLocation().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_DATABASE:
		return pMtNode->GetFolderDatabase().GetListObject(pInfo, pStream, bCopy2Stream);
	default:
		break;
	}

	//
	// Find Database:
	CString strDatabase = pInfo->GetDatabase();
	ASSERT (!strDatabase.IsEmpty());
	if (strDatabase.IsEmpty())
		return E_FAIL;
	CmtFolderDatabase& folderDatabase = pMtNode->GetFolderDatabase();
	CaDatabase db (strDatabase, _T(""));
	CmtDatabase* pMtDatabase = folderDatabase.SearchObject (pInfo, &db, b2QueryIfNotDone);
	ASSERT(pMtDatabase);
	if (!pMtDatabase)
		return E_FAIL;
	//
	// folderDatabase.SearchObject must have marked 'pMtDatabase' as accessed so that another
	// thread wants to remove it must wait !
	ASSERT(pMtDatabase->IsAccessed());
	//
	// Only unaccess 'pMtDatabase' on Destructor:
	CmtAccess access4(pMtDatabase, FALSE);

	//
	// Mark the access to the folder of database:
	CmtAccess access5(&folderDatabase);

	switch (pInfo->GetObjectType())
	{
	case OBT_TABLE:
		return pMtDatabase->GetFolderTable().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_VIEW:
		return pMtDatabase->GetFolderView().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_PROCEDURE:
		return pMtDatabase->GetFolderProcedure().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_DBEVENT:
		return pMtDatabase->GetFolderDBEvent().GetListObject(pInfo, pStream, bCopy2Stream);
	case OBT_SYNONYM:
		return pMtDatabase->GetFolderSynonym().GetListObject(pInfo, pStream, bCopy2Stream);
	default:
		break;
	}


	//
	// Find Table:
	if (pInfo->GetObjectType() == OBT_TABLECOLUMN ||
	    pInfo->GetObjectType() == OBT_INDEX ||
	    pInfo->GetObjectType() == OBT_RULE ||
	    pInfo->GetObjectType() == OBT_INTEGRITY)
	{
		CString strItem2 = pInfo->GetItem2();
		CString strItem2Owner = pInfo->GetItem2Owner();
		ASSERT (!strItem2.IsEmpty() && !strItem2Owner.IsEmpty());
		if (strItem2.IsEmpty() || strItem2Owner.IsEmpty())
			return E_FAIL;

		CmtFolderTable& folderTable = pMtDatabase->GetFolderTable();
		CaTable tb(strItem2, strItem2Owner);
		CaLLQueryInfo infoTable(*pInfo);
		infoTable.SetObjectType (OBT_TABLE);

		CmtTable* pMtTable = folderTable.SearchObject (&infoTable, &tb, b2QueryIfNotDone);
		ASSERT(pMtTable);
		if (!pMtTable)
			return E_FAIL;

		//
		// folderTable.SearchObject must have marked 'pMtTable' as accessed so that another
		// thread wants to remove it must wait !
		ASSERT(pMtTable->IsAccessed());
		//
		// Only unaccess 'pMtTable' on Destructor:
		CmtAccess access6(pMtTable, FALSE);
		//
		// Mark the access to the folder of Table:
		CmtAccess access7(&folderTable);

		switch (pInfo->GetObjectType())
		{
		case OBT_TABLECOLUMN:
			return pMtTable->GetFolderColumn().GetListObject(pInfo, pStream, bCopy2Stream);
		case OBT_INDEX:
			return pMtTable->GetFolderIndex().GetListObject(pInfo, pStream, bCopy2Stream);
		case OBT_RULE:
			return pMtTable->GetFolderRule().GetListObject(pInfo, pStream, bCopy2Stream);
		case OBT_INTEGRITY:
			return pMtTable->GetFolderIntegrity().GetListObject(pInfo, pStream, bCopy2Stream);
		default:
			break;
		}
	}

	//
	// Find View:
	if (pInfo->GetObjectType() == OBT_VIEWCOLUMN)
	{
		CString strItem2 = pInfo->GetItem2();
		CString strItem2Owner = pInfo->GetItem2Owner();
		ASSERT (!strItem2.IsEmpty() && !strItem2Owner.IsEmpty());
		if (strItem2.IsEmpty() || strItem2Owner.IsEmpty())
			return E_FAIL;

		CmtFolderView& folderView = pMtDatabase->GetFolderView();
		CaView view(strItem2, strItem2Owner);
		CaLLQueryInfo infoView(*pInfo);
		infoView.SetObjectType (OBT_VIEW);
		CmtView* pMtView = folderView.SearchObject (&infoView, &view, b2QueryIfNotDone);
		ASSERT(pMtView);
		if (!pMtView)
			return E_FAIL;

		//
		// folderView.SearchObject must have marked 'pMtView' as accessed so that another
		// thread wants to remove it must wait !
		ASSERT(pMtView->IsAccessed());
		//
		// Only unaccess 'pMtTable' on Destructor:
		CmtAccess access6(pMtView, FALSE);
		//
		// Mark the access to the folder of Table:
		CmtAccess access7(&folderView);

		switch (pInfo->GetObjectType())
		{
		case OBT_VIEWCOLUMN:
			return pMtView->GetFolderColumn().GetListObject(pInfo, pStream, bCopy2Stream);
		default:
			break;
		}
	}

	//
	// Need the implementation ?
	ASSERT (FALSE);
	return E_FAIL;
}



//
// This function ignores the parameter 'pInfo'.
HRESULT CmtAccount::GetListNode (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListNodeServer(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListNodeLogin(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListNodeConnection(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListNodeAttribute(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListUser (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListGroup (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListRole (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListProfile (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListLocation(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListDatabase(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NO_ERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListTable(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListView(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListTableCol(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListViewCol(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListProcedure(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);

	return hError;
}

HRESULT CmtAccount::GetListDBEvent (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListSynonym (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListIndex (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}


HRESULT CmtAccount::GetListRule (CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

HRESULT CmtAccount::GetListIntegrity(CaLLQueryInfo* pInfo, IStream*& pStream)
{
	HRESULT hError = NOERROR;
	hError = GetListObject(pInfo, pStream);
	return hError;
}

