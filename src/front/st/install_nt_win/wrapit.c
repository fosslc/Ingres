#include	<windows.h>
#include	<winbase.h>

#include	<compat.h>
#include        <cv.h>
#include        <pc.h>
#include        <ci.h>
#include        <me.h>
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
**  This program is a generic wrapper that allows most console programs to
**  be called from an icon.  It solves the problem of the program console
**  disappearing before the user gets a chance to see what happened.  Written
**  originally for Ingstart and Ingstop, which would fail but not give the
**  user a chance to see why.
**
**  wrapit also resizes the buffer, allowing the user to scroll back through
**  previous information, and adds some color to an otherwise drab looking
**  console.
**
**  History:
**
**	31-aug-95 (tutto01)
**		Created.
**	10-may-97 (mcgem01)
**		Changed product name to OpenIngres.
**	09-aug-1999 (mcgem01)
**		Changed nat to i4.
*/

void cls(HANDLE hConsole);
DWORD	dwGlobalError;

#undef main

i4
main(argc, argv)
i4	argc;
char	*argv[];
{
	ARGRET	rarg;
	i4	pos;
	i4	i;
	TCHAR	tchBuf[_MAX_PATH];
	TCHAR	tchII_SYSTEM[_MAX_PATH];

	char	*tchDBname;
	LPTSTR	lpszOldValue;
	BOOL	isstarted;
	PROCESS_INFORMATION	piProcInfo;
	STARTUPINFO		siStartInfo;
	DWORD	dwWait;
	CHAR	*ptr;
	CHAR	szPath[256];
	CHAR	szProg[256];
	CHAR	szParms[256];
	OFSTRUCT	ofstruct;
	HFILE	hFile;
	DWORD   wait_status;
	CHAR    c;
	HANDLE	hConsole;
	COORD	coords;

        if (argc < 3)
        {
            SIprintf ("usage: wrapit <application title> <executable>\n");
            SIprintf ("\t<application title> is the title to be displayed.\n");
            SIprintf ("\t<executable> is the executable or batch file to be run.\n");
            PCexit (FAIL);
        }

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole,BACKGROUND_BLUE | FOREGROUND_GREEN |
		FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
	cls(hConsole);
        coords.X = 80;  coords.Y = 1000;
        SetConsoleScreenBufferSize(hConsole, coords);
	STcopy("OpenIngres ", szProg);
	STcat(szProg, argv[1]);
        SetConsoleTitle(szProg);

	NMgtAt("II_SYSTEM", &ptr);
	if (!(ptr && *ptr))
	    {
	    printf("II_SYSTEM can not be determined. Please check your installation.\n");
	    PCexit(FAIL);
	    }

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.lpReserved = NULL;
	siStartInfo.lpDesktop = NULL;
	siStartInfo.lpTitle = NULL;
	siStartInfo.lpReserved2 = NULL;
	siStartInfo.cbReserved2 = 0;
	siStartInfo.dwFlags = 0;

	lpszOldValue = GetEnvironmentVariable("ii_system", tchII_SYSTEM, _MAX_PATH);

	STcopy("\0", szParms);
	for (i=2 ; i<argc; i++)
	{
		STcat(szParms, argv[i]);
		STcat(szParms, " ");
	}
	STcat(szParms, "\0");
	strcpy( tchBuf, szParms);

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
		if (dwGlobalError) printf("\nUnsuccessful program execution... exit code %d\n", dwGlobalError);
		printf("\nPress any key to continue...\n");
		getch(c);
	}
	else
	{
		printf("\nPress any key to continue...\n");
		getch(c);
	}
	exit;
}



/************************************************************************
* FUNCTION: cls(HANDLE hConsole)                                        *
*                                                                       *
* PURPOSE: clear the screen by filling it with blanks, then home cursor *
*                                                                       *
* INPUT: the console buffer to clear                                    *
*                                                                       *
* RETURNS: none                                                         *
*************************************************************************/

void cls(HANDLE hConsole)
{
  COORD coordScreen = { 0, 0 }; /* here's where we'll home the cursor */
  BOOL bSuccess;
  DWORD cCharsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
  DWORD dwConSize; /* number of character cells in the current buffer */

  /* get the number of character cells in the current buffer */
  bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
  dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
  /* fill the entire screen with blanks */
  bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ',
      dwConSize, coordScreen, &cCharsWritten);
  /* get the current text attribute */
  bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
  /* now set the buffer's attributes accordingly */
  bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
      dwConSize, coordScreen, &cCharsWritten);
  /* put the cursor at (0, 0) */
  bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
  return;
}

