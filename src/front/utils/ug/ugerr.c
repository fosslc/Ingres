/*
** Copyright (c) 1987, 2008 Ingres Corporation
**
*/

#include	<compat.h> 
#include	<me.h>
#include	<pc.h>
#include	<si.h>
#include	<cm.h>
#include	<st.h>
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<sqlstate.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:	ugerr.c -	Front-End Utility Error Message Module.
**
** Description:
**	Contains routines used to display error messages.  Defines:
**
**	IIUGber()		error display to buffer
**	IIUGsber()		Same as IIUGber(), except returns SQLSTATE too.
**	IIUGerr()		error display
**	IIUGfer()		error display to file
**	IIUGeppErrorPrettyPrint() writes buffered errors to output.
**	IIUGfemFormatErrorMessage()	format an error message into buffer.
**
** History:
**	Revision 6.4  1991/06/27  wong
**	Modified logic in 'ugfrmerr()' to detect unformatted error messages
**	and avoid intermediate buffer overflow.
**
**	Revision 6.0
**	10-apr-1987 (peter)
**		Written.
**	20-aug-1987 (peter)
**		Add IIUGepp and IIUGret.  Also, change default to stdout.
**	08-aug-1987 (peter)
**		Add IIUGfem and removed IIUGretReturnErrorText.
**	02-nov-1987 (bab)
**		Declare IIUGber and IIUGfem to avoid compiler errors on DG.
**	08-nov-1988 (ncg)
**		Reduced overhead of IIUGfem.
**	mar-15-1989 (danielt)
**		changed ER arguments to PTRs from i4's (portability
**			problems)
**	08/12/91 (emerson)
**		Bug fix in IIUGber.
**	16-mar-93 (fraser)
**		Cast argument to function called via pointer.
**	06/07/93 (dkh) - Removed the ifdef TERMINATOR constructs since
**			 it is no longer needed and is NO longer defined
**			 with the the demise of dbms.h
**      10/06/94 (newca01) release 1.1 bug B62778 
**              Added compare to ugfrmerr to accept "W_" as a correct 
**              identifier for formatted messages.
**	04/23/96 (chech02)
**		cast integer constant to i4  in  STprintf for windows 3.1 port.
**	13-feb-1997 (donc)
**	    Call CL wrapper routine that determined if we are running in an 
**	    OpenROAD context and calls Error printing routine with a NULL
**	    stdout (which is the way OpenROAD expects things in order for
**	    tem to go to the Trace Window and Log File..
**	26-nov-1997 (donc) Bug 87195
**	    Apply last change to PrettyPrint also.
** 	2-dec-1997 (donc)
**	    Pull last change and do it in FEuterr instead.
**      23-dec-97 (fucch01)
**          Changed db_sqlst to db_sqlst->db_sqlstate as the 4th parameter
**          passed to ERslookup, due to a compile error on sgi_us5.
**	    Also had to cast indent_sp as PTR (char *) to get rid of 
**	    incompatible types compile errors.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      26-oct-2001 (loera01) Bug 106191
**          Fix error messages to support mutiple product builds.
**      08-nov-2001 (loera01)
**          Fixed a typo.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	19-may-2005 (abbjo03)
**	    If UG_ERR_FATAL is passed as severity, exit with FAIL, not -1.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

GLOBALREF DB_STATUS	(*IIugefa_err_function)();
GLOBALREF DB_STATUS	(*IIugmfa_msg_function)();

FUNC_EXTERN	bool	GetOpenRoadStyle();

VOID		iiuggeterr();
static VOID	ugfrmerr();
VOID		ugstderr();
VOID		IIUGber();
char		*IIUGfemFormatErrorMessage();

#define	UG_STDLINE	74	/* Size of line for std output */
static const char 	indent_sp[] = ERx("    "); /* Spaces to lead lines */
#define	UG_SPLEN	(sizeof(indent_sp)-1)	
#define	UG_LDNAME	8	/* Number of chars in name of error to keep */
#define UG_LDTIME	50	/* Max characters in timestamp */

/*{
** Name:	IIUGerr() -		Front-End Error Display Utility.
**
** Description:
**	This is the standard front end error utility, which can
**	be called by any of the front ends, or the forms system
**	or any layer in order to print out formatted numbered
**	error messages.  This can be called either in forms
**	mode or in non-forms mode, and an appropriate routine
**	will be called in either case, depending on whether
**	the forms system is on or not.
**
**	This will simply pass on control to IIUGfer to do the
**	work.
**
**	See IIUGfmt for details on the formatting syntax.
**
** Inputs:
**	msgid		- number of a message to print
**	severity	- Severity level.  If set to 0 or UG_ERR_ERROR,
**			  then a return is done.  If set to UG_ERR_FATAL
**			  then the program will abort.
**	parcount	- number of parameters to follow.
**	a1-a10		- parameters to the message.  These
**			  must either be pointers to character
**			  strings or PTRs to i4s and correspond
**			  to the numbered parameters in the message
**
** Returns:
**	Never if "severity == UG_ERR_FATAL".
**
** Exceptions:
**	If errors found in accessing the message files,
**	an exception will be generated with the error
**	return from the ER library.
**
** History:
**	01-apr-1987 (peter)	Written.
**	31-jul-1987 (peter)	Add parcount paramter..
**	05-aug-1987 (peter)	Add UG_ERR_FATAL flag.
**	20-aug-1987 (peter)	Change to use stdout.
*/

/*VARARGS3*/
VOID
IIUGerr (msgid, severity, parcount, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
ER_MSGID	msgid;
i4		severity;
i4		parcount;
PTR		a1;
PTR		a2;
PTR		a3;
PTR		a4;
PTR		a5;
PTR		a6;
PTR		a7;
PTR		a8;
PTR		a9;
PTR		a10;
{
    bool isOpenROADStyle;
    isOpenROADStyle = GetOpenRoadStyle();
    if (isOpenROADStyle)
    {
        IIUGfer((FILE *)NULL, msgid, severity, parcount,
		a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }
    else
    {
        IIUGfer(stdout, msgid, severity, parcount, 
	        a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 );
    }
}



/*{
** Name:    IIUGfer() -		Front-End Error Display to File Utility.
**
** Description:
**	This is the standard front end error utility, which can
**	be called by any of the front ends, to put error messages
**	to files in the standard format.
**
**	See IIUGfmt for details on the formatting syntax.
**
** Inputs:
**	file		- file to put error message to
**	msgid		- number of a message to print
**	severity	- Severity level.  If set to 0 or UG_ERR_ERROR,
**			  then a return is done.  If set to UG_ERR_FATAL
**			  then the program will abort.
**	parcount	- number of parameters sent.
**	a1-a10		- parameters to the message.  These
**			  must either be pointers to character
**			  strings or PTRs to i4s and correspond
**			  to the numbered parameters in the message
**
** Returns:
**	Never if "severity == UG_ERR_FATAL".
**
** Exceptions:
**	If errors found in accessing the message files,
**	an exception will be generated with the error
**	return from the ER library.
**
** History:
**	01-apr-1987 (peter)	Written.
**	05-aug-1987 (peter)	Add UG_ERR_FATAL flag.
**	19-may-2005 (abbjo03)
**	    Exit with FAIL not -1, if UG_ERR_FATAL.
*/
/*VARARGS4*/
VOID
IIUGfer (file, msgid, severity, parcount,
	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
FILE		*file;
ER_MSGID	msgid;
i4		severity;
i4		parcount;
PTR		a1;
PTR		a2;
PTR		a3;
PTR		a4;
PTR		a5;
PTR		a6;
PTR		a7;
PTR		a8;
PTR		a9;
PTR		a10;
{
    char		msg_buf[ER_MAX_LEN + UG_LDNAME + 4];
    i4			msg_len = ER_MAX_LEN;

    
    IIUGber(msg_buf, msg_len, msgid, severity, parcount,
	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);

    IIUGeppErrorPrettyPrint(file, msg_buf, FALSE);

    if (severity == UG_ERR_FATAL)
    {
	FEexits(ERx(""));
	PCexit(FAIL);
    }
    return;
}


/*
** Name:    IIUGber()		- format error to buffer.
**
** Description:
**	This entry point is called by users and IIUGerr to
**	move the error message to a buffer.  It is the standard
**	frontend to ERlookup used by the frontend products.
**	This hides many of the nasty parameters to ERlookup,
**	and allows parameters to be passed in as a list of PTRs.
**
**	The formatted message always contains the timestamp at
**	the start of the message.
**
** Inputs:
**	msg_len		Length of output buffer.
**	msgid		message number
**	severity	currently not used.
**	parcount	number of parameters in params.
**	params		Array of ER_ARGUMENTs to msgid.
**
** Outputs:
**	msg_buf		buffer to format message into.
**
** Exceptions:
**	If errors found in accessing the message files,
**	an exception will be generated with the error
**	return from the ER library.  Also, a message will
**	be written to standard output.
**
**  History:
**	07-apr-1987 (peter)
**		Written.
**	14-oct-1987 (peter)
**		Rewritten as IIUGber.
**	29-08-1988 (elein)
**		Changed stderr to stdout for exceptions
**	08/12/91 (emerson)
**		On second call to ERlookup (to report a bad message ID),
**		I changed the parameter count (the penultimate parameter) to 1.
**		Previously, it was parcount (the count left over from the
**		original call to ERlookup (that specified a bad message ID).
**		This sometimes caused horrible symptoms:  The messages
**		"Error in parameters to ERlookup" and "Internal error.
**		Report this problem to your technical representative."
**		would appear, and the terminal was sometimes left
**		in a weird state.  (Part of bug 37365).
**	29-oct-1992 (larrym)
**		Added functionality to return SQLSTATE.  Renamed the function
**		to IIUGsber(), for which IIUGber is now a cover.
*/
/*VARARGS5*/
VOID
IIUGber(msg_buf, msg_len, msgid, severity, parcount,
	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char		*msg_buf;
i4		msg_len;
ER_MSGID	msgid;
i4		severity;
i4		parcount;
PTR		a1;
PTR		a2;
PTR		a3;
PTR		a4;
PTR		a5;
PTR		a6;
PTR		a7;
PTR		a8;
PTR		a9;
PTR		a10;
{

DB_SQLSTATE	db_sqlst;
    /* we actually retrieve an sqlstate, but this function ignores it */
    IIUGsber(&db_sqlst, msg_buf, msg_len, msgid, severity, parcount,
	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    return;
}

/*
** Name:    IIUGsber()		- format error to buffer with SQLSTATE.
**
** Description:
**	This entry point is called by users and IIUGerr to
**	move the error message to a buffer.  It is the standard
**	frontend to ERslookup used by the frontend products.
**	This hides many of the nasty parameters to ERslookup,
**	and allows parameters to be passed in as a list of PTRs.
**
**	The formatted message always contains the timestamp at
**	the start of the message.
**
** Inputs:
**	msg_len		Length of output buffer.
**	msgid		message number
**	severity	currently not used.
**	parcount	number of parameters in params.
**	params		Array of ER_ARGUMENTs to msgid.
**
** Outputs:
**	db_sqlst	SQLSTATE returned by ERslookup.
**	msg_buf		buffer to format message into.
**
** Exceptions:
**	If errors found in accessing the message files,
**	an exception will be generated with the error
**	return from the ER library.  Also, a message will
**	be written to standard output.
**
**  History:
**	07-apr-1987 (peter)
**		Written.
**	14-oct-1987 (peter)
**		Rewritten as IIUGber.
**	29-08-1988 (elein)
**		Changed stderr to stdout for exceptions
**	08/12/91 (emerson)
**		On second call to ERlookup (to report a bad message ID),
**		I changed the parameter count (the penultimate parameter) to 1.
**		Previously, it was parcount (the count left over from the
**		original call to ERlookup (that specified a bad message ID).
**		This sometimes caused horrible symptoms:  The messages
**		"Error in parameters to ERlookup" and "Internal error.
**		Report this problem to your technical representative."
**		would appear, and the terminal was sometimes left
**		in a weird state.  (Part of bug 37365).
**	29-oct-1992 (larrym)
**		Added functionality to return SQLSTATE.  Renamed the function
**		to IIUGsber(), for which IIUGber is now a cover.
*/


/*VARARGS5*/
VOID
IIUGsber(
    DB_SQLSTATE	*db_sqlst,
    char		*msg_buf,
    i4		msg_len,
    ER_MSGID	msgid,
    i4		severity,
    i4		parcount,
    PTR		a1,
    PTR		a2,
    PTR		a3,
    PTR		a4,
    PTR		a5,
    PTR		a6,
    PTR		a7,
    PTR		a8,
    PTR		a9,
    PTR		a10
)

{
    ER_ARGUMENT		params[10];		/* error arguments */
    CL_SYS_ERR		sys_err;		/* Used to pass to ERlookup */
    register i4	index;			/* temp index */
    DB_STATUS		ret_status;
    i4			len = msg_len;		/* length of message */

    /*
    **	First, move the parameters into an ER_ARGUMENT structure.
    */

    for (index = 0 ; index < parcount ; ++index)
    {	/* Put in the indicator that the parameter is a pointer */
	params[index].er_size = ER_PTR_ARGUMENT;
    }
    params[0].er_value = a1;
    params[1].er_value = a2;
    params[2].er_value = a3;
    params[3].er_value = a4;
    params[4].er_value = a5;
    params[5].er_value = a6;
    params[6].er_value = a7;
    params[7].er_value = a8;
    params[8].er_value = a9;
    params[9].er_value = a10;

    /* Call ERlookup to get the error text */

    ret_status = ERslookup(msgid, (CL_SYS_ERR *)NULL, ER_TIMESTAMP,
         db_sqlst->db_sqlstate, msg_buf, msg_len, iiuglcd_langcode(),
         &len, &sys_err, parcount, params);
    if (ret_status == ER_NOT_FOUND || ret_status == ER_BADCLASS)
    { /* Message not found.  Get generic message */
	ER_MSGID	baderr;

	params[0].er_size = ER_PTR_ARGUMENT;
	params[0].er_value = (PTR) &msgid;
	baderr = (ret_status == ER_NOT_FOUND) ?
			E_UG0002_Bad_Msgid : E_UG0003_Bad_Classid;
	/* If we couldn't get the original error, then we won't get SQLSTATE */
	STlcopy (SS5000K_SQLSTATE_UNAVAILABLE, db_sqlst->db_sqlstate,
	     DB_SQLSTATE_STRING_LEN);

	ret_status = ERlookup(baderr, (CL_SYS_ERR*)NULL, ER_TIMESTAMP, NULL, msg_buf,
			msg_len, iiuglcd_langcode(), &len, &sys_err, 1, params);
    }

    if (ret_status != ER_OK)
    { /* This is pretty bad.  Better get out. */
	static char	_ChkLogicals[] =
		ERx("Check %s_MSGDIR and %s_LANGUAGE logical variables.\r\n");
	static char	_ChkInstallation[] =
		ERx("Check messages files in the installation.\r\n");
	static char	_Internal[] =
ERx("Internal error.  Report this problem to your technical representative.\r\n");

	char	buf[UG_STDLINE + 1];

	SIfprintf(stdout,ERx("\rERlookup:  Error accessing message text:\r\n"));
	switch (ret_status)
	{
	  case ER_NO_FILE:
	    SIfprintf(stdout, ERx("No message file found.\r\n"));
            SIfprintf(stdout, _ChkLogicals,SystemVarPrefix, SystemVarPrefix);
	    SIfprintf(stdout, _ChkInstallation);
	    break;
	  case ER_BADOPEN:
	    SIfprintf(stdout, ERx("Error opening message file.\r\n"));
            SIfprintf(stdout, _ChkLogicals,SystemVarPrefix, SystemVarPrefix);
	    SIfprintf(stdout, _ChkInstallation);
	    break;
	  case ER_BADPARAM:
	    SIfprintf(stdout, ERx("Error in parameters to ERlookup.\r\n"));
	    SIfprintf(stdout, _Internal);
	    break;
	  case ER_TRUNCATED:
	    SIfprintf(stdout, ERx("Truncation of system error.\r\n"));
	    SIfprintf(stdout, _Internal);
	    break;
	  case ER_DIRERR:
	    SIfprintf(stdout, ERx("Error accessing message directory.\r\n"));
            SIfprintf(stdout, _ChkLogicals,SystemVarPrefix, SystemVarPrefix);
	    break;
	  case ER_NOALLOC:
	  case ER_NOFREE:
	    SIfprintf(stdout, ERx("Error in memory allocation.\r\n"));
	    SIfprintf(stdout, _Internal);
	    break;
	  case ER_BADLANGUAGE:
	    SIfprintf(stdout, ERx("Bad language value for %s_LANGUAGE.\r\n"),
                SystemVarPrefix);
            SIfprintf(stdout, _ChkLogicals,SystemVarPrefix, SystemVarPrefix);
	    break;
	  default:
	    SIfprintf(stdout, ERx("For unknown reasons.\r\n"));
	    SIfprintf(stdout, _ChkInstallation);
	    SIfprintf(stdout, _Internal);
	    break;
	} /* end switch */
	SIfprintf(stdout, ERx("\n%s ERROR:  %ld\r\n"), SystemProductName,msgid);
	SIflush(stdout);

	/* DOES NOT RETURN */
	EXsignal(EXFEBUG, 1, STprintf(buf, ERx("ERlookup:  %d"), ret_status));
    }
    return;
}



/*{
** Name:    IIUGeppErrorPrettyPrint() -	print out error buffer
**
** Description:
**	Given a buffer containing an error message in the following
**	format:
**	  (timestamp) MSGID error text
**	this will print out the error in an attractive way for both
**	forms and non-forms programs.
**
**	For forms programs, a one line description is given at the
**	bottom of the screen, and the user can use the HELP key to
**	see more text of the message.
**
**	For non-forms programs, the error is printed in full with
**	the error number prominently displayed.
**
** Inputs:
**	file	{FILE*}		A pointer to file to put message to.
**	errbuf	{char *}	Pointer to error buf containing
**				error buffer with leading timestamp
**				and error text (as printed by
**				ERlookup)
**	printtime	{bool}	Set to TRUE if the time stamp is
**				to be appended to the message, or
**				FALSE if it is not.  This is set
**				to TRUE for backend system errors.
**
** Side Effects:
**	The input buffer 'errbuf' may be overwritten in the underlying
**	routines.
**
** History:
**	16-aug-1987 (peter)
**		Written.
**	14-oct-1987 (peter)
**		Added the printtime parameter.
*/

VOID
IIUGeppErrorPrettyPrint (file, errbuf, printtime)
FILE	*file;
char	*errbuf;
bool	printtime;
{
    /*
    **	Split here for forms and non-forms processing.
    */

    /* First get the start of name and text */

    if (IIugefa_err_function != NULL && (file == stdout || file == stderr))
    {
	ugfrmerr(errbuf, printtime);
    }
    else
    {	/* Must be batch program.  Simply print out to file */
	ugstderr(file, errbuf, printtime);
    }
    return;
}


/*
** Name:    ugfrmerr() -	write error to forms system.
**
** Description:
**	This routine writes errors for forms system programs.
**	It first breaks out the error number and the text, 
**	and puts them into separate buffers for use by the
**	IIboxerr routine.
**
** Inputs:
**	errbuf		- pointer to message buffer.
**	printtime	- TRUE if timestamp to be printed.
**
** Outputs:
**	VOID
**
** History:
**	18-aug-1987 (peter)
**		Written.
**	14-oct-1987 (peter)
**		Updated parameters.  Added printtime.
**	27-jun-1991 (jhw)  Modified logic to detect unformatted messages to
**		avoid overflow of the 'etime' and 'msg_name' buffers.
**	18-oct-1992 (mgw)
**		Fixed previous change to include "S_" error messages in
**		search for legitimately formatted messages.
**	16-mar-93 (fraser)
**		Added cast to call of (*IIugmfa_msg_function)()
**	16-jun-1993 (rdrane)
**		Correct extraction of etime.  The timestamp is of variable
**		length (cp - errbuf), and not fixed max double-byte length
**		UG_LDTIME.  This corrects bug 49889.
**      10/06/94 (newca01) release 1.1 bug 62778
**              Added code to include "W_" error messages in search for 
**              legitimately formatted messages.
*/

#define	FMT_LENGTH	4	/* sizeof(" ()") [includes EOS] */

static
VOID
ugfrmerr ( errbuf, printtime )
char	*errbuf;
bool	printtime;
{
    char	msg_buf[ER_MAX_LEN + UG_LDTIME + UG_LDNAME + 2*FMT_LENGTH];
    char	msg_name[UG_LDNAME + FMT_LENGTH];	/* buffer for name */
    char 	etime[UG_LDTIME + FMT_LENGTH];		/* buffer for time */	
    register char	*cp;
    i4			len;

    /*
    ** Add check for "W_" in error message 10/06/94 newca01 B62778
    ** First, check for "E_" or "S_" in error message to determine if it is
    ** a formatted message.  (Generally, unformatted messages will be present
    ** if an error occurred in reading the message files.)
    */

    for ( cp = errbuf ; *cp != EOS ; CMnext(cp) )
    {
	if ( (*cp == 'E' || *cp == 'S' || *cp == 'W') && *(cp+1) == '_' )
		break;	/* found it */
    }

    if ( *cp == EOS )
    { /* didn't detect formatted message */
	printtime = FALSE;
	STcopy(ERx(" ()"), msg_name);
    }
    else
    { /* ... save message name, timestamp from formatted message */
	/* "%.*s" is used in the following string formatting to avoid overflow
	** of the intermediate ('etime', 'msg_name') buffers.
	*/
	len = (cp - errbuf);
	if  (len > UG_LDTIME)
	{
		len = UG_LDTIME;
	}
	_VOID_ STprintf(etime, ERx("\n(%.*s"), len, errbuf);
	_VOID_ STtrmwhite(etime);
	_VOID_ STcat(etime, ERx(")"));

	/* Now, move the name. */
	_VOID_ STprintf( msg_name, ERx(" (%.*s)"), (i4)UG_LDNAME, cp );
	cp += STlength(msg_name) - FMT_LENGTH;

	/* Now, skip over the rest to get to the text of the message */
	while ( *cp != EOS && !CMwhite(cp) )
	{
		CMnext(cp);
	}
	if (CMwhite(cp))
	{	/* Skip over the first white space */
		CMnext(cp);
	}
    }

    /*
    ** Now construct the message from the text, the parenthesized name,
    ** and the optional timestamp.
    */

    len = STlength(msg_name);
    if (printtime)
    {
	len += STlength(etime);
    }

    _VOID_ STlcopy(cp, msg_buf, (i2) (sizeof(msg_buf) - len));
    _VOID_ STcat(msg_buf, msg_name);
    if (printtime)
    {
	_VOID_ STcat(msg_buf, etime);
    }

    if ( (*IIugefa_err_function)(msg_buf, msg_buf) != OK )
    { /* Try to simply print it out */
	(*IIugmfa_msg_function)(msg_buf, (bool) TRUE);
    }
}

/*
** Name:    ugstderr() -	write error to standard error.
**
** Description:
**	This routine writes errors to standard error.  It uses
**	the IIUGfemFormatErrorMessage to format the error, and
**	then simply prints it out.
**
** Inputs:
**	file		- file for output
**	errbuf		- buffer, from IIUGber, of error text.
**	printtime	- TRUE if timestamp to be printed.
**
** Outputs:
**	none.
**
** History:
**	04-apr-1987 (peter)
**		Written.
*/

static bool	isopen = FALSE;

VOID
ugstderr (file, errbuf, printtime)
FILE	*file;
char	*errbuf;
bool	printtime;
{
    char	newbuf[ER_MAX_LEN * 2];
    i4		newsize;

    newsize = ER_MAX_LEN +2;

    _VOID_ IIUGfemFormatErrorMessage(newbuf, newsize, errbuf, printtime);

    /* Check if output is open.
    **
    ** This is for non-C EQUEL (or ESQL) programs on non-Unix systems, which do
    ** not necessarily open up 'stdout' or 'stderr', and that did not start up
    ** the DBMS or the Forms system (i.e., through "## ingres"), which do open
    ** up 'stdout' or 'stderr', before calling the LIBQ or RUNTIME interfaces.
    **
    ** Note:  This is necessary only because some users are stupid and do not
    ** read the manual.  Also, this does nothing for the case when users closed
    ** either 'stdout' or 'stderr'.  Finally, this probably should be done as
    ** part of 'IIUGber()', which may try to print to 'stderr' if it couldn't
    ** get the error message, but if users screw up that badly, they shouldn't
    ** be touching computers anyway.
    */
    if (!isopen && (file == stderr || file == stdout))
	isopen = (SIeqinit() == OK);

    SIfprintf(file, ERx("%s\n"), newbuf);
    SIflush(file);

    return;
}



/*
** Name:    IIUGfemFormatErrorMessage() -	format error for output.
**
** Description:
**	This routine formats errors for output.  This uses
**	wrapping formatting to do it's trick.  Input to this routine
**	is the return buffer from ERlookup, in the following
**	format:
**
**	 (timestamp) long-name text
**
**	(Note that the default error message if an error can't be
**	found does not match this format. This routine has to account
**	for that.)
**	This rearranges the pieces of the text for much more attractive
**	output formatting.  It shortens the name to the first 8 characters,
**	wraps the text at word boundaries, and optionally puts the timestamp on
**	a separate line at the end of the message.
**
**	The output for the routine goes into the user buffer specified.
**	The length is given for safety, as many extra blanks and
**	newline characters may be added in putting this together.
**	A good rule of thumb is 2*ER_MAX_LEN.
**	The new buffer is always ended with a newline character 
**	before the end of string, even if it is a single line.  This
**	means that it is ready for output through SIprintf.
**
**	The formatting done uses the following idea.  The first line
**	of the message starts in column 1, and all subsequent lines
**	are offset by the string indent_sp.  In order to do this
**	and keep the justified formatting with the first line and
**	the subsequent lines, the first few characters (the length
**	of indent_sp characters) are kept off on the side and the
**	formatting routines are called to formatted the shifted
**	text, in order to keep the borders straight.  Then, on the
**	first line, the stashed away characters are printed out.
**
** Inputs:
**	newsize	{nat}		- size of new buffer, in bytes.
**				  This routine does not check sizes
**				  and the buffer should be 2*sizeof(errbuf).
**				  A typical size is 2*ER_MAX_LEN.
**	errbuf	{char *}	- text of error message.
**	printtime {bool{	- TRUE if timestamp to be used.
**
** Outputs:
**	newbuf {char *}		- new buffer, fully formatted.
**
**	Returns:
**		pointer to newbuf.
**
** History:
**	04-oct-1987 (peter)
**		Written.
**	02-nov-1987 (neil)
**		Added the "\r" character which is default in C but not in
**		other languages.
**	19-jan-1988 (joe)
**		Made to handle an input without a time or an E_XXXX at
**		that start.  This can happen with the default error
**		when an error number isn't found.  Changed to only allow
**		UG_LDTIME characters for the time string.
**	16-nov-1988 (neil)
**		Modified routine to not use fmt for line wrapping and stop
**		generating the '\r' character.
**	15-mar-89 (kathryn)	Bug #4978  (Error Message Corruption)
**		Now splits an error message correctly onto two lines.
**		-sp should be compared with ocp not cp.
**	22-mar-94 (geri)
**		Bug 60406: error messages which are the same size as
**		UG_STDLINE are no longer truncated.
**		
*/

char	*
IIUGfemFormatErrorMessage(newbuf, newsize, errbuf, printtime)
char	*newbuf;
i4	newsize;			/* IGNORED */
char	*errbuf;
bool	printtime;
{
	register char	*cp;			/* Pointer to input buffer */
	register char	*ocp;			/* 	   to output buffer */
	register i4	size;
	register char	*sp;			/* Space marker */
	char		etime[UG_LDTIME+5];	/* Start of buffer for time */	
	bool		gotnum = FALSE;	/* If we found the _ of the number */

    /*
    ** Move the characters where the time should be into the etime
    ** buffer.  Stop when we run out of characters in the input, see
    ** a '_' (which should be the E_XXXX), or fill up etime.  We assume
    ** that the time will fit in UG_LDTIME bytes (note that the
    ** code looks like it is allowing only UG_LDTIME-1 bytes, but
    ** if the last character is a double-byte character it will take
    ** up UG_LDTIME bytes).
    */
    etime[0] = '(';
    ocp = &etime[1];
    for (cp = errbuf; (*cp != '_' && *cp != EOS && ocp <= &etime[UG_LDTIME]);
	 CMnext(cp))
    {
	CMcpychar(cp, ocp);
	CMnext(ocp);
    }
    /*
    ** If we got to the '_', we went too far. Chop off the E or W or
    ** I preceding it.  Add a newline as well.
    */
    if (*cp == '_')
    {
	gotnum = TRUE;
	cp--;			/* Move back so cp is pointing at E_ */
	ocp--;
    }
    else if (ocp > &etime[UG_LDTIME])
    {
	/*
	** If we went passed the end of the time stamp buffer,
	** assume there was a problem in getting the time stamp.
	** Delete all and just put an empty time stamp.
	*/
	ocp = &etime[1];
	cp = errbuf;
    }
    *ocp = EOS;
    STcopy( ERx(")\n"), etime + STtrmwhite(etime) );

    /* Start up the output buffer */
    ocp = newbuf;
    *ocp = EOS;

    /*
    ** If we think we hit the error number, then copy a shortened
    ** portion to the output buffer.  Otherwise just treat the
    ** entire message as text.
    */
    if (gotnum)
    {
	cp += STlcopy(cp, ocp, UG_LDNAME);	/* Copy the number/name */
	ocp += UG_LDNAME;	/* assumes UG_LDNAME bytes were copied */
	while (CMnmchar(cp))		/* Skip rest of name and blank */
	   CMnext(cp);
	while (CMwhite(cp))
	    CMnext(cp);
	*ocp++ = ' ';			/* Add blank to output */
    }

    /* Copy first short part of message - supposed to be 1-liner
    ** but in case it's not, make sure we don't copy more than a
    ** standard line.
    */
    size = UG_LDNAME + 1;
    while ( *cp != '\n' && *cp != EOS && size < UG_STDLINE )
    {
	if ( *cp == ' ' )
		sp = ocp;
	CMcpyinc(cp, ocp);
	++size;
    }
    
    if ( *cp != '\n' && *cp != EOS )
    { /* longer than a line; assumes there was a space */
	if ( *cp == ' ' )
	{ /* break where we are */
		sp = ocp++;
		++cp;
	}
	else if (ocp > sp + 1 )			/* Bug 4978 */
	{ /* break where we were */
		register char	*m1p = ocp;
		register char	*m2p = ocp + UG_SPLEN;

		while ( m1p > sp )
			*m2p-- = *m1p--;
	}
	*sp++ = '\n';
	MEcopy((PTR)indent_sp, UG_SPLEN, sp);
	ocp += UG_SPLEN;
    }
    else
    {
	*ocp++ = '\n';
    }

    if ( *cp != EOS )
    { /* More to the message */
	if ( *cp == '\n' )
	{
		*cp++;			/* Skip the newline */
		while ( CMwhite(cp) )	/* Skip early blanks */
			CMnext(cp);
		/* Add some indent */
		if (*cp != EOS)
		{
		    MEcopy((PTR)indent_sp, UG_SPLEN, ocp);
		    ocp += UG_SPLEN;
		}
	}
	while (STlength(cp) > UG_STDLINE)	/* Format long lines */
	{
	    register char	*xsp;		/* Space marker */

	    /* Check for last space in line (or end of line) */
	    for (sp = (char *)0, xsp = cp; xsp < cp + UG_STDLINE-1;)
	    {
		if (*xsp == '\n')
		{
		    sp = xsp;
		    break;
		}
		if (*xsp == ' ')
		    sp = xsp;
		CMnext(xsp);
	    }
	    if (sp == (char *)0)	/* None found so break mid-stream */
		sp = xsp;
	    MEcopy(cp, sp-cp, ocp);
	    ocp += sp-cp;
	    *ocp++ = '\n';		/* Add end-of-line */
	    /* Add some indent */
	    MEcopy((PTR)indent_sp, UG_SPLEN, ocp);
	    ocp += UG_SPLEN;
	    cp = sp;		/* Mid-stream break */
	    if (*sp == ' ' || *sp == '\n')	/* Replaced by newline */
		++cp;
	}
	if (*cp != EOS)
	{
	    STcopy(cp, ocp);		/* Copy remaining text */
	    ocp += STlength(cp);
	}
	*ocp++ = '\n';			/* Add trailing new-line */
    }
    *ocp = EOS;

    /* Put in the time stamp on a separate line if printtime is TRUE */
    if (printtime)
    {
	MEcopy((PTR)indent_sp, UG_SPLEN, ocp);
	ocp += UG_SPLEN;
	STcopy(etime, ocp);
	ocp += STlength(etime);
    }
    return newbuf;
}
