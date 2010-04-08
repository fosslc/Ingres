/*
** Copyright (c) 1989, 1999 Ingres Corporation
*/

#include <compat.h>
#include <clconfig.h>
#include <meprivate.h>

/**
** Name: cslocal.h - defines completely private to CS.
**
** Description:
**      This file contains the definitions of datatypes and constants
**      used exclusively within the CS module.
**
** History:
**	27-jun-89 (daveb)
**		Created
**	15-may-97 (mcgem01)
**		Fix up function prototype - CS_swuser returns STATUS.
**	08-dec-1997 (canor01)
**	    Add defines from PSAPI.H (from the Platform SDK) to allow 
**	    calling GetProcessMemoryInfo().
**	07-nov-1998 (canor01)
**	    Add reference to the GC async timeout completion TLS key.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      09-Feb-2009 (smeke01) b119586
**          Added FUNC_EXTERN for CSMT_get_thread_stats().
**	28-Jan-2010 (horda03) Bug 121811
**          Enable Evidence Sets on Windows.
**/

/*
**  Defines of other constants.
*/

/*
** The maximum number of file descriptors the server will use
** internally.  The value of (CL_NFILE() - CS_RESERVED_FDS) is an
** approximation of the number of file descriptors that will be
** left for other purposes.  With the current implementation of GCA,
** this is the limit on the number of user connections that can be made.
**
** The value here is a "best guess" -- it changes depending on the amount
** of logging and tracing going on.
*/

#define                 CS_RESERVED_FDS	14

/* csinterface.c */

GLOBALREF CS_SYSTEM	      Cs_srv_block;
GLOBALREF CS_SCB	      Cs_queue_hdrs[CS_LIM_PRIORITY];
GLOBALREF CS_SCB	      Cs_known_list_hdr;
GLOBALREF CS_SCB	      Cs_to_list_hdr;
GLOBALREF CS_SCB	      Cs_wt_list_hdr;
GLOBALREF CS_SCB	      Cs_as_list_hdr;
/* GLOBALREF CS_STK_CB	      Cs_stk_list_hdr; */
GLOBALREF CS_SCB	      Cs_idle_scb;
GLOBALREF CS_SCB	      Cs_repent_scb;
/* GLOBALREF CS_ADMIN_SCB	      Cs_admin_scb; */
GLOBALREF i4 		      CSdebug;


/* cshl.c */

GLOBALREF i4 		      Cs_incomp;

/* func externs -- should be ptorotyped later */

FUNC_EXTERN CS_SCB *CS_find_scb( /* i4  sid */ );

FUNC_EXTERN CS_SCB *CS_xchg_thread();
FUNC_EXTERN STATUS CS_admin_task();
FUNC_EXTERN STATUS CS_event_shutdown();
FUNC_EXTERN STATUS CS_find_events();
FUNC_EXTERN STATUS CS_parse_option();
FUNC_EXTERN STATUS CS_set_server_connect();
FUNC_EXTERN VOID   CS_setup();
FUNC_EXTERN STATUS CS_tas();
FUNC_EXTERN STATUS cs_handler();
FUNC_EXTERN VOID CS_dump_stack();
FUNC_EXTERN VOID CS_eradicate();
FUNC_EXTERN VOID CS_fmt_scb();
FUNC_EXTERN VOID CS_mkframe();
FUNC_EXTERN VOID CS_mo_init(void);
FUNC_EXTERN VOID CS_move_async();
FUNC_EXTERN STATUS CS_swuser();
FUNC_EXTERN VOID CS_toq_scan();
FUNC_EXTERN VOID CSdump_statistics();
FUNC_EXTERN VOID CSsigchld();
FUNC_EXTERN VOID iiCLintrp();
FUNC_EXTERN VOID CSdiag_server_link();
FUNC_EXTERN i4  CS_quantum();
FUNC_EXTERN i4  CSsigterm();
FUNC_EXTERN i4  CSslave_handler();
FUNC_EXTERN VOID CS_breakpoint(); 
FUNC_EXTERN STATUS CSMT_get_thread_stats();

FACILITYREF ME_TLS_KEY          gc_compl_key;

/* defines from PSAPI.H to allow calling GetProcessMemoryInfo() */

typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb;
    DWORD PageFaultCount;
    DWORD PeakWorkingSetSize;
    DWORD WorkingSetSize;
    DWORD QuotaPeakPagedPoolUsage;
    DWORD QuotaPagedPoolUsage;
    DWORD QuotaPeakNonPagedPoolUsage;
    DWORD QuotaNonPagedPoolUsage;
    DWORD PagefileUsage;
    DWORD PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;
typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;

BOOL
WINAPI
GetProcessMemoryInfo(
    HANDLE Process,
    PPROCESS_MEMORY_COUNTERS ppsmemCounters,
    DWORD cb
    );
