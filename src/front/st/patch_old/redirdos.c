#include <windows.h>
#include <stdio.h>

/*
**  Name: conspawn.c
**
**  Description:
**	Due to differences between Windows 95 and Windows NT platforms,
**	special steps must be taken to allow the same code to redirect
**	the output of MS-DOS applications and batch files on both platforms.
**	This file builds a hidden Win32 console application which acts as
**	an interface between the parent W32 application and the MS-DOS child.
**	Without this level of indirection, the redirected pipe of a MS-DOS
**	application will not close and the parent will hang. (on the 
**	WaitForSingleObject(Process, INFINITE)).
**
**	Note that the name of the MS-DOS application or batch file is included
**	as the second parameter of this method (argv[1])
**
**  History:
**	12-Mar-1999 (andbr08)
**	    Created.
*/

void main (int argc, char *argv[])
{
	BOOL			CreationValid	= FALSE;
	STARTUPINFO		si				= {0};
	PROCESS_INFORMATION	pi			= {0};

	/*Make child process use this app's standard files.*/
	si.cb = sizeof(si);
	si.dwFlags		= STARTF_USESTDHANDLES;
	si.hStdInput	= GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput	= GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError	= GetStdHandle(STD_ERROR_HANDLE);

	CreationValid = CreateProcess(NULL, argv[1], NULL, NULL, TRUE, 0,
						NULL, NULL, &si, &pi);
	if(CreationValid)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle (pi.hProcess);
		CloseHandle (pi.hThread);
	}
}
