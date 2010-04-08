/*
**  Copyright (c) 2001-2009 Ingres Corporation.
*/

/*
**  Name: netmm.c
**
**  Description:
**	This file becomes the DLL for the Ingres NET Merge Module. It
**	contains custom actions that are required to be executed.
**
**	    ingres_set_net_reg_entries	- Set the appropriate registry
**					  entries for the post install.
**	    ingres_remove_net_files	- Removes directories created during
**					  the DBMS post install.
**
**  History:
**	09-may-2001 (somsa01)
**	    Created.
**	23-may-2001 (somsa01)
**	    Removed the setting of "PostInstallationNeeded" from here.
**	18-jun-2001 (somsa01)
**	    Fixed up Modify operation.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	18-jun-2003 (penga03)
**	    Do NOT set registry entries for NET if either of following two cases 
**	    happens: 
**	    1) Feature IngresNet is not published
**	    2) IngresNet is not and will not be installed
**	26-jun-2003 (penga03)
**	    In ingres_rollback_net() and ingres_unset_net_reg_entries(), reset 
**	    "InstalledFeatures" and "RemovedFeatures" to the registry when their 
**	    values is being changed.
**	20-oct-2003 (penga03)
**	    Replaced StringReverse() with DelTok().
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	11-jan-2005 (penga03)
**	    Set the registry entries if major upgrade.
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	16-Jan-2009 (whiro01) SD Issue 132899
**	    In conjunction with the fix for this issue, fixed a few compiler warnings.
*/

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <MsiQuery.h>
#include "commonmm.h"


/*{
**  Name: ingres_set_net_reg_entries
**
**  Description:
**	Sets the appropriate registry entries for the NET Merge Module.
**	These entries will drive the post installation for the NET Server.
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
**	    need to set it in the registry.
**	23-may-2001 (somsa01)
**	    Removed the setting of "PostInstallationNeeded" from here.
**	09-Aug-2001 (penga03)
**	    Reinstall Ingres/Net if make some change on protocols.
**	05-jun-2002 (penga03)
**	    ONLY if no other products referencing NET (by checking the 
**	    Bin component in NET merge module), then add it to 
**	    RemovedFeatures.
*/

UINT __stdcall
ingres_set_net_reg_entries (MSIHANDLE ihnd)
{
    HKEY		hKey;
    char		KeyName[256];
    int			status = 0;
    DWORD		dwSize, dwDisposition, dwType;
    TCHAR		tchValue[2048];
    INSTALLSTATE	iInstalled, iAction;
	INSTALLSTATE iInstalledTCPIP, iActionTCPIP, iInstalledBIOS, iActionBIOS;
	INSTALLSTATE iInstalledSPX, iActionSPX, iInstalledDECNET, iActionDECNET;
    char szID[3], szComponentGUID[39], szProductGUID[39];
	BOOL protocolsChanged=FALSE;

    if (!MsiGetFeatureState(ihnd, "IngresNet", &iInstalled, &iAction))
    {
	if (iInstalled==INSTALLSTATE_ABSENT && iAction==INSTALLSTATE_UNKNOWN) 
	    return ERROR_SUCCESS;
    }
    else 
	return ERROR_SUCCESS;

	MsiGetFeatureState(ihnd, "IngresTCPIP", &iInstalledTCPIP, &iActionTCPIP);
	MsiGetFeatureState(ihnd, "IngresNetBIOS", &iInstalledBIOS, &iActionBIOS);
	MsiGetFeatureState(ihnd, "IngresNovellSPXIPX", &iInstalledSPX, &iActionSPX);
	MsiGetFeatureState(ihnd, "IngresDECNet", &iInstalledDECNET, &iActionDECNET);
	if ((iInstalledTCPIP != INSTALLSTATE_LOCAL && iActionTCPIP == INSTALLSTATE_LOCAL) || 
		(iInstalledTCPIP == INSTALLSTATE_LOCAL && iActionTCPIP == INSTALLSTATE_ABSENT) ||
		(iInstalledBIOS != INSTALLSTATE_LOCAL && iActionBIOS == INSTALLSTATE_LOCAL) ||
		(iInstalledBIOS == INSTALLSTATE_LOCAL && iActionBIOS == INSTALLSTATE_ABSENT) || 
		(iInstalledSPX != INSTALLSTATE_LOCAL && iActionSPX == INSTALLSTATE_LOCAL) || 
		(iInstalledSPX == INSTALLSTATE_LOCAL && iActionSPX == INSTALLSTATE_ABSENT) || 
		(iInstalledDECNET != INSTALLSTATE_LOCAL && iActionDECNET == INSTALLSTATE_LOCAL) || 
		(iInstalledDECNET == INSTALLSTATE_LOCAL && iActionDECNET == INSTALLSTATE_ABSENT))
	{
		protocolsChanged=TRUE;
	}

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
	    strcpy(tchValue, "NET");
	}
	else if (!strstr(tchValue, "NET"))
	    strcat(tchValue, ",NET");
	
	status = RegSetValueEx( hKey, "InstalledFeatures", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
    }

    dwSize=sizeof(szID);
    MsiGetProperty(ihnd, "II_INSTALLATION", szID, &dwSize);
    if (!_stricmp(szID, "II"))
	strcpy(szComponentGUID, "{89BE49FC-2EC5-11D5-BDFC-00B0D0AD4485}");
    else
    {
	int idx;

	idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - 'A';
	if (idx <= 0) 
	    idx = (toupper(szID[0]) - 'A') * 26 + toupper(szID[1]) - '0';
	sprintf(szComponentGUID, "{89BE49FC-2EC5-11D5-%04X-%012X}", idx, idx*idx);
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
	    strcpy(tchValue, "NET");
	}
	else if (!strstr(tchValue, "NET"))
	    strcat(tchValue, ",NET");
	
	status = RegSetValueEx( hKey, "RemovedFeatures", 0, REG_SZ,
				tchValue, strlen(tchValue) + 1 );
	RegCloseKey(hKey);
	return (status==ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
    }
    else if ((iInstalled == INSTALLSTATE_LOCAL && !protocolsChanged) || 
		(iInstalled != INSTALLSTATE_LOCAL && iAction != INSTALLSTATE_LOCAL))
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
	MsiGetProperty( ihnd, "INGRES_TCPIP", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	RegCloseKey(hKey);
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "enableTCPIP", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_NETBIOS", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	RegCloseKey(hKey);
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "enableNETBIOS", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_SPXIPX", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	RegCloseKey(hKey);
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "enableSPX", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    dwSize = sizeof(tchValue);
    if (status ||
	MsiGetProperty( ihnd, "INGRES_DECNET", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	RegCloseKey(hKey);
	return (ERROR_INSTALL_FAILURE);
    }
    else
    {
	status = RegSetValueEx( hKey, "enableDECNET", 0, REG_SZ, tchValue,
				strlen(tchValue) + 1 );
    }

    strcpy(tchValue, "TRUE");
    status = RegSetValueEx( hKey, "Ingres/Net", 0, REG_SZ, 
			    (CONST BYTE *)&tchValue, strlen(tchValue) + 1 );

    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
}

/*{
**  Name: ingres_remove_net_files
**
**  Description:
**	Removes any files/directories created by the NET Merge Module
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
**	09-may-2001 (somsa01)
**	    Created.
*/

UINT __stdcall
ingres_remove_net_files (MSIHANDLE ihnd)
{
    char		cmd[2048];
    char		Host[33];
    char		KeyName[256];
    HKEY		hKey;
    DWORD		status = 0;
    DWORD		dwSize, dwType;
    TCHAR		tchValue[2048];
    char		tchBuf[1024], tchBuf2[2048];
    char		tmpchr = '\0';

    dwSize = sizeof(tchValue);
    if (MsiGetProperty( ihnd, "CustomActionData", tchValue,
			&dwSize ) != ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }

    sprintf(KeyName,
	    "Software\\IngresCorporation\\Ingres\\%s_Installation",
	    tchValue);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS,
			  &hKey)) return ERROR_SUCCESS;

    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "RemovedFeatures", NULL, &dwType,
			tchValue, &dwSize) != ERROR_SUCCESS)
    {
	/* Nothing is being removed. */
	RegCloseKey(hKey);
	return (ERROR_SUCCESS);
    }
    if (!strstr(tchValue, "NET"))
    {
	/* We're not being removed. */
	RegCloseKey(hKey);
	return (ERROR_SUCCESS);
    }

    /*
    ** Set up our environment to execute commands.
    */
    dwType = REG_SZ;
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, tchValue, &dwSize) !=
	ERROR_SUCCESS)
    {
	return (ERROR_INSTALL_FAILURE);
    }
    SetEnvironmentVariable("II_SYSTEM", tchValue);
    RegCloseKey(hKey);

    GetSystemDirectory(tchBuf, sizeof(tchBuf));
    sprintf(tchBuf2, "%s\\ingres\\bin;%s\\ingres\\utility;%s", tchValue,
	    tchValue, tchBuf);
    SetEnvironmentVariable("PATH", tchBuf2);

    /*
    ** Change the startup count to zero.
    */
    Local_PMhost(Host);
    sprintf(cmd, "iisetres.exe ii.%s.ingstart.*.gcc 0", Host);
    Execute(cmd, TRUE);
    return (ERROR_SUCCESS);
}

/*
**  Name:ingres_rollback_net
**
**  Description:
**	Reverse the changes to the system made by ingres_set_net_reg_entries and 
**	ingres_remove_net_files. Executed only during an installation rollback.
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
ingres_rollback_net(MSIHANDLE hInstall)
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
		bRemoved=DelTok(data, "NET");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "NET");
		if(bRemoved)
		{
			size=sizeof(ii_system);
			RegQueryValueEx(hkRegKey, "II_SYSTEM", NULL, &type, (BYTE *)ii_system, &size);
			SetEnvironmentVariable("II_SYSTEM", ii_system);
			GetSystemDirectory(sdir, sizeof(sdir));
			sprintf(buf, "%s\\ingres\\bin;%s\\ingres\\utility;%s", ii_system, ii_system, sdir);
			SetEnvironmentVariable("PATH", buf);
			
			Local_PMhost(Host);
			sprintf(buf, "iisetres.exe ii.%s.ingstart.*.gcc 1", Host);
			Execute(buf, TRUE);
		}
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}

	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres/Net", NULL, &type, (BYTE *)data, &size)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}
	
	RegDeleteValue(hkRegKey, "Ingres/Net");
	RegDeleteValue(hkRegKey, "enableTCPIP");
	RegDeleteValue(hkRegKey, "enableNETBIOS");
	RegDeleteValue(hkRegKey, "enableSPX");
	RegDeleteValue(hkRegKey, "enableDECNET");

	RegCloseKey(hkRegKey);
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_unset_net_reg_entries
**
**  Description:
**	Reverse the changes to the system made by ingres_set_net_reg_entries 
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
ingres_unset_net_reg_entries(MSIHANDLE hInstall)
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
		bRemoved=DelTok(data, "NET");
		if(*data==0)
			RegDeleteValue(hkRegKey, "InstalledFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "InstalledFeatures", 0, REG_SZ, data, strlen(data)+1);
	}
	
	size=sizeof(data); *data=0;
	if(RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
	{
		bRemoved=DelTok(data, "NET");
		if(*data==0)
			RegDeleteValue(hkRegKey, "RemovedFeatures");
		else if (bRemoved)
			RegSetValueEx(hkRegKey, "RemovedFeatures", 0, REG_SZ, data, strlen(data)+1);
	}

	size=sizeof(data);
	if(RegQueryValueEx(hkRegKey, "Ingres/Net", NULL, &type, (BYTE *)data, &size)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkRegKey);
		return ERROR_SUCCESS;
	}
	
	RegDeleteValue(hkRegKey, "Ingres/Net");
	RegDeleteValue(hkRegKey, "enableTCPIP");
	RegDeleteValue(hkRegKey, "enableNETBIOS");
	RegDeleteValue(hkRegKey, "enableSPX");
	RegDeleteValue(hkRegKey, "enableDECNET");

	RegCloseKey(hkRegKey);
	return ERROR_SUCCESS;
}


/*
**  Name:ingres_store_vnode
**
**  Description:
**	Stores virtual node information specified by the user
**  in the VnodeInformation dialog in the msi properties
**  appending a virtual node count to each property.
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
**	18-Sep-2008 (drivi01)
**		Created.
*/
UINT __stdcall
ingres_store_vnode(MSIHANDLE hInstall)
{
	int vn_count=0;
	char szBuf[MAX_PATH], newprop[MAX_PATH];
	DWORD dwSize=0;
	int status;

	dwSize=sizeof(szBuf);
	if (MsiGetProperty(hInstall, "VNODE_COUNT", szBuf, &dwSize) != ERROR_SUCCESS)
	{
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		vn_count = atoi(szBuf);
		vn_count++;
		sprintf(szBuf, "%d", vn_count);
		MsiSetProperty(hInstall, "VNODE_COUNT", szBuf);
		MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");
	}
	
	//check VNodeName property first in case if there's duplicates
	dwSize=sizeof(szBuf);
	if (MsiGetProperty(hInstall, "VNodeName", szBuf, &dwSize)!= ERROR_SUCCESS)
	{
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		//check duplicate names
		int count=vn_count;
		char tempBuf[MAX_PATH];
		int i;
		for (i=count-1; i>0; i--)
		{
			dwSize=sizeof(tempBuf);
			sprintf(newprop, "VNODE_NAME%d", i);
			if (MsiGetProperty(hInstall, newprop, tempBuf, &dwSize))
				return ERROR_INSTALL_FAILURE;
			if (strcmp(tempBuf, szBuf) == 0)
			{
				//if we're here then node name is duplicated, undo everything in this CA
				MsiSetProperty(hInstall, "VNodeName", "");
				vn_count--;
				sprintf(szBuf, "%d", vn_count);
				MsiSetProperty(hInstall, "VNODE_COUNT", szBuf);
				MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
				return ERROR_SUCCESS;
			}
		}
		sprintf(newprop, "VNODE_NAME%d", vn_count);
		status = MsiSetProperty(hInstall, newprop, szBuf);
	}

	dwSize=sizeof(szBuf);
	if (MsiGetProperty(hInstall, "VHostname", szBuf, &dwSize)!= ERROR_SUCCESS)
	{
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		sprintf(newprop, "VHOSTNAME%d", vn_count);
		status = MsiSetProperty(hInstall, newprop, szBuf);
		dwSize=sizeof(szBuf);
		MsiGetProperty(hInstall, newprop, szBuf, &dwSize);
	}
	dwSize=sizeof(szBuf);
	if (MsiGetProperty(hInstall, "Protocol", szBuf, &dwSize)!= ERROR_SUCCESS)
	{
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		sprintf(newprop, "PROTOCOL%d", vn_count);
		status = MsiSetProperty(hInstall, newprop, szBuf);
	}
	dwSize=sizeof(szBuf);
	if (MsiGetProperty(hInstall, "ListenAddress", szBuf, &dwSize)!= ERROR_SUCCESS)
	{
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		sprintf(newprop, "LISTEN_ADDRESS%d", vn_count);
		status = MsiSetProperty(hInstall, newprop, szBuf);
	}

	dwSize=sizeof(szBuf);
	if (MsiGetProperty(hInstall, "Username", szBuf, &dwSize)!= ERROR_SUCCESS)
	{
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		sprintf(newprop, "USERNAME%d", vn_count);
		status = MsiSetProperty(hInstall, newprop, szBuf);
	}
	dwSize=sizeof(szBuf);
	if (MsiGetProperty(hInstall, "Password", szBuf, &dwSize)!= ERROR_SUCCESS)
	{
		return (ERROR_INSTALL_FAILURE);
	}
	else
	{
		sprintf(newprop, "PASSWORD%d", vn_count);
		status = MsiSetProperty(hInstall, newprop, szBuf);
	}

	return ERROR_SUCCESS;

}
/*
**  Name:ingres_add_vnode_to_reg
**
**  Description:
**	Searches MSI for all possible virtual nodes and their information
**	and stores the information in the registry for post installer.
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
**	18-Sep-2008 (drivi01)
**	    Created.
*/
UINT __stdcall
ingres_add_vnode_to_reg(MSIHANDLE hInstall)
{
     int vn_count=0;
     char szBuf[MAX_PATH], tchII_INSTALLATION[3], KeyName[MAX_PATH], kname[MAX_PATH];
     DWORD dwSize;
     HKEY hKey;
	
     dwSize=sizeof(tchII_INSTALLATION);
     MsiGetProperty(hInstall, "II_INSTALLATION", tchII_INSTALLATION, &dwSize);

     sprintf(KeyName,"Software\\IngresCorporation\\Ingres\\%s_Installation",
		tchII_INSTALLATION);
     if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey))
	return ERROR_INSTALL_FAILURE;

	 dwSize=sizeof(szBuf);
	 if (MsiGetProperty(hInstall, "VNODE_COUNT", szBuf, &dwSize) != ERROR_SUCCESS)
	 {
		 RegCloseKey(hKey);
		 return (ERROR_INSTALL_FAILURE);
	 }
	 else
	 {
		vn_count = atoi(szBuf);
	 }

     	 RegSetValueEx(hKey, "vnode_count", 0, REG_SZ, szBuf, strlen(szBuf)+1);
	
	 while (vn_count>0)
	 {
		dwSize=sizeof(szBuf);
		sprintf(kname, "VHOSTNAME%d", vn_count);
		if (MsiGetProperty(hInstall, kname, szBuf, &dwSize) != ERROR_SUCCESS)
		{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
		}
		else
		{
		RegSetValueEx(hKey, kname, 0, REG_SZ, szBuf, strlen(szBuf)+1);
		}
		dwSize=sizeof(szBuf);
		sprintf(kname, "PROTOCOL%d", vn_count);
		if (MsiGetProperty(hInstall, kname, szBuf, &dwSize) != ERROR_SUCCESS)
		{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
		}
		else
		{
		RegSetValueEx(hKey, kname, 0, REG_SZ, szBuf, strlen(szBuf)+1);
		}

	    dwSize=sizeof(szBuf);
		sprintf(kname, "LISTEN_ADDRESS%d", vn_count);
		if (MsiGetProperty(hInstall, kname, szBuf, &dwSize) != ERROR_SUCCESS)
		{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
		}
		else
		{
		RegSetValueEx(hKey, kname, 0, REG_SZ, szBuf, strlen(szBuf)+1);
		}

		dwSize=sizeof(szBuf);
		sprintf(kname, "VNODE_NAME%d", vn_count);
		if (MsiGetProperty(hInstall, kname, szBuf, &dwSize) != ERROR_SUCCESS)
		{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
		}
		else
		{
		RegSetValueEx(hKey, kname, 0, REG_SZ, szBuf, strlen(szBuf)+1);
		}

		dwSize=sizeof(szBuf);
		sprintf(kname, "USERNAME%d", vn_count);
		if (MsiGetProperty(hInstall, kname, szBuf, &dwSize) != ERROR_SUCCESS)
		{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
		}
		else
		{
		RegSetValueEx(hKey, kname, 0, REG_SZ, szBuf, strlen(szBuf)+1);
		}

		dwSize=sizeof(szBuf);
		sprintf(kname, "PASSWORD%d", vn_count);
		if (MsiGetProperty(hInstall, kname, szBuf, &dwSize) != ERROR_SUCCESS)
		{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
		}
		else
		{     
		RegSetValueEx(hKey, kname, 0, REG_SZ, szBuf, strlen(szBuf)+1);
		}
		vn_count--;
	 }
	
     RegCloseKey(hKey);

     return ERROR_SUCCESS;
   
}

