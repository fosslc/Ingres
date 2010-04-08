/*
** Copyright (c) 2007 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <me.h>
#include <iicommon.h>
#include <adf.h>
#include <ulf.h>
#include <adfint.h>
#include <aduint.h>
#include <aduucol.h>
#include "adustrcmp.h"

/**
**
** Name: aduuquel.c -	Routines for implementing QUEL comparison on
**			NATIONAL CHARACTER datatypes (NCHAR, NVARCHAR).
**
** This file includes the following externally visible routines:
**
**	adu_ulexcomp -	Compares two NCHAR or NVARCHAR string data values for
**			equality. QUEL pattern matching characters are
**			allowed ('*' matches any number of characters, 
**			including none; '?' matches exactly one character). 
**			Also, the '\' character is supported as escape. If
**			the '*' or '?', '[' and ']' characters are escaped, 
**			they loose their special pattern match meaning, while 
**			the '[' and ']' characters, take on the QUEL range
**			pattern match meaning. 
**
** This file also includes the following static routines:
**
**	ad0_uquel    -	Recursive routine used by adu_ulexcomp() to compare two
**			strings using QUEL pattern matching semantics.
**	ad0_uquel_qmatch -	Process a `MATCH-ONE' character for QUEL.
**	ad0_uquel_pmatch -	Process a `MATCH-ANY' character for QUEL.
**	ad0_uquel_lmatch -	Process a `range sequence' for QUEL.
**	ad0_uquel_comp   -	Compare two strings of collation elements.
**
**  History:
**	23-Oct-2007 (gupsh01)
**	    Created.
**/

/*
**  Definition of static variables and forward static functions.
*/
static DB_STATUS ad0_uquel(ADF_CB *adf_scb, UCS2 *sptr, UCS2 *ends, UCS2 *pptr,
	UCS2 *endp, i4 *rcmp, bool bignore);

static DB_STATUS ad0_uquel_qmatch(ADF_CB  *adf_scb, UCS2 *sptr, UCS2 *ends,
	UCS2 *pptr, UCS2 *endp, i4 *rcmp, bool bignore);

static DB_STATUS ad0_uquel_pmatch(ADF_CB  *adf_scb, UCS2 *sptr, UCS2 *ends,
	UCS2 *pptr, UCS2 *endp, i4 *rcmp, bool bignore);

static DB_STATUS ad0_uquel_lmatch(ADF_CB  *adf_scb, UCS2 *sptr, UCS2 *ends,
	UCS2 *pptr, UCS2 *endp, i4 *rcmp, bool bignore);

static	i4 ad0_uquel_comp(
	UCS2	*sptr,
	UCS2	*ends,
	UCS2	*pptr,
	UCS2	*endp, 
	bool bignore);

static void ad0_uquel_dumpCE(char *label, UCS2 *starta, UCS2 *enda, 
	UCS2 *startb, UCS2 *endb);

/* static routine to identify blank string */
static bool ad0_uquel_isblank(UCS2 *p, UCS2 *e) 
{
    bool isnull = TRUE;
    bool isblank = TRUE;
    int	 i = 0;
    /* weight for a blank character from Unicode collation table */
    UCS2 blankcelist[5] = { 0x0209, 0x0020, 0x0002, 0x0020, 0x0000 };

    if (p < e)
    {
      while (++p < e && *p != ULIKE_NORM && *p != ULIKE_PAT )
      {
	if (*p != 0)
	  isnull = FALSE;

	if (*p != blankcelist[i++])
	   isblank = FALSE;
      }
      return (isnull || isblank);
    }
    else
	return FALSE;
}

/* Macro to position on the next character's CE list */
#define	NextCCE(p , e) \
    while ( ++p < e && *p != ULIKE_NORM \
		    && *p != ULIKE_PAT )

/*{
** Name: adu_ulexcomp - Compare two NCHAR/NVARCHAR string data values using the
**		     QUEL pattern matching characters.
**
** Description:
**	This routine compares two NATIONAL CHARACTER (NCHAR or NVARCHAR) string
**	data values for equality using the quel pattern matching characters '*' 
** 	(match zero or more arbitrary characters) and '?' (match exactly one 
**	arbitrary character). Pattern matching characters are already converted
**	to pattern matching symbols DB_PAT_ANY etc. by the scanner, 
**	prior to this routine. 
**	Both strings can have pattern but this will only happen if users 
**	type in such expression. 
**	Fix me:
**	For the initial implementation we are assuming source has no patterns.
**
**	Shorter strings will be considered less than longer strings.
**
**	Examples:
**
**		 'Smith, Homer' < 'Smith,Homer'
**			'abcd ' > 'abcd'
**			 'abcd' = 'a*d'
**			 'abcd' = 'a?cd*'
**			 'abcd' = 'abcd*'
**			'abcd ' > 'abc?'
**			'abc* ' > 'abcd'
**			 'abcd' < 'abcd?'
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	str_dv				Ptr to DB_DATA_VALUE containing first
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 1st string (for NCHAR) or
**					DB_NVCHR_STRING holding first string
**					(for NVARCHAR).
**	pat_dv				Ptr to DB_DATA_VALUE containing second
**					string.
**	    .db_datatype		Its datatype (assumed to be either
**					NCHAR or NVARCHAR).
**	    .db_length			Its length.
**	    .db_data			Ptr to pattern string (for
**					NCHAR) or DB_NVCHR_STRING holding pat
**					string (for NVARCHAR).
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
**	    E_DB_OK, E_DB_ERROR
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
**	23-Oct-2007 (gupsh01)
**	    Written.
**	06-Nov-2007 (gupsh01)
**	    Always call the specific routine for making a quel pattern.
**	27-Oct-2009 (wanfr01) b122799
**	    Use a local variable for unicode manipulation rather than
**	    allocating a buffer every time
*/
DB_STATUS
adu_ulexcomp(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*str_dv,
DB_DATA_VALUE	*pat_dv,
i4		*rcmp,
bool		bignore)
{
    DB_STATUS	db_stat;
    UCS2	*sptr;
    i4		slen;
    UCS2	*ends;
    UCS2	*pptr;
    i4		plen;
    UCS2	*endp;
    UCS2	local_lista[ADF_UNI_TMPLEN_UCS2];
    UCS2	local_listb[ADF_UNI_TMPLEN_UCS2];
    UCS2	*celists = local_lista;
    UCS2	*celistp = local_listb;

    for (;;)
    {
	/* Get string and its length */
	if ( db_stat = adu_lenaddr(adf_scb, str_dv, &slen, (char **)&sptr) )
	    break;
	ends = sptr + slen / sizeof(UCS2);

	if (str_dv->db_datatype == DB_NCHR_TYPE)
	{
	   while ((ends > sptr) && (*(ends - 1) == U_BLANK)) 
	    ends--;
	}

	/* Get pattern and its length */
	if ( db_stat = adu_lenaddr(adf_scb, pat_dv, &plen, (char **)&pptr) )
	    break;
	endp = pptr + plen / sizeof(UCS2);

	if (pat_dv->db_datatype == DB_NCHR_TYPE)
	{
	   while ((endp > pptr) && (*(endp - 1) == U_BLANK)) 
	    endp--;
	}

	/* Produce CE list for string , could also have a pattern but a rarity */
	if ( db_stat = adu_umakeqpat(adf_scb, sptr, ends, NULL, &celists,
				str_dv->db_collID) )
	    break;
	sptr = celists+1;
	ends = sptr + celists[0];

	/* Produce CE list for pattern */
	if ( db_stat = adu_umakeqpat(adf_scb, pptr, endp, NULL, &celistp,
				str_dv->db_collID) )
	    break;
	pptr = celistp+1;
	endp = pptr + celistp[0];

	/* Execute comparison ... */
	db_stat = ad0_uquel(adf_scb, sptr, ends, pptr, endp, rcmp, bignore);
	break;
    }

    /* We're responsible for freeing the CE lists */
    if ( celists != local_lista)
	MEfree((char*)celists);
    if ( celistp != local_listb)
	MEfree((char*)celistp);

    return(db_stat);
}

/*{
** Name: ad0_uquel - Recursive routine used by adu_ulexcomp() to compare two
**		     NCHAR/NVARCHAR strings
**
**  Description:
**	Main routine for supporting QUEL comparison.
**
**  History:
**	23-Oct-2007 (gupsh01)
**	    Written.
*/
static	DB_STATUS
ad0_uquel(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp,
bool	bignore)
{
    i4	cc;	/* the `character class' for pch */
    i4	stat;

    for (;;)	/* loop through pattern string */
    {
	/* If ignoring blanks get the non blank character first */
	if (bignore) 
	{
	  while ( ad0_uquel_isblank (pptr, endp))
		NextCCE(pptr, endp);
	  while ( ad0_uquel_isblank (sptr, ends))
		NextCCE(sptr, ends);
	}
	/*
	** Get the next character from the pattern string, handling escape
	** sequences if required.
	*/
	if (pptr >= endp)
	{
	    /* end of pattern string */
	    cc = AD_CC6_EOS;
	}
	else
	{
	    if ( *pptr == ULIKE_PAT )
	    {
		/* we have a pattern character */
		/* The pattern character */
		switch (*(++pptr))
		{
		  case DB_PAT_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case DB_PAT_ANY:
		    cc = AD_CC3_ANY;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
		    break;

		  case DB_PAT_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case DB_PAT_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  default:
		    return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
		}
	    }
	    else if ( *pptr == ULIKE_NORM )
		cc = AD_CC0_NORMAL;
	    else
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	}
	
	/* Now we have the next pattern string character and its class */
	switch (cc)
	{
	  case AD_CC0_NORMAL:
	  case AD_CC1_DASH:
	    if (sptr >= ends)
	    {
		*rcmp = -1;	/* string is shorter than pattern */
		return E_DB_OK;
	    }

	    /* in AD_CC1_DASH the pptr has skipped on the 
	    ** dash pattern but sptr still points to the dash, 
	    ** if present,  pptr and sptr should both have the 
	    ** dash then only we wish to proceed comparing 
	    ** further else we can return now.
	    */
            if (cc == AD_CC1_DASH)
            {
              if(*(pptr - 1) == *sptr)
                ++sptr;
              else
              {
                *rcmp = *sptr - *(pptr - 1);
                return E_DB_OK;
              }
            }

	    if ( stat = ad0_uquel_comp(sptr, ends, pptr, endp, bignore) )
	    {
		*rcmp = stat;
		return E_DB_OK;
	    }
	    break;

	  case AD_CC2_ONE:
	    return ad0_uquel_qmatch(adf_scb, sptr, ends, pptr+1, endp, 
							rcmp, bignore);

	  case AD_CC3_ANY:
	    return ad0_uquel_pmatch(adf_scb, sptr, ends, pptr+1, endp, 
							rcmp, bignore);

	  case AD_CC4_LBRAC:
	    return ad0_uquel_lmatch(adf_scb, sptr, ends, pptr+1, endp, 
							rcmp, bignore);

	  case AD_CC5_RBRAC:
	    /*
	    ** ERROR: bad range specification.
	    */
	    return adu_error(adf_scb, E_AD1015_BAD_RANGE, 0);

	  case AD_CC6_EOS:
	    /* End of pattern string: are we at end of other string too? */
	    *rcmp = (sptr != ends) ? 1 : 0;
	    return E_DB_OK;

	  default:
	    return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	}
	NextCCE(pptr, endp);
	NextCCE(sptr, ends);
    }

    /* Should *NEVER* get here */
    return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
}


/*{
**  Name: ad0_uquel_qmatch - Process a `MATCH-ONE' character for QUEL.
**
**  Description:
**	Process a MATCH-ONE character for QUEL comparison.
**
**  History:
**	03-oct-2001 (gupsh01)
**	    Written.
*/
static	DB_STATUS
ad0_uquel_qmatch(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp,
bool	bignore)
{
    while (sptr < ends)
    {
	NextCCE(sptr, ends); /* skip till we get a marker */
	return ad0_uquel(adf_scb, sptr, ends, pptr, endp, rcmp, bignore);
    }
    *rcmp = -1;	/* string is shorter than pattern */
    return E_DB_OK;
}

/*{
**  Name: ad0_uquel_pmatch - Process a `MATCH-ANY' character for QUEL.
**
**  Description:
**	Process a MATCH-ANY character for QUEL comparison.
**
**  History:
**	23-Oct-2007 (gupsh01)
**	    Written.
*/
static	DB_STATUS
ad0_uquel_pmatch(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp,
bool	bignore)
{
    DB_STATUS	db_stat;

    if (pptr >= endp)
    {
	/* Last relevant char in pattern was `MATCH-ANY' so we have a match. */
	*rcmp = 0;
	return E_DB_OK;
    }

    while (sptr <= ends)	    /* must be `<=', not just `<' */
    {
	if ((db_stat = ad0_uquel(adf_scb, sptr, ends, pptr, endp, rcmp, bignore))
		!= E_DB_OK)
	    return db_stat;

	if (*rcmp == 0)
	    return E_DB_OK;	/* match */

	NextCCE(sptr, ends);
    }

    /* Finished with string and no match found ... call it `<' by convention. */
    *rcmp = -1;
    return E_DB_OK;
}

/*{
**  Name: ad0_uquel_lmatch - Process a `range sequence' for QUEL.
**
**  Description:
**	Process a range-sequence for QUEL comparison.
**
**  History:
**	23-Oct-2007 (gupsh01)
**	    Written.
**	08-Sep-2008 (gupsh01)
**	    Quel pattern matching on C type using a range match
**	    and having trailing blanks may give incorrect results.
*/
static	DB_STATUS
ad0_uquel_lmatch(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp,
bool	bignore)
{
    UCS2	*savep;
    i4		cc;
    i4		cur_state = AD_S1_IN_RANGE_DASH_NORM;
    bool	empty_range = TRUE;
    bool	match_found = FALSE;
    bool	blank_seen = FALSE;
    bool	non_blank_seen = FALSE;
    UCS2	*tpptr;

    for (;;)
    {
	/*
	** Get the next character from the pattern string.
	*/
	if (pptr >= endp)
	{
	    return adu_error(adf_scb, E_AD1015_BAD_RANGE, 0);
	}
	else
	{
	    if ( *pptr == ULIKE_PAT )
	    {
		/* we have a pattern character */
		switch (*(++pptr))
		{
		  case DB_PAT_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case DB_PAT_ANY:
		    cc = AD_CC3_ANY;
		    break;

		  case DB_PAT_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case DB_PAT_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
		    break;
		}
	    }
	    else if ( *pptr == ULIKE_NORM )
		cc = AD_CC0_NORMAL;
	    else
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	}

	if (cc == AD_CC6_EOS)
	    return adu_error(adf_scb, E_AD1015_BAD_RANGE, 0);

	if (cc == AD_CC2_ONE || cc == AD_CC3_ANY || cc == AD_CC4_LBRAC)
	    return adu_error(adf_scb, E_AD1016_PMCHARS_IN_RANGE, 0);

	/*
	** Now, we have the next pattern character.  Switch on the current
	** state, then do something depending on what character class
	** the next pattern character falls in. Note, that we should be
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
		return ad0_uquel(adf_scb, sptr, ends, pptr+1, endp, rcmp, bignore);

	      default:
		/* should *NEVER* get here */
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    }
	    break;

	  case AD_S2_IN_RANGE_DASH_IS_OK:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if (bignore) 
		{	
		  if ((ad0_uquel_isblank(pptr, endp)))
                  {
		    if (non_blank_seen)
                      blank_seen = TRUE;
                    break; /* loop to next pattern character */
                  }
		  else
		    non_blank_seen = TRUE;
		}

		if ( sptr < ends && (ad0_uquel_comp(sptr, ends, savep, 
							endp, bignore)) == 0 )
		    match_found = TRUE;
		savep = pptr;
		/* cur_state remains AD_S2_IN_RANGE_DASH_IS_OK */
	        break;

	      case AD_CC1_DASH:
		/* do nothing, but change cur_state */
		cur_state = AD_S3_IN_RANGE_AFTER_DASH;
	        break;

	      case AD_CC5_RBRAC:
	      {
		int ret_val = -1;

		/*
		** end of the range spec, and we have a saved char so range
		** was not empty.
		*/

		if (bignore && blank_seen)
		{
		    tpptr = pptr + 1;
		    /* Skip all blanks in the remaining pattern */
		    while(ad0_uquel_isblank(tpptr, endp))
		        NextCCE(tpptr, endp);

		    if (tpptr < endp) 
		    {	
		      /* If pptr + 1 is a non blank then */
		      /* if bignore and blank_seen then check the source
		      ** without the range.
		      */
		      ret_val = ad0_uquel(adf_scb, sptr, ends, 
					tpptr, endp, rcmp, bignore);
		    }	
		}

		if ((ret_val != 0) || (*rcmp != 0))
		{
		      if ( sptr < ends && 
			(ad0_uquel_comp(sptr, ends, savep, endp, bignore)) == 0 )
		       match_found = TRUE;

		      if (match_found)
		      {
		        NextCCE(sptr, ends);
		        return ad0_uquel(adf_scb, sptr, ends, pptr+1, 
						endp, rcmp, bignore);
		      }
		      else
		      {
		        *rcmp = -1; /* if string not in range, call it < pat */
		        return E_DB_OK;
		      }
		}
		else
		{
		      *rcmp = 0; /* Matched */
		       return 0;
		}
	      }

	      default:
		/* should *NEVER* get here */
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    }
	    break;

	  case AD_S3_IN_RANGE_AFTER_DASH:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if ( (ad0_uquel_comp(savep, endp, pptr, endp, bignore)) <= 0 )
		{
		    if (sptr < ends && (ad0_uquel_comp(sptr, ends, savep, endp, bignore)) >= 0 &&
			    (ad0_uquel_comp(sptr, ends, pptr, endp, bignore)) <= 0)
			match_found = TRUE;
		    cur_state = AD_S4_IN_RANGE_DASH_NOT_OK;
		    break;
		}
		/* fall through to BAD-RANGE ... x-y, where x > y */

	      case AD_CC1_DASH:
	      case AD_CC5_RBRAC:
		return adu_error(adf_scb, E_AD1015_BAD_RANGE, 0);

	      default:
		/* should *NEVER* get here */
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
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
		return adu_error(adf_scb, E_AD1015_BAD_RANGE, 0);

	      case AD_CC5_RBRAC:
		/*
		** end of the range spec, no saved char, and range was not
		** empty.
		*/
		if (match_found)
		{
		    NextCCE(sptr, ends);
		    return ad0_uquel(adf_scb, sptr, ends, pptr+1, endp, rcmp, bignore);
		}
		*rcmp = -1;	/* if string not in range, call it < pat */
		return E_DB_OK;

	      default:
		/* should *NEVER* get here */
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    }
	    break;

	  default:
	    /* should *NEVER* get here */
	    return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	}
	NextCCE(pptr, endp);
    }

    /* should *NEVER* get here */
    return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
}

/*{
**  Name: ad0_uquel_comp - Compare two lists of collation elements.
**
**  Description:
**	Compares two lists of collation elements.
**
**  Inputs:		
**	cea		Pointer to first list of collation elements
**			representing some UCS2 character.
**			The first element is the marker ULIKE_NORM
**			and the last element occurs when the
**			end of the list or one of the special
**			markers ULIKE_NORM,or ULIKE_PAT is encountered.
**	enda		The end of the list.
**
**	ceb		Pointer to second list of collation elements
**			representing some UCS2 character.
**			The first element is the marker ULIKE_NORM
**			and the last element occurs when the
**			end of the list or one of the special
**			markers ULIKE_NORM, or ULIKE_PAT is encountered.
**	endb		The end of the list.
**
**  Returns:
**	<0		if list "a" is shorter than list "b", or the
**			corresponding CE in list "a" is less than 
**			list "b"'s.
**	0		if the collation elements are in all
**			ways equal.
**	>0		if list "a" is longer than list "b", or the
**			corresponding CE in list "a" is greater than
**			list "b"'s.
**
**  History:
**	23-Oct-2007 (gupsh01)
**	    Written.
*/
static	i4
ad0_uquel_comp(
UCS2	*cea,
UCS2	*enda,
UCS2	*ceb,
UCS2	*endb,
bool	bignore)
{
    i4	eoa = 0, eob = 0;

    while ( !eoa && !eob && *cea == *ceb )
    {
	if ( ++cea >= enda || *cea == ULIKE_NORM 
			   || *cea == ULIKE_PAT )
	    eoa++;
	if ( ++ceb >= endb || *ceb == ULIKE_NORM 
			   || *ceb == ULIKE_PAT )
	    eob++;
    }

    if ( eoa || eob )
	/* All CEs are equal, or one list is shorter */
	return(eob - eoa);
    /* Unequal collation element */
    return(*cea - *ceb);
}

/* Name: ad0_uquel_dumpCE - Dumps the collation weight. Only useful 
**	 for debugging.
**  History:
**	23-Oct-2007 (gupsh01)
**	    Written.
*/
static 
void 
ad0_uquel_dumpCE(
char *label, 
UCS2 *starta, 
UCS2 *enda, 
UCS2 *startb, 
UCS2 *endb)
{
    char        *tbsa = (char*)starta;
    char        *tbsb = (char*)startb;
    i4        x = 0;

    TRdisplay("%s : \n", label);
    TRdisplay("\t source : ");
    while ( ++tbsa < (char*)enda)
    {
	MEcopy((char*)tbsa, 2, (char*)&x+2);
	TRdisplay("%04x ", x);
	tbsa++;
    }
    TRdisplay("\n\t Pattern: ");
    while ( ++tbsb < (char*)endb)
    {
	MEcopy((char*)tbsb, 2, (char*)&x+2);
        TRdisplay("%04x ", x);
        tbsb++;
    }
    TRdisplay("\n");
}
