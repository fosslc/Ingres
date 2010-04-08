/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
NO_OPTIM = rs4_us5
*/

/**
** Name: AFCEXECUTE.C - ADF FE Compiled Expression Execution Module.
**
**  Description:
**	Contains routines used to execute compiled ADF expressions for use by
**	front-end programs.  Defines:
**
**          adc_execute_cx() - Execute a specified segment of a CX.
**
**	Source for this file can be found in "ade/adeexecute.c".  This file
**	is compiled by including the latter file, which defines the back-end
**	versions of the above routines:
**
**          ade_execute_cx()
**
**  History:
**	Revision 60.0  87/03  wong
**	Initial revision.
**      26-dec-96 (mosjo01)
**          Forced to NO_OPTIM rs4_us5 inorder to process "select _version()"
**          and "select user_name, dba_name from iidbconstants" as used in
**          upgradefe (iisudbms).
**/

#define ADE_FRONTEND

/* Function name maps */

#define ade_execute_cx	    afc_execute_cx
#define ade_countstar_loc   afc_countstar_loc

#include     "adeexecute.c"
