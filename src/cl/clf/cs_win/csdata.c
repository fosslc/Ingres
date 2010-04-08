/*
** Copyright (c) 1996, 2004 Ingres Corporation
*/

/*
**  csdata.c -  CS Data
**
**  Description
**      File added to hold all GLOBALDEFs in CS facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	21-feb-1996 (canor01)
**	    Remove unused CsThreadsRunningEvent.
**	21-feb-1996 (canor01)
**	    Add system-wide shared memory mutex for CS_SEM_MULTI semaphores.
**	01-may-1996 (canor01)
**	    Change default for Cs_desched_usec_sleep.
**	23-sep-1996 (canor01)
**	    Add globals from csmonitor.c
**	03-mar-1997 (canor01)
**	    Add global semaphore MO classes here.
**	14-may-97 (mcgem01)
**	    Moved global definition of CS_classes here.
**	16-jun-1997 (canor01)
**	    Add CsMultiTaskSem.
**	14-aug-1998 (canor01)
**	    Removed dependency on generated erglf.h file.
** 	03-dec-1998 (wonst02)
** 	    Added GLOBALREF for CS_samp_thread_index; added include "classsamp.h".
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	
**	25-aug-1999 (somsa01)
**	    Added CSsamp_stopping, which is true if the Sampler thread is
**	    about to shut down.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Added exp.clf.unix.cs.scb_thread_id and CS_thread_id_get
**	    method to return OS thread id.
**	10-oct-2000 (somsa01)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	22-jan-2004 (somsa01)
**	    Removed ConditionCriticalSection.
**	23-jan-2004 (somsa01)
**	    Added CSmo_num_active_get(), CSmo_hwm_active_get().
*/
#include    <compat.h>
#include    <sp.h>
#include    <cs.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <me.h>
#include    <mo.h>
#include    <nm.h>
#include    <pc.h>
#include    <lk.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <pc.h>
#include    <csinternal.h>
#include    <stdio.h>
#include    <machconf.h>

#include    "csmgmt.h"
#include    "cssampler.h"


static    CS_SEMAPHORE     CS_ME_sem[1];

GLOBALDEF CS_SYSTEM        Cs_srv_block ZERO_FILL;
GLOBALDEF CS_SCB           Cs_main_scb ZERO_FILL;
GLOBALDEF CS_SCB           Cs_known_list_hdr ZERO_FILL;
GLOBALDEF CS_SEMAPHORE     Cs_known_list_sem ZERO_FILL;
GLOBALDEF PTR              Cs_old_last_chance;
GLOBALDEF i4               nonames_opt ZERO_FILL;
GLOBALDEF CRITICAL_SECTION SemaphoreInitCriticalSection;
GLOBALDEF i4               Cs_numprocessors = DEF_PROCESSORS; 
GLOBALDEF i4     	   Cs_max_sem_loops = DEF_MAX_SEM_LOOPS;
GLOBALDEF i4     	   Cs_desched_usec_sleep = 20000;


GLOBALDEF i4               Cs_lastquant = 0;
GLOBALDEF i4 		   dio_resumes = 0;

GLOBALDEF HANDLE CsMultiTaskEvent;
GLOBALDEF HANDLE CsMultiTaskSem;
GLOBALDEF HANDLE CS_init_sem ZERO_FILL;

GLOBALREF MO_INDEX_METHOD CS_scb_index;
GLOBALREF MO_GET_METHOD   CS_scb_self_get;
GLOBALREF MO_GET_METHOD   CS_scb_state_get;
GLOBALREF MO_GET_METHOD   CS_scb_sync_obj_get;
GLOBALREF MO_GET_METHOD   CS_scb_thread_type_get;
GLOBALREF MO_GET_METHOD   CS_scb_mask_get;
GLOBALREF MO_GET_METHOD	  CS_scb_cpu_get;
GLOBALREF MO_GET_METHOD	  CS_scb_inkernel_get;
GLOBALREF MO_GET_METHOD	  CS_thread_id_get;
GLOBALREF MO_GET_METHOD   CSmo_bio_done_get;
GLOBALREF MO_GET_METHOD   CSmo_bio_waits_get;
GLOBALREF MO_GET_METHOD   CSmo_bio_time_get;
GLOBALREF MO_GET_METHOD   CSmo_dio_done_get;
GLOBALREF MO_GET_METHOD   CSmo_dio_waits_get;
GLOBALREF MO_GET_METHOD   CSmo_dio_time_get;
GLOBALREF MO_GET_METHOD   CSmo_lio_done_get;
GLOBALREF MO_GET_METHOD   CSmo_lio_waits_get;
GLOBALREF MO_GET_METHOD   CSmo_lio_time_get;
GLOBALREF MO_GET_METHOD   CSmo_num_active_get;
GLOBALREF MO_GET_METHOD   CSmo_hwm_active_get;

#include "classint.h"

GLOBALDEF i4 		IIcs_mo_sems = TRUE; 		/* attach sems? */

#ifdef USE_MO_WITH_SEMS
GLOBALREF MO_GET_METHOD	CS_sem_type_get;
GLOBALREF MO_GET_METHOD	CS_sem_scribble_check_get;
GLOBALREF MO_GET_METHOD	CS_sem_name_get;
#endif /* USE_MO_WITH_SEMS */
GLOBALREF MO_GET_METHOD CS_scb_bio_get;
GLOBALREF MO_GET_METHOD CS_scb_dio_get;
GLOBALREF MO_GET_METHOD CS_scb_lio_get;

#include "classscb.h"

#include "classsem.h"

#include "classcnd.h"

GLOBALREF MO_SET_METHOD   CS_breakpoint_set;
GLOBALREF MO_SET_METHOD   CS_stopcond_set;
GLOBALREF MO_SET_METHOD   CS_stopserver_set;
GLOBALREF MO_SET_METHOD   CS_crashserver_set;
GLOBALREF MO_SET_METHOD   CS_shutserver_set;
GLOBALREF MO_SET_METHOD   CS_rm_session_set;
GLOBALREF MO_SET_METHOD   CS_suspend_session_set;
GLOBALREF MO_SET_METHOD   CS_resume_session_set;
GLOBALREF MO_SET_METHOD   CS_debug_set;

#include "classmon.h"

GLOBALREF MO_INDEX_METHOD CS_samp_thread_index;

#include "classsamp.h"

/* csmonitor.c */
GLOBALDEF HANDLE                hCsSamplerSem = NULL;
GLOBALDEF CSSAMPLERBLKPTR       CsSamplerBlkPtr = NULL;
GLOBALDEF bool			CSsamp_stopping = FALSE;

GLOBALDEF MO_CLASS_DEF    CS_classes[] =
{
        /* Nothing here... */
        {0}
};

