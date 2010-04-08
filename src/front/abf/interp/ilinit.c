/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilops.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"

/**
** Name:	ilinit.c -	Initializations for interpreter's globals
**
** Description:
**	Performs initializations for the interpreter's global variables.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		change type of IIOframeptr and add IIOcontrolptr.
**
**	Revision 5.1  86/04  arthur
**	Initial revision.
**/

GLOBALDEF IRBLK		il_irblk;

GLOBALDEF char		*IIOapplname = NULL;	/* name of the application */
GLOBALDEF IFFRAME	*IIOframeptr = NULL;	/* frame stack */
GLOBALDEF IFCONTROL	*IIOcontrolptr = NULL;	/* control stack */
GLOBALDEF IL		*IIOstmt = NULL;	/* ptr to current IL stmt */
GLOBALDEF bool		IIOfrmErr = FALSE;	/* if TRUE, exit from frame */
GLOBALDEF bool		IIOdebug = FALSE;	/* if TRUE, print IL stmts */
