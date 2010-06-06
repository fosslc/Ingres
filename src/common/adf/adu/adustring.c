/*
** Copyright (c) 1986, 2008, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adftrace.h>
#include    <aduint.h>
#include    <adudate.h>
#include    "adumonth.h"
#include    "adutime.h"

/**
**  ADUSTRING.C -- Routines implementing operations on the c, text, char, and
**		   varchar datatypes.
**
**  Contains procedures implementing the operators allowed on c, text, char, and
**  varchar datatypes.
**
**  The routines included in this file are:
**
**	adu_1cvrt_date() -- Convert internal time to date in form dd-mmm-yy
**	adu_1cvrt_date4() -- Convert internal time to date as per II_DATE_FORMAT
**	adu_2cvrt_time() -- Convert internal time to external time HH:MM
**			    on 24 hour clock.
**      adu_4strconcat() -- Concatenate two character strings
**      adu_6strleft() -- Gets leftmost character in string.
**      adu_7strlength() -- Gets the length of string, without trailing
**                          blanks.
**      adu_8strlocate() -- Finds the first occurrence of a string within
**			    another string.
**      adu_9strlower() -- Converts a string to lowercase.
**      adu_10strright() -- Gets rightmost character in string.
**      adu_11strshift() -- Shift a string to the left or right.
**      adu_12strsize() -- Gets the declared length of a string.
**	adu_13strtocstr() -- Converts an Ingres string type to a "C" string,
**			     null-terminated that is.
**	adu_1strtostr() -- Converts one Ingres string type to another.
**	adu_2alltobyte() -- Converts any ol' Ingres type to byte/varbyte.
**      adu_15strupper() -- Converts a string to uppercase.
**	adu_notrm() -- Converts string to string without trimming blanks.
**	adu_16strindex() -- Gets the n'th character in a string.
**      adu_17structure() -- Given the iirelation.relspec code, return the
**			     storage structure name.
**	adu_18cvrt_gmt() -- Convert internal time to date in form 
**                          "yyyy_mm_dd hh:mm:ss GMT"
**	adu_lvch_move() -- Move a long varchar string to any other string type
**      adu_19strsoundex() -- Gets the soundex code from the string.
**      adu_soundex_dm() -- Gets the Daitch-Mokotoff soundex code from a string.
**	adu_20substr	--  Substring ( char FROM int )
**	adu_21substrlen	--  Substring ( char FROM int FOR int )
**	adu_22charlength() -- Returns the ANSI char_length function value
**	adu_23octetlength() -- Returns the ANSI octet_length function value
**	adu_24bitlength() -- Returns the ANSI bit_length function value
**	adu_25position() -- Returns the position of the str1 in str2
**	adu_strextract() -- Internals for LEFT, RIGHT, CHAREXTRACT, and SUBSTRING
**    adu_strgenerate_digit() -- Generate a check digit on a string using a nominated scheme
**    adu_strvalidate_digit() -- Validate a check digit on a string using a nominated scheme

**  Function prototypes defined in ADUINT.H file.
**  
**  History:    
**      12/7/82 (kooi) -- make cnvt_date handle years >= 2000 right.
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type
**      3/9/83 (kooi)   -- fix bug in pmatch where "a* " does not match
**          "aaa".
**	23-apr-86 (ericj)
**	    Incorporated into Jupiter code.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made all routines work for char, varchar, and longtext.
**	28-jan-87 (thurston)
**	    Fixed bug in adu_8strlocate() that caused access violation whenever
**	    string2 WAS located in string1.  Bug was a faulty termination on a
**	    for loop.
**	29-jan-87 (thurston)
**	    Fixed bug in adu_12strsize() that caused size(text) to return the
**	    internal length, not the declared length.
**      25-feb-87 (thurston)
**          Made changes for using server CB instead of GLOBALDEF/REFs.
**	25-mar-87 (thurston)
**	    Fixed bug in adu_8strlocate(); now the empty string is NEVER found.
**          found.  That is, if the string to search for is the empty string,
**          adu_8strlocate() will return the size of the first string + 1. 
**	10-jun-87 (thurston)
**	    Added the adu_notrm() and adu_16strindex() routines.
**	11-jun-87 (thurston)
**	    Added the adu_17structure() routine.
**	05-aug-87 (thurston)
**	    Added code to both adu_1cvrt_date() (the `_date' function), and
**	    adu_2cvrt_time() (the `_time' function) to return an empty or
**	    blank string if the input integer was zero.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	14-jan-88 (thurston)
**          Fixed bug in adu_4strconcat() that would cause memory overwrite if
**          the combined length of the two string were greater than the max
**          possible length that could be held by the result.
**	06-jul-88 (jrb)
**	    Changes for Kanji support.
**	01-sep-88 (thurston)
**	    Increased size of local buf in adu_4strconcat() from DB_MAXSTRING
**	    to DB_GW4_MAXSTRING.
**	30-jan-1990 (fred)
**	    Added support for DB_LVCH_TYPE (long varchar).  This is a peripheral
**	    type, so support is somewhat limited at this time.
**	09-mar-92 (jrb, merged by stevet)
**	    Added a check to adu_8strlocate to make sure datatype of inputs are
**	    character string types (necessary since we use an "all type"
**	    function instance for locate()).
**      03-aug-92 (stevet)
**          Added gmt_timestamp() function.
**      29-aug-92 (stevet)
**          Added new timezone support.
**	03-sep-92 (stevet)
**	    Change output of _date to YY-MMM-DD if date format is YMD and 
**          output to MMM-DD-YY if date format is MDY.
**      14-Dec-1992 (fred)
**          Added support for long varchar function instances.
**      20-May-1993 (fred)
**          Fixed bug in adu_lvch_move() wherein coercion from
**          peripheral type to c/char type generated empty string.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      19-Aug-1993 (fred)
**          Bug fix to correctly cast large objects when necessary.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	26-feb-1996 (thoda04)
**          Corrected call to adu_error in adu_lvch_move.
**	15-mar-1998 (popri01)
**	    Correct typo/bug apparently introduced in 1993 - bad parameter
**	    in adu_size call from strlength (DOUBLE BYTE only).
**	23-oct-1998 (kitch01)
**		Bug 92735. Introduce a new function _date4().
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	20-Apr-1999 (shero03)
**	    Add substring(c FROM n [FOR l])
**	26-May-1999 (shero03)
**	    Add substring error: AD2092
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-Apr-2001 (gupsh01)
**          Added support to handle unicode data types to adu_4strconcat.
**      11-Apr-2001 (gupsh01)
**          Fixed stack overflow, by using MEreqmem to adu_4strconcat.
**	08-jun-2001 (somsa01)
**	    When allocating temporary buffers to hold strings that come
**	    in, we need to account for a maximum of
**	    DB_GW4_MAXSTRING + DB_CNTSIZE. Previously we were not
**	    accounting for DB_CNTSIZE.
**	11-jun-2001 (somsa01)
**	    In adu_4strconcat() and adu_lvch_move(), to save stack space,
**	    we now dynamically allocate space for 'temp' and 'segspace',
**	    respectively.
**      12-jun-2001 (stial01)
**	    adu_lvch_move() changes for DB_NVCHR_TYPE
**	13-dec-2001 (devjo01)
**	    In adu_4strconcat call adu_nchar_concat for unicode concats,
**	    and remove redundant code.
**	11-feb-2005 (gupsh01)
**	    Added support for coercion between char/varchar -> long varchar.
**	07-Jul-2006 (kiria01) b116095
**	    Added missing support for DB_ISO4_DFMT to adu_1cvt_date and
**	    adu_1cvrt_date4.
**      23-oct-2006 (stial01)
**          Back out Ingres2007 change to iistructure()
**      08-dec-2006 (stial01)
**          Added adu_26positionfrom() for str,byte and normal/nocollation nchrs
**      14-dec-2006 (stial01)
**          adu_2alltobyte() added varbyte(DB_LBYTE_TYPE)
**      19-dec-2006 (stial01)
**          adu_lvch_move() LNVCHR->NVCHR
**      25-jan-2007 (stial01)
**          Added support for varbyte(locators)
**	24-Apr-2007 (kiria01) b118215
**	    Created adu_strextract to unify and correct multi-byte handling
**          and behaviour of SUBSTRING(S,I,[I]), CHAREXTRACT(S,I), LEFT(S,I)
**          and RIGHT(S,I).
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	12-Mar-2009 (kiria01) SIR121788
**	    Direct the missing DT for conversion into long nvarchar
**      22-Apr-2009 (Marty Bowes) SIR 121969
**          Add adu_strgenerate_digit() and adu_strvalidate_digit()
**	11-May-2009 (kschendel) b122041
**	    Compiler caught value instead of pointer being pass to adu-error.
**      01-Aug-2009 (martin bowes) SIR122320
**          Added soundex_dm (Daitch-Mokotoff soundex)
**      03-Sep-2009 (coomi01) b122473
**          Add adu_3alltobyte as a wrapper around adu_2alltobyte allowing a
**          output length parameter to be specified.
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
**/


/*
**  Defines of other constants.
*/

#define		    SEC_PER_MIN			60
#define		    SEC_PER_DAY		     86400


/*  Static variable references	*/

static	i4	    dum1;   /* Dummy to give to ult_check_macro() */
static	i4	    dum2;   /* Dummy to give to ult_check_macro() */

/* Static function prototypes */

static DB_STATUS adu_strextract(
	ADF_CB          *adf_scb,
	DB_DATA_VALUE	*dv_src,
	i4	        startpos,
	i4              forlen,
	DB_DATA_VALUE	*rdv);

/* local defines */
#define ADU_UTF8_CASE_TOUPPER	1
#define ADU_UTF8_CASE_TOLOWER	2

static	DB_STATUS ad0_utf8_casetranslate (
	ADF_CB            *adf_scb,
	DB_DATA_VALUE     *src_dv,
	DB_DATA_VALUE     *rdv, 
	i4		  flag);

static bool soundex_dm_vowelage (
	const char *buffer,
	i4	b_ptr,
	i4	b_len,
	i4	skip);

/*{
** Name: adu_1cvrt_date() - Convert internal time to a date string in
**			    form dd-mmm-yy.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing the
**					internal time from which date is gen-
**					erated.  NOTE:  it is assumed that its
**					datatype is DB_INT_TYPE with length 4.
**	    .db_data			Ptr to an i4 containing internal time,
**					the number of seconds since Jan. 1, 1970
**	rdv				Ptr to destination DB_DATA_VALUE which
**					will hold resulting date string.
**	    .db_datatype		Its type.
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
**	rdv
**	    .db_data			Ptr to area where resulting date string
**					will be put.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	The destinations DB_DATA_VALUE was an
**					inappropriate type for this operation.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**	1-may-86 (ericj)
**	    Converted for Jupiter.  Replaced reference to read-only table, smon,
**	    to a reference to read-only table, Adu_monthtab[].
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	05-aug-87 (thurston)
**	    Added code to return an empty or blank string if the input integer
**	    was zero.
**	23-mar-91 (jrb, stevet)
**	    Bug 32951.  This routine assumed that the int parameter was always
**	    an i4; put in a switch to get the value based on db_length.
**      29-aug-92 (stevet)
**          Added new timezone support by call TMtz_search() to find the
**          correct timezone adjustments.
**	03-sep-92 (stevet)
**	    Output YYYY-MMM-DD if date format is YMD and output MMM-DD-YY 
**          if date format is MDY.
**	20-Oct-97 (islsa01)
**	    Fixed for bug# 81956.  DB_MLT4_DFMT is included in the switch state
**	    ment to support when II_DATE_FORMAT set to  MULTINATIONAL4.
**	01-aug-2006 (gupsh01)
**	    Add nanosecond parameter to adu_cvtime() call.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_1cvrt_date(
ADF_CB                     *adf_scb,
register DB_DATA_VALUE     *dv1,
register DB_DATA_VALUE     *rdv)
{
    DB_STATUS           db_stat;
    char		*p;
    struct timevect	tvect;
    i4		        value;
    i8		        i8temp;

    switch (dv1->db_length)
    {
	case 1:
	    value =  I1_CHECK_MACRO( *(i1 *) dv1->db_data);
	    break;
	case 2: 
	    value =  *(i2 *) dv1->db_data;
	    break;
	case 4:
	    value =  *(i4 *) dv1->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv1->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvrt_date value overflow"));

	    value = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvrt_date value length"));
    }

    if (value == 0)
    {
	/* Return empty or blank string */
	if (    rdv->db_datatype == DB_VCH_TYPE
	    ||  rdv->db_datatype == DB_TXT_TYPE
	    ||  rdv->db_datatype == DB_LTXT_TYPE
	   )
	{
	    ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = 0;
	}
	else
	{
	    MEfill( rdv->db_length,
		    (u_char) ' ',
		    (PTR)    rdv->db_data
		  );
	}
	return (E_DB_OK);
    }

    value += TMtz_search(adf_scb->adf_tzcb, TM_TIMETYPE_GMT, value);

    if (value < 0)
    {
	/* If negative, assume date is 31-dec-69 */
	tvect.tm_mday = 31;
	tvect.tm_mon  = 11;
	tvect.tm_year = 69;
    }
    else
    {
	adu_cvtime(value, 0, &tvect);
    }

    if ((db_stat = adu_3straddr(adf_scb, rdv, &p)) != E_DB_OK)
	return (db_stat);

    switch (adf_scb->adf_dfmt)
    {
	case DB_US_DFMT:
	case DB_MLTI_DFMT:
	case DB_FIN_DFMT:
	case DB_ISO_DFMT:
	case DB_ISO4_DFMT:
	case DB_GERM_DFMT:
	case DB_MLT4_DFMT: 
	case DB_DMY_DFMT:
	    *p++ = (tvect.tm_mday / 10) ?  (tvect.tm_mday / 10) + '0' : ' ';
	    *p++ = tvect.tm_mday % 10 + '0';
	    *p++ = '-';
	    MEcopy((PTR) Adu_monthtab[tvect.tm_mon].code, 3, (PTR) p);
	    p += 3;
	    *p++ = '-';
	    *p++ = (tvect.tm_year / 10) % 10 + '0';    /* do years >= 2000 right */
	    *p++ = tvect.tm_year % 10 + '0';
	    break;
	case DB_YMD_DFMT:
	    *p++ = (tvect.tm_year / 10) % 10 ? (tvect.tm_year / 10) % 10 + '0' : ' ';
	    *p++ = tvect.tm_year % 10 + '0';
	    *p++ = '-';
	    MEcopy((PTR) Adu_monthtab[tvect.tm_mon].code, 3, (PTR) p);
	    p += 3;
	    *p++ = '-';
	    *p++ = (tvect.tm_mday / 10) + '0';
	    *p++ = tvect.tm_mday % 10 + '0';
	    break;
	case DB_MDY_DFMT:
	    MEcopy((PTR) Adu_monthtab[tvect.tm_mon].code, 3, (PTR) p);
	    p += 3;
	    *p++ = '-';
	    *p++ = (tvect.tm_mday / 10) + '0';
	    *p++ = tvect.tm_mday % 10 + '0';
	    *p++ = '-';
	    *p++ = (tvect.tm_year / 10) % 10 + '0';
	    *p++ = tvect.tm_year % 10 + '0';
	    break;
	default:
	    return (adu_error(adf_scb, (i4) E_AD5060_DATEFMT, 2,
			(i4) sizeof (adf_scb->adf_dfmt),
			&adf_scb->adf_dfmt));
    }

    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )
        ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = 9;

    return (E_DB_OK);
}


/*{
** Name: adu_1cvrt_date4() - Convert internal time to a date string in
**			    form determined by II_DATE_FORMAT.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing the
**					internal time from which date is gen-
**					erated.  NOTE:  it is assumed that its
**					datatype is DB_INT_TYPE with length 4.
**	    .db_data			Ptr to an i4 containing internal time,
**					the number of seconds since Jan. 1, 1970
**	rdv				Ptr to destination DB_DATA_VALUE which
**					will hold resulting date string.
**	    .db_datatype		Its type.
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
**	rdv
**	    .db_data			Ptr to area where resulting date string
**					will be put.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	The destinations DB_DATA_VALUE was an
**					inappropriate type for this operation.
**
**  History:
**
**		07-sep-1998 (kitch01)
**			Created from a copy of adu_1cvrt_date(), to address bug 92735.
**			All year output will be 4 characters, day output will be 
**			2 characters, month output will will be 2 or 3 characters
**			and II_DATE_FORMAT is used to correctly position the date
**			components.
**	01-aug-2006 (gupsh01)
**	    Add nanosecond parameter to adu_cvtime() call.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_1cvrt_date4(
ADF_CB                     *adf_scb,
register DB_DATA_VALUE     *dv1,
register DB_DATA_VALUE     *rdv)
{
    DB_STATUS           db_stat;
    char		*p;
    struct timevect	tvect;
    i4		        value;
	i2				result_length;
    i8		        i8temp;

    switch (dv1->db_length)
    {
	case 1:
	    value =  I1_CHECK_MACRO( *(i1 *) dv1->db_data);
	    break;
	case 2: 
	    value =  *(i2 *) dv1->db_data;
	    break;
	case 4:
	    value =  *(i4 *) dv1->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv1->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvdate4 value overflow"));

	    value = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvdate4 value length"));
    }

    if (value == 0)
    {
	/* Return empty or blank string */
	if (    rdv->db_datatype == DB_VCH_TYPE
	    ||  rdv->db_datatype == DB_TXT_TYPE
	    ||  rdv->db_datatype == DB_LTXT_TYPE
	   )
	{
	    ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = 0;
	}
	else
	{
	    MEfill( rdv->db_length,
		    (u_char) ' ',
		    (PTR)    rdv->db_data
		  );
	}
	return (E_DB_OK);
    }

    value += TMtz_search(adf_scb->adf_tzcb, TM_TIMETYPE_GMT, value);

    if (value < 0)
    {
	/* If negative, assume date is 31-dec-69 */
	tvect.tm_mday = 31;
	tvect.tm_mon  = 11;
	tvect.tm_year = 69;
    }
    else
    {
	adu_cvtime(value, 0, &tvect);
    }

    if ((db_stat = adu_3straddr(adf_scb, rdv, &p)) != E_DB_OK)
	return (db_stat);

	/* tm_year is number of years since 1900, so add 1900 to get to 
	** current year in format of yyyy
	*/
	tvect.tm_year += 1900;

	/* In the following calculations to get the correct month 1 must be added.
	** This is because tvect.tm_mon = 0 = January 
	*/
    switch (adf_scb->adf_dfmt)
    {
	case DB_MLTI_DFMT:
	case DB_MLT4_DFMT: 
	    *p++ = (tvect.tm_mday / 10) ?  (tvect.tm_mday / 10) + '0' : '0';
	    *p++ = tvect.tm_mday % 10 + '0';
	    *p++ = '/';
	    *p++ = ((tvect.tm_mon + 1) / 10) ?  ((tvect.tm_mon + 1) / 10) + '0' : '0';
	    *p++ = (tvect.tm_mon + 1) % 10 + '0';
	    *p++ = '/';
		CVna(tvect.tm_year, p);
		result_length = 10;
	    break;
	case DB_US_DFMT:
	case DB_DMY_DFMT:
	    *p++ = (tvect.tm_mday / 10) ?  (tvect.tm_mday / 10) + '0' : '0';
	    *p++ = tvect.tm_mday % 10 + '0';
	    *p++ = '-';
	    MEcopy((PTR) Adu_monthtab[tvect.tm_mon].code, 3, (PTR) p);
	    p += 3;
	    *p++ = '-';
		CVna(tvect.tm_year, p);
		result_length = 11;
	    break;
	case DB_GERM_DFMT:
	    *p++ = (tvect.tm_mday / 10) ?  (tvect.tm_mday / 10) + '0' : '0';
	    *p++ = tvect.tm_mday % 10 + '0';
	    *p++ = '.';
	    *p++ = ((tvect.tm_mon + 1) / 10) ?  ((tvect.tm_mon + 1) / 10) + '0' : '0';
	    *p++ = (tvect.tm_mon + 1) % 10 + '0';
	    *p++ = '.';
		CVna(tvect.tm_year, p);
		result_length = 10;
	    break;
	case DB_ISO_DFMT:
	case DB_ISO4_DFMT:
		CVna(tvect.tm_year, p);
		p += STlength(p);
	    *p++ = ((tvect.tm_mon + 1) / 10) ?  ((tvect.tm_mon + 1) / 10) + '0' : '0';
	    *p++ = (tvect.tm_mon + 1) % 10 + '0';
	    *p++ = (tvect.tm_mday / 10) ?  (tvect.tm_mday / 10) + '0' : '0';
	    *p++ = tvect.tm_mday % 10 + '0';
		result_length = 8;
	    break;
	case DB_YMD_DFMT:
		CVna(tvect.tm_year, p);
		p += STlength(p);
	    *p++ = '-';
	    MEcopy((PTR) Adu_monthtab[tvect.tm_mon].code, 3, (PTR) p);
	    p += 3;
	    *p++ = '-';
	    *p++ = (tvect.tm_mday / 10) + '0';
	    *p++ = tvect.tm_mday % 10 + '0';
		result_length = 11;
	    break;
	case DB_FIN_DFMT:
		CVna(tvect.tm_year, p);
		p += STlength(p);
	    *p++ = '-';
	    *p++ = ((tvect.tm_mon + 1) / 10) ?  ((tvect.tm_mon + 1) / 10) + '0' : '0';
	    *p++ = (tvect.tm_mon + 1) % 10 + '0';
	    *p++ = '-';
	    *p++ = (tvect.tm_mday / 10) + '0';
	    *p++ = tvect.tm_mday % 10 + '0';
		result_length = 10;
	    break;
	case DB_MDY_DFMT:
	    MEcopy((PTR) Adu_monthtab[tvect.tm_mon].code, 3, (PTR) p);
	    p += 3;
	    *p++ = '-';
	    *p++ = (tvect.tm_mday / 10) + '0';
	    *p++ = tvect.tm_mday % 10 + '0';
	    *p++ = '-';
		CVna(tvect.tm_year, p);
		result_length = 11;
	    break;
	default:
	    return (adu_error(adf_scb, (i4) E_AD5060_DATEFMT, 2,
			(i4) sizeof (adf_scb->adf_dfmt),
			&adf_scb->adf_dfmt));
    }

    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )
		((DB_TEXT_STRING *) rdv->db_data)->db_t_count = result_length;

    return (E_DB_OK);
}


/*{
** Name: adu_2cvrt_time() - Convert internal time to a string in the form HH:MM
**			    on 24 hour clock.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing internal
**					time to be converted.  NOTE: it is
**					assumed that this value is of type
**					DB_INT_TYPE with a length of 4.
**	    .db_data			Ptr to an i4 containing the number of
**					seconds since Jan. 1, 1970.
**	rdv				Ptr to DB_DATA_VALUE containing a string
**					type to hold the result.
**	    .db_datatype		Its datatype.
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
**	rdv
**	    .db_data			Ptr to area to hold the result.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	The destination's DB_DATA_VALUE datatype
**					was inappropriate for this operation.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**	1-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Made work for char, varchar, and longtext.
**	05-aug-87 (thurston)
**	    Added code to return an empty or blank string if the input integer
**	    was zero.
**	23-mar-91 (jrb, stevet)
**	    Bug 32951.  This routine assumed that the int parameter was always
**	    an i4; put in a switch to get the value based on db_length.
**      29-aug-92 (stevet)
**          Added new timezone support by call TMtz_search() to find the
**          correct timezone adjustments.
**	01-aug-2006 (gupsh01)
**	    Add nanosecond parameter to adu_cvtime() call.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_2cvrt_time(
ADF_CB                     *adf_scb,
register DB_DATA_VALUE     *dv1,
register DB_DATA_VALUE     *rdv)
{
    DB_STATUS		    db_stat;
    char		    *p;
    struct timevect	    tvect;
    i4      		    value;
    i8 		            i8temp;

    switch (dv1->db_length)
    {
	case 1:
	    value =  I1_CHECK_MACRO( *(i1 *) dv1->db_data);
	    break;
	case 2: 
	    value =  *(i2 *) dv1->db_data;
	    break;
	case 4:
	    value =  *(i4 *) dv1->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv1->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvtime value overflow"));

	    value = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvtime value length"));
    }

    if (value == 0)
    {
	/* Return empty or blank string */
	if (    rdv->db_datatype == DB_VCH_TYPE
	    ||  rdv->db_datatype == DB_TXT_TYPE
	    ||  rdv->db_datatype == DB_LTXT_TYPE
	   )
	{
	    ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = 0;
	}
	else
	{
	    MEfill( rdv->db_length,
		    (u_char) ' ',
		    (PTR)    rdv->db_data
		  );
	}
	return (E_DB_OK);
    }

    value += TMtz_search(adf_scb->adf_tzcb, TM_TIMETYPE_GMT, value);

    if (value < 0)
	value += SEC_PER_DAY;	/* That line only exists to handle the case
				** where adjusting `value' to the local time
				** zone would make it negative.  That is, when
				** the input represents some time between 1/1/70
				** in Greenwich and 1/1/70 local time.
				*/
    adu_cvtime(value, 0, &tvect);

    if ((db_stat = adu_3straddr(adf_scb, rdv, &p)) != E_DB_OK)
	return (db_stat);

    *p++ = tvect.tm_hour / 10 + '0';
    *p++ = tvect.tm_hour % 10 + '0';
    *p++ = ':';
    *p++ = tvect.tm_min / 10 + '0';
    *p++ = tvect.tm_min % 10 + '0';
    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	|| rdv->db_datatype == DB_LTXT_TYPE
       )
        ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = 5;

    return (E_DB_OK);
}


/*{
** Name: adu_4strconcat() -- Concatenate two character strings.
**
** Description:
**	  adu_4strconcat takes the two character strings in dv1 and dv2
**	and concatenates them together into a new location.
**	
**	  Trailing blanks are removed from the first data value if it is a
**	 c type.  (Note that if the first data value is a c type, and it is
**	 all blanks, then one blank will be left.)  The size of the
**	 concatenation equals the sum of the two original strings.
**
**  Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first string.
**	    .db_datatype		Datatype of first string.
**	    .db_length			Length of first string.
**	    .db_data			Ptr to the data of second string.
**      dv2				Ptr to DB_DATA_VALUE for second string.
**	    .db_datatype		Datatype of second string.
**	    .db_length			Length of second string.
**	    .db_data			Ptr to the data of second string.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		The datatype of the result.
**
**  Outputs:
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
**	rdv				The result DB_DATA_VALUE.
**	    .db_data			Ptr to memory area which holds the
**					concatenated string result.
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
**	    E_AD0100_NOPRINT_CHAR	WARNING:  Unprintable characters were
**					translated to blanks.  (Only happens if
**					result type is c.
**	    E_AD2004_BAD_DTID		Datatype is unknown to ADF.
**	    E_AD5001_BAD_STRING_TYPE	One of the DB_DATA_VALUE's datatypes is
**					inappropriate for the string
**					concatenation operation.
**
**	Side Effects:
**	      If the result string is of type c, non-printing characters are
**	    converted to blanks.  If the result length of the concatenation
**	    is greater than the length of the destination DB_DATA_VALUE, it
**	    is truncated to the result length.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**      03/04/83 (lichtman) -- fixed way it was getting result length
**      09/13/83 (lichtman) -- completely re-written
**	23-apr-86 (ericj)
**	    Re-written for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	14-jan-88 (thurston)
**	    Fixed bug that would cause memory overwrite if the combined length
**	    of the two string were greater than the max possible length that
**	    could be held by the result.
**	30-jun-88 (jrb)
**	    Changes for Kanji support.
**	01-sep-88 (thurston)
**	    Increased size of local buf from DB_MAXSTRING to
**  	    DB_GW4_MAXSTRING.
**       9-Apr-1993 (fred)
**          Added byte string datatype support.
**      04-Apr-2001 (gupsh01)
**          Added support to handle unicode data types.
**	11-Apr-2001 (gupsh01)
**	    Fixed stack overflow, by using MEreqmem.
**	08-jun-2001 (somsa01)
**	    When allocating temporary buffers to hold strings that come
**	    in, we need to account for a maximum of
**	    DB_GW4_MAXSTRING + DB_CNTSIZE. Previously we were not
**	    accounting for DB_CNTSIZE.
**	11-jun-2001 (somsa01)
**	    To save stack space, we now dynamically allocate space for
**	    'temp'.
**	01-oct-2001 (gupsh01)
**	    Fixed copying of data for unicode nchar concatination.
**	13-dec-2001 (devjo01)
**	    Call adu_nchar_concat for unicode concats.
**	12-aug-02 (inkdo01)
**	    Modification to buffer alloc to impose MEreqmem overhead only 
**	    if string is bigger than 2004.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_4strconcat(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS           db_stat;
    char                *p;
    char	        *p1;
    char	        *p2;
    i4		size1;
    i4		size2;
    i4			max_size;
    char		*temp;
    STATUS		stat;
    u_char		localbuf[2004];
    bool		uselocal = TRUE;


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_4strconcat: entered with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
    }
#endif

    /* If unicode, call unicode concat routine */
    if ( rdv->db_datatype == DB_NCHR_TYPE ||
	 rdv->db_datatype == DB_NVCHR_TYPE )
    {
	db_stat = adu_nvchr_concat(adf_scb, dv1, dv2, rdv);
	return db_stat;
    }

    /* First, figure out what the max size of the result string can be */
    max_size = rdv->db_length;
    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_VBYTE_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )
	max_size -= DB_CNTSIZE;
	
    if (db_stat = adu_3straddr(adf_scb, dv1, &p1))
        return (db_stat);

    /* ... and length of 1st string */
    if (dv1->db_datatype == DB_CHR_TYPE)
    {
	/*
	** If first string is a "c" then get
	** the # chars w/o trailing blanks.
	*/
	if (db_stat = adu_size(adf_scb, dv1, &size1))
	    return (db_stat);

	/* If this "c" string consists entirely of blanks, keep one blank */
	if (size1 == 0  &&  dv1->db_length != 0)
	{
	    *p1 = ' ';	    /* in case it was a double-byte blank */
	    size1 = 1;
	}
    }
    else
    {
	/*
	** If first string is not a "c" then get
	** the # chars including trailing blanks.
	*/
	if (db_stat = adu_5strcount(adf_scb, dv1, &size1))
	    return (db_stat);
    }
    if (size1 > max_size)
	size1 = max_size;

    /* Get address and length of 2nd string ... */
    /* Always trim trailing blanks from second string if it's "c" or "char" */
    
    if (db_stat = adu_3straddr(adf_scb, dv2, &p2))
	return (db_stat);
	
    if (db_stat = adu_size(adf_scb, dv2, &size2))
	return (db_stat);
    if (size1 + size2 > max_size)
	size2 = max_size - size1;

    if (size1+size2 >= 2004)
    {
	temp = (char *)MEreqmem(0, DB_GW4_MAXSTRING + DB_CNTSIZE,
			    FALSE, &stat);
	uselocal = FALSE;
    }
    else temp = &localbuf[0];

    p = temp;
    MEcopy((PTR) p1, size1, (PTR) p);
    p += size1;
    MEcopy((PTR) p2, size2, (PTR) p);

    db_stat = adu_movestring(adf_scb, (u_char *) temp, size1 + size2, dv1->db_datatype, rdv);

    if (!uselocal)
	MEfree((PTR)temp);

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_4strconcat: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}


/*
** Name: adu_6strleft() - Gets the leftmost N characters in a string.
**
** Description:
**	  Returns the leftmost N characters as determined in dv2 of the string
**	held in dv1.  The size of the result is the same as dv1. (Note that
**	a character may be more than one byte, so the result is not necessarily
**	N bytes long.)
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE containing string to
**					extract N leftmost characters from.
**	    .db_datatype		Its datatype (must be a string type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**	dv2				DB_DATA_VALUE containing length, N.
**	    .db_datatype		Its datatype(assumed to be DB_INT_TYPE).
**	    .db_length			Its length.
**	    .db_data			Ptr to the i1, i2, or i4.
**	rdv				DB_DATA_VALUE for result.
**	    .db_datatype		Datatype of the result.
**	    .db_length			Length of the result.
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
**	rdv
**	    .db_data			Ptr to area to hold the result string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**      03/10/83 (lichtman) -- changed to get result length correctly
**	25-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	30-jun-88 (jrb)
**	    Changes for Kanji support.
**       9-Apr-1993 (fred)
**          Added byte string datatype support.
**      30-Apr-2007 (kiria01) b118215
**	    Reworked LEFT to call adu_strextract(1,n)
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_6strleft(
ADF_CB                  *adf_scb,
register DB_DATA_VALUE	*dv1, 
register DB_DATA_VALUE  *dv2,
DB_DATA_VALUE		*rdv)
{
    DB_STATUS           db_stat;
    i4			len;
    i8			i8len;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_6strleft: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
    }
#endif

    /* get the value of N, the leftmost chars wanted */
    switch (dv2->db_length)
    {	
      case 1:
	len = I1_CHECK_MACRO(*(i1 *) dv2->db_data);
	break;
      case 2:
	len = *(i2 *) dv2->db_data;
	break;
      case 4:	
	len = *(i4 *) dv2->db_data;
	break;
      case 8:	
	i8len = *(i8 *) dv2->db_data;

	 /* limit to i4 values */
	if ( i8len > MAXI4 || i8len < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strleft len overflow"));

	len = (i4) i8len;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strleft len length"));
    }

    if (len < 0)
        len = 0;
    /* Return len characters counting from the left */
    db_stat = adu_strextract(adf_scb, dv1, 1, len, rdv);
	
#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_6strleft: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}


/*{
** Name: adu_7strlength() - Returns the length of an Ingres character string.
**
** Description:
**	  This routine returns the length of a string in a DB_DATA_VALUE,
**	without trailing blanks if datatype is c or char.  For strings of type
**	text, longtext, or [long] varchar, its actual length is returned.
**	
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE, containing
**					string.
**	    .db_length			Length of string, including blanks.
**	    .db_data			Ptr to string.
**	rdv				Ptr to destination DB_DATA_VALUE.  Note,
**					it is assumed that this is of type
**					DB_INT_TYPE and has a length of 2.
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
**	rdv
**	    .db_data			Ptr to an i2 which will hold the strings
**					length.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	dv1's datatype was inappropriate for
**					this operation.
**
** History:
**	29-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	01-jul-88 (jrb)
**	    Removed code for obsolete text0 and textn types.
**	05-jul-88 (jrb)
**	    Added ifdef section for DOUBLEBYTE (kanji) support.  Kanji version
**	    must actually step through the string to determine the number of
**	    characters whereas the single-byte version need not.
**      14-Dec-1992 (fred)
**          Added long varchar support.
**       9-Apr-1993 (fred)
**          Added byte string type support.
**	15-mar-1996 (popri01)
**	    In the DOUBLE BYTE logic, length is calculated using the
**	    local variable "count". But adu_size, when called, used
**	    the tmp_size variable without updating count. In those
**	    cases, this routine always returned a length of zero (the
**	    init value of count).  So, use count instead of tmp_size.
**	09-May-2007 (gupsh01)
**	    Added support for UTF8 character sets.
**	    
*/

DB_STATUS
adu_7strlength(
ADF_CB                  *adf_scb,
register DB_DATA_VALUE	*dv1,
register DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    i4             tmp_size;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_7strlength: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
    }
#endif

    if ((Adf_globs->Adi_status & ADI_DBLBYTE) ||
        (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
    {
	char	*p = (char *)dv1->db_data;
	char	*endp;
	i4	count = 0;
	i4	size;
	i4	inter = 0;
	
	switch (dv1->db_datatype)
	{
	  case DB_CHR_TYPE:
          case DB_CHA_TYPE:
	    endp = p + dv1->db_length;
	    
	    while (p < endp)
	    {
		inter++;
		if (!CMspace(p))
		{
		    count += inter;
		    inter = 0;
 		}
		CMnext(p);
	    }
	    break;
	
	  case DB_VCH_TYPE:
	  case DB_TXT_TYPE:
	  case DB_LTXT_TYPE:
	    if (db_stat = adu_lenaddr(adf_scb, dv1, &size, &p))
		return (db_stat);
	    endp = p + size;
	    
	    while (p < endp)
	    {
		count++;
		CMnext(p);
	    }
	    break;

	  case DB_LVCH_TYPE:
	  case DB_LBYTE_TYPE:
	  {
		ADP_PERIPHERAL	    *periph = (ADP_PERIPHERAL *) dv1->db_data;

		count = periph->per_length1;
		break;
	  }

	  case DB_BYTE_TYPE:
	  case DB_VBYTE_TYPE:
	    if ((db_stat = adu_size(adf_scb, dv1, &count)) != E_DB_OK)
		return (db_stat);
	    break;
	    

	  default:
	    return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
	}

	if (rdv->db_length == 2)
	    *(i2 *)rdv->db_data = count;
	else
	    *(i4 *)rdv->db_data = count;
    }
    else
    {
        if ((db_stat = adu_size(adf_scb, dv1, &tmp_size)) != E_DB_OK)
	    return (db_stat);

	if (rdv->db_length == 2)
	    *(i2 *)rdv->db_data = tmp_size;
	else
	    *(i4 *)rdv->db_data = tmp_size;
    }

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_7strlength: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_8strlocate() - Returns the position of the first occurrence of
**			    'dv2' within 'dv1'.
**
** Description:
**	  This routine returns the position of the first occurrence of 'dv2'
**	within 'dv1'.   'dv2' can be any number of characters.  If not found, it
**	will return the size of dv1 + 1.  This retains trailing blanks
**	in the matching string.  The empty string is never found.
**
**	This routine works for all INGRES character types: c, text, char, and
**	varchar.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing search
**					string.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to the string to be searched.
**	dv2				Ptr to DB_DATA_VALUE containing string
**					pattern to be found.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string pattern.
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
**	rdv				Ptr to result DB_DATA_VALUE.  Note, it
**					is assumed that rdv's type will be
**					DB_INT_TYPE and its length will be 2.
**	    .db_data			Ptr to an i2 which holds the result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	One of the operands was not a string
**					type.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**      03/10/83 (lichtman) -- made references to cptype into calls to
**                  pstraddr and fixed result if text type
**                  and string not found.
**	29-apr-86 (ericj)
**	    Converted to Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	28-jan-87 (thurston)
**	    Fixed bug that caused access violation whenever string2 WAS located
**	    in string1.  Bug was a faulty termination on a for loop.
**	25-mar-87 (thurston)
**	    Fixed bug; now the empty string is NEVER found.  That is, if the
**	    string to search for is the empty string, this routine will return
**	    the size of the first string + 1.
**	05-jul-88 (jrb)
**	    Re-wrote for Kanji support.
**	07-dec-88 (jrb)
**	    Fixed bug introduced by Kanji changes where I was returning
**	    length+1 instead of size+1 if dv2 was not found in dv1.
**	09-mar-92 (jrb, stevet)
**	    Added a check to make sure datatype of inputs are character string
**	    types (necessary since we use an "all type" function instance for
**	    locate()).
**	12-apr-04 (inkdo01)
**	    4 byte integer result.
**      23-Mar-2010 (hanal04) Bug 122436
**          adu_8strlocate() is now called for LOCATE operations on
**          ALL datatypes and returns an appropriate error if supplied
**          with an unsupported datatype.
**          If either argument is a UNISTR (NCHR or NVCHR) we coerce
**          the other argument to NVCHR (if it was not a UNISTR) and
**          call adu_nvchr_locate(). This stop the resolver from
**          coercing p2 to varchar which was stripping trailing blanks if
**          p2 was a C or CHAR type.
*/

DB_STATUS
adu_8strlocate(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *rdv)
{
    register char   *p1,
	            *p2;
    char	    *p1save,
		    *p2save,
		    *p1end,
		    *p2end,
		    *p1last,
		    *ptemp;
    i4		    size1,
		    size2,
		    pos = 1;
    i4		    found;
    DB_DT_ID	    bdt1;
    DB_DT_ID	    bdt2;
    DB_STATUS	    db_stat;
    bool            bdt1_unistr = FALSE;
    bool            bdt2_unistr = FALSE;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_8strlocate: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
    }
#endif

    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;

    switch (bdt1)
    {
      case DB_CHR_TYPE:
      case DB_TXT_TYPE:
      case DB_CHA_TYPE:
      case DB_VCH_TYPE:
        break;

      case DB_NCHR_TYPE:
      case DB_NVCHR_TYPE:
        bdt1_unistr = TRUE;
        break;

      default:
        return(adu_error(adf_scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    switch (bdt2)
    {
      case DB_CHR_TYPE:
      case DB_TXT_TYPE:
      case DB_CHA_TYPE:
      case DB_VCH_TYPE:
        break;

      case DB_NCHR_TYPE:
      case DB_NVCHR_TYPE:
        bdt2_unistr = TRUE;
        break;

      default:
        return(adu_error(adf_scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    if(bdt1_unistr || bdt2_unistr)
    {
        DB_DATA_VALUE   cdata;
        i4		inlen;
        char		*in_string;
        DB_DATA_VALUE   *coerce_dbv = (DB_DATA_VALUE *)NULL;
        ADI_LENSPEC	lenspec;

        if(bdt1_unistr == FALSE)
        {
            coerce_dbv = dv1;
            dv1 = &cdata;
        }

        if(bdt2_unistr == FALSE)
        {
            coerce_dbv = dv2;
            dv2 = &cdata;
        }

        if(coerce_dbv)
        {
            ADI_FI_ID       cvid;
            ADI_FI_DESC     *adi_fdesc;

            MEfill(sizeof(adi_fdesc), '\0', (char *)&adi_fdesc);

            cdata.db_datatype = DB_NVCHR_TYPE;
            cdata.db_collID = coerce_dbv->db_collID;

            if ((db_stat = adi_ficoerce(adf_scb, coerce_dbv->db_datatype,
                                  cdata.db_datatype, &cvid)) != E_DB_OK)
            {
                return(db_stat);
            }

            if ((db_stat = adi_fidesc(adf_scb, cvid, &adi_fdesc)) != E_DB_OK)
            {
                return(db_stat);
            }

            if (adi_fdesc->adi_numargs > 1)
            {
                /* We're expecting a coercion routine with 1 parameter and
                ** a result. Error if we get something unexpected.
                */
                return (adu_error(adf_scb, E_AD5502_WRONG_NUM_OPRS, 0));
            }

            /* Calculate the maximum result length of the coercion */
            if ((db_stat = adi_0calclen(adf_scb, &adi_fdesc->adi_lenspec, 1,
                                  &coerce_dbv, &cdata)) != E_DB_OK)
            {
                cdata.db_length = DB_MAXSTRING + DB_CNTSIZE;
            } 

            cdata.db_data = MEreqmem (0, cdata.db_length, TRUE, &db_stat);
            if((cdata.db_data == NULL) || (db_stat != OK))
            {
                return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2,
                                  (i4) sizeof(db_stat), (i4 *)&db_stat));
            }

            /* Call the coercion routine */
            if((db_stat = adu_nvchr_coerce(adf_scb, coerce_dbv, &cdata))
                                   != E_DB_OK)
            {
                return(db_stat);
            }
            
        }

        db_stat = adu_nvchr_locate(adf_scb, dv1, dv2, rdv);

        if(coerce_dbv)
            MEfree (cdata.db_data);
        return(db_stat);
    }

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &ptemp)) != E_DB_OK)
	return (db_stat);
    p1save = ptemp;
    if ((db_stat = adu_lenaddr(adf_scb, dv2, &size2, &ptemp)) != E_DB_OK)
	return (db_stat);
    p2save = ptemp;

    found = FALSE;

    /* p1last is one past the last position we should start a search from */
    p1last = p1save + max(0, size1 - size2 + 1);
    p1end  = p1save + size1;
    p2end  = p2save + size2;

    if (size2 > 0)	/* The empty string will never be found */
    {
	while (p1save < p1last  &&  !found)
	{
	    p1 = p1save;
	    p2 = p2save;
	    
	    while (p2 < p2end && !CMcmpcase(p1, p2))
	    {
		 CMnext(p1);
		 CMnext(p2);
	    }

	    if (p2 >= p2end)
	    {
	    	found = TRUE;
	    }
	    else
	    {
		CMnext(p1save);
		pos++;
	    }
	}
    }
	    
    if (!found)	    /* if not found, make pos = size plus 1 */
    {
	pos = dv1->db_length + 1;

	if (    dv1->db_datatype == DB_VCH_TYPE
	    ||  dv1->db_datatype == DB_TXT_TYPE
	    ||  dv1->db_datatype == DB_LTXT_TYPE
	   )
	    pos -= DB_CNTSIZE;
    }

    if (rdv->db_length == 2)
	*(i2 *)rdv->db_data = pos;
    else
	*(i4 *)rdv->db_data = pos;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_8strlocate: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*
** Name: adu_9strlower() - Convert all upper case letters in an Ingres string
**			   data value to lowercase.
**
** Description:
**	  This routine converts all the upper case letters in the input Ingres
**	string data value to lowercase in the output Ingres string data value.
**      If the result type is c or char, it will be padded out with blanks.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE source.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string to be lowercased.
**	rdv				Ptr to destination DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
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
**	rdv
**	    .db_data			Ptr to area holding the resultant
**					lowercased string.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatypes were
**					not "string" compatible.
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**      03/10/83 (lichtman) -- fixed the way it gets the result size
**	29-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	05-jul-88 (jrb)
**	    Made changes for Kanji support.
**	12-feb-2007 (gupsh01)
**	    Made temporary changes to handle nchar/nvarchar.
**	16-feb-2007 (gupsh01)
**	    Previous change assumes that size1 is actually number of
**	    unicode characters but actually it is # of bytes.
**	09-mar-2007 (gupsh01)
**	    Remove the temporary fix for handling nchar/nvarchar.
**	09-may-2007 (gupsh01)
**	    Added support for UTF8 character sets.
*/

DB_STATUS
adu_9strlower(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv1,
DB_DATA_VALUE     *rdv)
{
    register i4	i;
    register char       *p;
    char                *p1,
			*psave,
			*pend;
    DB_STATUS           db_stat;
    i4			size1,
			sizer;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_9strlower: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
    }
#endif

    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
      return (ad0_utf8_casetranslate(adf_scb, dv1, rdv, ADU_UTF8_CASE_TOLOWER));

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &p1)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_3straddr(adf_scb, rdv, &psave)) != E_DB_OK)
	return (db_stat);
    p = psave;

    sizer = rdv->db_length;
    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )
	sizer -= DB_CNTSIZE;

    /* String cannot exceed size of the result data value, check it */
    size1 = min(size1, sizer);

    /* copy string into result buffer */
    MEcopy((PTR) p1, size1, (PTR) p);

    /* lowercase the characters in the result buffer */
    pend = p + size1;
    while (p < pend)
    {
	CMtolower(p, p);
	CMnext(p);
    }
    
    switch (rdv->db_datatype)
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	/* pad result with blanks */
	pend = psave + sizer;
	while (p < pend)
	    *p++ = ' ';
	break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
        ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = size1;
	break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_9strlower: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*
** Name: adu_10strright() - Gets the rightmost characters in a string.
**
** Description:
**	  Returns the rightmost dv2 characters in string dv1.
**	The size of the result is the same as dv1.  Note that trailing
**	blanks are NOT ignored.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE source.
**	    .db_datatype		Its datatype
**	    .db_length			Its length.
**	    .db_data			Ptr to string to be extracted from.
**	dv2				Ptr to DB_DATA_VALUE containing
**					number of rigthmost chars to extract.
**	    .db_length			Integer size of dv2.
**	    .db_data			Ptr to integer number.
**	rdv				Ptr to DB_DATA_VALUE destination.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
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
**	rdv
**	    .db_data			Ptr to area to hold resulting rightmost
**					characters.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD50001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**      03/10/83 (lichtman) -- changed to get result length correctly
**                  and eliminate textptype reference
**	29-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	05-jun-88 (jrb)
**	    Added ifdef section for DOUBLEBYTE (kanji) support.  Kanji version
**	    must make two passes through the string to implement this function;
**	    the single-byte version can do it by direct calculation.
**	07-dec-88 (jrb)
**	    Fixed DOUBLEBYTE problem where if number of characters requested
**	    was >= number in the string, we would return 1 fewer characters
**	    than were in the string instead of all of them.
**       9-Apr-1993 (fred)
**          Added byte string datatype support.
**      30-Apr-2007 (kiria01) b118215
**	    Reworked RIGHT to call adu_strextract(-1,n)
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_10strright(
ADF_CB              *adf_scb,
register DB_DATA_VALUE *dv1, 
register DB_DATA_VALUE *dv2,
DB_DATA_VALUE          *rdv)
{
    DB_STATUS           db_stat;
    i4			len;
    i8			i8len;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_10strright: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
    }
#endif

    /* get the value of N, the rightmost chars wanted */
    switch (dv2->db_length)
    {	
      case 1:
	len = I1_CHECK_MACRO(*(i1 *) dv2->db_data);
	break;
      case 2:
	len = *(i2 *) dv2->db_data;
	break;
      case 4:	
	len = *(i4 *) dv2->db_data;
	break;
      case 8:	
	i8len = *(i8 *) dv2->db_data;

	 /* limit to i4 values */
	if ( i8len > MAXI4 || i8len < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strright len overflow"));

	len = (i4) i8len;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strright len length"));
    }

    if (len < 0)
        len = 0;
    /* Return len characters counting from the right */
    db_stat = adu_strextract(adf_scb, dv1, -1, len, rdv);

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_10strright: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}


/*{
** Name: adu_11strshift() - Shift a character string without end around.
**
** Description:
**	  Shift the string to the right, if length is positive, leaving
**	leading blanks.  Shift to the left if negative, leaving
**	trailing blanks.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be shifted.
**      dv2				Ptr to DB_DATA_VALUE containing the 
**					number of chars to shift.
**	rdv				Ptr to the result DB_DATA_VALUE. 
**	    .db_datatype		The datatype of the result.
**	    .db_length			The length of the result string.
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
**	rdv
**	    .db_data			Ptr to the resulting shifted string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Input or Result DB_DATA_VALUES were of
**					inappropriate type.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**	25-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	06-jul-88 (jrb)
**	    Partial re-write for Kanji.  Slight performance hit (did not
**	    justify ifdefing to me) incurred with negative shift.
**       9-Apr-1993 (fred)
**          Added byte string datatype support.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_11strshift(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS           db_stat;
    register char       *pin, 
			*pout;
    char		*endpin,
			*endpout,
			*psave,
			*ptemp;
    i4                  size1,
	                sizer,
			nshift;
    i4                  char_type = TRUE;
    char                fill_character = ' ';
    i8			i8shift;
    
#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_11strshift: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
    }
#endif
    
    /* get the shifting value */
    switch (dv2->db_length)
    {	
      case 1:
        nshift = I1_CHECK_MACRO(*(i1 *) dv2->db_data);
        break;
      case 2:
        nshift = *(i2 *) dv2->db_data;
        break;
      case 4:	
        nshift = *(i4 *) dv2->db_data; /* NOTE: value should be <= MAXI2 */
        break;
      case 8:	
	i8shift = *(i8 *) dv2->db_data;

	 /* limit to i4 values */
	if ( i8shift > MAXI4 || i8shift < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strshift shift overflow"));

	nshift = (i4) i8shift;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strshift shift length"));
    }

    /* get the length and address of the string to be shifted */
    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &ptemp)) != E_DB_OK)
	return (db_stat);
    pin = ptemp;

    /* get the length and address of the result string */
    if ((db_stat = adu_3straddr(adf_scb, rdv, &psave)) != E_DB_OK)
	return (db_stat);
    pout = psave;


    sizer = rdv->db_length;
    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_VBYTE_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )
	sizer -= DB_CNTSIZE;
    
    if (   (rdv->db_datatype == DB_BYTE_TYPE)
	|| (rdv->db_datatype == DB_VBYTE_TYPE))
    {
	char_type = FALSE;
	fill_character = '\0';
    }

    endpin  = pin  + size1;
    endpout = pout + sizer;
    
    if (nshift < 0)
    {
	/* skip the start of input string since it is negative; */
	/* we have to actually step through the string because of */
	/* Kanji */

	if (char_type)
	{
	    while (nshift++ < 0  &&  pin < endpin)
		CMnext(pin);
	}
	else
	{
	    while (nshift++ < 0 && pin < endpin)
		pin++;
	}
    }
    else
    {
	/* put in leading blanks */
        nshift = min(nshift, sizer);
	while (nshift-- > 0)
            *pout++ = fill_character;
    }

    /* if anything left in input, move it over */
    if (char_type)
    {
	while (pin < endpin  &&  pout + CMbytecnt(pin) <= endpout)
	    CMcpyinc(pin, pout);
    }
    else
    {
	while (pin < endpin  &&  pout + 1 <= endpout)
	    *pout++ = *pin++;
    }

    switch (rdv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_BYTE_TYPE:
        /* pad with blanks if necessary */
        while (pout < endpout)
            *pout++ = fill_character;
        break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
        ((DB_TEXT_STRING *)rdv->db_data)->db_t_count = pout - psave;
        break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_11strshift: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_12strsize() - Returns the declared size of a string.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
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
**	rdv				Ptr to destination DB_DATA_VALUE.  Note,
**					it is assumed that its datatype is
**					DB_INT_TYPE and its length is 2.
**	    .db_data			Ptr to an i2 to hold declared size of
**					dv1.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**
**  History:
**      03/10/83 (lichtman) -- fixed to not include the length of the
**                  count on text.
**	29-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	29-jan-87 (thurston)
**	    Fixed bug that caused size(text) to return the internal length, not
**	    the declared length.
**      14-dec-1992 (fred)
**          Added support for long varchar.  Although [at present],
**          these have no declared size, they will eventually support
**          a declared size as an option.  For all long varchar's
**          created without a declared size, return 0 (internally
**          means "whatever is the appropriate size" == unknown).  In
**          the future, support returning the declared size.
**       9-Apr-1993 (fred)
**          Added byte string datatypes.
**	02-oct-2001 (gupsh01)
**	    Added support for unicode datatypes.
**	12-apr-04 (inkdo01)
**	    Support 4 byte integer result.
*/

DB_STATUS
adu_12strsize(
ADF_CB                  *adf_scb,
register DB_DATA_VALUE	*dv1,
register DB_DATA_VALUE	*rdv)
{
    i4	size = 0;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_12strsize: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
    }
#endif

    if ((dv1->db_datatype == DB_LVCH_TYPE)
	|| (dv1->db_datatype == DB_LBYTE_TYPE)
	|| (dv1->db_datatype == DB_LNVCHR_TYPE))
    {
	size = 0;
    }
    else
    {
	size = dv1->db_length;
	if (    dv1->db_datatype == DB_VCH_TYPE
	    ||  dv1->db_datatype == DB_TXT_TYPE
	    ||  dv1->db_datatype == DB_VBYTE_TYPE
	    ||  dv1->db_datatype == DB_LTXT_TYPE
	    ||	dv1->db_datatype == DB_NVCHR_TYPE
	    )
	    size -= DB_CNTSIZE;
    }

	if ((dv1->db_datatype == DB_NCHR_TYPE) 
	    || (dv1->db_datatype == DB_NVCHR_TYPE))
	{
		size /= sizeof(UCS2);
	}

    if (rdv->db_length == 2)
	*(i2 *)rdv->db_data = size;
    else
	*(i4 *)rdv->db_data = size;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_12strsize: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}



/*{
** Name: adu_13strtocstr() - Converts a string DB_DATA_VALUE to a
**			     null-terminated string.
**
** Description:
**	  Converts a string in a DB_DATA_VALUE into a null-terminated string.
**	Returns a pointer to null-terminated string for caller's convenience.
**
**	BEWARE:  DB_DATA_VALUEs of type char and varchar may have NULLs as
**		 part of their data.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	dv1				Ptr to DB_DATA_VALUE which describes
**					string to be converted.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to actual string to convert.
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
**	buf				Ptr to area holding converted string.
**					NOTE:  It is assumed that the length
**					of the buffer is greater than the length
**					of the string by at least one to allow
**					for null-termination.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    buf				(to make it convenient to use,like iocv)
**
**  History:
**      09/20/83 (lichtman) -- written
**	1-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	01-jul-88 (jrb)
**	    Removed obsolete text0 type and textn type code.
*/

char *
adu_13strtocstr(
DB_DATA_VALUE	    *dv1,
char		    *buf)
{
    u_i2	len;

    switch (dv1->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
        MEcopy((PTR) dv1->db_data, dv1->db_length, (PTR) buf);
        buf[dv1->db_length] = EOS;
        break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
        len = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	MEcopy((PTR) ((DB_TEXT_STRING *) dv1->db_data)->db_t_text, len,
	       (PTR) buf);
        buf[len] = EOS;
	break;

    }	/* end of switch */

    return (buf);
}


/*{
** Name: adu_1strtostr() - String to string conversion.
**
**  Description:
**	Used for converting c to text, text to c, c to char,
**      char to varchar, etc.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be converted.
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
**	rdv				Ptr to destination DB_DATA_VALUE for
**					converted string.
**  Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	E_AD0000_OK			Completed successfully.
**	E_AD5001_BAD_STRING_TYPE	One of the DB_DATA_VALUE operands was
**					of the wrong type for this operation.
**	E_AD0100_NOPRINT_CHAR		One or more of the non-printing chars
**					were transformed to blanks on conver-
**					sion (for text, longtext, char, or
**					varchar to c conversions).
**
**	Side Effects:
**	      If the input string is text, varchar, or char, and the output
**	    string is c, non-printing characters will be converted to blanks.
**          Also, if the the source string is longer than the destination
**          string, it will be truncated to the length of the destination
**          string. 
**
**  History:
**      02/23/83    -- Written by Jeff Lichtman
**      03/04/83    -- changed to take length of count on output
**                  string into account when figuring output length.
**	1-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	30-mar-2001 (gupsh01)
**	    Added support for unicode strings.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_1strtostr(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    char	*dv1_str_addr;
    UCS2        *dv1_uni_str_addr;
    i4	dv1_len;
    DB_STATUS   db_stat;

 if ( (dv1->db_datatype == DB_NVCHR_TYPE) ||
         (dv1->db_datatype == DB_NCHR_TYPE) )
 { 
    if ((db_stat = adu_7straddr(adf_scb, dv1, &dv1_uni_str_addr)) != E_DB_OK)
	return (db_stat);

    if ((db_stat = adu_size(adf_scb, dv1, &dv1_len)) != E_DB_OK)
	return (db_stat);

    return (adu_moveunistring(adf_scb, (UCS2 *) dv1_uni_str_addr, dv1_len, rdv));
 }
 else
 {	
    if ((db_stat = adu_3straddr(adf_scb, dv1, &dv1_str_addr)) != E_DB_OK)
        return (db_stat);

    if ((db_stat = adu_size(adf_scb, dv1, &dv1_len)) != E_DB_OK)
        return (db_stat);
       
    return (adu_movestring(adf_scb, (u_char *) dv1_str_addr, dv1_len, dv1->db_datatype, rdv));
 }
}


/*{
** Name: adu_2alltobyte() - Any type to byte/varbyte conversion
**
**  Description:
**	Used for converting miscellaneous types (int, float, etc.)
**      to byte/varbyte. String types are already converted using adu_1strtostr
**	and adu_ascii, so this just mops up the rest.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be converted.
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
**	rdv				Ptr to destination DB_DATA_VALUE for
**					converted string.
**  Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	E_AD0000_OK			Completed successfully.
**	E_AD5001_BAD_STRING_TYPE	One of the DB_DATA_VALUE operands was
**					of the wrong type for this operation.
**
**	Side Effects:
**	    none.
**
**  History:
**      10-oct-97 (inkdo01)
**	    Written.
**	13-nov-01 (inkdo01)
**	    Added special case code to handle "byte(nnn)" where nnn is int2 and
**	    high order byte is 0. Clearly users would want this to be a single 
**	    byte assign of the low order byte, whereas it actually assigns
**	    the 0 byte first.
**	3-dec-01 (inkdo01)
**	    Minor fix to previous change because some dunderhead in the pre-history
**	    of Ingres decided that BYTE_SWAP is for non-byte swapped machines.
**	17-jul-2006 (gupsh01)
**	    Added support for ANSI datetime datatypes to adu_2alltobyte.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
**	17-Mar-2009 (kiria01) SIR121788
**	    Add missing LNLOC support and let adu_lvch_move
**	    couponify the locators.
**	18-May-2010 (kiria01) b123442
**	    Added missing DB_LTXT_TYPE.
*/

DB_STATUS
adu_2alltobyte(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    char	*dv1_str_addr;
    i4	dv1_len;
    DB_STATUS   db_stat;

    /* Establish addr of source value */
    switch (dv1->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_BYTE_TYPE:
      case DB_VCH_TYPE:
      case DB_VBYTE_TYPE:
      case DB_LTXT_TYPE:
	return (adu_1strtostr(adf_scb, dv1, rdv));
	break;

      case DB_INT_TYPE:
      case DB_FLT_TYPE:
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
      case DB_MNY_TYPE:
      case DB_DEC_TYPE:
      case DB_TXT_TYPE:
      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	dv1_str_addr = (char *)dv1->db_data;
	break;

      case DB_LVCH_TYPE:
      case DB_LBYTE_TYPE:
      case DB_LNVCHR_TYPE:
      case DB_LCLOC_TYPE:
      case DB_LBLOC_TYPE:
      case DB_LNLOC_TYPE:
	return ( adu_lvch_move(adf_scb, dv1, rdv));
	break;
      default:
	return(adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    /* Establish size of source value */
    switch (dv1->db_datatype)
    {
      case DB_INT_TYPE:
#ifdef BIG_ENDIAN_INT
	dv1_len = dv1->db_length;
	if (dv1_len == 2)
	{
	    /* "byte(nnn)" should only produce a single byte if the 
	    ** high order byte is 0. Otherwise, assignment of byte(nnn)
	    ** to a byte(1) column incorrectly assigns 0. This is treated
	    ** as a special case because of the way the parser handles
	    ** "byte(nnn)".
	    **
	    ** Note: this adjustment is only required for non-byte swapped
	    ** machines. */
	    if (*(i2 *)dv1_str_addr <= 255 && *(i2 *)dv1_str_addr >= 0)
	    {
		dv1_len--;
		dv1_str_addr = dv1_str_addr+1;
	    }
	}
	break;
#endif
      case DB_FLT_TYPE:
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
      case DB_MNY_TYPE:
      case DB_DEC_TYPE:
      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	dv1_len = dv1->db_length;
	break;
      case DB_TXT_TYPE:
	dv1_len = ((DB_TEXT_STRING *)dv1->db_data)->db_t_count+2;
	break;
      default:
	return(adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }
    return (adu_movestring(adf_scb, (u_char *) dv1_str_addr, dv1_len, dv1->db_datatype, rdv));
}

/*{
** Name: adu_3alltobyte() - Any type to byte/varbyte conversion
**
**  Description:
**	A wrapper routine to limit the result lenght of the adu_2alltobyte() above
**      is used to capture the 'all' version of the Byte(expr,len) function call.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be converted.
**      dv2                             Ptr to DB_INT_VALUE containing desired
**					length of the output data.
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
**	rdv				Ptr to destination DB_DATA_VALUE for
**					converted string.
**  Returns:
**	    The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**          as per adu_2alltobyte()
**
**	    If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	E_AD0000_OK			Completed successfully.
**	E_AD5001_BAD_STRING_TYPE	One of the DB_DATA_VALUE operands was
**					of the wrong type for this operation.
**
**	Side Effects:
**	    none.
**
**  03-Sep-09 (coomi01) b122473 
**      First written.
*/
DB_STATUS
adu_3alltobyte(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE	    *rdv)
{
    i4 r4len;
    i8 r8len;

    /* 
    ** Read the result length as appropriate
    */
    switch( dv2->db_length)
    {
        case 1:
            r4len = I1_CHECK_MACRO(*(i1 *)dv2->db_data);
            break;

        case 2:
            r4len = *(i2 *)dv2->db_data;
            break;

        case 4:
            r4len = *(i4 *)dv2->db_data;
            break;

        case 8:
            r8len = *(i8 *)dv2->db_data;
	    /* limit to i4 values */
	    if ( r8len > MAXI4 || r8len < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "byte value overflow"));

            /* Safe to cast downwards */
            r4len = (i4)r8len;
            break;

        default:
            return (adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
    }


    /*
    ** Check the bounds of len specifier 
    */
    if ((r4len < 1) || (r4len > DB_MAXSTRING))
    {
        return (adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
    }

    /* Set it */
    rdv->db_length = r4len;

    /* Now call the worker routine */
    return adu_2alltobyte(adf_scb, dv1, rdv);
}


/*{
** Name: adu_15strupper() - Convert all lower case letters in an Ingres string
**			    data value to uppercase.
**
** Description:
**	  This routine converts all lowercase letters in a source DB_DATA_VALUE
**	string to uppercase letters in a destination DB_DATA_VALUE.  If the
**	destination DB_DATA_VALUE is of type c or char, it is padded with
**	blanks.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to source string to be uppercased.
**	rdv				Ptr to destination DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
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
**	rdv
**	    .db_data			Ptr to area holding resultant uppercased
**					string.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype were
**					incompatible with this operation.
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.
**      03/10/83 (lichtman) -- fixed the way it got the result length
**                  and eliminated textptype reference
**	29-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	06-jul-88 (jrb)
**	    Modified for Kanji support.
**	09-may-2007 (gupsh01)
**	    Added support for UTF8 character sets. 
*/

DB_STATUS
adu_15strupper(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv1,
DB_DATA_VALUE     *rdv)
{
    register i4	i;
    register char       *p;
    char                *p1,
			*psave,
			*pend;
    DB_STATUS           db_stat;
    i4			size1,
			sizer;
#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_9strupper: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
    }
#endif

    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
      return (ad0_utf8_casetranslate(adf_scb, dv1, rdv, ADU_UTF8_CASE_TOUPPER));

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &p1)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_3straddr(adf_scb, rdv, &psave)) != E_DB_OK)
	return (db_stat);
    p = psave;

    sizer = rdv->db_length;
    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )
	sizer -= DB_CNTSIZE;

    /* String cannot exceed size of the result data value, check it */
    size1 = min(size1, sizer);

    /* copy string into result buffer */
    MEcopy((PTR) p1, size1, (PTR) p);

    /* uppercase the characters in the result buffer */
    pend = p + size1;
    while (p < pend)
    {
	CMtoupper(p, p);
	CMnext(p);
    }
    
    switch (rdv->db_datatype)
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	/* pad result with blanks */
	pend = psave + sizer;
	while (p < pend)
	    *p++ = ' ';
	break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
        ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = size1;
	break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_9strupper: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_notrm() - Convert any string type to any string type without
**		       trimming blanks.
**
** Description:
**	  This routine converts any string DB_DATA_VALUE into any other string
**	DB_DATA_VALUE without trimming blanks.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to source string to be uppercased.
**	rdv				Ptr to destination DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
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
**	rdv
**	    .db_data			Ptr to string holding result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype were
**					incompatible with this operation.
**
**  History:
**	10-jun-87 (thurston)
**	    Initial creation.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_notrm(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    char	*s;
    i4		slen;
    DB_STATUS   db_stat;

    if ((db_stat = adu_3straddr(adf_scb, dv1, &s)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_5strcount(adf_scb, dv1, &slen)) != E_DB_OK)
	return (db_stat);

    return (adu_movestring(adf_scb, (u_char *) s, slen, dv1->db_datatype, rdv));
}


/*{
** Name: adu_xyzzy() - Magic function.
**
** Description:
**	  Do you believe in magic?
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to data.
**	rdv				Ptr to destination DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
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
**	rdv
**	    .db_data			Ptr to string holding result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype were
**					incompatible with this operation.
**
**  History:
**	03-aug-87 (thurston)
**	    Initial creation.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_xyzzy(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    if (dv1->db_datatype == DB_VCH_TYPE && dv1->db_length == 5 &&
	STbcompare("Wim", 3, (char *)(dv1->db_data+2), 3, 1) == 0 ||
	dv1->db_datatype == DB_CHA_TYPE && dv1->db_length == 3 &&
	STbcompare("Wim", 3, dv1->db_data, 3, 1) == 0)
    return (adu_movestring(adf_scb,(u_char *)"Nothing happens to Wim.", (i4)23, dv1->db_datatype, rdv));
    else return (adu_movestring(adf_scb,(u_char *)"Nothing happens.", (i4)16, dv1->db_datatype, rdv));
}


/*{
** Name: adu_16strindex() - Gets the n'th byte in a string.
**
** Description:
**	  This routine returns the n'th byte in the input string.  If n
**	is out of range of the input string, a blank will be returned.  This
**	routine assumes the result is a char type with length one.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                          Ptr to string DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string.
**      dv2                          Ptr to integer DB_DATA_VALUE.
**	    .db_datatype		Its type (assumed to be DB_INT_TYPE).
**	    .db_length			Its length.
**	    .db_data			Ptr to the integer.  (This integer is
**					refered to as, `n'.)
**	rdv				Ptr to destination DB_DATA_VALUE.
**	    .db_datatype		Its type (assumed to be DB_CHA_TYPE).
**	    .db_length			Its length (assumed to be 1).
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
**	rdv
**	    .db_data			Ptr to string holding the byte.
**					If `n' is non-positive, or greater than
**					the length of the string, a blank will
**					be the result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	dv_str's datatype was not a string type.
**
**  History:
**	10-jun-87 (thurston)
**	    Initial creation.
**	07-jul-88 (jrb)
**	    Changed comments to say that this routine returns a "byte" instead
**	    of a "character."  This is because a one byte result was assumed,
**	    but now we have double-byte characters.
**       9-Apr-1993 (fred)
**          Added byte string datatype support.  This only entails
**          looking for byte types, and returning a 0 instead of a blank.
**      30-Apr-2007 (kiria01) b118215
**	    Reworked CHAREXTRACT to call adu_strextract(n,1)
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_16strindex(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS           db_stat;
    i4			startpos;
    i8			i8pos;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_16strindex: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
    }
#endif

    /* get the value of N, the index of the char wanted */
    switch (dv2->db_length)
    {	
      case 1:
        startpos = I1_CHECK_MACRO(*(i1 *) dv2->db_data);
        break;
      case 2:
        startpos = *(i2 *) dv2->db_data;
        break;
      case 4:	
        startpos = *(i4 *) dv2->db_data;
        break;
      case 8:	
	i8pos = *(i8 *) dv2->db_data;

	/* limit to i4 values */
	if ( i8pos > MAXI4 || i8pos < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strindex start overflow"));

	startpos = (i4) i8pos;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "strindex start length"));
    }

    if (startpos < 0)
        startpos = 0;
    /* Return 1 characters counting from char startpos - if -ve return null */
    db_stat = adu_strextract(adf_scb, dv1, startpos, startpos ? 1 : 0, rdv);

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_16strindex: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}


/*{
** Name: adu_17structure() - Given the iirelation.relspec code
**			     return the storage structure name.
**
** Description:
**	  This routine returns the storage structure name (one of `HEAP',
**	`HASH', `ISAM', `BTREE') 
**	given the iirelation.relspec code.
**	It assumes the first input is `i', and the output is a string type.
**	If the input integer is not a valid storage structure, then the empty
**	string is returned.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to int DB_DATA_VALUE representing
**					the iirelation.relspec code.
**	    .db_datatype		Its type (assumed to be DB_INT_TYPE).
**	    .db_length			Its length.
**	    .db_data			Ptr to the integer.
**	rdv				Ptr to destination DB_DATA_VALUE.
**	    .db_datatype		Its type (assumed to be a string type).
**	    .db_length			Its length.
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
**	rdv
**	    .db_data			Ptr to string holding the storage
**					structure name, or empty string if
**					input was invalid.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	rdv's datatype was not a string type.
**
**  History:
**	11-jun-87 (thurston)
**	    Initial creation.
**	08-may-1996 (shero03)
**	    Added Rtree
**	30-May-2006 (jenjo02)
**	    Added second input parm, iirelation.relstat, to check
**	    the "clustered" bit.
**	27-Apr-2007 (kiria01) b118215
**	    Corrected a potential NULL-dereference.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_17structure(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1, 
DB_DATA_VALUE	    *rdv)
{
    char	*s;
    i4		slen;
    i4		relspec;
    i4		relstat;
    i8	        i8temp;

    switch (dv1->db_length)
    {
	case 1:
	    relspec =  I1_CHECK_MACRO( *(i1 *) dv1->db_data);
	    break;
	case 2: 
	    relspec =  *(i2 *) dv1->db_data;
	    break;
	case 4:
	    relspec =  *(i4 *) dv1->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv1->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "struct relspec overflow"));

	    relspec = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "struct relspec length"));
    }

    switch (relspec)
    {
      case 3:
	s = "HEAP";
	slen = 4;
	break;
      case 5:
	s = "ISAM";
	slen = 4;
	break;
      case 7:
	s = "HASH";
	slen = 4;
	break;
      case 11:
	s = "BTREE";
	slen = 5;
	break;
      case 13:
	s = "RTREE";
	slen = 5;
	break;
      default:
	s = "<bad relspec>";
	slen = 13;
    }

    return (adu_movestring(adf_scb, (u_char *) s, slen, dv1->db_datatype, rdv));
}

/*{
** Name: adu_18cvrt_gmt() - Convert internal time to a date string in
**			    the form "yyyy_mm_dd hh:mm:ss GMT".
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing the
**					internal time from which date is gen-
**					erated.  NOTE:  it is assumed that its
**					datatype is DB_INT_TYPE with length 4.
**	    .db_data			Ptr to an i4 containing internal time,
**					the number of seconds since Jan. 1, 1970
**	rdv				Ptr to destination DB_DATA_VALUE which
**					will hold resulting date string.
**	    .db_datatype		Its type.
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
**	rdv
**	    .db_data			Ptr to area where resulting date string
**					will be put.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD9999_INTERNAL_ERROR     The destinations DB_DATA_VALUE was an
**					inappropriate type for this operation.
**
**  History:
**      03-aug-92 (stevet) -- Created.
**	01-aug-2006 (gupsh01)
**	    Add nanosecond parameter to adu_cvtime() call.
**	07-nov-2006 (gupsh01)
**	    Fixed seconds calculations.
**	14-nov-2006 (gupsh01)
**	    Revert previous change as the date used is 
**	    ingresdate.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_18cvrt_gmt(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv1,
DB_DATA_VALUE     *rdv)
{
    DB_STATUS           db_stat;
    DB_DATA_VALUE       dv;
    AD_DATENTRNL        d;
    char		*p;
    struct timevect	tvect;
    i4		sec_time_zone;
    i4		value;
    i8		        i8temp;

    switch (dv1->db_length)
    {
	case 1:
	    value =  I1_CHECK_MACRO( *(i1 *) dv1->db_data);
	    break;
	case 2: 
	    value =  *(i2 *) dv1->db_data;
	    break;
	case 4:
	    value =  *(i4 *) dv1->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv1->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvgmt value overflow"));

	    value = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cvgmt value length"));
    }

    if (value <= 0)
    {
	/* If negative or zero, assume date is 1-jan-70 */
	tvect.tm_mday = 1;
	tvect.tm_mon  = 0;
	tvect.tm_year = 70;
	tvect.tm_hour = 0;
	tvect.tm_min  = 0;
	tvect.tm_sec  = 0;
	tvect.tm_nsec  = 0;
    }
    else
    {
	adu_cvtime(value, 0, &tvect);
    }

    d.dn_status  = AD_DN_ABSOLUTE | AD_DN_TIMESPEC; 
    d.dn_year    = tvect.tm_year + AD_TV_NORMYR_BASE;
    d.dn_month   = tvect.tm_mon + AD_TV_NORMMON;
    d.dn_highday = 0;
    d.dn_lowday  = tvect.tm_mday;
    d.dn_time    = tvect.tm_hour * AD_8DTE_IMSPERHOUR +
	           tvect.tm_min  * AD_7DTE_IMSPERMIN +
		   tvect.tm_sec  * AD_6DTE_IMSPERSEC;
    dv.db_length = sizeof(d);
    dv.db_datatype = DB_DTE_TYPE;
    dv.db_prec   = 0;
    dv.db_data   = (PTR)&d;
    
    return(adu_dgmt(adf_scb, &dv, rdv));

}

/*
** {
** Name: adu_lvch_move	- Move to/from long string elements
**
** Description:
**      This routine moves data to or from long string elements.  These
**	elements are different from `normal' string handling, because long
**	implies a peripheral datatype.  This requires different handling.
**
**	For most string datatypes, there can be only a single operation, since
**	any other string datatype will fit into memory, and long varchar will
**	not.  However, if this is a long varchar to long varchar move, then this
**	is a more complicated operation. 
**
**	    NOTE: FI_DEFN using this function should use:
**		lenspec ADI_FIXED sizeof(ADP_PERIPHERAL)
**
** Inputs:
**      adf_scb                         ADF's session control block
**      dv_in                           Ptr to DB_DATA_VALUE for original string
**      dv_out                          Ptr to DB_DATA_VALUE for output
**
** Outputs:
**      dv_out                          Filled appropriately
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Jan-1990 (fred)
**          Created.
**       9-Apr-1993 (fred)
**          Made more generic so that it will correctly support long
**          byte as well as long varchar.
**      20-May-1993 (fred)
**          Altered so that non-string tagged datatypes (c/char/byte)
**          will work correctly as output.
**      19-Aug-1993 (fred)
**          Bug fix to correctly cast large objects when necessary.
**      12-Oct-1993 (fred)
**          Remove call to ADP_INFORMATION -- subsumed in
**          adi_per_under().
**      26-feb-1996 (thoda04)
**          Corrected call to adu_error.  First parm is always ADF_CB.
**	13-Oct-1998 (thaju02)
**	    copy from file into table with blob column resulted in server
**	    SEGV. MEfill'd dv_out coupon. (B93844)
**      02-Feb-1999 (stial01)
**          adu_lvch_move() If we need to coerce DB_VCH_TYPE to DB_BYTE_TYPE
**          don't truncate the length.
**	23-feb-99 (stephenb)
**	    Init new pop_cb field pop_info
**	08-jun-2001 (somsa01)
**	    When allocating temporary buffers to hold strings that come
**	    in, we need to account for a maximum of
**	    DB_GW4_MAXSTRING + DB_CNTSIZE. Previously we were not
**	    accounting for DB_CNTSIZE.
**	11-jun-2001 (somsa01)
**	    To save stack space, we now dynamically allocate space for
**	    'segspace'.
**	11-May-2004 (schka24)
**	    Name change for temp indicator.
**	11-feb-2005 (gupsh01)
**	    Added support for coercion between char/varchar -> long varchar.
**	05-feb-2007 (gupsh01)
**	    Fixed the coercion between char/varchar and long varchar cases.
**	13-feb-2007 (dougi)
**	    And from long text to long nvarchar.
**	14-Feb-2008 (kiria01) b119902
**	    Add support for LONG NVARCHAR->NCHAR, LONG VARCHAR->NCHAR
**	    and LONG VARCHAR->NVARCHAR
**	    Dropped LVARCHAR from non-peripheral context
**	05-Jun-2008 (kiria01) b119210
**	    Don't limit the db_length on transfers into LONG. Rather allow
**	    max allocated size of intermediate transfers that require
**	    coercion. The loop that follows breaks the buffer up into under_dv
**	    size chuncks anyway though this too might not be be needed now.
**	21-Mar-2009 (kiria01) SIR121788
**	    Allow LBYTE <-> LVCH with no translation and generate
**	    an error if datatypes still come through differing -
**	    they should be handled by adu_long_coerce.
**	15-Apr-2009 (kiria01) SIR121788
**	    Drop back from LBYTE <-> LVCH as conversion is needed after all.
**	25-Apr-2009 (kiria01) SIR121788
**	    Re-add LBYTE <-> LVCH with no translation as there are benefits
**	    in performance.
*/
DB_STATUS
adu_lvch_move(
ADF_CB             *adf_scb,
DB_DATA_VALUE	   *dv_in,
DB_DATA_VALUE	   *dv_out)
{
    DB_STATUS           status;
    ADP_POP_CB		pop_cb;
    DB_DATA_VALUE	segment_dv;
    DB_DATA_VALUE	under_dv;
    DB_DATA_VALUE	cpn_dv;
    DB_DATA_VALUE	loc_dv;
    ADP_PERIPHERAL	cpn;
    ADP_PERIPHERAL	*p;
    ADF_AG_STRUCT	work;
    i4			length;
    i4                  inbits;
    i4                  outbits;
    char		*segspace = NULL;
    i4			done;
    i4			bytes;

    if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi == NULL)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "lvch fexi null"));

    for (;;)
    {
	status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits);
	if (status)
	    break;
	if (inbits & AD_LOCATOR)
	{
	    loc_dv = *dv_in;
	    loc_dv.db_length = sizeof(ADP_PERIPHERAL);
	    loc_dv.db_data = (PTR)&cpn;
	    loc_dv.db_datatype = abs(dv_in->db_datatype)==DB_LCLOC_TYPE
				? DB_LVCH_TYPE
				: abs(dv_in->db_datatype)==DB_LNLOC_TYPE
				? DB_LNVCHR_TYPE : DB_LBYTE_TYPE;
	    if (status = adu_locator_to_cpn(adf_scb, dv_in, &loc_dv))
		break;
	    dv_in = &loc_dv;
	    if (status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits))
		break;
	}
	status = adi_dtinfo(adf_scb, dv_out->db_datatype, &outbits);
	if (status)
	    break;

	if (inbits & AD_PERIPHERAL)
	{
	    status = adi_per_under(adf_scb,
				   dv_in->db_datatype,
				   &under_dv);
	    cpn_dv.db_datatype = dv_in->db_datatype;
	}
	else
	{
	    status = adi_per_under(adf_scb,
				   dv_out->db_datatype,
				   &under_dv);
	    cpn_dv.db_datatype = dv_out->db_datatype;
	}
	if (status)
	    break;

	pop_cb.pop_length = sizeof(pop_cb);
	pop_cb.pop_type = ADP_POP_TYPE;
	pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
	/* This is irrelevant if the output side isn't long */ 
	pop_cb.pop_temporary = ADP_POP_TEMPORARY;
	pop_cb.pop_underdv = &under_dv;
	    under_dv.db_data = 0;
	pop_cb.pop_segment = &segment_dv;
	    STRUCT_ASSIGN_MACRO(under_dv, segment_dv);
	pop_cb.pop_coupon = &cpn_dv;
	    cpn_dv.db_length = sizeof(ADP_PERIPHERAL);
	    cpn_dv.db_prec = 0;
	    cpn_dv.db_data = (PTR) 0;
	pop_cb.pop_info = 0;

	if (segspace == NULL)
	{
	    segspace = (char *)MEreqmem(0, DB_GW4_MAXSTRING + DB_CNTSIZE,
					FALSE, NULL);
	}
	work.adf_agwork.db_data = segspace;
	
	if ((inbits & AD_PERIPHERAL) == 0)
	{
	    /* not-long --> long */

	    p = (ADP_PERIPHERAL *) dv_out->db_data;
	    p->per_tag = ADP_P_COUPON;
	    cpn_dv.db_data = dv_out->db_data;
	    MEfill(sizeof(p->per_value.val_coupon), 0, 
			(char *)&(p->per_value.val_coupon));

	    work.adf_agwork.db_length = DB_GW4_MAXSTRING + DB_CNTSIZE;
	    work.adf_agwork.db_datatype = under_dv.db_datatype;
	    work.adf_agwork.db_prec = under_dv.db_prec;

	    if (under_dv.db_datatype != dv_in->db_datatype)
	    {
		/* Then we have another coercion to perform.  do that first */

		switch (under_dv.db_datatype)
		{
		  case DB_NVCHR_TYPE:
		  {
		    switch (dv_in->db_datatype)
		    {
			case DB_BYTE_TYPE:	
			case DB_VBYTE_TYPE:	
			case DB_LTXT_TYPE:	
		  	  status = adu_1strtostr(adf_scb, dv_in, &work.adf_agwork);
			  break;

			case DB_NCHR_TYPE:	
			case DB_CHA_TYPE:	
			case DB_CHR_TYPE:	
			case DB_TXT_TYPE:	
			case DB_VCH_TYPE:	
		  	  status = adu_nvchr_coerce(adf_scb, dv_in, &work.adf_agwork);
			  break;
			
			case DB_UTF8_TYPE:	
		  	  status = adu_nvchr_toutf8(adf_scb, dv_in, &work.adf_agwork);
		 	  break;

			default:
			  if (segspace)
		    	    MEfree((PTR)segspace);
			  return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2,
					0, "lvch in-under type"));
		    }
		  }
		  break;

		  case DB_VCH_TYPE:
		  {
		    switch (dv_in->db_datatype)
		    {
			case DB_NCHR_TYPE:	
			case DB_NVCHR_TYPE:	
		  	  status = adu_nvchr_coerce(adf_scb, dv_in, &work.adf_agwork);
			  break;
			
			default:
		          status = adu_1strtostr(adf_scb, dv_in, &work.adf_agwork);
		    }
		  }
		  break;

		  default:
		    status = adu_1strtostr(adf_scb, dv_in, &work.adf_agwork);
		}


		if (status)
		{
		  if (segspace)
		      MEfree((PTR)segspace);
		  return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "lvch in-under cv"));
		  break;
		}
	    }
	    else
	    {
		MECOPY_VAR_MACRO((PTR) dv_in->db_data,
				    (u_i4)dv_in->db_length,
				    (PTR) work.adf_agwork.db_data);
		work.adf_agwork.db_length = dv_in->db_length;
		work.adf_agwork.db_prec = dv_in->db_prec;
	    }
	    dv_in = &work.adf_agwork;

	    pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
	    segment_dv.db_data = dv_in->db_data;
	    p->per_length0 = 0;
	    p->per_length1 =
		    ((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count;

	    do
	    {
		length = ((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count;
		if (segment_dv.db_datatype == DB_NVCHR_TYPE)
		    bytes = length * sizeof(UCS2);
		else
		    bytes = length;
		
		if ((bytes + 2) > under_dv.db_length)
		{
		    segment_dv.db_length = under_dv.db_length;
		    if (segment_dv.db_datatype == DB_NVCHR_TYPE)
			((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count =
				    (under_dv.db_length - 2) / sizeof(UCS2);
		    else
			((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count =
				    under_dv.db_length - 2;
		}
		else
		{
		    segment_dv.db_length = bytes + 2;
		    pop_cb.pop_continuation |= ADP_C_END_MASK;
		}
		status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
				    ADP_PUT, &pop_cb);
		pop_cb.pop_continuation &= ~ADP_C_BEGIN_MASK;
		if (status)
		{
		    adf_scb->adf_errcb.ad_errcode = pop_cb.pop_error.err_code;
		    break;
		}
		if (pop_cb.pop_continuation & ADP_C_END_MASK)
		    break;

		done = ((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count;
		segment_dv.db_data = ((char *) segment_dv.db_data +
						    segment_dv.db_length - 2);
		((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count =
			    length - done;
		
	    } while (DB_SUCCESS_MACRO(status) &&
			((pop_cb.pop_continuation & ADP_C_END_MASK) == 0));
	}
	else if ((outbits & AD_PERIPHERAL) == 0)
	{
	    /* long --> not-long */

	    DB_DATA_VALUE       *res_dv;
	    DB_DATA_VALUE       result;
	    char                buffer[DB_GW4_MAXSTRING + DB_CNTSIZE];
	    
	    p = (ADP_PERIPHERAL *) dv_in->db_data;
	    cpn_dv.db_data = (PTR) p;

	    if (under_dv.db_datatype == DB_NVCHR_TYPE)	
	    {
		  res_dv = &result;
		  res_dv->db_datatype = DB_NVCHR_TYPE;
		  res_dv->db_prec = 0;
		  res_dv->db_data = buffer;

	    	  switch (dv_out->db_datatype)
	    	  {
	    	    case DB_NCHR_TYPE:
	    	    case DB_CHR_TYPE:
	    	    case DB_CHA_TYPE:
	    	    case DB_BYTE_TYPE:
			  res_dv->db_length = dv_out->db_length + DB_CNTSIZE;
			  break;
	    
	    	    case DB_VCH_TYPE:
	    	    case DB_TXT_TYPE:
	    	    case DB_VBYTE_TYPE:
	    	    case DB_UTF8_TYPE:
		    case DB_NVCHR_TYPE:
			  res_dv->db_length = dv_out->db_length;
			  break;

	    	    default:
			  if (segspace)
		    	  MEfree((PTR)segspace);
			  return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2,
					0, "lvch out-under u-type"));
		  }
	    }
	    else
	    {
		switch (dv_out->db_datatype)
		{
		case DB_CHR_TYPE:
		case DB_CHA_TYPE:
		case DB_BYTE_TYPE:
		case DB_NCHR_TYPE:
		    res_dv = &result;
		    if (dv_out->db_datatype == DB_BYTE_TYPE)
			res_dv->db_datatype = DB_VBYTE_TYPE;
		    else
			res_dv->db_datatype = DB_VCH_TYPE;
		    res_dv->db_prec = 0;
		    res_dv->db_data = buffer;
		    res_dv->db_length = dv_out->db_length + DB_CNTSIZE;
		    break;

		case DB_VCH_TYPE:
		case DB_TXT_TYPE:
		case DB_VBYTE_TYPE:
		case DB_NVCHR_TYPE:
		    res_dv = dv_out;
		    break;

		default:
		    if (segspace)
			 MEfree((PTR)segspace);
		    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2,
					0, "lvch out-under type"));
		}
	    }
	    ((DB_TEXT_STRING *) res_dv->db_data)->db_t_count = 0;
	    pop_cb.pop_segment = &work.adf_agwork;
	    pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
	    pop_cb.pop_segment->db_datatype = under_dv.db_datatype;
	    pop_cb.pop_segment->db_length = under_dv.db_length;
	    pop_cb.pop_segment->db_prec = under_dv.db_prec;
	    
	    do
	    {
		status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
				    ADP_GET, &pop_cb);
		if (status)
		{
		    if (DB_FAILURE_MACRO(status) ||
			    (pop_cb.pop_error.err_code != E_AD7001_ADP_NONEXT))
		    {
			adf_scb->adf_errcb.ad_errcode =
			    	    	pop_cb.pop_error.err_code;
			break;
		    }
		}
		pop_cb.pop_continuation &= ~ADP_C_BEGIN_MASK;
		status = adu_4strconcat(adf_scb,
						res_dv,
						pop_cb.pop_segment,
						res_dv);
	    } while (DB_SUCCESS_MACRO(status) &&
			(pop_cb.pop_error.err_code != E_AD7001_ADP_NONEXT));
	    if (pop_cb.pop_error.err_code == E_AD7001_ADP_NONEXT)
		status = E_DB_OK;
	    if (DB_SUCCESS_MACRO(status) && (res_dv != dv_out))
	    {
		if (under_dv.db_datatype != DB_NVCHR_TYPE &&
	               dv_out->db_datatype != DB_NVCHR_TYPE &&
	               dv_out->db_datatype != DB_NCHR_TYPE)
		    /* We only want to drop down here if not Unicode */
		    status = adu_1strtostr(adf_scb, res_dv, dv_out);
		else if (dv_out->db_datatype == DB_UTF8_TYPE)	
		    status = adu_nvchr_toutf8 (adf_scb, res_dv, dv_out);
		else 
		    status = adu_nvchr_coerce (adf_scb, res_dv, dv_out);
	    }
	}
	else if (abs(dv_in->db_datatype) == abs(dv_out->db_datatype) ||
		abs(dv_in->db_datatype) == DB_LBYTE_TYPE &&
		abs(dv_out->db_datatype) == DB_LVCH_TYPE ||
		abs(dv_in->db_datatype) == DB_LVCH_TYPE &&
		abs(dv_out->db_datatype) == DB_LBYTE_TYPE)
	{
	    /* long --> long, let underlying handler do it all
	    ** (As long as no conversion needed) */
	    pop_cb.pop_segment = dv_in;
	    pop_cb.pop_coupon = dv_out;
	    p = (ADP_PERIPHERAL *) dv_out->db_data;
	    p->per_tag = ADP_P_COUPON;
	    p->per_length0 = ((ADP_PERIPHERAL *) dv_in->db_data)->per_length0;
	    p->per_length1 = ((ADP_PERIPHERAL *) dv_in->db_data)->per_length1;
	    
	    status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
				ADP_COPY, &pop_cb);
	    if (status)
	    {
		adf_scb->adf_errcb.ad_errcode = pop_cb.pop_error.err_code;
		break;
	    }
	}
	else
	{
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "lvch dt diff"));
	}
	break;
    }

    if (segspace)
	MEfree((PTR)segspace);

    return(status);
}

/*
** Name: adu_19strsoundex() - Gets the soundex code from a string.
**
** Description:
**	This file contains an implementation of the Soundex algorithm (see
**	"The Art of Computer Programming", Volume 3, "Searching and Sorting",
**	by D.E. Knuth).
**
**	The Soundex algorithm, originally developed sometime between 1918 and
**	1922 by Margaret K. Odell and Robert C. Russell for finding names that
**	sound alike, works by producing a four character Soundex code for the
**	name as follows:
**
**		1. Retain the first letter of the name, and drop all
**		occurrences of 'a', 'e', 'h', 'i', 'o', 'u', 'w', 'y' in
**		other positions.
**
**		2. Assign the following numbers to the remaining letters
**		after the first:
**
**			'b', 'f', 'p', 'v'	= 1
**			'c', 'g', 'j', 'k',
**			'q', 's', 'x', 'z'	= 2
**			'd', 't'		= 3
**			'l'			= 4
**			'm', 'n'		= 5
**			'r'			= 6
**
**		3. If two or more letters with the same code were adjacent
**		in the original names (before step 1), omit all but the first.
**
**		4. Convert to the form "letter, digit, digit, digit" by adding
**		trailing zeros (if there are less than three digits), or by
**		dropping rightmost digits (if there are more than three).
**
**	For example, some names and their Soundex codes are shown below:
**
**		Euler		E460
**		Gauss		G200
**		Hilbert		H416
**		Knuth		K530
**		Lloyd		L300
**		Lukasiewicz	L222
**		ffolkes		F422
**		Glendinning	G453
**
**	Non-alphabetic characters in the name are ignored.  If the specified
**	name contains no alphabetic characters, then an empty or blank
**	Soundex code is returned.

**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE containing string to
**					extract N leftmost characters from.
**	    .db_datatype		Its datatype (must be a string type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**	rdv				DB_DATA_VALUE for result.
**	    .db_datatype		Datatype of the result.
**	    .db_length			Length of the result.
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
**	rdv
**	    .db_data			Ptr to area to hold the result string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**
**  History:
**	09-Jun-1994 (mikeg) - Created UDT.
**	19-Feb-1997 (shero03) - copied and changed Mike's original code
**	    from a UDT to a base function.
*/

DB_STATUS
adu_19strsoundex(
ADF_CB                  *adf_scb,
register DB_DATA_VALUE	*dv1, 
DB_DATA_VALUE		*rdv)
{

    DB_STATUS           db_stat;
    char		*lcode;
    char		*code;
    char		*endsrc;
    char		*enddest;
    char		*srcptr;
    char		*destptr;
    char		ch;
    i4			size1;  
    i4			sizer;
    char		firsttime = 'Y';
    char ccode[] = "0123456";
    char zeroes[] = "00000";	/* destination initialization field */


    if (db_stat = adu_lenaddr(adf_scb, dv1, &size1, &srcptr))
	return (db_stat);
    if (db_stat = adu_3straddr(adf_scb, rdv, &destptr))
	return (db_stat);

    sizer = rdv->db_length;
    if (    rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_VBYTE_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )
	sizer -= DB_CNTSIZE;

    /* Limit the length of the resulting Soundex code. */

    if (sizer > AD_LEN_SOUNDEX)
        sizer = AD_LEN_SOUNDEX;

    enddest = destptr + sizer;
    endsrc = srcptr + size1;
	
    /* Initialize the Soundex code with zero characters */

    MEcopy((PTR) zeroes, sizer, (PTR) destptr);

    /* Compute the Soundex code for the specified name. */

    for (lcode = NULL; (srcptr < endsrc) && (destptr < enddest); CMnext(srcptr))
    {
    	if (CMalpha(srcptr))
        {
            if (CMlower(srcptr))
	       CMtoupper(srcptr, &ch);
	    else
	       ch = *srcptr;		

	    switch (ch)
	    {
	    	case 'B': case 'F':
	    	case 'P': case 'V':
	          code = ccode + 1;
	          break;
		case 'C': case 'G':
		case 'J': case 'K':
		case 'Q': case 'S':
		case 'X': case 'Z':
		  code = ccode + 2;
		  break;
		case 'D': case 'T':
		  code = ccode + 3;
		  break;
		case 'L':
		  code = ccode + 4;
		  break;
		case 'M': case 'N':
		  code = ccode + 5;
		  break;
		case 'R':
		  code = ccode + 6;
		  break;
		default:
		  code = ccode;
		  break;
	    }
	    if (firsttime == 'Y')
	    {
	       *destptr++ = ch;
	       firsttime = 'N';
	    }
	    else if ((code != ccode) && (code != lcode))
	       *destptr++ = *code;
	    lcode = code;
	}
	else
	  lcode = NULL;
    }

    switch (rdv->db_datatype)
    {
      case DB_BYTE_TYPE:
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	break;

      case DB_VCH_TYPE:
      case DB_VBYTE_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
        ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = sizer;
        break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    return (E_DB_OK);
}

/*
** Name: adu_soundex_dm()
**       Gets the Daitch-Mokotoff (D-M) soundex code from a string.
**
** Description:
**    This file contains an implementation of the Daitch-Mokotoff Soundex
**  algorithm (see http://www.avotaynu.com/soundex.html)
**
** It returns one or more 6 character digits in a comma seperated list. Each
** element may have leading zeroes. The list is not sorted, but each element is
** unique.
** For example:
**     Word         soundex_dm(Word)
**     Moskowitz    645740
**     Peterson     739460,734600
**     Jackson      154600,454600,145460,445460
**
**  Rules on words:
**  - The words are converted to uppercase for the soundex generation.
**  - Leading and embedded whitespace is ignored. This allows for multiple
**    word surnames such as 'De Souza', which would be treated as 'desouza'.
**  - With the exception of the hyphen '-', apostrophe and period '.' character,
**    the word(s) are terminated by the first non alpha character encountered.
**    These exceptions allow for common punctuation encountered in many names
**    and place names. For example:
**        smyth-brown would be treated as smythbrown.
**        O'brien would be treated as obrien
**        St.Kilda would be treated as Stkilda
**    The first occurence of any of these chractes is ignored, but a
**    subsequent occurrence would terminate the word.
**
** Inputs:
**    adf_scb                Pointer to an ADF session control block.
**        .adf_errcb            ADF_ERROR struct.
**        .ad_ebuflen        The length, in bytes, of the buffer
**                    pointed to by ad_errmsgp.
**        .ad_errmsgp        Pointer to a buffer to put formatted
**                    error message in, if necessary.
**    string                  DB_DATA_VALUE containing string to read
**        .db_datatype        Its datatype (must be a string type).
**        .db_length          Its length.
**        .db_data            Ptr to the data.
**
**    rdv                     DB_DATA_VALUE for result.
**        .db_datatype        Datatype of the result.
**        .db_length          Length of the result.
**
** Outputs:
**    adf_scb                Pointer to an ADF session control block.
**        .adf_errcb            ADF_ERROR struct.  If an
**                    error occurs the following fields will
**                    be set.  NOTE: if .ad_ebuflen = 0 or
**                    .ad_errmsgp = NULL, no error message
**                    will be formatted.
**        .ad_errcode        ADF error code for the error.
**        .ad_errclass        Signifies the ADF error class.
**        .ad_usererr        If .ad_errclass is ADF_USER_ERROR,
**                    this field is set to the corresponding
**                    user error which will either map to
**                    an ADF error code or a user-error code.
**        .ad_emsglen        The length, in bytes, of the resulting
**                    formatted error message.
**        .adf_errmsgp        Pointer to the formatted error message.
**    rdv
**        .db_data            Ptr to area to hold the result string.
**
**    Returns:
**        The following DB_STATUS codes may be returned:
**        E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**        If a DB_STATUS code other than E_DB_OK is returned, the caller
**        can look in the field adf_scb.adf_errcb.ad_errcode to determine
**        the ADF error code.  The following is a list of possible ADF error
**        codes that can be returned by this routine:
**
**        E_AD0000_OK            Routine completed successfully.
**          E_AD9998_INTERNAL_ERROR
**              The internal buffer used in the soundex_dm calculation would
**              overflow. This is extremely unlikely!
**
**  History:
**	01-Mar-2009 (Martin Bowes)
**	    Created.
**	28-May-2010 (kiria01) b122320
**	    Bring in line with CL for CM character handling. Made char constant
**	    buffers static const and identified the bools.
**
*/
DB_STATUS
adu_soundex_dm(
    ADF_CB                      *adf_scb,
    DB_DATA_VALUE               *string, 
    DB_DATA_VALUE               *rdv
    )
{
    /* Sundry initialisation */
    bool        hyphen = FALSE;
    bool        start_word = FALSE;
    i4          oi, ei = 0, encoded = 0;
    bool        before_a_vowel = FALSE;
    i4          total_choices;
    i4          choice, total_routes;
    i4          i, j;

    /* The internal buffer to hold the uppercase'd and stripped input string.
    ** This is sized to more than easily allow for enough characters to 
    ** generate the 6 character D-M soundex value (AD_LEN_SOUNDEX_DM)
    */
    char        buffer[AD_SOUNDEX_DM_INT_BUFFER + AD_SOUNDEX_DM_PAD_BUFFER]; 

    /* These are fixed codes used in the D-M soundex algorithm */
    static const char FourtyThree[]   = "43";
    static const char FourtyFive[]    = "45";
    static const char FiftyFour[]     = "54";
    static const char SixtySix[]      = "66";
    static const char NinetyFour[]    = "94";

    /* Define the structures for the encoding array and the buffer of possible
     * output values
     */
    struct _dm_element_bit
    {
        i2 length;
        char  string[2];
    };

    struct _dm_element
    {
        enum _dm_element_source
        {character, phrase} source; /* On occasion we need to know the source of
                                    ** the element. This is for characters with
                                    ** two possible sounds, where we must allow
                                    ** for the chance of two successive 
                                    ** characters with the same sound number.
                                    ** See following note on Rule 4.
                                    */
        i2             choice_mask; /* If non zero, there are two choices.
                                    ** The mask is a bit pattern, essentially 
                                    ** indicating the choice number as a power
                                    ** of two, which allows the following task
                                    ** of printing all choices to navigate the
                                    ** encoded array.
                                    */
        struct _dm_element_bit left;
        struct _dm_element_bit right;
    };

    struct _dm_prior_element /* This is used in some specialised handling */
    {
        enum _dm_element_source source;
        i2 length;
        char  string[2];
    } prior;

    struct _dm_element encode[AD_SOUNDEX_DM_MAX_ENCODE];
    /* I'm not going to initialise all elements of the encode[] array. This will
    ** save time but we have to be careful when processing the array later!
    */
    char output[AD_SOUNDEX_DM_MAX_OUTPUT][AD_LEN_SOUNDEX_DM];
    char *chp, *endp;

    /* Confirm input is a varchar */
    switch (abs(string->db_datatype))
    {
      case DB_VCH_TYPE:
        break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    /* Confirm output is a correctly sized varchar */
    if (rdv->db_datatype != DB_VCH_TYPE || rdv->db_length != AD_SOUNDEX_DM_OUTPUT_LEN)
    {
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    /* Pad the internal buffer with spaces */
    MEfill(sizeof(buffer), ' ', buffer);

    /* Preprocess: Fill internal buffer[]
    ** Point to first alpha char in input. None? Return an error!
    ** Convert char to upper until first non-alpha. If non-alpha is one or more
    ** blanks then skip these and continue. Ignore first occurrence of hyphen
    ** apostrophe or period characters.
    */
    chp = (char*)string->db_data + sizeof(i2);
    endp = chp + *(i2 *)string->db_data;
    for (j = 0; chp < endp && j < AD_SOUNDEX_DM_INT_BUFFER; CMnext(chp))
    {
        /*ignore spaces*/
        if (CMwhite(chp)) continue;

        /* Ignore first hyphen, apostrophe or period only */
        if ((*chp == '-' || *chp == '\'' || *chp == '.') &&
	    !hyphen)
        {
            hyphen = TRUE;
            continue;
        }

        /* Process Alpha characters */
        if (CMalpha(chp)) 
        {
            /* Convert to uppercase and store in buffer */
	    CMtoupper(chp, &buffer[j]);
            start_word = TRUE;
	    j++;
            continue;
        }
        break; /* Anything else breaks the pre-process loop */
    } /* For */

    if (!start_word)
    {
        *(i2 *)(rdv->db_data) = AD_LEN_SOUNDEX_DM;
        MEfill(AD_LEN_SOUNDEX_DM, '0', (PTR)(rdv->db_data + sizeof(i2)));
        return (E_DB_OK); /* Nothing to do, return "000000" */
    }

    buffer[j] = EOS; /* Terminate the buffer */
#ifdef xDEBUG
    TRdisplay("soundex_dm(): Called on %s\n", buffer);
#endif
    /* Now process the data stored in the buffer[].
     * This for loop loads the encode array with the list of possibilities.
     */

    /* Sundry Initialisation */
    total_choices = 0;
    for (i = 0, ei = 0; 
        i < j && ei < AD_SOUNDEX_DM_MAX_ENCODE && encoded <= AD_LEN_SOUNDEX_DM + 1;
        ei++, start_word = FALSE
        )
    {
#ifdef xDEBUG
        TRdisplay("soundex_dm(): Consider string starting at %c\n", buffer[i]);
#endif
        /* The most likely settings, which will be overridden when required */
        encode[ei].choice_mask     = 0;
        encode[ei].source          = phrase;
        encode[ei].left.length     = 1;       /* Most likely one char long */
        encode[ei].right.length    = 1;
        encode[ei].right.string[0] = '-'; /*'-' means 'Not Coded' */

        /* The most likely 'before a vowel' scenario, which will be restetd if 
        ** required
        */
        before_a_vowel = soundex_dm_vowelage(buffer, i, j, 1);

        /* The 'A' cases... */
        if (!STncmp((char *)(buffer + i), "AI", 2)
         || !STncmp((char *)(buffer + i), "AJ", 2)
         || !STncmp((char *)(buffer + i), "AY", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                before_a_vowel = soundex_dm_vowelage(buffer, i, j, 2);
                if (before_a_vowel)
                {
                    encode[ei].left.string[0] = '1';
                    encoded++;
                }
                else
                {
                    encode[ei].left.string[0] = '-';
                }
            }
            i += 2;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "AU", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                before_a_vowel = soundex_dm_vowelage(buffer, i, j, 2);
                if (before_a_vowel)
                {
                    encode[ei].left.string[0] = '7';
                    encoded++;
                }
                else
                {
                    encode[ei].left.string[0] = '-';
                }
            }
            i += 2;
            continue;
        }

        if (buffer[i] == 'A')
        {
            encode[ei].source = character;
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i++;
            continue;
        }

        /* The 'B', 'V', 'W' cases... */
        if (buffer[i] == 'B' || buffer[i] == 'V' || buffer[i] == 'W')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '7';
            encoded++;
            i++;
            continue;
        }

        /* The 'C' cases... */
        if (!STncmp((char *)(buffer + i), "CHS", 3))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '5';
            }
            else 
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FiftyFour, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i += 3;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "CSZ", 3)
          || !STncmp((char *)(buffer + i), "CZS", 3))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 3;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "CH", 2)) /* Try KH(5) and TCH(4) */
        {
            encode[ei].choice_mask = 1 << total_choices;
            total_choices++;
            encode[ei].left.string[0]  = '5'; /* As KH */
            encode[ei].right.string[0] = '4'; /* As TCH */
            encoded++;
            i += 2;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "CK", 2)) /* Try K(5) and TSK(45) */
        {
            encode[ei].choice_mask = 1 << total_choices;
            total_choices++;
            encode[ei].left.string[0] = '5'; /* As K */
            encode[ei].right.length = 2;
            MEcopy((PTR )FourtyFive, 2, (PTR )(encode[ei].right.string));
            encoded++;
            i += 2;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "CS", 2) 
         || !STncmp((char *)(buffer + i), "CZ", 2))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] == 'C') /* Try K(5) and TZ(4) */
        {
            encode[ei].source = character;
            encode[ei].choice_mask = 1 << total_choices;
            total_choices++;
            encode[ei].left.string[0]  = '5'; /* As K */
            encode[ei].right.string[0] = '4'; /* As TZ */
            encoded++;
            i++;
            continue;
        }

        /* The 'D' cases... */
        if (!STncmp((char *)(buffer + i), "DRZ", 3)
         || !STncmp((char *)(buffer + i), "DRS", 3)
         || !STncmp((char *)(buffer + i), "DSH", 3)
         || !STncmp((char *)(buffer + i), "DSZ", 3)
         || !STncmp((char *)(buffer + i), "DZH", 3)
         || !STncmp((char *)(buffer + i), "DZS", 3))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 3;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "DS", 2) 
         || !STncmp((char *)(buffer + i), "DZ", 2))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 2;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "DT", 2))
        {
            encode[ei].left.string[0] = '3';
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] == 'D')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '3';
            encoded++;
            i++;
            continue;
        }

        /* The 'E' cases... */
        if (!STncmp((char *)(buffer + i), "EI", 2)
         || !STncmp((char *)(buffer + i), "EJ", 2)
         || !STncmp((char *)(buffer + i), "EY", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
                }
            else
            {
                before_a_vowel = soundex_dm_vowelage(buffer, i, j, 2);
                if (before_a_vowel)
                {
                    encode[ei].left.string[0] = '1';
                    encoded++;
                }
                else
                {
                    encode[ei].left.string[0] = '-';
                }
            }
            i += 2;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "EU", 2))
        {
            before_a_vowel = soundex_dm_vowelage(buffer, i, j, 2);
            if (start_word || before_a_vowel)
            {
                encode[ei].left.string[0] = '1';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i += 2;
            continue;
        }

        if (buffer[i] =='E')
        {
            encode[ei].source = character;
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i++;
            continue;
        }

        /* The 'F' cases... */
        if (!STncmp((char *)(buffer + i), "FB", 2))
        {
            encode[ei].left.string[0] = '7';
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] =='F')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '7';
            encoded++;
            i++;
            continue;
        }

        /* The 'G' and 'Q' cases... */
        if (buffer[i] =='G' || buffer[i] =='Q')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '5';
            encoded++;
            i++;
            continue;
        }

        /* The 'H' cases... */
        if (buffer[i] =='H')
        {
            encode[ei].source = character;
            if (start_word || before_a_vowel)
            {
                encode[ei].left.string[0] = '5';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i++;
            continue;
        }

        /* The 'I' cases... */
        if (!STncmp((char *)(buffer + i), "IA", 2)
         || !STncmp((char *)(buffer + i), "IE", 2)
         || !STncmp((char *)(buffer + i), "IO", 2)
         || !STncmp((char *)(buffer + i), "IU", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '1';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i += 2;
            continue;
        }
    
        if (buffer[i] =='I')
        {
            encode[ei].source = character;
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i++;
            continue;
        }

        /* The 'J' cases... */
        if (buffer[i] =='J') /* Try Y(1) and DZH(4) */
        {
            encode[ei].source = character;
            encode[ei].choice_mask = 1 <<total_choices;
            total_choices++;
            encode[ei].right.string[0] = '4'; /* As DZH(4) */
            if (start_word) 
            {
                encode[ei].left.string[0] = '1'; /* As Y(1) */
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-'; /* ie. Not Coded */
            }
            i++;
            continue;
        }

        /* The 'K' cases... */
        if (!STncmp((char *)(buffer + i), "KS", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '5';
            }
            else 
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FiftyFour, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i += 2;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "KH", 2))
        {
            encode[ei].left.string[0] = '5';
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] =='K')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '5';
            encoded++;
            i++;
            continue;
        }

        /* The 'L' cases... */
        if (buffer[i] =='L')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '8';
            encoded++;
            i++;
            continue;
        }

        /* The 'M' cases... */
        if (!STncmp((char *)(buffer + i), "MN", 2))
        {
            encode[ei].left.length = 2;
            MEcopy((PTR )SixtySix, 2, (PTR )(encode[ei].left.string));
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] =='M')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '6';
            encoded++;
            i++;
            continue;
        }

        /* The 'N' cases... */
        if (!STncmp((char *)(buffer + i), "NM", 2))
        {
            encode[ei].left.length = 2;
            MEcopy((PTR )SixtySix, 2, (PTR )(encode[ei].left.string));
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] =='N')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '6';
            encoded++;
            i++;
            continue;
        }

        /* The 'O' cases... */
        if (!STncmp((char *)(buffer + i), "OI", 2)
         || !STncmp((char *)(buffer + i), "OJ", 2)
         || !STncmp((char *)(buffer + i), "OY", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                before_a_vowel = soundex_dm_vowelage(buffer, i, j, 2);
                if (before_a_vowel)
                {
                    encode[ei].left.string[0] = '1';
                    encoded++;
                }
                else
                {
                    encode[ei].left.string[0] = '-';
                }
            }
            i += 2;
            continue;
        }
    
        if (buffer[i] =='O')
        {
            encode[ei].source = character;
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i++;
            continue;
        }

        /* The 'P' cases... */
        if (!STncmp((char *)(buffer + i), "PF", 2)
         || !STncmp((char *)(buffer + i), "PH", 2))
        {
            encode[ei].left.string[0] = '7';
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] =='P')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '7';
            encoded++;
            i++;
            continue;
        }

        /* The 'R' cases... */
        if (!STncmp((char *)(buffer + i), "RTZ", 3))
        {
            encode[ei].left.length = 2;
            MEcopy((PTR )NinetyFour, 2, (PTR )(encode[ei].left.string));
            encoded++;
            i += 3;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "RS", 2) /* Try RTZ(94) and ZH(4) */
         || !STncmp((char *)(buffer + i), "RZ", 2))
        {
            encode[ei].choice_mask = 1 << total_choices;
            total_choices++;
            encode[ei].left.length = 2;
            MEcopy((PTR )NinetyFour, 2, (PTR )(encode[ei].left.string)); /* Try RTZ(94) */

            encode[ei].right.string[0] = '4'; /* Try ZH(4) */

            encoded++;
            i += 2;
            continue;
        }

        if (buffer[i] =='R')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '9';
            encoded++;
            i++;
            continue;
        }

        /* The 'S' cases... */
        if (!STncmp((char *)(buffer + i), "SCHTSCH", 7))
        {
            encode[ei].source = phrase;
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 7;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "SCHTSH", 6)
         || !STncmp((char *)(buffer + i), "SCHTCH", 6))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 6;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "SHTCH", 5)
         || !STncmp((char *)(buffer + i), "SHTSH", 5)
         || !STncmp((char *)(buffer + i), "STSCH", 5))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 5;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "SHCH", 4)
         || !STncmp((char *)(buffer + i), "STRZ", 4)
         || !STncmp((char *)(buffer + i), "STRS", 4)
         || !STncmp((char *)(buffer + i), "STSH", 4))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 4;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "SCHT", 4)
         || !STncmp((char *)(buffer + i), "SCHD", 4))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FourtyThree, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i += 4;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "STCH", 4)
         || !STncmp((char *)(buffer + i), "SZCZ", 4)
         || !STncmp((char *)(buffer + i), "SZCS", 4))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 4;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "SCH", 3))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 3;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "SHT", 3)
         || !STncmp((char *)(buffer + i), "SZT", 3)
         || !STncmp((char *)(buffer + i), "SHD", 3)
         || !STncmp((char *)(buffer + i), "SZD", 3))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FourtyThree, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i += 3;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "SH", 2)
         || !STncmp((char *)(buffer + i), "SZ", 2))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 2;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "SC", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 2;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "ST", 2)
         || !STncmp((char *)(buffer + i), "SD", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FourtyThree, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i += 2;
            continue;
        }

        if (buffer[i] =='S')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '4';
            encoded++;
            i++;
            continue;
        }

        /* The 'T' cases... */
        if (!STncmp((char *)(buffer + i), "TTSCH", 5))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 5;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "TTCH", 4)
         || !STncmp((char *)(buffer + i), "TSCH", 4)
         || !STncmp((char *)(buffer + i), "TTSZ", 4))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 4;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "TSK", 3))
        {
            encode[ei].left.length = 2;
            MEcopy((PTR )FourtyFive, 2, (PTR )(encode[ei].left.string));
            encoded++;
            i += 3;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "TCH", 3)
         || !STncmp((char *)(buffer + i), "TRZ", 3)
         || !STncmp((char *)(buffer + i), "TRS", 3)
         || !STncmp((char *)(buffer + i), "TSH", 3)
         || !STncmp((char *)(buffer + i), "TTS", 3)
         || !STncmp((char *)(buffer + i), "TTZ", 3)
         || !STncmp((char *)(buffer + i), "TSZ", 3)
         || !STncmp((char *)(buffer + i), "TZS", 3))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 3;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "TH", 2))
        {
            encode[ei].left.string[0] = '3';
            encoded++;
            i += 2;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "TC", 2)
         || !STncmp((char *)(buffer + i), "TZ", 2)
         || !STncmp((char *)(buffer + i), "TS", 2))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] =='T')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '3';
            encoded++;
            i++;
            continue;
        }

        /* The 'U' cases... */
        if (!STncmp((char *)(buffer + i), "UI", 2)
         || !STncmp((char *)(buffer + i), "UJ", 2)
         || !STncmp((char *)(buffer + i), "UY", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                before_a_vowel = soundex_dm_vowelage(buffer, i, j, 2);
                if (before_a_vowel)
                {
                    encode[ei].left.string[0] = '1';
                    encoded++;
                }
                else
                {
                    encode[ei].left.string[0] = '-';
                }
            }
            i += 2;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "UE", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i += 2;
            continue;
        }

        if (buffer[i] =='U')
        {
            encode[ei].source = character;
            if (start_word)
            {
                encode[ei].left.string[0] = '0';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i++;
            continue;
        }

        /* The 'X' cases... */
        if (buffer[i] =='X')
        {
            encode[ei].source = character;
            if (start_word)
            {
                encode[ei].left.string[0] = '5';
            }
            else
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FiftyFour, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i++;
            continue;
        }

        /* The 'Y' cases... */
        if (buffer[i] =='Y')
        {
            encode[ei].source = character;
            if (start_word)
            {
                encode[ei].left.string[0] = '1';
                encoded++;
            }
            else
            {
                encode[ei].left.string[0] = '-';
            }
            i++;
            continue;
        }

        /* The 'Z' cases... */
        if (!STncmp((char *)(buffer + i), "ZHDZH", 5))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 5;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "ZDZH", 4))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 4;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "ZSCH", 4))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 4;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "ZDZ", 3))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else
            {
                encode[ei].left.string[0] = '4';
            }
            encoded++;
            i += 3;
            continue;
        }

        if (!STncmp((char *)(buffer + i), "ZHD", 3))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FourtyThree, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i += 3;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "ZSH", 3))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 3;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "ZD", 2))
        {
            if (start_word)
            {
                encode[ei].left.string[0] = '2';
            }
            else 
            {
                encode[ei].left.length = 2;
                MEcopy((PTR )FourtyThree, 2, (PTR )(encode[ei].left.string));
            }
            encoded++;
            i += 2;
            continue;
        }
    
        if (!STncmp((char *)(buffer + i), "ZH", 2)
         || !STncmp((char *)(buffer + i), "ZS", 2))
        {
            encode[ei].left.string[0] = '4';
            encoded++;
            i += 2;
            continue;
        }
    
        if (buffer[i] =='Z')
        {
            encode[ei].source = character;
            encode[ei].left.string[0] = '4';
            encoded++;
            i++;
            continue;
        }
    } /* For each character in string */

#ifdef xDEBUG
    TRdisplay("soundex_dm(): commence processing encode array\n");
    for (i = 0; i <ei; i++)
    {
        TRdisplay("encode[%d].choice_mask %d\n", i, encode[i].choice_mask);
        TRdisplay("encode[%d].left %c", i, encode[i].left.string[0]);
        if (encode[i].left.length>1)
            TRdisplay("%c", encode[i].left.string[1]);
        TRdisplay("\n");
        TRdisplay("encode[%d].right %c", i, encode[i].right.string[0]);
        if (encode[i].right.length>1)
            TRdisplay("%c", encode[i].right.string[1]);
        TRdisplay("\n");
    }
#endif
    /* Process the encode array into the output array.
    ** 
    ** Note that ei is the index of the last element in the encode array plus 1.
    ** 
    ** At this point we do the final weed of adjacent characters with the same
    ** sound code. 
    */
    total_routes = (i4 )1 << total_choices;
#ifdef xDEBUG
    TRdisplay("soundex_dm(): There are %d routes through encode array\n", total_routes);
#endif
    for (oi = 0; oi < total_routes && oi < AD_SOUNDEX_DM_MAX_OUTPUT; oi++)
    {
#ifdef xDEBUG
        TRdisplay("Process route %d\n", oi);
#endif
        MEfill(AD_LEN_SOUNDEX_DM, '0', (PTR) output[oi]);
        j = 0;                 /* j is character position in output[oi] */
        prior.source = phrase; /* init 'prior' case */
        prior.length = 1;
        prior.string[0] = '-';
        for (i = 0; i < ei; i++) /* i is index of encoded array */
        {
            if (encode[i].choice_mask == 0)
            {
#ifdef xDEBUG
                TRdisplay("No choice at encode[%d]\n", i);
#endif
                if (encode[i].left.string[0] != '-')
                {
                    if (encode[i].source == character
                     && prior.source == character
                     && !STncmp(prior.string, encode[i].left.string,
                         encode[i].left.length))
                    {
                        continue;
                    }
                    /* Otherwise....*/
                    output[oi][j++] = encode[i].left.string[0];
                    if (encode[i].left.length > 1 && j < AD_LEN_SOUNDEX_DM)
                        output[oi][j++] = encode[i].left.string[1];
                }
                /* Save left as 'prior' case */
                prior.source = encode[i].source;
                prior.length = encode[i].left.length;
                MEcopy(encode[i].left.string, prior.length, prior.string);
            }
            else
            {
                choice = oi & encode[i].choice_mask;
                /* Left if choice = 0, Right if choice = 1 */
#ifdef xDEBUG
                TRdisplay("oi (%d) & choice_mask (%d) at encode[%d] = %d\n", oi, encode[i].choice_mask, i, choice);
#endif
                if (!choice)
                {
#ifdef xDEBUG
                    TRdisplay("Went left\n");
#endif
                    if (encode[i].left.string[0] != '-')
                    {
                        if (encode[i].source == character
                         && prior.source == character
                         && !STncmp(prior.string, encode[i].left.string,
                             encode[i].left.length))
                        {
                            continue;
                        }
                        /* Otherwise....*/
                        output[oi][j++] = encode[i].left.string[0];
                        if (encode[i].left.length > 1 && j < AD_LEN_SOUNDEX_DM)
                            output[oi][j++] = encode[i].left.string[1];
                    }
                    /* Save left as 'prior' case */
                    prior.source = encode[i].source;
                    prior.length = encode[i].left.length;
                    MEcopy(encode[i].left.string, prior.length, prior.string);
                }
                else
                {
#ifdef xDEBUG
                    TRdisplay("Went right\n");
#endif
                    if (encode[i].right.string[0] != '-')
                    {
                        if (encode[i].source == character
                         && prior.source == character
                         && !STncmp(prior.string, encode[i].right.string,
                             encode[i].right.length))
                        {
                            continue;
                        }
                        /* Otherwise....*/
                        output[oi][j++] = encode[i].right.string[0];
                        if (encode[i].right.length > 1 && j < AD_LEN_SOUNDEX_DM)
                            output[oi][j++] = encode[i].right.string[1];
                    }
                    /* Save right as 'prior' case */
                    prior.source = encode[i].source;
                    prior.length = encode[i].right.length;
                    MEcopy(encode[i].right.string, prior.length, prior.string);
                }
            }
        } /* for each element in encoded array */
    } /* For each possible route through the array */

#ifdef xDEBUG
    TRdisplay("soundex_dm(): Output array is:\n");
    for (i = 0; i < oi; i++)
    {
        TRdisplay("output[%d]: ", i);
        for (j = 0; j < AD_LEN_SOUNDEX_DM; j++) TRdisplay("%c", output[i][j]);
        TRdisplay("\n");
    }
    TRdisplay("soundex_dm(): Now build return string\n");
#endif
    /* Build the rdv->data from the output array, removing duplicates along
    ** the way.
    */
    MEcopy(output[0], AD_LEN_SOUNDEX_DM, (PTR)(rdv->db_data + sizeof(i2)));
    *(i2 *)(rdv->db_data) = AD_LEN_SOUNDEX_DM;
    for (i = 1; i < oi; i++)
    {
        bool unique = TRUE;
        for (j = 0; j < i; j++) /* check for duplicates */
        {
            if (!STncmp(output[j], output[i], AD_LEN_SOUNDEX_DM))
            {
#ifdef xDEBUG
                TRdisplay("output[%d] == output[%d]\n", i,j);
#endif
                unique = FALSE;
                break;
            }
        }

        if (unique)
        {
            if (rdv->db_length >= (*(i2 *)rdv->db_data + AD_LEN_SOUNDEX_DM + 1))
            {
                *(char *)(rdv->db_data + sizeof(i2) + *(i2 *)rdv->db_data) = ',';
                MEcopy(output[i], AD_LEN_SOUNDEX_DM,
                    (PTR)(rdv->db_data + sizeof(i2) + *(i2 *)rdv->db_data) + 1);
                *(i2 *)rdv->db_data += AD_LEN_SOUNDEX_DM + 1;
            }
            else
            {
                return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2,
                    0, "soundex_dm(): unexpected overflow in return string length."));
            }
        }
    }
    return (E_DB_OK);
} /* adu_soundex_dm */

/* soundex_dm_vowelage:
**     Simply checks if the current code set is before a vowel.
**     In this case a vowel is in the set: A, E, I, O, U, J and Y
*/
static bool
soundex_dm_vowelage (
    char const *buffer,   /* The buffer of characters to check  */
    i4  b_ptr,     /* The current position in the buffer */
    i4  b_len,     /* The length of the buffer           */
    i4  skip       /* How far ahead to check for a vowel */
    )
{
    /* return (0) if we have exhausted the buffer */
    if (b_ptr + skip >= b_len) 
	return FALSE;

    buffer += b_ptr + skip;
    /* return (1) if before a vowel */
    return (*buffer == 'A' ||
	    *buffer == 'E' ||
	    *buffer == 'I' || 
	    *buffer == 'O' ||
	    *buffer == 'U' ||
	    *buffer == 'J' ||
	    *buffer == 'Y');
} /* soundex_dm_vowelage */

/*
** Name: adu_20substr() - Return the substring from a string.
**
** Description:
**	This file contains an implementation of 
**	SUBSTRING ( char FROM int ).  It is based on the
**	description for substring in ISO/IEC FDIS 9075-2:1999
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE containing string to
**					do substring from.
**	    .db_datatype		Its datatype (must be a string type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**      dv2                             DB_DATA_VALUE containing FROM operand
**	    .db_datatype		Its datatype (must be an integer type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**	rdv				DB_DATA_VALUE for result.
**	    .db_datatype		Datatype of the result.
**	    .db_length			Length of the result.
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
**	rdv
**	    .db_data			Ptr to area to hold the result string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**	    E_AD2092_BAD_STARTR_FOR_SUBSTR Specified start is past char expr
**
**  History:
**	21-apr-1999 (shero03)
**	    Created.
**	26-May-1999 (shero03)
**	    Add substring error: AD2092
**      30-Apr-2007 (kiria01) b118215
**	    Reworked SUBSTRING(n) to call adu_strextract(n,MAX)
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_20substr(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    i4			startpos;
    i8			i8pos;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_20substr: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
    }
#endif

    /* get the value of FROM */
    switch (dv2->db_length)
    {	
      case 1:
        startpos = I1_CHECK_MACRO(*(i1 *) dv2->db_data);
        break;
      case 2:
        startpos = *(i2 *) dv2->db_data;
        break;
      case 4:	
        startpos = *(i4 *) dv2->db_data;
        break;
      case 8:	
	i8pos = *(i8 *) dv2->db_data;

	/* limit to i4 values */
	if ( i8pos > MAXI4 || i8pos < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "substr start overflow"));

	startpos = (i4) i8pos;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "substr start length"));
    }

    /*
    ** ISO/IEC 9075-2:1999 (E) (6.18 string functions 3.e.ii.1)
    ** the startpos is maximised with 1
    */
    if (startpos < 1)
        startpos = 1;
    /* Return rest of string counting from startpos */
    db_stat = adu_strextract(adf_scb, dv1, startpos, MAXI4, rdv);

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_20substr: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}

/*
** Name: adu_21substrlen() - Return the substring from a string.
**
** Description:
**	This file contains an implementation of 
**	SUBSTRING ( char FROM int FOR int ).  It is based on the
**	description for substring in ISO/IEC FDIS 9075-2:1999
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE containing string to
**					do substring from.
**	    .db_datatype		Its datatype (must be a string type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**      dv2                             DB_DATA_VALUE containing FROM operand
**	    .db_datatype		Its datatype (must be an integer type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**      dv3                             DB_DATA_VALUE containing FOR operand
**	    .db_datatype		Its datatype (must be an integer type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**	rdv				DB_DATA_VALUE for result.
**	    .db_datatype		Datatype of the result.
**	    .db_length			Length of the result.
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
**	rdv
**	    .db_data			Ptr to area to hold the result string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**
**  History:
**	21-apr-1999 (shero03) - Created.
**	2-dec-03 (inkdo01)
**	    Added 2093 message for bad forlen computation.
**      30-Apr-2007 (kiria01) b118215
**	    Reworked SUBSTRING(p,n) to call adu_strextract(p,n)
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_21substrlen(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*dv3,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    i4			startpos;
    i4			forlen;
    i8		        i8temp;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_21substrlen: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(dv3);
        adu_prdv(rdv);
    }
#endif

    /* get the value of FROM */
    switch (dv2->db_length)
    {
	case 1:
	    startpos =  I1_CHECK_MACRO( *(i1 *) dv2->db_data);
	    break;
	case 2: 
	    startpos =  *(i2 *) dv2->db_data;
	    break;
	case 4:
	    startpos =  *(i4 *) dv2->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv2->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "asubstr start overflow"));

	    startpos = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "asubstr start length"));
    }

    /* get the value of FOR */
    switch (dv3->db_length)
    {
	case 1:
	    forlen =  I1_CHECK_MACRO( *(i1 *) dv3->db_data);
	    break;
	case 2: 
	    forlen =  *(i2 *) dv3->db_data;
	    break;
	case 4:
	    forlen =  *(i4 *) dv3->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv3->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "asubstr for overflow"));

	    forlen = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "asubstr for length"));
    }

    /*
    ** ISO/IEC 9075-2:1999 (E) (6.18 string functions 3.d)
    ** forlen < 0 generates an error
    */
    if (forlen < 0)
	return (adu_error(adf_scb, E_AD2093_BAD_LEN_FOR_SUBSTR, 0));

    /*
    ** ISO/IEC 9075-2:1999 (E) (6.18 string functions 3.e.ii.1)
    ** the startpos is maximised with 1
    */
    if (startpos < 1)
        startpos = 1;
    /* Return forlen characters counting from startpos */
    db_stat = adu_strextract(adf_scb, dv1, startpos, forlen, rdv);

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_21substrlen: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}


/*{
** Name: adu_22charlength() - Returns the ANSI char_length function value (almost
**	the same as Ingres' length function, except c/char types aren't blank
**	trimmed).
**
** Description:
**	This routine returns the number of characters in a string in a DB_DATA_VALUE.
**	
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE, containing
**					string.
**	    .db_length			Length of string, including blanks.
**	    .db_data			Ptr to string.
**	rdv				Ptr to destination DB_DATA_VALUE.  Note,
**					it is assumed that this is of type
**					DB_INT_TYPE and has a length of 2.
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
**	rdv
**	    .db_data			Ptr to an i2 which will hold the strings
**					length.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	dv1's datatype was inappropriate for
**					this operation.
**
** History:
**	12-nov-01 (inkdo01)
**	    Cloned from adu_7strlength.
**	13-dec-01 (inkdo01)
**	    Renamed to please Steve (and follow standards).
**	16-dec-03 (inkdo01)
**	    Propagated from 3.0 to main.
**	09-May-2007 (gupsh01)
**	    Added support for UTF8 character sets.
**      17-Jul-2008 (hanal04) Bug 120646
**          adu_lenaddr() does not provide support for DB_LVCH_TYPE and
**          DB_LBYTE_TYPE because the address can not be established. As we
**          only want the length get it in line.
*/

DB_STATUS
adu_22charlength(
ADF_CB                  *adf_scb,
register DB_DATA_VALUE	*dv1,
register DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    i4             tmp_size;
    char	   *p = (char *)dv1->db_data;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_22charlength: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
    }
#endif

    if ((Adf_globs->Adi_status & ADI_DBLBYTE)
        || (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
    {
	char	*endp;
	i4	count = 0;
	i4	size;
	i4	inter = 0;
	
	switch (dv1->db_datatype)
	{
	  case DB_CHR_TYPE:
          case DB_CHA_TYPE:
	  case DB_VCH_TYPE:
	  case DB_TXT_TYPE:
	  case DB_LTXT_TYPE:
	    if (db_stat = adu_lenaddr(adf_scb, dv1, &size, &p))
		return (db_stat);
	    endp = p + size;
	    
	    while (p < endp)
	    {
		count++;
		CMnext(p);
	    }
	    break;

	  case DB_LVCH_TYPE:
	  case DB_LBYTE_TYPE:
	  {
		ADP_PERIPHERAL	    *periph = (ADP_PERIPHERAL *) dv1->db_data;

		count = periph->per_length1;
		break;
	  }

	  case DB_BYTE_TYPE:
	  case DB_VBYTE_TYPE:
	    if ((db_stat = adu_size(adf_scb, dv1, &count)) != E_DB_OK)
		return (db_stat);
	    break;
	    

	  default:
	    return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
	}

	if (rdv->db_length == 2)
	    *(i2 *)rdv->db_data = count;
	else
	    *(i4 *)rdv->db_data = count;
    }
    else
    {
	switch (dv1->db_datatype)
	{
	  case DB_LVCH_TYPE:
	  case DB_LBYTE_TYPE:
	  {
		ADP_PERIPHERAL	    *periph = (ADP_PERIPHERAL *) dv1->db_data;

		tmp_size = periph->per_length1;
		break;
	  }

	  default:
            if ((db_stat = adu_lenaddr(adf_scb, dv1, &tmp_size, &p)) != E_DB_OK)
	        return (db_stat);
        }

        if (rdv->db_length == 2)
	    *(i2 *)rdv->db_data = (i2)tmp_size;
        else
	    *(i4 *)rdv->db_data = tmp_size;
    }
    
#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_22charlength: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_23octetlength() - Returns the ANSI octet_length function value (almost
**	the same as Ingres' length function, except c/char types aren't blank
**	trimmed).
**
** Description:
**	This routine returns the number of 8-bit bytes in a string in a DB_DATA_VALUE.
**	
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE, containing
**					string.
**	    .db_length			Length of string, including blanks.
**	    .db_data			Ptr to string.
**	rdv				Ptr to destination DB_DATA_VALUE.  Note,
**					it is assumed that this is of type
**					DB_INT_TYPE and has a length of 2.
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
**	rdv
**	    .db_data			Ptr to an i2 which will hold the strings
**					length.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	dv1's datatype was inappropriate for
**					this operation.
**
** History:
**	12-nov-01 (inkdo01)
**	    Cloned from adu_22charlength.
**	13-dec-01 (inkdo01)
**	    Renamed to please Steve (and follow standards).
**	16-dec-03 (inkdo01)
**	    Propagated from 3.0 to main.
*/

DB_STATUS
adu_23octetlength(
ADF_CB                  *adf_scb,
register DB_DATA_VALUE	*dv1,
register DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    i4             tmp_size;
    char	   *p = (char *)dv1->db_data;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_23octetlength: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
    }
#endif


    if ((db_stat = adu_lenaddr(adf_scb, dv1, &tmp_size, &p)) != E_DB_OK)
	return (db_stat);

    if (rdv->db_length == 2)
	*(i2 *)rdv->db_data = (i2)tmp_size;
    else
	*(i4 *)rdv->db_data = tmp_size;
    
#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_23octetlength: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_24bitlength() - Returns the ANSI bit_length function value (basically, 
**	8 * octet_length).
**
** Description:
**	This routine returns the number bits in a string in a DB_DATA_VALUE.
**	
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to source DB_DATA_VALUE, containing
**					string.
**	    .db_length			Length of string, including blanks.
**	    .db_data			Ptr to string.
**	rdv				Ptr to destination DB_DATA_VALUE.  Note,
**					it is assumed that this is of type
**					DB_INT_TYPE and has a length of 2.
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
**	rdv
**	    .db_data			Ptr to an i2 which will hold the strings
**					length.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	dv1's datatype was inappropriate for
**					this operation.
**
** History:
**	8-jan-02 (inkdo01)
**	    Cloned from adu_23octetlength.
**	16-dec-03 (inkdo01)
**	    Propagated from 3.0 to main.
**	4-june-2007 (dougi)
**	    Genericize to be a DB_ALL_TYPE function.
*/

DB_STATUS
adu_24bitlength(
ADF_CB                  *adf_scb,
register DB_DATA_VALUE	*dv1,
register DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    i4             tmp_size;
    char	   *p = (char *)dv1->db_data;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_23octetlength: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
    }
#endif

    switch (dv1->db_datatype)
    {
	/* call lenaddr for the stringish types - it gets varying length
	** lengths, among other things. */
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
        case DB_LTXT_TYPE:
	case DB_VBYTE_TYPE:
	case DB_NCHR_TYPE:
	case DB_NVCHR_TYPE:
	 if (db_stat = adu_lenaddr(adf_scb, dv1, &tmp_size, &p))
	 return(db_stat);
	 break;
	default:	/* all the others */
	 tmp_size = dv1->db_length;	/* fixed length */
	 break;
    }

    tmp_size *= BITSPERBYTE;

    if (rdv->db_length == 2)
	*(i2 *)rdv->db_data = (i2)tmp_size;
    else
	*(i4 *)rdv->db_data = tmp_size;
    
#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_24bitlength: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_25position() - Returns the position of the first occurrence of
**			    'dv1' within 'dv2'. This is the ANSI standard locate.
**
** Description:
**	This routine returns the position of the first occurrence of 'dv1'
**	within 'dv2'. 'dv1' can be any number of characters.  If not found, it
**	will return 0. This retains trailing blanks in the matching string.  
**	The empty string is found (by definition) at offset 1. Note: the difference
**	between 'locate' and 'position' is in the handling of the empty
**	search string and an unfound search string.
**
**	This routine works for all INGRES character types: c, text, char, and
**	varchar.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be found.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to search string.
**	dv2				Ptr to DB_DATA_VALUE containing string
**					being searched.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string to be searched.
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
**	rdv				Ptr to result DB_DATA_VALUE.  Note, it
**					is assumed that rdv's type will be
**					DB_INT_TYPE and its length will be 2.
**	    .db_data			Ptr to an i2 which holds the result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	One of the operands was not a string
**					type.
**
**  History:
**      09-nov-01 (inkdo01)
**	    Cloned from adu_8strlocate.
**	8-jan-02 (inkdo01)
**	    Renamed from adu_8aposition to conform and please Steve.
**	16-dec-03 (inkdo01)
**	    Propagated from 3.0 to main.
**	09-May-2007 (gupsh01)
**	    Added support for UTF8 character sets.
*/

DB_STATUS
adu_25strposition(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *rdv)
{
    DB_STATUS	    status;

    status =  adu_26positionfrom(adf_scb, dv1, dv2, (DB_DATA_VALUE *)0, rdv);
    return(status);
}


DB_STATUS
adu_26positionfrom(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE	    *dv_from,
DB_DATA_VALUE       *rdv)
{
    register char   *p1,
	            *p2;
    char	    *p1save,
		    *p2save,
		    *p1end,
		    *p2end,
		    *p2last,
		    *ptemp;
    i4		    size1,
		    size2,
		    pos = 1;
    i4		    found;
    DB_DT_ID	    bdt1;
    DB_DT_ID	    bdt2;
    DB_STATUS	    db_stat;
    i4		    start_pos;
    i4		    elmsize;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_26positionfrom: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
	if (dv_from)
	    adu_prdv(dv_from);
        adu_prdv(rdv);
    }
#endif

    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;

    /* FIX ME position nvchr with alternate collations not supported */
    /* FIX ME db_collID is not init here */
    if ((bdt1 == DB_NCHR_TYPE || bdt1 == DB_NVCHR_TYPE) &&
    	(dv1->db_collID > DB_UNICODE_COLL || dv2->db_collID > DB_UNICODE_COLL))
    {
	return(adu_error(adf_scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &ptemp)) != E_DB_OK)
	return (db_stat);
    p1save = ptemp;
    if ((db_stat = adu_lenaddr(adf_scb, dv2, &size2, &ptemp)) != E_DB_OK)
	return (db_stat);
    p2save = ptemp;

    found = FALSE;

    if (bdt1 == DB_NCHR_TYPE || bdt1 == DB_NVCHR_TYPE)
	elmsize = sizeof(UCS2);
    else if (bdt1 == DB_BYTE_TYPE || bdt1 == DB_VBYTE_TYPE ||
		    (((Adf_globs->Adi_status & ADI_DBLBYTE) == 0) &&
        	        ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) == 0)))
	elmsize = 1;
    else /* doublebyte, character size varies */
	elmsize = 0;

    /* p1last is one past the last position we should start a search from */
    p2last = p2save + max(0, size2 - size1 + 1);
    p1end  = p1save + size1;
    p2end  = p2save + size2;

    if (dv_from)
    {
	/* get the start position (the FROM value) */
	switch (dv_from->db_length)
	{
	    case 1:
		start_pos =  *(i1 *) dv_from->db_data;
		break;
	    case 2:
		start_pos =  *(i2 *) dv_from->db_data;
		break;
	    case 4:
		start_pos =  *(i4 *) dv_from->db_data;
		break;
	    default:
		return (adu_error(adf_scb, 
			E_AD9998_INTERNAL_ERROR, 2, 0, "pos start len"));
	}
    }
    else
	start_pos = 1;

    if (size1 > 0)	/* The empty string will be found at the start */
    {
	/* Do FROM processing */
	if (start_pos > pos)
	{
	    if (elmsize)
	    {
		p2save += (start_pos - pos) * elmsize;
		pos = start_pos;
	    }
	    else
	    {
		while (pos != start_pos && p2save < p2last)
		{
		    CMnext(p2save);
		    pos++;
		}
	    }
	}

	while (p2save < p2last  &&  !found)
	{
	    p1 = p1save;
	    p2 = p2save;
	    
	    if (!MEcmp(p1, p2, size1))
	    {
		p1 = p1end;
		found = TRUE;
		break;
	    }

	    /* No match */
	    if (elmsize)
	    {
		p2save += elmsize;
		pos++;
	    }
	    else
	    {
		CMnext(p2save);
		pos++;
	    }
	}
    }
    else found = TRUE;	/* empty string - leave pos = 1 */
	    
    if (!found)	    /* if not found, make pos = size plus 1 */
	pos = 0;		/* not found - pos = 0 */

    if (rdv->db_length == 2)
	*(i2 *)rdv->db_data = pos;
    else
	*(i4 *)rdv->db_data = pos;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_25strposition: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (E_DB_OK);
}

/*
** Name: adu_strextract() - Extract a substring from a string.
**
** Description:
**	This routine implementats string extraction to consistantly support:
**	 SUBSTRING(char FROM int FOR int)
**	 SUBSTRING(char FROM int)
**       CHAREXTRACT(char, int)
**       LEFT(char, int)
**       RIGHT(char, int)
**
**      NOTE: This function can also be used to support the SUBSTR(s,p,l)
**            of other vendors where a negative start point then counts from the
**            right. SUBSTRING(s FROM p [FOR n]) is strictly defined as left only.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE containing string to
**					do substring from.
**	    .db_datatype		Its datatype (must be a string type).
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.
**      startpos                        The FROM operand
**                                      If negative, count from right hand end.
**                                      If 0 treat as for 1
**      forlen                          The FOR operand
**                                      Must not be negative.
**	rdv				DB_DATA_VALUE for result.
**	    .db_datatype		Datatype of the result.
**	    .db_length			Length of the result.
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
**	rdv
**	    .db_data			Ptr to area to hold the result string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**
**  History:
**	24-Apr-2007 (kiria01) b118215
**	    Created to unify and correct multi-byte handling and behaviour
**          of SUBSTRING(S,I,[I]), CHAREXTRACT(S,I), LEFT(S,I) and RIGHT(S,I).
**          The SUBSTRINGs and CHAREXTRACT did not handle multibyte data and
**          LEFT and RIGHT handled BYTE types differently. Also undefined
**          was behaviour at boundaries such as startpos <= 0.
**	20-Jan-2009 (kiria01) b121520
**	    Ensure BYTEextract able to pull apart UTF8
**	02-Mar-2009 (kiria01) b121520
**	    Correct prior change by actually changing the return type
**	    as it is ultimatly needed to avoid UTF8 interferance with
**	    single characters.
*/

static DB_STATUS
adu_strextract (
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv_src,
i4	        startpos,
i4              forlen,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS           db_stat;
    char		*dest;
    char		*dest0;
    char		*enddest;
    i4			lendest;
    char		*src;
    char		*endsrc;
    i4			lensrc;
    bool                multi_byte = FALSE;

    if (db_stat = adu_lenaddr(adf_scb, dv_src, &lensrc, &src))
	return (db_stat);
    if (db_stat = adu_3straddr(adf_scb, rdv, &dest))
	return (db_stat);
    dest0 = dest;

    lendest = rdv->db_length;
    switch(rdv->db_datatype)
    {
      /* Variable length return datatypes */
      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
	if ((Adf_globs->Adi_status & ADI_DBLBYTE) ||
            (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
	    multi_byte = TRUE;
	/*FALLTHROUGH*/
      case DB_VBYTE_TYPE:
	lendest -= DB_CNTSIZE;
	/*FALLTHROUGH*/
      case DB_BYTE_TYPE:
	break;
      default:
	if (Adf_globs->Adi_status & ADI_DBLBYTE)
	    multi_byte = TRUE;
    }
    endsrc = src + lensrc;
    enddest = dest + lendest;

    /*
    ** We need locate start position and then copy the requisite number of charaters
    **  LEFT(s, l)          -> STREXTRACT(s, 1, l)
    **  RIGHT(s, l)         -> STREXTRACT(s, -1, l)
    **  SUBSTRING(s, p)     -> STREXTRACT(s, p, LEN(s)-p+1)
    **  SUBSTRING(s, p, l)  -> STREXTRACT(s, p, l)
    **  CHAREXTRACT(s, p)   -> STREXTRACT(s, p, 1)
    **  STREXTRACT(s, -p, l)-> STREXTRACT(s, LEN(s)-p+1, l)
    */

    /* We will adjust startpos to be 0 origin and cater for going from right (-ve) */
    if (startpos < 0)
    {
	/* Negative startpos so we need to locate from RHS */
	i4 rstartpos = -startpos-1;

	if (multi_byte)
	{
	    i4 char_len = 0;
	    char *src1 = src;

	    while (src1 + CMbytecnt(src) < endsrc)
	    {
		CMnext(src1);
		char_len++;
	    }
	    if (src1 < endsrc)
	       char_len++;
	    /* We need to skip char_len - forlen chars */
	    startpos = char_len - rstartpos - forlen;
	}
	else
	{
	    startpos = lensrc - rstartpos - forlen;
	}
	if (startpos < 0)
	    startpos = 0;
    }
    else if (startpos > 0)
    {
	startpos--;
    }

    if (multi_byte)
    {
	while (startpos-- && src < endsrc)
	    src += CMbytecnt(src);
	/* Copy the characters if any left */
	while (forlen-- &&  src < endsrc && dest + CMbytecnt(src) <= enddest)
	   CMcpyinc(src, dest);
    }
    else if (startpos < lensrc)
    {
        if (forlen > lensrc - startpos)
            forlen = lensrc - startpos;
	if (forlen > lendest)
	    forlen = lendest;
	MEcopy((PTR) src + startpos, (u_i2) forlen, (PTR) dest0);
	dest += forlen;
    }

    /* Sort out length representation */
    switch (rdv->db_datatype)
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
        /* pad with blanks if necessary */
	while (dest < enddest)
	    *dest++ = ' ';
	break;

      case DB_BYTE_TYPE:
        /* pad with zeroes if necessary */
	while (dest < enddest)
	    *dest++ = '\0';
	break;

      case DB_VCH_TYPE:
      case DB_VBYTE_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
	/* Just adjust the var length */
        ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = dest - dest0;
        break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    return (E_DB_OK);
}


/*
** Name: adu_varbyte_2arg() - Convert a data value to byte data value.
**
** Description:
**      Support the two argument version of string functions VARBYTE
**      Simply call adu_2alltobyte to do the real work.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**      dv1                             DB_DATA_VALUE describing Ingres datatype
**					to be converted.
**	    .db_datatype		Its type.
**	    .db_prec			Its precision/scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to the data to be converted into
**					a character string.
**	rdv				DB_DATA_VALUE describing character
**					string result.
**	    .db_length			The maximum length of the result.
**	    .db_data			Ptr to buffer to hold converted string.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	rdv
**	    .db_data			The converted string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**
**  History:
**     19-jan-2007 (stial01)
**         Created from adu_ascii_2arg
*/
DB_STATUS
adu_varbyte_2arg(
ADF_CB			*adf_scb,
DB_DATA_VALUE	        *dv1,
DB_DATA_VALUE	        *dv2,
DB_DATA_VALUE		*rdv)
{
    ADF_STRTRUNC_OPT  input_strtrunc=adf_scb->adf_strtrunc_opt;
    STATUS            status;

    adf_scb->adf_strtrunc_opt = ADF_IGN_STRTRUNC;
    if (dv1->db_datatype == DB_NCHR_TYPE || dv1->db_datatype == DB_NVCHR_TYPE)
	status = adu_nvchr_coerce(adf_scb, dv1, rdv);
    else
	status = adu_2alltobyte(adf_scb, dv1, rdv);
    adf_scb->adf_strtrunc_opt = input_strtrunc;
    return( status);
}

/*
** Name: adu_strgenerate_digit() - Generate a check digit for a string based
**                                 on a nominated encoding scheme.
**
** Description:
**    (char ) generate_digit((varchar )scheme, (varchar )string);
**         Generates the check digit for the string using the nominated
**         encoding scheme. The check digit may be employed as a means to 
**         determine if a string has been entered incorrectly. But it does not
**         provide error correction capability.
**
**         The check digit for a string is a character computed from the other
**         characters in the string. Typically, the check digit is a single
**         character and is included as the last character in the string.
**
**         There are several error types that a check digit may be able to 
**         detect. Specifically:
**         - mistyped characters: eg '0123' not '0124'
**         - ommissions or additions of characters.
**         - adjacent transposition errors: eg '01234' not '01243'
**         - twin errors: eg '11' becomes '22'
**         - jump transpositions: 123 becomes 321
**         - jump twin errors: 121 becomes 323
**         - phonetic errors: ie. the mixing of similar sounding numbers
**           eg. sixty '60' not sixteen '16'
**         
**         There are many check digit schemes of varying complexity, purpose and
**         abilities. For example the Luhn algorithm is widely used in credit
**         card numbers and is programatically simple to implement. But the
**         Verhoeff algorithm is considered superior in detection of certain
**         error types such as jump transpositions. However it is a more
**         complicated algorithm.
**
**         A brief description of each supported check digit scheme follows.
**
**         NOTE:
**         1. The check digit is returned as a varchar(2) item.
**            It is not appended to the string.
**         2. string formatting.
**             Although many schemes use whitespace, dashes etc to seperate
**             their strings into 'blocks'. This implementation demands that
**             such seperators are removed from the strings.
**
** Supported Encoding Schemes. 
** Supported schemes are as listed below. Note that the scheme name may be in
** mixed case.
** Scheme:     Description (Brief)
** EAN:        European Article Number
**             Refer to: http://en.wikipedia.org/wiki/EAN-13
**                Synonyms: EAN_13, GTIN, GTIN_13, JAN
**                Subversions: EAN_8, GTIN_8, RCN_8.
**                Acronyms: JAN (Japanese Article Number)
**                          GTIN (Global Trade Identification Number) 
**
**             The EAN is a barcoding standard which is a superset of the 
**             original 12 digit Universal Product Code (UPC) system developed 
**             in North America. The EAN_13 barcode is defined by the standards
**             organisation GS1. It is also called a Japanese Article Number
**             (JAN) in Japan. UPC, EAN, and JAN numbers are collectively called
**             Global Trade Item Numbers (GTIN), though they can be expressed
**             in different types of barcodes.
**
**             The EAN_13 and EAN_8 codes have internal formatting. For
**             example an EAN_13 is split into a GS1 Prefix, The Company Number,
**             an Item refrence and the check digit. 
**             Currently this code will only calculate the check digit.
**             An enhancement may be to enforce a validation of the GS1
**             Prefix.
**
**             The EAN_13 barcodes are used worldwide for marking retail goods.
**             The less commonly used EAN_8 barcodes are used also for marking
**             retail goods; They are derived from the EAN_13's and are
**             intended for smaller packages where a full length EAN_13 would 
**             be too cumbersome...as per UPC_E's.
**
**             The EAN_8 may also be used internal to companies to identify
**             restricted or 'own-brand' products to be sold within their stores
**             only. These are often referred to with the synonym: RCN_8. Note
**             that RCN_8's must begin with a 0(zero) or 2(two), but as with
**             the GS1 Prefix, this will not be enforced. An enhancement
**             may choose to enforce this.
**   
** ISBN        International Standard Book Number
** ISBN_13     International Standard Book Number 13 character version.
**             Refer To: http://en.wikipedia.org/wiki/ISBN
**             The standard ISBN is a 10 character numeric identifier for 
**             printed music. Since 1st January, 2007, the ISBN have been 13 
**             digits. Note that these are compatible with EAN_13.
**
**             The check digit may be either a digit or the alpha 'X'.
**
**             For example:
**             9992158107
**             9971502100
**             080442957X
** 
** ISSN:       International Standard Serial Number
**             Refer To: http://en.wikipedia.org/wiki/ISSN
**             ISSNs are 8 digit numbers used to identify electronic or print
**             periodicals.
**             Example: The Journal of Hearing Research: 0378-5595
** 
** LUHN        The LUHN algorithm
**             Refer To: http://en.wikipedia.org/wiki/Luhn_algorithm
**             Created by IBM scientist Hans Peter Luhn. Most credit cards and
**             many government identification numbers are based on this simple
**             algorithm.
**
**             For example:
**             1. Credit Card numbers are an implementation of the Luhn
**                Algorithm, but the number is restricted to the range of 13 to
**                19 characters.
**             2. The Canadian Social Insurance Number (SIN) is a 9 digit
**                  number which implements the Luhn Algorithm.
**
**             The string may be of any length, but must be composed entirely
**             of digits.
**
** LUHN_A      This is an extension of the Luhn Algorithm which allows for 
**             alphabetic characters by assigning numbers 10 to 'A', 11 to 'B'
**             ...35 to 'Z'. Note that it is case insensitive.
**             
**             Examples of usage are:
**             1. The Committee on Uniform Security Identification Procedures
**                (CUSIP) provides a 9-character alphanumeric string based on
**                this algorithm.
**
**             2. The International Securities Identification Number (ISIN) is a
**                12 character version of this algorithm.
**
** UPC:        Universal Product Code
**             Refer to: http://en.wikipedia.org/wiki/Universal_Product_Code
**             Used to identify products to bar code readers.
**
**             UPC or UPC_A encodes 12 digits. Note that a UPC_A number is also
**             an EAN_12 number. Furthermore if the UPC_A is prefixed with a '0'
**             it becomes an EAN_13.
**     
**             UPC_E encodes 6 letters and were intended for smaller packages
**             where a full size UPC_A would be cumbersome. Unlike the UPC_A
**             the check digit is not appended to the number. Rather, it is used
**             to determine the 'odd/even' parity assigned to the numbers for
**             when they are encoded as bar-code lines.
**
** VERHOEFF    The Verhoeff Algorithm as described in Wikipedia.
** VERHOEFFNR: The Verhoeff Algorithm as described in Numerical Recipes.
**             Refer To:
**                 http://en.wikipedia.org/wiki/Verhoeff_Algorithm.
**                 http://www.augustana.ab.ca/~mohrj/algorithms/checkdigit.html
**             Developed by Dutch Mathematician Jacobus Verhoeff. This algorithm
**             detects all transposition errors as well as other types of error
**             that would not be detected by the Luhn Algorithm. The algorithm
**             is based on the properties of a non-commutative system of
**             operations on ten elements.
**
**             The Verhoeff Algorithm, like the Luhn algorithm can also handle
**             strings of any length. But the strings must be composed
**             entirely of digits.
**
**             NOTE: The algorithm described in wikipedia, implemented here and
**                   corroborated by the augustana site differs from that found
**                   in Numerical Recipes.  Hence I have implemented VERHOEFFNR
**                   which is the verhoeff algorith as published in Numerical 
**                   Recipes.
**
** Inputs:
**    adf_scb    Pointer to an ADF session control block.
**  scheme     DB_DATA_VALUE containing encoding scheme name as a varchar
**             string.
**  string     DB_DATA_VALUE containing varchar string to generate the check
**             digit for.
**
** Outputs:
**    adf_scb    Pointer to an ADF session control block.
**    rdv        DB_DATA_VALUE for resulting check digit.
**
**    Returns:
**        The following DB_STATUS codes may be returned:
**        E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**          If a DB_STATUS code other than E_DB_OK is returned, the caller
**        can look in the field adf_scb.adf_errcb.ad_errcode to determine
**        the ADF error code.  The following is a list of possible ADF error
**        codes that can be returned by this routine:
**
**        E_AD0000_OK            Routine completed successfully.
**        E_AD9999_INTERNAL_ERROR     The returns DB_DATA_VALUE or db_length
**                    was inappropriate type for this operation.
**
**  History:
**    11-Jan-2009 (martinb) - Created to support schemes 'Verhoeff',
**      'VerhoeffNR' and 'Luhn'.
*/

#define CHECK_DIGIT_NUMERIC 0
#define CHECK_DIGIT_ALPHA_NUMERIC 1
#define CHECK_DIGIT_SCHEME_LENGTH 32
enum CHECK_DIGIT_SCHEMES
{
    UNKNOWN_SCHEME,
    EAN_12, EAN_13, EAN_8,
    ISBN,
    ISBN_13,
    ISSN,
    LUHN,
    LUHN_A,
    UPC, UPC_E,
    VERHOEFF,
    VERHOEFFNR,
};

/*
** generate_cd_scheme():
**     This is used by the adu_strgenerate_digit() and adu_strvalidate_digit()
**     functions to vet the scheme_name provided. This function has assumed
**     that when called the scheme_name has already had some vetting and been 
**     properly terminated.
*/
enum CHECK_DIGIT_SCHEMES
generate_cd_scheme(char *scheme_name)
{
    if (!STcasecmp(scheme_name, "ean_12")) return EAN_12;

    if (!STcasecmp(scheme_name, "ean")
     || !STcasecmp(scheme_name, "ean_13")
     || !STcasecmp(scheme_name, "gtin")
     || !STcasecmp(scheme_name, "gtin_13")
     || !STcasecmp(scheme_name, "jan")) return EAN_13;

    if (!STcasecmp(scheme_name, "ean_8")
     || !STcasecmp(scheme_name, "rcn_8")
     || !STcasecmp(scheme_name, "gtin_8")) return EAN_8;

    if (!STcasecmp(scheme_name, "isbn")) return ISBN;

    if (!STcasecmp(scheme_name, "isbn_13")) return ISBN_13;

    if (!STcasecmp(scheme_name, "issn")) return ISSN;

    if (!STcasecmp(scheme_name, "luhn")) return LUHN;

    if (!STcasecmp(scheme_name, "luhn_a")) return LUHN_A;

    if (!STcasecmp(scheme_name, "upc")
     || !STcasecmp(scheme_name, "upc_a")) return UPC;
    
    if (!STcasecmp(scheme_name, "upc_e")) return UPC_E;
    
    if (!STcasecmp(scheme_name, "verhoeff")) return VERHOEFF;
    
    if (!STcasecmp(scheme_name, "verhoeffnr")) return VERHOEFFNR;

    return UNKNOWN_SCHEME;
}

DB_STATUS
adu_strgenerate_digit(
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *scheme, 
    DB_DATA_VALUE    *string, 
    DB_DATA_VALUE    *rdv)
{
    i2 scheme_length, string_length;
    char scheme_name[CHECK_DIGIT_SCHEME_LENGTH + 1];
    static char valgen[]="generate"; /* passed to error string when required */
    enum CHECK_DIGIT_SCHEMES cd_scheme;

    /* Confirm input data types are acceptable */
    if (scheme->db_datatype != DB_VCH_TYPE)
        return(adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    if (string->db_datatype != DB_VCH_TYPE)
        return(adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    /* Confirm output datatype matches expectations */
    if (rdv->db_datatype != DB_VCH_TYPE 
      ||rdv->db_length != 2 + sizeof(i2))
        /* Somethings gone way wrong */
        return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

    /* Confirm scheme is supported */
    scheme_length=*(i2 *)scheme->db_data;
    if (scheme_length > CHECK_DIGIT_SCHEME_LENGTH)
    {
        STncpy(scheme_name, (char *)scheme->db_data + sizeof(i2), CHECK_DIGIT_SCHEME_LENGTH);
        scheme_name[CHECK_DIGIT_SCHEME_LENGTH] = EOS;
        return(adu_error(adf_scb, E_AD2055_CHECK_DIGIT_SCHEME, 2,
            CHECK_DIGIT_SCHEME_LENGTH, scheme_name
            ));
    }
    /* Whinge if input string is empty.  */
    string_length=*(i2 *)string->db_data;
    if (string_length == 0)
        return(adu_error(adf_scb, E_AD2057_CHECK_DIGIT_EMPTY, 2,
            STlength(valgen), valgen
            ));

    /*
    ** Pass off to the dedicated routines for the individual encoding schemes.
    */
    STncpy(scheme_name, (char *)scheme->db_data + sizeof(i2), scheme_length);
    scheme_name[scheme_length]='\0';

    cd_scheme = generate_cd_scheme(scheme_name);
    switch (cd_scheme)
    {
    case ISBN_13:
    case EAN_13:
        return(generate_ean_digit(adf_scb, string, rdv, 12));

    case EAN_8:
        return(generate_ean_digit(adf_scb, string, rdv, 7));

    case ISBN:
        return(generate_isXn_digit(adf_scb, string, rdv, 9));

    case ISSN:
        return(generate_isXn_digit(adf_scb, string, rdv, 7));

    case LUHN:
        return(generate_luhn_digit(adf_scb, string, rdv, CHECK_DIGIT_NUMERIC));

    case LUHN_A:
        return(generate_luhn_digit(adf_scb, string, rdv, CHECK_DIGIT_ALPHA_NUMERIC));

    case EAN_12:
    case UPC:
        return(generate_ean_digit(adf_scb, string, rdv, 11));

    case UPC_E:
        return(generate_upce_digit(adf_scb, string, rdv));

    case VERHOEFF:
        return(generate_verhoeff_digit(adf_scb, string, rdv));

    case VERHOEFFNR:
        return(generate_verhoeffNR_digit(adf_scb, string, rdv));

    case UNKNOWN_SCHEME:
    default:
        /* Houston, we have a problem! */
        return(adu_error(adf_scb, E_AD2055_CHECK_DIGIT_SCHEME, 2,
            scheme_length, scheme_name));
    }
} /* adu_strgenerate_digit */

/*
**    (int1 ) validate_digit((varchar )scheme, (varchar )string);
**         Created 24th September, 2008.
**         Validates the checksum in the string for the nominated scheme.
**         The function returns 1 for valid and 0 for invalid.
**
**         NOTE:
**         1. validate_digit() has the same caveats on scheme_name and
**            strings as noted for generate_digit().
**         2. The temptation is to write a routine which simply checks the last
**            digit in the string matches what would be generated by the 
**            generate_digit() routine on the rest of the string. This is 
**            probably not a good idea...puts all the eggs in one basket. 
**
*/
DB_STATUS
adu_strvalidate_digit(
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *scheme,
    DB_DATA_VALUE    *string,
    DB_DATA_VALUE    *rdv
    )
{
    i2 scheme_length, string_length;
    char scheme_name[CHECK_DIGIT_SCHEME_LENGTH + 1];
    static char valgen[]="validate"; /* passed to error string when required */
    enum CHECK_DIGIT_SCHEMES cd_scheme;

    /* Confirm input data types are acceptable */
    if (scheme->db_datatype != DB_VCH_TYPE)
        return(adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    if (string->db_datatype != DB_VCH_TYPE)
        return(adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    /* Confirm output datatype matches expectations */
    if (rdv->db_datatype != DB_INT_TYPE
     || rdv->db_length != 1)
        /* Somethings gone way wrong */
        return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

    /* Confirm scheme is supported. */
    scheme_length=*(i2 *)scheme->db_data;
    if (scheme_length > CHECK_DIGIT_SCHEME_LENGTH)
    {
        STncpy(scheme_name, (char *)scheme->db_data + sizeof(i2), CHECK_DIGIT_SCHEME_LENGTH);
        scheme_name[CHECK_DIGIT_SCHEME_LENGTH] = EOS;
        return(adu_error(adf_scb, E_AD2055_CHECK_DIGIT_SCHEME, 2,
            CHECK_DIGIT_SCHEME_LENGTH, scheme_name));
    }

    /* Whinge if input string is empty. */
    string_length=*(i2 *)string->db_data;
    if (string_length == 0)
        return(adu_error(adf_scb, E_AD2057_CHECK_DIGIT_EMPTY, 2,
            STlength(valgen), valgen
            ));

    /*
    ** Pass off to the dedicated routines for the individual encoding
    ** schemes.
    */
    STncpy(scheme_name, (char *)scheme->db_data + sizeof(i2), scheme_length);
    scheme_name[scheme_length] = EOS;

    cd_scheme = generate_cd_scheme(scheme_name);
    switch (cd_scheme)
    {
    case ISBN_13:
    case EAN_13:
        return(validate_ean_digit(adf_scb, string, rdv, 13));

    case EAN_8:
        return(validate_ean_digit(adf_scb, string, rdv, 8));

    case ISBN:
        return(validate_isXn_digit(adf_scb, string, rdv, 10));

    case ISSN:
        return(validate_isXn_digit(adf_scb, string, rdv, 8));

    case LUHN:
        return(validate_luhn_digit(adf_scb, string, rdv, CHECK_DIGIT_NUMERIC));

    case LUHN_A:
        return(validate_luhn_digit(adf_scb, string, rdv, CHECK_DIGIT_ALPHA_NUMERIC));

    case EAN_12:
    case UPC:
        return(validate_ean_digit(adf_scb, string, rdv, 12));

    case UPC_E:
        return(validate_upce_digit(adf_scb, string, rdv));

    case VERHOEFF:
        return(validate_verhoeff_digit(adf_scb, string, rdv));

    case VERHOEFFNR:
        return(validate_verhoeffNR_digit(adf_scb, string, rdv));

    case UNKNOWN_SCHEME:
    default:
        /* Houston, we have a problem! */
        return(adu_error(adf_scb, E_AD2055_CHECK_DIGIT_SCHEME, 2,
            scheme_length, scheme_name));
    }
} /* validate_digit */

DB_STATUS
generate_isXn_digit(
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *p1,
    DB_DATA_VALUE    *rdv,
    i4               isXn_length
    )
{
    /*
    ** ISBN are 10 character strings in the format: [0-9]{9}C
    ** eg. 030640615C, C should caclulate to 2
    ** ISSN are 8 character strings in the format:  [0-9]{7}C
    ** eg. 0378-595C, C should calculate to be 5
    **
    ** This routine will accept only a string, of the correct length and it
    ** must be entirely composed of digits.
    **
    ** It will then generate the appropriate value C for the check digit.
    ** The check digit will be a number or the character 'X'.
    */
    u_i2 length_of_int, valid_length = isXn_length;
    i4 i, n, char_number, sum;

    *(i2 *)rdv->db_data = 1; /* this is a single character check digit */

    length_of_int = *(u_i2 *)p1->db_data;
    if (length_of_int != valid_length)
        return(adu_error(adf_scb, E_AD2058_CHECK_DIGIT_STRING_LENGTH, 4,
            sizeof(i2), &length_of_int,
            sizeof(i2), &valid_length));

    for (char_number = 2, sum = 0, i = length_of_int - 1;
      i >=0;
      --i, char_number++)
    {
	char *c = (char *)(p1->db_data+sizeof(i2) + i);
        if (!CMdigit(c))
            return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
        else
            n = (i4 )*c - (i4 )'0';

        sum += char_number*n;
    } /* for each character */

    sum--;
    n = (i4 )(10 - (sum % 11));
    if (n == 10)
        *(rdv->db_data + sizeof(i2)) = 'X';
    else 
        *(rdv->db_data + sizeof(i2)) = (char )(n + (i4 )'0');
    return(E_DB_OK);
} /* generate_isXn_digit */

DB_STATUS
validate_isXn_digit(
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *p1,
    DB_DATA_VALUE    *rdv,
    i4               isXn_length
    )
{
    /*
    ** ISBN are 10 character strings in format: [0-9]{9}C; where C =~[0-9X].
    ** eg. 030640615C, C should caclulate to 2
    ** ISSN are 9 character strings in format: [0-9]{8}C; where C=~[0-9X]
    ** eg. 0378-5955C, C should calculate to be 5
    */
    u_i2 length_of_int, valid_length = isXn_length;
    i4 i, n, char_number, sum;

    length_of_int = *(u_i2 *)p1->db_data;
    if (length_of_int != valid_length)
        return(adu_error(adf_scb, E_AD2058_CHECK_DIGIT_STRING_LENGTH, 4,
            sizeof(i2), &length_of_int,
            sizeof(i2), &valid_length));

    for (char_number = 1, sum = 0, i = length_of_int - 1;
      i >=0;
      --i, char_number++)
    {
	char *c = (char *)(p1->db_data+sizeof(i2)+i);
        if (!CMdigit(c))
        {
            if (char_number == 1 && (*c == 'X' || *c == 'x'))
                n = 10; /* ie. 'X' or 'x' in the rightmost position */
            else
                return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
        }
        else
        {
            n = (i4 )*c - (i4 )'0';
        }

        sum += char_number*n;
    } /* for each character */

    if (sum % 11 == 0)
        *(rdv->db_data) = (u_char )1;
    else
        *(rdv->db_data) = (u_char )0;

    return(E_DB_OK);
} /* validate_isXn_digit */

DB_STATUS
generate_ean_digit(
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *p1,
    DB_DATA_VALUE    *rdv,
    i4               ean_length
    )
{
    /*
    ** EAN numbers are similar to IBM numbers, but:
    ** 1. Use a weighting factor of three not two for the digits in odd
    **    positions (counting from the right).
    ** 2. When a number multiplied by the weighting factor is greater than 10
    **    there is no attempt made to morph the number back to something < 10
    **
    ** ISBN_13 == EAN_13
    **     For example, the ISBN_13 check digit of 978030640615C
    **        The check digit 'C' is 7.
    */
    u_i2 length_of_int;
    i4 i, n, alternate, sum, weighting;

    *(i2 *)rdv->db_data = 1; /* this is a single character check digit */

    length_of_int = *(u_i2 *)p1->db_data;
    if (length_of_int != ean_length)
        return(adu_error(adf_scb, E_AD2058_CHECK_DIGIT_STRING_LENGTH, 4,
            sizeof(i2), &length_of_int,
            sizeof(i4), &ean_length));

    if (ean_length == 2)
    {
        weighting = 10;
        alternate = 0;
    }
    else
    {
        weighting = 3;
        alternate = 1;
    }

    for (sum = 0, i = length_of_int - 1; i >=0; i--)
    {
	char *c = (char *)(p1->db_data+sizeof(i2)+i);
        if (!CMdigit(c))
            return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
        else
            n = (i4 )*c - (i4 )'0';

        if (alternate) n *= weighting;
        sum += n;
        alternate = !alternate;
        if (ean_length == 5)
        {
            if (weighting == 3) 
                weighting = 9;
            else
                weighting = 3;
        }
    } /* for each character */

    if (ean_length == 2)
    {
        n = sum % 4;
    }
    else
    {
        n = sum % 10;
        if (n != 0 && ean_length !=5)
            n = 10-n;
    }
    *(rdv->db_data + sizeof(i2)) = (char )(n + (i4 )'0');
    return(E_DB_OK);
} /* generate_ean_digit */

DB_STATUS
validate_ean_digit(
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *p1,
    DB_DATA_VALUE    *rdv,
    i4               ean_length
    )
{
    /*
    ** EAN numbers are similar to IBM numbers, but:
    ** 1. Use a weighting factor of three not two for the digits in odd
    **    positions (counting from the right).
    ** 2. When a number multiplied by the weighting factor is greater than 10
    **    there is no attempt made to morph the number back to something  < 10
    **
    ** For EAN-2 and EAN-5:
    **     The checksum is not appended to the number, ergo we have nothing
    **     to check other than the number is of the correct length and 
    **     composed entirely of digits.
    **
    ** For all other lengths:
    **     Input formats permitted: [0-9]{ean_length-1}C
    **     All characters are digits. The last digit 'C' being the checksum.
    **
    **     It will then check that the check digit 'C' is correct.
    */
    u_i2 length_of_int, valid_length = ean_length;
    i4 i, n, alternate, sum;

    length_of_int = *(u_i2 *)p1->db_data;
    if (length_of_int != valid_length)
        return(adu_error(adf_scb, E_AD2058_CHECK_DIGIT_STRING_LENGTH, 4,
            sizeof(i2), &length_of_int,
            sizeof(i2), &valid_length));

    for (alternate = 0, sum = 0, i = length_of_int - 1; i >=0; --i)
    {
	char *c = (char *)(p1->db_data+sizeof(i2)+i);
        if (!CMdigit(c))
            return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
        else
            n = (i4 )*c - (i4 )'0';

        if (alternate) n*=3;
        sum += n;
        alternate = !alternate;
    } /* for each character */

    if (sum % 10 == 0 || ean_length == 2 || ean_length == 5)
        *(rdv->db_data) = (u_char )1;
    else
        *(rdv->db_data) = (u_char )0;
    return(E_DB_OK);
} /* validate_ean_digit */

DB_STATUS
generate_luhn_digit(
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *p1,
    DB_DATA_VALUE    *rdv,
    i4               permit_alphas
    /*
    ** If alphas are permitted, the code is case insensitive. It will map the
    ** characters to numbers as follows:
    ** 'a'==10, 'b'==11, ... 'z'==35
    ** The resulting number is treated as two characters in the input string.
    **
    ** The algorithm multiplies alternate characters by 2. 
    ** If the result of any single multiplication is > 9 then it subtracts 9.
    ** ie. 10 becomes 1, 12 becomes 3, 14 becomes 5, 16 becomes 7, 18 becomes 9,
    */
    )
{
    u_i2  length_of_int;
    i4    i, x, y, n, alternate, sum;

    *(i2 *)rdv->db_data = 1; /* this is a single character check digit */

    length_of_int = *(u_i2 *)p1->db_data;

    for (alternate = 1, sum = 0, i = length_of_int - 1; i >= 0; --i)
    {
	char *c = (char *)(p1->db_data+sizeof(i2)+i);
        if (!CMdigit(c))
        {
            if (!permit_alphas)
            {
                return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
            }
            if (CMupper(c))
            {
                n = (i4)*c - (i4)'A' + 10;
            }
            else
            {
                if (CMlower(c))
                    n = (i4)*c - (i4)'a' + 10;
                else
                    return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, &c));
            }
            y = (i4)n/10;
            x = n - 10*y; /* ie n==yx */
            if (alternate)
            {
                x *= 2;
                if (x > 9) x = x-9;
                sum += x+y;
            }
            else
            {
                y *= 2;
                if (y > 9) y = y-9;
                sum += x+y;
            }
        }
        else
        { /* A numeric character */
            n = (i4)*c - (i4)'0';
            if (alternate)
		n *= 2;
            if (n > 9)
		n = n-9;
            sum += n;
            alternate = !alternate;
        }
    } /* foreach character in string */

    /*
    ** Build the rdv...
    ** Build the tens complement of the final digit in the sum
    */
    n = sum % 10;
    if (n != 0) n = 10-n;
    *(rdv->db_data + sizeof(i2)) = (char)(n + (i4)'0');
    return(E_DB_OK);
} /* generate_luhn_digit */

DB_STATUS
validate_luhn_digit(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *p1, 
    DB_DATA_VALUE *rdv,
    i4            permit_alphas
    /*
    ** If alphas are permitted, the code is case insensitive. It will map the
    ** characters to numbers as follows:
    ** 'a'==10, 'b'==11, ... 'z'==35
    */
    )
{
    i2   length_of_int;
    i4   i, x, y, n, alternate, sum;

    length_of_int=*(i2 *)p1->db_data;
    for (alternate = 0, sum = 0, i = length_of_int - 1; i >=0; --i) 
    {
	char *c = (char *)(p1->db_data+sizeof(i2)+i);
        if (!CMdigit(c))
        {
            if (!permit_alphas)
                return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
            if (CMupper(c))
            {
                n = (i4)*c - (i4)'A' + 10;
            }
            else
            {
                if (CMlower(c))
                    n = (i4)*c - (i4)'a' + 10;
                else
                    return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
            }

            y = (i4)n/10; x = n - 10*y; /* ie n==yx */
            if (alternate)
            {
                x *= 2;
                if (x > 9) x = x-9;
                sum += x+y;
            }
            else
            {
                y *= 2;
                if (y > 9) y = y-9;
                sum += x+y;
            }
        }
        else
        { /* A numeric character */
            n = (i4)*c - (i4)'0';
            if (alternate) n *= 2;
            if (n > 9) n = n - 9;
            sum += n;
            alternate = !alternate;
        }
    } /* Foreach character in string */

    if (sum % 10 == 0)
        *(rdv->db_data) = (u_char)1;
    else
        *(rdv->db_data) = (u_char)0;
    return(E_DB_OK);
} /* validate_luhn_digit */

DB_STATUS
generate_upce_digit(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *p1,
    DB_DATA_VALUE *rdv
    )
{
    /*
    ** See: http://en.wikipedia.org/wiki/Universal_Product_Code
    ** Also: http://www.morovia.com/education/utility/upc-ean.asp
    **
    ** UPC-E numbers are 6 characters long with no checkdigit. The check digit
    ** is encoded by using it to determine the odd/even parity of each number
    ** in the string when converted to barcode bars.
    ** 
    ** To generate the check digit we convert the number to a UPC-A format and
    ** then use that to generate the check digit as per normal rules for that
    ** format.
    **
    */
    u_i2 length_of_int, upce_length = 6;
    i4 i;
    char c;
    DB_DATA_VALUE workv;
    struct _vchar {
        short len;
        char string[11];
        } UPCA;

    *(i2 *)rdv->db_data = 1; /* this is a single character check digit */

    length_of_int = *(u_i2 *)p1->db_data;
    if (length_of_int != upce_length)
        return(adu_error(adf_scb, E_AD2058_CHECK_DIGIT_STRING_LENGTH, 4,
            sizeof(i2), &length_of_int,
            sizeof(i2), &upce_length));

    /* Initialise the workv */
    workv.db_data     = (char *)&UPCA;
    workv.db_datatype = DB_VCH_TYPE;
    workv.db_length   = (i4 )11;
    workv.db_prec     = (i2 )0;

    /* Initialise the UPCA struct */
    UPCA.len = (i2 )11;
    for (i = 0;i < 11;i++) UPCA.string[i] = '0';

    /*
    **  The number is converted to UPC-A by the following rules.
    ** Last   UPC-E     UPC-A equivalent is: 
    ** digit  012345     012345-6789A
    **  0     XXNNN0     0XX000-00NNN + check 
    **  1     XXNNN1     0XX100-00NNN + check 
    **  2     XXNNN2     0XX200-00NNN + check 
    **  3     XXXNN3     0XXX00-000NN + check 
    **  4     XXXXN4     0XXXX0-0000N + check 
    **  5     XXXXX5     0XXXXX-00005 + check 
    **  6     XXXXX6     0XXXXX-00006 + check 
    **  7     XXXXX7     0XXXXX-00007 + check 
    **  8     XXXXX8     0XXXXX-00008 + check 
    **  9     XXXXX9     0XXXXX-00009 + check 
    */
    UPCA.string[1]      = *(char *)(p1->db_data+sizeof(i2) + 0);
    UPCA.string[2]      = *(char *)(p1->db_data+sizeof(i2) + 1);
    c = *(char *)(p1->db_data+sizeof(i2) + 5);
    switch (c)
    {
    case '0': 
        UPCA.string[8]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[9]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[10] = *(char *)(p1->db_data+sizeof(i2) + 4);
        break;
    case '1':
        UPCA.string[3]  = (char )'1';
        UPCA.string[8]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[9]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[10] = *(char *)(p1->db_data+sizeof(i2) + 4);
        break;
    case '2':
        UPCA.string[3]  = (char )'2';
        UPCA.string[8]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[9]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[10] = *(char *)(p1->db_data+sizeof(i2) + 4);
        break;
    case '3':
        UPCA.string[3]  = *(char *)(p1->db_data+sizeof(i2) + 2);      
        UPCA.string[9]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[10] = *(char *)(p1->db_data+sizeof(i2) + 4);
        break;
    case '4':
        UPCA.string[3]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[4]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[10] = *(char *)(p1->db_data+sizeof(i2) + 4);
        break;
    case '5':
        UPCA.string[3]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[4]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[5]  = *(char *)(p1->db_data+sizeof(i2) + 4);
        UPCA.string[10] = (char )'5';
        break;
    case '6':
        UPCA.string[3]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[4]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[5]  = *(char *)(p1->db_data+sizeof(i2) + 4);
        UPCA.string[10] = (char )'6';
        break;
    case '7':
        UPCA.string[3]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[4]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[5]  = *(char *)(p1->db_data+sizeof(i2) + 4);
        UPCA.string[10] = (char )'7';
        break;
    case '8':
        UPCA.string[3]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[4]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[5]  = *(char *)(p1->db_data+sizeof(i2) + 4);
        UPCA.string[10] = (char )'8';
        break;
    case '9':
        UPCA.string[3]  = *(char *)(p1->db_data+sizeof(i2) + 2);
        UPCA.string[4]  = *(char *)(p1->db_data+sizeof(i2) + 3);
        UPCA.string[5]  = *(char *)(p1->db_data+sizeof(i2) + 4);
        UPCA.string[10] = (char )'9';
        break;
    default:
        return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, &c));
        break;
    } /*switch*/

    /*
    ** Call the generate_ean_digit() to get the correct check digit, 
    ** pass that routines return code and result back up the line.
    */    
    return(generate_ean_digit(adf_scb, &workv, rdv, (short )11));
} /* generate_upce_digit */

DB_STATUS
validate_upce_digit(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *p1,
    DB_DATA_VALUE *rdv
    )
{
    /*
    ** UPC-E numbers are unique in that the check digit is not part of the
    ** string. Its used to define the parity of the encoding of each number
    ** in the string, but is not otherwise present.
    **
    ** Hence as long as the string is 6 characters long and numeric, we
    ** consider it valid!
    */
    u_i2 length_of_int, upce_length = 6;
    i4 i;

    length_of_int = *(u_i2 *)p1->db_data;
    if (length_of_int != upce_length)
    {
        return(adu_error(adf_scb, E_AD2058_CHECK_DIGIT_STRING_LENGTH, 4,
            sizeof(i2), &length_of_int,
            sizeof(i2), &upce_length));
    }
    else
    {
        *(rdv->db_data)=(u_char )1;
        for (i = 0; i < 6; i++)
        {
	    char *c = (char *)(p1->db_data + sizeof(i2) + i);
            if (!CMdigit(c))
                return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
        }
    }
    return(E_DB_OK);
} /* validate_upce_digit */

DB_STATUS
generate_verhoeff_digit(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *p1,
    DB_DATA_VALUE *rdv
    )
{
    u_i2   length_of_int;
    i4   i, j, k, m, n_digit;
    char *data;

    static const i4 F[8][10]=
    { /* The permutations of each number */
        {0,1,2,3,4,5,6,7,8,9},
        {1,5,7,6,2,8,3,0,9,4},
        {5,8,0,3,7,9,6,1,4,2},
        {8,9,1,6,0,4,3,5,2,7},
        {9,4,5,3,1,2,6,8,7,0},
        {4,2,8,6,5,7,3,9,0,1},
        {2,7,9,3,8,0,6,4,1,5},
        {7,0,4,6,9,1,3,2,5,8}
    };

    static const i4 d[10][10]=
    { /* The results of multiplying [i] * [j] in D5 */
        {0,1,2,3,4,5,6,7,8,9},
        {1,2,3,4,0,6,7,8,9,5},
        {2,3,4,0,1,7,8,9,5,6},
        {3,4,0,1,2,8,9,5,6,7},
        {4,0,1,2,3,9,5,6,7,8},
        {5,9,8,7,6,0,4,3,2,1},
        {6,5,9,8,7,1,0,4,3,2},
        {7,6,5,9,8,2,1,0,4,3},
        {8,7,6,5,9,3,2,1,0,4},
        {9,8,7,6,5,4,3,2,1,0}
    };

    static const char inv[10] = {'0', '4', '3', '2', '1', '5', '6', '7', '8', '9'};

    /* Sundry Initialisation */
    j = 0; k = 0; m = 1;
    rdv->db_prec = (i2)0;

    *(i2 *)rdv->db_data = 1; /* this is a single character check digit */

    /* Get the input parameter length.
    ** And then read from 'data' instead of 'p1->db_data'.
    ** This routine will then be able to handle either CHAR or VARCHAR. When it
    ** is called internally later on - in validate_verhoeff_digit() - setup a 
    ** temporary dbv as DB_CHA_TYPE which points to the bit of the varchar you 
    ** want looked at. This avoids the need to adjust the VAR length of a
    ** potentially r/o VARCHAR.
    */
    data = p1->db_data;
    if(p1->db_datatype==DB_CHA_TYPE)
    {
      length_of_int = p1->db_length;
    }
    else /* DB_VCH_TYPE */
    {
      length_of_int = *(u_i2*)data;
      data += sizeof(u_i2);
    }

    /*
    ** Do the calculation:
    ** Note that we start from the right of the input string and work to the
    ** left.
    */
    for (i = length_of_int-1; i >= 0; i--)
    {
        char *c = (char *)(data + i);
        if (CMdigit(c))
        {
            n_digit = (i4)*c - (i4)'0';
            k = d[k][ F[7 & m++][n_digit] ];
        }
        else
        {
            return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
        }
    }

    /* Build the rdv...*/
    *(rdv->db_data + sizeof(i2)) = inv[k];
    return(E_DB_OK);
} /* generate_verhoeff_digit */

DB_STATUS
validate_verhoeff_digit(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *p1, 
    DB_DATA_VALUE *rdv
    )
{
    u_i2   length_of_int;
    char c_digit, calc_digit[2 + sizeof(i2)];
    DB_DATA_VALUE temp_dbv; /* Used for a temporary data item */
    DB_DATA_VALUE temp_rdv; /* Used for a temporary rdv */
    DB_STATUS     gen_return;

    /* Sundry Initialsation */
    rdv->db_prec = (i2)0;

    /* Get the check digit on this input string */
    length_of_int = *(u_i2 *)p1->db_data;
    c_digit = *(char *)(p1->db_data + sizeof(i2) + length_of_int - 1);

    /* Initialise the temp rdv */
    temp_rdv.db_data     = (PTR)&calc_digit;
    temp_rdv.db_datatype = DB_VCH_TYPE;
    temp_rdv.db_length   = (i4)(2 + sizeof(i2));
    temp_rdv.db_prec     = (i2)0;

    /* Initialise the temp_dbv. Note that it is a copy on the input data, 
    ** however it is DB_CHA_TYPE, with a shortened copy of the input data 
    ** string.
    */
    *(i2 *)p1->db_data = length_of_int - 1;
    temp_dbv.db_data     = p1->db_data + sizeof(i2);
    temp_dbv.db_datatype = DB_CHA_TYPE;
    temp_dbv.db_length   = (i4) length_of_int - 1;
    temp_dbv.db_prec     = (i2)0;

    /* Generate the check digit for the shortened string */
    gen_return = generate_verhoeff_digit(adf_scb, &temp_dbv, &temp_rdv);

    if (gen_return == E_DB_OK)
    {
        /* Check if the check digit matches what we just calculated */
        if (c_digit == *(char *)(calc_digit + sizeof(i2)))
            *(rdv->db_data) = (u_char)1;
        else
            *(rdv->db_data) = (u_char)0;
    }
    return(gen_return);
} /* validate_verhoeff_digit */

DB_STATUS
generate_verhoeffNR_digit(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *p1,
    DB_DATA_VALUE *rdv
    )
{
    u_i2   length_of_int;
    i4   c, i, j, k, m;
    char *data;

    static const i4 ip[10][8]={
        {0,1,5,8,9,4,2,7}, {1,5,8,9,4,2,7,0},
        {2,7,0,1,5,8,9,4}, {3,6,3,6,3,6,3,6},
        {4,2,7,0,1,5,8,9}, {5,8,9,4,2,7,0,1},
        {6,3,6,3,6,3,6,3}, {7,0,1,5,8,9,4,2},
        {8,9,4,2,7,0,1,5}, {9,4,2,7,0,1,5,8}
    };

    static const i4 ij[10][10]={
        {0,1,2,3,4,5,6,7,8,9}, {1,2,3,4,0,6,7,8,9,5},
        {2,3,4,0,1,7,8,9,5,6}, {3,4,0,1,2,8,9,5,6,7},
        {4,0,1,2,3,9,5,6,7,8}, {5,9,8,7,6,0,4,3,2,1},
        {6,5,9,8,7,1,0,4,3,2}, {7,6,5,9,8,2,1,0,4,3},
        {8,7,6,5,9,3,2,1,0,4}, {9,8,7,6,5,4,3,2,1,0}
     };

    /* Sundry Initialisation */
    j = 0; k = 0; m = 0;
    rdv->db_prec = (i2)0;

    *(i2 *)rdv->db_data = 1; /* this is a single character check digit */

    /* Get the input parameter length.
    ** And then read from 'data' instead of 'p1->db_data'.
    ** This routine will then be able to handle either CHAR or VARCHAR. When it
    ** is called internally later on - in validate_verhoeffNR_digit() - setup a 
    ** temporary dbv as DB_CHA_TYPE which points to the bit of the varchar you 
    ** want looked at. This avoids the need to adjust the VAR length of a
    ** potentially r/o VARCHAR.
    */
    data = p1->db_data;
    if(p1->db_datatype==DB_CHA_TYPE)
    {
      length_of_int = p1->db_length;
    }
    else /* DB_VCH_TYPE */
    {
      length_of_int = *(u_i2*)data;
      data += sizeof(u_i2);
    }

    /*
    ** And now do the calculation:
    ** Note that we start from the right of the input string and work to the
    ** left.
    */
    for (i = 0; i < length_of_int; i++)
    {
	char *c = (char *)(data + i);
        if (CMdigit(c))
	{
            k = ij[k][ ip[((i4)*c+2) % 10][7 & m++] ];
        }
        else
        {
            return(adu_error(adf_scb, E_AD2056_CHECK_DIGIT_STRING, 2, 1, c));
        }
    }

    for ( j = 0; j < 10; j++ ) 
    {
        if ( ! ij[k][ ip[j][ m&7 ] ] ) break;
    }

    /* And build the rdv... */
    *(rdv->db_data + sizeof(i2)) = (char )( j + (i4) '0' );
    return(E_DB_OK);
} /* generate_verhoeffNR_digit */

DB_STATUS
validate_verhoeffNR_digit(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *p1, 
    DB_DATA_VALUE *rdv
    )
{
    u_i2   length_of_int;
    char c_digit, calc_digit[2 + sizeof(i2)];
    DB_DATA_VALUE temp_dbv; /* Used for a temporary data item */
    DB_DATA_VALUE temp_rdv; /* Used for a temporary rdv */
    DB_STATUS     gen_return;

    /* Sundry Initialsation */
    rdv->db_prec = (i2)0;

    /* Get the check digit on this input string */
    length_of_int = *(u_i2 *)p1->db_data;
    c_digit = *(char *)(p1->db_data + sizeof(i2) + length_of_int - 1);

    /* Initialise the temp rdv */
    temp_rdv.db_data     = (PTR)&calc_digit;
    temp_rdv.db_datatype = DB_VCH_TYPE;
    temp_rdv.db_length   = (i4)(2 + sizeof(i2));
    temp_rdv.db_prec     = (i2)0;

    /* Initialise the temp_dbv. Note that it is a copy on the input data, 
    ** however it is DB_CHA_TYPE, with a shortened copy of the input data 
    ** string.
    */
    *(i2 *)p1->db_data = length_of_int - 1;
    temp_dbv.db_data     = p1->db_data + sizeof(i2);
    temp_dbv.db_datatype = DB_CHA_TYPE;
    temp_dbv.db_length   = (i4) length_of_int - 1;
    temp_dbv.db_prec     = (i2)0;

    /* Generate the check digit for the shortened string */
    gen_return = generate_verhoeffNR_digit(adf_scb, &temp_dbv, &temp_rdv);

    if (gen_return == E_DB_OK)
    {
        /* See if the check digit matches what we just calculated */
        if (c_digit == *(char *)(calc_digit + sizeof(i2)))
            *(rdv->db_data) = (u_char)1;
        else
            *(rdv->db_data) = (u_char)0;
        return(E_DB_OK); 
    }
    return(E_DB_ERROR);
} /* validate_verhoeffNR_digit */
/*{
** Name: adu_27lpad() - Prepends blanks into dv1 to length dv2.
**
** Description:
**	This routine is a stub that calls adu_27alpad() with the extra
**	DB_DATA_VALUE parameter used to prepend the blanks.
**	varchar.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be prepended to.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to search string.
**	dv2				Ptr to DB_DATA_VALUE containing integer
**					indicating result length.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string to be searched.
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
**	rdv				Ptr to result DB_DATA_VALUE.  
**	    .db_data			Ptr to string to contain the result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	One of the operands was not a string
**					type.
**
**  History:
**	17-apr-2007 (dougi)
**	    Written to support lpad().
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_27lpad(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *rdv)
{
    DB_STATUS	    status;

    status =  adu_27alpad(adf_scb, dv1, dv2, (DB_DATA_VALUE *)0, rdv);
					/* dv3 0 indicates blank */
    return(status);
}


DB_STATUS
adu_27alpad(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE	    *dv3,
DB_DATA_VALUE       *rdv)
{
    register char   *p1,
	            *p3;
    char	    *p1save,
		    *p3save,
		    *ptemp,
		    *wtemp;
    i4		    size1,
		    size3,
		    outlen, wlen,
		    maxsize,
		    pos = 1;
    i4		    found;
    DB_DT_ID	    bdt1;
    DB_DT_ID	    bdt2;
    DB_DT_ID	    bdt3;
    DB_STATUS	    db_stat;
    STATUS	    stat;
    u_char	    localbuf[4000];
    bool	    uselocal = TRUE;


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_27lpad: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(dv3);
        adu_prdv(rdv);
    }
#endif

    /* Acquire info about, and validate function parameters. */
    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;
    if (dv3 == (DB_DATA_VALUE *) 0)
	bdt3 = DB_CHA_TYPE;
    else bdt3 = dv3->db_datatype;
    if (   bdt1 != DB_CHR_TYPE  &&  bdt1 != DB_TXT_TYPE
	&& bdt1 != DB_CHA_TYPE  &&  bdt1 != DB_VCH_TYPE
	|| bdt2 != DB_INT_TYPE
	|| bdt3 != DB_CHR_TYPE  &&  bdt3 != DB_TXT_TYPE
	&& bdt3 != DB_CHA_TYPE  &&  bdt3 != DB_VCH_TYPE
       )
    {
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "pad bad types"));
    }

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &ptemp)) != E_DB_OK)
	return (db_stat);
    p1save = ptemp;

    if (dv3 == (DB_DATA_VALUE *) 0)
    {
	size3 = 1;
	ptemp = (char *)&" ";
    }
    else if ((db_stat = adu_lenaddr(adf_scb, dv3, &size3, &ptemp)) != E_DB_OK)
	return (db_stat);
    p3save = ptemp;

    maxsize = rdv->db_length;
    if (rdv->db_datatype == DB_VCH_TYPE ||
	rdv->db_datatype == DB_TXT_TYPE)
	maxsize -= DB_CNTSIZE;

    /* Load result length. */
    switch (dv2->db_length)
    {
      case 1:
	outlen = I1_CHECK_MACRO(*(i1 *) dv1->db_data);
	break;
      case 2:
	outlen = *((i2 *)dv2->db_data);
	break;
      case 4:
	outlen = *((i4 *)dv2->db_data);
	break;
      case 8:
	outlen = *((i8 *)dv2->db_data);
	break;
    }

    if (outlen <= 0 || outlen > maxsize)
    	return(adu_error(adf_scb, E_AD20A0_BAD_LEN_FOR_PAD, 0));
    wlen = (outlen <= size1)? outlen : size1;

    if (outlen > 4000)
    {
	ptemp = MEreqmem(0, outlen + DB_CNTSIZE, FALSE, &stat);
	uselocal = FALSE;
    }
    else ptemp = (char *)&localbuf[0];

    /* Start by copying dv1 to end of temp area. */
    if (size3 <= 0)
	outlen = wlen;
    if (wlen > 0)
	MEcopy(p1save, wlen, &ptemp[outlen-wlen]);
    if (wlen < outlen)
    {
	/* Result is bigger than dv1, so now do the padding on left. */
	wtemp = &ptemp[0];
	wlen = outlen - wlen;
	for (; wlen > 0; wlen -= size3)
	{
	    MEcopy(p3save, (wlen > size3) ? size3 : wlen, wtemp);
	    wtemp = &wtemp[size3];
	}
    }

    /* Job done - move result string to rdv and exit. */
    db_stat = adu_movestring(adf_scb, (u_char *)ptemp, outlen, dv1->db_datatype, rdv);

    if (!uselocal)
	MEfree(ptemp);

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_27lpad: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);

}


/*{
** Name: adu_28rpad() - Appends blanks onto dv1 to length dv2.
**
** Description:
**	This routine is a stub that calls adu_28arpad() with the extra
**	DB_DATA_VALUE parameter used to append the blanks.
**	varchar.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be appended to.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string.
**	dv2				Ptr to DB_DATA_VALUE containing integer
**					indicating result length.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string length value.
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
**	rdv				Ptr to result DB_DATA_VALUE.  
**	    .db_data			Ptr to string to contain the result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	One of the operands was not a string
**					type.
**
**  History:
**	17-apr-2007 (dougi)
**	    Written to support rpad().
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring routine, 
**	    in order to allow for binary data to be copied untainted. Since the 
**	    character input to movestring was created in this routine itself 
**	    we pass in DB_NODT to this routine.
*/

DB_STATUS
adu_28rpad(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *rdv)
{
    DB_STATUS	    status;

    status =  adu_28arpad(adf_scb, dv1, dv2, (DB_DATA_VALUE *)0, rdv);
					/* dv3 0 indicates blank */
    return(status);
}


DB_STATUS
adu_28arpad(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE	    *dv3,
DB_DATA_VALUE       *rdv)
{
    register char   *p1,
	            *p3;
    char	    *p1save,
		    *p3save,
		    *ptemp,
		    *wtemp;
    i4		    size1,
		    size3,
		    outlen, wlen,
		    maxsize,
		    pos = 1;
    i4		    found;
    DB_DT_ID	    bdt1;
    DB_DT_ID	    bdt2;
    DB_DT_ID	    bdt3;
    DB_STATUS	    db_stat;
    STATUS	    stat;
    u_char	    localbuf[4000];
    bool	    uselocal = TRUE;


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_28rpad: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(dv3);
        adu_prdv(rdv);
    }
#endif

    /* Acquire info about, and validate function parameters. */
    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;
    if (dv3 == (DB_DATA_VALUE *) 0)
	bdt3 = DB_CHA_TYPE;
    else bdt3 = dv3->db_datatype;
    if (   bdt1 != DB_CHR_TYPE  &&  bdt1 != DB_TXT_TYPE
	&& bdt1 != DB_CHA_TYPE  &&  bdt1 != DB_VCH_TYPE
	|| bdt2 != DB_INT_TYPE
	|| bdt3 != DB_CHR_TYPE  &&  bdt3 != DB_TXT_TYPE
	&& bdt3 != DB_CHA_TYPE  &&  bdt3 != DB_VCH_TYPE
       )
    {
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "pad bad types"));
    }

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &ptemp)) != E_DB_OK)
	return (db_stat);
    p1save = ptemp;

    if (dv3 == (DB_DATA_VALUE *) 0)
    {
	size3 = 1;
	ptemp = (char *)&" ";
    }
    else if ((db_stat = adu_lenaddr(adf_scb, dv3, &size3, &ptemp)) != E_DB_OK)
	return (db_stat);
    p3save = ptemp;

    maxsize = rdv->db_length;
    if (rdv->db_datatype == DB_VCH_TYPE ||
	rdv->db_datatype == DB_TXT_TYPE)
	maxsize -= DB_CNTSIZE;

    /* Load result length. */
    switch (dv2->db_length)
    {
      case 1:
	outlen = I1_CHECK_MACRO(*(i1 *) dv1->db_data);
	break;
      case 2:
	outlen = *((i2 *)dv2->db_data);
	break;
      case 4:
	outlen = *((i4 *)dv2->db_data);
	break;
      case 8:
	outlen = *((i8 *)dv2->db_data);
	break;
    }

    if (outlen <= 0 || outlen > maxsize)
    	return(adu_error(adf_scb, E_AD20A0_BAD_LEN_FOR_PAD, 0));
    wlen = (outlen <= size1)? outlen : size1;

    if (outlen > 4000)
    {
	ptemp = MEreqmem(0, outlen + DB_CNTSIZE, FALSE, &stat);
	uselocal = FALSE;
    }
    else ptemp = (char *)&localbuf[0];

    /* Start by copying dv1 to beginning of temp area. */
    if (wlen > 0)
	MEcopy(p1save, wlen, ptemp);
    if (size3 <= 0)
	outlen = wlen;
    else if (wlen < outlen)
    {
	/* Result is bigger than dv1, so now do the padding on right. */
	wtemp = &ptemp[wlen];
	wlen = outlen - wlen;
	for (; wlen > 0; wlen -= size3)
	{
	    MEcopy(p3save, (wlen > size3) ? size3 : wlen, wtemp);
	    wtemp = &wtemp[size3];
	}
    }

    /* Job done - move result string to rdv and exit. */
    db_stat = adu_movestring(adf_scb, (u_char *)ptemp, outlen, DB_NODT, rdv);

    if (!uselocal)
	MEfree(ptemp);

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_28rpad: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);

}


/*{
** Name: adu_29replace*() - replaces instances of one string with those
**	of another.
**
** Description:
**	These routines search a string for instances of a second string,
**	then replaces them by a third string. The three routines have the
**	same algorithm but differ with their types, the standard being
**	varchar/char, adu_29replace_raw for Binary and adu_29replace_uni
**	for Unicode.
**	In the multi-byte character case, both the search and replace
**	strings are trusted to be whole characters and the main string
**	will not be split searched mid-multibyte character.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be searched.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string.
**	dv2				Ptr to DB_DATA_VALUE containing string 
**					to be searched for.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string.
**	dv3				Ptr to DB_DATA_VALUE containing 
**					replacement string.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string.
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
**	rdv				Ptr to result DB_DATA_VALUE.  
**	    .db_data			Ptr to string to contain the result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	One of the operands was not a string
**					type.
**
**  History:
**	17-apr-2007 (dougi)
**	    Written to support replace().
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring routine, 
**	    in order to allow for binary data to be copied untainted. Since the 
**	    character input to movestring was created in this routine itself 
**	    we pass in DB_NODT to this routine.
**	13-Mar-2010 (kiria01) b123422
**	    Reworked original algorithm to to write directly to output buffer
**	    without intermediate copies. String truncation checks are applied.
**	    Also replicated for adu_29replace_raw and adu_29replace_uni.
*/

DB_STATUS
adu_29replace(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *dv3,
DB_DATA_VALUE       *rdv)
{
    u_char	*p;
    u_char	*src;
    u_char	*sea;
    u_char	*repl;
    i4		src_len;
    i4		sea_len;
    i4		repl_len;
    DB_DT_ID	bdt1;
    DB_DT_ID	bdt2;
    DB_DT_ID	bdt3;
    DB_STATUS	db_stat;
    STATUS	stat;


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_29replace: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(dv3);
        adu_prdv(rdv);
    }
#endif

    /* Acquire info about, and validate function parameters. */
    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;
    bdt3 = dv3->db_datatype;
    if (bdt1 != DB_CHR_TYPE && bdt1 != DB_TXT_TYPE &&
	bdt1 != DB_CHA_TYPE && bdt1 != DB_VCH_TYPE ||
	bdt2 != DB_CHR_TYPE && bdt2 != DB_TXT_TYPE &&
	bdt2 != DB_CHA_TYPE && bdt2 != DB_VCH_TYPE ||
	bdt3 != DB_CHR_TYPE && bdt3 != DB_TXT_TYPE &&
	bdt3 != DB_CHA_TYPE && bdt3 != DB_VCH_TYPE)
    {
    	return(adu_error(adf_scb, E_AD20B0_REPLACE_NEEDS_STR, 0));
    }

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &src_len, (char**)&src)) != E_DB_OK)
	return (db_stat);

    if ((db_stat = adu_lenaddr(adf_scb, dv2, &sea_len, (char**)&sea)) != E_DB_OK)
	return (db_stat);

    if ((db_stat = adu_lenaddr(adf_scb, dv3, &repl_len, (char**)&repl)) != E_DB_OK)
	return (db_stat);

    if (sea_len <= 0)
    {
	/* Copy src direct as search pattern empty */
	db_stat = adu_movestring(adf_scb, src, src_len, DB_NODT, rdv);
    }
    else
    {
	/* Scan for search string. When found, transfer any skipped
	** source string and copy replacement. Do this until no match and
	** then finish off with emitting tail of search string.
	** we only need to search the first src_len - sea_len + 1 characters */
	u_char	*src_end = src + src_len;
	u_char	*scan_end = src_end - sea_len + 1;
	u_char	*dest = rdv->db_data;
	i4	dest_len = rdv->db_length;
	u_char	*dest_end;
	i4	dest_left;	
	i4	l;

	if (rdv->db_datatype < 0)
	    dest_len--;
	if (abs(rdv->db_datatype) == DB_TXT_TYPE ||
	    abs(rdv->db_datatype) == DB_VCH_TYPE)
	{
	    dest += DB_CNTSIZE;
	    dest_len -= DB_CNTSIZE;
	}

	p = src;
	dest_left = dest_len;
	dest_end = dest;
	while (p < scan_end)
	{
	    if (MEcmp(p, sea, sea_len))
		CMnext(p);
	    else
	    {
		if ((l = p - src) > dest_left)
		{
		    /* Check bit we're dropping */
		    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
		    {
			while (l > dest_left && src[l-1] == ' ')
			    l--;
			if (l > dest_left)
			    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
				    2, l,(PTR)src);
		    }
		    l = dest_left;
		}
		MEcopy(src, l, dest_end);
		dest_end += l; dest_left -= l;
		if (dest_left <= 0 && adf_scb->adf_strtrunc_opt == ADF_IGN_STRTRUNC)
		    break;
		if ((l = repl_len) > dest_left)
		{
		    /* Check bit we're dropping */
		    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
		    {
			while (l > dest_left && src[l-1] == ' ')
			    l--;
			if (l > dest_left)
			    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
				    2, l,(PTR)repl);
		    }
		    l = dest_left;
		}
		MEcopy(repl, l, dest_end);
		dest_end += l; dest_left -= l;
		p += sea_len;
		src = p;
		if (dest_left <= 0 && adf_scb->adf_strtrunc_opt == ADF_IGN_STRTRUNC)
		    break;
	    }
	}
	if ((l = src_end - src) > dest_left)
	{
	    /* Check bit we're dropping */
	    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
	    {
		while (l > dest_left && src[l-1] == ' ')
		    l--;
		if (l > dest_left)
		    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
			    2, l,(PTR)src);
	    }
	    l = dest_left;
	}
	if (l)
	{
	    MEcopy(src, l, dest_end);
	    dest_end += l; dest_left -= l;
	}
	if (dest != (u_char*)rdv->db_data)
	{
	    u_i2 s = dest_end-dest;
	    I2ASSIGN_MACRO(s, ((DB_TEXT_STRING *)rdv->db_data)->db_t_count);
	}
	else if (dest_left)
	{
	    MEfill(dest_left, ' ', dest_end);
	}
    }


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_29replace: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}


DB_STATUS
adu_29replace_raw(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *dv3,
DB_DATA_VALUE       *rdv)
{
    u_char	*p;
    u_char	*src;
    u_char	*sea;
    u_char	*repl;
    i4		src_len;
    i4		sea_len;
    i4		repl_len;
    DB_DT_ID	bdt1;
    DB_DT_ID	bdt2;
    DB_DT_ID	bdt3;
    DB_STATUS	db_stat;
    STATUS	stat;


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_29replace_raw: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(dv3);
        adu_prdv(rdv);
    }
#endif

    /* Acquire info about, and validate function parameters. */
    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;
    bdt3 = dv3->db_datatype;
    if (bdt1 != DB_BYTE_TYPE && bdt1 != DB_VBYTE_TYPE ||
	bdt2 != DB_BYTE_TYPE && bdt2 != DB_VBYTE_TYPE ||
	bdt3 != DB_BYTE_TYPE && bdt3 != DB_VBYTE_TYPE)
    {
    	return(adu_error(adf_scb, E_AD20B0_REPLACE_NEEDS_STR, 0));
    }

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &src_len, (char**)&src)) != E_DB_OK)
	return (db_stat);

    if ((db_stat = adu_lenaddr(adf_scb, dv2, &sea_len, (char**)&sea)) != E_DB_OK)
	return (db_stat);

    if ((db_stat = adu_lenaddr(adf_scb, dv3, &repl_len, (char**)&repl)) != E_DB_OK)
	return (db_stat);

    if (sea_len <= 0)
    {
	/* Copy src direct as search pattern empty */
	db_stat = adu_movestring(adf_scb, src, src_len, DB_NODT, rdv);
    }
    else
    {
	/* Scan for search string. When found, transfer any skipped
	** source string and copy replacement. Do this until no match and
	** then finish off with emitting tail of search string.
	** we only need to search the first src_len - sea_len + 1 characters */
	u_char	*src_end = src + src_len;
	u_char	*scan_end = src_end - sea_len + 1;
	u_char	*dest = rdv->db_data;
	i4	dest_len = rdv->db_length;
	u_char	*dest_end;
	i4	dest_left;	
	i4	l;

	if (rdv->db_datatype < 0)
	    dest_len--;
	if (abs(rdv->db_datatype) == DB_VBYTE_TYPE)
	{
	    dest += DB_CNTSIZE;
	    dest_len -= DB_CNTSIZE;
	}

	p = src;
	dest_left = dest_len;
	dest_end = dest;
	while (p < scan_end)
	{
	    if (MEcmp(p, sea, sea_len))
		p++;
	    else
	    {
		if ((l = p - src) > dest_left)
		{
		    /* Check bit we're dropping */
		    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
		    {
			while (l > dest_left && src[l-1] == EOS)
			    l--;
			if (l > dest_left)
			    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
				    2, l,(PTR)src);
		    }
		    l = dest_left;
		}
		MEcopy(src, l, dest_end);
		dest_end += l; dest_left -= l;
		if (dest_left <= 0 && adf_scb->adf_strtrunc_opt == ADF_IGN_STRTRUNC)
		    break;
		if ((l = repl_len) > dest_left)
		{
		    /* Check bit we're dropping */
		    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
		    {
			while (l > dest_left && src[l-1] == EOS)
			    l--;
			if (l > dest_left)
			    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
				    2, l,(PTR)repl);
		    }
		    l = dest_left;
		}
		MEcopy(repl, l, dest_end);
		dest_end += l; dest_left -= l;
		p += sea_len;
		src = p;
		if (dest_left <= 0 && adf_scb->adf_strtrunc_opt == ADF_IGN_STRTRUNC)
		    break;
	    }
	}
	if ((l = src_end - src) > dest_left)
	{
	    /* Check bit we're dropping */
	    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
	    {
		while (l > dest_left && src[l-1] == EOS)
		    l--;
		if (l > dest_left)
		    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
			    2, l,(PTR)src);
	    }
	    l = dest_left;
	}
	if (l)
	{
	    MEcopy(src, l, dest_end);
	    dest_end += l; dest_left -= l;
	}
	if (dest != (u_char*)rdv->db_data)
	{
	    u_i2 s = dest_end-dest;
	    I2ASSIGN_MACRO(s, ((DB_TEXT_STRING *)rdv->db_data)->db_t_count);
	}
	else if (dest_left)
	{
	    MEfill(dest_left, EOS, dest_end);
	}
    }


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_29replace_raw: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}


DB_STATUS
adu_29replace_uni(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *dv3,
DB_DATA_VALUE       *rdv)
{
    UCS2	*p;
    UCS2	*src;
    UCS2	*sea;
    UCS2	*repl;
    i4		src_len;
    i4		sea_len;
    i4		repl_len;
    DB_DT_ID	bdt1;
    DB_DT_ID	bdt2;
    DB_DT_ID	bdt3;
    DB_STATUS	db_stat;
    STATUS	stat;


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_000_STRFI_ENTRY,&dum1,&dum2))
    {
        TRdisplay("adu_29replace_uni: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(dv3);
        adu_prdv(rdv);
    }
#endif

    /* Acquire info about, and validate function parameters. */
    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;
    bdt3 = dv3->db_datatype;
    if (bdt1 != DB_NCHR_TYPE && bdt1 != DB_NVCHR_TYPE ||
	bdt2 != DB_NCHR_TYPE && bdt2 != DB_NVCHR_TYPE ||
	bdt3 != DB_NCHR_TYPE && bdt3 != DB_NVCHR_TYPE)
    {
    	return(adu_error(adf_scb, E_AD20B0_REPLACE_NEEDS_STR, 0));
    }

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &src_len, (char**)&src)) != E_DB_OK)
	return (db_stat);
    src_len /= sizeof(UCS2);
    if ((db_stat = adu_lenaddr(adf_scb, dv2, &sea_len, (char**)&sea)) != E_DB_OK)
	return (db_stat);
    sea_len /= sizeof(UCS2);
    if ((db_stat = adu_lenaddr(adf_scb, dv3, &repl_len, (char**)&repl)) != E_DB_OK)
	return (db_stat);
    repl_len /= sizeof(UCS2);
    if (sea_len <= 0)
    {
	/* Copy src direct as search pattern empty */
	db_stat = adu_movestring(adf_scb, (u_char*)src, src_len, DB_NODT, rdv);
    }
    else
    {
	/* Scan for search string. When found, transfer any skipped
	** source string and copy replacement. Do this until no match and
	** then finish off with emitting tail of search string.
	** we only need to search the first src_len - sea_len + 1 characters */
	UCS2	*src_end = src + src_len;
	UCS2	*scan_end = src_end - sea_len + 1;
	UCS2	*dest = (UCS2*)rdv->db_data;
	i4	dest_len = rdv->db_length; /* Initially in bytes */
	UCS2	*dest_end;
	i4	dest_left;	
	i4	l;

	if (rdv->db_datatype < 0)
	    dest_len--;
	if (abs(rdv->db_datatype) == DB_NVCHR_TYPE)
	{
	    dest += DB_CNTSIZE/sizeof(UCS2);
	    dest_len -= DB_CNTSIZE;
	}

	dest_len /= sizeof(UCS2); /* Now not in bytes */

	p = src;
	dest_left = dest_len;
	dest_end = dest;
	while (p < scan_end)
	{
	    if (MEcmp(p, sea, sea_len*sizeof(UCS2)))
		p++;
	    else
	    {
		if ((l = p - src) > dest_left)
		{
		    /* Check bit we're dropping */
		    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
		    {
			while (l > dest_left && src[l-1] == U_BLANK)
			    l--;
			if (l > dest_left)
			    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
				    2, l,(PTR)src);
		    }
		    l = dest_left;
		}
		MEcopy(src, l*sizeof(UCS2), dest_end);
		dest_end += l; dest_left -= l;
		if (dest_left <= 0 && adf_scb->adf_strtrunc_opt == ADF_IGN_STRTRUNC)
		    break;
		if ((l = repl_len) > dest_left)
		{
		    /* Check bit we're dropping */
		    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
		    {
			while (l > dest_left && src[l-1] == U_BLANK)
			    l--;
			if (l > dest_left)
			    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
				    2, l,(PTR)repl);
		    }
		    l = dest_left;
		}
		MEcopy(repl, l*sizeof(UCS2), dest_end);
		dest_end += l; dest_left -= l;
		p += sea_len;
		src = p;
		if (dest_left <= 0 && adf_scb->adf_strtrunc_opt == ADF_IGN_STRTRUNC)
		    break;
	    }
	}
	if ((l = src_end - src) > dest_left)
	{
	    /* Check bit we're dropping */
	    if (adf_scb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
	    {
		while (l > dest_left && src[l-1] == U_BLANK)
		    l--;
		if (l > dest_left)
		    return adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
			    2, l,(PTR)src);
	    }
	    l = dest_left;
	}
	if (l)
	{
	    MEcopy(src, l*sizeof(UCS2), dest_end);
	    dest_end += l; dest_left -= l;
	}
	if (dest != (UCS2*)rdv->db_data) /* Is NVARCHAR? */
	{
	    u_i2 s = dest_end-dest;
	    I2ASSIGN_MACRO(s, ((DB_TEXT_STRING *)rdv->db_data)->db_t_count);
	}
	else if (dest_left)
	{
	    while (dest_left--)
		*dest_end++ = U_BLANK;
	}
    }


#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_29replace_uni: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
    }
#endif

    return (db_stat);
}

/*{
** Name: ad0_utf8_casetranslate - static routine to perform the case translation
**				  for UTF8 character sets.
**
** Description:
**	This routine is called by the adu_9strlower and adu_15strupper to 
**	convert the UTF8 string to upper or lower case.
**	This routine will convert the UTF8 string to UTF16 and then 
**	call the necessary case conversion routine to perform the conversion.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      src_dv                          Ptr to DB_DATA_VALUE containing string
**					to be case translated.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to string.
**	flag				contains the flag to indicate direction
**				 	of case conversion eg 
**					ADU_UTF8_CASE_TOUPPER or
**					ADU_UTF8_CASE_TOLOWER
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
**	rdv				Ptr to result DB_DATA_VALUE.  
**	    .db_data			Ptr to string to contain the result.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	One of the operands was not a string
**					type.
** History
**	10-may-2007 (gupsh01)
**	    Added
**	14-may-2007 (gupsh01)
**	    Pass NFC flag to adu_nvchr_fromutf8.
**	01-jun-2007 (gupsh01)
**	    Fix correctly calculating the intermediate unicode case values.
**	19-jun-2007 (gupsh01)
**	    Further fixed the intermediate lengths as it was not sufficient
**	    for nvarchars.
**	31-aug-2007 (gupsh01)
**	    upper/lowercase on char column of size 1 fails due to insufficient
**	    length for intermediate nvarchar datatype.
**	05-jun-2008 (gupsh01)
**	    Allow for maximum possible varchar length. 
*/  
static	DB_STATUS
ad0_utf8_casetranslate(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *src_dv,
DB_DATA_VALUE     *rdv,
i4		  flag)
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DATA_VALUE	temp_dv1, temp_dv2;
    u_i2		tempbuf1[1000];
    u_i2		tempbuf2[3000];
    char		*temp_ptr1;
    char		*temp_ptr2;
    i4			templen1 = 0;
    i4			templen2 = 0;
    bool		memexpanded = FALSE;
    i4			srclen;
    u_i2		*src;
    i2			saved_uninorm_flag = adf_scb->adf_uninorm_flag;

    if ((src_dv->db_datatype != DB_CHR_TYPE) && 
       (src_dv->db_datatype != DB_TXT_TYPE) && 
       (src_dv->db_datatype != DB_CHA_TYPE) && 
       (src_dv->db_datatype != DB_VCH_TYPE))
     return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    /* Find out input length etc */
    if (db_stat = adu_lenaddr(adf_scb, src_dv, &srclen, (char **)&src))
	return (db_stat);

    templen1 = DB_CNTSIZE + srclen * 2;
    if (templen1 > (DB_MAXSTRING + 2))
      templen1 = DB_MAXSTRING + 2;

    templen2 = DB_CNTSIZE + templen1 * 3;
    if (templen2 > (DB_MAXSTRING + 2))
      templen2 = DB_MAXSTRING + 2;

    if (templen2 > (i4)sizeof(tempbuf2))
    {
      temp_ptr1 = MEreqmem(0, templen1, TRUE, &db_stat);
      if (db_stat)
        return (db_stat);

      temp_ptr2 = MEreqmem(0, templen2, TRUE, &db_stat);
      if (db_stat)
        return (db_stat);

      memexpanded = TRUE;
    }
    else 
    {
	temp_ptr1 = (char*)&tempbuf1[0];
	temp_ptr2 = (char*)&tempbuf2[0];
    }

    temp_dv1.db_data = temp_ptr1;
    temp_dv1.db_datatype = DB_NVCHR_TYPE;
    temp_dv1.db_length = templen1;
    temp_dv1.db_prec = 0;
    temp_dv1.db_collID = 0;

    temp_dv2.db_data = temp_ptr2;
    temp_dv2.db_datatype = DB_NVCHR_TYPE;
    temp_dv2.db_length = templen2;
    temp_dv2.db_prec = 0;
    temp_dv2.db_collID = 0;

    /* Convert UTF8 to UCS2 first */
    adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
    if ((db_stat = adu_nvchr_fromutf8 (adf_scb, src_dv, &temp_dv1)) != E_DB_OK)
    {
      adf_scb->adf_uninorm_flag = saved_uninorm_flag;
      if (memexpanded) 
      {
	MEfree (temp_ptr1);
	MEfree (temp_ptr2);
      }
      return(db_stat);
    }
    adf_scb->adf_uninorm_flag = saved_uninorm_flag;

    if (flag == ADU_UTF8_CASE_TOUPPER)
    {
      if ((db_stat = adu_nvchr_toupper(adf_scb, &temp_dv1, &temp_dv2)) != E_DB_OK)
      {
        if (memexpanded) 
	{
	  MEfree (temp_ptr1);
	  MEfree (temp_ptr2);
        }  
        return(db_stat);
      }
    }
    else if (flag == ADU_UTF8_CASE_TOLOWER)
    {
      if ((db_stat = adu_nvchr_tolower(adf_scb, &temp_dv1, &temp_dv2)) != E_DB_OK)
      {
        if (memexpanded)
	{
	  MEfree (temp_ptr1);
	  MEfree (temp_ptr2);
	}
        return(db_stat);
      }
    }
    else 
    {
      if (memexpanded) 
      {
	  MEfree (temp_ptr1);
	  MEfree (temp_ptr2);
      }
      return (E_DB_ERROR);
    }

    /* Convert UCS2 back to UTF8 */
    if ((db_stat = adu_nvchr_toutf8 (adf_scb, &temp_dv2, rdv)) != E_DB_OK)
    {
      if (memexpanded) 
      {
	  MEfree (temp_ptr1);
	  MEfree (temp_ptr2);
      }
      return(db_stat);
    }

    if (memexpanded) 
    {
	  MEfree (temp_ptr1);
	  MEfree (temp_ptr2);
    }
    return (E_DB_OK);
}

