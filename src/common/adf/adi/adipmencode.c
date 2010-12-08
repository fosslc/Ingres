/*
** Copyright (c) 1987, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADIPMENCODE.C - Massage string constants.
**
**  Description:
**      Massage string constants or user parameters with backslash sequences or
**	QUEL pattern matching.  
**
**          adi_pm_encode() - Massage string constants.
**
**
**  History:
**      03-apr-87 (daved)
**          written
**      20-apr-87 (thurston)
**          Major revisions to be consistent with 5.0, and to conform to ADF
**          conventions.
**      23-sep-87 (thurston)
**          Added the adi_pmchars argument to adi_pm_encode() to tell the caller
**	    whether any PM characters were encoded or not.
**      15-oct-87 (thurston)
**          Complete revision.  Name `adi_pm_encode' is now a bit of a misnomer,
**	    since the routine does a hell of a lot more than just that.
**      06-nov-87 (thurston)
**          Added the extra semantics to the ADI_DO_PM request to
**	    adi_pm_encode() that allows it to do a minimal
**          amount of `backslash-escaping' for PM characters.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	23-jun-88 (jrb)
**	    Partial re-write for Kanji support.
**	03-may-89 (jrb)
**	    Changed code to allow all ranges in [] pattern matching for QUEL
**	    when the second value of the range was specified using the \xxx
**	    syntax.  This value should really be interpreted and checked, but
**	    this will involved a re-write of this routine.
**	17-may-90 (jrb)
**	    Fix for bug #21347: left bracket without matching right bracket was
**	    not generating an error.  This was occurring because we would encode
**	    the left bracket, find the error, return the error, and then (after
**	    the system automatically retried the query) the second time we would
**	    pass the string through without an error because the encoded left
**	    bracket didn't get range-checking done on it.
**      22-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	10-may-94 (kirke)
**	    Changed comparison from != 0 to > 0 in adi_pm_encode().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: adi_pm_encode() - Massage string constants.
**
** Description:
**      Massage string constants or user parameters with backslash sequences or
**	QUEL pattern matching.  This routine can be used to optionally perform
**	any combination of the following:
**	    o  Encode QUEL pattern match characters.
**	    o  Recognize and do the appropriate translations of
**		backslash sequences.
**	    o  Modify the .db_length field of the input DB_DATA_VALUE if the
**		length of the string changed due to this massaging.
**
**	This routine also does the following non-optional jobs:
**	    o  Update the input line counter if the `input' string (before
**		translation) contained any newlines.
**	    o  Report an error if an illegal character is found in the `input'
**		string, or if a requested translation would produce an illegal
**              character.
**	    o  Return a boolean telling the caller if the `output' string (after
**		translation) contains any QUEL pattern match characters.
**
**	All translations are done in-place.  If input is not a string datatype,
**	then this routine simply returns with E_DB_OK.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adi_pm_reqs			This is a bit map whose `on' bits tell
**					adi_pm_encode() what optional jobs it is
**					to perform, as follows:
**			    ADI_DO_PM:
**				If this bit is set, any QUEL pattern match
**                              characters found in the input string will be
**                              translated into their internal form.  A minimal
**				amount of `backslash-escaping' will also be
**				performed with this bit set; a backslash in
**				front of a PM character will escape the PM
**				character.  This is to be compatible with the
**				5.0 version of INGRES.  This bit should only be
**                              used for QUEL string constants and user
**                              parameters in the qualification.  It should
**                              never be used for SQL. 
**			    ADI_DO_BACKSLASH:
**				If this bit is set, a backslash character found
**                              in the input string will be interpretted as an
**                              `escape' character.  There are four classes of
**                              following characters:
**				    (1)	QUEL PM characters, which will NOT be
**					interpretted as such; 
**				    (2) One to three octal digits, which will be
**                                      translated into the corresponding
**                                      ASCII/EBCDIC character; 
**				    (3) 'n', 't', 'b', 'r', and 'f', which will
**                                      be translated into its appropriate
**					backslash counterpart (e.g. 'n' becomes
**					a newline character)
**				    (4) All other characters, which will be
**                                      translated into themselves. 
**				If this bit is NOT set, backslashes will be
**				treated just like a normal character.  This bit
**				should be used for all QUEL string constants and
**				user parameters, whether in the target list or
**				in the qualification.  It should never be used
**				for SQL.
**			    ADI_DO_MOD_LEN:
**				If this bit is set, the .db_length field of
**				adi_patdv will be decremented appropriately if
**				the length of the string is reduced due to some
**				requested translation (e.g. if backslash
**				processing is requested and the input string
**				has a '\\' followed by "123", then the length
**				of the output string would be three less).  If
**				this bit is NOT set, then the .db_length field
**				will be left alone and the string will be
**				either blank padded (for `c' and `char') or the
**				count field will be decremented (for `text' and
**				`varchar').  Note that the count field will
**				always be decremented for `text' and `varchar'
**				regardless of whether or not this bit is `on'.
**      adi_patdv                       Ptr to data value holding the string to
**					be encoded.
**	    .db_datatype		Its datatype; If not a string type, this
**					routine just returns E_DB_OK.
**	    .db_length			Its length.
**	    .db_data			Ptr to the data.  If `c' or `char' this
**					points to the string, blank padded.  If
**					`text' or `varchar', this points to a
**					DB_TEXT_STRING.
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
**      adi_patdv	 		Encoded string.
**	    .db_data			Pointer to data with special QUEL
**					pattern match characters encoded.  If
**					any escape characters (backslashes)
**					were included in the input string, then
**					the output string will either be blank
**					padded (for `char' and `c') or shortened
**					(for `varchar', `text', and `longtext').
**	adi_line_num			This will be incremented by the number
**					of actual newlines in the INPUT
**					string.  The scanner uses a line count
**					to properly display some error messages.
**                                      Therefore, it needs to know this
**                                      information when scanning a query.
**                                      Also, one of the error messages that now
**                                      gets formatted by this routine is the
**                                      one for `an illegal character in a
**                                      string constant,' and this error uses
**                                      the line number as a parameter, so it is
**                                      needed for that reason, as well. 
**      adi_pmchars	 		Set to TRUE if any pattern match chars
**					were encoded, FALSE if not.
**
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
**          E_AD0000_OK                 Operation succeeded.
**	    E_AD1015_BAD_RANGE		String contained a `[' without a closing
**					']', or a `]' without an opening '['.
**	    E_AD3050_BAD_CHAR_IN_STRING An illegal character was found in a
**					string constant.  This gets translated
**                                      into user error number 2170 (0x0A96),
**                                      and reported as such.  (Well, formatted
**                                      as such anyway.) 
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-apr-87 (daved)
**          written
**      20-apr-87 (thurston)
**          Major revisions to be consistent with 5.0, and to conform to ADF
**          conventions.
**      23-sep-87 (thurston)
**          Added the adi_pmchars argument to tell the caller whether any PM
**	    characters were encoded or not.
**      15-oct-87 (thurston)
**          Complete revision.  Name `adi_pm_encode' is now a bit of a misnomer,
**	    since the routine does a hell of a lot more than just that.
**      06-nov-87 (thurston)
**          Added the extra semantics to ADI_DO_PM that allow it to do a minimal
**          amount of `backslash-escaping' for PM characters.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	23-jun-88 (jrb)
**	    Partial re-write for Kanji support.
**	03-may-89 (jrb)
**	    Changed code to allow all ranges in [] pattern matching for QUEL
**	    when the second value of the range was specified using the \xxx
**	    syntax.  This value should really be interpreted and checked, but
**	    this will involved a re-write of this routine.
**	17-may-90 (jrb)
**	    Put the DB_PAT_* case labels next to their character counterparts.
**	    Previously they were all together and were just setting the
**	    adi_pmchars value to TRUE, but this way they do range-checking too.
**	    Fix for bug #21347.
**	10-may-94 (kirke)
**	    Defensive programming - changed comparison from != 0 to > 0.
**	    In certain double-byte situations, the loop will skip past 0
**	    into negative numbers and never terminate.
**	25-nov-2009 (kiria01, wanfr01) b122960
**	    Consolidate pattern matching characters here, rather than in adu
**	    so that consolidation only happens once per query.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adi_pm_encode(
ADF_CB		*adf_scb,
i4		adi_pm_reqs,
DB_DATA_VALUE	*adi_patdv,
i4		*adi_line_num,
bool		*adi_pmchars)
{
    register u_char	*from;
    register u_char	*to;
    register u_char	*endp;
    register u_char	*prev;
    register u_char	c;
    i4			n_ = 0;
    bool		star = FALSE;
    u_char		*out_bp;
    DB_STATUS		db_stat = E_DB_OK;
    i4		in_len;
    bool		isrange = FALSE;
    i4			nskips;
    i4			do_pm	    = adi_pm_reqs & ADI_DO_PM;
    i4			do_bs	    = adi_pm_reqs & ADI_DO_BACKSLASH;
    i4			do_modlen   = adi_pm_reqs & ADI_DO_MOD_LEN;
    DB_DT_ID		bdt;


    if (adi_patdv == NULL  ||  adi_patdv->db_data == NULL)
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

	
    *adi_pmchars = FALSE;	/* Assume no PM characters to begin with */

    
    /* Only encode string types */
    /* ------------------------ */
    bdt = abs(adi_patdv->db_datatype);
    switch (adi_patdv->db_datatype)
    {
      case -DB_CHR_TYPE:
      case -DB_CHA_TYPE:
	if (ADI_ISNULL_MACRO(adi_patdv))
	    return (E_DB_OK);	/* NULL value, return OK */
	in_len	= adi_patdv->db_length - 1;
	from    = (u_char *)adi_patdv->db_data;
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	in_len	= adi_patdv->db_length;
	from	= (u_char *)adi_patdv->db_data;
	break;

      case -DB_TXT_TYPE:
      case -DB_LTXT_TYPE:
      case -DB_VCH_TYPE:
	if (ADI_ISNULL_MACRO(adi_patdv))
	    return (E_DB_OK);	/* NULL value, return OK */
	/* Specifically no `break' stmt here */
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VCH_TYPE:
	in_len	= ((DB_TEXT_STRING*)adi_patdv->db_data)->db_t_count;
	from	= ((DB_TEXT_STRING*)adi_patdv->db_data)->db_t_text;
	break;

      default:
	return (E_DB_OK);   /* Not a string, return OK */
    }


    /* Encode characters */
    /* ----------------- */

    out_bp = prev = to = from;
    endp   = from + in_len;

    while (from < endp)
    {
	switch (c = *from)
	{
	  case (u_char) '\n':
	    (*adi_line_num)++;
	    break;

	  case (u_char) '*':
	  case (u_char) DB_PAT_ANY:
	    if (do_pm)
	    {
		if (isrange)
		    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
		*adi_pmchars = TRUE;
		star = TRUE;
		from++;
		continue;
	    }
	    break;

	  case (u_char) '?':
	  case (u_char) DB_PAT_ONE:
	    if (do_pm)
	    {
		if (isrange)
		    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
		*adi_pmchars = TRUE;
		n_++;
		from++;
		continue;
	    }
	    break;

	  case (u_char) '[':
	  case (u_char) DB_PAT_LBRAC:
	    if (do_pm)
	    {
		if (isrange)
		    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

		isrange = TRUE;
		c = (u_char) DB_PAT_LBRAC;
	    }
	    break;

	  case (u_char) ']':
	  case (u_char) DB_PAT_RBRAC:
	    if (do_pm)
	    {
		if (!isrange)
		    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

		isrange = FALSE;
		c = (u_char) DB_PAT_RBRAC;
		*adi_pmchars = TRUE;
	    }
	    break;

	  case (u_char) '-':
	    if (isrange)
	    {
		/* JRBFIX: The following code allows any \xxx character to  */
		/* pass on as a valid range, but we really need to	    */
		/* translate it to a value and then check (but for Kanji    */
		/* that means we have to translate n characters ahead	    */
		/* before calling CMcmpcase).  The correct solution is to   */
		/* change adi_pm_encode to run in two passes: first pass    */
		/* encodes pattern matching and backslash sequences, and    */
		/* the second pass does validation.			    */
		if (    from + 1 >= endp
		     || *(from + 1) == (u_char) ']'
		     || (CMcmpcase(prev, from + 1) > 0
			&& *(from + 1) != '\\')
		   )
		    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	    }
	    break;

	  case (u_char) '\\':
	    if (do_pm  &&  !do_bs)  /* Do minimal backslash escaping for PM */
	    {
		if (from + 1 < endp)	/* if more chars in string */
		{
		    switch (*(from + 1))    /* Is next char a PM char? */
		    {
		      case (u_char) '*':
		      case (u_char) '?':
		      case (u_char) '[':
		      case (u_char) ']':
			c = *(++from);	    /* Yes, escape it always. */
			break;

		      default:
			break;		    /* No, treat '\' as normal char */
		    }
		}
	    }
	    else if (do_bs)
	    {
		if (from + 1 < endp)	/* if more chars in string */
		{
		    c = *(++from);
		    switch (c)
		    {
		      case (u_char) 'n':
			c = (u_char) '\n';
			break;

		      case (u_char) 't':
			c = (u_char) '\t';
			break;

		      case (u_char) 'b':
			c = (u_char) '\b';
			break;

		      case (u_char) 'r':
			c = (u_char) '\r';
			break;

		      case (u_char) 'f':
			c = (u_char) '\f';
			break;

		      case (u_char) '0':
		      case (u_char) '1':
		      case (u_char) '2':
		      case (u_char) '3':
		      case (u_char) '4':
		      case (u_char) '5':
		      case (u_char) '6':
		      case (u_char) '7':
			{
			    i4	    digct = 0;
			    i4	    accum = 0;
			    bool    illchar;

			    while (	digct++ < 3
				    &&	c >= (u_char) '0'
				    &&	c <= (u_char) '7'
				  )
			    {
				accum = accum * 8 + (c - (u_char) '0');
				if (++from < endp)
				    c = *from;
				else
				    break;
			    }
			    from--;

			    /* Now check accum for an illegal character */
			    illchar = FALSE;
			    if (    accum > 0xFF
				|| (bdt == DB_TXT_TYPE  &&  accum == 0)
			       )
			    {
				illchar = TRUE;
			    }
			    else if (bdt == DB_CHR_TYPE)
			    {
				char	ch_tmp = (char) accum;

				/* JRBFIX: The following is incorrect.  If  */
				/* ch_tmp points to the first of a double   */
				/* byte character then the CMprint macro    */
				/* will be examining garbage memory for the */
				/* second byte.  Fixing this whole routine  */
				/* by the method outlined above is needed.  */
				if (!CMprint(&ch_tmp))
				    illchar = TRUE;
			    }

			    if (illchar)
			    {
				return (adu_error(adf_scb,
						E_AD3050_BAD_CHAR_IN_STRING,
						2, 
						(i4) sizeof(*adi_line_num),
						(i4 *) adi_line_num
						)
				       );
			    }

			    c = (u_char) accum;
			}
			break;

		      default:
			break;
		    }
		}
	    }
	    break;
	    
	  default:
	    break;
	}
	while (n_)
	{
	    n_--;
	    *to++ = DB_PAT_ONE;
	}
	if (star)
	{
	    *to++ = DB_PAT_ANY;
	    star = FALSE;
	}
	prev = to;

	if (CMdbl1st(from))
	{
	    CMcpyinc(from, to);
	}
	else
	{
	    *to++ = c;
	    from++;
	}
    }
    while (n_--)
	*to++ = DB_PAT_ONE;
    if (star)
	*to++ = DB_PAT_ANY;
    if (isrange)
	return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

    /* see if length changed and if so set nskips to the difference */
    if ((nskips = in_len - (to - out_bp)) > 0 )
    {
	if (do_modlen)
	    adi_patdv->db_length -= nskips;
		
	switch (abs(adi_patdv->db_datatype))
	{
	  case DB_CHA_TYPE:
	  case DB_CHR_TYPE:
	    while (nskips--)
		*to++ = (u_char) ' ';
	    break;

	  case DB_VCH_TYPE:
	  case DB_TXT_TYPE:
	  case DB_LTXT_TYPE:
	    ((DB_TEXT_STRING *)adi_patdv->db_data)->db_t_count -= nskips;
	    while (nskips--)
		*to++ = (u_char) NULLCHAR;
	    break;
	}
    }

    return (db_stat);
}
