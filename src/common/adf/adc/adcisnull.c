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
#include    <adudate.h>

/**
**
**  Name: ADCISNULL.C - Tell whether a data value is NULL or not.
**
**  Description:
**      Routine returns a i4  which is TRUE if the supplied data value is NULL 
**      and FALSE if it is non-NULL.  For non-NULLable types, FALSE will always 
**      be returned.
**
**	This file contains the following routines:
**
**          adc_isnull() - Tells whether a data value is NULL or not.
**
**
**  History:
**      30-may-86 (thurston)    
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	24-oct-86 (thurston)
**	    In the adc_1isnull_rti() routine, I added support for "char" and
**	    "varchar", and fixed up "date" so that a standard date datatype
**	    (i.e. non-nullable) always returns false, since the empty date
**	    (unfortunately refered to by the AD_DN_NULL bit in the dates status
**	    field) is not the NULL date.
**	17-feb-87 (thurston)
**	    Removed the adc_1isnull_rti() routine since it is no longer
**	    needed.  In fact, this routine does not need to be a "common
**	    datatype" routine anymore, since the approved definition of
**	    what constitutes a null value is datatype independent.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**      22-dec-1992 (stevet)
*           Added function prototype.
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
** Name: adc_isnull() - Tells whether a data value is NULL or not.
**
** Description:
**      Routine returns a i4  which is TRUE if the supplied data value is NULL 
**      and FALSE if it is non-NULL.  For non-NULLable types, FALSE will always 
**      be returned.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv                          Pointer to the supplied DB_DATA_VALUE
**                                      to check for a NULL value.
**	    .db_datatype		The datavalue's datatype.
**	    .db_prec			The datavalue's precision/scale.
**	    .db_length  		The datavalue's length.
**	    .db_data    		Pointer to the actual data.
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
**      adc_null                        This will be set to TRUE if the data
**                                      value was NULL, and FALSE if not.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Operation succeeded.
**	    E_AD2004_BAD_DTID		Unknown datatype.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-may-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	17-feb-87 (thurston)
**	    Removed the call to the adc_1isnull_rti() routine since it
**	    is no longer needed.  In fact, this routine does not need to
**	    be a "common datatype" routine anymore, since the approved
**	    definition of what constitutes a null value is datatype
**	    independent.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_isnull(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *adc_dv,
i4                 *adc_null)
# else
DB_STATUS
adc_isnull( adf_scb, adc_dv, adc_null)
ADF_CB             *adf_scb;
DB_DATA_VALUE      *adc_dv;
i4                 *adc_null;
# endif
{
    i4		   bdt = abs((i4) adc_dv->db_datatype);

    /* does the datatype id exist? */
    if (bdt <= 0  ||  ADI_DT_MAP_MACRO(bdt) > ADI_MXDTS
	||  Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)] == NULL)
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    if (adc_dv->db_datatype > 0)
	*adc_null = FALSE;
    else
	*adc_null = ADI_ISNULL_MACRO(adc_dv);

    return(E_DB_OK);
}
