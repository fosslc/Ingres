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
#include	<dudbms.h>
#include	<ulf.h>
#include        <cs.h>
#include	<scf.h>
#include	<ulm.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmucb.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<qeuqcb.h>
#include	<psfindep.h>
#include	<rqf.h>
#include	<rdf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<tr.h>
#include	<psfparse.h>
#include	<rdftree.h>

/* 
** prototype definitions 
*/

/* rdd_dview - setup distributed view info */
static DB_STATUS rdd_dview(
	RDF_GLOBAL	*global,
	QEUQ_CB	*qeuq_cb);

/* rdw_tuple - put data into IITREE catalog tuples */
static DB_STATUS rdw_tuple(
	RDF_GLOBAL	*global,
	PTR		ptr,
	i4		len,
	QEF_DATA	**qdata);

/* rdw_tree - put tree into IITREE catalog tuples */
static DB_STATUS rdw_tree(
	RDF_GLOBAL	*global,
	PST_QNODE	*tree,
	QEF_DATA	**qdata);

/* rdf_nzero	- Test for zero byte */
static bool
rdf_nzero(
        i4                 size,
	char               *bptr);

/**
**
**  Name: RDFUPDATE.C - append/delete qrymod information of a table.
**
**  Description:
**	This file contains the routines for append/delete qrymod information
**	of a table.
**
**	rdf_update - Append/delete qrymod information of a table.
**
**  History:
**      15-Aug-86 (ac)
**          written
**	12-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added support for maintaining iiusergroup & iiapplication_id
**	    catalogs.
**	19-mar-89 (neil)
**	    modified to support rules.
**	13-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added support for maintaining the iidbpriv catalog.
**      17-jul-89 (jennifer)
**          Added support for maintaining iiuser, iilocation, iisecuritystate,
**          iidbaccess.
**	25-jul-89 (neil)
**	    Fix bug where qrymod_id was checked for without checking if
**	    we're really deleting permits or integrities.
**	21-sep-89 (ralph)
**	    Fix support for maintaining iiuser (ALTER/DROP USER).
**	    Fix support for maintaining iilocations (ALTER/DROP LOCATION).
**	    Remove support for maintaining iidbaccess.
**	25-oct-89 (neil)
**	    Alerters - Support for CREATE/DROP EVENT and EVENT permissions.
**	16-jan-90 (ralph)
**	    Add support for dropping security alarms.
**      15-dec-89 (teg)
**          add logic to put ptr to RDF Server CB into GLOBAL for subordinate
**          routines to use.
**	17-apr-90 (teg)
**	    added logic to suppress E_QE0015 for drop permit and drop
**	    integrity if user specifies invalid permit/integrity number.
**	    This is for bug 4390.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	08-dec-92 (anitap)
**	    Changed the order of header file qsf.h for CREATE SCHEMA.
**	10-feb-93 (teresa)
**	    Add ming hint NO_OPTIM for deannaw for ris_us5
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	27-sep-93 (stephenb)
**	    Define qeuq_cb.qeuq_tabname in rdf_update() for qef_call().
**	3-feb-94 (arc)
**	    Added NO_OPTIM for su4_cmw to resolve iidbdb creation failing
**	    using Sun's ansi compiler (SC1.0)
**	11-feb-94 (robf)
**	    Added NO_OPTIM for su4_u42 to resolve iidbdb creation failing
**	    using Sun's ansi compiler (SC1.0)
**	25-feb-94 (ajc)
**	    Added NO_OPTIM for hp8_bls to resolve iidbdb creation failing
**	    using hp ansi compiler (c89). 
**      08-feb-95 (chech02)
**          Added NO_OPTIM for rs4_us5 (AIX 4.1).
**      28-feb-97 (nanpr01)
**	    Alter Table drop column needs to pack the rule and integrity
**	    tree and the tree id was getting reset.
**      06-oct-98 (matbe01)
**          Added NO_OPTIM for nc4_us5 (NCR).
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
**	    Supply session id to QEF in qeuq_d_id.
**      16-apr-2001 (stial01)
**          rdw_tree() Added PST_MOP and PST_OPERAND cases
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**/

/*
NO_OPTIM=ris_us5 su4_cmw su4_u42 hp8_bls rs4_us5 nc4_us5 i64_aix
*/

/*
**  Forward and/or External function references.
*/


/*
** Name: rdd_dview - setup distributed view info 
**
** Description:
**	This routine is called to setup view infomation at create view time.
**
**  Inputs:
**	global				pointer to RDF state variables
**  Outputs:

**  Return:
**	E_DB_{OK, WARN, ERROR, FATAL}
**
** Side Effects:
**		none
** History:
**      13-nov-87 (mings)
**          written
**	16-jul-92 (teresa)
**	    implement prototypes
**	08-sep-93 (teresa)
**	    fix bug 54383 (Decmial Data types).
*/
static DB_STATUS
rdd_dview(  RDF_GLOBAL	*global,
	    QEUQ_CB	*qeuq_cb)
{

    DB_STATUS	    status;
    DB_DATA_VALUE   dv;
    ADF_CB	    *adfcb;
    RDR_RB	    *rdf_rb = &global->rdfcb->rdf_rb;
    DMU_CB	    *dmu_cb = (DMU_CB *)rdf_rb->rdr_dmu_cb;
    DMF_ATTR_ENTRY  *col_ptr;
    DB_DATA_VALUE   col_width;
    DB_DT_ID	    datatype;
    i4		    i;
    
    /* language type */
    if (rdf_rb->rdr_status & DBQ_SQL)
	qeuq_cb->qeuq_ddb_cb.qeu_1_lang = DB_SQL;			
    else
	qeuq_cb->qeuq_ddb_cb.qeu_1_lang = DB_QUEL;			
	    
    /* with check option */
    if (rdf_rb->rdr_status & DBQ_WCHECK)
	qeuq_cb->qeuq_ddb_cb.qeu_2_view_chk_b = TRUE;
    else			
	qeuq_cb->qeuq_ddb_cb.qeu_2_view_chk_b = FALSE;			
		
    /* attribute count */
		    
    qeuq_cb->qeuq_ano = dmu_cb->dmu_attr_array.ptr_in_count;
    qeuq_cb->qeuq_ddb_cb.qeu_3_row_width = 0;

    /* Get adf session control block for column length conversion.
    ** Note that the physical length of nullable column is different
    ** from user defined length */
    adfcb = (ADF_CB*)global->rdf_sess_cb->rds_adf_cb;

    /* compute logical column and row width */
    for (i = 0; i < qeuq_cb->qeuq_ano; i++)
    {
	col_ptr = ((DMF_ATTR_ENTRY **)dmu_cb->dmu_attr_array.ptr_address)[i];

	/* Setup db data value for column width conversion */

	dv.db_datatype = col_ptr->attr_type;
	datatype = (col_ptr->attr_type > 0) ? col_ptr->attr_type :
					      0 - col_ptr->attr_type;
	if( datatype == DB_DEC_TYPE)
	{
	    dv.db_prec = DB_PS_ENCODE_MACRO(col_ptr->attr_precision,
					    col_ptr->attr_size);
	    dv.db_length =  DB_PREC_TO_LEN_MACRO( (i4)col_ptr->attr_size);
	}
	else
	{
	    dv.db_length = col_ptr->attr_size;
	    dv.db_prec = 0;
	}
	status = adc_lenchk(adfcb, FALSE, &dv, &col_width);

	if (status)
	    return(status);

        /* This is value returned by adf. */
	col_ptr->attr_size = col_width.db_length;  
	qeuq_cb->qeuq_ddb_cb.qeu_3_row_width += col_ptr->attr_size;

    }
    return(status);	        
}

/*
** Name: rdw_tuple - put data into IITREE catalog tuples
**
** Description:
**	This routine is called with a data and a QEF_DATA control block.
**  It puts the data into the control block and allocates and initializes
**  new control blocks as necessary.
**
**  Inputs:
**	data				pointer to output data
**	len				length of output data
**  Outputs:
**	qdata				QEF control block containing tree tuple
**	error				location for any error messages.
**  Return:
**	E_DB_{OK, WARN, ERROR, FATAL}
**
** Side Effects:
**		none
** History:
**	9-oct-86 (daved)
**	    written
**      13-nov-87 (seputis)
**          modified for error handling
**      30-May-89 (teg)
**          structure DB_IITREE changed.  db_tree was a DB_TEXT_STRING followed
**          by a dbt_pad (char string 1023 long).  Now it is a u_i2 dbt_l_tree
**          followed by a char dbt_tree[DB_MAXTREE], and DB_MAXTREE is 1024.
**          So, all equations using old dbt_tree struct are modifed to use
**          dbt_l_tree or dbt_tree, as appropriate.
**	16-jul-92 (teresa)
**	    implement prototypes
**      28-feb-97 (nanpr01)
**	    Alter Table drop column needs to pack the rule and integrity
**	    tree and the tree id was getting reset.
*/
static DB_STATUS
rdw_tuple(
	RDF_GLOBAL	*global,
	PTR		ptr,
	i4		len,
	QEF_DATA	**qdata)
{
    DB_STATUS		status;

    status = E_DB_OK;
    while (len > 0)
    {
	DB_IITREE		*tuple;	    /* current tuple being filled */
	u_i2			length;
	i4			remaining;  /* number of bytes remaining in
                                            ** current tuple */
	QEF_DATA		*qdatap;    /* ptr to next current QEF_DATA
					    ** descriptor */

	qdatap = *qdata;
	tuple	= (DB_IITREE*) qdatap->dt_data;
	/* get length remaining in current tuple */
	remaining = sizeof (tuple->dbt_tree) - tuple-> dbt_l_tree;

	/* if no more space, get next tuple */
	if (remaining == 0)
	{
	    DB_IITREE		*ptuple;/* previous tuple which has been
					** filled */
	    ptuple = tuple;		/* remember the old tuple */
	    status = rdu_malloc(global, (i4)sizeof(QEF_DATA),
		(PTR *)&qdatap->dt_next);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    /* Link the allocated data block with the input data block. */
	    qdatap = qdatap->dt_next;
	    *qdata = qdatap;
	    qdatap->dt_size = sizeof(DB_IITREE);
	    qdatap->dt_next = NULL;
	    status = rdu_malloc(global, (i4)sizeof(DB_IITREE), (PTR *)&tuple);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    qdatap->dt_data = (PTR)tuple;
	    /* increment sequence number */
	    tuple->dbt_trseq	= ptuple->dbt_trseq + 1;
	    tuple->dbt_trmode	= ptuple->dbt_trmode;
	    tuple->dbt_version	= ptuple->dbt_version;
	    tuple->dbt_l_tree = 0;
	    STRUCT_ASSIGN_MACRO(ptuple->dbt_tabid, tuple->dbt_tabid);
	    STRUCT_ASSIGN_MACRO(ptuple->dbt_trid, tuple->dbt_trid);
	    /* if this is a distributed session, do not fill the entire
	    ** buffer with data.  save some spaces for the text length */
	    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
		remaining = DB_TREELEN -
			    sizeof(tuple->dbt_l_tree);
	    else
		remaining = sizeof (tuple->dbt_tree);
	}
	    
	/* get size to move */
	length  =  len > remaining ? remaining : len;
	/* move the data into the tuple */
	MEcopy((PTR)ptr, length, 
	    (PTR) &tuple->dbt_tree[tuple->dbt_l_tree]);
	ptr			    = (PTR)&((char *)ptr)[length];
	tuple->dbt_l_tree	    += length;
	len			    -= length;
    }
    return (status);
}

/*
** Name: rdw_tree - put tree into IITREE catalog tuples
**
** Description:
**	This routine walks the query tree, calling rdw_tuple to put the
**  tree into the tree relation. Do a pre-order walk so we can build the
**  tree again without too much problem. The walk is right to left.
**
**  Inputs:
**	tree				query tree.
**	ulm_rcb				memory stream in which tree tuples
**					are created.
**  Outputs:
**	qdata				QEF control block containing tree tuple
**	error				location for any error messages.
**  Return:
**	E_DB_{OK, WARN, ERROR, FATAL}
**
** Side Effects:
**		none
** History:
**	9-oct-86 (daved)
**	    written
**	4-feb-87 (daved)
**	    allow for root nodes with unions.
**      13-nov-87 (seputis)
**          resource management, iterate replacing recursion
**	09-mar-89 (neil)
**	    added PST_RULEVAR check.
**	16-jul-92 (teresa)
**	    implement prototypes
**	18-jan-00 (inkdo01)
**	    Add case expression node types.
*/
static DB_STATUS
rdw_tree(
	RDF_GLOBAL	*global,
	PST_QNODE	*tree,
	QEF_DATA	**qdata)
{
    DB_STATUS	    status;
    do 
    {
	i4		symsize;	/* size of symbol to write */
	{
	    /* place number of children into tree relation */
	    RDF_CHILD	children;
	    if (tree->pst_left)
	    {
		if (tree->pst_right)
		    children = RDF_LRCHILD;
		else
		    children = RDF_LCHILD;
	    }
	    else if(tree->pst_right)
		children = RDF_RCHILD;
	    else
		children = RDF_0CHILDREN;
	    status = rdw_tuple(global, (PTR) &children, (i4) sizeof (RDF_CHILD),
		qdata);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}

	switch (tree->pst_sym.pst_type)
	{
	case PST_AND:
	case PST_OR:
	case PST_UOP:
	case PST_BOP:
	case PST_AOP:
	case PST_COP:
	case PST_MOP:
	    symsize = sizeof(PST_OP_NODE);
	    break;
	case PST_CONST:
	    symsize = sizeof(PST_CNST_NODE);
	    break;
	case PST_RESDOM:
	case PST_BYHEAD:
	    symsize = sizeof(PST_RSDM_NODE);
	    break;
	case PST_VAR:
	    symsize = sizeof(PST_VAR_NODE);
	    break;
	case PST_AGHEAD:
	case PST_ROOT:
	case PST_SUBSEL:
	    symsize = sizeof(PST_RT_NODE);
	    break;
	case PST_QLEND:
	case PST_TREE:
	case PST_NOT:
	case PST_WHLIST:
	case PST_WHOP:
	case PST_OPERAND:
	    symsize = 0;
	    break;
	case PST_RULEVAR:
	    symsize = sizeof(PST_RL_NODE);
	    break;
	case PST_CASEOP:
	    symsize = sizeof(PST_CASE_NODE);
	    break;
	case PST_SORT:
	case PST_CURVAL:
	default:
	    rdu_ierror(global, E_DB_ERROR, E_RD012F_QUERY_TREE);
	    return(E_DB_ERROR);
	}
	/* put the symbol into the tree relation */
	status = rdw_tuple(global, (PTR) &tree->pst_sym, 
	    (i4) (symsize+sizeof(PST_SYMBOL)-sizeof(PST_SYMVALUE)),
	    qdata);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	if ((tree->pst_sym.pst_type == PST_CONST)
	    &&
	    tree->pst_sym.pst_value.pst_s_cnst.pst_origtxt)
	{   /* a null terminated text string exists so store this
            ** since it will be useful when recreating the query
	    ** for distributed */
	    DB_TEXT_STRING	    stringsize;	    /* use this to store
				    ** the length of the text string */
	    stringsize.db_t_count = STlength(tree->pst_sym.pst_value.pst_s_cnst.pst_origtxt);
	    status = rdw_tuple(global, 
		(PTR)&stringsize.db_t_count,
		(i4)sizeof(stringsize.db_t_count), 
		qdata);		    /* write the size of null terminated string */
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    status = rdw_tuple(global, 
		(PTR)tree->pst_sym.pst_value.pst_s_cnst.pst_origtxt,
		(i4)(stringsize.db_t_count), 
		qdata);		    /* write the null terminated string */
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	{   /* put the data value into the tree */
	    DB_DATA_VALUE   *data;
	    if ((data = &tree->pst_sym.pst_dataval)->db_data)
	    {
		status = rdw_tuple(global, (PTR) data->db_data, (i4) data->db_length,
		    qdata);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		symsize = symsize+data->db_length; /*adjust length for consistency
					** check of the PST_QNODE */
	    }
	    if (symsize != tree->pst_sym.pst_len)
	    {
		rdu_ierror(global, E_DB_ERROR, E_RD0130_PST_LEN);
		return(E_DB_ERROR);
	    }
	}
	/* if root node, do union first */
	if (tree->pst_sym.pst_type == PST_ROOT &&
	    tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	{
	    status = rdw_tree(global, 
		tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next, 
		qdata);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	/* do the right node */
	if (tree->pst_right)	    /* recurse down the right side */
	{
	    status = rdw_tree(global, tree->pst_right, qdata);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	tree = tree->pst_left;	    /* iterate down the left side */
    } while (tree);
    return (status);
}

/*{
** Name: rdf_nzero	- Test for zero byte
**
** Description:
**      Routine will return TRUE if a non-zero byte found 
**
** Inputs:
**      size                            size of memory block
**      bptr                            ptr to memory block
**
** Outputs:
**	Returns:
**	    TRUE if one non-zero byte found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-feb-88 (seputis)
**          initial creation
**      29-aug-88 (seputis)
**          fix unix casting problems
**	16-jul-92 (teresa)
**	    implement prototypes
[@history_template@]...
*/
static bool
rdf_nzero(
        i4                 size,
	char               *bptr)
{
    while(size--)
    {
	if ( *(bptr++))
	    return(TRUE);
    }
    return(FALSE);
}

/*{
** Name: rdu_treenode_to_tuple - PSF treenode to tuples conversion routine.
**
**	Internal call format:	rdu_treenode_to_tuple(root, mode, ulm_rcb, 
**							&data, error)
** Description:
**      This function converts query tree nodes to tree tuples in order to
**	store tree tuples in IITREE relation.
**	
** Inputs:
**
**      root				The root node of the query tree.
**	mode				type of query this tree represents
**	ulm_rcb				The ulm control block for allocating
**					space.
**
** Outputs:
**	data				The output data block which contains
**					tree tuples and the number of tuples
**					generated.
**	error				error block.
**	Returns:
**		
**	Exceptions:
**		
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          created.
**	9-oct-86 (daved)
**	    re-written
**      13-nov-87 (seputis)
**          modified for errors, error recovery, resource management
**      30-May-89 (teg)
**          structure DB_IITREE changed.  db_tree was a DB_TEXT_STRING followed
**          by a dbt_pad (char string 1023 long).  Now it is a u_i2 dbt_l_tree
**          followed by a char dbt_tree[DB_MAXTREE], and DB_MAXTREE is 1024.
**          So, dbt_tree.db_t_cnt changed to dbt_l_tree.
**	29-sep-89 (teg)
**	    Added support for Outter Joins.
**	04-feb-91 (andre)
**	    The following changes were made as a part of the project to
**	    support QUEL shareable repeat queries:
**	     - consistency check was modified to make sure that *qtree->pst_info
**	       structure, if it was allocated, contains no useful information;
**	     - consistency check was modified to not treat it as an error if
**	       PST_SQL_QUERY bit is set in qtree->pst_mask1.
**	07-nov-91 (teresa)
**	    pick up ingres63p change 261233:
**	    26-feb-1991 (teresa)
**		Structure of PST_QTREE changed for version 6 of 6.3/04.  The
**		Range tab was replaced with a pointer to an array of range tables.
**		(part of improved diagnostics for bug 35862)
**	29-jun-92 (teresa)
**	    modify consistency check E_RD0131_QUERY_TREE to permit 
**	    PST_CDB_IIDD_TABLES and PST_CATUPD_OK bits to be set.
**	16-jul-92 (teresa)
**	    implement prototypes
**	01-sep-92 (teresa)
**	    remove consistency check for pst_numparam == 0 since sybil changes
**	    will usually populate it with -1 now.
**	28-dec-92 (andre)
**	    when packing a tree for a new view, we will be given address of the
**	    query tree header (PST_QTREE *), and not a (PST_PROCEDURE *)
**	30-dec-92 (andre)
**	    PST_QTREE.pst_mask1 may have two new bits turned on:
**	    PST_AGG_IN_OUTERMOST_SUBSEL and PST_GROUP_BY_IN_OUTERMOST_SUBSEL;
**	    this information is collected for views and will be stored in
**	    RDF_QTREE.mask to be restored when  a view tree gets retrieved.
**	    This change is made simpler by the fact that as a part of 6.5
**	    upgradedb all views and permits will be dropped and recreated.
**	31-dec-92 (andre)
**	    and yet another new bit in PST_QTREE.pst_mask1 -
**	    PST_SINGLETON_SUBSEL; corresponding bit in RDF_QTREE.mask will be
**	    RDF_SINGLETON_SUBSEL
**	07-jan-93 (andre)
**	    for STAR, repackage IITREE tuples to consist solely of a u_i2 count
**	    field followed by the flattenned tree
**	27-aug-93 (andre)
**	    if storing away view tree, save id of the view's underlying base 
**	    table
**	17-nov-93 (andre)
**	    several new bits expected in PST_QTREE.pst_mask1:
**	      - PST_CORR_AGGR - view involves correlated aggregates;
**		corresponding bit in RDF_QTREE.mask will be RDF_CORR_AGGR
**	      -	PST_SUBSEL_IN_OR_TREE - view involves a SUBSEL node having OR 
**		for an ancestor; corresponding bit in RDF_QTREE.mask will be 
**		RDF_SUBSEL_IN_OR_TREE
**	      - PST_ALL_IN_TREE - view involves ALL; corresponding bit in 
**		RDF_QTREE.mask will be RDF_ALL_IN_TREE
**	      - PST_MULT_CORR_ATTRS - view involves multiple correlated 
**		attributes; corresponding bit in RDF_QTREE.mask will be 
**		RDF_MULT_CORR_ATTRS
**	6-dec-93 (robf)
**          Allow for PST_NEED_QRY_SYSCAT in flags.
**      28-feb-97 (nanpr01)
**          Alter Table drop column needs to pack the rule and integrity
**	    tree and the tree id was getting reset.
**	17-jul-00 (hayke02)
**	    Set RDF_INNER_OJ in version.mask if PST_INNER_OJ has been
**	    set in pst_mask1.
**	2-feb-06 (dougi)
**	    Replace pst_rngtree integer flag by PST_RNGTREE single bit flag.
**	12-july-06 (dougi)
**	    Fix error when derived table is included in view definition
**	    and write parse tree for derived table with derived table range
**	    table entry.
**	29-dec-2006 (dougi)
**	    Add support for WITH list elements, too.
**	16-aug-2007 (dougi)
**	    Moved pst_rptqry flag to pst_mask1.
**	10-dec-2007 (smeke01) b118405
**	    Added pst_mask1 handling for 3 new view properties: PST_AGHEAD_MULTI, 
**	    PST_IFNULL_AGHEAD and PST_IFNULL_AGHEAD_MULTI.
**	20-may-2008 (dougi)
**	    Add support for table procedures.
**	21-nov-2008 (dougi) Bug 121265
**	    Changes to tolerate parameterless tprocs in view definitions.
*/
DB_STATUS
rdu_treenode_to_tuple(
	RDF_GLOBAL  *global,
	PTR	    root,
	DB_TAB_ID   *tabid,
	i4	    mode,
	RDD_QDATA   *data)
{
    DB_STATUS	    status;
    DB_IITREE	    *tuple;
    QEF_DATA	    *qdata;
    PST_QTREE	    *qtree;
    RDF_CB	    *rdfcb = global->rdfcb;

    status = rdu_malloc(global, (i4)sizeof(*qdata), (PTR *)&qdata);
    if (DB_FAILURE_MACRO(status))
	return(status);
    /* Link the allocated data block with the input data block. */
    data->qrym_data = qdata;
    status = rdu_malloc(global, (i4)sizeof(*tuple), (PTR *)&tuple);
    if (DB_FAILURE_MACRO(status))
	return(status);
    tuple->dbt_trmode = mode;
    tuple->dbt_trseq = 0;
    tuple->dbt_version = 0;
    tuple->dbt_l_tree = 0;
    STRUCT_ASSIGN_MACRO(*tabid, tuple->dbt_tabid);
    qdata->dt_next = NULL;
    qdata->dt_size = sizeof (DB_IITREE);
    qdata->dt_data = (PTR) tuple;

    /*
    ** for DB_VIEW we will be given a (PST_QTREE *) rather than
    ** (PST_PROCEDURE *)
    */
    if (mode != DB_VIEW)
    {
	/*
	** validate contents of PST_PROCEDURE and PST_STATEMENT nodes - they
	** will NOT be written to IITREE
	*/

	PST_STATEMENT   *statementp;
	PST_PROCEDURE   *procedurep;

	procedurep = (PST_PROCEDURE *) root;

	if (procedurep->pst_isdbp
	    ||
	    !procedurep->pst_stmts
	    ||
	    procedurep->pst_parms
	    ||
	    rdf_nzero(sizeof(procedurep->pst_dbpid), (char *)&procedurep->pst_dbpid))
	{   /* consistency check on query tree */
	    rdu_ierror(global, E_DB_ERROR, E_RD0131_QUERY_TREE);
	    return(E_DB_ERROR);
	}

	statementp = procedurep->pst_stmts;
	if ((statementp->pst_type != PST_QT_TYPE)
	    ||
	    statementp->pst_next
	    ||
	    statementp->pst_link
	    )
	{   /* consistency check on query tree */
	    rdu_ierror(global, E_DB_ERROR, E_RD0131_QUERY_TREE);
	    return(E_DB_ERROR);
	}

	if ((mode == DB_INTG) || (mode == DB_RULE))
           STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_rb.rdr_tree_id, 
				tuple->dbt_trid);

	qtree = (PST_QTREE *) statementp->pst_specific.pst_tree;
    }
    else
    {
	qtree = (PST_QTREE *) root;
    }

    {
	RDF_QTREE	version;
	i4		valid_tree;

	valid_tree = PST_CATUPD_OK | PST_SQL_QUERY | PST_AGG_IN_OUTERMOST_SUBSEL
	    | PST_GROUP_BY_IN_OUTERMOST_SUBSEL | PST_SINGLETON_SUBSEL
	    | PST_CORR_AGGR | PST_SUBSEL_IN_OR_TREE | PST_ALL_IN_TREE
	    | PST_MULT_CORR_ATTRS | PST_NEED_QRY_SYSCAT | PST_INNER_OJ
	    | PST_AGHEAD_MULTI | PST_IFNULL_AGHEAD | PST_IFNULL_AGHEAD_MULTI;

	if (rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	    valid_tree |= PST_CDB_IIDD_TABLES;

	if ((qtree->pst_mask1 & PST_RPTQRY)
	    ||
	    qtree->pst_stflag
	    ||
	    qtree->pst_stlist
	    ||
/*	    rdf_nzero(sizeof(qtree->pst_restab), (char *)&qtree->pst_restab)
**	    ||
** PSF fills this in but it is not needed
*/
	    rdf_nzero(sizeof(qtree->pst_cursid), (char *)&qtree->pst_cursid)
	    ||
	    qtree->pst_delete
	    ||
	    (qtree->pst_updtmode != PST_UNSPECIFIED)
	    ||
	    rdf_nzero(sizeof(qtree->pst_updmap), (char *)&qtree->pst_updmap)
	    ||
					    /*
					    ** PST_QRYHDR_INFO structure gets
					    ** allocated for all queries, but it
					    ** should be left empty for views,
					    ** permits, and integrities
					    */
	    qtree->pst_info &&
	    qtree->pst_info->pst_1_usage
	    ||
					    /* verify that this is actually
					    ** a type of tree that we want to
					    ** write to the catalogs.  If
					    ** non-distributed, may have either
					    ** PST_SQL_QUERY or PST_CATUPD_OK
					    ** bits set.  When distirbuted, may
					    ** also have PST_CDB_IIDD_TABLES bit
					    ** set.  If anything else is set,
					    ** generate a consistency check. */
	    (qtree->pst_mask1 & ~valid_tree)
	    ||
	    (   qtree->pst_distr
	     && !(rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS))
	    )
	{   /* consistency check on query tree */
	    rdu_ierror(global, E_DB_ERROR, E_RD0131_QUERY_TREE);
	    return(E_DB_ERROR);
	}

	MEfill(sizeof(RDF_QTREE), (u_char)0, (PTR)&version);
	version.mode = qtree->pst_mode;
	version.vsn = PST_CUR_VSN;   /* FIXME - use pst_vsn when testing finished */
	version.resvno = qtree->pst_restab.pst_resvno; /* number of 
					    ** result range variable */
	version.numjoins = (i2) qtree->pst_numjoins;   /* num of joins in tree*/
	if (qtree->pst_agintree)
	    version.mask |= RDF_AGINTREE;
	if (qtree->pst_subintree)
	    version.mask |= RDF_SUBINTREE;
	if (qtree->pst_mask1 & PST_RNGTREE)
	    version.mask |= RDF_RNGTREE;
	if (qtree->pst_wchk)
	    version.mask |= RDF_WCHK;
	if (qtree->pst_mask1 & PST_AGG_IN_OUTERMOST_SUBSEL)
	    version.mask |= RDF_AGG_IN_OUTERMOST_SUBSEL;
	if (qtree->pst_mask1 & PST_GROUP_BY_IN_OUTERMOST_SUBSEL)
	    version.mask |= RDF_GROUP_BY_IN_OUTERMOST_SUBSEL;
	if (qtree->pst_mask1 & PST_SINGLETON_SUBSEL)
	    version.mask |= RDF_SINGLETON_SUBSEL;
	if (qtree->pst_mask1 & PST_CORR_AGGR)
	    version.mask |= RDF_CORR_AGGR;
	if (qtree->pst_mask1 & PST_SUBSEL_IN_OR_TREE)
	    version.mask |= RDF_SUBSEL_IN_OR_TREE;
	if (qtree->pst_mask1 & PST_ALL_IN_TREE)
	    version.mask |= RDF_ALL_IN_TREE;
	if (qtree->pst_mask1 & PST_MULT_CORR_ATTRS)
	    version.mask |= RDF_MULT_CORR_ATTRS;
	if (qtree->pst_mask1 & PST_INNER_OJ)
	    version.mask |= RDF_INNER_OJ;
	if (qtree->pst_mask1 & PST_AGHEAD_MULTI)
	    version.mask |= RDF_AGHEAD_MULTI;
	if (qtree->pst_mask1 & PST_IFNULL_AGHEAD)
	    version.mask |= RDF_IFNULL_AGHEAD;
	if (qtree->pst_mask1 & PST_IFNULL_AGHEAD_MULTI)
	    version.mask |= RDF_IFNULL_AGHEAD_MULTI;

	if (mode == DB_VIEW)
	{
	    /* save id of the view's underlying base table */
	    version.pst_1basetab_id = qtree->pst_basetab_id.db_tab_base;
	    version.pst_2basetab_id = qtree->pst_basetab_id.db_tab_index;
	}

	{   /* write out the range table */
	    i4		check_range;

            for (check_range = 0, version.range_size = qtree->pst_rngvar_count;
                 check_range < qtree->pst_rngvar_count; check_range++)
	    {
		/* each entry in the tree must be a PST_TABLE, PST_RTREE,
		** PST_DRTREE (derived table) or PST_WETREEi (WITH list 
		** element). */
		switch (qtree->pst_rangetab[check_range]->pst_rgtype)
		{
		  case PST_TABLE:
		  case PST_RTREE:
		  case PST_DRTREE:
		  case PST_WETREE:
		  case PST_TPROC_NOPARMS:
			break;
		  case PST_TPROC:
			if (qtree->pst_rangetab[check_range]->pst_rgroot ==
						(PST_QNODE *) NULL)
			    qtree->pst_rangetab[check_range]->pst_rgtype =
						PST_TPROC_NOPARMS;
			break;
		  case PST_UNUSED:
		  default:
		  {	/* consistency check on query tree */
			rdu_ierror(global, E_DB_ERROR, E_RD0131_QUERY_TREE);
			return(E_DB_ERROR);
		  }
		}
	    }

	}
	status = rdw_tuple(global, (PTR)&version, (i4)sizeof(version), &qdata);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	if (version.range_size)
	{
	    i4	    ctr;

	    /* write out range table if it exists.  The range table is a
	    ** series of structs that need not be contigious.  pst_rangetab is
	    ** a contigious array of ptrs to these structs */
	    for (ctr = 0; ctr < version.range_size; ctr++)
	    {
		if (qtree->pst_rangetab[ctr])
		{
		    status = rdw_tuple(global, (PTR)qtree->pst_rangetab[ctr], 
				    (i4)sizeof(PST_RNGENTRY), &qdata);
		    if (DB_FAILURE_MACRO(status))
			return (status);

		    if (qtree->pst_rangetab[ctr]->pst_rgtype == PST_DRTREE ||
			qtree->pst_rangetab[ctr]->pst_rgtype == PST_WETREE ||
			(qtree->pst_rangetab[ctr]->pst_rgtype == PST_TPROC &&
			qtree->pst_rangetab[ctr]->pst_rgroot != (PST_QNODE *) NULL))
		    {
			/* Process parse trees for derived tables and WITH
			** list elements. */
			status = rdw_tree(global, qtree->pst_rangetab[ctr]->
					pst_rgroot, &qdata);
			if (DB_FAILURE_MACRO(status))
			    return(status);
		    }
		    /* RDF trick to allow later processing of compressed
		    ** parse tree of parameter-less tproc invocation. The 
		    ** type is set back to PST_TPROC when we reload the tree. */
		    if (qtree->pst_rangetab[ctr]->pst_rgtype == PST_TPROC &&
			qtree->pst_rangetab[ctr]->pst_rgroot == 
							(PST_QNODE *) NULL)
			qtree->pst_rangetab[ctr]->pst_rgtype = 
							PST_TPROC_NOPARMS;
		}
		else
		{
			/* consistency check on query tree */
                        rdu_ierror(global, E_DB_ERROR, E_RD0131_QUERY_TREE);
                        return(E_DB_ERROR);
		} /* endif - is this range table entry defined? */
	    }  /* end loop to pack away each Range table entry */
	} /* endif -- are there range table entries to pack away? */
    }

    /* put the tree into the catalog */
    status = rdw_tree(global, qtree->pst_qtree, &qdata);
    if (DB_FAILURE_MACRO(status))
	return (status);
    data->qrym_cnt = ((DB_IITREE*)qdata->dt_data)->dbt_trseq+1;
				/* count the number of QEF_DATA blocks 
				** which should be equal to the sequence
                                ** number plus 1 */

    /*
    ** In distributed mode, QEF will generate tree tuples. RDF 
    ** should not pass DB_IITREE to QEF. all QEF_DATA in link list 
    ** (qeuq_cb.qeuq_tre_tup) will actually contains the tree thru 
    ** dt_data. */

    if (rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {
	RDC_TREE        tree;
	
	/* restructure the QEF_DATA to contain query text */
	for(qdata = data->qrym_data; qdata; qdata = qdata->dt_next)
	{
	    MEfill(1027, ' ', tree.treetree);
	    tuple = (DB_IITREE *) qdata->dt_data;
	    MEcopy((PTR)&tuple->dbt_l_tree, 
		    sizeof(tuple->dbt_l_tree),
		    (PTR)tree.treetree);			       
	    MEcopy((PTR)tuple->dbt_tree, 
		    tuple->dbt_l_tree,
		    (PTR)&tree.treetree[2]);
	    qdata->dt_size = tuple->dbt_l_tree + 2;
	    MEcopy((PTR)tree.treetree, qdata->dt_size,
		(PTR)qdata->dt_data);   
	}
    }		

    return(status);
}

/*{
** Name: rdu_qrytext_to_tuple - PSF querytext to tuples conversion routine.
**
**	Internal call format:	rdu_qrytext_to_tuple(global, mode, &data)
** Description:
**      This function converts query text to iiqrytext tuples in order to
**	store tree tuples in IIQRYTEXT relation.
**	
** Inputs:
**
**      length				The length of the querytext.
**	qrytext				The query text.
**	mode				type of query text represents
**	qry_status			status word for qrytext tuples
**	ulm_rcb				The ulm control block for allocating
**					space.
**
** Outputs:
**	data				The output data block which contains
**					iiqrytext tuples and count of tuples
**					generated.
**	error				error block.
**	Returns:
**		
**	Exceptions:
**		
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          created.
**	8-oct-86 (daved)
**	    updated
**	13-nov-87 (seputis)
**          resource management fix
**	16-jul-92 (teresa)
**	    implement prototypes
**	07-jan-93 (andre)
**	    rdu_qrytext_to_tuple() will no longer be static
**
**	    for STAR instead of packing IIQRYTEXT tuples, we will pack up to 238
**	    chars in DB_TEXT_STRING format inside the tuples (this used to be
**	    done in rdf_update() after calling rdu_qrytext_to_tuple(), but it
**	    makes more sense to do it here)
*/
DB_STATUS
rdu_qrytext_to_tuple(
	RDF_GLOBAL  *global,
	i4	    mode,
	RDD_QDATA   *data)
{
    DB_STATUS	    status;
    RDF_CB	    *rdfcb = global->rdfcb;
    RDR_RB	    *rdf_rb = &rdfcb->rdf_rb;
    i4		    length = rdf_rb->rdr_l_querytext;
    char	    *qrytext = (char *) rdf_rb->rdr_querytext;
    DB_IIQRYTEXT    *tuple;
    QEF_DATA	    **qdatapp;		/* data blocks containing output rows */
    i4		    seq_no;		/* sequence number */
    i4		    txtsize;		/* amount of room a tuple will hold */

    seq_no = 0;				/* sequence number */

    /* if distributed session, do not fill the entire buffer.
    ** save some spaces for text length */
    if (rdf_rb->rdr_r1_distrib & DB_3_DDB_SESS)
	txtsize	= sizeof(tuple->dbq_pad) + sizeof(tuple->dbq_text.db_t_text) -
		  sizeof(tuple->dbq_text.db_t_count);
    else
	txtsize	= sizeof (tuple->dbq_pad) + sizeof(tuple->dbq_text.db_t_text);

    /* set the pointer to the data block list */
    qdatapp = &data->qrym_data;

    /* move the text to tuple buffers as long as we have more data to move */
    for(;length > 0;)
    {
	QEF_DATA	    *qdatap;		/* ptr to QEF data block */
	i4		    len;
	DB_TEXT_STRING      *qtxt_str;
	
	status = rdu_malloc(global, (i4)sizeof(*qdatap), (PTR *)&qdatap);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	    
	status = rdu_malloc(global, (i4)sizeof(*tuple), (PTR *)&tuple);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	    
	/* Link the allocated data block with the input data block. */
	qdatap->dt_data = (PTR) tuple;
	qdatap->dt_next	= NULL;
	*qdatapp = qdatap;
	qdatapp = &qdatap->dt_next;

	/* determine how much of text goes into this tuple */
	len = length > txtsize ? txtsize : length;

	if (rdf_rb->rdr_r1_distrib & DB_3_DDB_SESS)
	{
	    qtxt_str = (DB_TEXT_STRING *) tuple;
	    
	    /*
	    ** for STAR, even though we have allocated an IIQRYTEXT tuple, we
	    ** will fill up to 240 bytes of it with query text stored in
	    ** DB_TEXT_STRING format
	    */
	    qdatap->dt_size = len + sizeof(qtxt_str->db_t_count);
	}
	else
	{
	    qtxt_str = &tuple->dbq_text;
	    
	    /* for local populate DB_IIQRYTEXT structure */
	    qdatap->dt_size = sizeof (DB_IIQRYTEXT);
	    tuple->dbq_mode = mode;
	    tuple->dbq_seq  = seq_no;
	    tuple->dbq_status = rdfcb->rdf_rb.rdr_status;
	}

	/* move the data */
	qtxt_str->db_t_count = len;
	MEcopy((PTR) qrytext, len, (PTR) qtxt_str->db_t_text);

	/* increment number of tuples created so far */
	seq_no++;
	
	/* adjust remaining length and current text pointer */
	length	-= len;
	qrytext += len;
	
    }	/* End of for. */
    
    data->qrym_cnt	= seq_no;
    return(status);
}

/*{
** Name: rdf_update - append/delete qrymod information of a table.
**
**	External call format:	status = rdf_call(RDF_UPDATE, &rdf_cb);
**
** Description:
**      This function updates qrymod information in system catalogs.
**	This is an operation used only by PSF for appending/deleting 
** 	qrymod information of a table to/from system catalogs.
** 	Currently, one request is allowed for each rdf_update() call.
**	This can be easily extended to have multiple requests per 
**	rdf_update() call if needed.
**
**  Usages:
**	To define a procedure :
**	    rdr_types_mask = RDR_PROCEDURE; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define an integrity :
**	    rdr_types_mask = RDR_INTEGRITIES; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define a protection :
**	    rdr_types_mask = RDR_PROTECT; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define a procedure protection :
**	    rdr_types_mask = RDR_PROTECT | RDR_PROCEDURE; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define a list of independent objects/privileges for a dbproc and/or
**	update db_mask1 describing its status (i.e. whether it is active,
**	grantable and whether there is an independent object/privilege list
**	asociated with it):
**	    rdr_types_mask = RDR_PROCEDURE;
**	    rdr_2types_mask = RDR2_DBP_STATUS;
**	    rdr_update_op = RDR_REPLACE;
**
**	To define an event protection :
**	    rdr_types_mask = RDR_PROTECT | RDR_EVENT; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define a rule:
**	    rdr_types_mask = RDR_RULE; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define a sequence:
**	    rdr_types_mask = RDR_SEQUENCE;
**	    rdr_update_op = RDR_APPEND;
**
**	To define an event:
**	    rdr_types_mask = RDR_EVENT; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define a comment (in iidbms_comment):
**	    rdr_types_mask = RDR_COMMENT;
**	    rdr_update_op = RDR_APPEND;
**
**	To define a synonym (in iisynonym):
**	    rdr_types_mask = 0;
**	    rdr_2types_mask = RDR_SYNONYM;
**	    rdr_update_op = RDR_APPEND;
**
**	To destroy an integrity :
**	    rdr_types_mask = RDR_INTEGRITIES; 
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy a rule:
**	    rdr_types_mask = RDR_RULE; 
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy a sequence:
**	    rdr_types_mask = RDR_SEQUENCE;
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy an event:
**	    rdr_types_mask = RDR_EVENT; 
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy a procedure :
**	    rdr_types_mask = RDR_PROCEDURE; 
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy a protection :
**	    rdr_types_mask = RDR_PROTECT; 
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy all protections :
**	    rdr_types_mask = RDR_PROTECT | RDR_DROP_ALL;
**	    rdr_update_op = RDR_DELETE;
**
**	To REVOKE privileges on tables/views:
**	    rdr_types_mask = RDR_PROTECT;
**	    rdr_2types_mask = RDR2_REVOKE;
**	    rdr_update_op = RDR_DELETE;
**	    - for CASCADEd revocation, rdr_2types_mask |= RDR2_DROP_CASCADE
**	    - to revoke ALL [PRIVILEGES],  rdr_types_mask |= RDR_REVOKE_ALL
**
**	To destroy a security_alarm :
**	    rdr_types_mask = RDR_SECALM;
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy all security_alarms :
**	    rdr_types_mask = RDR_SECALM | RDR_DROP_ALL;
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy a procedure protection :
**	    rdr_types_mask = RDR_PROTECT | RDR_PROCEDURE; 
**	    rdr_update_op = RDR_DELETE;
**          - the procedure "rdfcb->rdf_rb.rdr_tabid" must be specified
**	    - rdfcb->rdf_rb.rdr_qrymod_id must be specified
**
**	To destroy all procedure protections :
**	    rdr_types_mask = RDR_PROTECT | RDR_PROCEDURE | RDR_DROP_ALL; 
**	    rdr_update_op = RDR_DELETE;
**          - the procedure "rdfcb->rdf_rb.rdr_tabid" must be specified
**
**	To REVOKE privileges on dbprocs:
**	    rdr_types_mask = RDR_PROTECT | RDR_PROCEDURE;
**	    rdr_2types_mask = RDR2_REVOKE;
**	    rdr_update_op = RDR_DELETE;
**	    - the procedure "rdfcb->rdf_rb.rdr_tabid" must be specified
**	    - for CASCADEd revocation, rdr_2types_mask |= RDR2_DROP_CASCADE
**	    - to revoke ALL [PRIVILEGES],  rdr_types_mask |= RDR_REVOKE_ALL
**
**	To destroy an event protection :
**	    rdr_types_mask = RDR_PROTECT | RDR_EVENT;
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy all event protections :
**	    rdr_types_mask = RDR_PROTECT | RDR_EVENT | RDR_DROP_ALL;
**	    rdr_update_op = RDR_DELETE;
**
**	To REVOKE privileges on dbevents:
**	    rdr_types_mask = RDR_PROTECT | RDR_EVENT;
**	    rdr_2types_mask = RDR2_REVOKE;
**	    rdr_update_op = RDR_DELETE;
**	    - for CASCADEd revocation, rdr_2types_mask |= RDR2_DROP_CASCADE
**	    - to revoke ALL [PRIVILEGES],  rdr_types_mask |= RDR_REVOKE_ALL
**
**	To destroy a view :
**	    rdr_types_mask = RDR_VIEW; 
**	    rdr_update_op = RDR_DELETE;
**
**	To define a group or add group members:
**	    rdr_types_mask = RDR_GROUP; 
**	    rdr_update_op = RDR_APPEND;
**
**	To define an application identifier:
**	    rdr_types_mask = RDR_APLID; 
**	    rdr_update_op = RDR_APPEND;
**
**	To destroy a group or drop group members:
**	    rdr_types_mask = RDR_GROUP; 
**	    rdr_update_op = RDR_DELETE;
**
**	To destroy an application identifier:
**	    rdr_types_mask = RDR_APLID;
**	    rdr_update_op = RDR_DELETE;
**
**	To drop all members of a group:
**	    rdr_types_mask = RDR_GROUP; 
**	    rdr_update_op = RDR_PURGE;
**
**	To update an application identifier:
**	    rdr_types_mask = RDR_APLID;
**	    rdr_update_op = RDR_REPLACE;
**
**	To destroy a comment (in iidbms_comment):
**	    rdr_types_mask = RDR_COMMENT;
**	    rdr_update_op = RDR_DELETE;
**	    NOTE:  this is NOT supported at this time because
**		   it is only possible to add comment tuples at this time.
**		   (adding a comment tuple of NULL long and short remarks 
**		    will cause QEF to remove the tuple from the catalog.)
**
**	To destroy a synonym:
**	    rdr_types_mask = 0;
**	    rdr_2types_mask = RDR2_SYNONYM;
**	    rdr_update_op = RDR_DELETE;
**
**	To drop a schema:
**	    rdr_types_mask = 0;
**	    rdr_2types_mask = RDR2_SCHEMA
**	    rdr_update_op = RDR_DELETE;
**	    for CASCADEd destruction, rdr_2types_mask |= RDR2_DROP_CASCADE
**
** Inputs:
**
**      rdf_cb                          
**		.rdf_rb
**			.rdr_db_id	Database id.
**			.rdr_session_id Session id.
**			.rdr_tabid	Table id.
**			.rdr_types_mask	Type of information requested.
**					The mask can be:
**					RDR_PROCEDURE
**					RDR_INTEGRITIES
**					RDR_PROTECT
**					RDR_DROP_ALL
**					RDR_REVOKE_ALL
**					RDR_VIEW
**					RDR_GROUP
**					RDR_APLID
**					RDR_RULE
**                                      RDR_USER
**                                      RDR_LOCATION
**                                      RDR_SECSTATE
**					RDR_EVENT
**					RDR_COMMENT
**			.rdr_2types_mask    Type of information requested.
**					RDR_SYNONYM
**					RDR2_DBP_STATUS
**					RDR2_REVOKE
**					RDR2_DROP_CASCADE
**					RDR2_SCHEMA
**					RDR2_PROFILE
**				        RDR2_ROLEGRANT
**					RDR2_SEQUENCE
**			.rdr_update_op	PSF uses this field for specifying
**					what operation needed to be performed.
**					The operation can be
**					RDR_APPEND
**					RDR_DELETE
**					RDR_REPLACE
**					RDR_PURGE
**
**			.rdr_qry_root_node The root node of the query tree 
**					that defines the view, integrity,
**					rule or protect.
**			.rdr_qrytuple	Pointer to the qrymod tuple to delete,
**					append or replace.
**			.rdr_qrymod_id	Protection or integrity number that
**					that must be destroyed. 
**					0 means destroy all or 	integrities on
**					a table.
**			.rdr_l_querytext The length of the query text for
**					 defining a view, protection,
**					 integrity, rule, event or procedure
**			.rdr_querytext	The query text for defining a view,
**					protection, integrity, rule, event
**					or procedure
**			.rdr_qtext_id	query text id for destroying a view.
**			.rdr_dmu_cb	A pointer to the DMU_CB for creating 
**					a view.
**			.rdr_cnt_base_id Count of the dependent table ids for
**					 defining qrymod object.
**			.rdr_base_id	An array of dependent table ids 
**			.rdr_status	status word for qrytext
**			.rdr_fcb	A pointer to the internal structure
**					which contains the global memory
**					allocation and caching control
**					information of a facility.
**
** Outputs:
**      rdf_cb                          
**	
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0001_NO_MORE_MEM
**					Out of memory.
**				E_RD0006_MEM_CORRUPT
**					Memory pool is corrupted.
**				E_RD0007_MEM_NOT_OWNED
**					Memory pool is not owned by the calling
**					facility.
**				E_RD0008_MEM_SEMWAIT
**					Error waiting for exclusive access of 
**					memory.
**				E_RD0009_MEM_SEMRELEASE
**					Error releasing exclusive access of 
**					memory.
**				E_RD000A_MEM_NOT_FREE
**					Error freeing memory.
**				E_RD000B_MEM_ERROR
**					Whatever memory error not mentioned
**					above.
**				E_RD000D_USER_ABORT
**					User abort.
**				E_RD0002_UNKNOWN_TBL
**					Table doesn't exist.
**				E_RD0014_BAD_QUERYTEXT_ID
**					Bad query text id.
**				E_RD0010_QEF_ERROR
**					Whatever error returned from QEF not
**					mentioned above.
**				E_RD0003_BAD_PARAMETER
**					Bad input parameters.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**				E_RD013D_CREATE_COMMENT
**					Error creating comment text tuple.
**				E_RD0143_CREATE_SYNONYM 
**					error creating an iisynonym tuple
**				E_RD0144_DROP_SYNONYM 
**					error destroying an iisynonym tuple
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
**		none
** Side Effects:
**		none
** History:
**      15-Aug-86 (ac)
**          written
**      11-nov-87 (seputis)
**          rewritten for resource handling
**      17-aug-88 (seputis)
**	    modified for grant protections on procedures
**	12-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added support for maintaining iiusergroup & iiapplication_id
**	    catalogs.
**	19-mar-89 (neil)
**	    modified to support appending and deleting rules.
**	13-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added support for maintaining the iidbpriv catalog.
**      17-jul-89 (jennifer)
**          Added support for maintaining iiuser, iilocation, iisecuritystate,
**          iidbaccess.
**	25-jul-89 (neil)
**	    Fix bug where qrymod_id was checked for without checking if
**	    we're really deleting permits or integrities.
**	21-sep-89 (ralph)
**	    Fix support for maintaining iiuser (ALTER/DROP USER).
**	    Fix support for maintaining iilocations (ALTER/DROP LOCATION).
**	    Remove support for maintaining iidbaccess.
**	25-oct-89 (neil)
**	    Alerters - Support for CREATE/DROP EVENT and EVENT permissions.
**      15-dec-89 (teg)
**          add logic to put ptr to RDF Server CB into GLOBAL for subordinate
**          routines to use.
**	16-jan-90 (ralph)
**	    Add support for dropping security alarms.
**	    Pass in dump dbp tuple when dropping dbp permits.
**	01-Feb-90 (teg)
**	    Added support for creation of iidbms_comment tuples (containing
**	    table or column comments).
**	28-feb-90 (carol)
**	    Changed references to rdr_2types_mask to rdr_types_mask for
**	    comments.
**	17-apr-90 (teg)
**	    added logic to suppress E_QE0015 for drop permit and drop
**	    integrity if user specifies invalid permit/integrity number.
**	    This is for bug 4390.
**	23-apr-90 (teg)
**	    addded support for creation and deletion of ii_synonym tuples for
**	    synonyms project.
**	21-may-90 (teg)
**	    changed RDF to use rdr_rb->rdr_instr for RDF_VGRANT_OK and
**	    RDF_DBA_VIEW instead of using rdr_rb->rdr_status.
**	01-feb-91 (teresa)
**          Add update statistics reporting.
**	10-jul-91 (andre)
**	    if multiple IIPROTECT tuples should be inserted to represent a
**	    permit, notify qeu_cprot() by setting QEU_SPLIT_PERM in
**	    qeuq_cb.qeuq_permit_mask
**	17-jan-92 (andre)
**	    when called to create a new view, rdf_cb->rdf_rb.rdr_indep must be
**	    non-NULL.  pass it to QEF.
**	01-jun-92 (andre)
**	    when called to create a new database procedure,
**	    rdf_cb->rdf_rb.rdr_indep must be non-NULL.  pass it to QEF.
**	07-jul-92 (andre)
**	    when called to create new privilege(s), rdf_cb->rdf_rb.rdr_indep
**	    must be non-NULL.  pass it to QEF.
**	16-jul-92 (teresa)
**	    implement prototypes
**	22-jul-92 (andre)
**	    will no longer provide special handling for ALL/RETRIEVE TO ALL
**	    permits, since they will no longer be created
**	02-aug-92 (andre)
**	    added support for REVOKE
**	29-sep-92 (andre)
**	    renamed RDR_2* masks over rdr_2types_mask to RDR2_*
**	13-jan-93 (andre)
**	    CREATE/DEFINE VIEW processing will no longer go through
**	    rdf_update() - remove newly obsolete code
**	19-feb-93 (andre)
**	    added support for DROP SCHEMA
**	26-aug-93 (robf)
**	    Add support for RDR2_PROFILE, user profiles
**	10-sep-93 (andre)
**	    when processing DROP RULE, set QEU_FORCE_QP_INVALIDATION 
**	    in qeuq_cb.qeuq_flag_mask.  This will serve to remind qeu_drule()
**	    that if the rule was defined on a view, timestamp of some base table
**	    on which the view depends must be altered to force invalidation of
**	    QPs dependent on this view
**	13-sep-93 (andre)
**	    when processing DROP PERMIT/SECURITY_ALARM, set 
** 	    QEU_FORCE_QP_INVALIDATION in qeuq_cb.qeuq_flag_mask.  This will 
**	    serve to remind qeu_dprot() to change timestamp of 
**	      - the table permit or security_alarm on which is being dropped 
**		(if permit/security_alarm on a base table is being dropped), 
**	      - base table used in the definition of a view (if 
**		permit/security_alarm on a view is being dropped)
**	       - base table used in the definition of a dbproc (if permit 
**		on a dbproc is being dropped)
**
**	    Similarly, when processing DROP SYNONYM, set 
**	    QEU_FORCE_QP_INVALIDATION in qeuq_cb.qeuq_flag_mask.
**	27-sep-93 (stephenb)
**	    initialized qeuq_cb.qeuq_tabname in the case of CREATE/DROP
**	    INTEGRITY, so that we can write an audit record in QEF.
**	4-mar-94 (robf)
**	    Add support for RDR2_ROLEGRANT operation
**	7-mar-02 (inkdo01)
**	    Add support for sequences.
**	13-may-02 (inkdo01)
**	    Add privileges for sequences.
**	21-mar-03 (inkdo01)
**	    Add revoke support for sequence.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Prototype fixes.
*/
DB_STATUS
rdf_update( RDF_GLOBAL	    *global,
	    RDF_CB	    *rdfcb)
{
	DB_STATUS	status;
	QEUQ_CB		qeuq_cb;
	QEF_DATA	*qdata_p;
	QEF_DATA	qef_qrydata;	/* pass tuples to QEF via this
					** structure */
	QEF_DATA	qef1_qrydata;	/* pass extra tuples of a different
					** format to QEF */
	DB_ERROR	error;		/* RDF error to report in case a
					** QEF error occurs */
	bool		bad_parm;	/* TRUE if caller has inconsistent
					** parameters */
	i4		operation;
	DB_PROCEDURE	temp_proc;	/* procedures are referenced
					** by name and owner instead of ID */
	DB_IIEVENT	temp_ev;	/* Temp tuple for protections */
	DB_IISEQUENCE	temp_seq;	/* Temp for protections */
	RDF_ERROR	suppress;	/* use this to specify a specific
					** external facility returned error
					** message from being printed to the
					** user terminal */

    /* set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */
    global->rdfcb = rdfcb;
    suppress = 0;		    /* init to NOT suppressing any err msgs */
    CLRDBERR(&rdfcb->rdf_error);
    bad_parm = FALSE;		    /* set TRUE if a parameter consistency
				    ** check error occurs */
    qeuq_cb.qeuq_flag_mask = 0;
    qeuq_cb.qeuq_permit_mask = 0;
    qeuq_cb.qeuq_d_id = global->rdf_sess_id;

    if 
    (
	rdfcb->rdf_rb.rdr_db_id == NULL 
	||
	rdfcb->rdf_rb.rdr_session_id == (CS_SID) 0
	||
	(((rdfcb->rdf_rb.rdr_types_mask & 
		(RDR_PROCEDURE | RDR_INTEGRITIES | RDR_PROTECT | RDR_VIEW | 
		 RDR_GROUP | RDR_APLID | RDR_RULE | RDR_DBPRIV | RDR_USER |
		 RDR_LOCATION | RDR_SECSTATE | RDR_SECALM | RDR_EVENT |
		 RDR_COMMENT)
	  ) == 0L) &&
	  ((rdfcb->rdf_rb.rdr_2types_mask & 
		(RDR2_SYNONYM | RDR2_SCHEMA | RDR2_PROFILE |
		 RDR2_ROLEGRANT | RDR2_SEQUENCE)
	  ) == 0L) 
	)
	|| 
	(
	    (rdfcb->rdf_rb.rdr_types_mask &
		(RDR_INTEGRITIES | RDR_PROTECT) )
	    && 
	    (rdfcb->rdf_rb.rdr_tabid.db_tab_base == 0)
	) 
	||
	(   rdfcb->rdf_rb.rdr_types_mask & RDR_DROP_ALL
	 && (   !(rdfcb->rdf_rb.rdr_types_mask & (RDR_PROTECT | RDR_SECALM))
	     || rdfcb->rdf_rb.rdr_update_op != RDR_DELETE
	    )
	)
	||
	(   rdfcb->rdf_rb.rdr_types_mask & RDR_REVOKE_ALL
	 && ~rdfcb->rdf_rb.rdr_2types_mask & RDR2_REVOKE
	)
	||
	(   rdfcb->rdf_rb.rdr_2types_mask & RDR2_REVOKE
	 && (   ~rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT
	     || rdfcb->rdf_rb.rdr_update_op != RDR_DELETE
	     || rdfcb->rdf_rb.rdr_types_mask &
		    ~(RDR_PROTECT | RDR_REVOKE_ALL | RDR_EVENT | RDR_PROCEDURE)
	     || rdfcb->rdf_rb.rdr_2types_mask &
		    ~(RDR2_REVOKE | RDR2_DROP_CASCADE | RDR2_SEQUENCE)
	    )
	)
	||
	(   rdfcb->rdf_rb.rdr_2types_mask & RDR2_SCHEMA
	 && (   rdfcb->rdf_rb.rdr_update_op != RDR_DELETE
	     || rdfcb->rdf_rb.rdr_2types_mask &
		    ~(RDR2_SCHEMA | RDR2_DROP_CASCADE)
	    )
	)
	|| 
	rdfcb->rdf_rb.rdr_fcb == NULL
    )
	bad_parm = TRUE;

    else switch (rdfcb->rdf_rb.rdr_update_op)
    {	/* this switch statement will initialize the parameters to the
        ** qeuq_cb structure which will be used for a QEF call */

    case RDR_APPEND:
    {
	/* Set up qef_call input parameters. */
	if (rdfcb->rdf_rb.rdr_types_mask & RDR_USER)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_USER);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_CUSER;
	    error.err_code = E_RD0168_AUSER;
	    Rdi_svcb->rdvstat.rds_a10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_PROFILE)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_PROFILE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_CPROFILE;
	    error.err_code = E_RD0182_APROFILE; 
	    Rdi_svcb->rdvstat.rds_a10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_LOCATION)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_LOCATIONS);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_CLOC;
	    error.err_code = E_RD016B_ALOCATION;
	    Rdi_svcb->rdvstat.rds_a10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_GROUP)
	{
	    qeuq_cb.qeuq_cagid = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_USERGROUP);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_CGROUP;
	    operation = QEU_GROUP;
	    error.err_code = E_RD0160_AGROUP;
	    Rdi_svcb->rdvstat.rds_a10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_APLID)
	{
	    qeuq_cb.qeuq_cagid = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_APPLICATION_ID);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_CAPLID;
	    operation = QEU_APLID;
	    error.err_code = E_RD0161_AAPLID;
	    Rdi_svcb->rdvstat.rds_a10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_DBPRIV)
	{
	    qeuq_cb.qeuq_cdbpriv = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_PRIVILEGES);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_dbpr_tup = &qef_qrydata;
	    qeuq_cb.qeuq_dbpr_mask = QEU_GDBPRIV;
	    operation = QEU_DBPRIV;
	    error.err_code = E_RD0166_GDBPRIV;
	    Rdi_svcb->rdvstat.rds_a10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_ROLEGRANT)
	{
	    qeuq_cb.qeuq_cdbpriv = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_ROLEGRANT);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_GROLE;
	    operation = QEU_ROLEGRANT;
	    error.err_code = E_RD0188_AROLEGRANT;
	    Rdi_svcb->rdvstat.rds_a10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_COMMENT)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_1_IICOMMENT);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_CCOMMENT;
	    error.err_code = E_RD013D_CREATE_COMMENT;
	    Rdi_svcb->rdvstat.rds_a17_comment++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SYNONYM)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IISYNONYM);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_CSYNONYM;
	    error.err_code = E_RD0143_CREATE_SYNONYM;
	    Rdi_svcb->rdvstat.rds_a16_syn++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE &&
	    !(rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT))
	{
	    /* Create sequence - but no privilege definitions. */
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IISEQUENCE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_CSEQ;
	    error.err_code = E_RD0155_CREATE_SEQUENCE;
	}
	else if ((!rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE &&
	    (rdfcb->rdf_rb.rdr_l_querytext == 0 ||
	    rdfcb->rdf_rb.rdr_querytext == NULL)) ||
	    (rdfcb->rdf_rb.rdr_qry_root_node == NULL &&
	     (rdfcb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
	    )
	    ||
	    ((rdfcb->rdf_rb.rdr_types_mask &	
		(RDR_INTEGRITIES | RDR_PROTECT | RDR_RULE |
		 RDR_GROUP | RDR_APLID | RDR_EVENT) ||
	      rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE)
		&& rdfcb->rdf_rb.rdr_qrytuple == NULL)
	    ||
	    (	/*
		** when creating permits, we must be given a structure
		** decribing privileges on which new privileges depend
	        */
	        rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT
	     && rdfcb->rdf_rb.rdr_indep == NULL
	    )
	    ||
	    (
	        (rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
	     && (
	            /* procedures do not have query trees to be stored */
	            rdfcb->rdf_rb.rdr_qry_root_node
		 ||
		    /*
		    ** for a new dbproc or if updating status of existing
		    ** dbproc, we must be given a structure decribing
		    ** objects/privileges on which it depends
		    */
		    rdfcb->rdf_rb.rdr_indep == NULL
		)
	    )
	   )
	    bad_parm = TRUE;
	else
	{
	    i4		mode;
	    /* 
	    ** Open a temporary stream for storing the tree tuples and
	    ** iiqrytext tuples. The memory can be reclaimed after the 
	    ** append operation is done.
	    */
	    status = rdu_cstream(global);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT) /* make sure
				    ** this is before the test for RDR_PROCEDURE
				    **  and RDR_EVENT since both can be set,
				    ** this assumes an object has been defined
				    ** prior to defining a protection on it */
	    {
		mode = DB_PROT;
		Rdi_svcb->rdvstat.rds_a2_permit++;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_SECALM)
	    {
		mode = DB_ALM;
		Rdi_svcb->rdvstat.rds_a20_alarm++;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
	    {
		mode = DB_INTG;
		Rdi_svcb->rdvstat.rds_a3_integ++;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
	    {
		mode = DB_DBP;
		Rdi_svcb->rdvstat.rds_a5_proc++;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_RULE)
	    {
		mode = DB_RULE;
		Rdi_svcb->rdvstat.rds_a6_rule++;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_EVENT)
	    {
		mode = DB_EVENT;
		Rdi_svcb->rdvstat.rds_a7_event++;
	    }
	    else
	    {
		bad_parm = TRUE;
		break;
	    }

	    {
		RDD_QDATA	qef_iiqrydata;

		/* Get the iiqrytext tuples. */
		status = rdu_qrytext_to_tuple( global, mode, &qef_iiqrydata);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		qeuq_cb.qeuq_qry_tup = qef_iiqrydata.qrym_data;
		qeuq_cb.qeuq_cq = qef_iiqrydata.qrym_cnt;
	    }

	    /* Get the tree tuples. */

	    if (rdfcb->rdf_rb.rdr_qry_root_node)
	    {
		RDD_QDATA	qef_treedata;

		status = rdu_treenode_to_tuple( global,
			rdfcb->rdf_rb.rdr_qry_root_node, 
			&rdfcb->rdf_rb.rdr_tabid,
			mode, 
			&qef_treedata);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		qeuq_cb.qeuq_tre_tup	= qef_treedata.qrym_data;
		qeuq_cb.qeuq_ct		= qef_treedata.qrym_cnt;
	    }
	    else
	    {
		qeuq_cb.qeuq_tre_tup	= 0;
		qeuq_cb.qeuq_ct		= 0;
	    }

	    if (rdfcb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
	    {
		/* Create the integrity definition. */

		qeuq_cb.qeuq_ci = 1;
		qef_qrydata.dt_next = NULL;
		qeuq_cb.qeuq_tabname = rdfcb->rdf_rb.rdr_name.rdr_tabname;
		qef_qrydata.dt_size = sizeof(DB_INTEGRITY);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
		qeuq_cb.qeuq_int_tup = &qef_qrydata;
		operation = QEU_CINTG;
		error.err_code = E_RD011F_DINTEGRITY;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT)
	    {
		qeuq_cb.qeuq_permit_mask = QEU_PERM_OR_SECALM;

		/* Create the protection definition. */
		qeuq_cb.qeuq_cp = 1;
		qef_qrydata.dt_next = NULL;
		qef_qrydata.dt_size = sizeof(DB_PROTECTION);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
		qeuq_cb.qeuq_pro_tup = &qef_qrydata;
		operation = QEU_CPROT;
		error.err_code = E_RD0120_DPERMIT;

		/*
		** pass the structure describing privileges on which new
		** privileges depend
		*/
		qeuq_cb.qeuq_indep = rdfcb->rdf_rb.rdr_indep;

		if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
		{
		    qef1_qrydata.dt_next = NULL;
		    qef1_qrydata.dt_size = sizeof(DB_PROCEDURE);
		    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_prcname,
			temp_proc.db_dbpname);
		    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
			temp_proc.db_owner);
		    qef1_qrydata.dt_data = (PTR)&temp_proc;
		    qeuq_cb.qeuq_dbp_tup = &qef1_qrydata;
		    qeuq_cb.qeuq_permit_mask |= QEU_DBP_PROTECTION;
		}
		else if (rdfcb->rdf_rb.rdr_types_mask & RDR_EVENT)
		{
		    qef1_qrydata.dt_next = NULL;
		    qef1_qrydata.dt_size = sizeof(DB_IIEVENT);
		    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_evname,
					temp_ev.dbe_name);
		    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
					temp_ev.dbe_owner);
		    qef1_qrydata.dt_data = (PTR)&temp_ev;
		    qeuq_cb.qeuq_culd = 1;
		    qeuq_cb.qeuq_uld_tup = &qef1_qrydata;
		    qeuq_cb.qeuq_permit_mask |= QEU_EV_PROTECTION;
		}
		else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE)
		{
		    qef1_qrydata.dt_next = NULL;
		    qef1_qrydata.dt_size = sizeof(DB_IISEQUENCE);
		    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_seqname,
					temp_seq.dbs_name);
		    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
					temp_seq.dbs_owner);
		    qef1_qrydata.dt_data = (PTR)&temp_seq;
		    qeuq_cb.qeuq_culd = 1;
		    qeuq_cb.qeuq_uld_tup = &qef1_qrydata;
		    qeuq_cb.qeuq_permit_mask |= QEU_SEQ_PROTECTION;
		}

		if (rdfcb->rdf_rb.rdr_instr & RDF_SPLIT_PERM)
		{
		    /*
		    ** qeu_cprot() will have to build multiple IIPROTECT tuples
		    ** to represent this permit
		    */
		    qeuq_cb.qeuq_permit_mask |= QEU_SPLIT_PERM;
		}
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_SECALM)
	    {
		qeuq_cb.qeuq_permit_mask = QEU_PERM_OR_SECALM;

		/* Create the alarm definition. */
		qef_qrydata.dt_next = NULL;
		qef_qrydata.dt_size = sizeof(DB_SECALARM);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
		qeuq_cb.qeuq_uld_tup = &qef_qrydata;
		operation = QEU_CSECALM;
		error.err_code = E_RD0185_ASECALARM;

	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
	    {
		qef_qrydata.dt_next = NULL;
		qef_qrydata.dt_size = sizeof(DB_PROCEDURE);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;

		/*
		** pass the structure describing objects and privileges on which
		** the dbproc depends to QEF
		*/
		qeuq_cb.qeuq_indep = rdfcb->rdf_rb.rdr_indep;

#ifdef QEU_CREDBP
		qeuq_cb.qeuq_dbp_tup = &qef_qrydata;
		qeuq_cb.qeuq_uld_tup = rdfcb->rdf_rb.rdr_proc_params;
		qeuq_cb.qeuq_culd = rdfcb->rdf_rb.rdr_proc_param_cnt;
		operation = QEU_CREDBP;
#endif
		error.err_code = E_RD0135_CREATE_PROCEDURE;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_RULE)
	    {
		qeuq_cb.qeuq_cr = 1;
		qef_qrydata.dt_next = NULL;
		qef_qrydata.dt_size = sizeof(DB_IIRULE);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
		qeuq_cb.qeuq_rul_tup = &qef_qrydata;
		operation = QEU_CRULE;
		error.err_code = E_RD0150_CREATE_RULE;
	    }
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_EVENT)
	    {
		qeuq_cb.qeuq_culd = 1;
		qef_qrydata.dt_next = NULL;
		qef_qrydata.dt_size = sizeof(DB_IIEVENT);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
		qeuq_cb.qeuq_uld_tup = &qef_qrydata;
		operation = QEU_CEVENT;
		error.err_code = E_RD0152_CREATE_EVENT;
	    }
	    else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE)
	    {
		Rdi_svcb->rdvstat.rds_a21_seqs++;
		qeuq_cb.qeuq_culd = 1;
		qef_qrydata.dt_next = NULL;
		qef_qrydata.dt_size = sizeof(DB_IISEQUENCE);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
		qeuq_cb.qeuq_uld_tup = &qef_qrydata;
		operation = QEU_CSEQ;
		error.err_code = E_RD0155_CREATE_SEQUENCE;
	    }
	    else
		bad_parm = TRUE;	/* a command should have been processed
					** by the time this point is reached */
	}
	break;
    }
    case RDR_DELETE:

	/* Set up qef_call input parameters. */

	if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
	{
	    qef1_qrydata.dt_next = NULL;
	    qef1_qrydata.dt_size = sizeof(DB_PROCEDURE);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_prcname,
			temp_proc.db_dbpname);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
			temp_proc.db_owner);
	    qef1_qrydata.dt_data = (PTR)&temp_proc;
	    qeuq_cb.qeuq_dbp_tup = &qef1_qrydata;
	    qeuq_cb.qeuq_permit_mask |= QEU_DBP_PROTECTION;
	    Rdi_svcb->rdvstat.rds_d5_proc++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE)
	{
	    qef1_qrydata.dt_next = NULL;
	    qef1_qrydata.dt_size = sizeof(DB_IISEQUENCE);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_seqname,
				temp_seq.dbs_name);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
				temp_seq.dbs_owner);
	    qef1_qrydata.dt_data = (PTR)&temp_seq;
	    qeuq_cb.qeuq_uld_tup = &qef1_qrydata;
	    qeuq_cb.qeuq_culd = 1;	/* One tuple fom REVOKE ... SEQUENCE */
	    qeuq_cb.qeuq_permit_mask |= QEU_SEQ_PROTECTION;
	    Rdi_svcb->rdvstat.rds_d21_seqs++;
	}

	if (rdfcb->rdf_rb.rdr_types_mask & RDR_USER)
	{
	    qeuq_cb.qeuq_culd= 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_USER);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_DUSER;
	    error.err_code = E_RD0169_DUSER;
	    Rdi_svcb->rdvstat.rds_d10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_PROFILE)
	{
	    qeuq_cb.qeuq_culd= 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_PROFILE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_DPROFILE;
	    error.err_code = E_RD0183_DPROFILE; 
	    Rdi_svcb->rdvstat.rds_d10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_LOCATION)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_LOCATIONS);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_DLOC;
	    error.err_code = E_RD016C_DLOCATION;
	    Rdi_svcb->rdvstat.rds_d10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_GROUP)
	{
	    qeuq_cb.qeuq_cagid = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_USERGROUP);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_DGROUP;
	    operation = QEU_GROUP;
	    error.err_code = E_RD0162_DGROUP;
	    Rdi_svcb->rdvstat.rds_d10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_APLID)
	{
	    qeuq_cb.qeuq_cagid = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_APPLICATION_ID);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_DAPLID;
	    operation = QEU_APLID;
	    error.err_code = E_RD0163_DAPLID;
	    Rdi_svcb->rdvstat.rds_d10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_DBPRIV)
	{
	    qeuq_cb.qeuq_cdbpriv = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_PRIVILEGES);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_dbpr_tup = &qef_qrydata;
	    qeuq_cb.qeuq_dbpr_mask = QEU_RDBPRIV;
	    operation = QEU_DBPRIV;
	    error.err_code = E_RD0167_RDBPRIV;
	    Rdi_svcb->rdvstat.rds_d10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_ROLEGRANT)
	{
	    qeuq_cb.qeuq_cdbpriv = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_ROLEGRANT);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_RROLE;
	    operation = QEU_ROLEGRANT;
	    error.err_code = E_RD0189_DROLEGRANT;
	    Rdi_svcb->rdvstat.rds_d10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_VIEW)
	{
	    if (rdfcb->rdf_rb.rdr_qtext_id.db_qry_high_time == 0 ||
		rdfcb->rdf_rb.rdr_qtext_id.db_qry_low_time == 0)
	    {
		bad_parm = TRUE;
		break;
	    }			    
	STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_qtext_id, qeuq_cb.qeuq_qid);
	    operation = QEU_DVIEW;
	    error.err_code = E_RD0121_DESTROY_VIEW;
	    Rdi_svcb->rdvstat.rds_d4_view++;

	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT)
	{
	    if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_REVOKE)
	    {
		qef_qrydata.dt_next = NULL;
		qef_qrydata.dt_size = sizeof(DB_PROTECTION);
		qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
		qeuq_cb.qeuq_pro_tup = &qef_qrydata;
		qeuq_cb.qeuq_v2b_col_xlate = rdfcb->rdf_rb.rdr_v2b_col_xlate;
		qeuq_cb.qeuq_b2v_col_xlate = rdfcb->rdf_rb.rdr_b2v_col_xlate;

		operation = QEU_REVOKE;
		qeuq_cb.qeuq_permit_mask |= QEU_REVOKE_PRIVILEGES;
		if (rdfcb->rdf_rb.rdr_types_mask & RDR_REVOKE_ALL)
		    qeuq_cb.qeuq_permit_mask |= QEU_REVOKE_ALL;
		if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_DROP_CASCADE)
		    qeuq_cb.qeuq_flag_mask |= QEU_DROP_CASCADE;
	    }
	    else
	    {
		qeuq_cb.qeuq_permit_mask |= QEU_PERM;
		operation = QEU_DPROT;
		qeuq_cb.qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;
		qeuq_cb.qeuq_v2b_col_xlate = qeuq_cb.qeuq_b2v_col_xlate = NULL;

		if (rdfcb->rdf_rb.rdr_types_mask & RDR_DROP_ALL)
		{
		    qeuq_cb.qeuq_permit_mask |= QEU_DROP_ALL;

		    /*
		    ** qeu_dprot() will not be looking at it, but it's nice to
		    ** have it initialized to a known value
		    */
		    qeuq_cb.qeuq_pno = 0;	
		}
		else if (rdfcb->rdf_rb.rdr_qrymod_id < 0)
		{
		    bad_parm = TRUE;
		    break;
		}
		else
		{
		    qeuq_cb.qeuq_pno = rdfcb->rdf_rb.rdr_qrymod_id;
		}
	    }
	    Rdi_svcb->rdvstat.rds_d2_permit++;
	    error.err_code = E_RD0122_DESTROY_PERMIT;
	    /* bug 4390 -- user may supply invalid protect numeric identifier.
	    **	in that case, qef will return E_QE0015_NO_MORE_ROWS.  That
	    **  message, and the corresponding E_RD0013 should be suppressed.
	    **  PSF will report the appropriate user error message. */
	    suppress = E_QE0015_NO_MORE_ROWS;	
	    if (rdfcb->rdf_rb.rdr_types_mask & RDR_EVENT)
	    {
		qef1_qrydata.dt_next = NULL;
		qef1_qrydata.dt_size = sizeof(DB_IIEVENT);
		STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_evname,
				    temp_ev.dbe_name);
		STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
				    temp_ev.dbe_owner);
		qef1_qrydata.dt_data = (PTR)&temp_ev;
		qeuq_cb.qeuq_culd = 1;
		qeuq_cb.qeuq_uld_tup = &qef1_qrydata;
		qeuq_cb.qeuq_permit_mask |= QEU_EV_PROTECTION;
	    }
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_SECALM)
	{

	    if (rdfcb->rdf_rb.rdr_types_mask & RDR_DROP_ALL)
	    {
		qeuq_cb.qeuq_permit_mask |= QEU_DROP_ALL;

		qeuq_cb.qeuq_pno = 0;	
	    }
	    
	    Rdi_svcb->rdvstat.rds_d20_alarm++;
	    operation = QEU_DSECALM;
	    error.err_code = E_RD0186_DSECALARM;
	    qeuq_cb.qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_SECALARM);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;

	    /* bug 4390 -- user may supply invalid protect/alarm numeric 
	    ** identifier.
	    **	in that case, qef will return E_QE0015_NO_MORE_ROWS.  That
	    **  message, and the corresponding E_RD0013 should be suppressed.
	    **  PSF will report the appropriate user error message. */
	    suppress = E_QE0015_NO_MORE_ROWS;	
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
	{
	    if (rdfcb->rdf_rb.rdr_qrymod_id < 0)
	    {
		bad_parm = TRUE;
		break;
	    }
	    Rdi_svcb->rdvstat.rds_d3_integ++;
	    operation = QEU_DINTG;
	    error.err_code = E_RD0124_DESTROY_INTEGRITY;
	    qeuq_cb.qeuq_tabname = rdfcb->rdf_rb.rdr_name.rdr_tabname;
	    qeuq_cb.qeuq_ino = rdfcb->rdf_rb.rdr_qrymod_id;
	    /* bug 4390 -- user may supply invalid protect numeric identifier.
	    **	in that case, qef will return E_QE0015_NO_MORE_ROWS.  That
	    **  message, and the corresponding E_RD0013 should be suppressed.
	    **  PSF will report the appropriate user error message. */
	    suppress = E_QE0015_NO_MORE_ROWS;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
	{   /* this needs to be checked after RDR_PROTECT is checked */
	    Rdi_svcb->rdvstat.rds_d5_proc++;
	    operation = QEU_DRPDBP;
	    error.err_code = E_RD0139_DROP_PROCEDURE;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_PROCEDURE);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_prcname,
		temp_proc.db_dbpname);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
		temp_proc.db_owner);
	    qef_qrydata.dt_data = (PTR)&temp_proc;
	    qeuq_cb.qeuq_dbp_tup = &qef_qrydata;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_RULE)
	{
	    Rdi_svcb->rdvstat.rds_d6_rule++;
	    qeuq_cb.qeuq_cr = 1;	/* One tuple fom DROP RULE */
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IIRULE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_rul_tup = &qef_qrydata;
	    qeuq_cb.qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;
	    operation = QEU_DRULE;
	    error.err_code = E_RD0151_DROP_RULE;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_EVENT)
	{
	    /* Check this after protections */
	    Rdi_svcb->rdvstat.rds_d7_event++;
	    qeuq_cb.qeuq_culd = 1;	/* One tuple fom DROP EVENT */
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IIEVENT);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_DEVENT;
	    error.err_code = E_RD0153_DROP_EVENT;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE)
	{
	    Rdi_svcb->rdvstat.rds_d21_seqs++;
	    qeuq_cb.qeuq_culd = 1;	/* One tuple fom DROP SEQUENCE */
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IISEQUENCE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_DSEQ;
	    error.err_code = E_RD0157_DROP_SEQUENCE;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_COMMENT)
	{
	    bad_parm = TRUE;	/* this is not supported yet, and may never be*/
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SYNONYM)
	{
	    Rdi_svcb->rdvstat.rds_d16_syn++;
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IISYNONYM);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    qeuq_cb.qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;
	    operation = QEU_DSYNONYM;
	    error.err_code = E_RD0144_DROP_SYNONYM;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SCHEMA)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IISCHEMA);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_DSCHEMA;

	    if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_DROP_CASCADE)
		qeuq_cb.qeuq_flag_mask |= QEU_DROP_CASCADE;

	    error.err_code = E_RD0154_DROP_SCHEMA;
	}
	else
	    bad_parm = TRUE;   /* unknown command */
	break;
    case RDR_REPLACE:
	if (rdfcb->rdf_rb.rdr_types_mask & RDR_SECSTATE)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_SECSTATE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    qeuq_cb.qeuq_sec_mask = QEU_SEC_NORM;
	    operation = QEU_MSEC;
	    error.err_code = E_RD016E_RSECSTATE;
	    Rdi_svcb->rdvstat.rds_o10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_APLID)
	{
	    qeuq_cb.qeuq_cagid = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_APPLICATION_ID);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_AAPLID;
	    operation = QEU_APLID;
	    error.err_code = E_RD0165_RAPLID;
	    Rdi_svcb->rdvstat.rds_o10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_USER)
	{
	    qeuq_cb.qeuq_culd= 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_USER);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_AUSER;
	    error.err_code = E_RD016A_RUSER; 
	    Rdi_svcb->rdvstat.rds_o10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_PROFILE)
	{
	    qeuq_cb.qeuq_culd= 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_PROFILE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_APROFILE;
	    error.err_code = E_RD0182_APROFILE; 
	    Rdi_svcb->rdvstat.rds_o10_dbdb++;
	}
	else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_SEQUENCE)
	{
	    qeuq_cb.qeuq_culd = 1;	/* One tuple fom ALTER SEQUENCE */
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_IISEQUENCE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_ASEQ;
	    error.err_code = E_RD0156_ALTER_SEQUENCE;
	}
	else if (rdfcb->rdf_rb.rdr_types_mask & RDR_LOCATION)
	{
	    qeuq_cb.qeuq_culd = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DU_LOCATIONS);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_uld_tup = &qef_qrydata;
	    operation = QEU_ALOC;
	    error.err_code = E_RD016D_RLOCATION;
	    Rdi_svcb->rdvstat.rds_o10_dbdb++;
	}
	else if (   rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE
		 && rdfcb->rdf_rb.rdr_2types_mask & RDR2_DBP_STATUS)
	{
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_PROCEDURE);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;

	    /*
	    ** pass the structure describing objects and privileges on which
	    ** the dbproc depends to QEF
	    */
	    qeuq_cb.qeuq_indep = rdfcb->rdf_rb.rdr_indep;

	    qeuq_cb.qeuq_dbp_tup = &qef_qrydata;
	    operation = QEU_DBP_STATUS;
	    error.err_code = E_RD016F_R_DBP_STATUS;
	}
	else
	    bad_parm = TRUE;
	break;
    case RDR_PURGE:
	if (rdfcb->rdf_rb.rdr_types_mask & RDR_GROUP)
	{
	    qeuq_cb.qeuq_cagid = 1;
	    qef_qrydata.dt_next = NULL;
	    qef_qrydata.dt_size = sizeof(DB_USERGROUP);
	    qef_qrydata.dt_data = rdfcb->rdf_rb.rdr_qrytuple;
	    qeuq_cb.qeuq_agid_tup = &qef_qrydata;
	    qeuq_cb.qeuq_agid_mask = QEU_PGROUP;
	    operation = QEU_GROUP;
	    error.err_code = E_RD0164_PGROUP;
	    Rdi_svcb->rdvstat.rds_o10_dbdb++;
	}
	else
	    bad_parm = TRUE;
	break;
    default:
	bad_parm = TRUE;
    }

    if (bad_parm)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
    }
    else
    {	/* execute the QEF update operation */
	qeuq_cb.qeuq_eflag  = QEF_EXTERNAL;
	qeuq_cb.qeuq_type   = QEUQCB_CB;
	qeuq_cb.qeuq_length = sizeof(QEUQ_CB);
	qeuq_cb.qeuq_tsz    = rdfcb->rdf_rb.rdr_cnt_base_id;
	qeuq_cb.qeuq_tbl_id = rdfcb->rdf_rb.rdr_base_id;
	qeuq_cb.qeuq_rtbl   = &rdfcb->rdf_rb.rdr_tabid;
	qeuq_cb.qeuq_d_id   = rdfcb->rdf_rb.rdr_session_id;
	qeuq_cb.qeuq_db_id  = rdfcb->rdf_rb.rdr_db_id;
	qeuq_cb.qeuq_obj_typ_p = rdfcb->rdf_rb.rdr_r4_obj_typ_p;
				    /* array of type of base objects */

	status = qef_call(operation, ( PTR ) &qeuq_cb);
	if (DB_FAILURE_MACRO(status))
	    rdu_ferror(global, status, &qeuq_cb.error, 
		error.err_code, suppress);
    }
    if (global->rdf_resources & RDF_RPRIVATE)
    {
	DB_STATUS	local_status;

	local_status = rdu_dstream(global, (PTR)NULL, (RDF_STATE *)NULL);
					    /* delete the private memory
					    ** stream if it was created, note
					    ** that resource cleanup will only
					    ** delete the stream if a bad
					    ** status was found */
	if (status == E_DB_OK)
	    status = local_status;	/* keep the status of the QEF call
					** in case of an error, since it will
					** indicate the success of the operation
					** and is more important - FIXME create
					** routine to always pass back the most
					** severe status of all the calls */
    }
    return(status);
}
