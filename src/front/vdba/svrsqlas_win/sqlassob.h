/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlassob.h , interface for the CoSqlAssistant class
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
**
**/



#if !defined(SQL_ASSISTANT_OBJECT_HEADER)
#define SQL_ASSISTANT_OBJECT_HEADER
class CaComServer;
#include "bthread.h"
#include "sqlasinf.h"
#include "libguids.h"


class CoSqlAssistant : public IUnknown, public CaComThreaded
{
public:
	//
	// Main Object Constructor & Destructor.
	CoSqlAssistant(IUnknown* pUnkOuter, CaComServer* pServer);
	~CoSqlAssistant(void);

	// A general method for initializing this newly created object.
	// Creates any subordinate arrays, structures, or objects.
	HRESULT Init(void);

	//
	// IUnknown methods. Main object, non-delegating.
	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

private:
	//
	// We declare nested class interface implementations here.
	// IQueryObject Interface:
	class CoImpSqlAssistant : public ISqlAssistant, public CaComThreaded
	{
	public:
		//
		// Interface Implementation Constructor & Destructor.
		CoImpSqlAssistant(){};
		CoImpSqlAssistant(CoSqlAssistant* pBackObj, IUnknown* pUnkOuter);
		~CoImpSqlAssistant(void);
		void Init(CoSqlAssistant* pBackObj, IUnknown* pUnkOuter)
		{
			m_cRefI = 0;           // Init the Interface Ref Count (used for debugging only).
			m_pBackObj = pBackObj; // Init the Back Object Pointer to point to the parent object.
			if (NULL == pUnkOuter)
			{
				m_pUnkOuter = pBackObj;
			}
			else
			{
				m_pUnkOuter = pUnkOuter;
			}
		}

		//
		// IUnknown methods.
		STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		//
		// IQueryObject methods.

		//
		// Parameters:
		//
		// [in ] pQueryInfo: Data of CaLLQueryInfo
		// [out] pStream   : Data of CaDBObject derived class, and depends on
		//                   the member 'm_nTypeObject' in pQueryInfo
		// COMMENT: In case of error, if pStream is not NULL it might contain
		//          error text.
		STDMETHODIMP SqlAssistant (HWND hwndParent, SQLASSISTANTSTRUCT* pStruct, BSTR* bstrStatement);
		STDMETHODIMP AboutBox (HWND hwndParent);

	private:
		//
		// Data private to this CoSqlAssistant interface implementation of IQueryObject.
		ULONG        m_cRefI;                // Interface Ref Count (debugging use).
		CoSqlAssistant*      m_pBackObj;     // Parent Object back pointer.
		IUnknown*    m_pUnkOuter;            // Outer unknown for Delegation.
	};



	// Make the otherwise private and nested interface implementation
	// a friend to COM object instantiations of this selfsame CoSqlAssistant
	// COM object class.
	friend CoImpSqlAssistant;

	// Private data of CoSqlAssistant COM objects.
	// Nested Interface implementation instantiation.  These interfaces
	// are instantiated inside this CoSqlAssistant object as a native interface.
	CoImpSqlAssistant m_ImpSqlAssistant;

	ULONG        m_cRefs;      // Main Object reference count.
	IUnknown*    m_pUnkOuter;  // Outer unknown (aggregation & delegation).
	CaComServer* m_pServer;    // Pointer to this component server's control object.

};


#endif // SQL_ASSISTANT_OBJECT_HEADER
