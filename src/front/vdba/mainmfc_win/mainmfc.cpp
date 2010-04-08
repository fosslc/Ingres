/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Visual DBA
**
**  Source : mainmfc.cpp
**  Defines the class behaviors for the application
**
**  Author :  Emmanuel Blattes
**
**  History (after 01-01-2000):
**  24-Jan-2000 (noifr01)
**      (SIR 100103) update m_iOneWindowType in any case, in order
**      to keep current, during the whole life of the executable, the 
**      information whether VDBA was started with exactly one DOM / SQL
**      or MONITOR window, with the parameters used when invoked by the
**      network utility (the info is available through the
**      GetOneWindowType() function)
**  28-01-2000 (noifr01)
**      (bug 100196) when VDBA is invoked in the context, enforce a refresh
**      of the nodes list in the cache when the list is needed later than 
**      upon initialization
**  03-Feb-2000
**      (BUG #100324)
**      Replace the named Mutex by the unnamed Mutex.
**  22-May-2000 (uk$so01)
**      bug 99242 (DBCS) modify the function: VDBA_EliminateTabs
**  26-Sep-2000 (uk$so01 and noifr01)
**      (SIR 102711) misc "callable in context" improvements for Manage-IT
**  08-dec-2000 (somsa01)
**      Corrected various build problems with HP.
**  22-Dec-2000 (noifr01)
**      (SIR 103548) cleanup about the application title
**  04-Jan-2001 (noifr01)
**   (bug 106735) updated the year to appear in the about box
**  11-Jun-2001 (uk$so01)
**      SIR #104736, Integrate IIA and Cleanup old code of populate.
**  09-Jul-2001 (hanje04)
**      Register libsvriia.so for UNIX.
**  07-Nov-2001 (uk$so01)
**      SIR #106290, new corporate guidelines and standards about box.
**  26-Nov-2001 (hanje04)
**      BUG 106461
**      Animation was causing problems on UNIX. Disabled
**  17-Dec-2001 (noifr01)
**     (sir 99596) removal of obsolete code and resources
**  06-Jan-2002 (noifr01)
**      (additional change for SIR 106290) for consistency of product
**      names in the product and the splash screen and "about" boxes,
**      restore "Ingres" at the left of "Visual DBA" in the about box
**      caption
**  18-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
**  22-feb-2002 (somsa01)
**      For HP-UX, the shared library to register is 'libsvriia.sl'.
**  27-May-2002 (uk$so01)
**      BUG #107880, Add the XML decoration encoding='UTF-8' or 'WINDOWS-1252'...
**      depending on the Ingres II_CHARSETxx.
**  04-Jun-2002 (hanje04)
**      Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**      unwanted ^M's in generated text files.
**  06-Jun-2002 (noifr01)
**      (SIR 107951) set the locale (for LC_TIME only) to the default on the
**      machine
**  18-Jun-2002 (noifr01)
**      (SIR 108060) set the locale (for LC_NUMERIC) to the default so that
**      at the few places where VDBA generates a numeric value with decimals,
**      these values are displayed with the decimal separator configured 
**      at the OS level
**  22-Sep-2002 (schph01)
**      Bug 108645 call the new function InitLocalVnode ( ) to retrieve the
**      "gcn.local_vnode" parameter in config.dat file.
**  16-Oct-2002 (noifr01)
**      restored change from 22-feb-2002 that had been lost in a later change
**  29-Jan-2003 (noifr01)
**      (SIR 108139) get the version string and year by parsing information 
**       from the gv.h file 
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
**  03-Nov-2003 (noifr01)
**     fixed errors from massive ingres30->main gui tools source propagation
**  07-Nov-2003 (uk$so01)
**    SIR #106648. Additional fix the session conflict between the container and ocx.
**  10-Feb-2004 (schph01)
**    (SIR 108139)  additional change for retrieved the year by parsing information 
**    from the gv.h file 
** 28-Apr-2004 (uk$so01)
**    BUG #112221 / ISSUE 13403162 (remove some warnings)
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, Add the functionality
**    to allow vdba to request the IPM.OCX and SQLQUERY.OCX
**    to know if there is an SQL session through a given vnode
**    or through a given vnode and a database.
** 20-Oct-2004 (uk$so01)
**    BUG #113271 / ISSUE #13751480 -- The string GV_VER does not contain 
**    information about Year anymore. So hardcode Year to 2004.
** 10-Nov-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
** 22-Nov-2004 (schph01)
**    Bug #113511 initialize _tsetlocale() function according to the
**    LC_COLLATE category setting of the current locale,
**    for sorting object name in tree and Rigth pane list.
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 14-Feb-2006 (drivi01)
**    Update the year to 2006.
** 08-Jan-2007 (drivi01)
**    Update the year to 2007.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 12-Mar-2009 (smeke01) b121780
**    Added self-registration for SQL Assistant along the same lines as
**    for Import/Export Assistant. At the same time, removed logically
**    pointless local declaration UINT nIDSErrorIISystem and generally
**    made the #defines layout more logical.
** 20-Mar-2009 (smeke01) b121832
**    Add call to INGRESII_BuildVersionInfo() to set product version 
**    and year for About screen.
** 21-Mar-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up warnings.
** 10-Mar-2010 (drivi01) SIR 123397
**    Add functionality to bring up "Generate Statistics" dialog 
**    from command line by providing proper vnode/database/table flags
**    to Visual DBA utility.
**    example: vdba.exe /c dom (local) table junk/mytable1 GENSTATANDEXIT
*/

#include "stdafx.h"
#include <locale.h>
#include "mainmfc.h"
#include "rcdepend.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "maindoc.h"
#include "mainview.h"
#include "mainvi2.h"

#include "qsqldoc.h"
#include "qsqlfram.h"
#include "qsqlview.h"

#include "ipmaxfrm.h"
#include "ipmaxdoc.h"
#include "ipmaxvie.h"

#include "dbeframe.h"
#include "dbedoc.h"
#include "dbeview.h"

#include "docvnode.h"
#include "frmvnode.h"
#include "vewvnode.h"

#include "splash.h"
#include "extccall.h" // INTERUPT_XXX

#include "cmixfile.h"

#include "cmdline.h"

#include "nodefast.h"   // for unicenter driven feature
#include "domfast.h"
#include "wmusrmsg.h"
#include "toolbars.h"
#include "prodname.h"
#include "dgdptrow.h"
#include "copyright.h"

extern "C"
{

#include <gv.h>
#include "dbaginfo.h"
#include "dbadlg1.h"    // for help stack management
int WinMainEnter(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow);
int WinMainTerminate();
//
// The 4 lines below are inserted instead of including "libguids.h" in ..\inc
// because in ..\inc there are some header files that have the same name of those
// in mainmfc and there will be the conflicts.
extern LPCTSTR cstrCLSID_IMPxEXPxASSISTANCT;
extern LPCTSTR cstrCLSID_SQLxASSISTANCT; 
BOOL LIBGUIDS_IsCLSIDRegistered(LPCTSTR lpszCLSID);
int LIBGUIDS_RegisterServer(LPCTSTR lpszServerName, LPCTSTR lpszPath);
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define CURSOR_OFF_LIMIT (INT_MAX - 100)

static void NotifyUpdateDBEvent (int nHnode, LPCTSTR lpszDBName = NULL);


void CaTrace::Start()
{
	if (StartSQLTrace())
		m_bStarted = TRUE;
}

void CaTrace::Stop ()
{
	if (m_bStarted)
		StopSQLTrace();
	m_bStarted = FALSE;
}


// from childfrm.cpp
extern "C" void MfcBlockUpdateRightPane();
extern "C" void MfcAuthorizeUpdateRightPane();

static BOOL AutomatedDomDoc(CTreeItemNodes* pSelectedItem, CuWindowDesc* pDesc)
{
  // Create Doc, masking out right pane update if must expand up to specified item
  BOOL bHasObjDesc = pDesc->HasObjDesc();
  BOOL bHasSingleItem = pDesc->HasSingleTreeItem();
  if (bHasSingleItem)
    ASSERT (bHasObjDesc);

  if (bHasObjDesc && !bHasSingleItem)
    MfcBlockUpdateRightPane();
  BOOL bSuccess = pSelectedItem->TreeConnect();
  if (bHasObjDesc && !bHasSingleItem)
    MfcAuthorizeUpdateRightPane();
  if (!bSuccess)
    return FALSE;

  // Pick most recently created doc pointer
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  CMDIChildWnd *pActiveMDI = pMainFrame->MDIGetActive();
  ASSERT (pActiveMDI);
  int type = QueryDocType(pActiveMDI->m_hWnd);
  ASSERT (type == TYPEDOC_DOM);
  ASSERT (pActiveMDI->IsKindOf(RUNTIME_CLASS(CChildFrame)));
  CChildFrame* pActiveDom = (CChildFrame*)pActiveMDI;

  // Manage toolbar visibility
  BOOL bHideDocToolbar = pDesc->DoWeHideDocToolbar();
  if (bHideDocToolbar)
    SetToolbarInvisible(pActiveDom, TRUE);    // TRUE forces immediate update
  
  // expand up to specified item if specified
  if (bHasObjDesc && !bHasSingleItem) {
    CuDomObjDesc* pObjDesc = pDesc->GetDomObjDescriptor();
    ASSERT (pObjDesc);
    if (!pObjDesc)
      return FALSE;

    // tree organization reflected in FastItemList, different according to record type
    CTypedPtrList<CObList, CuDomTreeFastItem*>  domTreeFastItemList;
    BOOL bSuccess = pObjDesc->BuildFastItemList(domTreeFastItemList);
    ASSERT (bSuccess);
	BOOL bContinue = TRUE;
	if (pDesc->GetAAD() == OBJECT_ADD_EXIT && pObjDesc->GetLevel() == 0)
		bContinue = pActiveDom->SpecialCmdAddAlterDrop(pObjDesc);
	else
    if (!ExpandUpToSearchedItem(pActiveDom, domTreeFastItemList, TRUE)) {
      AfxMessageBox (IDS_DOM_FAST_CANNOTEXPAND, MB_OK | MB_ICONEXCLAMATION);
      // Don't return FALSE so we will display the doc anyway
	  bContinue = FALSE;
    }

	if (bContinue)
	{
		//
		// uk$so01 (26-sep-2000)
		if (pActiveDom) 
		{
			switch (pDesc->GetAAD())
			{
			case OBJECT_ADD_EXIT:
				pActiveDom->ObjectAdd();
				break;
			case OBJECT_ALTER_EXIT:
				pActiveDom->ObjectAlter();
				break;
			case OBJECT_DROP_EXIT:
				pActiveDom->ObjectDrop();
				break;
			case OBJECT_OPTIMIZE_EXIT:
				pActiveDom->ObjectOptimize();
			default:
				break;
			}
		}
	}

    while (!domTreeFastItemList.IsEmpty())
      delete domTreeFastItemList.RemoveHead();
  }

  return TRUE;
}

static BOOL AutomatedSqlactDoc(CTreeItemNodes* pSelectedItem, CuWindowDesc* pDesc)
{
	CString strDatabase;
	LPCTSTR lpszDatabase;
	
	if (pDesc->HasObjDesc())
	    strDatabase = pDesc->GetObjectIdentifier();
	else
	    strDatabase = (LPCTSTR)_T("");

	if (strDatabase.IsEmpty())
	    lpszDatabase = NULL;
	else
	    lpszDatabase = (LPCTSTR)strDatabase;

	return pSelectedItem->TreeSqlTest(lpszDatabase);
}

static BOOL AutomatedMonitorDoc(CTreeItemNodes* pSelectedItem, CuWindowDesc* pDesc)
{
	BOOL bSuccess = pSelectedItem->TreeMonitor();   // Note : toolbar visibity taken care of in this call
	return bSuccess;
}

static BOOL AutomatedDbeventDoc(CTreeItemNodes* pSelectedItem, CuWindowDesc* pDesc)
{
	CString strDatabase;
	LPCTSTR lpszDatabase;
	
	if (pDesc->HasObjDesc())
	    strDatabase = pDesc->GetObjectIdentifier();
	else
	    strDatabase = (LPCTSTR)_T("");

	if (strDatabase.IsEmpty())
	    lpszDatabase = NULL;
	else
	    lpszDatabase = (LPCTSTR)strDatabase;

	return pSelectedItem->TreeDbevent(lpszDatabase);
}


/////////////////////////////////////////////////////////////////////////////
// CMainMfcApp

BEGIN_MESSAGE_MAP(CMainMfcApp, CWinApp)
	//{{AFX_MSG_MAP(CMainMfcApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	//ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	//ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainMfcApp construction

CMainMfcApp::CMainMfcApp(LPCTSTR lpszAppName):CWinApp(lpszAppName)
{
	lstrcpy (
		VDBA_strMemoryError, 
		"Low of Memory ...\nCannot allocate memory, please close some applications !");
	m_nCursorSequence = 0;

	m_bAnimateEnabled = TRUE;
	m_hMutexAnimate = CreateMutex (NULL, FALSE, NULL);
	if (m_hMutexAnimate == NULL)
	{
		AfxMessageBox (IDS_E_CREATE_MUTEX);
	}

	m_bDlgWaitRunning = FALSE;
	m_hMutexDlgWaitRunning = CreateMutex (NULL, FALSE, NULL);
	if (m_hMutexDlgWaitRunning == NULL)
	{
		AfxMessageBox (IDS_E_CREATE_MUTEX);
	}

	m_nInterruptType = INTERRUPT_NOT_ALLOWED;
	m_hMutexInterruptType = CreateMutex (NULL, FALSE, NULL);
	if (m_hMutexInterruptType == NULL)
	{
		AfxMessageBox (IDS_E_CREATE_MUTEX);
	}
	m_bCanCloseContextDrivenFrame = TRUE;
	m_bShowRighPaneHeader = TRUE;
	m_bPropertyChange = FALSE;
	m_bSavePropertyAsDefault = TRUE;
	m_pUnknownSqlQuery = NULL;
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

static BOOL INGRESII_llSetPasswordPrompt()
{
	BOOL bOK = TRUE;
	CString strDllName = _T("tksplash.dll");
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strDllName  = _T("libtksplash.sl");
	#else
	strDllName  = _T("libtksplash.so");
	#endif
#endif
	HINSTANCE hLib = LoadLibrary(strDllName);
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		bOK = FALSE;
	if (bOK)
	{
		void (PASCAL *lpDllEntryPoint)();
		(FARPROC&)lpDllEntryPoint = GetProcAddress (hLib, "INGRESII_llSetPrompt");
		if (lpDllEntryPoint == NULL)
			bOK = FALSE;
		else
		{
			(*lpDllEntryPoint)();
		}
	}

	return bOK;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMainMfcApp object

CMainMfcApp theApp;

CMixFile CFile4LoadSave;
CString  mystring;
/////////////////////////////////////////////////////////////////////////////
// CMainMfcApp initialization

BOOL CMainMfcApp::InitInstance()
{
	AfxEnableControlContainer();
  // Analyse command line in search of the Unicenter automation
  CuCmdLineParse cmdParse(m_lpCmdLine);
  cmdParse.ProcessCmdLine();

  InitContextNodeListStatus();
  InitInstallIDInformation();

  setlocale(LC_TIME,"" );
  setlocale(LC_NUMERIC,"");
  setlocale(LC_COLLATE ,"");
  setlocale(LC_CTYPE ,"");
  if (!InitLocalVnode ( ))
  {
    CString csMessError;
    csMessError.LoadString (IDS_MSG_LOCAL_VNODE_NOT_DEFINED);
    AfxMessageBox (csMessError, MB_ICONSTOP|MB_OK);
    return FALSE;
  }
  // Manage splash screen
  if (cmdParse.IsUnicenterDriven()) {
    BOOL bHideSplash = cmdParse.DoWeHideSplash();
		TRACE0("About to Enable Unicentered driven splash screen\n");
		CSplashWnd::EnableSplashScreen(!bHideSplash);
  }
  else {
  	// CG: The following block was added by the Splash Screen component.
	  {
		  CCommandLineInfo cmdInfo;
		  ParseCommandLine(cmdInfo);
		  TRACE0("About to Enable splash screen\n");
		  CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
	  }
  }

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	/* not need this block of codes for .NET
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	*/


	LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)
	m_generalDisplaySetting.Load();

	SetLastDocMaximizedState(m_generalDisplaySetting.m_bMaximizeDocs);
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		//AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

#if !defined (_DEBUG) && !defined (EDBC)
#if !defined (MAINWIN)
	CString strSvriia = _T("svriia.dll");
#else
#if defined(hpb_us5)
	CString strSvriia = _T("libsvriia.sl");
#else
	CString strSvriia = _T("libsvriia.so");
#endif
#endif

	if (!LIBGUIDS_IsCLSIDRegistered(cstrCLSID_IMPxEXPxASSISTANCT))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (strSvriia, NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, strSvriia);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 0:
			// Success:
			break;
		}
	}

#if !defined (MAINWIN)
	CString strSvrsqlas = _T("svrsqlas.dll");
#else
#if defined(hpb_us5)
	CString strSvrsqlas = _T("libsvrsqlas.sl");
#else
	CString strSvrsqlas = _T("libsvrsqlas.so");
#endif
#endif

	if (!LIBGUIDS_IsCLSIDRegistered(cstrCLSID_SQLxASSISTANCT))
	{
		CString strMsg;
		BOOL bOK = TRUE;
		int nRes = LIBGUIDS_RegisterServer (strSvrsqlas, NULL);
		switch (nRes)
		{
		case 2:
			//_T("II_SYSTEM is not defined");
			strMsg.LoadString (IDS_MSG_II_SYSTEM_NOT_DEFINED);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 1:
			AfxFormatString1(strMsg, IDS_MSG_FAILED_2_REGISTER_SERVER, strSvrsqlas);
			AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
			return FALSE;
		case 0:
			// Success:
			break;
		}
	}
#endif

	INGRESII_llSetPasswordPrompt();
	//
	// Create the image list for Ingres Object:
	// Call the create with the correct IDB_XX or Create without IDB_XX and load
	// Extract icons from other IMAGE LIST to add into it:
	m_ImageListIngresObject.Create (IDB_PREFERENCE_SMALL, 16, 4, RGB (255, 0, 255));

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

  // register dom document templates ( 1 per gateway)
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_MAINMFTYPE,
		RUNTIME_CLASS(CMainMfcDoc),
		RUNTIME_CLASS(CChildFrame),     // custom MDI child frame
		RUNTIME_CLASS(CMainMfcView));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
		IDR_MAINMFTYPE_NOTOI,
		RUNTIME_CLASS(CMainMfcDoc),
		RUNTIME_CLASS(CChildFrame),     // custom MDI child frame
		RUNTIME_CLASS(CMainMfcView));
	AddDocTemplate(pDocTemplate);

  // register sql document templates ( 1 per gateway)
	pDocTemplate = new CMultiDocTemplate(
		IDR_SQLACTTYPE,
		RUNTIME_CLASS(CdSqlQuery),
		RUNTIME_CLASS(CfSqlQueryFrame), // custom MDI child frame
		RUNTIME_CLASS(CvSqlQuery));
	AddDocTemplate(pDocTemplate);

  HINSTANCE hInstance = AfxGetInstanceHandle();
  int res = WinMainEnter(hInstance, 0, "", SW_SHOWNORMAL);    // will set hInst and hResource
  // Emb/Vut/Ps 11/07/97: test return value
  if (res == 0) {
    //"Cannot Initialize Application!"
    AfxMessageBox(IDS_E_INIT_APP, MB_OK | MB_ICONSTOP);
    return FALSE;
  }

  // register Ipm document templates ( only one )
	pDocTemplate = new CMultiDocTemplate(
    IDR_IPMTYPE,
 		RUNTIME_CLASS(CdIpm),
		RUNTIME_CLASS(CfIpm),
 		RUNTIME_CLASS(CvIpm));
	AddDocTemplate(pDocTemplate);

  // register DbEvent document templates ( only one )
	pDocTemplate = new CMultiDocTemplate(
    IDR_DBEVENTTYPE,
 		RUNTIME_CLASS(CDbeventDoc),
		RUNTIME_CLASS(CDbeventFrame),
 		RUNTIME_CLASS(CDbeventView));
	AddDocTemplate(pDocTemplate);

  // register virtual node mdi frame
  pDocTemplate = new CMultiDocTemplate(
    IDR_VNODEMDI,
    RUNTIME_CLASS(CDocVNodeMDIChild),
    RUNTIME_CLASS(CFrmVNodeMDIChild),
    RUNTIME_CLASS(CViewVNodeMDIChild));
  AddDocTemplate(pDocTemplate);

  //
  // New section for alternate menus due to context-driven options
  //
	pDocTemplate = new CMultiDocTemplate(
		IDR_MAINMFTYPE_NOMENU,
		RUNTIME_CLASS(CMainMfcDoc),
		RUNTIME_CLASS(CChildFrame),     // custom MDI child frame
		RUNTIME_CLASS(CMainMfcView));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
		IDR_MAINMFTYPE_NOTOI_NOMENU,
		RUNTIME_CLASS(CMainMfcDoc),
		RUNTIME_CLASS(CChildFrame),     // custom MDI child frame
		RUNTIME_CLASS(CMainMfcView));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
    IDR_IPMTYPE_NOMENU,
 		RUNTIME_CLASS(CdIpm),
		RUNTIME_CLASS(CfIpm),
 		RUNTIME_CLASS(CvIpm));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
		IDR_MAINMFTYPE_OBJMENU,
		RUNTIME_CLASS(CMainMfcDoc),
		RUNTIME_CLASS(CChildFrame),     // custom MDI child frame
		RUNTIME_CLASS(CMainMfcView));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
		IDR_MAINMFTYPE_NOTOI_OBJMENU,
		RUNTIME_CLASS(CMainMfcDoc),
		RUNTIME_CLASS(CChildFrame),     // custom MDI child frame
		RUNTIME_CLASS(CMainMfcView));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
    IDR_IPMTYPE_OBJMENU,
 		RUNTIME_CLASS(CdIpm),
		RUNTIME_CLASS(CfIpm),
 		RUNTIME_CLASS(CvIpm));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
		IDR_SQLACTTYPE_NOMENU,
		RUNTIME_CLASS(CdSqlQuery),
		RUNTIME_CLASS(CfSqlQueryFrame), // custom MDI child frame
		RUNTIME_CLASS(CvSqlQuery));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CMultiDocTemplate(
    IDR_DBEVENTTYPE_NOMENU,
 		RUNTIME_CLASS(CDbeventDoc),
		RUNTIME_CLASS(CDbeventFrame),
 		RUNTIME_CLASS(CDbeventView));
	AddDocTemplate(pDocTemplate);

  if (cmdParse.IsUnicenterDriven()) {
    if (cmdParse.SyntaxError()) {
      CString csError;
      //"Syntax error in command line near \"%s\""
      csError.Format(VDBA_MfcResourceString (IDS_E_COMMAND_SYNTAX), cmdParse.GetErrorToken());
      AfxMessageBox(csError, MB_ICONSTOP | MB_OK);
      return FALSE;   // Will terminate the application
    }
    else {
      SetUnicenterDriven(&cmdParse);
	}
  }

  // create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

  // manage command line with Unicenter automation
  if (cmdParse.IsUnicenterDriven()) {
    {
		if (cmdParse.GetHideApp())
		{
			m_nCmdShow = SW_HIDE;
			pMainFrame->SetNoSaveOnExit();
		}
//      SetUnicenterDriven(&cmdParse);
      BOOL bMaxApp    = cmdParse.DoWeMaximizeApp();
      BOOL bHideNodes = cmdParse.DoWeHideNodes();
      BOOL bHideMainToolbar = cmdParse.DoWeHideMainToolbar();
      BOOL bNoSaveOnExit = cmdParse.DoWeExitWithoutSave();

	  // Manage banning of CloseWindow
	  if (FlagsSayNoCloseWindow())
		ContextDrivenFrameInit(FALSE); // Inhibit close frame

      // Manage nodes bar visibility
      if (bHideNodes) {
        CuResizableDlgBar *pNodeBar = pMainFrame->GetVNodeDlgBar();
        ASSERT (pNodeBar);
        if (pNodeBar) {
          pMainFrame->ShowControlBar(pNodeBar, FALSE, FALSE); // Hide immediately
          // unmask if necessary: pMainFrame->RecalcLayout();
        }
      }

      // Manage main toolbar visibility
      if (bHideMainToolbar) {
        CToolBar* pMainToolBar = pMainFrame->GetPMainToolBar();
        ASSERT (pMainToolBar);
        pMainFrame->ShowControlBar(pMainToolBar, FALSE, TRUE);
      }

      // Manage main window "No save on exit" flag
      if (bNoSaveOnExit)
        pMainFrame->SetNoSaveOnExit();

	    // The main window has been initialized, so show and update it.
	  if (!IsInvokedInContextWithOneWinExactly()) {
		  if (bMaxApp)
			  pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);
		  else
			  pMainFrame->ShowWindow(m_nCmdShow);
			pMainFrame->UpdateWindow();
	  }
	  else { /* if 1 window exactly, and an object has been specified, get rid of the title of the "right pane"*/
		CuWindowDesc* pDesc = GetWinDescPtr();
		if (pDesc) {
			if (pDesc->HasObjDesc())
				SetShowRightPaneHeader(FALSE);
		}
	  }

      //
      // Create documents as specified in parameter list
      //
      CTreeCtrl* pNodeTree = pMainFrame->GetVNodeTreeCtrl();
      ASSERT (pNodeTree);

      CTypedPtrList<CObList, CuWindowDesc*>* pWinDescList = cmdParse.GetPWindowDescSerialList();
      POSITION pos = pWinDescList->GetHeadPosition();
      while (pos) {
        CuWindowDesc* pDesc = pWinDescList->GetNext(pos);

        // Expand nodes tree up to right item
        CString csNodeName        = pDesc->GetNodeName();   // can be (local)
        CString csServerClassName = pDesc->GetServerClassName();
        CString csUserName        = pDesc->GetUserName();
        BOOL    bHasServerClass   = !csServerClassName.IsEmpty();
        BOOL    bHasUser          = !csUserName.IsEmpty();
        CTypedPtrList<CObList, CuNodeTreeFastItem*>  NodeTreeFastItemList;
        NodeTreeFastItemList.AddTail(new CuNodeTreeFastItem(FALSE, OT_NODE, csNodeName));
        if (bHasServerClass) {
          NodeTreeFastItemList.AddTail(new CuNodeTreeFastItem(TRUE, OT_NODE_SERVERCLASS));
          NodeTreeFastItemList.AddTail(new CuNodeTreeFastItem(FALSE, OT_NODE_SERVERCLASS, csServerClassName));
        }
        if (bHasUser) {
          NodeTreeFastItemList.AddTail(new CuNodeTreeFastItem(TRUE, OT_USER));
          NodeTreeFastItemList.AddTail(new CuNodeTreeFastItem(FALSE, OT_USER, csUserName));
        }
        if (!ExpandAndSelectUpToSearchedItem(pMainFrame, NodeTreeFastItemList)) {
          AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
          return FALSE;   // Will terminate the application
        }
        while (!NodeTreeFastItemList.IsEmpty())
          delete NodeTreeFastItemList.RemoveHead();

        // Create the mdi doc and preselect item if requested
        HTREEITEM hSelectedItem = pNodeTree->GetSelectedItem();
        ASSERT (hSelectedItem);
        CTreeItemNodes* pSelectedItem = (CTreeItemNodes*)pNodeTree->GetItemData(hSelectedItem);
        ASSERT (pSelectedItem);
        BOOL bSuccess = FALSE;

        SetAutomatedWindowDescriptor(pDesc);
		m_iOneWindowType =  pDesc->GetType();
        switch (pDesc->GetType()) {
          case TYPEDOC_DOM:
            bSuccess = AutomatedDomDoc(pSelectedItem, pDesc);
            break;
          case TYPEDOC_SQLACT:
            bSuccess = AutomatedSqlactDoc(pSelectedItem, pDesc);
            break;
          case TYPEDOC_MONITOR:
            bSuccess = AutomatedMonitorDoc(pSelectedItem, pDesc);
            break;
          case TYPEDOC_DBEVENT:
            bSuccess = AutomatedDbeventDoc(pSelectedItem, pDesc);
            break;
        }
        ResetAutomatedWindowDescriptor();

        if (!bSuccess) {
            //_T("Error in creating documents")
            AfxMessageBox (IDS_E_CREATE_DOC);
            return FALSE;   // Will terminate the application
        }
      }
	  if (IsInvokedInContextWithOneWinExactly()) {
		  // m_iOneWindowType  already setup above (single pass in the loop)
		  if (bMaxApp)
			  pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);
		  else
			  pMainFrame->ShowWindow(m_nCmdShow);
			pMainFrame->UpdateWindow();
	  }
	  else 
		m_iOneWindowType = TYPEDOC_NONE;

      // Note: doc maximize now taken in account by PreCreateWindow() for each relevant frame type
    }
    ResetUnicenterDriven();
	RefreshNodesListIfNeededUponNextFetch();

	//
	// uk$so01 (26-sep-2000)
	if (cmdParse.GetHideApp())
	{
		pMainFrame->PostMessage(WM_CLOSE, 0, 0);
	}
  }
  else {
    // Not unicenter driven: let normal flow

	  // Parse command line for standard shell commands, DDE, file open
	  CCommandLineInfo cmdInfo;
	  ParseCommandLine(cmdInfo);

    CString cmdLine = CString("");
    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen) {
      cmdLine = cmdInfo.m_strFileName;
    }
    if (x_strlen(m_lpCmdLine) && cmdLine.IsEmpty())
      cmdLine = m_lpCmdLine;

	  // The main window has been initialized, so show and update it.
	  pMainFrame->ShowWindow(m_nCmdShow);
	  pMainFrame->UpdateWindow();

      m_iOneWindowType = TYPEDOC_NONE; // (we are not in the case of one window lauched with the ingnet params)

    // load initial environment if supplied
    if (!cmdLine.IsEmpty())
      pMainFrame->OnInitialLoadEnvironment(cmdLine);

  }

  return TRUE;
}


// App command to run the dialog
void CMainMfcApp::OnAppAbout()
{
	PushVDBADlg();
	CString strDllName = _T("tksplash.dll");
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strDllName  = _T("libtksplash.sl");
	#else
	strDllName  = _T("libtksplash.so");
	#endif
#endif
	BOOL bOK = TRUE;
	HINSTANCE hLib = LoadLibrary(strDllName);
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
			CString strVer;
			int year;
			
			/* get year and version information */
			INGRESII_BuildVersionInfo (strVer, year);

			// 0x00000002 : Show Copyright
			// 0x00000004 : Show End-User License Aggreement
			// 0x00000008 : Show the WARNING law
			// 0x00000010 : Show the Third Party Notices Button
			// 0x00000020 : Show System Info Button
			// 0x00000040 : Show Tech Support Button
			UINT nMask = 0x00000002|0x00000008;

#ifdef EDBC
			strTitle.LoadString(IDS_PRODNAME_CAPTION_EDBC);
#else
			strTitle.LoadString(IDS_PRODNAME_CAPTION);
#endif
			strAbout.Format(IDS_PRODUCT_VERSION,(LPCTSTR)strVer);
			(*lpDllEntryPoint)(strTitle, strAbout, (short)year, nMask);
		}
		FreeLibrary(hLib);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_FAIL_2_LOCATEDLL, (LPCTSTR)strDllName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
	PopVDBADlg();
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcApp commands

int CMainMfcApp::ExitInstance() 
{
	WinMainTerminate();
	if (m_pUnknownSqlQuery)
		m_pUnknownSqlQuery->Release();
	return CWinApp::ExitInstance();
}

int CMainMfcApp::GetCursorSequence()
{
	m_nCursorSequence++; 
	if (m_nCursorSequence >= CURSOR_OFF_LIMIT)
	{
		AfxMessageBox (IDS_E_RESTART_PROG);
	}
	return m_nCursorSequence;
}


//
// Global Functions:
// *****************
static TCHAR tchszOutOfMemoryMessage [] =
    "Low of Memory ...\n"
    "Cannot allocate memory, please close some applications !";
void VDBA_OutOfMemoryMessage ()
{
    BfxMessageBox (tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
}

void VDBA_ArchiveExceptionMessage (CArchiveException* e)
{
    TCHAR tchszMsg [128];
    tchszMsg[0] = '\0';
    switch (e->m_cause)
    {
    case CArchiveException::none:
        break;
    case CArchiveException::genericException:
        //"Serialization error:\nCause: Unknown."
        lstrcpy (tchszMsg, VDBA_MfcResourceString(IDS_E_SERIALIZATION_SEVEN));
        break;
    case CArchiveException::readOnly:
        //"Serialization error:\nCause: Tried to write into an archive opened for loading."
        lstrcpy (tchszMsg, VDBA_MfcResourceString(IDS_E_SERIALIZATION_ONE));
        break;
    case CArchiveException::endOfFile:
        //"Serialization error:\nCause: Reached end of file while reading an object."
        lstrcpy (tchszMsg, VDBA_MfcResourceString(IDS_E_SERIALIZATION_TWO));
        break;
    case CArchiveException::writeOnly:
        //"Serialization error:\nCause: Tried to read from an archive opened for storing."
        lstrcpy (tchszMsg, VDBA_MfcResourceString(IDS_E_SERIALIZATION_THREE));
        break;
    case CArchiveException::badIndex:
        //"Serialization error:\nCause: Invalid file format."
        lstrcpy (tchszMsg, VDBA_MfcResourceString(IDS_E_SERIALIZATION_FOUR));
        break;
    case CArchiveException::badClass:
        //"Serialization error:\nCause: Tried to read an object into an object of the wrong type."
        lstrcpy (tchszMsg, VDBA_MfcResourceString(IDS_E_SERIALIZATION_FIVE));
        break;
    case CArchiveException::badSchema:
        //"Serialization error:\nCause: Tried to read an object with a different version of the class."
        lstrcpy (tchszMsg, VDBA_MfcResourceString(IDS_E_SERIALIZATION_SIX));
        break;
    default:
        break;
    }
    if (tchszMsg[0])
        BfxMessageBox (tchszMsg, MB_ICONEXCLAMATION|MB_OK);
}

//
// Global function and external "C" Interface called.
extern "C" void UpdateFontDOMRightPane (HFONT hFont)
{
	HWND hwndCurDoc;
	// get first document handle (with loop to skip the icon title windows)
	hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
	while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
		hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
	
	while (hwndCurDoc) 
	{
		if (QueryDocType(hwndCurDoc) == TYPEDOC_DOM) 
		{
			CWnd *pWnd = CWnd::FromHandlePermanent(hwndCurDoc);
			ASSERT (pWnd);
			if (pWnd) 
			{
				CChildFrame* pFrame = (CChildFrame *)pWnd;
				if (pFrame)
				{
					CMainMfcView2* pDomRightPane = (CMainMfcView2*)pFrame->GetRightPane();
					if (pDomRightPane && pDomRightPane->m_pDomTabDialog && IsWindow (pDomRightPane->m_pDomTabDialog->m_hWnd))
					{
						//
						// Update font of some right panes:
						CWnd* pCurrentPage  = pDomRightPane->m_pDomTabDialog->m_pCurrentPage;
						if (pCurrentPage && IsWindow (pCurrentPage->m_hWnd))
						{
							pCurrentPage->SendMessage (WM_CHANGE_FONT, (WPARAM)0, (LPARAM)hFont);
						}
					}
				}
			}
		}
	
		// get next document handle (with loop to skip the icon title windows)
		hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
		while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
			hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
	}
}


extern "C" int DBETraceDisplayRaisedDBE (HWND hwnd, LPRAISEDDBE pStructRaisedDbe)
{
    return (int)(LRESULT)::SendMessage (hwnd, WM_USER_DBEVENT_TRACE_INCOMING, (WPARAM)0, (LPARAM)pStructRaisedDbe);
}

extern "C" int DBEReplMonDisplayRaisedDBE(int hdl, LPRAISEDREPLICDBE pStructRaisedReplicDbe)
{
/*UKS
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  CMDIChildWnd *pActiveMDI = pMainFrame->MDIGetActive();
  if (pActiveMDI) {
    CWnd *pMDIClient = pActiveMDI->GetParent();
    ASSERT (pMDIClient);

    // get first document handle (with loop to skip the icon title windows)
    CWnd *pCurDoc = pMDIClient->GetWindow(GW_CHILD);
    while (pCurDoc && pCurDoc->GetWindow(GW_OWNER))
      pCurDoc = pCurDoc->GetWindow(GW_HWNDNEXT);
    while (pCurDoc) {
      CMDIChildWnd *pCurFrame = (CMDIChildWnd *)CWnd::FromHandlePermanent(pCurDoc->m_hWnd);
      ASSERT (pCurFrame);
      int type = QueryDocType(pCurDoc->m_hWnd);
      if (type == TYPEDOC_MONITOR) {
        CDocument* pTheDoc = pCurFrame->GetActiveDocument();
        ASSERT (pTheDoc);
        CdIpm* pIpmDoc = (CdIpm*)pTheDoc;
        CfIpm* pIpmFrame = (CfIpm*)pCurFrame;
        CIpmView1* pIpmView1 = (CIpmView1*)pIpmFrame->GetLeftPane();
        ASSERT (pIpmView1);
        if (*(pIpmView1->GetPReplMonHandle()) == hdl) {
          // notify the frame
	        ::SendMessage (pCurFrame->m_hWnd, WM_USER_DBEVENT_TRACE_INCOMING, (WPARAM)0, (LPARAM)pStructRaisedReplicDbe);
        }
      }
      // get next document handle (with loop to skip the icon title windows)
      pCurDoc = pCurDoc->GetWindow(GW_HWNDNEXT);
      while (pCurDoc && pCurDoc->GetWindow(GW_OWNER))
        pCurDoc = pCurDoc->GetWindow(GW_HWNDNEXT);
    }
  }
*/
  return 0L;
}

extern "C" int MFCRefreshData (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return (int)(LRESULT)::SendMessage (hwnd, WM_USER_REFRESH_DATA, wParam, lParam);
}

//
// This function should be called when the cash is modified and affects the 
// DB Event Trace Windows.
// Notify all DB Event Documents that have m_hNode = nHnode and if specify Database 'lpszDBName'
// then only the one that has 'lpszDBName' will be updated.
static void NotifyUpdateDBEvent (int nHnode, LPCTSTR lpszDBName)
{
    CDocument*   pDoc;
    CMainMfcApp* appPtr = (CMainMfcApp*)AfxGetApp ();
    POSITION pos, curTemplatePos = appPtr->GetFirstDocTemplatePosition();
    while(curTemplatePos != NULL)
    {
        CDocTemplate* curTemplate = appPtr->GetNextDocTemplate (curTemplatePos);
        pos = curTemplate->GetFirstDocPosition ();
        while (pos)
        {
            pDoc = curTemplate->GetNextDoc (pos); 
            if(pDoc->IsKindOf (RUNTIME_CLASS (CDbeventDoc)))
            {
                CDbeventDoc* pDbe = (CDbeventDoc*)pDoc;
                if (nHnode == pDbe->m_hNode)
                {
                    if (lpszDBName == NULL)
                        pDoc->UpdateAllViews (NULL, (LPARAM)1);
                    else
                    if (pDbe->m_strDBName.CompareNoCase (lpszDBName) == 0)
                        pDoc->UpdateAllViews (NULL, (LPARAM)1);
                }
            }
        }
    }
}
extern "C" void UpdateDBEDisplay_MT(int hnodestruct, char * dbname)
{
  NotifyUpdateDBEvent(hnodestruct,(LPCTSTR)dbname);
}

extern "C" void UpdateDBEDisplay(int hnodestruct, char * dbname)
{
	if (CanBeInSecondaryThead()){
		UPDATEDBEDISPLAYPARAMS params;
		params.hnodestruct= hnodestruct;
		params.dbname     = dbname;
		Notify_MT_Action(ACTION_UPDATEDBEDISPLAY, (LPARAM) &params);
	}
	else
		UpdateDBEDisplay_MT(hnodestruct,dbname);
}


extern "C" BOOL VDBA_IsAnimateEnabled()
{
	return theApp.IsAnimateEnabled();
}
//
// If bLocked is TRUE:
// call VDBA_EnableAnimate or VDBA_IsAnimateEnabled will block
// untill VDBA_ReleaseAnimateMutex is called:
extern "C" void VDBA_EnableAnimate(BOOL bEnable, BOOL bLocked)
{
	theApp.EnableAnimate(bEnable, bLocked);
}

extern "C" void VDBA_ReleaseAnimateMutex()
{
	theApp.ReleaseAnimateMutex();
}

extern "C" UINT VDBA_GetInterruptType()
{
	return theApp.GetInterruptType();
}

extern "C" void VDBA_SetInterruptType(UINT nType)
{
	theApp.SetInterruptType(nType);
}

extern "C" BOOL HasLoopInterruptBeenRequested()
{
	return (theApp.GetInterruptType()&INTERRUPT_EXIT_LOOP)? TRUE: FALSE;
}

extern "C" int OneWinExactlyGetWinType()
{
	return theApp.GetOneWindowType();
}

//
// Help system
//

void PushHelpId(DWORD helpId)
{
  ASSERT (helpId);
  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT (helpId));
}

void PopHelpId()
{
  lpHelpStack = StackObject_POP (lpHelpStack);
}

int AFXAPI BfxMessageBox(LPCTSTR lpszText, UINT nType, UINT nIDHelp)
{
  PushVDBADlg();
  int res = AfxMessageBox(lpszText, nType, nIDHelp);
  PopVDBADlg();
  return res;
}

int AFXAPI BfxMessageBox(UINT nIDPrompt, UINT nType, UINT nIDHelp)
{
  PushVDBADlg();
  int res = AfxMessageBox(nIDPrompt, nType, nIDHelp);
  PopVDBADlg();
  return res;
}

#define MAX_STRINGARRAY		8


TCHAR * VDBA_MfcResourceString (UINT idString)
{
	static UINT call_counter;
	static CString Strings1[MAX_STRINGARRAY];

	call_counter = call_counter % MAX_STRINGARRAY;
	BOOL bResult = Strings1[call_counter].LoadString(idString);
	if (!bResult) {
		ASSERT (FALSE);		// Added Emb for test facilities
		Strings1[call_counter]= _T("<Load String Error>");
	}

	call_counter++;

	return (TCHAR *)(LPCTSTR)Strings1[call_counter-1];
}

BOOL CMainMfcApp::PreTranslateMessage(MSG* pMsg)
{
	// CG: The following lines were added by the Splash Screen component.
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;

	return CWinApp::PreTranslateMessage(pMsg);
}


void VDBA_OnGeneralSettingChange(CuListCtrl* pListCtrl)
{
	CaGeneralDisplaySetting* pGD = theApp.GetGeneralDisplaySetting();
	ASSERT (pListCtrl);
	if (!pListCtrl)
		return;
	if (!pGD)
		return ;
	pListCtrl->SetLineSeperator (pGD->m_bListCtrlGrid);
	pListCtrl->Invalidate();
}

BOOL VDBA_HasBugs()
{
	return ! IsIngresIISession();
}


BOOL CMainMfcApp::OnIdle(LONG lCount) 
{
  // Default idle management, uses lCount values 0 and 1
  BOOL bNeedMoreIdleTime4BaseClass = CWinApp::OnIdle(lCount);
  if (bNeedMoreIdleTime4BaseClass)
    return TRUE;

  //
  // Custom idle management - lCount may be less than 2, doc not good!
  //

  // global status
  UpdateGlobalStatus();

  // special for dom
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  CMDIChildWnd *pActiveMDI = pMainFrame->MDIGetActive();
  if (pActiveMDI) {
    int type = QueryDocType(pActiveMDI->m_hWnd);
    if (type == TYPEDOC_DOM) {
      ASSERT (pActiveMDI->IsKindOf(RUNTIME_CLASS(CChildFrame)));
      CChildFrame* pActiveDom = (CChildFrame*)pActiveMDI;
      pActiveDom->OnIdleGlobStatusUpdate();
    }
  }
  return FALSE; // Finished custom idle management
}

void CMainMfcApp::EnableAnimate(BOOL bEnable, BOOL bLocked)
{
	DWORD dwWaitResult = WaitForSingleObject (m_hMutexAnimate, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_bAnimateEnabled = bEnable;
		if (!bLocked)
			ReleaseMutex (m_hMutexAnimate);
		break;
	default:
		break;
	}
}

BOOL CMainMfcApp::IsAnimateEnabled()
{
#ifndef MAINWIN
	BOOL bEnable = FALSE;
	DWORD dwWaitResult = WaitForSingleObject (m_hMutexAnimate, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		bEnable = m_bAnimateEnabled;
		ReleaseMutex (m_hMutexAnimate);
		break;
	default:
		break;
	}
	return bEnable;
#else
	return FALSE;
#endif
}

void CMainMfcApp::SetDlgWaitState(BOOL bRunning)
{
	DWORD dwWaitResult = WaitForSingleObject (m_hMutexDlgWaitRunning, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_bDlgWaitRunning = bRunning;
		ReleaseMutex (m_hMutexDlgWaitRunning);
		break;
	default:
		break;
	}
}

BOOL CMainMfcApp::IsDlgWaitRunning()
{
	BOOL bRunning = FALSE;
	DWORD dwWaitResult = WaitForSingleObject (m_hMutexDlgWaitRunning, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		bRunning = m_bDlgWaitRunning;
		ReleaseMutex (m_hMutexDlgWaitRunning);
		break;
	default:
		break;
	}
	return bRunning;
}

void CMainMfcApp::SetInterruptType(UINT nType)
{
	DWORD dwWaitResult = WaitForSingleObject (m_hMutexInterruptType, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		m_nInterruptType = nType;
		ReleaseMutex (m_hMutexInterruptType);
		break;
	default:
		break;
	}
}

UINT CMainMfcApp::GetInterruptType()
{
	UINT nState = INTERRUPT_NOT_ALLOWED;
	DWORD dwWaitResult = WaitForSingleObject (m_hMutexInterruptType, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		nState = m_nInterruptType;
		ReleaseMutex (m_hMutexInterruptType);
		break;
	default:
		break;
	}
	return nState;
}


HICON CMainMfcApp::GetIgresIcon (int nIndex)
{
	return m_ImageListIngresObject.ExtractIcon (nIndex);
}

extern "C" BOOL IsSavePropertyAsDefault()
{
	return theApp.IsSavePropertyAsDefault();
}



extern "C" BOOL ExistSqlQueryAndIpm(LPCTSTR lpVirtNode, LPCTSTR lpszDatabase)
{
	CDocument* pDoc;
	POSITION pos, curTemplatePos = theApp.GetFirstDocTemplatePosition();
	while(curTemplatePos != NULL)
	{
		CDocTemplate* curTemplate = theApp.GetNextDocTemplate (curTemplatePos);
		pos = curTemplate->GetFirstDocPosition ();
		while (pos)
		{
			pDoc = curTemplate->GetNextDoc (pos); 
			if(pDoc->IsKindOf (RUNTIME_CLASS (CdIpm)))
			{
				// Check for ipm.ocx:
				CdIpm* pDocIpm = (CdIpm*)pDoc;
				CuIpm* pCtrl = pDocIpm->GetIpmCtrl();
				if (pCtrl)
				{
					short nConnected = pCtrl->GetConnected(lpVirtNode, lpszDatabase);
					if (nConnected > 0)
						return TRUE;
				}
			}
			else
			if (pDoc->IsKindOf (RUNTIME_CLASS (CdSqlQuery)))
			{
				// Check for SqlQuery.ocx used as MDI Document
				CdSqlQuery* pDocSql = (CdSqlQuery*)pDoc;
				CuSqlQueryControl* pCtrl = pDocSql->GetSqlQueryCtrl();
				if (pCtrl)
				{
					short nConnected = pCtrl->GetConnected(lpVirtNode, lpszDatabase);
					if (nConnected > 0)
						return TRUE;
				}
			}
			else
			if (pDoc->IsKindOf (RUNTIME_CLASS (CMainMfcDoc)))
			{
				// Check for SqlQuery.ocx used as DOM/TABLE/ROWS Right pane:
				CMainMfcDoc* pDomDoc = (CMainMfcDoc*)pDoc;
				CuDlgDomTabCtrl* pTab = pDomDoc->GetTabDialog();
				CWnd* pWnd = pTab->GetCreatedPage(IDD_DOMPROP_TABLE_ROWS);
				if (pWnd && IsWindow(pWnd->m_hWnd))
				{
					CuDlgDomPropTableRows* pPage = (CuDlgDomPropTableRows*)pWnd;
					CuSqlQueryControl& ctrl = pPage->GetSqlQueryControl();
					short nConnected = ctrl.GetConnected(lpVirtNode, lpszDatabase);
					if (nConnected > 0)
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}

