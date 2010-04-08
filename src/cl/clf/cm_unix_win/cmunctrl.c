/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	 <compat.h>
# include	 <gl.h>
# include	 <st.h>
# include	 <cm.h>

/*{
**  Name: CMunctrl	- Return a string representation of a contrl character.
**
**  Description:
**	Returns a pointer to a string representing the character that
**	was passed to it.  This routine is most useful when one wants to output
**	a control character to a user's terminal without strange side effects.
**	If the character is not a contrl character, the return string only
**	contains the character.
**
**	This uses a static buffer for the return string, so it CANNOT be
**	used in a server environment.
**
**	Also, please note that the ASCII and EBCDIC versions will be different.
**	An implementation is free to pick an appropriate representation of
**	control characters for the system.  For example, on ASCII systems
**	control characters in the range 0-037 and 0177 (octal) become their
**	upper-case equivalents preceded by a ^ character.  On VMS systems
**	the control characters above 0177 (octal) become the strings VMS
**	displays them as.  Those that have no VMS display are returned
**	as <XN> where N is their hex value.
**
**  Inputs:
**	str	pointer to the next character in a sting.
**
**  Outputs:
**	Returns:
**		the static result buffer
**
**  Side Effects:
**	None
**
**  History:
**		Written		(drh)
**		Modified 6/29/85 (jrc) Added strings for DEC control
**				       characters.
**		30-sep-1986 (yamamoto)
**			Copy from CHunctrl and change CH macros to CM.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p.
**	15-may-97 (mcgem01)
**	    Clean up compiler warning.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static	char	_CMunctrl[]	= 	/* unctrl codes for ttys */
{
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
	'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '~', '_',
};

static char	*_CMunstr[] = 	/* For characters above 0200 */
{
	"<X80>", "<X81>", "<X82>", "<IND>", "<NEL>", "<SSA>",
	"<ESA>", "<HTS>", "<HTJ>", "<VTS>", "<VTS>", "<PLD>",
	"<PLU>", "<RI>",  "<SS2>", "<SS3>", "<DCS>", "<PU1>",
	"<PU2>", "<STS>", "<CCH>", "<MW>",  "<SPA>", "<EPA>",
	"<X98>", "<X99>", "<X9A>", "<CSI>", "<ST>",  "<OSC>",
	"<PM>",  "<APC>"
};

char *
CMunctrl(str)
char	*str;
{
	static char	string[6];
	register char	*cp = string;
	i4		val;

	if (CMcntrl(str))
	{
		*cp++ = '^';
		val = *str & 0377;

		if (*str == '\177')
		{
			*cp++ = '?';
		}
		else if (val > 0200)
		{
			if (val < 0240)
				STcopy(_CMunstr[*str], string);
			else
				STprintf(string, "<X%p>", *str);
			return string;
		}
		else
		{
			*cp++ = _CMunctrl[*str];
		}
	}
	else
	{
		CMcpychar(str, cp);
		CMnext(cp);
	}
	*cp = '\0';

	return(string);
}
