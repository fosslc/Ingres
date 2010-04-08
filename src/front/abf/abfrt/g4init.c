/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"g4globs.h"


/*
** Name:	g4init.c - Initialization routine for EXEC 4GL access
**
** Description:
**	This file defines:
**
**	IIAG4Init	Initialize EXEC 4GL access
**
** History:
**	16-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*
** Function transfer vector table. 
*/
static AG4XFERS xfertbl = 
{
    IIAG4acArrayClear,
    IIAG4bpByrefParam,
    IIAG4ccCallComp,
    IIAG4chkobj,
    IIAG4drDelRow,
    IIAG4fdFillDscr,
    IIAG4gaGetAttr,
    IIAG4ggGetGlobal,
    IIAG4grGetRow,
    IIAG4i4Inq4GL,
    IIAG4icInitCall,
    IIAG4irInsRow,
    IIAG4rrRemRow,
    IIAG4rvRetVal,
    IIAG4s4Set4GL,
    IIAG4saSetAttr,
    IIAG4seSendEvent,
    IIAG4sgSetGlobal,
    IIAG4srSetRow,
    IIAG4udUseDscr,
    IIAG4vpValParam
};


/*{
** Name:	IIAG4Init        Initialize EXEC 4GL access
**
** Description:
**	Initialize EXEC 4GL access by:
**	1. Allocating the Known Object Stack (see g4utils.c).
**	2. Setting up the pointer table (needed for W4GL to handle
**	   VMS sharable images, and used here for consistency and maybe for
**	   future ABF requirements).
**
** Inputs:
**	NONE.
**
** Returns:	
**	STATUS, as returned from IIAG4iosInqObjectStack().
**
** History:
**	15-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4Init(void)
{
    i4  current_sp, current_alloc;

    /* Pass down a pointer to the transfer table */
    IIG4xfers(&xfertbl);

    /* Initialize the Known Object stack */
    return IIAG4iosInqObjectStack(&current_sp, &current_alloc);
}
