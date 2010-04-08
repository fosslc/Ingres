/*
**  Copyright (c) 1996, 2002 Ingres Corporation
*/

/*
**  Name: hdydata.c -    HANDY Data
**
**  Description
**      File added to hold all GLOBALDEFs in HANDY facility.
**
LIBRARY = COMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	12-Jun-98 (rajus01)
**	    Added GC_associd_sem.
**	05-jun-2002 (somsa01)
**	    Removed ME_tag_sem in favor of MEtaglist_mutex.
**
*/
#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <st.h>
#include   <cs.h>

GLOBALDEF	CS_SEMAPHORE	 CL_misc_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 CL_acct_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 FP_misc_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 GC_associd_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 GC_misc_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 GC_trace_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 TR_context_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 NM_loc_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 GCC_ccb_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 LGK_misc_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 ME_stream_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 ME_segpool_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 *ME_page_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 DI_sc_sem ZERO_FILL;
GLOBALDEF       CS_SEMAPHORE     PM_misc_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 DMF_misc_sem ZERO_FILL;
GLOBALDEF	CS_SEMAPHORE	 SCF_misc_sem ZERO_FILL;
