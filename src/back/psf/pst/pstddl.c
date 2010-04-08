/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <er.h>
#include    <me.h>
#include    <qu.h>
#include    <rdf.h>
#include    <psfindep.h>
#include    <pshparse.h>

/*
**
**  Name: PSTDDL.C - Functions for making statement blocks 
**                   for DDL statement trees (DDL => Data Definition Language)
**
**  Description:
**      This file contains functions for making statement blocks/nodes.
**
**          pst_crt_tbl_stmt - Make a CREATE_TABLE statement node, fill it in, 
**			       initialize its next and link ptrs to NULL,
**			       make a PROCEDURE node to head the tree,
**			       and stick the CREATE_TABLE node into it.
**          pst_attach_to_tree - gets root of DDL tree from QSF, and attaches
**				 a statement list to the end of the tree
**	    pst_crt_schema -	Make a CREATE_SCHEMA statement node, etc.,
**				for CREATE SCHEMA
**          pst_ddl_header   - Make a PROCEDURE node & put a stmt tree into it.
**
**  History:
**      28-aug-92 (rblumer)    
**          created.
**	09-dec-92 (anitap)
**	    Changed the order of #include <qsf.h> for CREATE SCHEMA.
**	12-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    Added pst_crt_schema()
**	23-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    Change pss_object to point to PST_PROCEDURE node in pst_crt_schema()
**      04-dec-92 (rblumer)    
**          added pst_attach_to_tree function;
**          initialize pst_flags in PST_PROCEDURE nodes;
**          moved procnode allocation to separate function: pst_ddl_header.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed declaration of qsf_call()
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner/qeu_owner which has changed type
**	    to PTR.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
*/

/*
** Name: pst_crt_tbl_stmt - allocate a PST_CREATE_TABLE statement node
**
** Description:
**	Open a memory stream, allocate PST_CREATE_TABLE statement node, 
**	allocate QEU_CB and initialize its header, and put QEU_CB into the
**	PST_CREATE_TABLE node.  Then allocate a PST_PROCEDURE node to head
**	the query tree, put the CREATE_TABLE node into it, and place the
**	PST_PROCEDURE node at the root of the QSF object.
** 
**      (Designed to replace calls to psl_qeucb for CREATE TABLE operations
**       that will now go through OPF and QEF; thus much of this code was
**       copied from psl_qeucb.  This function could eventually be expanded
**       to replace any use of psl_qeucb where the DDL is being converted to 
**       use query trees.) 
** 
** Input:
**	sess_cb			PSF session CB
**	    pss_ostream		memory stream from which PST_CREATE_TABLE node
**				will be allocated
**	    pss_dbid		database id for this session
**	    pss_sessid		session id
**	qmode			query mode of the statement (STMT.pst_node)
**	operation		opcode to store in QEU_CB (in PST_CREATE_TABLE)
**
** Output:
**	sess_cb
**	    pss_object		will contain address of the newly allocated
**				PST_CREATE_TABLE node
**	err_blk			filled in if an error occured.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	Opens a memory stream, allocates memory and sets root of QSF object
**      to point to that memory (a PST_CREATE_TABLE statement node).
**
** History:
**
**	28-aug-92 (rblumer)
**	    written
**	04-aug-93 (andre)
**	    zero-fill QEU_CB to ensure that accidentally set bits are not
**	    misinterpreted to have some meaning
**	22-jul-96 (ramra01 for bryanp)
**	    Support for Alter Table Add/Drop column
**	13-apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	13-may-2007 (dougi)
**	    Added support for table auto structure options.
*/
DB_STATUS
pst_crt_tbl_stmt(PSS_SESBLK *sess_cb, 
		 i4  qmode,
		 i4 operation, 
		 DB_ERROR *err_blk)
{
    DB_STATUS	     status;
    i4	     err_code;
    PST_STATEMENT    *stmt;
    PST_CREATE_TABLE *crt_node;
    QEU_CB	     *qeu_cb;
    
    status = psf_mopen(sess_cb, QSO_QP_OBJ, &sess_cb->pss_ostream, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(PST_STATEMENT), (PTR *) &stmt, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* initialize the statement node
     */
    MEfill(sizeof(PST_STATEMENT), (u_char) 0, (PTR) stmt);

    stmt->pst_mode = qmode;
    stmt->pst_type = PST_CREATE_TABLE_TYPE;

#ifdef xDEBUG
    stmt->pst_lineno = sess_cb->pss_lineno;
#endif

    crt_node = &(stmt->pst_specific.pst_createTable);
    
    /* set operation in based on qmode
    **     (could be expanded to include temp tables, modifies, ...)
    */
    switch (qmode) 
    {
    case PSQ_CREATE:
	crt_node->pst_createTableFlags = PST_CRT_TABLE;
	sess_cb->pss_crt_tbl_stmt = stmt;
	break;

    case PSQ_INDEX:
	crt_node->pst_createTableFlags = PST_CRT_INDEX;
	break;


    case PSQ_ATBL_ADD_COLUMN:
	crt_node->pst_createTableFlags = PST_ATBL_ADD_COLUMN;
	break;

    case PSQ_ATBL_DROP_COLUMN:
	crt_node->pst_createTableFlags = PST_ATBL_DROP_COLUMN;
	break;

    case PSQ_ATBL_ALTER_COLUMN:
	crt_node->pst_createTableFlags = PST_ATBL_ALTER_COLUMN;
	break;

    default:
	(VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, 
			 PSF_INTERR, &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }
    
    
    /* allocate the QEU control block
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(QEU_CB), (PTR *) &qeu_cb, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    crt_node->pst_createTableQEUCB = qeu_cb;

    /*
    ** zero fill QEU_CB to make sure we do not forget to initialize new fields
    ** added to the structure
    */
    MEfill(sizeof(QEU_CB), (u_char) 0, (PTR) qeu_cb);
    
    /* Fill in the control block header 
     */
    qeu_cb->qeu_length	    = sizeof(QEU_CB);
    qeu_cb->qeu_type	    = QEUCB_CB;
    qeu_cb->qeu_owner	    = (PTR)DB_PSF_ID;
    qeu_cb->qeu_ascii_id    = QEUCB_ASCII_ID;
    qeu_cb->qeu_db_id	    = sess_cb->pss_dbid;
    qeu_cb->qeu_d_id	    = sess_cb->pss_sessid;
    qeu_cb->qeu_eflag	    = QEF_EXTERNAL;
    qeu_cb->qeu_mask	    = 0;

    /* Give QEF the opcode 
     */
    qeu_cb->qeu_d_op	= operation;

    /* the rest of the CREATE productions assume that 
    ** pss_object points to the qeu_cb, so set that up
    */
    sess_cb->pss_object = (PTR) qeu_cb;
    

    /* allocate the proc header for the query tree
    ** and set the root of the QSF object to the head of the tree
    */
    status = pst_ddl_header(sess_cb, stmt, err_blk);

    if (DB_FAILURE_MACRO(status))
	return(status);
    
    return(E_DB_OK);

}  /* end pst_crt_tbl_stmt */
 

/*
** Name: pst_ddl_header -  allocate a procedure node to complete the query tree;
** 			   used for DDL queries.
**
** Description:
**	Allocates a PST_PROCEDURE statement node, fills it in and links
**	the DDL statement(s) into the procedure node.  Also sets root of the
**	QSF object to point to the procedure node.
**
**	This function (instead of pst_header) is used for DDL statements,
**	because pst_header expects a DML query tree and range table, which
**	DDL queries generally do not have.
**	
** Input:
**	sess_cb			PSF session CB
**	    pss_ostream		memory stream from which PST_CREATE_TABLE node
**				will be allocated
**	stmt			first statement in DDL stmt tree.
**
** Output:
**	sess_cb
**	    pss_ostream		root will now point to PST_PROCEDURE node
**	err_blk			filled in if an error occured.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory and sets root of QSF object to point to that memory
**	(i.e. a PST_PROCEDURE statement node). 
**
** History:
**
**	04-dec-92 (rblumer)
**	    written
*/
DB_STATUS
pst_ddl_header(
	       PSS_SESBLK    *sess_cb,
	       PST_STATEMENT *stmt,
	       DB_ERROR      *err_blk)
{
    PST_PROCEDURE   *proc_node;
    DB_STATUS	    status;

    /* now allocate the top-level procedure node, 
    ** put the create table statement node into it, 
    ** and fix it as the root of the statement tree in QSF
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(PST_PROCEDURE), (PTR *) &proc_node, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(sizeof(PST_PROCEDURE), (u_char) 0, (PTR) proc_node);

    proc_node->pst_vsn = (i2) PST_CUR_VSN;
    proc_node->pst_isdbp = FALSE;
    proc_node->pst_flags = 0;
    proc_node->pst_stmts = stmt;

    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR) proc_node, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    return (E_DB_OK);

}  /* end pst_ddl_header */	       



/*
** Name: pst_attach_to_tree - gets root of a tree from QSF 
**			      and attaches a stmt-list to the end of the tree
**
** Description:
**	Uses pss_ostream as the QSF object, and gets the root that object
**	from QSF.  It assumes the root is a PST_PROCEDURE node and that 
**	there is only ONE pst_statement hanging off of it.
**	Then it sets up the link and next pointers of that statement to 
**	point to the first statement in the stmt-list passed in.
** 
** Input:
**	sess_cb			PSF session CB
**	    pss_ostream		QSF memory stream from which to get tree
**	stmt_list		pointer to stmt to be added to end of tree
**				(these stmts are assumed to be allocated
**				 from the same memory stream, i.e. ostream)
** Output:
**	sess_cb
**	    pss_ostream		procedure tree pointed to by this object will
**				now contain stmt_list
**	err_blk			filled in if an error occured.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	None:
**
** History:
**
**	10-nov-92 (rblumer)
**	    written
*/
DB_STATUS
pst_attach_to_tree(
		   PSS_SESBLK    *sess_cb, 
		   PST_STATEMENT *stmt_list, 
		   DB_ERROR      *err_blk)
{
    QSF_RCB	qsf_rb;
    DB_STATUS	status;
    i4	err_code;
    PST_PROCEDURE    *proc_node;
    
    /* get head of statement tree from QSF (it's the root of the object);
    ** object is still exclusively locked from when we created it.
    */
    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(qsf_rb);
    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid = sess_cb->pss_sessid;
    STRUCT_ASSIGN_MACRO(sess_cb->pss_ostream.psf_mstream, qsf_rb.qsf_obj_id);

    status = qsf_call(QSO_INFO, &qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
			 PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    proc_node = (PST_PROCEDURE *) qsf_rb.qsf_root;

    /* attach statement list to end of tree
    ** (there is only one node in the tree--the CREATE_TABLE node)
    */
    proc_node->pst_stmts->pst_link = stmt_list;
    proc_node->pst_stmts->pst_next = stmt_list;
    
    return(E_DB_OK);

}  /* end pst_attach_to_tree */

/*
** Name: pst_crt_schema - allocate a PST_CREATE_SCHEMA statement node
**
** Description:
**	Open a memory stream, allocate PST_CREATE_SCHEMA statement node.
**	Then allocate a PST_PROCEDURE node to head the query tree,
**	put the CREATE_TABLE node into it, and place the
**	PST_PROCEDURE node at the root of the QSF object.
** 
** Input:
**	sess_cb			PSF session CB
**	    pss_ostream		memory stream from which PST_CREATE_SCHEMA
**				node will be allocated
**	    pss_dbid		database id for this session
**	    pss_sessid		session id
**	qmode			query mode of the statement (STMT.pst_node)
**
** Output:
**	sess_cb
**	    pss_object		will contain address of the newly allocated
**				PST_CREATE_SCHEMA node
**	err_blk			filled in if an error occured.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	Opens a memory stream, allocates memory and sets root of QSF object
**      to point to that memory (a PST_CREATE_SCHEMA statement node).
**
** History:
**
**	12-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    written
**	23-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    Change pss_object to point to PST_PROCEDURE node in pst_crt_schema()
**	    Change pst_link to point to PST_STATEMENT (self)
*/

DB_STATUS
pst_crt_schema(PSS_SESBLK *sess_cb, 
		 i4  qmode,
		 DB_ERROR *err_blk)
{
    DB_STATUS	 	status;
    i4	 	err_code;
    PST_STATEMENT	*stmt;
    PST_PROCEDURE	*prnode;
    PST_CREATE_SCHEMA	*crt_node;
    
    status = psf_mopen(sess_cb, QSO_QP_OBJ, &sess_cb->pss_ostream, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(PST_STATEMENT), (PTR *) &stmt, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* initialize the statement node
     */
    MEfill(sizeof(PST_STATEMENT), (u_char) 0, (PTR) stmt);

    stmt->pst_mode = qmode;
    stmt->pst_type = PST_CREATE_SCHEMA_TYPE;
    stmt->pst_link = stmt;

#ifdef xDEBUG
    stmt->pst_lineno = sess_cb->pss_lineno;
#endif

    crt_node = &(stmt->pst_specific.pst_createSchema);
    
    /* now allocate the top-level procedure node, 
    ** put the create schema statement node into it, 
    ** and fix it as the root of the statement tree in QSF
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(PST_PROCEDURE), (PTR *) &prnode, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(sizeof(PST_PROCEDURE), (u_char) 0, (PTR) prnode);

    prnode->pst_vsn = (i2) PST_CUR_VSN;
    prnode->pst_isdbp = FALSE;
    prnode->pst_flags = 0;
    prnode->pst_stmts = stmt;

    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR) prnode, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    /* the rest of the CREATE SCHEMA productions assume that 
    ** pss_object points to the PST_PROCEDURE, so set that up
    */
    sess_cb->pss_object = (PTR) prnode;

    return(E_DB_OK);

} /* end pst_crt_schema */
