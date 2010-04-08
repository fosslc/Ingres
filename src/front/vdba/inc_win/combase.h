/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : combase.h: interface for the CmtCache class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The main entry of the Muti-thread cache
**
** History:
**
** 21-Nov-2000 (uk$so01)
**    Created
** 05-Jul-2001 (uk$so01)
**    BUG #100250 Build under mainwin, Fixed CmtAccess::Access() must return value.
**/

#if !defined (COMBASE_HEADER)
#define COMBASE_HEADER
#include "qryinfo.h"
class CmtSessionManager;

//
// CmtItemData
// ***********
class CmtItemData: public CObject
{
public:
	enum {ITEM_NEW = 0, ITEM_EXIST, ITEM_DELETE};

	CmtItemData():
	    m_tTimeStamp(-1),
	    m_nAccess(0),
	    m_nState(ITEM_EXIST),
	    m_hEventAccess(CreateEvent(NULL, TRUE, TRUE, NULL))
	{
		InitializeCriticalSection(&m_criticalSectionAccess);
		InitializeCriticalSection(&m_criticalSectionTimeStamp);
	}
	~CmtItemData(){CloseHandle(m_hEventAccess);}
	virtual CmtSessionManager* GetSessionManager(){ASSERT(FALSE); return NULL;}

	//
	// Access is allowed only if m_hEventAccess is in a signaled state:
	// Increase the 'm_nAccess' by one.
	long Access();

	//
	// Decrease the 'm_nAccess' by one.
	// If 'm_nAccess' becomes 0 then set the state of 'm_hEventAccess' to signaled.
	long UnAccess();

	BOOL IsAccessed();
	//
	// Block all access to this object. When own the 'm_hEventAccess'
	// it set the state to non-signaled to block the threads that want to access.
	// Return when all threads have finished accessing to this object.
	// Before the function return TRUE, it sets the state of event 'm_hEventAccess' to non-signaled.
	// You must call the function WaitUntilNoAccessDone() to set 'm_hEventAccess' to signaled state.
	BOOL WaitUntilNoAccess();
	void WaitUntilNoAccessDone(){SetEvent (m_hEventAccess);}


	int GetState() {return m_nState;}
	void  SetState(int nState){m_nState = nState;}

	CTime GetLastQuery();
	void  SetLastQuery();
	void  ResetLastQuery();

protected:
	//
	// The time (the last queried object, local time)
	CTime m_tTimeStamp;
	//
	// Number of Threads currently access this object.
	long  m_nAccess;
	//
	// Event object that synchronize the access to this object.
	HANDLE m_hEventAccess;
	//
	// Critical section used to protect the 'm_nAccess' member:
	CRITICAL_SECTION m_criticalSectionAccess;
	//
	// Critical section used to protect the 'm_tTimeStamp' member:
	CRITICAL_SECTION m_criticalSectionTimeStamp;

	//
	// State of Item: (ITEM_NEW, ITEM_EXIST, ITEM_DELETE)
	// No need to be protected.
	int m_nState;

	class CmtAccess
	{
	public:
		CmtAccess(CmtItemData* pBackParent, BOOL bInitialAccessed = TRUE): m_pBackParent(pBackParent)
		{
			if (m_pBackParent && bInitialAccessed)
			{
				m_pBackParent->Access();
			}
		}
		long Access()
		{
			long lAccess = -1;
			if (m_pBackParent)
			{
				lAccess = m_pBackParent->Access();
			}
			return lAccess;
		}
		long UnAccess()
		{
			long lAccess = -1;
			if (m_pBackParent)
			{
				lAccess = m_pBackParent->UnAccess();
			}
			return lAccess;
		}

		~CmtAccess(){UnAccess();}
	private:
		CmtItemData* m_pBackParent;
	};

	class CmtWaitUntilNoAccess
	{
	public:
		CmtWaitUntilNoAccess(CmtItemData* pBackParent): m_pBackParent(pBackParent)
		{
			if (m_pBackParent)
				m_pBackParent->WaitUntilNoAccess();
		}
		~CmtWaitUntilNoAccess()
		{
			if (m_pBackParent)
				m_pBackParent->WaitUntilNoAccessDone();
		}
	private:
		CmtItemData* m_pBackParent;
	};

};


//
// Access is allowed only if m_hEventAccess is in a signaled state:
// Increase the 'm_nAccess' by one.
inline long CmtItemData::Access()
{
	UINT nAccess = 0;
	DWORD dwWait = WaitForSingleObject (m_hEventAccess, INFINITE);
	switch (dwWait)
	{
	case WAIT_OBJECT_0:
		break;
	default:
		return -1;
	}

	EnterCriticalSection(&m_criticalSectionAccess);
	m_nAccess++;
	nAccess = m_nAccess;
	LeaveCriticalSection(&m_criticalSectionAccess);

	return m_nAccess;
}

//
// Decrease the 'm_nAccess' by one.
// If 'm_nAccess' becomes 0 then set the state of 'm_hEventAccess' to signaled.
inline long CmtItemData::UnAccess()
{
	UINT nAccess = 0;

	EnterCriticalSection(&m_criticalSectionAccess);
	if (m_nAccess > 0)
		m_nAccess--;
	nAccess = m_nAccess;
	if (nAccess == 0)
	{
		//
		// Set the state of event 'm_hEventAccess' to signaled.
		SetEvent (m_hEventAccess);
	}
	LeaveCriticalSection(&m_criticalSectionAccess);

	return m_nAccess;
}

inline BOOL CmtItemData::IsAccessed()
{
	BOOL bAccessed = FALSE;
	EnterCriticalSection(&m_criticalSectionAccess);
	bAccessed = (m_nAccess > 0);
	LeaveCriticalSection(&m_criticalSectionAccess);
	return bAccessed;
}


//
// Block all access to this object. When own the 'm_hEventAccess'
// it set the state to non-signaled to block the threads that want to access.
// Return when all threads have finished accessing to this object.
inline BOOL CmtItemData::WaitUntilNoAccess()
{
	UINT nAccess = 0;

	EnterCriticalSection(&m_criticalSectionAccess);
	if (m_nAccess > 0)
	{
		// 
		// Set the state of event 'm_hEventAccess' to non-signaled
		// to block the access to this object:
		ResetEvent (m_hEventAccess);
		LeaveCriticalSection(&m_criticalSectionAccess);
	
		//
		// Wait until the already threads finished their accesses:
		DWORD dwWait = WaitForSingleObject (m_hEventAccess, INFINITE);
		switch (dwWait)
		{
		case WAIT_OBJECT_0:
			//
			// Set the event state to non-signaled before returning:
			ResetEvent (m_hEventAccess);
			return TRUE;
		default:
			return FALSE;
		}
	}
	LeaveCriticalSection(&m_criticalSectionAccess);

	return TRUE;
}


inline CTime CmtItemData::GetLastQuery()
{
	EnterCriticalSection(&m_criticalSectionTimeStamp);
	CTime t = m_tTimeStamp;
	LeaveCriticalSection(&m_criticalSectionTimeStamp);
	return t;
}

inline void  CmtItemData::SetLastQuery()
{
	EnterCriticalSection(&m_criticalSectionTimeStamp);
	m_tTimeStamp = CTime::GetCurrentTime();
	LeaveCriticalSection(&m_criticalSectionTimeStamp);
}

inline void  CmtItemData::ResetLastQuery()
{
	EnterCriticalSection(&m_criticalSectionTimeStamp);
	m_tTimeStamp = -1;
	LeaveCriticalSection(&m_criticalSectionTimeStamp);
}


//
// CmtProtect
// **********
class CmtProtect
{
public:
	CmtProtect():m_hMutex(CreateMutex(0, FALSE, 0)), m_bOwned(FALSE){}
	~CmtProtect(){}

	virtual BOOL OwnThis()
	{
		BOOL bOwned = FALSE;
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, INFINITE))
			m_bOwned = bOwned = TRUE;
		return bOwned;
	}

	virtual void UnOwnThis()
	{
		if (m_bOwned)
		{
			m_bOwned = FALSE;
			ReleaseMutex(m_hMutex);
		}
		return;
	}


protected:
	class CmtMutexReadWrite
	{
	public:
		//
		// Constructor:
		CmtMutexReadWrite(CmtProtect* pBackParent, BOOL bInitialLocked = TRUE)
		    :m_pBackParent(pBackParent), m_bLocked (bInitialLocked)
		{
			if (m_pBackParent && bInitialLocked)
				m_bLocked = m_pBackParent->OwnThis();
		}
		//
		// Destructor:
		virtual ~CmtMutexReadWrite()
		{
			if (m_pBackParent) 
				m_pBackParent->UnOwnThis();
			m_bLocked = FALSE;
		}
		BOOL IsLocked(){return m_bLocked;}
		BOOL Lock()
		{
			if (m_pBackParent)
				m_bLocked = m_pBackParent->OwnThis();
			return m_bLocked;
		}
		void UnLock()
		{
			if (m_pBackParent)
				m_pBackParent->UnOwnThis();
			m_bLocked = FALSE;
		}
	private:
		CmtProtect* m_pBackParent;
		BOOL m_bLocked;

	};


private:
	HANDLE m_hMutex;
	BOOL   m_bOwned;
};





#endif // COMBASE_HEADER