/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/**
**
**  Name: ADUSTRUTIL.C - Routines to support string processing and operations.
**
**  Description:
**        This file contains routines to support string, character and text,
**	processing and operations on strings.
**
**          adu_lenaddr() - Gets length and addr of string in a DB_DATA_VALUE.
**          adu_movestring() - Move a cstring into a DB_DATA_VALUE.
**	    adu_size() - Get the size of a string ignoring trailing blanks.
**          adu_3straddr() - Get the address of a string in a DB_DATA_VALUE.
**          adu_5strcount() - Get the length of a string in a DB_DATA_VALUE.
**          adu_7straddr() - Get the address of a string in a DB_DATA_VALUE
**		             for cases where the data type is nchar and 
**			     nvarchar type.
**	    du_moveunistring() - Move a unicode string into a DB_DATA_VALUE
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      23-apr-86 (ericj)    
**          Initial creation for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Adapted all routines to work on CHAR, VARCHAR, and LONGTEXT.
**	13-feb-87 (thurston)
**	    Made the adu_movestring routine increment the .ad_noprint_cnt field
**	    in the ADI_WARN struct of the ADF_CB if any non-printing character
**	    were converted to blanks.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	24-mar-87 (thurston)
**	    Made the following change to adu_movestring():  Even if the
**          `non-printing character' error occurs, do not return E_DB_WARN.
**          Instead, this will be handled in the same fashion that math
**          exceptions are handled; by calling adx_chkwarn() at the end of the
**          query. 
**	28-jul-87 (thurston)
**	    Added code to adu_movestring() to count null chars cvt'ed to blanks
**	    for text. 
**	03-aug-87 (thurston)
**	    Routine adu_movestring() now just count the number of strings that
**	    had chars cvt'ed to blank, not the number of chars cvt'ed to blank.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	07-jul-88 (jrb)
**	    Removed support for obsolete character types text0 and textn.
**	    Modified for Kanji support.
**	07-nov-88 (thurston)
**	    Added check in adu_5strcount() for count field in DB_TEXT_STRING
**	    structure to assure that the count is not totally hosed up.
**	30-jan-1990 (fred)
**	    Added support for DB_LVCH_TYPE.
**	26-apr-90 (jrb)
**	    Fixed bug (no number) where instead of converting NULLs to blanks
**	    when moving into a TEXT string, we were removing the NULLs
**	    altogether.
**      05-jan-1993 (stevet)
**          Added function prototypes.
**      05-Apr-1993 (fred)
**          Generally modified so that this routines would work with
**          the byte datatypes as well, since the byte datatypes are
**          [purposefully] just clones of [var]char.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      08-jun-1993 (stevet)
**          Return error on truncated string.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      30-Sep-1993 (fred)
**          Fix byte datatypes so that just as blanks are trimmed from 
**          the character datatypes, null characters are trimmed from 
**          the byte datatypes.
**	14-mar-1995 (peeje01)
**          Cross integration of DBCS change from 6500db_su4_us42. Original
**	    comment follows:
**	03-nov-93 (twai)
**	    Changed to NOT do "CM" type character handling on the char,
**	    varchar, and longtext types (DB_CHA_TYPE, DB_VCH_TYPE, and
**	    DB_LTXT_TYPE).  These types may contain any arbitrary characters
**	    (bytes).  It is incorrect to assume any sort of structure or
**	    meaning with this type of data.  In particular, multi-byte
**	    versions will incorrectly try to interpret arbitrary data, with
**	    unpredictable results.  In the case of Windows4GL, which splits
**	    objects into n number of tuples and stores them in the table
**	    ii_srcobj_encoded, the results were disastrous.  Data on the
**	    split boundary would be truncated, corrupting the object and
**	    rendering it unretrievable.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
** 	29-mar-2001 (gupsh01) 
**	    added new functions adu_7straddr() and adu_moveunistring() 
**	    in order to support unicode string operations. 
**	11-oct-2001 (abbjo03)
**	    In adu_moveunistring(), correct calculation of output string length.
**	13-dec-2001 (devjo01)
**	    In adu_5strcount make case for NCHR consistent with NVCHR case by
**          returning number of unicode chars, not raw byte size.
**  11-Apr-2005 (fanra01)
**      Bug 114274
**      Insert into a Unicode database created for NFC normalization causes
**      exception.
**      Adjust calculation of the end of Unicode string pointer to use
**      characters and not buffer length.
**	12-Dec-2008 (kiria01) b121366
**	    Protect against unaligned access on Solaris
**      16-Jun-2009 (thich01)
**          Treat GEOM type the same as LBYTE.
**      20-Aug-2009 (thich01)
**          Treat all spatial types the same as LBYTE.
**      12-Oct-2010 (horda03) b124550
**          Improve performance of string padding.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. No need for the reduced length
**	    as it doesn't use UCS2 for CEs.
**/


/*{
** Name: adu_lenaddr() - gets length and address of the string in a
**			 DB_DATA_VALUE.
**
** Description:
**	  For text, longtext, and varchar types this gets address of the actual
**      text, not the address of the db_data in the DB_DATA_VALUE.  Length is
**	declared length for c and char types, actual length for text, longtext,
**	and varchar types.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv				Ptr to the DB_DATA_VALUE.
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
**      len				Ptr to place to put length.
**      addr				Ptr to place to put address of string.
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
**	    E_AD5001_BAD_STRING_TYPE	The DB_DATA_VALUE was not a valid string
**					type.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**  History:
**      02/22/83 -- Written by Jeff Lichtman
**      03/03/83 -- changed to use straddr and strcount
**	7-apr-86 (ericj) -- modified for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Now works for char, varchar, and longtext.
**	24-nov-99 (inkdo01)
**	    Move 3straddr/5strcount code inline - called funcs are a bit silly.
**	26-mar-2001 (stephenb)
**	    Add support for nchar and nvarchar types.
**	03-dec-2003 (gupsh01)
**	    Added support for UTF8 internal data type.
**	20-Sep-2007 (gupsh01)
**	    Nvarchar used in intermediate calculations
**	    for UTF8 character set can go upto DB_MAXSTRING.
**	01-Oct-2008 (gupsh01)
**	    For UTF8 installations add a check to ensure 
**	    that the length value does not exceed maxstring/2. 
**	05-Feb-2009 (kiria01) SIR 120473
**	    Handle DB_PAT_TYPE too.
*/

DB_STATUS
adu_lenaddr(
ADF_CB			    *adf_scb,
register    DB_DATA_VALUE   *dv,
register    i4		    *len,
register    char	    **addr)
{
    DB_STATUS	    db_stat;
    u_i2	    si;
    /* Following is a merge of the 3straddr() and 5strcount() code, except
    ** DB_DEC_TYPE case from 3straddr() and DB_LVCH/LBYTE_TYPE cases from 
    ** 5strcount() are dropped, since all return error from one or other of 
    ** 3str/5str. */

    switch (dv->db_datatype)
    {
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
	case DB_NCHR_TYPE:
            *len = dv->db_length;
	    *addr = (char *) dv->db_data;
            break;

        case DB_LTXT_TYPE:
	case DB_VBYTE_TYPE:
	case DB_UTF8_TYPE:
            I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, si);
	    if (si > adf_scb->adf_maxstring)
		return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	    *len = si;
	    *addr = (char *) ((DB_TEXT_STRING *) dv->db_data)->db_t_text;
            break;

        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
            I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, si);
	    if (((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
		 dv->db_collID != DB_UCS_BASIC_COLL &&
		 (si > adf_scb->adf_maxstring/2)) ||
		(si > adf_scb->adf_maxstring))
		return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	    *len = si;
	    *addr = (char *) ((DB_TEXT_STRING *) dv->db_data)->db_t_text;
            break;

	case DB_NVCHR_TYPE:
	case DB_PAT_TYPE:
            I2ASSIGN_MACRO(((DB_NVCHR_STRING *) dv->db_data)->count, si);
	    si *= sizeof(UCS2);
	    if (si > DB_MAXSTRING)
		return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	    *len = si;
	    *addr = (char *) ((DB_NVCHR_STRING *) dv->db_data)->element_array;
            break;

        default:
	    return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }
    return (E_DB_OK);
}


/*{
** Name: adu_movestring() - moves a string into a DB_DATA_VALUE.
**
** Description:
**        Moves a "C" string into a DB_DATA_VALUE padding the DB_DATA_VALUE
**	string appropriately.  If the DB_DATA_VALUE string is of type c
**	any non-printing characters will be mapped to blanks.  If the string
**	is of type text, then null characters are converted to blanks.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      source                          The source "C" string.
**      sourcelen                       The length of the source string.
**	src_type			The data type of the source string. 
**					When this value is provided it is 
**					used for byte input which should not
**					be treated as if they were character 
**					even when it is stored in a character
**					output.
**      dest	                        Ptr to the destination DB_DATA_VALUE.
**	    .db_datatype		Datatype of the DB_DATA_VALUE.
**	    .db_length			Length of the result DB_DATA_VALUE.
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
**      dest                            Ptr to the destination DB_DATA_VALUE.
**	    .db_data			Ptr to memory where string is moved to.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Routine was successful.
**	    E_AD5001_BAD_STRING_TYPE	The destination DB_DATA_VALUE was an
**					incompatible datatype.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02/22/83 -- Written by Jeff Lichtman
**      02/24/83 (lichtman) -- rewrote to keep bad chars out of
**	    character type strings
**      8/2/85 (peb) -- Updated isprint for multinational characters.
**              This is now merged with EBCDIC support.
**      9/25/85 (peb)   Updated movestring to properly move MAX_CHAR as
**              part of a string containing pattern match info.
**      23-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Made work for char, varchar, and longtext.
**	13-feb-87 (thurston)
**	    Made this routine increment the .ad_noprint_cnt field in the
**	    ADI_WARN struct of the ADF_CB if any non-printing character were
**	    converted to blanks.
**	24-mar-87 (thurston)
**	    Even if the `non-printing character' error occurs, do not return
**	    E_DB_WARN.  Instead, this will be handled in the same fashion that
**	    math exceptions are handled; by calling adx_chkwarn() at the end of
**	    the query.
**	28-jul-87 (thurston)
**	    Added code to count null chars cvt'ed to blanks for text.
**	03-aug-87 (thurston)
**	    Just count the number of strings that had chars cvt'ed to blank,
**	    not the number of chars cvt'ed to blank.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	07-jul-88 (jrb)
**	    Removed support for obsolete character types text0 and textn.
**	    Modified for Kanji support.
**	30-Jan-1990 (fred)
**	    Added support for long varchar datatype.
**	26-apr-90 (jrb)
**	    Fixed bug (no number) where instead of converting NULLs to blanks
**	    when moving into a TEXT string, we were removing the NULLs
**	    altogether.
**       9-Apr-1993 (fred)
**          Added support for byte string datatypes.
**      08-jun-1993 (stevet)
**          Return error on string overflow if string overflow detection is
**          turned on.
**      30-Sep-1993 (fred)
**          Fix byte datatypes so that just as blanks are trimmed from 
**          the character datatypes, null characters are trimmed from 
**          the byte datatypes.
**      31-oct-1994 (tutto01)
**          Added case for decimal support.
**	15-April-1999 (thaal01) bug 94883
**	    When varchar is reassigned with a shorter string, remains of 
**	    the previous string are still present which causes problems
**	    when casting to (char*), as this expects a NULL terminator.
**	    This happens in 4GL connect to: statement, resulting in 
**	    E_US0010, Database does not exist...	
**       7-mar-2001 (mosjo01)
**          Had to make MAX_CHAR more unique, IIMAX_CHAR_VALUE, due to conflict
**          between compat.h and system limits.h on (sqs_ptx).
**	7-feb-00 (inkdo01)
**	    Fill remainder of destination when source is short. This 
**	    clears garbage which may have been on database because of var
**	    type handling.
**	30-mar-01 (gupsh01)
**	    Added support for nchar and nvarchar data type. If the source is a 
**	    data type of type DB_NVCHR_TYPE, DB_NCHR_TYPE then this routine 
**	    should not be called, but call adu_moveunistring.
**	1-may-01 (inkdo01)
**	    Re-fix the fill logic from above. Change 449714 accidentally
**	    disabled it.
**	12-aug-02 (inkdo01)
**	    Replaced byte-at-a-time copy with MEcopy.
**	03-dec-2003 (gupsh01)
**	    Added support for UTF8 internal data type.
**	04-apr-2005 (gupsh01)
**	    Fixed the end point calculation for unicode types.
**  11-Apr-2005 (fanra01)
**      End of string pointer calculation for padding loop updated for Unicode
**      strings.
**	10-may-2007 (gupsh01)
**	    Fixed the code to handle UTF8 characterset strings.
**	    If the source is truncated make sure it is done at a whole 
**	    character boundary.
**	16-Apr-2008 (kschendel)
**	    Count up any-warnings too if warning.
**	14-Oct-2008 (gupsh01)
**	    Pass in the type of source string to adu_movestring. 
**      12-Oct-2010 (horda03) b124550
**          Padding strings 1 character at a time seriosly impacts performance
**          as the number of pad characters needed increases. From impirical
**          testing, >4 characters is faster using MEfill.
*/

DB_STATUS
adu_movestring(
ADF_CB			    *adf_scb,
register u_char		    *source,
register i4		    sourcelen,
DB_DT_ID		    src_type,
register DB_DATA_VALUE	    *dest)
{
    i4			char_warn_cnt = 0;
    i4			textnull_warn_cnt = 0;
    DB_STATUS           db_stat;
    char		*cptr;
    UCS2        	*ucptr;
    u_char		*endsource;
    UCS2		*enduni;
    register u_char     *outstring;
    register UCS2       *unistring;
    register i4		outlength;
    u_char		*endout;
    u_char              fill_character = ' ';
    UCS2                fill_ucharacter = U_BLANK;
    bool		byte_input = FALSE; 
    u_i2		si;
    size_t              pad_chars;

    if ( (dest->db_datatype == DB_NVCHR_TYPE) ||
         (dest->db_datatype == DB_NCHR_TYPE) )
    {
	
       if (db_stat = adu_7straddr(adf_scb, dest, &ucptr) != E_DB_OK)
           return (db_stat);
    }
    else
    {
        if (db_stat = adu_3straddr(adf_scb, dest, &cptr) != E_DB_OK)
           return (db_stat);
    }

    if ((abs(src_type) == DB_BYTE_TYPE) ||
        (abs(src_type) == DB_VBYTE_TYPE))
	byte_input = TRUE;

    switch (dest->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_BYTE_TYPE:
      case DB_DEC_TYPE:
      case DB_NCHR_TYPE:
        outlength = dest->db_length;
        break;

      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VCH_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NVCHR_TYPE:
      case DB_UTF8_TYPE:
        outlength = dest->db_length - DB_CNTSIZE;
        break;
    }

    /* 
    ** Return error when output string is too small and we are told
    ** to check it.
    */
    if( outlength < sourcelen 
      && adf_scb->adf_strtrunc_opt == ADF_ERR_STRTRUNC)
    {
	/* get rid of blanks for all types except BYTE */
	if ( (dest->db_datatype == DB_NVCHR_TYPE) ||
             (dest->db_datatype == DB_NCHR_TYPE) )
        {
            while (sourcelen--  &&  *(source + sourcelen) == U_BLANK)
                ;
            sourcelen++;
        }
	else if( dest->db_datatype != DB_VBYTE_TYPE && 
	       dest->db_datatype != DB_BYTE_TYPE)
	{
	   ADF_TRIMBLANKS_MACRO(source, sourcelen);
	}
	else
	{
	   /* for byte DT's, kill the null characters */
	   while (sourcelen--  &&  *(source + sourcelen) == '\0')
	    	;
	   sourcelen++;
	 }

        /*
        ** if it now fit, change sourcelen to outlength so that
        ** a varchar(5)='abc  ' will become varchar(4)='abc '
        */

         if( outlength >= sourcelen)
	    sourcelen = outlength;
	   else
	      return (adu_error(adf_scb, E_AD1082_STR_TRUNCATE, 2, sourcelen, 
			      (PTR)source));
    }

    endsource = source + sourcelen;

    if ( (dest->db_datatype == DB_NVCHR_TYPE) ||
       (dest->db_datatype == DB_NCHR_TYPE) )
    {
     unistring = (UCS2 *)ucptr;
     /*
     ** Divide outlength by sizeof(UCS2) to calculate end of output string.
     ** End of string should always be an even number of characters.
     ** Odd remainder is ignored.  If the compiler uses integer divide
     ** instructions they should be replaced with a right shift.
     */
     enduni    = unistring + (outlength / sizeof(UCS2));
    }
    else
    {
     outstring = (u_char *)cptr;
     endout    = outstring + outlength;
    }
    
    if (dest->db_datatype == DB_CHR_TYPE)
    {
        while (source < endsource  &&  outstring + CMbytecnt(source) <= endout)
        {
	    /*
	    ** The characters that are allowed as part of a string of type
	    ** c are the printable characters(alpha, numeric, 
	    ** punctuation), blanks, MAX_CHAR, or the pattern matching
	    ** characters.  NOTE: other whitespace (tab, etc.) is not
	    ** allowed.
	    */
            if (!CMprint(source) && (*source) != ' ' && (*source) != IIMAX_CHAR_VALUE
		&& (*source) != DB_PAT_ANY && (*source) != DB_PAT_ONE
		&& (*source) != DB_PAT_LBRAC && (*source) != DB_PAT_RBRAC)
            {
		char_warn_cnt++;
                *outstring++ = ' ';
		CMnext(source);
            }
	    else
	    {
		CMcpyinc(source, outstring);
	    }
        }
    }
    else if (dest->db_datatype == DB_TXT_TYPE)
    {
	while (source < endsource  &&  outstring + CMbytecnt(source) <= endout)
        {
	    /* NULLCHARs get cvt'ed to blank for text */
	    if (*source == NULLCHAR)
	    {
		textnull_warn_cnt++;
		source++;
		*outstring++ = ' ';
	    }
	    else
	    {
		CMcpyinc(source, outstring);
	    }
	}
	fill_character = '\0';
    }
    else if ((dest->db_datatype == DB_CHA_TYPE)
	  || (dest->db_datatype == DB_VCH_TYPE)
	  || (dest->db_datatype == DB_UTF8_TYPE)
	  || (dest->db_datatype == DB_LTXT_TYPE))
    {
	i4	cpylen = (sourcelen > outlength) ? outlength : sourcelen;

	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && !(byte_input))
	{
	  /* For UTF8 character set need to copy incremently so we truncate 
	  ** at a valid character */
	  while (source < endsource  &&  outstring + CMbytecnt(source) <= endout)
	    CMcpyinc(source, outstring);
	}
	else	
	{
	  MEcopy((PTR)source, cpylen, (PTR)outstring);
	  outstring += cpylen;
	}
	if (dest->db_datatype != DB_CHA_TYPE) fill_character = '\0';
    }
    else if ((dest->db_datatype == DB_NCHR_TYPE)
          || (dest->db_datatype == DB_NVCHR_TYPE))
    {
	/* 
        ** The valid input data types here are 
	** byte, byte varying, long byte, long text, 
        ** nchar, and nvarchar.
        **
        */	 
       i4  to_move =  min(sourcelen, outlength);

        MEcopy((PTR) source, to_move, (PTR) unistring);
        unistring += to_move/sizeof(UCS2);
        fill_ucharacter = U_BLANK;
	if (dest->db_datatype == DB_NVCHR_TYPE)
	    fill_ucharacter = U_NULL;
    }
    else /* Byte datatypes */
    {
	i4       to_move = min(sourcelen, outlength);
	/* Byte datatypes allow anything -- don't care about contents */

	MEcopy((PTR) source, to_move, (PTR) outstring);
	outstring += to_move;
        fill_character = '\0';
    }

    switch (dest->db_datatype)
    {
      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
      case DB_UTF8_TYPE:
	si = outstring - (u_char *)cptr;
	I2ASSIGN_MACRO(si, ((DB_TEXT_STRING *)dest->db_data)->db_t_count);
      /* Fall through and execute the loop to fill out the string. */

      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_BYTE_TYPE:
        if ( (pad_chars = (size_t)(endout - outstring)) <= 4)
        {
	   while (outstring < endout)
	    *outstring++ = fill_character;
        }
        else
           MEfill ( pad_chars, fill_character, outstring);
        break;

     case DB_NVCHR_TYPE: 
	si = unistring - (UCS2 *)ucptr;
        I2ASSIGN_MACRO(si, ((DB_NVCHR_STRING *)dest->db_data)->count);
      /* Fall through and execute the loop to fill out the string. */

     case DB_NCHR_TYPE:
        /* This too could benefit from a global substitution (like MEfill above)
        ** alas the fill_ucharacter is 2 bytes (UCS2) so MEfill can't be used, and
        ** wmemset sets a word at a time and memsetw is not universal.
        */
        while ( unistring < enduni )
            *unistring++ = fill_ucharacter;
        break;

    }
    
    if (char_warn_cnt || textnull_warn_cnt)
    {
	adf_scb->adf_warncb.ad_anywarn_cnt++;
	if (char_warn_cnt)
	    adf_scb->adf_warncb.ad_noprint_cnt++;
	if (textnull_warn_cnt)
	    adf_scb->adf_warncb.ad_textnull_cnt++;
    }

    return (E_DB_OK);
}

/*
** Name: adu_moveunistring() - moves a unicode string into a DB_DATA_VALUE.
**
** Description:
**        Moves a unicode string into a DB_DATA_VALUE padding the DB_DATA_VALUE
**      string appropriately. The call to this routine should be made when the 
**	input data type is of the type DB_NCHR_TYPE or DB_NVCHR_TYPE. 
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      source                          The source unicode string.
**      sourcelen                       The length of the source string.
**      dest                            Ptr to the destination DB_DATA_VALUE.
**          .db_datatype                Datatype of the DB_DATA_VALUE.
**          .db_length                  Length of the result DB_DATA_VALUE.
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
**      dest                            Ptr to the destination DB_DATA_VALUE.
**          .db_data                    Ptr to memory where string is moved to.
**      Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Routine was successful.
**          E_AD5001_BAD_STRING_TYPE    The destination DB_DATA_VALUE was an
**                                      incompatible datatype.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
**  History:
**      29-mar-2001 (gupsh01)
**          Created.
**	11-oct-2001 (abbjo03)
**	    Correct calculation of output string length.
**	23-jan-2007 (gupsh01)
**	    Add error checking for unsupported datatype.
**      12-Oct-2010 (horda03) b124550
**          Padding strings 1 character at a time seriosly impacts performance
**          as the number of pad characters needed increases. From impirical
**          testing, >4 characters is faster using MEfill.
*/
DB_STATUS
adu_moveunistring(
ADF_CB                      *adf_scb,
register UCS2             *source,
register i4         sourcelen,
register DB_DATA_VALUE      *dest)
{
    DB_STATUS         db_stat;

    UCS2              *ucptr;
    char              *cptr;	
    UCS2              *endsource;
    register u_char   *outstring;
    register UCS2     *unistring;
    register i4       outlength = 0;
    u_char            *endout;
    UCS2              *enduni;
    UCS2              fill_ucharacter = U_BLANK;
    u_char            fill_character = ' ';
    u_i2	      si;
    size_t            pad_chars;

    if ( (dest->db_datatype == DB_NVCHR_TYPE) ||
	 (dest->db_datatype == DB_NCHR_TYPE) )
    { 
       if (db_stat = adu_7straddr(adf_scb, dest, &ucptr) != E_DB_OK)
           return (db_stat);
    } 
    else 
    {
	if (db_stat = adu_3straddr(adf_scb, dest, &cptr) != E_DB_OK)
           return (db_stat);
    }

    switch (dest->db_datatype)
    {
      case DB_BYTE_TYPE:
      case DB_NCHR_TYPE:
        outlength = dest->db_length / sizeof(UCS2);
        break;

      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NVCHR_TYPE:
        outlength = (dest->db_length - DB_CNTSIZE) / sizeof(UCS2);
        break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    /*
    ** Return error when output string is too small and we are told
    ** to check it. try compressing the input and see if it fits.
    */
    if( outlength < sourcelen
      && adf_scb->adf_strtrunc_opt == ADF_ERR_STRTRUNC)
    { 
        /* get rid of blanks for all types except BYTE */
        if( dest->db_datatype != DB_VBYTE_TYPE &&
            dest->db_datatype != DB_BYTE_TYPE)
        {
            while (sourcelen--  &&  *(source + sourcelen) == U_BLANK)
                ;
            sourcelen++;
        }
        else
        {
            /* for byte DT's, kill the null characters */
            while (sourcelen--  &&  *(source + sourcelen) == U_NULL)
                ;
            sourcelen++;
        }

        /* if it now fit, change sourcelen to outlength. */

        if( outlength >= sourcelen)
            sourcelen = outlength;
        else
	/* can't fit the data in here */
            return (adu_error(adf_scb, E_AD1082_STR_TRUNCATE, 2, sourcelen,
                              (PTR)source));
    }

    endsource = source + sourcelen;

    if ( (dest->db_datatype == DB_NVCHR_TYPE) ||
       (dest->db_datatype == DB_NCHR_TYPE) )
    {
     unistring = (UCS2 *)ucptr;
     enduni    = unistring + outlength;
    }	
    else
    {
     outstring = (u_char *)cptr;
     endout    = outstring + outlength;
    }  

    if ((dest->db_datatype == DB_NCHR_TYPE)
          || (dest->db_datatype == DB_NVCHR_TYPE))
    {
        while (source < endsource  &&  unistring < enduni)
            *unistring++ = *source++;

        if ( unistring < enduni )
            *unistring = U_BLANK ;

        fill_ucharacter = U_BLANK;
    }
    else /* Byte datatypes */
    {
        i4       to_move = min(sourcelen, outlength);

        MEcopy((PTR) source, to_move, (PTR) outstring);
        outstring += to_move;
        fill_character = '\0';
    }

    switch (dest->db_datatype)
    {
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
	si = outstring - (u_char *)cptr;
        I2ASSIGN_MACRO(si, ((DB_TEXT_STRING *)dest->db_data)->db_t_count);
      break;

      case DB_NVCHR_TYPE:
	si = unistring - (UCS2 *)ucptr;
        I2ASSIGN_MACRO(si, ((DB_NVCHR_STRING *)dest->db_data)->count);
      break;

      case DB_BYTE_TYPE:
        if ( (pad_chars = (size_t)(endout - outstring)) <= 4)
        {
            while (outstring < endout)
                *outstring++ = fill_character;
        }
        else
           MEfill( pad_chars, fill_character, outstring);
        break;

      case DB_NCHR_TYPE:
        /* This too could benefit from a global substitution (like MEfill above)
        ** alas the fill_ucharacter is 2 bytes (UCS2) so MEfill can't be used, and
        ** wmemset sets a word at a time and memsetw is not universal.
        */
        while ( unistring < enduni )
            *unistring++ = fill_ucharacter;
        break;

    }
    return (E_DB_OK);
}


/*{
** Name: adu_size() - determine size of a string without trailing blanks.
**
** Description:
**        This routine determines the size of a character string without
**	trailing blanks.  For the c and char types, this is the length of the
**	string without trailing blanks.  For text, longtext, and varchar types,
**	this is the length of the string.
**
**	For unicode types, size is in number of unicode characters.
**
**      For the byte datatypes, trailing blank stuff is ignored.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv                              Ptr to DB_DATA_VALUE holding string.
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
**      size                            Ptr to a i4 to return the size in.
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
**	    E_AD5001_BAD_STRING_TYPE	The datatype of the DB_DATA_VALUE is
**					inappropriate for a string operation.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**  History:
**      02/22/83 (lichtman) -- changed to work with both character type
**                  and new text type.  size of a text
**                  string is actual size, not size w/o
**                  trailing blanks.
**      04/12/84 (lichtman) -- added TEXTNCOPY and TEXT0COPY types.  Made
**                  more efficient for CHAR case.  Fixed byte
**                  alignment for TEXT case.
**	25-apr-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Made work for char, varchar, and longtext.
**	07-jul-88 (jrb)
**	    Removed support for obsolete character types text0 and textn.
**	    Modified for Kanji support.
**      05-Apr-1993 (fred)
**          Added support for byte datatypes.
**      30-Sep-1993 (fred)
**          Fix byte datatypes so that just as blanks are trimmed from 
**          the character datatypes, null characters are trimmed from 
**          the byte datatypes.
**	29-March-2001 (gupsh01)
**	    Added support for the unicode data type nchar and nvarchar. 
**	05-Apr-2001 (gupsh01)
**	    Added support for long nvarchar.
**	12-Dec-2001 (gupsh01)
**	    Corrected length calculation for nchar and nvarchar types.
**	14-dec-2001 (devjo01)
**	    Have NVCHR case return # unicode chars like NCHR.    
**	05-Feb-2009 (kiria01) SIR 120473
**	    Handle DB_PAT_TYPE too.
*/

DB_STATUS
adu_size(
ADF_CB			*adf_scb,
register DB_DATA_VALUE	*dv,
i4			*size)
{
    register char       *c;
    i4                   i;
    u_i2		 si;
    UCS2		*uc;

    switch (dv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_BYTE_TYPE:
        i = dv->db_length;
	c = (char *)dv->db_data;
	/*
	** Step backwards thru chars until a non-blank is found.  This was
	** found to be more efficient in Ingres 3.0.  The following macro steps
	** backwards for single-byte but must go forward for double-byte.
	*/

	if (dv->db_datatype != DB_BYTE_TYPE)
	{
	    ADF_TRIMBLANKS_MACRO(c, i);
	}
	else
	{
	    while (i--  &&  *(c + i) == '\0')
		;
	    i++;
	}
        *size = i;
        break;

      case DB_NCHR_TYPE:
	i = dv->db_length / sizeof(UCS2);
	uc = (UCS2 *)dv->db_data;
        while ((i)--  &&  *((uc) + (i)) == U_BLANK)
                 ;
        (i)++;
        *size = i;
	break;
          	
      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
        I2ASSIGN_MACRO(((DB_TEXT_STRING *)dv->db_data)->db_t_count, si);
	*size = si;
        break;

      case DB_NVCHR_TYPE:
      case DB_PAT_TYPE:
         /*  modify the size as it is ELMSZ times count */	
         I2ASSIGN_MACRO(((DB_NVCHR_STRING *)dv->db_data)->count, si);
	 *size = si;
         break;
 
      case DB_LVCH_TYPE:
      case DB_LBYTE_TYPE:
      case DB_GEOM_TYPE:
      case DB_POINT_TYPE:
      case DB_MPOINT_TYPE:
      case DB_LINE_TYPE:
      case DB_MLINE_TYPE:
      case DB_POLY_TYPE:
      case DB_MPOLY_TYPE:
      case DB_GEOMC_TYPE:
      case DB_LNVCHR_TYPE:
      {
	    ADP_PERIPHERAL	    *periph = (ADP_PERIPHERAL *) dv->db_data;

	    I4ASSIGN_MACRO(periph->per_length1, *size);
	    break;
      }

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    return (E_DB_OK);
}


/*
** Name: adu_3straddr() - Returns the address of the string in a DB_DATA_VALUE,
**			  similar to lenaddr.
**
** Description:
**	  This routine returns the address of a string in a DB_DATA_VALUE.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv				Ptr to the DB_DATA_VALUE.
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
**	str_ptr				Ptr to character ptr to be updated
**					with address of DB_DATA_VALUE string.
**  Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	E_AD0000_OK			Routine completed successfully.
**	E_AD5001_BAD_STRING_TYPE	The DB_DATA_VALUE's datatype was
**					inappropriate for this routine.
**
**  History:
**      02/22/83 -- Written by Jeff Lichtman
**	7-apr-86 (ericj) -- modified for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Enhanced to work for CHAR, VARCHAR, and LONGTEXT.
**	07-jul-88 (jrb)
**	    Removed support for obsolete character types text0 and
**          textn.
**      05-Apr-1993 (fred)
**          Added byte datatypes to the supported set.
**      31-oct-1994 (tutto01)
**          Added case for decimal support.
**	03-dec-2003 (gupsh01)
**	    Added support for UTF8 internal data type.
*/


DB_STATUS
adu_3straddr(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv,
char		    **str_ptr)
{
    switch (dv->db_datatype)
    {
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
	case DB_DEC_TYPE:
            *str_ptr = (char *) dv->db_data;
            break;

        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
        case DB_LTXT_TYPE:
	case DB_VBYTE_TYPE:
        case DB_UTF8_TYPE:
            *str_ptr = (char *) ((DB_TEXT_STRING *) dv->db_data)->db_t_text;
            break;

        default:
	    return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
	    break;

    }

    return (E_DB_OK);
}

/*
** Name: adu_7straddr() - Returns the address of the unicode string in a 
**			  DB_DATA_VALUE, similar to lenaddr.
**
** Description:
**        This routine returns the address of a string in a DB_DATA_VALUE.
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      dv                              Ptr to the DB_DATA_VALUE.
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
**      str_ptr                         Ptr to UCS2 ptr to be updated
**                                      with address of DB_DATA_VALUE string.
**  Returns:
**            The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**            If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**      E_AD0000_OK                     Routine completed successfully.
**      E_AD5001_BAD_STRING_TYPE        The DB_DATA_VALUE's datatype was
**
**  History:
**      29-mar-2001 (gupsh01) 
**	    Created.	
**	05-Feb-2009 (kiria01) SIR 120473
**	    Handle DB_PAT_TYPE too.
*/

DB_STATUS
adu_7straddr(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv,
UCS2                **str_ptr)
{
    switch (dv->db_datatype)
    {
        case DB_NCHR_TYPE:
            *str_ptr = (UCS2 *) dv->db_data;
            break;

        case DB_NVCHR_TYPE:
	case DB_PAT_TYPE:
            *str_ptr = (UCS2 *) 
			((DB_NVCHR_STRING *) dv->db_data)->element_array;
            break;

        default:
	    /* this is an illegal call to this function. */
            return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
            break;

    }

    return (E_DB_OK);
}


/*
** Name: adu_5strcount() - Return the length of a string.
**
** Description:
**	  For the the c and char types the length is the declared length.  For
**	the text, longtext, and varchar types the length is the actual length
**	stored in the string.
**
**      For byte strings, the semantics for byte and byte varying
**      follow char and varchar respectively.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv				Pointer to the DB_DATA_VALUE
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
**	len_ptr				Address of an i2 to hold length of
**					DB_DATA_VALUE's string length.
**  Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**      E_AD0000_OK			Routine completed successfully.
**	E_AD5001_BAD_STRING_TYPE	The DB_DATA_VALUE's string was 
**					inappropriate for this routine.
**
**  History:
**      03/04/83 (lichtman) -- written
**	7-apr-86 (ericj) -- modified for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-sep-86 (thurston)
**	    Made work for char, varchar, and longtext.
**	07-jul-88 (jrb)
**	    Removed support for obsolete character types text0 and textn.
**	07-nov-88 (thurston)
**	    Added check for count field in DB_TEXT_STRING structure to assure
**	    that the count is not totally hosed up.
**      05-Apr-1993 (fred)
**          Added support for byte datatypes.
**      04-Apr-2001 (gupsh01)
**          Added support for unicode datatype.
**	13-dec-2001 (devjo01)
**	    Make case for NCHR consistent with NVCHR case by
**          returning number of unicode chars, not raw byte size.
**	03-dec-2003 (gupsh01)
**	    Added support for UTF8 internal data type.
**	01-Oct-2008 (gupsh01)
**	    For UTF8 installations add a check to ensure 
**	    that the length value does not exceed maxstring/2. 
*/

DB_STATUS
adu_5strcount(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv,
i4		    *str_len)
{
    u_i2 si; 
    switch (dv->db_datatype)
    {
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
            *str_len = dv->db_length;
            break;

	case DB_NCHR_TYPE:
            *str_len = dv->db_length / sizeof(UCS2);
            break;

        case DB_LTXT_TYPE:
	case DB_VBYTE_TYPE:
	case DB_UTF8_TYPE:
            I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, si);
	    if (si > adf_scb->adf_maxstring)
		return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
            *str_len = si;
            break;

        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
            I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, si);
	    if (((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
		 dv->db_collID != DB_UCS_BASIC_COLL &&
		 (si > adf_scb->adf_maxstring/2)) ||
		(si > adf_scb->adf_maxstring))
		return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
            *str_len = si;
            break;

	case DB_NVCHR_TYPE:
            I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, si);
            if (si > adf_scb->adf_maxstring)
                return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	    *str_len = si;
            break;

        case DB_LVCH_TYPE:
	case DB_LBYTE_TYPE:
	case DB_GEOM_TYPE:
        case DB_POINT_TYPE:
        case DB_MPOINT_TYPE:
        case DB_LINE_TYPE:
        case DB_MLINE_TYPE:
        case DB_POLY_TYPE:
        case DB_MPOLY_TYPE:
        case DB_GEOMC_TYPE:
	case DB_LNVCHR_TYPE:
        {
	    ADP_PERIPHERAL	    *periph = (ADP_PERIPHERAL *) dv->db_data;

	    I4ASSIGN_MACRO(periph->per_length1, *str_len);
	    break;
        }


        default:
	    return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }
    return (E_DB_OK);
}
