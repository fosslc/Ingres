/*
** Copyright (c) 1999, 2001 Ingres Corporation
*/

/*
**  Name: iigetver.cpp
**
**  Description:
**	This file contains the definition of II_GetVersion().
**
**  History:
**	23-mar-99 (cucjo01)
**	    Created
**	    Exported function II_GetVersion() returns version information
**	    in a structure to the caller.  The return code of the function
**	    is a value based on how the inquiry went and defined in ii_ver.h
**	    separately to be compatbile with C and C++.
**	24-aug-2000 (rodjo04)
**	    Added check to see if II_SYSTEM directory really exists. 
**	    If not, we will return a new error II_ERROR_II_SYSTEM_DIR_NOT_EXIST.
**	24-mar-99 (cucjo01)
**	    Added check for registry environment variable since the current
**	    environment may not know about the value.
**	27-dec-2001 (somsa01)
**	    If we cannot find the version of the DBMS, then return the version
**	    of NET, if possible.
*/

#include "stdafx.h"
#include "iigetver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CIIgetverApp
*/
BEGIN_MESSAGE_MAP(CIIgetverApp, CWinApp)
    //{{AFX_MSG_MAP(CIIgetverApp)
	    // NOTE - the ClassWizard will add and remove mapping macros here.
	    //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CIIgetverApp construction
*/
CIIgetverApp::CIIgetverApp()
{
}

/*
** The one and only CIIgetverApp object
*/
CIIgetverApp theApp;

/*
** This function will obtain the version of ingres installed on
** a machine as well as II_SYSTEM.
*/
int
CIIgetverApp::II_GetVersion(II_VERSIONINFO *VersionInfo)
{
    int			rc, iPos;
    CStdioFile		theInputFile;
    CFileFind		finder;
    CFileException	fileException;
    CString		strInputString, strII_SYSTEM, strSearchString;
    char		szBuffer[MAX_STRING_LEN];
    char		szFileName[MAX_PATH];
    bool		bFound = FALSE;
    
    if (!VersionInfo)
	return (II_ERROR_INVALID_PARAMETER_PASSED);
    
    VersionInfo->szII_SYSTEM[0] = 0;
    VersionInfo->szII_VERSIONSTRING[0] = 0;
    
    /* Find II_SYSTEM */
    if (GetEnvironmentVariable("II_SYSTEM", szBuffer, sizeof(szBuffer)))
    {
	strncpy(VersionInfo->szII_SYSTEM, szBuffer, MAX_STRING_LEN);
	bFound = TRUE;
    }
    else
    {
	HKEY	hEnvKey;     
	DWORD	dwValLen, dwDataType;
    
	rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,	/* main subkey */
			  "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",	/* root of env var */
			  (DWORD)0,		/* reserved */
			  KEY_ALL_ACCESS,	/* need rw access */
			  &hEnvKey );		/* return handle here */

	if (rc == ERROR_SUCCESS)	/* success? */
	{
	    dwValLen = sizeof(szBuffer);
	    rc = RegQueryValueEx(hEnvKey,			/* handle to open key */
				 "II_SYSTEM",			/* user path */
				 0,				/* reserved */
				 &dwDataType,			/* data type */
				 (unsigned char *)szBuffer,	/* the buffer */
				 &dwValLen);			/* buffer len */
    
	    if (rc == ERROR_SUCCESS)	/* success */
	    {
		strncpy(VersionInfo->szII_SYSTEM, szBuffer, MAX_STRING_LEN);
		bFound = TRUE;
	    }
    
	    if (hEnvKey)
		RegCloseKey(hEnvKey);
	} /* if (ERROR_SUCCESS) */
    }

    if (!bFound)
	return (II_ERROR_CANNOT_FIND_II_SYSTEM);

    strII_SYSTEM = szBuffer;
    
    /* Check if II_SYSTEM exists */
    BOOL bExist = finder.FindFile(strII_SYSTEM);
    if (!bExist)
	return (II_ERROR_II_SYSTEM_DIR_NOT_EXIST);
    
    /* Create search string */
    char szComputerName[512];
    ULONG ul = sizeof(szComputerName);
    GetComputerName(szComputerName, &ul);

    strSearchString.Format("ii.%s.config.dbms.", szComputerName);
    strSearchString.MakeLower();

    /* Check config.dat file */
    sprintf(szFileName, "%s\\ingres\\files\\config.dat", strII_SYSTEM.GetBuffer(255));
    if (!theInputFile.Open(szFileName, CFile::modeRead | CFile::typeText, &fileException)) 
	return (II_ERROR_CANNOT_OPEN_CONFIG_DAT);
    
    while (theInputFile.ReadString(strInputString) != FALSE)
    {  
	strInputString.TrimRight();
	strInputString.MakeLower();
	
	iPos = strInputString.Find(strSearchString);
	if (iPos >= 0)	/* Successfully found key */
	{
	    strInputString =
		strInputString.Right(strInputString.GetLength()-strSearchString.GetLength());
	    strInputString.TrimLeft();
	    
	    iPos = strInputString.Find(":");
	    if (iPos >= 0)	/* Successfully found value */
	    {
		strInputString = strInputString.Left(iPos);
		strInputString.TrimRight();

		strncpy(VersionInfo->szII_VERSIONSTRING, strInputString,
			MAX_STRING_LEN);
		theInputFile.Close();
		return(II_OK);
	    }
	} 
    }
    
    theInputFile.Close();

    /*
    ** We didn't find it. Let's try "config.net"...
    */
    strSearchString.Format("ii.%s.config.net.", szComputerName);
    strSearchString.MakeLower();

    /* Check config.dat file */
    sprintf(szFileName, "%s\\ingres\\files\\config.dat",
	    strII_SYSTEM.GetBuffer(255));
    if (!theInputFile.Open(szFileName,
			   CFile::modeRead | CFile::typeText,
			   &fileException)) 
    {
	return (II_ERROR_CANNOT_OPEN_CONFIG_DAT);
    }
    
    while (theInputFile.ReadString(strInputString) != FALSE)
    {  
	strInputString.TrimRight();
	strInputString.MakeLower();
	
	iPos = strInputString.Find(strSearchString);
	if (iPos >= 0)	/* Successfully found key */
	{
	    strInputString =
		strInputString.Right(strInputString.GetLength()-strSearchString.GetLength());
	    strInputString.TrimLeft();
	    
	    iPos = strInputString.Find(":");
	    if (iPos >= 0)	/* Successfully found value */
	    {
		strInputString = strInputString.Left(iPos);
		strInputString.TrimRight();

		strncpy(VersionInfo->szII_VERSIONSTRING, strInputString,
			MAX_STRING_LEN);
		theInputFile.Close();
		return(II_OK);
	    }
	} 
    }
    
    theInputFile.Close();

    /* Value not found in config.dat */
    return (II_ERROR_CANNOT_FIND_VERSION_STRING);
}
