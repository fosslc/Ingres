/*
** Copyright (c) 2001, 2008 Ingres Corporation
*/

#include    <stdlib.h>

#include    <compat.h>
#include    <cv.h>
#include    <pc.h>
#include    <cs.h>
#include    <er.h>
#include    <lk.h>
#include    <pm.h>
#include    <st.h>
#include    <tr.h>
#include    <di.h>
#include    <cx.h>
#include    <cxprivate.h>
#include    <me.h>


/**
**
**  Name: CXAPI.C - CX informational functions for libingres.a.
**
**  Description:
**      This module contains routines which return basic information 
**	about the CX configuration.   This has been split from
**	CXINFO.C, so that the generated object file can be included
**	in the merged libraries without pulling in OS specific 
**	references, the CS facility, etc.
**
**	Public routines
**
**	    CXcluster_configured- Is cluster support configured
**                                at the Ingres level?
**
**	    CXnuma_configured	- Is NUMA clustering configured
**                                at the Ingres level?
**
**	    CXhost_name		- Host name of current node.
**
**	    CXnode_name		- Node name of current node.
**
**	    CXnode_number	- Node number for requested node.
**
**	    CXcluster_nodes	- Information about nodes configured.
**
**	    CXdecorate_file_name - Append host to file & keep < 36 chars.
**
**	    CXnuma_configured	- Is NUMA configured?
**
**	    CXnuma_cluster_configured - Is NUMA clustering configured?
**
**	    CXget_context        - Pre-parse user arguments to
**				   set cluster context.
**
**	    CXset_context        - Set value to be returned by CXnode_name.
**				   This will effect shared memory allocations
**				   and file extention "decoration".
**
**	    CXnode_info		 - Obtain info about a specific node.
**
**	    CXexec		 - Perform 'execvp' system call, but
**				   add '-rad' argument, IFF running
**				   NUMA clusters on current node &&
**				   CXset_context had been called, &&
**				   -rad or -node argument not already
**				   present.
**
**	Public data
**	
**	    CX_local_RAD	- Normally zero.  If Installation
**				  is configured for NUMA clusters,
**				  this will either be set by
**				  CXset_context().  This may also
**				  be set by CXnuma_bind_to_rad.  
**
**  History:
**      06-feb-2001 (devjo01)
**          Created.
**	17-sep-2002 (devjo01)
**	    Added CXnuma_configured, CXget_context, CX_local_RAD.
**	24-oct-2003 (kinte01)
**	    Add missing me.h include file
**	06-jan-2004 (devjo01)
**	    Rename cx_axp_dl_load_lib to cx_dl_load_lib and make it
**	    available to Linux.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      10-Sep-2004 (devjo01)
**          It's a pity lakvi01 had to back out in bulk all his
**          compile warning changes on 7-26, since his changes for
**          this file were perfectly legitimate and useful.
**	17-dec-2004 (devjo01)
**	    Correct 'cx_saved_node_name' ref to intended 'cx_hostname'.
**	21-jan-2005 (devjo01)
**	    Correct logic error in CXcluster_nodes where we would loop
**	    forever if we had a cluster id of 0.
**	15-Mar-2005 (devjo01)
**	    In CXnickname_from_node_name do not fill in nickname if
**	    no nickname defined.
**	12-apr-2005 (devjo01)
**	    Populate cx_host_name from "ii.!.gcn.local_vnode" instead of
**	    ii.!.config.host.
**	22-apr-2005 (devjo01)
**	    Add <stdlib.h> as part of VMS port.
**	07-jan-2008 (joea)
**	    Remove cluster nickname functions.
*/

GLOBALREF	CX_PROC_CB	CX_Proc_CB;

static bool cx_is_config_value_on( char * partial_value );

void cx_fmt_msg( i4 msgcode, char *buffer, i4 buflen,
		 i4 erargs, ER_ARGUMENT *erargv );


static char cx_saved_node_name[CX_MAX_NODE_NAME_LEN+1] = { '\0' };



/*{
** Name: CXcluster_configured	- Report if Ingres configured for clustering.
**
** Description:
**
**	This routine only checks to see if clustering is 
**	configured at the Ingres level.  OS or hardware support
**	may be lacking, but presumable the configuration program
**	has performed these checks.   This routine is used where
**	CXcluster_enabled might normally be used, except that 
**	it is in a module that is included in libingres.
**	By using CXcluster_configured instead, no special OS
**	library need be implicitly pulled in.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- Installation is configured for clustering.
**		FALSE	- Ingres was not configured for cluster support.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-jun-2001 (devjo01)
**	    Created.
*/
bool
CXcluster_configured( void )
{
    static	i4	configured = -1;

    if ( configured < 0 )
    {
	configured =
	 ( CXnuma_cluster_configured() || ( CXnode_number( NULL ) > 0 ) )
	   ? 1 : 0;
    }
    return (bool)( configured );
} /* CXcluster_configured */



/*{
** Name: CXnuma_configured	- Report if Ingres configured for NUMA.
**
** Description:
**
**	This routine calls only checks to see if NUMA is 
**	configured for the current node at the Ingres level. 
**	OS or hardware support may be lacking, but presumably
**	the configuration program performed these checks.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- Current NODE is configured for NUMA.
**		FALSE	- Ingres was not configured for NUMA support.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-sep-2002 (devjo01)
**	    Created.
*/
bool
CXnuma_configured( void )
{
    static	i4	configured = -1;

    if ( configured < 0 )
    {
	configured = cx_is_config_value_on( ERx("config.numa" ) );
    }
    return (bool)( configured );
} /* CXnuma_configured */


/*{
** Name: CXnuma_cluster_configured - Configured for NUMA clusters?
**
** Description:
**
**	This routine calls only checks to see if the current
**      Ingres instance is configured as a NUMA cluster node.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- Current NODE is configured for NUMA clustering.
**		FALSE	- Ingres was not configured for NUMA support.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-sep-2002 (devjo01)
**	    Created.
*/
bool
CXnuma_cluster_configured( void )
{
    static	i4	configured = -1;

    if ( configured < 0 )
    {
	configured = cx_is_config_value_on( ERx("config.cluster.numa" ) );
    }
    return (bool)( configured );
} /* CXnuma_cluster_configured */



/*{
** Name: CXhost_name	- Get host name for current machine.
**
** Description:
**
**	This is just a wrapper for PMhost, which assures that
**	the name is not more than CX_MAX_NODE_NAME_LEN characters
**	long and is entirely lower case.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**		Address of cached host name.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	01-oct-2001 (devjo01)
**	    Created.
**	17-dec-2004 (devjo01)
**	    Correct 'cx_saved_node_name' ref to intended 'cx_hostname'.
*/
char *
CXhost_name()
{
    static char cx_hostname[CX_MAX_HOST_NAME_LEN] = { '\0', };

    if ( '\0' == cx_hostname[0] )
    {
	STlcopy( PMhost(), cx_hostname, CX_MAX_HOST_NAME_LEN );
	CVlower(cx_hostname);
    }
    return cx_hostname;
} /* CXhost_name */



/*{
** Name: CXnode_name	- Get node name for current machine.
**
** Description:
**
**	This gets the node name of the current machine.
**
**	If we are running on a NUMA node of a NUMA cluster,
**	node name is prefixed by the RAD of the virtual node,
**	otherwise it is identical to the host name.
**
** Inputs:
**	nodename	- Ptr to buffer to fill in with nodename, or NULL
**
** Outputs:
**	nodename	- If not NULL, fill with '\0' node name.
**
**	Returns:
**		Address of nodename, or internal node name buffer
**		if NULL was passed.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	13-feb-2001 (devjo01)
**	    Created.
*/
char *
CXnode_name( char *nodename )
{
    if ( '\0' == cx_saved_node_name[0] )
    {
	STcopy( CXhost_name(), cx_saved_node_name );
    }
    if ( NULL == nodename )
	nodename = cx_saved_node_name;
    else
	STmove( cx_saved_node_name, '\0', CX_MAX_NODE_NAME_LEN, nodename );
    return (char*)nodename;
} /* CXnode_name */

/*{
** Name: CXnode_name_from_id	- Get node name given a machine ID
**
** Description:
**
**	This gets the node name given a machine ID.
**
** Inputs:
**      node_id		- Numeric identifier given to a machine
**			  (on VMS clusters, the CSID)
**
** Outputs:
**	node_name	- Ptr to buffer to fill in with nodename
**
**	Returns:
**		none
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	21-feb-2008 (joea)
**	    Created.
*/
void
CXnode_name_from_id(u_i4 node_id, char *node_name)
{
    bool		found = FALSE;
#if defined(VMS)
    CX_NODE_CB		*sms = CX_Proc_CB.cx_ncb;
    CX_NODE_INFO	*pnode;
    i4			i;

    /* special case for the local node */
    if (node_id == sms->cxvms_csids[0])
    {
	STcopy(CXhost_name(), node_name);
	return;
    }
    for (i = 1; i < sizeof(sms->cxvms_csids); ++i)
    {
	if (node_id == sms->cxvms_csids[i])
	{
	    pnode = &CX_Proc_CB.cx_ni->cx_nodes[i - 1];
	    STcopy(pnode->cx_host_name, node_name);
	    found = TRUE;
	}
    }
#endif
    if (!found)
	STcopy("Unknown", node_name);
}


/*{
** Name: CXnode_number	- Get Ingres cluster node number.
**
** Description:
**
**	This returns the Ingres cluster node number for the passed nodename,
**	or the current machine if NULL is passed.
**
** Inputs:
**	nodename	- Ptr to target nodename, or NULL.
**
** Outputs:
**	none
**
**	Returns:
**		Ingres cluster node number for target, or zero if
**		target is not configured by Ingres as part of a cluster,
**		or -1 if configured number is invalid.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Default PM context may be initialized.
**
** History:
**	13-feb-2001 (devjo01)
**	    Created.
*/
i4
CXnode_number( char *nodename )
{
    static	 i4	 local_node_number = -1;

    i4			 nodenumber;
    CX_NODE_INFO	*pni;
    char		*pnode;

    if ( ( NULL == ( pnode = nodename ) ) && ( local_node_number >= 0 ) )
    {
	/* Quick exit if value for local node already obtained. */
	return local_node_number;
    }

    if ( NULL == pnode )
    {
	pnode = CXnode_name( NULL );
    }

    pni = CXnode_info( pnode, 0 );
    if ( NULL == pni )
	return 0;

    nodenumber = pni->cx_node_number;

    if ( NULL == nodename )
    {
	local_node_number = nodenumber;
    }
    return nodenumber;
} /*CXnode_number*/


/*{
** Name: CXcluster_nodes	- Retrun information about configured nodes.
**
** Description:
**
**	Function will return information about the nodes configured
**	in this installation.
**
** Inputs:
**	
**	pnodecnt	- Pointer to u_i4 to hold node count, or NULL.
**
**	hconfig		- Address of pointer to be filled with pointer
**			  to an initialized CX_CONFIGURATION, or NULL.
**			  DO NOT alter the contents of the cached
**			  CX_CONFIGURATION through the returned address!
**
** Outputs:
**
**	*pnodecnt	- Filled in with # nodes.
**
**	*hconfig	- Holds pointer to cached CX_CONFIGURATION.
**
**	Returns:
**		OK		- Normal successful completion.
**		FAIL		- Problem with configuration found.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	18-apr-2001 (devjo01)
**	    Created. (Much of code lifted from DMFCSP)
**	15-Apr-2005 (fanch01)
**	    Modified cx_nodes array references to be valid from element zero.
*/
STATUS
CXcluster_nodes( u_i4 *pnodecnt, CX_CONFIGURATION **hconfig )
{
    static		 i4			 config_valid = 0;
    static		 CX_CONFIGURATION	 config = { 0 };

    STATUS		 status = OK;
    u_i4		 nodecnt;
    PM_SCAN_REC 	 scanrec;
    char		*resname;
    char		*resvalue;
    char		*nodename;
    i4			 nodename_l;
    i4			 clusterid;
    CX_NODE_INFO	*pni;
    i4			 i, rad;
    char		 param[80];

    if ( !config_valid )
    {
	status = PMinit();
	if ( OK == status )
	{
	    status = PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)0);
	}
	else if ( PM_DUP_INIT == status )
	{
	    status = OK;
	}
	if ( status == OK )
	{
	    nodecnt = 0;

	    /*
	    ** Start the scan
	    */
	    status = PMscan( "^II\\.[^.]*\\.CONFIG\\.CLUSTER\\.ID$",
			     &scanrec, (char *) NULL,
			     &resname, &resvalue );

	    while ( status == OK )
	    {
		/*
		** Extract out node name and convert value to integer
		*/
		nodename = PMgetElem( 1, resname );
		nodename_l = STlength(nodename);

		CVal( resvalue, &clusterid );

		/* Ignore entries with zero cluster id. */
		if ( 0 != clusterid )
		{
		    /*
		    ** Check id is withing range
		    */
		    if ( (clusterid > CX_MAX_NODES) ||
			 (clusterid < 0) )
		    {
			TRdisplay( ERx( \
"%@ CSP node config: Node %s has an invalid id: %d, valid range (0 - %d)\n"),
			     nodename, clusterid, CX_MAX_NODES );
			status = FAIL;
			break;
		    }

		    /*
		    ** Adjust clusterid so array references are zero based.
		    */
		    clusterid--;

		    /*
		    ** Check that specified id is not already used.
		    */
		    if ( config.cx_nodes[clusterid].cx_node_name_l != 0 )
		    {
			/*
			** Location is already being used issue a message 
			** accordingly
			*/
			if (STxcompare( nodename, nodename_l, 
			    config.cx_nodes[clusterid].cx_node_name,
			    config.cx_nodes[clusterid].cx_node_name_l,
			    TRUE, FALSE) == 0)
			{
			    TRdisplay(
ERx("%@ Cluster configuration: Node %~t already exists in correct location.\n"),
			     nodename_l, nodename );
			}
			else
			{
			    TRdisplay(
ERx("%@ Cluster configuration: Node %~t has same id as Node %~t.\n"),
			     nodename_l, nodename,
			     config.cx_nodes[clusterid].cx_node_name_l,
			     config.cx_nodes[clusterid].cx_node_name );
			}
			status = FAIL;
			break;
		    }

		    /*
		    ** Make sure nodename is unique
		    */
		    for (i = 1; i <= nodecnt; i++)
		    {
			if (STxcompare(
			     config.cx_nodes[config.cx_xref[i]].cx_node_name,
			     config.cx_nodes[config.cx_xref[i]].cx_node_name_l,
			     nodename, nodename_l, TRUE, FALSE) == 0)
			{
			    TRdisplay( 
		 ERx("%@ Cluster configuration: Node %s already exists.\n"),
			     nodename );
			    status = FAIL;
			    break;
			}
		    }

		    /*
		    ** Add to configuration
		    */
		    nodecnt++;
		    config.cx_xref[nodecnt] = clusterid;
		    pni = &config.cx_nodes[clusterid];
		    STmove( nodename, '\0', CX_MAX_NODE_NAME_LEN,
		     pni->cx_node_name );
		    pni->cx_node_name_l = 
		     STnlength(CX_MAX_NODE_NAME_LEN,
		     pni->cx_node_name );
		    pni->cx_node_number = clusterid + 1;
		    pni->cx_host_name_l = pni->cx_rad_id = 0;

		    STprintf(param,"ii.%s.gcn.local_vnode", nodename);
		    if ( OK == PMget(param, &resvalue) )
		    {
			STmove( resvalue, '\0', CX_MAX_HOST_NAME_LEN,
				 pni->cx_host_name );
			pni->cx_host_name_l = STnlength( CX_MAX_HOST_NAME_LEN,
			 pni->cx_host_name );
		    }

		    STprintf(param,"ii.%s.config.numa.rad", nodename);
		    if ( OK == PMget(param, &resvalue) &&
			 OK == CVan(resvalue, &rad) && rad > 0 )
		    {
			pni->cx_rad_id = rad;
		    }

		    /*
		    ** Find next element
		    */
		    status = PMscan( (char *) NULL, &scanrec, (char *) NULL,
			     &resname, &resvalue );
		}
	    }

	    /*
	    ** End condition 
	    */
	    if ( status == PM_NOT_FOUND )
		status = OK;
	}

	if (status != OK )    
	{
	    TRdisplay( ERx("%@ Cluster configuration Error\n") );
	    return (FAIL);
	}
	config.cx_node_cnt = nodecnt;
	config_valid = 1;

    }
    if ( pnodecnt )
	*pnodecnt = config.cx_node_cnt;
    if ( hconfig )
	*hconfig = &config;
    
    return OK;
} /*CXcluster_nodes*/


/*{
** Name: CXnode_info	- Return ptr to node info rec or NULL.
**
** Description:
**
**	Common subroutine for get/set context. 
**
** Inputs:
**	char *	host;	- If NULL, CXhost_name is used 
**	int	rad;	- If zero, ignore, else must match
**			  a valid RAD for 'host'
**
** Outputs:
**	none.
**
**	Returns:
**		ptr to node info rec or NULL.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-sep-2002 (devjo01)
**	    Created.
*/
CX_NODE_INFO *
CXnode_info(char *host, i4 rad)
{
    CX_CONFIGURATION	*pconfig;
    CX_NODE_INFO        *pni;
    i4			 i;
	    
    if ( OK != CXcluster_nodes( NULL, &pconfig ) )
    {
	return NULL;
    }

    if ( !host ) host = CXhost_name();

    for ( i = 1; i <= pconfig->cx_node_cnt; i++ )
    {
	pni = pconfig->cx_nodes + pconfig->cx_xref[i];
	if ( 0 == rad )
	{
	    if ( 0 == STxcompare( host, 0,
		      pni->cx_node_name, pni->cx_node_name_l,
		      TRUE, FALSE ) )
	    {
		/* Found the node name */
		return pni;
	    }
	}
	else
	{
	    if ( ( rad == pni->cx_rad_id ) &&
		 ( ( 0 == STxcompare( host, 0,
			  pni->cx_node_name,
			  pni->cx_node_name_l, TRUE, FALSE ) ) ||
		   ( 0 != pni->cx_host_name_l &&
		     0 == STxcompare( host, 0,
			  pni->cx_host_name,
			  pni->cx_host_name_l, TRUE, FALSE ) ) ) )
	    {
		/* Found node matching RAD & host */
		return pni;
	    }
	}
    } 

    return NULL;
}


/*{
** Name: CXdecorate_file_name - Append node name to filename.
**
** Description:
**
**	Append upper case node name to passed filename, keeping
**	total filelength 36 chars or below.
**
** Inputs:
**	filename - buffer holding filename.  Must be > 36 chars in length.
**	nodename - name to append, or if NULL, CXnode_name is used.
**
** Outputs:
**	none.
**
**	Returns:
**		- Pointer to passed filename, so this can be used in-line
**		  to printf, etc.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-sep-2002 (devjo01)
**	    Created.
*/
char *
CXdecorate_file_name( char *filename, char *node )
{
    i4		len, extlen;

    len = STlength(filename);

    if ( NULL == node ) node = CXnode_name(NULL);

    if (len < DI_FILENAME_MAX)
    {
	*(filename + len++) = '_';
	if (len < DI_FILENAME_MAX)
	{
	    extlen = STlcopy(node, filename + len, DI_FILENAME_MAX - len);
	    *(filename + len + extlen) = '\0';
	    CVupper(filename + len);
	}
    }
    return filename;
}


/*{
** Name: CXget_context - Parse command arguments for NUMA args.
**
** Description:
**
**	One particularly ugly requirement of NUMA clusters, is the need
**	to specify the target context of certain commands that
**	normally could easily determine a default context by themselves.
**
**	E.g. 'lockstat' run on any node of a non-NUMA cluster "knows"
**	implicitly that the current node corresponds to the current host.
**
**	However, in a NUMA cluster, this happy state, no longer applies.
**	If our sysadm logs onto a node which is configured into two or
**	more virtual NUMA nodes, there is no way for 'lockstat', etc.,
**	to guess which of the virtual nodes is the target for the Ingres
**	tool.
**
**	We try to make a virtue of this neccesity by centralizing the
**	parsing needed for these commands and in passing pick up a
**	generalized way of setting processor affinity for Ingres tools.
**	
**	Two types of arguments are checked here:
**	 '-rad' is a RAD spec, in which the host machine is implicitly
**	the local machine.
**
**	 '-node' may refer to either a local RAD by its node name or 
**	node alias, or to a node anywhere in thr cluster if CX_NSC_NODE_GLOBAL
**	is set.
**
**	Both arguments can be in the form -arg=value, or -arg value.
**
**	The users input	is checked against the following rules:
**
**	R1: If NOT running clusters, '-node' parameter will always
**	    cause an error.
**
**	R2: Only one of either "-node" or "-rad" is allowed.
**
**	R3: If "-rad" is present, value must correspond to a valid RAD.
**
**	R4: If "-node" is present, it must be a valid node, or node alias.
**
**	    R4a: If CX_NSC_NODE_GLOBAL is not set, node must match local host
**	         name or be hosted (virtual nodes only) by local host.
**
**	R5: If not otherwise specified, and running NUMA clusters,
**	    RAD value is taken from environment variable II_DEFAULT_RAD.
**
**	R6: If running NUMA clusters, and CX_NSC_RAD_OPTIONAL is not
**	    set, an error will be reported if context is not set by a
**	    valid -rad, or -node, or II_DEFAULT_RAD value.
**
**	R7: If rules R2-R6 are not violated, and CX_NSC_CONSUME is set,
**	    and an matching parameter or parameter pair was provided,
**	    the parameters are removed from the argument vector, and
**	    the argument count is adjusted.
**
**	R8: If CX_NSC_SET_CONTEXT is set, and not running NUMA clusters,
**	    and a valid RAD value was provided with an explicit '-rad'
**	    argument, CXnuma_bind_to_rad is called to set process affinity
**	    to the specified RAD.
**
**	R9: If CX_NSC_SET_CONTEXT is set, running NUMA clusters, or
**	    if a valid '-node' parameter was supplied, CXset_cluster
**	    is called for the specified node.
**
**	R10: If CX_NSC_REPORT_ERRS is set, and an error occurred,
**	    message is sent to stdout.
**	
**	If all appropriate rules are met, then OK is returned.
**	Caller should be set up to catch and complain about any
**	extra invalid parameters.
**	
**	If outbuf is not NULL, node name if copied out if status =
**	OK, else error message text is copies out.  In both cases
**	only a max of outlen chars are returned, including an EOS.
**
** Inputs:
**	
**	pargc	- Address of integer holding # of arg strings.
**
**	argv	- Array of string pointers to argument values.
**
**	flags	- Flag bits OR'ed together.  Valid flags are:
**
**		CX_NSC_REPORT_ERRS - Emit any error message to stdout.
**
**		CX_NSC_NODE_GLOBAL - Allow node argument to refer to
**				     Nodes outside the current host.
**
**		CX_NSC_CONSUME	   - Eat any -rad/-node args & values if OK.
**
**		CX_NSC_IGNORE_NODE - Ignore '-node' params in scan.
**
**		CX_NSC_SET_CONTEXT - Set CX context based on good params.
**
**		CX_NSC_RAD_OPTIONAL - Don't fail if NUMA & RAD not set.
**				     If running NUMA clusters, only
**				     set this in combination with
**				     CX_NSC_SET_CONTEXT if you are sure
**				     that shared memory won't be accessed,
**				     and all you need is to retreive the
**				     context later (e.g. iirun).
**
**	outbuf  - buffer to receive nodename, or error message text.
**
**	outlen  - limit to # bytes copied to outbuf.
**
** Outputs:
**	
**	*pargc	- decremented by if argument(s) are consumed.
**
**	argv	- Adjused to remove consumed argument(s).
**
**	outbuf  - Holds '\0' terminated string with either nodename
**		  or error message depending on status.
**
**	Returns:
**		OK				- All is well.
**		E_CL2C40_CX_E_BAD_PARAMETER	- Bad params values, 
**						  or bad combination
**						  of parameters.
**		E_CL2C43_CX_E_REMOTE_NODE	- Node specified was not on
**						  current host machine.
**		E_CL2C2F_CX_E_BADRAD		- Rad value out of range.
**		E_CL2C42_CX_E_NOT_CONFIGURED	- Node or RAD not configured
**						  as part of a cluster.
**		E_CL2C41_CX_E_MUST_SET_RAD	- Running NUMA clusters,
**						  and no argument, or
**						  environment variable, set
**					          the RAD context.
**	
**	Exceptions:
**	    none
**
** Side Effects:
**	    See CXnuma_bind_to_rad(), if called with a '-rad'
**	    argument outside a NUMA cluster, else see
**	    CXset_context().
**
** Notes:
**	Typically in a NUMA cluster configuration, the target RAD
**	information is needed very early in the applications logic,
**      since it cannot implicitly determine the virtual node it
**	is intended to be running on.
**
** History:
**	18-sep-2002 (devjo01)
**	    Created.
*/
STATUS
CXget_context( i4 *pargc, char *argv[], i4 flags, char *outbuf, i4 outlen )
{
    STATUS		 status = OK;
    CX_NODE_INFO        *pni = NULL;
    bool		 numa;
    i4			 arg, radarg = 0, nodearg = 0, badarg = 0;
    i4			 target_rad;
    i4			 argstokill = 1;
    char		*pvalue = NULL, *argp, *host;
    char		 lclbuf[256];
    i4			 erargc;
    ER_ARGUMENT	 	 erargs[2];

    host = CXhost_name();

    do
    {
	/* Scan passed arguments for "-rad" & "-node" */
	for ( arg = 1; arg < *pargc; arg++ )
	{
	    argp = argv[arg];
	    if ( '-' != *argp ) continue;

	    if ( 0 == STxcompare(argp, 4, ERx( "-rad" ), 4, FALSE, FALSE) )
	    {
		if ( radarg || nodearg )
		{
		    /* R2 */
		    badarg = arg;
		    break;
		}

		radarg = arg;
		if ( *(argp + 4) == '=' )
		{
		    pvalue = argp + 5;
		}
		else if ( '\0' != *(argp + 4) || 
			  (arg >= (*pargc - 1)) )
		{
		    /* R3 */
		    badarg = arg;
		    break;
		}
		else
		{
		    /* Next argument must be a valid RAD ID */
		    pvalue = argv[++arg];
		    argstokill = 2;
		}
		if ( '\0' == *pvalue )
		{
		    /* R3 */
		    badarg = radarg;
		    break;
		}
		if ( OK != CVan( pvalue, &target_rad ) ||
				  target_rad <= 0 )
		{
		    /* R3 */
		    badarg = arg;
		    break;
		}
	    }
	    else if ( !(CX_NSC_IGNORE_NODE & flags) &&
		      0 == STxcompare(argp, 5, ERx( "-node" ), 5,
				      FALSE, FALSE ) ) 
	    {
		if ( radarg || nodearg )
		{
		    /* R2 */
		    badarg = arg;
		    break;
		}
		nodearg = arg;
		if ( *(argp + 5) == '=' )
		{
		    pvalue = argp + 6;
		}
		else if ( ('\0' != *(argp + 5)) || 
			  (arg >= (*pargc - 1)) )
		{
		    /* R4 */
		    badarg = arg;
		    break;
		}
		else
		{
		    /* Next argument must be a node ID */
		    pvalue = argv[++arg];
		    argstokill = 2;
		}
		if ( '\0' == *pvalue )
		{
		    /* R4 */
		    badarg = nodearg;
		    break;
		}
	    }
	} /* end for */

	/* If bad syntax seen, scram & fail */
	if ( badarg )
	{
	    status = E_CL2C40_CX_E_BAD_PARAMETER;
	    erargc = 1;
	    erargs[0].er_size = 0; erargs[0].er_value = (PTR)argv[badarg];
	    break;
	}

	numa = CXnuma_cluster_configured();

	/* If no explicit parameters seen, try default */
	if ( numa && !pvalue && !(flags & CX_NSC_RAD_OPTIONAL) )
	{
	    /* R5 */
	    argp = ERx( "II_DEFAULT_RAD" );
	    pvalue = getenv( argp );
	    if ( pvalue &&
		 ( ( '\0' == *pvalue ) || OK != CVan(pvalue, &target_rad) ) )
		pvalue = NULL;
	}

	/* Got a value either from command line or environment */
	if ( pvalue )
	{
	    if ( nodearg )
	    {
		if ( CXcluster_configured() )
		{
		    pni = CXnode_info(pvalue, 0);
		    if ( pni )
		    {
			/* If can't be global, make sure its local */
			if ( !(flags & CX_NSC_NODE_GLOBAL) &&
			     !( ( 0 == STxcompare( host, 0,
				 pni->cx_node_name, pni->cx_node_name_l,
				 TRUE, FALSE ) ) ||
				( ( 0 != pni->cx_host_name_l ) &&
				  ( 0 == STxcompare( host, 0,
				    pni->cx_host_name,
				    pni->cx_host_name_l, TRUE, FALSE ) ) )
			      )
			   )
			{
			    /* Node must be local and was not.  (R4a) */
			    status = E_CL2C43_CX_E_REMOTE_NODE;
			    erargc = 2;
			    erargs[0].er_size = 0;
			    erargs[0].er_value = (PTR)argv[0];
			    erargs[1].er_size = pni->cx_host_name_l;
			    erargs[1].er_value = (PTR)(pni->cx_host_name);
			    break;
			}
		    }
		}
	    }
	    else if ( numa )
	    {
		pni = CXnode_info(host, target_rad);
	    }
	    else if ( target_rad > CXnuma_rad_count() )
	    {
		if ( !(flags & CX_NSC_RAD_OPTIONAL) )
		{
		    /* R3 */
		    status = E_CL2C2F_CX_E_BADRAD;
		    erargc = 0;
		    break;
		}
		target_rad = 0;
	    }

	    if ( ( numa || nodearg ) && ( NULL == pni ) )
	    {
		/* Requested node/rad not configured */
		/* R3, R4 */
		if ( !argp ) argp = argv[radarg+nodearg];
		status = E_CL2C42_CX_E_NOT_CONFIGURED;
		erargc = 1;
		erargs[0].er_size = 0; erargs[0].er_value = (PTR)argp;
		break;
	    }
	}
	else
	{
	    if ( numa && !(flags & CX_NSC_RAD_OPTIONAL) )
	    {
		/* If required argument missing, scram & fail */
		status = E_CL2C41_CX_E_MUST_SET_RAD;
		erargc = 1;
		erargs[0].er_size = 0; erargs[0].er_value = (PTR)argv[0];
		break;
	    }
	}

	if ( (flags & CX_NSC_CONSUME) && (radarg || nodearg) )
	{
	    for ( arg = radarg + nodearg + argstokill; arg < *pargc; arg++ )
	    {
		argv[arg-argstokill] = argv[arg];
	    }
	    *pargc = *pargc - argstokill;
	    argv[*pargc] = NULL;
	}

    } while (0);	

    if ( OK == status )
    {
	lclbuf[CX_MAX_NODE_NAME_LEN] = '\0';
	if ( NULL == pni )
	{
	    (void)STlcopy(host, lclbuf, CX_MAX_NODE_NAME_LEN);
	}
	else
	{
	    (void)STlcopy(pni->cx_node_name, lclbuf, CX_MAX_NODE_NAME_LEN);
	}
	CVlower( lclbuf );
	if ( flags & CX_NSC_SET_CONTEXT )
	{
	    if ( !numa && 0 != target_rad )
	    {
		(void)CXnuma_bind_to_rad( target_rad );
	    }
	    (void)CXset_context(lclbuf, 0);
	}
    }

    if ( OK != status )
    {
	cx_fmt_msg( status, lclbuf, sizeof(lclbuf), erargc, erargs );
        if ( flags & CX_NSC_REPORT_ERRS )
	    (void)SIprintf( "\n%s\n", lclbuf );
    }

    if ( outbuf )
    {
	STlcopy( lclbuf, outbuf, outlen - 1 );
	*(outbuf + outlen - 1) = '\0';
    }
    return status;
} /*CXget_context*/



/*{
** Name: CXset_context	- Set the CX node "context".
**
** Description:
**
**	If configured for clusters, the CX "context" determines
**	what node the calling utility is bound to.   That is,
**	what directory is used when mapping shared segments, how
**	file names are "decorated", and It determines the default
**	PM hostname, and if running NUMA clusters, the host name
**	override.
**
** Inputs:
**	host	- Node name or host name for the context.
**		  If NULL, value from PHhost is used.
**	rad	- If running NUMA clusters, 1st argument must be the host
**		  name, (or NULL), and a node name is constructed from
**		  the hostname(PMhost)/rad pair as "R<radid>_<hostname>"
**		  If not running NUMA clusters, this must be zero.
**
** Outputs:
**	none.
**
**	Returns:
**		OK				- Context was set.
**		E_CL2C2F_CX_E_BADRAD		- Rad value out of range.
**		E_CL2C42_CX_E_NOT_CONFIGURED	- Node or RAD not configured
**						  as part of the cluster.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Important & plentyful, see description.
**
** History:
**	16-sep-2002 (devjo01)
**	    Created.
*/
STATUS
CXset_context( char * host, i4 rad )
{
#   define HOST_PM_IDX	 1

    STATUS		 status = OK;
    CX_NODE_INFO        *pni;
    char		 namebuf[ CX_MAX_NODE_NAME_LEN + 1 ];

    do
    {
	if ( !host )
	{
	    host = CXhost_name();
	}

	if ( CXcluster_configured() )
	{
	    pni = CXnode_info( host, rad );
	    if ( NULL == pni )
	    {
		status = E_CL2C42_CX_E_NOT_CONFIGURED;
		break;
	    }

	    namebuf[CX_MAX_NODE_NAME_LEN] = '\0';
	    STlcopy( pni->cx_node_name, namebuf, CX_MAX_NODE_NAME_LEN );
	    CVlower( namebuf );
	    STcopy(namebuf, cx_saved_node_name);

	    if ( pni->cx_host_name_l &&
		 ( pni->cx_host_name_l != pni->cx_node_name_l ||
		   0 != STxcompare( pni->cx_node_name, pni->cx_node_name_l,
				    pni->cx_host_name, pni->cx_host_name_l,
				    TRUE, FALSE ) ) )
	    {
		STlcopy( pni->cx_host_name, namebuf, CX_MAX_NODE_NAME_LEN );
		CVlower( namebuf );
		PMsetDefault( HOST_PM_IDX, namebuf );
	    }
	    else
	    {
		PMsetDefault( HOST_PM_IDX, namebuf );
	    }
	    if ( 0 != pni->cx_rad_id )
		CX_Proc_CB.cx_numa_user_rad = pni->cx_rad_id;
	    CX_Proc_CB.cx_numa_cluster_rad = pni->cx_rad_id;
	}
	else
	{
	    PMsetDefault( HOST_PM_IDX, host );
	    if ( 0 != rad )
		CX_Proc_CB.cx_numa_user_rad = rad;
	}
    } while (0);
    return status;
} /* CXset_context */


# if 0	/* Faux-pas.  Was going to put this in PCspawn, and parse arguments
	   for invocations of Ingres utilities touching shared memory, but
	   thought better of it.  Difficulties envisioned were maintaining
	   this list, trapping cases where -rad argument was already given,
	   but most importantly, parsing out the utility name was not a no-
	   brainer, since it could be an argument to a shell script, and 
	   thus could appear anywhere in argument list to PCspawn.

	   Instead I bit the bullet, and examine & modifed as needed each
	   call to PCcmdline, PCspawn, etc.
	*/


/*{
** Name: CXexec	- execvp wrapper which adds -rad arg to certain utils if needed
**
** Description:
**
**	If CX context has been called, and caller is running on a
**	NUMA cluster configured host, add -rad=<radid> to establish
**	RAD context.
**
** Inputs:
**
** Outputs:
**	none.
**
**	Returns:
**		Doesn't!
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    execvp call replaces current executing image with specified
**	    image called with specified arguments.
**
** History:
**	16-sep-2002 (devjo01)
**	    Created.
*/
void
CXexec( char * image, char *argv[] )
{
    /*
    ** Ingres utilities to add "-rad" to if required.
    ** KEEP list in alpha order.
    */
    static	char	*image_table[] = {
	ERx( "ckpdb" ),
	ERx( "cscleanup" ),
	ERx( "csinstall" ),
	ERx( "cscleanup" ),
	ERx( "iimklog" ),
	ERx( "ipcclean" ),
    };

    /* More parameters than are needed by
       any of the executables of interest */
#   define	 MAX_CX_ARGS	15

    i4		 hi, lo, mid, res, argc;
    char	*ip;
    char	*bigargv[MAX_CX_ARGS+1];
    char	 affinity_arg[16];

    if ( CX_Proc_CB.cx_numa_cluster_rad )
    {
	/* Scan 'image' for last directory delimiter */
	ip = image + STlength(image);
	while ( --ip >= image && *ip != '/' ) /* do nothing */ ;
	ip++;

	/* See if image is in list of executables requiring '-rad' */
	lo = 0; hi = sizeof(image_table)/sizeof(image_table[0]);
	do
	{
	    mid = (lo + hi) / 2;
	    res = ( STcompare(ip, image_table[mid]) );
	    if ( res < 0 )
		hi = mid - 1;
	    else if ( res > 0 )
		lo = mid + 1;
	    else
	    {
		/*
		** We have a match, make sure -rad, or -node
		** is already specified.
		*/
		argc = 0;
		while ( ++argc < MAX_CX_ARGS && argv[argc] )
		{
		    if ( 0 == STxcompare(argv[argc], 5,
					 ERx( "-node" ), 5, FALSE, FALSE ) ||
			 0 == STxcompare(argv[argc], 4,
					 ERx( "-rad" ), 4, FALSE, FALSE ) )
		    { 
			/* Already specified */
			break;
		    }
		}
		if ( argc <= MAX_CX_ARGS && NULL == argv[argc] )
		{
		    /*
		    ** Room for arg & did'nt find it.  Copy it into
		    ** a buffer guaranteed large enough to hold it.
		    */
		    STprintf( affinity_arg, ERx( "-rad=%d" ), 
		     CX_Proc_CB.cx_numa_cluster_rad );
		    bigargv[0] = argv[0];
		    bigargv[1] = affinity_arg;
		    MEcopy( (PTR)(argv + 1), argc * sizeof(argv[0]), 
			    (PTR)(bigargv + 2) );
		    argv = bigargv;
		}
		break;
	    }
	} while ( hi >= lo );
    }
    execvp( image, argv );
} /* CXexec */


# endif /*0*/


#if defined(axp_osf) || defined(LNX)

#include    <dlfcn.h>

/*{
** Name: cx_dl_load_lib - resolves symbols in OS shared libraries.
**
** Description:
**
**	We manually resolve addresses to functions in certain shared
**	libraries, notably libimc.so, libnuma.so, & libdlm.so.
**
**	This is because not all OS levels or Ingres configurations
**	support or need these functions.
**
** Inputs:
**
** Outputs:
**	none.
**
**	Returns:
**		
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	??-???-???? (devjo01)
**	    Created.
**	06-jan-2004 (devjo01)
**	    Rename cx_axp_dl_load_lib to cx_dl_load_lib and make it
**	    available to Linux.
*/

/*
** Stub function to avoid leaving NULL function pointers in case
** of dynamic load failure.   Function just returns error code.
*/
static STATUS
cx_dl_null_func( void )
{
    return E_CL2C33_CX_E_OS_NOINIT;
} /* cx_dl_null_func */

/*
** Resolve function addessed for dynamically loaded functions.
*/
STATUS 
cx_dl_load_lib( char *libname, void **plibhandle,
		   void *func_array[], char *name_array[], int numentries )
{
    STATUS	 status = OK;
    void	*libhandle;
    i4		 i;

    libhandle = dlopen( libname, RTLD_LAZY );
    if ( NULL == libhandle )
    {
	status = E_CL2C33_CX_E_OS_NOINIT;
    }
    else
    {
	for ( i = 0; i < numentries; i++ )
	{
	    func_array[i] = dlsym( libhandle, name_array[i] );
	    if ( NULL == func_array[i] )
	    {
		/* could not resolve all functions */
		status = E_CL2C35_CX_E_OS_MISC_ERR;
		break;
	    }
	}
    }

    if ( OK != status )
    {
	for ( i = 0; i < numentries; i++ )
	{
	    func_array[i] = (void *)cx_dl_null_func;
	}
	if ( libhandle )
	    (void)dlclose(libhandle);
    }
    else
    {
	*plibhandle = libhandle;
    }

    return status;
} /* cx_dl_load_lib */

#endif /* axp_osf, intlnx */


/*{
** Name: cx_is_config_value_on	- Return true if a config paramter is "ON".
**
** Description:
**
**	Common subroutine for ON/OFF parameter checkers. 
**
** Inputs:
**	char *	param_suffix - param to check, excluding "ii.<hostname>."
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- Parameter is "ON".
**		FALSE	- Parameter is missing or not "ON"
**			  (presumably "OFF").
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-sep-2002 (devjo01)
**	    Created.
*/
static bool
cx_is_config_value_on( char * param_suffix )
{
    char	*pvalue;
    char	 parambuf[80];

    /* Call PMinit, just in case. */
    if ( OK == PMinit() )
    {
	/* Load parameters if PMinit did not return "dup init". */
	(void)PMload((LOCATION *)NULL, (PM_ERR_FUNC *)0);
    }

    /* Check to see if passed parameter is "ON" for this node */
    (void)STprintf( parambuf, ERx("ii.%s.%s"), 
	  CXhost_name(), param_suffix );
    return ( OK == PMget( parambuf, &pvalue ) && 
	 0 == STxcompare( pvalue, 0, "ON", 0, TRUE, FALSE ) );
} /* cx_is_config_value_on */


/*{
** Name: cx_fmt_msg	- lookup and format a message string.
**
** Description:
**
**	mini-ule_format().
**
** Inputs:
**	msgcode		- message to lookup.
**	buffer		- buffer to receive formatted text.
**	buflen		- size of buffer.
**	er_argc		- ER_ARGUMENT entries in er_argv array.
**	er_argv		- pointer to an array of ER_ARGUMENT structs.
**
** Outputs:
**	none.
**
**	Returns:
**		ptr to node info rec or NULL.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-sep-2002 (devjo01)
**	    Created.
*/
void
cx_fmt_msg( i4 msgcode, char *buffer, i4 buflen,
	    i4 erargc, ER_ARGUMENT *erargv )
{
#define FIXME_LANGUAGE	1

    CL_ERR_DESC	 sys_err;
    i4		 len;

    (void)ERslookup( msgcode, NULL, ER_TEXTONLY, NULL,
		     buffer, buflen, FIXME_LANGUAGE, &len, &sys_err,
		     erargc, erargv );
}

