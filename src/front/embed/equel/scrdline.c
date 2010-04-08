# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<cm.h>
# include	<ereq.h>

/*
+* Filename:	SCGETCH.C
** Purpose:	I/O character manipulation routines.
**
** Defines:	sc_reset()		- Reset file I/O buffers.
** 		sc_readline()		- Get next line from the input file.
**		sc_dump()		- Dump the scanner state.
** Locals:	
-*		sc_linebuf		- Input line buffer.
**
** History:	26-nov-1984	Rewritten. (ncg)
**		19-may-1987	Rewritten for Kanji. (mrw)
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	08-oct-96 (mcgem01)
**	    Global data moved to eqdata.c
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* 
** Standard input line - each line read is added to buffer. 
** If lex_need_input is set then a new line must be read.
** 2 extra bytes are needed for reading lines that are over the limit
** and need an extra newline.
*/

GLOBALREF char sc_linebuf[];
GLOBALREF char	*SC_PTR;
GLOBALREF i4	lex_need_input;		/* Do we need to read in a new line? */
GLOBALREF i4	lex_is_newline;		/* Do we need to process the line? */

/* How long is the sc_dump output line? */
# define SCRD_LINECNT	78

/* 
+* Procedure:	sc_reset 
** Purpose:	Reset I/O character buffers and pointers.
**
** Parameters:	None
-* Returns:	None
**
** Notes:
**	When compiling more than one file on a command line the buffer pointer
**	may have garbage from the old file, especially if the preprocessing of
**	the old file terminated abnormally.  This routine cleans out the local
**	state.
**	This is also used by SQL grammars to let the scanner know that the
**	grammar ate a newline.  Used in INCLUDE processing: the scanner saw
**	"include" and passed up an include line; it needs to know that the
**	line has been processed so that it will read a new line when it is
**	next called.
**
** Imports modified:
**	Sets lex_need_input.
*/

void
sc_reset()
{
    lex_need_input = 1;
}

/*{
+*  Name: sc_readline - Read a new input line into a static buffer.
**
**  Description:
**	Read a new line from the input file and terminate it with either
**	a newline or EOF, followed by a nul byte.
**
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    EscLINELEN		- Line too long.
**	    EscEOFMIDLINE	- EOF in the line.
-*
**  Side Effects:
**	Prompts if reading from the terminal.
**	Writes line to listing file if desired.
**	Fills local buffer.
**	Reset SC_PTR.
**	Increment line count.
**	
**  History:
**	19-may-1987 - Written (mrw)
**      8-Sep-2008 (gupsh01)
**          Fix usage of CMdbl1st for UTF8 and multibyte character sets
**          which can can be upto four bytes long. Use CMbytecnt to ensure
**          we do not assume that a multbyte string can only be a maximum
**          of two bytes long. (Bug 120865)
*/

void
sc_readline()
{
    register char	*cp;
    register i4	chint, iter, byte_cnt;

  /* prompt if reading from standard input (not include file) and flagged */
    if ((eq->eq_flags & EQ_STDIN) && (eq->eq_infile == stdin))
	SIprintf( ERx("%d> "), eq->eq_line+1 );	/* We increment linenum later */

  /* read in a line */
    cp = sc_linebuf;
    do
    {
	chint = SIgetc( eq->eq_infile );
	if (chint == EOF)
	    chint = SC_EOFCH;
	*cp = (char) chint;
	if (CMdbl1st(cp))
	{
	    byte_cnt = CMbytecnt(cp);

	    for (iter = 1; iter < byte_cnt; iter++)
	      *(cp+iter) = (char)SIgetc(eq->eq_infile);
	}
	CMnext(cp);
	if (chint == '\n' || chint == SC_EOFCH)
	    break;
    } while (cp < &sc_linebuf[SC_LINEMAX]);

  /* Dropped out if end of line, end of file or over the limit */
    if (cp >= &sc_linebuf[SC_LINEMAX] && chint != '\n' && chint != SC_EOFCH)
    {
      /* Truncate till end of line */
	er_write( E_EQ0212_scLINLONG, EQ_ERROR, 1, er_na(SC_LINEMAX) );
	do 
	{
	    /*
	    ** Toss characters until end-of-line.
	    ** Be sure to handle Kanji chars correctly;
	    ** don't examine the second byte.
	    */
	    if ((chint=SIgetc(eq->eq_infile)) == EOF)
		chint = SC_EOFCH;
	    else
	    {
		char	inttoch;

		inttoch = (char)chint;
		if (CMdbl1st(&inttoch))
		{
		  /* Toss the rest of the bytes in the
		  ** multibyte sequence
		  */ 
	    	  for (iter = CMbytecnt(&inttoch); iter > 1 ; iter--)
		    _VOID_ SIgetc(eq->eq_infile);
		}
	    }
	} while (chint != '\n' && chint != SC_EOFCH);
	*cp++ = chint;		/* Newline or Eof */
    }
    /*
    ** Line ends with "\n\0" or "EOF\0".
    ** We decrement cp w/o regard to double-byte chars because we know
    ** the the previous chars is single-byte (\n or EOF).
    */
    *cp-- = '\0';

  /* If listing then print line */
    if (eq->eq_flags & EQ_LIST)
    {
	static	FILE	*orig_o = NULL;
	static	FILE	*orig_l = NULL;

	/*
	** Don't list in an "out-of-line" inclusion.
	** In this case the output file has changed but the listing file
	** has not.  In an inline include neither has changed, and if we
	** moved to the next file in the command-line list both of them
	** will have changed.  The first file in the list looks like a
	** "new" one to this code.
	*/

	/* For each user's input file reset list and output files */
	if (orig_l != eq->eq_listfile)
	{
	    orig_o = eq->eq_outfile;
	    orig_l = eq->eq_listfile;
	}
	/* 
	** As long as we're writing to the original output file we can only
	** be down the list of main files, or in a direct 'inline' inclusion.
	*/
	if (eq->eq_outfile == orig_o)
	    SIfprintf( eq->eq_listfile, ERx("%s"), sc_linebuf );
    }

    SC_PTR = sc_linebuf;			/* Newly filled line */
    if (*SC_PTR != SC_EOFCH)
	eq->eq_line++;				/* Count non-empty lines */

    if (chint == SC_EOFCH)
    {
	if (eq->eq_flags & EQ_STDIN && eq->eq_infile == stdin)
	    SIprintf( ERx("\n") );
	if (cp != sc_linebuf)
	{
	    er_write( E_EQ0205_scEOFMIDLINE, EQ_ERROR, 0 );
	    sc_linebuf[0] = SC_EOFCH;	/* Ignore data */
	    sc_linebuf[1] = '\0';
	}
    }
}

/*{
+*  Name: scan_dump - Dump the scanner internal data structures.
**
**  Description:
**	dml->dm_exec host the HOST/DECL/EXEC SQL|FRS info for ESQL, or the
**	HOST/QUEL info for EQUEL.  Dump it, whether we need to read a new
**	line, have just read a new line, or are in the middle of a line,
**	and the line itself.
**
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	None.
**  History:
**	22-may-1987 - Written (mrw)
**	20-nov-1992 - Add extra logic for DML__4GL mode
**      8-Sep-2008 (gupsh01)
**	    Fix usage of CMbytecnt == 2 for UTF8 and multibyte character sets
**	    which can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
*/

void
sc_dump()
{
    register FILE	*df = eq->eq_dumpfile;
    register char	*cp;			/* Character counter */
    register i4	charcnt;		/* Counter for new lines */
    i4			e_index = 0;		/* Exec type index */
    i4			m_index = 0;		/* Exec modifier index */
    i4			iter, byte_cnt;
    static char		*execs[] = { ERx("HOST"), ERx("DECL"), ERx("EXEC"), 
				     ERx("QUEL") };
    static char		*modes[] = { ERx(""), 
				     ERx(" SQL"), 
				     ERx(" FRS"), 
				     ERx(" SQL FRS?"), 
				     ERx(" 4GL"), 
				     ERx(" SQL 4GL?"), 
				     ERx(" FRS 4GL?"),
				     ERx(" SQL FRS 4GL?")};

    switch (dml->dm_exec & ~(DML__SQL|DML__FRS|DML__4GL))	
						/* Mask out modifiers */
    {
      case DML_HOST:
	e_index = 0;
	break;
      case DML_DECL:
	e_index = 1;
	break;
      case DML_EXEC:
	e_index = 2;
	break;
      case DML_QUEL:
	e_index = 3;
	break;
    }

    if (dml->dm_exec & DML__SQL)
	m_index |= 1;
    if (dml->dm_exec & DML__FRS)
	m_index |= 2;
    if (dml->dm_exec & DML__4GL)
	m_index |= 4;

    SIfprintf( df, ERx("SCAN_DUMP:\n") );
    SIfprintf( df, ERx("%s%s: lex_need_input = %d, lex_is_newline = %d\n"),
	execs[e_index], modes[m_index], lex_need_input, lex_is_newline );

    SIfprintf( df, ERx("Current line: delimited by '<>', ") );
    SIfprintf( df, ERx("character pointer preceded by '@'.\n   <") );

    charcnt = 1;
    for (cp=sc_linebuf; *cp; CMnext(cp))
    {
	if (cp == SC_PTR)
	{
	    charcnt++;
	    SIputc( '@', df );
	}
	if (CMcntrl(cp))
	{
	    SIfprintf( df, CMunctrl(cp) );
	    charcnt += 2;	/* ^ and a letter are added */
	} else 
	{
	    byte_cnt = CMbytecnt(cp);
	    charcnt += byte_cnt; 
	    for (iter = 0; iter < byte_cnt; iter++)
		SIputc( *(cp+iter), df );
	}
	if (charcnt >= SCRD_LINECNT)
	{
	    SIfprintf( df, ERx("\n    ") );
	    charcnt = 1;
	}
    }
    SIfprintf( df, ERx(">\n") );
    SIflush( df );
}
