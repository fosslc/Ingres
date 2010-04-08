/*
** te.h
**
** Header file for Terminal I/O Compatibility library for
** Berkeley UNIX.
**
** Created 2-15-83 (dkh)
** 14-oct-87 (bab)
**      Added typedef of TEENV_INFO.
** 20-mar-1991 (fraser)
**      General cleanup of TE:
**        - Removed definitions used by tegbf.c
**        - Removed prototypes for static functions
** 04-jan-1995 (emmag)
**	Compiler errors forced the inclusion of checks as to whether
**	file had already been included or not.
** 24-sep-95 (tutto01)
**	Removed define for TEunbuf.  This function is defined in TE.
** 12-Dec-95 (fanra01)
**      Added definitions for extracting data for DLLs on windows NT and
**      using data from a DLL in code built for static libraries.
** 06-aug-1999 (mcgem01)
**      Replace nat and longnat with i4.
** 03-May-2001 (fanra01)
**      Sir 104700
**      Add TE_IO_MODE for direct stream I/O.
*/

#ifndef __TE_INCLUDED__
#define __TE_INCLUDED__

# include       <compat.h>              /* include compatiblity header file */
# include       <si.h>                  /* to get stdio.h stuff */

# ifndef        PMFEWIN3
# define        STDINFD         0       /* file descriptor for stdin on UNIX */
# define        STDOUTFD        1       /* file descriptor for stdout on UNIX*/
# define        STDERRFD        2       /* file descriptor for stderr on UNIX*/
# endif

/*
** Defined Constants
*/


/*
** Terminal characteristics you can set
*/

# define        TECRMOD         1

/* TEtest constants */

# define        TE_NO_FILE      0
# define        TE_SI_FILE      1
# define        TE_TC_FILE      2


/* TE return status codes. */

# define        TE_OK           0
# define        TE_TIMEDOUT     ((i4)-2)

/*
** Constants used by TErestore
*/

# define        TE_NORMAL       1
# define        TE_FORMS        2
# define        TE_CMDMODE      3
# define        TE_IO_MODE      4

/*
** The following are defines for the speeds that a terminal can run at.
** The list is longer than what UNIX supports due to VMS.
*/

# define        TE_BNULL        -1
# define        TE_B50          0
# define        TE_B75          1
# define        TE_B110         2
# define        TE_B134         3
# define        TE_B150         4
# define        TE_B200         5
# define        TE_B300         6
# define        TE_B600         7
# define        TE_B1200        8
# define        TE_B1800        9
# define        TE_B2000        10
# define        TE_B2400        11
# define        TE_B3600        12
# define        TE_B4800        13
# define        TE_B9600        14
# define        TE_B19200       15

/*
	Used by qrymod (protects) and journaling
*/

# define        TTYIDSIZE        8


extern  bool    GTE,     UPCSTERM;

/*
** Number of rows and columns as defined in the termcap file.
*/
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF       i2      TErows;
GLOBALDLLREF       i2      TEcols;

/*
** Max Width & Height of Main Window
*/

GLOBALDLLREF       i2      TExScreen;      /* Max Width of Main Window */
GLOBALDLLREF       i2      TEyScreen;      /* Max Height of Main Window */

GLOBALDLLREF       i4      TEstopPos;      /* Thumb Position at which to stop */
GLOBALDLLREF       i2      TEstopPaste;    /* 1 -> stop getting input from paste*/
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF       i2      TErows;
GLOBALREF       i2      TEcols;

/*
** Max Width & Height of Main Window
*/

GLOBALREF       i2      TExScreen;      /* Max Width of Main Window */
GLOBALREF       i2      TEyScreen;      /* Max Height of Main Window */

GLOBALREF       i4      TEstopPos;      /* Thumb Position at which to stop */
GLOBALREF       i2      TEstopPaste;    /* 1 -> stop getting input from paste*/
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/*}
** Name:        TEENV_INFO      - Terminal environment information.
**
** Description:
**      Terminal environment information.  Used by TEopen to return
**      information to the calling routine.  Currently contains
**      a delay constant and the number of rows and columns on
**      the terminal screen.  If rows (or cols) is zero (0), use
**      the value obtained by looking up TERM_INGRES in the termcap
**      file.  If rows (or cols) is greater than zero, then use this
**      value instead of the one obtained from termcap.  If rows
**      (or cols) is negative, then the absolute value is used to
**      indicate the number of rows (or cols) made unavailable to
**      the forms runtime system (i.e., subtract from termcap value).
**
**      Caveat:  This structure is not intended to be used for dynamically
**      re-sizing the forms system window size at runtime.
**
** History:
**
**      25-sep-87 (bab)
**              Created for use in TEopen.
**      06-mar-89 (rudy)
**              Changed   typedef struct {...} TEENV_INFO;
**              to
**                 typedef struct _TEENV_INFO {...} TEENV_INFO;
**
*/
typedef struct _TEENV_INFO{
	i4      delay_constant; /*
				**  A multiplier for determining the number
				**  of pad characters sent during terminal
				**  output.  Value based on terminal baud rate.
				*/
	i4      rows;           /*
				**  If positive, this is the number of rows
				**  to use for the terminal.  If zero, use
				**  value in termcap.  If negative, use
				**  absolute value to indicate number of
				**  rows made unavailable to the forms
				**  runtime system (subtract from value
				**  obtained from termcap).
				*/
	i4      cols;           /*
				**  If positive, this is the number of
				**  columns to use for the terminal.  If zero,
				**  use value in termcap.  If negative, use
				**  absolute value to indicate number of
				**  columns made unavailable to the forms
				**  runtime system (subtract from value
				**  obtained from termcap).
				*/
} TEENV_INFO;


/* function prototypes */

FUNC_EXTERN void    TEbrefresh(void);
FUNC_EXTERN void    TEerefresh(i4, i4, char **, char **, i4 *);
FUNC_EXTERN i2      TEgetmode(void);
FUNC_EXTERN bool    TEgtty(i4, char *);
FUNC_EXTERN void    TEinstrflush(char *);
FUNC_EXTERN i4      TEinitfp(FILE *, char);
FUNC_EXTERN i2      TElastKeyWasMouse(void);
FUNC_EXTERN i2      TEmousePos(i4 *, i4 *);
FUNC_EXTERN STATUS  TEname(char *);
FUNC_EXTERN VOID    TEonRing(bool);
FUNC_EXTERN VOID    TEread(void);
FUNC_EXTERN STATUS  TEswitch(void);
FUNC_EXTERN void    TEtermcap(i2,i2,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
FUNC_EXTERN VOID    TEwrite(char *, i4);
FUNC_EXTERN VOID    TEsetTitle(char *);
FUNC_EXTERN VOID    TEgokey(char *);
FUNC_EXTERN VOID    TEfindkey(char *);
FUNC_EXTERN VOID    TEmenukey(char *);
FUNC_EXTERN VOID    TEsetWriteall(i2);
FUNC_EXTERN VOID    TEbeginMove(i4 begx, i4 endx, i4 begy, i4 endy);
FUNC_EXTERN VOID    TEallowRestart(void);
FUNC_EXTERN VOID    TErestartMove(i4 begx, i4 endx, i4 begy, i4 endy);
FUNC_EXTERN VOID    TEendMove(void);
FUNC_EXTERN VOID    TEscreenLoc(i4, i4);
#endif  /* __TE_INCLUDED__ */
