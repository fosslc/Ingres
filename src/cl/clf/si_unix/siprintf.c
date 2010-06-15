/*
**	Copyright (c) 1983, 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<cm.h>
# include	<st.h>
# include	<stdarg.h>
# include       <ut.h>
# include	"silocal.h"


/**
** Name:	siprintf.c -	Formatted Print Compatibility Module.
**
** Description:
**	This file contains the definitions of the routines used to output
**	formatted strings through the SI module.  ('SIdofrmt()' is used by
**	the string formatting routines of ST ('STprintf()' and 'STzprintf()',
**	as well.)
**
**	SIprintf()	formatted print to standard output
**	SIfprintf()	formatted print to stream file
**
**		Revision 50.0  86/06/18  09:47:04  wong
**		Added support for '*' precision and widths.
**		
**		Revision 30.6  86/04/25  10:06:29  daveb
**		3.0 porting changes:  Fiddle with float
**		alignment for Gould.
**
**		Revision 30.5  86/01/28  10:18:10  joe
**		Internationalization changes.
**
**		Revision 30.4  86/01/08  16:16:25  boba
**		Bump pointer for f8's to be aligned on ALIGN_F8 
**		(for Gould).
**
**		Revision 30.2  85/09/27  17:53:38  daveb
**		3.0/23ibs international changes for floating point.
**
**		03/09/83 -- (mmm)
**			VMS code brought to UNIX and modified--
**			CV lib calls used, still uses 'fcvt()' and 'ecvt()'
**			assembly routines found in "CVdecimal.s".
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Replace STlength with a call to STLENGTH macro.
**	08-mar-91 (rudyw)
**	    Comment out text following #endif
**	07-jan-1992 (bonobo)
**	    Added the following change from ingres63p cl: 08-aug-91 (philiph).
**	08-aug-91 (philiph)
**	    Integate a change that got lost in 6.3/02. Use CM routines
**	    to ckeck object name strings for double-byte characters.
**	30-Apr-1993 (daveb)
**	    Add '%p' support.
**	19-jun-95 (emmag)
**	    Desktop porting changes.   SI is quite different on NT.
**	30-may-95 (tutto01, canor01)
**	    Changed call to SIdofrmt to allow caller to specify method of
**	    writing characters.  This changed addressed reentrancy issues.
**	21-dec-95 (tsale01)
**	    Corrected changes made by tutto01 and canor01 to work with
**	    double byte correctly.
**	    While we are at it, fix feature where only half of a double byte
**	    character is printed when printing using precision of wrong size.
**	    Now SIdofrmt will only print complete characters. Any byte position
**	    left over (only ever 1) will be space filled.
**	16-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    Added 02-sep-93 (kchin)
**	    Changed type of var_args in SIdofrmt() to long.
**	19-mar-1996 (canor01)
**	    For Windows NT only: remove thread-local storage definition.
**	24-jun-96 (somsa01)
**	    For Windows NT only; removed linebuf variable, since it is not used,
**	    and changed printbuf[512] to printbuf[2048].
**	07-may-96 (emmag)
**	    Remove wn.h which is no longer required on NT.
**	28-oct-1997 (canor01, by kosma01)
**	    Redo variable-arg function definitions for SIprintf() and
**	    SIfprintf(), matching prototype and removing NT dependency.
**	19-feb-1998 (toumi01)
**	    For Linux (lnx_us5) define FILE_PTR for FILE structure as
**	    _IO_write_ptr, else define FILE_PTR as _ptr.
**      24-jul-1998 (rigka01)
**          Cross integrate bug 86989 from 1.2 to 2.0 codeline
**          bug 86989: When passing a large CHAR parameter with report,
**          the generation of the report fails due to parm too big
**          changed temp in SIdofrmt() from 350 to 4096
**	15-jan-1999 (popri01)
**	   For Unixware 7, UNIX 95 compliance introduces FILE structure
**	   member name variations.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	19-apr-1999 (hanch04)
**	    Use FILE or char ptrs depending on type passed
**	13-May-2004 (schka24)
**	    Implement ll flag for i8:  %lld, %llo, %llx.
**	    Note that unlike the "real" printf, ll is always an i8, *not*
**	    a "long long" (which may actually be an i16 on 64-bit platforms).
**	    This also makes %ld work for 64-bit longs on lp64 platforms.
**	    Cut down on compiler warnings, CM wants uchar.
**	9-Jun-2004 (schka24)
**	    Fix up previous change to be more careful about unsigned-ness.
**	    (%o, %x).  Implement %u (with l, ll variants) for unsigned
**	    decimal.  Unsigned-ness is pretty important on 64-bit platforms
**	    for printing things like shared memory keys (ahem).
**	    Also change things a bit so that %x (o, d, u) is always for
**	    an i4 (or u_i4);  %llx (o, d, u) as noted above is always for
**	    an i8 (or u_i8);  %lx (o, d, u) will print a long-sized thing,
**	    which is typically appropriate for a pointer.
**       3-Apr-2008 (hanal04) Bug 120202
**          Provide uppercase hex with %X.
**	8-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets 
**	    which can can be upto four bytes long. Use CMbytecnt to ensure 
**	    we do not assume that a multbyte string can only be a maximum 
**	    of two bytes long. (Bug 120865)
**	14-May-2009 (kiria01) b122073
**	    Don't scan beyond a given precision with %s
**     12-Apr-2010 (maspa05) bug 123560
**          SIdofrmt unable to handle long strings (such as those passed by
**          SC930) because realwidth was an i2. Make it an i4.
**	4-Jun-2010 (kschendel)
**	    Fix constant CMcpychar's to be *ptr++ = c instead.  Otherwise,
**	    the double-byte macro might refer past the end of a constant, and
**	    indeed past the end of an object section, causing "local relocation
**	    in section __text does not target section __const" on MacOS x86_64.
*/

# define	LEFTADJUST	0x01
# define	ZEROFILL	0x02
# define	WIDTHGIVEN	0x04
# define	PRECGIVEN	0x08
# define	LONGGIVEN	0x10
# define	LLGIVEN		0x20
# define	UNSIGNED	0x40

VOID	SIdofrmt();

/*VARARGS1*/

VOID
SIprintf( const char *fmt, ... )
{
	va_list	ap;

	va_start( ap, fmt );
	SIdofrmt(PUTC, stdout, fmt, ap);
	va_end( ap );
}

/*VARARGS2*/

VOID
SIfprintf( FILE *ioptr, const char *fmt, ... )
{
	va_list	ap;

	va_start( ap, fmt );
	SIdofrmt(PUTC, ioptr, fmt, ap);
	va_end( ap );
}

/*VARARGS2*/

VOID
SIdofrmt(putfunc, outarg, fmt, ap)
i4		putfunc;
void		*outarg;
char	 	*fmt;
va_list		ap;
{
	register uchar	*p;
	register uchar	*r;
	i4		width;
	i4		prec;
	i4		flag;
	i4		realwidth;
	uchar		temp[SI_MAXOUT];
	bool		float_flag;
	FILE		*outfile=(FILE *)outarg;
	uchar		*outchar=(uchar *)outarg;
	i4 		byte_cnt, iter;

	for (p = (uchar *)fmt ; *p != EOS ; CMnext(p))
	{
		if (*p != '%')
		{
			if ( putfunc == PUTC )
			{
				SIputc(*p, outfile);
				if (CMdbl1st(p))
				{
				    byte_cnt = CMbytecnt(p);
				    for (iter = 1; iter < byte_cnt; iter++)
					SIputc(*(p+iter), outfile);
				}
			}
			else
			{
				CMcpychar(p, outchar);
                                CMnext(outchar);
			}
		}

		else for (flag = 0, CMnext(p);;)
		{
			float_flag = FALSE;

			/*
			** Comparing only against native character set in
			** this switch (single byte characters)
			*/
			switch (*p)
			{
			case '%':
				r = p;
				realwidth = 1;
				break;

			case '-':
				flag |= LEFTADJUST;
				CMnext(p);

				if ((!CMdigit(p) || *p == '*'))
					continue;

			case '0':
				if (*p == '0' && (flag & ZEROFILL) == 0)
				{
					flag |= ZEROFILL;
					CMnext(p);
				}

				if (!(CMdigit(p) || *p == '*'))
					continue;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '*':
				width = 0;
				flag |= WIDTHGIVEN;

				if (*p == '*')
				{
					width = va_arg( ap, long );
					CMnext(p);
				}
				else while (CMdigit(p))
				    {
					width = width * 10 + (*p - '0');
					CMnext(p);
				    }
	
				if (*p != '.')
					continue;

				/* FALL THROUGH */

			case '.':
				prec = 0;
				flag |= PRECGIVEN;
				CMnext(p);
				if (*p == '*')
				{
					prec = va_arg( ap, long );
					CMnext(p);
				}
				else while (CMdigit(p))
				    {
					prec = prec * 10 + (*p - '0');
					CMnext(p);
				    }

				continue;

			case 'c':
				r = temp;
				*r = va_arg( ap, long );
				realwidth = 1;
				break;

			case 'l':
				CMnext(p);
				if (flag & LONGGIVEN)
				    flag |= LLGIVEN;
				flag |= LONGGIVEN;
				continue;

			case 'u':
				flag |= UNSIGNED;
				/* fall thru... */
			case 'd':
			{
				i8	n;

				/* Note that architectures almost universally
				** pass int-like things as "long" on the
				** stack.  Doing a va-arg with anything smaller
				** can screw up the arg ptr on systems with
				** aligned stacks but poorly defined
				** va-arg macros.
				*/
				if (flag & UNSIGNED)
				{
				    if (flag & LLGIVEN)
					n = va_arg(ap, u_i8);
				    else if (flag & LONGGIVEN)
					n = (u_i8) (va_arg(ap, u_long));
				    else
					n = (u_i4) (va_arg(ap, u_long));
				}
				else
				{
				    if (flag & LLGIVEN)
					n = va_arg(ap, i8);
				    else if (flag & LONGGIVEN)
					n = va_arg(ap, long);
				    else
					n = (i4) va_arg( ap, long );
				}

				CVla8(n, r = temp);
				realwidth = STLENGTH_MACRO(r);
				break;
			}

			case 'e':
			case 'f':
			case 'g':
			case 'n':
			{
				f8	f8val;
				i4	dec_pt;
				i2      res_len;

				float_flag = TRUE;

				if ((flag & PRECGIVEN) == 0)
					prec = 3;

				if ((flag & WIDTHGIVEN) == 0)
					width = 20;

				/* pass large number of digits width
				** CVfa will pass back the real number
				** of digits printed.
				*/

#				ifdef PYRAMID	/**** C compiler bug ****/
				/* Clear hi-order 2 bytes of word on 	*/
				/* stack containing realwidth (PYRAMID	*/
				/* code assumes it is cleared.		*/

				realwidth = 0; 
#				endif /**** PYRAMID	C compiler bug ****/

				/*
				** With international support all floating
				** point formats have a decimal point character
				** after the value on the argument list.
				*/

				f8val =va_arg( ap, f8 ) ;
				/* get multinational decimal point char */
				dec_pt = va_arg( ap, i4  );

				(void)CVfa(f8val, width, prec, *p,
				    (char)(dec_pt & 0377), r = temp, &res_len);

				realwidth=(i4) res_len;

				break;
			}

			case 'o':
			case 'x':
                        case 'X':
			{
				static char	hextab[] = "0123456789abcdef";
                                static char     Uhextab[] = "0123456789ABCDEF";

				i4	mask;
				i4	shift;
                                char    *ht;

				u_i8 n; /* MUST BE UNSIGNED */

                                if (*p == 'X')
                                {
                                    ht = Uhextab;
                                }
                                else
                                {
                                    ht = hextab;
                                }

				if ((*p == 'x') || (*p == 'X'))
				{
					mask  = 0xf;
					shift = 4;
				}
				else
				{
					mask  = 0x7;
					shift = 3;
				}
				/* These are always unsigned conversions */
				if (flag & LLGIVEN)
				    n = va_arg(ap, u_i8);
				else if (flag & LONGGIVEN)
				    n = (u_i8) (va_arg(ap, u_long));
				else
				    n = (u_i4) (va_arg( ap, u_long ));

				r = &temp[100];

				do
				{
					*--r = ht[n & mask];
					n >>= shift;
				} while (n > 0);

				realwidth = &temp[100] - r;
				break;
			}

			case 'p':
			{
				static char	hextab[] = "0123456789abcdef";

				PTR	p;
				i4	mask;
				i4	shift;

				/* assumes unsigned long is 64 bits if
				   PTR is 64 bits */

				unsigned long n; /* MUST BE UNSIGNED */

				mask  = 0xf;
				shift = 4;

				p = va_arg( ap, PTR );
				n = (unsigned long)p;

				r = &temp[100];

				do
				{
					*--r = hextab[n & mask];
					n >>= shift;
				} while (n > 0);

				realwidth = &temp[100] - r;
				break;
			}

			case 's':
				if (r = va_arg( ap, uchar * ))
				{
				    if (flag & PRECGIVEN)
				    {
					/* Not necessarily null terminated ... */
					realwidth = 0;
					while (realwidth < prec && r[realwidth])
					    realwidth++;
				    }
				    else
					realwidth = STLENGTH_MACRO(r);
				}
				else
				{
					r = (uchar *) "<NULL>";
					realwidth = 6;
				}
				break;

			default:
				if (*p != EOS)
				{
					r = p;
					realwidth = 1;
				}

				break;
			}	/* end switch */

			if (*p != EOS)
			{
				if ((flag & WIDTHGIVEN) == 0)
					width = realwidth;

				if ((flag & LEFTADJUST) == 0 &&
						realwidth < width)
				{
					char	c = (flag & ZEROFILL) != 0
								? '0' : ' ';

					do
					{
			                    if ( putfunc == PUTC )
						 SIputc(c, outfile);
			                    else
						*outchar++ = c;
					} while (realwidth < --width);
				}

				/* test if its a float and format differently
				** if it is.
				*/

				if (!float_flag && realwidth > width)
					realwidth = width;
			
				width -= realwidth;

				/* have we space for another character? */
				while (realwidth >= CMbytecnt(r))
				{
				    CMbytedec(realwidth, r);
			            if ( putfunc == PUTC )
				    {
					SIputc(*r, outfile);
					if (CMdbl1st(r))
					{
					  byte_cnt = CMbytecnt(r);
					  for (iter = 1; iter < byte_cnt; iter++)
						SIputc(*(r+iter), outfile);
					}
				    }
			            else
			            {
				        CMcpychar(r, outchar);
                                        CMnext(outchar);
			            }
				    CMnext(r);
				}

				/* we may not have filled precision */
				/* 'adjust' the width */
				width += realwidth;

				while (--width >= 0)
				{
			            if ( putfunc == PUTC )
					SIputc(' ', outfile);
			            else
					*outchar++ = ' ';
				}
			}

			break;
		}

		if (*p == EOS)
			break;
	}	/* end for */
	if( putfunc != PUTC )
	    *outchar = '\0';
}

