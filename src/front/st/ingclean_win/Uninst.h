/*
**  Copyright (c) 2003 Ingres Corporation
*/

/*
**  Name: Uninst.h : interface for the CUninst and CCommandLineInfoEx class
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/

#if !defined(AFX_UNINST_H__11E6E0B7_FD8E_43A9_BEB6_003FC190CCD9__INCLUDED_)
#define AFX_UNINST_H__11E6E0B7_FD8E_43A9_BEB6_003FC190CCD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
**  Interface for the CUninst class
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/

class CUninst  
{
public:
	//member variables
	BOOL m_trymsiexec;
	BOOL m_silent;
	CString m_logbuf;
	HANDLE m_logfile;
	BOOL m_createicons;
	BOOL m_allusers;
	CStringList m_files;
	CStringList m_dirs;
	BOOL m_succeeded;
	HANDLE m_thread;
	CStatic *m_output;
	char m_host[MAX_COMPUTERNAME_LENGTH];
	char m_msidbpath[512];
	char m_upgradecode[39];
	char m_productcode[39];
	char m_ii_system[MAX_PATH]; 
	char m_id[3]; //installation identifier

	//member functions
	BOOL DeleteFileAssociations();
	BOOL GetAnotherInstallation(LPTSTR id, LPTSTR ii_system=NULL, BOOL bCheckDoc=0);
	void AppendToLog(UINT uiStrID);
	void AppendToLog(LPCTSTR p);
	BOOL UpdateEnvironmentStrings();
	BOOL RemoveShortcuts();
	BOOL RemoveRegistryValues();
	BOOL DeleteServices();
	BOOL UnpublishProduct();
	BOOL UnpublishComponents();
	BOOL RemoveFolders();
	BOOL RemoveFiles();
	BOOL PrepareUninstall();
	BOOL IsRunning();
	void ThreadUninstall();
	void Uninstall(CStatic *output);
	void AppendComment(UINT uiStrID);
	CUninst();
	virtual ~CUninst();
	BOOL ListAllFiles(LPCTSTR StartDir);
};

/*
**  Interface for the CCommandLineInfoEx class
**
**  History:
**
**	23-Sep-2003 (penga03)
**	    Created.
*/

class CCommandLineInfoEx : public CCommandLineInfo 
{
public:
	BOOL GetOption(LPCTSTR option, CString &val);
	BOOL GetOption(LPCTSTR option) {
		return GetOption(option , CString());
	}
protected:
	virtual void ParseParam(const TCHAR *pszParam, BOOL bFlag, BOOL bLast);
	CString m_sLastOption;
	CMapStringToString m_options;
};

#endif // !defined(AFX_UNINST_H__11E6E0B7_FD8E_43A9_BEB6_003FC190CCD9__INCLUDED_)
