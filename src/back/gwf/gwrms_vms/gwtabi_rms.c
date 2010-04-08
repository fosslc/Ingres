/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <gwf.h>


/**
** Name: GWTABI_RMS.C - gateway exit table initialization for RMS Gateway Server
**
** Description:
**	This file defines the VMS gateway exit initialization table.  This
**	table defines which initialization exits need to be called during GWF
**	initialization.
**
**	This version of 'gwtabi' is for the RMS Gateway server and includes
**	only the RMS Gateway exit.  gwtabi_dbms.c contains the 'gwtabi'
**	module used in the Ingres DBMS server.
**
** History:
**      2-Jun-1989 (alexh)
**          Created for Gateway project.
**	19-apr-90 (bryanp)
**	    Re-enabled the RMS Gateway.
**	11-may-90 (alan)
**	    Separate version for RMS Gateway image.
**      06-nov-92 (schang)
**          add dmrcb before dmtcb  
**      11-aug-93 (ed)
**          unnest dbms.h
**	15-sep-93 (swm)
**	    Added cs.h include above other header files that use
**	    its new CS_SID definition.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

FUNC_EXTERN	DB_STATUS gw02_init();

GLOBALDEF const	GWF_ITAB Gwf_itab =
{
    NULL,	/* note that 0 is never used. gw_id 0 is illegal.... */
    NULL,
    gw02_init,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
