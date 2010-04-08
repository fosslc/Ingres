/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tksplash.h : main header file for the TKSPLASH DLL
**    Project  : DLL (About Box). 
**    Author   : Sotheavut UK (uk$so01)
**
**
** History:
**
** 06-Nov-2001 (uk$so01)
**    created
*/



#if !defined(AFX_TKSPLASH_H__71950976_D2C6_11D5_8788_00C04F1F754A__INCLUDED_)
#define AFX_TKSPLASH_H__71950976_D2C6_11D5_8788_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"

class CTksplashApp : public CWinApp
{
public:
	CTksplashApp();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTksplashApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CTksplashApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TKSPLASH_H__71950976_D2C6_11D5_8788_00C04F1F754A__INCLUDED_)
