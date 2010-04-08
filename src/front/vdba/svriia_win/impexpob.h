/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : impexpob.h , interface for the CoImportExport class
**    Project  : COM Server
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Com Object of Import & Export Assistant
**
** History:
**
** 15-Oct-2000 (uk$so01)
**    Created
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**/



#if !defined(IMPORTxEXPORT_OBJECT_HEADER)
#define IMPORTxEXPORT_OBJECT_HEADER
class CaComServer;
#include "bthread.h"
#include "impexpas.h"
#include "libguids.h"


class CoImportExport : public IUnknown, public CaComThreaded
{
public:
	//
	// Main Object Constructor & Destructor.
	CoImportExport(IUnknown* pUnkOuter, CaComServer* pServer);
	~CoImportExport(void);

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
	class CoImpImportAssistant : public IImportAssistant, public CaComThreaded
	{
	public:
		//
		// Interface Implementation Constructor & Destructor.
		CoImpImportAssistant();
		CoImpImportAssistant(CoImportExport* pBackObj, IUnknown* pUnkOuter);
		~CoImpImportAssistant(void);
		void Init(CoImportExport* pBackObj, IUnknown* pUnkOuter)
		{
			m_cRefI = 0;           // Init the Interface Ref Count (used for debugging only).
			m_pBackObj = pBackObj; // Init the Back Object Pointer to point to the parent object.
			if (NULL == pUnkOuter)
			{
				m_pUnkOuter = pBackObj;
		//		INGCHSVR_TraceLog1("L<%X>: CoAad::CoImpIAad Constructor. Non-Aggregating.",TID);
			}
			else
			{
				m_pUnkOuter = pUnkOuter;
		//		INGCHSVR_TraceLog1("L<%X>: CoAad::CoImpIAad Constructor. Aggregating.",TID);
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
		STDMETHODIMP ImportAssistant (HWND hwndParent);
		STDMETHODIMP ImportAssistant2(HWND hwndParent, IIASTRUCT* pStruct);
		STDMETHODIMP AboutBox (HWND hwndParent);

	private:
		void UpdateTitle();
		//
		// Data private to this CoImportExport interface implementation of IQueryObject.
		ULONG        m_cRefI;        // Interface Ref Count (debugging use).
		CoImportExport*      m_pBackObj;     // Parent Object back pointer.
		IUnknown*    m_pUnkOuter;    // Outer unknown for Delegation.
	};


	//
	// We declare nested class interface implementations here.
	// IAddAlterDrop Interface:
	class CoImpExportAssistant : public IExportAssistant, public CaComThreaded
	{
	public:
		//
		// Interface Implementation Constructor & Destructor.
		CoImpExportAssistant();
		CoImpExportAssistant(CoImportExport* pBackObj, IUnknown* pUnkOuter);
		~CoImpExportAssistant(void);
		
		void Init(CoImportExport* pBackObj, IUnknown* pUnkOuter)
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
		// IAad methods.
		//
		// Parameters:
		//
		// [in ] pQueryInfo: Data of CaLLAddAlterDrop
		// [out] pStream   : if not NULL, it packs a CString indicates an error text.
		// COMMENT: In case of error, if pStream is not NULL it might contain
		//          error text. Caller must call Release of the return IStrem interface
		STDMETHODIMP ExportAssistant (HWND hwndParent);
		STDMETHODIMP ExportAssistant2(HWND hwndParent, IEASTRUCT* pStruct);
		STDMETHODIMP AboutBox (HWND hwndParent);

	private:
		void UpdateTitle();
		//
		// Data private to this CoAad interface implementation of IAddAlterDrop.
		ULONG        m_cRefI;        // Interface Ref Count (debugging use).
		CoImportExport*   m_pBackObj;     // Parent Object back pointer.
		IUnknown*    m_pUnkOuter;    // Outer unknown for Delegation.

	};



	// Make the otherwise private and nested interface implementation
	// a friend to COM object instantiations of this selfsame CoImportExport
	// COM object class.
	friend CoImpImportAssistant;
	friend CoImpExportAssistant;

	// Private data of CoImportExport COM objects.
	// Nested Interface implementation instantiation.  These interfaces
	// are instantiated inside this CoImportExport object as a native interface.
	CoImpImportAssistant m_ImpImportAssistant;
	CoImpExportAssistant m_ImpExportAssistant;

	ULONG        m_cRefs;      // Main Object reference count.
	IUnknown*    m_pUnkOuter;  // Outer unknown (aggregation & delegation).
	CaComServer* m_pServer;    // Pointer to this component server's control object.

};


#endif // IMPORTxEXPORT_OBJECT_HEADER
