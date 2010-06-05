/*
**  Copyright (c) 1983, 2002 Ingres Corporation
*/

/*
** History:
**	29-apr-94 (marc_b)
**	    Created history section and added fix for bug 63020 which
**	    is also bug 42003 (change 265486). Original change info
**	    below:
**	    02-apr-92 (andys/jpk)
**	    SIdofrmt was losing the top bit of unsigned ints printed
**	    with %x (bug 42003). The code to fix this problem was already
**	    in this file, #ifdef'ed on UNS_I4. Remove these preprocessor
**	    commands to install fix.
**	15-Feb-96 (wooke01)
**	    DBCS chars nbot being displayed correctly within messages 
**	    such as creating table '....' need to insert some code very much
**	    like that in siprintf.c to calculate widths etc of DBCS chars.
**	27-feb-1996 (canor01)
**	    Add '%p' support.
**	28-apr-1999 (somsa01)
**	    Corrected case where we would treat outarg as either a file or a
**	    character buffer, which matches what we do on UNIX platforms.
**	    Also, changed nat's and longnat's to i4's, and we don't need to
**	    include varargs.h on NT (stdarg.h contains the necessary
**	    definitions).
**      02-apr-2001 (mcgem01)
**          Add an OpenROAD specific version of SIdofrmt for backward
**          compatability.  This version is invoked in SIprint_ortrace.
**	13-oct-2001 (somsa01)
**	    Corrected printf "%p" printout on 64-bit.
**	21-dec-2001 (somsa01)
**	    Added LEVEL1_OPTIM for i64_win to prevent the Terminal Monitor
**	    from SEGV'ing when two successive "help <object> <object name>"
**	    are issued via SEP.
**	05-apr-2002 (somsa01)
**	    Removed LEVEL1_OPTIM restriction from i64_win.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	22-Aug-2005 (gupsh01)
**	    Match up with changes made to siprintf.c for unix, to 
**	    implement ll flag for i8: %lld, %llo, %llx. ll is always 18.
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
**	   
*/

# include       <compat.h>
# include       <si.h>
# include       <cm.h>
# include       <cv.h>
# include       <st.h>
# include       <gl.h>


# define        LEFTADJUST      0x01
# define        ZEROFILL        0x02
# define        WIDTHGIVEN      0x04
# define        PRECGIVEN       0x08
# define        LONGGIVEN       0x10
# define        LLGIVEN		0x20
# define        UNSIGNED	0x40

static char     hextab[] = "0123456789abcdef";
static char     Uhextab[] = "0123456789ABCDEF";

VOID
SIdofrmt(calltype, outarg, fmt, ap)
i4              calltype;
void            *outarg;
char            *fmt;
va_list         ap;
{
	register uchar * p;
	register uchar * r;
	uchar *          outp = (char *)outarg;
	i4              width;
	i4              prec;
	i4              flag;
	i4              realwidth;
	uchar            temp[2048];
	bool            float_flag;
	FILE		*outfile=(FILE *)outarg;
	i4		iter, byte_cnt;

	for (p = (uchar *)fmt ; *p != EOS ; CMnext(p))
	{
		if (*p != '%')
		{
			if (calltype == PUTC)
			{
				SIputc(*p,outfile);
				if (CMdbl1st(p))
				{
				    byte_cnt = CMbytecnt(p);
				    for (iter = 1; iter < byte_cnt; iter++)
					SIputc(*(p+iter), outfile);
				}
			}
			else
			{
			 	CMcpychar(p, outp);
				CMnext(outp);
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
					width = va_arg( ap, i4 );
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
					prec = va_arg( ap, i4 );
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
				*r = va_arg( ap, i4);   /* 6-x_PC_80x86 *//* changed from i2 to i4, jkronk */
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
				/* fall through ... */

			case 'd':
			{
				i8 n;

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
				  if ((flag & LLGIVEN) != 0)
					n = va_arg (ap, i8);
				  else if ((flag & LONGGIVEN) != 0)
					n = va_arg( ap, i4 );
				  else
					n = va_arg( ap, i4 );
			 	}	

				CVla8(n, r = temp);
				realwidth = STlength(r);
				break;
			}

			case 'e':
			case 'f':
			case 'g':
			case 'n':
			{
				f8      f8val;
				char    dec_pt;
				i2      res_len;

				float_flag = TRUE;

				if ((flag & PRECGIVEN) == 0)
					prec = 6;

				if ((flag & WIDTHGIVEN) == 0)
					width = 20;

				/* pass large number of digits width
				** CVfa will pass back the real number
				** of digits printed.
				*/


				/*
				** With international support all floating
				** point formats have a decimal point character
				** after the value on the argument list.
				*/
				f8val = va_arg( ap, f8);

				/* get multinational decimal point char */
				dec_pt = va_arg( ap, char );

				CVfa(f8val, (i4)width, (i4)prec, *p,
				    (char)(dec_pt & 0377), r = temp, &res_len);

				realwidth=(i4) res_len;

				break;
			}

			case 'o':
			case 'x':
                        case 'X':
			{
				i4      mask;
				i4      shift;
				u_i8	n;
                                char    *ht;

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

				if ((flag & LLGIVEN) != 0)
				        n = va_arg( ap, u_i8);
				else if ((flag & LONGGIVEN) != 0)
					n = (u_i8) va_arg( ap, u_i4 );
				else
					n = (u_i4) va_arg( ap, u_i4 );

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
                                PTR     p;
                                i4      mask;
                                i4      shift;
                                SIZE_TYPE n;	/* MUST BE UNSIGNED */

                                mask  = 0xf;
                                shift = 4;

                                p = va_arg( ap, PTR );
                                n = (SIZE_TYPE)p;

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
				if (*p != EOS)
				{
					r = p;
					realwidth = 1;
				}

				break;
			}       /* end switch */

			if (*p != EOS)
			{
				if ((flag & WIDTHGIVEN) == 0)
					width = realwidth;


			/* don't need to touch this part for DBCS as it */
			/* deals with left justification (wooke01) */

				if ((flag & LEFTADJUST) == 0 &&
						realwidth < width)
				{
					register char c = (flag & ZEROFILL) != 0
								? '0' : ' ';

					if (calltype == PUTC)
					{
						do
						{
						    SIputc(c, outfile);
						}
						while (realwidth < --width);
					}
					else
					{
						do
						{
							*outp++ = c;
						}
						while (realwidth < --width);
					}
				}

				/* test if its a float and format differently
				** if it is.
				*/

				if (!float_flag && realwidth > width)
					realwidth = width;

				width -= realwidth;
					
				/* have we space for another character? */
				while (realwidth >=CMbytecnt(r))
				{
					CMbytedec(realwidth,r);

					if (calltype == PUTC)
					{
					    SIputc(*r,outfile);
					    if (CMdbl1st(r))
					    {
				    		byte_cnt = CMbytecnt(r);
				    		for (iter = 1; 
							iter < byte_cnt; 
							iter++)
						SIputc(*(r+iter),outfile);
					    }
					}
					else
					{
					    CMcpychar(r,outp);
					    CMnext(outp);
					} /* end else */
					CMnext(r);

				} /* end while */

					/* we may not have filled the precision */
					/* 'adjust' realwidth */

					width +=realwidth;
				  	
					while (--width>=0)
					{
					    if (calltype == PUTC)
						SIputc(' ',outfile);
					    else
					    {
						CMcpychar((uchar *)" ", outp);
						CMnext(outp);
					    } /* end if */
					} /* end while  */
				
			break; 
		} /* end if */
						if (*p == EOS)
							break; /* get out of switch */
		}	 /* end for else */
	} /* end for */

			
	/* terminate the buffer */
	if (calltype != PUTC)
	{
	    *outp = 0;
	}
} 

VOID
SIdoorfrmt(calltype, outarg, fmt, ap)
i4             calltype;
FILE            *outarg;
char            *fmt;
va_list         ap;
{
	register char * p;
	register char * r;
	char *          outp = (char *)outarg;
	i4             width;
	i4             prec;
	i4             flag;
	i4              realwidth;
	char            temp[2048];
	bool            float_flag;
	i4		iter, byte_cnt;

	for (p = fmt ; *p != EOS ; CMnext(p))
	{
		if (*p != '%')
		{
			if (calltype == SI_SPRINTF)
			{
		 		/* old code *outp++ = *p; */

			 	CMcpychar(p, outp);
				CMnext(outp);
			}
			else
			{
				/* old code SIputc(*p, (FILE *)outarg); */
				SIputc(*p,outarg);
				if (CMdbl1st(p))
				{
				    byte_cnt = CMbytecnt(p);
				    for (iter = 1; iter < byte_cnt; iter++)
					SIputc(*(p+iter), outarg);
				}

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
					width = va_arg( ap, i4 );
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
					prec = va_arg( ap, i4 );
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
				*r = va_arg( ap, i4);   /* 6-x_PC_80x86 *//* changed from i2 to i4, jkronk */
				realwidth = 1;
				break;

			case 'l':
				CMnext(p);
				flag |= LONGGIVEN;
				continue;

			case 'd':
			{
				i4 n;

				if ((flag & LONGGIVEN) != 0)
					n = va_arg( ap, i4);
				else
					n = va_arg( ap, i4 );

				CVla(n, r = temp);
				realwidth = STlength(r);
				break;
			}

			case 'e':
			case 'f':
			case 'g':
			case 'n':
			{
				f8      f8val;
				char    dec_pt;
				i2      res_len;

				float_flag = TRUE;

				if ((flag & PRECGIVEN) == 0)
					prec = 6;

				if ((flag & WIDTHGIVEN) == 0)
					width = 20;

				/* pass large number of digits width
				** CVfa will pass back the real number
				** of digits printed.
				*/


				/*
				** With international support all floating
				** point formats have a decimal point character
				** after the value on the argument list.
				*/
				f8val = va_arg( ap, f8);

				/* get multinational decimal point char */
				dec_pt = va_arg( ap, char );

				CVfa(f8val, (i4)width, (i4)prec, *p,
				    (char)(dec_pt & 0377), r = temp, &res_len);

				realwidth=(i4) res_len;

				break;
			}

			case 'o':
			case 'x':
			{
				i4     mask;
				i4     shift;
				unsigned long n;

				if (*p == 'x')
				{
					mask  = 0xf;
					shift = 4;
				}
				else
				{
					mask  = 0x7;
					shift = 3;
				}

				if ((flag & LONGGIVEN) != 0)
					n = va_arg( ap, i4 );
				else
					n = va_arg( ap, i4 );

				r = &temp[100];

				do
				{
					*--r = hextab[n & mask];
					n >>= shift;
				} while (n > 0);

				realwidth = &temp[100] - r;
				break;
			}

                        case 'p':
                        {
                                PTR     p;
                                i4     mask;
                                i4     shift;

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
				r = va_arg( ap, char * );

				if (r != NULL)
					realwidth = STlength(r);
				else
				{
					r = "<NULL>";
					realwidth = 6;
				}

				if ((flag & PRECGIVEN) && (prec < realwidth))
					realwidth = prec;

				break;

			default:
				if (*p != EOS)
				{
					r = (char *)p;
					realwidth = 1;
				}

				break;
			}       /* end switch */

			if (*p != EOS)
			{
				if ((flag & WIDTHGIVEN) == 0)
					width = realwidth;


			/* don't need to touch this part for DBCS as it */
			/* deals with left justification (wooke01) */

				if ((flag & LEFTADJUST) == 0 &&
						realwidth < width)
				{
					register char c = (flag & ZEROFILL) != 0
								? '0' : ' ';

					if (calltype == SI_SPRINTF)
					{
						do
						{
							*outp++ = c;
						}
						while (realwidth < --width);
					}
					else
					{
						do
						{
							SIputc(c, (FILE *)outarg);
						}
						while (realwidth < --width);
					}
				}

				/* test if its a float and format differently
				** if it is.
				*/

				if (!float_flag && realwidth > width)
					realwidth = width;

				width -= realwidth;
					
				/* have we space for another character? */
				while (realwidth >=CMbytecnt(r))
				{
					CMbytedec(realwidth,r);

					if (calltype == SI_SPRINTF)
					{
				
						CMcpychar(r,outp);
						CMnext(outp);
					}

					else
					{
						SIputc(*r,outarg);
						if (CMdbl1st(r))
						{
				    		    byte_cnt = CMbytecnt(r);
				    		    for (iter = 1; 
							  iter < byte_cnt; 
							  iter++)
						    {
							SIputc(*(r+iter),outarg);
						    }
						} /* end if */
					} /* end else */
					CMnext(r);

				} /* end while */

					/* we may not have filled the precision */
					/* 'adjust' realwidth */

					width +=realwidth;
				  	
					while (--width>=0)
					{
						if (calltype == SI_SPRINTF)
							{
							CMcpychar((uchar *)" ", outp);
							CMnext(outp);
							}
						else
						{
							SIputc(' ',outarg);							
						} /* end if */
					} /* end while  */
				
			break; 
		} /* end if */
						if (*p == EOS)
							break; /* get out of switch */
		}	 /* end for else */
	} /* end for */

			
	/* terminate the buffer */
	if (calltype == SI_SPRINTF)
	{
		*outp = 0;
	}
}  /* end prog */

 /* end prog */
