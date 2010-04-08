/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rmtchatt.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_MTCH_ATT - find the ordinal of an attribute from the data relation,
**	given the attribute name.  Search the array of ATT structures until
**	the name is found.
**
**	If the special name "w_column" or "within_column" or "wi_column"
**	is given, return A_GROUP.
**
**	Parameters:
**		name - ptr to string containing the name to search for.
**
**	Returns:
**		ordinal of the attribute with the given name.  If "name" is
**		not one of the attributes in the data relation, a -1 is
**		returned.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		2.0, 2.3.
**
**	History:
**		3/15/81 (ps) - written.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**              4/5/90 (elein) performance
**                     Since fetching attribute call needs no range
**                     check use &Ptr_att_top[] directly instead of
**                     calling r_gt_att. We don't expect brk_attribute
**                     to be A_REPORT, A_GROUP, or A_PAGE
**		26-oct-1992 (rdrane)
**			If the db_data ptr is NULL, then we're dealing with
**			an unsupported datatype (e.g., BLOB).  So, any
**			attempt to reference such a column will result in a
**			return of A_ERROR.  See comments in rattdflt.c
**			for why we even allocated the ATT in the first place.
*/



ATTRIB
r_mtch_att(name)
STRING	*name;
{
	/* external declarations */
	/* internal declarations */

	register ATT	*attribute;		/* ptr to attribute in array
						** of ATT structures.
						*/
	register ATTRIB	natt = 1;		/* attribute counter */

	/* start of routine */


	for( natt=1; (natt <= En_n_attribs); natt++)
	{
		attribute = &(Ptr_att_top[natt-1]);
		/* next attribute in array */

		if (STequal(name,attribute->att_name) )
		{	/* match found */
			if  (attribute->att_value.db_data == (PTR)NULL)
			{
				return(A_ERROR);
			}
			return(natt);
		}

	}

	/* now compare to the "w_column" */

	if (r_gt_wcode(name) == A_GROUP)
	{
		return(A_GROUP);
	}

	/* no match found if it gets this far */


	return (A_ERROR);
}
