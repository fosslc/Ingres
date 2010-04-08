/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name: AFCDEBUG.C - ADF FE Compiled Expression Debugging Module.
**
**  Description:
**	Contains routines used to test ADF expressions as used by front-end
**	programs.  Defines:
**
**	    afc_cx_print() - Print the CX.
**	    afc_cxbrief_print() -   Print the CX in a compact form.
**
**	Source for this file can be found in "ade/adedebug.c".  This file
**	is compiled by including the latter file, which defines the back-end
**	versions of the above routines:
**
**	    ade_cx_print()
**	    ade_cxbrief_print()
**
**  History:
**	Revision 60.0  87/03  wong
**	Initial revision.
**/

#define ADE_FRONTEND

/* Function name maps */

#define ade_cx_print	    afc_cx_print
#define ade_cxbrief_print   afc_cxbrief_print

#include     "adedebug.c"

