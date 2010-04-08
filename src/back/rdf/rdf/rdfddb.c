/*
**Copyright (c) 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<me.h>
#include	<st.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include        <cs.h>
#include	<scf.h>
#include	<adf.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefcb.h>
#include	<qefscb.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<ulh.h>
#include	<qefqeu.h>
#include	<rqf.h>
#include	<rdf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<cm.h>

/* prototypes */
/* rdf_attrloc	- build local object attributes descriptors. */
static DB_STATUS rdf_attrloc(RDF_GLOBAL *global);

/* rdf_attcount	- get number of attributes. */
static DB_STATUS rdf_attcount(RDF_GLOBAL *global);

/*
**  Global references
*/
GLOBALREF char	Iird_upper_systab[];  /* upper case system catalog we
				      ** use to obtain catalog owner name 
				      */
GLOBALREF char	Iird_lower_systab[];  /* lower case system catalog we
				      ** use to obtain catalog owner name 
				      */

/**
**
**  Name: RDFDDB.C - Contains routines support distributed session startup and
**		     local table and attribute accesses.
**
**  Description:
**	This file contains routines support session startup and local table/attribute
**  access.
**
**	    rdf_ddbinfo - get access status of ddb. 
**	    rdf_tabinfo - retrieve local table info and attribute info if requested.
**	    rdf_ldbplus - retrieve ldb information.
**	    rdf_usrstat - retrieve user access status.
**	    rdf_iidbdb  - retrieve iidbdb information.
**	    rdf_ddbflush - flush ddb descriptor from rdf cache
**	    rdf_usuper - retrieve server wide user status
**	    rdf_clusterinfo - retrieve cluster information 
**	    rdf_open_cdbasso - force open privilege cdb association
**
**  History:
**	10-aug-88 (mings)
**	    written
**	10-aug-88 (mings)
**	    add rdf_open_cdbasso routine to force open privilege cdb association
**      17-jun-91 (fpang)
**          Initialized qef_rcb->qef_modifier, causes qef to get confused
**          about the tranxasction state if it is not initialized.
**          Fixes B37913.
**	20-dec-91 (teresa)
**	    removed include of qefddb.h since that is now part of qefrcb.h.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**      8-aug-93 (shailaja)
**          Unnested comments.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions.
**      17-Jun-2005 (lunbr01)  bug 114681
**          Change "!=" to "<>" to make OpenSQL compliant; else,  
**          get errors trying to register gateway and EDBC servers.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**/

/*{
** Name: rdf_open_cdbasso -  force open privilege cdb association.
**
** Description:
**	send a dummy select query through privilege cdb association to
**	force open a privilege association. 
**
**	Note that this is routine should be called by scf only when ddbinfo
**      is found in scf ddbinfo cache. 
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to DD_1LDB_INFO
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-jun-89 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
**	07-dec-92 (teresa)
**	    fix AV that arises because we call RDD_QUERY (which uses session
**	    CTRL block to see if a trace point is set.  But this routine is
**	    called before SCF has given us a session control block, so zero the
**	    PTR to the session control block.
**	10-Jan-2001 (jenjo02)
**	    Removed above change. global->rdf_sess_cb has
**	    been properly initialized by rdf_call().
*/
DB_STATUS
rdf_open_cdbasso(RDF_GLOBAL *global)
{
    DB_STATUS           status;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_NAME		result;
    DD_1LDB_INFO	ldbinfo;
    DD_PACKET           ddpkt;
    RQB_BIND    	rq_bind[RDD_01_COL],	/* for 1 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

#ifdef xDEBUG
    if (!(global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p))
    {
	/* scf must pass cdb info through this point */

	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
#endif
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select 'junk'
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	STprintf(qrytxt, "select 'open privilege cdb association';");

    STRUCT_ASSIGN_MACRO(*global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p,
			ldbinfo);

    /* use privilege association */
    ldbinfo.dd_i1_ldb_desc.dd_l1_ingres_b = TRUE;
    ddr_p->qer_d2_ldb_info_p = &ldbinfo; 
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
    {
	if (global->rdfcb->rdf_error.err_code == E_RD0277_CANNOT_GET_ASSOCIATION)
	    rdu_ierror(global, status, E_RD0277_CANNOT_GET_ASSOCIATION);
	return (status); 
    }
    rq_bind[0].rqb_addr = (PTR)result; 
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

    /* Fetch user status from iiuser */
    status = rdd_fetch(global);

    if (DB_FAILURE_MACRO(status))
	return(status);

    return(rdd_flush(global));
}

/*{
** Name: rdf_clusterinfo - cluster information request.
**
** Description:
**      This function returns cluster info to the caller.
**
** Inputs:
**      global				rdf global state
**		    
** Outputs:
**      global
**		.rdfcb
**		    .rdf_rb             rdf request control block
**			.rdr_r2_ddb_req
**			    .dd_d8_nodes
**					number of nodes in the cluster, zero if
**					not available
**			    .dd_d9_cluster_p
**					pointer to the cluster info
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-mar-89 (mings)
**          initial creation
**	03-oct-90 (teresa)
**	    add logic to populate DD_CLUSTER_INFO structure.
**	16-jul-92 (teresa)
**	    prototypes
[@history_template@]...
*/
DB_STATUS
rdf_clusterinfo(RDF_GLOBAL *global)
{
    RDR_DDB_REQ		*ddbreq = &global->rdfcb->rdf_rb.rdr_r2_ddb_req;
    
    ddbreq->rdr_d8_nodes = Rdi_svcb->rdv_num_nodes; /* number of nodes */
    ddbreq->rdr_d9_cluster_p = Rdi_svcb->rdv_cluster_p;
					    /* pointer to cluster info */
    ddbreq->rdr_d10_node_p = Rdi_svcb->rdv_node_p;
					    /* pointer to node info */	    
    ddbreq->rdr_d11_net_p = Rdi_svcb->rdv_net_p;
					    /* pointer to net info */	    
    /*
    ** populate DD_CLUSTER_INFO structure with cluster info that is also
    ** in rdr_d8_nodes and rdr_d9_cluster_p
    */
    ddbreq->rdr_d12_cluster.dd_nodenames = (DD_NODENAMES *)Rdi_svcb->rdv_cluster_p;
    ddbreq->rdr_d12_cluster.dd_node_count = Rdi_svcb->rdv_num_nodes;
    return (E_DB_OK);
}

/*{
** Name: rdf_usuper - retrieve server wide user status
**
** Description:
**	The routine send query to retrieve user status from iiuser in iidbdb.
**	It is called by scf (iimonitor session) to check whether user is 
**	ingres super user.
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to DD_1LDB_INFO
**			.rdr_d4_usr_desc_p  ptr to DD_USR_DESC
**			    .dd_u1_usrname  user name
**
** Outputs:
**		    .rdr_r2_ddb_req
**			.rdr_d7_userstat    user status
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-mar-89 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
**	07-dec-92 (teresa)
**	    fix AV that arises because we call RDD_QUERY (which uses session
**	    CTRL block to see if a trace point is set.  But this routine is
**	    called before SCF has given us a session control block, so zero the
**	    PTR to the session control block.
**	10-Jan-2001 (jenjo02)
**	    Removed 07-dec-92 change. global->rdf_sess_cb has
**	    been properly initialized by rdf_call().
*/
DB_STATUS
rdf_usuper(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    RDR_DDB_REQ		*ddbreq = &global->rdfcb->rdf_rb.rdr_r2_ddb_req;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    RQB_BIND    	rq_bind[RDD_01_COL],	/* for 1 columns */
			*bind_p = rq_bind;
    char		usrname[DB_MAXNAME + 1];
    char		qrytxt[RDD_QRY_LENGTH];

#ifdef xDEBUG
    if (!(ddbreq->rdr_d2_ldb_info_p) ||	!(ddbreq->rdr_d4_usr_desc_p))
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
#endif

    /* Get the user status from iiuser in iidbdb */

    MEcopy((PTR)ddbreq->rdr_d4_usr_desc_p->dd_u1_usrname, sizeof(DD_NAME),
			(PTR)usrname);
    usrname[DB_MAXNAME] = EOS;

    (VOID)STtrmwhite(usrname);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select status
    **	       from iiuser where name = 'usrname'
    **	
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	STprintf(qrytxt, "%s '%s';",
	    "select status from iiuser where name =",
            usrname);

    ddr_p->qer_d2_ldb_info_p = ddbreq->rdr_d2_ldb_info_p;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    rq_bind[0].rqb_addr = (PTR)&ddbreq->rdr_d7_userstat; 
    rq_bind[0].rqb_length = sizeof(ddbreq->rdr_d7_userstat);
    rq_bind[0].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

    /* Fetch user status from iiuser */
    status = rdd_fetch(global);

    if (DB_SUCCESS_MACRO(status))
	status = rdd_flush(global);

    return(status);
}

/*{
** Name: rdf_netnode_cost - retrieve net cost info from iidbdb
**
** Description:
**      This function send out query for net cost info.
**
** Inputs:
**      global				rdf global state
**		    
** Outputs:
**      global
**		.rdfcb
**		    .rdf_rb             rdf request control block
**			.rdr_r2_ddb_req
**			    .dd_d8_nodes
**					number of nodes in the cluster, zero if
**					not available
**			    .dd_d9_cluster_p
**					pointer to the cluster info
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-mar-89 (mings)
**          initial creation
**      14-jun-89 (mings)
**          initialize timeout to 600 seconds and also init dv count value to 0
**	20-jun-90 (teresa)
**	    add new parameter to rdu_ferror to fix bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
**	07-dec-92 (teresa)
**	    fix AV that arises because we call RDD_QUERY (which uses session
**	    CTRL block to see if a trace point is set.  But this routine is
**	    called before SCF has given us a session control block, so zero the
**	    PTR to the session control block.
**	17-feb-93 (teresa)
**	    add sem_type (RDF_BLD_LDBDESC) to rdd_setsem/rdd_resetsem interface.
**	15-Aug-1997 (jenjo02)
**	    Removed reference to rdf_scfcb and *sem, neither of which were
**	    being used.
**	10-Jan-2001 (jenjo02)
**	    Removed 07-dec-92 change. global->rdf_sess_cb has
**	    been properly initialized by rdf_call().
[@history_template@]...
*/
DB_STATUS
rdf_netnode_cost(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    ULM_RCB		*ulmrcb = &global->rdf_ulmcb;
    DD_NETCOST		netcost;
    DD_COSTS		cpucost;
    DD_NETLIST		*netlist_p = (DD_NETLIST *)NULL;
    DD_NODELIST		*cpulist_p = (DD_NODELIST *)NULL;
    DD_PACKET           ddpkt;
    RQB_BIND    	rq_bind[RDD_11_COL],	/* for 11 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];
    
    /* Get semaphore */

    status = rdd_setsem(global,RDF_BLD_LDBDESC);

    if (DB_FAILURE_MACRO(status))
	return(status);
	    
    /* just in case that other thread got here already.
    ** note that a window is opened here.  If multi treads
    ** are geting here at same time,  problem may happen. 
    ** The reason of this is that we don't want to hold a
    ** semaphore when we are sending sql query out.  Anyway,
    ** this should a very rare case.  By setting timeout in
    ** the query will resolve this problem. */ 

    if (Rdi_svcb->rdv_cost_set == RDV_1_COST_SET)
	return(rdd_resetsem(global,RDF_BLD_LDBDESC));

    Rdi_svcb->rdv_cost_set = RDV_1_COST_SET;

    /* release semaphore */

    status = rdd_resetsem(global,RDF_BLD_LDBDESC);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* use the same memory stream as cluster info for both
    ** net cost and node cost information */

    ulmrcb->ulm_poolid = Rdi_svcb->rdv_poolid;
    ulmrcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
    ulmrcb->ulm_streamid_p = &Rdi_svcb->rdv_cls_streamid;
    ulmrcb->ulm_facility = DB_RDF_ID;    /* rdf's id */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query for net cost 
    **
    **	select * from iiddb_netcost order by net_src
    **	
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	STprintf(qrytxt, "select * from iiddb_netcost order by net_src;");

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = qefrcb->qef_r3_ddb_req.qer_d1_ddb_p->dd_d4_iidbdb_p;
    ddr_p->qer_d4_qry_info.qed_q1_timeout = 600;
    ddr_p->qer_d4_qry_info.qed_q2_lang = DB_SQL;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p3_nxt_p = (DD_PACKET *)NULL;
    ddr_p->qer_d4_qry_info.qed_q5_dv_cnt = 0;
    ddr_p->qer_d4_qry_info.qed_q6_dv_p = (DB_DATA_VALUE *)NULL;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;

    /* Send query to iidbdb for net cost info */
    status = qef_call(QED_1RDF_QUERY, ( PTR ) qefrcb);	    

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
		   E_RD0250_DD_QEFQUERY,0); 
	return(status);
    }

    global->rdf_ddrequests.rdd_ddstatus |= RDD_FETCH; /* set status */ 

    rq_bind[0].rqb_addr = (PTR)netcost.net_source; 
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[1].rqb_addr = (PTR)netcost.net_dest;
    rq_bind[1].rqb_length = sizeof(DD_NAME);
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[2].rqb_addr = (PTR)&netcost.net_cost; 
    rq_bind[2].rqb_length = sizeof(netcost.net_cost);
    rq_bind[2].rqb_dt_id = DB_FLT_TYPE;
    rq_bind[3].rqb_addr = (PTR)&netcost.net_exp1;
    rq_bind[3].rqb_length = sizeof(netcost.net_exp1);
    rq_bind[3].rqb_dt_id = DB_FLT_TYPE;
    rq_bind[4].rqb_addr = (PTR)&netcost.net_exp2;
    rq_bind[4].rqb_length = sizeof(netcost.net_exp2);
    rq_bind[4].rqb_dt_id = DB_FLT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_05_COL;

    do
    {
	/* Fetch next tuple */
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    /* allocate memory space */
	    ulmrcb->ulm_psize = sizeof(DD_NETLIST);

	    status = ulm_palloc(ulmrcb);

	    if (DB_FAILURE_MACRO(status))
	    {
		rdu_ferror(global, status, &ulmrcb->ulm_error,
			       E_RD0127_ULM_PALLOC,0);
		return(status);
	    }
		
	    /* is this the first block */

	    if (netlist_p)
	    {
		/* no, this is not the first block. link it to the previous
		** node info. */
    
		netlist_p->net_next = (DD_NETLIST *)ulmrcb->ulm_pptr;	
	    }
	    else
	    {
		/* this is the first node so save it in the server control block */
		Rdi_svcb->rdv_net_p = (DD_NETLIST *)ulmrcb->ulm_pptr;
	    }

	    netlist_p = (DD_NETLIST *)ulmrcb->ulm_pptr;
	    netlist_p->net_next = (DD_NETLIST *)NULL;

	    STRUCT_ASSIGN_MACRO(netcost, netlist_p->netcost);
	}

    } while (ddr_p->qer_d5_out_info.qed_o3_output_b);

    /* set up query for node cost 
    **
    **	select * from iiddb_nodecosts order by cpu_name
    **	
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	STprintf(qrytxt, "select * from iiddb_nodecosts order by cpu_name;");

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = qefrcb->qef_r3_ddb_req.qer_d1_ddb_p->dd_d4_iidbdb_p;
    ddr_p->qer_d4_qry_info.qed_q1_timeout = 600;
    ddr_p->qer_d4_qry_info.qed_q2_lang = DB_SQL;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p3_nxt_p = (DD_PACKET *)NULL;
    ddr_p->qer_d4_qry_info.qed_q5_dv_cnt = 0;
    ddr_p->qer_d4_qry_info.qed_q6_dv_p = (DB_DATA_VALUE *)NULL;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;

    /* Send query to iidbdb for net cost info */
    status = qef_call(QED_1RDF_QUERY, ( PTR ) qefrcb);	    

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
		   E_RD0250_DD_QEFQUERY,0); 
	return(status);
    }

    global->rdf_ddrequests.rdd_ddstatus |= RDD_FETCH; /* set status */ 

    rq_bind[0].rqb_addr = (PTR)cpucost.cpu_name;
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[1].rqb_addr = (PTR)&cpucost.cpu_power;
    rq_bind[1].rqb_length = sizeof(cpucost.cpu_power);
    rq_bind[1].rqb_dt_id = DB_FLT_TYPE;
    rq_bind[2].rqb_addr = (PTR)&cpucost.dio_cost; 
    rq_bind[2].rqb_length = sizeof(cpucost.dio_cost);
    rq_bind[2].rqb_dt_id = DB_FLT_TYPE;
    rq_bind[3].rqb_addr = (PTR)&cpucost.create_cost;
    rq_bind[3].rqb_length = sizeof(cpucost.create_cost);
    rq_bind[3].rqb_dt_id = DB_FLT_TYPE;
    rq_bind[4].rqb_addr = (PTR)&cpucost.page_size;
    rq_bind[4].rqb_length = sizeof(cpucost.page_size);
    rq_bind[4].rqb_dt_id = DB_INT_TYPE;
    rq_bind[5].rqb_addr = (PTR)&cpucost.block_read;
    rq_bind[5].rqb_length = sizeof(cpucost.block_read);
    rq_bind[5].rqb_dt_id = DB_INT_TYPE;
    rq_bind[6].rqb_addr = (PTR)&cpucost.sort_size;
    rq_bind[6].rqb_length = sizeof(cpucost.sort_size);
    rq_bind[6].rqb_dt_id = DB_INT_TYPE;
    rq_bind[7].rqb_addr = (PTR)&cpucost.cache_size; 
    rq_bind[7].rqb_length = sizeof(cpucost.cache_size);
    rq_bind[7].rqb_dt_id = DB_INT_TYPE;
    rq_bind[8].rqb_addr = (PTR)&cpucost.cpu_exp0;
    rq_bind[8].rqb_length = sizeof(cpucost.cpu_exp0);
    rq_bind[8].rqb_dt_id = DB_INT_TYPE;
    rq_bind[9].rqb_addr = (PTR)&cpucost.cpu_exp1;
    rq_bind[9].rqb_length = sizeof(cpucost.cpu_exp1);
    rq_bind[9].rqb_dt_id = DB_FLT_TYPE;
    rq_bind[10].rqb_addr = (PTR)&cpucost.cpu_exp2;
    rq_bind[10].rqb_length = sizeof(cpucost.cpu_exp2);
    rq_bind[10].rqb_dt_id = DB_FLT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_11_COL;

    do
    {
	/* Fetch next tuple */

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    /* allocate memory space for node cost */

	    ulmrcb->ulm_psize = sizeof(DD_NODELIST);

	    status = ulm_palloc(ulmrcb);

	    if (DB_FAILURE_MACRO(status))
	    {
		rdu_ferror(global, status, &ulmrcb->ulm_error,
			       E_RD0127_ULM_PALLOC,0);
		return(status);
	    }
		
	    /* is this the first block */

	    if (cpulist_p)
	    {
		/* no, this is not the first block. link it to the previous
		** node info. */
    
		cpulist_p->node_next = (DD_NODELIST *)ulmrcb->ulm_pptr;	
	    }
	    else
	    {
		/* this is the first node so save it in the server control block */
		Rdi_svcb->rdv_node_p = (DD_NODELIST *)ulmrcb->ulm_pptr;
	    }

	    cpulist_p = (DD_NODELIST *)ulmrcb->ulm_pptr;
	    cpulist_p->node_next = (DD_NODELIST *)NULL;

	    STRUCT_ASSIGN_MACRO(cpucost, cpulist_p->nodecosts);

	}

    } while (ddr_p->qer_d5_out_info.qed_o3_output_b);

    
    return(E_DB_OK);	    
}

/*{
** Name: rdf_iidbdb	- call QEF to retrieve control information for accessing database IIDBDB.
**
** Description:
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to DD_1LDB_INFO
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	    add new parameter to rdu_ferror to fix bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
**	25-mar-93 (barbara)
**	    Pass pointer to DD_DDB_DESC to QEF so that case translation
**	    semantics of iidbdb may be recorded by QEF.
*/
DB_STATUS
rdf_iidbdb(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    RDR_DDB_REQ		*ddbreq = &global->rdfcb->rdf_rb.rdr_r2_ddb_req;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;

#ifdef xDEBUG
    if (!ddbreq->rdr_d2_ldb_info_p)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
#endif

    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_modifier = 0;
    qefrcb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;  /* set time out to 0 */
    qefrcb->qef_r3_ddb_req.qer_d2_ldb_info_p = ddbreq->rdr_d2_ldb_info_p;
    qefrcb->qef_r3_ddb_req.qer_d1_ddb_p = ddbreq->rdr_d3_ddb_desc_p;

    status = qef_call(QED_IIDBDB, ( PTR ) qefrcb);

    if (DB_FAILURE_MACRO(status))
	rdu_ferror(global, status, &qefrcb->error,
		   E_RD0262_QEF_IIDBDB,0);

    return(status);
}

/*{
** Name: rdf_usrstat	- get user's DB access status.
**
** Description:
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d4_usr_desc_p  ptr to DD_USR_DESC
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	    add new parameter to rdu_ferror to fix bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdf_usrstat(RDF_GLOBAL *global)
{
    DB_STATUS           status;
    QEF_RCB		*qefrcb;

    qefrcb = &global->rdf_qefrcb;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_modifier = 0;
    qefrcb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;
    qefrcb->qef_r3_ddb_req.qer_d2_ldb_info_p =                         /* pointer to ldb desc */
		     global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p;
    qefrcb->qef_r3_ddb_req.qer_d8_usr_p =
		global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d4_usr_desc_p;

    status = qef_call(QED_USRSTAT, ( PTR ) qefrcb);

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	       E_RD0261_QEF_USRSTAT,0);
    }
    return(status);
}

/*{
** Name: rdf_ldbplus	-  call QEF to retrieve LDB's dba name and capabilities.
**
** Description:
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to DD_1LDB_INFO
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	    add new parameter to rdu_ferror to fix bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
**	06-mar-96 (nanpr01)
**	    inquire buffer mgr capabilities after finding out ldbplus info. 
**	20-jun-1997 (nanpr01)
**	    Bug 81889 : Commit all the open sql statements.
*/
DB_STATUS
rdf_ldbplus(RDF_GLOBAL *global)
{
    DB_STATUS           status, x_status;
    QEF_RCB		*qefrcb;

    qefrcb = &global->rdf_qefrcb;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_modifier = 0;
    qefrcb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;
    qefrcb->qef_r3_ddb_req.qer_d2_ldb_info_p = 
		global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p;

    status = qef_call(QED_LDBPLUS, ( PTR ) qefrcb);

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	       E_RD0263_QEF_LDBPLUS,0);
    }
    else
    {
      DD_0LDB_PLUS        *ldbplus_p = 
       &global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p->dd_i2_ldb_plus;
      if ((ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl > 605) &&
        (MEcmp(ldbplus_p->dd_p3_ldb_caps.dd_c7_dbms_type, "INGRES", 6) == 0))
      {
	status = rdd_buffcap(global, 
		     global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p);
	x_status = rdd_commit(global);
	if ((status == E_DB_OK) && (x_status != E_DB_OK))
	  status = x_status;

      }
      else
      {
        i4 j;
 
        ldbplus_p->dd_p9_pagesize[0] = DB_MIN_PAGESIZE;
        ldbplus_p->dd_p8_maxpage = DB_MIN_PAGESIZE;
        ldbplus_p->dd_p10_tupsize[0] = DB_MAXTUP;
        /* Initialize rest */
        for (j = 1; j < DB_MAX_CACHE; j++)
        {
          ldbplus_p->dd_p9_pagesize[j] = 0;
          ldbplus_p->dd_p10_tupsize[j] = 0;
        }
        ldbplus_p->dd_p7_maxtup = DB_MAXTUP;
      }
    }

    return(status);
}

/*{
** Name: rdf_userlocal	- get the user name from ldb.
**
** Description:
**      This routine retrieves local user name.
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to ldb desc
**			    .dd_i2_ldb_plus DD_0LDB_PLUS
**
** Outputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**			    .dd_t2_owner_name
**					    local user name
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
[@history_template@]...
*/
DB_STATUS
rdf_userlocal(	RDF_GLOBAL         *global,
		DD_NAME		   user_name)
{
    DB_STATUS		status;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    DD_NAME		username;
    RQB_BIND    	rq_bind[RDD_01_COL],	/* for 1 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select dbmsinfo('username')
    **	
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
    	    STprintf(qrytxt, "select user_name from iidbconstants;");

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    status = rdd_query(global);	    /* send query to qef */

    if (DB_FAILURE_MACRO(status))
	return (status); 

    rq_bind[0].rqb_addr = (PTR)username; 
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

    /* Fetch next tuple */

    status = rdd_fetch(global);

    if (DB_FAILURE_MACRO(status))
	return(status);

    if (ddr_p->qer_d5_out_info.qed_o3_output_b)
    {
	MEcopy((PTR)username, sizeof(DD_NAME), (PTR)user_name); 
	status = rdd_flush(global);
    }
    else
	SETDBERR(&global->rdfcb->rdf_error, 0, E_RD026C_UNKNOWN_USER);

    return(status);
}

/*{
** Name: rdf_attrloc	- build local object attributes descriptors.
**
** Description:
**      This routine will build the attribute descriptors from iicolumns
**  catalog in ldb. It is caller's responsibility to allocate enough
**  space for the retrieved column descriptors.
**
**	Note if caller supply column conut in this call then this column
**  count will be compared to column count retrieve from ldb for consistency.
**  No column descriptor will be filled.
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to ldb desc
**			    .dd_i2_ldb_plus DD_0LDB_PLUS
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**			    .dd_t1_tab_name local table name
**			    .dd_t2_tab_owner
**					    local table owner 
**			    .dd_t7_col_cnt  zero for column descriptors
**					    or column count for consistency
**					    check only 
**
** Outputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**			    .dd_t2_tabowner user name, dba name, or $ingres
**			    .dd_t3_tabtype  table type
**			    .dd_t7_col_cnt  column count
**			    .dd_t8_cols_pp  ptr to column descriptor
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
[@history_template@]...
*/
static DB_STATUS
rdf_attrloc(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    DD_2LDB_TAB_INFO	*tab_info_p = global->rdfcb->rdf_rb.
        			      rdr_r2_ddb_req.rdr_d5_tab_info_p;
    DD_COLUMN_DESC	*col_ptr;
    i4		        att_cnt = 0;
    DD_NAME		col_name;
    i4			col_seq; 
    RQB_BIND    	rq_bind[RDD_02_COL],	/* for 2 columns */
			*bind_p = rq_bind;
    RDD_OBJ_ID		table_id,
			*id_p = &table_id;
    char		qrytxt[RDD_QRY_LENGTH];

    /* setup local table name and owner */

    rdd_rtrim((PTR)tab_info_p->dd_t1_tab_name,
	      (PTR)tab_info_p->dd_t2_tab_owner,
	       id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select column_name, column_sequence
    **	       from iicolumns where table_name = 'tab_name' and
    **	            table_owner = 'dd_tab_owner' order by column_sequence
    **	
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	STprintf(qrytxt, "%s '%s' and %s = '%s' order by column_sequence;",
	    "select column_name, column_sequence from iicolumns where table_name =",
	    id_p->tab_name,
 	    "table_owner",
            id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    rq_bind[0].rqb_addr = (PTR)col_name; 
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[1].rqb_addr = (PTR)&col_seq;
    rq_bind[1].rqb_length = sizeof(col_seq);
    rq_bind[1].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_02_COL;

    do
    {
	/* Fetch next tuple */
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    att_cnt += 1;

	    /* If caller supplies attribute count, check it here.
	    ** When error occurs, flush tuple buffer and return */
	    if (tab_info_p->dd_t7_col_cnt > 0
		&&
		col_seq > tab_info_p->dd_t7_col_cnt
	       )
	    {
		status = rdd_flush(global);
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0256_DD_COLMISMATCH);
		return(status);
	    }
	    /* Fill in column description.
	    ** Note that caller allocates space for the description */
	    col_ptr = (DD_COLUMN_DESC *)tab_info_p->dd_t8_cols_pp[col_seq];
	    MEcopy((PTR)col_name, sizeof(DD_NAME), (PTR)col_ptr->dd_c1_col_name);
	}
    } while (ddr_p->qer_d5_out_info.qed_o3_output_b);

    if (tab_info_p->dd_t7_col_cnt == 0)
        tab_info_p->dd_t7_col_cnt = att_cnt;
    else if (tab_info_p->dd_t7_col_cnt != att_cnt)
	 {/* This is the case when column number mismatch */
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0256_DD_COLMISMATCH);
         }
    return(status);
}

/*{
** Name: rdf_attcount	- get number of attributes.
**
** Description:
**      This routine retrieves number of attributes.
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to ldb desc
**			    .dd_i2_ldb_plus DD_0LDB_PLUS
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**			    .dd_t1_tab_name local table name
**			    .dd_t2_tab_owner
**					    local table owner 
**
** Outputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**			    .dd_t7_col_cnt  column count
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**      19-may-89 (mings)
**          work around count(*) in local dbms
**	16-jul-92 (teresa)
**	    prototypes
[@history_template@]...
*/
static DB_STATUS
rdf_attcount(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    DD_2LDB_TAB_INFO	*tab_info_p = global->rdfcb->rdf_rb.
        			      rdr_r2_ddb_req.rdr_d5_tab_info_p;
    RQB_BIND    	rq_bind[RDD_01_COL],	/* for 1 columns */
			*bind_p = rq_bind;
    i4			cnt;
    RDD_OBJ_ID		table_id,
			*id_p = &table_id;
    char		qrytxt[RDD_QRY_LENGTH];

    /* setup local table name and trim the trailing blanks */

    rdd_rtrim((PTR)tab_info_p->dd_t1_tab_name,
	      (PTR)tab_info_p->dd_t2_tab_owner,
	       id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select count(*)
    **	       from iicolumns where table_name = 'tab_name' and
    **	            table_owner = 'dd_tab_owner'
    **	
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	STprintf(qrytxt, "%s '%s' and %s = '%s';",
	    "select count(*) from iicolumns where table_name =",
	    id_p->tab_name,
 	    "table_owner",
            id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    rq_bind[0].rqb_addr = (PTR)&cnt; 
    rq_bind[0].rqb_length = sizeof(cnt);
    rq_bind[0].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

    /* Fetch next tuple */
    status = rdd_fetch(global);
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (ddr_p->qer_d5_out_info.qed_o3_output_b)
    {
	tab_info_p->dd_t7_col_cnt = cnt;
	status = rdd_flush(global);
    }
    else
	tab_info_p->dd_t7_col_cnt = 0;

    return(status);
}

/*{
** Name: rdf_localtab	- retrieve local table information.
**
** Description:
**	This routine calls QEF to retrieve local table information
**  from iitables catalog in ldb.
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to ldb desc
**			    .dd_i2_ldb_plus DD_0LDB_PLUS
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**			    .dd_t1_tabname  local table name
**
** Outputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**			    .dd_t2_tabowner user name, dba name, or $ingres
**			    .dd_t3_tabtype  table type
**			    .dd_t4_cre_date table create date
**			    .dd_t5_alt_date table alter date
**			    
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**      11-apr-89 (mings)
**         search ldb table with owner name
**      02-may-89 (mings)
**         use user_name and dba_name in iidbconatants for gateway support
**	08-aug-90 (teresa)
**	   added logic to allow user owned system catalogs to become owned by
**	   the system catalog ownere instead of the user.  (IE, user comes in
**	   with system catalog update priv.  If they create a table that starts
**	   with "ii", that table is owned by '$ingres' or the appropriate system
**	   catalog owner for the gateway.
**	16-jul-92 (teresa)
**	    prototypes
**	16-nov-92 (tersea)
**	    support registered procedures.
**	26-Feb-2004 (schka24)
**	    iitables reports a partition as P, which cvobject thinks is a
**	    registered procedure.  We want to pretend that partitions
**	    don't exist, so exclude them from the query results.
*/
DB_STATUS
rdf_localtab(	RDF_GLOBAL         *global,
		char		   *sys_owner)
{
    DB_STATUS		status = E_DB_OK;
    DD_2LDB_TAB_INFO	tab_info, /* temp area to store local table info */
			*tab_p = global->rdfcb->rdf_rb.rdr_r2_ddb_req.
				 rdr_d5_tab_info_p;
    DD_0LDB_PLUS        *ldb_ptr = &global->rdfcb->rdf_rb.rdr_r2_ddb_req.
				    rdr_d2_ldb_info_p->dd_i2_ldb_plus;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    bool		eof_found, 
			not_found,
			not_assigned,
			owner_assigned;
    i4			curr_user;
    RQB_BIND            rq_bind[RDD_05_COL], /* for 5 columns */
			*bind_p = rq_bind;
    char                tab_type[8];
    RDD_OBJ_ID		table_id,
			*id_p = &table_id;
    char		iuser[DB_MAXNAME + 1];
    char		qrytxt[RDD_QRY_LENGTH];

    /* setup system catalog owner name */

    iuser[DB_MAXNAME] = EOS;
    MEcopy((PTR)sys_owner, sizeof(DD_NAME), (PTR)iuser);
    (VOID)STtrmwhite(iuser);

    /* setup local table name and owner and trim the trailing blanks */

    owner_assigned = TRUE;

    rdd_rtrim((PTR)tab_p->dd_t1_tab_name,
	      (PTR)tab_p->dd_t2_tab_owner,
	       id_p);

    /* if PSF didn't pass owner name, then use dba name or system catalog
    ** owner name.  The only time we use system catalog owner name is when
    ** PSF tells us to by setting RDR_DD11_SYSOWNER.  PSF sets this flag when
    ** the user has system catalog update priv and the table name starts with
    ** "ii"
    */
    if (id_p->owner_length == 0)
    {
	owner_assigned = FALSE;
	if ( global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks &
	     RDR_DD11_SYSOWNER
	   )
	    /* use system catalog owner name */
	    MEcopy((PTR)sys_owner, sizeof(DD_NAME), (PTR)id_p->tab_owner);
	else
	    /* use dba name */
	    MEcopy((PTR)ldb_ptr->dd_p2_dba_name, sizeof(DD_NAME),
	       (PTR)id_p->tab_owner);
	id_p->tab_owner[DB_MAXNAME] = EOS;
	id_p->owner_length = STtrmwhite(id_p->tab_owner);
    }

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    if (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
    {
	/* this is a registered procedure, so query iiprocedures.  The owner
	** can either 1) be specified or 2) be iidbconstants.user_name or
	** 3) be regular search spacce (user, dba, $ingres).
	*/
	if (	
		owner_assigned ||
		(global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks &
		 RDR_DD11_SYSOWNER
		)
            )
	{
	    /* set up query for the specified table name and table owner 
	    **
	    **	select procedure_name, procedure_owner, create_date
	    **	       from iiprocedures where procedure_name = 'tab_name and 
	    **	       procedure_owner = 'table_owner;
	    */
	    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	    STprintf(qrytxt, "%s '%s' and procedure_owner = '%s';", 	    
		"select procedure_name, procedure_owner, create_date \
from iiprocedures where procedure_name = ",
		id_p->tab_name,
		id_p->tab_owner);
	}
	else if (global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks &
		 RDR_DD7_USERTAB)
	{
	    /* set up query 
	    **
	    **	select procedure_name, procedure_owner, create_date
	    **	       from iiprocedures where procedure_name = 'tab_name and 
	    **				   table_owner = username();
	    */
	    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
		STprintf(qrytxt, "%s '%s' and a.procedure_owner = b.user_name;",
		"select a.procedure_name, a.procedure_owner, a.create_date, \
from iiprocedures a, iidbconstants b where a.procedure_name =",
		id_p->tab_name);

	}
	else
	{
	    /* set up query for 
	    **
	    **	select a.procedure_name, a.procedure_owner, a.create_date
	    **      from iiprocedures a, iidbconstants b where
	    **	       a.procedure_name = 'tab_name' and (
	    **	       a.procedure_owner = b.user_name or 
	    **	       a.procedure_owner = b.dba_name or
	    **	       procedure_owner = '$ingres');		 
	    */

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	     STprintf(qrytxt, "%s '%s' and (a.procedure_owner = b.user_name or \
a.procedure_owner = '%s' or procedure_owner = '%s');", 	    
	    "select a.procedure_name, a.procedure_owner, a.create_date \
from iiprocedures a, iidbconstants b where a.procedure_name =",
	    id_p->tab_name,
	    id_p->tab_owner,
	    iuser);
	}
    }
    /* Not a registered Procedure, so query iitables */
    else if ( 
	owner_assigned ||
        (global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks&RDR_DD11_SYSOWNER)
       )
    {

	/* set up query for the specified table name and table owner 
	**
	**	select table_name, table_owner, create_date, alter_date, table_type
	**	       from iitables where table_name = tab_name and
	**	       table_owner = 'table_owner' and
	**	       table_type <> 'P';
	*/

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	     STprintf(qrytxt, "%s '%s' and table_owner = '%s' and table_type <> 'P';",
	    "select table_name, table_owner, create_date, alter_date, \
table_type from iitables where table_name =",
	    id_p->tab_name,
	    id_p->tab_owner);

    }
    else if (global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks &
	     RDR_DD7_USERTAB)
    {
	/* set up query 
	**
	**	select table_name, table_owner, create_date, alter_date, table_type
	**	       from iitables where table_name = tab_name and 
	**				   table_owner = username() and
	**				   table_type <> 'P';
	*/
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	    STprintf(qrytxt, "%s '%s' and a.table_owner = b.user_name and a.table_type <> 'P';",
	    "select a.table_name, a.table_owner, a.create_date, a.alter_date, \
a.table_type from iitables a, iidbconstants b where a.table_name =",
	    id_p->tab_name);
    }
    else
    {
	/* set up query for 
	**
	**	select a.table_name, a.table_owner, a.create_date, a.alter_date,
	**      a.table_type from iitables a, iidbconstants b where
	**	       a.table_name = 'tab_name' and (
	**	       a.table_owner = b.user_name or a.table_owner = b.dba_name or
	**	       table_owner = '$ingres') and
	**	       a.table_type <> 'P';
	*/

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	     STprintf(qrytxt, "%s '%s' and (a.table_owner = b.user_name or \
a.table_owner = '%s' or table_owner = '%s') \
and a.table_type <> 'P';",
	    "select a.table_name, a.table_owner, a.create_date, a.alter_date, \
a.table_type from iitables a, iidbconstants b where a.table_name =",
	    id_p->tab_name,
	    id_p->tab_owner,
	    iuser);
    }

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    not_found = TRUE;
    not_assigned = TRUE;
    eof_found = FALSE;

    /* table_name - dd_t1_tabname */
    rq_bind[0].rqb_addr = (PTR)tab_info.dd_t1_tab_name;
    rq_bind[0].rqb_length = sizeof(tab_info.dd_t1_tab_name);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    /* table_owner - dd_t2_tabowner */
    rq_bind[1].rqb_addr = (PTR)tab_info.dd_t2_tab_owner;
    rq_bind[1].rqb_length = sizeof(tab_info.dd_t2_tab_owner);
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
    /* create_date - dd_t4_cre_date */
    rq_bind[2].rqb_addr = (PTR)tab_info.dd_t4_cre_date;
    rq_bind[2].rqb_length = sizeof(DD_DATE);
    rq_bind[2].rqb_dt_id = DB_CHA_TYPE;
    if (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_03_COL;
    else
    {
	/* alter_date - dd_t5_alt_date */
	rq_bind[3].rqb_addr = (PTR)tab_info.dd_t5_alt_date;
	rq_bind[3].rqb_length = sizeof(DD_DATE);
	rq_bind[3].rqb_dt_id = DB_CHA_TYPE;
	/* table_type - dd_t3_tabtype */
	rq_bind[4].rqb_addr = (PTR)tab_type;
	rq_bind[4].rqb_length = sizeof(tab_type);
	rq_bind[4].rqb_dt_id = DB_CHA_TYPE;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_05_COL;
    }
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;

    do
    {
	/* Fetch next tuple */
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    /* First, look for owner equal to username. If failed, try matching dba name.
	    ** $ingres is used only when no username and dba name were found. */
	    if (MEcmp((PTR)tab_info.dd_t2_tab_owner, (PTR)sys_owner, sizeof(DD_NAME)) == 0)
		curr_user = RDD_INGRES;
	    else if (((ldb_ptr->dd_p1_character) & DD_1CHR_DBA_NAME)
		     &&
		     (MEcmp((PTR)tab_info.dd_t2_tab_owner, 
			    (PTR)ldb_ptr->dd_p2_dba_name, 
			    sizeof(DB_MAXNAME)) == 0)
		    )
		    curr_user = RDD_DBA;
		 else curr_user = RDD_USER;

	    if (curr_user == RDD_USER) 
	    {
		not_found = FALSE;
		not_assigned = FALSE;
		MEcopy((PTR)tab_info.dd_t2_tab_owner,
		       sizeof(DD_NAME),
		       (PTR)tab_p->dd_t2_tab_owner);

		MEcopy((PTR)tab_info.dd_t4_cre_date,
			sizeof(DD_DATE),
			(PTR)tab_p->dd_t4_cre_date);
		if (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
		{
		    /* if registered procedure, use create date as alter date */
		    tab_p->dd_t3_tab_type = DD_5OBJ_REG_PROC;
		    MEcopy((PTR)tab_info.dd_t4_cre_date,
			sizeof(DD_DATE),
			(PTR)tab_p->dd_t5_alt_date);
		}
		else
		{
		    (VOID)rdd_cvobjtype(tab_type, &tab_p->dd_t3_tab_type);
		    MEcopy((PTR)tab_info.dd_t5_alt_date,
			sizeof(DD_DATE),
			(PTR)tab_p->dd_t5_alt_date);
		}

            }
	    else if (not_assigned || curr_user == RDD_DBA)
	    {
		/* FIXME - this seems redundant.  When I get time, go though
		** logic and see why this is not reduced to just setting
		** not_found in the first case and doing away with this case
		** and making everything else common code */
		not_assigned = FALSE;
		MEcopy((PTR)tab_info.dd_t2_tab_owner,
		       sizeof(DD_NAME),
		       (PTR)tab_p->dd_t2_tab_owner);
		MEcopy((PTR)tab_info.dd_t4_cre_date,
			sizeof(DD_DATE),
			(PTR)tab_p->dd_t4_cre_date);
		if (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
		{
		    /* if registered procedure, use create date as alter date */
		    tab_p->dd_t3_tab_type = DD_5OBJ_REG_PROC;
		    MEcopy((PTR)tab_info.dd_t4_cre_date,
			sizeof(DD_DATE),
			(PTR)tab_p->dd_t5_alt_date);
		}
		else
		{
		    (VOID)rdd_cvobjtype(tab_type, &tab_p->dd_t3_tab_type);
		    MEcopy((PTR)tab_info.dd_t5_alt_date,
			sizeof(DD_DATE),
			(PTR)tab_p->dd_t5_alt_date);
		}
	    }
	}
	else
	    eof_found = TRUE;

    } while (not_found && !eof_found);

    if (!eof_found)
    {
	status = rdd_flush(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* if we do not find the table, then give PSF an error and a user name. */
    if (not_assigned)
    {
	SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0002_UNKNOWN_TBL);

	/*
	** this is at create table time, get the local user name and
	** give it to PSF
	*/
	if ( global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks &
	     RDR_DD7_USERTAB
	   )
		status = rdf_userlocal(global, tab_p->dd_t2_tab_owner);

	/* this is at create table time and the table being created is
	** forced to be a system catalog because the user has system
	** catalog update priv AND PSF has determined the table name forces
	** this table to be a system catalog (ie, starts with "ii")
	**
	** NOTE: RDR_DD7_USERTAB and RDR_DD11_SYSOWNER are mutually exclusive.
	**       They should NEVER be set at the same time.
	*/
	if ( global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks &
	     RDR_DD11_SYSOWNER
	   )
	    MEcopy((PTR)sys_owner, sizeof(DD_NAME),
		   (PTR) tab_p->dd_t2_tab_owner);

	status = E_DB_ERROR;

    }
    return(status);
}

/*{
** Name: rdf_sysowner	- retrieve ldb system catalog owner name.
**
** Description:
**	This routine retrieves ldb system catalog owner name.  This is
**	necessary because gateway may not support '$ingres' as owner
**      of system catalogs.  If this information is available in the 
**	DD_0LDB_PLUS, then use it.  Otherwise, query iitables.
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to ldb desc
**			    .dd_i2_ldb_plus DD_0LDB_PLUS
**				.dd_p1_character    DD_3CHR_SYS_NAME bit set if
**						    system owner name is set
**				.dd_p6_sys_name	    system owner name
**
** Outputs:
**	sys_owner			    ldb catalog owner
**		    
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-may-89 (mings)
**         initial creation
**	19-feb-91 (teresa)
**	    use dd_p6_sys_name from LDB INFO block if it is available.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdf_sysowner(	RDF_GLOBAL         *global,
		char		   *sys_table,
		char		   *sys_owner)
{
    DB_STATUS		status = E_DB_OK;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    RQB_BIND            rq_bind[RDD_01_COL], /* for 1 column */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];
    DD_0LDB_PLUS        *ldb_ptr = &global->rdfcb->rdf_rb.rdr_r2_ddb_req.
				    rdr_d2_ldb_info_p->dd_i2_ldb_plus;

    if ( ldb_ptr && (ldb_ptr->dd_p1_character & DD_3CHR_SYS_NAME) )
    {
	/* already have the system name in the LDB descriptor, so just use it */
	MEcopy((PTR)&ldb_ptr->dd_p6_sys_name, sizeof(DD_NAME), (PTR)sys_owner);
	return(status);
    }
    else
    {
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

	/* set up query to retrieve system catalog owner 
	**
	**	select table_owner from iitables where table_name = 'iitables';
	*/
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
		STprintf(qrytxt,
		"select table_owner from iitables where table_name = '%s';",
		sys_table);	    

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			     (i4)STlength((char *)qrytxt);
	ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

	status = rdd_query(global);	    /* send query to qef */
	if (DB_FAILURE_MACRO(status))
	    return (status); 

	/* table_owner */
	rq_bind[0].rqb_addr = (PTR)sys_owner;
	rq_bind[0].rqb_length = sizeof(DD_NAME);
	rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
	
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

	status = rdd_fetch(global);	    /* fetch one tuple */
	if (DB_FAILURE_MACRO(status))
	    return (status); 

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	    return(rdd_flush(global));

	/* this is a terrible case. iitables not found in ldb */

	rdu_ierror(global, E_DB_ERROR, E_RD027B_IITABLES_NOTFOUND);
	return(E_DB_ERROR);
    }
}

/*{
** Name: rdf_gldbinfo	- Retrieve LDB description.
**
** Description:
**      This function is called to retrieve local database information.
**
** Inputs:
**      global                          ptr to RDF global state variable.
**		.qef_rcb	        ptr to QEF request control block.
**		.rdfcb
**		    .rdf_rb             rdf request control block
**			.rdr_r2_ddb_req
**			    .rdr_d2_ldb_info_p
**					ptr to DD_1LDB_INFO
**				.dd_i1_ldb_desc
**					LDB descriptior
**		    
** Outputs:
**      global
**		.rdfcb
**		    .rdf_rb             rdf request control block
**			.rdr_r2_ddb_req
**			    .rdr_d2_ldb_info_p
**					ptr to DD_1LDB_INFO
**				.dd_i2_ldb_plus
**					LDB dba info and capabilities
**				    .dd_p1_dba_b
**				    .dd_p2_dba_name
**				    .dd_p3_ldb_caps
**				    .dd_p4_ldb_alias
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**      25-mar-89 (mings)
**          added E_QE0506_CANNOT_GET_ASSOCIATION checking to take care
**          CREATE TABLE/LINK WITH error message
**	20-jun-90 (teresa)
**	    add new parameter to rdu_ferror to fix bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
**	06-mar-96 (nanpr01)
**	    inquire buffer mgr capabilities after finding out ldbplus info. 
[@history_template@]...
*/
DB_STATUS
rdf_gldbinfo(RDF_GLOBAL *global)
{
    DB_STATUS           status;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;

    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    qefrcb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;   /* set time out to 0 */
    if (global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p)
    {
        qefrcb->qef_r3_ddb_req.qer_d2_ldb_info_p =               /* pointer to ldb desc */
	    global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p;

	status = qef_call(QED_LDBPLUS, ( PTR ) qefrcb);

	if (DB_FAILURE_MACRO(status))
	{
	    if (qefrcb->error.err_code == E_QE0506_CANNOT_GET_ASSOCIATION)
	    {
		SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0277_CANNOT_GET_ASSOCIATION);
	    }
	    else
	    {
		rdu_ferror(global, status, &qefrcb->error,
			    E_RD025E_QEF_LDBINFO,0);
	    }
	}
        else
        {
          DD_0LDB_PLUS        *ldbplus_p =
           &global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p->dd_i2_ldb_plus;
          if ((ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl > 605) &&
            (MEcmp(ldbplus_p->dd_p3_ldb_caps.dd_c7_dbms_type, "INGRES", 6) == 0))
          {
            status = rdd_buffcap(global,
                         global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p);
          }
          else
          {
            i4 j;
 
            ldbplus_p->dd_p9_pagesize[0] = DB_MIN_PAGESIZE;
            ldbplus_p->dd_p8_maxpage = DB_MIN_PAGESIZE;
            ldbplus_p->dd_p10_tupsize[0] = DB_MAXTUP;
            /* Initialize rest */
            for (j = 1; j < DB_MAX_CACHE; j++)
            {
              ldbplus_p->dd_p9_pagesize[j] = 0;
              ldbplus_p->dd_p10_tupsize[j] = 0;
            }
            ldbplus_p->dd_p7_maxtup = DB_MAXTUP;
          }
	}
    }
    else
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD025F_NO_LDBINFO);
    }
    return(status);
}

/*{
** Name: rdf_tabinfo - Request description of a table.
**
** Description:
**      This function retrieves information of a table or/and its columns  
**	from LDB catalogs.
**
**	Request additional attributes information:
**
**	rdr_types_mask = RDR_ATTRIBUTES
**
** Inputs:
**
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_instr		    Casing instructions:
**						RDF_TBL_CASE - use LDBs case
**								for table name
**						RDF_OWN_CASE - use LDBs case
**								for owner name
**						RDF_DELIM_OWN - No case xlate
**						  if LDB has mixed case delims.
**						RDF_DELIM_TBL - No case xlate
**						  if LDB has mixed case delims.
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**
** Outputs:
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      05-aug-88(mings)
**          created for titan
**	28-apr-92 (teresa)
**	    bug 42981: Uppercase Name and Owner based on specific instructions
**			from PSF.  Uppercase sys_table based on iidbcapabilities
**	01-mar-93 (barbara)
**	    Star support for delimited ids.  The case translation that takes
**	    place before LDB table lookup depends on PSF flags and case
**	    translation rules of LDB.
**	02-sep-93 (barbara)
**	    Case translation doesn't depend upon OPEN/SQL_LEVEL of the LDB:
**	    removed that check.  Owner name case transation depends on a new
**	    LDB capability, DB_REAL_USER_CASE.
**	05-nov-93 (barbara)
**	    If the user name is not delimited, the user name case should be
**	    translated to obey the case translation semantics of the LDB's 
**	    regular identifiers (bug revealed by delimited id testing).
**	    (bug 56678)
*/
DB_STATUS
rdf_tabinfo(RDF_GLOBAL *global)
{	
    DB_STATUS   	status;
    RDR_RB		*rdfrb = &global->rdfcb->rdf_rb;
    DD_1LDB_INFO	*ldbinfo_p = rdfrb->rdr_r2_ddb_req.rdr_d2_ldb_info_p;
    char		*name_p,*c;		
    i4			cnt;	
    DD_NAME		sys_owner;
    char		sys_table[DB_MAXNAME + 1];

#ifdef xDEBUG
    /* Check two input parameters for local table access 
    ** 1. local db info pointer
    ** 2. pointer to local table which contains local table name
    */
    if (!(global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d2_ldb_info_p)
	||
	!(global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d5_tab_info_p)
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
#endif

    /* Get ldb information */

    status = rdf_gldbinfo(global);  /* Get LDB information.*/
    if (DB_FAILURE_MACRO(status))
       return(status);

    /* translate names to the correct case, based on the capability in
    ** iidbcapabilities and based on specific instructions from PSF in
    ** rdr_instr:
    **  table name - translate if RDF_TBL_CASE & capability=UPPER_CASE
    **  owner name - translate if RDF_OWN_CASE & capability=UPPER_CASE
    **	sys_table  - translate if capability=UPPER_CASE
    ** (this fixes bug 42981.)
    **
    ** To support delimited identifiers, override the above case translation
    ** rules if RDF_DELIM_TBL/RDF_DELIM_OWN set and if LDB supports mixed
    ** case delimited identifiers/mixed case real user identifiers.
    */
    if (   (   ldbinfo_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c11_real_user_case
							!= DD_1CASE_MIXED
	    || ~global->rdfcb->rdf_rb.rdr_instr & RDF_DELIM_OWN
           )
	&& (global->rdfcb->rdf_rb.rdr_instr & RDF_OWN_CASE)
       )
    {
	for(cnt = 0, c=global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d5_tab_info_p->
				dd_t2_tab_owner; cnt < DB_MAXNAME; c++, cnt++)
	{
	    /* translate case of owner name */
	    if (ldbinfo_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c6_name_case ==
							DD_2CASE_UPPER)
	    {
		CMtoupper(c, c);
	    }
	    else
	    {
		CMtolower(c, c);
	    }
	}
    }

    if (   (   ldbinfo_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c10_delim_name_case
							!= DD_1CASE_MIXED
	    || ~global->rdfcb->rdf_rb.rdr_instr & RDF_DELIM_TBL
           )
	&& (global->rdfcb->rdf_rb.rdr_instr & RDF_TBL_CASE)
       )
    {
	for(cnt = 0, name_p=global->rdfcb->rdf_rb.rdr_r2_ddb_req.
		rdr_d5_tab_info_p->dd_t1_tab_name; cnt < DB_MAXNAME; 
		name_p++, cnt++)
	{
	    /* translate case of table name */
	    if (ldbinfo_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c6_name_case ==
							DD_2CASE_UPPER)
	    {
		CMtoupper(name_p, name_p); 
	    }
	    else
	    {
		CMtolower(name_p, name_p); 
	    }
	}
    }

    if (ldbinfo_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c6_name_case == 
	DD_2CASE_UPPER)
    {
	/* upper cased system catalog we use to obtain catalog owner name */
	STcopy(Iird_upper_systab, sys_table);	
    }
    else
    {
	/* lower cased system catalog we use to obtain catalog owner name */
	STcopy(Iird_lower_systab, sys_table);
    }

    /* check to see if RDR_RELATION was set */

    if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_RELATION)
    {
	/* retrieve ldb system catalog owner */

	status = rdf_sysowner(global, (char *)sys_table, (char *)sys_owner);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	status = rdf_localtab(global, (char *)sys_owner);

	if (DB_FAILURE_MACRO(status))
	    return(status);
	
	if (
	     !(rdfrb->rdr_types_mask & RDR_ATTRIBUTES) &&
	     !(global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
	   )
	{
	    /* if RDR_ATTRIBUTES was not set, then it indicates
	    ** that PSF need the number of attributes to allocate
	    ** enough memory for attribute information, unless this is a
	    ** procedure
	    */
	
	    status = rdf_attcount(global);
	    if (DB_FAILURE_MACRO(status))
	        return(status);
	}
    }

    /* If attribute information requested call rdf_attrloc */
    if (rdfrb->rdr_types_mask & RDR_ATTRIBUTES) 
        status = rdf_attrloc(global);
    return(status);
}

/*{
** Name: rdf_ddbinfo	- call QEF to get access status of DDB, the CDB information.
**
** Description:
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d3_ddb_desc_p  ptr to DD_DDB_DESC
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdf_ddbinfo(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB		*qefrcb;

    qefrcb = &global->rdf_qefrcb;		/* set up qefrcb */
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_modifier = 0;
    qefrcb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;      /* set time out to 0 */
    qefrcb->qef_r3_ddb_req.qer_d1_ddb_p = 
		    global->rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d3_ddb_desc_p;

    status = qef_call(QED_DDBINFO, ( PTR ) qefrcb);	/* call qef for ddb info */

    if (DB_FAILURE_MACRO(status))
	return(status);
    
    if (Rdi_svcb->rdv_cost_set == RDV_1_COST_SET)
	return(E_DB_OK);

    status = rdf_netnode_cost(global);

    if (DB_FAILURE_MACRO(status))
    {
	/* any thing goes wrong in retrieving net and cpu costs
	** will cause RDF to clear both net and cpu pointers and
	** treats the environment as no net and cpu costs. Since,
	** net and cpu costs share the same memory stream as
	** cluster info, the memory space will be released at
	** the same time cluster info is released. */
 
	Rdi_svcb->rdv_node_p = (DD_NODELIST *)NULL;
        Rdi_svcb->rdv_net_p = (DD_NETLIST *)Rdi_svcb->rdv_net_p;
    }
    /* no matter what return OK */

    return(E_DB_OK);			
}

/*{
** Name: rdf_ddb    - Retrieve iidbdb, ddb/cdb, and ldb information.
**
**	External call format status = rdf_call(RDF_DDB, &rdf_cb);
**
** Description:
**
** Inputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d1_ddb_masks   type of info requested
**					    RDR_DD1_DDBINFO
**					    RDR_DD2_TABINFO
**					    RDR_DD3_LDBPLUS
**					    RDR_DD4_USRSTAT
**					    RDR_DD5_IIDBDB
** Outputs:
**      global				    ptr to RDF global state variable
**	    .rdfcb			    ptr to RDF control block
**		.rdf_rb			    ptr to RDF request control block
**		    .rdr_r2_ddb_req	    ptr to ddb request block
**			.rdr_d2_ldb_info_p  ptr to DD_1LDB_INFO
**			.rdr_d3_ddb_desc_p  ptr to DD_DDB_DESC
**			.rdr_d4_usr_desc_p  ptr to DD_USR_DESC
**			.rdr_d5_tab_info_p  ptr to DD_2LDB_TAB_INFO
**	
**		    .rdf_error              Filled with error if encountered.
**
**			.err_code	    One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD025F_NO_LDBINFO	
**					ldb desc did not setup by caller.
**				E_RD0260_QEF_DDBINFO	
**					rerieve ddbinfo error.
**				E_RD0261_QEF_USRSTAT	
**					retrieve user stat error.
**				E_RD0262_QEF_IIDBDB		
**					retrieve iidbdb error.
**				E_RD0263_QEF_LDBPLUS
**					 retrieve ldbplus error.
**				
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	01-nov-89 (teg)
**	    check status of rdf_open_cdbasso and return error if this routine
**	    fails.
**	16-jul-92 (teresa)
**	    prototypes
**	6-Jul-2006 (kschendel)
**	    Don't call no-op ddbunfix.
*/
DB_STATUS
rdf_ddb(    RDF_GLOBAL	    *global,
	    RDF_CB	    *rdfcb)
{
    DB_STATUS           status;

    global->rdfcb = rdfcb;

#ifdef xDEBUG
    if (!(rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
        ||
        (
	    (
		(rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks)
		&
		(
		    RDR_DD1_DDBINFO
		    | RDR_DD2_TABINFO
		    | RDR_DD3_LDBPLUS
		    | RDR_DD4_USRSTAT
		    | RDR_DD5_IIDBDB
		    | RDR_DD8_USUPER
		    | RDR_DD9_CLUSTERINFO
		    | RDR_DD10_OPEN_CDBASSO
		)
	    ) == 0L 
        )
       )	 
    {
	rdu_ierror(global, E_DB_ERROR, E_RD0003_BAD_PARAMETER);
	return (E_DB_ERROR);
    } 
#endif

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD1_DDBINFO)
    {
	status = rdf_ddbinfo(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }    

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD2_TABINFO)
    {
	status = rdf_tabinfo(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }    

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD3_LDBPLUS)
    {
	status = rdf_ldbplus(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }    

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD4_USRSTAT)
    {
	status = rdf_usrstat(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }    

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD5_IIDBDB)
    {
	status = rdf_iidbdb(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }    

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD8_USUPER)
    {
	status = rdf_usuper(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD9_CLUSTERINFO)
    {
	status = rdf_clusterinfo(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    if (rdfcb->rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks & RDR_DD10_OPEN_CDBASSO)
    {
	status = rdf_open_cdbasso(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    return(E_DB_OK);
}
