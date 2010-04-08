/*
**  Copyright (c) 2001-2009 Ingres Corporation.
*/

/*
**
**  Name: commonmm.h
**
**  Description:
**	This header file contains common defines for the Merge Module
**	DLLs.
**
**  History:
**	27-apr-2001 (somsa01)
**	    Created.
**	09-may-2001 (somsa01)
**	    Added Execute() and Local_PMhost().
**	14-may-2001 (penga03)
**	    Added VerifyPath(), and GetRegistryEntry().
**	03-Aug-2001 (penga03)
**	    Added SetRegistryEntry(), StringReverse(), and StringReplace().
**	30-oct-2001 (penga03)
**	    Added function OtherInstallationsExist.
**	10-dec-2001 (penga03)
**	    Added function GetInstallPath.
**	12-feb-2002 (penga03)
**	    Took the function IsWindows95() from setupmm.c.
**	06-mar-2002 (penga03)
**	    Took the function ExecuteEx() from dbmsnetmm.c.
**	14-may-2002 (penga03)
**	    Added CreateOneShortcut() and CreateDirectoryRecursively().
**	04-jun-2002 (penga03)
**	    Added GetRefCount().
**	01-aug-2002 (penga03)
**	    Added GetLinkInfo().
**	02-oct-2002 (penga03)
**	    Modified VerifyPath(). 
**	08-oct-2002 (penga03)
**	     Modified the return type of GetLinkInfo() from HRESULT to 
**	     BOOL.
**	26-jun-2003 (penga03)
**	    Added MyMessageBox() and VerifyInstance().
**	06-aug-2003 (penga03)
**	    Add RemoveOneDir().
**	21-aug-2003 (penga03)
**	    Add CheckUserRights().
**	20-oct-2003 (penga03)
**	    Modified VerifyPath(). Added DelTok().
**	10-dec-2003 (penga03)
**	    Also added CheckVer25() and CheckDBL().
**	22-apr-2004 (penga03)
**	    Added AskUserYN().
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	21-march-2005 (penga03)
**	    Added AddUserRights().
**	14-june-2005 (penga03)
**	    Added IsDotNETFXInstalled().
**	31-aug-2005 (penga03)
**	    Added VerifyIngTimezone(), VerifyIngCharset(), VerifyIngTerminal(),
**	    VerifyIngDateFormat(), VerifyIngMoneyFormat().
**	14-Nov-2008 (drivi01)
**	    Define Ingres Version here so that it could be pulled to
**	    other custom actions.
**	16-Jan-2009 (whiro01) SD Issue 132899
**	    Added seven prototypes here to quiet compiler warnings: 
**	    IsServiceStartupTypeAuto, IsVista, IsDotNETFX20Installed,
**	    AskUserOkCancel, CreateOneAdminShortcut, ExecuteShellEx
**	    and GetProductCode.
**	10-Jun-2009 (drivi01) SIR 122142
**	    Update the version number to 9.4 (ING_VER).
**	    Updated the product code for IngresII_Doc.ism
**	    from {B9C4293D-F959-4C40-805C-E3DC0A79CA0E}
**	    to   {12D64643-5305-430F-B579-23E029F80C9A}.
**	    Making a note of the product code in this file
**	    as it wouldn't be a visible change in a binary file
**	    IngresII_Doc.ism.
**	10-Aug-2009 (drivi01)
**	    Update the version number to 10.0 (ING_VER).
**	    Updated the product code for IngresII_Doc.ism
**	    from {12D64643-5305-430F-B579-23E029F80C9A}
**	    to   {07B11A75-FCF3-4963-8651-1619E958DED4}.
**	    Making a note of the product code in this file
**	    as it wouldn't be a visible change in a binary file
**	    IngresII_Doc.ism.
*/
extern int	Execute(char *lpCmdLine, BOOL bwait);
extern int	Local_NMgtIngAt(char *strKey, char *strRetValue);
extern int	Local_PMget(char *strKey, char *strRetValue);
extern void	Local_PMhost(char *hostname);
extern int	RemoveDir(char *DirName, BOOL DeleteDir);
extern BOOL	VerifyPath(char *szPath);
extern int	GetRegistryEntry(HKEY hRootKey, char *subKey, char *name, char *value);
extern BOOL	SetRegistryEntry(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueName, DWORD dwType, void *lpData);
extern void	StringReverse(char *string);
extern void	StringReplace(char *string);
extern BOOL	OtherInstallationsExist(char *cur_code, char *code);
extern BOOL	GetInstallPath(char *strInstallCode, char *strInstallPath);
extern BOOL	IsWindows95();
extern BOOL	ExecuteEx(char *lpCmdLine);
extern int	CreateOneShortcut(char *szFolder, char *szDisplayName, char *szTarget, char *szArguments, char *szIconFile, char *szWorkDir, char *szDescription, BOOL Startup, BOOL AllUsers);
extern int	CreateDirectoryRecursively(char *szDir);
extern int	GetRefCount(char *szComponentGUID, char *szProductGUID);
extern BOOL	GetLinkInfo(LPCTSTR lpszLinkName, LPSTR lpszPath);
extern int	MyMessageBox(MSIHANDLE hInstall, char *lpText);
extern int	VerifyInstance(MSIHANDLE hInstall, char *id, char *instdir);
extern BOOL	RemoveOneDir(char *DirName);
extern BOOL	CheckUserRights(char *UserName);
extern BOOL	DelTok(char *string, const char *token);
extern BOOL	CheckVer25(char *id, char *ii_system);
extern BOOL	CheckDBL(char *id, char *ii_system);
extern BOOL	AskUserYN(MSIHANDLE hInstall, char *lpText);
extern BOOL	AskUserOkCancel(MSIHANDLE hInstall, char *lpText);
extern unsigned int CheckSharedDiskInSameRG(LPSTR *sharedDiskList, INT sharedDiskCount);
extern BOOL	CheckServiceExists(LPSTR lpszServiceName);
extern BOOL	AddUserRights(char *UserName);
extern BOOL	IsServiceStartupTypeAuto(LPSTR lpszServiceName);
extern int	IsDotNETFXInstalled();
extern int	IsDotNETFX20Installed();
extern BOOL	IsVista();
extern BOOL	VerifyIngTimezone(char *IngTimezone);
extern BOOL	VerifyIngCharset(char *IngCharset);
extern BOOL	VerifyIngTerminal(char *IngTerminal);
extern BOOL	VerifyIngDateFormat(char *IngDateFormat);
extern BOOL	VerifyIngMoneyFormat(char *IngMoneyFormat);
extern int	CreateOneAdminShortcut(char *szFolder, char *szDisplayName, char *szTarget, 
		  char *szArguments, char *szIconFile, char *szWorkDir, 
		  char *szDescription, BOOL Startup, BOOL AllUsers);
extern DWORD	GetProductCode(char *path, char *guid, DWORD size);
extern BOOL	ExecuteShellEx(char *lpCmdLine);


#define ING_VERS	"10.0"
