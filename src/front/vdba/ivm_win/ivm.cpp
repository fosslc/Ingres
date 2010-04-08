/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : ivm.cpp , Implementation File
**
**  Project  : Ingres II / Visual Manager
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Main application behaviour
**
** History:
** 14-Dec-1998 (uk$so01)
**    Created.
** 27-Jan-2000 (uk$so01)
**    (Bug #100157): Activate Help Topic
** 31-Jan-2000 (noifr01)
**    (SIR 100237) (reenabled the logic where IVM manages the fact
**    that Ingres may be started as a service or not).
**    updated the Ingres service name (now depends on II_INSTALLATION)
** 01-Feb-2000 (uk$so01)
**    (Bug #100279)
**    Add the platform management, no ingres service and external
**    access for WIN 95,98
** 02(Feb-2000 (noifr01)
**    (bug 100309) under 95/98 specifically, increased the frequency
**    for searching if new messages were added in the errlog.log
**    from 5 seconds to 15 seconds (but the frequency is still 200 ms
**    during a start/stop operation). This is because the technique
**    now used under 95/98 because of bug 100309 may be more resource
**    consuming if the files are not cached conveniently by the OS
** 02-Feb-2000 (uk$so01)
**    (Sir #100316)
**    Provide the command line "-stop" to stop ivm from the prompt:
**    ivm -stop.
** 03-Feb-2000 (uk$so01)
**    (BUG #100327)
**    Move the data files of ivm to the directory II_SYSTEM\ingres\files.
** 10-Feb-2000 (somsa01)
**    Updated message sending functions to use the proper frame document
**    handle from m_pMainDoc. Also, initialize m_LSSfileStatus.
** 28-Feb-2000 (uk$so01)
**    Bug #100621
**    Reoganize the way of manipulation of ingres variable.
**    Due to the unexpected behaviour of the release version of app, now the app uses the member 
**    CTypedPtrList<CObList, CaParameter*> m_listEnvirenment instead of static CHARxINTxTYPExMINxMAX* IngresEnvironmentVariable
**    allocated on heap.
**    m_strVariableNotSet set to "" instead of "<not set>".
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 02-Mar-2000 (uk$so01)
**    SIR #100690, If user runs the second Ivm than activate the first one.
** 03-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.(Also connect help !!!)
** 09-Mar-2000 (uk$so01)
**    SIR #100706. Also include the help topic if genven the help id = 0
** 31-Mar-2000 (uk$so01)
**    BUG 101126
**    Put the find dialog at top. When open the modal dialog, 
**    if the find dialog is opened, then close it.
** 26-April-2000 (uk$so01)
**    SIR #101251
**    When starting IVM, if the INGRES SERVICE is currently started pending
**    then IVM will wait (5 mins max) for the SERVICE completly running.
** 03-May-2000 (uk$so01)
**    SIR #101402
**    Start ingres (or ingres component) is in progress ...:
**    If start the Name Server of the whole installation, then just sleep an elapse
**    of time before querying the instance. (15 secs or less if process terminated ealier).
**    Change the accelerate phase of refresh instances from 200ms to 1600ms.
** 06-Jul-2000 (uk$so01)
**    BUG #102020
**    Replace pListComponent allocated and post to the window by
**    m_pListComponent member allocated once and post to the window and
**    clean up when application exits.
** 25-Jul-2000 (noifr01)
**    emergency fix for bug 102155: when refreshing the whole list of messages
**    after having changed the preferences, use (indirectly) SendMessage() rather 
**    than PostMessage() in the loop that re-reads the errlog.log file and sends
**    back one windows message per block of 2500 lines in the errlog.log.
**    This ensures the messages get treated in the right order, which was
**    probably not the case without the fix (the deleted messages were apparently
**    not always the first ones).
** 31-Jul-2000 (uk$so01)
**    (bug 99242) cleaunup for DBCS
** 20-Dec-2000 (noifr01)
**    (SIR 103548) added the installation ID information in the "balloon help"
**    of the IVM "corner icon"
** 02-Jan-2001 (noifr01)
**    (bug 103610) enforce a refresh after initial delay before (fast) refresh
**    of the instances of Ingres components upon a start/stop. This covers
**    the case where the start/stop operation has completed before the end of 
**    this initial delay
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one IVM per installation ID
** 20-Jun-2001 (noifr01)
**    (bug 105065) m_LSSfileStatus and associated management have been removed
**    (m_fileStatus being restored and managed in the CuDlgStatusInstallationTab2
**    class)
** 21-Jun-2001 (noifr01)
**    (bug  105075) don't continue any more if the mutex already exists (other
**    instance of IVM is started), but the corresponding IVM window is not 
**    found
** 28-Jul-2001 (schph01)
**    (BUG # 105359)
**    Change the type of access in function OpenSCManager() by  GENERIC_READ instead of
**    SC_MANAGER_ALL_ACCESS for the WaitIngresService() function .
** 20-Aug-2001 (uk$so01)
**    Fix BUG #105382, Incorrectly notify an alert message when there are no 
**    events displayed in the list.
** 17-Sep-2001 (noifr01)
**    (bug 105758) CreateMutex didn't fail under 98 if it already existed. 
**    only the tests on the windows class and title are kept for such platforms
** 21-Sep-2001 (somsa01)
**    Added call to InitMessageSystem() in InitInstance().
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 01-Nov-2001 (hanje04)
**    wc.lpfnWndProc should be set to the address of AfxWndProc, stop compiler
**    error on Linux.
** 07-Nov-2001 (uk$so01)
**    SIR #106290, new corporate guidelines and standards about box.
**    (the old code of about box and its resource are not cleaned yet because
**     of the ivm.dsp contains the 64bits target and we don't want this change)
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 29-Jan-2003 (noifr01)
**    (SIR 108139) get the version string and year by parsing information 
**    from the gv.h file 
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 10-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**    Update the previous merge on change #462290
** 11-Mar-2003 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu
** 04-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 07-May-2003 (uk$so01)
**    BUG #110195, issue #11179264. Ivm show the alerted messagebox to indicate that
**    there are alterted messages but the are not displayed in the right pane
**    according to the preferences
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 02-Feb-2004 (noifr01)
**    removed any unneeded reference to iostream libraries, including now
**    unused internal test code 
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 22-Mar-2004 (uk$so01)
**    BUG #111996 / ISSUE 13305632
**    If Ivm failed to create Mutex, it should display a single message and abort
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 19-Oct-2004 (uk$so01)
**    SIR #113260 / ISSUE 13730838. Add a command line -NOMAXEVENTSREACHEDWARNING
**    to disable the Max Event reached MessageBox.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 14-Feb-2006 (drivi01)
**    Update the year to 2006.
** 08-Jan-2006 (drivi01)
**    Updated the year to 2007.
** 18-May-2007 (karye01)
**    SIR #118282, added Help menu item for support url.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 05-May-2008 (drivi01)
**    Added routines for ivm in DBA Tools package.
**    When ivm is part of DBA Tools package, tooltips in
**    taskbar above ivm icons will show that it's DBA Tools installation.
** 20-Mar-2009 (smeke01) b121832
**    Product year is returned by INGRESII_BuildVersionInfo() so does 
**    not need to be set in here.
** 21-Mar-2009 (drivi01)
**    Replace 'CArchiveException::generic' with 
**    'CArchiveException::genericException' to clean up warnings
**    in efforts to port to Visual Studio 2008 compiler.
** 16-Dec-2009 (drivi01) b123065
**	  Fix the way the IVM GUI window is activated to get around
**	  the different integirty levels when IVM running with 
**	  elevated privileges can not be activated by the Start
**    menu shortcut. 
**/

#include "stdafx.h"
#include "ivm.h"
#include "mainfrm.h"
#include "maindoc.h"
#include "mainview.h"
#include "ivmdml.h"
#include "xdlgpref.h"
#include "wmusrmsg.h"
#include "readflg.h"
#include "xdgevset.h"
#include "tlsfunct.h"
#include "constdef.h"
#include "ingobdml.h"
#include "copyright.h"

#include "ll.h"
#include <winsvc.h>
#include <locale.h>
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//
// If defined "ALERTxMSGBOXxONLYxNEWxEVENTS", then only the alerted messages that have not
// been alerted that cause to popup a message box and or a beep. Otherwise, when IVM starts (or changes preferences)
// and if there are alerted messages that have not been read the message box will be popuped.
//#define ALERTxMSGBOXxONLYxNEWxEVENTS
//

#ifdef MAINWIN
#define REFRESHWHILESTARTSTOPFREQ 4000
#else
#define REFRESHWHILESTARTSTOPFREQ 1000
#endif /* MAINWIN */

static UINT nIDTipArray [MAX_SHELL_ICON] =
{
	IDS_CORNER_R_NORMAL,
	IDS_CORNER_R_STOPPED,
	IDS_CORNER_R_STARTED,
	IDS_CORNER_R_STARTEDMORE,
	IDS_CORNER_R_HALFSTARTED,
	IDS_CORNER_N_NORMAL,
	IDS_CORNER_N_STOPPED,
	IDS_CORNER_N_STARTED,
	IDS_CORNER_N_STARTEDMORE,
	IDS_CORNER_N_HALFSTARTED
};

static UINT nIDTipArrayDba [MAX_SHELL_ICON] =
{
	IDS_CORNER_R_NORMAL_DBA,
	IDS_CORNER_R_STOPPED_DBA,
	IDS_CORNER_R_STARTED_DBA,
	IDS_CORNER_R_STARTEDMORE_DBA,
	IDS_CORNER_R_HALFSTARTED_DBA,
	IDS_CORNER_N_NORMAL_DBA,
	IDS_CORNER_N_STOPPED_DBA,
	IDS_CORNER_N_STARTED_DBA,
	IDS_CORNER_N_STARTEDMORE_DBA,
	IDS_CORNER_N_HALFSTARTED_DBA
};

typedef struct {
	DWORD cbSize;
	DWORD ExtStatus;
} CHANGEFILTERSTRUCT, *PCHANGEFILTERSTRUCT;
static BOOL WaitIngresService();
static BOOL (FAR WINAPI *pfnChangeWindowMessageFilter)(
		HWND hWnd,
		UINT message,
		DWORD action,
		PCHANGEFILTERSTRUCT pChangeFilterStruct) = NULL;

/////////////////////////////////////////////////////////////////////////////
// CappIvmApplication

BEGIN_MESSAGE_MAP(CappIvmApplication, CWinApp)
	//{{AFX_MSG_MAP(CappIvmApplication)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_SUPPORTONLINE, OnHelpOnlineSupport)
	ON_COMMAND(ID_PREFERENCE, OnPreference)
	ON_COMMAND(ID_MESSAGE_SETTING, OnMessageSetting)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CappIvmApplication construction

CappIvmApplication::CappIvmApplication()
{
	m_bCriticalError = FALSE;
	m_nModalDlg = 0;
	m_bIsCleanedUp = FALSE;
	m_bVisibleAtStartTime = FALSE;
	m_bInitInstanceOK = FALSE;
	m_bShowStartStopAnimate = TRUE;
	m_bAutomaticUnalert = TRUE;
	m_bRequestExit = FALSE;
	m_bRequestComponentStartStop = FALSE;
	m_bRequestReinitializeEvents = FALSE;
	m_bRequestInitializeReadFlag = FALSE;
	m_bConfigureInAction = FALSE;
	m_lLastNameServerID = -1;
	m_hMutexSingleInstance = NULL;

	m_bMarkAllAsUnread_errlogchanged = FALSE;

	m_lfileSize = 0;
	m_nSimulate =
#if defined (_DEBUG) && defined (SIMULATION)
		SIMULATE_COMPONENT|
		SIMULATE_INSTANCE |
		SIMULATE_LOGGEDEVENT|
		SIMULATE_PARAMETER|
		SIMULATE_EVENTCATEGORY|
#endif
		0;

	m_nEllapse  = 5 * 1000;          // 5  seconds
	m_nInstanceTick = 60 * 1000;     // 60 seconds

	m_pThreadComponent = NULL;
	m_pThreadEvent = NULL;
	m_bHelpFileAvailable = TRUE;
	m_bMessageBoxEvent = FALSE;      // No message Box is shown
	m_bMaxEventReachedWarning = TRUE;
	try
	{
		//
		// Do not call any LoadString in the constructor:

		m_strAll = _T("(all)");
		m_strHelpFile = _T("ivm.chm");
		m_strIngresServiceName = _T("Ingres_Database");
		//
		// Ex: 1999-05-06,16:29:32
		m_strTimeFormat = _T("%Y-%m-%d,%H:%M:%S");
		m_strII_INSTALLATION = INGRESII_QueryInstallationID();

		m_strStartNAMExINSTALLATION = _T("EventStartNAMEorINSTALLATION");
		m_strStartNAMExINSTALLATION +=m_strII_INSTALLATION;
		m_nDelayNAMExINSTALLATION   = 10 * 1000; // 10 seconds
		m_nInstanceFastElapse       = 1600;
		//
		// Because of calling the member Serialize of CdMainDoc directly
		// the schema number is not stored to the archive and CArchiveException is not thrown.
		// So, these are constants to be checked.
		// Change these constants if the data serialized in the files:
		// ivmevent.dat or ivmmsgct.dat is changed in the new version !!!
		m_strIVMEVENTSerializeVersionNumber = _T("29-SEP-1999: VER01");
		m_strMSCATUSRSerializeVersionNumber = _T("29-SEP-1999: VER01");

		m_dwPlatformId = VER_PLATFORM_WIN32_NT;
		OSVERSIONINFO vsinfo;
		vsinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (GetVersionEx (&vsinfo))
			m_dwPlatformId = vsinfo.dwPlatformId;
		/* the technique under 95/98 could be more resource consuming (if the files are not cached in the OS)*/
		/* increase the frequency (initially 5 seconds) for getting the errlog.log file size through*/
		/* an independent sopen(). The frequency is still 200 ms during start/stop operations*/
		if (theApp.GetPlatformID() == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
			m_nEllapse  = 15 * 1000;          // 15 seconds
		
		m_strWordSeparator = _T(" ,()[]'\t\n");
		m_strLastFindWhat  = _T("");
		m_pListComponent = new CTypedPtrList<CObList, CaTreeComponentItemData*>;
		m_pMessageDescriptionManager = NULL;
	}
	catch (CMemoryException* e)
	{
		e->Delete();
		IVM_OutOfMemoryMessage();
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CappIvmApplication::CappIvmApplication):\nThe program will be aborted"), MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
		m_bCriticalError = TRUE;
	}
}

CdMainDoc* CappIvmApplication::GetMainDocument()
{
	if (m_pMainDoc)
		return m_pMainDoc;
	CDocument* pDoc;
	POSITION pos, curTemplatePos = GetFirstDocTemplatePosition();
	while(curTemplatePos != NULL)
	{
		CDocTemplate* curTemplate = GetNextDocTemplate (curTemplatePos);
		pos = curTemplate->GetFirstDocPosition ();
		while (pos)
		{
			pDoc = curTemplate->GetNextDoc (pos);
			if(!pDoc->IsKindOf (RUNTIME_CLASS (CdMainDoc)))
				continue;
			m_pMainDoc = (CdMainDoc*)pDoc;
			return m_pMainDoc;
		}
	}
	return NULL;
}


BOOL CappIvmApplication::RefreshComponentIcon()
{
	CdMainDoc* pMainDoc = GetMainDocument();
	ASSERT (pMainDoc);
	if (!pMainDoc)
		return FALSE;

	HWND hLeftView = pMainDoc->GetLeftViewHandle();
	if (IsWindow (hLeftView))
	{
		::SendMessage (hLeftView, WMUSRMSG_UPDATE_COMPONENT_ICON, 0, 0);
		UpdateShellIcon();
	}
	return TRUE;
}



BOOL CappIvmApplication::RefreshComponentData(BOOL bComponentInstance, BOOL bBlocking)
{
	try
	{
		TRACE0 ("CappIvmApplication::RefreshComponentData: Query Components\n");

		//
		// Update the components:
		if (bComponentInstance)
		{
			TRACE0 ("CappIvmApplication::RefreshComponentData: Initialize Instances\n");
			IVM_InitializeInstance();
		}

		if (!m_pListComponent)
			m_pListComponent = new CTypedPtrList<CObList, CaTreeComponentItemData*>;
		if (m_pListComponent)
			while (!m_pListComponent->IsEmpty())
				delete m_pListComponent->RemoveHead();

		if (IVM_llQueryComponent (m_pListComponent, bComponentInstance))
		{
			TRACE1("IVM_llQueryComponent: count = %d\n", m_pListComponent->GetCount());
			if (!m_pMainDoc)
				m_pMainDoc = GetMainDocument();
			if (m_pMainDoc && IsWindow (m_pMainDoc->GetLeftViewHandle()))
			{
				HWND hLeftView = m_pMainDoc->GetLeftViewHandle();
				if (bBlocking)
					::SendMessage (hLeftView, WMUSRMSG_UPDATE_COMPONENT, (WPARAM)bComponentInstance, (LPARAM)m_pListComponent);
				else
					::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_UPDATE_COMPONENT, (WPARAM)bComponentInstance, (LPARAM)m_pListComponent);
			}
			else
			{
				SetEvent (m_hEventComponent);
			}
		}
		else
		{
			SetEvent (m_hEventComponent);
		}

		if (m_pMainDoc)
		    ::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_UPDATE_SHELLICON, (WPARAM)0, (LPARAM)0);
		else
		    ::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_UPDATE_SHELLICON, (WPARAM)0, (LPARAM)0);
	}
	catch (CMemoryException* e)
	{
		if (m_pMainDoc)
		    ::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_IVM_MESSAGEBOX, 0, 0);
		else
		    ::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, 0);
		e->Delete();
		SetEvent (m_hEventComponent);
	}
	catch(...)
	{
		TRACE0 ("System error (CappIvmApplication::RefreshComponentData): \nFail to Refresh Data\n");
		LPTSTR lpszError = new TCHAR [128];
		lstrcpy (lpszError, _T("System error (CappIvmApplication::RefreshComponentData): \nFail to Refresh Data"));
		if (m_pMainDoc)
		    ::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
		else
		    ::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
		SetEvent (m_hEventComponent);
	}
	return TRUE;
}


static void NotifyAndReset4ChangedLogFile()
{
	IVM_PostModelessMessageBox(IDS_W_ERRLOG_CHGD_EXTERNALLY);
	theApp.SetRequestInitializeEvents(TRUE);
	theApp.SetRequestInitializeReadFlag(TRUE);
	return;
}

BOOL CappIvmApplication::RefreshEventData(BOOL bBlocking)
{
	BOOL bMore = TRUE;
	LONG lFirstEvent = -1;
	LONG lLastEvent  = -1;
	LONG lIndex = 0;
	CTypedPtrList<CObList, CaLoggedEvent*>* plistEvent = NULL;
	CaQueryEventInformation* pQueryEventInfo = NULL;

	try
	{
		int iEvtsStatus =  IVM_llGetNewEventsStatus();
		switch ( iEvtsStatus ) {
			case RET_LOGFILE_CHANGED_BY_HAND:	
				NotifyAndReset4ChangedLogFile();
				return FALSE;
				break;
			case RET_NO_NEW_EVENT:
				{
					TRACE0 ("CappIvmApplication::RefreshEventData: No new events ...\n");
					int nType = theApp.m_setting.GetEventMaxType();
					if (nType == EVMAX_SINCEDAY || nType == EVMAX_SINCENAME)
						RefreshCacheEvents();
					return TRUE;
				}
				break;
			default:
				break;
		}
		//
		// There are new events:
		TRACE0 ("CappIvmApplication::RefreshEventData: There are new events ...\n");
		BOOL bFirst = TRUE;
		while (bMore)
		{
			plistEvent = new CTypedPtrList<CObList, CaLoggedEvent*>;
			pQueryEventInfo = new CaQueryEventInformation();
			if (bFirst)
				pQueryEventInfo->SetBlockSequence(0);
			else
				pQueryEventInfo->SetBlockSequence(1);
			bFirst = FALSE;

			int iQueryEvtsResult = IVM_llQueryLoggedEvent (plistEvent, pQueryEventInfo);
			switch (iQueryEvtsResult) {
				case RET_LOGFILE_CHANGED_BY_HAND:
					NotifyAndReset4ChangedLogFile();
					delete plistEvent;
					delete pQueryEventInfo;
					return FALSE;
				case RET_ERROR:
					ASSERT (FALSE);
					delete plistEvent;
					delete pQueryEventInfo;
					return FALSE;
				default:
					break;
			}

			lFirstEvent = pQueryEventInfo->GetFirstEventId();
			lLastEvent  = pQueryEventInfo->GetLastEventId();

			//
			// Update the read/unread flags of events:
			if (!plistEvent->IsEmpty())
			{
				long lEvID = -1;
				LONG lId = -1;
				CaLoggedEvent* pEv = NULL;
				POSITION pos = plistEvent->GetHeadPosition();
				while (pos != NULL)
				{
					pEv = plistEvent->GetNext (pos);
					lId = pEv->GetIdentifier();
					lEvID = IVM_NewEvent (lId, FALSE);
					if (lEvID == -1) // Old event (ie it has the read/unread flag)
					{
						pEv->SetRead (IVM_IsReadEvent(lId));
#if defined (ALERTxMSGBOXxONLYxNEWxEVENTS)
						pEv->AltertNotify(TRUE);
#endif
					}
					else
					{
						pEv->AltertNotify(FALSE); // a new event, so consider it as already notified.
					}
				}
				ASSERT (lLastEvent != -1);
				if (lLastEvent != -1)
					theApp.SetLastReadEventId (lLastEvent);
				
				if (!m_pMainDoc)
					m_pMainDoc = GetMainDocument();
				if (m_pMainDoc && IsWindow (m_pMainDoc->GetRightViewHandle()))
				{
					if (bBlocking)
						::SendMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_UPDATE_EVENT, (WPARAM)pQueryEventInfo, (LPARAM)plistEvent);
					else
						::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_UPDATE_EVENT, (WPARAM)pQueryEventInfo, (LPARAM)plistEvent);
				}
			}
			else
			{
				if (plistEvent) delete plistEvent;
				if (pQueryEventInfo) delete pQueryEventInfo;
				plistEvent = NULL;
				pQueryEventInfo = NULL;
			}
			
			//
			// Are there more events ?
			if  (theApp.m_nSimulate & SIMULATE_LOGGEDEVENT)
				bMore = FALSE;
			else {
				int iEvtsStatus =  IVM_llGetNewEventsStatus();
				switch ( iEvtsStatus ) {
					case RET_LOGFILE_CHANGED_BY_HAND:	
						NotifyAndReset4ChangedLogFile();
						if (plistEvent) delete plistEvent;
						if (pQueryEventInfo) delete pQueryEventInfo;
						plistEvent = NULL;
						pQueryEventInfo = NULL;
						return FALSE;
						break;
					case RET_NO_NEW_EVENT:
						bMore = FALSE;
						break;
					default:
						bMore = TRUE;
						break;
				}
			}
		}
		
		BOOL bReinit = theApp.IsRequestInitializeEvents();
		if (bReinit)
			SetEvent (theApp.GetEventReinitializeEvent());
		//
		// Indicate the Mainframe that the Thread has finished reading events:
		if (!bFirst)
		{
			if (m_pMainDoc && IsWindow (m_pMainDoc->GetRightViewHandle()))
			{
				pQueryEventInfo = new CaQueryEventInformation();
				pQueryEventInfo->SetBlockSequence(-1);
				if (bBlocking)
					::SendMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_UPDATE_EVENT, (WPARAM)pQueryEventInfo, (LPARAM)0);
				else
					::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_UPDATE_EVENT, (WPARAM)pQueryEventInfo, (LPARAM)0);
			}
		}
	}
	catch (CMemoryException* e)
	{
		if (m_pMainDoc)
		    ::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_IVM_MESSAGEBOX, 0, 0);
		else
		    ::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, 0);
		e->Delete();
	}
	catch(...)
	{
		TRACE0 ("CappIvmApplication::RefreshEventData: Fail to Refresh Data\n");
		LPTSTR lpszError = new TCHAR [128];
		lstrcpy (lpszError, _T("System error (CappIvmApplication::RefreshEventData): \nFail to Refresh Data"));
		if (m_pMainDoc)
		    ::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
		else
		    ::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
	}
	return TRUE;
}

//
// Call periodically only the MaxEventType is
// EVMAX_SINCEDAY or EVMAX_SINCENAME
// And there is no events to query. This function will remove
// the events out of the cache if thier dates are not matched the condition
// specified in the settings:
void CappIvmApplication::RefreshCacheEvents()
{
	if (!m_pMainDoc)
		m_pMainDoc = GetMainDocument();
	if (m_pMainDoc && IsWindow (m_pMainDoc->GetRightViewHandle()))
	{
		::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_UPDATE_EVENT, (WPARAM)0, (LPARAM)0);
	}
}



UINT CappIvmApplication::ThreadProcControlComponents (LPVOID pParam)
{
	UINT nQueryInstance = 0;
	UINT nStep = 200;
	UINT nPitStop  = 0;
	BOOL bStartStop = FALSE;
	BOOL bOneMore = FALSE;
	BOOL bDelayTime = FALSE;

	while (1)
	{
		Sleep (nStep);
		if (theApp.IsRequestExited())
			break;
		nPitStop+= nStep;
		bStartStop = theApp.IsComponentStartStop();
		
		//
		// Start / Stop ingres (or ingres component) is in progress ...:
		// If start the Name Server of the whole installation, then just sleep an elapse
		// of time before querying the instance.
		if (bStartStop)
		{
			if (!bDelayTime)
			{
				HANDLE hEvent = OpenEvent (EVENT_ALL_ACCESS, TRUE, theApp.m_strStartNAMExINSTALLATION);
				if (hEvent)
				{
					TRACE0 ("Delay for IINAMU ...\n");
					WaitForSingleObject (hEvent, theApp.m_nDelayNAMExINSTALLATION);
					bDelayTime = TRUE;
					/* enforce refresh immediatly after initial delay for   */
					/* refresh. Fixes the case where the start/stop         */
					/* operation already terminated after the initial delay */
					nPitStop = m_nInstanceFastElapse;
				}
				else
				{
					bDelayTime = FALSE;
				}
			}
		}
		else
			bDelayTime = FALSE;


		if (nPitStop >= m_nEllapse || (bStartStop && (nPitStop >= m_nInstanceFastElapse)) || m_bConfigureInAction)
		{
			if (bStartStop)
				bOneMore = TRUE;
			nPitStop = 0;
			nQueryInstance+= m_nEllapse;
			WaitForSingleObject (m_hEventComponent, 10*60*1000);
			ResetEvent (m_hEventComponent);
			RefreshComponentData(((nQueryInstance >= m_nInstanceTick) || bStartStop || m_bConfigureInAction)? TRUE: FALSE);
			if (nQueryInstance >= m_nInstanceTick)
				nQueryInstance = 0;

			if (m_bConfigureInAction)
				m_bConfigureInAction = FALSE;
			if (!bStartStop && bOneMore)
			{
				RefreshComponentData(TRUE);
				bOneMore = FALSE;
			}

		}
	}
	return 0;
}

static DWORD IFThreadControlComponents(LPVOID pParam)
{
	CappIvmApplication* pApp = (CappIvmApplication*)pParam;
	return pApp->ThreadProcControlComponents((LPVOID)NULL);
}

UINT CappIvmApplication::ThreadProcControlEvents (LPVOID pParam)
{
	BOOL NormalProcess = TRUE;
	BOOL bStartStop = FALSE;
	UINT nEllapse = m_nEllapse;
	UINT nStep = 200;
	UINT nPitStop  = 0;
	while (1)
	{
		Sleep (nStep);
		if (theApp.IsRequestExited())
			break;
		
		BOOL bReinit = theApp.IsRequestInitializeEvents();
		bStartStop = theApp.IsComponentStartStop();
		nEllapse = bStartStop? REFRESHWHILESTARTSTOPFREQ: m_nEllapse;
		nPitStop+= nStep;

		if (nPitStop >= nEllapse || bReinit)
		{
			WaitForSingleObject (theApp.GetEventMaxLimitReached(), INFINITE);

			//
			// If the Events is being reinitialized (setting changed)
			// then block the update:
			CaEventDataMutex mutextEvent(INFINITE);
			TRACE0 ("CappIvmApplication::ThreadProcControlEvents()\n");
			nPitStop = 0;
			if (bReinit)
			{
				TRACE0 ("CappIvmApplication::ThreadProcControlEvents: Re-Initialize logged events !!!\n");
				IVM_ll_RestartLFScanFromStart();
				ResetReadEventId(); // Simulation used only
				if (!m_pMainDoc)
					m_pMainDoc = GetMainDocument();
				if (m_pMainDoc && IsWindow (m_pMainDoc->GetRightViewHandle()))
				{
					ResetEvent (theApp.GetEventReinitializeEvent());
					::PostMessage (m_pMainDoc->m_hwndMainFrame, WMUSRMSG_REINITIALIZE_EVENT, (WPARAM)0, (LPARAM)0);
					WaitForSingleObject (theApp.GetEventReinitializeEvent(), INFINITE);
				}
			}

			if (bReinit)
			{
				ResetEvent (theApp.GetEventReinitializeEvent());
				if (theApp.IsRequestInitializeReadFlag() && m_pMainDoc)
				{
					CaReadFlag* pRFlag = theApp.GetReadFlag();
					pRFlag->Cleanup();
					pRFlag->Init();
					m_pMainDoc->StoreEvents();
					theApp.SetRequestInitializeReadFlag(FALSE);
				}
			}

			NormalProcess = RefreshEventData(bReinit); // 25-07-2000 (noifr01) (bug 102155) default (in ivm.h) is FALSE (when not specified). now match to bReinit
			                                           // and ensure SendMessage() is used upon changes in the preferences
			if (bReinit && NormalProcess)
			{
				WaitForSingleObject (theApp.GetEventReinitializeEvent(), INFINITE);
				theApp.SetRequestInitializeEvents(FALSE);

				if (m_pMainDoc && IsWindow (m_pMainDoc->GetRightViewHandle()))
				{
					ResetEvent (theApp.GetEventReinitializeEvent());
					::PostMessage (m_pMainDoc->GetRightViewHandle(), WMUSRMSG_REACTIVATE_SELECTION, (WPARAM)0, (LPARAM)0);
					WaitForSingleObject (theApp.GetEventReinitializeEvent(), 15*60*1000);
				}
			}

			if (m_setting.m_nEventMaxType == EVMAX_SINCENAME && theApp.GetLastNameStartedup() != -1)
			{
				long lLast = theApp.GetLastNameStartedup();
				theApp.SetLastNameStartedup(-1);
				TRACE1 ("Last Name Server started up: %d\n", lLast);
				::PostMessage (m_pMainDoc->GetRightViewHandle(), WMUSRMSG_NAMESERVER_STARTED, (WPARAM)0, (LPARAM)lLast);
			}
		}
	}
	return 0;
}

static DWORD IFThreadControlEvents(LPVOID pParam)
{
	CappIvmApplication* pApp = (CappIvmApplication*)pParam;
	return pApp->ThreadProcControlEvents((LPVOID)NULL);
}




/////////////////////////////////////////////////////////////////////////////
// The one and only CappIvmApplication object

CappIvmApplication theApp;

/////////////////////////////////////////////////////////////////////////////
// CappIvmApplication initialization

BOOL CappIvmApplication::InitInstance()
{
	CString strMsg;
	m_bInitInstanceOK = TRUE;
	_tsetlocale(LC_TIME, _T(""));

	InitMessageSystem();
	try
	{
		CString strTitle;
		CString strWndName;
		//
		// _T("Cannot initialize Ingres Visual Manager data !\nThe application will terminate.");
		strMsg.LoadString (IDS_MSG_FAIL_INITIALIZE_DATA);
		if (m_bCriticalError)
			return FALSE;

		WNDCLASS wc;
		memset (&wc, 0, sizeof(wc));
		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpszClassName = _T("IVM");
		wc.lpfnWndProc = &AfxWndProc;
		wc.hInstance = AfxGetInstanceHandle();
		wc.hIcon = LoadIcon(IDR_MAINFRAME);
		wc.hCursor = LoadStandardCursor(IDC_ARROW);
		wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
		wc.lpszMenuName = NULL;

		if (!AfxRegisterClass (&wc))
			return FALSE;

		CString strCmdLine = m_lpCmdLine;
		strCmdLine.TrimLeft();
		strCmdLine.TrimRight();

		CString strNamedMutex = _T("INGRESxIVMxSINGLExINSTANCE");
		strNamedMutex +=  GetCurrentInstallationID();
		strTitle.LoadString (AFX_IDS_APP_TITLE);
		strTitle += GetCurrentInstallationID();

		BOOL bCreateMutex = TRUE;
#ifndef MAINWIN
		if (theApp.GetPlatformID() == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
			bCreateMutex = FALSE;
#endif
		DWORD dwError = 0;
		if (bCreateMutex)
		{
			m_hMutexSingleInstance = CreateMutex (NULL, NULL, strNamedMutex);
			dwError = GetLastError();
		}
		if (strCmdLine.CompareNoCase (_T("-NOMAXEVENTSREACHEDWARNING")) == 0 || strCmdLine.CompareNoCase (_T("/NOMAXEVENTSREACHEDWARNING")) == 0)
			m_bMaxEventReachedWarning = FALSE;
		/* (initialized to 0 in the constructor) - covers the case where bCreateMutex ==FALSE */
		if (!bCreateMutex || dwError == ERROR_ALREADY_EXISTS)
		{
			//
			// An instance of IVM already exists:
			strWndName.LoadString (AFX_IDS_APP_TITLE);
			CWnd* pWndPrev = CWnd::FindWindow(_T("IVM"), strTitle);
			if (pWndPrev)
			{
				if (strCmdLine.CompareNoCase (_T("-stop")) == 0 || strCmdLine.CompareNoCase (_T("/stop")) == 0)
				{
					//
					// Only exit ivm (not performe any ingres shutdown !!!)
					pWndPrev->SendMessage (WMUSRMSG_APP_SHUTDOWN, 0, 0);
					return FALSE;
				}
				//_T("An instance of Ingres Visual Manager is already running.");
				// Activate the previous application:
				::SendMessage (pWndPrev->m_hWnd, WMUSRMSG_ACTIVATE, 0, 0);
				::SetForegroundWindow (pWndPrev->m_hWnd);
				return FALSE;
			}
			if (bCreateMutex)
				return FALSE;
		}
		if (dwError != 0 && dwError != ERROR_ALREADY_EXISTS)
		{
			AfxMessageBox (IDS_MSG_CANNOT_START_IVM);
			return FALSE;
		}

		if (strCmdLine.CompareNoCase (_T("-stop")) == 0 || strCmdLine.CompareNoCase (_T("/stop")) == 0)
			return FALSE;

		CString strMessageFile;
		TCHAR* pEnv;
		pEnv = _tgetenv(_T("II_SYSTEM"));
		if (!pEnv)
		{
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg);
			return FALSE;
		}
		
		m_strIISystem = pEnv;
		m_strLoggedEventFile     = m_strIISystem + consttchszIngFiles + _T("errlog.log");
		m_strLoggedStartStop     = m_strIISystem + consttchszIngFiles + _T("ingstart.log");
		m_strLoggedArchiver      = m_strIISystem + consttchszIngFiles + _T("iiacp.log");
		m_strLoggedRecovery      = m_strIISystem + consttchszIngFiles + _T("iircp.log");
		m_strMessageSettingsFile = m_strIISystem + consttchszIngFiles + _T("ivmmsgct.dat");
		m_strArchiveEventFile    = m_strIISystem + consttchszIngFiles + _T("ivmevent.dat");
		m_strIngresVdba          = m_strIISystem + consttchszIngVdba;
		m_strIngresBin           = m_strIISystem + consttchszIngBin;

		#if defined (MAINWIN)
		strMessageFile = _T("english/messages/messages.txt");
		#else
		strMessageFile = _T("english\\messages\\messages.txt");
		#endif

		CString strFile = m_strIISystem + consttchszIngFiles + strMessageFile;
		m_pMessageDescriptionManager = new CaMessageDescriptionManager(strFile);
		m_pMessageDescriptionManager->Initialize();

		if (m_nSimulate & SIMULATE_LOGGEDEVENT)
			m_strLoggedEventFile = _T("logevent.dat");

		AfxEnableControlContainer();
		m_TrayImageList.Create (IDB_TRAYIMAGE, 32, 0, RGB(255,0,255));

		// Standard initialization
		// (No need for .NET) Enable3dControls(); // Call this when using MFC in a shared DLL

		strNamedMutex = _T("m_hMutexTreeData");
		strNamedMutex+= GetCurrentInstallationID();
		m_hMutexTreeData = CreateMutex (NULL, FALSE, strNamedMutex);
		if (m_hMutexTreeData == NULL)
		{
			AfxMessageBox (_T("System error(m_hMutexTreeData): Cannot create mutex."));
			m_bCriticalError = TRUE;
		}

		strNamedMutex = _T("m_hMutexEventData");
		strNamedMutex+= GetCurrentInstallationID();
		m_hMutexEventData = CreateMutex (NULL, FALSE, strNamedMutex);
		if (m_hMutexEventData == NULL)
		{
			AfxMessageBox (_T("System error(m_hMutexEventData): Cannot create mutex."));
			m_bCriticalError = TRUE;
		}

		strNamedMutex = _T("m_hMutexInstanceData");
		strNamedMutex+= GetCurrentInstallationID();
		m_hMutexInstanceData = CreateMutex (NULL, FALSE, strNamedMutex);
		if (m_hMutexInstanceData == NULL)
		{
			AfxMessageBox (_T("System error(m_hMutexInstanceData): Cannot create mutex."));
			m_bCriticalError = TRUE;
		}

		strNamedMutex = _T("E_ReinitializeEvent");
		strNamedMutex+= GetCurrentInstallationID();
		m_hEventReinitializeEvent = CreateEvent (NULL, TRUE, TRUE, strNamedMutex);
		if (m_hEventReinitializeEvent == NULL)
		{
			AfxMessageBox (_T("System error(m_hEventReinitializeEvent): Cannot create event."));
			m_bCriticalError = TRUE;
		}

		strNamedMutex = _T("E_EventComponent");
		strNamedMutex+= GetCurrentInstallationID();
		m_hEventComponent = CreateEvent(NULL, TRUE, TRUE, strNamedMutex);
		if (m_hEventComponent == NULL)
		{
			AfxMessageBox (_T("System error(m_hEventComponent): Cannot create event."));
			m_bCriticalError = TRUE;
		}

		strNamedMutex = _T("E_EventStartStop");
		strNamedMutex+= GetCurrentInstallationID();
		m_hEventStartStop = CreateEvent(NULL, TRUE, TRUE, strNamedMutex);
		if (m_hEventStartStop == NULL)
		{
			AfxMessageBox (_T("System error(m_hEventStartStop): Cannot create event."));
			m_bCriticalError = TRUE;
		}

		strNamedMutex = _T("E_EventMaxEventReached");
		strNamedMutex+= GetCurrentInstallationID();
		m_hEventMaxEventReached = CreateEvent(NULL, TRUE, TRUE, strNamedMutex);
		if (m_hEventMaxEventReached == NULL)
		{
			AfxMessageBox (_T("System error(m_hEventMaxEventReached): Cannot create event."));
			m_bCriticalError = TRUE;
		}

		m_pMainDoc = NULL;
		m_pReadFlag = new CaReadFlag();
		m_lLastReadEventId = -1;

		CTime t = CTime::GetCurrentTime();
		m_strStartupTime = t.Format (m_strTimeFormat);

		m_hCursorDropCategory  =LoadCursor (IDC_DROPCATEGORY);
		m_hCursorNoDropCategory=LoadCursor (IDC_NODROPCATEGORY);

		TCHAR tchszCurDir[MAX_PATH];
		DWORD dwLen =  GetCurrentDirectory(MAX_PATH, tchszCurDir);
		m_strCurrentPath = tchszCurDir;

		// Change the registry key under which our settings are stored.
		// You should modify this string to be something appropriate
		// such as the name of your company or organization.
		//SetRegistryKey(_T("Local AppWizard-Generated Applications"));

		LoadStdProfileSettings();  // Load standard INI file options (including MRU)
		if (!m_strVariableNotSet.LoadString (IDS_VARIABLE_NOTSET))
			m_strVariableNotSet = _T("<not set>");
		CString strNoDescription, strNotSupport4Unix;
		strNoDescription.LoadString (IDS_NOVARAIABLE_DESCRIPTION);
		strNotSupport4Unix.LoadString (IDS_UNIX_OPERATION_NOT_SUPPORTED);

		//
		// Initialize GLOBAL DATA:

		if (!IVM_llinit())
		{
			AfxMessageBox (strMsg,  MB_ICONSTOP | MB_OK);
			return FALSE;
		}
		CString strII = INGRESII_QueryInstallationID(FALSE);
		if (strII.IsEmpty())
			strII = _T("II");
		m_strIngresServiceName.Format(_T("Ingres_Database_%s"), (LPCTSTR)strII);
		if (!IVM_IsDBATools(strII))
		{
		for (int i=0; i<MAX_SHELL_ICON; i++) /* to be called after IVM_llinit() */
			m_strShellTip[i].Format(nIDTipArray[i], (LPCTSTR)strII);
		}
		else
		{
		for (int i=0; i<MAX_SHELL_ICON; i++) /* to be called after IVM_llinit() */
			m_strShellTip[i].Format(nIDTipArrayDba[i], (LPCTSTR)strII);
		}
		//
		// This function should be called after IVM_llinit has been called.
		m_ingresVariable.SetStringUnset(m_strVariableNotSet);
		m_ingresVariable.SetStringNoDescription(strNoDescription);
		m_ingresVariable.SetStringNotSupport4Unix(strNotSupport4Unix);
		IVM_InitializeIngresVariable();

		//
		// Check to see if INGRES SERVICE started pending, if so
		// wait until it has finished starting:
		WaitIngresService();


		// Register the application's document templates.  Document templates
		//  serve as the connection between documents, frame windows and views.

		CSingleDocTemplate* pDocTemplate;
		pDocTemplate = new CSingleDocTemplate(
			IDR_MAINFRAME,
			RUNTIME_CLASS(CdMainDoc),
			RUNTIME_CLASS(CfMainFrame),       // main SDI frame window
			RUNTIME_CLASS(CvMainView));
		AddDocTemplate(pDocTemplate);

		//
		// Parse command line for standard shell commands, DDE, file open
		CCommandLineInfo cmdInfo;
		ParseCommandLine(cmdInfo);
		//
		// Dispatch commands specified on the command line
		if (!ProcessShellCommand(cmdInfo))
			return FALSE;

		HICON hIcon = m_TrayImageList.ExtractIcon (IM_CORNER_R_NORMAL);
		IVM_ShellNotification (NIM_ADD, 1, hIcon, m_strShellTip[IM_CORNER_R_NORMAL]);

		TRACE0 ("****CappIvmApplication::InitInstance, Begin init component/event\n");
		RefreshComponentData(FALSE, TRUE);
		RefreshEventData(TRUE);
		RefreshComponentData(TRUE, TRUE);

		RefreshComponentIcon();
		TRACE0 ("****CappIvmApplication::InitInstance, End init component/event\n");
		//
		// The one and only window has been initialized, so show and update it.
#if defined (NOSHELLICON)
		m_pMainWnd->ShowWindow(SW_SHOW);
#else
		m_pMainWnd->ShowWindow(m_bVisibleAtStartTime? SW_SHOW: SW_HIDE);
#endif
		m_pMainWnd->UpdateWindow();

		//
		// Add the first title here, otherwise the main window will not have the title
		// because it remains invisible since the first launch.
		// The main view 'CvMainView' will draw up the title when the window becomes visible to
		// overwrite the default title from the Document. (MFC Architecture Frame, View, Doc)
		m_pMainWnd->SetWindowText((LPCTSTR)strTitle);

		/* This code is added here for Windows 7 and Vista for bug #123065.
		** The elevated IVM GUI can not be activated by a shortcut which runs
		** at stripped user level due to User Interface Process Isolation (UIPI).
		** The ChangeWidowMessageFilterEx call tells the high integrity process
		** that WMUSRMSG_ACTIVATE is an allowed message from lower integrity process.
		*/
		int dw = FALSE;
		HMODULE hDll;

		if ((hDll = LoadLibrary( TEXT("user32.dll") )) != NULL)
		{
			pfnChangeWindowMessageFilter = (BOOL (FAR WINAPI *)(
				HWND hWnd,
				UINT message,
    			DWORD action,
				PCHANGEFILTERSTRUCT pChangeFilterStruct))
			GetProcAddress(hDll, TEXT("ChangeWindowMessageFilterEx"));
		}

		if (pfnChangeWindowMessageFilter != NULL)
		{
			dw = ((*pfnChangeWindowMessageFilter)(m_pMainWnd->m_hWnd, WMUSRMSG_ACTIVATE, 1, NULL));
			//if the message is successfully added to "allowed list" then dw will be equal to TRUE.
		}

#if defined (NOSHELLICON)
		CdMainDoc* pDoc = GetMainDocument();
		if (pDoc && IsWindow(pDoc->GetLeftViewHandle()))
		{
			CString strInstallation = m_treeData.GetInstallationDisplayName();
			::SendMessage (pDoc->GetLeftViewHandle(), WMUSRMSG_REACTIVATE_SELECTION, (WPARAM)(LPCTSTR)strInstallation, 0);
		}
#endif

		m_pThreadComponent = AfxBeginThread((AFX_THREADPROC)IFThreadControlComponents, this, THREAD_PRIORITY_BELOW_NORMAL);
		m_pThreadEvent     = AfxBeginThread((AFX_THREADPROC)IFThreadControlEvents, this, THREAD_PRIORITY_BELOW_NORMAL);
	}
	catch (CArchiveException* e)
	{
		IVM_ArchiveExceptionMessage (e);
		e->Delete();
		return FALSE;
	}
	catch (CMemoryException* e)
	{
		e->Delete();
		IVM_OutOfMemoryMessage();
	}
	catch (...)
	{
		AfxMessageBox (strMsg,  MB_ICONSTOP | MB_OK);
		return FALSE;
	}
	return TRUE;
}

void CappIvmApplication::LaunchAboutBox(UINT nIDSTitle, UINT nIDBitmap)
{
	BOOL bOK = TRUE;
	CString strLibraryName = _T("tksplash.dll");
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strLibraryName  = _T("libtksplash.sl");
	#else
	strLibraryName  = _T("libtksplash.so");
	#endif
#endif
	HINSTANCE hLib = LoadLibrary(strLibraryName);
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		bOK = FALSE;
	if (bOK)
	{
		void (PASCAL *lpDllEntryPoint)(LPCTSTR, LPCTSTR, short, UINT);
		(FARPROC&)lpDllEntryPoint = GetProcAddress (hLib, "AboutBox");
		if (lpDllEntryPoint == NULL)
			bOK = FALSE;
		else
		{
			CString strTitle;
			CString strAbout;
			int year;
			CString strVer;
			// 0x00000002 : Show Copyright
			// 0x00000004 : Show End-User License Aggreement
			// 0x00000008 : Show the WARNING law
			// 0x00000010 : Show the Third Party Notices Button
			// 0x00000020 : Show System Info Button
			// 0x00000040 : Show Tech Support Button
			UINT nMask = 0x00000002|0x00000008;
			INGRESII_BuildVersionInfo (strVer, year);

			strAbout.Format (IDS_PRODUCT_VERSION, strVer);
			strTitle.LoadString (nIDSTitle);
			(*lpDllEntryPoint)(strTitle, strAbout, (short)year, nMask);
		}
		FreeLibrary(hLib);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_FAIL_2_LOCATEDLL, (LPCTSTR)strLibraryName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
}


// Menu command to open support site in default browser
void CappIvmApplication::OnHelpOnlineSupport()
{
	ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
}


// App command to run the dialog
void CappIvmApplication::OnAppAbout()
{
	LaunchAboutBox(AFX_IDS_APP_TITLE, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CappIvmApplication commands

void CappIvmApplication::CleanUp()
{
	if (!m_bInitInstanceOK)
		return;
	if (m_bIsCleanedUp)
		return;

	if (m_pThreadComponent && m_pThreadComponent->m_hThread)
	{
		TerminateThread (m_pThreadComponent->m_hThread, 0);
		delete m_pThreadComponent;
		m_pThreadComponent = NULL;
	}
	if (m_pThreadEvent && m_pThreadEvent->m_hThread)
	{
		TerminateThread (m_pThreadEvent->m_hThread, 0);
		delete m_pThreadEvent;
		m_pThreadEvent = NULL;
	}

	IVM_llterminate();
	m_treeData.CleanUp();
	m_loggedEvent.CleanUp();

	m_ParseSpecialInstancesInfo.CleanUp();

	m_bIsCleanedUp = TRUE;
	CloseHandle (m_hMutexTreeData);
	CloseHandle (m_hMutexEventData);
	CloseHandle (m_hMutexInstanceData);
	CloseHandle (m_hEventReinitializeEvent);
	CloseHandle (m_hEventComponent);
	
	m_hMutexTreeData = NULL;
	m_hMutexEventData = NULL;
	m_hMutexInstanceData = NULL;
	m_hEventReinitializeEvent = NULL;
	m_hEventComponent = NULL;
	if (m_hMutexSingleInstance)
		CloseHandle(m_hMutexSingleInstance);
	if (m_pReadFlag)
		delete m_pReadFlag;
	m_pReadFlag = NULL;
	if (m_pListComponent)
	{
		while (!m_pListComponent->IsEmpty())
			delete m_pListComponent->RemoveHead();
		delete m_pListComponent;
	}
	if (m_pMessageDescriptionManager)
		delete m_pMessageDescriptionManager;
}


int CappIvmApplication::ExitInstance() 
{
	if (!m_bInitInstanceOK)
		return CWinApp::ExitInstance();
	CleanUp();
	return CWinApp::ExitInstance();
}



static TCHAR tchszOutOfMemoryMessage [] =
	_T("Low of Memory...\nCannot allocate memory, please close some applications !");
void IVM_OutOfMemoryMessage()
{
	AfxMessageBox (tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
}

void IVM_ArchiveExceptionMessage (CArchiveException* e)
{
	TCHAR tchszMsg [128];
	tchszMsg[0] = _T('\0');
	switch (e->m_cause)
	{
	case CArchiveException::none:
		break;
	case CArchiveException::genericException:
		//"Serialization error:\nCause: Unknown."
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Unknown."));
		break;
	case CArchiveException::readOnly:
		//"Serialization error:\nCause: Tried to write into an archive opened for loading."
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Tried to write into an archive opened for loading."));
		break;
	case CArchiveException::endOfFile:
		//"Serialization error:\nCause: Reached end of file while reading an object."
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Reached end of file while reading an object."));
		break;
	case CArchiveException::writeOnly:
		//"Serialization error:\nCause: Tried to read from an archive opened for storing."
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Tried to read from an archive opened for storing."));
		break;
	case CArchiveException::badIndex:
		//"Serialization error:\nCause: Invalid file format."
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Invalid file format."));
		break;
	case CArchiveException::badClass:
		//"Serialization error:\nCause: Tried to read an object into an object of the wrong type."
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Tried to read an object into an object of the wrong type."));
		break;
	case CArchiveException::badSchema:
		//"Serialization error:\nCause: Tried to read an object with a different version of the class."
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Tried to read an object with a different version of the class."));
		break;
	default:
		lstrcpy (tchszMsg, _T("Serialization error:\nCause: Unknown."));
		break;
	}
	if (tchszMsg[0])
		AfxMessageBox (tchszMsg, MB_ICONEXCLAMATION|MB_OK);
}

BOOL IVM_ShellNotification (DWORD dwMessage, UINT uID, HICON hIcon, LPCTSTR lpszTip)
{
	BOOL  res = TRUE;
#if defined (NOSHELLICON)
	return res;
#else
	CWnd* pWnd = AfxGetMainWnd();

	NOTIFYICONDATA tnd;
	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd   = pWnd? pWnd->m_hWnd: NULL;
	tnd.uID    = uID;

	tnd.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
	tnd.uCallbackMessage = WMUSRMSG_SHELL_NOTIFY_ICON;
	tnd.hIcon  = hIcon;
	if (lpszTip)
	{
		if (_tcslen (lpszTip) >= 64)
			_tcsncpy(tnd.szTip, lpszTip, 63);
		else
			_tcscpy(tnd.szTip, lpszTip);
	}
	else
	{
		tnd.szTip[0] = _T('\0');
	}

	res = Shell_NotifyIcon(dwMessage, &tnd);
#endif
	return res;
}

static BOOL IsReinitializeEventRequired(CaGeneralSetting& oldSetting, CaGeneralSetting& newSetting)
{
	if (oldSetting.m_nEventMaxType != newSetting.m_nEventMaxType)
		return TRUE;
	if (oldSetting.m_nMaxEvent != newSetting.m_nMaxEvent)
		return TRUE;
	if (oldSetting.m_nMaxMemUnit != newSetting.m_nMaxMemUnit)
		return TRUE;
	if (oldSetting.m_nMaxDay != newSetting.m_nMaxDay)
		return TRUE;

	return FALSE;
}


void CappIvmApplication::OnPreference() 
{
	CaModalUp modalUp;
	CxDlgPropertySheetPreference propSheet;
	propSheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;  // Remove Apply Now Button
	propSheet.m_Page2.UpdateSetting (FALSE);
	CaGeneralSetting oldSetting = theApp.m_setting;
	
	if (propSheet.DoModal() == IDOK)
	{
		theApp.m_setting.Save();
		//
		// Check if sth has been changed and take the appropriate actions:
		BOOL bChanged = (oldSetting == theApp.m_setting)? FALSE: TRUE;
		if (bChanged)
		{
			if (IsReinitializeEventRequired(oldSetting, theApp.m_setting))
			{
				//
				// If the Events is being updated (Worker thread)
				// then block the reinitialize process:
				CaEventDataMutex mutextEvent(INFINITE);
				theApp.SetRequestInitializeEvents (TRUE);
			}
		}
	}
}

void CappIvmApplication::UpdateShellIcon()
{
#if defined (NOSHELLICON)
	return;
#endif
	int  nIconID = IM_CORNER_R_NORMAL;
	UINT nState = m_treeData.GetComponentState();
	BOOL bAlert  = (m_treeData.AlertGetCount() > 0)? TRUE: FALSE;

	if (nState == 0 || nState == COMP_NORMAL)
	{
		//
		// NORMAL:
		nIconID = bAlert? IM_CORNER_N_NORMAL: IM_CORNER_R_NORMAL;
	}
	else
	if (nState == COMP_STOP || nState == (COMP_NORMAL|COMP_STOP))
	{
		//
		// STOPPED:
		nIconID = bAlert? IM_CORNER_N_STOPPED: IM_CORNER_R_STOPPED;
	}
	else
	if (nState == COMP_START || nState == (COMP_NORMAL|COMP_START))
	{
		//
		// STARTED:
		nIconID = bAlert? IM_CORNER_N_STARTED: IM_CORNER_R_STARTED;
	}
	else
	if (nState&COMP_STARTMORE && !(nState&COMP_STOP) && !(nState&COMP_HALFSTART))
	{
		//
		// STARTED MORE:
		nIconID = bAlert? IM_CORNER_N_STARTEDMORE: IM_CORNER_R_STARTEDMORE;
	}
	else
	{
		//
		// HALF STARTED:
		nIconID = bAlert? IM_CORNER_N_HALFSTARTED: IM_CORNER_R_HALFSTARTED;
	}

	HICON hIcon = m_TrayImageList.ExtractIcon (nIconID);
	IVM_ShellNotification (NIM_MODIFY, 1, hIcon, m_strShellTip[nIconID]);
	DestroyIcon (hIcon);
}


void CappIvmApplication::OnMessageSetting() 
{
	CaModalUp modalUp;
	CaEventDataMutex mutextEvent(INFINITE);
	CxDlgEventSetting dlg;
	dlg.DoModal();
}

void IVM_TraceLog (LPCTSTR lpszMessageText)
{
}

void IVM_PostModelessMessageBox(UINT nID, HWND hwnd )
{
	CString strMsg;
	if (!strMsg.LoadString(nID))
		strMsg = _T("Cannot Load String Resource");
	if (hwnd == (HWND)0) {
		ASSERT (theApp.m_pMainWnd);
		if (theApp.m_pMainWnd) {
			ASSERT (theApp.m_pMainWnd->m_hWnd);
			hwnd = (theApp.m_pMainWnd->m_hWnd);
		}
	}
	ASSERT(hwnd);
	if (hwnd) {
		LPTSTR lpszError = new TCHAR [strMsg.GetLength() + 1];
		lstrcpy (lpszError, strMsg);
		::PostMessage (hwnd, WMUSRMSG_IVM_MESSAGEBOX, 1, (LPARAM)lpszError);
	}
}


BOOL APP_HelpInfo(UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#ifdef MAINWIN
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += theApp.m_strHelpFile;
	}
	else
		strHelpFilePath = theApp.m_strHelpFile;
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (theApp.m_bHelpFileAvailable)
	{
		if (nHelpID == 0)
			HtmlHelp(theApp.m_pMainWnd->m_hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
		else
			HtmlHelp(theApp.m_pMainWnd->m_hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);
		return TRUE;
	}
	else
	{
		TRACE0 ("Help file is not available: <theApp.m_bHelpFileAvailable = FALSE>\n");
		return FALSE;
	}
	return FALSE;
}

void CappIvmApplication::ModalDialogInc()
{
	m_nModalDlg++;
	//
	// Close the find dialog if it's opened:
	CfMainFrame* pMain = (CfMainFrame*)AfxGetMainWnd();
	if (pMain && IsWindow (pMain->m_hWnd))
		pMain->Search(NULL);
}

static BOOL WaitIngresService()
{
	SC_HANDLE  schSCManager;
	SC_HANDLE  schService;
	SERVICE_STATUS svrstatus;
	memset (&svrstatus, 0, sizeof(svrstatus));
	//
	// Connect to service control manager on the local machine and 
	// open the ServicesActive database
	schSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (schSCManager == NULL )
	{
		CString strMsg;
		LONG err = GetLastError();
		
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
		case ERROR_DATABASE_DOES_NOT_EXIST:
		case ERROR_INVALID_PARAMETER:
			if (!strMsg.LoadString(IDS_E_CONNECT_SERVICE))
				strMsg = _T("Failed to connect to the Service Control Manager");
			AfxMessageBox (strMsg);
			break;
		}
		return FALSE;
	}
	// 
	// Check if the Service 'lpszServiceName' is already installed
	// REGEDT32.EXE can check this on
	// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	schService = OpenService (schSCManager, theApp.m_strIngresServiceName, GENERIC_READ);
	if (schService == NULL)
		return FALSE;
	// 
	// Check if the Service  is Running
	int nTimeOut = 5*60*1000; // 5 mins
	while (nTimeOut > 0)
	{
		Sleep (500);
		nTimeOut -= 500;
		if (!QueryServiceStatus (schService, &svrstatus))
		{
			CString strMsg;
			if (!strMsg.LoadString(IDS_E_SERVICE_STATUS))
				strMsg = _T("Cannot query the service status information");
			AfxMessageBox (strMsg);
			return FALSE;
		}

		switch (svrstatus.dwCurrentState)
		{
		case SERVICE_START_PENDING:
			break;
		default:
			return TRUE;
		}
	}
	return TRUE;
}
