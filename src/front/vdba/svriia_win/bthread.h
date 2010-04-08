/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : bthread.h: interface for the CaComThreaded class
**    Project  : Meta Data / COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Provide multi-threaded access to objects of the derived class
**               This is a utility base class providing useful methods for 
**               achieving mutually exclusive access to data in objects of 
**               the derived class.
**
** History:
**
** 15-Oct-1999 (uk$so01)
**    Created
**/

#if ! defined (BASETHREAD_HEADER)
#define BASETHREAD_HEADER

class CaComThreaded
{
protected:
	// Variables for providing mutually exclusive access by multiple
	// threads to objects of classes derived from this class.
	HANDLE m_hOwnerMutex;
	BOOL   m_bOwned;

public:
	CaComThreaded(void): m_hOwnerMutex(CreateMutex(0,FALSE,0)), m_bOwned(FALSE){}
	~CaComThreaded(void){CloseHandle(m_hOwnerMutex);}

	// The following virtual functions have overriding definitions in
	// the derived class to provide convenient trace logging. Rely on
	// the below default definitions for non-tutorial applications.

	virtual BOOL OwnThis(void)
	{
		BOOL bOwned = FALSE;
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hOwnerMutex, INFINITE))
			m_bOwned = bOwned = TRUE;
		return bOwned;
	}

	virtual void UnOwnThis(void)
	{
		if (m_bOwned)
		{
			m_bOwned = FALSE;
			ReleaseMutex(m_hOwnerMutex);
		}
		return;
	}

	//
	// Class helper that call UnOwnThis() automatically on destructor
	class CmtOwnMutex
	{
	public:
		//
		// Constructor:
		CmtOwnMutex(CaComThreaded* pBackParent, BOOL bInitialLocked = TRUE)
		    :m_pBackParent(pBackParent)
		{
			m_bLocked = FALSE;
			if (bInitialLocked)
				Lock();
		}
		//
		// Destructor:
		virtual ~CmtOwnMutex(){UnLock();}
		BOOL IsLocked(){return m_bLocked;}
		BOOL Lock()
		{
			if (m_pBackParent && !m_bLocked)
				m_bLocked = m_pBackParent->OwnThis();
			return m_bLocked;
		}
		void UnLock()
		{
			if (m_pBackParent && m_bLocked)
				m_pBackParent->UnOwnThis();
			m_bLocked = FALSE;
		}
	private:
		CaComThreaded* m_pBackParent;
		BOOL m_bLocked;

	};

};


#endif // BASETHREAD_HEADER
