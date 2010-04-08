
/*--------------------------- iixagbl.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iixagbl.h - LIBQXA global constant and variable definitions
**
**  History:
**      28-Mar-94(larrym)
**          First written. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Jul-2008 (hweho01)
**	    For IIxa_do_2phase_operation() function declaration, replace  
**	    XID with DB_XA_DIS_TRAN_ID that is defined in iicommon.h.  
*/

#ifndef  IIXAMAIN_H
#define  IIXAMAIN_H

/*
** Function prototypes
*/

/* 
** this routine encapsulates all the work done for an xa_prepare/xa_commit 
** and xa_rollback.  
*/

FUNC_EXTERN int
IIxa_do_2phase_operation( i4              xa_call,
                          DB_XA_DIS_TRAN_ID  *xid,
                          int             rmid,
                          long            flags,
			  i4              branch_seqnum,
			  i4              branch_flag 
			);


/* connect, or setup via set_sql to a recovery session. The session is to   */
/* the IIDBDB. There is one recovery session per RMI. */
FUNC_EXTERN int
IIxa_connect_to_iidbdb( IICX_FE_THREAD_CB *fe_thread_cb_p,
                   	IICX_XA_RMI_CB    *xa_rmi_cb_p,
                   	char              **dbname_p_p
                 	);

#endif /* ifndef IIXAMAIN_H */

/*------------------------------end of iixagbl.h ----------------------------*/
