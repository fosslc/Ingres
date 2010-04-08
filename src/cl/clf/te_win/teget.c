/*
** TEget -- get an input character
**
*/


#undef INCLUDE_ALL_CL_PROTOS

# include <compat.h>
# include <er.h>
# include <me.h>
# include <cv.h>
# include <nm.h>
# include <te.h>
# include <tc.h>
# include <cs.h>
# include "telocal.h"

# define BEL            0x07
# define ESC            0x1B
# define DEL            0x7F

/*
    History

	09-Dec-91 (kevinq)
	    Removed some unneeded code.  Changed behavior of timeouts.
	10-Dec-91 (kevinq) OS/2
	    removed D16REGS reference, and some other DOS stuff.
	    WARNING:  the Unix version handles exceptions.  May
	    need to consider that here.
        26-Sep-95 (fanra01)
            Added support for AT enhanced keyboards.
        27-Sep-95 (fanra01)
            Reverted to revision 1 of teget function.  The implementation of
            timeout reduces processor usage.
            Also added in the TEinflush function.
        28-Sep-95 (fanra01)
            Implemented a timeout on console input.  This prevents calling
            functions from over scheduling. eg FRS.
        18-Oct-95 (fanra01)
            Modified timeout not to use console handle.  This conflicts with
            the streams related functions.  Replaced with a sampling timeout
            code.
        11-Jun-96 (fanra01)
            b75225 Special german characters not displayed with ASCII based
                   tools.
            Changed TEget from using the provided streams input functions to
            lower level Win32 console input functions.
        12-Nov-96 (fanra01)
            Changed from using GetStdHandle to use a CreateFile to get console
            input buffer, as input may be redirected.  Also disabled processed
            input to allow ctrl-c to be passed back to application.
        05-Dec-96 (fanra01)
            Extended function key range check to include F11 and F12 keys.
            At present will return pf11 and pf12 codes respectively.
            SIR 45128.
        03-May-2001 (fanra01)
            Sir 104700
            Add handling for the TEstreamEnabled case for direct stream I/O.
	03-oct-2003 (somsa01)
	    For AMD64, turned off global optimization around TEget() to
	    stop CBF from not working properly when processing key events.
	20-Jul-2004 (lakvi01)
		SIR #112703, cleaned-up warnings.
        26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 


    teget maps keys as follows:

	Key                 Returned by teget       Ingres key name
	---                 -----------------       ---------------
	Shift-tab           Control-P

	Up arrow            \E[A
	Down arrow          \E[B
	Right arrow         \E[C
	Left arrow          \E[D

	F1                  \E[G                    pf1
	F2                  \E[H                    pf2
	F3                  \E[I                    pf3
	F4                  \E[J                    pf4
	F5                  \E[K                    pf5
	F6                  \E[L                    pf6
	F7                  \E[M                    pf7
	F8                  \E[N                    pf8
	F9                  \E[O                    pf9
	F10                 \E[P                    pf10

	Shift-F1            \E[Q                    pf11
	Shift-F2            \E[R                    pf12
	Shift-F3            \E[S                    pf13
	Shift-F4            \E[T                    pf14
	Shift-F5            \E[U                    pf15
	Shift-F6            \E[V                    pf16
	Shift-F7            \E[W                    pf17
	Shift-F8            \E[X                    pf18
	Shift-F9            \E[Y                    pf19
	Shift-F10           \E[Z                    pf20

	Alt-F1              \E[g                    pf21
	Alt-F2              \E[h                    pf22
	Alt-F3              \E[i                    pf23
	Alt-F4              \E[j                    pf24
	Alt-F5              \E[k                    pf25
	Alt-F6              \E[l                    pf26
	Alt-F7              \E[m                    pf27
	Alt-F8              \E[n                    pf28
	Alt-F9              \E[o                    pf29
	Alt-F10             \E[p                    pf30

			    \E[q                    pf31
			    \E[r                    pf32
			    \E[s                    pf33
	Escape              \E[t                    pf34
	Insert              \E[u                    pf35
	Delete              \E[v                    pf36
	Home                \E[w                    pf37
	End                 \E[x                    pf38
	PgUp                \E[y                    pf39
	PgDn                \E[z                    pf40

    Note that the escape sequences returned by teget must match the
    definitions in the termcap file.
*/

# define MAX_BUFFER         10
# define CTRL_MASK          ( LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | \
                              RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED | \
                              SHIFT_PRESSED )

# define ADD_TO_BUFFER(c)   {   if (nCharCount < MAX_BUFFER) \
                                { \
                                    aRetChars[nCharCount++] = c; \
                                } \
                            }

# define CTRL_KEY(c)        (c & ~(0x40))


GLOBALREF i4            TEstreamEnabled;
GLOBALREF int           IIte_use_tc;
GLOBALREF TCFILE        *IItc_in;


typedef struct readkey
{                                       /* structure is used for holding     */
                                        /* information required for building */
                                        /* TE mapped character sequences     */
    BOOL          blCmplt;              /* completed getting input sequence  */
    unsigned char aScanBuf[MAX_BUFFER]; /* storage area for built sequences  */
    int           nPos;                 /* index into storage area           */
    unsigned char ucChar;               /* character returned                */
    DWORD         dwLastCtrlState;      /* last keyboard control state       */
    WORD          wLastKey;             /* last virtual key code             */
} RKEY;


static BOOL blInitialised = FALSE;
static i4 nTrace = 0;
static unsigned char aRetChars[MAX_BUFFER];
                                        /* storage for returned characters   */
static int nCharCount = 0;              /* index into return storage area    */
static unsigned char * pRetChars = aRetChars;
                                        /* pointer for referencing storage   */

/*
** Name: DumpKeyInfo - Display contents of keyboard event
**
** Description:
**
**      Displays the contents of the KEY_EVENT_RECORD structure.
**      Function is called during key presses and key releases.
**
** Inputs:
**
**      pKeyEvent    Pointer to a KEY_EVENT_RECORD.  Structure defined in
**                   Win32.
**
** Outputs:
**      None
**
** Returns:
**      None
**
** History :
**      11-Jun-96 (fanra01)
**          Created.
*/

static VOID
DumpKeyInfo(KEY_EVENT_RECORD * pKeyEvent)
{

    if (!nTrace)
        return;

    SIfprintf(stdout,
              "KEY = [%s]  V_KEY = [%04X]  V_SCAN = [%04X]  CHAR = [%c,%04X]  ",
              pKeyEvent->bKeyDown ? "DOWN" : "UP  ",
              pKeyEvent->wVirtualKeyCode,
              pKeyEvent->wVirtualScanCode,
              pKeyEvent->uChar.AsciiChar,
              (unsigned char)pKeyEvent->uChar.AsciiChar);
    if (pKeyEvent->dwControlKeyState & CAPSLOCK_ON )
        SIfprintf(stdout,"CAPS  ");
    if (pKeyEvent->dwControlKeyState & ENHANCED_KEY )
        SIfprintf(stdout,"ENH   ");
    if (pKeyEvent->dwControlKeyState & LEFT_ALT_PRESSED )
        SIfprintf(stdout,"LALT  ");
    if (pKeyEvent->dwControlKeyState & LEFT_CTRL_PRESSED )
        SIfprintf(stdout,"LCTL  ");
    if (pKeyEvent->dwControlKeyState & NUMLOCK_ON )
        SIfprintf(stdout,"NUM   ");
    if (pKeyEvent->dwControlKeyState & RIGHT_ALT_PRESSED )
        SIfprintf(stdout,"RALT  ");
    if (pKeyEvent->dwControlKeyState & RIGHT_CTRL_PRESSED )
        SIfprintf(stdout,"RCTL  ");
    if (pKeyEvent->dwControlKeyState & SCROLLLOCK_ON )
        SIfprintf(stdout,"SCR   ");
    if (pKeyEvent->dwControlKeyState & SHIFT_PRESSED )
        SIfprintf(stdout,"SHIFT ");
    SIfprintf(stdout,"\n");
}

/*
** Name: MapKey - Transforms virtual keycodes for the numeric key pad
**
** Description:
**      Maps the virtual key codes for the numeric keypad to the ASCII number
**      representation.
**      Assumes that virtual key codes are different when the Num Lock is on.
**
** Inputs:
**      wVirtKey    virtual key code read from the keyboard event.
**
** Outputs:
**      None
**
** Returns:
**      ASCII representation of the numeric key press.
**      There is no error return.
**
** History :
**      11-Jun-96 (fanra01)
**          Created.
*/

static WORD
MapKey(WORD wVirtKey)
{
    if (wVirtKey == VK_CLEAR)
        wVirtKey = '5';

    if (wVirtKey == VK_INSERT)
        wVirtKey = '0';

    if ((wVirtKey >= VK_PRIOR) &&
        (wVirtKey <= VK_DOWN))
    {
        switch(wVirtKey)
        {
            case VK_END  :
                wVirtKey = '1';
                break;
            case VK_DOWN :
                wVirtKey = '2';
                break;
            case VK_NEXT :
                wVirtKey = '3';
                break;
            case VK_LEFT :
                wVirtKey = '4';
                break;
            case VK_RIGHT:
                wVirtKey = '6';
                break;
            case VK_HOME :
                wVirtKey = '7';
                break;
            case VK_UP   :
                wVirtKey = '8';
                break;
            case VK_PRIOR:
                wVirtKey = '9';
                break;
        }

    }
    if ((wVirtKey >= VK_NUMPAD0) &&
        (wVirtKey <= VK_NUMPAD9))
    {
        wVirtKey -= '0';
    }
    return(wVirtKey);
}

/*
** Name: ReadKeyboardInit - Initialise the console handle to be standard
**                          imput.
**
** Description:
**      Setup the console handle to be the standard input handle.
**      Also set the attributes to be not line input or echo.
**
** Inputs:
**      None.
**
** Outputs:
**      None
**
** Returns:
**      None
**
** History :
**      11-Jun-96 (fanra01)
**          Created.
**      12-Nov-96 (fanra01)
**          Changed from using GetStdHandle to use a CreateFile to get console
**          input buffer, as input may be redirected.  Also disabled processed
**          input to allow ctrl-c to be passed back to application.
*/

BOOL ReadKeyboardInit()
{
BOOL blStatus = FALSE;
BOOL blSuccess = FALSE;
DWORD dwStdInMode;
SECURITY_ATTRIBUTES  sa;

    iimksec(&sa);
    if ((!blInitialised) && (hTEconsoleInput == 0))
    {
        if ((hTEconsoleInput = CreateFile("CONIN$",
                                          GENERIC_READ|GENERIC_WRITE,
                                          FILE_SHARE_READ|FILE_SHARE_WRITE,
                                          &sa,
                                          OPEN_EXISTING,
                                          0,
                                          NULL)) != INVALID_HANDLE_VALUE)
        {
            if ((blSuccess = GetConsoleMode(hTEconsoleInput, &dwStdInMode)) != FALSE)
            {
                blSuccess = SetConsoleMode(hTEconsoleInput,
                                         (dwStdInMode & ~(ENABLE_LINE_INPUT  |
                                                          ENABLE_ECHO_INPUT)));
                if (blSuccess != FALSE)
                {
                    blInitialised = TRUE;
                }
            }
        }
    }
    if (!blInitialised)
        blStatus = GetLastError();

    return(blStatus);
}

/*
** Name: ProcessKeyDown - Handler for key press events.
**
** Description:
**      Takes the key press event and maps it to a TE character sequence if
**      there is one.  Control key sequences are checked first.  If a
**      character is matched its value is returned or stored into the return
**      character storage area.
**
** Inputs:
**      pRkey       pointer to the structure for building the TE key sequences
**      pKeyEvent   pointer to the event structure filled in by read console
**
** Outputs:
**      pRkey       blCmplt     TRUE if character is recognised and mapped
**                  ucChar      holds the character to return
**
** Returns:
**      OK          Function is returning with a character or for another read.
**      FAIL        Character is not supported
**
** History:
**      11-Jun-96 (fanra01)
**          Created.
**      05-Dec-96 (fanra01)
**          Extended the function key range check to include F11 and F12.
*/

STATUS
ProcessKeyDown(RKEY *pRkey, KEY_EVENT_RECORD * pKeyEvent)
{
WORD wRep;

    for (wRep=0;(wRep < pKeyEvent->wRepeatCount); wRep++)
    {                                   /* for each instance of the key press*/
        if (pKeyEvent->dwControlKeyState & CTRL_MASK)
        {                               /* any of the control keys a pressed?*/
            if ((pRkey->wLastKey == pKeyEvent->wVirtualKeyCode) &&
                (pRkey->dwLastCtrlState == pKeyEvent->dwControlKeyState))
            {                           /* the state and key hasn't changed  */
                return(OK);             /* get another keyboard event        */
            }
        }

        /*
        **  A keyboard event for handling.
        **  Save the new state and character.
        */
        pRkey->dwLastCtrlState = pKeyEvent->dwControlKeyState;
        pRkey->wLastKey = pKeyEvent->wVirtualKeyCode;
        DumpKeyInfo(pKeyEvent);

        /*
        **  If the left alt key is pressed and not trying an enhanced key
        **  combination.  Check the key is not the alt key on it own and
        **  if it is either numeric key pad or function key.
        */
        if (pKeyEvent->dwControlKeyState & LEFT_ALT_PRESSED)
        {
            if (!(pKeyEvent->dwControlKeyState & ENHANCED_KEY))
            {
                if (pKeyEvent->wVirtualKeyCode != VK_MENU)
                {
                WORD wVirtualKey;

                    /*
                    **  if the virtual key is for the numeric key pad convert
                    **  it to the ascii number.
                    */
                    wVirtualKey = MapKey(pKeyEvent->wVirtualKeyCode);
                    if ((wVirtualKey >='0') && (wVirtualKey <='9'))
                    {                   /* if key is from key pad            */
                                        /* save it and read again            */
                        pRkey->aScanBuf[(pRkey->nPos)++] = wVirtualKey;
                        pRkey->blCmplt = FALSE;
                        return(OK);
                    }
                    else if ((wVirtualKey >= VK_F1) && (wVirtualKey <= VK_F12))
                    {                   /* if function key return sequence   */
                        pRkey->ucChar = ESC;
                        ADD_TO_BUFFER('[');
                        ADD_TO_BUFFER(wVirtualKey - VK_F1 + 'g');
                        pRkey->blCmplt = TRUE;
                        return(OK);
                    }
                    else
                        return(FAIL);   /* bad key combination               */
                }
            }
            else
                return(FAIL);
        }

        /*
        **  If the shift key is pressed, check specifically for TAB and
        **  function keys. Otherwise fall through as shift can be used will
        **  all the ASCII characters.
        */
        if (pKeyEvent->dwControlKeyState & SHIFT_PRESSED)
        {
            if (pKeyEvent->wVirtualKeyCode == VK_TAB)
            {
                pRkey->ucChar = CTRL_KEY('P');
                pRkey->blCmplt = TRUE;
                return(OK);
            }
            if ((pKeyEvent->wVirtualKeyCode >= VK_F1) &&
                (pKeyEvent->wVirtualKeyCode <= VK_F12))
            {
                pRkey->ucChar = ESC;
                ADD_TO_BUFFER('[');
                ADD_TO_BUFFER(pKeyEvent->wVirtualKeyCode - VK_F1 + 'Q');
                pRkey->blCmplt = TRUE;
                return(OK);
            }
        }

        /*
        **  If either the control keys is pressed. Check for specific
        **  combinations.  Again fall through as control can be used with other
        **  ASCII characters.
        */
        if ((pKeyEvent->dwControlKeyState & LEFT_CTRL_PRESSED) ||
            (pKeyEvent->dwControlKeyState & RIGHT_CTRL_PRESSED))
        {
            if (((pKeyEvent->wVirtualKeyCode >= VK_PRIOR)  &&
                 (pKeyEvent->wVirtualKeyCode <= VK_DOWN )) ||
                ((pKeyEvent->wVirtualKeyCode == VK_ADD  )  ||
                 (pKeyEvent->wVirtualKeyCode == VK_SUBTRACT  )))
            {
                switch(pKeyEvent->wVirtualKeyCode)
                {
                    case VK_PRIOR:
                    case VK_UP:
                    case VK_SUBTRACT:
                        pRkey->ucChar = CTRL_KEY('U');
                        break;
                    case VK_NEXT:
                    case VK_DOWN:
                    case VK_ADD:
                        pRkey->ucChar = CTRL_KEY('O');
                        break;
                    case VK_LEFT:
                        pRkey->ucChar = CTRL_KEY('R');
                        break;
                    case VK_RIGHT:
                        pRkey->ucChar = CTRL_KEY('B');
                        break;

                    default:
                        return(FAIL);
                        break;
                }
                if (pRkey->ucChar)
                    pRkey->blCmplt = TRUE;
                return(OK);
            }
        }

        /*
        **  If the key is a function key return the escape sequence
        */
        if ((pKeyEvent->wVirtualKeyCode >= VK_F1) &&
            (pKeyEvent->wVirtualKeyCode <= VK_F12))
        {
            pRkey->ucChar = ESC;
            ADD_TO_BUFFER('[');
            ADD_TO_BUFFER(pKeyEvent->wVirtualKeyCode - VK_F1 + 'G');
            pRkey->blCmplt = TRUE;
            return(OK);
        }

        /*
        **  If the key is any of the movement keys return the escape sequence
        */
        if (((pKeyEvent->wVirtualKeyCode >= VK_PRIOR)  &&
             (pKeyEvent->wVirtualKeyCode <= VK_DOWN )) ||
            ((pKeyEvent->wVirtualKeyCode == VK_INSERT) ||
             (pKeyEvent->wVirtualKeyCode == VK_DELETE)))
        {
            pRkey->ucChar = ESC;
            ADD_TO_BUFFER('[');
            switch(pKeyEvent->wVirtualKeyCode)
            {
                case VK_PRIOR:
                    ADD_TO_BUFFER('y');
                    break;
                case VK_NEXT:
                    ADD_TO_BUFFER('z');
                    break;
                case VK_END:
                    ADD_TO_BUFFER('x');
                    break;
                case VK_HOME:
                    ADD_TO_BUFFER('w');
                    break;
                case VK_LEFT:
                    ADD_TO_BUFFER('D');
                    break;
                case VK_UP:
                    ADD_TO_BUFFER('A');
                    break;
                case VK_RIGHT:
                    ADD_TO_BUFFER('C');
                    break;
                case VK_DOWN:
                    ADD_TO_BUFFER('B');
                    break;
                case VK_INSERT:
                    ADD_TO_BUFFER('u');
                    break;
                case VK_DELETE:
                    ADD_TO_BUFFER('v');
                    break;
            }
            pRkey->blCmplt = TRUE;
            return(OK);
        }

        /*
        **  If the key is the escape key
        */
        if (pKeyEvent->wVirtualKeyCode == VK_ESCAPE)
        {
            pRkey->ucChar = ESC;
            ADD_TO_BUFFER('[');
            ADD_TO_BUFFER('t');
            pRkey->blCmplt = TRUE;
            return(OK);
        }

        /*
        **  If we get here we have a standard character which should just be
        **  returned
        */
        pRkey->ucChar = pKeyEvent->uChar.AsciiChar;
        pRkey->blCmplt = TRUE;
        return(OK);
    }
}

/*
** Name: ProcessKeyUp - Handler for key release events
**
** Description:
**      This function is implemented only for alt key processing.  During the
**      alt & numeric key processing sequence the way to determine the sequence
**      id complete is by detecting when the alt key is released.  This allows
**      up to 3 digit codes to be entered.
**      If the converted value is greater than 255 then the character is not
**      valid and the console input buffer is flushed as multiple the key
**      sequences may now be out of synch.
**
** Inputs:
**
**      pRkey       pointer to the structure for building the TE key sequences
**      pKeyEvent   pointer to the event structure filled in by read console
**
** Outputs:
**      pRkey       blCmplt     TRUE if character is recognised and mapped
**                  ucChar      holds the character to return
**
** Returns:
**      OK          Function is returning with a character.
**      FAIL        Character is not supported
**
** History :
**      11-Jun-96 (fanra01)
**          Created.
*/

STATUS
ProcessKeyUp(RKEY *pRkey, KEY_EVENT_RECORD * pKeyEvent)
{
    DumpKeyInfo(pKeyEvent);
    if (pKeyEvent->wVirtualKeyCode == VK_MENU)
    {
    i4 natChar;

        if (CVan(pRkey->aScanBuf, &natChar) == OK)
        {
            if ((natChar >= 0) && (natChar < 256))
            {
                pRkey->ucChar = (unsigned char) natChar;
                pRkey->blCmplt = TRUE;
            }
            else
            {
                FlushConsoleInputBuffer(hTEconsoleInput);
                pRkey->nPos = 0;
                return(FAIL);
            }
        }
        pRkey->nPos = 0;
    }
    return(OK);
}

/*
** Name: ReadKeybEvent - Return a keyboard event
**
** Description:
**      Wait for a console event for the specified timeout.  If the console
**      input buffer has events stored in it the console object is signalled
**      which will cause the first one to be read.
**
** Inputs:
**      pInputRec       pointer to an input record.
**
** Outputs:
**      pInputRec       EventType and associated event structure.
**
** Returns:
**      dwRecsRead      number of records read
**      0               if an error occurs.
**
** History :
**      11-Jun-96 (fanra01)
**          Created.
*/
DWORD
ReadKeybEvent(PINPUT_RECORD pInputRec, DWORD dwTimeout)
{
DWORD dwRecsRead = 0;
BOOL blStatus;

    if (WaitForSingleObject(hTEconsoleInput,(DWORD) dwTimeout) == WAIT_TIMEOUT)
          return((DWORD)0);
    else
    {
        blStatus = ReadConsoleInput(hTEconsoleInput, pInputRec, 1, &dwRecsRead);
    }
    return(dwRecsRead);
}

/*
** Name: TEget -    console read function returning TE character sequences
**                  which map to termcap definitions.
**
** Description:
**      On first call initialise the trace flag.
**      If run from sep read from the provided input handle.
**      If the character buffer still has characters return them from there
**      otherwise setup the return storage pointer and initialise the key
**      structure.
**
**      Read the console event and process it with either the key down or
**      key up functions.  Return the character or output a BEL if the
**      character is not supported and try reading another character.
**
** Inputs:
**
**      seconds     number of seconds to wait for console input.
**
** Outputs:
**      None
**
** Returns:
**      Character read
**      TE_TIMEDOUT         no console activity for the specified time.
**
** History:
**      11-Jun-96 (fanra01)
**          Re-created.
**      03-May-2001 (fanra01)
**          Add a TEstreamEnabled case and use getchar to read from stdin.
**	03-oct-2003 (somsa01)
**	    For AMD64, turned off global optimization around TEget() to
**	    stop CBF from not working properly when processing key events.
*/

#ifdef a64_win
#pragma optimize("g", off)
#endif

i4 TEget(i4 seconds)
{
INPUT_RECORD stInputRec;
DWORD dwRecsRead = 0;
RKEY stReadKey;
STATUS Status;
DWORD dwTimeout;
PTR env = NULL;

    dwTimeout = (seconds) ? 1000 * seconds : INFINITE;

    if (!blInitialised)
    {
        ReadKeyboardInit();
        NMgtAt( ERx("II_TEGET_DEBUG"), &env);
        if ( env && *env)
        {
            CVan(env,&nTrace);
        }
        blInitialised = TRUE;
    }

    if (TEstreamEnabled)
    {
        if (seconds > 0)
        {
            return(TE_TIMEDOUT);
        }
        else
        {
            return( (char)getchar() );
        }
    }

    if (IIte_use_tc)
        return(TCgetc(IItc_in, seconds) );

    if (nCharCount)
    {
        nCharCount -= 1;
        return(*pRetChars++);
    }

    pRetChars = aRetChars;

    stReadKey.blCmplt = FALSE;
    MEfill(sizeof(stReadKey.aScanBuf), 0, (PTR) stReadKey.aScanBuf);
    stReadKey.nPos = 0;
    stReadKey.ucChar = 0;
    stReadKey.dwLastCtrlState = 0;
    stReadKey.wLastKey = 0;

    while(TRUE)
    {
        if ((dwRecsRead = ReadKeybEvent(&stInputRec, dwTimeout)) == 0)
            return(TE_TIMEDOUT);

        if (stInputRec.EventType == KEY_EVENT)
        {
            if (stInputRec.Event.KeyEvent.bKeyDown)
            {
                if ((Status = ProcessKeyDown(&stReadKey,
                                              &stInputRec.Event.KeyEvent))!= OK)
                {
                    TEput(BEL);
                }
            }
            else
            {
                if ((Status = ProcessKeyUp(&stReadKey,
                                            &stInputRec.Event.KeyEvent))!= OK)
                {
                    TEput(BEL);
                }
            }

            if ((stReadKey.blCmplt == TRUE) && (stReadKey.ucChar))
            {
                return(stReadKey.ucChar);
            }
        }
    }
}

#ifdef a64_win
#pragma optimize("g", on)
#endif

/*{
** Name:	TEinflush - Flush input.
**
** Description:
**	Flush any type ahead input from the terminal.  The
**	assumption here is that this routine will NOT be
**	called if a forms based front end is in test mode
**	(i.e., input coming from a file).  This will
**	eliminate the possibility of doing a flush on
**	input that comes from a file.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/11/87 (dkh) - Initial version.
**	10/17/87 (dkh) - Changed to work properly on Pyramid.
*/
VOID
TEinflush(void)
{
	/*
	** call SIflush function to do the flush.
	*/
	SIflush(stdin);
}
