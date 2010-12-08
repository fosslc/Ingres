/*
** Copyright (c) 1986, 2008, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <cv.h>
#include    <cm.h>
#include    <sl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adumoney.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adulcol.h>
#include    <aduucol.h>
#include    "adustrcmp.h"

/**
**
** Name: ADUSTRCMP.C - Routines for implementing lexical comparison on
**		       string datatypes (c, char, varchar, text).
**
** This file includes the following externally visible routines:
**
**	adu_ccmp() -    Compares two "c" data values.  No pattern matching
**			is allowed.  Blanks are ignored.  Shorter strings
**                      are considered less than longer strings.
**	adu_textcmp() - Compares two "text" data values.  No pattern matching
**			is allowed.  Blanks are significant.  Shorter strings
**                      are considered less than longer strings.
**	adu_varcharcmp() - Compares two string data values.  The data values can
**                      be "c", "text", "char", or "varchar", but the semantics
**                      used for the comparison will be those for "char"/
**                      "varchar".  That is, no pattern matching is allowed.
**                      Blanks are significant.  The shorter string is
**                      effectively padded with blanks to the length of the
**                      longer string.
**	adu_lexcomp() - Compares two string data values for equality.  QUEL
**			pattern matching characters are allowed (DB_PAT_ANY,
**			DB_PAT_ONE, DB_PAT_LBRAC, DB_PAT_RBRAC), if the language
**			is QUEL.  If either of the string data values is a "c",
**			then blanks and null chars will be ignored.  If no
**			pattern match characters, "char" and "varchar" values
**			will effectively be blank padded to the length of the
**			longer.
**	adu_like() -	Compares two string data values for equality.  SQL's
**			"LIKE" pattern matching characters are allowed in the
**			second data value ('%' matches any number of characters,
**			including none; '_' matches exactly one character).
**			If the first string data values is a "c", then blanks
**			and null chars will be ignored.  Also, the ESCAPE
**			character is supported.  If the '%' or '_' characters
**			are escaped, they loose their special pattern match
**			meaning, while the '[' and ']' characters, if escaped,
**			take on the QUEL-like range pattern match meaning.  This
**			latter feature is an upward compatible extension to the
**			ANSI SQL LIKE predicate.
**	adu_collweight - Produces a collation weight for a non-Unicode 
**			string value.
**
** This file also includes the following static routines:
**
**      ad0_1lexc_pm() - Recursive routine used by adu_lexcomp() to compare
**		         two strings using QUEL pattern matching semantics.
**	ad0_qmatch() -	 Process a `MATCH-ONE' character for QUEL.
**	ad0_pmatch() -	 Process a `MATCH-ANY' character for QUEL.
**	ad0_lmatch() -	 Process a `range sequence' for QUEL.
**	ad0_pmchk() -	 Process blank padding for non-LIKE string compares.
**      ad0_2lexc_nopm() - Routine used by adu_lexcomp() to compare two strings
**		           without using QUEL pattern matching semantics.
**	ad0_like() -     Recursive routine used by adu_like() to compare
**		         two strings using SQL LIKE pattern matching semantics.
**	ad0_lkqmatch() - Process a `MATCH-ONE' character for LIKE.
**	ad0_lkpmatch() - Process a `MATCH-ANY' character for LIKE.
**	ad0_lklmatch() - Process a `range sequence' for LIKE.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      29-apr-86 (ericj)    
**          Converted for Jupiter.  Taken originally from 4.0/02 code
**	    and file string.c.
**	19-jun-86 (ericj)
**	    Added adu_ccmp() routine and changed adu_textcmp() parameters
**	    to be compatible with adc_compare() external interface.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	01-oct-86 (thurston)
**	    Changed the name of adu_charcmp() to adu_ccmp().  Also added the
**	    routines adu_varcharcmp() and adu_like() (with its ad0_like() and
**	    ad0_lkmatch() static routines).
**	20-nov-86 (thurston)
**	    Made routine adu_lexcomp() sensitive to query language, so that for
**	    QUEL, pattern matching characters are treated as such, but for SQL
**	    they are interpretted litterally.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	08-mar-87 (thurston)
**	    Fixed string comparison bugs that caused pattern matching to not
**	    be used when CHAR or VARCHAR were involved ... even though the query
**	    language was QUEL.
**	21-apr-87 (thurston)
**	    Added `use_quel_pm' arg to adu_lexcomp() routine.
**	13-aug-87 (thurston)
**	    Fixed bug in ad0_lmatch() for the following compare:
**
**		"[ ab]c" = "  c      "
**		\______/   \_________/
**	           |            |
**		  text          c
**
**	    In this type of compare, blanks and null chars are to be ignored,
**	    so the compare should return TRUE.  It was not.  It does now.
**
**	    ALSO ...
**
**	    Added code to ad0_1lexc_pm() to allow a range with a blank or null
**	    char specifically in it to be skipped if blanks and null chars are
**	    to be ignored.
**	14-sep-87 (thurston)
**	    Fixed bug in ad0_like() that caused a '_' at the end of the pattern
**          to match TWO characters. 
**	19-nov-87 (thurston)
**          A couple of bugs were fixed in adu_like() and its subordinates, as
**          well as a big semantic change:  No blank padding will ever be done
**          now, even if CHAR or VARCHAR are involved. After reading the ANSI
**          SQL standard, February 1986, for the <like predicate> (section 5.14,
**          pages 40-41) for the millionth time, I think I finally understand
**          it, and it means that no blank padding should ever be done.  This
**          was also tested out in DB2, which agrees. Therefore, 
**			    'abc' = 'abc '	... is TRUE
**	    while,
**			    'abc' LIKE 'abc '	... is FALSE
**	20-nov-87 (thurston)
**	    Major changes to the QUEL PM code in order to avoid doing blank
**	    padding when pattern match characters are present (even when `char'
**	    or `varchar' are involved).  This is consistent with the LIKE
**	    operator in SQL, and makes a lot of sense.  A couple of other minor
**	    bugs came out in the wash, also; e.g. "ab" now matches "ab[]".
**	    These changes included modifications to the static routines,
**	    ad0_1lexc_pm(), ad0_pmatch(), and ad0_lmatch, as well as the
**	    creation of two new static routines, ad0_qmatch() and ad0_pmchk().
**	06-jul-88 (thurston)
**	    Added support for the escape character to the LIKE processing
**	    routines, along with the new `[ ... ]' type pattern matching
**	    allowed by the INGRES version of the LIKE predicate.  This involved
**	    re-writing the entire algorithm, including the new static routines
**	    ad0_lkqmatch(), ad0_lkpmatch() (replaces the former ad0_lkmatch()),
**	    and ad0_lklmatch().
**	13-jul-88 (jrb)
**	    Pervasive changes for Kanji support.
**	24-aug-88 (thurston)
**	    Commented out a `"should *NEVER* get here" return with error' at the
**	    end of ad0_like() and another at the end of ad0_lklmatch(), since
**	    the Sun compiler produced a warning that you could never get there.
**	24-Apr-89 (anton)
**	    Added local collation support
**	    Also corrected a QUEL pattern match problem with '<' and '>'
**	27-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	09-aug-89 (jrb)
**	    Fixed bug where two strings of unequal length might compare as
**	    unequal when they should compare as equal.
**	08-aug-91 (seg)
**	    Can't do arithmetic or dereferencing on PTR types.
**      05-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-AUg-93 (shailaja)
**	    Unnested comments.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	14-mar-95 (peeje01)
**	    cross integration from DBCS line 6500db_su4_us42
**	20-Jul-95 (rcs)
**	    Fixed bug where the endp1 and endp2 could be pointing
**	    to the end of the wrong data area, which caused the
**	    CMcmpcaselen call to be wrong for doublebyte.
**	11-jun-1993 (prida01)
**	    Create new function to do long varchar compares.
**	09-sep-1998 (kinte01)
**	    Add include of me.h to pick up prototypes for MEcopy. Previous
**	    integration breaks VMS builds.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-mar-2001 (somsa01)
**	    In adu_lvarcharcmp(), since blob segments cannot be greater
**	    than 2K at the moment, use 4096 (the old value of
**	    DB_GW4_MAXSTRING) for now.
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**	01-Feb-2007 (kiria01) b117544
**	    Rework ad0_lkpmatch and ad0_lkqmatch to effectivly coalesce runs
**	    of MATCH-ONE and MATCH_ANY chars into equivalent patterns that
**	    avoid unnecessary cpu cycles. Also change ad0_1lexc_pm to allow for
**	    a similar effect but by optimising the patterns up front.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. Don't allow UTF8 strings with it
**	    to use UCS2 CEs for comparison related actions.
**      02-Dec-2010 (gupsh01) SIR 124685
**          Prototype cleanup.
**/


/*
**  Defines of other constants.
*/

#define		AD_BLANK_CHAR	    ((u_char) ' ')  /* A blank */
#define		AD_DASH_CHAR 	    ((u_char) '-')  /* A dash */


/*
**  Definition of static variables and forward static functions.
*/
static i4  ad0_1lexc_pm(u_char  *str1,
			u_char  *endstr1,
			u_char  *str2,
			u_char  *endstr2,
			bool    bpad,
			bool    bignore);

static i4  ad0_1lexc_pm1(u_char  *str1,
			u_char  *endstr1,
			u_char  *str2,
			u_char  *endstr2,
			bool    bpad,
			bool    bignore);

static i4  ad0_qmatch(u_char  *pat,	    
		      u_char  *endpat,    
		      u_char  *str,	    
		      u_char  *endstr,    
		      bool    bignore);

static i4  ad0_pmatch(u_char  *pat,	    
		      u_char  *endpat,    
		      u_char  *str,	    
		      u_char  *endstr,    
		      bool    bignore);

static i4  ad0_lmatch(u_char  *pat,	    
		      u_char  *endpat,    
		      u_char  *str,	    
		      u_char  *endstr,    
		      bool    bignore);

static bool ad0_pmchk(u_char  *str,
		      u_char  *endstr);

static i4  ad0_2lexc_nopm(u_char  *str1,
			  u_char  *endstr1,
			  u_char  *str2,
			  u_char  *endstr2,
			  bool    bpad,
			  bool    bignore);

static DB_STATUS ad0_like(ADF_CB  *adf_scb,
			  u_char  *sptr,
			  u_char  *ends,
			  u_char  *pptr,
			  u_char  *endp,
			  u_char  *eptr,
			  bool    bignore,
			  i4      *rcmp);

static DB_STATUS ad0_lkqmatch(ADF_CB  *adf_scb,
			      u_char  *sptr,
			      u_char  *ends,
			      u_char  *pptr,
			      u_char  *endp,
			      u_char  *eptr,
			      bool    bignore,
			      i4      n,
			      i4      *rcmp);

static DB_STATUS ad0_lkpmatch(ADF_CB  *adf_scb,
			      u_char  *sptr,
			      u_char  *ends,
			      u_char  *pptr,
			      u_char  *endp,
			      u_char  *eptr,
			      bool    bignore,
			      i4      n,
			      i4      *rcmp);

static DB_STATUS ad0_lklmatch(ADF_CB  *adf_scb,
			      u_char  *sptr,
			      u_char  *ends,
			      u_char  *pptr,
			      u_char  *endp,
			      u_char  *eptr,
			      bool    bignore,
			      i4      *rcmp);

static i4 cmicmpcaselen( char *str1,
    			 char *endstr1,
    			 char *str2,
    			 char *endstr2);

/* integrated from 6500db_su4_us42 14-mar-95 peeje01 */
/* special hack for Bug #54559 - the BAAN bug (kirke) */

# ifdef DOUBLEBYTE
# define CMcmpcaselen(str1, endstr1, str2, endstr2) ((CMGETDBL)? cmicmpcaselen(str1, endstr1, str2, endstr2) : ((i4)(CM_DEREF(str1)) - (i4)(CM_DEREF(str2))))
# else
/* same as CMcmpcase() */
# define CMcmpcaselen(str1, endstr1, str2, endstr2) CMcmpcase(str1, str2)
# endif
# define CMcmpcaselen_SB(str1, endstr1, str2, endstr2) CMcmpcase_SB(str1, str2)

static i4
cmicmpcaselen(
char *str1,
char *endstr1,
char *str2,
char *endstr2)
{
    int result;

    if ((str1 + CMbytecnt(str1) > endstr1) || (str2 + CMbytecnt(str2) > endstr2))
    {
	/* either str1 or str2 is a partial character */
	/* compare the first byte */
	if ((result = (i4)(*(str1)&0377) - (i4)(*(str2)&0377)) == 0)
	{
	    /* first byte is equal */
	    if ((str1 + CMbytecnt(str1) > endstr1) && (str2 + CMbytecnt(str2) > endstr2))
		/* both str1 and str2 are partial characters */
		return(0);
	    else
		if (str1 + CMbytecnt(str1) > endstr1)
		    /* str1 contains partial char,
		    str2 contains full char */
		    return(-1);
		else
		    /* str2 contains partial char,
		    str1 contains full char */
		    return(1);
	}
	else
	{
	    /* first byte not equal */
	    return(result);
	}
    }
    else
    {
	/* both str1 and str2 are full characters */
	return(CMcmpcase(str1, str2));
    }
} /* cmicmpcaselen() */




/*{
** Name: adu_ccmp() - Compares two "c" data values for equality.
**
** Description:
**	Compares two "c" data values.  No pattern matching
**	is allowed.  Blanks are ignored.  Shorter strings
**      are considered less than longer strings.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      cdv1				DB_DATA_VALUE describing one of the
**					"c" data value to be compared.  Its
**					datatype is assumed to be "c".
**	    .db_length			Its length.
**	    .db_data			Ptr to "c" string to be compared.
**      cdv2				DB_DATA_VALUE describing the other
**					"c" data value to be compared.  Its
**					datatype is assumed to be "c".
**	    .db_length			Its length.
**	    .db_data			Ptr to "c" string to be compared.
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
**	rcmp			    	Result of comparison, as follows:
**					    <0  if  cdv1 < cdv2
**					    >0  if  cdv1 > cdv2
**					    =0  if  cdv1 = cdv2
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
**
** History:
**	19-jun-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	01-oct-86 (thurston)
**	    Changed the name of this routine from adu_charcmp() to adu_ccmp().
**	24-Apr-89 (anton)
**	    Added local collation support
**	27-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	08-aug-91 (seg)
**	    Can't do arithmetic or dereferencing on PTR types.
*/

DB_STATUS
adu_ccmp(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *cdv1,
DB_DATA_VALUE	    *cdv2,
i4		    *rcmp)
{
    if (adf_scb->adf_collation)
    {
	*rcmp = adugcmp((ADULTABLE *)adf_scb->adf_collation, ADUL_SKIPBLANK,
			(u_char *)cdv1->db_data,
			(u_char *)cdv1->db_data + cdv1->db_length,
		        (u_char *)cdv2->db_data,
			(u_char *)cdv2->db_data + cdv2->db_length);
    }
    else
    {
	*rcmp = STscompare(cdv1->db_data, cdv1->db_length,
			   cdv2->db_data, cdv2->db_length);
	
    }
    return (E_DB_OK);
}


/*{
** Name: adu_textcmp() - Compares two "text" data values for equality.
**
** Description:
**	Compares two "text" data values.  No pattern matching
**	is allowed.  Blanks are significant.  Shorter strings
**      are considered less than longer strings.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	tdv1				DB_DATA_VALUE describing first text
**					string to be compared.
**	    .db_data                    Ptr to DB_TEXT_STRING containing 
**					the first text strings.
**		.db_t_count		Number of characters in this text
**					string.
**		.db_t_text		Ptr to the first text string to be
**					compared.
**	tdv2				DB_DATA_VALUE describing the second
**					text string to be compared.
**	    .db_data			Ptr to DB_TEXT_STRING containing the
**					second text string.
**		.db_t_count		Number of characters in this text
**					string.
**		.db_t_text		Ptr to the second text string to be
**					compared.
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
**	rcmp			    	Result of comparison, as follows:
**					    <0  if  tdv1 < tdv2
**					    >0  if  tdv1 > tdv2
**					    =0  if  tdv1 = tdv2
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
**
**  History:
**      03/14/83 (lichtman) -- written
**	03-jun-86 (ericj)
**	    Converted for Jupiter.
**	21-jun-86 (ericj)
**	    Changed interface to conform to adc_compare's interface.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	13-jul-88 (jrb)
**	    Modified for Kanji support.
**	24-Apr-89 (anton)
**	    Added local collation support.
**	27-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**      13-Jan-2010 (wanfr01) Bug 123139
**          Optimizations for single byte
*/

DB_STATUS
adu_textcmp(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *tdv1,
DB_DATA_VALUE   *tdv2,
i4		*rcmp)
{
    register i4	i;
    register u_char	*p1,
			*p2;
    u_char		*endp1,
			*endp2;


    p1 = (u_char *) ((DB_TEXT_STRING *) tdv1->db_data)->db_t_text;
    p2 = (u_char *) ((DB_TEXT_STRING *) tdv2->db_data)->db_t_text;

    endp1 = p1 + ((DB_TEXT_STRING *) tdv1->db_data)->db_t_count;
    endp2 = p2 + ((DB_TEXT_STRING *) tdv2->db_data)->db_t_count;

    if (adf_scb->adf_collation)
    {
	*rcmp = adugcmp((ADULTABLE *)adf_scb->adf_collation, 0,
		       p1, endp1, p2, endp2);
    }
    else
    {
	if (CMGETDBL)
	{
	while (p1 < endp1  &&  p2 < endp2)
	{
	    if (i = CMcmpcaselen(p1, endp1, p2, endp2))
	    {
		*rcmp = i;
		return (E_DB_OK);
	    }
	    CMnext(p1);
	    CMnext(p2);
	}
	}
	else
	{
	while (p1 < endp1  &&  p2 < endp2)
	{
	    if (i = CMcmpcaselen_SB(p1, endp1, p2, endp2))
	    {
		*rcmp = i;
		return (E_DB_OK);
	    }
	    CMnext_SB(p1);
	    CMnext_SB(p2);
	}
	}
	
	*rcmp = (  ((DB_TEXT_STRING *)tdv1->db_data)->db_t_count
		 - ((DB_TEXT_STRING *)tdv2->db_data)->db_t_count);
    }
    return (E_DB_OK);
}


/*{
** Name: adu_varcharcmp() - Compare two string data values for equality using
**			    the "char/varchar" semantics.
**
** Description:
**      Compares two string data values.  The data values can be "c",
**      "text", "char", or "varchar", but the semantics used for the
**      comparison will be those for "char/varchar".  That is, no
**      pattern matching is allowed.  Blanks are significant.  The
**      shorter string is effectively padded with blanks to the length
**      of the longer string.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	vcdv1				Ptr DB_DATA_VALUE describing first
**					"varchar" string to be compared.  Its
**					datatype is assumed to be "varchar".
**	    .db_data                    Ptr to DB_TEXT_STRING containing 
**					this "varchar" string.
**		.db_t_count		Number of characters in this "varchar"
**					string.
**		.db_t_text		Ptr to the actual characters.
**	vcdv2				Ptr DB_DATA_VALUE describing second
**					"varchar" string to be compared.  Its
**					datatype is assumed to be "varchar".
**	    .db_data                    Ptr to DB_TEXT_STRING containing 
**					this "varchar" string.
**		.db_t_count		Number of characters in this "varchar"
**					string.
**		.db_t_text		Ptr to the actual characters.
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
**	rcmp			    	Result of comparison, as follows:
**					    <0  if  vcdv1 < vcdv2
**					    >0  if  vcdv1 > vcdv2
**					    =0  if  vcdv1 = vcdv2
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
** History:
**	01-oct-86 (thurston)
**          Initial creation.
**	13-jul-88 (jrb)
**	    Modified for Kanji support.
**	24-Apr-89 (anton)
**	    Added local collation support.
**	27-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	09-aug-89 (jrb)
**	    Fixed bug where two strings of unequal length might compare as
**	    unequal when they should compare as equal.
**	20-Jul-95 (rcs)
**	    Fixed bug where the endp1 and endp2 could be pointing
**	    to the end of the wrong data area, which caused the
**	    CMcmpcaselen call to be wrong for doublebyte. blank
**	    extended to allow valid pointer to end of string.
**	3-may-2007 (dougi)
**	    Add code to do MEcmp() on char/varchar to speed it up.
**	13-jun-2007 (toumi01)
**	    Fix return value equality check for MEcmp().
*/

DB_STATUS
adu_varcharcmp(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *vcdv1,
DB_DATA_VALUE	    *vcdv2,
i4		    *rcmp)
{
    DB_STATUS           db_stat;
    i4			len1;
    i4			len2;
    i4			diff;
    register i4	stat;
    u_char		blank[2] ={AD_BLANK_CHAR,NULLCHAR};
    register u_char	*p1;
    u_char		*p1_temp;   /* Needed to send by address */
    u_char		*endp1;
    u_char		*l1;
    register u_char	*p2;
    u_char		*p2_temp;   /* Needed to send by address */
    u_char		*endp2;
    u_char		*l2;



    if (db_stat = adu_lenaddr(adf_scb, vcdv1, &len1, (char **) &p1_temp))
	return (db_stat);
    p1    = p1_temp;
    endp1 = p1 + len1;
    
    if (db_stat = adu_lenaddr(adf_scb, vcdv2, &len2, (char **) &p2_temp))
	return (db_stat);
    p2    = p2_temp;
    endp2 = p2 + len2;

    if (adf_scb->adf_collation)
    {
	*rcmp = adugcmp((ADULTABLE *)adf_scb->adf_collation, ADUL_BLANKPAD,
		       p1, endp1, p2, endp2);
    }
    else if (Adf_globs->Adi_status & ADI_DBLBYTE)
    {
	/* CM stuff is done for double byte. */
	for (;;)
	{
	    if (p1 < endp1)
	    {
		l1 = p1;
		CMnext(p1);
	    }
	    else
	      {
		l1 = &blank[0];
		p1 = l1;
		endp1 = &blank[1];
	    }

	    if (p2 < endp2)
	    {
		l2 = p2;
		CMnext(p2);
	    }
	    else
	    {
		l2 = &blank[0];
		p2 = l2;
		endp2 = &blank[1];
	    }

	    /* if both pointing to blank or we happen to be comparing a string
	    ** to itself then break
	    */
	    if (l1 == l2)
		break;

	    if ((stat = CMcmpcaselen(l1, endp1, l2, endp2)) != 0)
	    {
		*rcmp = stat;
		return(E_DB_OK);
	    }
	}
	
	*rcmp = 0;	    /* They are equal to the length of the longer */
    }
    else
    {
	/* Otherwise, try the fast path - MEcmp the prefixes, then check
	** for blank termination of the longer string, if necessary. */
	diff = len1 - len2;
	if (diff > 0)
	    *rcmp = MEcmp(p1, p2, len2);
	else *rcmp = MEcmp(p1, p2, len1);

	if (!(*rcmp) && diff)	/* if prefixes equal and lengths differ */
	{
	    /* If lengths are different and prefixes are equal ... */
	    if (diff > 0)
	    {
		/* p1 is longer - check for trailing blanks. */
		p1 += len2;
		p2 = p1 + diff;		/* loop terminator */
		while (p1 < p2 && *p1 == ' ')
		    p1++;

		if (p1 < p2)
		    *rcmp = (*p1 > ' ') ? 1 : -1;
	    }
	    else
	    {
		/* p2 is longer - check for trailing blanks. */
		p2 += len1;
		p1 = p2 - diff;		/* loop terminator */
		while (p1 > p2 && *p2 == ' ')
		    p2++;

		if (p1 > p2)
		    *rcmp = (*p2 > ' ') ? -1 : 1;
	    }
	}
    }

    return (E_DB_OK);
}


/*{
** Name: adu_lvarcharcmp() - Compare two string data values for equality using
**                          the "lvarchar" semantics.
**
** Description:
**      Compares two string data values of long varchar types.
**	Blanks are significant. The function basically gets each segment of two
**	long var chars and compares them against each other. When a difference is
**	found it is returned immediately without checking all the other segments.
**	If no difference is found we could checks a couple of gigabytes of data.
**	I wouldn't call this function too regularly cause it will use up quite a
**	bit of time.
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      vcdv1                           Ptr DB_DATA_VALUE describing first
**                                      "varchar" string to be compared.  Its
**                                      datatype is assumed to be "varchar".
**          .db_data                    Ptr to DB_TEXT_STRING containing
**                                      this "varchar" string.
**              .db_t_count             Number of characters in this "varchar"
**                                      string.
**              .db_t_text              Ptr to the actual characters.
**      vcdv2                           Ptr DB_DATA_VALUE describing second
**                                      "varchar" string to be compared.  Its
**                                      datatype is assumed to be "varchar".
**          .db_data                    Ptr to DB_TEXT_STRING containing
**                                      this "varchar" string.
**              .db_t_count             Number of characters in this "varchar"
**                                      string.
**              .db_t_text              Ptr to the actual characters.
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
**          The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**          If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Completed successfully.
**
** History:
**	11-jun-1996  prida01
**	    Function given birth.
**      09-Sep-1998  wanya01 thaju02 islsa01
**          Bug #93134 - Server crashes after inserting duplicate rows 
**          into a table with a blob column and noduplicates.     
**	19-mar-2001 (somsa01)
**	    Since blob segments cannot be greater than 2K at the moment,
**	    use 4096 (the old value of DB_GW4_MAXSTRING) for now.
**	11-May-2004 (schka24)
**	    Name change for temp indicator.
**      04-Feb-2008 (coomi01) b120493
**          ADP_C_END_MASK and ADP_C_BEGIN_MASK should not be set 
**          simultaneously, this leads to a SEGV. So delay setting
**          ADP_C_END_MASK until ADP_C_BEGIN_MASK drops out after
**          first fetch.
**      25-Mar-2009 (hanal04 & Karl Schendel) Bug 121754
**          Zero length coupons will return a failure from dmpe_get().
**          Trap and handle the zero length case.
**          length can not be set until we have retrieved the segments
**          move the segment retrieval to the start of the loop.
**          Add checks for loop exit conditions immediately after the GET
**          operations to avoid comparing segments that were not fetched.
**       3-Apr-2009 (hanal04) Bug 121754
**          If inp1_cpn and/or inp2_cpn are not byte aligned then copy
**          the content to a local buffer to avoid SIGBUS on 64-bit platforms.
**       12-Nov-2009 (coomi01) Bug 122840
**          To Share code, now redirect to common blob comparison routine.        
*/

DB_STATUS
adu_lvarcharcmp(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *vcdv1,
DB_DATA_VALUE       *vcdv2,
i4                  *rcmp)
{
    return  adu_longcmp(adf_scb,vcdv1,vcdv2,rcmp);
}


/*{
** Name: adu_lexcomp() - Compare two strings data values, with optional pattern
**			 matching.
**
** Description:
**      adu_lexcomp() performs string comparisons between the two string data
**      values dv1 and dv2.  Blanks and null chars are ignored in both strings
**      if one of them is a "c" data value.  In addition QUEL pattern matching
**      is performed if requested (via the `use_quel_pm' arg) using a syntax
**      similar to Unix's "shell syntax".  Pattern matching characters are
**      converted to the pattern matching symbols DB_PAT_ANY etc. by the
**      scanner, prior to this routine. Pattern matching characters can appear
**      in either or both strings.  Since they cannot be stored in relations
**      (Editor's note: "Bull!"  - Gene Thurston), pattern matching chars in
**      both strings can only happen if the user types in such an expression. 
**
**	NOTE:  Pattern matching character will only be treated as such if the
**	       `use_quel_pm argument is TRUE.  Otherwise, these characters will
**	       be interpretted literally.
**
**	Examples:
**
**	       (c)"Smith, Homer" = "Smith,Homer"(c)
**
**	    (text)"Smith, Homer" < "Smith,Homer"(text)
**
**	               (c)"abcd" < "abcdd"(c)
**
**	            (text)"abcd" = "aPAT_ANYd"(text)	(QUEL)
**	            (text)"abcd" < "aPAT_ANYd"(text)	(SQL)
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	use_quel_pm			If TRUE, do QUEL pattern matching.  If
**					FALSE, do not.
**      dv1                             Ptr to DB_DATA_VALUE containing first
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 1st string (for "c" and "char")
**					or DB_TEXT_STRING holding first string
**					(for "text" and "varchar").
**	dv2				Ptr to DB_DATA_VALUE containing second
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 2nd string (for "c" and "char")
**					or DB_TEXT_STRING holding first string
**					(for "text" and "varchar").
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
**	rcmp				Result of comparison, as follows:
**					    <0	if  dv1 < dv2
**					    =0	if  dv1 = dv2
**					    >0	if  dv1 > dv2
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
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or dv2's datatype was
**					incompatible with this operation.
**
**  History:
**	07-apr-86 (ericj)
**	    Modified for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-nov-86 (thurston)
**	    Made routine sensitive to query language, so that for QUEL, pattern
**	    matching characters are treated as such, but for SQL they are
**	    interpretted litterally.
**	21-apr-87 (thurston)
**	    Added `use_quel_pm' arg.
**	24-Apr-89 (anton)
**	    Added local collation support
**	27-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	23-mar-2001 (stephenb)
**	    Add support for nchar and nvarchar
**	16-dec-04 (inkdo01)
**	    Add collation ID parm to aduucmp() call.
**	8-may-2007 (dougi)
**	    Redirect comparisons of c/text/char/varchar data in UTF8 server.
**	05-nov-2007 (gupsh01)
**	    Pass in the quel pattern matching flag to adu_nvchr_utf8comp.
**	25-nov-2009 (kiria01, wanfr01) b122960
**	    Don't need to use ad0_1lexc_pm - pattern matching consolidation
**	    is now done in adi_pm_encode
*/

DB_STATUS
adu_lexcomp(
ADF_CB          *adf_scb,
i4		use_quel_pm,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
i4		*rcmp)
{
    DB_STATUS   db_stat;
    bool	blank_pad;
    bool	ignore_blanks;
    u_char	*p1;
    u_char	*endp1;
    u_char	*p2;
    u_char	*endp2;
    i4		size1;
    i4		size2;
    ADULcstate	s1, s2;

    /* Skim off c/text/char/varchar comparisons in UTF* server unless UCS_BASIC. */
    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED &&
	dv1->db_collID != DB_UCS_BASIC_COLL && dv2->db_collID != DB_UCS_BASIC_COLL &&
	(dv1->db_datatype == DB_VCH_TYPE || dv1->db_datatype == DB_CHA_TYPE ||
	 dv1->db_datatype == DB_TXT_TYPE || dv1->db_datatype == DB_CHR_TYPE) &&
	(dv2->db_datatype == DB_VCH_TYPE || dv2->db_datatype == DB_CHA_TYPE ||
	 dv2->db_datatype == DB_TXT_TYPE || dv2->db_datatype == DB_CHR_TYPE))
	return(adu_nvchr_utf8comp(adf_scb, use_quel_pm, dv1, dv2, rcmp));

    if (db_stat = adu_lenaddr(adf_scb, dv1, &size1, (char **) &p1))
	return (db_stat);
    endp1 = p1 + size1;
    
    if (db_stat = adu_lenaddr(adf_scb, dv2, &size2, (char **) &p2))
	return (db_stat);
    endp2 = p2 + size2;

    blank_pad       = (dv1->db_datatype == DB_CHA_TYPE ||
		       dv1->db_datatype == DB_VCH_TYPE ||
		       dv2->db_datatype == DB_CHA_TYPE ||
		       dv2->db_datatype == DB_VCH_TYPE ||
		       dv1->db_datatype == DB_NCHR_TYPE ||
		       dv1->db_datatype == DB_NVCHR_TYPE ||
		       dv2->db_datatype == DB_NCHR_TYPE ||
		       dv2->db_datatype == DB_NVCHR_TYPE);
    ignore_blanks   = (dv1->db_datatype == DB_CHR_TYPE ||
		       dv2->db_datatype == DB_CHR_TYPE);
		       
    if (dv1->db_datatype == DB_NCHR_TYPE || dv2->db_datatype == DB_NCHR_TYPE ||
	dv1->db_datatype == DB_NVCHR_TYPE || dv2->db_datatype == DB_NVCHR_TYPE)
    {
	db_stat = aduucmp(adf_scb,
		    ignore_blanks ? ADUL_SKIPBLANK :
		    blank_pad ? ADUL_BLANKPAD : 0,
		    (u_i2 *)p1, (u_i2 *)endp1, 
		    (u_i2 *)p2, (u_i2 *)endp2, rcmp,
		    dv1->db_collID);
	if (db_stat != E_DB_OK)
	    return(db_stat);
    }			
    else if (adf_scb->adf_collation)
    {
	if (use_quel_pm)
	{
	    adulstrinit((ADULTABLE *)adf_scb->adf_collation, p1, &s1);
	    adulstrinit((ADULTABLE *)adf_scb->adf_collation, p2, &s2);
	    *rcmp = ad0_3clexc_pm(&s1, endp1, &s2, endp2,
				  blank_pad, ignore_blanks);
	}
	else
	{
	    *rcmp = adugcmp((ADULTABLE *)adf_scb->adf_collation,
			   ignore_blanks ? ADUL_SKIPBLANK :
			   blank_pad ? ADUL_BLANKPAD : 0,
			   p1, endp1, p2, endp2);
	}
    }
    else
    {
	if (use_quel_pm)
	{
	    *rcmp = ad0_1lexc_pm1(p1, endp1, p2, endp2,
				 blank_pad, ignore_blanks);
	}
	else
	{
	    *rcmp = ad0_2lexc_nopm(p1, endp1, p2, endp2,
				   blank_pad, ignore_blanks);
	}
    }

    return (E_DB_OK);
}


/*{
** Name: adu_numeric_norm() - normalise data item for binary comaprison
**
** Description:
**      adu_numeric_norm() supports comparisons between the two data values dv1
**	and dv2. The comparison is a psuedo numeric compare. The data values
**	will be inspected to see if they are numeric and if so, a numeric
**	comparison will take place. The truth is though that we will form a
**	representation of the data that can be binary compared elsewhere.
**
**	Examples:
**
**		        (int)100 = "    100   "(char)
**			(flt)1.9 < "Smith,Homer"(char)
**		(decimal)100.011 < 1e3(float)
**	       (c)"Smith, Homer" = "Smith,Homer"(c)
**
**	    (text)"Smith, Homer" < "Smith,Homer"(text)
**
**	               (c)"abcd" < "abcdd"(c)
**
**	            (text)"abcd" = "aPAT_ANYd"(text)	(QUEL)
**	            (text)"abcd" < "aPAT_ANYd"(text)	(SQL)
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	use_quel_pm			If TRUE, do QUEL pattern matching.  If
**					FALSE, do not.
**      dv1                             Ptr to DB_DATA_VALUE containing first
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the LHS data
**	dv2				Ptr to DB_DATA_VALUE containing second
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the RHS data
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
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or dv2's datatype was
**					incompatible with this operation.
**
**  History:
**	12-Apr-2008 (kiria0) b119410
**	    Created to cater for comparing numeric string data.
*/

DB_STATUS
adu_numeric_norm (
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS	db_stat = E_DB_OK;
    char	decimal = (adf_scb->adf_decimal.db_decspec ?
			(char) adf_scb->adf_decimal.db_decimal : '.');
    f8		dv1fval;
    i4		length;
    i4		rlength;
    char	temp[64];
    char	fill = 0; /* Ideally ' ' but byte compare */
    DB_DATA_VALUE cdata;
    enum tag {
	neg_flt = 10,
	pos_flt = 11,
	string = 100
    };
    struct tagged_bytes {
	char tag;
	u_i1 data[1];
    } *r = (struct tagged_bytes*)rdv->db_data;

    rlength = rdv->db_length - sizeof(r->tag);
    r->tag = string;
    switch (dv1->db_datatype)
    {
    case DB_FLT_TYPE:
        if (dv1->db_length == 4)
            dv1fval = *(f4 *)dv1->db_data;
        else
            dv1fval = *(f8 *)dv1->db_data;
store_flt:
	if (dv1fval < 0.0)
	    r->tag = neg_flt;
	else
	    r->tag = pos_flt;
	if (rlength > sizeof(dv1fval))
	{
#ifdef LITTLE_ENDIAN_INT
	    i4 i;
	    for (i = sizeof(dv1fval); i > 0; i--)
		r->data[sizeof(dv1fval)-i] = ((u_i1*)&dv1fval)[i-1];
#else
	    MEcopy(&dv1fval, sizeof(dv1fval), r->data);
#endif
	    MEfill(rlength - sizeof(dv1fval), 0, r->data+sizeof(dv1fval));
	}
	else
	{
#ifdef LITTLE_ENDIAN_INT
	    i4 i;
	    for (i = sizeof(dv1fval); i > sizeof(dv1fval)-rlength; i--)
		r->data[sizeof(dv1fval)-i] = ((u_i1*)&dv1fval)[i-1];
#else
	    MEcopy(&dv1fval, rlength, r->data);
#endif
	}
        return (E_DB_OK);

    case DB_DEC_TYPE:
	CVpkf((PTR)dv1->db_data,
		(i4)DB_P_DECODE_MACRO(dv1->db_prec),
		(i4)DB_S_DECODE_MACRO(dv1->db_prec),
		&dv1fval);
	goto store_flt;
      
    case DB_INT_TYPE:
        switch (dv1->db_length)
        {
        case 1:
            dv1fval = I1_CHECK_MACRO(*(i1 *) dv1->db_data);
            break;
        case 2:
            dv1fval = *(i2 *)dv1->db_data;
            break;
        case 4:
            dv1fval = *(i4 *)dv1->db_data;
            break;
        case 8:
            dv1fval = *(i8 *)dv1->db_data;
            break;
	default:
	    return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
        }
	goto store_flt;

    case DB_MNY_TYPE:
        dv1fval = ((AD_MONEYNTRNL *) dv1->db_data)->mny_cents;
	goto store_flt;

    case DB_CHR_TYPE:
	fill = 0;
    case DB_CHA_TYPE:
        length = dv1->db_length;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
	MEcopy(dv1->db_data, length, temp);
        temp[length] = EOS;
        if (CVaf(temp, decimal, &dv1fval) == OK)
	    goto store_flt;
	length = dv1->db_length;
	if (rlength > length)
	{
	    MEcopy(dv1->db_data, length, r->data);
	    MEfill(rlength - length, fill, r->data+length);
	}
	else
	    MEcopy(dv1->db_data, rlength, r->data);
        return (E_DB_OK);

    case DB_TXT_TYPE:
	fill = 0;
    case DB_VCH_TYPE:
    case DB_DYC_TYPE:
    case DB_LTXT_TYPE:
    case DB_QUE_TYPE:
    case DB_UTF8_TYPE: /* Not expecting UTF8 numbers to need normalising */
    case DB_QTXT_TYPE:
    case DB_TFLD_TYPE:
        length = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
        MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, length, (PTR)temp);
        temp[length] = EOS;
        if (CVaf(temp, decimal, &dv1fval) == OK)
	    goto store_flt;
        length = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	if (rlength > length)
	{
	    MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, length, r->data);
	    MEfill(rlength - length, fill, r->data+length);
	}
	else
	    MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, rlength, r->data);
        return (E_DB_OK);

    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_NQTXT_TYPE:
	cdata.db_datatype = DB_CHA_TYPE;
	cdata.db_length = sizeof(temp) - 1;
	cdata.db_data = (PTR)temp;

	db_stat = adu_nvchr_coerce(adf_scb, dv1, &cdata);
	if (db_stat)
	    return (db_stat);
	length = cdata.db_length;
	temp[length] = EOS;
	if (CVaf(temp, decimal, &dv1fval) == OK)
	    goto store_flt;
	cdata.db_length = rlength;
	cdata.db_data = (PTR)r->data;

	return (adu_nvchr_coerce(adf_scb, dv1, &cdata));

    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
}


/*{
** Name: adu_lexnumcomp() - Compare two data values, with optional pattern
**			 matching.
**
** Description:
**      adu_lexnumcomp() performs comparisons between the two data values dv1
**	and dv2. The comparison is a psuedo numeric compare. The data values
**	will be inspected to see if they are numeric and if so, a numeric
**	comparison will take place. Otherwise, the comparison will be performed
**	as per adu_lexcomp. If one operand is deemed to be numeric and the other
**	not, then the ordering will be such that all numbers are before all
**	not-numbers. Numbers don't match any patterns including "*"!
**	For details of the non-numeric actions see adu_lexcomp()
**
**	Examples:
**
**		        (int)100 = "    100   "(char)
**			(flt)1.9 < "Smith,Homer"(char)
**		(decimal)100.011 < 1e3(float)
**	       (c)"Smith, Homer" = "Smith,Homer"(c)
**
**	    (text)"Smith, Homer" < "Smith,Homer"(text)
**
**	               (c)"abcd" < "abcdd"(c)
**
**	            (text)"abcd" = "aPAT_ANYd"(text)	(QUEL)
**	            (text)"abcd" < "aPAT_ANYd"(text)	(SQL)
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	use_quel_pm			If TRUE, do QUEL pattern matching.  If
**					FALSE, do not.
**      dv1                             Ptr to DB_DATA_VALUE containing first
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the LHS data
**	dv2				Ptr to DB_DATA_VALUE containing second
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the RHS data
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
**	rcmp				Result of comparison, as follows:
**					    <0	if  dv1 < dv2
**					    =0	if  dv1 = dv2
**					    >0	if  dv1 > dv2
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
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or dv2's datatype was
**					incompatible with this operation.
**
**  History:
**	03-Nov-2007 (kiria0) b119410
**	    Created to cater for comparing numeric string data.
*/

DB_STATUS
adu_lexnumcomp(
ADF_CB          *adf_scb,
i4		use_quel_pm,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
i4		*rcmp)
{
    DB_STATUS	db_stat = E_DB_OK;
    char	decimal = (adf_scb->adf_decimal.db_decspec ?
			(char) adf_scb->adf_decimal.db_decimal : '.');
    DB_STATUS	dv1fsts = OK;
    DB_STATUS	dv2fsts = OK;
    f8		dv1fval;
    f8		dv2fval;
    i4		length;
    char	temp[64];

    /*
    ** First we need to get both sides into shape to determine whether
    ** we are dealing numbers or not.
    ** Essentially coerce lhs (dv1)
    */
    switch (dv1->db_datatype)
    {
    case DB_FLT_TYPE:
        if (dv1->db_length == 4)
            dv1fval = *(f4 *)dv1->db_data;
        else
            dv1fval = *(f8 *)dv1->db_data;
        break;

    case DB_DEC_TYPE:
	CVpkf((PTR)dv1->db_data,
		(i4)DB_P_DECODE_MACRO(dv1->db_prec),
		(i4)DB_S_DECODE_MACRO(dv1->db_prec),
		&dv1fval);
	break;
      
    case DB_INT_TYPE:
        switch (dv1->db_length)
        {
        case 1:
            dv1fval = I1_CHECK_MACRO(*(i1 *) dv1->db_data);
            break;
        case 2:
            dv1fval = *(i2 *)dv1->db_data;
            break;
        case 4:
            dv1fval = *(i4 *)dv1->db_data;
            break;
        case 8:
            dv1fval = *(i8 *)dv1->db_data;
            break;
	default:
	    return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
        }
        break;

    case DB_MNY_TYPE:
        dv1fval = ((AD_MONEYNTRNL *) dv1->db_data)->mny_cents;
	break;

    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
        length = dv1->db_length;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
	MEcopy(dv1->db_data, length, temp);
        temp[length] = EOS;
        dv1fsts = CVaf(temp, decimal, &dv1fval);
        break;

    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_DYC_TYPE:
    case DB_LTXT_TYPE:
    case DB_QUE_TYPE:
    case DB_UTF8_TYPE: /* Not expecting UTF8 numbers to need normalising */
    case DB_QTXT_TYPE:
    case DB_TFLD_TYPE:
        length = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
        MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, length, (PTR)temp);
        temp[length] = EOS;
        dv1fsts = CVaf(temp, decimal, &dv1fval);
        break;

    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_NQTXT_TYPE:
	{
	    DB_DATA_VALUE cdata;
	    cdata.db_datatype = DB_CHA_TYPE;
	    cdata.db_length = sizeof(temp) - 1;
	    cdata.db_data = (PTR)temp;

	    db_stat = adu_nvchr_coerce(adf_scb, dv1, &cdata);
	    if (db_stat)
		return (db_stat);
	    length = cdata.db_length;
	}
	temp[length] = EOS;
	dv1fsts = CVaf(temp, decimal, &dv1fval);
	break;

    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /*
    ** Now the turn for the rhs (dv2)
    */
    switch (dv2->db_datatype)
    {
    case DB_FLT_TYPE:
        if (dv2->db_length == 4)
            dv2fval = *(f4 *)dv2->db_data;
        else
            dv2fval = *(f8 *)dv2->db_data;
        break;

    case DB_DEC_TYPE:
	CVpkf((PTR)dv2->db_data,
		(i4)DB_P_DECODE_MACRO(dv2->db_prec),
		(i4)DB_S_DECODE_MACRO(dv2->db_prec),
		&dv2fval);
	break;
      
    case DB_INT_TYPE:
        switch (dv2->db_length)
        {
        case 1:
            dv2fval = I1_CHECK_MACRO(*(i1 *) dv2->db_data);
            break;
        case 2:
            dv2fval = *(i2 *)dv2->db_data;
            break;
        case 4:
            dv2fval = *(i4 *)dv2->db_data;
            break;
        case 8:
            dv2fval = *(i8 *)dv2->db_data;
            break;
	default:
	    return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
        }
        break;

    case DB_MNY_TYPE:
        dv2fval = ((AD_MONEYNTRNL *) dv2->db_data)->mny_cents;
	break;

    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
        length = dv2->db_length;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
	MEcopy(dv2->db_data, length, temp);
        temp[length] = EOS;
        dv2fsts = CVaf(temp, decimal, &dv2fval);
        break;

    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_DYC_TYPE:
    case DB_LTXT_TYPE:
    case DB_QUE_TYPE:
    case DB_UTF8_TYPE: /* Not expecting UTF8 numbers to need normalising */
    case DB_QTXT_TYPE:
    case DB_TFLD_TYPE:
        length = ((DB_TEXT_STRING *) dv2->db_data)->db_t_count;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
        MEcopy((PTR)((DB_TEXT_STRING *)dv2->db_data)->db_t_text, length, (PTR)temp);
        temp[length] = EOS;
        dv2fsts = CVaf(temp, decimal, &dv2fval);
        break;

    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_NQTXT_TYPE:
	{
	    DB_DATA_VALUE cdata;
	    cdata.db_datatype = DB_CHA_TYPE;
	    cdata.db_length = sizeof(temp) - 1;
	    cdata.db_data = (PTR)temp;

	    db_stat = adu_nvchr_coerce(adf_scb, dv2, &cdata);
	    if (db_stat)
		return (db_stat);
	    length = cdata.db_length;
	}
	temp[length] = EOS;
	dv2fsts = CVaf(temp, decimal, &dv2fval);
	break;

    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    if (dv1fsts == OK)
    {
	if (dv2fsts == OK)
	{
	    /* We have two numbers - do the compare */
	    dv1fval -= dv2fval;
	    *rcmp = (dv1fval < 0) ? -1 : (dv1fval > 0)? 1 : 0;
	    return E_AD0000_OK;
	}
	else
	{
	    *rcmp = -1;	/* NUM cf STR */
	    return E_AD0000_OK;
	}
    }
    else if (dv2fsts == OK)
    {
	*rcmp = 1; /* STR cf NUM */
	return E_AD0000_OK;
    }
    /* Otherwise drop through before to string compare */

    return adu_lexcomp(adf_scb, use_quel_pm, dv1, dv2, rcmp);
}

/*{
** Name: adu_isinteger() - Check data value for integer data.
**
** Description:
**      adu_isinteger() performs check of dv1 for data in integer form.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing first
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the data
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
**	rcmp				Result of comparison, as follows:
**					    =0	if  dv1 is not integer
**					    =1	if  dv1 is integer
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
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD9999_INTERNAL_ERROR	dv1's datatype was incompatible with
**					this operation.
**
**  History:
**	26-Dec-2007 (kiria0) SIR119658
**	    Allow for checking data for integer compatibility.
*/

DB_STATUS
adu_isinteger(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
i4		*rcmp)
{
    DB_STATUS	db_stat = E_DB_OK;
    char	decimal = (adf_scb->adf_decimal.db_decspec ?
			(char) adf_scb->adf_decimal.db_decimal : '.');
    i8		ival;
    f8		dv1fval;
    i4		length;
    char	temp[64];
    char        *frac = NULL, *exp = NULL;

    /* Assume not integer */
    *rcmp = 0;

    switch (dv1->db_datatype)
    {
    case DB_MNY_TYPE:
    case DB_FLT_TYPE:
        /* For float types assert that there is no fractional part
        **and that the representation scale is within range.
        */
        if (dv1->db_length == 4)
        {
            dv1fval = *(f4 *)dv1->db_data;
            if (dv1fval >= 0.0
		? dv1fval <= (f8)MAXI8
		: -dv1fval < (f8)MAXI8)
            {
		ival = (i8)dv1fval;
		if ((f4)ival == dv1fval)
		    *rcmp = 1;
	    }
	}
        else
        {
	    if (dv1->db_datatype == DB_MNY_TYPE)
		dv1fval = ((AD_MONEYNTRNL *)dv1->db_data)->mny_cents;
	    else
		dv1fval = *(f8 *)dv1->db_data;
            if (dv1fval >= 0.0
		? dv1fval <= (f8)MAXI8
		: -dv1fval < (f8)MAXI8)
            {
		ival = (i8)dv1fval;
		if ((f8)ival == dv1fval)
		    *rcmp = 1;
	    }
	}
	return E_AD0000_OK;

    case DB_DEC_TYPE:
        /* Would use CVpkl8 but it is happy to truncate
        ** to int - not what we're after. Instead convert
        ** to string and treat it in the common code.
        */
	if (CVpka((PTR)dv1->db_data,
		(i4)DB_P_DECODE_MACRO(dv1->db_prec),
		(i4)DB_S_DECODE_MACRO(dv1->db_prec),
		decimal,
		(i4)DB_P_DECODE_MACRO(dv1->db_prec)+1,
		(i4)DB_S_DECODE_MACRO(dv1->db_prec),
		CV_PKZEROFILL,
		temp,
		&length) != OK)
	{
	    /* Treat any conversion error as - not int either */
	    return E_AD0000_OK;
	}
	/* Drop out for common string check */
	break;

    case DB_INT_TYPE:
	/* No conversion */
        *rcmp = 1;
	return E_AD0000_OK;

    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
        length = dv1->db_length;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
	MEcopy(dv1->db_data, length, temp);
	/* Drop out for common string check */
        break;

    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_DYC_TYPE:
    case DB_LTXT_TYPE:
    case DB_QUE_TYPE:
    case DB_UTF8_TYPE: /* Not expecting UTF8 numbers to need normalising */
    case DB_QTXT_TYPE:
    case DB_TFLD_TYPE:
        length = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
        MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, length, (PTR)temp);
	/* Drop out for common string check */
        break;

    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_NQTXT_TYPE:
	{
	    DB_DATA_VALUE cdata;
	    cdata.db_datatype = DB_CHA_TYPE;
	    cdata.db_length = sizeof(temp) - 1;
	    cdata.db_data = (PTR)temp;

	    db_stat = adu_nvchr_coerce(adf_scb, dv1, &cdata);
	    if (db_stat)
		return (db_stat);
	    length = cdata.db_length;
	}
	/* Drop out for common string check */
	break;

    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /* Common string check */
    temp[length] = EOS;
    frac = STchr(temp, decimal);
    exp = STchr(temp, 'e');
    if (exp == NULL)
	exp = STchr(temp, 'E');

    /* If there is not exponent then any fractional part
    ** must be zero and the mantissa withing range for I8
    */
    if (!exp)
    {
        if (frac)
        {
            /* Terminate at DP */
            *frac++ = '\0';

	    if (CMGETDBL)
	    {
	    	/* Skip zeros */
                while (*frac == '0')
               	    CMnext(frac);
	    	/* Skip trailing spaces */
            	while (CMspace(frac))
                    CMnext(frac);
	    }
	    else
	    {
	    	/* Skip zeros */
            	while (*frac == '0')
                    CMnext_SB(frac);
	    	/* Skip trailing spaces */
            	while (CMspace_SB(frac))
                    CMnext_SB(frac);
	    }

            /* We must be at end of string otherwise not integral */
            if (*frac)
                return E_AD0000_OK;
	    /* Fraction was zero so ignore it & let CVal8 do the rest */
	}
        if (CVal8(temp, &ival) == OK)
	    *rcmp = 1;
    }
    /* Looks like a float form but might still be integral? */
    else if (CVaf(temp, decimal, &dv1fval) == OK &&
	    (dv1fval >= 0.0 ? dv1fval : -dv1fval) < (f8)MAXI8)
    {
	ival = (i8)dv1fval;
	if ((f8)ival == dv1fval)
	    *rcmp = 1;
    }
    return E_AD0000_OK;
}

/*{
** Name: adu_isdecimal() - Check data value for decimal data.
**
** Description:
**      adu_isdecimal() performs check of dv1 for data in decimal form.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing first
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the LHS data
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
**	rcmp				Result of comparison, as follows:
**					    =0	if  dv1 is not decimal
**					    =1	if  dv1 is decimal
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
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD9999_INTERNAL_ERROR	dv1's datatype was incompatible with
**					this operation.
**
**  History:
**	26-Dec-2007 (kiria0) SIR119658
**	    Allow for checking data for decimal compatibility.
**      13-Jan-2010 (wanfr01) Bug 123139
**          Optimizations for single byte
*/

DB_STATUS
adu_isdecimal(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
i4		*rcmp)
{
    DB_STATUS	db_stat = E_DB_OK;
    char	decimal = (adf_scb->adf_decimal.db_decspec ?
			(char) adf_scb->adf_decimal.db_decimal : '.');
    DB_STATUS	dv1fsts = OK;
    f8		dv1fval;
    i4		length;
    char	temp[64];
    char        *frac = NULL, *exp = NULL, *p;

    /* Assume not decimal */
    *rcmp = 0;

    switch (dv1->db_datatype)
    {
    case DB_MNY_TYPE:
    case DB_FLT_TYPE:
	if (dv1->db_datatype == DB_MNY_TYPE)
	    dv1fval = ((AD_MONEYNTRNL *) dv1->db_data)->mny_cents;
	else if (dv1->db_length == 4)
	    dv1fval = *(f4 *)dv1->db_data;
	else
	    dv1fval = *(f8 *)dv1->db_data;

	/* Check that the magnitude is representable as decimal */
        if ((dv1fval >= 0.0 ? dv1fval : -dv1fval) < CL_LIMIT_DECVALF8)
            *rcmp = 1;
	return E_AD0000_OK;

    case DB_DEC_TYPE:
    case DB_INT_TYPE:
	/* All ints will fit as should the decimals */
        *rcmp = 1;
	return E_AD0000_OK;

    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
        length = dv1->db_length;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
	MEcopy(dv1->db_data, length, temp);
	/* Drop to common string processing */
        break;

    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_DYC_TYPE:
    case DB_LTXT_TYPE:
    case DB_QUE_TYPE:
    case DB_UTF8_TYPE: /* Not expecting UTF8 numbers to need normalising */
    case DB_QTXT_TYPE:
    case DB_TFLD_TYPE:
        length = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
        MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, length, (PTR)temp);
	/* Drop to common string processing */
        break;

    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_NQTXT_TYPE:
	{
	    DB_DATA_VALUE cdata;
	    cdata.db_datatype = DB_CHA_TYPE;
	    cdata.db_length = sizeof(temp) - 1;
	    cdata.db_data = (PTR)temp;

	    db_stat = adu_nvchr_coerce(adf_scb, dv1, &cdata);
	    if (db_stat)
		return (db_stat);
	    length = cdata.db_length;
	}
	/* Drop to common string processing */
	break;

    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /* Common string check */
    temp[length] = EOS;
    p = temp;
    /* Lose leading spaces */

    if (CMGETDBL)
    {
        while (CMspace(p))
	    CMnext(p);
        if (*p == '-' || *p == '+')
        {
	    /* Skip sign and any associated spaces */
	    CMnext(p);
	    while (CMspace(p))
	        CMnext(p);
        }
        /* Skip any leading zeros */
        while (*p == '0')
	    CMnext(p);
    }
    else
    {
        while (CMspace_SB(p))
	    CMnext_SB(p);
        if (*p == '-' || *p == '+')
    	{
	    /* Skip sign and any associated spaces */
	    CMnext_SB(p);
	    while (CMspace_SB(p))
	    	CMnext_SB(p);
    	}
    	/* Skip any leading zeros */
    	while (*p == '0')
	    CMnext_SB(p);
    }

    exp = STchr(p, 'e');
    if (exp == NULL)
	exp = STchr(p, 'E');

    /* If there is no exponent then the remaining characters
    ** must be digits and possibly a DP
    */
    if (!exp)
    {
	i4 maxplaces = CL_MAX_DECPREC;

        char *start = p;
        while (CMdigit(p))
            CMnext(p);
        if (*p == decimal)
        {
            CMnext(p);
	    while (CMdigit(p))
		CMnext(p);
	    if (p - start > CL_MAX_DECPREC + 1)
		/* Number too long */
		return E_AD0000_OK;
        }
        else if (p - start > CL_MAX_DECPREC)
	    /* Number too long */
	    return E_AD0000_OK;
	while (CMspace(p))
	    CMnext(p);
	if (!*p)
	    /* Length ok and valid number */
	    *rcmp = 1;
    }
    /* Looks like a float form but might still fit? */
    else if (CVaf(temp, decimal, &dv1fval) == OK &&
	    (dv1fval >= 0.0 ? dv1fval : -dv1fval) < CL_LIMIT_DECVALF8)
    {
	*rcmp = 1;
    }

    return E_AD0000_OK;
}

/*{
** Name: adu_isfloat() - Check data value for float data.
**
** Description:
**      adu_isfloat() performs check of dv1 for data in float form.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing first
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the LHS data
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
**	rcmp				Result of comparison, as follows:
**					    =0	if  dv1 is not float
**					    =1	if  dv1 is float
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
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD9999_INTERNAL_ERROR	dv1's datatype was incompatible with
**					this operation.
**
**  History:
**	26-Dec-2007 (kiria0) SIR119658
**	    Allow for checking data for float compatibility.
*/

DB_STATUS
adu_isfloat(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
i4		*rcmp)
{
    DB_STATUS	db_stat = E_DB_OK;
    char	decimal = (adf_scb->adf_decimal.db_decspec ?
			(char) adf_scb->adf_decimal.db_decimal : '.');
    f8		dv1fval;
    i4		length;
    char	temp[64];

    switch (dv1->db_datatype)
    {
    case DB_FLT_TYPE:
    case DB_DEC_TYPE:
    case DB_INT_TYPE:
    case DB_MNY_TYPE:
	*rcmp = 1;
	return E_AD0000_OK;

    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
        length = dv1->db_length;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
	MEcopy(dv1->db_data, length, temp);
	/* Drop to common string processing */
        break;

    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_DYC_TYPE:
    case DB_LTXT_TYPE:
    case DB_QUE_TYPE:
    case DB_UTF8_TYPE: /* Not expecting UTF8 numbers to need normalising */
    case DB_QTXT_TYPE:
    case DB_TFLD_TYPE:
        length = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
        MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, length, (PTR)temp);
	/* Drop to common string processing */
        break;

    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_NQTXT_TYPE:
	{
	    DB_DATA_VALUE cdata;
	    cdata.db_datatype = DB_CHA_TYPE;
	    cdata.db_length = sizeof(temp) - 1;
	    cdata.db_data = (PTR)temp;

	    db_stat = adu_nvchr_coerce(adf_scb, dv1, &cdata);
	    if (db_stat)
		return (db_stat);
	    length = cdata.db_length;
	}
	/* Drop to common string processing */
	break;

    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /* Common string check */
    temp[length] = EOS;
    if (CVaf(temp, decimal, &dv1fval) == OK)
	*rcmp = 1;
    else
        *rcmp = 0;
    return E_AD0000_OK;
}

/*
** Name: ad0_copy_pat() - Copy pattern and rationalise it.
**
**  Description:
**	This routine strips out parts of the input pattern that can
**	cause unnecessary cpu usage. The sort of thing that we wish
**	to avoid amount to replications of the ANY pattern as by default
**	this cause 'n' levels of recursion to find an embedded match.
**	There are however other ways to get the same results and a
**	malicious user could expliot these. What this routine looks
**	for are runs of match one and match any which it re-arranges
**	into N match ones and a single match any if needed. Also looked
**	for are cases of null ranges which are to be ignored and
**	potentially these could contain ignorable whitespace. Any ignorable
**	whitespace will be removed from the pattern along with null ranges.
**
** Parameters:
**	s		Buffer pointer to receive potimised pattern - must be
**			at least the same length as endstr-str
**	str		Input pattern buffer start
**	endstr		Input pattern buffer end
**	bignore		Flag to allow for ignoring whitespace.
**
** Returns:
**	End of output buffer pointer.
**
**  History:
**	13-Feb-2007 (kiria01)
**	    Created
*/
static	u_char*
ad0_copy_pat(
u_char		*s,
u_char		*str,
u_char		*endstr,
bool		bignore)
{
    i4	n = 0;
    i4	any = FALSE;

    while (str < endstr)
    {
	switch (*str)
	{
	case DB_PAT_ONE:
	    n++;
	    CMnext(str);
	    break;

	case DB_PAT_ANY:
	    any = TRUE;
	    CMnext(str);
	    break;

	case DB_PAT_LBRAC:
	    {
		u_char *hold = s;
		i4 i;
		i4 bseen = 0;
		/* Expect to have to emit the ANY and ONE patterns */
		for (i = 0; i < n; i++)
		    *s++ = DB_PAT_ONE;
		if (any)
		    *s++ = DB_PAT_ANY;
		CMcpyinc(str, s);
		while (str < endstr && *str != DB_PAT_RBRAC)
		{
		    if (!bignore || *str && !CMspace(str))
			CMcpyinc(str, s);
		    else
		    {
			/*
			** This should just skip the space but strangly a range
			** with a non-leading space could match a zero-length
			** string! So we may need to leave a space in.
			*/
			if (s[-1] != DB_PAT_LBRAC)
			    bseen = 1;
			CMnext(str);
		    }
		}
		if (s[-1] == DB_PAT_LBRAC)
		{
		    /* Null range - lose it even if bseen */
		    s = hold;
		    CMnext(str);
		}
		else
		{
		    if (bseen)
			*s++ = ' ';
		    if (str < endstr)
			CMcpyinc(str, s);
		    n = 0;
		    any = FALSE;
		}
		break;
	    }
	default:
	    if (!bignore || *str && !CMspace(str))
	    {
		for (; n > 0; n--)
		    *s++ = DB_PAT_ONE;
		if (any)
		    *s++ = DB_PAT_ANY;
		any = FALSE;
		CMcpyinc(str, s);
	    }
	    else
	    {
		CMnext(str);
	    }
	}
    }
    /* May be some unwritten data */
    for (; n > 0; n--)
	*s++ = DB_PAT_ONE;
    if (any)
	*s++ = DB_PAT_ANY;
    return s;
}

/*
** Name: ad0_1lexc_pm() - Wrapper routine to real recursive routine used to compare
**		          two strings using QUEL pattern matching semantics.
**
**  Description:
**	This entry performs a quick optimize of the patterns.
**
**  History:
**	13-Feb-2007 (kiria01)
**	    Created
*/

static	i4
ad0_1lexc_pm(
register u_char     *str1,
u_char		    *endstr1,
register u_char     *str2,
u_char		    *endstr2,
bool		    bpad,
bool		    bignore)
{
	STATUS	cl_stat;
	i4	sts;
	u_char	*s1 = (u_char *)MEreqmem(0, endstr1-str1 + endstr2-str2, 
						FALSE, &cl_stat);
	u_char	*e1, *e2;

	if (s1 && !cl_stat)
	{
		e1 = ad0_copy_pat(s1, str1, endstr1, bignore);
		e2 = ad0_copy_pat(e1, str2, endstr2, bignore);
		sts = ad0_1lexc_pm1(s1, e1, e1, e2, bpad, bignore);
		MEfree((char *)s1);
	}
	else
	{
		sts = ad0_1lexc_pm1(str1, endstr1, str2, endstr2, bpad, bignore);
	}
	return sts;
}
/*
** Name: ad0_1lexc_pm1() - Recursive routine used by adu_lexcomp() to compare
**		          two strings using QUEL pattern matching semantics.
**
**  Description:
**
**  History:
**      03/11/83 (lichtman) -- changed the way switch statements are
**                  handled due to a Whitesmith's C bug:
**                  characters are sign-extended to i4
**                  when switching on them.  Since the
**                  pattern matching characters now have
**                  the sign bit on, they are sign extended
**                  to negative numbers which don't compare
**                  right with the constants.  Also did
**                  same thing with if statements since
**                  those didn't seem to work either.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	13-aug-87 (thurston)
**	    Added code to allow a range with a blank or null char specifically
**	    in it to be skipped if blanks and null chars are to be ignored.
**	20-nov-87 (thurston)
**	    Major changes in order to avoid doing blank
**	    padding when pattern match characters are present (even when `char'
**	    or `varchar' are involved).  This is consistent with the LIKE
**	    operator in SQL, and makes a lot of sense.  A couple of other minor
**	    bugs came out in the wash, also; e.g. "ab" now matches "ab[]".
**	14-jul-88 (jrb)
**	    Changed for Kanji support.
**	09-dec-88 (jrb)
**	    Fixed problem where bracket pattern matching didn't work if they
**	    were in the first string passed to this routine.
**	24-Apr-89 (anton)
**	    Fixed problem where '<' or '>' pattern matching would be incorrect.
**	27-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
*/

static	i4
ad0_1lexc_pm1(
register u_char     *str1,
u_char		    *endstr1,
register u_char     *str2,
u_char		    *endstr2,
bool		    bpad,
bool		    bignore)
{
    u_i4	    ch1;
    u_i4	    ch2;
    i4		    stat;
    bool	    have_bpadded = FALSE;
    u_char	    blank = AD_BLANK_CHAR;


loop:
    while (str1 < endstr1)
    {
        ch1 = *str1;

	switch (ch1)
	{
	  case DB_PAT_ONE:
	    if (have_bpadded)
		return (1);	/* must have bpadded str2 */
	    else
		return (ad0_qmatch(str1+1, endstr1, str2, endstr2, bignore));

	  case DB_PAT_ANY:
	    if (have_bpadded)
		return (1);	/* must have bpadded str2 */
	    else
		return (ad0_pmatch(str1+1, endstr1, str2, endstr2, bignore));

	  case DB_PAT_LBRAC:
	    if (have_bpadded)
		return (1);	/* must have bpadded str2 */
	    else
		return (ad0_lmatch(str1+1, endstr1, str2, endstr2, bignore));

	  case NULLCHAR:
	    if (bignore)
		break;

	    /* else, fall thru to `default:' */

	  default:
	    if (CMspace(str1)  &&  bignore)
		break;
		
            while (str2 < endstr2)
            {
                ch2 = *str2;

		switch (ch2)
		{
		  case DB_PAT_ONE:
		    if (have_bpadded)
			return (-1);	/* must have bpadded str1 */
		    else
			return (-ad0_qmatch(str2+1,
					    endstr2, str1, endstr1, bignore));

		  case DB_PAT_ANY:
		    if (have_bpadded)
			return (-1);	/* must have bpadded str1 */
		    else
			return (-ad0_pmatch(str2+1,
					    endstr2, str1, endstr1, bignore));

		  case DB_PAT_LBRAC:
		    if (have_bpadded)
			return (-1);	/* must have bpadded str1 */
		    else
			return (-ad0_lmatch(str2+1,
					    endstr2, str1, endstr1, bignore));

		  case NULLCHAR:
		    if (bignore)
		    {
			str2++;
			continue;
		    }
		    /* else, fall thru to `default:' */

		  default:
		    if (CMspace(str2)  &&  bignore)
		    {
			CMnext(str2);
			continue;
		    }
		    
                    if ((stat = CMcmpcaselen(str1, endstr1, str2, endstr2)) == 0)
		    {
			CMnext(str1);
			CMnext(str2);
                        goto loop;
		    }
		    else
		    {
			return (stat);
		    }
                }
            }

	    /* string 2 is out of characters, string 1 still has some */
	    /* examine remainder of string 1 for any characters */
	    while (str1 < endstr1)
	    {
		switch (ch1 = *str1)
		{
		  case DB_PAT_ONE:
		    return (1);		/* must have bpadded str2 */

		  case DB_PAT_ANY:
		    if (have_bpadded)
			return (1);	/* must have bpadded str2 */
		    else
			bpad = FALSE;
		    continue;

		  case DB_PAT_LBRAC:
		    if (have_bpadded)
			return (1);	/* must have bpadded str2 */
		    else
			return (ad0_lmatch(str1+1,
						endstr1, str2, str2, bignore));

		  case NULLCHAR:
		    str1++;
		    if (bignore)
			continue;
		    if (bpad  &&  !ad0_pmchk(str1, endstr1))
			return (-1);	/* nullchar < padded blank */
		    else
			return (1);	/* str1 longer than str2 */

		  default:
		    if (CMspace(str1))
		    {
			CMnext(str1);
			if (bignore)
			    continue;
			if (bpad)
			{
			    have_bpadded = TRUE;
			    continue;	/* blank = blank */
			}
			else
			{
			    return (1);	/* str1 longer than str2 */
			}
		    }
		    else
		    {
			if (    bpad
			    &&  CMcmpcase(str1, &blank) < 0
			    &&  !ad0_pmchk(str1+1, endstr1)
			   )
			    return (-1);
			else
			    return (1);	/* str1 longer than str2, or
					** ch1 greater than a blank.
					*/
		    }
		}
	    }
			
	    return (0);	    /* str1 = str2 */
        }
	CMnext(str1);
    }

    /* string 1 is out of characters */
    /* examine remainder of string 2 for any characters */
    while (str2 < endstr2)
    {
	switch (ch2 = *str2)
	{
	  case DB_PAT_ONE:
	    return (-1);	/* must have bpadded str1 */

	  case DB_PAT_ANY:
	    str2++;
	    if (have_bpadded)
		return (-1);	/* must have bpadded str1 */
	    else
		bpad = FALSE;
	    continue;

	  case DB_PAT_LBRAC:
	    if (have_bpadded)
		return (-1);	/* must have bpadded str1 */
	    else
		return (-(ad0_lmatch(str2+1,
			    endstr2, (u_char *)NULL, (u_char *)NULL, bignore)));

	  case NULLCHAR:
	    str2++;
	    if (bignore)
		continue;
	    if (bpad  &&  !ad0_pmchk(str2, endstr2))
		return (1);	/* blank > nullchar */
	    else
		return (-1);	/* str1 shorter than str2 */

	  default:
	    if (CMspace(str2))
	    {
		CMnext(str2);
		if (bignore)
		    continue;
		if (bpad)
		{
		    have_bpadded = TRUE;
		    continue;	/* blank = blank */
		}
		else
		{
		    return (-1);	/* str1 shorter than str2 */
		}
	    }
	    else
	    {
		if (   bpad
		    && CMcmpcase(str2, &blank) < 0
		    && !ad0_pmchk(str2+1, endstr2)
		   )
		    return (1);
		else
		    return (-1);	/* str1 shorter than str2, or
					** ch2 greater than a blank.
					*/
	    }
	}
    }
		
    return (0);	    /* str1 = str2 */
}


/*
** Name: ad0_qmatch()
**
**  History:
**	20-nov-87 (thurston)
**	    Created to solve the "don't do blank padding with pattern match"
**	    problem (even when `char' or `varchar' are involved).
**	14-jul-88 (jrb)
**	    Changes for Kanji support.
*/

static  i4
ad0_qmatch(
u_char		*pat,	    /* the string holding the pattern matching char */
u_char		*endpat,    /* pointer to end of char string */
u_char		*str,	    /* the other string */
u_char		*endstr,    /* pointer to the end */
bool		bignore)    /* If TRUE, ignored blanks and null chars */
{
    /* find a non-blank, non-null char in str, if ignoring these */
    while (str < endstr)
    {
	if (!bignore || (!CMspace(str)  &&  *str != NULLCHAR))
	    break;

	CMnext(str);
    }
    
    if (str < endstr)
    {
	CMnext(str);
	return (ad0_1lexc_pm1(pat, endpat, str, endstr, FALSE, bignore));
    }
    else
    {
	return (1);
    }
}


/*
**  Name: ad0_pmatch() 
**
**  History:
**      03/11/83 (lichtman) -- changed to get around Whitesmith C bug
**                  mentioned above in ad0_1lexc_pm1().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-nov-87 (thurston)
**	    Some changes in order to avoid doing blank
**	    padding when pattern match characters are present (even when `char'
**	    or `varchar' are involved).  This is consistent with the LIKE
**	    operator in SQL, and makes a lot of sense.
**	14-jul-88 (jrb)
**	    Changes for Kanji support.
**	13-Dec-06 (wanfr01)
**	    Bug 116757 - consolidate consecutive match-any characters
*/

static  i4
ad0_pmatch(
u_char          *pat,       /* the string holding the pattern matching char */
u_char	    	*endpat,    /* pointer to end of pattern char string */
u_char      	*str,       /* the string to be checked */
u_char	    	*endstr,    /* pointer to end of string */
bool		bignore)    /* if TRUE, we ignore blanks and null chars */
{
    register u_char *s;
    u_i4	    c;
    u_i4	    d;


    s = str;

    /*
    ** Skip over blanks and null chars if requested -- This fixes a
    ** bug where "a* " would not match "aaa".  The extra space after
    ** the '*' is often put in by EQUEL programs.
    ** Also skip any contiguous 'MATCH-ANY' characters since they are
    ** extraneous.
    */
    while (pat < endpat && 
	     (  (bignore && (CMspace(pat)  || (c = *pat) == NULLCHAR)) ||
	   	((c = *pat) == DB_PAT_ANY)
	     )
	  )
    {
        CMnext(pat);
    }

    if (pat >= endpat)
        return  (0);    /* a match if no more chars in p */

    /*
    ** If the next character in "pat" is not another
    ** pattern matching character, then scan until
    ** first matching char and continue comparison.
    */
    c = *pat;
    if (c != DB_PAT_ANY && c != DB_PAT_LBRAC && c != DB_PAT_ONE)
    {
        while (s < endstr)
        {
            d = *s;
            if (    CMcmpcase(pat, s) == 0
		||  d == DB_PAT_ANY
		||  d == DB_PAT_LBRAC
		||  d == DB_PAT_ONE
	       )
            {
                if (ad0_1lexc_pm1(pat, endpat, s, endstr, FALSE, bignore) == 0)
                    return (0);
            }
            CMnext(s);
        }
    }
    else
    {
        while (s < endstr)
            if (ad0_1lexc_pm1(pat, endpat, s++, endstr, FALSE, bignore) == 0)
                return (0); /* match */
    }
    return (-1);    /* no match */
}


/*
** Name: ad0_lmatch()
**
**  History:
**      05/23/85 (lichtman) -- fixed bug #5445.  For some reason, ad0_1lexc_pm()
**                  was called with last parameter 0 instead of bignore.
**                  This caused [] type pattern matching not to work
**                  on C type strings with trailing blanks.
**      12/17/85 (eda) --   change if statement that included logical operators
**                  in conjunction with *p++.  If the early part of the
**                  test failed, p++ was not happening. (see K&R p.190)
**
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	13-aug-87 (thurston)
**	    Fixed bug for the following compare:
**
**		"[ ab]c" = "  c      "
**		\______/   \_________/
**	           |            |
**		  text          c
**
**	    In this type of compare, blanks and null chars are to be ignored,
**	    so the compare should return TRUE.  It was not.  It does now.
**	20-nov-87 (thurston)
**	    Some changes in order to avoid doing blank
**	    padding when pattern match characters are present (even when `char'
**	    or `varchar' are involved).  This is consistent with the LIKE
**	    operator in SQL, and makes a lot of sense.
**	14-jul-88 (jrb)
**	    Changes for Kanji support.
*/

static  i4
ad0_lmatch(
u_char		*pat,	    /* the string holding the pattern matching char */
u_char		*endpat,    /* end of the pattern string */
u_char		*str,	    /* the other string */
u_char		*endstr,    /* end of the string */
bool		bignore)    /* If TRUE, ignore blanks and null chars */
{
    register u_char	*p;
    register u_char	*s;
    u_char		*oldc = NULL;
    i4			found;
    i4			bfound;


    p = pat;
    s = str;

    /* Look for an empty range, if found, just ignore it */
    while (p < endpat)
    {
	if (*p == DB_PAT_RBRAC)
	{
	    /* empty range, ignore it */
	    return (ad0_1lexc_pm1(++p, endpat, s, endstr, FALSE, bignore));
	}
	else
	{
	    if ((CMspace(p)  ||  *p == NULLCHAR) && bignore)
		CMnext(p);
	    else
		break;
	}
    }


    /* find a non-blank, non-null char in s, if ignoring these */
    while (s < endstr)
    {
	if ((CMspace(s)  ||  *s == NULLCHAR) && bignore)
	{
	    CMnext(s);
	    continue;
	}

	/* search for a match on 'c' */
	found = 0;	/* assume failure */
	bfound = 0;	/* assume failure */

	while (p < endpat)
	{
	    switch(*p)
	    {
	      case DB_PAT_RBRAC:
		{
		    i4	    ret_val = -1;

		    if (bfound)
		    {   /*
			** Since we found a blank or null char in the pattern
			** range, and blanks and null chars are being ignored,
			** try this first to see if the two are equal by
			** ignoring the range altogether.
			*/
			ret_val = ad0_1lexc_pm1(p+1, endpat,
						    s, endstr, FALSE, bignore);
		    }

		    if (ret_val != 0  &&  found)
		    {
			CMnext(s);
			ret_val = ad0_1lexc_pm1(p+1, endpat,
						  s, endstr, FALSE, bignore);
		    }
		    return (ret_val);
		}

	      case AD_DASH_CHAR:
		if (oldc == NULL  ||  ++p >= endpat)
		    return (-1);    /* not found ... really an error */
	    
		if (CMcmpcase(oldc, s) <= 0  &&  CMcmpcase(p, s) >= 0)
		    found++; 
		break;

	      default:
		oldc = p;

		if ((CMspace(p)  ||  *p == NULLCHAR)  &&  bignore)
		    bfound++;

		if (CMcmpcase(s, p) == 0)
		    found++;
	    }
	    CMnext(p);
	}
	return (-1);    /* no match ... this should actually be an error,
			** because if we reach here, we would have an LBRAC
			** without an associated RBRAC.
			*/
    }
    return (1);
}


/*
** Name: ad0_pmchk() - Returns TRUE if string has PM chars, FALSE otherwise.
**
**  History:
**	20-nov-87 (thurston)
**	    Created to solve the "don't do blank padding with pattern match"
**	    problem (even when `char' or `varchar' are involved).
**	15-jul-88 (jrb)
**	    Changes for Kanji support.
*/

static  bool
ad0_pmchk(
u_char		*str,	    /* string to look for PM chars in */
u_char		*endstr)    /* end of str */
{
    while (str < endstr)
    {
	switch (*str)
	{
	  case DB_PAT_ONE:
	  case DB_PAT_ANY:
	  case DB_PAT_LBRAC:	/* Don't want to look for RBRAC */
	    return (TRUE);

	  default:
	    CMnext(str);
	    break;
	}
    }
    
    return (FALSE);
}


/*
** Name: ad0_2lexc_nopm() - Routine used by adu_lexcomp() to compare two strings
**		            without using QUEL pattern matching semantics.
**
**  Description:
**
**  History:
**      08-mar-87 (thurston)
**	    Initial creation.
**	15-jul-88 (jrb)
**	    Changes for Kanji support.
**	2-Dec-88 (eric)
**	    Fixed comparison bugs that relate to blank padding. Instead of
**	    trying to fix just those bugs, I replaced the contents of this
**	    routine with the the contents of ado_1lexc_pm() minus the cases
**	    to handle pattern matching. This will insure that the semantics
**	    of these two routines will be the same, except for pattern matching,
**	    which is as it should be. At some point, we should look at merging
**	    the two routines into one, to avoid having a bug fixed in one
**	    but not the other.
**	    
*/

static	i4
ad0_2lexc_nopm(
register u_char     *str1,
u_char		    *endstr1,
register u_char     *str2,
u_char		    *endstr2,
bool		    bpad,
bool		    bignore)
{
    u_i4	    ch1;
    u_i4	    ch2;
    i4		    stat;
    bool	    have_bpadded = FALSE;
    u_char	    blank = AD_BLANK_CHAR;


loop:
    while (str1 < endstr1)
    {
        ch1 = *str1;

	switch (ch1)
	{
	  case NULLCHAR:
	    if (bignore)
		break;
	    /* else, fall thru to `default:' */

	  default:
	    if (CMspace(str1)  &&  bignore)
		break;
		
            while (str2 < endstr2)
            {
                ch2 = *str2;

		switch (ch2)
		{
		  case NULLCHAR:
		    if (bignore)
		    {
			str2++;
			continue;
		    }
		    /* else, fall thru to `default:' */

		  default:
		    if (CMspace(str2)  &&  bignore)
		    {
			CMnext(str2);
			continue;
		    }
		    
                    if ((stat = CMcmpcaselen(str1, endstr1, str2, endstr2)) == 0)
		    {
			CMnext(str1);
			CMnext(str2);
                        goto loop;
		    }
		    else
		    {
			return (stat);
		    }
                }
            }

	    /* string 2 is out of characters, string 1 still has some */
	    /* examine remainder of string 1 for any characters */
	    while (str1 < endstr1)
	    {
		switch (ch1 = *str1)
		{
		  case NULLCHAR:
		    str1++;
		    if (bignore)
			continue;
		    if (bpad)
			return (-1);	/* nullchar < padded blank */
		    else
			return (1);	/* str1 longer than str2 */

		  default:
		    if (CMspace(str1))
		    {
			CMnext(str1);
			if (bignore)
			    continue;
			if (bpad)
			{
			    have_bpadded = TRUE;
			    continue;	/* blank = blank */
			}
			else
			{
			    return (1);	/* str1 longer than str2 */
			}
		    }
		    else
		    {
			if (    bpad
			    &&  CMcmpcase(str1, &blank) < 0
			   )
			    return (-1);
			else
			    return (1);	/* str1 longer than str2, or
					** ch1 greater than a blank.
					*/
		    }
		}
	    }
			
	    return (0);	    /* str1 = str2 */
        }
	CMnext(str1);
    }

    /* string 1 is out of characters */
    /* examine remainder of string 2 for any characters */
    while (str2 < endstr2)
    {
	switch (ch2 = *str2)
	{
	  case NULLCHAR:
	    str2++;
	    if (bignore)
		continue;
	    if (bpad)
		return (1);	/* blank > nullchar */
	    else
		return (-1);	/* str1 shorter than str2 */

	  default:
	    if (CMspace(str2))
	    {
		CMnext(str2);
		if (bignore)
		    continue;
		if (bpad)
		{
		    have_bpadded = TRUE;
		    continue;	/* blank = blank */
		}
		else
		{
		    return (-1);	/* str1 shorter than str2 */
		}
	    }
	    else
	    {
		if (   bpad
		    && CMcmpcase(str2, &blank) < 0
		   )
		    return (1);
		else
		    return (-1);	/* str1 shorter than str2, or
					** ch2 greater than a blank.
					*/
	    }
	}
    }
		
    return (0);	    /* str1 = str2 */
}


/*{
** Name: adu_like() - Compare two string data values using the LIKE operator
**		      pattern matching characters.
**
** Description:
**      This routine compares two string data values for equality using the
**      LIKE operator pattern matching characters '%' (match zero or more
**      arbitrary characters) and '_' (match exactly one arbitrary character).
**      These pattern matching characters only have special meaning in the
**      second data value, patdv.  If the first string is of datatype "c",
**	then blanks and null chars will be ignored.  Shorter strings will be
**	considered less than longer strings.  (Since LIKE only knows about
**	whether the two are "equal" or not, this should not matter.)
**
**	Examples:
**
**	             'Smith, Homer' < 'Smith,Homer' (if neither is "c")
**
**	             'Smith, Homer' = 'Smith,Homer' (if either is "c")
**
**	                     'abcd ' > 'abcd' (if neither is "c")
**
**	                      'abcd' = 'a%d'
**
**			      'abcd' = 'a_cd%'
**
**			      'abcd' = 'abcd%'
**
**			      'abcd ' > 'abc_' (if neither is "c")
**
**			      'abcd ' = 'abc_' (if either is "c")
**
**		      'a b      c d ' = 'abc_' (if either is "c")
**
**			      'abc% ' < 'abcd' (Note % treated literally in 1st)
**
**			      'abcd' < 'abcd_'
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      str_dv                          Ptr to DB_DATA_VALUE containing first
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 1st string (for "c" and "char")
**					or DB_TEXT_STRING holding first string
**					(for "text" and "varchar").
**      pat_dv                          Ptr to DB_DATA_VALUE containing second
**					string.  This is the pattern string.
**	    .db_datatype		Its datatype (assumed to be either
**					"char" or "varchar").
**	    .db_length			Its length.
**	    .db_data			Ptr to pattern string (for "c" and
**					"char") or DB_TEXT_STRING holding pat
**					string (for "text" and "varchar").
**      eptr                            Ptr to an u_char which will be used as
**					the escape character.  If this pointer
**					is NULL, then no escape character.
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
**	rcmp				Result of comparison, as follows:
**					    <0  if  str_dv1 < pat_dv2
**					    =0  if  str_dv1 = pat_dv2
**					    >0  if  str_dv1 > pat_dv2
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
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either str_dv's or pat_dv's datatype was
**					incompatible with this operation.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-oct-86 (thurston)
**          Initial creation.
**	19-nov-87 (thurston)
**	    A couple of bugs fixed, as well as a big semantic change:  No blank
**	    padding will ever be done now, even if CHAR or VARCHAR are involved.
**	    After reading the ANSI SQL standard, February 1986, for the
**	    <like predicate> (section 5.14, pages 40-41) for the millionth time,
**	    I think I finally understand it, and it means that no blank padding
**	    should ever be done.  This was also tested out in DB2, which agrees.
**	    Therefore,
**			    'abc' = 'abc '	... is TRUE
**	    while,
**			    'abc' LIKE 'abc '	... is FALSE	(except for 'c')
**	06-jul-88 (thurston)
**	    Added support for the escape character.
**	15-jul-88 (jrb)
**	    Changes for Kanji support.
**	24-Apr-89 (anton)
**	    Added local collation support
**	27-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**      22-jul-2005 (gnagn01)
**          Changes are made for proper varchar string termination
**          before initializing the string for collation
**	16-may-2007 (dougi)
**	    Add support for UTF8-enabled server.
*/

DB_STATUS
adu_like(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*str_dv,
DB_DATA_VALUE	*pat_dv,
u_char		*eptr,
i4		*rcmp)
{
    DB_STATUS	    db_stat;
    bool	    bignore;
    u_char	    *sptr;
    i4		    slen;
    u_char	    *ends;
    u_char	    *pptr;
    i4		    plen;
    u_char	    *endp;
    ADULcstate	    sst, pst, est;
    u_char          *tempsptr;
    u_char          *tempstr;
   
    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	return (adu_nvchr_utf8like(adf_scb, str_dv, pat_dv, eptr, rcmp));

    if (db_stat = adu_lenaddr(adf_scb, str_dv, &slen, (char **) &sptr))
	return (db_stat);
    
    if (db_stat = adu_lenaddr(adf_scb, pat_dv, &plen, (char **) &pptr))
	return (db_stat);
    endp = pptr + plen;

    bignore = (str_dv->db_datatype == DB_CHR_TYPE ||
	       pat_dv->db_datatype == DB_CHR_TYPE);

    if (adf_scb->adf_collation)
    {   
        tempsptr = sptr;
        tempstr  = (u_char *)MEreqmem((u_i2)0,(SIZE_TYPE)(slen+1 \
    			), TRUE, &db_stat);
          
	/*String is copied to temporary buffer and 
	terminated at the exact length so as to avoid wrong initialisation of 
	string in adulcstate structure  in adulstrinit which just makes the two         pointers (passed string pointer and adulcstate string pointer) point 
	the same location.*/

	STncpy((char *)tempstr,(char *)tempsptr,slen);
	tempstr[slen]='\0'; 
      	ends = tempstr + slen;       /*initiating end pointer for new terminated                                     string*/	
	adulstrinit((ADULTABLE *)adf_scb->adf_collation, tempstr, &sst);
	adulstrinit((ADULTABLE *)adf_scb->adf_collation, pptr, &pst);
	if (eptr)
	{
	    adulstrinit((ADULTABLE *)adf_scb->adf_collation, eptr, &est);
	    db_stat = ad0_llike(adf_scb, &sst, ends, &pst, endp, &est,
			     bignore, rcmp);
	}
	else
	{
	    db_stat = ad0_llike(adf_scb, &sst, ends, &pst, endp,
			     (ADULcstate *)NULL, bignore, rcmp);
	}

	MEfree((char *)tempstr);
	return(db_stat);
    }
    ends = sptr + slen;
    return (ad0_like(adf_scb, sptr, ends, pptr, endp, eptr, bignore, rcmp));
}


/*{
** Name: adu_collweight() - compute the collation weight of a non-Unicode
**			string value.
**
** Description:
**      For C, char, varchar, txt values with default collation, the result
**	is simply the source value. With non-default collation, it is the
**	sortkey returned by the alternate collation functions.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      str_dv                          Ptr to DB_DATA_VALUE containing first
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 1st string (for "c" and "char")
**					or DB_TEXT_STRING holding first string
**					(for "text" and "varchar").
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
**      res_dv                          Ptr to DB_DATA_VALUE containing result
**					string.  This is a varbyte field into
**					which the collation weight is placed.
**	    .db_datatype		Varbyte.
**	    .db_length			Source length or 2 times source
**					length for alternate collation.
**	    .db_data			Ptr to result collation weight vector.
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
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either str_dv's or pat_dv's datatype was
**					incompatible with this operation.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-dec-04 (inkdo01)
**	    Written.
**      12-May-2009 (hanal04) Bug 122031
**          For UTF8 installations non-unicode character strings should be 
**          coerced to unicode and passed to adu_ucollweight().
**	19-Nov-2010 (kiria01) SIR 124690
**	    Handle UCS_BASIC collation for UTF8 by not switching to UCS2.
*/

DB_STATUS
adu_collweight(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*str_dv,
	DB_DATA_VALUE	*res_dv)
{
    DB_STATUS	    db_stat, stat = E_DB_OK;
    u_char	    *sptr, *endsptr;
    i4		    slen;
    u_char	    *rptr;
    i4		    rlen;
    char            temps[ADF_UNI_TMPLEN];
    bool            memexpanded = FALSE;
    DB_DATA_VALUE   comp;
    i2              saved_uninorm_flag = adf_scb->adf_uninorm_flag;

    if (db_stat = adu_lenaddr(adf_scb, str_dv, &slen, (char **) &sptr))
	return (db_stat);
    
    if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
	str_dv->db_collID != DB_UCS_BASIC_COLL)
    {
        for(;;)
        {
            slen = slen * 2 + DB_CNTSIZE;
            if(slen > DB_MAXSTRING)
                slen = DB_MAXSTRING;

            if (slen > ADF_UNI_TMPLEN)
            {
#ifdef xDEBUG
                TRdisplay ("adu_collweight - MEreqmem call made for %d bytes\n", slen);
                /* Expensive MEreqmem() call. */
#endif
                sptr = MEreqmem(0, slen, TRUE, &db_stat);
                if (db_stat)
                    return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2,
                                        (i4) sizeof(db_stat), (i4 *)&db_stat));
                memexpanded = TRUE;
            }
            else sptr = &temps[0];

            /* Set up UCS2 DB_DATA_VALUEs. */
            comp.db_datatype = DB_NVCHR_TYPE;
            comp.db_length = slen;
            comp.db_prec = 0;
            comp.db_collID = 0;
            comp.db_data = sptr;

            adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
            if (db_stat = adu_nvchr_fromutf8(adf_scb, str_dv, &comp))
            {
                 adf_scb->adf_uninorm_flag = saved_uninorm_flag;
                 break;
            }
            adf_scb->adf_uninorm_flag = saved_uninorm_flag;

            db_stat = adu_ucollweight(adf_scb, &comp, res_dv);
            break;
        }
    }
    else if (adf_scb->adf_collation)
    {
	/* Do it the other way. */
    }
    else
    {
	if (slen > res_dv->db_length - DB_CNTSIZE)
	    slen = res_dv->db_length - DB_CNTSIZE;
	MEcopy(sptr, slen, 
	    (char *) &((DB_TEXT_STRING *)res_dv->db_data)->db_t_text);
	((DB_TEXT_STRING *)res_dv->db_data)->db_t_count = slen;
    }

    if(memexpanded)
        stat = MEfree (comp.db_data);
#ifdef xDEBUG
    if (stat)
	TRdisplay ("adu_collweight - MEfree status = %d \n", stat);
#endif

    return (db_stat);
}


/*
** Name: ad0_like()
**
**  Description:
**	Main routine for supporting SQL's like operator.
**
**  History:
**      01-oct-86 (thurston)
**	    Initial creation.
**	14-sep-87 (thurston)
**	    Fixed bug that caused a '_' at the end of the pattern to match TWO
**	    characters.
**	06-jul-88 (thurston)
**	    Added support for the escape character, and the new `[ ... ]' type
**	    pattern matching allowed by INGRES.  This involved re-writing the
**	    entire algorithm, including the new static routines ad0_lkqmatch(),
**	    ad0_lkpmatch() (replaces the former ad0_lkmatch()), and
**	    ad0_lklmatch().
**	15-jul-88 (jrb)
**	    Changes for Kanji support.
**	24-aug-88 (thurston)
**	    Commented out a `"should *NEVER* get here" return with error' since
**	    the Sun compiler produced a warning that you could never get there.
**	09-Jul-2008 (kiria01) b120328
**	    Correct end-of-pattern checks to avoid buffer overrun.
*/

static	DB_STATUS
ad0_like(
ADF_CB		    *adf_scb,
register u_char     *sptr,
u_char		    *ends,
register u_char     *pptr,
u_char		    *endp,
u_char		    *eptr,
bool		    bignore,
i4		    *rcmp)
{
    i4			cc;	/* the `character class' for pch */
    i4			stat;


    for (;;)	/* loop through pattern string */
    {
	int count = 0;
	DB_STATUS (*lkmatch)(ADF_CB*,u_char*,u_char*,u_char*,u_char*,
	                         u_char*,bool,i4,i4*) = ad0_lkpmatch;
	/*
	** Get the next character from the pattern string,
	** handling escape sequences, and ignoring blanks if required.
	** -----------------------------------------------------------
	*/
	if (pptr >= endp)
	{
	    /* end of pattern string */
	    cc = AD_CC6_EOS;
	}
	else
	{
	    if (eptr != NULL  &&  *pptr == *eptr)   /* currently *eptr must */
	    {					    /* be single-byte	    */
		/* we have an escape sequence */
		if (++pptr >= endp)
		{
		    /* ERROR:  escape at end of pattern string not allowed */
		    return (adu_error(adf_scb, E_AD1017_ESC_AT_ENDSTR, 0));
		}
		switch (*pptr)
		{
		  case AD_1LIKE_ONE:
		  case AD_2LIKE_ANY:
		  case '-':
		    cc = AD_CC0_NORMAL;
		    break;

		  case AD_3LIKE_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case AD_4LIKE_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  default:
		    if (*pptr == *eptr)
			cc = AD_CC0_NORMAL;
		    else
			/* ERROR:  illegal escape sequence */
			return (adu_error(adf_scb, E_AD1018_BAD_ESC_SEQ, 0));
		    break;
		}
	    }
	    else
	    {
		/* not an escape character */
		switch (*pptr)
		{
		  case AD_1LIKE_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case AD_2LIKE_ANY:
		    cc = AD_CC3_ANY;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
		    break;

		  case AD_3LIKE_LBRAC:
		  case AD_4LIKE_RBRAC:
		  default:
		    cc = AD_CC0_NORMAL;
		    break;
		}
	    }
	}

	if (	bignore
	    &&	cc == AD_CC0_NORMAL
	    &&	(CMspace(pptr)  ||  *pptr == NULLCHAR)
	   )
	{
	    CMnext(pptr);
	    continue;	/* ignore blanks and null chars for the C datatype */
	}


	/* Now we have the next pattern string character and its class */
	/* ----------------------------------------------------------- */

	switch (cc)
	{
	  case AD_CC0_NORMAL:
	  case AD_CC1_DASH:
	    for (;;)
	    {
		if (sptr >= ends)
		{
		    *rcmp = -1;	/* string is shorter than pattern */
		    return (E_DB_OK);
		}
		if (!bignore  ||  (!CMspace(sptr)  &&  *sptr != NULLCHAR))
		    break;
		    
		CMnext(sptr);
	    }

	    if ((stat = CMcmpcaselen(sptr, ends, pptr, endp)) != 0)
	    {
		*rcmp = stat;
		return (E_DB_OK);
	    }
	    break;

	  case AD_CC2_ONE:
	    count = 1;
	    lkmatch = ad0_lkqmatch;
	    /*FALLTHROUGH*/
	  case AD_CC3_ANY:
	    CMnext(pptr);
	    while (pptr < endp)
	    {
		if (*pptr == AD_1LIKE_ONE)
		    count++;
		else if (*pptr == AD_2LIKE_ANY)
		    lkmatch = ad0_lkpmatch;
		else if (!bignore || !CMspace(pptr) && *pptr != NULLCHAR)
		    break;
	        CMnext(pptr);
	    }
	    return lkmatch(adf_scb, sptr, ends, pptr, endp,
					eptr, bignore, count, rcmp);

	  case AD_CC4_LBRAC:
	    return (ad0_lklmatch(adf_scb, sptr, ends, pptr+1, endp,
					eptr, bignore, rcmp));

	  case AD_CC5_RBRAC:
	    /*
	    ** ERROR:  bad range specification.
	    */
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	  case AD_CC6_EOS:
	    /*
	    ** End of pattern string.  Check for rest of other string.
	    */
	    while (sptr < ends)
	    {
		if (!bignore  ||  (!CMspace(sptr)  &&  *sptr != NULLCHAR))
		{
		    *rcmp = 1;	    /* string is longer than pattern */
		    return (E_DB_OK);
		}
		CMnext(sptr);
	    }
	    *rcmp = 0;
	    return (E_DB_OK);

	  default:
	    /*
	    ** ERROR:  should *NEVER* get here.
	    */
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}
	CMnext(pptr);
    	CMnext(sptr);

    }

    /* Should *NEVER* get here 
    ****************************************************************************
    ** Commented out because of Sun compiler warning that you indeed          **
    ** cannot get here.							      **
    ** ---------------------------------------------------------------------- **
    **
    ** return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    **
    ***************************************************************************/
}


/*
**  Name: ad0_lkqmatch()
**
**  Description:
**	Process 'n' MATCH-ONE character for LIKE operator.
**
**  History:
**	06-jul-88 (thurston)
**	    Inital creation and coding.
**	18-jul-88 (jrb)
**	    Conversion for Kanji support.
*/

static	DB_STATUS
ad0_lkqmatch(
ADF_CB		    *adf_scb,
register u_char     *sptr,
u_char		    *ends,
register u_char     *pptr,
u_char		    *endp,
u_char		    *eptr,
bool		    bignore,
i4		    n,
i4		    *rcmp)
{
    while (sptr < ends)
    {
	if (bignore && (CMspace(sptr) || *sptr == NULLCHAR))
	{
	    CMnext(sptr);
	}
	else
	{
	    CMnext(sptr);
	    if (!--n)
	    {
		return (ad0_like(adf_scb, sptr, ends, pptr, endp, eptr,
								bignore, rcmp));
	    }
	}
    }
    *rcmp = -1;	/* string is shorter than pattern */
    return (E_DB_OK);
}


/*
**  Name: ad0_lkpmatch() 
**
**  Description:
**	Process 'n' MATCH-ONE then MATCH-ANY character for LIKE operator.
**
**  History:
**	06-jul-88 (thurston)
**	    Inital creation and coding.
**	18-jul-88 (jrb)
**	    Conversion for Kanji support.
**	01-May-2008 (kiria01) b120328
**	    Correct end of '_' run handling.
*/

static	DB_STATUS
ad0_lkpmatch(
ADF_CB		    *adf_scb,
register u_char     *sptr,
u_char		    *ends,
register u_char     *pptr,
u_char		    *endp,
u_char		    *eptr,
bool		    bignore,
i4		    n,
i4		    *rcmp)
{
    DB_STATUS		db_stat;

    while (n && sptr < ends)
    {
	if (!bignore || !CMspace(sptr) && *sptr != NULLCHAR)
	    n--;
	CMnext(sptr);
    }

    if (n > 0)
    {
	/* String too short for pattern */
	*rcmp = -1;
	return (E_DB_OK);
    }

    if (pptr >= endp)
    {
	/* Last relevant char in pattern was `MATCH-ANY' so we have a match. */
	*rcmp = 0;
	return (E_DB_OK);
    }


    while (sptr <= ends)	    /* must be `<=', not just `<' */
    {
	if (db_stat = ad0_like(adf_scb, sptr, ends, pptr, endp,
				    eptr, bignore, rcmp)
	   )
	    return (db_stat);

	if (*rcmp == 0)
	    return (E_DB_OK);	/* match */

	CMnext(sptr);
    }

    /* Finished with string and no match found ... call it `<' by convention. */
    *rcmp = -1;
    return (E_DB_OK);
}


/*
**  Name: ad0_lklmatch() 
**
**  Description:
**	Process a range-sequence for LIKE operator.
**
**  History:
**	06-jul-88 (thurston)
**	    Inital creation and coding.
**	18-jul-88 (jrb)
**	    Conversion for Kanji support.
**	24-aug-88 (thurston)
**	    Commented out a `"should *NEVER* get here" return with error' since
**	    the Sun compiler produced a warning that you could never get there.
**	04-mar-91 (jrb)
**	    Bug 35775.  We weren't skipping blanks in sptr when bignore was
**	    TRUE.
*/

static	DB_STATUS
ad0_lklmatch(
ADF_CB		    *adf_scb,
register u_char     *sptr,
u_char		    *ends,
register u_char     *pptr,
u_char		    *endp,
u_char		    *eptr,
bool		    bignore,
i4		    *rcmp)
{
    u_char		*savep;
    i4			cc;
    i4			cur_state = AD_S1_IN_RANGE_DASH_NORM;
    bool		empty_range = TRUE;
    bool		match_found = FALSE;


    for (;;)
    {
	/*
	** Get the next character from the pattern string,
	** handling escape sequences, and ignoring blanks if required.
	** -----------------------------------------------------------
	*/
	if (pptr >= endp)
	{
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
	}
	else
	{
	    if (eptr != NULL  &&  *pptr == *eptr)
	    {
		/* we have an escape sequence */
		CMnext(pptr);
		if (pptr >= endp)
		{
		    /* ERROR:  escape at end of pattern string not allowed */
		    return (adu_error(adf_scb, E_AD1017_ESC_AT_ENDSTR, 0));
		}
		switch (*pptr)
		{
		  case AD_1LIKE_ONE:
		  case AD_2LIKE_ANY:
		  case '-':
		    cc = AD_CC0_NORMAL;
		    break;

		  case AD_3LIKE_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case AD_4LIKE_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  default:
		    if (*pptr == *eptr)
			cc = AD_CC0_NORMAL;
		    else
			/* ERROR:  illegal escape sequence */
			return (adu_error(adf_scb, E_AD1018_BAD_ESC_SEQ, 0));
		    break;
		}
	    }
	    else
	    {
		/* not an escape character */
		switch (*pptr)
		{
		  case AD_1LIKE_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case AD_2LIKE_ANY:
		    cc = AD_CC3_ANY;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
		    break;

		  case AD_3LIKE_LBRAC:
		  case AD_4LIKE_RBRAC:
		  default:
		    cc = AD_CC0_NORMAL;
		    break;
		}
	    }
	}

	if (cc == AD_CC6_EOS)
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	if (cc == AD_CC2_ONE  ||  cc == AD_CC3_ANY  ||  cc == AD_CC4_LBRAC)
	    return (adu_error(adf_scb, E_AD1016_PMCHARS_IN_RANGE, 0));

	if (	bignore
	    &&	cc == AD_CC0_NORMAL
	    &&	(CMspace(pptr)  ||  *pptr == NULLCHAR)
	   )
	{
	    CMnext(pptr);
	    continue;	/* ignore blanks and null chars for the C datatype */
	}

	/* Skip blanks in sptr if C datatype is being used */
	while (sptr < ends)
	{
	    if (bignore  &&  (CMspace(sptr)  ||  *sptr == NULLCHAR))
		CMnext(sptr);
	    else
		break;
	}

	/*
	** Now, we have the next pattern character.  Switch on the current
	** state, then do something depending on what character class
	** the next pattern character falls in.  Note, that we should be
	** guaranteed at this point to have either `NORMAL', `DASH', or `RBRAC'.
	** All other cases have been handled above as error conditions.
	*/

	switch (cur_state)
	{
	  case AD_S1_IN_RANGE_DASH_NORM:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
	      case AD_CC1_DASH:
		empty_range = FALSE;
		savep = pptr;
		cur_state = AD_S2_IN_RANGE_DASH_IS_OK;
	        break;

	      case AD_CC5_RBRAC:
		/* end of the range spec, range *MUST* have been empty. */
		return (ad0_like(adf_scb, sptr, ends, pptr+1, endp,
					    eptr, bignore, rcmp));

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  case AD_S2_IN_RANGE_DASH_IS_OK:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if (sptr < ends  &&  CMcmpcase(sptr, savep) == 0)
		    match_found = TRUE;
		savep = pptr;
		/* cur_state remains AD_S2_IN_RANGE_DASH_IS_OK */
	        break;

	      case AD_CC1_DASH:
		/* do nothing, but change cur_state */
		cur_state = AD_S3_IN_RANGE_AFTER_DASH;
	        break;

	      case AD_CC5_RBRAC:
		/* end of the range spec, and we have a saved char so range */
		/* was not empty.					    */
		if (sptr < ends  &&  CMcmpcase(sptr, savep) == 0)
		    match_found = TRUE;

		if (match_found)
		{
		    CMnext(sptr);
		    return (ad0_like(adf_scb, sptr, ends, pptr+1, endp,
					    eptr, bignore, rcmp));
		}
		*rcmp = -1;	/* if string not in range, call it < pat */
		return (E_DB_OK);

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  case AD_S3_IN_RANGE_AFTER_DASH:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if (CMcmpcase(savep, pptr) <= 0)
		{
		    if (    sptr < ends
			&&  CMcmpcase(sptr, savep) >= 0
		        &&  CMcmpcase(sptr, pptr) <= 0
		       )
		    {
			match_found = TRUE;
		    }
		    cur_state = AD_S4_IN_RANGE_DASH_NOT_OK;
		    break;
		}
		/* fall through to BAD-RANGE ... x-y, where x > y */

	      case AD_CC1_DASH:
	      case AD_CC5_RBRAC:
		return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  case AD_S4_IN_RANGE_DASH_NOT_OK:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		savep = pptr;
		cur_state = AD_S2_IN_RANGE_DASH_IS_OK;
	        break;

	      case AD_CC1_DASH:
		return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	      case AD_CC5_RBRAC:
		/* end of the range spec, no saved char, and range was not  */
		/* empty.						    */
		if (match_found)
		{
		    CMnext(sptr);
		    return (ad0_like(adf_scb, sptr, ends, pptr+1, endp,
					    eptr, bignore, rcmp));
		}
		*rcmp = -1;	/* if string not in range, call it < pat */
		return (E_DB_OK);

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  default:
	    /* should *NEVER* get here */
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}
	CMnext(pptr);
    }

    /* should *NEVER* get here */
    /**************************************************************************/
    /* Commented out because of Sun compiler warning that you indeed          */
    /* cannot get here.							      */
    /* ---------------------------------------------------------------------- */
    /*
    /* return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    /*
    /**************************************************************************/
}
