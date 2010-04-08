/*
**
**  Name: dp.cpp
**
**  Description:
**	This file contains the necessary general routines for the application.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "PropSheet.h"
#include <lmcons.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* Global defines */
HPALETTE    hSystemPalette=0;
CString	    DestFile, StructName, OldItemValue, Version;
CString	    ConnectLine, Options1, Options2, Options3, Options4;
CString	    OutFile, ErrFile, Message, Message2;
DWORD	    FileIndex, TableIndex;
BOOL	    FileInput, FFFLAG, OFFLAG, PBFLAG, PEFLAG;
BOOL	    OutputLog, ErrorLog;
char	    UserID[UNLEN + 1];
HWND	    MainWnd;

/*
** CDpApp
*/
BEGIN_MESSAGE_MAP(CDpApp, CWinApp)
    //{{AFX_MSG_MAP(CDpApp)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/*
** CDpApp construction
*/
CDpApp::CDpApp()
{
}

/*
** The one and only CDpApp object
*/
CDpApp theApp;

/*
** CDpApp initialization
*/
BOOL
CDpApp::InitInstance()
{
    CString Message, Message2;

    /*
    ** Make sure II_SYSTEM is set.
    */
    if (!strcmp(getenv("II_SYSTEM"), ""))
    {
        Message.LoadString(IDS_NO_IISYSTEM);
        Message2.LoadString(IDS_TITLE);
        MessageBox(NULL, Message, Message2, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    /*
    ** Standard initialization
    ** If you are not using these features and wish to reduce the size
    ** of your final executable, you should remove from the following
    ** the specific initialization routines you do not need.
    */
#ifdef _AFXDLL
	Enable3dControls();	  /* Call this when using MFC in a shared DLL */
#else
	Enable3dControlsStatic(); /* Call this when linking to MFC statically */
#endif

    PropSheet psDlg("DP");

    CString DirLoc = CString(getenv("II_SYSTEM")) +
#ifdef MAINWIN
		     CString("/ingres/sig/dp");
#else
		     CString("\\ingres\\sig\\dp");
#endif
    SetCurrentDirectory(DirLoc);
    m_pMainWnd = &psDlg;
    int nResponse = psDlg.DoModal();
    if (nResponse == IDOK)
    {
    }
    else if (nResponse == IDCANCEL)
    {
    }

    /*
    ** Since the dialog has been closed, return FALSE so that we exit the
    ** application, rather than start the application's message pump.
    */
    return FALSE;
}
