/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<cv.h>

/*
**  Name: CVAHXL.C - Hex to long
**
**  Description:
**      This file contains the following external routines:
**    
**      CVahxl()           -  Hex to long
**      CVahxi64()         -  Hex to 64-bit integer
**      CVuahxl()          -  Unsigned hex to unsigned long
**      CVuahxi64()        -  Unsigned hex to unsigned 64-bit integer
**
**  History:
 * Revision 1.2  90/04/10  14:20:59  source
 * sc
 * 
 * Revision 1.3  90/04/04  15:54:27  source
 * sc
 * 
 * Revision 1.2  90/03/26  16:27:30  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:20  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:05  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:19  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:43  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/17  16:54:08  mikem
**      changed to not use CH anymore
**      
**      Revision 1.2  87/11/10  12:36:56  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:16  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      16-mar-90 (fredp)
**          Added CVuahxl().
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      16-jan-1996 (toumi01; from 1.1 axp_osf port) (muhpa01)
**          Added kirke's change for axp_osf
**          10-aug-93 (kirke)
**          Added CVahxl8() and CVuahxl8() for DEC AXP 64 bit longs.
**	29-may-1996 (toumi01)
**	    Make CVahxl8 and CVuahxl8 axp_osf platform-conditional and
**	    replace inclusion of <stdlib.h> with declaration
**	    "extern long int strtol ();" to avoid dragging in other
**	    definitions (such as "abs()") should this code be used in
**	    the future for other 64-bit platforms.
**	16-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      22-mar-1999 (hweho01)
**          Make CVahxl8 and CVuahxl8 available for AIX 64 bit platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-apr-1999 (hanch04)
**	    Make CVahxl8 and CVuahxl8 for LP64
**	07-Sep-2000 (hanje04)
**          Make CVahxl8 and CVuahxl8 available for axp_lnx (Alpha Linux)
**	12-oct-2001 (somsa01)
**	    Added 64-bit versions of CVahxl() and CVuahxl().
**	30-oct-2001 (devjo01)
**	    Replaced MAX/MINI64 (system specific symbols for max/min 64 bit
**	    signed integer value), with MAX/MINI8 (generic Ingres equivalent).
**	15-oct-2003 (somsa01)
**	    Corrected result type in CVahxl8 and CVuahxl8, and use the proper
**	    OS function for Windows 64-bit in those functions.
**	14-Apr-2005 (lakvi01)
**	    Added dummy CVahxi64, CVuahxl8, CVuahxi64, CVahxl8 functions for 
**	    int_w32,so that we could use the same iilibcompat.def file for 
**	    win32 and win64.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Simplify conditionals.
*/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name:	CVahxl - HEX CHARACTER STRING TO 32-BIT
**		         INTEGER CONVERSION
**
** Description:
**	  Convert a hex character string pointed to by `str' to
**	an i4 pointed to by `result'. A valid string is of the
**	form:
**
**		{<sp>} [+|-] {<sp>} {<digit>|<a-f>} {<sp>}
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	i4 pointer, will contain the converted long value
**
**   Returns:
**	OK:		succesful conversion; `i' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow; `i' is unchanged
**	CV_UNDERFLOW:	numeric underflow; 'i' is unchanged
**	CV_SYNTAX:	syntax error; `i' is unchanged
**	
** History:
**	03/09/83 -- (mmm)
**		brought code from vms and modified.   
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**	19-mar-87 (mgw)	Bug # 11825
**		Made it work as it does in Unix and IBM so that for instance,
**		it will now return CV_SYNTAX for "a 1" instead of
**		converting it to 161 decimal. Also cleaned up the spec
**		and added negative number handling.
**	31-mar-87 (mgw)
**		Made it work for MINI4 (= -MAXI4 - 1).
*/
STATUS
CVahxl(str, result)
register char	*str;
u_i4	*result;
{
	register i4	sign;
	register i4	num;
	char		tmp_buf[3];
	char		a;


	num = 0;
	sign = 0;

	/* skip leading blanks */
	for (; *str == ' '; str++)
		;

	/* check for sign */
	switch (*str)
	{
	  case '-':
		sign = -1;

	  case '+':
		while (*++str == ' ')
			;
	}

	/* at this point everything should be a-f or 0-9 */
	while (*str)
	{
		CMtolower(str, tmp_buf);

		if (sign == 0)	/* positive value */
		{
			if (CMdigit(tmp_buf))
			{
				/* check for overflow */

				if ((num > MAXI4 / 10)		||
				    (num = num * 16 + (*str - '0')) < 0)
				{
					/* positive overflow */
					return(CV_OVERFLOW);
				}
			}
			else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
			{
				/* check for overflow */

				if ((num > MAXI4 / 10)		||
				    (num = num * 16 + (tmp_buf[0] - 'a' + 10)) < 0)
				{
					/* positive overflow */
					return(CV_OVERFLOW);
				}
			}
			else if (tmp_buf[0] == ' ')	/* there'd better be only */
			{
				break;		/* blanks left in this case */
			}
			else
			{
				return (CV_SYNTAX);
			}
		}
		else	/* (sign == -1) negative value */
		{
			if (CMdigit(tmp_buf))
			{
				/* check for underflow */

				if ((num < MINI4 / 10)		||
				    (num = num * 16 - (*str - '0')) > 0)
				{
					/* negative overflow */
					return(CV_UNDERFLOW);
				}
			}
			else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
			{
				/* check for underflow */

				if ((num < MINI4 / 10)		||
				    (num = num * 16 - (tmp_buf[0] - 'a' + 10)) > 0)
				{
					/* negative overflow */
					return(CV_UNDERFLOW);
				}
			}
			else if (tmp_buf[0] == ' ')	/* there'd better be only */
			{
				break;		/* blanks left in this case */
			}
			else
			{
				return (CV_SYNTAX);
			}
		}
		str++;
	}

	/* Done with number, better be EOS or all blanks from here */
	while (a = *str++)
		if (a != ' ')			/* syntax error */
			return (CV_SYNTAX);

	*result = num;

	return (OK);
}

/*{
** Name:	CVuahxl - HEX CHARACTER STRING TO UNSIGNED 32-BIT
**		          INTEGER CONVERSION
**
** Description:
**	  Convert a hex character string pointed to by `str' to
**	a u_i4 pointed to by `result'. A valid string is of the
**	form:
**
**		{<sp>} {<digit>|<a-f>} {<sp>}
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	u_i4 pointer, will contain the converted unsigned long value
**
**   Returns:
**	OK:		succesful conversion; `i' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow; `i' is unchanged
**	CV_SYNTAX:	syntax error; `i' is unchanged
**	
** History:
**	16-mar-90 (fredp)
**		Written.
*/
STATUS
CVuahxl(str, result)
register char	*str;
u_i4		*result;
{
	register u_i4	overflow_mask = (u_i4)0xf0000000;
	register u_i4	num;
	char		tmp_buf[3];
	char		a;


	num = 0;

	/* skip leading blanks */
	for (; *str == ' '; str++)
		;

	/* at this point everything should be a-f or 0-9 */
	while (*str)
	{
		CMtolower(str, tmp_buf);

		if (CMdigit(tmp_buf))
		{
			/* check for overflow */

			if ( (num & overflow_mask) != 0 )
			{
				return(CV_OVERFLOW);
			}
			num = num * 16 + (*str - '0');
		}
		else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
		{
			/* check for overflow */

			if ( (num & overflow_mask) != 0 )
			{
				return(CV_OVERFLOW);
			}
			num = num * 16 + (tmp_buf[0] - 'a' + 10);
		}
		else if (tmp_buf[0] == ' ')	/* there'd better be only */
		{
			break;		/* blanks left in this case */
		}
		else
		{
			return (CV_SYNTAX);
		}
		str++;
	}

	/* Done with number, better be EOS or all blanks from here */
	while (a = *str++)
		if (a != ' ')			/* syntax error */
			return (CV_SYNTAX);

	*result = num;

	return (OK);
}

#if defined(LP64)
#if !defined(NT_IA64) && !defined(NT_AMD64)
extern long int strtol ();
#endif  /* !NT_IA64 && !NT_AMD64 */

STATUS
CVahxl8(str, result)
register char   *str;
OFFSET_TYPE     *result;
{
#if defined(NT_IA64) || defined(NT_AMD64)
    *result = _strtoi64(str, (char **)NULL, 16);
#else
    *result = strtol(str, (char **)NULL, 16);
#endif  /* NT_IA64 || NT_AMD64 */
    
    return(OK);
}


STATUS
CVuahxl8(str, result)
register char   *str;
SIZE_TYPE       *result;
{
#if defined(NT_IA64) || defined(NT_AMD64)
    *result = _strtoui64(str, (char **)NULL, 16);
#else
    *result = strtol(str, (char **)NULL, 16);
#endif  /* NT_IA64 || NT_AMD64 */
    
    return(OK);
}
#endif /* LP64 */

#if defined(LP64)
/*{
** Name: CVahxi64 - HEX CHARACTER STRING TO 64-BIT INTEGER CONVERSION
**
** Description:
**	Convert a hex character string pointed to by `str' to
**	an OFFSET_TYPE pointed to by `result'. A valid string is of the
**	form:
**
**		{<sp>} [+|-] {<sp>} {<digit>|<a-f>} {<sp>}
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	OFFSET_TYPE pointer, will contain the converted value
**
**   Returns:
**	OK:		succesful conversion; `i' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow; `i' is unchanged
**	CV_UNDERFLOW:	numeric underflow; 'i' is unchanged
**	CV_SYNTAX:	syntax error; `i' is unchanged
**	
** History:
**	12-oct-2001 (somsa01)
**	    Created from 32-bit version.
*/
STATUS
CVahxi64(str, result)
register char	*str;
OFFSET_TYPE	*result;
{
    register i4			sign;
    register OFFSET_TYPE	num;
    char			tmp_buf[3];
    char			a;

    num = 0;
    sign = 0;

    /* skip leading blanks */
    for (; *str == ' '; str++);

    /* check for sign */
    switch (*str)
    {
	case '-':
	    sign = -1;

	case '+':
	    while (*++str == ' ');
    }

    /* at this point everything should be a-f or 0-9 */
    while (*str)
    {
	CMtolower(str, tmp_buf);

	if (sign == 0)	/* positive value */
	{
	    if (CMdigit(tmp_buf))
	    {
		/* check for overflow */
		if ((num > MAXI8 / 10) || (num = num * 16 + (*str - '0')) < 0)
		{
		    /* positive overflow */
		    return(CV_OVERFLOW);
		}
	    }
	    else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
	    {
		/* check for overflow */
		if ((num > MAXI8 / 10) ||
		    (num = num * 16 + (tmp_buf[0] - 'a' + 10)) < 0)
		{
		    /* positive overflow */
		    return(CV_OVERFLOW);
		}
	    }
	    else if (tmp_buf[0] == ' ')
	    {
		/*
		** there'd better be only blanks left in this case
		*/
		break;
	    }
	    else
	    {
		return (CV_SYNTAX);
	    }
	}
	else
	{
	    /* (sign == -1) negative value */
	    if (CMdigit(tmp_buf))
	    {
		/* check for underflow */
		if ((num < MINI8 / 10) ||
		    (num = num * 16 - (*str - '0')) > 0)
		{
		    /* negative overflow */
		    return(CV_UNDERFLOW);
		}
	    }
	    else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
	    {
		/* check for underflow */
		if ((num < MINI8 / 10) ||
		    (num = num * 16 - (tmp_buf[0] - 'a' + 10)) > 0)
		{
		    /* negative overflow */
		    return(CV_UNDERFLOW);
		}
	    }
	    else if (tmp_buf[0] == ' ')
	    {
		/*
		** there'd better be only blanks left in this case
		*/
		break;
	    }
	    else
	    {
		return (CV_SYNTAX);
	    }
	}

	str++;
    }

    /* Done with number, better be EOS or all blanks from here */
    while (a = *str++)
	if (a != ' ')			/* syntax error */
	    return (CV_SYNTAX);

    *result = num;

    return (OK);
}

/*{
** Name: CVuahxi64 - HEX CHARACTER STRING TO UNSIGNED 64-BIT INTEGER CONVERSION
**
** Description:
**	Convert a hex character string pointed to by `str' to
**	a size_t pointed to by `result'. A valid string is of the
**	form:
**
**		{<sp>} {<digit>|<a-f>} {<sp>}
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	size_t pointer, will contain the converted unsigned value
**
**   Returns:
**	OK:		succesful conversion; `i' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow; `i' is unchanged
**	CV_SYNTAX:	syntax error; `i' is unchanged
**	
** History:
**	13-oct-2001 (somsa01)
**	    Created from the 32-bit version.
*/
STATUS
CVuahxi64(str, result)
register char	*str;
SIZE_TYPE	*result;
{
    register SIZE_TYPE	overflow_mask = (SIZE_TYPE)0xf000000000000000;
    register SIZE_TYPE	num;
    char		tmp_buf[3];
    char		a;

    num = 0;

    /* skip leading blanks */
    for (; *str == ' '; str++);

    /* at this point everything should be a-f or 0-9 */
    while (*str)
    {
	CMtolower(str, tmp_buf);

	if (CMdigit(tmp_buf))
	{
	    /* check for overflow */
	    if ((num & overflow_mask) != 0)
	    {
		return(CV_OVERFLOW);
	    }

	    num = num * 16 + (*str - '0');
	}
	else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
	{
	    /* check for overflow */
	    if ((num & overflow_mask) != 0)
	    {
		return(CV_OVERFLOW);
	    }

	    num = num * 16 + (tmp_buf[0] - 'a' + 10);
	}
	else if (tmp_buf[0] == ' ')
	{
	    /*
	    ** there'd better be only blanks left in this case
	    */
	    break;
	}
	else
	{
	    return (CV_SYNTAX);
	}

	str++;
    }

    /* Done with number, better be EOS or all blanks from here */
    while (a = *str++)
	if (a != ' ')			/* syntax error */
	    return (CV_SYNTAX);

    *result = num;

    return (OK);
}
#else /* LP64 */

STATUS CVahxi64(register char	*str, OFFSET_TYPE	*result)
{
	return(OK); /*don't do anything for non 64-bit windows.*/
}

STATUS CVuahxl8(register char	*str, OFFSET_TYPE	*result)
{
	return(OK); /*don't do anything for non 64-bit windows.*/
}

STATUS CVuahxi64(register char	*str, OFFSET_TYPE	*result)
{
	return(OK); /*don't do anything for non 64-bit windows.*/
}

STATUS CVahxl8(register char	*str, OFFSET_TYPE	*result)
{
	return(OK); /*don't do anything for non 64-bit windows.*/
}
#endif  /* LP64 */
