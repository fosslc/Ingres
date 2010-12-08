/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adudate.h>
#include    <adumoney.h>

/**
**
**  Name: ADCVALCHK.C - Make sure data is a valid value for its type.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_valchk()" which
**      checks to make sure data is a valid value for its type.
**
**      This file defines:
**
**          adc_valchk() - Make sure data is a valid value for its type.
**          adc_1valchk_rti() - Does the adc_valchk() function for RTI datatypes
**
**
**  History:
**      25-mar-87 (thurston)
**          Initial creation.
**	11-may-87 (thurston)
**	    In adc_1valchk_rti():  Added fix for dates that get encoded from the
**          string `today' (and possibly from `now').  Historically, these dates
**          only got the AD_DN_ABSOLUTE flag set, therefore, some of the
**          validation checks I was applying would invalidate any such dates
**          from existing data bases if used in the 6.0 copy command.
**          Therefore, the validation criteria for absolute dates has been
**          relaxed ... it will always be assumed that absolute dates have at
**          least a year, month, and day.
**	19-jun-87 (thurston)
**	    In adc_1valchk_rti(), I added another check for text:  Now looks for
**	    chars > AD_MAX_TEXT. 
**	24-aug-87 (thurston)
**	    Bug fix to adc_valchk(); did not look for NULL values, and call them
**	    OK.  Instead, it just passed them as non-nullable values and had the
**	    low level routines check them.  This caused random failures.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	21-jun-88 (jrb)
**	    Mods for Kanji support.
**	06-nov-88 (thurston)
**	    Removed the `readonly' from the static variable `all_date_bits'.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_2tmcvt_rti().
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jun-89 (fred)
**	    Completed that fix.
**	05-jul-89 (jrb)
**	    Added decimal support.
**      14-dec-90 (jrb)
**          Bug 32679.  Allow negative values in the time field of date values.
**          This can sometimes occur when the timezone offset is negative.
**	04-sep-1992 (fred)
**	    Added DB_LVCH_TYPE support.  Also fixed some ANSI C issues.
**      23-dec-1992 (stevet)
**          Added function prototype.
**      06-Apr-1993 (fred)
**          Added byte datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	23-feb-2001 (gupsh01)
**	    Added support for unicode nchar and nvarchar datatype.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-apr-2001 (horda03) Bug 104518 INGCBT 348
**          In Feb 1998 support for dates between years 0001 and 9999 was
**          introduced. This created two new bit flags for Dates
**          AD_DN_AFTER_EPOCH and AD_DN_BEFORE_EPOCH. Because all_date_bits
**          was defined in this file, these new values were ommitted, thus
**          allowing valid dates to be considered invalid. Modified the code
**          to use AD_DN_VALID_DATE_STATES which is the list of valid date
**          bit states.
**      06-Sep-2001 (hanal04) Bug 105417 INGSRV 1515
**          Value checking of absolute dates in adc_1valchk_rti() assumed
**          that year, month and day values are present. This may not be
**          the case. We should only check the values which are present.
**      16-oct-2006 (stial01)
**          Value checking for locator datatypes
**	18-Feb-2008 (kiria01) b120004
**	    Consolidate timezone handling. Updated adu_6to_dtntrnl
**	    to new form adu_6to_dtntrnl for inteface change.
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**      13-aug-2009 (joea)
**          Add case for DB_BOO_TYPE in adc_1valchk_rti.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**/


/*{
** Name: adc_valchk() - Make sure data is a valid value for its type.
**
** Description:
**      This function is the external ADF call "adc_valchk()" which
**      checks to make sure data is a valid value for its type.
**
**      If the supplied data is indeed a valid value for its datatype,
**      then the routine will return E_DB_OK.  If everything else checks
**	out except the validity of the data, then E_DB_ERROR will be
**	returned with the E_AD1014_BAD_VALUE_FOR_DT error code.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv                          The data value to check.
**              .db_datatype            Datatype id for the data value.
**		.db_prec		Its precision/scale.
**              .db_length              Its length.
**		.db_data		Pointer to the data.
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
**	    E_DB_OK			All is fine, and value is valid for
**					its type.
**	    E_DB_ERROR			( see below )
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD1014_BAD_VALUE_FOR_DT   Value was NOT valid for its type.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**		Also, if xDEBUG is being used ...
**
**          E_AD2005_BAD_DTLEN          Illegal length for this datatype.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      25-mar-87 (thurston)
**          Initial creation.
**	24-aug-87 (thurston)
**	    Bug fix; did not look for NULL values, and call them OK.  Instead,
**          it just passed them as non-nullable values and had the low level
**          routines check them.  This caused random failures.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adc_valchk(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *adc_dv)
{
    i4		   	bdt = (i4) ADF_BASETYPE_MACRO(adc_dv);
    DB_STATUS		db_stat;
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);


    /* does the datatype id exist? */
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
	||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

#ifdef xDEBUG
    /* is the length valid? */
    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dv, NULL))
	return (db_stat);
#endif /* xDEBUG */


    if (adc_dv->db_datatype > 0)	/* non-nullable */
    {
        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_valchk_addr)
			(adf_scb, adc_dv);
    }
    else if (ADI_ISNULL_MACRO(adc_dv))	/* nullable, with NULL value */
    {
	db_stat = E_DB_OK;
    }
    else				/* nullable, with non-NULL value */
    {
	DB_DATA_VALUE	    tmp_dv;

	STRUCT_ASSIGN_MACRO(*adc_dv, tmp_dv);
	tmp_dv.db_datatype = bdt;
	tmp_dv.db_length--;

        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_valchk_addr)
			(adf_scb, &tmp_dv);
    }

    return (db_stat);
}


/*
** Name: adc_1valchk_rti() - Does the adc_valchk() function for RTI datatypes.
**
** Description:
**        This function is an internal implementation of the ADF call
**	"adc_valchk".  It performs the same function as the external 
**	"adc_valchk".  Its purpose is to do the "real work" for datatypes
**	known to the RTI products.  These datatypes include c, text,
**	money, date, i, decimal, f, logical_key, char, and varchar.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv                          The data value to check.
**              .db_datatype            Datatype id for the data value.
**		.db_prec		Its precision/scale
**              .db_length              Its length.
**		.db_data		Pointer to the data.
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
**	    E_DB_OK			All is fine, and value is valid for
**					its type.
**	    E_DB_ERROR			( see below )
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD1014_BAD_VALUE_FOR_DT   Value was NOT valid for its type.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      25-mar-87 (thurston)
**	    Initially written.
**	11-may-87 (thurston)
**	    Added fix for dates that get encoded from the string `today' (and
**	    possibly from `now').  Historically, these dates only got the
**	    AD_DN_ABSOLUTE flag set, therefore, some of the validation checks
**	    I was applying would invalidate any such dates from existing data
**	    bases if used in the 6.0 copy command.  Therefore, the validation
**	    criteria for absolute dates has been relaxed ... it will always be
**	    assumed that absolute dates have at least a year, month, and day.
**	19-jun-87 (thurston)
**	    Added another check for text:  Now looks for chars > AD_MAX_TEXT.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	20-jun-88 (jrb)
**	    Mods for Kanji support.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_2tmcvt_rti().
**	05-jul-89 (jrb)
**	    Added decimal support.
**	04-sep-1992 (fred)
**	    Added DB_LVCH_TYPE support.  Also fixed some ANSI C issues.
**	22-sep-1992 (fred)
**	    Added DB_BIT_TYPE, DB_LBIT_TYPE, and DB_VBIT_TYPE support.
**      06-Apr-1993 (fred)
**          Added DB_{BYTE,VBYTE,LBYTE}_TYPEs.
**	04-Aug-93   (shailaja)
**	    Unnested comments.
**	24-Aug-1993 (wolf) 
**	    Really unnested the comments.
**      23-feb-2001 (gupsh01)
**          Added support for unicode nchar and nvarchar datatype.
**	03-apr-2001 (gupsh01)
**	    Added support for unicode long nvarchar datatype.
**      06-Sep-2001 (hanal04) Bug 105417 INGSRV 1515
**          thurston's change in May 1987 was not complete. It is true that
**          the year, month and day may not all be present. However,
**          the subsequent value checking of these fields should only check
**          the fields that are present.
**	16-Jun-2006 (gupsh01)
**	    Added support for new ANSI datetime data types.
**	12-Dec-2006 (gupsh01)
**	    Further restrict the timezhone hours and minutes values to 
**	    remain within 14:00 and -12:59.
**	7-feb-2007 (dougi)
**	    Minor fix to previous change - it left an uninit'ed ptr.
**	7-feb-2007 (dougi)
**	    Another minor fix to error checking.
**      23-Jul-2007 (hanal04) Bug 118788
**          Correct the TZ boundary checks. -12:59 and 14:00 hours is the 
**          range.
*/

DB_STATUS
adc_1valchk_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_dv)
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			bad = FALSE;
    i4			cnt;
    f8			cents;
    i4			tmp;
    i4			pr;
    u_char		*ch;
    u_char		*endch;
    AD_DATENTRNL	*d;
    AD_NEWDTNTRNL	*nd, ndt;
    i4		longday;
    i4		longbits;
    bool		fallthru = FALSE;
    i2			tzmins;
    i1 i1val;

    switch (adc_dv->db_datatype)
    {
      case DB_LVCH_TYPE:
      case DB_CHA_TYPE:
      case DB_INT_TYPE:
      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
      case DB_BIT_TYPE:
      case DB_LBIT_TYPE:
      case DB_BYTE_TYPE:
      case DB_LBYTE_TYPE:
      case DB_GEOM_TYPE:
      case DB_POINT_TYPE:
      case DB_MPOINT_TYPE:
      case DB_LINE_TYPE:
      case DB_MLINE_TYPE:
      case DB_POLY_TYPE:
      case DB_MPOLY_TYPE:
      case DB_GEOMC_TYPE:
      case DB_NCHR_TYPE:
      case DB_LNVCHR_TYPE:
      case DB_LBLOC_TYPE:
      case DB_LCLOC_TYPE:
      case DB_LNLOC_TYPE:
      case DB_NBR_TYPE:
	/* All bit patterns will be legal for these datatypes */
	break;

      case DB_BOO_TYPE:
        i1val = ((DB_ANYTYPE *)adc_dv->db_data)->db_booltype;
        if (!(i1val == DB_FALSE || i1val == DB_TRUE))
            bad = TRUE;
        break;

      case DB_TXT_TYPE:
	/* Look for bad count field, NULLCHARs, or chars > AD_MAX_TEXT */
	cnt = ((DB_TEXT_STRING *)adc_dv->db_data)->db_t_count;
	if (cnt > adc_dv->db_length - DB_CNTSIZE  ||  cnt < 0)
	{
	    bad = TRUE;
	}
	else
	{
	    ch = ((DB_TEXT_STRING *)adc_dv->db_data)->db_t_text;
	    endch = ch + cnt;
	    while (ch < endch  &&  !bad)
	    {
		if (*ch == NULLCHAR  ||  *ch > AD_MAX_TEXT)
		    bad = TRUE;
		CMnext(ch);
	    }
	}
	break;

      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NVCHR_TYPE:
	/* Look for bad count field */
	if (adc_dv->db_datatype != DB_NVCHR_TYPE)
	    cnt = ((DB_TEXT_STRING *)adc_dv->db_data)->db_t_count;
	else
	    cnt = ((DB_NVCHR_STRING *)adc_dv->db_data)->count;
	if (cnt > adc_dv->db_length - DB_CNTSIZE  ||  cnt < 0)
	    bad = TRUE;
	break;

      case DB_VBIT_TYPE:
      {
	DB_BIT_STRING 	bs;

	/* Look for invalid count fields */
	longbits = ((DB_BIT_STRING *) adc_dv->db_data)->db_b_count;
	if ((longbits < 0) ||
	    (longbits > ((adc_dv->db_length - sizeof(bs.db_b_count))
			                                  * BITSPERBYTE)))
	{
	    bad = TRUE;
	}
	break;
      }

      case DB_CHR_TYPE:
	/* Search for non-printing characters */
	cnt = adc_dv->db_length;
	if (cnt < 0)
	{
	    bad = TRUE;
	}
	else
	{
	    ch = (u_char *)adc_dv->db_data;
	    endch = ch + cnt;
	    
	    while (ch < endch  &&  !bad)
	    {
		if (!CMprint((char *)ch))
		    bad = TRUE;
		CMnext(ch);
	    }
	}
	break;


      case DB_DEC_TYPE:
	ch    = (u_char *)adc_dv->db_data;
	pr    = DB_P_DECODE_MACRO(adc_dv->db_prec);

	/* if the precision is even, the high nybble of the first byte should
	** be zero since it's not used.
	*/
	if ((pr & 1) == 0  &&  (*ch & 0xf0) != 0)
	{
	    bad = TRUE;
	    break;
	}

	/* now check each digit for validity */
	while (pr > 0)
	{
	    if ((u_char) (((pr-- & 1)  ?
			(*ch >> 4)  :  *ch++) & 0xf) > (u_char) 0x9)
	    {
		bad = TRUE;
		break;
	    }
	}

	/* now check the sign if ok so far */
	if (!bad  &&
		((u_char) (*ch & 0xf) < (u_char) 0xa  ||
		 (u_char) (*ch & 0xf) > (u_char) 0xf))
	    bad = TRUE;
	break;
	
      case DB_FLT_TYPE:
	/* {@fix_me@} Do we want to look for illegal bit patterns for floats? */
	break;

      case DB_MNY_TYPE:
	/* Look for illegal money values */
	/* {@fix_me@} Do we want to look for illegal bit patterns for floats? */
	cents = ((AD_MONEYNTRNL *)adc_dv->db_data)->mny_cents;
	if (cents > AD_1MNY_MAX_NTRNL  ||  cents < AD_3MNY_MIN_NTRNL)
	    bad = TRUE;
	break;

      case DB_DTE_TYPE:
	/* Look for illegal dates */
	d = (AD_DATENTRNL *)adc_dv->db_data;
	tmp = d->dn_status;

	/* Check for empty date ... OK */
	if (tmp == AD_DN_NULL)
	    break;	/* empty date */

	/* Look for unused status bits being set */
	if (tmp & (~AD_DN_VALID_DATE_STATES))
	{
	    bad = TRUE;
	    break;
	}

	/* Must be at least 2 bits set if LENGTH ... LENGTH, plus a SPEC */
	if (	tmp & AD_DN_LENGTH
	    &&  BTcount((char *)&tmp, (i4)(BITS_IN(tmp))) < 2
	   )
	{
	    bad = TRUE;
	    break;
	}

	/* Make sure date is an absolute or an interval ... and not both */
	tmp = d->dn_status & (AD_DN_ABSOLUTE | AD_DN_LENGTH);
	if (BTcount((char *)&tmp, (i4)(BITS_IN(tmp))) != 1)
	{
	    bad = TRUE;
	    break;
	}

	if (d->dn_status & AD_DN_ABSOLUTE)	/* If absolute date ... */
	{
/* Cannot do this check because historically, dates encoded from the `today'
** string only got the AD_DN_ABSOLUTE flag set ... no spec flags were set.
** -------------------------------------------------------------------------
**	    ** ... make sure we have at least a year, month, and day spec **
**	    tmp = d->dn_status & (  AD_DN_YEARSPEC
**				  | AD_DN_MONTHSPEC
**				  | AD_DN_DAYSPEC);
**	    if (BTcount(tmp, (i4)BITS_IN(tmp)) != 3)
**	    {
**		bad = TRUE;
**		break;
**	    }
*/
	    /* ... check year, month, day for out of range */
            longday = (I1_CHECK_MACRO(d->dn_highday) << AD_21DTE_LOWDAYSIZE)
		    + d->dn_lowday;
	    if (((d->dn_status & AD_DN_YEARSPEC) && 
                       (d->dn_year < AD_23DTE_MIN_ABS_YR
		       || d->dn_year > AD_24DTE_MAX_ABS_YR))
		||  
                ((d->dn_status & AD_DN_MONTHSPEC) &&
                       (d->dn_month < 1 || d->dn_month > 12))
		||
                ((d->dn_status & AD_DN_DAYSPEC) &&
                       (longday < 1 || longday > 31))
	       )
	    {
		bad = TRUE;
		break;
	    }
	}
	else					/* If interval date ... */
	{
	    /* ... with yearspec, check for out of range */
	    if (    d->dn_status & AD_DN_YEARSPEC
		&& (    d->dn_year < AD_25DTE_MIN_INT_YR
		    ||  d->dn_year > AD_26DTE_MAX_INT_YR
		   )
	       )
	    {
		bad = TRUE;
		break;
	    }
	}

	break;

	case DB_ADTE_TYPE:
	    nd = &ndt;
	    if (db_stat = adu_6to_dtntrnl (adf_scb, adc_dv, nd))
		return (db_stat);
            if ((nd->dn_year < AD_23DTE_MIN_ABS_YR ||
		 nd->dn_year > AD_24DTE_MAX_ABS_YR)
                ||
                (nd->dn_month < 1 || nd->dn_month > 12)
                ||
                ((nd->dn_day < 1 || nd->dn_day > 31)))
            {
                bad = TRUE;
		break;
            }

	break;

    	case DB_TSTMP_TYPE:
	case DB_TSWO_TYPE:
	case DB_TSW_TYPE:
	    nd = &ndt;
	    if (db_stat = adu_6to_dtntrnl (adf_scb, adc_dv, nd))
		return (db_stat);
            if ((nd->dn_year < AD_23DTE_MIN_ABS_YR ||
		 nd->dn_year > AD_24DTE_MAX_ABS_YR)
                ||
                (nd->dn_month < 1 || nd->dn_month > 12)
                ||
                (nd->dn_day < 1 || nd->dn_day > 31))
            {
    		bad = TRUE;
		break;
            }
            fallthru = TRUE;
            /* no break here intentionally */
	case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
	case DB_TME_TYPE:
            if(fallthru == FALSE)
            {
	        nd = &ndt;
		if (db_stat = adu_6to_dtntrnl (adf_scb, adc_dv, nd))
		    return (db_stat);
            }
	    if ((nd->dn_nsecond > AD_33DTE_MAX_NSEC ||
		 nd->dn_nsecond < AD_32DTE_MIN_NSEC)
		||
                (nd->dn_tzoffset > AD_48DTE_MAX_TZ_MN ||
		 nd->dn_tzoffset < AD_47DTE_MIN_TZ_MN))
	    {
    		bad = TRUE;
		break;
            }
	    break;

	case DB_INYM_TYPE:
	    nd = &ndt;
	    if (db_stat = adu_6to_dtntrnl (adf_scb, adc_dv, nd))
		return (db_stat);
            if ((nd->dn_year < AD_25DTE_MIN_INT_YR ||
		 nd->dn_year > AD_26DTE_MAX_INT_YR))
            {
		bad = TRUE;
                break;
	    }
            break;

	case DB_INDS_TYPE:
	    nd = &ndt;
	    if (db_stat = adu_6to_dtntrnl (adf_scb, adc_dv, nd))
		return (db_stat);
	    if ((nd->dn_day > AD_28DTE_MAX_INT_DAY ||
		 nd->dn_day < AD_27DTE_MIN_INT_DAY)
		||
	        (nd->dn_nsecond > AD_33DTE_MAX_NSEC ||
		 nd->dn_nsecond < AD_32DTE_MIN_NSEC))
            {
		bad = TRUE;
                break;
            }
            break;

      default:
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }	/* end of switch stmt */


    if (bad)
	return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));

    return (db_stat);
}
