/*
**  lex.c
**
**  lexical analyzer for validation routines.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**
**  7/19/85 - (dkh) Integrated International Support.
**
**  12/03/85 -	(cgd) yylex() should return i4  instead of i2
**	04/04/87 (dkh) - Added support for ADTs.
**  10/18/86 (KY)  -- Changed CH.h to CM.h.
**	05/05/87 (dkh) - Fixed to accept '_' as a valid ALPHA char.
**	06/19/87 (dkh) - Added "<>" as a NOT EQUAL operator.
**	07/11/87 (dkh) - Fixed jup bug 383.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	09/05/87 (dkh) - Changes for 8000 series error numbers.
**	11/19/87 (dkh) - Fixed jup bug 1365.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	06-jul-88 (sylviap)
**		Added call to vl_nopar_error().
**	17-may-89 (sylviap)
**		Changed parameter list to vl_nopar_error.
**	06/10/89 (dkh) - Fixed bug 6148.
**	06/23/89 (dkh) - Added support for derived fields.  Normal yacc
**			 variable names, etc. changed to allow two
**			 parsers in forms system.
**	89/08  wong
**	Modified 'vl_strcnv()' to call 'IIAFpmEncode()' instead of redundant
**	'afe_txtconst()' and non-operative 'afe_vchconst()'.
**	02/06/90 (dkh) - Increases size of static buffer "sbuf" in yylex().
**			 Buffer was too small for some of the token names
**			 and caused problems on decstation 3100 that allocates
**			 static variable and runtime dynamic memory from
**			 the same address space.
**	27-sep-90 (bruceb)	Fix for bug 33530.
**		Changed routines to use (u_char *) instead of (u_char) since
**		CM routines need to sometimes look at 2nd byte of the char.
**      24-feb-91 (stevet)      Fix for bug 33530 as well.
**              Add CMdbl1st() call to complete the fix.
**	06/11/92 (dkh) - Changed the code to convert floating numbers to
**			 DB_DEC_TYPE (when possible) to support the DECIMAL
**			 datatype for 6.5.
**	09/25/92 (dkh) - Fixed support for owner.table.
**	10/28/92 (dkh) - Changed ALPHA to VL_ALPHA for compiling on DEC/ALPHA.
**	3-12-93 (fraser)
**	    Changed "errno" to "errnum" because "errno" is reserved in ANSI C
**	    (and conflicts with Microsoft library).  Added return types to
**	    static functions vl_err, errcall, and vlbaderr and provided
**	    declarations for these functions.   
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	11-may-94 (kirke/mikehan)
**	    Removed extra ptr++ from double byte handling in vl_string().
**      23-par-96 (chech02)
**          fixed compiler complant for windows 3.1 port.
**	18-nov-1996(angusm)
**	    bug 79824: Change scanner IIFVvlex() to allow decimal point 
**	    to be whatever user specified via II_4GL_DECIMAL.
**	20-march-1997(angusm)
**	    previous change introduced a small error for comma-separated
**	    lists of numbers e.g. [1,2,3] which would erroneously be 
**	    interpreted as decimals (bug 80718).
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	10-Nov-1999 (kitch01)
**		During work on bug 98227 I noticed that the change for bug 79824
**		would no longer work under certain circumstances. This is 
**		because of the fix for bug b92987 in getdata.c. Small change to
**		restore the setting of II_APP_DECIMAL in the Adf_cb structure
**		to ensure that the parser uses this value as intended by the 
**		fix for 79824.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h>
# include	<afe.h>
# include	<cm.h>
# include	<ex.h>
# include	<me.h>
# include	<st.h>
# include	<frserrno.h>
# include	<er.h>
# include	"erfv.h"


GLOBALREF	IIFVvstype IIFVvlval;
GLOBALREF	bool	vl_syntax;
GLOBALREF	char	IIFVfloat_str[];

FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	STATUS	IIAFpmEncode();
FUNC_EXTERN	STATUS	afe_vchconst();

FUNC_EXTERN	i4	vl_string();
FUNC_EXTERN	char	*vl_delim();

typedef		u_char	u_chr;

static	PTR	decdata = NULL;

static	i4	scan_delim = FALSE;
static 	VOID	vl_err();
static	VOID	errcall();
static	VOID	vlbadchr();


/*
**  Next several routines are new or have been changed for
**  fix to BUG 7562. (dkh)
*/

static VOID
vl_err(errnum, ptr)
ER_MSGID	errnum;
char		*ptr;
{
	vl_nopar_error(errnum, 3, vl_curfrm->frname, 
		vl_curhdr->fhdname, ptr);
}

static VOID
errcall(errnum)
ER_MSGID	errnum;
{
	vl_err(errnum, Lastok);
}


static VOID
vlbadchr(ch)
char	*ch;
{
	ER_MSGID	errnum;
	char		buf[40];

	if (CMcntrl(ch))
	{
		errnum = VALCHAR;
		STcopy(CMunctrl(ch), buf);
	}
	else
	{
		errnum = VALUCHR;
		buf[1] = EOS;
		buf[2] = EOS;
		CMcpychar(ch, buf);
	}
	vl_err(errnum, buf);
}


/*
**  v_chtype
**
**  Routine to return the type of the character passed to it
*/

char
vl_chtype(ch)
u_char	*ch;
{
	char	retval;

	if (CMnmstart(ch) || *ch == '_')
	{
		retval = VL_ALPHA;
	}
	else if (CMdigit(ch))
	{
		retval = NUMBR;
	}
	else
	{
		switch(*ch)
		{
			case '\t':
			case '\n':
			case '\r':
			case ' ':
				retval = PUNCT;
				break;

			case '(':
			case ')':
			case '[':
			case ']':
			case '.':
			case ',':
			case '-':
			case '\"':
			case '\'':
			case '=':
			case '<':
			case '>':
			case '!':
				retval = OPATR;
				break;

			default:
				retval = CNTRL;
				break;
		}
	}
	return(retval);
}



/*
**	LEX.c  -  Input/Output Routines for the lexical analyzer
**
**	These routines do the I/O for the lexical analyzer of the
**	validation checks.
**	These routines do a one character input of the data.
**
**	History: 1 Aug 1981  -	modified from the parser (JEN)
**
*/
# define	GETCHAR()	IIFVgcGetChar()
# define	BACKUP()	(vl_buffer--)


u_chr	*
IIFVgcGetChar()
{
	return((u_char *) (vl_buffer++));
}


/*
** YYLEX
** This is the control program for the scanner (lexical analyzer).
** Each call to yylex() returns a token for the next syntactic unit.
** If the object is of type I4CONST, F8CONST, SCONST or NAME,
** that object will also be entered in the symbol table, indexed by
** 'IIFVvlval'. If the object is not one of these types, IIFVvlval is the
** opcode field of the v_operator or keyword tables.
** The end-of-file token is zero.
*/
static	ADF_CB	*Adf_cb;
static	char	decimal;

i4
IIFVvlex()
{
	register u_chr	*chr;
	register char	*ptr;
	register i2	rtval;
	register struct special *op;
	register i4	ttype;
static	f8		ftemp;
static	i4		ltemp;
static	DB_DATA_VALUE	decdbv;
static	char		sbuf[fdVSTRLEN + 1];
	DB_DATA_VALUE	dmydbv;
	char		buf[256];
	i4		is_exponential;
	i4		digit_count;
	i4		scale_digits;
	DB_TEXT_STRING	*textstr;
	AFE_DCL_TXT_MACRO(fdVSTRLEN)	dectext;
	i4	status;
	char	cp[2];

	if (Adf_cb == NULL)
	{
		Adf_cb = FEadfcb();
		/* Overwrite II_DECIMAL setting with II_4GL_DECIMAL */
		cp[0]=cp[1]='\0';
		status = IIUGlocal_decimal(Adf_cb, cp);
		if (status)
		{
			vl_nopar_error(E_FI2270_8816_BadDecimal, 1, cp);
			return(status);
		}
		decimal = Adf_cb->adf_decimal.db_decimal;
	}
	/* Ensure we use the decimal decided by the previous logic */
	Adf_cb->adf_decimal.db_decimal = decimal;

	if (decdata == NULL)
	{
		decdata = MEreqmem((u_i4) 0, (u_i4) DB_MAX_DECLEN,
			FALSE, (STATUS *) NULL);
	}

	rtval = -1;
	Lastok = ERx("");
	op = &Tokens;
	/* GET NEXT TOKEN */
	do
	{
		chr = GETCHAR();
		if (*chr == EOS)
		{
			rtval = 0;
			break;
		}

		switch(vl_chtype(chr))
		{
		case VL_ALPHA:
			/* fill in the name */
			ptr = buf;
			*ptr = *chr;
			do
			{
				/*
				**  to ignore a 2-nd byte character of a
				**  Double Bytes character
				*/
				if (CMdbl1st(ptr))
				{
					chr = GETCHAR();
					*(++ptr) = *chr;
				}
				chr = GETCHAR();
				*(++ptr) = *chr;
				if ((ptr - buf) > FE_MAXNAME)
				{
					/* name too long */
					errcall(VALNAME);
				}

/* Bug fix #33530 */
			}  while (CMnmchar(chr) || CMdbl1st(ptr));
			BACKUP();
			*ptr = EOS;

			rtval = op->name;
			switch (buf[0])
			{
			  case 'A':
			  case 'a':
				if (STbcompare((char *) buf, 0, ERx("and"), 0,
					TRUE) == 0)
				{
					rtval = op->lbop;
					ttype = v_opAND;
				}
				break;

			  case 'I':
			  case 'i':
				if (STbcompare((char *) buf, 0, ERx("in"),
					0, TRUE) == 0)
				{
					rtval = op->in;
					ttype = 0;
				}
				else if (STbcompare((char *) buf, 0, ERx("is"),
					0, TRUE) == 0)
				{
					rtval = op->is;
					ttype = 0;
				}
				break;

			  case 'L':
			  case 'l':
				if (STbcompare((char *) buf, 0, ERx("like"), 0,
					TRUE) == 0)
				{
					rtval = op->like;
					ttype = v_opLK;
				}
				break;

			  case 'N':
			  case 'n':
				if (STbcompare((char *) buf, 0, ERx("not"), 0,
					TRUE) == 0)
				{
					rtval = op->not;
					ttype = v_opNOT;
				}
				else if (STbcompare((char *) buf, 0,
					ERx("null"), 0, TRUE) == 0)
				{
					rtval = op->null;
					ttype = v_opNULL;
				}
				break;

			  case 'O':
			  case 'o':
				if (STbcompare((char *) buf, 0, ERx("or"),
					0, TRUE) == 0)
				{
					rtval = op->lbop;
					ttype = v_opOR;
				}
				break;
			}

			if (rtval != op->name)
			{
				STcopy((char *) buf, (char *) sbuf);
				Lastok = sbuf;
				IIFVvlval.type_type = ttype;
			}
			else
			{	/* else, USER DEFINED NAME */
				Lastok = IIFVvlval.name_type =
					STalloc((char *) buf);
			}
			break;

		case NUMBR:
			number:;
			is_exponential = FALSE;
			ptr = buf;
/*
compare to decimal
*/
			if ((*ptr = *chr) != decimal)
			{
                                /*
                                **  We already have one digit otherwise
                                **  we would not be here.
                                */
				digit_count = 1;

				do
				{
					/*
					**  Keep track of the number of
					**  digits that we find.
					*/
					digit_count++;
					chr = GETCHAR();
					*++ptr = *chr;
				} while (vl_chtype(chr) == NUMBR);

				/*
				**  Subtract one since the above loop
				**  always counts ahead.
				*/
				digit_count--;
			}
			else
			{
				/*
				**  No digits yet since we are here due
				**  to a goto from the OPATR case having
  				**  found a decimal point  (.  or ,).  
				**  This will probably
				**  be a floating point number starting with
				**  a decimal point.
				*/
				digit_count = 0;
			}

			/* do rest of type determination */
			switch (*ptr)
			{
			case '.':
			case ',':
				/* check if this is decimal or not (b80718)*/
				if (*ptr != decimal)
				{
					goto conv2;
				}
				/* floating point */
				scale_digits = 0;
				do
				{
					/* fill into ptr with up to next non-digit */
					digit_count++;
					scale_digits++;
					chr = GETCHAR();
					*++ptr = *chr;
				} while (vl_chtype(chr) == NUMBR);

				digit_count--;
				scale_digits--;

				if (*ptr != 'e' && *ptr != 'E')
				{
					BACKUP();
					*ptr = EOS;
					goto convr;
				}

			case 'e':
			case 'E':
				is_exponential = TRUE;
				chr = GETCHAR();
				*++ptr = *chr;
				if (vl_chtype(chr) == NUMBR || *ptr == '-' || *ptr == '+')
				{
					do
					{
						/* get exponent */
						chr = GETCHAR();
						*++ptr = *chr;
					} while (vl_chtype(chr) == NUMBR);
				}
				BACKUP();
				*ptr = EOS;
			    convr:
				if (is_exponential ||
					digit_count > DB_MAX_DECPREC)
				{
					/*
					** Use decimal point 
					** since this is not user input,
					** but a specification.
					*/
					if (CVaf(buf, decimal, &ftemp))
					{
						/* floating conversion error */
						errcall(VALCONV);
					}
					IIFVvlval.F8_type = ftemp;
					Lastok = (char *) buf;
					rtval = Tokens.f8const;
				}
				else
				{
					/*
					**  Convert to a decimal datatype to
					**  retain precision since the number
					**  of digits found for the constant
					**  is less than DB_MAX_DEC_PREC and
					**  it was not in exponential format.
					*/
					dmydbv.db_datatype = DB_LTXT_TYPE;
					dmydbv.db_length = sizeof(dectext);
					dmydbv.db_prec = 0;
					textstr = (DB_TEXT_STRING *) &dectext;
					textstr->db_t_count = digit_count + 1;
					MEcopy((PTR) buf,
						(u_i2) textstr->db_t_count,
						(PTR) textstr->db_t_text);
					dmydbv.db_data = (PTR) textstr;
					decdbv.db_datatype = DB_DEC_TYPE;
					decdbv.db_length =
					  DB_PREC_TO_LEN_MACRO(digit_count);
					decdbv.db_prec =
						DB_PS_ENCODE_MACRO(digit_count,scale_digits);
					decdbv.db_data = decdata;
					if (afe_cvinto(Adf_cb, &dmydbv,
						&decdbv) != OK)
					{
						/*
						**  Decimal conversion error.
						*/
						errcall(VALCONV);
					}
					STcopy(buf, IIFVfloat_str);
					IIFVvlval.dec_type = &decdbv;
					Lastok = (char *) buf;
					rtval = Tokens.decconst;
				}
				break;

			conv2:
			default:
				/* integer */
				BACKUP();
				*ptr = EOS;
				/* numeric conversion error */
				if (CVal(buf, &ltemp))
				{
					/*
					**  Set digit_count to a value
					**  that will force a floating
					**  point conversion for proper
					**  overflow handling.
					*/
					digit_count = DB_MAX_DECPREC + 1;
					scale_digits = 0;
					is_exponential = FALSE;
					goto convr;
				}
				IIFVvlval.I4_type = ltemp;
				Lastok = (char *) buf;
				rtval = Tokens.i4const;
				break;
			}
			break;

		case OPATR:
			/* get lookahead characer */
			buf[1] = '\0';
			buf[2] = '\0';
			CMcpychar(chr, buf);

			ttype = 0;
			rtval = op->relop;
			switch (*chr)
			{
			  case '(':
				rtval = op->lparen;
				break;

			  case ')':
				rtval = op->rparen;
				break;

			  case '[':
				rtval = op->lbrak;
				break;

			  case ']':
				rtval = op->rbrak;
				break;

			  case '.':
			  case ',':
			  {
				char	*nextchr;
/* combine , and . */
				if (*chr == '.')
					rtval = op->period;
				else
					rtval = op->comma;

				nextchr = (char *)GETCHAR();
				BACKUP();
				if ((*chr == decimal) && 
					(vl_chtype(nextchr) == NUMBR))
					goto number;
				break;
			  }
			  case '-':
				rtval = op->uminus;
				ttype = v_opMINUS;
				break;

			  case '\"':
				if (scan_delim)
				{
					/*
					**  If looking for delimited ids,
					**  just return them as names to
					**  simplify life in the grammar.
					*/
					Lastok = IIFVvlval.name_type =
						vl_delim();
					rtval = op->name;
				}
				else
				{
					rtval = vl_string();
				}
				break;

			  case '\'':
				rtval = vl_vchstr();
				break;

			  case '=':
				ttype = v_opEQ;
				break;

			  case '<':
				ttype = v_opLT;
				chr = GETCHAR();
				if (*chr == '=')
				{
					ttype = v_opLE;
					buf[1] = '=';
				}
				else if (*chr == '>')
				{
					ttype = v_opNE;
					buf[1] = '>';
				}
				else
				{
					BACKUP();
				}
				break;

			  case '>':
				chr = GETCHAR();
				if (*chr == '=')
				{
					ttype = v_opGE;
					buf[1] = '=';
				}
				else
				{
					ttype = v_opGT;
					BACKUP();
				}
				break;

			  case '!':
				chr = GETCHAR();
				if (*chr == '=')
				{
					ttype = v_opNE;
					buf[1] = '=';
					break;
				}
				/* else invalid input, fall through */

			  default:
				/* invalid v_operator */
				errcall(VALOPER);
				rtval = -1;
				break;
			}
			if (rtval == -1 || rtval == op->sconst ||
				rtval == op->svconst || rtval == op->name)
			{
				break;
			}

			STcopy(buf, sbuf);
			Lastok = sbuf;
			IIFVvlval.type_type = ttype;
			break;

		case PUNCT:
			continue;

		case CNTRL:
			/* already converted v_number ? */
			/*
			**  Fix for BUG 7562. (dkh)
			*/
			vlbadchr(chr);
			continue;

		default:
			errcall(VALTYPE);
		}
	}  while (rtval == -1);
	if (rtval == 0)
	{
		Lastok = ERx("");
	}
	return (rtval);
}

/*
** STRING
** A string is defined as any sequence of MAXSTRING or fewer chars,
** surrounded by string delimiters.  New-line ;characters are purged
** from strings unless preceeded by a '\'; QUOTE's must be similarly
** prefixed in order to be correctly inserted within a string.  Each
** string is entered in the symbol table, indexed by 'IIFVvlval'.  A
** token or the error condition -1 is returned.
**
** For 6.0, the validation string has been increased to 255.  Which
** still allows the code below to work.
*/

i4
vl_string()
{
	register i4	esc;
	register char	*ptr;
	char		buf[MAXSTRING + 1];
	u_i2		len;
	/* disable case conversion and fill in v_string */
	ptr = buf;
	do
	{
		/* get next character */
		if ((*ptr = *GETCHAR()) == EOS)
		{
			/* non term v_string */
			errcall(VALSTRNG);
			break;
		}

		/* Skip the second byte of a double byte character. */
		if (CMdbl1st(ptr))
		{
			if ((*++ptr = *GETCHAR()) == EOS)
			{
				/*
				**  This should never happen but we do
				**  it just in case.
				*/
				errcall(VALSTRNG);
				break;
			}

			/*
			**  If we are dealing with a double byte character,
			**  we can skip the checks in the rest of the
			**  loop since they will never be true.  So we
			**  just do a continue to speed things up.
			*/
			continue;
		}
		/* handle escape characters */
		esc = (*ptr == '\\');
		if (esc == 1)
		{
			if ((*++ptr = *GETCHAR()) == EOS)
			{
				*ptr = EOS;
				/* non term v_string */
				errcall(VALSTRNG);
				break;
			}
			if (*ptr == '\"')
				*--ptr = '\"';
		}

		if (CMcntrl(ptr))
		{
			/* cntrl in v_string from equel */
			if (*ptr != '\t' && *ptr != '\r' && *ptr != '\n')
			{
				/*
				**  Fix for BUG 7562. (dkh)
				*/
				vlbadchr(ptr);
				break;
			}
		}
	} while (*ptr++ != '\"' || esc == 1);


	/*
	**  Find out the length of the string in bytes.
	*/
	len = (ptr - buf) - 1;

	*--ptr = EOS;
	return(vl_strcnv(buf, len, FALSE));
}

char *
vl_delim()
{
	char	*ptr;
	char	buf[MAXSTRING + 1];
	i4	len;
	i4	normal_end = FALSE;

	/*
	**  Position starting point at the starting
	**  double quote character.
	*/
	ptr = vl_buffer - 1;

	while (*vl_buffer != EOS)
	{
		if (*vl_buffer == '"')
		{
			CMnext(vl_buffer);

			if (*vl_buffer == EOS || *vl_buffer != '"')
			{
				normal_end = TRUE;
				break;
			}
		}
		else
		{
			CMnext(vl_buffer);
		}
	}

	if (!normal_end)
	{
		/*
		**  This never returns.
		*/
		errcall(VALSTRNG);
	}

	/*
	**  Determine the length (in bytes) of the string including
	**  the closing double quote.  vl_buffer is pointing to the
	**  byte after the closing quote, so the calculation below
	**  is correct.
	*/
	len = vl_buffer - ptr;

	STlcopy(ptr, buf, len);

	buf[len] = EOS;

	ptr = STalloc(buf);
	return(ptr);
}


vl_vchstr()
{
	i4	esc;
	char	buf[MAXSTRING + 1];
	u_i2	len;
	char	*ptr;

	ptr = buf;
	do
	{
		if ((*ptr = *GETCHAR()) == '\0')
		{
			errcall(VALSTRNG);
			break;
		}
		/* Skip the second byte of a double byte character. */
		if (CMdbl1st(ptr))
		{
			if ((*++ptr = *GETCHAR()) == EOS)
			{
				/*
				**  This should never happen but we do
				**  it just in case.
				*/
				errcall(VALSTRNG);
				break;
			}

			/*
			**  If we are dealing with a double byte character,
			**  we can skip the checks in the rest of the
			**  loop since they will never be true.  So we
			**  just do a continue to speed things up.
			*/
			continue;
		}
		esc = 0;
		if (*ptr == '\'')
		{
			if ((*++ptr = *GETCHAR()) == EOS)
			{
				BACKUP();
				break;
			}
			if (*ptr == '\'')
			{
				esc = 1;
				ptr--;
			}
			else
			{
				BACKUP();
				break;
			}
		}
		if (CMcntrl(ptr))
		{
			vlbadchr(ptr);
			break;
		}
	} while (*ptr++ != '\'' || esc == 1);

	/*
	**  Find out the length of the string in bytes.
	*/
	len = (ptr - buf) - 1;

	*--ptr = EOS;
	return(vl_strcnv(buf, len, TRUE));
}





vl_strcnv(buf, len, isvchar)
char	*buf;
u_i2	len;
i4	isvchar;
{
	STATUS		stat = OK;
	DB_TEXT_STRING	*sptr;
	DB_TEXT_STRING	*output;
	DB_DATA_VALUE	outdbv;
	i4		dummy;
	AFE_DCL_TXT_MACRO(MAXSTRING)	faketext;

	/*
	**  Convert the string to a DB_TXT_TYPE DB_DATA_VALUE.
	*/
	output = (DB_TEXT_STRING *) &faketext;
	if (isvchar)
	{
		outdbv.db_datatype = DB_VCH_TYPE;
	}
	else
	{
		outdbv.db_datatype = DB_TXT_TYPE;
	}
	outdbv.db_length = sizeof(faketext);
	outdbv.db_data = (PTR) output;
	output->db_t_count = len;
	MEcopy((PTR) buf, (u_i2) len, (PTR) output->db_t_text);
	if (!isvchar)
	{
		outdbv.db_datatype = DB_TXT_TYPE;
		stat = IIAFpmEncode(&outdbv, (bool) !vl_syntax, &dummy);
	}
	if (stat != OK)
	{
		vl_par_error(ERx("SCANNER"), ERget(E_FV000B_Bad_string), buf);
		return(-1);
	}

	/*
	**  For error handling, we need to save the user's input
	**  as well as the internal value as what the user entered
	**  may be different from the internal value (due to
	**  back slashes, etc.
	*/
	Lastok = STalloc(buf);

	/*
	**  Save the internal value by putting it in IIFVvlval.
	*/
	len = DB_CNTSIZE + output->db_t_count;
	sptr = (DB_TEXT_STRING *)MEreqmem((u_i4)0, (u_i4)len,
	    FALSE, (STATUS *)NULL);
	MEcopy((PTR) output, (u_i2) len, (PTR) sptr);

	IIFVvlval.string_type = sptr;

	if (isvchar)
	{
		return(Tokens.svconst);
	}
	else
	{
		return (Tokens.sconst);
	}
}

void
IIFVndNegateDecimal(decdbv, is_validation)
DB_DATA_VALUE	*decdbv;
i4		is_validation;
{
	DB_DATA_VALUE			dmydbv;
	DB_TEXT_STRING			*textstr;
	AFE_DCL_TXT_MACRO(fdVSTRLEN)	dectext;
	char				dbuf[fdVSTRLEN + 1];
	char				cp[2];
	DB_STATUS			status;

	dbuf[0] = '-';
	dbuf[1] = EOS;
	STcat(dbuf, IIFVfloat_str);

	dmydbv.db_datatype = DB_LTXT_TYPE;
	dmydbv.db_length = sizeof(dectext);
	dmydbv.db_prec = 0;
	textstr = (DB_TEXT_STRING *) &dectext;
	textstr->db_t_count = STlength(dbuf);
	MEcopy((PTR) dbuf, (u_i2) textstr->db_t_count, (PTR)textstr->db_t_text);
	dmydbv.db_data = (PTR) textstr;
	
	if (Adf_cb == NULL)
	{
		Adf_cb = FEadfcb();

		/* Overwrite II_DECIMAL setting with II_4GL_DECIMAL */
		cp[0]=cp[1]='\0';
		status = IIUGlocal_decimal(Adf_cb, cp);
		if (status)
		{
			vl_nopar_error(E_FI2270_8816_BadDecimal, 1, cp);
			return;
		}
		decimal = Adf_cb->adf_decimal.db_decimal;
	}

	if (afe_cvinto(Adf_cb, &dmydbv, decdbv) != OK)
	{
		/*
		**  Conversiion error.
		*/
		if (is_validation)
		{
			errcall(VALCONV);
		}
		else
		{
			errcall(E_FI2081_8321);
		}
	}
}

void
IIFVsdsScanDelimString(delim)
i4	delim;
{
	scan_delim = delim;
}
