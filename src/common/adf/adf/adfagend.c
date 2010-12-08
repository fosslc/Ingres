/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADFAGEND.C - Return the result of an aggregation.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adf_agend()" which
**      returns the result of the aggregation that has been
**      (is being) performed.
**
**      This file defines:
**
**          adf_agend() - Return the result of an aggregation.
**
**
**  History:
**      27-feb-86 (thurston)
**          Initial creation.
**	23-jul-86 (thurston)
**	    Initial coding of adf_agend().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	10-nov-87 (thurston)
**	    Added code to adf_agend() to handle nullable datatypes for result
**	    of aggregates, as well as the SQL semantics for aggregates over
**	    empty sets.
**	28-feb-89 (fred)
**	    Altered references to Adi_* global variables to be thru the
**	    Adf_globs structure for UDADT support.
**	09-mar-92 (jrb, merged by stevet)
**	    Handle DB_ALL_TYPE in function instance.  For outer join project.
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
** Name: adf_agend()	- Return the result of an aggregation.
**
** Description:
**      This function is the external ADF call "adf_agend()" which
**      returns the result of the aggregation that has been
**      (is being) performed.
**
**      This routine is part of the "three phase" aggregate processing
**      approach adopted for Jupiter.  The code that needs to perform
**      an aggregate will now do it in three steps.  First, the
**      adf_agbegin() routine will be called to initialize the
**      aggregate.  Second, the adf_agnext() routine will be called
**      once for each data value that is to be added to the aggregate.
**      Third, a call to this routine (adf_agend()) will yield the
**      desired result.
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
**	    .adf_qlang			Query language in effect.
**      adf_agstruct                    Pointer to an ADF_AG_STRUCT that
**                                      was previously initialized via
**                                      the adf_agbegin() call.  This
**                                      structure will not be modified
**                                      by this routine, thereby
**                                      allowing the result of this
**					routine to be an intermediate
**					result where more calls to
**					adf_agnext() can be made to
**					continue the aggregation.
**	adf_dvresult			Pointer to the DB_DATA_VALUE
**					to put the result of the
**					aggregation in.
**		.db_datatype		Datatype id of the result.
**		.db_length  		Length of the result.
**		.db_data		Pointer to location to put the
**					actual data of the result in.
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
**	adf_dvresult			Data value which is the result
**					of the aggregation.
**	       *.db_data		The actual data of the resulting
**					data value will be placed here.
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
**	    E_AD1012_NULL_TO_NONNULL	The aggregate now ending should return a
**					NULL value, but the result datatype is
**					not nullable.
**          E_AD2004_BAD_DTID		Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN		Length is illegal for the
**					given datatype.
**          E_AD2010_BAD_FIID           Function instance id unknown to ADF.
**          E_AD4002_FIID_IS_NOT_AG     The function instance id is not
**                                      that of an aggregate function.
**          E_AD4004_BAD_AG_DTID	Datatype id of result is not
**                                      what this aggregation is
**					expecting.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-jan-86 (thurston)
**          Initial creation.
**	23-jul-86 (thurston)
**	    Initial coding.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	10-nov-87 (thurston)
**	    Added code to handle nullable datatypes for result of aggregates, as
**	    well as the SQL semantics for aggregates over empty sets.
**	09-mar-92 (jrb, merged by stevet)
**	    Handle DB_ALL_TYPE in function instance.  For outer join project.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adf_agend(
ADF_CB             *adf_scb,
ADF_AG_STRUCT      *adf_agstruct,
DB_DATA_VALUE      *adf_dvresult)
{
    ADI_FI_ID		fid = adf_agstruct->adf_agfi;
    ADI_FI_DESC		*fidesc;
    DB_DATA_VALUE	tmp_dv;
    DB_DATA_VALUE	*dv;
    i4			bdt = abs((i4) adf_dvresult->db_datatype);
    DB_STATUS		db_stat;
    i4		err_code;
    

    for (;;)	    /* something to break out of */
    {
	/* Check for valid ag-fiid */
	/* ----------------------- */
	err_code = E_AD2010_BAD_FIID;
	if (    fid < (ADI_FI_ID) 0
	    ||  fid > Adf_globs->Adi_fi_biggest
	    ||  (fidesc =
		    (ADI_FI_DESC *)Adf_globs->Adi_fi_lkup[
						ADI_LK_MAP_MACRO(fid)
							    ].adi_fi) == NULL
	   )
	    break;

	err_code = E_AD4002_FIID_IS_NOT_AG;
	if (fidesc->adi_fitype != ADI_AGG_FUNC)
	    break;


	/* Make sure datatype id is what is expected */
	/* ----------------------------------------- */
	err_code = E_AD4004_BAD_AG_DTID;
	if (bdt != fidesc->adi_dtresult && fidesc->adi_dtresult != DB_ALL_TYPE)
	    break;

	err_code = E_AD0000_OK;
	break;
    }

    if (err_code != E_AD0000_OK)
    {
	db_stat = adu_error(adf_scb, err_code, 0);
    }
    else
    {
	db_stat = E_DB_OK;

	/*
	** Check for possible NULL value return (only happens on empty set, when
	** the query language is SQL, and the ag-op is NOT count or count(*)):
	** ---------------------------------------------------------------------
	*/
	if (    adf_agstruct->adf_agcnt == 0
	    &&  adf_scb->adf_qlang == DB_SQL
	    &&  fidesc->adi_fiopid != ADI_CNT_OP
	    &&  fidesc->adi_fiopid != ADI_CNTAL_OP
	   )
	{
	    if (adf_dvresult->db_datatype < 0)    /* nullable result */
	    {
		ADF_SETNULL_MACRO(adf_dvresult);
		db_stat = E_DB_OK;
	    }
	    else
	    {
		db_stat = adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0);
	    }
	}
	else
	{
	    if (adf_dvresult->db_datatype < 0)    /* nullable result */
	    {
		ADF_CLRNULL_MACRO(adf_dvresult);
		STRUCT_ASSIGN_MACRO(*adf_dvresult, tmp_dv);
		tmp_dv.db_datatype = bdt;
		tmp_dv.db_length--;
		dv = &tmp_dv;
	    }
	    else
	    {
		dv = adf_dvresult;
	    }

	    /* Call the proper agend routine found in the fi_lkup table */
	    /* -------------------------------------------------------- */
	    db_stat =((*Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fid)].adi_agend)
						      (adf_scb, adf_agstruct,
								  dv));
	}
    }

    return (db_stat);
}
