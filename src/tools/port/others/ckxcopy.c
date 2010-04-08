/*
**  Copyright (c) 1996, 2002 Ingres Corporation
*/

/*
**  Name: ckxcopy - wrapper utility for XCOPY, used by ckpdb, rollforwarddb
**
**  Description:
**      This utility executes the NT xcopy command, optionally
**      taking a list of files. It is used in the cktmpl.def file
**      as a replacement for ckcopyd.bat. Ckcopyd had the limitation
**      of not being able to communicate an xcopy error (like "out of
**      disk space") back to the originating program (dmfjsp).
**
**      The syntax as used in cktmpl.def is:
**
**          ckxcopy %D %A BACKUP
**          ckxcopy %D %A BACKUP PARTIAL %B
**          ckxcopy %D %A RESTORE
**          ckxcopy %D %A RESTORE PARTIAL %B
**
**          %D  Database directory
**          %A  Checkpoint directory
**          %B  List of individual table files
**
**  Examples of passed parameters:
**
**      ckxcopy d:\oping\ingres\data\default\mydb
**              d:\oping\ingres\ckp\default\mydb\c0001001.ckp
**              BACKUP
**
**      ckxcopy d:\oping\ingres\data\default\mydb
**              d:\oping\ingres\ckp\default\mydb\c0002002.ckp
**              RESTORE PARTIAL
**              aaaaaamp.t00 aaaaaalb.t00
**
**  History:
**      01-May-96 (sarjo01)
**          Created.
**      21-May-97 (musro02)
**          Made compilation conditional on NT_GENERIC
**      29-Jul-97 (merja01)
**          Added preproccessor stmt for NT_GENERIC to prevent compile 
**          errors on unix platforms.
**	26-sep-97 (mcgem01)
**	    Remove extraneous ifdef.
**	20-feb-1998 (somsa01)
**	    Since this executable is spawned from dmfjsp, the "Control C"
**	    handler which is setup by EX in dmfjsp will not pass through
**	    to here. We also do not want to allow the user to "Control C"
**	    out of this when it is running. Therefore, disable "Control C"
**	    processing from here as well as from children spawned from here.
**	15-nov-1997 (canor01)
**	    Must quote pathnames when passing them on command line to
**	    suppport embedded spaces.
**	27-apr-2000 (cucjo01)
**	    Added extra XCopy flag "/Y" to supress prompting if a file 
**	    exists in Windows 2000 since it will currently prompt you 
**	    if a file exists in the destination.  This behavior has
**	    changed since Windows NT 4.0.
**	15-may-2002 (somsa01)
**	    Added copy_files() function, which replaces the running of
**	    xcopy. This was done to properly take care of localization
**	    issues. Also, removed the docommand() function since it is
**	    not used anymore.
**    	22-Jun-2007 (drivi01)
**          On Vista, this binary should only run in elevated mode.
**          If user attempts to launch this binary without elevated
**          privileges, this change will ensure they get an error
**          explaining the elevation requriement.
**	 7-sep-2007 (hayke02)
**	    Call copy_files() for both full and partial checkpoints. Modify
**	    copy_files() to use DI functions (open, create, read, galloc,
**	    write, close) rather than CopyFile(). This prevents the copy
**	    locking the file. This change fixes bug 119090.
**	 1-nov-2007 (hayke02)
**	    Pass mode into copy_files(). Delete file first if a partial
**	    restore. This prevents GPF when attempting to write to the
**	    exisiting file. This change fixes bug 119391.
**      14-Apr-2008 (hanal04) Bug 120259
**          Add back code that prints the file names that are copied.
**          Code was mistakenly removed by the fix for bug 119391.
**	15-jul-2008 (hayke02)
**	    Improve performance by calling DIsense() to determine the number
**	    of 2K pages in the file, then call DIgalloc() once, then
**	    DIread()/DIwrite() in 63MB chunks (64MB and above failed in
**	    testing, configurable downwards using II_CKXCOPY_IO_SIZE), then
**	    a single DIread()/DIwrite() for the remainder. This change fixes
**	    bug 120605.
**	07-Dec-2009 (drivi01)
**	    Add SIflush after SIprintf in copy_files function to flush out
**	    the output and prevent fragmented print statements.
*/
#ifdef NT_GENERIC  /* only build for NT_GENERIC */

#include <compat.h>
#include <di.h>
#include <cm.h>
#include <si.h>
#include <st.h>
#include <stdio.h>
#include <string.h>
#include <ep.h>
#include <nm.h>
#include <cv.h>

# define PAGE_SIZE 2048
# define MEGABYTE 1048576
# define IO_SIZE 63*MEGABYTE

void syntax();
int  copy_files(char *source, char *destination, int partial, int mode, char *filename);

int
main(int argc, char *argv[])
{
    int     partial = 0;
    int     stat = 0;
    int     mode = 0;  /* backup */
    int     tblcount, i, j;
    char    *src, *dest;

    /*
    ** Check if this is Vista, and if user has sufficient privileges
    ** to execute.
    */
    ELEVATION_REQUIRED();

    if (argc < 4)
        syntax();

    CMset_locale();

    SetConsoleCtrlHandler(NULL, TRUE);

    if (stricmp(argv[3], "restore") == 0)
    {
        mode = 1;
        src  = argv[2];
        dest = argv[1];
    }
    else if (stricmp(argv[3], "backup") != 0)
        syntax();
    else
    {
	/*
	** backup
	*/
        src  = argv[1];
        dest = argv[2];
    }

    if (argc > 4)
    {
        if (argc < 6 || stricmp(argv[4], "partial") != 0) 
            syntax();
        partial = 1;
        tblcount = argc - 5;
    }

    if (!partial)
	stat = copy_files(src, dest, partial, mode, NULL);
    else
    {
	/*
	** partial
	*/
        if (mode == 0)  /* backup */
	    CreateDirectory(dest, NULL);

        for (i = 0, j = 5; i < tblcount; i++, j++) 
	    stat = copy_files(src, dest, partial, mode, argv[j]);
    }

    exit(stat);
}

void
syntax()
{
    printf("Usage: ckxcopy <db-path> <ckp-path>\n"
           "               { RESTORE | BACKUP } [ PARTIAL file1 file2 ... ]\n");
    exit(1);
}

int
copy_files(char *source, char *destination, int partial, int mode, char *filename)
{
    WIN32_FIND_DATA	FindFileData;
    HANDLE		hFind;
    char		src[1024], FullFilePath[1024];
    char		*fname, *buf, *string;
    bool		MoreData = TRUE;
    int			status = 0, NumFiles = 0;
    DI_IO		srcdir, destdir;
    CL_ERR_DESC		err_desc;
    i4			page_num, num_pages, num_reads, pages_left, io_count;
    SIZE_TYPE		io_size = IO_SIZE;

    STprintf(src, "%s\\*", source);
    if (!partial )
    {
	if ((hFind = FindFirstFile(src, &FindFileData)) == INVALID_HANDLE_VALUE)
	{
	SIprintf("Error = %d\n", GetLastError());
	SIflush(stdout);
	return(1);
	}
	fname = (char *)&FindFileData.cFileName;
    }
    else
	fname = filename;

    if (!partial && (GetFileAttributes(destination) == -1))
	CreateDirectory(destination, NULL);

    NMgtAt("II_CKXCOPY_IO_SIZE", &string);
    if (string && *string && !CVal(string, &io_size))
    {
	if (io_size * MEGABYTE < IO_SIZE)
	    io_size *= MEGABYTE;
	else
	    io_size = IO_SIZE;
    }

    buf = MEreqmem(0, io_size, FALSE, &status);
    if (status != OK)
    {
	SIprintf("Error = %d\n", GetLastError());
	SIflush(stdout);
	return(1);
    }

    while (MoreData)
    {
	if (partial || (STcompare(FindFileData.cFileName, ".") != 0 &&
	    STcompare(FindFileData.cFileName, "..") != 0))
	{
	    STprintf(FullFilePath, "%s\\%s", source, fname);
	    SIprintf("%s\n", FullFilePath);
	    SIflush(stdout);

	    CSinitiate(0,NULL,NULL);

	    status = DIopen(&srcdir, source, (u_i4) STlength(source),
		fname, (u_i4) STlength(fname),
		(i4) PAGE_SIZE, (i4) DI_IO_READ,
		(u_i4) DI_SYNC_MASK, &err_desc);
	    if (status != OK)
		break;

	    status = DIsense(&srcdir, &num_pages, &err_desc);
	    if (status != OK)
		break;

	    num_pages++;
	    num_reads = (num_pages * PAGE_SIZE)/io_size;
	    pages_left = ((num_pages * PAGE_SIZE) % io_size)/PAGE_SIZE;
	    if (partial && (mode == 1))
	    {
		/* partial backup, delete file first */
		DIdelete(&destdir, destination,
		    (u_i4) STlength(destination), fname,
		    (u_i4) STlength(fname), &err_desc);
	    }
	    status = DIcreate(&destdir, destination,
			(u_i4) STlength(destination), fname,
			(u_i4) STlength(fname), 
			(i4) PAGE_SIZE, &err_desc);
	    if (status == OK)
	    {
		status = DIopen(&destdir, destination,
				(u_i4) STlength(destination),
				fname,
				(u_i4) STlength(fname),
				(i4) PAGE_SIZE, (i4) DI_IO_WRITE,
				(u_i4) DI_SYNC_MASK, &err_desc);

		if (status != OK)
		    break;
	    }
	    else
		break;

	    page_num = 0;
	    status = DIgalloc(&destdir, num_pages, &page_num, &err_desc);
	    if (status != OK)
		break;

	    for (io_count = 0, page_num = 0; io_count < num_reads; io_count++)
	    {
		num_pages = io_size/PAGE_SIZE;

		status = DIread(&srcdir, &num_pages, page_num, buf, &err_desc);
		if (status != OK)
		    break;

		status = DIwrite(&destdir, &num_pages, page_num, buf, &err_desc);
		if (status != OK)
		    break;
		page_num += num_pages;
	    }
	    if (status != OK)
		break;
	    if (pages_left > 0)
	    {
		status = DIread(&srcdir, &pages_left, page_num, buf, &err_desc);
		if (status != OK)
		    break;

		status = DIwrite(&destdir, &pages_left, page_num, buf, &err_desc);
		if (status != OK)
		    break;
	    }

	    if (status == OK)
		NumFiles++;
	    (VOID) DIclose(&srcdir, &err_desc);
	    (VOID) DIclose(&destdir, &err_desc);
	}

	if (!partial)
	{
	    MoreData = FindNextFile(hFind, &FindFileData);
	    fname = (char *)&FindFileData.cFileName;
	}
	else
	    MoreData = FALSE;
    }

    SIprintf("%d File(s) copied\n", NumFiles);
    SIflush(stdout);
    return(status);
}
# else
int main();
#endif /* NT_GENERIC */
