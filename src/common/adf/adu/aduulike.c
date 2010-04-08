/*
** Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <iicommon.h>
#include <adf.h>
#include <ulf.h>
#include <adfint.h>
#include <aduint.h>
#include <aduucol.h>
#include "adustrcmp.h"

/**
**
** Name: aduulike.c -	Routines for implementing LIKE comparison on
**			NATIONAL CHARACTER datatypes (NCHAR, NVARCHAR).
**
** This file includes the following externally visible routines:
**
**	adu_ulike    -	Compares two NCHAR or NVARCHAR string data values for
**			equality. SQL's "LIKE" pattern matching characters are
**			allowed in the second data value ('%' matches any number
**			of characters, including none; '_' matches exactly one
**			character). Also, the ESCAPE character is supported. If
**			the '%' or '_' characters are escaped, they loose their
**			special pattern match meaning, while the '[' and ']'
**			characters, if escaped, take on the QUEL-like range
**			pattern match meaning. This latter feature is an upward
**			compatible extension to the ANSI SQL LIKE predicate.
**
** This file also includes the following static routines:
**
**	ad0_ulike    -	Recursive routine used by adu_ulike() to compare two
**			strings using SQL LIKE pattern matching semantics.
**	ad0_lkqmatch -	Process a `MATCH-ONE' character for LIKE.
**	ad0_lkpmatch -	Process a `MATCH-ANY' character for LIKE.
**	ad0_lklmatch -	Process a `range sequence' for LIKE.
**	ad0_lkcomp   -	Compare two strings of collation elements.
**
**  History:
**	19-apr-2001 (abbjo03)
**	    Created.
**	25-Sep-2002 (jenjo02)
**	    Modified to utilize collation elements rather than 
**	    code points.
**	16-Jan-2002 (jenjo02)
**	    Added dropped case AD_CC1_DASH statement.
**	27-Feb-2002 (gupsh01)
**	    Fixed the AD_CC1_DASH case, to give the correct comparison
**	    results. 
**/


/*
**  Definition of static variables and forward static functions.
*/
static DB_STATUS ad0_ulike(ADF_CB *adf_scb, UCS2 *sptr, UCS2 *ends, UCS2 *pptr,
	UCS2 *endp, i4 *rcmp);

static DB_STATUS ad0_lkqmatch(ADF_CB  *adf_scb, UCS2 *sptr, UCS2 *ends,
	UCS2 *pptr, UCS2 *endp, i4 *rcmp);

static DB_STATUS ad0_lkpmatch(ADF_CB  *adf_scb, UCS2 *sptr, UCS2 *ends,
	UCS2 *pptr, UCS2 *endp, i4 *rcmp);

static DB_STATUS ad0_lklmatch(ADF_CB  *adf_scb, UCS2 *sptr, UCS2 *ends,
	UCS2 *pptr, UCS2 *endp, i4 *rcmp);

static	i4 ad0_lkcomp(
	UCS2	*sptr,
	UCS2	*ends,
	UCS2	*pptr,
	UCS2	*endp);

/* Macro to position on the next character's CE list */
#define	NextCCE(p , e) \
    while ( ++p < e && *p != ULIKE_NORM \
		    && *p != ULIKE_ESC \
		    && *p != ULIKE_PAT )

/*{
** Name: adu_ulike - Compare two NCHAR/NVARCHAR string data values using the
**		     LIKE operator pattern matching characters.
**
** Description:
**	This routine compares two NATIONAL CHARACTER (NCHAR or NVARCHAR) string
**	data values for equality using the LIKE operator pattern matching
**	characters '%' (match zero or more arbitrary characters) and '_' (match
**	exactly one arbitrary character). These pattern matching characters only
**	have special meaning in the second data value, patdv. Shorter strings
**	will be considered less than longer strings. (Since LIKE only knows
**	about whether the two are "equal" or not, this should not matter.)
**
**	Examples:
**
**		 'Smith, Homer' < 'Smith,Homer'
**
**			'abcd ' > 'abcd'
**
**			 'abcd' = 'a%d'
**
**			 'abcd' = 'a_cd%'
**
**			 'abcd' = 'abcd%'
**
**			'abcd ' > 'abc_'
**
**			'abc% ' < 'abcd' (Note % treated literally in 1st)
**
**			 'abcd' < 'abcd_'
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
**					string.  This is the pattern string.
**	    .db_datatype		Its datatype (assumed to be either
**					NCHAR or NVARCHAR).
**	    .db_length			Its length.
**	    .db_data			Ptr to pattern string (for
**					NCHAR) or DB_NVCHR_STRING holding pat
**					string (for NVARCHAR).
**	eptr				Ptr to an UCS2 which will be used as
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
**	19-apr-2001 (abbjo03)
**	    Written.
**	25-Sep-2002 (jenjo02)
**	    Modified to utilize collation elements rather than 
**	    code points.
**	4-feb-05 (inkdo01)
**	    Add collation ID to adu_umakelces/adu_umakelpat calls.
**	27-Oct-2009 (wanfr01) b122799
**	    Use a local variable for unicode manipulation rather than
**	    allocating a buffer every time
*/
DB_STATUS
adu_ulike(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*str_dv,
DB_DATA_VALUE	*pat_dv,
UCS2		*eptr,
i4		*rcmp)
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

	/* Get pattern and its length */
	if ( db_stat = adu_lenaddr(adf_scb, pat_dv, &plen, (char **)&pptr) )
	    break;
	endp = pptr + plen / sizeof(UCS2);

	/* Produce CE list for string */
	if ( db_stat = adu_umakelces(adf_scb, sptr, ends, &celists,
				str_dv->db_collID) )
	    break;
	sptr = celists+1;
	ends = sptr + celists[0];

	/* Produce CE list for pattern */
	if ( db_stat = adu_umakelpat(adf_scb, pptr, endp, eptr, &celistp,
				str_dv->db_collID) )
	    break;
	pptr = celistp+1;
	endp = pptr + celistp[0];

	/* Execute like predicate... */
	db_stat = ad0_ulike(adf_scb, sptr, ends, pptr, endp, rcmp);
	break;
    }

    /* Free allocated memory if it was used instead of local buffers */
    if ( celists != local_lista )
	MEfree((char*)celists);
    if ( celistp != local_listb )
	MEfree((char*)celistp);

    return(db_stat);
}


/*{
** Name: ad0_ulike - Recursive routine used by adu_ulike() to compare two
**		     NCHAR/NVARCHAR strings
**
**  Description:
**	Main routine for supporting SQL's LIKE operator.
**
**  History:
**	19-apr-2001 (abbjo03)
**	    Written.
**	25-Sep-2002 (jenjo02)
**	    Modified to utilize collation elements rather than 
**	    code points.
**	16-Jan-2002 (jenjo02)
**	    Added dropped case AD_CC1_DASH statement.
**      27-Feb-2002 (gupsh01)
**          Fixed the AD_CC1_DASH case, to give the correct comparison
**          results. 
**	08-Oct-2007 (gupsh01)
**	    Support quel pattern matching syntax.
*/
static	DB_STATUS
ad0_ulike(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp)
{
    i4	cc;	/* the `character class' for pch */
    i4	stat;

    for (;;)	/* loop through pattern string */
    {
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
	    if ( *pptr == ULIKE_ESC )
	    {
		/* we have an escape sequence */

		/* The escaped character */
		switch (*(++pptr))
		{
		  case ULIKE_NORM:
		    cc = AD_CC0_NORMAL;
		    break;

		  case AD_3LIKE_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case AD_4LIKE_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  default:
		    /* ERROR: illegal escape sequence */
		    return adu_error(adf_scb, E_AD1018_BAD_ESC_SEQ, 0);
		    break;
		}
	    }
	    else if ( *pptr == ULIKE_PAT )
	    {
		/* we have a pattern character */

		/* The pattern character */
		switch (*(++pptr))
		{
		  case AD_1LIKE_ONE:
		  case DB_PAT_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case AD_2LIKE_ANY:
		  case DB_PAT_ANY:
		    cc = AD_CC3_ANY;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
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

	    if ( stat = ad0_lkcomp(sptr, ends, pptr, endp) )
	    {
		*rcmp = stat;
		return E_DB_OK;
	    }
	    break;

	  case AD_CC2_ONE:
	    return ad0_lkqmatch(adf_scb, sptr, ends, pptr+1, endp, rcmp);

	  case AD_CC3_ANY:
	    return ad0_lkpmatch(adf_scb, sptr, ends, pptr+1, endp, rcmp);

	  case AD_CC4_LBRAC:
	    return ad0_lklmatch(adf_scb, sptr, ends, pptr+1, endp, rcmp);

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
**  Name: ad0_lkqmatch - Process a `MATCH-ONE' character for LIKE.
**
**  Description:
**	Process a MATCH-ONE character for LIKE operator.
**
**  History:
**	19-apr-2001 (abbjo03)
**	    Written.
*/
static	DB_STATUS
ad0_lkqmatch(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp)
{
    while (sptr < ends)
    {
	NextCCE(sptr, ends);
	return ad0_ulike(adf_scb, sptr, ends, pptr, endp, rcmp);
    }
    *rcmp = -1;	/* string is shorter than pattern */
    return E_DB_OK;
}


/*{
**  Name: ad0_lkpmatch - Process a `MATCH-ANY' character for LIKE.
**
**  Description:
**	Process a MATCH-ANY character for LIKE operator.
**
**  History:
**	19-apr-2001 (abbjo03)
**	    Written.
**	01-Feb-2007 (kiria01) b117544
**	    Stop looking for runs of 'any' wildcard chars in the runtime compare
**	    code as MakeCE can do it the once and also take care with the
**	    ULIKE_PAT prefix character in the ce buffer.
*/
static	DB_STATUS
ad0_lkpmatch(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp)
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
	if ((db_stat = ad0_ulike(adf_scb, sptr, ends, pptr, endp, rcmp))
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
**  Name: ad0_lklmatch - Process a `range sequence' for LIKE.
**
**  Description:
**	Process a range-sequence for LIKE operator.
**
**  History:
**	19-apr-2001 (abbjo03)
**	    Written.
**	25-Sep-2002 (jenjo02)
**	    Modified to utilize collation elements rather than 
**	    code points.
*/
static	DB_STATUS
ad0_lklmatch(
ADF_CB	*adf_scb,
UCS2	*sptr,
UCS2	*ends,
UCS2	*pptr,
UCS2	*endp,
i4	*rcmp)
{
    UCS2	*savep;
    i4		cc;
    i4		cur_state = AD_S1_IN_RANGE_DASH_NORM;
    bool	empty_range = TRUE;
    bool	match_found = FALSE;

    for (;;)
    {
	/*
	** Get the next character from the pattern string, handling escape
	** sequences if required.
	*/
	if (pptr >= endp)
	{
	    return adu_error(adf_scb, E_AD1015_BAD_RANGE, 0);
	}
	else
	{
	    if ( *pptr == ULIKE_ESC )
	    {
		/* we have an escape sequence */
		switch (*(++pptr))
		{
		  case ULIKE_NORM:
		    cc = AD_CC0_NORMAL;
		    break;

		  case AD_3LIKE_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case AD_4LIKE_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  default:
		    /* ERROR: illegal escape sequence */
		    return adu_error(adf_scb, E_AD1018_BAD_ESC_SEQ, 0);
		}
	    }
	    else if ( *pptr == ULIKE_PAT )
	    {
		/* we have a pattern character */
		switch (*(++pptr))
		{
		  case AD_1LIKE_ONE:
		  case DB_PAT_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case AD_2LIKE_ANY:
		  case DB_PAT_ANY:
		    cc = AD_CC3_ANY;
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
		return ad0_ulike(adf_scb, sptr, ends, pptr+1, endp, rcmp);

	      default:
		/* should *NEVER* get here */
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    }
	    break;

	  case AD_S2_IN_RANGE_DASH_IS_OK:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if ( sptr < ends && (ad0_lkcomp(sptr, ends, savep, endp)) == 0 )
		    match_found = TRUE;
		savep = pptr;
		/* cur_state remains AD_S2_IN_RANGE_DASH_IS_OK */
	        break;

	      case AD_CC1_DASH:
		/* do nothing, but change cur_state */
		cur_state = AD_S3_IN_RANGE_AFTER_DASH;
	        break;

	      case AD_CC5_RBRAC:
		/*
		** end of the range spec, and we have a saved char so range
		** was not empty.
		*/
		if ( sptr < ends && (ad0_lkcomp(sptr, ends, savep, endp)) == 0 )
		    match_found = TRUE;

		if (match_found)
		{
		    NextCCE(sptr, ends);
		    return ad0_ulike(adf_scb, sptr, ends, pptr+1, endp, rcmp);
		}
		*rcmp = -1;	/* if string not in range, call it < pat */
		return E_DB_OK;

	      default:
		/* should *NEVER* get here */
		return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    }
	    break;

	  case AD_S3_IN_RANGE_AFTER_DASH:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if ( (ad0_lkcomp(savep, endp, pptr, endp)) <= 0 )
		{
		    if (sptr < ends && (ad0_lkcomp(sptr, ends, savep, endp)) >= 0 &&
			    (ad0_lkcomp(sptr, ends, pptr, endp)) <= 0)
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
		    return ad0_ulike(adf_scb, sptr, ends, pptr+1, endp, rcmp);
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
**  Name: ad0_lkcomp - Compare two lists of collation elements.
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
**			markers ULIKE_NORM, ULIKE_ESC, or
**			ULIKE_PAT is encountered.
**	enda		The end of the list.
**
**	ceb		Pointer to second list of collation elements
**			representing some UCS2 character.
**			The first element is the marker ULIKE_NORM
**			and the last element occurs when the
**			end of the list or one of the special
**			markers ULIKE_NORM, ULIKE_ESC, or
**			ULIKE_PAT is encountered.
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
**	23-Sep-2002 (jenjo02)
**	    Written.
*/
static	i4
ad0_lkcomp(
UCS2	*cea,
UCS2	*enda,
UCS2	*ceb,
UCS2	*endb)
{
    i4	eoa = 0, eob = 0;

    while ( !eoa && !eob && *cea == *ceb )
    {
	if ( ++cea >= enda || *cea == ULIKE_NORM 
			   || *cea == ULIKE_ESC 
			   || *cea == ULIKE_PAT )
	    eoa++;
	if ( ++ceb >= endb || *ceb == ULIKE_NORM 
			   || *ceb == ULIKE_ESC 
			   || *ceb == ULIKE_PAT )
	    eob++;
    }

    if ( eoa || eob )
	/* All CEs are equal, or one list is shorter */
	return(eob - eoa);
    /* Unequal collation element */
    return(*cea - *ceb);
}
