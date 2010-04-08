/*
**  Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cm.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/*
**  Name: ADUCOMPATSTR.C - Routines to perform f.i.'s on "char", "varchar", and
**			   "text", only.
**
**  Description:
**	Implements routines to perform the string type function instances that
**	are defined only for "char", "varchar", and "text" (i.e. not for "c").
**
**          adu_pad() - Pad the input string with blanks to the full length of
**		        the output string.
**	    adu_squeezewhite() - Squeezes whitespace from a string.
**	    adu_trim() - Trim blanks off of the end of a string.
**	    adu_atrim() - Trim arb. chars from front or back.
**	    adu_ltrim() - Trim bloanks from front of a string.
**	    adu_chr() - return ASCII char of input int value.
**
**  Function prototypes defined in ADUINT.H
**
**  History:    $Log-for RCS$
**      2-may-86 (ericj)    
**          Brought these routines from separate files into this one file.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	29-sep-86 (thurston)
**	    Documented all routines as workable for varchar as well as text.
**	30-sep-86 (thurston)
**	    Added code adu_squeezewhite() to enable that routine to work for
**	    char as well as text and varchar.
**	25-feb-88 (thurston)
**	    Modified all routines to accept any combination of character string
**	    datatypes as input and output, and made them more robust in order to
**	    catch certain warnings.
**	28-jun-88 (jrb)
**	    Made changes for Kanji support.
**	01-sep-88 (thurston)
**	    Increased size of local bufs from DB_MAXSTRING to DB_GW4_MAXSTRING.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-jun-2001 (somsa01)
**	    In adu_pad() and adu_squeezewhite(), to save stack space, allocate
**	    space for the temp buffer array dynamically.
**	21-nov-2002 (somsa01)
**	    In adu_squeezewhite(), make sure we free buf AFTER running
**	    adu_movestring().
*/


/*{
** Name: adu_pad() - Pad the input string with blanks to the full length of
**		     the output string.
**
** Description:
**        This routine pads a input string with blanks to its full length in an
**	output string.  The input and output strings can be of any legal INGRES
**	string type, whether it makes sense to do this or not.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing string to be
**					padded.
**	    .db_length			Its length.
**	    .db_data			Ptr to string to be padded.
**	rdv				DB_DATA_VALUE describing result string
**					to hold padded result.
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
**	rdv
**	    .db_data			string hoding padded result.
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
**	    E_AD0000_OK			Successful Completion.
**	    E_AD5001_BAD_STRING_TYPE	The input DB_DATA_VALUE was not a
**					string type.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-apr-84 (lichtman)
**	    written.
**	2-may-86 (ericj)
**          Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-feb-88 (thurston)
**	    Modified to accept any combo of string types.
**	01-sep-88 (thurston)
**	    Increased size of local buf from DB_MAXSTRING to DB_GW4_MAXSTRING.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	11-jun-2001 (somsa01)
**	    To save stack space, allocate space for the temp buffer array
**	    dynamically.
**	12-aug-02 (inkdo01)
**	    Modification to above to impose MEreqmem overhead only if string is
**	    bigger than 2004.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_pad(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *rdv)
{
    u_char		*inptr;
    u_char		*outptr;
    i4			inlen;
    i4			outlen;
    DB_DT_ID		in_dt;
    DB_DT_ID		out_dt;
    bool		var_result;
    DB_STATUS		db_stat;


    switch (in_dt = dv1->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
	inlen = dv1->db_length;
	inptr = (u_char *) dv1->db_data;
	break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
	inlen = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	inptr = ((DB_TEXT_STRING *) dv1->db_data)->db_t_text;
	break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    switch (out_dt = rdv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
	var_result = FALSE;
	outlen = rdv->db_length;
	outptr = (u_char *) rdv->db_data;
	break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
	var_result = TRUE;
	outlen = rdv->db_length - DB_CNTSIZE;
	outptr = ((DB_TEXT_STRING *) rdv->db_data)->db_t_text;
	break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    /*
    ** Check input and output datatypes to see if we need to use
    ** adu_movestring() to catch any possible `null char in text' or
    ** `non-print char in c' warnings.  If we don't have to worry about
    ** this, then just move the string into the destination directly.
    ** Otherwise, first move it into buf[] and then use adu_movestring()
    ** to place in its final location.
    */
    if ((out_dt == DB_CHR_TYPE && in_dt != DB_CHR_TYPE) ||
	(out_dt == DB_TXT_TYPE && in_dt != DB_CHR_TYPE && in_dt != DB_TXT_TYPE))
    {
	/* Must use adu_movestring() */
	u_char		localbuf[2004];
	u_char		*buf;
	bool		uselocal = (outlen <= 2004);

	if (!uselocal)
	    buf = (u_char *)MEreqmem(0, DB_GW4_MAXSTRING, FALSE, NULL);
	else buf = &localbuf[0];

	MEmove(inlen,
		(PTR) inptr,
	       (char) ' ',
	       outlen,
		(PTR) &buf[0]);

	db_stat = adu_movestring(adf_scb, &buf[0], outlen, dv1->db_datatype, rdv);

	if (!uselocal)
	    MEfree((PTR)buf);
    }
    else
    {
	/* Can move directly into the destination DB_DATA_VALUE */
	MEmove(inlen,
		(PTR) inptr,
	       (char) ' ',
	       outlen,
		(PTR) outptr);

	if (var_result)
	    ((DB_TEXT_STRING *) rdv->db_data)->db_t_count = outlen;

	db_stat = E_DB_OK;
    }

    return (db_stat);
}


/* 
** Name: adu_squeezewhite() - Squeezes whitespace from a string.
**
** Description:
**	Trims whitespace off of front and back of string, and changes all
**  other whitespace sequences to single blanks.  Whitespace is defined as
**  spaces, tabs, newlines, form-feeds, carriage returns, and NULLs (for char
**  and varchar, since they can't appear in c or text).
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE describing string
**					to be squeezed.
**	    .db_length			Its length.
**	    .db_data			Ptr to data area containing string
**					to be squeezed.
**
** Outputs:
**	rdv				Ptr to DB_DATA_VALUE describing result
**					string.
**	    .db_data			Ptr to string which will hold the
**					squeezed result.
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
**	    E_AD0000_OK			Successful completion.
**	    E_AD5001_BAD_STRING_TYPE	The input DB_DATA_VALUE was not a
**					string type.
**
**  History:
**      06/01/83 (lichtman) -- written
**	2-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	30-sep-86 (thurston)
**	    Added code to enable routine to work for char as well as text and
**	    varchar.
**	25-feb-88 (thurston)
**	    Modified the routine to be much more robust, and allow any
**	    combination of character string datatypes as input and output.
**	28-jun-88 (jrb)
**	    Made changes for Kanji support.
**	01-sep-88 (thurston)
**	    Increased size of local buf from DB_MAXSTRING to DB_GW4_MAXSTRING.
**	11-jun-2001 (somsa01)
**	    To save stack space, allocate space for the temp buffer array
**	    dynamically.
**	12-aug-02 (inkdo01)
**	    Modification to above to impose MEreqmem overhead only if string is
**	    bigger than 2004.
**	21-nov-2002 (somsa01)
**	    Make sure we free buf AFTER executing adu_movestring().
**	19-Oct-2006 (kiria01) b116284
**	    Use CMwhite instead of local macro so that CM attributes are used
**	    and use the same algorithm as the NVARCHAR SQUEEZE.
**	25-Apr-2006 (kiria01) b118201
**	    Remove NULLs along with CMwhite chars.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_squeezewhite(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    u_char		localbuf[2004];
    u_char		*buf = localbuf;
    register u_char	*outchar;
    register u_char	*inchar;
    register u_char	*endinchar;
    register i4	outlength;
    bool		varlen_result;
    bool		wrote_space = FALSE;
    bool		uselocal = (dv1->db_length <= 2004);
    STATUS		localstatus;
    i4			inspc = FALSE;


    switch (dv1->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
	inchar    = (u_char *) dv1->db_data;
	endinchar = inchar + dv1->db_length;
	break;
	
      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
	inchar    = ((DB_TEXT_STRING *) dv1->db_data)->db_t_text;
	endinchar = inchar + ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    switch (rdv->db_datatype)
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	varlen_result = FALSE;
	break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
	varlen_result = TRUE;
	break;

      default:
	return (adu_error(adf_scb, E_AD5001_BAD_STRING_TYPE, 0));
    }

    /* trim whitespace off front of string */
    while (inchar < endinchar && (CMwhite(inchar) || !*inchar))
	CMnext(inchar);

    /* Allocate bigger buffer if need be */
    if (endinchar - inchar >= sizeof(localbuf))
	buf = (u_char *)MEreqmem(0, endinchar - inchar, FALSE, NULL);

    /*
    ** Collapse any run of whitespace in the body to a space and trim trailing
    */
    outchar = buf;

    while (inchar < endinchar)
    {
        if (CMwhite(inchar) || !*inchar)
	{
	    inspc = TRUE;
	    CMnext(inchar);
	}
	else
	{
	    if (inspc)
	    {
		*outchar++ = ' ';
		inspc = FALSE;
	    }
	    CMcpyinc(inchar, outchar);
	}
    }
    outlength = outchar - buf;
    /*
    ** NOTE: It would be feasible to perform the squeeze directly to
    ** the destination buffer and hence avoid the buffer and extra copy.
    */
    localstatus = adu_movestring(adf_scb, buf, outlength, dv1->db_datatype, rdv);

    /* If bigger buffer borrowed, give it back */
    if (buf != &localbuf[0])
	MEfree((PTR)buf);

    return (localstatus);
}


/*
** Name: adu_trim() - Trim blanks off of the end of a string.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the input
**					string from which trailing blanks
**					are to be stripped.
**	    .db_length			It's length.
**	    .db_data			Ptr to data.
**      rdv                             DB_DATA_VALUE for result.
**	    .db_length			It's length.
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
**	rdv				DB_DATA_VALUE describing the trimmed
**					result.
**	    .db_data			Ptr to data area holding trimmed string.
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
**	    E_AD5001_BAD_STRING_TYPE	The input DB_DATA_VALUE was not a
**					string type.
**
**  History:
**      06/16/83 (lichtman) -- written
**	19-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	25-feb-88 (thurston)
**	    Modified to accept any combo of string types.
**	29-jun-88 (jrb)
**	    Re-wrote for Kanji support (using ADF_TRIMBLANKS_MACRO).
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_trim(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    char		*instart;
    i4			len;
    DB_STATUS		db_stat;

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &len, &instart)) != E_DB_OK)
	return (db_stat);

    /* set len to length of string disregarding trailing blanks */
    ADF_TRIMBLANKS_MACRO(instart, len);

    return (adu_movestring(adf_scb, (u_char *) instart, len, dv1->db_datatype, rdv));
}


/*
** Name: adu_atrim() - ANSI trim function to trim arbitrary chars from front
**	and/or back of string. Note that this differs from the Ingres trim 
**	function that simply trims trailing blanks from a string. The ANSI
**	trim function also uses different syntax.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the input
**					string from which trailing blanks
**					are to be stripped.
**	    .db_length			It's length.
**	    .db_data			Ptr to data.
**      dv2                             DB_DATA_VALUE describing the trim 
**					parameters. It is a char[3] field, 
**					the 1st byte indicates whether trim is
**					from leading, trailing or both ends
**					of string. 2nd (and possibly 3rd) is
**					the char to trim.
**	    .db_length			It's length.
**	    .db_data			Ptr to data.
**      rdv                             DB_DATA_VALUE for result.
**	    .db_length			It's length.
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
**	rdv				DB_DATA_VALUE describing the trimmed
**					result.
**	    .db_data			Ptr to data area holding trimmed string.
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
**	    E_AD5001_BAD_STRING_TYPE	The input DB_DATA_VALUE was not a
**					string type.
**
**  History:
**      12-nov-01 (inkdo01)
**	    Cloned from adu_trim.
**	16-dec-03 (inkdo01)
**	    Propagated to main.
**      10-Mar-2008 (coomi01) : b119995
**          ANSI TRIM should only allow 1 character as TRIM-CHARACTER
**          Throw an error whenever this is not the case.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
**	15-nov-2008 (dougi) BUG 121229
**	    After 7 years, I note that TRAILING was treated as BOTH.
*/

DB_STATUS
adu_atrim(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    char		*instart;
    i4			i, len;
    DB_STATUS		db_stat;
    i1			trimspec;
    char		trimchar[2];

    /* Get length and start address of string to trim. */
    if ((db_stat = adu_lenaddr(adf_scb, dv1, &len, &instart)) != E_DB_OK)
	return (db_stat);

    /* Save away trim code and character(s). */
    trimspec = *((i1 *)dv2->db_data);
    trimchar[0] = *((char *)dv2->db_data + 1);
    trimchar[1] = *((char *)dv2->db_data + 2);

    /*
    ** trimchar[1] was/is always ignored in the code below.
    ** Rather than ignoring non-null values quietly, 
    ** we shall now throw an error
    */
    if ( trimchar[1] != EOS )
    {
	return adu_error(adf_scb, E_AD2051_BAD_ANSITRIM_LEN, 0);
    }

    /* Reset length, if trimspec is trailing or both. */
    if (trimspec == ADF_TRIM_TRAILING || 
		trimspec == ADF_TRIM_BOTH)
    {
	while ((len)-- && *(instart + len) == trimchar[0]);
	len++;			/* len leaves loop 1 too small */
    }

    /* Reset instart and len, if trimspec is leading or both. */
    if (trimspec == ADF_TRIM_LEADING || 
		trimspec == ADF_TRIM_BOTH)
    {
	i = 0;
	if (len)	/* Nothing to do if 0 length string */
	 while (*(instart + i) == trimchar[0] && (i++) < len);

	instart = (char *)(instart + i);
	len -= i;
    }

    /* Move result string (using modified instart and len values). */
    return (adu_movestring(adf_scb, (u_char *) instart, len, dv1->db_datatype, rdv));
}


/*
** Name: adu_ltrim() - Trim function to trim blanks from front of string.
**	This is just a stub that adds a trimspec parm and calls adu_atrim().
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the input
**					string from which leading blanks
**					are to be stripped.
**	    .db_length			It's length.
**	    .db_data			Ptr to data.
**      rdv                             DB_DATA_VALUE for result.
**	    .db_length			It's length.
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
**	rdv				DB_DATA_VALUE describing the trimmed
**					result.
**	    .db_data			Ptr to data area holding trimmed string.
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
**	    E_AD5001_BAD_STRING_TYPE	The input DB_DATA_VALUE was not a
**					string type.
**
**  History:
**      14-apr-2007 (dougi)
**	    Written.
*/

DB_STATUS
adu_ltrim(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)

{
    DB_DATA_VALUE	dv2;
    char		trim[3];


    /* Simply set up trim parameter and call adu_atrim(). */
    dv2.db_datatype = DB_CHA_TYPE;
    dv2.db_length = 3;
    dv2.db_prec = 0;
    dv2.db_collID = -1;
    dv2.db_data = (PTR)&trim;
    trim[0] = ADF_TRIM_LEADING;
    trim[1] = ' ';
    trim[2] = 0;

    return(adu_atrim(adf_scb, dv1, &dv2, rdv));
}


/*
** Name: adu_chr() - Chr() function returns single char from integer input.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the input
**					integer value.
**	    .db_length			It's length.
**	    .db_data			Ptr to data.
**      rdv                             DB_DATA_VALUE for result.
**	    .db_length			It's length.
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
**	rdv				DB_DATA_VALUE describing the trimmed
**					result.
**	    .db_data			Ptr to data area holding trimmed string.
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
**	    E_AD5001_BAD_STRING_TYPE	The input DB_DATA_VALUE was not a
**					string type.
**
**  History:
**      14-apr-2007 (dougi)
**	    Written.
*/

DB_STATUS
adu_chr(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)

{
    i8		intval;
    PTR		resval = rdv->db_data;


    /* Pretty trivial - load integer, mod it with 256 (a la Oracle)
    ** and assign to result byte. */

    switch (dv1->db_length) {
      case 1:
	intval = *((i1 *)dv1->db_data);
	break;
      case 2:
	intval = *((i2 *)dv1->db_data);
	break;
      case 4:
	intval = *((i4 *)dv1->db_data);
	break;
      case 8:
	intval = *((i8 *)dv1->db_data);
	break;
      default:
	intval = 0;
	break;
    }

    intval = intval % 256;
    resval[0] = intval;
    return(E_DB_OK);
}
