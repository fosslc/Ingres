/*
** Copyright (c) 2004,2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ade.h>	    /* Only needed for ADE_LEN_UNKNOWN */
#include    <adudate.h>
#include    <adumoney.h>
#include    <ulf.h>
#include    <adfint.h>

/*
**  Global strings, defined in adgstartup.c.  These are server-wide strings
**  used for comparison when testing string and logical key types for minimum
**  or maximum values.  31-jul-90 (linda) for RMS Gateway.
*/
GLOBALREF   u_char    *Chr_min;
GLOBALREF   u_char    *Chr_max;
GLOBALREF   u_char    *Cha_min;
GLOBALREF   u_char    *Cha_max;
GLOBALREF   u_char    *Txt_max;
GLOBALREF   u_char    *Vch_min;
GLOBALREF   u_char    *Vch_max;
GLOBALREF   u_char    *Lke_min;
GLOBALREF   u_char    *Lke_max;
GLOBALREF   u_char    *Bit_min;
GLOBALREF   u_char    *Bit_max;

    /*
    ** Note to maintainers of the _min & _max stuff.  The byte 
    ** datatype is currently considered equivalent to CHA & VCH.
    ** Changes to these values should be coordinated.
    */

/**
**
**  Name: ADCISMINMAX.C - Compare with min or max value for given datatype.
**
**  Description:
**	This file contains all of the routines necessary to perform the
**	external ADF call "adc_isminmax()" which will tell whether the data
**	value passed in is the "minimum", the "maximum", or a null data value
**	for the given datatype.
**
**      This file defines:
**
**          adc_isminmax() - Tell whether value is min, max or null value for
**			     given datatype.
**	    adc_1isminmax_rti() - Actually compares against min, max or null
**				  value for the given RTI datatype as opposed
**				  to a user defined datatype.
**
**
**  History:
**	30-jul-90 (linda)
**	    Written for RMS Gateway project.
**	22-sep-1992 (fred)
**	    Added BIT types.
**      22-dec-1992 (stevet)
**          Added function prototype.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**       1-Jul-1993 (fred)
**          Removed (bool) cast from second argument to adc_lenchk().
**          Argument is now a nat.  See adf.h for details.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-jun-2001 (somsa01)
**	    Changed type of min / max arrays.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL & add missing case code for ANSI dates.
**      14-aug-2009 (joea)
**          Change case for DB_BOO_TYPE in adc_1isminmax_rti.
**      4-dec-2009 (stial01)
**          Added NCHR,NVCHR
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**/


/*{
** Name: adc_isminmax() - Tell whether value is min, max or null value for given
**			  datatype.
**
** Description:
**	This function is the external ADF call "adc_isminmax()" which will tell
**	whether the input value is the "minimum", "maximum", or a null data
**	value for the given datatype.
**
**	For any input data value, this routine returns one of four return
**	codes:  E_AD0117_IS_MINIMUM, E_AD0118_IS_MAXIMUM, E_AD0119_IS_NULL, or
**	E_DB_OK, indicating whether the value is the minimum value for the
**	datatype, the maximum value for the datatype, the null value for the
**	datatype, or not one of those cases.  This information is used by
**	gateways to evaluate keys for positioning and scan limits.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adc_dv				Pointer to DB_DATA_VALUE to evaluate.
**		.db_datatype		Its datatype.
**		.db_length		Its length.
**		.db_data		The data value.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an error occurs,
**					the following fields will be set.
**					NOTE: if .ad_ebuflen = 0 or .ad_errmsgp
**					= NULL, no error message will be
**					formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR, this
**					field is set to the corresponding user
**					error which will either map to an ADF
**					error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
**      Returns:
**	    The following DB_STATUS codes may be returned: E_DB_OK, E_DB_WARN,
**	    E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL.
**
**	    A status of E_DB_OK indicates that the value is not a minimum,
**	    maximum or null value for its datatype.
**
**	    The following codes are returned along with a status of E_DB_WARN:
**
**	    E_AD0117_IS_MINIMUM		Minimum value for type and length given
**	    E_AD0118_IS_MAXIMUM		Maximum value for type and length given
**	    E_AD0119_IS_NULL		Null value for type and length given
**
**	    The following codes are returned along with a status of E_DB_ERROR:
**
**          E_AD2004_BAD_DTID           Datatype unknown to ADF.
**          E_AD2005_BAD_DTLEN          Length is illegal for this datatype.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	30-jul-90 (linda)
**	    Written.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_isminmax(
ADF_CB         *adf_scb,
DB_DATA_VALUE  *adc_dv)
# else
DB_STATUS
adc_isminmax( adf_scb, adc_dv)
ADF_CB         *adf_scb;
DB_DATA_VALUE  *adc_dv;
# endif
{
    i4			dt;
    i4			bdt;
    i4			mbdt;
    DB_STATUS		db_stat;
    DB_DATA_VALUE	tmp_dv;
    DB_DATA_VALUE	*ptr_dv;


    /* First, check to see if data value ptr is NULL */
    /* ----------------------------------------------------- */
    if (adc_dv == NULL)
    {
	/* Requestor should always supply a data value. */
	adf_scb->adf_errcb.ad_errcode = E_AD9000_BAD_PARAM;
	return(E_DB_FATAL);
    }

    /* Does the datatype exist? */
    /* ------------------------ */
    dt = adc_dv->db_datatype;
    bdt = abs(dt);
    mbdt = ADI_DT_MAP_MACRO(bdt);
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
		||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    /* Now find the correct function to call to do the processing */
    /* ---------------------------------------------------------- */
    ptr_dv = adc_dv;
    if (dt < 0)		/* nullable */
    {
	STRUCT_ASSIGN_MACRO(*adc_dv, tmp_dv);
	tmp_dv.db_datatype = bdt;
        tmp_dv.db_length--;
	ptr_dv = &tmp_dv;

        if (adc_dv->db_length != ADE_LEN_UNKNOWN &&
	    adc_dv->db_data != NULL)
		ADF_CLRNULL_MACRO(adc_dv);
    }

    db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
		adi_dt_com_vect.adp_isminmax_addr)
		    (adf_scb, ptr_dv);

    return(db_stat);
}


/*
** Name: adc_1isminmax_rti() - Actually determines whether the value is a min,
**			       a max, or a null value for the given RTI
**			       datatype as oppossed to a user defined datatype.
**
** Description:
**	This function is the internal routine to perform the adc_isminmax()
**	algorithm for the RTI datatypes.  See description of that general
**	routine above for more detail.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adc_dv				Pointer to DB_DATA_VALUE to evaluate.
**		.db_datatype		Its datatype.
**		.db_length		Its length.
**		.db_data		Pointer to location of the data value.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an error occurs,
**					the following fields will be set.
**					NOTE: if .ad_ebuflen = 0 or .ad_errmsgp
**					= NULL, no error message will be
**					formatted.
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
**	    The following DB_STATUS codes may be returned: E_DB_OK, E_DB_WARN,
**	    E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL.
**
**	    A status of E_DB_OK indicates that the value is not a minimum,
**	    maximum or null value for its datatype.
**
**	    The following codes are returned along with a status of E_DB_WARN:
**
**	    E_AD0117_IS_MINIMUM		Minimum value for type and length given
**	    E_AD0118_IS_MAXIMUM		Maximum value for type and length given
**	    E_AD0119_IS_NULL		Null value for type and length given
**
**	    The following codes are returned along with a status of E_DB_ERROR:
**
**          E_AD2004_BAD_DTID           Datatype unknown to ADF.
**          E_AD2005_BAD_DTLEN          Length is illegal for this datatype.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	31-jul-90 (linda)
**	    Written.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**       1-Jul-1993 (fred)
**          Removed (bool) cast from second argument to adc_lenchk().
**          Argument is now a nat.  See adf.h for details.
**	12-may-04 (inkdo01)
**	    Added support for bigint.
*/

DB_STATUS
adc_1isminmax_rti(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *adc_dv)
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DT_ID		dt;
    i4		len;
    PTR			data;

    adf_scb->adf_errcb.ad_errcode = E_DB_OK;

    dt   = adc_dv->db_datatype;
    len  = adc_dv->db_length;
    data = adc_dv->db_data; 

    for (;;)	/* Just to break out of... */
    {
	/* First, make sure supplied length is valid for this datatype */
	if ((db_stat = adc_lenchk(adf_scb, FALSE, adc_dv, NULL))
		!= E_DB_OK)
	    /* adf_scb->adf_errcb.ad_errcode was set by adc_lenchk(). */
	    break;

	/* Now check for  null value */
	if (adc_dv->db_datatype < 0)    /* datatype is nullable */
	{
	    if (ADI_ISNULL_MACRO(adc_dv))
	    {
		adf_scb->adf_errcb.ad_errcode = E_AD0119_IS_NULL;
		break;
	    }
	}

	/* Now, we can evaluate the value */
	switch (dt)
	{
	  case DB_BOO_TYPE:
            if (((DB_ANYTYPE *)data)->db_booltype == DB_FALSE)
		adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
            else if (((DB_ANYTYPE *)data)->db_booltype == DB_TRUE)
		adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    break;

	  case DB_FLT_TYPE:
	    if (len == 4)
	    {
		if (*(f4 *)data == -FMAX)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (*(f4 *)data == FMAX)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    else
	    {
		if (*(f8 *)data == -FMAX)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (*(f8 *)data == FMAX)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    break;

	  case DB_INT_TYPE:
	    switch (len)
	    {
	      case 1:
		if (*(i1 *)data == MINI1)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (*(i1 *)data == MAXI1)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
		break;
	      case 2:
		if (*(i2 *)data == MINI2)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (*(i2 *)data == MAXI2)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
		break;
	      case 4:
		if (*(i4 *)data == MINI4)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (*(i4 *)data == MAXI4)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
		break;
	      case 8:
		if (*(i8 *)data == MINI8)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (*(i8 *)data == MAXI8)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
		break;
	    }
	    break;

	  case DB_CHR_TYPE:
	    if (MEcmp((PTR)data, (PTR)Chr_min, len) == 0)
		adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
	    else if (MEcmp((PTR)data, (PTR)Chr_max, len) == 0)
		adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    break;

	  case DB_CHA_TYPE:
	  case DB_NCHR_TYPE:
	  case DB_BYTE_TYPE:
          case DB_NBR_TYPE:
	    if (MEcmp((PTR)data, (PTR)Cha_min, len) == 0)
		adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
	    else if (MEcmp((PTR)data, (PTR)Cha_max, len) == 0)
		adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    break;

	  case DB_TXT_TYPE:
	    {
		i4	tmplen = len - DB_CNTSIZE;

		if (((DB_TEXT_STRING *) data)->db_t_count == 0)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (MEcmp((PTR)((DB_TEXT_STRING *)data)->db_t_text,
			       (PTR)Txt_max, tmplen) == 0)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
		break;
	    }

	  case DB_VCH_TYPE:
	  case DB_NVCHR_TYPE:
	  case DB_LTXT_TYPE:
	  case DB_VBYTE_TYPE:
	    {
		i4	tmplen = len - DB_CNTSIZE;

		if (MEcmp((PTR)((DB_TEXT_STRING *)data)->db_t_text,
			  (PTR)Vch_min, tmplen) == 0)
		    adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (MEcmp((PTR)((DB_TEXT_STRING *)data)->db_t_text,
			       (PTR)Vch_max, tmplen) == 0)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
		break;
	    }

	  case DB_MNY_TYPE:
	    if (((AD_MONEYNTRNL *) data)->mny_cents == AD_3MNY_MIN_NTRNL)
		adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
	    else if (((AD_MONEYNTRNL *) data)->mny_cents == AD_1MNY_MAX_NTRNL)
		adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    break;

	  case DB_DTE_TYPE:
	    {
		AD_DATENTRNL    *d = (AD_DATENTRNL *) data;

		if (
		    (d->dn_status == AD_DN_NULL) &&
		    (d->dn_highday == 0) &&
		    (d->dn_year == 0) &&
		    (d->dn_month == 0) &&
		    (d->dn_lowday == 0) &&
		    (d->dn_time == 0)
		   )
		   adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (
			 (d->dn_status & AD_DN_ABSOLUTE) &&
			 (d->dn_status & AD_DN_YEARSPEC) &&
			 (d->dn_status & AD_DN_MONTHSPEC) &&
			 (d->dn_status & AD_DN_DAYSPEC) &&
			 (d->dn_status & AD_DN_TIMESPEC) &&
			 (d->dn_highday == 0) &&
			 (d->dn_year == AD_24DTE_MAX_ABS_YR) &&
			 (d->dn_month == 12) &&
			 (d->dn_lowday == 31) &&
			 (d->dn_time == AD_9DTE_IMSPERDAY - 1)
			)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    break;

	  case DB_ADTE_TYPE:
	    {
		AD_ADATE *d = (AD_ADATE *) data;

		if (
		    d->dn_year == 0 &&
		    d->dn_month == 0 &&
		    d->dn_day == 0
		   )
		   adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (
			 d->dn_year == AD_24DTE_MAX_ABS_YR &&
			 d->dn_month == 12 &&
			 d->dn_day == 31
			)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    break;

	  case DB_TMWO_TYPE:
	  case DB_TMW_TYPE:
	  case DB_TME_TYPE:
	    {
		AD_TIME *adtm = (AD_TIME *) data;

		if (
	     	    adtm->dn_seconds == 0 &&
	     	    adtm->dn_nsecond == 0
		   )
		   adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (
			adtm->dn_seconds == AD_40DTE_SECPERDAY-1 &&
			adtm->dn_nsecond == AD_33DTE_MAX_NSEC-1
			)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    break;

	  case DB_TSWO_TYPE:
	  case DB_TSW_TYPE:
	  case DB_TSTMP_TYPE:
	    {
		AD_TIMESTAMP *atstmp = (AD_TIMESTAMP *) data;

		if (
	            atstmp->dn_year == 0 &&
	            atstmp->dn_month == 0 &&
	            atstmp->dn_day == 0 &&
	            atstmp->dn_seconds == 0 &&
	            atstmp->dn_nsecond == 0
		   )
		   adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (
			atstmp->dn_year == AD_24DTE_MAX_ABS_YR &&
			atstmp->dn_month == 12 &&
			atstmp->dn_day == 31 &&
			atstmp->dn_seconds == AD_40DTE_SECPERDAY-1 &&
			atstmp->dn_nsecond == AD_33DTE_MAX_NSEC
			)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    break;

          case DB_INYM_TYPE:
	    {
		AD_INTYM *aintym = (AD_INTYM *) data;

		if (
	            aintym->dn_years == 0 &&
	            aintym->dn_months == 0
		   )
		   adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (
			aintym->dn_years == AD_26DTE_MAX_INT_YR &&
			aintym->dn_months == 12
			)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    break;

	  case DB_INDS_TYPE:
               {
	            AD_INTDS *aintds = (AD_INTDS *) data;

		if (
	            aintds->dn_days == 0 &&
	            aintds->dn_seconds == 0 &&
	            aintds->dn_nseconds == 0
		   )
		   adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		else if (
			aintds->dn_days == AD_28DTE_MAX_INT_DAY &&
			aintds->dn_seconds == AD_40DTE_SECPERDAY-1 &&
			aintds->dn_nseconds == AD_33DTE_MAX_NSEC
			)
		    adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    }
	    break;

	  case DB_LOGKEY_TYPE:
	  case DB_TABKEY_TYPE:
	    if (MEcmp((PTR)data, (PTR)Lke_min, len) == 0)
		adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
	    else if (MEcmp((PTR)data, (PTR)Lke_max, len) == 0)
		adf_scb->adf_errcb.ad_errcode = E_AD0118_IS_MAXIMUM;
	    break;

	  case DB_BIT_TYPE:
	  case DB_VBIT_TYPE:
	  {
	      DB_DATA_VALUE	bit_dv;
	      i4		result;
	      
	      bit_dv.db_datatype = dt;
	      bit_dv.db_length = len;
	      bit_dv.db_prec = adc_dv->db_prec;

	      bit_dv.db_data = (PTR) Bit_min;
	      db_stat = adc_compare(adf_scb, adc_dv, &bit_dv, &result);
	      if (DB_SUCCESS_MACRO(db_stat))
	      {	
		 if (result == 0)
		 {
		     adf_scb->adf_errcb.ad_errcode = E_AD0117_IS_MINIMUM;
		 }
		 else
		 {
		     bit_dv.db_data = (PTR) Bit_max;
		     db_stat = adc_compare(adf_scb, adc_dv, &bit_dv, &result);
		     if (DB_SUCCESS_MACRO(db_stat))
		     {	
			 if (result == 0)
			 {
			     adf_scb->adf_errcb.ad_errcode =
				 E_AD0118_IS_MAXIMUM;
			 }
		     }
		 }
	     }
	    break;
	  }

	  default:
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}   /* end case */

	break;

    }	/* end for (;;) */

    if (adf_scb->adf_errcb.ad_errcode != E_DB_OK)
	if (
	    (adf_scb->adf_errcb.ad_errcode == E_AD0117_IS_MINIMUM)
	     ||
	    (adf_scb->adf_errcb.ad_errcode == E_AD0118_IS_MAXIMUM)
	     ||
	    (adf_scb->adf_errcb.ad_errcode == E_AD0119_IS_NULL)
	    )
		db_stat = E_DB_WARN;

    return(db_stat);
}
