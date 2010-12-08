/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <usererror.h>
#include    <cs.h>
#include    <lk.h>
#include    <st.h>
#include    <me.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: qeudschm.c - Provide DROP SCHEMA support for RDF and PSF.
**
**  Description:
**      The routine defined in this file provide the catalog update
**	support for the DROP SCHEMA <schema name> CASCADE|RESTRICT statement.
**
**  Defines:
**      qeu_dschema	- Drop a schema
**
**  History:
** 	20-feb-93 (andre)
**	   Written 
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	29-jun-93 (rickh)
**	    Added ST.H and ME.H.
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	1-oct-93 (stephenb)
**	    DROP SCHEMA is an auditable event, add call to qeu_secaudit().
**	7-jan-94 (robf)
**          Update qeu_secaudit() calls  to new interface
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	11-Apr-2008 (kschendel)
**	    Revise DMF-qualification call sequence.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** static and forward declarations
*/
static DB_STATUS
qeu_ev_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams);

static DB_STATUS
qeu_syn_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams);

static DB_STATUS
qeu_dbp_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams);

static DB_STATUS
qeu_idx_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams);

/*{
** Name: qeu_dschema	- perform destruction of a schema
**
** External QEF call:   status = qef_call(QEU_DSCHEMA, &qeuq_cb);
**
** Description:
**	Perform destruction of a specified schema.
**
**	First, we must ascertain that a schema exists; if not - produce an error
**	message and return.
**	
**	DROP SCHEMA RESTRICT will result in destruction of a IISCHEMA row
**	describing specified schema providing that it [schema] does not contain
**	any tables, views, synonyms, dbevents, or dbprocs.
**
**	DROP SCHEMA CASCADE will result in destruction of a IISCHEMA row
**	describing specified schema along with all tables, views, synonyms,
**	dbevents, and database procedures contained therein.
**
**
** Inputs:
**      qef_cb				QEF session control block
**      qeuq_cb
**	    .qeuq_eflag			Designate error handling for user
**					errors.
**		QEF_INTERNAL		Return error code.
**		QEF_EXTERNAL		Send message to user.
**	    .qeuq_uld_tup		
**		dt_data			IISCHEMA tuple.
**		    db_schname		schema name
**		    db_schowner		schema owner
**	    .qeuq_flag_mask
**		QEU_DROP_CASCADE	set if cascading destruction of a schema
**					was specified
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_QE0255_NO_SCHEMA_TO_DROP
**				E_QE0256_NONEMPTY_SCHEMA
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	20-feb-93 (andre)
**	    Written
**	12-apr-93 (andre)
**	    error checking was performed incorrectly in that error code returned
**	    by qeu_drpdbp(), qeu_d_cascade(), qeu_devent(), and
**	    qeu_drop_synonym() would get overwritten
**	1-oct-93 (stephenb)
**	    DROP SCHEMA is an auditable event, add call to qeu_secaudit().
**	10-sep-93 (andre)
**	    set QEU_FORCE_QP_INVALIDATION in loc_qeuqcb.qeuq_flag_mask to 
**	    indicate to functions responsible for destroying objects belonging 
**	    to the schema that QPs which may depend on these objects need to be
**	    invalidated
**	08-oct-93 (andre)
**	    qeu_d_cascade() expects one more parameter - an address of a 
**	    DMT_TBL_ENTRY describing table/view/index being dropped; for all 
**	    other types of objects, NULL must be passed
**	2-Dec-2010 (kschendel) SIR 124685
**	    Solaris warning fix (prototypes).
*/
DB_STATUS
qeu_dschema(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    GLOBALREF QEF_S_CB	    *Qef_s_cb;
    DB_IISCHEMA		    *schema_to_drop = 
				(DB_IISCHEMA *) qeuq_cb->qeuq_uld_tup->dt_data;
    DB_IISCHEMA		    *sch_tuple;
    DB_PROCEDURE	    *dbp_tuple;
    DB_IIEVENT		    *ev_tuple;
    DB_IISYNONYM	    *syn_tuple;
    DB_IIREL_IDX	    *idx_tuple;
    struct drop_schema_tuples
    {
	union
	{
	    DB_IISCHEMA		schema;
	    DB_PROCEDURE	procedure;
	    DB_IIEVENT		event;
	    DB_IISYNONYM	synonym;
	    DB_IIREL_IDX	relidx;
	} tuple;
    };
    struct drop_schema_tuples	*tuples;
    DB_STATUS		    status, local_status;
    i4		    error = E_QE0000_OK;
    QEU_CB		    tranqeu;
    QEU_CB		    schemaqeu, *sch_qeu = (QEU_CB *) NULL;
    QEU_CB		    dbpqeu, *dbp_qeu = (QEU_CB *) NULL;
    QEU_CB		    evqeu, *ev_qeu = (QEU_CB *) NULL;
    QEU_CB		    synqeu, *syn_qeu = (QEU_CB *) NULL;
    QEU_CB		    idxqeu, *idx_qeu = (QEU_CB *) NULL;
    QEU_QUAL_PARAMS	    qparams;
    ULM_RCB		    ulm;
    QEF_DATA		    sch_qefdata;
    QEF_DATA		    dbp_qefdata;
    QEF_DATA		    ev_qefdata;
    QEF_DATA		    syn_qefdata;
    QEF_DATA		    idx_qefdata;
    /*
    ** only IISCHEMA will be accessed by key - the rest of cagtalogs will be
    ** scanned
    */
    DMR_ATTR_ENTRY	    sch_key_array[1];
    DMR_ATTR_ENTRY	    *schkey_ptr_array[1];
    DMT_SHW_CB		    dmt_shw_cb;
    DMT_TBL_ENTRY	    table;
    QEUQ_CB		    loc_qeuqcb;
	/*
	** obj_type ad obj_len ill be set only if a user has requested
	** restricted destruction of a non-empty schema
	*/
    char		    *obj_type = (char *) NULL;
    char		    *obj_name = (char *) NULL;
    
    bool		    drop_restrict;
    bool		    transtarted = FALSE;

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_streamid = (PTR)NULL;
    
    for (;;)	/* something to break out of */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}

	if (   qeuq_cb->qeuq_db_id == NULL
	    || qeuq_cb->qeuq_d_id == (CS_SID) 0
	    || qeuq_cb->qeuq_flag_mask & ~QEU_DROP_CASCADE
	    || qeuq_cb->qeuq_uld_tup == (QEF_DATA *) NULL
	    || qeuq_cb->qeuq_uld_tup->dt_size != sizeof(DB_IISCHEMA)
	    || qeuq_cb->qeuq_uld_tup->dt_data == NULL
	   )
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}

	schema_to_drop = (DB_IISCHEMA *) qeuq_cb->qeuq_uld_tup->dt_data;
	drop_restrict = (qeuq_cb->qeuq_flag_mask & QEU_DROP_CASCADE) == 0;
	status = E_DB_OK;
	
	/* 
	** Check to see if a transaction is in progress,
	** if so, set transtarted flag to FALSE, otherwise
	** set it to TRUE after beginning a transaction.
	** If we started a transaction them we will abort
        ** it if an error occurs, otherwise we will just
        ** return the error and let the caller abort it.
	*/

	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = qeuq_cb->qeuq_d_id;
	    tranqeu.qeu_flag = 0;
	    tranqeu.qeu_mask = 0;
	    status = qeu_btran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {	
		error = tranqeu.error.err_code;
		break;	
	    }	    
	    transtarted = TRUE;
	}
	/* escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	/*
	** first, determine whether the schema exists
	*/

	/* open the memory stream and allocate space for IISCHEMA tuple */
	for (;;)
	{

	    /*
	    ** allocate memory for the structure that UNIONs together IISCHEMA,
	    ** IIPROCEDURE, IIEVENT, IISYNONYM, and IIREL_IDX tuples
	    */
	    /* Open stream and allocate memory with one effort */
	    ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	    ulm.ulm_psize = ulm.ulm_blocksize = sizeof(struct drop_schema_tuples);

	    if ((status = qec_mopen(&ulm)) != E_DB_OK)
		break;

	    tuples = (struct drop_schema_tuples *) ulm.ulm_pptr;

	    sch_tuple = &tuples->tuple.schema;
	    dbp_tuple = &tuples->tuple.procedure;
	    ev_tuple  = &tuples->tuple.event;
	    syn_tuple = &tuples->tuple.synonym;
	    idx_tuple = &tuples->tuple.relidx;

	    break;
	}

	if (status != E_DB_OK)
	{
	    error = E_QE001E_NO_MEM;
	    break;
	}

	error = E_QE0000_OK;
    
	/*
	** open IISCHEMA; we will access it by schema name supplied by the
	** caller
	*/
	schemaqeu.qeu_type = QEUCB_CB;
        schemaqeu.qeu_length = sizeof(QEUCB_CB);
        schemaqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        schemaqeu.qeu_lk_mode = DMT_IX;
        schemaqeu.qeu_flag = DMT_U_DIRECT;
        schemaqeu.qeu_access_mode = DMT_A_WRITE;
	schemaqeu.qeu_mask = 0;
	schemaqeu.qeu_qual = 0;
	schemaqeu.qeu_qarg = 0;
	schemaqeu.qeu_f_qual = 0;
	schemaqeu.qeu_f_qarg = 0;

	schemaqeu.qeu_tab_id.db_tab_base  = DM_B_IISCHEMA_TAB_ID;
        schemaqeu.qeu_tab_id.db_tab_index = DM_I_IISCHEMA_TAB_ID;
	status = qeu_open(qef_cb, &schemaqeu);
	if (status != E_DB_OK)
	{
	    error = schemaqeu.error.err_code;
	    break;
	}
	sch_qeu = &schemaqeu;

	/*
	** determine whether the specified schema owned by the specified
	** user exists
	*/
	sch_qeu->qeu_tup_length = sizeof(DB_IISCHEMA);
	sch_qeu->qeu_output = &sch_qefdata;
	sch_qefdata.dt_next = 0;
	sch_qefdata.dt_size = sizeof(DB_IISCHEMA);
	sch_qefdata.dt_data = (PTR) sch_tuple;

	sch_qeu->qeu_key = schkey_ptr_array;
	schkey_ptr_array[0] = sch_key_array;
	schkey_ptr_array[0]->attr_number = DM_1_IISCHEMA_KEY;
	schkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	schkey_ptr_array[0]->attr_value = (char *) &schema_to_drop->db_schname;

	sch_qeu->qeu_count = 1;
	sch_qeu->qeu_getnext = QEU_REPO;
	sch_qeu->qeu_klen = 1;

	status = qeu_get(qef_cb, sch_qeu);
	if (status != E_DB_OK)
	{
	    /*
	    ** if no tuple could be found, tell the user that there was no
	    ** schema to drop
	    */
	    if (sch_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		error = E_QE0255_NO_SCHEMA_TO_DROP;
	    }
	    else
	    {
		error = sch_qeu->error.err_code;
	    }
	    break;
	}

	/*
	** if the name of schema owner contained in the tuple is different from
	** that specified by the caller, there is no sense in proceeding - tell
	** the caller that specified schema could not be found
	*/
	if (MEcmp((PTR) &schema_to_drop->db_schowner,
		  (PTR) &sch_tuple->db_schowner,
		  sizeof(schema_to_drop->db_schowner)))
	{
	    status = E_DB_ERROR;
	    error = E_QE0255_NO_SCHEMA_TO_DROP;
	    break;
	}

	/* now delete the IISCHEMA tuple */
	sch_qeu->qeu_getnext = QEU_NOREPO;
	sch_qeu->qeu_klen = 0;

	status = qeu_delete(qef_cb, sch_qeu);
	if (status != E_DB_OK)
	{
	    error = sch_qeu->error.err_code;
	    break;
	}

	/*
	** make a copy of QEUQ_CB structure passed in by the caller since we
	** plan to mess with it a bit (set QEU_DROPPING_SCHEMA and 
	** QEU_FORCE_QP_INVALIDATION, and, when dropping synonyms defined on 
	** objects in other schemas, reset qeuq_uld_tup to point at a IISYNONYM
	** tuple)
	*/
	STRUCT_ASSIGN_MACRO((*qeuq_cb), loc_qeuqcb);
	loc_qeuqcb.qeuq_flag_mask |= 
	    QEU_DROPPING_SCHEMA | QEU_FORCE_QP_INVALIDATION;
	
	/*
	** Open IIREL_IDX, IIPROCEDURE, IIEVENT, and IISYNONYM for reading
	** (since we are going to scan them, opening them for writing will
	** assure that we lock them exclusively which does not seem like a very
	** good idea, no?)
	*/
	STRUCT_ASSIGN_MACRO(schemaqeu, dbpqeu);
	dbpqeu.qeu_lk_mode = DMT_S;
	dbpqeu.qeu_access_mode = DMT_A_READ;
	dbpqeu.qeu_f_qual = NULL;
	dbpqeu.qeu_f_qarg = NULL;
	qparams.qeu_qparms[0] = (PTR) &schema_to_drop->db_schname;
	dbpqeu.qeu_qual = qeu_dbp_qual_func;
	dbpqeu.qeu_qarg = &qparams;

	dbpqeu.qeu_tup_length = sizeof(DB_PROCEDURE);
	dbpqeu.qeu_output = &dbp_qefdata;
	dbp_qefdata.dt_next = 0;
	dbp_qefdata.dt_size = sizeof(DB_PROCEDURE);
	dbp_qefdata.dt_data = (PTR) dbp_tuple;

	dbpqeu.qeu_key = (DMR_ATTR_ENTRY **) NULL;
	dbpqeu.qeu_count = 1;
	dbpqeu.qeu_getnext = QEU_REPO;
	dbpqeu.qeu_klen = 0;

	/* Note that these all get the same qeu-qual stuff */
	STRUCT_ASSIGN_MACRO(dbpqeu, evqeu);
	STRUCT_ASSIGN_MACRO(dbpqeu, synqeu);
	STRUCT_ASSIGN_MACRO(dbpqeu, idxqeu);
	
	dbpqeu.qeu_tab_id.db_tab_base  = DM_B_DBP_TAB_ID;
        dbpqeu.qeu_tab_id.db_tab_index = DM_I_DBP_TAB_ID;
	status = qeu_open(qef_cb, &dbpqeu);
	if (status != E_DB_OK)
	{
	    error = dbpqeu.error.err_code;
	    break;
	}
	dbp_qeu = &dbpqeu;

	evqeu.qeu_qual = qeu_ev_qual_func;

	evqeu.qeu_tup_length = sizeof(DB_IIEVENT);
	evqeu.qeu_output = &ev_qefdata;
	ev_qefdata.dt_next = 0;
	ev_qefdata.dt_size = sizeof(DB_IIEVENT);
	ev_qefdata.dt_data = (PTR) ev_tuple;

	evqeu.qeu_tab_id.db_tab_base  = DM_B_EVENT_TAB_ID;
	evqeu.qeu_tab_id.db_tab_index = DM_I_EVENT_TAB_ID;
	
	status = qeu_open(qef_cb, &evqeu);
	if (status != E_DB_OK)
	{
	    error = evqeu.error.err_code;
	    break;
	}
	ev_qeu = &evqeu;

	synqeu.qeu_qual = qeu_syn_qual_func;

	synqeu.qeu_tup_length = sizeof(DB_IISYNONYM);
	synqeu.qeu_output = &syn_qefdata;
	syn_qefdata.dt_next = 0;
	syn_qefdata.dt_size = sizeof(DB_IISYNONYM);
	syn_qefdata.dt_data = (PTR) syn_tuple;

	synqeu.qeu_tab_id.db_tab_base  = DM_B_SYNONYM_TAB_ID;
	synqeu.qeu_tab_id.db_tab_index = DM_I_SYNONYM_TAB_ID;
	
	status = qeu_open(qef_cb, &synqeu);
	if (status != E_DB_OK)
	{
	    error = synqeu.error.err_code;
	    break;
	}
	syn_qeu = &synqeu;

	idxqeu.qeu_qual = qeu_idx_qual_func;

	idxqeu.qeu_tup_length = sizeof(DB_IIREL_IDX);
	idxqeu.qeu_output = &idx_qefdata;
	idx_qefdata.dt_next = 0;
	idx_qefdata.dt_size = sizeof(DB_IIREL_IDX);
	idx_qefdata.dt_data = (PTR) idx_tuple;

	idxqeu.qeu_tab_id.db_tab_base  = DM_B_RIDX_TAB_ID;
	idxqeu.qeu_tab_id.db_tab_index = DM_I_RIDX_TAB_ID;
	
	status = qeu_open(qef_cb, &idxqeu);
	if (status != E_DB_OK)
	{
	    error = idxqeu.error.err_code;
	    break;
	}
	idx_qeu = &idxqeu;

	/*
	** First drop dbprocs
	*/
	status = qeu_get(qef_cb, dbp_qeu);
	if (status != E_DB_OK)
	{
	    if (dbp_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/* no dbprocs in the schema - so far so good */
		status = E_DB_OK;
	    }
	    else
	    {
		error = dbp_qeu->error.err_code;
		break;
	    }
	}
	else
	{
	    /*
	    ** found a dbproc in the schema; if restricted destruction was
	    ** ordered, report error
	    */
	    if (drop_restrict)
	    {
		status = E_DB_ERROR;
		error = E_QE0256_NONEMPTY_SCHEMA;
		obj_type = "database procedure";
		obj_name = (char *) &dbp_tuple->db_dbpname;
		break;
	    }

	    /* scan the rest of IIPROCEDURE; do not reposition */
	    dbp_qeu->qeu_getnext = QEU_NOREPO;
	    
	    loc_qeuqcb.qeuq_dbp_tup = &dbp_qefdata;
	    
	    do
	    {
		status = qeu_drpdbp(qef_cb, &loc_qeuqcb);
		if (status != E_DB_OK)
		{
		    error = loc_qeuqcb.error.err_code;
		    break;
		}

		status = qeu_get(qef_cb, dbp_qeu);
		if (status != E_DB_OK)
		{
		    if (dbp_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			/* ran out of tuples; reset status */
			status = E_DB_OK;
		    }
		    else
		    {
			error = dbp_qeu->error.err_code;
		    }

		    break;
		}

	    } while (1);

	    if (status != E_DB_OK)
		break;
	}

	/*
	** Try to drop all tables and view in this schema
	** (unless, of course, restricted destruction was requested in which
	** case we do not want to find any.  This will require that we scan
	** IIREL_IDX looking for objects in the current schema and then do
	** dmt_show() to get the object id (required by qeu_dview().)
	*/

	/*
	** Initialize DMT_SHW_CB which may be used a lot when dropping tables,
	** views, and indices
	*/

	dmt_shw_cb.type = DMT_SH_CB;
	dmt_shw_cb.length = sizeof(DMT_SHW_CB);
	dmt_shw_cb.dmt_char_array.data_in_size = 0;
	dmt_shw_cb.dmt_char_array.data_out_size  = 0;
	dmt_shw_cb.dmt_tab_id.db_tab_base =
	    dmt_shw_cb.dmt_tab_id.db_tab_index = 0;
	dmt_shw_cb.dmt_flags_mask = DMT_M_TABLE | DMT_M_NAME;
	dmt_shw_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
	dmt_shw_cb.dmt_session_id = qeuq_cb->qeuq_d_id;
	dmt_shw_cb.dmt_table.data_address = (PTR) &table;
	dmt_shw_cb.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	dmt_shw_cb.dmt_char_array.data_address = (PTR)NULL;
	dmt_shw_cb.dmt_char_array.data_in_size = 0;
	dmt_shw_cb.dmt_char_array.data_out_size  = 0;

	status = qeu_get(qef_cb, idx_qeu);
	if (status != E_DB_OK)
	{
	    if (idx_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/* no tables in the schema - so far so good */
		status = E_DB_OK;
	    }
	    else
	    {
		error = idx_qeu->error.err_code;
		break;
	    }
	}
	else
	{
	    /*
	    ** found a table, view, or index in the schema; if restricted
	    ** destruction was ordered, report error
	    */
	    if (drop_restrict)
	    {
		status = E_DB_ERROR;
		error = E_QE0256_NONEMPTY_SCHEMA;
		obj_type = "table";
		obj_name = (char *) &idx_tuple->relname;
		break;
	    }

	    /* scan the remainder of IIREL_IDX, do not reposition */
	    idx_qeu->qeu_getnext = QEU_NOREPO;
	    
	    /*
	    ** store schema name into DMT_SHW_CB outside of the loop since it
	    ** will always be the same
	    */
	    STRUCT_ASSIGN_MACRO(idx_tuple->relowner, dmt_shw_cb.dmt_owner);

	    do
	    {
		DB_QMODE	    obj_type;

		/*
		** call DMT_SHOW to get the object id, then call qeu_dview() to
		** destroy the object
		*/
		STRUCT_ASSIGN_MACRO(idx_tuple->relname, dmt_shw_cb.dmt_name);

		status = dmf_call(DMT_SHOW, &dmt_shw_cb);
		if (status != E_DB_OK)
		{
		    if (dmt_shw_cb.error.err_code != E_DM0114_FILE_NOT_FOUND)
		    {
			error = dmt_shw_cb.error.err_code;
			break;
		    }
		    else
		    {
			status = E_DB_OK;
		    }
		}

		loc_qeuqcb.qeuq_rtbl = &table.tbl_id;
		STRUCT_ASSIGN_MACRO(table.tbl_qryid, loc_qeuqcb.qeuq_qid);

		if (table.tbl_status_mask & DMT_VIEW)
		    obj_type = DB_VIEW;
		else if (table.tbl_status_mask & DMT_IDX)
		    obj_type = DB_INDEX;
		else
		    obj_type = DB_TABLE;

		status = qeu_d_cascade(qef_cb, &loc_qeuqcb, obj_type, &table);
		if (status != E_DB_OK)
		{
		    error = loc_qeuqcb.error.err_code;
		    break;
		}

		status = qeu_get(qef_cb, idx_qeu);
		if (status != E_DB_OK)
		{
		    if (idx_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			/* ran out of tuples; reset status */
			status = E_DB_OK;
		    }
		    else
		    {
			error = idx_qeu->error.err_code;
		    }

		    break;
		}

	    } while (1);

	    if (status != E_DB_OK)
		break;
	}

	/*
	** Now drop dbevents
	*/
	status = qeu_get(qef_cb, ev_qeu);
	if (status != E_DB_OK)
	{
	    if (ev_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/* no dbevents in the schema - so far so good */
		status = E_DB_OK;
	    }
	    else
	    {
		error = ev_qeu->error.err_code;
		break;
	    }
	}
	else
	{
	    /*
	    ** found a dbevent in the schema; if restricted destruction was
	    ** ordered, report error
	    */
	    if (drop_restrict)
	    {
		status = E_DB_ERROR;
		error = E_QE0256_NONEMPTY_SCHEMA;
		obj_type = "dbevent";
		obj_name = (char *) &ev_tuple->dbe_name;
		break;
	    }

	    /* scan the rest of IIEVENT; do not reposition */
	    ev_qeu->qeu_getnext = QEU_NOREPO;
	    
	    loc_qeuqcb.qeuq_culd = 1;
	    loc_qeuqcb.qeuq_uld_tup = &ev_qefdata;
	    do
	    {
		status = qeu_devent(qef_cb, &loc_qeuqcb);
		if (status != E_DB_OK)
		{
		    error = loc_qeuqcb.error.err_code;
		    break;
		}

		status = qeu_get(qef_cb, ev_qeu);
		if (status != E_DB_OK)
		{
		    if (ev_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			/* ran out of tuples; reset status */
			status = E_DB_OK;
		    }
		    else
		    {
			error = ev_qeu->error.err_code;
		    }

		    break;
		}

	    } while (1);

	    if (status != E_DB_OK)
		break;
	}

	/*
	** Lastly, drop synonyms
	*/
	status = qeu_get(qef_cb, syn_qeu);
	if (status != E_DB_OK)
	{
	    if (syn_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/* no synonyms in the schema - so far so good */
		status = E_DB_OK;
	    }
	    else
	    {
		error = syn_qeu->error.err_code;
		break;
	    }
	}
	else
	{
	    /*
	    ** found a synonym in the schema; if restricted destruction was
	    ** ordered, report error
	    */
	    if (drop_restrict)
	    {
		status = E_DB_ERROR;
		error = E_QE0256_NONEMPTY_SCHEMA;
		obj_type = "synonym";
		obj_name = (char *) &syn_tuple->db_synname;
		break;
	    }

	    /* scan the rest of IISYNONYM; do not reposition */
	    syn_qeu->qeu_getnext = QEU_NOREPO;
	    
	    loc_qeuqcb.qeuq_culd = 1;
	    loc_qeuqcb.qeuq_uld_tup = &syn_qefdata;
	    do
	    {
		status = qeu_drop_synonym(qef_cb, &loc_qeuqcb);
		if (status != E_DB_OK)
		{
		    error = loc_qeuqcb.error.err_code;
		    break;
		}

		status = qeu_get(qef_cb, syn_qeu);
		if (status != E_DB_OK)
		{
		    if (syn_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			/* ran out of tuples; reset status */
			status = E_DB_OK;
		    }
		    else
		    {
			error = syn_qeu->error.err_code;
		    }
		    
		    break;
		}

	    } while (1);

	    if (status != E_DB_OK)
		break;
	}
	/*
	** audit successful drop of schema
	*/
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR	e_error;

	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		schema_to_drop->db_schname.db_schema_name,
		&schema_to_drop->db_schowner, 
		sizeof(schema_to_drop->db_schname.db_schema_name),
		SXF_E_SECURITY, I_SX203F_SCHEMA_DROP,
		SXF_A_SUCCESS | SXF_A_DROP, &e_error);
	    if (status != E_DB_OK)
	    {
		error = e_error.err_code;
		break;
	    }
	}

	break;
    } /* End dummy for */

    if (status != E_DB_OK)
    {
	/*
	** call qef_error to handle error messages
	** E_QE0255_NO_SCHEMA_TO_DROP and E_QE0256_NONEMPTY_SCHEMA require that
	** parameters be passed
	*/
	if (error == E_QE0255_NO_SCHEMA_TO_DROP)
	{
	    (VOID) qef_error(E_QE0255_NO_SCHEMA_TO_DROP, 0L, status, &error,
		&qeuq_cb->error, 2,
		qec_trimwhite(sizeof(schema_to_drop->db_schname),
		    (char *) &schema_to_drop->db_schname),
		(PTR) &schema_to_drop->db_schname,
		qec_trimwhite(sizeof(schema_to_drop->db_schowner),
		    (char *) &schema_to_drop->db_schowner),
		(PTR) &schema_to_drop->db_schowner);
	}
	else if (error == E_QE0256_NONEMPTY_SCHEMA)
	{
	    (VOID) qef_error(E_QE0256_NONEMPTY_SCHEMA, 0L, status, &error,
		&qeuq_cb->error, 4,
		qec_trimwhite(sizeof(schema_to_drop->db_schname),
		    (char *) &schema_to_drop->db_schname),
		(PTR) &schema_to_drop->db_schname,
		qec_trimwhite(sizeof(schema_to_drop->db_schowner),
		    (char *) &schema_to_drop->db_schowner),
		(PTR) &schema_to_drop->db_schowner,
		STlength(obj_type), (PTR) obj_type,
		qec_trimwhite(DB_MAXNAME, obj_name), (PTR) obj_name);
	}
	else
	{
	    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
	}
    }
    else
    {
	/* reset err_code to OK in case it was set to E_QE0015_NO_MORE_ROWS */
	qeuq_cb->error.err_code = E_QE0000_OK;
    }

    local_status = E_DB_OK;

    /* close off all the tables that have been opened */
    if (idx_qeu)
    {
	local_status = qeu_close(qef_cb, idx_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(idx_qeu->error.err_code, 0L, local_status, &error,
		&qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (sch_qeu)
    {
	local_status = qeu_close(qef_cb, sch_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(sch_qeu->error.err_code, 0L, local_status, &error,
		&qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (dbp_qeu)
    {
	local_status = qeu_close(qef_cb, dbp_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(dbp_qeu->error.err_code, 0L, local_status, &error,
		&qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (ev_qeu)
    {
	local_status = qeu_close(qef_cb, ev_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(ev_qeu->error.err_code, 0L, local_status, &error,
		&qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (syn_qeu)
    {
	local_status = qeu_close(qef_cb, syn_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(syn_qeu->error.err_code, 0L, local_status, &error,
		&qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (ulm.ulm_streamid != (PTR)NULL)
    {
	/* Close the input stream */

	(VOID) ulm_closestream(&ulm);
    }

    if (transtarted)
    {
        if (status == E_DB_OK)
        {	
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {	
		(VOID) qef_error(tranqeu.error.err_code, 0L, status, &error,
		     &qeuq_cb->error, 0);
	    }
	    else
	    {
	        qeuq_cb->error.err_code = E_QE0000_OK;
	    }
	}
	
	if (status != E_DB_OK)
        {
	    local_status = qeu_atran(qef_cb, &tranqeu);
	    if (local_status != E_DB_OK)
	    {
	        (VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			    &error, &qeuq_cb->error, 0);
	        if (local_status > status)
		    status = local_status;
	    }
	}
    }

    return (status);
} /* qeu_dschema */

/*
** Name: qeu_idx_qual_func - qualification function for retrieving IIREL_IDX
**			     tuples
**
** Description:
**	This function will be used to qualify IIREL_IDX tuples retrieved by DMF.
**	The only criterion here is that the schema name (referred to as owner
**	name prior to 6.5) matches that of the schema being dropped
**
** Input:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	Schema name being dropped
**	    .qeu_rowaddr	current IIREL_IDX tuple
**
** Output
**	qparams.qeu_retval	Set to ADE_TRUE if match, else ADE_NOT_TRUE
**
** Returns:
**	E_DB_OK
*/
static DB_STATUS
qeu_idx_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams)
{
    DB_SCHEMA_NAME *schema_name = (DB_SCHEMA_NAME *) qparams->qeu_qparms[0];
    DB_IIREL_IDX *relidx_tuple = (DB_IIREL_IDX *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;
    if ( MEcmp((PTR) &relidx_tuple->relowner, (PTR) schema_name,
	    DB_SCHEMA_MAXNAME) == 0)
	qparams->qeu_retval = ADE_TRUE;
    return (E_DB_OK);
}

/*
** Name: qeu_dbp_qual_func - qualification function for retrieving IIPROCEDURE
**			     tuples
**
** Description:
**	This function will be used to qualify IIPROCEDURE tuples retrieved by
**	DMF. The only criterion here is that the schema name (referred to as
**	owner name prior to 6.5) matches that of the schema being dropped
**
** Input:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	Schema name being dropped
**	    .qeu_rowaddr	current IIPROCEDURE tuple
**
** Output
**	qparams.qeu_retval	Set to ADE_TRUE if match, else ADE_NOT_TRUE
**
** Returns:
**	E_DB_OK always
*/
static DB_STATUS
qeu_dbp_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams)
{
    DB_SCHEMA_NAME *schema_name = (DB_SCHEMA_NAME *) qparams->qeu_qparms[0];
    DB_PROCEDURE *dbp_tuple = (DB_PROCEDURE *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;
    if ( MEcmp((PTR) &dbp_tuple->db_owner, (PTR) schema_name,
	    DB_SCHEMA_MAXNAME) == 0)
	qparams->qeu_retval = ADE_TRUE;
    return (E_DB_OK);
}

/*
** Name: qeu_syn_qual_func - qualification function for retrieving IISYNONYM
**			     tuples
**
** Description:
**	This function will be used to qualify IISYNONYM tuples retrieved by
**	DMF. The only criterion here is that the schema name (referred to as
**	owner name prior to 6.5) matches that of the schema being dropped
**
** Input:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	Schema name being dropped
**	    .qeu_rowaddr	current IISYNONYM tuple
**
** Output
**	qparams.qeu_retval	Set to ADE_TRUE if match, else ADE_NOT_TRUE
**
** Returns:
**	E_DB_OK always
*/
static DB_STATUS
qeu_syn_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams)
{
    DB_SCHEMA_NAME *schema_name = (DB_SCHEMA_NAME *) qparams->qeu_qparms[0];
    DB_IISYNONYM *syn_tuple = (DB_IISYNONYM *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;
    if ( MEcmp((PTR) &syn_tuple->db_synowner, (PTR) schema_name,
	    DB_SCHEMA_MAXNAME) == 0)
	qparams->qeu_retval = ADE_TRUE;
    return (E_DB_OK);
}

/*
** Name: qeu_ev_qual_func - qualification function for retrieving IIEVENT
**			    tuples
**
** Description:
**	This function will be used to qualify IIEVENT tuples retrieved by
**	DMF. The only criterion here is that the schema name (referred to as
**	owner name prior to 6.5) matches that of the schema being dropped
**
** Input:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	Schema name being dropped
**	    .qeu_rowaddr	current IISYNONYM tuple
**
** Output
**	qparams.qeu_retval	Set to ADE_TRUE if match, else ADE_NOT_TRUE
**
** Returns:
**	E_DB_OK always
*/
static DB_STATUS
qeu_ev_qual_func(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams)
{
    DB_SCHEMA_NAME *schema_name = (DB_SCHEMA_NAME *) qparams->qeu_qparms[0];
    DB_IIEVENT *ev_tuple = (DB_IIEVENT *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;
    if ( MEcmp((PTR) &ev_tuple->dbe_owner, (PTR) schema_name,
	    DB_SCHEMA_MAXNAME) == 0)
	qparams->qeu_retval = ADE_TRUE;
    return (E_DB_OK);
}
