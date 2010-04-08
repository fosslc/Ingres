/*
** Copyright (c) 1996, 2005 Ingres Corporation
**
*/

/*
**
LIBRARY = IMPDMFLIBDATA
**
*/

# include   <compat.h>
# include   <cs.h>
# include   <pc.h>
# include   <lkdef.h>

/*
** Name:	lkdata.c
**
** Description:	Global data for lk facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	15-feb-1999 (nanpr01)
**	    Added update mode lock in LK_compatibility array.
**	    Note : After the update mode lock is added the compatibility matrix 
**	    is no more orthogonal and consequently the compatibility matrix
**	    must be used consistently as LK_compatible[grant_mode] >> req_mode
**	    to check for compatibility.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	    Remove LK_my_pid.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPDMFLIBDATA
**	    in the Jamfile.
**	09-May-2005 (devjo01)
**	    Added LK_main_dsmc, LK_cont_dsmc.
**	06-jun-2005 (devjo01)
**	    Correct definition of LK_read_ppp_ctx.
**	02-jan-2007 (abbjo03)
**	    Move LK_main_dsmc, LK_cont_dsmc and LK_read_ppp_ctx to local
**	    modules.
*/

/*
** Following switch moved here from DMFCSP.C so that LK routines can benefit
** from CSP tracing setting as well. (boama01, 08/21/96)
*/
GLOBALDEF  i4	csp_debug;

/* lkrqst.c */
GLOBALDEF char	LK_compatible[8] = { 0x7f, 0x1f, 0x07, 0x2b, 0x03, 0x01 ,0x01 };

/* lkcback.c */
GLOBALDEF i4 LK_mycbt ZERO_FILL;
