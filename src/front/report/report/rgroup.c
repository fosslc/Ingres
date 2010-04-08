/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rgroup.c	30.1	11/14/84"; */

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
**   R_GROUP - routines to loop through the columns in a .WITHIN
**	group while setting up the commands.  These routines set
**	up and process the columns one at a time.
**	If they are started with a standard column ordinal, then
**	the routines will simply return that ordinal.  This is
**	useful when either is needed.
*/



	/* external to both routines */

# define	TYPELOOP 	1		/* loop type command */
# define	TYPESIMPLE	2		/* simple column ordinal */
# define	TYPEERR 	0		/* error type */


	static	i4	grp_type = {0};		/* type of looping */
	static	ATTRIB	grp_ordinal = {0};	/* col ordinal if TYPE2 */
	static	TCMD	*grp_tcmd = {0};		/* ptr to current TCMD in
						** group */


/*
**   R_GRP_SET - set up the group commands and make sure that it is
**	legal at this point.
**
**	Parameters:
**		ordinal	group commands if ordinal = A_GROUP.
**			simple ordinal if ordinal != A_GROUP.
**
**	Returns:
**		TRUE	if started ok, and legal.
**		FALSE	if any problem.
**
**	Side Effects:
**		changes value of grp_tcmd, grp_type, grp_ordinal.
**
**	Trace Flags:
**		150, 153.
**
**	History:
**		1/8/82 (ps)	written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

bool
r_grp_set(ordinal)
ATTRIB	ordinal;
{
	/* start of routine */



	grp_type = TYPEERR;
	grp_ordinal = 0;
	grp_tcmd = NULL;

	if (ordinal!=A_GROUP)
	{
		grp_type = TYPESIMPLE;
		if((ordinal>0) && (ordinal<=En_n_attribs))
		{
			grp_ordinal = ordinal;
			goto getout;
		}
		return(FALSE);		/* bad ordinal */
	}

	grp_type = TYPELOOP;
	if ((St_ccoms==NULL) || (St_cwithin==NULL))
	{
		return(FALSE);
	}

	grp_tcmd = St_cwithin;
	grp_ordinal = grp_tcmd->tcmd_val.t_v_long;

	getout:

	return(TRUE);
}



/*
**   R_GRP_NXT - get the next column ordinal in the group of .WI columns.
**
**	Parameters:
**		none.
**
**	Returns:
**		ordinal of next column, or A_ERROR at end.
**
**	Side Effects:
**		changes value of grp_tcmd, grp_type.
**
**	Trace Flags:
**		150, 153.
**
**	History:
**		1/8/82 (ps)	written.
*/

ATTRIB
r_grp_nxt()
{
	/* internal declarations */

	ATTRIB		ord;		/* return value */

	/* start of routine */



	switch (grp_type)
	{
		case(TYPELOOP):
			/* loop type command */
			if ((St_ccoms==NULL)||(St_cwithin==NULL)||(grp_tcmd==NULL))
			{
				return(A_ERROR);
			}
			if (grp_tcmd->tcmd_code != P_WITHIN)
			{
				return(A_ERROR);
			}

			ord = grp_tcmd->tcmd_val.t_v_long;

			if((ord <= 0) || (ord > En_n_attribs) || (ord == A_GROUP))
			{
				return(A_ERROR);
			}

			grp_tcmd = grp_tcmd->tcmd_below;
			break;

		case(TYPESIMPLE):
			/* simple ordinal */
			grp_type = TYPEERR;
			ord = grp_ordinal;
			break;

		default:
			return(A_ERROR);
	}


	return(ord);
}
