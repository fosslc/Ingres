/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <st.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<er.h>
# include	<errw.h>

/*
**	R_RUNERR -- print special error message for runtime error.
**		This just calls r_error with message parameters determined
**		in this routine. These parameters are the section,
**		attribute, command, and text where the error occurred
**		gotten from the RCOMMAND table.
**		This is called for the following errors:
**			510 thru 518.
**
**	Parameters:
**		err_num - error number, modulo ERRBASE.
**		type - seriousness of error.
**			FATAL - fatal error.
**			NONFATAL - non fatal (continue processing).
**	Returns:
**		none.  Exits on FATAL errors.
**
**	History:
**		8/16/84 (gac)	written.
**		12/1/84 (rlm)	ACT reduction - modified to call r_gsv_act.
**		3/15/90 (elein) performance
**		Do lookup of action, don't use
**		actions cached to a file.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_runerr(err_num,type)
ERRNUM		err_num;
i2		type;
{
	ACT *abuf;		/* storage buffer for action items */
	char	*type_name;

	if (Sequence_num < 1 || Sequence_num > En_acount)
		r_syserr(E_RW0049_not_legal_sequence_no);

	abuf = r_gch_act (1, Sequence_num);
	if (STequal(abuf->act_section, B_HEADER) )
	{
		type_name = NAM_HEADER;
	}
	else if (STequal(abuf->act_section, B_FOOTER) )
	{
		type_name = NAM_FOOTER;
	}
	else
	{
		type_name = NULL;
	}

	r_error(err_num, type, type_name, abuf->act_attid, abuf->act_command,
		abuf->act_text, NULL);
	return;

}
