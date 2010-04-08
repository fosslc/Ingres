/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : frysqlas.cpp: Implementation of the CaFactorySqlAssistant class
**    Project  : COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : COM Class factory for Sql Assistant Objects.
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    Created, SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**/

#include "stdafx.h"
#include "server.h"
#include "frysqlas.h"
#include "sqlassob.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaFactorySqlAssistant::CaFactorySqlAssistant(IUnknown* pUnkOuter, CaComServer* pServer)
{
	m_ImpIClassFactory.Init(this, pUnkOuter, pServer);

	m_cRefs = 0;             // Zero the COM object's reference count.
	m_pUnkOuter = pUnkOuter; // No AddRef necessary if non-NULL, as we're nested.
	m_pServer = pServer;     // Init the pointer to the server control object.
}

CaFactorySqlAssistant::~CaFactorySqlAssistant(void)
{
}


STDMETHODIMP CaFactorySqlAssistant::QueryInterface(REFIID riid, LPVOID* ppv)
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
	else if (IID_IClassFactory == riid)
	{
		*ppv = &m_ImpIClassFactory;
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


STDMETHODIMP_(ULONG) CaFactorySqlAssistant::AddRef(void)
{
	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return m_cRefs;

	m_cRefs++;
	mBlock.UnLock();
	m_pServer->ObjectsUp();
	return m_cRefs;
}


STDMETHODIMP_(ULONG) CaFactorySqlAssistant::Release(void)
{
	ULONG cRefs;
	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return m_cRefs;

	cRefs = --m_cRefs;
	m_pServer->ObjectsDown();

	if (0 == cRefs)
	{
		// We artificially bump the main ref count to prevent reentrancy
		// via the main object destructor.  Not really needed in this
		// CaFactorySqlAssistant but a good practice because we are aggregatable and
		// may at some point in the future add something entertaining like
		// some Releases to the CaFactorySqlAssistant destructor.
		m_cRefs++;
		mBlock.UnLock();
		delete this;
		return cRefs;
	}

	mBlock.UnLock();
	return cRefs;
}


CaFactorySqlAssistant::CaImpIClassFactory::CaImpIClassFactory(CaFactorySqlAssistant* pBackObj, IUnknown* pUnkOuter, CaComServer* pServer)
{
	// Init the Interface Ref Count (used for debugging only).
	m_cRefI = 0;

	// Init the Back Object Pointer to point to the parent object.
	m_pBackObj = pBackObj;

	// Init the pointer to the server control object.
	m_pServer = pServer;

	// Init the CImpIClassFactory interface's delegating Unknown pointer.
	// We use the Back Object pointer for IUnknown delegation here if we are
	// not being aggregated.  If we are being aggregated we use the supplied
	// pUnkOuter for IUnknown delegation.  In either case the pointer
	// assignment requires no AddRef because the CImpIClassFactory lifetime is
	// quaranteed by the lifetime of the parent object in which
	// CImpIClassFactory is nested.
	if (NULL == pUnkOuter)
	{
		m_pUnkOuter = pBackObj;
	}
	else
	{
		m_pUnkOuter = pUnkOuter;
	}
}



CaFactorySqlAssistant::CaImpIClassFactory::~CaImpIClassFactory(void)
{
}



STDMETHODIMP CaFactorySqlAssistant::CaImpIClassFactory::QueryInterface(REFIID riid, LPVOID* ppv)
{
	//
	// Delegate this call to the outer object's QueryInterface.
	return m_pUnkOuter->QueryInterface(riid, ppv);
}



STDMETHODIMP_(ULONG) CaFactorySqlAssistant::CaImpIClassFactory::AddRef(void)
{
	//
	// Increment the Interface Reference Count.
	++m_cRefI;

	//
	// Delegate this call to the outer object's AddRef.
	return m_pUnkOuter->AddRef();
}



STDMETHODIMP_(ULONG) CaFactorySqlAssistant::CaImpIClassFactory::Release(void)
{
	//
	// Decrement the Interface Reference Count.
	--m_cRefI;

	//
	// Delegate this call to the outer object's Release.
	return m_pUnkOuter->Release();
}



STDMETHODIMP CaFactorySqlAssistant::CaImpIClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppv)
{
	HRESULT hError = E_FAIL;

	CoSqlAssistant* pCob = NULL;

	// NULL the output pointer.
	*ppv = NULL;

	// If the creation call is requesting aggregation (pUnkOuter != NULL),
	// the COM rules state the IUnknown interface MUST also be concomitantly
	// requested.  If it is not so requested (riid != IID_IUnknown) then
	// an error must be returned indicating that no aggregate creation of
	// the CaFactorySqlAssistant COM Object can be performed.
	if (NULL != pUnkOuter && riid != IID_IUnknown)
		hError = CLASS_E_NOAGGREGATION;
	else
	{
		//
		// Instantiate a CoQueryObject COM Object.
		pCob = new CoSqlAssistant(pUnkOuter, m_pServer);

		if (NULL != pCob)
		{
			// We initially created the new COM object so tell the server
			// to increment its global server object count to help ensure
			// that the server remains loaded until this partial creation
			// of a COM component is completed.
			m_pServer->ObjectsUp();

			// We QueryInterface this new COM Object not only to deposit the
			// main interface pointer to the caller's pointer variable, but to
			// also automatically bump the Reference Count on the new COM
			// Object after handing out this reference to it.
			hError = pCob->QueryInterface(riid, (LPVOID*)ppv);
			if (FAILED(hError))
			{
				delete pCob;
				m_pServer->ObjectsDown();
			}
		}
		else
		{
			// If we were launched to create this object and could not then
			// we should force a shutdown of this server.
			m_pServer->ObjectsUp();
			m_pServer->ObjectsDown();
			hError = E_OUTOFMEMORY;
		}
	}

	return hError;
}



STDMETHODIMP CaFactorySqlAssistant::CaImpIClassFactory::LockServer(BOOL fLock)
{
	HRESULT hr = NOERROR;
	if (fLock)
		m_pServer->Lock();
	else
		m_pServer->Unlock();
	return hr;
}

