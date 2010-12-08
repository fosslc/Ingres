/*
** Copyright (c) 1986, Ingres Corporation
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
**  Name: ADIFIDESC.C - Get full description of function instance from id.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adi_fidesc()" which
**      returns a pointer to a full description of a function
**      instance given its id.
**
**      This file defines:
**
**          adi_fidesc() - Get full description of function instance from id.
**
**
**  History:
**      25-feb-1986 (thurston)
**          Initial creation.
**	19-may-1986 (thurston)
**	    Putting together a dummy (prototype) version of adi_fidesc() so
**	    Jeff can link with it and test his code.
**	12-jun-86 (thurston)
**	    Added the function instances for the "=" and "!=" operaters to the
**	    prototype version of adi_fidesc() so Jeff can test them.
**	27-jun-86 (thurston)
**	    Wrote the "real" version of the adi_fidesc() routine, now that the
**	    f.i. tables are ready.
**	30-jun-86 (thurston)
**	    Upgraded output parameter comments for the adi_fidesc() routine
**	    to describe the .adi_agprime field of the function instance
**          description. 
**	08-jul-86 (thurston)
**	    Fixed bug in adi_fidesc().  The routine would not return an f.i.
**	    description for the largest numbered f.i. ID.  It would return
**	    the error status E_AD2010_BAD_FIID instead.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	28-feb-89 (fred)
**	    Altered references to Adi_* global variables to be thur Adf_globs
**	    for user-defined datatype support.
**      21-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**/


/*{
** Name: adi_fidesc()     - Get full description of function instance from id.
**
** Description:
**      This function is the external ADF call "adi_fidesc()" which
**      returns a pointer to a full description of a function
**      instance given its id.
**
**      This description will be an ADI_FI_DESC structure and will contain
**      function instance id,  the function instance id of the complement
**      function instance (for the comparisons), the type of function instance,
**      the operator id of the function, the method for calculating the length
**      of the result, the length to make the DB_DATA_VALUE used for work space
**      by the aggregates (for the aggregates), whether the aggregate f.i. is
**	prime or not (for the aggregates), the number of arguments (or input
**	operands), and the datatype ids for the result and all arguments.
**
**	Note that if the any input argument's type is DB_ALL_TYPE, this means
**	that any type is acceptable as an input type for this function
**	instance and that its output type will be determined by calling
**	adi_res_type().  The rest of ADF doesn't know about this type so it should
**	not be stored in (for example) DB_DATA_VALUEs.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_fid                         The function instance id to
**                                      find the full description for.
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
**	adi_fdesc                       Pointer to the full description of the
**					function instance.
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
**          E_AD2010_BAD_FIID           Function instance id unknown
**					to ADF.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      25-feb-86 (thurston)
**          Initial creation.
**	19-may-86 (thurston)
**	    Putting together a dummy (prototype) version of adi_fidesc() so
**	    Jeff can link with it and test his code.
**	12-jun-86 (thurston)
**	    Added the function instances for the "=" and "!=" operaters to the
**	    prototype so Jeff can test them.
**	27-jun-86 (thurston)
**	    Wrote the "real" version of the routine, now that the f.i. tables
**	    are ready.
**	30-jun-86 (thurston)
**	    Upgraded output parameter comments to describe the .adi_agprime
**	    field of the function instance description.
**	08-jul-86 (thurston)
**	    Fixed bug.  This routine would not return an f.i.
**	    description for the largest numbered f.i. ID.  It would return
**	    the error status E_AD2010_BAD_FIID instead.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adi_fidesc(
ADF_CB		    *adf_scb,
ADI_FI_ID	    adi_fid,
ADI_FI_DESC	    **adi_fdesc)
{
    ADI_FI_DESC		*fi;


    *adi_fdesc = (ADI_FI_DESC *) NULL;	    /* return NULL if not found */

    if (    adi_fid < (ADI_FI_ID) 0
        ||  adi_fid > Adf_globs->Adi_fi_biggest
        ||  (fi = (ADI_FI_DESC *)Adf_globs->Adi_fi_lkup[
					ADI_LK_MAP_MACRO(adi_fid)
							    ].adi_fi) == NULL
       )
	return (adu_error(adf_scb, E_AD2010_BAD_FIID, 0));

    *adi_fdesc = fi;
    return (E_DB_OK);
}
