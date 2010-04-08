/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMACB.H - Typedefs for DMF Security Audit support.
**
** Description:
** 
**      This file contains the control block used for all security 
**      audit operations.
**      Note, not all the fields of a control block need to be set for every 
**      operation.  Refer to the Interface Design Specification for 
**      inputs required for a specific operation.
**
** History:    
**    20-mar-89 (jennifer)
**        Created.
**    09-feb-90 (neil)
**	  Added user EVENT auditing.
**    11-apr-90 (jennifer) 
**        Added DMA_A_TERMINATE to actions.
**    08-aug-90 (ralph)
**	  Added DMA_S_SUMMARY to get summary of server security state.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**/

/*}
** Name:  DMA_CB - DMF security audit call control block.
**
** Description:
**      This typedef defines the control block required for
**      the following DMF security audit operations:
**      
**      DMA_WRITE_RECORD - Write an audit record.
**      DMA_SET_STATE    - Set auditing state.
**
**      Note:  Included structures are defined in DMF.H if they 
**      start with DM_, otherwise they are defined as global
**      DBMS types.
**
** History:
**    20-mar-89 (jennifer) 
**          Created.
**    09-feb-90 (neil)
**	  Added user EVENT auditing (DMA_E_EVENT).
**    08-aug-90 (ralph)
**	  Added DMA_S_SUMMARY to get summary of server security state.
**     05-nov-1993 (smc)
**        Bug #58635
**        Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
*/
typedef struct _DMA_CB
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMA_AUDIT_CB        13
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         ascii_id;             
#define                 DMA_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'A')
    DB_ERROR        error;                  /* Common DMF error block. */
    i4         dma_flags_mask;         /* Modifier to operation. */
#define                 DMA_S_ENABLE        1
                                            /* For operation DMA_SET_STATE
                                            ** this indicates that state 
                                            ** should be enabled.
                                            */
#define                 DMA_S_DISABLE       2
                                            /* For operation DMA_SET_STATE
                                            ** this indicates that state 
                                            ** should be disabled.
                                            */
#define			DMA_S_SUMMARY	    3
					    /* For operation DMA_SHOW_STATE
					    ** this indicates that state
					    ** information should be
					    ** summarized instead of
					    ** elaborated.
					    */
    PTR             dma_session_id;         /* SCF SESSION ID obtained from
                                            ** SCF.  Every request from a 
                                            ** server must be associated with a 
                                            ** session.  This can be zero
                                            ** if request is from a utility. */
    i4         dma_event_type;         /* Type of security audit event. */
#define                 DMA_E_DATABASE      1
#define                 DMA_E_APPLICATION   2
#define                 DMA_E_PROCEDURE     3
#define                 DMA_E_TABLE         4
#define                 DMA_E_LOCATION      5
#define                 DMA_E_VIEW          6
#define                 DMA_E_RECORD        7
#define                 DMA_E_SECURITY      8
#define                 DMA_E_USER          9
#define                 DMA_E_LEVEL         10
#define                 DMA_E_ALARM         11
#define			DMA_E_RULE	    12
#define			DMA_E_EVENT	    13
#define                 DMA_E_ALL           16
#define			DMA_E_MAXIMUM	    16
    i4         dma_access_type;        /* Type of access requested. */
#define                 DMA_A_SUCCESS       0x0001
#define                 DMA_A_FAIL          0x0002
#define			DMA_A_SELECT        0x0004
#define			DMA_A_INSERT        0x0008
#define			DMA_A_DELETE        0x0010
#define			DMA_A_UPDATE        0x0020
#define			DMA_A_COPYINTO      0x0040
#define			DMA_A_COPYFROM      0x0080
#define			DMA_A_SCAN          0x0100
#define			DMA_A_ESCALATE      0x0200
#define			DMA_A_EXECUTE       0x0400
#define			DMA_A_EXTEND        0x0800
#define                 DMA_A_CREATE        0x1000
#define                 DMA_A_MODIFY        0x2000
#define                 DMA_A_DROP          0x4000
#define                 DMA_A_INDEX         0x8000
#define                 DMA_A_ALTER         0x10000
#define                 DMA_A_RELOCATE      0x20000
#define                 DMA_A_CONTROL       0x40000
#define			DMA_A_AUTHENT	    0x80000
/*  Must correlate to dm0a.h in jpt_dmf_hdr not all numbers available. */
#define                 DMA_A_TERMINATE     0x800000
    i4	    dma_msg_id;		    /* Audit message text id. */
    i4	    dma_msg_cnt;	    /* Count of arguments to message. */
    PTR		    dma_msg_args;	    /* Pointer to actual arguments. */
    DB_OWN_NAME     dma_user;               /* Name of user performing event.*/
    DB_OWN_NAME     dma_ruser;              /* Name of real user connected.*/
    DB_NAME         dma_db_name;            /* Name of database. */
    DM_DATA         dma_name_object;        /* Pointer to area containing 
                                            ** name of object being accessed. */
    DM_DATA         dma_l_event;            /* Pointer to area where list
                                            ** of enabled events can be 
                                            ** returned. */
    DM_DATA         dma_l_level;            /* Pointer to area where list
                                            ** of enabled levels can be 
                                            ** returned. */
    
}   DMA_CB;
 
