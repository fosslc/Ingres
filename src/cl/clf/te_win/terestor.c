# include       <fcntl.h>
# include       <dos.h>
# include       <stdlib.h>
# include       "telocal.h"
# include       <cv.h>
# include       <me.h>
# include       <pc.h>
# include       <te.h>
# include       <nm.h>
# include       <cs.h>

GLOBALREF  bool TEcmdEnabled;
GLOBALREF  bool TEstreamEnabled;

/*
** TErestore
**      Restore terminal characteristics.
**
**      Restore the terminal to either forms or normal
**      state depending on the value of "which" (TE_NORMAL or TE_FORMS).
**      This will be a no-op for VMS and for IBM/PC
**
** Arguments:
**      i1      which;
** Returns:
**      OK - if the terminal state was restored.
**      FAIL - if we could not restored the terminal state or
**              if there was no previous call to TEsave or
**              if a call to TEsave was unsuccessful.
** History:
**      11-Jun-96 (fanra01)
**          Modified restoration of TE_FORMS to setup the standard input
**          handle.
**      23-aug-1996 (wilan06)
**          removed redundant include of wn.h
**
**      30-Oct-96 (fanra01)
**          Modified use of GetStdHandle to use a CreateFile to get handle to
**          console input buffer.  The SetConsoleMode call would fail with
**          invalid handle if the input had been redirected.
**          Also tried to tidy spacing.
**	24-Jun-97 (somsa01)
**	    Added the FILE_SHARE_READ flag to the CreateConsoleScreenBuffer()
**	    primarily so that processes which inject threads into front-end
**	    processes will be able to read our multiple screen buffers (such
**	    as the Pragma telnet daemon server).
**	26-jun-97 (kitch01)
**	    Bug 79761/83247. Upfront cannot comunicate with OI on NT. This is 
**	    because TEcmdMode is missing from TErestore. I have added this
**	    The only thing required is to set globaldef TEcmdEnabled to true.
**	    This is used in TEwrite.
**	11-jul-97 (somsa01)
**	    When setting the terminal type back to TE_NORMAL, we needed to
**	    also reset the ENABLE_LINE_INPUT and ENABLE_ECHO_INPUT flags if
**	    hTEconsoleInput was not NULL; this meant that the stdin console
**	    was modified (Bug #83734).
**	20-jan-98 (kitch01) - cross integrated from 1.2 to 2.0 by (rigka01)
**		Bug 88301. Upfront fails to display error messages that have embedded LF
**		This is because MS runtime write routines convert LF to CRLF. I have
**		amended TEcmdMode to change stdout to binary mode writes. Also as MWS may
**		switch to TEforms mode if TEcmdEnabled is set I reset stdout to text mode
**		writes.
**	21-feb-1999 (andbr08)
**	    Bug# 95339.  Restoration of terminal fails on windows 95.  Now store and 
**	    restore the previous Console's output and input handle on Win 95 only.
**	    Also added a check to see if Win 95.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add handling of new TE_IO_MODE for direct streams access.
**	28-jun-2001 (somsa01)
**	    For OS version, use GVosvers().
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      01-aug-2007 (horda03) Bug 94283
**          Allow users to specify the size of the cursor in an ABF application.
*/

/*
** The following fields are for saving the terminal state
** information so we can restore a user's terminal after
** our front-ends exit.  They are declared here so only
** the routines TEsave and TErestore will be able to
** use them.  Boolean TE_saveok is a flag indicating that
** a called to TEsave was successful.
*/

STATUS
TErestore(
i4      which)
{
DWORD	dwErrCode;
DWORD   dwStdInMode;
SECURITY_ATTRIBUTES  sa;
DWORD   dwModeMask = ~(ENABLE_LINE_INPUT |
                       ENABLE_ECHO_INPUT);
BOOL	Is_w95 = FALSE;
i4 cur_size;
char *size;
OSVERSIONINFO lpvers;


    iimksec(&sa);

    if (TEmode == which)
        return OK;

    TEok2write = OK;
    lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
    GetVersionEx(&lpvers);
    Is_w95=(lpvers.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)? TRUE: FALSE;
    switch (which)
    {
        case TE_FORMS:
        {
            if (!hTEconsole)
            {
		if(Is_w95)
                {
		    hTEprevconsole = GetStdHandle(STD_OUTPUT_HANDLE);
		    hTEprevconsoleInput = GetStdHandle(STD_INPUT_HANDLE);
                }
                if ((( hTEconsole = CreateConsoleScreenBuffer(
                                        GENERIC_WRITE | GENERIC_READ,
                                        FILE_SHARE_WRITE,
                                        &TEsa,
                                        CONSOLE_TEXTMODE_BUFFER,
                                        NULL)) == INVALID_HANDLE_VALUE)
                    || (SetConsoleMode( hTEconsole,
                                        ENABLE_PROCESSED_OUTPUT) == FALSE)
                    || (GetConsoleScreenBufferInfo( hTEconsole,
                                                    &TEcsbi) == FALSE)
                    || (GetConsoleCursorInfo(hTEconsole, &TEcci) == FALSE)
                    || ((hTEconsoleInput=CreateFile(
                                            "CONIN$",
                                            GENERIC_READ|GENERIC_WRITE,
                                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                                            &sa,
                                            OPEN_EXISTING,
                                            0,
                                            NULL)) == INVALID_HANDLE_VALUE)
                    || ((GetConsoleMode( hTEconsoleInput,
                                         &dwStdInMode)) == FALSE)
                    || (SetConsoleMode(hTEconsoleInput,
                                       (dwStdInMode & dwModeMask)) == FALSE))
                {
                    dwErrCode = GetLastError();
                    return FAIL;
                }

                NMgtAt( "II_CURSOR_SIZE", &size);

                if (size && *size &&
                    (CVal(size, &cur_size) == OK) &&
                    (cur_size >= 1)  &&
                    (cur_size <= 100) )
                {
                   TEcci.dwSize = cur_size;
                }
                SetConsoleCursorInfo(hTEconsole, &TEcci);
            }
	/* Reset stdout to text mode if TEcmdEnabled */
	if (TEcmdEnabled)
	{
		if (_setmode( _fileno( stdout ), _O_TEXT ) == -1)
			return FAIL;
		TEcmdEnabled = FALSE;
                TEstreamEnabled = FALSE;
	}
            break;
        }
		
        case TE_NORMAL:
        {
        COORD coordChar;
			
            coordChar.X = TEcols - 1;
            coordChar.Y = TErows - 1;
            SetConsoleCursorPosition(hTEconsole, coordChar);
            SetConsoleTextAttribute(hTEconsole, TEcsbi.wAttributes);
	    if (hTEconsoleInput)
	    {
		/*
		** Since the ENABLE_LINE_INPUT and ENABLE_ECHO_INPUT were
		** taken off in TE_FORMS, the normal mode needs to reset
		** these flags in order to properly re-enable console input.
		*/
		GetConsoleMode(hTEconsoleInput, &dwStdInMode);
		SetConsoleMode(hTEconsoleInput, dwStdInMode | ~dwModeMask);
	    }
            CloseHandle(hTEconsole);
            hTEconsole = 0;
            if (Is_w95)
            {
                SetConsoleActiveScreenBuffer(hTEprevconsole);
                FlushConsoleInputBuffer(hTEprevconsoleInput);
            }

	/* Reset stdout to text mode if TEcmdEnabled */
	if (TEcmdEnabled)
	{
		if (_setmode( _fileno( stdout ), _O_TEXT ) == -1)
			return FAIL;
		TEcmdEnabled = FALSE;
                TEstreamEnabled = FALSE;
	}
            break;
        }

        case TE_IO_MODE:
            TEstreamEnabled = TRUE;
            /*
            ** Intended fall through of case statement
            */
	case TE_CMDMODE:
	{
	    /* Bug 88301 Set "stdout" to have binary mode: */
	    if (_setmode( _fileno( stdout ), _O_BINARY ) == -1)
		return FAIL;
	    TEcmdEnabled = TRUE;
	    break;
	}

        default:
            return FAIL;
    }
    bNewTEconsole = TRUE;
    TEmode = which;
    return OK;
}

/*
** TEgetmode
**  return value of TEmode.
*/

i2
TEgetmode(void)
{
    return(TEmode);
}

#ifdef	TESAVE
/*
** TEsave
**      Saves the terminal state so we can restore it
**      when the user exits from one of the front-ends.
**      This will be a no-op for VMS.
**
**      This routine is internal to the TE routines and
**      is not to be used outside the compatibility library.
**
** Returns:
**      OK - if the terminal state was saved.
**      FAIL - if we could not save the terminal state.
*/

STATUS
TEsave(void)
{
        return(OK);
}
#endif
