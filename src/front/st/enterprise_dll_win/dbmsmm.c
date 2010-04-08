/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: dbmsmm.c
**
**  Description:
**	This file becomes the DLL for the Ingres DBMS Merge Module. It
**	contains custom actions that are required to be executed.
**
**	    ingres_set_dbms_reg_entries	- Set the appropriate registry
**					  entries for the post install.
**	    ingres_remove_dbms_files	- Removes directories created during
**					  the DBMS post install.
**
**  History:
**	22-apr-2001 (somsa01)
**	    Created.
**	07-may-2001 (somsa01)
**	    In ingres_remove_dbms_files(), do not remove the registry
**	    entries, as they are now removed by the post installation
**	    process. Also, in ingres_set_registry_entries(), if
**	    checkpointdatabases is needed, then do it here; there's no
**	    need to set it in the registry. Changed name of function to
**	    ingres_set_dbms_reg_entries().
**	23-may-2001 (somsa01)
**	    Removed the setting of "PostInstallationNeeded" from here.
**	18-jun-2001 (somsa01)
**	    Fixed up Modify operation.
**	16-Aug-2001 (penga03)
**	    Deleted ingres_cleanup_dbms_files, added 
**	    ingres_prepare_remove_dbms_files.
**	    Our new policy on the dbms directories and files that were  
**	    not installed as part of the initial installation is to set 
**	    up the RemoveFile table in DBMS Merge Module for them so that 
**	    they can be removed from the system by windows installer 
**	    during an uninstall.
**	22-Oct-2001 (penga03)
**	    Moved setting registry key EmbeddedRelease to dbmsnetmm.c, since we 
**	    set this key if DBMS or Net is seletcted.
**	12-dec-2001 (penga03)
**	    Changed INGRESFOLDER to
**	    INSTALLDIR.870341B5_2D6D_11D5_BDFC_00B0D0AD4485 since the dll
**	    functions defined here are specific to IngresIIDBMS Merge 
**	    Module.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	30-oct-2002 (penga03)
**	    If install Ingres on a double-byte Windows machine, 
**	    custom action ingres_prepare_remove_dbms_files fails to insert 
**	    those directories, that are created during the post installation 
**	    process configuring DBMS, into RemoveFile table if those
**	    directories has local characters. To fix the bug, in the function 
**	    ingres_prepare_remove_dbms_files(), replaced strlen, _stricmp, 
**	    strcat and sprintf with lstrlen, lstrcmpi, lstrcat and wsprintf, 
**	    that support the single-byte, double-byte, and Unicode character 
**	    sets.
**	18-jun-2003 (penga03)
**	    Do NOT set registry entries for DBMS if either of following two
**	    cases happens: 
**	    1) Feature IngresDBMS is not published
**	    2) IngresDBMS is not and will not be installed
**	26-jun-2003 (penga03)
**	    In ingres_rollback_dbms() and ingres_unset_dbms_reg_entries(),
**	    reset "InstalledFeatures" and "RemovedFeatures" to the registry
**	    when their values is being changed.
**	20-oct-2003 (penga03)
**	    Replaced StringReverse() with DelTok().
**	14-may-2004 (somsa01)
**	    In ingres_prepare_remove_dbms_files(), make sure we use the
**	    64-bit GUID, 8CCBF50C_6C17_4366_B8FE_FBB31A4092E0, when needed.
**	17-may-2004 (somsa01)
**	    Backed out prior change, as it does NOT properly fix the problem.
**	23-jun-2004 (penga03)
**	    Added two registry entries: installmdb, mdbname.
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	10-sep-2004 (penga03)
**	    Removed MDB.
**	03-nov-2004 (penga03)
**	    Added one registry entry mdbinstall.
**	15-nov-2004 (penga03)
**	    Modified ingres_prepare_remove_dbms_files() so that files under
**	    II_DATABASE, II_LOG, etc. will not be removed if they do not
**	    belong to the Ingres installation.
**	07-dec-2004 (penga03)
**	    Changed the formulation to generate GUIDs since the old one does
**	    not work for id from A1 to A9.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
**	11-jan-2005 (penga03)
**	    Set the registry entries if major upgrade.
**	13-jan-2005 (penga03)
**	    Added two regitry entries mdbsize and mdbdebug.
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	16-march-2005 (penga03)
**	    In ingres_prepare_remove_dbms_files() check the availability of the
**	    shared disk.
**	16-march-2005 (penga03)
**	    Corrected some error in ingres_prepare_remove_dbms_files().
**	17-march-2005 (penga03)
**	    Corrected some error in ingres_cluster_unregister_service().
**	17-march-2005 (penga03)
**	    Added a change so that Ingres cluster resource will not be removed if 
**	    it is still used by other nodes.
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
*/

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <MsiQuery.h>
#include <clusapi.h>
#include <resapi.h>
#include "commonmm.h"

#define DIRLIST_SIZE 1024
typedef struct tagDIRECTORY
{
	char DirName[MAX_PATH+1];
}DIRECTORY;

DIRECTORY DirList[DIRLIST_SIZE];
int Count=-1;

BOOL 
CreateDirList(char *StartDir)
{
	char FileName[MAX_PATH+1];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	BOOL bRet=TRUE;

	wsprintf(FileName, "%s\\*", StartDir);
	hFind=FindFirstFile(FileName, &FindFileData);
	if(hFind!=INVALID_HANDLE_VALUE)
	{
		strcpy(DirList[++Count].DirName, StartDir);
		do
		{
			if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(*(FindFileData.cFileName)!='.')
				{
					wsprintf(FileName, "%s\\%s", StartDir, FindFileData.cFileName);
					bRet=CreateDirList(FileName);
				}
			}
		}
		while (bRet && FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
		return bRet;
	}
	return bRet;
}


/*{
**  Name: ingres_set_dbms_reg_entries
**
**  Description:
**	Sets the appropriate registry entries for the DBMS Merge Module.
**	These entries will drive the post installation for the DBMS Server.
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
**	08-may-2001 (somsa01)
**	    If checkpointdatabases is needed, then do it here. There's no
**	    need to set it in the registry. Also, change name of function
**	    to ingres_set_dbms_reg_entries().
**	23-may-2001 (somsa01)
**	    Removed the setting of "PostInstallationNeeded" from here.
**	05-jun-2002 (penga03)
**	    ONLY if no other products referencing DBMS (by checking the 
**	    Bin component in DBMS merge module), then add it to 
**	    RemovedFeatures.
*/

UINT __stdcall
ingres_set_dbms_reg_entries (MSIHANDLE ihnd)
{
    HKEY		hKey;
    char		KeyName[256];
    int			status = 0;
    DWORD		dwSize, dwDisposition, dwType;
    TCHAR		tchValue[2048];
    INSTALLSTATE	iInstalled, iAction;
    char szID[3], szComponentGUID[39], szProductGUID[39];

    if (!MsiGetFeatureState(ihnd, "IngresDBMS", &iInstalled, &iAction))
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
	    strcpy(tchValue, "DBMS");
	}
	else if (!strstr(tchValue, "DBMS"))
	    strcat(tchValue, ",DBMS");

	status = RegSetValueEx( hKey, "InstalledFeatures", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize=sizeof(szID);
    MsiGetProperty(ihnd, "II_INSTALLATION", szID, &dwSize);
    if (!_stricmp(szID, "II"))
	strcpy(szComponentGUID, "{6C233EE5-2D74-11D5-BDFC-00B0D0AD4485}");
    else
    {
	int idx;

	idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - 'A';
	if (idx <= 0) 
	    idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - '0';
	sprintf(szComponentGUID, "{6C233EE5-2D74-11D5-%04X-%012X}", idx, idx*idx);
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
	    strcpy(tchValue, "DBMS");
	}
	else if (!strstr(tchValue, "DBMS"))
	    strcat(tchValue, ",DBMS");

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

    /*
    ** If this is an upgrade install, then checkpoint the databases
    ** if INGRES_CKPDBS is set.
    */

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_LOGFILESIZE", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "logfilesize", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_ENABLEDUALLOG", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "enableduallogging", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_ENABLESQL92", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "enablesql92", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_DATADIR", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';
	status = RegSetValueEx( hKey, "iidatabasedir", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_CKPDIR", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';
	status = RegSetValueEx( hKey, "iicheckpointdir", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_JNLDIR", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';
	status = RegSetValueEx( hKey, "iijournaldir", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_DMPDIR", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';
	status = RegSetValueEx( hKey, "iidumpdir", 0, REG_SZ, tchValue,						strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_WORKDIR", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';
	status = RegSetValueEx( hKey, "iiworkdir", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_PRIMLOGDIR", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';
	status = RegSetValueEx( hKey, "primarylogdir", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_DUALLOGDIR", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	if (tchValue[strlen(tchValue)-1] == '\\')
	    tchValue[strlen(tchValue)-1] = '\0';
	status = RegSetValueEx( hKey, "duallogdir", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_DESTROYTXLOGFILE", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "destroytxlogfile", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_UPGRADEUSERDBS", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "upgradeuserdatabases", 0, REG_SZ, 
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_CLUSTER_INSTALL", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, 
				"clusterinstall", 
				0, REG_SZ, (CONST BYTE *)&tchValue, 
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_CLUSTER_RESOURCE", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, 
				"clusterresource", 
				0, REG_SZ, (CONST BYTE *)&tchValue, 
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_MDB_INSTALL", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, 
				"mdbinstall", 
				0, REG_SZ, (CONST BYTE *)&tchValue, 
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_MDB_NAME", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, 
				"mdbname", 
				0, REG_SZ, (CONST BYTE *)&tchValue, 
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_MDB_SIZE", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, 
				"mdbsize", 
				0, REG_SZ, (CONST BYTE *)&tchValue, 
				strlen(tchValue) + 1 );

    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_MDB_DEBUG", tchValue,
			&dwSize ) != ERROR_SUCCESS || !tchValue[0])
    {
    }
    else
    {
	status = RegSetValueEx( hKey, 
				"mdbdebug", 
				0, REG_SZ, (CONST BYTE *)&tchValue, 
				strlen(tchValue) + 1 );

    }

    strcpy(tchValue, "TRUE");
    status = RegSetValueEx( hKey, "Ingres DBMS Server", 0, REG_SZ, 
			    (CONST BYTE *)&tchValue, strlen(tchValue) + 1 );

    return (status == ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
}

/*{
**  Name: ingres_remove_dbms_files
**
**  Description:
**	Removes any files/directories created by the DBMS Merge Module
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
**	23-July-2001 (penga03)
**	    Clean up the directories of database files and log files created 
**	    during post installation.
**	23-July-2001 (penga03)
**	    Replace "\\" with "\" in the file pathes read from config.dat.
**	03-Aug-2001 (penga03)
**	    Let the DBMD Merge Module delete the files/directories created  
**	    by the post installation process. For those files/directories 
**	    created by users, save their names in a file called 
**	    ingres_leftover.log under the system temp directory and 
**	    ingres_cleanup_dbms_files will remove them upon successful 
**	    completion of the installation script.
**	    For Example, as for log files, the DBMS Merge Module will delete 
**	    the primary log file that created by the post installation process, 
**	    ingres_cleanup_dbms_files will delete other log files that created 
**	    by users.
**	16-Aug-2001 (penga03)
**	    Upon our new policy on the dbms directoris and files that were not 
**	    installed as part of initial installation, we do not have to create 
**	    ingres_leftover.log to let ingres_cleanup_dbms_files clean them up.
*/

UINT __stdcall
ingres_remove_dbms_files (MSIHANDLE ihnd)
{
    char		KeyName[256];
    char		cmd[2048];
    char		Host[33];
    HKEY		hKey;
    DWORD		status = 0;
    DWORD		dwSize, dwType;
    TCHAR		tchValue[2048], tchII_SYSTEM[MAX_PATH+1];
    char		tchBuf[1024], tchBuf2[2048];

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
    if (!strstr(tchValue, "DBMS"))
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
    ** Set up our environment to execute commands.
    */
    GetSystemDirectory(tchBuf, sizeof(tchBuf));
    sprintf(tchBuf2, "%s\\ingres\\bin;%s\\ingres\\utility;%s", tchII_SYSTEM,
	    tchII_SYSTEM, tchBuf);
    SetEnvironmentVariable("PATH", tchBuf2);

    /*
    ** Change the startup count to zero.
    */
	Local_PMhost(Host);
    sprintf(cmd, "iisetres.exe ii.%s.ingstart.*.dbms 0", Host);
    Execute(cmd, TRUE);
    sprintf(cmd, "iisetres.exe ii.%s.ingstart.*.rmcmd 0", Host);
    Execute(cmd, TRUE);

    return (ERROR_SUCCESS);
}

/*
**  Name:ingres_rollback_dbms
**
**  Description:
**	Reverse the changes to the system made by ingres_set_dbms_reg_entries and 
**	ingres_remove_dbms_files. Executed only during an installation rollback.
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
**	16-Aug-2001 (penga03)
**	    Do not have to delete leftover.log since it belongs to old policy.
*/
UINT __stdcall
ingres_rollback_dbms(MSIHANDLE hInstall)
{
	char ii_code[3], ii_system[MAX_PATH+1], sdir[MAX_PATH+1], buf[3*(MAX_PATH+1)], Host[33];
	DWORD ret;
	char RegKey[256], data[256];
	DWORD type, size;
	HKEY hkRegKey;
	BOOL bRemoved=0;

	ret=sizeof(ii_code);
	if(MsiGetProperty(hInstall, "CustomActionData", ii_code, &ret)!=ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;
	
	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", ii_code);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
		return ERROR_SUCCESS;

	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "InstalledFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "DBMS");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "DBMS");
		if(bRemoved)
		{
			size=sizeof(ii_system);
			RegQueryValueEx(hkRegKey, "II_SYSTEM", NULL, &type, (BYTE *)ii_system, &size);
			SetEnvironmentVariable("II_SYSTEM", ii_system);
			GetSystemDirectory(sdir, sizeof(sdir));
			sprintf(buf, "%s\\ingres\\bin;%s\\ingres\\utility;%s", ii_system, ii_system, sdir);
			SetEnvironmentVariable("PATH", buf);
			
			Local_PMhost(Host);
			sprintf(buf, "iisetres.exe ii.%s.ingstart.*.dbms 1", Host);
			Execute(buf, TRUE);
			sprintf(buf, "iisetres.exe ii.%s.ingstart.*.rmcmd 1", Host);
			Execute(buf, TRUE);
		}
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}

	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres DBMS Server", NULL, &type, (BYTE *)data, &size)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}
	
	RegDeleteValue(hkRegKey, "Ingres DBMS Server");
	RegDeleteValue(hkRegKey, "logfilesize");
	RegDeleteValue(hkRegKey, "enableduallogging");
	RegDeleteValue(hkRegKey, "enablesql92");
	RegDeleteValue(hkRegKey, "iidatabasedir");
	RegDeleteValue(hkRegKey, "iicheckpointdir");
	RegDeleteValue(hkRegKey, "iijournaldir");
	RegDeleteValue(hkRegKey, "iidumpdir");
	RegDeleteValue(hkRegKey, "iiworkdir");
	RegDeleteValue(hkRegKey, "primarylogdir");
	RegDeleteValue(hkRegKey, "duallogdir");
	RegDeleteValue(hkRegKey, "destroytxlogfile");
	RegDeleteValue(hkRegKey, "upgradeuserdatabases");
	RegDeleteValue(hkRegKey, "clusterinstall");
	RegDeleteValue(hkRegKey, "clusterresource");
	RegDeleteValue(hkRegKey, "mdbinstall");
	RegDeleteValue(hkRegKey, "mdbname");
	RegDeleteValue(hkRegKey, "mdbsize");
	RegDeleteValue(hkRegKey, "mdbdebug");

	RegCloseKey(hkRegKey);
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_prepare_remove_dbms_files
**
**  Description:
**	Set up the RemoveFile table in DBMS Merge Module for the 
**	directories and files that were not installed as part of 
**	the initial installation so that they can be removed from  
**	the system during an uninstall.
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
**	16-Aug-2001 (penga03)
**	    Created.
**	17-Aug-2001 (penga03)
**	    Do not put the INSTALLDIR in the RemoveFile table.
**	14-may-2004 (somsa01)
**	    Make sure we use the 64-bit GUID,
**	    8CCBF50C_6C17_4366_B8FE_FBB31A4092E0, when needed.
**	17-may-2004 (somsa01)
**	    Backed out prior change, as it does NOT properly fix the problem.
*/
UINT __stdcall
ingres_prepare_remove_dbms_files(MSIHANDLE hInstall)
{
	TCHAR tchValue[MAX_PATH+1], ii_system[MAX_PATH+1], szBuf[MAX_PATH+1];
	DWORD dwSize;
	MSIHANDLE hDatabase, hView, hRecord;
	TCHAR ConfigKey[256], Host[64];
	INSTALLSTATE iInstalled, iAction;
	TCHAR inst_id[64] = "INSTALLDIR.870341B5_2D6D_11D5_BDFC_00B0D0AD4485";
	TCHAR bin_id[64] = "Bin.870341B5_2D6D_11D5_BDFC_00B0D0AD4485";
	char sharedDisk[3];

	MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction);
	if(iAction!=INSTALLSTATE_ABSENT)
		return ERROR_SUCCESS;

	dwSize=sizeof(ii_system);
	if(MsiGetTargetPath(hInstall, inst_id, ii_system, &dwSize)!=ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;
	if(ii_system[lstrlen(ii_system)-1] == '\\')
		ii_system[lstrlen(ii_system)-1] = '\0';
	SetEnvironmentVariable("II_SYSTEM", ii_system);
	
	MsiSetProperty(hInstall, "INGRES_CLUSTER_RESOURCE_INUSE", "0");
	if(Local_NMgtIngAt("II_DATABASE", tchValue)==ERROR_SUCCESS)
	{
		strncpy(sharedDisk, tchValue, 2);
		sharedDisk[2]='\0';
		if (GetFileAttributes(sharedDisk) == -1)
		{
			MsiSetProperty(hInstall, "INGRES_CLUSTER_RESOURCE_INUSE", "1");
			return ERROR_SUCCESS;
		}
	}

	hDatabase=MsiGetActiveDatabase(hInstall);
	if(!hDatabase)
		return ERROR_INSTALL_FAILURE;
	if(MsiDatabaseOpenView(hDatabase, "INSERT INTO `RemoveFile` (`FileKey`, `Component_`, `FileName`, `DirProperty`, `InstallMode`) VALUES (?, ?, ?, ?, ?) TEMPORARY", &hView)!=ERROR_SUCCESS)
		return ERROR_INSTALL_FAILURE;
	hRecord=MsiCreateRecord(5);
	if(!hRecord)
		return ERROR_INSTALL_FAILURE;

	if(Local_NMgtIngAt("II_DATABASE", tchValue)==ERROR_SUCCESS)
	{
		int KeyCount;
		char szKey[MAX_PATH+1], szKey2[MAX_PATH+1];

		wsprintf(szBuf, "%s\\ingres\\data", tchValue);
		Count=-1;
		CreateDirList(szBuf);
		for(KeyCount=0; KeyCount<=Count; KeyCount++)
		{
			wsprintf(szKey, "RemoveDatabaseDir_%d", KeyCount);
			wsprintf(szKey2, "RemoveDatabaseDir_%d_Dir", KeyCount);

			MsiSetProperty(hInstall, szKey, DirList[KeyCount].DirName);
			
			MsiRecordSetString(hRecord, 1, szKey);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "*.*");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;

			MsiRecordSetString(hRecord, 1, szKey2);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;
		}

		Count++;
		wsprintf(szKey, "RemoveDatabaseDir_%d", Count);
		wsprintf(szKey2, "RemoveDatabaseDir_%d_Dir", Count);

		wsprintf(szBuf, "%s\\ingres", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;

		Count++;
		wsprintf(szKey, "RemoveDatabaseDir_%d", Count);
		wsprintf(szKey2, "RemoveDatabaseDir_%d_Dir", Count);
		
		wsprintf(szBuf, "%s", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;
	}

	if(Local_NMgtIngAt("II_CHECKPOINT", tchValue)==ERROR_SUCCESS)
	{
		int KeyCount;
		char szKey[MAX_PATH+1], szKey2[MAX_PATH+1];

		wsprintf(szBuf, "%s\\ingres\\ckp", tchValue);
		Count=-1;
		CreateDirList(szBuf);
		for(KeyCount=0; KeyCount<=Count; KeyCount++)
		{
			wsprintf(szKey, "RemovecCheckpointDir_%d", KeyCount);
			wsprintf(szKey2, "RemoveCheckpointDir_%d_Dir", KeyCount);

			MsiSetProperty(hInstall, szKey, DirList[KeyCount].DirName);
			
			MsiRecordSetString(hRecord, 1, szKey);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "*.*");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;

			MsiRecordSetString(hRecord, 1, szKey2);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;
		}

		Count++;
		wsprintf(szKey, "RemovecCheckpointDir_%d", Count);
		wsprintf(szKey2, "RemovecCheckpointDir_%d_Dir", Count);

		wsprintf(szBuf, "%s\\ingres", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;

		Count++;
		wsprintf(szKey, "RemovecCheckpointDir_%d", Count);
		wsprintf(szKey2, "RemovecCheckpointDir_%d_Dir", Count);
		
		wsprintf(szBuf, "%s", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;
	}

	if(Local_NMgtIngAt("II_JOURNAL", tchValue)==ERROR_SUCCESS)
	{
		int KeyCount;
		char szKey[MAX_PATH+1], szKey2[MAX_PATH+1];

		wsprintf(szBuf, "%s\\ingres\\jnl", tchValue);
		Count=-1;
		CreateDirList(szBuf);
		for(KeyCount=0; KeyCount<=Count; KeyCount++)
		{
			wsprintf(szKey, "RemoveJournalDir_%d", KeyCount);
			wsprintf(szKey2, "RemoveJournalDir_%d_Dir", KeyCount);

			MsiSetProperty(hInstall, szKey, DirList[KeyCount].DirName);
			
			MsiRecordSetString(hRecord, 1, szKey);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "*.*");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;

			MsiRecordSetString(hRecord, 1, szKey2);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;
		}

		Count++;
		wsprintf(szKey, "RemoveJournalDir_%d", Count);
		wsprintf(szKey2, "RemoveJournalDir_%d_Dir", Count);

		wsprintf(szBuf, "%s\\ingres", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;

		Count++;
		wsprintf(szKey, "RemoveJournalDir_%d", Count);
		wsprintf(szKey2, "RemoveJournalDir_%d_Dir", Count);
		
		wsprintf(szBuf, "%s", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;
	}

	if(Local_NMgtIngAt("II_DUMP", tchValue)==ERROR_SUCCESS)
	{
		int KeyCount;
		char szKey[MAX_PATH+1], szKey2[MAX_PATH+1];

		wsprintf(szBuf, "%s\\ingres\\dmp", tchValue);
		Count=-1;
		CreateDirList(szBuf);
		for(KeyCount=0; KeyCount<=Count; KeyCount++)
		{
			wsprintf(szKey, "RemoveDumpDir_%d", KeyCount);
			wsprintf(szKey2, "RemoveDumpDir_%d_Dir", KeyCount);

			MsiSetProperty(hInstall, szKey, DirList[KeyCount].DirName);
			
			MsiRecordSetString(hRecord, 1, szKey);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "*.*");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;

			MsiRecordSetString(hRecord, 1, szKey2);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;
		}

		Count++;
		wsprintf(szKey, "RemoveDumpDir_%d", Count);
		wsprintf(szKey2, "RemoveDumpDir_%d_Dir", Count);

		wsprintf(szBuf, "%s\\ingres", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;

		Count++;
		wsprintf(szKey, "RemoveDumpDir_%d", Count);
		wsprintf(szKey2, "RemoveDumpDir_%d_Dir", Count);
		
		wsprintf(szBuf, "%s", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;
	}

	if(Local_NMgtIngAt("II_WORK", tchValue)==ERROR_SUCCESS)
	{
		int KeyCount;
		char szKey[MAX_PATH+1], szKey2[MAX_PATH+1];

		wsprintf(szBuf, "%s\\ingres\\work", tchValue);
		Count=-1;
		CreateDirList(szBuf);
		for(KeyCount=0; KeyCount<=Count; KeyCount++)
		{
			wsprintf(szKey, "RemoveWorkDir_%d", KeyCount);
			wsprintf(szKey2, "RemoveWorkDir_%d_Dir", KeyCount);

			MsiSetProperty(hInstall, szKey, DirList[KeyCount].DirName);
			
			MsiRecordSetString(hRecord, 1, szKey);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "*.*");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;

			MsiRecordSetString(hRecord, 1, szKey2);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;
		}

		Count++;
		wsprintf(szKey, "RemoveWorkDir_%d", Count);
		wsprintf(szKey2, "RemoveWorkDir_%d_Dir", Count);

		wsprintf(szBuf, "%s\\ingres", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;

		Count++;
		wsprintf(szKey, "RemoveWorkDir_%d", Count);
		wsprintf(szKey2, "RemoveWorkDir_%d_Dir", Count);
		
		wsprintf(szBuf, "%s", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
			
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;
	}

	Local_PMhost(Host);
	wsprintf(ConfigKey, "ii.%s.rcp.log.log_file_parts", Host);
	if(Local_PMget(ConfigKey, tchValue) == ERROR_SUCCESS)
	{
		int num_logs, i;
		
		num_logs=atoi(tchValue);
		for(i=1; i<=num_logs; i++)
		{
			wsprintf(ConfigKey, "ii.%s.rcp.log.log_file_%d", Host, i);
			if(Local_PMget(ConfigKey, tchValue) == ERROR_SUCCESS)
			{
				int KeyCount;
				char szKey[MAX_PATH+1], szKey2[MAX_PATH+1];
				
				StringReplace(tchValue);
				wsprintf(szBuf, "%s\\ingres\\log", tchValue);
				Count=-1;
				CreateDirList(szBuf);
				for(KeyCount=0; KeyCount<=Count; KeyCount++)
				{
					wsprintf(szKey, "RemoveLogDir_%d", KeyCount);
					wsprintf(szKey2, "RemoveLogDir_%d_Dir", KeyCount);
					
					MsiSetProperty(hInstall, szKey, DirList[KeyCount].DirName);
					
					MsiRecordSetString(hRecord, 1, szKey);
					MsiRecordSetString(hRecord, 2, bin_id);
					MsiRecordSetString(hRecord, 3, "*.*");
					MsiRecordSetString(hRecord, 4, szKey);
					MsiRecordSetInteger(hRecord, 5, 2);
					if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
						return ERROR_INSTALL_FAILURE;
					
					MsiRecordSetString(hRecord, 1, szKey2);
					MsiRecordSetString(hRecord, 2, bin_id);
					MsiRecordSetString(hRecord, 3, "");
					MsiRecordSetString(hRecord, 4, szKey);
					MsiRecordSetInteger(hRecord, 5, 2);
					if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
						return ERROR_INSTALL_FAILURE;
				}
				
				Count++;
				wsprintf(szKey, "RemoveLogDir_%d", Count);
				wsprintf(szKey2, "RemoveLogDir_%d_Dir", Count);
				
				wsprintf(szBuf, "%s\\ingres", tchValue);
				MsiSetProperty(hInstall, szKey, szBuf);
				
				MsiRecordSetString(hRecord, 1, szKey2);
				MsiRecordSetString(hRecord, 2, bin_id);
				MsiRecordSetString(hRecord, 3, "");
				MsiRecordSetString(hRecord, 4, szKey);
				MsiRecordSetInteger(hRecord, 5, 2);
				if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
					return ERROR_INSTALL_FAILURE;
				
				Count++;
				wsprintf(szKey, "RemoveLogDir_%d", Count);
				wsprintf(szKey2, "RemoveLogDir_%d_Dir", Count);
				
				wsprintf(szBuf, "%s", tchValue);
				MsiSetProperty(hInstall, szKey, szBuf);
				
				MsiRecordSetString(hRecord, 1, szKey2);
				MsiRecordSetString(hRecord, 2, bin_id);
				MsiRecordSetString(hRecord, 3, "");
				MsiRecordSetString(hRecord, 4, szKey);
				MsiRecordSetInteger(hRecord, 5, 2);
				if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
					return ERROR_INSTALL_FAILURE;
			}
		}
	}
	
	wsprintf(ConfigKey, "ii.%s.rcp.log.dual_log_1", Host);
	if(Local_PMget(ConfigKey, tchValue) == ERROR_SUCCESS)
	{
		int KeyCount;
		char szKey[MAX_PATH+1], szKey2[MAX_PATH+1];
		
		StringReplace(tchValue);
		wsprintf(szBuf, "%s\\ingres\\log", tchValue);
		Count=-1;
		CreateDirList(szBuf);
		for(KeyCount=0; KeyCount<=Count; KeyCount++)
		{
			wsprintf(szKey, "RemoveDuallogDir_%d", KeyCount);
			wsprintf(szKey2, "RemoveDuallogDir_%d_Dir", KeyCount);

			MsiSetProperty(hInstall, szKey, DirList[KeyCount].DirName);
			
			MsiRecordSetString(hRecord, 1, szKey);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "*.*");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;

			MsiRecordSetString(hRecord, 1, szKey2);
			MsiRecordSetString(hRecord, 2, bin_id);
			MsiRecordSetString(hRecord, 3, "");
			MsiRecordSetString(hRecord, 4, szKey);
			MsiRecordSetInteger(hRecord, 5, 2);
			if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
				return ERROR_INSTALL_FAILURE;
		}

		Count++;
		wsprintf(szKey, "RemoveDuallogDir_%d", Count);
		wsprintf(szKey2, "RemoveDuallogDir_%d_Dir", Count);
				
		wsprintf(szBuf, "%s\\ingres", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
				
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;
		
		Count++;
		wsprintf(szKey, "RemoveDuallogDir_%d", Count);
		wsprintf(szKey2, "RemoveDuallogDir_%d_Dir", Count);
		
		wsprintf(szBuf, "%s", tchValue);
		MsiSetProperty(hInstall, szKey, szBuf);
		
		MsiRecordSetString(hRecord, 1, szKey2);
		MsiRecordSetString(hRecord, 2, bin_id);
		MsiRecordSetString(hRecord, 3, "");
		MsiRecordSetString(hRecord, 4, szKey);
		MsiRecordSetInteger(hRecord, 5, 2);
		if(MsiViewExecute(hView, hRecord)!=ERROR_SUCCESS)
			return ERROR_INSTALL_FAILURE;
	}

	MsiCloseHandle(hRecord);
	MsiCloseHandle(hView);
	MsiCloseHandle(hDatabase);

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_unset_dbms_reg_entries
**
**  Description:
**	Reverse the changes to the system made by ingres_set_dbms_reg_entries 
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
ingres_unset_dbms_reg_entries(MSIHANDLE hInstall)
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
		bRemoved=DelTok(data, "DBMS");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "DBMS");
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}

	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres DBMS Server", NULL, &type, (BYTE *)data, &size)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}
	
	RegDeleteValue(hkRegKey, "Ingres DBMS Server");
	RegDeleteValue(hkRegKey, "logfilesize");
	RegDeleteValue(hkRegKey, "enableduallogging");
	RegDeleteValue(hkRegKey, "enablesql92");
	RegDeleteValue(hkRegKey, "iidatabasedir");
	RegDeleteValue(hkRegKey, "iicheckpointdir");
	RegDeleteValue(hkRegKey, "iijournaldir");
	RegDeleteValue(hkRegKey, "iidumpdir");
	RegDeleteValue(hkRegKey, "iiworkdir");
	RegDeleteValue(hkRegKey, "primarylogdir");
	RegDeleteValue(hkRegKey, "duallogdir");
	RegDeleteValue(hkRegKey, "destroytxlogfile");
	RegDeleteValue(hkRegKey, "upgradeuserdatabases");
	RegDeleteValue(hkRegKey, "clusterinstall");
	RegDeleteValue(hkRegKey, "clusterresource");
	RegDeleteValue(hkRegKey, "mdbinstall");
	RegDeleteValue(hkRegKey, "mdbname");
	RegDeleteValue(hkRegKey, "mdbsize");
	RegDeleteValue(hkRegKey, "mdbdebug");
	
	RegCloseKey(hkRegKey);
	return ERROR_SUCCESS;
}

/*
**  Name: ingres_cluster_isinstalled
**
**  Description:
**	This is a type 1 custom action.
**
**  History:
**	28-feb-2004 (penga03)
**	    Created.
*/

UINT __stdcall 
ingres_cluster_isinstalled(MSIHANDLE hInstall) 
{
	MsiSetProperty(hInstall, "INGRES_CLUSTER_ISINSTALLED", "0");
	if(CheckServiceExists("ClusSvc"))
		MsiSetProperty(hInstall, "INGRES_CLUSTER_ISINSTALLED", "1");
	
	return (ERROR_SUCCESS);
}

/*
**  Name: ingres_cluster_unregister_service
**
**  Description:
**	This is a type 1 custom action.
**
**  History:
**	28-feb-2004 (penga03)
**	    Created.
*/
BOOL SetupRegistryKeys(HRESOURCE hResource, LPSTR lpszInstallCode, BOOL bInstall)
{
	DWORD dRet;
	char SubKey[128];
	WCHAR szRegistryKey[128];
	LPVOID lpInBuffer;
	DWORD cbInBufferSize;
	int i;

	struct {
		char *szRegistryKey;
	} RegistryKeys[] = {
		"SOFTWARE\\Classes\\Ingres_Database_%s",
		"SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", 0
	};

	for(i=0; i<sizeof(RegistryKeys)/sizeof(RegistryKeys[0]); i++)
	{
		sprintf(SubKey, RegistryKeys[i].szRegistryKey, lpszInstallCode);
		mbstowcs(szRegistryKey, SubKey, sizeof(SubKey));
		lpInBuffer = (LPVOID)szRegistryKey;
		cbInBufferSize = (wcslen(szRegistryKey) + 1) * sizeof(WCHAR);

		if(bInstall)
		{
			dRet = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT, lpInBuffer, cbInBufferSize, NULL, 0, NULL);
			if (!(dRet == ERROR_SUCCESS || dRet == ERROR_ALREADY_EXISTS))
				return FALSE;
		}
		else
		{
			dRet = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT, lpInBuffer, cbInBufferSize, NULL, 0, NULL);
			if (!(dRet == ERROR_SUCCESS || dRet == ERROR_FILE_NOT_FOUND))
				return FALSE;
		}
	}

	return TRUE;
}
UINT __stdcall 
ingres_cluster_unregister_service(MSIHANDLE hInstall) 
{
	HCLUSTER hCluster;
	HRESOURCE hResource;
	WCHAR wszResourceName[256];
	char szResourceName[256];
	char szInstallCode[3];
	DWORD dwSize;
	INSTALLSTATE iInstalled, iAction;
	char szBuf[256];

	MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction);
	if(iAction != INSTALLSTATE_ABSENT)
		return ERROR_SUCCESS;

	dwSize=sizeof(szBuf); *szBuf=0;
	if (!MsiGetProperty(hInstall, "INGRES_CLUSTER_RESOURCE_INUSE", szBuf, &dwSize) 
		&& !strcmp(szBuf, "1"))
		return ERROR_SUCCESS;

	dwSize=sizeof(szResourceName); *szResourceName=0;
	MsiGetProperty(hInstall, "INGRES_CLUSTER_RESOURCE", szResourceName, &dwSize);
	mbstowcs(wszResourceName, szResourceName, sizeof(szResourceName));
	dwSize=sizeof(szInstallCode); *szInstallCode=0;
	MsiGetProperty(hInstall, "II_INSTALLATION", szInstallCode, &dwSize);

	hCluster = OpenCluster(NULL);
	if(hCluster)
	{
		hResource = OpenClusterResource(hCluster, wszResourceName);
		if(hResource)
		{
			if (!DeleteClusterResource(hResource))
				SetupRegistryKeys(hResource, szInstallCode, FALSE);
			CloseClusterResource(hResource);
		}
		CloseCluster(hCluster);
	}

	return ERROR_SUCCESS;
}
