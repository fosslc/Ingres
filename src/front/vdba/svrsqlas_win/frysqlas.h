/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : frysqlas.h: interface for the CaFactorySqlAssistant class
**    Project  : COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : COM Class factory for SQL assistant Objects.
**
** History:
**
** 10-Jul-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**/


#if !defined(FACTORY_IMPORTxEXPORT_HEADER)
#define FACTORY_IMPORTxEXPORT_HEADER
#include "bthread.h"



class CaFactorySqlAssistant : public IUnknown, public CaComThreaded
{
public:
	//
	// Main Object Constructor & Destructor.
	CaFactorySqlAssistant(IUnknown* pUnkOuter, CaComServer* pServer);
	~CaFactorySqlAssistant(void);

	//
	// IUnknown methods. Main object, non-delegating.
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

private:
	// We declare nested class interface implementations here.

	// We implement the IClassFactory interface (ofcourse) in this class
	// factory COM object class.
	class CaImpIClassFactory : public IClassFactory
	{
	public:
		//
		// Interface Implementation Constructor & Destructor.
		CaImpIClassFactory(){};
		CaImpIClassFactory(CaFactorySqlAssistant* pBackObj, IUnknown* pUnkOuter, CaComServer* pServer);
		~CaImpIClassFactory(void);
		void Init(CaFactorySqlAssistant* pBackObj, IUnknown* pUnkOuter, CaComServer* pServer)
		{
			m_cRefI = 0;
			m_pBackObj = pBackObj;
			m_pServer = pServer;

			if (NULL == pUnkOuter)
			{
				m_pUnkOuter = pBackObj;
			}
			else
			{
				m_pUnkOuter = pUnkOuter;
			}
		}

		// IUnknown methods.
		STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		// IClassFactory methods.
		STDMETHODIMP         CreateInstance(IUnknown*, REFIID, LPVOID*);
		STDMETHODIMP         LockServer(BOOL);

	private:
		// Data private to this interface implementation of IClassFactory.
		ULONG          m_cRefI;       // Interface Ref Count (for debugging).
		CaFactorySqlAssistant* m_pBackObj;    // Parent Object back pointer.
		IUnknown*      m_pUnkOuter;   // Outer unknown for Delegation.
		CaComServer*   m_pServer;     // Server's control object.
	};

	// Make the otherwise private and nested IClassFactory interface
	// implementation a friend to COM object instantiations of this
	// selfsame CaFactorySqlAssistant COM object class.
	friend CaImpIClassFactory;

	// Private data of CaFactorySqlAssistant COM objects.
	// Nested IClassFactory implementation instantiation.
	CaImpIClassFactory m_ImpIClassFactory;

	// Main Object reference count.
	ULONG             m_cRefs;

	// Outer unknown (aggregation & delegation). Used when this
	// CaFactorySqlAssistant object is being aggregated.  Otherwise it is used
	// for delegation if this object is reused via containment.
	IUnknown*         m_pUnkOuter;

	// Pointer to this component server's control object.
	CaComServer*      m_pServer;
};



#endif // FACTORY_IMPORTxEXPORT_HEADER
