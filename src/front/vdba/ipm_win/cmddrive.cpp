/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : cmddrive.cpp : implementation file.
**    Project  : INGRES II/ Monitoring (ActiveX Control).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Command line driven (select the specific object in the tree
**               and display its properties on the right pane)
**
** History:
**
** 15-Apr-2002 (uk$so01)
**    Created
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "cmddrive.h"
#include "ipmdml.h"
#include "ipmdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define _IGNORE_NOT_MATCHED_ITEM

static BOOL MatchedStruct (int nObjectType, LPCTSTR lpItem, void* lpStruct)
{
	ASSERT (lpStruct);
	if (!lpStruct)
		return FALSE;
	switch (nObjectType)
	{
	case OT_MON_SERVER:
		{
			LPSERVERDATAMIN lpsvrdata = (LPSERVERDATAMIN)lpStruct;
			if (_tcsicmp (lpItem, lpsvrdata->listen_address) == 0)
				return TRUE;
		}
		break;

	case OT_MON_SESSION:
		{
			LPSESSIONDATAMIN lpsessiondata = (LPSESSIONDATAMIN)lpStruct;
			//DWORD dwObjVal = 0;
			//_stscanf(lpItem, _T("%lx"), &dwObjVal);
			//DWORD dwSessVal = 0;
			//_stscanf((LPCTSTR)lpsessiondata->session_id, _T("%lu"), &dwSessVal);
			//ASSERT (dwObjVal);
			//ASSERT (dwSessVal);
			//if (dwObjVal == dwSessVal)
			//	return TRUE;
			TCHAR tchszCh[32];
			FormatHexa64 (lpsessiondata->session_id, tchszCh);
			if (_tcsicmp(lpItem, tchszCh) == 0)
				return TRUE;
		}
		break;
	case OT_DATABASE:
		{
			LPRESOURCEDATAMIN lpData = (LPRESOURCEDATAMIN)lpStruct;
			if (_tcsicmp (lpItem, lpData->res_database_name) == 0)
				return TRUE;
			return FALSE;
		}
		break;

	case OT_MON_REPLIC_SERVER:
		{
			LPREPLICSERVERDATAMIN lpSvr = (LPREPLICSERVERDATAMIN)lpStruct;
			int nInputSvrNo = 0;
			_stscanf(lpItem, _T("%d"), &nInputSvrNo);
			if (lpSvr->serverno == nInputSvrNo)
				return TRUE;
		}
		break;

	case OT_MON_LOGINFO:
		return TRUE;
		break;

	default:
		//
		// Not implemented
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}



CaTreeItemPath::CaTreeItemPath(BOOL bStatic, int type, void* pFastStruct, BOOL bCheckName, LPCTSTR lpsCheckName)
	:CuIpmTreeFastItem(bStatic, type, pFastStruct, bCheckName, lpsCheckName)
{
	m_bDeleteStruct = TRUE;
}

CaTreeItemPath::~CaTreeItemPath()
{
	if (m_bDeleteStruct && m_pStruct)
		delete m_pStruct;
}


BOOL IPM_BuildItemPath (CdIpmDoc* pDoc, CTypedPtrList<CObList, CuIpmTreeFastItem*>& listItemPath)
{
	ASSERT(!pDoc->GetSelectKey().IsEmpty());
	if (pDoc->GetSelectKey().IsEmpty())
		return FALSE;

	BOOL bOK = FALSE;
	int  nLevel = 0;
	int  nParent0type = 0;
	int  nObjectType  = 0;
	CString strParent0 = _T("");
	CString strStaticParent = _T("");
	CStringArray& arrayItemPath = pDoc->GetItemPath();
	void* pStructParent = NULL;
	int narrayItemPathSize = arrayItemPath.GetSize();
	ASSERT(narrayItemPathSize >= 1);
	if (narrayItemPathSize < 1)
		return FALSE;
	CString strObject = arrayItemPath.GetAt(0);
	//
	// Default (from old class CuIpmObjDesc in vdba):
	BOOL bObjectNeedsNameCheck = FALSE;
	BOOL bObjectHasStaticBranch= TRUE;
	BOOL bStaticParentNeedsNameCheck= FALSE;

	if (pDoc->GetSelectKey().CompareNoCase (_T("SERVER")) == 0)
	{
		//
		// Old class CuIpmObjDesc_SERVER in vdba (no overwrite the 3 default booleans above.
		nObjectType  = OT_MON_SERVER;
	}
	else
	if (pDoc->GetSelectKey().CompareNoCase (_T("SESSION")) == 0)
	{
		//
		// Old class CuIpmObjDesc_SESSION in vdba (no overwrite the 3 default booleans above.
		nLevel = 1; // Parent is the SERVER. EX: ii\ingres\158
		nParent0type = OT_MON_SERVER;
		nObjectType  = OT_MON_SESSION;
		ASSERT(narrayItemPathSize >= 2);
		if (narrayItemPathSize < 2)
			return FALSE;
		strParent0   = arrayItemPath.GetAt(0);
		strObject    = arrayItemPath.GetAt(1);
	}
	else
	if (pDoc->GetSelectKey().CompareNoCase (_T("REPLICSERVER")) == 0)
	{
		//
		// Old class CuIpmObjDesc_REPLICSERVER in vdba (overwrite the 2 default booleans above.
		bStaticParentNeedsNameCheck = TRUE;
		bObjectHasStaticBranch = FALSE;
		nLevel = 1; // Parent is the database name
		nParent0type = OT_DATABASE;
		nObjectType  = OT_MON_REPLIC_SERVER;
		ASSERT(narrayItemPathSize >= 1);
		if (narrayItemPathSize < 1)
			return FALSE;
		strStaticParent = _T("Replication");
		strParent0   = arrayItemPath.GetAt(0);
		if (narrayItemPathSize > 1)
			strObject    = arrayItemPath.GetAt(1);
	}
	else
	if (pDoc->GetSelectKey().CompareNoCase (_T("LOGINFO")) == 0)
	{
		//
		// Old class CuIpmObjDesc_LOGINFO in vdba (overwrite the 1 default boolean above.
		bObjectNeedsNameCheck = TRUE;
		strObject.LoadString(IDS_TM_LOGINFO); // "Log information"
	}

	if (nLevel > 0)
	{
		ASSERT (nParent0type);
		ASSERT (!strParent0.IsEmpty());
		// Static parent
		if (bStaticParentNeedsNameCheck) 
		{
			//
			// Only for replication:
			listItemPath.AddTail(new CaTreeItemPath(TRUE, nParent0type, NULL, TRUE, strStaticParent));
		}
		else
		{
			listItemPath.AddTail(new CaTreeItemPath(TRUE, nParent0type));
		}

		int reqSize = IPM_StructSize(nParent0type);
		CPtrList listInfoStruct;
		IPMUPDATEPARAMS ups;
		memset (&ups, 0, sizeof(ups));
		CaIpmQueryInfo queryInfo(pDoc, nParent0type, &ups, (LPVOID)NULL, reqSize);
		queryInfo.SetReportError(FALSE);

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			POSITION p = NULL, pos = listInfoStruct.GetHeadPosition();

			while (pos != NULL)
			{
				p = pos;
				LPVOID pObject = listInfoStruct.GetNext(pos);
				if (MatchedStruct (nParent0type, strParent0, pObject))
				{
					pStructParent = pObject;
					listInfoStruct.RemoveAt (p);
					listItemPath.AddTail(new CaTreeItemPath(FALSE, nParent0type, pObject)); // non-static
					break;
				}
			}
			while (!listInfoStruct.IsEmpty())
				delete listInfoStruct.RemoveHead();
		}

		if (!pStructParent)
			return FALSE;
	}

	//
	// Object itself:
	if (strObject.IsEmpty())
		return bOK;

	if (bObjectNeedsNameCheck)
	{
		listItemPath.AddTail(new CaTreeItemPath(TRUE,  nObjectType, NULL, TRUE, strObject));  // Root item
		bOK = TRUE;
	}
	else
	{
		if (bObjectHasStaticBranch)
			listItemPath.AddTail(new CaTreeItemPath(TRUE, nObjectType));  // static

		int reqSize = IPM_StructSize(nObjectType);
		CPtrList listInfoStruct;
		IPMUPDATEPARAMS ups;
		memset (&ups, 0, sizeof(ups));
		ups.nType   = nParent0type;
		ups.pStruct = pStructParent;
		ups.pSFilter= NULL;

		CaIpmQueryInfo queryInfo(pDoc, nObjectType, &ups, (LPVOID)NULL, reqSize);
		queryInfo.SetReportError(FALSE);

		BOOL bHasObjects = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bHasObjects)
		{
#if !defined (_IGNORE_NOT_MATCHED_ITEM)
			bOK = FALSE;
#endif
			POSITION p = NULL, pos = listInfoStruct.GetHeadPosition();
			while (pos != NULL)
			{
				p = pos;
				LPVOID pObject = listInfoStruct.GetNext(pos);
				if (MatchedStruct (nObjectType, strObject, pObject))
				{
					bOK = TRUE;
					listInfoStruct.RemoveAt (p);
					listItemPath.AddTail(new CaTreeItemPath(FALSE, nObjectType, pObject)); // non-static
					break;
				}
			}
			while (!listInfoStruct.IsEmpty())
				delete listInfoStruct.RemoveHead();
		}
	}

	return bOK;
}

int GetDefaultSelectTab (CdIpmDoc* pDoc)
{
	int nSel = 0;
	CStringArray& arrayItemPath = pDoc->GetItemPath();
	if (pDoc->GetSelectKey().CompareNoCase (_T("LOGINFO")) == 0)
	{
		CString strObject = arrayItemPath.GetAt(0);
		int nLen = strObject.GetLength();
		BOOL bNum = TRUE;
		for (int i=0; i<nLen; i++)
		{
			if (!_istdigit(strObject.GetAt(i)))
			{
				bNum = FALSE;
				break;
			}
		}
		if (bNum)
		{
			nSel = _ttoi (strObject);
		}
		else
		{
			UINT nIds[6] = {IDS_TAB_SUMMARY, IDS_TAB_HEADER, IDS_TAB_LOGFILE, IDS_TAB_PROCESS, IDS_TAB_ACTIVEDB, IDS_TAB_TRANSACTION};
			CString strTab;
			strObject.MakeLower();
			for (int k=0; k<6; k++)
			{
				strTab.LoadString (nIds[k]);
				strTab = strTab.Left(3);
				strTab.MakeLower();
				if (strObject.Find (strTab) == 0)
				{
					nSel = k;
					break;
				}
			}
		}
	}

	return nSel;
}

