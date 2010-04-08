/* WN.h - windows-specific
**
**	note... requires <Windows.h>
**
*/


#ifndef WN_H
#define WN_H

struct	_MINMAXINFO
{
	POINTS	ptInternal;
	POINTS	ptMaxSize;
	POINTS	ptMaxPosition;
	POINTS	ptMaxTrackSize;
	POINTS	ptMinTrackSize;
};
#define	MINMAXINFO		struct	_MINMAXINFO

#define MAX_MENUBUTTON          10
#define WIDTH_BUTTON_TEXT       12

#define BUTTON_PRIOR            0       /* [<] */
#define BUTTON_NEXT             1       /* [>] */
#define BUTTON_NORMAL           2       /* [WIDTH_BUTTON_TEXT] */
#define BUTTON_WIDE             3       /* [WIDTH_BUTTON_TEXT*2] */

GLOBALREF HWND WNhWndMain;          /* main window handle */
GLOBALREF HWND WNhWndButton[MAX_MENUBUTTON];    /* button bar handles */
GLOBALREF HWND WNhWndMore[2];                   /* "<" and ">" button handles */
GLOBALREF HANDLE WNhInstance;       /* current instance handle */
GLOBALREF i2 WNnCmdShow;            /* window-show option */

#if 0   /* to be compatible with WINDOWS4GL */
GLOBALREF HBITMAP WNhCaretBitmap;   /* handle of CARET bit map */
#endif

GLOBALREF STATUS WNexitCode;        /* This will be loaded with the spawned  */
GLOBALREF i2 WNcxClient;            /* client area width */
GLOBALREF i2 WNcyClient;            /* client area height */
GLOBALREF i2 WNcxChar;              /* character width */
GLOBALREF i2 WNcyChar;              /* character height */
GLOBALREF i2 WNwinCols;             /* main window width in chars */
GLOBALREF i2 WNwinRows;             /* main window height in chars */
GLOBALREF i2 WNcursorCol;           /* cursor column coordinate */
GLOBALREF i2 WNcursorRow;           /* cursor row coordinate */

GLOBALREF i2 WNspawnLevel;	    /* This is a count of the current number */
				    /* of spawned programs - Inc'd by UTexe()*/
				    /* Dec'd by PCexit() of spawned program. */
GLOBALREF STATUS WNexitCode;	    /* This will be loaded with the spawned  */
				    /* program's return code before it exits.*/
GLOBALREF i2 WNparentMinSw;	    /* 1 -> parent should minimize itself    */
				    /*  clr'd by UTexe(), set on MainWin open*/
GLOBALREF i2 WNnHoldMsgs;	    /* Hold msgs for parent of spawned prog. */

GLOBALREF i2 WNfocus;               /* Set by SetFocus, Reset by KillFocus. */


/* Macros to convert relative (to 0) rows/columns to pixels */
#if 1
#define ROWTOY(r) (WNcyChar * ((r) + 2))  /* RBF */
#define YTOROW(y) (((y) / WNcyChar) - 2)  /* RBF */
#else
#define ROWTOY(r) (WNcyChar * ((r)))
#define YTOROW(y) (((y) / WNcyChar))
#endif
#define COLTOX(c) (WNcxChar * (c))
#define XTOCOL(x) ((x) / WNcxChar)


/* function prototypes */

#endif /* WN_H */
