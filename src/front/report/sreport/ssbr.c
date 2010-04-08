
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<st.h>
# include	<fe.h>
# include	<ug.h>
# include	 <stype.h>
# include	 <sglob.h>
# include	<er.h>
# include	"ersr.h"



/*
** Name:	ssbr.c		This file contains routines for accessing
**				the SBR structure for saving break info.
**
** Description:
**	This file defines:
**
**	s_sbr_add	Adds a break column to the SBR structure.
**	s_sbr_find	Finds a break column in the SBR structure.
**	s_sbr_set	Sets some info for a break column in the SBR structure.
**
** History:
**	5-may-87 (grant)	created.
**	26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file. Eliminate references to
**			r_syserr() and use IIUGerr() directly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*
**   S_SBR_ADD - add an SBR structure for another break attribute to the
**	current report.	 If this is the first (report) SBR structure,
**	put the pointer into the REN structure also to start the linked
**	list.
**	If the SBR structure already exists, return with add of it.
**
**	Parameters:
**		name - name of break attribute
**
**	Returns:
**		address of added SBR structure, or found structure.
**
**
**	History:
**		5-may_87 (grant)	stolen from s_rso_add.
**		3/20/90 (elein) performance
**			Change STcompare to STequal
*/



SBR	*
s_sbr_add(name)
char	*name;
{
	/* internal declarations */

	SBR	*sbr;				/* temp ptr to new sbr struct */
	SBR	*osbr = NULL;			/* ptr to old sbr struct */

	/* start of routine */

	if (name == NULL)
	{
		IIUGerr(E_SR000F_s_sbr_add_Null_name,UG_ERR_FATAL,0);
	}

	/* if no report currently in effect, abort */

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);	/* ABORT!!!!!! */
	}

	/* if SBR exists for this name, return it. If not, create one */

	for (sbr = Cact_ren->ren_sbrtop; sbr != NULL; sbr = sbr->sbr_below)
	{
		if (STequal(name, sbr->sbr_name) )
		{
			return(sbr);
		}
		osbr = sbr;		/* save old one */
	}

	/* gets here only if name not in list */

	sbr = (SBR *) MEreqmem(0, (u_i4) sizeof(SBR), TRUE, (STATUS *) NULL);
	if (STlength(name) > DB_MAXNAME)
	{
		*(name+DB_MAXNAME) = '\0';		/* truncate it */
	}

	sbr->sbr_name = (char *) STalloc(name);

	if (osbr == NULL)
	{
		Cact_ren->ren_sbrtop = sbr;
	}
	else
	{
		osbr->sbr_below = sbr;
	}

	En_n_breaks++;

	/* add break order */

	if (osbr == NULL)
	{
		sbr->sbr_order = 0;
	}
	else
	{
		sbr->sbr_order = osbr->sbr_order+1;
	}

	return(sbr);
}


/*
**   S_SBR_FIND - find an SBR structure for a given name.
**
**	Parameters:
**		name - name of break attribute
**
**	Returns:
**		address of matching SBR structure or NULL.
**
**
**	History:
**		5-may-87 (grant)	stolen from s_rso_find.
*/



SBR	*
s_sbr_find(name)
char	*name;
{
	/* internal declarations */

	SBR	*sbr;				/* temp ptr to new sbr struct */

	/* start of routine */

	if (name == NULL)
	{
		IIUGerr(E_SR0010_s_sbr_find_Null_name,UG_ERR_FATAL,0);
	}

	/* if no report currently in effect, abort */

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);	/* ABORT!!!!!! */
	}

	/* find corresponding name to this one */

	for (sbr = Cact_ren->ren_sbrtop; sbr != NULL; sbr = sbr->sbr_below)
	{
		if (STequal(name, sbr->sbr_name) )
		{
			return(sbr);
		}
	}

	return(NULL);
}


/*
**   S_SBR_SET - set the Cact_sbr pointer.  This is called in response
**	to a heading or footing command to change the break description.
**	Also set up the Command fields to point to this break.
**
**	Parameters:
**		name - name of new break.
**		type - either B_HEADER or B_FOOTER.
**
**	Returns:
**		address of new Cact_sbr, or NULL if not found.
**
**	History:
**		5-may-87 (grant)	stolen from s_rso_set.
*/



SBR	*
s_sbr_set(name, type)
char	*name;
char	*type;
{
	/* internal declarations */

	SBR	*sbr;
	bool	sameone = FALSE;		/* TRUE if this break is the
						** same as the last one given */

	/* start of routine */

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);		/* Abort!!!! */
	}

	sameone = (STequal(Ctype,NAM_ACTION) &&
		   STequal(Csection,type) &&
		   STequal(Cattid,name) );

	if ((sbr = s_sbr_find(name)) != NULL)
	{

		/* if this is the same as the last break, go right
		**   ahead with the old break	*/

		if (!sameone &&
		    ((STequal(B_HEADER,type) && sbr->sbr_sqheader > 0) ||
		     (STequal(B_FOOTER,type) && sbr->sbr_sqfooter > 0)))
		{	/* header/footer already specified */
			s_error(0x399, NONFATAL, name, NULL);
		}

		Cact_sbr = sbr;
		STcopy(NAM_ACTION,Ctype);
		STcopy(type,Csection);
		STcopy(name,Cattid);
		STcopy(ERx(" "),Ccommand);
		STcopy(ERx(" "),Ctext);

		/* Notice.  Don't set Csequence !!! */

		return(Cact_sbr);
	}

	/* did not find a match */

	return(NULL);
}
