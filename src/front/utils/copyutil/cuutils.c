/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>
# include	<cm.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cu.h>
# include	"ercu.h"
# include	"cuntr.h"

/**
** Name:	cuutils.c -	Utility Routines for Copy.
**
** Description:
**	This file defines:
**
**	IICUrtnRectypeToName	Get the string for a rectype.
**	cu_rectype	get the record type for an intermediate file record.
**	cu_gettok	get the next tab-separated token in a string.
**	cu_filtoks	fill a tokens array.
**	cu_inarray	determine is string exists previously in array.
**	cu_ckhdr	check the header of a file.
**
** History:
**	22-Apr-87 (rdesmond) - written.
**	23-jun-1987 (Joe)
**		Took cfutils.c and changed to be for copyutil.
**	10/11/92 (dkh) - Removed readonly qualification on references
**			 declaration to iiCUntrNameToRectype.  Alpha
**			 cross compiler does not like the qualification
**			 for globalrefs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	iicuChkHdr() -	Check the Name and Version in the Header.
**
** Description:
**	This takes a line from a file (presumably the first) and checks
**	the header to see if it matches the program that is expected, and
**	whether the versions match, or a conversion is needed.
**
** Inputs:
**	name		The name to expect in the line.
**
**	linebuf		The input line to check.
**
** Returns:
**	{STATUS} CU_CMPAT	if the header matches and the version of
**				the header is completely compatible.
**
**		CU_FAIL		the header has an invalid format
**		CU_CONV		the intermdiate file needs to be converted
**				(doesn't happen currently)
**		CU_NOCNV	major versions don't match & can't be converted
**		CU_UPWD		user is trying to copy in a file from a later
**				release - can't do this
**
** History:
**	4-aug-1987 (Joe)
**		Initial Version
**	03/90 (jhw) -- Add check for incompatible upward versions.  (That is,
**		user is trying to copy in file from a later release.)
**	6-jan-93 (blaise)
**		Added new status values for the various types of failure;
**		changed return status for major versions less than 6 to reflect
**		the fact that from 6.5 we'll no longer convert 5.0 intermediate
**		files.
*/
STATUS
iicuChkHdr ( name, linebuf, majv, minv )
char	*name;
char	*linebuf;
i4	*majv;
i4	*minv;
{
	char	*nametok;

	char	*cu_gettok();

	*majv = *minv = 0;
	nametok = cu_gettok(linebuf);
	if ( !STequal(name, nametok)
		|| CVan(cu_gettok((char *)NULL), majv) != OK
		|| CVan(cu_gettok((char *)NULL), minv) != OK)
	{
		/* not a valid copyapp intermediate file */
		return CU_FAIL;
	}
	else if (*majv < 6)
	{
		/*
		** From 6.5 we'll no longer convert pre-6.0 intermediate files
		*/
		return CU_NOCNV;
	}
	else if ( *minv > IICUMINOR )
	{
		/* not upward compatible */
		return CU_UPWD;
	}
	else
	{
		/* completely compatible */
		return CU_CMPAT;
	}
}

/*{
** Name:	IICUrtnRectypeToName	- Get Name for a rectype.
**
** Description:
**	Given a rectype returns the string for it in a file.
**	Does not include a ':'.
**
**	The return string is in READONLY storage.
**
** Inputs:
**	rectype		The rectype to find a name for.
**
** Outputs:
**	Returns:
**		The name for the rectype, or NULL if not found.
**
** History:
**	26-jul-1987 (Joe)
*/
char *
IICUrtnRectypeToName ( rectype )
OOID	rectype;
{
	GLOBALREF CUNTR	iiCUntrNameToRectype[];

	register CUNTR	*c;

	for ( c = iiCUntrNameToRectype ; c->curectype != CU_BADTYPE ; ++c )
	{
		if ( c->curectype == rectype )
			return c->cuname;
	}
	return NULL;
}

/*{
** Name:	IICUrtoRectypeToObjname	- Get Object Name for a rectype.
**
** Description:
**	Given a rectype returns the string which says what kind
**	of object it is for.
**
**	The return string is in READONLY storage.
**
** Inputs:
**	rectype		The rectype to find a name for.
**
** Outputs:
**	Returns:
**		The name for the rectype, or NULL if not found.
**
** History:
**	9-aug-1987 (Joe)
**		Initial Version
*/
char *
IICUrtoRectypeToObjname(rectype)
OOID	rectype;
{
	GLOBALREF CUNTR	iiCUntrNameToRectype[];

	register CUNTR	*c;

	for ( c = iiCUntrNameToRectype ; c->curectype != CU_BADTYPE ; ++c )
	{
		if ( c->curectype == rectype )
		{
			if ( c->cunmid == 0 )
				return NULL;
			/*
			** These are always FAST strings
			** so it is okay to return a pointer to
			** them
			*/
			return ERget(c->cunmid);
		}
	}
	return NULL;
}

/*{
** Name:	cu_rectype - get the record type of a record from the
**				intermediate COPYFORM file.
**
** Description:
**	Given a ptr to an input string, returns the record type of the
**	intermediate form file.  The record type is determined by a pattern
**	at the beginning of the string passed.  If the first character of
**	the input string is a tab, then the record type is determined from the
**	record type from the last call to this procedure.
**
** Input params:
**	linebuf		ptr to record from file.
**
** Output params:
**
** Returns:
**	record_type found in record type table or
**	RT_BADTYPE.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	23-Apr-87 (rdesmond) written.
**	23-jun-1987 (Joe)
**		Modified for copyutl.
*/
i4
cu_rectype ( linebuf )
char	*linebuf;
{
	GLOBALREF CUNTR	iiCUntrNameToRectype[];

	register CUNTR	*c;

	if (linebuf[0] == '\t')
	{
		return CUD_LINE;
	}
	for ( c = iiCUntrNameToRectype ; c->curectype != CU_BADTYPE ; ++c )
	{
	    /*
	    ** Names are always written in UPPERCASE.
	    ** Since linebuf is the record from the file the name
	    ** is delimited by a ':'.  This first checks to make
	    ** sure there is a ':' at the length of the name
	    ** we will check it against.
	    */
	    if (linebuf[c->cunmlen] == ':' &&
	        STbcompare(linebuf, 0, c->cuname, c->cunmlen, FALSE) == 0)
	    {
		return c->curectype;
	    }
	}
	return CU_BADTYPE;
}

/*{
** Name:	cu_filtoks	- Fill up a token array.
**
** Description:
**	Given a line from a copy app file, this routine fills
**	up an array of pointers to point at the tokens in the
**	line.
**
** Inputs:
**	line		The line from the file.
**
** Outputs:
**	tokens		The token array.  Assumed to be big enough.
**
** History:
**	26-jul-1987 (Joe)
**		Initial Version.
*/
VOID
cu_filtoks(line, tokens)
char	*line;
char	**tokens;
{
    register char	**tp = tokens;

    char	*cu_gettok();

    for ( cu_gettok(line) ; (*tp = cu_gettok((char *)NULL)) != NULL ; ++tp )
	;
}

/*{
** Name:	cu_gettok - get next tab-separated token from string
**
** Description:
**	Simple scanner to retrieve tokens from a string.  Called first
**	with the pointer to a string, then successively called with a null 
**	pointer to return the next token in the string.  A pointer to the
**	token is returned with a null character replacing the tab separator
**	(this is destructive to the input string).  A token with a null
**	value is legal, i.e. if there are two consecutive tabs in the input
**	string.  When the end of the string is reached, delimited by either
**	a null or a new-line character, a null pointer will be returned.
**	Tokens may have embedded spaces, and there is no compression done
**	on tokens.
**
** Input params:
**	instring	pointer to input string or NULL for successive calls
**
** Output params:
**	none
**
** Returns:
**	pointer to next token in string
**
** Exceptions:
**	none
**
** Side Effects:
**	Destroys input string by replacing tabs with nulls.
**
** History:
**	19-Apr-87 (rdesmond) written.
*/
static char	*save_loc;

char *
cu_gettok ( instring )
char	*instring;
{
	char		*start_loc;
	register char	*curr_loc;

	start_loc = (instring != NULL) ? instring : save_loc;
	if (start_loc == NULL)
		return NULL;

	curr_loc = start_loc;

	while ( *curr_loc != EOS && *curr_loc != '\t' && *curr_loc != '\n' )
		CMnext(curr_loc);

	save_loc = (*curr_loc == '\t') ? curr_loc + 1 : NULL;
	*curr_loc = EOS;

	return start_loc;
}

/*{
** Name:	cu_inarray - determine if string appears in array of strings
**
** Description:
**	given an array of ptrs to strings and an index, determine if the string
**	pointed to by the element of the array referenced by index appears in
**	the array before the given index.  This is used when there can be
**	multiple occurences of a string in the array to determine if the
**	string occurs previously.
**	
**
** Input params:
**	array		array of pointers to strings
**	index		index into array pointing to string to be checked.
**
** Output params:
**	none
**
** Returns:
**	TRUE		string appears previously in array.
**	FALSE		string does not appear previously in array.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	22-Apr-87 (rdesmond) - written.
*/
bool
cu_inarray ( array, index )
char	*array[];
i4	index;
{
	register i4  i;

	for ( i = 0 ; !STequal(array[i], array[index]) ; ++i )
		;

	return (bool)( i != index );
}

/*{
** Name:	IICUbsdBSDecode
**
** Description:
**	Strip backslash encoded control characters from string.
**	This is done in place.  This undoes IICUbseBSEncode.
**	Note that we simplify octal character encoding by ALWAYS
**	using 3 characters, so that we don't have to worry about
**	octal encoded characters running into real digits, etc.
**
** Input params:
**	str - string.
**
** History:
**	3/90 (bobm) written.
**	10/90 (bobm) fix \\ decoding.
*/

VOID
IICUbsdBSDecode(str)
char *str;
{
	char *out;
	char nb[4];
	char real;
	i4 oc;

	for (out = str; *str != EOS; CMnext(out),CMnext(str))
	{
		/*
		** native character comparison.
		*/
		if (*str != '\\')
		{
			CMcpychar(str,out);
			continue;
		}

		/*
		** we have a backslash
		*/
		CMnext(str);

		/*
		** legit - these are all native char comparisons
		*/
		switch(*str)
		{
		case 't':
			CMcpychar(ERx("\t"),out);
			break;
		case 'n':
			CMcpychar(ERx("\n"),out);
			break;
		case 'r':
			CMcpychar(ERx("\r"),out);
			break;
		case 'b':
			CMcpychar(ERx("\b"),out);
			break;
		case 'f':
			CMcpychar(ERx("\f"),out);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
			/*
			** There are supposed to be 3 numeric digits
			** here, but we check for EOS anyway.
			*/
			nb[0] = *str;
			CMnext(str);
			nb[1] = *str;
			if (*str != EOS)
				CMnext(str);
			nb[2] = *str;
			nb[3] = EOS;
			CVol(nb,&oc);
			nb[0] = oc;
			nb[1] = EOS;
			CMcpychar(nb,out);
			break;
		default:
			CMcpychar(str,out);
			break;
		}

		/* safety - this shouldn't happen */
		if (*str == EOS)
		{
			CMnext(out);
			break;
		}
	}

	*out = EOS;
}

/*{
** Name:	IICUbseBSEncode
**
** Description:
**	Backslash encode string for decoding by above routine.
**	This is NOT done in place, since STcopy doesn't conveniently
**	allow you to right shift a string.  Not that to be bombproof
**	caller's output buffer must be 4 times the length of the input,
**	but in most cases nowhere near that length is required.
**
** Input params:
**	in - input string.
**
** Output
**	out - output string.
**
** History:
**	3/90 (bobm) written.
*/

VOID
IICUbseBSEncode(in,out)
char *in;
char *out;
{
	i4 len;
	i4 i;

	for ( ; *in != EOS; CMnext(in),out += len)
	{
		switch(*in)
		{
		case '\n':
			STcopy(ERx("\\n"),out);
			len = 2;
			break;
		case '\t':
			STcopy(ERx("\\t"),out);
			len = 2;
			break;
		case '\b':
			STcopy(ERx("\\b"),out);
			len = 2;
			break;
		case '\r':
			STcopy(ERx("\\r"),out);
			len = 2;
			break;
		case '\f':
			STcopy(ERx("\\f"),out);
			len = 2;
			break;
		case '\\':
			STcopy(ERx("\\\\"),out);
			len = 2;
			break;
		default:
			len = CMbytecnt(in);
			if ( ! CMcntrl(in) )
			{
				CMcpychar(in,out);
				break;
			}

			/*
			** octal encode control characters.  If there
			** are multiple bytes, octal encode each one.
			*/
			for ( i = 0 ; i < len ; ++i )
				STprintf(out+4*i,ERx("\\%03o"),(i4) in+i);
			len *= 4;
			break;
		}
	}

	*out = EOS;
}
