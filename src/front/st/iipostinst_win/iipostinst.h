/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**
**  Name: iipostinst.h
**
**  Description:
**      This is the main header file for the IIPOSTINST EXE.
**
**  History:
**	10-apr-2001 (somsa01)
**	    Created.
**	07-may-2001 (somsa01)
**	    Changed ODBC to act like a separate product. Also,
**	    added prototype for liccheck().
**	06-jun-2001 (somsa01)
**	    Added SIZE_ESQLFORTRAN.
**	06-nov-2001 (penga03)
**	    Added functions CheckMicrosoft, CheckNetscape, CheckSpyGlass and 
**	    GetColdFusionPath and doGetRegistryEntry. 
**	16-jul-2002 (penga03)
**	    Removed CheckSpyGlass(), added CheckApache().
**	23-jun-2005 (penga03)
**	    Added ExitInstance() and return_code.
**  15-Nov-2006 (drivi01)
**      SIR 116599
**      Enhanced post-installer in effort to improve installer usability.
**	    Included header files for new classes.
*/

#if !defined(AFX_IIPOSTINST_H__9990F267_AE41_4801_AA60_087567F869C3__INCLUDED_)
#define AFX_IIPOSTINST_H__9990F267_AE41_4801_AA60_087567F869C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		/* main symbols */
#include <ddeml.h>
#include <winreg.h>
#include <winsvc.h>
#include "256bmp.h"
#include "install.h"
#include "psheet.h"
#include "postinst.h"
#include "Summary.h"
#include "InfoDialog.h"

#define SIZE_DBMS		    (56.7)
#define SIZE_NET		    (19.1)
#define SIZE_STAR		    (0.187)
#define SIZE_ESQLC		    (0.541)
#define SIZE_ESQLCOB		    (0.512)
#define SIZE_ESQLFORTRAN	    (0.512)
#define SIZE_TOOLS		    (1.37)
#define SIZE_VISION		    (2.79)
#define SIZE_REPLICAT		    (1.18)
#define SIZE_ICE		    (1.99)
#define SIZE_OPENROAD		    (24.7)
#define SIZE_OPENROAD_RUN	    (22.7)
#define SIZE_OPENROAD_DEV	    (24.7)
#define SIZE_DOC		    (25.1)
#define SIZE_ORACLE_GATEWAY	    (1)
#define SIZE_SYBASE_GATEWAY	    (1)
#define SIZE_INFORMIX_GATEWAY	    (1)
#define SIZE_SQLSERVER_GATEWAY	    (1)
#define SIZE_OF_COMMON_COMPONENTS   (17.7)
#define SIZE_VDBA		    (9.2)
#define SIZE_EMBEDDED_COMPONENTS    (1)
#define SIZE_JDBC		    (1)
#define SIZE_ODBC		    (1)

extern int	return_code;
extern HPALETTE		hSystemPalette;
extern CInstallation	theInstall;
extern CPropSheet	*property;
extern HICON		theIcon;
extern LPCSTR		END_OF_LINE;

extern BOOL AskUserYN(UINT uiStrID, LPCSTR param=0);
extern void Error(UINT uiStrID,LPCSTR param=0);
extern BOOL FileExists(LPCSTR s);
extern BOOL IsWindows95();
extern void Message(UINT uiStrID, LPCSTR param=0);
extern int  liccheck (char *lic_code, BOOL bStartService);
extern BOOL CheckMicrosoft(CString &strValue);
extern BOOL CheckNetscape(CString &strValue);
extern BOOL CheckApache(CString &strValue);
extern BOOL GetColdFusionPath(CString &strValue);
extern int doGetRegistryEntry(HKEY hRootKey, char *szRegKey, char *szKeyName, CString &szRetValue);

/*
** CIipostinstApp
** See iipostinst.cpp for the implementation of this class
*/

class CIipostinstApp : public CWinApp
{
public:
    CIipostinstApp();

/* Overrides */
    /* ClassWizard generated virtual function overrides */
    //{{AFX_VIRTUAL(CIipostinstApp)
    public:
    virtual BOOL InitInstance();
	virtual int ExitInstance();
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CIipostinstApp)
	    // NOTE - the ClassWizard will add and remove member functions here.
	    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IIPOSTINST_H__9990F267_AE41_4801_AA60_087567F869C3__INCLUDED_)
