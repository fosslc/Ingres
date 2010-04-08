/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rtcmdset.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_TCMD_SET - load the ACT lines from the RCOMMANDS system relation 
**	into the TCMD structures, and link them to the BRK structures.
**	This is the overall control routine for setting up the TCMD
**	structures.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_rep_ld.
**
**	Side Effects:
**		Sets the Cact_type code.
**		Sets the pointer Cact_rtext.
**		Sets the pointer Cact_command.
**		Sets the pointer Cact_attribute.
**		May set the Cact_stk ptr.
**
**	Trace Flags:
**		100, 140, 121, 120 (for reading).
**		Note:  Flag 121 will print out the TCMD structures
**			as they are created.  140 prints them out
**			at the end of the routine.
**
**	History:
**		3/24/81 (ps) - written.
**		6/10/81 (ps) - changed to read all tuples and process
**				changes in the value of Cact_attribute
**				or Cact_type.  This was two routines.
**		12/22/81 (ps)	modified for two table version.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		12/1/84 (rlm)	ACT reduction - call r_gch_act.
**		1/8/84 (rlm)	Oact can be static arrays.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_tcmd_set()
{
	ACT *cur_act,*r_gch_act();

	/* start of routine */


	/* loop through the ACT lines */


	for (cur_act=r_gch_act((i2)1,(i4)1);
				cur_act != NULL; cur_act=r_gch_act((i2)0,(i4)1))
	{
		/* move the fields */

		Cact_type = cur_act->act_section;
		Cact_attribute = cur_act->act_attid;
		Cact_command = cur_act->act_command;
		Cact_rtext = cur_act->act_text;



		/* set up a new break, or TCMD structure */

		r_nxt_set();

	}

	r_end_type();		/* stop the last break */


	return;
}
