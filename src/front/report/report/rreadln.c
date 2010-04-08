/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <rcons.h>
# include	 <si.h>
# include	<cm.h>
# include	<errw.h>

/*
**   R_READLN - read in one text line from the referenced file.
**	If more than a specified number of chars are read before
**	finding an endofline, return only that many.
**
**	Parameters:
**		fd - file descriptor of file to read.
**		line - character string to hold line.
**		maxchar - maximum length of read.
**
**	Returns:
**		number of characters read.
**
**	Error Messages:
**		Error exit may be taken.
**
**	Trace Flags:
**		2.0, 2,13..
**
**	History:
**		6/1/81 (ps) - written.
**		4/7/83 (mmm)- did compat conversion.
**		07-apr-1987 (yamamoto)
**			Modified for double byte characters.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-feb-2009 (gupsh01)
**	    Fix the reading of multibyte characters from a data
**	    file, as multibyte strings can be > 2 bytes.
*/


i4
r_readln(fp,line,maxchar)
FILE		*fp;
register char	*line;
i2		maxchar;
{
	/* external declarations */

	FUNC_EXTERN	STATUS	SIread();	/* read chars from file */

	/* internal declarations */

	register char	*c;			/* ptr to line */
	i4		count;			/* number of chars read */
	STATUS		status;			/* (mmm) added */
	i4		iter, bcnt;

	/* start of routine */

	SIflush(stdout);	/* GAC -- added to flush prompt */

	c = line;
	while (maxchar > 0)
	{
		status = SIread(fp, sizeof(char), &count, c);
		
		if ( (bcnt = CMbytecnt(c)) > 1 )
		{
		   for (iter = 1; iter < bcnt; iter++)
		   { 
		     if ( (status = SIread(fp, 
				      sizeof(char), &count, c + iter) ) != OK )
			  break;
		   } 
		}

		CMbytedec(maxchar, c);

		switch(status)
		{
			case(FAIL): 
			case(ENDFILE):
				/* end of file */
				CMnext(c);
				*c = '\0';
				return(0);

			case(OK):
				/* standard char */
				if (*c == '\n')
				{
					if (c == line)
					{
						*c++ = ' ';	/* add blank at start */
					}
					*c = '\0';


					return(STlength(line));
				}
				CMnext(c);
				break;

			default:
				/* error condition */
				SIprintf(ERget(E_RW0044_read_returns_error));
				return(-1);
		}
	}

	/* too many chars read */

	*c = '\0';
	return(STlength(line));
}
