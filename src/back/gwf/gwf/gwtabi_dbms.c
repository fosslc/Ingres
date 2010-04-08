/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

/*
**
LIBRARY = IMPGWFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <gwf.h>


/**
** Name: GWTABI_DBMS.C - gateway exit table initialization for Ingres DBMS
**
** Description:
**	This file defines the gateway exit initialization table.  This
**	table defines which initialization exits need to be called during GWF
**	initialization.
**
**	This version of 'gwtabi' is for the Ingres DBMS server and includes
**	the logging (LG) and Ingres (DMF) gateways, plus the C2 Security Audit
**	gateway and the IMA gateway.
**
** History:
**      2-Jun-1989 (alexh)
**          Created for Gateway project.
**	19-apr-90 (bryanp)
**	    Re-enabled the RMS Gateway.
**	11-may-90 (alan)
**	    New filename for Ingres DBMS version (without RMS Gateway).
**	14-sep-92 (robf)
**	    Added the C2 Security Audit gateway (SXA) as id 6.
**	16-nov-1992 (bryanp)
**	    Added the IMA gateway (07).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-sep-93 (swm)
**	    Added cs.h include above other header files that need the
**	    its new CS_SID definition.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGWFLIBDATA
**	    in the Jamfile.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

FUNC_EXTERN	DB_STATUS gw07_init();
FUNC_EXTERN	DB_STATUS gw08_init();
FUNC_EXTERN	DB_STATUS gw09_init();
FUNC_EXTERN	DB_STATUS gwsxa_init();

GLOBALDEF const	GWF_ITAB Gwf_itab =
{
    NULL,	/* note that 0 is never used. gw_id 0 is illegal.... */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gwsxa_init,	/* C2 Security Audit gateway */
    gw07_init,	/* Reserved for IMA gateway */
    gw08_init,
    gw09_init
};
