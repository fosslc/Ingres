/*
**Copyright (c) 1987, 2004 Ingres Corporation
** All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	"gnspace.h"
# include	"gnint.h"

/**
** Name:	gnhfunc.c - hash and compare functions
**			GN internal use only
**
** Description:
**	hash function and key comparison function used for the hashtables
**	in GN.  The hash function only compares keys up to the length
**	required for uniqueness in the name space.  This assures that
**	all colliding names entered as "old" are entered to the same
**	hash table slot.   The reason this is important is that the
**	comparison we want is different at different points.  We want
**	to find an entry which matches to within uniqueness length
**	when we are generating new names.  We want to find an exact
**	match when we are entering an old name, or deleting a name.
**
**	Example:
**
**		Suppose old names "xyzijk" and "xyzabc" are entered in a space
**		with uniqueness length = 3.  When we try to generate say
**		"xyzxyz" as a new name, we want to find a collision.  The
**		hash function will assure that all xyz... names are in
**		the same hash slot, so that when we only compare the first
**		three characters of the new name, we WILL find one of the
**		two old names.  For the purposes of verifying uniqueness,
**		it doesn't matter which one.  NOW, when we want to DELETE
**		"xyzabc", the comparison should be full string, so we find
**		THAT specific entry, and not the "xyzijk" entry it collided
**		with in the hash table.
**	
** Defines
**	hash_func
**	comp_func
**
** History:
**	13-feb-91 (blaise)
**		Added history section.
**		Changed comp_func to call STbcompare with both length arguments
**		set to zero. This causes STbcompare() to compare the entire
**		strings, and not just the first n characters, where n is the
**		length of the shorter string. Bug #35004.
**	27-mar-91 (blaise)
**		Changed comp_func to check whether the first Clen characters
**		of the two strings are equal (Clen is the comparison length),
**		rather than the whole string. Bug #36634.
**	05-apr-93 (dkhor)
**		Fixed problem with DOUBLEBYTE VISION applname (e.g. those
**		starting with 0xB5EE).  Cast *s to unsigned char *s in 
**		the for-loop in hash_func.
**	02-jun-93 (dkhor)
**              Fixed SEGV problem with DOUBLEBYTE VISION applname (e.g. those
**              starting with 0xB5EE).  Cast *s to unsigned char *s in
**              the for-loop in hash_func.
**	9-jun-93 (blaise)
**		Removed the fix above, and instead changed s from char* to
**		u_char*. The ANSI C compiler doesn't like casting a char*
**		to an unsigned char*.
**	29-jul-93 (blaise)
**		More casting to keep the ansi C compiler happy - it doesn't
**		like "%" to have one signed operand and one unsigned operand.
*/

extern i4  Hlen;
extern i4  Clen;
extern bool Igncase;

/*{
** Name:   hash_func
**
** Description:
**	hash function.   This essentially uses the same algorithm as the
**	IIUGhiHtabInit default routine, but it also stops at the last
**	complete character before Hlen bytes, and pays attention to case.
**	The later is important to assure that differently cased versions
**	of a string hash to the same location.  Without a length limit,
**	we wouldn't care about multi-byte characters, but because of
**	the way characters are inserted to make unique names, we do not
**	want to hash half a character.  Table size has been set up as
**	a prime number so that the division of string modulo table size
**	algorithm produces good numbers.  Routine will hash different
**	cased versions of the key to the same location, but the original
**	case will be preserved.
**
** Inputs:
**
**	key
**	size
**
**     return:
**		table index
*/

hash_func(s,size)
register u_char *s;
i4  size;
{
	register i4 rem;
	register i4  count;
	register i4  newcount;
	register bool cflag;

	rem = count = newcount = 0;
	while (*s != EOS)
	{
		/*
		** get the byte count to end of next character, break
		** out if this CHARACTER pushes us past Hlen.
		*/
		if ((newcount += CMbytecnt(s)) >= Hlen)
			break;
		/*
		** now hash all bytes of the character
		** If upper case, we turn into lower case, then
		** back to upper case.
		*/
		for ( ; count < newcount; count++)
		{
			if (cflag = CMupper(s))
				CMtolower(s,s);
			rem = (i4)(rem * 128 + *s) % size;
			if (cflag)
				CMtoupper(s,s);
			++s;
		}
	}

	return((i4) rem);
}

/*{
** Name:   comp_func
**
** Description:
**	comparison function.  string compare with a length limit.
**	this may also ignores case, so we may freely make generated names
**	any case we like, but they will be checked for collision in
**	a case insensitive fashion.  This flag is set by find_space().
**	We only compare with case significant if the space's case rule is
**	GN_SIGNIFICANT.
**
** Inputs:
**
**	s1, s2 : strings for comparison.
**
**     return:
**		table index
*/

comp_func(s1,s2)
char *s1;
char *s2;
{
	STATUS	status;
	int	len1, len2;

	/*
	** Compare the string lengths. If they're different, and at least one
	** of the strings is shorter than the comparison length, then the
	** strings are different and we don't need to call STbcompare. We need
	** this comparison because STbcompare considers two strings to be equal
	** if one is a substring of the other.
	*/
	len1 = STlength(s1);
	len2 = STlength(s2);
	if ((len1 < Clen || len2 < Clen) && len1 != len2)
	{
		status = FAIL;
	}
	else
	{
		status = (STbcompare(s1,Clen,s2,Clen,Igncase));
	}
	return status;
}
