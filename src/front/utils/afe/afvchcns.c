/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cm.h>
#include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fe.h>
#include	<afe.h>
# include       <er.h>

/**
** Name:	afvchconst.c -	Translate Strings With Respect to
**					SQL Pattern-Match Characters.
** Description:
**	Contains routines used to translate strings containing SQL pattern-
**	matching characters.  Defines:
**
** 	afe_vchconst()		convert string w.r.t. pattern-match into DBV.
**	afe_sqltrans()		translate string w.r.t. to SQL pattern-match.
**
** History:
**	Revision 6.0  87/04/01  daver
**	Initial revision.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	afe_vchconst() -	Convert String w.r.t SQL Pattern-Match
**						Into VARCHAR Data Value.
** Description:
**	This routine converts a string possibly containing SQL pattern-
**	matching characters into a VARCHAR data value.  It uses 'afe_sqltrans()'
**	to translate the string, which it places into the VARCHAR data value.
**
** Inputs:
**	str	{char *}  The string to be converted, not including any string
**			  delimiters (single-quotes, etc.)
**	len	{nat}  The number of bytes in the string.
**	value	{DB_DATA_VALUE *}  The result data value.
**		.db_datatype	{DB_DT_ID}  Must be DB_VCH_TYPE.
**		.db_length	{nat}  The number of bytes value can hold.
**		.db_data	{PTR}  Must point to a DB_TEXT_STRING that can
**				hold db_length bytes.
**
**	match	{bool}  Boolean indicating whether to treat pattern matching
**			characters as 'special' ones. Should be "TRUE" if 
**			afe_vchconst is called within a where clause.
**
** Outputs:
**	value	{DB_DATA_VALUE *}  Will contain the converted value.
**		.db_data	{PTR}  The DB_TEXT_STRING will contain the
**				converted value.
**
** Returns:
**	OK			If sucessful.
**
**	E_AF600B_BAD_LENGTH	If the length passed in, len, is negative.
**
**	E_AF601E_NOT_VCHR_TYPE	If the DB_DATA_VALUE passed in, 
**				value, isn't of type DB_VCH_TYPE.
**
**	E_AF600E_NO_DBDV_ROOM	The DB_DATA_VALUE provided doesn't 
**				have enough room to hold the converted 
**				VARCHAR constant.
**
** History:
**	01-apr-87	(daver)
**		Stolen from "aftxtconst.c".
**	06/87 (jhw) -- Modified to use 'afe_sqltrans()'.
*/
DB_STATUS
afe_vchconst (cb, str, len, value, match)
ADF_CB		*cb;
char		*str;
i4		len;
DB_DATA_VALUE	*value;
bool		match;
{
    i4		valcount;
    DB_STATUS	rval;

    if (len < 0)
	return afe_error(cb, E_AF600B_BAD_LENGTH, 0);

    if (abs(value->db_datatype) != DB_VCH_TYPE)
	return afe_error(cb, E_AF601E_NOT_VCHR_TYPE, 0);

    /* Size of VARCHAR buffer string */
    valcount = value->db_length - DB_CNTSIZE;

    rval = afe_sqltrans(cb, match, str, len,
		((DB_TEXT_STRING *)(value->db_data))->db_t_text, &valcount
    );

    /* Set length of VARCHAR buffer string */
    if (rval == OK)
	((DB_TEXT_STRING *)(value->db_data))->db_t_count = valcount;

    return rval;
} /* end afe_vchconst */

/*{
** Name:    afe_sqltrans() -	Translate String w.r.t SQL Pattern-Matching.
**
** Description:
**	This routine translates a string possibly containing SQL pattern-
**	matching characters it into an output buffer.  It replaces SQL
**	pattern-matching characters with their internal ADF representations.
**
**	IMPORTANT NOTE: See History below.
**
**	SQL pattern-match characters:
**
**		? _
**
** Inputs:
**	match	{bool}  Whether to translate SQL pattern-matching characters.
**	str	{char *}  The string to be translated, not including any string
**			  delimiters (single-quotes, etc.)
**	len	{nat}  The number of characters in the string to be translated.
**	buf	{char []}  The address of the output buffer.
**	buflen	{nat *}  The size of the output buffer.
**
** Outputs:
**	buf	{char []}  The translated string.
**	buflen	{nat *}  The size of the translated string.
**
** Returns:
**	OK			If sucessful.
**
**	E_AF600E_NO_DBDV_ROOM	The output buffer doesn't have enough room to
**				contain the translated string. 
**
** History:
**	06/87 (jhw) -- Modified from original 'afe_vchconst()'.
**	8-sep-1987 (arthur)
**		Removed all special-casing of SQL pattern-match characters,
**		since ADF uses the SQL (not the old QUEL) wildcards.
**		This routine now merely copies the string passed-in.
**	29-Jan-90 (GordonW)
**		Rewrote usage of 'CMcopy()' into an if/else rather than
**		in a ternary expression so that it can be a cover for
**		MECOPY_VAR_MACRO in the compatability library.
*/
DB_STATUS
afe_sqltrans (cb, match, str, len, buf, buflen)
ADF_CB		*cb;
bool		match;
register char	*str;
register i4	len;
char		buf[];
i4		*buflen;
{
    register char	*bp = buf;
    register i4	blen = *buflen;

    if (len > 0 && !match)
    { /* Copy only */
	/* Optimization:  Don't copy if source == destination */
	if ( str != buf )
		*buflen = CMcopy(str, min(len, blen), buf);
	else
		*buflen = min(len, blen);

	return OK;
    }
    else while (len > 0)
    {
	/* check length remaining in destination */
	if (bp - buf + CMbytecnt(str) > blen)
	    return afe_error(cb, E_AF600E_NO_DBDV_ROOM, 0);

	/* No longer special-case SQL pattern-matching characters here. */
	CMcpychar(str, bp);
	CMnext(bp);
	CMbytedec(len, str);
        CMnext(str);
    } /* end while */

    *buflen = bp - buf;

    return OK;
} /* end afe_sqltrans() */
