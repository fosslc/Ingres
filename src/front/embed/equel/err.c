/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include 	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<si.h>
# include	<ex.h>
# include	<er.h>
# include	<lo.h>
# include	<fe.h>
# include	<ug.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<ereq.h>

/*
+* Filename:	err.c
** Purpose:	Error reporting routines.
**
** Defines:	er_write 	- Preprocessor interface to IIUG error routines.
**		er_exit		- Return error status for exit.
-*
** History:
**		07-nov-1984 	- Written (ncg)
**		19-apr-1985	- Rewritten to use standard Ingres files (ncg)
**		16-aug-1985	- Modified to allow entry from ESQL (ncg)
**		08-aug-1986	- Moved error text retrieval to "errgen.c"
**				  so it can be used w/o requiring
**				  'gen_ertab'. (jhw)
**		20-may-1987	- Updated for 6.0 compiled file error handling.
**				  (bjb)
**		29-jun-87 (bab)	Code cleanup.
**		18-aug-87	- Completed compiled error handling (bjb)
**		07-oct-93 	- Added <fe.h> now needed by <ug.h> (sandyd)
**		20-mar-96 (thoda04)
**		    Cast constant arguments of IIUGfmt() calls to proper nat.
**		    Cast constant arguments of IIUGfer() calls to proper nat.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



static i4	er_status = OK;		/* Error status for exiting */
static char	*er_emsg = (char *)0;  /* Linenum info for err msg */
static char	*er_wmsg = (char *)0;  /* Linenum info for warning msg */
static char	*er_fmsg = (char *)0;  /* Linenum info for fatal err msg */

/*
+* Procedure:	er_write 
** Purpose:	Interface to IIUG routines for printing preprocessor errors.
**
** Parameters:	ernum 		- Number of error message to print
**		severity 	- Severity level.  Preprocessor uses 3 levels:
**				  EQ_FATAL, EQ_WARN and EQ_ERROR.  EQ_FATAL
**				  will cause the program to stop.  EQ_FATAL
**				  and EQ_ERROR will set the program's return
**				  status to FAIL, EQ_WARN will not.
**		parcount	- Count of parameters
**		e1..e10		- Next (<=) 10 are error parameters.  In
**				  general the preprocessor routines always 
**				  send these as character strings;
**				  however, other types are permissible.
-* Returns:	None
**
** Based on whether Fatal or not, either stop or keep on going.
**
** Error Output Format Example:
**
** File f.qc, Line 1:
**	E_EQ0004_EeqSMALL: Buffer is too small for "abcdefgh".
*/

/* VARARGS3 */
void
er_write(ernum, severity, parcount, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10)
ER_MSGID	ernum;
i4		severity;
i4		parcount;
PTR		e1;
PTR		e2;
PTR		e3;
PTR		e4;
PTR		e5;
PTR		e6;
PTR		e7;
PTR		e8;
PTR		e9;
PTR		e10;
{

    char	er_buf[MAX_LOC +1];
    char	*ermsg;

    if (er_emsg == (char *)0)
    {
	if (eq->eq_infile == stdin)
	{
	    er_emsg = ERget(F_EQ0027_eqEMSGSTD);
	    er_wmsg = ERget(F_EQ0028_eqWMSGSTD);
	    er_fmsg = ERget(F_EQ0029_eqFMSGSTD);
	}
	else
	{
	    er_emsg = ERget(F_EQ0030_eqEMSGFILE);
	    er_wmsg = ERget(F_EQ0031_eqWMSGFILE);
	    er_fmsg = ERget(F_EQ0032_eqFMSGFILE);
	}
    }

    switch (severity) {
      case EQ_ERROR:
	ermsg = er_emsg;
	break;
      case EQ_WARN:
	ermsg = er_wmsg;
	break;
      case EQ_FATAL:
	ermsg = er_fmsg;
	break;
    }

    /* If output is to standard out, then try to start at a new line */
    if (eq->eq_outfile == ER_FILE && out_cur->o_charcnt > 0)
	SIfprintf( ER_FILE, ERx("\n") );

    /*
    ** Print file name and line number of error -if not a command line
    ** error.
    */
    if (eq->eq_line > 0)
    {
	i4	linenum = eq->eq_line;	/* IIUGfmt expects ptr to long i4  */

	if (eq->eq_infile == stdin)
	{
	    SIfprintf( ER_FILE, IIUGfmt(er_buf, (i4)MAX_LOC, ermsg, (i4)1,  
		    &linenum) );
	}
	else
	{
	    SIfprintf( ER_FILE, 
		IIUGfmt(er_buf, (i4)MAX_LOC, ermsg, (i4)3, eq->eq_filename,
		    eq->eq_ext, &linenum) );
	}
	/* 
	** er_buf includes the banner without a trailing new line.  The call
	** to IIUGfer implicitly precedes everything with a newline.
	** For the interactive session, we follow the message with an extra 
	** newline because the new 2-line error messages get confused with 
	** generated code.
	*/
    }

    IIUGfer( ER_FILE, ernum, (i4)0, parcount, e1, e2, e3, e4, e5, e6, e7, e8,
	    e9, e10 );
    if (eq->eq_infile == stdin)
	SIfprintf( ER_FILE, ERx("\n") );
    /*
    ** Write error message to list file if needed.  List file may be
    ** null if we are still processing the command line.
    */
    if ((eq->eq_flags & EQ_LIST) && eq->eq_listfile)
    {
	if (eq->eq_line > 0)
	    SIfprintf(eq->eq_listfile, er_buf);
	IIUGfer( eq->eq_listfile, ernum, (i4)0, parcount, e1, e2, e3, e4, e5,
		e6, e7, e8, e9, e10 );
	SIfprintf( eq->eq_listfile, ERx("\n") );
    }

    /* 
    ** FATAL and "general" errors cause the preprocessor to return
    ** a "FAIL" status; WARNings leave status as OK
    */
    if (severity == EQ_FATAL || severity== EQ_ERROR)
	er_status = FAIL;

    /* In case of a Fatal error try to clean up */
    if (severity == EQ_FATAL)
	EXsignal( EXFATER, 0 );

    /* Inform outputting routines of another 3 lines */
    if (eq->eq_flags & EQ_STDOUT)
	out_erline( 5 );
}

/*
+* Procedure:	er_exit 
** Purpose:	The main program should exit with an error status.
**
** Parameters:	None
-* Returns:	i4 - If there was an error FAIL, else OK.
*/

i4
er_exit()
{
# ifdef UNIX
    if (er_status == FAIL)
	er_status = -1;
# endif /* UNIX */
    return er_status;
}
