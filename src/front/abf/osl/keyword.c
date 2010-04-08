/*
** Copyright (c) 1984, 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>
# include	<oslkw.h>

/**
** Name:	keyword.c -	OSL Parser Keyword Module.
**
** Description:
**	Contains routines used to search keyword tables for a particular
**	keyword or double keyword.  (This is in a separate file so the
**	implementation can easily be changed.)  Defines:
**
**	iskeyword()	return keyword table entry for keyword.
**	isdblkeyword()	return keyword table entry for double keyword.
**
** History:
**	Revision 5.1  86/11/04  14:44:43  wong
**	Modified to use binary search.  Now supports double keywords.
**	10/11/92 (dkh) - Removed readonly qualification on references
**			 declaration to kwtab and kwtab_size.  Alpha
**			 cross compiler does not like the qualification
**			 for globalrefs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	iskeyword() -	Return Keyword Table Entry for Keyword.
**
** Description:
**	Checks an identifier to see if it is a keyword.  (Does so by searching
**	the keyword table for the identifier.)
**
** Parameters:
**	name	{char *}  The identifier found by the scanner.
**
** Returns:
**	{KW *}  The keyword table entry corresponding to the input identifier,
**		NULL otherwise.
**
** Called by:
**	yylex()
**
** History:
**	10/86 (jhw) -- Now uses binary search.
**	Written (jrc)
*/

KW *
iskeyword (name)
char	*name;
{
	GLOBALREF KW	kwtab[];
	GLOBALREF i4	kwtab_size;

	if ( name != NULL && *name != EOS )
	{
		register KW	*kw;
		register KW	*start;
		register KW	*end;
		char		buf[32]; /* sizeof > max. keyword length */

		_VOID_ STlcopy( name, buf, sizeof(buf)-1 );
		CVlower(buf);

		/* Binary search */
		start = kwtab;
		end = start + (kwtab_size - 1);
		do
		{
			register i4	dir;

			kw = start + (end - start) / 2;
			if ( (dir = *buf - *kw->kwname) == 0 &&
					(dir = STcompare(buf, kw->kwname)) == 0 )
				return kw;
			else if (dir < 0)
				end = kw - 1;
			else
				start = kw + 1;
		} while ( start <= end );
	}
	return NULL;
}

/*{
** Name:	isdblkeyword() -	Returns Keyword Table Entry
**						for Double Keyword.
** Description:
**	Checks that the input identifier is in the input keyword table.
**	(The input keyword table is a list for a particular double keyword.)
**
** Input:
**	name	{char *}  The identifier found by the scanner.
**	keytab	{KW *}  The keyword table for a double keyword.
**
** Returns:
**	{KW *}  The keyword table entry corresponding to the input identifier
**		in the double keyword table, NULL otherwise.
**
** Called by:
**	yylex()
**
** History:
**	10/86 (jhw) -- Written (from linear search version of 'iskeyword()'.)
*/
KW *
isdblkeyword (name, keytab)
char	*name;
KW	*keytab;
{
	if ( name != NULL && *name != EOS )
	{
		register KW	*kw;
		char		buf[32]; /* sizeof > max. keyword length */

		_VOID_ STlcopy( name, buf, sizeof(buf)-1 );
		CVlower(buf);

		/* Linear search (NULL terminated) (since table should be small) */
		for ( kw = keytab ; kw->kwname != NULL ; ++kw )
			if ( STequal(buf, kw->kwname) )
				return kw;
	}
	return NULL;
}
