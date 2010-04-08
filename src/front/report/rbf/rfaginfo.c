
/*	static	char	Sccsid[] = "@(#)rfaginfo.c	60.1	2/3/87";	*/
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/**
** Name:	rfaginfo.c - set up arrays needed for aggregate processing
**
** Description:
**	This file defines:
**	rFset_aggarrs	-	Set up aggregate arrays for relation.
**	rFget_agnms	-	Set up descriptions of aggregates.
**	rFnum_types	-	Determine number of data types in relation.
**	rFinit_aginfo	-	Get aggregate information for one attribute.
**
** History:
**	2/3/87 (rld) - written.
**	16-dec-88 (sylviap)
**		Changed memory allocation to use FEreqmem.
**      9/21/89 (elein) PC integration
**              Changed printf calls to SIprintf
**              9/22/89 (elein) UG changes
**                      ingresug change #90045
**			changed <rbftype.h> and <rbfglob.h> to
**			"rbftype.h" and "rbfglob.h"
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

/* # define's */

/* GLOBALDEF's */

GLOBALDEF  AGG_TYPE	*Agg_top ZERO_FILL;	/* ptr to array of AGGTYPE */
GLOBALDEF  i2		Aggtype_tag ZERO_FILL;	/* Tag for AGGTYPE array */

/* extern's */

GLOBALREF  i2		Agg_tag;		/* Tag for AGNAMES array */

/* static's */

static i4	num_types;	/* number of datatypes in relation */

static VOID	rFget_agnms();
static i4	rFnum_types();
static i4	rFinit_aginfo();

/*{
** Name:	RFSET_AGARRS - Set up aggregate arrays for relation.
**
** Description:
**	Set up array containing aggregate information for each datatype in the
**	relation, which describes all the possible aggregates for the report.
**	Allocate storage for description of each aggregate function for each
**	attribute in relation.	Set ptrs to appropriate ADF_AGNAMES structures
**	and blank out variables for storage of 'x' for aggregate selection.
**
** Pseudocode:
**	Allocate and set values in AGG_TYPE and ADF_AGFNAMES arrays,
**	    for all possible aggregates in report (rFget_agnms).
**	FOR each attribute in relation.
**		Get COPT ptr.
**		Allocate and initialize AGG_INFO array and set values
**		    in COPT structure (rFinit_aginfo).
**	Free AGG_TYPE array.
**
** Inputs:
**	none.
**
** Globals used: (module globals)
**	i4		En_n_attribs	Number of attributes in relation.
**	i2		Aggtype_tag	Tag for AGGTYPE array.
**
** Outputs:
**	none.
**	Returns:
**		none.
**	Exceptions:
**
** Side Effects:
**	Allocates and fills arrays of ADF_AGNAMES and AGGINFO for relation
**	   and set ptrs and counts in COPT array.
**
** History:
**	2/3/87 (rld) - written.
**	5/12/87 (rld) - fixed conditional checking if datatype already added
**			to agg_type array.
**	12-nov-1992 (rdrane)
**		Ensure that we ignore unsupported datatypes.
*/
VOID
rFset_aggarrs()
{
	i4		i;
	COPT		*copt;		/* ptr to COPT structure */
	ATT		*att;		/* ptr to ATT structure */

	/* allocate and fill array of AGG_TYPES and arrays of ADF_AGNAMES */
	rFget_agnms();

	/* for each attribute in relation */
	for (i=1; i<=En_n_attribs; i++)
	{
		/* get ptr to ATT structure for attribute */
		if  ((att = r_gt_att(i)) == (ATT *)NULL)
		{
			continue;
		}
		/* get ptr to COPT structure for attribute */
		copt = rFgt_copt(i);
		/* allocate AGGINFO array and set values */
		rFinit_aginfo(copt, &att->att_value);
	}

	/*	Free the AGGTYPE array */

	if (Aggtype_tag > 0)	/* Free the associated memory */
	{
		FEfree(Aggtype_tag);
		Aggtype_tag = -1;
	}
}

/*{
**
** Name:	RFGETAGNMS - Set up descriptions of aggregates.
**
** Description:
**	Set up descriptions of aggregates allowed for report.
**	This allocates storage and fills the data into arrays of
**	ADF_AGNAMES structures, one array per datatype in the relation.
**
** Pseudocode:
**	Determine number of datatypes in relation (rFnum_types).
**	Allocate array of AGG_TYPE structures, one per datatype in rel.
**	FOR each attribute of relation
**		IF element of AGG_TYPE for atts datatype not yet filled
**			Determine number of agg functions (afe_numaggs).
**			Allocate array of ADF_AGNAMES.
**			Fill array of ADF_AGNAMES just allocated (afe_agnames).
**			Set value of datatype in AGG_TYPE element
**			Set ptr in AGG_TYPE element to ADF_AGNAMES array.
**			Set count in AGG_TYPE element to number of ADF_AGNAMES
**				arrays for datatype.
**		ENDIF
**
** Inputs:
**	none.
**
** Globals used:
**	En_n_attribs	Number of attributes in relation.
**
** Outputs:
**	none.
**	Returns:
**		none.
**	Exceptions:
**		r_syserr	Can't allocate memory.
**		r_syserr	Can't get number of aggregate functions.
**		r_syserr	Can't get aggregate names.
**
** Side Effects:
**	Allocate and fills AGG_TYPE array and one ADF_AGNAMES array per
**	    datatype in the relation.
**	Sets module global num_types.
**
** History:
**	2/3/87 (rld) - written.
**	12-nov-1992 (rdrane)
**		Ensure that we ignore unsupported datatypes.
*/
static
VOID
rFget_agnms()
{
	i4		i;
	i4		j;
	i4		num_ags;
	i4		num_ags_ret;
	i4		num_entries;
	i4		retstatus;		/* function return status */
	ATT		*att;
	AGG_TYPE	*agg_type;
	ADF_AGNAMES	*adf_agnames;

	/* Determine number of datatypes in relation */
	num_types = rFnum_types();

	/* Allocate AGG_TYPE array */
	Aggtype_tag = FEgettag();

        Agg_top = (AGG_TYPE *)FEreqmem((u_i4)Aggtype_tag,
		(u_i4)(num_types*(sizeof(AGG_TYPE))), TRUE, &retstatus);
	if (retstatus != OK)
	{
		IIUGerr(E_RF0004_rFget_agnms__Can_t_al, UG_ERR_FATAL, 0);
	}

	Agg_tag = FEgettag();
	num_entries = 0;
	for(i=1; i<=En_n_attribs; i++)
	{
		if  ((att = r_gt_att(i)) == (ATT *)NULL)
		{
			continue;
		}
		/* Check if datatype already added to agg_type array */
		for(agg_type = Agg_top, j=1; j <= num_entries &&
		    att->att_value.db_datatype != agg_type->dbdv.db_datatype;
		    agg_type++, j++)
			;

		/* If datatype has not been added, then add it here */
		if (j > num_entries)
		{
			/* Determine number of aggregates for data type */
			retstatus = afe_numaggs(FEadfcb(), &att->att_value,
			    &num_ags);
			if (retstatus != OK)
			{
				IIUGerr(E_RF0005_rFget_agnms__Can_t_ge, 
				  UG_ERR_FATAL, 0);
			}

			/* Allocate ADF_AGNAMES arrays */
        		adf_agnames = (ADF_AGNAMES *)FEreqmem
				((u_i4)Agg_tag, 
				(u_i4)(num_ags*(sizeof(ADF_AGNAMES))), 
				TRUE, &retstatus);
			if (retstatus != OK)
			{
				IIUGerr(E_RF0004_rFget_agnms__Can_t_al, 
				  UG_ERR_FATAL, 0);
			}

			/* Fill ADF_AGNAMES array */
			retstatus = afe_agnames(FEadfcb(), &att->att_value,
			    num_ags, adf_agnames, &num_ags_ret);
			if (retstatus != OK)
			{
				IIUGerr(E_RF0006_rFget_agnms__Can_t_ge, 
				  UG_ERR_FATAL, 0);
			}

			/* Set datatype, ptr and count in AGG_TYPE */
			agg_type->dbdv.db_datatype = att->att_value.db_datatype;
			agg_type->agnames = adf_agnames;
			agg_type->num_ags = num_ags;

			/* increment count for filled in entries in AGG_TYPE */
			num_entries++;
		}
	}
	return;
}

/*{
** Name:	RFNUMTYPES - Determine number of data types in relation.
**
** Description:
**	Determines the number of different datatypes for the attributes in
**	the current relation.
**
** Pseudocode:
**	Set count to 0
**	FOR each attribute in relation
**		IF datatype of attribute is different from all previous atts
**			THEN increment count
**		ENDIF
**	RETURN count
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**	Returns:
**		count	number of datatypes in relation.
**	Exceptions:
**		none.
**
** Side Effects:
**		none.
**
** History:
**	2/3/87 (rld) - written.
**	12-nov-1992 (rdrane)
**		Ensure that we ignore unsupported datatypes.  Break the loop
**		on check attribute strictly less that current attribute.
**		This avoids a comparison with itself, which is what previously
**		always ended the FOR loop.
*/
static
i4
rFnum_types()
{
	i4	count;
	i4	i;
	i4	j;
	ATT	*att;
	ATT	*chk_att;

	for(i=1, count=0; i<=En_n_attribs; i++)
	{
		if  ((att = r_gt_att(i)) == (ATT *)NULL)
		{
			continue;
		}
		for(j = 1; j < i; j++)
		{
			if  ((chk_att = r_gt_att(j)) == (ATT *)NULL)
			{
				continue;
			}
			if  (att->att_value.db_datatype ==
				chk_att->att_value.db_datatype)
			{
				break;
			}
		}
		if (i == j)
		{
			count++;
		}
	}
	return (count);
}

/*{
** Name:	RFINIT_AGINFO - Get aggregate information for one attribute.
**
** Description:
**	Allocate storage for description of each aggregate function for one
**	attribute in relation.	Set ptrs to appropriate ADF_AGNAMES structures
**	and blank out variables for storage of 'x' for aggregate selection.
**
** Pseudocode:
**	Locate aggregate information for passed datatype. ie. scan passed
**		AGG_TYPE array for match with datatype.
**	Allocate AGGINFO array for number of aggregate functions for datatype.
**	Set ptr in passed COPT to AGGINFO array just allocated.
**	Set count in passed COPT element to number of ADF_AGNAMES arrays
**		for datatype.
**	FOR each element in AGGINFO array
**		Set ptr to appropriate ADF_AGNAMES structure.
**		Blank out variables for storage of aggregate selections.
**
** Inputs:
**	COPT		*copt		ptr to column option structure.
**	DB_DATA_VALUE	*datatype	ptr to datatype.
**
** Globals used: (module globals)
**	i4		num_types	number of types in relation.
**	AGG_TYPE	*Agg_top	ptr to array of AGGTYPE.
**	i2		Aggtype_tag	Tag for AGGTYPE array.
**
** Outputs:
**	none.
**	Returns:
**		none.
**	Exceptions:
**		r_syserr	Datatype not in AGG_TYPE array.
**		r_syserr	Can't allocate memory.
**
** Side Effects:
**	Allocates and fills array of AGGINFO for attribute.
**
** History:
**	2/3/87 (rld) - written.
**	5/12/87 (rld) - sets copt->num_ags instead of copt->agginfo->num_ags
**	11/28/89 (martym) - Deleted references to passe fields of structure 
**			AGG_INFO.
*/
static i4
rFinit_aginfo(copt, dbdv)
COPT		*copt;
DB_DATA_VALUE	*dbdv;
{
	i4		i;
	AGG_TYPE	*agg_type;	/* ptr to array of AGG_TYPE */
	AGG_INFO	*agg_info;	/* ptr to array of AGG_INFO */
	ADF_AGNAMES	*agnames;	/* ptr to current names for one agg */
	STATUS		retstatus;

	/* Scan AGG_TYPE array for element matching datatype of passed type */
	for (i = 0, agg_type = Agg_top; agg_type->dbdv.db_datatype
	    != dbdv->db_datatype && i < num_types; agg_type++)
		;
	/*
	** agg_type now points to the correct AGGTYPE structure for the
	** attribute's datatype
	*/
	if (i == num_types)
	{
		IIUGerr(E_RF000B_rFinit_aginfo__dataty, UG_ERR_FATAL, 0);
	}

	/* Allocate AGG_INFO array */
        agg_info = (AGG_INFO *)FEreqmem((u_i4)Agg_tag,
		(u_i4)((agg_type->num_ags)*(sizeof(AGG_INFO))), 
		TRUE, &retstatus);
	if (retstatus != OK)
	{
		IIUGerr(E_RF0004_rFget_agnms__Can_t_al, UG_ERR_FATAL, 0);
	}

	/* set ptr to AGG_INFO array, set number of aggs for att */
	copt->agginfo = agg_info;
	copt->num_ags = agg_type->num_ags;

	/* for each element in AGGINFO array */
	for (i = 0, agnames = agg_type->agnames; i < agg_type->num_ags;
	  i++, agg_info++, agnames++)
	{
		/* set ptr to appropriate ADF_AGNAMES structure */
		agg_info->agnames = agnames;
	}
}
#	ifdef	xRTR2
rfpr_agnames(adf_agnames)
ADF_AGNAMES	*adf_agnames;
{
	SIprintf(ERx("\n\radf_agname address is %d"), adf_agnames);
	SIprintf(ERx("\n\radf_agnames->ag_fid is %d"), adf_agnames->ag_fid);
	SIprintf(ERx("\n\radf_agnames->ag_opid is %d"), adf_agnames->ag_opid);
	SIprintf(ERx("\n\radf_agnames->ag_funcname is %s"), adf_agnames->ag_funcname);
	SIprintf(ERx("\n\radf_agnames->ag_framelabel is %s"),adf_agnames->ag_framelabel);
	SIprintf(ERx("\n\radf_agnames->ag_reportlabel is %s"),adf_agnames->ag_reportlabel);
	SIprintf(ERx("\n\radf_agnames->ag_prime is %d"),adf_agnames->ag_prime);
}
#	endif
