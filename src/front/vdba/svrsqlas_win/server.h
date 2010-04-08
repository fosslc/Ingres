/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : server.h: interface for the CaComServer class
**    Project  : Meta Data / COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Server, for internal use as Server of Component.
**
** History:
**
** 15-Oct-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**/


#if !defined (SERVER_HEADER)
#define SERVER_HEADER
#include "bthread.h"


//
// Appartment Thread Initialization data.
enum { NUM_APARTMENTS = 1 };
enum { APTINGRESOBJECT = 0 };

struct APT_INIT_DATA
{
	REFCLSID rclsid;
	IUnknown* pcf;

	// Member initializer MUST be used here because VC++ 4.0+ is strict
	// about const and reference (&) types like REFCLSID that need to
	// be initialized in this app.  For example, VC++ 4.x will not permit
	// a simple assignment of rclsid in the constructor.
	APT_INIT_DATA(REFCLSID rclsidi) : rclsid(rclsidi)
	{
		pcf = NULL;
	};

	~APT_INIT_DATA() {};
};



class CaComServer : public CaComThreaded
{
public:
	CaComServer();
	~CaComServer();

	void Lock();
	void Unlock();
	void ObjectsUp();
	void ObjectsDown();

	//
	// CThreaded method overrides.
	BOOL OwnThis();
	void UnOwnThis();

	
	HINSTANCE m_hInstServer; // A place to store the server's instance handle.
	HWND      m_hWndServer;  // A place to store the server's main window.
	LONG      m_cObjects;    // Global Server living Object count.
	LONG      m_cLocks;      // Global Server Client Lock count.

};


#endif // SERVER_HEADER
