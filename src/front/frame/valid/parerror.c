/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h> 
# include	<ex.h> 
# include	<ug.h>
# include	<er.h> 
# include	<frserrno.h>

/*
**   PAR_ERROR -- the error routine for the parser
**
**	Par_error sends its arguments to print_error(), the front-end
**	error processor.  Errors are assumed to be fatal.
**
**	Returns:
**		never returns. (signals exception)
**
**	Called By:
**		parser routines
**		scanner routines
**
**  History:
**	09/05/87 (dkh) - Changes for 8000 series error numbers.
**	06-jul-88 (sylviap) 
**		Created vl_nopar_error for errors that are NOT parser
**		errors.
**	17-may-89 (sylviap) 
**		Changed parameters for vl_nopar_error.  Deleted 'msg'.
**		Not needed.
**	06/23/89 (dkh) - Changes for derived field support.
**	10/02/89 (dkh) - Changed IIFVderrflag to IIFVdeflag.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


GLOBALREF 	i4	IIFVverrflag;
GLOBALREF 	i4	IIFVdeflag;

static 	char	bufr[ER_MAX_LEN + 1] = {0};

/* VARARGS3 */
vl_par_error(procname, fmt, p1, p2, p3, p4, p5)
char	*procname;
char	*fmt;
i4	p1;
i4	p2;
i4	p3;
i4	p4;
i4	p5;
{
	char	*cp = bufr;

	IIFVverrflag = 1;		/* tell yyparse, error detected */
	STprintf(cp, fmt, p1, p2, p3, p4, p5);
	IIFDerror(VALERR, 5, procname, vl_curfrm->frname,
		vl_curhdr->fhdname, cp, Lastok);

	EXsignal(EXVALJMP, 0);
}

vl_nopar_error(fmt, par_no, p1, p2, p3, p4, p5)
ER_MSGID	fmt;
i4	par_no;
PTR	p1;
PTR	p2;
PTR	p3;
PTR	p4;
PTR	p5;
{
	IIFVverrflag = 1;		/* tell yyparse, error detected */
	IIUGfmt(bufr, ER_MAX_LEN, ERget(fmt), par_no, p1, p2, p3, p4, p5);
	IIFDerror(fmt, par_no, p1, p2, p3, p4, p5);
	EXsignal(EXVALJMP, 0);
}

char *
FDpar_str()
{
	return (bufr);
}



/*{
** Name:	IIFVdsmDrvSetMsg - Set derivation error message.
**
** Description:
**	Save away passed in error message (in "bufr") for later
**	use by derivation error recovery frame of Vifred.
**
** Inputs:
**	msg	Error message to save away into "bufr".
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
**	07/21/89 (dkh) - Initial version.
*/
VOID
IIFVdsmDrvSetMsg(msg)
char	*msg;
{
	STcopy(msg, bufr);
}


/*{
** Name:	IIIFVdpError - Display error message.
**
** Description:
**	Formats passed in error message and its parameters into local
**	buffer and displays it to user at the terminal.  Message
**	is normally a derivation parser error message.
**
** Inputs:
**	msgid		Id of message to display.
**	numarg		Number of parameters passed in to be used in
**			formatting the error message.
**	a1		Parameter 1.
**	a2		Parameter 2.
**	a3		Parameter 3.
**	a4		Parameter 4.
**	a5		Parameter 5.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Exits back to caller of the derivation parser via the
**	exception mechanism..
**
** History:
**	07/21/89 (dkh) - Initial version.
*/
VOID
IIFVdpError(msgid, numarg, a1, a2, a3, a4, a5)
ER_MSGID	msgid;
i4		numarg;
PTR		a1;
PTR		a2;
PTR		a3;
PTR		a4;
PTR		a5;
{
	IIFVdeflag = 1;		/* tell yyparse, error detected */
	IIUGfmt(bufr, ER_MAX_LEN, ERget(msgid), numarg, a1, a2, a3, a4, a5);
	IIFDerror(msgid, numarg, a1, a2, a3, a4, a5);
	EXsignal(EXVALJMP, 0);
}
