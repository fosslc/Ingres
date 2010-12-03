/*
**Copyright (c) 1995, 2005, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <bt.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cm.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <cui.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <ulm.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psykview.h>

/**
**
**  Name: PSYKVIEW.C - Functions used to destroy views.
**
**  Description:
**      This file contains the functions necessary to destroy views.
**
**          psy_kview   - Destroy a view.
**	    psy_basetbl - given id of a view, determine id of one of the base
**			  tables on which the view was defined
**	    psy_kregproc- destroy a QP for a registered procedure in star
**
**
**  History:    $Log-for RCS$
**      02-oct-85 (jeff)    
**          written
**	19-feb-88 (stec)
**	    Improve error handling.
**	13-oct-88 (stec)
**	    Handle following QEF return codes:
**		- E_QE0024_TRANSACTION_ABORTED
**		- E_QE002A_DEADLOCK
**		- E_QE0034_LOCK_QUOTA_EXCEEDED
**		- E_QE0099_DB_INCONSISTENT
**	16-dec-88 (stec)
**	    Correct octal constant problem.
**	19-jun-90 (andre)
**	    added psy_basetbl().
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: set DMU_TEMP_TABLE characteristic.
**	07-oct-92 (robf)
**	     Handle Gateway errors from QEF better - a failed REMOVE
**	     operation would silently fail to the user. Added error
**	     E_PS110F_GW_REM_ERR to display what really happened.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	30-dec-92 (tam)
**	    added function psy_kregproc to destroy QP when a regproc is
**	    removed in star.
**	31-may-93 (barbara)
**	    Delimit names of tables to be dropped if PSF tells us to delimit.
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	6-oct-93 (stephenb)
**	    set tablename in dmu_cb so that we can audit drop temp table.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	16-jun-94 (andre)
**	    Bug #64395
**	    it is dangerous to cast a (char *) into a (DB_CURSOR_ID *) and then
**	    dereference the resulting pointer because the chat ptr may not be 
**	    pointing at memory not aligned on an appopriate boundary.  Instead,
**	    we will allocate a DB_CURSOR_ID structure on the stack, initialize 
**	    it and MEcopy() it into the char array
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	19-Oct-2005 (schka24)
**	    Delete alter-timestamp routine, done inside QEF these days.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_kview(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb,
	QEU_CB *qeu_cb);
i4 psy_basetbl(
	PSS_SESBLK *sess_cb,
	DB_TAB_ID *view_id,
	DB_TAB_ID *tbl_id,
	DB_ERROR *err_blk);
i4 psy_kregproc(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb,
	DB_TAB_NAME *procname);

/*{
** Name: psy_kview	- Destroy views.
**
**  INTERNAL PSF call format: status = psy_kview(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_KVIEW, &psy_cb, &sess_cb);
**
** Description:
**      The psy_kview function destroys views by removing their definitions
**      from the tree, iiqrytext, relation, and attribute relations.
**
**	NOTE: This entry point is also used for destroying tables.  It is used
**	    this way because the "destroy" command in QUEL doesn't distinguish
**	    between tables and views.
**
** Inputs:
**      psy_cb
**          .psy_tables[]               Table ids of views to destroy
**	    .psy_tabname[]		Names of views to destroy
**	    .psy_numtabs		Number of tables to destroy
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**		.err_code		    What the error was
**		    E_PS0000_OK			Success
**		    E_PS0001_USER_ERROR		User made a mistake
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0003_INTERRUPTED	User interrupt
**		    E_PS0005_USER_MUST_ABORT	User must abort xact
**		    E_PS0008_RETRY		Query should be retried.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removes the definition of the view from the tree and iiqrytext
**	    relations.
**
** History:
**	02-oct-85 (jeff)
**          written
**	19-feb-88 (stec)
**	    Improve error handling - take care of DROP tbl, view_on_tbl case.
**	16-mar-88 (stec)
**	    Take out the above change.
**	08-apr-88 (stec)
**	    Handle QE0023_INVALID_QUERY_PLAN error.
**	13-oct-88 (stec)
**	    Handle following QEF return codes:
**		- E_QE0024_TRANSACTION_ABORTED
**		- E_QE002A_DEADLOCK
**		- E_QE0034_LOCK_QUOTA_EXCEEDED
**		- E_QE0099_DB_INCONSISTENT
**	16-dec-88 (stec)
**	    Correct octal constant problem (0038 -> 38).
**	22-jun-89 (andre)
**	    If the object being dropped is a view, call pst_rgdel() to mark
**	    possible entries for this object in pss_usrrange as unused.
**	14-jun-90 (andre)
**	    if dropping a view, we need to change a timestamp on one of the
**	    view's underlying tables to force invalidation of all QEPs for
**	    queries using this view.  A new QEF opcode, QEU_CTIMESTAMP has been
**	    defined to enable us to accomplish this.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancments: depending on the value of PSY_IS_TMPTBL
**	    in the psy_obj_mask field, set up a DMU_TEMP_TABLE characteristic
**	    appropriately for the DMU_DESTROY_TABLE operation.
**	15-jun-92 (barbara)
**	    Sybil merge. Changed interface to pst_rgdel to pass in sess_cb.
**	    Give QEF info on underlying LDB table when dropping Star tables.
**	    Merged Star history:
**	    11-nov-88 (andre)
**		give more info to QEF when dropping STAR objects with underlying
**		LDB table.
**	    13-dec-88 (andre)
**		Modify to send DESTROY rather than DROP when getting rid of an
**		LDB object.
**	    02-may-89 (andre)
**		Handle new error code -- E_QE0704_MULTI_SITE_WRITE
**	    07-may-91 (andre)
**		In STAR to destroy an LDB object using SQL, send
**		DROP TABLE|VIEW|INDEX rather than DROP since "DROP tbl_name"
**		is not in Open SQL, and the GWs get confused.  This should fix
**		bug 37414.
**	03-aug-92 (barbara)
**	    Invalidate RDF cached info of each object being dropped.
**	14-aug-92 (teresa)
**	    If the object being dropped is a secondary index, also call
**	    RDF to invalidate the base table's cache entry.
**	08-sep-92 (andre)
**	    if dropping a temporary table, set QEU_TMP_TBL over
**	    qeu_cb.qeu_flag
**	07-oct-92 (robf)
**	     Handle Gateway errors from QEF better - a failed REMOVE
**	     operation would silently fail to the user. Added error
**	     E_PS110F_GW_REM_ERR to display what really happened.
**	07-dec-92 (andre)
**	    address of QEU_DDL_INFO will be stored in QEU_CB.qeu_ddl_info
**	    instead of overloading qeu_qso.
**	30-dec-92 (tam)
**	    if removing a registered procedure in star, call psy_kregproc
**	    to destroy QP.
**	11-feb-93 (andre)
**	    if dropping IIDBDEPENDS or IIINTEGRITY as a part of UPGRADEDB, tell
**	    QEF to drop the table without regard for any objects that may depend
**	    on it
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-jul-93 (barbara)
**	    Must reset outgoing length before each call to cui_idunorm; also
**	    cleaned up error handling after call.
**	22-sep-93 (andre)
**	    PSF has gotten out of business of forcing invalidation of QPs which
**	    could be affected by destruction of views, revocation of privileges,
**	    etc.  Now the responsibility lies entirely with QEF.  Accordingly, 
**	    I have removed code which was responsible for collecting IDs of 
**	    views' underlying base tables and altering their timestamps
**	6-oct-93 (stephenb)
**	    set tablename in dmu_cb so that we can audit drop temp table.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	10-Sep-2004 (schka24)
**	    Upgradedb wants to drop/recreate iipriv, let it.
**	17-Nov-2005 (schka24)
**	    Don't invalidate RDF for temp table drops.  DMF will do it,
**	    so it's redundant.  Worse, the invalidate done here was being
**	    propagated to other DBMS servers!  beating up the event system.
**	17-nov-2008 (dougi)
**	    Extricate sequence names from default expression of identity 
**	    columns so identity sequences can be dropped, too.
**	14-Oct-2010 (kschendel) SIR 124544
**	    Make sure DMU_CB is zeroed, drop dmu-char-array.
*/
DB_STATUS
psy_kview(
	PSY_CB          *psy_cb,
	PSS_SESBLK	*sess_cb,
	QEU_CB		*qeu_cb)
{
    DB_STATUS		status;
    i4		err_code;
    register i4	i;
    DMU_CB		dmu_cb;
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    QED_DDL_INFO	ddl_info;
    QED_QUERY_INFO	qry_info;
    DD_2LDB_TAB_INFO	ldb_tab_info;
    DD_1LDB_INFO	ldb_info;
    DD_PACKET		packet;
    /*
    ** may hold "DESTROY tbl_name " or "DROP TABLE|INDEX|VIEW tbl_name " (note
    ** that the object name will be followed with a blank) if DROPping STAR
    ** object along with underlying LDB table
    */
    char		str[14 + sizeof(DD_TAB_NAME)*2];
    char		*tbl_name = str + sizeof("drop table ") -1;
    PSS_LTBL_INFO	*next_tbl = (PSS_LTBL_INFO *) psy_cb->psy_tblq.q_next;
    u_i4 		unorm_len, norm_len;
    PST_SEQOP_NODE	*seqp;

    MEfill(sizeof(DMU_CB), 0, &dmu_cb);
    dmu_cb.type = DMU_UTILITY_CB;
    dmu_cb.length = sizeof(dmu_cb);
    dmu_cb.owner = (PTR)DB_PSF_ID;
    dmu_cb.ascii_id = DMU_ASCII_ID;
    dmu_cb.dmu_db_id = sess_cb->pss_dbid;
    dmu_cb.dmu_table_name = psy_cb->psy_tabname[0];

    qeu_cb->qeu_d_cb = (PTR) &dmu_cb;
    qeu_cb->qeu_d_op = DMU_DESTROY_TABLE;


    /* Set up for RDF invalidation of cached informed of dropped object */

    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_2types_mask |= RDR2_KILLTREES;

    /* if there are any LDB tables to drop */
    if ((sess_cb->pss_distrib & DB_3_DDB_SESS) &&
	(next_tbl != (PSS_LTBL_INFO *) NULL))
    {
	MEcopy((PTR) sess_cb->pss_user.db_own_name, sizeof(DD_OWN_NAME),
	       (PTR) ddl_info.qed_d2_obj_owner);

	if ((qry_info.qed_q2_lang = sess_cb->pss_lang) == DB_SQL)
	{
	    MEcopy((PTR) "drop ", sizeof("drop ") - 1, (PTR) str);
	}
	else
	{
	    MEcopy((PTR) "destroy    ", sizeof("destroy    ") - 1,
		   (PTR) str);
	}

	packet.dd_p2_pkt_p = str;
	packet.dd_p3_nxt_p = (DD_PACKET *) NULL;

	qry_info.qed_q1_timeout = 0;
	qry_info.qed_q3_qmode = DD_2MODE_UPDATE;
	qry_info.qed_q4_pkt_p = &packet;
	ddl_info.qed_d5_qry_info_p = &qry_info;

	ddl_info.qed_d6_tab_info_p = &ldb_tab_info;
	qeu_cb->qeu_ddl_info = (PTR) &ddl_info;

	ldb_tab_info.dd_t9_ldb_p = &ldb_info;

	/* make sure the last char in the buffer is a space */
	CMcpychar(" ", (tbl_name + sizeof(DD_TAB_NAME)));
    }

    for (i = 0; i < psy_cb->psy_numtabs; i++)
    {
	/* Destroy each table, link, or view */
	if (psy_cb->psy_obj_mask[i] & PSY_IS_TMPTBL)
	{
	    qeu_cb->qeu_flag |= QEU_TMP_TBL;
	}
	else
	{
	    qeu_cb->qeu_flag &= ~((i4) QEU_TMP_TBL);
	}

	if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    /* Assign for QEF opcode (saved in the grammar) */
	    qeu_cb->qeu_d_op = (i4) psy_cb->psy_numbs[i];

	    if (qeu_cb->qeu_d_op == DMU_DESTROY_TABLE)
	    {
		char	*objtype;

		MEcopy((PTR) psy_cb->psy_tabname[i].db_tab_name,
		    sizeof(DD_TAB_NAME), (PTR) ddl_info.qed_d1_obj_name);
		STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[i],
					ddl_info.qed_d7_obj_id);

		/* in SQL we need to specify DROP TABLE|INDEX|VIEW */
		if (next_tbl->pss_obj_type == PSS_LTBL_IS_BASETABLE)
		{
		    objtype = "table ";
		}
		else if (next_tbl->pss_obj_type == PSS_LTBL_IS_VIEW)
		{
		    objtype = "view  ";
		}
		else
		{
		    objtype = "index ";
		}

		MEcopy((PTR) objtype, 6, (PTR) (str + sizeof("drop ") - 1));

		if (next_tbl->pss_delim_tbl)
		{
		    norm_len = (u_i4) psf_trmwhite(sizeof(DD_TAB_NAME),
					next_tbl->pss_tbl_name);
    		    unorm_len = DB_TAB_MAXNAME*2 +2;
		    status = cui_idunorm( (u_char*)next_tbl->pss_tbl_name,
					&norm_len, (u_char *)tbl_name,
					&unorm_len, CUI_ID_DLM, 
					&psy_cb->psy_error);

		    if (DB_FAILURE_MACRO(status))
		    {
		        (VOID) psf_error(psy_cb->psy_error.err_code, 0L,
				PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
		       		psf_trmwhite(sizeof(next_tbl->pss_tbl_name), 
			    	next_tbl->pss_tbl_name), next_tbl->pss_tbl_name);
		    	/* In case there are no more tables we need to
		    	** exit with proper status.
		        */
		        status = E_DB_OK;
		        continue;
		    }

		    packet.dd_p1_len   = unorm_len + 11 + CMbytecnt(" ");
	    	    CMcpychar(" ", (tbl_name + unorm_len));
		}
		else
		{
		    MEcopy((PTR) next_tbl->pss_tbl_name, sizeof(DD_TAB_NAME),
			(PTR) tbl_name);
	    	    packet.dd_p1_len = sizeof(DD_TAB_NAME) + 11 + CMbytecnt(" ");
	    	    CMcpychar(" ", (tbl_name + sizeof(DD_TAB_NAME)));
		}
		
		STRUCT_ASSIGN_MACRO(next_tbl->pss_ldb_desc,
		    ldb_info.dd_i1_ldb_desc);

		next_tbl = next_tbl->pss_next;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[i], dmu_cb.dmu_tbl_id);
	    }
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[i], dmu_cb.dmu_tbl_id);
	}

	/*
	** if dropping any table used in qeu_d_cascade while running
	** UPGRADEDB, tell QEF to drop the table without regard for
	** any objects that may depend on it
	*/
	qeu_cb->qeu_flag &= ~QEU_DROP_BASE_TBL_ONLY;
	if (sess_cb->pss_ses_flag & PSS_RUNNING_UPGRADEDB)
	{
	    i4 baseid = psy_cb->psy_tables[i].db_tab_base;
	    i4 indexid = psy_cb->psy_tables[i].db_tab_index;

	    if ((baseid == DM_B_DEPENDS_TAB_ID && indexid == DM_I_DEPENDS_TAB_ID)
	      || (baseid == DM_B_INTEGRITY_TAB_ID && indexid == DM_I_INTEGRITY_TAB_ID)
	      || (baseid == DM_B_PRIV_TAB_ID && indexid == DM_I_PRIV_TAB_ID)
	      || (baseid == DM_B_XPRIV_TAB_ID && indexid == DM_I_XPRIV_TAB_ID)
	      || (baseid == DM_B_TREE_TAB_ID && indexid == DM_I_TREE_TAB_ID)
	      || (baseid == DM_B_QRYTEXT_TAB_ID && indexid == DM_I_QRYTEXT_TAB_ID) )
		qeu_cb->qeu_flag |= QEU_DROP_BASE_TBL_ONLY;
	}
	
	status = qef_call(QEU_DBU, ( PTR ) qeu_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    switch (qeu_cb->error.err_code)
	    {
		/* some gateway problem */
		case E_QE0400_GATEWAY_ERROR:

		    err_code = E_PS110F_GW_REM_ERR;
		    (VOID) psf_error(err_code, 0L, PSF_USERERR, &err_code,
			&psy_cb->psy_error, 1,
		       psf_trmwhite(sizeof(psy_cb->psy_tabname[i]), 
			    (char *) &psy_cb->psy_tabname[i]),
			&psy_cb->psy_tabname[i]);
		    /* In case there are no more tables we need to
		    ** exit with proper status.
		    */
		    status = E_DB_OK;
		    continue;

		/* object unknown */
		case E_QE0031_NONEXISTENT_TABLE:

		/* could not drop because of single-site update requirement */
		case E_QE0704_MULTI_SITE_WRITE:
		{
		    if (qeu_cb->error.err_code == E_QE0704_MULTI_SITE_WRITE)
		    {
			err_code = E_PS0918_MULTI_SITE_DROP;
		    }
		    else
		    {
			err_code = E_PS0903_TAB_NOTFOUND;
		    }

		    (VOID) psf_error(err_code, 0L, PSF_USERERR, &err_code,
			&psy_cb->psy_error, 1,
		       psf_trmwhite(sizeof(psy_cb->psy_tabname[i]), 
			    (char *) &psy_cb->psy_tabname[i]),
			&psy_cb->psy_tabname[i]);

		    /* In case there are no more tables we need to
		    ** exit with proper status.
		    */
		    status = E_DB_OK;
		    continue;
		}
	    }
	    break;

	}

	if (psy_cb->psy_idseq_defp[i] && 
	    (seqp = &psy_cb->psy_idseq_defp[i]->pst_sym.pst_value.pst_s_seqop))
	{
	    /* Table has an identity column - prepare psy_sequence and
	    ** call psy_dsequence(). */
	    STRUCT_ASSIGN_MACRO(seqp->pst_owner, 
			psy_cb->psy_tuple.psy_sequence.dbs_owner);
	    STRUCT_ASSIGN_MACRO(seqp->pst_seqname,
			psy_cb->psy_tuple.psy_sequence.dbs_name);
	    STRUCT_ASSIGN_MACRO(seqp->pst_seqid,
			psy_cb->psy_tuple.psy_sequence.dbs_uniqueid);

	    status = psy_dsequence(psy_cb, sess_cb);
	}
	    
	/* if dropping a view, remove possible entries in QUEL range table */
	if (psy_cb->psy_obj_mask[i] & PSY_IS_VIEW)
	{
	    (VOID) pst_rgdel(sess_cb, psy_cb->psy_tabname + i,
			     &sess_cb->pss_usrrange, &psy_cb->psy_error);
	}

	/* Invalidate RDF unless it's a temp table.  (DMF has to handle
	** RDF invalidations for temp tables, because not all temp tables
	** are session temp tables.  Some QEF-created temps can get into
	** RDF, e.g. set-of parameter tables.)
	*/
	if ((psy_cb->psy_obj_mask[i] & PSY_IS_TMPTBL) == 0)
	{

	    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[i], rdf_rb->rdr_tabid);
	    if (status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb))
	    {
		(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
				&psy_cb->psy_error);
		return(status);
	    }
    
	    /* if a secondary index, invalidate the base table as well */
	    if (rdf_rb->rdr_tabid.db_tab_index > 0)
	    {   
		rdf_rb->rdr_tabid.db_tab_index = 0;
		if (status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb))
		{
		    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
				    &psy_cb->psy_error);
		    return(status);
		}
	    }
	}

	if (psy_cb->psy_flags & PSY_REMOVE_PROC)
	{
	    if (status = psy_kregproc(psy_cb, sess_cb, &psy_cb->psy_tabname[i]))
	    {
		return (status);
	    }
	}
    }

    if (DB_FAILURE_MACRO(status))
    {
	psf_qef_error(QEU_DBU, &qeu_cb->error, &psy_cb->psy_error);
    }
	
    return    (status);
}

/*
** psy_basetbl().  Given a view, determine table id of some underlying base
** table.
**
** Input:
**	    sess_cb		PSF session cb
**	    view_id		table id of the view
**
** Output:
**	    tbl_id		id of some underlying base table; will be set to
**				(0,0) if no underlying base table was found
**				because the view is a constant view
**				(e.g. create view V as select 5) or it is based
**				only on core catalogs (the SCONCUR catalogs)
**	    err_blk		will be filled in if some error occurs.
**
** Side effects:
**	pss_auxrng will be overwritten and cleared before returning to the
**	caller.
**
** History:
**	18-jun-90 (andre)
**	    written.
**	15-jun-92 (barbara)
**	    Changed interface to pst_sent and pst_clrrng.
**	03-aug-92 (barbara)
**	    Instead of clearing the view object from the RDF cache,
**	    tell RDF to get the view description afresh.
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	14-jul-93 (andre)
**	    if running UPGRADEDB, do not bother looking up the underlying base
**	    table; for one thing, the view tree may be of the format that RDF
**	    cannot digest
**	26-aug-93 (andre)
**	    view trees will now carry id of the view's underlying base table
**	    which greatly simplifies our task.  Instead of trying to, in
**	    essence, unravel the view tree, we will invoke psy_ubtid() which
**	    will fetch the view tree and obtain the underlying base table id,
**	    if one was found at the time the view was created.
*/
DB_STATUS
psy_basetbl(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*view_id,
	DB_TAB_ID	*tbl_id,
	DB_ERROR	*err_blk)
{
    PSS_RNGTAB	    *cur_var;
    i4	    mask;
    bool	    in_retry = ((sess_cb->pss_retry & PSS_REFRESH_CACHE) != 0);
    DB_STATUS	    status = E_DB_OK;

    /*
    ** if running UPGRADEDB, do not even try to look up underying base table -
    ** RDF may not know how to read the view tree node + we don't really care
    ** to update the underlying base table's timestamp
    */
    if (sess_cb->pss_ses_flag & PSS_RUNNING_UPGRADEDB)
    {
	tbl_id->db_tab_base = tbl_id->db_tab_index = (i4) 0;
	return(E_DB_OK);
    }
    
    /* initialize the range table */
    status = pst_rginit(&sess_cb->pss_auxrng);
    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    /*
    ** first we refresh the description of the alleged view from the cache in
    ** case it has been dropped, but is still in the cache
    */

    if (!in_retry)
        sess_cb->pss_retry |= PSS_REFRESH_CACHE;

    /* get description of the alleged view */
    status = pst_sent(sess_cb, &sess_cb->pss_auxrng, -1, "!", PST_SHWID,
	(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL,
	view_id, TRUE, &cur_var, (i4) 0, err_blk);

    if (!in_retry)
	sess_cb->pss_retry &= ~PSS_REFRESH_CACHE;

    if (DB_FAILURE_MACRO(status))
    {
	if (err_blk->err_code == E_PS0903_TAB_NOTFOUND)
	{
	    tbl_id->db_tab_base = tbl_id->db_tab_index = (i4) 0;
	    err_blk->err_code = E_PS0000_OK;
	    status = E_DB_OK;
	}

	/* no need for clean up at this point */

	return(status);
    }

    mask = cur_var->pss_tabdesc->tbl_status_mask;

    if (~mask & DMT_VIEW)
    {
	/*
	** object is not a view - search is over; we generally do not expect
	** this to happen.
	** NOTE: we will not be changing the timestamp for "core" catalogs
	*/
	if (mask & DMT_CATALOG && mask & DMT_CONCURRENCY)
	{
	    tbl_id->db_tab_base = tbl_id->db_tab_index = (i4) 0;
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(cur_var->pss_tabid, (*tbl_id));
	}
    }
    else
    {
	status = psy_ubtid(cur_var, sess_cb, err_blk, tbl_id);
    }

    {
	/* clear the range table */
	
	DB_STATUS  	local_status;

	local_status = pst_clrrng(sess_cb, &sess_cb->pss_auxrng, err_blk);
	if (local_status > status)
	{
	    status = local_status;
	}
    }
	
    return(status);
}

/*
** Name: psy_kregproc	- Destroy the QP for a registered procedure.
**
** Description:
**	This function destroys the QP object for a registered procedure
**	when the procedure is being removed.  The code is borrowed from
**	psy_kproc().
**
** Inputs:
**	psy_cb
**
**	sess_cb
**		pss_user
**		pss_udbid
**
**	procname	Ptr to name of the procedure
**
** Outputs:
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Destroy QP in QSF
**
** History:
**	30-dec-92 (tam)
**	    written
**	19-apr-93 (tam)
**	    Return OK if query plan is not in QSF.
**	16-jun-94 (andre)
**	    Bug #64395
**	    it is dangerous to cast a (char *) into a (DB_CURSOR_ID *) and then
**	    dereference the resulting pointer because the chat ptr may not be 
**	    pointing at memory not aligned on an appopriate boundary.  Instead,
**	    we will allocate a DB_CURSOR_ID structure on the stack, initialize 
**	    it and MEcopy() it into the char array
*/
DB_STATUS
psy_kregproc(
	PSY_CB		*psy_cb,
	PSS_SESBLK	*sess_cb,
	DB_TAB_NAME	*procname)
{
    QSF_RCB         qsf_rb;
    PSS_DBPALIAS    dbpid;
    DB_CURSOR_ID    dbp_curs_id;
    DB_STATUS	    status;
    i4	    err_code;

    /* Initialize the header of the QSF control block */
    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(qsf_rb);
    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid = sess_cb->pss_sessid;

    qsf_rb.qsf_feobj_id.qso_type = QSO_ALIAS_OBJ;
    qsf_rb.qsf_feobj_id.qso_lname = sizeof(dbpid);

    /* Identify the object first */
    dbp_curs_id.db_cursor_id[0] = dbp_curs_id.db_cursor_id[1] = 0;
    MEcopy((PTR)procname, DB_DBP_MAXNAME, (PTR)dbp_curs_id.db_cur_name);

    MEcopy((PTR) &dbp_curs_id, sizeof(DB_CURSOR_ID), (PTR) dbpid);

    MEcopy((PTR) &sess_cb->pss_user, DB_OWN_MAXNAME,
        (PTR) (dbpid + sizeof(DB_CURSOR_ID)));

    I4ASSIGN_MACRO(sess_cb->pss_udbid,
        *(i4 *) (dbpid + sizeof(DB_CURSOR_ID) + DB_OWN_MAXNAME));

    MEcopy((PTR) dbpid, sizeof(dbpid),
        (PTR) qsf_rb.qsf_feobj_id.qso_name);

    /* See if QEP for the alias already exists. */
    qsf_rb.qsf_lk_state = QSO_SHLOCK;
    status = qsf_call(QSO_JUST_TRANS, &qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
        if (qsf_rb.qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
        {
            (VOID) psf_error(E_PS037A_QSF_TRANS_ERR,
                    qsf_rb.qsf_error.err_code, PSF_INTERR,
                    &err_code, &psy_cb->psy_error, 0);
	    return (status);
        }
        else
        {
            /* Nothing to destroy, QP not in QSF */
	    return (E_DB_OK);
        }
    }

    /* Now destroy the ALIAS and the Q objects in QSF */
    status = qsf_call(QSO_DESTROY, &qsf_rb);


    if (DB_FAILURE_MACRO(status))
    {
        (VOID) psf_error(E_PS0A09_CANTDESTROY, qsf_rb.qsf_error.err_code,
                    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	return (status);
    }

    return (status);
}
