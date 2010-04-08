
/*--------------------------- iicxxa.h ------------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iicxxa.h   - Utility Context Management routines and data structures.
**                     This module contains the CBs and CX routines that drive
**                     XA related information.
**
**  History:
**	15 Oct 1993 (mhughes)
**	    History started. Added TUX_MAX_SGID_NAME_LEN constant.
**	05-Nov-1993 (larrym)
**	    Moved the above change to iicxfe.h
**      30-May-1996 (stial01)
**          Added prototype for IICXset_xa_xn_server() (B75073)
**      12-Aug-1996 (stial01)
**          Added listen_addr for TUXEDO specific (B78248)
**      06-Oct-1998 (thoda04)
**          Added prototype for IICXassign_xa_xid
*/

#ifndef  IICXXA_H
#define  IICXXA_H

/* 
**  Please see IICX.H for understanding how these structures are used/hooked in
**  to other structures.
*/

#define    IICX_XA_MAX_RESERV_XA_SESSIONS     32
#define    IICX_XA_MAX_FREE_XA_XN_CBS         32
#define    IICX_XA_MAX_FREE_XA_XN_THRD_CBS    32


/* Max connect args that can be parsed for an 'exec sql connect' 
**
** NOTE: This needs to be kept in sync with II_MAXARG in iilibq.h   
**   
** We define a separate constant there so that the IICX sub-component is 
** independent of LIBQ/LIBQXA.
*/

#define    IICX_XA_MAX_OPENSTR_TOKENS           20

/* 
**  this needs to be kept in sync with SS_MAXCONNAME in iilibq.h, but since
**  it's from the ANSI standard, it probably won't change.
*/

#define    IICX_XA_MAX_CONNECT_NAME_LEN		128

/*
**   Name: IICX_XA_RMI_CB
**
**   Description: 
**        The CX XA RMI control block has information that pertains to a 
**        specific Resource Manager Instance (RMI) that's open within the 
**        process. There is one instance of this structure that's created on
**        each IIxa_open() call that comes into LIBQXA from the TP system. 
**        There is a single list of XA RMI CBs for the process. 
**
**   History:
**	25-Aug-1992  (mani)
**          First written for Ingres65 DTP/XA project.
**	27-Aug-1993 (mhughes)
**	    Add field for tuxedo server group ID.
**	05-Nov-1993 (larrym)
**	    moved above group ID field to iicxfe.h
**	11-nov-1993 (larrym)
**	    Modified connection_name to be a pointer into usr_argv.  Now it's
**	    called connection_name_p.
**	    Added dbname_p, made it a pointer into usr_argv too.
*/

typedef struct IICX_XA_RMI_
{
    i4                      xa_state;
#define           IICX_XA_R_CLOSED        0x0001   /* RMI is closed     */
#define           IICX_XA_R_OPEN          0x0002   /* RMI is open       */
#define           IICX_XA_R_NUM_STATES    2
    char                    open_string[DB_XA_XIDDATASIZE];
                                          /* currently, null-terminated, and */
                                          /* upto 256 bytes including NULL.  */
                                          /* specified by the TP admin person*/
    char                    *connect_name_p;
                                          /* extracted from rmi open string */
                                          /* unique within RMI CB list      */
    char                    *dbname_p;	  /* name of database to connect to */
    i4                      rec_sid;      /* rec session, to node::iidbdb   */
    char                    usr_arg_buff[DB_XA_XIDDATASIZE];
                                         /* contains null-terminated strings */
                                         /* that are user args, pointed at   */
                                         /* by the usr_argv param below.     */
    i4                      usr_arg_count;
    i4                      flags_start_index;
    char                    *usr_argv[IICX_XA_MAX_OPENSTR_TOKENS];
                                         /* extract of flags for an Ingres */
                                         /* connect to backend, specified  */
                                         /* on the rmi open string         */
} IICX_XA_RMI_CB;




/*
**   Name: IICX_XA_RMI_MAIN
**
**   Description: 
**        The CX XA RMI MAIN CB has the top level information about all the
**        RMIs in the AS process. 
**
**   History:
**       16-Sep-1992  (mani)
**           First written for Ingres65 DTP/XA project.
*/

typedef struct IICX_XA_RMI_MAIN_
{
   IICX_CB      *xa_rmi_list;
   i4           num_open_rmis;        /* Number of Open RMIs in this process */
   
   /* may need semaphore access control, if multiple threads can  */
   /* access concurrently, e.g. for xa_close() in thread1, and    */
   /* xa_start() in thread2.                                      */
   
} IICX_XA_RMI_MAIN_CB;




/*
**   Name: IICX_XA_XN_THRD_CB
**
**   History:
**
**   Description: 
**        The CX XA XN THRD CB has information that pertains to the state
**        of the association (with a thread) for a specific XA XN branch.
**        There is one instance of this structure that's created for every
**        new XID that is bound into LIBQXA from the TP system. There is a
**        a single list of XA XN THRD CBs for the process. 
**
**   History:
**       24-Mar-1993  (mani, larrym)
**           First written for Ingres65 DTP/XA project.
**	06-Aug-1993 (larrym)
**	    Replaced registered_rmi_count with the three *assoc*_rmi_count
**	    counters to track the number of XID's that have been changed to
**	    the new state.
**	09-Aug-1993 (larrym)
**	    added num_reg_rmis.  If we're supporting dynamic registration
**	    this will get incremented by ax_reg, otherwise it gets
**	    set to the number of rmi's opened.
*/

typedef struct IICX_XA_XN_THRD_
{
    i4                      xa_assoc_state;
#define        IICX_XA_T_BAD_ASSOC       0      /* invalid assoc state */
#define        IICX_XA_T_NO_ASSOC        1      /* no active XA XN     */
#define        IICX_XA_T_ASSOC           2      /* active XA XN present*/
#define        IICX_XA_T_ASSOC_SUSP      3      /* suspended XA XN     */
#define        IICX_XA_T_ASSOC_SUSP_MIG  4      /* suspended migratably  */
#define        IICX_XA_T_NUM_STATES      4

    i4	       num_reg_rmis;			/* number of registered RMIs */	
						/* set by XA_START or AX_REG */
    i4         assoc_state_by_rmi[ IICX_XA_T_NUM_STATES ]; 
						/* 
						** number of rmi's currently 
						** in a given state.  Used
						** to track when all rmi's
						** in a given gbl tran have
						** moved to a new state.
						*/ 
    IICX_CB                *curr_xa_xn_cx_cb_p; /* current xid,rmid */

} IICX_XA_XN_THRD_CB;




/*
**   Name: IICX_XA_XN_THRD_MAIN_
**
**   Description: 
**        The CX XA XN THRD MAIN CB has the top level information about all the
**        XIDs and their thread association states in the AS process. 
**
**   History:
**       24-Mar-1993 (larrym, mani)
**           First written for Ingres65 DTP/XA project.
*/

typedef struct IICX_XA_XN_THRD_MAIN_
{

   IICX_CB      *xa_xn_thrd_list;
   i4           num_known_xids;      /* Number of XIDs bound to this process */
   i4           max_free_xa_xn_thrd_cbs;
   i4           num_free_xa_xn_thrd_cbs;
   IICX_CB      *xa_xn_thrd_cb_free_list;
   
   /* may need semaphore access control, if multiple threads can  */
   /* access concurrently, e.g. for xa_close() in thread1, and    */
   /* xa_start() in thread2.                                      */

} IICX_XA_XN_THRD_MAIN_CB;



/*
**   Name: IICX_XA_XN_CB
**
**   Description: 
**        The CX XA XN control block has information that pertains to a 
**        specific XID that gets bound to the process. Within the process,
**        the XA XN CB is bound to a specific *thread* within the process
**        (in active/suspended state) at any point in time. 
**
**        When we support Association Migration, we will move the XA XN CB 
**        *between* threads within the process. The XA XN CB would go from
**        a suspended state in thread1, to an active state in thread2.
**
**   History:
**	25-Aug-1992  (mani)
**          First written for Ingres65 DTP/XA project.
**	08-dec-1992 (mani)
**	    Changed states to go (1,2,3,4,...) instead of (1,2,4,8,...)
**	15-jun-1993 (larrym)
**	    Added moved xa_state from xa_xn_thrd structure to here and
**	    called it xa_assoc_state.  This way each RMI has it's own 
**	    assoc state (and can be independently suspended)
**	08-Oct-1993 (mhughes)
**          Added seqnum field for TUXEDO specific
**      12-Aug-1996 (stial01)
**          Added listen_addr for TUXEDO specific (B78248)
**
*/

typedef struct IICX_XA_XN_
{
    i4                      xa_state; /* tracks the state of the XA XN ID    */
#define           IICX_XA_X_NONEXIST      0x0001    /* logically deleted     */
#define           IICX_XA_X_ACTIVE        0x0002    /* active for this thread*/
#define           IICX_XA_X_IDLE          0x0003    /* xa_end'ed successfully*/
#define           IICX_XA_X_PREPARED      0x0004    /* prepared successfully */
#define           IICX_XA_X_ROLLBACK_ONLY 0x0005    /* rollback the xa xn    */
#define           IICX_XA_X_HEUR_COMPLETE 0x0006    /* heuristically done    */
#define           IICX_XA_X_NUM_STATES    6

    i4                      xa_assoc_state;
#define           IICX_XA_X_BAD_ASSOC       0   /* invalid assoc state   */
#define           IICX_XA_X_NO_ASSOC        1   /* not assoc'd w/any thrd*/
#define           IICX_XA_X_ASSOC           2   /* assoc'd w/a thrd      */
#define           IICX_XA_X_ASSOC_SUSP      3   /* suspended, non migrate*/
#define           IICX_XA_X_ASSOC_SUSP_MIG  4   /* suspended migratably  */
#define           IICX_XA_X_NUM_ASSOC_STATES    4
    
/*  ...	                    thread_id		       last known thrd assoc */

    i4                      ingres_state;
#define           IICX_XA_X_ING_NEW       0x0001    /* start a new DBMS sess */
#define           IICX_XA_X_ING_IN_USE    0x0002    /* re-use existing sess  */
    int                     xa_rb_value;            /* rollback return value */
    char                    connect_name[IICX_XA_MAX_CONNECT_NAME_LEN];   
					      /* maps 1:1 to an RMI/db name  */
    int                     xa_sid;       /* unique within the process   */

/* Following 2 fields for TUXEDO only */
    i4                      seq_num;      /* maps XID-AS pair to specific local transaction */
    char                    listen_addr[DB_MAXNAME];

}IICX_XA_XN_CB;





/*
**   Name: IICX_XA_XN_MAIN
**
**   Description: 
**        The CX XA XN MAIN CB has the top level information about all the
**        XA XN CBs in the AS process. For now it is structured as a 
**        doubly linked list. We will later change this to be a hash
**        table implementation if that makes sense.
**
**   History:
**       16-Sep-1992  (mani)
**           First written for Ingres65 DTP/XA project.
*/

typedef struct IICX_XA_XN_MAIN_
{
   IICX_CB                  *xa_xn_list;
   i4                       max_free_xa_xn_cbs;
   i4                       num_free_xa_xn_cbs;
   IICX_CB                  *xa_xn_cb_free_list; 

   /* CS_SEMAPHORE          sem;                                  */
   /* multiple threads could look for an XA XN CB                 */
   /* e.g. an xa_end() in thread1, and xa_start/ax_reg calls in   */
   /* other threads.                                              */
   
} IICX_XA_XN_MAIN_CB;


/*
**   Name: IICXcreate_xa_rmi_cb()
**
**   Description: 
**        The XA RMI control block is created and initialized with default
**        values to the extent possible. 
**
**   Outputs:
**       Returns: Pointer to the RMI CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_xa_rmi_cb(
                           IICX_U_SUB_CB      *xa_rmi_sub_cb_p, /* IN [opt] */
                           IICX_CB            **cx_cb_p_p     /* OUT */
                        );



 
/*
**   Name: IICXcreate_xa_rmi_sub_cb()
**
**   Description: 
**        The XA RMI SUB control block is created, if necessary. It's also
**        initialized with default values to the extent possible. 
**
**   Outputs:
**       Returns: Pointer to the RMI SUB CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**       17-Nov-1992 - First written (mani)
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_xa_rmi_sub_cb(
                           IICX_XA_RMI_CB      **xa_rmi_cb_p_p /* INOUT */
                        );




/*
**   Name: IICXfree_xa_rmi_sub_cb()
**
**   Description: 
**        The XA RMI control block is physically deleted. Any memory used by
**        the structure is freed. 
**
**
**   Inputs:
**       RMI CB            - RMI CB to be deleted. 
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful delete.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

FUNC_EXTERN   DB_STATUS  IICXfree_xa_rmi_sub_cb(
                            IICX_XA_RMI_CB   *xa_rmi_cb_p  /* IN */
                         );



/*
**   Name: IICXcheck_if_unique_rmi()
**
**   Description: 
**       Checks if the input dbname, and/or the static session identifier have
**       been previously encountered, for any of the previously opened RMIs.
**
**   Inputs:
**       dbname       - database name.
**       connect_name - connection name identifying RMI.
** 
**   Outputs:
**       Returns: DB_ERROR if this is a duplicate. DB_SUCCESS/WARN if this
**                dbname/static sid is unique.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/

FUNC_EXTERN  
DB_STATUS        IICXcheck_if_unique_rmi(
                           char                *dbname,
                           char	               *connect_name
                          );



/*
**   Name: IICXget_rmi_by_connect_name()
**
**   Description: 
**       Returns the rmid of the RMI that corresponds to this static sid.
**       The rmid <--> static sid mapping is established at the time of the
**       initialization (typically) of the Ingres Client/XA AS process.
**
**   Inputs:
**       connect_name - connection name identifying RMI.
** 
**   Outputs:
**       rmid  - of RMI that corresponds to this static sid.
**
**   Returns: DB_ERROR if there was an error and/or rmid was not found.
**
**   History:
**       16-nov-1992 - First written (mani)
*/

FUNC_EXTERN  
DB_STATUS        IICXget_rmi_by_connect_name(
                           char	               *connect_name,
                           i4                  *rmid_p
                          );



/*
**   Name: IICXcreate_xa_xn_thrd_cb()
**
**   Description: 
**        The XA XN THRD CB is created, if needed. Else it is obtained from a 
**        free list. 
**
**   Inputs:
**       None.
**  
**   Outputs:
**       Returns: Pointer to the XA XA THRD CB, if properly created.
**                NULL if there was an error. 
**
**   History:
**       24-Mar-1993 - First written (mani)
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_xa_xn_thrd_cb(
                           IICX_ID             *cx_id,          /* IN */
                           IICX_CB             **cx_cb_p_p      /* OUT */
                        );





/*
**   Name: IICXdelete_xa_xn_thrd_cb()	
**
**   Description: 
**        The XA XN THRD CB associated with a specific global transaction 
**        branch is freed. It is placed in the process-wide free list of
**        XA XN THRD CBs (rooted at the XA XN THRD MAIN CB.)
**
**   Inputs:
**        cx_cb_p  - pointer to the CB that has the  xa_xn_thrd_cb. The CB is
**                   placed in a free list.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successfully freed.
**
**   History:
**       24-Mar-1993 - First written (larrym, mani)
*/

FUNC_EXTERN   DB_STATUS  IICXdelete_xa_xn_thrd_cb(
                            IICX_CB    *cx_cb_p         /* IN */
                         );





/*
**   Name: IICXfree_xa_xn_thrd_cb()	
**
**   Description: 
**        The XA XN THRD CB is deleted. All memory used by the XA XN THRD CB
**        is freed. Any sub-lists are also freed. This is typically called at
**        process shutdown time.
**
**   Inputs:
**        xa_xn_thrd_cb_p  - the xa_xn_thrd_cb is deleted. All memory used is 
**        freed.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successfully freed.
**
**   History:
**       24-Mar-1993 - First written (mani, larrym)
*/

FUNC_EXTERN   
DB_STATUS  IICXfree_xa_xn_thrd_cb(
                            IICX_XA_XN_THRD_CB    *xa_xn_thrd_cb_p    /* IN */
                         );




/*
**   Name: IICXcreate_xa_xn_cb()
**
**   Description: 
**        The XA XN CB is created, if needed. Else it is obtained from a 
**        free list. 
**
**   Inputs:
**       None.
**  
**   Outputs:
**       Returns: Pointer to the XA XA CB, if properly created.
**                NULL if there was an error. 
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_xa_xn_cb(
                           IICX_ID             *cx_id,          /* IN */
                           IICX_U_SUB_CB       *xa_xn_sub_cb_p, /* IN, opt */
                           IICX_CB             **cx_cb_p_p   /* OUT */
                        );





/*
**   Name: IICXdelete_xa_xn_cb()	
**
**   Description: 
**        The XA XN CB associated with a specific global transaction branch
**        is freed. It is placed in the process-wide free list of XA XN CBs
**        (rooted at the XA XN MAIN CB.)
**
**   Inputs:
**        cx_cb_p  - pointer to the CB that has the  xa_xn_cb. The CB is
**                   placed in a free list.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successfully freed.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

FUNC_EXTERN   DB_STATUS  IICXdelete_xa_xn_cb(
                            IICX_CB    *cx_cb_p         /* IN */
                         );





/*
**   Name: IICXfree_xa_xn_cb()	
**
**   Description: 
**        The XA XN CB is deleted. All memory used by the XA XN CB is freed.
**        Any sub-lists are also freed. This is typically called at process
**        shutdown time.
**
**   Inputs:
**        xa_xn_cb_p  - the xa_xn_cb is deleted. All memory used is freed.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successfully freed.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

FUNC_EXTERN   
DB_STATUS  IICXfree_xa_xn_cb(
                            IICX_XA_XN_CB    *xa_xn_cb_p    /* IN */
                         );


/*
**   Name: IICXcompare_xa_xid()
**
**   Description: 
**       Compares one XA XID with another. Returns 0 if they are equal.
**
**   Inputs:
**       xid1_p      - pointer to an XA XID.
**       xid2_p      - pointer to another XA XID.
**
**   Outputs:
**       Returns: 0, if they are equal. 1 otherwise.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/

i4
IICXcompare_xa_xid(DB_XA_DIS_TRAN_ID     *xid1_p,
                   DB_XA_DIS_TRAN_ID      *xid2_p);




/*
**   Name: IICXconv_to_struct_xa_xid()
**
**   Description: 
**       Converts the XA XID from a text format to an XID structure format.
**
**   Inputs:
**       xid_text       - pointer to a text buffer that has the XID in text
**                        format. 
**       xid_str_max_len- max length of the xid string.
**       xa_xid_p       - address of an XA XID structure.
**       branch_seqnum  - address of a nat.
**       branch_flag    - address of a i4
**
**   Outputs:
**       Returns: the XA XID structure, appropriately filled in.
**                branch_seqnum - with value extracted from xid_str.
**                branch_flag   - with value extracted from xid_str.
**
**   History:
**       11-Jan-1993 - First written (mani)
**       23-Sep-1993 (iyer)
**             Two additional parameters are now passed to the function.
**             The function now extracts the branch_seqnum and branch_flag
**             from the xid_str and passes it back in these parameters.
**             
**           
*/

FUNC_EXTERN  
DB_STATUS       IICXconv_to_struct_xa_xid(
                           char                *xid_text,
                           i4             xid_str_max_len,
                           DB_XA_DIS_TRAN_ID   *xa_xid_p,
                           i4                  *branch_seqnum,
                           i4                  *branch_flag);



/*
**   Name: IICXassign_xa_xid()
**
**   Description: 
**       Assigns an XA XID from source to destination.
**
**   Inputs:
**       src_xid_p      - pointer to the source XA XID.
**       dest_xid_p     - pointer to the dest   XA XID.
**
**   Outputs:
**       Returns: the dest XA XID initialized.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/
FUNC_EXTERN  
DB_STATUS  IICXassign_xa_xid(DB_XA_DIS_TRAN_ID     *src_xid_p,
                             DB_XA_DIS_TRAN_ID     *dest_xid_p);



/*
**   Name: IICXformat_xa_xid()
**
**   Description: 
**       Formats the XA XID into the text buffer.
**
**   Inputs:
**       xa_xid_p       - pointer to the XA XID.
**       text_buffer    - pointer to a text buffer that will contain the text
**                        form of the ID.
**   Outputs:
**       Returns: the character buffer with text.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/

FUNC_EXTERN  
void        IICXformat_xa_xid(
                           DB_XA_DIS_TRAN_ID   *xa_xid_p,
                           char                *text_buffer );




/*
**   Name: IICXformat_xa_extd_xid()
**
**   Description: 
**       Formats the branch seqnum and branch flag to the end of text buffer.
**
**   Inputs:
**       branch_seqnum   - Value of the branch Seq Number
**       branch_flag     - Value of Branch Flag
**       text_buffer     - pointer to a text buffer that will contain the text
**                         form of the ID.
**   Outputs:
**       Returns: the character buffer with branch_seqnum and branch_flag
**                appended to its end.
**
**   History:
**       23-Sep-1993 - First written (iyer)
*/

FUNC_EXTERN  
void  IICXformat_xa_extd_xid(i4  branch_seqnum,
                             i4   branch_flag,
                             char *text_buffer);




/*
**   Name: IICXvalidate_xa_xid()
**
**   Description: 
**       Validates an XA XID for proper gtrid_length, bqual_length.
**
**   Inputs:
**       xid_p      - pointer to the XA XID.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if everything was all right.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/
DB_STATUS
IICXvalidate_xa_xid(DB_XA_DIS_TRAN_ID     *xid_p);





/*
**   Name: IICXget_lock_xa_xn_cb_by_rmi()
**
**   Description: 
**         Checks if the input XA XN CB pointer is set up for the right
**         RMI. If yes, then it merely locks the XA XN CB and returns.
**         If not, then it retrieves the right XA XN CB, based on the input
**         rmid, and the XID of the input XA XN CB. It switches to this new
**         XA XN CB.
**
**   Inputs:
**         rmid            - rmid of interest.
**         xa_xn_cx_cb_p_p - pointer to pointer to an XA XN CB.
** 
**   Outputs:
**         xa_xn_cx_cb_p_p - (possibly different) ptr to ptr to XA XN CB.
**
**
**   Returns: E_DB_OK/WARN if successful.
**
**   History:
**       17-nov-1992 - First written (mani)
*/

FUNC_EXTERN  
DB_STATUS        IICXget_lock_xa_xn_cx_cb_by_rmi(
                           int                 rmid,
                           IICX_CB             **xa_xn_cx_cb_p_p
                          );




/*
**   Name: IICXget_lock_curr_xa_xn_cx_cb()
**
**   Description:
**         Locates the "current" xa_xn_cx_cb based on LIBQ's notion of the
**         current session; used to mark xa_xn_cb's as ROLLBACK only
**         in the case of rogue SQL statements out of band.
**
**   Inputs:
**         xa_xn_cx_cb_p_p - pointer to pointer to an XA XN CB.
**
**   Outputs:
**         xa_xn_cx_cb_p_p - (possibly different) ptr to ptr to XA XN CB.
**
**
**   Returns: E_DB_OK/WARN if successful.
**
**   History:
**       17-Aug-1993 - First written (larrym)
*/

FUNC_EXTERN
DB_STATUS
IICXget_lock_curr_xa_xn_cx_cb(
                              IICX_CB             **xa_xn_cx_cb_p_p
                             );
/*
**   Name: IICXset_xa_xn_server()
**
**   Description:
**        Copy server name into XA XN ID
**
**   Inputs:
**       cx id
**       Server Name
**
**   Outputs:
**       None.
**
**   History:
**      30-May-1996 (stial01)
**          Created. (B75073)
*/
FUNC_EXTERN
VOID
IICXset_xa_xn_server(
	IICX_ID     *cx_id_p,
	char        *server_name);


/*
** 
**  GLOBAL structures for supporting XA CB types in the IICX modules.
**
*/

GLOBALREF     IICX_XA_RMI_MAIN_CB      *IIcx_xa_rmi_main_cb;

GLOBALREF     IICX_XA_XN_THRD_MAIN_CB  *IIcx_xa_xn_thrd_main_cb;

GLOBALREF     IICX_XA_XN_MAIN_CB       *IIcx_xa_xn_main_cb;

#endif  /* ifndef iicx_h */

/*--------------------------- end of iicx.h ------------------------------*/










