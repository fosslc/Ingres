/******************************************************************************
**
**	Copyright (c) 1983, 1991 Ingres Corporation
**
******************************************************************************/

#include	<compat.h>
#include	<pc.h>
#include	<wn.h>
#include	<cv.h>

# define        MAXFUNCS    32

#ifdef xde
#define	STATICDEF       static
#define CLEAR_ERR(s) \
    { \
        (s)->intern = ((u_i4) 0); \
        (s)->callid = ((u_i4) 0); \
        (s)->errnum  = ((nat)  0); \
    }
#endif

STATICDEF	i4      PCexiting = 0;
STATICDEF	bool    bPChelpCalled = FALSE;
STATICDEF	VOID    (*PCexitfuncs[MAXFUNCS])() = {0};
STATICDEF	VOID    (**PClastfunc)() = NULL;
STATICDEF	char	szProgram[_MAX_PATH+10] = "INGRES";
STATICDEF	bool    isOpenROAD_PM;
STATICDEF	i4      isOpenROADCount = 0;

GLOBALREF	HWND	hWndStdout;		/* valerier, A25 */
/******************************************************************************
**	Name:
**		PCexit.c
**
**	Function:
**		PCexit
**
**	Arguments:
**		i4	status;
**
**	Result:
**		Program exit.
**
**		Status is value returned to shell or command interpreter.
**
**	Side Effects:
**		None
**
**
**	History:
**	 ?	Initial version
**	17-dec-1996 (donc)
**	   Melded the OpenINGRES PCexit with the OpenROAD version. Created
**	   a version which allows the former, character-based, behavior to 
**	   co-exist with a Windows/GUI behavior. 
**	25-may-97 (mcgem01)
**	   Cleaned up compiler warnings.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	8-dec-1999 (crido01) Bug 99701
**	   Add isOpenROADCount static.  It keeps track of how many times
**	   OpenROAD calls PCpinit.  If PCPinit is called more than once, do
**	   not show popup message via PCexit_or when shutting down.   
**	09-Apr-2002 (yeema01) Bug 107276, 105960
**	   Provided support for Trace Window.
**	   Added the #define in order to build this file on Unix with Mainwin.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	19-Nov-2010 (bonro01) Bug 124685
**	    Update PCatexit return type to VOID to match new prototype.
**
******************************************************************************/
VOID
PCexit(i4  status)
{
    VOID PCexit_or();

    if (!isOpenROAD_PM)
        exit(status);
    else
	PCexit_or( status );
}

/******************************************************************************
**
**
**
******************************************************************************/
VOID
PCatexit( func )
VOID (*func)();
{
    VOID PCatexit_or();

    if (!isOpenROAD_PM)
	atexit(func);
    else
	PCatexit_or( func );
}



VOID
PCexit_or(i4 status)
{
	register VOID   (**f)() = PClastfunc;

	/* 
	** A19 - put in popup if exiting with bad status
	*/
	/* valerier, A25 - Get rid of message box for console apps. */
	if (isOpenROADCount < 2 )
	{
	   if (hWndStdout && status && (status != OK_TERMINATE))
	   {
		GLOBALREF HWND hWndDummy;

		ShowWindow( hWndStdout, SW_SHOWNORMAL );
		strcat(szProgram, " exiting");
		MessageBox(hWndDummy, szProgram, NULL, MB_APPLMODAL);
	   }
	   if (bPChelpCalled)
		WinHelp(NULL, (LPCTSTR)NULL, HELP_QUIT, (DWORD)NULL);
        }

	if (!PCexiting && f)
	{
		/* avoid recursive calls */
		PCexiting = 1;

		/* execute all exit functions */
		while (f-- != PCexitfuncs)
			(**f)();
	}

	if (isOpenROADCount < 2 )
	{
	   SetCursor(LoadCursor(NULL, IDC_ARROW));
	   if (hWndStdout)
		DestroyWindow(hWndStdout);	// This avoids a bug in Win32s version 1.2
	}
	exit(status);
}


/*
**  Name:   PCatexit - set exit function in the function table
**
**  History:
**      15-mar-89 (puree)
**          Rollover from 6.1 DOS version
*/

VOID
PCatexit_or(
VOID    (*func)())
{
	if (!PClastfunc)
		PClastfunc = PCexitfuncs;
    if (PClastfunc < &PCexitfuncs[MAXFUNCS])
		*PClastfunc++ = func;
}

VOID
PCsaveModuleName(
char *	szPath)
{
	char *	p;
	
	for (p = szPath + strlen(szPath); --p > szPath; )
	{
		if ((*p == '\\') || (*p == ':'))
		{
			++p;
			break;
		}
	}
	strcpy(szProgram, p);
	CVupper(szProgram);
}

VOID
PChelpCalled(
i4  	command, 
BOOL	status)
{
	if (status)
		bPChelpCalled = (command != HELP_QUIT) ? TRUE : FALSE;
}

VOID 
PCpinit()
{
	isOpenROAD_PM = TRUE;
	isOpenROADCount++;
}
bool
PCgetisOpenROAD_PM()
{
	return isOpenROAD_PM;
}
