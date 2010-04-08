/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : ivm.h , Header File
**
**  Project  : Ingres II / Visual Manager
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Main application behaviour
**
**  History:
**  14-Dec-1998 (uk$so01)
**      Created
**  27-Jan-2000 (uk$so01)
**      (Bug #100157): Activate Help Topic
**  01-Feb-2000 (uk$so01)
**      (Bug #100279)
**      Add the platform management, no ingres service and external
**      access for WIN 95,98
**  02-Feb-2000 (uk$so01)
**      (Sir #100316)
**      Provide the command line "-stop" to stop ivm from the
**      prompt: ivm -stop.
**  03-Feb-2000 (uk$so01)
**      (BUG #100327)
**      Move the data files of ivm to the directory II_SYSTEM\ingres\files.
**  11-Feb-2000 (somsa01)
**      Added global m_LSSfileStatus to keep track of the status of the
**      ingstart.log file.
** 28-Feb-2000 (uk$so01)
**      Bug #100621
**      Reoganize the way of manipulation of ingres variable.
**      Due to the unexpected behaviour of the release version of app, now the app uses the member 
**      CTypedPtrList<CObList, CaParameter*> m_listEnvirenment instead of static CHARxINTxTYPExMINxMAX* IngresEnvironmentVariable
**      allocated on heap.
** 01-Mar-2000 (uk$so01)
**      SIR #100635, Add the Functionality of Find operation.
** 31-Mar-2000 (uk$so01)
**    BUG 101126
**    Put the find dialog at top. When open the modal dialog, 
**    if the find dialog is opened, then close it.
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
** 02-Aug-2000 (uk$so01)
**    BUG #102169
**    Make the function GetScrollManagementLimit() return 0 so that only
**    one list control with special scroll is used to display logged events.
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one IVM per installation ID
** 20-Jun-2001 (noifr01)
**    (bug 105065) removed m_LSSfileStatus (m_fileStatus being restored
**    in the CuDlgStatusInstallationTab2 class)
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 11-Mar-2003 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu
** 19-Oct-2004 (uk$so01)
**    SIR #113260 / ISSUE 13730838. Add a command line -NOMAXEVENTSREACHEDWARNING
**    to disable the Max Event reached MessageBox.
** 18-May-2007 (karye01)
**    SIR #118282, added Help menu item for support url.
** 25-Jul-2007 (drivi01)
**    Define m_bVista field which will get set to TRUE on Vista.
**/

#if !defined(AFX_IVM_H__63A2E045_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_IVM_H__63A2E045_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"  // main symbols
#include "dtagsett.h"  // setting
#include "compdata.h"  // component data
#include "readmsg.h"



void IVM_OutOfMemoryMessage();
BOOL IVM_ShellNotification (DWORD dwMessage, UINT uID, HICON hIcon, LPCTSTR lpszTip = NULL);



/////////////////////////////////////////////////////////////////////////////
// CappIvmApplication:
// See Ivm.cpp for the implementation of this class
//
#define MAX_SHELL_ICON  10

#define SIMULATE_COMPONENT     0x0001
#define SIMULATE_INSTANCE      0x0002
#define SIMULATE_LOGGEDEVENT   0x0004
#define SIMULATE_PARAMETER     0x0008
#define SIMULATE_EVENTCATEGORY 0x0010

#define M_SPECIAL_SCROLL 0
class CdMainDoc;
class CaReadFlag;
class CappIvmApplication : public CWinApp
{
public:
	CappIvmApplication();
	void CleanUp();
	//
	// If the bBlocking = TRUE, this function use the send message to
	// the specific window, otherwise use the PostThreadmessage to the Main Thread:
	BOOL RefreshComponentData(BOOL bComponentInstance = TRUE, BOOL bBlocking = FALSE);
	BOOL RefreshEventData(BOOL bBlocking = FALSE);
	void UpdateShellIcon();
	
	LONG GetLastReadEventId(){return m_lLastReadEventId;}
	void SetLastReadEventId(LONG lLastRead){m_lLastReadEventId = lLastRead;}
	void ResetReadEventId(){m_lLastReadEventId = -1;}
	CdMainDoc* GetMainDocument();

	//
	// TRUE: It means that all events marked as alert in the cache
	//       will automatically marked as unalert if they are remove from the cache
	//       so that if we read all events in the cache the Tree of Components shows
	//       no alert icons.
	//       BUT, physically the removed events are still alerted, eg: if user changes the settings
	//       to containt those events in the cache, they all automatically realert !!!.
	BOOL IsAutomaticUnalert(){return m_bAutomaticUnalert;}

	BOOL IsRequestExited(){return m_bRequestExit;}
	void SetRequestExited(){m_bRequestExit = TRUE;}
	BOOL IsComponentStartStop(){return m_bRequestComponentStartStop;}
	void SetComponentStartStop(BOOL bStartStop){m_bRequestComponentStartStop = bStartStop;}

	void SetRequestInitializeEvents(BOOL bRequest){m_bRequestReinitializeEvents = bRequest;}
	BOOL IsRequestInitializeEvents(){return m_bRequestReinitializeEvents;}

	BOOL IsRequestInitializeReadFlag(){return m_bRequestInitializeReadFlag;}
	void SetRequestInitializeReadFlag(BOOL bRequest){m_bRequestInitializeReadFlag = bRequest;}
	
	void SetLastNameStartedup (long lLast = -1){m_lLastNameServerID = lLast;}
	long GetLastNameStartedup (){return m_lLastNameServerID;}

	CWinThread* GetWorkerThreadComponent() {return m_pThreadComponent;}
	CWinThread* GetWorkerThreadEvent() {return m_pThreadEvent;}
	//
	// Set the Worker threads to be NULL so that they won't be 
	// delete twice:
	void NullWorkerThreads()
	{
		m_pThreadComponent = NULL;
		m_pThreadEvent = NULL;
	}

	UINT GetInstanceTick(){return m_nInstanceTick;}

	//
	// If the logged events are more than M_SPECIAL_SCROLL in the list control
	// then the special scroll is activated:
	int  GetScrollManagementLimit(){return M_SPECIAL_SCROLL;}


	void ConfigureInAction (BOOL bAction){m_bConfigureInAction = bAction;}
	CString GetCurrentInstallationID(){return m_strII_INSTALLATION;}
	//
	// Overrides

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappIvmApplication)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//
	// Implementation
	UINT m_lfileSize;
	UINT m_nSimulate;
	BOOL m_bMessageBoxEvent;
	CaGeneralSetting m_setting;
	CString m_strLoggedArchiver;          // Archiver log file
	CString m_strLoggedRecovery;          // Recovery log file
	CString m_strLoggedStartStop;         // Ingstart log file. (ingstart.log)
	CString m_strLoggedEventFile;         // Log file (errlog.log) to parse for events
	CString m_strArchiveEventFile;        // Archive the log event messages's state (read / not read, alert, ..)
	CString m_strMessageSettingsFile;     // File for Msg Entries + User Defined Folders
	CString m_strIVMEVENTSerializeVersionNumber;  // Manual check the vesion of serialization of file ivmevent.dat
	CString m_strMSCATUSRSerializeVersionNumber;  // Manual check the vesion of serialization of file ivmmsgct.dat
	CString m_strVariableNotSet;
	CString m_strAll;
	BOOL m_bHelpFileAvailable;
	CString m_strHelpFile;
	CString m_strCurrentPath;
	BOOL m_bVisibleAtStartTime;
	CString m_strTimeFormat;
	CString m_strStartupTime;
	HCURSOR m_hCursorDropCategory;
	HCURSOR m_hCursorNoDropCategory;
	CString m_strIngresServiceName;       // Service Name of Ingres Database
	CString m_strIISystem;                // II_SYSTEM Directory
	CString m_strIngresBin;               // Bin Directory of Ingres
	CString m_strIngresVdba;              // Vdba Directory;
	CString m_strWordSeparator;           // Separator of Word (for find functionality).
	CString m_strLastFindWhat;            // Text of the last find operation
	CString m_strStartNAMExINSTALLATION;  // To Synchronize between the thread 'Refresh component' and start stop component
	int     m_nDelayNAMExINSTALLATION;    // Delay time before querying instance if starting NAME or INSTALLATION
	UINT    m_nInstanceFastElapse;        // Fast elapse time to query instances.
	CaMessageDescriptionManager* m_pMessageDescriptionManager;
	BOOL    m_bMaxEventReachedWarning;
	BOOL    m_bVista;						//TRUE on Vista machines

	HANDLE GetMutexTreeData() {return m_hMutexTreeData;}
	HANDLE GetMutexEventData(){return m_hMutexEventData;}
	HANDLE GetMutexInstanceData(){return m_hMutexInstanceData;}
	HANDLE GetEventReinitializeEvent(){return m_hEventReinitializeEvent;}
	HANDLE GetEventComponent(){return m_hEventComponent;}
	HANDLE GetEventStartStop(){return m_hEventStartStop;}
	HANDLE GetEventMaxLimitReached(){return m_hEventMaxEventReached;}

	CaIvmTree&  GetTreeData() {return m_treeData;}
	CaIvmEvent& GetEventData(){return m_loggedEvent;}
	CaIvmEventStartStop& GetEventStartStopData(){return m_loggedEventStartStop;}
	CaParseSpecialInstancesInfo& GetSpecialInstanceInfo() {return m_ParseSpecialInstancesInfo;}

	BOOL IsShowStartStopAnimation() {return m_bShowStartStopAnimate;}

	int  GetModalDialogCount(){return m_nModalDlg;}
	void ModalDialogInc();
	void ModalDialogDec(){m_nModalDlg--;}
	CaReadFlag* GetReadFlag(){return m_pReadFlag;}
	void SetReadFlag(CaReadFlag* pReadFlag){m_pReadFlag = pReadFlag;}

	//
	// Return the total events (CaLoggedEvent) in memory:
	INT_PTR GetEventCount(){return m_loggedEvent.GetEventCount();}
	//
	// Return the total size (in bytes) of events (CaLoggedEvent) in memory:
	UINT GetEventSize() {return m_loggedEvent.GetEventSize();}

	// read/write flag specifying if errlog.log has changed since last IMV stop
	void SetMarkAllAsUnread_flag(BOOL bSet) {m_bMarkAllAsUnread_errlogchanged = bSet;}
	BOOL GetMarkAllAsUnread_flag() {return m_bMarkAllAsUnread_errlogchanged;}

	//
	// Platform ID:
	DWORD GetPlatformID(){return m_dwPlatformId;}
private:
	CdMainDoc* m_pMainDoc;
	BOOL m_bAutomaticUnalert;
	BOOL m_bRequestExit;
	BOOL m_bRequestComponentStartStop;
	BOOL m_bRequestReinitializeEvents;
	BOOL m_bRequestInitializeReadFlag;
	BOOL m_bConfigureInAction;
	BOOL m_bCriticalError; 

	long m_lLastNameServerID;
	CString m_strII_INSTALLATION;
protected:
	int m_nModalDlg;
	BOOL m_bIsCleanedUp;
	BOOL m_bInitInstanceOK;
	BOOL m_bShowStartStopAnimate;
	//
	// Ellapse time to refresh (in ms):
	UINT m_nEllapse;
	UINT m_nInstanceTick;

	CWinThread* m_pThreadComponent;
	CWinThread* m_pThreadEvent;

	HANDLE m_hMutexTreeData;
	HANDLE m_hMutexEventData;
	HANDLE m_hMutexInstanceData;
	HANDLE m_hEventReinitializeEvent;
	HANDLE m_hEventComponent;
	HANDLE m_hEventStartStop;
	HANDLE m_hEventMaxEventReached;
	HANDLE m_hMutexSingleInstance;

	CImageList m_TrayImageList;
	CaIvmTree m_treeData;
	CaIvmEvent m_loggedEvent;
	CaIvmEventStartStop m_loggedEventStartStop;
	DWORD m_dwPlatformId;

	// parse info on special server class and config names
	CaParseSpecialInstancesInfo m_ParseSpecialInstancesInfo;

	CString m_strShellTip [MAX_SHELL_ICON];
	CaReadFlag* m_pReadFlag;
	//
	// Event ID is started from 0. 
	// Ex: If the list containts 100 events
	//     Then, the first event must have an ID = 0, and the last one 
	//     must have an ID = 99.
	LONG m_lLastReadEventId;

	BOOL m_bMarkAllAsUnread_errlogchanged;
	CTypedPtrList<CObList, CaTreeComponentItemData*>* m_pListComponent;

	BOOL RefreshComponentIcon();
	void RefreshCacheEvents();

public:
	//
	// List of ingres variable environments (hard coded):
	//CTypedPtrList<CObList, CaEnvironmentParameter*> m_listEnvirenment;
	CaIngresVariable m_ingresVariable;    // Environment variables

	UINT ThreadProcControlComponents (LPVOID pParam);
	UINT ThreadProcControlEvents (LPVOID pParam);
	void LaunchAboutBox(UINT nIDSTitle, UINT nIDBitmap);

	//{{AFX_MSG(CappIvmApplication)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpOnlineSupport();
	afx_msg void OnPreference();
	afx_msg void OnMessageSetting();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CappIvmApplication theApp;
void IVM_OutOfMemoryMessage();
void IVM_ArchiveExceptionMessage (CArchiveException* e);
void IVM_TraceLog (LPCTSTR lpszMessageText);

// Posts a modeless messagebox, with string ID nID. 
// Specifying the hwnd parameter is required only upon startup.
// After startup, the default hwnd = 0 will result in theApp.m_pMainWnd->m_hWnd 
// to be used
void IVM_PostModelessMessageBox(UINT nID, HWND hwnd = (HWND) 0);
BOOL APP_HelpInfo(UINT nHelpID);


class CaModalUp
{
public:
	CaModalUp(){theApp.ModalDialogInc();}
	~CaModalUp(){theApp.ModalDialogDec();}
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IVM_H__63A2E045_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
