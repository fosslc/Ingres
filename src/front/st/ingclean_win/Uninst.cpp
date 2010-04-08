/*
**  Copyright (c) 2006 Ingres Corporation.
*/

/*
**  Name: Uninst.cpp : implementation of the CUninst and CCommandLineInfoEx class
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
**	04-Nov-2004 (penga03)
**	    Updated the registry key and shortcut folder.
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	 07-May-2009 (drivi01)
**	    In efforts to port to Visual Studio 2008
**	    clean up warnings.
*/

#include "stdafx.h"
#include "Uninst.h"
#include "IngClean.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MAX_ENV_SIZE 4096
#define SEPARATOR ";"

typedef struct tagING_ENV_STRINGS
{
	char *key;
	char *value;
}ING_ENV_STRINGS;

static ING_ENV_STRINGS env_strings[]=
{
	{"Path","%s\\ingres\\bin",},
	{"Path","%s\\ingres\\utility",},
	{"Include","%s\\ingres\\files",},
	{"Lib","%s\\ingres\\lib",},
	{"II_SYSTEM","",},
	{"II_TEMPORARY","",},
	{0,0}
};

static char *EnvKey="SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";

/*
**  Implementation of the CUninst class
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/
//////////////////////////////////////////////////////////////////////
// Construction/Destruction, Member Functions
//////////////////////////////////////////////////////////////////////

CUninst::CUninst()
{
	char szBuf[MAX_PATH];
	DWORD dwSize;

	*m_id = 0;
	*m_ii_system = 0;
	*m_msidbpath = 0;
	*m_productcode = 0;
	m_thread = 0;
	m_succeeded = TRUE;
	m_output = 0;

	*m_host = 0;
	*szBuf = 0;
	dwSize = sizeof(szBuf);
	GetComputerName(szBuf, &dwSize);
	_tcscpy(m_host, _tcslwr(szBuf));

	m_createicons = 1;
	m_allusers = 1;

	*szBuf = 0;
	dwSize = sizeof(szBuf);
	GetTempPath(dwSize, szBuf);
	strcat(szBuf,"\\ingclean.log");
	m_logfile = CreateFile(szBuf, GENERIC_WRITE, FILE_SHARE_READ, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
}

CUninst::~CUninst()
{

}

void CUninst::AppendComment(UINT uiStrID)
{
	if (m_silent)
		return;

	CString s, t;

	s.LoadString(uiStrID);
	m_output->GetWindowText(t);
	t+=s;
	m_output->SetWindowText(t);
	m_output->UpdateWindow();
}

DWORD _stdcall ThreadUninstallFunc(LPVOID param)
{
	((CUninst *)param)->ThreadUninstall();
	theUninst.m_thread = 0;
	return theUninst.m_succeeded;
}

void CUninst::Uninstall(CStatic *output)
{
	DWORD ThreadId;
	
	m_output=output;
	m_thread=CreateThread(NULL, 0, ::ThreadUninstallFunc, (LPVOID)this, 0, &ThreadId);
}

void CUninst::ThreadUninstall()
{
	char cmd[MAX_PATH];

	//Preparing uninstall...
	//AppendToLog("\r\n");
	//AppendToLog(IDS_PREPAREUNINSTALL);
	if (m_succeeded)
	{
		AppendComment(IDS_PREPAREUNINSTALL);
		m_succeeded = PrepareUninstall();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Unpublishing components...
	AppendToLog("\r\n");
	AppendToLog(IDS_UNPUBLISHCOMPONENTS);
	if (m_succeeded)
	{
		AppendComment(IDS_UNPUBLISHCOMPONENTS);
		m_succeeded = UnpublishComponents();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Unpublishing product information...
	AppendToLog("\r\n");
	AppendToLog(IDS_UNPUBLISHPRODUCT);
	if (m_succeeded)
	{
		AppendComment(IDS_UNPUBLISHPRODUCT);
		m_succeeded = UnpublishProduct();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Deleting services..
	AppendToLog("\r\n");
	AppendToLog(IDS_DELETESERVICES);
	if (m_succeeded)
	{
		AppendComment(IDS_DELETESERVICES);
		m_succeeded = DeleteServices();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Removing perfmon counters...
	AppendToLog("\r\n");
	AppendToLog(IDS_REMOVEPERFMONCOUNTERS);
	if (m_succeeded)
	{
		AppendComment(IDS_REMOVEPERFMONCOUNTERS);
		Execute("unlodctr oipfctrs");
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Unregistering ActiveX Controls...
	AppendToLog("\r\n");
	AppendToLog(IDS_UNREGISTERINGCONTROLS);
	if (m_succeeded)
	{
		AppendComment(IDS_UNREGISTERINGCONTROLS);
		
		sprintf(cmd, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\IJACTRL.OCX\"", (LPCSTR)m_ii_system);
		Execute(cmd);
		sprintf(cmd, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\SVRIIA.DLL\"", (LPCSTR)m_ii_system);
		Execute(cmd);
		sprintf(cmd, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\ipm.ocx\"", (LPCSTR)m_ii_system);
		Execute(cmd);
		sprintf(cmd, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\sqlquery.ocx\"", (LPCSTR)m_ii_system);
		Execute(cmd);
		sprintf(cmd, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\vcda.ocx\"", (LPCSTR)m_ii_system);
		Execute(cmd);
		sprintf(cmd, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\vsda.ocx\"", (LPCSTR)m_ii_system);
		Execute(cmd);
		sprintf(cmd, "REGSVR32.EXE /s /u \"%s\\ingres\\bin\\svrsqlas.dll\"", (LPCSTR)m_ii_system);
		Execute(cmd);

		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Deleting file associations...
	AppendToLog("\r\n");
	AppendToLog(IDS_DELETEFILEASSOCIATIONS);
	if (m_succeeded)
	{
		AppendComment(IDS_DELETEFILEASSOCIATIONS);
		m_succeeded = DeleteFileAssociations();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Removing shortcuts...
	AppendToLog("\r\n");
	AppendToLog(IDS_REMOVESHORTCUTS);
	if (m_succeeded && m_createicons)
	{
		AppendComment(IDS_REMOVESHORTCUTS);
		m_succeeded = RemoveShortcuts();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Unregistering product...
	AppendToLog("\r\n");
	AppendToLog(IDS_REMOVEREGISTRYVALUES);
	if (m_succeeded)
	{
		AppendComment(IDS_REMOVEREGISTRYVALUES);
		m_succeeded = RemoveRegistryValues();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Updating environment strings...
	AppendToLog("\r\n");
	AppendToLog(IDS_UPDATEENVIRONMENTSTRINGS);
	if (m_succeeded)
	{
		AppendComment(IDS_UPDATEENVIRONMENTSTRINGS);
		m_succeeded = UpdateEnvironmentStrings();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Removing files...
	AppendToLog("\r\n");
	AppendToLog(IDS_REMOVEFILES);
	if (m_succeeded)
	{
		AppendComment(IDS_REMOVEFILES);
		m_succeeded = RemoveFiles();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}
	//Removing folders...
	AppendToLog("\r\n");
	AppendToLog(IDS_REMOVEFOLDERS);
	if (m_succeeded)
	{
		AppendComment(IDS_REMOVEFOLDERS);
		m_succeeded = RemoveFolders();
		AppendComment(m_succeeded ? IDS_DONE : IDS_FAILED);
	}

	CloseHandle(m_logfile);
	m_logfile = 0;
}

BOOL CUninst::IsRunning()
{
	return (m_thread != 0 );
}

BOOL CUninst::ListAllFiles(LPCTSTR StartDir)
{
	char szBuf[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	BOOL status=TRUE;
	
	wsprintf(szBuf, "%s\\*.*", StartDir);
	hFind=FindFirstFile(szBuf, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		m_dirs.AddHead(StartDir);
		do 
		{
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//directory
				if (*FindFileData.cFileName != '.')
				{
					wsprintf(szBuf,"%s\\%s", StartDir, FindFileData.cFileName);
					status=ListAllFiles(szBuf);
				}
			}
			else
			{
				//file
				wsprintf(szBuf, "%s\\%s", StartDir, FindFileData.cFileName);
				m_files.AddTail(szBuf);
			}
			
		} while (status && FindNextFile(hFind, &FindFileData));
	}

	FindClose(hFind);
	return status;
}

BOOL CUninst::PrepareUninstall()
{
	CString s, k;
	BOOL bRet=TRUE;

	CString ii_system(m_ii_system);
	bRet=ListAllFiles(ii_system + "\\ingres");
	
	s = ii_system + "\\CAREGLOG.LOG";
	if (GetFileAttributes(s) != -1) 
		m_files.AddTail(s);
	s = ii_system + "\\readme.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_a64_lnx.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_axp_osf.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_hp2_us5.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_hpb_us5.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_int_lnx.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_int_rpl.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_int_w32.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_r64_us5.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_rs4_us5.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_su4_us5.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_su9_us5.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	s = ii_system + "\\readme_usl_us5.html";
	if (GetFileAttributes(s) != -1)
		m_files.AddTail(s);
	
	/* List DATA, CKP, DMP, JNL, WORK and LOG if they 
	differ from II_SYSTEM. */

	if (Local_NMgtIngAt(ii_system, "II_DATABASE", s) 
		&& s.CompareNoCase(ii_system))
	{
		bRet=ListAllFiles(s + "\\ingres");
		m_dirs.AddTail(s);
	}
	if (Local_NMgtIngAt(ii_system, "II_CHECKPOINT", s) 
		&& s.CompareNoCase(ii_system))
	{
		bRet=ListAllFiles(s + "\\ingres");
		m_dirs.AddTail(s);
	}
	if (Local_NMgtIngAt(ii_system, "II_JOURNAL", s) 
		&& s.CompareNoCase(ii_system))
	{
		bRet=ListAllFiles(s + "\\ingres");
		m_dirs.AddTail(s);
	}
	if (Local_NMgtIngAt(ii_system, "II_DUMP", s) 
		&& s.CompareNoCase(ii_system))
	{
		bRet=ListAllFiles(s + "\\ingres");
		m_dirs.AddTail(s);
	}
	if (Local_NMgtIngAt(ii_system, "II_WORK", s) 
		&& s.CompareNoCase(ii_system))
	{
		bRet=ListAllFiles(s + "\\ingres");
		m_dirs.AddTail(s);
	}

	k.Format("ii.%s.rcp.log.log_file_parts", m_host);
	if (Local_PMget(ii_system, k, s))
	{
		int num_logs, i;
		
		num_logs = atoi(s);
		for (i = 1; i <= num_logs; ++i)
		{
			k.Format("ii.%s.rcp.log.log_file_%d", m_host, i);
			if (Local_PMget(ii_system, k, s) 
				&& s.CompareNoCase(ii_system))
			{
				bRet=ListAllFiles(s + "\\ingres");
				m_dirs.AddTail(s);
			}
		}
	}

	k.Format("ii.%s.rcp.log.dual_log_1", m_host);
	if (Local_PMget(ii_system, k, s) 
		&& s.CompareNoCase(ii_system))
	{
		bRet=ListAllFiles (s + "\\ingres");
		m_dirs.AddTail(s);
	}

	m_dirs.AddTail(ii_system);

	s.Format("C:\\WINDOWS\\Installer\\%s", m_productcode);
	bRet=ListAllFiles(s);
	m_files.AddTail(m_msidbpath);

	return bRet;
}

BOOL CUninst::RemoveFiles()
{
	BOOL bRet=TRUE;

	for (POSITION p = m_files.GetHeadPosition(); p;)
	{
		CString s = m_files.GetNext(p);
		SetFileAttributes(s, FILE_ATTRIBUTE_NORMAL);
		if (DeleteFile(s))
			m_logbuf.Format("file: %s...done", s);
		else
			m_logbuf.Format("file: %s...error: %d", s, GetLastError());
		AppendToLog(m_logbuf);
	}

	return bRet;
}

BOOL CUninst::RemoveFolders()
{
	BOOL bRet=TRUE;

	for (POSITION p = m_dirs.GetHeadPosition(); p;)
	{
		CString s = m_dirs.GetNext(p);

		SetFileAttributes(s, FILE_ATTRIBUTE_NORMAL);
		if (RemoveDirectory(s))
			m_logbuf.Format("dir: %s...done", s);
		else
			m_logbuf.Format("dir: %s...error: %d", s, GetLastError());
		AppendToLog(m_logbuf);
	}

	HKEY hKey=0;

	AppendToLog("\r\n");
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders", 
		0, KEY_ALL_ACCESS, &hKey))
	{
		for (POSITION p = m_dirs.GetHeadPosition(); p;)
		{
			CString s = m_dirs.GetNext(p);
			s+="\\";
			
			LONG nErrCode = RegDeleteValue(hKey, s);
			if (!nErrCode)
				m_logbuf.Format("value: HKLM\\...\\Installer\\Folders\\%s...done", s);
			else
				m_logbuf.Format("value: HKLM\\...\\Installer\\Folders\\%s...error: %d", s, nErrCode);
			AppendToLog(m_logbuf);
		}
	}

	return bRet;
}

BOOL CUninst::UnpublishComponents()
{
	MSIHANDLE hDb, hView, hRecord;
	char szQuery[512], reg[33];
	
	GUID_TO_REG(m_productcode, reg);
	
	if (MsiOpenDatabase(m_msidbpath, MSIDBOPEN_DIRECT, &hDb))
		return FALSE;
	
	sprintf(szQuery, "SELECT ComponentId FROM Component"); 
	if (!MsiDatabaseOpenView(hDb, szQuery, &hView))
	{
		if (!MsiViewExecute(hView, 0))
		{
			while (!MsiViewFetch(hView, &hRecord))
			{
				char szBuf[39], szKey[512], szTemp[33];
				DWORD dwSize=sizeof(szBuf);
				HKEY hKey=0;
				
				MsiRecordGetString(hRecord, 1, szBuf, &dwSize);
				GUID_TO_REG(szBuf, szTemp);
				
				sprintf(szKey, 
					"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Components\\%s", 
					szTemp);
				if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_ALL_ACCESS, &hKey))
				{
					LONG nErrCode = RegDeleteValue(hKey, reg);

					if (!nErrCode)
						m_logbuf.Format("component: %s...done", szTemp);
					else 
						m_logbuf.Format("component: %s...error: %d", szTemp, nErrCode);
					AppendToLog(m_logbuf);
					RegCloseKey(hKey);
				}
				SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, szKey);
			} //end of while loop
			MsiCloseHandle(hRecord);
		}
		MsiCloseHandle(hView);
	}
	MsiCloseHandle(hDb);
	return TRUE;
}

BOOL CUninst::UnpublishProduct()
{
	BOOL bRet=TRUE;
	char reg[33], szKey[512];
	DWORD nErrCode;

	GUID_TO_REG(m_productcode, reg);

	// features
	sprintf(szKey, "Installer\\Features\\%s", reg);
	nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKCR\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKCR\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, "SOFTWARE\\Classes\\Installer\\Features\\%s", reg);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	// products
	sprintf(szKey, "Installer\\Products\\%s", reg);
	nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKCR\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKCR\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, "SOFTWARE\\Classes\\Installer\\Products\\%s", reg);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Products\\%s", 
		reg);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	// upgradecodes
	GUID_TO_REG(m_upgradecode, reg);

	sprintf(szKey, "Installer\\UpgradeCodes\\%s", reg);
	nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKCR\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKCR\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);
	
	sprintf(szKey, "SOFTWARE\\Classes\\Installer\\UpgradeCodes\\%s", reg);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UpgradeCodes\\%s", 
		reg);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	return bRet;
}

BOOL CUninst::DeleteServices()
{
	BOOL bRet=TRUE;

	Execute("opingsvc remove");
	Execute("repinst remove");
	return bRet;
}

BOOL CUninst::RemoveRegistryValues()
{
	BOOL bRet=TRUE;
	char szKey[512];
	DWORD nErrCode;

	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", 
		m_productcode);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Ingres %s", 
		m_id);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, 
		"SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", 
		m_id);
	nErrCode = SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKLM\\%s...done", szKey);
	else 
		m_logbuf.Format("key: HKLM\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, "SOFTWARE\\IngresCorporation\\Ingres");
	SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, szKey);
	sprintf(szKey, "SOFTWARE\\IngresCorporation");
	SHDeleteEmptyKey(HKEY_LOCAL_MACHINE, szKey);

	return bRet;
}

static HRESULT UpdateIt(LPCSTR pszLink, LPCSTR pszFile, LPCSTR pszDir)
{
	HRESULT hres;
	IShellLink* psl;
	
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
		IID_IShellLink, (LPVOID *)&psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;
		
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
		if (SUCCEEDED(hres))
		{
			WORD wsz[MAX_PATH];
			
			hres = psl->SetPath(pszFile);
			hres = psl->SetWorkingDirectory(pszDir);
			MultiByteToWideChar(CP_ACP, 0, pszLink, -1, (LPWSTR)wsz, MAX_PATH);
			hres = ppf->Save((LPCOLESTR)wsz, TRUE);			
			ppf->Release();
		}
		psl->Release();
	}
	return hres;
}

static HRESULT UpdateDocShortcuts(LPCTSTR lpFolder, LPCTSTR lpPath, BOOL bEmbed=0)
{
	HRESULT hRet;
	CString csLink, csFile, csDir;
	
	csDir.Format("\"%s\\ingres\\files\\english\"", lpPath);
	
	csFile.Format("\"%s\\ingres\\files\\english\\gs.pdf\"", lpPath);
	csLink.Format("%s\\Getting Started.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\mg.pdf\"", lpPath);
	csLink.Format("%s\\Migration Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\ome.pdf\"", lpPath);
	csLink.Format("%s\\Object Management Extension User Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\cmdref.pdf\"", lpPath);
	csLink.Format("%s\\Command Reference Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\dba.pdf\"", lpPath);
	csLink.Format("%s\\Database Administrator's Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\dtp.pdf\"", lpPath);
	csLink.Format("%s\\Distributed Transaction Processing User Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\esqlc.pdf\"", lpPath);
	csLink.Format("%s\\Embedded SQL for C Companion Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\esqlcob.pdf\"", lpPath);
	csLink.Format("%s\\Embedded SQL for COBOL Companion Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\iceug.pdf\"", lpPath);
	csLink.Format("%s\\ICE User Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\oapi.pdf\"", lpPath);
	csLink.Format("%s\\OpenAPI User Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\osql.pdf\"", lpPath);
	csLink.Format("%s\\OpenSQL Reference Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\quelref.pdf\"", lpPath);
	csLink.Format("%s\\QUEL Reference Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\rep.pdf\"", lpPath);
	csLink.Format("%s\\Replicator User Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\sqlref.pdf\"", lpPath);
	csLink.Format("%s\\SQL Reference Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\sysadm.pdf\"", lpPath);
	csLink.Format("%s\\System Administrators Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\starug.pdf\"", lpPath);
	csLink.Format("%s\\Star User Guide.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\qrytools.pdf\"", lpPath);
	csLink.Format("%s\\Using Character-Based Querying and Reporting Tools.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	csFile.Format("\"%s\\ingres\\files\\english\\ufadt.pdf\"", lpPath);
	csLink.Format("%s\\Using Forms-Based Application Development Tools.lnk", lpFolder);
	hRet=UpdateIt(csLink, csFile, csDir);
	
	if (bEmbed)
	{
		csFile.Format("\"%s\\ingres\\files\\english\\esqlref.pdf\"", lpPath);
		csLink.Format("%s\\Embedded Ingres SQL Reference Guide.lnk", lpFolder);
		hRet=UpdateIt(csLink, csFile, csDir);
	
		csFile.Format("\"%s\\ingres\\files\\english\\esysadm.pdf\"", lpPath);
		csLink.Format("%s\\Embedded Ingres Administrator's Guide.lnk", lpFolder);
		hRet=UpdateIt(csLink, csFile, csDir);
	}
	
	return hRet;
}

BOOL CUninst::RemoveShortcuts()
{
	BOOL bRet=TRUE;
	LPITEMIDLIST pidl;
	char szPath[MAX_PATH], szFolder[MAX_PATH];

	if (!m_createicons)
		return bRet;
	
	CoInitialize(NULL);
	
	if (m_allusers)
	{
		SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidl);
		SHGetPathFromIDList(pidl, szPath);
	}
	else
	{
		SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidl);
		SHGetPathFromIDList(pidl, szPath);
	}
	
	sprintf(szFolder, "%s\\Computer Associates\\Ingres [ %s ]", szPath, m_id);
	if (Local_RemoveDirectory(szFolder))
	{
		m_logbuf.Format("folder: %s...done", szFolder);
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szFolder, 0);
	}
	else
		m_logbuf.Format("folder:%s...failed", szFolder);
	AppendToLog(m_logbuf);

	sprintf(szFolder, "%s\\Computer Associates", szPath);
	if (RemoveDirectory(szFolder))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szFolder, 0);
	
	if (m_allusers)
	{
		SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTUP, &pidl);
		SHGetPathFromIDList(pidl, szPath);
	}
	else
	{
		SHGetSpecialFolderLocation(NULL, CSIDL_STARTUP, &pidl);
		SHGetPathFromIDList(pidl, szPath);
	}
	sprintf(szFolder, "%s\\Ingres Visual Manager [ %s ].lnk", szPath, m_id);
	if (DeleteFile(szFolder))
		SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szFolder, 0);

	CoUninitialize();
	return bRet;	
}

static BOOL Local_GetEnvironmentVariable(LPCTSTR lpName, LPTSTR lpBuffer, DWORD nSize)
{
	HKEY hKey=0;
	DWORD dwType;
	BOOL bRet=FALSE;
	
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, EnvKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		if (!RegQueryValueEx(hKey, lpName, NULL, &dwType, 
			(BYTE *)lpBuffer, &nSize))
			bRet=TRUE;
		RegCloseKey(hKey);
	}
	return bRet;
}

static BOOL Local_SetEnvironmentVariable(LPCTSTR lpName, LPCTSTR lpValue)
{
	DWORD dwDisposition;
	HKEY hKey=0;
	BOOL bRet=FALSE;

	if (!RegCreateKeyEx(HKEY_LOCAL_MACHINE, EnvKey, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, 
		&dwDisposition))
	{
		LONG nErrCode;

		if (lpValue && *lpValue)
		{
			nErrCode = RegSetValueEx(hKey, lpName, 0, REG_EXPAND_SZ, 
				(BYTE *)lpValue, lstrlen(lpValue)+1);
			if (!nErrCode)
			{
				bRet=TRUE;
				theUninst.m_logbuf.Format("env: %s=%s...done", lpName, lpValue);
			}
			else
				theUninst.m_logbuf.Format("env: %s=%s...error: %d", lpName, lpValue, nErrCode);
			theUninst.AppendToLog(theUninst.m_logbuf);
		}
		else
		{
			nErrCode = RegDeleteValue(hKey, lpName);
			if (!nErrCode)
			{
				theUninst.m_logbuf.Format("env: %s...done", lpName);
				bRet=TRUE;
			}
			else
				theUninst.m_logbuf.Format("env: %s...error: %d", lpName, nErrCode);
			theUninst.AppendToLog(theUninst.m_logbuf);
		}
		RegCloseKey(hKey);
	}
		
	return bRet;
}

static BOOL RemoveValue(LPTSTR oldValue, LPTSTR newValue)
{
	char szBuf[MAX_ENV_SIZE];
	CStringList sl;
	BOOL bRemoved=FALSE;

	*szBuf=0;
	while (oldValue)
	{
		LPTSTR p=_tcsstr(oldValue, SEPARATOR);
		if (p) *p=0;
		sl.AddTail(oldValue);
		oldValue=p ? p+1 : p;
	}
	for (POSITION p=sl.GetHeadPosition(); p; )
	{
		CString s=sl.GetNext(p);
		if (s.CompareNoCase(newValue))
		{
			_tcscat(szBuf, s);
			_tcscat(szBuf, SEPARATOR);
		}
		else
			bRemoved=TRUE;
	}
	if (*szBuf)
		szBuf[_tcsclen(szBuf)-1]=0;

	_tcscpy(newValue, szBuf);
	return bRemoved;
}

BOOL CUninst::UpdateEnvironmentStrings()
{
	BOOL bRet=TRUE;

	for (int i=0; env_strings[i].key; i++)
	{
		char oldValue[MAX_ENV_SIZE];

		if (Local_GetEnvironmentVariable(env_strings[i].key, oldValue, sizeof(oldValue)))
		{
			char newValue[MAX_ENV_SIZE];

			if (*env_strings[i].value)
			{
				wsprintf(newValue, env_strings[i].value, m_ii_system);
				RemoveValue(oldValue, newValue);
			}
			else
				*newValue=0;
			Local_SetEnvironmentVariable(env_strings[i].key, newValue);
		}
	}

	::SendMessage(::FindWindow("Progman", NULL), WM_WININICHANGE, 0L, (LPARAM)EnvKey);
	return bRet;
}

void CUninst::AppendToLog(LPCTSTR p)
{
	if (!m_logfile)
		return;

	CString s;
	DWORD dwCount;

	s=p; s+="\r\n";
	WriteFile(m_logfile, s, s.GetLength(), &dwCount, NULL);
}

void CUninst::AppendToLog(UINT uiStrID)
{
	CString s;s.LoadString(uiStrID);
	AppendToLog(s);
}

BOOL CUninst::GetAnotherInstallation(LPTSTR id, LPTSTR ii_system/*=NULL*/, BOOL bCheckDoc/*=0*/)
{
	BOOL bRet=FALSE;
	HKEY hKey=0;
	
	if (!id || strlen(id) < 3)
		return bRet;
	
	*id=0;

	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\IngresCorporation\\Ingres", 
		0, KEY_ENUMERATE_SUB_KEYS, &hKey))
	{
		LONG nErrCode;
		int i;
		
		for (i=0, nErrCode=0; !nErrCode; i++)
		{
			char SubKey[16];
			DWORD dwSize=sizeof(SubKey);
			
			nErrCode=RegEnumKeyEx(hKey, i, SubKey, &dwSize, NULL, NULL, NULL, NULL);
			strncpy(id, SubKey, 2);
			id[2] = '\0';

			if (!nErrCode && _stricmp(id, m_id)) 
			{
				// found one
				HKEY hSubKey=0;
				
				if (!RegOpenKeyEx(hKey, SubKey, 0, KEY_QUERY_VALUE, &hSubKey))
				{
					char szData[MAX_PATH];
					DWORD dwType, dwSize=sizeof(szData);
					
					if (!RegQueryValueEx(hSubKey, "II_SYSTEM", NULL, &dwType, 
						(BYTE *)szData, &dwSize))
					{
						char szTemp[MAX_PATH];

						if (ii_system)
							_tcscpy(ii_system, szData);

						sprintf(szTemp, "%s\\ingres\\files\\english\\gs.pdf", szData);
						strcat(szData, "\\ingres\\files\\config.dat");
						if ((GetFileAttributes(szData) != -1) 
							&& (!bCheckDoc || GetFileAttributes(szTemp) != -1))
						{				
							bRet=TRUE; 
							RegCloseKey(hSubKey);
							RegCloseKey(hKey);
							return bRet;
						}
					}
					RegCloseKey(hSubKey);
				}
			}
		} //end of for loop
		RegCloseKey(hKey);
	}


	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\ComputerAssociates\\Ingres", 
		0, KEY_ENUMERATE_SUB_KEYS, &hKey))
	{
		LONG nErrCode;
		int i;
		
		for (i=0, nErrCode=0; !nErrCode; i++)
		{
			char SubKey[16];
			DWORD dwSize=sizeof(SubKey);
			
			nErrCode=RegEnumKeyEx(hKey, i, SubKey, &dwSize, NULL, NULL, NULL, NULL);
			strncpy(id, SubKey, 2);
			id[2] = '\0';

			if (!nErrCode && _stricmp(id, m_id)) 
			{
				// found one
				HKEY hSubKey=0;
				
				if (!RegOpenKeyEx(hKey, SubKey, 0, KEY_QUERY_VALUE, &hSubKey))
				{
					char szData[MAX_PATH];
					DWORD dwType, dwSize=sizeof(szData);
					
					if (!RegQueryValueEx(hSubKey, "II_SYSTEM", NULL, &dwType, 
						(BYTE *)szData, &dwSize))
					{
						char szTemp[MAX_PATH];

						if (ii_system)
							_tcscpy(ii_system, szData);

						sprintf(szTemp, "%s\\ingres\\files\\english\\gs.pdf", szData);
						strcat(szData, "\\ingres\\files\\config.dat");
						if ((GetFileAttributes(szData) != -1) 
							&& (!bCheckDoc || GetFileAttributes(szTemp) != -1))
						{				
							bRet=TRUE; 
							RegCloseKey(hSubKey);
							RegCloseKey(hKey);
							return bRet;
						}
					}
					RegCloseKey(hSubKey);
				}
			}
		} //end of for loop
		RegCloseKey(hKey);
	}


	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\ComputerAssociates\\Advantage Ingres", 
		0, KEY_ENUMERATE_SUB_KEYS, &hKey))
	{
		LONG nErrCode;
		int i;
		
		for (i=0, nErrCode=0; !nErrCode; i++)
		{
			char SubKey[16];
			DWORD dwSize=sizeof(SubKey);
			
			nErrCode=RegEnumKeyEx(hKey, i, SubKey, &dwSize, NULL, NULL, NULL, NULL);
			strncpy(id, SubKey, 2);
			id[2] = '\0';

			if (!nErrCode && _stricmp(id, m_id)) 
			{
				// found one
				HKEY hSubKey=0;
				
				if (!RegOpenKeyEx(hKey, SubKey, 0, KEY_QUERY_VALUE, &hSubKey))
				{
					char szData[MAX_PATH];
					DWORD dwType, dwSize=sizeof(szData);
					
					if (!RegQueryValueEx(hSubKey, "II_SYSTEM", NULL, &dwType, 
						(BYTE *)szData, &dwSize))
					{
						char szTemp[MAX_PATH];

						if (ii_system)
							_tcscpy(ii_system, szData);

						sprintf(szTemp, "%s\\ingres\\files\\english\\gs.pdf", szData);
						strcat(szData, "\\ingres\\files\\config.dat");
						if ((GetFileAttributes(szData) != -1) 
							&& (!bCheckDoc || GetFileAttributes(szTemp) != -1))
						{				
							bRet=TRUE; 
							RegCloseKey(hSubKey);
							RegCloseKey(hKey);
							return bRet;
						}
					}
					RegCloseKey(hSubKey);
				}
			}
		} //end of for loop
		RegCloseKey(hKey);
	}

	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\ComputerAssociates\\IngresII", 
		0, KEY_ENUMERATE_SUB_KEYS, &hKey))
	{
		LONG nErrCode;
		int i;
		
		for (i=0, nErrCode=0; !nErrCode; i++)
		{
			char SubKey[16];
			DWORD dwSize=sizeof(SubKey);
			
			nErrCode=RegEnumKeyEx(hKey, i, SubKey, &dwSize, NULL, NULL, NULL, NULL);
			strncpy(id, SubKey, 2);
			id[2] = '\0';

			if (!nErrCode && _stricmp(id, m_id)) 
			{
				// found one
				HKEY hSubKey=0;
				
				if (!RegOpenKeyEx(hKey, SubKey, 0, KEY_QUERY_VALUE, &hSubKey))
				{
					char szData[MAX_PATH];
					DWORD dwType, dwSize=sizeof(szData);
					
					if (!RegQueryValueEx(hSubKey, "II_SYSTEM", NULL, &dwType, 
						(BYTE *)szData, &dwSize))
					{
						if (ii_system)
							_tcscpy(ii_system, szData);
						strcat(szData, "\\ingres\\files\\config.dat");
						if (GetFileAttributes(szData) != -1)
						{
							bRet=TRUE; 
							RegCloseKey(hSubKey);
							RegCloseKey(hKey);
							return bRet;
						}
					}
					RegCloseKey(hSubKey);
				}
			}
		} //end of for loop
		RegCloseKey(hKey);
	}

	return bRet;
}

BOOL CUninst::DeleteFileAssociations()
{
	BOOL bRet=TRUE;
	char szKey[512];
	DWORD nErrCode;

	sprintf(szKey, "Ingres.VDBA.%s", m_id);
	nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKCR\\%s...done", szKey);
	else
		m_logbuf.Format("key: HKCR\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, "Ingres.IIA.%s", m_id);
	nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKCR\\%s...done", szKey);
	else
		m_logbuf.Format("key: HKCR\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	sprintf(szKey, "Ingres_Database_%s", m_id);
	nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, szKey);
	if (!nErrCode)
		m_logbuf.Format("key: HKCR\\%s...done", szKey);
	else
		m_logbuf.Format("key: HKCR\\%s...error: %d", szKey, nErrCode);
	AppendToLog(m_logbuf);

	char id[3];

	if (GetAnotherInstallation(id))
	{
		HKEY hKey=0;
		char szData[32];

		if (!RegOpenKeyEx(HKEY_CLASSES_ROOT, ".vdbacfg", 0, KEY_SET_VALUE, &hKey))
		{			
			sprintf(szData, "Ingres.VDBA.%s", id);
			nErrCode = RegSetValueEx(hKey, NULL, 0, REG_SZ, 
				(BYTE *)szData, lstrlen(szData)+1);
			if (!nErrCode)
				m_logbuf.Format("reset HKCR\\.vdbacfg=%s...done", szData);
			else
				m_logbuf.Format("reset HKCR\\.vdbacfg=%s...error: %d", szData, nErrCode);
			AppendToLog(m_logbuf);
			RegCloseKey(hKey);
		}

		if (!RegOpenKeyEx(HKEY_CLASSES_ROOT, ".ii_imp", 0, KEY_SET_VALUE, &hKey))
		{			
			sprintf(szData, "Ingres.IIA.%s", id);
			nErrCode = RegSetValueEx(hKey, NULL, 0, REG_SZ, 
				(BYTE *)szData, lstrlen(szData)+1);
			if (!nErrCode)
				m_logbuf.Format("reset HKCR\\.ii_imp=%s...done", szData);
			else
				m_logbuf.Format("reset HKCR\\.ii_imp=%s...error: %d", szData, nErrCode);
			AppendToLog(m_logbuf);
			RegCloseKey(hKey);
		}
	}
	else
	{
		nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, ".vdbacfg");
		if (!nErrCode)
			m_logbuf.Format("key: HKCR\\%s...done", ".vdbacfg");
		else
			m_logbuf.Format("key: HKCR\\%s...error: %d", ".vdbacfg", nErrCode);

		nErrCode = SHDeleteKey(HKEY_CLASSES_ROOT, ".ii_imp");
		if (!nErrCode)
			m_logbuf.Format("key: HKCR\\%s...done", ".ii_imp");
		else
			m_logbuf.Format("key: HKCR\\%s...error: %d", ".ii_imp", nErrCode);
	}
	return bRet;
}

/*
**  Implementation of the CCommandLineInfoEx class
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/
//////////////////////////////////////////////////////////////////////
// CCommandLineInfoEx Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Member Functions
//////////////////////////////////////////////////////////////////////

BOOL CCommandLineInfoEx::GetOption(LPCTSTR option, CString &val)
{
	return m_options.Lookup(option, val);
}

void CCommandLineInfoEx::ParseParam(const TCHAR *pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag)
	{
		m_options[pszParam] = "";
		m_sLastOption = pszParam;
	}
	else if (!m_sLastOption.IsEmpty())
	{
		m_options[m_sLastOption] = pszParam;
		m_sLastOption.Empty();
	}
	CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}

