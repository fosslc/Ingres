/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rgetatt.c	30.1	11/14/84"; */

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
**   R_GET_ATT - given an ordinal of an attribute from the data relation,
**	return the ATT structre.
**
**	Parameters:
**		ordinal	ordinal of attribute.
**			If ordinal is A_GROUP, this will return the
**			ATT for the current group column.
**
**	Returns:
**		ATT structure of the attribute with the given ordinal.
**		If the ordinal is bad or the ATT represents an unsupported
**		datatype, return NULL.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		2.0, 2.3.
**
**	History:
**		1/7/82 (ps)	written.
**		26-oct-1992 (rdrane)
**			If the db_data ptr is NULL, then we're dealing with
**			an unsupported datatype (e.g., BLOB).  So, any
**			attempt to reference such a column will result in a
**			return of NULL.  See comments in rattdflt.c
**			for why we even allocated the ATT in the first place.
*/



ATT  *
r_gt_att(ordinal)
ATTRIB	ordinal;
{

	/* internal declarations */

	register ATT	*att;			/* ptr to return struct */

	/* start of routine */



	if  ((ordinal == A_GROUP) && (St_cwithin != NULL))
	{	/* In a .WITHIN block, and group ordinal specified */
		ordinal = St_cwithin->tcmd_val.t_v_long;
	}

	if  ((ordinal <= 0) || (ordinal > En_n_attribs))
	{
		att = (ATT *)NULL;
	}
	else
	{
		att = &(Ptr_att_top[ordinal-1]);
		if  (att->att_value.db_data == (PTR)NULL)
		{
			return((ATT *)NULL);
		}
	}

	return (att);
}
