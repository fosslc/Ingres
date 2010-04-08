/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : TkAnimat.cpp : Defines the initialization routines for the DLL.
**    Project  : Extension DLL (Task Animation).
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog to show the progression of Task (Interruptible)
**
**  History:
**  07-Feb-1999 (uk$so01)
**     created
**  16-Mar-2001 (uk$so01)
**     Changed to be an Extension DLL
**  31-Oct-2001 (hanje04)
**     Removed obselete extern from declaration of TaskAnimateInitialize.
**  04-Feb-2002 (noifr01)
**     restored a trailing } sign that probably disappeared upon a "propagation"
**     operation
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, cleanup.
**  20-Aug-2008 (whiro01)
**    Moved <afx...> include to "stdafx.h"
*/


#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static AFX_EXTENSION_MODULE TkAnimatDLL = { NULL, NULL };
CaExtensionState::CaExtensionState()
{
	m_hInstOld = AfxGetResourceHandle();
	AfxSetResourceHandle(TkAnimatDLL.hResource);
}

CaExtensionState::~CaExtensionState()
{
	AfxSetResourceHandle(m_hInstOld);
}

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("TKANIMAT.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(TkAnimatDLL, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(TkAnimatDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("TKANIMAT.DLL Terminating!\n");
		// Terminate the library before destructors are called
		AfxTermExtensionModule(TkAnimatDLL);
	}
	return 1;   // ok
}


extern "C" BOOL WINAPI TaskAnimateInitialize()
{
	if (AfxGetModuleState() != AfxGetAppModuleState())
	{
		//
		// construct new dynlink library in this context for core resources
		try
		{
			CDynLinkLibrary* pDLL = new CDynLinkLibrary(TkAnimatDLL, TRUE);
			ASSERT(pDLL != NULL);
			if (pDLL)
			{
				pDLL->m_factoryList.m_pHead = NULL;
				TRACE0("TKANIMAT.DLL Initializing (explitly calls to TaskAnimateInitialize()...)!\n");
			}
			else
				return FALSE;
		}
		catch(...)
		{
			return FALSE;
		}
	}
	return TRUE;
}
