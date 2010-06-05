/*
**    Copyright (c) 1986, 2000 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include	<cv.h>
# include	<silocal.h>
# include	<stdarg.h>

/**
** Name:	sidofmt.c	- Shared formating routine.
**
** Description:
**	This file contains SIdofrmt, which is the shared
**	formatting routine for the various printf fucntions:
**	SIprintf, SIfprintf, and STprintf.
**
**	SIdofrmt	-	Shared formatting routine.
**
** History:
**	15-sep-86 (Joe)
**		Extracted from siprintf.
**	11/03/92 (dkh) - Changed to use the ansi c stadrag mechanism for dealing
**			 with variable number of arguments to a routine.
**      16-jul-93 (ed)
**	    added gl.h
**	26-Jul-1993 (daveb)
**	    Add '%p' support.
**      11-Apr-1994 (lan)
**          Bug #61163 - when II_EMBED_SET is set to PRINTGCA and you try to
**          bring up the TM, you get an Access Violation.
**      19-Jul-1995 (whitfield)
**          Bug #69918 - Added %p case to switch in function SIdofrmt.  This
**          case was present for VAX but not for Alpha conditionalized code.
**          As a result, uses of %p lead to var_arg skew which lead to an
**          access violation in frontend tools that enabled tracing by defining
**          the logical name ii_embed_set to be printgca.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
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
**	14-May-2009 (kiria01) b122073
**	    Don't scan beyond a given precision with %s
**/

# define	LEFTADJUST	0x01
# define	ZEROFILL	0x02
# define	WIDTHGIVEN	0x04
# define	PRECGIVEN	0x08
# define	LONGGIVEN	0x10
# define	LLGIVEN		0x20
# define	UNSIGNED	0x40

/*{
** Name:	SIdofrmt	- Shared formatting routine.
**
** Description:
**	This routine is called by the various printf to format
**	the output.  It is passed a FILE structure that it
**	uses to do the outputing.  Output is always done
**	a character at a time through the macro SIput_macro.
**	This macro is based on SIputc, but instead of
**	calling flsbuf, it takes a function as an argument
**	which the caller passs in.  This permits STprintf to be called
**	without requiring the rest of the SI routines.
**
** Inputs:
**	outarg		The FILE structure to print to.
**
** 	fmt		The formatting string to use.
**
**	flfunc		The function to call to flush
**			the output.
**
**	p1		The arguments for fmt.  This
**			is a pointer to the argument list of
**			the calling routine.
**
** History:
**	15-sep-86  (Joe)
**		Extracted from siprintf.c and
**		parameterized.
**	26-Jul-1993 (daveb)
**	    Add '%p' support.
**	20-oct-93 (swm)
**	    Bug #56449
**	    The '%p' case "forgot" to initialise its local pointer to the
**	    next vararg (*p1). Added in the initialisation. INGRES server
**	    no longer access violates.
**      11-Apr-1994 (lan)
**          Fixed bug #61163.  Eliminated the local variable p in the case for
**          the %p format.
**      13-Jul-1995 (whitfield)
**          Fixed bug #69918.  Add support for %p for Alpha.
**      19-may-99 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**       3-Apr-2008 (hanal04) Bug 120202
**          Provide uppercase hex with %X.
**       2-may-2008 (horda03) Bug 120202
**          Fix syntax error.
**      12-Apr-2010 (maspa05) bug 123560
**          SIdofrmt unable to handle long strings (such as those passed by
**          SC930) because realwidth was an i2. Make it an i4.

*/

/* VARARGS2 */
VOID
SIdofrmt(outarg, fmt, flfunc, ap)
register FILE	*outarg;
const char	*fmt;
STATUS		(*flfunc)();
va_list		ap;
{
	register uchar	*p;
	register u_i4		c;
	static	char		hextab[] = "0123456789abcdef";
	static	char		Uhextab[] = "0123456789ABCDEF";
        char                    *ht;
	uchar			*r;
	i4			width;
	i4			prec;
	i4			flag;
	i4			realwidth;
	uchar		temp[SI_MAXOUT];
	bool			float_flag;


	for (p = (uchar *)fmt; *p != EOS; CMnext(p))
	{
		if (*p != '%')
		{
			SIput_macro(flfunc, *p, outarg);
			continue;
		}

		for (flag = 0, CMnext(p);;)
		{

			float_flag = FALSE;

			switch (*p)
			{
			case '%':
				r = (char *)p, realwidth = 1;
				break;

			case '-':
				flag |= LEFTADJUST;

				CMnext(p);
				if (!(CMdigit(p) || *p == '*'))
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
					width = va_arg(ap, long);
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
					prec = va_arg(ap, long);
					CMnext(p);
				}
				else while (CMdigit(p))
				    {
					prec = prec * 10 + (*p - '0');
					CMnext(p);
				    }

				continue;

			case 'c':
				r	= temp;
				*r = va_arg(ap, long);
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
				f8	val;
				i4	decp;
				char	decchar;
				i2	res_len;

				/*
				** With international support
				** all floating formats have a
				** decimal point character after
				** the value.
				*/
				val = va_arg(ap, f8);
				decp = va_arg(ap, i4);
				decchar = decp & 0377;

				float_flag = TRUE;

				if ((flag & PRECGIVEN) == 0)
					prec = 3;

				if ((flag & WIDTHGIVEN) == 0)
					width = 20;


				/* pass large number of digits width
				** CVfa will pass back the real number
				** of digits printed.
				*/

				CVfa(val, width, prec, *p, decchar, r = temp, &res_len);
				realwidth=(i4) res_len;

				break;
			}

			case 'o':
			case 'x':
                        case 'X':
			{
				i4	mask, shift;

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
					mask = 0xf, shift = 4;
				else
					mask = 0x7, shift = 3;

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
				i4	mask;
				i4	shift;
				
				/* assumes unsigned long is 64 bits if
				   PTR is 64 bits */
				
				unsigned long n; /* MUST BE UNSIGNED */
				
				mask  = 0xf;
				shift = 4;
				
				n = va_arg(ap, unsigned long);
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
				if (r = va_arg(ap, uchar *))
				{
				    if (flag & PRECGIVEN)
				    {
					/* Not necessarily null terminated ... */
					realwidth = 0;
					while (realwidth < prec && r[realwidth])
					    realwidth++;
				    }
				    else
					realwidth = STlength(r);
				}
				else
				{
					r = (uchar *) "<NULL>";
					realwidth = 6;
				}
				break;

			default:
				if (*p)
					r = p, realwidth = 1;
				else
					return;
				break;
			}

			if ((flag & WIDTHGIVEN) == 0)
				width = realwidth;

			if ((flag & LEFTADJUST) == 0 && realwidth < width)
				for (c = flag & ZEROFILL ? '0' : ' '; realwidth < width; SIput_macro(flfunc, c, outarg), width--)
					;

			/* test if its a float and format differently
			** if it is.
			*/

			if (!(float_flag) && (realwidth > width))
				realwidth = width;
			
			width -= realwidth;

			while (--realwidth >= 0)
			{
				SIput_macro(flfunc, *r, outarg);
				 r++;
			}

			while (--width >= 0)
				SIput_macro(flfunc, ' ', outarg);

			break;
		}
	}
}

