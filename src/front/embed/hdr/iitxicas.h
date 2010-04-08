/*--------------------------- iitxicas.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iitxicas.h - ICAS service entrypoints
**
**  History:
**      25-Aug-1993 (mhughes)
**          First version for Ingres65 DTP/XA/TUXEDO project.
**	29-Nov-1993 (larrym)
**	    moved typedef of IITUX_ICAS_SVR to iicxtx so libqxa files
**	    can include it.
**	02-Dec-1993 (mhughes)
**	    Added icas status checking services
**	20-Dec-1993 (mhughes)
**	    Increased MAX_FREE_ICAS_SVR_CBS > 256
**	24-Dec-1993 (mhughes)
**	    Pass rmi_cb and rmid into IItux_icas_open()
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef  IITXICAS_H
#define  IITXICAS_H


/* List header for icas server cb list. */

typedef struct IITUX_ICAS_SVR_HEAD_
{
    struct IITUX_ICAS_SVR_ 	*next_svr_cb_p;
    struct IITUX_ICAS_SVR_ 	*last_svr_cb_p;
    struct IITUX_ICAS_SVR_ 	*free_svr_cb_list_p;
    i4				num_active_svrs;
    i4				num_free_cbs;
#define IITUX_MAX_FREE_ICAS_SVR_CBS 256
} IITUX_ICAS_SVR_HEAD;


/* ICAS server List entry */
/* MOVED to iicxtx.h so libqxa can see it. */

/* Entry Points for icas-xa call processing */

/* xa_open processing for an icas. Initialises data structures and ensures
** server group is consistent on startup
*/

FUNC_EXTERN  int
IItux_icas_open( IICX_XA_RMI_CB *xa_rmi_cb_p, i4  *rmid );

/* xa_close processing for an icas. Tidies data structures and
** unadvertises services
*/

FUNC_EXTERN int
IItux_icas_close( );


/* Entry points for icas services. These are functions which are coded to
** act as services in a TUXEDO application. They accept a pointer to the 
** TUXEDO communication buffer TPSVCINFO as their only argument.
*/

/* Register an AS as part of a server group */

FUNC_EXTERN void
IItux_as_reg_with_icas( TPSVCINFO *iitux_info );

/* UnRegister an AS as part of a server group */

FUNC_EXTERN void
IItux_as_unreg_with_icas( TPSVCINFO *iitux_info );

/* Register an AS as participating in a global transaction */

FUNC_EXTERN void
IItux_as_reg_SVN_in_XID( TPSVCINFO *iitux_info );

/* Tidy data structures after commiting a global transaction */

FUNC_EXTERN void
IItux_tms_purge_XID( TPSVCINFO *iitux_info );

/* ICAS provides the TMS with a list of AS's active upon a global transaction*/

FUNC_EXTERN void
IItux_tms_get_XID_SVNs( TPSVCINFO *iitux_info );

/* ICAS provides a test client with list of active XID's */

FUNC_EXTERN void
IItux_icas_XID_status( TPSVCINFO *iitux_info );

/* ICAS provides a test client with a list of active SVNs */

FUNC_EXTERN void
IItux_icas_SVN_status( TPSVCINFO *iitux_info );


#endif  /* ifndef IITXICAS_H */
/*--------------------------- end of iitxicas.h  ----------------------------*/

