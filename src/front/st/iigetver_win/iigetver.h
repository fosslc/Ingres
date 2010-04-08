/******************************************************************************
**
** Copyright (c) 1999 Ingres Corporation
**
******************************************************************************/

/******************************************************************************
**
** iigetver.h : header file
**
** History:
**	23-mar-99 (cucjo01)
**	    Created
**      Header file for version information
**
******************************************************************************/

#if !defined(AFX_IIGETVER_H__59D86425_E143_11D2_9FF1_006008924264__INCLUDED_)
#define AFX_IIGETVER_H__59D86425_E143_11D2_9FF1_006008924264__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "ii_ver.h"         // error codes, structure definitions, etc

/////////////////////////////////////////////////////////////////////////////
// CIIgetverApp
// See iigetver.cpp for the implementation of this class
//

class CIIgetverApp : public CWinApp
{
public:
    int II_GetVersion(II_VERSIONINFO *VersionInfo);
	CIIgetverApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIIgetverApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CIIgetverApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IIGETVER_H__59D86425_E143_11D2_9FF1_006008924264__INCLUDED_)
