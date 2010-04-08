/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : compfile.cpp: implementation file.
**    Project  : ActiveX Component
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Compound File Management
**
** History:
**
** 07-Mar-2002 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 29-Aug-2007 (drivi01)
**    On Vista, store iiguis.prf in %USERPROFILE% location.
** 30-Aug-2008 (drivi01)
**    On Vista, move iiguis.prf to %ALLUSERSPROFILE% instead
**    to preserve 1 preference file per installation.
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> includes which are already in "stdafx.h"
** 08-Sep-2009 (drivi01)
**    Add IsDBATools function to detect if the installation is a 
**    DBA Tools installation.
**    Fix temporary directory on Vista and above for iiguis.prf file
**    for DBA Tools installations.s
** 09-Mar-2010 (drivi01)
**    Clean up the warnings.
**/

#include "stdafx.h"
#include "compfile.h"
#include <compat.h>
#include <gv.h>
#include <nm.h>
#include <er.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL IsDBATools(CString strII);
//
// CLASS: CaCompoundFile 
// ************************************************************************************************
CaCompoundFile::CaCompoundFile(IStorage* pStorage)
{
	m_pIStorage = pStorage;
	m_pIStorage->AddRef();
}

CaCompoundFile::CaCompoundFile(LPCTSTR lpszFileName, DWORD grfMode)
{
	USES_CONVERSION;
	CString tmp, config_path;
	int rw;
	m_pIStorage = NULL;
	HRESULT hr = NOERROR;
	ASSERT(lpszFileName != NULL);
	LPCWSTR wszName = T2CW(lpszFileName);
	
	//
	//Create directory for compound file if it doesn't already exist
	//
	if (GVvista() && GetFileAttributes((LPCSTR)lpszFileName) == -1)
	{
		int pos;
		tmp.Format("%s", lpszFileName);
		config_path.Format("%s", lpszFileName);
		pos = tmp.ReverseFind('\\');
		if (pos>=0)
		{
			tmp = config_path = tmp.Mid(0, pos);
			pos = tmp.ReverseFind('\\');
			if (pos>=0)
				tmp = tmp.Mid(0, pos);
		}
		if (GetFileAttributes((LPCSTR)config_path) == -1)
		{
			rw = CreateDirectoryRecursively(tmp.GetBuffer());
			if (rw || (rw == 0 && GetLastError() == ERROR_ALREADY_EXISTS))
				rw = CreateDirectoryRecursively(config_path.GetBuffer());
			if (!rw)
				AfxMessageBox("Failed to create configuration directory!", MB_OK);
		}
	}
	//
	// Use COM service to check if the file is out there and is actually
	// a valid compound file.
	if (StgIsStorageFile(wszName) == S_OK)
	{
		// We have a compound file. Use COM service to open the compound file
		// and obtain a IStorage interface.
		hr = StgOpenStorage(
			wszName,
			NULL,
			grfMode,
			NULL,
			0,
			&m_pIStorage);
		if (FAILED(hr))
		{
			throw CeCompoundFileException (hr);
		}
	}
	else
	{
		// If there was no existing compound file use COM services to
		// create a new compound file.
		grfMode |= STGM_CREATE;
		hr = StgCreateDocfile(
			wszName,
			grfMode,
			0,
			&m_pIStorage);

		if (FAILED(hr))
		{
			throw CeCompoundFileException (hr);
		}
	}
}

CaCompoundFile::~CaCompoundFile()
{
	if (m_pIStorage)
	{
		m_pIStorage->Commit(STGC_OVERWRITE);
		m_pIStorage->Release();
	}
}


IStorage* CaCompoundFile::NewStorage(IStorage* pParent, LPCTSTR lpszName, DWORD grfMode)
{
	USES_CONVERSION;
	ASSERT(m_pIStorage);
	if (!m_pIStorage)
		return NULL;
	LPCWSTR wszName = T2CW(lpszName);
	IStorage* pStorage = pParent? pParent: m_pIStorage;
	IStorage* pNewStorage = NULL;
	HRESULT hr = pStorage->CreateStorage(
		wszName,           // Points to the name of the new storage object
		grfMode,           // Access mode for the new storage object
		0,                 // Reserved; must be zero
		0,                 // Reserved; must be zero
		&pNewStorage);     // Points to new storage object

	if (hr == STG_E_FILEALREADYEXISTS)
		throw CeCompoundFileException (hr);
	if (SUCCEEDED (hr))
		return pNewStorage;

	return pNewStorage;
}

IStorage* CaCompoundFile::OpenStorage(IStorage* pParent, LPCTSTR lpszName, DWORD grfMode)
{
	USES_CONVERSION;
	LPCWSTR pwcsName = T2CW(lpszName);
	IStorage* pStorage = pParent? pParent: m_pIStorage;
	ASSERT(pStorage);
	if (!pStorage)
		return NULL;

	IStorage* pExistStorage = NULL;
	HRESULT hr = pStorage->OpenStorage(
		pwcsName,      // Points to name of stream to open
		NULL,          // Previous storage
		grfMode,       // Access mode for the new stream
		NULL,          // Reserved; must be NULL
		0,             // Reserved; must be 0
		&pExistStorage);     // Address of output variable that 
	if (FAILED (hr))
		throw CeCompoundFileException (hr);
	return pExistStorage;

}

IStream* CaCompoundFile::NewStream (IStorage* pIStorage, LPCTSTR lpszName, DWORD grfMode)
{
	USES_CONVERSION;
	IStream* pNewStream = NULL;
	ASSERT(m_pIStorage);
	if (!m_pIStorage)
		return NULL;
	IStorage* pStorage = pIStorage? pIStorage: m_pIStorage;

	LPCWSTR wszName = T2CW(lpszName);
	HRESULT hr = pStorage->CreateStream(
		wszName,           // Points to the name of the new stream
		grfMode,           // Access mode for the new stream
		0,                 // Reserved; must be zero
		0,                 // Reserved; must be zero
		&pNewStream);      // Points to new stream object

	if (hr == STG_E_FILEALREADYEXISTS)
		throw CeCompoundFileException (hr);
	if (SUCCEEDED (hr))
		return pNewStream;
	return NULL;
}


IStream* CaCompoundFile::OpenStream(IStorage* pParent, LPCTSTR lpszName, DWORD grfMode)
{
	USES_CONVERSION;
	LPCWSTR pwcsName = T2CW(lpszName);
	IStorage* pStorage = pParent? pParent: m_pIStorage;
	ASSERT(pStorage);
	if (!pStorage)
		return NULL;

	IStream* pStream = NULL;
	HRESULT hr = pStorage->OpenStream(
		pwcsName,      // Points to name of stream to open
		NULL,          // Reserved; must be NULL
		grfMode,       // Access mode for the new stream
		0,             // Reserved; must be zero
		&pStream);     // Address of output variable that 
	if (FAILED (hr))
		throw CeCompoundFileException (hr);
	return pStream;
}


BOOL CaCompoundFile::FindElement(IStorage* pParent, LPCTSTR lpszName, DWORD dwElemntType)
{
	USES_CONVERSION;
	IStorage* pStorage = pParent? pParent: m_pIStorage;
	ASSERT(pStorage);
	if (!pStorage)
		return FALSE;
	LPCOLESTR pwcsName = T2COLE(lpszName);

	IEnumSTATSTG* penumStatStg = NULL;
	STATSTG statstg;
	IMalloc* pIMalloc = NULL;
	CoGetMalloc(1, &pIMalloc);
	BOOL bFound = FALSE;

	HRESULT hr = pStorage->EnumElements(0, NULL, 0, &penumStatStg);
	if (!SUCCEEDED(hr))
		return FALSE;
	while (hr != S_FALSE)
	{
		hr = penumStatStg->Next(1, &statstg, NULL);
		if (FAILED(hr))
			return FALSE;
		else if (S_OK == hr)
		{
			if (statstg.type != dwElemntType)
			{
				if (statstg.pwcsName && pIMalloc)
					pIMalloc->Free((LPVOID)statstg.pwcsName);
				continue;
			}

			if (statstg.pwcsName)
			{
				if( _wcsicmp (statstg.pwcsName, pwcsName) == 0 )
					bFound = TRUE;
				if (statstg.pwcsName && pIMalloc)
					pIMalloc->Free((LPVOID)statstg.pwcsName);
				if (bFound)
					break;
			}
		}
	}
	
	if (pIMalloc)
		pIMalloc->Release();
	if (penumStatStg)
		penumStatStg->Release();
	return bFound;
}


//
// Utilities Functions
// ************************************************************************************************
BOOL SaveStreamInit (IUnknown* pUnk, LPCTSTR lpszStreamName, IStorage* pStorage)
{
	IStorage* pActiveXStorage = NULL;
	IStream* pStream  = NULL;
	CaCompoundFile* pCompoundFile = NULL;
	BOOL bFound = FALSE;
	BOOL bOK = TRUE;
	char *ii_installation;
	try
	{
		DWORD grfMode1 = STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
		DWORD grfMode2 = STGM_DIRECT|STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
		if (!pStorage)
		{
			TCHAR* pEnv;
			if (GVvista())
			pEnv = _tgetenv(consttchszUSERPROFILE);
			else
			pEnv = _tgetenv(consttchszII_SYSTEM);
			if (!pEnv)
				return FALSE;

			CString strPath = pEnv;
			if (GVvista())
			{
			strPath += consttchszIngConf;
			NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
			if (IsDBATools(ii_installation) && stricmp(ii_installation, "VT")==0)
				strPath += consttchszDBATools;
			else
			{
			      if (IsDBATools(ii_installation) && stricmp(ii_installation, "VT")!=0)
			      {
				strPath += consttchszDBATools;
				strPath += " ";
                              }
			      else
				strPath += consttchszIngresProd;
			      strPath += ii_installation;
			}
			strPath += consttchszIngConfFiles;
			}
			else
			strPath += consttchszIngVdba;
			strPath += _T("iiguis.prf");

			pCompoundFile = new CaCompoundFile(strPath);
			bFound = pCompoundFile->FindElement(NULL, _T("ActiveXRootStorage"), STGTY_STORAGE);
			if (bFound)
				pActiveXStorage = pCompoundFile->OpenStorage(NULL, _T("ActiveXRootStorage"), grfMode1);
			else
				pActiveXStorage = pCompoundFile->NewStorage (NULL, _T("ActiveXRootStorage"), grfMode2);
			if (!pActiveXStorage)
				throw CeCompoundFileException();

			bFound = pCompoundFile->FindElement(pActiveXStorage, lpszStreamName, STGTY_STREAM);
			if (bFound)
				pStream = pCompoundFile->OpenStream (pActiveXStorage, lpszStreamName, grfMode1);
			else
				pStream = pCompoundFile->NewStream  (pActiveXStorage, lpszStreamName, grfMode2);
			if (!pStream)
				throw CeCompoundFileException();
		}
		else
		{
			pCompoundFile = new CaCompoundFile(pStorage);
			bFound = pCompoundFile->FindElement(NULL, lpszStreamName, STGTY_STREAM);
			if (bFound)
				pStream = pCompoundFile->OpenStream (NULL, lpszStreamName, grfMode1);
			else
				pStream = pCompoundFile->NewStream  (NULL, lpszStreamName, grfMode2);
			if (!pStream)
				throw CeCompoundFileException();
		}
		
		IPersistStreamInit* pPersistStreammInit = NULL;
		HRESULT hr = E_FAIL;
		if(pUnk)
		{
			hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
			if(SUCCEEDED(hr) && pPersistStreammInit)
			{
				hr = pPersistStreammInit->Save(pStream, TRUE);
				pPersistStreammInit->Release();
			}
		}
	}
	catch (CeCompoundFileException e)
	{
		bOK = FALSE;
	}
	if (pStream)
		pStream->Release();
	if (pActiveXStorage)
		pActiveXStorage->Release();
	if (pCompoundFile)
		delete pCompoundFile;
	return bOK;
}


BOOL OpenStreamInit (IUnknown* pUnk, LPCTSTR lpszStreamName, IStorage* pStorage)
{
	//
	// Load the propertis from file IIGUIS.PRF:
	USES_CONVERSION;
	BOOL bOK = TRUE;
	CaCompoundFile* pCompoundFile = NULL;
	IStorage* pActiveXStorage = NULL;
	IStream* pStream = NULL;
	char *ii_installation;
	try
	{
		DWORD grfMode = STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
		if (!pStorage)
		{
			TCHAR* pEnv;
			if (GVvista())
			pEnv = _tgetenv(consttchszUSERPROFILE);
			else
			pEnv = _tgetenv(consttchszII_SYSTEM);
			if (!pEnv)
				return FALSE;
			CString strPath = pEnv;
			if (GVvista())
			{
			strPath += consttchszIngConf;
			NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
			if (IsDBATools(ii_installation) && stricmp(ii_installation, "VT")==0)
				strPath += consttchszDBATools;
			else
			{
			      if (IsDBATools(ii_installation) && stricmp(ii_installation, "VT")!=0)
			      {
				strPath += consttchszDBATools;
				strPath += " ";
                              }
			      else
				strPath += consttchszIngresProd;
			      strPath += ii_installation;
			}
			strPath += consttchszIngConfFiles;
			}
			else
			strPath += consttchszIngVdba;
			strPath += _T("iiguis.prf");

			if (StgIsStorageFile(T2COLE((LPCTSTR)strPath)) != S_OK)
				return FALSE;
			pCompoundFile = new CaCompoundFile(strPath);
			pActiveXStorage = pCompoundFile->OpenStorage(NULL, _T("ActiveXRootStorage"), grfMode);
			if (!pActiveXStorage)
			{
				delete pCompoundFile;
				return bOK;
			}

			pStream = pCompoundFile->OpenStream (pActiveXStorage, lpszStreamName, grfMode);
			if (!pStream)
			{
				pActiveXStorage->Release();
				delete pCompoundFile;
				return TRUE;
			}
		}
		else
		{
			pCompoundFile = new CaCompoundFile(pStorage);
			pStream = pCompoundFile->OpenStream (NULL, lpszStreamName, grfMode);
			if (!pStream)
			{
				delete pCompoundFile;
				return TRUE;
			}
		}

		IPersistStreamInit* pPersistStreammInit = NULL;
		HRESULT hr = E_FAIL;
		if(pUnk)
		{
			hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
			if(SUCCEEDED(hr) && pPersistStreammInit)
			{
				hr = pPersistStreammInit->Load(pStream);
				pPersistStreammInit->Release();
			}
		}
	}
	catch (CeCompoundFileException e)
	{
		bOK = FALSE;
	}
	
	if (pStream)
		pStream->Release();
	if (pActiveXStorage)
		pActiveXStorage->Release();
	if (pCompoundFile)
		delete pCompoundFile;
	return bOK;
}


CaPersistentStreamState::CaPersistentStreamState(LPCTSTR lpszStreamName, BOOL bLoad)
{
	char *ii_installation;
	m_bStateOk = FALSE;
	TCHAR* pEnv;
	if (GVvista())
	pEnv = _tgetenv(consttchszUSERPROFILE);
	else
	pEnv = _tgetenv(consttchszII_SYSTEM);
	if (!pEnv)
		return;
	CString strPath = pEnv;
	if (GVvista())
	{
		strPath += consttchszIngConf;
		NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
		if (IsDBATools(ii_installation) && stricmp(ii_installation, "VT")==0)
			strPath += consttchszDBATools;
		else
		{
		      if (IsDBATools(ii_installation) && stricmp(ii_installation, "VT")!=0)
		      {
			strPath += consttchszDBATools;
			strPath += " ";
                      }
		      else
			strPath += consttchszIngresProd;
		      strPath += ii_installation;
		}
		strPath += consttchszIngConfFiles;
	}
	else
	strPath += consttchszIngVdba;
	strPath += _T("iiguis.prf");

	m_pcpFile = new CaCompoundFile(strPath);

	DWORD grfMode1 = STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
	DWORD grfMode2 = STGM_DIRECT|STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
	IStorage* pPersistentStorage = NULL;

	BOOL bFound = m_pcpFile->FindElement(NULL, _T("PersistentStateStorage"), STGTY_STORAGE);
	if (bFound)
		pPersistentStorage = m_pcpFile->OpenStorage(NULL, _T("PersistentStateStorage"), grfMode1);
	else
	{
		if (bLoad)
			return ;

		pPersistentStorage = m_pcpFile->NewStorage (NULL, _T("PersistentStateStorage"), grfMode2);
	}
	if (!pPersistentStorage)
		return;
	bFound = m_pcpFile->FindElement(pPersistentStorage, lpszStreamName, STGTY_STREAM);
	if (bFound)
		m_bStateOk = OpenStream (pPersistentStorage, lpszStreamName);
	else
	{
		if (bLoad)
			return;
		m_bStateOk = CreateStream(pPersistentStorage, lpszStreamName);
	}
}

int
CaCompoundFile::CreateDirectoryRecursively(char *szDirectory)
{
    char    szBuffer[MAX_PATH] = "";
    char    szNewDirectory[MAX_PATH] = "";
    char    *p;
    int	    rc;

    if (!szDirectory[0])
	return(0);

    if (GetFileAttributes(szDirectory) != -1)
	return(1);

    sprintf(szNewDirectory, "%s", szDirectory);

    p = strtok(szNewDirectory, "\\");

    if (p)
	strcpy(szBuffer, p);

    while (p)
    {
	p = strtok(NULL, "\\");

	if (p)
	{
	    strcat(szBuffer, "\\");
	    strcat(szBuffer, p);
	    rc = CreateDirectory(szBuffer, NULL);
	}   
    }  /* while... */

    if (GetFileAttributes(szDirectory) != -1)
	return(1);
	else
	return(0);
}


BOOL 
IsDBATools(CString strII)
{
     HKEY hKey;
     char szKey[256];
     static int isDBATools=-1;
     char szData[1024];
     DWORD dwSize;

     if (isDBATools < 0)
     {
	isDBATools=0;
	sprintf(szKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", strII);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		dwSize=sizeof(szData);
		if (RegQueryValueEx(hKey, "dbatools", 0, 0, (BYTE*)szData, &dwSize)==ERROR_SUCCESS)
		{
			if (stricmp(szData, "1")==0)
			{
				isDBATools=1;
			}
		}
	}
     }	
     return isDBATools;				
}
