/*
**    Copyright (c) 1987, 1993, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <cv.h>

/**
** Name:	CVptrax.c -- convert pointer to character string.
**
** Description:
**	This file defines:
**
**	CVptra()	-- convert pointer to a decimal character string.
**
** History:
**	30-Apr-1993 (daveb)
**	    created.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/

/*{
** Name: CVptra -- convert pointer to unsigned decimal character representation.
**
** Description:
**
**     Converts the input pointer to an unsigned hexadecimal
**     representation, occupying up to CV_DEC_PTR_SIZE characters,
**     plus a null termination.  If necessary, the output is padded
**     with leading zeros.
** 
**     Note that CV_DEC_PTR_SIZE will be larger on 64 bit machines
**     than on 32 bit systems.
** 
**     This is the exact inverse of CVaptr.
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
**     ptr	pointer to convert
** 
** Outputs:
** 
**     buf	destination buffer, assumed to be at least
**		CV_DEC_PTR_SIZE + 1 in length.
** 
** Returns:
** 
**     none.
** 
** History:
**	30-Apr-1993 (daveb)
**	    created.
*/

void
CVptra( PTR ptr, char *string )
{
    i4 i;
    unsigned i4 num = (unsigned i4)ptr;

    for (i = CV_DEC_PTR_SIZE ; --i >= 0; )
    {
	string[i] = (num % 10) + '0';
	num /= 10;
    }
    string[ CV_DEC_PTR_SIZE ] = 0;
}


