/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<st.h>		 
# include	<me.h>		 
# include	<cv.h>		 
#include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"iamstd.h"
#include	"iamtok.h"

/**
** Name:	iamtok.c -	Encoded IL Token Parsing Module.
**
** Description:
**
**	Token scanning utilities for OO encoded string parsing.  Internal to
**	IIAM.  The problem is that OO catalog format breaks database records
**	containing the encoded string in a completely arbitrary fashion, and
**	we can't read the entire string into memory to analyze it.
**
**	The way these routines work:
**
**	Caller calls 'tok_setbuf()' to set up a buffer area into which to read
**	data.  Caller passes in maximum length of a single read, and the tag
**	to use in allocating storage for tokenis.  Caller may then make one
**	read into buffer address which is passed back.
**
**	Caller calls 'tok_next()' to obtain tokens from the buffer.  These
**	routines keep internal pointers to handle and convert successive
**	tokens.  The input string may end in mid-token, in which case these
**	routines keep a pointer to the partial token remaining, as well as
**	returning the "end of string" token.	Returns OK, or failure for
**	memory allocation.
**
**	When the "end of string" token is obtained, the caller calls
**	'tok_catbuf()' if there is more data to be read.  This shifts the
**	remaining string to be interpreted to the beginning of the buffer,
**	and returns to the caller an address to read at to concatenate to the
**	remaining input string.	 Caller may then proceed with more 'tok_next()'
**	calls.  The caller may actually NOT do a read after 'tok_setbuf()'.
**	The first tok_next call will immediately return "end of string", and
**	'tok_catbuf()' will return the beginning of the buffer.  This may be a
**	more convenient way for the caller to read buffers.
**
**	tok_setbuf allocates a dynamic buffer which may be increased by
**	tok_catbuf if more is needed.  Caller will call tok_bfree() after
**	all parsing has taken place to free memory.
**
**	The caller may use the 'tok_more()' call after an "end of string" token
**	to see if there is a partial token left in the input buffer.  Useful
**	to check that there was not an expected end after there is no more
**	input to be read.
**
** History:
**	Revision 6.4
**	03/11/91 (emerson)
**		Integrated another set of DESKTOP porting changes.
**
**	Revision 6.3  90/03  wong
**	Cleaned up scanning to correctly use CM module only when necessary.
**	3/90 (bobm)	made buffers dynamic.
**
**	Revision 6.1  88/08  wong
**	Changed to use 'FEreqmem()'.
**
**	Revision  5.1  86/08  bobm
**	Initial revision.
**
**	25-jul-91 (johnr)
**		hp3_us5 no longer requires NO_OPTIM.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	24-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

static i4	scan_nat();
static i4	scan_long();
static f8	scan_float();
static char	*scan_str();
static STATUS	scan_xs();

/*
** debugging requires UNIX/MSDOS environment.  oo_decode opens this file
*/

static char	*Bufr = NULL;		/* input buffer */
static char	*Current = ERx("");	/* current token */
static i4	MaxReadLen = 0;		/* max buffer read */
static i4	Buflen = 0;		/* actual size of allocated buffer */
static u_i4	Tag = 0;		/* storage tag for allocations */

/*
** tok_more returns TRUE if there is input remaining to be parsed, FALSE
** otherwise
*/
bool
tok_more()
{
	return (bool)( *Current != EOS );
}

/*
** tok_setbuf sets up a buffer where text for scanning will be placed.
** The maximum length for a token is also passed in.  Following this call,
** the caller may legally read a record at addr.  There will be enough
** room in the returned buffer for len PLUS an EOS.  We actually allocate
** a bit more in anticipation of tok_catbuf()
**
** return NULL for allocation error.
*/

char *
tok_setbuf ( len, st_tag )
i4	len;
u_i4	st_tag;
{
	/* allow 1.5 times read length for initial buffer */
	Buflen = (3 * len)/2 + 1;

	MaxReadLen = len;
	Tag = st_tag;

	Bufr = (char *) MEreqmem(0, Buflen, FALSE, NULL);
	if (Bufr == NULL)
	{
		Current = ERx("");
		Buflen = 0;
		return NULL;
	}

	*Bufr = EOS;
	Current = Bufr;

	return Bufr;
}

/*
** tok_catbuf returns an address to read more characters at, or NULL if
** current string is too long to allow it.  tok_catbuf shifts the current
** string to the beginning of the buffer, and returns a pointer to its
** terminating '\0'.  It may allocate a new buffer if the existing one
** will not allow MaxReadLen bytes to be read.
*/

char *
tok_catbuf()
{
	i4 len;
	char *old;

	len = STlength(Current);
	old = NULL;

	/* <= to assure room for EOS */
	if ((Buflen - len) <= MaxReadLen)
	{
		old = Bufr;
		Buflen = len + MaxReadLen + 1;
		Bufr = (char *) MEreqmem(0, Buflen, FALSE, NULL);
		if (Bufr == NULL)
		{
			Current = ERx("");
			Buflen = 0;
			if (old != NULL)
				MEfree((PTR) old);
			return NULL;
		}
	}

	/* STcopy is supposed to work for left shifts */
	STcopy(Current,Bufr);
	Current = Bufr;

	if (old != NULL)
		MEfree((PTR) old);

	return Bufr+len;
}

tok_bfree()
{
	MEfree((PTR) Bufr);
	Current = ERx("");
	Bufr = NULL;
	Buflen = 0;
}

/*
** scan and decode next token and return to caller.
*/
STATUS
tok_next (tok)
register TOKEN	*tok;
{
	char	*term;

	/* skip over whitespace */
	while ( CMwhite(Current) )
		CMnext(Current);

	/* "default" terminator address = next character */
	term = ( *Current != EOS ) ? Current+1 : Current;

	/*
	** char gives us token type.  If an unexpected end is encountered
	** while converting argument, set term to Current as well as setting
	** type TOK_eos, so we may reparse the token properly after caller has
	** called tok_catbuf and read more characters.
	*/
	switch (*Current)
	{
	case 'i':
		tok->dat.i2val = scan_nat(term,&term);
		if ( *term == EOS )
		{
			term = Current;
			tok->type = TOK_eos;
			break;
		}
#ifdef AOMDTRACE
		aomprintf(ERx("\nshort %d "),tok->dat.i2val);
#endif
		tok->type = TOK_i2;
		break;
	case 'F':
		tok->dat.f8val = scan_float(term,&term);
		if ( *term == EOS )
		{
			term = Current;
			tok->type = TOK_eos;
			break;
		}
#ifdef AOMDTRACE
		aomprintf(ERx("\nfloat %12.4g "),tok->dat.f8val);
#endif
		tok->type = TOK_f8;
		break;
	case 'I':
		tok->dat.i4val = scan_long(term,&term);
		if ( *term == EOS )
		{
			term = Current;
			tok->type = TOK_eos;
			break;
		}
#ifdef AOMDTRACE
		aomprintf(ERx("\nlong %d "),tok->dat.i4val);
#endif
		tok->type = TOK_i4;
		break;
	case '"':
	{
		char	tchar;

		if ((tok->dat.str = scan_str(term,&term,&tchar)) == NULL)
		{
			if ( tchar != EOS )
				return FAIL;
			term = Current;
			tok->type = TOK_eos;
			break;
		}
#ifdef AOMDTRACE
		aomprintf(ERx("\nstring \"%s\" "),tok->dat.str);
#endif
		tok->type = TOK_str;
		break;
	}
	case 'X':
	{
		char	tchar;

		if ( scan_xs(term, &term, &tchar, tok) != OK )
		{
			if ( tchar != EOS )
				return FAIL;
			term = Current;
			tok->type = TOK_eos;
			break;
		}
#ifdef AOMDTRACE
		aomprintf(ERx("\nhex string length %ld "),tok->dat.hexstr->db_t_count);
#endif
		tok->type = TOK_xs;
		break;
	}
	case 'A':
		tok->dat.i4val = scan_long(term,&term);
		if ( *term == EOS )
		{
			term = Current;
			tok->type = TOK_eos;
			break;
		}
#ifdef AOMDTRACE
		aomprintf(ERx("\narray %d "),tok->dat.i4val);
#endif
		tok->type = TOK_obj;
		break;
	case '{':
#ifdef AOMDTRACE
		aomprintf(ERx("\n{ "),tok->dat.i4val);
#endif
		tok->type = TOK_start;
		break;
	case '}':
#ifdef AOMDTRACE
		aomprintf(ERx("\n} "),tok->dat.i4val);
#endif
		tok->type = TOK_end;
		break;
	case EOS:
		tok->type = TOK_eos;
		break;
	case '*':
		tok->dat.i4val = scan_long(term,&term);
		if ( *term == EOS )
		{
			term = Current;
			tok->type = TOK_eos;
			break;
		}
#ifdef AOMDTRACE
		aomprintf(ERx("\nid %d "),tok->dat.i4val);
#endif
		tok->type = TOK_id;
		break;
	case 'V':
		tok->dat.i4val = scan_long(term,&term);
		if ( *term == EOS )
		{
			term = Current;
			tok->type = TOK_eos;
			break;
		}
		if (*term == '\n')
			++term;
		tok->type = TOK_vers;
#ifdef AOMDTRACE
		aomprintf(ERx("\nVERSION %d\n"),tok->dat.i4val);
#endif
		break;
	default:
		tok->type = TOK_char;
		tok->dat.i2val = (i2) *Current;
#ifdef AOMDTRACE
		aomprintf(ERx("\nWHAT!!!! ASCII %d !!!\n"),tok->dat.i2val);
#endif
		break;
	} /* end switch */

	Current = term;
	return OK;
}

/*
** local conversion routines.  Local routines are used to avoid multiple
** passes on string.  These routines convert as they parse the string,
** returning the pointer to the terminating character as an argument.
**
** Note:  Routines that scan for numbers do not use 'CMnext()' while
** scanning digits.  (Digits are guaranteed as single-byte characters.)
**
** scan_nat - convert integer.	Separated from scan_long in order to use
**	"nat" arithmetic, which is much faster than i4 on PC's.
**
** scan_long - convert i4.
**
** scan_float - convert f8.  Parses out the individual parts of the string,
**	converting them as integers before calculating the floating point
**	result, minimizing the floating point calculations.
*/

static i4
scan_nat ( str, term )
register char *str;
char	**term;
{
	register i4	num;
	i4		sign;

	for ( sign = 1 ; *str == '-' || *str == '+' ; ++str )
		if (*str == '-')
			sign = -sign;
	for ( num = 0 ; CMdigit(str) ; ++str )
		num = num*10 + (*str - '0');

	*term = str;
	return num * sign;
}

static i4
scan_long ( str, term )
register char *str;
char	**term;
{
	register i4	num;
	i4			sign;

	for ( sign = 1 ; *str == '-' || *str == '+' ; ++str )
		if (*str == '-')
			sign = -sign;
	for ( num = 0 ; CMdigit(str) ; ++str )
		num = num*10 + (*str - '0');

	*term = str;
	return num * sign;
}

#ifdef SPECIALFLOAT
/*
** Name:	scan_slong() -	Scan Long Integer.
**
** Description:
**	Scans long integer specification with an overflow check that assumes
**	the integer is positive and does not begin with the zero digit.  It
**	will return the count of digits scanned.
*/
static i4
scan_slong ( str, term, count )
register char	*str;
char		**term;
i4		*count;
{
	register i4	num;
	register i4		cnt;

	cnt = 0;
	for ( num = 0 ; CMdigit(str) ; ++str )
	{
		register i4	newnum;

		++cnt;
		if ( (newnum = num*10 + (*str - '0') > 0 )
			num = newnum;
		else
		{ /* overflow */
			/* eat-up rest of digits */
			while ( CMdigit(++str) );
			{
				++cnt;
			}
			break;
		}
	}

	*term = str;
	*count = cnt;

	return num;
}

static f8
scan_float ( str, term )
register char	*str;
register char	**term;
{
	f8	mantissa,
		dpart;
	i4	zeros;
	i4	ipart;

	f8 quick10();

	/* leading integer part */
	ipart = scan_long(str,term);

	str = *term;
	/*
	** check for decimal point.  If there is one, get the integer part
	** following the decimal point, and count the actual characters
	** scanned to determine decimal places.
	*/
	if ( *str != '.' )
		zeros = 0;
	else
	{
		dpart = scan_slong(++str, term, &zeros);
		str = *term;
	}

	/*
	** calculate floating value, adjusting decimal part
	** by dividing by appropriate power of ten.
	*/
	mantissa = ipart;
	if ( zeros != 0 )
		mantissa += dpart/quick10(zeros);

	/*
	** if the terminator indicates exponential notation, get another
	** integer, and multiply by appropriate power of ten.
	*/
	if ( *str == 'e' || *str == 'E' )
	{
		zeros = scan_nat(++str, term);
		mantissa *= quick10(zeros);
	}

	return mantissa;
}

/* quick10 - fast table lookup routine for "reasonable" powers of 10 */

static f8 Tab10 [] =
{
	1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0
};

#define T10SIZE (sizeof(Tab10)/sizeof(f8))

static f8
quick10 ( n )
register i4	n;
{
	/*
	** if too big or small, let MHipow do the work.	 This also takes care
	** of worrying about what magnitude we can represent - that's the math
	** library's problem
	*/
	if ( n < -24 || n > 24 )
		return MHipow((f8) 10.0, (i4) n);

	/* if negative, take reciprical.  |n| small enough not to underflow */
	if ( n < 0 )
		return 1.0/quick10(-n);

	if ( n >= T10SIZE )
		return Tab10[T10SIZE-1] * quick10(n-T10SIZE+1);

	return Tab10[n];
}
#else
static f8
scan_float ( str, term )
char	*str;
char	**term;
{
	f8		res;
	register char	*start;
	char		hold;

	start = str;
	if (*str == '-' || *str == '+')
		++str;
	while ( CMwhite(str) || CMdigit(str) )
		CMnext(str);
	if (*str == '.')
	{
		while ( CMdigit(++str) )
			;
	}
	while ( CMwhite(str) )
		CMnext(str);
	if (*str == 'E' || *str == 'e')
	{
		++str;
		if (*str == '-' || *str == '+')
			++str;
		while ( CMdigit(str) )
			++str;
	}

	hold = *str;
	*term = str;
	*str = EOS;
	CVaf(start,'.',&res);
	*str = hold;

	return res;
}
#endif

/*
** scan string.	 This routine differs slightly from the other scan routines.
** it can fail, in which case it returns NULL, it's destructive, and it
** allocates storage for the parsed string.  This routine handles backslash
** interpretation for embedded double quotes and backslashes in the string,
** ie. anything following a backslash is literal.  If NULL is returned,
** endchar may be used to distinguish unterminated string from allocation
** failure.
**
** History:
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
**	22-dec-93 (rudyw)
**		Fix bug 55290. The EOS of a string being scanned must not
**		be treated as an escaped character. Situation can occur when
**		string is part of an incomplete string requiring next segment
**		add on, and '\' is the final character of the current segment.
*/

static char *
scan_str ( str, term, endchar )
char	*str;
char	**term;
char	*endchar;
{
	register char	*inp,
			*outp;

	/* find terminator, use outp to point to first backslash */
	outp = NULL;
	for ( inp = str ; *inp != '"' && *inp != EOS ; CMnext(inp) )
	{
		/* Process escape sequence, but avoid treating EOS as escaped */
		if (*inp == '\\' && *(inp+1) != EOS )
		{
			if (outp == NULL)
				outp = inp;
			++inp;	/* skip literal character */
		}
	}

	/* overwrite terminating ", and return pointer to next char */
	*endchar = *inp;
	if ( *inp == EOS )
		return NULL;
	*term = inp + 1;
	*inp = EOS;

	if (outp != NULL)
	{
		/*
		** we have backslashes to remove.  We could not have
		** done this in the earlier scan because we don't want
		** to destroy the input stream until we know we are not
		** returning NULL.
		*/
		inp = outp;
		do
		{
			if (*inp == '\\')
				++inp;
			CMcpyinc(inp, outp);
		} while ( *inp != EOS );
		*outp = EOS;
	}

	/* allocate string */
	return FEtsalloc(Tag, str);
}

/*
** scan_xs also allocates and can fail.	 It returns STATUS OK for success, FAIL
* for failure.  As with scan_str, a fail return may mean EOS or alloc failure,
** or in this case, bad syntax.	 Check the returned end character to see which.
*/
static STATUS
scan_xs ( str, term, endchar, tok )
char	*str;
char	**term;
char	*endchar;
TOKEN	*tok;
{
	register char	*inp;
	DB_TEXT_STRING	*hstr;
        register char	*outp;
	char		*tp;
	register i4	size;
	char		cbuf[3];

	/* get numeric size */
	size = scan_nat(str, &tp);
	inp = tp;

	/*
	** if we terminated on an EOS we return end of string.	If
	** terminator is NOT a single quote, we have bad syntax.
	** otherwise, skip to the first hexadecimal digit.
	*/
	if ( (*endchar = *inp++) != '\'' )
		return FAIL;

	/*
	** if we can't allocate a buffer for the hex string storage,
	** we'll return a failure
	*/

	if ( (hstr = (DB_TEXT_STRING *)FEreqmem((u_i4)Tag,
			(u_i4)size+DB_CNTSIZE,
			(bool)FALSE, (STATUS *)NULL)) == NULL)
	{
		return FAIL;
	}

	tok->dat.hexstr = hstr;
	hstr->db_t_count = size;
	outp = (char *) hstr->db_t_text;

	/*
	** convert pairs of hex digits.	 Yes, it does assume that
	** 0-9, A-Z, a-z are single byte chars.
	*/
	cbuf[2] = EOS;
	while ( *inp != '\'' && *inp != EOS )
	{
		i4	hexval;

		/*
		** silently truncate values beyond size of buffer
		*/
		if (size <= 0)
		{
			/* scan for EOS */
			while ( *++inp != EOS )
				;
			break;
		}
		--size;		/* one less character */

		/*
		** get hex value from next 2 characters.  If not hex
		** digits, return FAIL - endchar is nonzero at this point.
		*/

		if ( !CMhex(inp) )
			return FAIL;

		cbuf[0] = *inp++;	/* first hex digit */
		/* check for next digit */
		if ( *inp == EOS )
			break;	/* hex string spans record on odd digit */
		else if ( !CMhex(inp) )
			return FAIL;
		cbuf[1] = *inp++;

		CVahxl(cbuf,&hexval);

		/*
		** place in buffer and increment
		*/
		*outp++ = hexval;
	}

	/*
	** if we hit EOS, set endchar to EOS, return -1.
	*/
	if (*inp == EOS)
	{
		*endchar = EOS;
		return FAIL;
	}

	*term = inp+1;

	return OK;
}
