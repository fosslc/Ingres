/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcda.h : main header file for VCDA.OCX
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Initialize Instance
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, created
*/

#if !defined(AFX_VCDA_H__EAF345F0_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_VCDA_H__EAF345F0_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h" 


class CVcdaApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
extern CVcdaApp theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VCDA_H__EAF345F0_D514_11D6_87EA_00C04F1F754A__INCLUDED)
