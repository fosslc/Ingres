/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : serproc.cpp, implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store Procedure in the compound file.
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
#include "serproc.h"
#include "dmlproc.h"
#include "dmlgrant.h"
#include "ingobdml.h"
#include "comsessi.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static BOOL StoreProcedure (IStorage* pIRootObject, CaVsdStoreSchema& storeSchema, CaProcedureDetail* pDetail, CaCompoundFile& cmpFile);

// STRUCTURE: 
// pIParent (IStorage, name = database name)  Root of database 
//   |- PROCEDURE (IStorage, name = "PROCEDURE") main root of procedure
//       |_ PROCxENTRY   (IStream) List of Procedures
//       |_ <PROCEDURE>*   (IStorage, name = PR%08d where d is the position of procedure in PROCxENTRY)
//           |_ PROCEDURE  (IStream) detail of procedure
//           |_ GRANTEE    (IStream) list of grantees of this procedure
BOOL VSD_StoreProcedure (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaProcedure* > ls;
	CTypedPtrList< CObList, CaProcedureDetail* > lDetail;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	storeSchema.SetObjectType(OBT_PROCEDURE);
	
	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_PROCEDURES);
		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Procedures...")

		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaProcedure* pObj = (CaProcedure*)lNew.RemoveHead();
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}

			CaProcedureDetail* pDetail = new CaProcedureDetail();
			pDetail->SetItem(pObj->GetName(), pObj->GetOwner());
			storeSchema.SetItem2(pObj->GetName(),pObj->GetOwner());

			CString strMsg;
			strMsg.Format(_T("Procedure %s / %s"), (LPCTSTR)storeSchema.GetDatabase(), (LPCTSTR)pDetail->GetName());
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

	try
	{
		IStorage* pIRootMain = NULL;
		IStorage* pIRootObject = NULL;
		pIRootMain =  cmpFile.NewStorage(pIParent, _T("PROCEDURE"), grfMode);
		ASSERT(pIRootMain);
		if (pIRootMain)
		{
			IStream* pStreamEntry=  cmpFile.NewStream(pIRootMain, _T("PROCxENTRY"), grfMode);
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
				CaProcedureDetail* pDetail = lDetail.RemoveHead();
				CString strName;
				strName.Format(_T("PR%08d"), nIndex);
				pIRootObject =  cmpFile.NewStorage(pIRootMain, strName, grfMode);
				ASSERT(pIRootObject);
				if (pIRootObject)
				{
					StoreProcedure (pIRootObject, storeSchema, pDetail, cmpFile);
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
	return TRUE;
}

//
// STRUCTURE: 
// pIRootObject (IStorage, name = view name)  Root of procedure 
//   |_ PROCEDURE (IStream) detail of procedure
//   |_ GRANTEE   (IStream) list of grantees of this procedure
static BOOL StoreProcedure (IStorage* pIRootObject, CaVsdStoreSchema& storeSchema, CaProcedureDetail* pDetail, CaCompoundFile& cmpFile)
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
		// Get list of grantees for this procedure (grantee object is in a detail format)
		storeSchema.SetObjectType(OBT_GRANTEE);
		storeSchema.SetSubObjectType(OBT_PROCEDURE);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			lDetailGrantee.AddTail((CaGrantee*)pObj);
		}

		//
		// Store the detail of this object and its children:
		IStream* pStreamDetail =  cmpFile.NewStream(pIRootObject, _T("PROCEDURE"), grfMode);
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



