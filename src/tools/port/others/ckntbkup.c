/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**
**  Wrapper Utility for NTBACKUP, used by ckpdb, rollforwarddb
**
**  Description:
**
**      This utility executes the NT ntbackup command, optionally
**      taking a list of files. It is used in the cktmpl.def file
**      as a replacement for ckcopyt.bat. Ckcopyt had the limitation
**      of not being able to communicate appl errors back to
**      the originating program (dmfjsp).
**
**      The syntax as used in cktmpl.def is:
**
**          ckntbckup %C %N %D BACKUP
**          ckntbckup %C %N %D BACKUP PARTIAL %B
**          ckntbckup %C %N %D RESTORE
**          ckntbckup %C %N %D RESTORE PARTIAL %B
**
**          %C  Tape Device
**          %N  Location number
**          %D  Database directory
**          %B  List of individual table files
**
**  Examples of passed parameters:
**
**      ckntbckup 0
**                1
**                d:\oping\ingres\ckp\default\mydb
**                BACKUP
**
**      ckntbckup 0
**                1
**                d:\oping\ingres\ckp\default\mydb
**                RESTORE PARTIAL
**                aaaaaamp.t00 aaaaaalb.t00
**
**  History:
**
**      01-May-96 (sarjo01)
**          Created.
**      21-May-97 (musro02)
**          Made compilation conditional on NT_GENERIC
**      29-Jul-97 (merja01)
**          Added preproccesor stmt for NT_GENERIC to prevent compile     
**          errors on unix builds.
**	26-sep-97 (mcgem01)
**	    Removed extraneous ifdef.
**	15-nov-1997 (canor01)
**	    Ntbackup does not support filenames with embedded spaces, so
**	    must translate into the "short" filename.
**      15-mar-2001 (rigka01) bug#103755
**	    ntbackup command on Windows 2000 is different so check for
**	    OS and then use appropriate parameters for the command
**	    problem occurs with checkpoint to tape
**
*/
#ifdef NT_GENERIC  /* Only for NT_GENERIC */

#include <compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gl.h" 

#define	MAJOR_VERSION_WINDOWS_NT 4

void syntax();
int  docommand(char *cmdline);

int main(int argc, char *argv[])
{
    int     partial = 0;
    int     stat = 0;
    int     tblcount, i, j;
    char    *location, *oper, *tapedev, *dbdir;
    char    cmdbuff[1024];
    char    altdir[_MAX_PATH];

    if (argc < 5)
        syntax();

    tapedev  = argv[1];
    location = argv[2];
    dbdir    = argv[3];
    oper     = argv[4];

    if (stricmp(oper, "restore") != 0 &&
        stricmp(oper, "backup")  != 0)
        syntax();
    if (argc > 5)
    {
        if (argc < 7 || stricmp(argv[5], "partial") != 0)
            syntax();
        partial = 1;
        tblcount = argc - 5;
    }

    if ( strchr( dbdir, ' ' ) != NULL )
    {
	if ( GetShortPathName( dbdir, &altdir, sizeof(altdir) ) > 0 )
	    dbdir = altdir;
    }

    if (partial == 0)
    {
	OSVERSIONINFO lpvers;
        if (stricmp(oper, "backup") == 0)
        {
	    lpvers.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	    GetVersionEx(&lpvers);
	    if (lpvers.dwMajorVersion > MAJOR_VERSION_WINDOWS_NT)
                sprintf(cmdbuff,
                        "ntbackup backup %s /a /d "
                        "\"CKPDB Loc: %s\" /m normal /t "
		        "\"%s Location %s", 
                        dbdir, location, SYSTEM_PRODUCT_NAME, location
                       );
	    else
                sprintf(cmdbuff,
                        "ntbackup backup %s /a /d "
                        "\"CKPDB Loc: %s\" /T NORMAL /tape:%s",
                        dbdir, location, tapedev
                       );
	}
        else
            sprintf(cmdbuff,
                    "ntbackup"
                   );
        if (lpvers.dwMajorVersion > MAJOR_VERSION_WINDOWS_NT)
            printf("\nMount tape named: %s Location %s\n"
                   "Type any key to continue...", 
		   SYSTEM_PRODUCT_NAME, location);
        else
            printf("\nMount tape for location %s\n"
                   "Type any key to continue...", location);
        getch();
        printf("\n");
        stat = docommand(cmdbuff);
    }
    else  // partial
    {
        exit(1); // not supported yet
    }
    exit(stat);
}
void syntax()
{
    printf("Usage: ckxcopy <db-path> <ckp-path>\n"
           "               { RESTORE | BACKUP } [ PARTIAL file1 file2 ... ]\n");
    exit(1);
}
int docommand(char *cmdline)
{
    int   status, wait_status;
    PROCESS_INFORMATION ProcInfo;
    SECURITY_ATTRIBUTES child_security;
    STARTUPINFO   StartUp;

    memset(&StartUp, '\0', sizeof(StartUp));
    memset(&child_security, '\0', sizeof(child_security));

    StartUp.cb = sizeof(StartUp);

    child_security.nLength = sizeof(child_security);
    child_security.bInheritHandle = FALSE;

    status = CreateProcess( NULL,
                            cmdline,
                            &child_security,
                            &child_security,
                            TRUE,
                            NORMAL_PRIORITY_CLASS,
                            NULL,
                            NULL,
                            &StartUp,
                            &ProcInfo );

    if (status != TRUE)
    {
        status = GetLastError();
        TerminateProcess(ProcInfo.hProcess, 0);
    }

    if (status == TRUE)
    {
        wait_status = WaitForSingleObject(ProcInfo.hProcess, INFINITE);
        status = GetExitCodeProcess(ProcInfo.hProcess, &wait_status);
        if (status != TRUE)
        {
            status = GetLastError();
        }

        if (status == TRUE)
            status = 0;
    }

    return (status || wait_status) ? 1 : 0;
}
# else
int main();
#endif /* NT_GENERIC */
