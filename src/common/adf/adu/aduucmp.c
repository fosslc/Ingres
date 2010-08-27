/*
** Copyright (c) 2001, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adulcol.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduucol.h>

/*
** Name: ADUUCMP.C - Collation modified string compare for Unicode
**
** Description:
**	This file contains the implimentation of the Unicode standard collation
**	algorithmn The algorithmn can be found at:
**	
**	    http://www.unicode.org/unicode/reports/tr10/
**
**	This file contains the following external functions:
**
**	    aduucmp - Main entry point, compare two Unicode strings
**	    aduunorm - normalize a unicode string (normal form D)
**	    adu_ucollweight - returns varbyte collation weight (cortkey)
**		of source value.
**
** History:
**	08-mar-2001 (stephenb)
**	    Initial coding
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**          Redefined aduucmp() to return STATUS and comparison result
**          Also fixed CE and sort key lists memory leak in aduucmp()
**	25-Sep-2002 (jenjo02)
**	    Restructured to accomodate "LIKE" predicate processing.
**	6-dec-04 (inkdo01)
**	    Add support for case insensitive ordering.
**	11-jan-05 (inkdo01)
**	    Change CaseInsensitive to 8 (4 was already in use).
**	29-Jul-2005 (gupsh01)
**	    Fixed the case for like pattern matching where the 
**	    input string also contains the Like pattern.
**	12-Aug-2005 (gupsh01)
**	    Fixed the case for like pattern matching where the 
**	    pattern string may contain a '-' character. 
**	08-Sep-2005 (fanra01)
**	    Bug 115175
**	    A predicate comparing a NULL value used with a Unicode key column
**	    eventually calls adu_umakeskey with a string containing the maximum
**	    code point values of 0xFFFF which causes an unhandled exception within
**	    MakeCE.
**	17-May-2007 (gupsh01)
**	    Added support for Unicode French collation.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
**	27-Apr-2010 (kiria01) b123632
**	    combent entries in the in-memory tables were being skipped if
**	    they were the last in the list. This had a knock on effect in
**	    adu_pat_MakeCEchar where not executing this loop at all could
**	    result in an infinite loop with the source string pointer never
**	    advancing.
**      15-Jun-2010 (horda03) b123926
**          ifnull(lnvchar,lnvchar) calls adu_utf8_unorm with the LNVCHAR
**          datatype, so handle this type.
**      18-Jun-2010 (horda03) b123926
**          After testing in main found that the calls to adu_long_unorm
**          supplied the parameters in the wrong order.
**      21-Jun-2010 (horda03) b123926
**          Because adu_unorm() and adu_utf8_unorm() are also called via 
**          adu_lo_filter() change parameter order.
*/

/* forward refs */
static	DB_STATUS
decompose(ADF_CB	*adf_scb,
	  u_i2		ichar,
	  u_i2		*ochar,
	  u_i2		*ochar_end,
	  i4		*dlen);

/* Option directives for MakeCE */
#define	LikeCE		0x0001
#define	Sortkeys	0x0002
#define	CaseInsensitive	0x0008
#define LikePat		0x0010
#define FrenchCollation 0x0020
#define QuelPat		0x0040

static DB_STATUS MakeCE(
       ADF_CB	*adf_scb,
       u_i2	*bst,
       u_i2	*est,
       u_i2	*esc,
       u_i2	**listp,
       i4	opts);

/*{
** Name: aduucmp() - compare two unicode strings
**
** Description:
**	Compares two unicode strings using the unicode collation algorithmn
**	strings are assumed to be already normalized
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block
**	flags - to indicate comparison semantics
**	bsa - start of 1st string
**	esa - end of 1st string
**	bsb - start of  2nd string
**	esb - end of 2nd string
**
** Outputs:
**     rcmp
**	    USTRING_LESS - a is less than b
**	    USTRING_EQUAL - strings are equal
**	    USTRING_GREATER - a is greater than b
**
**	Returns:
**	    USTRING_BAD_TAB - Collation element lookup for string was bad
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-mar-2001 (stephenb)
**	    Initial implementation
**	13-apr-2001 (stephenb)
**	    Must ignore trailing blanks to make nchar and nvarchar compare
**	6-jun-2001 (stephenb)
**	    Make correction to offset calculation when creating the sort keys
**	    from the collation element list.
**	26-jun-2001 (stephenb)
**	    Fix sort key comparison to take into account different length
**	    strings.
**	3-jul-2001 (stephenb)
**	    Fix comparison to take into acount collapsed sortkeys which
**	    occur when the sring contains variable weighted characters
**	    (such as embeded spaces).
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**          Redefined aduucmp() to return STATUS and comparison result
**          Also fixed CE and sort key lists memory leak in aduucmp()
**	13-aug-2002 (gupsh01)
**	    Added check for the esb not to go beyond the bsb when removing
**          blank spaces from the input strings. 
**	25-Sep-2002 (jenjo02)
**	    Restructured to accomodate "LIKE" predicate processing.
**	    Moved CE/sortkey list production into MakeCE static
**	    function.
**	30-Jan-2002 (jenjo02)
**	    Correctly MEcmp length as differences in PTR,
**	    not u_i2*. Added xDEBUG code to display operands,
**	    sortkeys, and result.
**	14-Sep-2007 (gupsh01)
**	    Fixed nchar comparison of two values one which is padded with 
**	    blanks and the other padded with nulls, do not compare equal.
**	26-Mar-2008 (gupsh01)
**	    Add support for comparing with the ADUL_SKIPBLANK option,
**	    which will ignore blanks when comparing C types in a UTF8
**	    installation.
**	31-Mar-2009 (kiriai01) 121875
**	    Protect against odd buffer lengths as the code assumes integral
**	    UCS2s.
**	27-Oct-2009 (wanfr01) b122799
**	    Use a local variable for unicode manipulation rather than
**	    allocating a buffer every time
*/
DB_STATUS
aduucmp(
	ADF_CB		*adf_scb,
	i4		flags,
	u_i2		*input_bsa,
	u_i2		*input_esa,
	u_i2		*input_bsb,
	u_i2		*input_esb,
	i4		*rcmp,
	i4		collID)
{
    u_i2		*keylista, *keya, *enda;
    u_i2		*keylistb, *keyb, *endb;
    STATUS		cl_stat;
    STATUS		stat;
    DB_STATUS		status;
    PTR			mem;
    i2			atr_len;
    u_i2		temp_buf[1024];
    u_i2		temp_buf_used = 0;
    u_i2		*bsa_buf = (u_i2 *)0;
    u_i2		*bsb_buf = (u_i2 *)0;
    u_i2		*bsa;
    u_i2		*esa;
    u_i2		*bsb;
    u_i2		*esb;
    u_i2 		*inptr; 
    u_i2 		*bufptr; 
    i4			cnt;
    UCS2		local_lista[ADF_UNI_TMPLEN_UCS2];
    UCS2		local_listb[ADF_UNI_TMPLEN_UCS2];

    keylista = local_lista;
    keylistb = local_listb;

    if ((input_bsa == input_bsb) && (input_esa == input_esb))
    {
        /* i.e. strings are the same */
	*rcmp = USTRING_EQUAL;
	return (E_DB_OK);
    }

    if ( (input_esa - input_bsa) == (input_esb - input_bsb) &&
	(MEcmp((PTR)input_bsa, (PTR)input_bsb, 
		((PTR)input_esa-(PTR)input_bsa))) == 0 )
    {
	/* strings are the same length and equal */
	*rcmp = USTRING_EQUAL;
	return(E_DB_OK);
    }

    /*
    ** UCS2 strings must be aligned, even if length is zero!
    ** If buffer is necessary for alignment, allocate atr_len + 1,
    ** in case atr_len = 0
    */
    if ((flags & ADUL_SKIPBLANK) ||
        (ME_ALIGN_MACRO((PTR)input_bsa, sizeof(u_i2)) != (PTR)input_bsa))
    {
	atr_len = ((char *)input_esa - (char *)input_bsa); /* bytes */
	if (atr_len > sizeof(temp_buf)) 
	{
	    bsa = (u_i2 *)MEreqmem(0, atr_len + 1, FALSE, &cl_stat);
	    bsa_buf = bsa;
	    if (bsa == NULL || cl_stat)
		return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(cl_stat), (i4 *)&cl_stat));
	}
	else
	{
	    bsa = (u_i2 *)&temp_buf;
	    temp_buf_used = atr_len;
	}
	  
	if (atr_len)
	{
    	  if (flags & ADUL_SKIPBLANK)
    	  {
		cnt = atr_len / sizeof(UCS2);
		inptr = input_bsa;
		bufptr = bsa;

		/* Squeeze out blanks from the strings. */
		while (inptr < input_esa) 
		{
	  	  if (*inptr == U_BLANK) 
		  {
		    inptr++; cnt--;
		  }
		  else
		    *bufptr++ = *inptr++;
		}
	        esa = bsa + cnt;
	    }
	    else
	    {
	      MEcopy(input_bsa, atr_len, (char *)bsa);
	      esa = (u_i2 *)((char *)bsa + atr_len);
	    }
	}
	else
	  esa = bsa;
    }
    else
    {
	bsa = input_bsa;
	esa = input_esa;
    }

    if ((flags & ADUL_SKIPBLANK) ||
        (ME_ALIGN_MACRO((PTR)input_bsb, sizeof(u_i2)) != (PTR)input_bsb))
    {
	atr_len = ((char *)input_esb - (char *)input_bsb); /* bytes */
	if (atr_len > sizeof(temp_buf) - temp_buf_used)
	{
	    bsb = (u_i2 *)MEreqmem(0, atr_len + 1, FALSE, &cl_stat);
	    bsb_buf = bsb;
	    if (bsb == NULL || cl_stat != OK)
	    {
		if (bsa_buf)
		    MEfree((char *)bsa_buf);
		return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2,
				(i4) sizeof(cl_stat), (i4 *)&cl_stat));
	    }
	}
	else
	{
	    bsb = (u_i2 *)((char *)&temp_buf + temp_buf_used);
	}
	if (atr_len)
	{
    	  if (flags & ADUL_SKIPBLANK)
    	  {
		/* Squeeze out blanks from the strings. */
		cnt = atr_len / sizeof(UCS2);
		inptr = input_bsb;
		bufptr = bsb;
		while (inptr < input_esb) 
		{
	  	  if (*inptr == U_BLANK) 
		  {
		    inptr++; cnt--;
		  }
		  else
		    *bufptr++ = *inptr++;
		}
		esb = bsb + cnt;
	  }
	  else
	  {
	      MEcopy(input_bsb, atr_len, (char *)bsb);
	      esb = (u_i2 *)((char *)bsb + atr_len);
	  }
	}
	else
	  esb = bsb;
    }
    else
    {
	bsb = input_bsb;
	esb = input_esb;
    }

    /* We know that the start is aligned to a word boundary but we are susceptible
    ** to an abend or stack overflow if the tail pointer is not also aligned. In
    ** otherwords, we better be passed a whole number of UCS2 in each buffer. */
    if ((((char*)esa-(char*)bsa) | ((char*)esb-(char*)bsb)) & (sizeof(UCS2)-1))
    {
	return adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "bad length");
    }

    if (flags & ADUL_BLANKPAD)
    {
	/* ignore trailing blanks */
	for (;esa > bsa && ((*(esa-1) == U_BLANK) || (*(esa-1) == U_NULL)); esa--);
	for (;esb > bsb && ((*(esb-1) == U_BLANK) || (*(esb-1) == U_NULL)); esb--);
    }

    status = E_DB_OK;

    /* Check for totally blank string(s) */
    if ( esa == bsa || esb == bsb )
	*rcmp = (esa - bsa) - (esb - bsb);
    /* produce sort keys, one for string a, one for string b */
    else if ( (status = adu_umakeskey(adf_scb, bsa, esa, &keylista, 
				(i2)collID)) == E_DB_OK
	   && (status = adu_umakeskey(adf_scb, bsb, esb, &keylistb, 
				(i2)collID)) == E_DB_OK )

    {
	i4	eoa = 0, eob = 0;

	/* First element in list is the count of keys which follow */
	keya = keylista+1;
	enda = keya + keylista[0];
	keyb = keylistb+1;
	endb = keyb + keylistb[0];

	/* compare the sort keys */
	while ( !eoa && !eob && *keya == *keyb )
	{
	    if ( ++keya >= enda )
	       eoa++;
	    if ( ++keyb >= endb )
	       eob++;
	}

	if ( eoa || eob )
	    /* All keys are equal, or one list is shorter */
	    *rcmp = eob - eoa;
	else
	    /* key mismatch */
	    *rcmp = *keya - *keyb;

#ifdef xDEBUG
	/* Display the characters, sortkeys, and result */
	{
	    char	*tbsa = (char*)bsa;
	    char	*tbsb = (char*)bsb;
	    u_i2	*tkeya = keylista+1;
	    u_i2	*tkeyb = keylistb+1;
	    i4		x = 0;

	    TRdisplay("BSA ");
	    while ( ++tbsa < (char*)esa )
	    {
		TRdisplay("%c", *(tbsa)++);
	    }
	    TRdisplay("\tKEYA ");
	    while ( tkeya != enda )
	    {
		MEcopy((char*)tkeya, 2, (char*)&x+2);
		TRdisplay("%x ", x);
		tkeya++;
	    }
	    TRdisplay("\nBSB ");
	    while ( ++tbsb < (char*)esb )
	    {
		TRdisplay("%c", *(tbsb)++);
	    }
	    TRdisplay("\tKEYB ");
	    while ( tkeyb != endb )
	    {
		MEcopy((char*)tkeyb, 2, (char*)&x+2);
		TRdisplay("%x ", x);
		tkeyb++;
	    }
	    TRdisplay("\nrcmp %d\n", *rcmp);
	}
#endif /* xDEBUG */
    }

    /* Free allocated memory if it was used instead of local buffers */
    if ( keylista != local_lista )
	MEfree((char *)keylista);
    if ( keylistb != local_listb )
	MEfree((char *)keylistb);
    if ( bsa_buf )
	MEfree((char *)bsa_buf);
    if ( bsb_buf )
	MEfree((char *)bsb_buf);

    return(status);
}

/*
** Name: adu_umakeskey - Produce a sortkey list from a
**			 normalized string.
**
** Description:
**	Makes a sortkey list per section 4.3 of the Unicode
**	Collation Algorithm.
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block
**	bst	  - start of normalized string
**	est	  - end of string
**	collID	  - collation ID
**
** Outputs:
**	listp     - points to the produced list. The caller
**		    is responsible for MEfree-ing the list.
**
** Returns:
**	DB_STATUS
**
** History:
**	25-Sep-2002 (jenjo02)
**	    Invented.
**	17-May-2002 (gupsh01)
**	    Added French Unicode Collation support.
*/
DB_STATUS
adu_umakeskey(
       ADF_CB	*adf_scb,
       u_i2	*bst,
       u_i2	*est,
       u_i2	**listp,
       i2	collID)
{
    i4		opts = Sortkeys;
    ADUUCETAB   *cetable = (ADUUCETAB *)adf_scb->adf_ucollation;

    if (collID == DB_UNICODE_CASEINSENSITIVE_COLL)
	opts |= CaseInsensitive;

    if (cetable == NULL)
       return (adu_error(adf_scb, E_AD5012_UCETAB_NOT_EXISTS, 0));
    else
    {
	if ((bst && cetable[*bst].flags & CE_COLLATEFRENCH) ||
              (collID == DB_UNICODE_FRENCH_COLL))
	  opts |= FrenchCollation;
    }

    return( MakeCE(adf_scb, bst, est, (u_i2*)0, listp, opts) );
}

/*
** Name: adu_umakelces - Produce a collation element list
**			 for the "string" of a LIKE
**			 predicate.
**
** Description:
**	Makes a parsable CE list for a LIKE string.
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block
**	bst	  - start of normalized string
**	est	  - end of string
**	collID	  - collation ID of comaparand
**
** Outputs:
**	listp     - points to the produced list. The caller
**		    is responsible for MEfree-ing the list.
**
** Returns:
**	DB_STATUS
**
** History:
**	25-Sep-2002 (jenjo02)
**	    Invented.
**	4-feb-05 (inkdo01)
**	    Added collID parm for collation ID support.
**	17-May-2007 (gupsh01)
**	    Added support for French Unicode Collation.
*/
DB_STATUS
adu_umakelces(
       ADF_CB	*adf_scb,
       u_i2	*bst,
       u_i2	*est,
       u_i2	**listp,
       i4	collID)
{
    i4		opts = LikeCE;
    ADUUCETAB   *cetable = (ADUUCETAB *)adf_scb->adf_ucollation;

    if (collID == DB_UNICODE_CASEINSENSITIVE_COLL)
	opts |= CaseInsensitive;

    if ((bst && cetable[*bst].flags & CE_COLLATEFRENCH) ||
         (collID == DB_UNICODE_FRENCH_COLL))
	  opts |= FrenchCollation;

    return( MakeCE(adf_scb, bst, est, (u_i2*)0, listp, opts) );
}

/*
** Name: adu_umakelpat - Produce a collation element list
**			 for the "pattern" of a LIKE
**			 predicate.
**
** Description:
**	Makes a parsable CE list for a LIKE pattern.
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block
**	bst	  - start of normalized string
**	est	  - end of string
**	esc	  - pointer to a u_i2 escape character, if any.
**	collID	  - collation ID of comaparand
**
** Outputs:
**	listp     - points to the produced list. The caller
**		    is responsible for MEfree-ing the list.
**
** Returns:
**	DB_STATUS
**
** History:
**	25-Sep-2002 (jenjo02)
**	    Invented.
**	4-feb-05 (inkdo01)
**	    Added collID parm for collation ID support.
**	29-jul-2005 (gupsh01)
**	    Fixed the case where for like pattern matching
**	    where pattern character is present in the input
**	    string. Set the opts correctly.
**	17-May-2007 (gupsh01)
**	    Added support for French Unicode Collation.
*/
DB_STATUS
adu_umakelpat(
       ADF_CB	*adf_scb,
       u_i2	*bst,
       u_i2	*est,
       u_i2	*esc,
       u_i2	**listp,
       i4	collID)
{
    i4		opts = LikeCE | LikePat;
    ADUUCETAB   *cetable = (ADUUCETAB *)adf_scb->adf_ucollation;

    if (collID == DB_UNICODE_CASEINSENSITIVE_COLL)
	opts |= CaseInsensitive;

    if ((bst && cetable[*bst].flags & CE_COLLATEFRENCH) ||
         (collID == DB_UNICODE_FRENCH_COLL))
	  opts |= FrenchCollation;

    return( MakeCE(adf_scb, bst, est, esc, listp, opts) );
}

/*
** Name: adu_umakeqpat - Produce a collation element list
**			 for the "pattern" of a Quel  
**			 predicate.
**
** Description:
**	Makes a parsable CE list for a LIKE pattern.
**	sets QuelPat flag in opts to MakeCE.
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block
**	bst	  - start of normalized string
**	est	  - end of string
**	esc	  - pointer to a u_i2 escape character, if any.
**	collID	  - collation ID of comaparand
**
** Outputs:
**	listp     - points to the produced list. The caller
**		    is responsible for MEfree-ing the list.
**
** Returns:
**	DB_STATUS
**
** History:
**	06-Nov-2007 (gupsh01)
**	    Written.
*/
adu_umakeqpat(
       ADF_CB	*adf_scb,
       u_i2	*bst,
       u_i2	*est,
       u_i2	*esc,
       u_i2	**listp,
       i4	collID)
{
    i4		opts = LikeCE | LikePat | QuelPat;
    ADUUCETAB   *cetable = (ADUUCETAB *)adf_scb->adf_ucollation;

    if (collID == DB_UNICODE_CASEINSENSITIVE_COLL)
	opts |= CaseInsensitive;

    if ((bst && cetable[*bst].flags & CE_COLLATEFRENCH) ||
         (collID == DB_UNICODE_FRENCH_COLL))
	  opts |= FrenchCollation;

    return( MakeCE(adf_scb, bst, est, esc, listp, opts) );
}
/*
** Name: MakeCEcaseFold - fold uppercase values of tertiary weight
**	into corresponding lower case values
**
** Description:
**	Examines tertiary weight value for upper case and replaces
**	with corresponding lower case values.
**
** Inputs:
**      tertiary	- ptr to tertiary value of current CE entry
**
** Outputs:
**	tertiary	- ptr to possibly folded tertiary valueler
**
** Returns:
**	NONE
**
** History:
**	13-sep-05 (inkdo01)
**	    Written for improved (?) case insensitive Unicode sorts.
*/
static VOID
MakeCEcaseFold(
	u_i2	*tertiary)
{

    /* Just switch on tertiary value - replacing uppers with 
    ** lowers, as necessary. Note that the annotations for each 
    ** of the case's below come from version 4.1.0 of the Unicode
    ** Collation Algorithm. */
    switch (*tertiary) {

      case 0x0008:	/* most alphabetic upper cases */
	*tertiary = 0x0002;	/* lower case */
	break;

      case 0x0009:	/* uppercase <wide> */
	*tertiary = 0x0003;	/* lowercase <wide> */
	break;

      case 0x000a:	/* uppercase <compatibility> */
	*tertiary = 0x0004;	/* lowercase <compatibility> */
	break;

      case 0x000b:	/* uppercase <font> */
	*tertiary = 0x0005;	/* lowercase <font> */
	break;

      case 0x000c:	/* uppercase <circle> */
	*tertiary = 0x0006;	/* lowercase <circle> */
	break;

      case 0x0014:	/* lowercase <super> */
      case 0x0015:	/* lowercase <sub> */
      case 0x001c:	/* lowercase <square> */
	/* <super>, <sub> and <square> all have only one uppercase
	** equivalent (for reasons unknown). */
	*tertiary = 0x001d;
	break;

    }

}

/*
** Name: MakeCE - make a collation element list from a string
**
** Description:
** 	This function follows section 4.2 of the algorithmn to create
**	a collation element list from a normalized input string
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block with
**		adf_scb->adf_ucollation - Unicode collation table
**
**	bst	  - start of normalized string
**	est	  - end of string
**	esc	  - pointer to u_i2 escape character, if any.
**		    Meaningful only when producing a LikePat list.
**			NOTE: With LikePat option set this parameter
**			shows whether we are being called from a
**			Quel or SQL context and if the latter, whether
**			and escape character is active:
**			    NULL - 	Use Quel LIKE syntax
**			    Not NULL -	Use SQL LIKE syntax
**				In this case this will be the address of
**				a unicode string to use as the escape
**				character. If this is an empty string
**				then no escape char will be expected
**
**	opts	  - The type of list to produce:
**
**			Sortkeys: Form a sort key from the
**				  collation elements.
**			LikeCE:   Form a collation element list
**				  for use by LIKE. In this form,
**				  the CE list for each starter
**				  character in "string" is preceded
**				  by the special marker "ULIKE_NORM"
**				  enabling LIKE to parse the CE
**				  list and do character-by-character
**				  comparisons.
**			LikePat:  Form a collation element list
**				  for a LIKE pattern string. In this
**				  form the Escape character is
**				  replaced by a "ULIKE_ESC" marker,
**				  the special LIKE characters
**				  preceded by a "ULIKE_PAT"
**				  marker, and all other starter
**				  characters preceded with a
**				  "ULIKE_NORM" marker.
**
**				  Collation elements for LIKE
**				  strings are produced without
**				  regard for "ignorability" or
**				  variable weighting.
**			CaseInsensitive: Form case insensitive sort key.
** Outputs:
**	listp     - points to the produced list. The caller
**		    is responsible for MEfree-ing the list.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**		
**	possible errors:
**			E_AD2042_MEMALLOC_FAIL
**			E_AD1017_ESC_AT_ENDSTR
**			E_AD1018_BAD_ESC_SEQ
**			E_AD1014_BAD_VALUE_FOR_DT
**
** History:
**	23-mar-2001 (stephenb)
**	    created
**	2-apr-2001 (stephenb)
**	    Fix ignorability
**	16-jan-2001 (gusph01)
**	    Fixed the combining character case. Uninitialized 
**	    variable match caused incorrect matching of the 
**	    characters from the table. 
**	25-Sep-2002 (jenjo02)
**	    Modified to accomodate LIKE CE list production.
**	6-dec-04 (inkdo01)
**	    Add support of CaseInsensitive flag for case insensitive 
**	    collation.
**	20-dec-04 (inkdo01)
**	    Added a bit of code in support of collation_weight() scalar
**	    function.
**	11-july-05 (inkdo01)
**	    Disallow collation table access if db isn't Unicode enabled.
**	22-july-2005 (gupsh01)
**	    Fixed this routine so that the ignore_level is not ignored 
**	    completely but weighed in as zero weight. This fixes the
**	    Hiragana and Katakana sorting which would collate as same
**	    for nocase collation.
**	12-Aug-2005 (gupsh01)
**	    Fixed the case for like pattern matching where the 
**	    pattern string may contain a '-' character. 
**	08-Sep-2005 (fanra01)
**	    Synthesis of collation elements for a code point that is not
**	    represented within the collation table includes an additional
**	    collation element in the instance where a collation level should be
**	    ignored.  The following collation elements will be corrupted and the
**	    storage for the collation list could be exceeded resulting in a
**	    stack corruption.
**	13-sep-05 (inkdo01)
**	    Remove ill considered use of tertiary weight dropping to
**	    perform case insensitive sort. This led to several problems.
**	    New approach will be simply to fold certain tertiary weight
**	    values for case insensitivity.
**	17-Oct-2006 (kiria01) b116858
**	    SQL calls to LIKE use escaped brackets for range delimiters which
**	    meant that the dash got ignored. As Quel and SQL have differing
**	    needs here the code uses the 'esc' parameter to determine the
**	    context: Quel never passes it and SQL always does. If SQL is presented
**	    with an unescaped LIKE operator, it passes a pointer to a U_NULL character.
**	01-Feb-2007 (kiria01) b117544
**	    In MakeCE, coalesce runs of wildcard characters transforming '%_' into the
**	    the equivalent '_%' and '%%' into '%'. This substancially reduces the
**	    otherwise exponential work required for multi-wild card patterns.
**	17-May-2007 (gupsh01)
**	    Added support for French Unicode Collation.
**	17-Sep-2007 (gupsh01)
**	    Make sure that we do not run over the input buffer limits.
**	08-Oct-2007 (gupsh01)
**	    Account for quel pattern matching characters.
**	19-Oct-2007 (gupsh01)
**	    More changes to enable quel pattern matching. Now separate out Quel and SQL 
**	    pattern matching.
**	08-Jan-2008 (kiria01) b119709
**	    In a previous change to MakeCE the buffer length check was removed due
**	    to the space having been calculated algorithmicly and in theory we could
**	    not exceed it. However this was not the case when addressing collation
**	    weights in histograms where we are passed a fixed length buffer!
**	    The logic for allocating the SortKey buffer has been reworked so that
**	    Sortkey option always allocates the buffer for the caller and the cecount
**	    is now dropped. Although coll_weight on short strings is now penalised by
**	    the forced memory allocation, the alternatives would have mean increased
**	    CPU to cope for the buffer checks on all the other callers. MakeCE is
**	    called relativly infrequently from adu_ucollweight.
**	28-May-2008 (kiria01) SIR120473
**	    Used symbol AD_5LIKE_RANGE in place of literal '-'
**	27-Oct-2009 (wanfr01) b122799
**	    Callers now pass in a buffer of size ADF_UNI_TMPLEN.  
**	    Local memory allocation only used if this buffer is too small - this 
**	    avoids doing a MEget/MEfree for every comparison.
*/
static DB_STATUS
MakeCE(
       ADF_CB	*adf_scb,
       u_i2	*bst,
       u_i2	*est,
       u_i2	*esc,
       u_i2	**listp,
       i4	opts)
{
    ADUUCETAB   *cetable = (ADUUCETAB *)adf_scb->adf_ucollation;
    i4		num_ces = 0, num_sch = 0;
    i4		sizece;
    i4		i, k, l, c;
    u_i2	*sptr, *celist, *sklist, *ce;
    PTR		mem;
    PTR		ce_mem = (PTR)NULL, sk_mem = (PTR)NULL;
    STATUS	status = E_DB_OK;
    i4		error = 0;
    u_i2	local_list[1024];
    i4		max_levels;
    bool 	gotLbracket = FALSE;	
    bool	caseFold = ((opts & CaseInsensitive) != 0);
    bool	french_coll = ((opts & FrenchCollation) != 0);
    bool	sk_mem_alloc = FALSE;

    if (cetable == (ADUUCETAB *) NULL)
	return (adu_error(adf_scb, E_AD5082_NON_UNICODE_DB, 0));
				/* better be Unicode enabled */

    /*
    ** If producing CE list for "like", ignore level 4
    ** (variable weighting), as "like" is only concerned
    ** with equality (with the exception of "[]" ranges).
    ** Variable weighting may "ignore" some characters
    ** or "shift" them such that they become equivalent
    ** with other characters. "like" needs to match
    ** character-by-character, so we're interested only
    ** in the unmolested "as-is" CE values for each
    ** character.
    */
    max_levels = (opts & LikeCE) ? MAX_CE_LEVELS-1 : MAX_CE_LEVELS;

    /* Determine size of sortkey memory, if optioned */
    if ( opts & Sortkeys )
    {
        i4 skcount;
	for ( sptr = bst; sptr < est; sptr++, num_ces++ )
	{
	    if ( cetable[*sptr].flags & CE_HASOVERFLOW )
		num_ces += cetable[*sptr].overflow->num_values;
	}
	num_sch = 0;
	skcount = num_ces * max_levels + (max_levels - 1) + 1;

	if (skcount > ADF_UNI_TMPLEN_UCS2)
	{
		sk_mem = MEreqmem(0, skcount*sizeof(u_i2), TRUE, &status);
    		sk_mem_alloc = TRUE;
	}
	else
		sk_mem = (PTR)(*listp);
	if ( sk_mem == NULL || status )
	{
	    return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2,
			    (i4) sizeof(status), (i4 *)&status));
	}
	sklist = (u_i2*)sk_mem;
    }
    else
    {
	/* 
	** Count the number of starter characters in the string 
	** as well as the number of regular and overflow CEs.
	**
	** Starter characters are those with a combining class
	** of zero.
	*/
	for ( sptr = bst; sptr < est; sptr++, num_ces++ )
	{
	    if ( cetable[*sptr].comb_class == 0 )
		num_sch++;
	    if ( cetable[*sptr].flags & CE_HASOVERFLOW )
		num_ces += cetable[*sptr].overflow->num_values;
	}
    }
		 
    /*
    ** Get durable memory for the list we are producing (sklist).
    ** Use the stack, if possible, for the other list (celist),
    ** if needed.
    */
    
    /* Determine size of CE memory */
    sizece = sizeof(u_i2) * ((num_ces * max_levels) + num_sch + 1);

    /* Get memory for CE list */
    if ( opts & LikeCE || sizece > sizeof(local_list) )
    {
	ce_mem = MEreqmem(0, sizece, TRUE, &status);
	if ( ce_mem == NULL || status )
	{
	    if (sk_mem_alloc)
		(VOID)MEfree(sk_mem);
	    return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2,
			    (i4) sizeof(status), (i4 *)&status));
	}
	celist = (u_i2*)ce_mem;
    }
    else
    {
	celist = local_list;
	MEfill(sizece, 0, (char *)&local_list);
    }

    /* produce collation elements */
    
    /* combining marks:
    ** The assumtion here is that the only combining characters that count
    ** are the ones listed in the table, these appear to be defined as
    ** contracting characters (multiple characters have a single list of
    ** collation elements). so here's how we play it:
    **  - First check if the character in hand has a combining element in the 
    **    table
    **  - for each combining element found, check if the following combining
    **    characters match the following characters in the string. This must be
    **    an exhaustive search, since we need to take "blocked" characters into
    **    account (what this appears to mean is that if there is a longer 
    **    combining list in the search, then we must use it, i.e. if we find a 
    **    match and there is another list that contains more characters, which 
    **    is also a match, then we must user the longer). I don't know what we 
    **    do if there are two matching lists of the same length. I'm assuming 
    **    this counts as a duplicate entry in the table, making the table bad, 
    **    we should report an error for this.
    **  - if we find a match, then use it's collation element list instead for 
    **    the matched substring, and move on past the substring
    */

    /* get each collation element */

    /* celist[0] will hold actual number of CEs */
    ce = celist+1;

    for (sptr = bst; sptr != est;)
    {

	/* Set starter character marker, if LIKE */
	if ( opts & LikeCE && cetable[*sptr].comb_class == 0 )
	{
	    /* Producing LIKE pattern? */
	    if ( opts & LikePat )
	    {
		/* If processing SQL form LIKE we check for Escape character */
		if ( esc && *esc && *sptr == *esc )
		{
		    /* Set ESC marker */
		    *(ce)++ = ULIKE_ESC;

		    if ( ++sptr >= est )
		    {
			/* Esc at end of pattern string */
			error = E_AD1017_ESC_AT_ENDSTR;
			break;
		    }
		    /* The escaped character */
		    switch ( *sptr )
		    {
			case AD_1LIKE_ONE:
			case AD_2LIKE_ANY:
			case AD_5LIKE_RANGE:
			    /* Procede to CE generation */
			    break;

			case AD_3LIKE_LBRAC:
			    gotLbracket = TRUE;
			    *(ce)++ = *(sptr)++;
			    continue;
			
			case AD_4LIKE_RBRAC:
			    gotLbracket = FALSE;
			    if (ce > celist+4 &&
			        ce[-3] == ULIKE_ESC &&
			        ce[-2] == AD_3LIKE_LBRAC
			        /*ce[-1] == ULIKE_ESC*/
			       )
			    {
				/* We have a null pattern - drop it */
				ce -= 3;
			    }
			    else
				*(ce)++ = *(sptr)++;
			    continue;


			default:
			    if ( *sptr != *esc )
				error = E_AD1018_BAD_ESC_SEQ;
			    break;
		    }
		}
		else 
		{
		  /* Quel or SQL with no escape */
		  if (opts & QuelPat)
		  {
		    /* Quel pattern generation */
		    switch ( *sptr )
		    {
			case '-':
			    /* Only if in a valid bracket context do we treat dash as operator */
			    if (!gotLbracket) break;
			    /* If dash is leftmost char then treat normally */
			    if (ce > celist+2 && ce[-1] == AD_3LIKE_LBRAC) break;
			    *ce++ = ULIKE_PAT;
			    *ce++ = *sptr++;
			    continue;

			case DB_PAT_ONE:
			    /*
			    ** If adding LIKE_ONE after a LIKE_ANY - interchange
			    ** This will cause "*?" to become "?*"
			    ** and cumulatively will migrate the LIKE_ANY to
			    ** the end of the run thus "A?*?????A" -> "A??????*A"
			    */
			    if (ce > celist+3 &&
			        ce[-2] == ULIKE_PAT &&
			        ce[-1] == DB_PAT_ANY)
			    {
				/*ce[-2] = ULIKE_PAT;*/
				ce[-1] = *sptr++;
				*ce++ = ULIKE_PAT;
				*ce++ = DB_PAT_ANY;
			    }
			    else
			    {
				*ce++ = ULIKE_PAT;
				*ce++ = *sptr++;
			    }
			    continue;
			
			case DB_PAT_ANY:
			    /*
			    ** Only add wild card if not already in adjacent position.
			    ** Together with the check above the string
			    ** "A**??*A*?***?****A" would become "A??*A??*A"
			    */
			    if (ce > celist+3 &&
			        ce[-2] == ULIKE_PAT &&
			        ce[-1] == DB_PAT_ANY)
			    {
				sptr++;
			    }
			    else
			    {
				*ce++ = ULIKE_PAT;
				*ce++ = *sptr++;
			    }
			    continue;

			case DB_PAT_LBRAC:
			    /* Quel uses unescaped L bracket */
			    if (!esc) gotLbracket = TRUE;
			    *ce++ = ULIKE_PAT;
			    *ce++ = *sptr++;
			    continue;

			case DB_PAT_RBRAC:
			    /* Quel uses unescaped R bracket */
			    if (!esc) gotLbracket = FALSE;
			    if (ce > celist+2 &&
			        ce[-1] == DB_PAT_LBRAC
			       )
			    {
			       ce -=2;
			       *sptr++;
			    }
			    else
			    {
			      *ce++ = ULIKE_PAT;
			      *ce++ = *sptr++;
			    }
			    continue;

			default:
			    /* Procede to CE generation */
			    break;
		    }
		  }
		  else
		  {
		    /* SQL and no escape given */
		    switch ( *sptr )
		    {
			case AD_5LIKE_RANGE:
			    /* Only if in a valid bracket context do we treat dash as operator */
			    if (!gotLbracket) break;
			    /* If dash is leftmost char then treat normally */
			    if (ce > celist+3 &&
			        ce[-2] == ULIKE_ESC && ce[-1] == AD_3LIKE_LBRAC) break;
			    *ce++ = ULIKE_PAT;
			    *ce++ = *sptr++;
			    continue;

			case AD_1LIKE_ONE:
			    /*
			    ** If adding LIKE_ONE after a LIKE_ANY - interchange
			    ** This will cause "%_" to become "_%"
			    ** and cumulatively will migrate the LIKE_ANY to
			    ** the end of the run thus "A_%_____A" -> "A______%A"
			    */
			    if (ce > celist+3 &&
			        ce[-2] == ULIKE_PAT &&
			        ce[-1] == AD_2LIKE_ANY)
			    {
				/*ce[-2] = ULIKE_PAT;*/
				ce[-1] = *sptr++;
				*ce++ = ULIKE_PAT;
				*ce++ = AD_2LIKE_ANY;
			    }
			    else
			    {
				*ce++ = ULIKE_PAT;
				*ce++ = *sptr++;
			    }
			    continue;
			
			case AD_2LIKE_ANY:
			    /*
			    ** Only add wild card if not already in adjacent position.
			    ** Together with the check above the string
			    ** "A%%__%A%_%%%_%%%%A" would become "A__%A__%A"
			    */
			    if (ce > celist+3 &&
			        ce[-2] == ULIKE_PAT &&
			        ce[-1] == AD_2LIKE_ANY)
			    {
				sptr++;
			    }
			    else
			    {
				*ce++ = ULIKE_PAT;
				*ce++ = *sptr++;
			    }
			    continue;

			case AD_3LIKE_LBRAC:
			    /* Quel uses unescaped L bracket */
			    if (!esc) gotLbracket = TRUE;
			    break;

			case AD_4LIKE_RBRAC:
			    /* Quel uses unescaped R bracket */
			    if (!esc) gotLbracket = FALSE;
			    break;

			default:
			    /* Procede to CE generation */
			    break;
		    }
		  } /* SQL and no escape */
		} /* end escape check */
	    }

	    if ( error )
		break;

	    /* Mark that CE list follows */
	    *(ce)++ = ULIKE_NORM;
	}

	/* check for combining */
	if (cetable[*sptr].flags & CE_COMBINING)
	{
	    ADU_COMBENT	*combent;
	    bool	match = FALSE;
	    i4		match_size = 0;
	    u_i2	lcelist[MAX_CE_LEVELS];
	    bool	variable = FALSE;

	    for (combent = cetable[*sptr].combent; 
		 combent;
		 combent = combent->next_combent)
	    {
		match = TRUE;
		for (i = 0; match && i < combent->num_values; i++)
		{
		    if ( &sptr[i] == est ||
		         combent->values[i] != sptr[i] )
		    {
			match = FALSE;
		    }
		}
		if (match && combent->num_values == match_size)
		{
		    /* bad table, see above comment */
		    error = E_AD1014_BAD_VALUE_FOR_DT;
		    break;
		}
		else if (match && combent->num_values > match_size)
		{
		    bool	completely_ignorable = TRUE;
		    /* 
		    ** save it. Assumption is that there is only one CE list
		    ** for combining entries. 
		    ** @FIX_ME@
		    ** The algorithmn states that
		    ** characters can be both contracting (which we are now
		    ** processing), and expanding (which means that they have
		    ** multiple collation element lists). We don't handle that
		    ** case, and there are no examples of it in the default
		    ** collation element table.
		    ** @FIX_ME@
		    ** We don't currently support "alternate" directives
		    ** the default is "shifted" which appears to mean
		    **  - variable characters recieve zero for intitial
		    **    level weights, and their level 1 weight for a
		    **    final level weight
		    **  - completely ignorable characters (ones with zeros
		    **    for all levels except the final level) recieve zero
		    **    for their final level weight
		    **	- all other characters recieve FFFF for their final
		    **    level weight, regardless of what it originally was
		    */
		    match_size = combent->num_values;
		    for (i = 0; i < max_levels; i++)
		    {
			if ( opts & LikeCE )
			    lcelist[i] = combent->ce[i];
			else if ( i == max_levels-1 )
			{
			    if (combent->flags & CE_VARIABLE)
				lcelist[i] = combent->ce[0];
			    else if (completely_ignorable)
				lcelist[i] = 0;
			    else
				lcelist[i] = 0xFFFF;
			}
			else 
			{
			    if (completely_ignorable && combent->ce)
				completely_ignorable = FALSE;
			    if (combent->flags & CE_VARIABLE)
				lcelist[i] = 0;
			    else
				lcelist[i] = combent->ce[i];
			}
		    }
		}
	    }

	    if ( error )
		break;

	    if (caseFold)
		MakeCEcaseFold(&lcelist[2]);

	    if (match)
	    {
		for (i = 0; i < max_levels; i++)
		{
		    *(ce)++ = lcelist[i];
		}
		/* move past substring */
		sptr += match_size;
		continue;
	    }
	}
	/* 
	** at this point we didn't find a combining entry. If we have a CE list 
	** in the table for the single character, then use it
	*/
	if (cetable[*sptr].flags & CE_HASMATCH)
	{
	    bool	completely_ignorable = TRUE;

	    /* add to CE list */
	    for (i = 0; i < max_levels; i++)
	    {
		if ( opts & LikeCE )
		    ce[i] = cetable[*sptr].ce[i];
		/* for alternate weighting, see comment and FIX_ME above */
		else if ( i == max_levels-1 )
		{
		    if (cetable[*sptr].flags & CE_VARIABLE)
			ce[i] = cetable[*sptr].ce[0];
		    else if (completely_ignorable)
			ce[i] = 0;
		    else
			ce[i] = 0xFFFF;
		} 
		else 
		{
		    if (completely_ignorable && cetable[*sptr].ce[i])
			completely_ignorable = FALSE;
		    if (cetable[*sptr].flags & CE_VARIABLE)
			ce[i] = 0;
		    else
			ce[i] = cetable[*sptr].ce[i];
		}
	    }

	    if (caseFold)
		MakeCEcaseFold(&ce[2]);

	    ce += max_levels;
	    /* 
	    ** Check for overflow collation elements 
	    ** (expanding characters) 
	    */
	    if (cetable[*sptr].flags & CE_HASOVERFLOW)
	    {
		i4		cevalue;
		ADU_CEOVERFLOW	*ceover = cetable[*sptr].overflow;

		for (cevalue = 0; 
		     cevalue < ceover->num_values; cevalue++)
		{
		    bool	completely_ignorable = TRUE;

		    /* add to CE list */
		    for (i = 0; i < max_levels; i++)
		    {
			if ( opts & LikeCE )
			    ce[i] = ceover->oce[cevalue].ce[i];
			/* for alternate weighting, see comment 
			** and FIX_ME above */
			else if ( i == max_levels-1 )
			{
			    if (ceover->oce[cevalue].variable)
				ce[i] = cetable[*sptr].ce[0];
			    else if (completely_ignorable)
				ce[i] = 0;
			    else
				ce[i] = 0xFFFF;
			}
			else
			{
			    if (completely_ignorable && 
				ceover->oce[cevalue].ce[i])
				completely_ignorable = FALSE;
			    if (ceover->oce[cevalue].variable)
				ce[i] = 0;
			    else
				ce[i] = ceover->oce[cevalue].ce[i];
			}
		    }

		    if (caseFold)
			MakeCEcaseFold(&ce[2]);

		    ce += max_levels;
		}
	    }
	}
	else
	{
	    /* no weight in table, need to synthesize */

	    /*
	    ** This is relatively simple:
	    **  - CE[1] = character value
	    **  - CE[2] = min level2 value from table
	    **  - CE[3] = min level3 value from table
	    **  - CE[>3] = 0001
	    **
	    ** @FIX_ME@??? (maybe not)
	    ** The algorithmn indicates that the values other than level 1
	    ** are non relavent, provided thet are non-zero and less than or
	    ** equal to the minimum value at that level, so we should
	    ** be able to simply use 0001 for these levels. right?
	    */
	    *(ce)++ = *sptr;	/* ce[0], primary level */
	    for (i = 1; i < max_levels; i++)
	    {
		*(ce)++ = 1;
	    }
	}
	/* move to next char */
	sptr++;
    } /* each character */

    if ( error )
    {
	if ( ce_mem )
	    (VOID)MEfree(ce_mem);
	if ( sk_mem_alloc )
	    (VOID)MEfree(sk_mem);
	return (adu_error(adf_scb, error, 0));
    }

    /* Set the number of CEs actually in celist */
    celist[0] = (u_i2)(ce - &celist[1]);

    /* create sort keys from collation elements */
    if ( opts & Sortkeys )
    {
	/* sklist[0] is reserved for the count of keys */
	sptr = &sklist[1];

	for ( l = 0; l < max_levels; l++ )
	{
	    /* NOTE: max_levels controls loop because ignored levels
	    ** have already been ignored by now. */

	    /* insert level separator at each level beyond zero */
	    if ( l )
		*sptr++ = (u_i2)0;

	    if ((l == 1) && (french_coll))
	    {
		/* For French Collation second level
		** weights for accents are placed in 
		** reverse.
		*/
		for ( c = (num_ces-1); c >= 0;c--)
		{
		    ce = celist+1 + (c * max_levels) + l;
		    if ( *ce )
			*sptr++ = *ce;
		}
	    }
	    else	
	    {
		for ( c = 0; c < num_ces; c++)
		{
		    ce = celist+1 + (c * max_levels) + l;
		    if ( *ce )
			*sptr++ = *ce;
		}
	    }
	}

	/* Set number of keys */
	sklist[0] = (u_i2)(sptr - &sklist[1]);

	/* Return pointer to sortkey list */
	*listp = sklist;

	/* Toss the CE list memory, if acquired */
	if ( ce_mem )
	    (VOID)MEfree(ce_mem);
    }
    else
	/* Return pointer to CE list */
	*listp = celist;

    return (E_DB_OK);
}

/*
** Name: adu_unorm - Unicode normalization (form D)
**
** Decsription:
**	Normalize a unicode string to form D according to the algorothmn at:
**
**		http://www.unicode.org/unciode/reports/tr15
**
** Inputs:
**	adf_scb	- ADF session control block
**	input_dv - input Unicode string
**	nfc_flag - When given the normalization is always to NFC.
**		   even if the normalization form for the server is NFD.
**
** Outputs:
**	output_dv - Normalized version of the string
**
** Returns:
**  	E_DB_OK
**
** History:
**	9-apr-2001 (stephenb)
**	    Initial creation
**	12-apr-2001 (stephenb)
**	    Blank fill NCHAR's
**	17-may-2001 (stephenb)
**	    Use combining class to re-arrange after decomposition
**	05-aug-2002 (gupsh01)
**	    Added error checking E_AD5019_UDECOMP_COL to avoid segv when
**	    charcater is marked as CE_HASDECOMP, although there is no 
**	    decomposition information.
**	29-aug-2002 (gupsh01)
**	    Added additional check in this routine so that if the input
**	    datatype is not a unicode string then return an error.
**	28-Oct-2002 (jenjo02)
**	    Added checks for truncation.
**      09-jul-2004 (huazh01)
**          When computing 'dstlen' for NVARCHAR type of data, we should 
**          substract the length byte from dst_dv->db_length. 
**          This fixes b112641, INGSRV2903. 
**	09-Nov-2004 (gupsh01)
**	    Fixed the check for truncation return the truncation error
**	    only if the session is started with -string_truncation=fail.
**	02-Dec-2004 (gupsh01)
**	    Refix bug 113415 add error check for error 
**	    E_AD0103_UNINORM_IGN_TRUNC.
** 	15-Feb-2005 (gupsh01)
**	    Added support for normalizing to NFC if we are connecting to 
**	    a database with NFC set.
**	24-Mar-2005 (gupsh01)
**	    Fixed NFC conversion.
**	1-Apr-2005 (gupsh01)
**	    More changes to fix NFC conversion.
**	7-apr-05 (inkdo01)
**	    adu_unorm() has always incorrectly altered the db_length of
**	    dst_dv for NVARCHAR values. The actual length should only be
**	    stored in the value instance count field, not in dst_dv. This
**	    fixes 114247.
**	25-apr-2005 (gupsh01)
**	    Fix nfc conversion to correctly copy the results to output and
**	    also prevent codepoint scanning beyond allocated memory.
**	29-jun-2005 (thaju02)
**	    Result buffer length may be insufficent for decomposition.
**	    Allocate tmpbuf. (B114780)
**	12-jun-2006 (gupsh01)
**	    When allocating temp buffer for decomposition for bug 114780, 
**	    previous data for decomposition is not being copied over.
**	09-may-2007 (gupsh01)
**	    Added nfc_flag to indicate if explicit nfc normalization is 
**	    requested.
**	15-may-2007 (gupsh01)
**	    Return adf error if collation table does not exist.
**	30-may-2007 (gupsh01)
**	    Check for empty source strings.
**      12-Sep-2007 (hanal04) Bug 119061
**          Disallow collation table access if db isn't Unicode enabled.
**	16-Oct-2007 (gupsh01)
**	    Allow embedded NULLS in the unicode string while converting
**	    to NFC.
**	14-Oct-2008 (gupsh01)
**	    Pass in the string datatype to the adu_movestring function.
**	05-May-2009 (kiria01) b122030
**	    Handle the long case.
**      15-May-2009 (hanal04) Bug 121850
**          If nfc_flag then reCompose the output as stated in the function
**          description.
**      18-Jun-2010 (horda03) b123926
**          Swap order of dst_dv and src_dv in adu_long_unorm.
*/
DB_STATUS
adu_unorm(ADF_CB		*adf_scb,
	DB_DATA_VALUE	*src_dv, 
	DB_DATA_VALUE	*dst_dv)
{
    ADUUCETAB		*cetable = (ADUUCETAB *)adf_scb->adf_ucollation;
    DB_STATUS		db_stat = E_DB_OK;
    i4			srclen, dstlen;
    u_i2		*src, *srcchar, *srcend;
    u_i2		*dst, *dstchar, *dstend;
    i4			dlen;
    i4			ccount = 0;
    u_i2		tchar;
    i4			i;

    UCS2        	basechar;
    UCS2        	*comblist;
    i4          	normcstrlen = 0;
    i4			norm_diff = 0;
    u_i2		*tmpbuf = NULL;
    i4			NFC_TRUNCATED = 0;
    i4			nfc_dstlen = 0;
    i4			bytes_copied = 0;
  
    if (cetable == (ADUUCETAB *) NULL)
        return (adu_error(adf_scb, E_AD5082_NON_UNICODE_DB, 0));
                                /* better be Unicode enabled */

    if (src_dv->db_datatype == DB_LNVCHR_TYPE ||
	src_dv->db_datatype == DB_LNLOC_TYPE)
	return adu_long_unorm (adf_scb, src_dv, dst_dv);

   if ( src_dv->db_datatype != DB_NCHR_TYPE && 
	src_dv->db_datatype != DB_NVCHR_TYPE )
       return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

   /* If collation table is not initialize bail out now */
    if ( cetable == NULL )
       return (adu_error(adf_scb, E_AD5012_UCETAB_NOT_EXISTS, 0));

    if (db_stat = adu_lenaddr(adf_scb, src_dv, &srclen, (char **)&src))
	return (db_stat);

    /* output data is empty, so get length from data-value */
    if (dst_dv->db_datatype == DB_NCHR_TYPE)
    {
	dst = (u_i2 *)dst_dv->db_data;
	dstlen = dst_dv->db_length;
    }
    else if (dst_dv->db_datatype == DB_NVCHR_TYPE)
    {
	dst = (u_i2 *)
            ((DB_NVCHR_STRING *)dst_dv->db_data)->element_array;
        dstlen = dst_dv->db_length - DB_CNTSIZE;
    }
    else
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    if (srclen)    /* If empty source string then don't do anything */
    {
    /*
    ** Return error if output is too small and we are told to
    ** check it, but first try tossing trailing blanks from
    ** the source.
    */
    if ( dstlen < srclen && adf_scb->adf_strtrunc_opt == ADF_ERR_STRTRUNC )
    {
	srcend = (u_i2*)((char*)src + srclen);

	for ( ; srcend > src && *(srcend-1) == U_BLANK; srcend--);

	srclen = (char*)srcend - (char*)src;

	if ( dstlen >= srclen )
	    srclen = dstlen;
	else
	    return(adu_error(adf_scb, E_AD1082_STR_TRUNCATE, 
			     2, srclen, (PTR)src));
    }

    srcend = (u_i2 *)((char *)src + srclen);
    dstend = (u_i2 *)((char *)dst + dstlen);

    ccount = 0;
	nfc_dstlen = dstlen;

    for (srcchar = src, dstchar = dst; 
	 srcchar != srcend && dstchar != dstend;
	 srcchar++)
    {
	if (cetable[*srcchar].flags & CE_HASCDECOMP)
	{
	    /* decompose character */
	    dlen = 0;
	    if (cetable[*srcchar].decomp)
	    {
		bytes_copied = (dstchar - dst)* sizeof(UCS2);

		if ((adf_scb->adf_uninorm_flag == AD_UNINORM_NFC) &&
		    !tmpbuf && 
		    ((bytes_copied + (cetable[*srcchar].decomp->num_values * 
			sizeof(UCS2) * 4)) > nfc_dstlen))
		{
		    /*
		    **result buffer is too small to use for
		    ** decomposition. Allocate a temp buffer 
		    */		    			
			nfc_dstlen += 1024;
			tmpbuf = (UCS2 *) MEreqmem (0, nfc_dstlen, TRUE, &db_stat);
			dstchar = (u_i2 *)((char *)tmpbuf + bytes_copied );
			dstend = (u_i2 *)((char *)tmpbuf + nfc_dstlen);
			MEcopy (dst, bytes_copied, tmpbuf);
			dst = tmpbuf;
		}
		if (db_stat = decompose(adf_scb, 
		      *srcchar, dstchar, dstend, &dlen))
		{
		    if ((db_stat == E_DB_INFO) && 
			(adf_scb->adf_errcb.ad_errcode == E_AD0103_UNINORM_IGN_TRUNC))
		    {
	 	      adf_scb->adf_errcb.ad_errcode = E_AD0000_OK;
	 	      break;	
		    }
		    else
		    {
		      if (tmpbuf) MEfree((char*)tmpbuf);
		      return (db_stat);
		    }
		}
	    }
	    else
	      return (adu_error(adf_scb, E_AD5019_UDECOMP_COL, 0));

	    dstchar += dlen;
	    ccount += dlen;
	}
	else
	{
	    *(dstchar)++ = *srcchar;
	    ccount++;
	}
    }

    /* re-arrange characters according to combining class */
    for (i = 0; i < ccount-1; i++)
    {
	if (cetable[dst[i+1]].comb_class &&
	    cetable[dst[i]].comb_class > cetable[dst[i+1]].comb_class)
	{
	    /* Exchange characters */
	    tchar = dst[i];
	    dst[i] = dst[i+1];
	    dst[i+1] = tchar;
	}
    }

    /* If the database we are connecting to is created with
    ** normal form C then we convert NFD to NFC
    */
    if (adf_scb->adf_uninorm_flag == AD_UNINORM_NFC)
    {
      u_i2	*nfcbuf;
      u_i2	*nfcbufptr;
      u_i2	uni_char;
      i4	inlen = dstchar - dst; 
      i4 	nfd_count = 0;
      i4 	nfc_count= 0;
      i4 	subc = 0;
      i4 	pos = 0;  
      i4 	endscan = 0;  

      /* Allocate nfc conversion buffer */
      nfcbuf = (UCS2 *) MEreqmem (0, sizeof(UCS2) * dstlen, TRUE, &db_stat);
      nfcbufptr = nfcbuf;

      /* Allocate working buffer */
      comblist = (UCS2 *) MEreqmem (0, sizeof(UCS2) * inlen, TRUE, &db_stat);
   
      /* Allow embedded nulls in nchar/nvarchar strings however remove
      ** any trailing nulls for efficiency.
      */

      endscan = inlen;
      while ((endscan > 0) && (dst[endscan - 1] == U_NULL))
        endscan--;

      for (; (nfd_count < inlen) ; nfd_count+=1)
      {
        subc = 0;
        normcstrlen = 0;


	if (nfc_count >= dstlen)
	{
	  /* NFD buffer may still have data in it 
	  ** check if we found end of source 
	  */
	  if (dst[nfd_count] == U_NULL)
	  {
	    nfcbuf [nfc_count] = U_NULL;
	    nfd_count++;
	  }	
	  else
	    NFC_TRUNCATED = 1;

	  break;
	}

        /* obtain the base character */
        uni_char = dst[nfd_count];
	basechar = (cetable[uni_char].comb_class == 0) ? uni_char : 0;      

	if (basechar)
        { 
	    for (pos=nfd_count+1;
                (pos < endscan) &&
                    (cetable[(uni_char = dst[pos])].comb_class != 0);
                pos+=1, subc+=1 )
            {
                comblist[subc] = uni_char;  /* copy combining codepoint */
            }
            /*
            ** Reset loop counter to current input string position.
            ** This should be the start of the next character or the end
            ** of the string.
            */
            nfd_count = pos-1;
        }

	/* Convert the substring to Normal form C */
	if  (basechar && subc > 0)
	{
	  db_stat = adu_map_conv_normc (adf_scb, cetable,
					basechar,
					comblist, subc,
					comblist, &normcstrlen);

	  MEcopy(comblist, normcstrlen * sizeof(UCS2), nfcbufptr);
	  nfcbufptr += normcstrlen;
	  nfc_count += normcstrlen; 
	}
	else
	{
	  if (!basechar) 
	  {
	    basechar = dst[nfd_count];
	  }
	  nfcbuf[nfc_count++] = basechar;
	  nfcbufptr++;
	}			
      } /* end FOR */

      if (!(NFC_TRUNCATED))
      {
        if ((db_stat = adu_movestring (adf_scb, (u_char *)nfcbuf, 
		nfc_count * sizeof(UCS2), src_dv->db_datatype, dst_dv)) == E_DB_OK)
        {
          norm_diff = nfd_count - nfc_count;	

          if (norm_diff > 0)	
          {
	    dstchar -= norm_diff;
	    ccount -= norm_diff;
          }
	}
      }
      else if (adf_scb->adf_strtrunc_opt == ADF_ERR_STRTRUNC)
      {
	MEfree((char *)comblist);
	MEfree((char *)nfcbuf);
	if (tmpbuf) MEfree((char *)tmpbuf);
        return(adu_error(adf_scb, E_AD1082_STR_TRUNCATE,
                2, dstend - dst, (PTR)dst));
      }

      /* free work buffer */
      MEfree((char *)comblist);
      MEfree((char *)nfcbuf);
      if (tmpbuf)
      {
	MEfree((char *)tmpbuf);
	/* need to reset dstchar/dstend for NCHR blank-fill below */
	dstchar = (u_i2 *)(dst_dv->db_data) + ccount;
	dstend = (u_i2 *)(dst_dv->db_data + dstlen);
      }
    }	/* NFC */
    } /* srclen given */

    if (dst_dv->db_datatype == DB_NVCHR_TYPE)
    {
	/* set length field */
	((DB_NVCHR_STRING *)dst_dv->db_data)->count = ccount;
    }
    else 
    {
	/* Blank-fill excess NCHR destination slots */
	for ( ; dstchar < dstend; *(dstchar++) = U_BLANK );
    }

    return (E_DB_OK);
}

/*
** Name: adu_utf8_unorm - Unicode normalization routine for UTF8 characterset
**
** Decsription:
**	Normalize a unicode string in UTF-8 encoding to form C.
**	This routine will be used for UTF8 characterset in Ingres and 
**	is intended for regular char/varchar datatypes.
**
** Inputs:
**	adf_scb	- ADF session control block
**	input_dv - input Unicode string
**
** Outputs:
**	output_dv - NFC Normalized version of the UTF8 string.
**
** Returns:
**  	E_DB_OK
**
** History:
**	9-may-2007 (gupsh01)
**	    Created.
**	14-may-2007 (gupsh01)
**	    Pass NFC flag to adu_nvchr_fromutf8.
**	24-may-2007 (gupsh01)
**	    Increase the length of the temporary
**	    buffer.
**	05-sep-2007 (gupsh01)
**	    Restrict the length of temp buffer 
**	    so it does not exceed the max allowed
**	    string length. 
**	05-sep-2007 (gupsh01)
**	    Include DB_LTXT_TYPE.
**	03-jun-2008 (gupsh01)
**	    Fix the maximum length for nvarchar
**	    case.
**	04-Feb-2009 (gupsh01)
**	    Use ADF_UNI_TMPLEN define for temporary buffer space for
**	    unicode normalization. 
**	05-May-2009 (kiria01) b122030
**	    Handle the long case.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      20-May-2009 (hanal04) Bug 122080
**          Set the nfc_flag passed to adu_nvchr_fromutf8() based on the
**          dst_dv->db_datatype. For non-unicode types we should not
**          reCombine characters.
**      15-Jun-2010 (horda03) b123
**          Don't error if DB_LNVCHR_TYPE paramaeter supplied.
**      18-Jun-2010 (horda03) b123926
**          Swap order of dst_dv and src_dv in adu_long_unorm.
*/
DB_STATUS
adu_utf8_unorm(ADF_CB	*adf_scb,
	DB_DATA_VALUE	*src_dv,
	DB_DATA_VALUE	*dst_dv)
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DATA_VALUE	temp_dv;
    u_i2		tempbuf[ADF_UNI_TMPLEN];
    char		*temp_ptr;
    i4			templen = 0;
    bool		memexpanded = FALSE;
    i4			srclen;
    u_i2		*src;
    u_i2		saved_uninorm_flag;

    if (src_dv->db_datatype == DB_LVCH_TYPE ||
	src_dv->db_datatype == DB_LCLOC_TYPE ||
        src_dv->db_datatype == DB_LNVCHR_TYPE ||
        src_dv->db_datatype == DB_LNLOC_TYPE)
	return adu_long_unorm (adf_scb, src_dv, dst_dv);

    if ((src_dv->db_datatype != DB_CHR_TYPE) && 
       (src_dv->db_datatype != DB_TXT_TYPE) && 
       (src_dv->db_datatype != DB_LTXT_TYPE) && 
       (src_dv->db_datatype != DB_CHA_TYPE) && 
       (src_dv->db_datatype != DB_VCH_TYPE))
     return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    /* Find out input length etc */
    if (db_stat = adu_lenaddr(adf_scb, src_dv, &srclen, (char **)&src))
	return (db_stat);

    templen = srclen * 4;

    if (templen > (DB_MAXSTRING + 2))
      templen = (DB_MAXSTRING + 2);

    if (templen > ADF_UNI_TMPLEN)
    {
      temp_ptr = MEreqmem(0, templen, TRUE, &db_stat);
      if (db_stat)
        return (db_stat);
      memexpanded = TRUE;
    }
    else 
	temp_ptr = (char *) &tempbuf[0];

    temp_dv.db_data = temp_ptr;
    temp_dv.db_datatype = DB_NVCHR_TYPE;
    temp_dv.db_length = templen;
    temp_dv.db_prec = 0;
    temp_dv.db_collID = 0;

    saved_uninorm_flag = adf_scb->adf_uninorm_flag;
    switch(dst_dv->db_datatype)
    {
        case DB_NCHR_TYPE:
        case DB_NVCHR_TYPE:
        case DB_LNVCHR_TYPE:
            /* Force NFC for UTF8 types, we'll restore the default after
            ** calling adu_nvchr_fromutf8().
            */
            adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
            break;

        default:
            break;
    }

    /* Convert UTF8 to UCS2 this will automatically normalize the data */
    if ((db_stat = adu_nvchr_fromutf8 (adf_scb, src_dv, &temp_dv)) != E_DB_OK)
    {
      if (memexpanded) 
	MEfree (temp_ptr);
      adf_scb->adf_uninorm_flag = saved_uninorm_flag;
      return(db_stat);
    }
    adf_scb->adf_uninorm_flag = saved_uninorm_flag;

    /* Convert UCS2 back to UTF8 */
    if ((db_stat = adu_nvchr_toutf8 (adf_scb, &temp_dv, dst_dv)) != E_DB_OK)
    {
      if (memexpanded) 
	MEfree (temp_ptr);
      return(db_stat);
    }

    if (memexpanded) 
	MEfree (temp_ptr);

    return (E_DB_OK);
}

/*
** Name: decompose - canonical decomposition of a Unicode character
**
** Description:
**	Recursive function to decompose a character into it's canonical
**	form D, using the collation element table
**
** Inputs:
**	ichar	- Input character
**	ochar	- Place to put output string
**	ochar_end - end of output string memory
**	cetable	- collation table
**
** Outputs:
**	ochar	- decomposed string
**	dlen	- resultant length of decomposed form
**
** Returns:
**	DB_STATUS
**
** History:
**	9-apr-2001 (stephenb)
**	    Initial creation.
**	17-jul-2002 (gupsh01) 
**	    Increment charlen if there is no further decomposition.
**	    so that the output is correctly copied out.
**	28-Oct-2002 (jenjo02)
**	    Added checks for truncation.
**	09-Nov-2004 (gupsh01)
**	    Corrected error reporting for truncation.
**	02-Dec-2004 (gupsh01)
**	    Refix bug 113415 add error E_AD0103_UNINORM_IGN_TRUNC.
**	18-Jan-2005 (gupsh01)
**	    Fix case when the decomposed length exactly fits in the
**	    output buffer. 
**	15-Apr-2005 (gupsh01)
**	    Fixed the length calculation for the result string after
**	    recursive calls to decompose.
**      12-Sep-2007 (hanal04) Bug 119061
**          Disallow collation table access if db isn't Unicode enabled.
*/
static	DB_STATUS
decompose(ADF_CB	*adf_scb,
	  u_i2		ichar,
	  u_i2		*ochar,
	  u_i2		*ochar_end,
	  i4		*dlen)
{
    ADUUCETAB	*cetable = (ADUUCETAB *)adf_scb->adf_ucollation;
    u_i2	*curchar;
    i4		i;
    i4		charlen;
    u_i2	dvalue;
    i4		len;
    DB_STATUS	db_stat;

    if (cetable == (ADUUCETAB *) NULL)
        return (adu_error(adf_scb, E_AD5082_NON_UNICODE_DB, 0));
                                /* better be Unicode enabled */

    /* find decomp string */
    if (ochar + cetable[ichar].decomp->num_values > ochar_end)
    {
	/* 
	** decomposition too long:
	**
	** We're in the middle of a decomposition. What else 
	** can be done but return an error?
	*/
        if ( adf_scb->adf_strtrunc_opt == ADF_ERR_STRTRUNC )
	    return(adu_error(adf_scb, E_AD1082_STR_TRUNCATE, 
			     2, ochar_end - ochar, (PTR)ochar));
	else 
	{
	  adf_scb->adf_errcb.ad_errcode = E_AD0103_UNINORM_IGN_TRUNC;
    	  return(E_DB_INFO);
	}
    }
    *dlen += cetable[ichar].decomp->num_values;

    for (i = 0, charlen = 0; i < cetable[ichar].decomp->num_values; i++)
    {
	dvalue = cetable[ichar].decomp->dvalues[i];
	len = 0;
	if (cetable[dvalue].flags & CE_HASCDECOMP)
	{
	    /* character decomposes further */
	    if (db_stat = decompose(adf_scb, 
		    dvalue, &ochar[charlen], ochar_end, &len))
		return (db_stat);
	    *dlen += len - 1;
	    charlen += len;
	}
	else
	{
	    ochar[charlen] = dvalue;
	    charlen++; 
	}
    }

    return(E_DB_OK);
}

/*
** Name: adu_ucollweightn - Compute collation weight (sortkey) of Unicode
**	input value.
**
** Decsription:
**	Compute collation weight of Unicode input value. This is simply the 
**	sortkey used for Unicode collation, but made available externally.
**	This is used for histogram construction on Unicode columns. This
**	version of the function includes an explicit collation ID override.
**
** Inputs:
**	adf_scb	- ADF session control block
**	src_dv - input Unicode string
**	col_dv - collation ID.
**
** Outputs:
**	dst_dv - Result sortkey as varbyte.
**
** Returns:
**  	E_DB_OK
**
** History:
**	20-dec-04 (inkdo01)
**	    Written.
*/
DB_STATUS	adu_ucollweightn(ADF_CB	*adf_scb,
			 DB_DATA_VALUE	*src_dv,
			 DB_DATA_VALUE	*col_dv,
			 DB_DATA_VALUE	*dst_dv)
{
    DB_DATA_VALUE	new_dv;
    i2			collID;

    STRUCT_ASSIGN_MACRO(*src_dv, new_dv);
    MEcopy(col_dv->db_data, sizeof(i2), (char *)&collID);
    new_dv.db_collID = collID;
    return(adu_ucollweight(adf_scb, &new_dv, dst_dv));
}
    

/*
** Name: adu_ucollweight - Compute collation weight (sortkey) of Unicode
**	input value.
**
** Decsription:
**	Compute collation weight of Unicode input value. This is simply the 
**	sortkey used for Unicode collation, but made available externally.
**	This is used for histogram construction on Unicode columns.
**
** Inputs:
**	adf_scb	- ADF session control block
**	src_dv - input Unicode string
**
** Outputs:
**	dst_dv - Result sortkey as varbyte.
**
** Returns:
**  	E_DB_OK
**
** History:
**	20-dec-04 (inkdo01)
**	    Written.
**	28-feb-05 (inkdo01)
**	    Add code to flip bytes in u_i2 result array on byte swapped
**	    platforms, since result of func is char array. optimizedb breaks
**	    if result is left asis.
**	17-May-2007 (gupsh01)
**	    Added support for French Unicode Collation.
**      12-Sep-2007 (hanal04) Bug 119061
**          Disallow collation table access if db isn't Unicode enabled.
**	08-Jan-2008 (kiria01) b119709
**	    Change to handle the buffer returned by MakeCE and its
**	    copy back to the passed in dst_dv.
**	    The data can now passed back so that the fixed zero word can now
**	    eliminated from the head of every collation weight.
**	14-Jan-08 (kiria01) b119738
**	    Eliminated the unused pair of null bytes from collation data
	    permanently along with the buffer corrections in adc_helem.c
	    that relate to this.
**	27-Oct-2009 (wanfr01) b122799
**	    Use a local variable for unicode manipulation rather than
**	    allocating a buffer every time
*/
DB_STATUS	adu_ucollweight(ADF_CB	*adf_scb,
			 DB_DATA_VALUE	*src_dv,
			 DB_DATA_VALUE	*dst_dv)
{
    ADUUCETAB		*cetable = (ADUUCETAB *)adf_scb->adf_ucollation;
    DB_STATUS		db_stat;
    i4			srclen, dstlen;
    u_i2		*bst, *est;
    UCS2		local_list[ADF_UNI_TMPLEN_UCS2];
    u_i2		*dst = local_list;
    u_i2		i;
    i4			opts = Sortkeys;

    if (cetable == (ADUUCETAB *) NULL)
        return (adu_error(adf_scb, E_AD5082_NON_UNICODE_DB, 0));
                                /* better be Unicode enabled */
  
   if ( src_dv->db_datatype != DB_NCHR_TYPE && 
	src_dv->db_datatype != DB_NVCHR_TYPE )
       return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));

    /* Get source/destination addrs/lengths. */
    if (db_stat = adu_lenaddr(adf_scb, src_dv, &srclen, (char **)&bst))
	return (db_stat);

    est = (u_i2 *) ((char *)bst + srclen);

    if (src_dv->db_collID == DB_UNICODE_CASEINSENSITIVE_COLL)
	opts |= CaseInsensitive;

    if ((bst && cetable[*bst].flags & CE_COLLATEFRENCH) ||
         (src_dv->db_collID == DB_UNICODE_FRENCH_COLL))
	  opts |= FrenchCollation;

    db_stat = MakeCE(adf_scb, bst, est, (u_i2*)0, &dst, opts);

    if (db_stat == E_DB_OK && dst)
    {
	u_i2 len = dst[0] * sizeof(u_i2);	/* Actual return length in BYTES */
	u_i1 *s = (u_i1 *)&dst[1];		/* Actual start of coll weights */
	u_i2 *d = (u_i2 *)&((DB_TEXT_STRING *)dst_dv->db_data)->db_t_text;
						/* The actual start of where the data should
						** be copied
						*/
	/* Find max available space in output buffer in bytes */
	dstlen = dst_dv->db_length - DB_CNTSIZE;

	/* There may have been less actual data */
	if (dstlen > len)
	    dstlen = len;

	/* Set BYTE count length in place of actual stored data */
	((DB_TEXT_STRING *)dst_dv->db_data)->db_t_count = dstlen;
	
	/* These collation weights are an array of shorts but
	** in order for these to compare correctly as byte arrays
	** we may have to byte swap the entries.
	** Big-endian just requires the buffer copying.
	** Little-endian need byteswapping.
	*/
#ifdef LITTLE_ENDIAN_INT
	for (i = 0; i < dstlen; i += 2)
	{
	    register u_i2 tmp = (s[i] << 8) | s[i+1];
	    *d++ = tmp;
	}
#else
	MEcopy(s, dstlen, d);
#endif
	/* Free allocated memory if it was used instead of local buffers */
	if (dst != local_list)
         MEfree((char *)dst);
    }
    else
    {
	((DB_TEXT_STRING *)dst_dv->db_data)->db_t_count = 0;
    }

    return(db_stat);
}

/*
** Name: adu_pat_MakeCEchars - make a collation element list from a string
**
** Description:
** 	This function follows section 4.2 of the algorithmn to create
**	a collation element list from a normalized input string
**
**	It is substantially derived from MakeCE but is driven in a
**	different manner. Unlike MakeCE, this is optimised for LIKE
**	type collation and can be driven 1 character at a time.
**
**	NOTE: NO ERROR CHECKING IS DONE IN THIS ROUTINE!
**	It is expected that this routine will be used in conjunction
**	with the next routine - adu_pat_MakeCElen() which will check
**	for errors in the course of calculating the length.
**	Having called adu_pat_MakeCElen() for a segment of text it will
**	be the callers duty to ensure that the output buffer for *wrtp
**	will be large enough - no buffer overrun will be checked for.
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block with
**		adf_scb->adf_ucollation - Unicode collation table
**
**	rdp	  - start of normalized string
**	rdend	  - end of string
**	pat_flags - AD_PAT_WO_CASE
**		    AD_PAT_BIGNORE
**		    AD_PAT_DIACRIT_OFF
**
** Input/Output:
**	wrtp     - will be the pointer into the buffer where the
**		   CE data will be written. It should be WORD aligned
**		   and the pointer will be adjusted to mark the new
**		   buffer end.
**
** Returns:
**	VOID
**
** History:
**	28-May-2008 (kiria01) SIR120473
**	    Created from MakeCE for pattern compiler
*/
VOID
adu_pat_MakeCEchar(
       ADF_CB	*adf_scb,
       const UCS2 **rdp,
       const UCS2 *rdend,
       UCS2	**wrtp,
       u_i4	pat_flags)
{
    ADUUCETAB   *cetable = (ADUUCETAB *)adf_scb->adf_ucollation;

    register const u_i2 *sptr = *rdp;
    register u_i2 *ce = *wrtp;
    i4 max_levels = MAX_CE_LEVELS-1;

    /*
    ** We are producing CE list for "like" so will ignore level 4 (variable
    ** weighting), as we are only concerned with equality (with the exception
    ** of "[]" ranges). Variable weighting may "ignore" some characters or
    ** "shift" them such that they become equivalent with other characters.
    ** "like" needs to match character-by-character, so we're interested only
    ** in the unmolested "as-is" CE values for each character.
    **
    ** Combining marks:
    ** The assumption here is that the only combining characters that count
    ** are the ones listed in the table, these appear to be defined as
    ** contracting characters (multiple characters have a single list of
    ** collation elements). so here's how we play it:
    **  - First check if the character in hand has a combining element in the 
    **    table
    **  - for each combining element found, check if the following combining
    **    characters match the following characters in the string. This must be
    **    an exhaustive search, since we need to take "blocked" characters into
    **    account (what this appears to mean is that if there is a longer 
    **    combining list in the search, then we must use it, i.e. if we find a 
    **    match and there is another list that contains more characters, which 
    **    is also a match, then we must user the longer). I don't know what we 
    **    do if there are two matching lists of the same length. I'm assuming 
    **    this counts as a duplicate entry in the table, making the table bad, 
    **    we should report an error for this.
    **  - if we find a match, then use it's collation element list instead for 
    **    the matched substring, and move on past the substring
    */

    while (sptr < rdend)
    {
	if ((*sptr == U_BLANK || !*sptr) &&
		(pat_flags & AD_PAT_BIGNORE))
	{
	    sptr++;
	    continue;
	}
	/* check for combining */
	if (cetable[*sptr].flags & CE_COMBINING)
	{
	    ADU_COMBENT	*combent;
	    i4 match_size = 0;

	    for (combent = cetable[*sptr].combent; 
		 combent;
		 combent = combent->next_combent)
	    {
		i4 i;
		if (rdend - sptr < combent->num_values)
		    goto no_match;
		for (i = 0; i < combent->num_values; i++)
		{
		    if (combent->values[i] != sptr[i])
		         goto no_match;
		}
		if (combent->num_values > match_size)
		{
		    /* 
		    ** save it. Assumption is that there is only one CE list
		    ** for combining entries. 
		    ** @FIX_ME@
		    ** The algorithmn states that
		    ** characters can be both contracting (which we are now
		    ** processing), and expanding (which means that they have
		    ** multiple collation element lists). We don't handle that
		    ** case, and there are no examples of it in the default
		    ** collation element table.
		    ** @FIX_ME@
		    ** We don't currently support "alternate" directives
		    ** the default is "shifted" which appears to mean
		    **  - variable characters recieve zero for intitial
		    **    level weights, and their level 1 weight for a
		    **    final level weight
		    **  - completely ignorable characters (ones with zeros
		    **    for all levels except the final level) recieve zero
		    **    for their final level weight
		    **	- all other characters recieve FFFF for their final
		    **    level weight, regardless of what it originally was
		    */
		    match_size = combent->num_values;
		    for (i = 0; i < max_levels; i++)
		    {
			ce[i] = combent->ce[i];
		    }
		}
	    }
	    if (match_size)
	    {
		if (pat_flags & AD_PAT_DIACRIT_OFF)
		    ce[1] = 0;
		if (pat_flags & AD_PAT_WO_CASE)
		    MakeCEcaseFold(&ce[2]);
		ce += max_levels;
		/* move past substring */
		sptr += match_size;
		continue;
	    }
	}
	/* 
	** at this point we didn't find a combining entry. If we have a CE list 
	** in the table for the single character, then use it
	*/
no_match:
	if (cetable[*sptr].flags & CE_HASMATCH)
	{
	    i4 i;
	    /* add to CE list */
	    for (i = 0; i < max_levels; i++)
	    {
		ce[i] = cetable[*sptr].ce[i];
	    }

	    if (pat_flags & AD_PAT_DIACRIT_OFF)
		ce[1] = 0;
	    if (pat_flags & AD_PAT_WO_CASE)
		MakeCEcaseFold(&ce[2]);

	    ce += max_levels;
	    /* 
	    ** Check for overflow collation elements 
	    ** (expanding characters) 
	    */
	    if (cetable[*sptr].flags & CE_HASOVERFLOW)
	    {
		i4 cevalue;
		ADU_CEOVERFLOW	*ceover = cetable[*sptr].overflow;

		for (cevalue = 0; 
		     cevalue < ceover->num_values; cevalue++)
		{
		    /* add to CE list */
		    for (i = 0; i < max_levels; i++)
		    {
			ce[i] = ceover->oce[cevalue].ce[i];
		    }

		    if (pat_flags & AD_PAT_DIACRIT_OFF)
			ce[1] = 0;
		    if (pat_flags & AD_PAT_WO_CASE)
			MakeCEcaseFold(&ce[2]);

		    ce += max_levels;
		}
	    }
	}
	else
	{
	    i4 i;
	    /* no weight in table, need to synthesize */

	    /*
	    ** This is relatively simple:
	    **  - CE[1] = character value
	    **  - CE[2] = min level2 value from table
	    **  - CE[3] = min level3 value from table
	    **  - CE[>3] = 0001
	    **
	    ** @FIX_ME@??? (maybe not)
	    ** The algorithmn indicates that the values other than level 1
	    ** are non relavent, provided thet are non-zero and less than or
	    ** equal to the minimum value at that level, so we should
	    ** be able to simply use 0001 for these levels. right?
	    */
	    *ce++ = *sptr;	/* ce[0], primary level */
	    *ce++ = (pat_flags & AD_PAT_DIACRIT_OFF)? 0 : 1;
	    *ce++ = 1; /* No case implications */
	}
	/* move to next char */
	sptr++;
    } /* each character */

    *rdp = sptr;
    *wrtp = ce;
}

/*
** Name: adu_pat_MakeCElen - calc length of a collation element list
**
** Description:
** 	This function follows section 4.2 of the algorithmn to create
**	a collation element list from a normalized input string
**
**	It is substantially derived from MakeCE but is driven in a
**	different manner. To be used with adu_pat_MakeCEchar - see
**	its comments.
**
** Inputs:
**      adf_scb - Pointer to an ADF session control block with
**		adf_scb->adf_ucollation - Unicode collation table
**
**	rdp	  - start of normalized string
**	rdend	  - end of string
**	
** Outputs:
**	reslen    - returned length (in bytes) required for CE list
**		    which will always be a multiple of sizeof(UCS2) and
**		    max_levels (=MAX_CE_LEVELS-1).
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**		
**	possible errors:
**			E_AD1014_BAD_VALUE_FOR_DT
**			E_AD5082_NON_UNICODE_DB
**
** History:
**	28-May-2008 (kiria01) SIR120473
**	    Created from MakeCE for pattern compiler
*/
DB_STATUS
adu_pat_MakeCElen(
       ADF_CB	*adf_scb,
       const UCS2 *rdp,
       const UCS2 *rdend,
       u_i4	*reslen)
{
    ADUUCETAB   *cetable = (ADUUCETAB *)adf_scb->adf_ucollation;
    register const u_i2 *sptr = rdp;
    register u_i4 ce = 0;
    i4 max_levels = MAX_CE_LEVELS-1;

    if (cetable == (ADUUCETAB *) NULL)
    {
	*reslen = 0;
	return (adu_error(adf_scb, E_AD5082_NON_UNICODE_DB, 0));
				/* better be Unicode enabled */
    }

    while (sptr < rdend)
    {
	ce += max_levels;
	/* check for combining */
	if (cetable[*sptr].flags & CE_COMBINING)
	{
	    ADU_COMBENT	*combent;
	    i4 match_size = 0;

	    for (combent = cetable[*sptr].combent; 
		 combent;
		 combent = combent->next_combent)
	    {
		i4 i;
		if (rdend - sptr < combent->num_values)
		    goto no_match;
		for (i = 0; i < combent->num_values; i++)
		{
		    if (combent->values[i] != sptr[i])
		         goto no_match;
		}
		if (combent->num_values == match_size)
		    return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));

		if (combent->num_values > match_size)
		{
		    match_size = combent->num_values;
		}
	    }

	    if (match_size)
	    {
		/* move past substring */
		sptr += match_size;
		continue;
	    }
	}
no_match:
	if (cetable[*sptr].flags & CE_HASMATCH)
	{
	    if (cetable[*sptr].flags & CE_HASOVERFLOW)
	    {
		ADU_CEOVERFLOW	*ceover = cetable[*sptr].overflow;
		ce += max_levels*ceover->num_values;
	    }
	}
	sptr++;
    }

    *reslen = ce * sizeof(UCS2);
    return (E_DB_OK);
}
