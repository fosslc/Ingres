/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.h, header file 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main Frame of the application
**
** History:
**
** 14-Dec-1998 (uk$so01)
**    Created
** 27-Jan-2000 (uk$so01)
**    (Bug #100157): Activate Help Topic
** 02-Feb-2000 (uk$so01)
**    (Sir #100316)
**    Provide the command line "-stop" to stop ivm from the prompt: ivm -stop.
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 02-Mar-2000 (uk$so01)
**    SIR #100690, If user runs the second Ivm than activate the first one.
** 06-Mar-2000 (uk$so01)
**    BUG #100746, Update to the detection of NT shut down to exit ivm properly.
** 31-Mar-2000 (uk$so01)
**    BUG 101126
**    Put the find dialog at top. When open the modal dialog, 
**    if the find dialog is opened, then close it.
** 20-Apr-2000 (uk$so01)
**    SIR #101252
**    When user start the VCBF, and if IVM is already running
**    then request IVM the start VCBF
** 27-jun-2000 (somsa01)
**    Removed m_pDlgFind and made it a static in mainfrm.cpp . For some reason,
**    it was getting trashed on UNIX platforms via MainWin.
**	27-Jul-2001 (hanje04)
**	    For MAINWIN move declaration of  m_findInfo to mainfrm.cpp
**	    as static.
** 20-Aug-2001 (uk$so01)
**    Fix BUG #105382, Incorrectly notify an alert message when there are no 
**    events displayed in the list.
** 17-Sep-2001 (uk$so01)
**    SIR #105345, Add menu item "Start Client".
**	17-Sep-2001 (hanje04)
**	    Move declaration of m_findInfo back to mainfrm.h because 
**	    Bug 105367 has been fixed.
** 17-Jun-2002 (uk$so01)
**    BUG #107930, fix bug of displaying the message box when the number of
**    events reached the limit of setting.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 11-Mar-2003 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu
** 13-Mar-2003 (uk$so01)
**    SIR #109775, Additional update to change #462359
** 07-May-2003 (uk$so01)
**    BUG #110195, issue #11179264. Ivm show the alerted messagebox to indicate that
**    there are alterted messages but the are not displayed in the right pane
**    according to the preferences
** 27-Apr-2004 (uk$so01)
**    SIR #109775, Provide access to all the GUI tools through toolbar and shell icon menu.
**    (additional changes: add new menu items and change captions)
** 11-Aug-2004 (uk$so01)
**    SIR #109775, Additional change:
**    Add a menu item "Ingres Command Prompt" in Ivm's taskbar icon menu.
** 07-Aug-2007 (drivi01)
**    Added functions to help display icons in the File menu for start,
**    start client, and stop selections.
**/

#if !defined(AFX_MAINFRM_H__63A2E049_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_MAINFRM_H__63A2E049_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "compdata.h"
#include "findinfo.h"
#include "evtcats.h"
#include "fminifrm.h"
#include <windows.h>

class CuDlgShowMessage;
class CuDlgYouHaveNewMessage;
class CfMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CfMainFrame();
	DECLARE_DYNCREATE(CfMainFrame)

	UINT GetHelpID();

public:
	CWnd* GetActiveRightPane();
	CWnd* IsPaneCreated(UINT nPaneID);
	CStatusBar& GetStatusBarControl() {return m_wndStatusBar;}
	void Search(CWnd* pWndCaller);
	CfMiniFrame* ShowMessageDescriptionFrame(BOOL bCreate, MSGCLASSANDID* pMsg);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CfMainFrame();
	UINT ThreadProcControlPeriodicalStoreData (LPVOID pParam);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	static UINT WM_FINDREPLACE;

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	BOOL m_bExit;
	BOOL m_bVisible;
	BOOL m_bEndSession;
	BOOL m_bContinueShowMessageLimitCache;
	BOOL m_bInfromAppShutDown;
	BOOL m_bFromDialog; // Invoke the Search dialog from the dialog box.

	CuDlgYouHaveNewMessage* m_pDlgMessage;
	CuDlgShowMessage* m_pDlgMessageLimitCache;
	CWinThread* m_pThreadSave;
	CfMiniFrame* m_pPropFrame;
	CaFindInformation   m_findInfo;

	HANDLE m_hEventStoreData;
	HANDLE m_heventExceedMax;
	CWnd* m_pWndSearchCaller; 
	//
	// When the Thread begins to read events and sends the first block of events to the Mainframe
	// (this window) the 'm_bDoAlert' is set to FALSE before updating it to TRUE if the are alerted
	// events.
	// And when the Thread finished reading events, if this member 'm_bDoAlert' is TRUE then Beep the user.
	// and set 'm_bDoAlert' to FALSE;
	BOOL m_bDoAlert;
	long m_nNewEventCount; // total number of events comming per thread activity (sum of number of event arrived by block)

	void HandleLimitEvents (WPARAM wParam, LPARAM lParam, BOOL bNoLimit = FALSE);
	void KeepEvents (LPCTSTR lpszFromTime, WPARAM wParam, LPARAM lParam);
	BOOL EventOutofCacheMessage(CaLoggedEvent* pEvent);
	void CreatePropertyMessageFrame();

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CfMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnExpandAll();
	afx_msg void OnStart();
	afx_msg void OnStartClient();
	afx_msg void OnStop();
	afx_msg void OnHaltProgram();
	afx_msg void OnActivateProgram();
	afx_msg void OnIgnore();
	afx_msg void OnUpdateStart(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStartClient(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStop(CCmdUI* pCmdUI);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnConfigure();
	afx_msg void OnCallapseAll();
	afx_msg void OnVisualDBA();
	afx_msg void OnFind();
	afx_msg void OnUpdateFind(CCmdUI* pCmdUI);
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnMessageDescription();
	afx_msg void OnNetworkUtility();
	afx_msg void OnSqlquery();
	afx_msg void OnIpm();
	afx_msg void OnIia();
	afx_msg void OnIea();
	afx_msg void OnVcda();
	afx_msg void OnVdda();
	afx_msg void OnIja();
	afx_msg void OnIilink();
	afx_msg void OnPerformanceWizard();
	afx_msg void OnIngresCache();
	afx_msg void OnErrorLog();
	afx_msg void OnReadme();
	afx_msg void OnIgresdocCmdref();
	afx_msg void OnIgresdocDba();
	afx_msg void OnIgresdocDtp();
	afx_msg void OnIgresdocEsqlc();
	afx_msg void OnIgresdocEquel();
	afx_msg void OnIgresdocGs();
	afx_msg void OnIgresdocIceug();
	afx_msg void OnIgresdocMg();
	afx_msg void OnIgresdocOapi();
	afx_msg void OnIgresdocOme();
	afx_msg void OnIgresdocOsql();
	afx_msg void OnIgresdocQrytools();
	afx_msg void OnIgresdocQuelref();
	afx_msg void OnIgresdocRep();
	afx_msg void OnIgresdocSqlref();
	afx_msg void OnIgresdocStarug();
	afx_msg void OnIgresdocSysadm();
	afx_msg void OnIgresdocUfadt();
	afx_msg void OnIgresdocConnectivity();
	afx_msg void OnIgresdocIpm();
	afx_msg void OnIgresdocReleaseSummary();
	afx_msg void OnAppAbout();
	afx_msg void OnVcbf();
	//}}AFX_MSG
	afx_msg void OnHelpTopic();
	afx_msg LONG OnTrayNotificationCallback (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnPopupMenu(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnSystemShutDown(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnNotifyNewEvent (WPARAM wParam, LPARAM lParam);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg LONG OnUpdateComponent(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnUpdateEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnUpdateShellIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnReinitializeEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnExternalRequestShutDown(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpdis);
    afx_msg void OnInitMenuPopup(CMenu* pMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpmis);
    HICON GetIconForItem(UINT itemID) const;
	
	//
	// lParam contains a string allocated on the heap:
	// after using it, you must called delete operator
	afx_msg LONG OnIvmMessageBox(WPARAM wParam, LPARAM lParam);
	//
	// Periodical save data:
	afx_msg LONG OnSaveData(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFindReplace(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnMakeActive(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnRunProgram(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnIngresPrompt();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__63A2E049_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
