/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: repmm.c
**
**  Description:
**	This file becomes the DLL for the Ingres REPLICATOR Merge Module.
**	It contains custom actions that are required to be executed.
**
**	    ingres_set_rep_reg_entries	- Set the appropriate registry
**					  entries for the post install.
**	    ingres_remove_rep_files	- Removes directories created during
**					  the REPLICATOR post install.
**
**  History:
**	18-may-2001 (somsa01)
**	    Created.
**	22-may-2001 (somsa01)
**	    In ingres_set_rep_reg_entries(), grab and set the service
**	    info in the registry if not already set.
**	18-jun-2001 (somas01)
**	    Fixed up Modify operation.
**	24-jan-2002 (penga03)
**	    If install Ingres using response file, set registry entry "SilentInstall" 
**	    to "Yes", post installation process will be executed in silent mode.
**	28-jan-2002 (penga03)
**	    If this is a fresh install on Windows 9X, set registry entry "RebootNeeded" 
**	    to "Yes", user will be asked to reboot when post installation completes.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	18-jun-2003 (penga03)
**	    Do NOT set registry entries for REP if either of following two cases 
**	    happens: 
**	    1) Feature IngresReplicator is not published
**	    2) IngresReplicator is not and will not be installed
**	26-jun-2003 (penga03)
**	    In ingres_rollback_rep() and ingres_unset_rep_reg_entries(), reset 
**	    "InstalledFeatures" and "RemovedFeatures" to the registry when their 
**	    values is being changed.
**	20-oct-2003 (penga03)
**	    Replaced StringReverse() with DelTok().
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	07-sep-2004 (penga03)
**	   Removed all references to service password since by default 
**	   Ingres service is installed using Local System account.
**	15-sep-2004 (penga03)
**	   Backed out the last change (471628).
**	07-dec-2004 (penga03)
**	    Changed the formulation to generate GUIDs since the old one does
**	    not work for id from A1 to A9.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
**	11-jan-2005 (penga03)
**	    Set the registry entries if major upgrade.
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	16-Nov-2006 (drivi01)
**	    On upgrade will detect if existing service is set to
**	    start automatically and will preserve this behavior
**	    once upgrade has completed.
*/

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <MsiQuery.h>
#include "commonmm.h"


/*{
**  Name: ingres_set_rep_reg_entries
**
**  Description:
**	Sets the appropriate registry entries for the REPLICATOR Merge
**	Module.
**	These entries will drive the post installation for REPLICATOR.
**
**  Inputs:
**	ihnd		MSI environment handle.
**
**  Outputs:
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	18-may-2001 (somsa01)
**	    Created.
**	22-may-2001 (somsa01)
**	    Grab and set the service info in the registry if not already set.
**	14-jan-2002 (penga03)
**	    Set property INGRES_POSTINSTALLATIONNEEDED to "1" if post installation 
**	    is needed.
**	05-jun-2002 (penga03)
**	    ONLY if no other products referencing REP (by checking the 
**	    Bin component in REP merge module), then add it to 
**	    RemovedFeatures.
**	13-Mar-2007 (drivi01)
**	    Fixed check for existing service.  The code was always falling though
**	    and trying to perform upgrade on service.
*/

UINT __stdcall
ingres_set_rep_reg_entries (MSIHANDLE ihnd)
{
    HKEY		hKey;
    char		KeyName[256];
    int			status = 0;
    DWORD		dwSize, dwDisposition, dwType;
    TCHAR		tchValue[2048];
    INSTALLSTATE	iInstalled, iAction;
    BOOL    	bInstalled, bVer25, bVersion9X;
    char szID[3], szComponentGUID[39], szProductGUID[39];
	
    bInstalled=bVer25=bVersion9X=FALSE;

    if (!MsiGetFeatureState(ihnd, "IngresReplicator", &iInstalled, &iAction))
    {
	if (iInstalled==INSTALLSTATE_ABSENT && iAction==INSTALLSTATE_UNKNOWN) 
	    return ERROR_SUCCESS;
    }
    else 
	return ERROR_SUCCESS;

    dwSize = sizeof(tchValue);
    if (MsiGetProperty( ihnd, "II_INSTALLATION", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	sprintf(KeyName,
		"Software\\IngresCorporation\\Ingres\\%s_Installation",
		tchValue);
	status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, NULL,
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
				&hKey, &dwDisposition);
    }

    if (iInstalled == INSTALLSTATE_LOCAL)
    {
	/*
	** Set up the InstalledFeatures registry key for possible use.
	*/
	dwType = REG_SZ;
	dwSize = sizeof(tchValue);
	if (RegQueryValueEx(hKey, "InstalledFeatures", NULL, &dwType,
			    tchValue, &dwSize) != ERROR_SUCCESS)
	{
	    strcpy(tchValue, "REP");
	}
	else if (!strstr(tchValue, "REP"))
	    strcat(tchValue, ",REP");

	status = RegSetValueEx( hKey, "InstalledFeatures", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize=sizeof(szID);
    MsiGetProperty(ihnd, "II_INSTALLATION", szID, &dwSize);
    if (!_stricmp(szID, "II"))
	strcpy(szComponentGUID, "{531C8AD8-2F05-11D5-BDFC-00B0D0AD4485}");
    else
    {
	int idx;

	idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - 'A';
	if (idx <= 0) 
	    idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - '0';
	sprintf(szComponentGUID, "{531C8AD8-2F05-11D5-%04X-%012X}", idx, idx*idx);
    }

    dwSize=sizeof(szProductGUID);
    MsiGetProperty(ihnd, "ProductCode", szProductGUID, &dwSize);

    if (iAction==INSTALLSTATE_ABSENT && GetRefCount(szComponentGUID, szProductGUID)==0)
    {
	/*
	** Set up the RemovedFeatures registry key for use later on.
	*/
	dwType = REG_SZ;
	dwSize = sizeof(tchValue);
	if (RegQueryValueEx(hKey, "RemovedFeatures", NULL, &dwType,
			    tchValue, &dwSize) != ERROR_SUCCESS)
	{
	    strcpy(tchValue, "REP");
	}
	else if (!strstr(tchValue, "REP"))
	    strcat(tchValue, ",REP");

	status = RegSetValueEx( hKey, "RemovedFeatures", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );

	RegCloseKey(hKey);
	return (status==ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
    }
    else if (iInstalled == INSTALLSTATE_LOCAL || iAction != INSTALLSTATE_LOCAL)
    {
	dwSize = sizeof(tchValue);
	if (!MsiGetProperty(ihnd, "INGRES_UPGRADE", tchValue, &dwSize) && strcmp(tchValue, "2"))
	{
	    RegCloseKey(hKey);
	    return (ERROR_SUCCESS);
	}
    }

    strcpy(tchValue, "TRUE");
    status = RegSetValueEx( hKey, "Ingres/Replicator", 0, REG_SZ, 
			    (CONST BYTE *)&tchValue, strlen(tchValue) + 1 );

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "PostInstallationNeeded", NULL, &dwType,
			tchValue, &dwSize) != ERROR_SUCCESS)
    {
	dwSize = sizeof(tchValue);
	if (status ||
	    MsiGetProperty( ihnd, "INGRES_SERVICEAUTO", tchValue,
			    &dwSize ) != ERROR_SUCCESS)
	{
	    return (ERROR_INSTALL_FAILURE);
	}
	else
	{
	    char szBuf[MAX_PATH+1];
	    sprintf(szBuf, "Ingres_Database_%s", szID);
	    if (CheckServiceExists(szBuf))
	    {
	         if (IsServiceStartupTypeAuto(szBuf))
		    sprintf(tchValue, "1");
		 else
		    sprintf(tchValue, "0");
  	         
		 status = RegSetValueEx( hKey, "serviceauto", 0, REG_SZ, 
				    tchValue, strlen(tchValue) + 1 );
		
	    }
	    else
	    	 status = RegSetValueEx( hKey, "serviceauto", 0, REG_SZ, 
				    tchValue, strlen(tchValue) + 1 );
	}

	dwSize = sizeof(tchValue);
	if (status ||
	    MsiGetProperty( ihnd, "INGRES_SERVICELOGINID", tchValue,
			    &dwSize ) != ERROR_SUCCESS)
	{
	    return (ERROR_INSTALL_FAILURE);
	}
	else
	{
	    status = RegSetValueEx( hKey, "serviceloginid", 0, REG_SZ, 
				    tchValue, strlen(tchValue) + 1 );
	}

	dwSize = sizeof(tchValue);
	if (status ||
	    MsiGetProperty( ihnd, "INGRES_SERVICEPASSWORD", tchValue,
			    &dwSize ) != ERROR_SUCCESS)
	{
	    return (ERROR_INSTALL_FAILURE);
	}
	else
	{
	    /* Clear the password from the property table. */
	    MsiSetProperty(ihnd, "INGRES_SERVICEPASSWORD", "");
	    MsiSetProperty(ihnd, "INGRES_SERVICEPASSWORD2", "");

	    status = RegSetValueEx( hKey, "servicepassword", 0, REG_SZ, 
				    tchValue, strlen(tchValue) + 1 );
	}

	strcpy(tchValue, "YES");
	status = RegSetValueEx( hKey, "PostInstallationNeeded", 0, REG_SZ, 
				tchValue, strlen(tchValue) + 1 );
	MsiSetProperty(ihnd, "INGRES_POSTINSTALLATIONNEEDED", "1");

	dwSize = sizeof(tchValue);
	MsiGetProperty(ihnd, "INGRES_RSP_LOC", tchValue, &dwSize);
	if (tchValue[0] && strcmp(tchValue, "0"))
	{
	    strcpy(tchValue, "YES");
	    RegSetValueEx( hKey, "SilentInstall", 0, REG_SZ, (CONST BYTE *)&tchValue, strlen(tchValue) + 1 );
	}

	dwSize = sizeof(tchValue);
	MsiGetProperty(ihnd, "Installed", tchValue, &dwSize);
	if (tchValue[0])
	    bInstalled=TRUE;
	dwSize = sizeof(tchValue);
	MsiGetProperty(ihnd, "INGRES_VER25", tchValue, &dwSize);
	if (tchValue[0] && !strcmp(tchValue, "1"))
	    bVer25=0;
	dwSize=sizeof(tchValue);
	MsiGetProperty(ihnd, "Version9X", tchValue, &dwSize);
	if (tchValue[0])
	    bVersion9X=TRUE;
	if (!bInstalled && !bVer25 && bVersion9X)
	{
	    strcpy(tchValue, "YES");
	    RegSetValueEx( hKey, "RebootNeeded", 0, REG_SZ, (CONST BYTE *)&tchValue, strlen(tchValue) + 1 );
	}
    }

    return (status == ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
}

/*{
**  Name: ingres_remove_rep_files
**
**  Description:
**	Removes any files/directories created by the REPLICATOR Merge
**	Module when it was installed, as well as its specific registry
**	entries.
**
**  Inputs:
**	ihnd		MSI environment handle.
**
**  Outputs:
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	18-may-2001 (somsa01)
**	    Created.
**	03-Aug-2001 (penga03)
**	    Let the REPLICATOR Merge Module delete the files/directories created 
**	    by the post installation process.
*/

UINT __stdcall
ingres_remove_rep_files (MSIHANDLE ihnd)
{
    char		KeyName[256];
    HKEY		hKey;
    DWORD		status = 0;
    DWORD		dwSize, dwType;
    TCHAR		tchValue[2048], tchII_SYSTEM[MAX_PATH+1];

    dwSize = sizeof(tchValue);
    if (MsiGetProperty( ihnd, "CustomActionData", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }

    sprintf(KeyName,
	    "Software\\IngresCorporation\\Ingres\\%s_Installation",
	    tchValue);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0,
			  KEY_ALL_ACCESS, &hKey)) return ERROR_SUCCESS;

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "RemovedFeatures", NULL, &dwType,
			tchValue, &dwSize) != ERROR_SUCCESS)
    {
	/* Nothing is being removed. */
	RegCloseKey(hKey);
	return (ERROR_SUCCESS);
    }
    if (!strstr(tchValue, "REP"))
    {
	/* We're not being removed. */
	RegCloseKey(hKey);
	return (ERROR_SUCCESS);
    }

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, tchValue, &dwSize) !=
	ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    SetEnvironmentVariable("II_SYSTEM", tchValue);
    strcpy(tchII_SYSTEM, tchValue);
    RegCloseKey(hKey);

    /*
    ** Let's delete all directories that would have been created
    ** outside of InstallShield.
    */
    /*
    ** sprintf(tchValue, "%s\\ingres\\rep", tchII_SYSTEM);
    ** if (RemoveDir(tchValue, TRUE) != ERROR_SUCCESS)
    ** return (ERROR_INSTALL_FAILURE);
    */
    return (ERROR_SUCCESS);
}

/*
**  Name:ingres_rollback_rep
**
**  Description:
**	Reverse the changes to the system made by ingres_set_rep_reg_entries and 
**	ingres_remove_rep_files. Executed only during an installation rollback.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS			The function succeeds.
**	    ERROR_INSTALL_FAILURE	The function fails.
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	03-Aug-2001 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_rollback_rep(MSIHANDLE hInstall)
{
	char ii_code[3];
	DWORD ret;
	char RegKey[256], data[256];
	DWORD type, size;
	HKEY hkRegKey;
	BOOL bRemoved=0;
	
	ret=sizeof(ii_code);
	if(MsiGetProperty(hInstall, "CustomActionData", ii_code, &ret)!=ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;
	
	sprintf(RegKey, "Software\\CIngresCorporation\\Ingres\\%s_Installation", ii_code);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
		return ERROR_SUCCESS;

	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "InstalledFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "REP");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "REP");
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres/Replicator", NULL, &type, (BYTE *)data, &size)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}
	
	RegDeleteValue(hkRegKey, "Ingres/Replicator");
	RegDeleteValue(hkRegKey, "serviceauto");
	RegDeleteValue(hkRegKey, "serviceloginid");
	RegDeleteValue(hkRegKey, "servicepassword");
	RegDeleteValue(hkRegKey, "PostInstallationNeeded");
	RegDeleteValue(hkRegKey, "SilentInstall");
	RegDeleteValue(hkRegKey, "RebootNeeded");
	
	RegCloseKey(hkRegKey);
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_unset_rep_reg_entries
**
**  Description:
**	Reverse the changes to the system made by ingres_set_rep_reg_entries 
**	during windows installer is creating script.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS			The function succeeds.
**	    ERROR_INSTALL_FAILURE	The function fails.
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	21-Aug-2001 (penga03)
**	    Created.
**	21-mar-2002 (penga03)
**	    Changed the function always return ERROR_SUCCESS since
**	    installation already completes at this point.
*/
UINT __stdcall
ingres_unset_rep_reg_entries(MSIHANDLE hInstall)
{
	char ii_code[3];
	DWORD ret;
	char RegKey[256], data[256];
	DWORD type, size;
	HKEY hkRegKey;
	BOOL bRemoved=0;
	
	ret=sizeof(ii_code);
	if(MsiGetProperty(hInstall, "II_INSTALLATION", ii_code, &ret)!=ERROR_SUCCESS)
		return ERROR_SUCCESS;
	
	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", ii_code);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
		return ERROR_SUCCESS;

	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "InstalledFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "REP");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "REP");
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres/Replicator", NULL, &type, (BYTE *)data, &size)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}
	
	RegDeleteValue(hkRegKey, "Ingres/Replicator");
	RegDeleteValue(hkRegKey, "serviceauto");
	RegDeleteValue(hkRegKey, "serviceloginid");
	RegDeleteValue(hkRegKey, "servicepassword");
	RegDeleteValue(hkRegKey, "PostInstallationNeeded");
	RegDeleteValue(hkRegKey, "SilentInstall");
	RegDeleteValue(hkRegKey, "RebootNeeded");
	
	RegCloseKey(hkRegKey);
	return ERROR_SUCCESS;
}
