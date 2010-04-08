/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<oslconf.h>
#include	<oserr.h>

/**
** Name:    osstrsem.c -	OSL Parser String Semantics Module.
**
** Description:
**	Contains routines that translate an OSL parser string into TEXT or
**	VARCHAR strings under the semantics defined by the DBMS interface.
**	Defines:
**
**	osstring()	translate string w.r.t. DML string semantics.
**	oshexvchar()	translate string w.r.t. hexadecimal VARCHAR semantics.
**
** History:
**	Revision 6.0  87/06  wong.
**	87/02/03 (jhw)  Added 'oshexvchar()'.
**	86/12/29 (jhw)  Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static VOID	single_quote();

/*{
** Name:    osstring() -	Translate String with respect to
**					DML String Semantics.
** Description:
**	Applies the semantic rules defined for the respective DMLs, QUEL or SQL.
**	For QUEL, strings are translated with respect to TEXT strings semantics.
**	For SQL, strings are translated with respect to SQL single-quote string
**	semantics.
**
** Input:
**	match	{bool}  Whether to translate pattern-match characters.
**					(QUEL only.)
**	str	{char *}  The OSL parser string.
**	dml	{DB_LANG}  The respective DML semantics to apply. 
**
** Returns:
**	{char *}	The translated string.
**
** History:
**	06/87 (jhw) -- Written to use 'ostxtconst()' and 'single_quote()'.
*/
char *
osstring ( match, str, dml )
bool	match;
char	*str;
DB_LANG	dml;
{
	char	buf[OSBUFSIZE];

	char	*iiIG_string();

	if (dml == DB_QUEL)
		ostxtconst( match, str, buf, sizeof(buf)-1 );
	else
		single_quote( str, buf );

	return iiIG_string(buf);
}

/*{
** Name:    single_quote() -	Translate String with respect to
**					SQL Single-Quote String Semantics.
** Description:
**	Applies the semantic rules defined for SQL single-quote strings to
**	an OSL parser string, translating doubled single-quote characters
**	to a single single-quote character.  This routine assumes the only
**	single-quotes in the input string are doubled single-quotes, which
**	should be the case for strings returned by the OSL parser scanner.
**
** Input:
**	str	{char *}  The OSL parser string.
**
** Output:
**	buf	{char []}  Buffer to hold translated string.
**
** History:
**	87/06/29 (wong)  Initial revision.
*/
static VOID
single_quote (str, buf)
register char	*str;
char		buf[];
{
    register char	*bp = buf;

    while ((*bp++ = *str) != EOS)
	if (*str++ == '\'')
	    ++str;
}

/*{
** Name:    oshexvchar() -	Translate String with respect to
**					Hexadecimal VARCHAR String Semantics.
** Description:
**	Applies the semantic rules defined for hexadecimal VARCHAR strings by
**	the DBMS to an OSL parser string that consist of hexadecimal digits.
**	Each pair of hexadecimal digits is translated to a single character.
**
**	(Note that the lexical scanner should report if the string is not a
**	legal hexadecimal VARCHAR string.  That is, contains characters that
**	are not hexadecimal digits, or does not contain an even number of
**	digits.)
**
** Input:
**	str	{char *}  The OSL parser string containing the hexadecimal
**			  digits.
**
** Returns:
**	{char *}	The translated string.
**
** History:
**	87/02/03 (wong)  Initial revision.
*/

char *
oshexvchar (str)
register char	*str;
{
    char	    buf[OSBUFSIZE];
    register char   *bp = buf;

    char    *iiIG_vstr();

    if (*str != EOS)
    {
	register char	*dp = str;
	register i4	n = 0;

	do
	{
	    if (CMdigit(dp))
		n = n * 16 + *dp - '0';
	    else
	    { /* hexadecimal */
		char	c;

		CMtoupper(dp, &c);
		n = n * 16 + c - 'A' + 10;
	    }
	    if ((dp - str) % 2 != 0)	/* every other one */
	    {
		*bp++ = (char)n;
		n = 0;
	    }
	} while (*++dp != EOS);
    }

    return iiIG_vstr(buf, bp - buf);
}
