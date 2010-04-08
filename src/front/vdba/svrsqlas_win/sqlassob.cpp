/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlassob.cpp , implementation of the CoSqlAssistant class
**    Project  : COM Server
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Com Object of SQL Assistant Wizard
**
** History:
**
** 10-Jul-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 21-Aug-2008 (whiro01)
**    Moved <afx...> include to "stdafx.h".
**/



#include "stdafx.h"
#include "rcdepend.h"
#include "server.h"
#include "sqlassob.h"
#include "sqlwpsht.h"
#include "sqlepsht.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CoSqlAssistant::CoSqlAssistant(IUnknown* pUnkOuter, CaComServer* pServer)
{
	m_ImpSqlAssistant.Init(this, pUnkOuter);
	m_cRefs = 0;
	m_pUnkOuter = pUnkOuter; // No AddRef necessary if non-NULL, as we're nested.
	m_pServer   = pServer;   // Assign the pointer to the server control object.
}



CoSqlAssistant::~CoSqlAssistant(void)
{
}


STDMETHODIMP CoSqlAssistant::QueryInterface(REFIID riid, LPVOID* ppv)
{
	HRESULT hError = E_NOINTERFACE;
	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return hError;

	*ppv = NULL;
	if (IID_IUnknown == riid)
	{
		*ppv = this;
	}
	else 
	if (IID_ISqlAssistant == riid)
	{
		*ppv = &m_ImpSqlAssistant;
	}

	if (NULL != *ppv)
	{
		// We've handed out a pointer to the interface so obey the COM rules
		// and AddRef the reference count.
		((LPUNKNOWN)*ppv)->AddRef();
		hError = NOERROR;
	}

	mBlock.UnLock();
	return hError;
}



STDMETHODIMP_(ULONG) CoSqlAssistant::AddRef(void)
{
	ULONG cRefs;
	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return m_cRefs;

	cRefs = ++m_cRefs;
	mBlock.UnLock();

	return cRefs;
}



STDMETHODIMP_(ULONG) CoSqlAssistant::Release(void)
{
	ULONG cRefs;
	CaComServer* pServer;

	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return m_cRefs;

	cRefs = --m_cRefs;
	if (0 == cRefs)
	{
		//
		// Save a local copy of the pointer to the server control object.
		pServer = m_pServer;
		//
		// We artificially bump the main ref count to prevent reentrancy
		// via the main object destructor.  Not really needed in this
		// CoSqlAssistant but a good practice because we are aggregatable and
		// may at some point in the future add something entertaining like
		// some Releases to the CoSqlAssistant destructor. We relinquish thread
		// ownership of this object before deleting it--a good practice.
		m_cRefs++;
		mBlock.UnLock();
		delete this;

		//
		// We've reached a zero reference count for this COM object.
		// So we tell the server housing to decrement its global object
		// count so that the server will be unloaded if appropriate.
		if (NULL != pServer)
			pServer->ObjectsDown();
	}

	return cRefs;
}





// ************************************************************************************************
//                           CoImpSqlAssistant 
// ************************************************************************************************

CoSqlAssistant::CoImpSqlAssistant::CoImpSqlAssistant(CoSqlAssistant* pBackObj, IUnknown* pUnkOuter)
{
	m_cRefI = 0;            // Init the Interface Ref Count (used for debugging only).
	m_pBackObj = pBackObj;  // Init the Back Object Pointer to point to the parent object.

	//
	// Init the CoImpIQueryObject interface's delegating Unknown pointer.  We use
	// the Back Object pointer for IUnknown delegation here if we are not
	// being aggregated.  If we are being aggregated we use the supplied
	// pUnkOuter for IUnknown delegation.  In either case the pointer
	// assignment requires no AddRef because the CoImpIQueryObject lifetime is
	// guaranteed by the lifetime of the parent object in which
	// CoImpIQueryObject is nested.
	if (NULL == pUnkOuter)
	{
		m_pUnkOuter = pBackObj;
	}
	else
	{
		m_pUnkOuter = pUnkOuter;
	}
}



CoSqlAssistant::CoImpSqlAssistant::~CoImpSqlAssistant(void)
{
}



STDMETHODIMP CoSqlAssistant::CoImpSqlAssistant::QueryInterface(REFIID riid, LPVOID* ppv)
{
	//
	// Delegate this call to the outer object's QueryInterface.
	return m_pUnkOuter->QueryInterface(riid, ppv);
}



STDMETHODIMP_(ULONG) CoSqlAssistant::CoImpSqlAssistant::AddRef(void)
{
	//
	// Increment the Interface Reference Count.
	CmtOwnMutex mBlock (this);
	++m_cRefI;

	//
	// Delegate this call to the outer object's AddRef.
	return m_pUnkOuter->AddRef();
}


STDMETHODIMP_(ULONG) CoSqlAssistant::CoImpSqlAssistant::Release(void)
{
	//
	// Decrement the Interface Reference Count.
	CmtOwnMutex mBlock (this);
	--m_cRefI;

	//
	// Delegate this call to the outer object's Release.
	return m_pUnkOuter->Release();
}







STDMETHODIMP CoSqlAssistant::CoImpSqlAssistant::SqlAssistant(HWND hwndParent, SQLASSISTANTSTRUCT* pStruct, BSTR* bstrStatement)
{
	USES_CONVERSION;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWnd* pWnd = hwndParent? CWnd::FromHandle(hwndParent): NULL;

	HRESULT hErr = NOERROR;
	BOOL bOk = FALSE;
	try
	{
		*bstrStatement = NULL;
		ASSERT(pStruct);
		if (!pStruct)
			return E_FAIL;
		CaSessionManager& ssmgr = theApp.GetSessionManager();
		ssmgr.SetSessionStart(pStruct->nSessionStart + 1);
		ssmgr.SetDescription(W2CT(pStruct->wczSessionDescription));

		switch (pStruct->nActivation)
		{
		case 1:   // Wizard for select statement
			{
				CxDlgPropertySheetSqlWizard dlg (TRUE, NULL);
				dlg.m_queryInfo.SetObjectType(OBT_VNODE);
				dlg.m_queryInfo.SetNode (W2CT(pStruct->wczNode));
				dlg.m_queryInfo.SetServerClass (W2CT(pStruct->wczServerClass));
				dlg.m_queryInfo.SetDatabase(W2CT(pStruct->wczDatabase));
				dlg.m_queryInfo.SetFlag(pStruct->nDbFlag);
				dlg.m_queryInfo.SetIncludeSystemObjects(FALSE);
				dlg.m_queryInfo.SetUser(W2CT(pStruct->wczUser));
				dlg.m_queryInfo.SetOptions(W2CT(pStruct->wczConnectionOption));
				if (pStruct->lpPoint)
					dlg.m_pPointTopLeft = pStruct->lpPoint;

				int nRes = dlg.DoModal();
				CString strStatement;
				dlg.GetStatement (strStatement);
				strStatement.TrimLeft();
				strStatement.TrimRight();
				if (nRes != IDCANCEL && !strStatement.IsEmpty())
				{
					TRACE1("STATEMENT=%s\n", (LPCTSTR)strStatement);
					*bstrStatement = strStatement.AllocSysString();
				}
			}
			break;
		case 2:   // Wizard for search condition
			{
				CxDlgPropertySheetSqlExpressionWizard dlg;
				dlg.m_queryInfo.SetObjectType(OBT_VNODE);
				dlg.m_queryInfo.SetNode (W2CT(pStruct->wczNode));
				dlg.m_queryInfo.SetServerClass (W2CT(pStruct->wczServerClass));
				dlg.m_queryInfo.SetDatabase(W2CT(pStruct->wczDatabase));
				dlg.m_queryInfo.SetFlag(pStruct->nDbFlag);
				dlg.m_queryInfo.SetIncludeSystemObjects(FALSE);
				dlg.m_queryInfo.SetUser(W2CT(pStruct->wczUser));
				dlg.m_queryInfo.SetOptions(W2CT(pStruct->wczConnectionOption));

				dlg.m_nFamilyID    = FF_PREDICATES;
				dlg.m_nAggType     = IDC_RADIO1;
				dlg.m_nCompare     = IDC_RADIO1;
				dlg.m_nIn_notIn    = IDC_RADIO1;
				dlg.m_nSub_List    = IDC_RADIO1;
				if (pStruct->lpPoint)
					dlg.m_pPointTopLeft = pStruct->lpPoint;
				//
				// Initialize Tables or Views:
				TOBJECTLIST* lpList = pStruct->pListTables;
				CString strItemOwner;
				while (lpList)
				{
					TOBJECTLIST* pTxV = lpList;
					lpList = lpList->pNext;
					if (pTxV)
					{
						CaDBObject* pObject = new CaDBObject(W2T(pTxV->wczObject), W2T(pTxV->wczObjectOwner));
						pObject->SetObjectID(pTxV->nObjType);
						dlg.m_listObject.AddTail (pObject);
					}
				}
				
				lpList = pStruct->pListColumns;
				while (lpList)
				{
					TOBJECTLIST* pCol = lpList;
					lpList = lpList->pNext;
					if (pCol)
					{
						dlg.m_listStrColumn.AddTail (W2T(pCol->wczObject));
					}
				}

				int nRes = dlg.DoModal();
				CString strStatement;
				dlg.GetStatement (strStatement);
				strStatement.TrimLeft();
				strStatement.TrimRight();
				if (nRes != IDCANCEL && !strStatement.IsEmpty())
				{
					TRACE1("STATEMENT=%s\n", (LPCTSTR)strStatement);
					*bstrStatement = strStatement.AllocSysString();
				}
			}
			break;
		default:  // Wizard general
			{
				CxDlgPropertySheetSqlWizard dlg;
				dlg.m_queryInfo.SetObjectType(OBT_TABLE);
				dlg.m_queryInfo.SetNode (W2CT(pStruct->wczNode));
				dlg.m_queryInfo.SetServerClass (W2CT(pStruct->wczServerClass));
				dlg.m_queryInfo.SetDatabase(W2CT(pStruct->wczDatabase));
				dlg.m_queryInfo.SetFlag(pStruct->nDbFlag);
				dlg.m_queryInfo.SetIncludeSystemObjects(FALSE);
				dlg.m_queryInfo.SetUser(W2CT(pStruct->wczUser));
				dlg.m_queryInfo.SetOptions(W2CT(pStruct->wczConnectionOption));
				if (pStruct->lpPoint)
					dlg.m_pPointTopLeft = pStruct->lpPoint;

				int nRes = dlg.DoModal();
				CString strStatement;
				dlg.GetStatement (strStatement);
				strStatement.TrimLeft();
				strStatement.TrimRight();
				if (nRes != IDCANCEL && !strStatement.IsEmpty())
				{
					TRACE1("STATEMENT=%s\n", (LPCTSTR)strStatement);
					*bstrStatement = strStatement.AllocSysString();
				}
			}
			break;
		}
		return hErr;
	}
	catch (...)
	{
		hErr = E_FAIL;
	}
	*bstrStatement = NULL;
	return hErr;
}


STDMETHODIMP CoSqlAssistant::CoImpSqlAssistant::AboutBox(HWND hwndParent)
{
	HRESULT hErr = NOERROR;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	AfxMessageBox(_T("Not available yet"));
	return hErr;
}



