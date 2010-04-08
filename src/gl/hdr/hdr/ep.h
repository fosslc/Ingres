/*
**	Copyright (c) 2007 Ingres Corporation
*/


/*
** Name:	EP.h	- Define execution privileges
**
** Specification:
**
** Description:
**	Contains execution privileges
**
** History:
**	13-Aug-2007 (drivi01)
**      24-Dec_2007 (rapma01)
**          added new line at the end of the file 
*/

#ifndef EP_INCLUDED
#define EP_INCLUDED 1

# include <epcl.h>

#define E_CL2D01_NO_PRIVILEGE          ( E_CL_MASK | E_EP_MASK | 0x01 )

#endif

