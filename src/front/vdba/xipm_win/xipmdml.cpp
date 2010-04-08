/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xipmdml.cpp, implementation file 
**    Project  : IPM
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Stand alone Ingres Performance Monitor.
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "sessimgr.h"
#include "ipmdoc.h"
#include "ingobdml.h"
#include "xipmdml.h"
#include "usrexcep.h"
#include "constdef.h"
extern "C"
{
#include "libmon.h"
}


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL DML_QueryNode(CdIpm* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = (VNODE_QueryNode (listObj) == NOERROR)? TRUE: FALSE;
	return bOK;
}


BOOL DML_QueryServer(CdIpm* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = (VNODE_QueryServer (pNode, listObj) == NOERROR)? TRUE: FALSE;
	return bOK;
}

BOOL DML_QueryUser(CdIpm* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = FALSE;
	CaSessionManager sessionManager;
	CaLLQueryInfo queryInfo (OBT_USER, pDoc->GetNode());
	queryInfo.SetIndependent(TRUE);
	sessionManager.SetDescription(_T("Ingres Visual Performance Monitor"));

	CString strDefault;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();

	strDefault.LoadString(IDS_DEFAULT_SERVER);
	if (strDefault.CompareNoCase(strServer) == 0)
		strServer = _T("");
	strDefault.LoadString(IDS_DEFAULT_USER);
	if (strDefault.CompareNoCase(strUser) == 0)
		strUser = pDoc->GetDefaultConnectedUser();
	queryInfo.SetServerClass(strServer);
	queryInfo.SetUser(strUser);

	bOK = INGRESII_llQueryObject (&queryInfo, listObj, (void*)&sessionManager);
	return bOK;
}

BOOL DML_FillComboResourceType(CComboBox* pCombo)
{
	// insert in combobox scanning Type_Lock array from monitor.cpp.
	// the combobox will sort items
	LPSTYPELOCK pTypeLock;
	for (pTypeLock = Type_Lock; pTypeLock->iResource != -1; pTypeLock++)
	{
		int restype = pTypeLock->iResource;
		if (restype != RESTYPE_ALL && 
		    restype != RESTYPE_DATABASE &&
		    restype != RESTYPE_TABLE &&
		    restype != RESTYPE_PAGE) 
		{
			int index = pCombo->AddString ((LPCTSTR)pTypeLock->szStringLock);
			pCombo->SetItemData(index, restype);
		}
	}

	// force insert of "All Other resource types" at first position
	CString strType;
	strType.LoadString (IDS_ALLRESOURCETYPES);
	int index = pCombo->InsertString (0, (LPCTSTR)strType);
	pCombo->SetItemData(index, (DWORD)RESTYPE_ALL);
	pCombo->SetCurSel (0);

	return TRUE;
}