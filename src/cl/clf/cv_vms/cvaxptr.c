/*
** Copyright (c) 1987, 1993 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<cv.h>

/**
** Name:	cvaxptr.c - CVaxptr() 
**
** Description:
**	This file defines:
**
**	CVaxptr	convert hex string to a pointer.
**
** History:
**	30-Apr-1993 (daveb)
**	    created
**/

/*{
** Name:  CVaxptr() -- convert hex string to a pointer.
**
**	Converts the input buffer in unsigned hexadecimal
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
**		unsigned hex.
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
*/

STATUS
CVaxptr( char *str, PTR *ptrp )
{
    unsigned long	num;	/* presumed to be 64 bits on 64 bit machine */
    PTR			ptr;
    char		a;
    char		tmp_buf[3];
    
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
	    if ( num > ((unsigned long)~0/10) )
		return(CV_OVERFLOW);
	    num = num * 16 + (*str - '0');
	}
	else if (tmp_buf[0] >= 'a' && tmp_buf[0] <= 'f')
	{
	    /* check for overflow */
	    if ( num > (unsigned long)~0 / 10)
		return(CV_OVERFLOW);
	    num = num * 16 + (tmp_buf[0] - 'a' + 10);
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

