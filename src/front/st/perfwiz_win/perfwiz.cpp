/*
**
**  Name: perfwiz.cpp
**
**  Description:
**	This file defines the class behaviors for the application.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	18-oct-1999 (somsa01)
**	    Added NumCountersSelected, a global variable which keeps track of
**	    counters selected for charting.
**	29-oct-1999 (somsa01)
**	    Added the global variable PersonalCounterID for keeping track
**	    of the counter id form which the personal stuff starts.
**	06-May-2008 (drivi01)
**	    Update the way visual controls are loaded. Enable3dControls has been
**	    been depricated.
**	30-May-2008 (whiro01) Bug 119642, 115719; SD issue 122646
**	    Put common declaration of our main registry key value (PERFCTRS_KEY) in here.
**	03-Nov-2008 (drivi01)
**	    Replace Windows help with HTML help.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "PropSheet.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HPALETTE		hSystemPalette=0;
HWND			MainWnd;
CString			ChosenVnode, ChosenServer, ChartName="";
CString			ObjectName, ObjectDescription;
BOOL			PersonalGroup, InitFileRead=FALSE, UseExisting=FALSE;
int			CounterID, PersonalCounterID;
DWORD			NumCountersSelected=0;
struct grouplist	*GroupList=NULL;

GroupClass		GroupNames[Num_Groups];
Group			CacheGroup[NUM_CACHE_GROUPS*Num_Cache_Counters];
Group			LockGroup[NUM_LOCK_GROUPS*Num_Lock_Counters];
Group			ThreadGroup[NUM_THREAD_GROUPS*Num_Thread_Counters];
Group			SamplerGroup[NUM_SAMPLER_GROUPS*Num_Sampler_Counters];
PERFCTR			PersonalCtr_Init[NUM_PERSONAL_COUNTERS];

// Registry key values
const char * const PERFCTRS_KEY = "SYSTEM\\CurrentControlSet\\Services\\oiPfCtrs\\Performance";

/*
** CPerfwizApp
*/
BEGIN_MESSAGE_MAP(CPerfwizApp, CWinApp)
    //{{AFX_MSG_MAP(CPerfwizApp)
	    // NOTE - the ClassWizard will add and remove mapping macros here.
	    //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CPerfwizApp::OnHelp)
END_MESSAGE_MAP()

/*
** CPerfwizApp construction
*/
CPerfwizApp::CPerfwizApp()
{
}

/*
** The one and only CPerfwizApp object
*/
CPerfwizApp theApp;

/*
** CPerfwizApp initialization
*/
BOOL CPerfwizApp::InitInstance()
{
    INT_PTR	nResponse;

    AfxEnableControlContainer();

    hSystemPalette=0;
/*  // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);
*/
    InitCommonControls();
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    PropSheet psDlg("Ingres Performance Monitor Wizard");
    m_pMainWnd = &psDlg;
    nResponse = psDlg.DoModal();
    if (nResponse == IDOK)
    {
    }
    else if (nResponse == IDCANCEL)
    {
    }

    return FALSE;
}

VOID CPerfwizApp::OnHelp()
{
	::HtmlHelp(GetDesktopWindow(), "./perfwiz.chm", HH_DISPLAY_TOPIC, NULL);
}

