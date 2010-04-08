/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<st.h>
# include	<ug.h>
# include	<pv.h> 
# include	<gtdef.h>

/*
** Name:	gterror.c -	GT Error Handling Module.
**
** Description:
**	Contains routines that output error messages for the Graphics system.
**
**	GTerror()	interface to 'IIUGerror()' in alpha mode
**	GTperror()	GTerror with pause.
**	GTsyserr()	system error in alpha mode
**
** History:
**		Revision 40.101  86/04/09  13:17:32  bobm
**		LINT changes
**		
**		Revision 40.100  86/02/26  17:45:11  sandyd
**		BETA-1 CHECKPOINT
**		
**		Revision 40.0  85/12/31  13:56:47  wong
**		Initial revision.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

extern bool	G_interactive;

extern i4	(*G_mfunc)();	/* Mode function pointer (referenced) */

/*{
** Name:	GTerror() -	Error interface for terminal in graphics mode
**
**	Force alphamode, produce 1 line message message
**
** History:
**	12/85 (jhw) -- Copied from "frame/error.c".
**	9/87 (bobm) -- Redone for ER
*/

GTerror(errnum, numargs, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
	ER_MSGID	errnum;
	i4	numargs;
	PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
	if (G_mfunc != NULL)
		(*G_mfunc)(ALPHAMODE);
	IIUGmsg(ERget(errnum), FALSE, numargs,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);
}

/*{
** Name:	GTperror() -	Error interface for terminal in graphics mode
**
**	Force alphamode, produce 1 line message message with pause
**
** History:
**	9/88 (bobm) -- cloned from GTerror
**	3/89 (Mike S) -- Force to bottom line.
**	8/89 (Mike S) -- Be sure it's a 1-line message.
*/

GTperror(errnum, numargs, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
	ER_MSGID	errnum;
	i4	numargs;
	PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
	char 		*TDtgoto();
	i4 		GTchout();
	extern i4  	G_lines;
	extern char 	*G_tc_cm;
	char		*p;
	char		msgbuf[200];

	if (G_mfunc != NULL)
		(*G_mfunc)(ALPHAMODE);
	TDtputs(TDtgoto(G_tc_cm, 0, G_lines - 1), 1, GTchout);
	/* 
	** Terminate after first line . This make it almost certain the
	** [HIT RETURN] will fit. (Jupbug 7654)
	*/
	IIUGfmt(msgbuf, sizeof(msgbuf), ERget(errnum), numargs, 
		a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	if ((p = STindex(msgbuf, ERx("\n"), 0)) != NULL)
		*p = EOS;
	IIUGmsg(msgbuf, TRUE, 0);
}

/*{
** Name:	GTsyserr()  -	System error with alpha mode
**
** Description:
**	System error routine used by GT when in graphics state.  Set alpha mode,
**	THEN fatal error.  May also be used when NOT in graphics state -
**	all that will happen is an unneeded set alpha mode.
**
** Inputs:
**	error message parameters without "fatal" flag - implicitly fatal
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**		calls routine which exits
**
** History:
**	10/85 (rlm)	written
**	8/87 (bobm)	changed to use mdg id's
*/

GTsyserr (id, num, a, b, c, d)
ER_MSGID id;
i4  num;
PTR a,b,c,d;
{
	G_interactive = TRUE;
	if (G_mfunc != NULL)
		(*G_mfunc)(ALPHAMODE);
	IIUGerr(id, UG_ERR_FATAL, num, a, b, c, d);
}
