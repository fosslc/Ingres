/*
**      Copyright (c) 1983, 2009 Ingres Corporation
**      All rights reserved.
*/

# include       <compat.h>
# include       <si.h>
# include       <cm.h>
# include       <cv.h>
# include       <nm.h>
# include       <pc.h>
# include       <lo.h>
# include       <st.h>
# include	<stdarg.h>
# include       "silocal.h"

# ifdef INT
# undef INT
# endif 
#define MAXLINES        	600
#define VGAWIDTH        	79  	 /* Width of VGA screen in Chars    */
#define VGA_WIDTH       	640	 /* Width of VGA screen in Pixels   */
#define MAXWIDTH        	84  	 /* max width of formatted output   */
#define W4GL_MAX_LOG_SIZE	32768	 /* jkronk, B5-w4gl00 Runtime stuff */

GLOBALREF HWND      hWndStdout;
GLOBALREF HWND      hWndDummy;
GLOBALREF i4	    SIlogit;	
GLOBALREF BOOL      SIchar_mode;
GLOBALREF char	    TraceWindowTitle [48];
GLOBALREF char	    TraceWindowClass [];
GLOBALREF int       SIwantDialogBox;
GLOBALREF HANDLE    SIhInst;

GLOBALREF FILE *    W4glErrorLog; 
GLOBALREF bool      need2del_font;
GLOBALREF HFONT     tracewin_font;
GLOBALREF char      printbuf[65536]; /* arbitrary long-enough length */
GLOBALREF char      linebuf[65536];

STATICDEF HANDLE    hDLLInst = 0;
STATICDEF char *    LogFileName = NULL;
STATICDEF char *    TruncateLogfile = NULL;
STATICDEF char *    W4glErrorLogBuf = NULL;
STATICDEF bool      isOpenROADStyle = FALSE;
STATICDEF i2        wBufferNumLines = 0;
STATICDEF i4	    wBufferUsableWidth = MAXWIDTH;
STATICDEF i2        xChar = 0, yChar = 0;
STATICDEF i2        xClient = 0, yClient = 0;
STATICDEF i2        wVscrollMax     = 0, wVscrollPos = 0, wVscrollInc =     0;
STATICDEF i2        wScreenNumLines = 0;
STATICDEF i2	    wBufferCurrentLine = 0;
STATICDEF i2        wWrappedAround = 0;
STATICDEF i2        wClearLine = 1;
STATICDEF i2        wScreenCurrentLine = 0;

typedef struct  _LINEBUFFER
{
	char    line[MAXWIDTH+1];
} LINEBUFFER;

STATICDEF LINEBUFFER *  wBuffer = NULL;

/*static*/ INT          pinit_docre8    = 0;
/*static*/ HANDLE       pinit_hInst     = 0;
/*static*/ HWND         pinit_hWnd      = 0;
/*static*/ INT          pinit_TraceFlag = 0;

long	FAR PASCAL  SIDbgWndProc(HWND, UINT, WPARAM, LPARAM);



/*
** "SIpinit" can be invoked in 1 of 2 ways. It can ask for a special debugging
** window to be created, by setting "docreate"=TRUE. In this case it needs to
** pass the HANDLE of the hInstance of the parent window.
**
** Or it can set "docreate"=FALSE, and pass the HWND of the window to be used
** for doing the SIprintf's.
**
** History:
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      23-aug-1996 (wilan06)
**          Removed include of wn.h, which is no longer used on NT.
**          #undef INT, which is unwisely #defined by mecl.h to be 30.
**	10-dec-1996 (donc)
**	    Replaced old SIpinit with new and improved OpenROAD version.
**	    Calls to the old version are compatible with this new version
**	04-feb-97 (mcgem01)
**	    Include lo.h so that this will compile on NT.
**	02-mar-1997 (donc)
**	    Pass NMloc a "naked" file name, unadorned with a full path, This
**	    is the way the OpenINGRES CL expcts things.
**	14-may-97 (mcgem01)
**	    Removed duplicate definition of SIwantDialogBox.
**	10-nov-97 (joe)
**		Fixed a problem with the handling of II_TRUNCATE_W4GL_LOGFILE, the code
**		expects the static pointer TruncateLogfile to be NULL if the variable is
**		not set, NMgtAt was setting to point at an empty string.  Note this is
**		only used by OPENROAD and does not affect any other product.
**	26-jan-1998 (canor01)
**	    Clean up compiler warnings with ANSI compiler.
**	29-may-1998 (canor01)
**	    Clean up variable parameter passing using stdarg in SIprint_ortrace.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	19-dec-2000 (somsa01)
**	    SIftell() now returns the offset.
**	02-apr-2001 (mcgem01)
**	    Add an OpenROAD specific version of SIdofrmt for backward
**	    compatability.  This version is invoked in SIprint_ortrace.
**      8-feb-2005 (fraser)
**          Removed some dead code that was ifdef-ed out.
**      8-feb-05 (fraser)
**          Added new function SIpinitEx.  This function has a new parameter,
**          fail_if_no_logfile; if this parameter is true and SIpinitEx cannot
**          create a log file, the function does not put up a message box, but
**          simply returns FAIL. Changed the third parameter of SIpinit and
**          SIpinitEx from i2 to HWND.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	27-Jan-2005 (ianma02) Bug 113742
**	    Backed out the change for SIR 107535 which was intended only for
**	    OpenROAD r5.
**      20-feb-2007 (wridu01)
**          Re-added new function SIpinitEx.  This function has a new parameter,
**          fail_if_no_logfile; if this parameter is true and SIpinitEx cannot
**          create a log file, the function does not put up a message box, but
**          simply returns FAIL. Changed the third parameter of SIpinit and
**          SIpinitEx from i2 to HWND.
**	11-may-2009 (zhayu01) Bug 122037
**	    Convert and write out UTF16 string to trace window for UTF8 data.
*/



void
SIpinit
(
    short	docreate,       /* Create a trace window */
    void *	hInstance,      /* The module handle */
    HWND        hWnd,           /* Handle to Stdout if docreate = FALSE */
    i2	        TraceFlg,       /* State of trace window; if zero, trace window */
                                /* not created */
    char *	LogFile,        /* Name of the log file */
    i2	        AppendFlg,      /* "w" or "a" */
    char *	TrcTitle,       /* Title of trace windoe */
    bool	bFixedFontFlag  /* If TRUE, use system fixed font */
)
{
    SIpinitEx(docreate, hInstance, hWnd, TraceFlg, LogFile,
                        AppendFlg, TrcTitle, bFixedFontFlag, FALSE);
}


i4
SIpinitEx
(
    short	docreate,       /* Create a trace window */
    void *	hInstance,      /* The module handle */
    HWND        hWnd,           /* Handle to Stdout if docreate = FALSE */
    i2	        TraceFlg,       /* State of trace window; if zero, trace window */
                                /* not created */
    char *	LogFile,        /* Name of the log file */
    i2	        AppendFlg,      /* "w" or "a" */
    char *	TrcTitle,       /* Title of trace windoe */
    bool	bFixedFontFlag,  /* If TRUE, use system fixed font */
    bool        fail_if_no_logfile
)
{
    RECT        rect;
    WNDCLASS	wndclass;
    i2          xScreen, yScreen;
    char *	lpszClass = (char *)&TraceWindowClass;
    char *	lpszTitle = (char *)&TraceWindowTitle;
    WORD        xsize, ytop, ysize;
    LOGFONT     logfont;
    LOCATION	loc;
    VOID	SIpterm();
    VOID	GetMetrics();

    static char * SIgetUniqueLogName();
    static void   SIunableToOpenLogFile(void); 

    SIhInst = hInstance;
    hDLLInst = hInstance;

    /* Create the font used to print in the trace window. */
    MEfill(sizeof(LOGFONT), 0, &logfont);
    logfont.lfHeight = 8;
    logfont.lfWeight = FW_BOLD;
    strcpy(logfont.lfFaceName, "Helv");
    tracewin_font = bFixedFontFlag ? GetStockObject(SYSTEM_FIXED_FONT)
	                           : CreateFontIndirect(&logfont);
    need2del_font = bFixedFontFlag ? FALSE : TRUE;

    /*
    **      jkronk, B5-w4gl00 runtime stuff.
    **
    **      If the error log has not already been opened, then open it and honor the
    **      append state requested by the user on the command line.
    */
    if (LogFile)
    {
  	/* Determine if we are going to truncate the logfile */

	NMgtAt("II_TRUNCATE_W4GL_LOGFILE",&TruncateLogfile);
	if( TruncateLogfile && *TruncateLogfile == NULL)
	{
            TruncateLogfile = NULL;
	}

	if (!W4glErrorLog)
	{
	    /*
	    ** Open the Window4GL error log file
	    */
	    
	    char	LogName[_MAX_PATH + 1];
	    
	    strcpy(LogName, LogFile);
	    LogFile = SIgetUniqueLogName(LogFile, AppendFlg);
	    LogFileName = STalloc( LogFile );

	    SIlogit = 0;
	    if (!(LOfroms(PATH&FILENAME, LogFileName, &loc) == OK &&
		    SIopen(&loc, AppendFlg ? "a" : "w", &W4glErrorLog) == OK))
            {
                if (fail_if_no_logfile)
                {
                    return(FAIL);
                }
                else
                {
                    SIunableToOpenLogFile();
                }
            }
	    else
            {
                SIclose(W4glErrorLog);
            }
	}
    }
    else
    {
        W4glErrorLog = NULL;
    }

    if (!TraceFlg)
    {
        return(OK);
    }
    if (docreate)
    {
	/* Printfs go to an OpenROAD-style trace window */
        isOpenROADStyle = TRUE;

	if (TrcTitle)
	{
	    strcpy(TraceWindowTitle, TrcTitle);
	}

	wBuffer = (LINEBUFFER *) MEreqmem(0, (MAXLINES+1) * sizeof(LINEBUFFER), TRUE, NULL);
	hWndStdout = FindWindow(lpszClass, lpszTitle);

	if (!hWndStdout)
	{
	    wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	    wndclass.lpfnWndProc   = SIDbgWndProc;
	    wndclass.cbClsExtra    = 0;
	    wndclass.cbWndExtra    = 0;
	    wndclass.hInstance     = hDLLInst;
	    if (!(wndclass.hIcon   = LoadIcon(hDLLInst, "TRACEWIN")))
		    wndclass.hIcon     = LoadIcon(NULL, IDI_ASTERISK);
	    wndclass.hCursor       = LoadCursor (0, IDC_ARROW);

            /*  valerier, A08 - Black background */
	    wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	    wndclass.lpszMenuName  = NULL;
	    wndclass.lpszClassName = lpszClass;

	    /*
	    ** Don't check for success of RegisterClass. If the app
	    ** that originally registered it has gone away, this
	    ** can legitimately fail.
	    */
	    RegisterClass (&wndclass);
	}

	/*
	** Determine where we wish to place the window on the screen.
	*/
	xScreen = GetSystemMetrics (SM_CXSCREEN);
	yScreen = GetSystemMetrics (SM_CYSCREEN);

	/*
	** A16-w4gl00357-ruchlis
	*/
	ytop = 0;

	/*  valerier, A08 - Width is narrowed for high-res screens */
	if (xScreen > VGA_WIDTH)
	{
	    xsize = xScreen/2 + xScreen/4;
	    ysize = yScreen/3;
	}
	else
	{
	    xsize = xScreen;
	    ysize = yScreen/2;
	    wBufferUsableWidth = VGAWIDTH;
	}

	strcat (TraceWindowTitle, " ");

	hWndDummy = CreateWindow (lpszClass, "",
				WS_OVERLAPPEDWINDOW | WS_VSCROLL,
				0, ytop,    		/* initial (x,y) */
				xsize, ysize,		/* size of window */
				0, 0, hDLLInst, 0);

	hWndStdout = CreateWindow (lpszClass, lpszTitle,
				WS_OVERLAPPEDWINDOW | WS_VSCROLL,
				0, ytop,    		/* initial (x,y) */
				xsize, ysize,		/* size of window */
				0, 0, hDLLInst, 0);


	GetMetrics(hWndStdout);
	if (xScreen > VGA_WIDTH)
	{
	    xsize = min(xScreen, (xChar * 81) + GetSystemMetrics(SM_CXVSCROLL)
			                + (GetSystemMetrics(SM_CXFRAME) * 2));
	    ysize = min(yScreen/2,(yChar * 14) + GetSystemMetrics(SM_CYCAPTION)
			                + (GetSystemMetrics(SM_CYFRAME) * 2));
	    SetWindowPos(hWndDummy,  0, 0, ytop, xsize, ysize, SWP_NOZORDER);
	    SetWindowPos(hWndStdout, 0, 0, ytop, xsize, ysize, SWP_NOZORDER);
	}

	GetClientRect( hWndStdout, &rect );
	yClient = rect.bottom - rect.top;
	xClient = rect.right - rect.left;
	wScreenNumLines = (yClient / yChar);

	ShowWindow(hWndStdout,
				(TraceFlg == TRACE_MINIMIZED) ? SW_SHOWMINNOACTIVE :
				(TraceFlg == TRACE_MAXIMIZED) ? SW_SHOWMAXIMIZED :
				SW_SHOWNORMAL);
	UpdateWindow (hWndStdout);
    }
    else
    {
	GetMetrics (hWnd);
	GetWindowRect ((HWND)hWnd, &rect);
	xClient = rect.right - rect.left;
	yClient = rect.bottom - rect.top;
	hWndStdout = (HWND) hWnd;
    }
    PCatexit (SIpterm);

    return(OK);
}




/*
** call "SIpinit" again with the same values as the previous call.
*/
void
SIpReInit(void)
{
    HWND    svFocus = GetFocus();

    hWndStdout = FindWindow(TraceWindowClass, TraceWindowTitle);
    SIpinit((short)pinit_docre8, pinit_hInst, pinit_hWnd, (i2)pinit_TraceFlag,
	    NULL, (i2)1, TraceWindowTitle, FALSE);
    SetFocus(svFocus);
}



BOOL
SIget_charmode()
{
    return SIchar_mode;
}



/*static*/  WORD    prevTshowCmd = -1;
/*static*/  HWND    prevFocus = NULL;

/*
** Show the Trace/History Window as NORMAL.
*/
VOID
SIshowTraceNormal
(
    i2      type           /* 0 -> temp; 1 -> perm */
)
{
    prevFocus = GetFocus();
    if ((hWndStdout = FindWindow(TraceWindowClass, TraceWindowTitle)) != NULL)
    {
	if (type == 0)
	{
	    if (IsZoomed(hWndStdout))
		prevTshowCmd = -1;
	    else
	    {
		prevTshowCmd = IsIconic(hWndStdout) ? SW_SHOWMINIMIZED
						    : SW_NORMAL;
		ShowWindow(hWndStdout, SW_SHOWNORMAL);
	    }
	}
	else if (type == 1)
	{
	    prevTshowCmd = -1;
	    if (IsIconic(hWndStdout))
		ShowWindow(hWndStdout, SW_SHOWNORMAL);
	}
	else if (type == 2)
	{
	    prevTshowCmd = -1;
	    ShowWindow(hWndStdout, SW_SHOWMAXIMIZED);
	}
	SetFocus(hWndStdout);
    }
}




/*
** Show the Trace/History Window as it previously was shown.
*/
VOID
SIshowTraceAsPrev(void)
{
    if (hWndStdout == 0)
	return;

    if (((hWndStdout = FindWindow(TraceWindowClass, TraceWindowTitle)) != NULL)
                && (prevTshowCmd != -1) && !IsZoomed(hWndStdout))
    {
	ShowWindow(hWndStdout, prevTshowCmd);
	prevTshowCmd = -1;
    }
    if (prevFocus)
    {
	SetFocus(prevFocus);
	prevFocus = NULL;
    }
}



HWND
SIget_hWndStdout(void)
{
    return(hWndStdout = FindWindow(TraceWindowClass, TraceWindowTitle));
}



VOID
SIsetFocus2TraceWindow(void)
{
    if (hWndStdout == 0)
	return;

    if ((hWndStdout = FindWindow(TraceWindowClass, TraceWindowTitle)) != NULL)
	SetFocus(hWndStdout);
}



/*
** SIpterm terminates the SIprintf window.
*/
#pragma data_seg("si_share")
#define	SIZEPROCESSIDTBL	64
DWORD	SIprocessID[SIZEPROCESSIDTBL] = {0};
i4 	SIprocessCount = 0;
i4 	SIuniqueID = 0;
#pragma data_seg()
void
SIpterm(void)
{
    /*
    ** A16-w4gl00357-ruchlis
    ** Do not DestroyWindow or Unregister class here. Let Windows do this
    ** itself.
    */

    i4 	        i;
    DWORD	pID = GetCurrentProcessId();

    if (--SIprocessCount <= 0)
        SIprocessCount = SIuniqueID = 0;

    for (i = -1; ++i < SIZEPROCESSIDTBL; )
    {
	if (SIprocessID[i] == pID)
	{
	    SIprocessID[i] = 0;
	    break;
	}
    }
    if (wBuffer)
        MEfree(wBuffer);

    if (need2del_font)
        DeleteObject(tracewin_font);
}



/*
** SIgetUniqueLogName:
**
**	Inputs: p = Name to use if SIprocessCount == 0 or append is set,
**				or to build from if != 0 and not appending;
**			append = create new file or append to existing file.
**
**	Returns: Its input if SIprocessCount == 0 or a new file name file with
**		the same base name as its input but with extension the value of the
**		offset (as a string) into the SIprocessID table where the current
**		process's ID is stored.
*/
static
char *
SIgetUniqueLogName
(
    char *  p,
    i4      append
)
{
    char *  q;
    i4 	    i = 0;
    
    if (SIprocessCount++)
    {
	while (SIprocessID[++i] && (i < SIZEPROCESSIDTBL))
		;
	    if (i >= SIZEPROCESSIDTBL)
		i  = SIZEPROCESSIDTBL + SIuniqueID++;

	q = p + strlen(p);
	while(--q > p)
	{
	    if (*q == '.')
	        break;
	    if ((*q == '\\') || (*q == ':'))
	    {
	        q = p + strlen(p);
	        break;
	    }
	}
	STprintf(q, ".%03x", i);
    }

    if (i < SIZEPROCESSIDTBL)
        SIprocessID[i] = GetCurrentProcessId();
    return p;
}



void
SIw30_write
(
    char *	str
)
{
	MSG 		msg;
	short		buflen;
	LOCATION	loc;
	char		*fname;

	/*
	** If we are writing new output to the trace window then scroll to
	** bottom of the trace window.
	*/
	if (wVscrollPos != wVscrollMax)
		SendMessage(hWndStdout, WM_VSCROLL, SB_BOTTOM, NULL);

	if (W4glErrorLog)
	{
		i4 FilePos;
		i4 NumRead;

		SIlogit = 0;
		if (!((LOfroms(PATH&FILENAME, LogFileName, &loc) == OK) &&
			(SIopen(&loc, "a", &W4glErrorLog) == OK)))
		{
			SIunableToOpenLogFile();
		}
		else
		{
			SIputrec(str, W4glErrorLog);
			if( TruncateLogfile )
			{
				/*
				** We only let our log file grow to W4GL_MAX_LOG_SIZE
				** (initally 32K) because we want to be able to view it in
				** notepad, which can only handle  ~45K bytes of info.
				** If the log grows past our limit, we then truncate the log
				** to W4GL_MAX_LOG_SIZE / 2.  This is done so the user never
				** loses recent information.
				*/

				if (((FilePos = SIftell(W4glErrorLog)) >= 0)
			 	 && (FilePos > W4GL_MAX_LOG_SIZE))
				{
					if (!W4glErrorLogBuf)
					{
						W4glErrorLogBuf = MEreqmem(NULL, W4GL_MAX_LOG_SIZE / 2 + 1, FALSE, NULL );
						if(!W4glErrorLogBuf)
						{
							MessageBox(NULL,
								"Unable to allocate memory for the w4gl.log file management. Windows4GL will be shutdown.",
								NULL, MB_OK );
							PCexit(FAIL);
						}
					}
					SIclose(W4glErrorLog);
					fname = STalloc( loc.fname );
					if (!((NMloc(LOG, FILENAME, fname, &loc) == OK) &&
						(SIopen(&loc, "r", &W4glErrorLog) == OK)))
					{
						SIunableToOpenLogFile();
					}
					else
					{
						i4 count;
	
						if (SIfseek(W4glErrorLog, W4GL_MAX_LOG_SIZE / 2, SI_P_START) != OK)
						{
							MessageBox(NULL,
								"Unable to seek to desired position in the w4gl.log file. Windows4GL will be shutdown.",
								NULL, MB_OK );
							PCexit(FAIL);
						}
						SIread(W4glErrorLog, W4GL_MAX_LOG_SIZE, &NumRead, W4glErrorLogBuf);
						if (!((NMloc(LOG, FILENAME, fname, &loc) == OK) &&
							(SIopen(&loc, "w", &W4glErrorLog) == OK)))
						{
							SIunableToOpenLogFile();
						}
						else
						{
							SIwrite(NumRead, W4glErrorLogBuf, &count, W4glErrorLog);
						}
					}
					MEfree( fname );
				}
			}
			SIclose(W4glErrorLog);
		}
		SIlogit = 1;
	}

	if (!hWndStdout)
		return;

	buflen = strlen(wBuffer[wBufferCurrentLine].line);

	if (wClearLine)
	{
		wBuffer[wBufferCurrentLine].line[0] = '\0';
		buflen = 0;
	}

	strncat (wBuffer[wBufferCurrentLine].line, str, wBufferUsableWidth-buflen);
	buflen = strlen(wBuffer[wBufferCurrentLine].line);

        /*
        **  This fixes the trace window problem where a string reaches the maximum
        **  length before an ending newline.  Previously, this stopped all output
        **  to the trace window.
        */
	if (wBuffer[wBufferCurrentLine].line[buflen-1] == '\n' ||
		 (buflen == wBufferUsableWidth) )
	{
		wClearLine = 1;
		wBuffer[wBufferCurrentLine].line[buflen-1] = '\0';
		wBufferNumLines = min(wBufferNumLines+1, MAXLINES);
		if ((wBufferNumLines > wScreenNumLines)
		 || (wScreenCurrentLine ==
		      (wBufferCurrentLine * wWrappedAround - wScreenNumLines)))
		{
				if (wVscrollPos == wVscrollMax)
				{
					wScreenCurrentLine++;
					if (wScreenCurrentLine > (MAXLINES-1))
						wScreenCurrentLine = 0;
					wVscrollInc = 1;
					ScrollWindow(hWndStdout, 0, -yChar, NULL,NULL);
				}
				else
					wVscrollInc = -1;

			wVscrollMax = max (0, wBufferNumLines-wScreenNumLines);
			wVscrollPos = max (0, min(wVscrollPos+wVscrollInc, wVscrollMax));
			SetScrollRange(hWndStdout, SB_VERT, 0, wVscrollMax, FALSE);
			if ( wVscrollMax )
				SetScrollPos  (hWndStdout, SB_VERT, wVscrollPos, TRUE);
		}
		else
			wVscrollInc     = 0;

		wBufferCurrentLine++;
		if (wBufferCurrentLine > (MAXLINES-1))
		{
			wBufferCurrentLine = 0;
			wWrappedAround = 1;
		}
		wBuffer[wBufferCurrentLine].line[0] = '\0';		/* clear next line - bug # 61566 */

	}
	else
		wClearLine = 0;

	/*
	** Redraw screen based on our new circular buffer.
	*/
	if (!wVscrollInc)
		InvalidateRect( hWndStdout, NULL, FALSE );

	UpdateWindow (hWndStdout);

	while (PeekMessage(&msg, hWndStdout, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage (&msg);
	}
}



/**
** Name: siprint_ortrace.c -    Formatted OpenROAD Print Compatibility Module.
**
** Description:
**      This file contains the definitions of the routines used to output
**      formatted strings through the SI module.  ('SIdofrmt()' is used by
**      the string formatting routines of ST ('STprintf()' and 'STzprintf()',
**      as well.)
**
**      SIprintf()      formatted print to standard output
**      SIfprintf()     formatted print to stream file
**
** History:
**	29-may-1998 (canor01)
**	    Clean up variable parameter passing using stdarg.
**
*/

VOID SIprint_ortrace( char    *fmt, va_list ap )
{

	SIdoorfrmt((i4)SI_SPRINTF, (FILE *)printbuf, fmt, ap);

	if ( !hWndStdout )
	{	
		printf("%s", printbuf);
		return;
	}

	/*
	** A19-w4gl00xxx-ruchlis
	** If the string contains '\0x0a' we want to go thru a second stage
	** of formatting, by writing this out as a sequence of strings.
	*/
	{
		char *src = printbuf;
		char *dst = linebuf;
		i2    len = strlen(wBuffer[wBufferCurrentLine].line);

		while (*src)
		{
			/*
			**	Check to see if either a natural break
			**	in the line (i.e. \n) has occurred or
			**	if the maximum line length has been reached.
			**	An additional attempt is made to break the
			**	the line on a blank rather than in the middle
			**	of a word.
			*/
			if (*src == '\n'        			||
 				len == wBufferUsableWidth-2		||
				len >= wBufferUsableWidth-8 && *src == ' ' 
				                            && *(src+1) != ' '
				                            && *(src+2) != '\n')
			{
				*dst++ = *src++;

				if (len == wBufferUsableWidth-2 	||
				    len >= wBufferUsableWidth-8 && *(src-1) == ' ' 
				                                && *src != ' '
				                                && *(src+1) != '\n')
				{
					*dst++ = '\n';
				}

				*dst = EOS;    /* null terminate the string */
				SIw30_write(linebuf);
				/* reinitialize */
				len     = 0;
				dst     = linebuf;
			}
			else if (*src == '\r')
			{
				/* throw away '\r' */
				src++;
				continue;
			}
			else {          /* normal case */
				*dst++ = *src++;
				len++;
			}
		}

		if (len == (wBufferUsableWidth-2))
			*dst++ = '\n';
		if (len)
		{
			*dst++ = EOS;
			SIw30_write(linebuf);
		}
	}
	/*
	** A19-w4gl00xxx-ruchlis-end
	*/
}

STATICDEF i2       wPaintLineBeg = 0, wPaintLineEnd = 0, wPaintLinePos = 0;

GLOBALREF char  szStdin[];


long FAR PASCAL
SIDbgWndProc
(
    HWND	hWnd,
    UINT        iMessage,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    i2          bltemp = wBufferCurrentLine;
    RECT        wRect;
    i2          wmod;
    VOID        SIDoPaint();
    VOID        GetMetrics();

    switch (iMessage)
    {
	case WM_CREATE:
	    GetMetrics (hWnd);
	    break;

	case WM_LBUTTONDBLCLK:
	    SIwantDialogBox = 1;
	    break;

	case WM_SIZE:

	    if (wParam != SIZE_MINIMIZED)
	    {
		yClient = HIWORD (lParam);
		xClient = LOWORD (lParam);
	    }

	    if ((wmod=yClient%yChar) != 0)
	    {
		GetWindowRect(hWnd, &wRect);
		SetWindowPos (hWnd, (HWND)NULL, 0, 0,
					  wRect.right-wRect.left,
					  wRect.bottom-wRect.top-wmod,
					  SWP_NOMOVE);
	    }
	    else
	    {
	        i2      wPrevScreenNumLines = wScreenNumLines;

	        wScreenNumLines = (yClient / yChar);


		wVscrollMax = max(0, wBufferNumLines-wScreenNumLines);
		SetScrollRange(hWnd, SB_VERT, 0, wVscrollMax, FALSE);
		if (wVscrollMax)
			SetScrollPos(hWnd, SB_VERT, wVscrollPos, TRUE);
		if (wBufferNumLines < wScreenNumLines)
			PostMessage(hWnd, WM_VSCROLL, SB_BOTTOM, 0L);
		else if (wPrevScreenNumLines < wScreenNumLines)
		{
			PostMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0L);
			PostMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0L);
		}
	    }
	    break;

	case WM_VSCROLL:

	    switch (LOWORD(wParam))
	    {
		case SB_TOP:
		    wVscrollInc = -wVscrollPos;
		    break;

		case SB_BOTTOM:
		    wVscrollInc = wVscrollMax - wVscrollPos;
		    break;

		case SB_LINEUP:
		    wVscrollInc = -1;
		    break;

		case SB_LINEDOWN:
		    wVscrollInc = 1;
		    break;

		case SB_PAGEUP:
		    wVscrollInc = min (-1, -yClient / yChar);
		    break;

		case SB_PAGEDOWN:
		    wVscrollInc = max (1, yClient / yChar);
		    break;

		case SB_THUMBTRACK:
		    wVscrollInc = HIWORD (wParam) - wVscrollPos;
		    break;

		default:
		    wVscrollInc = 0;
	    }

	    if (wVscrollInc = max(-wVscrollPos,
		      min(wVscrollInc, wVscrollMax - wVscrollPos)))
	    {
		wVscrollPos = max(0,min(wVscrollPos+wVscrollInc,wVscrollMax));

		wScreenCurrentLine += wVscrollInc;
		if (wScreenCurrentLine < 0)
		    wScreenCurrentLine += MAXLINES;
		else if (wScreenCurrentLine > (MAXLINES-1))
		    wScreenCurrentLine -= MAXLINES;

		SetScrollRange(hWnd, SB_VERT, 0, wVscrollMax, FALSE);
		if (wVscrollMax)
		    SetScrollPos(hWnd, SB_VERT, wVscrollPos, TRUE);

		ScrollWindow(hWndStdout, 0, -yChar*wVscrollInc, NULL, NULL);
		UpdateWindow(hWnd);
	    }
	    break;

	case WM_PAINT:
	    SIDoPaint();
	    break;

	case WM_DESTROY:
	    PostQuitMessage(0);
	    DeleteObject(tracewin_font);
	    if (hWndStdout == hWnd)
		hWndStdout = 0;
	    break;

	case WM_ENTERIDLE:
	    EnableWindow(hWnd, TRUE);
	default:
	    return DefWindowProc (hWnd, iMessage, wParam, lParam);
    }
    return 0L;
}



VOID
SIDoPaint()
{
    PAINTSTRUCT	    ps;
    i2              wBufIndex;
    i2              i;
    i2              wLineNum, wNumLines;
    i2              wBaseLine;
    i2              wPaintStart;
    STATICDEF i2    wPaintIt = 0;
#ifdef ENABLE_UTF8
    wchar_t	strW[MAXWIDTH + 1];
    i4	lenW;
#endif

    BeginPaint (hWndStdout, &ps);

    SelectObject( ps.hdc, tracewin_font );
    SetBkMode( ps.hdc, TRANSPARENT );
    SetTextColor( ps.hdc, RGB(255, 255, 255) );

    if (!wPaintIt &&
       (ps.rcPaint.top%yChar != 0 || ps.rcPaint.bottom%yChar != 0 ||
	    ps.rcPaint.left != 0 || ps.rcPaint.right != xClient))
    {
	EndPaint (hWndStdout, &ps);
	wPaintIt = 1;
	InvalidateRect (hWndStdout, NULL, TRUE);
	UpdateWindow (hWndStdout);
	return;
    }

    wLineNum = wScreenCurrentLine + ps.rcPaint.top/yChar;
    wNumLines = (ps.rcPaint.bottom - ps.rcPaint.top)/yChar;

    wPaintStart = ps.rcPaint.top - ps.rcPaint.top%yChar;

    for     (i = 0; i <= wNumLines; i++, wLineNum++)
    {
	if (wWrappedAround)
            wBaseLine = wBufferCurrentLine;
	else
            wBaseLine = 0;

	wBufIndex = wLineNum;

	if (wBufIndex > (MAXLINES-1))
	    wBufIndex -= MAXLINES;

#ifdef ENABLE_UTF8
    if (CM_ischarsetUTF8())
    {
        lenW = MultiByteToWideChar(CP_UTF8, 0, wBuffer[wBufIndex].line, 
                      strlen(wBuffer[wBufIndex].line), strW, MAXWIDTH);
        if (lenW == 0)
        {
            TabbedTextOut (ps.hdc, xChar, ps.rcPaint.top+(yChar*i),
						wBuffer[wBufIndex].line,
						strlen(wBuffer[wBufIndex].line),
						0, NULL, 0);		
        }
        else
        {
            *(strW + lenW) = (wchar_t)0;
            TabbedTextOutW (ps.hdc, xChar, ps.rcPaint.top+(yChar*i),
						strW,
						lenW,
						0, NULL, 0);		

        }
    }
    else
#endif
	/* rionc; A27  #54592
	** need tabs in trace window
	*/
	TabbedTextOut (ps.hdc, xChar, ps.rcPaint.top+(yChar*i),
						wBuffer[wBufIndex].line,
						strlen(wBuffer[wBufIndex].line),
						0, NULL, 0);
    }

    wPaintIt = 0;
    EndPaint (hWndStdout, &ps);
}



void
GetMetrics (hWnd)
HWND hWnd;
{
    HDC         hDC;
    TEXTMETRIC	tm;

    hDC   = GetDC (hWnd);
    SelectObject( hDC, tracewin_font );
    GetTextMetrics (hDC, &tm);
    xChar = tm.tmAveCharWidth;
    yChar = tm.tmHeight + tm.tmInternalLeading/*+ tm.tmExternalLeading*/;
    ReleaseDC (hWnd, hDC);
}



char *
SIgetBufQLine(void)
{
    i2	wQueryLine;
    i2 	offset = (wClearLine ? -1 : 0);

    wQueryLine = wBufferCurrentLine + offset;
    if (wQueryLine < 0)
        wQueryLine += MAXLINES;
    return(wBuffer[wQueryLine].line);
}



static
void
SIunableToOpenLogFile(void)
{
    if ((MessageBox(NULL,
	    "Unable to open the w4gl log file. Do you want to continue?",
	    "Warning", MB_DEFBUTTON2 | MB_ICONQUESTION | MB_YESNO)) == IDNO)
    {
        PCexit( FAIL );
    }
    else
    {
        W4glErrorLog = NULL;
    }
}



/* Wrapper routine that retrieves the static isOpenROADStyle */
bool
GetOpenRoadStyle()
{
    return isOpenROADStyle;
}
