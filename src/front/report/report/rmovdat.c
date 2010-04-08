/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rmovdat.c	30.1	11/14/84"; */

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
**   R_MOV_DAT - move the data read from the data table into the
**	previous data position.  This simply transfers the data
**	values in the ATT structures immediately before another
**	data row is read from the data table.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_rep_do.
**
**	Side Effects:
**		Moves data in the ATT structures.
**
**	Error Messages:
**		Syserr:Bad attribute type.
**
**	Trace Flags:
**		4.0
**
**	History:
**		6/30/81 (ps)	written.
**		2/1/84 (gac)	added date type.
**		7/26/85 (gac)	break on formatted dates.
**		4/4/90 (elein) performance
**			Changed to do pointer swapping rathger
**			MEcopies--we've always got the same kind
**			of thing
**              4/5/90 (elein) performance
**                     Since fetching attribute call needs no range
**                     check use &Ptr_att_top[] directly instead of
**                     calling r_gt_att. We don't expect brk_attribute
**                     to be A_REPORT, A_GROUP, or A_PAGE
**		7/30/90 (elein)
**			left side of assignments cast to PTR caused
**			compilation warnings.  Casting should not
**			be a problem since all pointers are declared
**			to be the same type.
**		26-oct-1992 (rdrane)
**			If the db_data ptr in NULL, then we're dealing with
**			an unsupported datatype (e.g., BLOB).  So, don't bother
**			moving what isn't there.
**		14-nov-2008 (gupsh01)
**			Added support for -noutf8align flag. 
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_mov_dat()
{
	DB_DATA_VALUE	temp_dbv;
	ATT		temp_att;
	/* internal declarations */

	register ATT	*att;		/* ptr to attribute structure */
	register ATTRIB	natt;		/* counter of attributes */
		FMT	*fmt;

	/* start of routine */


	/* transfer data from current to previous locations */

	for (natt = 1; natt <= En_n_attribs; natt++)
	{
		att = &(Ptr_att_top[natt-1]);
		if  (att->att_value.db_data == (PTR)NULL)
		{
			continue;
		}
		/* Save the current data pointer
		** swap the previous data pointer to current
		** move current to previous.  Now the
		** current one (formerly the previous) will
		** be overwritten by the new values.  Are you
		** following? We are swapping pointers rather than
		** copying the data from the current to the 
		** previous db_data elements.
		*/
		temp_dbv.db_data = (PTR) att->att_value.db_data;
		att->att_value.db_data = (PTR) att->att_prev_val.db_data;
		att->att_prev_val.db_data  = (PTR) temp_dbv.db_data;
		temp_dbv.db_data = (PTR) att->att_value.db_data;

		/* OBSOLETE 
		** MEcopy((PTR)att->att_value.db_data,
		**       (u_i2)att->att_value.db_length,
		**       (PTR)att->att_prev_val.db_data);
		*/
		if (att->att_brk_ordinal > 0 && att->att_dformatted)
		{
			/* OBSOLETE 
			** MEcopy((PTR)att->att_cdisp, (u_i2)att->att_dispwidth,
			**        (PTR)att->att_pdisp);
			*/
			temp_att.att_cdisp = att->att_cdisp;
			att->att_cdisp = att->att_pdisp;
			att->att_pdisp = temp_att.att_cdisp;
		}
	}
	
	if (En_need_utf8_adj)
          En_utf8_adj_cnt = 0;

	return;
}
