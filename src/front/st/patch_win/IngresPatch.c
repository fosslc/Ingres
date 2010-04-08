/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: IngresPatch.c: Compiles into dll that contains custom
**						 actions for PatchInstaller to use during
**						 installation.
**
**  History:
**		01-Dec-2005 (drivi01)
**			Created.
**		10-Feb-2006 (drivi01)
**			Updated function update_vers to reset the timestamp on version.rel
**			file after it's been updated with patch number to avoid problems
**			during upgrade to later versions.  Modified routines for updating
**			version.rel and detecting if patch number already exists, added
**			function ReadSingleLine.
**		22-Feb-2006 (drivi01)
**			BUG 115780 
**			Modified routines that detect files that have the same timestamps
**			to be intelligent enough to determine if the file is the same via
**			checksum and timestamps.
**		03-Mar-2006 (drivi01)
**			Due to the fact that patches may contain files for features that
**			weren't installed we need to check if in those cases destination
**			directory even exists.
**		06-Mar-2006 (drivi01)
**			Modified customer action backup_files to retrieve patch image
**			directory from Property instead of Directory table.
**		20-Mar-2006 (drivi01)
**			BUG 115858
**			Added retry when patch installer fails to copy files due to shared violation.
**		03-Apr-2006
**			If patch id in version.rel on the installed image doesn't have
**			new line and/or carriage return at the end of the file, then
**			update_vers custom action appends new patch # to the end of
**			the line with previous patch #.  This change is to detect
**			last character in the file and append carriage return before
**			new patch id.
**		12-Apr-2006 (drivi01)
**			Reworked routine that retrieve directory path from a file path.
**			Added a rollback algorithm in case if something goes wrong during
**			patch installation.
**			Updated the information contained in patch.log.
**		25-Apr-2006 (drivi01)
**			Porting patch installer to Ingres 2006.
**		01-May-2006 (drivi01)
**			Patch Installer fails to update files that are hidden due to Access
**			violation, ingres should not have hidden files, therefore hidden 
**			attribute will be removed from a file.	
**		02-May-2006 (drivi01)
**			When getting a file size of patch files or installed files, Open a 
**			file with FILE_SHARE_READ flag.
**		04-May-2006 (drivi01)
**			Added custom action to delete all files that were backed up.
**			Fix routines that create directories.
**		16-Jun-2006 (drivi01)
**			Fixed delete_backups custom action to be able to find Ingres 2006 
**			in the registry.  Added routines for Patch Installer to recognize
**			installs with patch id 00000 as SOL distribution.
**		29-Jun-2006 (drivi01)
**			Disabled the prompt when Patch Installer detects older files in the
**			patch its installing than files in the installation it's patching.
**		20-may-2009 (smeke01) b121784
**			Don't upgrade a .crs file if it is not installed already.
**		15-Jul-2009 (wanfr01) b122236
**			SetBackupDir call in UserInterface is not called if
**			this is a silent install.  Do an Explicit call to insure BACKUPDIR
**			has a legit default value.
**		07-Aug-2009 (drivi01)
**			Add pragma to remove warning 4996 about deprecated POSIX functions
**			b/c it's a bug.
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <MsiQuery.h>
#include <clusapi.h>
#include <resapi.h>
#include "patchcl.h"
#include <errno.h>

/* Forward Definitions */
BOOL AskUserYN(MSIHANDLE hInstall, char *lpText);
BOOL ExecuteEx(char *lpCmdLine);
int  Execute(char *lpCmdLine, BOOL bwait);
int  IngresAlreadyRunning(char *installpath);
int  IsUpdatableFile(FILETIME, FILETIME);
int  Local_NMgtIngAt(char *strKey, char *strRetValue);
BOOL perform_backup_and_copy(char *file1, char *patchFile, char *patchid);
char * ReadSingleLine(int, char *);
BOOL rollback_patch(MSIHANDLE handle);
BOOL TellUser(MSIHANDLE hInstall, char *lpText);


struct WeekDay
{
    CHAR        Day[4];
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
    CHAR        Month[4];
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





/* variables for backups & restoring backups */
# define MAXSTRING 1024
#define  STATUSITEM  31			/* handle for STATUSITEM */
char ii_system[MAX_PATH+1];		/* II_SYSTEM for target installation */
CHAR    PatchBackupLoc  [_MAX_PATH];    /* patch backup location, versioned */
HWND    hDlgGlobal;  /* Handle to Main Dialog */
char	patchid[PATCH_SIZE]; 		/* patch number from registry */
char    	manifestfile[MAX_PATH+1];


HWND	hWnd = NULL;
char	buffer[BUFF_SIZE];

/*
**  Name: FileExists
**
**  Description:
**      Determines whether the given file/path exists... well, sort of.
*/
BOOL
FileExists(CHAR *filename)
{
    if (GetFileAttributes(filename) != -1)
        return TRUE;

    return FALSE;
}


/*
**  Name: BoxErr1
**
**  Description:
**      Quick little function for printing messages to the screen.
**      Takes two strings as it's input.
*/
VOID
BoxErr1 (CHAR *MessageString, CHAR *MessageString2)
{
    CHAR        MessageOut[1024];

    sprintf(MessageOut,"%s%s",MessageString, MessageString2);
#ifdef IngresII
    MessageBox( hWnd, MessageOut, "Ingres Patch Installer",
#else
    MessageBox( hWnd, MessageOut, "OpenIngres Patch Installer",
#endif
                MB_APPLMODAL | MB_ICONINFORMATION | MB_OK);
}

/*
**  Name: BoxErr2
**
**  Description:
**      Same as BoxErr, but accepts a string and a LONG.
*/
VOID
BoxErr2 (CHAR *MessageString, LONG MessageNum)
{
    CHAR        MessageOut[1024];

    sprintf(MessageOut,"%s\n%d",MessageString, MessageNum);
#ifdef IngresII
    MessageBox( hWnd, MessageOut, "Ingres Patch Installer",
#else
    MessageBox( hWnd, MessageOut, "OpenIngres Patch Installer",
#endif
                MB_APPLMODAL | MB_ICONINFORMATION | MB_OK);
}



/*
**  Name: BoxErr3
**
**  Description:
**      Prints message plus english description of dwErrCode.
*/
VOID
BoxErr3(CHAR *MessageString, DWORD dwErrCode)
{
    LPVOID      lpMessageBuffer;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMessageBuffer, 0, NULL);

    strcat(MessageString, "\n");
    BoxErr1(MessageString, lpMessageBuffer);

    /* Free the buffer allocated by the system */
    LocalFree( lpMessageBuffer );
}


/*
**  Name: CheckMakeDir
**
**  Description:
**      Checks for the existence of a directory, creates it if missing
**      and reports if it is unable to do so.
**
**      15-Mar-2000 (fanch01)
**              Change workbuf size to _MAX_PATH.
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
**      Checks for the existence of a directory structure, creates it if
**      missing and reports if it is unable to do so.
**
**      15-Mar-2000 (fanch01)
**              Change workbuf, temp size to _MAX_PATH.
*/
BOOL
CheckMakeDirRec(CHAR *cDirName, BOOL PermanentDir)
{
    CHAR        workbuf[_MAX_PATH], temp[_MAX_PATH], *ptr1, *ptr2;
    BOOL        FoundLastDir = FALSE;
    INT         i, j, numlevels, len;

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
**  Name: BoxErr4
**
**  Description:
**      Quick little function for printing messages to the screen, with
**      the stop sign. Takes two strings as it's input.
*/
VOID
BoxErr4 (CHAR *MessageString, CHAR *MessageString2)
{
    CHAR        MessageOut[1024];

    sprintf(MessageOut,"%s%s",MessageString, MessageString2);
#ifdef IngresII
    MessageBox( hWnd, MessageOut, "Ingres Patch Installer",
#else
    MessageBox( hWnd, MessageOut, "OpenIngres Patch Installer",
#endif
                MB_APPLMODAL | MB_ICONSTOP | MB_OK);
}


/*
**  Name: GetTimeOfDay
**
**  Description:
**      This procedure gets the current time of day and returns it as an
**      ASCII string.
**
**  History:
**      24-jan-1999 (somsa01)
**          Created.
*/
VOID
GetTimeOfDay(BOOL timestamp, CHAR *TimeString)
{
    SYSTEMTIME  LocalTime;

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
**  Name: GetCurrentFileChecksum()
**
**  Description:
**      This method returns the checksum of a file passed into the method as
**      a character pointer.  The checksum algorithm is the one used in the UNIX
**      ingbuild executable and is defined in ip_file_info_comp() in
**      front!st!install ipfile.sc
**
**  History:
**      24-Mar-1999 (andbr08)
**          Created.
**      15-Mar-2000 (fanch01)
**              Change dataRead size to _MAX_PATH.
*/
DWORD GetCurrentFileChecksum(CHAR* fname)
{
        HANDLE          hFile;
    CHAR                dataRead[1024];
    CHAR                *dataptr;
        DWORD           CheckSum;
    int                 numBytesRead;
    int                 readDataFlag;
        DWORD           ByteCount;

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
**  Name: GetCurrentFileSize
**
**  Description:
**      Retrieves the current size of the given file name.
**
**  History:
**      24-jan-1999 (somsa01)
**          Created.
*/
LONG
GetCurrentFileSize (CHAR *fname)
{
    HANDLE      hFile;
    DWORD       size;

    hFile = CreateFile( fname, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, 0 ,0 );

    size = GetFileSize(hFile, NULL);
    CloseHandle(hFile);

    return (size);
}


/*
**  Name: CheckBackup
**
**  Description:
**      This procedure checks the sizes and creation dates between II_SYSTEM
**      and the backup, to make sure that backup was successful.
**
**  History:
**      26-jan-1999 (somsa01)
**          Created.
**      27-May-1999 (andbr08)
**          Changed incorrect formatting of install log where '<' was used twice.
**      15-Mar-2000 (fanch01)
**              Change SourceFile, DestFile, TmpFile, BackupFile, NewBackupFile size
**              to _MAX_PATH.
*/
INT
CheckBackup(INT sequence, char *CurCaption)
{
    CHAR        SourceFile[_MAX_PATH], DestFile[_MAX_PATH];
    CHAR        mline[MAXSTRING];
    BOOL        BadCheck = FALSE, PrintHeader = FALSE;
    LONG        SourceSize, DestSize;
    CHAR        BackupFile[_MAX_PATH], NewBackupFile[_MAX_PATH];
    FILE        *BackupPtr, *NewBackupPtr;
    DWORD       SourceChecksum, DestChecksum;
    char        logBuf[BUFF_SIZE];
    BOOL tempbool;
HWND    hText;
    FILE *ManifestPtr;
    CHAR tempfile[MAX_PATH+1];
    char *charp;

    /* Update log */
    sprintf(logBuf, "CheckBackup: ii_system=%s\r\n", ii_system);
    AppendToLog(logBuf);

        hWnd=FindWindow(NULL, CurCaption);

/**** using base  */

    hText = GetDlgItem(hWnd,STATUSITEM);
    EnableWindow(hText,TRUE);
    tempbool = SetWindowText(hText,
                    "Verifying files which were backed up ...");
    SendDlgItemMessage(hWnd, STATUSITEM, WM_ENABLE, 0,0);
    SendDlgItemMessage(hWnd, STATUSITEM, WM_PAINT,  0,0);


    /*
    ** For each file copied, check its size and checksum.
    */

    ManifestPtr = fopen(manifestfile, "r");
    if (!ManifestPtr)
    {
	sprintf(logBuf, "Error reading %s\r\n",manifestfile);
	BoxErr4(logBuf, "");
	return (ERROR_INSTALL_FAILURE);
    }

    while (fscanf (ManifestPtr,"%s",&tempfile) != EOF)
    {
	    /* Manifest files have the wrong slashes - convert */
	    for (charp = tempfile;*charp != '\0';charp++)
	        if (*charp == '/') *charp='\\';

	    sprintf(SourceFile, "%s\\ingres\\%s", ii_system, tempfile);

            if (FileExists(SourceFile))
            {
                sprintf(DestFile, "%s\\%s", PatchBackupLoc, tempfile);
                SourceSize = GetCurrentFileSize(SourceFile);
                DestSize = GetCurrentFileSize(DestFile);

                SourceChecksum = GetCurrentFileChecksum(SourceFile);
                DestChecksum = GetCurrentFileChecksum(DestFile);

                if ((SourceSize != DestSize) ||
                    (SourceChecksum != DestChecksum))
                {
                    if (!PrintHeader)
                    {
                        sprintf(logBuf, "Check of file(s) differed between source area and backup area.\n");
                        AppendToLog(logBuf);
                        sprintf(logBuf, "\"<\":  values of file(s) in source area - %s\n",
                                ii_system);
                        AppendToLog(logBuf);
                        sprintf(logBuf, "\">\":  values of file(s) in backup area - %s\n\n",
                                PatchBackupLoc);
                        AppendToLog(logBuf);
                        PrintHeader = TRUE;
                    }

                    sprintf(logBuf, "< %-58s  %9d  %8d\n", tempfile,
                            SourceSize, SourceChecksum);
                    AppendToLog(logBuf);
                    sprintf(logBuf, "> %-58s  %9d  %8d\n", tempfile,
                            DestSize, DestChecksum);
                    AppendToLog(logBuf);

                    BadCheck = TRUE;
                }
            }
    }
    fclose(ManifestPtr);

    if (!BadCheck)
    {
        /*
        ** Update the status of the patch backup in the pre-pXXXX_Y file.
        */
        sprintf(BackupFile, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
                ii_system, patchid, sequence);
        sprintf(NewBackupFile,
                "%s\\ingres\\install\\control\\pre-p%s_%i.dat.new",
                ii_system, patchid, sequence);
        BackupPtr = fopen(BackupFile, "r");
        NewBackupPtr = fopen(NewBackupFile, "w");

        while ( (fgets(mline, MAXSTRING - 1, BackupPtr) != NULL) &&
                (!strstr(mline, "## Status:")) )
            fprintf(NewBackupPtr, "%s", mline);

        sprintf(mline, "## Status:      valid\n");
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
**  Name: CalcTotals
**
**  Description:
**      Calculates the space needed for the files selected.  Uses the
**      "filesizes" array created by "space".
**
**      15-Mar-2000 (fanch01)
**              Change DestFile size to _MAX_PATH.
*/
FLOAT
CalcTotals(MSIHANDLE handle)
{
    FLOAT       fProductsTotal = (FLOAT) 0.0;
    CHAR	SourceFile[_MAX_PATH];
    LONG	SourceSize;
    char        logBuf[BUFF_SIZE];
    char    	patchfile[MAX_PATH+1];
    FILE	*ManifestPtr;
    char	*charp;

    sprintf(logBuf, "ii_system = %s\r\n", ii_system);
    AppendToLog(logBuf);

    ManifestPtr = fopen(manifestfile, "r");
    if (!ManifestPtr)
    {
        sprintf(logBuf, "Error reading %s\r\n",manifestfile);
        BoxErr4(logBuf, "");
	return (-1);
    }

    while (fscanf (ManifestPtr,"%s",&patchfile) != EOF) 
    {
        sprintf(SourceFile, "%s\\ingres\\%s", ii_system, patchfile);

	/* Manifest files have the wrong slashes - convert */
	for (charp = SourceFile;*charp != '\0';charp++)
	{
	    if (*charp == '/') *charp='\\';
	}

        if (FileExists(SourceFile))
        {
            SourceSize = GetCurrentFileSize(SourceFile);
            fProductsTotal += SourceSize;
        }
    } 
    fclose(ManifestPtr);

    return fProductsTotal;
}


/*
**  Name: SetBackupDir
**
**  Description:
**	Set MSI Backup directory properties to appropriate values.
**
*/

BOOL
SetBackupDir(MSIHANDLE handle)
{
    char        KeyName[MAX_PATH+1];
    CHAR        BackupDir[_MAX_PATH];
    char        iicode[II_SIZE];
    DWORD       dwSize;
    HKEY        hKey;
    char        logBuf[BUFF_SIZE];

    /* retrieves installation id from msi */
    memset(&iicode, 0, sizeof(iicode));
    dwSize=sizeof(iicode);
    if (MsiGetProperty(handle, "II_INSTALLATION", iicode, &dwSize) != ERROR_SUCCESS)
        return (ERROR_INSTALL_FAILURE);

    /* retrieves ii_system from registry */
    sprintf(KeyName, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", iicode);
    if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,KeyName, 0, KEY_QUERY_VALUE, &hKey))
    {
                dwSize=sizeof(ii_system);
                RegQueryValueEx(hKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
                RegCloseKey(hKey);
    }

    sprintf(BackupDir, "%s\\ingres\\install\\backup",
                ii_system);

    sprintf(logBuf, "Default backup dir:  %s\r\n",BackupDir);
    AppendToLog(logBuf);

    MsiSetProperty(handle, "NEWBACKUPDIR", BackupDir);
    MsiSetProperty(handle, "BACKUPDIR2", BackupDir);
    MsiSetProperty(handle, "_BrowseProperty", BackupDir);
    return ERROR_SUCCESS;
}

/*
**  Name: backup_files
**  
**  Description: Custom action used by PatchInstaller to
**				 backup files that are being replaced and
**				 then apply new files, backs up files to 
**				 files.bak.[patchid].
**				 Originally compares checksums and sizes of
**				 files in 'patchfiles.lis' on patch image to
**				 files to be updated on patch image stored in
**				 directory 'patch'.
**
**	Parameters:  handle - Handle to MSI Database.
**
**  Return Code: ERROR_INSTALL_FAILURE - Custom action failed.
**				 ERRIR_SUCCESS - Custom action completed successfully.
**
**    History:
**	12/19/07 (wanfr01)
**	    Bug 119639 
**	    Avoid system to avoid command prompts appearing
**	12/20/07 (wanfr01)
**	    Bug 119655
**	    dwSize needs to be initialized correctly to read the full MSI value
**
*/
BOOL
backup_files(MSIHANDLE handle)
{
char	iicode[II_SIZE];
char	KeyName[MAX_PATH+1];
char 	fileName[MAX_PATH+1], patchFile[MAX_PATH+1],
			file[MAX_PATH+1], file1[MAX_PATH+1], currDir[MAX_PATH+1] ;
char 	patchfile[MAX_PATH+1], line[MAX_PATH+1];
char	chksum[CKSUM_SIZE+1], filesize[SHORT_BUFF+1], chksumR[CKSUM_SIZE+1], 
			filesizeR[SHORT_BUFF+1];
char	logBuf[BUFF_SIZE];
char	tab[4] = "\t\r\n";
char	*ptab = (char *) &tab;
char	*pchecksum = (char *) &chksum;
char	*pfilesize = (char *) &filesize;
char	*pchecksumR = (char *) &chksumR;
char	*pfilesizeR = (char *) &filesizeR;
char	*pfile = (char *) &file;
char	*pfile1 = (char *) &file1;
int		pos = 1;
unsigned long	cksum;
FILE 	*fp;
HKEY	hKey;
DWORD	dwSize, dwError;
HANDLE	hnd, hnd2, hFile;
WIN32_FIND_DATA	fData, oldData;
char	buf[MAX_PATH+1], lpCaption[SMALLER_BUFF];
BOOL	bSilent = FALSE; /* Silent install if set to TRUE */
int forceReplace = -1;  /* indicates on how to handle installation of older files */
LONG	bCompare = -1;

/* Variables for Full Manifest Backup */
char	FullBackup;
int  	sequence, nBufferSize=128;
FLOAT   AvailBytes;     /* available bytes */
FLOAT   Bytes;          /* needed Kbytes for a backup */
HWND    hText;
CHAR    BACKUP_LOC      [_MAX_PATH];    /* backup location */
CHAR    II_VOLUME       [_MAX_PATH];    /* disk volume, derived from BACKUP_LOC */
CHAR    DestFile[_MAX_PATH];
CHAR    FileBuf[_MAX_PATH];		/* for random file usage */
DWORD   SectPerClus, BytesPerSect, NumFreeClus, TotClus;
char    CurCaption[SMALLER_BUFF];
FILE    *PreDataPtr;
    CHAR    TimeStamp[25];

FILE    *ManifestPtr;
char 	tempfile[MAX_PATH+1];
char    *charp;
CHAR    SourceFile[_MAX_PATH];
CHAR    DestDir[_MAX_PATH];
int	filecount=0,samecount=0,updatecount=0,errorcount=0, newcount=0;
CHAR	patch_fail=0;
	

/* retrieves installation id from msi */
memset(&iicode, 0, sizeof(iicode));
dwSize=sizeof(iicode);
if (MsiGetProperty(handle, "II_INSTALLATION", iicode, &dwSize) != ERROR_SUCCESS)
	return (ERROR_INSTALL_FAILURE);

/* retireves ii_system from registry */
sprintf(KeyName, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", iicode);
if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_QUERY_VALUE, &hKey))
   {
	dwSize=sizeof(ii_system);
	RegQueryValueEx(hKey, "II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
	RegCloseKey(hKey);
   }
else
   {
	sprintf(KeyName, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", iicode);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,KeyName, 0, KEY_QUERY_VALUE, &hKey))
	{
        	dwSize=sizeof(ii_system);
        	RegQueryValueEx(hKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
        	RegCloseKey(hKey);
	}
	else
	{
		sprintf(logBuf, "backup_files: Could not retrieve II_SYSTEM from the registry. Aborting...\r\n");
		AppendToLog(logBuf);
		return (ERROR_INSTALL_FAILURE);
	}
}

/* Update log */
sprintf(logBuf, "backup_files: iicode=%s ii_system=%s\r\n", iicode, ii_system);
AppendToLog(logBuf);

/* retrieve patchid from msi */
memset(&patchid, 0, sizeof(patchid));
dwSize=sizeof(patchid);
if (MsiGetProperty(handle, "PatchID", patchid, &dwSize) !=ERROR_SUCCESS)
	return (ERROR_INSTALL_FAILURE);


/* Update log */
sprintf(logBuf, "backup_files: patchid=%s\r\n", patchid);
AppendToLog(logBuf);

/* Figure out if this is silent install */
dwSize=sizeof(logBuf);
MsiGetProperty(handle, "UILevel", logBuf, &dwSize);
if (!strcmp(logBuf, "2"))
	bSilent=TRUE;
if (bSilent)
    {
	AppendToLog("Performing Silent Install\r\n");

	/* Set default backup directories */
	SetBackupDir(handle);
    }

/*Setup a window for messaging*/
dwSize = sizeof(buf);
MsiGetProperty(handle, "ProductName", buf, &dwSize);
sprintf(lpCaption, "%s - InstallShield Wizard", buf);
hWnd=FindWindow(NULL, lpCaption);

sprintf(CurCaption, "%s - InstallShield Wizard", buf);

/*set II_SYSTEM */
SetEnvironmentVariable( "II_SYSTEM" , ii_system);

/* retrieving target directory */
dwSize=sizeof(currDir);
//MsiGetSourcePath(handle, "TARGETDIR", currDir, &dwSize);
MsiGetProperty(handle, "PATCH_IMAGE", currDir, &dwSize);
/* Update log */
sprintf(logBuf, "backup_files: Image Directory = %s\r\n",currDir);
AppendToLog(logBuf);

    sprintf(manifestfile, "%s\\manifest.dat", currDir);
    sprintf(logBuf, "CalcTotals: manifest list = %s\r\n", manifestfile);
    AppendToLog(logBuf);

    MsiGetProperty(handle, "ISFULLBACKUP", logBuf, &nBufferSize);
    FullBackup = nBufferSize;

    dwSize=sizeof(BACKUP_LOC);
    MsiGetProperty(handle, "BACKUPDIR2", BACKUP_LOC, &dwSize);

    sprintf(logBuf, "backup directory from MSI: = %s\r\n",BACKUP_LOC);
    AppendToLog(logBuf);

    if (FullBackup)
    {
	int x;

	hWnd=FindWindow(NULL, CurCaption);

	AppendToLog("Performing full backup...\r\n\r\n");

        /*
        ** If we are backing up, make sure we have enough
        ** space in BACKUP_LOC.
        */

        hText = GetDlgItem(hWnd,STATUSITEM);
        EnableWindow(hText,TRUE);
        SetWindowText(hText,"Calculating disk space...");
        SendDlgItemMessage(hWnd, STATUSITEM, WM_ENABLE, 0,0);
        SendDlgItemMessage(hWnd, STATUSITEM, WM_PAINT,  0,0);

        strcpy(II_VOLUME, BACKUP_LOC);
        II_VOLUME[3] = '\0';
        GetDiskFreeSpace((LPTSTR)&II_VOLUME, &SectPerClus,
                         &BytesPerSect, &NumFreeClus, &TotClus);
        AvailBytes=((FLOAT)((LONGLONG)SectPerClus*
                    (LONGLONG)BytesPerSect*
                    (LONGLONG)NumFreeClus));
        Bytes = CalcTotals(handle);

	/* Bail out if you can't calculate totals  */
	if (Bytes == -1)
            return (ERROR_INSTALL_FAILURE);


        sprintf(logBuf,"Disk space on %s = %f, needed = %f\r\n",II_VOLUME,AvailBytes,Bytes);
        AppendToLog(logBuf);
        if (AvailBytes < Bytes)
        {
            sprintf(logBuf,"The amount of space needed for backup in directory %s is insufficient.  Required amount:  %.0f bytes\n",BACKUP_LOC,Bytes);
            BoxErr4(logBuf," ");
            return (ERROR_INSTALL_FAILURE);
        }

	/*
	** Now, as a sanity check, make sure we have
	** permission to create the directory (tree).  
	*/
	if (!CheckMakeDirRec(BACKUP_LOC, FALSE))
	{
	    BoxErr4("The backup directory was not able to be made.\n",
	            "Please check and correct directory permissions, or choose another location.");
	    return (ERROR_INSTALL_FAILURE);
	}

        sequence = 1;
        sprintf(DestFile, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
                ii_system, patchid, sequence);
        while (FileExists(DestFile))
        {
            /* Find a new sequence number. */
            sequence++;
            sprintf(DestFile, "%s\\ingres\\install\\control\\pre-p%s_%i.dat",
                    ii_system, patchid, sequence);
        }

        /* make sure the control directory exists */

        sprintf(FileBuf, "%s\\ingres\\install\\control", ii_system);
        if (!CheckMakeDirRec(FileBuf, TRUE))
        {
            sprintf(logBuf, "Warning: Unable to create %s\r\n",FileBuf);
            AppendToLog(logBuf);
	}

        sprintf(logBuf, "destfile = %s\r\n",DestFile);
        AppendToLog(logBuf);

        sprintf(PatchBackupLoc, "%s\\pre-p%s_%i", BACKUP_LOC, patchid,
                sequence);


        sprintf(logBuf, "Backup directory = %s\r\n",PatchBackupLoc);
        AppendToLog(logBuf);

        if (!CheckMakeDirRec(PatchBackupLoc, TRUE))
        {
            sprintf(logBuf, "Unable to create backup directory structure! Please check\nand correct permissions.\r\n\r\n");
            AppendToLog(logBuf);

            SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
            BoxErr4("Unable to create backup directory structure!", "");
            SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
            return(1);
        }


        hText = GetDlgItem(hWnd,STATUSITEM);
        EnableWindow(hText,TRUE);
        SetWindowText(hText,
                    "Backing up files ...");
        SendDlgItemMessage(hWnd, STATUSITEM, WM_ENABLE, 0,0);
        SendDlgItemMessage(hWnd, STATUSITEM, WM_PAINT,  0,0);

        PreDataPtr = fopen(DestFile, "w");
        if (!PreDataPtr)
        {
            sprintf(logBuf, "Error creating %s\r\n",DestFile);
            BoxErr4(logBuf, "");
        }
        fprintf(PreDataPtr, "## Patch:  %s\n", patchid);
        fprintf(PreDataPtr, "## Task:   backup_install\n");
        GetTimeOfDay(FALSE, TimeStamp);
        fprintf(PreDataPtr, "## Performed:      %s\n", TimeStamp);
        fprintf(PreDataPtr, "## Source: %s\n", ii_system);
        fprintf(PreDataPtr, "## Target: %s\n", PatchBackupLoc);
        fprintf(PreDataPtr, "## Sequence:       %i\n", sequence);
        fprintf(PreDataPtr, "##\n##\n## Contents:\n");
        fprintf(PreDataPtr, "## Backup information recorded during the backup of all\n");
        fprintf(PreDataPtr, "## affected files in \"Source\" installation to \"Target\"\n");
        fprintf(PreDataPtr, "## area prior to the application of \"Patch\" noted above.\n");
        fprintf(PreDataPtr, "##\n##\n");
        fprintf(PreDataPtr, "##                  file                        size     checksum\n");
        fprintf(PreDataPtr, "##\n");

        sprintf(logBuf, "Backing up Source files...\r\n");
        AppendToLog(logBuf);

	/* Perform actual file copy */
	ManifestPtr = fopen(manifestfile, "r");
	if (!ManifestPtr)
	{
            sprintf(logBuf, "Error reading %s\r\n",manifestfile);
            BoxErr4(logBuf, "");
            return (ERROR_INSTALL_FAILURE);
	}

	while (fscanf (ManifestPtr,"%s",&tempfile) != EOF)
	{
	    /* Manifest files have the wrong slashes - convert */
	    for (charp = tempfile;*charp != '\0';charp++)
		if (*charp == '/') *charp='\\';

	    sprintf(SourceFile, "%s\\ingres\\%s", ii_system, tempfile);

            if (FileExists(SourceFile))
            {
                sprintf(DestFile, "%s\\%s", PatchBackupLoc, tempfile);
                sprintf(DestDir, "%s\\%s", PatchBackupLoc, tempfile);

		/* Truncate filename to leave directory */
	        for (charp = &(DestDir[strlen(DestDir)]);*charp != '\\';charp--);
		*charp='\0';

                if (!FileExists(DestDir))
                {
                    if (!CheckMakeDirRec(DestDir, TRUE))
                    {
                        AppendToLog("Unable to create backup directory structure! Please check\r\nand correct permissions.\r\n\r\n");
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

                    sprintf(logBuf, "Unable to backup files, CopyFile error %d. Please check\r\nand correct permissions.\r\n\r\n", status);
                    AppendToLog(logBuf);
                    SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
                    BoxErr2("Unable to back up files!\nError = ", status);
                    SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
                    return(1);
                }

                fprintf(PreDataPtr, "%-58s  %9d  %10u\n", tempfile,
                        GetCurrentFileSize(SourceFile), 
                	GetCurrentFileChecksum(SourceFile));

	    }  /* if FileExists */
	}

        fclose(ManifestPtr);

        fprintf(PreDataPtr, "##\n##\n## File format version: 1\n");
        fprintf(PreDataPtr, "##\n##\n## State:  completed\n");
        fclose(PreDataPtr);

        sprintf(logBuf, "Backup completed; verifying files which were backed up ...\r\n\r\n");
        AppendToLog(logBuf);
        if(!CheckBackup(sequence, CurCaption))
        {
            sprintf(logBuf, "Verification of backup complete.\r\n\r\n");
            AppendToLog(logBuf);
            sprintf(logBuf, "Backup completed successfully.\r\n\r\n");
            AppendToLog(logBuf);
        }
        else
        {
            CHAR        MessageOut[1024];

            sprintf(MessageOut, "%s\\ingres\\files\\install_p%s.log file for more information.",
                    ii_system, patchid);
            sprintf(logBuf,
                    "\nVerification failed; Backup failed.\n\n");
            AppendToLog(logBuf);
            SendMessage(hDlgGlobal, WM_ENDSESSION, 0, 0L);
            BoxErr4("The backup did not complete due to verification failure. Please see the\n",
                    MessageOut);
            SendMessage(hDlgGlobal, WM_CLOSE, 0, 0L);
            return(1);
        }
	
    }  /* End FullBackup */

    hText = GetDlgItem(hWnd,STATUSITEM);
    EnableWindow(hText,TRUE);
    SetWindowText(hText,
                    "Installing patch... ");
    SendDlgItemMessage(hWnd, STATUSITEM, WM_ENABLE, 0,0);
    SendDlgItemMessage(hWnd, STATUSITEM, WM_PAINT,  0,0);


/* forming path to patchfiles.lis on patch image */
sprintf(patchfile, "%s\\patchfiles.lis", currDir);
sprintf(logBuf, "backup_files: List of patch files = %s\r\n", patchfile);
AppendToLog(logBuf);

/*
** Open patchfiles.lis for parsing, compare checksums
** and files sizes from patchfiles.lis to actual files
** stored on patch image.  Eliminate corruptions.
*/
fp = fopen(patchfile, "r");
/* Update log */
sprintf(logBuf, "backup_files: Comparing patch files with expected checksums\r\n");
AppendToLog(logBuf);
sprintf(logBuf, "backup_files: Opened list of files with handle=%x\r\n", fp);
AppendToLog(logBuf);
    if (fp != NULL)
    {
	while (fgets(line, sizeof(line), fp) != NULL && line != EOF)
	{	
		filecount++;

		pchecksum = strtok(line, tab);
		pfilesize = strtok(NULL, tab);		
		pfile = strtok(NULL, tab);
		sprintf(fileName, "%s\\%s", currDir, pfile);
        if ((hFile = CreateFile(fileName, GENERIC_READ , FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) != NULL)
		{
			dwSize = GetFileSize(hFile, NULL);
			CloseHandle(hFile);
		}
		else
		{
			/* Update log */
			sprintf(logBuf, "backup_files: Couldn't find physical file on the patch image: %s\r\n", fileName);
			AppendToLog(logBuf);
			patch_fail=TRUE;
			continue;
		}
		cksum = checksum(fileName);
		sprintf(pchecksumR, "%u", cksum);
		sprintf(pfilesizeR, "%d", dwSize);
		/* Update log */
		if (strcmp(pchecksum, pchecksumR) !=0 || strcmp(pfilesize, pfilesizeR) != 0)
		{
			sprintf(logBuf, "\r\nbackup_files: Error, corrupted file detected: filename = %s\r\n", pfile);
			AppendToLog(logBuf);
			sprintf(logBuf, "backup_files: checksum expected= %s, filesize expected= %s, checksum=%s, filesize = %s\r\n", 
				pchecksum, pfilesize, pchecksumR, pfilesizeR);
			AppendToLog(logBuf);
			patch_fail=TRUE;
			continue;
		}
	}

        fclose(fp);

	sprintf (logBuf, "backup_files:  %d files processed\r\n\r\n",filecount);
	AppendToLog(logBuf);
	if (patch_fail)
	    return(ERROR_INSTALL_FAILURE);
    }
    else
    {
	sprintf(logBuf, "backup_files: Error while reading %s\r\n", patchfile);
	AppendToLog(logBuf);
	return(ERROR_INSTALL_FAILURE);
    }

/*
** If we got this far in a code, then no corruption detected in new binaries.
** Backup old files and copy new files in place of them.
**
*/
sprintf(logBuf, "****** Comparing Patch Timestamps to installation Timestamps ***\r\n"); 
AppendToLog(logBuf);
filecount = 0;

fp = fopen(patchfile, "r");
 while (fp != NULL && fgets(line, sizeof(line), fp) != NULL && line != EOF)
 {
	SYSTEMTIME write, write2;
	int t = 0;
	dwError = ERROR_SUCCESS;

	filecount++;
	pchecksum = strtok(line, tab); 
	pfilesize = strtok(NULL, tab);
	pfile = strtok(NULL, tab);
	sprintf(patchFile, "%s\\%s", currDir, pfile);
	sprintf(file1, "%s\\%s", ii_system, pfile+6);
	if (access(file1, 00) == -1 && errno == ENOENT)
	{
	     char path[MAX_PATH+1];
	     int i ;

	     /* if file doesn't exist on installed image directory might not exist either, create directory */
   	     memcpy(path, file1, sizeof(file1));
	     for (i=strlen(path)-1; i>0; i--)
	     {
			 if (path[i] != '\\')
				 path[i]='\0';
			 else
			 {
				 path[i]='\0';
				 break;
			 }
		 }
		 sprintf(logBuf, "backup_files: Creating directory %s\r\n", path);
		 AppendToLog(logBuf);	  
		 CheckMakeDirRec(path,TRUE);
	}
	if (t++ == 10)
		dwError=1;
	if ( (hnd2 = FindFirstFile(file1, &oldData) ) != INVALID_HANDLE_VALUE
		&&  (hnd = FindFirstFile(patchFile, &fData) ) != INVALID_HANDLE_VALUE  )
	{
		FileTimeToSystemTime(&oldData.ftLastWriteTime, &write);
		FileTimeToSystemTime(&fData.ftLastWriteTime, &write2);
/*
		sprintf(logBuf, "backup_files: %s = %d/%d/%d %d:%d:%d ,\r\n 
Patch File %s = %d/%d/%d %d:%d:%d\r\n", 
file1,      write.wMonth, write.wDay, write.wYear,write.wHour, write.wMinute, write.wSecond,
 patchFile, write2.wMonth, write2.wDay, write2.wYear, write2.wHour, write2.wMinute, write2.wSecond
);
		sprintf(logBuf, "backup_files: %s = %d/%d/%d %d:%d:%d,\r\n Patch File %s = %d/%d/%d %d:%d:%d\r\n", file1, write.wMonth, write.wDay, write.wYear,write.wHour, write.wMinute, write.wSecond, patchFile, write2.wMonth, write2.wDay, write2.wYear, write2.wHour, write2.wMinute, write2.wSecond);
		AppendToLog(logBuf);
*/
		
		/*
		** We need to define a few seconds tolerance, if write timestamps between patch file and isntalled file
		** differ by 1 or 2 seconds it is safe to assume it's the same timestamp.
		** Code below finds a difference between two timestamps and defines a tolerance level of 3 seconds
		** If difference between two timestamps is less than 3 seconds then timestamps is the same.
		*/
		FileTimeToSystemTime(&oldData.ftLastWriteTime, &write);
		FileTimeToSystemTime(&fData.ftLastWriteTime, &write2);
		if (((bCompare = CompareFileTime( &oldData.ftLastWriteTime, &fData.ftLastWriteTime)) < 0 )
			|| forceReplace == IDYES)
		{
			dwError = perform_backup_and_copy(file1, patchFile, patchid);
			if (dwError != ERROR_SUCCESS)
			{
				sprintf(logBuf, "backup_files: %s = %d/%d/%d %d:%d:%d - System Error %d.\r\n ", 
				    file1, write.wMonth, write.wDay, write.wYear,write.wHour, write.wMinute, write.wSecond,
				    dwError);
				AppendToLog(logBuf);
				errorcount++;	
				continue;
			}
			else
			{
				sprintf(logBuf, "backup_files: %s = %d/%d/%d %d:%d:%d - Updated.\r\n", 
				    file1, write.wMonth, write.wDay, write.wYear,write.wHour, write.wMinute, write.wSecond);
				AppendToLog(logBuf);
				updatecount++;	
			}
		}
		else if (bCompare == 0 || (bCompare > 0 && write.wYear == write2.wYear && write.wMonth == write2.wMonth 
			&& write.wDay == write2.wDay && write.wHour == write2.wHour && (write.wSecond - write2.wSecond < 3)))
		{
			bCompare = 0;
/*
			sprintf(logBuf, "backup_files: %s = %d/%d/%d %d:%d:%d - Same Timestamp\r\n ", 
			    file1, write.wMonth, write.wDay, write.wYear,write.wHour, write.wMinute, write.wSecond);
			AppendToLog(logBuf);
*/
			if ((hFile = CreateFile(file1, GENERIC_READ , FILE_SHARE_READ,
						NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) != NULL)
			{
					dwSize = GetFileSize(hFile, NULL);
					CloseHandle(hFile);
			}
			else
			{
					/* Update log */
					sprintf(logBuf, "backup_files: Couldn't open physical file on the installed image: %s\r\n", file1);
					AppendToLog(logBuf);
					dwError = GetLastError();
					errorcount++;
					continue;
			}
			cksum = checksum(file1);
			sprintf(pchecksumR, "%u", cksum);
			sprintf(pfilesizeR, "%d", dwSize);
			if (strcmp(pchecksum, pchecksumR) ==0 || strcmp(pfilesize, pfilesizeR) == 0)
			{
			    sprintf(logBuf, "backup_files: %s = %d/%d/%d %d:%d:%d - Skipped:  Same Time/Checksum\r\n", 
			    file1, write.wMonth, write.wDay, write.wYear,write.wHour, write.wMinute, write.wSecond);
			    AppendToLog(logBuf);
			    samecount++;
			}
			else 
			{
				bCompare = 1;
			}
						
		}

		if (bCompare > 0 && !bSilent && forceReplace ==-1)
		{
		/* Enable this code if you want user to be prompted when install detects that files being installed are older
		** than the files in the installation.
		*/
      		/*sprintf(logBuf, "One or more files being installed are older than the files on the installed image.\r\n Would you like to replace them anyway?", file1);
			forceReplace = MessageBox( hWnd, logBuf, "Ingres Patch Installer", MB_APPLMODAL | MB_ICONSTOP | MB_YESNOCANCEL);
			switch (forceReplace)
			{
				case IDYES:
					AppendToLog("***User agreed to replace installed binaries with older binaries. ***\r\n");
					dwError = perform_backup_and_copy(file1, patchFile, patchid);
					break;
				case IDNO:
					sprintf(logBuf, "backup_files: File %s was not replaced, it's up to date.\r\n", file1);
					AppendToLog(logBuf);
					break;
				case IDCANCEL:
				default:
					AppendToLog("Installation was canceled by the user\r\n");
					dwError = 1;
			}
			if (dwError)
				break;
		*/

		/*
		**  This bit (next 3 lines) will need to be commented out if above section is enabled.
		*/
			
			sprintf(logBuf, "\r\n backup_files: %s = %d/%d/%d %d:%d:%d - TimeStamp Error.  Patch file is older: \r\n ", 
			    file1, write.wMonth, write.wDay, write.wYear,write.wHour, write.wMinute, write.wSecond);
				AppendToLog(logBuf);

			sprintf(logBuf, "backup_files: Patch File     %s = %d/%d/%d %d:%d:%d\r\n\r\n",
 patchFile, write2.wMonth, write2.wDay, write2.wYear, write2.wHour, write2.wMinute, write2.wSecond
);
				AppendToLog(logBuf);
			dwError = 1;
			errorcount++;
			continue;
		}
		else
		{
/*
			sprintf(logBuf, "backup_files: File %s is up to date.\r\n\r\n", file1);
	 		AppendToLog(logBuf);
*/
		}
	}
	else if (hnd2 == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		/* File exists on patch image but doesn't exist on installed image. */
		
		/* However, in such a case, don't copy in the file if it is a .crs file (b121784). */
		if(IsSuffixed(patchFile, ".crs"))
			continue;
		
		CopyFile(patchFile, file1, FALSE);
		sprintf(logBuf, "backup_files: Copy new file from %s to %s\r\n", patchFile, file1);
		AppendToLog(logBuf);
		newcount++;
	}
	else
	{
		dwError = GetLastError();
		AppendToLog("Unknown problem was encountered while copying files\r\n");
		sprintf(logBuf, "backup_files: FindFirstFile returned with error: (%d)\r\n", dwError);
		AppendToLog(logBuf);
		errorcount++;
		continue;
	}

	if (dwError)
	{
		sprintf(logBuf, "backup_files: Failed to Copy file %s.\r\nAborting with system error (%d)!\r\n", patchFile, dwError);
		AppendToLog(logBuf);
		errorcount++;
		continue;
	}
}
fclose(fp);

		sprintf(logBuf, "\r\nInstall statistics:\r\nTotal Files:  %d\r\n",filecount);
		AppendToLog(logBuf);

		sprintf(logBuf, "Total Updated:  %d\r\nTotal identical files skipped:  %d\r\n",updatecount,samecount);
		AppendToLog(logBuf);
		sprintf(logBuf, "Total new files:  %d\r\nTotal files with Errors:  %d\r\n",newcount,errorcount);
		AppendToLog(logBuf);

    if (errorcount)
    {
	rollback_patch(handle);
	return (ERROR_INSTALL_FAILURE);
    }


    return ERROR_SUCCESS;
}

BOOL 
perform_backup_and_copy(char *file1, char *patchFile, char *patchid)
{
	char file2[MAX_PATH+1];
	char logBuf[BUFF_SIZE];
	DWORD dwError = ERROR_SUCCESS;
	DWORD attrib;

	sprintf(file2, "%s.bak.%s", file1, patchid);
	if (CopyFile(file1, file2, FALSE) != 0)
	{
/*
		sprintf(logBuf, "perform_backup_and_copy: Copy %s to %s\r\n", file1, file2);
		AppendToLog(logBuf);
*/
	}
	else
	{
		sprintf(logBuf, "perform_backup_and_copy: Error performing copy %s to %s\r\n", file1, file2);
		AppendToLog(logBuf);
		dwError = GetLastError();
	}
	if (dwError == ERROR_SUCCESS && CopyFile(patchFile, file1, FALSE) != 0)
	{
/*
		sprintf(logBuf, "perform_backup_and_copy: Copy update file from %s to %s\r\n", patchFile, file1);
		AppendToLog(logBuf);
*/
	}
	else
	{
		sprintf(logBuf, "\r\nperform_backup_and_copy: Copy update file from %s to %s\r\n", patchFile, file1);
		AppendToLog(logBuf);
		dwError = GetLastError();
		sprintf(logBuf, "Copy of file %s to %s failed with system error (%d).  Retry.\r\n", patchFile, file1, dwError);
		AppendToLog(logBuf);
		Sleep(3000);
		dwError = 0;
		attrib = GetFileAttributes(file1);
		if (attrib & FILE_ATTRIBUTE_HIDDEN)
			SetFileAttributes(file1, attrib & ~ FILE_ATTRIBUTE_HIDDEN);
		if (CopyFile(patchFile, file1, FALSE) == 0)
			dwError = GetLastError();
	}

	if (dwError == ERROR_SHARING_VIOLATION)
	{
		DeleteFile(file2);
	}

	return dwError;

}

/*
**  Name: update_vers
**  
**  Description: Update vers file on the image to contain patchid.
**
**	Parameters:  handle - Handle to MSI Database.
**
**  Return Code: ERROR_INSTALL_FAILURE - Custom action failed.
**				 ERRIR_SUCCESS - Custom action completed successfully.
*/
BOOL
update_vers(MSIHANDLE handle)
{
char	iicode[II_SIZE];
char	patchid[PATCH_SIZE];
char	logBuf[BUFF_SIZE];	
char	KeyName[MAX_PATH+1], KeyName2[MAX_PATH+1], ii_system[MAX_PATH+1]; 
char	versFile[MAX_PATH+1], versBuf[BUFF_SIZE], lineBuff[BUFF_SIZE];
char 	*plineBuff = NULL;           
HKEY	hKey;
DWORD	dwType, dwSize;
HANDLE	hVers;
char	newLine[3]="\r\n";
FILETIME	lCreate, lAccess, lWrite;
CONST FILETIME *plCreate = &lCreate;
CONST FILETIME *plAccess = &lAccess;
CONST FILETIME *plWrite = &lWrite;
SYSTEMTIME sysTime1, sysTime2, sysTime3;


	/* retrieve installation id */
	memset(&iicode, 0, sizeof(iicode));
	dwSize=sizeof(iicode);
	if (MsiGetProperty(handle, "II_INSTALLATION", iicode, &dwSize) != ERROR_SUCCESS)
		return (ERROR_INSTALL_FAILURE);

	/* Update log */
	sprintf(logBuf, "update_vers: iicode=%s\r\n", iicode);
	AppendToLog(logBuf);

	/* retrieve patch id */
	memset(&patchid, 0, sizeof(patchid));
	dwSize=sizeof(patchid);
	if (MsiGetProperty(handle, "PatchID", patchid, &dwSize) !=ERROR_SUCCESS)
		return (ERROR_INSTALL_FAILURE);
  if (strcmp(patchid, "00000"))
  {
	/* Update log */
	sprintf(logBuf, "update_vers: patchid=%s\r\n", patchid);
	AppendToLog(logBuf);

	/* retireve ii_system */
	sprintf(KeyName, "Software\\IngresCorporation\\Ingres\\%s_Installation", iicode);
	sprintf(KeyName2, "Software\\ComputerAssociates\\Ingres\\%s_Installation",  iicode);
	/* Update log */
	sprintf(logBuf, "update_vers: Looking for Key=%s or Key=%s\r\n", KeyName, KeyName2);
	AppendToLog(logBuf);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey))
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName2, 0, KEY_ALL_ACCESS, &hKey))
			return (ERROR_INSTALL_FAILURE);

	dwType = REG_SZ;
	dwSize = sizeof(ii_system);
	memset(&ii_system, 0, sizeof(ii_system));
	if (RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, ii_system, &dwSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
	}
	RegCloseKey(hKey);

	/* Update log */
	sprintf(logBuf, "update_vers: iisystem=%s\r\n", ii_system);
	AppendToLog(logBuf);

	SetEnvironmentVariable( "II_SYSTEM" , ii_system);

	/* Open version.rel in a target installation for update and update */
	sprintf(versFile, "%s\\ingres\\version.rel", ii_system);
	hVers = CreateFile(versFile, GENERIC_READ|GENERIC_WRITE , 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	GetFileTime(hVers, &lCreate, &lAccess, &lWrite);
	FileTimeToSystemTime(plCreate, &sysTime1);
	FileTimeToSystemTime(plAccess, &sysTime2);
	FileTimeToSystemTime(plWrite, &sysTime3);
	sprintf(logBuf, "update_vers: version.rel CreateDate = %d/%d/%d, WriteDate = %d/%d/%d\r\n", sysTime1.wMonth, sysTime1.wDay, sysTime1.wYear, sysTime3.wMonth, sysTime3.wDay, sysTime3.wYear);
	AppendToLog(logBuf);
                if (hVers != NULL)
	{
		memset(&versBuf, 0, sizeof(versBuf));
		if (ReadFile(hVers, versBuf, sizeof(versBuf), &dwSize, NULL) && dwSize != 0 && versBuf != 0)
		{
			int lineNo = 1;	
			char smBuf[32];
			do
			{
			     plineBuff = ReadSingleLine(lineNo++, versBuf);	
			     if (plineBuff != NULL)
			     sprintf(lineBuff, "%s", plineBuff);
			     else
			     lineBuff[0]='\0';
			     sprintf(logBuf, "update_vers: Patch  #%d in version.rel = %s\r\n", lineNo-1, lineBuff);
			     AppendToLog(logBuf);
			     if (*lineBuff && strncmp(lineBuff, patchid, PATCH_SIZE-1) == 0)
				break;
			     else if (*lineBuff == NULL )
			     {
				SetFilePointer(hVers, -1, NULL, FILE_END);
				ReadFile(hVers, smBuf, 1, &dwSize, NULL);
				if (smBuf && !(*smBuf == '\r' || *smBuf == '\n') )
				{
					SetFilePointer(hVers, 0, NULL, FILE_END);
					WriteFile(hVers, newLine, strlen(newLine), &dwSize, NULL);
				}					
				SetFilePointer(hVers, 0, NULL, FILE_END);
				WriteFile(hVers, patchid, strlen(patchid), &dwSize, NULL);
				WriteFile(hVers, newLine, strlen(newLine), &dwSize, NULL);
			     }
			}
			while(plineBuff != 0);
			
		}
	}
	FlushFileBuffers(hVers);
	SetFileTime(hVers, plCreate, plAccess, plWrite);
	CloseHandle(hVers);

	sprintf(logBuf, "update_vers: VERS file was updated successfully.\r\n");
	AppendToLog(logBuf);

  }
  else
  {
	sprintf(logBuf, "update_vers: Detected SOL package install, version.rel will not be updated.\r\n");
	AppendToLog(logBuf);
  }
	return ERROR_SUCCESS;
	
}

/*
**  Name: rollback_patch
**  
**  Description: Rolls back patch if the failure occurs in the middle of copying
**		 new files to installation.
**
**	Parameters:  handle - Handle to MSI database.
**
**  Return Code: 0 for success
**		 rc for failure.  rc=GetLastError();
*/
BOOL
rollback_patch(MSIHANDLE handle)
{
char	iicode[II_SIZE];
char	patchid[PATCH_SIZE];
char	logBuf[BUFF_SIZE];	
char	ii_system[MAX_PATH+1], KeyName[MAX_PATH+1], KeyName2[MAX_PATH+1], currDir[MAX_PATH+1], patchfile[MAX_PATH+1];
char	line[MAX_PATH*2], file[MAX_PATH+1], nfile[MAX_PATH+1], ofile[MAX_PATH+1]; 
char	tab[4] = "\t\r\n";
char	*pfile=(char *) &file;
DWORD	dwSize, dwType;
HKEY	hKey;
FILE	*fp;
DWORD	rc = ERROR_SUCCESS;

	/* retrieve installation id */
	memset(&iicode, 0, sizeof(iicode));
	dwSize=sizeof(iicode);
	if (MsiGetProperty(handle, "II_INSTALLATION", iicode, &dwSize) != ERROR_SUCCESS)
		return (ERROR_INSTALL_FAILURE);

	/* retrieve patch id */
	memset(&patchid, 0, sizeof(patchid));
	dwSize=sizeof(patchid);
	if (MsiGetProperty(handle, "PatchID", patchid, &dwSize) !=ERROR_SUCCESS)
		return (ERROR_INSTALL_FAILURE);

	/* retireve ii_system */
	sprintf(KeyName, "Software\\IngresCorporation\\Ingres\\%s_Installation", iicode);
	sprintf(KeyName2, "Software\\ComputerAssociates\\Ingres\\%s_Installation",  iicode);
	/* Update log */
	sprintf(logBuf, "rollback_patch: Looking for Key=%s\r\n", KeyName);
	AppendToLog(logBuf);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName2, 0, KEY_ALL_ACCESS, &hKey))
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey))
			return (ERROR_INSTALL_FAILURE);

	dwType = REG_SZ;
	dwSize = sizeof(ii_system);
	memset(&ii_system, 0, sizeof(ii_system));
	if (RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, ii_system, &dwSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
	}
	RegCloseKey(hKey);


	/* retrieving image directory */
	dwSize=sizeof(currDir);
	MsiGetProperty(handle, "PATCH_IMAGE", currDir, &dwSize);
	/* forming path to patchfiles.lis on patch image */
	sprintf(patchfile, "%s\\patchfiles.lis", currDir);

	fp = fopen(patchfile, "r");
	if (fp != NULL)
	{
		while (fgets(line, sizeof(line), fp) !=NULL && line != EOF)
		{
			strtok(line, tab);
			strtok(NULL, tab);
			pfile = strtok(NULL, tab);
			sprintf(nfile, "%s\\%s", ii_system, pfile+6);
			sprintf(ofile, "%s\\%s.bak.%s", ii_system, pfile+6, patchid);
			if (_access(ofile, 0) != -1)
			{
				if (CopyFile(ofile, nfile, FALSE))
				{
					DeleteFile(ofile);
					sprintf(logBuf, "rollback_patch: Copied original file to %s, removed %s\r\n", nfile, ofile);
					AppendToLog(logBuf);
				}
				else
				{
					sprintf(logBuf, "rollback_patch: Failed to copy %s to %s\r\n", ofile, nfile);
					AppendToLog(logBuf);
					rc = GetLastError();
				}

			}
			else
			{
				sprintf(logBuf, "rollback_patch: File %s was not yet patched\r\n", nfile);
				AppendToLog(logBuf);
			}
		}
	}
	if (rc)
	{
		sprintf(logBuf, "*** There was at least one problem while rolling back, last error (%d)***\r\n", rc);
		AppendToLog(logBuf);
	}
	else
	{
		sprintf(logBuf, "*** Rollback comleted successfully!! ***\r\n");
		AppendToLog(logBuf);
	}

	return rc;
}

/*
**  Name: delete_backups
**  
**  Description: Removes all files that were backed up while applying the patch.
**
**	Parameters:  handle - Handle to MSI database.
**
**  Return Code: 0 for success
**		 rc for failure.  rc=GetLastError();
*/
BOOL
delete_backups(MSIHANDLE handle)
{
char	iicode[II_SIZE];
char	patchid[PATCH_SIZE];
char	logBuf[BUFF_SIZE];	
char	ii_system[MAX_PATH+1], KeyName[MAX_PATH+1], KeyName2[MAX_PATH+1], currDir[MAX_PATH+1], patchfile[MAX_PATH+1];
char	line[MAX_PATH*2], file[MAX_PATH+1], ofile[MAX_PATH+1]; 
char	tab[4] = "\t\r\n";
char	*pfile= (char *) &file;
DWORD	dwSize, dwType;
HKEY	hKey;
FILE	*fp;
DWORD	rc = ERROR_SUCCESS;

	/* retrieve installation id */
	memset(&iicode, 0, sizeof(iicode));
	dwSize=sizeof(iicode);
	if (MsiGetProperty(handle, "II_INSTALLATION", iicode, &dwSize) != ERROR_SUCCESS)
		return (ERROR_INSTALL_FAILURE);

	/* retrieve patch id */
	memset(&patchid, 0, sizeof(patchid));
	dwSize=sizeof(patchid);
	if (MsiGetProperty(handle, "PatchID", patchid, &dwSize) !=ERROR_SUCCESS)
		return (ERROR_INSTALL_FAILURE);

	/* retireve ii_system */
	sprintf(KeyName, "Software\\IngresCorporation\\Ingres\\%s_Installation", iicode);
	sprintf(KeyName2, "Software\\ComputerAssociates\\Ingres\\%s_Installation",  iicode);
	/* Update log */
	sprintf(logBuf, "delete_backups: Looking for Key=%s\r\n", KeyName);
	AppendToLog(logBuf);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_ALL_ACCESS, &hKey))
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyName2, 0, KEY_ALL_ACCESS, &hKey))
			return (ERROR_INSTALL_FAILURE);

	dwType = REG_SZ;
	dwSize = sizeof(ii_system);
	memset(&ii_system, 0, sizeof(ii_system));
	if (RegQueryValueEx(hKey, "II_SYSTEM", NULL, &dwType, ii_system, &dwSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return (ERROR_INSTALL_FAILURE);
	}
	RegCloseKey(hKey);


	/* retrieving image directory */
	dwSize=sizeof(currDir);
	MsiGetProperty(handle, "PATCH_IMAGE", currDir, &dwSize);
	/* forming path to patchfiles.lis on patch image */
	sprintf(patchfile, "%s\\patchfiles.lis", currDir);

	fp = fopen(patchfile, "r");
	if (fp != NULL)
	{
		while (fgets(line, sizeof(line), fp) !=NULL && line != EOF)
		{
			strtok(line, tab);
			strtok(NULL, tab);
			pfile = strtok(NULL, tab);
			sprintf(ofile, "%s\\%s.bak.%s", ii_system, pfile+6, patchid);
			if (_access(ofile, 0) != -1)
			{
				if (DeleteFile(ofile))
				{
					sprintf(logBuf, "delete_backups: Removed backup %s\r\n", ofile);
					AppendToLog(logBuf);
				}
				else
				{
					sprintf(logBuf, "delete_backups: Failed to delete %s\r\n", ofile);
					AppendToLog(logBuf);
					rc = GetLastError();
				}

			}
			else
			{
				sprintf(logBuf, "delete_backups: Backup %s was not found\r\n", ofile);
				AppendToLog(logBuf);
			}
		}
	}
	if (rc)
	{
		sprintf(logBuf, "*** There was at least one problem removing backed up files, last error (%d)***\r\n", rc);
		AppendToLog(logBuf);
	}
	else
	{
		sprintf(logBuf, "*** Backups removed successfully!! ***\r\n");
		AppendToLog(logBuf);
	}

	return rc;
}

/*
**  Name: ReadSingleLine
**  
**  Description: Given buffer with multiple lines returns a line specified by a line number parameter.
**
**	Parameters:  lineNum - Line number that this function will return.
**	                    fileBuffer - Buffer to search for line number in.
**
**  Return Code: char pointer to a line being returned.
*/
char *
ReadSingleLine(int lineNum, char *fileBuffer)
{
     	char 	c;
	char	*pbuffer = (char *) &buffer;
	int	line, cNum = 0;
	int	res;

	memset(&buffer, 0, sizeof(buffer));
	for (line=0; line<=lineNum; line++)
	{
		cNum = 0;
		while ((res = sscanf(fileBuffer, "%c", &c) )!= EOF)
		{
			fileBuffer++;
			if (c == 10 || c == 13)
			{
			     if (*fileBuffer == 10 || *fileBuffer == 13)
				fileBuffer++;	
			     break;
			}
			
			if (line == lineNum)
			{	
			     buffer[cNum++] = c;
			     buffer[cNum] = '\0';
			}
		}	
		if (res == EOF && cNum == 0 )
		{
			AppendToLog("Returning NULL from ReadSingleLine\r\n");
			pbuffer = 0;
			break;
		}
	}

	return pbuffer;
}

/*
**  Name: ingres_stop_ingres
**  
**  Description: Stops ingres installation if it's running and shuts down ivm.
**
**	Parameters:  handle - Handle to MSI Database.
**
**  Return Code: ERROR_INSTALL_FAILURE - Custom action failed.
**				 ERRIR_SUCCESS - Custom action completed successfully.
*/
UINT __stdcall 
ingres_stop_ingres(MSIHANDLE hInstall) 
{
	char ii_id[3], ii_system[MAX_PATH];
	DWORD dwSize;
	HKEY hRegKey;
	char szBuf[MAX_PATH], szBuf2[MAX_PATH];
	BOOL bSilent, bCluster;
	HCLUSTER hCluster = NULL;
	HRESOURCE hResource = NULL;
	WCHAR lpwResourceName[256];

    /* get II_INSTALLATION and II_SYSTEM */
    dwSize=sizeof(ii_id); *ii_id=0;
    MsiGetProperty(hInstall, "II_INSTALLATION", ii_id, &dwSize);

    dwSize=sizeof(ii_system); *ii_system=0;
    MsiGetTargetPath(hInstall, "INGRESFOLDER", ii_system, &dwSize);

    if (!ii_system[0])
    {
	if (!Local_NMgtIngAt("II_INSTALLATION", szBuf) && !_stricmp(szBuf, ii_id) )
	    GetEnvironmentVariable("II_SYSTEM", ii_system, sizeof(ii_system));
    }
    if (!ii_system[0])
    {
	sprintf(szBuf, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", ii_id);
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	     dwSize=sizeof(ii_system);
	     RegQueryValueEx(hRegKey, "II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
	     RegCloseKey(hRegKey);
	}
    }
    if (!ii_system[0])
    {
	sprintf(szBuf, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", ii_id);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	    dwSize=sizeof(ii_system);
	    RegQueryValueEx(hRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
	    RegCloseKey(hRegKey);
	}
    }
    if (!ii_system[0])
    {
	sprintf(szBuf, "SOFTWARE\\ComputerAssociates\\IngresII\\%s_Installation", ii_id);
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hRegKey))
	{
	    dwSize=sizeof(ii_system);
	    RegQueryValueEx(hRegKey,"II_SYSTEM", NULL, NULL, (BYTE *)ii_system, &dwSize);
	    RegCloseKey(hRegKey);
	}
    }

    if (ii_system[strlen(ii_system)-1] == '\\')
	ii_system[strlen(ii_system)-1] = '\0';

    SetEnvironmentVariable( "II_SYSTEM" , ii_system);

    /* shut down ivm */
    sprintf(szBuf, "%s\\ingres\\bin\\ivm.exe", ii_system);
    if (GetFileAttributes(szBuf) != -1)
    {
	sprintf(szBuf, "\"%s\\ingres\\bin\\ivm.exe\" -stop", ii_system);
 	Execute(szBuf, FALSE);
    }
	

    dwSize=sizeof(szBuf);
    MsiGetProperty(hInstall, "INGRES_SDK", szBuf, &dwSize);
    if(!strcmp(szBuf, "1"))
    {
	/* stop OpenROAD AppServer */
	ExecuteEx("net stop orsposvc");

	/* stop portal */
	GetCurrentDirectory(sizeof(szBuf), szBuf);
	sprintf(szBuf2, "%s\\PortalSDK", ii_system);
	SetCurrentDirectory(szBuf2);
	ExecuteEx("stopportal.bat");
	SetCurrentDirectory(szBuf);

	/* stop apache */
	if(RegOpenKeyEx(HKEY_CURRENT_USER, 
	  "SOFTWARE\\Apache Group\\Apache\\1.3.23", 
	  0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
	{
	    char szData[256];
	    DWORD dwSize=sizeof(szData);
		
	    if(RegQueryValueEx(hRegKey, "ServerRoot", 
	       NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS)
	    {		
		if (szData[strlen(szData)-1]=='\\') 
		    szData[strlen(szData)-1]='\0';
		sprintf(szBuf, "\"%s\\Apache.exe\" -k stop", szData);
		ExecuteEx(szBuf);
	    }
	    RegCloseKey(hRegKey);
	}
    }
	
	/* shutdown Ingres */
	MsiSetProperty(hInstall, "INGRES_RUNNING", "0");

	if (!IngresAlreadyRunning(ii_system))
		return ERROR_SUCCESS;

	MessageBeep(MB_ICONEXCLAMATION);
	MsiSetProperty(hInstall, "INGRES_RUNNING", "1");

	bSilent=FALSE;
	dwSize=sizeof(szBuf);
	MsiGetProperty(hInstall, "UILevel", szBuf, &dwSize);
	if (!strcmp(szBuf, "2"))
		bSilent=TRUE;

	bCluster=FALSE;
	dwSize=sizeof(szBuf);
	MsiGetProperty(hInstall, "INGRES_CLUSTER_INSTALL", szBuf, &dwSize);
	if (!strcmp(szBuf, "1"))
		bCluster=TRUE;

	if (bSilent || 
		AskUserYN(hInstall, "Your Ingres instance is currently running. Would you like to shut it down now?"))
	{
		SetEnvironmentVariable("II_SYSTEM", ii_system);
		GetSystemDirectory(szBuf, sizeof(szBuf));
		sprintf(szBuf2, "%s\\ingres\\bin;%s\\ingres\\utility;%s", ii_system, ii_system, szBuf);
		SetEnvironmentVariable("PATH", szBuf2);
		
		if (!bCluster)
		{
			if (bSilent)
			{
				sprintf(szBuf, "\"%s\\ingres\\utility\\ingstop.exe\"", ii_system);
				ExecuteEx(szBuf);
			}
			else
			{
				sprintf(szBuf, "\"%s\\ingres\\bin\\winstart.exe\" /stop", ii_system);
				Execute(szBuf, TRUE);
			
				sprintf(szBuf, "Ingres - Service Manager [%s]", ii_id);
				while (FindWindow(NULL, szBuf))
					Sleep(1000);
			}
		}
		else
		{//take the cluster resource offline
			dwSize=sizeof(szBuf); *szBuf=0;
			MsiGetProperty(hInstall, "INGRES_CLUSTER_RESOURCE", szBuf, &dwSize);

			hCluster = OpenCluster(NULL);
			if (hCluster)
			{
				mbstowcs(lpwResourceName, szBuf, 256);
				hResource = OpenClusterResource(hCluster, lpwResourceName);
				if (hResource)
				{
					OfflineClusterResource(hResource);
					CloseClusterResource(hResource);
				}
			}
			while (IngresAlreadyRunning(ii_system))
				Sleep(1000);
		}

		if(!IngresAlreadyRunning(ii_system))
			MsiSetProperty(hInstall, "INGRES_RUNNING", "0");
	}
	else
	{
		AppendToLog("Install was aborted due to the fact that installation is still running!\r\n");
		TellUser(hInstall, "Aborting Patch Installation!");
		return (ERROR_INSTALL_FAILURE);
	}
	
	return ERROR_SUCCESS;
}

/*
**  Name: IngresAlreadyRunning
**
**  Description:
**	Check if Ingres identified by II_SYSTEM is already running.
**
**  Returns:
**	 0	Ingres is not running
**	 1	Ingres is running
**
**  History:
**	10-dec-2001 (penga03)
**	    Created.
*/
int
IngresAlreadyRunning(char *installpath)
{
	char file2check[MAX_PATH], temp[MAX_PATH];
	
	sprintf(file2check, "%s\\ingres\\files\\config.dat", installpath);
	if (GetFileAttributes(file2check) == -1)
		return 0;
	
	sprintf(file2check, "%s\\ingres\\bin\\iigcn.exe", installpath);
	if (GetFileAttributes(file2check) != -1)
	{
		sprintf(temp, "%s\\ingres\\bin\\temp.exe", installpath);
		CopyFile(file2check, temp, FALSE);
		if (DeleteFile(file2check))
		{
			/* iigcn is not running */
			CopyFile(temp, file2check, FALSE);
			DeleteFile(temp);
			return 0;
		}
		else
		{
			/* iigcn is running */
			DeleteFile(temp);
			return 1;
		}
	}
	else
	{
		/* iigcn.exe does not exist.*/
		return 0;
	}
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
	sprintf(lpCaption, "%s - InstallShield Wizard", buf);
	
	hWnd=FindWindow(NULL, lpCaption);
	return ((MessageBox(hWnd, lpText, lpCaption, MB_YESNO|MB_ICONQUESTION)==IDYES));
}

/*
**	Name: TellUser
**
**	Description: Responsible for popup box informing user 
**               of a message specified.
**	
**  Parameters: hInstall - Handle to MSI database.
**				lpText - Message to the user.
**  
**  Return code: 1 - User clicked OK
**               0 - Otherwise
*/
BOOL
TellUser(MSIHANDLE hInstall, char *lpText)
{
	char buf[MAX_PATH+1], lpCaption[512];
	DWORD nret;
	
	nret=sizeof(buf);
	MsiGetProperty(hInstall, "ProductName", buf, &nret);
	sprintf(lpCaption, "%s - InstallShield Wizard", buf);
	
	hWnd=FindWindow(NULL, lpCaption);
	return ((MessageBox(hWnd, lpText, lpCaption, MB_OK)==IDOK));
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






