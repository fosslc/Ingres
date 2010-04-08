# include	<si.h>
# include	<lo.h>
# include	<buf.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
**  MONITOR.H -- globals for the interactive terminal monitor
**
**	History:
**		3/2/79 (eric) -- Error_id and {querytrap} stuff
**			added.	Also, the RC_* stuff.
**		08/06/84 (Mike Berrow)-- Peekch becomes an i2 instead of a char
**					   (According to latest UNIX 2.1)
**		11/27/84 (SW) -- added # define for getch and getfilenm
**				 to invoke renamed RTI versions
**		12/10/84 (lichtman) -- added C_MACRO, C_NOMACRO, Do_macros,
**			Savedomacros for \nomacro command.
**		8/17/85 (peb)	Added Outisterm and Inisterm for
**				multinational support.
**		2/20/85 (garyb) -- changed tTf to type cast values for
**				comparison.
**		20-apr-89 (kathryn)	-- Added Err_terminate for go_block
**			to continue/terminate on user error.
**		30-sep-89 (teresal) -- IBM porting change - remove "Version[]".
**		30-sep-89 (teresal) -- PC porting change.
**		05-oct-89 (teresal) -- Added '-partial' flag for DBMS access.
**		9-jun-93 (rogerl) -- Add defines for SQL style comment parts
**              28-Nov-97 (carsu07)
**                   Added Switch_func to allow II_TM_ON_ERROR (when set to
**                   'nocontinue') to termintate processing go_block instead
**                   of all further processing. (Bug #87415)
**		17-Mar-99 (wanfr01) -- Add \suppress and \nosuppress to hide 
**			query output.  (for getting accurate qe90 results of 
**			large queries)
**		11-Aug-09 (maspa05) -- trac 397, SIR 122324
**			added commands associated with silent mode \silent,
**			\nosilent, \titles, \notitles, \vdelim, \trim, \notrim
**	26-Aug-2009 (kschendel) b121804
**	    macgetch prototype to keep gcc 4.3 happy.
**
*/

/* various global names and strings */
extern char		Qbname[30];	/* pathname of query buffer */

/* flags */
GLOBALREF bool		Err_terminate;
GLOBALREF bool          Switch_func;
GLOBALREF bool		Partial_Access;
extern i2		Nodayfile;	/* suppress dayfile/prompts */
				/* 0 - print dayfile and prompts
				** 1 - suppress dayfile but not prompts
				** -1 - supress dayfile and prompts
				*/
extern i2		Userdflag;	/* same: user flag */
				/*  the Nodayfile flag gets reset by include();
				**  this is the flag that the user actually
				**  specified (and what s/he gets when in
				**  interactive mode.			*/
extern char		Autoclear;	/* clear query buffer automatically if set */
extern char		Notnull;	/* set if the query is not null */
extern char		P_rompt;		/* set if a prompt is needed */
extern char		Nautoclear;	/* if set, disables the autoclear option */
extern char		Phase;		/* set if in processing phase */


/* query buffer stuff */
extern struct qbuf	*Qryiop;	/* the query buffer */
extern char		Newline;	/* set if last character was a newline */

/* other stuff */
extern i4		Error_id;	/* the error number of the last err */

extern bool		Inisterm;	/* If input is terminal for multinational terminal translation */
extern bool		Outisterm;	/* If output is terminal */

/* \include support stuff */
extern FILE		*Input;		/* current input file */
extern i4		Idepth;		/* include depth */
extern char		Oneline;	/* deliver EOF after one line input */
extern i2		Peekch;		/* lookahead character for getch */
extern LOCATION		Ifile_loc[6];	/* include file locations for rewinding */
extern FILE		*Ifiled[6];	/* fd's for include file longjmp handling */

/* commands to monitor */
# define	C_APPEND	1
# define	C_BRANCH	2
# define	C_CHDIR		3
# define	C_EDIT		4
# define	C_GO		5
# define	C_INCLUDE	6
# define	C_MARK		7
# define	C_LIST		8
# define	C_PRINT		9
# define	C_QUIT		10
# define	C_RESET		11
# define	C_TIME		12
# define	C_EVAL		13
# define	C_WRITE		14
# define	C_SHELL		15
# define	C_SCRIPT	16
# define	C_BEEP		17
# define	C_NOBEEP	18
# define	C_FORMAT	19
# define	C_NOFORMAT	20
# define	C_MACRO		21
# define	C_NOMACRO	22
# define	C_ECHO		23
# define	C_NOECHO	24
# define	C_QUEL		25
# define	C_SQL		26
# define	C_SEMICOLON	27
# define	C_NOSEMICOLON	28
# define	C_CONTINUE	29
# define	C_NOCONTINUE	30
# define	C_SUPPRESS	31
# define	C_NOSUPPRESS	32
# define	C_SILENT	33
# define	C_NOSILENT	34
# define	C_VDELIM	35
# define	C_TITLES	36
# define	C_NOTITLES	37
# define	C_PADDING	38
# define	C_NOPADDING	39
/* system control commands to the monitor */
# define	SC_TRACE	1
# define	SC_RESET	2

/* stuff for script logging */

extern	FILE	*Script;

# ifdef xMTR1
extern	short	tTttymon[];
# define	tTf(a,b)	(((i4)b<(i4)0) ? tTttymon[a] : (tTttymon[a] & ((i4)1<< (i4)b)))
# endif /* xMTR1 */

/* insert newlines after screen width exceeded if set */
extern	i2	Term_format;

/* flag to tell whether to do macro processing */
extern	i4	Do_macros;
extern	i4	Savedomacros;	/* reset Do_macros to Savedomacros in case of interrupt */


# define	QUOTE		'\"'
# define	SLASH		'/'
# define	STAR		'*'
# define	SQL_COMM_BEG	ERx('-')
# define	SQL_COMM_END	ERx('\n')
#ifdef CMS
# define	getch		tmgetch	 /* new name for RTI's routine */
# define	getfilenm	getfname /* new name for RTI's routine */
#endif
#ifdef PMFE
# define	getch		tmgetch	 /* PC porting addition 9/30/89 */
# define	getfilenm	getfname /* PC porting addition 9/30/89 */	
#endif

/* Function prototypes for tm/monitor */

FUNC_EXTERN char macgetch(void);
FUNC_EXTERN char *mcall(char *);

/* size of vertical delimiter in bytes 
** the intent is to allow for a char array to contain a single multi-byte 
** character. As far as I'm aware the largest UTF8 char is 6 bytes so 10 ought 
** to be more than enough. 
*/
#define		VDELIM_SIZE	10

