/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<cm.h>
#include	<fe.h>
#include	<st.h>
#include	<ui.h>

/**
** Name:    ugwildcard.c
**
** Description:
**
** Defines:
**	IIUGwildcard()
**	IIUGscanwildcard()
**	IIUGewc_EscapeWildcard()
**
** History:
**  11-nov-1987 (danielt)
**		written 
**	05-jun-1992 (rdrane)
**		Redefined and re-written.  No current client actually uses the sql_wild
**		parameter, and it was only being set properly for NULL pointers and
**		empty strings.  In addition, it was unclear from the previous comments
**		if the intent was to return TRUE if it found an SQL wildcard in the
**		input string, placed an SQL wildcard in the output string, or both.
**		As a result, the sql_wild parameter is being retained for compatability
**		and future use.  It will be set to TRUE if the resultant string contains
**		SQL wildcards, regardless of their origin.
**
**		The output description now reflects that the routine simply scans
**		an input string and converts QUEL wildcards to SQL wildcards IN PLACE.
**		The scan is done via the CM routines to	ensure support for double-byte
**		characters (this was not the case previously).
**
**	19-jun-1992 (rdrane)
**		Added IIUGscanwildcard() routine (SQL only).  Besides ignoring
**		conversion of QUEL wildcards, the routine accepts specification of
**		an escape character, does not flag SQL wildcards so escaped, and does
**		flag brackets so escaped (range specification).
**
**		Changed the setting of sql_wild in IIUGwildcard() to reflect TRUE only
**		if a substitution occurred.  The previous change was to support the
**		functionality added by IIUGscanwildcard(), and it actually introduced
**		more problems than it solved (it didn't support skipping of escaped
**		wildcards, nor would it recognize a bracket range).
**	20-jul-1992
**		Added IIUGewc_EscapeWildcard() routine (SQL only).  Effect escaping
**		of SQL wildcards.
*/

/*
** Name:    IIUGwildcard() -
**
** Description:
**
** Inputs:
**	pattern	    address of string to be converted
**	sql_wild	address of boolean to receive conversion status
**
** Outputs:
**	pattern	    string with any QUEL wildcards converted to SQL wildcards
**	sql_wild	TRUE -	QUEL wildcards were found and SQL wildcards were
**						substituted in their place.
**				FALSE -	No substitutions occurred.  The string may or may not
**						contain SQL wildcards.
**
** History:
**	2-nov-1987 (danielt)
**	    written
**	05-jun-92 (rdrane)
**		Re-defined and re-written.  See module history for details.
**	19-jun-92 (rdrane)
**		Redefined the setting of sql_wild (again).  We no longer look for
**		SQL wildcards since they don't mean anything in the context of this
**		routine (it's a character substitution routine.  Period.).
*/

VOID
IIUGwildcard(pattern,sql_wild)
			char	*pattern;
			bool	*sql_wild;
{
	register char	*p;


	*sql_wild = FALSE;		/* Assume no SQL wildcards		*/

	if  (pattern == NULL)	/* Ignore NULL string pointers!	*/
	{
		return ;
	}
							/* We assume that a pattern match character
							** will never be a double byte character!
							** It would be nice if these were #define'd
							** at the system level ...  maybe in dbms.h?
							*/
	p = pattern;
	while (*p != EOS)		/* Empty string will return imediately	*/
	{
		switch(*p)
		{
		case '*':			/* Convert QUEL wildcards				*/
			*p = '%';
			*sql_wild = TRUE;
			break;
		case '?':
			*p = '_';
			*sql_wild = TRUE;
			break;
		default:
			break;
		}
		CMnext(p);
	}

	return;
}

/*
** Name:    IIUGscanwildcard() -
**
** Description:
**		Determine if a string contains SQL wildcards, and so should be the
**		object of a LIKE predicate instead of an '=' operator.
**
**		Note that no detection/reporting of unmatched open brackets is
**		performed.
**
** Inputs:
**	pattern	    address of the NULL terminated string to be scanned.
**	sql_escape	character to be used as the escape character.  Note that we
**				assume that this will always be a single-byte character.
**				For use with FE routines, this should always be backslash.
**				This should be EOS if no escape character is defined.
**
** Returns:
**	TRUE -	the string contains active pattern matching characters.
**	FALSE -	the string does not contain any pattern matching characters,
**			is empty, or was specified as a NULL pointer.
**
** History:
**	19-jun-92 (rdrane)
**		Created.  It's what was intended for the 05-jun-92 change to
**		IIUGwildcard(), only now we don't care about QUEL.
*/

bool
IIUGscanwildcard(pattern,sql_escape)
			char	*pattern;
			char	sql_escape;
{
	register char	*p;
			bool	sql_wild;


	sql_wild = FALSE;		/* Assume no SQL wildcards		*/

	if  (pattern == NULL)	/* Ignore NULL string pointers!	*/
	{
		return(sql_wild);
	}

	/* We assume that a pattern match character will never be a double byte
	** character!  It would be nice if these were #define'd at the system
	** level ...  maybe in dbms.h?
	*/
	p = pattern;
	while (*p != EOS)		/* Empty string will return imediately	*/
	{
		/*
		** Character following an escape character is not a wild card
		** unless it is an open bracket for a range specification.  If we
		** see an escaped close bracket, then there's a syntax error, but
		** that's not our problem ...
		*/
		if  (*p == sql_escape)
		{
			CMnext(p);
			if  (*p == EOS)	/* Don't get caught here!	*/
			{
				return(sql_wild);
			}
			if  (*p == '[')	/* Escaped bracket signifies a range match	*/
			{
				sql_wild = TRUE;
			}
		}
		else
		{
			switch(*p)
			{
			case '%':			/* Flag unescaped SQL wildcards	*/
			case '_':
				sql_wild = TRUE;
				break;
			default:
				break;
			}
		}
		CMnext(p);
	}

	return(sql_wild);
}

/*
** IIUGewc_EscapeWildcard -
**
** Description:
**	Detects and escapes SQL wildcards in a string.  This permits delimited
**	identifiers to be prepared for use in pattern matching situations.
**  The string is not expected to have extraneous trailing blanks, is not
**  expected to exceed FE_MAXNAME in length, and is expected to be NULL
**  terminated.
**
**	The string is expected to be in normalized form, i.e., there are no
**	"already escaped wildcards".  Thus, '\%' is transformed into '\\\%'.
**
**	Range matches are not well behaved.  This routine will transform
**	'\[A-Z\]' into '\\[A-Z\\]'.  This may or may not be what was intended,
**	but remember that for an identifier in normalized form, there is no
**	such thing as a range match specification!
**
** Inputs:
**	in_str		Character pointer to the NULL terminated string to be
**				escaped.  Assumed to reside in a buffer of max length
**				(FE_MAXNAME + 1).
**	out_str		Character pointer to the buffer in which to place the
**				result string, which will have any SQL wildcards escaped.
**				Assumed to go into a buffer with a minimum of
**				((2 * FE_MAXNAME) + 2 + 1) available length.
**	sql_escape	character to be used as the escape character.  Note that we
**				assume that this will always be a single-byte character.
**				For use with FE routines, this should always be backslash.
**				If specified as EOS, then no escape character is defined
**				and this routine simply copies in_str to out_str.
**
** Output:
**	out_str		The escaped string representation of the input.  A NULL
**				or empty input string will be reflected as EOS.
**
** Returns:
**	Nothing.
**
** History:
**	20-jul-1992 (rdrane)
**		Created.
*/

VOID
IIUGewc_EscapeWildcard(in_str,out_str,sql_escape)
			char	*in_str;
			char	*out_str;
			char	sql_escape;
{


	if  (out_str == NULL)
	{
		return;
	}

	if  ((in_str == NULL) || (*in_str == EOS))
	{
		*out_str = EOS;
		return;
	}
	
	if  (sql_escape == EOS)
	{
		STcopy(in_str,out_str);
		return;
	}

	while (*in_str != EOS)
	{
		switch(*in_str)
		{
		case '%':
		case '_':
			if  (sql_escape != EOS)
			{
				*out_str++ = sql_escape;
			}
			break;
		default:
			/*
			** Look for the escape character.  If it's EOS, we'll never
			** find one!
			*/
			if  (*in_str == sql_escape)
			{
				*out_str++ = sql_escape;	/* Always put the escape char	*/
			}
			break;
		}
		CMcpyinc(in_str,out_str);
	}

	*out_str = EOS;
	return;
}
