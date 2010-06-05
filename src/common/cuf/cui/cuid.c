/*
** Copyright (c) 1992, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<usererror.h>
# include	<cui.h>
# include	<st.h>

/*
** Name:	cuid.c		- Routines for manipulating identifiers
**
** Description:
**	Contains routines for manipulating identifiers, including:
**
**		cui_idxlate	- Normalize and translate identifier
**		cui_idunorm	- Unnormalize identifier
**		cui_idlen	- Find length of identifier
**		cui_f_idxlate	- fast version of cui_idxlate,
**				  but only handles regular identifiers
**
**  History:
**	26-oct-1992 (ralph)
**	    Written
**	13-jan-1993 (ralph)
**	    Added psl_idunorm()
**	08-feb-1993 (ralph)
**	    Moved to cuf.
**	25-mar-93 (rickh)
**	    Prototyped the functions in this file.
**	25-mar-1993 (rblumer)
**	    added CUI_ID_NOLIMIT to allow unlimited length strings;
**	    used for character defaults (which can be up to 1500 bytes).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (peterk)
**	    remove <dbdbms.h>
**	20-sep-93 (jnash)
**	    Add cud_idlen().
**	22-sep-1993 (rblumer)
**	    use DB_MAX_DELIMID for max delim id, instead of (DB_MAXNAME*2 +2).
**      18-feb-94 (rblumer)
**          Added cui_f_idxlate function (faster than cui_idxlate for
**          translating regular identifiers in-place).  Currently used for
**          procedure-parameter translating in SCS, to improve performance in
**          TPC benchmarks.
**          Also changed cui_idxlate to look at reg_id cases first, since that
**          will be the most common usage.
**	18-apr-1994 (rblumer)
**	    In cui_idxlate, allow any character in a delimited id (instead of
**	    just allowing the additional characters defined as 'SQL Special
**	    Characters' in SQL92).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** Name:	cui_idunorm	- Unnormalize identifier
**
**  Description:
**
**	Wrap delimiters/quotes around identifier and escape embedded
**	delimiters/quotes.
**
**  Inputs:
**	src		- source string to be translated
**	src_len_parm	- ptr to length of src;
**			  if zero length value, input string null terminated;
**			  default is zero if src_len_parm is NULL.
**	dst_len_parm	- ptr to length of dst
**			  default is DB_MAX_DELIMID if dst_len_parm is NULL.
**	mode		- bit mask indicating the following:
**	  CUI_ID_DLM	- Unnormalize the identifier as delimited
**			  This is the default with neither CUI_ID_DLM nor
**			  CUI_ID_QUO are specified.
**	  CUI_ID_QUO	- Unnormalize the identifier as quoted
**	  CUI_ID_STRIP	- Strip trailing blanks (delimited id only)
**	  CUI_ID_NOLIMIT- Allow unlimited length (used for char defaults)
**
**  Outputs:
**	*src_len_parm	- length of input buffer used,
**			  set if src_len_parm not NULL
**	dst		- buffer containing unnormalized identifier
**	*dst_len_parm	- length of unnormalized identifier
**			  set if dst_len_parm not NULL
**
**	Returns:
**	DB_STATUS	- Status of translation:
**	    E_DB_OK		- identifier unnormalized appropriately
**	    E_DB_WARN		- identifier unnormalized with warnings
**	    E_DB_ERROR		- identifier not unnormalized
**
**	err_blk
**	.err_code  	- Error message:
**	    E_CU_ID_PARAM_ERROR	- Parameter error
**	    E_US1A24_ID_TOO_SHORT- Identifier too short (e.g. "")
**	    E_US1A25_ID_TOO_LONG- Identifier too long (> DB_MAXNAME)
**
**  History:
**	13-jan-1993 (ralph)
**	    Written
**	09-feb-1993 (ralph)
**	    Moved from pslid
**	    Changed constant names
**	    Changed mode to u_i4
**	    Changed return values
**	    Trim off trailing blanks if delimited id
**	    Changed to return length of source string parsed
**	25-mar-93 (rickh)
**	    Prototyped the functions in this file.
**	25-mar-1993 (rblumer)
**	    added CUI_ID_NOLIMIT to allow unlimited length strings;
**	    used for character defaults (which can be up to 1500 bytes).
*/
DB_STATUS
cui_idunorm(
u_char		*src,
u_i4		*src_len_parm,
u_char		*dst,
u_i4		*dst_len_parm,
u_i4	mode,
DB_ERROR	*err_blk
)
{
    register u_char	*src_pos	= src;
    register u_char	*src_end;
    register u_char	*dst_pos	= dst;
    register u_char	*dst_end;
    u_i4		*src_len;
    u_i4		*dst_len;
    u_i4		src_len_default = 0;
    u_i4		dst_len_default = DB_MAX_DELIMID;
    bool		dlm_id;
    bool		quo_id;
    bool		src_noend;
    bool		trailing_blanks = FALSE;
    register u_char	*trail;

    err_blk->err_code = 0;
    err_blk->err_data = 0;

    src_len =  (src_len_parm != NULL ?
		src_len_parm : &src_len_default);

    dst_len =  (dst_len_parm != NULL ?
		dst_len_parm : &dst_len_default);

    src_end =  src+*src_len;
    dst_end =  dst+*dst_len;
    src_noend = (src == src_end);

    /*
    ** Check for invalid parameters
    */

    if ((*dst_len < 1) ||
	((*src_len > DB_MAXNAME) && (~mode & CUI_ID_NOLIMIT)) ||
	((mode & (CUI_ID_DLM | CUI_ID_QUO)) == (CUI_ID_DLM | CUI_ID_QUO)))
    {
	/* ERROR: Invalid parameter */
	err_blk->err_code = E_CU_ID_PARAM_ERROR;
	return(E_DB_ERROR);
    }

    *src_len = 0;

    /*
    ** Determine the target mode
    */

    quo_id = (bool)(mode & CUI_ID_QUO);
    dlm_id = !quo_id;

    /*
    ** Set initial delimiter/quote
    */

    if (quo_id)
	*dst_pos = '\'';
    else
	*dst_pos = '"';
    CMnext(dst_pos);

    /*
    ** Scan the body of the identifier
    */

    while ((src_noend && (*src_pos != EOS)) || (src_pos < src_end))
    {
	/*
	** Check for excessive source length 
	*/
	if ((++*src_len > DB_MAXNAME) && (~mode & CUI_ID_NOLIMIT))
	{
	    /*
	    ** ERROR: Source string too long
	    */
	    err_blk->err_code = E_US1A25_ID_TOO_LONG;
	    return(E_DB_ERROR);
	}

	/* 
	** Check for delimiter
	*/
	if ((dlm_id && (*src_pos == '"')) ||
	    (quo_id && (*src_pos == '\'')))
	{
	    if ((dst_end - dst_pos) < CMbytecnt(src_pos))
	    {
		/*
		** ERROR: Out of space in output buffer
		*/
	        err_blk->err_code = E_US1A25_ID_TOO_LONG;
		return(E_DB_ERROR);
	    }
	    CMcpychar(src_pos, dst_pos);
	    CMnext(dst_pos);
	}

	/*
	** Check for excessive length
	*/
	if ((dst_end - dst_pos) < CMbytecnt(src_pos))
	{
	    /*
	    ** ERROR: Out of space in output buffer
	    */
	    err_blk->err_code = E_US1A25_ID_TOO_LONG;
	    return(E_DB_ERROR);
	}

	/*
	** Track location of trailing blanks
	*/
	if (dlm_id)
	{
	    if (*src_pos == ' ')
	    {
		if (!trailing_blanks)
		{
		    trailing_blanks = TRUE;
		    trail = dst_pos;
		}
	    }
	    else 
		trailing_blanks = FALSE;
	}

	/*
	** Copy character to destination
	*/
	CMcpychar(src_pos, dst_pos);
	CMnext(src_pos);
	CMnext(dst_pos);
    }

    /*
    ** Set final delimiter/quote
    */

    if (trailing_blanks &&
	(mode & CUI_ID_STRIP))
	dst_pos = trail;

    if ((dst_end - dst_pos) < CMbytecnt(src_pos))
    {
	/*
	** ERROR: Out of space in output buffer
	*/
	err_blk->err_code = E_US1A25_ID_TOO_LONG;
	return(E_DB_ERROR);
    }

    if (quo_id)
	*dst_pos = '\'';
    else
	*dst_pos = '"';
    CMnext(dst_pos);

    *dst_len = dst_pos - dst;

    /*
    ** Ensure that something was placed in the destination buffer
    */

    if (*dst_len <= 2)
    {
	/*
	** ERROR: Identifier is too short
	*/
	err_blk->err_code = E_US1A24_ID_TOO_SHORT;
	return(E_DB_ERROR);
    }

    /*
    ** Return to caller
    */

    return(E_DB_OK);

}

/*
** Name:	cui_idxlate	- Normalize and translate identifier
**
**  Description:
**
**	Normalize and case translate identifier according to specified
**	standards.
**
**
**  Inputs:
**	src		- source string to be translated
**	src_len_parm	- ptr to length of src;
**			  if zero length value, input string null terminated;
**			  default is zero if src_len_parm is NULL.
**	dst_len_parm	- ptr to length of dst
**			  default is DB_MAXNAME if dst_len_parm is NULL.
**	mode		- bit mask indicating the following:
**	  CUI_ID_DLM	- Treat the identifier as delimited
**	  CUI_ID_DLM_M	- Don't translate case of delimited ids
**	  CUI_ID_DLM_U	- Translate delimited ids to upper case
**	  CUI_ID_DLM_L	- Translate delimited ids to lower case
**			  Default is to tanslate delimited ids to lower case
**	  CUI_ID_QUO	- Treat the identifier as quoted
**	  CUI_ID_QUO_M  - Don't translate case of quoted ids
**	  CUI_ID_QUO_U	- Translate quoted ids to upper case
**	  CUI_ID_QUO_L	- Translate quoted ids to lower case
**			  Default is not to tanslate quoted ids
**	  CUI_ID_REG	- Treat the identifier as regular
**	  CUI_ID_REG_M	- Don't translate case of regular ids
**	  CUI_ID_REG_U	- Translate regular ids to upper case
**	  CUI_ID_REG_L	- Translate regular ids to lower case
**			  Default is to tanslate regular ids to lower case
**	  CUI_ID_ANSI	- Check for special ANSI conditions
**	  CUI_ID_NORM	- Treat the identifier as if normalized
**	  CUI_ID_STRIP	- Ignore trailing white space in source identifier
**	  CUI_ID_NOLIMIT- Allow unlimited length (used for char defaults)
**
**  Outputs:
**	*src_len_parm	- length of input buffer used,
**			  set if src_len_parm not NULL
**	dst		- buffer containing translated identifier;
**			  if null, id must be regular, and will be
**			  case translated in place.
**	*dst_len_parm	- length of translated identifier
**			  set if dst_len_parm not NULL
**	*retmode	- Set to recognized identifier type, if not NULL:
**				CUI_ID_DLM - Source identifier was delimited
**				CUI_ID_QUO - Source identifier was quoted
**				CUI_ID_REG - Source identifier was regular
**
**	Returns:
**	DB_STATUS	- Status of translation:
**	    E_DB_OK		- identifier unnormalized appropriately
**	    E_DB_WARN		- identifier unnormalized with warnings
**	    E_DB_ERROR		- identifier not unnormalized
**
**	err_blk
**	.err_code  	- Error message:
**	    E_US1A11_ID_ANSI_TOO_LONG -
**				  Output string too long for ANSI identifier
**				  NOTE: This is a WARNING only.  It does not
**					stop the scan, and is returned only
**					if CUI_ID_ANSI is specified in mode.
**	    E_US1A12_ID_ANSI_END -
**				  Invalid last character for an ANSI identifier
**				  NOTE: This is a WARNING only.  It does not
**					stop the scan, and is returned only
**					if CUI_ID_ANSI is specified in mode.
**	    E_US1A13_ID_ANSI_BODY -
**				  Invalid character in body of ANSI identifier
**				  NOTE: This is a WARNING only.  It does not
**					stop the scan, and is returned only
**					if CUI_ID_ANSI is specified in mode.
**	    E_US1A14_ID_ANSI_START -
**				  Invalid first character for an ANSI identifier
**				  NOTE: This is a WARNING only.  It does not
**					stop the scan, and is returned only
**					if CUI_ID_ANSI is specified in mode.
**	    E_CU_ID_PARAM_ERROR	- Parameter error
**	    E_US1A20_ID_START	- Invalid first character for relular id
**	    E_US1A21_ID_DLM_BODY- Invalid character in body of delimited id
**	    E_US1A22_ID_BODY	- Invalid character in body of regular id
**	    E_US1A23_ID_DLM_END	- Missing terminating delimiter or quote
**	    E_US1A24_ID_TOO_SHORT- Identifier too short (e.g. "")
**	    E_US1A25_ID_TOO_LONG- Identifier too long (> DB_MAXNAME)
**
**  History:
**	26-oct-1992 (ralph)
**	    Written
**	03-dec-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	10-feb-1993 (ralph)
**	    Moved from pslid
**	    Changed constant names
**	    Changed mode to u_i4
**	    Changed return values
**	    Added retmode
**	    Allow case translation in place
**	25-mar-93 (rickh)
**	   Changed return status to DB_STATUS.
**	25-mar-1993 (rblumer)
**	    added CUI_ID_NOLIMIT to allow unlimited length strings;
**	    used for character defaults (which can be up to 1500 bytes).
**	05-may-1993 (ralph)
**	    Minor bug fixes:
**		Check for length > DB_MAXNAME when CUI_ID_ANSI.
**	18-apr-1994 (rblumer)
**	    Allow any character in a delimited id (instead of just allowing
**	    the additional characters defined as 'SQL Special Characters' in
**	    SQL92).
**	23-mar-1995 (peeje01)
**          Cross integration double byte changes from 6500db_su4_us42:
**	    10-dec-1993 (mikehan)
**		    Use CMbyteinc to increment src_len and dst_len.
**		    ++*src_len and ++*dst_len would not do it if the
**		    query statements contain double-byte characters.
**		    Syntax errors occur otherwise in STAR database.
*/
DB_STATUS
cui_idxlate(
u_char		*src,
u_i4		*src_len_parm,
u_char		*dst,
u_i4		*dst_len_parm,
u_i4	mode,
u_i4	*retmode,
DB_ERROR        *err_blk
)
{
    register u_char	*src_pos	= src;
    register u_char	*src_end;
    register u_char	*dst_pos	= dst;
    register u_char	*dst_end;
    u_i4		*src_len;
    u_i4		*dst_len;
    u_i4		src_len_default = 0;
    u_i4		dst_len_default = DB_MAXNAME;
    i4		retcode		= 0;
    bool		reg_id		= FALSE;
    bool		dlm_id		= FALSE;
    bool		quo_id		= FALSE;
    bool		normal		= FALSE;
    bool		upcase		= FALSE;
    bool		lowcase		= FALSE;
    bool		ansi_id		= FALSE;
    bool		dlm_ok		= FALSE;
    bool		white_found	= FALSE;
    bool		inplace		= FALSE;
    bool		src_noend;

    err_blk->err_code = 0;
    err_blk->err_data = 0;

    src_len =  (src_len_parm != NULL ?
		src_len_parm : &src_len_default);

    dst_len =  (dst_len_parm != NULL ?
		dst_len_parm : &dst_len_default);

    if (dst == (u_char *)NULL)
    {
	inplace = TRUE;
	dst = src;
	dst_pos = dst;
    }

    src_end =  src+*src_len;
    dst_end =  dst+*dst_len;
    src_noend = (src == src_end);

    /*
    ** Check for invalid parameters
    */

    if (*dst_len < 1)
    {
	/* ERROR: Invalid parameter */
	err_blk->err_code = E_CU_ID_PARAM_ERROR;
	return(E_DB_ERROR);
    }

    *src_len = 0;
    *dst_len = 0;

    /*
    ** Determine the type of identifier and translation mode
    */

    if (mode & CUI_ID_NORM)
    {
	/*
	** We have a pre-normalized identifier
	*/
	normal = TRUE;
	dlm_ok = TRUE;
	if (mode & CUI_ID_DLM)
	{
	    /*
	    ** We have a delimited identifier
	    */
	    dlm_id = TRUE;
	    if (mode & CUI_ID_DLM_M)
		/*
		** Suppress case translation requested for delimited identifiers
		*/
		;
	    else if (mode & CUI_ID_DLM_U)
		/*
		** Upper casing requested for delimited identifiers
		*/
		upcase = TRUE;
	    else
		/*
		** Default is translate to lower case for delimited identifiers
		*/
		lowcase = TRUE;
	}
	else if (mode & CUI_ID_QUO)
	{
	    /*
	    ** We have a single-quoted identifier
	    */
	    quo_id = TRUE;

	    if (mode & CUI_ID_QUO_U)
		/*
		** Upper casing requested for quoted identifiers
		*/
		upcase = TRUE;
	    else if (mode & CUI_ID_QUO_L)
		/*
		** Lower casing requested for quoted identifiers
		*/
		lowcase = TRUE;
	    else
		/*
		** Default is no case translation for quoted identifiers
		*/
		;
	}
	else
	{
	    /*
	    ** We have a regular identifier
	    */
	    reg_id = TRUE;
	    dlm_ok = TRUE;
	    if (mode & CUI_ID_REG_M)
		/*
		** Suppress case translation requested for regular identifiers
		*/
		;
	    else if (mode & CUI_ID_REG_U)
		/*
		** Upper casing requested for regular identifiers
		*/
		upcase = TRUE;
	    else
		/*
		** Default is translate to lower case for regular identifiers
		*/
		lowcase = TRUE;
	}
    }
    else
    {
	/*
	** We have an unnormalized identifier
	*/
	if (*src_pos == '"')
	{
	    /*
	    ** We have a delimited identifier
	    */
	    dlm_id = TRUE;
	    CMnext(src_pos);
	    ++*src_len;
	    if (mode & CUI_ID_DLM_M)
		/*
		** Suppress case translation requested for delimited identifiers
		*/
		;
	    else if (mode & CUI_ID_DLM_U)
		/*
		** Upper casing requested for delimited identifiers
		*/
		upcase = TRUE;
	    else
		/*
		** Default is translate to lower case for delimited identifiers
		*/
		lowcase = TRUE;
	}
	else if (*src_pos == '\'')
	{
	    /*
	    ** We have a single-quoted identifier
	    */
	    quo_id = TRUE;
	    CMnext(src_pos);
	    ++*src_len;
	    if (mode & CUI_ID_QUO_U)
		/*
		** Upper casing requested for quoted identifiers
		*/
		upcase = TRUE;
	    else if (mode & CUI_ID_QUO_L)
		/*
		** Lower casing requested for quoted identifiers
		*/
		lowcase = TRUE;
	    else
		/*
		** Default is no case translation for quoted identifiers
		*/
		;
	}
	else
	{
	    /*
	    ** We have a regular identifier
	    */
	    reg_id = TRUE;
	    dlm_ok = TRUE;
	    if (mode & CUI_ID_REG_M)
		/*
		** Suppress case translation requested for regular identifiers
		*/
		;
	    else if (mode & CUI_ID_REG_U)
		/*
		** Upper casing requested for regular identifiers
		*/
		upcase = TRUE;
	    else
		/*
		** Default is translate to lower case for regular identifiers
		*/
		lowcase = TRUE;
	}
    }

    /*
    ** Check for invalid parameters
    */

    if (inplace && !reg_id)
    {
	/* ERROR: Invalid parameter */
	err_blk->err_code = E_CU_ID_PARAM_ERROR;
	return(E_DB_ERROR);
    }

    /*
    ** Set return mode
    */
    if (retmode != NULL)
    {
	if (reg_id)
	    *retmode = CUI_ID_REG;
	else if (dlm_id)
	    *retmode = CUI_ID_DLM;
	else
	    *retmode = CUI_ID_QUO;
    }
    /*
    ** Determine if we are to perform special ANSI processing,
    ** including:
    **
    **		o Restrict character set
    **		o Restrict starting character
    **		o Restrict ending character
    **		o Restrict length
    */

    if (mode & CUI_ID_ANSI)
    {
	/*
	** Check for ANSI Entry SQL 92 identifier conventions
	*/
	ansi_id = TRUE;
    }

    /*
    ** Validate the first character, if a regular identifier
    */

    if (reg_id)
    {
	/*
	** Check for valid leading character
	*/
	if (!CMnmstart(src_pos))
	{
	    switch (*src_pos)
	    {
		case '$':
	    	    /*@FIX_ME@
	    	    ** We'll allow this one for $ingres
	    	    */
		    break;
		default:
	    	    /*
	    	    ** ERROR: Invalid first character in identifier
	    	    */
		    err_blk->err_code = E_US1A20_ID_START;
		    ++*src_len;
	    	    return(E_DB_ERROR);
	    }
		
	}
	else if (ansi_id)
	{
	    /*
	    ** Check special cases for ANSI identifiers
	    */
	    if (*src_pos == '_')
	    {
		/*
		** WARNING: Invalid first character for ANSI identifier
		*/
		retcode = (retcode > E_US1A14_ID_ANSI_START ?
			   retcode : E_US1A14_ID_ANSI_START);
	    }
	}
    }

    /*
    ** Scan the body of the identifier, performing the following:
    **
    **		o Detect ending delimiter
    **		o Strip off escaped delimiters
    **		o Validate that the character wrt the identifier type
    **		o Copy the character to the destination buffer
    **		o Translate case of the destination character (if required)
    */

    while ((src_noend && (*src_pos != EOS)) || (src_pos < src_end))
    {
	/*
	** Check for delimiter
	*/

	if (dlm_id && !normal && (*src_pos == '"'))
	{
	    CMnext(src_pos);
	    ++*src_len;
	    if ((!src_noend && (src_pos >= src_end)) ||
	        (*src_pos != '"'))
	    {
	        /*
		** Ending delimiter encountered
		*/
	        dlm_ok = TRUE;
	        break;
	    }
	}
	else if (quo_id && !normal && (*src_pos == '\''))
	{
	    CMnext(src_pos);
	    ++*src_len;
	    if ((!src_noend && (src_pos >= src_end)) ||
		(*src_pos != '\''))
	    {
	        /*
		** Ending delimiter encountered
		*/
	        dlm_ok = TRUE;
	        break;
	    }
	}
	
	/*
	** Validate the character wrt the identifier type
	*/

	if (!CMnmchar(src_pos))
	{
	    if (reg_id)
	    {
		/*
		** Invalid character in identifier
		*/
		if (!CMwhite(src_pos) ||
		    !(mode & CUI_ID_STRIP))
		{
		    err_blk->err_code = E_US1A22_ID_BODY;
		    ++*src_len;
		    return(E_DB_ERROR);
		}
		/*
		** We found white space; ignore for now 
		*/
		white_found = TRUE;
	    }
	    /*
	    ** else any characters are valid in a
	    ** delimited or single-quoted identfier
	    */
	}
	else if (white_found && reg_id)
	{
	    /*
	    ** We found embedded white space
	    */
	    err_blk->err_code = E_US1A22_ID_BODY;
	    ++*src_len;
	    return(E_DB_ERROR);
	}
	else if (ansi_id)
	{
	    /*
	    ** Check special cases for ANSI identifiers
	    */
	    switch (*src_pos)
	    {
		case '#':
		case '@':
		case '$':
		    /*
		    ** WARNING: Invalid character in ANSI identifier
		    */
		    retcode = (retcode > E_US1A13_ID_ANSI_BODY ?
			       retcode : E_US1A13_ID_ANSI_BODY);
		    break;
	    }
	}

	/*
	** Copy the character to the destination buffer
	*/

	if ((dst_end - dst_pos) < CMbytecnt(src_pos))
	{
	    /*
	    ** ERROR: Out of space in output buffer
	    */
	    err_blk->err_code = E_US1A25_ID_TOO_LONG;
	    ++*src_len;
	    return(E_DB_ERROR);
	}

	if (!inplace)
	    CMcpychar(src_pos, dst_pos);

	/*
	** Translate case of the destination character, if required
	*/

	if (lowcase)
	    CMtolower(dst_pos, dst_pos);
	else if (upcase)
	    CMtoupper(dst_pos, dst_pos);

	/*
	** Bump to the next character
	*/

	CMbyteinc(*src_len, src_pos);
	CMnext(src_pos);
	CMbyteinc(*dst_len, dst_pos);
	CMnext(dst_pos);

    }    

    /*
    ** Ensure matching delimiter encountered, if required
    */

    if (!dlm_ok)
    {
	/*
	** ERROR: No ending delimiter encountered
	*/
	err_blk->err_code = E_US1A23_ID_DLM_END;
	return(E_DB_ERROR);
    }

    /*
    ** Ensure that something was placed in the destination buffer
    */

    if (*dst_len == 0)
    {
	/*
	** ERROR: Identifier is too short
	*/
	err_blk->err_code = E_US1A24_ID_TOO_SHORT;
	return(E_DB_ERROR);
    }

    /*
    ** Ensure we end with a valid character for ANSI identifiers
    */

    if (reg_id && ansi_id)
    {
	/* Check for valid ending character */
	CMprev(dst_pos, dst);
	switch (*dst_pos)
	{
	    /*@FIX_ME@ Determine all cases! */
	    case '_':
		retcode = (retcode > E_US1A12_ID_ANSI_END ?
			   retcode : E_US1A12_ID_ANSI_END);
	}
	CMnext(dst_pos);
    }

    /*
    ** Ensure the identifier is not too long
    */

    if (ansi_id && *dst_len > 18)
    {
	/*
	** WARNING: Identifier too long for ANSI
	*/
	retcode = (retcode > E_US1A11_ID_ANSI_TOO_LONG ?
		   retcode : E_US1A11_ID_ANSI_TOO_LONG);
    }

    if ((*dst_len > DB_MAXNAME) && (~mode & CUI_ID_NOLIMIT))
    {
	/*
	** ERROR: Identifier too long for INGRES
	*/
	err_blk->err_code = E_US1A25_ID_TOO_LONG;
	return(E_DB_ERROR);
    }

    /*
    ** Return to caller
    */

    err_blk->err_code = retcode;
    if (retcode != 0)
	return(E_DB_WARN);

    return(E_DB_OK);

}  /* end cui_idxlate */

/*{
** Name: cui_f_idxlate		- fast, in-place translation of an identifier.
**
** Description:
**	If the identifier passed in is a regular identifier,
**	just case-translate it according to given translation semantics
**	and return TRUE.
**	
**	If the identifier passed in is NOT a regular identifier
**	(i.e. is a delimited identifier), then return FALSE
**	without doing anything.
**
**	    Written as an optimization of cui_idxlate functionality
**	    for dbproc parameter translation.  Cui_idxlate was taking a
**	    significant amount of time in a profile of EXEC PROC performance.
**	    This function optimizes for the usual case of a regular identifier,
**	    and does an in-place translation as well.
**
** Inputs:
**	ident	    pointer to identifier to be translated
**	length	    length  of identifier to be translated
**	dbxlate	    case-translation semantics for the current database
**
** Outputs:
**	ident	    case-translated identifier (if return-code is TRUE)
**
** Returns:
**	TRUE	-if identifier was translated
**	FALSE	-if identifier wasn't translated; this implies caller must call
**		 normal cui_idxlate routine instead.
**
** Side Effects:
**	    none
**
** Example Usage:
** 	if (cui_f_idxlate(proc_param, ics_dbxlate) == FALSE)
**	{
**	    templen = DB_MAXNAME;
**	    status = cui_idxlate(
**			   (u_char *) proc_param, &param_length,
**			   (u_char *) trans_param, &trans_len,
**			   scb->scb_sscb.sscb_ics.ics_dbxlate,
**			   (u_i4 *)NULL, &cui_err);
**
**	    if (DB_FAILURE_MACRO(status))
**	    {
**	       process error
**	    }
**	}
**	
** History:
**	18-feb-94 (rblumer, mikem)
**	    written as an optimization of cui_idxlate functionality
**	    for dbproc parameter translation.
*/
i4
cui_f_idxlate(
	      char	*ident,
	      i4	length,
	      u_i4 dbxlate)
{
    register i4  i;

    if ((CMcmpcase(ident, "\"") == 0) || (CMcmpcase(ident, "\'") == 0))
	return(FALSE);

    if (dbxlate & CUI_ID_REG_U)
    {
	/* convert to upper case 
	 */
	for (i = length; i > 0; i--)
	{
	    if (CMlower(ident))
	    {
		CMtoupper(ident, ident);
	    }
	    CMnext(ident);
	}
    }
    else
    {
	/* convert to lower case 
	 */
	for (i = length; i > 0; i--)
	{
	    if (CMupper(ident))
	    {
		CMtolower(ident, ident);
	    }
	    CMnext(ident);
	}
    }
    
    return(TRUE);
    
}  /* end cui_f_idxlate */

/*{
** Name: cui_idlen		- Find length of delimited object.
**
** Description:
**
**	Determine the length of a possibly delimited object in a 
**	string (e.g. command line).  No check is made to determine if
**	length is valid.
**
** Inputs:
**      src			pointer to object, either separator or EOS
**      			 terminated
**      separator		separator character (comma, etc.)
**
** Outputs:
**      idlen			Returned length of id
**
**	Returns:
**	DB_STATUS	- Status of translation:
**	    E_DB_OK		- length returned
**	    E_DB_ERROR		- length not returned
**	err_blk
**	.err_code  		- Error message:
**	    E_US1A23_ID_DLM_END - No ending delimiter found
**
** Side Effects:
**	    none
**
** History:
**      20-sep-1993 (jnash)
**          Created in support of ASCII lists of delimited objects, such as 
**	    utility program command line arguments.
*/
DB_STATUS
cui_idlen(
u_char		*src,
u_char		separator,
u_i4		*idlen,
DB_ERROR	*err_blk
)
{
    u_i4	quote_cnt = 0;
    u_char	*ch_ptr = src;

    err_blk->err_code = 0;
    err_blk->err_data = 0;

    /*
    ** Count double quotes.  If separator or EOS found after an even 
    ** number, we have found the end.
    */
    while (*ch_ptr != EOS)
    {
	if (*ch_ptr == '"')
	    quote_cnt++;
	else if ((*ch_ptr == separator) && (quote_cnt % 2 == 0))
	    break;

	CMnext(ch_ptr);
    }

    /*
    ** EOS with odd quote count is an error.
    */
    if ((*ch_ptr == EOS) && (quote_cnt % 2 != 0))
    {
	/*
	** ERROR: No ending delimiter encountered
	*/
  	err_blk->err_code = E_US1A23_ID_DLM_END; 
	return(E_DB_ERROR);
    }

    *idlen = ch_ptr - src;

    return(E_DB_OK);
}


/*{
** Name: cui_chk1_dbname() -	check the syntax of a database name.
**
** Description:
**        This routine checks the syntactical correctness of a database name.
**	A valid database name is not more than 9 characters long, begins
**	with an alphabetic character, and contains only alphanumerics.
**	Underscore is not allowed.  Note, this routine has the side effect
**	of lower-casing the string passed in.
**
** Inputs:
**      dbname                          Ptr to a buffer containing dbname.
**
** Outputs:
**	*dbname				Ptr to buffer containing lower-cased
**					dbname.
**	Returns:
**	    OK				If the name was valid.
**	    FAIL			If the name was invalid.
**
** Side Effects:
**	  Lower cases the database name being checked.
**
** History:
**      08-Sep-86 (ericj)
**          Initial creation.
**      16-Dec-88 (teg)
**          The CL routine CMnmchar changed its behavior and started returning
**	    TRUE for some non alphanumeric characters (@,$,!).  So, I modified
**	    this routine to stop using CMnmchar and to check explicately for
**	    alpha, digit and underscore.
**      24-May-99 (kitch01)
**         Bug 91063. Moved duc_chk1_dbname() from duuchknm.c and renamed to
**         cui_chk1_dbname. This is so that backend programs can access the
**         function to check supplied database names.
*/
STATUS
cui_chk1_dbname(dbname)
char               *dbname;
{
    register char	*p;

    p	= dbname;

    /* check string length */
    if ((STlength(p) > DB_DB_MAXNAME) || (*p == EOS))
	return (FAIL);

    /* lowercase the database name */
    CVlower(p);

    /* Check the first character of the string for alphabetic.  Note,
    ** we don't use CMnmstart() here as this name will map to the
    ** O.S.s file system naming restrictions.
    */
    if (!CMalpha(p))
	return (FAIL);

/*
** if @!$ are valid for table names etc. (regular identifiers),
** then they're valid for DB names, I'm told.
*/
    for (CMnext(p); *p != EOS; CMnext(p))
    {
    	if (!CMnmchar(p))
	    return (FAIL);
    }

    return (OK);
}


/*{
** Name: cui_chk3_locname() -	check location name syntax.
**
** Description:
**        This routine checks the validity of a location name's syntax.
**
** Inputs:
**      cp				Ptr to a buffer containing the
**					location name.
**
** Outputs:
**	none
**	Returns:
**	    OK			If a valid location name.
**	    FAIL			If an invalid location name.
**
** Side Effects:
**	      Lower cases the user's name.
**
** History:
**      08-Sep-86 (ericj)
**          Initial creation.
**      30-Jan-89 (teg)
**          replaced magic number (12) with appropriate constant (DB_MAXNAME)
**	    to fix bug 4581.
**	28-sep-2004 (devjo01)
**	    Renamed from du_chk3_locname, and Moved here from duuchknm.c to
**	    resolve linkage deps. (also it should have been here from the 1st).
[@history_line@]...
[@history_template@]...
*/

cui_chk3_locname(cp)
register char	*cp;
{

    if ((STlength(cp) > DB_LOC_MAXNAME) || (*cp == EOS))
	return(FAIL);

    if(!CMnmstart(cp))
	return(FAIL);

    for (CMnext(cp); *cp != EOS; CMnext(cp))
    {
	if (!CMnmchar(cp))
	    return(FAIL);
    }

    return(OK);
}
