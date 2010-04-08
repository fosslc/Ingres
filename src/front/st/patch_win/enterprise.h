/*
**  Copyright (c) 2001, 2006 Ingres Corporation.  All Rights Reserved.
*/

/*
**  Name: enterprise.h : main header file for the ENTERPRISE application
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	07-Jun-2001 (penga03)
**	    Execute has external linkage.
**	15-Jun-2001 (penga03)
**	    Added Local_NMgtIngAt.
**	23-July-2001 (penga03)
**	    Changed Execute so that it allows command that will be executed to have 
**	    parameters.
**	23-July-2001 (penga03)
**	    Added functions IsWindows9X, GetII_SYSTEM, Local_PMget, WinstartRunning, 
**	    IngresAlreadyRunning.
**	05-nov-2001 (penga03)
**	    Deleted GetII_SYSTEM.
**	19-feb-2004 (penga03)
**	    Added ExecuteEx() and InstallMSRedistributions().
**	17-jun-2004 (somsa01)
**	    Added ExecuteEx() prototype.
**	13-dec-2004 (penga03)
**	    Removed the abundant function ExecuteEx(char *).
**	08-feb-2005 (penga03)
**	    Added two more parameters for Error().
**	14-march-2005 (penga03)
**	    Added <clusapi.h>.
**	23-jun-2005 (penga03)
**	    Added ExitInstance() and return_code.
**	18-aug-2005 (penga03)
**	    Added hLogFile and AppendToLog().
**	01-Mar-2006 (drivi01)
**	    Reused for MSI Patch Installer on Windows.
*/

#if !defined(AFX_ENTERPRISE_H__1B5DC485_E871_4127_9A90_6F5428F69B03__INCLUDED_)
#define AFX_ENTERPRISE_H__1B5DC485_E871_4127_9A90_6F5428F69B03__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include <windows.h>
#include <clusapi.h>
#include "256bmp.h"
#include "InstallCode.h"
#include "Welcome.h"
#include "PropSheet.h"
#include "PreInstallation.h"
#include "WaitDlg.h"

#define SPLASH_DURATION   (3000)	// duration of first splash screen

/////////////////////////////////////////////////////////////////////////////
// CEnterpriseApp:
// See enterprise.cpp for the implementation of this class
//

class CEnterpriseApp : public CWinApp
{
public:
	CEnterpriseApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnterpriseApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CEnterpriseApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern HANDLE hLogFile;
extern int return_code;
extern HPALETTE hSystemPalette;
extern HICON theIcon;
extern CPropSheet *property;
extern CPreInstallation thePreInstall;
extern char ii_installpath[MAX_PATH+1];

extern void Error(UINT uiStrID,LPCSTR param=0,LPCSTR param2=0,LPCSTR param3=0);
extern BOOL AskUserYN(UINT uiStrID,LPCSTR param=0);
extern BOOL Execute(char *cmd, char *par=0, BOOL bWait=TRUE);
extern BOOL Local_NMgtIngAt(CString strKey, CString ii_system, CString &strRetValue);
extern BOOL IsWindows9X();
extern BOOL Local_PMget(CString strKey, CString ii_system, CString &strRetValue);
extern int IngresAlreadyRunning();
extern BOOL WinstartRunning();
extern BOOL ExecuteEx(LPCSTR lpCmdLine, BOOL bWait=TRUE, BOOL bWindow=FALSE);
extern BOOL InstallMSRedistributions();
extern void AppendToLog(LPCSTR p);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTERPRISE_H__1B5DC485_E871_4127_9A90_6F5428F69B03__INCLUDED_)
