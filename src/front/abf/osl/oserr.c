/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<osglobs.h>
#include	<oserr.h>
#include	<osfiles.h>
#include	<ossym.h>

/**
** Name:    oserr.c -	OSL Parser Error Reporting Module.
**
** Description:
**	Contains routines used report errors for the OSL parser.  Defines:
**
**	osuerr()	report fatal error and quit.
**	oscerr()	report error with context.
**	osOpenwarn()	report open-SQL warning with context.
**	oswarn()	report warning with context.
**	osReportErr()	report error or warning with context.
**	oserrsummary()	summarize reported errors.
**
** History:
**	Revision 8.0  89/07  wong
**	Abstracted out 'osReportErr()', which supports severity level
**	to be passed to 'oserrline()'.
**
**	Revision 6.2  89/05/25  billc
**	Add 'osOpenwarn()' for reporting Open-SQL warnings.
**	89/03  wong  Completed support for listing files.
**
**	Revision 6.0  87/02/07  wong
**	Add input line number as last argument for fatal errors.
**	Added 'oswarn()'.  Modified to use 'IIUGfer()'.
**
**	Revision 5.1  86/10/17  16:14:14  wong
** 	Moved 'oserrsummary()' here.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF i4	osErrcnt;
GLOBALREF i4	osWarncnt;
GLOBALREF bool	osWarning;

VOID	osReportErr();

/*{
** Name:    osuerr() -	Report Fatal Error and Quit.
**
** Description:
**	Prints error message and exits.
**
** Input:
**	error	{ER_MSGID}  Error number.
**	N	{nat}  Number of arguments.
**	a1-a6	{PTR}  Arguments.
**
** History:
**	06/87 (jhw) -- Add line number as argument for all fatal errors.
*/
/* VARARGS2 */
VOID
osuerr (error, N, a1, a2, a3, a4, a5, a6)
ER_MSGID	error;
i4		N;
PTR		a1,a2,a3,a4,a5,a6;
{
    i4  line = osscnlno();

    ++osErrcnt;
    switch (N++)
    {
      case 0:
	a1 = (PTR)&line;
	break;
      case 1:
	a2 = (PTR)&line;
	break;
      case 2:
	a3 = (PTR)&line;
	break;
      case 3:
	a4 = (PTR)&line;
	break;
      case 4:
	a5 = (PTR)&line;
	break;
      case 5:
	a6 = (PTR)&line;
	break;
      case 6:
      default:
	--N;    /* do nothing */
	break;
    } /* end switch */

    osReportErr(error, _ErrorSeverity, N, a1, a2, a3, a4, a5, a6);
    osexit(FAIL);
}

/*{
** Name:	oscerr() -	Report Error with Context.
**
** Description:
**	Prints input line on which error occurred and the error message itself.
**
** Input:
**	error	{ER_MSGID}  Error number.
**	N	{nat}  Number of arguments.
**	a1-6	{PTR}  Arguments.
**
** Side Effects:
**	Increments the error count, 'osErrcnt'.
*/
/* VARARGS2 */
VOID
oscerr (error, N, a1, a2, a3, a4, a5, a6)
i4	error, N;
PTR	a1,a2,a3,a4,a5,a6;
{
	++osErrcnt;
	osReportErr(error, _ErrorSeverity, N, a1, a2, a3, a4, a5, a6);
}

/*{
** Name:    osOpenwarn() -	Report Open-SQL Warning with Context.
**
** Description:
**	Reports Open-SQL incompatibility warning and line on which
**	it was detected.
**
** Input:
**	stmt	{char *}  Statement name.
**
** Side Effects:
**	Increments the warning count, 'osWarncnt'.
**
** History:
**	05/89 (billc) -- Written.
*/
VOID
osOpenwarn (stmt)
PTR	stmt;
{
    if (osChkSQL)
    {
	++osWarncnt;
	osReportErr(OSNOTSQL0, _WarningSeverity, 1, stmt);
    }
}
/*{
** Name:    oswarn() -	Report Warning with Context.
**
** Description:
**	Prints input line to which warning applies and the warning itself.
**
** Input:
**	warning	{ER_MSGID}  Warning number.
**	N	{nat}  Number of arguments.
**	a1-6	{PTR}  Arguments.
**
** Side Effects:
**	Increments the warning count, 'osWarncnt'.
**
** History:
**	02/87 (wong) Written.
*/
/* VARARGS2 */
VOID
oswarn (warning, N, a1, a2, a3, a4, a5, a6)
ER_MSGID	warning;
i4		N;
PTR		a1,a2,a3,a4,a5,a6;
{
    if (osWarning)
    {
	++osWarncnt;
	osReportErr(warning, _WarningSeverity, N, a1, a2, a3, a4, a5, a6);
    }
}

/*{
** Name:	osReportErr() -	Report Error or Warning with Context.
**
** Description:
**	Prints an error or warning message and the line on which it occurred
**	with the last scanned positioned shown.
**
** Input:
**	error		{ER_MSGID}  Error or warning number.
**	severity	{ER_MSGID}  The severity level message.
**	N		{nat}  Number of arguments.
**	a1-6		{PTR}  Arguments.
**
** History:
**	07/89 (jhw) Abstracted from previous code.
*/
/* VARARGS3 */
VOID
osReportErr ( error, severity, N, a1, a2, a3, a4, a5, a6 )
ER_MSGID	error;
ER_MSGID	severity;
i4		N;
PTR		a1,a2,a3,a4,a5,a6;
{
	register FILE	*outfile;

	outfile = osLisfile != NULL ? osLisfile : osErrfile;

	oserrline(outfile, severity);
	IIUGfer(outfile, error, 0, N, a1, a2, a3, a4, a5, a6);

	if ( outfile == osErrfile )
		SIputc('\n', osErrfile);
}

/*{
** Name:    oserrsummary() -	Summarize OSL Parser Reported Errors.
**
** Description:
**	Print out a listing of all variables which were referenced
**	in the frame but not defined.
**
** Input:
**	form	{OSSYM *}  The form symbol reference.
*/
VOID
oserrsummary (form)
OSSYM	*form;
{
	register OSSYM	*fld;
	register OSSYM	*tbl;
	register i4	errcount = 0;
	register FILE	*file = osLisfile == NULL ? osErrfile : osLisfile;

	for (fld = form->s_fields ; fld != NULL ; fld = fld->s_sibling)
	{
		if (fld->s_kind == OSUNDEF)
			++errcount;
	}
	for (tbl = form->s_tables ; tbl != NULL ; tbl = tbl->s_sibling)
	{
		for (fld = tbl->s_columns ; fld != NULL ; fld = fld->s_sibling)
		{
			if (fld->s_kind == OSUNDEF)
				++errcount;
		}
	}
	if ( errcount > 0 )
	{
		SIfprintf(file, ERget(_UndefinedVars), errcount);
		for (fld = form->s_fields ; fld != NULL ; fld = fld->s_sibling)
		{
			if (fld->s_kind == OSUNDEF)
				SIfprintf(file, ERx("\t%s\n"), fld->s_name);
		}
		for (tbl = form->s_tables ; tbl != NULL ; tbl = tbl->s_sibling)
		{
			for ( fld = tbl->s_columns ; fld != NULL ;
					fld = fld->s_sibling )
			{
	    			if (fld->s_kind == OSUNDEF)
					SIfprintf( file, ERx("\t%s.%s\n"),
						tbl->s_name, fld->s_name
					);
			}
		}
	}
}
