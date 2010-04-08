/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADFAGNEXT.C - Add another data value to an aggregation in progress.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adf_agnext()" which
**      compiles another data value into an aggregation already
**      in progress.
**
**      This file defines:
**
**          adf_agnext() - Add another data value to an aggregation in progress.
**
**
**  History:
**      26-feb-86 (thurston)
**          Initial creation.
**	23-jul-86 (thurston)
**	    Initial coding of adf_agnext().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	10-nov-87 (thurston)
**          Added code to adf_agnext() to handle nullable datatypes.  Not sure
**          how this routine has survived so long without it, but I guess the
**          only caller is the Report Writer, so it is not a heavily used
**          interface.  Also not sure how it got left out in the first place.
**	28-feb-89 (fred)
**	    Altered references to Adi_* global variables to be thru Adf_globs
**	    for UDADT support
**      06-jul-91 (jrb)
**          Removed check to insure workspace in agstruct was sufficient.
**	09-mar-91 (jrb, merged by stevet)
**	    Handle DB_ALL_TYPE for outer-join project.
**      23-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: adf_agnext()	- Add another data value to an aggregation in progress.
**
** Description:
**      This function is the external ADF call "adf_agnext()" which
**      compiles another data value into an aggregation already
**      in progress.
**
**      This routine is part of the "three phase" aggregate processing
**      approach adopted for Jupiter.  The code that needs to perform
**      an aggregate will now do it in three steps.  First, the
**      adf_agbegin() routine will be called to initialize the
**      aggregate.  Second, this routine (adf_agnext()) will be called
**      once for each data value that is to be added to the aggregate.
**      Third, a call to the adf_agend() routine will yield the desired
**      result.
**
**      This system has been designed such that intermediate results
**      can be retrieved if desired by calling adf_agend() in the middle
**      of the adf_agnext() calls, since it does not disturb the work
**      space used by the aggregation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			Pointer to an ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adf_dvnext			Pointer to the data value to be
**					added to the aggregation defined
**					by adf_agstruct.
**		.db_datatype		Datatype id of this data value.
**		.db_length		Length of this data value.
**		.db_data    		Pointer to the actual data for
**					this data value.
**      adf_agstruct                    Pointer to an ADF_AG_STRUCT that
**                                      was previously initialized via
**                                      the adf_agbegin() call.  This
**                                      structure will constantly be
**                                      updated during the aggregate
**                                      processing by this routine, and
**                                      should not be touched by any
**                                      other code until the aggregate
**                                      processing is completed.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			Pointer to an ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2010_BAD_FIID           Function instance id unknown to ADF.
**          E_AD4002_FIID_IS_NOT_AG     The function instance id is not
**                                      that of an aggregate function.
**
**		The following returns will only happen when the system is
**		compiled with the xDEBUG option:
**
**          E_AD4004_BAD_AG_DTID	Datatype id of "next" data value
**					is not what this aggregation is
**					expecting.
**
**      Exceptions:
**          none
**
** Side Effects:
**          The ADF_AG_STRUCT passed in via the pointer adf_agstruct
**          will be altered during this routine.  Also, the work area
**          pointed to by adf_agstruct.adf_agwork.db_data may be altered,
**          depending on the aggregate function instance being used.
**
** History:
**      26-jan-86 (thurston)
**          Initial creation.
**	23-jul-86 (thurston)
**	    Initial coding.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	10-nov-87 (thurston)
**	    Added code to handle nullable datatypes.  Not sure how this routine
**	    has survived so long without it, but I guess the only caller is the
**	    Report Writer, so it is not a heavily used interface.  Also not sure
**	    how it got left out in the first place.
**      06-jul-91 (jrb)
**          Removed check to insure workspace in agstruct was sufficient.
**          This is because we now adjust the db_length in the agstruct in
**          the ag_next action.  Bug 37069.
**	09-mar-92 (jrb, merged by stevet)
**	    Handle DB_ALL_TYPE for outer-join project.
**	10-may-2001 (gupsh01)
**	    Added support for ADE_5CXI_CLR_SKIP. (Bug #104697)
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adf_agnext(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *adf_dvnext,
ADF_AG_STRUCT      *adf_agstruct)
# else
DB_STATUS
adf_agnext( adf_scb, adf_dvnext, adf_agstruct)
ADF_CB             *adf_scb;
DB_DATA_VALUE      *adf_dvnext;
ADF_AG_STRUCT      *adf_agstruct;
# endif
{
    ADI_FI_ID		fid = adf_agstruct->adf_agfi;
    ADI_FI_DESC		*fidesc;
    DB_DATA_VALUE	*dv;
    DB_DATA_VALUE	tmp_dv;
    i4			bdt;
    

    /* Check for valid ag-fiid */
    /* ----------------------- */
    if (    fid < (ADI_FI_ID) 0
	||  fid > Adf_globs->Adi_fi_biggest
	||  (fidesc = (ADI_FI_DESC *)
		Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fid)].adi_fi)
				== NULL
       )
	return (adu_error(adf_scb, E_AD2010_BAD_FIID, 0));

    if (fidesc->adi_fitype != ADI_AGG_FUNC)
	return (adu_error(adf_scb, E_AD4002_FIID_IS_NOT_AG, 0));

    /* Make sure datatype id is what is expected */
    /* ----------------------------------------- */
    if (fidesc->adi_numargs == 0)
    {
	dv = NULL;
	bdt = 0;
    }
    else
    {
	dv = adf_dvnext;
	bdt = abs((i4) dv->db_datatype);
	if (bdt != fidesc->adi_dt[0]  &&  fidesc->adi_dt[0] != DB_ALL_TYPE)
	    return (adu_error(adf_scb, E_AD4004_BAD_AG_DTID, 0));
    }



/* ======================================= */
/* Finally, execute the function instance: */
/* ======================================= */

    /* If nullable, and need a nullable pre-instruction, handle specially */
    /* ------------------------------------------------------------------ */
    if (    fidesc->adi_npre_instr != ADE_0CXI_IGNORE
	&&  (dv != NULL  &&  adf_dvnext->db_datatype < 0)
       )
    {
	switch (fidesc->adi_npre_instr)
	{
	  case ADE_2CXI_SKIP:	/* Only valid for aggregates */
	  case ADE_5CXI_CLR_SKIP:	/* Only valid for aggregates */

	    if (dv != NULL  &&  ADI_ISNULL_MACRO(dv))
	    {	/*
		** Null value found:  Skip this one.
		*/
		return (E_DB_OK);
	    }
	    else
	    {	/*
		** No null values found:  Set up temp DB_DATA_VALUE
		** and execute instr as though non-nullable.
		*/
		if (dv != NULL  &&  dv->db_datatype < 0)
		{
		    STRUCT_ASSIGN_MACRO(*dv, tmp_dv);
		    tmp_dv.db_datatype = bdt;
		    tmp_dv.db_length--;
		    dv = &tmp_dv;
		}
	    }
	    break;

	  case ADE_1CXI_SET_SKIP:	/* Only valid for non-aggregates */
	  case ADE_3CXI_CMP_SET_SKIP:	/* Only valid for non-aggregates */
	  default:
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}	/* END OF SWITCH STATEMENT */
    }


    /* Finally, call the proper agnext routine found in the fi_lkup table */
    /* ------------------------------------------------------------------ */
    return ((*Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fid)].adi_func)
					    (adf_scb, dv, adf_agstruct));
}
