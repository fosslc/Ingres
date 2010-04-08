/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<cm.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqrun.h>

/*{
+*  Name: IIParProc - Process a format in a param string
**
**  Description:
**	This routine is called just after a single "%" has been
**	seen in a param string.  It steps though the format string,
**	accepting one instance of any of the following format
**	specifications and assigning appropriate values to htype, 
**	hlen and indlen:
**
**		%fn	f4 or f8 type, depening on 'n'
**		%in	i1, i2, i4, or i8, depending on 'n'
**		%notrim character (only relevant to "set"-type param strings)
**		%nat	i4ural-sized integer
**		%c	character
**		%cn	character (ABF generated)
**		%a	DB_DATA_VALUE (for internal use)
**		%t	DB_DTE_TYPE (for internal use)
**		%vn	VARCHAR (n bytes of text, not including 2 byte length)
**		%p(p,s) Packed decimal where (p,s) are precision and scale
**
**	The above formats may be followed by 
**		:%in	i2 is only allowable size; 'n' may be 1,4 in future.
**	An indicator type is accepted on all types, but should not
**	be used with a %a.
**
**	White space is not allowed anywhere within a format specification.
**
**	This routine is currently called by IIparret, IIparset and IIcsParget.
**
**  Inputs:
**	target		- Pointer into param format string
**	htype		- Pointer to host type specification
**	hlen		- Pointer to host length specification
**	is_ind		- Pointer to indicator flag
**	errbuf		- Pointer to character buffer for (LIBQ) error messages
**	flag		- II_PARSET - Param string to send data
**			- II_PARRET - Param string to return data
**			- II_PARFRS - Param string from FRS statement
**
**  Outputs:
**	htype		- Will contain type of parameter
**	hlen		- Will contain length of parameter.  If no length
**			  is specified for strings then will contain zero.
**	is_ind		- Will contain 
**			       >0 - indicator specified in param string
**				0 - no indicator specified in param string
**	errbuf		- Will contain error text (if any) only if LIBQ
**	Returns:
**	    Number of chars processed in legal param string 
**	    or -1 if illegal param string
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	21-oct-1986 - extracted from iiparset.c, iiparret.c (bjb)
**	13-may-1988 - Bug 2629 - modified unspecified character lengths 
**		      as they are handled by ADH. (ncg)
**	09-aug-1988 - Removed check for maximum length. Handled by ADH. (ncg)
**	09-aug-1990 - Added decimal param type. (teresal)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Jul-2004 (schka24)
**	    Allow i8
*/

i4
IIParProc(target, htype, hlen, is_ind, errbuf, flag)
register char	*target;
i4	*htype, *hlen, *is_ind;
char	*errbuf;
i4	flag;
{
    char	buf[20];	/* Temp buf for error message fragments */
    char	numbuf[20];	/* Buffer for parameter */
    char	*cp = target;	/* Pointer into target string */

    *is_ind = 0;			/* Default : no indicator */
    buf[0] = '%';			/* Start of error */
    switch (*cp)
    {
      case 'a' :
      case 'A' :
	*htype = DB_DBV_TYPE;
	*hlen = sizeof(DB_DATA_VALUE);
	break;
      case 'f' :
      case 'F' :
	*htype = DB_FLT_TYPE;
	switch (cp[1])
	{
	  case '4':
	  case '8':
	    *hlen = cp[1] - '0';
	    cp++;			/* Not double */
	    break;
	  default:
	    STlcopy( cp, buf+1, 2);
	    goto par_err;
	}
	break;

      case 'i' :
      case 'I' :
	*htype = DB_INT_TYPE;
	switch (cp[1])
	{
	  case '1':
	  case '2':
	  case '4':
	  case '8':
	    *hlen = cp[1] - '0';
	    cp++;			/* Not double */
	    break;
	  default:
	    STlcopy( cp, buf+1, 2);
	    goto par_err;
	}
	break;

      case 'n':
      case 'N':
	if ((flag & II_PARFRS) == 0)
	{
	/* 
	** Notrim should only appear on LIBQ "set" param strings.  However,
	** we will silently ignore it on LIBQ "ret" param strings.
	*/
	    if (STbcompare(ERx("notrim"), 6, cp, 6, TRUE) == 0)
	    {
		cp += 5;
		*htype = DB_CHR_TYPE;
		if (flag & II_PARSET)
		    *hlen = -1;	/* Flag no trim for transfunc */
		else
		    *hlen = 0;
		break;
	    }
	}
	if (STbcompare(ERx("nat"), 3, cp, 3, TRUE) == 0)
	{
	    cp += 2;
	    *htype = DB_INT_TYPE;
	    *hlen = sizeof(i4);
	    break;
	}
	STlcopy( cp, buf+1, 6 );	/* Give some context in err msg */
	goto par_err;

      case 'p' :
      case 'P' :
	if (cp[1] == '(' )		/* Must have "(p,s)" */
	{
	    char	*bufps = numbuf;
	    i4		plen, slen, i = 0;

	    CMnext(cp);			/* Skip 'p' */
	    CMnext(cp);			/* Skip '(' */

	    while (CMdigit(cp) && i < 2) /* Allow two digits for precision */
	    {
		i++;
		CMcpyinc(cp, bufps);
	    }
	    bufps = '\0';
	    if (CVan(numbuf, &plen) != OK || *cp != ',')
	    {
	        STlcopy( cp, buf+1, 5);
	        goto par_err;
	    }
	    CMnext(cp);

	    bufps = numbuf;		/* Reset number buffer pointer */
	    i = 0;			/* Reset counter */
	    while (CMdigit(cp) && i < 2) /* Allow two digits for scale */
	    {
		i++;
		CMcpyinc(cp, bufps);
	    }
	    bufps = '\0';
	    if (CVan(numbuf, &slen) != OK || *cp != ')')
	    {
	        STlcopy( cp, buf+1, 5);
	        goto par_err;
	    }	
	    *hlen = (i4) DB_PS_ENCODE_MACRO(plen,slen);
	    *htype = DB_DEC_TYPE;
	}
	else
	{
	    STlcopy( cp, buf+1, 5);
	    goto par_err;
	}
	break;

      case 'c' :
      case 'C' :
      case 'v':
      case 'V' :
	if (*cp == 'c' || *cp == 'C')
	    *htype = DB_CHR_TYPE;
	else
	    *htype = DB_VCH_TYPE;
	/* 
	** ABF may include a length on "ret"-type param character strings.
	** In 6.0 users may include a length on param char and varchar
	** (ret and set) strings.
	*/
	if (!CMdigit(cp+1))
	{
	    /*
	    ** Set length to zero as will be set to required length
	    ** within ADH.
	    */
	    *hlen = 0;
	}
	else
	{
	    char	*bufp = numbuf;
	    i4		i = 0;
	    CMnext(cp);		/* Skip the current char, and point at digit */
	    do
	    {
		CMbyteinc(i, cp);
		CMcpyinc(cp, bufp);
	    } while (CMdigit(cp) && i < 20);
	    cp--;				/* Last was a digit - so okay */
	    *bufp = '\0';
	    if (i >= 20 || CVan(numbuf, hlen) != OK)
	    {
		STprintf( buf+1, ERx("c%s"), numbuf );
		goto par_err;
	    } 
	    if (*htype == DB_VCH_TYPE)
		*hlen += 2;		/* Add in length of text count field */
	}
	break;
	 
      case 'd':
      case 'D':
	/*
	** ABF may generate dummy format descriptions on FRS statements
	** only.
	*/
	if (flag & II_PARFRS)
	{
	  *htype = DB_DMY_TYPE;
	  *hlen = 0;
	  break;
	}
	else
	{
	  buf[1] = 'd';
	  buf[2] = '\0';
	  goto par_err;
	}
      case 't':
      case 'T':
	/*
	** Front end code may generate DTE types
	*/
	*htype = DB_DTE_TYPE;
	*hlen = 0;
	break;

      default:	/* BUG 1112 - Disallow illegal % */
	STlcopy( cp, buf+1, 3 );	/* Give some context in err msg */
	goto par_err;
    } /* switch */

    /* Check for indicator variables */
    CMnext(cp);
    if (cp[0] == ':')
    {
	if (cp[1] == '%' && (cp[2] == 'i' || cp[2] == 'I'))
	{
	    if (cp[3] != '2')
	    {
		STlcopy(cp, buf, 4);
		goto par_err;
	    }
	    else
	    {
		*is_ind = 1;
	    }
	    cp += 4;	/* STlength(ERx(":%i2")) */
	}
	else
	{
	    STlcopy(cp, buf, 4);
	    goto par_err;
	} 
    }	/* if indicator variable */
    return (cp -target);		/* Number of chars scanned */

par_err:				/* Handle errors in param strings */
    
    if ((flag & II_PARFRS) == 0)
	STlcopy( buf, errbuf, 20 );	/* Only LIBQ wants error text */
    return -1;
}
