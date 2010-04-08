/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : serdbase.cpp, Implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store Database in the compound file.
**           
** History:
**
** 18-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 07-May-2003 (schph01)
**    SIR #107523, Add Sequence Objects
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 17-Nov-2004 (uk$so01)
**    BUG #113119, optimize the display.
*/

#include "stdafx.h"
#include "serdbase.h"
#include "sertable.h"
#include "serview.h"
#include "serproc.h"
#include "serseq.h"
#include "serdbev.h"
#include "dmldbase.h"
#include "dmlsynon.h"
#include "dmlgrant.h"
#include "dmlalarm.h"
#include "comsessi.h"
#include "ingobdml.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL StoreDatabase (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile);
static BOOL StoreSynonym  (CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreGrantee  (CaVsdStoreSchema& storeSchema, IStream* pStream);
static BOOL StoreAlarm    (CaVsdStoreSchema& storeSchema, IStream* pStream);


// STRUCTURE: 
// pIParent
//   |_ DBNAMExENTRY   (IStream) List of databases
//   |_ <DB NAME>*     (IStorage, name = DB%08d where d is the position of database in DBNAMExENTRY) 
//     |_ DATABASE     (IStream). Detail of database
//     |_ SYNONYM      (IStream)  list of synonyms of in this database
//     |_ GRANTEE      (IStream)  list of grantees of database
//     |_ ALARM        (IStream)  list of alarm of database
//     |_ <TABLE>      (IStorage)
//     |_ <VIEW>       (IStorage)
//     |_ <PROCEUDURE> (IStorage)
//     |_ <SEQUENCE>   (IStorage)
//     |_ <DBEVENT>    (IStorage)
BOOL VSD_StoreDatabase (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile, BOOL bInstallation)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	//
	// Remove the old stream first (if any) before creating a new one:
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	IStream* pStream = NULL;
	IStorage* pIDatabase = NULL;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetObjectType(OBT_DATABASE);
	CString strSaveDatabase = storeSchema.GetDatabase();
	int     nSaveObjectType = storeSchema.GetObjectType();

	try
	{
		CTypedPtrList< CObList, CaDatabase* > ldb;
		if (bInstallation) // All databases of a node
		{
			CString csTemp;
			csTemp.LoadString(IDS_MSG_WAIT_DB);//_T("List of Databases...")
			storeSchema.ShowAnimateTextInfo(csTemp);

			INGRESII_llQueryObject (&queryInfo, lNew, pMgr);
		}
		else
		{
			CaDatabase* pDb = new CaDatabase (queryInfo.GetDatabase(), _T(""));
			lNew.AddTail(pDb);
		}
		//
		// DBNAMExENTRY:
		while (!lNew.IsEmpty())
		{
			CaDatabase* pDb = (CaDatabase*)lNew.RemoveHead();
			ldb.AddTail(pDb);
		}
		pStream =  cmpFile.NewStream(pIParent, _T("DBNAMExENTRY") , grfMode);
		if (pStream)
		{
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::store);
			ldb.Serialize(ar);
			ar.Flush();
			file.Detach();
			pStream->Release();
		}

		CString strEntry = _T("");
		int nIndex = 0;
		while (!ldb.IsEmpty())
		{
			nIndex++;

			strEntry.Format(_T("DB%08d"), nIndex);
			CaDatabase* pDb = ldb.RemoveHead();
			CString strDatabase = pDb->GetName();
			delete pDb;

			// 
			// Root of one database:
			pIDatabase =  cmpFile.NewStorage(pIParent, strEntry, grfMode);
			ASSERT(pIDatabase);
			if (pIDatabase)
			{
				storeSchema.SetDatabase(strDatabase);
				StoreDatabase (pIDatabase, storeSchema, cmpFile);
				pIDatabase->Commit(STGC_OVERWRITE);
				pIDatabase->Release();
			}
		}
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}
	storeSchema.SetDatabase(strSaveDatabase);
	storeSchema.SetObjectType(nSaveObjectType);
	if (!bOK)
		throw CeSqlException (strError, nError);
	return bOK;
}


static BOOL StoreDatabase (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile)
{
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CaDatabaseDetail* pDetail = NULL;
	IStream* pStream = NULL;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetObjectType(OBT_DATABASE);
	HWND hwndAnimate = storeSchema.GetHwndAnimate();
	//
	// Detail of database
	pStream =  cmpFile.NewStream(pIParent, _T("DATABASE") , grfMode);
	if (pStream)
	{
		CString strMsg;
		strMsg.Format(_T("Database %s"), (LPCTSTR)storeSchema.GetDatabase());
		storeSchema.ShowAnimateTextInfo(strMsg);

		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::store);
		pDetail = new CaDatabaseDetail(storeSchema.GetDatabase(), _T(""));
		INGRESII_llQueryDetailObject (&queryInfo, pDetail, pMgr);

		ar << pDetail;
		ar.Flush();
		file.Detach();

		delete pDetail;
		pStream->Release();
	}
	//
	// Synonyms in this database
	pStream =  cmpFile.NewStream(pIParent, _T("SYNONYM") , grfMode);
	if (pStream)
	{
		StoreSynonym  (storeSchema, pStream);
		pStream->Release();
	}
	//
	// Grantees of this database
	pStream =  cmpFile.NewStream(pIParent, _T("GRANTEE") , grfMode);
	if (pStream)
	{
		StoreGrantee  (storeSchema, pStream);
		pStream->Release();
	}
	//
	// Alarms of this database
	pStream =  cmpFile.NewStream(pIParent, _T("ALARM") , grfMode);
	if (pStream)
	{
		StoreAlarm (storeSchema, pStream);
		pStream->Release();
	}

	VSD_StoreTable (pIParent, storeSchema, cmpFile);
	VSD_StoreView (pIParent, storeSchema, cmpFile);
	VSD_StoreProcedure (pIParent, storeSchema, cmpFile);
	VSD_StoreSequence (pIParent, storeSchema, cmpFile);
	VSD_StoreDBEvent (pIParent, storeSchema, cmpFile);

	return TRUE;
}

static BOOL StoreSynonym (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaSynonymDetail* > lDetail;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	storeSchema.SetObjectType(OBT_SYNONYM);

	try
	{
//		CString csTemp;
//		csTemp.LoadString(IDS_MSG_WAIT_SYNONYMS);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Synonyms...")

		storeSchema.SetItem2(_T(""), _T(""));
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);

		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}

			CaSynonymDetail* pDetail = new CaSynonymDetail();
			pDetail->SetItem(pObj->GetName(), pObj->GetOwner());
			storeSchema.SetItem2(pObj->GetName(),pObj->GetOwner());

			INGRESII_llQueryDetailObject (&storeSchema, pDetail, pMgr);
			lDetail.AddTail(pDetail);
		
//			CString strMsg;
//			strMsg.Format(_T("Synonym %s / %s"), (LPCTSTR)storeSchema.GetDatabase(), (LPCTSTR)pObj->GetName());
//			storeSchema.ShowAnimateTextInfo(strMsg);

			delete pObj;
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


static BOOL StoreGrantee  (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaGrantee* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetObjectType(OBT_GRANTEE);
	queryInfo.SetSubObjectType(OBT_DATABASE);
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

static BOOL StoreAlarm (CaVsdStoreSchema& storeSchema, IStream* pStream)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaAlarm* > lDetail;
	CaLLQueryInfo queryInfo (*((CaLLQueryInfo*)&storeSchema));
	queryInfo.SetObjectType(OBT_ALARM);
	queryInfo.SetSubObjectType(OBT_DATABASE);
	try
	{
//		CString csTemp;
//		csTemp.LoadString(IDS_MSG_WAIT_SEC_ALARMS);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Security Alarms...")

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

