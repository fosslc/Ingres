/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <gca.h>
#include    <ulc.h>
#include    <me.h>
#include    <st.h>
#include    <ex.h>
#include    <tr.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
#include    <opckey.h>
#include    <opcd.h>
#include    <opxlint.h>
#include    <opclint.h>
/**
**
**  Name: OPCDQCMP.C - Compile a query plan into query text.
**
**  Description:
**      Compile a query plan for distributed INGRES into a set of actions 
**	that consist of query text strings (i.e. QUEL and/or SQL statements).
**	If SQL, these statements will be legal 'Gateway SQL' that should work on
**	any SQL gateway, or INGRES back end.  
**
**
**
**  History:
**	23-jan-91 (stec)
**	    Modified opc_dentry().
**	24-jan-91 (stec)
**	    General cleanup of spacing, identation, lines shortened to 80 chars.
**	    Changed code to make certain that BT*() routines use initialized
**	    size of bitmaps, when applicable.
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
**      17-dec-92 (ed)
**          remove common code with local OPC, as part of merging
**          of local and distributed OPC
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	30-jun-93 (rickh)
**	    Added TR.H.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-feb-2003 (huazh01)
**          Do not mark QP as shareable/repeated for multi-site 
**          repeated query on Ingres Star server.
**          This fixes bug 109618. INGSTR 52.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*{
** Name: opc_dldbadd	- Add an ldb to the list for this query plan
**
** Description:
**      Allocates a QEQ_LDB_DESC, copies the DD_LDB_DESC into it, and adds
**	it to the end of the QEQ_LDB_DESC list that is anchored in the
**	query plan header.
**	DRHFIXME - do I need to check that the ldb is not already in the list? 
**
** Inputs:
**      global                          global state variable
**	ldbptr				DD_LDB_DESC to add to the list
**
** Outputs:
**	outldb				Ptr to added (copied) DD_LDB_DESC
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      25-sep-88 (robin)
**          Created
**	18-jan-93 (fpang)
**	    Copy DD_LDB_DESC field dd_l6_flags also.
*/

VOID
opc_dldbadd(
	OPS_STATE   *global,
	DD_LDB_DESC *ldbptr,
	DD_LDB_DESC **outldb )
{
    QEQ_LDB_DESC        *holdit;
    QEQ_LDB_DESC	*listptr;
    QEQ_LDB_DESC	*qldbptr;
    DD_LDB_DESC		*ldb_desc;
    bool		found;
    bool		match;

    if ( ldbptr == NULL || global == NULL )
	return;

    listptr = global->ops_cstate.opc_qp->qp_ddq_cb.qeq_d2_ldb_p;

    for (holdit = NULL, found = FALSE; listptr != NULL; 
	listptr = listptr->qeq_l2_nxt_ldb_p )
    {
	holdit = listptr;
	ldb_desc = &listptr->qeq_l1_ldb_desc;
	match = TRUE;
	if ( ldb_desc->dd_l1_ingres_b != ldbptr->dd_l1_ingres_b )
	    match = FALSE;
	if ( MEcmp( ldb_desc->dd_l2_node_name, ldbptr->dd_l2_node_name, 
		sizeof(DD_NODE_NAME)) != 0)
	    match = FALSE;
	if ( MEcmp( ldb_desc->dd_l3_ldb_name, ldbptr->dd_l3_ldb_name, 
		sizeof( DD_256C)) != 0)
	    match = FALSE;
	if ( MEcmp( ldb_desc->dd_l4_dbms_name, ldbptr->dd_l4_dbms_name, 
		DB_TYPE_MAXLEN ) != 0)
	    match = FALSE;
	if ( ldb_desc->dd_l5_ldb_id != ldbptr->dd_l5_ldb_id )
	    match = FALSE;

	if ( match )
	{
	    /*  the ldb is already on the list */
	    found = TRUE;
	    *outldb = &(listptr->qeq_l1_ldb_desc);
	    break;
	}
    }
    if (found != TRUE)
    {
	/*  Need to add the ldb to the descriptor list */
	
	qldbptr = (QEQ_LDB_DESC *) opu_qsfmem( global, sizeof( QEQ_LDB_DESC ));
        qldbptr->qeq_l2_nxt_ldb_p = NULL;
	qldbptr->qeq_l1_ldb_desc.dd_l1_ingres_b = ldbptr->dd_l1_ingres_b;
	MEcopy( ldbptr->dd_l2_node_name, sizeof(DD_NODE_NAME), 
	    qldbptr->qeq_l1_ldb_desc.dd_l2_node_name );
	MEcopy( ldbptr->dd_l3_ldb_name, sizeof(DD_256C), 
	    qldbptr->qeq_l1_ldb_desc.dd_l3_ldb_name );
	MEcopy( ldbptr->dd_l4_dbms_name, DB_TYPE_MAXLEN, 
	    qldbptr->qeq_l1_ldb_desc.dd_l4_dbms_name );
	qldbptr->qeq_l1_ldb_desc.dd_l5_ldb_id = ldbptr->dd_l5_ldb_id;	
	qldbptr->qeq_l1_ldb_desc.dd_l6_flags  = ldbptr->dd_l6_flags;	

	if ( holdit != NULL )
	    holdit->qeq_l2_nxt_ldb_p = qldbptr;
	else
	    global->ops_cstate.opc_qp->qp_ddq_cb.qeq_d2_ldb_p = qldbptr;
	*outldb = &(qldbptr->qeq_l1_ldb_desc);
    }
    
    return;
}

/*{
** Name: opc_ssite	- Compile single-site distributed query
**
** Description:
**      This routine is the 'fast path' for single site queries.  The parser
**	will have packaged the query text into a DD_PACKET, with all
**	name substitution for distributed completed.  This routine simply
**	wraps the packet into the appropriate data structures for QEF. 
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-sep-88 (robin)
**          Created.
**	01-dec-88 (robin)
**	    Modified for DD_PACKET structure change.
**	20-feb-89 (robin)
**	    Added support for QEA_D9_UPD (cursor update)
**	08-apr-89 (robin)
**	    Added the qeq_q3_ctl_info flag settings.
**	19-may-89 (robin)
**	    Changed test from OPD_TOUSER to OPD_NOFIXEDSITE.
**	    Partial fix for bug 5481.
**	31-aug-01 (inkdo01)
**	    Add firstn support to Star.
**      14-feb-08 (rapma01)
**          SIR 119650
**          Removed qeq_g1_firstn since first n is not currently
**          supported in star
**
*/

VOID
opc_ssite(
	OPS_STATE   *global )
{
    QEF_AHD		*actionhdr;
    QEQ_TXT_SEG		*segptr;
    QEQ_LDB_DESC	*ldbptr;
    QEQ_D1_QRY		*qptr;
    DD_PACKET		*pktptr;
    DD_PACKET		*lastnew;
    DD_PACKET		*newpkt;
    DB_LANG		qlang;


    if ( (global->ops_qheader->pst_distr == NULL ) ||
	 (global->ops_qheader->pst_distr->pst_qtext == NULL ) ||
	 (global->ops_qheader->pst_distr->pst_ldb_desc == NULL ) )
    {
	/* DRHFIXME error - parser should have supplied these */
	return;
    }

    /*
    **  Build a text segment to hold the DD_PACKET
    **  Unfortunately, the DD_PACKET(S) have to be copied.
    */

    segptr = (QEQ_TXT_SEG *) opu_qsfmem( global, sizeof( QEQ_TXT_SEG ) );
    segptr->qeq_s1_txt_b = TRUE;
    segptr->qeq_s3_nxt_p = NULL;
    
    for ( pktptr = global->ops_qheader->pst_distr->pst_qtext,
	    lastnew = NULL;
	    pktptr != NULL; pktptr = pktptr->dd_p3_nxt_p )
    {
	newpkt = (DD_PACKET *) opu_qsfmem( global,
	    (i4) (sizeof(DD_PACKET) + pktptr->dd_p1_len));
	newpkt->dd_p1_len = pktptr->dd_p1_len;
	newpkt->dd_p2_pkt_p = (char *) newpkt + sizeof( DD_PACKET);
	newpkt->dd_p4_slot = DD_NIL_SLOT;
	MEcopy( (PTR) pktptr->dd_p2_pkt_p, pktptr->dd_p1_len,
	    (PTR) newpkt->dd_p2_pkt_p );
	newpkt->dd_p3_nxt_p = (DD_PACKET *) NULL;
	if (lastnew != NULL)
	{
	    lastnew->dd_p3_nxt_p = newpkt;
	}    
	else
	{
	    /* it's first in the list */
	    segptr->qeq_s2_pkt_p = newpkt;
	}
	lastnew = newpkt;
    }

    /*
    **  Set the mode of the statement, and the
    **  query language
    */


    qlang = global->ops_qheader->pst_qtree->
	pst_sym.pst_value.pst_s_root.pst_qlang;

    /*
    **  Build a distributed query action
    */

    opc_dahd( global, &actionhdr );


    if ( global->ops_qheader->pst_mode == PSQ_DEFCURS )
    {
	actionhdr->ahd_atype = QEA_D7_OPN;
    }
    else if ( global->ops_qheader->pst_mode == PSQ_REPCURS )
    {
	actionhdr->ahd_atype = QEA_D9_UPD;
    }
    else
    {
	actionhdr->ahd_atype = QEA_D1_QRY;
    }

    /*
    **  Add the ldb descriptor to the list of ldb's in the query plan
    */

    opc_dldbadd( global, global->ops_qheader->pst_distr->pst_ldb_desc, 
	(DD_LDB_DESC **) &ldbptr );

    /*
    **  Store query info in the action
    */
    
    opc_qrybld( global, &qlang, 
	(global->ops_gdist.opd_gmask & OPD_NOFIXEDSITE) != 0, /* indicates that
					    ** transaction needed for 2pc */
	segptr, (DD_LDB_DESC *) ldbptr, &actionhdr->qhd_obj.qhd_d1_qry );
    qptr = &actionhdr->qhd_obj.qhd_d1_qry;

    /*
    **  Modify the query action definition for a cursor update
    */

    if (actionhdr->ahd_atype == QEA_D9_UPD)
    {
	qptr->qeq_q12_qid_first = FALSE;
	qptr->qeq_q13_csr_handle = global->ops_caller_cb->
	    opf_parent_qep.qso_handle; 
    }

    if ( global->ops_qheader->pst_mode == PSQ_RETRIEVE )
    {
	qptr->qeq_q3_ctl_info |= QEQ_001_READ_TUPLES;
    }

    if (  !(global->ops_gdist.opd_gmask & OPD_NOFIXEDSITE))
    {
	qptr->qeq_q3_ctl_info |= QEQ_002_USER_UPDATE;
    }

    /*
    ** If the action was a query (i.e. one that returns rows, then
    ** build a GET action to follow it for QEF
    */

    if ( (global->ops_qheader->pst_mode == PSQ_RETRIEVE)
	||
	 (global->ops_qheader->pst_mode == PSQ_RETINTO))
    {
	opc_dahd( global, &actionhdr );
	actionhdr->ahd_atype = QEA_D2_GET;
	actionhdr->qhd_obj.qhd_d2_get.qeq_g1_ldb_p = (DD_LDB_DESC *) ldbptr;
    }


    if (global->ops_cb->ops_check && 
	opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED))
    {
	/* print trace of all query text */

	opt_qtdump( global, actionhdr);
    }

    return;
}

/*{
** Name: opc_dentry	- Entry point for distributed query plan compilation
**
** Description:
**      Perform compilation of a distributed query plan.  Allocates
**	and initializes the query plan header, and the parameter block
**	for the text generation routines.  Calls opc_pltext to actually
**	convert the CO tree into a set of actions.  Finishes up the
**	the query plan header.
**	    [@description_or_none@]
**
** Inputs:
**      global
**	    ptr to global state variable
**
** Outputs:
**      global->ops_caller_cb->opf_qep
**	    Distributed QSF qep initialized
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-aug-88 (robin)
**          Created for distributed from opc_querycomp starting point, the 
**	    local version
**	22-feb-89 (robin)
**	    Modified to add parameter related data to QEQ_DDQ_CB
**	03-mar-89 (robin)
**	    Added qeq_d7_deltable for define cursor with delete
**	03-mar-89 (seputis)
**	    fixed problem in which simple aggregates and variables occur in the
**	    same query
**	26-jul-90 (seputis)
**	    modified opc_makename to fix quoted owner name problem for DB2
**	23-jan-91 (stec)
**	    Cleaned up code not to print partial output for op161 trace point
**	    for single site queries.
**	10-sep-92 (ed)
**	    added parameter to ulc_bld_descriptor, which was missed when ulc
**	    was changed
**	8-nov-92 (ed)
**	    move OP161 tracepoint code to opcentry.
**	01-mar-93 (barbara)
**	    For cursor open, save owner and tablename info separately.
**      12-feb-2003 (huazh01)
**          For multi-site repeated query on Ingres Star server, 
**          mark query plan as not shareable/repeatable unless 
**          43683 has been fixed. This fixes bug 109618. INGSTR 52.
*/
VOID
opc_dentry(
	OPS_STATE          *global )
{
    DB_STATUS	    ret;

    /*
    **  Compile the query plan into a query text.  Deal with the
    **  single site specially.
    */

    if (global->ops_statement != NULL)
    {
    	if  ( opc_tstsingle( global ) == (i4) TRUE ) 
    	{ 
    	    if (global->ops_cb->ops_check &&
    		opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED ))
    	    {
    		TRformat( opt_scc, (i4 *) global,
    		    (char *) &global->ops_trace.opt_trformat[0],
    		    (i4) sizeof(global->ops_trace.opt_trformat),
    		    "\n QUERY COMPILATION - SINGLE SITE PATH");
    	    }
    	    opc_ssite( global );
    	}
    	else
    	{
    	    if (global->ops_cb->ops_check &&
    		opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED ))
    	    {
    		TRformat( opt_scc, (i4 *) global,
    		    (char *) &global->ops_trace.opt_trformat[0],
    		    (i4) sizeof(global->ops_trace.opt_trformat),
    		    "\n QUERY COMPILATION - MULTI SITE PATH");
    	    }
 
            /* 
             ** bug 109618 
             ** mark query plan as not SHAREABLE/REPEATABLE, unless 
             ** SIR 43683 has been implemented.
            */
            global->ops_cstate.opc_qp->qp_status &= ~PST_SHAREABLE_QRY;
            global->ops_cstate.opc_qp->qp_status &= ~QEQP_RPT;

    	    opc_pltext(global); /* real multisite */
    	}
    	if (global->ops_cb->ops_check &&
	    opt_strace( global->ops_cb, OPT_F036_OPC_DISTRIBUTED ))
	{
	    opt_qpdump( global, global->ops_cstate.opc_qp );
	}
    }

    /* if it's time to stop compiling the query, then lets close stuff. */

    if (global->ops_statement == NULL || 
	    global->ops_procedure->pst_isdbp == FALSE
	)
    {
	/* We're finished compiling all of the statements, so lets finish
	** the QP
	*/
	QEF_QP_CB	    *qp;

	/* fill in the sqlda stuff for SCF */
	/* if (this is a retrieve or a define cursor) */
	qp = global->ops_cstate.opc_qp;
	/*
	**  If this is a define cursor which allows the DELETE operation,
	**  then store the tablename in the query plan for gcf.
	*/

	if (global->ops_qheader->pst_mode == PSQ_DEFCURS &&
	    global->ops_qheader->pst_delete == TRUE)
	{
		OPV_GRV	    *grv_ptr;
		OPV_IGVARS  gno;
		DD_TAB_NAME *tabname;
		DD_OWN_NAME *ownername;
		i2	    namelen;
		DD_CAPS	    *cap_ptr;

		gno = global->ops_qheader->pst_restab.pst_resvno;

		if (gno >= 0)
		{
		    grv_ptr = global->ops_rangetab.opv_base->opv_grv[gno];
		    tabname = (DD_TAB_NAME *) &grv_ptr->opv_relation->
			rdr_obj_desc->dd_o9_tab_info.dd_t1_tab_name[0];
		    ownername = (DD_OWN_NAME *) &grv_ptr->opv_relation->
			rdr_obj_desc->dd_o9_tab_info.dd_t2_tab_owner[0];
		    cap_ptr = &global->ops_gdist.opd_base->
			opd_dtable[OPD_TSITE]->opd_dbcap->dd_p3_ldb_caps;
		}
		else
		{
		    tabname = (DD_TAB_NAME *)NULL;
		    ownername = (DD_OWN_NAME *)NULL;
		}		

		/*
		** Allocate memory for owner name (qeq_d9_delown) and
		** for table name (qeq_d7_deltable).  Both may be
		** unnormalized and delimited by opc_makename so
		** allocate space beyond size of DD_NAME.
		*/

		qp->qp_ddq_cb.qeq_d9_delown = (DD_OWN_NAME *) opu_qsfmem( global,
		    ( sizeof(DD_OWN_NAME) + sizeof(DD_TAB_NAME) + 3) );
		opc_makename( ownername, (DD_TAB_NAME *) NULL,
		    (char *) qp->qp_ddq_cb.qeq_d9_delown, &namelen , cap_ptr,
		    (i4)global->ops_qheader->pst_mode, global);

		qp->qp_ddq_cb.qeq_d7_deltable = (DD_TAB_NAME *) opu_qsfmem( global,
		    ( sizeof(DD_OWN_NAME) + sizeof(DD_TAB_NAME) + 3) );
		opc_makename( (DD_OWN_NAME *) NULL, tabname,
		    (char *) qp->qp_ddq_cb.qeq_d7_deltable, &namelen , cap_ptr,
		    (i4)global->ops_qheader->pst_mode, global);

	}
	if ((global->ops_qheader->pst_mode == PSQ_RETRIEVE) /* this means 
				** that the query is read only */
	    &&
	    ((global->ops_qheader->pst_mask1 & PST_CDB_IIDD_TABLES) != 0)) /* 
				** this means that query contains only 
				** references to iidd_* style tables */
	    qp->qp_ddq_cb.qeq_d8_mask = QEQ_D8_INGRES_B;
	else
	    qp->qp_ddq_cb.qeq_d8_mask = 0;


	/*
	** If trace flag op173 is turned on, check all text segments
	** to see that they contain printable characters.  If any do not,
	** an error will be written to the log file.  This check was added
	** as a way to catch memory trashing by OPC or other facilities.
	*/

    	if (global->ops_cb->ops_check &&
	    opt_strace( global->ops_cb, OPT_F047_CHK_SEG ))
	{
	    opt_qtchk( global, global->ops_cstate.opc_qp );
	}
    }
}
