/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)saveem.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 

/*
**   SAVE_EM/GET_EM - routines to save and restore the fields
**	in the Command variables.  This allows the Action
**	Commands to be interspersed amoungst the .SORT, .DATA
**	and .OUTPUT commands.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		s_srt_set, s_dat_set, s_out_set.
**
**	Side Effects:
**		SAVE_EM - none.
**		GET_EM - replaces values of all externals.
**
**	Error Messages:
**		none.
**
**	History:
**		12/22/81 (ps)	written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


	/* external for saving the variables */

	static	char	type[MAXRNAME+1]={0};	/* type of command */
	static	i4	sequence=0;		/* sequence information */
	static	char	section[MAXRNAME+1]={0};	/* section of command */
	static	char	attid[FE_MAXNAME+1]={0};	/* attribute name */
	static	char	text[MAXRTEXT+1]={0};	/* command text */

/*
**   SAVE_EM  - store into temporaries.
*/

VOID
save_em()
{
	/* start of routine */

	STcopy(Ctype,type);
	sequence = Csequence;
	STcopy(Csection,section);
	STcopy(Cattid,attid);
	STcopy(Ctext,text);

	return;
}

/*
**   GET_EM - get back the temporaries.
*/

VOID
get_em()
{
	/* start of routine */

	STcopy(type,Ctype);
	Csequence = sequence;
	STcopy(section,Csection);
	STcopy(attid,Cattid);
	STcopy(text,Ctext);

	return;
}
