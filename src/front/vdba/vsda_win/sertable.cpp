/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : sertable.cpp, implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store Table in the compound file.
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
#include "sertable.h"
#include "dmltable.h"
#include "dmlindex.h"
#include "dmlinteg.h"
#include "dmlrule.h"
#include "dmlgrant.h"
#include "dmlalarm.h"
#include "ingobdml.h"
#include "comsessi.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL StoreTable (IStorage* pIRootObject, CaTableDetail* pDetail, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile);


//
// STRUCTURE: 
// pIParent (IStorage, name = database name)  Root of database 
//   |- TABLE (IStorage, name = "TABLE") main root of table
//      |_ TABLExENTRY  (IStream) List of tables
//      |_ <TABLE>*      (IStorage, name = TB%08d where d is the position of table in TABLExENTRY)
//          |_ TABLE     (IStream) detail of table
//          |_ INDEX     (IStream) list of indices in this table
//          |_ RULE      (IStream) list of rules in this table
//          |_ INTEGRITY (IStream) list of integrities in this table
//          |_ GRANTEE   (IStream) list of grantees of this table
//          |_ ALARM     (IStream) list of alarms in this table
BOOL VSD_StoreTable (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaTable* > ls;
	CTypedPtrList< CObList, CaTableDetail* > lDetail;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	storeSchema.SetObjectType(OBT_TABLE);
	
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_TABLES);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Tables...")
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);

		while (!lNew.IsEmpty())
		{
			CaTable* pObj = (CaTable*)lNew.RemoveHead();
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}

			CaTableDetail* pDetail = new CaTableDetail();
			pDetail->SetQueryFlag(DTQUERY_ALL);
			pDetail->SetItem(pObj->GetName(), pObj->GetOwner());
			storeSchema.SetItem2(pObj->GetName(),pObj->GetOwner());

			CString strMsg;
			strMsg.Format(_T("Table %s / %s"), (LPCTSTR)storeSchema.GetDatabase(), (LPCTSTR)pDetail->GetName());
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
		while (!lNew.IsEmpty())
			delete lNew.RemoveHead();
		while (!ls.IsEmpty())
			delete ls.RemoveHead();
		while (!lDetail.IsEmpty())
			delete lDetail.RemoveHead();
		throw CeSqlException (strError, nError);
	}

	bOK = TRUE;
	try
	{
		IStorage* pIRootMain = NULL;
		IStorage* pIRootObject = NULL;
		pIRootMain =  cmpFile.NewStorage(pIParent, _T("TABLE"), grfMode);
		ASSERT(pIRootMain);
		if (pIRootMain)
		{
			IStream* pStreamEntry=  cmpFile.NewStream(pIRootMain, _T("TABLExENTRY"), grfMode);
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
				CaTableDetail* pDetail = lDetail.RemoveHead();
				CString strName;
				strName.Format(_T("TB%08d"), nIndex);
				pIRootObject =  cmpFile.NewStorage(pIRootMain, strName, grfMode);
				ASSERT(pIRootObject);
				if (pIRootObject)
				{
					StoreTable (pIRootObject, pDetail, storeSchema, cmpFile);
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
// pIRootObject (IStorage, name = table name)  Root of table 
//   |_ TABLE     (IStream) detail of table
//   |_ INDEX     (IStream) list of indices in this table
//   |_ RULE      (IStream) list of rules in this table
//   |_ INTEGRITY (IStream) list of integrities in this table
//   |_ GRANTEE   (IStream) list of grantees of this table
//   |_ ALARM     (IStream) list of alarms in this table
static BOOL StoreTable (IStorage* pIRootObject, CaTableDetail* pDetail, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile)
{
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CTypedPtrList< CObList, CaIndexDetail* > lDetailIndex;
	CTypedPtrList< CObList, CaRuleDetail* >  lDetailRule;
	CTypedPtrList< CObList, CaIntegrityDetail* >  lDetailIntegrity;
	CTypedPtrList< CObList, CaGrantee* >  lDetailGrantee;
	CTypedPtrList< CObList, CaAlarm* >  lDetailAlrm;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;

	try
	{
//		CString csTemp;
//		csTemp.LoadString(IDS_MSG_WAIT_INDICES);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Indices...")
		//
		// Get list of indices for this table
		storeSchema.SetObjectType(OBT_INDEX);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			/*
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}
			*/
			CaIndexDetail* pIdxDetail = new CaIndexDetail(pObj->GetName(), pObj->GetOwner());
			lDetailIndex.AddTail(pIdxDetail);
			INGRESII_llQueryDetailObject (&storeSchema, pIdxDetail, pMgr);

			delete pObj;
		}
//		csTemp.LoadString(IDS_MSG_WAIT_RULES);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Rules...")
		//
		// Get list of rules for this table
		storeSchema.SetObjectType(OBT_RULE);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			/*
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}
			*/

			CaRuleDetail* pRuleDetail = new CaRuleDetail(pObj->GetName(), pObj->GetOwner());
			lDetailRule.AddTail(pRuleDetail);
			INGRESII_llQueryDetailObject (&storeSchema, pRuleDetail, pMgr);

			delete pObj;
		}
//		csTemp.LoadString(IDS_MSG_WAIT_INTEGRITIES);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Integrities...")
		//
		// Get list of integrities for this table
		storeSchema.SetObjectType(OBT_INTEGRITY);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			/* No owner for integrity
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}
			*/

			CaIntegrityDetail* pIntegrityDetail = new CaIntegrityDetail(pObj->GetName(), pObj->GetOwner());
			pIntegrityDetail->SetNumber(((CaIntegrity*)pObj)->GetNumber());
			lDetailIntegrity.AddTail(pIntegrityDetail);
			INGRESII_llQueryDetailObject (&storeSchema, pIntegrityDetail, pMgr);

			delete pObj;
		}
//		csTemp.LoadString(IDS_MSG_WAIT_GRANTEES);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Grantees...")
		//
		// Get list of grantees for this table (grantee object is in a detail format)
		storeSchema.SetObjectType(OBT_GRANTEE);
		storeSchema.SetSubObjectType(OBT_TABLE);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			lDetailGrantee.AddTail((CaGrantee*)pObj);
		}
//		csTemp.LoadString(IDS_MSG_WAIT_SECURITY_ALARMS);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Security Alarms...")
		//
		// Get list of alarms for this table (alarm object is in a detail format)
		storeSchema.SetObjectType(OBT_ALARM);
		storeSchema.SetSubObjectType(OBT_TABLE);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			lDetailAlrm.AddTail((CaAlarm*)pObj);
		}

		//
		// Store the detail of this object and its children:
		IStream* pStreamDetail =  cmpFile.NewStream(pIRootObject, _T("TABLE"), grfMode);
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
		IStream* pStreamIndex =  cmpFile.NewStream(pIRootObject, _T("INDEX"), grfMode);
		if (pStreamIndex)
		{
			COleStreamFile file (pStreamIndex);
			CArchive ar(&file, CArchive::store);
			lDetailIndex.Serialize(ar);
			ar.Flush();
			file.Detach();

			pStreamIndex->Release();
			pStreamIndex = NULL;
		}
		IStream* pStreamRule =  cmpFile.NewStream(pIRootObject, _T("RULE"), grfMode);
		if (pStreamRule)
		{
			COleStreamFile file (pStreamRule);
			CArchive ar(&file, CArchive::store);
			lDetailRule.Serialize(ar);
			ar.Flush();
			file.Detach();

			pStreamRule->Release();
			pStreamRule = NULL;
		}
		IStream* pStreamIntegrity =  cmpFile.NewStream(pIRootObject, _T("INTEGRITY"), grfMode);
		if (pStreamIntegrity)
		{
			COleStreamFile file (pStreamIntegrity);
			CArchive ar(&file, CArchive::store);
			lDetailIntegrity.Serialize(ar);
			ar.Flush();
			file.Detach();

			pStreamIntegrity->Release();
			pStreamIntegrity = NULL;
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
		IStream* pStreamAlarm =  cmpFile.NewStream(pIRootObject, _T("ALARM"), grfMode);
		if (pStreamAlarm)
		{
			COleStreamFile file (pStreamAlarm);
			CArchive ar(&file, CArchive::store);
			lDetailAlrm.Serialize(ar);
			ar.Flush();
			file.Detach();

			pStreamAlarm->Release();
			pStreamAlarm = NULL;
		}
		while (!lDetailIndex.IsEmpty())
			delete lDetailIndex.RemoveHead();
		while (!lDetailRule.IsEmpty())
			delete lDetailRule.RemoveHead();
		while (!lDetailIntegrity.IsEmpty())
			delete lDetailIntegrity.RemoveHead();
		while (!lDetailGrantee.IsEmpty())
			delete lDetailGrantee.RemoveHead();
		while (!lDetailAlrm.IsEmpty())
			delete lDetailAlrm.RemoveHead();
	}
	catch (...)
	{
		while (!lDetailIndex.IsEmpty())
			delete lDetailIndex.RemoveHead();
		while (!lDetailRule.IsEmpty())
			delete lDetailRule.RemoveHead();
		while (!lDetailIntegrity.IsEmpty())
			delete lDetailIntegrity.RemoveHead();
		while (!lDetailGrantee.IsEmpty())
			delete lDetailGrantee.RemoveHead();
		while (!lDetailAlrm.IsEmpty())
			delete lDetailAlrm.RemoveHead();
		throw;
	}

	return TRUE;
}

