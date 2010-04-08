/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulm.h>

#include    <iifas.h>
#include    <ursm.h>

/**
**
**  Name: URSMETHOD - User Request Services Method routines
**
**  Description:
**      This file contains the service routines for the Methods defined 
** 	within an interface in the Frontier application server. 
**
**	urs_call_method		Call a method (with a variable number of parms)
**
**  History:
** 	23-jun-1998 wonst02
** 	    Original
**	10-Aug-1998 (fanra01)
**	    Removed unnecessary headers.
** 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: urs_call_method	Call a method 
**
** Description:
** 	Call a method (with an indeterminate number of arguments). Determine
** 	the number of arguments for the routine and switch to the appropriate
** 	call. 
** 
** 	[As far as I know, there is no portable way to build a parameter
** 	list dynamically. This giant switch statement is the best I could
** 	come up with.]
**
** Inputs:
**      ursm				The URS Manager Control Block
**
** Outputs:
** 		
**	Returns:
**              AS_OK                   0
**              AS_ERROR                1
**              AS_UNKNOWN_INTERFACE    2
**              AS_UNKNOWN_METHOD       3
**              AS_MEMORY_FAILURE       4
**
**	Exceptions:
** 		The total number of arguments must be no more than 50
** 		[unless this routine is coded to handle more].
**
**
** Side Effects:
**	    none
**
** History:
**      23-jun-1998 wonst02
**          Original
*/
int
urs_call_method(URS_MGR_CB	*ursm,
		URSB		*ursb)
{
	FAS_METHOD	*method = (FAS_METHOD *)ursb->ursb_method;
	URSB_PARM	*ursbparm = ursb->ursb_parm;
	IIAPI_DATAVALUE	*parm;
	II_PTR		p[50];
	i4		i;
	int		s;
	int 		(*pMethod) ();

	if (method->fas_method_argcount > 50)
	{	
	    urs_error(E_UR0704_URS_TOO_MANY_ARGS, URS_CALLERR, 3,
		      STlength(ursbparm->methodname), ursbparm->methodname,
		      sizeof method->fas_method_argcount, 
		        &method->fas_method_argcount,
		      sizeof (i4), (i4)50);
	    SET_URSB_ERROR(ursb, E_UR0144_AS_EXECUTE_ERROR, E_DB_ERROR)
	    return AS_ERROR;
	}
	/* The input and in/out arguments */
	for (parm = ursbparm->pInValues, i = 0;
	     parm && (i < method->fas_method_inargcount);
	     parm++, i++)
	{
	    p[i] = parm->dv_value;
	}
	/* The in/out and output arguments */
	ursbparm->sNumOutValues = method->fas_method_outargcount;
	for (parm = ursbparm->pOutValues;
	     parm && (i < method->fas_method_inargcount + 
	     		  method->fas_method_outargcount);
	     parm++, i++)
	{
	    p[i] = &parm->dv_value;
	}

	pMethod = (int (*)()) method->fas_method_addr;

	switch (method->fas_method_argcount) 
	{
	    case 0:
		s = (*pMethod) ();
		break;
	    case 1:
		s = (*pMethod) (p[0]);
		break;
	    case 2:
		s = (*pMethod) (p[0], p[1]);
		break;
	    case 3:
		s = (*pMethod) (p[0], p[1], p[2]);
		break;
	    case 4:
		s = (*pMethod) (p[0], p[1], p[2], p[3]);
		break;
	    case 5:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4]);
		break;
	    case 6:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5]);
		break;
	    case 7:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6]);
		break;
	    case 8:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		break;
	    case 9:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8]);
		break;
	    case 10:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9]);
		break;
	    case 11:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10]);
		break;
	    case 12:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11]);
		break;
	    case 13:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12]);
		break;
	    case 14:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13]);
		break;
	    case 15:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14]);
		break;
	    case 16:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15]);
		break;
	    case 17:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16]);
		break;
	    case 18:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17]);
		break;
	    case 19:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18]);
		break;
	    case 20:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19]);
		break;
	    case 21:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20]);
		break;
	    case 22:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21]);
		break;
	    case 23:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22]);
		break;
	    case 24:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23]);
		break;
	    case 25:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24]);
		break;
	    case 26:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25]);
		break;
	    case 27:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26]);
		break;
	    case 28:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27]);
		break;
	    case 29:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28]);
		break;
	    case 30:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29]);
		break;
	    case 31:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30]);
		break;
	    case 32:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31]);
		break;
	    case 33:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32]);
		break;
	    case 34:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33]);
		break;
	    case 35:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34]);
		break;
	    case 36:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35]);
		break;
	    case 37:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36]);
		break;
	    case 38:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37]);
		break;
	    case 39:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38]);
		break;
	    case 40:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39]);
		break;
	    case 41:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40]);
		break;
	    case 42:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41]);
		break;
	    case 43:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42]);
		break;
	    case 44:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42],p[43]);
		break;
	    case 45:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42],p[43],p[44]);
		break;
	    case 46:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42],p[43],p[44],p[45]);
		break;
	    case 47:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42],p[43],p[44],p[45],p[46]);
		break;
	    case 48:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42],p[43],p[44],p[45],p[46],p[47]);
		break;
	    case 49:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42],p[43],p[44],p[45],p[46],p[47],
				p[48]);
		break;
	    case 50:
		s = (*pMethod) (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], 
				p[8], p[9], p[10],p[11],p[12],p[13],p[14],p[15],
				p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23],
				p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31],
				p[32],p[33],p[34],p[35],p[36],p[37],p[38],p[39],
				p[40],p[41],p[42],p[43],p[44],p[45],p[46],p[47],
				p[48],p[49]);
		break;
	}

	return s;
}
