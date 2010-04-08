# include <windows.h>
# include <string.h>
# include <stdlib.h>
# include <stdio.h>
# include <conio.h>
# include <direct.h>
# include <sys\types.h>
# include <sys\stat.h>
# include <process.h>
# include <time.h>
# include <commctrl.h>
# include <imagehlp.h>
# include "resource.h"

/*
**  Name : installer.c -  Front end to CA-Installer for OpenIngres patches.
**
**
**  History:
**      16-may-96 (somsa01)
**          Created from installer.c for CA-OpenIngres.
**	19-jun-96 (somsa01)
**		Changed NUM_PRODUCTS to 1. Also added exit(0) if the compiler
**		version is MSVC 2.2, if the install button is clicked.
**	14-Jan-97 (somsa01)
**	    Add patch information in the registry.
**	22-jan-97 (somsa01)
**	    Anyone can add a patch (removed "must be ingres" and "must be an
**	    Administrator" restrictions).
**	11-mar-97 (somsa01)
**	    Removed setting of PATCHNO; this is set when space.exe is run, in
**	    ingresnt.mas . Also modified setting of IngVersion.
**	17-mar-97 (somsa01)
**	    Changed around registry patch info such that ALL patch numbers
**	    installed, rather than the last one installed, are tracked.
**	29-jul-97 (mcgem01)
**	    Updated for OpenIngres 2.0
**	28-feb-1998 (somsa01)
**	    The release of OI 2.0/9712 (int.wnt/00) improperly set the
**	    "IngVersion" keyword in the registry to the old release. Therefore,
**	    until the next release of OI 2.0/9712, added the re-setting of
**	    this registry entry.
**	07-may-1998 (somsa01)
**	    For Windows 95, do not reset the IngVersion in the registry.
**	04-jan-1999 (somsa01)
**	    Differentiate between an Ingres II patch and an OpenIngres 2.0
**	    patch.
**	20-jan-1999 (somsa01)
**	    Made changes consistent with new patching procedures, as laid
**	    out by the patch group. Revamped patch installer.
**	03-feb-1999 (somsa01)
**	    Corrected typo in CheckCdimage().
**	04-feb-1999 (somsa01)
**	    Added wait dialog while we find the installed files. Also, use
**	    GetModuleFileName() to retrieve the cdimage location.
**	05-feb-1999 (somsa01)
**	    Check the version of Ingres before installation (only if the
**	    version.rel exists).
**	09-feb-1999 (somsa01)
**	    For the system DLLs (ctl3d32.dll and msvcrt.dll), just make sure
**	    that they exist. If they don't, copy over the ones from the
**	    cdimage.
**	22-feb-1999 (somsa01)
**	    Run InitCommonControls() to set up what's necessary to use the
**	    common controls.
**	09-Mar-1999 (andbr08)
**	    Disabled the checksum for windows 95 as the function
**	    MapFileAndChecksumA() does not work correctly under windows 95
**	    on all files.
**	12-Mar-1999 (andbr08)
**	    Added functionality to allow redirection of input (from the 
**	    instaux.bat file) to work in windows 95.
**	24-Mar-1999 (andbr08)
**	    Added checksum used in UNIX, defined in ip_file_info_comp()
**	    in front!st!install ipfile.sc.  Added GetCurrentFileChecksum().
**	26-Mar-1999 (andbr08)
**	    Changed MultipleBackupDlgProc to use the correct sequence number
**	    and initialized item.iItem = 0 as is needed
**	26-May-1999 (andbr08)
**	    Added functionality to check which version of gca32_1.dll to
**	    install based on the OS, 95 or NT.
**	27-May-1999 (andbr08)
**	    Changed incorrect formatting of install log where '<' was used
**	    twice.
**	18-June-1999 (andbr08)
**	    Get rid of II_INSTAUX_EXCODE environment variable when done
**	    with it.
**	07-October-1999 (andbr08)
**	    Get rid of version.rel check because of inherent problems when
**	    doing an upgrade, version.rel is not updated.  Also, Black Box
**	    ingres is release 9905 which also causes problems.
**	15-Mar-2000 (fanch01)
**	    Cleanup and correction of path lengths due to issue 8650454.
**	    Removed global char workbuf[256], had private declaration of
**	    same name in all functions, incorrectly memset w/count==1024
**	    according to global size.
**	    Removed II_COMPUTERNAME - unused.
**	    Changed szFile, szFileTitle, II_VOLUME, II_SYSTEM, BACKUP_LOC,
**	    PatchBackupLoc, CDIMAGE_LOC to size _MAX_PATH.
**	    5-10 bugs cleared up in the process.
**	09-jun-2000 (kitch01)
**	    Add functionallity to check for existing utility DLLs and remove
**	    them if asked to. These DLLs must be removed as they are no
**	    longer supplied and also 9808 and 0001 will not even install
**	    them. Also added back in the version check. We now check a copy
**	    of config.dat and will allow the user to continue to install the
**	    patch if they want to after warning them and stressing the need
**	    for backup. Fix for crash if createprocess fails.
**	01-Sep-2000 (hanal04) Bug 102490
**	    Implemented new rules for version checking:
**	    - Version match then go ahead. (Hack bad version strings if
**	      0001 patch over 0001 base release.
**	    - Not a recognised 2.0 maintenance level, disallow install.
**	    - If 2.0 release but version do not match then;
**	    - Disallow 0001 patch over 9712 install
**	    - Allow 0001 patch over 9808 install (updating config.dat)
**	    - Allow all other combination, suggest backup.
**	06-Oct-2000 (hanal04) Bug 102490
**	    Add 9905 Embedded release as a valid version string.
**	    Add checks for a TNG embedded install. If embed ensure
**	    embed_install and embed_user are in the config.dat
**	28-Feb-2001 (zhahu02)
**	    Updated for doing 25 NT patch.
**	22-May-2001 (lunbr01)  Bug 105129
**	    Fixed access violation when no gca32_1.dll.
**	15-May-2002 (inifa01)
**	    Modified to include 2.6 version string and to accomodate longer
**	    2.6 path lengths.
**	31-May-2002 (somsa01)
**	    Changed "Ingres II" references to "Ingres". Updated copyright
**	    notice. Added 2.6/0201 (int.w32/05) as a valid version.
**	    Changed registry location to match 2.6.
**	25-jun-2002 (somsa01)
**	    Added 2.6/0207 (int.w32/00) as a valid version.
**	25-jun-2002 (somsa01)
**	    In CheckIngVersion(), corrected setting of current version
**	    printout.
**	12-jul-2002 (somsa01)
**	    If IVM is running, shut it down.
**	15-jul-2002 (somsa01)
**	    Don't check if IVM is running; just shut it down.
**	24-jul-2002 (somsa01)
**	    Corrected some RunDialog() statements with WaitDlgProc so that
**	    we pass the proper parent window handle.
**	10-dec-2002 (penga03)
**	    Took away the "your" from the message "Please contact your CA 
**	    Technical Support". Added a "to" to the message "The patch 
**	    installer was unable determine your current Ingres version."
**	23-Aug-2002 (inifa01)
**	    Changed reference to patch.txt to patch.html
**	23-Aug-2002 (inifa01)
**	    Changed 2.6/0207 (int.w32/00) version to 2.6/0207 (int.w32/02)
**	    since that is build version.
**	10-dec-2002 (penga03)
**	    Took away the "your" from the message "Please contact your CA 
**	    Technical Support". Added a "to" to the message "The patch 
**	    installer was unable determine your current Ingres version."
**	6-Jan-2003 (inifa01)
**	    Added 2.6/0207 (int.w32/00) version to installer to prevent patch
**	    install failure over SP1 due to Mismatch b/w actual SP1 version
**	    [2.6/0207 (int.w32/02)] and version entered by SP1 during upgrade
**	    [2.6/0207 (int.w32/00)] in config.dat
**      19-feb-2003 (rigka01) bug 111272
**          Notepad should not be used to display the patch.html file 
**	    during patch installation
**	17-apr-2003 (penga03)
**	    Made changes in CheckIngVersion() so that the installer will not 
**	    pop up the warning message if only the build number of the Ingres 
**	    version changed.
**	28-jul-2003 (penga03)
**	    Added 2.6/0305 (int.w32/00) as a valid version.
**	04-dec-2003 (rigka01) bug 111400 
**	     Update config.dat COMPLETE messages for 2.6/0305 upgrade 
*/


# define MAXSTRING 1024

/*
** defines for GetPlatform
*/
# define II_WIN32S	1
# define II_WINNT	2
# define II_WIN95	6

/*
** Structure for holding sizes of files
*/
struct FILEINF
{
    CHAR	dircode[2];
    CHAR	*filename;
    CHAR	*cdimage_name;
    CHAR	*filepath;
    LONG	filesize;
    INT		newfile;
    INT		found;
};

/*
** fsizes.roc is generated by running "space"
*/
# include "fsizes.roc"


/*
** forward declarations
*/
LONG WINAPI	WndProc			(HWND,UINT,UINT,LONG);
BOOL WINAPI	InstallDlgProc		(HWND,UINT,UINT,LONG);
BOOL WINAPI	MultipleBackupDlgProc	(HWND,UINT,UINT,LONG);
BOOL WINAPI	WaitDlgProc		(HWND,UINT,UINT,LONG);
BOOL WINAPI	Wait2DlgProc		(HWND,UINT,UINT,LONG);

INT			AskBox(CHAR *, CHAR *, BOOL);
VOID 		BoxErr1(CHAR *, CHAR *);
VOID 		BoxErr2(CHAR *, LONG);
VOID		BoxErr3(CHAR *, DWORD);
VOID		BoxErr4(CHAR *, CHAR *);
VOID		BoxErr5(CHAR *, CHAR *);
FLOAT		CalcTotals(VOID);
VOID		CenterBox(HWND);
INT		CheckBackup(INT);
INT		CheckCdimage(VOID);
INT		CheckIngVersion(VOID);
VOID		HackIngVersion(int, int);
INT		CheckInstall(INT);
BOOL		CheckMakeDir(CHAR *);
BOOL		CheckMakeDirRec(CHAR *, BOOL);
BOOL		CheckSystemDlls(VOID);
BOOL		CheckUtilityDllsPresent(VOID);
BOOL		CheckVersion(VOID);
INT		CompareVersion(char *, char *);
DWORD 		DoInstall(VOID);
BOOL		FileExists(CHAR *);
VOID		FindInstalledFiles(VOID);
INT		FormatInstallLog(VOID);
VOID		GenerateISC(VOID);
LONG		GetCurrentFileSize(CHAR *);
DWORD		GetPlatform(VOID);
VOID		GetTimeOfDay(BOOL, CHAR *);
INT		InstallerSetupThread(VOID);
BOOL		IsUserIngres(VOID);
BOOL WINAPI	LocSelDlgProc(HWND);
BOOL CALLBACK	LocSelHookProc(HWND, UINT, WPARAM, LPARAM);
VOID 		MakeResponse(VOID);
INT		PostInstallThread(VOID);
VOID		ProcessCDError(DWORD, HWND);
DWORD		RunDialog(FARPROC, CHAR *, HWND);
BOOL		RunningAsAdministrator(VOID);
BOOL		SupLongFname(CHAR *);

static CHAR	szAppName[27];
HANDLE		hModule	    = 0;
FARPROC		Mod_Address = NULL;
HINSTANCE	ghInst;		/* Handle to instance */
HWND		ghWndMain;	/* Handle to main window */
HWND		hDlgGlobal;	/* Handle to Main Dialog */
BOOL		Backup;		/* Are we to backup installation files? */
BOOL		ForceInstall;	/* Force install even if checksums for */
				/* cdimage fail. */
INT		PatchSeq = 1;	/* Number of times patch was attempted. */
INT		Upgradeto0001 = 0;
INT		Upgradeto26NEW = 0;
OPENFILENAME	OpenFileName;
CHAR		szFile[_MAX_PATH] = "\0";
CHAR		szFileTitle[_MAX_PATH];

CHAR	II_VOLUME	[_MAX_PATH];	/* disk volume, derived from BACKUP_LOC */
CHAR	II_SYSTEM	[_MAX_PATH];	/* installation location */
CHAR	BACKUP_LOC	[_MAX_PATH];	/* backup location */
CHAR	PatchBackupLoc	[_MAX_PATH];	/* patch backup location, versioned */
CHAR	CDIMAGE_LOC	[_MAX_PATH];	/* cd image location */
CHAR	PATCHNOTES_CMD	[_MAX_PATH * 2];	/* Command to view patch notes */
FILE	*PatchInstallLog=NULL;	/* The Patch Installer log file */


INT 	DebugMode=0;		/* for debugging purposes */

FLOAT	Bytes;		/* needed Kbytes for a backup */
CHAR	BackupKB[256];	/* string for holding the ascii rep. of bytes needed */
FLOAT	AvailBytes;     /* available bytes */
CHAR	AvailKB[256];   /* string for holding the ascii rep. of free space */

DWORD GetCurrentFileChecksum(CHAR*);

struct WeekDay
{
    CHAR	Day[4];
};

struct WeekDay DayOfWeek[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

struct MonthNum
{
    CHAR	Month[4];
};

struct MonthNum Month[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

struct IngRel
{
	CHAR Vers[22];
};

struct IngRel IngVers[] = {
	"209712intwnt00",
	"209808intwnt00",
	"209905intwnt00",
	"200001intw3200",
	"250011intw3200",
	"260201intw3200",
	"260201intw3205",
	"260207intw3200",
	"260207intw3202",
	"260207intw3204",
	"260207intw3205",
	"260207intw3206",
	"260207intw3207",
	"260305intw3200",
	""
};

INT     ep0001 = 3;	/* Element pointer for 0001 */
INT     ep26NEW = 13;	/* Element pointer for 260305 */
struct IngRel Ing0001hack[] = {
	"dbms.ii209808intw3200",
	"net.ii209909intw3200",
	"star.ii209808intw3200",
	""
};

# define II0001_BASE_TO_PATCH	0x0001   /* Check for bad 2.0/0001 strings	*/
# define IIOLD_TO_II0001		0x0002   /* Adds 2.0/0001 strings			*/
# define II26OLD_TO_II26NEW		0x0010   /* Adds 2.6/0305 strings			*/
# define BACKUP_CONFIG			0x0004   /* Backup the config.dat			*/

/*
**  Name: WinMain()
**
**  Description:
**	Entry point of the Application.
*/
INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPSTR lpszCmdLine, INT nCmdShow)
{
    MSG			lpMsg;
    WNDCLASS		wndclass;
    HWND		hWnd;
    CHAR		TimeStamp[25], cmdline[16];
    DWORD		wait_status;
    STARTUPINFO		siStInfo;
    PROCESS_INFORMATION	piProcInfo;
    DWORD		ThreadID;
    CHAR		file2check[MAX_PATH+1];

    /*
    ** Get the cd image location.
    */
    GetModuleFileName(NULL, CDIMAGE_LOC, sizeof(CDIMAGE_LOC));
    *(strrchr(CDIMAGE_LOC, '\\')) = '\0';

    if (!CheckSystemDlls())
	return (1);

    InitCommonControls();

    /*
    ** set defaults
    */
    if (getenv("II_SYSTEM")==NULL)
    {
#ifdef IngresII
      BoxErr4("The II_SYSTEM environment variable is not set.\n", "Check to make sure that you have installed Ingres successfully.\n");
#else
      BoxErr4("The II_SYSTEM environment variable is not set.\n", "Check to make sure that you have installed OpenIngres successfully.\n");
#endif
      exit(1);
    }

    strcpy(II_SYSTEM, getenv("II_SYSTEM") );

    /*
    ** Check to see if the installation version matches what the patch is
    ** for.
    */
    if (!CheckIngVersion())
	exit(1);

    /*
    ** See if the installation is up by checking for the Name Server.
    */
    memset (&siStInfo, 0, sizeof (siStInfo)) ;

    siStInfo.cb	= sizeof (siStInfo);
    siStInfo.lpReserved	= NULL;
    siStInfo.lpReserved2 = NULL;
    siStInfo.cbReserved2 = 0;
    siStInfo.lpDesktop = NULL;
    siStInfo.dwFlags = STARTF_USESHOWWINDOW;
    siStInfo.wShowWindow = SW_HIDE;
    sprintf(cmdline, "pinggcn.exe");

    CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL,
		  &siStInfo, &piProcInfo);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    GetExitCodeProcess(piProcInfo.hProcess, &wait_status);

    if (!wait_status)
    {
#ifdef IngresII
      BoxErr4("Your Ingres installation is running.\n", "Please shut it down before running the Ingres Patch Installer.\n");
#else
      BoxErr4("Your OpenIngres installation is running.\n", "Please shut it down before running the OpenIngres Patch Installer.\n");
#endif
      exit(1);
    }

    /*
    ** See if IVM is up; shut it down if it is.
    */
    sprintf(file2check, "%s\\ingres\\vdba\\ivm.exe", II_SYSTEM);
    if (GetFileAttributes(file2check) != -1)
    {
	memset (&siStInfo, 0, sizeof (siStInfo)) ;

	siStInfo.cb = sizeof (siStInfo);
	siStInfo.lpReserved = NULL;
	siStInfo.lpReserved2 = NULL;
	siStInfo.cbReserved2 = 0;
	siStInfo.lpDesktop = NULL;
	siStInfo.dwFlags = STARTF_USESHOWWINDOW;
	siStInfo.wShowWindow = SW_HIDE;
	sprintf(cmdline, "ivm.exe /stop");

	CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL,
		&siStInfo, &piProcInfo);
	WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    }

    /* If this is not 9712 then check for utility DLLs */
    if (strstr(PatchVersion,"9712") == NULL)
    {
	/* Check for dlls present in utility dir */
	if (CheckUtilityDllsPresent())
	    exit(1);
    }

    strcpy(BACKUP_LOC, II_SYSTEM);
    strcat(BACKUP_LOC, "\\ingres\\install\\backup");
    ghInst = hInstance;

    /*
    ** 3D stuff
    */
    hModule = LoadLibrary("CTL3D32");
    if (hModule)
    { 
      Mod_Address = GetProcAddress (hModule,"Ctl3dRegister");
      if (Mod_Address)
       (* Mod_Address)(NULL);    /* Register the 3D effect if available */
      Mod_Address = GetProcAddress (hModule, "Ctl3dAutoSubclass");
      if (Mod_Address)
       (* Mod_Address)(NULL);    /* AutoSubclass the 3D effect if available */
      Mod_Address = GetProcAddress (hModule,"Ctl3dColorChange");
      if (Mod_Address)
       (* Mod_Address)(NULL);    /* Register the 3D effect if available */
    }
    else 
      BoxErr3("Could not load CTL3D32.DLL ...\n", GetLastError());

#ifdef IngresII
    strcpy(szAppName, "Ingres Patch Installer");
#else
    strcpy(szAppName, "OpenIngres Patch Installer");
#endif

    /*
    ** Create window class (currently the main window is never seen!)
    */
#ifdef IngresII
    if (!FindWindow(szAppName, "Ingres Patch Installer"))
#else
    if (!FindWindow(szAppName, "OpenIngres Patch Installer"))
#endif
    {
	wndclass.lpfnWndProc = (WNDPROC)WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance, "WINCAP");
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszClassName = "PATCH INSTALL";
	wndclass.lpszMenuName  = szAppName;
	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.hbrBackground = GetStockObject(GRAY_BRUSH);
	wndclass.lpszClassName = (LPSTR)szAppName;

	if (!RegisterClass(&wndclass))
	{
	    BoxErr5("RegisterClass failed...","\n");
            return FALSE;
	}

	ghInst = hInstance;  /* Set Global variable */

	/*
        ** Create a main window for this application instance.
	*/
#ifdef IngresII
        hWnd = CreateWindow(szAppName, "Ingres Patch Installer",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
#else
        hWnd = CreateWindow(szAppName, "OpenIngres Patch Installer",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
#endif

	ghWndMain = hWnd;  /* Set global variable */


	/*
	** make sure we are running 3.51 or better on NT
	*/
	if (!CheckVersion()) return (1);

	/*
	** Find the files that the client has.
	*/
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FindInstalledFiles,
		     NULL, 0, &ThreadID);
#ifdef IngresII
	RunDialog(WaitDlgProc, "IDD_WAIT3II_DIALOG", ghWndMain);
#else
	RunDialog(WaitDlgProc, "IDD_WAIT3_DIALOG", ghWndMain);
#endif

	/*
	** Bring up main dialog box
	*/
#ifdef IngresII
	RunDialog(InstallDlgProc, "InstallIIDlgBox", hWnd);
#else
	RunDialog(InstallDlgProc, "InstallDlgBox", hWnd);
#endif

	if (PatchInstallLog)
	{
	    GetTimeOfDay(FALSE, TimeStamp);
	    fprintf(PatchInstallLog, "Ending at:  %s\n", TimeStamp);
	    fclose(PatchInstallLog);
	}
    }

    while (GetMessage(&lpMsg, NULL, 0, 0))
    {
	TranslateMessage(&lpMsg);
	DispatchMessage(&lpMsg);
    }
    return lpMsg.wParam;
}

/*
**  Name: WndProc()
**
**  Description:
**	Main window procedure.
*/
LONG WINAPI
WndProc(HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam)
{
    switch (wMessage)
    {
	case WM_CREATE:
	    ghWndMain=hWnd;
	    break;

	case WM_CLOSE:
	    return DefWindowProc(hWnd, wMessage, wParam, lParam);
	    break;

	case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

	default:
	    return DefWindowProc(hWnd, wMessage, wParam, lParam);
    }
    return 0L;
}

/*
**  Name: WaitDlgProc
**
**  Description:
**	Dialog procedure for wait.
*/
BOOL WINAPI
WaitDlgProc(HWND hDlg, UINT messg, UINT wParam, LONG lParam)
{
    switch (messg)
    {
	case WM_INITDIALOG:
	    EnableWindow(hDlgGlobal, FALSE);
	    hDlgGlobal = hDlg;
	    CenterBox(hDlg);
	    SendMessage(GetDlgItem(hDlg, IDC_ANIMATE1), ACM_OPEN, 0, IDR_AVI1);
	    SendMessage(GetDlgItem(hDlg, IDC_ANIMATE1), ACM_PLAY, (UINT) -1,
			MAKELONG(0, -1));
	    return TRUE;

	case WM_ENDSESSION:
	    SendMessage(GetDlgItem(hDlg, IDC_ANIMATE1), ACM_STOP, 0, 0L);
	    return TRUE;

	case WM_CLOSE:
	    EndDialog(hDlg, TRUE);
	    break;

	default:
	    return FALSE;
    } /* end of the mesg switch */
    return TRUE;
}

/*
**  Name: InstallDlgProc
**
**  Description:
**	Dialog procedure for main install dialog.
**	15-Mar-2000 (fanch01)
**		FileLoc size changed to _MAX_PATH.
*/
BOOL WINAPI
InstallDlgProc(HWND hDlg, UINT messg, UINT wParam, LONG lParam)
{
    CHAR	FileLoc[_MAX_PATH];
    DWORD	SectPerClus, BytesPerSect, NumFreeClus, TotClus, status;

    switch (messg)
    {
	case WM_INITDIALOG:
	    hDlgGlobal = hDlg;
	    CenterBox(hDlg);

	    SetDlgItemText(hDlg, EB_LOCATION, (LPTSTR)&II_SYSTEM);
	    SetDlgItemText(hDlg, ID_PATCH, (LPTSTR)PATCHNO);
	    SetDlgItemText(hDlg, ID_BACKUPLOC, (LPTSTR)&BACKUP_LOC);
	    /*EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SETDIR), FALSE);*/
	    /*EnableWindow(GetDlgItem(hDlg, IDC_STATIC_BACKUP), FALSE);*/
	    /*EnableWindow(GetDlgItem(hDlg, ID_BACKUPLOC), FALSE);*/
		SendMessage(GetDlgItem(hDlg,IDC_CHECK_BACKUP), BM_SETCHECK, 1, 0);
	    Backup = TRUE;

	    strcpy (PATCHNOTES_CMD, "cmd.exe /c start ");
	    strcat (PATCHNOTES_CMD, CDIMAGE_LOC);
	    strcat (PATCHNOTES_CMD, "\\ingres\\patch.html");

	    /*
	    ** Find out how much space a backup will need...
	    */
	    Bytes = CalcTotals() / 1024;
	    if (Bytes > 1000)
	    {
		Bytes = CalcTotals() / (1024*1000);
		sprintf(BackupKB, "%.2f", Bytes);
		strcat(BackupKB, " MB");
	    }
	    else
	    {
		sprintf(BackupKB, "%.2f", Bytes);
		strcat(BackupKB, " KB");
	    }
	    SetDlgItemText(hDlg, IDC_BACKUP_SIZE, (LPTSTR)&BackupKB);
	    /*EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SPACE), FALSE);*/
	    /*EnableWindow(GetDlgItem(hDlg, IDC_BACKUP_SIZE), FALSE);*/

	    ForceInstall = FALSE;

	    return TRUE;

	case WM_COMMAND:
	    switch (LOWORD(wParam))
	    {
		case ID_INSTALL:
		    if (Backup)
		    {
			/*
			** If multiple backups exist for this patch, let the
			** user know and find out if we still need to back up
			** of not.
			*/
			sprintf(FileLoc,
				"%s\\ingres\\install\\control\\pre-p%s_1.dat",
				II_SYSTEM, PATCHNO);
			if (FileExists(FileLoc))
			{
#ifdef IngresII
			    status = RunDialog( MultipleBackupDlgProc,
						"MULTIPLEIIBACKUP", hDlg);
#else
			    status = RunDialog( MultipleBackupDlgProc,
						"MULTIPLEBACKUP", hDlg);
#endif
			    if (!status)
				break;
			}
		    }

		    /*
		    ** If we are backing up, make sure we have enough
		    ** space in BACKUP_LOC.
		    */
		    if (Backup)
		    {
			strcpy(II_VOLUME, BACKUP_LOC);
			II_VOLUME[3] = '\0';
			GetDiskFreeSpace((LPTSTR)&II_VOLUME, &SectPerClus,
					 &BytesPerSect, &NumFreeClus, &TotClus);
			AvailBytes=((FLOAT)((LONGLONG)SectPerClus*
				    (LONGLONG)BytesPerSect*
				    (LONGLONG)NumFreeClus));
			Bytes = CalcTotals();
			if (AvailBytes < Bytes)
			{
			    BoxErr4("The amount of space needed for backup does not exist in the location selected.\n",
				    "Please select an alternate backup location.");
			    break;
			}

			/*
			** Now, as a sanity check, make sure we have
			** permission to create the directory (tree).
			*/
			if (!CheckMakeDirRec(BACKUP_LOC, FALSE))
			{
			    BoxErr4("The backup directory was not able to be made.\n",
				    "Please check and correct directory permissions, or choose another location.");
			    break;
			}
			
		    }

		    status = DoInstall();
		    SetForegroundWindow(hDlg);
		    if (!status)
		    {
			fprintf(PatchInstallLog, "\nThe patch install completed successfully.\n\n");
			BoxErr1("The patch was installed successfully!\n","");
		    }
		    else
		    {
			fprintf(PatchInstallLog, "\nThe patch was NOT successfully installed.\n\n");
			BoxErr5("The patch has NOT been installed successfully!\n", "");
		    }

		    EndDialog(hDlg,FALSE);
		    SendMessage(ghWndMain,WM_CLOSE,0,0L);
		    break;

		case IDC_CHECK_BACKUP:
		    if (SendMessage(GetDlgItem(hDlg,IDC_CHECK_BACKUP),
				    BM_GETCHECK, 3, 0))
		    {
			EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_SETDIR),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_STATIC_BACKUP),TRUE);
			EnableWindow(GetDlgItem(hDlg,ID_BACKUPLOC),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_STATIC_SPACE),TRUE);
			EnableWindow(GetDlgItem(hDlg,IDC_BACKUP_SIZE),TRUE);
			Backup = TRUE;
		    }
		    else
		    {
			EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_SETDIR),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_STATIC_BACKUP),FALSE);
			EnableWindow(GetDlgItem(hDlg,ID_BACKUPLOC),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_STATIC_SPACE),FALSE);
			EnableWindow(GetDlgItem(hDlg,IDC_BACKUP_SIZE),FALSE);
			Backup = FALSE;
		    }
		    break;

		case IDC_CHECK_FORCE:
		    if (SendMessage(GetDlgItem(hDlg,IDC_CHECK_FORCE),
				    BM_GETCHECK, 3, 0))
			ForceInstall = TRUE;
		    else
			ForceInstall = FALSE;

		    break;

		case IDC_BUTTON_SETDIR:
		    LocSelDlgProc(hDlg);
		    hDlgGlobal = hDlg;
		    SetDlgItemText(hDlg, ID_BACKUPLOC, (LPTSTR) &BACKUP_LOC);
		    break;

		case IDC_BUTTON_PATCHNOTES:
		    WinExec((LPCSTR)&PATCHNOTES_CMD, SW_NORMAL);
		    break;
         
		case IDC_BUTTON_EXIT:
#ifdef IngresII
		    BoxErr1("The Ingres Patch has not been installed!\n","");
#else
		    BoxErr1("The OpenIngres Patch has not been installed!\n",
			    "");
#endif
		    /* Close Main Window */
		    EndDialog(hDlg,FALSE);
		    SendMessage(ghWndMain,WM_CLOSE,0,0L);
		    break;
	
		default:
		    return FALSE;
	    }
	    break; /* this break is needed! */

	default:
	    return FALSE;
    } /* end of the mesg switch */
    return TRUE;
}

/*
**  Name: MultipleBackupDlgProc
**
**  Description:
**	Dialog procedure for showing the user that multiple backups exist,
**	and asking them if we want to still back up.
**
**	15-Mar-2000 (fanch01)
**		Change FileLoc, BackupLocation size to _MAX_PATH.
*/
BOOL WINAPI
MultipleBackupDlgProc(HWND hDlg, UINT messg, UINT wParam, LONG lParam)
{
    HWND	hListCtrl;
    INT		sequence, i, RowNum;
    CHAR	ColName[16], FileLoc[_MAX_PATH], mline[MAXSTRING], *CharPtr;
    CHAR	BackupDate[25], BackupStatus[8], BackupLocation[_MAX_PATH];
    LV_COLUMN	ColumnInfo;
    LV_ITEM	ItemInfo;
    FILE	*FilePtr;

    switch (messg)
    {
	case WM_INITDIALOG:
	    CenterBox(hDlg);
	    hListCtrl = GetDlgItem (hDlg, IDC_LIST_BACKUP);

	    /*
	    ** Set up the columns of the list box.
	    */
	    ColumnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	    ColumnInfo.fmt = LVCFMT_CENTER;
	    ColumnInfo.pszText = (CHAR *)&ColName;
	    ColumnInfo.cchTextMax = sizeof(ColName);
	    strcpy(ColName, "Backup Date");
	    ColumnInfo.cx = 162;
	    SendMessage(hListCtrl, LVM_INSERTCOLUMN, 1,
			(LPARAM)(const LV_COLUMN FAR *)&ColumnInfo);
	    strcpy(ColName, "Status");
	    ColumnInfo.cx = 49;
	    SendMessage(hListCtrl, LVM_INSERTCOLUMN, 2,
			(LPARAM)(const LV_COLUMN FAR *)&ColumnInfo);
	    strcpy(ColName, "Location");
	    ColumnInfo.cx = 200;
	    SendMessage(hListCtrl, LVM_INSERTCOLUMN, 3,
			(LPARAM)(const LV_COLUMN FAR *)&ColumnInfo);

	    /*
	    ** Find the last backup.
	    */
	    sequence = 0;
	    sprintf(FileLoc, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
		    II_SYSTEM, PATCHNO, sequence + 1);
	    while(FileExists(FileLoc))
	    {
		++sequence;
		sprintf(FileLoc, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
			II_SYSTEM, PATCHNO, sequence + 1);
	    }

	    /*
	    ** Now, place the appropriate info for each backup into the
	    ** list box.
	    */
	    ItemInfo.iItem = 0;
	    for (i=sequence; i > 0; i--)
	    {
		sprintf(FileLoc, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
			II_SYSTEM, PATCHNO, i);
		FilePtr = fopen(FileLoc, "r");
		while ( (fgets(mline, MAXSTRING - 1, FilePtr) != NULL) &&
			(!strstr(mline, "## Sequence:	")) )
		{
		    mline[strlen(mline)-1] = '\0';
		    CharPtr = mline;
		    while (*(CharPtr++) != '	');
		    if (strstr(mline, "## Performed:	"))
			strcpy(BackupDate, CharPtr);
		    else if (strstr(mline, "## Status:	"))
			strcpy(BackupStatus, CharPtr);
		    else if (strstr(mline, "## Target:	"))
			strcpy(BackupLocation, CharPtr);
		}
		fclose(FilePtr);

		ItemInfo.mask = LVIF_TEXT;
		ItemInfo.pszText = (CHAR *)&BackupDate;
		ItemInfo.iSubItem = 0;
		RowNum = SendMessage(hListCtrl, LVM_INSERTITEM, 0,
				     (LPARAM)(const LV_ITEM FAR *)&ItemInfo);
		ItemInfo.pszText = (CHAR *)&BackupStatus;
		ItemInfo.iSubItem = 1;
		SendMessage(hListCtrl, LVM_SETITEMTEXT, RowNum,
			    (LPARAM)(const LV_ITEM FAR *)&ItemInfo);
		ItemInfo.pszText = (CHAR *)&BackupLocation;
		ItemInfo.iSubItem = 2;
		SendMessage(hListCtrl, LVM_SETITEMTEXT, RowNum,
			    (LPARAM)(const LV_ITEM FAR *)&ItemInfo);
	    }

	    return TRUE;

	case WM_COMMAND:
	    switch (LOWORD(wParam))
	    {
		case IDOK:
		    EndDialog(hDlg,TRUE);
		    break;

		case IDNO:
		    Backup = FALSE;
		    EndDialog(hDlg,TRUE);
		    break;

		case IDCANCEL:
#ifdef IngresII
		    BoxErr1("The Ingres Patch has not been installed!\n","");
#else
		    BoxErr1("The OpenIngres Patch has not been installed!\n",
			    "");
#endif
		    /* Close Main Window */
		    EndDialog(hDlg, FALSE);
		    SendMessage(ghWndMain, WM_CLOSE, 0, 0L);
		    break;

		default:
		    return FALSE;
	    }
	    break; /* this break is needed! */

	default:
	    return FALSE;
    } /* end of the mesg switch */
    return TRUE;
}

/*
**  Name: CenterBox
**
**  Description:
**	Passed the handle to dialog, this procedure will center it onscreen.
*/
VOID
CenterBox(HWND hDlg)
{
    RECT	DialogRect;
    INT		xlen, ylen, ScreenX, ScreenY;

    GetWindowRect(hDlg, &DialogRect);	/* get dimensions */

    ScreenX=GetSystemMetrics(SM_CXSCREEN);	/* get screen size */
    ScreenY=GetSystemMetrics(SM_CYSCREEN);

    xlen=DialogRect.right-DialogRect.left;	/* calc height & width */
    ylen=DialogRect.bottom-DialogRect.top;

    SetWindowPos(hDlg, NULL, (ScreenX-xlen)/2, (ScreenY-ylen)/2,
		 xlen, ylen, SWP_SHOWWINDOW);
}

/*
**  Name: BoxErr1
**
**  Description:
**	Quick little function for printing messages to the screen.
**	Takes two strings as it's input.
*/
VOID
BoxErr1 (CHAR *MessageString, CHAR *MessageString2)
{
    CHAR	MessageOut[1024];

    sprintf(MessageOut,"%s%s",MessageString, MessageString2);
#ifdef IngresII
    MessageBox( hDlgGlobal, MessageOut, "Ingres Patch Installer",
#else
    MessageBox( hDlgGlobal, MessageOut, "OpenIngres Patch Installer",
#endif
		MB_APPLMODAL | MB_ICONINFORMATION | MB_OK);
}

/*
**  Name: BoxErr2
**
**  Description:
**	Same as BoxErr, but accepts a string and a LONG.
*/
VOID
BoxErr2 (CHAR *MessageString, LONG MessageNum)
{
    CHAR	MessageOut[1024];

    sprintf(MessageOut,"%s\n%d",MessageString, MessageNum);
#ifdef IngresII
    MessageBox( hDlgGlobal, MessageOut, "Ingres Patch Installer",
#else
    MessageBox( hDlgGlobal, MessageOut, "OpenIngres Patch Installer",
#endif
		MB_APPLMODAL | MB_ICONINFORMATION | MB_OK);
}

/*
**  Name: BoxErr3
**
**  Description:
**	Prints message plus english description of dwErrCode.
*/
VOID
BoxErr3(CHAR *MessageString, DWORD dwErrCode)
{
    LPVOID	lpMessageBuffer;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPTSTR) &lpMessageBuffer, 0, NULL);
	 
    strcat(MessageString, "\n");
    BoxErr1(MessageString, lpMessageBuffer);

    /* Free the buffer allocated by the system */
    LocalFree( lpMessageBuffer );
}

/*
**  Name: BoxErr4
**
**  Description:
**	Quick little function for printing messages to the screen, with
**	the stop sign. Takes two strings as it's input.
*/
VOID
BoxErr4 (CHAR *MessageString, CHAR *MessageString2)
{
    CHAR	MessageOut[1024];

    sprintf(MessageOut,"%s%s",MessageString, MessageString2);
#ifdef IngresII
    MessageBox( hDlgGlobal, MessageOut, "Ingres Patch Installer",
#else
    MessageBox( hDlgGlobal, MessageOut, "OpenIngres Patch Installer",
#endif
		MB_APPLMODAL | MB_ICONSTOP | MB_OK);
}

/*
**  Name: BoxErr5
**
**  Description:
**	Quick little function for printing messages to the screen, with
**	the exclamation sign. Takes two strings as it's input.
*/
VOID
BoxErr5 (CHAR *MessageString, CHAR *MessageString2)
{
    CHAR	MessageOut[1024];

    sprintf(MessageOut,"%s%s",MessageString, MessageString2);
#ifdef IngresII
    MessageBox( hDlgGlobal, MessageOut, "Ingres Patch Installer",
#else
    MessageBox( hDlgGlobal, MessageOut, "OpenIngres Patch Installer",
#endif
		MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
}


/*
**  Name: MakeResponse
**
**  Description:
**	The function creates a RESPONSE.RSP file for use with CA-Installer.
**
**	15-Mar-2000 (fanch01)
**		Change MessageOut size to _MAX_PATH.
*/
VOID
MakeResponse(VOID)
{
    CHAR	MessageOut[_MAX_PATH];
    HANDLE	hResponseFile;
    DWORD	BytesWritten;

    sprintf(MessageOut, "%s\\ingres\\temp\\PATCH.RSP", II_SYSTEM);
    hResponseFile = CreateFile(MessageOut, GENERIC_WRITE, 0,
				NULL, CREATE_ALWAYS, 0, NULL);

    /*
    ** Set products to install so that CA-Installer doesn't have to ask.
    */
    WriteFile(hResponseFile, "[product_list]\n", 15, &BytesWritten, NULL);
    WriteFile(hResponseFile, "1\n\n", 3, &BytesWritten, NULL);
    
    /*
    ** Set destination path so that CA-Installer doesn't ask for it again.
    */
    WriteFile(hResponseFile,"[destination_path]\n",19,&BytesWritten,NULL);
    sprintf(MessageOut, "1,\"%s\"\n\n", II_SYSTEM);
    WriteFile(hResponseFile, MessageOut, strlen(MessageOut), &BytesWritten,
		NULL);

    /*
    ** done with RESPONSE.RSP file
    */
    CloseHandle(hResponseFile);
    return;
}

/*
**  Name: DoInstall
**
**  Description:
**	Creates the process which is the CA-Installer!
**
**  History:
**	12-dec-95 (tutto01)
**	    Added creation of log areas.  Reports error if not possible.
**	10-jan-95 (tutto01)
**	    Do not create log areas if the DBMS has been previously installed.
**	14-Jan-97 (somsa01)
**	    Add patch information in the registry.
**	21-jan-1999 (somsa01)
**	    Updated to conform to new patch procedures.
**	15-Mar-2000 (fanch01)
**		Moved global Patches[] here as DoInstall is the only reference.
**		Change Patches size to MAX_PATH as this this max length for
**			registry key path.
**		Change cmdline (path w/2 path args) to _MAX_PATH * 3.
**		Change FileLoc size to _MAX_PATH.
**		Change wildly incorrect memset on workbuf[] to count sizeof() and
**			corrected '\n' (!) termination.
**		Moved global INSTALL_DEBUG[] here as DoInstall is the only reference.
*/
DWORD
DoInstall(VOID)          
{ 
    CHAR		INSTALL_DEBUG[MAXSTRING];	/* used to see
							** we're debugging
							** the install
							*/
    CHAR		Patches[MAX_PATH];
    STARTUPINFO		siStInfo;
    PROCESS_INFORMATION	piProcInfo;
    HKEY		hKey = HKEY_LOCAL_MACHINE, hkIngres = NULL;
    DWORD		status, dwDisposition, dwSize, dwType;
    CHAR		workbuf[_MAX_PATH], cmdline[_MAX_PATH * 3];
    CHAR		SysPath[2048];
    DWORD		wait_status;
    DWORD		ThreadID;
    HWND		hPreviousWnd;
    CHAR		TimeStamp[25], FileLoc[_MAX_PATH];
    HANDLE		hThread;
    FILE		*SymbolTblPtr;
    CHAR		SymbolTblFile[_MAX_PATH];
    CHAR		mline[MAXSTRING], RegKey[MAXSTRING], *cptr;

    /*
    ** As a sanity check, make sure that the temp directory is there.
    */
    sprintf(cmdline, "%s\\ingres\\temp", II_SYSTEM);
    CheckMakeDir(cmdline);

    sprintf(cmdline, "%s\\ingres\\files\\install_p%s.log", II_SYSTEM, PATCHNO);
    PatchInstallLog = fopen(cmdline, "a");
    fprintf(PatchInstallLog, "\n------------------------------------------------------------\n\n");
    GetTimeOfDay(FALSE, TimeStamp);
    fprintf(PatchInstallLog, "Starting the install program at:  %s\n\n",
	    TimeStamp);

    hThread = CreateThread( NULL, 0,
			    (LPTHREAD_START_ROUTINE)InstallerSetupThread,
			    NULL, 0, &ThreadID);
    hPreviousWnd = hDlgGlobal;
#ifdef IngresII
    RunDialog(WaitDlgProc, "IDD_WAITII_DIALOG", hPreviousWnd);
#else
    RunDialog(WaitDlgProc, "IDD_WAIT_DIALOG", hPreviousWnd);
#endif
    hDlgGlobal = hPreviousWnd;
    EnableWindow(hDlgGlobal, TRUE);

    while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
	UpdateWindow(hDlgGlobal);

    GetExitCodeThread(hThread, &wait_status);
    if (wait_status)
	return(wait_status);

    /*
    ** Create bogus env. variable to provide "slack" in environment.  This is
    ** done because on Windows 95, when the CA-Installer kicks off our post
    ** setup program, there is no room to expand the environment.
    */
    memset(workbuf, 'x', sizeof (workbuf));
    workbuf[sizeof(workbuf)-1] = '\0';
    SetEnvironmentVariable("II_TEMP", workbuf);

    /*
    ** Add the patch information into the registry.
    */
    sprintf (SymbolTblFile, "%s\\ingres\\files\\symbol.tbl", II_SYSTEM);
    SymbolTblPtr = fopen(SymbolTblFile, "r");
    while ( (fgets(mline, MAXSTRING - 1, SymbolTblPtr) != NULL) &&
        (!strstr(mline, "II_INSTALLATION")) );
    fclose(SymbolTblPtr);
    cptr = strstr(mline, "	");
    cptr++;
    *(cptr+2) = '\0';
    sprintf(RegKey, "SOFTWARE\\ComputerAssociates\\IngresII\\%s_Installation",
	    cptr);

    status = RegCreateKeyEx(hKey,
#ifdef IngresII
		RegKey,
#else
		"SOFTWARE\\Computer Associates International Inc.\\OpenIngres",
#endif
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
		&hkIngres, &dwDisposition);
    if (status != ERROR_SUCCESS) BoxErr5("RegCreateKeyEx failed...", "");
    dwSize = sizeof(Patches);
    status = RegQueryValueEx(hkIngres, "Patches", NULL, &dwType, Patches,
			     &dwSize);
    if (status != ERROR_SUCCESS)
	strcpy(Patches, PATCHNO);
    else
    {
	if (strstr(Patches, PATCHNO)==NULL)
	{
	    strcat(Patches, ", ");
	    strcat(Patches, PATCHNO);
	}
    }
    status = RegSetValueEx(hkIngres, "Patches", 0, REG_SZ, Patches,
			   strlen(Patches)+1);
    if (status != ERROR_SUCCESS) BoxErr5("RegSetValueEx failed...","");
    if (GetPlatform() != II_WIN95)
    {
	status = RegSetValueEx(hkIngres, "IngVersion", 0, REG_SZ,
		    (const unsigned char *)&IngVersion, strlen(IngVersion)+1);
	if (status != ERROR_SUCCESS) BoxErr5("RegSetValueEx failed...","");
    }
    RegFlushKey(hkIngres);
    RegCloseKey(hkIngres);

    /*
    **  Overkill, needed for some reason.
    */
    GetEnvironmentVariable("PATH", SysPath, sizeof(SysPath));
    SetEnvironmentVariable("PATH", SysPath);

    /*
    ** Execute the CA-Installer to copy the files.
    */
    memset (&siStInfo, 0, sizeof (siStInfo)) ;

    siStInfo.cb		= sizeof (siStInfo);
    siStInfo.lpReserved	= NULL;
    siStInfo.lpReserved2	= NULL;
    siStInfo.cbReserved2	= 0;
    siStInfo.lpDesktop	= NULL;
    siStInfo.dwFlags	= 0;

    GetEnvironmentVariable("INSTALL_DEBUG", INSTALL_DEBUG,
			   sizeof(INSTALL_DEBUG));
    if (!strcmp(INSTALL_DEBUG, "DEBUG"))
    {
	strcpy(workbuf, "cmd.exe");
	if (GetPlatform() == II_WIN95) strcpy(workbuf, "command.com");
        CreateProcess (NULL, workbuf, NULL, NULL, TRUE, 0,
                		      NULL, NULL,&siStInfo, &piProcInfo);
    }

    sprintf(cmdline, "CAINW32.EXE -A -M -F -I\"%s\\ingres\\temp\\PATCH.ISC\" -R\"%s\\ingres\\temp\\PATCH.RSP\"",
	    II_SYSTEM, II_SYSTEM);
    fprintf(PatchInstallLog,
	    "Copying files from cdimage to installation ...\n\n");
    if (!CreateProcess (NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL,
			&siStInfo, &piProcInfo))
    {
	/*
	** CreateProcess failed!
	*/
	wait_status = GetLastError();
	sprintf(workbuf,"(%d)",wait_status);
	fprintf(PatchInstallLog, "Unable to copy files, error = %s\n\n",
		workbuf);
	BoxErr5("CreateProcess error... ", workbuf);
	BoxErr5("Unable to execute installer process!", "");

	if (!strcmp(INSTALL_DEBUG, "DEBUG"))
	     CreateProcess(NULL, "cmd.exe", NULL, NULL, TRUE, 0, NULL, NULL,
			   &siStInfo, &piProcInfo);
	else
		return wait_status;
    }

    EnableWindow(hDlgGlobal, FALSE);
	
    while (WaitForSingleObject(piProcInfo.hProcess, 0) != WAIT_OBJECT_0)
	UpdateWindow(hDlgGlobal);

    EnableWindow(hDlgGlobal, TRUE);
    GetExitCodeProcess(piProcInfo.hProcess, &wait_status);
    if (wait_status)
    {
	fprintf(PatchInstallLog, "Unable to copy files from cdimage to installation, CA-Install error %d.\n\n",
		wait_status);
    }
    else
    {
	hThread = CreateThread( NULL, 0,
				(LPTHREAD_START_ROUTINE)PostInstallThread,
				NULL, 0, &ThreadID);
	hPreviousWnd = hDlgGlobal;
#ifdef IngresII
	RunDialog(WaitDlgProc, "IDD_WAIT2II_DIALOG", hPreviousWnd);
#else
	RunDialog(WaitDlgProc, "IDD_WAIT2_DIALOG", hPreviousWnd);
#endif
	hDlgGlobal = hPreviousWnd;
	EnableWindow(hDlgGlobal, TRUE);

	while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
	    UpdateWindow(hDlgGlobal);

	GetExitCodeThread(hThread, &wait_status);
    }

    sprintf(FileLoc, "%s\\ingres\\temp\\PATCH.ISC", II_SYSTEM);
    DeleteFile(FileLoc);
    sprintf(FileLoc, "%s\\ingres\\temp\\PATCH.RSP", II_SYSTEM);
    DeleteFile(FileLoc);

    return(wait_status);
}

/*
**  Name: RunDialog
**
**  Description:
**	Quick little routine for running a dialog, or reporting problem
**	if failure to run.
*/
DWORD
RunDialog( FARPROC DlgProc, CHAR *DialogTitle, HWND hWnd)
{
    FARPROC	fpDlgProc;
    INT		status;

    /*
    ** bring up main dialog box
    */
    fpDlgProc = MakeProcInstance((FARPROC)DlgProc, ghInst);
    if (!fpDlgProc) BoxErr5("MakeProcInstance failed...", "");

    status = DialogBox(ghInst, DialogTitle, hWnd, fpDlgProc);
    if (status == -1)
    {
	status = GetLastError();
	BoxErr2("DialogBox failed...", status);
    }

    return (status);
}

/*
**  Name: FileExists
**
**  Description:
**	Determines whether the given file/path exists... well, sort of.
*/
BOOL
FileExists(CHAR *filename)
{
    if (GetFileAttributes(filename) != -1)
	return TRUE;

    return FALSE;
}

/*
** Function: CompareVersion
**
** Return Codes:
**   0 = Files are the same version
**   1 = File1 is a newer version than file2
**   2 = File2 is a newer version than file1
**  -1 = Error getting information on file1
**  -2 = Error getting information on file2
**  -3 = Memory allocation error
*/
int
CompareVersion( char *file1, char *file2 )
{
    DWORD  dwHandle = 0L;	/* Ignored in call to GetFileVersionInfo */
    DWORD  cbBuf1   = 0L, cbBuf2   = 0L;
    LPVOID lpvData1 = NULL, lpvData2 = NULL;
    LPVOID lpValue1 = NULL, lpValue2 = NULL;
    UINT   wBytes1 = 0L, wBytes2 = 0L;
    WORD   wlang1 = 0, wcset1 = 0, wlang2 = 0, wcset2 = 0;
    char   SubBlk1[ 81 ], SubBlk2[ 81 ];
    int    rcComp = 0;

    /* Retrieve Size of Version Resource */
    if( ( cbBuf1 = GetFileVersionInfoSize( file1, &dwHandle ) ) == 0 )
    {
	rcComp = -1;
	goto QuickExit;
    }
	
    if( ( cbBuf2 = GetFileVersionInfoSize( file2, &dwHandle ) ) == 0 )
    {
	rcComp = -2;
 	goto QuickExit;
    }

    lpvData1 = ( LPVOID ) malloc( cbBuf1 );
    lpvData2 = ( LPVOID ) malloc( cbBuf2 );

    if( !lpvData1 && lpvData2 )
    {
	rcComp = -3;
	goto QuickExit;
    }

    /* Retrieve Version Resource */
    if( GetFileVersionInfo( file1, dwHandle, cbBuf1, lpvData1 ) == FALSE )
    {
	rcComp = -1;
 	goto QuickExit;
    }
	
    if( GetFileVersionInfo( file2, dwHandle, cbBuf2, lpvData2 ) == FALSE )
    {
	rcComp = -2 ;
 	goto QuickExit;
    }

    /* Retrieve the Language and Character Set Codes */
    VerQueryValue( lpvData1,
                   TEXT( "\\VarFileInfo\\Translation" ),
		   &lpValue1,
		   &wBytes1 );
    wlang1 = *( ( WORD * ) lpValue1 );
    wcset1 = *( ( ( WORD * ) lpValue1 ) + 1 );
	                   
    VerQueryValue( lpvData2,
                   TEXT( "\\VarFileInfo\\Translation" ),
		   &lpValue2,
		   &wBytes2 );
    wlang2 = *( ( WORD * ) lpValue2 );
    wcset2 = *( ( ( WORD * ) lpValue2 ) + 1 );

    /* Retrieve FileVersion Information */
    sprintf(SubBlk1, "\\StringFileInfo\\%.4x%.4x\\FileVersion", wlang1, wcset1);
    VerQueryValue( lpvData1, TEXT( SubBlk1 ), &lpValue1, &wBytes1 );
    sprintf(SubBlk2, "\\StringFileInfo\\%.4x%.4x\\FileVersion", wlang2, wcset2);
    VerQueryValue( lpvData2, TEXT( SubBlk2 ), &lpValue2, &wBytes2 );

    {
	int majver1 = 0, minver1 = 0, relno1 = 0, majver2 = 0, minver2 = 0;
	int relno2 = 0;
	sscanf( ( char * )lpValue1, "%d.%d.%d", &majver1, &minver1, &relno1 );
	sscanf( ( char * )lpValue2, "%d.%d.%d", &majver2, &minver2, &relno2 );

	/* Check Major Version Number */
	if( majver1 == majver2 )
	{
	    /* Check Minor Version Number */
	    if( minver1 == minver2 )
	    {
		/* Check Release Number */
		if( relno1 == relno2 )
		    rcComp = 0;
		else
		{
		    if( relno1 > relno2 )
			rcComp = 1;
		    else
			rcComp = 2;
		}
	    }
	    else
	    {
		if( minver1 > minver2 )
		    rcComp = 1;
		else
		    rcComp = 2;
	    }
	}
	else
	{
	    if( majver1 > majver2 )
		rcComp = 1;
	    else
		rcComp = 2;
	}
    }

QuickExit:
    if( lpvData1 )
	free( lpvData1 );
    if( lpvData2 )
	free( lpvData2 );
	
    return ( rcComp );
}

/*
**  Name: CheckSystemDlls
**
**  Description:
**	Checks copies of required system DLLs (ctl3d32.dll and msvcrt.dll)
**	and places them on the machine in the proper area if they do not
**	exist.
**
**	15-Mar-2000 (fanch01)
**		Change SysPath, workbuf, DllSrc size to _MAX_PATH.
*/
BOOL
CheckSystemDlls(VOID)
{
    CHAR	SysPath[_MAX_PATH];
    CHAR	DllSrc[_MAX_PATH];
    CHAR	workbuf[_MAX_PATH];
    DWORD	status;

    if (GetPlatform() == II_WINNT)
    	sprintf(DllSrc, "%s\\CTL3DNT.DLL", CDIMAGE_LOC);
    else
    	sprintf(DllSrc, "%s\\CTL3D95.DLL", CDIMAGE_LOC);

    /*
    ** Check to see if there is a copy already of the ctl3d32.dll .
    */
    GetSystemDirectory(SysPath, sizeof(SysPath));
    wsprintf(workbuf, "%s\\CTL3D32.DLL", SysPath);

    if(!FileExists(workbuf))
    {
	status = CopyFile(DllSrc, workbuf, TRUE);
	if (!status) 
	{
	    BoxErr3("Could not copy the file CTL3D32.DLL ...\n",
		    GetLastError());
	    return FALSE;
	}
    }

    /*
    ** Check to see if there is a copy already of the msvcrt.dll .
    */
    sprintf(DllSrc, "%s\\MSVCRT.DLL", CDIMAGE_LOC);
    wsprintf(workbuf, "%s\\MSVCRT.DLL", SysPath);

    if(!FileExists(workbuf))
    {
	status = CopyFile(DllSrc, workbuf, TRUE);
	if (!status) 
	{
	    BoxErr3("Could not copy the file MSVCRT.DLL ...\n",
		    GetLastError());
	    return FALSE;
	}
    }
    return TRUE;
}

/*
**  Name: GetPlatform
**
**  Description:  Gets the platform/OS identifier.
**
**  Returns:
**
**	II_WIN95  - Windows 95
**	II_WINNT  - Windows NT
**	II_WIN32S - Windows 32s
*/
DWORD
GetPlatform(VOID)
{
    DWORD	dwVersion;
    DWORD	dwOSVersion;
    DWORD	dwWindowsMajorVersion;
    DWORD	dwWindowsMinorVersion;
    DWORD	dwBuild;

    dwVersion = GetVersion();

    /* Get major and minor version numbers of Windows */

    dwWindowsMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwWindowsMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    /* Get build numbers for Windows NT or Win32s */

    if (dwVersion < 0x80000000)                /* Windows NT */
    {
	dwOSVersion = II_WINNT;
	dwBuild = (DWORD)(HIWORD(dwVersion));
    }
    else if (dwWindowsMajorVersion < 4)        /* Win32s */
    {
	dwOSVersion = II_WIN32S;
	dwBuild = (DWORD)(HIWORD(dwVersion) & ~0x8000);
    }
    else         /* Windows 95 -- No build numbers provided */
    {
	dwOSVersion = II_WIN95;
	dwBuild =  0;
    }

    return(dwOSVersion);
}

/*
**  Name: IsUserIngres
**
**  Description:
**	Determines the validity of the user installing this product.
**
**	15-Mar-2000 (fanch01)
**		Change aUserName, aMessage size to MAXSTRING.
*/
BOOL
IsUserIngres(VOID)
{
    char	aUserName[MAXSTRING], aMessage[MAXSTRING], aTitle[] = "User Verification";
    DWORD	dwUserNameSize = sizeof(aUserName); 			

    if (GetUserName(aUserName,&dwUserNameSize))
    {
	if (strncmp(aUserName,"ingres",6) != 0)
	{
	    strcpy(aMessage,"The user installing OpenIngres must be ingres.  Please refer to the installation instructions for details."); 
	    MessageBox( NULL, aMessage, aTitle,
			MB_APPLMODAL | MB_ICONSTOP | MB_OK);
	    return FALSE;
	}
	else
	    return TRUE;
    }
    strcpy(aMessage,"Unable to determine the User installing OpenIngres.  Please contact Computer Associates support."); 
    MessageBox(NULL, aMessage, aTitle, MB_APPLMODAL |  MB_ICONSTOP | MB_OK);
    return FALSE;
}

/*
**  Name: RunningAsAdministrator
**  Description:
**	Desc: checks if user has administrator privileges
*/
BOOL
RunningAsAdministrator(VOID)
{
    BOOL		fAdmin;
    HANDLE		hThread;
    TOKEN_GROUPS	*ptg = NULL;
    DWORD		cbTokenGroups;
    DWORD		dwGroup;
    PSID		psidAdmin;

    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;

    /* First we must open a handle to the access token for this thread. */

    if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread))
    {
	if ( GetLastError() == ERROR_NO_TOKEN)
	{
	    /*
	    ** If the thread does not have an access token, we'll examine the
	    ** access token associated with the process.
	    */
	    if (! OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, 
		&hThread))
	    return (FALSE);
	}
	else 
	    return (FALSE);
    }

    /*
    ** Then we must query the size of the group information associated with
    ** the token. Note that we expect a FALSE result from GetTokenInformation
    ** because we've given it a NULL buffer. On exit cbTokenGroups will tell
    ** the size of the group information.
    */

    if ( GetTokenInformation ( hThread, TokenGroups, NULL, 0, &cbTokenGroups))
	return ( FALSE);

    /*
    ** Here we verify that GetTokenInformation failed for lack of a large
    ** enough buffer.
    */

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	return ( FALSE);

    /*
    ** Now we allocate a buffer for the group information.
    ** Since _alloca allocates on the stack, we don't have
    ** to explicitly deallocate it. That happens automatically
    ** when we exit this function.
    */

    if ( ! ( ptg= _alloca ( cbTokenGroups))) 
	return ( FALSE);

    /*
    ** Now we ask for the group information again.
    ** This may fail if an administrator has added this account
    ** to an additional group between our first call to
    ** GetTokenInformation and this one.
    */

    if ( !GetTokenInformation ( hThread, TokenGroups, ptg, cbTokenGroups,
	 &cbTokenGroups) )
	return (FALSE);

    /* Now we must create a System Identifier for the Admin group. */

    if ( !AllocateAndInitializeSid( &SystemSidAuthority, 2, 
				    SECURITY_BUILTIN_DOMAIN_RID, 
				    DOMAIN_ALIAS_RID_ADMINS,
				    0, 0, 0, 0, 0, 0, &psidAdmin) )
	return (FALSE);

    /*
    ** Finally we'll iterate through the list of groups for this access
    ** token looking for a match against the SID we created above.
    */

    fAdmin= FALSE;

    for (dwGroup= 0; dwGroup < ptg->GroupCount; dwGroup++)
    {
	if ( EqualSid ( ptg->Groups[dwGroup].Sid, psidAdmin))
	{
	    fAdmin = TRUE;
	    break;
	}
    }

    /* Before we exit we must explicity deallocate the SID we created. */

    FreeSid (psidAdmin);

    return (fAdmin);
}

/*
**  Name: CheckVersion
**
**  Description:
**	Ensure that the version of the OS meets the requirements of the 
**	product being installed.
**
**	15-Mar-2000 (fanch01)
**		Change workbuf size to MAXSTRING.
*/
BOOL
CheckVersion(VOID)
{
    OSVERSIONINFO	OSinfo;
    CHAR		workbuf[MAXSTRING];

    /* version is only a restriction on NT */
    if (GetPlatform() != II_WINNT) return(TRUE);

    OSinfo.dwOSVersionInfoSize = sizeof(OSinfo);

    if (GetVersionEx(&OSinfo) == FALSE)
    {
	BoxErr3("Unable to obtain OS version information :\n", GetLastError());
	return(FALSE);
    }

    if ( (OSinfo.dwMajorVersion == 3) && (OSinfo.dwMinorVersion >= 51) )
	return(TRUE);

    if (OSinfo.dwMajorVersion >= 4) return(TRUE);

    wsprintf(workbuf, "The version running is %d.%d\n", OSinfo.dwMajorVersion, OSinfo.dwMinorVersion);
    BoxErr5("The minimum requirement for installing this product is Windows NT version 3.51 or later.  Please upgrade before reinstalling.\n\n", workbuf);
    return(FALSE);
}

/*
**  Name: LocSelHookProc
**
**  Description:
**      Hook Procedure for GetFileNameOpen() common dialog box/
**
**	15-Mar-2000 (fanch01)
**		Change cVolName, BACKUP_LOC_TMP size to _MAX_PATH.
*/
BOOL CALLBACK 
LocSelHookProc(
        HWND hDlg,                /* window handle of the dialog box */
        UINT messg,               /* type of message                 */
        WPARAM wParam,            /* message-specific information    */
        LPARAM lParam)
{
    CHAR	cVolName[_MAX_PATH], BACKUP_LOC_TMP[_MAX_PATH];
    DWORD	SectPerClus, BytesPerSect, NumFreeClus, TotClus, status;
    INT		i;
    BOOL	rc;

    rc=TRUE;
    switch (messg)
    {
	case WM_INITDIALOG:          /* Set Default Values for Entry Fields */
	    hDlgGlobal = hDlg;
	    CenterBox(hDlg);
	    SetDlgItemText(hDlg, IDC_EDIT1, (LPTSTR) &BACKUP_LOC);
	    strcpy(II_VOLUME, BACKUP_LOC);
	    II_VOLUME[3] = '\0';
	    status = GetDiskFreeSpace((LPTSTR)&II_VOLUME, &SectPerClus,
				      &BytesPerSect, &NumFreeClus, &TotClus);
	    AvailBytes=((FLOAT)((LONGLONG)SectPerClus*(LONGLONG)BytesPerSect*
			(LONGLONG)NumFreeClus)/1024);
	    if (AvailBytes > 1000)
	    {
		AvailBytes=((FLOAT)((LONGLONG)SectPerClus*
			    (LONGLONG)BytesPerSect*
			    (LONGLONG)NumFreeClus)/(1024*1000));
		sprintf(AvailKB, "%.2f", AvailBytes);
		strcat(AvailKB, " MB");
	    }
	    else
	    {
		sprintf(AvailKB, "%.2f", AvailBytes);
		strcat(AvailKB, " KB");
	    }
	    SetDlgItemText(hDlg, IDC_AVAIL_SPACE, (LPTSTR) &AvailKB);
	    SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
	    rc=TRUE;
	    break;

	case WM_COMMAND:
	    switch (LOWORD(wParam))
	    {
		case IDOK:
		    GetDlgItemText(hDlg, IDC_EDIT1, (LPTSTR)&BACKUP_LOC_TMP,
				   sizeof (BACKUP_LOC_TMP));
		    if (!(isalpha(BACKUP_LOC_TMP[0]) && BACKUP_LOC_TMP[1]==':'
				&& BACKUP_LOC_TMP[2]=='\\'))
		    {
			BoxErr1("The disk location must be of the form...",
			       "\n	<drive letter>:\\<directory-name>");
			rc=FALSE;
			break;
		    }

		    /*
		    **  We do not support setting BACKUP_LOC to a directory
		    **  name which contains spaces.
		    */
		    if (strstr(BACKUP_LOC_TMP, " ") != NULL)
		    {
#ifdef IngresII
			BoxErr1( "Ingres does not support directory names which",
				"\ncontain spaces.  Please specify another directory name." );
#else
			BoxErr1( "OpenIngres does not support directory names which",
				"\ncontain spaces.  Please specify another directory name." );
#endif
			rc=FALSE;
			break;
		    }

		    strcpy(cVolName, BACKUP_LOC_TMP);
		    cVolName[3] = '\0';
		    if (SupLongFname(cVolName) == FALSE) break;

		    if (!stricmp(BACKUP_LOC_TMP, II_SYSTEM))
		    {
			BoxErr1( "The patch you have chosen is the same as II_SYSTEM.",
				"\nPlease select an alternate location." );
			rc=FALSE;
			break;
		    }

		    if (!FileExists(BACKUP_LOC_TMP))
		    {
			BoxErr1( "The path you have chosen does not exist. However, it",
				"\nwill be created for you at the time of installation." );
		    }

		    strcpy(BACKUP_LOC, BACKUP_LOC_TMP);
		    EndDialog(hDlg,TRUE);
		    break;

		case IDCANCEL:
		    EndDialog(hDlg,FALSE);
		    break;

		default:
		    if (HIWORD(wParam)==CBN_SELCHANGE)
		    {
			i=SendMessage(GetDlgItem(hDlg,IDC_COMBO1), CB_GETCURSEL,
				      0 ,0);
			SendMessage(GetDlgItem(hDlg,IDC_COMBO1), CB_GETLBTEXT,
				    i, (LPARAM) (LPTSTR) &II_VOLUME);
			II_VOLUME[2] = '\\';
			II_VOLUME[3] = '\0';
			if (GetDiskFreeSpace((LPTSTR) &II_VOLUME,
						  &SectPerClus, &BytesPerSect,
						  &NumFreeClus, &TotClus))
			{
			    AvailBytes=((FLOAT)((LONGLONG)SectPerClus*
				(LONGLONG)BytesPerSect*(LONGLONG)NumFreeClus)/
		 		1024);
			    if (AvailBytes > 1000)
			    {
				AvailBytes=((FLOAT)((LONGLONG)SectPerClus*
					    (LONGLONG)BytesPerSect*
					    (LONGLONG)NumFreeClus)/(1024*1000));
				sprintf(AvailKB, "%.2f", AvailBytes);
				strcat(AvailKB, " MB");
			    }
			    else
			    {
				sprintf(AvailKB, "%.2f", AvailBytes);
				strcat(AvailKB, " KB");
			    }
			    SetDlgItemText( hDlg, IDC_AVAIL_SPACE,
					    (LPTSTR)&AvailKB );
			}
		    }
		    else
		    {
			rc=FALSE;
			break;
		    }
	    }
	    break;

	    default:
		rc=FALSE;
	    break;
    }
    return rc;
}

/*
**  Name: LocSelDlgProc
**
**  Description:
**	Dialog procedure for selecting location of install... aka II_SYSTEM.
*/
BOOL WINAPI
LocSelDlgProc(HWND hDlg)
{
    strcpy(II_VOLUME, II_SYSTEM);
    II_VOLUME[3] = '\0';

    strcpy( szFile, "");
    strcpy( szFileTitle, "");

    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hDlg;
    OpenFileName.hInstance         = ghInst;
    OpenFileName.lpstrFilter       = NULL;
    OpenFileName.lpstrCustomFilter = (LPSTR) NULL;
    OpenFileName.nMaxCustFilter    = 0L;
    OpenFileName.nFilterIndex      = 1L;
    OpenFileName.lpstrFile         = szFile;
    OpenFileName.nMaxFile          = sizeof(szFile);
    OpenFileName.lpstrFileTitle    = szFileTitle;
    OpenFileName.nMaxFileTitle     = sizeof(szFileTitle);
    OpenFileName.lpstrInitialDir   = II_VOLUME;
#ifdef IngresII
    OpenFileName.lpstrTitle        = "Ingres Patch Backup Location";
#else
    OpenFileName.lpstrTitle        = "OpenIngres Patch Backup Location";
#endif
    OpenFileName.Flags =  OFN_ENABLEHOOK | OFN_ENABLETEMPLATE |
                        OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = 1;
    OpenFileName.lpfnHook=(LPOFNHOOKPROC)MakeProcInstance(LocSelHookProc, NULL);
    OpenFileName.lpTemplateName = MAKEINTRESOURCE(BACKUP_LOC_DLG);
    if (!GetOpenFileName(&OpenFileName))
    {
	ProcessCDError(CommDlgExtendedError(), hDlg );
	return FALSE;
    } 
    return TRUE;
}

/*
**  Name: ProcessCDError
**
**  Description:
**	Procedure for processing errors returned from opening common dialog
**	boxes (e.g., GetOpenFileName()).
**
**	15-Mar-2000 (fanch01)
**		Change buf size to MAXSTRING.
*/
VOID
ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
    WORD wStringID;
    CHAR buf[MAXSTRING];

    switch(dwErrorCode)
    {
	case CDERR_DIALOGFAILURE:   wStringID=IDS_DIALOGFAILURE;   break;
	case CDERR_STRUCTSIZE:      wStringID=IDS_STRUCTSIZE;      break;
	case CDERR_INITIALIZATION:  wStringID=IDS_INITIALIZATION;  break;
	case CDERR_NOTEMPLATE:      wStringID=IDS_NOTEMPLATE;      break;
	case CDERR_NOHINSTANCE:     wStringID=IDS_NOHINSTANCE;     break;
	case CDERR_LOADSTRFAILURE:  wStringID=IDS_LOADSTRFAILURE;  break;
	case CDERR_FINDRESFAILURE:  wStringID=IDS_FINDRESFAILURE;  break;
	case CDERR_LOADRESFAILURE:  wStringID=IDS_LOADRESFAILURE;  break;
	case CDERR_LOCKRESFAILURE:  wStringID=IDS_LOCKRESFAILURE;  break;
	case CDERR_MEMALLOCFAILURE: wStringID=IDS_MEMALLOCFAILURE; break;
	case CDERR_MEMLOCKFAILURE:  wStringID=IDS_MEMLOCKFAILURE;  break;
	case CDERR_NOHOOK:          wStringID=IDS_NOHOOK;          break;
	case PDERR_SETUPFAILURE:    wStringID=IDS_SETUPFAILURE;    break;
	case PDERR_PARSEFAILURE:    wStringID=IDS_PARSEFAILURE;    break;
	case PDERR_RETDEFFAILURE:   wStringID=IDS_RETDEFFAILURE;   break;
	case PDERR_LOADDRVFAILURE:  wStringID=IDS_LOADDRVFAILURE;  break;
	case PDERR_GETDEVMODEFAIL:  wStringID=IDS_GETDEVMODEFAIL;  break;
	case PDERR_INITFAILURE:     wStringID=IDS_INITFAILURE;     break;
	case PDERR_NODEVICES:       wStringID=IDS_NODEVICES;       break;
	case PDERR_NODEFAULTPRN:    wStringID=IDS_NODEFAULTPRN;    break;
	case PDERR_DNDMMISMATCH:    wStringID=IDS_DNDMMISMATCH;    break;
	case PDERR_CREATEICFAILURE: wStringID=IDS_CREATEICFAILURE; break;
	case PDERR_PRINTERNOTFOUND: wStringID=IDS_PRINTERNOTFOUND; break;
	case CFERR_NOFONTS:         wStringID=IDS_NOFONTS;         break;
	case FNERR_SUBCLASSFAILURE: wStringID=IDS_SUBCLASSFAILURE; break;
	case FNERR_INVALIDFILENAME: wStringID=IDS_INVALIDFILENAME; break;
	case FNERR_BUFFERTOOSMALL:  wStringID=IDS_BUFFERTOOSMALL;  break;

	case 0:   /* User may have hit CANCEL or we got a *very* random error */
	    return;

	default:
	    wStringID=IDS_UNKNOWNERROR;
    }

    LoadString(NULL, wStringID, buf, sizeof(buf));
    MessageBox(hWnd, buf, NULL, MB_APPLMODAL | MB_OK);
    return;
}

/*
**  Name: SupLongFname
**
**  Description:
**      Determines whether a volume (C:\, D:\, etc.) supports long file names.
**
**	15-Mar-2000 (fanch01)
**		Change workbuf size to MAXSTRING.
*/
BOOL
SupLongFname(CHAR *cpVolume)
{
    BOOL status;
    DWORD dwCompLength;
    CHAR workbuf[MAXSTRING];

    GetEnvironmentVariable("CHECK_COMP_LENGTH", workbuf, sizeof(workbuf));
    if (!stricmp(workbuf, "SKIP")) return(TRUE);

    status = GetVolumeInformation(cpVolume, NULL, 0, NULL, &dwCompLength,
                                                NULL, NULL, 0);
    if (status == FALSE)
    {
        BoxErr3("Could not obtain volume information...\n",
                        GetLastError());
        return(FALSE);
    }

    if (dwCompLength == 255) return(TRUE);
    else
    {
        BoxErr1("The directory chosen must reside on a file system that supports long file names.\n", "Please choose another location, or reformat the drive selected with a file system which supports long file names.");
        return(FALSE);
    }
}

/*
**  Name: FindInstalledFiles
**  
**  Description:
**	This procedure searches II_SYSTEM for the Ingres files that are
**	installed. This information is used to copy the appropriate files
**	from the patch into the installation.
**
**  History:
**	21-jan-1999 (somsa01)
**	    Created.
**	04-feb-1999 (somsa01)
**	    This is now run as a separate thread.
**	15-Mar-2000 (fanch01)
**		Removed mline[] unreferenced local variable.
**		Changed FileFileName size to _MAX_PATH.
*/
VOID
FindInstalledFiles()
{
    CHAR	FullFileName[_MAX_PATH];
    INT		i;

    for (i=0; i < NUMFILES; i++)
    {
	sprintf(FullFileName, "%s%s\\%s", II_SYSTEM, filesizes[i].filepath,
		filesizes[i].filename);
	if (FileExists(FullFileName))
	    filesizes[i].found = TRUE;
    }

    Sleep(2000);

    /* Close out the Wait window we started in the main program. */
    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
}

/*
**  Name: CalcTotals
**
**  Description:
**      Calculates the space needed for the files selected.  Uses the
**      "filesizes" array created by "space".
**
**	15-Mar-2000 (fanch01)
**		Change DestFile size to _MAX_PATH.
*/
FLOAT
CalcTotals()
{
    INT		i, iNumFiles = sizeof(filesizes) / sizeof(struct FILEINF);
    FLOAT	fProductsTotal = (FLOAT) 0.0;
    CHAR	DestFile[_MAX_PATH];

    for (i=0; i<iNumFiles; i++)
    {
	if (filesizes[i].found)
	    fProductsTotal += filesizes[i].filesize;
	else if (filesizes[i].newfile)
	{
	    /* See if the new file exists for us to backup. */
	    sprintf(DestFile, "%s%s\\%s", II_SYSTEM, filesizes[i].filepath,
		    filesizes[i].filename);
	    if (FileExists(DestFile))
		fProductsTotal += filesizes[i].filesize;
	}
    }

    return fProductsTotal;
}

/*
**  Name: GenerateISC
**
**  Description:
**	This procedure uses the filesizes array to generate a CA-Installer
**	ISC file on the fly.
**
**  History:
**	22-jan-1999 (somsa01)
**	    Created.
**	26-June-1999 (andbr08)
***		Added functionality to copy the correct version of gca32_1.dll.
**	15-Mar-2000 (fanch01)
**		Changed dirpath entries to be _MAX_PATH size.
**		Removed ISCFile (replaced w/ORdllsrc) used only on two consecutive lines.
**		Changed ORdllsrc, ORdlldir, ORdllorig from 64 elements to _MAX_PATH.
**		Changed ProductName size to MAXSTRING.
**      22-May-2001 (lunbr01)  Bug 105129
**              Fixed access violation when no gca32_1.dll.  Program would
**              try to access past end of filesizes array.
*/
VOID
GenerateISC()
{
    CHAR	ProductName[MAXSTRING];
    FILE	*ISCPtr;
    CHAR	dirpath[100][_MAX_PATH];
    INT		i = 0, j, numdirs = 0;
    BOOL	Found;
    CHAR	ORdllsrc[_MAX_PATH], ORdlldir[_MAX_PATH], ORdllorig[_MAX_PATH];
    char	*tempstrstorage;

#ifdef IngresII
    strcpy(ProductName, "Ingres");
#else
    strcpy(ProductName, "OpenIngres");
#endif

    sprintf(ORdllsrc, "%s\\ingres\\temp\\PATCH.ISC", II_SYSTEM);
    ISCPtr = fopen(ORdllsrc, "w");

    /*
    ** Write the setup part.
    */
    fprintf(ISCPtr, "[setup]\n");
    fprintf(ISCPtr, "  \"PACKAGENAME\",\"%s Patch %s\"\n", ProductName,
	    PATCHNO);
    if (GetPlatform() != II_WIN95)
    {
	fprintf(ISCPtr, "  \"CAPTIONNAME\",\"%s Patch %s for Microsoft Windows NT Installation\"\n",
		ProductName, PATCHNO);
    }
    else
    {
	fprintf(ISCPtr, "  \"CAPTIONNAME\",\"%s Patch %s for Microsoft Windows 95 Installation\"\n",
		ProductName, PATCHNO);
    }
    fprintf(ISCPtr, "  \"APPNAME\", \"%s Patch %s\"\n", ProductName, PATCHNO);
    fprintf(ISCPtr, "  \"RELEASESTRING\", \"Version 2.6\"\n");
#ifdef IngresII
    fprintf(ISCPtr, "  \"BASEAPP\", \"IIPATCH\"\n");
#else
    fprintf(ISCPtr, "  \"BASEAPP\", \"OIPATCH\"\n");
#endif
    fprintf(ISCPtr, "  \"COPYRIGHTSTR\", \"Copyright 2002 Computer Associates International, Inc.\"\n");
    fprintf(ISCPtr, "  \"PRODIDMATCH\", \"000000\"\n");
    fprintf(ISCPtr, "  \"MAJORVERSION\",  \"2\"\n");
    fprintf(ISCPtr, "  \"MINORVERSION\",  \"0\"\n");
    fprintf(ISCPtr, "  \"REVLETTER\",     \"A\"\n\n");

    /*
    ** Write out some basic CA-Installer configs.
    */
    fprintf(ISCPtr, "[bypassui]\n\n");
    fprintf(ISCPtr, "[bypasspid]\n\n");
    fprintf(ISCPtr, "[not_complete]\n\n");
    fprintf(ISCPtr, "[skipwritekey]\n\n");
    fprintf(ISCPtr, "[cd-rom]\n\n");
    fprintf(ISCPtr, "[nosizecheck]\n\n");
    fprintf(ISCPtr, "[skipcaapps]\n\n");
    fprintf(ISCPtr, "[skipplugmsg]\n\n");
    fprintf(ISCPtr, "[one_path]\n\n");
    fprintf(ISCPtr, "[logfile]\n");
    fprintf(ISCPtr, "\"\\ingres\\install\\control\\install_patch.log\"\n\n");

    /*
    ** The product name.
    */
    fprintf(ISCPtr, "[products]\n");
    if (GetPlatform() != II_WIN95)
    {
        fprintf(ISCPtr, "\"A\",\"%s Patch %s\",\"PATCH\",\"%s Patch %s for Microsoft Windows NT\",\"D\"\n\n",
		ProductName, PATCHNO, ProductName, PATCHNO);
    }
    else
    {
        fprintf(ISCPtr, "\"A\",\"%s Patch %s\",\"PATCH\",\"%s Patch %s for Microsoft Windows NT\",\"D\"\n\n",
		ProductName, PATCHNO, ProductName, PATCHNO);
    }

    /*
    ** The license.
    */
    fprintf(ISCPtr, "[license]\n");
    fprintf(ISCPtr, "\"A\",\"NO LICENSE\"\n\n");

    /*
    ** disks for the installation (always CD-ROM).
    */
    fprintf(ISCPtr, "[disks]\n");
    fprintf(ISCPtr, "\"0\",\"\",\"%s Patch %s CD-ROM\"\n\n", ProductName,
	    PATCHNO);

    /*
    ** The directories. We can get this from the filesizes array.
    */
    fprintf(ISCPtr, "[directories]\n");
	
    while ( i < 100 )
    {
	dirpath[i][0] = '\0';
	i++;
    }

	    for (j=0; j < NUMFILES; j++)
	    {
	i = 0;
	Found = FALSE;
	while ( i < 100 && dirpath[i][0] )
	{
	    if (!strcmp(dirpath[i], filesizes[j].filepath))
	    {
		Found = TRUE;
		break;
	    }
	    ++i;
	}
	if ((i < 100) && !Found)
	{
	    strcpy(dirpath[i], filesizes[j].filepath);
	    if (dirpath[i][strlen(dirpath[i])-1] == '\\')
	    {
		dirpath[i][strlen(dirpath[i])-1] = '\0';
		filesizes[j].filepath[strlen(filesizes[j].filepath)-1] = '\0';
	    }
	    numdirs++;
	}
    }

    /* add the correct oigca32_1.dll to the filesizes array now so that it has its directory */
    /* code added in the next loop and is added to the isc file*/

	/*first check the platform and assign the directories */
	if (GetPlatform() == II_WINNT)
	{
	    /*Install winNT version of GCA32_1.DLL*/
	    sprintf(ORdllsrc, "%s\\ingres\\OR_NT\\gca32_1.dll", CDIMAGE_LOC);
	    sprintf(ORdlldir, "%s\\ingres\\bin\\gca32_1.dll", CDIMAGE_LOC);
	}

    else if (GetPlatform() == II_WIN95 || GetPlatform() == II_WIN32S)
	{
	    /*Install win95 version of GCA32_1.DLL*/
	    sprintf(ORdllsrc, "%s\\ingres\\OR_95\\gca32_1.dll", CDIMAGE_LOC);
	    sprintf(ORdlldir, "%s\\ingres\\bin\\gca32_1.dll", CDIMAGE_LOC);
	}
	sprintf(ORdllorig, "%s\\ingres\\bin\\gca32_1.dll", II_SYSTEM);

	/* find gca32_1.dll entry in filesizes array if it does not exist then don't try */
	/* this functionality to install the correct gca32_1.dll */
	for (j = 0; (j < NUMFILES && strcmp(filesizes[j].filename, "gca32_1.dll")); j++) {}
	if ( j < NUMFILES )
	   if ( (!strcmp(filesizes[j].filename, "gca32_1.dll")) && 
		( (!strcmp(filesizes[j].filepath, "\\ingres\\OR_95")) || 
		  (!strcmp(filesizes[j].filepath, "\\ingres\\OR_NT")) )) {
		/* first gca32_1.dll exists does the second? */
		for (i = (j + 1); (strcmp(filesizes[i].filename, "gca32_1.dll")) && i < NUMFILES; i++) {}
		if (!strcmp(filesizes[i].filename, "gca32_1.dll")) {
		/* both gca32_1.dll files exist in fsizes.roc */
			
			/* only install if both the new gca32_1.dll exists and there is an old */
			/* version installed */
			if (FileExists(ORdllsrc) && FileExists(ORdllorig))
			{
    			tempstrstorage = (char *)malloc(14);
				strcpy(tempstrstorage, "\\ingres\\bin");
				tempstrstorage[13] = '\0';
				filesizes[j].filepath = tempstrstorage;
				tempstrstorage = (char *)malloc(12);
				strcpy(tempstrstorage, "gca32_1.dll");
				tempstrstorage[11] = '\0';
				filesizes[j].filename = tempstrstorage;
				tempstrstorage = (char *)malloc(12);
				strcpy(tempstrstorage, "gca32_1.dll");
				tempstrstorage[11] = '\0';
				filesizes[j].cdimage_name = tempstrstorage;
				filesizes[j].dircode[0] = ' ';
				filesizes[j].dircode[1] = ' ';
				filesizes[j].found = 1;
				filesizes[j].newfile = 0;

				CopyFile(ORdllsrc, ORdlldir, FALSE);
				filesizes[j].filesize = GetCurrentFileSize(ORdllsrc);
			}
			else
			{
				filesizes[j].found = 0;
				filesizes[j].newfile = 0;
			}
			filesizes[i].found = 0;
			filesizes[i].newfile = 0;

		}
		else
		{
			filesizes[j].found = 0;
			filesizes[j].newfile = 0;
		}
	}

    i = 0;
    while (i < numdirs)
    {
	/*
	** Update the dircode for the files we will be copying. We will need
	** this later...
	*/
	j = 0;
	while (j < NUMFILES)
	{
	    if ((filesizes[j].found || filesizes[j].newfile) &&
		!strcmp(filesizes[j].filepath, dirpath[i]))
	    {
		filesizes[j].dircode[0] = ((i) / 26) + 'A';
		filesizes[j].dircode[1] = ((i) % 26) + 'A';
	    }
	    j++;
	}

	/*
	** Now, write out the directories.
	*/
	if (!strcmp(dirpath[i],"\\ingres\\files\\dictfile"))
	{
	    fprintf(ISCPtr, "\"A\",\"%c%c\",\"%ss\",\"%s\"\n",
		    ((i) / 26) + 'A', ((i) % 26) + 'A',
		    dirpath[i], dirpath[i]);
	}
	else if (!strcmp(dirpath[i],"\\ingres\\files\\collatio"))
	{
	    fprintf(ISCPtr, "\"A\",\"%c%c\",\"%sn\",\"%s\"\n",
		    ((i) / 26) + 'A', ((i) % 26) + 'A',
		    dirpath[i], dirpath[i]);
	}
	else
	{
	    fprintf(ISCPtr, "\"A\",\"%c%c\",\"%s\",\"\"\n",
		    ((i) / 26) + 'A', ((i) % 26) + 'A',
		    dirpath[i]);
	}
	i++;
    }

    fprintf(ISCPtr, "\n");

    /*
    ** The files section. Also retrieved from the filesizes array. Remember
    ** that we ONLY want to put files which exist currently in the
    ** installation OR files which are brand new to Ingres.
    */
    fprintf(ISCPtr, "[files]\n");
    for (i=0; i<NUMFILES; i++)
    {
	if (filesizes[i].found || filesizes[i].newfile)
	{
	    fprintf(ISCPtr, "\"0\",\"A\",\"RCO2\",\"%c%c\",\"%s\",\"%s\",\"%s\",\"\",\"\",\"\",\"%i\",\"%i\",\"\",\"\"\n",
		    filesizes[i].dircode[0], filesizes[i].dircode[1],
		    filesizes[i].filepath, filesizes[i].filename,
		    filesizes[i].cdimage_name, filesizes[i].filesize / 1024,
		    filesizes[i].filesize / 1024);
	}
    }

    fclose(ISCPtr);
}

/*
**  Name: InstallerSetupThread
**
**  Description:
**	This procedure runs in a separate thread. It performs all
**	necessary steps before we execute the CA-Installer.
**
**  History:
**	23-jan-1999 (somsa01)
**	    Created.
**	15-Mar-2000 (fanch01)
**		Change SourceFile, DestFile, DestDir, TmpBfr size to _MAX_PATH.
*/
INT
InstallerSetupThread()
{
    CHAR	SourceFile[_MAX_PATH], DestFile[_MAX_PATH], DestDir[_MAX_PATH], TmpBfr[_MAX_PATH];
    CHAR	TimeStamp[25], mline[MAXSTRING];
    FILE	*PreDataPtr, *VersPtr, *NewVersPtr;
    INT		sequence;
    DWORD	SourceChecksum;
	INT		hack_flag = 0;

    if (!Backup)
	fprintf(PatchInstallLog, "Warning:  The user has chosen not to backup of those files which will\nbe replaced by the patch installer.\n\n");

    /*
    ** Initially set up the version.dat file, backing the old one into
    ** the control directory.
    */
    sprintf(DestDir, "%s\\ingres\\install\\control\\save", II_SYSTEM);
    CheckMakeDirRec(DestDir, TRUE);
    sprintf(SourceFile, "%s\\ingres\\version.dat", II_SYSTEM);
    if (FileExists(SourceFile))
    {
	CHAR	PatchNum[6], PatchNum2[6];

	GetTimeOfDay(TRUE, TimeStamp);
	sprintf(DestFile, "%s\\ingres\\install\\control\\save\\version.dat.%s",
		II_SYSTEM, TimeStamp);
	CopyFile(SourceFile, DestFile, FALSE);

	/* Se if we've attempted to install this patch before. */
	VersPtr = fopen(SourceFile, "r");
	while (fgets(mline, MAXSTRING - 1, VersPtr) != NULL)
	{
	    PatchNum[0] = mline[45];
	    PatchNum[1] = mline[46];
	    PatchNum[2] = mline[47];
	    PatchNum[3] = mline[48];
	    PatchNum[4] = mline[49];
	    PatchNum[5] = '\0';
	    sprintf(PatchNum2, "%5s", PATCHNO);
	    if (!strcmp(PatchNum, PatchNum2))
	    {
		CHAR	seqnum[3];

		seqnum[0] = mline[52];
		seqnum[1] = mline[53];
		seqnum[2] = '\0';
		PatchSeq = atoi(seqnum);
		PatchSeq++;
		break;
	    }
	}
	fclose(VersPtr);

	sprintf(DestFile, "%s\\ingres\\version.dat.new", II_SYSTEM);
	NewVersPtr = fopen(DestFile, "w");
	VersPtr = fopen(SourceFile, "r");
	fprintf(NewVersPtr, "## action         performed          status  patch seq          version\n##\n");
	GetTimeOfDay(FALSE, TimeStamp);
	fprintf(NewVersPtr, "install   %24s  invalid  %5s  %2i  %s\n",
		TimeStamp, PATCHNO, PatchSeq, IngVersion);

	/* Skip the first two lines in the old vers file. */
	fgets(mline, MAXSTRING - 1, VersPtr);
	fgets(mline, MAXSTRING - 1, VersPtr);

	while (fgets(mline, MAXSTRING - 1, VersPtr) != NULL)
	    fprintf(NewVersPtr, "%s", mline);
	fclose(VersPtr);
	fclose(NewVersPtr);
	DeleteFile(SourceFile);
	CopyFile(DestFile, SourceFile, FALSE);
	DeleteFile(DestFile);
    }
    else
    {
	VersPtr = fopen(SourceFile, "w");
	fprintf(VersPtr, "## action         performed          status  patch seq          version\n##\n");
	GetTimeOfDay(FALSE, TimeStamp);
	fprintf(VersPtr, "install   %24s  invalid  %5s  %2i  %s\n",
		TimeStamp, PATCHNO, PatchSeq, IngVersion);
	fprintf(VersPtr, "##\n## File format version:	1\n##\n");
	fclose(VersPtr);
    }

    /*
    ** Run "checksums" on the cdimage.
    */
    if (CheckCdimage())
    {
	CHAR	MessageOut[1024];

	sprintf(MessageOut, "%s\\ingres\\files\\install_p%s.log file for more information.",
		II_SYSTEM, PATCHNO);
		
	/* Stop the wait clock. */
	SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
	BoxErr5("An error has occurred while verifying the cdimage. Please refer to the\n",
		MessageOut);

	/* Close out the Wait window. */
	SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
	return(1);
    }

    /*
    ** Generate CA-Installer files: PATCH.ISC and RESPONSE.RSP
    */
    SetDlgItemText( hDlgGlobal, IDC_TEXT_WAIT,
		    "Generating installation scripts ...");
    GenerateISC();
    MakeResponse();

    /*
    ** Copy the plist file into the control area.
    */
    sprintf(DestDir, "%s\\ingres\\install\\control", II_SYSTEM);
    CheckMakeDirRec(DestDir, TRUE);

    sequence = 1;
    sprintf(DestFile, "%s\\ingres\\install\\control\\plist-p%s_%i.dat",
	    II_SYSTEM, PATCHNO, sequence);
    while (FileExists(DestFile))
    {
	/* Find a new sequence number. */
	sequence++;
	sprintf(DestFile, "%s\\ingres\\install\\control\\plist_p%s_%i.dat",
		II_SYSTEM, PATCHNO, sequence);
    }
    sprintf(SourceFile, "%s\\ingres\\plist", CDIMAGE_LOC);
    CopyFile(SourceFile, DestFile, FALSE);

    /*
    ** If we are to backup installation files, then do it now. At the same
    ** time, we'll also create the pre-patch data file.
    */
    if (Backup)
    {
	INT	i;

	fprintf(PatchInstallLog,
		"Backing up files which will be replaced ...\n\n");
	SetDlgItemText( hDlgGlobal, IDC_TEXT_WAIT,
			"Backing up files which will be replaced ...");
	sequence = 1;
	sprintf(DestFile, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
		II_SYSTEM, PATCHNO, sequence);
	while (FileExists(DestFile))
	{
	    /* Find a new sequence number. */
	    sequence++;
	    sprintf(DestFile, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
		    II_SYSTEM, PATCHNO, sequence);
	}

	sprintf(PatchBackupLoc, "%s\\pre-p%s_%i", BACKUP_LOC, PATCHNO,
		sequence);
	fprintf(PatchInstallLog, "Backup directory:  %s\n\n",
		PatchBackupLoc);
	if (!CheckMakeDirRec(PatchBackupLoc, TRUE))
	{
	    fprintf(PatchInstallLog, "Unable to create backup directory structure! Please check\nand correct permissions.\n\n");
	    SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
	    BoxErr4("Unable to create backup directory structure!", "");
	    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
	    return(1);
	}

	PreDataPtr = fopen(DestFile, "w");
	fprintf(PreDataPtr, "## Version:	%s\n", IngVersion);
	fprintf(PreDataPtr, "## Patch:	%s\n", PATCHNO);
	if (Backup)
	    fprintf(PreDataPtr, "## Task:	backup_install\n");
	else
	    fprintf(PreDataPtr, "## Task:	install_patch\n");
	GetTimeOfDay(FALSE, TimeStamp);
	fprintf(PreDataPtr, "## Performed:	%s\n", TimeStamp);
	fprintf(PreDataPtr, "## Status:	invalid\n");
	fprintf(PreDataPtr, "## Source:	%s\n", II_SYSTEM);
	fprintf(PreDataPtr, "## Target:	%s\n", PatchBackupLoc);
	fprintf(PreDataPtr, "## Sequence:	%i\n", sequence);
	fprintf(PreDataPtr, "##\n##\n## Contents:\n");
	fprintf(PreDataPtr, "##	Backup information recorded during the backup of all\n");
	fprintf(PreDataPtr, "##	affected files in \"Source\" installation to \"Target\"\n");
	fprintf(PreDataPtr, "##	area prior to the application of \"Patch\" noted above.\n");
	fprintf(PreDataPtr, "##\n##\n");
	fprintf(PreDataPtr, "##                  file                        size     checksum\n");
	fprintf(PreDataPtr, "##\n");

	for (i=0; i < NUMFILES; i++)
	{
	    if (filesizes[i].found || filesizes[i].newfile)
	    {
		sprintf(SourceFile, "%s%s\\%s", II_SYSTEM,
			filesizes[i].filepath, filesizes[i].filename);
		if (FileExists(SourceFile))
		{
		    sprintf(DestFile, "%s%s\\%s", PatchBackupLoc,
			    filesizes[i].filepath, filesizes[i].filename);
		    sprintf(DestDir, "%s%s", PatchBackupLoc,
			    filesizes[i].filepath);
		    if (!FileExists(DestDir))
		    {
			if (!CheckMakeDirRec(DestDir, TRUE))
			{
			    fprintf(PatchInstallLog, "Unable to create backup directory structure! Please check\nand correct permissions.\n\n");
			    SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
			    BoxErr4("Unable to create backup directory structure!",
				    "");
			    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
			    return(1);
			}
		    }
		    
		    if (!CopyFile(SourceFile, DestFile, FALSE))
		    {
			DWORD status = GetLastError();

			fprintf(PatchInstallLog, "Unable to backup files, CopyFile error %d. Please check\nand correct permissions.\n\n", status);
			SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
			BoxErr2("Unable to back up files!\nError = ", status);
			SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
			return(1);
		    }

		    sprintf(TmpBfr, "%s\\%s", filesizes[i].filepath,
			    filesizes[i].filename);
			SourceChecksum = GetCurrentFileChecksum(SourceFile);
			fprintf(PreDataPtr, "%-58s  %9d  %10u\n", TmpBfr,
			    GetCurrentFileSize(SourceFile), SourceChecksum);
		}
	    }
	}

	fprintf(PreDataPtr, "##\n##\n## File format version: 1\n");
	fprintf(PreDataPtr, "##\n##\n## State:	completed\n");
	fclose(PreDataPtr);

	fprintf(PatchInstallLog, "Backup completed; verifying files which were backed up ...\n\n");
	if(!CheckBackup(sequence))
	{
	    fprintf(PatchInstallLog, "Verification of backup complete.\n\n");
	    fprintf(PatchInstallLog, "Backup completed successfully.\n\n");
	}
	else
	{
	    CHAR	MessageOut[1024];

	    sprintf(MessageOut, "%s\\ingres\\files\\install_p%s.log file for more information.",
		    II_SYSTEM, PATCHNO);
	    fprintf(PatchInstallLog,
		    "\nVerification failed; Backup failed.\n\n");
	    SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
	    BoxErr5("The backup did not complete due to verification failure. Please see the\n",
		    MessageOut);
	    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
	    return(1);
	}
    }

	/* Add 2.0/0001 entries to config.dat and back it up */
	if (Upgradeto0001)
	{
		hack_flag |= IIOLD_TO_II0001;
		if (Backup)
			hack_flag |= BACKUP_CONFIG;
		HackIngVersion(hack_flag, Upgradeto0001);
	}
	/* Add 2.6/0305 entries to config.dat and back it up */
	if (Upgradeto26NEW)
	{
		hack_flag |= II26OLD_TO_II26NEW;
		if (Backup)
			hack_flag |= BACKUP_CONFIG;
		HackIngVersion(hack_flag, Upgradeto26NEW);
	}

    Sleep(2000);

    /* Close out the Wait window we started in the main program. */
    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
    return(0);
}

/*
**  Name: CheckMakeDir
**
**  Description:
**	Checks for the existence of a directory, creates it if missing
**	and reports if it is unable to do so.
**
**	15-Mar-2000 (fanch01)
**		Change workbuf size to _MAX_PATH.
*/
BOOL
CheckMakeDir(CHAR *cDirName)
{
    CHAR workbuf[_MAX_PATH];
    DWORD dwErrCode;

    if (FileExists(cDirName))
	return TRUE;
    if (CreateDirectory(cDirName, NULL))
	return TRUE;
    dwErrCode = GetLastError();

    wsprintf(workbuf, "Unable to access %s :\n", cDirName);
    BoxErr3(workbuf, dwErrCode);

    return FALSE;
}

/*
**  Name: CheckMakeDirRec
**
**  Description:
**	Checks for the existence of a directory structure, creates it if
**	missing and reports if it is unable to do so.
**
**	15-Mar-2000 (fanch01)
**		Change workbuf, temp size to _MAX_PATH.
*/
BOOL
CheckMakeDirRec(CHAR *cDirName, BOOL PermanentDir)
{
    CHAR	workbuf[_MAX_PATH], temp[_MAX_PATH], *ptr1, *ptr2;
    BOOL	FoundLastDir = FALSE;
    INT		i, j, numlevels, len;

    if (FileExists(cDirName))
	return TRUE;

    ptr1 = ptr2 = cDirName;
    numlevels = 0;
    while (ptr2!=NULL)
    {
	ptr2 = strstr(ptr1, "\\");
	if (ptr2!=NULL)
	{
	    numlevels++;
	    ptr2++;
	    ptr1=ptr2;
	}
    }

    strcpy (temp, cDirName);
    ptr2 = temp;
    ptr1 = strstr(ptr2, "\\"); ptr1++;
    ptr2 = strstr(ptr1, "\\");
    if (ptr2!=NULL)
        strset(ptr2,'\0');
    strcpy (workbuf, temp);
    if (!PermanentDir)
    {
	if (!FileExists(workbuf))
	{
	    if (!CheckMakeDir(workbuf))
	        return FALSE;
	    RemoveDirectory(workbuf);
	    return TRUE;
	}
    }
    else
    {
	if (!CheckMakeDir(workbuf))
	    return FALSE;
    }

    for (j=1; j<numlevels; j++)
    {
	strcpy (temp, cDirName);
	ptr1 = temp;
	strcat (workbuf, "\\");
	len = strlen(workbuf);
	for (i=0; i<len; i++)
	    ptr1++;
	ptr2 = strstr(ptr1, "\\");
	if (ptr2!=NULL)
	    strset(ptr2, '\0');
	strcat (workbuf, ptr1);

	if (!PermanentDir)
	{
	    if (!FileExists(workbuf))
	    {
		if (!CheckMakeDir(workbuf))
		    return FALSE;
		RemoveDirectory(workbuf);
		return TRUE;
	    }
	}
	else
	{
	    if (!CheckMakeDir(workbuf))
		return FALSE;
	}
    }
    return TRUE;
}

/*
**  Name: GetTimeOfDay
**
**  Description:
**	This procedure gets the current time of day and returns it as an
**	ASCII string.
**
**  History:
**	24-jan-1999 (somsa01)
**	    Created.
*/
VOID
GetTimeOfDay(BOOL timestamp, CHAR *TimeString)
{
    SYSTEMTIME	LocalTime;

    GetLocalTime(&LocalTime);

    if (!timestamp)
    {
	sprintf(TimeString, "%s %s %02d %02d:%02d:%02d %d",
		DayOfWeek[LocalTime.wDayOfWeek].Day,
		Month[LocalTime.wMonth - 1].Month,
		LocalTime.wDay, LocalTime.wHour,
		LocalTime.wMinute, LocalTime.wSecond,
		LocalTime.wYear);
    }
    else
    {
	sprintf(TimeString, "%d%02d%02d_%02d%02d%02d",
		LocalTime.wYear, LocalTime.wMonth, LocalTime.wDay,
		LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond);
    }
}

/*
**  Name: GetCurrentFileSize
**
**  Description:
**	Retrieves the current size of the given file name.
**
**  History:
**	24-jan-1999 (somsa01)
**	    Created.
*/
LONG
GetCurrentFileSize (CHAR *fname)
{
    HANDLE	hFile;
    DWORD	size;

    hFile = CreateFile( fname, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, 0 ,0 );

    size = GetFileSize(hFile, NULL);
    CloseHandle(hFile);

    return (size);
}

/*
**  Name: FormatInstallLog
**
**  Description:
**	This procedure formats the log file generated by the CA-Installer
**	during the installation of the patch. We also update the status
**	in the version.dat file to "valid".
**
**  History:
**	24-jan-1999 (somsa01)
**	    Created.
**	15-Mar-2000 (fanch01)
**		Change InstallLog, NewVersFile size to _MAX_PATH.
**		Change buffer<-mline copy loop - possible bug [-1] reference.
**		Removed i, j local vars, used only in code mentioned above.
**		Change buffer size to _MAX_PATH.
*/
INT
FormatInstallLog()
{
    CHAR	InstallLog[_MAX_PATH], mline[MAXSTRING] = "\0", buffer[_MAX_PATH];
    CHAR	TimeStamp[25], NewVersFile[_MAX_PATH];
    INT		sequence;
    FILE	*InstallDataPtr, *InstallLogPtr, *VersPtr, *NewVersPtr;
    DWORD	CheckSum;

    SetDlgItemText( hDlgGlobal, IDC_TEXT_WAIT, "Finishing up ...");
    sequence = 1;
    sprintf(InstallLog, "%s\\ingres\\install\\control\\install-p%s_%i.dat",
		II_SYSTEM, PATCHNO, sequence);
    while (FileExists(InstallLog))
    {
	/* Find a new sequence number. */
	sequence++;
	sprintf(InstallLog, "%s\\ingres\\install\\control\\install-p%s_%i.dat",
		II_SYSTEM, PATCHNO, sequence);
    }
    InstallDataPtr = fopen(InstallLog, "w");

    fprintf(InstallDataPtr, "## Version:	%s\n", IngVersion);
    fprintf(InstallDataPtr, "## Patch:	%s\n", PATCHNO);
    if (Backup)
	fprintf(InstallDataPtr, "## Task:	backup_install\n");
    else
	fprintf(InstallDataPtr, "## Task:	install_patch\n");
    GetTimeOfDay(FALSE, TimeStamp);
    fprintf(InstallDataPtr, "## Performed:	%s\n", TimeStamp);
    fprintf(InstallDataPtr, "## Status:	invalid\n");
    fprintf(InstallDataPtr, "## Source:	%s\n", CDIMAGE_LOC);
    fprintf(InstallDataPtr, "## Target:	%s\\ingres\n", II_SYSTEM);
    fprintf(InstallDataPtr, "## Sequence:	%i\n", sequence);
    fprintf(InstallDataPtr, "##\n##\n## Contents:\n");
    fprintf(InstallDataPtr, "##	Version and patch ID information for installation.\n");
    fprintf(InstallDataPtr, "##	List of installed patch files.\n");
    fprintf(InstallDataPtr, "##\n##\n## Notes:\n");
    if (Backup)
	fprintf(InstallDataPtr, "##	Backup location:	%s\n",
		PatchBackupLoc);
    else
	fprintf(InstallDataPtr, "##	No backup performed during this install run.\n");
    fprintf(InstallDataPtr, "##\n##\n");
    fprintf(InstallDataPtr, "##                  file                        size     checksum\n");
    fprintf(InstallDataPtr, "##\n");
    
    sprintf(InstallLog, "%s\\ingres\\install\\control\\install_patch.log",
	    II_SYSTEM);
    InstallLogPtr = fopen(InstallLog, "r");

    while (fgets(mline, MAXSTRING - 1, InstallLogPtr) != NULL)
    {
	if (strstr(mline, "[Files]"))
	{
	    if (fgets(mline, MAXSTRING - 1, InstallLogPtr) == NULL)
		break;
	    while ((fgets(mline, MAXSTRING - 1, InstallLogPtr) != NULL) &&
		    strcmp(mline, "[Files_End]\n"))
	    {
		if (strstr(mline, "install_patch.log"))
		    continue;  /* Ignore the actual log file copied. */

		if (mline[strlen(mline) - 1] == '\n')
		    mline[strlen(mline) - 1] = '\0';

		/* Strip off II_SYSTEM and place the file into the buffer. */
		strcpy (buffer, &mline[strlen(II_SYSTEM)]);

		CheckSum = GetCurrentFileChecksum(mline);
		fprintf(InstallDataPtr, "%-58s  %9d  %10u\n", buffer,
			GetCurrentFileSize(mline), CheckSum);
	    }
	}
    }

    fprintf(InstallDataPtr, "##\n##\n## File format version:	1\n");
    fprintf(InstallDataPtr, "##\n##\n## State:	completed\n");

    fclose(InstallDataPtr);
    fclose(InstallLogPtr);
    DeleteFile(InstallLog);

    /*
    ** Verify what was copied.
    */
    fprintf(PatchInstallLog, "Patch install completed; verifying files which were installed ...\n\n");
    if(!CheckInstall(sequence))
    {
	fprintf(PatchInstallLog, "Verification of patch install complete.\n\n");
    }
    else
    {
	CHAR	MessageOut[1024];

	sprintf(MessageOut, "%s\\ingres\\files\\install_p%s.log file for more information.",
		II_SYSTEM, PATCHNO);
	fprintf(PatchInstallLog, "\nVerification failed; Install failed.\n\n");
	SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
	BoxErr5("The patch install did not complete due to verification failure. Please see the\n",
		MessageOut);
	SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
	return(1);
    }

    /*
    ** Update the status of the patch install in the version.dat file.
    */
    sprintf(InstallLog, "%s\\ingres\\version.dat", II_SYSTEM);
    sprintf(NewVersFile, "%s\\ingres\\version.dat.new", II_SYSTEM);
    VersPtr = fopen(InstallLog, "r");
    NewVersPtr = fopen(NewVersFile, "w");
    fgets(mline, MAXSTRING - 1, VersPtr);
    fprintf(NewVersPtr, "%s", mline);
    fgets(mline, MAXSTRING - 1, VersPtr);
    fprintf(NewVersPtr, "%s", mline);

    fgets(mline, MAXSTRING - 1, VersPtr);
    mline[36] = ' ';
    mline[37] = ' ';
    fprintf(NewVersPtr, "%s", mline);

    while (fgets(mline, MAXSTRING - 1, VersPtr) != NULL)
	fprintf(NewVersPtr, "%s", mline);
    fclose(VersPtr);
    fclose(NewVersPtr);
    DeleteFile(InstallLog);
    CopyFile(NewVersFile, InstallLog, FALSE);
    DeleteFile(NewVersFile);    

    return(0);
}

/*
**  Name: CheckCdimage
**
**  Description:
**	This procedure checks the sizes and checksums between the
**	cdimage and plist, to make sure that it is intact.
**
**  History:
**	26-jan-1999 (somsa01)
**	    Created.
**	03-feb-1999 (somsa01)
**	    Corrected typo in fprintf() statement.
**	15-Mar-2000 (fanch01)
**		Change FileName, PlistFileName size to _MAX_PATH.
**  01-Apr-2003 (rodjo04) bug 109344
**      Added error checking for fopen() in case the patch was 
**		unpacked without the directory structure. 
**  15-Jun-2004 (lunbr01) bug 109344 correction
**      Previous change required extra parens to set PlistPtr value
**      before comparing to NULL.  PlistPtr was being set to zero
**      and the patch install would fail in the verification step
**      with same symptom as original bug 109344.
**      
*/
INT
CheckCdimage()
{
    FILE	*PlistPtr;
    CHAR	FileName[_MAX_PATH], mline[MAXSTRING], PlistChecksum[11], ErrorMsg[MAXSTRING];
    CHAR	PlistSize[10], PlistFileName[_MAX_PATH], *CharPtr;
    INT		i;
    BOOL	BadCheck = FALSE, PrintHeader = FALSE, ValidFile;
    LONG	FileSize;
    DWORD	FileChecksum;

    fprintf(PatchInstallLog, "Verifying files in the cdimage ...\n\n");
    SetDlgItemText( hDlgGlobal, IDC_TEXT_WAIT,
		    "Verifying files in the cdimage ...");

    sprintf(FileName, "%s\\ingres\\plist", CDIMAGE_LOC);
    if ((PlistPtr = fopen(FileName, "r")) == NULL)
	{
      sprintf(ErrorMsg, "%s was not found.", FileName);
      BoxErr4(ErrorMsg, "\nPlease verify file structure." );
	  exit(1);
	}
		  

    /*
    ** For each file found, check its size and checksum.
    */
    while ( (fgets(mline, MAXSTRING - 1, PlistPtr) != NULL) &&
	    (strstr(mline, "##")) );
    do
    {
	mline[strlen(mline)-1] = '\0';
	CharPtr = mline;
	strcpy(PlistFileName, CDIMAGE_LOC);
	strncat(PlistFileName, CharPtr, 58);
	while(PlistFileName[strlen(PlistFileName)-1] == ' ')
	    PlistFileName[strlen(PlistFileName)-1] = '\0';
	for (i=0; i < 60; i++)
	    CharPtr++;

	strncpy(PlistSize, CharPtr, 9);
	PlistSize[9] = '\0';
	for (i=0; i < 11; i++)
	    CharPtr++;

	strncpy(PlistChecksum, CharPtr, 10);
	PlistChecksum[10] = '\0';

	if (!FileExists(PlistFileName))
	    ValidFile = FALSE;
	else
	{
	    FileSize = GetCurrentFileSize(PlistFileName);
	    FileChecksum = GetCurrentFileChecksum(PlistFileName);
		
	    ValidFile = TRUE;
	}

	if ((!ValidFile) ||
	    (FileChecksum != (DWORD)atoi(PlistChecksum)) ||
	    (FileSize != (LONG)atoi(PlistSize)))
	{
	    CharPtr = mline;
	    strncpy(PlistFileName, CharPtr, 58);
	    PlistFileName[58] = '\0';
	    if (!PrintHeader)
	    {
		fprintf(PatchInstallLog, "Check of file(s) differed between plist and cdimage.\n");
		fprintf(PatchInstallLog, "\"<\":  values of file(s) from cdimage\n");
		fprintf(PatchInstallLog, "\">\":  values of file(s) recorded in plist\n\n");
		PrintHeader = TRUE;
	    }

		if (ValidFile)
	    {
		fprintf(PatchInstallLog, "< %s  %9d  %10u\n", PlistFileName,
			FileSize, FileChecksum);
	    }
	    else
	    {
		fprintf(PatchInstallLog, "< FILE DOES NOT EXIST\n");
	    }
	    fprintf(PatchInstallLog, "> %s\n", mline);
		
	    BadCheck = TRUE;
	}
    }
    while ( (fgets(mline, MAXSTRING - 1, PlistPtr) != NULL) &&
	    (!strstr(mline, "##")) );
    fclose(PlistPtr);
    if (BadCheck)
    {
	if (ForceInstall)
	{
	    fprintf(PatchInstallLog, "\nContinuing...\n\n");
	    BadCheck = FALSE;
	}
	else
	    fprintf(PatchInstallLog, "\nAn error occurred during the pre-install phase.\n\n");
    }
    else
	fprintf(PatchInstallLog, "Files in the cdimage look to be OK.\n\n");
    return(BadCheck);
}

/*
**  Name: CheckBackup
**
**  Description:
**	This procedure checks the sizes and creation dates between II_SYSTEM
**	and the backup, to make sure that backup was successful.
**
**  History:
**	26-jan-1999 (somsa01)
**	    Created.
**	27-May-1999 (andbr08)
**	    Changed incorrect formatting of install log where '<' was used twice.
**	15-Mar-2000 (fanch01)
**		Change SourceFile, DestFile, TmpFile, BackupFile, NewBackupFile size
**		to _MAX_PATH.
*/
INT
CheckBackup(INT sequence)
{
    CHAR	SourceFile[_MAX_PATH], DestFile[_MAX_PATH];
    CHAR	TmpFile[_MAX_PATH], mline[MAXSTRING];
    INT		i;
    BOOL	BadCheck = FALSE, PrintHeader = FALSE;
    LONG	SourceSize, DestSize;
    CHAR	BackupFile[_MAX_PATH], NewBackupFile[_MAX_PATH];
    FILE	*BackupPtr, *NewBackupPtr;
    DWORD	SourceChecksum, DestChecksum;

    SetDlgItemText( hDlgGlobal, IDC_TEXT_WAIT,
		    "Verifying files which were backed up ...");
    /*
    ** For each file copied, check its size and checksum.
    */
    for (i=0; i < NUMFILES; i++)
    {
	if (filesizes[i].found || filesizes[i].newfile)
	{
	    sprintf(SourceFile, "%s%s\\%s", II_SYSTEM,
		    filesizes[i].filepath, filesizes[i].filename);
	    if (FileExists(SourceFile))
	    {
		sprintf(DestFile, "%s%s\\%s", PatchBackupLoc,
			filesizes[i].filepath, filesizes[i].filename);
		SourceSize = GetCurrentFileSize(SourceFile);
		DestSize = GetCurrentFileSize(DestFile);
		
		SourceChecksum = GetCurrentFileChecksum(SourceFile);
		DestChecksum = GetCurrentFileChecksum(DestFile);

		if ((SourceSize != DestSize) ||
		    (SourceChecksum != DestChecksum))
		{
		    if (!PrintHeader)
		    {
			fprintf(PatchInstallLog, "Check of file(s) differed between source area and backup area.\n");
			fprintf(PatchInstallLog, "\"<\":  values of file(s) in source area - %s\n",
				II_SYSTEM);
			fprintf(PatchInstallLog, "\">\":  values of file(s) in backup area - %s\n\n",
				PatchBackupLoc);
			PrintHeader = TRUE;
		    }

		    sprintf(TmpFile, "%s\\%s", filesizes[i].filepath,
			    filesizes[i].filename);
		    fprintf(PatchInstallLog, "< %-58s  %9d  %8d\n", TmpFile,
			    SourceSize, SourceChecksum);
		    fprintf(PatchInstallLog, "> %-58s  %9d  %8d\n", TmpFile,
			    DestSize, DestChecksum);

		    BadCheck = TRUE;
		}
	    }
	}
    }

    if (!BadCheck)
    {
	/*
	** Update the status of the patch backup in the pre-pXXXX_Y file.
	*/
	sprintf(BackupFile, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
		II_SYSTEM, PATCHNO, sequence);
	sprintf(NewBackupFile,
		"%s\\ingres\\install\\control\\pre-p%s_%i.dat.new",
		II_SYSTEM, PATCHNO, sequence);
	BackupPtr = fopen(BackupFile, "r");
	NewBackupPtr = fopen(NewBackupFile, "w");

	while ( (fgets(mline, MAXSTRING - 1, BackupPtr) != NULL) &&
		(!strstr(mline, "## Status:")) )
	    fprintf(NewBackupPtr, "%s", mline);

	sprintf(mline, "## Status:	valid\n");
	fprintf(NewBackupPtr, "%s", mline);

	while (fgets(mline, MAXSTRING - 1, BackupPtr) != NULL)
	    fprintf(NewBackupPtr, "%s", mline);
	fclose(BackupPtr);
	fclose(NewBackupPtr);
	DeleteFile(BackupFile);
	CopyFile(NewBackupFile, BackupFile, FALSE);
	DeleteFile(NewBackupFile);
    }

    return(BadCheck);
}

/*
**  Name: CheckInstall
**
**  Description:
**	This procedure checks the sizes and creation dates between II_SYSTEM
**	and the cdimage, to make sure that the CA-Installer completed
**	successfully.
**
**  History:
**	26-jan-1999 (somsa01)
**	    Created.
**	27-May-1999 (andbr08)
**	    Changed incorrect formatting of install log where '<' was used twice.
**	15-Mar-2000 (fanch01)
**		Change SourceFile, DestFile, TmpFile, BackupFile, NewBackupFile
**		size to _MAX_PATH.
*/
INT
CheckInstall(INT sequence)
{
    CHAR	SourceFile[_MAX_PATH], DestFile[_MAX_PATH];
    CHAR	TmpFile[_MAX_PATH], mline[MAXSTRING];
    INT		i;
    BOOL	BadCheck = FALSE, PrintHeader = FALSE;
    LONG	SourceSize, DestSize;
    CHAR	BackupFile[_MAX_PATH], NewBackupFile[_MAX_PATH];
    FILE	*BackupPtr, *NewBackupPtr;
    DWORD	SourceChecksum, DestChecksum;

    SetDlgItemText( hDlgGlobal, IDC_TEXT_WAIT,
		    "Verifying files which were installed ...");

    /*
    ** For each file copied, check its size and checksum.
    */
    for (i=0; i < NUMFILES; i++)
    {
	if (filesizes[i].found || filesizes[i].newfile)
	{
	    sprintf(SourceFile, "%s%s\\%s", II_SYSTEM,
		    filesizes[i].filepath, filesizes[i].filename);
	    if (FileExists(SourceFile))
	    {
		sprintf(DestFile, "%s%s\\%s", CDIMAGE_LOC,
			filesizes[i].filepath, filesizes[i].filename);
		SourceSize = GetCurrentFileSize(SourceFile);
		DestSize = GetCurrentFileSize(DestFile);
		
		SourceChecksum = GetCurrentFileChecksum(SourceFile);
		DestChecksum = GetCurrentFileChecksum(DestFile);

		if ((SourceSize != DestSize) ||
		    (SourceChecksum != DestChecksum))
		{
		    if (!PrintHeader)
		    {
			fprintf(PatchInstallLog, "Check of file(s) differed between installation and cdimage.\n");
			fprintf(PatchInstallLog, "\"<\":  values of file(s) in installation - %s\n",
				II_SYSTEM);
			fprintf(PatchInstallLog, "\">\":  values of file(s) in cdimage - %s\n\n",
				CDIMAGE_LOC);
			PrintHeader = TRUE;
		    }

		    sprintf(TmpFile, "%s\\%s", filesizes[i].filepath,
			    filesizes[i].filename);
		    fprintf(PatchInstallLog, "< %-58s  %9d  %8d\n", TmpFile,
			    SourceSize, SourceChecksum);
		    fprintf(PatchInstallLog, "> %-58s  %9d  %8d\n", TmpFile,
			    DestSize, DestChecksum);

		    BadCheck = TRUE;
		}
	    }
	}
    }

    if (!BadCheck)
    {
	/*
	** Update the status of the patch backup in the install-pXXXX_Y file.
	*/
	sprintf(BackupFile, "%s\\ingres\\install\\control\\install-p%s_%i.dat",
		II_SYSTEM, PATCHNO, sequence);
	sprintf(NewBackupFile,
		"%s\\ingres\\install\\control\\install-p%s_%i.dat.new",
		II_SYSTEM, PATCHNO, sequence);
	BackupPtr = fopen(BackupFile, "r");
	NewBackupPtr = fopen(NewBackupFile, "w");

	while ( (fgets(mline, MAXSTRING - 1, BackupPtr) != NULL) &&
		(!strstr(mline, "## Status:")) )
	    fprintf(NewBackupPtr, "%s", mline);

	sprintf(mline, "## Status:	valid\n");
	fprintf(NewBackupPtr, "%s", mline);

	while (fgets(mline, MAXSTRING - 1, BackupPtr) != NULL)
	    fprintf(NewBackupPtr, "%s", mline);
	fclose(BackupPtr);
	fclose(NewBackupPtr);
	DeleteFile(BackupFile);
	CopyFile(NewBackupFile, BackupFile, FALSE);
	DeleteFile(NewBackupFile);
    }

    return(BadCheck);
}

/*
**  Name: PostInstallThread
**
**  Description:
**	This procedure runs in a separate thread. It performs all
**	necessary steps AFTER we successfully execute the CA-Installer.
**
**  History:
**	27-jan-1999 (somsa01)
**	    Created.
**	12-Mar-1999 (andbr08)
**	    Changed to allow redirection of input (from the instaux.bat 
**	    file) to work in windows 95.
**	18-June-1999 (andbr08)
**	    Get rid of II_INSTAUX_EXCODE environment variable when done
**	    with it.
**	15-Mar-2000 (fanch01)
**		Change some char array sizes to reflect actual path lengths.
**		Change dubious use of &char[] in Read/Write redirect loop
**			and eliminated extraneous local var associated with it.
*/
INT
PostInstallThread()
{
	CHAR	PostFile[_MAX_PATH];
	CHAR	cmdline[_MAX_PATH * 2];
    FILE	*PostFilePtr;
    INT		status;
	CHAR	fname[_MAX_PATH];
	HANDLE	hFile;
	DWORD	BytesWritten;
	SECURITY_ATTRIBUTES	sa			= {0};
	STARTUPINFO			si			= {0};
	PROCESS_INFORMATION	pi			= {0};
	HANDLE			hPipeOutputRead		= NULL;
	HANDLE			hPipeOutputWrite	= NULL;
	HANDLE			hPipeInputRead		= NULL;
	HANDLE			hPipeInputWrite		= NULL;
	BOOL			bTest				= 0;
	DWORD			dwNumberOfBytesRead	= 0;

    status = FormatInstallLog();
    if (status)
	return(status);

    /*
    ** If the post-install batch file exists on the cdimage, run it now.
    */
    sprintf(PostFile, "%s\\instaux.bat", CDIMAGE_LOC);
    if (FileExists(PostFile))
    {
	CHAR		mline[MAXSTRING], *cptr;
	INT			excode;

	fprintf(PatchInstallLog,
		"Executing post-install batch file, \"instaux.bat\" ...\n\n");
	SetDlgItemText( hDlgGlobal, IDC_TEXT_WAIT,
			"Executing post-install script ...");

	sprintf(fname, "%s\\ingres\\files\\instaux_p%s.log", II_SYSTEM, PATCHNO);

	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;


	/* Create pipe for standard output and input redirection */
	CreatePipe(&hPipeOutputRead, &hPipeOutputWrite, &sa, 0);
	CreatePipe(&hPipeInputRead, &hPipeInputWrite, &sa, 0);

	/* Make child process use hPipeOutputWrite as standard out, */
	/* and make sure it does not show on the screen. */
	si.cb 		= sizeof(si);
	si.dwFlags	= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow	= SW_HIDE;
	si.hStdInput	= hPipeInputRead;
	si.hStdOutput	= hPipeOutputWrite;
	si.hStdError	= hPipeOutputWrite;

	/* create the command line the second parameter of which passes the */
	/* batch file name */
	sprintf(cmdline, "%s\\redirdos.exe %s\\instaux.bat", CDIMAGE_LOC, CDIMAGE_LOC);

	if (!CreateProcess(NULL, cmdline, NULL, NULL,
			TRUE, 0, NULL, NULL, &si, &pi) )
	{
	    fprintf(PatchInstallLog, "Unable to execute the auxiliary install routine (instaux.bat).\n\n");
	    SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
	    BoxErr5("The patch install did not complete due to an error\n",
		    "executing the auxiliary install routine (instaux.bat).");
	    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
	    return(1);
	}

	/* Now that handles have been inherited, close them to be safe */
	CloseHandle(hPipeOutputWrite);
	CloseHandle(hPipeInputRead);

	/* Open the file for redirection output, appending data to file if exists */
	hFile = CreateFile( fname, GENERIC_WRITE, 0, NULL,
			OPEN_ALWAYS, 0 ,0 );
	SetFilePointer( hFile, 0, NULL, FILE_END);
	
	/* capture DOS application output by reading hPipeOutputRead */
	while(TRUE)
	{
		bTest = ReadFile(hPipeOutputRead, mline, MAXSTRING, &dwNumberOfBytesRead, NULL);
	
		if(!bTest)
		{
			/* End of output from instaux.bat file so quit */
			break;
		}
		if(!dwNumberOfBytesRead)
		{
			/* No information received, must be end of output */
			break;
		}

		/* write out results of instaux to file */
		WriteFile(hFile, mline, dwNumberOfBytesRead, &BytesWritten, NULL);

	}

	/* Wait for CONSPAWN.exe to finish */
	WaitForSingleObject(pi.hProcess, INFINITE);

	/* Close all remaining handles */
	CloseHandle(pi.hProcess);
	CloseHandle(hPipeOutputRead);
	CloseHandle(hPipeInputWrite);
	CloseHandle(hFile);

	/*
	** Since this is a batch file, we will retrieve its "exit code" by
	** the Ingres environment variable that it set - II_INSTAUX_EXCODE
	*/
	sprintf (PostFile, "%s\\ingres\\files\\symbol.tbl", II_SYSTEM);
	PostFilePtr = fopen(PostFile, "r");
	while ( (fgets(mline, MAXSTRING - 1, PostFilePtr) != NULL) &&
		(!strstr(mline, "II_INSTAUX_EXCODE")) );
	fclose(PostFilePtr);
	cptr = strstr(mline, "	");
	cptr++;
	excode = atoi(cptr);

	/* Get rid of environment variable as we got return code */
	sprintf(cmdline, "%s\\ingres\\bin\\ingunset.exe II_INSTAUX_EXCODE", II_SYSTEM);
   	CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL,
		  &si, &pi);
    	WaitForSingleObject(pi.hProcess, INFINITE);
    	
	if (excode)
	{
	    fprintf(PatchInstallLog,
		    "Auxiliary install routine (instaux.bat) failed during the post-install phase.\n\n");
	    SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
	    BoxErr5("The patch install did not complete due to a failure from",
		    "\nthe auxiliary install routine (instaux.bat).");
	    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
	    return(1);
	}
	else
	{
	    fprintf(PatchInstallLog,
		  "The post-install batch file completed successfully.\n\n");
	}
    }

    Sleep(2000);

    /* Close out the Wait window we started in the main program. */
    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
    return(0);
}

/*
**  Name: AskBox
**
**  Description:
**	Quick little function for printing messages to the screen, with
**	the exclamation sign. Takes two strings as it's input.
**
**	Output: yes or no
**
**  History:
**		09-jun-2000 (kitch01)
**		   Created.
*/
int
AskBox (CHAR *MessageString, CHAR *MessageString2, BOOL Warn)
{
    CHAR	MessageOut[1024];
	CHAR	title[512];
	int		style;

    sprintf(MessageOut,"%s\n%s",MessageString, MessageString2);
#ifdef IngresII
	strcpy(title, "Ingres Patch Installer");
#else
	strcpy(title, "OpenIngres Patch Installer");
#endif
		
	style = MB_APPLMODAL | MB_YESNO;

	if (Warn)
		style = style | MB_ICONWARNING | MB_DEFBUTTON1;
	else
		style = style | MB_ICONQUESTION | MB_DEFBUTTON2;

    return MessageBox( hDlgGlobal, MessageOut, title, style);
}

/*
**  Name: CheckIngVersion
**
**  Description:
**	This procedure checks to make sure the installation version matches
**	the version that this patch is made for.
**
**  History:
**	05-feb-1999 (somsa01)
**	    Created.
**	15-Mar-2000 (fanch01)
**		Change VersLocation size to _MAX_PATH.
**		Change MessageOutput, MessageOutput2 size to MAXSTRING.
**	09-jun-2000 (kitch01)
**		Substantially updated to change what gets checked.
**	25-jun-2002 (somsa01)
**	    Corrected setting of current version printout.
**	17-apr-2003 (penga03)
**	    Do not pop up the warning message if only the build number 
**	    of the Ingres version changed.
*/
INT
CheckIngVersion()
{
    CHAR	VersLocation[_MAX_PATH];
    CHAR	mline[MAXSTRING];
    CHAR	MessageOutput[MAXSTRING], MessageOutput2[MAXSTRING];
	CHAR	temploc	[_MAX_PATH];
	int		version, i, j, pVer, veroffset = 0;
	CHAR	*verstart;
	CHAR	save_ver[MAXSTRING];
	CHAR	actversion[MAXSTRING];
	CHAR	orig_mline[MAXSTRING];
	CHAR	reltime[20];

	pVer = i = j = 0;
	version = -1;
	/* First find the current version in our table */
	do
	{
		if (!_stricmp(PatchVersion,IngVers[pVer].Vers))
			break;
		pVer++;
	} while (*IngVers[pVer].Vers != '\0');

	/* If we cannot locate the current version it is a serious error
	** or perhaps we have a new version that has not been added to the
	** table yet
	*/
	if (*IngVers[pVer].Vers == '\0')
	{
	   sprintf(MessageOutput, "Internal error checking Ingres Version");
	   sprintf(MessageOutput2, "\nPlease contact CA Technical Support");
	   BoxErr4(MessageOutput, MessageOutput2);
	   return(0);
	}

	/* Locate the config.dat file */
    sprintf(VersLocation, "%s\\ingres\\files\\config.dat", II_SYSTEM);
    if (FileExists(VersLocation))
    {
		/* For 2.0/0001 hack the version in the bacse release if needed */

		if (pVer == ep0001)
			HackIngVersion(II0001_BASE_TO_PATCH, 0);

		/* Take a copy of it to read through */
		sprintf(temploc, "%s\\ingres\\files\\config.tmp", II_SYSTEM);
		if (CopyFile(VersLocation, temploc, FALSE))
		{
			FILE	*fptr;

			/*
			** Get a line from config.tmp
			*/
			fptr = fopen(temploc, "r");
			fgets(mline, MAXSTRING - 1, fptr);
			mline[strlen(mline)-1] = '\0';
			do
			{
				/* Save the original line */
				strcpy(orig_mline,mline);
				/* Uppercase the line */
				_strupr(mline);
				/* Line(s) containing the version installed contain config and complete */
				if ( (strstr(mline, "CONFIG") != NULL) &&
					 (strstr(mline, "COMPLETE") != NULL) )
				{
					/* Does this line contain the right version? */
					if (strstr(orig_mline, PatchVersion))
					{
						if(version < pVer)
							version = pVer;
					}
					else
					{
						/* This is the wrong version but which? */
						if ((verstart = strstr(orig_mline,".oping")) != NULL)
							verstart = verstart+6;
						else
						{
							if ((verstart = strstr(orig_mline,".ii")) == NULL)
								continue;
							verstart = verstart+3;
						}

						verstart[14] = '\0';
						i = 0;
						/* Check for later versions */
						while (*IngVers[i].Vers != '\0')
						{
							if ((strstr(verstart,IngVers[i].Vers)) != NULL)
							{
								/* We've know the version from the config line
								** but is it the latest found?
								*/
								if(version < i)
								{
									veroffset = verstart - &orig_mline[0];
									version = i;
									strcpy(save_ver,orig_mline);
								}
								break;
							}
							i++;
						}
					}
				}
				fgets(mline, MAXSTRING - 1, fptr);
			} while (!feof(fptr));
			fclose(fptr);
			DeleteFile(temploc);
		}
    }
    else
    {
	/*
	** The version cannot be checked. There is no config.dat !!
	*/
	   sprintf(MessageOutput, "Unable to locate %s to complete version checking", VersLocation);
	   sprintf(MessageOutput2, "\nPlease check your II_SYSTEM setting");
	   BoxErr4(MessageOutput, MessageOutput2);
	   return(0);
    }

	/* What version did we find */

	/* Unable to determine installer version */
	if(version == -1)
	{
		sprintf(MessageOutput, "The patch installer was unable to determine your current Ingres version.");	
		sprintf(MessageOutput2, "\nPlease contact CA Technical Support");
		BoxErr4(MessageOutput, MessageOutput2);
		return 0;
	}
	else if(pVer == version)
	{
		return 1;
    }
	else 
    {
		if (!strncmp(IngVers[pVer].Vers, IngVers[version].Vers, 12))
		    return 1;

		if(version == 0)
			actversion[0] = 'O';
		else if (version >= 1)
			actversion[0] = 'I';

		verstart = &save_ver[0] + veroffset;

		actversion[1] = 'I';
		actversion[2] = ' ';
				
		verstart[14] = '\0';

		j=3;
		for (i=0; i<strlen(verstart); i++)
		{
			if (i==1 || i==9)
				actversion[j++] = '.';
			else if (i==2 || i==12)
				actversion[j++] = '/';
			else if (i==6)
			{
				actversion[j++] = ' ';
				actversion[j++] = '(';
			}
			actversion[j++] = verstart[i];
		}
		actversion[j++] = ')';
		actversion[j] = '\0';

		sprintf(MessageOutput2, "Your apparent Ingres installation version is %s .",actversion);
		sprintf(MessageOutput, "This patch is for %s .\n\n%s", IngVersion, MessageOutput2);
		if(version > pVer)
			sprintf(reltime, " later");
		else
			sprintf(reltime, "n earlier");
		sprintf(MessageOutput, "%s\nwhich is a%s release than this patch is intended for.", MessageOutput,reltime);

		/* Do not allow 2.0/0001 patch to be installed ober 2.0/9712 */
		if((pVer == ep0001) && (version == 0))
		{
			sprintf(MessageOutput2, "\n\nThis patch can not be installed on your current installation.");
			BoxErr4(MessageOutput, MessageOutput2);
			return 0;
		}

        sprintf(MessageOutput2, "\nDo you wish to continue?");

		/* Allow SFP upgrade to 2.0/0001 from 2.0/9808 and 2.0/9905 */
		if((pVer == ep0001) && ((version == 1) || (version == 2)))
		{
			sprintf(MessageOutput, "%s\n\nInstallation of this patch will upgrade your", MessageOutput);
			sprintf(MessageOutput, "%s\nversion to %s.", MessageOutput, IngVersion);
		Upgradeto0001 = version;
		}
		/* Allow SFP upgrade to 2.6/0305 from 2.6/0207 and 2.6/0201 */
		if((pVer == ep26NEW) && ((version >= 5) && (version <= 12)))
		{
			sprintf(MessageOutput, "%s\n\nInstallation of this patch will upgrade your", MessageOutput);
			sprintf(MessageOutput, "%s\nversion to %s.", MessageOutput, IngVersion);
			Upgradeto26NEW = version;
		}

		/* Ask the user if they wish to continue. */
		if (AskBox(MessageOutput, MessageOutput2, FALSE) == IDYES)
		{
			/* They want to continue so advise them to backup first */
			sprintf(MessageOutput, "Having chosen to continue it is STRONGLY recommended that you check the Backup option");
			BoxErr1(MessageOutput, "");
			return 1;
		}
		else
			return 0;
	
	}

}

/*
**  Name: GetCurrentFileChecksum()
**
**  Description:
**	This method returns the checksum of a file passed into the method as
**	a character pointer.  The checksum algorithm is the one used in the UNIX
**	ingbuild executable and is defined in ip_file_info_comp() in 
**	front!st!install ipfile.sc
**
**  History:
**	24-Mar-1999 (andbr08)
**	    Created.
**	15-Mar-2000 (fanch01)
**		Change dataRead size to _MAX_PATH.
*/
DWORD GetCurrentFileChecksum(CHAR* fname)
{
	HANDLE		hFile;
    CHAR		dataRead[1024];
    CHAR		*dataptr;
	DWORD		CheckSum;
    int			numBytesRead;
    int			readDataFlag;
	DWORD		ByteCount;

	hFile = CreateFile( fname, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, 0 ,0 );
	if (hFile == NULL) {
		return 0;
	}

	ByteCount = 0;
	CheckSum = 0;
    while (1) {
		readDataFlag = ReadFile(hFile, &dataRead, 1024, &numBytesRead, NULL); 
		if (numBytesRead == 0) {
			break;
		}
		else if (readDataFlag == 0) {
			break;
		}
		else {
			dataptr = dataRead;
			while (numBytesRead > 0) {
				--numBytesRead;
				ByteCount++;
				CheckSum ^= (*dataptr) << ( ByteCount & 3 );
				++dataptr;
			}
		}
    }
	
    CloseHandle(hFile);

    return CheckSum;
}

/*
**  Name: CheckUtilityDllsPresent()
**
**  Description:
**		This method will check the II_SYSTEM utility directory to see
**		if there are any DLLs present. If there are then it will ask the
**		user if they want the installer to delete them. The installer will
**		then try to delete them. If the delete fails or the user does not
**		want us to delete them we advise them to delete them manually and
**		refer them to the patch notes.
**
**  History:
**		09-jun-2000 (kitch01)
**		   Created.
*/
BOOL CheckUtilityDllsPresent()
{
    WIN32_FIND_DATA FindFileData;
	CHAR	utildll[_MAX_PATH];
	CHAR	delfile[_MAX_PATH];
	HANDLE	hFile;
	BOOL	fDone = FALSE;
    CHAR	MessageOutput[MAXSTRING], MessageOutput2[MAXSTRING];
	BOOL	removed = TRUE;

	sprintf(utildll, "%s\\ingres\\utility\\*.dll", II_SYSTEM);

	/* Are there any DLLs present in this directory?*/
	hFile=FindFirstFile(utildll,&FindFileData);
	if (hFile==INVALID_HANDLE_VALUE)
		return FALSE;
	FindClose(hFile);

	sprintf(MessageOutput, "The %s\\ingres\\utility directory \
contains obsolete Ingres system DLLs.\n\nBefore installing this patch these DLLs must be removed.",II_SYSTEM);
	sprintf(MessageOutput2, "\nWould you like the installer to do this now?\n");

	/* Ask the user if we are to delete them */
	if (AskBox(MessageOutput, MessageOutput2, TRUE) == IDYES)
	{
		hFile = FindFirstFile( utildll,&FindFileData );
		/* This should not happen but just in case the user went
		** and deleted them we cater for this. The installer will 
		** still exit however.
		*/
		if( hFile == INVALID_HANDLE_VALUE )
		{
			sprintf(MessageOutput, "Unable to locate obsolete Ingres system DLLs\n\n");
			removed = FALSE;
		}
		else
		{
			/*
			** Walk all the dll files in the directory.
			*/
			while( !fDone )
			{
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
				{
					sprintf(MessageOutput,"One or more of the utility DLLs are marked read only\n\n");
					removed = FALSE;
					break;
				}

				sprintf(delfile, "%s\\ingres\\utility\\%s", II_SYSTEM, FindFileData.cFileName);

				if (!DeleteFile(delfile))
				{
					sprintf(MessageOutput,"Unable to delete %s \n\n", delfile);
					removed = FALSE;
					break;
				}

				fDone = !FindNextFile( hFile, &FindFileData );
			}

			FindClose(hFile);
		}

		if (removed)
		{
			/* Sanity check */
			hFile=FindFirstFile(utildll,&FindFileData);
			if (hFile!=INVALID_HANDLE_VALUE)
			{
				/* How can this happen? Well just cope if it ever does */
				sprintf(MessageOutput, "One or more utility DLLs were still present after deletion\n\n");
				removed = FALSE;
				FindClose(hFile);
			}
			else
			/* All OK so return false and the installer will carry on */
			   return FALSE;
		}
	}
	else
		sprintf(MessageOutput, "");

	sprintf(MessageOutput2, "Obsolete system DLLs are still present in %s\\ingres\\utility \
\nManual deletion of these DLLs is required before installation of this patch\nPlease refer to \
the patch notes for guidence", II_SYSTEM);

	BoxErr4(MessageOutput, MessageOutput2);
	
	return TRUE;

}

/*
**  Name: HackIngVersion
**
**  Description:
**	The ii200001wntw3200 MR has incorrect version strings in the
**  config.dat. This procedure will rectify the values in the
**  config.dat if this is a 2.0/0001 (wnt.w32/00) patch.
**
**  History:
**	18-Aug-2000 (hanal04)
**	    Created.
**      06-Oct-2000 (hanal04) Bug 102490
**          Add Embbed install checks and updates if we are upgrading via
**          a 0001 patch.
*/
VOID
HackIngVersion(int hack_type, int oldVers)
{
    CHAR	VersBakLocation[_MAX_PATH];
    CHAR	ConfigBakLocation[_MAX_PATH];
	CHAR	VersLocation[_MAX_PATH];
	CHAR	RelLocation[_MAX_PATH];
    CHAR	mline[MAXSTRING];
	CHAR	temploc	[_MAX_PATH];
	int		version, i, j, pVer;
	CHAR	*verstart;
	CHAR	orig_mline[MAXSTRING];
	CHAR	tng_line[MAXSTRING];
	FILE	*fptr, *tmp_fptr;
        BOOL	tng_install = FALSE;
        BOOL    tng_fields = FALSE;

	version = i = j = 0;

	pVer = ep26NEW; /* hack_type is II26OLD_TO_II26NEW)

    /* If there's a version.rel check to see if we are an embdded instal */
    if(hack_type & IIOLD_TO_II0001)
    {
        sprintf(RelLocation, "%s\\ingres\\version.rel", II_SYSTEM);
        /*
        ** Get a line from config.dat
        */
        fptr = fopen(RelLocation, "r");
        if(fptr != NULL)
        {
            fgets(mline, MAXSTRING - 1, fptr);
    
            do
            {
    
                /* Uppercase the line */
                _strupr(mline);    
	        if (strstr(mline, "TNG") != NULL)
                {
                    tng_install = TRUE;
                    break;
                }
	        fgets(mline, MAXSTRING - 1, fptr);
	    } while (!feof(fptr));
	    fclose(fptr);
        }
    }


	/*
	** Define config.dat file and copy.
	*/
    sprintf(VersLocation, "%s\\ingres\\files\\config.dat", II_SYSTEM);
    sprintf(VersBakLocation, "%s\\ingres\\files\\config.bak", II_SYSTEM);
	sprintf(temploc, "%s\\ingres\\files\\config.tmp", II_SYSTEM);
	sprintf(ConfigBakLocation, "%s\\ingres\\files\\config.dat", PatchBackupLoc); 

	CopyFile(VersLocation, VersBakLocation, FALSE);

	/*
	** Get a line from config.dat
	*/
	fptr = fopen(VersLocation, "r");
	tmp_fptr = fopen(temploc, "w");
	fgets(mline, MAXSTRING - 1, fptr);
	
	do
	{
		/* Save the original line */
		strcpy(orig_mline,mline);
		/* Uppercase the line */
		_strupr(mline);
		/* Line(s) containing the version installed contain config and complete */
		if ( (strstr(mline, "CONFIG") != NULL) &&
			 (strstr(mline, "COMPLETE") != NULL) )
		{
			if(hack_type & II0001_BASE_TO_PATCH)
			{
				/* Check for hackable version */
				while (*Ing0001hack[i].Vers != '\0')
				{
					if ((verstart = strstr(orig_mline, Ing0001hack[i].Vers)) != NULL)
					{
						/* We have to hack this line.
						*/
						switch (i)
						{
							case 0:
							case 2:
								verstart+=7;
								break;
							case 1: 
								verstart+=6;
								break;
						}
						strncpy(verstart, (char *)&IngVers[ep0001], 14);
						break;
					}
					i++;
				}
				i = 0;
			}
		 	else if(hack_type & IIOLD_TO_II0001)
			{
				if ((verstart = strstr(orig_mline, IngVers[oldVers].Vers)) != NULL)
				{
					/* Write the original line back */
					fputs(orig_mline, tmp_fptr);
					/* We have to duplicate this line for 2.0/0001.
					*/
					strncpy(verstart, (char *)&IngVers[ep0001], 14);
		                        /* Save the original line, we may use
                                        ** it to generate tng entries
                                        */
		                        strcpy(tng_line,orig_mline);
				}
			}
			  else
		 	  if(hack_type & II26OLD_TO_II26NEW)
			  {
				if ((verstart = strstr(orig_mline, IngVers[oldVers].Vers)) != NULL)
				{
					/* Write the original line back */
					fputs(orig_mline, tmp_fptr);
					/* We have to duplicate this line for 2.0/0001.
					*/
					strncpy(verstart, (char *)&IngVers[ep26NEW], 14);
		                        /* Save the original line, we may use
                                        ** it to generate tng entries
                                        */
		                        strcpy(tng_line,orig_mline);
				}
			  }
		}
                else
                {
                    /* If we are upgrading to 0001 via a patch we need to
                    ** Check for Embedded Release and possibly add
                    ** embed_installation and embed_user
                    */
                    if(hack_type & IIOLD_TO_II0001)
                    {
		        if ((verstart = strstr(mline, "TNG_INSTALLATION")) 
						!= NULL)
                        {  
                            verstart+=16;
                            if ((verstart = strstr(verstart, "ON")) != NULL)
                            { 
                                tng_install = TRUE; 
                            }
                        }
		        if (strstr(mline, "EMBED_INSTALLATION") != NULL)
                        {  
                            tng_fields = TRUE; 
                        }
                    }
                }
		fputs(orig_mline, tmp_fptr);
		fgets(mline, MAXSTRING - 1, fptr);
	} while (!feof(fptr));

        if((hack_type & IIOLD_TO_II0001) && (tng_install == TRUE) && 
		(tng_fields == FALSE))
        {
            if ((verstart = strstr(tng_line, "config")) != NULL)
            {
                strcpy(verstart, "setup.embed_installation:\tON\n");
                fputs(tng_line, tmp_fptr);
                if ((verstart = strstr(tng_line, "_installation")) != NULL)
                {
                    strcpy(verstart, "_user:\t\tON\n");
                    fputs(tng_line, tmp_fptr);
                }
            }
        }

	fclose(fptr);
	fclose(tmp_fptr);
	if(hack_type & BACKUP_CONFIG)
		CopyFile(VersLocation, ConfigBakLocation, FALSE);
	DeleteFile(VersLocation);
	CopyFile(temploc, VersLocation, FALSE);
	DeleteFile(temploc);
		
	return;

}
