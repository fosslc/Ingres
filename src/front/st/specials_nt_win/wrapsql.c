#include	<windows.h>
#include	<winbase.h>

#include	<conio.h>
#include	<compat.h>
#include        <cv.h>
#include        <pc.h>
#include        <ci.h>
#include        <me.h>
#include        <nm.h>
#include        <st.h>
#include        <ex.h>
#include        <er.h>
#include        <gl.h>
#include        <sl.h>
#include        <iicommon.h>
#include        <fe.h>
#include        <ug.h>

#include	<dbms.h>

/*
**	This program is a wrapper for tm to allow it to be run as sql by
**	clicking an icon.  This wrapper prompts for the database name
**	and stays in the same window.
**
**  History:
**	03-may-94 (johnr)
**	    This is a new module for the NT to allow sql (tm) to be run 
**	    by clicking an icon and prompting the user for a database name.
**	13-may-94 (johnr)
**	    Added a blank after sysmod.exe so that the database name would
**	    not be concatenated with the exe.  Also forced it to look for
**	    wrapsql instead of wrapsmod in utexe.def.
**	20-may-94 (johnr)
**	    Tested for II_SYSTEM and tested for exstance of wrapped exe.
**	24-nov-1997 (canor01)
**	    Allow quoted pathnames for executable to support embedded
**	    spaces. Cleaned up compiler warnings.
**	09-aug-1999 (mcgem01)
**	    Changed nat to i4.
*/

DWORD	dwGlobalError;

#undef main

i4
main(argc, argv)
i4	argc;
char	*argv[];
{
	ARGRET	rarg;
	i4	pos;
	TCHAR	tchBuf[_MAX_PATH];
	TCHAR	tchII_SYSTEM[_MAX_PATH];

	char	*tchDBname;
	BOOL	isstarted;
	PROCESS_INFORMATION	piProcInfo;
	STARTUPINFO		siStartInfo;
	DWORD	dwWait;
	CHAR	*ptr;
	CHAR	szPath[256];
	OFSTRUCT	ofstruct;
	HFILE	hFile;
	DWORD   wait_status;
	CHAR    c;

	NMgtAt("II_SYSTEM", &ptr);
	if (!(ptr && *ptr))
	    {
	    printf("II_SYSTEM can not be determined. Please check your installation.\n");
	    PCexit(FAIL);
	    }
	strcpy( szPath, ptr );
	strcat( szPath, "\\ingres\\bin\\wrapsql.exe" );
	hFile = OpenFile( szPath, &ofstruct, OF_EXIST );
	if ( hFile == HFILE_ERROR ) 
	    {
	    printf("Can't find wrapsql.exe.  Check the setting for II_SYSTEM.\n");
	    PCexit(FAIL);
	    } 
	if (FEutaopen(argc, argv, ERx("wrapsql")) != OK)
		PCexit(FAIL);

	if(FEutaget(ERx("database"), 0,FARG_PROMPT, &rarg, &pos) == OK)
		tchDBname = rarg.dat.name;

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.lpReserved = NULL;
	siStartInfo.lpDesktop = NULL;
	siStartInfo.lpTitle = NULL;
	siStartInfo.lpReserved2 = NULL;
	siStartInfo.cbReserved2 = 0;
	siStartInfo.dwFlags = 0;

	GetEnvironmentVariable("ii_system", tchII_SYSTEM, _MAX_PATH);

	/* quote the executable name if there are embedded spaces */
	if ( STindex( tchII_SYSTEM, " ", 0 ) != NULL )
	{
	    strcpy( tchBuf, "\"" );
	    strcat( tchBuf, tchII_SYSTEM );
	    strcat( tchBuf, "\\ingres\\bin\\tm.exe\" -qsql " );
	}
	else
	{
	    strcpy( tchBuf, tchII_SYSTEM );
	    strcat( tchBuf, "\\ingres\\bin\\tm.exe -qsql " );
	}
	strcat( tchBuf, tchDBname );

	isstarted = CreateProcess(
			NULL,		/* image path */
			tchBuf,		/* command line arguments	*/
			NULL,
			NULL,
			TRUE,
			NORMAL_PRIORITY_CLASS,
			NULL,
			NULL,
			&siStartInfo,
			&piProcInfo);

	dwGlobalError = GetLastError();

	dwWait = WaitForSingleObject(
			piProcInfo.hProcess,
			INFINITE);

	GetExitCodeProcess(piProcInfo.hProcess, &wait_status);
	if (wait_status) 
	{
		printf("\nPress a key...\n");
		c = getch();
	}

	PCexit(0);
	/*NOTREACHED*/
	return (0);
}
