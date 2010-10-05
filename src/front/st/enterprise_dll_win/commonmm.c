/*
**  Copyright (c) 2001-2009 Ingres Corporation.
*/

/*
**  Name: commonmm.c
**
**  Description:
**	This file contains common routines used by merge module DLLs.
**
**	    Local_PMget		- PMget().
**	    Local_NMgtIngAt	- NMgtIngAt().
**	    RemoveDir		- Removes a directory.
**	    Execute		- Create a new process to execute an
**				  executable file. 
**	    VerifyPath		- Verify a path.
**	    GetRegistryEntry	- Obtain the data for a specified value
**				  name associated with an open registry 
**				  key's subkey.
**
**  History:
**	22-apr-2001 (somsa01)
**	    Created.
**	08-may-2001 (somsa01)
**	    In Local_NMgtIngAt(), corrected proper error code returns.
**	    Also, added Execute() and Local_PMhost().
**	14-may-2001 (penga03)
**	    Added VerifyPath(), and GetRegistryEntry().
**	03-Aug-2001 (penga03)
**	    Added SetRegistryEntry(), StringReverse(), and StringReplace().
**	29-oct-2001 (penga03)
**	    In Local_NMgtIngAt() and Local_PMget(), initialize the output string, 
**	    strRetValue, to be 0.
**	30-oct-2001 (penga03)
**	    Added function OtherInstallationsExist.
**	10-dec-2001 (penga03)
**	    Added function GetInstallPath.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage Ingres\\".
**	12-feb-2002 (penga03)
**	    Took the function IsWindows95() from setupmm.c.
**	06-mar-2002 (penga03)
**	    Took the function ExecuteEx() from dbmsnetmm.c.
**	14-may-2002 (penga03)
**	    Added CreateOneShortcut() and CreateDirectoryRecursively().
**	04-jun-2002 (penga03)
**	    Added a function GetRefCount() and a macro GUID_TO_REG().
**	01-aug-2002 (penga03)
**	    Added GetLinkInfo() to obtain the path and file name of a 
**	    given link file.
**	02-oct-2002 (penga03)
**	    Corrected VerifyPath() so as to fix the problem happened on 
**	    double bytes Windows machines, if the installation path includes 
**	    local characters, the last few valid characters will be chopped 
**	    off when it goes to the next page. Also modified the function to 
**	    write the path being verified back, if being corrected. 
**	08-oct-2002 (penga03)
**	     Modified GetLinkInfo() so that it will return TRUE if it 
**	     finds the path of a given link, otherwise return FALSE.
**	26-mar-2003 (penga03)
**	     In Local_NMgtIngAt and Local_PMget, replaced isalnum with 
**	     isspace when parsing a line to get the value of the passed 
**	     symbol, because the value could have an non alphanumeric 
**	     character.
**	26-jun-2003 (penga03)
**	     Moved MyMessageBox() from setupmm.c to commonmm.c and added 
**	     VerifyInstance().
**	09-jul-2003 (penga03)
**	    In GetLinkInfo(), convert the string to lowercase before return.
**	06-aug-2003 (penga03)
**	    Add RemoveOneDir().
**	21-aug-2003 (penga03)
**	    Added CheckUserRights() to verify if the user that currently 
**	    logged on has following privilege and rights required: 
**	    SE_TCB_NAME, SE_SERVICE_LOGON_NAME, SE_NETWORK_LOGON_NAME.
**	20-oct-2003 (penga03)
**	    Added a new function DelTok().
**	    Modified VerifyInstance() to make sure Enterprise and Embedded 
**	    are not be convertible.
**	10-dec-2003 (penga03)
**	    Modified VerifyInstance() so that following rules must be  
**	    followed during upgrade:
**	    (1) Two different Ingres instances can neither share the same 
**	        installation identifier nor be installed to the same 
**	        destination folder;
**	    (2) Enterprise Ingres can NOT be upgraded to Embedded and vice 
**	        versa;
**	    (3) Single byte Ingres can NOT be upgraded to Double byte and 
**	        vice versa;
**	    (4) An Ingres installation installed by CA-installer can not be 
**	        upgraded to its MSI version. This rule can be turned off by 
**	        setting the value of the INGRES_UPGRADE_NONMSIINGRES property 
**	        to 1.
**	    With one exception: if ETRUSTDIR_UPGRADE_INGRES20 property is 
**	    set to 1, those rules will be bypassed. This property is ONLY 
**	    used by eTrust Directory.
**	    Also added CheckVer25() and CheckDBL().
**	22-apr-2004 (penga03)
**	    Added AskUserYN().
**	16-jul-2004 (penga03)
**	    Modified GetRefCount() so we get reference count of a component 
**	    using MsiEnumClients instead of looking into the Registry.
**	26-jul-2004 (penga03)
**	    Removed all references to "Advantage".
**	07-dec-2004 (penga03)
**	    Changed the formulation to generate GUIDs since the old one does
**	    not work for id from A1 to A9.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
**	28-jan-2005 (penga03)
**	    In VerifyInstance(), take away rule (2) and rule (3), enterprise 
**	    and embedded, single byte and double byte cannot mutual upgrade.
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	16-march-2005 (penga03)
**	    Corrected some error in CheckSharedDiskInSameRG.
**	16-march-2005 (penga03)
**	    Corrected some error in CheckSharedDiskInSameRG again.
**	21-march-2005 (penga03)
**	    Added AddUserRights().
**	14-apr-2005 (penga03)
**	    In AddUserRights(), open the policy with POLICY_CREATE_ACCOUNT and 
**	    POLICY_LOOKUP_NAMES access.
**	14-june-2005 (penga03)
**	    Added IsDotNETFXInstalled().
**	12-jul-2005 (penga03)
**	    In CreateOneShortcut() replaced SHGetSpecialFolderLocation with 
**	    SHGetFolderLocation.
**	31-aug-2005 (penga03)
**	    Added VerifyIngTimezone(), VerifyIngCharset(), VerifyIngTerminal(),
**	    VerifyIngDateFormat(), VerifyIngMoneyFormat().
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	25-Jan-2006 (drivi01)
**	    Added missing WIN1253, PC737, KOI8, ISO88597 and PC857 
**	    charsets.
**	13-Feb-2007 (drivi01)
**	    Added "IsVista()" function to determine if we're on the Vista OS.
**	14-May-2007 (drivi01)
**	    Changed the title of installer window in the routines throughout
**	    the code to make sure all the errors are brought on foreground.
**	16-May-2007 (drivi01)
**	    Added UTF8 character set.
**	16-Jan-2009 (whiro01) SD Issue 132899
**	    In conjunction with the fix for this issue, fixed a few compiler warnings.
**	07-Aug-2009 (drivi01)
**	    Add pragma to remove deprecated POSIX functions warning 4996 
**          as it is a bug.
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <shlobj.h> 
#include <objbase.h>
#include <msi.h>
#include <MsiQuery.h>
#include <ntsecapi.h>
#include <lm.h>
#include <clusapi.h>
#include <shellapi.h>

#define MIN_DIR 3
#define HA_SUCCESS					0
#define HA_FAILURE					1
#define HA_INVALID_PARAMETER		2
#define HA_CLUSTER_NOT_RUNNING		3
#define HA_NO_DISK_RESOURCE_MATCH	4
#define HA_SERVICE_NOT_FOUND		5
#define SIZE			256
#define INGRESSVCNAME	L"Ingres_Database_"
#define DISKPROPNAME	L"Disk"
#define PHYSICALDISK	L"Physical Disk"

#define GUID_TO_REG(_guid,_reg) \
    {   \
    _reg[0]     = _guid[8]; \
    _reg[1]     = _guid[7]; \
    _reg[2]     = _guid[6]; \
    _reg[3]     = _guid[5]; \
    _reg[4]     = _guid[4]; \
    _reg[5]     = _guid[3]; \
    _reg[6]     = _guid[2]; \
    _reg[7]     = _guid[1]; \
    _reg[8]     = _guid[13]; \
    _reg[9]     = _guid[12]; \
    _reg[10]    = _guid[11]; \
    _reg[11]    = _guid[10]; \
    _reg[12]    = _guid[18]; \
    _reg[13]    = _guid[17]; \
    _reg[14]    = _guid[16]; \
    _reg[15]    = _guid[15]; \
    _reg[16]    = _guid[21]; \
    _reg[17]    = _guid[20]; \
    _reg[18]    = _guid[23]; \
    _reg[19]    = _guid[22]; \
    _reg[20]    = _guid[26]; \
    _reg[21]    = _guid[25]; \
    _reg[22]    = _guid[28]; \
    _reg[23]    = _guid[27]; \
    _reg[24]    = _guid[30]; \
    _reg[25]    = _guid[29]; \
    _reg[26]    = _guid[32]; \
    _reg[27]    = _guid[31]; \
    _reg[28]    = _guid[34]; \
    _reg[29]    = _guid[33]; \
    _reg[30]    = _guid[36]; \
    _reg[31]    = _guid[35]; \
    _reg[32]    = '\0'; \
    };

char *IngTimezones[]=
{
    "AUSTRALIA-LHI",        "AUSTRALIA-NORTH",      "AUSTRALIA-NSW",
    "AUSTRALIA-QUEENSLAND", "AUSTRALIA-SOUTH",      "AUSTRALIA-TASMANIA",
    "AUSTRALIA-VICTORIA",   "AUSTRALIA-WEST",       "AUSTRALIA-YANCO",
    "BRAZIL-ACRE",          "BRAZIL-DENORONHA",     "BRAZIL-EAST",
    "BRAZIL-WEST",          "CANADA-ATLANTIC",      "CANADA-NEWFOUNDLAND",
    "CANADA-YUKON",         "CHILE-CONTINENTAL",    "CHILE-EASTER-ISLAND",
    "EGYPT",                "EUROPE-CENTRAL",       "EUROPE-EASTERN",
    "EUROPE-WESTERN",       "GMT",                  "GMT-1",
    "GMT-10",               "GMT-11",               "GMT-12",
    "GMT-2",                "GMT-3",                "GMT-4",
    "GMT-5",                "GMT-6",                "GMT-7",
    "GMT-8",                "GMT-9",                "GMT1",
    "GMT10",                "GMT11",                "GMT12",
    "GMT13",                "GMT2",                 "GMT3",
    "GMT3-and-half",        "GMT4",                 "GMT5",
    "GMT5-and-half",        "GMT7",                 "GMT8",
    "GMT9",                 "GMT9-and-half",        "HONG-KONG",
    "INDIA",                "INDONESIA-CENTRAL",    "INDONESIA-EAST",
    "INDONESIA-WEST",       "IRAN",                 "IRELAND",
    "ISRAEL",               "JAPAN",                "KUWAIT",
    "MALAYSIA",             "MEXICO-BAJANORTE",     "MEXICO-BAJASUR",
    "MEXICO-GENERAL",       "NA-CENTRAL",           "NA-EASTERN",
    "NA-MOUNTAIN",          "NA-PACIFIC",           "NEW-ZEALAND",
    "NEW-ZEALAND-CHATHAM",  "PAKISTAN",             "PHILIPPINES",
    "POLAND",               "GMT6",                 "KOREA",
    "SAUDI-ARABIA",         "SINGAPORE",            "THAILAND",
    "TURKEY",               "UNITED-KINGDOM",       "US-ALASKA",
    "US-HAWAII",            "VIETNAM",              "GMT-2-and-half",
    "GMT-3-and-half",       "MOSCOW",               '\0'
};

char *IngCharsets[]=
{
    "SLAV852",    "ISO88599",    "ISO88597"	"ISO88595",    "ISO88592",    
    "ISO88591",   "IBMPC850",    "IBMPC437",    "HPROMAN8",    "HEBREW",      
    "PCHEBREW",   "WHEBREW",     "GREEK",       "ELOT437",     "DECMULTI",    
    "ARABIC",     "WARABIC",     "IS885915",    "WIN1250",     "WIN1252",     
    "WIN1253",    "DOSASMO",     "THAI",        "WTHAI",       "CHINESES",    
    "KANJIEUC",   "KOREAN",      "SHIFTJIS",    "CHTBIG5",     "CHTHP",       
    "CHTEUC",     "CSGBK",       "CW",          "KOI8",	       "PC737",       
    "PC857",	  "UTF8",	 '\0'
};

char *IngTerminals[]=
{
    "IBMPC",
    "IBMPCD",
    "PCANSIC",
    "PCANSIM",
    "PCCOLOR",
    '\0'
};

char *IngDateFormats[]=
{
    "US",
    "ISO",
    "MULTINATIONAL",
    "SEWDEN",
    "FINLAND",
    "GERMAN",
    "DMY",
    "MDY",
    "YMD",
    '\0'
};

BOOL CheckVer25(char *id, char *ii_system);


/*{
**  Name: Local_NMgtIngAt
**
**  Description:
**	A version of NMgtIngAt independent of Ingres CL functions.
**
**  Inputs:
**	strKey				Name of symbol.
**
**  Outputs:
**	strRetValue			Returned value of symbol.
**
**      Returns:
**          ERROR_SUCCESS		Function completed normally.
**          ERROR_INSTALL_FAILURE	Function completed abnormally.
**      Exceptions:
**          none
**
**  Side Effects:
**      none
**
**  History:
**      22-apr-2001 (somsa01)
**          Created.
**	08-may-2001 (somsa01)
**	    Corrected proper error code returns.
*/

int
Local_NMgtIngAt(char *strKey, char *strRetValue)
{
    FILE	*fPtr;
    char	szBuffer[MAX_PATH+1];
    char	szFileName[MAX_PATH+1];
    char	*p, *q;

	*strRetValue=0;

    /* Check system environment variable */
    if (GetEnvironmentVariable(strKey, szBuffer, sizeof(szBuffer)))
    {
	strcpy(strRetValue, szBuffer);
	return (ERROR_SUCCESS);
    }

    /* Check symbol.tbl file */
    if (!GetEnvironmentVariable("II_SYSTEM", szBuffer, sizeof(szBuffer)))
	return (ERROR_INSTALL_FAILURE);

    sprintf(szFileName, "%s\\ingres\\files\\symbol.tbl", szBuffer);
    if (!(fPtr = fopen(szFileName, "r")))
	return (ERROR_INSTALL_FAILURE);

    while (fgets(szBuffer, sizeof(szBuffer), fPtr) != NULL)
    {
	p = strstr(szBuffer, strKey);
	if (p)
	{
	    /* We found it! */
	    q = strchr(szBuffer, '\t');
	    if (q)
	    {
		q++;
		p = &szBuffer[strlen(szBuffer)-1];
		while (isspace(*p))
		    p--;
		*(p+1) = '\0';

		strcpy(strRetValue, q);
		fclose(fPtr);
		return (ERROR_SUCCESS);
	    }
	    else
	    {
		/* Something's not right. */
		break;
	    }
	}
    }
  
  fclose(fPtr);
  return (ERROR_INSTALL_FAILURE);  /* Value not found */
}

/*{
**  Name: RemoveDir
**
**  Description:
**	Removes a directory from the system, which can contain either
**	files or directories.
**
**  Inputs:
**	DirName				Name of directory to delete.
**	DeleteDir			Do we want to delete this directory,
**					or just remove what's underneath?
**
**  Outputs:
**      Returns:
**          ERROR_SUCCESS		Function completed normally.
**          ERROR_INSTALL_FAILURE	Function completed abnormally.
**      Exceptions:
**          none
**
**  Side Effects:
**      none
**
**  History:
**      22-apr-2001 (somsa01)
**          Created.
*/

int
RemoveDir(char *DirName, BOOL DeleteDir)
{
    HANDLE		hFile;
    WIN32_FIND_DATA	FindFileData;
    char		SearchString[MAX_PATH+1];
    char		FullFilePath[MAX_PATH+1];
    int			MoreData = 1;

    /*
    ** First, delete all that is in the directory.
    */
    sprintf(SearchString, "%s\\*", DirName);
    if ((hFile = FindFirstFile(SearchString, &FindFileData)) ==
	INVALID_HANDLE_VALUE)
    {
	return (ERROR_SUCCESS);	/* The directory does not exist. */
    }

    while (MoreData)
    {
	if (strcmp(FindFileData.cFileName, ".") != 0 &&
	    strcmp(FindFileData.cFileName, "..") != 0)
	{
	    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
		sprintf(FullFilePath, "%s\\%s", DirName,
			FindFileData.cFileName);
		if (RemoveDir(FullFilePath, TRUE) != ERROR_SUCCESS)
		{
		    FindClose(hFile);
		    return (ERROR_INSTALL_FAILURE);
		}
	    }
	    else
	    {
		sprintf(FullFilePath, "%s\\%s", DirName,
			FindFileData.cFileName);
		if (!DeleteFile(FullFilePath))
		{
		    FindClose(hFile);
		    return (ERROR_INSTALL_FAILURE);
		}
	    }
	}

	MoreData = FindNextFile(hFile, &FindFileData);
    }

    FindClose(hFile);
    if (DeleteDir)
    {
	if (!RemoveDirectory(DirName))
	    return (ERROR_INSTALL_FAILURE);
    }

    return (ERROR_SUCCESS);
}

/*{
**  Name: Local_PMget
**
**  Description:
**	A version of PMget independent of Ingres CL functions.
**
**  Inputs:
**	strKey				Name of symbol.
**
**  Outputs:
**	strRetValue			Returned value of symbol.
**
**      Returns:
**          ERROR_SUCCESS		Function completed normally.
**          ERROR_INSTALL_FAILURE	Function completed abnormally.
**      Exceptions:
**          none
**
**  Side Effects:
**      none
**
**  History:
**      22-apr-2001 (somsa01)
**          Created.
*/

int
Local_PMget(char *strKey, char *strRetValue)
{
    FILE	*fPtr;
    char	szBuffer[MAX_PATH+1];
    char	szFileName[MAX_PATH+1];
    char	*p, *q;

	*strRetValue=0;

    /* Check config.dat file */
    if (!GetEnvironmentVariable("II_SYSTEM", szBuffer, sizeof(szBuffer)))
	return (ERROR_INSTALL_FAILURE);
 
    sprintf(szFileName, "%s\\ingres\\files\\config.dat", szBuffer);
    if (!(fPtr = fopen(szFileName, "r")))
	return (ERROR_INSTALL_FAILURE);
 
    while (fgets(szBuffer, sizeof(szBuffer), fPtr) != NULL)
    {
	p = strstr(szBuffer, strKey);
	if (p)
	{
	    /* We found it! */
	    q = strchr(szBuffer, ':');
	    if (q)
	    {
		while (!isalnum(*q))
		    q++;

		p = &szBuffer[strlen(szBuffer)-1];
		while (isspace(*p))
		    p--;
		if (*p=='\'') p--;
		*(p+1) = '\0';

		strcpy(strRetValue, q);
		fclose(fPtr);
		return (ERROR_SUCCESS);
	    }
	    else
	    {
		/* Something's not right. */
		break;
	    }
	}
    }
  
    fclose(fPtr);
    return (ERROR_INSTALL_FAILURE);  /* Value not found */
}

/*{
**  Name: Execute
**
**  Description:
**	Executes a process.
**
**  Inputs:
**	lpCmdLine			Command line to execute.
**	bwait				Should we wait for the child to
**					finish?
**
**  Outputs:
**	none
**
**      Returns:
**          ERROR_SUCCESS		Function completed normally.
**          ERROR_INSTALL_FAILURE	Function completed abnormally.
**      Exceptions:
**          none
**
**  Side Effects:
**      none
**
**  History:
**      09-may-2001 (somsa01)
**          Created.
**	    20-aug-2001 (penga03)
**	        Close the opened handles before return.
*/

int
Execute(char *lpCmdLine, BOOL bwait)
{
    PROCESS_INFORMATION	pi;
    STARTUPINFO		si;

    memset((char*)&pi, 0, sizeof(pi)); 
    memset((char*)&si, 0, sizeof(si)); 
    si.cb = sizeof(si);  

    if (CreateProcess ( NULL, lpCmdLine, NULL, NULL, TRUE,
			DETACHED_PROCESS | NORMAL_PRIORITY_CLASS, NULL,
			NULL, &si, &pi ))
    {
	if (bwait)
	{
	    DWORD   dw;

	    WaitForSingleObject(pi.hProcess, INFINITE);
	    if (GetExitCodeProcess(pi.hProcess, &dw))
		{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return (dw == 0 ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE);
		}
		else
		{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		}
	}
	else
	{
	    CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);
	    return (ERROR_SUCCESS);
	}
    }
    return (ERROR_INSTALL_FAILURE);
}


/*
**  Name: ExecuteEx
**
**  Description:
**	Launch an executable without creating window.
**
**  History:
**	17-jan-2002 (penga03)
**	    Created.
*/
BOOL 
ExecuteEx(char *lpCmdLine)
{
    PROCESS_INFORMATION	pi;
    STARTUPINFO		si;
	
    memset((char*)&pi, 0, sizeof(pi));
    memset((char*)&si, 0, sizeof(si));
    si.cb = sizeof(si);

    if (CreateProcess(NULL, lpCmdLine, NULL, NULL, TRUE, 
                      CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, 
                      NULL, NULL, &si, &pi))
    {
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (TRUE);
    }
    return (FALSE);
}

/*
**  Name: ExecuteShellEx
**
**  Description:
**	Launch an application using ShellExecute.
**	Primarily used to launch a browser.
**
**  History:
**	11-jul-2007 (drivi01)
**	    Created.
*/
BOOL
ExecuteShellEx(char *lpCmdLine)
{
	SHELLEXECUTEINFO shex;

	memset(&shex, 0, sizeof(shex));
	
	shex.cbSize	= sizeof(SHELLEXECUTEINFO);
	shex.hwnd	= NULL;
	shex.lpVerb	= "open";
	shex.lpFile	= lpCmdLine;
	shex.nShow	= SW_NORMAL;
	
	return ShellExecuteEx(&shex);

}

/*
**  Name: VerifyPath
**
**  Description:
**	Verify a path.
**
**  Inputs:
**	szPath2Verify	The path to be verified.
**
**  Outputs:
**	None.
**	Returns:
**	    TRUE	The path is valid.
**	    FALSE	The path is invalid.
**	Exceptions:
**	    None.
**
**  History:
**	09-may-2001 (penga03)
**	    Created.
*/
BOOL 
VerifyPath(char *szPath)
{
    unsigned char *q;
    char *p;
    int iLen;
    char szDrive[3];
	
    if (!szPath[0])
	return FALSE;

    iLen=_tcsclen(szPath);
    if (iLen < MIN_DIR || iLen > _MAX_DIR ) 
	return FALSE;
	
    while ( _tcsclen(szPath) > (MIN_DIR + 1) ) 
    {
	q = (unsigned char *)_tcsdec(szPath, szPath + _tcslen(szPath));
	if (*q == '\\' || *q == '/' || *q <= ' ')
	    *q=0;
	else
	    break;
    }
	
    for (p=szPath; *p; p=_tcsinc(p))
    {
	if (*p == ',' || *p == '?' || *p == '|' || *p == '*' ||
	    *p == '%' || *p == '<' || *p == '>' || *p == '&')
	    return FALSE;
    }
	
    strncpy(szDrive, szPath, 2);
    szDrive[2]='\0';
    if (GetFileAttributes(szDrive) == -1)
	return FALSE;

    return TRUE;
}

/*
**  Name: GetRegistryEntry
**
**  Description:
**	Obtain the data for a specified value name associated with
**	an open registry key's subkey.
**
**  Inputs:
**	hRootKey	A currently open key or one of the four predefined reserved keys.
**	subKey		Name of a subkey of the opened key. 	
**	name		Name of a value to query.
**
**  Outputs:
**	value		Returned data for the specified value name.
**	Returns:
**	    -1		Passed wrong arguments.
**	    0		Don't retrieve the data.
**	    1		Retrieve the data.
**	Exceptions:
**	    None.
**
**  History:
**	11-may-2001 (penga03)
**	    Created.
*/
int 
GetRegistryEntry(HKEY hRootKey, char *subKey, char *name, char *value)
{ 
	DWORD size, type;
	char data[MAX_PATH+1];
	HKEY hkResult;
	int iKeyFound;
	
	iKeyFound=0;
	size=sizeof(data);
	
	if((!subKey) || (!name) || (!value))
		return (-1);
	
	if(RegOpenKeyEx(hRootKey, subKey, 0, KEY_ALL_ACCESS, &hkResult)==ERROR_SUCCESS)
	{
		if(RegQueryValueEx(hkResult, name, 0, &type, (BYTE *)data, &size)==ERROR_SUCCESS)
		{
			strcpy(value, data);
			iKeyFound=1;
		}
		RegCloseKey(hkResult);
	}
	
	return (iKeyFound);
}

/*{
**  Name: Local_PMhost() - return PM-modified local host name
**
**  Description:
**	Returns pointer to local host name modified to allow use as
**	a PM name component. Converts illegal characters (i.e. '.')
**	to the underbar character.
**
**  Inputs:
**	lpCmdLine			Command line to execute.
**	bwait				Should we wait for the child to
**					finish?
**
**  Outputs:
**	none
**
**      Returns:
**          ERROR_SUCCESS		Function completed normally.
**          ERROR_INSTALL_FAILURE	Function completed abnormally.
**      Exceptions:
**          none
**
**  Side Effects:
**      none
**
**  History:
**      09-may-2001 (somsa01)
**          Created.
*/

void
Local_PMhost(char *hostname)
{
    char	pm_hostname[65];
    int		len = sizeof(pm_hostname);

    if (Local_NMgtIngAt("II_HOSTNAME", pm_hostname) == ERROR_SUCCESS)
    {
	strcpy(hostname, _strlwr(pm_hostname));
    }
    else if (GetComputerName(pm_hostname, &len) == TRUE)
    {
	char	*p;

	for (p = pm_hostname; *p != '\0'; p++)
	{
	    switch(*p)
	    {
		case '.':
		case '&':
		case '\'':
		case '$':
		case '#':
		case '!':
		case ' ':
		case '%':
		    *p = '_';
	    }
	}

	strcpy(hostname, _strlwr(pm_hostname));
    }
    else
	strcpy(hostname, "");
}

/*
**  Name: SetRegistryEntry
**
**  Description:
**	Sets the data and type of a specified value under a registry key. 
**
**  Inputs:
**	hKey		Handle to a currently open key.
**	lpSubKey	Handle to a currently open subkey key.
**	lpValueName	Pointer to the name of the value to set.
**	dwType		Specifies the type of data pointed to by the lpData parameter.
**	lpData		Pointer to the data to be stored with the specified value name.
**
**  Outputs:
**	None.
**	Returns:
**	    TRUE	The function succeeds.
**	    FALSE	The function failes.
**	Exceptions:
**	    None.
**
**  History:
**	03-Aug-2001 (penga03)
**	    Created.
*/
BOOL 
SetRegistryEntry(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueName, DWORD dwType, void *lpData)
{
	HKEY hkSubKey;
	BOOL bRet=FALSE;

	if((dwType!=REG_SZ) && (dwType!=REG_DWORD))
		return FALSE;

	if(RegCreateKeyEx(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSubKey, NULL)!=ERROR_SUCCESS)
		return FALSE;

	if((dwType==REG_SZ) && lpData)
		bRet=(RegSetValueEx(hkSubKey, lpValueName, 0, dwType, (CONST BYTE *)lpData, strlen((char *)lpData)+1)==ERROR_SUCCESS);
	
	if((dwType==REG_DWORD) && lpData)
		bRet=(RegSetValueEx(hkSubKey, lpValueName, 0, dwType, (CONST BYTE *)&lpData, sizeof(lpData))==ERROR_SUCCESS);

	RegCloseKey(hkSubKey);
	return bRet;
}

/*
**  Name: StringReverse
**
**  Description:
**	Reverse tokens of a string.
**
**  Inputs:
**	string	Null-terminated string to reverse.
**
**  Outputs:
**	string	Null-terminated string reversed.
**	Returns:
**	    None.
**	Exceptions:
**	    None.
**
**  History:
**	03-Aug-2001 (penga03)
**	    Created.
*/
void
StringReverse(char *string)
{
	char tmp1[1024], tmp2[1024], *token, *tokens[10];
	int i;
	char seps[]=",";

	strcpy(tmp1, string);
	token=strtok(tmp1, seps);
	i=0;
	while(token != NULL)
	{
		tokens[i]=token;
		token=strtok(NULL, seps );
		i++;
	}
	
	i=i-1;
	strcpy(tmp2, tokens[i]);
	for(--i; i>=0; i--)
	{
		strcat(tmp2, ",");
		strcat(tmp2, tokens[i]);
	}
	strcpy(string, tmp2);
}

/*
**  Name: StringReplace
**
**  Description:
**	Replace \\ with \ in a string. 
**
**  Inputs:
**	string	Null-terminated string to replace.
**
**  Outputs:
**	string	Null-terminated string replaced.
**	Returns:
**	    None.
**	Exceptions:
**	    None.
**
**  History:
**	03-Aug-2001 (penga03)
**	    Created.
*/
void
StringReplace(char *string)
{
	char tmp1[MAX_PATH+1], tmp2[MAX_PATH+1], *token, *tokens[10];
	int i, j;

	strcpy(tmp1, string);
	token=strtok(tmp1, "\\\\");
	i=0;
	while(token!=NULL)
	{
		tokens[i]=token;
		token=strtok(NULL, "\\\\");
		i++;
	}

	strcpy(tmp2, tokens[0]);
	for(j=1; j<i; j++)
	{
		strcat(tmp2, "\\");
		strcat(tmp2, tokens[j]);
	}

	strcpy(string, tmp2);
}

/*
**  Name: OtherInstallationsExist
**
**  Description:
**	Check if there exist Ingres installations other than the one being 
**	removed.
**
**  Inputs:
**	cur_code	the identifier of the installation being removed
**
**  Outputs:
**	code		the identifier of an existing Ingres installation
**	Returns:
**	    TRUE	exist
**	    FALSE	not exist
**	Exceptions:
**	    None.
**
**  History:
**	30-oct-2001 (penga03)
**	    Created.
*/
BOOL 
OtherInstallationsExist(char *cur_code, char *code)
{
	char *RegKey="SOFTWARE\\IngresCorporation\\Ingres";
	HKEY hRegKey=0;
	int i=0;
	char code2del[16], file2check[MAX_PATH+1];
	
	strncpy(code2del, cur_code, 2);
	code2del[2]='\0';
	strcat(code2del, "_Installation");

	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hRegKey))
	{
		LONG retCode;
		
		for (i, retCode = 0; !retCode; i++) 
		{
			char SubKey[16];
			DWORD dwSize=sizeof(SubKey);
			HKEY hSubKey=0;
			
			retCode=RegEnumKeyEx(hRegKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
			if(!retCode && _stricmp(SubKey, code2del))
			{
				if(!RegOpenKeyEx(hRegKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
				{
					DWORD dwType;
					char ii_system[MAX_PATH+1];
					DWORD dwSize2=sizeof(ii_system);
					
					if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize2))
					{	
						sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
						if(GetFileAttributes(file2check)!=-1)
						{
							strncpy(code, SubKey, 2);
							code[2]='\0';
							RegCloseKey(hSubKey);
							RegCloseKey(hRegKey);
							
							return TRUE;
						}						
					}
					RegCloseKey(hSubKey);
				}
			}
		} /* end of for loop */
		RegCloseKey(hRegKey);
	}
	return FALSE;
}
/*
**  Name: IsOtherODBCDriver
**
**  Description:
**	Check if there exist Ingres ODBC drivers other than the one being 
**	removed. Only search for Read Only driver if the driver being 
**	removed is read only, or read/write if the driver being removed 
**	is of the same type.
**
**  Inputs:
**	cur_code	the identifier of the installation being removed
**	bReadOnly	whether the driver being removed and being searched
**			for is read only or not
**
**  Outputs:
**	strDrvPath	the path to the still installed driver
**	Returns:
**	    TRUE	exist
**	    FALSE	not exist
**	Exceptions:
**	    None.
**
**  History:
**	15-Sep-2010 (drivi01)
**	    Created.
*/
BOOL 
IsOtherODBCDriver(char *cur_code, char *strDrvPath, BOOL bReadOnly)
{
	char *RegKey = "SOFTWARE\\ODBC\\ODBCINST.INI";
	char *RegKey3 = "SOFTWARE\\IngresCorporation\\Ingres";
	char RegKey2[MAX_PATH+1];
	char RegKey4[MAX_PATH+1];
	char ii_system[MAX_PATH+1];
	char *szReadOnly = "Read Only";
	HKEY hRegKey, hRegKey2;
	int i = 0;
	BOOL bRet = FALSE;
	BOOL bUpdate = FALSE;

	/* First check if the "Ingres Read Only"/"Ingres" Driver 
	** specified in the registry
	** is from the installation/ODBC driver you're removing.
	** If it is not then we don't need to update anything.
	** If it is, then see if there's another driver of the same
	** type (for example Read Only for Read Only or Read/Write for
	** Read/Write available in the different installation 
	*/
	if (bReadOnly)
		sprintf(RegKey2, "%s\\Ingres Read Only", RegKey);
	else
		sprintf(RegKey2, "%s\\Ingres", RegKey);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey2, 0, KEY_QUERY_VALUE, &hRegKey))
	{
		DWORD dwType;
		char drvPath[MAX_PATH+1];
		DWORD dwSize=sizeof(drvPath);
		if (!RegQueryValueEx(hRegKey, "Driver", NULL, &dwType, (BYTE *)drvPath, &dwSize))
		{
			sprintf(RegKey4, "%s\\%s_Installation", RegKey3, cur_code);
			if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey4,0,KEY_QUERY_VALUE,&hRegKey2))
			{
				dwSize=sizeof(ii_system);
				if(!RegQueryValueEx(hRegKey2,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize))
				{
					if (*ii_system && strstr(drvPath, ii_system)!=NULL)
						bUpdate = TRUE;
				}
			}
		}
	}
	if(bUpdate && !RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hRegKey))
	{
		LONG retCode;
		for (i, retCode = 0; !retCode; i++)
		{
			char subKey[MAX_PATH];
			DWORD dwSize;
			HKEY hSubKey;

			dwSize=sizeof(subKey);
			retCode = RegEnumKeyEx(hRegKey, i, subKey, &dwSize, NULL, NULL, NULL, NULL);
			if (!retCode && strstr(subKey, cur_code) == NULL && strstr(subKey, "Ingres") != NULL && 
				stricmp(subKey, "Ingres") != 0 && stricmp(subKey, "Ingres Read Only") != 0 &&
				((bReadOnly && strstr(subKey, szReadOnly)!=NULL) ||
					(!bReadOnly && strstr(subKey, szReadOnly) == NULL)))
			{
				if (!RegOpenKeyEx(hRegKey, subKey, 0, KEY_QUERY_VALUE, &hSubKey))
				{
					DWORD dwType;
					char drvPath[MAX_PATH+1];
					char drvType[MAX_PATH+1];
					DWORD dwSize2=sizeof(drvType);
					if (!RegQueryValueEx(hSubKey, "DriverReadOnly", NULL, &dwType, (BYTE *)drvType, &dwSize2))
					{
						if ((bReadOnly && stricmp(drvType, "Y") == 0) ||
							(!bReadOnly && stricmp(drvType, "N") == 0 ))
						{
						dwSize2=(sizeof(drvPath));
						if (!RegQueryValueEx(hSubKey,"Driver",NULL,&dwType,(BYTE *)drvPath,&dwSize2))
						{
							strcpy(strDrvPath, drvPath);
							RegCloseKey(hSubKey);
							bRet = TRUE;
							break;
						}
						}
					}
				}
				RegCloseKey(hSubKey);
			} 
		} /* for */
	}
	RegCloseKey(hRegKey);
	return bRet;
}

/*
**  Name: GetInstallPath
**
**  Description:
**	Get the II_SYSTEM according to the installation id input.
**
**  History:
**	10-dec-2001 (penga03)
**	    Created.
**	17-oct-2002 (penga03)
**	    Initialize strInstallPath.
*/
BOOL 
GetInstallPath(char *strInstallCode, char *strInstallPath)
{
	char szKey[1024];
	HKEY hKey=0;
	
	*strInstallPath=0;

	sprintf(szKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", strInstallCode);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey)==ERROR_SUCCESS)
	{
		char ii_system[MAX_PATH];
		DWORD dwType, dwSize=sizeof(ii_system);
		
		if(RegQueryValueEx(hKey,"II_SYSTEM", NULL, &dwType, (BYTE *)ii_system,&dwSize)==ERROR_SUCCESS)
		{
			strcpy(strInstallPath, ii_system);
			RegCloseKey(hKey);
			return TRUE;
		}
		RegCloseKey(hKey);
	}
	
	return FALSE;
}

BOOL 
IsWindows95()
{
	OSVERSIONINFO osver; 
	memset((char *) &osver,0,sizeof(osver)); 
	osver.dwOSVersionInfoSize=sizeof(osver); 
	GetVersionEx(&osver);
	
	if((osver.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) && (osver.dwMajorVersion>3))
		return (TRUE);
	return (FALSE);
}

BOOL 
IsVista()
{
	OSVERSIONINFO osver; 
	memset((char *) &osver,0,sizeof(osver)); 
	osver.dwOSVersionInfoSize=sizeof(osver); 
	GetVersionEx(&osver);
	
	if((osver.dwPlatformId==VER_PLATFORM_WIN32_NT) && (osver.dwMajorVersion>=6))
		return (TRUE);
	return (FALSE);
}

/*
**  Name: CreateDirectoryRecursively
**
**  Description:
**	
**
**  History:
**	14-may-2002 (penga03)
**	    Created.
*/
int 
CreateDirectoryRecursively(char *szDir)
{ 
    char szBuf[MAX_PATH], szNewDir[MAX_PATH], *p;
    int rc;
	
    if (!szDir)
	return (-1);
    if (GetFileAttributes(szDir)!=-1)
	return (0);
    
    strcpy(szNewDir, szDir);
    p=strtok(szNewDir, "\\");
    if (p) strcpy(szBuf, p);
    while (p)
    {
	p=strtok(NULL, "\\");
	if (p)
	{
	    strcat(szBuf, "\\");
	    strcat(szBuf, p);
	    rc=CreateDirectory(szBuf, NULL);
	}
    }
    return (0);
}

/*
**  Name: CreateOneShortcut
**
**  Description:
**	
**
**  History:
**	14-may-2002 (penga03)
**	    Created.
*/
int 
CreateOneShortcut(char *szFolder, char *szDisplayName, char *szTarget, 
                  char *szArguments, char *szIconFile, char *szWorkDir, 
                  char *szDescription, BOOL Startup, BOOL AllUsers)
{
    LPITEMIDLIST pidlStartMenu;
    char szPath[MAX_PATH], szLinkFile[MAX_PATH];
    HRESULT hres; 
    IShellLink *pShellLink;
    LPMALLOC pMalloc = NULL;

    CoInitialize(NULL);
    SHGetMalloc(&pMalloc);

    if(Startup)
    {
	if(AllUsers)
	    SHGetFolderLocation(NULL, CSIDL_COMMON_STARTUP, NULL, 0, &pidlStartMenu);
	else
	    SHGetFolderLocation(NULL, CSIDL_STARTUP, NULL, 0, &pidlStartMenu);
	SHGetPathFromIDList(pidlStartMenu, szPath);
    }
    else
    {
	if(AllUsers)
	    SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlStartMenu);
	else
	    SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlStartMenu);
	SHGetPathFromIDList(pidlStartMenu, szPath);

	strcat(szPath, "\\");
	strcat(szPath, szFolder);
	CreateDirectoryRecursively(szPath);
	SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, szPath, 0);
    }

    /*Get a pointer to the IShellLink interface.*/
    hres=CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                          &IID_IShellLink, (LPVOID *)&pShellLink);
    if(SUCCEEDED(hres))
    {
	IPersistFile *pPersistFile;
	
	hres=pShellLink->lpVtbl->SetPath(pShellLink, szTarget);
	if (szArguments)
	    hres=pShellLink->lpVtbl->SetArguments(pShellLink, szArguments);
	if (szIconFile)
	    hres=pShellLink->lpVtbl->SetIconLocation(pShellLink, szIconFile, 0);
	if (szWorkDir)
	    hres=pShellLink->lpVtbl->SetWorkingDirectory(pShellLink, szWorkDir);
	if (szDescription)
	    hres=pShellLink->lpVtbl->SetDescription(pShellLink, szDescription);

	/*
	Query IShellLink for the IPersistFile interface for saving the 
	shortcut in persistent storage. 
	*/
	sprintf(szLinkFile, "%s\\%s.lnk", szPath, szDisplayName);
	hres = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IPersistFile, (LPVOID *)&pPersistFile);
	if(SUCCEEDED(hres))
	{
	    WCHAR wsz[MAX_PATH];
		
	    /*Ensure that the string is Unicode.*/
	    MultiByteToWideChar(CP_ACP, 0, szLinkFile, -1, wsz, MAX_PATH);
	    /*Save the link by calling IPersistFile::Save.*/
	    hres=pPersistFile->lpVtbl->Save(pPersistFile, wsz, TRUE);
	    pPersistFile->lpVtbl->Release(pPersistFile);
	}
	pShellLink->lpVtbl->Release(pShellLink);
    }

    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlStartMenu, NULL);

    if (pidlStartMenu)
	pMalloc->lpVtbl->Free(pMalloc, pidlStartMenu);

    pMalloc->lpVtbl->Release(pMalloc);
    CoUninitialize();
    return (0);
}

/*
**  Name: CreateOneAdminShortcut
**
**  Description: Creates shortcut which always runs with elevated 
**		 privileges. (Vista shortcut)
**	
**
**  History:
**	13-Jul-2007 (drivi01)
**	    Created.
*/
int 
CreateOneAdminShortcut(char *szFolder, char *szDisplayName, char *szTarget, 
                  char *szArguments, char *szIconFile, char *szWorkDir, 
                  char *szDescription, BOOL Startup, BOOL AllUsers)
{
    LPITEMIDLIST pidlStartMenu;
    char szPath[MAX_PATH], szLinkFile[MAX_PATH];
    HRESULT hres; 
    IShellLink *pShellLink;
    IShellLinkDataList* pdl;
    LPMALLOC pMalloc = NULL;
    DWORD dwFlags = 0;

    CoInitialize(NULL);
    SHGetMalloc(&pMalloc);

    if(Startup)
    {
	if(AllUsers)
	    SHGetFolderLocation(NULL, CSIDL_COMMON_STARTUP, NULL, 0, &pidlStartMenu);
	else
	    SHGetFolderLocation(NULL, CSIDL_STARTUP, NULL, 0, &pidlStartMenu);
	SHGetPathFromIDList(pidlStartMenu, szPath);
    }
    else
    {
	if(AllUsers)
	    SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlStartMenu);
	else
	    SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlStartMenu);
	SHGetPathFromIDList(pidlStartMenu, szPath);

	strcat(szPath, "\\");
	strcat(szPath, szFolder);
	CreateDirectoryRecursively(szPath);
	SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, szPath, 0);
    }

    /*Get a pointer to the IShellLink interface.*/
    hres=CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                          &IID_IShellLink, (LPVOID *)&pShellLink);
    if(SUCCEEDED(hres))
    {
	IPersistFile *pPersistFile;
	
	hres=pShellLink->lpVtbl->SetPath(pShellLink, szTarget);
	if (szArguments)
	    hres=pShellLink->lpVtbl->SetArguments(pShellLink, szArguments);
	if (szIconFile)
	    hres=pShellLink->lpVtbl->SetIconLocation(pShellLink, szIconFile, 0);
	if (szWorkDir)
	    hres=pShellLink->lpVtbl->SetWorkingDirectory(pShellLink, szWorkDir);
	if (szDescription)
	    hres=pShellLink->lpVtbl->SetDescription(pShellLink, szDescription);

	/*
	Query IShellLink for the IPersistFile interface for saving the 
	shortcut in persistent storage. 
	*/
	sprintf(szLinkFile, "%s\\%s.lnk", szPath, szDisplayName);
	hres = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IPersistFile, (LPVOID *)&pPersistFile);
	if(SUCCEEDED(hres))
	{
	    WCHAR wsz[MAX_PATH];
		
	    /*Ensure that the string is Unicode.*/
	    MultiByteToWideChar(CP_ACP, 0, szLinkFile, -1, wsz, MAX_PATH);
	    
            hres = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IShellLinkDataList, (LPVOID *)&pdl);
	    if (SUCCEEDED(hres))
	    {
            	hres = pdl->lpVtbl->GetFlags(pdl, &dwFlags);
		/*Set shortcut to always run as Administrator user (elevated privileges)*/
	    	if (SUCCEEDED(hres) && (dwFlags & SLDF_RUNAS_USER) != SLDF_RUNAS_USER)
		     hres = pdl->lpVtbl->SetFlags(pdl, dwFlags | SLDF_RUNAS_USER);
	    }

	    /*Save the link by calling IPersistFile::Save.*/
	    hres=pPersistFile->lpVtbl->Save(pPersistFile, wsz, TRUE);
	    hres=pPersistFile->lpVtbl->SaveCompleted(pPersistFile, wsz);
	    pPersistFile->lpVtbl->Release(pPersistFile);
	}
	pShellLink->lpVtbl->Release(pShellLink);
    }

    SHChangeNotify(SHCNE_ALLEVENTS, SHCNF_IDLIST, pidlStartMenu, NULL);

    if (pidlStartMenu)
	pMalloc->lpVtbl->Free(pMalloc, pidlStartMenu);

    pMalloc->lpVtbl->Release(pMalloc);
    CoUninitialize();
    return (0);
}


/*
**  Name: GetRefCount
**
**  Description:
**	Check if there are other products referencing the component
**	being removed.
**
**  History:
**	31-may-2002 (penga03)
**	    Created.
**	08-aug-2002 (penga03)
**	    Corrected an error, put the first return inside its if 
**	    statement.
*/
int 
GetRefCount(char *szComponentGUID, char *szProductGUID)
{
    DWORD ret;
    int i, refcount;
    char szProductBuf[256];
	
    i=refcount=0;
    do
    {
	ret=MsiEnumClients(szComponentGUID, i, szProductBuf);
	if (!ret) 
	{
	    i++;
	    if (_stricmp(szProductBuf, szProductGUID)) 
		refcount++;
	}
	else break;
    } while (!ret);

    return refcount;
}

/*
**  Name: GetLinkInfo
**
**  Description:
**	Obtain the path and file name of a given link file.
**
**  History:
**	01-aug-2002 (penga03)
**	    Created. lpszLinkName is name of the link file, lpszPath 
**	    is the buffer that receives the file's path.
**	09-jul-2003 (penga03)
**	    Convert the string, lpszPath, to lowercase before return.
*/
BOOL 
GetLinkInfo(LPCTSTR lpszLinkName, LPSTR lpszPath)
{
    HRESULT hres;
    IShellLink *pShLink;
    WIN32_FIND_DATA wfd;
    BOOL bFound=FALSE;

    *lpszPath='\0';
    hres=CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                          &IID_IShellLink, (LPVOID *)&pShLink);
    if (SUCCEEDED(hres))
    {
	IPersistFile *ppf;
	
	hres=pShLink->lpVtbl->QueryInterface(pShLink, &IID_IPersistFile, (LPVOID *)&ppf);
	if (SUCCEEDED(hres))
	{
	    WORD wsz[MAX_PATH];
		
	    MultiByteToWideChar(CP_ACP, 0, lpszLinkName, -1, wsz, MAX_PATH);
		hres=ppf->lpVtbl->Load(ppf, wsz, STGM_READ);
	    if (SUCCEEDED(hres))
		{
		hres=pShLink->lpVtbl->GetPath(pShLink, lpszPath, MAX_PATH, &wfd, SLGP_SHORTPATH);
		if (lpszPath) bFound=TRUE;
		}
	    ppf->lpVtbl->Release(ppf);
	}
	pShLink->lpVtbl->Release(pShLink);
    }

    if (bFound) _tcslwr(lpszPath);
    return bFound;
}

BOOL 
MyMessageBox(MSIHANDLE hInstall, char *lpText)
{   
	char buf[MAX_PATH+1], lpCaption[512];
	DWORD nret, rc;
	HWND hWnd;

	nret=sizeof(buf);
	MsiGetProperty(hInstall, "ProductName", buf, &nret);
	sprintf(lpCaption, "%s - Installation Wizard", buf);
	
	hWnd=FindWindow(NULL, lpCaption);
	rc=MessageBox(hWnd, lpText, lpCaption, MB_OK|MB_ICONEXCLAMATION);
	return rc;
}

/*
**  Name: AskUserYN
**
**  History:
**	10-dec-2001 (penga03)
**	    Created.
*/
BOOL 
AskUserYN(MSIHANDLE hInstall, char *lpText)
{   
	char buf[MAX_PATH+1], lpCaption[512];
	DWORD nret;
	HWND hWnd;
	
	nret=sizeof(buf);
	MsiGetProperty(hInstall, "ProductName", buf, &nret);
	sprintf(lpCaption, "%s - Installation Wizard", buf);
	
	hWnd=FindWindow(NULL, lpCaption);
	return ((MessageBox(hWnd, lpText, lpCaption, MB_YESNO|MB_ICONQUESTION)==IDYES));
}
/*
**  Name: AskUserOkCancel
**
**  History:
**	12-Jul-2007 (drivi01)
**	    Created.
*/
BOOL
AskUserOkCancel(MSIHANDLE hInstall, char *lpText)
{
	char buf[MAX_PATH+1], lpCaption[512];
	DWORD nret;
	HWND hWnd;
	
	nret=sizeof(buf);
	MsiGetProperty(hInstall, "ProductName", buf, &nret);
	sprintf(lpCaption, "%s - Installation Wizard", buf);
	
	hWnd=FindWindow(NULL, lpCaption);
	return ((MessageBox(hWnd, lpText, lpCaption, MB_OKCANCEL|MB_ICONWARNING)==IDOK));	
}

/*
**  Name: VerifyInstance
**
**  Description:
**	Given II_INSTALLATION and II_SYSTEM verify if it is able to be 
**	installed or upgraded.
**
**  History:
**	26-jun-2003 (penga03)
**	    Created.
**	20-oct-2003 (penga03)
**	    Added more checks, verifying installation identifier, INGRESFOLDER 
**	    and making sure Enterprise and Embedded are not convertible.
**	    Modified the return type to int. If returns -1, the caller should 
**	    return ERROR_INSTALL_FAILURE; if returns 0, the caller should return 
**	    ERROR_SUCCESS; if returns 1, the caller should set INGRES_CONTINUE 
**	    to "NO", and return ERROR_SUCCESS.
*/
int 
VerifyInstance(MSIHANDLE hInstall, char *id, char *instdir)
{   
    char szBuf[MAX_PATH],RegKey[1024];
    char szII_SYSTEM[MAX_PATH], szId[3];
    HKEY hRegKey=0;
    BOOL bUpgradeNonMsi;
    DWORD dwSize;

    if (!id || !*id || strlen(id)!=2 || !isalpha(id[0]) || !isalnum(id[1]))
    {
	sprintf(szBuf, "Invalid installation identifier : '%s'!\nInstallation identifier must be 2 characters, beginning with a non-numeric character.", id);
	MyMessageBox(hInstall, szBuf);
	return -1;
    }
	
    if (!instdir || !*instdir)
    {
	sprintf(szBuf, "You must set INGRESFOLDER as the destination folder of Ingres in Directory table!");
	MyMessageBox(hInstall, szBuf);
	return -1;
    }
    if (instdir[strlen(instdir)-1] == '\\')
	instdir[strlen(instdir)-1] = '\0';

    /* 
    ** The ETRUSTDIR_UPGRADE_INGRES20 property is ONLY used by 
    ** eTrust Directory. 
    */
    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "ETRUSTDIR_UPGRADE_INGRES20", szBuf, &dwSize);
    if (!strcmp(szBuf, "1")) 
	return 0;

    /* 
    ** Upgrading Ingres should comply following rules: 
    ** (1) Two different Ingres instances can neither share the same 
    **     installation identifier nor be installed to the same destination 
    **     folder;
    ** (4) An Ingres installation installed by CA-installer can not be 
    **     upgraded to its MSI version. This rule can be turned off by 
    **     setting the value of the INGRES_UPGRADE_NONMSIINGRES property 
    **     to 1.
    */
    bUpgradeNonMsi=0;
    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_UPGRADE_NONMSIINGRES", szBuf, &dwSize);
    if (!strcmp(szBuf, "1")) 
	bUpgradeNonMsi=1;

    if (GetEnvironmentVariable("II_SYSTEM", szII_SYSTEM, sizeof(szII_SYSTEM))
       && !Local_NMgtIngAt("II_INSTALLATION", szId))
    {
	sprintf(szBuf, "%s\\ingres\\files\\config.dat", szII_SYSTEM);
	if (GetFileAttributes(szBuf) == -1)
	    return 0;

	/* II_INSTALLATION and II_SYSTEM must be unique. */
	if (!_stricmp(id, szId) && _stricmp(instdir, szII_SYSTEM))
	{
	    sprintf(szBuf, "The Ingres instance '%s' has already been installed in '%s', you can NOT re-install it to '%s'!", id, szII_SYSTEM, instdir);
	    MyMessageBox(hInstall, szBuf);
	    return 1;
	}
	if (_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
	{
	    sprintf(szBuf, "You already has an Ingres instance '%s' installed in '%s' and can NOT install another instance '%s' in the same directory!", szId, szII_SYSTEM, id);
	    MyMessageBox(hInstall, szBuf);
	    return 1;
	}
	if (!_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
	{
	    /* The instance was already installed. */
	    /* 
	    ** Do NOT upgrade a non-MSI installation.
	    */
	    if (CheckVer25(id, instdir) && !bUpgradeNonMsi)
	    {
		sprintf(szBuf, "The Ingres instance '%s' was already installed by CA-Installer.", id);
		MyMessageBox(hInstall, szBuf);
		return -1;
	    }

	    /* 
	    ** This installation can be upgraded.
	    */
	    return 0;
	}
    }
	
    sprintf(RegKey, "SOFTWARE\\IngresCorporation\\Ingres");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hRegKey))
    {
	LONG retCode;
	int i=0;
	
	for (i=0, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwSize=sizeof(SubKey);
	    HKEY hSubKey=0;
	    
	    retCode=RegEnumKeyEx(hRegKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		if(!RegOpenKeyEx(hRegKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    DWORD dwSize=sizeof(szII_SYSTEM);
		    
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,NULL,(BYTE *)szII_SYSTEM,&dwSize))
		    {
			sprintf(szBuf, "%s\\ingres\\files\\config.dat", szII_SYSTEM);
			if (GetFileAttributes(szBuf) == -1)
			{
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}
				
			strncpy(szId, SubKey, 2);
			szId[2]='\0';
			
			if (!_stricmp(id, szId) && _stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "The Ingres instance '%s' has already been installed in '%s', you can NOT re-install it to '%s'!", id, szII_SYSTEM, instdir);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "You already has an Ingres instance '%s' installed in '%s' and can NOT install another instance '%s' in the same directory!", szId, szII_SYSTEM, id);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (!_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    if (CheckVer25(id, instdir) && !bUpgradeNonMsi)
			    {
				sprintf(szBuf, "The Ingres instance '%s' was already installed by CA-Installer.", id);
				MyMessageBox(hInstall, szBuf);
				RegCloseKey(hSubKey);
				RegCloseKey(hRegKey);
				return -1;
			    }

			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hRegKey);
    }

    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Ingres");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hRegKey))
    {
	LONG retCode;
	int i=0;
	
	for (i=0, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwSize=sizeof(SubKey);
	    HKEY hSubKey=0;
	    
	    retCode=RegEnumKeyEx(hRegKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		if(!RegOpenKeyEx(hRegKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    DWORD dwSize=sizeof(szII_SYSTEM);
		    
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,NULL,(BYTE *)szII_SYSTEM,&dwSize))
		    {
			sprintf(szBuf, "%s\\ingres\\files\\config.dat", szII_SYSTEM);
			if (GetFileAttributes(szBuf) == -1)
			{
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}
				
			strncpy(szId, SubKey, 2);
			szId[2]='\0';
			
			if (!_stricmp(id, szId) && _stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "The Ingres instance '%s' has already been installed in '%s', you can NOT re-install it to '%s'!", id, szII_SYSTEM, instdir);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "You already has an Ingres instance '%s' installed in '%s' and can NOT install another instance '%s' in the same directory!", szId, szII_SYSTEM, id);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (!_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    if (CheckVer25(id, instdir) && !bUpgradeNonMsi)
			    {
				sprintf(szBuf, "The Ingres instance '%s' was already installed by CA-Installer.", id);
				MyMessageBox(hInstall, szBuf);
				RegCloseKey(hSubKey);
				RegCloseKey(hRegKey);
				return -1;
			    }

			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hRegKey);
    }
	
    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\Advantage Ingres");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hRegKey))
    {
	LONG retCode;
	int i=0;
	
	for (i=0, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwSize=sizeof(SubKey);
	    HKEY hSubKey=0;
	    
	    retCode=RegEnumKeyEx(hRegKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		if(!RegOpenKeyEx(hRegKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    DWORD dwSize=sizeof(szII_SYSTEM);
		    
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,NULL,(BYTE *)szII_SYSTEM,&dwSize))
		    {
			sprintf(szBuf, "%s\\ingres\\files\\config.dat", szII_SYSTEM);
			if (GetFileAttributes(szBuf) == -1)
			{
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}
				
			strncpy(szId, SubKey, 2);
			szId[2]='\0';
			
			if (!_stricmp(id, szId) && _stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "The Ingres instance '%s' has already been installed in '%s', you can NOT re-install it to '%s'!", id, szII_SYSTEM, instdir);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "You already has an Ingres instance '%s' installed in '%s' and can NOT install another instance '%s' in the same directory!", szId, szII_SYSTEM, id);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (!_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    if (CheckVer25(id, instdir) && !bUpgradeNonMsi)
			    {
				sprintf(szBuf, "The Ingres instance '%s' was already installed by CA-Installer.", id);
				MyMessageBox(hInstall, szBuf);
				RegCloseKey(hSubKey);
				RegCloseKey(hRegKey);
				return -1;
			    }

			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hRegKey);
    }

    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKey,0,KEY_ENUMERATE_SUB_KEYS,&hRegKey))
    {
	LONG retCode;
	int i=0;
	
	for (i=0, retCode=0; !retCode; i++) 
	{
	    char SubKey[16];
	    DWORD dwSize=sizeof(SubKey);
	    HKEY hSubKey=0;
	    
	    retCode=RegEnumKeyEx(hRegKey,i,SubKey,&dwSize,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		if(!RegOpenKeyEx(hRegKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey))
		{
		    DWORD dwSize=sizeof(szII_SYSTEM);
		    
		    if(!RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,NULL,(BYTE *)szII_SYSTEM,&dwSize))
		    {
			sprintf(szBuf, "%s\\ingres\\files\\config.dat", szII_SYSTEM);
			if (GetFileAttributes(szBuf) == -1)
			{
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}

			strncpy(szId, SubKey, 2);
			szId[2]='\0';
			
			if (!_stricmp(id, szId) && _stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "The Ingres instance '%s' has already been installed in '%s', you can NOT re-install it to '%s'!", id, szII_SYSTEM, instdir);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    sprintf(szBuf, "You already has an Ingres instance '%s' installed in '%s' and can NOT install another instance '%s' in the same directory!", szId, szII_SYSTEM, id);
			    MyMessageBox(hInstall, szBuf);
			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 1;
			}
			if (!_stricmp(id, szId) && !_stricmp(instdir, szII_SYSTEM))
			{
			    if (CheckVer25(id, instdir) && !bUpgradeNonMsi)
			    {
				sprintf(szBuf, "The Ingres instance '%s' was already installed by CA-Installer.", id);
				MyMessageBox(hInstall, szBuf);
				RegCloseKey(hSubKey);
				RegCloseKey(hRegKey);
				return -1;
			    }

			    RegCloseKey(hSubKey);
			    RegCloseKey(hRegKey);
			    return 0;
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hRegKey);
    }

    return 0;
}

/*
**  Name: RemoveOneDir
**
**  Description:
**	Remove a directory and all underlying files, but do NOT 
**	remove its subdirectories.
**
**  History:
**      06-aug-2003 (penga03)
**          Created.
*/
BOOL 
RemoveOneDir(char *DirName)
{
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    DWORD dwAttrib;
    char FileName[MAX_PATH], FullFileName[MAX_PATH];
    BOOL status=TRUE;

    dwAttrib = GetFileAttributes(DirName);

    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
    {
	sprintf(FileName, "%s\\*.*", DirName);
	hFind = FindFirstFile(FileName, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
	    do
	    {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		    continue;
		
		sprintf(FullFileName, "%s\\%s", DirName, FindFileData.cFileName);
		status = DeleteFile(FullFileName);
		if (!status)
		    break;

	    } while (FindNextFile(hFind, &FindFileData));
	    FindClose(hFind);
	}
	status = RemoveDirectory(DirName);
    }
    else
	status = DeleteFile(DirName);

    return status;
}

/*
**  Name: CheckUserRights
**
**  Description:
**	Verify if the user that currently logged on has following 
**	privileges and rights: 
**	1) Act as part of operating system
**	2) Log on as a service
**	3) Access this computer from the network
**
**  History:
**      21-aug-2003 (penga03)
**          Created.
**	13-Aug-2010 (drivi01)
**          NetApiBufferFree(pGroups) call is causing a
**          SEGV on x64 while running install as ingres.
**          Added a pointer to store pGroups before 
**          enumerating so that the original pGroups is
**          freed instead of some address at the end of the
**          enumeration.
*/
BOOL 
CheckUserRights(char *UserName)
{
    TCHAR AccountName[UNLEN + 1];
    WCHAR wcAccountName[UNLEN + 1]=L"";
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    char p[512], q[512];
    DWORD i,j, n, m;
    NTSTATUS nts;
    BOOL bTcb, bServiceLogon, bNetworkLogon;   
    WKSTA_USER_INFO_1 *pUserInfo;
    NET_API_STATUS nets;
    SID *UserSid, *GroupSid;
    char DomName[MAX_PATH];
    DWORD SidSize, DomSize;
    SID_NAME_USE SidUse;
    LSA_UNICODE_STRING *UserRights, *GroupRights;
    ULONG CountOfRights;
    LOCALGROUP_USERS_INFO_0 *pGroups;
    DWORD EntriesRead, TotalEntries;

    bTcb = bServiceLogon = bNetworkLogon = 0;
    EntriesRead = TotalEntries = 0;

    /* 
    ** Open the policy object on the target machine.
    */
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    nts = LsaOpenPolicy(NULL, &ObjectAttributes, GENERIC_EXECUTE, &PolicyHandle);
    if (nts) 
	return 0;

    /* 
    ** Get the name of the domain in which the user 
    ** is currently logged on. 
    */
    nets = NetWkstaUserGetInfo(NULL, 1, (LPBYTE *)&pUserInfo);
    if (nets == NERR_Success && pUserInfo)
    {
	n = wcstombs(p, (LPCWSTR)pUserInfo->wkui1_logon_domain, 512);
	p[n] = '\0';
	m = wcstombs(q, (LPCWSTR)pUserInfo->wkui1_username, 512);
	q[m] = '\0';
	wsprintf(AccountName, TEXT("%hS\\%hS"), p, q);
    }
    else
	wsprintf(AccountName, TEXT("%hS"), UserName);
    if (pUserInfo) NetApiBufferFree(pUserInfo);

    /* 
    ** Obtain the SID of the user, and enumerate his/her 
    ** privileges and account rights 
    */
    UserSid = (SID *) malloc(MAX_PATH);
    SidSize = DomSize = MAX_PATH;
    if (LookupAccountName(NULL, AccountName,UserSid, &SidSize, 
		DomName, &DomSize, &SidUse))
    {
	CountOfRights = 0;
	nts = LsaEnumerateAccountRights(PolicyHandle, UserSid, &UserRights, &CountOfRights);
	if (!nts)
	{
	    for (i = 0; i < CountOfRights; i++)
	    {
		n = wcstombs(p, UserRights[i].Buffer, 512);
		p[n] = '\0';
		if (!_stricmp(p, "SeTcbPrivilege")) bTcb = 1;
		if (!_stricmp(p, "SeServiceLogonRight")) bServiceLogon = 1;
		if (!_stricmp(p, "SeNetworkLogonRight")) bNetworkLogon = 1;
	    }
	}
	if (UserRights) LsaFreeMemory(UserRights);
    }
    if (UserSid) free(UserSid);

    if (bTcb && bServiceLogon && bNetworkLogon)
    {
	if (PolicyHandle) LsaClose(PolicyHandle);
	return 1;
    }

    /* 
    ** Retrive local groups to which the user belongs and 
    ** each group's privileges and account rights. 
    */
    wsprintfW(wcAccountName, L"%hS", AccountName);
    nets = NetUserGetLocalGroups(NULL, wcAccountName, 0, LG_INCLUDE_INDIRECT, 
		(LPBYTE *)&pGroups, -1, &EntriesRead, &TotalEntries);
    if (nets == NERR_Success)
    {
	BYTE *hdpGroups = pGroups;
	for (i = 0; i < EntriesRead; i++)
	{
	    n = wcstombs(p, pGroups->lgrui0_name, 512);
	    p[n] = '\0';

	    GroupSid = (SID *) malloc(MAX_PATH);
	    SidSize = DomSize = MAX_PATH;
	    if (LookupAccountName(NULL, p, GroupSid, &SidSize, 
			DomName, &DomSize, &SidUse))
	    {
		CountOfRights = 0;
		nts = LsaEnumerateAccountRights(PolicyHandle, GroupSid, 
			&GroupRights, &CountOfRights);
		if (!nts)
		{
		    for (j = 0; j < CountOfRights; j++)
		    {
			n = wcstombs(p, GroupRights[j].Buffer, 512);
			p[n] ='\0';
			if (!_stricmp(p, "SeTcbPrivilege")) bTcb = 1;
			if (!_stricmp(p, "SeServiceLogonRight")) bServiceLogon = 1;
			if (!_stricmp(p, "SeNetworkLogonRight")) bNetworkLogon = 1;
		    }
		}
		if (GroupRights) LsaFreeMemory(GroupRights);
	    }
	    if(GroupSid) free(GroupSid);

	    pGroups++;
	} /* end of for loop */
	pGroups = hdpGroups;
	}
    if (pGroups) NetApiBufferFree(pGroups);

    /* Close the policy handle. */
    if (PolicyHandle) LsaClose(PolicyHandle);

    if (bTcb && bServiceLogon && bNetworkLogon)
	return 1;
    else 
	return 0;
}

/*
**  Name: DelTok
**
**  Description:
**	Remove all occurrences of specified token from a string.
**
**  History:
**	15-Oct-2003 (penga03)
**	    Created.
*/
BOOL 
DelTok(char *string, const char *token)
{
    char tmp[512], *tok, *toks[10];
    int i, j, bFound=0;
	
    if (!string || *string==0) 
	return(bFound);
	
    strcpy(tmp, string);
    tok=strtok(tmp, ", ");
    i=0;
    while(tok)
    {
	if (!_stricmp(tok, token)) 
	    bFound=1;

	toks[i]=tok;
	tok=strtok(NULL, ", ");
	i++;
    }
    if (!bFound) 
	return(bFound);

    *string=0;
    for(j=0; j<i; j++)
    {
	if (_stricmp(toks[j], token))
	{
	    strcat(string, toks[j]);
	    strcat(string, ",");
	}
    }
    if (string[strlen(string)-1]==',')
	string[strlen(string)-1]='\0';

    return (bFound);
}

/*
**  Name: CheckVer25
**
**  Description:
**	Find out if the Ingres instance to be installed has already 
**	been installed by CA-Installer.
**
**  History:
**	24-Nov-2003 (penga03)
**	    Created.
*/
BOOL 
CheckVer25(char *id, char *ii_system)
{
    char szTemp1[MAX_PATH], szTemp2[MAX_PATH];
    char RegKey[255];
    HKEY hKey;
    char szComponentGUID[39], szComponentREG[33];
    BOOL bFlag1, bFlag2;

    bFlag1=bFlag2=0;
    sprintf(szTemp1, "%s\\ingres\\files\\config.dat", ii_system);
    sprintf(szTemp2, "%s\\CAREGLOG.LOG", ii_system);
    if (GetFileAttributes(szTemp1)!=-1 && GetFileAttributes(szTemp2)!=-1)
	bFlag1=1;

    if (!_stricmp(id, "II"))
	strcpy(szComponentGUID, "{844FEE64-249D-11D5-BDF5-00B0D0AD4485}");
    else
    {
	int idx;
	
	idx = (toupper(id[0]) - 'A') * 26 + toupper(id[1]) - 'A';
	if (idx <= 0) 
	    idx = (toupper(id[0]) - 'A') * 26 + toupper(id[1]) - '0';

	sprintf(szComponentGUID, "{844FEE64-249D-11D5-%04X-%012X}", idx, idx*idx);
    }
	
    GUID_TO_REG(szComponentGUID, szComponentREG);
	
    sprintf(RegKey, 
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Components\\%s", 
		szComponentREG);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hKey))
	bFlag2=1;

    sprintf(RegKey, 
		"SOFTWARE\\Microsoft\\Windows\\Currentversion\\Installer\\Components\\%s", 
		szComponentREG);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hKey))
	bFlag2=1;
    
    return (bFlag1 && bFlag2);
}

/*
**  Name: CheckDBL
**
**  Description:
**	Find out if the Ingres instance with the specified installation 
**	identifier is double byte.
**
**  History:
**	24-Nov-2003 (penga03)
**	    Created.
*/
BOOL 
CheckDBL(char *id, char *ii_system)
{
    char szFileName[MAX_PATH];
    char szBuf[MAX_PATH];
    FILE *fptr;
    BOOL bDbl=0;

    sprintf(szFileName, "%s\\ingres\\version.rel", ii_system);
    if ((fptr=fopen(szFileName, "r")) != NULL)
    {
	while (fgets(szBuf, sizeof(szBuf), fptr) != NULL)
	{
	    _strlwr(szBuf);
	    if (strstr(szBuf, "dbl")) {bDbl=1; fclose(fptr); break;}
	}
	fclose(fptr);
    }

    return bDbl;
}

BOOL 
CheckServiceExists(LPSTR lpszServiceName)
{
	SC_HANDLE hSCManager, hCService;
	BOOL rc;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if(!hSCManager)	
		return FALSE;

	hCService = OpenService(hSCManager, lpszServiceName, GENERIC_READ);
	if(hCService)
	{
		rc = TRUE;
		CloseServiceHandle(hCService);
	}
	else
		rc = FALSE;

	CloseServiceHandle(hSCManager);

	return rc;
}


/*
**  Name: IsServiceStartupTypeAuto
**
**  Description:
**	Find out if Ingres Service is setup to start automatically
**      or manually.  This function is only called on upgrade and
**  	only to figure out if we need to start service at the end
**	of installation.
**
**  History:
**	16-Nov-2006 (drivi01)
**	    Created.
*/
BOOL
IsServiceStartupTypeAuto(LPSTR lpszServiceName)
{
	SC_HANDLE hSCManager, hCService;
	BOOL rc;
	QUERY_SERVICE_CONFIG sp;
	DWORD	dwSize;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if(!hSCManager)	
		return FALSE;

	hCService = OpenService(hSCManager, lpszServiceName, GENERIC_READ);
	if(hCService)
	{
		if (!QueryServiceConfig(hSCManager, &sp, sizeof(sp),  &dwSize))
		{
			if (sp.dwStartType==SERVICE_AUTO_START)
			     rc = TRUE;
			else
			     rc = FALSE;
		}
		else
			rc = FALSE;
		CloseServiceHandle(hCService);
	}
	else
		rc = FALSE;

	CloseServiceHandle(hSCManager);

	return rc;
}

BOOL 
GetDriveLetterFromResource(HRESOURCE hResource, LPSTR driveLetter)
{
	LPVOID lpValueList;
	DWORD dwResult;
	DWORD dwBytesReturned;
	DWORD dwPosition;
    DWORD dwOutBufferSize;
	CLUSPROP_BUFFER_HELPER cbh;

	lpValueList = NULL;
	dwResult = 0;
	dwBytesReturned = 0;
	dwPosition = 0;
    dwOutBufferSize = 512;

	lpValueList = LocalAlloc(LPTR, dwOutBufferSize);
	if(!lpValueList)
		return FALSE;
	
	dwResult = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO, NULL, 0, lpValueList, dwOutBufferSize, &dwBytesReturned);
	if(dwResult == ERROR_MORE_DATA)
	{
		LocalFree(lpValueList);
		dwOutBufferSize = dwBytesReturned;
		lpValueList = LocalAlloc(LPTR, dwOutBufferSize);
		if(!lpValueList)
			return FALSE;
	
		dwResult = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO, NULL, 0, lpValueList, dwOutBufferSize, &dwBytesReturned);
	}
	
	if(dwResult != ERROR_SUCCESS)
		return FALSE;
	
	cbh.pb = (PBYTE)lpValueList;
	while(1)
	{
		if(cbh.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
		{
			if(wcslen(cbh.pPartitionInfoValue->szDeviceName) > 0)
			{
				memset(driveLetter, '\0', sizeof(driveLetter));
				wcstombs(driveLetter, cbh.pPartitionInfoValue->szDeviceName, sizeof(cbh.pPartitionInfoValue->szDeviceName));
			}
		}
		else if(cbh.pSyntax->dw == CLUSPROP_SYNTAX_ENDMARK)
			break;
	
		dwPosition = dwPosition + sizeof(CLUSPROP_VALUE) + ALIGN_CLUSPROP(cbh.pValue->cbLength);

		if((dwPosition + sizeof(DWORD)) > dwBytesReturned)
			break;
	
		cbh.pb = cbh.pb + sizeof(CLUSPROP_VALUE) + ALIGN_CLUSPROP(cbh.pValue->cbLength);
	}

	if(lpValueList)
		LocalFree(lpValueList);
	
	return TRUE;
}

unsigned int 
CheckSharedDiskInSameRG(LPSTR *sharedDiskList, INT sharedDiskCount)
{
	HCLUSTER hCluster;
	HCLUSENUM hClusEnum;
	HGROUP hGroup;
	HGROUPENUM hGroupEnum;
	HRESOURCE hResource;
	CHAR lpszDiskName[SIZE];
	WCHAR lpwszGroupName[SIZE];
	WCHAR lpwszResourceType[SIZE];
	WCHAR lpwszResourceName[SIZE];
	WCHAR lpwszSDgroupName[SIZE];
	DWORD dwIndex;
	DWORD dwValueListSize;
	DWORD dwGroupType;
	DWORD dwGroupSize;
	DWORD dwResourceType;
	DWORD dwResourceSize;
	BOOL bFoundDisk;
	unsigned int iRet;
	int resourceIndex;
	int SDindex;
	int sharedDiskMatch;

	hCluster = NULL;
	hClusEnum = NULL;
	hGroup = NULL;
	hGroupEnum = NULL;
	hResource = NULL;
	dwIndex = 0;
	dwValueListSize = 0;
	dwGroupType = SIZE;
	dwGroupSize = SIZE;
	dwResourceType = SIZE;
	dwResourceSize = SIZE;
	bFoundDisk = FALSE;
	iRet = HA_FAILURE;
	sharedDiskMatch = 0;

	hCluster = OpenCluster(NULL);
	if(!hCluster)
		return HA_CLUSTER_NOT_RUNNING;
	
	hClusEnum = ClusterOpenEnum(hCluster, CLUSTER_ENUM_GROUP);
	if(!hClusEnum)
	{
		CloseCluster(hCluster);
		return HA_FAILURE;
	}
	
	memset(lpwszSDgroupName, '\0', sizeof(lpwszSDgroupName));

	while(ERROR_SUCCESS == ClusterEnum(hClusEnum, dwIndex, &dwGroupType, lpwszGroupName, &dwGroupSize))
	{
		hGroup = OpenClusterGroup(hCluster, lpwszGroupName);
		if(!hGroup)
		{
			CloseCluster(hCluster);
			return HA_FAILURE;
		}

		hGroupEnum = ClusterGroupOpenEnum(hGroup, CLUSTER_GROUP_ENUM_CONTAINS);
		if(!hGroupEnum)
		{
			CloseClusterGroup(hGroup);
			CloseCluster(hCluster);
			return HA_FAILURE;
		}
				
		for(resourceIndex=0; ; resourceIndex++)
		{
			bFoundDisk = FALSE;

			if(ClusterGroupEnum(hGroupEnum, resourceIndex, &dwResourceType, lpwszResourceName, &dwResourceSize) == ERROR_NO_MORE_ITEMS)
				break;
				
			hResource = OpenClusterResource(hCluster, lpwszResourceName);
			if(!hResource)
			{
				CloseClusterGroup(hGroup);
				CloseCluster(hCluster);
				return HA_FAILURE;
			}
			
			memset(lpwszResourceType, '\0', sizeof(lpwszResourceType));
			memset(lpszDiskName, '\0', sizeof(lpszDiskName));

			if(ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, NULL, 0, lpwszResourceType, SIZE, NULL) == ERROR_SUCCESS)
			{
				if(wcscmp(lpwszResourceType, PHYSICALDISK) == 0)
				{
					if(GetDriveLetterFromResource(hResource, lpszDiskName))
					{
						for(SDindex = 0; SDindex < sharedDiskCount; SDindex++)
						{
							if(strnicmp(lpszDiskName, sharedDiskList[SDindex], 2) == 0)
							{
								bFoundDisk = TRUE;
								break;
							}
						}
					}
				}
			}

			if(bFoundDisk)
			{
				if(wcslen(lpwszSDgroupName) == 0)
				{
					wcscpy(lpwszSDgroupName, lpwszGroupName);
					iRet = HA_SUCCESS;
				}
				else if (wcscmp(lpwszSDgroupName, lpwszGroupName) == 0)
				{
					iRet = HA_SUCCESS;
				}
				else
				{
					iRet = HA_FAILURE;
					break;
				}
				sharedDiskMatch++;
				bFoundDisk = FALSE;
			}
			CloseClusterResource(hResource);
			dwResourceSize = SIZE;
		}
		CloseClusterGroup(hGroup);

		dwIndex++;
		dwGroupSize = SIZE;
	}

	if(sharedDiskMatch != sharedDiskCount)
	iRet = HA_FAILURE;

	ClusterGroupCloseEnum(hGroupEnum);
	ClusterCloseEnum(hClusEnum);
	CloseCluster(hCluster);

	return iRet;
}

/*
**  Name: AddUserRights
**
**  Description:
**	Added to the user that currently logged on following 
**	privileges and rights: 
**	1) Act as part of operating system
**	2) Log on as a service
**	3) Access this computer from the network
**
**  History:
**      21-march-2005 (penga03)
**          Created.
*/
void 
InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String)
{
    USHORT StringLength;
	
    if (String == NULL)
    {
	LsaString->Buffer = NULL;
	LsaString->Length = 0;
	LsaString->MaximumLength = 0;
	return;
    }
    // Get the length of the string without the null terminator.
    StringLength = wcslen(String);
    // Store the string.
    LsaString->Buffer = String;
    LsaString->Length = StringLength * sizeof(WCHAR);
    LsaString->MaximumLength= (StringLength+1) * sizeof(WCHAR);
}

BOOL 
AddUserRights(char *UserName)
{
    TCHAR AccountName[UNLEN + 1];
    WCHAR wcAccountName[UNLEN + 1]=L"";
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    char p[512], q[512];
    DWORD n, m;
    NTSTATUS nts;
    WKSTA_USER_INFO_1 *pUserInfo;
    NET_API_STATUS nets;
    SID *UserSid;
    char DomName[MAX_PATH];
    DWORD SidSize, DomSize;
    SID_NAME_USE SidUse;
    LSA_UNICODE_STRING lucPrivilege;

    /* 
    ** Open the policy object on the target machine.
    */
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    nts = LsaOpenPolicy(NULL, &ObjectAttributes, 
	      POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &PolicyHandle);
    if (nts) 
	return 0;

    /* 
    ** Get the name of the domain in which the user 
    ** is currently logged on. 
    */
    nets = NetWkstaUserGetInfo(NULL, 1, (LPBYTE *)&pUserInfo);
    if (nets == NERR_Success && pUserInfo)
    {
	n = wcstombs(p, (LPCWSTR)pUserInfo->wkui1_logon_domain, 512);
	p[n] = '\0';
	m = wcstombs(q, (LPCWSTR)pUserInfo->wkui1_username, 512);
	q[m] = '\0';
	wsprintf(AccountName, TEXT("%hS\\%hS"), p, q);
    }
    else
	wsprintf(AccountName, TEXT("%hS"), UserName);
    if (pUserInfo) NetApiBufferFree(pUserInfo);

    /* 
    ** Obtain the SID of the user, and add privileges and account rights 
    */
    UserSid = (SID *) malloc(MAX_PATH);
    SidSize = DomSize = MAX_PATH;
    if (LookupAccountName(NULL, AccountName,UserSid, &SidSize, 
		DomName, &DomSize, &SidUse))
    {
	InitLsaString(&lucPrivilege, L"SeTcbPrivilege");
	nts = LsaAddAccountRights(PolicyHandle, UserSid, &lucPrivilege, 1);

	InitLsaString(&lucPrivilege, L"SeServiceLogonRight");
	nts = LsaAddAccountRights(PolicyHandle, UserSid, &lucPrivilege, 1);

	InitLsaString(&lucPrivilege, L"SeNetworkLogonRight");
	nts = LsaAddAccountRights(PolicyHandle, UserSid, &lucPrivilege, 1);
    }
    if (UserSid) free(UserSid);
    if (!nts)
	return TRUE;
    else 
	return FALSE;
}
/*
**  Name: IsDotNETFXInstalled
**
**  Description:
**	This function checks if .Net Framework 1.1 is installed.
**
**  History:
**      10-July-2007 (drivi01)
**          Add routines to check for existance of .NET Framework 1.1
**	    on 64-bit version of Windows.
*/
int 
IsDotNETFXInstalled()
{
    char SubKey[MAX_PATH], SubKey64[MAX_PATH];
    HKEY hkSubKey;

    sprintf(SubKey, "SOFTWARE\\Microsoft\\.NETFramework\\policy\\v1.1");
    sprintf(SubKey64, "SOFTWARE\\Wow6432Node\\Microsoft\\.NETFramework\\policy\\v1.1");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_QUERY_VALUE, &hkSubKey))
	return 1;
    else if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey64, 0, KEY_QUERY_VALUE, &hkSubKey))
	return 1;
    else return 0;
}

/*
**  Name: IsDotNETFX20Installed
**
**  Description:
**	This function checks if .Net Framework 2.0 is installed.
**
**  History:
**      16-Nov-2006 (drivi01)
**          Created.
**      10-July-2007 (drivi01)
**          Add routines to check for existance of .NET Framework 2.0
**	    on 64-bit version of Windows.
*/
int 
IsDotNETFX20Installed()
{
    char SubKey[MAX_PATH], SubKey64[MAX_PATH];
    HKEY hkSubKey;

    sprintf(SubKey, "SOFTWARE\\Microsoft\\.NETFramework\\policy\\v2.0");
    sprintf(SubKey64, "SOFTWARE\\Wow6432Node\\Microsoft\\.NETFramework\\policy\\v2.0");
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_QUERY_VALUE, &hkSubKey))
	return 1;
    else if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey64, 0, KEY_QUERY_VALUE, &hkSubKey))
	return 1;
    else return 0;
}

BOOL 
VerifyIngTimezone(char *IngTimezone)
{
    int i;

    if (!IngTimezone[0])
	return FALSE;

    i=0;
    while(IngTimezones[i])
    {
	if (!_stricmp(IngTimezone, IngTimezones[i]))
	    return TRUE;
	i++;
    }
    return FALSE;
}

BOOL 
VerifyIngCharset(char *IngCharset)
{
    int i;

    if (!IngCharset[0])
	return FALSE;

    i=0;
    while(IngCharsets[i])
    {
	if (!_stricmp(IngCharset, IngCharsets[i]))
	    return TRUE;
	i++;
    }
    return FALSE;
}

BOOL 
VerifyIngTerminal(char *IngTerminal)
{
    int i;

    if (!IngTerminal[0])
	return FALSE;

    i=0;
    while(IngTerminals[i])
    {
	if (!_stricmp(IngTerminal, IngTerminals[i]))
	    return TRUE;
	i++;
    }
    return FALSE;
}

BOOL 
VerifyIngDateFormat(char *IngDateFormat)
{
    int i;

    if (!IngDateFormat[0])
	return FALSE;

    i=0;
    while(IngDateFormats[i])
    {
	if (!_stricmp(IngDateFormat, IngDateFormats[i]))
	    return TRUE;
	i++;
    }
    return FALSE;
}

BOOL 
VerifyIngMoneyFormat(char *IngMoneyFormat)
{
    if (!IngMoneyFormat[0])
	return FALSE;

    if (IngMoneyFormat[0] != 'L' && IngMoneyFormat[0] != 'l' 
        && IngMoneyFormat[0] != 'T' && IngMoneyFormat[0] != 't')
	return FALSE;

    return TRUE;
}

/*
**  Name: GetProductCode
**  Description: Retrieves product code guid from msi
**
**  Parameters:
**			path - Full path to the msi 
**			guid - Pointer to the buffer where product code guid will be stored
**			size - Size of the buffer to hold product code guid
**
**	Returns:
**			0 - Success
**			1 or above - System Error
**
**	History:
**	18-Nov-2008 (drivi01)
**	    Created.
** 
*/
DWORD GetProductCode(char *path, char *guid, DWORD size)
{
	DWORD dwError=0;
	MSIHANDLE msih=0, hView=0, hRec=0;
	char cmd[MAX_PATH];
	
	if (!((dwError=MsiOpenDatabase(path, MSIDBOPEN_READONLY, &msih))==ERROR_SUCCESS))
	{
		goto CLEANUP;
	}

	sprintf(cmd, "SELECT * FROM Property WHERE Property = 'ProductCode'");
	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
	{
		dwError=GetLastError();
		goto CLEANUP;
	}

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
	{
		dwError=GetLastError();
		goto CLEANUP;
	}

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
	{
		dwError=GetLastError();
		goto CLEANUP;
	}

	if (!(MsiRecordGetString(hRec, 2, (LPSTR)guid, &size) == ERROR_SUCCESS))
	{
		dwError=GetLastError();
		goto CLEANUP;
	}


CLEANUP:

	if (hRec)
		MsiCloseHandle(hRec);

	if (hView)
		if (MsiViewClose( hView )==ERROR_SUCCESS)
			MsiCloseHandle(hView);
	
	if (msih)
		MsiCloseHandle(msih);

	return dwError;
}
