/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name: AFCCOMPILE.C - ADF FE Compiled Expression Compilation Module.
**
**  Description:
**	Contains routines used to compile ADF expressions for use by front-end
**	programs.  Defines:
**
**          afc_cx_space() - Estimate size of the CX to be compiled.
**          afc_bgn_comp() - Begin the compilation of an expression.
**          afc_end_comp() - End the compilation of an expression.
**          afc_instr_gen() - Compile an instruction into the CX.
**	    afc_cxinfo() - Return various pieces of information about a CX.
**          afc_inform_space() - Inform ADE that the CX size has changed.
**
**	Source for this file can be found in "ade/adecompile.c".  This file
**	is compiled by including the latter file, which defines the back-end
**	versions of the above routines:
**
**	    ade_cx_space()
**	    ade_bgn_comp()
**	    ade_end_comp()
**	    ade_instr_gen()
**	    ade_cxinfo()
**	    ade_inform_space()
**
**  History:
**	Revision 60.0  87/03  wong
**	Initial revision.
**/

#define ADE_FRONTEND

/* Function name maps */

#define ade_cx_space	    afc_cx_space
#define ade_bgn_comp	    afc_bgn_comp
#define ade_end_comp	    afc_end_comp
#define ade_instr_gen	    afc_instr_gen
#define ade_cxinfo	    afc_cxinfo
#define ade_inform_space    afc_inform_space

#include     "adecompile.c"
