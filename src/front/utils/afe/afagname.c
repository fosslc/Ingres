/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<bt.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include       <er.h>

/* # include's */

/**
** Name:	afagname -	Routines to deal with aggregate names.
**
** Description:
**	This file contains routines that give information about aggregates
**	applicability to a particular type, and information about the
**	aggregates name.  The name information includes display names
**	for RBF.
**
**	afe_numaggs	Return number of aggs for a type.
**	afe_agnames	Return aggregate information for a particular type.
**
** History:
**	27-mar-1987 (Joe)
**		Written
**	25-jun-1987 (danielt)
**		changed equality op to assignment op (~line 263)
**	11-sep-1987 (rdesmond)
**		changed to set query language in ADF_CB to quel for local use
**			in afe_numaggs so all aggs are known to adf.
**	10-oct-92 (leighb) DeskTop Porting Change:
**		added STATUS return type to afe_numaggs()
**	25-feb-93 (mohan) added tag to AGNAMES struct.
**	12-mar-93 (fraser)
**		Changed structure tag name so that it doesn't begin
**		with underscore.
**	31-aug-1993 (rdrane)
**		Ensure that all ME calls have address arguments cast to PTR
**		so the CL prototyping doesn't complain.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**      25-sep-96 (mcgem01)
**              Global data moved to afedata.c
**      18-dec-1996 (canor01)
**          Move definition of AGNAMES structure to afe.h. Add 
**	    AGNAMES_COUNT for size of Afe_agtab array.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	10-may-1999 (shero03)
**	    Support Multivariate functions
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-oct-2001 (abbjo03)
**	    In afe_agnames(), test for DB_ALL_TYPE.
**	29-jul-2002 (abbjo03)
**	    In the initialization phase of afe_numaggs, set all bits of
**	    ag_dtmask if the datatype is DB_ALL_TYPE.
*/

/* extern's */

/*
** This contains the generic information for each of the known aggregates.
** The ag_fid for the record is not known until the applicable type
** is known.
*/
GLOBALREF AGNAMES	Afe_agtab[AGNAMES_COUNT];

/*{
**Name:   afe_agnames   -   fill array with aggregate names and attributes
**
**Description:
**      Given a datatype (DB_DT_ID), fills array of structures pointed to 
**      by agnames with the function name, frame and report labels, and
**      the value of the attribute "prime" for each aggregate function
**      which is allowable for that datatype.  The array is filled sorted
**      by ADF_AGNAMES.ag_framelabel.  
**	
**	If a null pointer is passed instead of a pointer to a valid datatype, 
**	then all the aggregate functions for the installation are returned.  
**	In that case the value of ag_fid will be set to 0, since there can be
**	many values of ag_fid for one aggregate function, as a function of
**	the datatype of the operand.
**
** Inputs:
**        cb              A pointer to the current ADF_CB control block.
**
**        dbv            This is a DB_DATA_VALUE used to hold the datatype.
**			If this pointer is null, then all of the allowable 
**			aggregate functions for the installation are requested.
**
**            .db_datatype    The ADF datatype id for the type.
**
**        max_agnames     Max number of ADF_AGNAMES structures in array to fill.
**
**        agnames         Must point to an array of ADF_AGNAMES structures.
**
**        num_ags         Must point to nat.
**
** Outputs:
**        agnames         Filled with aggregate names and attributes.
**
**        num_ags         Set to number of ADF_AGNAMES structures filled.
**
**        Returns:
**                OK      if successful.
**                E_AF601B_MORE_AGG_NEED	if number of eligible
**						aggregates exceeds max_agnames.
*/
STATUS 
afe_agnames(
ADF_CB		*cb,
DB_DATA_VALUE	*dbv,
i4		max_agnames,
ADF_AGNAMES	*agnames,
i4		*num_ags)
{
    DB_DT_ID	type;
    STATUS	rval;
    ADI_FI_DESC	*fptr;
    AGNAMES	*ag;

    /*
    ** Find the number of aggregates for the type.  The
    ** remainder of the code depends on the side effect that
    ** afe_numaggs will initialize the Afe_agtab table.
    */
    if ((rval = afe_numaggs(cb, dbv, num_ags)) != OK)
	return rval;
    if (*num_ags > max_agnames)
    {
	if (dbv == NULL)
	    type = -1;
	return afe_error(cb, E_AF601B_MORE_AGG_NEED, 6,
			sizeof(*num_ags), (PTR) num_ags,
			sizeof(max_agnames), (PTR) &max_agnames,
			sizeof(type), (PTR) &type);
    }
    /*
    ** if the dbv is NULL, then return generic informatin
    ** for all aggregates.
    */
    if (dbv == NULL)
    {
	for (ag = Afe_agtab; ag->ag_name.ag_opid != ADI_NOOP; ag++, agnames++)
	{
	    MEcopy((PTR)&(ag->ag_name), sizeof(ag->ag_name), (PTR)agnames);
	}
	return OK;
    }
    type = abs(dbv->db_datatype);
    /*
    ** Now step through the aggregate table defined above.
    ** If the aggregate is applicable for the type look for
    ** the function instance for this type and add the information
    ** to the ag_name structure.
    */
    for (ag = Afe_agtab; ag->ag_name.ag_opid != ADI_NOOP; ag++)
    {
	/*
	** If the bit for the type is set, then the aggregate in
	** the current element is applicable for the type.
	*/
	if (BTtest(type, (char *) &ag->ag_dtmask))
	{
	    /*
	    ** agfitab points to what adi_fitab returns for
	    ** this aggregates opid.  Now search the entries in
	    ** the fitab to find the one for this type.  If we
	    ** don't find it (even though the bit test said we
	    ** should) we simply ignore it, and lower num_ags to
	    ** show we are returning one less.
	    */
	    for (fptr = ag->ag_fitab.adi_tab_fi;
		 fptr->adi_fiopid == ag->ag_name.ag_opid;
		 fptr++)
	    {
		if (fptr->adi_dt[0] == type || fptr->adi_dt[0] == DB_ALL_TYPE)
		    break;
	    }
	    if (fptr->adi_fiopid != ag->ag_name.ag_opid)
	    {
		*num_ags--;
	    }
	    else
	    {
		MEcopy((PTR)&(ag->ag_name), sizeof(ag->ag_name), (PTR)agnames);
		agnames->ag_fid = fptr->adi_finstid;
		agnames++;
	    }
	}
    }
    return OK;
}

/*{
**Name:   afe_numaggs   -   return number of aggregate functions
**
**Description:
**      Given a datatype (DB_DT_ID), returns the number of aggregate functions 
**      allowable for that datatype.  This information can be used to allocate 
**      an array of structures containing the aggregate information.
**	If a null pointer is passed instead of a pointer to a valid datatype,
**	then the number of all the aggregate functions for the installation
**	is returned.
**
** Inputs:
**        cb              A pointer to the current ADF_CB control block.
**
**        dbv            This is a DB_DATA_VALUE used to hold the datatype.
**			If this pointer is null, then the number of all 
**			of the allowable aggregate functions for the 
**			installation is requested.
**
**            .db_datatype    The ADF datatype id for the type.
**
**        result          Must point to a nat.
**
** Outputs:
**        result          Set to the number of aggregate functions.
**
**        Returns:
**                OK      if successful.
**		  E_AF601C_BAD_AGG_INIT  If error initializing an internal
**					 structure.
**
** History:
**	29-jul-2002 (abbjo03)
**	    In the initialization phase, set all bits of ag_dtmask if the
**	    datatype is DB_ALL_TYPE.
*/
STATUS
afe_numaggs(
ADF_CB          *cb,
DB_DATA_VALUE   *dbv,
i4              *result)
{
    AGNAMES	*ag;
    DB_DT_ID	type;
    STATUS	rval;
    ADI_FI_DESC	*fptr;
    ADI_OP_NAME	opname;
    DB_LANG	qlang_save;

    /*
    save query language from ADF_CB and set to "quel" for local use, so that
    all aggregates are known to adf.  This is because "any" and the unique
    aggregate functions ("avgu", "countu" and "sumu") are not valid for sql.
    */
    qlang_save = cb->adf_qlang;
    cb->adf_qlang = DB_QUEL;

    /*
    ** If the table has been initialized, then run through
    ** the table getting the opids for each aggregate and
    ** set the data type bit mask.
    */
    if (Afe_agtab[0].ag_name.ag_opid == ADI_NOOP)
    {
	/*
	** Initialize Afe_agtab.
	** For each aggregate in the table, gets its opid,
	** set the datatype bitmask, and initialize fitab
	** to what adi_fitab returns.
	*/
	for (ag = Afe_agtab; *ag->ag_name.ag_funcname != '\0'; ag++)
	{
	    STcopy(ag->ag_name.ag_funcname, opname.adi_opname);
	    if ((rval = adi_opid(cb, &opname, &(ag->ag_name.ag_opid))) != OK
		|| (rval = adi_fitab(cb, ag->ag_name.ag_opid,
					&(ag->ag_fitab))) != OK)
	    {
		Afe_agtab[0].ag_name.ag_opid = ADI_NOOP;
		/* restore original query language in ADF_CB */
		cb->adf_qlang = qlang_save;
		return afe_error(cb, E_AF601C_BAD_AGG_INIT, 2,
				sizeof(rval),(PTR) &rval);
	    }
	    fptr = ag->ag_fitab.adi_tab_fi;
	    MEfill(sizeof(ag->ag_dtmask), '\0', (PTR) &ag->ag_dtmask);
	    if (fptr->adi_dt[0] == DB_ALL_TYPE)
	    {
		for (type = 0; type < sizeof(ag->ag_dtmask.adi_dtbits); ++type)
		    BTset(type, (char *)&ag->ag_dtmask);
	    }
	    else
	    {
		for (; fptr->adi_fiopid == ag->ag_name.ag_opid; fptr++)
		    BTset(fptr->adi_dt[0], (char *) &ag->ag_dtmask);
	    }
	}
    }
    if (dbv == NULL)
    {
	*result = sizeof(Afe_agtab)/sizeof(Afe_agtab[0]);
    }
    else
    {
	/*
	** Go through the table checking the data type bit mask
	** for applicability to this type.
	*/
	*result = 0;
	type = abs(dbv->db_datatype);
	for (ag = Afe_agtab; *ag->ag_name.ag_funcname != '\0'; ag++)
	{
	    if (BTtest(type, (char *) &(ag->ag_dtmask)))
		(*result)++;
	}
    }
    /* restore original query language in ADF_CB */
    cb->adf_qlang = qlang_save;
    return OK;
}
