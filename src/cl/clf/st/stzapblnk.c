/*
**Copyright (c) 2004 Ingres Corporation
**
**	STzapblank.c - remove trailing whitespace from a string.
**
**	History:
**		2-15-83   - Written (jen)
**		10-87 (bobm) - converted to use CM
**				integrated 5.0 - correct arg types
**		11-may-89 (daveb)
**			Now we are i4  instead of u_i4.  Also fix
**			comments which descibed the wrong function...
**		30-nov-92 (pearl)
**			Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped 
**			function headers.
**		08-feb-93 (pearl)
**			Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      21-apr-1999 (hanch04)
**          replace i4 with size_t
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

size_t
STzapblank_DB(
	register char	*string,
	register char	*buffer)
{
	register size_t	len = 0;
	register size_t	clen;

	for (;;)
	{
		while (CMwhite(string))
			CMnext(string);
		if (*string == EOS)
			break;
		CMcpychar(string,buffer);
		buffer += (clen = CMbytecnt(buffer));
		string += clen;
		len += clen;
	}
	*buffer = EOS;
	return (len);
}


size_t
STzapblank_SB(
	register char	*string,
	register char	*buffer)
{
	register size_t	len = 0;
	register size_t	clen;

	for (;;)
	{
		while (CMwhite_SB(string))
			CMnext_SB(string);
		if (*string == EOS)
			break;
		CMcpychar_SB(string,buffer);
		buffer += (clen = CMbytecnt_SB(buffer));
		string += clen;
		len += clen;
	}
	*buffer = EOS;
	return (len);
}
