/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clfloat.h>
#include    <cv.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADCDFCHK.C - Check to make sure default value can be coerced into
**                     result datatype/length/precision.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_defchk()" which
**      checks the input value for DEFAULT to see if it can be
**      coerced to the given resulting datatype/length/precision.
**
**      This file defines:
**
**          adc_defchk() - Check DEFAULT value.
**
**
**  History:    
**      03-feb-1993 (stevet)
**          Initial creation.
**      15-jun-1993 (stevet)
**          Cannot rely on adc_compare to check for lostless for 
**          DB_FLT_TYPE since it is not an exact numeric type.
**      24-jun-1993 (rblumer)
**          Unmap coercion error; allow it to be returned to PSF,
**          as PSF maps this to a different error than E_AD1090_BAD_DTDEF.
**      19-jul-1993 (stevet)
**          Checking MAX_FLT and MIN_FLT for float4 datatype.  Disabled
**          checking for MATH exceptions since DEFAULT does not using
**          the normal MATH exception handling mechanism.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      10-aug-1993 (stevet, rblumer)
**          map datatype into defined range using ADI_DT_MAP_MACRO.
**          Also put paren's around the assignment in 'db_stat = x != E_DB_OK'
**          statements, because db_stat was getting set to bogus value
**          (1 instead of 5 (E_DB_ERROR)).
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: adc_defchk()	- Check DEFAULT value.
**
** Description:
**      This function is the external ADF call "adc_defchk()" which
**      will coerce the input value into the output datatype/length/precision
**      and make sure the input is lost-less (i.e. no truncation of 
**      string, overflow of integer etc).  To guarantee lost-less, 
**      the input data value is coerced to output data value and then
**      coerced back to input datatype/length/precision before comparing
**      for equality.  
**
**      This function will return an error when
**
**          datatype    value       description or comment
**          --------	-----       ----------------------
**          char(10)   	5           Cannot coerce INT to CHAR.
**          date	'2/30/93'   Invalid date.
**          i2      	1234567890  Integer overflow
**          dec(5,2)    1.2345      Lost of precision
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv     			Pointer to DB_DATA_VALUE to
**                                      place input data value for DEFAULT.
**		.db_datatype		The datatype for the input data
**					value.
**		.db_prec		The precision/scale input data
**					value.
**		.db_length		The length for the input data
**					value.
**		.db_data		Pointer to location to place
**					the .db_data field for the input
**					data value. 
**      adc_rdv     			Pointer to DB_DATA_VALUE to
**                                      place result data value for DEFAULT.
**		.db_datatype		The datatype for the result data
**					value.
**		.db_prec		The precision/scale result data
**					value.
**		.db_length		The length for the result data
**					value.
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
**      adc_rdv                         The result data value.
**              .db_data		The data for the result data
**                                      value will be placed here.
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
**          E_DB_OK                     Operation succeeded.
**          E_AD2004_BAD_DTID           One of the datatype ids is
**					unknown to ADF.
**          E_AD2009_NOCOERCION         There is no coercion for the
**                                      given datatypes.
**          E_AD1090_BAD_DTDEF          Bad datatype default value
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      03-feb-1993 (stevet)
**          Initial creation.
**      06-apr-1993 (stevet)
**          Map coercion error to E_AD1090_BAD_DTDEF.
**      15-jun-1993 (stevet)
**          Cannot rely on adc_compare to check for lostless for 
**          DB_FLT_TYPE since it is not an exact numeric type.
**      24-jun-1993 (rblumer)
**          Unmap coercion error; allow it to be returned to PSF,
**          as PSF maps this to a different error than E_AD1090_BAD_DTDEF.
**      19-jul-1993 (stevet)
**          Checking MAX_FLT and MIN_FLT for float4 datatype.  Disabled
**          checking for MATH exceptions since DEFAULT does not using
**          the normal MATH exception handling mechanism.
**      10-aug-1993 (stevet, rblumer)
**          map datatypes into defined range using ADI_DT_MAP_MACRO.
**          Also put parentheses around assignment in 'db_stat = x != E_DB_OK'
**          statements, because db_stat was getting set to bogus value
**          (1 instead of 5 (E_DB_ERROR)).
**          Also, don't map initial coercion errors to E_AD1090_BAD_DTDEF;
**          this allows the caller (PSF) to print out both a general default
**          error and then a more specific error.
**      16-aug-1993 (stevet)
**          Disable string truncation detection during the DEFAULT
**          processing since the build in mechanism already able to
**          detect string truncations.
**	28-apr-94 (vijay)
**	    Change abs to fabs.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
*/
DB_STATUS
adc_defchk(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DT_ID            dv_dt   = abs(dv->db_datatype);
    DB_DT_ID            res_dt  = abs(rdv->db_datatype);
    i4                  dv_len  = dv->db_length;
    i4                  res_len = rdv->db_length;
    ADX_MATHEX_OPT      input_exmathopt=adf_scb->adf_exmathopt;
    ADF_STRTRUNC_OPT    input_strtrunc = adf_scb->adf_strtrunc_opt;


    /* Return error if input is abstract but output is intrinsic */
    if(   (Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(dv_dt)]->adi_dtstat_bits 
	   & AD_INTR) == 0
       && (Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(res_dt)]->adi_dtstat_bits 
	   & AD_INTR) != 0)  
	return( adu_error( adf_scb, E_AD1090_BAD_DTDEF, 0));

    /* Disable MATH exception handling and string truncation */
    adf_scb->adf_exmathopt = ADX_IGN_MATHEX;
    adf_scb->adf_strtrunc_opt  = ADF_IGN_STRTRUNC;

    /* Only do work if output type/length/precision is different from input */
    if( dv->db_datatype < 0)
	dv_len--;

    if( rdv->db_datatype < 0)
	res_len--;

    if( dv_dt != res_dt || dv_len != res_len ||
        (dv_dt == DB_DEC_TYPE && dv->db_prec != rdv->db_prec))
    {
	do
	{
	    /* Try coerce input DV to output DV and report error if failed */
	    if( (db_stat = adc_cvinto( adf_scb, dv, rdv)) != E_DB_OK)
	    {
		break;
	    }
	    /* 
	    ** If output type is intrinsic then we want to see if the coercions
	    ** is lost-less
	    */
	    if ((Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(res_dt)]
		                             ->adi_dtstat_bits & AD_INTR) != 0)
	    {
		/* Since float is non exact numeric, we cannot compare the
		** conversion results to make sure the default value is
		** valid.  Instead we only check f8 -> f4 overflow
		*/
		if( res_dt == DB_FLT_TYPE)
		{
		    char           temp[8 + DB_ALIGN_SZ];
		    DB_DATA_VALUE  tmp_dv;

		    /* Nothing we can check for float8 */
		    if( res_len == 4)
		    {
			tmp_dv.db_datatype = DB_FLT_TYPE;    
			tmp_dv.db_length = 8;
			tmp_dv.db_data = ME_ALIGN_MACRO((PTR)temp,DB_ALIGN_SZ);

			if( (db_stat = adc_cvinto( adf_scb, dv, &tmp_dv))
			   != E_DB_OK)
			    break;

			/* we want to make sure f8 -> f4 will fit */
			if( CV_ABS_MACRO(*(f8 *)tmp_dv.db_data) > FLT_MAX ||
			    CV_ABS_MACRO(*(f8 *)tmp_dv.db_data) < FLT_MIN)
			    db_stat = 
				adu_error(adf_scb, E_AD1090_BAD_DTDEF, 0);
		    }
		}
		else
		{
		    char           temp[DB_MAXTUP + DB_ALIGN_SZ + 1];
		    i4             cmp;
		    DB_DATA_VALUE  tmp_dv;

		    /* Set up temporary DV */
		    STRUCT_ASSIGN_MACRO( *dv, tmp_dv);    

		    /* Align output buffer */
		    tmp_dv.db_data = ME_ALIGN_MACRO((PTR)temp, DB_ALIGN_SZ);

		    if( (db_stat = adc_cvinto( adf_scb, rdv, &tmp_dv) )
		        != E_DB_OK)
		    break;

		    /* Compare results */
		    if( (db_stat = adc_compare( adf_scb, dv, &tmp_dv, &cmp))
		       != E_DB_OK)
		    break;

		    /* Return error if values are not equal */
		    if( cmp != 0)
			db_stat = adu_error(adf_scb, E_AD1090_BAD_DTDEF, 0);
		}
	    }
	} while( FALSE);
    }
    adf_scb->adf_exmathopt = input_exmathopt;
    adf_scb->adf_strtrunc_opt = input_strtrunc;
    return(db_stat);
}

