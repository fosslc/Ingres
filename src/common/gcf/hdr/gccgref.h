/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: GCCGREF.H - Global data references for GCC routines.
**
** Description:
**
**      This module contains the GLOBALREF declarations needed by GCC routines.
**
** History: $Log-for RCS$
**      05-Jan-88 (jbowers)
**          Initial module creation
**	15-Mar-90 (seiwald)
**	    Removed IIGCc_inxl_tbl, which went to gccpdc.c, and IIGCc_pct
**	    which doesn't exist.
**	09-Feb-96 (rajus01)
**	    Added revision level for Protocol Bridge(GCB).
**/

/*
** References to global variables declared in other C files.
*/

GLOBALREF   GCC_GLOBAL		*IIGCc_global;	    /* GCC global data area */
GLOBALREF   char		IIGCc_rev_lvl[];    /* GCC revision level */
GLOBALREF   char		IIGCb_rev_lvl[];    /* GCB revision level */
						   
