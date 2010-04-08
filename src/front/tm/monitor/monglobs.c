# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<lo.h>
# include	"monitor.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  MONGLOBS.C -- global definitions of terminal monitor
**
**	History:
** Revision 60.24  87/04/08  01:38:09  joe
** Added compat, dbms and fe.h
** 
** Revision 60.23  87/01/23  13:49:09  peterk
** removed obsolete variable definitions.
** 
** Revision 1.3  86/08/02  17:55:03  perform
** Mon_intro and Mon_exit_msg moved from gver.roc as they are
** 	now system independent.
** 
** Revision 1.2  86/07/26  23:54:36  perform
** Properly initialize Ifiled.  New pyramid compiler removes symbol.
** 
**		9/10/80 (djf)    written for C compiler portability.
**			added.  Also, the RC_* stuff.
**		08/06/84 (Mike Berrow) --  Peekch becomes an i2 instead of a
**			char (According to latest UNIX 2.1)
**		09/11/84 (lichtman)	-- added dummy variables Reldcache,
**			Top_used_cache, and Reld_open_file for linking.
**		11/01/84 (dreyfus) -- added Term_format for newlines in output
**		12/10/84 (lichtman) -- added Do_macros, Savedomacros flags to
**			tell whether to use macro processing.  Two flags are
**			used because Do_macros is temporarily set to TRUE in
**			some cases; it is restored to Savedomacros if an
**			interrupt occurs.
**		08/05/85 (gb) -- zap Ifile_str_buf[] it is declared (and used
**			ONLY) in include.c so this was useless duplicate.
**			Trace flag ifdef tTcrash and tTintr (why are they
**			Qt, etc. in here?).
**		08/17/85 (peb)	Added Outisterm and Inisterm to support
**				multinational terminal translation.
**		10/30/85 (ericj) -- changed ifdefs of tTcrash and tTintr to
**			be xCRASH and xINTERRUPT respectively.  Included
**			trace.h to them on and off.
**		04/20/89 (kathryn) -- Err_terminate  - boolean flag
**			Terminate/Continue go_block after user error is
**			encountered.
**		09/28/89 (teresal) Added Runtime INGRES.
**		10/05/89 (teresal) Added '-partial' command flag.
**		11/28/89 (teresal) Removed Runtime INGRES flag - it is now
**			 defined in qrsql.c in QR.
**		03/19/90 (teresal) Add flag Qb_memory_full to indicate memory
**			 allocation failed for query buffer. Bug 9037.
**		12/21/92 (rogerl) Qryiop is no longer a FILE *.	Change to
**			struct qbuf *.
**		09/06/93 (rogerl) Declare Monincomment1; used for SQL style
**			comments.
**	14-sep-1995 (sweeney)
**	    get rid of "trigraph" in gb history above.
**      28-Nov-1997 (carsu07)
**          Switch_func - boolean flag to allow a change in functionality of
**          II_TM_ON_ERROR i.e. from terminating all further processing on an
**          error to terminating processing on the current \g block of
**          statements. (Bug #87415)
**      10-Aug-2009 (maspa05) trac 397, SIR 122324
**          Added Uservflag and Uservchar to track whether the user specified
**          a vertical delimiter and what it was, so we can revert if
**          necessary with \nosilent
*/

GLOBALDEF char	Qbname[30]	= {0};	/* pathname of query buffer */

/* flags */

GLOBALDEF i2	Nodayfile	= 0;	/* suppress dayfile/prompts */
					/* 0 - print dayfile and prompts
					** 1 - suppress dayfile but not prompts
					** -1 - supress dayfile and prompts
					*/
GLOBALDEF i2	Userdflag	= 0;	/* same: user flag */
					/*  the Nodayfile flag gets reset by
					**  include() this is the flag that
					**  the user actually specified (and
					**  what s/he gets when in interactive
					**  mode.
					*/
GLOBALDEF bool  Uservflag	= FALSE;/* Has the user specified -v for
                                        ** vertical delimiter?
                                        */
GLOBALDEF char  Uservchar[VDELIM_SIZE+1]   ="|";   /* character specified by -v flag */
GLOBALDEF char	Autoclear	= '\0';	/* clear query buffer automatically
					**	if set */
GLOBALDEF char	Notnull		= '\0';	/* set if the query is not null */
GLOBALDEF char	P_rompt		= '\0';	/* set if a prompt is needed */
GLOBALDEF char	Nautoclear	= '\0';	/* if set, disables the autoclear
					** option */
GLOBALDEF char	Phase		= '\0';	/* set if in processing phase */
GLOBALDEF i2	Term_format	= FALSE;/* don't format is default	*/
GLOBALDEF i4	Do_macros	= FALSE;/* process macros if set */
GLOBALDEF i4	Savedomacros	= FALSE;/* used to reset Do_macros in case of
					** an interrupt */

GLOBALDEF bool 	Err_terminate 	= FALSE; /* On error continue/terminate
					 ** go_block */
GLOBALDEF bool  Switch_func     = FALSE; /* On error terminate go_block or all
                                         ** further processing */
GLOBALDEF bool 	Partial_Access 	= FALSE; /* Partial Access to DBMS */
					 /* TRUE - Allow partial access to
					 **	   DBMS by ignoring FE catalogs.
					 ** FALSE - No partial access to DBMS.
					 */
/* query buffer stuff */

GLOBALDEF struct qbuf	*Qryiop		= NULL;	/* the query buffer */
GLOBALDEF char	Newline		= '\0';	/* set if last character was a newline*/
GLOBALDEF bool	Qb_memory_full	= FALSE; /* Memory alloc failed for query buf */

/* other stuff */

GLOBALDEF i4	Xwaitpid	= 0;	/* pid to wait on - zero means none */
GLOBALDEF i4	Error_id	= 0;	/* the error number of the last err */

/*
**  Support for multinational character sets
*/

GLOBALDEF bool	Outisterm	= FALSE;
GLOBALDEF bool	Inisterm	= FALSE;

/* \include support stuff */

GLOBALDEF FILE		*Input		= NULL; /* current input file */
GLOBALDEF i4		Idepth		= 0;	/* include depth */
GLOBALDEF char		Oneline		= '\0';	/* deliver EOF after one line
						**	input */
GLOBALDEF LOCATION	Ifile_loc[6]	= {0};	/* include file locations for
						**	rewinding--Ifile_str_buf
						**	now ONLY in include.c
						*/
GLOBALDEF FILE		*Ifiled[6]	= {0};	/* fd's for include files for
						**	longjmp */
GLOBALDEF i2		Peekch		= 0;	/* lookahead character for
						**	getch */

/* script file support */

GLOBALDEF FILE		*Script		= NULL;	/* script file descriptor */
GLOBALDEF i4		Moninquote	= FALSE;/* is the monitor inside a
						**	double-quoted string? */
GLOBALDEF i4		Monincomment	= FALSE;/* is the monitor inside a
						**	comment */
GLOBALDEF i4		Monincomment1	= FALSE;/* is the monitor inside an
						**	SQL-style comment */
