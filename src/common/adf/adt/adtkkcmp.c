/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <gl.h>
#include    <sl.h>
#include    <st.h>
#include    <cm.h>
#include    <sl.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adulcol.h>
#include    <aduucol.h>
#include    <aduint.h>
#include    <adudate.h>
#include    <adumoney.h>

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
** NO_OPTIM = nc4_us5
*/

/**
**
**  Name: ADTKKCMP.C - Compare a key with a key.
**
**  Description:
**	This file contains all of the routines necessary to perform the
**	external ADF call "adt_kkcmp()" which compares a key with a key.
**
**      This file defines:
**
**          adt_kkcmp() - Compare a key with a key.
**
**
**  History:
**      11-jun-86 (jennifer)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	19-nov-86 (thurston)
**	    Fixed the comparison for text in adt_tupcmp().
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	07-apr-1987 (Derek)
**	    Added support for new ADT_ATTS.
**	22-sep-88 (thurston)
**	    In adt_kkcmp(), I added bit representation check as a first pass
**	    on each attr.  Also, added code to deal with nullable types here
**	    ... this could be a big win, since all nullable types were going
**	    through adc_compare(), now they need not.  Also, added cases for
**	    CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	21-Apr-89 (anton)
**	    Added local collation support
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	18-aug-89 (jrb)
**	    Copied bug fix from adu code to adt routines.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	28-oct-1992 (rickh)
**	    Replaced references to ADT_ATTS with DB_ATTS.  This compresses
**	    into one common structure the identical ADT_ATTS, DMP_ATTS, and
**	    GW_DMP_ATTS.
**      29-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	26-feb-96 (thoda04)
**	    Added mh.h to include function prototype for MKpkcmp().
**	    Added aduint.h to include function prototype for adu_1seclbl_cmp().
**	13-jul-00 (hayke02)
**	    Added NO_OPTIM for NCR (nc4_us5). This change fixes bug 99864.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	06-apr-2001 (abbjo03)
**	    Fix 3/28 change: include aduucol.h and correct first parameter to
**	    aduucmp().
**	01-may-2001 (abbjo03)
**	    Further correction to 3/28 change: fix calculation of DB_NCHR_TYPE
**	    endpoints.
**	10-aug-2001 (abbjo03)
**	    Yet another fix to 3/28/01 change. Correct the DB_NVCHR_TYPE start
**	    of data points.
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**	9-Jan-2004 (schka24)
**	    Add 8-byte int case.
**      4-dec-2009 (stial01)
**          Added adt_key_incr(), adt_key_decr(), adt_key_max()
**/

static DB_STATUS adt_key_max(
				ADF_CB		*adf_scb,
				DB_ATTS	        *atr,
				char		*val);


/*{
** Name: adt_kkcmp() - Compare a key with a key.
**
** Description:
**	This routine compares two keys.  This will tell the caller if the
**	supplied tuples are equal, or if not, which one is "greater than" the
**	other.
**
**	This routine exists as a performance boost for DMF.  DMF is not allowed
**	to know how to compare data values for the various datatypes.
**	Therefore, if this routine did not exist, DMF would have to make a
**	function call to "adc_compare()" for every attribute in the tuples
**	being compared.  Even though tuples are constructs that seem a level
**	above what ADF logically handles, the performance improvement in this
**	case justifies this routine.
**
**	To avoid the overhead of a function call for each attribute in the
**	tuples to be compared, this routine has built into it the semantics for
**	comparing several of the most common datatypes supported by INGRES.
**
**	If the routine does run across a datatype that is not in this list, it
**	will call the general routine "adc_compare()", thereby guaranteeing
**	that it will function (perhaps slower), for ALL datatypes, even future
**	user defined ADTs.
**
**	When NULL values are involved, this routine will use the same semantics
**	that "adc_compare()" uses:  a NULL value will be considered greater
**	than any other value, and equal to another NULL value.
**
**	PLEASE NOTE:  As with "adc_compare()", no pattern matching for
**	-----------   the string datatypes is done within this routine.
**
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adt_nkatts                      Number of attributes that make
**                                      up the key.
**	adt_katts                       Pointer to vector of pointers.
**					Each pointer in this vector
**					will point to a DB_ATTS
**					corresponding to an attribute
**					in the tuple that helps make
**					up the key.  This vector will
**					be in "key" order.
**	adt_key1			Pointer to the key.  This is
**					merely a byte string, with the
**					attributes in tuple format,
**					lined up in key order.
**      adt_key2                        Pointer to the tuple data
**                                      record.
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
**      adt_cmp_result                  Result of comparison.  This is
**                                      guranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
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
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.
**		(Others not yet defined)
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      11-jun-86 (jennifer)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	19-nov-86 (thurston)
**	    Fixed the comparison for text.
**	07-apr-1987 (Derek)
**	    Added support for new DB_ATTS structure.  This structure now
**	    contains the key offset explicitly instead of running down the
**	    key by using the previous key length fields.
**	22-sep-88 (thurston)
**	    Added bit representation check as a first pass on each attr.  Also,
**	    added code to deal with nullable types here ... this could be a big
**	    win, since all nullable types were going through adc_compare(), now
**	    they need not.  Also, added cases for CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	21-Apr-89 (anton)
**	    Added local collation support
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	08-aug-91 (seg)
**	    "d1" and "d2" are dereferenced too often with the assumption
**	    that (PTR) == (char *) to fix.  Made them (char *).
**	28-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	16-dec-04 (inkdo01)
**	    Add collation ID parms to aduucmp() calls.
**	02-Feb-2005 (jenjo02)
**	    Changed to call (new) adt_compare function for
**	    each attribute.
*/

DB_STATUS
adt_kkcmp(
ADF_CB		    *adf_scb,
i4		    adt_nkatts,		/* Number attrs in key. */
DB_ATTS	            **adt_katts,	/* Ptr to vector of ptrs.
					** Each ptr in this vector
					** will point to a DB_ATTS
					** corresponding to an attribute
					** in the tuple that helps make
					** up the key.  This vector will
					** be in "key" order.
					*/
char		    *adt_key1,		/* Ptr to the key. */
char		    *adt_key2,		/* Ptr to the key. */
i4		    *adt_cmp_result)	/* Place to put cmp result. */
{
    DB_STATUS		status;
    DB_ATTS		*atr;

    *adt_cmp_result = 0;
    status = E_DB_OK;

    /* As long as there are equal attributes... */
    while ( adt_nkatts-- > 0 && status == E_DB_OK && *adt_cmp_result == 0 )
    {
	atr = *(adt_katts++);
	*adt_cmp_result = adt_compare(adf_scb,
				      atr,
				      adt_key1 + atr->key_offset,
				      adt_key2 + atr->key_offset,
				      &status);
    }

    return(status);
}


/*{
** Name: adt_key_incr() - Increment a key.
**
** Description:
**      This routine will increment a key.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	nkatts                          Number of attributes in the key.
**	katts                           Pointer to vector of pointers.
**					Each pointer in this vector
**					will point to a DB_ATTS
**					corresponding to an attribute
**					in the tuple that helps make
**					up the key.  This vector will
**					be in "key" order.
**	klen				
**	incr_key			Pointer to key to be incremented.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**                                      Error information set in adf_errcb field
**      incr_key                        If the key can be incremented
**                                      
** Returns:
**	The following DB_STATUS codes may be returned:
**	E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	If a DB_STATUS code other than E_DB_OK is returned, the caller
**	can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	more information about the error.
**
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      04-dec-2009 (stial01) 
**          Initial creation.
*/
DB_STATUS
adt_key_incr(
ADF_CB		*adf_scb,
i4		nkatts,		/* Number attrs in key. */
DB_ATTS		**katts,	/* Ptr to vector of ptrs.
				** Each ptr in this vector
				** will point to a DB_ATTS
				** corresponding to an attribute
				** in the tuple that helps make
				** up the key.  This vector will
				** be in "key" order.
				*/
i4		klen,
char		*incr_key)	/* Ptr to the key. */
{
    DB_STATUS		status;
    DB_ATTS		*atr;
    char		*val;
#define TMPSIZE sizeof(DB_ANYTYPE) + sizeof(AD_DATENTRNL) + sizeof(AD_NEWDTNTRNL) + sizeof(ALIGN_RESTRICT)
    ALIGN_RESTRICT	tmp[TMPSIZE];
    i4			atr_bdt;	/* Base datatype of attr */
    i4			atr_len;	/* Length of current attr */
    DB_ATTS	        **cur_att;
    i4			tmplen;
    i4			chlen;
    i4			startpos;
    char		charbuf[20]; /* big enough for multi byte chars */
    bool		multi_byte = FALSE;
    DB_DATA_VALUE	ldv, chdv, dv;
    char		*msp; /* most significant */
    u_char		*p;
    u_char		*p_end;
    u_char		*null_byte;
    i4			origlen;
    u_i2		dlen; /* declared length */
    u_i2		alen; /* actual length */
    i4			pos;
    bool		incr_done = FALSE;
    bool		incr_tmp = FALSE;
    u_char		*max;
    u_char		*min;
    bool		isbyte;
    bool		ischar;
    bool		is_nchar;
    i4			elmsize;
    UCS2		umin = AD_MIN_UNICODE;
    UCS2		umax = AD_MAX_UNICODE;
    UCS2		utmp;

    status = E_DB_OK;

    if ((Adf_globs->Adi_status & ADI_DBLBYTE) ||
	(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
	multi_byte = TRUE;

    /* start at least significant, break as soon as we incr any component */
    cur_att = katts + (nkatts - 1);

    /* key not aligned */
    while ( nkatts-- > 0 && status == E_DB_OK )
    {
	atr = *(cur_att--);
	val = incr_key + atr->key_offset;
	atr_len = atr->length;
	atr_bdt = atr->type;
	min = NULL;
	max = NULL;
	isbyte = FALSE;
	ischar = FALSE;
	is_nchar = FALSE;
	elmsize = 0;

	/* Check for nullable dt */
	if ( atr_bdt < 0 )
	{
	    null_byte = (u_char *)val + atr_len - 1;
	    if (*null_byte & ADF_NVL_BIT)
	    {
		/* nulls are > non null values, so can't increment */
		continue;
	    }
	    atr_bdt = -atr_bdt;
	    atr_len--;
	}

	/* Don't know how to increment */
	if (atr_bdt == DB_DEC_TYPE || 
		atr_bdt == DB_NCHR_TYPE || atr_bdt == DB_NVCHR_TYPE ||
		atr_bdt == DB_BIT_TYPE || atr_bdt == DB_VBIT_TYPE)
	    continue;

	/* Init a dv with no nulls, aligned numeric data */
	dv.db_datatype = atr_bdt;
	dv.db_length = atr_len;
	dv.db_data = val;
	if (atr_len <= sizeof(tmp))
	{
	    /* align the numeric datatypes */
	    MEcopy(val, atr_len, tmp);
	    dv.db_data = (PTR)&tmp[0];
	}

	status = adc_isminmax(adf_scb, &dv);
	if (status == E_DB_WARN)
	{
	    if (adf_scb->adf_errcb.ad_errcode == E_AD0118_IS_MAXIMUM)
	    {
		if (atr->type < 0)
		{
		    /* null is greater than any other value, increment */
		    *((char *)val + atr->length- 1) = ADF_NVL_BIT;
		    incr_done = TRUE;
		    break;
		}
		else
		{
		    status = E_DB_OK;
		    continue; /* to prev key */
		}
	    }
	}

	if (status == E_DB_ERROR)
	    break;

	switch (atr_bdt)
	{
	    case DB_INT_TYPE:
	    {
		incr_tmp = TRUE;
		if (atr_len == 1)
		    (*(i1 *)tmp)++;
		else if (atr_len == 2)
		    (*(i2 *)tmp)++;
		else if (atr_len == 4)
		    (*(i4 *)tmp)++;
		else if (atr_len == 8)
		    (*(i8 *)tmp)++;
		break;
	    }

	    case DB_FLT_TYPE:
	    {
		incr_tmp = TRUE;
		if (atr_len == 4)
		{
		    if (*(f4 *)tmp > 0.0)
			(*(f4 *)tmp) = (*(f4 *)tmp) * Adf_globs->Adc_hdec.fp_f4neg;
		    else if (*(f4 *)tmp < 0.0)
			(*(f4 *)tmp) = (*(f4 *)tmp) * Adf_globs->Adc_hdec.fp_f4pos;
		    else
			(*(f4 *)tmp) = Adf_globs->Adc_hdec.fp_f4nil;
		}
		else if (atr_len == 8)
		{
		    if (*(f8 *)tmp > 0.0)
			(*(f8 *)tmp) = (*(f8 *)tmp) * Adf_globs->Adc_hdec.fp_f8neg;
		    else if (*(f8 *)tmp < 0.0)
			(*(f8 *)tmp) = (*(f8 *)tmp) * Adf_globs->Adc_hdec.fp_f8pos;
		    else
			(*(f8 *)tmp) = Adf_globs->Adc_hdec.fp_f8nil;
		}
		break;
	    }

	    case DB_MNY_TYPE:
	    {
		((AD_MONEYNTRNL *)tmp)->mny_cents++;
		incr_tmp = TRUE;
		break;
	    }

	    case DB_BOO_TYPE:
	    {
		(*(i1 *)tmp) = DB_TRUE;
		incr_tmp = TRUE;
		break;
	    }

	    case DB_DTE_TYPE:
	    {
		AD_DATENTRNL    *d = (AD_DATENTRNL *) tmp;

		incr_tmp = TRUE;
		if ((d->dn_status & AD_DN_TIMESPEC) &&
			    d->dn_time < AD_9DTE_IMSPERDAY - 2)
		    d->dn_time += 1;
		else if ((d->dn_status & AD_DN_DAYSPEC) && 
			(d->dn_status & AD_DN_MONTHSPEC) && 
			(d->dn_status & AD_DN_YEARSPEC) && 
			d->dn_lowday < adu_2monthsize(d->dn_month, d->dn_year))
		{
		    d->dn_lowday += 1;
		}
		else if ((d->dn_status & AD_DN_MONTHSPEC) && d->dn_month < 12)
		{
		    d->dn_month += 1;
		    d->dn_lowday = 1; 
		}
		else if ((d->dn_status & AD_DN_YEARSPEC) && 
			    d->dn_year < AD_24DTE_MAX_ABS_YR - 1)
		{
		    d->dn_year += 1;
		    d->dn_month = 1;
		    d->dn_lowday = 1;
		}
		else
		    incr_tmp = FALSE;
		break;
	    }

	    case DB_ADTE_TYPE:
	    {
		AD_ADATE *ad = (AD_ADATE *) tmp;

		incr_tmp = TRUE;
		if (ad->dn_day < adu_2monthsize(ad->dn_month, ad->dn_year))
		    ad->dn_day +=1;
		else if (ad->dn_month < 12)
		{
		    ad->dn_month +=1;
		    ad->dn_day = 1;
		}
		else if (ad->dn_year < AD_24DTE_MAX_ABS_YR - 1)
		{
		    ad->dn_year +=1;
		    ad->dn_month = 1;
		    ad->dn_day = 1;
		}
		else
		    incr_tmp = FALSE;
		break;
	    }

	    case DB_TMWO_TYPE:
	    case DB_TMW_TYPE:
	    case DB_TME_TYPE:
	    {
		AD_TIME *adtm = (AD_TIME *) tmp;

		incr_tmp = TRUE;
		if (adtm->dn_nsecond < AD_33DTE_MAX_NSEC-1)
		    adtm->dn_nsecond +=1;
		else if (adtm->dn_seconds < AD_40DTE_SECPERDAY-1)
		{
		    adtm->dn_seconds +=1;
		    adtm->dn_nsecond = AD_32DTE_MIN_NSEC;
		}
		else
		    incr_tmp = FALSE;
		break;
	    }

	    case DB_TSWO_TYPE:
	    case DB_TSW_TYPE:
	    case DB_TSTMP_TYPE:
	    {
		AD_TIMESTAMP *atstmp = (AD_TIMESTAMP *) tmp;

		incr_tmp = TRUE;
		if (atstmp->dn_nsecond < AD_33DTE_MAX_NSEC-1)
		    atstmp->dn_nsecond  +=1;
		else if (atstmp->dn_seconds < AD_40DTE_SECPERDAY-1)
		{
		    atstmp->dn_seconds +=1;
		    atstmp->dn_nsecond = AD_32DTE_MIN_NSEC;
		}
		else if (atstmp->dn_day < 
			adu_2monthsize(atstmp->dn_month, atstmp->dn_year))
		{
		    atstmp->dn_day  +=1;
		    atstmp->dn_seconds = 0;
		    atstmp->dn_nsecond = AD_32DTE_MIN_NSEC;
		}
		else if (atstmp->dn_month < 12)
		{
		    atstmp->dn_month  +=1;
		    atstmp->dn_day  = 1;
		    atstmp->dn_seconds = 0;
		    atstmp->dn_nsecond = AD_32DTE_MIN_NSEC;
		}
		else if (atstmp->dn_year <= AD_24DTE_MAX_ABS_YR)
		{
		    atstmp->dn_year  +=1;
		    atstmp->dn_month  = 1;
		    atstmp->dn_day  = 1;
		    atstmp->dn_seconds = 0;
		    atstmp->dn_nsecond = AD_32DTE_MIN_NSEC;
		}
		else
		    incr_tmp = FALSE;
		break;
	    }

	    case DB_INYM_TYPE:
	    {
		AD_INTYM *aintym = (AD_INTYM *) tmp;

		incr_tmp = TRUE;
		if (aintym->dn_months < 12)
		    aintym->dn_months +=1;
		else if (aintym->dn_years < AD_26DTE_MAX_INT_YR)
		{
		    aintym->dn_years +=1;
		    aintym->dn_months = 1;
		}
		else
		    incr_tmp = FALSE;
		break;
	    }

	    case DB_INDS_TYPE:
	    {
		AD_INTDS *aintds = (AD_INTDS *) tmp;
     
		incr_tmp = TRUE;
		if (aintds->dn_nseconds < AD_33DTE_MAX_NSEC-1)
		    aintds->dn_nseconds +=1;
		else if (aintds->dn_seconds < AD_40DTE_SECPERDAY-1)
		{
		    aintds->dn_seconds +=1;
		    aintds->dn_nseconds = AD_32DTE_MIN_NSEC;
		}
		else if (aintds->dn_days < AD_28DTE_MAX_INT_DAY)
		{
		    aintds->dn_days +=1;
		    aintds->dn_seconds = 0;
		    aintds->dn_nseconds = AD_32DTE_MIN_NSEC;
		}
		else
		    incr_tmp = FALSE;
		break;
	    }

	    case DB_DEC_TYPE:
	    {
		/* FIX ME me incr decimal */
		break;
	    }

	    case DB_BIT_TYPE:
	    case DB_VBIT_TYPE:
	    {
		min = Bit_min;
		max = Bit_max;
		break;
	    }

	    case DB_CHR_TYPE:
	    {
		min = Chr_min;
		max = Chr_max;
		break;
	    }

	    case DB_CHA_TYPE:
	    case DB_BYTE_TYPE:
	    {
		min = Cha_min;
		max = Cha_max;
		break;
	    }

	    case DB_TXT_TYPE:
	    {
		min = Chr_min;
		max = Txt_max;
		break;
	    }

	    case DB_VCH_TYPE:
	    case DB_LTXT_TYPE:
	    case DB_VBYTE_TYPE:
	    {
		min = Vch_min;
		max = Vch_max;
		break;
	    }

	    case DB_NCHR_TYPE:
	    case DB_NVCHR_TYPE:
	    {
		/* Just increment the UCS2 until we have a better way */
		min = (uchar *)&umin;
		max = (uchar *)&umax;
		break;
	    }
	    default: break;
	}

	if (atr_bdt == DB_NCHR_TYPE || atr_bdt == DB_NVCHR_TYPE)
	    elmsize = sizeof(UCS2);
	else if (atr_bdt == DB_BYTE_TYPE || atr_bdt == DB_VBYTE_TYPE ||
			multi_byte == 0)
	    elmsize = 1;
	else /* doublebyte, character size varies */
	    elmsize = 0;

	if (min && max && elmsize != 0)
	{
	    dv.db_data = val;

	    if (status = adu_lenaddr(adf_scb, &dv, &origlen, &msp))
		return (status);

	    /* start at least significant char */
	    p = ((u_char *)msp) + (origlen - 1);

	    p_end = p;

	    for (alen = origlen; 
		    alen && (char *)p >= msp;
			p -= elmsize, alen -= elmsize)
	    {
		/* if char < max increment */
		if (elmsize == 1)
		{
		    if (*p < *max)
		    {
			*p = *p + 1;
			incr_done = TRUE;
		    }
		}
		else /* NCHR, NVCHR */
		{
		    MEcopy(p, sizeof(UCS2), &utmp);
		    if (utmp < umax)
		    {
			utmp = utmp + 1;
			MEcopy(&utmp, sizeof(UCS2), p);
			incr_done = TRUE;
		    }
		}
		if (incr_done)
		{
		    /* set all chars after this to min value */
		    for (p++;  p <= p_end; p++)
			MEcopy(min, elmsize, p);
		    break;
		}
	    }
	}
	else if (min && max && elmsize == 0)
	{
	    /* the multibyte case requires next/prev by collation */
	    continue;
	}

	/* if keyval changed, copy it into callers buf and break */
	if (incr_tmp)
	{
	    MEcopy(tmp, atr_len, val);
	    break;
	}
	else if (incr_done)
	    break;
	else
	{
	    /* adc_isminmax should have detected E_AD0118_IS_MAXIMUM!! */
	    continue;
	}
    }

    return(status);
}

/*{
** Name: adt_key_decr() - Decrement a key.
**
** Description:
**      This routine will decrement a key.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	nkatts                          Number of attributes in the key.
**	katts                           Pointer to vector of pointers.
**					Each pointer in this vector
**					will point to a DB_ATTS
**					corresponding to an attribute
**					in the tuple that helps make
**					up the key.  This vector will
**					be in "key" order.
**	klen				
**	decr_key			Pointer to key to be decremented.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**                                      Error information set in adf_errcb field
**      decr_key                        If the key can be decremented
**                                      
** Returns:
**	The following DB_STATUS codes may be returned:
**	E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	If a DB_STATUS code other than E_DB_OK is returned, the caller
**	can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	more information about the error.
**
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      04-dec-2009 (stial01) 
**          Initial creation.
*/
DB_STATUS
adt_key_decr(
ADF_CB		*adf_scb,
i4		nkatts,		/* Number attrs in key. */
DB_ATTS		**katts,	/* Ptr to vector of ptrs.
				** Each ptr in this vector
				** will point to a DB_ATTS
				** corresponding to an attribute
				** in the tuple that helps make
				** up the key.  This vector will
				** be in "key" order.
				*/
i4		klen,
char		*decr_key)	/* Ptr to the key. */
{
    DB_STATUS		status;
    DB_ATTS		*atr;
    char		*val;
#define TMPSIZE sizeof(DB_ANYTYPE) + sizeof(AD_DATENTRNL) + sizeof(AD_NEWDTNTRNL) + sizeof(ALIGN_RESTRICT)
    ALIGN_RESTRICT	tmp[TMPSIZE];
    i4			atr_bdt;	/* Base datatype of attr */
    i4			atr_len;	/* Length of current attr */
    DB_ATTS	        **cur_att;
    i4			tmplen;
    i4			chlen;
    i4			startpos;
    char		charbuf[20]; /* big enough for multi byte chars */
    bool		multi_byte = FALSE;
    DB_DATA_VALUE	ldv, chdv, dv;
    char		*msp; /* most significant */
    u_char		*p;
    u_char		*p_end;
    u_char		*null_byte;
    i4			origlen;
    u_i2		dlen; /* declared length */
    u_i2		alen; /* actual length */
    i4			pos;
    bool		decr_done = FALSE;
    bool		decr_tmp = FALSE;
    u_char		*max;
    u_char		*min;
    bool		isbyte;
    bool		ischar;
    bool		is_nchar;
    i4			elmsize;
    UCS2		umin = AD_MIN_UNICODE;
    UCS2		umax = AD_MAX_UNICODE;
    UCS2		utmp;
    UCS2		ublank = U_BLANK;
    i4			ucompare;

    status = E_DB_OK;

    if ((Adf_globs->Adi_status & ADI_DBLBYTE) ||
	(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
	multi_byte = TRUE;

    /* start at least significant, break as soon as we decr any component */
    cur_att = katts + (nkatts - 1);

    /* key not aligned */
    while ( nkatts-- > 0 && status == E_DB_OK )
    {
	atr = *(cur_att--);
	val = decr_key + atr->key_offset;
	atr_len = atr->length;
	atr_bdt = atr->type;
	min = NULL;
	max = NULL;
	isbyte = FALSE;
	ischar = FALSE;
	is_nchar = FALSE;
	elmsize = 0;

	/* Check for nullable dt */
	if ( atr_bdt < 0 )
	{
	    null_byte = (u_char *)val + atr_len - 1;
	    if (*null_byte & ADF_NVL_BIT)
	    {
		/* decrement null -> return MAX */
		status = adt_key_max(adf_scb, atr, val);
		if (status == E_DB_OK)
		{
		    *null_byte = 0;
		    return (status);
		}
		status = E_DB_OK;
		continue; /* try prev key */
	    }
	    atr_bdt = -atr_bdt;
	    atr_len--;
	}

	/* Don't know how to decrement */
	if (atr_bdt == DB_DEC_TYPE || 
		atr_bdt == DB_BIT_TYPE || atr_bdt == DB_VBIT_TYPE)
	    continue;

	/* Init a dv with no nulls, aligned numeric data */
	dv.db_datatype = atr_bdt;
	dv.db_length = atr_len;
	dv.db_data = val;
	if (atr_len <= sizeof(tmp))
	{
	    /* align the numeric datatypes */
	    MEcopy(val, atr_len, tmp);
	    dv.db_data = (PTR)&tmp[0];
	}

	status = adc_isminmax(adf_scb, &dv);
	if (status == E_DB_WARN)
	{
	    if (adf_scb->adf_errcb.ad_errcode == E_AD0117_IS_MINIMUM)
	    {
		status = E_DB_OK;
		continue; /* to prev key */
	    }
	}

	if (status == E_DB_ERROR)
	    break;

	switch (atr_bdt)
	{
	    case DB_INT_TYPE:
	    {
		decr_tmp = TRUE;
		if (atr_len == 1)
		    (*(i1 *)tmp)--;
		else if (atr_len == 2)
		    (*(i2 *)tmp)--;
		else if (atr_len == 4)
		    (*(i4 *)tmp)--;
		else if (atr_len == 8)
		    (*(i8 *)tmp)--;
		break;
	    }

	    case DB_FLT_TYPE:
	    {
		decr_tmp = TRUE;
		if (atr_len == 4)
		{
		    if (*(f4 *)tmp > 0.0)
			(*(f4 *)tmp) = (*(f4 *)tmp) * Adf_globs->Adc_hdec.fp_f4pos;
		    else if (*(f4 *)tmp < 0.0)
			(*(f4 *)tmp) = (*(f4 *)tmp) * Adf_globs->Adc_hdec.fp_f4neg;
		    else
			(*(f4 *)tmp) = Adf_globs->Adc_hdec.fp_f4nil;
		}
		else if (atr_len == 8)
		{
		    if (*(f8 *)tmp > 0.0)
			(*(f8 *)tmp) = (*(f8 *)tmp) * Adf_globs->Adc_hdec.fp_f8pos;
		    else if (*(f8 *)tmp < 0.0)
			(*(f8 *)tmp) = (*(f8 *)tmp) * Adf_globs->Adc_hdec.fp_f8neg;
		    else
			(*(f8 *)tmp) = -Adf_globs->Adc_hdec.fp_f8nil;
		}
		break;
	    }

	    case DB_MNY_TYPE:
	    {
		((AD_MONEYNTRNL *)tmp)->mny_cents--;
		decr_tmp = TRUE;
		break;
	    }

	    case DB_BOO_TYPE:
	    {
		(*(i1 *)tmp) = DB_FALSE;
		decr_tmp = TRUE;
		break;
	    }

	    case DB_DTE_TYPE:
	    {
		AD_DATENTRNL    *d = (AD_DATENTRNL *) tmp;

		decr_tmp = TRUE;
		if ((d->dn_status & AD_DN_TIMESPEC) &&
			    d->dn_time > 0)
		    d->dn_time -= 1;
		else if ((d->dn_status & AD_DN_DAYSPEC) && d->dn_lowday > 1)
		    d->dn_lowday -= 1;
		else if ((d->dn_status & AD_DN_MONTHSPEC) && d->dn_month > 1)
		{
		    d->dn_month -= 1;
		    d->dn_lowday = adu_2monthsize(d->dn_month, d->dn_year);
		}
		else if ((d->dn_status & AD_DN_YEARSPEC) && 
			    d->dn_year > AD_23DTE_MIN_ABS_YR)
		{
		    d->dn_year -= 1;
		    d->dn_month = 12;
		    d->dn_lowday = adu_2monthsize(d->dn_month, d->dn_year);
		}
		else
		    decr_tmp = FALSE;
		break;
	    }

	    case DB_ADTE_TYPE:
	    {
		AD_ADATE *ad = (AD_ADATE *) tmp;

		decr_tmp = TRUE;
		if (ad->dn_day > 1)
		    ad->dn_day -=1;
		else if (ad->dn_month > 1)
		{ 
		    ad->dn_month -=1;
		    ad->dn_day = adu_2monthsize(ad->dn_month, ad->dn_year);
		}
		else if (ad->dn_year > AD_23DTE_MIN_ABS_YR)
		{
		    ad->dn_year -=1;
		    ad->dn_month = 12;
		    ad->dn_day = adu_2monthsize(ad->dn_month, ad->dn_year);
		}
		else
		   decr_tmp = FALSE;
		break;
	    }

	    case DB_TMWO_TYPE:
	    case DB_TMW_TYPE:
	    case DB_TME_TYPE:
	    {
		AD_TIME *adtm = (AD_TIME *) tmp;

		decr_tmp = TRUE;
		if (adtm->dn_nsecond > AD_32DTE_MIN_NSEC-1)
		    adtm->dn_nsecond -=1;
		else if (adtm->dn_seconds > 0)
		    adtm->dn_seconds -=1;
		else
		    decr_tmp = FALSE;
		break;
	    }

	    case DB_TSWO_TYPE:
	    case DB_TSW_TYPE:
	    case DB_TSTMP_TYPE:
	    {
		AD_TIMESTAMP *atstmp = (AD_TIMESTAMP *) tmp;

		decr_tmp = TRUE;
		if (atstmp->dn_nsecond > AD_32DTE_MIN_NSEC-1)
		    atstmp->dn_nsecond  -=1;
		else if (atstmp->dn_seconds > 0)
		{
		    atstmp->dn_seconds -=1;
		    atstmp->dn_nsecond  = AD_33DTE_MAX_NSEC;
		}
		else if (atstmp->dn_day > 1)
		{
		    atstmp->dn_day  -=1;
		    atstmp->dn_nsecond  = AD_33DTE_MAX_NSEC;
		}
		else if (atstmp->dn_month > 1)
		{
		    atstmp->dn_month  -=1;
		    atstmp->dn_day = adu_2monthsize(atstmp->dn_month,
					atstmp->dn_year);
		    atstmp->dn_nsecond  = AD_33DTE_MAX_NSEC;
		}
		else if (atstmp->dn_year > AD_23DTE_MIN_ABS_YR)
		{
		    atstmp->dn_year  -=1;
		    atstmp->dn_month = 12;
		    atstmp->dn_day = adu_2monthsize(atstmp->dn_month,
					atstmp->dn_year);
		    atstmp->dn_nsecond  = AD_33DTE_MAX_NSEC;
		}
		else
		    decr_tmp = FALSE;
		break;
	    }

	    case DB_INYM_TYPE:
	    {
		AD_INTYM *aintym = (AD_INTYM *) tmp;

		decr_tmp = TRUE;
		if (aintym->dn_months > 1)
		    aintym->dn_months -=1;
		else if (aintym->dn_years > AD_25DTE_MIN_INT_YR)
		{
		    aintym->dn_years -=1;
		    aintym->dn_months = 12;
		}	
		else
		    decr_tmp = FALSE;
		break;
	    }

	    case DB_INDS_TYPE:
	    {
		AD_INTDS *aintds = (AD_INTDS *) tmp;
     
		decr_tmp = TRUE;
		if (aintds->dn_nseconds > AD_32DTE_MIN_NSEC)
		    aintds->dn_nseconds -=1;
		else if (aintds->dn_seconds > 0)
		{
		    aintds->dn_seconds -=1;
		    aintds->dn_nseconds  = AD_33DTE_MAX_NSEC;
		}
		else if (aintds->dn_days > AD_27DTE_MIN_INT_DAY)
		{
		    aintds->dn_days -=1;
		    aintds->dn_seconds = AD_40DTE_SECPERDAY;
		    aintds->dn_nseconds  = AD_33DTE_MAX_NSEC;
		}
		else
		    decr_tmp = FALSE;
		break;
	    }

	    case DB_DEC_TYPE:
	    {
		/* FIX ME me decrement decimal */
		break;
	    }

	    case DB_BIT_TYPE:
	    case DB_VBIT_TYPE:
	    {
		min = Bit_min;
		max = Bit_max;
		break;
	    }

	    case DB_CHR_TYPE:
	    {
		min = Chr_min;
		max = Chr_max;
		break;
	    }

	    case DB_CHA_TYPE:
	    case DB_BYTE_TYPE:
	    {
		min = Cha_min;
		max = Cha_max;
		break;
	    }

	    case DB_TXT_TYPE:
	    {
		min = Chr_min;
		max = Txt_max;
		break;
	    }

	    case DB_VCH_TYPE:
	    case DB_LTXT_TYPE:
	    case DB_VBYTE_TYPE:
	    {
		min = Vch_min;
		max = Vch_max;
		break;
	    }

	    case DB_NCHR_TYPE:
	    case DB_NVCHR_TYPE:
	    {
		/* Just decrement the UCS2 until we have a better way */
		min = (uchar *)&umin;
		max = (uchar *)&umax;
		break;
	    }

	    default: break;
	}

	if (atr_bdt == DB_NCHR_TYPE || atr_bdt == DB_NVCHR_TYPE)
	    elmsize = sizeof(UCS2);
	else if (atr_bdt == DB_BYTE_TYPE || atr_bdt == DB_VBYTE_TYPE ||
			multi_byte == 0)
	    elmsize = 1;
	else /* doublebyte, character size varies */
	    elmsize = 0;

	if (min && max && elmsize != 0)
	{
	    dv.db_data = val;

	    if (status = adu_lenaddr(adf_scb, &dv, &origlen, &msp))
		return (status);

	    /* start at least significant char */
	    p = ((u_char *)msp) + (origlen - elmsize);
	    p_end = p;

	    for (alen = origlen; 
		    alen && (char *)p >= msp;
			p -= elmsize, alen -= elmsize)
	    {
		/* if char > min decrement */
		if (elmsize == 1)
		{
		    if (*p > *min)
		    {
			*p = *p - 1;
			decr_done = TRUE;
			break;
		    }
		}
		else /* NCHR, NVCHR */
		{
		    MEcopy(p, sizeof(UCS2), &utmp);
		    status = aduucmp(adf_scb, 0, 
			&ublank, &ublank + 1, &utmp, &utmp + 1,
			&ucompare, atr->collID);
		    if (status == E_DB_OK && ucompare < 0)
		    {
			MEcopy(&ublank, sizeof(UCS2), p);
			decr_done = TRUE;
			break;
		    }
		}
	    }
	}
	else if (min && max && elmsize == 0)
	{
	    /*
	    ** the multibyte case requires next/prev by collation
	    ** For now try to decrement with a blank
	    */
	}

	/* if keyval changed, copy it into callers buf and break */
	if (decr_tmp)
	{
	    MEcopy(tmp, atr_len, val);
	    break;
	}
	else if (decr_done)
	    break;
	else
	{
	    /* adc_isminmax should have detected E_AD0117_IS_MINIMUM!! */
	    continue;
	}
    }

    return(status);
}

/*{
** Name: adt_key_max() - Init max value for some key component
**
** Description:
**      This routine will will init the max value for a key component 
**      It is called when we want to decrement a null key.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**      atr				DB_ATT for the key
**      val				key value
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**                                      Error information set in adf_errcb field
**      val                             If the max is set
**                                      
** Returns:
**	The following DB_STATUS codes may be returned:
**	E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	If a DB_STATUS code other than E_DB_OK is returned, the caller
**	can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	more information about the error.
**
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      04-dec-2009 (stial01) 
**          Initial creation.
*/
static DB_STATUS
adt_key_max(
ADF_CB		*adf_scb,
DB_ATTS	        *atr,
char		*val)
{
#define TMPSIZE sizeof(DB_ANYTYPE) + sizeof(AD_DATENTRNL) + sizeof(AD_NEWDTNTRNL) + sizeof(ALIGN_RESTRICT)
    ALIGN_RESTRICT	tmpdata[TMPSIZE];
    i4			dt;	/* Base datatype of attr */
    i4			len;	/* Length of current attr */
    u_char		*data;
    bool		tmpmax = FALSE;
    bool		datamax = FALSE;

    len = atr->length;
    dt = atr->type;
    data = val;

    if ( dt < 0 )
    {
	dt = -dt;
	len--;
    }

    if (len <= sizeof(tmpdata))
    {
	/* align the numeric datatypes */
	MEcopy(val, len, tmpdata);
	data = (u_char *)&tmpdata[0];
    }

    switch (dt)
    {
      case DB_BOO_TYPE:
	{
	    ((DB_ANYTYPE *)tmpdata)->db_booltype = DB_TRUE;
	    tmpmax = TRUE;
	    break;
	}

      case DB_FLT_TYPE:
	{
	    if (len == 4)
		*(f4 *)tmpdata = FMAX;
	    else
		*(f8 *)tmpdata = FMAX;
	    tmpmax = TRUE;
	    break;
	}

      case DB_INT_TYPE:
	{
	    if (len == 1)
		*(i1 *)tmpdata = MAXI1;
	    else if (len == 2)
		*(i2 *)tmpdata = MAXI2;
	    else if (len == 4)
		*(i4 *)tmpdata = MAXI4;
	    else if (len == 8)
		*(i8 *)tmpdata = MAXI8;
	    tmpmax = TRUE;
	    break;
	}

      case DB_CHR_TYPE:
	{
	    MEfill(len, *Chr_max, data);
	    datamax = TRUE;
	    break;
	}

      case DB_CHA_TYPE:
      case DB_NCHR_TYPE:
      case DB_BYTE_TYPE:
	{
	    MEfill(len, *Cha_max, data);
	    datamax = TRUE;
	    break;
	}

      case DB_TXT_TYPE:
	{
	    i4	tmplen = len - DB_CNTSIZE;

	    MEfill(tmplen, *Txt_max, data);
	    datamax = TRUE;
	    break;
	}

      case DB_VCH_TYPE:
      case DB_NVCHR_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
	{
	    i4	tmplen = len - DB_CNTSIZE;

	    MEfill(tmplen, *Vch_max, data);
	    datamax = TRUE;
	    break;
	}

      case DB_MNY_TYPE:
	{
	    ((AD_MONEYNTRNL *) tmpdata)->mny_cents = AD_1MNY_MAX_NTRNL;
	    tmpmax = TRUE;
	    break;
	}

      case DB_DTE_TYPE:
	{
	    AD_DATENTRNL    *d = (AD_DATENTRNL *) data;

	    d->dn_status = (AD_DN_ABSOLUTE | AD_DN_YEARSPEC | AD_DN_MONTHSPEC
			| AD_DN_DAYSPEC | AD_DN_TIMESPEC);
	    d->dn_year = AD_24DTE_MAX_ABS_YR;
	    d->dn_month = 12;
	    d->dn_highday = 0;
	    d->dn_lowday = adu_2monthsize(12,AD_24DTE_MAX_ABS_YR);
	    d->dn_time == AD_9DTE_IMSPERDAY - 1;
	    tmpmax = TRUE;
	    break;
	}

      case DB_ADTE_TYPE:
	{
	    AD_ADATE *d = (AD_ADATE *) tmpdata;

	    d->dn_year = AD_24DTE_MAX_ABS_YR;
	    d->dn_month = 12;
	    d->dn_day = adu_2monthsize(12,AD_24DTE_MAX_ABS_YR);
	    tmpmax = TRUE;
	    break;
	}

      case DB_TMWO_TYPE:
      case DB_TMW_TYPE:
      case DB_TME_TYPE:
	{
	    AD_TIME *adtm = (AD_TIME *) tmpdata;

	    adtm->dn_seconds = AD_40DTE_SECPERDAY-1;
	    adtm->dn_nsecond = AD_33DTE_MAX_NSEC-1;
	    tmpmax = TRUE;
	    break;
	}

      case DB_TSWO_TYPE:
      case DB_TSW_TYPE:
      case DB_TSTMP_TYPE:
	{
	    AD_TIMESTAMP *atstmp = (AD_TIMESTAMP *) tmpdata;

	    atstmp->dn_year = AD_24DTE_MAX_ABS_YR;
	    atstmp->dn_month = 12;
	    atstmp->dn_day = adu_2monthsize(12,AD_24DTE_MAX_ABS_YR);
	    atstmp->dn_seconds = AD_40DTE_SECPERDAY-1;
	    atstmp->dn_nsecond = AD_33DTE_MAX_NSEC;
	    tmpmax = TRUE;
	    break;
	}

      case DB_INYM_TYPE:
	{
	    AD_INTYM *aintym = (AD_INTYM *) data;

	    aintym->dn_years = AD_26DTE_MAX_INT_YR;
	    aintym->dn_months = 12;
	    tmpmax = TRUE;
	    break;
	}

      case DB_INDS_TYPE:
	{
	    AD_INTDS *aintds = (AD_INTDS *) data;

	    aintds->dn_days = AD_28DTE_MAX_INT_DAY;
	    aintds->dn_seconds = AD_40DTE_SECPERDAY-1;
	    aintds->dn_nseconds = AD_33DTE_MAX_NSEC;
	    tmpmax = TRUE;
	    break;
	}
	

      case DB_DEC_TYPE:
	{
	    break;
	}

      case DB_BIT_TYPE:
      case DB_VBIT_TYPE:
	{
	    /* Not sure about isminmax code for BIT */
	    break;
	}

      default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }   /* end case */

    if (datamax)
	return (E_DB_OK);
    else if (tmpmax)
    {
	MEcopy(tmpdata, len, val);
	return (E_DB_OK);
    }
    else
    {
	/* didn't set max */
	return (E_DB_WARN);    
    }

}
