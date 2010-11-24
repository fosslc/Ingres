/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <cs.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <ddb.h>
# include    <ulm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <add.h>
# include    <dmf.h>
# include    <dmtcb.h>
# include    <scf.h>
# include    <qsf.h>
# include    <qefrcb.h>
# include    <rdf.h>
# include    <psfparse.h>
# include    <qefnode.h>
# include    <qefact.h>
# include    <qefqp.h>
# include    <me.h>
# include    <mh.h>
# include    <cm.h>
# include    <si.h>
# include    <pc.h>
# include    <lo.h>
# include    <ex.h>
# include    <er.h>
# include    <st.h>
# include    <tm.h>
# include    <cv.h>
# include    <generr.h>
# include    <clfloat.h>
/* beginning of optimizer header files */
# include    <opglobal.h>
# include    <opdstrib.h>
# include    <opfcb.h>
# include    <opgcb.h>
# include    <opscb.h>
# include    <ophisto.h>
# include    <opboolfact.h>
# include    <oplouter.h>
# include    <opeqclass.h>
# include    <opcotree.h>
# include    <opvariable.h>
# include    <opattr.h>
# include    <openum.h>
# include    <opq.h>

/*
**	opqscanf - portable sscanf-like function
**
**	Description:
**	  This function is used by optimizedb utility when parsing
**	contents of a text file with statistics. It is similiar to 
**	the "C" language sscanf function, the difference being that
**	it implements a subset of functionality offered by sscanf.
**
**	History:
**	17-feb-89 (stec)
**	    Copied from CLF, since STscanf is going away. Will
**	    have to be maintained locally in OPF.
**	20-mar-89 (stec)
**	    Changed some code to remove LINT complaints, changed
**	    some i4 ptr tp PTRs. 
**	19-jan-90 (stec)
**	    made changes for portability.
**	13-mar-90 (stec)
**	    Made it really portable.
**	10-jan-91 (stec)
**	    Change INT, FLOAT defines, because of conflicts on UNIX.
**	    Also changed remaining defines to make naming consistent - all
**	    defines start now with OPQ prefix.
**	06-apr-92 (rganski)
**	    Merged changes from seg:
**	    Function declarations must be had from the mandatorily-included
**	    CL header files, not declared locally.  Eliminated spurious ST and
**	    CV declarations, and included the appropriate header files.
**	09-nov-92 (rganski)
**	    Added prototypes. Added #includes so that opq.h could be included
**	    here, which allows prototype checking for opq_scanf().
**	    Minor cosmetic changes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-jul-93 (rganski)
**	    Changed result param to CVahxl to u_i4, according to
**	    prototype.
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
static i4 opq_instr(
	PTR pptr,
	char type,
	i4 len,
	char **bufp,
	i4 *eof);
static i4 opq_innum(
	PTR *ptr,
	char type,
	i4 len,
	i4 size,
	char **bufp,
	i4 *eof,
	char decimal);
static i4 opq_doscan(
	char *buf,
	i4 blen,
	char decimal,
	register char *fmt,
	register PTR *pargp);
i4 opq_scanf(
	char *buf,
	char *fmt,
	char decimal,
	PTR arg1,
	PTR arg2,
	PTR arg3,
	PTR arg4,
	PTR arg5);

# define	OPQSHORT	0
# define	OPQREGULAR	1
# define	OPQLONG		2

# define	OPQINT		0
# define	OPQFLOAT	1

# define	OPQFLTSRT	020
# define	OPQFLTREG	021
# define	OPQFLTLNG	022
# define	OPQINTSRT	00
# define	OPQINTREG	01
# define	OPQINTLNG	02
# define	OPQSPLEN	30000


static i4
opq_instr(
PTR	    pptr,
char	    type,
i4	    len,
char	    **bufp,
i4	    *eof)
{
    char	*ptr = (char *)pptr;
    char	*buf = *bufp;
    char	*optr = ptr;
    i4		ret_val = 0;

    if (type == 'c' && len == OPQSPLEN)
	len = 1;

    if (type == 's')
    {
	while (CMcntrl(buf))
	{
	    CMbytedec(len, buf);
	    CMnext(buf);
	}
    }

    while (len > 0 && *buf != EOS && !CMcntrl(buf) && !CMwhite(buf))
    {
	CMbytedec(len, buf);

	if (ptr)
	{
	    CMcpyinc(buf, ptr);
	}
	else
	{
	    CMnext(buf);
	}
    }
 	
    if (*buf != EOS)
    {
	*eof = 0;
    }
    else
    {
	*eof = 1;
    }

    if (ptr && ptr != optr)
    {
	if (type != 'c')
	{
	    *ptr = EOS;
	    CMnext(ptr);
	}

	ret_val = 1;
    }

    *bufp = buf;
    return (ret_val);
}


static i4
opq_innum(
PTR	    *ptr,
char	    type,
i4	    len,
i4	    size,
char	    **bufp,
i4	    *eof,
char	    decimal)
{
    char	*np;
    char	numbuf[64];
    char	*buf;
    i4	lcval;
    i4		base, expseen, decseen;
    i4		scale, negflg, ndigit;

    /* This initialization is for opq_innum and opq_instr */
    *eof = 0;

    /* for chars and strings call a separate proc */
    if (type == 'c' ||  type == 's')
    {
	if (ptr)
	    return opq_instr(*ptr, type, len, bufp, eof);
	else
	    return opq_instr((PTR)NULL, type, len, bufp, eof);
    }

    /* process a number */
    lcval = 0;
    ndigit = 0;
    expseen = 0;
    decseen = 0;
    negflg = 0;
    scale = OPQINT;
    buf = *bufp;
    np = numbuf;

    if (type == 'e' || type == 'f')
	scale = OPQFLOAT;

    /* set base */
    if (type == 'o')
	base = 8;
    else if (type == 'x')
	base = 16;
    else
	base = 10;

    /* remove leading white space */
    while (CMwhite(buf))
    {
	CMbytedec(len, buf);
	CMnext(buf);
    }

    /* process sign if any */
    if (*buf == '-')
    {
	negflg++;
	CMbytedec(len, buf);
	CMcpyinc(buf, np);
    }
    else if (*buf == '+')
    {
	CMbytedec(len, buf);
	CMnext(buf);
    }

    /* process the number */
    for(; CMbytedec(len, buf) >= 0; )
    {
	char	c[2];

	CMcpychar(buf, c);
	CMtolower(c,c);

	if (CMdigit(c)
	    ||
	    (base == 16 && CMhex(c))
	   )
	{
	    ndigit++;

	    if (base == 8)
		lcval <<= 3;			    /* fast multiply by 8 */
	    else if (base == 10)
		lcval = ((lcval<<2) + lcval) << 1;  /* fast multiply by 10 */
	    else
		lcval <<= 4;			    /* fast multiply by 16 */

	    if (CMdigit(c))
	    {
		/* is a digit in the 0..9 range */
		i4 val;

		c[1] = EOS;		/* null terminate for conversion */
		(VOID) CVan(c, &val);
		lcval += (i4)val;
	    }
	    else
	    {
		/* it is a hex digit in the A..F range */
		u_i4	val;

		c[1] = EOS;		/* null terminate for conversion */
		(VOID) CVahxl(c, &val);
		lcval += (i4)val;
	    }
	}
	else if (*buf == decimal && decseen == 0)
	{
	    if (base != 10 || scale == OPQINT)
		break;

	    decseen++;
	    ndigit++;
	}
	else if ((*buf == 'e' || *buf == 'E') && expseen == 0)
	{
	    if (base != 10 || scale == OPQINT || ndigit == 0)
		break;

	    expseen++;

	    CMcpyinc(buf, np);

	    if (*buf != '+' && *buf != '-' && !CMdigit(buf))
		break;
	}
	else
	{
	    break;
	}

	CMcpychar(buf, np);
	CMnext(np);

	if (len != 0)
	{
	    CMnext(buf);
	}
    }

    /* negate the value if necessary */
    if (negflg)
	lcval = - lcval;

    /* set end-of-file indicator */
    if (*buf != EOS)
    {
	*eof = 0;
    }
    else
    {
	*eof = 1;
    }

    /* there is no number or it's to be placed nowhere */
    if (!ptr || np == numbuf)
    {
	return (0);
    }

    /* return the number into the scan argument */
    switch((scale << 4) | size)
    {
    case OPQFLTSRT:
    case OPQFLTREG:
	*np = EOS;  /* null terminate str */
	(VOID) CVaf(numbuf, decimal, *(f8 **)ptr);
	break;
    case OPQFLTLNG:
	*np = EOS;  /* null terminate str */
	(VOID) CVaf(numbuf, decimal, *(f8 **)ptr );
	break;
    case OPQINTSRT:
	**(i2**)ptr = lcval;
	break;
    case OPQINTREG:
	**(i4**)ptr = lcval;
	break;
    case OPQINTLNG:
	**(i4**)ptr = lcval;
	break;
    }

    /*
    ** let the caller know how far in 
    ** the buffer we got and return.
    */
    *bufp = buf;
    return (1);
}


static i4
opq_doscan(
char		*buf,
i4		blen,
char		decimal,
register char	*fmt,
register PTR	*pargp)
{
    char	  f[2];		/* double byte char from the format str. */
    i4		  len;		/* length from the format spec. */
    i4		  lenspec;	/* length spec'd in the format spec.- flag */
    i4		  nmatch = 0;	/* number of matched format specs. */
    PTR		  *ptr;
    i4		  size;
    PTR		  *argp = pargp;
    i4		  endfile = 0;
    register char *bend = buf + blen;	/* end of text buffer to be scanned. */

    while (buf <= bend)
    {
	CMcpychar(fmt, f);
	CMnext(fmt);

	switch(f[0])
	{
	/* consume white space */
	case ' ': case '\t': case '\n': case '\r':
	    while (CMwhite(buf))
	    {
		CMnext(buf);
	    }
	    continue;

	/* this may be the beginning of a format spec. */
	case '%':
	    CMcpychar(fmt, f);
	    CMnext(fmt);

	    if((f[0]) == '%')
		goto def;

	    /* we have found a format spec. */  
	    lenspec = 0;
	    len = 0;
	    size = OPQREGULAR;

	    if (f[0] != '*')
	    {
		ptr = argp++;
	    }
	    else
	    {
		ptr = (PTR *)NULL;
		CMcpychar(fmt, f);
		CMnext(fmt);
	    }

	    while (CMdigit(f))
	    {
		i4 val;

		f[1] = EOS;	/* null terminate for conversion */
		(VOID) CVan(f, &val);
		len = len * 10 + val;
		CMcpychar(fmt, f);
		CMnext(fmt);
		lenspec++;
	    }

	    if (f[0] == 'l')
	    {
		size = OPQLONG;
		CMcpychar(fmt, f);
		CMnext(fmt);
	    }
	    else if (f[0] == 'h')
	    {
		size = OPQSHORT;
		CMcpychar(fmt, f);
		CMnext(fmt);
	    }

	    if (f[0] == EOS)
		return (0);

	    /*
	    **  Change "len" to OPQSPLEN if format string is for reading
	    **  some sort of string so call to opq_instr() will keep 
	    **  same semantics.
	    */
	    if (!lenspec && (f[0] == 'c' || f[0] == 's'))
	    {
		len = OPQSPLEN;
	    }
	    else
	    {
		len = STlength(buf);
	    }

	    if (opq_innum(ptr, f[0], len, size, &buf,
		    &endfile, decimal) && ptr)
		nmatch++;

	    if (endfile)
		return (nmatch ? nmatch : 0);

	    break;

	/* char is not a white space char not have format spec. */	
	def:
	default:
	    if (CMcmpcase(f, buf))
	    {
		return (nmatch);
	    }
	    CMnext(buf);
	    break;
	} /* switch */
    } /* while */

    return (nmatch);
}

i4
opq_scanf(
char	*buf,
char	*fmt,
char	decimal,
PTR	arg1,
PTR	arg2,
PTR	arg3,
PTR	arg4,
PTR	arg5)
{
    PTR	    args[5];
    i4	    blen;

    blen = STlength(buf);

    args[0] = arg1;
    args[1] = arg2;
    args[2] = arg3;
    args[3] = arg4;
    args[4] = arg5;

    return opq_doscan(buf, blen, decimal, fmt, args);
}
