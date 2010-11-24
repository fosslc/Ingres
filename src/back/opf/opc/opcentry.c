/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
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
#include    <me.h>
#include    <st.h>
#include    <ex.h>
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
#define             OPC_QUERYCOMP           TRUE
#define             OPC_EXHANDLER           TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCENTRY.C - Entry points into the query compilation phase
**
**  Description:
**      This file contains all the entry points, currently only one, for query 
**	compilation phase.
**
**	    opc_exhandler()	- exception handler for OPC.
**	    opcquerycomp()	- main entry point to OPC.
**
**  History:
**      20-jul-86 (seputis)
**          initial creation
**      24-jul-86 (eric)
**	    began coding
**	29-jan-89 (paul)
**	    Added support for compiling rule action lists.
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	06-nov-90 (stec)
**	    Changed opc_querycomp() to return correct error code in case of
**	    QSO_UNLOCK call failure.
**	    Added better error handling (more info) in case of a failure to
**	    create a QSF object.
**	26-nov-90 (stec)
**	    Modified opc_querycomp().
**	28-dec-90 (stec)
**	    Modified opc_querycomp().
**	11-jan-91 (stec)
**	    Added an exception handler.
**      15-oct-91 (anitap)
**          Modified opc_querycomp() such that OPS_STATE is printed only once.
**	    Added an exception handler.
**	23-jun-92 (davebf)
**	   Modified for Sybil.
**	14-may-92 (rickh)
**	    Added OP199 trace point to print out QEPs without cost statistics
**	    chatter.
**	8-nov-92 (ed)
**	    replace distributed code accidently deleted in outer join integration
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**	26-nov-93 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to qsf_owner whose type has
**	    changed.
**      2-may-95 (hanch04,inkdo01)
**          Added check for  && global->ops_subquery
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 opc_exhandler(
	EX_ARGS *ex_args);
void opc_querycomp(
	OPS_STATE *global);
static void opc_checkcons(
	PST_STATEMENT *stmtp,
	DMU_CB *dmucb);

/*{
** Name: opc_exhandler() - Exception handler for OPC.
**
** Description:
**      This is the exception handler for OPC. Currently it is used for
**	dealing with floating-point number overflow.
**
** Inputs:
**      ex_args                         EX_ARGS struct.
**	    .ex_argnum			Tag for the exception that occurred.
**
** Outputs:
**	none.
**
**	Returns:
**	    The signal is resignaled if it is not recognized.
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**      11-jan-91 (stec)
**          Created.
*/
STATUS
opc_exhandler(
		EX_ARGS	*ex_args)
{
    STATUS  stat; 

    switch (EXmatch(ex_args->exarg_num, (i4)1, (EX)EXFLTOVF))
    {
	case 1:
	    {
		stat = EXDECLARE;
		break;
	    }
	default:
	    {
		stat = EXRESIGNAL;
		break;
	    }
    }

    return(stat);
}

/*{
** Name: opc_querycomp	- query compilation phase of optimizer
**
** Description:
**      This phase (routine) will convert an OPS_STATE struct and 
**	a list of OPS_SUBQUERY structs into a QP (QEF_QP_CB) and 
**	a list of action headers (QEF_AHD) for QEF.
**
** Inputs:
**      global
**	    ptr to global state variable
**
** Outputs:
**      global->ops_caller_cb->opf_qep
**	    QSF qep initialized
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
**      20-jul-86 (seputis)
**          null routine created
**      24-jul-86 (eric)
**          written
**      10-nov-86 (seputis)
**          saved QSF object information for error recovery
**      1-may-88 (eric)
**          added changes for DB procedures.
**	29-jan-89 (paul)
**	    Enahnced to support rules processing. The overall control flow
**	    has been changed slightly so that all statements are processed in
**	    an identical manner. OPC must be called once per statement
**	    (including statements in rule lists) and then once at the end of
**	    each user statement or db procedure to complete the compilation.
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	    The statement compiled may be a random list of statements w/o any
**	    associated query tree.
**	06-nov-90 (stec)
**	    Changed error code to be returned in case of QSO_UNLOCK call
**	    failure. Added code to log info when a failure occurs on
**	    QSO_CREATE request.
**	26-nov-90 (stec)
**	    Added initialization of print buffer for trace code.
**	28-dec-90 (stec)
**	    Changed code to reflect fact that print buffer is in OPC_STATE
**	    struct.
**      26-jun-91 (seputis)
**          mark optimizer as being in query compilation for error reporting
**      15-oct-91 (anitap)
**          Modified opc_querycomp() so that OPS_STATE structure is printed
**          only on the first call to the routine opc_querycomp().
**	23-jun-92 (davebf)
**	    Modified for Sybil
**	14-may-92 (rickh)
**	    Added OP199 trace point to print out QEPs without cost statistics
**	    chatter.
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**	2-may-95 (hanch04,inkdo01)
**	    Added check for  && global->ops_subquery
**	15-june-06 (dougi)
**	    Added support for "before" triggers.
**	5-may-2007 (dougi)
**	    Check for constraints in create table to use for default structure.
**	30-oct-2007 (dougi)
**	    Support cached dynamic query plans.
**	14-Nov-2008 (kiria01) b120998
**	    Handle multi-thread QSO_CREATE clash better - ignore it
**	    and use the created object anyway.
**	02-Dec-2008 (wanfr01) 
**	    SIR 121316 - created trace point op212 to allow enabling 
**	    table_auto_structure.
**	10-march-2009 (dougi) bug 121773
**	    Changes to allow locator and non-locator versions of same 
**	    cached dynamic query plan to be separately stored.
**	29-may-2009 (wanfr01) bug 122125
**	    dbid needs to be added to QSF object name to avoid using it in
**	    the wrong database.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Dig autostruct out of the DMU_CHARACTERISTICS.
*/


VOID
opc_querycomp(
		OPS_STATE          *global)
{
    DB_STATUS	    	ret;

    global->ops_gmask |= OPS_OPCEXCEPTION;  /* mark facility as being in OPC */
#ifdef OPT_F033_OPF_TO_OPC
    if (opt_strace(global->ops_cb, OPT_F033_OPF_TO_OPC) == TRUE)
    {
	char	temp[OPT_PBLEN + 1];
	bool	init = 0;

	if (global->ops_cstate.opc_prbuf == NULL)
	{
	    global->ops_cstate.opc_prbuf = temp;
	    init++;
	}

	/* Trace all of 'global' */
        if (global->ops_statement != NULL)
        {
	    opt_state(global);
	}
	    
	if (init)
	{
	    global->ops_cstate.opc_prbuf = NULL;
	}
    }
#endif

    if ( opt_strace(global->ops_cb, OPT_F071_QEP_WITHOUT_COST ) == TRUE && global->ops_subquery)
    {
	opt_cotree_without_stats( global );
    }

    /* If this is CREATE TABLE, check for primary, unique, foreign key
    ** constraints to use for default base table structure. */
    if (global->ops_statement &&
	global->ops_statement->pst_type == PST_CREATE_TABLE_TYPE &&
	global->ops_statement->pst_specific.pst_createTable.
			pst_createTableFlags == PST_CRT_TABLE)
    {
	QEU_CB *qeucb = global->ops_statement->pst_specific.pst_createTable.pst_createTableQEUCB;
	DMU_CB *dmucb = (DMU_CB *) qeucb->qeu_d_cb;
	bool checkit = FALSE;

	if (BTtest(DMU_AUTOSTRUCT, dmucb->dmu_chars.dmu_indicators))
	    checkit = (dmucb->dmu_chars.dmu_flags & DMU_FLAG_AUTOSTRUCT) != 0;
	else
	    checkit = opt_strace(global->ops_cb, OPT_F084_TBLAUTOSTRUCT ) ||
			global->ops_cb->ops_alter.ops_autostruct != 0;
	if (checkit)
	    opc_checkcons(global->ops_statement, dmucb);
    }

    /* On entry for rule processing, assume ops_qpinit == TRUE. There	    */
    /* is no need to allocate a memory stream since we are contuing	    */
    /* processing on the QP that was started by the triggering statement. */
    if (global->ops_qpinit == FALSE)
    {
	/* First, lets open the stack ULM memory stream that OPC uses */
	opu_Osmemory_open(global);

	/* Tell QSF that we want to store an object; */
	global->ops_qsfcb.qsf_obj_id.qso_type = QSO_QP_OBJ;
	if (global->ops_procedure->pst_flags & PST_REPEAT_DYNAMIC)
	{
	    char	*p;

	    global->ops_qsfcb.qsf_obj_id.qso_lname = sizeof(DB_CURSOR_ID) + sizeof(i4);
	    MEfill(sizeof(global->ops_qsfcb.qsf_obj_id.qso_name), 0,
		   global->ops_qsfcb.qsf_obj_id.qso_name);
	    MEcopy((PTR)&global->ops_procedure->pst_dbpid.db_cursor_id[0],
		   sizeof (global->ops_procedure->pst_dbpid.db_cursor_id[0]),
		   (PTR)global->ops_qsfcb.qsf_obj_id.qso_name);
	    p = (char *) global->ops_qsfcb.qsf_obj_id.qso_name + 2*sizeof(i4);
	    if (global->ops_caller_cb->opf_locator)
		MEcopy((PTR)"ql", sizeof("ql"), p);
	    else MEcopy((PTR)"qp", sizeof("qp"), p);
	    p = (char *) global->ops_qsfcb.qsf_obj_id.qso_name + sizeof(DB_CURSOR_ID);
	    I4ASSIGN_MACRO(global->ops_caller_cb->opf_udbid, *(i4 *) p); 


	}
	else if (   global->ops_procedure->pst_isdbp == TRUE
	    || (   global->ops_qheader != NULL
		&& (global->ops_qheader->pst_mask1 & PST_RPTQRY)
	       )
	   )
	{
	    global->ops_qsfcb.qsf_obj_id.qso_lname = 
		sizeof (global->ops_procedure->pst_dbpid);
	    MEcopy((PTR)&global->ops_procedure->pst_dbpid, 
		   sizeof (global->ops_procedure->pst_dbpid),
		   (PTR)global->ops_qsfcb.qsf_obj_id.qso_name);
	}
	else
	{
	    global->ops_qsfcb.qsf_obj_id.qso_lname = 0;
	}

	/* Also allow for the case where a concurrent clash causes a new
	** object at an awkward point */
	if ((ret = qsf_call(QSO_CREATE, &global->ops_qsfcb)) != E_DB_OK &&
	    !(ret == E_DB_ERROR &&
		global->ops_qsfcb.qsf_error.err_code == E_QS001C_EXTRA_OBJECT) &&
	    !((global->ops_procedure->pst_flags & PST_SET_INPUT_PARAM)  &&
	     global->ops_qsfcb.qsf_error.err_code == E_QS001C_EXTRA_OBJECT))
	{
	    /* if object exists and we have a named query plan. */
	    if (global->ops_qsfcb.qsf_error.err_code
		    == E_QS000A_OBJ_ALREADY_EXISTS
	       )
	    {
		/* Log query info */
		QSO_OBID    *obj = &global->ops_qsfcb.qsf_obj_id;
		char	    *qrytype;
		char	    *objtype;
		char	    *objname;
		char	    *qrytext;
		char	    tmp[(DB_OWN_MAXNAME + DB_CURSOR_MAXNAME)  + 3 + 1];
		DB_STATUS   status;
		QSF_RCB	    qsf_rb;
		PSQ_QDESC   *qdesc;

		if (global->ops_procedure->pst_isdbp == TRUE)
		    qrytype = "database procedure";
		else if (global->ops_qheader != NULL
			 && (global->ops_qheader->pst_mask1 & PST_RPTQRY)
			)
		    qrytype = "repeat query";
		else
		    qrytype = "non-repeat query";

		objtype = "QSO_QP_OBJ";

		if (obj->qso_lname == 0)
		{
		    objname = "QSF object has no name";
		}
		else
		{
		    char	    fmt[30];
		    DB_CURSOR_ID    *curid;
		    char	    *user;
		    i4		    *dbid;

		    curid = (DB_CURSOR_ID *)obj->qso_name;
		    user = curid->db_cur_name + DB_CURSOR_MAXNAME;
		    dbid = (i4 *)(user + DB_OWN_MAXNAME);

		    STprintf(fmt, ":%%lx:%%lx:%%.%ds:%%.%ds:%%lx:",
			DB_CURSOR_MAXNAME, DB_OWN_MAXNAME);

		    STprintf(tmp, fmt, (i4)curid->db_cursor_id[0], 
			(i4)curid->db_cursor_id[1],
			curid->db_cur_name, user, (i4)(*dbid));

		    objname = tmp;
		}
 
		qsf_rb.qsf_type = QSFRB_CB;
		qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
		qsf_rb.qsf_length = sizeof(qsf_rb);
		qsf_rb.qsf_owner = (PTR)DB_OPF_ID;
		qsf_rb.qsf_obj_id.qso_handle = global->ops_caller_cb->opf_thandle;

		qrytext = "Query text was not available.";

		if (qsf_rb.qsf_obj_id.qso_handle != NULL)
		{
		    status = qsf_call(QSO_INFO, &qsf_rb);

		    if (DB_SUCCESS_MACRO(status))
		    {
			qdesc = (PSQ_QDESC*) qsf_rb.qsf_root;

			qrytext = qdesc->psq_qrytext;
		    }
		}

		/* log an error */
		opx_lerror((OPX_ERROR)E_OP089F_QSF_FAILCREATE, (i4)4,
		    (PTR)qrytype, (PTR)objtype, (PTR)objname, (PTR)qrytext);
	    }

	    opx_verror(ret, E_OP0882_QSF_CREATE, 
		global->ops_qsfcb.qsf_error.err_code);
	}

	/* Put the handle for the QEP into the callers CB; 
	** - will be used for deallocation in case of an error
	** - both the object id and the lock id are needed in order to destroy
	** the object
	*/
	STRUCT_ASSIGN_MACRO(global->ops_qsfcb.qsf_obj_id,
	    global->ops_caller_cb->opf_qep);
	global->ops_qplk_id = global->ops_qsfcb.qsf_lk_id;
	global->ops_qpinit = TRUE;

	/* Allocate and initialize the QP. */
	opc_iqp_init(global);
    }

    /* Continue the QP compilation by adding the current statement */
    if (global->ops_statement != NULL)
    {
	opc_cqp_continue(global);
    }

    /* if it's time to stop compiling the query, then lets close stuff. */
    /* The caller is responsible for making one additional call to OPC	    */
    /* with ops_statement == NULL after all statements in the QP we are	    */
    /* currently building have been compiled. Note that this is a change    */
    /* from the previous version of this routine which required the extra   */
    /* call only if a db procedure was being compiled. Such a call must	    */
    /* also be made after the last statement in each rule list. This allows */
    /* OPC to link all conditionals statements in the rule list together    */
    /* before continuing with the next user statement to be compiled. */
    if (global->ops_statement == NULL)
    {
	/* We're finished compiling all of the statements, so lets finish
	** the QP
	*/
	opc_fqp_finish(global);

	/* The QP is only associated with the outer query, not a rule list  */
	if (!global->ops_inAfterRules && !global->ops_inBeforeRules)
	{
	    /* Tell QSF what the root of the QEP is; */
	    global->ops_qsfcb.qsf_root = (PTR) global->ops_cstate.opc_qp;
	    if ((ret = qsf_call(QSO_SETROOT, &global->ops_qsfcb)) != E_DB_OK)
	    {
		opx_verror(ret, E_OP0883_QSF_SETROOT, 
					global->ops_qsfcb.qsf_error.err_code);
	    }

	    if ((ret = qsf_call(QSO_UNLOCK, &global->ops_qsfcb)) != E_DB_OK)
	    {
		opx_verror(ret, E_OP089E_QSF_UNLOCK, 
		    global->ops_qsfcb.qsf_error.err_code);
	    }

	    /* Now lets close the stack ULM memory stream that OPC used */
	    opu_Csmemory_close(global);
	}
    }
    global->ops_gmask &= (~OPS_OPCEXCEPTION);  /* mark facility as leaving OPC */
}


/**
** Name:	opc_checkcons - look for constraint action that can be used
**	to set default base table structure.
**
** Inputs:	stmtp	- ptr to PST_STATEMENT of CREATE TABLE
**
** Outputs:	none
**
** Side Effects: sets PST_TABLE_STRUCT flag in chosen constraint action
**
** History:
**
**	5-may-2007 (dougi)
**	    Written.
**	28-Jul-2010 (kschendel) SIR 124104
**	    Continue the hand-off of compression, so that eventually it can
**	    end up in the create integrity action header for QEF.
*/

static VOID
opc_checkcons(
PST_STATEMENT	*stmtp,
DMU_CB		*dmucb)

{
    PST_STATEMENT	*wptr;
    bool		gotone = FALSE;
    i4			i;


    /* Not much to this - loop 3 times (if needed) over CONSTRAINT
    ** actions. First, looking for primary key, then unique constraint, 
    ** finally referential (foreign key) constraint. Quit loops when
    ** one is found. */

    for (i = 0; i < 3 && !gotone; i++)
     for (wptr = stmtp->pst_link; wptr && wptr->pst_type == 
		PST_CREATE_INTEGRITY_TYPE && !gotone; wptr = wptr->pst_link)
     {
	if (i == 0 && wptr->pst_specific.pst_createIntegrity.
			pst_integrityTuple->dbi_consflags & CONS_PRIMARY)
	{
	    gotone = TRUE;
	    break;
	}
	else if (i == 1 && wptr->pst_specific.pst_createIntegrity.
			pst_integrityTuple->dbi_consflags & CONS_UNIQUE)
	{
	    gotone = TRUE;
	    break;
	}
	else if (i == 2 && wptr->pst_specific.pst_createIntegrity.
			pst_integrityTuple->dbi_consflags & CONS_REF)
	{
	    gotone = TRUE;
	    break;
	}
     }

    if (gotone)
    {
	wptr->pst_specific.pst_createIntegrity.pst_createIntegrityFlags
			|= PST_CONS_TABLE_STRUCT;
	/* Hand off table compression setting too, which is in turn passed
	** to QEF via the create integrity action.
	** Note: only need to worry about data compression, the original
	** table couldn't reasonably have had key compression because
	** the whole idea here is that it is autostruct.  The table
	** was probably originally declared heap.
	*/
	wptr->pst_specific.pst_createIntegrity.pst_autocompress =
		dmucb->dmu_chars.dmu_dcompress;
    }
}
