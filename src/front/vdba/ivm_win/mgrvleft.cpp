/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : mgrvleft.cpp , Implementation File
**
**  Project  : Ingres II / Visual Manager
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Tree View that contains the components
**
**  History:
**  15-Dec-1998 (uk$so01)
**      Created.
**  31-Jan-2000 (noifr01)
**      (SIR 100237) (reenabled the logic where IVM manages the fact
**      that Ingres may be started as a service or not).
**  01-Feb-2000 (noifr01)
**      (SIR 100238) when the user askes to Start ingres as a service,
**      test whether he has the administrators rights
**  02-Feb-2000 (noifr01)
**      (bug 100315) rmcmd instances are now differentiated. take care
**      of the fact that there is one level less of branches in the
**      tree than usually and use the same technique as for the name
**      server
**  08-feb-2000 (somsa01)
**      In ThreadProcControlS3(), redirect output to NULL. Also, cleaned
**      it up to properly set events. Also, in OnUpdate(), fixed setting
**      of "-kill" option for "ingstop".
** 20-Apr-2000 (uk$so01)
**    SIR #101252
**    When user start the VCBF, and if IVM is already running
**    then request IVM the start VCBF
** 25-Apr-2000 (uk$so01)
**    SIR #101312
**    When user start the ingres as service, use the command: 'ingstart -service'
**    instead of running the service directly.
** 03-May-2000 (uk$so01)
**    SIR #101402
**    Start ingres (or ingres component) is in progress ...:
**    If start the Name Server of the whole installation, then just sleep an elapse
**    of time before querying the instance. (15 secs or less if process terminated ealier).
**    Change the accelerate phase of refresh instances from 200ms to 1600ms.
** 12-Mai-2000 (uk$so01)
**    SIR #101312
**    When user stop the ingres installation, always ask if user wants to issue the
**    -kill option.
** 06-Jul-2000 (uk$so01)
**    BUG #102020
**    No need to cleanup the pointer pListComponent, it is now replaced by m_pListComponent
**    a member of CappIvmApplication. The CappIvmApplication will take care to clean it.
** 25-Sep-2000 (uk$so01)
**    BUG #102726
**    CvManagerViewLeft::ThreadProcControlS1, and CvManagerViewLeft::ThreadProcControlS2, 
**    add the current directory to the function CreateProcess(...)
** 25-Sep-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 18-Jan-2001 (uk$so01)
**    SIR #103727 (Make IVM support JDBC Server)
**    Handle the bitmap for JDBC Server.
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one IVM per installation ID
** 29-Jun-2000 (uk$so01)
**    SIR #105160, created for adding custom message box. 
** 14-Aug-2001 (uk$so01)
**    SIR #105383,
**    When changing the selection in a tree, resulting in the right panes to be different, 
**    the selected right pane for the new selected tree item should be the same (that with the same name) 
**    as that of the previous selection in the tree, if there is one with the same name.
** 17-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client".
** 23-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client". (missed fix)
** 27-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client". (No Start Client if Win9x)
** 29-Oct-2001 (noifr01)
**    (bug 106193) test the value of pItem->GetPageInformation(), which may be NULL when
**    there is no right pane
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 29-May-2002 (noifr01)
**   (sir 107879) use isDBMS() instead of VCBFllGetFirstComp/VCBFllGetNextComp
**   loop for avoiding potential synchronization issues in these functions
**   (now normally called only from the same thread).
**   Also don't wait for vcbf to terminate and refresh only then. The refresh
**   process is now independent
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 11-Mar-2003 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu
** 13-Mar-2003 (uk$so01)
**    SIR #109775, Additional update to change #462359
** 07-May-2003 (uk$so01)
**    BUG #110195, issue #11179264. Ivm show the alerted messagebox to indicate that
**    there are alterted messages but the are not displayed in the right pane
**    according to the preferences
** 26-Jun-2003 (uk$so01)
**    BUG #110477, issue #12789432. Force to refresh the status pane each time
**    the thread updates the components.
** 24-Mar-2004 (uk$so01)
**    BUG #112013 / ISSUE 13278495
**    On some platforms and WinXPs, CrateProcess failed if the path
**    contains characters '[' ']'.
**    This change will remove the full path from the command line of the
**    CreateProcess()or ShellExecute() because now the path for the GUI
**    tools has been set by the installer.
** 30-Mar-2004 (uk$so01)
**    BUG #112013 / ISSUE 13278495 (additional fix)
** 27-Apr-2004 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu.
**    (additional changes: add new menu items and change captions)
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 11-Aug-2004 (uk$so01)
**    SIR #109775, Additional change:
**    Add a menu item "Ingres Command Prompt" in Ivm's taskbar icon menu.
** 14-Sep-2004 (uk$so01)
**    SIR #109775, Additional change:
**    Ingres Document PDF file names have changed, update the references
**    to the new file names.
** 14-Sep-2004 (uk$so01)
**    SIR #109775, Additional change:
**    Put the working dir of Command Prompt to %HOMEDRIVE%%HOMEPATH% ie 
**    Use CSIDL_PROFILE  with SHGetFolderPath()
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
** 31-Jan-2005 (drivi01)
**    (bug 113825)
**    Updated shortcuts to new pdf files, as well as removed shortcut for
**    Ingres Readme File. 
** 04-March-2005 (penga03)
**    Added back the shortcut for Ingres Readme File. 
** 25-Jul-2007 (drivi01)
**    For Vista, update code to use winstart to start/stop servers
**    whenever start/stop menu is activated.  IVM will run as a user
**    application and will elevate privileges to start/stop by
**    calling ShellExecuteEx function, since ShellExecuteEx can not
**    subscribe to standard input/output, we must use winstart to
**    stop/start ingres.
** 07-Aug-2007 (drivi01)
**    Added icons to popup menu to indicate elevation.  Icons 
**    will display only in popup menus off of menu bar.
** 22-Oct-2008 (drivi01)
**    Added rouines to the case statement that brings up a readme in a 
**    popup menu.  If ivm is running from within DBA Tools package, 
**    the readme will be readme_dba.html.
** 02-Dec-2008 (whiro01)
**    Changed the default button to "No" (second button) on the IDS_MSG_ASK2STOP_SERVICE
**    question, since "No" is the recommended option.
** 19-Mar-2009 (drivi01)
**    Remove usage of bSuccess before it is initialized in 
**    CvManagerViewLeft::ThreadProcControlS4 
*/

#include "stdafx.h"
#include "constdef.h"
#include "ivm.h"
#include "dlgmain.h"
#include "mgrvleft.h"
#include "mgrfrm.h"
#include "compdata.h"
#include "ivmdml.h"
#include "wmusrmsg.h"
#include "regsvice.h"
#include "xdlgmsg.h"
#include "ll.h"
#include "tlsfunct.h"
#include <compat.h>
#include <gvcl.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CvManagerViewLeft, CTreeView)

static UINT StartIngresApplication(LPVOID pParam);
static BOOL QueryPathValues (HKEY hKey, LPCTSTR lpszSubKey, CString& strPath);
static CString GetIngresDocumentation(int nHint);

class CaStartApplication
{
public:
	CaStartApplication(LPCTSTR lpsCmdName, /*LPCTSTR lpsCmdPath,*/ BOOL bShowWindow = FALSE)
	{
		m_strName = lpsCmdName;
		m_bShowWindow = bShowWindow;
		m_hWndCaller = NULL;
	}
	~CaStartApplication(){}
	void SetWindowCaller(HWND hWnd){m_hWndCaller = hWnd;}

	CString GetName(){return m_strName;}
	BOOL GetShowWindow(){return m_bShowWindow;}
	HWND GetWindowCaller(){return m_hWndCaller;}
protected:
	CString m_strName;
	BOOL m_bShowWindow;
	HWND m_hWndCaller;
};


class CaStartNAMExINSTALLATION
{
public:
	CaStartNAMExINSTALLATION(BOOL bStart = TRUE)
	{
		m_hEvent = NULL;
		if (bStart)
			m_hEvent = CreateEvent (NULL, TRUE, FALSE, theApp.m_strStartNAMExINSTALLATION);
		else
			m_hEvent = CreateEvent (NULL, TRUE, TRUE, theApp.m_strStartNAMExINSTALLATION);
	}

	~CaStartNAMExINSTALLATION()
	{
		if (m_hEvent)
		{
			SetEvent (m_hEvent);
			CloseHandle (m_hEvent);
		}
	}
	
	void SetStart (BOOL bStart)
	{
		if (!m_hEvent && bStart)
			m_hEvent = CreateEvent (NULL, TRUE, FALSE, theApp.m_strStartNAMExINSTALLATION);
		if (!bStart && m_hEvent)
		{
			SetEvent (m_hEvent);
		}
	}

protected:
	HANDLE m_hEvent;
};



CvManagerViewLeft::CvManagerViewLeft()
{
	m_bAlreadyCreateImageList = FALSE;
	m_pThread1 = NULL;
	m_pThread2 = NULL;
	m_pThread3 = NULL;
	m_bPopupOpen = FALSE;
	m_pItem = NULL;
	m_nPreviousPageID = 0;
	theApp.m_bVista = GVvista();
}

CvManagerViewLeft::~CvManagerViewLeft()
{
}


BEGIN_MESSAGE_MAP(CvManagerViewLeft, CTreeView)
	//{{AFX_MSG_MAP(CvManagerViewLeft)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_START, OnStart)
	ON_COMMAND(ID_START_CLIENT, OnStartClient)
	ON_COMMAND(ID_STOP, OnStop)
	ON_COMMAND(ID_CONFIGURE, OnConfigure)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_GETITEMDATA,   OnGetItemData)
	ON_MESSAGE (WMUSRMSG_UPDATE_COMPONENT,  OnUpdateComponent)
	ON_MESSAGE (WMUSRMSG_UPDATE_COMPONENT_ICON,  OnUpdateComponentIcon)
	ON_MESSAGE (WMUSRMSG_UPDATE_ALERT,  OnUpdateAlert)
	ON_MESSAGE (WMUSRMSG_UPDATE_UNALERT,  OnUpdateUnalert)
	ON_MESSAGE (WMUSRMSG_REACTIVATE_SELECTION, OnReactivateSelection)
	ON_MESSAGE (WMUSRMSG_APP_SHUTDOWN, OnShutDownIngres)
	ON_MESSAGE (WMUSRMSG_GET_STATE, OnGetState)
	ON_MESSAGE (WMUSRMSG_ERROR, OnErrorOccurred)
	ON_MESSAGE (WMUSRMSG_RUNEXTERNALPROGRAM, OnRunProgram)
END_MESSAGE_MAP()

static DWORD IFThreadControlRunApp(LPVOID pParam)
{
	return StartIngresApplication(pParam);
}


static DWORD IFThreadControlS1(LPVOID pParam)
{
	CvManagerViewLeft* pView = (CvManagerViewLeft*)pParam;
	return pView->ThreadProcControlS1((LPVOID)pView->GetComponentItem());
}

class CaStartStopParam
{
public:
	CaStartStopParam():bStart(TRUE), pParam(NULL), pItem(NULL), bKill(FALSE), bClient(FALSE){}
	~CaStartStopParam(){}
	BOOL bStart;
	BOOL bClient; // TRUE to indicate to start the client installation
	BOOL bKill;
	LPVOID pParam;
	CaTreeComponentItemData* pItem;
};

static DWORD IFThreadControlS3(LPVOID pParam)
{
	try
	{
		CaStartStopParam* p = (CaStartStopParam*)pParam;
		if (!p)
			return 1;
		CvManagerViewLeft* pView = (CvManagerViewLeft*)p->pParam;
		if (!pView)
			return 1;
		CaTreeComponentItemData* pItem = (CaTreeComponentItemData*)p->pItem;
		CString strCmd = pItem->GetStartStopString (p->bStart);

		if (p->bClient)
			if (GVvista())
			{
				if (strCmd.Find("param")<0)
				{
					strCmd+= _T(" /param=\"-client\"");
				}
				else
				{
					int length = strCmd.GetLength();
					strCmd.Trim();
					strCmd.Truncate(length - 1);
					strCmd+= _T(" -client\"");
				}
			}
			else
			strCmd += _T(" -client");
		if (!p->bStart && p->bKill)
			if (GVvista())
			{
				if (strCmd.Find("param")<0)
				{
					strCmd+= _T(" /param=\"-kill\"");
				}
				else
				{
					int length = strCmd.GetLength();
					strCmd.Trim();
					strCmd.Truncate(length - 1);
					strCmd+= _T(" -kill\"");
				}
			}
			else
			strCmd+= _T(" -kill");

		BOOL bStartName = FALSE;
		CaPageInformation* pPageInfo = pItem->GetPageInformation();
		if (p->bStart && pPageInfo)
		{
			CString strClassName = pPageInfo->GetClassName();
			if (strClassName.CompareNoCase (_T("NAME")) == 0 || strClassName.CompareNoCase (_T("INSTALLATION")) == 0)
				bStartName = TRUE;
		}

		delete p;
		return pView->ThreadProcControlS3((LPVOID)(LPCTSTR)strCmd, bStartName);
	}
	catch (...)
	{
		TRACE0 ("Raise exception at: IFThreadControlS3(LPVOID pParam) \n");
	}
	return 0;
}

static DWORD IFThreadControlS4(LPVOID pParam)
{
	try
	{
		CaStartStopParam* p = (CaStartStopParam*)pParam;
		if (!p)
			return 1;
		CvManagerViewLeft* pView = (CvManagerViewLeft*)p->pParam;
		if (!pView)
			return 1;
		CString strCmd;
		if (p->bStart)
		{
			if (p->bClient)
				if (GVvista())
					strCmd = _T("winstart /start /param=\"-client -service\"");
				else
				strCmd = _T("ingstart -client -service");
			else
				if(GVvista())
				strCmd = _T("winstart /start /param=\"-service\"");
				else
				strCmd = _T("ingstart -service");
		}
		else
		{
			if (GVvista())
			{
				strCmd = _T("winstart /stop");
				if (p->bKill)
					strCmd+=_T(" /param=\"-kill\"");
			}
			else
			{
				strCmd = _T("ingstop");
				if (p->bKill)
					strCmd+= _T(" -kill");
			}
		}
		
		BOOL bStart = p->bStart;
		delete p;
		return pView->ThreadProcControlS4((LPVOID)(LPCTSTR)strCmd, bStart);
	}
	catch (...)
	{
		TRACE0 ("Raise exception: IFThreadControlS4(LPVOID pParam)\n");
	}
	return 0;
}



UINT CvManagerViewLeft::ThreadProcControlS1 (LPVOID pParam)
{
	CaTreeComponentItemData* pItem = (CaTreeComponentItemData*)pParam;
	BOOL bSuccess;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	int nStatus = 0;
	SHELLEXECUTEINFO shex;

	try
	{
		CString strVcbf;
		#if defined (MAINWIN)
		TCHAR tchszCommand [] = _T("vcbf");
		#else
		TCHAR tchszCommand [] = _T("vcbf.exe");
		#endif
		strVcbf = tchszCommand;
		memset(&StartupInfo, 0, sizeof(STARTUPINFO));
		StartupInfo.cb = sizeof(STARTUPINFO);
		StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
		StartupInfo.wShowWindow = SW_SHOWDEFAULT;
		
		//
		// Check to see if VCBF is already running:
		CString strNamedMutex = _T("INGRESxVCBFxSINGLExINSTANCE");
		strNamedMutex +=  theApp.GetCurrentInstallationID();
		HANDLE hMutexSingleInstance = CreateMutex (NULL, NULL, strNamedMutex);
		UINT nEorror = GetLastError();
		if (hMutexSingleInstance)
			CloseHandle(hMutexSingleInstance);
		if (nEorror == ERROR_ALREADY_EXISTS)
		{
			CString strTitleVcbf = _T("Configuration Manager"); // Must synchronize with vcbf resouce.
			strTitleVcbf += theApp.GetCurrentInstallationID();
			//
			// An instance of VCBF already exists:
			CWnd* pWndPrev = CWnd::FindWindow(_T("VCBF"), strTitleVcbf);
			if (pWndPrev)
			{
				//_T("An instance of Visual CBF is already running.");
				// Activate the previous application:
				::SendMessage (pWndPrev->m_hWnd, WMUSRMSG_ACTIVATE, 0, 0);
				::SetForegroundWindow (pWndPrev->m_hWnd);
				return FALSE;
			}
			return FALSE;
		}

		if (GVvista())
		{
			CString vcbfParams;

			if (pItem && !pItem->m_strComponentType.IsEmpty())
			{
			vcbfParams = _T("/c=");
			vcbfParams += _T('"');
			vcbfParams += pItem->m_strComponentTypes;
			if (!pItem->m_strComponentName.IsEmpty() && pItem->m_strComponentName.CompareNoCase(GetLocDefConfName()) !=0 )
			{
				vcbfParams += _T('\\');
				vcbfParams += pItem->m_strComponentName;
			}
			vcbfParams += _T('"');
			}

		    memset(&shex, 0, sizeof(shex));
		    shex.cbSize	= sizeof( SHELLEXECUTEINFO );
		    shex.hwnd		= NULL;
		    shex.lpVerb 	= "open";
		    shex.lpFile	= strVcbf ;
		    shex.lpParameters	= vcbfParams;
		    shex.nShow		= SW_NORMAL;
		    
			if (!(bSuccess = ShellExecuteEx( &shex )) )
			{
				throw _T("error");
			}

		}
		else
		{

		//
		// Prepare to run VCBF:
		if (pItem && !pItem->m_strComponentType.IsEmpty())
		{
			strVcbf += _T(" /c=");
			strVcbf += _T('"');
			strVcbf += pItem->m_strComponentTypes;
			if (!pItem->m_strComponentName.IsEmpty() && pItem->m_strComponentName.CompareNoCase (GetLocDefConfName()) != 0)
			{
				strVcbf += _T('\\');
				strVcbf += pItem->m_strComponentName;
			}
			strVcbf += _T('"');
		}
		TRACE1 ("Run %s\n", (LPCTSTR)strVcbf);
		bSuccess = CreateProcess(
			NULL,                    // pointer to name of executable module
			(LPTSTR)(LPCTSTR)strVcbf,// pointer to command line string
			NULL,                    // pointer to process security attributes
			NULL,                    // pointer to thread security attributes
			FALSE,                   // handle inheritance flag
			0,                       // creation flags
			NULL,                    // pointer to new environment block
			NULL,                    // pointer to current directory name 
			&StartupInfo,            // pointer to STARTUPINFO
			&ProcInfo                // pointer to PROCESS_INFORMATION
		);
		if (!bSuccess)
		{
			throw _T("error");
		}
		}
	}
	catch (...)
	{
		LPTSTR lpszError = new TCHAR [128];
		lstrcpy (lpszError, _T("Cannot Start the VCBF executable."));
		::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);

		nStatus = 1;
	}
	return nStatus;
}


UINT CvManagerViewLeft::ThreadProcControlS3 (LPVOID pParam, BOOL bStartName)
{
    int			nStatus = 0;
    BOOL		bSuccess;
    STARTUPINFO		StartupInfo;
    PROCESS_INFORMATION	ProcInfo;
    HANDLE		m_hChildStdoutRd, m_hChildStdoutWr;
    HANDLE		m_hChildStdinRd, m_hChildStdinWr;
    HANDLE		hChildStdinWr;
    SECURITY_ATTRIBUTES	sa;
	SHELLEXECUTEINFO shex;

    LPTSTR lpszCommand = (LPTSTR)pParam;
    try
    {
	BOOL bError = FALSE;

	if (!lpszCommand)
	{
	    m_pThread3 = NULL;
	    return 1;
	}
	
	CaStartNAMExINSTALLATION StartName (FALSE);
	if (bStartName)
		StartName.SetStart(TRUE);

	CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
	theApp.SetComponentStartStop(TRUE);

	/* Set the Start/Stop event to non-signaled: */
	ResetEvent (theApp.GetEventStartStop());

	if (pDoc)
	{
#ifndef MAINWIN
	    ::PostMessage ( pDoc->m_hwndMainDlg, WMUSRMSG_IVM_START_STOP,
			    (WPARAM)ID_START, 0 );
#else
	    ::SendMessage ( pDoc->m_hwndMainDlg, WMUSRMSG_IVM_START_STOP,
			    (WPARAM)ID_START, 0 );

	    /* Give a brief moment for the clock to start up. */
	    Sleep(500);
#endif /* MAINWIN */
	}
	if (GVvista())
	{
			 char seps[] = " ,\t\n";
		     CString file;
			 CString parameters;
			 char *token;
			 char command[MAX_PATH];
			 strncpy(command, lpszCommand, strlen(lpszCommand));

		     token = strtok((char *)command, seps);	
			 file.SetString(token);
			 while (token != NULL)
			 {
				 token = strtok(NULL, seps);
				 parameters += token;
				 parameters += _T(" ");
			 }

		     memset(&shex, 0, sizeof(shex));
		     shex.cbSize	= sizeof( SHELLEXECUTEINFO );
		     shex.fMask 	= SEE_MASK_NOCLOSEPROCESS;
		     shex.hwnd		= NULL;
		     shex.lpVerb 	= "open";
		     shex.lpFile	= file ;
		     shex.lpParameters	= parameters;
		     shex.nShow		= SW_NORMAL;
		     if (!(bSuccess = ShellExecuteEx( &shex )) )
			 {
				bError = TRUE;
				goto Finish;
			 }

			 if (bSuccess)
			 {
				/* Set the Start/Stop event to non-signaled: */
				ResetEvent (theApp.GetEventStartStop());

				DWORD dwResult = WaitForSingleObject (shex.hProcess, INFINITE);

				switch (dwResult)
				{
				case WAIT_OBJECT_0:
					//theApp.PostThreadMessage (WMUSRMSG_IVM_WORKERTHREADUPDATE, 0, 0);
					nStatus = 0;
					break;

				case WAIT_FAILED:
				case WAIT_ABANDONED:
					TRACE0("ThreadProcControlS3::command fails\n");
					nStatus = 1;
					break;

				default:
					TRACE0("ThreadProcControlS3::command unknown error\n");
					nStatus = 2;
					break;
				}
			 }
			 else
			 {
				TRACE0("ThreadProcControlS3::command creates process failed\n");
				bError = TRUE;
				goto Finish;
			 }
	}
	else
	{
	memset (&sa, 0, sizeof (sa));
	sa.nLength              = sizeof(sa);
	sa.bInheritHandle       = TRUE;	/* make pipe handles inheritable */
	sa.lpSecurityDescriptor = NULL;

	/* Create a pipe for the child's STDOUT */
	if (!CreatePipe(&m_hChildStdoutRd, &m_hChildStdoutWr, &sa, 0))
	{
	    TRACE0("ThreadProcControlS3::command create pipe failed\n");
	    bError = TRUE;
	    goto Finish;
	}

	/* Create a pipe for the child's STDIN */
	if (!CreatePipe(&m_hChildStdinRd, &hChildStdinWr, &sa, 0))
	{
	    TRACE0("ThreadProcControlS3::command create pipe failed\n");
	    bError = TRUE;
	    goto Finish;
	}

	/*
	** Duplicate the write handle to the STDIN pipe so it's
	** not inherited
	*/
	if (!DuplicateHandle(
		GetCurrentProcess(),
		hChildStdinWr,
		GetCurrentProcess(),
		&m_hChildStdinWr,
		0, FALSE,
		DUPLICATE_SAME_ACCESS))
	{
	    TRACE0("ThreadProcControlS3::command duplicate handle failed\n");
	    bError = TRUE;
	    goto Finish;
	}

	/* Don't need it any more */
	CloseHandle(hChildStdinWr);

	memset(&StartupInfo, 0, sizeof(STARTUPINFO));
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	StartupInfo.wShowWindow = SW_HIDE;
	StartupInfo.hStdInput = m_hChildStdinRd;
	StartupInfo.hStdOutput = m_hChildStdoutWr;
	StartupInfo.hStdError = m_hChildStdoutWr;
	bSuccess = CreateProcess(
			NULL,         // pointer to name of executable module
			lpszCommand,  // pointer to command line string
			NULL,         // pointer to process security attributes
			NULL,         // pointer to thread security attributes
			TRUE,         // handle inheritance flag
			NORMAL_PRIORITY_CLASS, // creation flags
			NULL,         // pointer to new environment block
			NULL,         // pointer to current directory name
			&StartupInfo, // pointer to STARTUPINFO
			&ProcInfo     // pointer to PROCESS_INFORMATION
	);

	CloseHandle(m_hChildStdinRd);
	CloseHandle(m_hChildStdoutWr);

	if (bSuccess)
	{
	    /* Set the Start/Stop event to non-signaled: */
	    ResetEvent (theApp.GetEventStartStop());

	    DWORD dwResult = WaitForSingleObject (ProcInfo.hProcess, INFINITE);

	    switch (dwResult)
	    {
		case WAIT_OBJECT_0:
		    //theApp.PostThreadMessage (WMUSRMSG_IVM_WORKERTHREADUPDATE, 0, 0);
		    nStatus = 0;
		    break;

		case WAIT_FAILED:
		case WAIT_ABANDONED:
		    TRACE0("ThreadProcControlS3::command fails\n");
		    nStatus = 1;
		    break;

		default:
		    TRACE0("ThreadProcControlS3::command unknown error\n");
		    nStatus = 2;
		    break;
	    }
	}
	else
	{
	    TRACE0("ThreadProcControlS3::command creates process failed\n");
	    bError = TRUE;
	    goto Finish;
	}
	}

Finish:
	StartName.SetStart (FALSE);
	if (pDoc)
	{
	    ::PostMessage ( pDoc->m_hwndMainDlg, WMUSRMSG_IVM_START_STOP,
			    (WPARAM)ID_STOP, TRUE );
	}

	/* Set the Start/Stop event to signaled: */
	SetEvent (theApp.GetEventStartStop());

	/* Cause to refresh at a hight frequency: */
	Sleep (theApp.GetInstanceTick()/4);
	theApp.SetComponentStartStop(FALSE);
	m_pThread3 = NULL;

	if (bError)
		if (GetLastError() != ERROR_CANCELLED)
		    throw _T("error");
		else
			bError = FALSE;
    }
    catch (...)
    {
	LPTSTR lpszError = new TCHAR [128];
	wsprintf (lpszError, _T("System error (CvManagerViewLeft::ThreadProcControlS3): \nFail to run %s !"), lpszCommand);
	::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
	nStatus = 1;
    }
    return nStatus;
}


UINT CvManagerViewLeft::ThreadProcControlS4 (LPVOID pParam, BOOL bStart)
{
	int         nStatus = 0;
	BOOL        bSuccess;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	HANDLE      m_hChildStdoutRd, m_hChildStdoutWr;
	HANDLE      m_hChildStdinRd, m_hChildStdinWr;
	HANDLE      hChildStdinWr;
	SECURITY_ATTRIBUTES sa;
	LPTSTR lpszCommand = (LPTSTR)pParam;
	SHELLEXECUTEINFO shex;

	try
	{
		BOOL bError = FALSE;
		if (!lpszCommand)
		{
			m_pThread3 = NULL;
			return 1;
		}
		CaStartNAMExINSTALLATION StartInstallation(FALSE);
		if (bStart)
			StartInstallation.SetStart (TRUE);

		CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
		theApp.SetComponentStartStop(TRUE);
		//
		// Set the Start/Stop event to non-signaled:
		ResetEvent (theApp.GetEventStartStop());

		if (pDoc)
		{
			::PostMessage (pDoc->m_hwndMainDlg, WMUSRMSG_IVM_START_STOP, (WPARAM)ID_START, 0);
			Sleep(500);
		}
		if (GVvista())
		{
		     char seps[] = " ,\t\n";
		     CString file, parameters;
			 char *token;
		     token = strtok((char *)lpszCommand, seps);	
			 file.SetString(token);
			 while (token != NULL)
			 {
				 token = strtok(NULL, seps);
				 parameters += token;
				 parameters += _T(" ");
			 }


		     memset(&shex, 0, sizeof(shex));
		     shex.cbSize	= sizeof( SHELLEXECUTEINFO );
		     shex.fMask 	= SEE_MASK_NOCLOSEPROCESS;
		     shex.hwnd		= NULL;
		     shex.lpVerb 	= "open";
		     shex.lpFile	= file ;
		     shex.lpParameters	= parameters;
		     shex.nShow		= SW_NORMAL;
		     if (!ShellExecuteEx( &shex ) )
			bError = TRUE;

		     if (shex.hProcess != NULL)
		     {
			//
			// Set the Start/Stop event to non-signaled:
			ResetEvent (theApp.GetEventStartStop());
			DWORD dwResult = WaitForSingleObject (shex.hProcess, INFINITE);

			switch (dwResult)
			{
			case WAIT_OBJECT_0:
				nStatus = 0;
				break;

			case WAIT_FAILED:
			case WAIT_ABANDONED:
				TRACE0("ThreadProcControlS4::command fails\n");
				nStatus = 1;
				break;

			default:
				TRACE0("ThreadProcControlS4::command unknown error\n");
				nStatus = 2;
				break;
			}
		     }
		     else
		     {
			TRACE0("ThreadProcControlS4::command create process failed\n");
			bError = TRUE;
		     }
		     StartInstallation.SetStart (FALSE);

    		     if (pDoc)
		     {
			::PostMessage (pDoc->m_hwndMainDlg, WMUSRMSG_IVM_START_STOP, (WPARAM)ID_STOP, TRUE);
			Sleep (500);
		     }
		     //
		     // Set the Start/Stop event to signaled:
		     SetEvent (theApp.GetEventStartStop());
		     theApp.SetComponentStartStop(FALSE);
		     return 1;
		}
		else
		{
		memset (&sa, 0, sizeof (sa));
		sa.nLength              = sizeof(sa);
		sa.bInheritHandle       = TRUE; // make pipe handles inheritable 
		sa.lpSecurityDescriptor = NULL;

		//
		// Create a pipe for the child's STDOUT
		if (!bError && !CreatePipe(&m_hChildStdoutRd, &m_hChildStdoutWr, &sa, 0))
		{
			TRACE0("ThreadProcControlS3::command create pipe failed\n");
			bError = TRUE;
		}

		//
		// Create a pipe for the child's STDIN 
		if (!bError && !CreatePipe(&m_hChildStdinRd, &hChildStdinWr, &sa, 0))
		{
			TRACE0("ThreadProcControlS3::command create pipe failed\n");
			bError = TRUE;
		}

		//
		// Duplicate the write handle to the STDIN pipe so it's not inherited
		if (!bError && 
			!DuplicateHandle(
			GetCurrentProcess(),
			hChildStdinWr,
			GetCurrentProcess(),
			&m_hChildStdinWr,
			0, FALSE,
			DUPLICATE_SAME_ACCESS))
		{
			TRACE0("ThreadProcControlS3::command duplicate handle failed\n");
			bError = TRUE;
		}

		if (!bError)
		{
			//
			// Don't need it any more:
			CloseHandle(hChildStdinWr);

			memset(&StartupInfo, 0, sizeof(STARTUPINFO));
			StartupInfo.cb = sizeof(STARTUPINFO);
			StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
			StartupInfo.wShowWindow = SW_HIDE;
			StartupInfo.hStdInput = m_hChildStdinRd;
			StartupInfo.hStdOutput = m_hChildStdoutWr;
			StartupInfo.hStdError = m_hChildStdoutWr;

			bSuccess = CreateProcess(
				NULL,         // pointer to name of executable module
				lpszCommand,  // pointer to command line string
				NULL,         // pointer to process security attributes
				NULL,         // pointer to thread security attributes
				TRUE,         // handle inheritance flag
				NORMAL_PRIORITY_CLASS, // creation flags
				NULL,         // pointer to new environment block
				NULL,         // pointer to current directory name
				&StartupInfo, // pointer to STARTUPINFO
				&ProcInfo     // pointer to PROCESS_INFORMATION
			);

			CloseHandle(m_hChildStdinRd);
			CloseHandle(m_hChildStdoutWr);


			if (bSuccess)
			{
				//
				// Set the Start/Stop event to non-signaled:
				ResetEvent (theApp.GetEventStartStop());
				DWORD dwResult = WaitForSingleObject (ProcInfo.hProcess, INFINITE);

				switch (dwResult)
				{
				case WAIT_OBJECT_0:
					nStatus = 0;
					break;

				case WAIT_FAILED:
				case WAIT_ABANDONED:
					TRACE0("ThreadProcControlS4::command fails\n");
					nStatus = 1;
					break;

				default:
					TRACE0("ThreadProcControlS4::command unknown error\n");
					nStatus = 2;
					break;
				}
			}
			else
			{
				TRACE0("ThreadProcControlS4::command creates process failed\n");
				bError = TRUE;
			}
			StartInstallation.SetStart (FALSE);

			if (pDoc)
			{
				::PostMessage (pDoc->m_hwndMainDlg, WMUSRMSG_IVM_START_STOP, (WPARAM)ID_STOP, TRUE);
				Sleep (500);
			}
			//
			// Set the Start/Stop event to signaled:
			SetEvent (theApp.GetEventStartStop());
			theApp.SetComponentStartStop(FALSE);
			return 1;
		}
		} /* if (GVvista()) */
		Sleep (theApp.GetInstanceTick()/4);
		//
		// Set the Start/Stop event to signaled:
		SetEvent (theApp.GetEventStartStop());
		if (pDoc)
		{
			::PostMessage (pDoc->m_hwndMainDlg, WMUSRMSG_IVM_START_STOP, (WPARAM)ID_STOP, TRUE);
			Sleep(500);
		}

		theApp.SetComponentStartStop(FALSE);
		
	}
	catch (...)
	{
		LPTSTR lpszError = new TCHAR [128];
		wsprintf (lpszError, _T("System error (CvManagerViewLeft::ThreadProcControlS4): \nFail to run %s !"), lpszCommand);
		::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
		nStatus = 1;

		TRACE0 ("Raise exception: CvManagerViewLeft::ThreadProcControlS4\n");
	}
	m_pThread3 = NULL;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CvManagerViewLeft drawing

void CvManagerViewLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvManagerViewLeft diagnostics

#ifdef _DEBUG
void CvManagerViewLeft::AssertValid() const
{
	CTreeView::AssertValid();
}

void CvManagerViewLeft::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvManagerViewLeft message handlers


void CvManagerViewLeft::InitializeData()
{
	CWaitCursor doWaitCursor;
	int i;
	CTreeCtrl& cTree = GetTreeCtrl();
	
	if (!m_bAlreadyCreateImageList)
	{
		CImageList imageList;
		HICON hIcon;
		m_ImageList.Create (IM_WIDTH, IM_HEIGHT, TRUE, 18, 8);
		//
		// FOLDERS:
		imageList.Create (IDB_FOLDER26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i <20; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// INSTALLATIONS:
		imageList.DeleteImageList();
		imageList.Create (IDB_INSTALLATION26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i <10; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// BRIDGES:
		imageList.DeleteImageList();
		imageList.Create (IDB_BRIDGE26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// DBMS:
		imageList.DeleteImageList();
		imageList.Create (IDB_DBMS26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// NETS:
		imageList.DeleteImageList();
		imageList.Create (IDB_NET26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// STARS:
		imageList.DeleteImageList();
		imageList.Create (IDB_STAR26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// ICES:
		imageList.DeleteImageList();
		imageList.Create (IDB_ICE26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// GATEWAY:
		imageList.DeleteImageList();
		imageList.Create (IDB_GATEWAY26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// NAMES:
		imageList.DeleteImageList();
		imageList.Create (IDB_NAME26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// RMCMD:
		imageList.DeleteImageList();
		imageList.Create (IDB_RMCMD26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// SECURITY:
		imageList.DeleteImageList();
		imageList.Create (IDB_SECURITY26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// LOCKING:
		imageList.DeleteImageList();
		imageList.Create (IDB_LOCKING26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// LOGGING:
		imageList.DeleteImageList();
		imageList.Create (IDB_LOGGING26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// TRANSACTION:
		imageList.DeleteImageList();
		imageList.Create (IDB_TRANSACTION26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// RECOVERY:
		imageList.DeleteImageList();
		imageList.Create (IDB_RECOVERY26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// ARCHIVER:
		imageList.DeleteImageList();
		imageList.Create (IDB_ARCHIVER26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// JDBC:
		imageList.DeleteImageList();
		imageList.Create (IDB_JDBC26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}
		//
		// DASVR:
		imageList.DeleteImageList();
		imageList.Create (IDB_DASVR26x16, IM_WIDTH, 0, RGB(255,0,255));
		for (i = 0; i < 12; i++)
		{
			hIcon = imageList.ExtractIcon (i);
			m_ImageList.Add (hIcon);
			DestroyIcon (hIcon);
		}

		imageList.DeleteImageList();
		cTree.SetImageList (&m_ImageList, TVSIL_NORMAL);
		m_bAlreadyCreateImageList = TRUE;
	}
}

void CvManagerViewLeft::Refresh(int& nChange)
{
	CTreeCtrl& cTree = GetTreeCtrl();
	CaIvmTree& treeData = theApp.GetTreeData();
ASSERT (FALSE); // leine bellow
//	treeData.Update (&cTree, nChange);
}



BOOL CvManagerViewLeft::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

void CvManagerViewLeft::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
	
	CWaitCursor doWaitCursor;
	CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;
	if (!pFrame->IsAllViewsCreated())
		return;
	CvManagerViewRight* pViewRight = pFrame->GetRightView();
	ASSERT (pViewRight);
	if (!pViewRight)
		return;
	CuMainTabCtrl* pMainTab = pViewRight->GetTabCtrl();
	ASSERT (pMainTab);
	if (!pMainTab)
		return;

	TRACE0("CvManagerViewLeft::OnSelchanged\n");

	CaTreeComponentItemData* pItem = NULL;
	CaPageInformation* pPageInfo = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	
	try
	{
		//
		// Unselect old item:
		if (pNMTreeView->itemOld.hItem != NULL)
		{
		
		}
		
		//
		// Select new item:
		if (pNMTreeView->itemNew.hItem != NULL)
		{
			pItem = (CaTreeComponentItemData*)cTree.GetItemData (pNMTreeView->itemNew.hItem);
			pPageInfo = pItem? pItem->GetPageInformation(): NULL;
			if (!pPageInfo && m_nPreviousPageID == 0)
			{
				CaIvmProperty* pProp = pMainTab->GetCurrentProperty();
				if (pProp)
				{
					CaPageInformation* pOldPageInfo = pProp->GetPageInfo();
					m_nPreviousPageID = pOldPageInfo->GetDlgID(pProp->GetCurSel());
				}
			}

			if (pPageInfo && pItem)
			{
				int nActiveTab = 0;
				pPageInfo->SetIvmItem (pItem);
				CaIvmProperty* pProp = pMainTab->GetCurrentProperty();
				if (pProp) // The previous selected tree item has right pane.
				{
					//
					// Check if the new selected tree item has the right pane which is the same as
					// the previous activated right pane then select that right pane by default:
					CaPageInformation* pOldPageInfo = pProp->GetPageInfo();
					int nMaxPage = pPageInfo->GetNumberOfPage();
					for (int i=0; i<nMaxPage; i++)
					{
						if (pOldPageInfo->GetDlgID(pProp->GetCurSel()) == pPageInfo->GetDlgID(i))
						{
							nActiveTab = i;
							break;
						}
					}
				}
				else
				if (m_nPreviousPageID != 0) // The previous selected tree item has no right pane.
				{
					//
					// Check if the new selected tree item has the right pane which is the same as
					// the last activated right pane then select that right pane by default:
					int nMaxPage = pPageInfo->GetNumberOfPage();
					for (int i=0; i<nMaxPage; i++)
					{
						if (pPageInfo->GetDlgID(i) == m_nPreviousPageID)
						{
							nActiveTab = i;
							break;
						}
					}
				}
				pPageInfo->SetDefaultPage(nActiveTab);
				m_nPreviousPageID = 0;

				BOOL bNameServer = IsNameServer (pPageInfo);
				BOOL bRmcmdServer = IsRmcmdServer(pPageInfo);
		
				CaEventFilter& filter = pPageInfo->GetEventFilter();
				filter.m_strCategory = pItem->m_strComponentType.IsEmpty()? theApp.m_strAll: pItem->m_strComponentType;
				filter.m_strName = pItem->m_strComponentName.IsEmpty() && !bNameServer && !bRmcmdServer? theApp.m_strAll: pItem->m_strComponentName;
				filter.m_strInstance = pItem->m_strComponentInstance.IsEmpty()? theApp.m_strAll: pItem->m_strComponentInstance;
			}
			pMainTab->DisplayPage(pPageInfo);
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvManagerViewLeft::OnSelchanged): \nCannot display page."));
	}
}

LONG CvManagerViewLeft::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CaIvmTree& treeData = theApp.GetTreeData();
	int nChange = COMPONENT_UNCHANGED;
	int nHint = (int)wParam;
	switch (nHint)
	{
	case 0:
		if (IVM_llIsComponentChanged())
			Refresh (nChange);
		break;
	case 1:
		{
			//
			// New events are arriving:
			CTypedPtrList<CObList, CaLoggedEvent*>* pListEvent = (CTypedPtrList<CObList, CaLoggedEvent*>*)lParam;
			if (pListEvent && !pListEvent->IsEmpty())
			{
				CTreeCtrl& cTree = GetTreeCtrl();
				CaLoggedEvent* pEvent = NULL;
				POSITION pos = pListEvent->GetHeadPosition();
				BOOL bAlert = FALSE;
				while (pos != NULL)
				{
					pEvent = pListEvent->GetNext (pos);
					if (pEvent->IsAlert())
					{
						treeData.AlertNew  (&cTree, pEvent);
						bAlert = TRUE;
					}
				}
			}
		}
		break;
	default:
		ASSERT (FALSE);
		break;
	}
	return nChange;
}

LONG_PTR CvManagerViewLeft::OnGetItemData (WPARAM wParam, LPARAM lParam)
{
	CaTreeComponentItemData* pItem = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSel);
	}
	return (LONG_PTR)pItem;
}


int CvManagerViewLeft::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeView::OnCreate(lpCreateStruct) == -1)
		return -1;
	CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc)
	{
		pDoc->m_hwndLeftView = m_hWnd;
		CuDlgMain* pDlgMain = NULL;
		CfManagerFrame* pMFrame = (CfManagerFrame*)GetParentFrame();
		if (pMFrame)
			pDlgMain = (CuDlgMain*)pMFrame->GetParent();
		if (pDlgMain)
			pDoc->m_hwndMainDlg = pDlgMain->m_hWnd;
	}

	return 0;
}

void CvManagerViewLeft::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
	int nImage = -1;
	*pResult = 0;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = GetTreeCtrl();

	try
	{
		CaTreeComponentItemData* pItem = (CaTreeComponentItemData*)cTree.GetItemData (hExpand);
		if (!pItem)
			return;
		if (pNMTreeView->action & TVE_COLLAPSE)
		{
			//
			// Collapseing:
			pItem->m_treeCtrlData.SetExpand (FALSE);
			pItem->SetExpandImages (&cTree, FALSE);
		}
		else
		if (pNMTreeView->action & TVE_EXPAND)
		{
			//
			// Expanding:
			pItem->m_treeCtrlData.SetExpand (TRUE);
			pItem->SetExpandImages (&cTree, TRUE);
		}
	}
	catch(...)
	{
		TRACE0 ("Raise exception: CvManagerViewLeft::OnItemexpanded\n");
	}
}

void CvManagerViewLeft::AlertRead (CaLoggedEvent* pEvent)
{
	int nChange = 0;
	CTreeCtrl& cTree = GetTreeCtrl();
	CaIvmTree& treeData = theApp.GetTreeData();
	treeData.AlertRead(&cTree, pEvent);
	treeData.Display (&cTree, nChange);
	theApp.UpdateShellIcon();
}

void CvManagerViewLeft::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWaitCursor doWaitCursor;
	try
	{
		CaTreeComponentItemData* pItem = NULL;
		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hSelectedItem = cTree.GetSelectedItem();
		int nHint = (int)lHint;
		CaIvmTree& treeData = theApp.GetTreeData();
		//
		// Expand All:
		if (pSender == NULL && nHint == ID_EXPAND_ALL)
		{
			//
			// Lock Tree Components:
			CaTreeDataMutex treeDataMutex;

			treeData.ExpandAll(&cTree);
			if (hSelectedItem)
				cTree.EnsureVisible (hSelectedItem);
			return;
		}

		//
		// Collapse All:
		if (pSender == NULL && nHint == ID_COLLAPSE_ALL)
		{
			//
			// Lock Tree Components:
			CaTreeDataMutex treeDataMutex;

			treeData.CollapseAll(&cTree);
			CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
			ASSERT (pFrame);
			if (!pFrame)
				return;
			if (!pFrame->IsAllViewsCreated())
				return;
			CvManagerViewRight* pViewRight = pFrame->GetRightView();
			ASSERT (pViewRight);
			if (!pViewRight)
				return;
			CuMainTabCtrl* pMainTab = pViewRight->GetTabCtrl();
			ASSERT (pMainTab);
			if (!pMainTab)
				return;
			
			CaTreeComponentItemData* pItem = NULL;
			CaPageInformation* pPageInfo = NULL;
			if (!hSelectedItem)
				return;

			pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelectedItem);
			pPageInfo = pItem? pItem->GetPageInformation(): NULL;
			pMainTab->DisplayPage(pPageInfo);

			return;
		}


		//
		// Start:
		if (pSender == NULL && (nHint == ID_START || nHint == ID_START_CLIENT))
		{
			pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelectedItem);
			if (!pItem)
				return;
			ActivateTabLastStartStop();

			HANDLE hEventStartStop = theApp.GetEventStartStop();
			DWORD dwWait = WaitForSingleObject (hEventStartStop, 0);
			CString strReason;
			switch (dwWait)
			{
			case WAIT_OBJECT_0:
				//
				// We can start or stop component, but before doing this
				// check to see if the dependency rule is not violated ?
				strReason = IVM_AllowedStart (&cTree, pItem);
				break;
			default:
				//
				// The process of Start/Stop is still running. We cannot check the
				// dependency yet at this point:
				
				//_T("A Start or Stop operation is already in progress.");
				strReason.LoadString(IDS_MSG_PROCESS_STARTxSTOP_RUNNING);
				break;
			}
			
			if (!strReason.IsEmpty())
			{
				AfxMessageBox (strReason);
				return;
			}

			CaStartStopParam* pStartStop = new CaStartStopParam();
			pStartStop->bStart = TRUE;
			pStartStop->pParam = (LPVOID)this;
			pStartStop->pItem = pItem;
			if( nHint == ID_START_CLIENT)
				pStartStop->bClient = TRUE;
			//
			// Run ignstart -service:
			#if !defined (MAINWIN)
			CaPageInformation* pPageInfo = pItem->GetPageInformation();
			BOOL ServiceMode = theApp.m_setting.m_bAsService;
			if (ServiceMode && pPageInfo && pPageInfo->GetClassName().CompareNoCase (_T("INSTALLATION")) == 0)
			{
				BOOL bService = IVM_IsServiceInstalled(theApp.m_strIngresServiceName);
				if (bService)
				{
					/* test if DBMS server installed (if not, no need to have the administrators user rights)*/
					BOOL bNeedAdminRights = isDBMS();

					if (bNeedAdminRights && !IVM_HasAdmistratorRights()) {
						int iAnswer = IDCANCEL;
						CString strMsg;
						strMsg.LoadString (IDS_WARNING_NOT_ADMINISTRATOR);
						iAnswer = AfxMessageBox (strMsg, MB_OKCANCEL|MB_ICONEXCLAMATION );
						if (iAnswer == IDCANCEL)
						{
							if (pStartStop)
								delete pStartStop;
							return;
						}
					}
					if (!IVM_IsServiceRunning (theApp.m_strIngresServiceName))
					{
						INT_PTR nAnswer = IDCANCEL;
						if (theApp.m_setting.m_bShowThisMsg1)
						{
							CString strMsg;
							strMsg.LoadString (IDS_MSG_RUN_INGRES_SERVICE);
							CxDlgMessage dlg(this);
							dlg.m_strMessage = strMsg;
							dlg.m_bShowItLater = theApp.m_setting.m_bShowThisMsg1;
							nAnswer = dlg.DoModal();
							if (nAnswer == IDCANCEL)
							{
								if (pStartStop)
									delete pStartStop;
								return;
							}
							theApp.m_setting.m_bShowThisMsg1 = dlg.m_bShowItLater;
							theApp.m_setting.Save();
							if (nAnswer == IDNO)
							{
								CaServiceConfig cf;
								BOOL bSet = FALSE;
								bSet = IVM_FillServiceConfig (cf);
								if (bSet)
									bSet = IVM_SetServicePassword(theApp.m_strIngresServiceName, cf);
								if (!bSet)
								{
									if (pStartStop)
										delete pStartStop;
									return;
								}
							}
						}
						m_pThread3 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS4, (LPVOID)pStartStop, THREAD_PRIORITY_BELOW_NORMAL);
					}
					return;
				}
			}
			#endif

			m_pThread3 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS3, (LPVOID)pStartStop, THREAD_PRIORITY_BELOW_NORMAL);
			return;
		}

		//
		// Stop:
		if (pSender == NULL && nHint == ID_STOP)
		{
			pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelectedItem);
			if (!pItem)
				return;
			ActivateTabLastStartStop();

			HANDLE hEventStartStop = theApp.GetEventStartStop();
			DWORD dwWait = WaitForSingleObject (hEventStartStop, 0);
			CString strReason;
			switch (dwWait)
			{
			case WAIT_OBJECT_0:
				//
				// We can stop or stop component, but before doing this
				// check to see if the dependency rule is not violated ?
				strReason = IVM_AllowedStop (&cTree, pItem);
				break;
			default:
				//
				// The process of Start/Stop is still running. We cannot check the
				// dependency yet at this point:
				
				//_T("A Start or Stop operation is already in progress.");
				strReason.LoadString(IDS_MSG_PROCESS_STARTxSTOP_RUNNING);
				break;
			}
			
			if (!strReason.IsEmpty())
			{
				AfxMessageBox (strReason);
				return;
			}

			CaStartStopParam* pStartStop = new CaStartStopParam();
			pStartStop->bStart = FALSE;
			pStartStop->bKill = FALSE;
			pStartStop->pParam = (LPVOID)this;
			pStartStop->pItem = pItem;
			
			//
			// Run ingstop [-kill]
			#if  defined (OPING20) && !defined (MAINWIN) 
			CaPageInformation* pPageInfo = pItem->GetPageInformation();
			BOOL ServiceMode = theApp.m_setting.m_bAsService;
			if (ServiceMode && pPageInfo && pPageInfo->GetClassName().CompareNoCase (_T("INSTALLATION")) == 0)
			{
				BOOL bService = IsServiceInstalled(theApp.m_strIngresServiceName);
				if (bService)
				{
					if (IsServiceRunning (theApp.m_strIngresServiceName))
						m_pThread3 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS4, (LPVOID)pStartStop, THREAD_PRIORITY_BELOW_NORMAL);
					return;
				}
			}
			#endif

			CString strStopItem;
			CString strMsg;
			strStopItem = pItem->m_strComponentType;
			if (!pItem->m_strComponentInstance.IsEmpty())
			{
				strStopItem += _T(" <");
				strStopItem += pItem->m_strComponentInstance;
				strStopItem += _T(">");
			}
			if (strStopItem.IsEmpty() && pItem->GetPageInformation()->GetClassName().CompareNoCase (_T("INSTALLATION")) == 0)
				strStopItem.LoadString (IDS_THE_INGRES_INST);

			// _T("You've asked to stop %s\nDo you wish to kill the remaining sessions ?")
			AfxFormatString1 (strMsg, IDS_MSG_ASK2STOP_SERVICE, (LPCTSTR)strStopItem);

			int nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNOCANCEL|MB_DEFBUTTON2);
			switch (nAnswer)
			{
			case IDYES:
				pStartStop->bKill = TRUE;
				break;
			case IDNO:
				break;
			default:
				return;
			}

			m_pThread3 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS3, (LPVOID)pStartStop, THREAD_PRIORITY_BELOW_NORMAL);
			return;
		}

		if (pSender == NULL)
		{
			CString strCmd = _T("");
			switch (nHint)
			{
			case ID_CONFIGURE: 
				m_pItem = NULL;
				if (hSelectedItem)
					m_pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelectedItem);
				m_pThread1 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS1, this, THREAD_PRIORITY_NORMAL);
				return;
			case ID_VCBF:
				m_pItem = NULL;
				m_pThread1 = AfxBeginThread((AFX_THREADPROC)IFThreadControlS1, this, THREAD_PRIORITY_NORMAL);
				return;

			case ID_NETWORKUTILITY:
				strCmd = _T("ingnet");
				break;
			case ID_VISUALDBA:
				strCmd = _T("vdba");
				break;
			case ID_SQLQUERY:
				strCmd = _T("vdbasql");
				break;
			case ID_IPM:
				strCmd = _T("vdbamon");
				break;
			case ID_IIA:
				strCmd = _T("iia -loop");
				break;
			case ID_IEA:
				strCmd = _T("iea");
				break;
			case ID_VCDA:
				strCmd = _T("vcda");
				break;
			case ID_VDDA:
				strCmd = _T("vdda");
				break;
			case ID_IJA:
				strCmd = _T("ija");
				break;
			case ID_IILINK:
				strCmd = _T("iilink");
				break;
			case ID_PERFORMANCE_WIZARD:
				strCmd = _T("perfwiz");
				break;
			case ID_INGRES_PROMPT:
				{
					TCHAR tchPath[MAX_PATH+1];
					CString strIngresParam;
					CString strWorkPath = _T("");
					strCmd  = _T("cmd.exe");
					HRESULT hr = SHGetFolderPath(m_hWnd, CSIDL_SYSTEM,  NULL, 0, tchPath);
					if (SUCCEEDED(hr))
					{
						strCmd = tchPath;
						strCmd += _T("\\cmd.exe");
					}
					hr = SHGetFolderPath(m_hWnd, CSIDL_PROFILE ,  NULL, 0, tchPath);
					if (SUCCEEDED(hr))
						strWorkPath = tchPath;
					strIngresParam.Format(_T("/K \"%s\\ingres\\bin\\setingenvs.bat\""), (LPCTSTR)theApp.m_strIISystem);

					SHELLEXECUTEINFO shell;
					memset (&shell, 0, sizeof(shell));
					shell.cbSize = sizeof(shell);
					shell.fMask  = 0;
					shell.lpFile = strCmd;
					shell.nShow  = SW_SHOWNORMAL;
					shell.lpDirectory = strWorkPath; 
					shell.lpParameters= strIngresParam; 
					ShellExecuteEx(&shell);
				}
				return;
			case ID_INGRES_CACHE:
			case ID_ERROR_LOG:
			case ID_README:
			case ID_IGRESDOC_CMDREF:
			case ID_IGRESDOC_DBA:
			case ID_IGRESDOC_DTP:
			case ID_IGRESDOC_ESQLC:
			case ID_IGRESDOC_EQUEL:
			case ID_IGRESDOC_GS:
			case ID_IGRESDOC_ICEUG:
			case ID_IGRESDOC_MG:
			case ID_IGRESDOC_OME:
			case ID_IGRESDOC_OAPI:
			case ID_IGRESDOC_OSQL:
			case ID_IGRESDOC_QUELREF:
			case ID_IGRESDOC_REP:
			case ID_IGRESDOC_SQLREF:
			case ID_IGRESDOC_STARUG:
			case ID_IGRESDOC_SYSADM:
			case ID_IGRESDOC_QRYTOOLS:
			case ID_IGRESDOC_INGNET:
			case ID_IGRESDOC_IPM:
			case ID_IGRESDOC_RELEASE:
			case ID_IGRESDOC_UFADT:
				{
					CString strPath = GetIngresDocumentation(nHint);
					SHELLEXECUTEINFO shell;
					memset (&shell, 0, sizeof(shell));
					shell.cbSize = sizeof(shell);
					shell.fMask  = 0;
					shell.lpFile = strPath;
					shell.nShow  = SW_SHOWNORMAL;
					CWnd* pMainWnd = AfxGetMainWnd();
					if (pMainWnd)
						shell.hwnd = pMainWnd->m_hWnd;
					ShellExecuteEx(&shell);
				}
				return;
			case ID_ABOUT_ADVANTAGE:
				OnAppAbout();
				return;
			default:
				return;
			}

			if (!strCmd.IsEmpty())
			{
				CaStartApplication* pAppParam = new CaStartApplication(strCmd);
				CWnd* pMainWnd = AfxGetMainWnd();
				if (pMainWnd)
					pAppParam->SetWindowCaller(pMainWnd->m_hWnd);
				AfxBeginThread((AFX_THREADPROC)IFThreadControlRunApp, (LPVOID)pAppParam, THREAD_PRIORITY_NORMAL);
			}
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvManagerViewLeft::OnUpdate): \nExecute command failed"));
	}
}


LONG CvManagerViewLeft::OnUpdateComponent(WPARAM wParam, LPARAM lParam)
{
	CTypedPtrList<CObList, CaTreeComponentItemData*>* pListComponent = NULL;
	pListComponent = (CTypedPtrList<CObList, CaTreeComponentItemData*>*)lParam;
	BOOL bInstance = (BOOL)wParam;
	SHORT nVk[3];
	nVk[0] = GetKeyState(VK_LBUTTON);
	nVk[1] = GetKeyState(VK_RBUTTON);
	nVk[2] = GetKeyState(VK_MBUTTON);

	//
	// Mouse button is down, so do not update the tree control:
	if ((nVk[0] & ~1) != 0 || (nVk[1] & ~1) != 0 || (nVk[1] & ~1) != 0 || m_bPopupOpen)
	{
		return 0;
	}

	try
	{
		CaIvmTree& treeData = theApp.GetTreeData();
		int nChange = COMPONENT_UNCHANGED;

		CTreeCtrl& cTree = GetTreeCtrl();
		treeData.Update(pListComponent, &cTree, bInstance, nChange);
		// Force to refresh status panes (all panes that handle WMUSRMSG_IVM_COMPONENT_CHANGED)
		nChange = COMPONENT_CHANGEEDALL;
		if (nChange != COMPONENT_UNCHANGED)
		{
			CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
			ASSERT (pFrame);
			if (pFrame)
			{
				if (pFrame->IsAllViewsCreated())
				{
					CvManagerViewRight* pViewRight = pFrame->GetRightView();
					ASSERT (pViewRight);
					if (pViewRight)
					{
						CuMainTabCtrl* pMainTab = pViewRight->GetTabCtrl();
						ASSERT (pMainTab);
						if (pMainTab)
						{
							CWnd* pCurPane = pMainTab->GetCurrentPage();
							if (pCurPane && IsWindow (pCurPane->m_hWnd))
								pCurPane->SendMessage (WMUSRMSG_IVM_COMPONENT_CHANGED);
						}
					}
				}
			}
		}
	}
	catch (...)
	{


	}
	return 0;
}


LONG CvManagerViewLeft::OnUpdateComponentIcon(WPARAM wParam, LPARAM lParam)
{
	try
	{
		int nChange = 0;
		CaIvmTree& treeData = theApp.GetTreeData();
		CTreeCtrl& cTree = GetTreeCtrl();
		treeData.Display (&cTree, nChange);
	}
	catch (...)
	{

	}
	return 0;
}

//
// lParam is a list of events.
// wParam (BOOL): TRUE -> Notify altert (message box) 
LONG CvManagerViewLeft::OnUpdateAlert(WPARAM wParam, LPARAM lParam)
{
	BOOL bAlert = FALSE;
	BOOL bNotify = (BOOL)wParam;
	CTypedPtrList<CObList, CaLoggedEvent*>* plistEvent = NULL;
	CaLoggedEvent* pEvent = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	CaIvmTree& treeData = theApp.GetTreeData();

	plistEvent = (CTypedPtrList<CObList, CaLoggedEvent*>*)lParam;
	if (plistEvent)
	{
		POSITION pos = plistEvent->GetHeadPosition();
		while (pos != NULL)
		{
			pEvent = plistEvent->GetNext (pos);
			if (pEvent->IsAlert() && !pEvent->IsRead())
			{
				treeData.AlertNew  (&cTree, pEvent);
				bAlert = TRUE;
			}
		}
	}
	
	//
	// Update the Icons:
	int nChange = 0;
	if (bAlert && bNotify)
	{
		treeData.Display (&cTree, nChange);
		CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
		ASSERT (pDoc);
		if (!pDoc)
			return 0;
		if (!theApp.m_bMessageBoxEvent && theApp.m_setting.m_bMessageBox)
		{
			::PostMessage (pDoc->m_hwndMainFrame, WMUSRMSG_IVM_NEW_EVENT, 0, 0);
		}
	}

	return 0;
}

//
// wParam = 0 (Update Unalert event by event)
// wParam = 1 (Update Unalert all events)
LONG CvManagerViewLeft::OnUpdateUnalert(WPARAM wParam, LPARAM lParam)
{
	if ((int)wParam == 0)
	{
		CaLoggedEvent* pEvent = (CaLoggedEvent*)lParam;
		if (!pEvent)
			return 0;
		AlertRead (pEvent);
	}
	else
	if ((int)wParam == 1)
	{
		AlertRead(NULL);
	}
	return 0;
}


void CvManagerViewLeft::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CTreeView::OnRButtonDown(nFlags, point);
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUP_TREECOMPONENT));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;
	//
	// Menu handlers are coded in this source:
	// So do not use the following statement:
	// while (pWndPopupOwner->GetStyle() & WS_CHILD)
	//     pWndPopupOwner = pWndPopupOwner->GetParent();
	//
	CBitmap bmShield, bmShield2;
	bmShield.LoadBitmap(IDB_SHIELD16X16);
	bmShield2.LoadBitmap(IDB_SHIELD16X16);
	if (pPopup && GVvista())
	{
		pPopup->SetMenuItemBitmaps(ID_START, MF_BYCOMMAND, &bmShield, &bmShield2); 
		pPopup->SetMenuItemBitmaps(ID_STOP, MF_BYCOMMAND, &bmShield, &bmShield2); 
		pPopup->SetMenuItemBitmaps(ID_START_CLIENT, MF_BYCOMMAND, &bmShield, &bmShield2); 
		pPopup->SetMenuItemBitmaps(ID_CONFIGURE, MF_BYCOMMAND, &bmShield, &bmShield2); 
	}
	CPoint p = point;
	ClientToScreen(&p);
	BOOL bStartEnable = FALSE;
	BOOL bStartClientEnable = FALSE;
	BOOL bStopEnable = FALSE;
	CaTreeComponentItemData* pItem = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	UINT nFlag = TVHT_ONITEM;
	HTREEITEM hHit = cTree.HitTest (point, &nFlag);
	if (hHit)
		cTree.SelectItem (hHit);
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSel);
		if (pItem)
		{
			if (pItem->IsFolder())
				return;
			bStartEnable = pItem->IsStartEnabled();
			bStopEnable = pItem->IsStopEnabled();
			if (pItem->GetPageInformation()!=NULL) { /* tree branch with no tab (cannot be the installation branch) */
				if (pItem->GetPageInformation()->GetClassName().CompareNoCase (_T("INSTALLATION")) == 0 && bStartEnable)
					bStartClientEnable = TRUE;
			}
		}
	}

#if !defined (MAINWIN)
	if (theApp.GetPlatformID() == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
	{
		if (pPopup)
			pPopup->DeleteMenu (ID_START_CLIENT, MF_BYCOMMAND);
	}
#endif

	if (bStartEnable)
		menu.EnableMenuItem (ID_START, MF_BYCOMMAND|MF_ENABLED);
	else
		menu.EnableMenuItem (ID_START, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
	if (bStartClientEnable)
		menu.EnableMenuItem (ID_START_CLIENT, MF_BYCOMMAND|MF_ENABLED);
	else
		menu.EnableMenuItem (ID_START_CLIENT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
	if (bStopEnable)
		menu.EnableMenuItem (ID_STOP, MF_BYCOMMAND|MF_ENABLED);
	else
		menu.EnableMenuItem (ID_STOP, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

	m_bPopupOpen = TRUE;
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, pWndPopupOwner);
	m_bPopupOpen = FALSE;
}

LONG CvManagerViewLeft::OnReactivateSelection(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor doWaitCursor;
	CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return 0;
	if (!pFrame->IsAllViewsCreated())
		return 0;
	CvManagerViewRight* pViewRight = pFrame->GetRightView();
	ASSERT (pViewRight);
	if (!pViewRight)
		return 0;
	CuMainTabCtrl* pMainTab = pViewRight->GetTabCtrl();
	ASSERT (pMainTab);
	if (!pMainTab)
		return 0;

	TRACE0("CvManagerViewLeft::OnReactivateSelection\n");

	CaTreeComponentItemData* pItem = NULL;
	CaPageInformation* pPageInfo = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	
	try
	{
		LPCTSTR lpszItem = (LPCTSTR)wParam;
		if (lpszItem != NULL)
		{
			CString strItem;
			HTREEITEM hRoot  = cTree.GetRootItem();
			while (hRoot)
			{
				strItem = cTree.GetItemText (hRoot);
				if (strItem.CompareNoCase (lpszItem) == 0)
				{
					cTree.SelectItem (hRoot);
					break;
				}
				hRoot = cTree.GetNextItem(hRoot, TVGN_NEXT);
			}
			return 0;
		}

		HTREEITEM hSel = cTree.GetSelectedItem();
		if (!hSel)
			return 0;
		pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSel);
		pPageInfo = pItem? pItem->GetPageInformation(): NULL;
		if (pPageInfo && pItem)
		{
			pPageInfo->SetIvmItem (pItem);
			CaIvmProperty* pProp = pMainTab->GetCurrentProperty();
			if (pProp)
			{
				CaPageInformation* pOldPageInfo = pProp->GetPageInfo();
				pPageInfo->SetDefaultPage(pProp->GetCurSel());
			}
		}
		pMainTab->DisplayPage(pPageInfo);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvManagerViewLeft::OnReactivateSelection): \nCannot display page."));
	}

	return 0;
}

LONG CvManagerViewLeft::OnGetState(WPARAM wParam, LPARAM lParam)
{
	BOOL bStarted = FALSE;
	try
	{
		int nType = (int)wParam;
		CTreeCtrl& cTree = GetTreeCtrl();
		bStarted = IVM_IsStarted(&cTree, nType);
	}
	catch (...)
	{

	}
	return (LONG)bStarted;
}


LONG CvManagerViewLeft::OnShutDownIngres(WPARAM wParam, LPARAM lParam)
{
	try
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		if (m_pThread3)
		{
			//
			// If the start/stop is being running, just wait until it finishes.
			UINT nMn = 5*60*1000;
			WaitForSingleObject (m_pThread3->m_hThread, nMn);
			m_pThread3 = NULL;
		}
		IVM_ShutDownIngres(&cTree);
	}
	catch (...)
	{

	}
	return 0;
}



void CvManagerViewLeft::OnStart()
{
	OnUpdate(NULL, (LPARAM)ID_START, NULL);
}

void CvManagerViewLeft::OnStartClient()
{
	OnUpdate(NULL, (LPARAM)ID_START_CLIENT, NULL);
}

void CvManagerViewLeft::OnStop()
{
	OnUpdate(NULL, (LPARAM)ID_STOP, NULL);
}

void CvManagerViewLeft::OnConfigure()
{
	OnUpdate(NULL, (LPARAM)ID_CONFIGURE, NULL);
}

void CvManagerViewLeft::ActivateTabLastStartStop()
{
	CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;
	if (!pFrame->IsAllViewsCreated())
		return;
	CvManagerViewRight* pViewRight = pFrame->GetRightView();
	ASSERT (pViewRight);
	if (!pViewRight)
		return;
	CuMainTabCtrl* pMainTab = pViewRight->GetTabCtrl();
	ASSERT (pMainTab);
	if (!pMainTab)
		return;
	CWnd* pCurrentActivePane = pMainTab->GetCurrentPage();
	//
	// For the status pane, activate immediately the Tab "Last Start/Stop output"
	if (pCurrentActivePane && IsWindow(pCurrentActivePane->m_hWnd))
		pCurrentActivePane->SendMessage (WMUSRMSG_IVM_NEW_EVENT, 1, 0);
}

LONG CvManagerViewLeft::OnErrorOccurred(WPARAM wParam, LPARAM lParam)
{
	DWORD dwError = (DWORD)wParam;
	try
	{
		switch (dwError)
		{
		case ERROR_SERVICE_LOGON_FAILED:
			{
				CaTreeComponentItemData* pItem = NULL;
				CTreeCtrl& cTree = GetTreeCtrl();
				HTREEITEM hSelectedItem = cTree.GetSelectedItem();
				if (!hSelectedItem)
					break;
				pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelectedItem);
				if (!pItem)
					break;
				if (pItem->GetPageInformation()->GetClassName().CompareNoCase(_T("INSTALLATION")) != 0)
					break;
				CaServiceConfig cf;
				if (IVM_FillServiceConfig (cf) && IVM_SetServicePassword(theApp.m_strIngresServiceName, cf))
					OnStart();
			}
			break;
		default:
			break;
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvManagerViewLeft::OnErrorOccurred): Cannot handle error."));
	}
	return 0;
}



LONG CvManagerViewLeft::OnRunProgram(WPARAM wParam, LPARAM lParam)
{
	int nCode = (int)wParam;
	//
	// Convention: 1 -> Run VCBF!
	if (nCode == 1)
	{
		OnConfigure();
	}
	return 0;
}

static UINT StartIngresApplication(LPVOID pParam)
{
	CString strFail = _T("");
	DWORD dwLastError = 0;
	CaStartApplication* pInfo = (CaStartApplication*)pParam;
	if (!pInfo)
		return 0;
	BOOL bOk;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	CString strCmd = pInfo->GetName();

	// Execute the command with a call to the CreateProcess API call.
	memset(&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = pInfo->GetShowWindow()? SW_SHOW: SW_SHOWDEFAULT;
	bOk = CreateProcess(NULL, (LPTSTR)(LPCTSTR)strCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!bOk)
	{
		dwLastError = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage (
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwLastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);
		strFail = (LPCTSTR)lpMsgBuf;
		LocalFree(lpMsgBuf);
		HWND hWndCaller = pInfo->GetWindowCaller();
		if (hWndCaller && !strFail.IsEmpty())
		{
			LPTSTR lpszError = new TCHAR [strFail.GetLength() + 1];
			_tcscpy (lpszError, strFail);
			::PostMessage (hWndCaller, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
		}
	}
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	delete pInfo;
	if (!bOk)
		return 0;
	return 1;
}

static BOOL QueryPathValues (HKEY hKey, LPCTSTR lpszSubKey, CString& strPath)
{
	CString strSubkey = lpszSubKey; 
	TCHAR tchszBufferValue[1024];
	long lRetSize;
	long ls = RegQueryValue (HKEY_CLASSES_ROOT, lpszSubKey, tchszBufferValue, &lRetSize);
	if (ls == ERROR_SUCCESS)
	{
		strPath = tchszBufferValue;
		return TRUE;
	}
	else
	{
		strPath = _T("");
		return FALSE;
	}
}

static CString GetIngresDocumentation(int nHint)
{
	CString strFile = theApp.m_strIISystem + consttchszIngFiles;
#if defined (MAINWIN)
	strFile += _T("english/");
#else
	strFile += _T("english\\");
#endif
	switch (nHint)
	{
	case ID_IGRESDOC_CMDREF:
		strFile += _T("P002792E.pdf"); // Command Reference Guide
		break;
	case ID_IGRESDOC_DBA:
		strFile += _T("P002662E.pdf"); // Database Administrator Guide
		break;
	case ID_IGRESDOC_DTP:
		strFile += _T("P002772E.pdf"); // Distributed Transaction Processing User Guide
		break;
	case ID_IGRESDOC_ESQLC:
		strFile += _T("P002812E.pdf"); // Embedded SQL Companion Guide
		break;
	case ID_IGRESDOC_EQUEL:
		strFile += _T("P002822E.pdf"); // Embedded QUEL Companion Guide
		break;
	case ID_IGRESDOC_GS:
		strFile += _T("P002612E.pdf"); // Getting Started
		break;
	case ID_IGRESDOC_ICEUG:
		strFile += _T("P002802E.pdf"); // Web Deployment Option User Guide
		break;
	case ID_IGRESDOC_MG:
		strFile += _T("P002752E.pdf"); // Migration Guide
		break;
	case ID_IGRESDOC_OME:
		strFile += _T("P002692E.pdf"); // Object Management Extension User Guide
		break;
	case ID_IGRESDOC_OAPI:
		strFile += _T("P002842E.pdf"); // OpenAPI User Guide
		break;
	case ID_IGRESDOC_OSQL:
		strFile += _T("P002832E.pdf"); // OpenSQL Reference Guide
		break;
	case ID_IGRESDOC_QUELREF:
		strFile += _T("P002682E.pdf"); // QUEL Reference Guide
		break;
	case ID_IGRESDOC_REP:
		strFile += _T("P002702E.pdf"); // Replication Option User Guide
		break;
	case ID_IGRESDOC_SQLREF:
		strFile += _T("P002672E.pdf"); // SQL Reference Guide
		break;
	case ID_IGRESDOC_STARUG:
		strFile += _T("P002782E.pdf"); // Distributed Option User Guide
		break;
	case ID_IGRESDOC_SYSADM:
		strFile += _T("P002652E.pdf"); // System Administrator Guide
		break;
	case ID_IGRESDOC_QRYTOOLS:
		strFile += _T("P002732E.pdf"); // Character-based Querying and Reporting Tools User Guide
		break;
	case ID_IGRESDOC_UFADT:
		strFile += _T("P002722E.pdf"); // Forms-based Application Development Tools User Guide
		break;
	case ID_IGRESDOC_INGNET:
		strFile += _T("P002642E.pdf"); // Connectivity Guide
		break;
	case ID_IGRESDOC_IPM:
		strFile += _T("P002712E.pdf"); // Interactive Performance Monitor User Guide
		break;
	case ID_IGRESDOC_RELEASE:
		strFile += _T("P002622E.pdf"); // Release Summary
		break;
	case ID_INGRES_CACHE:
		strFile = theApp.m_strIISystem + consttchszIngFiles + _T("ingcache.pmw");
		break;
	case ID_ERROR_LOG:
		strFile = theApp.m_strIISystem + consttchszIngFiles + _T("errlog.log");
		break;
	case ID_README:
		strFile  = theApp.m_strIISystem;
		if (!IVM_IsDBATools(theApp.GetCurrentInstallationID()))
		strFile +=  _T("\\readme.html"); // under MAINWIN, we define NOSHELLICON, then there is no tray's menu !
		else
		strFile +=  _T("\\readme_dba.html"); //DBA Tools readme
		break;
	default:
		strFile = _T("");
		break;
	}

	return strFile;
}

void CvManagerViewLeft::OnAppAbout()
{
	theApp.LaunchAboutBox(IDS_INGRES_ADVANTAGE_TITLE, 0);
}

