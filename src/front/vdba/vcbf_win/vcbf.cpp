/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcbf.cpp, Implementation file
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Main application header file
**
**  History:
**	xx-Sep-1997 (uk$so01)
**	    Created.
**	20-Apr-2000 (uk$so01)
**	    SIR #101252
**	    When user start the VCBF, and if IVM is already running
**	    then request IVM the start VCBF
**	06-Jun-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	05-Apr-2001 (uk$so01)
**	    (SIR 103548) only allow one vcbf per installation ID
**	17-Sep-2001 (noifr01)
**	    (bug 105758) CreateMutex didn't fail under 98 if it already
**	    existed. only the tests on the windows class and title are
**	    kept for such platforms
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 	01-Nov-2001 (hanje04)
**	    wc.lpfnWndProc should be set to the address of AfxWndProc, stop 
**	    compiler error on Linux.
**  27-May-2002 (noifr01)
**      (sir 107879) have VCBF now work independendly from IVM (IVM now
**      refreshes the list of components in the background, rather than
**      synchronizing with the start and stop of VCBF)
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
**  31-Oct-2003 (noifr01)
**    removed unused test code that was integrated upon massive
**    ingres30->main codeline integration
**  17-Dec-2003 (schph01)
**     SIR #111462, Put string into resource files
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 8-Jun-2004 (lakvi01)
**	  BUG 111984, Corrected the call for VCBFllinit. Define FUTURE_CLUSTER when VCBF
**	  is ported to clusters.
** 29-Jul-2004 (noifr01)
**    (BUG 111984) Corrective fix for previous change:
**    - replaced hard-copied definition of CX_MAX_NODE_NAME_LEN by
**      inclusion of <cx.h>
**    - updated the flow of the program if argc!=1 so that the behavior in this
**      situation remains identical to what it used to be
**/

#include "stdafx.h"
#include "vcbf.h"
extern "C"{
#define INGRESII
#ifndef MAINWIN
#define DESKTOP
#define NT_GENERIC
#define IMPORT_DLL_DATA
#endif
#include <compat.h>
#include <cs.h>
#include <pc.h>
#include <cx.h>
}

#include "mainfrm.h"
#include "vcbfdoc.h"
#include "vcbfview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CVcbfApp



BEGIN_MESSAGE_MAP(CVcbfApp, CWinApp)
	//{{AFX_MSG_MAP(CVcbfApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVcbfApp construction

CVcbfApp::CVcbfApp()
{
	m_pConfListViewLeft = NULL;
	m_hMutexSingleInstance = NULL;
	m_bInitData = FALSE;
	m_strII_INSTALLATION = INGRESII_QueryInstallationID();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVcbfApp object

CVcbfApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CVcbfApp initialization


 


BOOL CVcbfApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	//
	// Initialize the CBF Data
	try
	{
		CString strTitle;
		CString strWndName;
		WNDCLASS wc;
 
		memset (&wc, 0, sizeof(wc));
		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpszClassName = _T("VCBF");
		wc.lpfnWndProc = &AfxWndProc;
		wc.hInstance = AfxGetInstanceHandle();
		wc.hIcon = LoadIcon(IDR_MAINFRAME);
		wc.hCursor = LoadStandardCursor(IDC_ARROW);
		wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
		wc.lpszMenuName = NULL;

		if (!AfxRegisterClass (&wc))
			return FALSE;

		//
		// To require that VCBF accept the -stop command line
		// Change the instruction bellow to "CString strCmdLine = m_lpCmdLine;"
		CString strCmdLine = _T(""); // m_lpCmdLine;

		CString strNamedMutex = _T("INGRESxVCBFxSINGLExINSTANCE");
		strNamedMutex +=  GetCurrentInstallationID();
		strTitle.LoadString (AFX_IDS_APP_TITLE);
		strTitle += GetCurrentInstallationID();

		BOOL bCreateMutex = TRUE;
#ifndef MAINWIN
		OSVERSIONINFO vsinfo;
		vsinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (GetVersionEx (&vsinfo)) {
			if (vsinfo.dwPlatformId == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
			bCreateMutex = FALSE;
		}
#endif
		if (bCreateMutex)
			m_hMutexSingleInstance = CreateMutex (NULL, NULL, strNamedMutex);
		if (!bCreateMutex || GetLastError() == ERROR_ALREADY_EXISTS)
		{
			//
			// An instance of VCBF already exists:
			CWnd* pWndPrev = CWnd::FindWindow(_T("VCBF"), strTitle);
			if (pWndPrev)
			{
				//
				// Code copied from IVM for facility of maintenance.
				// VCBF -STOP will apply ONLY IF the line obove strCmdLine is defined as 
				// "CString strCmdLine = m_lpCmdLine";
				if (strCmdLine.CompareNoCase (_T("-stop")) == 0 || strCmdLine.CompareNoCase (_T("/stop")) == 0)
				{
					//
					// Only exit vcbf:
					pWndPrev->SendMessage (WMUSRMSG_APP_SHUTDOWN, 0, 0);
					return FALSE;
				}
				//_T("An instance of Visual CBF is already running.");
				// Activate the previous application:
				::SendMessage (pWndPrev->m_hWnd, WMUSRMSG_ACTIVATE, 0, 0);
				::SetForegroundWindow (pWndPrev->m_hWnd);
				return FALSE;
			}
		}
		//
		// Code copied from IVM for facility of maintenance.
		// VCBF -STOP will apply ONLY IF the line obove strCmdLine is defined as 
		// "CString strCmdLine = m_lpCmdLine";
		if (strCmdLine.CompareNoCase (_T("-stop")) == 0 || strCmdLine.CompareNoCase (_T("/stop")) == 0)
			return FALSE;
		
		//
		// Normal process of Starting VCBF:
#ifdef FUTURE_CLUSTER
		char strHostName[CX_MAX_NODE_NAME_LEN]={0};

		if(__argc == 1)
		{
#endif
			if (!VCBFllinit(NULL))
			{
				AfxMessageBox (IDS_ERR_INITIALIZE,  MB_ICONSTOP | MB_OK);
				return FALSE;
			}
#ifdef FUTURE_CLUSTER
		}
		else if(__argc == 2)
		{	
			strcpy(strHostName,__argv[1]);
			if (!VCBFllinit(strHostName))
			{
				AfxMessageBox (IDS_ERR_INITIALIZE,  MB_ICONSTOP | MB_OK);
				return FALSE;
			}
		}

		else
		{
			AfxMessageBox( IDS_USAGE_ERROR, MB_ICONERROR | MB_OK);
			return FALSE;
		}
#endif
		m_bInitData = TRUE;
		/* No need in .NET
		#ifdef _AFXDLL
			Enable3dControls();			// Call this when using MFC in a shared DLL
		#else
			Enable3dControlsStatic();	// Call this when linking to MFC statically
		#endif
		*/

		LoadStdProfileSettings();  // Load standard INI file options (including MRU)
		
		// Register the application's document templates.  Document templates
		//  serve as the connection between documents, frame windows and views.
		
		CSingleDocTemplate* pDocTemplate;
		pDocTemplate = new CSingleDocTemplate(
			IDR_MAINFRAME,
			RUNTIME_CLASS(CVcbfDoc),
			RUNTIME_CLASS(CMainFrame),       // main SDI frame window
			RUNTIME_CLASS(CVcbfView));
		AddDocTemplate(pDocTemplate);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CVcbfApp::InitInstance has caught exception: %s\n", e.m_strReason);
		return FALSE;
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
		return FALSE;
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
		return FALSE;
	}
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
#ifdef FUTURE_CLUSTER
	cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;
#endif

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CVcbfApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CVcbfApp commands

LRESULT CVcbfApp::ProcessWndProcException(CException* e, const MSG* pMsg) 
{
	TRACE0 ("CVcbfApp::ProcessWndProcException(CException* e, const MSG* pMsg)...\n");
	return CWinApp::ProcessWndProcException(e, pMsg);
}

int CVcbfApp::ExitInstance() 
{
	if (m_hMutexSingleInstance)
		CloseHandle (m_hMutexSingleInstance);
	if (m_bInitData)
		VCBFllterminate();
	return CWinApp::ExitInstance();
}



//----------------------------------------------------------------------------------------
//                                HELPER FUNCTION SECTION
//                               *************************

//
// DO NOT put this string in the RC file !
static TCHAR tchszOutOfMemoryMessage [] = _T("Low of Memory ...\nCannot allocate memory, please close some applications !");
void VCBF_OutOfMemoryMessage()
{
	AfxMessageBox (tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
}

//
// Helper functions for the Generic Control.
// ---------------------------------------
void VCBF_GenericAddItem (CuEditableListCtrlGeneric* pList, GENERICLINEINFO* pData, int iIndex)
{
	CWaitCursor waitCursor;
	ASSERT (pList);
	ASSERT (iIndex != -1);
	int index = pList->InsertItem (iIndex, (LPTSTR)pData->szname);
	if (index == -1)
		return;
	pList->HideProperty();
	if (pData->iunittype == GEN_LINE_TYPE_BOOL)
	{
		if (pData->szvalue[0] && _tcsicmp (_T("ON"), (LPCTSTR)pData->szvalue) == 0)
			pList->SetCheck(iIndex, 1, TRUE);
		else 
			pList->SetCheck(iIndex, 1, FALSE);
	}
	else
		pList->SetItemText(iIndex, 1, (LPTSTR)pData->szvalue);
	pList->SetItemText(iIndex, 2, (LPTSTR)pData->szunit);
	GENERICLINEINFO* lData = new GENERICLINEINFO;
	memcpy (lData, pData, sizeof(GENERICLINEINFO));
	pList->SetItemData (iIndex, (DWORD_PTR)lData);
}


void VCBF_GenericAddItems (CuEditableListCtrlGeneric* pList)
{
	CWaitCursor waitCursor;
	ASSERT (pList);
	int i, index, nCount = pList->GetItemCount();

	BOOL bContinue = TRUE;
	GENERICLINEINFO  data;
	pList->HideProperty ();
	//
	// Test if we need to do so.
	if (VCBFllGenLinesRead())
	{
		//
		// If there is an old selected item, then deselect it
		// and reselected it again, this has an effect to call the OnItemChanged of
		// CListCtrl to Enable/Disable the buttons.
		UINT nState;
		for (i=0; i<nCount; i++)
		{
			nState = pList->GetItemState (i, LVIS_SELECTED);
			if (nState & LVIS_SELECTED)
			{
				pList->SetItemState (i, 0, LVIS_SELECTED);
				pList->SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
			}
		}
		return;
	}

	VCBF_GenericDestroyItemData (pList);
	pList->DeleteAllItems();
	memset (&data, 0, sizeof (data));
	try
	{
		bContinue = VCBFllGetFirstGenericLine(&data);
		while (bContinue)
		{
			nCount = pList->GetItemCount();
			//
			// Main Item: col 0
			index = pList->InsertItem (nCount, (LPTSTR)data.szname);
			if (index == -1)
				return;
			//
			// Value: col 1
			if (data.iunittype == GEN_LINE_TYPE_BOOL)
			{
				if (&data.szvalue[0] && _tcsicmp (_T("ON"), (LPCTSTR)data.szvalue) == 0)
					pList->SetCheck(nCount, 1, TRUE);
				else 
					pList->SetCheck(nCount, 1, FALSE);
			}
			else
				pList->SetItemText(nCount, 1, (LPTSTR)data.szvalue);
			//
			// Unit: col 2
			pList->SetItemText(nCount, 2, (LPTSTR)data.szunit);
			GENERICLINEINFO* lData = new GENERICLINEINFO;
			memcpy (lData, &data, sizeof(GENERICLINEINFO));
			pList->SetItemData (index, (DWORD_PTR)lData);
		
			bContinue = VCBFllGetNextGenericLine(&data);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("VCBF_GenericAddItems has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}

void VCBF_GenericSetItem (CuEditableListCtrlGeneric* pList, GENERICLINEINFO* pData, int iIndex)
{
	CWaitCursor waitCursor;
	ASSERT (pList);
	ASSERT (iIndex != -1);
	if (pData->iunittype == GEN_LINE_TYPE_BOOL)
	{
		if (pData->szvalue[0] && _tcsicmp (_T("ON"), (LPCTSTR)pData->szvalue) == 0)
			pList->SetCheck(iIndex, 1, TRUE);
		else 
			pList->SetCheck(iIndex, 1, FALSE);
	}
	else
		pList->SetItemText(iIndex, 1, (LPTSTR)pData->szvalue);
	pList->SetItemText(iIndex, 2, (LPTSTR)pData->szunit);
}


void VCBF_GenericDestroyItemData (CuEditableListCtrlGeneric* pList)
{
	CWaitCursor waitCursor;
	GENERICLINEINFO* lData = NULL;
	int i, nCount = pList->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		lData = (GENERICLINEINFO*)pList->GetItemData (i);	
		if (lData)
			delete lData;
	}
}


void VCBF_CreateControlGeneric (CuEditableListCtrlGeneric& ctrl, CWnd* pParent, CImageList* pImHeight, CImageList* pImCheck)
{
	CWaitCursor waitCursor;
	const int GLAYOUT_NUMBER = 3;
	VERIFY (ctrl.SubclassDlgItem (IDC_LIST1, pParent));
	LONG style = GetWindowLong (ctrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (ctrl.m_hWnd, GWL_STYLE, style);
	pImHeight->Create(1, 20, TRUE, 1, 0);
	ctrl.SetImageList (pImHeight, LVSIL_SMALL);

	pImCheck->Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	ctrl.SetCheckImageList(pImCheck);

	//
	// Initalize the Column Header of CListCtrl
	//
	LV_COLUMN lvcolumn;
	UINT strHeaderID[GLAYOUT_NUMBER] = { IDS_COL_PARAMETER, IDS_COL_VALUE,IDS_COL_UNIT};
	int  i, hWidth  [GLAYOUT_NUMBER] = {140, 80, 167};
	//
	// Set the number of columns: LAYOUT_NUMBER
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	for (i=0; i<GLAYOUT_NUMBER; i++)
	{
		lvcolumn.fmt = LVCFMT_LEFT;
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		(i==1) ? ctrl.InsertColumn (i, &lvcolumn, TRUE): ctrl.InsertColumn (i, &lvcolumn); 
	}
}


void VCBF_GenericResetValue (CuEditableListCtrlGeneric& ctrl, int iIndex)
{
	GENERICLINEINFO* pData = (GENERICLINEINFO*)ctrl.GetItemData(iIndex);
	try
	{
		if (!pData)
			return;
		VCBFllResetGenValue (pData);
		VCBF_GenericSetItem (&ctrl, pData, iIndex);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("VCBF_GenericResetValue caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}

//
// Helper function for the Generic Derived Control.
// ------------------------------------------------

void VCBF_GenericDerivedAddItem (CuEditableListCtrlGenericDerived* pList, LPCTSTR lpszCheckItem, BOOL bCache)
{
	CWaitCursor waitCursor;
	int  iIndex    = 0;
	BOOL bContinue = TRUE;
	DERIVEDLINEINFO  data;
//	pList->HideProperty ();
	VCBF_GenericDerivedDestroyItemData (pList);
	pList->DeleteAllItems();
	memset (&data, 0, sizeof (data));
	try
	{
		if (!VCBFllInitDependent(bCache))
			return;
		bContinue = VCBFllGetFirstDerivedLine(&data);
		while (bContinue)
		{
			//
			// Main Item: col 0
			int index = pList->InsertItem (iIndex, (LPTSTR)data.szname);
			if (index == -1)
				return;
			//
			// Value: col 1
			if (data.iunittype == GEN_LINE_TYPE_BOOL)
			{
				if (&data.szvalue[0] && _tcsicmp (_T("ON"), (LPCTSTR)data.szvalue) == 0)
					pList->SetCheck(iIndex, 1, TRUE);
				else 
					pList->SetCheck(iIndex, 1, FALSE);
			}
			else
				pList->SetItemText(iIndex, 1, (LPTSTR)data.szvalue);
			//
			// Unit: col 2
			pList->SetItemText(iIndex, 2, (LPTSTR)data.szunit);
			//
			// Protected: col 3
			BOOL bCheck = (&data.szprotected[0] && _tcsicmp ((LPCTSTR)data.szprotected, _T("yes")) == 0)? TRUE: FALSE;
			pList->SetCheck(iIndex, 3, bCheck);
			
			DERIVEDLINEINFO* lData = new DERIVEDLINEINFO;
			memcpy (lData, &data, sizeof(DERIVEDLINEINFO));
			pList->SetItemData (iIndex, (DWORD_PTR)lData);
			
			bContinue = VCBFllGetNextDerivedLine(&data);
			iIndex++;
		}
		if (!lpszCheckItem)
			return;
		int i, nCount = pList->GetItemCount();
		CString strItem;
		for (i=0; i<nCount; i++)
		{
			strItem = pList->GetItemText (i, 0);
			if (strItem == lpszCheckItem)
			{
				pList->SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
				break;
			}
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("VCBF_GenericDerivedAddItem has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}


void VCBF_GenericDerivedSetItem (CuEditableListCtrlGenericDerived* pList, DERIVEDLINEINFO* pData, int iIndex)
{
	CWaitCursor waitCursor;
	ASSERT (pList);
	ASSERT (iIndex != -1);
	//
	// Value: col 1
	if (pData->iunittype == GEN_LINE_TYPE_BOOL)
	{
		if (pData->szvalue[0] && _tcsicmp (_T("ON"), (LPCTSTR)pData->szvalue) == 0)
			pList->SetCheck(iIndex, 1, TRUE);
		else 
			pList->SetCheck(iIndex, 1, FALSE);
	}
	else
		pList->SetItemText(iIndex, 1, (LPTSTR)pData->szvalue);
	//
	// Unit: col 2
	pList->SetItemText(iIndex, 2, (LPTSTR)pData->szunit);
	//
	// Protected: col 3
	BOOL bCheck = (pData->szprotected[0] && _tcsicmp ((LPCTSTR)pData->szprotected, _T("ON")) == 0)? TRUE: FALSE;
	pList->SetCheck(iIndex, 3, bCheck);
}


void VCBF_GenericDerivedDestroyItemData (CuEditableListCtrlGenericDerived* pList)
{
	CWaitCursor waitCursor;
	DERIVEDLINEINFO* lData = NULL;
	int i, nCount = pList->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		lData = (DERIVEDLINEINFO*)pList->GetItemData (i);	
		if (lData)
			delete lData;
	}
}


void VCBF_CreateControlGenericDerived (CuEditableListCtrlGenericDerived& ctrl, CWnd* pParent, CImageList* pImHeight, CImageList* pImCheck)
{
	CWaitCursor waitCursor;
	const int GDLAYOUT_NUMBER = 4;
	VERIFY (ctrl.SubclassDlgItem (IDC_LIST1, pParent));
	LONG style = GetWindowLong (ctrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (ctrl.m_hWnd, GWL_STYLE, style);
	pImHeight->Create(1, 20, TRUE, 1, 0);
	pImCheck->Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));

	ctrl.SetImageList (pImHeight, LVSIL_SMALL);
	ctrl.SetCheckImageList(pImCheck);

	//
	// Initalize the Column Header of CListCtrl
	//
	LV_COLUMN lvcolumn;
	UINT       strHeaderID[GDLAYOUT_NUMBER] = { IDS_COL_PARAMETER, IDS_COL_VALUE, IDS_COL_UNIT, IDS_COL_PROTECTED};
	int       i, hWidth   [GDLAYOUT_NUMBER] = {100, 80, 80, 90};
	//
	// Set the number of columns: GDLAYOUT_NUMBER
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<GDLAYOUT_NUMBER; i++)
	{
		lvcolumn.fmt = (i == (GDLAYOUT_NUMBER-1))? LVCFMT_CENTER: LVCFMT_LEFT;
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		if (i == (GDLAYOUT_NUMBER-1) || i == 1)
			ctrl.InsertColumn (i, &lvcolumn, TRUE); 
		else
			ctrl.InsertColumn (i, &lvcolumn); 
	}
}

void VCBF_GenericDerivedRecalculate (CuEditableListCtrlGenericDerived& ctrl, int iIndex, BOOL bCache)
{
	DERIVEDLINEINFO* pData = (DERIVEDLINEINFO*)ctrl.GetItemData(iIndex);
	try
	{
		if (pData) 
		{
			VCBFllDepOnRecalc (pData);
			//
			// Refresh data
			CString strSelectItem = ctrl.GetItemText (iIndex, 0);
			VCBF_GenericDerivedAddItem (&ctrl, (LPCTSTR)strSelectItem, bCache);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("VCBF_GenericDerivedRecalculate has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}

BOOL bExitRequested=FALSE;
BOOL VCBFRequestExit()
{
    bExitRequested=TRUE;
	throw CeVcbfException();
	return TRUE;
}







//----------------------------------------------------------------------------------------//
//                               END OF HELPER FUNCTION SECTION                           //
//----------------------------------------------------------------------------------------//

