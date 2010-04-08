/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<cv.h>

/**
** Name:	cvaptr.c - CVaptr() 
**
** Description:
**	This file defines:
**
**	CVaptr	convert decimal string to a pointer.
**
** History:
**	30-Apr-1993 (daveb)
**	    created
**/

/*{
** Name:  CVaptr() -- convert decimal string to a pointer.
**
**	Converts the input buffer in unsigned decimal
**	representation to a pointer value.
**
**	Possible errors include a string that is a value wider than
**	can be represented as a pointer.
**
**	This is the exact inverse of CVptrax.
**
** Notes:
**	Presumes that the "unsigned long" type is 64 bits on a machine
**	with 64 bit pointers.  This may need to be "unsigned long long"
**	instead, turned on with an ifdef.  
**
** Re-entrancy:
**	yes.
**
** Inputs:
**
**	buf	input buffer, assumed to be in 
**		unsigned decimal.
**
** Outputs
**
**	ptrp	pointer to pointer to stuff.
**
** Returns:
**
**	OK or error status.  
**
** History:
**	30-Apr-1993 (daveb)
**	    created.
**      23-may-97 (mcgem01)
**          Clean up compiler warnings.
**	07-aug-2001 (somsa01)
**	    On IA64, an 'unsigned long' is not 64-bit. Thus, instead
**	    use SIZE_TYPE.
*/

STATUS
CVaptr( char *str, PTR *ptrp )
{
    SIZE_TYPE	num;	/* presumed to be 64 bits on 64 bit machine */
    char	a;
    char	tmp_buf[3];
    
    num = 0;
    
    /* skip leading blanks */
    for (; *str == ' '; str++)
	;
    
    /* at this point everything should be 0-9 */
    while (*str)
    {
	CMtolower(str, tmp_buf);
	if (CMdigit(tmp_buf))
	{
	    if ( num > ((SIZE_TYPE)~0/10) )
		return(CV_OVERFLOW);

	    num = num * 10 + (*str - '0');
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
    
    *ptrp = (PTR)num;
    
    return (OK);
}

