/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : serdbev.cpp, implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store DBEvent in the compound file.
**           
** History:
**
** 18-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 17-Nov-2004 (uk$so01)
**    BUG #113119, optimize the display.
*/

#include "stdafx.h"
#include "serdbev.h"
#include "dmldbeve.h"
#include "dmlgrant.h"
#include "ingobdml.h"
#include "comsessi.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static BOOL StoreDBEvent (IStorage* pIRootObject, CaVsdStoreSchema& storeSchema, CaDBEventDetail* pDetail, CaCompoundFile& cmpFile);



// STRUCTURE: 
// pIParent (IStorage, name = database name)  Root of database 
//   |- DBEVENT (IStorage, name = "DBEVENT") main root of dbevent
//      |_ DBEVENTxENTRY(IStream) List of dbevents
//      |_ <DBEVENT>*   (IStorage, name =  EV%08d where d is the position of dbevent in DBEVENTxENTRY)
//          |_ DBEVENT  (IStream) detail of dbevent
//          |_ GRANTEE  (IStream) list of grantees of this dbevent
BOOL VSD_StoreDBEvent (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaDBEvent* > ls;
	CTypedPtrList< CObList, CaDBEventDetail* > lDetail;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	storeSchema.SetObjectType(OBT_DBEVENT);
	
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_DB_EVENTS);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Database Events...")

		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBEvent* pObj = (CaDBEvent*)lNew.RemoveHead();
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}

			CaDBEventDetail* pDetail = new CaDBEventDetail();
			pDetail->SetItem(pObj->GetName(), pObj->GetOwner());
			storeSchema.SetItem2(pObj->GetName(),pObj->GetOwner());

			CString strMsg;
			strMsg.Format(_T("Database Event %s / %s"), (LPCTSTR)storeSchema.GetDatabase(), (LPCTSTR)pDetail->GetName());
			storeSchema.ShowAnimateTextInfo(strMsg);

			INGRESII_llQueryDetailObject (&storeSchema, pDetail, pMgr);
			lDetail.AddTail(pDetail);
			ls.AddTail(pObj);
		}
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}

	if (!bOK)
	{
		while (!ls.IsEmpty())
			delete ls.RemoveHead();
		while (!lDetail.IsEmpty())
			delete lDetail.RemoveHead();
		throw CeSqlException (strError, nError);
	}

	try
	{
		IStorage* pIRootMain = NULL;
		IStorage* pIRootObject = NULL;
		pIRootMain =  cmpFile.NewStorage(pIParent, _T("DBEVENT"), grfMode);
		ASSERT(pIRootMain);
		if (pIRootMain)
		{
			IStream* pStreamEntry=  cmpFile.NewStream(pIRootMain, _T("DBEVENTxENTRY"), grfMode);
			if (pStreamEntry)
			{
				COleStreamFile file (pStreamEntry);
				CArchive ar(&file, CArchive::store);
				ls.Serialize(ar);
				ar.Flush();
				file.Detach();
				pStreamEntry->Release();
				pStreamEntry = NULL;
			}

			int nIndex = 0;
			while (!lDetail.IsEmpty())
			{
				nIndex++;
				CaDBEventDetail* pDetail = lDetail.RemoveHead();
				CString strName;
				strName.Format(_T("EV%08d"), nIndex);
				pIRootObject =  cmpFile.NewStorage(pIRootMain, strName, grfMode);
				ASSERT(pIRootObject);
				if (pIRootObject)
				{
					StoreDBEvent (pIRootObject, storeSchema, pDetail, cmpFile);
					pIRootObject->Release();
				}
				delete pDetail;
			}
			pIRootMain->Release();
		}
	}
	catch (CeSqlException e)
	{
		bOK = FALSE;
		strError = e.GetReason();
		nError   = e.GetErrorCode();
	}

	while (!ls.IsEmpty())
		delete ls.RemoveHead();
	while (!lDetail.IsEmpty())
		delete lDetail.RemoveHead();
	if (!bOK)
		throw CeSqlException (strError, nError);
	return bOK;
}


//
// STRUCTURE: 
// pIRootObject (IStorage, name = dbevent name)  Root of dbevent 
//   |_ DBEVENT   (IStream) detail of dbevent
//   |_ GRANTEE   (IStream) list of grantees of this dbevent
static BOOL StoreDBEvent (IStorage* pIRootObject, CaVsdStoreSchema& storeSchema, CaDBEventDetail* pDetail, CaCompoundFile& cmpFile)
{
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CTypedPtrList< CObList, CaGrantee* >  lDetailGrantee;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;

	try
	{
//		CString csTemp;
//		csTemp.LoadString(IDS_MSG_WAIT_GRANTEES);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Grantees...")
		//
		// Get list of grantees for this dbevent (grantee object is in a detail format)
		storeSchema.SetObjectType(OBT_GRANTEE);
		storeSchema.SetSubObjectType(OBT_DBEVENT);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			lDetailGrantee.AddTail((CaGrantee*)pObj);
		}

		//
		// Store the detail of this object and its children:
		IStream* pStreamDetail =  cmpFile.NewStream(pIRootObject, _T("DBEVENT"), grfMode);
		if (pStreamDetail)
		{
			COleStreamFile file (pStreamDetail);
			CArchive ar(&file, CArchive::store);
			ar << pDetail;
			ar.Flush();
			file.Detach();
			pStreamDetail->Release();
			pStreamDetail = NULL;
		}

		IStream* pStreamGrantee =  cmpFile.NewStream(pIRootObject, _T("GRANTEE"), grfMode);
		if (pStreamGrantee)
		{
			COleStreamFile file (pStreamGrantee);
			CArchive ar(&file, CArchive::store);
			lDetailGrantee.Serialize(ar);
			ar.Flush();
			file.Detach();

			pStreamGrantee->Release();
			pStreamGrantee = NULL;
		}
		while (!lDetailGrantee.IsEmpty())
			delete lDetailGrantee.RemoveHead();
	}
	catch (...)
	{
		while (!lDetailGrantee.IsEmpty())
			delete lDetailGrantee.RemoveHead();
		throw;
	}

	return TRUE;
}
