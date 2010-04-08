
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<si.h>
# include	<er.h>
# include	"ermo.h"
# include	"monitor.h"
# include	"maceight.h"
#include	<qr.h>
/*
NO_DBLOPTIM=hp8_us5
*/
/*
** Copyright (c) 2004 Ingres Corporation
**
**  MACRO PROCESSOR
**
**  History:
**  06/11/85 Changes for CMS.  CMS has 8-bit characters.  (gwb)
**	9/18/85 (peb)	Made corrections for quoted characters in the
**			multinational macro package. Also corrected
**			an integration problem introduced by IBM (can't
**			change my definition of CHARMASK.
**	03/05/87 (scl) change CMS ifdef's to EBCDIC
**	27-may-88 (bruceb)
**		Parenthesize arg of sizeof so compiles on DG.
**	03-aug-89 (wolf)
**		Change macprim() to macdoprim() to avoid conflict with
**		Macprims. (IBM porting change 9/30/89 - teresal)
**	14-jul-92 (sweeney)
**		switch off optimization for apollo - it produces
**		bogus code which makes the tm die while processing startup
**		with a SIGILL.
**	14-sep-93 (dianeh)
**		Removed NO_OPTIM setting for obsolete Apollo port (apl_us5).
**	29-oct-93 (swm)
**		Added comment about possible lint int/pointer truncation
**		warning for valid "suspect" code.
**		Replaced call to CVla with call to CVptra since a pointer
**		parameter was being truncated and converted to a string, also
**		removed the truncating cast to i4.
**      24-jul-95 (thoro01, chimi03)
**		Added the NO_DBLOPTIM comment for hp8_us5.
**		With optimization, the ? operator in the /branch operand of a
**		QUEL macro was not being recognized. When unoptimized, the
**		/branch operand was processed correctly.
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Added kchin's changes (from 6.4) for axp_osf
**		10/15/93 (kchin)
**		Cast 2nd argument to PTR when calling putprintf(), this
**		is due to the change of datatype of variable parameters
**		in putprintf() routine itself.	The change is made in
**		macdefine() and mactcvt().
**	19-jan-1998 (nicph02)
**		Fix for bug 83894. macgetch is called by the QR code regardless
**		of the query language type in order to read the query text.
**		Sometimes the query language is set to DB_SQL but the 
**		Do_macros flag is set : The SQL query is therefore read with
**		macro checking, leading to bug 83894.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      24-Jul-2009 (hanal04) Bug 122338
**          Update recursion detection code so that we are able to detect
**          the recursion in {define;table_name;uppercase(table_name)}.
**          The existing coude would see the 'u' of uppercase in the peek
**          buffer and decide that we had finished processing the initial
**          macro substitution.
**      29-Sep-2009 (frima01) 122490
**          Add return type for macinit.
*/


# define	ANYDELIM	'\020'		/* \| -- zero or more delims */
# define	ONEDELIM	'\021'		/* \^ -- exactly one delim */
# define	CHANGE		'\022'		/* \& -- token change */

# define	PARAMN		'\023'		/* $ -- non-preprocessed param */
# define	PARAMP		'\024'		/* $$ -- preprocessed param */

# define	PRESCANENABLE	'@'		/* character to enable prescan */
# define	LBRACE		'{'		/* left brace */
# define	RBRACE		'}'		/* right brace */
# define	BACKSLASH	'\\'		/* backslash */
# define	LQUOTE		'`'		/* left quote */
# define	RQUOTE		'\''		/* right quote */
# define	SPACE		' '
# define	TAB		'\t'
# define	NEWLINE		'\n'
# define	SLASH		'/'
# define	STAR		'*'
# define	LPAREN		'('
# define	RPAREN		')'

#ifdef EBCDIC
# define	CHARMASK	0377		/* character part */
#endif

# define	ITERTHRESH	100		/* iteration limit */
# define	NPRIMS		(sizeof Macprims / sizeof Macprims[0])

/* token modes, used to compute token changes */
# define	NONE		0		/* guarantees a token change */
# define	ID		1		/* identifier */
# define	NUMBER		2		/* number (i4 or f4) */
# define	DELIM		3		/* delimiter, guarantees a token change */
# define	QUOTEMODE	4		/* quoted construct */
# define	OP		5		/* operator */
# define	NOCHANGE	6		/* guarantees no token change */


char macgetch(void);


/* macro definitions */
struct macro
{
	struct macro	*nextm;		/* pointer to next macro header */
	char		*template;	/* pointer to macro template */
	char		*substitute;	/* pointer to substitution text */
};

/* primitive declarations */
GLOBALDEF struct macro	Macprims[] =
{
	&Macprims[1],	ERx("{define;\020\024t;\020\024s}"),
(char *) 1,
	&Macprims[2],	ERx("{rawdefine;\020\024t;\020\024s}"),
(char *) 2,
	&Macprims[3],	ERx("{remove;\020\024t}"),
(char *) 3,
	&Macprims[4],	ERx("{dump}"),						(char *) 4,
	&Macprims[5],	ERx("{type\020\024m}"),					(char *) 5,
	&Macprims[6],	ERx("{read\020\024m}"),					(char *) 6,
	&Macprims[7],	ERx("{readdefine;\020\024n;\020\024m}"),		(char *) 7,
	&Macprims[8],	ERx("{ifsame;\020\024a;\020\024b;\020\023t;\020\023f}"),(char *) 8,
	&Macprims[9],	ERx("{ifeq;\020\024a;\020\024b;\020\023t;\020\023f}"),	(char *) 9,
	&Macprims[10],	ERx("{ifgt;\020\024a;\020\024b;\020\023t;\020\023f}"),	(char *) 10,
	&Macprims[11],	ERx("{eval\020\024e}"),					(char *) 11,
	&Macprims[12],	ERx("{substr;\020\024f;\020\024t;\024s}"),		(char *) 12,
	&Macprims[13],	ERx("{dnl}"),						(char *) 13,
	&Macprims[14],	ERx("{remove}"),					(char *) 3,
	0,		ERx("{dump;\020\024n}"),				(char *) 4,
};

GLOBALDEF struct macro	*Machead	= &Macprims[0]; /* head of macro list */


/* parameters */
struct param
{
	struct param	*nextp;
	char		mode;
	char		name;
	char		*paramt;
};



/* the environment */
struct env
{
	struct env	*nexte;		/* next environment */
	i4		(*rawget)();	/* raw character get routine */
	char		**rawpar;	/* a parameter to that routine */
	i4		prevchar;	/* previous character read */
	char		tokenmode;	/* current token mode */
	char		change;		/* token change flag */
	char		eof;		/* eof flag */
	char		newline;	/* set if bol */
	char		rawnewline;	/* same for raw input */
	struct buf	*pbuf;		/* peek buffer */
	struct buf	*mbuf;		/* macro buffer */
	char		endtrap;	/* endtrap flag */
	char		pass;		/* pass flag */
	char		pdelim;		/* current parameter delimiter */
	struct param	*params;	/* parameter list */
	i4		itercount;	/* iteration count */
        i4		iterlogged;     /* iteration message logged */
        i4              s_total;        /* Number of macro characters */
	i4		quotelevel;	/* quote nesting level */
	i4		inquote;	/* double-quoted string indicator */
	i4		incomment;	/* comment indicator */
	i4		passnext;	/* TRUE if next character must be passed */
	i4		inmac;		/* TRUE if currently processing a macro */
};

/* current environment pointer */
GLOBALDEF struct env	*Macenv;


extern ADF_CB   *Tm_adfscb;


/*! BEGIN UNIX_INTR */
FUNC_EXTERN char	*bufcrunch();
/*! END UNIX_INTR */

/*
**  MACINIT -- initialize for macro processing
**
**	*** EXTERNAL INTERFACE ***
**
**	The macro processor is initialized.  Any crap left over from
**	previous processing (which will never occur normally, but may
**	happen on an interrupt, for instance) will be cleaned up.  The
**	raw input is defined.
**
**	This routine must always be called prior to any processing.
**
**	History:
**		12/10/84 (lichtman) -- don't spring {begintrap} if
**			macro processing is turned off
**		8/8/85 (peb) -- Added support for 8-bit character sets.
**		85.11.21 (bml) -- changed a few variable declarations
**			to globaldef, where appropriate.
**		8/86 (peterk) - took out endtrap parameter and code to
**			spring {begintrap}; {begintrap} and {endtrap}
**			now executed only in go().
*/

VOID
macinit(rawget, rawpar)
i4	(*rawget)();
char	**rawpar;
{
	static struct env	env;
	register struct env	*e;
	register struct env	*f;

	/* clear out old crap */
	for (e = Macenv; e != 0; e = f)
	{
		bufpurge(&e->mbuf);
		bufpurge(&e->pbuf);
		macpflush(e);
		f = e->nexte;
		if (f != 0)
			buffree((struct buf *) e);
	}

	/* set up the primary environment */
	Macenv = e = &env;
	MEfill(sizeof *e, '\0', (char *) e);

	e->rawget = rawget;
	e->rawpar = rawpar;
	e->newline = 1;

}




/*
**  MACGETCH -- get character after macro processing
**
**	*** EXTERNAL INTERFACE ROUTINE ***
**
**	The macro processor must have been previously initialized by a
**	call to macinit().
**
**	History:
**		12/10/84 (lichtman)	- added \nomacro command.  This turns off macro
**			processing, which makes it possible to get rid of much of the
**			overhead of th terminal monitor when macro processing isn't wanted.
**			The heart of this is at this function, which has been modified to
**			return characters without applying the macro algorithm.
**		25-oct-01 (inkdo01)
**			re-enable macros for SQL and fix left quote bug differently.
**	26-Aug-2009 (kschendel) b121804
**	    Declare as char since it's used as such by qr.
*/

char
macgetch(void)
{
	register struct env	*e;
	register i4		c;
	i4			inquote;
	i4			incomment;
	i4			passnext;
        i4			s_cnt;

	e = Macenv;

	/* if macro processing is turned off, then just return raw characters */
	if (!Do_macros)
		return (char)((*e->rawget)(e->rawpar));

	for (;;)
	{
		/* remember the current state in case we have to back up */
		inquote = e->inquote;
		incomment = e->incomment;
		passnext = e->passnext;
		/* get an input character */
		c = macgch();

                /* If a macro substitution took place s_total is > 0
                ** until we've consumed the substituted characters we
                ** can not rule out macro recursion.
                */
                if(e->s_total > 0)
                    e->s_total--;

		/* check for end-of-file processing */
		if (c == 0)
		{
			/* check to see if we should spring {endtrap} */
			if (e->endtrap)
			{
				e->endtrap = 0;
				macspring(ERx("{endtrap}"));
				continue;
			}

			/* don't spring endtrap -- real end of file */
			return ('\0');
		}

		/* not an end of file -- check for pass character through */
		if (e->pass)
		{
			e->pass = 0;
			e->change = 0;
		}
#ifdef EBCDIC
		if (!e->change || e->tokenmode == DELIM)
#else
		if ((c & QUOTED) != 0 || !e->change || e->tokenmode == DELIM)
#endif
		{
			/* the character is to be passed through */
			/* reset iteration count and purge macro buffer */
                        /* unless we are still processing characters from */
                        /* an earlier macro substitution */
                        if(e->s_total == 0)
                        {
			    e->itercount = 0;
                            e->iterlogged = 0;
                        }
			bufflush(&e->mbuf);
			e->newline = (c == NEWLINE);
			return (char)(c & BYTEMASK);
		}

		/* this character is a candidate for macro processing */
		macunget((i4)0);
		/* restore the previous state */
		e->inquote = inquote;
		e->incomment = incomment;
		e->passnext = passnext;
		/* we are now processing a macro */
		e->inmac = TRUE;
		bufflush(&e->mbuf);

		/* check for infinite loop */
		if (e->itercount > ITERTHRESH)
		{
                        if(e->iterlogged == FALSE)
                        {
			    putprintf(ERget(F_MO0001_Infiniteloop_in_macro));
                            e->iterlogged = TRUE;
                        }
			e->pass++;
			/* no longer processing a macro */
			e->inmac = FALSE;
			continue;
		}

		/* see if we have a macro match */
		if (s_cnt = macallscan())
		{
			/* yep -- count iterations and rescan it */
			e->itercount++;
                        e->s_total += s_cnt;
		}
		else
		{
			/* nope -- pass the next token through raw */
			e->pass++;
		}
		/* no longer processing a macro */
		e->inmac = FALSE;
	}
}


/*
**  MACGCH -- get input character, knowing about tokens
**
**	The next input character is returned.  In addition, the quote
**	level info is maintained and the QUOTED bit is set if the
**	returned character is (a) quoted or (b) backslash escaped.
**	As a side effect the change flag is maintained.	 Also, the
**	character is saved in mbuf.
**
**	History:
**		05/20/83 (lichtman) -- added stuff to prevent special processing
**			of characters inside double-quoted strings
**		14-jan-1991 (kathryn) -- Bug 21762
**			When character is RQUOTE (') and this ending-quote
**			puts quotelevel at 0 then return e-pdelim so that   
**			we will evaluate `a' to "a" and not "a'".
**		02-may 1992 (kathryn) Bug 40091
**			Do not put first LQUOTE or last RQUOTE in macro buffer.
*/

macgch()
{
	register i4		c;
	register struct env	*e;
/* unused variable	register i4		i; */
	register char		look;	/* look-ahead character */

	e = Macenv;

	for (;;)
	{
		/* get virtual raw character, save in mbuf, and set change */
		c = macfetch((i4)(e->quotelevel > 0));

		if (!e->inmac)
			if (e->passnext)	/* TRUE means pass this character */
			{
				e->pass = TRUE;
				e->passnext = FALSE;
				return(c);
			}

		if (e->incomment)
			e->pass = TRUE;

		/* test for magic frotz */
		switch (c)
		{
		  case 0:	/* end of file */
			return (0);

		  case QUOTE:
			/* always pass a double quote through as a raw character
			** unless it is in a comment
			*/
			if (!e->inmac)
			{
				e->pass = TRUE;
				if (!e->incomment)
				{
					e->inquote = !e->inquote;
					return (c);
				}
			}
			if (e->quotelevel > 0)
				break;
			return (c);

		  case SLASH:
			/* a slash is a normal character if it is in a
			** double-quoted string or a comment
			*/
			if (!e->inmac)
			{
				if (e->inquote || e->incomment)
				{
					if (e->quotelevel > 0)
						break;
					return (c);
				}
				/* we are not in a quoted string or a comment.
				** this may be the beginning of a comment.
				*/
#ifdef EBCDIC
				look = macfetch((i4)0);
#else
				look = macfetch((i4)1);
#endif
				if (look == STAR)
					e->incomment = TRUE;
				macunget((i4)1);
				/* if it was the beginning of a comment we want
				** to return both the / and the * as raw characters
				*/
				if (e->incomment)
				{
					e->pass = TRUE;
					e->passnext = TRUE;
					return (c);
				}
			}
			/* it was not the beginning of a comment.  treat
			** normally
			*/
			if (e->quotelevel > 0)
				break;
			return (c);

		  case STAR:
			/* if we are not in a double-quoted string or a
			** comment then a star is a normal character
			*/
			if (!e->inmac)
			{
				if (e->inquote || !e->incomment)
				{
					if (e->quotelevel > 0)
						break;
					return (c);
				}
				/* this may be the end of a comment
				*/
#ifdef EBCDIC
				look = macfetch((i4)0);
#else
				look = macfetch((i4)1);
#endif
				if (look == SLASH)
					e->incomment = FALSE;
				macunget((i4)1);
				/* if it was the end of a comment then we want to
				** return both the * and the / as raw characters
				*/
				if (!e->incomment)
				{
					e->pass = TRUE;
					e->passnext = TRUE;
					return(c);
				}
			}
			/* it was not the end of a comment.  treat normally.
			*/
			if (e->quotelevel > 0)
				break;
			return (c);

		  case LQUOTE:
			if (Tm_adfscb->adf_qlang == DB_SQL)
			    break;	/* lquote's aren't recognized in SQL */
			if (e->quotelevel++ == 0)
			    continue;
			break; 

		  case RQUOTE:
			if (e->quotelevel == 0)
			    break;
			else if (e->quotelevel > 0)
			    --e->quotelevel;
			if (e->quotelevel == 0)
			    continue;
			break;

		  case BACKSLASH:
			if (!e->inmac)
			{
				if (e->inquote)
				{
#ifdef EBCDIC
					if ((look = macfetch((i4)0)) == QUOTE)
#else
					if ((look = macfetch((i4)1)) == QUOTE)
#endif
						e->passnext = TRUE;
					if (look == BACKSLASH || (look == RQUOTE) || look == LQUOTE)
						e->passnext = TRUE;
					macunget((i4)1);
					/* always pass a \" or \\ escape sequence through as raw characters */
					e->pass = TRUE;
					return (c);
				}
			}
#ifdef EBCDIC
			c = macfetch((i4)0);
#else
			c = macfetch((i4)1);
#endif

			/* handle special cases */
			if (c == e->pdelim)
				break;

			/* do translations */
			switch (c)
			{
			  case SPACE:	/* space */
			  case TAB:	/* tab */
#ifndef EBCDIC
			  case NEWLINE: /* newline */
#endif
			  case RQUOTE: 
			  case LQUOTE: 
			  case '$':
			  case LBRACE:
			  case RBRACE:
			  case BACKSLASH:
				break;

#ifdef EBCDIC
  			  case NEWLINE: /* macro continuation, get next char */
  				c = m_bufget(&e->mbuf);	   /* remove newline from macro buffer */
  				c = m_bufget(&e->mbuf);	   /* and backslash */
  				c = m_bufget(&e->mbuf);	   /* and extra blank */
  				c = macfetch((i4)0);
  				break;
#endif
			  default:
				/* take character as is (unquoted) */
				c = 0;
				break;
			}

			if (c != 0)
				break;

			/* not an escapable character -- treat it normally */
			macunget((i4)1);
			c = BACKSLASH;
			/* do default character processing on backslash */

		  default:
			if (e->quotelevel > 0)
				break;
			return (c);
		}


#ifdef EBCDIC
		/* the character is not quoted in EBCDIC */
		return(c);
#else
		/* the character is quoted */
		return(c | QUOTED);
#endif
	}
}




/*
**  MACFETCH -- fetch virtual raw character
**
**	A character is fetched from the peek buffer.  If that buffer is
**	empty, it is fetched from the raw input.  The character is then
**	saved away, and the change flag is set accordingly.
**	The QUOTED bit on the character is set if the 'quote' flag
**	parameter is set; used for backslash escapes.
**	Note that the QUOTED bit appears only on the character which
**	goes into the macro buffer; the character returned is normal.
**
**	History:
**		05/23/83 (lichtman) -- added Macinquote test to prevent
**			backslash processing when inside double quotes
**		06/13/83 (lichtman) -- changed Macinquote & other state
**			variables to be part of env struct.  Added special
**			code to ignore quoted strings & comments when
**			processing macros
*/

macfetch(quote)
i4	quote;
{
	register struct env	*e;
	register i4		c;
	register i4		escapech;

	e = Macenv;
	escapech = 0;

	for (;;)
	{
		/* get character from peek buffer */
		c = m_bufget(&e->pbuf);

		if (c == 0)
		{
			/* peek buffer is empty */
			/* check for already raw eof */
			if (!e->eof)
			{
				/* note that c must be i4  so that the QUOTED bit is not negative */
				c = (*e->rawget)(e->rawpar);
				if (c <= 0)
				{
					c = 0;
					e->eof++;
				}
				else
				{
					if (e->rawnewline)
						e->prevchar = NEWLINE;
					e->rawnewline = (c == NEWLINE);
				}
			}
		}

		/* test for escapable character */
		if (escapech)
		{
			switch (c)
			{
			  case 't':	/* become quoted tab */
#ifdef EBCDIC
				c = TAB;
#else
				c = TAB | QUOTED;
#endif
				break;
			  case 'n':	/* become quoted newline */
#ifdef EBCDIC
				c = NEWLINE;
#else
				c = NEWLINE | QUOTED;
#endif
				break;

			  default:
				m_bufput(c, &e->pbuf);
				c = BACKSLASH;
			}
			escapech = 0;
		}
		else
		{
			if (c == BACKSLASH && !e->inquote)
			{
				escapech++;
				continue;
			}
		}
		break;
	}

	/* quote the character if appropriate to mask change flag */
	/* ('escapech' now becomes the maybe quoted character) */
	escapech = c;
#ifndef EBCDIC
	if (quote && c != 0)
		escapech |= QUOTED;
#endif

	/* set change flag */
	macschng(escapech);

	if (c != 0)
	{
		/* save the character in the macro buffer */
		m_bufput(escapech, &e->mbuf);
	}

	return (c);
}



/*
**  MACSCHNG -- set change flag and compute token type
**
**	The change flag and token type is set.	This does some tricky
**	stuff to determine just when a new token begins.  Most notably,
**	notice that quoted stuff IS scanned, but the change flag is
**	reset in a higher level routine so that quoted stuff looks
**	like a single token, but any begin/end quote causes a token
**	change.
**
**	History:
**		11/07/84 (lichtman)	-- made it detect a new token when an
**					OP character follows a '.'
*/

macschng(ch)
i4	ch;
{
	register struct env	*e;
	register i4		c;
	register i4		thismode;
	i4			changeflag;
	static i4		wasid = 0;

	e = Macenv;
	c = ch;
	changeflag = 0;
	thismode = macmode(c);

	switch (e->tokenmode)
	{
	  case NONE:
		/* always cause token change */
		wasid = 0;
		break;

	  case QUOTEMODE:
		/* change only on initial entry to quotes */
		break;

	  case DELIM:
		wasid = 0;
		changeflag++;
		break;

	  case ID:
		/* take any sequence of letters and numerals */
		if (thismode == NUMBER)
			thismode = ID;
		wasid = 1;
		break;

	  case NUMBER:
		/* take string of digits and decimal points */
		if (wasid == 1)
		{
			if ((thismode == NUMBER) || (thismode == ID))
				thismode = e->tokenmode = ID;
		}
		else if (c == '.')
			thismode = NUMBER;
		break;

	  case OP:
		wasid = 0;
		switch (e->prevchar)
		{
		  case '<':
		  case '>':
		  case '!':
			if (c != '=')
				changeflag++;
			break;

		  case '*':
			if (c != '*' && c != '/')
				changeflag++;
			break;

		  case '/':
			if (c != '*')
				changeflag++;
			break;

		  case '.':
			if (thismode == NUMBER)
				e->tokenmode = thismode;
			else
				changeflag++;
			break;

		  default:
			changeflag++;
			break;
		}
		break;

	  case NOCHANGE:	/* never cause token change */
		e->tokenmode = thismode;
		break;
	}

	e->prevchar = c;
	if (thismode != e->tokenmode)
		changeflag++;
	e->tokenmode = thismode;
	e->change = changeflag;
}




/*
**  MACMODE -- return mode of a character
*/

macmode(ch)
i4	ch;
{
	register i4	c;

	c = ch;

#ifndef EBCDIC
	if ((c & QUOTED) != 0)
		return (QUOTEMODE);
#endif
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'))
		return (ID);
	if (c >= '0' && c <= '9')
		return (NUMBER);
	if (c == SPACE || c == TAB || c == NEWLINE)
		return (DELIM);
	return (OP);
}



/*
**  MACALLSCAN -- scan to see if input matches a macro
**
**	Returns true if there was a match, false if not.  In any case,
**	the virtual raw input (i.e., the peek buffer) will contain
**	either the old raw input, or the substituted macro.
*/

i4
macallscan()
{
	register struct macro	*m;
        register i4		cnt;
 

	for (m = Machead; m != 0; m = m->nextm)
	{
		/* check to see if it matches this macro */
		if (macscan(m))
		{
			/* it does -- substituted value is in mbuf */
			cnt = macrescan();
			return (cnt);
		}

		/* it doesn't match this macro -- try the next one */
		(void) macrescan();
	}

	/* it doesn't match any of them -- tough luck */
	return (0);
}


/*
**  MACSCAN -- scan a single macro for a match
**
**	As is scans it also collects parameters for possible future
**	substitution.  If it finds a match, it takes responsibility
**	for doing the substitution.
*/

macscan(mac)
struct macro	*mac;
{
	register struct macro	*m;
	register char		c;
	register char		*temp;
	char			pname, pdelim;

	m = mac;

	/* check for anchored mode */
	temp = m->template;
	if (*temp == ONEDELIM)
	{
		if (!Macenv->newline)
			return (0);
		temp++;
	}

	/* scan the template */
	for ( ; c = *temp; temp++)
	{
		if (c == PARAMN || c == PARAMP)
		{
			/* we have a parameter */
			pname = *++temp;
			pdelim = *++temp;
			if (macparam(c, pname, pdelim))
			{
				/* parameter ok */
				continue;
			}

			/* failure on parameter scan */
			return (0);
		}

		if (!macmatch((i4)c & BYTEMASK))
		{
			/* failure on literal match */
			return (0);
		}
	}

	/* it matches!!	 substitute the macro */
	macsubs(m);
	return (1);
}



/*
**  MACPARAM -- collect a parameter
**
**	The parameter is collected and stored away "somewhere" with
**	name 'name'.  The delimiter is taken to be 'delim'.  'Mode'
**	tells whether to prescan the parameter (done immediately before
**	substitute time to avoid side effects if the macro actually
**	turns out to not match).
**  HISTORY:
**	06-apr-89 (kathryn)	Bug# 3596 and Bug# 3449
**		Check to determine if chars which are the same as the
**		delimiter may actually be part of the macro argument.
**		Characters which are the same as the delimiter, but which are
**		actually arguments to the macro will no longer be removed
**		by call to macmatch(). If a '(' is encountered in the macro
**		parameter then the same number of ')'s will be considered
**		part of the parameter. Anything in quotes will remain unchanged.
**	01-may-1992 (kathryn) Bug 40091
**		Check quotelevel before and after the call to macgch. When an
**		RQUOTE or LQUOTE is read by macgch, the quotelevel is changed 
**		and the char following it is returned.  If an ending RQUOTE was
**		read, then the chacter read should be put back, as we are at
**		the end of the parameter.
*/

macparam(mode, name, delim)
char	mode;
char	name;
char	delim;
{
	register i4		c;
	register struct env	*e;
	struct buf		*b;
	register struct param	*p;
	i4			bracecount;
	i4			parencount;
	bool			more;
	i4			qlevel;
	FUNC_EXTERN char	*bufalloc(),*bufcrunch();
	e = Macenv;
	b = 0;

	e->pdelim = delim;
	if (mode == PARAMP)
	{
		/* check for REALLY prescan */
		c = macgch();
		if (c != PRESCANENABLE)
		{
			mode = PARAMN;
			macunget((i4)0);
		}
	}

	bracecount = 0;
	parencount = 0;
	e->tokenmode = NOCHANGE;

	/* Bug # 3596 and #3449	*/

	if (( (delim == RPAREN) && (parencount > 0)) || (e->quotelevel > 0))
		more = TRUE;
	else if (!macmatch((i4)delim))
		more = TRUE;
	else
		more = FALSE;

	while (more)
	{
		do
		{
			qlevel = e->quotelevel;
			c = macgch();
			if (qlevel > e->quotelevel && e->quotelevel == 0)
			    macunget((i4)0);
			else
			{
			    m_bufput(c, &b);
			    if (c == 0 || c == NEWLINE)
			    {
				e->pdelim = 0;
				bufpurge(&b);
				return (0);
			    }
			}
			if (c == LBRACE)
				bracecount++;
			else if (c == RBRACE && bracecount > 0)
				bracecount--;

			/* Bug # 3596 and #3449	*/

			if (c == LPAREN)
				parencount++;
			else if ((c == RPAREN) && (parencount > 0))
				parencount--;
		} while (bracecount > 0);
		
		if ( ((delim == RPAREN) && (parencount > 0)) || 
		     (e->quotelevel > 0) )
			more = TRUE;
		else if (!macmatch((i4)delim))
			more = TRUE;
		else
			more = FALSE;
	}

	e->pdelim = 0;

	/* allocate and store the parameter */
	p = (struct param *) bufalloc((i4)sizeof(*p));
	p->mode = mode;
	p->name = name;
	p->nextp = e->params;
	e->params = p;
	p->paramt = bufcrunch(&b);
	bufpurge(&b);

	return (1);
}


/*
**  MACMATCH -- test for a match between template character and input.
**
**	The parameter is the character from the template to match on.
**	The input is read.  The template character may be a meta-
**	character.  In all cases if the match occurs the input is
**	thrown away; if no match occurs the input is left unchanged.
**
**	Return value is true for a match, false for no match.
*/

macmatch(template)
i4	template;
{
	i4	t;
	i4	c;
	u_char	t_char;
	u_char 	c_char;
	register i4	res;

	t = template;

	switch (t)
	{
	  case ANYDELIM:	/* match zero or more delimiters */
		/* chew and chuck delimiters */
		while (macdelim())
			;

		/* as a side effect, must match a token change */
		if (!macckch())
		{
			return (0);
		}
		return (1);

	  case ONEDELIM:	/* match exactly one delimiter */
		res = macdelim();
		return (res);

	  case CHANGE:		/* match a token change */
	  case 0:		/* end of template */
		res = macckch();
		return (res);

	  default:		/* must have exact character match */
		c = macgch();
		/*
		** Move the characters from an i4 var to a character
		** var so the address can be taken for CMcmpnocase.
		*/
		t_char = (u_char) t;
		c_char = (u_char) c;
		if (CMcmpnocase((char *) &t_char, (char *) &c_char) == 0)
		{
			return (1);
		}

		/* failure */
		macunget((i4)0);
		return (0);
	}
}



/*
**  MACDELIM -- test for next input character a delimiter
**
**	Returns true if the next input character is a delimiter, false
**	otherwise.  Delimiters are chewed.
*/

macdelim()
{
	register char	c;

	c = macgch();
	if (macmode(c) == DELIM)
	{
		return (1);
	}
	macunget((i4)0);
	return (0);
}


/*
**  MACCKCH -- check for token change
**
**	Returns true if a token change occurs between this and the next
**	character.  No characters are ever chewed, however, the token
**	change (if it exists) is always chewed.
*/

macckch()
{
	register i4		change;
/* set but not used variable	register char		c; */
	register struct env	*e;

	e = Macenv;
	if (e->tokenmode == NONE)
	{
		/* then last character has been ungotten: take old change */
		change = e->change;
	}
	else
	{
/* set but not used		c = */
		macgch();
		change = Macenv->change;
		macunget((i4)0);
	}

	/* chew the change and return */
	e->tokenmode = NOCHANGE;
	return (change);
}


/*
**  MACSUBS -- substitute in macro substitution
**
**	This routine prescans appropriate parameters and then either
**	loads the substitution into the macro buffer or calls the
**	correct primitive routine.
*/

macsubs(mac)
struct macro	*mac;
{
	register struct param	*p;
	register struct env	*e;
	register char		*s;
	char			*macdoprim();

	e = Macenv;

	for (p = e->params; p != 0; p = p->nextp)
	{
		/* check to see if we should prescan */
		if (p->mode != PARAMP)
		{
			continue;
		}

		/* prescan parameter */
		macprescan(&p->paramt);
		p->mode = PARAMN;
	}

	s = mac->substitute;

	/* clear out the macro call */
	bufflush(&e->mbuf);

	if (s <= (char *) NPRIMS)
	{
		/* it is a primitive */
		/* suspect code --- s is (char *) but macdoprim needs an i4 to SWITCH with */
		/* lint truncation warning if size of ptr > int, but code
		   valid since we know s <= NPRIMS */
		macload(macdoprim((i4)(SCALARP)s));
	}
	else
	{
		/* it is a user-defined macro */
		macload(s);
	}
}



/*
**  MACPRESCAN -- prescan a parameter
**
**	The parameter pointed to by 'pp' is fed once through the macro
**	processor and replaced with the new version.
*/

macprescan(pp)
char	**pp;
{
	struct buf		*b;
	char			*p;
/* set but not used variable	register struct env	*e; */
	register i4		c;
	FUNC_EXTERN i4		macsget();

	b = 0;
	p = *pp;

	/* set up a new environment */
	macnewev(macsget, &p);
/* set but not used	e = Macenv; */

	/* scan the parameter */
	while ((c = macgetch()) != 0)
		m_bufput(c, &b);

	/* free the old parameter */
	buffree((struct buf *) *pp);

	/* move in the new one */
	*pp = bufcrunch(&b);
	bufpurge(&b);

	/* restore the old environment */
	macpopev();
}



/*
**  MACNEWEV -- set up new environment
**
**	Parameters are raw get routine and parameter
*/

macnewev(rawget, rawpar)
i4	(*rawget)();
char	**rawpar;
{
	register struct env	*e;
	FUNC_EXTERN char		*bufalloc();

	e = (struct env *) bufalloc((i4)sizeof(*e));
	e->rawget = rawget;
	e->rawpar = rawpar;
	e->nexte = Macenv;
	e->newline = 1;
	Macenv = e;
}



/*
**  MACPOPEV -- pop an environment
**
**	Makes sure all buffers and stuff are purged
*/

macpopev()
{
	register struct env	*e;

	e = Macenv;
	bufpurge(&e->mbuf);
	bufpurge(&e->pbuf);
	macpflush(e);
	Macenv = e->nexte;
	buffree((struct buf *) e);
}



/*
**  MACPFLUSH -- flush all parameters
**
**	Used to deallocate all parameters in a given environment.
*/

macpflush(env)
struct env	*env;
{
	register struct env	*e;
	register struct param	*p;
	register struct param	*q;

	e = env;

	for (p = e->params; p != 0; p = q)
	{
		buffree((struct buf *) p->paramt);
		q = p->nextp;
		buffree((struct buf *) p);
	}

	e->params = 0;
}



/*
**  MACSGET -- get from string
**
**	Works like a getchar from a string.  Used by macprescan().
**	The parameter is a pointer to the string.
*/

i4
macsget(pp)
char	**pp;
{
	register char	**p;
	register i4	escape_char;
	register i4	c;

	p = pp;

	c = **p & BYTEMASK;
	if (c != 0)
		(*p)++;

	/*
	** Check if the character just returned is QUOTED. This
	** is done by looking at the next byte. If it's the quote
	** indicator then quote the character.
	*/
	escape_char = **p & BYTEMASK;
	if (escape_char == ESC_QUOTE)
	{
		(*p)++;
		c |= QUOTED;
	}
	return (c);
}




/*
**  MACLOAD -- load a string into the macro buffer
**
**	The parameters are a pointer to a string to be appended to
**	the macro buffer and a flag telling whether parameter substi-
**	tution can occur. (removed flag since it wasn't being used [Mike Berrow])
*/

macload(str)
char	*str;
{
	register struct env	*e;
	register char		*s;
	register i4		c;
	FUNC_EXTERN char		*macplkup();

	e = Macenv;
	s = str;

	if (s == 0)
		return;

	while ((c = *s++) != 0)
	{
		if (c == PARAMN)
			macload(macplkup(*s++));
		else
			m_bufput(c & BYTEMASK, &e->mbuf);
	}
}



/*
**  MACRESCAN -- rescan the macro buffer
**
**	Copies the macro buffer into the peek buffer so that it will be
**	reread.	 Also deallocates any parameters which may happen to be
**	stored away.
*/

i4
macrescan()
{
	register struct env	*e;
	register i4		c;
        register i4             cnt = 0;

	e = Macenv;

	while ((c = m_bufget(&e->mbuf) & BYTEMASK) != 0)
        {
		m_bufput(c, &e->pbuf);
                cnt++;
        }

	e->quotelevel = 0;
	e->tokenmode = NONE;
	macpflush(e);
        /* Return the number of characters we moved to the peek buffer */
        return(cnt);
}




/*
**  MACUNGET -- unget a character
**
**	Moves one character from the macro buffer to the peek buffer.
**	If 'mask' is set, the character has the quote bit stripped off.
*/

macunget(mask)
i4	mask;
{
	register struct env	*e;
	register i4		c;

	e = Macenv;

	if (e->prevchar != 0)
	{
		c = m_bufget(&e->mbuf);
		if (mask)
			 c &= BYTEMASK;
		m_bufput(c, &e->pbuf);
		e->tokenmode = NONE;
	}
}




/*
**  MACPLKUP -- look up parameter
**
**	Returns a pointer to the named parameter.  Returns null
**	if the parameter is not found ("cannot happen").
*/

char *
macplkup(name)
char	name;
{
	register struct param	*p;

	for (p = Macenv->params; p != 0; p = p->nextp)
	{
		if (p->name == name)
			return (p->paramt);
	}

	return (0);
}



/*
**  MACSPRING -- spring a trap
**
**	The named trap is sprung, in other words, if the named macro is
**	defined it is called, otherwise there is no replacement text.
*/

macspring(trap)
char	*trap;
{
	register struct env	*e;
	register char		*p;
	char			*macro();

	e = Macenv;

	bufflush(&e->mbuf);

	/* fetch the macro */
	p = macro(trap);

	/* if not defined, don't bother */
	if (p == 0)
		return;

	/* load the trap */
	macload(p);

	/* insert a newline after the trap */
	m_bufput('\n', &e->mbuf);

	(void) macrescan();
}



/*
**  MACPRIM -- do primitives
**
**	The parameter is the primitive to execute.
*/

char *
macdoprim(n)
i4	n;
{
	register struct env	*e;
	FUNC_EXTERN char		*macplkup();
	FUNC_EXTERN char		*macsstr();
	register char		*p;

	e = Macenv;

	switch (n)
	{
	  case 1:	/* {define; $t; $s} */
		macdnl();
		macdefine(macplkup('t'), macplkup('s'), (i4)0);
		break;

	  case 2:	/* {rawdefine; $t; $s} */
		macdnl();
		macdefine(macplkup('t'), macplkup('s'), (i4)1);
		break;

	  case 3:	/* {remove $t} */
		macdnl();
		macremove(macplkup('t'));
		break;

	  case 4:	/* {dump} */
			/* {dump; $n} */
		macdnl();
		macdump(macplkup('n'));
		break;

	  case 5:	/* {type $m} */
		macdnl();
		p = macplkup('m');
		putprintf(ERx("%s"), p);
		putprintf(ERx("\n"));
		break;

	  case 6:	/* {read $m} */
		p = macplkup('m');
		putprintf(ERx("%s "), p);
		macread();
		break;

	  case 7:	/* {read; $n; $m} */
		p = macplkup('m');
		putprintf(ERx("%s "), p);
		macread();
		macdefine(macplkup('n'), bufcrunch(&e->mbuf), (i4)1);
		return(ERx("{readcount}"));

	  case 8:	/* {ifsame; $a; $b; $t; $f} */
		if (!STcompare(macplkup('a'), macplkup('b')))
			return (macplkup('t'));
		else
			return (macplkup('f'));


	  case 9:	/* {ifeq; $a; $b; $t; $f} */
		if (macnumber(macplkup('a')) == macnumber(macplkup('b')))
			return (macplkup('t'));
		else
			return (macplkup('f'));

	  case 10:	/* {ifgt; $a; $b; $t; $f} */
		if (macnumber(macplkup('a')) > macnumber(macplkup('b')))
			return (macplkup('t'));
		else
			return (macplkup('f'));

	  case 12:	/* {substr; $f; $t; $s} */
		return (macsstr((i4)macnumber(macplkup('f')), (i4)macnumber(macplkup('t')), macplkup('s')));

	  case 13:	/* {dnl} */
		macdnl();
		break;

	  default:
		/* macro: bad primitive %d */
		ipanic(E_MO0052_1500800, n);
	}

	return (ERx(""));
}



/*
**  MACDNL -- delete to newline
**
**	Used in general after macro definitions to avoid embarrassing
**	newlines.  Just reads input until a newline character, and
**	then throws it away.
*/

macdnl()
{
	register char		c;
	register struct env	*e;

	e = Macenv;

	while ((c = macgch()) != 0 && c != NEWLINE)
		;

	bufflush(&e->mbuf);
}



/*
**  MACDEFINE -- define primitive
**
**	This function defines a macro.	The parameters are the
**	template, the substitution string, and a flag telling whether
**	this is a raw define or not.  Syntax checking is done.
*/

macdefine(template, subs, raw)
char	*template;
char	*subs;
i4	raw;
{
	register struct env	*e;
#ifdef EBCDIC
	char			paramdefined[256];
#else
	char			paramdefined[128];
#endif
	char			*p;
	register i4		c;
	struct buf		*b;
	register struct macro	*m;
	FUNC_EXTERN i4		macsget();
	FUNC_EXTERN char		*bufalloc(),*bufcrunch();
	char			*mactcvt();

	/* remove any old macro definition */
	macremove(template);

	/* get a new environment */
	macnewev(macsget, &p);
	b = 0;
	e = Macenv;

	/* undefine all parameters */
	MEfill(sizeof paramdefined, '\0', paramdefined);

	/* avoid an initial token change */
	e->tokenmode = NOCHANGE;
/* set but not used	escapech = 1; */

	/* allocate macro header and template */
	m = (struct macro *) bufalloc((i4)sizeof(*m));

	/* scan and convert template, collect available parameters */
	p = template;
	m->template = mactcvt((i4)raw, paramdefined);
	if (m->template == 0)
	{
		/* some sort of syntax error */
		buffree((struct buf *) m);
		macpopev();
		return;
	}

	bufflush(&e->mbuf);
	bufflush(&e->pbuf);
	e->eof = 0;

	/* scan substitute string */
	for (p = subs; c = macfetch((i4)0); )
	{
		if (c != '$')
		{
			/* substitute non-parameters literally */
			m_bufput(c & BYTEMASK, &b);
			continue;
		}

		/* it's a parameter */
		m_bufput(PARAMN, &b);
		c = macfetch((i4)0);

		/* check to see if name is supplied */
		if (paramdefined[c] == 0)
		{
			/* nope, it's not */
			putprintf(ERget(F_MO0002_undef_param_reference), (PTR)c);
			buffree((struct buf *) m->template);
			buffree((struct buf *) m);
			macpopev();
			bufpurge(&b);
			return;
		}
		m_bufput(c & BYTEMASK, &b);
	}

	/* allocate substitution string */
	m->substitute = bufcrunch(&b);

	/* allocate it as a macro */
	m->nextm = Machead;
	Machead = m;

	/* finished... */
	macpopev();
	bufpurge(&b);
}




/*
**  MACTCVT -- convert template to internal form
**
**	Converts the template from external form to internal form.
**
**	Parameters:
**	raw -- set if only raw type conversion should take place.
**	paramdefined -- a map of flags to determine declaration of
**		parameters, etc.  If zero, no parameters are allowed.
**
**	Return value:
**	A character pointer off into mystic space.
**
**	The characters of the template are read using macfetch, so
**	a new environment should be created which will arrange to
**	get this.
*/

char *
mactcvt(raw, paramdefined)
i4	raw;
#ifdef EBCDIC
char	paramdefined[256];
#else
char	paramdefined[128];
#endif
{
	register i4		c;
	struct buf		*b;
	register i4		d;
	register i4		escapech;
	char			*p;

	b = 0;
	escapech = 1;

	while (c = macfetch((i4)0))
	{
		switch (c)
		{
		  case '$':		/* parameter */
			if (escapech < 0)
			{
				putprintf(ERget(F_MO0003_no_delim_for_param));
				bufpurge(&b);
				return (0);
			}

			/* skip delimiters before parameter in non-raw */
			if (Macenv->change && !escapech && !raw)
				m_bufput(ANYDELIM, &b);

			escapech = 0;
			c = macfetch((i4)0);
			d = PARAMN;
			if (c == '$')
			{
				/* prescanned parameter */
				d = PARAMP;
				c = macfetch((i4)0);
			}

			/* process parameter name */
			if (c == 0)
			{
				/* no parameter name */
				putprintf(ERget(F_MO0004_null_param_name));
				bufpurge(&b);
				return (0);
			}

			m_bufput(d, &b);
			escapech = -1;

			/* check for legal parameter */
			if (paramdefined == 0)
				break;

			if (paramdefined[c])
			{
				putprintf(ERget(F_MO0005_parameter_redeclared), (PTR)c);
				bufpurge(&b);
				return (0);
			}
			paramdefined[c]++;

			/* get parameter delimiter */
			break;

		  case BACKSLASH:		/* a backslash escape */
			escapech = 1;
			c = macfetch((i4)0);
			switch (c)
			{
			  case '|':
				c = ANYDELIM;
				break;

			  case '^':
				c = ONEDELIM;
				break;

			  case '&':
				c = CHANGE;
				break;

			  default:
				escapech = 0;
				c = BACKSLASH;
				macunget((i4)0);
				break;
			}
			break;

		  case NEWLINE | QUOTED:
		  case TAB | QUOTED:
		  case SPACE | QUOTED:
#ifdef EBCDIC
		  case NEWLINE:
		  case TAB:
		  case SPACE:
#endif
			if (escapech < 0)
				c &= BYTEMASK;
			escapech = 1;
			break;

		  default:
			/* change delimiters to ANYDELIM */
			if (macmode(c) == DELIM && !raw)
			{
				while (macmode(c = macfetch((i4)0)) == DELIM)
					;
				macunget((i4)0);
				if (c == 0)
					c = ONEDELIM;
				else
					c = ANYDELIM;
				escapech = 1;
			}
			else
			{
				if (Macenv->change && !escapech)
				{
					m_bufput(ANYDELIM, &b);
				}

				if (escapech < 0)
				{
					/* parameter: don't allow quoted delimiters */
					c &= BYTEMASK;
				}
				escapech = 0;
			}
			break;
		}
		m_bufput(c, &b);
	}
	if (escapech <= 0)
		m_bufput(CHANGE, &b);

	p = bufcrunch(&b);
	bufpurge(&b);
	return (p);
}




/*
**  MACREMOVE -- remove macro
**
**	The named macro is looked up.  If it is found it is removed
**	from the macro list.
**
**	History:
**		10/11/84 (lichtman)	-- fixed bug #4245: {remove}\g caused a syserr.
**					The problem was that buffree was being called on
**					cname even when cname was never set to point to
**					a buffer.  This is the case when name is zero,
**					which happens when no macro is named in the
**					{remove} call.
*/

macremove(name)
char	*name;
{
	register struct macro	*m;
	register struct macro	**mp;
	FUNC_EXTERN i4		macsget();
	char			*p;
	register char		*cname;
	struct macro		*macmlkup();

	if (name != 0)
	{
		/* convert name to internal format */
		macnewev(macsget, &p);
		p = name;
		cname = mactcvt((i4)0, (char *)NULL);
		macpopev();
		if (cname == 0)
		{
			/* some sort of syntax error */
			return;
		}
	}

	/* find macro */
	while (name == 0 ? ((m = Machead)->substitute > (char *) NPRIMS) : ((m = macmlkup(cname)) != 0))
	{
		/* remove macro from list */
		mp = &Machead;

		/* find it's parent */
		while (*mp != m)
			mp = &(*mp)->nextm;

		/* remove macro from list */
		*mp = m->nextm;
		buffree((struct buf *) m->template);
		buffree((struct buf *) m->substitute);
		buffree((struct buf *) m);
	}

	if (name != 0)
		buffree((struct buf *) cname);
}




/*
**  MACMLKUP -- look up macro
**
**	The named macro is looked up and a pointer to the macro header
**	is returned.  Zero is returned if the macro is not found.
**	The name must be in internal form.
*/

struct macro *
macmlkup(name)
char	*name;
{
	register struct macro	*m;
	register char		*n;

	n = name;

	/* scan the macro list for it */
	for (m = Machead; m != 0; m = m->nextm)
	{
		if (macmmatch(n, m->template, (i4)0))
			return (m);
	}
	return (0);
}




/*
**  MACMMATCH -- check for macro name match
**
**	The given 'name' and 'temp' are compared for equality.	If they
**	match true is returned, else false.
**	Both must be converted to internal format before the call is
**	given.
**
**	"Match" is defined as two macros which might scan as equal.
**
**	'Flag' is set to indicate that the macros must match exactly,
**	that is, neither may have any parameters and must end with both
**	at end-of-template.  This mode is used for getting traps and
**	such.
*/

macmmatch(name, temp, flag)
char	*name;
char	*temp;
i4	flag;
{
	register char	ac;
	register char	bc;
	char		*ap, *bp;

	ap = name;
	bp = temp;

	/* scan character by character */
	for (;; ap++, bp++)
	{
		ac = *ap;
		bc = *bp;

		if (bc == ANYDELIM)
		{
			if (macmmchew(&ap))
				continue;
		}
		else
		{
			switch (ac)
			{
			  case SPACE:
			  case NEWLINE:
			  case TAB:
				if (ac == bc || bc == ONEDELIM)
					continue;
				break;

			  case ONEDELIM:
				if (ac == bc || macmode(bc) == DELIM)
					continue;
				break;

			  case ANYDELIM:
				if (macmmchew(&bp))
					continue;
				break;

			  case PARAMP:
			  case PARAMN:
			  case 0:
				if (bc == PARAMN || bc == PARAMP || bc == 0 ||
				    bc == ANYDELIM || bc == ONEDELIM ||
				    bc == CHANGE || macmode(bc) == DELIM)
				{
					/* success */
					if (!flag)
						return (1);
					if (ac == 0 && bc == 0)
						return (1);
				}
				break;

			  default:
				if (ac == bc)
					continue;
				break;
			}
		}

		/* failure */
		return (0);
	}
}




/*
**  MACMMCHEW -- chew nonspecific match characters
**
**	The pointer passed as parameter is scanned so as to skip over
**	delimiters and pseudocharacters.
**	At least one character must match.
*/

macmmchew(pp)
char	**pp;
{
	register char	*p;
	register char	c;
	register i4	matchflag;

	p = *pp;

	for (matchflag = 0; ; matchflag++)
	{
		c = *p;
		if (c != ANYDELIM && c != ONEDELIM && c != CHANGE &&
		    macmode(c) != DELIM)
			break;
		p++;
	}

	p--;
	if (matchflag == 0)
		return (0);
	*pp = p;
	return (1);
}




/*
**  MACREAD -- read a terminal input line
**
**	Reads one line from the user.  Returns the line into mbuf,
**	and a count of the number of characters read into the macro
**	"{readcount}" (-1 for end of file).
*/

macread()
{
	register struct env	*e;
	register i4		count;
	register char		c;
	char			outbuf[30];	/* This should be long enough for any long */
	FILE			*save_inp;	/* PLace to save Input */
	extern FILE		*Input;
	e = Macenv;
	count = -1;

	/*
	** Since getch() has the ability to handle international terminals,
	** we use that to read to the end-of-line for macread.
	** Note that getch uses the global Input to determine where it is
	** currently reading. If this is currently a file, we must reset
	** to stdin and restore its original value after user input is complete.
	*/

	save_inp = Input;
	Input = stdin;

	while ((c = getch()) != 0)
	{
		count++;
		if (c == NEWLINE)
			break;
		m_bufput((i4)c, &e->mbuf);
	}

	Input = save_inp;

	CVla(count, outbuf);
	macdefine(ERx("{readcount}"), outbuf, (i4)1);
}



/*
**  MACNUMBER -- return converted number
**
**	This procedure is essentially identical to the system atoi
**	routine, in that it does no syntax checking whatsoever.
*/

macnumber(s)
char	*s;
{
	register char		*p;
	register char		c;
	register i4		result;
	i4			minus;

	result = 0;
	p = s;
	minus = 0;

	while ((c = *p++) == SPACE)
		;

	if (c == '-')
	{
		minus++;
		while ((c = *p++) == SPACE)
			;
	}

	while (c >= '0' && c <= '9')
	{
		result = result * 10 + (c - '0');
		c = *p++;
	}

	if (minus)
		result = -result;

	return (result);
}




/*
**  MACSUBSTR -- substring primitive
**
**	The substring of 'string' from 'from' to 'to' is extracted.
**	A pointer to the result is returned.  Note that macsstr
**	in the general case modifies 'string' in place.
*/

char *
macsstr(from, to, string)
i4	from;
i4	to;
char	*string;
{
	register i4	f;
	i4		l;
	register char	*s;
	register i4	t;

	s = string;
	t = to;
	f = from;

	if (f < 1)
		f = 1;

	if (f > t)
		return (ERx(""));
	l = STlength(s);
	if (t < l)
		s[t] = 0;
	return (&s[f - 1]);
}



/*
**  MACDUMP -- dump a macro definition to the terminal
**
**	All macros matching 'name' are output to the buffer.  If
**	'name' is the null pointer, all macros are printed.
*/

macdump(name)
char	*name;
{
	register struct macro	*m;
	register char		*p;
	register char		*n;
	FUNC_EXTERN i4		macsget();
	FUNC_EXTERN char		*macmocv();
	char			*ptr;

	n = name;
	if (n != 0)
	{
		macnewev(macsget, &ptr);
		ptr = n;
		n = mactcvt((i4)0, (char *)NULL);
		macpopev();
		if (n == 0)
			return;
	}

	for (m = Machead; m != 0; m = m->nextm)
	{
		if (n == 0 || macmmatch(n, m->template, (i4)0))
		{
			if (m->substitute <= (char *) NPRIMS)
				continue;
			p = macmocv(m->template);
			macload(ERx("`{rawdefine; "));
			macload(p);
			macload(ERx("; "));
			p = macmocv(m->substitute);
			macload(p);
			macload(ERx("}'\n"));
		}
	}
	if (n != 0)
		buffree((struct buf *) n);
}



/*
**  MACMOCV -- macro output conversion
**
**	This routine converts the internal format of the named macro
**	to an unambigous external representation.
**
**	Note that much work can be done to this routine to make it
**	produce cleaner output, for example, translate "\|" to " "
**	in most cases.
*/

char *
macmocv(m)
char	*m;
{
	register char	*p;
	struct buf	*b;
	register i4	c;
	register i4	pc;
	static char	*lastbuf;
	FUNC_EXTERN char	*bufcrunch();

	p = m;

	/* release last used buffer (as appropriate) */
	if (lastbuf != 0)
	{
		buffree((struct buf *) lastbuf);
		lastbuf = 0;
	}

	if (p <= (char *) NPRIMS)
	{
		/* we have a primitive */
		p = ERget(F_MO0006_Primitive_xxx);
		CVptra(m, &p[10]);
		return (p);
	}

	b = 0;
	pc	=	*p;
	for (; (c = *p++) != 0; pc = c)
	{
		switch (c)
		{
		  case BACKSLASH:
		  case '|':
		  case '&':
		  case '^':
			break;

		  case ANYDELIM:
			c = ((i4) '\\' << 8) | '|';
			break;

		  case ONEDELIM:
			c = ((i4) '\\' << 8) | '^';
			break;

		  case CHANGE:
			c = ((i4) '\\' << 8) | '&';
			break;

		  case PARAMN:
			c = '$';
			break;

		  case PARAMP:
			c = ((i4) '$' << 8) | '$';
			break;

		  case '$':
			c = ((i4) '\\' << 8) | '$';
			break;

		  case NEWLINE:
			c = ('\\' | QUOTED) | ('\n' << 8);
			break;

		  default:
			m_bufput(c, &b);
			continue;
		}

		if (pc == BACKSLASH)
			m_bufput(pc, &b);
		pc = (c >> 8) & BYTEMASK;
		m_bufput(pc, &b);
		pc = c & BYTEMASK;
		if (pc != 0)
		{
			c = pc;
			m_bufput((char)c, &b);
		}
	}

	p = bufcrunch(&b);
	bufpurge(&b);
	lastbuf = p;
	return (p);
}




/*
**  MACRO -- get macro substitution value
**
**	***  EXTERNAL INTERFACE	 ***
**
**	This routine handles the rather specialized case of looking
**	up a macro and returning the substitution value.  The name
**	must match EXACTLY, character for character.
**
**	The null pointer is returned if the macro is not defined.
**
**	History:
**		12/10/84 (lichtman)	-- temporarily turn on macro processing
**			if it is disabled.
*/

char *
macro(name)
char	*name;
{
	register struct macro	*m;
	register char		*n;
	FUNC_EXTERN i4		macsget();
	char			*p;
	i4			savedomac;

	/* temporarily turn on macro processing */
	savedomac = Do_macros;
	Do_macros = TRUE;

	/* convert macro name to internal format */
	macnewev(macsget, &p);
	p = name;
	n = mactcvt((i4)0, (char *)NULL);
	macpopev();
	if (n == 0)
	{
		/* some sort of syntax error */
		Do_macros = savedomac;
		return (0);
	}

	for (m = Machead; m != 0; m = m->nextm)
	{
		if (macmmatch(n, m->template, (i4)1))
		{
			buffree((struct buf *) n);
			Do_macros = savedomac;
			return (m->substitute);
		}
	}

	buffree((struct buf *) n);
	Do_macros = savedomac;
	return (0);
}
