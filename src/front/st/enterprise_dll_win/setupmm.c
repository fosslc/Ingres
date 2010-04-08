/*
**  Copyright (c) 2001-2009 Ingres Corporation.
*/

/*
**  Name: setupmm.c
**
**  Description:
**	All the functions needed by front end during Ingres installation.
**
**  History:
**	07-May-2001 (penga03)
**	    Created.
**	06-Jun-2001 (penga03)
**	    Changed INGRES_GOON to INGRES_CONTINUE.
**	15-Jun-2001 (penga03)
**	    Added ingres_set_msiloc_in_reg(MSIHANDLE).
**	28-jun-2001 (somsa01)
**	    Changed "silent.bat" to "silent.exe".
**	23-July-2001 (penga03)
**	    Added functions GetII_SYSTEM and ingres_upgrade_ver25.
**	17-Aug-2001 (penga03)
**	    Added function ingres_set_feature_attribute.
**	23-Aug-2001 (penga03)
**	    Added function ingres_check_targetpath_duplicated.
**	28-Aug-2001 (penga03)
**	    Added function ingres_cleanup_temp_msi.
**	11-Sep-2001 (penga03)
**	    Set RebootNeeded in Registry for post-installation process
**	    to determine if a reboot needed.
**	24-oct-2001 (penga03)
**	    Pass the handle of the parent window to MessageBox. The way
**	    to find the parent window is to use function FindWindow.
**	29-oct-2001 (penga03)
**	    In 2.0/9808, the existance of Ingres/Ice is determined by
**	    checking ice.crs.
**	02-nov-2001 (penga03)
**	    The existance of Ingres/EsqlCobol is determined by checking
**	    eqmdef.cbl.
**	06-nov-2001 (penga03)
**	    Deleted function GetII_SYSTEM and modified function
**	    GetInstallPath to get install path (i.e. II_SYSTEM) from
**	    registry according to the installation id that input. Replaced
**	    all invocations of GetII_SYSTEM with GetInstallPath.
**	19-nov-2001 (penga03)
**	    If upgrade, delete the old Ingres ODBC Driver.
**	20-nov-2001 (somas01)
**	    LogonUser() can also return ERROR_LOGON_TYPE_NOT_GRANTED.
**	10-dec-2001 (penga03)
**	    Moved the function GetInstallPath to commonmm.c.
**	17-dec-2001 (somsa01)
**	   Modified CheckMicrosoft() to work on Win64.
**	31-dec-2001 (penga03)
**	    Added function ingres_install_license.
**	28-jan-2002 (penga03)
**	    Removed ingres_cleanup_temp_msi and ingres_set_msiloc_in_reg. 
**	    Added ingres_cleanup_commit, ingres_cleanup_immediate, and 
**	    ingres_update_properties.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	30-jan-2002 (penga03)
**	    Added ingres_set_targetpath.
**	12-feb-2002 (penga03)
**	    Moved ingres_set_timezone to dbmsnetmm.c, and IsWindows95() to commonmm.c.
**	28-feb-2002 (penga03)
**	    Added Ingres SDK specific functions, ingressdk_check_targetpath_duplicated, 
**	    ingressdk_cleanup, ingressdk_set_environment and ingressdk_uninstall.
**	12-mar-2002 (penga03)
**	    Added ingressdk_remove. 
**	26-mar-2002 (penga03)
**	    Completed the OpenROAD part in ingressdk_uninstall and 
**	    commented it out since the SDK doesn't include OpenROAD 
**	    for now.
**	16-jul-2002 (penga03)
**	    Since Ingres/ICE does not support Spyglass any more, instead, 
**	    it supports Apache, replace wherever that are associated with 
**	    Spyglass with Apache.
**	04-aug-2002 (penga03)
**	     Changed the way we checked which Ingres feature installed. 
**	     Following lists the new way:
**
**	     DBMS: ii.<computer name>.config.dbms.<release id> COMPLETE
**	     REPLICATOR: %II_SYSTEM%\ingres\rep
**	     STAR: ii.<computer name>.config.star.<release id> COMPLETE
**	     ICE: %II_SYSTEM%\ingres\ice
**	     NET: ii.<computer name>.config.net.<release id> COMPLETE
**	     ESQLC: %II_SYSTEM%\ingres\demo\esqlc
**	     ESQLCOB: %II_SYSTEM%\ingres\files\eqmdef.cbl
**	     ESQLFORTRAN: %II_SYSTEM%\ingres\files\eqdef.f
**	     TOOLS: %II_SYSTEM%\ingres\files\english\rbfcat.hlp
**	     VISION: %II_SYSTEM%\ingres\abf
**	     DOC: shortcuts or %II_SYSTEM%\ingres\files\english\gs.pdf
**	     EMBEDDED: ii.<computer name>.setup.embed_installation ON
**
**	     Functions ingres_set_feature_attribute() and 
**	     ingres_upgrade_ver25() have been affected.
**	02-oct-2002 (penga03)
**	    Modified ingres_verify_paths() to write the path being 
**	    verified back, if being corrected. Also, corrected the 
**	    error message here. 
**	08-oct-2002 (penga03)
**	     Based on the change on GetLinkInfo(), made corresponding 
**	     correction on ingres_upgrade_ver25() and 
**	     ingres_set_feature_attribute().
**	05-feb-2003 (penga03)
**	    Added a new response file parameter called "installcalicensing" 
**	    to control the running of the CA licensing install.
**	06-feb-2003 (penga03)
**	    Removed ingres_check_license().
**	20-may-2003 (penga03)
**	    In ingressdk_uninstall, took away the comment on OpenROAD, 
**	    and changed the command that uninstalls Portal 4.51.
**	29-may-2003 (penga03)
**	    Changed the way to decide which Ingres components are 
**	    installed by checking careglog.log.
**	04-jun-2003 (penga03)
**	    Corrected Ingres documentation component name to "Ingres 
**	    II On-line Documentation" that is logged in CAREGLOG.LOG 
**	    by CA-installer.
**	09-jul-2003 (penga03)
**	    In ingres_upgrade_ver25(), replaced strstr with _tcsstr, 
**	    and before calling _tcsstr, converted both strings to 
**	    lowercase. This is to fix problem INGINS:183.
**	06-aug-2003 (penga03)
**	    Add a new property INGRES_ORFOLDER to hold the OpenROAD 
**	    folder. 
**	    And, fix the bug existed in 2.5/0011 installer: it creates 
**	    an additional "Ingres II Online Documentation" folder. 
**	    Also, refresh the folders we may have touched.
**	    In ingres_cleanup_commit(), remove the folder "Ingres II" 
**	    if it is empty.
**	18-aug-2003 (penga03)
**	    In ingres_verify_user(), made changes so that the user needs 
**	    to input password ONLY if he/she chooses to install 
**	    Enterprise DBMS.
**	21-aug-2003 (penga03)
**	    To install DBMS, it is required that the user that currently 
**	    logged on has following three privileges and rights:
**	    1) Act as part of operating system (SE_TCB_NAME)
**	    2) Log on as a service (SE_SERVICE_LOGON_NAME)
**	    3) Access this computer from the network (SE_NETWORK_LOGON_NAME)
**	    We used to use LogonUser() to check if the user have the 
**	    SE_TCB_NAME privilege. Now we changed to use CheckUserRights() 
**	    (defined in commonmm.c) because of following reasons: 
**	    1) In MSDN (01/2003), it is said that on Windows 2000, the 
**	       process calling LogonUser requires the SE_TCB_NAME privilege; 
**	       on Windows XP the privilege is no longer required. 
**	    2) LogonUser() needs password, but if embed install, the user 
**	       does NOT input password.
**	    3) LogonUser() can NOT verify the two account rights 
**	       SE_NETWORK_LOGON_NAME and SE_SERVICE_LOGON_NAME at the same 
**	       time.
**	20-oct-2003 (penga03)
**	    In ingres_verify_user(), made changes to verify user's rights 
**	    only when feature IngresDBMS is to be installed.
**	    Made changes so that call SHChangeNotify(SHCNE_RMDIR... only 
**	    when the folder has been removed successfully.
**	04-dec-2003 (somsa01)
**	    Added TCP_IP as a valid protocol.
**	17-dec-2003 (penga03)
**	    Added ingres_checkiflicenseviewed().
**	30-jan-2004 (penga03)
**	    Added Ingres .NET Data Provider feature.
**	09-mar-2004 (penga03)
**	    In ingres_set_targetpath(), set the default directory of Ingres 
**	    to "Program Files\CA\Advantage Ingres [instanceid]".
**	19-mar-2004 (penga03)
**	    Modified ingres_verify_user() so that the installation will 
**	    continue even the user does not have those required rights.
**	23-mar-2004 (penga03)
**	    In ingres_check_targetpath_duplicated() added a check to make 
**	    sure the installation being checked do exists.
**	23-jun-2004 (penga03)
**	    Added two response file parameters installmdb, mdbname, and
**	    corresponding properties are INGRES_INSTALL_MDB("0"), 
**	    INGRES_MDB_NAME("mdb").
**	30-jun-2004 (penga03)
**	    Added a response file parameter noaddremoveprogram, 
**	    corresponding property is INGRES_NO_ARP. For embedded install, 
**	    if this parameter is set to 1, Ingres will not be displayed in 
**	    the Add/Remove Programs list.
**	16-jul-2004 (penga03)
**	    Modified the response file parameters installmdb, mdbname to
**	    II_INSTALL_MDB and II_MDB_NAME.
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	27-jul-2004 (penga03)
**	    Added a response file parameter removecab.
**	04-aug-2004 (penga03)
**	    Corrected some errors after removing the references to 
**	    "Advantage".
**	18-aug-2004 (penga03)
**	    Modified the ingres_cleanup_commit() to remove the registry 
**	    key if the Ingres instance is being removed.
**	07-sep-2004 (penga03)
**	   Removed all references to service password since by default 
**	   Ingres service is installed using Local System account.
**	10-sep-2004 (penga03)
**	    Removed MDB.
**	15-sep-2004 (penga03)
**	   Backed out the change made on 07-sep-2004 (471628).
**	22-sep-2004 (penga03)
**	    Removed ingres_install_license.
**	27-sep-2004 (penga03)
**	   If the response file parameter leaveingresrunninginstallincomplete
**	   is set to NO, set the property INGRES_LEAVE_RUNNING to 0.
**	03-nov-2004 (penga03)
**	    Added two response file parameters ingresprestartuprunprg, 
**	    ingresprestartuprunparams, and corresponding properties are 
**	    INGRES_PRESTARTUPRUNPRG, INGRES_PRESTARTUPRUNPARAMS.
**	03-nov-2004 (penga03)
**	    Added one response file parameter II_MDB_INSTALL, its 
**	    corresponding property is INGRES_MDB_INSTALL.
**	05-nov-2004 (penga03)
**	    Modified ingres_set_shortcuts so that documentation 
**	    shortcuts can be created under the current user's 
**	    folder.
**	08-dec-2004 (penga03)
**	    In ingres_upgrade_ver25 corrected the error while checking
**	    250011 version.
**	14-dec-2004 (penga03)
**	    Added ingres_start_ingres() to start Ingres and IVM.
**	16-dec-2004 (penga03)
**	    Added a new response file parameter "ivmflag" whose value is
**	    set to be NOMAXEVENTSREACHEDWARNING. 
**	11-jan-2005 (penga03)
**	    Added a new response file parameter II_DECIMAL.
**	12-jan-2005 (penga03)
**	    The user now has an option in response file to select one of 
**	    following languages: english, deu, esn, fra, ita, jpn, ptb, 
**	    sch by setting II_LANGUAGE.
**	13-jan-2005 (penga03)
**	    Added two response files paramters II_MDBNAME, II_MDBDEBUG.
**	14-feb-2005 (penga03)
**	    Fail the installation if MsiSetTargetPath fails.
**	28-feb-2005 (penga03)
**	    In ingres_start_ingres(), start Ingres as service if auto_start
**	    is set.
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	14-march-2005 (penga03)
**	    Added a new response file parameter II_EMBED_DOC.
**	14-march-2005 (penga03)
**	    The user needs to input the password for Ingres service for a 
**	    cluster install.
**	15-march-2005 (penga03)
**	    Corrected some error when creating embedded doc shortcuts. 
**	21-march-2005 (penga03)
**	    Add the required rights to the logon user if he/she agrees.
**	23-march-2005 (penga03)
**	    Modified ingres_cleanup_commit() to clean up %ii_system% after 
**	    uninstall.
**	31-march-2005 (penga03)
**	    Do not verify user's password for a cluster install since the  
**	    Ingres service will use LocalSystem account for this case.
**	01-apr-2005 (penga03)
**	    For a cluster install, the startup type for the Ingres service
**	    should always be manual.
**	08-apr-2005 (penga03)
**	    Fixed the error in ingres_verify_user().
**	19-may-2005 (penga03)
**	    Fixed an error in ingres_start_ingres().
**	23-may-2005 (penga03)
**	    Moved the checks on II_SYSTEM to the pre-installer.
**	14-june-2005 (penga03)
**	    Before install .NET Data Provider feature, check if .NET
**	    Framework 1.1 was installed, if not, the user is not allowed 
**	    to install .NET Data Provider.
**	15-june-2005 (penga03)
**	    Modified ingres_base_settings, ingres_set_targetpath and 
**	    ingres_check_targetpath_duplicated so that if the instance 
**	    to be installed was already installed by another product, we 
**	    will keep the consistency of II_SYSTEM.
**	29-jun-2005 (penga03)
**	    Added ingres_set_controls().
**	05-jul-2005 (penga03)
**	    In ingres_set_properties() set REINSTALL only when necessary
**	    and set the feature/features to be installed to ADDLOCAL.
**	12-jul-2005 (penga03)
**	    In ingres_upgrade_ver25() and ingres_cleanup_commit(), 
**	    replaced SHGetSpecialFolderLocation with SHGetFolderLocation.
**	18-jul-2005 (penga03)
**	    Set INGRES_UPGRADEUSERDBS to FALSE by default.
**	29-jul-2005 (penga03)
**	    Do not block the serviceauto if cluster is set while reading
**	    the response file, because if DBMS is not chosen, the user 
**	    still should be able to set Ingres service startup type to 
**	    automatic.
**	31-aug-2005 (penga03)
**	    Added verification for timezone, terminal, date and money format.
**	22-Nov-2005 (drivi01)
**	    Modified routines that store service password to store it if it's
**	    been entered regardless if the install is Silent or if the
**	    Automatic startup option was chosent.
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	09-May-2006 (drivi01)
**	    Modify example in ingres_verify_paths to give example of
**	    ingres path instead of CA.
**    	06-Sep-2006 (drivi01)
**          SIR 116599
**          As part of the initial changes for this SIR.  Modify Ingres
**          name to "Ingres II" instead of "Ingres [ II ]", Ingres location
**          to "C:\Program Files\Ingres\IngresII" instead of
**          C:\Program Files\Ingres\Ingres [ II ]. Remove old shortcut
**	    and add new shortcut to Start->Programs->Ingres->Ingres II.
**	04-Dec-2006 (drivi01)
**	    Added routines for processing new property INGRES_DATE_TYPE
**	    when ingres is being installed from response file. Response
**	    file value corresponding to this property is II_DATE_TYPE_ALIAS.
**	05-Jan-2007 (bonro01)
**	    Change Getting Started Guide to Installation Guide.
**	03-May-2007 (karye01)
**	    Adding missing comment from change #486469.
**	    Bug #118202
**	    Changed minimum size allowed for Log File to 32 MB;
**	    changed default size for Log File to 256 MB.
**      14-May-2007 (drivi01)
**          Changed the title of installer window in the routines throughout
**          the code to make sure all the errors are brought on foreground.
**	16-Jan-2009 (whiro01) SD Issue 132899
**	    In conjunction with the fix for this issue, fixed a few compiler warnings.
*/
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <malloc.h>
#include <shlobj.h>
#include <Ddeml.h>
#include "commonmm.h"
#include <lm.h>
#include <tchar.h>

#define HA_SUCCESS					0
#define HA_FAILURE					1
typedef struct tagDRIVE
{
    char DriveName[3];
}DRIVE;

typedef BOOL (WINAPI *PSQLREMOVEDRIVERPROC)(LPCSTR, BOOL, LPDWORD);

extern int liccheck (char *lic_code, BOOL bStartService);

char *IngLangs[]=
{
"deu", "esn", "fra", "ita", "jpn", "ptb", "sch", "english", 0
};

BOOL 
IsValidLang(char *Lang)
{
    int i;

    if (!Lang[0])
	return FALSE;

    i=0;
    while(IngLangs[i])
    {
	if (!_stricmp(Lang, IngLangs[i]))
	    return TRUE;
	i++;
    }
    return FALSE;
}

char *MdbSizes[]=
{
"tiny", "small", "medium", "large", "huge", 0
};

BOOL 
IsValidMdbSize(char *MdbSize)
{
    int i;

    if (!MdbSize[0])
	return FALSE;

    i=0;
    while(MdbSizes[i])
    {
	if (!_stricmp(MdbSize, MdbSizes[i]))
	    return TRUE;
	i++;
    }
    return FALSE;
}

typedef struct tagCOMPONENT
{
    char ComponentName[128];
}COMPONENT;

COMPONENT InstalledComponents[30];
int Count=-1;

int 
ComponentInstalled(char *c)
{
    int i=0;

    if (Count==-1)
	return 0;

    while (i <= Count)
    {
	if (!_stricmp(InstalledComponents[i].ComponentName, c))
	    return 1;
	i++;
    }

    return 0;
}

int 
InsertComponent(char *c)
{
    if (Count==30)
	return 0;

    if (!ComponentInstalled(c))
    {
	Count++;
	strcpy(InstalledComponents[Count].ComponentName, c);
	return 1;
    }

    return 0;
}

BOOL 
CheckMicrosoft(MSIHANDLE hInstall)
{ 
    char	strValue[MAX_PATH+1];
    char	*p;

    if (GetRegistryEntry(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\InetStp",
			 "PathWWWRoot", strValue))
    {
	MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", strValue);
	return TRUE;
    }
	
    if(GetRegistryEntry(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots", "/", strValue))
	goto ValueFound;

    if(GetRegistryEntry(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots", "/,", strValue))
	goto ValueFound;
	
    return FALSE;
	
ValueFound:
    p=strchr(strValue, ',');
    if(*p)
	*p=0;

    MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", strValue);
    return TRUE;
}

BOOL 
CheckNetscape(MSIHANDLE hInstall)
{  
	char szComputerName[MAX_COMPUTERNAME_LENGTH+1], strValue[MAX_PATH+1], szRegKey[512];
	DWORD size;
	FILE *file;
	char *p, line[80], path[MAX_PATH+1];
	
	size=sizeof(szComputerName);
	GetComputerName(szComputerName,&size);
	
	sprintf(szRegKey, "SOFTWARE\\Netscape\\https-%s", szComputerName);
	if(GetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
		goto ValueFound;
	
	sprintf(szRegKey, "SOFTWARE\\Netscape\\httpd-%s", szComputerName);
	if (GetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
		goto ValueFound;
	
	sprintf(szRegKey, "SOFTWARE\\Netscape\\Enterprise\\3.5.1\\https-%s", szComputerName);
	if (GetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
		goto ValueFound;
	
	sprintf(szRegKey, "SOFTWARE\\Netscape\\Enterprise\\3.5.1\\httpd-%s", szComputerName);
	if (GetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
		goto ValueFound;
	
	sprintf(szRegKey, "SOFTWARE\\Netscape\\Enterprise\\4.0\\https-%s", szComputerName);
	if (GetRegistryEntry(HKEY_LOCAL_MACHINE, szRegKey, "ConfigurationPath", strValue))
		goto ValueFound;
	
	return FALSE;
	
ValueFound:
	
	/* Get netscape web server configuration file name */
	p=strchr(strValue, '/');
	while(p!=NULL)
	{
		*p='\\';
		p=strchr(strValue, '/');
	}
	strcat(strValue, "\\obj.conf");
	
	/* Get netscape web server path */
	file=fopen(strValue, "r");
	if(file==NULL)
		return FALSE;	
	while(!feof(file))
	{
		fgets(line, 80, file);
		p=strstr(line, "root=");
		if(p!=NULL)
		{
			p+=6;
			break;
		}
	}
	fclose(file);
	*(p+strlen(p)-2)=0;
	strcpy(path, p);
	
	/* Convert '/' to '\' */
	p=strchr(path, '/');
	while(p!=NULL)
	{
		*p='\\';
		p=strchr(path, '/');
	}
	
	MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", path);
	return TRUE;
}

BOOL 
CheckApache(MSIHANDLE hInstall)
{ 	
    char KeyName[]="SOFTWARE\\Apache Group\\Apache";
    HKEY hKey;

    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey))
    {
	DWORD dwIndex;

	if(!RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwIndex, NULL, 
	                    NULL, NULL, NULL, NULL, NULL, NULL))
	{
	    char szName[7];

	    dwIndex--;
	    if(!RegEnumKey(hKey, dwIndex, szName, sizeof(szName)))
	    {
		HKEY hSubKey;

		if(!RegOpenKeyEx(hKey, szName, 0, KEY_QUERY_VALUE, &hSubKey))
		{
		    char szData[MAX_PATH];
		    DWORD dwSize=sizeof(szData);

		    if(!RegQueryValueEx(hSubKey, "ServerRoot", 0, NULL, (BYTE *)szData, &dwSize))
		    {
			strcat(szData, "htdocs");
			MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", szData);
			return TRUE;
		    }
		}
	    }
	}
    }

    if(!RegOpenKeyEx(HKEY_CURRENT_USER, KeyName, 0, KEY_ALL_ACCESS, &hKey))
    {
	DWORD dwIndex;

	if(!RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwIndex, NULL, 
	                    NULL, NULL, NULL, NULL, NULL, NULL))
	{
	    char szName[7];

	    dwIndex--;
	    if(!RegEnumKey(hKey, dwIndex, szName, sizeof(szName)))
	    {
		HKEY hSubKey;

		if(!RegOpenKeyEx(hKey, szName, 0, KEY_QUERY_VALUE, &hSubKey))
		{
		    char szData[MAX_PATH];
		    DWORD dwSize=sizeof(szData);

		    if(!RegQueryValueEx(hSubKey, "ServerRoot", 0, NULL, (BYTE *)szData, &dwSize))
		    {
			strcat(szData, "htdocs");
			
			MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", szData);
			return TRUE;
		    }
		}
	    }
	}
    }
    return FALSE;	
}

BOOL
GetColdFusionPath(char *strColdFusionPath) 
{  
	char strValue[MAX_PATH+1];
	
	if(GetRegistryEntry(HKEY_CURRENT_USER, "Software\\Allaire\\Studio45\\FileLocations", "ProgramDir", strValue))
	{ 
		strcpy(strColdFusionPath, strValue);
		return TRUE;
	}
	if(GetRegistryEntry(HKEY_CURRENT_USER, "Software\\Allaire\\Studio4\\FileLocations", "ProgramDir", strValue))
	{ 
		strcpy(strColdFusionPath, strValue);
		return TRUE;
	}
	
	return FALSE;
}

/*
**  Name:ingres_verify_paths 
**
**  Description:
**	Varify pathes of database files, transaction log file and dual log file.
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
**	09-May-2001 (penga03)
**	    Created.
*/
UINT __stdcall 
ingres_verify_paths(MSIHANDLE hInstall)
{
	char buf[MAX_PATH+1], lpText[512];
	DWORD nret;
	double LogFileSize;
	char *databaseName[] = {"INGRES_DATADIR",
							"INGRES_CKPDIR",
							"INGRES_JNLDIR",
							"INGRES_DMPDIR",
							"INGRES_WORKDIR",
							"INGRES_PRIMLOGDIR",
							0
							};
	char **sharedDisks, sharedDisk[3];
	int sharedDiskCount, i, j;
	BOOL bFound;
	
	sharedDisks = (char**)malloc(sizeof(char*) * 256);
	sharedDisks[0] = (char*)malloc(sizeof(char) * 256 * 3);
	
	sharedDiskCount=0; i=0;
    while(databaseName[i])
	{
		nret=sizeof(buf);
		memset(buf, 0, nret);
		MsiGetProperty(hInstall, databaseName[i], buf, &nret);
		if ( !VerifyPath(buf) )
		{ 
			sprintf(lpText, "Invalid path: '%s'\nPlease enter a valid path string.\nFor example:  'C:\\Program Files\\Ingres\\IngresII'.", buf);
			MyMessageBox(hInstall, lpText);
			MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
			return (ERROR_SUCCESS);
		}
		MsiSetProperty(hInstall, databaseName[i], buf);

		strncpy(sharedDisk, buf, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}

		i++;
	}



	nret=sizeof(buf);
	memset(buf, 0, nret);
	if (!MsiGetProperty(hInstall, "INGRES_ENABLEDUALLOG", buf, &nret) 
		&& buf[0] && !_stricmp(buf, "TRUE"))
	{
		nret=sizeof(buf);
		memset(buf, 0, nret);
		MsiGetProperty(hInstall, "INGRES_DUALLOGDIR", buf, &nret);
		if ( !VerifyPath(buf) )
		{ 
			sprintf(lpText, "Invalid path: '%s'\nPlease enter a valid path string.\nFor example:  'C:\\Program Files\\Ingres\\IngresII'.", buf);
			MyMessageBox(hInstall, lpText);
			MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
			return (ERROR_SUCCESS);
		}
		MsiSetProperty(hInstall, "INGRES_DUALLOGDIR", buf);

		strncpy(sharedDisk, buf, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
	}

	nret=sizeof(buf);
	memset(buf, 0, nret);
	if (!MsiGetProperty(hInstall, "INGRES_CLUSTER_INSTALL", buf, &nret) && !strcmp(buf, "1") 
		&& CheckSharedDiskInSameRG(sharedDisks, sharedDiskCount)!=HA_SUCCESS)
	{
		MyMessageBox(hInstall, "Not all shared disks are in the same Resource Group!");
		MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
		return (ERROR_SUCCESS);
	}
	free ((*sharedDisks));

	nret=sizeof(buf);
	memset(buf, 0, nret);
	MsiGetProperty(hInstall, "INGRES_LOGFILESIZE", buf, &nret);
	LogFileSize = atof(buf);
	if ( LogFileSize < 32 )
	{
		strcpy(lpText,"The minimum allowable size for a transaction log file is 32MB.");
		MyMessageBox(hInstall, lpText);
		MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
		return (ERROR_SUCCESS);
	}
	
	MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");
	return (ERROR_SUCCESS);
}

/*
**  Name:ingres_warning_on_sql92 
**
**  Description:
**	Give a warning message when user checks ANSI/ISO Entery SQL-92.
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
**	09-May-2001 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_warning_on_sql92(MSIHANDLE hInstall) 
{
	char *lpText;
	char buf[MAX_PATH+1];
	DWORD nret;
	
	nret = sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_ENABLESQL92", buf, &nret);
	if(!_stricmp(buf, "TRUE"))
	{
		lpText="Please note:  Selecting the ANSI/ISO Entry SQL-92 option means that Ingres will enforce the ANSI/ISO standards for case sensitivity.  This means that user names, database names, etc. will be case sensitive.  If you want the SQL functionality of outer join, ANSI constraints, but do not want case sensitivity, do not select this option.  Note that this option cannot be changed after installation.";
		MyMessageBox(hInstall, lpText);
	}
	
	return (ERROR_SUCCESS);
}

/*
**  Name:ingres_set_http_server
**
**  Description:
**	Set HTTP server.
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
**	11-May-2001 (penga03)
**	    Created.
*/

UINT __stdcall 
ingres_set_http_server(MSIHANDLE hInstall) 
{
	char buf[512];
	int nret;
	
	nret=sizeof(buf);
	
	MsiGetProperty(hInstall, "INGRES_HTTP_SERVER", buf, &nret);
	if(!_stricmp(buf, "Microsoft"))
	{
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "Microsoft Internet Information Server");
		if(!CheckMicrosoft(hInstall))
			MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", "");
		return ERROR_SUCCESS;
	}
	if(!_stricmp(buf, "Netscape"))
	{
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "iPlanet Web Server");
		if(!CheckNetscape(hInstall))
			MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", "");
		return ERROR_SUCCESS;
	}
	if(!_stricmp(buf, "Apache"))
	{
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "Apache HTTP Server");
		if(!CheckApache(hInstall))
			MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", "");
		return ERROR_SUCCESS;
	}
	
	MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "Other Web Server");
	MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", "");
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_ini_ice_config
**
**  Description:
**	Server HTTP server and ColdFusion.
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
**	11-May-2001 (penga03)
**	    Created.
*/

UINT __stdcall 
ingres_ini_ice_config(MSIHANDLE hInstall) 
{
	char strColdFusionPath[MAX_PATH+1];
	
	/* set coldfusion studio path */
	if(GetColdFusionPath(strColdFusionPath))
		MsiSetProperty(hInstall, "INGRES_COLDFUSION_PATH", strColdFusionPath);
	else
		MsiSetProperty(hInstall, "INGRES_COLDFUSION_PATH", "");
	
	/* set HTTP server path */
	if(CheckMicrosoft(hInstall))
	{
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER", "Microsoft");
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "Microsoft Internet Information Server");
		return ERROR_SUCCESS;
	}
	
	if(CheckNetscape(hInstall))
	{
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER", "Netscape");
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "iPlanet Web Server");
		return ERROR_SUCCESS;
	}
	
	if(CheckApache(hInstall))
	{
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER", "Apache");
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "Apache HTTP Server");
		return ERROR_SUCCESS;
	}
	
	MsiSetProperty(hInstall, "INGRES_HTTP_SERVER", "Other");
	MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_TITLE", "Other Web Server");
	MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", "");
	
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_set_coldfusion
**
**  Description:
**	Set ColdFusion.
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
**	11-May-2001 (penga03)
**	    Created.
*/

UINT __stdcall 
ingres_set_coldfusion(MSIHANDLE hInstall) 
{
	char buf[16];
	DWORD nret;
	char strColdFusionPath[MAX_PATH+1];
	
	nret=sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_COLDFUSION", buf, &nret);
	if(!_stricmp(buf, "TRUE"))
	{
		if(GetColdFusionPath(strColdFusionPath))
			MsiSetProperty(hInstall, "INGRES_COLDFUSION_PATH", strColdFusionPath);
		else
			MyMessageBox(hInstall, "You must install ColdFusion Studio in order to add Ingres ICE Extensions.  You can download the latest version from:  http://www.allaire.com.");
	}
	
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_verify_user
**
**  Description:
**	Verify user's rights and passwords for installing Ingres.
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
**	14-May-2001 (penga03)
**	    Created.
**	05-Jun-2001 (penga03)
**	    Verify user rights and passwords if user chooses those
**	    features that need postinstallation.
**	11-Jun-2001 (penga03)
**	    Show the user name that currently logged onto the system in
**	    the message box.
**	29-Aug-2001 (penga03)
**	    If user doesn't provide password, tell user that he/she must
**	    enter a password and why he/she must to do so.
**	12-nov-2001 (penga03)
**	    Allow user without password. Also, on Windows NT (or Windows
**	    2000), to install Ingres user must have "Log on as a service"
**	    and "Act as part of opertaing system" rights.
**	20-nov-2001 (somas01)
**	    LogonUser() can also return ERROR_LOGON_TYPE_NOT_GRANTED.
**	07-dec-2001 (penga03)
**	    If embed install, or logon user is not "ingres", set property 
**	    INGRES_SERVICELOGINID to NULL.
**	08-oct-2002 (penga03)
**	    Close the handle, phToken, at the end when it is no longer 
**	    needed.
**	20-oct-2003 (penga03)
**	    Verify user's rights only when feature IngresDBMS is set to be 
**	    installed.
**	25-oct-2003 (penga03)
**	    There is no maximum size for transaction log.
**	16-Nov-2006 (drivi01)
**	    Modify the way service is setup. By default LocalSystem 
**	    account owns the service now unless otherwise specified.
*/

UINT __stdcall 
ingres_verify_user(MSIHANDLE hInstall) 
{
    char	UserName[512], PassWord[512], lpText[512], ach[512];
    DWORD	nret, rc;
    HANDLE	phToken;
    INSTALLSTATE iInstalled, iAction;
    BOOL bDbmsSelected, bAuto, bSilent;
	
    MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");
	
    if (IsWindows95())
	return (ERROR_SUCCESS);
	
    bAuto = bDbmsSelected = bSilent = 0;
    nret=sizeof(ach);
    MsiGetProperty(hInstall, "INGRES_SERVICEAUTO", ach, &nret);
    if (!_stricmp(ach, "TRUE"))
	bAuto = 1;
    nret=sizeof(ach);
    MsiGetProperty(hInstall, "UILevel", ach, &nret);
    if (!strcmp(ach, "2"))
	bSilent = 1;
    if (!MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction) 
		&& (iAction == INSTALLSTATE_LOCAL)) 
	bDbmsSelected = 1;

    if (bDbmsSelected)
    {
	nret=sizeof(UserName);
	if (!bSilent)
	{
	    MsiGetProperty(hInstall, "INGRES_SERVICEUSER", UserName, &nret);
	    if(strcmp(UserName, "LocalSystem"))
	    {
		    nret=sizeof(UserName);
		    GetUserName(UserName, &nret);
	    }
	    MsiSetProperty(hInstall, "INGRES_SERVICELOGINID", UserName);
	}
	else
	    MsiGetProperty(hInstall, "INGRES_SERVICELOGINID", UserName, &nret);

	nret=sizeof(PassWord);
	MsiGetProperty(hInstall, "INGRES_SERVICEPASSWORD", PassWord, &nret);

	/* 
	** Verify the password. 
	*/
	if (UserName[0] && PassWord[0])
	{
	    if(!LogonUser(UserName, NULL, PassWord, 
			LOGON32_LOGON_INTERACTIVE, 
			LOGON32_PROVIDER_DEFAULT, &phToken))
	    {
		rc = GetLastError();
		if (rc == ERROR_LOGON_FAILURE)
		{
		    if (!PassWord[0])
		    {
			sprintf(lpText, "Please provide the password for user \"%s\".\nThis is required for the Ingres configuration process to proceed properly.", UserName);
			MyMessageBox(hInstall, lpText);
		    }
		    else
		    {
			sprintf(lpText, "The password you entered for user \"%s\" is incorrect. Please try again.", UserName);
			MyMessageBox(hInstall, lpText);
		    }

		    if (phToken) CloseHandle(phToken);
		    if (!bSilent)
		    {
			MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
			return (ERROR_SUCCESS);
		    }
		    else return (ERROR_INSTALL_FAILURE);
		}
	    } /* end of LogonUser */
	    if (phToken) CloseHandle(phToken);
	}

	/* 
	** Verify the user's privileges and rights. 
	*/
	if (!CheckUserRights(UserName))
	{
	    sprintf(lpText, "You must have the privileges \"Act as part of the operating system\", \"Log on as a service\" and \"Access this computer from the network\". Do you want to assign above privileges to user \"%s\"?", UserName);
	    if (bSilent || AskUserYN(hInstall, lpText))
	    {
		if (!AddUserRights(UserName))
		{
		    if (!bSilent){
		    sprintf(lpText, "Failed to add privileges, \"Act as part of the operating system\", \"Log on as a service\" and \"Access this computer from the network\", to user \"%s\". Please refer to the Installation Guide for information about granting these privileges.", UserName);
		    MyMessageBox(hInstall, lpText);}
		}
	    }
	    MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");
	    return (ERROR_SUCCESS);
	}
    }

    return (ERROR_SUCCESS);
}

/*
**  Name:ingres_ice_verify_paths
**
**  Description:
**	Verify the HTTP server path and ColdFusion path.
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
**	16-May-2001 (penga03)
**	    Created.
*/

UINT __stdcall 
ingres_ice_verify_paths(MSIHANDLE hInstall) 
{
	char buf[MAX_PATH+1], lpText[512];
	DWORD nret;

	MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");

	nret=sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", buf, &nret);
	if(!(buf[0]))
	{
		MyMessageBox(hInstall, "You must enter a value for \"Install Path\" of the HTTP server.");
		MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
		return ERROR_SUCCESS;
	}
	if(GetFileAttributes(buf)==-1)
	{
		sprintf(lpText, "The directory \"%s\" doesn't exist. Please ensure that it is a valid path.", buf);
		MyMessageBox(hInstall, lpText);
		MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
		return ERROR_SUCCESS;
	}

	nret=sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_COLDFUSION", buf, &nret);
	if(!_stricmp(buf, "TRUE"))
	{
		nret=sizeof(buf);
		MsiGetProperty(hInstall, "INGRES_COLDFUSION_PATH", buf, &nret);
		if(!(buf[0]))
		{
			MyMessageBox(hInstall, "You must enter a value for \"Install Path\" of the ColdFusion Studio.");
			MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
			return ERROR_SUCCESS;
		}
		if(GetFileAttributes(buf)==-1)
		{
			sprintf(lpText, "The directory \"%s\" doesn't exist. Please ensure that it is a valid path.", buf);
			MyMessageBox(hInstall, lpText);
			MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
			return ERROR_SUCCESS;
		}
	}

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_set_properties
**
**  Description:
**	This function is used to set Ingres-specific properties (ONLY used in silent
**	mode installation).
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	21-May-2001 (penga03)
**	    Created.
**	16-nov-2001 (penga03)
**	    Set the values of ODBC related properties to 0/1.
**	10-jan-2002 (penga03)
**	    If VBDA is not mentioned in the response file, it should not be 
**	    installed by default. The same with JDBC Driver and JDBC Server.
**	14-jan-2002 (penga03)
**	    Get the "serviceloginid" from the response file. "serviceloginid" 
**	    is set to be "" by default.
**	14-jan-2002 (penga03)
**	    changed the names of the following properties INGRES_RSP, INGRES_RSP_JDBCD, 
**	    INGRES_RSP_JDBCS, INGRES_RSP_VDBA to the corresponding meaningful names, 
**	    INGRES_SILENT_INSTALLED, INGRES_JDBC_DRIVER, INGRES_JDBC_SERVER, INGRES_VDBA.
**	28-jan-2002 (penga03)
**	    If this is a modify (in silent mode) on an installation installed in 
**	    silent mode, set REINSTALL to be "IngresDBMS, IngresNet", so that we 
**	    can add VDBA and JDBC by using response file during modify.
**	14-may-2002 (penga03)
**	    Added a new response file parameter, allusersshortcuts, to 
**	    decide whether or not we create Ingres shortcuts for current 
**	    user or everyone.
**	06-jun-2002 (penga03)
**	    Added a new property, INGRES_LEAVE_RUNNING 
**	    (leaveingresrunninginstallincomplete in response file), to 
**	    give user an option to determine whether leave Ingres running 
**	    or not when post install process completes.
**	26-jan-2004 (penga03)
**	    Added two entries in response file, dateformat and moneyformat.
**	23-mar-2004 (penga03)
**	    Added startivm in response file, to support for an option to 
**	    prevent the installer starting IVM.
**	01-aug-2004 (penga03)
**	    Modified ingres_set_properties so that if the user sets 
**	    timezone="default" in the response file, the installer will 
**	    get OS default timezone. The same as charset and terminal.
**	16-Nov-2006 (drivi01) and (karye01)
**	    Enhanced response file processing to handle new version of
**	    response file.
**	06-Feb-2007 (drivi01)
**	    BUG 117635
**	    Added "User Defined Properties" section to the response file.
**	    Any property can be now moved to "User Defined Properties" section
**	    and it will be properly processed by the installer.
**	06-Mar-2007 (drivi01)
**	    Set INGRES_RSP_JDBCS property if II_COMPONENT_NET is set to "YES".
**	    In new response file we don't have separate property for DAS/JDBC
**	    server, we will install it with NET package.
**	14-Mar-2007 (Drivi01)
**	    Fixed routine for INGRES_DATE_TYPE, the comparsion results were
**	    handled in a wrong way.
**	28-May-2008 (drivi01)
**	    Add a redesigned name for setup_ingresodbcreadonly which is now
**	    II_SETUP_ODBC_READONLY.
**	14-Jul-2009 (drivi01)
**	    SIR: 122309
**	    Add a new parameter to the response file called II_CONFIG_TYPE.
**	    The parameter will setup configuration type for the Ingres instance.
**	    Parameter will take 3 options: TXN, BI, CM.
**	    II_CONFIG_TYPE=NULL or the absence of II_CONFIG_TYPE parameter
**	    will revert back to Classic/Traditional Ingres configuration.	
**	    
*/
UINT __stdcall 
ingres_set_properties(MSIHANDLE hInstall) 
{

	char strRSPFile[MAX_PATH+1], lpText[512], lpReturnedString[MAX_PATH+1];
	char strInstallCode[3], strInstallPath[MAX_PATH+1];
	DWORD size, nret;
	char strBuf[1024];
	BOOL bLangPack=0;
	char **sharedDisks, sharedDisk[3];
	int sharedDiskCount, j;
	BOOL bFound;
	BOOL bDuallog;
	BOOL bInstalled, bSilent;
	BOOL bRSPVdba, bRSPVdbaInstalled, bRSPJdbcD, bRSPJdbcDInstalled, bRSPJdbcS, bRSPJdbcSInstalled;

	size=sizeof(strRSPFile);
	MsiGetProperty(hInstall, "INGRES_RSP_LOC", strRSPFile, &size);
	if(!strcmp(strRSPFile, "0"))
		return ERROR_SUCCESS;

	if(GetFileAttributes(strRSPFile)==-1)
	{
		sprintf(lpText, "Ingres response file %s doesn't exist.", strRSPFile);
		MyMessageBox(hInstall, lpText);
		return ERROR_INSTALL_FAILURE;
	}

	/* II_INSTALLATION, II_SYSTEM */
	GetPrivateProfileString("Ingres Configuration", "installationcode", "", strInstallCode, sizeof(strInstallCode), strRSPFile);
	if(strlen(strInstallCode)==0)
		GetPrivateProfileString("Ingres Configuration", "II_INSTALLATION", "", strInstallCode, sizeof(strInstallCode), strRSPFile);
	if (strlen(strInstallCode)==0)
		GetPrivateProfileString("User Defined Properties", "II_INSTALLATION", "", strInstallCode, sizeof(strInstallCode), strRSPFile);
	if(strlen(strInstallCode)==2 && isalpha(strInstallCode[0]) && isalnum(strInstallCode[1]))
		MsiSetProperty(hInstall, "II_INSTALLATION", strInstallCode);
	
	
	
	if(!GetInstallPath(strInstallCode, strInstallPath))
	{
		GetPrivateProfileString("Ingres Configuration", "II_SYSTEM", "", strInstallPath, sizeof(strInstallPath), strRSPFile);
		if(strlen(strInstallPath)==0)
			GetPrivateProfileString("Ingres Locations", "II_SYSTEM", "", strInstallPath, sizeof(strInstallPath), strRSPFile);
		if (strlen(strInstallPath)==0)
			GetPrivateProfileString("User Defined Properties", "II_SYSTEM", "", strInstallPath, sizeof(strInstallPath), strRSPFile);
	}
	
	/* General */
	MsiSetProperty(hInstall, "INGRES_EMBED_DOC", "0");
	GetPrivateProfileString("Ingres Configuration", "II_EMBED_DOC", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (!lpReturnedString[0])
		GetPrivateProfileString("User Defined Properties", "II_EMBED_DOC", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetFeatureState(hInstall, "IngresDoc", INSTALLSTATE_LOCAL);//MsiSetProperty(hInstall, "INGRES_EMBED_DOC", "1"); 

	MsiSetProperty(hInstall, "INGRES_LANGPACK", "0");
	GetPrivateProfileString("Ingres Configuration", "II_LANGPACK", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (!lpReturnedString[0])
		GetPrivateProfileString("User Defined Properties", "II_LANGPACK", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	{
		bLangPack=1;
		MsiSetProperty(hInstall, "INGRES_LANGPACK", "1");
	}

	MsiSetProperty(hInstall, "INGRES_LANGUAGE", "ENGLISH");
	GetPrivateProfileString("Ingres Configuration", "language", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(!lpReturnedString[0])
		GetPrivateProfileString("User Defined Properties", "language", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && IsValidLang(lpReturnedString))
		MsiSetProperty(hInstall, "INGRES_LANGUAGE", _strupr(lpReturnedString));
	GetPrivateProfileString("Ingres Configuration", "II_LANGUAGE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (!lpReturnedString[0])
		GetPrivateProfileString("User Defined Properties", "II_LANGUAGE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && IsValidLang(lpReturnedString))
		MsiSetProperty(hInstall, "INGRES_LANGUAGE", _strupr(lpReturnedString));
	
	if (!bLangPack)
		MsiSetProperty(hInstall, "INGRES_LANGUAGE", "ENGLISH");
	
	GetPrivateProfileString("Ingres Configuration", "timezone", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_TIMEZONE_NAME", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_TIMEZONE_NAME", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && _stricmp(lpReturnedString, "default") && VerifyIngTimezone(lpReturnedString))
		MsiSetProperty(hInstall, "INGRES_TIMEZONE", lpReturnedString);
	
	GetPrivateProfileString("Ingres Configuration", "terminal", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_TERMINAL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_TERMINAL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && _stricmp(lpReturnedString, "default") && VerifyIngTerminal(lpReturnedString))
		MsiSetProperty(hInstall, "INGRES_TERMINAL", lpReturnedString);
	
	GetPrivateProfileString("Ingres Configuration", "charset", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_CHARSET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_CHARSET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && _stricmp(lpReturnedString, "default") && VerifyIngCharset(lpReturnedString))
		MsiSetProperty(hInstall, "INGRES_CHARSET", lpReturnedString);

	GetPrivateProfileString("Ingres Configuration", "dateformat", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_DATE_FORMAT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_DATE_FORMAT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyIngDateFormat(lpReturnedString))
		MsiSetProperty(hInstall, "INGRES_DATE_FORMAT", lpReturnedString);

	GetPrivateProfileString("Ingres Configuration", "II_DATE_TYPE_ALIAS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (!lpReturnedString[0])
		GetPrivateProfileString("User Defined Properties", "II_DATE_TYPE_ALIAS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (lpReturnedString[0] && _stricmp(lpReturnedString, "ANSI")==0)
		MsiSetProperty(hInstall, "INGRES_DATE_TYPE", lpReturnedString);
	if (lpReturnedString[0] && _stricmp(lpReturnedString, "ANSIDATE")==0)
		MsiSetProperty(hInstall, "INGRES_DATE_TYPE", "ANSI");

	GetPrivateProfileString("Ingres Configuration", "moneyformat", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_MONEY_FORMAT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_MONEY_FORMAT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyIngMoneyFormat(lpReturnedString))
		MsiSetProperty(hInstall, "INGRES_MONEY_FORMAT", lpReturnedString);

	MsiSetProperty(hInstall, "INGRES_DECIMAL", ".");
	GetPrivateProfileString("Ingres Configuration", "II_DECIMAL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (!lpReturnedString[0])
		GetPrivateProfileString("User Defined Properties", "II_DECIMAL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && (!strcmp(lpReturnedString, ".") || !strcmp(lpReturnedString, ",")))
		MsiSetProperty(hInstall, "INGRES_DECIMAL", lpReturnedString);

	/* DBMS */
    MsiSetProperty(hInstall, "INGRES_CLUSTER_INSTALL", "");
    GetPrivateProfileString("Ingres Configuration", "II_CLUSTER_INSTALL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(!lpReturnedString[0])
	GetPrivateProfileString("User Defined Properties", "II_CLUSTER_INSTALL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES") && CheckServiceExists("ClusSvc"))
	MsiSetProperty(hInstall, "INGRES_CLUSTER_INSTALL", "1");

    sprintf(strBuf, "Ingres Service [ %s ]", strInstallCode);
    MsiSetProperty(hInstall, "INGRES_CLUSTER_RESOURCE", strBuf);
    GetPrivateProfileString("Ingres Configuration", "II_CLUSTER_RESOURCE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if (!lpReturnedString[0])
	GetPrivateProfileString("User Defined Properties", "II_CLUSTER_RESOURCE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0])
	MsiSetProperty(hInstall, "INGRES_CLUSTER_RESOURCE", lpReturnedString);

    GetPrivateProfileString("Ingres Configuration", "II_MDB_INSTALL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if (!lpReturnedString[0])
	GetPrivateProfileString("User Defined Properties", "II_MDB_INSTALL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	MsiSetProperty(hInstall, "INGRES_MDB_INSTALL", "1");
    else
	MsiSetProperty(hInstall, "INGRES_MDB_INSTALL", "0");

    GetPrivateProfileString("Ingres Configuration", "II_MDB_NAME", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if (!lpReturnedString[0])
	GetPrivateProfileString("User Defined Properties", "II_MDB_NAME", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0])
	MsiSetProperty(hInstall, "INGRES_MDB_NAME", lpReturnedString);

    GetPrivateProfileString("Ingres Configuration", "II_MDB_SIZE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if (!lpReturnedString[0])
	GetPrivateProfileString("User Defined Properties", "II_MDB_SIZE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0] && IsValidMdbSize(lpReturnedString))
	MsiSetProperty(hInstall, "INGRES_MDB_SIZE", lpReturnedString);

    GetPrivateProfileString("Ingres Configuration", "II_MDB_DEBUG", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if (!lpReturnedString[0])
	GetPrivateProfileString("User Defined Properties", "II_MDB_DEBUG", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	MsiSetProperty(hInstall, "INGRES_MDB_DEBUG", "1");
    else
	MsiSetProperty(hInstall, "INGRES_MDB_DEBUG", "0");

	sharedDisks = (char**)malloc(sizeof(char*) * 256);
	sharedDisks[0] = (char*)malloc(sizeof(char) * 256 * 3);

	sharedDiskCount=0;
	GetPrivateProfileString("Ingres Configuration", "iidatabasedir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Locations", "II_DATABASE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_DATABASE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyPath(lpReturnedString))
	{
		MsiSetProperty(hInstall, "INGRES_DATADIR", lpReturnedString);

		strncpy(sharedDisk, lpReturnedString, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
	}
	else
		MsiSetProperty(hInstall, "INGRES_DATADIR", strInstallPath);
	
	GetPrivateProfileString("Ingres Configuration", "iicheckpointdir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Locations", "II_CHECKPOINT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_CHECKPOINT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyPath(lpReturnedString))
	{
		MsiSetProperty(hInstall, "INGRES_CKPDIR", lpReturnedString);

		strncpy(sharedDisk, lpReturnedString, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
	}
	else
		MsiSetProperty(hInstall, "INGRES_CKPDIR", strInstallPath);
	
	GetPrivateProfileString("Ingres Configuration", "iijournaldir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Locations", "II_JOURNAL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_JOURNAL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyPath(lpReturnedString))
	{
		MsiSetProperty(hInstall, "INGRES_JNLDIR", lpReturnedString);

		strncpy(sharedDisk, lpReturnedString, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
	}
	else
		MsiSetProperty(hInstall, "INGRES_JNLDIR", strInstallPath);
	
	GetPrivateProfileString("Ingres Configuration", "iidumpdir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Locations", "II_DUMP", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_DUMP", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyPath(lpReturnedString))
	{
		MsiSetProperty(hInstall, "INGRES_DMPDIR", lpReturnedString);

		strncpy(sharedDisk, lpReturnedString, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
	}
	else
		MsiSetProperty(hInstall, "INGRES_DMPDIR", strInstallPath);
	
	GetPrivateProfileString("Ingres Configuration", "iiworkdir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Locations", "II_WORK", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_WORK", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyPath(lpReturnedString))
	{
		MsiSetProperty(hInstall, "INGRES_WORKDIR", lpReturnedString);

		strncpy(sharedDisk, lpReturnedString, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
	}
	else
		MsiSetProperty(hInstall, "INGRES_WORKDIR", strInstallPath);
	
	GetPrivateProfileString("Ingres Configuration", "logfilesize", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_LOG_FILE_SIZE_MB", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_LOG_FILE_SIZE_MB", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(atof(lpReturnedString)>=16)
		MsiSetProperty(hInstall, "INGRES_LOGFILESIZE", lpReturnedString);
	
	GetPrivateProfileString("Ingres Configuration", "primarylogdir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Locations", "II_LOG_FILE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_LOG_FILE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && VerifyPath(lpReturnedString))
	{
		MsiSetProperty(hInstall, "INGRES_PRIMLOGDIR", lpReturnedString);

		strncpy(sharedDisk, lpReturnedString, 2);
		sharedDisk[2]='\0';

		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
	}
	else
		MsiSetProperty(hInstall, "INGRES_PRIMLOGDIR", strInstallPath);
	
	bDuallog=FALSE;
	GetPrivateProfileString("Ingres Configuration", "enableduallogging", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	{
		bDuallog=TRUE;
		MsiSetProperty(hInstall, "INGRES_ENABLEDUALLOG", "TRUE");
		GetPrivateProfileString("Ingres Configuration", "duallogdir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	}
	else if (strlen(lpReturnedString)==0)
	{
		GetPrivateProfileString("Ingres Locations", "II_DUAL_LOG", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if (!lpReturnedString[0])
			GetPrivateProfileString("User Defined Properties", "II_DUAL_LOG", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && strlen(lpReturnedString)!=0)
		{
			bDuallog=TRUE;
			MsiSetProperty(hInstall, "INGRES_ENABLEDUALLOG", "TRUE");
		}
		else
			MsiSetProperty(hInstall, "INGRES_ENABLEDUALLOG", "");
	}
	else
	{
		MsiSetProperty(hInstall, "INGRES_ENABLEDUALLOG", "");
		lpReturnedString[0] = '\0';
	}


	if(lpReturnedString[0] && VerifyPath(lpReturnedString))
	{
		MsiSetProperty(hInstall, "INGRES_DUALLOGDIR", lpReturnedString);
		if (bDuallog)
		{
			strncpy(sharedDisk, lpReturnedString, 2);
			sharedDisk[2]='\0';

			bFound=FALSE;
			for (j=0; j< sharedDiskCount; j++)
			{
				if (!_stricmp(sharedDisks[j], sharedDisk))
					bFound=TRUE;
			}
			if (!bFound){
				sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
				sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
				sharedDiskCount++;
			}
		}
	}
	else
		MsiSetProperty(hInstall, "INGRES_DUALLOGDIR", strInstallPath);

	nret=sizeof(strBuf);
	memset(strBuf, 0, nret);
	if (!MsiGetProperty(hInstall, "INGRES_CLUSTER_INSTALL", strBuf, &nret) && !strcmp(strBuf, "1") 
		&& CheckSharedDiskInSameRG(sharedDisks, sharedDiskCount)!=HA_SUCCESS)
	{
		MyMessageBox(hInstall, "Not all disks are in the same Resource Group!");
		return ERROR_INSTALL_FAILURE;
	}
	free ((*sharedDisks));
	
	GetPrivateProfileString("Ingres Configuration", "enablesql92", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_ENABLE_SQL92", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_ENABLE_SQL92", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_ENABLESQL92", "TRUE");
	else
		MsiSetProperty(hInstall, "INGRES_ENABLESQL92", "");
	
	/* ICE */
	GetPrivateProfileString("Ingres Configuration", "iihttpserver", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && GetFileAttributes(lpReturnedString)!=-1)
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER", lpReturnedString);
	else
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER", strInstallPath);

	GetPrivateProfileString("Ingres Configuration", "iihttpserverdir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && GetFileAttributes(lpReturnedString)!=-1)
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", lpReturnedString);
	else
		MsiSetProperty(hInstall, "INGRES_HTTP_SERVER_PATH", strInstallPath);
	
	GetPrivateProfileString("Ingres Configuration", "installcoldfusionsupport", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_COLDFUSION", "TRUE");
	else
		MsiSetProperty(hInstall, "INGRES_COLDFUSION", "");
	
	GetPrivateProfileString("Ingres Configuration", "iicoldfusiondir", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && GetFileAttributes(lpReturnedString)!=-1)
		MsiSetProperty(hInstall, "INGRES_COLDFUSION_PATH", lpReturnedString);
	else
		MsiSetProperty(hInstall, "INGRES_COLDFUSION_PATH", strInstallPath);
	
	GetPrivateProfileString("Ingres Configuration", "serviceauto", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_SERVICE_START_AUTO", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_SERVICE_START_AUTO", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_SERVICEAUTO", "TRUE");
	else
		MsiSetProperty(hInstall, "INGRES_SERVICEAUTO", "");

	MsiSetProperty(hInstall, "INGRES_SERVICELOGINID", "");
	GetPrivateProfileString("Ingres Configuration", "serviceloginid", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_SERVICE_START_USER", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_SERVICE_START_USER", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0])
		MsiSetProperty(hInstall, "INGRES_SERVICELOGINID", lpReturnedString);

	MsiSetProperty(hInstall, "INGRES_SERVICEPASSWORD", "");
	GetPrivateProfileString("Ingres Configuration", "servicepassword", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_SERVICE_START_USERPASSWORD", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_SERVICE_START_USERPASSWORD", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0])
		MsiSetProperty(hInstall, "INGRES_SERVICEPASSWORD", lpReturnedString);

	/* NET */
	GetPrivateProfileString("Ingres Configuration", "enableTCPIP", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_ENABLE_TCPIP", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_ENABLE_TCPIP", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_TCPIP", "TRUE");
	else
		MsiSetProperty(hInstall, "INGRES_TCPIP", "FALSE");

	GetPrivateProfileString("Ingres Configuration", "enableNETBIOS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_ENABLE_NETBIOS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_ENABLE_NETBIOS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_NETBIOS", "TRUE");
	else
		MsiSetProperty(hInstall, "INGRES_NETBIOS", "FALSE");

	GetPrivateProfileString("Ingres Configuration", "enableSPX", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_ENABLE_SPX", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_ENABLE_SPX", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_SPXIPX", "TRUE");
	else
		MsiSetProperty(hInstall, "INGRES_SPXIPX", "FALSE");

	GetPrivateProfileString("Ingres Configuration", "enableDECNET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_DECNET", "TRUE");
	else
		MsiSetProperty(hInstall, "INGRES_DECNET", "FALSE");

	/* MIX */
    GetPrivateProfileString("Ingres Configuration", "ingresprestartuprunprg", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0])
	MsiSetProperty(hInstall, "INGRES_PRESTARTUPRUNPRG", lpReturnedString);

    GetPrivateProfileString("Ingres Configuration", "ingresprestartuprunparams", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0])
	MsiSetProperty(hInstall, "INGRES_PRESTARTUPRUNPARAMS", lpReturnedString);

    GetPrivateProfileString("Ingres Configuration", "noaddremoveprogram", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_ADD_REMOVE_PROGRAMS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_ADD_REMOVE_PROGRAMS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
    {
	MsiSetProperty(hInstall, "INGRES_NO_ARP", "1");
	MsiSetProperty(hInstall, "INGRES_REMOVE_CAB", "1");
    }
    else 
	MsiSetProperty(hInstall, "INGRES_NO_ARP", "0");

    GetPrivateProfileString("Ingres Configuration", "removecab", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	MsiSetProperty(hInstall, "INGRES_REMOVE_CAB", "1");
    if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
	MsiSetProperty(hInstall, "INGRES_REMOVE_CAB", "0");

	GetPrivateProfileString("Ingres Configuration", "createicons", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_INSTALL_ALL_ICONS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_INSTALL_ALL_ICONS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
		MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "0");

    GetPrivateProfileString("Ingres Configuration", "allusersshortcuts", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(!IsWindows95() && lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
	MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");

	GetPrivateProfileString("Ingres Configuration", "openroad256colors", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_OR_256_COLORS", "TRUE");
	
	MsiSetProperty(hInstall, "INGRES_UPGRADEUSERDBS", "FALSE");
	GetPrivateProfileString("Ingres Configuration", "upgradeuserdatabases", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_UPGRADE_USER_DB", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_UPGRADE_USER_DB", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_UPGRADEUSERDBS", "TRUE");
	
	GetPrivateProfileString("Ingres Configuration", "destroytxlogfile", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_DESTROY_TXLOG", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_DESTROY_TXLOG", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_DESTROYTXLOGFILE", "TRUE");
	
	GetPrivateProfileString("Ingres Configuration", "removeoldfiles", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_REMOVEOLDFILES", "TRUE");
	
	GetPrivateProfileString("Ingres Configuration", "checkpointdatabases", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_CKPDBS", "TRUE");

	GetPrivateProfileString("Ingres Configuration", "leaveingresrunninginstallincomplete", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_START_INGRES_ON_COMPLETE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_START_INGRES_ON_COMPLETE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_LEAVE_RUNNING", "1");
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
		MsiSetProperty(hInstall, "INGRES_LEAVE_RUNNING", "0");

	GetPrivateProfileString("Ingres Configuration", "startivm", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_START_IVM_ON_COMPLETE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_START_IVM_ON_COMPLETE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
		MsiSetProperty(hInstall, "INGRES_START_IVM", "0");

	GetPrivateProfileString("Ingres Configuration", "installcalicensing", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
		MsiSetProperty(hInstall, "INGRES_INSTALLCALICENSING", "0");

	GetPrivateProfileString("Ingres Components", "II_COMPONENT_DOTNET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString) == 0)
		GetPrivateProfileString("Ingres Configuration", "Ingres .NET Data Provider", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString) == 0)
		GetPrivateProfileString("User Defined Properties", "Ingres .NET Data Provider", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (_stricmp(lpReturnedString, "YES") == 0)
	{
		GetPrivateProfileString("Ingres Components", "II_COMPONENT_DBMS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if (strlen(lpReturnedString) == 0)
			GetPrivateProfileString("Ingres Configuration", "Ingres DBMS Server", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if (_stricmp(lpReturnedString, "YES") == 0)
			MsiSetProperty(hInstall, "INGRES_DBMS_DOTNET", "TRUE");
	}


	/* Demo DB */
    GetPrivateProfileString("Ingres Configuration", "II_DEMODB_CREATE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(!lpReturnedString[0])
	GetPrivateProfileString("User Defined Properties", "II_DEMODB_CREATE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_INSTALLDEMODB", "1");
    else
		MsiSetProperty(hInstall, "INGRES_INSTALLDEMODB", "");

	/* Include Ingres into PATH */
    GetPrivateProfileString("Ingres Configuration", "II_ADD_TO_PATH", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(!lpReturnedString[0])
        GetPrivateProfileString("User Defined Properties", "II_ADD_TO_PATH", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
    if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_INCLUDEINPATH", "1");
	else if(strlen(lpReturnedString)==0)
		MsiSetProperty(hInstall, "INGRES_INCLUDEINPATH", "1");
    else
		MsiSetProperty(hInstall, "INGRES_INCLUDEINPATH", "");

 

	/* ODBC */
	GetPrivateProfileString("Ingres Configuration", "setup_ingresodbc", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_SETUP_ODBC", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_SETUP_ODBC", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
		MsiSetProperty(hInstall, "INGRES_ODBC", "0");
	
	GetPrivateProfileString("Ingres Configuration", "setup_ingresodbcreadonly", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Configuration", "II_SETUP_ODBC_READONLY", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_SETUP_ODBC_READONLY", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
		MsiSetProperty(hInstall, "INGRES_ODBC_READONLY", "0");
	else if (lpReturnedString[1] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_ODBC_READONLY", "1");

	GetPrivateProfileString("Ingres Configuration", "remove_ingresodbc", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_ODBC_REMOVE", "1");
	
	GetPrivateProfileString("Ingres Configuration", "remove_ingresodbcreadonly", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		MsiSetProperty(hInstall, "INGRES_ODBC_READONLY_REMOVE", "1");

	/* VDBA */
	bRSPVdba=0;
	GetPrivateProfileString("Ingres Configuration", "Ingres Visual DBA", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
	{
		GetPrivateProfileString("Ingres Components", "II_COMPONENT_DBMS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(!lpReturnedString[0])
		     GetPrivateProfileString("User Defined Properties", "II_COMPONENT_DBMS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NO"))
		{
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_NET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
			if(!lpReturnedString[0])
			     GetPrivateProfileString("User Defined Properties", "II_COMPONENT_NET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);	
		}
		
	}
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	{
		bRSPVdba=1;
		MsiSetProperty(hInstall, "INGRES_RSP_VDBA", "1");
	}

	GetPrivateProfileString("Ingres Configuration", "ivmflag", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "NOMAXEVENTSREACHEDWARNING"))
	{
		sprintf(strBuf, "-%s", lpReturnedString);
		MsiSetProperty(hInstall, "INGRES_IVMFLAG", strBuf);
	}

	/* JDBC */
	bRSPJdbcD=0;
	GetPrivateProfileString("Ingres Configuration", "JDBC Driver", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Components", "II_COMPONENT_JDBC_CLIENT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_COMPONENT_JDBC_CLIENT", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	{
		bRSPJdbcD=1;
		MsiSetProperty(hInstall, "INGRES_RSP_JDBCD", "1");
	}

	bRSPJdbcS=0;
	GetPrivateProfileString("Ingres Configuration", "JDBC Server", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString)==0)
		GetPrivateProfileString("Ingres Components", "II_COMPONENT_NET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (strlen(lpReturnedString)==0)
		GetPrivateProfileString("User Defined Properties", "II_COMPONENT_NET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
	{
		bRSPJdbcS=1;
		MsiSetProperty(hInstall, "INGRES_RSP_JDBCS", "1");
	}
	/* Config Type */
	GetPrivateProfileString("Ingres Configuration", "II_CONFIG_TYPE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
	if (!lpReturnedString[0])
	     GetPrivateProfileString("User Defined Properties", "II_CONFIG_TYPE", "", lpReturnedString, 
		sizeof(lpReturnedString), strRSPFile);
	if (lpReturnedString[0] && !_stricmp(lpReturnedString, "BI"))
		MsiSetProperty(hInstall, "CONFIG_TYPE", "2");
	else if (lpReturnedString[0] && !_stricmp(lpReturnedString, "CM"))
		MsiSetProperty(hInstall, "CONFIG_TYPE", "3");
	else if (lpReturnedString[0] && !_stricmp(lpReturnedString, "NULL"))
		MsiSetProperty(hInstall, "CONFIG_TYPE", "1");
	else if (lpReturnedString[0] && !_stricmp(lpReturnedString, "TXN"))
		MsiSetProperty(hInstall, "CONFIG_TYPE", "0");
	else
		MsiSetProperty(hInstall, "CONFIG_TYPE", "1");
		

	/* REINSTALL */
	bInstalled=0;
	nret=sizeof(strBuf); *strBuf=0;
	if (!MsiGetProperty(hInstall, "Installed", strBuf, &nret) && strBuf[0])
		bInstalled=1;

	bSilent=0;
	nret=sizeof(strBuf); *strBuf=0;
	if (!MsiGetProperty(hInstall, "UILevel", strBuf, &nret) && !_stricmp(strBuf, "2"))
		bSilent=1;

	bRSPVdbaInstalled=0;
	nret=sizeof(strBuf); *strBuf=0;
	if (!MsiGetProperty(hInstall, "INGRES_VDBA_SILENT_INSTALLED", strBuf, &nret) && !_stricmp(strBuf, "1"))
		bRSPVdbaInstalled=1;

	bRSPJdbcDInstalled=0;
	nret=sizeof(strBuf); *strBuf=0;
	if (!MsiGetProperty(hInstall, "INGRES_JDBCD_SILENT_INSTALLED", strBuf, &nret) && !_stricmp(strBuf, "1"))
		bRSPJdbcDInstalled=1;

	bRSPJdbcSInstalled=0;
	nret=sizeof(strBuf); *strBuf=0;
	if (!MsiGetProperty(hInstall, "INGRES_JDBCS_SILENT_INSTALLED", strBuf, &nret) && !_stricmp(strBuf, "1"))
		bRSPJdbcSInstalled=1;

	if (bInstalled && bSilent 
	   && ((bRSPVdba && !bRSPVdbaInstalled) || (bRSPJdbcD && !bRSPJdbcDInstalled) || (bRSPJdbcS && !bRSPJdbcSInstalled)))
		MsiSetProperty(hInstall, "REINSTALL", "IngresDBMS,IngresNet");

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_base_settings
**
**  Description:
**	This function is used to decide the installation path and feature selection (ONLY
**	used in slient mode installation).
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	21-May-2001 (penga03)
**	    Created.
**	14-Jun-2001 (penga03)
**	    Changed Ingres/Star to be a subfeature of Ingres DBMS.
**	14-Jun-2001 (penga03)
**	    Added IngresEsqlFortran as a subfeature of IngresEmeddedPrecompilers.
**	23-July-2001 (penga03)
**	    For fresh install in silent mode, do not set the feature state to be 
**	    INSTALLSTATE_ABSENT if user does not set the feature to be installed 
**	    in response file, because INSTALLSTATE_ABSENT means the feature was 
**	    already installed, and is going to be removed.
**	30-Aug-2001 (penga03)
**	    Changed Ingres/Replicator to be a subfeature of Ingres DBMS.
**	16-Nov-2006 (drivi01) and (karye01)
**	    Enhanced response file processing to handle new version of
**	    response file.
**	06-Feb-2007 (drivi01)
**	    BUG 117635
**	    Added "User Defined Properties" section to the response file.
**	    Any property can be now moved to "User Defined Properties" section
**	    and it will be properly processed by the installer.
**	15-Feb-2007 (drivi01)
**	    Add Ice to a new style response file. ICE is being deprecated, adding
**	    it response file as it will be the only way to install it now.
**	12-Aug-2009 (drivi01)
**	    Add Spatial Objects component to the response file.  The new 
** 	    component name in the response file is II_COMPONENT_SPATIAL.
*/

UINT __stdcall 
ingres_base_settings(MSIHANDLE hInstall) 
{
	char strRSPFile[MAX_PATH+1], lpText[512], lpReturnedString[MAX_PATH+1];
	char strInstallPath[MAX_PATH+1], strInstallCode[3];
	DWORD size;
	INSTALLSTATE iInstalled, iAction;
	int dbms, net;
	UINT uiRet;
	char strBuf[512];
	BOOL bInstalled;
	char szAddLocal[1024]; *szAddLocal=0;

	/* Get response file location */
	size=sizeof(strRSPFile);
	MsiGetProperty(hInstall, "INGRES_RSP_LOC", strRSPFile, &size);
	if(!strcmp(strRSPFile, "0"))
		return ERROR_SUCCESS;
	
	if(GetFileAttributes(strRSPFile)==-1)
	{
		sprintf(lpText, "Ingres response file %s doesn't exist.", strRSPFile);
		MyMessageBox(hInstall, lpText);
		return ERROR_INSTALL_FAILURE;
	}
	
	/* Set II_INSTALLATIONCODE and INGRESFOLDER */
	size=sizeof(strInstallCode);
	MsiGetProperty(hInstall, "II_INSTALLATION", strInstallCode, &size);
	
	bInstalled=0;
	if (GetInstallPath(strInstallCode, strInstallPath))
	{
		sprintf(strBuf, "%s\\ingres\\files\\config.dat", strInstallPath);
		if (GetFileAttributes(strBuf) != -1)
			bInstalled=1;
	}
	if(!bInstalled )
	{
		GetPrivateProfileString("Ingres Configuration", "II_SYSTEM", "", strInstallPath, sizeof(strInstallPath), strRSPFile);
		if(strlen(strInstallPath)==0)
			GetPrivateProfileString("Ingres Locations", "II_SYSTEM", "", strInstallPath, sizeof(strInstallPath), strRSPFile);
		if(strlen(strInstallPath)==0)
			GetPrivateProfileString("User Defined Properties", "II_SYSTEM", "", strInstallPath, sizeof(strInstallPath), strRSPFile);
	}

	size=sizeof(strBuf); *strBuf=0;
	if (MsiGetProperty(hInstall, "Installed", strBuf, &size) || !strBuf[0])
	{
		strcpy(szAddLocal, "IngresBlankFeature");

		uiRet=MsiSetTargetPath(hInstall, "INGRESFOLDER", strInstallPath);
		if (uiRet)
		{
			sprintf(lpText, "Failed to set II_SYSTEM to \'%s\'. MsiSetTargetPath returns: %d.", strInstallPath, uiRet);
			MyMessageBox(hInstall, lpText);
			return ERROR_INSTALL_FAILURE;
		}
	}

	/* Set Ingres features */
	MsiSetInstallLevel(hInstall, 1);
	/* Ingres DBMS */
	dbms=0;
	MsiGetFeatureState(hInstall, "IngresDBMS", &iInstalled, &iAction);
	if(iInstalled==INSTALLSTATE_LOCAL)
		dbms=1;
	else
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres DBMS Server", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_DBMS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_DBMS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			dbms=1;
			MsiSetFeatureState(hInstall, "IngresDBMS", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresDBMS");
			else if (!strstr(szAddLocal, "IngresDBMS"))
				strcat(szAddLocal, ",IngresDBMS");
		}
	}

	/* Spatial Objects */
	MsiGetFeatureState(hInstall, "IngresSpat", &iInstalled, &iAction);
	if (iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Components", "II_COMPONENT_SPATIAL", "", lpReturnedString, sizeof(lpReturnedString),
		strRSPFile);
		if (strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_SPATIAL", "", lpReturnedString, 						sizeof(lpReturnedString),strRSPFile);
		if (lpReturnedString[0] && !stricmp(lpReturnedString, "YES"))
		{
			dbms=1;
			MsiSetFeatureState(hInstall, "IngresSpat", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresDBMS, IngresSpat");
			else if (!strstr(szAddLocal, "IngresDBMS"))
				strcat(szAddLocal, ",IngresDBMS");
			else if (!strstr(szAddLocal, "IngresSpat"))
				strcat(szAddLocal, ",IngresSpat");
		}

	}

	/* Ingres/ICE */
	MsiGetFeatureState(hInstall, "IngresIce", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres/ICE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if (strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_ICE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if (strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_ICE", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			dbms=1;
			MsiSetFeatureState(hInstall, "IngresIce", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresDBMS, IngresIce");
			else if (!strstr(szAddLocal, "IngresDBMS"))
				strcat(szAddLocal, ",IngresDBMS");
			else if (!strstr(szAddLocal, "IngresIce"))
				strcat(szAddLocal, ",IngresIce");
		}
	}

	/* Ingres/Replicator */
	MsiGetFeatureState(hInstall, "IngresReplicator", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)			
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres/Replicator", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_REPLICATOR", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_REPLICATOR", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			dbms=1;
			MsiSetFeatureState(hInstall, "IngresReplicator", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresDBMS, IngresReplicator");
			else if (!strstr(szAddLocal, "IngresDBMS"))
				strcat(szAddLocal, ",IngresDBMS");
			else if (!strstr(szAddLocal, "IngresReplicator"))
				strcat(szAddLocal, ",IngresReplicator");
		}
	}

	/* Ingres/Star */
	MsiGetFeatureState(hInstall, "IngresStar", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres/Star", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_STAR", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_STAR", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			dbms=1;
			MsiSetFeatureState(hInstall, "IngresStar", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresDBMS, IngresStar");
			else if (!strstr(szAddLocal, "IngresDBMS"))
				strcat(szAddLocal, ",IngresDBMS");
			else if (!strstr(szAddLocal, "IngresStar"))
				strcat(szAddLocal, ",IngresStar");
		}
	}
	
	/* Ingres/Net */
	net=0;
	MsiGetFeatureState(hInstall, "IngresNet", &iInstalled, &iAction);
	if(iInstalled==INSTALLSTATE_LOCAL)
		net=1;
	else
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres/Net", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_NET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_NET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			net=1;
			MsiSetFeatureState(hInstall, "IngresNet", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresNet");
			else if (!strstr(szAddLocal, "IngresNet"))
				strcat(szAddLocal, ",IngresNet");
		}
	}
	
	MsiGetFeatureState(hInstall, "IngresTCPIP", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "enableTCPIP", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			net=1;
			MsiSetFeatureState(hInstall, "IngresTCPIP", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresNet,IngresTCPIP");
			else if (!strstr(szAddLocal, "IngresNet"))
				strcat(szAddLocal, ",IngresNet");
			else if (!strstr(szAddLocal, "IngresTCPIP"))
				strcat(szAddLocal, ",IngresTCPIP");
		}
	}
	
	MsiGetFeatureState(hInstall, "IngresNetBIOS", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "enableNETBIOS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			net=1;
			MsiSetFeatureState(hInstall, "IngresNetBIOS", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresNet,IngresNetBIOS");
			else if (!strstr(szAddLocal, "IngresNet"))
				strcat(szAddLocal, ",IngresNet");
			else if (!strstr(szAddLocal, "IngresNetBIOS"))
				strcat(szAddLocal, ",IngresNetBIOS");
		}
	}
	
	MsiGetFeatureState(hInstall, "IngresNovellSPXIPX", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "enableSPX", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			net=1;
			MsiSetFeatureState(hInstall, "IngresNovellSPXIPX", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresNet,IngresNovellSPXIPX");
			else if (!strstr(szAddLocal, "IngresNet"))
				strcat(szAddLocal, ",IngresNet");
			else if (!strstr(szAddLocal, "IngresNovellSPXIPX"))
				strcat(szAddLocal, ",IngresNovellSPXIPX");
		}
	}
	
	MsiGetFeatureState(hInstall, "IngresDECNet", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "enableDECNET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			net=1;
			MsiSetFeatureState(hInstall, "IngresDECNet", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0]) 
				strcpy(szAddLocal, "IngresNet,IngresDECNet");
			else if (!strstr(szAddLocal, "IngresNet"))
				strcat(szAddLocal, ",IngresNet");
			else if (!strstr(szAddLocal, "IngresDECNet"))
				strcat(szAddLocal, ",IngresDECNet");
		}
	}

	/* Ingres Embedded Precompilers */
	MsiGetFeatureState(hInstall, "IngresEsqlC", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres Embedded SQL/C", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			MsiSetFeatureState(hInstall, "IngresEsqlC", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresEsqlC");
			else if (!strstr(szAddLocal, "IngresEsqlC"))
				strcat(szAddLocal, ",IngresEsqlC");
		}
	}
	
	MsiGetFeatureState(hInstall, "IngresEsqlCobol", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres Embedded SQL/COBOL", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			MsiSetFeatureState(hInstall, "IngresEsqlCobol", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresEsqlCobol");
			else if (!strstr(szAddLocal, "IngresEsqlCobol"))
				strcat(szAddLocal, ",IngresEsqlCobol");
		}
	}

	MsiGetFeatureState(hInstall, "IngresEsqlFortran", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres Embedded SQL/FORTRAN", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			MsiSetFeatureState(hInstall, "IngresEsqlFortran", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresEsqlFortran");
			else if (!strstr(szAddLocal, "IngresEsqlFortran"))
				strcat(szAddLocal, ",IngresEsqlFortran");
		}
	}	
	
	/* Ingres Character Based Tools */
	MsiGetFeatureState(hInstall, "IngresTools", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres Querying and Reporting Tools", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			MsiSetFeatureState(hInstall, "IngresTools", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresTools");
			else if (!strstr(szAddLocal, "IngresTools"))
				strcat(szAddLocal, ",IngresTools");
		}
	}
	
	MsiGetFeatureState(hInstall, "IngresVision", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)			
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres/Vision", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_FRONTTOOLS", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			if(!(dbms || net))
			{
				MyMessageBox(hInstall, "You cannot install Ingres Charater Based Tools without Ingres DBMS or Ingres/Net.");
				return ERROR_INSTALL_FAILURE;
			}
			MsiSetFeatureState(hInstall, "IngresVision", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresVision");
			else if (!strstr(szAddLocal, "IngresVision"))
				strcat(szAddLocal, ",IngresVision");
		}
	}

	/* Ingres .NET Data Provider */
	MsiGetFeatureState(hInstall, "IngresDotNETDataProvider", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres .NET Data Provider", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_DOTNET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_DOTNET", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES") && IsDotNETFXInstalled())
		{
			MsiSetFeatureState(hInstall, "IngresDotNETDataProvider", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresDotNETDataProvider");
			else if (!strstr(szAddLocal, "IngresDotNETDataProvider"))
				strcat(szAddLocal, ",IngresDotNETDataProvider");
		}
	}
	
	/* Ingres Documentation */
	MsiGetFeatureState(hInstall, "IngresDoc", &iInstalled, &iAction);
	if(iInstalled!=INSTALLSTATE_LOCAL)
	{
		GetPrivateProfileString("Ingres Configuration", "Ingres Online Documentation", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("Ingres Components", "II_COMPONENT_DOCUMENTATION", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(strlen(lpReturnedString)==0)
			GetPrivateProfileString("User Defined Properties", "II_COMPONENT_DOCUMENTATION", "", lpReturnedString, sizeof(lpReturnedString), strRSPFile);
		if(lpReturnedString[0] && !_stricmp(lpReturnedString, "YES"))
		{
			MsiSetFeatureState(hInstall, "IngresDoc", INSTALLSTATE_LOCAL);
			if (!szAddLocal[0])
				strcpy(szAddLocal, "IngresDoc");
			else if (!strstr(szAddLocal, "IngresDoc"))
				strcat(szAddLocal, ",IngresDoc");
		}
	}

	MsiSetProperty(hInstall, "ADDLOCAL", szAddLocal);
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_set_environment
**
**  Description:
**	Delete the tailing back slash of INGRES_II_SYSTEM.
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
**	13-Jun-2001 (penga03)
**	    Created.
**	10-Sep-2001 (penga03)
**	    Changed the way to delete the tailing back slash of INGRESFOLDER.
**	11-Sep-2001 (penga03)
**	    Set property INGRES_FRESHINSTALL.
**	28-jan-2001 (penga03)
**	    Removed the obsolete property INGRES_FRESHINSTALL.
*/

UINT __stdcall 
ingres_set_environment(MSIHANDLE hInstall) 
{
	char path[MAX_PATH+1];
	DWORD nret;

	nret=sizeof(path);
	MsiGetTargetPath(hInstall, "INGRESFOLDER", path, &nret);
	if(path[strlen(path)-1] == '\\')
		path[strlen(path)-1] = '\0';
	MsiSetProperty(hInstall, "INGRES_II_SYSTEM", path);
	
	return ERROR_SUCCESS;
}

/*
**  Name:ingres_upgrade_ver25
**
**  Description:
**	Upgrade the installation installed by old installer. Keep the old path; 
**	set each feature state according to corresponding package installed or not; 
**	delete the old shortcut and service.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	11-Jun-2001 (penga03)
**	    Created.
**	10-Aug-2001 (penga03)
**	    Set INGRES_VER25_NET to 1 if there exists Ingres/Net installed by 
**	    old installer.
**	30-Aug-2001 (penga03)
**	    Changed Ingres/Replicator to be a subfeature of Ingres DBMS.
**	10-Sep-2001 (penga03)
**	    ONLY Windows NT and Windows 2000 need to remove the service.
**	17-Sep-2001 (penga03)
**	    While upgrading II 2.6/0106, remove the obsolete link, 
**	    Uninstall Ingres.link and the link of Ingres 2.6 Documentation.
**	12-Oct-2001 (penga03)
**	    Made some changes on the way to check if a Ingres component installed 
**	    or not.
**	19-nov-2001 (penga03)
**	    Delete the old Ingres ODBC Driver.
**	30-jan-2002 (penga03)
**	    Commented the part to delete, Ingres service and Ingres ODBC driver 
**	    since they will be done during setup.
**	23-may-2002 (penga03)
**	    Set INGRES_SHORTCUTS to 0 if there are no shortcuts exist for 
**	    the Ingres installation being upgraded;
**	    Set INGRES_ALLUSERS to 0 if the shortcuts were created for  
**	    current user.
**	01-aug-2002 (penga03)
**	    In ingres_upgrade_ver25(), starighten the way we set 
**	    INGRES_SHORTCUTS and INGRES_ALLUSERS, as well as removing the 
**	    obsolete Ingres shortcuts.
**	08-jul-2003 (penga03)
**	    Replaced RemoveDirectory with RemoveDir.
**	08-aug-2003 (penga03)
**	    Enlarge the size of ConfigKeyValue.
*/

UINT __stdcall 
ingres_upgrade_ver25(MSIHANDLE hInstall) 
{
    char buf[MAX_PATH+1], ii_system[MAX_PATH+1];
    DWORD nret;
    LPITEMIDLIST pidlComProg, pidlProg, pidlComStart, pidlStart;
    char szPath[MAX_PATH+1], szPath2[MAX_PATH+1];
    char Host[33], ConfigKey[MAX_PATH+1], ConfigKeyValue[256];
    char code[3];
    char szTemp1[MAX_PATH], szTemp2[MAX_PATH];
    char szTemp3[MAX_PATH], szTemp4[MAX_PATH];
    FILE *fp;
    char filename[MAX_PATH], line[128];
    UINT uiRet;
    char lpText[512];
    BOOL bVer250011, bShortcuts, bAllUsers;
    LPMALLOC pMalloc = NULL;
	
    nret=sizeof(buf);
    MsiGetProperty(hInstall, "INGRES_VER25", buf, &nret);
    if(strcmp(buf, "1"))
	return ERROR_SUCCESS;

    /* Keep the old path */
    nret=sizeof(code);
    MsiGetProperty(hInstall, "II_INSTALLATION", code, &nret);
    if(!GetInstallPath(code, ii_system))
    {
	sprintf(lpText, "Cannot find II_SYSTEM for the instance %s!", code);
	MyMessageBox(hInstall, lpText);
	return ERROR_INSTALL_FAILURE;
    }
    uiRet=MsiSetTargetPath(hInstall, "INGRESFOLDER", ii_system);
    if (uiRet)
    {
	sprintf(lpText, "Failed to set II_SYSTEM to its old path \'%s\'. MsiSetTargetPath returns: %d.", ii_system, uiRet);
	MyMessageBox(hInstall, lpText);
	return ERROR_INSTALL_FAILURE;
    }

    SetEnvironmentVariable("II_SYSTEM", ii_system);
    Local_PMhost(Host);

    /* Find out all installed Ingres features  */
    sprintf(filename, "%s\\CAREGLOG.LOG", ii_system);
    if (fp=fopen(filename, "r"))
    {
	while (fgets(line, 128, fp))
	{
	    if (strstr(line, "[Name]"))
	    {
		if (fgets(line, 128, fp))
		{
		    char *p=line;
			 
		    while (isalnum(*p) || *p==' ' || *p=='/' || *p=='.' || *p=='&' || *p=='-')
			p++;
		    *p='\0';

		    if (!ComponentInstalled(line)) InsertComponent(line);
		}
	    }
	}
    }
    fclose(fp);

    /* Upgrade features */
    MsiSetInstallLevel(hInstall, 1);

    if (ComponentInstalled("Ingres DBMS Server"))
    {
	MsiSetFeatureState(hInstall, "IngresDBMS", INSTALLSTATE_LOCAL);
	MsiSetProperty(hInstall, "INGRES_VER25_DBMS", "1");
    }

    if (ComponentInstalled("Ingres/ICE"))
    {
	MsiSetFeatureState(hInstall, "IngresIce", INSTALLSTATE_LOCAL);
	MsiSetProperty(hInstall, "INGRES_VER25_ICE", "1");
    }

    if (ComponentInstalled("Ingres/Replicator"))
	MsiSetFeatureState(hInstall, "IngresReplicator", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres/Star"))
	MsiSetFeatureState(hInstall, "IngresStar", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres/Net"))
    {
	MsiSetFeatureState(hInstall, "IngresNet", INSTALLSTATE_LOCAL);
	MsiSetProperty(hInstall, "INGRES_VER25_NET", "1");
				
	sprintf(ConfigKey, "ii.%s.gcb.*.wintcp.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)== ERROR_SUCCESS 
	    && !_stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureState(hInstall, "IngresTCPIP", INSTALLSTATE_LOCAL);

	sprintf(ConfigKey, "ii.%s.gcb.*.tcp_ip.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)== ERROR_SUCCESS 
	    && !_stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureState(hInstall, "IngresTCPIP", INSTALLSTATE_LOCAL);

	sprintf(ConfigKey, "ii.%s.gcb.*.lanman.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)== ERROR_SUCCESS 
	    && !_stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureState(hInstall, "IngresNetBIOS", INSTALLSTATE_LOCAL);

	sprintf(ConfigKey, "ii.%s.gcb.*.decnet.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)== ERROR_SUCCESS 
	    && !_stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureState(hInstall, "IngresDECNet", INSTALLSTATE_LOCAL);

	sprintf(ConfigKey, "ii.%s.gcb.*.nvlspx.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)== ERROR_SUCCESS 
	    && !_stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureState(hInstall, "IngresNovellSPXIPX", INSTALLSTATE_LOCAL);
    }
	
    if (ComponentInstalled("Ingres ESQL/COBOL"))
	MsiSetFeatureState(hInstall, "IngresEsqlCobol", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres ESQL/FORTRAN"))
	MsiSetFeatureState(hInstall, "IngresEsqlFortran", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres ESQL/C"))
	MsiSetFeatureState(hInstall, "IngresEsqlC", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres Querying & Reporting Tools"))
	MsiSetFeatureState(hInstall, "IngresTools", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres/Vision"))
	MsiSetFeatureState(hInstall, "IngresVision", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres .NET Data Provider") && IsDotNETFXInstalled())
	MsiSetFeatureState(hInstall, "IngresDotNETDataProvider", INSTALLSTATE_LOCAL);

    if (ComponentInstalled("Ingres II On-line Documentation") 
        || ComponentInstalled("Ingres 2.6 Documentation"))
	MsiSetFeatureState(hInstall, "IngresDoc", INSTALLSTATE_LOCAL);
	
    SHGetMalloc(&pMalloc);
    SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlComProg);
    SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlProg);
    SHGetFolderLocation(NULL, CSIDL_COMMON_STARTUP, NULL, 0, &pidlComStart);
    SHGetFolderLocation(NULL, CSIDL_STARTUP, NULL, 0, &pidlStart);

    /* Setting INGRES_SHORTCUTS */
    _tcslwr(ii_system);
    bShortcuts=0;
    MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "0");

    SHGetPathFromIDList(pidlProg, szPath);
    SHGetPathFromIDList(pidlComProg, szPath2);

    sprintf(szTemp1, "%s\\Computer Associates\\Advantage\\Ingres [ %s ]", szPath, code);
    sprintf(szTemp2, "%s\\Computer Associates\\Advantage\\Ingres [ %s ]", szPath2, code);
    if (GetFileAttributes(szTemp1)!=-1 || GetFileAttributes(szTemp2)!=-1)
    {
	bShortcuts=1;
	MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
    }
    else
    {
	sprintf(szTemp1, "%s\\Ingres II [ %s ]", szPath, code);
	sprintf(szTemp2, "%s\\Ingres II [ %s ]", szPath2, code);
	if (GetFileAttributes(szTemp1)!=-1 || GetFileAttributes(szTemp2)!=-1)
	{
	    bShortcuts=1;
	    MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
	}
	else
	{
	    sprintf(szTemp1, "%s\\Ingres II\\Ingres Service Manager.lnk", szPath);
	    sprintf(szTemp2, "%s\\Ingres II\\Ingres Service Manager.lnk", szPath2);
	    if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system) 
	        || GetLinkInfo(szTemp2, szTemp4) && _tcsstr(szTemp4, ii_system))
	    {
		bShortcuts=1;
		MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
	    }
	    else
	    {
		sprintf(szTemp1, "%s\\Ingres II\\Start Ingres.lnk", szPath);
		sprintf(szTemp2, "%s\\Ingres II\\Start Ingres.lnk", szPath2);
		if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system) 
		    || GetLinkInfo(szTemp2, szTemp4) && _tcsstr(szTemp4, ii_system))
		{
		    bShortcuts=1;
		    MsiSetProperty(hInstall, "INGRES_SHORTCUTS", "1");
		}
	    }
	}
    }

    /* Setting INGRES_ALLUSERS */
    bAllUsers=1;
    MsiSetProperty(hInstall, "INGRES_ALLUSERS", "1");

    SHGetPathFromIDList(pidlProg, szPath);

    sprintf(szTemp1, "%s\\Computer Associates\\Advantage\\Ingres [ %s ]", 
            szPath, code);
    if (GetFileAttributes(szTemp1)!=-1)
    {
	bAllUsers=0;
	MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
    }
    else
    {
	sprintf(szTemp1, "%s\\Ingres II [ %s ]", szPath, code);
	if (GetFileAttributes(szTemp1)!=-1)
	{
	    bAllUsers=0;
	    MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
	}
	else
	{
	    sprintf(szTemp1, "%s\\Ingres II\\Ingres Service Manager.lnk", szPath);
	    if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system))
	    {
		bAllUsers=0;
		MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
	    }
	    else
	    {
		sprintf(szTemp1, "%s\\Ingres II\\Start Ingres.lnk", szPath);
		if (GetLinkInfo(szTemp1, szTemp3) && _tcsstr(szTemp3, ii_system))
		{
		    bAllUsers=0;
		    MsiSetProperty(hInstall, "INGRES_ALLUSERS", "0");
		}
	    }
	}
    }

    if (bShortcuts)
    {
	if (bAllUsers)
	{
	    SHGetPathFromIDList(pidlComProg, szPath);
	    SHGetPathFromIDList(pidlComStart, szPath2);
	}
	else
	{
	    SHGetPathFromIDList(pidlProg, szPath);
	    SHGetPathFromIDList(pidlStart, szPath2);
	}

	/* Save OpenROAD folder. */
	sprintf(szTemp1, 
		"%s\\Computer Associates\\Advantage\\Ingres [ %s ]\\OpenROAD Demos", 
		szPath, code);
	if (GetFileAttributes(szTemp1)!=-1)
	{
	    sprintf(szTemp1, "Computer Associates\\Advantage\\Ingres [ %s ]", code);
	    MsiSetProperty(hInstall, "INGRES_ORFOLDER", szTemp1);
	}
	else
	{
	    sprintf(szTemp1, "%s\\Ingres II [ %s ]\\OpenROAD Demos", szPath, code);
	    if (GetFileAttributes(szTemp1)!=-1)
	    {
		sprintf("Ingres II [ %s ]", code);
		MsiSetProperty(hInstall, "INGRES_ORFOLDER", szTemp1);
	    }
	    else
	    {
		sprintf(szTemp1, "%s\\Ingres II\\OpenROAD Demos", szPath);
		if (GetFileAttributes(szTemp1)!=-1)
		    MsiSetProperty(hInstall, "INGRES_ORFOLDER", "Ingres II");
	    }
	}

	/* Removing the obsolete Ingres shortcuts */
	sprintf(szTemp1, "%s\\Computer Associates\\Ingres [ %s ]", szPath, code);
	if (GetFileAttributes(szTemp1)!=-1)
	{
	    if (RemoveOneDir(szTemp1))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);

	    sprintf(szTemp1, "%s\\Ingres Documentation", szPath);
	    if (RemoveOneDir(szTemp1))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
	}
	else	
	{
	   sprintf(szTemp1, "%s\\Computer Associates\\Advantage\\Ingres [ %s ]", szPath, code);
	   if (GetFileAttributes(szTemp1)!=-1)
	   {
	       if (RemoveOneDir(szTemp1))
		   SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);

	       sprintf(szTemp1, "%s\\Ingres Documentation", szPath);
	       if (RemoveOneDir(szTemp1))
		   SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
       }
	   else
	   {
	    sprintf(szTemp1, "%s\\Ingres II [ %s ]", szPath, code);
	    if (GetFileAttributes(szTemp1)!=-1)
	    {
		if (RemoveOneDir(szTemp1))
		    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
		
		sprintf(szTemp1, "%s\\Ingres 2.6 Documentation", szPath);
		if (RemoveOneDir(szTemp1))
		    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
	    }
	    else
	    {
		sprintf(szTemp1, "%s\\Ingres II", szPath);
		if (RemoveOneDir(szTemp1))
		    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
		
		sprintf(szTemp1, "%s\\Ingres II Online Documentation", szPath);
		if (RemoveOneDir(szTemp1))
		    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
		}
	   }
	}

	sprintf(szTemp1, "%s\\Ingres Visual Manager.lnk", szPath2);
	if (DeleteFile(szTemp1))
	    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szTemp1, 0);

	/* 
	** Fix the bug that has been existed in the installer of 
	** version 2.5/0011. 
	*/
	bVer250011=0;
	wsprintf(ConfigKey, "ii.%s.config.dbms.ii250011intw3200", Host);
	if (!Local_PMget(ConfigKey, ConfigKeyValue) 
	   && !_stricmp(ConfigKeyValue, "COMPLETE"))
	    bVer250011=1;
	else
	{
	    wsprintf(ConfigKey, "ii.%s.config.net.ii250011intw3200", Host);
	    if (!Local_PMget(ConfigKey, ConfigKeyValue) 
	       && !_stricmp(ConfigKeyValue, "COMPLETE"))
		bVer250011=1;
	}
	if (bVer250011)
	{
	    SHGetPathFromIDList(pidlProg, szPath);
	    
	    sprintf(szTemp1, "%s\\Ingres II Online Documentation", szPath);
	    if (RemoveOneDir(szTemp1))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
	}

	/* Refresh the folders we may have touched. */
	SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlComProg, NULL);
	SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlProg, NULL);
	SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlComStart, NULL);
	SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlStart, NULL);

    }//end if (bShortcuts)
	
    if (pidlComProg)
	pMalloc->lpVtbl->Free(pMalloc, pidlComProg);
    if (pidlProg)
	pMalloc->lpVtbl->Free(pMalloc, pidlProg);
    if (pidlComStart)
	pMalloc->lpVtbl->Free(pMalloc, pidlComStart);
    if (pidlStart)
	pMalloc->lpVtbl->Free(pMalloc, pidlStart);

    pMalloc->lpVtbl->Release(pMalloc);
    return ERROR_SUCCESS;
}

/*
**  Name:ingres_set_feature_attribute
**
**  Description:
**	Set feature attributes.
**	
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS
**	    ERROR_INSTALL_FAILURE
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	17-Aug-2001 (penga03)
**	    Created.
**	30-Aug-2001 (penga03)
**	    Changed Ingres/Replicator to be a subfeature of Ingres DBMS.
**	    Changed IngresTools, IngresVision, IngresEslqC, IngresEsqlCob, and 
**	    IngresEsqlFortran to be separate features.
**	12-Oct-2001 (penga03)
**	    Made some changes on the way to check if a Ingres component installed 
**	    or not.
*/
UINT __stdcall 
ingres_set_feature_attribute(MSIHANDLE hInstall) 
{
    char ver25[2];
    DWORD nret;
    char code[3], ii_system[MAX_PATH+1];
    char Host[MAX_COMPUTERNAME_LENGTH+1], ConfigKey[1024], ConfigKeyValue[256];
    BOOL ice, rep, star;
    FILE *fp;
    char filename[MAX_PATH], line[128];
    char lpText[512];

    nret=sizeof(ver25);
    MsiGetProperty(hInstall, "INGRES_VER25", ver25, &nret);
    if(!strcmp(ver25, "0"))
    {
	MsiSetFeatureAttributes(hInstall, "IngresDBMS", 16);
	MsiSetFeatureAttributes(hInstall, "IngresIce", 16);
	MsiSetFeatureAttributes(hInstall, "IngresReplicator", 16);
	MsiSetFeatureAttributes(hInstall, "IngresStar", 16);
	MsiSetFeatureAttributes(hInstall, "IngresNet", 16);
	MsiSetFeatureAttributes(hInstall, "IngresTCPIP", 16);
	MsiSetFeatureAttributes(hInstall, "IngresNetBIOS", 16);
	MsiSetFeatureAttributes(hInstall, "IngresDECNet", 16);
	MsiSetFeatureAttributes(hInstall, "IngresNovellSPXIPX", 16);
	MsiSetFeatureAttributes(hInstall, "IngresEmeddedPrecompilers", 16);
	MsiSetFeatureAttributes(hInstall, "IngresEsqlC", 16);
	MsiSetFeatureAttributes(hInstall, "IngresEsqlCobol", 16);
	MsiSetFeatureAttributes(hInstall, "IngresEsqlFortran", 16);
	MsiSetFeatureAttributes(hInstall, "IngresCharTools", 16);
	MsiSetFeatureAttributes(hInstall, "IngresTools", 16);
	MsiSetFeatureAttributes(hInstall, "IngresVision", 16);
	MsiSetFeatureAttributes(hInstall, "IngresDotNETDataProvider", 16);
	MsiSetFeatureAttributes(hInstall, "IngresDoc", 16);	
	return ERROR_SUCCESS;
    }
	
    nret=sizeof(code);
    MsiGetProperty(hInstall, "II_INSTALLATION", code, &nret);
    if(!GetInstallPath(code, ii_system))
    {
	sprintf(lpText, "Cannot find II_SYSTEM for the instance %s!", code);
	MyMessageBox(hInstall, lpText);
	return ERROR_INSTALL_FAILURE;
    }
    SetEnvironmentVariable("II_SYSTEM", ii_system);
    Local_PMhost(Host);

    /* Find out all installed Ingres features  */
    sprintf(filename, "%s\\CAREGLOG.LOG", ii_system);
    if (fp=fopen(filename, "r"))
    {
	while (fgets(line, 128, fp))
	{
	    if (strstr(line, "[Name]"))
	    {
		if (fgets(line, 128, fp))
		{
		    char *p=line;
			 
		    while (isalnum(*p) || *p==' ' || *p=='/' || *p=='.' || *p=='&' || *p=='-')
			p++;
		    *p='\0';

		    if (!ComponentInstalled(line)) InsertComponent(line);
		}
	    }
	}
    }
    fclose(fp);

    ice=rep=star=TRUE;
    if (!ComponentInstalled("Ingres/ICE"))
    {
	ice=FALSE;
	MsiSetFeatureAttributes(hInstall, "IngresIce", 16);
    }

    if (!ComponentInstalled("Ingres/Replicator"))
    {
	rep=FALSE;
	MsiSetFeatureAttributes(hInstall, "IngresReplicator", 16);
    }

    if (!ComponentInstalled("Ingres/Star"))
    {
	star=FALSE;
	MsiSetFeatureAttributes(hInstall, "IngresStar", 16);
    }

    if (!ComponentInstalled("Ingres DBMS Server") && !ice && !rep && !star)
    {
	MsiSetFeatureAttributes(hInstall, "IngresDBMS", 16);
    }

    if (!ComponentInstalled("Ingres/Net"))
    {
	MsiSetFeatureAttributes(hInstall, "IngresNet", 16);
	MsiSetFeatureAttributes(hInstall, "IngresTCPIP", 16);
	MsiSetFeatureAttributes(hInstall, "IngresNetBIOS", 16);
	MsiSetFeatureAttributes(hInstall, "IngresDECNet", 16);
	MsiSetFeatureAttributes(hInstall, "IngresNovellSPXIPX", 16);
    }
    else
    {		
	sprintf(ConfigKey, "ii.%s.gcb.*.wintcp.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)!=ERROR_SUCCESS
	    || _stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureAttributes(hInstall, "IngresTCPIP", 16);

	sprintf(ConfigKey, "ii.%s.gcb.*.tcp_ip.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)!=ERROR_SUCCESS
	    || _stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureAttributes(hInstall, "IngresTCPIP", 16);

	sprintf(ConfigKey, "ii.%s.gcb.*.lanman.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)!=ERROR_SUCCESS
	    || _stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureAttributes(hInstall, "IngresNetBIOS", 16);

	sprintf(ConfigKey, "ii.%s.gcb.*.decnet.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)!=ERROR_SUCCESS
	    || _stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureAttributes(hInstall, "IngresDECNet", 16);

	sprintf(ConfigKey, "ii.%s.gcb.*.lanman.status", Host);
	if (Local_PMget(ConfigKey, ConfigKeyValue)!=ERROR_SUCCESS
	    || _stricmp(ConfigKeyValue, "ON"))
	    MsiSetFeatureAttributes(hInstall, "IngresNovellSPXIPX", 16);
    }

    if (!ComponentInstalled("Ingres ESQL/C"))
	MsiSetFeatureAttributes(hInstall, "IngresEsqlC", 16);

    if (!ComponentInstalled("Ingres ESQL/COBOL"))
	MsiSetFeatureAttributes(hInstall, "IngresEsqlCobol", 16);

    if (!ComponentInstalled("Ingres ESQL/FORTRAN"))
	MsiSetFeatureAttributes(hInstall, "IngresEsqlFortran", 16);

    if (!ComponentInstalled("Ingres Querying & Reporting Tools"))
	MsiSetFeatureAttributes(hInstall, "IngresTools", 16);

    if (!ComponentInstalled("Ingres/Vision"))
	MsiSetFeatureAttributes(hInstall, "IngresVision", 16);

    if (!ComponentInstalled("Ingres .NET Data Provider"))
	MsiSetFeatureAttributes(hInstall, "IngresDotNETDataProvider", 16);

    if (!ComponentInstalled("Ingres II On-line Documentation") 
        && !ComponentInstalled("Ingres 2.6 Documentation"))
	MsiSetFeatureAttributes(hInstall, "IngresDoc", 16);

    return ERROR_SUCCESS;
}

/*
**  Name:ingres_check_targetpath_duplicated
**
**  Description:
**	Check if the target path user chooses already has an installation.
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
**	24-Aug-2001 (penga03)
**	    Created.
**	14-Sep-2001 (penga03)
**	    If this is an upgrade, don't check the duplication of the target
**	    path.
**	26-feb-2004 (somsa01)
**	    Corrected error message output.
*/
UINT __stdcall 
ingres_check_targetpath_duplicated(MSIHANDLE hInstall) 
{
	char path[MAX_PATH+1], buf[MAX_PATH+1], id[3], szId[3];
	DWORD ret;
	char *szRegKey="SOFTWARE\\IngresCorporation\\Ingres";
	HKEY hkRegKey=0;

	MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");

	ret=sizeof(buf); *buf=0;
	MsiGetProperty(hInstall, "Installed", buf, &ret);
	if(buf[0])
		return ERROR_SUCCESS;

	ret=sizeof(buf); *buf=0;
	MsiGetProperty(hInstall, "INGRES_VER25", buf, &ret);
	if(!strcmp(buf, "1"))
		return ERROR_SUCCESS;

	ret=sizeof(id);
	MsiGetProperty(hInstall, "II_INSTALLATION", id, &ret);

	ret=sizeof(path);
	MsiGetTargetPath(hInstall, "INGRESFOLDER", path, &ret);
	if(path[strlen(path)-1] == '\\')
		path[strlen(path)-1] = '\0';

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_ENUMERATE_SUB_KEYS, &hkRegKey)==ERROR_SUCCESS)
	{
		LONG retCode;
		int i;
	
		for(i=0, retCode=ERROR_SUCCESS; retCode==ERROR_SUCCESS; i++) 
		{
			char szSubKey[16];
			DWORD dwSize=sizeof(szSubKey);
			HKEY hkSubKey=0;

			retCode=RegEnumKeyEx(hkRegKey, i, szSubKey, &dwSize, NULL, NULL, NULL, NULL);
			if(retCode==ERROR_SUCCESS && (RegOpenKeyEx(hkRegKey,szSubKey,0,KEY_QUERY_VALUE,&hkSubKey)==ERROR_SUCCESS))
			{
				DWORD Type;
				char Data[MAX_PATH+1];
				DWORD cbData=sizeof(Data);
				
				if(RegQueryValueEx(hkSubKey, "II_SYSTEM", NULL,&Type,(BYTE *)Data,&cbData)==ERROR_SUCCESS)
				{
					sprintf(buf, "%s\\ingres\\files\\config.dat", Data);
					
					strncpy(szId, szSubKey, 2);
					szId[2]='\0';

					if(_stricmp(id, szId) && !_stricmp(Data, path) && GetFileAttributes(buf)!=-1)
					{
					    sprintf(buf, "You already have an Ingres instance '%s' installed in '%s' and can NOT install another instance '%s' in the same directory!", szId, Data, id);						MyMessageBox(hInstall, buf);
						MyMessageBox(hInstall, buf);
						MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
					}
					if(!_stricmp(id, szId) && _stricmp(Data, path) && GetFileAttributes(buf)!=-1)
					{
						sprintf(buf, "The Ingres instance '%s' has already been installed in '%s', you can NOT re-install it to '%s'!", szId, Data, path);
						MyMessageBox(hInstall, buf);
						MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
					}
				}
				RegCloseKey(hkSubKey);
			}
		}
		RegCloseKey(hkRegKey);
	}

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_update_properties
**
**  Description:
**	Update some properties whose values will be save in the Registry. Must 
**	be inserted before WriteRegistryValues in Execute sequence.
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
**	28-jan-2002 (penga03)
**	    Created.
*/

UINT __stdcall 
ingres_update_properties(MSIHANDLE hInstall) 
{
	char buf[256];
	DWORD nret;

	nret=sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_RSP_LOC", buf, &nret);
	if (strcmp(buf, "0"))
		MsiSetProperty(hInstall, "INGRES_SILENT_INSTALLED", "1");
	
	nret=sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_RSP_JDBCD", buf, &nret);
	if (!strcmp(buf, "1"))
		MsiSetProperty(hInstall, "INGRES_JDBCD_SILENT_INSTALLED", "1");
	
	nret=sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_RSP_JDBCS", buf, &nret);
	if (!strcmp(buf, "1"))
		MsiSetProperty(hInstall, "INGRES_JDBCS_SILENT_INSTALLED", "1");
	
	nret=sizeof(buf);
	MsiGetProperty(hInstall, "INGRES_RSP_VDBA", buf, &nret);
	if (!strcmp(buf, "1"))
		MsiSetProperty(hInstall, "INGRES_VDBA_SILENT_INSTALLED", "1");

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_cleanup_immediate
**
**  Description:
**	Clean up the temporary files and registry enteries. Run in 
**	immediate execution mode.
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
**	28-jan-2002 (penga03)
**	    Created.
*/
UINT __stdcall 
ingres_cleanup_immediate(MSIHANDLE hInstall) 
{
	char code[3], msiloc[MAX_PATH+1], szBuf[MAX_PATH+1];
	DWORD dwSize;
	char RegKey[256];
	HKEY hkRegKey;
	char prodcode[39], removecab[2];
    	char szTemp1[MAX_PATH], szTemp2[MAX_PATH], szPath[MAX_PATH+1];
	LPITEMIDLIST pidlProg, pidlComProg;
	LPMALLOC pMalloc = NULL;


	dwSize=sizeof(code); *code=0;
	MsiGetProperty(hInstall, "II_INSTALLATION", code, &dwSize);

	dwSize=sizeof(msiloc); *msiloc=0;
	MsiGetProperty(hInstall, "INGRES_MSI_LOC", msiloc, &dwSize);
	if (msiloc[strlen(msiloc)-1] == '\\')
		msiloc[strlen(msiloc)-1] = '\0';

	dwSize=sizeof(prodcode); *prodcode=0;
	MsiGetProperty(hInstall, "ProductCode", prodcode, &dwSize);

	dwSize=sizeof(removecab); *removecab=0;
	MsiGetProperty(hInstall, "INGRES_REMOVE_CAB", removecab, &dwSize);

	/* Delete the temporary cab and msi files. */
	sprintf(RegKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", prodcode);
	if ( _stricmp(code, "II") && 
		(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey) || !_stricmp(removecab, "1")) )
	{
		sprintf(szBuf, "%s\\IngresII[%s].cab", msiloc, code);
		DeleteFile(szBuf);
		sprintf(szBuf, "%s\\IngresII[%s].msi", msiloc, code);
		DeleteFile(szBuf);
	}
	if (hkRegKey) RegCloseKey(hkRegKey);

	/* Delete the temporary registry values.*/
	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", code);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
	{
		RegDeleteValue(hkRegKey, "InstalledFeatures");
		RegDeleteValue(hkRegKey, "RemovedFeatures");
		RegCloseKey(hkRegKey);
	}

	/* Delete now obsolete CA shortcut from Start Menu */
    SHGetMalloc(&pMalloc);
    SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlProg);
    SHGetPathFromIDList(pidlProg, szPath);

	sprintf(szTemp1, "%s\\Computer Associates\\Ingres [ %s ]", szPath, code);
	if (GetFileAttributes(szTemp1)!=-1)
	{
		sprintf(szTemp2, "%s\\Ingres Documentation", szTemp1);
	    if (RemoveOneDir(szTemp2))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp2, 0);
	    if (RemoveOneDir(szTemp1))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
	}
    
	SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlComProg);
	SHGetPathFromIDList(pidlComProg, szPath);
	sprintf(szTemp1, "%s\\Computer Associates\\Ingres [ %s ]", szPath, code);
	if (GetFileAttributes(szTemp1)!=-1)
	{
		sprintf(szTemp2, "%s\\Ingres Documentation", szTemp1);
	    if (RemoveOneDir(szTemp2))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp2, 0);
	    if (RemoveOneDir(szTemp1))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szTemp1, 0);
	}

	if (pidlProg)
	    pMalloc->lpVtbl->Free(pMalloc, pidlProg);
	if (pidlComProg)
		pMalloc->lpVtbl->Free(pMalloc, pidlComProg);
	pMalloc->lpVtbl->Release(pMalloc);

	/* Delete the obsolete registry key.*/
	sprintf(RegKey, "Software\\ComputerAssociates\\Ingres\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "Software\\ComputerAssociates\\Advantage Ingres\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Advantage Ingres");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "Software\\ComputerAssociates\\IngresII\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_cleanup_commit
**
**  Description:
**	Clean up the temporary files and registry enteries. Run in 
**	commit execution mode.
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
**	28-jan-2002 (penga03)
**	    Created.
**	29-Aug-2007 (drivi01)
**	    Remove newly added configuration location in 
**	    USERPROFILE.
*/
UINT __stdcall 
ingres_cleanup_commit(MSIHANDLE hInstall) 
{
	char code[3], ii_system[1024], msiloc[MAX_PATH+1];
	char szBuf[MAX_PATH+1], szBuf2[MAX_PATH+1];
	DWORD dwSize, dwSize2;
	char RegKey[256];
	HKEY hkRegKey;
	char *p, *q, *tokens[10];
	int i;
    char szPath[MAX_PATH];
	LPITEMIDLIST pidlProg, pidlComProg;
	char prodcode[39];
	BOOL bRemoveCAB=0, bRemove=0;
	LPMALLOC pMalloc = NULL;
	char *env;

	/*
	**CustomActionData:
	**[II_INSTALLATION];[INGRES_MSI_LOC];[ProductCode];[INGRES_REMOVE_CAB]
	*/
	dwSize=sizeof(szBuf);
	MsiGetProperty(hInstall, "CustomActionData", szBuf, &dwSize);
	p=szBuf;
	for (i=0; i<=3; i++)
	{
		if (p) q=strchr(p, ';');
		if (q) *q='\0';
		tokens[i]=p;
		if (q) p=q+1; else p=NULL;
	}
	strcpy(code,tokens[0]);
	strcpy(msiloc, tokens[1]);
	if (msiloc[strlen(msiloc)-1] == '\\') 
		msiloc[strlen(msiloc)-1] = '\0';
	strcpy(prodcode, tokens[2]);
	bRemoveCAB=0;
	if (!_stricmp(tokens[3], "1")) bRemoveCAB=1;

	/* Delete the temporary cab and msi files. */
	sprintf(RegKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", prodcode);
	if ( _stricmp(code, "II") && 
		(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey) || bRemoveCAB) )
	{
		sprintf(szBuf, "%s\\IngresII[%s].cab", msiloc, code);
		DeleteFile(szBuf);
		sprintf(szBuf, "%s\\IngresII[%s].msi", msiloc, code);
		DeleteFile(szBuf);
	}
	if (hkRegKey) RegCloseKey(hkRegKey);

	/* Delete the temporary registry values, as well as the key (if needed). */
	sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\%s_Installation", code);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey))
	{
		dwSize=sizeof(szBuf);
		dwSize2=sizeof(szBuf2);
		if (!(RegQueryValueEx(hkRegKey, "InstalledFeatures", NULL, NULL, (BYTE *)szBuf, &dwSize)) 
		&& !(RegQueryValueEx(hkRegKey, "RemovedFeatures", NULL, NULL, (BYTE *)szBuf2, &dwSize2))
		&& !_stricmp(szBuf, szBuf2))
		bRemove=1; /* This requries the two values has the same order of the features. */

		RegDeleteValue(hkRegKey, "InstalledFeatures");
		RegDeleteValue(hkRegKey, "RemovedFeatures");
		RegCloseKey(hkRegKey);
	}
	if (bRemove)
	{
		DWORD dwError;
		if (GetInstallPath(code, ii_system))
		{
			sprintf(szBuf, "%s\\ingres", ii_system);
			RemoveDir(szBuf, TRUE);
			RemoveDirectory(ii_system);
		}
		if ((env = getenv("USERPROFILE")) != NULL )
		{
		    sprintf(szBuf, "%s\\AppData\\Local\\Ingres", env);
		    if (_access(szBuf, 00) == 0)
			{
				if (RemoveDir(szBuf, TRUE) != ERROR_SUCCESS)
					dwError = GetLastError();	
			}
			
		}		
		sprintf(RegKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", code);
		RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
		sprintf(RegKey, "SOFTWARE\\IngresCorportation\\Ingres");
		RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	}

	/* Delete the obsolete registry key.*/
	sprintf(RegKey, "Software\\ComputerAssociates\\Ingres\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "Software\\ComputerAssociates\\Advantage Ingres\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Advantage Ingres");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "Software\\ComputerAssociates\\IngresII\\%s_Installation", code);
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);
	sprintf(RegKey, "SOFTWARE\\ComputerAssociates");
	RegDeleteKey(HKEY_LOCAL_MACHINE, RegKey);

    /* Remove the empty Ingres folder. */
    CoInitialize(NULL);

    SHGetMalloc(&pMalloc);
    SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlComProg);
    SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlProg);

    SHGetPathFromIDList(pidlComProg, szPath);
	/* remove old CA shortcut first */
	sprintf(szBuf, "%s\\Computer Associates\\Ingres [ %s ]", szPath, code);
	if (GetFileAttributes(szBuf)!=-1)
	{
		sprintf(szBuf2, "%s\\Ingres Documentation", szBuf);
	    if (RemoveOneDir(szBuf2))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf2, 0);
	    if (RemoveOneDir(szBuf))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
	}
    sprintf(szBuf, "%s\\Ingres II [ %s ]", szPath, code);
    if (RemoveDirectory(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
    sprintf(szBuf, "%s\\Ingres II", szPath);
    if (RemoveDirectory(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);

    /* remove old Ingres shortcuts*/
    sprintf(szBuf, "%s\\Ingres\\Ingres [ %s ]", szPath, code);
    if (GetFileAttributes(szBuf)!=-1)
    {
	sprintf(szBuf2, "%s\\Ingres Documentation", szBuf);
	if (RemoveOneDir(szBuf2))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf2, 0);
	if (RemoveOneDir(szBuf))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
    }

    SHGetPathFromIDList(pidlProg, szPath);
	/* remove old CA shortcut first */
	sprintf(szBuf, "%s\\Computer Associates\\Ingres [ %s ]", szPath, code);
	if (GetFileAttributes(szBuf)!=-1)
	{
		sprintf(szBuf2, "%s\\Ingres Documentation", szBuf);
	    if (RemoveOneDir(szBuf2))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf2, 0);
	    if (RemoveOneDir(szBuf))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
	}
    sprintf(szBuf, "%s\\Ingres II [ %s ]", szPath, code);
    if (RemoveDirectory(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
    sprintf(szBuf, "%s\\Ingres II", szPath);
    if (RemoveDirectory(szBuf))
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);

    /* remove old Ingres shortcuts*/
    sprintf(szBuf, "%s\\Ingres\\Ingres [ %s ]", szPath, code);
    if (GetFileAttributes(szBuf)!=-1)
    {
	sprintf(szBuf2, "%s\\Ingres Documentation", szBuf);
	if (RemoveOneDir(szBuf2))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf2, 0);
	if (RemoveOneDir(szBuf))
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szBuf, 0);
    }
    

    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlComProg, NULL);
    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlProg, NULL);

    if (pidlComProg) 
	pMalloc->lpVtbl->Free(pMalloc, pidlComProg);
    if (pidlProg)
	pMalloc->lpVtbl->Free(pMalloc, pidlProg);

    pMalloc->lpVtbl->Release(pMalloc);
    CoUninitialize();

	return ERROR_SUCCESS;
}

/*
**  Name:ingres_set_targetpath
**
**  Description:
**	By default, always install Ingres under the directory 
**	"Program Files\CA\Advantage Ingres [instanceid]".
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
**	30-jan-2002 (penga03)
**	    Created.
*/
UINT __stdcall 
ingres_set_targetpath(MSIHANDLE hInstall) 
{
    char szCode[3], szBuf[MAX_PATH], szII_SYSTEM[MAX_PATH];
    DWORD dwSize;
    BOOL bInstalled;

    dwSize=sizeof(szCode);
    MsiGetProperty(hInstall, "II_INSTALLATION", szCode, &dwSize);

    bInstalled=0;
    if (GetInstallPath(szCode, szII_SYSTEM))
    {
	sprintf(szBuf, "%s\\ingres\\files\\config.dat", szII_SYSTEM);
	if (GetFileAttributes(szBuf) != -1)
	    bInstalled=1;
    }

    if (!bInstalled)
    {
	dwSize=sizeof(szBuf);
	MsiGetTargetPath(hInstall, "INGRESFOLDER", szBuf, &dwSize);
	if (szBuf[strlen(szBuf)-1] == '\\')
	    szBuf[strlen(szBuf)-1] = '\0';
	sprintf(szII_SYSTEM, "%s%s\\", szBuf, szCode);
    }

    MsiSetTargetPath(hInstall, "INGRESFOLDER", szII_SYSTEM);

    return ERROR_SUCCESS;
}

/*
**  Name:ingressdk_cleanup
**
**  Description:
**	Clean up the temporary registry enteries. Run in commit 
**	execution mode.
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
**	28-feb-2002 (penga03)
**	    Created.
*/
UINT __stdcall 
ingressdk_cleanup(MSIHANDLE hInstall) 
{
    char RegKey[256];
    HKEY hkRegKey;

    /* Delete the temporary registry keys.*/
    sprintf(RegKey, "Software\\IngresCorporation\\Ingres\\SD_Installation");
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hkRegKey)==ERROR_SUCCESS)
    {
	RegDeleteValue(hkRegKey, "InstalledFeatures");
	RegDeleteValue(hkRegKey, "RemovedFeatures");
	RegCloseKey(hkRegKey);
    }
    return ERROR_SUCCESS;
}

/*
**  Name:ingressdk_check_targetpath_duplicated
**
**  Description:
**	Check if the target path user chooses already has an installation.
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
**	28-feb-2002 (penga03)
**	    Created.
*/
UINT __stdcall 
ingressdk_check_targetpath_duplicated(MSIHANDLE hInstall) 
{
    char szBuf[MAX_PATH];
    DWORD dwSize;

    MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "Installed", szBuf, &dwSize);
    if (szBuf[0])
	return ERROR_SUCCESS;

    dwSize=sizeof(szBuf);
    MsiGetTargetPath(hInstall, "INSTALLDIR", szBuf, &dwSize);
    if (szBuf[strlen(szBuf)-1] == '\\')
	szBuf[strlen(szBuf)-1] = '\0';
    strcat(szBuf, "\\ingres\\files\\config.dat");
    if (GetFileAttributes(szBuf)!=-1)
    {
	MyMessageBox(hInstall, "You cannot choose this folder, it already have an installation!");
	MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
    }
    return ERROR_SUCCESS;
}

/*
**  Name:ingressdk_set_environment
**
**  Description:
**	Delete the tailing back slash of INGRES_II_SYSTEM.
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
**	28-feb-2002 (penga03)
**	    Created.
*/
UINT __stdcall 
ingressdk_set_environment(MSIHANDLE hInstall) 
{
    char path[MAX_PATH];
    DWORD nret;

    nret=sizeof(path);
    MsiGetTargetPath(hInstall, "INSTALLDIR", path, &nret);
    if (path[strlen(path)-1] == '\\')
	path[strlen(path)-1] = '\0';
    MsiSetProperty(hInstall, "INGRES_II_SYSTEM", path);
    return ERROR_SUCCESS;
}

/*
**  Name:ingressdk_uninstall
**
**  Description:
**	Uninstall CleverPath Portal, OpenROAD and Forest & Trees.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS		The function succeeds.
**	    ERROR_INSTALL_FAILURE	The function fails.
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	28-feb-2002 (penga03)
**	    Created.
**	26-mar-2002 (penga03)
**	    Commented the OpenROAD part out since the SDK doesn't 
**	    include OpenROAD for now.
**	20-may-2003 (penga03)
**	    Took away the comment on OpenROAD, and changed the 
**	    command that uninstalls Portal 4.51.
*/
UINT __stdcall 
ingressdk_uninstall(MSIHANDLE hInstall) 
{ 
    char cmd[MAX_PATH], WindDir[1024], CurDir[MAX_PATH], NewDir[MAX_PATH];
    char szBuf[1024], InstallDir[1024];
    DWORD dwSize;

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "REMOVE", szBuf, &dwSize);
    if(!szBuf[0] || !strstr(szBuf, "IngresSDK"))
	return ERROR_SUCCESS;

    dwSize=sizeof(InstallDir);
    MsiGetTargetPath(hInstall, "INSTALLDIR", InstallDir, &dwSize);
    if(InstallDir[strlen(InstallDir)-1]=='\\')
	InstallDir[strlen(InstallDir)-1]='\0';

    /* stop portal */
    GetCurrentDirectory(sizeof(CurDir), CurDir);
    sprintf(NewDir, "%s\\PortalSDK", InstallDir);
    SetCurrentDirectory(NewDir);
    ExecuteEx("stopportal.bat");
    SetCurrentDirectory(CurDir);
    
    /* remove CleverPath Portal: %INSTALLDIR%\PortalSDK\uninst.exe -silent */
    sprintf(cmd, "\"%s\\PortalSDK\\uninst.exe\" -silent", InstallDir);
    ExecuteEx(cmd);

    /* 
    ** remove OpenRoad: 
    ** "%ProgramFiles%\InstallShield Installation Information\{8568E000-AE70-11D4
    ** -8EE7-00C04F81B484}\Setup.exe" -s -f1"%ProgramFiles%\InstallShield Installatio
    ** n Information\{8568E000-AE70-11D4-8EE7-00C04F81B484}\uninstall.iss"
    */
    GetEnvironmentVariable("programfiles", szBuf, sizeof(szBuf));
    sprintf(cmd, "\"%s\\InstallShield Installation Information\\{8568E000-AE70-11D4-8EE7-00C04F81B484}\\Setup.exe\" -s -f1\"%s\\InstallShield Installation Information\\{8568E000-AE70-11D4-8EE7-00C04F81B484}\\uninstall.iss\"", szBuf, szBuf);
    ExecuteEx(cmd);

    /* remove Forest And Trees: E:\WINNT\IsUninst.exe -a -fE:\WINNT\FTWRTUNI.isu */
    GetWindowsDirectory(WindDir, sizeof(WindDir));
    sprintf(cmd, "%s\\IsUninst.exe -a -f%s\\FTWRTUNI.isu", WindDir, WindDir);
    ExecuteEx(cmd);

    return ERROR_SUCCESS;
}

/*
**  Name:ingressdk_remove
**
**  Description:
**	Remove Apache HTTP Server, remove OS user created by the SDK.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS		The function succeeds.
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	12-mar-2002 (penga03)
**	    Created.
*/
UINT __stdcall 
ingressdk_remove(MSIHANDLE hInstall) 
{ 
    char szBuf[1024], InstallDir[MAX_PATH];
    DWORD dwSize;
    char cmd[1024], RegKey[256];
    HKEY hRegKey;
    WCHAR wUsername[MAX_PATH];
    NET_API_STATUS nStatus;
    BOOL bRemove=FALSE;

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "_IsMaintenance", szBuf, &dwSize);
    if(_stricmp(szBuf, "Remove"))	
    return ERROR_SUCCESS;

    strcpy(RegKey, "Software\\IngresCorporation\\Ingres\\SD_Installation");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_ALL_ACCESS, &hRegKey) ==ERROR_SUCCESS)
    {
	DWORD dwSize=sizeof(InstallDir);

	if(RegQueryValueEx(hRegKey, "II_SYSTEM", NULL, NULL, (BYTE *)InstallDir, &dwSize)==ERROR_SUCCESS)
	{
	    if (InstallDir[strlen(InstallDir)-1]=='\\')
		InstallDir[strlen(InstallDir)-1]='\0';
	}
	RegCloseKey(hRegKey);
    }

    /* remove Apache HTTP Server if installed by Ingres */
    strcpy(RegKey, "SOFTWARE\\Apache Group\\Apache\\1.3.23");
    if(RegOpenKeyEx(HKEY_CURRENT_USER, RegKey, 0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
	char szData[256];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "InstalledBy", NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS 
	                   && !strcmp(szData, "CA-IngresSDK"))
	{
	    bRemove=TRUE;
	}
	RegCloseKey(hRegKey);
    }

    if(bRemove)
    {
	strcpy(cmd, "MsiExec.exe /x {5D29A4EF-A57F-4F47-89F8-4EB3C5302A53} /qn");
	ExecuteEx(cmd);
	RegDeleteKey(HKEY_CURRENT_USER, RegKey);
	sprintf(szBuf, "%s\\Apache", InstallDir);
	RemoveDir(szBuf, TRUE);
    }
    
    /* clean up SDK directory */
    sprintf(szBuf, "%s\\PortalSDK", InstallDir);
    RemoveDir(szBuf, TRUE);
    sprintf(szBuf, "%s\\JRE", InstallDir);
    RemoveDir(szBuf, TRUE);
    sprintf(szBuf, "%s\\ingres\\ice\\HTTPArea", InstallDir);
    RemoveDir(szBuf, TRUE);
    RemoveDir(InstallDir, TRUE);

    /* remove OS user 'icedbuser' */
    MultiByteToWideChar(CP_ACP, 0, "icedbuser", -1, wUsername, MAX_PATH);
    nStatus = NetUserDel(NULL, wUsername);

    return ERROR_SUCCESS;
}

/*
**  Name:ingressdk_check_existing_ingres
**
**  Description:
**	If SDK, the install will exit if find out at least one Ingres 
**	instance currently installed.
**
**  Inputs:
**	hInstall	Handle to the installation.
**
**  Outputs:
**	None.
**	Returns:
**	    ERROR_SUCCESS		No Ingres instance installed
**	    ERROR_INSTALL_FAILURE	At least one Ingres instance installed
**	Exceptions:
**	    None.
**
**  Side Effects:
**	None.
**
**  History:
**	11-jun-2002 (penga03)
**	    Created.
*/
UINT __stdcall 
ingressdk_check_existing_ingres(MSIHANDLE hInstall) 
{
    char szBuf[MAX_PATH],ii_system[MAX_PATH], RegKey[512];
    DWORD dwSize, dwType;
    HKEY hKey;
    int i;

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "Installed", szBuf, &dwSize);
    if(szBuf[0])
	return ERROR_SUCCESS;

    /* Get II_SYSTEM */
    if(GetEnvironmentVariable("II_SYSTEM", ii_system, sizeof(ii_system)))
    {
	dwSize=sizeof(szBuf);
	sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
	if (GetFileAttributes(szBuf) != -1)
	{
	    MyMessageBox(hInstall, "The Ingres SDK can only be installed on machines which do not contain any existing instances of Ingres or OpenROAD. This install cannot continue.");
	    return ERROR_INSTALL_FAILURE;
	}
    }

    strcpy(RegKey, "SOFTWARE\\IngresCorporation\\Ingres");
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;

	for(i=0, retCode=ERROR_SUCCESS; retCode==ERROR_SUCCESS; i++) 
	{
	    char SubKey[16];
	    HKEY hSubKey;

	    dwSize=sizeof(SubKey);
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    dwSize=sizeof(ii_system);
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
		    {
			sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
			if (GetFileAttributes(szBuf)!=-1)
			{
			    MyMessageBox(hInstall, "The Ingres SDK can only be installed on machines which do not contain any existing instances of Ingres or OpenROAD. This install cannot continue.");
			    RegCloseKey(hSubKey);
			    RegCloseKey(hKey);
			    return ERROR_INSTALL_FAILURE;
			}						
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */
	RegCloseKey(hKey);
    }

    strcpy(RegKey, "SOFTWARE\\ComputerAssociates\\Ingres");
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;

	for(i=0, retCode=ERROR_SUCCESS; retCode==ERROR_SUCCESS; i++) 
	{
	    char SubKey[16];
	    HKEY hSubKey;

	    dwSize=sizeof(SubKey);
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    dwSize=sizeof(ii_system);
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
		    {
			sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
			if (GetFileAttributes(szBuf)!=-1)
			{
			    MyMessageBox(hInstall, "The Ingres SDK can only be installed on machines which do not contain any existing instances of Ingres or OpenROAD. This install cannot continue.");
			    RegCloseKey(hSubKey);
			    RegCloseKey(hKey);
			    return ERROR_INSTALL_FAILURE;
			}						
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */
	RegCloseKey(hKey);
    }

    strcpy(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII");
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;

	for(i=0, retCode=ERROR_SUCCESS; retCode==ERROR_SUCCESS; i++) 
	{
	    char SubKey[16];
	    HKEY hSubKey;
	    
	    dwSize=sizeof(SubKey);
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    dwSize=sizeof(ii_system);
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
		    {
			sprintf(szBuf, "%s\\ingres\\files\\config.dat", ii_system);
			if (GetFileAttributes(szBuf)!=-1)
			{
			    MyMessageBox(hInstall, "The Ingres SDK can only be installed on machines which do not contain any existing instances of Ingres or OpenROAD. This install cannot continue.");
			    RegCloseKey(hSubKey);
			    RegCloseKey(hKey);
			    return ERROR_INSTALL_FAILURE;
			}						
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */
	RegCloseKey(hKey);
    }

    return ERROR_SUCCESS;
}

/*
**  Name:ingres_checkiflicenseviewed
**
**  Description:
**	Check if the user has read the license agreement.
**
**  History:
**	15-dec-2002 (penga03)
**	    Created.
*/
BOOL CALLBACK EnumChildProc(HWND hwnd,LPARAM lParam)
{
    TCHAR buf[100];

    GetClassName( hwnd, (LPTSTR)&buf, 100 );
    if ( _tcscmp( buf, _T("RichEdit20W") ) == 0 )
    {
	*(HWND*)lParam = hwnd;
	return FALSE;
    }
    return TRUE;
}
 
UINT __stdcall 
ingres_checkiflicenseviewed(MSIHANDLE hInstall)
{
    DWORD size;
    char buf[1024], dialogName[1024];
    HWND hWnd;
    HWND hWndChild=NULL;
    SCROLLINFO scrollInfo;
    int max_pos;

    MsiSetProperty(hInstall,"INGRES_LICENSE_VIEWED","0");

    size = sizeof(buf);
    MsiGetProperty(hInstall, "ProductName", buf, &size);
    sprintf(dialogName, "%s - Installation Wizard", buf);

    hWnd = FindWindow(NULL, dialogName);
    if ( hWnd )
    {
	EnumChildWindows( hWnd, EnumChildProc, (LPARAM)&hWndChild );
	if ( hWndChild )
	{
  	    ZeroMemory(&scrollInfo,sizeof(SCROLLINFO));
  	    scrollInfo.cbSize = sizeof(SCROLLINFO);
  	    scrollInfo.fMask = SIF_ALL;

  	    GetScrollInfo(hWndChild,SB_VERT,&scrollInfo);
	    max_pos = scrollInfo.nMax - scrollInfo.nPage - 5;
	    if (scrollInfo.nPos >= max_pos)
		MsiSetProperty(hInstall, TEXT("INGRES_LICENSE_VIEWED"), "1");
	    else
		MsiSetProperty(hInstall, TEXT("INGRES_LICENSE_VIEWED"), "0");
	}
    }
	
    return ERROR_SUCCESS;
}

/*
**  Name: ingres_start_ingres
**
**  Description:
**	Start Ingres and IVM.
**
**  History:
**	14-dec-2004 (penga03)
**	    Created.
*/
UINT __stdcall
ingres_start_ingres(MSIHANDLE hInstall)
{
    char szId[3], szII_SYSTEM[1024], szBuf[2048];
    DWORD dwSize, dwBytesNeeded;
    SC_HANDLE schSCManager, schService;
    LPQUERY_SERVICE_CONFIG pqsc;
    BOOL bServiceAuto;

    dwSize = sizeof(szId);
    if (MsiGetProperty(hInstall, "CustomActionData", szId, &dwSize) || !szId[0])
	return ERROR_SUCCESS;

    bServiceAuto=FALSE;
    schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager)
    {
	sprintf(szBuf, "Ingres_Database_%s", szId);
	schService=OpenService(schSCManager, szBuf, SERVICE_QUERY_CONFIG);
	if (schService)
	{
	    QueryServiceConfig(schService, NULL, 0, &dwBytesNeeded);
	    pqsc=malloc(dwBytesNeeded);
	    QueryServiceConfig(schService, pqsc, dwBytesNeeded, &dwBytesNeeded);
	    if (pqsc->dwStartType == SERVICE_AUTO_START)
		bServiceAuto=TRUE;
	    free(pqsc);
	    CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);
    }

    if (GetInstallPath(szId, szII_SYSTEM))
    {
	if (bServiceAuto)
	{
	    sprintf(szBuf, "\"%s\\ingres\\utility\\ingstart.exe\" -service", szII_SYSTEM);
	    Execute(szBuf, TRUE);
	}
	else
	{
	    sprintf(szBuf, "\"%s\\ingres\\utility\\ingstart.exe\"", szII_SYSTEM);
	    Execute(szBuf, TRUE);
	}
	sprintf(szBuf, "\"%s\\ingres\\bin\\ivm.exe\"", szII_SYSTEM);
	Execute(szBuf, FALSE);
    }

    return ERROR_SUCCESS;
}

/*
**  Name: ingres_set_controls
**
**  Description:
**	Set dialog controls dynamically.
**
**  History:
**	28-jun-2005 (penga03)
**	    Created. Set the default control of ReadyToInstall dialog
**	    to CreateRSP if the user is creating a response file.
*/
UINT __stdcall
ingres_set_controls(MSIHANDLE hInstall)
{
    char szBuf[1024], szQuery[1024];
    DWORD dwSize;
    MSIHANDLE hDB, hView, hRecord;
    BOOL bCreateRSP;

    bCreateRSP=0;
    dwSize = sizeof(szBuf);
    if (!MsiGetProperty(hInstall, "INGRES_CREATE_RSP", szBuf, &dwSize) && !_stricmp(szBuf, "1"))
	bCreateRSP=1;

    sprintf(szQuery, "SELECT * FROM Dialog WHERE Dialog='ReadyToInstall'");
    hDB=MsiGetActiveDatabase(hInstall);
    if (hDB)
    {
	if (!MsiDatabaseOpenView(hDB, szQuery, &hView))
	{
	    if (!MsiViewExecute(hView, 0))
	    {
		if (!MsiViewFetch(hView, &hRecord))
		{
		    if (bCreateRSP)
			MsiRecordSetString(hRecord, 9, "CreateRSP");
		    else
			MsiRecordSetString(hRecord, 9, "InstallNow");

		    MsiViewModify(hView, MSIMODIFY_DELETE, hRecord);
		    MsiViewModify(hView, MSIMODIFY_INSERT_TEMPORARY, hRecord);

		    MsiCloseHandle(hRecord);
		}
	    }
	    MsiCloseHandle(hView);
	}
	MsiCloseHandle(hDB);
    }

    return ERROR_SUCCESS;
}


/*
**  Name: replace_location_string
**
**  Description: This function replaces a hardcoded path in RTF file with the one
**		 specified.
**
**  Inputs: 
**	pathBuf - Pointer to the new path that will be placed into RTF file.
**	location - pointer to the name of the location that the path is being replaced for.
**	txt_buf - Pointer to the pointer of the buffer that contains entire rtf file.
**	txt_buf_head, txt_buf_tail - Pointers to the buffers that are used to manipulate 
**				     contents of main buffer. Always the same size as txt_buf.
**	szBuf - Size of the txt_buf, txt_buf_head, txt_buf_tail.
**
**  Outputs: 
**	txt_buf - Pointer to the buffer with updated path.
**
**
**  History:
**	06-Oct-2006 (drivi01)
**	    Created.
*/
void
replace_location_string(char *pathBuf, char *location, char **txt_buf, char *txt_buf_head, char *txt_buf_tail,  int szBuf)
{
	int		length;
	char	*point, *point2;
	int		len1;
	int		len2;
	int		i=0;
	char	*buf;
	char	default_location[] = {"C:\\\\Program Files\\\\Ingres\\\\IngresII\\\\"};


	buf=*txt_buf;
	if ((point=strstr(buf, location))!= NULL)
	{
		if ((point2 = strstr(point, default_location)) != NULL)
		{
			length = point2-buf;
			strncpy(txt_buf_head, buf, length);
			txt_buf_head[length]='\0';
			point2=point2+sizeof(default_location)-1;
			len1=sizeof(default_location);
			len2=szBuf-length-len1;
			strncpy(txt_buf_tail, point2, len2);
			txt_buf_tail[len2]='\0';
			sprintf(buf, "%s%s%s%c", txt_buf_head, pathBuf, txt_buf_tail, '\0');
		}
	}

}


/*
**  Name: duplicate_slashes
**
**  Description: This function duplicates slashes in the path.
**		 Replaces each "\" with "\\".
**
**
**  History:
**	06-Oct-2006 (drivi01)
**	    Created.
**	16-Jan-2009 (whiro01)
**	    Made "pathBuf" a static because we pass its address back
**	    to the caller.
*/
char *
duplicate_slashes(char *path)
{
	static char    pathBuf[MAX_PATH+1];
	char	*ptr=pathBuf;
	int		i=0;

	while(path[i]!='\0')
	{
		if (path[i] == '\\')
		{
			*ptr++=path[i];
			*ptr++=path[i];
		}
		else
			*ptr++=path[i];

		i++;
	}
	*ptr='\0';
	return pathBuf;
}


/*
**  Name: ingres_addsummary
**
**  Description: This function is responsible for processing
**		 RTF file containing summary for the installation.
**		 It searches for hardcoded strings in the RTF file and
**		 replaces them with information relevant to the install.
**	         The way this is designed to work is as follows:
**		 when user gets to ReadyToInstall dialog in the installer
**	         this function is called and new rtf file is generated
**		 relevant to the installation settings while old file is
**		 backed up, if user clicks back button on ReadyToInstall
**	         dialog, new file gets destroyed, old file gets loaded back
**		 into MSI from a backed up version, so user basically
**		 starts with a clean slate.
**		 
**
**
**  History:
**	16-Nov-2006 (drivi01)
**	    Created.
**	02-Jan-2007 (drivi01)
**	    Added Ingres Vision to packages in the Summary.
**	    Added Date Type Alias to the Summary.
**	    Slightly modified code to improve routines for package 
**	    selection processing.
**	14-Feb-2007 (drivi01)
**	    Renaming Federated Database Support component to Ingres Star.
**	    selection processing.
**	14-Feb-2007 (drivi01)
**	    Fix routines for update time zone information.
**          Update to time zone property is causing corruption in summary file.
**	20-Feb-2007 (drivi01)
**	    "Include Ingres in PATH" option in installer has been expanded to
**	    include INCLUDE and LIB variables as well.
**	03-May-2007 (drivi01)
**	    Add "Dual Transaction File Location" parameter to the Summary of
**	    the installation.
**	29-May-2008 (drivi01)
**	    Replace "Date Type Alias" with "Date Alias".
**	    Add read-only ODBC to the summary.
**	21-Aug-2008 (drivi01)
**	    Added summary generation routines for Spatial Object.
**	14-Jul-2009 (drivi01)
**	    SIR: 122309
**	    Update summary to include a new configuration system parameter.
**	    The parameter will setup configuration type for the Ingres instance.
**	    The parameter options include: Transactional System, Business 
**          Intelligence, Content Management and Traditional Ingres Systems.
**	24-Jul-2009 (drivi01)
**	    BUG 122359
**	    Rename "Ingres Networking" component to "Ingres Client".
*/
UINT __stdcall
ingres_addsummary(MSIHANDLE hInstall)
{
	char	iicode[3];
	DWORD	dwSize, szBuf, dwError;
	MSIHANDLE msih,  hView, hRec;
	char	setup_type[25];		
	char	component[25];
	char	component_name[50];
	char	componentBuf[MAX_PATH];
	const char	title[] = "Components Selected:";
	char cmd[MAX_PATH+1];
	char pathBuf[MAX_PATH+1];
	char pathBufD[(MAX_PATH+1)*2];
	char temp[MAX_PATH+1];
	char *txt_buf, *txt_buf_head, *txt_buf_tail, *txt_buf_tmp, *txt_buf_short = NULL;
	char *path;
	unsigned int length;
	int counter;
	u_char *point, *point2;
	HANDLE	handle;
	char	IngresDBMS[] = {"Ingres Database Server and Tools"};
	char	IngresNet[]  = {"Ingres Client"};
	char	IngresReplicator[] = {"Ingres Replicator"};
	char	IngresDotNet[] = {"Ingres .NET Data Provider"};
	char	IngresTools[] = {"Ingres Querying and Reporting Tools"};
	char	IngresVision[] = {"Ingres Vision"};
	char	IngresStar[] = {"Ingres Star"};
	char	IngresDoc[] = {"Ingres Documentation"};
	char	IngresSpat[] = {"Spatial Objects"};
	char	*components[] = {"IngresDBMSCheck", "IngresNetCheck", "IngresReplicatorCheck", "IngresDotNetCheck", 
							"IngresToolsCheck", "IngresStarCheck", "IngresDocCheck", "\0"};
	char	*component_names[] = {"Ingres Database Server and Tools", "Ingres Client", "Ingres Replicator", 
					"Ingres .NET Data Provider", "Ingres Querying and Reporting Tools",
					"Ingres Star", "Ingres Documentation", "\0"};
	char    *subcomponents[] = {"IngresVision", "IngresSpat", "\0"};
	char	*subcomponent_names[] = {"Ingres Vision", "Spatial Objects", "\0"};
	char	yes[] = {"Yes"};
	char	no[] = {"No"};
	char    default_config_type[] = {"Transactional System"};
	char	default_path[] = {"C:\\Program Files\\Ingres\\IngresII\\"};
	char	default_location[] = {"C:\\\\Program Files\\\\Ingres\\\\IngresII\\\\"};
	char    *config_types[] = {"Transactional System", "Traditional Ingres System", "Business Intelligence System", "Content Management System"};
	char	config_type[] = {"Configuration Type:"};
	char	location[] = {"location:"};
	char	dblocation[] = {"Database Location:"};
	char	backuplocation[] = {"Backup Location:"};
	char	journallocation[] = {"Journal Location:"};
	char	dumplocation[] = {"Dump Location:"};
	char	templocation[] = {"Temporary Location:"};
	char	translocation[] = {"Transaction File Location:"};
	char    dualtranslocation[] = {"Dual Transaction File Location:"};
	char	logsize_string[] = {"Transaction Log File Size:"};
	char	timezone_string[] = {"Time Zone:"};
	char	charset_string[] = {"Character Set:"};
	char	date_type_alias[] = {"Date Alias"};
	char	sqlcompliance_string[] = {"Strict SQL-92 Compliance:"};
	char	ingresinpath_string[] = {"Include Ingres in PATH, INCLUDE and LIB:"};
	char	demodb_string[] = {"Create Demonstration Database:"};
	char	readonly_odbc_string[] = {"Install Read-Only ODBC Driver:"};
	char	serviceauto_string[] = {"Service Start Auto:"};
	char	serviceuser_string[] = {"Service User:"};
	char	default_instance[] = {"Ingres II"};
	char	default_log_size[] = {"256"};
	char	default_timezone[] = "NA-EASTERN";
	char	default_charset[] = "WIN1252";
	char	default_date_type[] = "INGRES";
	char	default_service_user[] = "LocalSystem";
	char	secondary_service_user[] = "User";
	char	name[MAX_PATH];

	memset((char *)&componentBuf, 0, sizeof(componentBuf));	
	dwSize = sizeof(iicode);
	MsiGetProperty(hInstall, "II_INSTALLATION", iicode, &dwSize);
	msih = MsiGetActiveDatabase(hInstall);
	if (!msih)
		return FALSE;
	sprintf(cmd, "SELECT * FROM Control WHERE Dialog_ = 'ReadyToInstall' AND Control = 'ScrollableText1'");
	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
		return FALSE;
	szBuf = MsiRecordDataSize(hRec, 10);
	length = szBuf;
	txt_buf_short = malloc(szBuf+1);
	txt_buf = malloc(szBuf*2+1);
	txt_buf_head = malloc(szBuf*2+1);
	txt_buf_tail = malloc(szBuf*2+1);
	memset((char *)txt_buf_short, 0, szBuf+1);
	memset((char *)txt_buf, 0, szBuf*2+1);
	memset((char *)txt_buf_head, 0, szBuf*2+1);
	memset((char *)txt_buf_tail, 0, szBuf*2+1);
	length = szBuf*2+1;
	if (!MsiRecordGetString(hRec, 10, txt_buf_short, &szBuf))
		return FALSE;
	sprintf(txt_buf, "%s%c", txt_buf_short, '\0');
	if (txt_buf_short)
		free(txt_buf_short);
	szBuf=length;

	//remove junk at the end
	if (txt_buf_tmp = strstr(txt_buf, "t making the"))
	{
		length = txt_buf_tmp-txt_buf;
		txt_buf[length]= '\0';
	}

	/* Write out buffer to a temporary file in case if user decides to change settings */
	dwSize = sizeof(temp);
	if (GetTempPath(dwSize, temp))
	{
		sprintf(pathBuf, "%s\\%s%s%s", temp, "Summary", iicode, ".rtf");
		handle = CreateFile(pathBuf, GENERIC_WRITE,FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle != INVALID_HANDLE_VALUE)
		{	
			dwSize = 0;
			WriteFile(handle, txt_buf, strlen(txt_buf), &dwSize, NULL);
			CloseHandle(handle);
			dwError = GetLastError();
		}
		
	}
	else
		return FALSE;

	while((point=strstr(txt_buf, title))!=NULL) 
	{

		point=point+sizeof(title)+2;

		dwSize=sizeof(setup_type);
		MsiGetProperty(hInstall, "_IsSetupTypeMin", setup_type, &dwSize);
		/* Places "Yes" next to components that were chosen to be installed. */
		if (strcmp(setup_type, "Custom") == 0 || strcmp(setup_type, "Complete") == 0 || 
			strcmp(setup_type, "Typical") == 0)
		{
			INSTALLSTATE iInstalled, iAction;
			
			if (strcmp(setup_type, "Custom") == 0)
				*setup_type='\0';		
			//Special case to take care of Vision package and Spatial objects
			counter = 0;
			while (strcmp(subcomponents[counter], "") && strcmp(subcomponent_names[counter], ""))
			{
			     if (!MsiGetFeatureState(hInstall, subcomponents[counter], &iInstalled, &iAction) 
				&& (iAction == INSTALLSTATE_LOCAL)) 
			     {
				if ((point=strstr(txt_buf, subcomponent_names[counter])) != NULL)
				{
					sprintf(name, "%s", subcomponent_names[counter]);
					point = point+strlen(name)+1;
					length = point - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point+sizeof(no);
					strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(no));
					txt_buf_tail[_msize(txt_buf)-length-sizeof(no)]='\0';
					memset((char *)txt_buf, 0, _msize(txt_buf));
					sprintf(txt_buf, "%s%s%s%c", txt_buf_head, yes, txt_buf_tail, '\0');
				}
			     }
			     counter++;
			}

		}

		counter = 0;
		while (strcmp(components[counter], "") && strcmp(component_names[counter], ""))
		{
			sprintf(component_name, "%s%s", components[counter], setup_type);
			dwSize=sizeof(component);
			MsiGetProperty(hInstall, component_name, component, &dwSize);
			if (strcmp(component, "1") == 0)
			{
				if ((point=strstr(txt_buf, component_names[counter])) != NULL)
				{
					sprintf(name, "%s", component_names[counter]);
					point = point+strlen(name)+1;
					length=point-txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point=point+sizeof(no);
					strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(no));
					txt_buf_tail[_msize(txt_buf)-length-sizeof(no)]='\0';
					memset((char *)txt_buf, 0, _msize(txt_buf));
					sprintf(txt_buf, "%s%s%s%c", txt_buf_head, yes, txt_buf_tail, '\0');
				}		
			}
			counter++;
		}
		break;
	}

	//Update Configuration type
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "CONFIG_TYPE", pathBuf, &dwSize);
	counter = 0;
	counter = atoi(pathBuf);
	/* for counter=0 (TXN) we don't need to do anything b/c it's a default.
	** for counter 1,2, or 3 we need to update.
	** anything outside this range is invalid.
	*/
	if (counter >= 0 && counter < 4) 
	{
		if (strcmp(config_types[counter], default_config_type) !=0 )
		{
			if ((point=strstr(txt_buf, config_type)) != NULL)
			{
				char *point2;
				point = point+sizeof(config_type)+1;
				point2 = strstr(point, default_config_type);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+strlen(default_config_type);
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(default_config_type)+1);
				if (strlen(config_types[counter]) > sizeof(default_config_type))
				{
					length = strlen(txt_buf)+strlen(config_types[counter])-sizeof(default_config_type);
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf, length);
						if (txt_buf_tmp == NULL && txt_buf)
						{
							free(txt_buf);
							txt_buf = malloc(length);
						}
						else
						{
							txt_buf=txt_buf_tmp;
							txt_buf_tmp = NULL;
						}
					}
				}
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, config_types[counter], txt_buf_tail, '\0');
				if (strlen(config_types[counter]) > sizeof(default_config_type))
				{
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf_head, length);
						if (txt_buf_tmp == NULL && txt_buf_head)
						{
							free(txt_buf_head);
							txt_buf_head = malloc(length);
						}
						else
							txt_buf_head = txt_buf_tmp;
						txt_buf_tail = realloc(txt_buf_tail, length);
						if (txt_buf_tmp == NULL && txt_buf_tail)
						{
							free(txt_buf_tail);
							txt_buf_tail = malloc(length);
						}
						else
							txt_buf_tail = txt_buf_tmp;
						txt_buf_tmp = NULL;
					}
				}
			}
		}
	}


	//Update Installation path
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INSTALLDIR", pathBuf, &dwSize);
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf_head) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
		szBuf=length;
	}
	replace_location_string(path, location, &txt_buf, txt_buf_head, txt_buf_tail, length);
	
	//Update Database Location
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_DATADIR", pathBuf, &dwSize);
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf_head) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
		szBuf=length;
	}
	replace_location_string(path, dblocation, &txt_buf, txt_buf_head, txt_buf_tail, length);

	
	//Update Backup Location
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_CKPDIR", pathBuf, &dwSize);
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
	}
	replace_location_string(path, backuplocation, &txt_buf, txt_buf_head, txt_buf_tail, length);

	//Update Journal Location
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_JNLDIR", pathBuf, &dwSize);
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
		szBuf=length;
	}
	replace_location_string(path, journallocation, &txt_buf, txt_buf_head, txt_buf_tail, length);
	
	//Update Dump Location
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_DMPDIR", pathBuf, &dwSize);
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
		szBuf=length;
	}
	replace_location_string(path, dumplocation, &txt_buf, txt_buf_head, txt_buf_tail, length);

	//Update Temporary Location
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_WORKDIR", pathBuf, &dwSize);
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
		szBuf=length;
	}
	replace_location_string(path, templocation, &txt_buf, txt_buf_head, txt_buf_tail, length);

	//Update Transaction Log Location
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_PRIMLOGDIR", pathBuf, &dwSize);
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
		szBuf=length;
	}
	replace_location_string(path, translocation, &txt_buf, txt_buf_head, txt_buf_tail, length);

	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_ENABLEDUALLOG", pathBuf, &dwSize);
	if (strcmp(pathBuf, "0") == 0 || strcmp(pathBuf, "") == 0)
		sprintf(pathBuf, "Disabled");
	else	
	{
		dwSize=sizeof(pathBuf);
		MsiGetProperty(hInstall, "INGRES_DUALLOGDIR", pathBuf, &dwSize);
	}
	path = duplicate_slashes(pathBuf);
	strncpy(pathBufD, path, strlen(path)+1);
	path=(char *)&pathBufD;
	length = szBuf;
	if (strlen(path)> strlen(default_location))
	{
		length = strlen(txt_buf)-strlen(default_location)+strlen(path)+1;
		strncpy(txt_buf_head, txt_buf, strlen(txt_buf)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf, length);
			if (txt_buf_tmp == NULL && txt_buf)
			{
				free(txt_buf);
				txt_buf = malloc(length);
			}
			else
			{
				txt_buf=txt_buf_tmp;
				txt_buf_tmp = NULL;
			}
		}
		strncpy(txt_buf, txt_buf_head, strlen(txt_buf_head)+1);
		if (_msize(txt_buf) < length)
		{
			txt_buf_tmp = realloc(txt_buf_head, length);
			if (txt_buf_tmp == NULL && txt_buf_head)
			{
				free(txt_buf_head);
				txt_buf_head = malloc(length);
			}
			else
				txt_buf_head = txt_buf_tmp;
			txt_buf_tail = realloc(txt_buf_tail, length);
			if (txt_buf_tmp == NULL && txt_buf_tail)
			{
				free(txt_buf_tail);
				txt_buf_tail = malloc(length);
			}
			else
				txt_buf_tail = txt_buf_tmp;
			txt_buf_tmp = NULL;
		}
		szBuf=length;
	}
	replace_location_string(path, translocation, &txt_buf, txt_buf_head, txt_buf_tail, length);

	//Update Ingres Instance Name
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "II_INSTALLATION", pathBuf, &dwSize);
	if (strcmp(pathBuf, default_instance) != 0)
	{
		if ((point=strstr(txt_buf, default_instance)) != NULL)
		{
			point = point+sizeof(default_instance)-3;
			point[0] = pathBuf[0];
			point[1] = pathBuf[1];
		}			
	}	

	//Update Transaction Log file Size
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_LOGFILESIZE", pathBuf, &dwSize);
	if (strcmp(pathBuf, default_log_size) !=0 )
	{
			if ((point=strstr(txt_buf, logsize_string)) != NULL)
			{
				char *point2;
				point = point+sizeof(logsize_string)-1;
				length=point-txt_buf;
				strncpy(txt_buf_head, txt_buf, length);
				txt_buf_head[length]='\0';
				point2 = strstr(point, default_log_size);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+strlen(default_log_size);
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(default_log_size)+1);
				if (strlen(pathBuf) > sizeof(default_log_size))
				{
					length = strlen(txt_buf)+strlen(pathBuf)-sizeof(default_log_size);
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf, length);
						if (txt_buf_tmp == NULL && txt_buf)
						{
							free(txt_buf);
							txt_buf = malloc(length);
						}
						else
						{
							txt_buf=txt_buf_tmp;
							txt_buf_tmp = NULL;
						}
					}			
				}
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, pathBuf, txt_buf_tail, '\0');
				if (strlen(pathBuf) > sizeof(default_log_size))
				{
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf_head, length);
						if (txt_buf_tmp == NULL && txt_buf_head)
						{
						free(txt_buf_head);
						txt_buf_head = malloc(length);
						}
						else
						txt_buf_head = txt_buf_tmp;
						txt_buf_tail = realloc(txt_buf_tail, length);
						if (txt_buf_tmp == NULL && txt_buf_tail)
						{
						free(txt_buf_tail);
						txt_buf_tail = malloc(length);
						}
						else
						txt_buf_tail = txt_buf_tmp;
						txt_buf_tmp = NULL;
					}
				}
			}
	}
	//Update Time Zone
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_TIMEZONE", pathBuf, &dwSize);
	if (strcmp(pathBuf, default_timezone) !=0 )
	{
			if ((point=strstr(txt_buf, timezone_string)) != NULL)
			{
				char *point2;
				point = point+sizeof(timezone_string)+1;
				point2 = strstr(point, default_timezone);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+strlen(default_timezone);
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(default_timezone)+1);
				if (strlen(pathBuf) > sizeof(default_timezone))
				{
					length = strlen(txt_buf)+strlen(pathBuf)-sizeof(default_timezone);
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf, length);
						if (txt_buf_tmp == NULL && txt_buf)
						{
							free(txt_buf);
							txt_buf = malloc(length);
						}
						else
						{
							txt_buf=txt_buf_tmp;
							txt_buf_tmp = NULL;
						}
					}
				}
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, pathBuf, txt_buf_tail, '\0');
				if (strlen(pathBuf) > sizeof(default_timezone))
				{
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf_head, length);
						if (txt_buf_tmp == NULL && txt_buf_head)
						{
							free(txt_buf_head);
							txt_buf_head = malloc(length);
						}
						else
							txt_buf_head = txt_buf_tmp;
						txt_buf_tail = realloc(txt_buf_tail, length);
						if (txt_buf_tmp == NULL && txt_buf_tail)
						{
							free(txt_buf_tail);
							txt_buf_tail = malloc(length);
						}
						else
							txt_buf_tail = txt_buf_tmp;
						txt_buf_tmp = NULL;
					}
				}
			}
	}
	
	//Update Character Set
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_CHARSET", pathBuf, &dwSize);
	if (strcmp(pathBuf, default_charset) !=0 )
	{
			if ((point=strstr(txt_buf, charset_string)) != NULL)
			{
				point = point+sizeof(charset_string)+1;
				point2 = strstr(point, default_charset);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+strlen(default_charset);
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(default_charset)+1);
				if (strlen(pathBuf) > sizeof(default_charset))
				{
					length = strlen(txt_buf)+strlen(pathBuf)-sizeof(default_charset);
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf, length);
						if (txt_buf_tmp == NULL && txt_buf)
						{
							free(txt_buf);
							txt_buf = malloc(length);
						}
						else
						{
							txt_buf=txt_buf_tmp;
							txt_buf_tmp = NULL;
						}
					}

				}
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, pathBuf, txt_buf_tail, '\0');
				if (strlen(pathBuf) > sizeof(default_charset))
				{
					if (_msize(txt_buf) < length)
					{
						txt_buf_tmp = realloc(txt_buf_head, length);
						if (txt_buf_tmp == NULL && txt_buf_head)
						{
							free(txt_buf_head);
							txt_buf_head = malloc(length);
						}
						else
							txt_buf_head = txt_buf_tmp;
						txt_buf_tail = realloc(txt_buf_tail, length);
						if (txt_buf_tmp == NULL && txt_buf_tail)
						{
							free(txt_buf_tail);
							txt_buf_tail = malloc(length);
						}
						else
							txt_buf_tail = txt_buf_tmp;
						txt_buf_tmp = NULL;
					}
				}
			}
	}

	//Update Date Type Alias
	dwSize = sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_DATE_TYPE", pathBuf, &dwSize);
	if (strcmp(pathBuf, "INGRES") != 0)
	{
		if ((point = strstr(txt_buf, date_type_alias)) != NULL)
		{
			char *point2;
			point = point+sizeof(date_type_alias)+1;
			point2 = strstr(point, default_date_type);
			if (point2 != NULL)
			{
				length = point2 - txt_buf;
				strncpy(txt_buf_head, txt_buf, length);
				txt_buf_head[length]='\0';
				point = point2;
			}
			point = point+sizeof(default_date_type)-1;
			strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(default_date_type)/*+1*/);
			if (strlen(pathBuf) > sizeof(default_date_type))
			{
				length = strlen(txt_buf)+strlen(pathBuf)-sizeof(default_date_type);
				if (_msize(txt_buf) < length)
				{
					txt_buf_tmp = realloc(txt_buf, length);
					if (txt_buf_tmp == NULL && txt_buf)
					{
						free(txt_buf);
						txt_buf = malloc(length);
					}
					else
					{
						txt_buf=txt_buf_tmp;
						txt_buf_tmp = NULL;
					}
				}
			}
			memset((char *)txt_buf, 0, _msize(txt_buf));
			sprintf(txt_buf, "%s%s%s%c", txt_buf_head, pathBuf, txt_buf_tail, '\0');
		}
	}

	//Update Strict SQL-92 Compliance
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_ENABLESQL92", pathBuf, &dwSize);
	if (strcmp(pathBuf, "") !=0 )
	{
			if ((point=strstr(txt_buf, sqlcompliance_string)) != NULL)
			{
				char *point2;
				point = point+sizeof(sqlcompliance_string)+1;
				point2 = strstr(point, no);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+sizeof(no)-1;
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(no)+1);
				length = strlen(txt_buf)+1;
				if (_msize(txt_buf) < length)
				{	
					txt_buf_tmp = realloc(txt_buf, length);
					if (txt_buf_tmp == NULL && txt_buf)
					{
						free(txt_buf);
						txt_buf = malloc(length);
					}
					else
					{
						txt_buf=txt_buf_tmp;
						txt_buf_tmp = NULL;
					}
				}
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, yes, txt_buf_tail, '\0');
				if (_msize(txt_buf) < length)
				{
				txt_buf_head = realloc(txt_buf_head, length);
				txt_buf_tail = realloc(txt_buf_tail, length);
				}
				
			}
	}

	//Update "Include Ingres in PATH, INCLUDE and LIB" parameter
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_INCLUDEINPATH", pathBuf, &dwSize);
	if (strcmp(pathBuf, "1") !=0 )
	{
			if ((point=strstr(txt_buf, ingresinpath_string)) != NULL)
			{
				char *point2;
				point = point+sizeof(ingresinpath_string);
				point2 = strstr(point, yes);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+sizeof(yes)-1;
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(yes)+1);
				txt_buf_tail[_msize(txt_buf)-length-sizeof(yes)+1] = '\0';
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, no, txt_buf_tail, '\0');
			}
	}

	//Update "Create Demonstration Database" parameter
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_INSTALLDEMODB", pathBuf, &dwSize);
	if (strcmp(pathBuf, "1") !=0 )
	{
			if ((point=strstr(txt_buf, demodb_string)) != NULL)
			{
				char *point2;
				point = point+sizeof(demodb_string)+1;
				point2 = strstr(point, yes);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+sizeof(yes)-1;
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(yes)+1);
				txt_buf_tail[_msize(txt_buf)-length-sizeof(yes)+1]='\0';
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, no, txt_buf_tail, '\0');
			}
	}
	//Update "Read-Only ODBC Driver" parameter
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_ODBC_READONLY", pathBuf, &dwSize);
	if (strcmp(pathBuf, "") != 0 )
	{
		if ((point=strstr(txt_buf, readonly_odbc_string)) != NULL)
		{
			char *point2;
			point = point+sizeof(readonly_odbc_string)+1;
			point2 = strstr(point, no);
			if (point2 != NULL)
			{
				length = point2 - txt_buf;
				strncpy(txt_buf_head, txt_buf, length);
				txt_buf_head[length]='\0';
				point=point2;
			}
			point=point+sizeof(no)-1;
			strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(no)+1);
			txt_buf_tail[_msize(txt_buf)-length-sizeof(no)+1]='\0';
			memset((char *)txt_buf, 0, _msize(txt_buf));
			sprintf(txt_buf, "%s%s%s%c", txt_buf_head, yes, txt_buf_tail, '\0');
		}
	}

	//Update "Service Start Auto" parameter
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_SERVICEAUTO", pathBuf, &dwSize);
	if (strcmp(pathBuf, "1") !=0 )
	{
			if ((point=strstr(txt_buf, serviceauto_string)) != NULL)
			{
				char *point2;
				point = point+sizeof(serviceauto_string)+1;
				point2 = strstr(point, yes);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+sizeof(yes)-1;
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(yes)+1);
				txt_buf_tail[_msize(txt_buf)-length-sizeof(yes)+1] = '\0';
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, no, txt_buf_tail, '\0');
			}
	}

	//Update Service User
	dwSize=sizeof(pathBuf);
	MsiGetProperty(hInstall, "INGRES_SERVICEUSER", pathBuf, &dwSize);
	if (strcmp(pathBuf, default_service_user) !=0 )
	{
			if ((point=strstr(txt_buf, serviceuser_string)) != NULL)
			{
				char *point2;
				point = point+sizeof(serviceuser_string)+1;
				point2 = strstr(point, default_service_user);
				if (point2 != NULL)
				{
					length = point2 - txt_buf;
					strncpy(txt_buf_head, txt_buf, length);
					txt_buf_head[length]='\0';
					point = point2;
				}
				point=point+sizeof(default_service_user);
				strncpy(txt_buf_tail, point, _msize(txt_buf)-length-sizeof(default_service_user)+1);
				txt_buf_tail[_msize(txt_buf)-length-sizeof(default_service_user)+1]='\0';
				memset((char *)txt_buf, 0, _msize(txt_buf));
				sprintf(txt_buf, "%s%s%s%c", txt_buf_head, secondary_service_user, txt_buf_tail, '\0');
			}
	}


	if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiRecordSetString(hRec, 10, txt_buf) == ERROR_SUCCESS))
		return FALSE;
	
	if (!((dwError = MsiViewModify(hView, MSIMODIFY_INSERT_TEMPORARY, hRec)) == ERROR_SUCCESS))
 		return FALSE;


	if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
		return FALSE;
		/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;
	
	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	    return FALSE;


	if (GetTempPath(dwSize, temp))
	{
		sprintf(pathBuf, "%s\\%s%s%s", temp, "Summary", iicode, "-final.rtf");
		handle = CreateFile(pathBuf, GENERIC_WRITE,FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle != INVALID_HANDLE_VALUE)
		{	
			dwSize = 0;
			WriteFile(handle, txt_buf, szBuf, &dwSize, NULL);
			CloseHandle(handle);
			dwError = GetLastError();
		}
		
	}
	else
		return FALSE;

	if (txt_buf != NULL)
		free(txt_buf);
	if (txt_buf_head!=NULL)
		free(txt_buf_head);
	if (txt_buf_tail!=NULL)
		free(txt_buf_tail);
	

	return ERROR_SUCCESS;

}


/*
**  Name: ingres_cleansummary
**
**  Description: This function is called when user clicks Back button
**				 on ReadyToInstall dialog. It's a cleanup function that
**				 retrieves backedup template of RTF summary file and
**				 stores it in the binary table of MSI in preparations
**				 for ingres_addsummary function.
**
**
**  History:
**	06-Oct-2006 (drivi01)
**	    Created.
*/
UINT __stdcall
ingres_cleansummary(MSIHANDLE hInstall)
{
	HANDLE handle;
	char temp[MAX_PATH+1], pathBuf[MAX_PATH+1], cmd[MAX_PATH+1];
	char *txt_buf;
	DWORD dwSize, szBuf, dwError;
	MSIHANDLE msih, hView, hRec;
	char iicode[3];

	dwSize = sizeof(iicode);
	MsiGetProperty(hInstall, "II_INSTALLATION", iicode, &dwSize);
	dwSize = sizeof(temp);
	if (GetTempPath(dwSize, temp))
	{
		sprintf(pathBuf, "%s\\%s%s%s", temp, "Summary", iicode, ".rtf");
		handle = CreateFile(pathBuf, GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle != INVALID_HANDLE_VALUE)
		{	
			dwSize = 0;
			szBuf=0;
			if ((szBuf = GetFileSize(handle, NULL)) != 0xFFFFFFFF)
			{
				txt_buf = malloc(szBuf);
				while (ReadFile(handle, txt_buf, szBuf, &dwSize, NULL) == ERROR_INSUFFICIENT_BUFFER)
				{
					if (txt_buf)
						free(txt_buf);
					txt_buf = malloc(szBuf * 2);
				}
			}
			CloseHandle(handle);
			dwError = GetLastError();
		}
		
	}
	else
		return FALSE;



	msih = MsiGetActiveDatabase(hInstall);
	if (!msih)
		return FALSE;
	sprintf(cmd, "SELECT * FROM Control WHERE Dialog_ = 'ReadyToInstall' AND Control = 'ScrollableText1'");
	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
		return FALSE;

//////

	if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiRecordSetString(hRec, 10, txt_buf) == ERROR_SUCCESS))
		return FALSE;
	
	if (!((dwError = MsiViewModify(hView, MSIMODIFY_INSERT_TEMPORARY, hRec)) == ERROR_SUCCESS))
 		return FALSE;


	if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
		return FALSE;
		/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;
	
	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	    return FALSE;

	return ERROR_SUCCESS;
}


/*
**  Name: ingres_removesummary
**
**  Description: This function removes generated Summary file that reflects
**				 current settings of the installation. This is just a clean
**				 up function for when the file is not needed.
**
**
**  History:
**	06-Oct-2006 (drivi01)
**	    Created.
*/
UINT __stdcall
ingres_removesummary(MSIHANDLE hInstall)
{
	char temp[MAX_PATH+1], pathBuf[MAX_PATH+1];
	DWORD dwSize;
	char iicode[3];

	dwSize = sizeof(iicode);
	MsiGetProperty(hInstall, "II_INSTALLATION", iicode, &dwSize);
	dwSize = sizeof(temp);
	if (GetTempPath(dwSize, temp))
	{
		sprintf(pathBuf, "%s\\%s%s%s", temp, "Summary", iicode, "-final.rtf");
		if (_access(pathBuf, 00) == 0)
			DeleteFile(pathBuf);
	}	
	else
		return FALSE;

	return TRUE;
}

/*
**  Name: ingres_install_documentation
**
**  Description: This function is responsible for launching documentation
**				 installer. (Called in Interactive Mode only)
**				 This function is used by Ingres installer only.
**
**
**  History:
**	06-Oct-2006 (drivi01)
**	    Created.
**	16-Jun-2008 (drivi01)
**	    Update name of msi for documentation image.
**	    Update documentation product code for release 9.2.
**	14-Nov-2008 (drivi01)
**	    Pull the version # from commonmm.h.
**	    Retrieve documentation product code from the 
**	    documentation package to check if the product 
**	    requires an upgrade or if it's a fresh install.
**		
*/
UINT __stdcall 
ingres_install_documentation(MSIHANDLE hInstall)
{
	char iicode[3];
	DWORD dwSize, dwError;
	int idx;
	char guid[MAX_PATH], buf[MAX_PATH+1], cmd[MAX_PATH+1], cmd2[MAX_PATH+1], installdir[MAX_PATH+1];
	HKEY hRegKey;
	WIN32_FIND_DATA	wfd;
	HANDLE handle;
	int bUpgrade=0; /*if registry entry already exists then this is treated as upgrade*/

	dwSize=sizeof(installdir);
	MsiGetProperty(hInstall, "INGRESCORPFOLDER", installdir, &dwSize);
	*buf=0; dwSize=sizeof(buf);
	if (MsiGetProperty(hInstall, "INGRES_CDIMAGE", buf, &dwSize) || !buf[0] || !strcmp(buf, "0"))
	{
	dwSize=sizeof(iicode);
	MsiGetProperty(hInstall, "II_INSTALLATION", iicode, &dwSize);
	
	idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - 'A';
	if (idx <= 0)
	    idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - '0';
	sprintf (guid, "{A78D%04X-2979-11D5-BDFA-00B0D0AD4485}", idx);

	//find out path to cdimage by quering ingres product code registry entry
	sprintf (buf, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", guid);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	    dwSize=sizeof(buf); *buf=0;
	    RegQueryValueEx(hRegKey, "Cdimage", NULL, NULL, (BYTE *)buf, &dwSize);
	    RegCloseKey(hRegKey);
	}
	}

	if (buf[strlen(buf)-1]=='\\') buf[strlen(buf)-1]='\0';
	sprintf(cmd, "%s\\files\\documentation\\*.msi", buf);
	if ((handle = FindFirstFile(cmd, &wfd)) != INVALID_HANDLE_VALUE)
	{
		sprintf(cmd, "%s\\files\\documentation\\%s", buf, wfd.cFileName);
		if (GetProductCode(cmd, guid, sizeof(guid)) != ERROR_SUCCESS)
			*guid='\0';

	}
	
	if (*guid)
	{
	sprintf(cmd2, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", guid);
	//if product code for documentation is already in the registry then this is an upgrade scenario.
	//Otherwise fresh install.
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, cmd2, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	     bUpgrade=1;
	     RegCloseKey(hRegKey);
	}
	}
	if (handle != INVALID_HANDLE_VALUE)
	{
	     if (!bUpgrade)
	     	sprintf(cmd, "MsiExec.exe /i \"%s\\files\\documentation\\%s\" /qb INGRESCORPFOLDER=\"%s\"", buf, wfd.cFileName, installdir);
	     else
		sprintf(cmd, "MsiExec.exe /i \"%s\\files\\documentation\\%s\" /qb REINSTALL=ALL REINSTALLMODE=vomus", buf, wfd.cFileName);
	     FindClose(handle);
	}
	else
	{
		dwError=GetLastError();
	     if (!bUpgrade)
	          sprintf(cmd, "MsiExec.exe /i \"%s\\files\\documentation\\Ingres Documentation %s.msi\" /qb INGRESCORPFOLDER=\"%s\"", buf, installdir, ING_VERS);  
	     else
		  sprintf(cmd, "MsiExec.exe /i \"%s\\files\\documentation\\Ingres Documentation %s.msi\" /qb REINSTALL=ALL REINSTALLMODE=vomus", buf, ING_VERS);   
	}

	ExecuteEx(cmd);

	return ERROR_SUCCESS;
}

/*
**  Name: ingres_install_dotnet
**
**  Description: This function is responsible for launching .Net Data Provider
**				 installer. (Called in Interactive Mode only)
**				 This function is used by Ingres installer only.
**
**
**  History:
**	06-Oct-2006 (drivi01)
**	    Created.
**	31-Jul-2008 (drivi01)
**	    Update product code for .NET Data Provider 2.1.
**	    Update .NET Data Provider msi to reflect version change.
*/
UINT __stdcall 
ingres_install_dotnet(MSIHANDLE hInstall)
{
	char iicode[3];
	DWORD dwSize;
	int idx;
	char guid[MAX_PATH], buf[MAX_PATH+1], cmd[MAX_PATH+1], cmd2[MAX_PATH+1], installdir[MAX_PATH];
	HKEY hRegKey;
	WIN32_FIND_DATA	wfd;
	HANDLE handle;
	int bUpgrade=0;

	dwSize=sizeof(installdir);
	MsiGetProperty(hInstall, "INGRESCORPFOLDER", installdir, &dwSize);
	*buf=0; dwSize=sizeof(buf);
	if (MsiGetProperty(hInstall, "INGRES_CDIMAGE", buf, &dwSize) || !buf[0] || !strcmp(buf, "0"))
	{
	dwSize=sizeof(iicode);
	MsiGetProperty(hInstall, "II_INSTALLATION", iicode, &dwSize);
	
	idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - 'A';
	if (idx <= 0)
	    idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - '0';
	sprintf (guid, "{A78D%04X-2979-11D5-BDFA-00B0D0AD4485}", idx);

	//find out path to cdimage by quering ingres product code registry entry
	sprintf (buf, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", guid);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	    dwSize=sizeof(buf); *buf=0;
	    RegQueryValueEx(hRegKey, "Cdimage", NULL, NULL, (BYTE *)buf, &dwSize);
	    RegCloseKey(hRegKey);
	}
	}

	if (buf[strlen(buf)-1]=='\\') buf[strlen(buf)-1]='\0';
	sprintf(cmd, "%s\\files\\dotnet\\*.msi", buf);
	sprintf(guid,"{E4F5FEFF-AF01-4D35-B245-68D47C1ACA6A}"); //current product code for .NET Data Provider
	sprintf(cmd2, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", guid);
	//if product code for documentation is already in the registry then this is an upgrade scenario.
	//Otherwise fresh install.
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, cmd2, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	    bUpgrade=1;
	    RegCloseKey(hRegKey);
	}
	if ((handle = FindFirstFile(cmd, &wfd)) != INVALID_HANDLE_VALUE)
	{
	     if (!bUpgrade)
		 sprintf(cmd, "MsiExec.exe /i \"%s\\files\\dotnet\\%s\" /qb INGRESCORPFOLDER=\"%s\"", buf, wfd.cFileName, installdir);
	     else
		 sprintf(cmd, "MsiExec.exe /i \"%s\\files\\dotnet\\%s\" /qb REINSTALL=ALL REINSTALLMODE=vamus", buf, wfd.cFileName);
	     FindClose(handle);
	}
	else
	     if (!bUpgrade)
		 sprintf(cmd, "MsiExec.exe /i \"%s\\files\\dotnet\\Ingres .NET Data Provider 2.1.msi\" /qn INGRESCORPFOLDER=\"%s\"", buf, installdir);  
	     else
	     sprintf(cmd, "MsiExec.exe /i \"%s\\files\\dotnet\\Ingres .NET Data Provider 2.1.msi\" /qn REINSTALL=ALL REINSTALLMODE=vamus", buf);  

	ExecuteEx(cmd);

	return ERROR_SUCCESS;
}

/*
**  Name: ingres_dbms_dotnet
**
**  Description: This function sets the INGRES_DBMS_DOTNET property to "TRUE" 
**				 when Ingres DBMS feature and Ingres .NET Data Provider have
**				 been selected for installation.  This function is only called
**				 from Ingres installer and only called in the interactive mode.
**				 The purpose of this property is to figure out if CSharp 
**				 Frequent Flyer Demo needs to be installed, since it is only
**				 installed when both features are selected for installation.
**
**
**  History:
**	06-Oct-2006 (drivi01)
**	    Created.
*/
UINT __stdcall 
ingres_dbms_dotnet(MSIHANDLE hInstall)
{
	
	INSTALLSTATE iInstall;
	INSTALLSTATE iAction;
	if (MsiGetFeatureState(hInstall, "IngresDBMS", &iInstall, &iAction) == ERROR_SUCCESS)
	{

		if ((iInstall == INSTALLSTATE_LOCAL && iAction != INSTALLSTATE_ABSENT) || iAction == INSTALLSTATE_LOCAL)
	    {
			if (MsiGetFeatureState(hInstall, "IngresDotNETDataProvider", &iInstall, &iAction) == ERROR_SUCCESS)
		    {
				if ((iInstall == INSTALLSTATE_LOCAL && iAction != INSTALLSTATE_ABSENT) || iAction == INSTALLSTATE_LOCAL)
					MsiSetProperty(hInstall, "INGRES_DBMS_DOTNET", "TRUE");
			}
		}
	}

	return ERROR_SUCCESS;

}


/*
**  Name: ingres_set_corpdir
**
**  Description: This function takes installation directory and uses it to determine 
**		 location one directory up to display in the summary, that's where
**		 the rest of the products will be installed.
**
**
**  History:
**	13-Mar-2007 (drivi01)
**	    Created.
*/
UINT __stdcall
ingres_set_corpdir(MSIHANDLE hInstall)
{
	char installdir[MAX_PATH];
	char corpdir[MAX_PATH];
	DWORD dwSize;
	char *tokens[MAX_PATH];
	char *token;
	char seps[] = "\\";
	int i=0;
	int count=0;

	dwSize=sizeof(installdir);
	if (MsiGetProperty(hInstall, "INGRESFOLDER", installdir, &dwSize) == ERROR_SUCCESS)
	{
		token = strtok(installdir, seps);
		while( token != NULL)
		{
		 	tokens[count]=token;
			token = strtok(NULL, seps);
			count++;
		}
		*corpdir='\0';
		while (i< (count -1))
		{
			strcat(corpdir, tokens[i]);
			strcat(corpdir, seps);
			i++;
		}
		MsiSetTargetPath(hInstall, "INGRESCORPFOLDER", corpdir);
	}

	return ERROR_SUCCESS;
}


/*
**  Name: ingres_set_installlvl_doc
**
**  Description: This function disables IngresDoc feature so it doesn't appear in 
**		 the list of features.
**
**  History:
**	30-Oct-2007 (drivi01)
**	    Created.
*/
UINT __stdcall
ingres_set_installlvl_doc(MSIHANDLE hInstall)
{
	char cmd[MAX_PATH+1];
	MSIHANDLE msih, hView, hRec;
	DWORD dwError;

	msih = MsiGetActiveDatabase(hInstall);
	if (!msih)
		return FALSE;
	sprintf(cmd, "SELECT * FROM Feature WHERE Feature = 'IngresDoc'");
	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
		return FALSE;

//////

	if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiRecordSetString(hRec, 5, "0") == ERROR_SUCCESS))
		return FALSE;
	
	if (!((dwError = MsiViewModify(hView, MSIMODIFY_INSERT_TEMPORARY, hRec)) == ERROR_SUCCESS))
 		return FALSE;


	if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
		return FALSE;
		/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;
	
	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	    return FALSE;
	
	return ERROR_SUCCESS;

}


/*
**  Name: ingres_set_installlvl_dotnet
**
**  Description: This function disables IngresDotNETDataProvider
**	         feature so it doesn't appear in the list of features.
**
**  History:
**	30-Oct-2007 (drivi01)
**	    Created.
*/
UINT __stdcall
ingres_set_installlvl_dotnet(MSIHANDLE hInstall)
{
	char cmd[MAX_PATH+1];
	MSIHANDLE msih, hView, hRec;	
	DWORD	dwError;

	msih = MsiGetActiveDatabase(hInstall);
	if (!msih)
		return FALSE;
	sprintf(cmd, "SELECT * FROM Feature WHERE Feature = 'IngresDotNETDataProvider'");
	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
		return FALSE;

//////

	if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiRecordSetString(hRec, 5, "0") == ERROR_SUCCESS))
		return FALSE;
	
	if (!((dwError = MsiViewModify(hView, MSIMODIFY_INSERT_TEMPORARY, hRec)) == ERROR_SUCCESS))
 		return FALSE;


	if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
		return FALSE;
		/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;
	
	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	    return FALSE;
	
	return ERROR_SUCCESS;
}


/*
**  Name:ingres_warn_remove_dbms
**
**  Description:
**	Display a message to the user warning them about consequences of unselecting DBMS component.
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
**	16-Mar-2009 (drivi01)
**	    Created.
*/
UINT __stdcall
ingres_warn_remove_dbms(MSIHANDLE hInstall)
{

	BOOL bAnswer;
	DWORD dwSize=0;
	char buf[MAX_PATH];

	bAnswer = AskUserYN(hInstall, "Unselecting the DBMS component will delete all databases and checkpoints.  Are you sure you want to continue?");
	if (bAnswer)
	{
		dwSize=sizeof(buf);
		MsiSetProperty(hInstall, "INGRES_CONTINUE", "YES");
	}
	else
	{
		MsiSetProperty(hInstall, "INGRES_CONTINUE", "NO");
	}

	return ERROR_SUCCESS;
	
}
