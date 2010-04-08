/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADFAGBEGIN.C - Init for performing an aggregate function instance.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adf_agbegin()" which
**      does initialization for performing any supplied aggregate
**      function instance.
**
**      This file defines:
**
**          adf_agbegin() - Init for performing an aggregate function instance.
**
**
**  History:    $Log-for RCS$
**      26-feb-86 (thurston)
**          Initial creation.
**	23-jul-86 (thurston)
**	    Initial coding of adf_agbegin().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	28-feb-89 (fred)
**	    Changed all references to global areas Adi_* to be thru Adf_globs
**	    members (of the same names) for udadt support
**      06-jul-91 (jrb)
**          Removed check to insure workspace in agstruct was sufficient.
**      23-dec-1992 (stevet)
**          Added function prototype.
**/


/*{
** Name: adf_agbegin()	- Init for performing an aggregate function instance.
**
** Description:
**      This function is the external ADF call "adf_agbegin()" which
**      does initialization for performing any supplied aggregate
**      function instance.
**
**      This routine is part of the "three phase" aggregate processing
**      approach adopted for Jupiter.  The code that needs to perform
**      an aggregate will now do it in three steps.  First, this routine
**      (adf_agbegin()) will be called to initialize the aggregate.
**      Second, the adf_agnext() routine will be called once for each
**      data value that is to be added to the aggregate.  Third, a call
**      to the adf_agend() routine will yield the desired result.
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
**      adf_agstruct                    Pointer to an ADF_AG_STRUCT that
**                                      will be initialized by this
**                                      routine for the upcoming
**                                      aggregate processing.  This
**                                      structure will constantly be
**                                      updated during the aggregate
**                                      processing by the adf_agnext()
**                                      routine, and should not be
**                                      touched by any other code until
**                                      the aggregate processing is
**                                      completed.
**              .adf_agfi               Function instance id for the
**                                      operation being requested.
**                                      (Must be that of an aggregate
**                                      function instance.)
**              .adf_agwork             This is a DB_DATA_VALUE that can
**                                      be used as work space during the
**                                      upcoming aggregate processing.
**                      .db_length      Length of work space.
**                      .db_data        Pointer to work space.  This
**                                      area will be used throughout
**                                      the aggregate processing by
**                                      the adf_agnext() routine, and
**                                      should not be touched by any
**                                      other code until the aggregate
**                                      processing is completed.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
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
**      Exceptions:
**          none
**
** Side Effects:
**          The ADF_AG_STRUCT passed in via the pointer adf_agstruct
**          will be altered during this aggregate initialization.  Also,
**          the work area pointed to by adf_agstruct.adf_agwork.db_data
**          may be altered, depending on the aggregate function instance
**          being initialized for.
**
** History:
**      26-jan-86 (thurston)
**          Initial creation.
**	23-jul-86 (thurston)
**	    Initial coding.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**      06-jul-91 (jrb)
**          Removed check to insure workspace in agstruct was sufficient.
**          This is because we now adjust the db_length in the agstruct in
**          the ag_next action.  Bug 37069.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adf_agbegin(
ADF_CB             *adf_scb,
ADF_AG_STRUCT      *adf_agstruct)
# else
DB_STATUS
adf_agbegin( adf_scb, adf_agstruct)
ADF_CB             *adf_scb;
ADF_AG_STRUCT      *adf_agstruct;
# endif
{
    ADI_FI_ID		fid = adf_agstruct->adf_agfi;
    ADI_FI_DESC		*fidesc;


    /* Check for valid ag-fiid */
    /* ----------------------- */
    if (    fid < (ADI_FI_ID) 0
	||  fid > Adf_globs->Adi_fi_biggest
	||  (fidesc = (ADI_FI_DESC *)
		    Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fid)].adi_fi)
					    == NULL
       ) return(adu_error(adf_scb, E_AD2010_BAD_FIID, 0));

    if (fidesc->adi_fitype != ADI_AGG_FUNC)
        return(adu_error(adf_scb, E_AD4002_FIID_IS_NOT_AG, 0));


    /* Call the proper agbegin routine found in the fi_lkup table */
    /* ---------------------------------------------------------- */
    return((*Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fid)].adi_agbgn)
				(adf_scb, adf_agstruct));
}
