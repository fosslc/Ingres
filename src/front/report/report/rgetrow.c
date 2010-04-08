
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>


/**
** Name:	rgetrow.c	Contains routine which gets the next row of
**				data.
**
** Description:
**	This file defines:
**
**	r_getrow	Gets the next row of data.
**
** History:
**	21-apr-87 (grant)	created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	r_getrow	Gets the next row of data.
**
** Description:
**	This routine gets the next row of data and places the data in the
**	current value for each attribute.
**
** Input params:
**	None.
**
** Output params:
**	None.
**
** Returns:
**	OK	if another row was returned successfully.
**	FAIL	if no more rows or an error occurred.
**
** History:
**	21-apr-87 (grant)	implemented.
**      4/5/90 (elein) performance
**                     Since fetching attribute call needs no range
**                     check use &Ptr_att_top[] directly instead of
**                     calling r_gt_att. We don't expect brk_attribute
**                     to be A_REPORT, A_GROUP, or A_PAGE
*/

STATUS
r_getrow()
{
    i4	i;
    ATT	*att;

    if (IInextget() != 0)
    {
	/* get the value of each attribute */

	for (i = 1; i <= En_n_attribs; i++)
	{
	    att = &(Ptr_att_top[i-1]);

	    /* get 1 domain value */
	    IIretdom(1, DB_DBV_TYPE, 0, &(att->att_value));

	    if (IIerrtest() != 0)
	    {
		St_ing_error = TRUE;
		return FAIL;
	    }
	}

	return OK;
    }

    /* no more tuples to retrieve */
    return FAIL;
}
