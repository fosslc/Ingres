/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:	feprefix.c -	Front-End Prefix Comparison Routine.
**
** Description:
**	Contains routines that check whether a prefix is part of a string
**	(or strings in an argument list.)  Defines:
**
**	FEprefix()	check string for prefix.
**	FEpresearch()	search argument list for unique prefix.
**
** History:
**	Revision 6.1  88/07/29  wong
**	Modified for CM support.
**
**	Revision 3.0  84/05/30  nadene
**	Initial revision - stolen from vifred
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	FEprefix() -	Check String for Prefix.
**
** Description:
**	This routine does string comparison to see if the input prefix is part
**	of the input string.  It compares the prefix and the string up to a
**	maximum of 'plen' characters.  It is useful for the "Find" operation
**	in various Front-Ends.  If 'plen' is <= 0 then a full compare of the
**	prefix to the string is performed (upto EOS in the string.)
**
** Input:
**	prefix	{char *}  Prefix string.
**	str	{char *}  String to be searched for prefix.
**	plen	{nat}  Number of characters in 'prefix' to be compared.
**
** Returns:
**	{nat}	0	-  prefix is a prefix of str
**		<0	-  prefix is lexically less than str
**		>0	-  prefix is lexically greater than str
**		
** Written:
**	5/30/84 (nml) - stolen from vifred
**	07/29/88 (jhw) -- Added CM support.
*/

i4
FEprefix ( prefix, str, plen )
register char	*prefix;
char		*str;
i4		plen;
{
	register i4	len = ( plen <= 0 ) ? MAXI2 : plen;
	register i4	result = 0;

	while ( len > 0 && *prefix != EOS &&
			(result = CMcmpcase(prefix, str)) == 0 )
	{
		CMnext(prefix);
		CMnext(str);
		--len;	/* one less (double or single byte) character */
	}
	return result;
}

/*{
** Name:	FEpresearch() -	Search Argument List for Unique Prefix.
**
** Description:
**	Returns index of unique prefix in argument list, or FE_NONUNIQUE if
**	prefix is not unique, or FE_NOTEXIST if prefix does not match any
**	argument.
**
** Inputs:
**	argc	{nat}  Size of the argument list.
**	argv	{char *[]}  The argument list, an array of string references.
**	prefix	{char *}  The prefix for which to search.
**
** Returns:
**	{nat}	>= 0  The index of the argument that matched.
**		== FE_NONUNIQUE  If more than one argument matched.
**		== FE_NOTEXIST  If no argument matched.
**
** History:
**	9/19/84 (ab) written
*/

i4
FEpresearch ( argc, argv, prefix )
register i4	argc;
char		*argv[];
register char	*prefix;
{
	register i4	i;
	register i4	length = STlength(prefix);
	i4		place = FE_NOTEXIST;
	bool		found = FALSE;

	for ( i = 0 ; i < argc ; ++i )
	{
		if ( FEprefix(prefix, argv[i], length) == 0 )
		{
			if ( found )
				return FE_NONUNIQUE;
			found = TRUE;
			place = i;
		}
	}
	return place;
}
