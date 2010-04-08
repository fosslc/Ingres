/*
** Copyright (c) 2004 Ingres Corporation
**
*/
/*
**   Name: - This module has the data structures used to coordinate TUXEDO 
**           transaction closure. This information is kept in shared  
**           memory and is updated by AS's and TMS's.
**
**      01-jan-1996 (stial01, thaju02)
**          Created.
**      30-may-1996 (stial01)
**          Added xn_server_name (B75073)
**	18-aug-1997(angusm)
**	    Add TUX_XN_START for 0 value used to initialise in IIxa_start().
**	7-oct-2004 (thaju02)
**	    Change tux_alloc_pages to SIZE_TYPE.
*/

/*
** Name: IITUX_XN_ENTRY
**
** Description:
**
*/
typedef struct _IITUX_XN_ENTRY
{
    DB_XA_DIS_TRAN_ID         xn_xid;
    char                      xn_server_name[DB_MAXNAME];
    i4                        xn_seqnum;
    int                       xn_rmid;
    i4                        xn_state;           /* tracks the state */
#define		  TUX_XN_START	    0x0
#define           TUX_XN_COMMIT     0x01
#define           TUX_XN_ROLLBACK   0x02
#define           TUX_XN_PREPARED   0x04
#define           TUX_XN_END        0x08
#define           TUX_XN_2PC        0x10

    int                       xn_prev_xn;     /* index of previous branch */
					      /* OPTIMIZATION             */
    int                       xn_rb_value;    /* rollback return value */
    i4                        xn_inuse;       /* ignore if not in use  */
    i4                        xn_intrans;     /* are we in a transaction */
    char                      xn_group[4];    /* unique 4 char group id */
					      /* Prepare,commit,rollback   */	
					      /* should only look at       */
					      /* entries in the same group */

}IITUX_XN_ENTRY;

/*
** Name: IITUX_AS_DESC
**
** Description:
**
*/
typedef struct _IITUX_AS_DESC
{
    PID                       as_pid;
    i4                   as_xn_firstix;
    i4                   as_xn_origcnt;
    i4                   as_xn_cnt;
    char                      as_group[4];
    i4                        as_flags;
#define  AS_FREE  0x01
}IITUX_AS_DESC;

/*
** Name: IITUX_MAIN_CB
**
** Description:
**         This is the header for the shared memory. An array of 
**         IITUX_XN_ENTRYs will follow it.
*/
typedef struct _IITUX_MAIN_CB
{
    i4                   tux_initflag;
    char		      tux_eyecatch[4];
    i4                   tux_next_shmid; /* future */
    i4                   tux_as_max;     
    i4                   tux_xn_max;
    i4                   tux_xn_cnt;
    i4                   tux_xn_array_offset;
    i4                   tux_connect_cnt;
    SIZE_TYPE		 tux_alloc_pages;
    IITUX_AS_DESC             tux_as_desc[1];
} IITUX_MAIN_CB;

/*
** Name: IITUX_LCB
**
** Description:
**     This control block maintains useful pointers.
**
*/
typedef struct _IITUX_LCB
{
    IITUX_AS_DESC             *lcb_as_desc;
    IITUX_XN_ENTRY            *lcb_xn_array;
    IITUX_XN_ENTRY            *lcb_cur_xn_array;
    IITUX_MAIN_CB             *lcb_tux_main_cb;
} IITUX_LCB;

#ifndef VMS
GLOBALREF   IITUX_LCB              IItux_lcb;
#endif  /* ifndef VMS */

FUNC_EXTERN IITUX_XN_ENTRY *
IItux_max_seq(DB_XA_DIS_TRAN_ID *xid);
