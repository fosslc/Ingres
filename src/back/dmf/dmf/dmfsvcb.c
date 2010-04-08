/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <dm.h>

/**
**
**  Name: DMFSVCB.C - Definition of the DMF SVCB.
**
**  Description:
**      This routine allocates storage for the pointer to the
**	DMF SVCB.
**
**
**  History:    
**     03-mar-1987 (Derek)
**          Created for Jupiter.
**      13-jul-1993 (ed)
**	    unnest dbms.h
**	[@history_template@]...
**/


/*
** Definition of all global variables owned by this file.
*/

GLOBALDEF DM_SVCB     *dmf_svcb	ZERO_FILL;	    /* DMF SERVER CONTROL BLOCK. */
