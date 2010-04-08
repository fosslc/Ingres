/*--------------------------- iitxtms.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iitxtms.h - TMS service entrypoints 
**
**  Description:
**
**  History:
**      03-Nov-1993 (larrym)
**          First version for Ingres65 DTP/XA/TUXEDO project.
**	31-Jan-1994 (larrym)
**	    Modified s_IITUX_TMS_XID_ACTION to comply with FEreqmem
**	    convention
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    11-Jul-2008 (hweho01)
 **        replace XID with DB_XA_DIS_TRAN_ID that is defined in iicommon.h.
*/

#ifndef  IITXTMS_H
#define  IITXTMS_H

/* transaction action table, used for normal operations */

#define	IITUX_ACTION_ALLOC	10	/* 
					** swag: how many entries to alloc 
					** at one time using FEreqmem
					*/

typedef struct s_IITUX_TMS_2P_ACTION
{
    DB_XA_DIS_TRAN_ID xid;
    i4			first_seq;
    i4			last_seq;
    i4	 		SVN[ IITUX_MAX_APP_SERVERS ]; /* indexed by seqnum */
    long		tp_flags;
    struct s_IITUX_TMS_2P_ACTION	*action_next;
} IITUX_TMS_2P_ACTION;

typedef struct s_IITUX_TMS_2P_ACTION_LIST {
    IITUX_TMS_2P_ACTION			*action_list;	/* list of actions */
    IITUX_TMS_2P_ACTION			*action_pool;	/* list of actions */
    u_i4				action_tag;	/* allocation tag */
} IITUX_TMS_2P_ACTION_LIST;

/* -- Entry Points for tms-xa call processing -- */


/* 
** xa_open processing for a tms. Initializes data structures 
*/

/*
** called at the end of IIxa_open to clean up lgmo table 
*/
FUNC_EXTERN  int
IItux_tms_init( IICX_XA_RMI_CB *xa_rmi_cb_p );

/* xa_close processing for a tms. a noop for now.*/

FUNC_EXTERN  int
IItux_tms_shutdown( );

FUNC_EXTERN  int
IItux_tms_coord_AS_2phase( i4  xa_call, DB_XA_DIS_TRAN_ID *xa_xid, int rmid, long flags);

FUNC_EXTERN  int
IItux_tms_recover( int rmid, IICX_XA_RMI_CB *xa_rmi_cb_p );

#endif  /* ifndef IITXTMS_H */
/*--------------------------- end of iitxtms.h  ----------------------------*/

