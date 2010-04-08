/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h> 
#include	<cv.h>
#include	<me.h>
#include	<cm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>


/**
** Name:    ugfmt.c -	Front-End Utility Message Formatting Routine.
**
** Description:
**	This file defines:
**
**	IIUGfmt		- format a string in a buffer, using ER conventions.
**
** History:
**	Revision 6.0  87/08/03  peter
**	Initial revision.
**      15-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Added kchin's change (from 6.4) for axp_osf
**              10-mar-93 (kchin)
**              Added NO_OPTIM for axp_osf.
**      15-jan-1999 (schte01)
**              Removed NO_OPTIM for axp_osf.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

#define	CBUFSIZE	100		/* Size of formatting buf */
#define	NULLPARAM	ERx("< ??? >")	/* use when string unknown */
#define	F_WIDTH		20
#define	F_PREC 		5


/*{
** Name:    IIUGfmt() -	string formatting routine
**
** Description:
**	This routine can be used to create a buffer which has
**	a set of parameters to substitute, using the formatting
**	conventions of the ER library.  These conventions are
**	defined to provide the following capabilities:
**
**	1.  Provide international message support.  This allows
**	    parameters to be substituted in strings in any order
**	    in order to allow translators flexibility in the
**	    grammer of languages.
**	2.  Provide a portable routine, which uses PTR for all
**	    parameters.
**	3.  Provide a simple interface which convers the most
**	    common cases.
**
**	This routine should be used by the frontends, in place of
**	STprintf, when formatting messages which will be seen by
**	end users.
**
**	The parameter substitutions work in the following way.
**	The formatting string will be scanned looking for a syntax
**	of '%nx', where 'n' is the positional number of the parameter
**	numbered from 0, and 'x' is the format designator, which 
**	indicates the datatype of the parameter and the format
**	into which the parameter should be put.  It is the programmer's
**	responsibility to ensure that the parameters are of the correct
**	type.  This routine will only check for the most obvious errors,
**	such as NULL pointers, and parameter count mismatches.
**
**	The valid formats are:
**		c	parameter is a pointer to a null terminated string,
**			and will be printed out in full.
**		d	parameter is a pointer to a i4.  It will
**			be formatted as an integer value, with leading
**			and trailing spaces removed.
**		x	parameter is a pointer to a i4.  It will
**			be formatted as a hexidecimal value.
**		f	parameter is a pointer to a double.  It will be
**			formatted with leading and trailing spaces 
**			removed.
**
**	Example formatting strings:
**		"table %0c does not exist"
**		"the value '%1x' (integer '%0d') is incorrect"
**
**	If you need fancier numeric formatting, use the formatting library
**	to put values into strings, and then pass as strings to this
**	routine.
**
** Inputs:
**	char	*buf		- buffer to place formatted results in.
**	i4	bufsize		- size of 'buf'
**	char	*fmt		- null terminated format string
**	i4	parcount	- number of parameters (used for error check)
**	PTR	a1-a10		- parameters to the string.  These
**				  must either be pointers to character
**				  strings or PTRs to i4s and correspond
**				  to the numbered parameters in the message
**
** Returns:
**	{char *}  Pointer to buf.
**
** History:
**	01-aug-1987 (peter)
**		Written.
*/

/*VARARGS4*/
char	*
IIUGfmt (buf, bufsize, fmt, parcount, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char		*buf;
register i4	bufsize;
char		*fmt;
i4		parcount;
PTR		a1;
PTR		a2;
PTR		a3;
PTR		a4;
PTR		a5;
PTR		a6;
PTR		a7;
PTR		a8;
PTR		a9;
PTR		a10;
{
    PTR			params[10];		/* params in array */
    register char	*inbuf;
    register char	*outbuf;

    params[0] = a1;
    params[1] = a2;
    params[2] = a3;
    params[3] = a4;
    params[4] = a5;
    params[5] = a6;
    params[6] = a7;
    params[7] = a8;
    params[8] = a9;
    params[9] = a10;

    /*
    **  Format the text with parameters into the callers buffer.
    **  The message is truncated if it will not fit.
    */

    MEfill((u_i2)bufsize, EOS, (PTR)buf);
    outbuf = buf;

    for(inbuf = fmt ; *inbuf != EOS && bufsize > 1 ; CMnext(inbuf))
    {
	if (*inbuf == '\\') 
	{ /* escaped character */
	    ++inbuf;
	    if (CMbytecnt(inbuf) < bufsize)
	    {	
		CMcpychar(inbuf, outbuf);
		CMbytedec(bufsize, outbuf);
		CMnext(outbuf);
	    }
	}
	else if (*inbuf != '%')
	{ /* plain character - copy */
	    if (CMbytecnt(inbuf) < bufsize)
	    {
		CMcpychar(inbuf, outbuf);
		CMbytedec(bufsize, outbuf);
		CMnext(outbuf);
	    }
	}
	else
	{ /* A parameter, "%?" */
	    i4	pnumber;

	    ++inbuf;
	    if (!CMdigit(inbuf) || (pnumber = *inbuf - '0') + 1 > parcount)
	    { /* Must not be a number */
		if ((CMbytecnt(inbuf)+1) < bufsize)
		{
		    *outbuf++ = '%';
		    --bufsize;
	    	    CMcpychar(inbuf, outbuf);
		    CMbytedec(bufsize, outbuf);
	    	    CMnext(outbuf);
		}
	    }
	    else if (params[pnumber] == NULL)
	    { /* A null parameter.  Put in funny string */
		if (sizeof(NULLPARAM) - 1 < bufsize)
		{
		    STcat(outbuf, NULLPARAM);
		    bufsize -= sizeof(NULLPARAM) - 1;
		    outbuf += sizeof(NULLPARAM) - 1;
		}
		CMnext(inbuf);
	    }
	    else
	    { /* format parameter */
		register char	*cp;
		char		cbuf[CBUFSIZE+1];

		cbuf[0] = EOS;
		cp = cbuf;
		switch (*++inbuf)
		{
		  case 'd':
	    	    /*
		    ** Convert a pointer to a i4 into a string
		    ** in the buffer
		    */
	    	    CVla((i4)*(i4 *)params[pnumber], cbuf);
		    break;

	    	  case 'f':
		  {
		    i2	res_width;

		    /*
		    ** Convert a floating point number of 
		    ** width 20 into the buffer.
		    */

	    	    CVfa(*(double *)params[pnumber],
			    (i4)F_WIDTH, (i4)F_PREC, 'n', '.', cbuf, &res_width
		    );
		    break;
		  }

		  case 'c':
	    	    /* Convert a character array into buffer. */
		    cp = (char *) params[pnumber]; 
		    break;

		  case 'x':
		    cp = STprintf(cbuf, ERx("0x%08x"),
					*(u_i4 *)params[pnumber]
		    );
		    break;

		  default:
	    	    break;
		} /* end switch */
		if (cp == cbuf)
		{
		    while (*cp != EOS && CMwhite(cp))
			CMnext(cp);
		}
		while (*cp != EOS && bufsize > 1)
		{
		    CMcpychar(cp, outbuf);
		    CMbytedec(bufsize, outbuf);
		    CMnext(outbuf);
		    CMnext(cp);
		}
	    } /* end format parameter */
	} /* end parameter processing */
    } /* end for */
    return buf;
}
