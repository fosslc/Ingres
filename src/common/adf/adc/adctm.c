/*
** Copyright (c) 1986, 2009 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adudate.h>
#include    <adumoney.h>


#include <clfloat.h>

/**
**
**  Name: ADCTM.C - Contains routines for converting datatypes to be
**		    displayed by the terminal monitor.
**
**  Description:
**        This file contains routines used to convert data values
**	to be displayed by the terminal monitor.
**
**          adc_tmlen() - Provide default and worst-case lengths for display
**			  format of passed in datatype.
**	    adc_tmcvt() - Convert data value to terminal monitor display format.
**          adc_1tmlen_rti() - ... for the RTI datatypes.
**          adc_2tmcvt_rti() - ... for the RTI datatypes.
**
**  History:
**      24-nov-86 (thurston)
**	    Initial creation.
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**      14-apr-87 (thurston)
**          Added longtext to adc_1tmlen_rti() and adc_1tmcvt_rti().  Also fixed
**	    bug in adc_1tmcvt_rti() for displaying dates.
**      07-jul-87 (thurston)
**          Changed an erroneous F4ASSIGN_MACRO to the correct F8ASSIGN_MACRO in
**          adc_2tmcvt_rti().
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	20-jun-88 (jrb)
**	    Mods for Kanji support
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	27-mar-89 (mikem)
**	    Added support for the logical_key datatype to adc_1tmlen_rti() and
**	    to adc_2tmcvt_rti().
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	31-may-89 (jrb)
**	    Added a cast to f8 on f4tmp in CVfa call although it still worked
**	    without this cast (since floats get casted to double for function
**	    calls in C).
**      05-jun-89 (fred)
**          Completed last change.
**	06-jul-89 (jrb)
**	    Added decimal support.
**	10-feb-90 (dkh)
**	    Put in changes to reduce calls on STlength to speed up terminal
**	    monitor.
**	15-feb-90 (dkh)
**	    Changed handling of non-printable characters adc_2tmcvt_rti() to
**	    make it go slightly faster.
**      24-July-1991 (fredv)
**          Need to use I1_CHECK_MACRO for i1 data type.
**          Otherwise, we get incorrect result from tm for -128.
**      03-Oct-1992 (fred)
**          Add BIT and VBIT type support.
**      23-dec-1992 (stevet)
**          Added function prototype.
**       6-Apr-1993 (fred)
**          Added byte string types.
**      23-apr-1993 (smc)
**          Removed text after endif.
**      23-apr-1993 (stevet)
**          Added missing define <st.h>.
**	21-mar-1993 (robf)
**	    Include sl.h
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-aug-1993 (stevet)
**          Fixed type mismatch problem when calling CVpka().
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	19-may-94 (jrb/kirke)
**	    Fixed problem in adc_2tmcvt_rti() where tm could endlessly
**	    loop when blank padding output field for DB_CHR_TYPE.
**	02-jul-96 (thoda04)
**	    Added parameter prototypes to ad0_printfatt();
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-feb-2001 (gupsh01)
**	    Added support for nchar and nvarchar data type.
**	09-march-2001 (gupsh01)
**	    Corrected error with nchar types.
**	13-mar-2001 (somsa01)
**	    Changed defwid and worstwid to i4's, as potentially the worst
**	    case width is 4 * 32000, which is more than an i2.
**	25-nov-2003 (gupsh01)
**	    Chanded the defwid and worstwid for nchar and nvarchar's. Since 
**	    These datatypes are coerced to varchar types, the defwid should
**	    be of the equivalent character types.
**	29-jun-2005 (thaju02)
**	    adc_2tmcvt_rti(): For nvchr, use length of coerced data to 
**	    determine endp. (B114780)
**  19-feb-2009 (joea)
**      Remove #ifdefs from 5-mar-2008 fix to adc_2tmcvt_rti so that it's
**      visible on all platforms.
**      22-sep-2009 (joea)
**          Add cases for DB_BOO_TYPE in adc_1tmlen_rti and adc_2tmcvt_rti.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Ensure whole DBV copied.
**/


/*
**  Definition of static variables and forward static functions.
*/

static  VOID        ad0_printfatt(char *value, i4  vallen, 
	                              i4  width, char *buf);


/*{
** Name: adc_tmlen() - Provide default and worst-case lengths for display format
**		       of passed in datatype.
**
** Description:
**      This routine is used to retrieve both the default width (smallest
**	display field width) and the worst-case width for the supplied datatype.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_outarg 		Output formatting information.  This
**					is an ADF_OUTARG structure, and contains
**                                      many pieces of information on how the
**                                      various intrinsic datatypes are supposed
**                                      to be formatted when converted into
**                                      string representation.
**		.ad_c0width		Minimum width of "c" or "char" field.
**		.ad_i1width		Width of "i1" field.
**		.ad_i2width		Width of "i2" field.
**		.ad_i4width		Width of "i4" field.
**		.ad_f4width		Width of "f4" field.
**		.ad_f8width		Width of "f8" field.
**		.ad_i8width		Width of "i8" field.
**		.ad_t0width		Min width of "text" or "varchar" field.
**	    .adf_nullstr		Description of the NULLstring in use.
**	        .nlst_length		Length of the NULLstring.
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					datatype and length in question.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision/scale
**	    .db_length			Its length.
**
** Outputs:
**	adf_scb
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
**	adc_defwid			The length, in bytes, of the default
**					print field size for the given datatype
**					and length.
**	adc_worstwid			The length, in bytes, of the worst-case
**					print field size for the given datatype
**					and length.  That is, what is the
**					largest possible print field needed for
**					any data value with the given datatype
**					and length.  Note, that if the datatype
**					in question is a nullable one, this
**					will be at least as large as the length
**					of the NULLstring:
**					adf_scb->adf_nullstr.nlst_length.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			If some error occurred in the
**					conversion process.
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**	    NOTE:  The following error codes are internal consistency
**	    checks which will be returned only when the system is
**	    compiled with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-nov-86 (thurston)
**	    Initial creation.
**      19-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	06-jul-89 (jrb)
**	    Copied db_prec field into tmp_dv from adc_dv.
**	13-mar-2001 (somsa01)
**	    Changed adc_defwid and adc_worstwid to i4's.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adc_tmlen(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_dv,
i4		    *adc_defwid,
i4		    *adc_worstwid)
{
    DB_STATUS		db_stat = E_DB_OK;
    i4      		bdt = abs((i4) adc_dv->db_datatype);
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);


    /* Check the consistency of the input arguments */

    /* Check the DB_DATA_VALUE's datatype for correctness */
    if	(mbdt <= 0  ||  mbdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

#ifdef xDEBUG
    /* Check the validity of the DB_DATA_VALUE length passed in. */
    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dv, NULL))
	return (db_stat);
#endif /* xDEBUG */

    /*
    **	 If datatype is non-nullable, call the routine associated with the
    ** supplied datatype to perform this function.  Currently, this is only one
    ** routine for all the RTI known datatypes.  This level of indirection is
    ** provided for the future implementation of user-defined datatypes.
    **
    **   If datatype is nullable, use temp DB_DATA_VALUE to call with, and
    ** possibly adjust worst case width.
    */

    if (adc_dv->db_datatype > 0)	/* non-nullable */
    {
    	db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_tmlen_addr)
			(adf_scb, adc_dv, adc_defwid, adc_worstwid);
    }
    else				/* nullable */
    {
        DB_DATA_VALUE tmp_dv = *adc_dv; 

	tmp_dv.db_datatype = bdt;
	tmp_dv.db_length--;

    	db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_tmlen_addr)
			(adf_scb, &tmp_dv, adc_defwid, adc_worstwid);

        if (    DB_SUCCESS_MACRO(db_stat)
	    &&  *adc_worstwid < adf_scb->adf_nullstr.nlst_length
	   )
	    *adc_worstwid = adf_scb->adf_nullstr.nlst_length;
    }

    return (db_stat);
}


/*{
** Name: adc_tmcvt() - Convert data value to terminal monitor display format.
**
** Description:
**      This routine is used to convert a data value into its terminal monitor
**	display format.  If the value is NULL, then the NULLstring pointed to
**	by the ADF session control block is used.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_dfmt			The multinational date format in use.
**					One of:
**					    DB_US_DFMT
**					    DB_MLTI_DFMT
**					    DB_FIN_DFMT
**					    DB_ISO_DFMT
**					    DB_GERM_DFMT
**					    DB_YMD_DFMT
**					    DB_MDY_DFMT
**					    DB_DMY_DFMT
**	    .adf_mfmt
**		.db_mny_sym		The money sign in use (null terminated).
**		.db_mny_lort		One of:
**					    DB_LEAD_MONY  - leading mny symbol.
**					    DB_TRAIL_MONY - trailing mny symbol.
**					    DB_NONE_MONY  - no mny symbol.
**		.db_mny_prec		Number of digits after decimal point.
**					Only legal values are 0, 1, and 2.
**	    .adf_decimal
**		.db_decspec		Set to TRUE if a decimal marker is
**					specified in .db_decimal.  If FALSE,
**					ADF will assume '.' is the decimal char.
**		.db_decimal		If .db_decspec is TRUE, this is taken
**					to be the decimal character in use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_outarg 		Output formatting information.  This
**					is an ADF_OUTARG structure, and contains
**                                      many pieces of information on how the
**                                      various intrinsic datatypes are supposed
**                                      to be formatted when converted into
**                                      string representation.
**		.ad_c0width		Minimum width of "c" or "char" field.
**		.ad_i1width		Width of "i1" field.
**		.ad_i2width		Width of "i2" field.
**		.ad_i4width		Width of "i4" field.
**		.ad_f4width		Width of "f4" field.
**		.ad_f8width		Width of "f8" field.
**		.ad_i8width		Width of "i8" field.
**    		.ad_f4prec     		Number of decimal places on "f4".
**    		.ad_f8prec     		Number of decimal places on "f8".
**    		.ad_f4style    		"f4" output style ('e', 'f', 'g', 'n').
**    		.ad_f8style    		"f8" output style ('e', 'f', 'g', 'n').
**		.ad_t0width		Min width of "text" or "varchar" field.
**	    .adf_nullstr		Description of NULLstring in use.
**	        .nlst_length		Length of the NULLstring.
**	        .nlst_string		Ptr to the NULLstring.
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					data value to be converted.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision/scale
**	    .db_length			Its length.
**	    .db_data			Ptr to data to be converted.
**	adc_tmdv			Ptr to a DB_DATA_VALUE struct for
**					the result.
**	    .db_length			Length of the buffer.
**	    .db_data			Ptr to buffer for result.
**
** Outputs:
**	adf_scb
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
**	adc_tmdv			Ptr to a DB_DATA_VALUE struct holding
**					the result.
**	    .db_data			The result will be placed in the buffer
**					pointed to by this.  Remember, the data-
**					type is completely ignored here.  Just
**					treat this as a stream of bytes.
**	adc_outlen			Number of characters used for the
**					result.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			If some error occurred in the
**					conversion process.
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**	    E_AD1060_TM_BUF_TOO_SMALL	The buffer given this routine is not
**					large enough to hold the result.
**
**	    NOTE:  The following error codes are internal consistency
**	    checks which will be returned only when the system is
**	    compiled with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-nov-86 (thurston)
**	    Initial creation.
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**      08-may-92 (stevet)
**          Added DB_GERM_DFMT, DB_YMD_DFMT, DB_MDY_DFMT, DB_DMY_DFMT to
**          the adf_scb.adf_dfmt field description.
**	01-Jul-1999 (shero03)
**	    Support II_MONEY_FORMAT=NONE for Sir 9541.
**	13-mar-2001 (somsa01)
**	    Changed defwid and worstwid to i4's.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adc_tmcvt(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_dv,
DB_DATA_VALUE	    *adc_tmdv,
i4		    *adc_outlen)
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			bdt = abs((i4) adc_dv->db_datatype);
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);


    /* Check the consistency of the input DB_DATA_VALUE */

    /* Check the input DB_DATA_VALUEs datatype for correctness */
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

#ifdef xDEBUG
    /* Check the validity of the DB_DATA_VALUE lengths passed in. */
    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dv, NULL))
	return (db_stat);
#endif /* xDEBUG */

    /*
    **	 Call the routine associated with the supplied datatype
    ** to perform this function.  Currently, this is only one
    ** routine for all the RTI known datatypes.  This level of
    ** indirection is provided for the future implementation of
    ** user-defined datatypes.
    */

    if (adc_dv->db_datatype > 0)	/* non-nullable */
    {
        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_tmcvt_addr)
		    (adf_scb, adc_dv, adc_tmdv, adc_outlen);
    }
    else if (ADI_ISNULL_MACRO(adc_dv))	/* nullable with NULL value */
    {
	if (adc_tmdv->db_length < adf_scb->adf_nullstr.nlst_length)
	{
	    db_stat = adu_error(adf_scb, E_AD1060_TM_BUF_TOO_SMALL, 0);
	}
	else
	{
	    i4		defwid;
	    i4		worstwid;

	    db_stat = adc_tmlen(adf_scb, adc_dv, &defwid, &worstwid);
	    if (DB_SUCCESS_MACRO(db_stat))
	    {
		*adc_outlen = max(adf_scb->adf_nullstr.nlst_length, defwid);
		MEmove( adf_scb->adf_nullstr.nlst_length,
			(PTR) adf_scb->adf_nullstr.nlst_string,
			(char)' ',
			*adc_outlen,
			(PTR) adc_tmdv->db_data
		      );
		db_stat = E_DB_OK;
	    }
	}
    }
    else				/* nullable, but not NULL */
    {
	DB_DATA_VALUE tmp_dv = *adc_dv;

	tmp_dv.db_datatype = bdt;
	tmp_dv.db_length--;

        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_tmcvt_addr)
		    (adf_scb, &tmp_dv, adc_tmdv, adc_outlen);
    }

    return (db_stat);
}


/*
** Name: adc_1tmlen_rti() - Provide default and worst-case lengths for display
**		            format of passed in datatype.
**
** Description:
**      This routine is used to retrieve both the default width (smallest
**	display field width) and the worst-case width for the supplied datatype.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_outarg 		Output formatting information.  This
**					is an ADF_OUTARG structure, and contains
**                                      many pieces of information on how the
**                                      various intrinsic datatypes are supposed
**                                      to be formatted when converted into
**                                      string representation.
**		.ad_c0width		Minimum width of "c" or "char" field.
**		.ad_i1width		Width of "i1" field.
**		.ad_i2width		Width of "i2" field.
**		.ad_i4width		Width of "i4" field.
**		.ad_f4width		Width of "f4" field.
**		.ad_f8width		Width of "f8" field.
**		.ad_i8width		Width of "i8" field.
**		.ad_t0width		Min width of "text" or "varchar" field.
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					datatype and length in question.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision/scale
**	    .db_length			Its length.
**
** Outputs:
**	adf_scb
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
**	adc_defwid			The length, in bytes, of the default
**					print field size for the given datatype
**					and length.
**	adc_worstwid			The length, in bytes, of the worst-case
**					print field size for the given datatype
**					and length.  That is, what is the
**					largest possible print field needed for
**					any data value with the given datatype
**					and length.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			If some error occurred in the
**					conversion process.
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-nov-86 (thurston)
**	    Initial creation.
**      14-apr-87 (thurston)
**          Added longtext.
**	27-mar-89 (mikem)
**	    Added support for the logical_key datatype to adc_1tmlen_rti() and
**	    to adc_2tmcvt_rti().
**	06-jul-89 (jrb)
**	    Added decimal support.
**      03-Oct-1992 (fred)
**          Added BIT and VBIT support.
**       6-Apr-1993 (fred)
**          Added byte string types.
**	1-apr-1993 (robf)
**	    Updated SEC_TYPE worst width case.
**	23-dec-03 (inkdo01)
**	    Add display support for bigint (i8).
**	16-jun-2005 (gups01)
**	    Fixed output results for nchar and nvarchar types.
**	17-jul-2006 (gupsh01)
**	    Added support for new ANSI datetime types.
*/

DB_STATUS
adc_1tmlen_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_dv,
i4                  *adc_defwid,
i4                  *adc_worstwid)
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			len = adc_dv->db_length;


    switch (adc_dv->db_datatype)
    {
      case DB_BOO_TYPE:
        *adc_defwid = 5;
        *adc_worstwid = 5;
        break;

      case DB_INT_TYPE:
	switch (len)
	{
	  case 1:
	    *adc_defwid   = adf_scb->adf_outarg.ad_i1width;
	    *adc_worstwid = 4;	    /* -128 */
	    break;

	  case 2:
	    *adc_defwid   = adf_scb->adf_outarg.ad_i2width;
	    *adc_worstwid = 6;	    /* -32768 */
	    break;

	  case 4:
	    *adc_defwid   = adf_scb->adf_outarg.ad_i4width;
	    *adc_worstwid = 11;	    /* -2147483648 */
	    break;

	  case 8:
	    *adc_defwid   = adf_scb->adf_outarg.ad_i8width;
	    *adc_worstwid = 20;	    /* -9223372036854775808 */
	    break;

	  default:
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    break;
	}
	break;

      case DB_DEC_TYPE:
	{
	    i4		pr = DB_P_DECODE_MACRO(adc_dv->db_prec);
	    i4		sc = DB_S_DECODE_MACRO(adc_dv->db_prec);
	    
	    *adc_defwid   =
	    *adc_worstwid = AD_PS_TO_PRN_MACRO(pr, sc);
	}
	break;
	
      case DB_FLT_TYPE:
	switch (len)
	{
	  case 4:
	    *adc_defwid = *adc_worstwid = adf_scb->adf_outarg.ad_f4width;
	    break;

	  case 8:
	    *adc_defwid = *adc_worstwid = adf_scb->adf_outarg.ad_f8width;
	    break;

	  default:
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    break;
	}
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_BYTE_TYPE:
      case DB_NBR_TYPE:
	*adc_defwid   = max(len, adf_scb->adf_outarg.ad_c0width);
	/* worst case is 4 times longer in case of \000 format on output */
	*adc_worstwid = max((len * 4), *adc_defwid);
	break;

      case DB_NCHR_TYPE:
	*adc_defwid   = max(len, adf_scb->adf_outarg.ad_c0width);
	*adc_worstwid = max(((len/(DB_CNTSIZE)) * 3 * 4), *adc_defwid);
	break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
	*adc_defwid   = max((len - DB_CNTSIZE),
			    adf_scb->adf_outarg.ad_t0width);

	/* worst case is 4 times longer in case of \000 format on output */
	*adc_worstwid = max(((len - DB_CNTSIZE) * 4),
			    *adc_defwid);
	break;

      case DB_NVCHR_TYPE:
	*adc_defwid   = max(len - DB_CNTSIZE, adf_scb->adf_outarg.ad_c0width);
	/* worst case is 4 times longer in case of \000 format on output */
	*adc_worstwid = max((((len - DB_CNTSIZE)/DB_CNTSIZE) * 3) * 4 , 
				*adc_defwid);
	break;

      case DB_DTE_TYPE:
	*adc_defwid = *adc_worstwid = AD_1DTE_OUTLENGTH;
	break;

      case DB_ADTE_TYPE:
	*adc_defwid = *adc_worstwid = AD_2ADTE_OUTLENGTH;
	break;

      case DB_TMWO_TYPE:
	*adc_defwid = *adc_worstwid = AD_3TMWO_OUTLENGTH;
	break;

      case DB_TMW_TYPE:
	*adc_defwid = *adc_worstwid = AD_4TMW_OUTLENGTH;
	break;

      case DB_TME_TYPE:
	*adc_defwid = *adc_worstwid = AD_5TME_OUTLENGTH;
	break;

      case DB_TSWO_TYPE:
	*adc_defwid = *adc_worstwid = AD_6TSWO_OUTLENGTH;
	break;

      case DB_TSW_TYPE:
	*adc_defwid = *adc_worstwid = AD_7TSW_OUTLENGTH;
	break;

      case DB_TSTMP_TYPE:
	*adc_defwid = *adc_worstwid = AD_8TSTMP_OUTLENGTH;
	break;

      case DB_INYM_TYPE:
	*adc_defwid = *adc_worstwid = AD_9INYM_OUTLENGTH;
	break;

      case DB_INDS_TYPE:
	*adc_defwid = *adc_worstwid = AD_10INDS_OUTLENGTH;
	break;

      case DB_MNY_TYPE:
	*adc_defwid = *adc_worstwid = AD_5MNY_OUTLENGTH;
	break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	*adc_defwid   = max((len * 4), adf_scb->adf_outarg.ad_c0width);
	/* worst case is 4 times longer in case of \000 format on output */
	*adc_worstwid = max((len * 4), *adc_defwid);
	break;

      case DB_BIT_TYPE:
	{
	    i4 bit_len;

	    bit_len = adc_dv->db_length * BITSPERBYTE;
	    if (adc_dv->db_prec)
		bit_len = bit_len - BITSPERBYTE + adc_dv->db_prec;

	    *adc_defwid = *adc_worstwid =
		max(bit_len, adf_scb->adf_outarg.ad_c0width);
	    break;
	}

      case DB_VBIT_TYPE:
	{
	    i4 bit_len;

	    bit_len = (adc_dv->db_length - DB_BCNTSIZE) * BITSPERBYTE;
	    if (adc_dv->db_prec)
		bit_len = bit_len - BITSPERBYTE + adc_dv->db_prec;

	    *adc_defwid = *adc_worstwid =
		max(bit_len, adf_scb->adf_outarg.ad_c0width);
	    break;
	}

      default:
	db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	break;
    }

    return (db_stat);
}


/*
** Name: adc_2tmcvt_rti() - Convert data value to terminal monitor display
**			    format.
**
** Description:
**      This routine is used to convert a data value into its terminal monitor
**	display format.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_dfmt			The multinational date format in use.
**					One of:
**					    DB_US_DFMT
**					    DB_MLTI_DFMT
**					    DB_FIN_DFMT
**					    DB_ISO_DFMT
**	    .adf_mfmt
**		.db_mny_sym		The money sign in use (null terminated).
**		.db_mny_lort		One of:
**					    DB_LEAD_MONY  - leading mny symbol.
**					    DB_TRAIL_MONY - trailing mny symbol.
**		.db_mny_prec		Number of digits after decimal point.
**					Only legal values are 0, 1, and 2.
**	    .adf_decimal
**		.db_decspec		Set to TRUE if a decimal marker is
**					specified in .db_decimal.  If FALSE,
**					ADF will assume '.' is the decimal char.
**		.db_decimal		If .db_decspec is TRUE, this is taken
**					to be the decimal character in use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_outarg 		Output formatting information.  This
**					is an ADF_OUTARG structure, and contains
**                                      many pieces of information on how the
**                                      various intrinsic datatypes are supposed
**                                      to be formatted when converted into
**                                      string representation.
**		.ad_c0width		Minimum width of "c" or "char" field.
**		.ad_i1width		Width of "i1" field.
**		.ad_i2width		Width of "i2" field.
**		.ad_i4width		Width of "i4" field.
**		.ad_f4width		Width of "f4" field.
**		.ad_f8width		Width of "f8" field.
**		.ad_i8width		Width of "i8" field.
**    		.ad_f4prec     		Number of decimal places on "f4".
**    		.ad_f8prec     		Number of decimal places on "f8".
**    		.ad_f4style    		"f4" output style ('e', 'f', 'g', 'n').
**    		.ad_f8style    		"f8" output style ('e', 'f', 'g', 'n').
**		.ad_t0width		Min width of "text" or "varchar" field.
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					data value to be converted.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision/scale
**	    .db_length			Its length.
**	    .db_data			Ptr to data to be converted.
**	adc_tmdv			Ptr to a DB_DATA_VALUE struct for
**					the result.
**	    .db_length			Length of the buffer.
**	    .db_data			Ptr to buffer for result.
**
** Outputs:
**	adf_scb
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
**	adc_tmdv			Ptr to a DB_DATA_VALUE struct holding
**					the result.
**	    .db_data			The result will be placed in the buffer
**					pointed to by this.  Remember, the data-
**					type is completely ignored here.  Just
**					treat this as a stream of bytes.
**	adc_outlen			Number of characters used for the
**					result.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			If some error occurred in the
**					conversion process.
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**	    E_AD1060_TM_BUF_TOO_SMALL	The buffer given this routine is not
**					large enough to hold the result.
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-nov-86 (thurston)
**	    Initial creation.
**	9-jan-87 (daved)
**	    added case for DB_CHA_TYPE and made DB_CHR_TYPE look for null.
**      14-apr-87 (thurston)
**          Added longtext.  Also fixed bug in displaying dates.
**      07-jul-87 (thurston)
**          Changed an erroneous F4ASSIGN_MACRO to the correct F8ASSIGN_MACRO.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	03-may-88 (jrb)
**	    Removed DB_ANYTYPE typedef usage and replaced with proper casting.
**	31-may-89 (jrb)
**	    Added a cast to f8 on f4tmp in CVfa call although it still worked
**	    without this cast (since floats get casted to double for function
**	    calls in C).
**	20-jun-88 (jrb)
**	    Mods for Kanji support
**	06-jul-89 (jrb)
**	    Added decimal support.
**	10-feb-90 (dkh)
**	    Put in changes to reduce calls on STlength to speed up terminal
**	    monitor.
**	15-feb-90 (dkh)
**	    Changed handling of non-printable characters adc_2tmcvt_rti() to
**	    make it go slightly faster.
**      03-oct-1992 (fred)
**	    Added DB_[V]BIT_TYPE support.
**       6-Apr-1993 (fred)
**          Added byte string types.
**	01-apr-1993 (robf)
**	    Fixed security label handling - add BREAK after conversion.
**	25-nov-2003 (gupsh01)
**	    Fixed the terminal monitor to coerce the nvarchar/ nchar
**	    data to Unicode form.
**	23-dec-03 (inkdo01)
**	    Add display support for bigint (i8).
**	29-jun-2005 (thaju02)
**	    For nvchr, use length of coerced data to determine endp. 
**	    (B114780)
**	04-apr-2006 (gupsh01)
**	    Fixed the Unicode conversions by allocating a buffer for
**	    them. The conversion gave incorrect results when it has 
**	    control characters like '\n'.
**	17-jul-2006 (gupsh01)
**	    Added support for new ANSI datetime types.
**	13-sep-2006 (gupsh01)
**	    Zero fill the result buffer before copying in the date value
**	    preceding conversion to string.
**	28-Nov-2007 (kiria01) b119532
**	    Don't reset db_stat as it will lose evidence of unicode
**	    mapping errors and other potential error conditions that
**	    need to be seen. Also treat NCHAR the same as NVARCHAR for
**          output as otherwise control characters will pass through.
**      05-Mar-2008 (coomi01) : b119968
**          Check f4 case for boundary condition at +/- FLT_MAX
**          here we must make a slight adjustment to ensure rounding
**          does not cause overflow.
**	15-Nov-2009 (kschendel) SIR 122890
**	    Don't lose error status from a failed NVCHAR coercion.
**	13-Jul-2010 (coomi01) b124070
**          Do not use CM macros for Byte data.
*/

DB_STATUS
adc_2tmcvt_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_dv,
DB_DATA_VALUE	    *adc_tmdv,
i4		    *adc_outlen)
{
    DB_STATUS		db_stat = E_DB_OK;
    register i4	length = adc_dv->db_length;
    register u_char	*f = (u_char *)adc_dv->db_data;
    register char	*t = (char *)adc_tmdv->db_data;
    register u_char	*p;
    register i4	i;
    register i4	reslen;
    char		numbuf[35];
    i2			res_width;
    DB_DATA_VALUE	local_dv;
    char		unicode_buffer[4096];	
    i2			unibuf_alloc = 0;
    STATUS		stat;
    bool                overflow = FALSE;

    switch (adc_dv->db_datatype)
    {
      case DB_BOO_TYPE:
        MEcopy(((DB_ANYTYPE *)adc_dv->db_data)->db_booltype == DB_FALSE ?
	       "FALSE" : "TRUE ", 5, t);
        *adc_outlen = 5;
        break;

      case DB_INT_TYPE:
	switch (length)
	{
	  case 1:
	    {
		i4	    i4tmp;

		i4tmp = (i4) *(i1 *)f;
		CVla(I1_CHECK_MACRO(i4tmp), numbuf);
		reslen = STlength(numbuf);
		*adc_outlen = max(reslen, adf_scb->adf_outarg.ad_i1width);
	    }
	    break;

	  case 2:
	    {
		i2	    i2tmp;

		I2ASSIGN_MACRO(*(i2 *)f, i2tmp);
		CVla((i4) i2tmp, numbuf);
		reslen = STlength(numbuf);
		*adc_outlen = max(reslen, adf_scb->adf_outarg.ad_i2width);
	    }
	    break;

	  case 4:
	    {
		i4	    i4tmp;

		I4ASSIGN_MACRO(*(i4 *)f, i4tmp);
		CVla(i4tmp, numbuf);
		reslen = STlength(numbuf);
		*adc_outlen = max(reslen, adf_scb->adf_outarg.ad_i4width);
	    }
	    break;

	  case 8:
	    {
		longlong    i8tmp;

		MEcopy((PTR)f, sizeof(i8tmp), (PTR)(&i8tmp));
		CVla8(i8tmp, numbuf);
		reslen = STlength(numbuf);
		*adc_outlen = max(reslen, adf_scb->adf_outarg.ad_i8width);
	    }
	    break;

	  default:
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    break;
	}

	if (db_stat == E_DB_OK)
	    ad0_printfatt(numbuf,reslen, *adc_outlen,(char *)adc_tmdv->db_data);
	break;

      case  DB_DEC_TYPE:
	{
	    i4		pr = DB_P_DECODE_MACRO(adc_dv->db_prec);
	    i4		sc = DB_S_DECODE_MACRO(adc_dv->db_prec);
	    char	decimal = (adf_scb->adf_decimal.db_decspec
					? (char) adf_scb->adf_decimal.db_decimal
					: '.'
				  );

	    /* now convert to ascii: use formula from tm_len for length, get
	    ** scale # of digits after decimal point, no options (so we get
	    ** a right-justified output), and put resulting length in
	    ** adc_outlen
	    */
	    if (CVpka((PTR)f, pr, sc, decimal, AD_PS_TO_PRN_MACRO(pr, sc),
		    sc, 0, t, adc_outlen)
		== CV_OVERFLOW)
	    {
		/* this should never happen */
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	}
	break;
	
      case  DB_FLT_TYPE:
	{
	    char	buf[ADI_OUTMXFIELD];
	    char	decimal = (adf_scb->adf_decimal.db_decspec
					? (char) adf_scb->adf_decimal.db_decimal
					: '.'
				  );

	    switch (length)
	    {
	      case 4:
		{
		    f4	    f4tmp;
		    f8      f8tmp;

		    F4ASSIGN_MACRO(*(f4 *)f, f4tmp); 
		    f8tmp = (f8)f4tmp;

		    /*  BUG : b119968
		    **
		    **  CVfa is a utility routine for both f8 and f4 floats.
		    **  In the f4 case, the value is converted to f8 prior 
		    **  to call (as here).
		    **
		    **  This works well except for the boundary case, where 
		    **  the f4 is FLT_MAX. In this case, an effective 
		    **  rounding up of the f8 that occurs as we convert to 
		    **  a string (eventually through fcvt) "overflows" the 
		    **  original f4 value.
		    **
		    **  We bump down the last sig bit of the f4, and then
		    **  we have no problems.
		    */
		    if ( f8tmp == FLT_MAX )
		    {
			f8tmp = FLT_MAX_ROUND;
		    }
                    else if ( f8tmp == -FLT_MAX ) 
                    {
			f8tmp = -FLT_MAX_ROUND;
                    }
		    CVfa(f8tmp, adf_scb->adf_outarg.ad_f4width,
			    adf_scb->adf_outarg.ad_f4prec,
			    adf_scb->adf_outarg.ad_f4style,
			    decimal, buf, &res_width);
		    *adc_outlen = adf_scb->adf_outarg.ad_f4width;
		}
		break;

	      case 8:
		{
		    f8		    f8tmp;

		    F8ASSIGN_MACRO(*(f8 *)f, f8tmp); 
		    CVfa(f8tmp, adf_scb->adf_outarg.ad_f8width,
			    adf_scb->adf_outarg.ad_f8prec,
			    adf_scb->adf_outarg.ad_f8style,
			    decimal, buf, &res_width);
		    *adc_outlen = adf_scb->adf_outarg.ad_f8width;
		}
		break;

	      default:
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
		break;
	    }

	    if (db_stat == E_DB_OK)
	    {
		ad0_printfatt(buf, (i4)res_width, *adc_outlen,
			(char *)adc_tmdv->db_data);
	    }
	}
	break;

      case  DB_MNY_TYPE:
	{
	    f8		    f8tmp;
	    char	    buf[AD_5MNY_OUTLENGTH + 1 + 4];
	    register char   *b = buf;
	    char	    decimal = (adf_scb->adf_decimal.db_decspec
					? (char) adf_scb->adf_decimal.db_decimal
					: '.'
				      );
	    char	    *mny_sym = &adf_scb->adf_mfmt.db_mny_sym[0];
	    i4 		    mny_lort =  adf_scb->adf_mfmt.db_mny_lort;
	    i4		    mny_prec =  adf_scb->adf_mfmt.db_mny_prec;
	    i4		    lsym = STlength(mny_sym);
	    i4		    used;

#ifdef xDEBUG
	    /* make sure mny_lort is either "leading", "trailing", or "none" */
	    if (mny_lort != DB_LEAD_MONY  &&  mny_lort != DB_TRAIL_MONY &&
	    	mny_lort != DB_NONE_MONY)
	    {
		db_stat = adu_error(adf_scb, E_AD1003_BAD_MONY_LORT, 2,
				    (i4) sizeof(mny_lort),
				    (i4 *) &mny_lort);
		break;
	    }

	    /* make sure mny_prec is either 0, 1, or 2 */
	    if (mny_prec < 0  ||  mny_prec > 2)
	    {
		db_stat = adu_error(adf_scb, E_AD1004_BAD_MONY_PREC, 2,
				    (i4) sizeof(mny_prec),
				    (i4 *) &mny_prec);
		break;
	    }
#endif /* xDEBUG */

	    if (mny_lort == DB_LEAD_MONY)
	    {
		STcopy(mny_sym, b);
		b += lsym;
		used = lsym;
	    }

	    F8ASSIGN_MACRO(((AD_MONEYNTRNL*)adc_dv->db_data)->mny_cents, f8tmp);
	    f8tmp /= 100;	/* because money is stored as cents */
	    CVfa(f8tmp, (i4)(AD_5MNY_OUTLENGTH - lsym), mny_prec, 'f',
			decimal, b, &res_width);
	    used += res_width;

	    if (mny_lort == DB_TRAIL_MONY)
	    {
		if (res_width + lsym <= AD_5MNY_OUTLENGTH)
		{
		    b += STlength(b);
		    STcopy(mny_sym, b);
		}
		b = &buf[0];
	    }
	    else
	    {
		if (used > AD_5MNY_OUTLENGTH)
		{
		    /* use &buf[lsym] instead of &buf[0] */
		    b = &buf[lsym];
		}
		else
		{
		    b = &buf[0];
		}
	    }

	    *adc_outlen = AD_5MNY_OUTLENGTH;
	    ad0_printfatt(buf, (i4) STlength(buf), *adc_outlen,
		(char *)adc_tmdv->db_data);
	}
	break;

      case DB_CHR_TYPE:
	{
	    u_char	*endp = (u_char *)adc_dv->db_data + length;
	    	    
	    p = (u_char *)adc_dv->db_data;

	    while (*p != '\0'  &&  p < endp)
		CMcpyinc(p, t);

	    while (p++ < endp)
		*t++ = ' ';
	    
	    if ((i = adf_scb->adf_outarg.ad_c0width - length) > 0)
		while (i--)
		    *t++ = ' ';

	    *adc_outlen = t - (char *)adc_tmdv->db_data;
	}
	break;

      case DB_BYTE_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NBR_TYPE:
	{
	    i2	    i2tmp;
	    u_char  *endp;
	    u_char  *endt;

	    if ( DB_BYTE_TYPE == adc_dv->db_datatype )
		{
			p = f;
			i = length;
			length = max(length, adf_scb->adf_outarg.ad_c0width);
		}
	    else
		{
			p = f + DB_CNTSIZE;
			I2ASSIGN_MACRO(*(i2 *)f, i2tmp); 
			i = i2tmp;
			length = max(length-DB_CNTSIZE, adf_scb->adf_outarg.ad_t0width);
		}

	    /* Calculate pointer to end of output, tmdv, buffer */
	    endt = adc_tmdv->db_data + adc_tmdv->db_length;
	    endp = p + i;

	    /* Careful, t might progress towards 
	    ** end of buffer more quickly than p
	    */
	    overflow = FALSE;
	    while (p < endp && !overflow)
	    {
		/* If input is less than ASCII(Space)
		** or has top bit set, we'll escape it
		*/
		if ((*p < 0x20)||(0x7F < *p))
		{
		    switch (*p)
		    {
		      case (u_char)'\n':
			  if (endt-(u_char *)t > 1)
			  {
			      t[0] = '\\';
			      t[1] = 'n';
			      t += 2;
			  }
			  else
			  {
			      /* Not enough space, 
			      ** Don't overrun the buffer
			      ** But flag the problem
			      */
			      overflow=TRUE;
			  }
			break;

		      case (u_char)'\t':
			  if (endt-(u_char *)t > 1)
			  {
			      t[0] = '\\';
			      t[1] = 't';
			      t += 2;
			  }
			  else
			  {
			      overflow=TRUE;
			  }
			break;

		      case (u_char)'\b':
			  if (endt-(u_char *)t > 1)
			  {
			      t[0] = '\\';
			      t[1] = 'b';
			      t += 2;
			  }
			  else
			  {
			      overflow=TRUE;
			  }
			break;

		      case (u_char)'\r':
			  if (endt-(u_char *)t > 1)
			  {
			      t[0] = '\\';
			      t[1] = 'r';
			      t += 2;
			  }
			  else
			  {
			      overflow=TRUE;
			  }
			break;

		      case (u_char)'\f':
			  if (endt-(u_char *)t > 1)
			  {
			      t[0] = '\\';
			      t[1] = 'f';
			      t += 2;
			  }
			  else
			  {
			      overflow=TRUE;
			  }
			break;

		      default:
		      {
			u_char	curval;
			int	curpos;

			if (endt-(u_char *)t > 3)
			{
			    /* convert the character to \ddd, where
			    ** the d's are octal digits
			    */
			    t[0] = '\\';
			    curval = *p;
			    /* while there are more octal digits */
			    for (curpos = 3; curpos > 0; curpos--)
			    {
				/* put the digit in the output string */
				t[curpos] = (u_char)'0' + (curval % 8);
				/* remove the digit from the current value */
				curval /= 8;
			    }
			    t += 4;
			}
			else
			{
			    overflow=TRUE;
			}

			break;
		      }
		    }
		}
		else if (*p == (u_char)'\\')
		{
		    if (endt-(u_char *)t > 1)
		    {
			t[0] = '\\';
			t[1] = '\\';
			t += 2;
		    }
		    else
		    {
			overflow=TRUE;
		    }
		}
		else
		{
		     /* 'Ordinary' bytes just get copied straight over 
		      */
		    if (endt-(u_char *)t > 0)
		    {
			*t = *p;
			t++;
		    }
		    else
		    {
			overflow=TRUE;
		    }		    
		}

		p++;
	    }

	    if (overflow)
	    {
		/* this should never happen */
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }

	    length -= t - (char *)adc_tmdv->db_data;
	    
	    while (length-- > 0)
		*t++ = (u_char)' ';

	    *adc_outlen = t - (char *)adc_tmdv->db_data;
	}
	break;

      case DB_NCHR_TYPE:
      case DB_NVCHR_TYPE:
	{
	    /* Treat NCHAR the same as NVARCHAR
	    ** Coerce both to VARCHAR for outputting
	    */
	    local_dv.db_length = adc_tmdv->db_length;
	    if (adc_tmdv->db_length > (4096 - 1))
	    {
		local_dv.db_data = (char *)MEreqmem (0, 
			adc_tmdv->db_length + 2, TRUE, &stat);
		unibuf_alloc = 1;
	    }
	    else
		local_dv.db_data = unicode_buffer;
	    local_dv.db_prec = 0;
	    local_dv.db_collID = DB_UNSET_COLL;
	    local_dv.db_datatype = DB_VCH_TYPE;

	    db_stat = adu_nvchr_coerce(adf_scb, adc_dv, &local_dv);
	    if (db_stat != E_DB_OK)
	    {
		/* Set converted length to zero */
		res_width = 0;
		I2ASSIGN_MACRO(res_width, *(i2 *)local_dv.db_data);
	    }
	}
	/* no break, fall through here */
      case DB_CHA_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
	{
	    i2	    i2tmp;
	    u_char  *endp;

	    switch (adc_dv->db_datatype)
	    {
	    case DB_CHA_TYPE:
	    case DB_BYTE_TYPE:
            case DB_NBR_TYPE:
		p = f;
		i = length;
		length = max(length, adf_scb->adf_outarg.ad_c0width);
		break;
	    case DB_NVCHR_TYPE:
		/* For NVARCHAR we need to adjust the input length */
		length -= DB_CNTSIZE;
		/* FALLTHROUGH */
	    case DB_NCHR_TYPE:
		/* For Unicode this has been translated in VARCHAR above */
		p = (u_char *)local_dv.db_data + DB_CNTSIZE;
		I2ASSIGN_MACRO(*(i2 *)local_dv.db_data, i2tmp);
		i = i2tmp;
		length = max(length, adf_scb->adf_outarg.ad_t0width);
		break;
	    default:
		p = f + DB_CNTSIZE;
		I2ASSIGN_MACRO(*(i2 *)f, i2tmp); 
		i = i2tmp;
		length = max(length-DB_CNTSIZE, adf_scb->adf_outarg.ad_t0width);
	    }

	    endp = p + i;

	    while (p < endp)
	    {
		if (CMcntrl((char *)p))
		{
		    switch (*p)
		    {
		      case (u_char)'\n':
			STcopy("\\n", (char *)t);
			t += 2;
			break;

		      case (u_char)'\t':
			STcopy("\\t", (char *)t);
			t += 2;
			break;

		      case (u_char)'\b':
			STcopy("\\b", (char *)t);
			t += 2;
			break;

		      case (u_char)'\r':
			STcopy("\\r", (char *)t);
			t += 2;
			break;

		      case (u_char)'\f':
			STcopy("\\f", (char *)t);
			t += 2;
			break;

		      default:
		      {
			u_char	curval;
			int	curpos;

			/* convert the character to \ddd, where
			** the d's are octal digits
			*/
			STcopy("\\", (char *)t);
			curval = *p;
			/* while there are more octal digits */
			for (curpos = 3; curpos > 0; curpos--)
			{
			    /* put the digit in the output string */
			    t[curpos] = (u_char)'0' + (curval % 8);
			    /* remove the digit from the current value */
			    curval /= 8;
			}
			t += 4;
			break;
		      }
		    }
		}
		else if (*p == (u_char)'\\')
		{
		    STcopy("\\\\", (char *)t);
		    t += 2;
		}
		else
		{
		     CMcpychar(p, t);
		     CMnext(t);
		}

		CMnext(p);
	    }
	    length -= t - (char *)adc_tmdv->db_data;
	    
	    while (length-- > 0)
		*t++ = (u_char)' ';

	    *adc_outlen = t - (char *)adc_tmdv->db_data;
	}
	break;

      case DB_DTE_TYPE:
      case DB_ADTE_TYPE:
      case DB_TMWO_TYPE:
      case DB_TMW_TYPE:
      case DB_TME_TYPE:
      case DB_TSWO_TYPE:
      case DB_TSW_TYPE:
      case DB_TSTMP_TYPE:
      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
	{
    	  DB_DATA_VALUE   	str_dv;
    	  i2			datelen;

	  switch (adc_dv->db_datatype)
	  {
		case DB_DTE_TYPE:
        	  datelen = AD_1DTE_OUTLENGTH;
		  break;
      		case DB_ADTE_TYPE:
        	  datelen = AD_2ADTE_OUTLENGTH;
		  break;
      		case DB_TMWO_TYPE:
        	  datelen = AD_3TMWO_OUTLENGTH;
		  break;
      		case DB_TMW_TYPE:
        	  datelen = AD_4TMW_OUTLENGTH;
		  break;
      		case DB_TME_TYPE:
        	  datelen = AD_5TME_OUTLENGTH;
		  break;
      		case DB_TSWO_TYPE:
        	  datelen = AD_6TSWO_OUTLENGTH;
		  break;
      		case DB_TSW_TYPE:
        	  datelen = AD_7TSW_OUTLENGTH;
		  break;
      		case DB_TSTMP_TYPE:
        	  datelen = AD_8TSTMP_OUTLENGTH;
		  break;
      		case DB_INYM_TYPE:
        	  datelen = AD_9INYM_OUTLENGTH;
		  break;
      		case DB_INDS_TYPE:
        	  datelen = AD_10INDS_OUTLENGTH;
		  break;
	  }

	  if (adc_tmdv->db_length < datelen)
	  {
		db_stat = adu_error(adf_scb, E_AD1060_TM_BUF_TOO_SMALL, 0);
		break;
	  }

	  str_dv.db_data = adc_tmdv->db_data;
	  str_dv.db_datatype = DB_CHA_TYPE;
	  str_dv.db_prec = 0;
	  str_dv.db_collID = DB_UNSET_COLL;
	  *adc_outlen = str_dv.db_length = datelen;
	  MEfill (datelen, NULLCHAR, (PTR) (str_dv.db_data));
	  db_stat = adu_6datetostr(adf_scb, adc_dv, &str_dv);
	}
	break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	{
	    u_char	*endp;
	    u_char	curval;
	    int		curpos;

	    p = f;
	    i = length;

	    length = max(length, adf_scb->adf_outarg.ad_c0width);

	    endp = p + i;

	    while (p < endp)
	    {
		/* convert the character to \ddd, where
		** the d's are octal digits
		*/
		STcopy("\\", (char *)t);
		curval = *p;
		/* while there are more octal digits */
		for (curpos = 3; curpos > 0; curpos--)
		{
		    /* put the digit in the output string */
		    t[curpos] = (u_char)'0' + (curval % 8);
		    /* remove the digit from the current value */
		    curval /= 8;
		}
		t += 4;
		p++;
	    }
	    length -= t - (char *)adc_tmdv->db_data;


	    *adc_outlen = t - (char *)adc_tmdv->db_data;
	    db_stat = E_DB_OK;

	}

	break;

      case DB_BIT_TYPE:
      case DB_VBIT_TYPE:
      {
	DB_DATA_VALUE	fake_dv;

	STRUCT_ASSIGN_MACRO(*adc_tmdv, fake_dv);
	fake_dv.db_prec = 0;
	fake_dv.db_datatype = DB_CHA_TYPE;
	length = adc_dv->db_length * BITSPERBYTE;
	if (adc_dv->db_prec)
	    length = length - BITSPERBYTE + adc_dv->db_prec;
	if (adc_dv->db_datatype == DB_VBIT_TYPE)
	    length -= (DB_BCNTSIZE * BITSPERBYTE);

	if (fake_dv.db_length < length)
	{
	    db_stat = adu_error(adf_scb, E_AD1060_TM_BUF_TOO_SMALL, 0);
	    break;
	}

	fake_dv.db_length = *adc_outlen =
		max(length, adf_scb->adf_outarg.ad_c0width);
	adu_bit2str(adf_scb, adc_dv, &fake_dv);
	break;
      }

      default:
	db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	break;
    }
    if ( unibuf_alloc == 1)
	MEfree (local_dv.db_data);

    return (db_stat);
}


/*
** Name: ad0_printfatt() - Copy formatted numeric value into TM output buffer.
**
** Description:
**	Formatted numeric attribute string 'value' is copied into field of
**	width 'width' in TM output buffer 'buf'.  This routine is important
**	since it right justifies the numeric string 'value' into 'buf'.
**
**	NOTE!!!  It is assumed that:  width >= STlength(value)
**
** Inputs:
**	value				    The formatted value, null
**					    terminated.
**	vallen				    Length, in bytes, of 'value'.
**	width				    Number of bytes of 'buf' to use.
**
** Outputs:
**	buf				    The formatted value is placed into
**					    the first 'width' bytes of 'buf',
**					    and right justified.
**
**	Returns:
**	    none -- a VOID function
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-nov-86 (thurston)
**	    Initial creation.
**	10-feb-90 (dkh)
**	    Changed interface to this routine to also pass the length of the
**	    value being printed.  This eliminates the need to call STlength()
**	    since most callers already know the length of the value being
**	    passed in.
*/

static VOID
ad0_printfatt(value, vallen, width, buf)
char		*value;
i4		vallen;
i4		width;
char		*buf;
{
	register char	*v = value;
	register char	*b = buf;
	register i4	w  = width;
 	register i4	l  = vallen;

 
	/* right justify the output field */
 
	for (w -= l; w > 0; w--)
		*b++ = ' ';
 
	while(l--)
		*b++ = *v++;
}
