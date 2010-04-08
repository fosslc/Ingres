/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : loadsave.cpp,Implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store Schema in the compound file.
**           
** History:
**
** 17-Sep-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 07-May-2003 (schph01)
**    SIR #107523, Add Sequence Objects
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 17-Nov-2004 (uk$so01)
**    BUG #113119, optimize the display.
** 19-Mar-2009 (drivi01)
**    Remove _LOADSAVE_SCHEMA_HEADER define.
*/

#include "stdafx.h" 
#include "constdef.h"
#include "comsessi.h"
#include "loadsave.h"
#include "compfile.h"
#include "ingobdml.h"
#include "vsdtree.h"
#include "serdbase.h"
#include "resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const CLSID CLSID_SchemaDiff = {0xCC2DA2B6,0xB8F1,0x11D6,{0x87,0xD8,0x00,0xC0,0x4F,0x1F,0x75,0x4A}};
static BOOL CheckSchemaFile(LPCTSTR lpszFile, REFCLSID clsId);

static BOOL StoreSchemaInfo (CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile, IStream* pStream);
static BOOL StoreUser    (CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreGroup   (CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreRole    (CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreProfile (CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreLocation(CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreGrantee (CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreAlarm   (CaVsdStoreSchema& storeSchema, IStream* pStream);

static BOOL EnumElements(IStorage* pStorage, CStringList& listElement, STGTY stgType = STGTY_STORAGE);
static BOOL EnumElements(IStorage* pStorage, CStringList& listElement, STGTY stgType)
{
	USES_CONVERSION;
	IEnumSTATSTG* penumStatStg = NULL;
	STATSTG statstg;
	IMalloc* pIMalloc = NULL;
	CoGetMalloc(1, &pIMalloc);
	
	HRESULT hr = pStorage->EnumElements(0, NULL, 0, &penumStatStg);
	if (!SUCCEEDED(hr))
		return FALSE;
	while (hr != S_FALSE)
	{
		hr = penumStatStg->Next(1, &statstg, NULL);
		if (FAILED(hr))
			return FALSE;
		else if (S_OK == hr)
		{
			if ((statstg.type == (DWORD)stgType) && statstg.pwcsName)
			{
				CString strName = W2T(statstg.pwcsName);
				listElement.AddTail(strName);
				if (statstg.pwcsName && pIMalloc)
					pIMalloc->Free((LPVOID)statstg.pwcsName);
			}
		}
	}
	if (pIMalloc)
		pIMalloc->Release();
	if (penumStatStg)
		penumStatStg->Release();

	return TRUE;
}


static BOOL DestroyElements(IStorage* pRoot)
{
	USES_CONVERSION;

	CStringList listElement;
	if (!EnumElements(pRoot, listElement))
		return FALSE;
	while (!listElement.IsEmpty())
	{
		CString strElement = listElement.RemoveHead();
		LPWSTR pwcsName = T2W((LPTSTR)(LPCTSTR)strElement);
		pRoot->DestroyElement(pwcsName);
	}

	return TRUE;
}

static void VSDExtractNameOwner(LPCTSTR lpszString, CString& strName, CString& strOwner)
{
	int nLen = 0;
	CString strElement = lpszString;
	CString strLen = strElement.Left(3);
	strElement = strElement.Mid(3);
	nLen = _ttoi(strLen);
	strOwner = strElement.Left(nLen);
	strName = strElement.Mid(nLen);
}


//
// FILE     : Compound File
// STRUCTURE: 
// Root (IStorage)
//   |_ SCHEMAINFO (IStream)
//   |_ USER       (IStream)
//   |_ GROUP      (IStream)
//   |_ ROLE       (IStream)
//   |_ PROFILE    (IStream)
//   |_ LOCATION   (IStream)
//   |_ GRANTEE    (IStream)
//   |_ ALARM      (IStream)
//   |_ <DB NAME>* (IStorage)
BOOL VSD_StoreInstallation (CaVsdStoreSchema& storeSchema)
{
	USES_CONVERSION;
	BOOL bFound = FALSE;
	IStream* pStream = NULL;
/*UKS
	try
	{
*/
		CaCompoundFile cmpFile (storeSchema.m_strFile);
		IStorage* pRoot = cmpFile.GetRootStorage();
		ASSERT(pRoot);
		if (!pRoot)
			return FALSE;
		pRoot->SetClass(CLSID_SchemaDiff);
		if (!DestroyElements(pRoot))
			return FALSE;

		//
		// Remove the old stream first (if any) before creating a new one:
		DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
		//
		// Store General Info
		pStream =  cmpFile.NewStream(NULL, _T("SCHEMAINFO"), grfMode);
		if (pStream)
		{
			StoreSchemaInfo (storeSchema, cmpFile, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store list of users
		pStream =  cmpFile.NewStream(NULL, _T("USER"), grfMode);
		if (pStream)
		{
			StoreUser (storeSchema, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store list of groups
		pStream =  cmpFile.NewStream(NULL, _T("GROUP"), grfMode);
		if (pStream)
		{
			StoreGroup (storeSchema, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store list of Roles
		pStream =  cmpFile.NewStream(NULL, _T("ROLE"), grfMode);
		if (pStream)
		{
			StoreRole (storeSchema, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store list of Profiles
		pStream =  cmpFile.NewStream(NULL, _T("PROFILE"), grfMode);
		if (pStream)
		{
			StoreProfile (storeSchema, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store list of Locations
		pStream =  cmpFile.NewStream(NULL, _T("LOCATION"), grfMode);
		if (pStream)
		{
			StoreLocation (storeSchema, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store list of Grantees
		pStream =  cmpFile.NewStream(NULL, _T("GRANTEE"), grfMode);
		if (pStream)
		{
			StoreGrantee (storeSchema, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store list of Alarms
		pStream =  cmpFile.NewStream(NULL, _T("ALARM"), grfMode);
		if (pStream)
		{
			StoreAlarm (storeSchema, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store Database(s) and its sub-branches
		VSD_StoreDatabase (pRoot, storeSchema, cmpFile, TRUE);
		return TRUE;
/*UKS
	}
	catch(...)
	{

	}
*/
	return FALSE;
}

//
// Object: CaVsdaLoadSchema 
// ************************************************************************************************
CaVsdaLoadSchema::CaVsdaLoadSchema():CaVsdStoreSchema()
{
	m_pCompoundFile = NULL;
}

CaVsdaLoadSchema::~CaVsdaLoadSchema()
{
	Close();
}

void CaVsdaLoadSchema::Close()
{
	if (m_pCompoundFile)
		delete m_pCompoundFile;
	m_pCompoundFile = NULL;
}

IStorage* CaVsdaLoadSchema::FindStorage (LPCTSTR lpszName)
{
	BOOL bFound = m_pCompoundFile->FindElement(NULL, lpszName, STGTY_STORAGE);
	if (!bFound)
		return NULL;
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStorage* pIStorage = m_pCompoundFile->OpenStorage(NULL, lpszName, grfMode);
	return pIStorage;
}


IStorage* CaVsdaLoadSchema::FindIRootTVDPObject(IStorage* pRootObject, int nObjectType, LPCTSTR lpObject, LPCTSTR lpOwner)
{
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	CString strEntry = _T("");
	int nIndex = 0;
	POSITION pos = NULL;
	
	switch (nObjectType)
	{
	case OBT_TABLE:
		{
			CTypedPtrList<CObList, CaTable*> ls;
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("TABLExENTRY"), grfMode);
			if (pStream)
			{
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();
			}
			pos = ls.GetHeadPosition();
			while (pos != NULL)
			{
				nIndex++;
				CaTable* pObj = ls.GetNext(pos);
				if (pObj->GetName().CompareNoCase (lpObject) == 0 && pObj->GetOwner().CompareNoCase (lpOwner) == 0)
				{
					strEntry.Format(_T("TB%08d"), nIndex);
					break;
				}
			}
			while (!ls.IsEmpty())
				delete ls.RemoveHead();
			if (!strEntry.IsEmpty())
			{
				IStorage* pIObj = m_pCompoundFile->OpenStorage(pRootObject, strEntry, grfMode);
				return pIObj;
			}
			return NULL;
		}
		break;
	case OBT_VIEW:
		{
			CTypedPtrList<CObList, CaView*> ls;
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("VIEWxENTRY"), grfMode);
			if (pStream)
			{
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();
			}
			pos = ls.GetHeadPosition();
			while (pos != NULL)
			{
				nIndex++;
				CaView* pObj = ls.GetNext(pos);
				if (pObj->GetName().CompareNoCase (lpObject) == 0 && pObj->GetOwner().CompareNoCase (lpOwner) == 0)
				{
					strEntry.Format(_T("VI%08d"), nIndex);
					break;
				}
			}
			while (!ls.IsEmpty())
				delete ls.RemoveHead();
			if (!strEntry.IsEmpty())
			{
				IStorage* pIObj = m_pCompoundFile->OpenStorage(pRootObject, strEntry, grfMode);
				return pIObj;
			}
			return NULL;
		}
		break;
	case OBT_PROCEDURE:
		{
			CTypedPtrList<CObList, CaProcedure*> ls;
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("PROCxENTRY"), grfMode);
			if (pStream)
			{
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();
			}
			pos = ls.GetHeadPosition();
			while (pos != NULL)
			{
				nIndex++;
				CaProcedure* pObj = ls.GetNext(pos);
				if (pObj->GetName().CompareNoCase (lpObject) == 0 && pObj->GetOwner().CompareNoCase (lpOwner) == 0)
				{
					strEntry.Format(_T("PR%08d"), nIndex);
					break;
				}
			}
			while (!ls.IsEmpty())
				delete ls.RemoveHead();
			if (!strEntry.IsEmpty())
			{
				IStorage* pIObj = m_pCompoundFile->OpenStorage(pRootObject, strEntry, grfMode);
				return pIObj;
			}
			return NULL;
		}
		break;
	case OBT_SEQUENCE:
		{
			CTypedPtrList<CObList, CaSequence*> ls;
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("SEQUxENTRY"), grfMode);
			if (pStream)
			{
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();
			}
			pos = ls.GetHeadPosition();
			while (pos != NULL)
			{
				nIndex++;
				CaSequence* pObj = ls.GetNext(pos);
				if (pObj->GetName().CompareNoCase (lpObject) == 0 && pObj->GetOwner().CompareNoCase (lpOwner) == 0)
				{
					strEntry.Format(_T("PR%08d"), nIndex);
					break;
				}
			}
			while (!ls.IsEmpty())
				delete ls.RemoveHead();
			if (!strEntry.IsEmpty())
			{
				IStorage* pIObj = m_pCompoundFile->OpenStorage(pRootObject, strEntry, grfMode);
				return pIObj;
			}
			return NULL;
		}
		break;
	case OBT_DBEVENT:
		{
			CTypedPtrList<CObList, CaDBEvent*> ls;
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("DBEVENTxENTRY"), grfMode);
			if (pStream)
			{
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();
			}
			pos = ls.GetHeadPosition();
			while (pos != NULL)
			{
				nIndex++;
				CaDBEvent* pObj = ls.GetNext(pos);
				if (pObj->GetName().CompareNoCase (lpObject) == 0 && pObj->GetOwner().CompareNoCase (lpOwner) == 0)
				{
					strEntry.Format(_T("EV%08d"), nIndex);
					break;
				}
			}
			while (!ls.IsEmpty())
				delete ls.RemoveHead();
			if (!strEntry.IsEmpty())
			{
				IStorage* pIObj = m_pCompoundFile->OpenStorage(pRootObject, strEntry, grfMode);
				return pIObj;
			}
			return NULL;
		}
		break;
	default:
		ASSERT(FALSE); // implementation
		return NULL;
	}
}


BOOL CaVsdaLoadSchema::LoadSchema ()
{
	USES_CONVERSION;
	ASSERT(!m_strFile.IsEmpty());
	if (m_strFile.IsEmpty())
		return FALSE;
	if (!CheckSchemaFile(m_strFile, CLSID_SchemaDiff))
	{
		CString strMsg = _T("Unknown file");
		AfxMessageBox(strMsg);
		return FALSE;
	}

	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE;
	DWORD grfMode2= STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE ;
	IStream* pStream = NULL;
	m_pCompoundFile = new CaCompoundFile(m_strFile, grfMode);
	IStorage* pRoot = m_pCompoundFile->GetRootStorage();
	ASSERT(pRoot);
	if (!pRoot)
		return FALSE;
	//
	// Load Schema Information:
	pStream = m_pCompoundFile->OpenStream(NULL, _T("SCHEMAINFO"), grfMode2);
	if (pStream)
	{
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);

		ar >> m_strDate;
		ar >> m_nVersion;
		ar >> m_strNode;
		ar >> m_strDatabase;
		ar >> m_strSchema;

		ar.Flush();
		pStream->Release();
	}
	return TRUE;
}

void CaVsdaLoadSchema::GetListUser(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	ASSERT (m_strDatabase.IsEmpty()); // Must be an installation
	if (!m_strDatabase.IsEmpty())
		return;

	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = m_pCompoundFile->OpenStream(NULL, _T("USER"), grfMode);
	if (pStream)
	{
		CTypedPtrList<CObList, CaUserDetail*> listDetail;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listDetail.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listDetail.IsEmpty())
		{
			CaUserDetail* pObj = listDetail.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
}

void CaVsdaLoadSchema::GetListGroup(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	ASSERT (m_strDatabase.IsEmpty()); // Must be an installation
	if (!m_strDatabase.IsEmpty())
		return;

	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = m_pCompoundFile->OpenStream(NULL, _T("GROUP"), grfMode);
	if (pStream)
	{
		CTypedPtrList<CObList, CaGroupDetail*> listDetail;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listDetail.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listDetail.IsEmpty())
		{
			CaGroupDetail* pObj = listDetail.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
}


void CaVsdaLoadSchema::GetListRole(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	ASSERT (m_strDatabase.IsEmpty()); // Must be an installation
	if (!m_strDatabase.IsEmpty())
		return;

	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = m_pCompoundFile->OpenStream(NULL, _T("ROLE"), grfMode);
	if (pStream)
	{
		CTypedPtrList<CObList, CaRoleDetail*> listDetail;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listDetail.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listDetail.IsEmpty())
		{
			CaRoleDetail* pObj = listDetail.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
}

void CaVsdaLoadSchema::GetListProfile(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	ASSERT (m_strDatabase.IsEmpty()); // Must be an installation
	if (!m_strDatabase.IsEmpty())
		return;

	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = m_pCompoundFile->OpenStream(NULL, _T("PROFILE"), grfMode);
	if (pStream)
	{
		CTypedPtrList<CObList, CaProfileDetail*> listDetail;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listDetail.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listDetail.IsEmpty())
		{
			CaProfileDetail* pObj = listDetail.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
}

void CaVsdaLoadSchema::GetListLocation(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	ASSERT (m_strDatabase.IsEmpty()); // Must be an installation
	if (!m_strDatabase.IsEmpty())
		return;

	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = m_pCompoundFile->OpenStream(NULL, _T("LOCATION"), grfMode);
	if (pStream)
	{
		CTypedPtrList<CObList, CaLocationDetail*> listDetail;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listDetail.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listDetail.IsEmpty())
		{
			CaLocationDetail* pObj = listDetail.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
}

void CaVsdaLoadSchema::GetListGrantee(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	IStorage* pRoot = NULL;
	IStorage* pIDatabase = NULL;
	IStorage* pIRootObject = NULL;
	IStorage* pIObject = NULL;
	IStream* pStream = NULL;
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	int nSubObjectType = pInfo->GetSubObjectType();
	ASSERT(nSubObjectType != -1);
	switch (nSubObjectType)
	{
	case OBT_INSTALLATION:
		pStream = m_pCompoundFile->OpenStream(NULL, _T("GRANTEE"), grfMode);
		break;
	case OBT_DATABASE:
		pRoot = m_pCompoundFile->GetRootStorage();
		if (pRoot)
		{
			CString strDatabase = pInfo->GetDatabase();
			pIDatabase = FindIStorageDatabase(strDatabase);
			if (pIDatabase)
			{
				pStream = m_pCompoundFile->OpenStream(pIDatabase, _T("GRANTEE"), grfMode);
			}
		}
		break;
	case OBT_TABLE:
	case OBT_VIEW:
	case OBT_PROCEDURE:
	case OBT_SEQUENCE:
	case OBT_DBEVENT:
		pRoot = m_pCompoundFile->GetRootStorage();
		if (pRoot)
		{
			CString strDatabase = pInfo->GetDatabase();
			pIDatabase = FindIStorageDatabase(strDatabase);
			if (pIDatabase)
			{
				CString strStorageName = _T("");
				switch (nSubObjectType)
				{
				case OBT_TABLE:
					strStorageName = _T("TABLE");
					break;
				case OBT_VIEW:
					strStorageName = _T("VIEW");
					break;
				case OBT_PROCEDURE:
					strStorageName = _T("PROCEDURE");
					break;
				case OBT_SEQUENCE:
					strStorageName = _T("SEQUENCE");
					break;
				case OBT_DBEVENT:
					strStorageName = _T("DBEVENT");
					break;
				default:
					ASSERT(FALSE);
					break;
				}
				if (!strStorageName.IsEmpty())
					pIRootObject = m_pCompoundFile->OpenStorage(pIDatabase, strStorageName, grfMode);
				if (pIRootObject)
				{
					CString strObject = pInfo->GetItem2();
					CString strObjectOwner = pInfo->GetItem2Owner();
					pIObject = FindIRootTVDPObject(pIRootObject, nSubObjectType, strObject, strObjectOwner);
					if (pIObject)
					{
						pStream = m_pCompoundFile->OpenStream(pIObject, _T("GRANTEE"), grfMode);
					}
				}
			}
		}
		break;
	default:
		ASSERT(FALSE);
		return;
		break;
	}

	if (pStream)
	{
		CTypedPtrList<CObList, CaGrantee*> listDetail;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listDetail.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listDetail.IsEmpty())
		{
			CaGrantee* pObj = listDetail.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
	if (pIObject)
		pIObject->Release();
	if (pIRootObject)
		pIRootObject->Release();
	if (pIDatabase)
		pIDatabase->Release();
}

void CaVsdaLoadSchema::GetListAlarm(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	IStorage* pRoot = NULL;
	IStorage* pIDatabase = NULL;
	IStorage* pIRootObject = NULL;
	IStorage* pIObject = NULL;
	IStream* pStream = NULL;
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	int nSubObjectType = pInfo->GetSubObjectType();
	ASSERT(nSubObjectType != -1);
	switch (nSubObjectType)
	{
	case OBT_INSTALLATION:
		pStream = m_pCompoundFile->OpenStream(NULL, _T("ALARM"), grfMode);
		break;
	case OBT_DATABASE:
		pRoot = m_pCompoundFile->GetRootStorage();
		if (pRoot)
		{
			CString strDatabase = pInfo->GetDatabase();
			pIDatabase = FindIStorageDatabase(strDatabase);
			if (pIDatabase)
			{
				pStream = m_pCompoundFile->OpenStream(pIDatabase, _T("ALARM"), grfMode);
			}
		}
		break;
	case OBT_TABLE:
		pRoot = m_pCompoundFile->GetRootStorage();
		if (pRoot)
		{
			CString strDatabase = pInfo->GetDatabase();
			pIDatabase = FindIStorageDatabase (strDatabase);
			if (pIDatabase)
			{
				CString strStorageName = _T("TABLE");
				pIRootObject = m_pCompoundFile->OpenStorage(pIDatabase, strStorageName, grfMode);
				if (pIRootObject)
				{
					CString strObject = pInfo->GetItem2();
					CString strObjectOwner = pInfo->GetItem2Owner();
					pIObject = FindIRootTVDPObject(pIRootObject, nSubObjectType, strObject, strObjectOwner);
					if (pIObject)
					{
						pStream = m_pCompoundFile->OpenStream(pIObject, _T("ALARM"), grfMode);
					}
				}
			}
		}
		break;
	default:
		ASSERT(FALSE);
		return;
		break;
	}

	if (pStream)
	{
		CTypedPtrList<CObList, CaAlarm*> listDetail;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listDetail.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listDetail.IsEmpty())
		{
			CaAlarm* pObj = listDetail.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
	if (pIObject)
		pIObject->Release();
	if (pIRootObject)
		pIRootObject->Release();
	if (pIDatabase)
		pIDatabase->Release();
}

IStorage* CaVsdaLoadSchema::FindIStorageDatabase(LPCTSTR lpszDatabase)
{
	CString strEntry = _T("");
	CTypedPtrList<CObList, CaDatabase*> listdb;
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = m_pCompoundFile->OpenStream(NULL, _T("DBNAMExENTRY"), grfMode);
	if (pStream)
	{
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listdb.Serialize(ar);
		ar.Flush();
		pStream->Release();
	}

	int nIndex = 0;
	POSITION pos = listdb.GetHeadPosition();
	while (pos != NULL)
	{
		nIndex++;
		CaDatabase* pObj = listdb.GetNext(pos);
		if (pObj->GetName().CompareNoCase (lpszDatabase) == 0)
		{
			strEntry.Format(_T("DB%08d"), nIndex);
			break;
		}
	}
	while (!listdb.IsEmpty())
		delete listdb.RemoveHead();
	if (!strEntry.IsEmpty())
	{
		IStorage* pIDatabase = FindStorage (strEntry);
		return pIDatabase;
	}
	return NULL;
}


void CaVsdaLoadSchema::GetListDatabase(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = m_pCompoundFile->OpenStream(NULL, _T("DBNAMExENTRY"), grfMode);
	if (pStream)
	{
		CTypedPtrList<CObList, CaDatabase*> listdb;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);
		listdb.Serialize(ar);
		ar.Flush();
		pStream->Release();

		while (!listdb.IsEmpty())
		{
			CaDatabase* pObj = listdb.RemoveHead();
			listObject.AddTail(pObj);
		}
	}
}


void CaVsdaLoadSchema::GetListTable(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("TABLE"), grfMode);
		if (pRootObject)
		{
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("TABLExENTRY"), grfMode);
			if (pStream)
			{
				CTypedPtrList<CObList, CaTable*> ls;
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();

				while (!ls.IsEmpty())
				{
					CaTable* pObj = ls.RemoveHead();
					listObject.AddTail(pObj);
				}
			}

			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListView(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("VIEW"), grfMode);
		if (pRootObject)
		{
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("VIEWxENTRY"), grfMode);
			if (pStream)
			{
				CTypedPtrList<CObList, CaView*> ls;
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();

				while (!ls.IsEmpty())
				{
					CaView* pObj = ls.RemoveHead();
					listObject.AddTail(pObj);
				}
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListProcedure(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("PROCEDURE"), grfMode);
		if (pRootObject)
		{
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("PROCxENTRY"), grfMode);
			if (pStream)
			{
				CTypedPtrList<CObList, CaProcedure*> ls;
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();

				while (!ls.IsEmpty())
				{
					CaProcedure* pObj = ls.RemoveHead();
					listObject.AddTail(pObj);
				}
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListSequence(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("SEQUENCE"), grfMode);
		if (pRootObject)
		{
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("SEQUxENTRY"), grfMode);
			if (pStream)
			{
				CTypedPtrList<CObList, CaSequence*> ls;
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();

				while (!ls.IsEmpty())
				{
					CaSequence* pObj = ls.RemoveHead();
					listObject.AddTail(pObj);
				}
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListDBEvent(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("DBEVENT"), grfMode);
		if (pRootObject)
		{
			IStream* pStream = m_pCompoundFile->OpenStream(pRootObject, _T("DBEVENTxENTRY"), grfMode);
			if (pStream)
			{
				CTypedPtrList<CObList, CaDBEvent*> ls;
				COleStreamFile file (pStream);
				CArchive ar(&file, CArchive::load);
				ls.Serialize(ar);
				ar.Flush();
				pStream->Release();

				while (!ls.IsEmpty())
				{
					CaDBEvent* pObj = ls.RemoveHead();
					listObject.AddTail(pObj);
				}
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListSynonym(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);;
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStream* pStream = m_pCompoundFile->OpenStream(pIDatabase, _T("SYNONYM"), grfMode);
		if (pStream)
		{
			CTypedPtrList<CObList, CaSynonymDetail*> listDetail;
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::load);
			listDetail.Serialize(ar);
			ar.Flush();
			pStream->Release();

			while (!listDetail.IsEmpty())
			{
				CaSynonymDetail* pObj = listDetail.RemoveHead();
				listObject.AddTail(pObj);
			}
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListIndex(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strTable = pInfo->GetItem2();
	CString strTableOwner = pInfo->GetItem2Owner();
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);;
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("TABLE"), grfMode);
		if (pRootObject)
		{
			IStorage* pRootTable = FindIRootTVDPObject(pRootObject, OBT_TABLE, strTable, strTableOwner);
			if (pRootTable)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pRootTable, _T("INDEX"), grfMode);
				if (pStream)
				{
					CTypedPtrList<CObList, CaIndexDetail*> listDetail;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					listDetail.Serialize(ar);
					ar.Flush();
					pStream->Release();

					while (!listDetail.IsEmpty())
					{
						CaIndexDetail* pObj = listDetail.RemoveHead();
						listObject.AddTail(pObj);
					}
				}
				pRootTable->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListRule(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strTable = pInfo->GetItem2();
	CString strTableOwner = pInfo->GetItem2Owner();
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);;
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("TABLE"), grfMode);
		if (pRootObject)
		{
			IStorage* pRootTable = FindIRootTVDPObject(pRootObject, OBT_TABLE, strTable, strTableOwner);
			if (pRootTable)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pRootTable, _T("RULE"), grfMode);
				if (pStream)
				{
					CTypedPtrList<CObList, CaRuleDetail*> listDetail;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					listDetail.Serialize(ar);
					ar.Flush();
					pStream->Release();

					while (!listDetail.IsEmpty())
					{
						CaRuleDetail* pObj = listDetail.RemoveHead();
						listObject.AddTail(pObj);
					}
				}
				pRootTable->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetListIntegrity(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CString strTable = pInfo->GetItem2();
	CString strTableOwner = pInfo->GetItem2Owner();
	CString strDatabase = pInfo->GetDatabase();
	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);;
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("TABLE"), grfMode);
		if (pRootObject)
		{
			IStorage* pRootTable = FindIRootTVDPObject(pRootObject, OBT_TABLE, strTable, strTableOwner);
			if (pRootTable)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pRootTable, _T("INTEGRITY"), grfMode);
				if (pStream)
				{
					CTypedPtrList<CObList, CaIntegrityDetail*> listDetail;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					listDetail.Serialize(ar);
					ar.Flush();
					pStream->Release();

					while (!listDetail.IsEmpty())
					{
						CaIntegrityDetail* pObj = listDetail.RemoveHead();
						listObject.AddTail(pObj);
					}
				}
				pRootTable->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}


void CaVsdaLoadSchema::GetDetailUser(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strName = pDetail->GetName();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListUser(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaUserDetail* pObj = (CaUserDetail*)listObject.GetNext(pos);
		if (pObj->GetName().CompareNoCase(strName) == 0)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailGroup(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strName = pDetail->GetName();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListGroup(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaGroupDetail* pObj = (CaGroupDetail*)listObject.GetNext(pos);
		if (pObj->GetName().CompareNoCase(strName) == 0)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailRole(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strName = pDetail->GetName();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListRole(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaRoleDetail* pObj = (CaRoleDetail*)listObject.GetNext(pos);
		if (pObj->GetName().CompareNoCase(strName) == 0)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailProfile(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strName = pDetail->GetName();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListProfile(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaProfileDetail* pObj = (CaProfileDetail*)listObject.GetNext(pos);
		if (pObj->GetName().CompareNoCase(strName) == 0)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailLocation(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strName = pDetail->GetName();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListLocation(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaLocationDetail* pObj = (CaLocationDetail*)listObject.GetNext(pos);
		if (pObj->GetName().CompareNoCase(strName) == 0)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailGrantee(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strName = pDetail->GetName();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListGrantee(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaGrantee* pObj = (CaGrantee*)listObject.GetNext(pos);
		if (pObj->GetName().CompareNoCase(strName) == 0)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}


void CaVsdaLoadSchema::GetDetailAlarm(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	int nSecurityUser = ((CaAlarm*)pDetail)->GetSecurityNumber();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListAlarm(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaAlarm* pObj = (CaAlarm*)listObject.GetNext(pos);
		if (pObj->GetSecurityNumber() == nSecurityUser)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailDatabase(CaDBObject*& pDetail)
{
	IStorage* pIDatabase = FindIStorageDatabase (pDetail->GetName());
	pDetail = NULL;
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStream* pStream = m_pCompoundFile->OpenStream(pIDatabase, _T("DATABASE"), grfMode);
		if (pStream)
		{
			CaDatabaseDetail* pDetailObject = NULL;
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::load);
			ar >> pDetailObject;
			pDetail = pDetailObject;
			ar.Flush();
			pStream->Release();

			pDetail = pDetailObject;
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetDetailTable(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strDatabase = pInfo->GetDatabase();
	CString strObject = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;

	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("TABLE"), grfMode);
		if (pRootObject)
		{
			IStorage* pIObject = FindIRootTVDPObject(pRootObject, OBT_TABLE, strObject, strOwner);
			if (pIObject)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pIObject, _T("TABLE"), grfMode);
				if (pStream)
				{
					CaTableDetail* pDetailObject = NULL;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					ar >> pDetailObject;
					pDetail = pDetailObject;
					ar.Flush();
					pStream->Release();
				}
				pIObject->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetDetailView(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strDatabase = pInfo->GetDatabase();
	CString strObject = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;

	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("VIEW"), grfMode);
		if (pRootObject)
		{
			IStorage* pIObject = FindIRootTVDPObject(pRootObject, OBT_VIEW, strObject, strOwner);
			if (pIObject)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pIObject, _T("VIEW"), grfMode);
				if (pStream)
				{
					CaViewDetail* pDetailObject = NULL;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					ar >> pDetailObject;
					pDetail = pDetailObject;
					ar.Flush();
					pStream->Release();
				}
				pIObject->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetDetailProcedure(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strDatabase = pInfo->GetDatabase();
	CString strObject = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;

	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("PROCEDURE"), grfMode);
		if (pRootObject)
		{
			IStorage* pIObject = FindIRootTVDPObject(pRootObject, OBT_PROCEDURE, strObject, strOwner);
			if (pIObject)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pIObject, _T("PROCEDURE"), grfMode);
				if (pStream)
				{
					CaProcedureDetail* pDetailObject = NULL;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					ar >> pDetailObject;
					pDetail = pDetailObject;
					ar.Flush();
					pStream->Release();
				}
				pIObject->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetDetailSequence(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strDatabase = pInfo->GetDatabase();
	CString strObject = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;

	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("SEQUENCE"), grfMode);
		if (pRootObject)
		{
			IStorage* pIObject = FindIRootTVDPObject(pRootObject, OBT_SEQUENCE, strObject, strOwner);
			if (pIObject)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pIObject, _T("SEQUENCE"), grfMode);
				if (pStream)
				{
					CaSequenceDetail* pDetailObject = NULL;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					ar >> pDetailObject;
					pDetail = pDetailObject;
					ar.Flush();
					pStream->Release();
				}
				pIObject->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetDetailDBEvent(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strDatabase = pInfo->GetDatabase();
	CString strObject = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;

	IStorage* pIDatabase = FindIStorageDatabase (strDatabase);
	if (pIDatabase)
	{
		DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
		IStorage* pRootObject = m_pCompoundFile->OpenStorage(pIDatabase, _T("DBEVENT"), grfMode);
		if (pRootObject)
		{
			IStorage* pIObject = FindIRootTVDPObject(pRootObject, OBT_DBEVENT, strObject, strOwner);
			if (pIObject)
			{
				IStream* pStream = m_pCompoundFile->OpenStream(pIObject, _T("DBEVENT"), grfMode);
				if (pStream)
				{
					CaDBEventDetail* pDetailObject = NULL;
					COleStreamFile file (pStream);
					CArchive ar(&file, CArchive::load);
					ar >> pDetailObject;
					pDetail = pDetailObject;
					ar.Flush();
					pStream->Release();
				}
				pIObject->Release();
			}
			pRootObject->Release();
		}
		pIDatabase->Release();
	}
}

void CaVsdaLoadSchema::GetDetailSynonym(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strName = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListSynonym(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaSynonymDetail* pObj = (CaSynonymDetail*)listObject.GetNext(pos);
		if (!m_strSchema.IsEmpty())
		{
			if (pObj->GetOwner().CompareNoCase(m_strSchema) != 0)
				continue;
		}
		if (pObj->GetName().CompareNoCase(strName) == 0 &&
		    pObj->GetOwner().CompareNoCase(strOwner) == 0)
		{
			pDetail = pObj;
			listObject.RemoveAt(p);
			break;
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailIndex(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strBase = pInfo->GetItem2();
	CString strBaseOwner = pInfo->GetItem2Owner();
	CString strName = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListIndex(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaIndexDetail* pObj = (CaIndexDetail*)listObject.GetNext(pos);
		if (!m_strSchema.IsEmpty())
		{
			if (pObj->GetOwner().CompareNoCase(m_strSchema) != 0)
				continue;
		}
		if (strBase.CompareNoCase(pObj->GetBaseTable()) == 0 && 
		    strBaseOwner.CompareNoCase(pObj->GetBaseTableOwner()) == 0)
		{
			if (pObj->GetName().CompareNoCase(strName) == 0 &&
				pObj->GetOwner().CompareNoCase(strOwner) == 0)
			{
				pDetail = pObj;
				listObject.RemoveAt(p);
				break;
			}
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailRule(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strBase = pInfo->GetItem2();
	CString strBaseOwner = pInfo->GetItem2Owner();
	CString strName = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListRule(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaRuleDetail* pObj = (CaRuleDetail*)listObject.GetNext(pos);
		if (!m_strSchema.IsEmpty())
		{
			if (pObj->GetOwner().CompareNoCase(m_strSchema) != 0)
				continue;
		}
		if (strBase.CompareNoCase(pObj->GetBaseTable()) == 0 && 
		    strBaseOwner.CompareNoCase(pObj->GetBaseTableOwner()) == 0)
		{
			if (pObj->GetName().CompareNoCase(strName) == 0 &&
				pObj->GetOwner().CompareNoCase(strOwner) == 0)
			{
				pDetail = pObj;
				listObject.RemoveAt(p);
				break;
			}
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}

void CaVsdaLoadSchema::GetDetailIntegrity(CaLLQueryInfo* pInfo, CaDBObject*& pDetail)
{
	CString strBase = pInfo->GetItem2();
	CString strBaseOwner = pInfo->GetItem2Owner();
	CString strName = pDetail->GetName();
	CString strOwner= pDetail->GetOwner();
	int nNumber = ((CaIntegrityDetail*)pDetail)->GetNumber();
	pDetail = NULL;
	CTypedPtrList<CObList, CaDBObject*> listObject;
	GetListIntegrity(pInfo, listObject);
	POSITION p, pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaIntegrityDetail* pObj = (CaIntegrityDetail*)listObject.GetNext(pos);
		if (!m_strSchema.IsEmpty())
		{
			if (pObj->GetOwner().CompareNoCase(m_strSchema) != 0)
				continue;
		}
		if (strBase.CompareNoCase(pObj->GetBaseTable()) == 0 && 
		    strBaseOwner.CompareNoCase(pObj->GetBaseTableOwner()) == 0)
		{
			if (pObj->GetNumber() == nNumber &&
				pObj->GetOwner().CompareNoCase(strOwner) == 0)
			{
				pDetail = pObj;
				listObject.RemoveAt(p);
				break;
			}
		}
	}
	while (!listObject.IsEmpty())
		delete listObject.RemoveHead();
}


//
// Functions:
// ************************************************************************************************
static BOOL CheckSchemaFile(LPCTSTR lpszFile, REFCLSID clsId)
{
	USES_CONVERSION;
	HRESULT hr = NOERROR;
	//
	// Check the compound file:
	LPCWSTR wszName = T2CW(lpszFile);
	if (StgIsStorageFile(wszName) != S_OK)
		return FALSE;
	// We have a compound file. Use COM service to open the compound file
	// and obtain a IStorage interface.
	IStorage* pIStorage = NULL;
	hr = StgOpenStorage(
		wszName,
		NULL,
		STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE,
		NULL,
		0,
		&pIStorage);
	if (FAILED(hr))
	{
		throw CeCompoundFileException (hr);
	}

	BOOL bOK = FALSE;
	STATSTG stat;
	if (pIStorage)
	{
		hr = pIStorage->Stat(&stat, STATFLAG_NONAME);
		if (FAILED(hr))
			throw CeCompoundFileException (hr);
		bOK = IsEqualCLSID(stat.clsid, clsId);
		pIStorage->Release();
	}
	return bOK;
}

//
// If lpszUser = NULL then all users are considered.
BOOL VSD_StoreSchema (CaVsdStoreSchema& storeSchema)
{
	//
	// Remove the old stream first (if any) before creating a new one:
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	BOOL bOK = TRUE;
	IStream* pStream = NULL;
/*UKS
	try
	{
*/
		CaCompoundFile cmpFile (storeSchema.m_strFile);
		IStorage* pRoot = cmpFile.GetRootStorage();
		ASSERT(pRoot);
		if (pRoot)
			pRoot->SetClass(CLSID_SchemaDiff);

		pStream =  cmpFile.NewStream(NULL, _T("SCHEMAINFO"), grfMode);
		if (pStream)
		{
			StoreSchemaInfo (storeSchema, cmpFile, pStream);
			pStream->Release();
			pStream = NULL;
		}
		//
		// Store Database(s) and its sub-branches
		VSD_StoreDatabase (pRoot, storeSchema, cmpFile, FALSE);
/*UKS
	}
	catch(...)
	{
		bOK = FALSE;
	}
*/
	return bOK;
}


static BOOL StoreSchemaInfo (CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile, IStream* pStream)
{
	//
	// Date of create/update: (text)->  yyyy-mm-dd HH:MM:SS (2002-09-19 10:50:35)
	// File version:          (int) ->  1
	// Node name:             (text)
	// Database:              (text)
	// user (schema)          (text)
	BOOL bOK = TRUE;
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
//	CString csTemp;
//	csTemp.LoadString(IDS_MSG_WAIT_GENERAL);
//	storeSchema.ShowAnimateTextInfo(csTemp);//_T("General information...")
	int nVersion = 1;
	CTime t = CTime::GetCurrentTime();
	CString date = t.Format(_T("%Y-%m-%d %H:%M:%S"));

	COleStreamFile file (pStream);
	CArchive ar(&file, CArchive::store);
	ar << date;
	ar << nVersion;
	ar << storeSchema.GetNode();
	ar << storeSchema.GetDatabase(); // Empty -> Installation
	ar << storeSchema.GetSchema();   // Empty -> All Schemas (users)

	ar.Flush();
	file.Detach();
	return bOK;
}


static BOOL StoreUser (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaUserDetail* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetDatabase(_T("iidbdb"));
	queryInfo.SetObjectType(OBT_USER);
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_USERS);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Users...")

		INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			CaUserDetail* pDetail = new CaUserDetail(pObj->GetName());
			delete pObj;
			INGRESII_llQueryDetailObject (&queryInfo, pDetail, pMgr);
			lDetail.AddTail(pDetail);
		}

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		lDetail.Serialize(ar);
		ar.Flush();
		file.Detach();
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	while (!lNew.IsEmpty())
		delete lNew.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);

	return bOK;
}

static BOOL StoreGroup (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaGroupDetail* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetDatabase(_T("iidbdb"));
	queryInfo.SetObjectType(OBT_GROUP);
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_GROUPS);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Groups...")

		INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			CaGroupDetail* pDetail = new CaGroupDetail(pObj->GetName());
			delete pObj;
			INGRESII_llQueryDetailObject (&queryInfo, pDetail, pMgr);
			lDetail.AddTail(pDetail);
		}

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		lDetail.Serialize(ar);
		ar.Flush();
		file.Detach();
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	while (!lNew.IsEmpty())
		delete lNew.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);

	return bOK;
}

static BOOL StoreRole (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaRoleDetail* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetDatabase(_T("iidbdb"));
	queryInfo.SetObjectType(OBT_ROLE);
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_ROLES);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Roles...")

		INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			CaRoleDetail* pDetail = new CaRoleDetail(pObj->GetName());
			delete pObj;
			INGRESII_llQueryDetailObject (&queryInfo, pDetail, pMgr);
			lDetail.AddTail(pDetail);
		}

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		lDetail.Serialize(ar);
		ar.Flush();
		file.Detach();
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	while (!lNew.IsEmpty())
		delete lNew.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);

	return bOK;
}

static BOOL StoreProfile (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaProfileDetail* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetDatabase(_T("iidbdb"));
	queryInfo.SetObjectType(OBT_PROFILE);
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_PROFILES);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Profiles...")

		INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			CaProfileDetail* pDetail = new CaProfileDetail(pObj->GetName());
			delete pObj;
			INGRESII_llQueryDetailObject (&queryInfo, pDetail, pMgr);
			lDetail.AddTail(pDetail);
		}

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		lDetail.Serialize(ar);
		ar.Flush();
		file.Detach();
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	while (!lNew.IsEmpty())
		delete lNew.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);

	return bOK;
}

static BOOL StoreLocation(CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaLocationDetail* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetDatabase(_T("iidbdb"));
	queryInfo.SetObjectType(OBT_LOCATION);
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_LOCATIONS);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Locations...")

		INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			CaLocationDetail* pDetail = new CaLocationDetail(pObj->GetName());
			delete pObj;
			INGRESII_llQueryDetailObject (&queryInfo, pDetail, pMgr);
			lDetail.AddTail(pDetail);
		}

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		lDetail.Serialize(ar);
		ar.Flush();
		file.Detach();
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	while (!lNew.IsEmpty())
		delete lNew.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);

	return bOK;
}


static BOOL StoreGrantee (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaGrantee* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetDatabase(_T("iidbdb"));
	queryInfo.SetObjectType(OBT_GRANTEE);
	queryInfo.SetSubObjectType(OBT_INSTALLATION);
	try
	{
//		CString csTemp;
//		csTemp.LoadString(IDS_MSG_WAIT_GRANTEES);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Grantees...")

		INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			lDetail.AddTail((CaGrantee*)pObj);
		}

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		lDetail.Serialize(ar);
		ar.Flush();
		file.Detach();
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	while (!lNew.IsEmpty())
		delete lNew.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);

	return bOK;
}


static BOOL StoreAlarm   (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaAlarm* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetObjectType(OBT_ALARM);
	queryInfo.SetSubObjectType(OBT_INSTALLATION);
	try
	{
//		CString csTemp;
//		csTemp.LoadString(IDS_MSG_WAIT_SEC_ALARMS);
//		storeSchema.ShowAnimateTextInfo(csTemp); //_T("List of Security Alarms...")

		INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			lDetail.AddTail((CaAlarm*)pObj);
		}

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		lDetail.Serialize(ar);
		ar.Flush();
		file.Detach();
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	while (!lNew.IsEmpty())
		delete lNew.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);

	return bOK;
}


BOOL VSD_llQueryObject (CaLLQueryInfo* pInfo, CaVsdaLoadSchema* pLoadedSchema, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	BOOL bOk = TRUE;
	try
	{
		switch (pInfo->GetObjectType())
		{
		case OBT_USER:
			pLoadedSchema->GetListUser(pInfo, listObject);
			break;
		case OBT_GROUP:
			pLoadedSchema->GetListGroup(pInfo, listObject);
			break;
		case OBT_ROLE:
			pLoadedSchema->GetListRole(pInfo, listObject);
			break;
		case OBT_PROFILE:
			pLoadedSchema->GetListProfile(pInfo, listObject);
			break;
		case OBT_LOCATION:
			pLoadedSchema->GetListLocation(pInfo, listObject);
			break;
		case OBT_DATABASE:
			pLoadedSchema->GetListDatabase(pInfo, listObject);
			break;
		case OBT_TABLE:
			pLoadedSchema->GetListTable(pInfo, listObject);
			break;
		case OBT_VIEW:
			pLoadedSchema->GetListView(pInfo, listObject);
			break;
		case OBT_PROCEDURE:
			pLoadedSchema->GetListProcedure(pInfo, listObject);
			break;
		case OBT_SEQUENCE:
			pLoadedSchema->GetListSequence(pInfo, listObject);
			break;
		case OBT_DBEVENT:
			pLoadedSchema->GetListDBEvent(pInfo, listObject);
			break;
		case OBT_SYNONYM:
			pLoadedSchema->GetListSynonym(pInfo, listObject);
			break;
		case OBT_INDEX:
			pLoadedSchema->GetListIndex(pInfo, listObject);
			break;
		case OBT_RULE:
			pLoadedSchema->GetListRule(pInfo, listObject);
			break;
		case OBT_INTEGRITY:
			pLoadedSchema->GetListIntegrity(pInfo, listObject);
			break;
		case OBT_GRANTEE:
			pLoadedSchema->GetListGrantee(pInfo, listObject);
			break;
		case OBT_ALARM:
			pLoadedSchema->GetListAlarm(pInfo, listObject);
			break;
		default:
			//
			// Need to implement the new handlerQueryXyz ?
			ASSERT (FALSE);
			bOk = FALSE;
			break;
		}
	}
	catch (...)
	{
		bOk = FALSE;
		while (!listObject.IsEmpty())
			delete listObject.RemoveHead();
		throw;
	}

	return bOk;
}

BOOL VSD_llQueryDetailObject(CaLLQueryInfo* pInfo, CaVsdaLoadSchema* pLoadedSchema, CaDBObject*& pDetail)
{
	BOOL bOk = TRUE;
	try
	{
		switch (pInfo->GetObjectType())
		{
		case OBT_USER:
			pLoadedSchema->GetDetailUser(pInfo, pDetail);
			break;
		case OBT_GROUP:
			pLoadedSchema->GetDetailGroup(pInfo, pDetail);
			break;
		case OBT_ROLE:
			pLoadedSchema->GetDetailRole(pInfo, pDetail);
			break;
		case OBT_PROFILE:
			pLoadedSchema->GetDetailProfile(pInfo, pDetail);
			break;
		case OBT_LOCATION:
			pLoadedSchema->GetDetailLocation(pInfo, pDetail);
			break;
		case OBT_DATABASE:
			pLoadedSchema->GetDetailDatabase(pDetail);
			break;
		case OBT_TABLE:
			pLoadedSchema->GetDetailTable(pInfo, pDetail);
			break;
		case OBT_VIEW:
			pLoadedSchema->GetDetailView(pInfo, pDetail);
			break;
		case OBT_PROCEDURE:
			pLoadedSchema->GetDetailProcedure(pInfo, pDetail);
			break;
		case OBT_SEQUENCE:
			pLoadedSchema->GetDetailSequence(pInfo, pDetail);
			break;
		case OBT_DBEVENT:
			pLoadedSchema->GetDetailDBEvent(pInfo, pDetail);
			break;
		case OBT_SYNONYM:
			pLoadedSchema->GetDetailSynonym(pInfo, pDetail);
			break;
		case OBT_INDEX:
			pLoadedSchema->GetDetailIndex(pInfo, pDetail);
			break;
		case OBT_RULE:
			pLoadedSchema->GetDetailRule(pInfo, pDetail);
			break;
		case OBT_INTEGRITY:
			pLoadedSchema->GetDetailIntegrity(pInfo, pDetail);
			break;
		case OBT_GRANTEE:
			pLoadedSchema->GetDetailGrantee(pInfo, pDetail);
			break;
		case OBT_ALARM:
			pLoadedSchema->GetDetailAlarm(pInfo, pDetail);
			break;
		default:
			//
			// Need to implement the new handlerQueryXyz ?
			ASSERT (FALSE);
			bOk = FALSE;
			break;
		}
	}
	catch (...)
	{
		pDetail = NULL;
		bOk = FALSE;
		throw;
	}

	return bOk;
}


