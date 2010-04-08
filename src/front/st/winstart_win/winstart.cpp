/*
**  Copyright (c) 1995, 2002 Ingres Corporation
/* 

/*
**  Name: winstart.cpp
**
**  Description:
**	Contains definition of CWinstartApp functions / procedures.
**
**  History:
**	08-oct-98 (cucjo01)
**	    Added command line option /start and /stop to make application
**	    be a "start-only" or "stop-only" program.
**	09-oct-98 (cucjo01)
**	    Added command line option /client to override and start Ingres
**	    Client
**	30-mar-99 (cucjo01)
**	    Added command line option /param="xxxx" where xxxx can be
**	    -client or -iigcn, etc.
**	    Removed /client option which can now be replaced by
**	    "/param="-client"
**	    This is the command line parameter passed to ingstart & ingstart.
**	20-nov-00 (penga03)
**	    Created a semaphore for winstart. The semaphore is named as
**	    IngresII_Winstart_(installation idenitifier).
**	29-nov-00 (penga03)
**	    If there already exists a winstart with the same installation
**	    identifier running, bring the window to foreground and restore
**	    it to its original size and position.
**	22-feb-2001 (somsa01)
**	    Modified code to properly close hSemaphore at the appropriate
**	    time.
**	26-feb-2002 (somsa01)
**	    Once again corrected closing of hSemaphore.
**	31-oct-2002 (penga03)
**	    Disable the F1 Help.
**	02-May-2007 (drivi01)
**	    Change Service Manager title to not contain Ingres Version
**	    string.  Pre-installer has problems finding the window
**	    on the desktop when name changes.
**	25-Jul-2007 (drivi01)
**	    Update Control functions with recommended functions for
**	    new version of controls.
*/

#include "stdafx.h"
#include "winstart.h"
#include "winstdlg.h"
#include <compat.h>
#include <er.h>
#include <nm.h>
#include <st.h>
#include <gl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/*
** CWinstartApp
*/

BEGIN_MESSAGE_MAP(CWinstartApp, CWinApp)
	//{{AFX_MSG_MAP(CWinstartApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
END_MESSAGE_MAP()

/*
** CWinstartApp construction
*/

CWinstartApp::CWinstartApp()
{
}

/*
** The one and only CWinstartApp object
*/

CWinstartApp	theApp;
HANDLE		hSemaphore = NULL;
LONG		cMax = 1;
CString		csSemaphore;

/*
** CWinstartApp initialization
*/

BOOL CWinstartApp::InitInstance()
{
    char	*ii_installation;
    HWND	hWnd;
    CString	csWindow;

    NMgtAt(ERx( "II_INSTALLATION" ), &ii_installation);
    if (ii_installation == NULL || *ii_installation == EOS)
	ii_installation = ERx("II");
    else
	ii_installation = STalloc(ii_installation);
    csSemaphore.Format("IngresII_Winstart_%s", ii_installation);
    csWindow.Format(IDS_CAPTIONTEXT,(LPCTSTR)ii_installation);

    hSemaphore = OpenSemaphore( SEMAPHORE_ALL_ACCESS, FALSE,
				(LPCTSTR)csSemaphore );
    if (hSemaphore)
    {
	/*
	** We've already got a WINSTART running for this installation.
	** Attempt to find it and change focus to it.
	*/
	CloseHandle(hSemaphore);
	hWnd = FindWindow(NULL, csWindow);
	if(hWnd)
	{
	    SetForegroundWindow(hWnd);
	    ShowWindow(hWnd, SW_RESTORE);
	    exit(0);
	}
    }

    hSemaphore = CreateSemaphore(NULL, cMax, cMax, (LPCTSTR)csSemaphore);

    /*
    ** Standard initialization
    ** If you are not using these features and wish to reduce the size
    ** of your final executable, you should remove from the following
    ** the specific initialization routines you do not need.
    */
    InitCommonControls();
    CWinApp::InitInstance();

    AfxEnableControlContainer ();

    /* Load standard INI file options (including MRU) */
    LoadStdProfileSettings();

    CWinstartDlg dlg;
    m_pMainWnd = &dlg;

    CString strCmdLine="";
    if (m_lpCmdLine)
      strCmdLine = m_lpCmdLine;

    strCmdLine.MakeLower();
    
    if (strCmdLine.Find("/start") != -1)
       dlg.m_bStartFlag = TRUE;
    else
       dlg.m_bStartFlag = FALSE;

    if (strCmdLine.Find("/stop") != -1)
       dlg.m_bStopFlag = TRUE;
    else
       dlg.m_bStopFlag = FALSE;

    int i, iPos, iLen, iLeft, iChars=0;
    TCHAR p;
    dlg.m_strParam="";

    iPos = strCmdLine.Find("/param=\"");
    if (iPos != -1)
    { 
	dlg.m_bParamFlag = TRUE;
	iLen = strCmdLine.GetLength();
	iLeft = iPos + 8;
	for (i=iLeft; i<iLen; i++)
	{ 
	    p = strCmdLine.GetAt(i);
	    if (p == '\"')
		break;
	    iChars++;
	}

	dlg.m_strParam=strCmdLine.Mid(iLeft, iChars);
    }
    else
       dlg.m_bParamFlag = FALSE;

    dlg.DoModal();
    /*
    ** Since the dialog has been closed, return FALSE so that we exit the
    ** application, rather than start the application's message pump.
    */
    return FALSE;
}
