/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: wincluster.h : main header file for the WinCluster application
**
**  History:
**
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
*/

#if !defined(AFX_WINCLUSTER_H__1B5DC485_E871_4127_9A90_6F5428F69B03__INCLUDED_)
#define AFX_WINCLUSTER_H__1B5DC485_E871_4127_9A90_6F5428F69B03__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include <windows.h>
#include "256bmp.h"
#include "InstallCode.h"
#include "Welcome.h"
#include "PropSheet.h"
#include "PreSetup.h"

#define SPLASH_DURATION   (3000)	// duration of first splash screen
#define MAXIMUM_NAMELENGTH   1024		// maximum name length for cluster groups, resources, etc.

/////////////////////////////////////////////////////////////////////////////
// WcWinClusterApp:
// See wincluster.cpp for the implementation of this class
//

class WcWinClusterApp : public CWinApp
{
public:
	WcWinClusterApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(WcWinClusterApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(WcWinClusterApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern HPALETTE hSystemPalette;
extern HICON theIcon;
extern WcPropSheet *property;
extern WcPreSetup thePreSetup;
extern char ii_installpath[MAX_PATH+1];

extern void MyError(UINT uiStrID,LPCSTR param=0);
extern void SysError(UINT stringId, LPCSTR insertChr, DWORD lastError);
extern BOOL AskUserYN(UINT uiStrID,LPCSTR param=0);
extern BOOL Execute(LPCSTR lpCmdLine, BOOL bWait /*=TRUE*/);
extern BOOL Execute(LPCSTR lpCmdLine, LPCSTR par /* =0 */, BOOL bWait /* =TRUE */);
extern BOOL Local_NMgtIngAt(CString strKey, CString ii_system, CString &strRetValue);
extern BOOL Local_PMget(CString strKey, CString ii_system, CString &strRetValue);
extern int IngresAlreadyRunning();
extern BOOL WinstartRunning();
extern BOOL ExecuteEx(LPCSTR lpCmdLine, BOOL bWait=TRUE, BOOL bWindow=FALSE);
extern BOOL SetupHighAvailability();
extern "C" bool ScanResources(LPSTR rexp, LPCSTR from, LPCSTR to);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WINCLUSTER_H__1B5DC485_E871_4127_9A90_6F5428F69B03__INCLUDED_)
