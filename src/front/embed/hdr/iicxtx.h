/*--------------------------- iicxtx.h ------------------------------------*/
/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
*/

/*
**  Name: iicxtx.h   
**
** Description:        Utility Context Management routines and data structures.
**                     This module contains the CBs and CX routines that drive
**                     TX (Tuxedo) related information.
**
** History:
**	01-Oct-93	(mhughes)
**	First Implementation
**	29-nov-93 (larrym)
**	    Added typedef of IITUX_ICAS_SVR, from iitxicas.h into here so
**	    libqxa can reference it.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef  IICXTX_H
#define  IICXTX_H

/* 
**  Please see IICX.H for understanding how these structures are used/hooked in
**  to other structures.
**
*/

#define    IICX_TUX_MAX_FREE_ICAS_XN_CBS    32


/*
**   Name: IICX_ICAS_XN_MAIN
**
**   Description: 
**        The CX ICAS XN MAIN CB has the top level information about
**	  all the active XID's about which the ICAS is holding information.
**
**   History:
**       30-Sep-1993  (mhughes)
**           First written for Ingres65 DTP/XA for TUXEDO project.
*/

typedef struct IICX_ICAS_XN_MAIN_
{
   IICX_CB      *icas_xn_list;
   i4           num_active_xids;  /* Number of active XID's managed by this ICAS */
   i4           max_free_icas_xn_cbs;
   i4           num_free_icas_xn_cbs;
   IICX_CB      *icas_xn_cb_free_list; 

/* CHECK Free list cbs should be strung off IIcx_main_cb->free_list_cbs  
**       so we probably don't need to duplicate here
**
**   i4           max_free_icas_xn_list_cbs;
**   i4           num_free_icas_xn_list_cbs;
**   IICX_ICAS_XN_LIST_CB
**               *icas_xn_list_cb_free_list; 
*/

} IICX_ICAS_XN_MAIN_CB;


/* 
** ICAS server List entry 
** CHECK should probably be renamed to IICX_ICAS_SVR or something
*/

typedef struct IITUX_ICAS_SVR_
{
    struct IITUX_ICAS_SVR_ 	*next_svr_cb_p;
    i4			svr_id;
} IITUX_ICAS_SVR;


/*
**   Name: IICX_ICAS_XN_CB
**
**   Description: 
**        The CX ICAS XN control block has information that 
**        relates AS number in a group to a specific XID.
**        Each IItux_register_SVN call by an AS which 
**	  contains a reference to a new XID will create a new
**	  instance of this CB. 
**        Each CB will have a number of list entries for each SVN
**        working on that XID
** 
**   History:
**      30-Sep-1993	(mhughes)
**           First written for Ingres65 DTP/XA for TUXEDO project.
**	18-mar-1994 (larrym)
**	     added mark_2p kludge.  See iitxtms.c
*/

typedef struct IICX_ICAS_XN_
{
    i4        		next_seq_num;
    i4        		num_active_servers;
    i4			mark_2pc;
    IITUX_ICAS_SVR	*icas_svn_list;
} IICX_ICAS_XN_CB;


/*
**   Name: IICXcreate_icas_xn_cb()
**
**   Description: 
**        The ICAS XN CB is created, if needed.
**	  Else it is obtained from a free list. 
**
**   Inputs:
**       None.
**  
**   Outputs:
**       Returns: Pointer to the ICAS XN CB, if properly created.
**                NULL if there was an error. 
**
**   History:
**       24-Mar-1993 - First written (mani)
**       30-Sep-1993 	(mhughes)
**		modified xa xn thrd for icas xn cb
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_icas_xn_cb(
                           IICX_ID             *cx_id,          /* IN */
                           IICX_CB             **cx_cb_p_p      /* OUT */
                        );

/*
**   Name: IICXdelete_icas_xn_cb()	
**
**   Description: 
**        The ICAS XN CB associated with a specific global transaction 
**        is freed. It is placed in the process-wide free list of
**        ICAS XN CBs (rooted at the ICAS XN MAIN CB.)
**
**   Inputs:
**        cx_cb_p  - pointer to the CB that has the  icas_xn_cb.
**		     The CB is placed in a free list.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successfully freed.
**
**   History:
**       24-Mar-1993 - First written (larrym, mani)
**       30-Sep-1993   (mhughes)
**		Modified xn_thrd for icas_xn_cb's
*/

FUNC_EXTERN   DB_STATUS  IICXdelete_icas_xn_cb(
                            IICX_CB    *cx_cb_p         /* IN */
                         );

/*
**   Name: IICXfree_icas_xn_cb()	
**
**   Description: 
**        The ICAS XN CB is deleted. All memory used by the ICAS XN CB
**        is freed. Any sub-lists are also freed. This is typically called at
**        process shutdown time.
**
**   Inputs:
**        icas_xn_cb_p  - the icas_xn_cb is deleted. All memory used is 
**        freed.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successfully freed.
**
**   History:
**       24-Mar-1993 - First written (mani, larrym)
**       30-Sep-1993   (mhughes)
**		Modified xn_thrd for icas_xn_cb's
*/

FUNC_EXTERN   
DB_STATUS  IICXfree_icas_xn_cb(
                              IICX_ICAS_XN_CB    *icas_xn_cb_p 
                              );

/*
** 
**  GLOBAL structures for supporting TX (Tuxedo) CB types in the IICX modules.
**
*/

GLOBALREF     IICX_ICAS_XN_MAIN_CB  *IIcx_icas_xn_main_cb;

#endif  /* ifndef IICXTX_H */

/*--------------------------- end of iicxtx.h ------------------------------*/










