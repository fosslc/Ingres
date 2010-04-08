/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: LGKSHARE.H - defines for routines shared by lg and lk
**
** Description:
**	The unix specific implementation of LG and LK shares routines useful
**	to both modules.  Defines needed locally by LG and LK are maintained
**	here.  
**
** History: 
**      25-jan-88 (mmm)
**         Created.
**      09-nov-88 (mikem)
**	    added status fields to LGK_ACB.
**	12-feb-89 (mikem)
**	    added FUNC_EXTERNS for LGK_*() routines, and defines for use by
**	    callers to LGK_resource_alloc(), also removed LGK_MEM definition
**	    and put it into local hdr (lgklocal.h) in lgk directory.
**	10-aug-89 (mikem)
**	    Added two members to the LGK_ACB structure to support value block
**	    information passing.
**	17-jan-90 (mikem)
**	    Added defines for parameter to LGK_open: LGKD_SERVER, LGKD_RCP, 
**	    LGKD_NONSERVER.
**	18-jan-90 (mikem) integrated following change: 3-oct-89 (daveb)
**          Remove superfluous "typedef" before struct.
**          Causes noise warning from some compilers.
**	04-feb-90 (mikem)
**	    Added defines for LGK_{START,FINISH}... macro's for performance
**	    elapsed time information gathering.
**	25-sep-1991 (mikem) integrated following change: 19-aug-91 (rogerk)
**	    Added statistic for counting number of times the RCP has been
**	    signaled to write a log buffer: LGKS_05_RCP_SIGNAL_WRITE.
**	13-nov-1991 (bryanp)
**	    Added LGK_ID8_INLINE_HDR_WRITE for dual logging support.
**	20-jun-95 (emmag)
**	    NT complains because it doesn't have a definition for EXT *
**	    Since these are vms specific functions we're going to ifdef
**	    them out for now.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _LGK_ACB LGK_ACB;


/*
**  Forward and/or External function references.
*/

# ifndef NT_GENERIC
FUNC_EXTERN EXT *	vms_allocate_ext();
FUNC_EXTERN VOID	vms_deallocate_ext();
FUNC_EXTERN STATUS	vms_initialize_mem();
# endif /* NT_GENERIC */
FUNC_EXTERN STATUS	LGK_initialize();
FUNC_EXTERN STATUS	LGK_destroy();
FUNC_EXTERN VOID	LGK_lk_get_addr();
FUNC_EXTERN VOID	LGK_lg_get_addr();
FUNC_EXTERN VOID	LGK_rundown();
FUNC_EXTERN STATUS	LGK_qast();
FUNC_EXTERN STATUS	LGK_sleep();
FUNC_EXTERN VOID	LGK_wakeup();
FUNC_EXTERN STATUS	LGK_mutex();
FUNC_EXTERN STATUS	LGK_unmutex();
FUNC_EXTERN VOID	LGK_event_handler();
FUNC_EXTERN VOID	LGK_resource_alloc();


/*
**  Defines of other constants.
*/

/* parameters to LGK_resource_alloc() identifying type of memory being sized */

#define                 LGK_OBJ0_LK_LLB		0x00
#define                 LGK_OBJ1_LK_LLB_TAB	0x01
#define                 LGK_OBJ2_LK_BLK		0x02
#define                 LGK_OBJ3_LK_BLK_TAB	0x03
#define                 LGK_OBJ4_LK_LKH		0x04
#define                 LGK_OBJ5_LK_RSH		0x05
#define                 LGK_OBJ6_LG_BUFFER	0x06
#define                 LGK_OBJ7_LG_LXB		0x07
#define                 LGK_OBJ8_LG_LXB_TAB	0x08
#define                 LGK_OBJ9_LG_LDB		0x09
#define                 LGK_OBJA_LG_LDB_TAB	0x0a
#define                 LGK_OBJB_LK_ACB		0x0b
#define                 LGK_OBJC_LGK_ACB	0x0c
#define                 LGK_NUM_OBJECTS		0x0d

/* parameter's used when calling LGK_open(). */

#define                 LGKD_SERVER		0x00000001
#define                 LGKD_RCP		0x00000002
#define                 LGKD_NONSERVER		0x00000004


/*}
** Name: LGK_ACB - LGK ast control block
**
** Description:
**	structure used to maintain queue of ast's to simulate VMS ast delivery
**	mechanism.  Contains queing information and information to execute
**	procedures in processes comunicating through shared memory.
**
** History:
**      05-apr-88 (mikem)
**	    written.
**      09-nov-88 (mikem)
**	    added status fields so that we could mimic vms ast mechanism which
**	    provides status return to caller through a pointer.
**	10-aug-89 (mikem)
**	    Added two members to the LGK_ACB structure to support value block
**	    information passing.
**	18-jan-90 (mikem) integrated following change: 3-oct-89 (daveb)
**          Remove superfluous "typedef" before struct.
**          Causes noise warning from some compilers.
**	13-nov-1991 (bryanp)
**	    Added LGK_ID8_INLINE_HDR_WRITE for dual logging support.
*/
struct _LGK_ACB
{
    LGK_ACB         *lacb_next;         /* next acb on queue */
    LGK_ACB         *lacb_prev;         /* prev acb on queue */
    PID		    lacb_source;	/* pid of procedure which queue'd it */
    PID		    lacb_dest;		/* pid of procedure to execute it */
    VOID	    (*lacb_ast)();	/* func to exec in case of foce abort */
    i4	    lacb_func_id;	/* numeric id of function to call */
#define                 LGK_ID1_WRITE_BLOCK    		1
#define                 LGK_ID2_WRITE_COMPLETE  	2
#define                 LGK_ID3_PGYBACK_WRITE   	3
#define                 LGK_ID4_LGK_WAKEUP    		4
#define                 LGK_ID5_FORCE_ABORT    		5
#define                 LGK_ID6_CS_RESUME		6
#define                 LGK_ID7_LGK_NONSERVER_RESUME	7
#define			LGK_ID8_INLINE_HDR_WRITE	8
#define                 LGK_ID_MAXNUM    		8
    i4	    lacb_param1;	/* parameter to the function */
    i4	    lacb_param2;	/* parameter to the function */

#define			LGK_NO_SERVER_NUMBER	-1
    i4	    lacb_param3;	/* parameter to the function */
    STATUS	    lacb_status;	/* status of executed acb */
    STATUS	    *lacb_addr_status;	/* address in calling process to write
					** status apon completion of async
					** events.
					*/
    i4	    *lacb_addr_value;	/* address in calling process to write
					** value block values upon return.
					*/
    i4	    lacb_value[3];	/* place to store the values */
};

/*}
** Name: LGK_START_TIME_MACRO, LGK_FINISH_TIME_MACRO - et. performance info
**
** Description:
**	Macro's to gather elapsed time information about parts of the 
**	logging, locking, and lgk system which suspend activity waiting for
**	something to happen.  These macro's only add code if the constant
**	xLGK_PERFORMANCE_TUNING is defined.
**
**	LGK_START_TIME_MACRO() gets the address of a SYSTIME structure as it's
**	only argument, and should be called prior to whatever call you wish to
**	measure the elapsed time of.
**
**	LGK_FINISH_TIME_MACRO() calls the routine LGK_finish_time_macro() when
**	xLGK_PERFORMANCE_TUNING is defined.  See comments of that routine 
**	regarding it's arguments and actions.
**
** History:
**      04-feb-90 (mikem)
**	    created.    
*/
# ifdef xLGK_PERFORMANCE_TUNING

#     define LGK_START_TIME_MACRO(time1)	TMet(time1)

#     define LGK_FINISH_TIME_MACRO(a1, a2, a3)	LGK_finish_time(a1, a2, a3)

# else


#     define LGK_START_TIME_MACRO(time1)

#     define LGK_FINISH_TIME_MACRO(a1, a2, a3)

# endif /* xLGK_PERFORMANCE_TUNING */

/* defines used in calls to LGK_FINISH_TIME_MACRO */

# define	LGKS_00_II_NAP			0
# define	LGKS_01_CSSUSPEND_FROM_LGK	1
# define	LGKS_02_CSCP_PSEM		2
# define	LGKS_03_LOG_WRITE		3  /* not used yet */
# define	LGKS_04_DI_WRITE		4  /* not used yet */
# define	LGKS_05_RCP_SIGNAL_WRITE	5
# define	LGKS_STAT_MAX			6
