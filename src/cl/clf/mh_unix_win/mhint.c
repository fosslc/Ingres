/*
**Copyright (c) 2004 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <mh.h>
#include    <ex.h>
/**
**
**  Name: MHINT.C - MH routines for integer.
**
**  Description:
**      This file contains the MH routines necessary to perform 
**      basic mathematical operations on integer values.
**
**      This file defines the following routines:
**
**          MHi4add() - Add 2 4 byte integers.
**          MHi4sub() - Subtract 2 4 byte integers.
**          MHi4mul() - Multiply 2 4 byte integers.
**          MHi4div() - Divide 2 4 byte integers.
**
**          MHi8add() - Add 2 8 byte integers.
**          MHi8sub() - Subtract 2 8 byte integers.
**          MHi8mul() - Multiply 2 8 byte integers.
**          MHi8div() - Divide 2 8 byte integers.
**
**  History:    $Log-for RCS$
**      24-feb-1993 (stevet)
**          Initial creation.
**      17-sep-1993 (stevet)
**          Fixed MHi4add to avoid raising exception when two
**          valid negative numbers are added (B55029).
**	18-mar-04 (inkdo01)
**	    Added i8 functions.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/
 

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/

/**
**  Name: MHi4add() - Add two i4 values 
** 
**  Description:
**       Return the resultant of adding the two i4 operands and signal 
**       EXINTOVF if the resultant excess MAXI4 or less than MINI4.
**
** Inputs:
**      dv1,dv2                         The input i4 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1+dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      08-mar-1993 (stevet)
**        Initial Creation.
**	15-jul-93 (ed)
**	  adding <gl.h> after <compat.h>
**      17-sep-1993 (stevet)
**        Fixed problem with incorrectly raising exception when adding
**        two negative numbers.
*/
i4
MHi4add(i4 dv1, i4 dv2)
{
    if((dv1 > 0 && dv2 > 0) && MAXI4 - dv1 < dv2)
	EXsignal(EXINTOVF,  0);
    if((dv1 < 0 && dv2 < 0) && MINI4 - dv1 > dv2)
	EXsignal(EXINTOVF,  0);
    return( dv1+dv2);
}

/**
**  Name: MHi8add() - Add two i8 values 
** 
**  Description:
**       Return the resultant of adding the two i8 operands and signal 
**       EXINTOVF if the resultant excess MAXI8 or less than MINI8.
**
** Inputs:
**      dv1,dv2                         The input i8 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1+dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      18-mar-04 (inkdo01)
**	    Written for bigint support. 
*/
i8
MHi8add(i8 dv1, i8 dv2)
{
    if((dv1 > 0 && dv2 > 0) && MAXI8 - dv1 < dv2)
	EXsignal(EXINTOVF,  0);
    if((dv1 < 0 && dv2 < 0) && MINI8 - dv1 > dv2)
	EXsignal(EXINTOVF,  0);
    return( dv1+dv2);
}

/**
**  Name: MHi4sub() - Subtract two i4 values 
** 
**  Description:
**       Return the resultant of subtracting the two i4 operands and signal 
**       EXINTOVF if the resultant excess MAXI4 or less than MINI4.
**
** Inputs:
**      dv1,dv2                         The input i4 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1-dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      08-mar-1993 (stevet)
**        Initial Creation.
*/
i4
MHi4sub(i4 dv1, i4 dv2)
{
    if((dv1 > 0 && dv2 < 0) && MAXI4 + dv2 < dv1)
	EXsignal(EXINTOVF,  0);
    if((dv1 < 0 && dv2 > 0) && MINI4 + dv2 > dv1)
	EXsignal(EXINTOVF,  0);
    return( dv1-dv2);
}


/**
**  Name: MHi8sub() - Subtract two i8 values 
** 
**  Description:
**       Return the resultant of subtracting the two i8 operands and signal 
**       EXINTOVF if the resultant excess MAXI8 or less than MINI8.
**
** Inputs:
**      dv1,dv2                         The input i8 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1-dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      18-mar-04 (inkdo01)
**	    Written for bigint support. 
*/
i8
MHi8sub(i8 dv1, i8 dv2)
{
    if((dv1 > 0 && dv2 < 0) && MAXI8 + dv2 < dv1)
	EXsignal(EXINTOVF,  0);
    if((dv1 < 0 && dv2 > 0) && MINI8 + dv2 > dv1)
	EXsignal(EXINTOVF,  0);
    return( dv1-dv2);
}


/**
**  Name: MHi4mul() - Multiply two i4 values 
** 
**  Description:
**       Return the resultant of multiplying the two i4 operands and signal 
**       EXINTOVF if the resultant excess MAXI4 or less than MINI4.
**
** Inputs:
**      dv1,dv2                         The input i4 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1*dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      08-mar-1993 (stevet)
**        Initial Creation.
*/
i4
MHi4mul(i4 dv1, i4 dv2)
{
    f8  temp1,temp2;
    
    temp1 = dv1;
    temp2 = temp1*dv2;

    if(temp2 > MAXI4 || temp2 < MINI4)
	EXsignal(EXINTOVF,  0);
    return( dv1*dv2);
}


/**
**  Name: MHi8mul() - Multiply two i8 values 
** 
**  Description:
**       Return the resultant of multiplying the two i8 operands and signal 
**       EXINTOVF if the resultant excess MAXI8 or less than MINI8.
**
** Inputs:
**      dv1,dv2                         The input i8 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1*dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      18-mar-04 (inkdo01)
**	    Written for bigint support. 
*/
i8
MHi8mul(i8 dv1, i8 dv2)
{
    f8  temp1,temp2;
    
    temp1 = dv1;
    temp2 = temp1*dv2;

    if(temp2 > MAXI8 || temp2 < MINI8)
	EXsignal(EXINTOVF,  0);
    return( dv1*dv2);
}


/**
**  Name: MHi4div() - Divide two i4 values 
** 
**  Description:
**       Return the resultant of dividing the two i4 operands and signal 
**       EXINTDIV if the dividsor is equal to zero.
**
** Inputs:
**      dv1,dv2                         The input i4 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1/dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      08-mar-1993 (stevet)
**        Initial Creation.
**      06-apr-1993 (stevet)
**        Return 0 on divid-by-zero error.
*/
i4
MHi4div(i4 dv1, i4 dv2)
{
    if(dv2 == 0)
    {
	EXsignal(EXINTDIV,  0);
	return( 0);
    }
    return( dv1/dv2);
}

/**
**  Name: MHi8div() - Divide two i8 values 
** 
**  Description:
**       Return the resultant of dividing the two i8 operands and signal 
**       EXINTDIV if the divisor is equal to zero.
**
** Inputs:
**      dv1,dv2                         The input i8 operands.
**
** Outputs:
**      none
**
**      Returns:
**          dv1/dv2
**
**      Exceptions:
**          EXINTOVF
**
** Side Effects:
**          none
**
** History:
**      18-mar-04 (inkdo01)
**	    Written for bigint support. 
*/
i8
MHi8div(i8 dv1, i8 dv2)
{
    if(dv2 == 0)
    {
	EXsignal(EXINTDIV,  0);
	return( 0);
    }
    return( dv1/dv2);
}
