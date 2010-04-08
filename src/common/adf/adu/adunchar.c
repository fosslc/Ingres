/*
** Copyright (c) 2001, 2008 Ingres Corporation
*/
# include       <compat.h>
# include       <gl.h>
# include       <me.h>
# include	<tr.h>
# include       <lo.h>
# include       <st.h>
# include       <cm.h>
# include       <bt.h>
# include       <er.h>
# include       <iicommon.h>
# include       <adf.h>
# include       <ulf.h>
# include       <adfint.h>
# include       <aduint.h>
# include	<adfops.h>
# include       <adulcol.h>
# include       <aduucol.h>
# include       <aduucoerce.h>
# include	<adp.h>

/* typedefs */

typedef       unsigned char   UTF8;
typedef       unsigned long   UCS4;

/* STATIC function declarations */

static DB_STATUS
adu_utf8_to_ucs2 (
                ADF_CB            *adf_scb,
                UTF8**            sourceStart,
                const UTF8*       sourceEnd,
                UCS2**            targetStart,
                const UCS2*       targetEnd,
                i2                *reslen);

static DB_STATUS
adu_ucs2_to_utf8 (
                ADF_CB            *adf_scb,
                UCS2**            sourceStart,
                const UCS2*       sourceEnd,
                UTF8**            targetStart,
                const UTF8*       targetEnd,
                i2                *reslen);

static bool check_utf8();

static DB_STATUS
adu_nvchr_chartouni(
		ADF_CB           *scb,
                DB_DATA_VALUE    *dv_1,
                DB_DATA_VALUE    *dv_2);

static DB_STATUS
ad0_nvchr_casemap(
		ADF_CB		 *adf_scb,
		DB_DATA_VALUE	 *dv_in,
		DB_DATA_VALUE	 *rdv,
		i4		 caseflag);
/**
** ADUNCHAR.C - This file contains functions for implementating operations on 
**	              nchar and nvarchar unicode type.
**
**  The routines included in this file are:
**	
** Name: adu_nvchrcomp  	- Compare two unicode string data values. 
** Name: adu_nvchbld_key 	- Build an ISAM, BTREE, or HASH key for 
**				  unicode strings. 
** Name: adu_nvchr_convert      - Convert to/from DB_NVCHR_STRINGs.
** Name: adu_nvchr_dbtoev       - Determine which external datatype to convert.
** Name: adu_nvchr_concat       - Concatenate two nvarchar or nchar values.
** Name: adu_nvchr_left  	- Gets the leftmost N characters in a unicode string
** Name: adu_nvchr_right  	- Gets the rightmost N characters in an nvarchar 
**				  type. 
** Name: adu_nvchr_trim  	- Trim blanks off of the end of a unicode string
** Name: adu_nvchr_coerce       - This will carry out coercion of type,nchar->nchar, 
**				  nvarchar->nvarchar, nchar-> nchar or 
**				  nvarchar->nvarchar. 
** Name: adu_nvchr_locate       - locates the first occurrance of a string
** Name: adu_nvchr_shift        - This will carry out the shift operations on unicode
** Name: adu_nvchr_charextract  - This will carry out coercion of type
** Name: adu_nvchr_substr1      - This implements function substring(nvarchar, int)
** Name: adu_nvchr_substr2      - Return the substring from a string.
** Name: adu_nvchr_pad          - Pad the input unicode string with blanks to
** Name: adu_nvchr_squeezewhite() - Squeezes whitespace from a unicode string.
** Name: adu_nvchr_toutf8() 	- Externally visible function call that will 
**				  return UTF8 equivalent of Unicode string.
** Name: adu_nvchr_fromutf8()	- Externally visible function call that will 
**				  return Unicode data from a UTF8 string.
** Name: adu_nvchr_strutf8conv()- converts UTF8 to nchar/nvarchar type. This 
**				  function should not be generally used. This 
**				  is useful to convert internal datatype UTF8
**				  meant for copy operations where we need to 
**				  convert the value of a column default which is
**				  varchar to UTF8.
** Name: adu_nvchr_tolower()	- Externally visible function call to convert
**				  nchar/nvarchar to lowercase.
** Name: adu_nvchr_toupper()	- Externally visible function call to convert
**				  nchar/nvarchar to uppercase.
** Name: adu_nvchr_utf8comp()	- Externally visible function call to convert
**				  c/text/char/varchar UTF8 parms to UCS2, then
**				  compare them using the UCA.
**
**  History:
**	09-march-2001 (gupsh01)
**	    Created.
**	15-march-2001 (gupsh01)
**	    Added new unicode functions for like, left, 
**	    right, length, trim, and coercion.
**	21-march-2001 (gupsh01)
**	    Added new functions for locate, shift, charextract, substring
**	    pad and squeeze.
**	24-apr-2001 (abbjo03)
**	    Remove adu_nvchr_like.
**	24-may-2001 (abbjo03)
**	    In adu_nvchrbld_key(), replace calls to adu_nvchr_compare() by
**	    adu_nvchr_coerce() since the former does nothing at this time.
**	02-oct-2001 (gupsh01)
**	    Reinstate the xDebug defines for TRdisplay calls. [ Bug 105936]
**	08-oct-2001 (gupsh01)
** 	    Remove function adu_nvchr_size. This functionality is now 
**	    moved to adu_12strsize.
**	11-oct-2001 (abbjo03)
**	    In adu_nvchr_coerce(), fix handling of NCHAR/NVARCHAR strings that
**	    are longer than their declared length.
**	09-jan-2002 (somsa01)
**	    In adu_nvchr_length(), do not use I2_ASSIGN_MACRO and
**	    I4_ASSIGN_MACRO to set the result.
**	16-aug-2002 (gupsh01)
**	    Fixed the inclusion of header file aduucol.h, missed out during
**	    previos cross integration of changes.
**      15-aug-2003 (stial01)
**          Check for II_CHARSET UTF8 in adg_startup (b110727)
**	23-Jan-2004 (gupsh01)
**	    Revised code for mapping based coercion to/from local characters.
**	12-Feb-2005 (gupsh01)
**	    Modified adu_nvchr_coerce to return an error if unsupported 
**	    datatypes are send to the routine.
**  01-Mar-2005 (fanra01)
**      Bug 113994
**      Coercion of DBCS strings composed of both one and two byte
**      code points wrongly assumed code points are always one byte,
**      which caused processing beyond the end of the string,
**      resulting in E_AD5015_INVALID_BYTESEQ errors for random memory values.
**  20-Apr-2005 (fanra01)
**      Bug 114365
**      adu_nvchr_unitochar exception during coercion of a UTF-16 string array
**      to a local character set string caused by a read beyond the end of
**      the input string array.
**	25-apr-05 (inkdo01)
**	    Replaced AD1014 errors by AD5081 - 1014 is internal error that 
**	    gets buried by SCF and doesn't indicate the problem.
**    20-Oct-2005 (hanje04)
**        Move declaration of adu_nvchr_chartouni() here as
**        it's a static function and GCC 4.0 doesn't like it being
**        a FUNC_EXTERN and a static.
**    19-dec-2006 (stial01)
**        Added adu_nvchr_2args() for nvarchar(all,i)
**        Modified adu_nvchr_coerce for LNVCHR input
**    12-jan-2007 (stial01)
**        left/right NVCHR should not trim trailing blanks. (b117482)
**      25-jan-2007 (stial01)
**          Added support for nvarchar(locators)
**	09-mar-2007 (gupsh01)
**          Added support for lower/upper case for nchar/nvarchar.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	27-Mar-2009 (kiria01) b121841
**	    Ensure key build code can cope with a NULL buffer for
**	    the key so that datatype specific checks can be made even
**	    in the absence of a buffer.
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
*/

/*{
** Name: adu_nvchrcomp() - Compare two unicode string data values for equality
**			using the "nchar/nvarchar" semantics 
**
** Description:
**      Compare unicode character datatype "nchar" and "nvarchar type. The 
**	strings are compared by comparing each unicode code point of the 
**	string. If two strings are of different lengths, but equal up to 
**	the length of the shorter string, then the shorter string padded 
**	with blanks. No pattern matching is allowed.
**
** 
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      vcdv1                           Ptr DB_DATA_VALUE describing first
**                                      "nvarchar" string to be compared.  Its
**                                      datatype is assumed to be "nvarchar".
**          .db_data                    Ptr to DB_NVCHR_STRING containing
**                                      this "nvarchar" string.
**              .db_t_count             Number of characters in this "nvarchar"
**                                      string.
**              .db_t_text              Ptr to the actual characters.
**      vcdv2                           Ptr DB_DATA_VALUE describing second
**                                      "nvarchar" string to be compared.  Its
**                                      datatype is assumed to be "nvarchar".
**          .db_data                    Ptr to DB_TEXT_STRING containing
**                                      this "nvarchar" string.
**              .db_t_count             Number of characters in this "nvarchar"
**                                      string.
**              .db_t_text              Ptr to the actual characters.
**
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rcmp                            Result of comparison, as follows:
**                                          <0  if  vcdv1 < vcdv2
**                                          >0  if  vcdv1 > vcdv2
**                                          =0  if  vcdv1 = vcdv2
**      Returns:
**           The following DB_STATUS codes may be returned:
**           E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**          If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Completed successfully.
**
**      Exceptions:
**          none
**
** 	Side Effects:
**          none
**
** History:
**     01-feb-2001  gupsh01
**	    Added.
**	26-apr-2001 (abbjo03)
**	    Dereference ucsp1/ucsp2 when assigning them to ucschar1/2.
**	9-nov-05 (inkdo01)
**	    The intro says trailing blanks aren't significant, but they
**	    are. Fix it to ignore 'em.
**	4-apr-06 (dougi)
**	    Fix ill-considered change above.
**	9-nov-2006 (dougi)
**	    Change to use aduucmp() (& Unicode collation weights) to perform
**	    comparison. This is required for non-standard collations and for
**	    <, <=, >= and > comparisons. Still does bytewise compares for 
**	    standard collation if result is equal.
**	7-jan-2007 (dougi)
**	    Slight adjustment to above change to compute string end's using
**	    number of Unicode chars, not simply bytes.
*/

DB_STATUS
adu_nvchrcomp(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *vcdv1,
DB_DATA_VALUE       *vcdv2,
i4                  *rcmp)
{
    DB_STATUS           db_stat;
    DB_NVCHR_STRING	*nvch1; 
    DB_NVCHR_STRING	*nvch2; 
    i4			index;
    i4			end_result;
    i2			ucschar1_count;
    i2			ucschar2_count;
    UCS2	 	*ucsp1; 
    UCS2		*ucsp2;
    UCS2	 	*enducsp1; 
    UCS2		*enducsp2;
    UCS2		ucschar1;
    UCS2		ucschar2;
    i4			size1, size2;
    DB_DATA_VALUE       local_dv1;
    DB_DATA_VALUE       local_dv2;

    /* Check whether the data type is valid */	
    if (!(( vcdv1->db_datatype == DB_NVCHR_TYPE && 
		vcdv2->db_datatype == DB_NVCHR_TYPE) || 
	    ( vcdv1->db_datatype == DB_NCHR_TYPE && 
		vcdv2->db_datatype == DB_NCHR_TYPE))) 
    {
	return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
    }

    /* If Collation ID is default, try the byte-by-byte compare to see
    ** if values are equal. This is much faster than the use of collation
    ** weights. If an alternate collation is being used or if the values
    ** are not bytewise equal, we drop to logic to call aduucmp(). */

    if (vcdv1->db_collID <= DB_UNICODE_COLL &&
	vcdv2->db_collID <= DB_UNICODE_COLL)
    {
	if( db_stat = adc_1lenchk_rti(adf_scb, 0, vcdv1, &local_dv1)) 
	    return (db_stat);

	if( db_stat = adc_1lenchk_rti(adf_scb, 0, vcdv2, &local_dv2))
	    return (db_stat);

        /*
        ** Now perform the comparison based on the rules described above.
	** FIXME: add a call to the adugcmp for collation sequence. Right 
	** now we just compare based on Unicode code point values.
        */

        end_result = 0;

        /*
        ** Result of search is negative when the first operand is smaller,
        ** positive when the first operand is larger, and zero when they are
        ** equal.
        */
	
	if (vcdv1->db_datatype == DB_NVCHR_TYPE)
	{  
	   /* NVARCHAR case */
	    nvch1 = (DB_NVCHR_STRING *) vcdv1->db_data;
	    nvch2 = (DB_NVCHR_STRING *) vcdv2->db_data;

	    I2ASSIGN_MACRO(nvch1->count, ucschar1_count);
	    I2ASSIGN_MACRO(nvch2->count, ucschar2_count);

	    for (index = 0;
		end_result == 0 && index < ucschar2_count; index++)
	    {
		I2ASSIGN_MACRO(nvch1->element_array[index], ucschar1);
		I2ASSIGN_MACRO(nvch2->element_array[index], ucschar2);
		if (ucschar1 == ucschar2)
		;       /* EMPTY */
		else if (ucschar1 < ucschar2)
		end_result = -1;
		else if (ucschar1 > ucschar2)
		end_result = 1;
	    }

	    /* Check for trailing blanks if strings aren't same length. */
	    if (end_result == 0 && ucschar1_count != ucschar2_count)
	    {
		if (ucschar1_count < ucschar2_count)
		{
		    /* Swap operands so nvch1 is longer. */
		    nvch1 = nvch2;
		    ucschar1_count = ucschar2_count;
		}
		for (; index < ucschar1_count && end_result == 0; index++)
		{
		    /* If the rest are blank, they're still equal. */
		    I2ASSIGN_MACRO(nvch1->element_array[index], ucschar1);
		    if (ucschar1 != U_BLANK)
		     if (nvch1 == (DB_NVCHR_STRING *)vcdv1->db_data)
			end_result = 1;		/* no swap, 1 is bigger */
		     else end_result = -1;	/* swapped, 2 is bigger */
		}
	    }
	}
	else
	{ 
	    /* 
	    ** NCHR case 
	    **
	    ** The length in this case is as 
	    ** calculated by the lenchk function. 
	    */

	    ucschar1_count = local_dv1.db_length;
	    ucschar2_count = local_dv2.db_length;

	    ucsp1 = (UCS2 *) vcdv1->db_data;
	    ucsp2 = (UCS2 *) vcdv2->db_data;

            for (index = 0;
                end_result == 0 && index < ucschar1_count
                 && index < ucschar2_count; index++)
	    {
		I2ASSIGN_MACRO(*ucsp1, ucschar1);
		I2ASSIGN_MACRO(*ucsp2, ucschar2);

		if (ucschar1 == ucschar2)
		;       /* EMPTY */
		else if (ucschar1 < ucschar2)
		end_result = -1;
		else if (ucschar1 > ucschar2)
		end_result = 1;
		
		ucsp1++;
		ucsp2++;
	    }
	}

        /*
        ** If the two strings are equal (so far),
        ** then their difference is determined by their length.
        */

        if (vcdv1->db_datatype != DB_NVCHR_TYPE && end_result == 0)
        {
	    end_result = ucschar1_count - ucschar2_count;
	} 

	/* If bytewise compare is equal, return. Otherwise drop to 
	** collation weight driven compare. */
	if ((*rcmp = end_result) == 0)
            return(E_DB_OK);
    }

    /* Set up parameters and let aduucmp() do the comparison. */
    if (db_stat = adu_lenaddr(adf_scb, vcdv1, &size1, (char **) &ucsp1))
        return (db_stat);
    enducsp1 = ucsp1 + size1/sizeof(UCS2);

    if (db_stat = adu_lenaddr(adf_scb, vcdv2, &size2, (char **) &ucsp2))
        return (db_stat);
    enducsp2 = ucsp2 + size2/sizeof(UCS2);

    return(aduucmp(adf_scb, ADUL_BLANKPAD,
                    (u_i2 *)ucsp1, (u_i2 *)enducsp1,
                    (u_i2 *)ucsp2, (u_i2 *)enducsp2, rcmp,
                    vcdv1->db_collID));
}

/*{
** Name: adu_nvchbld_key - Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	the DB_NVCHR_STRING data, the key is simply the value itself.
**
**	However, the type of operation to be used is also included amongst the
**	parameters.  The possible operators, and their results, are listed
**	below.
**
**	    ADI_EQ_OP ('=')
**	    ADI_NE_OP ('!=')
**	    ADI_LT_OP ('<')
**	    ADI_LE_OP ('<=')
**	    ADI_GT_OP ('>')
**	    ADI_GE_OP ('>=')
**	    ADI_LIKE_OP
**
**	In addition to returning the key pair, the type of key produced is
**	returned.  The choices are
**
**	    ADC_KNOMATCH    --> No values will match this key.
**				    No key is formed.
**	    ADC_KEXACTKEY   --> The key will match exactly one value.
**				    The key that is formed is placed as the low
**				    key.
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.  The caller
**				    should seek to the low-key, then scan until
**				    reaching a point greater than or equal to
**				    the high key.
**	    ADC_KHIGHKEY    --> High key found -- only high key formed.
**	    ADC_KLOWKEY	    --> Low key found -- only low key formed.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				    No key was formed.
**
**	    Given these values, the combinations possible are shown below.
**
**	    ADI_EQ_OP ('=')
**		    returns ADC_KEXACTKEY, low_key == value provided
**	    ADI_NE_OP ('!=')
**		    returns ADC_KALLMATCH, no key provided
**	    ADI_LT_OP ('<')
**	    ADI_LE_OP ('<=')
**		    These return ADC_KHIGHKEY, with the high_key == value provided.
**	    ADI_GT_OP ('>')
**	    ADI_GE_OP ('>=')
**		    These return ADC_KLOWKEY, with the low_key == value provided.
**	    ADI_LIKE_OP
**		    returns ADC_KRANGEKEY with low and high key since a pattern
**		    starting with exact keys can be restated as a range test
**
**
** Inputs:
**      scb                             scb
**      key_block                       Pointer to key block data structure...
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
**	    .adc_escape			*** USED ONLY FOR THE `LIKE' familiy
**					of operators, and then only if
**					.adc_pat_flags AD_PAT_HAS_ESCAPE bit set
**					The escape char to use, if there was an
**					ESCAPE clause with the LIKE or NOT LIKE
**					predicate.
**	    .adc_pat_flags		*** USED ONLY FOR THE `LIKE' family
**					of operators ***
**					If there was an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then supply
**					.adc_pat_flags with AD_PAT_HAS_ESCAPE set,
**					and set .adc_escape to the escape char.
**					If there is not an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then this bit
**					must cleared. It is a good idea to set the
**					whole of .adc_pat_flags to 0 for other
**					operators that are not of the LIKE family.
**
** Outputs:
**      *key_block                      Key block filled with following
**	    .adc_tykey                       Type key provided
**	    .adc_lokey                       if .adc_tykey is ADC_KEXACTKEY or
**					     ADC_KLOWKEY, this is key built.
**	    .adc_hikey			     if .adc_tykey is ADC_KEXACTKEY or
**					     ADC_KHIGHKEY, this is the key built.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**
** History:
**     01-feb-2001  gupsh01
**          Added.
**	24-may-2001 (abbjo03)
**	    Replace calls to adu_nvchr_compare() by adu_nvchr_coerce() since
**	    the former does nothing at this time. Clean up indentation.
**	31-jul-02 (inkdo01)
**	    Fix "like" code to support NCHAR comparand and tidy a few other bits.
**	24-feb-05 (inkdo01)
**	    Normalize key constants.
**	22-Feb-2007 (kiria01) b117698
**	    Make the LIKE operator handle the range patterns and escape chars
**	    key building.
**	16-Apr-2007 (kiria01) b118069
**	    The normalisation of the LIKE pattern silently truncated the pattern
**	    length to the length of the column. This also accounts for why the
**	    routine in this case could not return ADC_KNOMATCH.
**	09-May-2007 (gupsh01)
**	    Added nfc_flag to adu_unorm to force normalization to NFC
**	    for UTF8 character sets.
**	07-Sep-2007 (gupsh01)
**	    Make sure that memory is freed only when allocated.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move adc_isescape into new adc_pat_flags.
**	    Drop out with KALLMATCH if an alternation character is seen as
**	    the alternatives will overly complicate things at present.
**	27-Oct-2008 (kiria01) SIR120473
**	    Handoff like work to pattern code.
**	30-May-2009 (kiria01) b122128
**	    Add back in the code for case and space ignore
*/
DB_STATUS
adu_nvchbld_key(
ADF_CB		*scb,
ADC_KEY_BLK	*key_block)
{
    DB_STATUS           result = E_DB_OK;
    STATUS		memstat = E_DB_OK;
    DB_DATA_VALUE	normdv;
    bool		memexpanded = FALSE;

    if (key_block->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All other datatypes are handled by the coercions
	** below and there are currently none to handle
	** differently */
	return result;
    }

    /* Allocate workspace and set up dv for normalization. */
    if (key_block->adc_opkey != ADI_LIKE_OP)
    {
	STRUCT_ASSIGN_MACRO(key_block->adc_lokey, normdv);
	normdv.db_data = MEreqmem(0, key_block->adc_lokey.db_length, FALSE, &memstat);
	if (memstat != OK)
	    return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(memstat), (i4 *)&memstat));
	else
	    memexpanded = TRUE;
    }

    switch (key_block->adc_opkey)
    {
    case ADI_NE_OP:
	/*key_block->adc_tykey = ADC_KALLMATCH;*/
	break;

    case ADI_EQ_OP:
	/*key_block->adc_tykey = ADC_KEXACTKEY;*/
	if (key_block->adc_lokey.db_data || key_block->adc_hikey.db_data)
	{
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (key_block->adc_lokey.db_data && result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_lokey, &normdv);
	    if (key_block->adc_hikey.db_data && result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_hikey, &normdv);
	}
	break;

    case ADI_LT_OP:
    case ADI_LE_OP:
	/*key_block->adc_tykey = ADC_KHIGHKEY;*/
	if (key_block->adc_hikey.db_data)
	{
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_hikey, &normdv);
	}
	break;

    case ADI_GT_OP:
    case ADI_GE_OP:
	/*key_block->adc_tykey = ADC_KLOWKEY;*/
	if (key_block->adc_lokey.db_data)
	{
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_lokey, &normdv);
	}
	break;

    case ADI_LIKE_OP:
    default:	
	/* This is a "like" predicate, adc_kdv is the pattern and lo/hikey
	** are the range that the comparand values can take. After coerce call, 
	** pattern will be stored as NCHR/NVCHR in lokey/hikey. Loops leave 
	** leading actual chars in place, but replace first pattern character
	** and all following chars by 0's (for lokey) and ff's (for hikey) to
	** define range of possible values that satisfy the predicate.
	** Additionally, escaped characters are put in the buffer in literal
	** form. No attempt is made to glean the extra character of min/max
	** out of a range pattern. This would require an appreciable amount
	** of collation handling to do it without risking the loss of of a
	** tuple that should have been in the solution set.
	*/
	if (abs(key_block->adc_kdv.db_datatype) == DB_PAT_TYPE)
	    return adu_patcomp_kbld_uni(scb, key_block);

	if (key_block->adc_pat_flags & (AD_PAT_WO_CASE|AD_PAT_DIACRIT_OFF|AD_PAT_BIGNORE))
	{
	    /* Text modification is implied that would
	    ** considerably complicate this routine
	    ** which would be better done using the pattern
	    ** keybld code if it were accessible.
	    ** For the moment, return ALLMATCH */
	    key_block->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}
	/*key_block->adc_tykey = ADC_KRANGEKEY;*/
	
	if (key_block->adc_lokey.db_data || key_block->adc_hikey.db_data)
	{
	    DB_DATA_VALUE *dvlo = &key_block->adc_lokey;
	    DB_DATA_VALUE *dvhi = &key_block->adc_hikey;
	    DB_DATA_VALUE normdv2;

	    STRUCT_ASSIGN_MACRO(key_block->adc_kdv, normdv);
	    normdv.db_length = key_block->adc_kdv.db_length*2;
	    normdv.db_datatype = DB_NVCHR_TYPE;
	    STRUCT_ASSIGN_MACRO(normdv, normdv2);
	    normdv.db_data = MEreqmem(0, normdv.db_length + normdv2.db_length, FALSE, &memstat);
	    normdv2.db_data = normdv.db_data + normdv.db_length;
	    if (memstat != OK)
		return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
			(i4) sizeof(memstat), (i4 *)&memstat));
	    else
	        memexpanded = TRUE;

	    /* We coerce the pattern into a temporary buffer which will be normallised*/
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (result == E_DB_OK)
		result = adu_unorm(scb, &normdv2, &normdv);

	    /* Now we must scan pattern and split into high and low */
	    if (result == E_DB_OK)
	    {
		i4	pm_found = FALSE;
		i4	inrange = 0;
		i4	i = 0;
		i2	pat_c, out_c;
		UCS2	*pat;
		UCS2	*patend;
		UCS2	*ptrlo = NULL;
		UCS2	*ptrhi = NULL;
		bool	pfound = 0;
		bool	isescape = (key_block->adc_pat_flags & AD_PAT_HAS_ESCAPE)!=0;
		i4	eschar   = key_block->adc_escape;
		DB_DATA_VALUE *dv = dvlo ? dvlo : dvhi;


		if (dv->db_datatype == DB_NCHR_TYPE)
		{
		    out_c = dv->db_length/sizeof(UCS2);
		    ptrlo = (UCS2 *)(dvlo->db_data);
		    ptrhi = (UCS2 *)(dvhi->db_data);
		}
		else
		{
		    out_c = (dv->db_length - DB_CNTSIZE)/sizeof(UCS2);
		    /* Grab the buffers and default the varchar length fields */
		    if (dvlo->db_data)
		    {
			ptrlo = ((DB_NVCHR_STRING *)dvlo->db_data)->element_array;
			((DB_NVCHR_STRING *)dvlo->db_data)->count = out_c;
		    }
		    if (dvhi->db_data)
		    {
			ptrhi = ((DB_NVCHR_STRING *)dvhi->db_data)->element_array;
			((DB_NVCHR_STRING *)dvhi->db_data)->count = out_c;
		    }
		}

		/* We have pinned type to NVARCHAR */
		pat_c = ((DB_NVCHR_STRING *)normdv2.db_data)->count;
		pat = ((DB_NVCHR_STRING *)normdv2.db_data)->element_array;
		patend = pat + pat_c;

		while (i < out_c && pat < patend && !pm_found)
		{
		    i4 cc;
		    UCS2 ch = *pat++;
		    if (isescape && ch == eschar)
		    {
			switch(ch = *pat++)
			{
			case AD_1LIKE_ONE:
			case AD_2LIKE_ANY:
			case AD_5LIKE_RANGE:
			    break;
			case AD_3LIKE_LBRAC:
			case AD_4LIKE_RBRAC:
			    pm_found = TRUE;
			    break;
			default:
			    if (ch != eschar)
			    {
				if ( memexpanded )
				  MEfree(normdv.db_data);/* release normalization workarea */
				if (ch == AD_7LIKE_BAR)
				{
				    /* If alternation is found, the likelihood is that
				    ** the min-max range will be of dubious value short
				    ** of evaluating it for every sub-pattern. To do
				    ** so will considerably complicate this routine
				    ** which would be better done using the pattern
				    ** keybld code if it were accessible.
				    ** For the moment, return ALLMATCH */
				    key_block->adc_tykey = ADC_KALLMATCH;
				    return E_DB_OK;
				}
			        /* ERROR:  illegal escape sequence */
				return adu_error(scb, E_AD1018_BAD_ESC_SEQ,0);
			    }
			    break;
			}
		    }
		    else
		    {   /* not an escape character */
			switch (ch)
			{
			case AD_1LIKE_ONE:
			case AD_2LIKE_ANY:
			    pm_found = TRUE;
			    break;
			}
		    }
		    if (pm_found)
			break;

		    if (ptrlo)
			ptrlo[i] = ch;

		    if (ptrhi)
			ptrhi[i] = ch;

		    i++;
		}
		
		/* Update nvarchar char count if needed */

		if (i >= out_c)
		{
		    /*
		    ** We've run out of column width against the pattern
		    ** but one remaining case could match and that is a
		    ** trailing sequence of match-any which would match for
		    ** zero extra width - so we eat such trailing.
		    */
		    while (pat < patend && *pat == AD_2LIKE_ANY)
			pat++;
		    /* Any remaining pattern and no match possible */
		    if (pat < patend)
		    {
			key_block->adc_tykey = ADC_KNOMATCH;
		    }
		    else
		    {
			key_block->adc_tykey = ADC_KEXACTKEY;
		    }
		}
		else if (ptrlo && ptrhi)
		{
		    for (; i < out_c; i++)
		    {
			ptrlo[i] = 0;
			ptrhi[i] = AD_MAX_UNICODE;
		    }
		}
		else if (ptrlo)
		{
		    for (; i < out_c; i++)
		    {
			ptrlo[i] = 0;
		    }
		}
		else
		{
		    for (; i < out_c; i++)
		    {
			ptrhi[i] = AD_MAX_UNICODE;
		    }
		}
	    }
	}
 	break;
    }

    if ( memexpanded )
      MEfree(normdv.db_data);			/* release normalization workarea */

    return (result);
}

/*{
** Name: adu_nvch_utf8_bldkey - Build an ISAM, BTREE, or HASH key for UTF8 
**			       datatypes.
**
** Description:
**	This routine is based on adu_nvchbld_key routine, except that it also
**	accepts a semantics flag which requests the routine whether to make a 
**	key for C type or a text type on a utf8 installation. 
**
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	the DB_NVCHR_STRING data, the key is simply the value itself.
**
**	However, the type of operation to be used is also included amongst the
**	parameters.  The possible operators, and their results, are listed
**	below.
**
**	    ADI_EQ_OP ('=')
**	    ADI_NE_OP ('!=')
**	    ADI_LT_OP ('<')
**	    ADI_LE_OP ('<=')
**	    ADI_GT_OP ('>')
**	    ADI_GE_OP ('>=')
**	    ADI_LIKE_OP
**
**	In addition to returning the key pair, the type of key produced is
**	returned.  The choices are
**
**	    ADC_KNOMATCH    --> No values will match this key.
**				    No key is formed.
**	    ADC_KEXACTKEY   --> The key will match exactly one value.
**				    The key that is formed is placed as the low
**				    key.
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.  The caller
**				    should seek to the low-key, then scan until
**				    reaching a point greater than or equal to
**				    the high key.
**	    ADC_KHIGHKEY    --> High key found -- only high key formed.
**	    ADC_KLOWKEY	    --> Low key found -- only low key formed.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				    No key was formed.
**
**	    Given these values, the combinations possible are shown below.
**
**	    ADI_EQ_OP ('=')
**		    returns ADC_KEXACTKEY, low_key == value provided
**	    ADI_NE_OP ('!=')
**		    returns ADC_KALLMATCH, no key provided
**	    ADI_LT_OP ('<')
**	    ADI_LE_OP ('<=')
**		    These return ADC_KHIGHKEY, with the high_key == value provided.
**	    ADI_GT_OP ('>')
**	    ADI_GE_OP ('>=')
**		    These return ADC_KLOWKEY, with the low_key == value provided.
**	    ADI_LIKE_OP
**		    returns ADC_KRANGEKEY with low and high key since a pattern
**		    starting with exact keys can be restated as a range test
**
**
** Inputs:
**      scb                             scb
**      key_block                       Pointer to key block data structure...
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
**	    .adc_escape			*** USED ONLY FOR THE `LIKE' familiy
**					of operators, and then only if
**					.adc_pat_flags AD_PAT_HAS_ESCAPE bit set
**					The escape char to use, if there was an
**					ESCAPE clause with the LIKE or NOT LIKE
**					predicate.
**	    .adc_pat_flags		*** USED ONLY FOR THE `LIKE' family
**					of operators ***
**					If there was an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then supply
**					.adc_pat_flags with AD_PAT_HAS_ESCAPE set,
**					and set .adc_escape to the escape char.
**					If there is not an ESCAPE clause with the
**					LIKE or NOT LIKE predicate, then this bit
**					must cleared. It is a good idea to set the
**					whole of .adc_pat_flags to 0 for other
**					operators that are not of the LIKE family.
**
** Outputs:
**      *key_block                      Key block filled with following
**	    .adc_tykey                       Type key provided
**	    .adc_lokey                       if .adc_tykey is ADC_KEXACTKEY or
**					     ADC_KLOWKEY, this is key built.
**	    .adc_hikey			     if .adc_tykey is ADC_KEXACTKEY or
**					     ADC_KHIGHKEY, this is the key built.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**
** History:
**     31-mar-2008 (gupsh01)
**          Added.
*/
DB_STATUS
adu_nvchr_utf8_bldkey(
ADF_CB		*scb,
i4		semantics,
ADC_KEY_BLK	*key_block)
{
    DB_STATUS           result = E_DB_OK;
    STATUS		memstat = E_DB_OK;
    DB_DATA_VALUE	normdv;
    bool		memexpanded = FALSE;
    UCS2		lowestchar;
    UCS2		highestchar;
    UCS2		utf8high2char = 0x06D5;
    UCS2		utf8high1char = 0x007A;

    if (key_block->adc_kdv.db_data == NULL)
    {
	/* Parameter marker or function obscuring data.
	** Use pre-loaded vale in adc_tykey - without a key
	** we cannot improve estimate.
	** All other datatypes are handled by the coercions
	** below and there are currently none to handle
	** differently */
	return result;
    }

    switch (semantics)
    {
      case AD_C_SEMANTICS:
        lowestchar  = (UCS2) U_BLANK;
        highestchar = (UCS2) AD_MAX_UNICODE;
        break;

      case AD_T_SEMANTICS:
        lowestchar  = (UCS2) 0x0001;
        highestchar = (UCS2) AD_MAX_TEXT;
        break;

      case AD_VC_SEMANTICS:
        lowestchar  = (UCS2) U_NULL;
        highestchar = (UCS2) AD_MAX_UNICODE;
        break;

      default:
        /* should *NEVER* get here */
        return (adu_error(scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /* Allocate workspace and set up dv for normalization. */
    if (key_block->adc_opkey != ADI_LIKE_OP)
    {
	STRUCT_ASSIGN_MACRO(key_block->adc_lokey, normdv);
	normdv.db_data = MEreqmem(0, key_block->adc_lokey.db_length, FALSE, &memstat);
	if (memstat != OK)
	    return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(memstat), (i4 *)&memstat));
	else
	    memexpanded = TRUE;
    }

    switch (key_block->adc_opkey)
    {
    case ADI_NE_OP:
	/*key_block->adc_tykey = ADC_KALLMATCH;*/
	break;

    case ADI_EQ_OP:
	/*key_block->adc_tykey = ADC_KEXACTKEY;*/
	if (key_block->adc_lokey.db_data || key_block->adc_hikey.db_data)
	{
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (key_block->adc_lokey.db_data && result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_lokey, &normdv);
	    if (key_block->adc_hikey.db_data && result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_hikey, &normdv);
	}
	break;

    case ADI_LT_OP:
    case ADI_LE_OP:
	/*key_block->adc_tykey = ADC_KHIGHKEY;*/
	if (key_block->adc_hikey.db_data)
	{
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_hikey, &normdv);
	}
	break;

    case ADI_GT_OP:
    case ADI_GE_OP:
	/*key_block->adc_tykey = ADC_KLOWKEY;*/
	if (key_block->adc_lokey.db_data)
	{
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (result == E_DB_OK)
		result = adu_unorm(scb, &key_block->adc_lokey, &normdv);
	}
	break;

    case ADI_LIKE_OP:
    default:	
	/* This is a "like" predicate, adc_kdv is the pattern and lo/hikey
	** are the range that the comparand values can take. After coerce call, 
	** pattern will be stored as NCHR/NVCHR in lokey/hikey. Loops leave 
	** leading actual chars in place, but replace first pattern character
	** and all following chars by 0's (for lokey) and ff's (for hikey) to
	** define range of possible values that satisfy the predicate.
	** Additionally, escaped characters are put in the buffer in literal
	** form. No attempt is made to glean the extra character of min/max
	** out of a range pattern. This would require an appreciable amount
	** of collation handling to do it without risking the loss of of a
	** tuple that should have been in the solution set.
	*/
	/*key_block->adc_tykey = ADC_KRANGEKEY;*/
	
	if (key_block->adc_lokey.db_data || key_block->adc_hikey.db_data)
	{
	    DB_DATA_VALUE *dvlo = &key_block->adc_lokey;
	    DB_DATA_VALUE *dvhi = &key_block->adc_hikey;
	    DB_DATA_VALUE normdv2;

	    STRUCT_ASSIGN_MACRO(key_block->adc_kdv, normdv);
	    normdv.db_length = key_block->adc_kdv.db_length*2;
	    normdv.db_datatype = DB_NVCHR_TYPE;
	    STRUCT_ASSIGN_MACRO(normdv, normdv2);
	    normdv.db_data = MEreqmem(0, normdv.db_length + normdv2.db_length, FALSE, &memstat);
	    normdv2.db_data = normdv.db_data + normdv.db_length;
	    if (memstat != OK)
		return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
			(i4) sizeof(memstat), (i4 *)&memstat));
	    else
	        memexpanded = TRUE;

	    /* We coerce the pattern into a temporary buffer which will be normallised*/
	    result = adu_nvchr_coerce(scb, &key_block->adc_kdv, &normdv);
	    if (result == E_DB_OK)
		result = adu_unorm(scb, &normdv2, &normdv);

	    /* Now we must scan pattern and split into high and low */
	    if (result == E_DB_OK)
	    {
		i4	pm_found = FALSE;
		i4	inrange = 0;
		i4	i = 0;
		i2	pat_c, out_c;
		UCS2	*pat;
		UCS2	*patend;
		UCS2	*ptrlo = NULL;
		UCS2	*ptrhi = NULL;
		bool	pfound = 0;
		bool	isescape = (key_block->adc_pat_flags & AD_PAT_HAS_ESCAPE)!=0;
		i4	eschar   = key_block->adc_escape;
		DB_DATA_VALUE *dv = dvlo ? dvlo : dvhi;


		if (dv->db_datatype == DB_NCHR_TYPE)
		{
		    out_c = dv->db_length/sizeof(UCS2);
		    ptrlo = (UCS2 *)(dvlo->db_data);
		    ptrhi = (UCS2 *)(dvhi->db_data);
		}
		else
		{
		    out_c = (dv->db_length - DB_CNTSIZE)/sizeof(UCS2);
		    /* Grab the buffers and default the varchar length fields */
		    if (dvlo->db_data)
		    {
			ptrlo = ((DB_NVCHR_STRING *)dvlo->db_data)->element_array;
			((DB_NVCHR_STRING *)dvlo->db_data)->count = out_c;
		    }
		    if (dvhi->db_data)
		    {
			ptrhi = ((DB_NVCHR_STRING *)dvhi->db_data)->element_array;
			((DB_NVCHR_STRING *)dvhi->db_data)->count = out_c;
		    }
		}

		/* We have pinned type to NVARCHAR */
		pat_c = ((DB_NVCHR_STRING *)normdv2.db_data)->count;
		pat = ((DB_NVCHR_STRING *)normdv2.db_data)->element_array;
		patend = pat + pat_c;

		while (i < out_c && pat < patend && !pm_found)
		{
		    i4 cc;
		    UCS2 ch = *pat++;
		    if (isescape && ch == eschar)
		    {
			switch(ch = *pat++)
			{
			case AD_1LIKE_ONE:
			case AD_2LIKE_ANY:
			case AD_5LIKE_RANGE:
			    break;
			case AD_3LIKE_LBRAC:
			case AD_4LIKE_RBRAC:
			    pm_found = TRUE;
			    break;
			default:
			    if (ch != eschar)
			        /* ERROR:  illegal escape sequence */
				return adu_error(scb, E_AD1018_BAD_ESC_SEQ,0);
			    break;
			}
		    }
		    else
		    {   /* not an escape character */
			switch (ch)
			{
			case AD_1LIKE_ONE:
			case AD_2LIKE_ANY:
			    pm_found = TRUE;
			    break;
			}
		    }
		    if (pm_found)
			break;

		    if (ptrlo)
			ptrlo[i] = ch;

		    if (ptrhi)
			ptrhi[i] = ch;

		    i++;
		}
		
		/* Update nvarchar char count if needed */

		if (i >= out_c)
		{
		    /*
		    ** We've run out of column width against the pattern
		    ** but one remaining case could match and that is a
		    ** trailing sequence of match-any which would match for
		    ** zero extra width - so we eat such trailing.
		    */
		    while (pat < patend && *pat == AD_2LIKE_ANY)
			pat++;
		    /* Any remaining pattern and no match possible */
		    if (pat < patend)
		    {
			key_block->adc_tykey = ADC_KNOMATCH;
		    }
		    else
		    {
			key_block->adc_tykey = ADC_KEXACTKEY;
		    }
		}
		else if (ptrlo && ptrhi)
		{
		    i4 stop;
		    i4 rem = 0;

		   if (out_c >= i)
		   {
			stop  = i + (out_c - i) / 3;
			rem = (out_c - i) % 3;
		   }
		   else  
			stop  = out_c;

		    for (; i < out_c; i++)
		    {
			ptrlo[i] = lowestchar;

			if (i < stop)
			  ptrhi[i] = highestchar;
			else if (rem-- == 2)
			  ptrhi[i] = utf8high2char;
			else 
			  ptrhi[i] = utf8high1char;
		    }
		}
		else if (ptrlo)
		{
		    for (; i < out_c; i++)
		    {
			ptrlo[i] = lowestchar;
		    }
		}
		else
		{
		    i4 stop;
		    i4 rem = 0;

		   if (out_c >= i)
		   {
			stop  = i + (out_c - i) / 3;
			rem = (out_c - i) % 3;
		   }
		   else  
			stop  = out_c;

		    for (; i < stop; i++)
		    {
			if (i < stop)
			  ptrhi[i] = highestchar;
			else if (rem-- == 2)
			  ptrhi[i] = utf8high2char;
			else 
			  ptrhi[i] = utf8high1char;
		    }
		}
	    }
	}
 	break;
    }

    if ( memexpanded )
      MEfree(normdv.db_data);			/* release normalization workarea */

    return (result);
}

/*{
** Name: adu_nvchr_convert	- Convert to/from DB_NVCHR_STRINGs.
**
** Description:
**      This routine converts values to and/or from DB_NVCHR_STRINGs.
** 	right now this is just a stub.
**
** Inputs:
**      scb                              scb
**      dv_in                           Data value to be converted
**      dv_out				Location to place result
**
** Outputs:
**      *dv_out                         Result is filled in here
**	    .db_data                         Contains result
**	    .db_length			     Contains length of result
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
**     01-feb-2001  gupsh01
**          Added.
**     24-nov-2003 (gupsh01)
**	    This function now calls adu_nvchr_coerce. This was a 
**	    stub earlier.
*/
DB_STATUS
adu_nvchr_convert(ADF_CB	*scb,
	      DB_DATA_VALUE	*dv_in,
	      DB_DATA_VALUE	*dv_out)
{
	return (adu_nvchr_coerce (scb, dv_in, dv_out));
} 


/*{
** Name: adu_nvchr_dbtoev	- Determine which external datatype this will convert to
**
** Description:
**      This routine returns the external type to which lists will be converted.
**      The correct type is DB_VARCHAR, with a length determined by this function.
**
**	The coercion function instance implementing this coercion must be have a
**	result length calculation specified as DB_RES_KNOWN.
**
** Inputs:
**      scb				Scb pointer.
**      db_value                        PTR to DB_DATA_VALUE for database type
**	ev_value			ptr to DB_DATA_VALUE for export type.
**
** Outputs:
**      *ev_value                       Filled in as follows
**          .db_datatype                Type of export value
**	    .db_length			Length of export value
**	    .db_prec			Precision of export value
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
**     01-feb-2001  gupsh01
**          Added.
*/
DB_STATUS
adu_nvchr_dbtoev(ADF_CB		*scb,
	     DB_DATA_VALUE	*db_value,
	     DB_DATA_VALUE	*ev_value)
{

	if (db_value->db_datatype == DB_NVCHR_TYPE)
	{
		ev_value->db_datatype = DB_VCH_TYPE;
		ev_value->db_length = db_value->db_length;
		ev_value->db_prec = 0;
	}
	else
	{
	   return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));	
	}

	return(E_DB_OK);
}

/*{
** Name: adu_nvchr_concat	- Concatenate two lists
**
** Description:
**	This function concatenates two DB_NVCHR_STRINGs.  The result is a string which is
**	the concatenation of the two strings.
**
** Inputs:
**      scb                             scb
**      dv_1                            Datavalue representing list 1
**	dv_2				Datavalue representing list 2
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              Result is placed here.
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
**     01-feb-2001  gupsh01
**          Added.
**	26-mar-2001 gupsh01
**	    Modified to include the other data types.
**	13-dec-2001 devjo01
**	    Truncate if result buffer too small.  Use MEcopy to do copies.
**	14-apr-2003 (gupsh01)
**	    Modified the check in this function for the result datatype.
*/
DB_STATUS
adu_nvchr_concat( ADF_CB		*scb,
		  DB_DATA_VALUE		*dv_1,
		  DB_DATA_VALUE		*dv_2,
		  DB_DATA_VALUE		*dv_result)
{
	DB_STATUS	result = E_DB_ERROR;
	UCS2		*str_1;
	UCS2		*str_2;
	UCS2		*res_str, *res_end;
	DB_NVCHR_STRING *temp;
	i4		sizer;
	i2		len_1;
	i2		len_2;
	i2		len_res;

#ifdef xDEBUG
       TRdisplay("adu_nvchr_concat: entered with args:\n");
        adu_prdv(dv_1);
        adu_prdv(dv_2);
        adu_prdv(dv_result);
#endif

	if ((dv_1->db_datatype != DB_NVCHR_TYPE &&
	     dv_1->db_datatype != DB_NCHR_TYPE) ||
	    (dv_2->db_datatype != DB_NVCHR_TYPE &&
	     dv_2->db_datatype != DB_NCHR_TYPE) ||
	    (dv_result->db_datatype != DB_NVCHR_TYPE &&
	     dv_result->db_datatype != DB_NCHR_TYPE) ||
	    dv_result->db_data == NULL
	   )
	{
	    return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0)); 
	}

	if(dv_1->db_datatype == DB_NCHR_TYPE)
	{
	   str_1 = (UCS2 *)dv_1->db_data;	
	   len_1 = dv_1->db_length / sizeof(UCS2);
        }
	else if(dv_1->db_datatype == DB_NVCHR_TYPE)
	{
	   str_1 = ((DB_NVCHR_STRING *)dv_1->db_data)->element_array;
	   I2ASSIGN_MACRO(((DB_NVCHR_STRING *)dv_1->db_data)->count, len_1);
	}
	else 
	    return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));

	if(dv_2->db_datatype == DB_NCHR_TYPE)
        {
           str_2 = (UCS2 *)dv_2->db_data;    
	   len_2 = dv_2->db_length / sizeof(UCS2);
        }
        else if(dv_2->db_datatype == DB_NVCHR_TYPE)
        {
           str_2 = ((DB_NVCHR_STRING *)dv_2->db_data)->element_array;
           I2ASSIGN_MACRO(((DB_NVCHR_STRING *)dv_2->db_data)->count, len_2);
        }
        else 
            return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));

        sizer = dv_result->db_length;
        if(dv_result->db_datatype == DB_NCHR_TYPE)
        {
	   res_str = (UCS2 *)dv_result->db_data;
        }
        else if(dv_result->db_datatype == DB_NVCHR_TYPE)
        {
	   sizer -= DB_CNTSIZE;
           res_str = ((DB_NVCHR_STRING *)dv_result->db_data)->element_array;
        }
        else 
            return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));

	sizer = sizer / sizeof(UCS2);

	if ( len_1 > sizer )
	    len_1 = sizer;

	MEcopy( (PTR)str_1, len_1 * sizeof(UCS2), (PTR)res_str );

	if ( len_1 + len_2 > sizer )
	    len_2 = sizer - len_1;

	if (len_2 > 0)
	    MEcopy( (PTR)str_2, len_2 * sizeof(UCS2), (PTR)(res_str + len_1) );

	if(dv_result->db_datatype == DB_NVCHR_TYPE)
	{
	    /* Set length */
	    len_res = len_1 + len_2;
	    temp  = (DB_NVCHR_STRING *)dv_result->db_data;	
	    I2ASSIGN_MACRO(len_res, temp->count);
	}
	else 
	{ 
	    /* pad the rest of the length with blanks */
	    res_end = res_str + sizer;
	    res_str = res_str + len_1 + len_2;
	    while ( res_str < res_end )
		*res_str++ = U_BLANK;
	}

	return(E_DB_OK);
}

/*{
** Name: adu_nvchr_length - Find number of UCS2 character in a unicode string
**
** Description:
**
**	This routine extracts the # of unicode characters in a DB_NVCHR_STRING
**
** Inputs:
**      scb                             scb
**      dv_1                            Datavalue representing the list
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              integer result is placed here.
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
**	05-feb-2001 (gupsh01)
**          Created.
**	04-oct-2001 (gupsh01)
**	    Modified, Now length function will return the
**	    number of unicode characters in the input  
**	    string.
**	13-dec-2001 (devjo01)
**	    adu_size already returns correct value for NVCHR.
**	09-jan-2002 (somsa01)
**	    Do not use I2_ASSIGN_MACRO and I4_ASSIGN_MACRO to set the result.
**	04-may-2004 (gupsh01)
**	    Fix the result length which has changed to 4. 
**	28-feb-05 (inkdo01)
**	    Add long nvarchar to supported arguments.
**/
DB_STATUS
adu_nvchr_length(ADF_CB		*scb,
	   DB_DATA_VALUE	*dv1,
	   DB_DATA_VALUE	*dv_result)
{
	DB_STATUS	result = E_DB_ERROR;
	DB_DATA_VALUE	local_dv;
	i4 		tmp_size;

        if (((dv1->db_datatype == DB_NVCHR_TYPE) 
        || (dv1->db_datatype == DB_NCHR_TYPE)
        || (dv1->db_datatype == DB_LNVCHR_TYPE))
	&&  dv_result->db_datatype == DB_INT_TYPE
	&&  dv_result->db_data
	&&  adc_1lenchk_rti (scb, 0, dv1, &local_dv) != E_DB_ERROR
	   )
	{
	  if ((result = adu_size(scb, dv1, &tmp_size)) != E_DB_OK)
	     return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));

          if (dv_result->db_length == 2)
	      *(i2 *)dv_result->db_data = tmp_size;
          else
	      *(i4 *)dv_result->db_data = tmp_size;

	  result = E_DB_OK;

	}
	else
       	    return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));
	    /* invalid input error */
	return (result);
}

/*{
** Name: adu_nvchr_left  - Gets the leftmost N characters in a unicode string
**                        resolve sql "left" predicates
** Description:
**        Returns the leftmost N characters (as determined in op2) of the string
**      held in op1.  The size of the result is the same as op1. 
**
** Inputs:
**
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      op1                             DB_DATA_VALUE containing unicode string to
**                                      extract N leftmost characters from.
**          .db_datatype                Its datatype.
**          .db_length                  Its length.
**          .db_data                    Ptr to the data.
**      op2                             DB_DATA_VALUE containing length, N.
**          .db_datatype                Its datatype(assumed to be DB_INT_TYPE).
**          .db_length                  Its length.
**          .db_data                    Ptr to the i1, i2, or i4.
**      result                             DB_DATA_VALUE for result.
**          .db_datatype                Datatype of the result.
**          .db_length                  Length of the result.
**
** Outputs:
**      scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**          .ad_errcode	                ADF error code for the error.
**          .ad_errclass                Signifies the ADF error class.
**          .ad_usererr                 If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**          .ad_emsglen                 The length, in bytes, of the resulting
**                                      formatted error message.
**          .adf_errmsgp                Pointer to the formatted error message.
**       result
**          .db_data                    Ptr to area to hold the result string.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Routine completed successfully.
**          E_AD5001_BAD_STRING_TYPE    Either dv1's or rdv's datatype was
**                                      inappropriate for this operation.
**
**  History:
**      09-mar-2001 (gupsh01) 
**	    Created.
**      15-may-2001 (gupsh01)
**	    Fixed the endpoint length for the input data pointer for 
**	    DB_NCHR_TYPE type of input data.
**	13-dec-2001 (devjo01)
**	    Fixed copy calculations for left.  Used MEcopy.  If dest
**	    is a NVCHR, then trim trailing blanks to match behavior of
**	    LEFT for regular VARCHAR.
**	22-may-2007 (smeke01) b118372
**	    Negative values should be treated as zero. 
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_nvchr_left(
ADF_CB             *scb,
register DB_DATA_VALUE      *op1,
register DB_DATA_VALUE      *op2,
DB_DATA_VALUE	   *result)
{
    UCS2                *p1;
    UCS2                *psave;
    i4                  size1;
    i4                  sizer;
    i4                  nleft;
    i8                  i8temp;

    if (op1->db_datatype == DB_NCHR_TYPE)
    {
       size1 = op1->db_length / sizeof(UCS2);
       p1 = (UCS2 *) op1->db_data;
       psave = (UCS2 *) result->db_data;
    }
    else if (op1->db_datatype == DB_NVCHR_TYPE)
    {
         size1 = ((DB_NVCHR_STRING *) op1->db_data)->count;
         if (size1 < 0  ||  size1 > DB_MAX_NVARCHARLEN )
             return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));
         p1 = (UCS2 *) ((DB_NVCHR_STRING *) op1->db_data)->element_array;
         psave = (UCS2 *) ((DB_NVCHR_STRING *) result->db_data)->element_array;
    }
    else     
	return (adu_error(scb, E_AD5001_BAD_STRING_TYPE, 0));	

    if ((result->db_datatype != DB_NVCHR_TYPE) &&  
	   (result->db_datatype != DB_NCHR_TYPE))
	return (adu_error(scb, E_AD5001_BAD_STRING_TYPE, 0));

    sizer = result->db_length;
    if (result->db_datatype == DB_NVCHR_TYPE)
        sizer -= DB_CNTSIZE;
    sizer = sizer / sizeof(UCS2);

    /* get the value of N, the leftmost chars wanted */
    switch (op2->db_length)
    {
	case 1:
	    nleft = I1_CHECK_MACRO(*(i1 *) op2->db_data);
	    break;
	case 2: 
	    nleft =  *(i2 *) op2->db_data;
	    break;
	case 4:
	    nleft =  *(i4 *) op2->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)op2->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_right nleft overflow"));

	    nleft = (i4) i8temp;
	    break;
	default:
	    return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_right nleft length"));
    }

    /* Negative values should be treated as zero. (b118372) */
    if ( nleft < 0 )
	nleft = 0;

    /* Only copy minimum of request,source size and dest size */
    if ( nleft > sizer )
	nleft = sizer;
    if ( nleft > size1 )
	nleft = size1;

    MEcopy( (PTR)p1, nleft * sizeof(UCS2), (PTR)psave );

    /*
    ** Pad nchar result types and trim nvarchar result types. 
    */
    psave += nleft;
    if ( DB_NCHR_TYPE == result->db_datatype )
    {
        /* pad with blanks if necessary */
	nleft = sizer - nleft;
        while (nleft-- > 0)
            *psave++ = U_BLANK;
    }
    else
    {
        ((DB_NVCHR_STRING *) result->db_data)->count = nleft;
    }
    return E_DB_OK;
}
/*{ Name: adu_nvchr_right-  Resolve sql "right" predicates
** Description:
**        Returns the rightmost dv2 characters in value op1.
**      The size of the result is the same as dv1.  
**	Note that trailing blanks are NOT ignored.
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE source.
**          .db_datatype                Its datatype
**          .db_length                  Its length.
**          .db_data                    Ptr to string to be extracted from.
**      dv2                             Ptr to DB_DATA_VALUE containing
**                                      number of rigthmost chars to extract.
**          .db_length                  Integer size of dv2.
**          .db_data                    Ptr to integer number.
**      rdv                             Ptr to DB_DATA_VALUE destination.
**          .db_datatype                Its datatype.
**          .db_length                  Its length.
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv
**          .db_data                    Ptr to area to hold resulting rightmost
**                                      characters.
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Completed successfully.
**          E_AD50001_BAD_STRING_TYPE   Either dv1's or rdv's datatype was
**                                      inappropriate for this operation.
**
**  History:
**      09-mar-2001 (gupsh01)
**          Created.
**      15-may-2001 (gupsh01)
**          Fixed the endpoint length for the input data pointer for
**          DB_NCHR_TYPE type of input data.
**	13-dec-2001 (devjo01)
**	    Trim if result is NVCHR to match behavior of RIGHT for VARCHAR,
**	    use MEcopy, truncate if needed.
**	22-may-2007 (smeke01) b118372
**	    Negative values should be treated as zero. 
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_nvchr_right(
ADF_CB              *adf_scb,
register DB_DATA_VALUE *dv1,
register DB_DATA_VALUE *dv2,
DB_DATA_VALUE          *rdv)
{
    UCS2                *p,
                        *p1,
                        *endp,
                        *psave;
    i4                  size1,
                        sizer,
			ncopy,
                        nright;
    i8			i8temp;
    
    if (dv1->db_datatype == DB_NCHR_TYPE)
    {
       size1 = dv1->db_length / sizeof(UCS2);
       p1 = (UCS2 *) dv1->db_data;
       psave = (UCS2 *) rdv->db_data;
    }
    else if (dv1->db_datatype == DB_NVCHR_TYPE)
    {
         size1 = ((DB_NVCHR_STRING *) dv1->db_data)->count;
         if (size1 < 0  ||  size1 > DB_MAX_NVARCHARLEN )
             return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
          p1 = (UCS2 *) ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
         psave = (UCS2 *) ((DB_NVCHR_STRING *) rdv->db_data)->element_array;
    }
    else  
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    if ((rdv->db_datatype != DB_NVCHR_TYPE) &&
           (rdv->db_datatype != DB_NCHR_TYPE))
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    sizer = rdv->db_length;
    if (rdv->db_datatype == DB_NVCHR_TYPE)
        sizer -= DB_CNTSIZE;

    /* get the number of the rightmost chars to extract */
    switch (dv2->db_length)
    {
	case 1:
	    nright = I1_CHECK_MACRO(*(i1 *) dv2->db_data);
	    break;
	case 2: 
	    nright =  *(i2 *) dv2->db_data;
	    break;
	case 4:
	    nright =  *(i4 *) dv2->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv2->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_right nright overflow"));

	    nright = (i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_right nright length"));
    }

    /* Negative values should be treated as zero. (b118372) */
    if ( nright < 0 )
	nright = 0;

    if ( nright > size1 )
	nright = size1;

    ncopy = nright;
    if (ncopy > sizer)
	ncopy = sizer;

    /* do the copy */
    MEcopy( (PTR)(p1 + size1 - nright), ncopy * sizeof(UCS2), (PTR)psave );

    p = psave + ncopy;
    if (DB_NCHR_TYPE == rdv->db_datatype)
    {
        /* pad with blanks if necessary */
	endp = psave + sizer;
        while (p < endp)
            *p++ = U_BLANK;
    }
    else
    {
        ((DB_NVCHR_STRING *)rdv->db_data)->count = p - psave;
    }

    return (E_DB_OK);
}

/*{
** Name: adu_nvchr_trim  - Trim blanks off of the end of a unicode string
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the input
**                                      string from which trailing blanks
**                                      are to be stripped.
**          .db_length                  It's length.
**          .db_data                    Ptr to data.
**      rdv                             DB_DATA_VALUE for result.
**          .db_length                  It's length.
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv                             DB_DATA_VALUE describing the trimmed
**                                      result.
**          .db_data                    Ptr to data area holding trimmed string.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Completed successfully.
**          E_AD5001_BAD_STRING_TYPE    The input DB_DATA_VALUE was not a
**                                      string type.
**
**  History:
**      09-mar-2001 (gupsh01)
**          Created.
**      15-may-2001 (gupsh01)
**          Fixed the endpoint length for the input data pointer for
**          DB_NCHR_TYPE type of input data.
**	19-sep-01 (inkdo01)
**	    Trim result is always nvchr - also corrected test for end of 
**	    result string.
**	13-dec-2001 (devjo01)
**	    Use MEcopy, truncate if needed.  Zero rest of dest, JIC
**	    this is being used by hash key generator, which will look
**	    at data past nominal end of user data.
*/
DB_STATUS
adu_nvchr_trim(
ADF_CB             *scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *rdv)
{
    UCS2                *instart;
    i4                  len, sizer;
    UCS2		*psave;
    UCS2                *endsource;

#ifdef xDEBUG 
        TRdisplay("adu_nvchr_trim: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
#endif 

    if (dv1->db_datatype == DB_NCHR_TYPE)
    {
       len = dv1->db_length / sizeof(UCS2) ;
       instart = (UCS2 *) dv1->db_data;
    }
    else if (dv1->db_datatype == DB_NVCHR_TYPE)
    {
         len = ((DB_NVCHR_STRING *) dv1->db_data)->count;
         if (len < 0  ||  len > DB_MAX_NVARCHARLEN )
             return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));
          instart = (UCS2 *) ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
    }
    else 
        return (adu_error(scb, E_AD5001_BAD_STRING_TYPE, 0));

    if (rdv->db_datatype != DB_NVCHR_TYPE)
        return (adu_error(scb, E_AD5009_BAD_RESULT_STRING_TYPE, 0));
    psave = (UCS2 *) ((DB_NVCHR_STRING *) rdv->db_data)->element_array;
    sizer = (rdv->db_length - DB_CNTSIZE) / sizeof(UCS2);

    /* set len to length of string disregarding trailing blanks */
    endsource = instart + len;
    while ( --endsource >= instart && U_BLANK == *endsource ) /* NOP */;

    len = endsource + 1 - instart;
    if ( len > sizer )
	len = sizer;

    MEcopy( (PTR)instart, len * sizeof(UCS2), (PTR)psave );
    ((DB_NVCHR_STRING *)rdv->db_data)->count = len;
    if ( len < sizer )
	MEfill( (sizer - len) * sizeof(UCS2), '\0', (PTR)(psave + len) );
    
#ifdef xDEBUG 
     TRdisplay("adu_nvchr_trim: returning DB_DATA_VALUE:\n");
     adu_prdv(rdv);
#endif 

    return E_DB_OK;
}


/*
** Name: adu_nvchr_2args() - Convert a data value to nvchr string data value.
**
** Description:
**      Support the two argument version of NVARCHAR()
**      Simply call adu_nvchr_coerce() to do the real work.
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
**     19-dec-2006 (stial01)
**         Created (from adu_ascii_2arg)
*/
DB_STATUS
adu_nvchr_2args(
ADF_CB			*adf_scb,
DB_DATA_VALUE	        *dv1,
DB_DATA_VALUE	        *dv2,
DB_DATA_VALUE		*rdv)
{
    ADF_STRTRUNC_OPT  input_strtrunc=adf_scb->adf_strtrunc_opt;
    STATUS            status;

    /* Ignore error if result is smaller than input */
    adf_scb->adf_strtrunc_opt = ADF_IGN_STRTRUNC;
    status = adu_nvchr_coerce(adf_scb, dv1, rdv);
    adf_scb->adf_strtrunc_opt = input_strtrunc;
    return( status);
}

/*{
** Name: adu_nvchr_coerce      - This will carry out coercion of type 
**	                         nchar -> nchar 
**				 nvarchar -> nvarchar
**				 nchar -> nvarchar
**				 nvarchar -> nchar
**                               nchar -> char / varchar / c / text
**                               nvarchar -> char / varchar / c / text
**                               char -> nchar / nvarchar
**                               varchar -> nchar / nvarchar
**								 c -> nchar 
**								 c -> nvarchar
**								 text -> nchar
**								 text -> nvarchar
**
** Description:
**      This routine converts values from nvarchar to nvarchar or nchar to nchar
**
** Inputs:
**      scb                              scb
**      dv_in                           Data value to be converted
**      dv_out                          Location to place result
**
** Outputs:
**      *dv_out                         Result is filled in here
**          .db_data                         Contains result
**          .db_length                       Contains length of result
**
**      Returns:
**          DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**     16-mar-2001  gupsh01
**          Added.
**	29-may-2001 (abbjo03)
**	    If count is zero for NVCHR, do not return an error since zero-
**	    length NVARCHAR strings are allowed.
**	29-jun-2001 (gupsh01)
**	    Corrected the coercion between the nvarchar and nchar.
**	11-oct-2001 (abbjo03)
**	    Correct handling of strings that are longer than their declared
**	    length.
**      20-mar-2002 (gupsh01)
**          Added support for coercion from and to normal character datatypes
**          char and varchar.
**	01-aug-2002 (gupsh01)
**          Added support for coercion from and to normal character datatypes
**          char and varchar.
**	20-apr-04 (drivi01)
**		Added routines to handle c and text coercions to/from unicode.
**	18-may-2004 (gupsh01)
**	    Added support for handling coercion of decimal, float, integer, 
**	    date and money types to unicode by first converting them to 
**	    char/varchar type using adu_ascii() function. 
**	18-jan-2004 (gupsh01)
**	    Fixed adu_moveunistring() to contain the correct
**	    size for the return length.
**	12-Feb-2005 (gupsh01)
**	    Modified adu_nvchr_coerce to return an error if unsupported 
**	    datatypes are send to the routine.
**	12-Feb-2007 (kiria01) b119902
**	    Add missing ANSI date input forms and correct length
**	    calculation for temporary buffer.
**	17-Mar-2009 (kiria01) SIR121788
**	    Add missing LNLOC support and let adu_lvch_move
**	    couponify the locators.
*/
DB_STATUS
adu_nvchr_coerce(ADF_CB        *scb,
              DB_DATA_VALUE     *dv_in,
              DB_DATA_VALUE     *dv_out)
{
    DB_STATUS		stat = E_DB_OK;
    dv_out->db_prec = 0;	

    if (dv_in->db_datatype == DB_NVCHR_TYPE && 
	(dv_out->db_datatype == DB_NCHR_TYPE ||
	dv_out->db_datatype == DB_NVCHR_TYPE))
    {
	/* FIXME: collID seems to be dropped? */
	return adu_moveunistring(scb, (UCS2 *)(dv_in->db_data + DB_CNTSIZE),
	    (i4)((DB_NVCHR_STRING *)dv_in->db_data)->count, dv_out);
    }
    else if (dv_in->db_datatype == DB_NCHR_TYPE && 
	(dv_out->db_datatype == DB_NCHR_TYPE ||
	dv_out->db_datatype == DB_NVCHR_TYPE))
    {
	/* FIXME: collID seems to be dropped? */
	return adu_moveunistring(scb, (UCS2 *)(dv_in->db_data),
	    dv_in->db_length/ sizeof(UCS2), dv_out);
    }
    else if ((dv_in->db_datatype == DB_NCHR_TYPE ||
              dv_in->db_datatype == DB_NVCHR_TYPE) &&
             (dv_out->db_datatype == DB_CHR_TYPE ||
              dv_out->db_datatype == DB_CHA_TYPE ||
              dv_out->db_datatype == DB_TXT_TYPE ||
              dv_out->db_datatype == DB_VCH_TYPE ||
	      dv_out->db_datatype == DB_BYTE_TYPE ||
              dv_out->db_datatype == DB_VBYTE_TYPE ||
              dv_out->db_datatype == DB_LTXT_TYPE))
    {
        /* Coercing from unicode to character data types */
        return adu_nvchr_unitochar (scb, dv_in, dv_out);
    }
    else if (dv_out->db_datatype == DB_NCHR_TYPE || 
	     dv_out->db_datatype == DB_NVCHR_TYPE)
    {
	switch (dv_in->db_datatype)
	{
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	case DB_TXT_TYPE:
        case DB_VCH_TYPE:
        case DB_BYTE_TYPE:
        case DB_VBYTE_TYPE:
        case DB_LTXT_TYPE:
	    /* Coercing from character data types to unicode types */
	    return adu_nvchr_chartouni (scb, dv_in, dv_out);

	case DB_DEC_TYPE:
	case DB_FLT_TYPE:
	case DB_INT_TYPE:
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
	    {
		/* convert the data into char type and 
 		** then do the conversion to unicode 
		*/
		DB_DATA_VALUE       cdata;
		char                utemp[256];

		/* How big may it be? */
		cdata.db_length = dv_out->db_length; 
		/* Drop prefixed length if NVARCHAR */
		if (dv_out->db_datatype == DB_NVCHR_TYPE)
		cdata.db_length -= DB_CNTSIZE;
		/* Adjust for UCS2 - we can only get so many into the output */
 		cdata.db_length /= sizeof(UCS2);
		/* Adjust length back for varchar and set type */
		cdata.db_length += DB_CNTSIZE;
		cdata.db_datatype = DB_VCH_TYPE;
		cdata.db_data = (PTR) utemp;

		/* Convert the number to ASCII first ... */
		if (stat = adu_ascii( scb, dv_in, &cdata) != E_DB_OK)
		    return (adu_error(scb, E_AD9999_INTERNAL_ERROR, 0));
		/* ... then to Unicode  */
		return adu_nvchr_chartouni (scb, &cdata, dv_out);
	    }
	case DB_LVCH_TYPE:
	case DB_LBYTE_TYPE:
	case DB_LNVCHR_TYPE:
	case DB_LCLOC_TYPE:
	case DB_LBLOC_TYPE:
	case DB_LNLOC_TYPE:
	    return (adu_lvch_move(scb, dv_in, dv_out));

	default:
            return adu_error (scb, E_AD5081_UNICODE_FUNC_PARM, 0);
	}
    }
    else
    /*  illegal call to this routine */
        return adu_error (scb, E_AD5081_UNICODE_FUNC_PARM, 0);
}

/*{
** Name: adu_nvchr_locate - locates the first occurrance of a string 
**	                    within a string. 
**
** Description:
**	This routine implements the function locate(nvarchar,nvarchar). 
**	ie, locates the first occurrance of a unicode string within a string. 
**	If not found, it will return the size of dv1 + 1.  
**	This retains trailing blanks in the matching string.  
**	The empty string is never found.
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing search
**                                      string.
**          .db_datatype                Its type.
**          .db_length                  Its length.
**          .db_data                    Ptr to the string to be searched.
**      dv2                             Ptr to DB_DATA_VALUE containing string
**                                      pattern to be found.
**          .db_datatype                Its type.
**          .db_length                  Its length.
**          .db_data                    Ptr to string pattern.
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv                             Ptr to result DB_DATA_VALUE.  Note, it
**                                      is assumed that rdv's type will be
**                                      DB_INT_TYPE and its length will be 2.
**          .db_data                    Ptr to an i2 which holds the result.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Completed successfully.
**          E_AD5001_BAD_STRING_TYPE    One of the operands was not a string
**                                      type.
** Exceptions:
**    none
**
** Side Effects:
**    none
**
** History:
**    19-march-2001 (gupsh01)
**	Added.
**	15-may-2001 (gupsh01)
**          Fixed the endpoint length for the input data pointer for
**          DB_NCHR_TYPE type of input data.
**	26-mar-2002 (gupsh01)
**	    Fixed the length returned when no match is found.
**	03-nov-2004 (gupsh01)
**	    Fixed the locate function to return the output data
**	    correctly.
**      23-Mar-2010 (hanal04) Bug 122436
**          If dv2 is a DB_NVCHR_TYPE we fail to setup up the local
**          variables because we were checking for dv1 again.
*/
DB_STATUS
adu_nvchr_locate(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *dv2,
DB_DATA_VALUE      *rdv)
{

    register UCS2   *p1,
                    *p2;
    UCS2            *p1save,
                    *p2save,
                    *p1end,
                    *p2end,
                    *p1last;
    i4              size1,
                    size2,
                    pos = 1;
    i4              found;
    DB_DT_ID        bdt1;
    DB_DT_ID        bdt2;

#ifdef xDEBUG 
        TRdisplay("adu_nvchr_locate: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
#endif 

    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;
    if ( (bdt1 != DB_NCHR_TYPE  &&  bdt1 != DB_NVCHR_TYPE)
        || (bdt2 != DB_NCHR_TYPE  &&  bdt2 != DB_NVCHR_TYPE)
       )
    {
        return(adu_error(adf_scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    if (dv1->db_datatype == DB_NCHR_TYPE)
    {
       size1 = dv1->db_length / sizeof(UCS2);
       p1save = (UCS2 *)dv1->db_data;
    }
    else if (dv1->db_datatype == DB_NVCHR_TYPE)
    {
         size1 = ((DB_NVCHR_STRING *) dv1->db_data)->count;
         if (size1 < 0  ||  size1 > DB_MAX_NVARCHARLEN )
             return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
          p1save = (UCS2 *) ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
    }

    if (dv2->db_datatype == DB_NCHR_TYPE)
    {
       size2 = dv2->db_length / sizeof(UCS2);
       p2save = (UCS2 *)dv2->db_data;
    }
    else if (dv2->db_datatype == DB_NVCHR_TYPE)
    {
         size2 = ((DB_NVCHR_STRING *) dv2->db_data)->count;
         if (size1 < 0  ||  size2 > DB_MAX_NVARCHARLEN )
             return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
          p2save = (UCS2 *) ((DB_NVCHR_STRING *) dv2->db_data)->element_array;
    }

    found = FALSE;

    /* p1last is one past the last position we should start a search from */
    p1last = p1save + max(0, size1 - size2 + 1);
    p1end  = p1save + size1;
    p2end  = p2save + size2;

    if (size2 > 0)      /* The empty string will never be found */
    {
        while (p1save < p1last  &&  !found)
        {
            p1 = p1save;
            p2 = p2save;

            while (p2 < p2end )
            {
		i4 compare;	
		compare = (i4)(*p1) - (i4)(*p2); 

                if (compare)
	            break; 
	        else 
                {
                     p1++;
                     p2++;
                }
	     }

             if (p2 >= p2end)
             {
                  found = TRUE;
             }
             else
             {
                  p1save++;
                  pos++;
             }
	}
    }

    if (!found)     /* if not found, make pos = size plus 1 */
    {
      if (dv1->db_datatype == DB_NVCHR_TYPE)
          pos = ((dv1->db_length - DB_CNTSIZE)/ sizeof(UCS2)) + 1;
      else
	  pos = (dv1->db_length / sizeof(UCS2)) + 1;
    }

    if (rdv->db_length == 2)
      *(i2 *)rdv->db_data = pos;
    else 
      *(i4 *)rdv->db_data = pos;

#ifdef xDEBUG 
     TRdisplay("adu_nvchr_locate: returning DB_DATA_VALUE:\n");
     adu_prdv(rdv);
#endif 

    return (E_DB_OK);
}

/*{
** Name: adu_nvchr_shift  - This will carry out the shift operations on unicode
**	    		    strings.
**
** Description:
**          This routine shifts the unicode strings to the right, if length  
**	is positive, leaving leading blanks. Shifts to the left if negative, 
**	leaving trailing blanks. 
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**                                      to be shifted.
**      dv2                             Ptr to DB_DATA_VALUE containing the
**                                      number of chars to shift.
**      rdv                             Ptr to the result DB_DATA_VALUE.
**          .db_datatype                The datatype of the result.
**          .db_length                  The length of the result string.
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv
**          .db_data                    Ptr to the resulting shifted string.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Routine completed successfully.
**          E_AD5001_BAD_STRING_TYPE    Input or Result DB_DATA_VALUES were of
**                                      inappropriate type.
** Side Effects:
**          none
**
** History:
**     20-mar-2001  gupsh01
**          Added.
**      15-may-2001 (gupsh01)
**          Fixed the endpoint length for the input data pointer for
**          DB_NCHR_TYPE type of input data.
**	13-dec-2001 (devjo01)
**	    Fixed copy amt calculations, trim results if dest is NVCHR
**	    to match behavior of SHIFT for VARCHAR.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_nvchr_shift(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *dv2,
DB_DATA_VALUE      *rdv)
{
    register UCS2       *pin,
                        *pout;
    UCS2                *endpout,
                        *psave;
    i4                  size1,
                        sizer,
			ncopy,
                        nshift;
    i8			i8shift; 

#ifdef xDEBUG 
        TRdisplay("adu_nvchr_shift: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
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
        nshift = *(i4 *) dv2->db_data;      /* NOTE: value should be <= MAXI2 */
	break;
      case 8:	
	i8shift = *(i8 *) dv2->db_data;

	/* limit to i4 values */
	if ( i8shift > MAXI4 || i8shift < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_shift shift overflow"));

	nshift = (i4) i8shift;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_shift shift length"));
    }

    /* get the length and address of the string to be shifted */

    if (dv1->db_datatype == DB_NCHR_TYPE)
    {
	size1 = dv1->db_length / sizeof(UCS2);
	pin = (UCS2 *) dv1->db_data;
    }
    else if (dv1->db_datatype == DB_NVCHR_TYPE)
    {
	size1 = ((DB_NVCHR_STRING *) dv1->db_data)->count;
	if (size1 < 0  ||  size1 > DB_MAX_NVARCHARLEN )
	    return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
	pin = (UCS2 *) ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
    }
    else
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    sizer = rdv->db_length;
    if (rdv->db_datatype == DB_NVCHR_TYPE)
    {
	pout = (UCS2 *) ((DB_NVCHR_STRING *) rdv->db_data)->element_array;
        sizer -= DB_CNTSIZE;
    }
    else if (rdv->db_datatype == DB_NCHR_TYPE)
    {
	pout = (UCS2 *) rdv->db_data;
    }
    else
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    sizer = sizer / sizeof(UCS2);
    endpout = pout + sizer;

    psave = pout;  
    if (nshift < 0)
    {
        /* skip the start of input string since it is negative; */
	nshift = 0 - nshift;
	pin = pin + nshift;
	if (nshift > size1)
	    ncopy = 0;
	else
	    ncopy = size1 - nshift;
	if (ncopy > sizer)
	    ncopy = sizer;
    }
    else
    {
	ncopy = size1;
	if (ncopy + nshift > sizer)
	    ncopy = sizer - nshift;
	if (nshift > sizer)
	    nshift = sizer;

        /* put in leading blanks */
        while (nshift-- > 0)
            *pout++ = U_BLANK;
    }

    if (ncopy > 0)
    {
	/* Copy surviving input chars. */
	MEcopy( (PTR)pin, ncopy * sizeof(UCS2), (PTR)pout );
	pout += ncopy;
    }

    if (DB_NCHR_TYPE == rdv->db_datatype)
    {
        /* pad with blanks if necessary */
        while (pout < endpout)
            *pout++ = U_BLANK;
    }
    else
    {
	/* Trim trailing spaces for NVCHR */
	while ( --pout >= psave && *pout == U_BLANK );
        ((DB_TEXT_STRING *)rdv->db_data)->db_t_count = pout + 1 - psave;
    }

#ifdef xDEBUG 
        TRdisplay("adu_nvchr_shift: returning DB_DATA_VALUE:\n");
        adu_prdv(rdv);
#endif 
    return (E_DB_OK);
}

/*{
** Name: adu_nvchr_charextract     - This will carry out coercion of type
**
** Description:
**	    This routine returns the n'th byte in the input unicode string. 
**	If n is out of range, a blank is returned. Output length is 1 and 
**      type is unicode character.  
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv_str                          Ptr to string DB_DATA_VALUE.
**          .db_datatype                Its type.
**          .db_length                  Its length.
**          .db_data                    Ptr to string.
**      dv_int                          Ptr to integer DB_DATA_VALUE.
**          .db_datatype                Its type (assumed to be DB_INT_TYPE).
**          .db_length                  Its length.
**          .db_data                    Ptr to the integer.  (This integer is
**                                      refered to as, `n'.)
**      rdv                             Ptr to destination DB_DATA_VALUE.
**          .db_datatype                Its type (assumed to be DB_CHA_TYPE).
**          .db_length                  Its length (assumed to be 1).
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv
**          .db_data                    Ptr to string holding the byte.
**                                      If `n' is non-positive, or greater than
**                                      the length of the string, a blank will
**                                      be the result.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Completed successfully.
**          E_AD5001_BAD_STRING_TYPE    dv_str's datatype was not a string type.
**
** History:
**     20-mar-2001  gupsh01
**          Added.
**      15-may-2001 (gupsh01)
**          Fixed the endpoint length for the input data pointer for
**          DB_NCHR_TYPE type of input data.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_nvchr_charextract(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv_str,
DB_DATA_VALUE      *dv_int,
DB_DATA_VALUE      *rdv)
{
    UCS2        *s;
    i4          slen;
    i4          n;
    i8          i8n;

    if (dv_str->db_datatype == DB_NCHR_TYPE)
    {
       slen = dv_str->db_length / sizeof(UCS2);
       s = (UCS2 *) dv_str->db_data;
    }
    else if (dv_str->db_datatype == DB_NVCHR_TYPE)
    {
         slen = ((DB_NVCHR_STRING *) dv_str->db_data)->count;
         if (slen < 0  ||  slen > DB_MAX_NVARCHARLEN )
             return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
          s = (UCS2 *) ((DB_NVCHR_STRING *) dv_str->db_data)->element_array;
    }
    else
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));


    switch (dv_int->db_length)
    {	
      case 1:
	n = I1_CHECK_MACRO(*(i1 *) dv_int->db_data);
	break;
      case 2:
	n = *(i2 *) dv_int->db_data;
	break;
      case 4:	
        n = *(i4 *) dv_int->db_data;      
	break;
      case 8:	
	i8n = *(i8 *) dv_int->db_data;

	/* limit to i4 values */
	if ( i8n > MAXI4 || i8n < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_charextract len overflow"));
	
	n = (i4) i8n;

	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_charextract len length"));
    }

    if (n <= 0  ||  n > slen)
    {
        *(UCS2 *) rdv->db_data = U_BLANK;
    }
    else
        *(UCS2 *) rdv->db_data = s[n-1];

    return (E_DB_OK);
}

/*{
** Name: adu_nvchr_substr1      - This implements function substring(nvarchar, int) 
**	                          for unicode data type.
**
** Description:
**      This file contains an implementation of
**      SUBSTRING ( ucs2 FROM int ).  It is based on the
**      description for substring in ISO/IEC FDIS 9075-2:1999
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             DB_DATA_VALUE containing string to
**                                      do substring from.
**          .db_datatype                Its datatype (must be a string type).
**          .db_length                  Its length.
**          .db_data                    Ptr to the data.
**      dv2                             DB_DATA_VALUE containing FROM operand
**          .db_datatype                Its datatype (must be an integer type).
**          .db_length                  Its length.
**          .db_data                    Ptr to the data.
**      rdv                             DB_DATA_VALUE for result.
**          .db_datatype                Datatype of the result.
**          .db_length                  Length of the result.
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv
**          .db_data                    Ptr to area to hold the result string.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Routine completed successfully.
**          E_AD5001_BAD_STRING_TYPE    Either dv1's or rdv's datatype was
**                                      inappropriate for this operation.
**          E_AD2092_BAD_STARTR_FOR_SUBSTR Specified start is past char expr
**
** History:
**     20-mar-2001  gupsh01
**          Added.
**      15-may-2001 (gupsh01)
**          Fixed the endpoint length for the input data pointer for
**          DB_NCHR_TYPE type of input data.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
DB_STATUS
adu_nvchr_substr1(
ADF_CB             *adf_scb,
register DB_DATA_VALUE      *dv1,
register DB_DATA_VALUE      *dv2,
DB_DATA_VALUE      *rdv)
{
    register UCS2       *dest;
    UCS2                *src;
    UCS2		*psave;	
    i4                  srclen;
    u_i2                reslen;
    i4                  startpos;
    i4                  endpos;
    i8                  i8startpos;
    i4                  i;

#ifdef xDEBUG 
        TRdisplay("adu_nvchr_substr1: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
#endif 

    if ((rdv->db_datatype != DB_NVCHR_TYPE)
	&& (rdv->db_datatype != DB_NCHR_TYPE)) 
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    if (dv1->db_datatype == DB_NCHR_TYPE)
    {
       srclen = dv1->db_length / sizeof(UCS2);
       src = (UCS2 *) dv1->db_data;
       dest =  (UCS2 *) rdv->db_data; 
    }
    else if (dv1->db_datatype == DB_NVCHR_TYPE)
    {
         srclen = ((DB_NVCHR_STRING *) dv1->db_data)->count;
         if (srclen < 0  ||  srclen > DB_MAX_NVARCHARLEN )
             return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
          src = (UCS2 *) ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
          dest = (UCS2 *) ((DB_NVCHR_STRING *) rdv->db_data)->element_array;
    }
    else
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    /* get the value of starting position */
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
	i8startpos = *(i8 *) dv2->db_data;

	/* limit to i4 values */
	if ( i8startpos > MAXI4 || i8startpos < MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_substr1 start overflow"));

	startpos = (i4) i8startpos;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_substr1 start length"));
    }

    endpos = max( srclen + 1, startpos);
    if (endpos < startpos)
      return (adu_error(adf_scb, E_AD2092_BAD_START_FOR_SUBSTR, 0));

    /* return an empty string */
    if ((startpos > srclen) || (endpos < 1))
    {
	if (rdv->db_datatype == DB_NVCHR_TYPE)
            ((DB_NVCHR_STRING *) rdv->db_data)->count = 0;
	else 
	    rdv->db_length = 0;

        return (E_DB_OK);
    }
       
    startpos = max(startpos, 1);
    reslen = min(endpos, srclen + 1) - startpos;

    /* Position to the starting character */
    for (i = 1; i < startpos; i++)
	src++;

     psave = dest; 
    /* Copy the source string to the result */ 
    for (i = 0; i < reslen; i++)
	 *(psave)++ = *(src)++;

     if (rdv->db_datatype == DB_NVCHR_TYPE)
    ((DB_NVCHR_STRING *) rdv->db_data)->count = reslen;

#ifdef xDEBUG
     TRdisplay("adu_nvchr_substr1: returning DB_DATA_VALUE:\n");
     adu_prdv(rdv);
#endif

     return (E_DB_OK);
}

/*{
** Name: adu_nvchr_substr2 - Return the substring from a string. 
**
** Description:
**      This file contains an implementation of substring(nvarchar, int, int) ie, 
**      SUBSTRING ( char FROM int FOR int ).  It is based on the
**      description for substring in ISO/IEC FDIS 9075-2:1999
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             DB_DATA_VALUE containing string to
**                                      do substring from.
**          .db_datatype                Its datatype (must be a string type).
**          .db_length                  Its length.
**          .db_data                    Ptr to the data.
**      dv2                             DB_DATA_VALUE containing FROM operand
**          .db_datatype                Its datatype (must be an integer type).
**          .db_length                  Its length.
**          .db_data                    Ptr to the data.
**      dv3                             DB_DATA_VALUE containing FOR operand
**          .db_datatype                Its datatype (must be an integer type).
**          .db_length                  Its length.
**          .db_data                    Ptr to the data.
**      rdv                             DB_DATA_VALUE for result.
**          .db_datatype                Datatype of the result.
**          .db_length                  Length of the result.
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv
**          .db_data                    Ptr to area to hold the result string.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Routine completed successfully.
**          E_AD5001_BAD_STRING_TYPE    Either dv1's or rdv's datatype was
**                                      inappropriate for this operation.
**
** History:
**     20-mar-2001  gupsh01
**          Created.
**      15-may-2001 (gupsh01)
**          Fixed the endpoint length for the input data pointer for
**          DB_NCHR_TYPE type of input data.
**	27-jun-2001 (gupsh01)
**	    Added E_AD2093_BAD_LEN_FOR_SUBSTR. 
**	29-oct-2001 (gupsh01)
**	    Fixed incorrect results for nchar data types.
**	    dest is now configured based on rdv type.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**		
*/
DB_STATUS
adu_nvchr_substr2(
ADF_CB       	*adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *dv3,
DB_DATA_VALUE	*rdv)
{
    UCS2                *dest;
    UCS2                *src;
    i4                  srclen;
    u_i2                reslen;
    i4                  forlen;
    i4                  startpos;
    i4                  endpos;
    i4                  i;
    i8                  i8temp;

#ifdef xDEBUG 
        TRdisplay("adu_nvchr_substr2: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(dv2);
        adu_prdv(rdv);
#endif 


    if (rdv->db_datatype != DB_NVCHR_TYPE)
	dest =  (UCS2 *) rdv->db_data;
    else if (rdv->db_datatype != DB_NCHR_TYPE)
	dest = (UCS2 *) ((DB_NVCHR_STRING *) rdv->db_data)->element_array;
    else 
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    if (dv1->db_datatype == DB_NCHR_TYPE)
    {
       srclen = dv1->db_length / sizeof(UCS2);
       src = (UCS2 *) dv1->db_data;
    }
    else if (dv1->db_datatype == DB_NVCHR_TYPE)
    {
        srclen = ((DB_NVCHR_STRING *) dv1->db_data)->count;
        if (srclen < 0  ||  srclen > DB_MAX_NVARCHARLEN )
            return (adu_error(adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0));
        src = (UCS2 *) ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
    }
    else
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

	
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
	i8temp= *(i8 *) dv2->db_data;

	/* limit to i4 values */
	if ( i8temp > MAXI4 || i8temp< MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_substr2 from overflow"));

	startpos = (i4) i8temp;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_substr2 from length"));
    }

    /* get the value of FOR */
    switch (dv3->db_length)
    {	
      case 1:
	forlen = I1_CHECK_MACRO(*(i1 *) dv3->db_data);
	break;
      case 2:
	forlen = *(i2 *) dv3->db_data;
	break;
      case 4:	
        forlen = *(i4 *) dv3->db_data;      
	break;
      case 8:	
	i8temp= *(i8 *) dv3->db_data;

	/* limit to i4 values */
	if ( i8temp > MAXI4 || i8temp< MINI4LL )
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_substr2 for overflow"));

	forlen = (i4) i8temp;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "nvchr_substr2 for length"));
    }

    endpos = startpos + forlen;

    if (endpos < startpos)
	return (adu_error(adf_scb, E_AD2093_BAD_LEN_FOR_SUBSTR, 0));

    /* return an empty string */
    if ((startpos > srclen) || (endpos < 1))
    {
        if (rdv->db_datatype == DB_NVCHR_TYPE)
            ((DB_NVCHR_STRING *) rdv->db_data)->count = 0;
        else
            rdv->db_length = 0;

        return (E_DB_OK);
     }

    startpos = max(startpos, 1);
    reslen = min(endpos, srclen + 1) - startpos;

    /* Position to the starting character */
    for (i = 1; i < startpos; i++)
        src++;

    /* Copy the source string to the result */

    for (i = 0; i < reslen; i++)
	*(dest)++ =  *(src)++;

    if (rdv->db_datatype == DB_NVCHR_TYPE)
    ((DB_NVCHR_STRING *) rdv->db_data)->count = reslen;

#ifdef xDEBUG 
     TRdisplay("adu_nvchr_substr2: returning DB_DATA_VALUE:\n");
     adu_prdv(rdv);
#endif 
     return (E_DB_OK);
}

/*{
** Name: adu_nvchr_pad() - Pad the input unicode string with blanks to 
**	             the full length of the output string.
**
** Description:
**        This routine pads a input string with blanks to its full length in an
**      output string.  
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing string to be
**                                      padded.
**          .db_length                  Its length.
**          .db_data                    Ptr to string to be padded.
**      rdv                             DB_DATA_VALUE describing result string
**                                      to hold padded result.
**          .db_length                  Its length.
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rdv
**          .db_data                    string hoding padded result.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Successful Completion.
**          E_AD5001_BAD_STRING_TYPE    The input DB_DATA_VALUE was not a
**                                      string type.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      20-mar-2001 (gupsh01)
**          Created. 
**	17-may-2001 (gupsh01)
**	    corrected inlen calculation for the DB_NVCHR_TYPE.  
**	18-sept-01 (inkdo01)
**	    Fix to compute result length of nvchr in characters, not bytes -
**	    also changed so that result is always nvchr and filled excess 
**	    chars individually with U_BLANK (not with memove which fills with
**	    0x2020 instead of 0x0020).
*/

DB_STATUS
adu_nvchr_pad(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *rdv)
{
    UCS2                *inptr;
    UCS2                *outptr;
    i4                  inlen;
    i4                  outlen;
    i4			extralen;
    DB_DT_ID            in_dt;
    DB_DT_ID            out_dt;
    DB_STATUS           db_stat;

#ifdef xDEBUG
        TRdisplay("adu_nvchr_pad: enter with args:\n");
        adu_prdv(dv1);
        adu_prdv(rdv);
#endif

    switch (in_dt = dv1->db_datatype)
    {
      case DB_NCHR_TYPE:
        inlen = dv1->db_length;
        inptr = (UCS2 *) dv1->db_data;
        break;

      case DB_NVCHR_TYPE:
        inlen = ((DB_NVCHR_STRING *) dv1->db_data)->count * sizeof(UCS2);
        inptr = ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
        break;

      default:
        return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    if (rdv->db_datatype != DB_NVCHR_TYPE)
        return (adu_error(adf_scb, E_AD5009_BAD_RESULT_STRING_TYPE, 0));

    outlen = rdv->db_length - DB_CNTSIZE;
    outptr = ((DB_NVCHR_STRING *) rdv->db_data)->element_array;
    extralen = (outlen - inlen) / sizeof(UCS2);

    /* Move the significant part into the result DB_DATA_VALUE. */
    MEmove(inlen,
                (PTR) inptr,
               (char) 0,
               outlen,
                (PTR) outptr);

    ((DB_NVCHR_STRING *) rdv->db_data)->count = outlen / sizeof(UCS2);

    /* If outlen is greater than inlen, fill the rest with U_BLANKs. */
    if (extralen > 0)
    {
	outptr = outptr + (inlen / sizeof(UCS2));
        while (extralen--)
	    *(outptr++) = U_BLANK;
    }

    db_stat = E_DB_OK;

#ifdef xDEBUG
     TRdisplay("adu_nvchr_pad: returning DB_DATA_VALUE:\n");
     adu_prdv(rdv);
#endif 

    return (db_stat);
}

/*
** Name: adu_nvchr_squeezewhite() - Squeezes whitespace from a unicode string.
**
** Description:
**      Trims whitespace off of front and back of string, and changes all
**  other whitespace sequences to blanks.  Whitespace is defined as
**  spaces, tabs, newlines, form-feeds, carriage returns, and NULLs (for char
**  and varchar, since they can't appear in c or text).
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE describing string
**                                      to be squeezed.
**          .db_length                  Its length.
**          .db_data                    Ptr to data area containing string
**                                      to be squeezed.
**
** Outputs:
**      rdv                             Ptr to DB_DATA_VALUE describing result
**                                      string.
**          .db_data                    Ptr to string which will hold the
**                                      squeezed result.
**
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Successful completion.
**          E_AD5001_BAD_STRING_TYPE    The input DB_DATA_VALUE was not a
**                                      string type.
**
**  History:
**      20-mar-2001 (gupsh01)
**          Implemented as stub.
**	18-Oct-2006 (kiria01) b116463
**	    Added body of function.
*/
DB_STATUS
adu_nvchr_squeezewhite(
ADF_CB              *scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *rdv)
{
    UCS2                *inptr;
    UCS2                *inend;
    i4                  len;
    UCS2                *outstart;
    UCS2                *outptr;
    UCS2                *outend;
    UCS2                ch;
    i4                  inspc = -1;	/* -1 -> at beginning of string */
					/*  0 -> currently in non-space */
					/*  1 -> currently in space */
#ifdef xDEBUG 
    TRdisplay("adu_nvchr_squeezewhite: enter with args:\n");
    adu_prdv(dv1);
    adu_prdv(rdv);
#endif 

    if (dv1->db_datatype == DB_NCHR_TYPE)
    {
        len = dv1->db_length / sizeof(UCS2) ;
        inptr = (UCS2 *) dv1->db_data;
    }
    else if (dv1->db_datatype == DB_NVCHR_TYPE)
    {
        len = ((DB_NVCHR_STRING *) dv1->db_data)->count;
        if (len < 0  ||  len > DB_MAX_NVARCHARLEN )
            return (adu_error(scb, E_AD5081_UNICODE_FUNC_PARM, 0));
        inptr = (UCS2 *) ((DB_NVCHR_STRING *) dv1->db_data)->element_array;
    }
    else 
        return (adu_error(scb, E_AD5001_BAD_STRING_TYPE, 0));
    inend = inptr + len;
    
    if (rdv->db_datatype != DB_NVCHR_TYPE)
        return (adu_error(scb, E_AD5009_BAD_RESULT_STRING_TYPE, 0));
    outstart = (UCS2 *) ((DB_NVCHR_STRING *) rdv->db_data)->element_array;
    outend = (UCS2 *) ((DB_NVCHR_STRING *) rdv->db_data)->element_array +
		    (rdv->db_length - DB_CNTSIZE) / sizeof(UCS2);
    outptr = outstart;

    while (inptr < inend && outptr < outend) {
	switch (ch = *inptr++) {
	/* spaces, tabs, newlines, form-feeds, carriage returns, and NULLs */
	case U_BLANK:
	case U_TAB:
	case U_LINEFEED:
	case U_FORMFEED:
	case U_RETURN:
	case U_NULL:
	    /* If we were not in spaces we are now */
	    if (!inspc) inspc++;
	    continue;

	default:
	    switch(inspc) {
	    case 1:
		/* We owe 1 space char in the output buffer */
		*outptr++ = U_BLANK;

		/* FALLTHROUGH */
	    
	    case -1:
		/* At start don't output space - saves pre-trim */
		inspc = 0;

		/* FALLTHROUGH */

	    case 0:
		/* Output actual data */
		*outptr++ = ch;
	    }
	    break;
	}
    }

    /* Save unicode string length */
    ((DB_NVCHR_STRING *)rdv->db_data)->count = outptr - outstart;

    /* Pad rest of buffer with NULs */
    while (outptr < outend) *outptr++ = U_NULL; 

#ifdef xDEBUG 
     TRdisplay("adu_nvchr_squeezewhite: returning DB_DATA_VALUE:\n");
     adu_prdv(rdv);
#endif 

    return E_DB_OK;
}

/*{
** Name: adu_nvchr_toutf8 - converts unicode datatypes to UTF8 form.
**
** Description:
**	This routine will convert the dv_in value which is of unicode
**	form to UTF8 encoded form for ascii operations, eg copying out 
**	to an ascii file, etc. 
**  History:
**      25-nov-2003 (gupsh01)
**          Implemented.
**	28-sep-2004 (gupsh01)
**	    Fixed this routine to call adu_3straddr instead of 
**	    adu_lenaddr in order to get the pointer to the data area. 
**	    (bug 113139).
**	01-jun-2007 (gupsh01)
**	    Set the output length correctly for regular Char types.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/
DB_STATUS
adu_nvchr_toutf8(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv_in,
DB_DATA_VALUE     *rdv)
{
    UCS2 *instr = 0;
    char *outstr = 0;
    i4 inlen = 0;
    i4 outlen = 0;
    i2 retlen = 0;
    DB_STATUS db_stat;

    if (db_stat = adu_lenaddr(adf_scb, dv_in, &inlen, (char **)&instr))
        return (db_stat);

    if (db_stat = adu_3straddr(adf_scb, rdv, &outstr))
        return (db_stat);

    inlen /= sizeof (UCS2);

    if ((rdv->db_datatype == DB_VCH_TYPE) || 
	(rdv->db_datatype == DB_UTF8_TYPE) || 
	(rdv->db_datatype == DB_TXT_TYPE))
        outlen = rdv->db_length - DB_CNTSIZE;
    else
        outlen = rdv->db_length;

    if ((db_stat = (adu_ucs2_to_utf8 (adf_scb, &instr, instr+inlen,
                      (u_char **)&outstr,(const u_char*)(outstr+outlen),
                              &retlen))) != OK)
      return db_stat;

    db_stat = adu_movestring (adf_scb, (u_char *)outstr, retlen, dv_in->db_datatype, rdv);

    return (db_stat);
}

/*{
** Name: adu_nvchr_fromutf8- converts UTF8 data to unicode form.
**
** Description:
**      This routine will convert the dv_in value which is of UTF8 
**	encoded form to unicode type defined in the rdv. 
**  History:
**      25-nov-2003 (gupsh01)
**          Implemented.
**	18-jan-2004 (gupsh01)
**	    Fixed adu_moveunistring() to contain the correct
**	    size for the return length.
**	14-feb-2005 (gupsh01)
**	    Rollback previous fix, length is already in count
**	    for adu_moveunistring.
**	16-jun-2005 (gupsh01)
**	    This routine now normalizes the input data. 
**	25-jul-2005 (gupsh01)
**	    Remove memory allocation for temp, buffer with
**	    a constant buffer of fixed size.
**	8-Aug-2005 (schka24)
**	    Allocate temp buffer if it's big.  Similar to what we do
**	    with strings elsewhere -- avoid excessive stack usage.
**	09-May-2007 (gupsh01)
**	    Added nfc_flag to adu_unorm to force normalization to NFC
**	    for UTF8 character sets.
**	14-May-2007 (gupsh01)
**	    Added nfc_flag to adu_nvchr_fromutf8() call.
**	04-Feb-2009 (gupsh01)
**	    Use ADF_UNI_TMPLEN define for temporary buffer space for
**	    unicode coercion.
*/
DB_STATUS
adu_nvchr_fromutf8(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv_in,
DB_DATA_VALUE     *rdv) 
{
    char *instr;
    UCS2 *outstr;
    i4 inlen = 0;
    i4 outlen = 0;
    i2  retlen = 0;
    DB_STATUS db_stat;
    DB_DATA_VALUE	tempdb;
    char tempbuf[ADF_UNI_TMPLEN];
    char *bigbuf = NULL;
    STATUS stat;

    if (db_stat = adu_lenaddr(adf_scb, dv_in, &inlen, &instr))
        return (db_stat);

    if (db_stat = adu_7straddr(adf_scb, rdv, &outstr))
        return (db_stat);

    if ((rdv->db_datatype == DB_NVCHR_TYPE) ||
	(rdv->db_datatype == DB_UTF8_TYPE))
        outlen = (rdv->db_length - DB_CNTSIZE) / sizeof (UCS2);
    else if (rdv->db_datatype == DB_NCHR_TYPE)
        outlen = (rdv->db_length) / sizeof (UCS2);
    else
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    if ((db_stat = adu_utf8_to_ucs2 (adf_scb, (u_char **)&instr,
               (const u_char *)(instr+inlen), &outstr,
                outstr+outlen, &retlen)) != OK)
      return db_stat;

    /* Exact size should be OK but allow +1 just in case! */
    if (rdv->db_length < ADF_UNI_TMPLEN)
	tempdb.db_data = tempbuf;
    else
	tempdb.db_data = bigbuf = (char *) MEreqmem(0, rdv->db_length+1,
				FALSE, &stat);
    tempdb.db_length = rdv->db_length;
    tempdb.db_datatype= rdv->db_datatype;
    tempdb.db_prec = rdv->db_prec;
    tempdb.db_collID = rdv->db_collID;

    if ((db_stat = adu_moveunistring (adf_scb, outstr, retlen, &tempdb) 
		!= E_DB_OK) || 
        (db_stat = adu_unorm (adf_scb, rdv, &tempdb)
		!= E_DB_OK))
    {
	if (bigbuf != NULL)
	    MEfree((PTR) bigbuf);
	return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
    }
    if (bigbuf != NULL)
	MEfree((PTR) bigbuf);

    return (db_stat);
}


/*{
** Name: adu_nvchr_chartouni - converts character datatypes to unicode datatypes.
**
** Description:
**      This will carry out coercion from character datatypes char and
**      varchar to unicode datatypes nchar and nvarchar.
**
** Inputs:
**      scb                             scb
**      dv_in                           Data value to be converted
**      dv_out                          Location to place result
**
** Outputs:
**      *dv_out                         Result is filled in here
**          .db_data                    Contains result
**          .db_length                  Contains length of result
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**     20-mar-2002 (gupsh01)
**          Added.
**     25-apr-2002 (gupsh01)
**	    Added code to convert from codepage to unicode by reading 
**	    from the mapping file.
**	31-jul-02 (inkdo01)
**	    Change lenaddr call to 7straddr when we only want address (NVCHR
**	    length may be uninit'ed).
**	23-Jan-2004 (gupsh01)
**	    Enabled mapping file based char to unicode lookup, for character
**	    sets other than UTF8.
**	2-Feb-2004 (gupsh01)
**	    Fixed adu_nvchr_chartouni function to work correctly for some
**	    local characters.
**	4-Feb-2004 (gupsh01)
**	    More fixes for mapping file based coercion.
**	19-Feb-2004 (gupsh01)
**	    Fixed substitution handling for unmapped characters.
**	22-Oct-2004 (gupsh01)
**	    Correctly swap the bytes for the windows platform.
**	01-Mar-2005 (fanra01)
**	    The adu_addrlen function returns an octet length that is used
**	    later to terminate a loop that processes characters.
**	    This change adjusts the processed byte length according to the
**	    octet count of each processed character (DBCS).
*/
DB_STATUS
adu_nvchr_chartouni (
               ADF_CB            *adf_scb,
               DB_DATA_VALUE     *dv_in,
               DB_DATA_VALUE     *rdv)
{
    char	*instr;
    UCS2	*outstr;
    i4		index = 0;
    i4		pos = 0;
    i4		count = 0;
    i4		inlen = 0;
    i4		outlen = 0;
    DB_STATUS	db_stat;
    i2		retlen = 0;
    i2		resultlen = 0;
    ADU_MAP_INFO	*map = NULL;
    u_char	*scanner;
    u_i2	local_value;
    bool	NO_MAP = FALSE;
    u_i2	res = 0;
    u_char	byteseq[4]; 
    int		bytecnt = 0;
    u_i4	bval;
    i4		i;

    if (db_stat = adu_lenaddr(adf_scb, dv_in, &inlen, &instr))
        return (db_stat);

    if (db_stat = adu_7straddr(adf_scb, rdv, &outstr))
        return (db_stat);

    if (rdv->db_datatype == DB_NVCHR_TYPE)
        outlen = (rdv->db_length - DB_CNTSIZE) / sizeof (UCS2);
    else if (rdv->db_datatype == DB_NCHR_TYPE)
        outlen = (rdv->db_length) / sizeof (UCS2);
    else
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    /* For UTF8 character set we will compute the unicode value*/
    if (check_utf8())
    {
      if ((db_stat = adu_utf8_to_ucs2 (adf_scb, (u_char **)&instr,
                (const u_char *)(instr+inlen), &outstr,
                outstr+outlen, &retlen)) != OK)
        return db_stat;

      resultlen = retlen;
    }
    else 
    {
        map = (ADU_MAP_INFO *) (Adf_globs->Adu_maptbl);

        if ((map == NULL) || (map->charmapptr == NULL))
          return (adu_error(adf_scb, E_AD5017_NOMAPTBL_FOUND, 0));

	scanner = (u_char *) instr;
	for(; (inlen > 0) && (count < outlen) ; CMnext(scanner))
        {
	  bytecnt = CMbytecnt(scanner);
	  CMcpychar(scanner, byteseq);

	  bval = byteseq[0];
	  for (i=1 ;i < bytecnt; i++)
	    bval = bval | byteseq[i] << (i * 8);

	  /* Validate the byte value. 
	  ** FIXME: Do not validate if users request so. */

	  bval =  BT_SWAP_BYTES_MACRO(bval); 

	  db_stat = adu_map_check_validity (map->validity_tbl, &bval);
	  if (db_stat)
	    return (adu_error(adf_scb, E_AD5015_INVALID_BYTESEQ, 
					  2, bytecnt, byteseq));	

          res =  adu_map_get_chartouni ((map->charmapptr)->charmap, 
	 				 &bval);
	 /* Retrieve the unicode value */
	  if (res == 0xFFFD)
	  { 
	    /* Found unmapped character */
	    return (adu_error(adf_scb, E_AD5011_NOCHARMAP_FOUND, 
				  2, bytecnt, scanner));	
	  }
	  else  
	  {
	     outstr[count] = res;
	  }
        inlen-=bytecnt;
	  count++;
        }
        resultlen = count;
    }
    db_stat = adu_moveunistring (adf_scb, outstr, resultlen, rdv);

    return (db_stat);
}

/*{
** Name: adu_nvchr_unitochar - converts unicode datatypes to character datatypes.
**
** Description:
**      This will carry out coercion from unicode datatypes nchar and
**      nvarchar to character datatypes char and varchar.
**
** Inputs:
**      scb                             scb
**      dv_in                           Data value to be converted
**      dv_out                          Location to place result
**
** Outputs:
**      *dv_out                         Result is filled in here
**          .db_data                    Contains result
**          .db_length                  Contains length of result
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**     04-apr-2002  (gupsh01)
**          Added.
**	25-apr-2002 (gupsh01)
**	    Added code for coercion from unicode to local code page.
**     08-jun-2002 (gupsh01)
**	    Removed check for collation table exists for utf8 encoding.	
**	23-Jan-2004 (gupsh01)
**	    Enabled mapping file based char to unicode lookup, for character
**	    sets other than UTF8.
**	02-Feb-2004 (gupsh01)
**	    Fixed dealing with combining characters for regular codepages.
**	19-Feb-2004 (gupsh01)	
**	    Fixed substitution handling for unmapped characters.  
**	28-sep-2004 (gupsh01)
**	    Fixed this routine to call adu_3straddr instead of
**	    adu_lenaddr in order to get the pointer to the data area.
**	    (bug 113139).
**	06-oct-2004 (gupsh01)
**	    Refix bug 113139. Previously we relied on adu_lenaddr function,
**	    to set out the result lengths for non variable types. With calling
**	    adu_3straddr we need to handle it in this routine.
**	22-Oct-2004 (gupsh01)
**	    Fix correct byte swapping for windows like platform. Also
**	    allow searching for equivalent unicode codepoint if the
**	    unicode code page we map to does not have equivalent
**	    character in the local code page. 
**	21-Nov-2004 (gupsh01)
**	    Fix this routine to correctly write single byte values for 
**	    unix platforms.
**	02-Dec-2004 (gupsh01)
**	    Fix copying of a single byte value only for BYTE_SWAP 
**	    platforms.
**	14-Feb-2005 (gupsh01)
**	    If maptbl is not initialized then return error 
**	    E_AD5017_NOMAPTBL_FOUND.
**	20-Apr-2005 (fanra01)
**          Adjust codepoint scanning to prevent reading beyond the end of the
**          input array.
**	14-Dec-2006 (gupsh01)
**	    Fixed the set unicode_substitution case, as extra blanks are 
**	    being copied to output buffer.
**	02-Apr-2006 (kiria01) b118016
**	    Avoid infinite loop when processing substitution characters.
**	31-Jul-2007 (gupsh01)
**	    For NFC conversion remove the U_NULL check added while 
**	    fixing bug 114365 to see if the unicode string is null 
**	    terminated. Embedded nulls are allowed in the unicode 
**	    string, just like regular varchar strings.
**	28-Nov-2007 (kiria01) b119532
**	    With mapping errors for which no substitution character has been
**	    set, do not abort processing the data leaving the output incomplete.
**	    Instead, provide a visible substitution character, finish the buffer
**	    and then return the error.
**	23-Jun-2008 (gupsh01)
**	    Previous bug fix for bug 118971, accidently removed 
**	    substring initialization, causing error in recombination
**	    from NFD.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
**	26-Oct-2008 (kiria01) SIR121012
**	    Add check for .adf_const_const
**       9-Mar-2010 (zhayu01) Bug 123389
**          Prevent buffer overrun and SEGVs by breaking out of the loop
**          before copying a multibyte character into outstr when there is
**          not enough room left in the buffer.
*/
DB_STATUS
adu_nvchr_unitochar (ADF_CB     *adf_scb,
              DB_DATA_VALUE     *dv_in,
              DB_DATA_VALUE     *rdv)
{
    UCS2        *instr = 0;
    char        *outstr = 0;
    u_i2        result = 0;
    i4          index = 0;
    i4          pos = 0;
    i4          count = 0;              /* input codepoint position  */
    i4          inlen = 0;
    i4          outlen = 0;
    i4          subc = 0;
    DB_STATUS   db_stat;
    bool        NOTFOUND = TRUE;
    i2          retlen = 0;
    i2          resultlen = 0;          /* output character position */
    ADUUCETAB   *cetbl = (ADUUCETAB *) adf_scb->adf_ucollation;
    UCS2        basechar;
    UCS2        *comblist;
    STATUS      stat = OK;
    u_i2        uni_char;
    u_i2        uni_char_err;
    u_i4        subchar = 0x1A;
    u_i2        subchar1 = 0x1A;
    i4          i = 0;
    i4          normcstrlen = 0;
    bool        substitution_on = FALSE;
    bool        substitution_err = FALSE;
    u_i2        bytenum;


    if (db_stat = adu_lenaddr(adf_scb, dv_in, &inlen, (char **)&instr))
          return (db_stat);

    if (db_stat = adu_3straddr(adf_scb, rdv, &outstr))
          return (db_stat);

    inlen /= sizeof (UCS2);

    /* Output length is not known at this time.
    ** Calculate the length, based on space available in rdv.
    */
    switch (rdv->db_datatype)
    {
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
           outlen = rdv->db_length;
           break;

        case DB_TXT_TYPE:
        case DB_LTXT_TYPE:
        case DB_VCH_TYPE:
        case DB_VBYTE_TYPE:
           outlen = rdv->db_length - DB_CNTSIZE;
     break; 
    }

    if (check_utf8())
    {
        if ((db_stat = (adu_ucs2_to_utf8 (adf_scb, &instr, instr+inlen, 
            (u_char **)&outstr,(const u_char*)(outstr+outlen), 
                &retlen))) != OK)
          return db_stat;
        resultlen = retlen;
    }
    else 
    {
        /* If cetbl does not exist, combining characters cannot be looked up 
        ** front has to make a call to aducolinit to get coercion working.
        */
        if ( cetbl == NULL )
           return (adu_error(adf_scb, E_AD5012_UCETAB_NOT_EXISTS, 0));

        /* Handle substitution for unmapped characters */ 
        if (Adf_globs->Adu_maptbl != NULL)
        {
            subchar1 = (((ADU_MAP_INFO *)
                           (Adf_globs->Adu_maptbl))->header)->subchar1;

            if (subchar1 != 0 )
                subchar = subchar1;
            else
                subchar = (((ADU_MAP_INFO *)
                          (Adf_globs->Adu_maptbl))->header)->subchar;

            if (adf_scb->adf_unisub_status == AD_UNISUB_ON)
                substitution_on = TRUE;
            if (adf_scb->adf_unisub_char[0] != '\0')
                subchar = adf_scb->adf_unisub_char[0]; /* FIXME for double byte */
	    if (!substitution_on && !subchar)
		subchar = '?';
        }
        else
            return (adu_error(adf_scb, E_AD5017_NOMAPTBL_FOUND, 0));

        /* Allocate working buffer */
        comblist = (UCS2 *) MEreqmem (0, sizeof(UCS2) * inlen, TRUE, &stat);

        /*
        ** Loop termination condition updated to test input codepoint array
        ** and output character lengths do not exceed their maximums; together
        ** with a null codepoint check.
        */
        for (;
		    ((count < inlen) && (resultlen < outlen));
		    count+=1)
        {
            subc = 0;  /* Reset count of combining code points found */
            normcstrlen = 0;  /* Reset length of NFC version of string */

            /* if the end of string of the inlength found, we are done */
            /* removed null termination of output string */

            /* obtain the next codepoint */
            uni_char = instr[count];

            /*
            ** check to see if uni_char is a base codepoint of a character, if
            ** not set it to a 'harmless' character to indicate the lack of
            ** a base codepoint.
            */
            basechar = (cetbl[uni_char].comb_class == 0) ? uni_char : 0;

            /*
            ** If the current codepoint from the input string is a base
            ** codepoint then collect the list of combining codepoints,
            ** if any. Comb class !=0 means we have a combining character
            ** This action may also be required for character type Mc.
            */
            if (basechar)
            {
                /*
                ** Scan the input string for combining codepoints and copy
                ** them into a temporary buffer.  Do not start if the
                ** current codepoint is the last one in the input array or
                ** if the codepoint is a Unicode null or
                ** if the codepoint is not a combining one.
                ** Stop if a non-combining codepoint or
                ** if the end of the string or
                ** if a Unicode null is encountered.
                **
                ** NB. uni_char is assigned within the condition and assumes
                ** that each sub-condition is evaluated in order.
                */
                for (pos=count+1;
                    (pos < inlen) && 
		      (uni_char = instr[pos]) && /* assignment */
                      (cetbl[uni_char].comb_class != 0);
                    pos+=1, subc+=1 )
                {
                    comblist[subc] = uni_char;  /* copy combining codepoint */
                }
                /*
                ** Update loop counter to set the current posistion to the end
                ** of the combining sequence or unchanged if no codepoint of
                ** combining class is detected.
                ** The next iteration should set the counter to be the start
                ** of the next character or the end of the string.
                */
                count = pos-1;
            }
            /*
            ** Convert the substring to Normal form C if there is a base
            ** codepoint and combining characters
            */
            if  (basechar && subc > 0)
            {
                stat = adu_map_conv_normc (adf_scb, cetbl, 
                    basechar, 
                    comblist, subc, 
                    comblist, &normcstrlen);
        
                /* find the equivalent local character */
                for (i=0; i < normcstrlen; i++)
                {
                    result = adu_map_get_unitochar (
                      (((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->unimapptr)->unimap, 
                      &comblist[i]);

                    if ((result == 0) && (comblist[i] != 0))    
                    {
                        uni_char = comblist[i];
                        /* Try for other equivalent unicode characters if any */
			for(;;)
                        {
			    UCS2 save_bc = comblist[i];
			    comblist[i] = adu_get_recomb (adf_scb, save_bc, 0);
			    if (comblist[i] == FAIL)
				break;
                            result = adu_map_get_unitochar (
                                (((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->unimapptr)->unimap
                                , &comblist[i]);

                            if (result == 0 && comblist[i] != 0 && comblist[i] != save_bc)
                                continue;
                            else 
                                break;
                        }
                    }

		    if ((result == 0) && (comblist[i] != 0))
		    {
			adf_scb->adf_const_const |= AD_CNST_USUPLMT;
			outstr[resultlen++] = subchar;
			if (!substitution_err && !substitution_on) 
                        {
                            uni_char_err = uni_char;
                            substitution_err = TRUE;
                        }
                    }
                    else     
                    {
                        bytenum = BT_BYTE_NUMS_MACRO (result);
                        result = BT_SWAP_BYTES_MACRO (result);  
#ifdef BIG_ENDIAN_INT
                        if (bytenum == 1)
                          outstr[resultlen] = *((char *)(&result) + 1);
                        else
#endif
                        {
			    /*
			    ** Bug 123389 (zhayu01)
			    ** If bytenum + resultlen is larger than outlen,
                            ** break out the loop.
			    */
			    if (bytenum + resultlen > outlen)
			        break;
                            MEcopy( (PTR)&result, bytenum, &outstr[resultlen]);
                        }

                        resultlen += bytenum;
                    }
                }
            }
            else 
            {
                /*
                ** Either basechar is zero or subc is zero here.
                ** If basechar is zero, indicating a baseless combining
                ** codepoint, set basechar to the codepoint from the input
                ** string and treat the combining codepoint as a single
                ** Unicode character.
                */
                if (basechar == 0)
                {
                    basechar = instr[count];
                }
                result = adu_map_get_unitochar (
                    (((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->unimapptr)->unimap, 
                    &basechar);

                if ((result == 0) && (basechar != 0))
                {
                    uni_char = basechar;
		    for (;;)
                    {
			UCS2 save_bc = basechar;
			basechar = adu_get_recomb (adf_scb, save_bc, 0);
			if (basechar == FAIL)
			    break;
			result = adu_map_get_unitochar (
                          (((ADU_MAP_INFO *)(Adf_globs->Adu_maptbl))->unimapptr)->unimap
                          , &basechar);
                        if (result == 0 && basechar != 0 && basechar != save_bc)
                            continue;
                        else 
                            break;
                    }                  
                }

                if ((result == 0) && (basechar != 0))
                {
		    adf_scb->adf_const_const |= AD_CNST_USUPLMT;
                    if (substitution_on) 
			result = subchar;
                    else
                    {
                        adu_error(adf_scb, E_AD5016_NOUNIMAP_FOUND, 2, 
                            sizeof(uni_char), &uni_char);
                        MEfree ((char *)comblist);
                        return (E_DB_ERROR);    
                    }
                }           

                bytenum = BT_BYTE_NUMS_MACRO (result); 
                result = BT_SWAP_BYTES_MACRO (result);
#ifdef BIG_ENDIAN_INT
                if (bytenum == 1)
                    outstr[resultlen] = *((char *)(&result) + 1);
                else
#endif
                {
		    /*
		    ** Bug 123389 (zhayu01)
		    ** If bytenum + resultlen is larger than outlen,
                    ** break out the loop.
		    */
		    if (bytenum + resultlen > outlen)
	 	        break;
                    MEcopy( (PTR)&result, bytenum, &outstr[resultlen]);
                }

                resultlen += bytenum;
            }
        }

        /* free work buffer */
        MEfree((char *)comblist);
    }

    db_stat = adu_movestring (adf_scb, (u_char *)outstr, resultlen, dv_in->db_datatype, rdv);
    if (substitution_err)
    {
        adu_error(adf_scb, E_AD5016_NOUNIMAP_FOUND, 2, 
			sizeof(uni_char_err), &uni_char_err);
        return (E_DB_ERROR);
    }
    return db_stat;
}

/*
** Name: adu_utf8_to_ucs2 - does conversion for UTF8 character data to UCS2 data.
**
** Description:
**      This is the actual routine that does the conversion from UTF8 to UCS2
**      conversion.
**
** Inputs:
**      adf_scb                         scb. Mostly used for error returning.
**      sourceStart                     is the start of the UTF8 string to convert.
**      sourceEnd                       is the end of the source UTF8 string.
**      targetStart                     is the start of the location where
**                                      resultant UCS2 string is placed.
**      sourceStart                     is the start of the UTF8 string.
**      reslen                          location to place the result length.
**
** Outputs:
**      reslen                          Result length is placed here
**      targetStart                     Contains result UCS2 string.
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none.
**
**      Note:  As per the ANSI standard a routine which writes out data to a target 
**	field, shorter in length than the source data, should return an error to the 
**	user, however since in Ingres we follow the paradigm that truncated data is 
**	silently written out the target area without returning an error message this 
**	routine follows the Ingres paradigm, to avoid change in behaviour for 
**	existing applications.
**
** Side Effects:
**          none
**
** History:
**     16-apr-2002  gupsh01
**          Added.
*/
static DB_STATUS
adu_utf8_to_ucs2 ( 
        ADF_CB            *adf_scb,
        UTF8**            sourceStart,
        const UTF8*       sourceEnd,
        UCS2**            targetStart,
        const UCS2*       targetEnd,
        i2                *reslen)
{
DB_STATUS           result = E_DB_OK;
register UTF8*              source = *sourceStart;
register UCS2*              target = *targetStart;
register UCS4               ch;
register u_i2 		    extraBytesToWrite;
const    i4                 halfShift = 10;

const    UCS4   halfBase = 0x0010000UL, halfMask = 0x3FFUL,
                kReplacementCharacter = 0x0000FFFDUL,
                kMaximumUCS2 = 0x0000FFFFUL, kMaximumUCS4 = 0x7FFFFFFFUL,
                kSurrogateHighStart = 0xD800UL, kSurrogateHighEnd = 0xDBFFUL,
                kSurrogateLowStart = 0xDC00UL, kSurrogateLowEnd = 0xDFFFUL;

         UCS4   offsetsFromUTF8[6] =
                {0x00000000UL, 0x00003080UL, 0x000E2080UL,
                 0x03C82080UL, 0xFA082080UL, 0x82082080UL};

         char   bytesFromUTF8[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5};

        while (source < sourceEnd) {
                ch = 0;
                extraBytesToWrite = bytesFromUTF8[*source];
                if (source + extraBytesToWrite > sourceEnd)
                {
                  *reslen = target - *targetStart;
                   return (adu_error(adf_scb, E_AD5010_SOURCE_STRING_TRUNC, 0));
                }

                switch(extraBytesToWrite) /* note: code falls through cases! */
                {
                case 5:
                        ch += *source++;
                        ch <<= 6;
                case 4:
                        ch += *source++;
                        ch <<= 6;
                case 3:
                        ch += *source++;
                        ch <<= 6;
                case 2:
                        ch += *source++;
                        ch <<= 6;
                case 1:
                        ch += *source++;
                        ch <<= 6;
                case 0:
                        ch += *source++;
                }
                ch -= offsetsFromUTF8[extraBytesToWrite];

                if (target >= targetEnd)
                {
                    *reslen = target - *targetStart;
                    return result;
                }

                if (ch <= kMaximumUCS2)
                        *target++ = ch;
                else if (ch > kMaximumUCS4)
                        *target++ = kReplacementCharacter;
                else {
                        if (target + 1 >= targetEnd)
                        {
                           *reslen = target - *targetStart;
                           return result;
                        }
                        ch -= halfBase;
                        *target++ = (ch >> halfShift) + kSurrogateHighStart;
                        *target++ = (ch & halfMask) + kSurrogateLowStart;
                }
        }
        *reslen = target - *targetStart;
        return result;
}

/*{
** Name: adu_ucs2_to_utf8 - does conversion for UTF8 character data to UCS2 data.
**
** Description:
**      This is the actual routine that does the conversion from UTF8 to UCS2
**      conversion.
**
** Inputs:
**      adf_scb                         scb. Mostly used for error returning.
**      sourceStart                     is the start of the UTF8 string to convert.
**      sourceEnd                       is the end of the source UTF8 string.
**      targetStart                     is the start of the location where
**                                      resultant UCS2 string is placed.
**      sourceStart                     is the start of the UTF8 string.
**      reslen                          location to place the result length.
**
** Outputs:
**      reslen                          Result length is placed here
**      targetStart                     Contains result UCS2 string.
**
**      Returns:
**          DB_STATUS
**
**      Exceptions:
**          none.
**
**      Note:  As per the ANSI standard a routine which writes out data to a target 
**	    field, shorter in length than the source data, should return an error to the 
**	    user,however since in Ingres we follow the paradigm that truncated data is 
**	    silently written out the target area without returning an error message this 
**	    routine follows the Ingres paradigm, to avoid change in behaviour for existing 
**	    applications.
**
** Side Effects:
**          none
**
** History:
**     16-apr-2002  gupsh01
**          Added.
*/
static DB_STATUS
adu_ucs2_to_utf8 (
      ADF_CB      *adf_scb,
      UCS2**      sourceStart,
const UCS2*       sourceEnd,
      UTF8**      targetStart,
const UTF8*       targetEnd,
i2                *reslen)
{
DB_STATUS           result = E_DB_OK;
register UCS2*              source = *sourceStart;
register UTF8*              target = *targetStart;
register UCS4               ch, ch2;
register u_i2 		    bytesToWrite;
register const UCS4         byteMask = 0xBF;
register const UCS4         byteMark = 0x80;
const    i4                 halfShift = 10;
         UTF8               firstByteMark[7] =
                            {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

const    UCS4   halfBase = 0x0010000UL, halfMask = 0x3FFUL,
                kReplacementCharacter = 0x0000FFFDUL,
                kMaximumUCS2 = 0x0000FFFFUL, kMaximumUCS4 = 0x7FFFFFFFUL,
                kSurrogateHighStart = 0xD800UL, kSurrogateHighEnd = 0xDBFFUL,
                kSurrogateLowStart = 0xDC00UL, kSurrogateLowEnd = 0xDFFFUL;


 while (source && target && source < sourceEnd) {
              bytesToWrite = 0;
              ch = *source++;
              if (ch >= kSurrogateHighStart && ch <= kSurrogateHighEnd
                              && source < sourceEnd)
              {
                     ch2 = *source;
                     if (ch2 >= kSurrogateLowStart && ch2 <= kSurrogateLowEnd)
                     {
                              ch = ((ch - kSurrogateHighStart) << halfShift)
                                      + (ch2 - kSurrogateLowStart) + halfBase;
                              ++source;
                     }
               }
               if (ch < 0x80)
                     bytesToWrite = 1;
               else if (ch < 0x800)
                     bytesToWrite = 2;
               else if (ch < 0x10000)
                     bytesToWrite = 3;
               else if (ch < 0x200000)
                     bytesToWrite = 4;
               else if (ch < 0x4000000)
                     bytesToWrite = 5;
               else if (ch <= kMaximumUCS4)
                     bytesToWrite = 6;
               else {
                     bytesToWrite = 2;
                     ch = kReplacementCharacter;
                    }

               target += bytesToWrite;
               if (target > targetEnd)
               {
                     target -= bytesToWrite;
                     *reslen = target - *targetStart;
                     return result;
               }
               switch (bytesToWrite)   /* note: code falls through cases! */
               {
                case 6:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 5:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 4:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 3:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 2:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 1:
                        *--target =  ch | firstByteMark[bytesToWrite];
                }
                target += bytesToWrite;
        }
        *reslen = target - *targetStart;
        return result;
}

/*{
** Name: check_utf8 - checks if the server charset is UTF8
**
** Description:
**      This routine checks if the server characterset is set to UTF8.
**
** Inputs:
**
** Outputs:
**
**      Returns - True if II_CHARSET'$II_INSTALLATION' is set to UTF8
**              - False otherwise
**
** Side Effects:
**          none
**
** History:
**     16-apr-2002  gupsh01
**          Added.
**	14-Jun-2004 (schka24)
**	    Avoid env variable stuff, let CM do it safely.
**	11-Jul-2005 (schka24)
**	    X-integration negated above fix, re-fix.
*/
static bool
check_utf8()
{
    char        chname[CM_MAXATTRNAME+1];
    static	bool inited = FALSE;

    if (inited && Adf_globs)
    {
	if (Adf_globs->Adi_status & ADI_UTF8)
	    return (TRUE);
	else
	    return (FALSE);
    }

    CMget_charset_name(&chname[0]);
    if (STcasecmp(chname, "UTF8") == 0)
    {
	if (Adf_globs)
	{
	    Adf_globs->Adi_status |= ADI_UTF8;
	    inited = TRUE;
	}
	return (TRUE);
    }
    if (Adf_globs)
    {
	Adf_globs->Adi_status &= ~ADI_UTF8;
	inited = TRUE;
    }
    return (FALSE);
}

/* Name: adu_nvchr_strtuf8conv - This funciton converts normal characters 
**	 to unicode UTF8 format.
**
**	This function is only meant for internal use. This function is 
**	useful to convert the default value string (varchar) described in 
**	ascii copy statements for nchar/nvarchar types.
**
**	The ascii copy uses internal format UTF8 datatype to write the
**	unicode string in UTF8 format into the ascii data file.
**
** History:
**     13-jul-2002  gupsh01
**          Added.
**	27-sep-2004 (gupsh01)
**	    Fixed length calculation of the intermediate UTF8 data. 
**	    (bug 113139).
**	06-sep-2007 (gupsh01)
**	    Fixed length calculation yet again for the intermediate
**	    UTF8 data.
*/
DB_STATUS
adu_nvchr_strutf8conv(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv_in,
DB_DATA_VALUE     *dv_out)
{
    UCS2 *ustr = 0;
    char *cstr = 0;
    DB_DATA_VALUE     dv_tmp;
    i4 ulen = 0;
    i4 stlen = 0;
    i2 retlen = 0;
    DB_STATUS db_stat = E_DB_OK;

    dv_tmp.db_data = 0;

    while (1)
    {
      dv_tmp.db_datatype = DB_NVCHR_TYPE;
      dv_tmp.db_length = (dv_in->db_length > dv_out->db_length) ? dv_in->db_length * 4 : dv_out->db_length * 4;
      dv_tmp.db_prec = 0;

      dv_tmp.db_data = (char *) MEreqmem (0, dv_tmp.db_length, TRUE, &db_stat);
      if (db_stat)
	break;

      if ((dv_in->db_datatype == DB_UTF8_TYPE) &&
	   ((dv_out->db_datatype == DB_VCH_TYPE) || (dv_in->db_datatype == DB_CHA_TYPE)))
      {
	 dv_tmp.db_length = dv_in->db_length * 2;

	 if (dv_tmp.db_length > DB_MAXSTRING)
	   dv_tmp.db_length = DB_MAXSTRING;

	 db_stat = adu_nvchr_fromutf8( adf_scb, dv_in, &dv_tmp);
	 if (db_stat) 
	break;

	 db_stat = adu_nvchr_unitochar (adf_scb, &dv_tmp, dv_out);
	 if (db_stat) 
	break;

      }
      else if ((dv_out->db_datatype == DB_UTF8_TYPE) &&
	   ((dv_in->db_datatype == DB_VCH_TYPE) || (dv_in->db_datatype == DB_CHA_TYPE)))
      {
	 dv_tmp.db_length = dv_in->db_length * 4;

	 db_stat = adu_nvchr_chartouni (adf_scb, dv_in, &dv_tmp);
	 if (db_stat) 
	break;

	 db_stat = adu_nvchr_toutf8( adf_scb, &dv_tmp, dv_out);
	 if (db_stat) 
	break;

      }
      else 
      {
	 if (dv_tmp.db_data) 
	   MEfree (dv_tmp.db_data);
	 return adu_error (adf_scb, E_AD5081_UNICODE_FUNC_PARM, 0);
      }
      break;
    }

    if (dv_tmp.db_data) 
      MEfree (dv_tmp.db_data);

    return (db_stat);
}

/*
** Name: adu_nvchr_toupper() - Convert unicode string to uppercase.
**
** Description:
**	  This routine is a wrapper to adu_nvchar_casemap. 
**
** History:
**     09-mar-2007 (gupsh01)
**          Created.
**
*/
DB_STATUS
adu_nvchr_toupper(
ADF_CB              *scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *rdv)
{
    return ( ad0_nvchr_casemap( scb, dv1, rdv, ADU_CASEMAP_UPPER)); 
}

/*
** Name: adu_nvchr_tolower() - Convert unicode string to lowercase.
**
** Description:
**	  This routine is a wrapper to adu_nvchar_casemap. 
**
** History:
**     09-mar-2007 (gupsh01)
**          Created.
**
*/
DB_STATUS
adu_nvchr_tolower(
ADF_CB              *scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *rdv)
{
    return ( ad0_nvchr_casemap( scb, dv1, rdv, ADU_CASEMAP_LOWER)); 
}

/*{
** Name: adu_nvchr_utf8comp() - Convert input UTF8 parameters to UCS2 and 
**			    do UCA comparison.
**
** Description:
**	  This function compares UTF8 values from c/text/char/varchar columns
**	  with the Unicode Collation Algorithm by converting them first to UCS2
**	  and then calling aduucmp() to perform the comparison.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	use_quel_pm			Figure out if quel pattern matching 
**					is to be used.
**      dv1                             Ptr to comparand 1 DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to source string to be compared.
**      dv2                             Ptr to comparand 2 DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to source string to be compared.
**	rcmp				Ptr to integer result of comparison.
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
**	rcmp				Ptr to integer result of comparison:
**					< 0 if comparand1 < comparand2
**					= 0 if comparand1 = comparand2
**					> 0 if comparand1 > comparand2
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
** History:
**     9-may-2007 (dougi)
**          Written for UTF8 server support.
**	14-may-2007 (gupsh01)
**	    Pass nfc flag to adu_nvchr_fromutf8 call, to force normalization
**	    to NFC. 
**	01-jun-2007 (gupsh01)
**	    Fix the end point calculation for passing into the aduucmp routine.
**	31-aug-2007 (gupsh01)
**	    Adjust the temporary buffer length calculation.
**	07-sep-2007 (gupsh01)	
**	    Fix lengths for intermediate calculations for UTF8.
**	05-oct-2007 (gupsh01)
**	    Fix comparison for quel types.
**	24-oct-2007 (gupsh01)
**	    Added support for 'C' style pattern matching as well
**	    full quel pattern matching support for UTF8 character set.
**	05-nov-2007 (gupsh01)
**	    Added parameter use_quel_pm to support quel comparison logic.
**	26-mar-2008 (gupsh01)
**	    Ignore blanks in comparison if we are comparing C types.
**	04-Feb-2009 (gupsh01)
**	    Use ADF_UNI_TMPLEN define for temporary buffer space for
**	    unicode coercion. Also free the memory allocated for
**	    temporary buffers.
*/
DB_STATUS
adu_nvchr_utf8comp(
ADF_CB              *scb,
i4		    use_quel_pm,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *dv2,
i4		    *rcmp)
{
    DB_STATUS	db_stat;
    STATUS	stat;
    i4		comp1len, comp2len;
    char	tempc1[ADF_UNI_TMPLEN], tempc2[ADF_UNI_TMPLEN];
    char	*c1ptr, *c2ptr;
    char	*endc1ptr, *endc2ptr;
    DB_DATA_VALUE comp1, comp2;
    bool	qpat = FALSE;
    bool	bignore = FALSE;
    bool	memexpanded1 = FALSE;
    bool	memexpanded2 = FALSE;
    i2		saved_uninorm_flag = scb->adf_uninorm_flag;

    /* Figure out space required for conversion and whether MEreqmem
    ** is required. */
    if (db_stat = adu_lenaddr(scb, dv1, &comp1len, (char **)&c1ptr))
	return(db_stat);
    if (db_stat = adu_lenaddr(scb, dv2, &comp2len, (char **)&c2ptr))
	return(db_stat);

    comp1len = comp1len * 2 + DB_CNTSIZE; 
    comp2len = comp2len * 2 + DB_CNTSIZE; 

    if (comp1len > DB_MAXSTRING)
      comp1len = DB_MAXSTRING;

    if (comp2len > DB_MAXSTRING)
      comp2len = DB_MAXSTRING;

    if (comp1len > ADF_UNI_TMPLEN)
    {
#ifdef xDEBUG 
	TRdisplay ("adu_nvchr_utf8comp - MEreqmem call made for %d bytes\n", comp1len);
	/* Expensive MEreqmem() call. */
#endif
	c1ptr = MEreqmem(0, comp1len, TRUE, &stat);
	if (stat)
	    return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(stat), (i4 *)&stat));
        memexpanded1 = TRUE;
    }
    else c1ptr = &tempc1[0];

    if (comp2len > ADF_UNI_TMPLEN)
    {
#ifdef xDEBUG 
	TRdisplay ("adu_nvchr_utf8comp - MEreqmem call made for %d bytes\n", comp1len);
#endif
	/* Expensive MEreqmem() call. */
	c2ptr = MEreqmem(0, comp2len, TRUE, &stat);
	if (stat)
	    return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(stat), (i4 *)&stat));
        memexpanded2 = TRUE;
    }
    else c2ptr = &tempc2[0];

    /* Set up UCS2 DB_DATA_VALUEs. */
    comp1.db_datatype = DB_NVCHR_TYPE;
    comp1.db_length = comp1len;
    comp1.db_prec = 0;
    comp1.db_collID = 0;
    comp1.db_data = c1ptr;

    comp2.db_datatype = DB_NVCHR_TYPE;
    comp2.db_length = comp2len;
    comp2.db_prec = 0;
    comp2.db_collID = 0;
    comp2.db_data = c2ptr;

    /* Take a breath and convert to UCS2. */
    scb->adf_uninorm_flag = AD_UNINORM_NFC;
    if (db_stat = adu_nvchr_fromutf8(scb, dv1, &comp1))
	goto err_return;
    if (db_stat = adu_nvchr_fromutf8(scb, dv2, &comp2))
	goto err_return;
    scb->adf_uninorm_flag = saved_uninorm_flag;

      /* Finally, set up parameters and call aduucmp() to perform 
      ** the comparison. */
      if (db_stat = adu_lenaddr(scb, &comp1, &comp1len, &c1ptr))
	goto err_return;
      if (db_stat = adu_lenaddr(scb, &comp2, &comp2len, &c2ptr))
	goto err_return;

      endc1ptr = c1ptr + comp1len; 
      endc2ptr = c2ptr + comp2len;

    if (use_quel_pm) 
    {
	u_i2 *ptr2 = (u_i2 *)c2ptr; 
	while (ptr2 < (u_i2 *)endc2ptr)
	{
	    if ( (qpat == FALSE) &&
                    ((*ptr2 == DB_PAT_ANY) ||
                     (*ptr2 == DB_PAT_ONE) ||
                     (*ptr2 == DB_PAT_LBRAC) ||
                     (*ptr2 == DB_PAT_RBRAC))
                 ) 
	    {
              qpat = TRUE;
	      break;
	    }
	    ptr2++;
	}
    }
    if ((dv1->db_datatype == DB_CHR_TYPE) || 
	(dv2->db_datatype == DB_CHR_TYPE))
	bignore = TRUE;

    if (qpat)
      db_stat = adu_ulexcomp (scb, &comp1, &comp2, rcmp, bignore);
    else
    {
      if (bignore)
        db_stat = aduucmp(scb, (ADUL_BLANKPAD | ADUL_SKIPBLANK), (u_i2 *)c1ptr, 
			(u_i2 *)endc1ptr,
			(u_i2 *)c2ptr, (u_i2 *)endc2ptr, rcmp, 0);
      else
        db_stat = aduucmp(scb, ADUL_BLANKPAD, (u_i2 *)c1ptr, 
			(u_i2 *)endc1ptr,
			(u_i2 *)c2ptr, (u_i2 *)endc2ptr, rcmp, 0);
    }

err_return:

   scb->adf_uninorm_flag = saved_uninorm_flag;
   if ( memexpanded1 == TRUE)
   {
	stat =	MEfree (comp1.db_data);
#ifdef xDEBUG 
        if (stat)
	  TRdisplay ("adu_nvchr_utf8comp - MEfree status = %d \n", stat);
#endif
    }

   if ( memexpanded2 == TRUE)
   {
	stat = MEfree (comp2.db_data);
#ifdef xDEBUG 
        if (stat)
	  TRdisplay ("adu_nvchr_utf8comp - MEfree status = %d \n", stat);
#endif
    }

    return(db_stat);
}

/*{
** Name: adu_nvchr_utf8like() - Convert input UTF8 parameters to UCS2 and 
**			    do Unicode like.
**
** Description:
**	  This function performs the like predicate on UTF8 values from 
**	  c/text/char/varchar columns by converting them to UCS2, then calling
**	  adu_ulike() to perform the comparison.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to comparand DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to source string to be compared.
**      dv2                             Ptr to pattern DB_DATA_VALUE.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to source string to be compared.
**	eptr				Ptr to escape value (or NULL).
**	rcmp				Ptr to integer result of comparison.
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
**	rcmp				Ptr to integer result of comparison:
**					< 0 if comparand1 < comparand2
**					= 0 if comparand1 = comparand2
**					> 0 if comparand1 > comparand2
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
** History:
**     16-may-2007 (dougi)
**          Cloned for UTF8 server support from adu_nvchr_utf8comp.
**	16-oct-2007 (gupsh01)
**	    When passing on the escape character make sure that the
**	    escape character is converted to UTF8.
**	08-jul-2008 (gupsh01)
**	     Like pattern matching on C columns does not ignore blanks or
**	     nulls.
**	04-Feb-2009 (gupsh01)
**	    Use ADF_UNI_TMPLEN define for temporary buffer space for
**	    unicode coercion. Also free the memory allocated for the
**	    temporary buffer.
*/
DB_STATUS
adu_nvchr_utf8like(
ADF_CB              *scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *dv2,
char		    *eptr,
i4		    *rcmp)
{
    DB_STATUS	db_stat;
    STATUS	stat;
    i4		comp1len, comp2len;
    char	tempc1[ADF_UNI_TMPLEN], tempc2[ADF_UNI_TMPLEN];
    char	*c1ptr, *c2ptr;
    char	*endc1ptr, *endc2ptr;
    DB_DATA_VALUE comp1, comp2, escdb, uescdb;
    UCS2	uesc = U_NULL;
    bool	bignore = FALSE;
    bool	memexpanded1 = FALSE;
    bool	memexpanded2 = FALSE;
    i2		saved_uninorm_flag = scb->adf_uninorm_flag;


    /* Figure out space required for conversion and whether MEreqmem
    ** is required. */
    if (db_stat = adu_lenaddr(scb, dv1, &comp1len, (char **)&c1ptr))
	return(db_stat);
    if (db_stat = adu_lenaddr(scb, dv2, &comp2len, (char **)&c2ptr))
	return(db_stat);

    comp1len = comp1len * 2 + DB_CNTSIZE;
    comp2len = comp2len * 2 + DB_CNTSIZE;

    if (comp1len > DB_MAXSTRING)
      comp1len = DB_MAXSTRING;

    if (comp2len > DB_MAXSTRING)
      comp2len = DB_MAXSTRING;

    if (comp1len > ADF_UNI_TMPLEN)
    {
	/* Expensive MEreqmem() call. */
	c1ptr = MEreqmem(0, comp1len, TRUE, &stat);
	if (stat)
	    return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(stat), (i4 *)&stat));
	memexpanded1 = TRUE;
    }
    else c1ptr = &tempc1[0];

    if (comp2len > ADF_UNI_TMPLEN)
    {
	/* Expensive MEreqmem() call. */
	c2ptr = MEreqmem(0, comp2len, TRUE, &stat);
	if (stat)
	    return (adu_error(scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(stat), (i4 *)&stat));
	memexpanded2 = TRUE;
    }
    else c2ptr = &tempc2[0];

    /* Set up UCS2 DB_DATA_VALUEs. */
    comp1.db_datatype = DB_NVCHR_TYPE;
    comp1.db_length = comp1len;
    comp1.db_prec = 0;
    comp1.db_collID = 0;
    comp1.db_data = c1ptr;

    comp2.db_datatype = DB_NVCHR_TYPE;
    comp2.db_length = comp2len;
    comp2.db_prec = 0;
    comp2.db_collID = 0;
    comp2.db_data = c2ptr;

    /* Take a breath and convert to UCS2. */
    scb->adf_uninorm_flag = AD_UNINORM_NFC;
    if (db_stat = adu_nvchr_fromutf8(scb, dv1, &comp1))
	goto failreturn;
    if (db_stat = adu_nvchr_fromutf8(scb, dv2, &comp2))
	goto failreturn;
    scb->adf_uninorm_flag = saved_uninorm_flag;

    /* If the source string or pattern string is c type then 
    ** squeeze blanks and nulls in the source and pattern string.  
    */ 
    if ((dv1->db_datatype == DB_CHR_TYPE) ||
        (dv2->db_datatype == DB_CHR_TYPE))
    {
	i2   srccnt = 0;	
	i2   patcnt = 0;	
	UCS2 *bs;
	UCS2 *es;
	i2  *cntptr;
	UCS2 *bufp;

	/* squeeze blanks and nulls from source string in place */
	cntptr = &(((DB_NVCHR_STRING *)(comp1.db_data))->count);
	srccnt = *cntptr;
	bs = bufp = ((DB_NVCHR_STRING *)(comp1.db_data))->element_array;
	es = bs + srccnt;
	while (bs < es)
        {
          if ((*bs == U_BLANK) || (*bs == U_NULL))
          {
             bs++; srccnt--;
          }
          else
             *bufp++ = *bs++;
        }
	*cntptr = srccnt;

	/* squeeze blanks and nulls from pattern string in place*/
	cntptr = &(((DB_NVCHR_STRING *)(comp2.db_data))->count);
	patcnt = *cntptr;
	bs = bufp = ((DB_NVCHR_STRING *)(comp2.db_data))->element_array;
	es = bs + patcnt;
	while (bs < es)
        {
          if ((*bs == U_BLANK) || (*bs == U_NULL))
          {
             bs++; patcnt--;
          }
          else
             *bufp++ = *bs++;
        }
	*cntptr = patcnt;
    }
    /* If escape is given then convert the escape character to 
    ** unicode.
    */

    if (eptr && *eptr)
    {
      escdb.db_datatype = DB_CHR_TYPE;
      escdb.db_length = 1;
      escdb.db_prec = 0;
      escdb.db_collID = 0;
      escdb.db_data = eptr;

      uescdb.db_datatype = DB_NCHR_TYPE;
      uescdb.db_length = sizeof(UCS2);
      uescdb.db_prec = 0;
      uescdb.db_collID = 0;
      uescdb.db_data = (PTR)&uesc;

      scb->adf_uninorm_flag = AD_UNINORM_NFC;
      if (db_stat = adu_nvchr_fromutf8(scb, &escdb, &uescdb))
	goto failreturn;
      scb->adf_uninorm_flag = saved_uninorm_flag;
    }

    db_stat = adu_ulike(scb, &comp1, &comp2, (UCS2 *)&uesc, rcmp);

failreturn:
   scb->adf_uninorm_flag = saved_uninorm_flag;
   if ( memexpanded1 == TRUE)
	MEfree (comp1.db_data);

   if ( memexpanded2 == TRUE)
	MEfree (comp2.db_data);

    return(db_stat);
}

/*{
** Name: adu_nvchar_toupper() - Convert all lower case letters in a unicode
**			    string to uppercase.
**
** Description:
**	  This routine converts nchar/nvarchar letters in a source DB_DATA_VALUE
**	  string to lower/uppercase letters in a destination DB_DATA_VALUE. 
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
** History:
**     09-mar-2007 (gupsh01)
**          Created.
**	09-May-2007 (gupsh01)
**	    Added nfc_flag to adu_unorm to force normalization to NFC
**	    for UTF8 character sets.
**	31-Jul-2007 (gupsh01)
**	    For NFC conversion remove the U_NULL check added while 
**	    fixing bug 114365 to see if the unicode string is null 
**	    terminated. Embedded nulls are allowed in the unicode 
**	    string, just like regular varchar strings.
**	06-Aug-2007 (gupsh01)
**	    Streamline handling for final sigma case.
**	23-Jun-2008 (gupsh01)
**	    Previous bug fix for bug 118971, accidently removed 
**	    substring initialization, causing error in recombination
**	    from NFD.
**	04-Feb-2009 (gupsh01)
**	    Use ADF_UNI_TMPLEN define for temporary buffer space for
**	    unicode normalization.
*/
static DB_STATUS
ad0_nvchr_casemap(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv_in,
DB_DATA_VALUE       *rdv,
i4		    caseflag
)
{
    UCS2        *instr = 0;
    i4          inlen = 0;
    UCS2	*outstr = 0;
    i4          outlen = 0;
    ADU_CASEMAP *casemap;
    u_i2        result = 0;
    i4          index = 0;
    i4          pos = 0;
    i4          count = 0;              /* input codepoint position  */
    i4          subc = 0;
    DB_STATUS   db_stat;
    i2          resultlen = 0;          /* output character position */
    ADUUCETAB   *cetbl = (ADUUCETAB *) adf_scb->adf_ucollation;
    UCS2        basechar;
    UCS2        *comblist;
    STATUS      stat = OK;
    u_i2        uni_char;
    i4          i = 0;
    i4          normcstrlen = 0;
    bool        substitution_on = FALSE;
    u_i2        bytenum;
    u_i2 	unicp;
    u_i2 	after_unicp = 0;
    u_i2 	upper_lim;
    u_i2 	lower_lim;
    DB_DATA_VALUE	tempdb;
    char 	tempbuf[ADF_UNI_TMPLEN];
    char 	*bigbuf = NULL;
    bool	case_before_sigma = FALSE;
    bool	case_after_sigma = FALSE;

    if (db_stat = adu_lenaddr(adf_scb, dv_in, &inlen, (char **)&instr))
          return (db_stat);

    if (db_stat = adu_7straddr(adf_scb, rdv, &outstr))
        return (db_stat);

    inlen /= sizeof (UCS2);

    /* Output length is not known at this time.
    ** Calculate the length, based on space available in rdv.
    */
    switch (rdv->db_datatype)
    {
        case DB_NCHR_TYPE:
           outlen = rdv->db_length;
           break;

        case DB_NVCHR_TYPE:
           outlen = rdv->db_length - DB_CNTSIZE;
     	   break; 
    }

    if ( cetbl == NULL )
       return (adu_error(adf_scb, E_AD5012_UCETAB_NOT_EXISTS, 0));

    /* Allocate working buffer */
    comblist = (UCS2 *) MEreqmem (0, sizeof(UCS2) * inlen, TRUE, &stat);

    for (;
	    ((count < inlen) && (resultlen < outlen));
		    count+=1)
    {
        subc = 0;  	  /* Reset count of combining code points found */
        normcstrlen = 1;  /* Reset length of NFC version of string */

        uni_char = instr[count];

	/* The expected sequence of characters must conform to
	** pattern <Base><comb><comb>... 
	** However for lower/upper case operation a combining character will 
	** pass through unchanged, hence for our current purpose we consider 
	** all characters as base characters.
        */
        basechar = (cetbl[uni_char].comb_class == 0) ? uni_char : 0;

        if (basechar)
        {
            for (pos=count+1;
                (pos < inlen) && 
		  (uni_char = instr[pos]) && /* assignment */
                  (cetbl[uni_char].comb_class != 0);
                pos+=1, subc+=1 )
            {
                comblist[subc] = uni_char;  /* copy combining codepoint */
            }
            count = pos-1;
        }
	else 
	{
	   basechar = uni_char;
	}

        /* Convert the substring to Normal form C.  */
	if (subc > 0 )
          stat = adu_map_conv_normc (adf_scb, cetbl, 
                basechar, 
                comblist, subc, 
                comblist, &normcstrlen);
    
	/* Now convert to upper or lower case based on the request */
        for (i=0; i < normcstrlen; i++)
        {
	    if (subc)
	      unicp = comblist[i];
	    else
	      unicp = basechar;


	    if (cetbl[unicp].flags & CE_HASCASE)
	    {
	    	casemap = cetbl[unicp].casemap;
	        if (caseflag == ADU_CASEMAP_LOWER)
	        {
			
		    lower_lim = 0;
		    upper_lim = casemap->num_lvals; 
		    case_before_sigma = TRUE;
	        }
	        else if (caseflag == ADU_CASEMAP_TITLE)
	        {
		    lower_lim = casemap->num_lvals;
		    upper_lim = casemap->num_lvals + casemap->num_tvals;
	        }
	        else if (caseflag == ADU_CASEMAP_UPPER)
	        {
		    lower_lim = casemap->num_lvals + casemap->num_tvals;
		    upper_lim = casemap->num_lvals + casemap->num_tvals + 
		                casemap->num_uvals;
	        }
	        for (i = lower_lim; i < upper_lim; i++)
		{
		    if ((case_before_sigma) && (unicp == U_CAPITAL_SIGMA))
		    {
			i4 index = count;
			while (index < (inlen - 1))
			{
			  after_unicp = instr[index++];
			  if (cetbl[after_unicp].flags & CE_HASCASE)
			  {
			     case_after_sigma = TRUE;
			     break;
			  }
			  else if (!(cetbl[after_unicp].flags & CE_CASEIGNORABLE))
			     break;
			}

        		if (case_after_sigma)
			  outstr[resultlen++] = U_SMALL_SIGMA;
			else 
			  outstr[resultlen++] = U_SMALL_FINAL_SIGMA;
			case_after_sigma = FALSE;
			case_before_sigma = FALSE;

			after_unicp = 0;
		    }
		    else
                      outstr[resultlen++] = casemap->csvals[i];
		}
	    }
	    else
	    {
                  outstr[resultlen++] = unicp;
	          if ((case_before_sigma) && !(cetbl[unicp].flags & CE_CASEIGNORABLE))
			case_before_sigma = FALSE;
	    }
        }
    }
    MEfree((char *)comblist);

    /* For conversion we changed to NFC. Normalize esp. for NFD database. */
    if (rdv->db_length < ADF_UNI_TMPLEN)
	tempdb.db_data = tempbuf;
    else
	tempdb.db_data = bigbuf = (char *) MEreqmem(0, rdv->db_length+1,
				FALSE, &stat);
    tempdb.db_length = rdv->db_length;
    tempdb.db_datatype= rdv->db_datatype;
    tempdb.db_prec = rdv->db_prec;
    tempdb.db_collID = rdv->db_collID;
    db_stat = adu_moveunistring (adf_scb, outstr, resultlen, &tempdb); 

    if (db_stat = adu_unorm (adf_scb, rdv, &tempdb) != E_DB_OK)
        return (db_stat);
    if (bigbuf != NULL)
	MEfree((PTR) bigbuf);

    return db_stat;
}
