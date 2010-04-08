/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h>
# include	<rtype.h> 
# include	<rglob.h> 
# include	<st.h>

/**
** Name:	rchkdat.c -	Report Writer Check Report Data Table Existence.
**
** Description:
**	Contains the routine that checks whether the query for a report can be
**	set up.
**
**	r_chk_dat()	check for report data.
**
** History:	
**		Revision 50.0  86/04/09  17:48:37  wong
**		Changed for PC:  Report writer no longer uses temporary view
**		for retrieve.
**
**		3/8/81 (ps)
**		Initial revision.
**/


/*{
** Name:	r_chk_dat() -	Check For Report Data (either table or query.)
**
** Description:
**   R_CHK_DAT - check to see if the relation to be reported exists.
**	If no relation is specified by the .DATA command,
**	then check to see that a query has been specified.  If that
**	fails, then return.
**	Also fill in some Env fields.
**
** Parameters:
**	none.
**
** Returns:
**	TRUE	if query setup or data relation found.
**	FALSE	if any error found.
**
** Side Effects:
**	none.
**
** Called by:
**	r_rep_ld().
**
** History:
**	3/8/81 (ps) - written.
**	4/11/86 (jhw) - Changes for PC:  No longer sets up query as temporary
**			view through 'r_qur_do()'.
**	8/11/89 (elein) garnet
**		Added $variable evaluation for En_relation
**	20-oct-1992 (rdrane)
**		"Demoted" the file to a ".c" since there is no ESQL processing
**		occuring here anymore.  Remove declarations of En_relation and
**		En_report since they're already declared in hdr files
**		included by this module, and ESQL doesn't need to know about
**		them here.  Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
**		Replace En_relation with reference to global FE_RSLV_NAME
**		En_ferslv.name.
**	09-feb-1994 (geri)
**		Changed the 2nd arg in call to r_rng_add to NULL to 
**		prevent erroneous capitalization of a table name
**		with delimited ids in fips databases (bug 58270).
*/

bool
r_chk_dat()
{
	/* start of routine */
	char *rel_par_name;
	char *relname;



	/* if the En_ferslv.name field is blank, check for a query */

	if (STtrmwhite(En_ferslv.name) < 1)
	{	/* No relation, check for query */
		if  (!r_qur_set())
		{	/* bad query */
			r_error(0x19, NONFATAL, NULL);
			return FALSE;
		}
	}
	else
	{
		/* Evaluate relation name if it is a variable */
		r_g_set(En_ferslv.name);
		if  (r_g_skip() == TK_DOLLAR)
		{
			CMnext(Tokchar);
			rel_par_name = r_g_name();
			if ((relname = r_par_get(rel_par_name)) == NULL)
			{
			 	r_error(0x3F0, NONFATAL, rel_par_name, NULL);
				return( FALSE);
			}
			En_ferslv.name = STalloc(relname);
		}
		if  (!FE_fullresolve(&En_ferslv))
		{	/* data relation does not exist */
			if  (En_rtype == RT_DEFAULT)
			{	/* default report */
				r_error(0x04, NONFATAL, En_report, NULL);
			}
			else
			{	/* report data table */
				r_error(0x05,NONFATAL,
					En_ferslv.name,En_report,NULL);
			}
			return FALSE;
		}
		else
		{
			/* Add to range list */
			if  (!r_rng_add(En_ferslv.name, NULL))
			{
				r_error(0x15,NONFATAL,
					En_ferslv.name,En_ferslv.name,NULL);
			}
		}
	}

	return TRUE;
}
