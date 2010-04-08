/*
**  Copyright (c) 2001-2009 Ingres Corporation.
*/

/*
**  Name: docmm.c
**
**  Description:
**	This file becomes the DLL for the IngresDoc Merge Module. It
**	contains custom actions that are required to be executed.
**
**  History:
**	28-May-2001 (penga03)
**	    Created.
**	24-Oct-2001 (penga03)
**	    Windows Installer used to register components in the following key 
**	    HKLY\Software\Microsoft\Windows\Currentversion\Installer\Components, 
**	    however, after I reinstalled OS and ugraded Windows Installer from 1.0 
**	    to 2.0, the key doesn't exist any more. As a result, we can't share 
**	    IngresDoc among Ingres installations. Now each installation has its 
**	    own copy of IngresDoc. As for shortcuts of IngresDoc, it can be shared. 
**	    Function ingres_set_doc_path, don't just its function from the name, 
**	    now has the ability to make sure that the IngresDoc shortcuts always 
**	    point to one of the installation that has IngresDoc installed.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	06-feb-2002 (penga03)
**	    Removed ingres_set_doc_path, added ingres_set_doc_shortcuts.
**	14-may-2002 (penga03)
**	    Added CheckOutAIngresPropertyValue(), GetSharedDocInfo(), deleted 
**	    CreateOneDocShortcut(). 
**	    Use CreateOneShortcut() that is defined in commonmm.c to restore 
**	    documentation shortcuts. While restoring those shortcuts, we decide 
**	    whether or not the shortcuts should be restored, and whether or not 
**	    should be restored for current user or everyone by checking 
**	    IngresCreateIcons, IngresAllUsers saved in the corresponding 
**	    installation registry entry. 
**	06-feb-2004 (penga03)
**	    Added three new doc shortcuts: ingnet, ipm and rs, replaced esqlc 
**	    and esqlcob with esql.
**	01-mar-2004 (penga03)
**	    Added a doc shortcut for quelref.pdf.
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	16-Nov-2006 (drivi01)
**	    Added new functions for documentation installer.
**	    IsCurrentAcrobatAlreadyInstalled()
**	    InstallAcrobat()
**	    OpenPageForAcrobat()
**	    ingres_install_adobe_reader(MSIHANDLE)
**	    ingres_start_bookshelf(MSIHANDLE).
**	16-Jan-2009 (whiro01) SD Issue 132899
**	    In conjunction with the fix for this issue, fixed a few compiler warnings.
*/

#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include "commonmm.h"
#include <shlobj.h>
#include <objbase.h>

BOOL CheckOutAIngresPropertyValue(char *szId, char *szProperty);
BOOL GetSharedDocInfo(char *szCurId, char *szId, char *szPath);
BOOL IsCurrentAcrobatAlreadyInstalled();
VOID InstallAcrobat(char *szPath);

/*
**  Name:ingres_set_doc_shortcuts
**	
**  Description:
**	If the Ingres to which the documentation shortcuts currently link is 
**	being removed, modify the shortcuts to link to an existing Ingres.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	06-feb-2002 (penga03)
**	    Created.
**	12-aug-2002 (penga03)
**	    Made the corresponding change on Ingres installer for SIR 
**	    108421, two embedded specific documentation shortcuts has 
**	    been added.
*/
UINT __stdcall 
ingres_set_doc_shortcuts(MSIHANDLE hInstall) 
{
    char szCurId[16], szId[3], szPath[MAX_PATH];
    DWORD dwSize;
    char szBuf[MAX_PATH];
 
    /* CustomActionData: [II_INSTALLATION] */
    dwSize=sizeof(szCurId);
    MsiGetProperty(hInstall, "CustomActionData", szCurId, &dwSize);  
    strcat(szCurId, "_Installation");

    if(GetSharedDocInfo(szCurId, szId, szPath))
    {
	char szFolder[MAX_PATH], szTarget[MAX_PATH];
	BOOL bAllUsers;

	bAllUsers=CheckOutAIngresPropertyValue(szId, "IngresAllUsers");

	strcpy(szFolder, "Computer Associates\\Ingres Documentation");
	sprintf(szTarget, "\"%sgs.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Getting Started", szTarget, "", 
	                  "", szPath, "Getting Started", 0, bAllUsers);

	sprintf(szTarget, "\"%smg.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Migration Guide", szTarget, "", 
	                  "", szPath, "Migration Guide", 0, bAllUsers);

	sprintf(szTarget, "\"%some.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Object Management Extension User Guide", szTarget, "", 
	                  "", szPath, "Object Management Extension User Guide", 0, bAllUsers);

	sprintf(szTarget, "\"%scmdref.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Command Reference Guide", szTarget, "", 
	                  "", szPath, "Command Reference Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%sdba.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Database Administrator Guide", szTarget, "", 
	                  "", szPath, "Database Administrator Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%sdtp.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Distributed Transaction Processing User Guide", szTarget, "", 
	                  "", szPath, "Distributed Transaction Processing User Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%sesql.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Embedded SQL Companion Guide", szTarget, "", 
		              "", szPath, "Embedded SQL Companion Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%siceug.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Web Deployment Option User Guide", szTarget, "", 
	                  "", szPath, "Web Deployment Option User Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%soapi.pdf\"", szPath);
	CreateOneShortcut(szFolder, "OpenAPI User Guide", szTarget, "", 
	                  "", szPath, "OpenAPI User Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%sosql.pdf\"", szPath);
	CreateOneShortcut(szFolder, "OpenSQL Reference Guide", szTarget, "", 
	                  "", szPath, "OpenSQL Reference Guide", 0, bAllUsers);

	sprintf(szTarget, "\"%sequel.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Embedded QUEL Companion Guide", szTarget, "", 
	                  "", szPath, "Embedded QUEL Companion Guide", 0, bAllUsers);

	sprintf(szTarget, "\"%squelref.pdf\"", szPath);
	CreateOneShortcut(szFolder, "QUEL Reference Guide", szTarget, "", 
	                  "", szPath, "QUEL Reference Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%srep.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Replication Option User Guide", szTarget, "", 
	                  "", szPath, "Replication Option User Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%ssqlref.pdf\"", szPath);
	CreateOneShortcut(szFolder, "SQL Reference Guide", szTarget, "", 
	                  "", szPath, "SQL Reference Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%ssysadm.pdf\"", szPath);
	CreateOneShortcut(szFolder, "System Administrator Guide", szTarget, "", 
	                  "", szPath, "System Administrator Guide", 0, bAllUsers);

	sprintf(szTarget, "\"%sstarug.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Distributed Option User Guide", szTarget, "", 
	                  "", szPath, "Distributed Option User Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%sqrytools.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Character-based Querying and Reporting Tools User Guide", szTarget, "", 
	                  "", szPath, "Character-based Querying and Reporting Tools User Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%sufadt.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Forms-based Application Development Tools User Guide", szTarget, "", 
	                  "", szPath, "Forms-based Application Development Tools User Guide", 0, bAllUsers);

	sprintf(szTarget, "\"%singnet.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Connectivity Guide", szTarget, "", 
	                  "", szPath, "Connectivity Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%sipm.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Interactive Performance Monitor User Guide", szTarget, "", 
	                  "", szPath, "Interactive Performance Monitor User Guide", 0, bAllUsers);
	
	sprintf(szTarget, "\"%srs.pdf\"", szPath);
	CreateOneShortcut(szFolder, "Release Summary", szTarget, "", 
	                  "", szPath, "Release Summary", 0, bAllUsers);

	sprintf(szBuf, "%sesqlref.pdf", szPath);
	if (GetFileAttributes(szBuf)!=-1)
	{
	    sprintf(szTarget, "\"%sesqlref.pdf\"", szPath);
	    CreateOneShortcut(szFolder, "Embedded Ingres SQL Reference Guide", szTarget, "", 
	                      "", szPath, "Embedded Ingres SQL Reference Guide", 0, bAllUsers);

	    sprintf(szTarget, "\"%sesysadm.pdf\"", szPath);
	    CreateOneShortcut(szFolder, "Embedded Ingres Administrator's Guide", szTarget, "", 
	                      "", szPath, "Embedded Ingres Administrator's Guide", 0, bAllUsers);
	}

    }
    return ERROR_SUCCESS;
}


/*
**  Name:ingres_remove_shortcuts
**	
**  Description:
**	Removes shorcuts from current user account.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	18-Mar-2007 (drivi01)
**	    Created.
**	16-Jun-2008 (drivi01)
**	    Rename "Ingres 2006 Documentation" directory to 
**	    "Ingres Documentation 9.2".
**	14-Nov-2008 (Drivi01)
**	    Update routine to pull documentation version from
**	    commonmm.c.
*/
UINT __stdcall 
ingres_remove_shortcuts(MSIHANDLE hInstall) 
{
	char pathBuf[MAX_PATH];
	char szPath[MAX_PATH], szPath2[MAX_PATH];	
	LPITEMIDLIST pidlProg, pidlComProg;

    	SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlProg);
    	SHGetPathFromIDList(pidlProg, szPath);
	SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlComProg);
    	SHGetPathFromIDList(pidlComProg, szPath2);
	sprintf(pathBuf, "%s\\Ingres\\Ingres Documentation %s", szPath, ING_VERS);
	if (GetFileAttributes(pathBuf) != -1)
	{
		if (RemoveOneDir(pathBuf))
			SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, pathBuf, 0);
	}

	MsiSetTargetPath(hInstall, "ProgramMenuFolder", szPath2);

	return ERROR_SUCCESS;
	

}

BOOL 
GetSharedDocInfo(char *szCurId, char *szId, char *szPath)
{
    char szKey[256], file2check[MAX_PATH];
    HKEY hKey;
    int i;

    strcpy(szKey, "SOFTWARE\\IngresCorporation\\Ingres");
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;

	for(i=0, retCode=ERROR_SUCCESS; retCode==ERROR_SUCCESS; i++) 
	{
	    char SubKey[16];
	    DWORD dwSize=sizeof(SubKey);

	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode && _stricmp(SubKey, szCurId))
	    {
		HKEY hSubKey;

		if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    char szII_SYSTEM[MAX_PATH];
		    DWORD dwType, dwSize=sizeof(szII_SYSTEM);
			
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)szII_SYSTEM,&dwSize))
		    {
			SubKey[2]='\0';
			sprintf(szId, "%s", SubKey);

			sprintf(file2check, "%s\\ingres\\files\\english\\gs.pdf", szII_SYSTEM);

			if (GetFileAttributes(file2check)!=-1 && 
			    CheckOutAIngresPropertyValue(szId, "IngresCreateIcons"))
			{
			    sprintf(szPath, "%s\\ingres\\files\\english\\", szII_SYSTEM);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hKey);
			    return TRUE;
			}						
		    }

		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */

	RegCloseKey(hKey);
    }

    return FALSE;
}

BOOL 
CheckOutAIngresPropertyValue(char *szId, char *szProperty)
{
    char szKey[256];
    HKEY hKey;
    int i;
    BOOL bValue=FALSE;

    strcpy(szKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;
	
	for (i=0, retCode = ERROR_SUCCESS; retCode == ERROR_SUCCESS; i++) 
	{
	    char SubKey[256];
	    DWORD dwSize=sizeof(SubKey);
	    
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {			
		HKEY hSubKey;

		if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    char szData[256];
		    DWORD dwType, dwSize=sizeof(szData);
		    
		    if (!RegQueryValueEx(hSubKey,"IngresInstallationID",
		                         NULL,&dwType,(BYTE *)szData,&dwSize)
		       && !_stricmp(szData, szId))
		    {
			char szData[2];
			DWORD dwType, dwSize=sizeof(szData);
			
			if (!RegQueryValueEx(hSubKey, szProperty,NULL,&dwType,
			                   (BYTE *)szData,&dwSize) 
			   && !strcmp(szData, "1"))
			{
			    bValue=TRUE;
			}

			RegCloseKey(hSubKey);
			RegCloseKey(hKey);
			return bValue;
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hKey);
    }

    return bValue;
}


/*
**  Name: IsCurrentAcrobatAlreadyInstalled
**	
**  Description:
**	Checks if Adobe Acrobat 7.0 is installed.
**
**  Side Effects:
**	None.
**
**  History:
**	16-Nov-2006 (drivi01)
**	    Created.
**  19-May-2008 (drivi01)
**		Update this routine to check for version of Acrobat Reader 
**		7.0 or above.
*/
BOOL
IsCurrentAcrobatAlreadyInstalled()
{
    HKEY	hKey = 0;
    int		rc;
    DWORD 	dwSize=0;
    char 	regKey[256];
	int		ret_code = FALSE;

#ifdef WIN64
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Wow6432Node\\Adobe\\Acrobat Reader", 0,
		      /*KEY_QUERY_VALUE*/KEY_READ, &hKey );
#else
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Adobe\\Acrobat Reader", 0,
		      /*KEY_QUERY_VALUE*/KEY_READ, &hKey );
#endif  /* WIN64 */

	if (rc == ERROR_SUCCESS)
    {
		int count=0;
		dwSize = sizeof(regKey);
		while ((rc = RegEnumKey(hKey, count, regKey, dwSize)) == ERROR_SUCCESS)
		{
			float vers;
			count++;
			sscanf(regKey, "%f", &vers);
			if (vers >= 7.0)
			{
				ret_code = TRUE;
				break;
			}
		}
    }

    if (rc == ERROR_SUCCESS)
    {
	if (hKey)
	    RegCloseKey(hKey);
	return ret_code;
    }
    else
	return ret_code;
}
/*
**  Name: InstallAcrobat
**	
**  Description:
**	This function installs Adobe Acrobat.  (Currently absolete)
**
**  Side Effects:
**	None.
**
**  History:
**	16-Nov-2006 (drivi01)
**	    Created.
*/

void
InstallAcrobat(char *szPath)
{
    char fullPath[MAX_PATH];

    sprintf(fullPath, "%s\\temp\\AdbeRdr707_DLM_en_US.exe", szPath);

    system(fullPath);
}

/*
**  Name: OpenPageForAcrobat
**	
**  Description:
**	This function opens a web browser to the page where
**	Adobe Acrobat can be downloaded.  
**
**  Side Effects:
**	None.
**
**  History:
**	16-Nov-2006 (drivi01)
**	    Created.
*/
void
OpenPageForAcrobat()
{
     HKEY 	hKey = 0;
     int	rc;
     DWORD	dwType, dwSize;
     char	browser[MAX_PATH+1];
     char 	cmd [MAX_PATH+1];

     rc = RegOpenKeyEx(HKEY_CLASSES_ROOT, "HTTP\\shell\\open\\command", 0, KEY_QUERY_VALUE, &hKey);
     if (rc == ERROR_SUCCESS)
     {
	dwSize=sizeof(browser);
	if (RegQueryValueEx(hKey, NULL, NULL, &dwType, (BYTE *)&browser, &dwSize) == ERROR_SUCCESS)
	{
		sprintf(cmd, "%s %s", browser, "http:\\\\www.adobe.com");
		Execute(cmd, TRUE);
		//CreateProcess(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		RegCloseKey(hKey);
	}

     }
	

}

/*
**  Name: ingres_install_adobe_reader
**	
**  Description:
**	This function is responsible for checking for existance of
**	Adobe Reader and will suggest to install it if it isn't detected.
**	This function has been modified to open just a page browser instead
**	of redistributing the Adobe Reader. 
**
**  Side Effects:
**	None.
**
**  History:
**	16-Nov-2006 (drivi01)
**	    Created.
**	19-May-2008 (drivi01)
**	    Remove locale routines.  Currently we have no
**	    way to pull up locale dependent adobe site, so 
**	    we will point everyone at adobe.com and they
**	    can choose to go to the locale of their choice.
**	16-Jan-2009 (whiro01)
**	    Removed now unused local variables (clears compiler warnings).
*/
UINT __stdcall 
ingres_install_adobe_reader(MSIHANDLE hInstall)
{
	if (!IsCurrentAcrobatAlreadyInstalled())
	{
	    if (AskUserYN(hInstall, "The Ingres documentation is provided in Portable Document Format (PDF).\nTo view PDF files, you must download and install Adobe Reader from the\nAdobe website if it is not already installed on your computer.\n\nWould you like to go to the Adobe website to download Adobe Reader?\n(Version 7.0 or above is required.)"))
		OpenPageForAcrobat();
	    else
		MyMessageBox(hInstall, "Please obtain the correct version of the Adobe Acrobat Reader from http://www.adobe.com.");
	}
	return ERROR_SUCCESS;
}


/*
**  Name: ingres_start_bookshelf
**	
**  Description:
**	This function is responsible for finding adobe acrobat 
**	and using it to open Bookshelf upon install completion.
**
**  Side Effects:
**	None.
**
**  History:
**	16-Nov-2006 (drivi01)
**	    Created.
**	19-Feb-2009 (drivi01)
**	    Rename Ingres Bookshelf.pdf to Ing_Bookshelf.pdf.
*/
UINT __stdcall
ingres_start_bookshelf(MSIHANDLE ihnd)
{
	char 	szPath[MAX_PATH];
	char 	szBuf[MAX_PATH];
	char	acrord[MAX_PATH];
	DWORD	dwSize, dwType;
	PROCESS_INFORMATION	pi;
    	STARTUPINFO		si;
	HKEY	hKey;
	int 	rc;
	char	subKey[MAX_PATH];
	memset((char*)&pi, 0, sizeof(pi));
	memset((char*)&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;
	dwSize = sizeof(szPath);
	//MsiGetTargetPath(ihnd, "DocFolder", szPath, &dwSize);
	MsiGetProperty(ihnd, "INSTALLDIR", szPath, &dwSize);
	if (_access(szPath, 00) == 0)
	{
		rc = RegOpenKeyEx(HKEY_CLASSES_ROOT,".pdf\\OpenWithList", 0,KEY_ENUMERATE_SUB_KEYS, &hKey );
		if (rc == ERROR_SUCCESS)
		{	
			dwSize = sizeof(subKey);
			rc = RegEnumKey(hKey, 0, subKey, dwSize);
			RegCloseKey(hKey);
			if (rc == ERROR_SUCCESS)
			{
				sprintf(szBuf, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s", subKey);
				rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf,  0, KEY_ALL_ACCESS, &hKey );
				if (rc == ERROR_SUCCESS)
				{
					dwSize = sizeof(acrord);
					rc = RegQueryValueEx(hKey, "Path", NULL, &dwType, acrord, &dwSize);
					RegCloseKey(hKey);
				}
			}
			if (rc == ERROR_SUCCESS)
			{
				sprintf(szBuf, "\"%s\\%s\" \"%s\\Ing_Bookshelf.pdf\"", acrord, subKey, szPath);
				if (CreateProcess(NULL, szBuf, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
				{
					/*Sleep(1000);
					sprintf(szBuf, "Ingres %s Post Installation", tchII_INSTALLATION);
					hWnd=FindWindow(NULL, szBuf);
					if (hWnd)
						BringWindowToTop(hWnd);
					*/
		
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					return (ERROR_SUCCESS);
				}
				else 
				{
					sprintf(szBuf, "Failed to execute \"%s\\Ing_Bookshelf.pdf\" with error %d", szPath, GetLastError());
					MyMessageBox(ihnd, szBuf);
					return (ERROR_INSTALL_FAILURE);
				}
			}
		}
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		return (ERROR_INSTALL_FAILURE);
	}
	
}

