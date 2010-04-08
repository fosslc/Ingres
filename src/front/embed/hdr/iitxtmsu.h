/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name:  iitxtmsu.h - TUXEDO TMS utility functions.
**
**  Description:
**      This file contains utility routines necessary to support TUXEDO TMS
**      functionality.  The routines in this file are called from
**      from iitxtms.c (front!embed!libqtxxa).
**
**  Defines:
**
**  History:
**      16-nov-1993 (larrym)
**          written.
**	12-May-1994 (mhughes)
**	    Added protos for newer tms utility Functions
**	25-march-1997(angusm)
**	    Add protos w.r. changes for bug 79824
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**  Name:	IItux_build_rec_action_table
**
**  Description:
**	build a list of willing-commit transactions that need to be committed 
**	or aborted to get the database into a state consistent with TUXEDO's 
**	understanding of it. 
**
**  Inputs:
**	rmi			- the RMI to recover 
**
**  Outputs:
**	iitux_tms_xid_action    - an array of structures containing a table
**				  of INGRES transactions that need to be
**				  committed or aborted.
**	rec_count		- The number of elements in iitux_tms_xid_action
**
**      Returns:
**          XA_OK          	- normal execution.
**          XAER_RMERR     	- input args invalid
**
**  History:
**	06-Jan-1994 (larrym)
**	    written.
*/
int
IItux_build_rec_action_table(
                       	IICX_XA_RMI_CB		  *xa_rmi_cb_p,
 			IITUX_TMS_2P_ACTION_LIST  *iitux_tms_2P_action_list,
                       	i4			  *rec_count);

/*
**  Name:	IItux_tms_recover_xid
**
**  Description:
**	based on the given XID,RMID, find all outstanding willing-commit
**	transactions that need to be committed or aborted to get the 
**	database into a state consistent with TUXEDO's understanding of it. 
**	This module is called when TUXEDO wants us to recover a specific
**	XID.
**
**  Inputs:
**	xa_call			- should be COMMIT or ROLLBACK
**	xa_xid			- the XID to recover
**	rmi			- the RMI to recover 
**
**  Outputs:
**
**      Returns:
**          XA_OK          	- normal execution.
**	    XAER_NOTA		- nothing to recover
**          XAER_RMERR     	- input args invalid
**
**  History:
**	31-Jan-1994 (larrym)
**	    written.
*/
int
IItux_tms_recover_xid( i4  xa_call, XID *xa_xid, int rmid);

STATUS IItux_shm_locktbl(char *username);
