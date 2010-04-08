/*
** Copyright (c) 2001, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <pc.h>
#include    <cs.h>
#include    <lk.h>
#include    <cx.h>
#include    <cxprivate.h>


/**
**
**  Name: CXDATA.C - Data used by CX faclilty.
**
**  Description:
**
**      This module contains data for the CX faclilty.
**
LIBRARY = COMPATLIBDATA
**
**  History:
**      06-feb-2001 (devjo01)
**          Created.
**	08-jan-2004 (devjo01)
**	    Linux support.
**/


/*
**	OS independent declarations
*/
/* Miscellaneous CX information maintained per process. */
GLOBALDEF CX_PROC_CB	CX_Proc_CB 	ZERO_FILL;

/* Information used by CMO if using DLM implementation */
GLOBALDEF CX_CMO_DLM_CB	CX_cmo_data[CX_MAX_CMO][CX_LKS_PER_CMO]	ZERO_FILL;

/* OS independent stat stucture. */
GLOBALDEF CX_STATS	CX_Stats	ZERO_FILL;

/* EOF:CXDATA.C */

