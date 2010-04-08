
/*--------------------------- iicx.h ----------------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iicx.h     - Utility Context Management routines. Initially for
**                     LIBQXA. Designed to cross over to "common" (a la ADF) 
**                     if this is adopted for wider use.
**  History:
**      28-May-1996 (stial01)
**          Added server_name, server_flag to IICX_XA_XN_ID (B75073)
**          IICXsetup_xa_xn_id_MACRO() init server_flag
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef  IICX_H
#define  IICX_H

/* 
**  List of context types supported by the CX routines.
**
**  Each "instance" of a context has two pieces:
**
**  a)  a "generic" CB, which has the context next/prev/type/id/state/sem info.
**
**  b)  a "context specific" CB, accessed thru the generic CB. This has all
**      the info about that context itself, as well as info about "other"
**      contexts that are scoped to it.
**
**  An "instance" of a specific type of context (e.g. XA XN context, XA RMI,
**  context, SQL session context) will live in either a hash table or a list
**  structure. The list vs hash table choice will be driven by the "volume" of
**  creates/deletes of that context type.
**
**  The "generic CB is laid out as:
**
**   ---------------- 
**   |  cx_next      |
**   |  cx_prev      |
**   |  cx_type      |
**   |  cx_id        |
**   |  cx_state     |
**   |  cx_sem       |
**   |  cx_sub_p     |
**   -----------------
**
**  We will have a "common" set of entrypoints to create/delete/find a generic
**  CB which will additionally set the context type/id and the ct_sub_p if 
**  specified on input. As we go along, we will elaborate on various kinds of
**  "context CB structures".
**
**  In the example above, if we anchored the XA XN CBs off a hash table, they
**  will look like this:
**
**                                                               
**  |---------|     -----------------                            
**  | cx_head |---> |  cx_next       |                           
**  | cx_tail |<--- |  cx_prev       |                           
**  | cx_sem  |     |  IICX_XA_XN    |                           
**  |---------|     |  IICX_XA_XN_ID |                           
**  | cx_head |     |  IICX_ACTIVE   |    ---------------------          
**  | cx_tail |     |  cx_sem        |--> |  xax_state         |   
**  | cx_sem  |     |  cx_sub_p      |    |  xax_rmi_sess_list |   
**  |---------|     ------------------    |  xax_num_rmis      | 
**                                        |  xax_num_prep_rmis | 
**                                        ---------------------- 
**
**  As another example, if we anchored RMI CBs off a list head, it will look
**  like this:
**
**    |---------|     ------------------                            
**    | cx_head |---> |  cx_next        |                           
**    | cx_tail |<--- |  cx_prev        |                           
**    | cx_sem  |     |  IICX_XA_RMI    |                           
**    |---------|     |  IICX_XA_RMI_ID |                           
**                    |  IICX_ACTIVE    |    ---------------------  
**                    |  cx_sem         |--> |  rmi_state         | 
**                    |  cx_sub_p       |    |  rmi_open_string   | 
**                    ------------------     |  rmi_flags         | 
**                                           |  rmi_dbname        | 
**                                           |  rmi_static_sid    |
**                                           |  rmi_usrargv       |
**                                           ---------------------- 
**
**
*/

#define              IICX_GENERIC_TYPE     0x01      /* unknown CB type.    */
#define              IICX_MIN_CB_TYPE      0x02      /* Min CB type value   */
#define              IICX_FE_THREAD        0x02      /* FE thread  context  */
#define              IICX_SQL_SESS         0x03      /* SQL session context */
#define              IICX_XA_XN            0x04      /* XA xn context       */
#define              IICX_XA_RMI           0x05      /* XA RMI context      */
#define              IICX_XA_XN_THRD       0x06      /* XID to THREAD assoc   */
#define              IICX_ICAS_XN          0x07      /* XID instance in ICAS  */
#define              IICX_ICAS_SVN_LIST    0x08      /* XID-SVN assoc in ICAS */
#define              IICX_MAX_CB_TYPE      0x08      /* Max CB type value   */

/*
** This constant is used for the size of a buffer that will hold any of
** the CX IDs, in an interchange form for sending over the network, or
** for writing to a file. Currently, the largest ID is an XID, that will
** be written to a file/sent over a network in the text/ascii form:
**
** "FormatID:GTridLength:BQualLength:XID data in hex format"
*/

#define   IICX_ID_ST_SIZE_MAX       512

typedef   i4        IICX_GENERIC_ID;

typedef   i4   IICX_FE_THREAD_ID; /* may need fixing. CHECK !         */

typedef   i4        IICX_SQL_SESS_ID;  /* may need fixing. CHECK !         */


typedef struct IICX_XA_XN_ID_
{
    DB_XA_DIS_TRAN_ID      *xid_p;  /* points to field below, or to an XID */
    DB_XA_DIS_TRAN_ID      xid;
    /* two fields above need to be replaced by an xid seq number. CHECK ! */
    int                    rmid;
    char                   server_name[DB_MAXNAME];
    int                    server_flag;
} IICX_XA_XN_ID;


typedef   int       IICX_XA_RMI_ID;


typedef struct IICX_XA_XN_THRD_ID_
{
    DB_XA_DIS_TRAN_ID      *xid_p;  /* points to field below, or to an XID */
    DB_XA_DIS_TRAN_ID      xid;
    /* will also have a xid seq number later on. CHECK !!! */
} IICX_XA_XN_THRD_ID;


typedef struct IICX_ICAS_XN_ID_
{
    DB_XA_DIS_TRAN_ID      *xid_p;  /* points to field below, or to an XID */
    DB_XA_DIS_TRAN_ID      xid;
} IICX_ICAS_XN_ID;


typedef struct IICX_ICAS_SVN_LIST_ID_
{
    i4     SVN;
    i4     seq_num;
    i4     tp_flags;
} IICX_ICAS_SVN_LIST_ID;


typedef struct
{
  i4          cx_type;         /* type of context */
  union
    {
      IICX_GENERIC_ID   generic_id;     /* default id until initialized  */
      IICX_FE_THREAD_ID fe_thread_id;   /* id for a  thread context      */
      IICX_SQL_SESS_ID  sql_sess_id;    /* id for an sql session context */
      IICX_XA_XN_ID     xa_xn_id;       /* id for an XA xn context       */
      IICX_XA_RMI_ID    xa_rmi_id;      /* id for an XA RMI context      */
      IICX_XA_XN_THRD_ID
			xa_xn_thrd_id;  /* id for an XID to THREAD assoc */
      IICX_ICAS_XN_ID   icas_xn_id;     /* id for an ICAS active XID cb */
      IICX_ICAS_SVN_LIST_ID   
                        icas_svn_list_id;/* id for an ICAS active SVN-XID assoc */
    } u_cx_id;
} IICX_ID;




typedef union
{
        PTR                       generic_cb_p;
        struct IICX_FE_THREAD_    *fe_thread_cb_p;    
        PTR                       sql_sess_cb_p;    /* unused, for now */
        struct IICX_XA_XN_        *xa_xn_cb_p;
        struct IICX_XA_RMI_       *xa_rmi_cb_p;
        struct IICX_XA_XN_THRD_   *xa_xn_thrd_cb_p;
        struct IICX_ICAS_XN_      *icas_xn_cb_p;
} IICX_U_SUB_CB;






/*
**   The Generic CB structure. All contexts are set up with this CB as the
**   top-level CB.
**
*/

typedef struct IICX_
{
  struct IICX_    *cx_next;
  struct IICX_    *cx_prev;
  i4              cx_state;      /* tracks the state of the context  */
#define        IICX_DELETED       0x0001     /* logically deleted context */
#define        IICX_ACTIVE        0x0002     /* context is active.        */
  IICX_ID         cx_id;                     /* id for this context       */
  IICX_U_SUB_CB   cx_sub_cb;

  /* add a semaphore variable here - post-Ingres65 */
}IICX_CB;





/*
**   The Generic List CB structure. All contexts track sub-contexts and
**   parent-contexts through the list CBs.
**
*/

typedef struct IICX_LIST_
{

  struct IICX_LIST_    *cx_next;
  IICX_ID              cx_id;               /* id for this list context type */

}IICX_LIST_CB;




/*
**   The MAIN CX CB structure, which is the ROOT of all the free lists. Each
**   specific CB type has it's own MAIN CB structure, since the CB type may
**   need a doubly-linked-list implementation, or a hash table implementation,
**   depending on the traffic for that CB type in the process.
**
**   Also, these free lists are accessed if the free lists rooted at the CB
**   type's main CB's are all exhausted.
**
*/

typedef struct IICX_MAIN_
{

  i4                    num_free_cbs;       /* secondary free list */
  IICX_CB               *cb_free_list;

  i4                    num_free_list_cbs;  /* primary free list   */
  IICX_LIST_CB          *list_cb_free_list; 

  /* will need semaphore control when we allow multiple AS threads to */
  /* get at these lists.                                              */

}IICX_MAIN_CB;




/*
**  Definitions of top-level routines/macros to setup various CX IDs. 
**  The caller can call these routines/macros first to set up a temp
**  CX ID. The temp CX ID can be passed in to all other CX routines.
**
**  Example:
**
**  foo(DB_XA_DIS_TRAN_ID *xid, i4   rmid, ...)
**  {
**     IICX_ID   cx_xa_xn_id;
**     IICX_ID   cx_xa_rmi_id;
**
**     IICXsetup_xa_xn_id_MACRO( cx_xa_xn_id, xid, rmid );
**
**     IICXsetup_xa_rmi_id_MACRO( cx_xa_rmi_id, rmid );
**     ....
**     ....
**  }
**
*/

#define  IICXsetup_fe_thread_id_MACRO( m_cx_id, m_fe_thread_id )  \
            {                                                     \
               (m_cx_id).cx_type = IICX_FE_THREAD;                \
               (m_cx_id).u_cx_id.fe_thread_id = (m_fe_thread_id); \
	    }


#define  IICXsetup_xa_xn_id_MACRO( m_cx_id, m_xid_p, m_rmid )   \
            {                                                   \
               (m_cx_id).cx_type = IICX_XA_XN;                  \
               (m_cx_id).u_cx_id.xa_xn_id.xid_p = (m_xid_p);    \
               (m_cx_id).u_cx_id.xa_xn_id.rmid  = (m_rmid);     \
               (m_cx_id).u_cx_id.xa_xn_id.xid.gtrid_length = 0; \
	       (m_cx_id).u_cx_id.xa_xn_id.server_flag = 0;      \
	    }


#define  IICXsetup_xa_rmi_id_MACRO( m_cx_id, m_rmid )           \
            {                                                   \
               (m_cx_id).cx_type = IICX_XA_RMI;                 \
               (m_cx_id).u_cx_id.xa_rmi_id = (m_rmid);          \
	    }


#define  IICXsetup_xa_xn_thrd_id_MACRO( m_cx_id, m_xid_p )           \
            {                                                        \
               (m_cx_id).cx_type = IICX_XA_XN_THRD;                  \
               (m_cx_id).u_cx_id.xa_xn_thrd_id.xid_p = (m_xid_p);    \
               (m_cx_id).u_cx_id.xa_xn_thrd_id.xid.gtrid_length = 0; \
	    }


#define  IICXsetup_icas_xn_id_MACRO( m_cx_id, m_xn_cb_p )           	\
            {                                                        	\
               (m_cx_id).cx_type = IICX_ICAS_XN;                     	\
               (m_cx_id).u_cx_id.icas_xn_id.xid_p = (m_xn_cb_p);        \
	    }

#define  IICXsetup_icas_svn_list_id_MACRO( m_cx_id, m_SVN )       	\
            {                                                        	\
               (m_cx_id).cx_type = IICX_ICAS_SVN_LIST;                 \
               (m_cx_id).u_cx_id.icas_svn_list_id.SVN =  (m_SVN);      \
	    }


/*
**  Definitions of top-level routines to create/find/delete various CBs.
**
*/

/*
** This routine gets a CB off a free list if available. It initializes the
** the CB with an input ID, gets a lock on the CB, and inserts the CB in a
** CB-specific list/hash table.
*/

FUNC_EXTERN DB_STATUS  IICXcreate_lock_cb(
                          IICX_ID       *cx_id,           /* IN */
                          IICX_U_SUB_CB *cx_sub_cb_p,     /* IN, optional */
                          IICX_CB       **cx_cb_p_p       /* OUT */
                       );

/*
** This routines finds and locks the CB in the CB-specific list/hash table.
**
*/

FUNC_EXTERN  DB_STATUS  IICXfind_lock_cb(
                           IICX_ID     *cx_id,      /* IN */
                           IICX_CB     **cx_cb_p_p  /* OUT */
                        );

/*
** This macro merely locks the CB, given a pointer to the CB.
**
*/

#define IICXlock_cb(m_cx_cb_p)     ;


/*
** This macro merely unlocks the CB, given a pointer to the CB.
**
*/
#define IICXunlock_cb(m_cx_cb_p)   ; 


/*
** This routine logically deletes the CB. Physical deletion may happen at
** the same time, or later, depending on whether the sub-contexts that
** depend on the presence of this CB have all been cleaned up and de-linked
** from this CB.
*/

FUNC_EXTERN  DB_STATUS  IICXdelete_cb(
                           IICX_ID   *cx_id       /* IN */
                        );


#endif  /* ifndef iicx_h */

/*--------------------------- end of iicx.h ------------------------------*/




