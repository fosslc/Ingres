/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: PreInstallation.cpp: implementation of the CPreInstallation class.
**
**  History:
**	05-Jun-2001 (penga03)
**	    Created.
**	06-jun-2001 (somsa01)
**	    Update more properties which use the installation identifier.
**	07-Jun-2001 (penga03)
**	    In function LaunchWindowsInstaller, before launching
**	    msiexec.exe, check if Windows installer is currently installed
**	    on the target machine. If it is not installed, run
**	    InstMsiW.exe (for Windows NT / 2000) or InstMsiA.exe (for
**	    Windows 9x) to install windows installer on the target
**	    machine. 
**	    Note that: Windows Installer installation on
**	    Windows NT / 2000 requires administrative priviledge.
**	14-Jun-2001 (penga03)
**	    Changed the name of the cabinet file copied to the temp
**	    directory to the same name as the MSI file.
**	14-Jun-2001 (penga03)
**	    Save the directory where MSI and Cabinet files are located
**	    in property INGRES_MSI_LOC.
**	15-Jun-2001 (penga03)
**	    If Ingres is already installed, copy the MSI and Cabinet files
**	    to the temp directory defined by II_TEMPORARY, otherwise,
**	    copy to the temp directory defined by the system.
**	15-Jun-2001 (penga03)
**	    Save the path of license files to property INGRES_LIC_LOC.
**	23-July-2001 (penga03)
**	    Do some preparations so that the user can upgrade the
**	    installation installed by old installer. If we checked there
**	    exists an installation installed by old installar, and 
**	    user wants to upgrade this installation, set INGRES_VER25 to
**	    "1" and pass this property to MsiExec. In addition, if an
**	    Ingres package was installed, change the attribute of
**	    corresponding feature to be Required so that the user cannot
**	    remove this feature during upgrade. Finally, after upgrade is  
**	    done, we change features' attributes back to default in both
**	    MSI and cached MSI.
**	15-aug-2001 (somsa01)
**	    Added spaces between the installation identifier and '[]'.
**	    Also, update the IVM Startup folder shortcut with the proper
**	    II_INSTALLATION.
**	17-Aug-2001 (penga03)
**	    We took away the function that set the feature attributes during an 
**	    upgrade from pre-installation process, and created a custom action 
**	    to achieve this.
**	23-Aug-2001 (penga03)
**	    Added UpdateComponentId(), that changes the component GUID
**	    to completely separate the components between different
**	    installations.
**	04-oct-2001 (somsa01)
**	    Changed the name of the default Cabinet file.
**	24-Oct-2001 (penga03)
**	    Windows Installer used to register components in the following
**	    key HKLY\Software\Microsoft\Windows\Currentversion\Installer\
**	    Components, however, after I reinstalled OS and ugraded Windows
**	    Installer from 1.0 to 2.0, the key doesn't exist any more. As
**	    a result, we can't share IngresDoc among Ingres installations.
**	    In UpdateComponentId(), we have to change the components GUID
**	    of IngresDoc to separate the components between different
**	    installations.
**	05-nov-2001 (penga03)
**	    Created a new class CInstallation to save the installation id,
**	    installation path (i.e. II_SYSTEM) of an existing Ingres
**	    installation. This class also has a BOOL variable that
**	    indicates whether the installation needs to be upgrade or not.
**	    Added a new member function FindOldInstallation(), and a new
**	    member variable m_Installations to find and save all the
**	    existing installations. To find existing Ingres installations,
**	    func FindOldInstallation first checks HKLM\\SOFTWARE\\
**	    ComputerAssociates\\IngresII, then checks the environment 
**	    variable II_SYSTEM under both HKLM\\SYSTEM\\CurrentControlSet\\
**	    Control\\SessionManager\\Environment, and HKCU\\Environment,
**	    finally checks II_SYSTEM may defined by user in command 
**	    line by using func GetEnvironmentVariable. If there exists an
**	    installation that doesn't has a key registred in HKLM\\
**	    SOFTWARE\\ComputerAssociates\\IngresII, a key will be created
**	    for it.
**	08-nov-2001 (somsa01)
**	    Made changes corresponding to the new CA branding.
**	12-nov-2001 (somsa01)
**	    Make sure we compare the version of the Windows Installer, as
**	    there are multiple versions of it now. Also, cleaned up 64-bit
**	    warnings.
**	31-dec-2001 (penga03)
**	    Do not set the property INGRES_LICENSE_CHECKED any more, since 
**	    during setup we will always install Ingres license instead of 
**	    checking the license.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	30-aug-2002 (penga03)
**	    Added a new command line option, /l, so that a verbose log 
**	    file will be generated by Windows Installer. The log file 
**	    will be placed in the system temp directory.
**	17-jan-2003 (penga03)
**	    Using MsiCloseHandle to close the handles that are opened by 
**	    MsiDatabaseOpenView or MsiViewFetch.
**	06-feb-2004 (somsa01)
**	    Use WinVer.h instead of version.h.
**	19-feb-2004 (penga03)
**	    Added installing Microsoft redistributions.
**	07-jun-2004 (penga03)
**	    Peform a completely silent install if using response file, and also
**	    for this case install.exe will wait until the installing completes.
**	17-jun-2004 (somsa01)
**	    In LaunchWindowsInstaller(), if we're dealing with 64-bit
**	    Windows, launch and wait for the licensing install here before
**	    running the main Ingres installer. A Merge Module version of
**	    licensing will not be available in time for this product to go GA,
**	    and since we cannot run child msi's from a parent msi, we need to
**	    run the licensing install outside of the main install.
**	21-jun-2004 (penga03)
**	    Corrected the error introduced by the change 468841.
**	15-jul-2004 (penga03)
**	    Set the temporary cab and msi files writable.
**	16-jul-2004 (penga03)
**	    If /nomdb, pass '0' to the property INGRES_INSTALL_MDB.
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	03-aug-2004 (penga03)
**	    Pass the default value of INGRES_INSTALL_MDB while launching 
**	    MsiExec.exe.
**	10-sep-2004 (penga03)
**	    Removed MDB.
**	14-sep-2004 (penga03)
**	    Replaced "Ingres Enterprise Edition" with "Ingres".
**	22-sep-2004 (penga03)
**	    Removed installing license and changed licloc to cdimage, 
**	    INGRES_LIC_LOC to INGRES_CDIMAGE.
**	27-sep-2004 (penga03)
**	    Moved launching iipostinst from main setup to preinstall if 
**	    install Ingres using response file (ie. silently).
**	17-nov-2004 (penga03)
**	    Call iipostinst.exe only if PostInstallationNeeded is set 
**	    to "YES".
**	06-dec-2004 (penga03)
**	    Clean up LaunchWindowsInstaller() and changed the formulation to
**	    generate GUIDs since the old one doesn't work for id from A1 to A9.
**	10-dec-2004 (penga03)
**	    Corrected the error introduced by last change. Should pass the 
**	    root of temp directory, instead of the full temp directory to 
**	    GetDiskFreeSpace().
**	13-dec-2004 (penga03)
**	    Added the ability to upgrade a MSI-version of Ingres to a
**	    MSI-version of Ingres.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
**	11-jan-2005 (penga03)
**	    Pass the upgrade type (1, minor upgrade; 2, major upgrade) to the 
**	    setup msi.
**	11-feb-2005 (penga03)
**	    When copying the cab file to the temp directory, try 10 times.
**	    This will allow recovery if the file copy fails due to the bug in 
**	    VMware. Related issue is 13776102.
**	16-feb-2005 (penga03)
**	    Use GetFileSizeEx and GetDiskFreeSpaceEx to retrieve a file size
**	    and a disk space that are larger than a DWORD value.
**	28-feb-2005 (penga03)
**	    Restart Ingres if necessary on re-install.
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	01-apr-2005 (penga03)
**	    Made a change so that we always do major upgrade.
**	23-jun-2005 (penga03)
**	    Made changes to get the exit code from msiexec.exe.
**	24-jun-2005 (penga03)
**	    Modified LaunchWindowsInstaller() to run setupmdb.bat
**	    for silent install.
**	13-jul-2005 (penga03)
**	    Corrected the error in LaunchWindowsInstaller() while puting MDB 
**	    start/complete message in the install.log.
**	28-jul-2005 (penga03)
**	    Add return code to the log file.
**	10-aug-2005 (penga03)
**	    Made following changes in LaunchWindowsInstaller(): 
**	    (1) close all opened handles before exit;
**	    (2) instead of using GetExitCodeProcess to get the exit code of 
**	    setupmdb.bat, we get its exit code from the Ingres symbol table, 
**	    in which setupmdb.bat sets MDB_RC and MDB_RC_MSG.
**	12-aug-2005 (penga03)
**	    Set return code to GetLastError() in GetExitCodeProcess or
**	    CreateProcess fails. 
**	17-aug-2005 (penga03)
**	    Before closing the handles created for setupmdb.bat, check if 
**	    they are empty.
**	18-aug-2005 (penga03) 
**	    Do not write the return code to the install.log in 
**	    LaunchWindowsInstaller().
**	01-Mar-2006 (drivi01)
**	    Reused and adopted for MSI Patch Installer on Windows.
** 	    LaunchWindowsInstaller was replaced with LaunchPatchInstaller
**	    from PatchUpdate.c binary which will be now deleted from
**	    patch_win directory in front/st.
**	25-Apr-2006 (drivi01)
**	    Porting patch installer to Ingres 2006.
**	07-jun-2006 (drivi01)
**	    Moved a popup error message to an if block to avoid it if
**	    we're in silent mode. Updated msi database update query
**	    to replace ingres directory name with no spaces.
**	16-jun-2006 (drivi01)
**   	    Updated PATCH_SIZE to 10 characters due to the _SOL suffix
**	    that need to be added for SOL patches.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "PreInstallation.h"
#include <msiquery.h>
#include <Winsvc.h>
#include <WinVer.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

char nfname[MAX_PATH+1], efname[MAX_PATH+1], ndcab[MAX_PATH+1], edcab[MAX_PATH+1];
CStringList CodeList;
time_t t;

void AppendToLog(LPCSTR p);
BOOL setupii_edit(char *iicode, char *path);
BOOL setupii_vexe(MSIHANDLE hDatabase, char *szQuery, char *szValue=0);
DWORD ThreadCopyCab(LPVOID lpParameter);
BOOL check_windowsinstaller(void);
BOOL IsAdmin(void);
BOOL UpdateComponentId(MSIHANDLE hDatabase, int id);
INT CompareVersion(char *file1, char *file2);
INT CompareIngresVersion(char *ii_system);
VOID BoxErr4 (CHAR *MessageString, CHAR *MessageString2);

HWND		hDlgGlobal;	/* Handle to Main Dialog */

#define II_SIZE	3
#define	PATCH_SIZE	10
#define BUFF_SIZE	1024
#define CKSUM_SIZE   64
#define	SHORT_BUFF	 32
#define SMALLER_BUFF 512

/*
** Construction/Destruction
*/

CPreInstallation::CPreInstallation()
: m_UpgradeType(0)
, m_RestartIngres(0)
{
    m_CreateResponseFile="0";
    m_EmbeddedRelease="0";
    m_InstallCode="";
    m_ResponseFile="";
    m_Ver25="0";
    m_MSILog="";
	m_SilentInstall=FALSE;

    FindOldInstallations();
}

CPreInstallation::~CPreInstallation()
{
    for(int i=0; i<m_Installations.GetSize(); i++)
    {
	CInstallation *inst=(CInstallation *) m_Installations.GetAt(i);

	if(inst)
	    delete inst;
    }
    m_Installations.RemoveAll();
}

/*
**  History:
**	05-nov-2001 (penga03)
**	    Created. 
**	28-jan-2001 (penga03)
**	    Also check the current Ingres registry key 
**	    "HKLM\\SOFTWARE\\ComputerAssociates\\Advantage Ingres".
**	31-jul-2002 (penga03)
**	    Modified FindOldInstallations() to double check each 
**	    Ingres instance found by checking the existence of its 
**	    config.dat. In addition, for each instance, determine 
**	    it was installed as embedded or enterprise by checking 
**	    whether or not the string 
**	    ii.<computer name>.setup.embed_installation is set to 
**	    ON in config.dat.
**	 21-oct-2002 (penga03)
**	    Add the installation id found to the CodeList only after 
**	    checking existence the corresponding config.dat.
*/
void 
CPreInstallation::FindOldInstallations()
{
    char CurKey[1024], Key[1024];
    HKEY hCurKey=0, hKey=0;
    char ii_code[3], ii_system[MAX_PATH], file2check[MAX_PATH];
    CStringList CodeList;
    CString Host, ConfigKey, ConfigKeyValue, embedded;
    char strBuffer[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize=sizeof(strBuffer);
    BOOL nonmsi;
	
    GetComputerName(strBuffer, &dwSize);
    Host=CString(strBuffer);
    Host.MakeLower();


	sprintf(CurKey, "SOFTWARE\\IngresCorporation\\Ingres");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,CurKey,0,KEY_ENUMERATE_SUB_KEYS,&hCurKey))
    {
	LONG retCode;
	int i=0;
	
	for (i, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwTemp=sizeof(SubKey);
	    
	    retCode=RegEnumKeyEx(hCurKey,i,SubKey,&dwTemp,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {			
		HKEY hSubKey=0;

		if(!RegOpenKeyEx(hCurKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    DWORD dwType, dwSize=sizeof(ii_system);
		    
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
		    {
			strncpy(ii_code, SubKey, 2);
			ii_code[2]='\0';
		
			sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
			if (GetFileAttributes(file2check)!=-1)
			{
			    CodeList.AddTail(ii_code);
				
			    ConfigKey.Format("ii.%s.setup.embed_installation", Host);
			    if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
			        && !ConfigKeyValue.CompareNoCase("ON"))
				embedded="1";
			    else
				embedded="0";

			    sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
			    if (GetFileAttributes(file2check)!=-1)
				nonmsi=1;
			    else 
				nonmsi=0;

			    AddInstallation(ii_code, ii_system, nonmsi, embedded);
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hCurKey);
    }

	sprintf(CurKey, "SOFTWARE\\ComputerAssociates\\Ingres");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;
	int i=0;

	for (i, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwTemp=sizeof(SubKey);
	    
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwTemp,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		strncpy(ii_code, SubKey, 2);
		ii_code[2]='\0';
		if(!CodeList.Find(ii_code))
		{
		    HKEY hSubKey=0;

		    if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		    {
			DWORD dwType, dwSize=sizeof(ii_system);
		    
			if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
			{				
			    sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
			    if (GetFileAttributes(file2check)!=-1)
			    {
				CodeList.AddTail(ii_code);
				
				ConfigKey.Format("ii.%s.setup.embed_installation", Host);
				if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
				    && !ConfigKeyValue.CompareNoCase("ON"))
				    embedded="1";
				else
				    embedded="0";
				
				sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
				if (GetFileAttributes(file2check)!=-1)
				    nonmsi=1;
				else 
				    nonmsi=0;
				
				AddInstallation(ii_code, ii_system, nonmsi, embedded);
				
			    }
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hKey);
    }

    sprintf(Key, "SOFTWARE\\ComputerAssociates\\Advantage Ingres");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;
	int i=0;

	for (i, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwTemp=sizeof(SubKey);
	    
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwTemp,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		strncpy(ii_code, SubKey, 2);
		ii_code[2]='\0';
		if(!CodeList.Find(ii_code))
		{
		    HKEY hSubKey=0;

		    if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		    {
			DWORD dwType, dwSize=sizeof(ii_system);
		    
			if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
			{				
			    sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
			    if (GetFileAttributes(file2check)!=-1)
			    {
				CodeList.AddTail(ii_code);
				
				ConfigKey.Format("ii.%s.setup.embed_installation", Host);
				if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
				    && !ConfigKeyValue.CompareNoCase("ON"))
				    embedded="1";
				else
				    embedded="0";
				
				sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
				if (GetFileAttributes(file2check)!=-1)
				    nonmsi=1;
				else 
				    nonmsi=0;
				
				AddInstallation(ii_code, ii_system, nonmsi, embedded);
				
			    }
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hKey);
    }
    
    sprintf(Key, "SOFTWARE\\ComputerAssociates\\IngresII");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_ENUMERATE_SUB_KEYS,&hKey))
    {
	LONG retCode;
	int i=0;

	for (i, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwTemp=sizeof(SubKey);
	    
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwTemp,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		strncpy(ii_code, SubKey, 2);
		ii_code[2]='\0';
		if(!CodeList.Find(ii_code))
		{
		    HKEY hSubKey=0;

		    if(!RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		    {
			DWORD dwType, dwSize=sizeof(ii_system);
		    
			if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
			{				
			    sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
			    if (GetFileAttributes(file2check)!=-1)
			    {
				CodeList.AddTail(ii_code);
				
				ConfigKey.Format("ii.%s.setup.embed_installation", Host);
				if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
				    && !ConfigKeyValue.CompareNoCase("ON"))
				    embedded="1";
				else
				    embedded="0";
				
				sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
				if (GetFileAttributes(file2check)!=-1)
				    nonmsi=1;
				else 
				    nonmsi=0;
				
				AddInstallation(ii_code, ii_system, nonmsi, embedded);
				
			    }
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hKey);
    }

    if (IsWindows9X())
	strcpy(Key, "SYSTEM\\CurrentControlSet\\Control\\SessionManager\\Environment");
    else
	strcpy(Key, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
    
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0,KEY_QUERY_VALUE, &hKey))
    {
	DWORD dwType, dwSize=sizeof(ii_system);
	
	if (!RegQueryValueEx(hKey, "II_SYSTEM", 0, &dwType, (BYTE *)ii_system, &dwSize))
	{	
	    CString temp;
	    
	    if (Local_NMgtIngAt("II_INSTALLATION", ii_system, temp) && !CodeList.Find(temp))
	    {
		strcpy(ii_code, temp.GetBuffer(2));

		ConfigKey.Format("ii.%s.setup.embed_installation", Host);
		if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
		    && !ConfigKeyValue.CompareNoCase("ON"))
		    embedded="1";
		else
		    embedded="0";

		sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
		if (GetFileAttributes(file2check)!=-1)
		{
		    CodeList.AddTail(ii_code);

		    sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
		    if(GetFileAttributes(file2check)!=-1)
			nonmsi=1;
		    else
			nonmsi=0;
			
		    AddInstallation(ii_code, ii_system, nonmsi, embedded);
		
		}
	    }
	}
	RegCloseKey(hKey);
    }
    
    if(!RegOpenKeyEx(HKEY_CURRENT_USER, "Environment", 0, KEY_QUERY_VALUE, &hKey))
    {
	DWORD dwType, dwSize=sizeof(ii_system);
	
	if(!RegQueryValueEx(hKey, "II_SYSTEM", 0, &dwType, (BYTE *)ii_system, &dwSize))
	{	
	    CString temp;
	    
	    if (Local_NMgtIngAt("II_INSTALLATION", ii_system, temp) && !CodeList.Find(temp))
	    {
		strcpy(ii_code, temp.GetBuffer(2));

		ConfigKey.Format("ii.%s.setup.embed_installation", Host);
		if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
		    && !ConfigKeyValue.CompareNoCase("ON"))
		    embedded="1";
		else
		    embedded="0";

		sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
		if (GetFileAttributes(file2check)!=-1)
		{
		    CodeList.AddTail(ii_code);

		    sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
		    if(GetFileAttributes(file2check)!=-1)
			nonmsi=1;
		    else
			nonmsi=0;
			
		    AddInstallation(ii_code, ii_system, nonmsi, embedded);

		}
	    }
	}
	RegCloseKey(hKey);
    }
    
    if(GetEnvironmentVariable("II_SYSTEM", ii_system, sizeof(ii_system)))
    {
	CString temp;
	
	if (Local_NMgtIngAt("II_INSTALLATION", ii_system, temp) && !CodeList.Find(temp))
	{
	    strcpy(ii_code, temp.GetBuffer(2));

	    ConfigKey.Format("ii.%s.setup.embed_installation", Host);
	    if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
	        && !ConfigKeyValue.CompareNoCase("ON"))
		embedded="1";
	    else
		embedded="0";

	    sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
	    if (GetFileAttributes(file2check)!=-1)
	    {
		CodeList.AddTail(ii_code);

		sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
		if (GetFileAttributes(file2check)!=-1)
		    nonmsi=1;
		else
		    nonmsi=0;

		AddInstallation(ii_code, ii_system, nonmsi, embedded);

	    }
	}
    }
}

/*
**  History:
**	23-July-2001 (penga03)
**	    Set INGRES_VER25 to "1" and pass it to MsiExec to indicate that
**	    this installation was installed by old installer and will be
**	    upgraded.
**	23-July-2001 (penga03)
**	    Because we changed the attributes of those features installed by
**	    old installer during upgrade. After upgrade is done, we need to
**	    change the features' attributes back to default in both MSI and
**	    cached MSI.
**	17-Aug-2001 (penga03)
**	    Take away the change made in 23-July-2001 (penga03).
**	28-Aug-2001 (penga03)
**	    If update Msi database failed, clean up the temporary Msi and
**	    Cabinet files.
**	12-Sep-2001 (penga03)
**	    If cannot find II_TEMPORARY, get the system temp directory. Also,
**	    correct some errors while calculating free disk space.
**	14-jan-2002 (penga03)
**	    If install Ingres using response file, the user interface level
**	    of MsiExec is set to be /qb-, which means "Basic UI with no modal
**	    dialog boxes displayed at the end of the installation".
**	17-jun-2004 (somsa01)
**	    If we're dealing with 64-bit Windows, launch and wait for the
**	    licensing install here before running the main Ingres installer.
**	    A Merge Module version of licensing will not be available in time
**	    for this product to go GA, and since we cannot run child msi's
**	    from a parent msi, we need to run the licensing install outside of
**	    the main install.
**	08-feb-2005 (penga03)
**	    Return more information if copy cab file fails.
**	08-march-2005 (penga03)
**	    Pass the Cdimage (the directory containing the installation package)
**	    during upgrade.
**	01-jul-2005 (penga03)
**	    Add log information for mdb install.
**	26-Jul-2007 (wanfr01)
**	    Bug 118654  
**	    Remove the \r character from the patchid - it gets used in 
**	    filenames which can lead to OS error 123 
**  21-Jul-2007 wonca01 
**      Bug 118994
**      Modify ProductName to patchid instead of iicode.
**  28-Feb-2008 (wonca01) Bug 120015
**      Handle the case when version.rel not be opened.
*/
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

BOOL
CPreInstallation::LaunchPatchInstaller()
{
	int rc = 0;
	char install_c[3];				/* installation id */
	char *iicode = install_c;
	char patchid[PATCH_SIZE];	/* id of a patch */
	char logBuf[BUFF_SIZE], cmd[BUFF_SIZE]; /* buffers used to storing log messages and command line */
	char szBuf[MAX_PATH], ii_system[MAX_PATH];
	char pathBuf[MAX_PATH+1], path[MAX_PATH+1], vers[MAX_PATH+1], nfname[MAX_PATH+1]; /* store various pathes to files */
	char *ptrBuf; /* points to patchBuf */
	MSIHANDLE msih, sumh;	
	char pack[64], code[64], guid[64], view[2048];
	int	idx; /* installation id converted into int*/
	FILE *fp;
	char *ch;
	PROCESS_INFORMATION pi; 
	STARTUPINFO si;
	DWORD	dwError;
	HKEY hRegKey;
	BOOL bSilent = FALSE;


	iicode = (LPTSTR)(LPCTSTR)m_InstallCode;
	if (thePreInstall.m_SilentInstall)
		bSilent=TRUE;

	

	memset(&pathBuf,0, sizeof(pathBuf));
	GetModuleFileName(AfxGetInstanceHandle(),pathBuf,sizeof(pathBuf));
	ptrBuf = &(pathBuf[MAX_PATH]);
	while (strncmp(ptrBuf, "\\", 1) != 0)
	{
		*ptrBuf='\0';
		ptrBuf--;
	}
	*ptrBuf='\0';
	sprintf(path, "%s\\Ingres Patch.msi", pathBuf);

	/*
	** Retrieve patchid from version.rel provided 
	** on the patch image.
	*/ 
 	sprintf(vers, "%s\\version.rel", pathBuf);
 	fp=fopen(vers, "r");
    if (fp)
    {
		ch = fgets(logBuf, sizeof(logBuf), fp);
		if (ch != NULL)
		{
			ch = fgets(patchid, sizeof(patchid), fp);

			/* patchid will include a '\r' character - remove */
			patchid[(strlen(patchid)-1)]='\0';
		}
		fclose(fp);
    }
	else
	{
		char tempbuf[256];
		sprintf(tempbuf,"%s cannot be accessed\n", vers);
		MessageBox(NULL,tempbuf,NULL,MB_OK);
		return -1;
	}

	/* retrieve ii_system */
	sprintf(szBuf, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", iicode);
   	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hRegKey))
   	{
        	 dwError = sizeof(ii_system);
       	     RegQueryValueEx(hRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwError);
        	 RegCloseKey(hRegKey);
	}
	else
	{
		sprintf(szBuf, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", iicode);
		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hRegKey))
		{
			 dwError = sizeof(ii_system);
       	     RegQueryValueEx(hRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwError);
        	 RegCloseKey(hRegKey);
		}
		else
		{
		sprintf(logBuf, "Please, make sure installation %s exists.", iicode);
		if (!bSilent)
			BoxErr4("Could not find II_SYSTEM in the registry.\n", logBuf);
		return_code = 1;
		return 1;
		}
	}

	/*create log file */
	sprintf(logBuf, "%s\\ingres\\files\\patch.log", ii_system);
	hLogFile=CreateFile(logBuf, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL, 0);
	if (hLogFile == INVALID_HANDLE_VALUE)
	    hLogFile=0;
	else
	{
		CTime cTime;
		CString s;
		SetFilePointer(hLogFile, 0, 0, FILE_END);
	    time(&t);
	    cTime=t;
		s.Format(IDS_SEPARATOR);
		AppendToLog(s);
	    s=cTime.Format(IDS_DATE);
	    AppendToLog(s);
		fgets(logBuf, sizeof(logBuf), fopen(vers, "r"));
		s.Format("## Version:	%s   \r\n## Patch:	%s\r\n", strtok(logBuf, "\r\n"), patchid);
		sprintf(logBuf, "## Source:	%s\r\n## Target:	%s\r\n", pathBuf, ii_system);
		s.Append(logBuf);
		AppendToLog(s);
		s.Format(IDS_SEPARATOR);
		AppendToLog(s);

	}

	/* copy msi file to temp directory */
	sprintf(nfname, "%s\\ingres\\temp\\IngresPatch[%s].msi", ii_system, iicode);
	if (!CopyFile(path, nfname, FALSE))
	{
		sprintf(logBuf, "Error while copying MSI file to %s, error returned (%d)\r\n", nfname, GetLastError());
		AppendToLog(logBuf);
		if (!bSilent)
			Error(IDS_CANNOTCOPYMSIFILE, nfname);
		return_code = 1;
	    return 1;
	}
	SetFileAttributes(nfname, FILE_ATTRIBUTE_NORMAL+FILE_ATTRIBUTE_ARCHIVE);


	/* compare versions in version.rel */
	if (CompareIngresVersion(ii_system) != 0)
	{
	     sprintf(logBuf, "Patch %s and installation %s are incompatible.\r\n", patchid, iicode);
		 AppendToLog(logBuf);
		 if (!bSilent)
		     BoxErr4(logBuf, "The version of patch must match the version of installation image.\r\n");
		 return_code = 1;
	     return 1;
	}


	if (!(MsiOpenDatabase(nfname, MSIDBOPEN_DIRECT, &msih)==ERROR_SUCCESS))
	{
		AppendToLog("Error openning MSI database. Please, make sure that .msi file exists\r\n");
		AppendToLog("in the patch folder and is not being used or updated\r\n");
		if (!bSilent)
		    BoxErr4("Error openning MSI database. Please, make sure that .msi file exists\r\n", 
				"in the patch folder and is not being used or updated\r\n");
		return_code = 1;
		return 1;
	}

	/* update patchid in msi*/
	sprintf(view, "UPDATE Property SET Value = '%s' WHERE Property = 'PatchID'", patchid);
	if (!setupii_vexe(msih, view))
	{
		AppendToLog("Error updating PatchID in Property table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating PatchID in Property table of the MSI.\r\n", "");
		return_code = 1;
		return 1;
	}

    if (_stricmp(iicode, "II"))
    {

	/* Compute the GUID index from the installation code */
	idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - 'A';
	if (idx <= 0)
	    idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - '0';
	
	/* Update PackageCode */
	sprintf(pack,"{ABBB%04X-3B80-4AF2-8374-F6B183897B74}",idx);
	if (!(MsiGetSummaryInformation(msih, 0, 1, &sumh)==ERROR_SUCCESS))
	{
		return_code = 1;
	    return 1;
	}
	if (!(MsiSummaryInfoSetProperty(sumh,9,VT_LPSTR,0,NULL,pack)==ERROR_SUCCESS))
	{
		return_code = 1;
	    return 1;
	}
	if (!(MsiSummaryInfoPersist(sumh)==ERROR_SUCCESS))
	{
		return_code = 1;
	    return 1;
	}
	MsiCloseHandle(sumh);
	
	/* Update UpgradeCode property */
	sprintf(code,"{ABBB%04X-3B80-4AF2-8374-F6B183897B74}",idx);
	sprintf(view,"UPDATE Property SET Value = '%s' WHERE Property = 'UpgradeCode'",code);
	if(!setupii_vexe(msih,view))
	{	
		AppendToLog("Error updating UpgradeCode in Property table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating UpgradeCode in Property table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}
	
	/* Update II_INSTALLATION property */
	sprintf(view,"UPDATE Property SET Value = '%s' WHERE Property = 'II_INSTALLATION'",iicode);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating II_INSTALLATION in Property table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating II_INSTALLATION in Property table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}
	
	/* Update ProductCode property */
	sprintf(guid,"{ABBB%04X-3B80-4AF2-8374-F6B183897B74}",idx);
	sprintf(view,"UPDATE Property SET Value = '%s' WHERE Property = 'ProductCode'",guid);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating PatchID in ProductCode table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating PatchID in ProductCode table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}
	
	/* Update ProductName property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres Patch %s ' WHERE Property = 'ProductName'",patchid);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating ProductName in Property table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating ProductName in Property table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}
	
	/* Update DisplayNameMinimal property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres [ %s ]' WHERE Property = 'DisplayNameMinimal'",iicode);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating DisplayNameMinimal in Property table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating DisplayNameMinimal in Property table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}
	
	/* Update DisplayNameCustom property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres [ %s ]' WHERE Property = 'DisplayNameCustom'",iicode);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating DisplayNameCustom in Property table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating DisplayNameCustom in Property table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}
	
	/* Update DisplayNameTypical property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres [ %s ]' WHERE Property = 'DisplayNameTypical'",iicode);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating DisplayNameTypical in Property table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating DisplayNameTypical in Property table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}

	/* Update shortcut folder display name */
	sprintf(view,"UPDATE Directory SET DefaultDir = 'Ingres II [ %s ]' WHERE DefaultDir = 'Ingres II [ II ]'",iicode);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating DefaultDir in Directory table of the MSI.\r\n");	
		if (!bSilent)
			BoxErr4("Error updating DefaultDir in Directory table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}
	sprintf(view,"UPDATE Directory SET DefaultDir = 'Ingres [%s]' WHERE DefaultDir = 'Ingres [ II ]'",iicode);
	if(!setupii_vexe(msih,view))
	{
		AppendToLog("Error updating DefaultDir in Directory table of the MSI.\r\n");
		if (!bSilent)
			BoxErr4("Error updating DefaultDir in Directory table of the MSI.\r\n", "");
		return_code = 1;
	    return 1;
	}

    }

	/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	{
		return_code = 1;
	    return 1;
	}
	
	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	{
		return_code = 1;
	    return 1;
	}

	if (hLogFile)
		CloseHandle(hLogFile);

	/* launch msi installation (add  "/l*v c:\\temp\\patchlog.log"  to the MsiExec command line for tracing"*/
	if (thePreInstall.m_MSILog.IsEmpty())
	{
		if (bSilent)
			sprintf(cmd, "MsiExec.exe /i \"%s\" /qn PATCH_IMAGE=\"%s\"", nfname, pathBuf);
		else
			sprintf(cmd, "MsiExec.exe /i \"%s\"  PATCH_IMAGE=\"%s\"", nfname, pathBuf);
	}
	else
	{
		if (bSilent)
			sprintf(cmd, "MsiExec.exe /i \"%s\" /qn /l*v \"%s\" PATCH_IMAGE=\"%s\"", nfname, m_MSILog, pathBuf);
		else
			sprintf(cmd, "MsiExec.exe /i \"%s\" /l*v \"%s\" PATCH_IMAGE=\"%s\"", nfname, m_MSILog, pathBuf);
	}
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	if(!CreateProcess(NULL,cmd,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi))
    {
		DeleteFile(nfname);
		dwError=GetLastError();
		sprintf(logBuf, "Error returned by Operating System: (%d).", dwError);
		AppendToLog(logBuf);
		if (!bSilent)
			BoxErr4("Error while launching MSI file.\r\n", logBuf);
		return_code = 1;
		return 1;
    }

	if (thePreInstall.m_SilentInstall)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		if (GetExitCodeProcess(pi.hProcess, &dwError)) 
		{
			return_code = 0;
			rc=dwError;
		}
		else 
		{
			return_code = 1;
			rc=GetLastError();
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

    return rc;
}

/*
**  History:
**	23-July-2001 (penga03)
**	    Change the brandings on each dialog from "InstallShield"
**	    to "Ingres II".
**	23-July-2001 (penga03)
**	    If this is a upgrade, set each feature attribute to be
**	    Required or not according to correspondig package installed
**	    or not.
**	15-aug-2001 (somsa01)
**	    Change the IVM Startup folder shortcut name to include the
**	    proper II_INSTALLATION.
**	17-Aug-2001 (penga03)
**	    Take away the change made in 23-July-2001 (penga03).
**	05-Sep-2001 (penga03)
**	    Don't change the brandings in pre-installer process any more. 
**	13-Sep-2001 (penga03)
**	    Don't open the MSI database if the installation identifier is II, 
**	    since we will do nothing on the database under such condition.
**	30-jan-2002 (penga03)
**	    Changed the default Ingres directory to 
**	    "Programs Files\\CA\\Advantage Ingres" with installation id is II 
**	    or "Programs Files\\CA\\Advantage Ingres [ %II_INSTALLATION% ] with 
**	    installation id is other than II.
**	    Also, changed the Ingres menu items under 
**	    "Start\\Programs\\Computer Associates\\Advantage\\Advantage Ingres 
**	    [ %II_INSTALLATION% ]".
*/
BOOL
setupii_edit(char *iicode, char *path)
{
    MSIHANDLE	msih, sumh;
    int		idx;
    char	pack[64], code[64], guid[64], view[2048];
	
    if (_stricmp(iicode, "II"))
    {
	if (!(MsiOpenDatabase(path, MSIDBOPEN_DIRECT, &msih)==ERROR_SUCCESS))
	    return FALSE;

	/* Compute the GUID index from the installation code */
	idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - 'A';
	if (idx <= 0)
	    idx = (toupper(iicode[0]) - 'A') * 26 + toupper(iicode[1]) - '0';
	
	/* Update PackageCode */
	sprintf(pack,"{A78C%04X-2979-11D5-BDFA-00B0D0AD4485}",idx);
	if (!(MsiGetSummaryInformation(msih, 0, 1, &sumh)==ERROR_SUCCESS))
	    return FALSE;
	if (!(MsiSummaryInfoSetProperty(sumh,9,VT_LPSTR,0,NULL,pack)==ERROR_SUCCESS))
	    return FALSE;
	if (!(MsiSummaryInfoPersist(sumh)==ERROR_SUCCESS))
	    return FALSE;
	MsiCloseHandle(sumh);
	
	/* Update UpgradeCode property */
	sprintf(code,"{A78B%04X-2979-11D5-BDFA-00B0D0AD4485}",idx);
	sprintf(view,"UPDATE Property SET Value = '%s' WHERE Property = 'UpgradeCode'",code);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update II_INSTALLATION property */
	sprintf(view,"UPDATE Property SET Value = '%s' WHERE Property = 'II_INSTALLATION'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update ProductCode property */
	sprintf(guid,"{A78D%04X-2979-11D5-BDFA-00B0D0AD4485}",idx);
	sprintf(view,"UPDATE Property SET Value = '%s' WHERE Property = 'ProductCode'",guid);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update ProductName property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres [ %s ]' WHERE Property = 'ProductName'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update DisplayNameMinimal property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres [ %s ]' WHERE Property = 'DisplayNameMinimal'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update DisplayNameCustom property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres [ %s ]' WHERE Property = 'DisplayNameCustom'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update DisplayNameTypical property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres [ %s ]' WHERE Property = 'DisplayNameTypical'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;

	/* Update INGRES_CLUSTER_RESOURCE property */
	sprintf(view,"UPDATE Property SET Value = 'Ingres Service [ %s ]' WHERE Property = 'INGRES_CLUSTER_RESOURCE'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;

	/* Update shortcut folder display name */
	sprintf(view,"UPDATE Directory SET DefaultDir = 'Ingres II [ %s ]' WHERE DefaultDir = 'Ingres II [ II ]'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	sprintf(view,"UPDATE Directory SET DefaultDir = 'Ingres [ %s ]' WHERE DefaultDir = 'Ingres [ II ]'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update IVM Startup shortcut display name */
	sprintf(view,"UPDATE Shortcut SET Name = 'Ingres Visual Manager [ %s ]' WHERE Name = 'INGRES~1|Ingres Visual Manager [ II ]'",iicode);
	if(!setupii_vexe(msih,view))
	    return FALSE;
	
	/* Update the Media table */
	sprintf(view,"UPDATE Media SET Cabinet = 'IngresII[%s].cab' WHERE Cabinet = 'Data1.cab'",iicode);
	if (!setupii_vexe(msih,view))
	    return FALSE;

	/* Update Component GUIDs */
	if (!UpdateComponentId(msih, idx))
	    return FALSE;

	/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;
	
	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	    return FALSE;

	return TRUE;
    }
	
    return TRUE;
}

/*
**  History:
**	23-July-2001 (penga03)
**	    Modified function setupii_vexe so that it can retrieve the string value of 
**	    a record field.
*/
BOOL 
setupii_vexe(MSIHANDLE hDatabase, char *szQuery, char *szValue)
{
    MSIHANDLE hView, hRecord;
    char szValueBuf[64];
    DWORD cchValueBuf=sizeof(szValueBuf);
    
    if (!(MsiDatabaseOpenView(hDatabase, szQuery, &hView)==ERROR_SUCCESS))
	return FALSE;
    if (!(MsiViewExecute(hView, 0)==ERROR_SUCCESS))
	return FALSE;
    
    if (szValue)
    {
	MsiViewFetch(hView, &hRecord);
	MsiRecordGetString(hRecord, 1, szValueBuf, &cchValueBuf);
	strcpy(szValue, szValueBuf);
	MsiCloseHandle(hRecord);
    }

    if (!(MsiCloseHandle(hView)==ERROR_SUCCESS))
	return FALSE;

    return TRUE;
}

/*
**  Name: count_tokens
**  
**  Description: Counts tokens in a string.  Tokens are considered
**				 any string separated by tab, new line, carriage return
**               or space.
**
**	Parameters:  str - String to be parsed for tokens.
**
**  Return Code: Number of tokens found in a string parameter. 
*/
int
count_tokens(char *str)
{
	char *token;
	int	counter=0;
	char	sep[] = " \t\r\n";
	token = strtok(str, sep);
	while (token != NULL)
	{
		counter++;
		token = strtok(NULL, sep);
	}

	return counter;
}



/*
**  History:
**	06-Sep-2001 (penga03)
**	    Determine whether windows installer installed or not by checking 
**	    the presence of MSI.DLL in the system directory.
**	12-nov-2001 (somsa01)
**	    Make sure we compare the version of the Windows Installer, as
**	    there are multiple versions of it now.
*/
BOOL 
check_windowsinstaller()
{
    char ach[MAX_PATH+1], *p;
    
    GetSystemDirectory(ach, sizeof(ach));
    strcat(ach, "\\msi.dll");
    if(GetFileAttributes(ach)!=-1)
    {
	int	result;
	char	instmsi[32];
	
	strcpy(instmsi, (IsWindows9X()) ? "instmsia.exe" : "instmsiw.exe");
	if ((result = CompareVersion(ach, instmsi)) >= 0)
	{
	    if (result == 0 || result == 1)
		return TRUE;
	}
    }

    /* install windows installer */
    GetModuleFileName(AfxGetInstanceHandle(),ach,sizeof(ach));
    p=_tcsrchr(ach,'\\');
    if(*p) *(p+1)=0;

    if(!IsWindows9X())
    {
	/* Windows NT, Windows 2000 need administrative privilege */
	if(!IsAdmin())
	{
	    Error(IDS_NOTADMINISTRATOR);
	    return FALSE;
	}
	strcat(ach, "instmsiw.exe");
	if(!ExecuteEx(ach))
	{
	    Error(IDS_INSTALLWIFAILED);
	    return FALSE;
	}
	return TRUE;
    }

    /* Windows 98, Windows ME */
    strcat(ach, "instmsia.exe");
    if(!ExecuteEx(ach))
    {
	Error(IDS_INSTALLWIFAILED);
	return FALSE;
    }
    return TRUE;
}

BOOL 
IsAdmin()
{
    /* Determine whether the current process is running under a 
       local administrator account. */

    HANDLE hAccessToken;
    UCHAR InfoBuffer[1024];
    DWORD dwInfoBufferSize;
    BOOL bSuccess;
    SID_IDENTIFIER_AUTHORITY sidNtAuthority=SECURITY_NT_AUTHORITY;
    PSID psidAdministrators;
    PTOKEN_GROUPS ptgGroups=(PTOKEN_GROUPS)InfoBuffer;
    UINT x;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken))
	return FALSE;
    bSuccess=GetTokenInformation(hAccessToken, TokenGroups, InfoBuffer, 1024, &dwInfoBufferSize);
    CloseHandle(hAccessToken);
    if (!bSuccess)
	return FALSE;
    if (!AllocateAndInitializeSid(&sidNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
	DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdministrators))
    {
	return FALSE;
    }
    
    bSuccess=FALSE;
    for (x=0; x<ptgGroups->GroupCount; x++)
    {
	if (EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid))
	{
	    bSuccess=TRUE;
	    break;
	}
    }
    FreeSid(psidAdministrators);
    return bSuccess;
}

/*
**	History:
**	23-Aug-2001 (penga03)
**	    Created.
*/
BOOL 
UpdateComponentId(MSIHANDLE hDatabase, int id)
{
    MSIHANDLE hView, hRecord;

    if (!(MsiDatabaseOpenView(hDatabase, "SELECT Component, ComponentId FROM Component", 
	&hView)==ERROR_SUCCESS))
    {
	return FALSE;
    }

    if (!(MsiViewExecute(hView, 0)==ERROR_SUCCESS))
	return FALSE;
    
    while (MsiViewFetch(hView, &hRecord)!=ERROR_NO_MORE_ITEMS)
    {
	char szValueBuf[39];
	DWORD cchValueBuf=sizeof(szValueBuf);
	char *token, *tokens[5];
	int num=0;

	MsiRecordGetString(hRecord, 2, szValueBuf, &cchValueBuf);
	
	token=strtok(szValueBuf, "{-}");
	while (token != NULL )
	{
	    tokens[num]=token;
	    token=strtok(NULL, "{-}");
	    num++;
	}
	sprintf(szValueBuf, "{%s-%s-%s-%04X-%012X}", tokens[0], tokens[1], tokens[2], id, id*id);

	MsiRecordSetString(hRecord, 2, szValueBuf);
	
	if (!(MsiViewModify(hView, MSIMODIFY_UPDATE, hRecord)==ERROR_SUCCESS))
	    return FALSE;
	
	MsiCloseHandle(hRecord);
    }
    
    if (!(MsiCloseHandle(hView)==ERROR_SUCCESS))
	return FALSE;

    return TRUE;
}

void 
CPreInstallation::AddInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded)
{
    CInstallation *inst=new CInstallation(id, path, ver25, embedded);
    if (inst)
	m_Installations.Add(inst);
}

/*
**  Name: CompareVersion
**
**  Return Codes:
**	 0 = Files are the same version
**	 1 = File1 is a newer version than file2
**	 2 = File2 is a newer version than file1
**	-1 = Error getting information on file1
**	-2 = Error getting information on file2
**	-3 = Memory allocation error
*/
INT
CompareVersion(char *file1, char *file2 )
{
    DWORD  dwHandle = 0L;	/* Ignored in call to GetFileVersionInfo */
    DWORD  cbBuf1   = 0L, cbBuf2 = 0L;
    LPVOID lpvData1 = NULL, lpvData2 = NULL;
    LPVOID lpValue1 = NULL, lpValue2 = NULL;
    UINT   wBytes1 = 0L, wBytes2 = 0L;
    WORD   wlang1 = 0, wcset1 = 0, wlang2 = 0, wcset2 = 0;
    char   SubBlk1[81], SubBlk2[81];
    INT    rcComp = 0;

    /* Retrieve Size of Version Resource */
    if ((cbBuf1 = GetFileVersionInfoSize(file1, &dwHandle)) == 0)
    {
	rcComp = -1;
	goto QuickExit;
    }
	
    if ((cbBuf2 = GetFileVersionInfoSize(file2, &dwHandle)) == 0)
    {
	rcComp = -2;
 	goto QuickExit;
    }

    lpvData1 = (LPVOID)malloc(cbBuf1);
    lpvData2 = (LPVOID)malloc(cbBuf2);

    if (!lpvData1 && lpvData2)
    {
	rcComp = -3;
	goto QuickExit;
    }

    /* Retrieve Version Resource */
    if (GetFileVersionInfo(file1, dwHandle, cbBuf1, lpvData1) == FALSE)
    {
	rcComp = -1;
 	goto QuickExit;
    }
	
    if (GetFileVersionInfo(file2, dwHandle, cbBuf2, lpvData2) == FALSE)
    {
	rcComp = -2 ;
 	goto QuickExit;
    }

    /* Retrieve the Language and Character Set Codes */
    VerQueryValue(lpvData1, TEXT("\\VarFileInfo\\Translation"), &lpValue1, &wBytes1);
    wlang1 = *((WORD *)lpValue1);
    wcset1 = *(((WORD *)lpValue1) + 1);
	                   
    VerQueryValue(lpvData2, TEXT("\\VarFileInfo\\Translation"), &lpValue2, &wBytes2);
    wlang2 = *((WORD *)lpValue2);
    wcset2 = *(((WORD *)lpValue2) + 1);

    /* Retrieve FileVersion Information */
    sprintf(SubBlk1, "\\StringFileInfo\\%.4x%.4x\\FileVersion", wlang1, wcset1);
    VerQueryValue(lpvData1, TEXT(SubBlk1), &lpValue1, &wBytes1);
    sprintf(SubBlk2, "\\StringFileInfo\\%.4x%.4x\\FileVersion", wlang2, wcset2);
    VerQueryValue(lpvData2, TEXT( SubBlk2), &lpValue2, &wBytes2);

    {
	int majver1=0, minver1=0, relno1=0, majver2=0, minver2=0, relno2=0;

	sscanf((char *)lpValue1, "%d.%d.%d", &majver1, &minver1, &relno1);
	sscanf((char *)lpValue2, "%d.%d.%d", &majver2, &minver2, &relno2);

	/* Check Major Version Number */
	if (majver1 == majver2)
	{
	    /* Check Minor Version Number */
	    if (minver1 == minver2)
	    {
		/* Check Release Number */
		if (relno1 == relno2)
		    rcComp = 0;
		else
		{
		    if (relno1 > relno2)
			rcComp = 1;
		    else
			rcComp = 2;
		}
	    }
	    else
	    {
		if (minver1 > minver2)
		    rcComp = 1;
		else
		    rcComp = 2;
	    }
	}
	else
	{
	    if (majver1 > majver2)
		rcComp = 1;
  	    else
		rcComp = 2;
	}
    }

QuickExit:
    if (lpvData1)
	free(lpvData1);
    if (lpvData2 )
	free(lpvData2);
    return (rcComp);
}


/*
**  Name: CompareIngresVersion
**
**  Return Codes:
**	 0 = same version
**	 1 = newer version, update/minor upgrade
**	 2 = newer version, major upgrade (need re-run iipostinst.exe)
**	 3 = older version
**	-1 = error 
*/
INT
CompareIngresVersion(char *ii_system)
{
    char strFile01[1024], strFile02[1024];
    DWORD dwHandle=0;	/* Ignored in call to GetFileVersionInfo */
    DWORD dwVerInfoSize=0;
    UINT nSize=0;
    void *pVerData=NULL;
    VS_FIXEDFILEINFO *pFixedFileInfo=NULL;
    DWORD MajVer1=0, MinVer1=0, PthNo1=0, BldNo1=0;
    DWORD MajVer2=0, MinVer2=0, PthNo2=0, BldNo2=0;
    INT rcComp = 0;

    if (!ii_system[0])
    {
	rcComp=-1;
	goto QuickExit;
    }

    /* Get the file version of install.exe. */
    GetModuleFileName(AfxGetInstanceHandle(), strFile01, sizeof(strFile01));

    if (!(dwVerInfoSize=GetFileVersionInfoSize(strFile01, &dwHandle)))
    {
	rcComp=-1;
	goto QuickExit;
    }
	
    pVerData=(LPVOID)malloc(dwVerInfoSize);
    if (!pVerData)
    {
	rcComp=-1;
	goto QuickExit;
    }

    if (!GetFileVersionInfo(strFile01, dwHandle, dwVerInfoSize, pVerData))
    {
	rcComp = -1;
 	goto QuickExit;
    }
	
    VerQueryValue(pVerData, TEXT("\\"), (void **)&pFixedFileInfo, &nSize);
    MajVer1=HIWORD(pFixedFileInfo->dwFileVersionMS);
    MinVer1=LOWORD(pFixedFileInfo->dwFileVersionMS);
    PthNo1=HIWORD(pFixedFileInfo->dwFileVersionLS);
    BldNo1=LOWORD(pFixedFileInfo->dwFileVersionLS);

    /* Get the version of the installation being upgraded/modified. */
    FILE *fp;
    char s[512], *p, *q, *tokens[3];
    int count;

    sprintf(strFile02, "%s\\ingres\\version.rel", ii_system);
    fp=fopen(strFile02, "r");
    if (fp)
    {
	fscanf(fp, "%s", s ); /* II */

	fscanf(fp, "%s", s ); /* 3.0.1 */
	p=s;
	for (count=0; count<=2; count++)
	{
	    q=strchr(p, '.');
	    if (q) *q='\0';
	    tokens[count]=p;
	    if (q) p=q+1;
	}
	MajVer2=atoi(tokens[0]);
	MinVer2=atoi(tokens[1]);
	PthNo2=atoi(tokens[2]);

	fscanf(fp, "%s", s ); /* (int.w32/108) */
	s[strlen(s)-1]=0;
	p=strchr(s, '/');
	if (p) BldNo2=atoi(p+1)*100;
	fclose(fp);
    }

    /*
    **	 0 = same version
    **	 1 = newer version, update/minor upgrade
    **	 2 = newer version, major upgrade
    **	 3 = older version 
    */

    if (MajVer1 == MajVer2)
    {
	if (MinVer1 == MinVer2)
	{
	    if (PthNo1 == PthNo2)
	    {
		if (BldNo1 == BldNo2)
		{
		    rcComp=0;
		}
		else if (BldNo1 > BldNo2)
		{
		    //rcComp=1;
		    rcComp=2;
		}
		else
		{
		    rcComp=3;
		}
	    } /* end of if (PthNo1 == PthNo2) */
	    else if (PthNo1 > PthNo2)
	    {
		rcComp=2;
	    }
	    else
	    {
		rcComp=3;
	    }
	} /* end of if (MinVer1 == MinVer2) */
	else if (MinVer1 > MinVer2)
	{
	    rcComp=2;
	}
	else
	{
	    rcComp=3;
	}
    } /* end of if (MajVer1 == MajVer2) */
    else if (MajVer1 > MajVer2)
    {
	rcComp=2;
    }
    else
    {
	rcComp=3;
    }

QuickExit:
    if (pVerData)
	free(pVerData);
    return (rcComp);
}


/*
** CInstallation Class
*/

/*
** Construction/Destruction
*/

CInstallation::CInstallation()
{

}

CInstallation::~CInstallation()
{

}

CInstallation::CInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded)
{
    m_id=id;
    m_path=path;
    m_ver25=ver25;
	m_embedded=embedded;
}

/*
**  Name: BoxErr4
**
**  Description:
**	Quick little function for printing messages to the screen, with
**	the stop sign. Takes two strings as it's input.
**
**	Parameter: MessageString - String 1.
**             MessageString2 - String2.
**
*/
VOID
BoxErr4 (CHAR *MessageString, CHAR *MessageString2)
{
    CHAR	MessageOut[1024];

    sprintf(MessageOut,"%s%s",MessageString, MessageString2);
    MessageBox( hDlgGlobal, MessageOut, "Ingres Patch Installer", MB_APPLMODAL | MB_ICONSTOP | MB_OK);
}
