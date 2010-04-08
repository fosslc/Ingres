/*
**  Copyright (c) 2006 Ingres Corporation.
*/

/*
**  Name: IngClean.cpp : defines the class behaviors for the application
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
**	04-Nov-2004 (penga03)
**	    Updated the registry key and shortcut folder.
**	07-dec-2004 (penga03)
**	    Changed the formulation to generate GUIDs since the old one does
**	    not work for id from A1 to A9.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	07-May-2009 (drivi01)
**	    Update the way application is initialized in effort
**	    to port to Visual Studio 2008.
*/

#include "stdafx.h"
#include "IngClean.h"
#include "IngCleanDlg.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DWORD Try01()
{
	char cmd[MAX_PATH];

	sprintf(cmd, "MsiExec.exe /qn /x %s", theUninst.m_productcode);
	Execute(cmd);

	char szKey[512];
	HKEY hKey=0;

	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", 
		theUninst.m_productcode);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
		return 0;
	else
		return 1;
}

DWORD Try02()
{
	MSIHANDLE hDb, hView, hView2, hRecord;
	char szQuery[512], cmd[MAX_PATH];
	
	if (!MsiOpenDatabase(theUninst.m_msidbpath, MSIDBOPEN_DIRECT, &hDb))
	{
		sprintf(szQuery, "SELECT DISTINCT Action FROM CustomAction");
		if (!MsiDatabaseOpenView(hDb, szQuery, &hView))
		{
			if (!MsiViewExecute(hView, 0))
			{
				while (!MsiViewFetch(hView, &hRecord))
				{
					char szBuf[512];
					DWORD dwSize=sizeof(szBuf);
					
					MsiRecordGetString(hRecord, 1, szBuf, &dwSize);
					if (!strstr(szBuf, "ingres_") && !strstr(szBuf, "embedingres_"))
						continue;

					sprintf(szQuery, "DELETE FROM InstallExecuteSequence WHERE Action = '%s'", szBuf);
					MsiDatabaseOpenView(hDb, szQuery, &hView2);
					if (!MsiViewExecute(hView2, 0))
						theUninst.m_logbuf.Format("custom action: %s...done", szQuery);
					else
						theUninst.m_logbuf.Format("custom action: %s...failed", szQuery);
					theUninst.AppendToLog(theUninst.m_logbuf);
					MsiCloseHandle(hView2);
				}
				MsiCloseHandle(hRecord);
			}
			MsiCloseHandle(hView);
		}
		MsiDatabaseCommit(hDb);
		MsiCloseHandle(hDb);
	}
	
	sprintf(cmd, "MsiExec.exe /qn /x %s", theUninst.m_productcode);
	Execute(cmd);

	char szKey[512];
	HKEY hKey=0;

	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", 
		theUninst.m_productcode);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
		return 0;
	else
		return 1;
}

DWORD Try03()
{
	theUninst.ThreadUninstall();

	char szKey[512];
	HKEY hKey=0;

	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", 
		theUninst.m_productcode);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
		return 0;
	else
		return 1;
}
/////////////////////////////////////////////////////////////////////////////
// CIngCleanApp

BEGIN_MESSAGE_MAP(CIngCleanApp, CWinApp)
	//{{AFX_MSG_MAP(CIngCleanApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIngCleanApp construction

CIngCleanApp::CIngCleanApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CIngCleanApp object

CIngCleanApp theApp;
CUninst theUninst;

/////////////////////////////////////////////////////////////////////////////
// CIngCleanApp initialization

BOOL CIngCleanApp::InitInstance()
{

/*	// InitCommonControlsEx() is required on Windows XP if an application
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

	CString title;
	
	title.LoadString(IDS_TITLE);
	
	if (FindWindow(NULL, title))
	{
		Message(IDS_ALREADYRUNNING);
		return TRUE;
	}
	
	CString id, temp;
	int	 idx;
	char szKey[512], reg[33];
	HKEY hKey=0;
	char cmd[MAX_PATH];
	HANDLE hThread;
	DWORD ExitCode, ThreadId;

	theUninst.m_silent = FALSE;
	theUninst.m_trymsiexec = FALSE;

	if (!(*m_lpCmdLine))
	{
		Message(IDS_INGCLEANUSAGE);
		return TRUE;
	}

	// parse the command line
	CCommandLineInfoEx cmdinfo;
	ParseCommandLine(cmdinfo);
	if(cmdinfo.GetOption("q"))
		theUninst.m_silent = TRUE;
	if(cmdinfo.GetOption("t"))
		theUninst.m_trymsiexec = TRUE;
	cmdinfo.GetOption("id", id);

	// get installation identifier
	if (id.GetLength() != 2 
		|| !isalpha(id.GetAt(0)) || !isalnum(id.GetAt(1)))
	{
		Error(IDS_INVALIDID, id);
		return TRUE;
	}
	sprintf(theUninst.m_id, "%s", _strupr(id.GetBuffer(3)));
	theUninst.m_logbuf.Format("Installation ID: %s", theUninst.m_id);
	theUninst.AppendToLog(theUninst.m_logbuf);

	// get II_SYSTEM
	sprintf(szKey, 
		"SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", 
		theUninst.m_id);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		DWORD dwType, dwSize=sizeof(theUninst.m_ii_system);

		RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, (BYTE *)theUninst.m_ii_system, &dwSize);
		RegCloseKey(hKey);
	}
	if (!(*theUninst.m_ii_system))
	{
		Error(IDS_NOIISYSTEM, theUninst.m_id);
		return TRUE;
	}
	theUninst.m_logbuf.Format("II_SYSTEM: %s", theUninst.m_ii_system);
	theUninst.AppendToLog(theUninst.m_logbuf);

	// get product code and upgrade code
	idx = (toupper(theUninst.m_id[0]) - 'A') * 26 + toupper(theUninst.m_id[1]) - 'A';
	if (idx <= 0)
		idx = (toupper(theUninst.m_id[0]) - 'A') * 26 + toupper(theUninst.m_id[1]) - '0';

	switch (idx)
	{
	case 216: //II
		sprintf(theUninst.m_productcode, "{A78D00D8-2979-11D5-BDFA-00B0D0AD4485}");
		sprintf(theUninst.m_upgradecode, "{500E20E5-2D6C-11D5-BDFC-00B0D0AD4485}");
		break;
	default:
		sprintf(theUninst.m_productcode, "{A78D%04X-2979-11D5-BDFA-00B0D0AD4485}", idx);
		sprintf(theUninst.m_upgradecode, "{A78B%04X-2979-11D5-BDFA-00B0D0AD4485}", idx);
	}
	theUninst.m_logbuf.Format("ProductCode: %s", theUninst.m_productcode);
	theUninst.AppendToLog(theUninst.m_logbuf);
	theUninst.m_logbuf.Format("UpgradeCode: %s", theUninst.m_upgradecode);
	theUninst.AppendToLog(theUninst.m_logbuf);

	// check if the instance exists, i.e. registered by msi
	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", 
		theUninst.m_productcode);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		Error(IDS_NOPRODUCT, theUninst.m_id);
		return TRUE;
	}

	// get the cashed msi package
	GUID_TO_REG(theUninst.m_productcode, reg);

	*theUninst.m_msidbpath = 0;
	sprintf(szKey, "SOFTWARE\\Microsoft\\Windows\\Currentversion\\Installer\\UserData\\S-1-5-18\\Products\\%s\\InstallProperties", reg);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		DWORD dwType, dwSize=sizeof(theUninst.m_msidbpath);

		RegQueryValueEx(hKey, "LocalPackage", NULL, &dwType, (BYTE *)theUninst.m_msidbpath, &dwSize);
		RegCloseKey(hKey);
	}
	if (!(*theUninst.m_msidbpath))
	{
		Error(IDS_NOMSIDB);
		return TRUE;
	}
	theUninst.m_logbuf.Format("LocalPackage: %s", theUninst.m_msidbpath);
	theUninst.AppendToLog(theUninst.m_logbuf);

	// get shortcuts information
	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", 
		theUninst.m_productcode);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		char szBuf[2];
		DWORD dwType, dwSize;

		*szBuf = 0;
		dwSize = sizeof(szBuf);
		if (!RegQueryValueEx(hKey, "IngresCreateIcons", NULL, &dwType, (BYTE *)szBuf, &dwSize) 
			&& !_stricmp(szBuf, "1"))
			theUninst.m_createicons = 1;
		else
			theUninst.m_createicons = 0;

		*szBuf = 0;
		dwSize = sizeof(szBuf);
		if (!RegQueryValueEx(hKey, "IngresAllUsers", NULL, &dwType, (BYTE *)szBuf, &dwSize) 
			&& !_stricmp(szBuf, "1"))
			theUninst.m_allusers = 1;
		else
			theUninst.m_allusers = 0;
	}

	// shut down ingres and ivm
	if (IngresAlreadyRunning(theUninst.m_ii_system))
	{
		if (AskUserYN(IDS_INGRESRUNNING))
		{	
			sprintf(cmd, "\"%s\\ingres\\bin\\ingwrap.exe\" \"%s\\ingres\\bin\\winstart\" /stop", 
				theUninst.m_ii_system, theUninst.m_ii_system);
			if (!Execute(cmd, 0)) return TRUE;
			while (!WinstartRunning(theUninst.m_ii_system)) 
				Sleep(250);
			while (WinstartRunning(theUninst.m_ii_system)) 
				Sleep(250);
		}
		else 
			return TRUE;
	}

	sprintf(cmd, "\"%s\\ingres\\bin\\ingwrap.exe\" \"%s\\ingres\\bin\\ivm\" /stop", 
		theUninst.m_ii_system, theUninst.m_ii_system);
	Execute(cmd);

	if (theUninst.m_trymsiexec)
	{
		// 1st try
		/*temp.LoadString(IDS_TRY01);
		CWaitDlg dlgWait(temp);
		hThread=::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Try01, 0, 0, &ThreadId);
		dlgWait.m_hThread=hThread;
		dlgWait.DoModal();
		GetExitCodeThread(hThread, &ExitCode);
		if(!ExitCode)
		{
			Message(IDS_SUCCEEDED, theUninst.m_id);
			return FALSE;
		}*/
		
		// 2nd try
		/* List DATA, CKP, DMP, JNL, WORK and LOG. */
		CString s, k;

		if (Local_NMgtIngAt(theUninst.m_ii_system, "II_DATABASE", s))
		{
			theUninst.ListAllFiles(s + "\\ingres\\data");
			theUninst.m_dirs.AddTail(s + "\\ingres");
			theUninst.m_dirs.AddTail(s);
		}
		if (Local_NMgtIngAt(theUninst.m_ii_system, "II_CHECKPOINT", s))
		{
			theUninst.ListAllFiles(s + "\\ingres\\ckp");
			theUninst.m_dirs.AddTail(s + "\\ingres");
			theUninst.m_dirs.AddTail(s);
		}
		if (Local_NMgtIngAt(theUninst.m_ii_system, "II_JOURNAL", s))
		{
			theUninst.ListAllFiles(s + "\\ingres\\jnl");
			theUninst.m_dirs.AddTail(s + "\\ingres");
			theUninst.m_dirs.AddTail(s);
		}
		if (Local_NMgtIngAt(theUninst.m_ii_system, "II_DUMP", s))
		{
			theUninst.ListAllFiles(s + "\\ingres\\dmp");
			theUninst.m_dirs.AddTail(s + "\\ingres");
			theUninst.m_dirs.AddTail(s);
		}
		if (Local_NMgtIngAt(theUninst.m_ii_system, "II_WORK", s))
		{
			theUninst.ListAllFiles(s + "\\ingres\\work");
			theUninst.m_dirs.AddTail(s + "\\ingres");
			theUninst.m_dirs.AddTail(s);
		}
		
		k.Format("ii.%s.rcp.log.log_file_parts", theUninst.m_host);
		if (Local_PMget(theUninst.m_ii_system, k, s))
		{
			int num_logs, i;
			
			num_logs = atoi(s);
			for (i = 1; i <= num_logs; ++i)
			{
				k.Format("ii.%s.rcp.log.log_file_%d", theUninst.m_host, i);
				if (Local_PMget(theUninst.m_ii_system, k, s))
				{
					theUninst.ListAllFiles(s + "\\ingres\\log");
					theUninst.m_dirs.AddTail(s + "\\ingres");
					theUninst.m_dirs.AddTail(s);
				}
			}
		}
		
		k.Format("ii.%s.rcp.log.dual_log_1", theUninst.m_host);
		if (Local_PMget(theUninst.m_ii_system, k, s))
		{
			theUninst.ListAllFiles(s + "\\ingres\\log");
			theUninst.m_dirs.AddTail(s + "\\ingres");
			theUninst.m_dirs.AddTail(s);
		}
		
		temp.LoadString(IDS_TRY02);
		CWaitDlg dlgWait2(temp);
		hThread=::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Try02, 0, 0, &ThreadId);
		dlgWait2.m_hThread=hThread;
		dlgWait2.DoModal();
		GetExitCodeThread(hThread, &ExitCode);
		if(!ExitCode)
		{
			sprintf(szKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", theUninst.m_id);
			SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
			sprintf(szKey, "SOFTWARE\\IngresCorporation\\Ingres");
			SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, szKey);
			sprintf(szKey, "SOFTWARE\\IngresCorporation");
			SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, szKey);
			DeleteFile(theUninst.m_msidbpath);
			theUninst.RemoveFiles();
			theUninst.RemoveFolders();

			Message(IDS_SUCCEEDED, theUninst.m_id);
			return FALSE;
		}
	}

	// finally
	if (theUninst.m_silent)
	{
		temp.LoadString(IDS_TRY03);
		CWaitDlg dlgWait3(temp);
		hThread=::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Try03, 0, 0, &ThreadId);
		dlgWait3.m_hThread=hThread;
		dlgWait3.DoModal();
		GetExitCodeThread(hThread, &ExitCode);
		if(!ExitCode){
			Message(IDS_SUCCEEDED, theUninst.m_id);
			return FALSE;
		}
		else {
			Message(IDS_NOTSUCCESSFULL);
			return TRUE;
		}
	}

	CIngCleanDlg dlg1;
	m_pMainWnd = &dlg1;
	dlg1.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// self-defined functions

static UINT 
MyMessageBox(UINT uiStrID, UINT uiFlags, LPCTSTR param)
{
	CString title; title.LoadString(IDS_TITLE);
	CString text; 
	if (param)
		text.Format(uiStrID,param);
	else
		text.LoadString(uiStrID);
	UINT ret=::MessageBox( (theApp.m_pMainWnd && theApp.m_pMainWnd->m_hWnd ) ? theApp.m_pMainWnd->m_hWnd : GetActiveWindow(),text,title,uiFlags);
	return ret;
}

BOOL 
AskUserYN(UINT uiStrID, LPCTSTR param)
{
	return (MyMessageBox(uiStrID,MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2,param)==IDYES);
}

void 
Error(UINT uiStrID,LPCTSTR param)
{
	MyMessageBox(uiStrID,MB_OK|MB_ICONERROR,param);
}

void 
Message(UINT uiStrID,LPCTSTR param)
{
	MyMessageBox(uiStrID,MB_OK|MB_ICONINFORMATION,param);
}

BOOL 
Execute(char *cmd, BOOL bWait)
{
	theUninst.m_logbuf.Format("Execute command: %s", cmd);
	theUninst.AppendToLog(theUninst.m_logbuf);

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD ExitCode =0;
	
	memset((char *)&pi, 0, sizeof(pi));
	memset((char *)&si, 0, sizeof(si));
	si.cb=sizeof(si);
	si.dwFlags=STARTF_USESHOWWINDOW;
	si.wShowWindow=SW_HIDE;
	
	if(!CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{		
		theUninst.m_logbuf.Format("Failed launch child process! GetLastError() returns: %d", GetLastError());
		theUninst.AppendToLog(theUninst.m_logbuf);
		return 0;
	}
	
	if (!bWait)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 1;
	}
	
	WaitForSingleObject(pi.hProcess, INFINITE);
	if (GetExitCodeProcess(pi.hProcess, &ExitCode))
	{
		switch(ExitCode)
		{
		case STILL_ACTIVE: 
			theUninst.m_logbuf.Format("Process is still active!");
			theUninst.AppendToLog(theUninst.m_logbuf);
			break;
		default:
			theUninst.m_logbuf.Format("Exit code= %d", ExitCode);
			theUninst.AppendToLog(theUninst.m_logbuf);
			break;
		}
	}
	else
	{
		theUninst.m_logbuf.Format("GetExitCodeProcess() failed! GetLastError() returns: %d", GetLastError());
		theUninst.AppendToLog(theUninst.m_logbuf);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (ExitCode == 0);
}

BOOL 
IngresAlreadyRunning(char *ii_system)
{
	CString csExe, csTemp;

    csExe.Format("%s\\ingres\\bin\\iigcn.exe", ii_system);
	csTemp.Format("%s\\ingres\\bin\\temp.exe", ii_system);
	if(!CopyFile(csExe, csTemp, FALSE)) /* iigcn doesn't exist */
		return FALSE;
	if(DeleteFile(csExe))/* not running */
	{	
		CopyFile(csTemp, csExe, FALSE);
		DeleteFile(csTemp);
		return FALSE;
	}
	else /* running */
	{
		DeleteFile(csTemp);
		return TRUE;
	}
}

BOOL 
WinstartRunning(char *ii_system)
{
	CString csExe, csTemp;

    csExe.Format("%s\\ingres\\bin\\winstart.exe", ii_system);
	csTemp.Format("%s\\ingres\\bin\\temp.exe", ii_system);
	if(!CopyFile(csExe, csTemp, FALSE))
		return FALSE;
	if(DeleteFile(csExe))
	{	
		CopyFile(csTemp, csExe, FALSE);
		DeleteFile(csTemp);
		return FALSE;
	}
	else
	{
		DeleteFile(csTemp);
		return TRUE;
	}
}

BOOL 
Local_NMgtIngAt(CString ii_system, CString csKey, CString &csValue)
{
	CString s;
	CStdioFile theInputFile;
	int iPos;

	if (csKey.IsEmpty())
		return FALSE;

	s = ii_system + "\\ingres\\files\\symbol.tbl";
	if (!theInputFile.Open(s, CFile::modeRead | CFile::typeText, 0))
		return FALSE;

	while (theInputFile.ReadString(s))
	{
		s.TrimRight();
		iPos = s.Find(csKey);
		if (iPos >= 0)
		{
			if ((s.GetAt(iPos + csKey.GetLength()) == '\t') || 
				(s.GetAt(iPos + csKey.GetLength()) == ' '))
			{ 
				s = s.Right(s.GetLength() - csKey.GetLength());
				s.TrimLeft();
				csValue=  s;
				theInputFile.Close();
				return TRUE;
			}
		}
	}
	theInputFile.Close();
	return FALSE;
}

BOOL 
Local_PMget(CString ii_system, CString csKey, CString &csValue)
{
	CString s;
	CStdioFile theInputFile;
	int iPos;

	if(csKey.IsEmpty())
		return FALSE;

	s = ii_system + "\\ingres\\files\\config.dat";
	if(!theInputFile.Open(s, CFile::modeRead | CFile::typeText, 0))
		return FALSE;

	while(theInputFile.ReadString(s) != FALSE)
	{
		s.TrimRight();
		iPos = s.Find(csKey);
		if(iPos >= 0)
		{
			if( (s.GetAt(iPos + csKey.GetLength()) == ':') || 
				(s.GetAt(iPos + csKey.GetLength()) == ' ') )
			{ 
				s = s.Right(s.GetLength() - csKey.GetLength() - 1);
				s.TrimLeft();
				s.TrimLeft("\'");
				s.TrimRight("\'");
				s.Replace("\\\\", "\\");

				csValue = s;
				theInputFile.Close();
				return TRUE;
			}
		}
	}
	theInputFile.Close();
	return FALSE;
}

BOOL 
Local_RemoveDirectory(char *DirName)
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	DWORD dwAttrib;
	char szBuf[MAX_PATH];
	BOOL status=TRUE;
	
	dwAttrib = GetFileAttributes(DirName);
	if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
	{
		sprintf(szBuf, "%s\\*.*", DirName);
		hFind = FindFirstFile(szBuf, &FindFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					continue;
				
				sprintf(szBuf, "%s\\%s", DirName, FindFileData.cFileName);
				status = DeleteFile(szBuf);
				
			} while (FindNextFile(hFind, &FindFileData));
			FindClose(hFind);
		}
		status = RemoveDirectory(DirName);
	}
	else 
		status = DeleteFile(DirName);
	return status;
}
