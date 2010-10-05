/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: odbcmm.c
**
**  Description:
**	This file becomes the DLL for the Ingres ODBC Merge Module. It
**	contains custom actions that are required to be executed.
**
**	    ingres_set_odbc_reg_entries	- Set the appropriate registry
**					  entries for the post install.
**	    ingres_remove_odbc_files	- Removes directories created during
**					  the ODBC post install.
**
**  History:
**	18-may-2001 (somsa01)
**	    Created.
**	22-may-2001 (somsa01)
**	    In ingres_set_odbc_reg_entries, grab and set the service
**	    info in the registry if not already set.
**	18-jun-2001 (somsa01)
**	    The feature name for NET is "IngresNet". Also, in
**	    ingres_set_odbc_reg_entries(), modified the conditions to
**	    "uninstall" ODBC.
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
**	26-feb-2004 (penga03)
**	    Before, Ingres ODBC was installed in system32 directory, we let Windows 
**	    Installer handle installing Ingres ODBC. Now, the destination of Ingres 
**	    ODBC is changed to %II_SYSTEM%\ingres\bin, Windows can not install Ingres 
**	    ODBC properly in such a situation as multiple installations. So now we are 
**	    back our old solution to create custom actions, together with 
**	    iipostinst.exe, to do the job.
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
**	30-Jan-2007 (drivi01)
**	    Updated registry entry for Ingres 3.0 ODBC driver to be Ingres 2006
**	    in release 2.
**	16-Jun-2008 (drivi01)
**	    Update "Ingres 2006" ODBC driver with "Ingres 9.2".
**	01-Sep-2010 (drivi01) 
**	    ODBC driver design has changed, ODBC is now installed 1 driver 
**	    per installation.  We will now install ODBC driver marked by 
**	    installation id instead of version number. 
*/

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <MsiQuery.h>
#include "commonmm.h"

typedef BOOL (WINAPI *PSQLREMOVEDRIVERPROC)(LPCSTR, BOOL, LPDWORD);

/*
**  Name:SetODBCEnvironment
**
**  Description:
**	Set ODBC environment. Called by ingres_rollback_odbc.
**
**  History:
**	03-Aug-2001 (penga03)
**	    Created.
**	10-Jun-2008 (drivi01)
**	    Updated this action to install a few more fields in the registry
**	    to keep track of read-only and read/write odbc driver.
**	14-Nov-2008 (drivi01)
**	    Pull Ingres version from commonmm.c
*/
void
SetODBCEnvironment(BOOL setup_IngresODBC, BOOL setup_IngresODBCReadOnly, BOOL bRollback)
{
	char szWindowsDir[512], szII_SYSTEM[MAX_PATH+1];
	char szODBCINIFileName[1024], szODBCDriverFileName[1024], szODBCSetupFileName[1024];
	char szODBCReg[MAX_PATH+1], szDrvName[MAX_PATH];
	char szODBCReadOnly[2];
	HKEY hKey;
	DWORD dwDisposition, dwUsageCount, dwType, dwSize;

	if (!setup_IngresODBC)
		return;

	*szWindowsDir=0;
	GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir));
	sprintf(szODBCINIFileName, "%s\\ODBCINST.INI", szWindowsDir);
	*szII_SYSTEM=0;
	GetEnvironmentVariable("II_SYSTEM", szII_SYSTEM, sizeof(szII_SYSTEM));
	if (setup_IngresODBC && !setup_IngresODBCReadOnly)
	{
		sprintf(szODBCDriverFileName, "%s\\ingres\\bin\\caiiod35.dll", szII_SYSTEM);
		sprintf(szODBCReadOnly, "N");
	}
	if (setup_IngresODBC && setup_IngresODBCReadOnly)
	{
		sprintf(szODBCDriverFileName, "%s\\ingres\\bin\\caiiro35.dll", szII_SYSTEM);
		sprintf(szODBCReadOnly, "Y");
	}
	sprintf(szODBCSetupFileName,  "%s\\ingres\\bin\\caiioc35.dll", szII_SYSTEM);
	if (setup_IngresODBC && !setup_IngresODBCReadOnly)
		sprintf(szODBCReg, "SOFTWARE\\ODBC\\ODBCINST.INI\\Ingres");
	if (setup_IngresODBC && setup_IngresODBCReadOnly)
		sprintf(szODBCReg, "SOFTWARE\\ODBC\\ODBCINST.INI\\Ingres Read Only");
	if (!RegCreateKeyEx(HKEY_LOCAL_MACHINE, szODBCReg, 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
	{
		RegSetValueEx(hKey, "Driver", 0, REG_SZ, (CONST BYTE *)szODBCDriverFileName, strlen(szODBCDriverFileName)+1);
		RegSetValueEx(hKey, "Setup", 0, REG_SZ, (CONST BYTE *)szODBCSetupFileName, strlen(szODBCSetupFileName)+1);
		RegSetValueEx(hKey, "APILevel", 0, REG_SZ, (CONST BYTE *)"1", strlen("1")+1);
		RegSetValueEx(hKey, "ConnectFunctions", 0, REG_SZ, (CONST BYTE *)"YYN", strlen("YYN")+1);
		RegSetValueEx(hKey, "DriverODBCVer", 0, REG_SZ, (CONST BYTE *)"03.50", strlen("03.50")+1);
		RegSetValueEx(hKey, "DriverReadOnly", 0, REG_SZ, (CONST BYTE *)szODBCReadOnly, strlen(szODBCReadOnly)+1);
		RegSetValueEx(hKey, "DriverType", 0, REG_SZ, (CONST BYTE *)"Ingres", strlen("Ingres")+1);
		RegSetValueEx(hKey, "FileUsage", 0, REG_SZ, (CONST BYTE *)"0", strlen("0")+1);
		RegSetValueEx(hKey, "SQLLevel", 0, REG_SZ, (CONST BYTE *)"0", strlen("0")+1);
		RegSetValueEx(hKey, "CPTimeout", 0, REG_SZ, (CONST BYTE *)"60", strlen("60")+1);

		dwSize = ( DWORD ) sizeof ( dwUsageCount);
		if (RegQueryValueEx(hKey, "UsageCount", NULL, &dwType, (LPBYTE)&dwUsageCount, &dwSize))
			dwUsageCount = 1;
		else
		{
			if (bRollback) dwUsageCount = dwUsageCount + 1;
		}
		RegSetValueEx(hKey, "UsageCount", 0, REG_DWORD, (LPBYTE)&dwUsageCount, sizeof(DWORD));
		RegSetValueEx(hKey, "Vendor", 0, REG_SZ, (CONST BYTE *)"Ingres Corporation", strlen("Ingres Corporation")+1);

		RegCloseKey(hKey);
	}


	if (!RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ODBC\\ODBCINST.INI\\ODBC Drivers", 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
	{
		RegSetValueEx(hKey, "Ingres", 0, REG_SZ, (CONST BYTE *)"Installed", strlen("Installed")+1);
		RegCloseKey(hKey);
	}

	WritePrivateProfileString("ODBC 32 bit Drivers", "Ingres (32 bit)", "Installed", szODBCINIFileName);
	WritePrivateProfileString("Ingres (32 bit)", "Driver", szODBCDriverFileName, szODBCINIFileName);
	WritePrivateProfileString("Ingres (32 bit)", "Setup", szODBCSetupFileName, szODBCINIFileName);
	WritePrivateProfileString("Ingres (32 bit)", "32Bit", "1", szODBCINIFileName);


	return;
}

/*
**  Name:IsReadOnlyDriver
**
**  Description:
**	Checks registry before it is removed to figure out if read-only driver is being used
**	in case of rollback.
**
**  History:
**	10-Jun-2008 (drivi01)
**	    Created.
*/
int
IsReadOnlyDriver(char *cur_id)
{
	char szII_SYSTEM[MAX_PATH+1], szKeyLoc[MAX_PATH], szKeyLoc2[MAX_PATH];
	HKEY hKey;
	DWORD dwSize, dwType;
	TCHAR tchValue[2048];
	int ret_val=FALSE;
	

	*szII_SYSTEM=0;
	*tchValue=0;
	GetEnvironmentVariable("II_SYSTEM", szII_SYSTEM, sizeof(szII_SYSTEM));

	sprintf(szKeyLoc, "SOFTWARE\\ODBC\\ODBCINST.INI\\Ingres Read Only");
	sprintf(szKeyLoc2, "SOFTWARE\\ODBC\\ODBCINST.INI\\Ingres %s Read Only", cur_id);	
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyLoc, 0, KEY_QUERY_VALUE | KEY_ALL_ACCESS, &hKey) ||
		!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyLoc2, 0, KEY_QUERY_VALUE | KEY_ALL_ACCESS, &hKey))
	{
		dwSize = sizeof(tchValue);
		if (RegQueryValueEx(hKey, "Driver", NULL, &dwType, (LPBYTE)tchValue, &dwSize) == ERROR_SUCCESS)
		{
			if (strstr(tchValue, szII_SYSTEM) != NULL)
				if (strstr(tchValue, "caiiro") != NULL) 
					ret_val=TRUE;
		}
		RegCloseKey(hKey);
	}
	
	return ret_val;
}


/*{
**  Name: ingres_set_odbc_reg_entries
**
**  Description:
**	Sets the appropriate registry entries for the ODBC Merge Module.
**	These entries will drive the post installation for ODBC.
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
**	18-jun-2001 (somsa01)
**	    The feature name for NET is "IngresNet". Also, modified the
**	    conditions to "uninstall" ODBC.
**	03-Aug-2001 (penga03)
**	    Set remove_ingresodbc, remove_ingresodbcreadonly, setup_ingresodbc 
**	    and setup_ingresodbcreadonly in the registry while adding ODBC into 
**	    RemovedFeatures, in case that if perform rollback, ingres_rollback_odbc 
**	    will use them to reset ODBC environment.
**	14-jan-2002 (penga03)
**	    Set property INGRES_POSTINSTALLATIONNEEDED to "1" if post installation 
**	    is needed.
**	13-Mar-2007 (drivi01)
**	    Fixed check for existing service.  The code was always falling though
**	    and trying to perform upgrade on service.
**	10-Jun-2008 (drivi01)
**	    Added routines to better detect existing read-only driver that could
**	    of been installed previously by a different installations or the
**	    the one that is possibly being upgraded now.
*/

UINT __stdcall
ingres_set_odbc_reg_entries (MSIHANDLE ihnd)
{
    HKEY		hKey, hKey2;
    char		KeyName[256], KeyNameODBC[256];
    int			status = 0;
    DWORD		dwSize, dwDisposition, dwType;
    TCHAR		tchValue[2048];
    INSTALLSTATE	iInstalled, iAction;
    BOOL		ODBC_Installed = FALSE;
    BOOL    	bInstalled, bVer25, bVersion9X;
    char szCode[3], szComponent[39], szProduct[39];
	
    ODBC_Installed = FALSE;
    bInstalled=bVer25=bVersion9X=FALSE;

    if (!MsiGetComponentState(ihnd, "Bin.70DC58B6_2D77_11D5_BDFC_00B0D0AD4485", &iInstalled, &iAction))
    {
	if (iInstalled==INSTALLSTATE_ABSENT && iAction==INSTALLSTATE_UNKNOWN)
	    return ERROR_SUCCESS;
    }
    else return ERROR_SUCCESS;

    dwSize = sizeof(tchValue);
    if (MsiGetProperty( ihnd, "II_INSTALLATION", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	strcpy(szCode, tchValue);
	sprintf(KeyName,
		"Software\\IngresCorporation\\Ingres\\%s_Installation",
		tchValue);
	status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, NULL,
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
				&hKey, &dwDisposition);
    }

    dwSize = sizeof(szProduct);
    MsiGetProperty(ihnd, "ProductCode", szProduct, &dwSize);
    if (!_stricmp(szCode, "II"))
	strcpy(szComponent, "{8FA72934-D481-405A-AD90-B7BDE7988247}");
    else
    {
	int idx;

	idx = (toupper(szCode[0]) - 'A') * 26 + toupper(szCode[1]) - 'A';
	if (idx <= 0) 
	    idx = (toupper(szCode[0]) - 'A') * 26 + toupper(szCode[1]) - '0';
	sprintf(szComponent, "{8FA72934-D481-405A-%04X-%012X}", idx, idx*idx);
    }
    dwSize=sizeof(tchValue);
    MsiGetComponentPath(szProduct, szComponent, tchValue, &dwSize);
    if (tchValue[0] && iInstalled==INSTALLSTATE_LOCAL)
	ODBC_Installed = TRUE;

    if (ODBC_Installed)
    {
	/*
	** Set up the InstalledFeatures registry key for possible use.
	*/
	dwType = REG_SZ;
	dwSize = sizeof(tchValue);
	if (RegQueryValueEx(hKey, "InstalledFeatures", NULL, &dwType,
			    tchValue, &dwSize) != ERROR_SUCCESS)
	{
	    strcpy(tchValue, "ODBC");
	}
	else
	    strcat(tchValue, ",ODBC");

	status = RegSetValueEx( hKey, "InstalledFeatures", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }


    if (iAction==INSTALLSTATE_ABSENT)
    {
	/*
	** Set up the RemovedFeatures registry key for use later on.
	*/
	dwType = REG_SZ;
	dwSize = sizeof(tchValue);
	if (RegQueryValueEx(hKey, "RemovedFeatures", NULL, &dwType,
			    tchValue, &dwSize) != ERROR_SUCCESS)
	{
	    strcpy(tchValue, "ODBC");
	}
	else
	    strcat(tchValue, ",ODBC");

	status = RegSetValueEx( hKey, "RemovedFeatures", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );

	RegCloseKey(hKey);
	return (status==ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
    }
    else if (ODBC_Installed || iAction!=INSTALLSTATE_LOCAL)
    {
	dwSize = sizeof(tchValue);
	if (!MsiGetProperty(ihnd, "INGRES_UPGRADE", tchValue, &dwSize) && strcmp(tchValue, "2"))
	{
	    RegCloseKey(hKey);
	    return (ERROR_SUCCESS);
	}
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_ODBC", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "setup_ingresodbc", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_ODBC_READONLY", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "setup_ingresodbcreadonly", 0,
				REG_SZ, tchValue, strlen(tchValue) + 1 );
    }

	//check if read-only odbc driver was installed by ODBC patch installer
	if ((!strcmp(tchValue, "") || !strcmp(tchValue, "0")) && ODBC_Installed && iAction==INSTALLSTATE_LOCAL)
	{
		sprintf(KeyNameODBC, "SOFTWARE\\ODBC\\ODBCINST.INI\\Ingres");
		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyNameODBC, 0, KEY_QUERY_VALUE, &hKey2))
		{
			dwSize=sizeof(tchValue);
			if (!RegQueryValueEx(hKey2, "Driver", 0, NULL, (BYTE *)tchValue, &dwSize))
			{
				if (strstr(tchValue, "caiiro") > 0) 
				{
					status = RegSetValueEx( hKey, "setup_ingresodbcreadonly", 0,
							REG_SZ, "1", strlen("1") + 1 );
					//Update the property for future possible upgrades of the release
					MsiSetProperty( ihnd, "INGRES_ODBC_READONLY", "1" );
				}
			}
			RegCloseKey(hKey2);
		}
	}

    strcpy(tchValue, "TRUE");
    status = RegSetValueEx( hKey, "Ingres ODBC Driver", 0, REG_SZ, 
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
	    sprintf(szBuf, "Ingres_Database_%s", szCode);
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
**  Name: ingres_remove_odbc_files
**
**  Description:
**	Removes any files/directories created by the ODBC Merge Module
**	when it was installed, as well as its specific registry entries.
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
**	22-apr-2001 (somsa01)
**	    Created.
**	07-may-2001 (somsa01)
**	    Do not remove the registry entries, as they are now removed
**	    by the post installation process. Also, grab II_INSTALLATION
**	    via the CustomActionData property, since this is a commit-time
**	    custom action.
**	10-Jun-2008 (drivi01)
**	    Add code for removing Ingres and Ingres 2006 ODBC driver entries.
**	17-11-2008 (drivi01)
**	    Update removal of ODBC driver to fetch version # automatically
**	    from commonmm.h.
*/

UINT __stdcall
ingres_remove_odbc_files (MSIHANDLE ihnd)
{
    char	KeyName[256];
    HKEY	hKey;
    DWORD	status = 0;
    DWORD	dwSize, dwType, dwError;
    TCHAR	tchValue[2048], tchValue2[2048];
    HINSTANCE	hLib;
    char	cur_id[3], id[3], ii_system[2048];
	char szDrvName[MAX_PATH], szDrvName2[MAX_PATH];
	char szWindowsDir[MAX_PATH];
	char szODBCINIFileName[MAX_PATH];
	int bReadOnly = FALSE;

    dwSize = sizeof(tchValue);
    if (MsiGetProperty( ihnd, "CustomActionData", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    strcpy(cur_id, tchValue);

    sprintf(KeyName,
	    "Software\\IngresCorporation\\Ingres\\%s_Installation",
	    tchValue);
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0,
			  KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "RemovedFeatures", NULL, &dwType,
			tchValue, &dwSize) != ERROR_SUCCESS)
    {
	/* Nothing is being removed. */
	RegCloseKey(hKey);
	return (ERROR_SUCCESS);
    }
    if (!strstr(tchValue, "ODBC"))
    {
	/* We're not being removed. */
	RegCloseKey(hKey);
	return (ERROR_SUCCESS);
    }

    RegCloseKey(hKey);
 

    /*
    ** Remove the registrations of the ODBC driver.
    */
    hLib = LoadLibrary("ODBCCP32");
    if (hLib)
    {
	PSQLREMOVEDRIVERPROC pSQLRemoveDriver = (PSQLREMOVEDRIVERPROC)GetProcAddress(hLib, "SQLRemoveDriver");
	if (pSQLRemoveDriver) 
	{
		int ret = FALSE;
		/* before removing the driver figure out if it was read-only driver 
		** just in case if we have to rollback.
		*/
		if (IsReadOnlyDriver(cur_id))
		{
			MsiSetProperty( ihnd, "INGRES_ODBC_READONLY", "1" );
			bReadOnly=TRUE;
		}
		if (bReadOnly)
		{
			sprintf(szDrvName, "Ingres %s Read Only", cur_id);
			sprintf(szDrvName2, "Ingres Read Only");
		}
		else
		{
			sprintf(szDrvName, "Ingres %s", cur_id);
			sprintf(szDrvName2, "Ingres");
		}
		ret = pSQLRemoveDriver(szDrvName2, TRUE, &dwSize);
		ret = pSQLRemoveDriver(szDrvName, TRUE, &dwSize);
	
	}
    }

	if (IsOtherODBCDriver(cur_id, tchValue, bReadOnly))
    {
		char *pIi_system;
		/* Restore ODBC driver. */
		strcpy(tchValue2, tchValue);
		pIi_system = strstr(tchValue2, "\\ingres\\bin\\");
		*pIi_system = '\0';
		pIi_system = tchValue2;
		SetEnvironmentVariable("II_SYSTEM", pIi_system);
		SetODBCEnvironment(TRUE, bReadOnly, 0);
    }

    return (ERROR_SUCCESS);
}

/*
**  Name:ingres_rollback_odbc
**
**  Description:
**	Reverse the changes to the system made by ingres_set_odbc_reg_entries and 
**	ingres_remove_odbc_files. Executed only during an installation rollback.
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
ingres_rollback_odbc(MSIHANDLE hInstall)
{
	char ii_code[3], buf[MAX_PATH+1];
	DWORD ret;
	char RegKey[256], data[256];
	DWORD type, size;
	HKEY hkRegKey;
	BOOL setup_IngresODBC, setup_IngresODBCReadOnly, bRemoved;
	char *p, *q, *tokens[3];
	int count;

	setup_IngresODBC=setup_IngresODBCReadOnly=bRemoved=0;

	/*
	**CustomActionData:[II_INSTALLATION];[INGRES_ODBC];[INGRES_ODBC_READONLY]
	*/
	ret=sizeof(buf);
	MsiGetProperty(hInstall, "CustomActionData", buf, &ret);
	p=buf;
	for (count=0; count<=2; count++)
	{
		if (p) q=strchr(p, ';');
		if (q) *q='\0';
		tokens[count]=p;
		if (q) p=q+1;
		else p=NULL;
	}
	strcpy(ii_code,tokens[0]);
	if (tokens[1] && !_stricmp(tokens[1], "1")) setup_IngresODBC=1;
	if (tokens[2] && !_stricmp(tokens[2], "1")) setup_IngresODBCReadOnly=1;
   
	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", ii_code);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey)!=ERROR_SUCCESS)
		return ERROR_SUCCESS;
	
	size=sizeof(data); *data=0;
	if(!RegQueryValueEx(hkRegKey, "InstalledFeatures", NULL, &type, (BYTE *)data, &size))
	{
		bRemoved=DelTok(data, "ODBC");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(!RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size))
	{
		bRemoved=DelTok(data, "ODBC");
		if(bRemoved)
		{
			size=sizeof(buf);
			RegQueryValueEx(hkRegKey, "II_SYSTEM", NULL, &type, (BYTE *)buf, &size);
			SetEnvironmentVariable("II_SYSTEM", buf);
			SetODBCEnvironment(setup_IngresODBC, setup_IngresODBCReadOnly, 1);			
		}
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres ODBC Driver", NULL, &type, (BYTE *)data, &size))
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}

	RegDeleteValue(hkRegKey, "Ingres ODBC Driver");
	RegDeleteValue(hkRegKey, "setup_ingresodbc");
	RegDeleteValue(hkRegKey, "setup_ingresodbcreadonly");
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
**  Name:ingres_unset_odbc_reg_entries
**
**  Description:
**	Reverse the changes to the system made by ingres_set_odbc_reg_entries 
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
ingres_unset_odbc_reg_entries(MSIHANDLE hInstall)
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
	if(!RegQueryValueEx(hkRegKey, "InstalledFeatures", NULL, &type, (BYTE *)data, &size))
	{
		bRemoved=DelTok(data, "ODBC");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(!RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size))
	{
		bRemoved=DelTok(data, "ODBC");
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres ODBC Driver", NULL, &type, (BYTE *)data, &size))
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}

	RegDeleteValue(hkRegKey, "Ingres ODBC Driver");
	RegDeleteValue(hkRegKey, "setup_ingresodbc");
	RegDeleteValue(hkRegKey, "setup_ingresodbcreadonly");
	RegDeleteValue(hkRegKey, "serviceauto");
	RegDeleteValue(hkRegKey, "serviceloginid");
	RegDeleteValue(hkRegKey, "servicepassword");
	RegDeleteValue(hkRegKey, "PostInstallationNeeded");
	RegDeleteValue(hkRegKey, "SilentInstall");
	RegDeleteValue(hkRegKey, "RebootNeeded");
	
	RegCloseKey(hkRegKey);
	return ERROR_SUCCESS;	
}

