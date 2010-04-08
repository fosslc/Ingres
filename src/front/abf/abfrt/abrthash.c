/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cm.h> 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<abfcnsts.h>

/**
** Name:	abrthash.c - 	ABF RunTime Hash Function for Name Lookup.
**
** Description:
**	Contains the hash functions that are used for name lookup by the ABF
**	runtime system and when the runtime tables are built.  Defines:
**
**	iiarHash()		hash a name for lookup.
**	iiarThTypeHash()	hash a class-name for lookup.
**
**	NOTE:  This routine is internal to the ABF runtime system.
**
** History:
**	Revision 5.1  86/08/14  joe
**	Extracted from "abfrts" for PC overlay work.
**	11/21/92 (dkh) - Created iiarSysHash() that does not lowercase
**			 the names.  This is for use on system objects
**			 that are preallocated using string literlas for
**			 names.  These string literals are read only and
**			 can't be updated.  Thus, the need for iiarSysHash().
**			 This does mean that allocation of system objects must
**			 only use lower case names and have no trailing blanks.
**			 Appropriate warning will be placed in the appropriate
**			 iaom (iamsystm.c) file that declares the system
**			 objects.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	iiarHash() -	Hash a Name for Lookup.
**
** Description:
**	Hash a name for a hash table of size ABHASHSIZE.
**
** Input:
**	name	{char *}  The name to be hashed.
**
** Returns:
**	{nat}  A hash index for the name.
**
** Side Effects:
**		lowercases the formname and strips trailing blanks.
**
** History:
**	85/04 (jrc) -- Written.
**	8-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets
**	    which can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
*/
i4
iiarHash ( name )
register u_char	*name;
{
	register i4	i, j;

	i = 0;
	while ( *name != EOS )
	{
		/* We don't allow blanks here.  Especially trailing blanks! */
		if (*name == ' ')
		{
			*name = EOS;
			break;
		}

		CMtolower(name, name);
		i = (i * 3) + (i4) *name;
		if ( CMdbl1st(name) )
		{
		    for (j = CMbytecnt(name); j > 1; j--)
			i = (i * 3) + (i4) *++name;
		}
		++name;
	}
	return ( (i %= ABHASHSIZE) < 0 ) ? -i : i;
}

/*{
** Name:	iiarThTypeHash() -	Hash a Class Name for Lookup.
**
** Description:
**	Hash a name for a hash table of size ABHASHSIZE.  Differs from
**	iiarHash only in that it does not strip trailing blanks.
**
** Input:
**	name	{char *}  The name to be hashed.
**
** Returns:
**	{nat}  A hash index for the name.
**
** Side Effects:
**		lowercases the formname.
**
** History:
**	89/11 (billc) -- Stolen.
**	8-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets
**	    which can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
*/
i4
iiarThTypeHash ( name )
register u_char	*name;
{
	register i4	i, j;

	i = 0;
	while ( *name != EOS )
	{
		CMtolower(name, name);
		i = (i * 3) + (i4) *name;
		if ( CMdbl1st(name) )
		{
		    for (j = CMbytecnt(name); j > 1; j--)
			i = (i * 3) + (i4) *++name;
		}
		++name;
	}
	return ( (i %= ABHASHSIZE) < 0 ) ? -i : i;
}

/*{
** Name:	iiarSysHash() -	Hash a System Name for Lookup.
**
** Description:
**	Hash a system name for a hash table of size ABHASHSIZE.  Difference
**	between this and the original iiarHash() routine is that this
**	one does not touch the name at all.  No lower casing or trimming
**	of trailing blanks.
**
** Input:
**	name	{char *}  The name to be hashed.
**
** Returns:
**	{nat}  A hash index for the name.
**
** Side Effects:
**	None.  The name is left alone.
**
** History:
**	11/21/92 (dkh) - Cloned from iiarHash().
**	8-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets
**	    which can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
*/
i4
iiarSysHash ( name )
register u_char	*name;
{
	register i4	i, j;

	i = 0;
	while ( *name != EOS )
	{
		i = (i * 3) + (i4) *name;
		if ( CMdbl1st(name) )
		{
		    for (j = CMbytecnt(name); j > 1; j--)
			i = (i * 3) + (i4) *++name;
		}
		++name;
	}
	return ( (i %= ABHASHSIZE) < 0 ) ? -i : i;
}
