/*
**  Copyright (c) 2003 Ingres Corporation
*/

/*
**  Name: IngClean.h : main header file for the INGCLEAN application
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/

#if !defined(AFX_INGCLEAN_H__26FFA293_7703_4F9C_B8FF_A459BC98808A__INCLUDED_)
#define AFX_INGCLEAN_H__26FFA293_7703_4F9C_B8FF_A459BC98808A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "Uninst.h"
#include <msi.h>
#include <msiquery.h>
#include <shlwapi.h>

#define GUID_TO_REG(_guid, _reg) \
{ \
    _reg[0]     = _guid[8]; \
    _reg[1]     = _guid[7]; \
    _reg[2]     = _guid[6]; \
    _reg[3]     = _guid[5]; \
    _reg[4]     = _guid[4]; \
    _reg[5]     = _guid[3]; \
    _reg[6]     = _guid[2]; \
    _reg[7]     = _guid[1]; \
    _reg[8]     = _guid[13]; \
    _reg[9]     = _guid[12]; \
    _reg[10]    = _guid[11]; \
    _reg[11]    = _guid[10]; \
    _reg[12]    = _guid[18]; \
    _reg[13]    = _guid[17]; \
    _reg[14]    = _guid[16]; \
    _reg[15]    = _guid[15]; \
    _reg[16]    = _guid[21]; \
    _reg[17]    = _guid[20]; \
    _reg[18]    = _guid[23]; \
    _reg[19]    = _guid[22]; \
    _reg[20]    = _guid[26]; \
    _reg[21]    = _guid[25]; \
    _reg[22]    = _guid[28]; \
    _reg[23]    = _guid[27]; \
    _reg[24]    = _guid[30]; \
    _reg[25]    = _guid[29]; \
    _reg[26]    = _guid[32]; \
    _reg[27]    = _guid[31]; \
    _reg[28]    = _guid[34]; \
    _reg[29]    = _guid[33]; \
    _reg[30]    = _guid[36]; \
    _reg[31]    = _guid[35]; \
    _reg[32]    = '\0'; \
}

extern CUninst theUninst;

extern BOOL Execute(char *cmd, BOOL bWait=1);
extern BOOL IngresAlreadyRunning(char *ii_system);
extern BOOL WinstartRunning(char *ii_system);
extern BOOL AskUserYN(UINT uiStrID, LPCTSTR param=0);
extern void Error(UINT uiStrID, LPCTSTR param=0);
extern void Message(UINT uiStrID, LPCTSTR param=0);
extern BOOL Local_NMgtIngAt(CString ii_system, CString csKey, CString &csValue);
extern BOOL Local_PMget(CString ii_system, CString csKey, CString &csValue);
extern BOOL Local_RemoveDirectory(char *DirName);

/////////////////////////////////////////////////////////////////////////////
// CIngCleanApp:
// See IngClean.cpp for the implementation of this class
//

class CIngCleanApp : public CWinApp
{
public:
	CIngCleanApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIngCleanApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CIngCleanApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INGCLEAN_H__26FFA293_7703_4F9C_B8FF_A459BC98808A__INCLUDED_)
