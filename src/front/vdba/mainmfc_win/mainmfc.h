/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainmfc.cpp, Header file 
**    Project  : CA-OpenIngres/VDBA.
**    Author   : Emmanuel Blattes
**    Purpose  : Defines the class behaviors for the application
**
** History after 12-Sept-2000:
**
** 12-Sep-2000 (uk$so01)
**    Show/Hide the control m_staticHeader in IPM and DOM depending on 
**    CMainMfcApp::GetShowRightPaneHeader()
**    Add funtions: GetShowRightPaneHeader and SetShowRightPaneHeader
** 26-Apr-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 18-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
** 18-Mar-2003 (schph01)
**    sir 107523 Add Help id for create sequences Dialog
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
** 03-Nov-2003 (noifr01)
**     fixed errors from massive ingres30->main gui tools source propagation
** 07-Nov-2003 (uk$so01)
**    SIR #106648. Additional fix the session conflict between the container and ocx.
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
**/


// MainMfc.h : main header file for the MAINMFC application
//
#ifndef MAINMFC_HEADER
#define MAINMFC_HEADER

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resmfc.h"       // main symbols. ex-resource.h
#include "dtagsett.h"     // General Display Settings.
#include "calsctrl.h"     // CuListCtrl
#include "rcdepend.h"

//
// Context help ids for dialogs and special cases
//
#define HELPID_MONITOR                8004  // monitor default help id
// defined in main.c:
//HELPID_DBEVENT      8005
//HELPID_NODESEL      8006
//HELPID_BLANKSCREEN  8007

#define IDHELP_IDD_VNODE_ADD          8008
#define IDHELP_IDD_VNODE_ALTER        8009

#define IDHELP_IDD_VNODEDATA_ADD      8010
#define IDHELP_IDD_VNODEDATA_ALTER    8011

#define IDHELP_IDD_VNODELOGIN_ADD     8012
#define IDHELP_IDD_VNODELOGIN_ALTER   8013


#define IDHELP_IDD_XSERVICE_ACCOUNT          8014
#define IDHELP_IDD_STAR_PROCREGISTER         8015
#define IDHELP_IDD_STAR_TABLECREATE          8016 // Sub-Dialog "Local Table Options" for Star when creating Table as native
#define IDHELP_IDD_STAR_TABLEREGISTER        8017
#define IDHELP_IDD_REPLICATION_BUILD_SERVER  8018
#define IDHELP_IDD_REPLIC_RUNNODE            8019
#define IDHELP_IDD_REPLICATION_SERVER_LIST   8020
#define IDHELP_IDD_STAR_VIEWREGISTER         8021
//
// Dialog for Create Star table as Native, in file creattbl.c,
// the constant "8022" is hard coded. Be careful when changing this define. !!!:
#define IDHELP_IDD_STAR_TABLEDIALOG          8022 

//
// Special Help IDs reseved for DOM: range [8030 - 9000]
#define HELPID_DOM                   8030  // dom default help id
#define HELPID_DOM_VIEW_ROWS         8031  // Rows of View
#define HELPID_DOM_TABLE_STAR_ROWS   8032  // Rows of Star Table
#define HELPID_DOM_VIEW_STAR_ROWS    8033  // Rows of Star View
#define HELPID_DOM_LOCATION_LOCAL    8034  // Location of local node
#define HELPID_DOM_LOCATION_REMOTE   8035  // Location of remote node
#define HELPID_DOM_INSTALL_GRANTEES  8036  // Installation grantees
#define HELPID_DOM_INSTALL_ALARMS    8037  // Installation Security alarms

//
// Special Popup Dialog:
#define HELPID_IDD_NODE_INSTALLATION_PASSWORD_ADD   9001 
#define HELPID_IDD_NODE_INSTALLATION_PASSWORD_ALTER 9002 
#define HELPID_IDD_NODE_ATTRIBUTE_ADD   9003 
#define HELPID_IDD_NODE_ATTRIBUTE_ALTER 9004 
#define HELPID_IDD_PRIMARYKEY_ADD       9005 
#define HELPID_IDD_PRIMARYKEY_ALTER     9006 // Not used
#define HELPID_IDD_INDEX_OPTION_ADD     9007 
#define HELPID_IDD_INDEX_OPTION_ALTER   9008 // Not used
#define HELPID_IDD_DATABASE_ADD         9009 // Not used (use the old IDD_CREATEDB) 
#define HELPID_IDD_DATABASE_ALTER       9010
#define HELPID_IDD_DISCONNECT_SVRCLASS  9011 // Disconnect from Server class
#define HELPID_IDD_COMMON_CB_WU_CONNECTION   9012 // Ice
#define HELPID_IDD_COMMON_CB_WU_ROLE         9013
#define HELPID_IDD_COMMON_CB_PROF_CONNECTION 9014
#define HELPID_IDD_COMMON_CB_PROF_ROLE       9015
#define HELPID_IDD_COMMON_CB_LOCATION        9016
#define HELPID_IDD_ICE_BUSINESS_PAGE         9017
#define HELPID_IDD_ICE_BUSINESS_FACET         707
#define HELPID_IDD_ICE_BUSINESS_ROLE         9018
#define HELPID_IDD_ICE_BUSINESS_USER         9019
#define HELPID_IDD_COMMON_ED_BUSINESS_UNIT   9020
#define HELPID_IDD_COMMON_ED_SESSION_GROUP   9021
#define HELPID_IDD_ICE_DATABASE_CONNECT       704
#define HELPID_IDD_ICE_DATABASE_USER          703
#define HELPID_IDD_ICE_LOCATION              9022
#define HELPID_IDD_ICE_PROFILE                706
#define HELPID_IDD_ICE_ROLE                   702
#define HELPID_IDD_ICE_SVR_VARIABLE          9023
#define HELPID_IDD_ICE_WEB_USER               705
#define HELPID_ALTER_IDD_ICE_BUSINESS_PAGE   9024
#define HELPID_ALTER_IDD_ICE_BUSINESS_FACET  9025
#define HELPID_ALTER_IDD_ICE_DATABASE_CONNECT 9026
#define HELPID_ALTER_IDD_ICE_DATABASE_USER   9027
#define HELPID_ALTER_IDD_ICE_PROFILE         9028
#define HELPID_ALTER_IDD_ICE_ROLE            9029
#define HELPID_ALTER_IDD_ICE_WEB_USER        9030
#define HELPID_ALTER_IDD_ICE_BUSINESS_ROLE   9031
#define HELPID_ALTER_IDD_ICE_BUSINESS_USER   9032
#define HELPID_ALTER_IDD_ICE_LOCATION        9033
#define HELPID_ALTER_IDD_ICE_SVR_VARIABLE    9034

#define HELPID_IDD_INDEX_OPTION_PRIMARYKEY   9035 
#define HELPID_IDD_INDEX_OPTION_UNIQUEKEY    9036 
#define HELPID_IDD_INDEX_OPTION_FOREIGNKEY   9037

#define HELPID_IDD_GRANT_INSTALLATION        9038
//
// The value 9039 below is hard coded in file SALARM02.C
// in DBADLG1.as Help ID. (This Header is not accessible from C file).
#define HELPID_IDD_SECALARM_INSTALLATION     9039
//
// The value 9040 below is hard coded in file RVKDB.C
// in DBADLG1.as Help ID. (This Header is not accessible from C file).
#define HELPID_IDD_REVOKE_INSTALLATION       9040

#define HELPID_ICE_ALTER_BUNIT_FACET_ROLE    9041 // Ice
#define HELPID_ICE_BUNIT_FACET_ROLE          9042
#define HELPID_ICE_ALTER_BUNIT_FACET_USER    9043
#define HELPID_ICE_ALTER_BUNIT_PAGE_ROLE     9044
#define HELPID_ICE_BUNIT_PAGE_ROLE           9045
#define HELPID_ICE_ALTER_BUNIT_PAGE_USER     9046
#define HELPID_ICE_BUNIT_PAGE_USER           9047

#define HELPID_CHOOSEOBJECT_INDEX            9048 // Generic Popup dialog for choosing object (ID help for Index)
#define HELPID_STATISIC_INDEX                9049 // DOM Right pane of statistic on index (COMPOSITE HISTOGRAM)

#define HELPID_IDD_CREATEDB_SEQUENCE         9050 // Sequences
#define HELPID_ALTER_IDD_CREATEDB_SEQUENCE   9051

// Global Variable:
// Critical use when low of memory.
static TCHAR VDBA_strMemoryError [128];


typedef enum
{ 
  LEAVINGPAGE_CHANGELEFTSEL = 1,
  LEAVINGPAGE_CHANGEPROPPAGE,
  LEAVINGPAGE_CLOSEDOCUMENT
} LeavePageType;

typedef enum
{ 
  DOMPAGE_ERROR = 0,  // For debug assertion if a page is not managed yet
  DOMPAGE_DETAIL,
  DOMPAGE_LIST,
  DOMPAGE_BOTH,       // Detail and list
  DOMPAGE_SPECIAL
} DomPageType;

//
// Global Functions:
// *****************
void VDBA_OutOfMemoryMessage ();
void VDBA_ArchiveExceptionMessage (CArchiveException* e);

//
// Apply to the list control: (line seperator, ...)
void VDBA_OnGeneralSettingChange(CuListCtrl* pListCtrl);

//
// Concern the opened multiple cursors:
BOOL VDBA_HasBugs();

int AFXAPI BfxMessageBox(LPCTSTR lpszText, UINT nType = MB_OK,
				UINT nIDHelp = 0);
int AFXAPI BfxMessageBox(UINT nIDPrompt, UINT nType = MB_OK,
				UINT nIDHelp = (UINT)-1);

TCHAR * VDBA_MfcResourceString (UINT idString);

// Help system
void PushHelpId(DWORD helpId);
void PopHelpId();

class CaTrace
{
public:
	CaTrace(): m_bStarted (FALSE){}
	~CaTrace()
	{
		if (m_bStarted)
			Stop();
	}

	void Start();
	void Stop ();

private:
	BOOL m_bStarted;
};

/////////////////////////////////////////////////////////////////////////////
// CMainMfcApp:
// See MainMfc.cpp for the implementation of this class
//

class CMainMfcApp : public CWinApp
{
public:
	CMainMfcApp(LPCTSTR lpszAppName = NULL );
	int GetCursorSequence();


	// 
	// Methods for Animation & Task execution:
	void EnableAnimate(BOOL bEnable=TRUE, BOOL bLocked = FALSE);
	BOOL IsAnimateEnabled();
	void ReleaseAnimateMutex(){if (m_hMutexAnimate) ReleaseMutex (m_hMutexAnimate);}
	BOOL IsDlgWaitRunning();
	void SetDlgWaitState(BOOL bRunning = TRUE);
	long DlgWaitGetDelayExecution(){return m_generalDisplaySetting.m_nDlgWaitDelayExecution * 1000;}

	//
	// For the possible values of state, please see the #define INTERRUPT_XXX in the file 'extccall.h' 
	void SetInterruptType(UINT nType);
	UINT GetInterruptType();
	//
	// General display settings
	CaGeneralDisplaySetting* GetGeneralDisplaySetting() {return &m_generalDisplaySetting;}
	void SetLastDocMaximizedState(BOOL bMaximizeNextDoc) {m_bMaximizeNextDoc = bMaximizeNextDoc; }
	BOOL GetLastDocMaximizedState() {return m_bMaximizeNextDoc; }

	//
	// On the context driven mode, if this funtion return FALSE
	// the frame window should not allow to be closed:
	BOOL CanCloseContextDrivenFrame(){return m_bCanCloseContextDrivenFrame;}
	void ContextDrivenFrameInit(BOOL bAbility = FALSE){m_bCanCloseContextDrivenFrame = bAbility;}
	int GetOneWindowType() { return m_iOneWindowType; }
	void SetOneWindowType(int ival) { m_iOneWindowType = ival; }

	//
	// Extract an icon from the image list and return it:
	HICON GetIgresIcon (int nIndex);

	// 
	// The member 'm_bShowRighPaneHeader' must be initialized the 
	// Constructor. FALSE -> Do not show header, TRUE -> Show header.
	BOOL GetShowRightPaneHeader(){return m_bShowRighPaneHeader;}
	void SetShowRightPaneHeader(BOOL bShow){m_bShowRighPaneHeader = bShow;}
	void PropertyChange(BOOL bSet){m_bPropertyChange = bSet;}
	BOOL IsPropertyChange(){return m_bPropertyChange;}
	void SavePropertyAsDefault(BOOL bSet){m_bSavePropertyAsDefault = bSet;}
	BOOL IsSavePropertyAsDefault(){return m_bSavePropertyAsDefault;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainMfcApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL


	COleStreamFile m_sqlStreamFile;
	COleStreamFile m_ipmStreamFile;
	BOOL m_bsqlStreamFile;
	BOOL m_bipmStreamFile;
	//
	// Work around for ISSUE #13768610: The purpose of m_pUnknownSqlQuery is to keep
	// a reference count of a control live until the app exits.
	// This is because of the serialization of the variable global "m_pGlobData" in different
	// classes that used it. (Normally it should be OK because MFC checks if the object has already
	// been serialized). But with the SQLQuery control that can be unloaded during this process of 
	// serialization, the the address space bay be mismatched when the object is stored when the control
	// is alive and reloaded it when the control live is dead. This causes a problem.
	IUnknown* m_pUnknownSqlQuery;
// Implementation
protected:
	int  m_nCursorSequence;
	//
	// A Mutex: m_hMutexAnimate is used to synchronize this below member:
	BOOL   m_bAnimateEnabled;
	HANDLE m_hMutexAnimate;
	//
	// A Mutex: m_hMutexDlgWaitRunning is used to synchronize this below member:
	BOOL   m_bDlgWaitRunning;
	HANDLE m_hMutexDlgWaitRunning;

	//
	// A Mutex: m_hMutexInterruptType is used to synchronize this below member:
	// 'm_nInterruptType' is a mode of interruption that will hint the worker Thread
	// of DlgWait to stop it execution.
	HANDLE m_hMutexInterruptType;
	UINT   m_nInterruptType;
	
	//
	// General Display Settings:
	CaGeneralDisplaySetting m_generalDisplaySetting;
	BOOL m_bMaximizeNextDoc;
	BOOL m_bCanCloseContextDrivenFrame;
	int  m_iOneWindowType;
	CImageList m_ImageListIngresObject;
	BOOL m_bShowRighPaneHeader;
	BOOL m_bPropertyChange;
	BOOL m_bSavePropertyAsDefault;


public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//{{AFX_MSG(CMainMfcApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CMainMfcApp theApp;
#endif

// SQL Trace Info
extern BOOL StartSQLTrace(void);
extern BOOL StopSQLTrace(void);
extern char *GetFirstTraceLine(void);  // returns NULL if no more data
extern char *GetNextTraceLine(void);   // returns NULL if no more data
extern char *GetNextSignificantTraceLine();
 

extern CString & GetTraceOutputBuf();
extern BOOL ExecRmcmdInBackground(char * lpVirtNode, char *lpCmd, CString InputLines);
/////////////////////////////////////////////////////////////////////////////
