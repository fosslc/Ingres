/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : serseq.cpp, implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : Schalk Philippe (schph01)
** Purpose  : Load / Store Sequence in the compound file.
**           
** History:
**
** 07-May-2003 (schph01)
**    SIR #107523 Add Sequence Objects
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 17-Nov-2004 (uk$so01)
**    BUG #113119, optimize the display.
*/

#include "stdafx.h"
#include "serseq.h"
#include "dmlseq.h"
#include "dmlgrant.h"
#include "ingobdml.h"
#include "comsessi.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static BOOL StoreSequence (IStorage* pIRootObject, CaVsdStoreSchema& storeSchema, CaSequenceDetail* pDetail, CaCompoundFile& cmpFile);

// STRUCTURE: 
// pIParent (IStorage, name = database name)  Root of database 
//   |- SEQUENCE (IStorage, name = "SEQUENCE") main root of Sequence
//       |_ SEQUxENTRY   (IStream) List of Sequences
//       |_ <SEQUENCE>*   (IStorage, name = PR%08d where d is the position of Sequence in SEQUxENTRY)
//           |_ SEQUENCE  (IStream) detail of Sequence
//           |_ GRANTEE    (IStream) list of grantees of this Sequence
BOOL VSD_StoreSequence (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile)
{
	BOOL bOK = TRUE;
	CString strError = _T("");
	int nError = 0;
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CTypedPtrList< CObList, CaSequence* > ls;
	CTypedPtrList< CObList, CaSequenceDetail* > lDetail;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;
	storeSchema.SetObjectType(OBT_SEQUENCE);
	
	try
	{
//		CString csTemp;
//		csTemp.LoadString(IDS_MSG_WAIT_SEQUENCES);
//		storeSchema.ShowAnimateTextInfo(csTemp);//_T("List of Sequences...")

		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaSequence* pObj = (CaSequence*)lNew.RemoveHead();
			if (!storeSchema.m_strSchema.IsEmpty())
			{
				if (pObj->GetOwner().CompareNoCase(storeSchema.m_strSchema) != 0)
				{
					delete pObj;
					continue;
				}
			}

			CaSequenceDetail* pDetail = new CaSequenceDetail();
			pDetail->SetItem(pObj->GetName(), pObj->GetOwner());
			storeSchema.SetItem2(pObj->GetName(),pObj->GetOwner());

//			CString strMsg;
//			strMsg.Format(_T("Sequence %s / %s"), (LPCTSTR)storeSchema.GetDatabase(), (LPCTSTR)pDetail->GetName());
//			storeSchema.ShowAnimateTextInfo(strMsg);

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
		pIRootMain =  cmpFile.NewStorage(pIParent, _T("SEQUENCE"), grfMode);
		ASSERT(pIRootMain);
		if (pIRootMain)
		{
			IStream* pStreamEntry=  cmpFile.NewStream(pIRootMain, _T("SEQUxENTRY"), grfMode);
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
				CaSequenceDetail* pDetail = lDetail.RemoveHead();
				CString strName;
				strName.Format(_T("PR%08d"), nIndex);
				pIRootObject =  cmpFile.NewStorage(pIRootMain, strName, grfMode);
				ASSERT(pIRootObject);
				if (pIRootObject)
				{
					StoreSequence (pIRootObject, storeSchema, pDetail, cmpFile);
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
// pIRootObject (IStorage, name = view name)  Root of Sequence 
//   |_ SEQUENCE (IStream) detail of Sequence
//   |_ GRANTEE   (IStream) list of grantees of this Sequence
static BOOL StoreSequence (IStorage* pIRootObject, CaVsdStoreSchema& storeSchema, CaSequenceDetail* pDetail, CaCompoundFile& cmpFile)
{
	DWORD grfMode = STGM_CREATE|STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE;
	CTypedPtrList< CObList, CaGrantee* >  lDetailGrantee;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CmtSessionManager* pMgr = storeSchema.m_pSessionManager;

	try
	{
		CString csTemp;
		csTemp.LoadString(IDS_MSG_WAIT_GRANTEES);
		storeSchema.ShowAnimateTextInfo(csTemp); //_T("List of Grantees...")
		//
		// Get list of grantees for this Sequence (grantee object is in a detail format)
		storeSchema.SetObjectType(OBT_GRANTEE);
		storeSchema.SetSubObjectType(OBT_SEQUENCE);
		storeSchema.SetItem2(pDetail->GetName(), pDetail->GetOwner());
		INGRESII_llQueryObject (&storeSchema, lNew, pMgr);
		while (!lNew.IsEmpty())
		{
			CaDBObject* pObj = lNew.RemoveHead();
			lDetailGrantee.AddTail((CaGrantee*)pObj);
		}

		//
		// Store the detail of this object and its children:
		IStream* pStreamDetail =  cmpFile.NewStream(pIRootObject, _T("SEQUENCE"), grfMode);
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



