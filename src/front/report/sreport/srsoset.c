/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <st.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<er.h>
 
/*
**   S_RSO_SET - set the Cact_rso pointer.  This is called in response
**	to a heading or footing command to change the break description.
**	Also set up the Command fields to point to this break.
**
**	Parameters:
**		name - name of new break.
**		type - either B_HEADER or B_FOOTER.
**
**	Returns:
**		address of new Cact_rso, or NULL if not found.
**
**	Called by:
**		s_brk_set, s_rep_set.
**
**	Side Effects:
**		Sets Ctype, Csection, Cattid.
**
**	Trace Flags:
**		13.0, 13.3.
**
**	History:
**		6/1/81 (ps)	written.
**		12/22/81 (ps)	changes for 2 tables.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.
*/



RSO	*
s_rso_set(name,type)
char	*name;
char	*type;
{
	/* internal declarations */

	RSO	*rso;
	bool	sameone = FALSE;		/* TRUE if this break is the
						** same as the last one given */

	/* start of routine */



	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);		/* Abort!!!! */
	}

	sameone = (STequal(Ctype,NAM_ACTION)  && STequal(Csection,type) 
			&& STequal(Cattid,name) ) ? TRUE : FALSE;

	if ((rso = s_rso_find(name)) != NULL)
	{


		/* if this is the same as the last break, go right
		**   ahead with the old break   */

		if (!sameone &&
		    ((STequal(B_HEADER,type) &&(rso->rso_sqheader>0)) ||
		    (STequal(B_FOOTER,type) &&(rso->rso_sqfooter>0))))
		{	/* header/footer already specified */
			s_error(0x399, NONFATAL, name, NULL);
		}

		Cact_rso = rso;
		STcopy(NAM_ACTION,Ctype);
		STcopy(type,Csection);
		STcopy(name,Cattid);
		STcopy(ERx(" "),Ccommand);
		STcopy(ERx(" "),Ctext);

		/* Notice.  Don't set Csequence !!! */

		return(Cact_rso);
	}

	/* did not find a match */


	return(NULL);
}
