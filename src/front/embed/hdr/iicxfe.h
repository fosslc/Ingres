
/*--------------------------- iicxfe.h -------------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iicxfe.h - This module has context management routines, data
**                   structures for managing and manipulating thread context
**                   CBs in the front-end (LIBQ/LIBQXA) space.
*/

#ifndef  IICXFE_H
#define  IICXFE_H

/*
**   Name: IICX_FE_THREAD_CB
**
**   Description: 
**        The CX FE thread control block has information that's scoped to a
**        specific thread within the FE process. For now, it contains the
**        list of global transaction branches (XIDs) that are bound to the
**        thread. Each active/suspended/idle XID has a corresponding 
**        connection or gca association to the back-end DBMS process. 
**
**        Post-Ingres65 Goals
**        -------------------
**        After Ingres-65 it is a goal to "grow" this structure so that it 
**        also contains the Ingres/SQL session control blocks. That will help
**        us move towards a thread-safe LIBQ.
**
**   History:
**	25-Aug-1992  (mani)
**           First written for Ingres65 DTP/XA project.
**	08-dec-1992 (mani)
**	    Changed xa_state to be integral instead of bit fields (i.e.,
**	    the states are 1,2,3,4 instead of 1,2,4,8).
**      09-dec-1992 (mani)
**          To allow multiple RMIs in a single AS that can be xa_start(ed)
**          or ax_reg(ed), added a "registered_rmi_count" field.
**	06-Aug-1993 (larrym)
**	    changed curr_xa_xn_cx_cb_p to curr_xa_xn_thrd_cx_cb_p, so 
**	    we can keep track of the current XID without concern for rmid
**	05-Nov-1993 (larrym)
**	    Added  server_group and IICX_FE_TUX_*SGID_SIZE to FE_THREAD_MAIN
**	10-Oct-1993 (mhughes)
**	    Added flag to indicate to TUXEDO code that we're doing run_all
**	    tests. If set tpreturn calls a replaced by real returns.
**	15-Dec-1993 (mhughes)
**	    Added fields for process ID & type strings in fe_thread_main_cb.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

typedef struct IICX_FE_THREAD_
{
    i4                     ingres_state;
/* These need to be bit fields */
#define        IICX_XA_T_ING_IDLE        0x0001  /* idle in XA mode          */
#define        IICX_XA_T_ING_INTERNAL    0x0002  /* internal to LIBQ/LIBQXA  */
#define        IICX_XA_T_ING_ACTIVE_PEND 0x0004  /* active, set_sql pending  */
#define        IICX_XA_T_ING_ACTIVE      0x0008  /* fully active,set_sql done*/
#define        IICX_XA_T_ING_APPL_ERROR  0x0010  /* appl esql coding error   */
#define        IICX_XA_T_ING_OUTSIDE     0x0020  /* work done outside XA     */

#define        IICX_XA_T_ING_ERROR  (IICX_XA_T_ING_APPL_ERROR | \
                                    IICX_XA_T_ING_OUTSIDE)

    i4                     rec_scan_in_progress; /* toggled by TMSTARTRSCAN  */

    IICX_CB                *curr_xa_xn_thrd_cx_cb_p;  
                             /* pointer to the currently "active" or      */
                             /* last "suspended" XID for this thread.     */

    /* the following field(s) will be added when we go multi-threaded    */
    /* CS_SEMAPHORE            sem;                                      */
    /* ...                     mem_zone;                                 */
    /* ...                     thread_id;                                */

}IICX_FE_THREAD_CB;





/*
**   Name: IICX_FE_THREAD_MAIN
**
**   Description: 
**        The CX FE THREAD MAIN CB has the top level information about all the
**        threads in the AS process. For now it is structured as a 
**        doubly linked list.
** 
**   Note: For Ingres65, we'll only have one thread in the AS process that is
**         "known" and manipulated in the IICX/LIBQ/LIBQXA space.
**
**   History:
**      16-Sep-1992  (mani)
**          First written for Ingres65 DTP/XA project.
**	13-jan-1993 (larrym)
**	    Added structure members to support process-wide XA tracing.
**      04-Oct-93 (mhughes)
**          Added tuxedo_switch field to structure. Fn pointer to tuxedo 
**	    top-level switching routine
**	09-Nov-1993 (larrym)
**	    moved SVN to iitxgbl.h
**	15-Dec-1993 (mhughes)
**	    Store the pid in character form if we are tracing process-by process.
*/

#define IICX_XA_MAIN_PID_LEN	8

typedef struct IICX_FE_THREAD_MAIN_
{
   IICX_CB                  *fe_thread_list;
   i4			    fe_main_flags;
#define IICX_XA_MAIN_NOFLAGS  0x0000 /* no MAIN flags specified    */
#define IICX_XA_MAIN_TRACE_XA 0x0001 /* trace XA calls and queries */

   FILE			    *fe_main_trace_file; /* trace file handle */

   /* Following 3 fields are used for Tuxedo only */
   i4                       (*fe_main_tuxedo_switch)();   
				           /* I/face to Tuxedo-specific code */
   char			    process_id[IICX_XA_MAIN_PID_LEN];
   char			    process_type[IICX_XA_MAIN_PID_LEN];

   i4                       num_free_fe_thread_cbs;
   IICX_CB                  *fe_thread_cb_free_list;

   /* CS_SEMAPHORE          sem;                                  */
   /* multiple threads could look for a free FE Thread CB         */
   /* e.g. an xa_end() in thread1, and xa_start/ax_reg calls in   */
   /* other threads.                                              */
   
} IICX_FE_THREAD_MAIN_CB;





/*
**   Name: IICXcreate_fe_thread_cb()
**
**   Description: 
**        The FE Thread control block is created and initialized when an
**        application thread is created and initialized for:
**
**        -- accepting XA calls from the TP system. (Ingres65)
**        -- accepting ESQL calls from the application on a thread-basis.
**           (Post-Ingres65).
**
**        Currently, we do not support multiple application threads of 
**        control. However, this design will allow evolution towards
**        multi-threaded Ingres clients/ASs in a fairly straightforward
**        manner.
**
**   Inputs:
**       Nothing.
**
**   Outputs:
**       Returns: status, as well as pointer to the Thread CB, if properly
**       created.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_fe_thread_cb(
                        IICX_U_SUB_CB       *fe_thread_sub_cb_p, /* IN,opt */
                        IICX_CB             **cx_cb_p_p
                     );


/*
**   Name: IICXget_fe_thread_cb()
**
**   Description:
**        The FE THREAD control block for this thread is retrieved.
**
**   Inputs:
**      thread_key              - key to the thread desired - may be NULL
**
**   Outputs:
**      cx_cb_p_p               - pointer to the Thread CB, if properly
**                                retrieved.
**      DB_STATUS (return)      - success or failure
**
**   History:
**      25-feb-1993 (larrym)
**          written.
*/
DB_STATUS
IICXget_fe_thread_cb(
	    PTR                     thread_key,           /* IN, may be NULL */
	    IICX_CB                 **fe_thread_cx_cb_p_p /* INOUT */
	   );
/*
**   Name: IICXget_lock_fe_thread_cb()
**
**   Description: 
**        The FE Thread control block is retrieved based on a thread key
**        that's input to this routine, or a key previously associated
**        with the thread, and available through an operating system call
**        or POSIX thread library call.
**
**   Inputs:
**	thread_key		- key to the thread desired - may be NULL
**
**   Outputs:
**	cx_cb_p_p		- pointer to the Thread CB, if properly
**				  retrieved. 
**	DB_STATUS (return)	- success or failure
**
**   History:
**       27-Oct-1992 - First written (mani)
*/

FUNC_EXTERN  DB_STATUS  IICXget_lock_fe_thread_cb(
                        PTR                 thread_key,  /* IN, may be NULL */
                        IICX_CB             **cx_cb_p_p  /* INOUT */
                     );


/*
**   Name: IICXfree_fe_thread_cb()
**
**   Description: 
**        The FE Thread CB of a specific application thread is physically
**        deleted. This is typically done as part of shutdown of the Ingres
**        client (AS) process. 
**
**
**   Inputs:
**       fe_thread_cb_p    - pointer to the FE Thread CB.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successfully deleted.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

FUNC_EXTERN  DB_STATUS  IICXfree_fe_thread_cb(
                           IICX_FE_THREAD_CB    *fe_thread_cb_p     /* IN */
                        );


/*
**  
** GLOBAL STRUCTURES for thread CBs.
**
*/

#ifndef VMS
GLOBALREF   IICX_FE_THREAD_MAIN_CB    *IIcx_fe_thread_main_cb;
#endif  /* ifndef VMS */

/* Used for trace handling output */ 
#define IIXATRCENDLN   ERx("------------------------------------------------\n")

#endif  /* ifndef IICXFE_H */

/*--------------------------- end of iicxfe.h ------------------------------*/

