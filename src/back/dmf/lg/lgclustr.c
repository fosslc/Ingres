/*
** Copyright (c) 1992, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <lgclustr.h>
#include    <lo.h>
#include    <pm.h>

/**
**
**  Name: LGCLUSTR.C - Logging system routines for "cluster" use.
**
**  Description:
**	This file contains some miscellaneous routines in support of the
**	Cluster implementation of Ingres. The routines are used to find out
**	status information about this node and other nodes within the cluster,
**	to link CSP and RCP processes together, etc.
**
**	The only "external" (non-LG) routine in this file is LGcluster(), which
**	is called throughout the server to ask the yes/no question, "is this
**	a cluster installation?".
**
** Name: LGcluster  - Determine if you are running a cluster.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging system.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	16-feb-1993 (bryanp)
**	    Added LGCn_name() to allow a node to get its node name easily.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	15-may-1993 (rmuth)
**	    Various changes to support some cluster tools.
**	    a. Add LGc_csp_online() and LGc_any_csp_online()
**	    b. Use the PM system to determine
**	          - If this is a cluster
**	          - Other cluster members.
**	21-jun-1993 (bryanp/andys)
**	    CSP/RCP process linkage: Add LGc_csp_enqueue, LGc_rcp_enqueue.
**	    Unfold LGcluster into LGcluster and LG_cluster_node.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <cv.h>, <nm.h>.
**	    Replaced MAX_NODE_NAME_LENGTH with LGC_MAX_NODE_NAME_LEN.
**	    Let ii.node.config.cluster.id=FALSE be legal syntax, and mean the
**		same thing as if ii.node.config.cluster.id were absent from the
**		config.dat file.
**	23-aug-1993 (andys)
**	    In LG_get_log_page_size log an error and return a failure if the 
**	    logfile header checksum is wrong.
**	30-sep-1993 (bryanp)
**	    Return the failure also if the page size is zero (even if the
**	    checksum is said to be ok).  This catches the case of a log file
**	    full of zeros that hasn't been initialized yet.
**	30-sep-1993 (walt)
**	    In LGcnode_info change sys$getsyi to sys$getsyiw to make sure we
**	    don't proceed until it's back with the information.
**	28-jan-1994 (rmuth)
**	    b59196 - Failing to request the CSP_NODE_RUNNING_LOCK lock as a 
**	    system-wide resource.
**	05-aug-1997 (teresak)
**	    Remove reference for VMS to exhdef.h being located in sys$library 
**	    as this is not a system header file but an Ingres header file 
**	    (located in jpt_clf_hdr).
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-apr-2001 (devjo01)
**	    Delete those routines which have been wholely replaced by macro
**	    wrappers around CX routines, and gut others which had a VMS
**	    only implementation, replacing their contents with CX calls
**	    to achieve the same ends.  Remove usage of obsolete CLUSTER.CNF.
**	    LGc_csp_enqueue & LGc_rcp_enqueue go away entirely, since
**	    RCP & CSP are now one process.
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	06-oct-2003 (devjo01)
**	    Sorry Joe, but integrating the 30-apr-2001! changes pretty
**	    much obliterated your 17-sep-2003 changes.
**      21-dec-2004 (devjo01)
**          Add CX_F_NO_DOMAIN to LK_CSP DLM calls.
**	22-Apr-2005 (fanch01)
**	    Adjust cx_nodes[] indexing to be zero based to agree with
**	    cx_nodes definition.
**      11-may-2005 (stial01)
**          Init key2 for LK_CSP lock
**	20-jul-2005 (devjo01)
**	    Use CX_INIT_REQ_CB to init DLM request blocks.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**/

/*{
** Name: LGcnode_info - Gives information on Vaxcluster.
**
** Description:
**      Can be called before LGinitialize.  This routine returns
**      a the node name for the current box on a cluster.  This routine
**      is only called if LGcluster returns a positive value
**      indicating we are running on a cluster.
**
** Inputs:
**	l_name			 limit of bytes to copy.
** Outputs:
**      name                     node name
**      sys_err                  Obsolete.
**	Returns:
**          OK                   If information was available
**	    LG_BADPARAM		 Parameter(s) in error.
**          LG_NOCLUSTERINFO     No cluster information was obtainable.
**              
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging system.
**	30-sep-1993 (walt)
**	    Change sys$getsyi to sys$getsyiw to make sure we don't proceed
**	    until it's back with the information.
**	30-apr-2001 (devjo01)
**	    s103715.  Use generic CX facility to get cluster info.
*/
i4
LGcnode_info(
char                *name,
i4                  l_name,
CL_ERR_DESC	    *sys_err)
{
    if ( sys_err )
	CL_CLEAR_ERR(sys_err);
    
    if ((name == 0) || (sys_err == 0))
	return(LG_BADPARAM);
    
    if ( CXcluster_enabled() )
    {
	STlcopy(CXnode_name(NULL), name, l_name);
	return (OK);
    }
    return (LG_NOCLUSTERINFO);
}

/*{
** Name: LGCnode_names	- Return node names for given node id's.
**
** Description:
**	This routine is called with a bitmask of node id's.  Each bit in
**	the mask represents a cluster node id.  The node name that is
**	stored in the cluster config file is returned in the node_names buffer.
**
**	A node id can be obtained by calling LGshow(LG_A_NODEID).  This
**	returns a node number.  To get the corresponding node name, the
**	node number must be changed into a bitmask value:
**
**	    LGshow(LG_A_NODEID, &node_id, sizeof(node_id), &sys_err);
**	    node_mask = (1 << node_id);
**	    LGCnode_names(node_mask, node_names, sizeof(node_names), &sys_err);
**
**	The node names are formatted into the node_names buffer into a comma
**	separated list.  The list is null terminated.  If the node_names
**	buffer is too small, then all of the names will not be formatted.
**
** Inputs:
**	node_mask			Bitmask of node id's.
**	node_names			Buffer for node names.
**	length				Length of node_names buffer.
**
** Outputs:
**	Returns:
**	    OK
**	    LG_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging system.
**	30-apr-2001 (devjo01)
**	    s103715.  Use generic CX facility to get cluster info.
**	    Information is now pulled from config.dat, rather
**	    than obsolete CLUSTER.CNF.
*/
STATUS
LGCnode_names(
i4		node_mask,
char		*node_names,
i4		length,
CL_ERR_DESC	*sys_err)
{
    u_i4		i, nodecnt;
    i4			used;
    CX_CONFIGURATION   *nodeinfo;

    if ( sys_err )
	CL_CLEAR_ERR(sys_err);

    if ( OK != CXcluster_nodes( &nodecnt, &nodeinfo ) ||
	 0 == nodecnt )
    {
	return FAIL;
    }

    /*
    ** Now format each entry in the array.
    */
    used = 0;
    for (i = 0; i < nodecnt; i++)
    {
	if ( ( (1 << (nodeinfo->cx_nodes[i].cx_node_number - 1)) & node_mask )
	     && ( (used + nodeinfo->cx_nodes[i].cx_node_name_l + 2) <
	     length ) )
	{
	    /*
	    ** If this is not the first entry, then preceed with a comma.
	    */
	    if (used)
	    {
		node_names[used++] = ',';
		node_names[used++] = ' ';
	    }

	    STlcopy( nodeinfo->cx_nodes[i].cx_node_name,
		    node_names + used, nodeinfo->cx_nodes[i].cx_node_name_l );
	    *(node_names + used + nodeinfo->cx_nodes[i].cx_node_name_l) = '\0';
	    used = STlength(node_names);
	}
    }
    return (OK);
}

/*
** Name: LG_get_log_page_size	    - find the log page size for this log
**
** Description:
**	This routine is called when we are opening a log file. We want to know
**	the log page size in order to call DIopen. So first we call this
**	routine, which opens the file, reads the header, extracts the page
**	size from the header, and then closes the file.
**
**	The result is that there is one extra open/close of the log file, but
**	since we don't always know the log page size ahead of time, it's
**	important to be able to find it out.
**
** Inputs:
**	path			    - path to the log file.
**	file			    - the log file name
**
** Outputs:
**	page_size		    - this log's page size
**	sys_err			    - reason for error, if error occurs.
**
** Returns:
**	STATUS
**
** History:
**	12-oct-1992 (bryanp)
**	    Externalized this routine for use by LGopen().
**	23-aug-1993 (andys)
**	    In LG_get_log_page_size log an error and return a failure if the 
**	    logfile header checksum is wrong.
**	30-sep-1993 (bryanp)
**	    Return the failure also if the page size is zero (even if the
**	    checksum is said to be ok).  This catches the case of a log file
**	    full of zeros that hasn't been initialized yet.
*/
STATUS
LG_get_log_page_size(
char *path,
char *file,
i4 *page_size,
CL_ERR_DESC *sys_err)
{
    DI_IO	logfile;
    STATUS	status;
    STATUS	local_status = E_DB_OK;
    i4		one_page;
    char	buffer[2048];
    i4	err_code;
    u_i4	sum;

    status = DIopen(&logfile, path, STlength(path),
			    file, STlength(file),
			    2048, DI_IO_WRITE,
			    (u_i4)0, sys_err);
    if (status)
	return (status);

    one_page = 1;
    status = DIread(&logfile, &one_page,
			    (i4)0,
			    buffer, sys_err);
    if (status)
	return (status);

    if ((sum = LGchk_sum(buffer, sizeof(LG_HEADER))) ==
				((LG_HEADER *)buffer)->lgh_checksum &&
	((LG_HEADER *)buffer)->lgh_size != 0)
    {
	*page_size = ((LG_HEADER *)buffer)->lgh_size;
    }
    else
    {
	uleFormat(NULL, E_DMA48C_LG_BAD_CHECKSUM,        
			(CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, path, 0, file,
			0, sum, 0, ((LG_HEADER *)buffer)->lgh_checksum);
	local_status = LG_CANTOPEN;
    }

    status = DIclose(&logfile, sys_err);

    if ((!status) && local_status)
	status = local_status;
    return (status);
}

/*
** Name: LGc_csp_online	- Check if a node CSP process is up and running.
**
** Description:
**	This routine returns TRUE or FALSE depending on whether the
**	node csp process is online or not. Uses the native DLM to
**	find this out.
**	
**	A request is made to lock the "node is running" lock in X
**	mode
**
** Inputs:
**	nodename		Node we are requesting information on.
**
** Outputs:
**	OK			Node is ONLINE
**	FAIL			Node is OFFLINE
**
** Side Effects:
**	None
**
** History:
**	26-may-1993 (rmuth)
**	    Created
**	21-jun-1993 (bryanp)
**	    Append the installatino code to the node running lock and make it
**		a system-wide resource so that processes in other groups can
**		"see" the lock.
**	26-jul-1993 (bryanp)
**	    Replaced MAX_NODE_NAME_LENGTH with LGC_MAX_NODE_NAME_LEN.
**	28-Jan-1994 (rmuth)
**	    b59196 - Failing to request the CSP_NODE_RUNNING_LOCK lock as a 
**	    system-wide resource.
**	30-apr-2001 (devjo01)
**	    Eviscerate VMS specific portions of routine & replace with
**	    CX calls.  N.b. now that we use LK structs in the DLM, this
**	    might more reasonably be located in LK, but rather than break 
**	    the continuality of file history, we'll keep this in LG.
**	    Note: this probe is not an entirely "safe" way to perform
**	    this check, since if CSP is not up, it is at least theoretically
**	    possible for some other process to see this lock allocated in
**	    the brief interval between the grant and release, and mistakenly
**	    think the CSP is up.  However the danger is very unlikely, and
**	    this was the logic since '93, so I'll keep it.
**      21-dec-2004 (devjo01)
**          Add CX_F_NO_DOMAIN to LK_CSP DLM call.
*/
STATUS
LGc_csp_online(
char 	*nodename)
{
    STATUS	online = FAIL, status;
    i4	        err_code;
    LK_LOCK_KEY	key;
    CX_REQ_CB	req;
    LK_UNIQUE	tranid;

    for ( ; ; )
    {
	key.lk_type = LK_CSP;
	if ( 0 >= ( key.lk_key1 = CXnode_number( nodename ) ) )
	    break;
	key.lk_key2 = 0;
	CX_INIT_REQ_CB(&req, LK_X, &key );
   
	tranid.lk_uhigh = 0;
	tranid.lk_ulow = 1;
	status = CXdlm_request( CX_F_NOWAIT | CX_F_PCONTEXT |
                                CX_F_NODEADLOCK | CX_F_NO_DOMAIN,
                                &req, &tranid );
	if ( CX_ERR_STATUS(status) )
	{
	    if ( E_CL2C27_CX_E_DLM_NOGRANT == status )
	    {
		req.cx_new_mode = LK_N;
                status = CXdlm_request( CX_F_NOWAIT | CX_F_PCONTEXT |
                                        CX_F_NODEADLOCK | CX_F_NO_DOMAIN,
                                        &req, &tranid );
		if ( CX_ERR_STATUS(status) )
		    break;
		if ( OK == status && CX_ST_JOINED == req.cx_value[1] )
		{
		    online = OK;
		}
	    }
	    else
	    {
		/* Unexpected error */
		uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }
	}
	status = CXdlm_release( CX_F_IGNOREVALUE, &req );
	if ( OK != status )
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	}
	break;
    }
    return online;
}

/*
** Name: LGc_any_csp_online	- Check if any CSP process is up and running.
**
** Description:
**	This routine returns TRUE or FALSE depending on whether any
**	csp process is online or not. Uses the native DLM to find this out.
**
**	To determine if any CSP is currently running we make a request for
**	the cluster master lock.
**
** Inputs:
**	None
**
** Outputs:
**	OK			Any CSP process is running
**	FAIL			None running
**
** Side Effects:
**	None
**
** History:
**	26-may-1993 (rmuth)
**	    Created
**	30-apr-2001 (devjo01)
**	    Eviscerate VMS specific portions of routine & replace with
**	    CX calls. (see other notes in LGc_csp_online).
**      21-dec-2004 (devjo01)
**          Add CX_F_NO_DOMAIN to LK_CSP DLM call.
*/
STATUS
LGc_any_csp_online()
{
    STATUS	online = FAIL, status;
    i4	        err_code;
    LK_LOCK_KEY	key;
    CX_REQ_CB	req;
    LK_UNIQUE	tranid;

    key.lk_type = LK_CSP;
    key.lk_key1 = 0;
    key.lk_key2 = 0;
    CX_INIT_REQ_CB(&req, LK_X, &key);
    tranid.lk_uhigh = 0;
    tranid.lk_ulow = 1;
    status = CXdlm_request( CX_F_NOWAIT | CX_F_PCONTEXT |
                            CX_F_NODEADLOCK | CX_F_NO_DOMAIN,
                            &req, &tranid );
    if ( CX_ERR_STATUS(status) )
    {
	if ( E_CL2C27_CX_E_DLM_NOGRANT == status )
	    online = OK;
	else
	{
	    /* Unexpected error */
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);

	}
    }
    else
    {
	status = CXdlm_release( CX_F_IGNOREVALUE, &req );
	if ( OK != status )
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	}
    }
    return online;
}
