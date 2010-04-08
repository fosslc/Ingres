/*
** 	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<rcons.h>

#ifdef IBM370
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>
#endif

# include	<er.h>
# include	<errw.h>

# ifndef PCINGRES
# ifndef SEINGRES
# ifndef FT3270
# define ITXLATE
# endif
# endif
# endif

/*{
** Name:	r_wrt() -	Report Writer Output Text Buffer.
**
** Description:
**   R_WRITE -	write out the text string to the referenced file
**		without an end of line.
**
**		This routine SHOULD NOT be called if the line is EMPTY!
**
** Parameters:
**	fp -	file pointer of file to write to.
**	line -	Pointer to a character buffer to write.  This buffer is not
**		required nor expected to be a "C" string, i.e., any EOS is
**		considered to be data.  This pointer should never be NULL.
**	len -	number of valid characters in the character buffer to be
**		written.  This should always be greater than zero.
**
** Returns:
**	-1 if bad write, in some way or other.
**
** Error Messages:
**	none.
**
** Trace Flags:
**	2.0, 2.13.
**
** History:
**	7/20/81 (ps) - written.
**	4/11/83 (mmm)- now use file pointers instead of file descriptors
**	4/86 (jhw) -- Conditional compilation for International Translation.
**      07/03/86 (a.dea) -- In CMS-only code: move declaration of 'dmy'
**		to outer scope, since someone added another usage of it.
**		Also removed meaningless "break" (not in loop).
**	08/25/87 (scl) Change CMS to IBM- use ASA carriage control for both
**	08-aug-89 (sylviap)
**		Added check of SIwrite return status.
**	1/3/90 (elein) Changed IBM to IBM370, porting change
**	8/27/90 (elein) VM Porting changes 
**      	08/25/87 (scl) Change CMS to IBM; use ASA carriage control for
**			       both
**      	05/02/90 (wolf) Test single byte char for '\n' too; sequence of
**              		inputs could be '\f', NULL, and '\n' (bug 21159)
**	15-Apr-93 (fredb)
**		Add code to handle NULL or empty line passed in.  Without
**		this new code, a NULL line pointer gets passed to STlength
**		and we codetrap when Stlength tries to dereference it.
**	19-may-1993 (rdrane)
**		Reparameterize to accept length of line as a parameter.
**		STlength() fails when q0 sequences are present which contain
**		embedded ASCII NUL's.  This obviates the need to determine the
**		length in this module, and so obsoletes fredb's 15-apr-1993
**		change.  Note that lines containing only single-byte q0
**		sequences could be misinterpreted as ANSI carriage control on
**		IBM370.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4
r_wrt(fp,line,len)
FILE	*fp;
register char	*line;
register i4	len;
{
	i4		total;

	/* start of routine */

# ifdef IBM370
	/*
	**     Provide support for ANSI carriage control under IBM
	**
	**     This code assumes that any '\f's and '\r's are coming
	**     through as null-terminated character strings of length
	**     1.  r_set_dflt() may send an initial '\f'; r_do_page()
	**     sends them too.  The '\r' comes after a string of
	**     characters which was *not* terminated with a newline.
	**     (The \r is intended to be in lieu of a real newline.)
	*/
	if (!St_to_term && St_ff_on)
	{
		i4 	dmy;
		static char lastchar = '\n';
		static char cc = ' ';

		if (len == 1)
		{	/* single byte; might be a control character */
			if (*line == '\f')
			{
			/*
			**	If the caller is asking for a form feed then
			**	just remember it for the next line and return.
			*/
				if (cc != '1')
				   cc = '1';
				else
				{

				   if (SIwrite(2, ERx("1\n"), &dmy, fp) != OK)
					return (-1);
				   lastchar = '\n';
				}
				return 1;
			}
			else if (*line == '\r')
			{
			/*
			**	  If the caller is trying to ask for a carriage
			**	  return without a line feed just turn it into
			**	  a newline; but remember it so we can tell the
			**	  next line not to skip forward.
			*/
				cc = '+';
				*line = '\n';
			}
                        else if (*line == '\n')
                        {
                        /*
                        **      If preceded by a \f, emit the carriage
                        **      control char. The '\n' itself is emitted
                        **      by the following WHILE.  Fix for 21159.
                        */
                                if (cc == '1')
                                {
                                        if(SIwrite(1, ERx("1"), &dmy, fp) != OK)
						return(-1);
                                        cc = ' ';
                        	}
                	}
		}
		else if (len != 0)
		{
		/*
		**    Not a control character, so write a 'blank' at the
		**    beginning of the line for carriage control ('skip
		**    1 line')
		*/
			if (lastchar=='\n')
			{
				if (SIwrite(1, &cc, &dmy, fp) != OK)
					return (-1);
				cc = ' ';
				lastchar = ' ';
			}
		}
		/*
		**	Remember the last character that was written to the file
		*/
		if (len > 0)
			lastchar = line[len-1];
	}
# endif		/* IBM370 */
	total = 0;
	while (len > 0)
	{
		i4		count;		/* length of line written */
		register i4	prlen;
		register char	*lp;
		char		buf[256];

		prlen = min(len, sizeof(buf) - 1);
		len -= prlen;
		lp = line;
		line += prlen;
# ifdef ITXLATE
		prlen = ITxout(lp, buf, prlen);
		lp = buf;
# endif
		count = 0;
		if (SIwrite(prlen, lp, &count, fp) != OK)
			return (-1);
		total += count;
	}

	return total;
}
