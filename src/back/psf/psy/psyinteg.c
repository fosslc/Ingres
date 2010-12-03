/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <qu.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSYINTEG.C - Integrity Constraint Processor
**
**  Description:
**	This module contains the integrity constraint processor.  This
**	processor modifies the query to add integrity constraints.
**
**	Currently only single-variable aggregate-free constraints are
**	handled.  Thus the algorithm is reduced to scanning the tree
**	for each variable modified and appending the constraints for
**	that variable to the tree.
**
**          psy_integ - Apply integrity constraints
**
**
**  History:
**      19-jun-86 (jeff)    
**          Adapted from integrity.c in 4.0
**	1-oct-86 (daved)
**	    centralize error handling, correct some useage of RDF. merge
**	    integrity range var with result range var of query.
**	03-dec-87 (stec)
**	    Change psy_apql interface.
**	06-feb-89 (ralph)
**	    Modified to use DB_COL_WORDS*2 as extent of dset array
**	23-Feb-89 (andre)
**	    Changed the way the tree consisting of qualifications obtained from
**	    the integrity trees is constructed and merged with the
**	    qualifications found in the original tree.
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	18-may-89 (neil)
**	    Use session memory for cursors with integrities (bug fix).
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-aug-92 (barbara)
**	    call pst_rdfcb_init() to initialize RDF_CB before call to RDF.
**	15-oct-92 (rblumer)
**	    Change dbi_domset to dbi_columns.db_dom_set.
**	30-nov-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	05-dec-92 (rblumer)
**	    ignore FIPS constraints during INGRES integrity processing
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	23-mar-93 (smc)
**	    Commented out text after ifdef.1
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time. Made sessid a CS_SID.
**	23-may-1997 (shero03)
**	    Save changed infoblk after UNFIX
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to pst_treedup().
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_integ(
	PSF_MSTREAM *mstream,
	PST_QNODE *root,
	PSS_USRRANGE *rngtab,
	PSS_RNGTAB *resvar,
	i4 qmode,
	PSS_SESBLK *sess_cb,
	PST_QNODE **result,
	DB_ERROR *err_blk);

/*{
** Name: psy_integ	- Apply integrity constraints
**
** Description:
**      This function applies integrity constraints.  It gets the constraints
**	from RDF and puts them in the query where appropriate.
**
** Inputs:
**      mstream                         QSF memory stream to allocate from
**	root				Root of query tree to constrain
**	rngtab				Pointer to the range table
**	resvar				Pointer to the result range variable
**	qmode				Query mode of user's query
**	sess_cb				session control block
**	result				Place to put pointer to constrained tree
**	err_blk				Filled in if an error happens
**
** Outputs:
**      root                            Integrity constraints may be appended
**	result				Filled in with pointer to constrained
**					tree
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	19-jun-86 (jeff)
**          Adapted from integrity.c in 4.0.
**      02-sep-86 (seputis)
**          changes for new RDF interface
**          fixed bug for no integrity case
**	04-dec-86 (daved)
**	    process define and replace cursor. Use copy of saved qual for
**	    replace cursor.
**	03-dec-87 (stec)
**	    Change psy_apql interface.
**	11-may-88 (stec)
**	    Make changes for db procs.
**	06-feb-89 (ralph)
**	    Modified to use DB_COL_WORDS*2 as extent of dset array
**	23-Feb-89 (andre)
**	    Changed the way the tree consisting of qualifications obtained from
**	    the integrity trees is constructed and merged with the
**	    qualifications found in the original tree.
**	18-may-89 (neil)
**	    Use session memory for cursors with integrities (bug fix).
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	04-sep-90 (andre)
**	    fixed bug 32976: for OPEN CURSOR (qmode==PSQ_DEFCURS) we need to
**			     compare attribute number(s) found in the integrity
**			     tuple with attribute number found in the VAR
**			     node(s) found in the portion of the target list
**			     which was built to represent the FOR UPDATE list
**			     (such node(s) are right children of RESDOM nodes
**			     with pst_rsupdt set to TRUE; note that for
**			     RESDOM nodes built to represent the real target
**			     list of SELECT, this field is set to FALSE.)
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	05-dec-92 (rblumer)
**	    ignore FIPS constraints during INGRES integrity processing
**	10-jan-93 (andre)
**	    after calling rdf_call() for IIINTEGRITY tuples, compare status to
**	    E_DB_OK instead of using DB_FAILURE_MACRO() since if there are fewer
**	    than 20 rows, rdf_call() sets err_code to E_RD0011 and status to
**	    E_DB_WARN
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	23-May-1997 (shero03)
**	    Save the rdr_info_blk after an UNFIX
**	22-Jul-2004 (schka24)
**	    Delete old ifdef'ed out normalization call
**	28-nov-2007 (dougi)
**	    Add PSQ_REPDYN to PSQ_DEFCURS test (cached dynamic).
*/
DB_STATUS
psy_integ(
	PSF_MSTREAM	*mstream,
	PST_QNODE	*root,
	PSS_USRRANGE	*rngtab,
	PSS_RNGTAB	*resvar,
	i4		qmode,
	PSS_SESBLK	*sess_cb,
	PST_QNODE	**result,
	DB_ERROR	*err_blk)
{
    register PST_QNODE	*r;
    PSC_CURBLK		*curblk = sess_cb->pss_crsr;
    i2			dset[DB_COL_WORDS*2];
    i2			doms;
    i2			*domset;
    bool		subset;
    PST_QNODE		*p;
    register i4	i;
    PST_QNODE		*iqual;
    PST_QNODE		**tmp1;
    DB_INTEGRITY	*inttup;
    i4		err_code;
    PST_QTREE		*qtree;
    PST_PROCEDURE	*pnode;
    DB_STATUS		status = E_DB_OK;
    i4			found;
    RDF_CB		rdf_cb;
    QEF_DATA		*qp;
    RDF_CB		rdf_tree_cb;
    PSS_DUPRB		dup_rb;
    i4			tupcount;
    i4			map[PST_NUMVARS];   /* Map for reassigning varnos */

    r = root;

    /* Initialize fields in dup_rb */
    dup_rb.pss_op_mask = 0;
    dup_rb.pss_num_joins = PST_NOJOIN;
    dup_rb.pss_tree_info = (i4 *) NULL;
    dup_rb.pss_mstream = mstream;
    dup_rb.pss_err_blk = err_blk;

    if (qmode == PSQ_REPCURS)
    {
	if (!curblk->psc_integ)
	{
	    *result = r;
	    return (E_DB_OK);
	}
	/*
	** On a replace cursor, get the query tree that was stored in the
	** cursor control block.
	*/
	/* copy the qual fragment so we can change below. */
	dup_rb.pss_tree = curblk->psc_integ;
	dup_rb.pss_dup  = &iqual;
	status = pst_treedup(sess_cb, &dup_rb);
	
	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}
    }
    else
    {
	register u_i4 u;
	/*
	**  Check to see if we should apply the integrity
	**  algorithm.
	**
	**  This means checking to insure that we have an update
	**  and seeing if any integrity constraints apply.
	*/
	if 
	(
	    resvar == 0 || 
	    (resvar->pss_tabdesc->tbl_status_mask & DMT_INTEGRITIES) == 0
	)
	{
	    *result = r;
	    return (E_DB_OK);
	}
	/*
	**  Create a set of the domains updated in this query.
	*/

	for (u = 0; u < DB_COL_WORDS*2; u++)
	    dset[u] = 0;

	for (p = r->pst_left, doms = 0;
	    p != (PST_QNODE *) NULL && p->pst_sym.pst_type != PST_TREE;
	    p = p->pst_left)
	{
	    if (p->pst_sym.pst_type != PST_RESDOM)
	    {
		psf_error(E_PS0D0C_NOT_RESDOM, 0L, PSF_INTERR, &err_code,
		    err_blk, 0);
		return (E_DB_SEVERE);
	    }

	    /*
	    ** if we are defining a cursor, RESDOM numbers are meaningless as
	    ** at best they reflect position of an attribute in the target list
	    ** of a subselect or, at worst, they are set to 1 for all columns
	    ** mentioned in the FOR UPDATE LIST.  We really are interested only
	    ** in the VAR nodes which are children of RESDOM nodes built to
	    ** repersent columns appearing in the FOR UPDATE list, so we will
	    ** skip over RESDOM nodes which represent a true subselect
	    ** (i.e. pst_rsupdt == FALSE)
	    */
	    if (qmode == PSQ_DEFCURS)
	    {
		if (p->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt)
		{
		    PST_QNODE	    *r_child = p->pst_right;

		    /*
		    ** this RESDOM was built to represent the column in the FOR
		    ** UPDATE list
		    */

		    /*
		    ** make sure the right child is a VAR node; otherwise flag
		    ** an error
		    */
		    if (r_child == (PST_QNODE *) NULL ||
		        r_child->pst_sym.pst_type != PST_VAR)
		    {
			psf_error(E_PS0C04_BAD_TREE, 0L, PSF_INTERR, &err_code,
			    err_blk, 0);
			return (E_DB_SEVERE);
		    }
		    
		    BTset((i4) r_child->
				 pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
			  (char *) dset);
		    ++doms;
		}
	    }
	    else
	    {
		BTset((i4) p->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
		    (char *) dset);
		++doms;
	    }
	}

	/*
	** Note if we are appending a subset of the relation's domains.
	** If we are, we'll need to be extra careful to avoid violating
	** constraints on those attributes not being explicitly appended:
	*/
	subset = ((doms < resvar->pss_tabdesc->tbl_attr_count)
	    && (qmode == PSQ_APPEND));

	
	/*
	**  Scan integrity catalog for possible tuples.  If found,
	**  include them in the integrity qualification.
	*/

	iqual = (PST_QNODE *) NULL;

        /* Set up constant part of rdf query tree control block */
	pst_rdfcb_init(&rdf_tree_cb, sess_cb);
        STRUCT_ASSIGN_MACRO(resvar->pss_tabid, rdf_tree_cb.rdf_rb.rdr_tabid);
        rdf_tree_cb.rdf_rb.rdr_types_mask = RDR_INTEGRITIES | RDR_QTREE;
	rdf_tree_cb.rdf_rb.rdr_qtuple_count = 1;  
		/* Get 1 integ tree at a time */
	rdf_tree_cb.rdf_info_blk = resvar->pss_rdrinfo;

	/* Get all integrity tuples */
	pst_rdfcb_init(&rdf_cb, sess_cb);
	STRUCT_ASSIGN_MACRO(resvar->pss_tabid, rdf_cb.rdf_rb.rdr_tabid);
	rdf_cb.rdf_rb.rdr_types_mask = RDR_INTEGRITIES;
	rdf_cb.rdf_rb.rdr_update_op = RDR_OPEN;
	rdf_cb.rdf_rb.rdr_rec_access_id = NULL;
	rdf_cb.rdf_rb.rdr_qtuple_count = 20;  /* Get 20 integrities at a time */
	rdf_cb.rdf_info_blk = resvar->pss_rdrinfo;
	/* For each group of 20 integrities */
	while (rdf_cb.rdf_error.err_code == 0)
	{
	  status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

	  /*
          ** RDF may choose to allocate a new info block and return its address
	  ** in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
	  ** memory leak and other assorted unpleasantries
          */
	  if (rdf_cb.rdf_info_blk != resvar->pss_rdrinfo)
	  {
	    resvar->pss_rdrinfo =
		rdf_tree_cb.rdf_info_blk = rdf_cb.rdf_info_blk;
	  }
	  
	  /*
	  ** Must not use DB_FAILURE_MACRO because E_RD0011 returns E_DB_WARN
	  ** that would be missed.
	  */
	  if (status != E_DB_OK)
	  {
	    if (   rdf_cb.rdf_error.err_code == E_RD0011_NO_MORE_ROWS
		|| rdf_cb.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	    {
		status = E_DB_OK;
		if (rdf_cb.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
		    continue;
	    }
	    else if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	    {
		(VOID) psf_error(2117L, 0L, PSF_USERERR,
		    &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) &resvar->pss_tabname),
		    &resvar->pss_tabname);
		status = E_DB_ERROR;
		goto exit;
	    }
	    else
	    {
		(VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
		goto exit;
	    }
	  }

	  rdf_cb.rdf_rb.rdr_update_op = RDR_GETNEXT;
    
	  /* FOR EACH INTEGRITY */
	  for 
	  (
	    qp = rdf_cb.rdf_info_blk->rdr_ituples->qrym_data,
	    tupcount = 0;
	    tupcount < rdf_cb.rdf_info_blk->rdr_ituples->qrym_cnt;
	    qp = qp->dt_next,
	    tupcount++
	  )
	  {
	    inttup = (DB_INTEGRITY*) qp->dt_data;

	    /*
	    ** Ignore FIPS constraints (i.e. SQL92 constraints)
	    ** (they are implemented via rules instead)
	    */
	    if (inttup->dbi_consflags != 0)
	    {
		continue;
	    }

	    /* check for some domain set overlap */

	    domset = (i2*) inttup->dbi_columns.db_domset;

	    for (u = 0; u < DB_COL_WORDS*2; u++)
	    {
		if ((dset[u] & domset[u]) != 0)
		    break;
	    }

	    /*
	    ** Check for appends where defaults don't satisfy integrity.
	    */

	    if ((u >= DB_COL_WORDS*2) && !subset)
	    {
		continue;
	    }

	    /* Get integrity tree and make qtree point to it */
	STRUCT_ASSIGN_MACRO(inttup->dbi_tree, rdf_tree_cb.rdf_rb.rdr_tree_id);
	    rdf_tree_cb.rdf_rb.rdr_qrymod_id = inttup->dbi_number;
	    rdf_tree_cb.rdf_rb.rdr_update_op = 0;
	    rdf_tree_cb.rdf_rb.rdr_rec_access_id = NULL;
	    rdf_tree_cb.rdf_info_blk = resvar->pss_rdrinfo;
	    rdf_tree_cb.rdf_rb.rdr_integrity = NULL;
	    STRUCT_ASSIGN_MACRO(inttup->dbi_tabid,
				rdf_tree_cb.rdf_rb.rdr_tabid);
	    rdf_tree_cb.rdf_rb.rdr_sequence = inttup->dbi_number;

	    status = rdf_call(RDF_GETINFO, (PTR) &rdf_tree_cb);
	    /*
	    ** RDF may choose to allocate a new info block and return its
	    ** address in rdf_info_blk - we need to copy it over to pss_rdrinfo
	    ** to avoid memory leak and other assorted unpleasantries
	    */
	    if (rdf_tree_cb.rdf_info_blk != resvar->pss_rdrinfo)
	    {
		resvar->pss_rdrinfo =
		    rdf_cb.rdf_info_blk = rdf_tree_cb.rdf_info_blk;
	    }

	    if (DB_FAILURE_MACRO(status))
	    {
		if (rdf_tree_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		{
		    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
			&err_code, err_blk, 1,
			psf_trmwhite(sizeof(DB_TAB_NAME), 
			    (char *) &resvar->pss_tabname),
			&resvar->pss_tabname);
		}
		else
		{
		    (VOID) psf_rdf_error(RDF_GETINFO, &rdf_tree_cb.rdf_error,
			err_blk);
		}
		goto exit;
	    }
	    pnode   = 
	    (PST_PROCEDURE *) rdf_tree_cb.rdf_rb.rdr_integrity->qry_root_node;
	    qtree = pnode->pst_stmts->pst_specific.pst_tree;
		
	    /* trim off (null) target list */
	    p = qtree->pst_qtree->pst_right;
	    /* use a copy of the qtree because the qtree is in RDF memory */
	    dup_rb.pss_tree = qtree->pst_qtree->pst_right;
	    dup_rb.pss_dup  = &p;
	    status = pst_treedup(sess_cb, &dup_rb);

	    {	/* unfix the query tree no matter what the above status is */
		DB_STATUS	temp_status;

		temp_status = rdf_call(RDF_UNFIX, (PTR) &rdf_tree_cb);
	        resvar->pss_rdrinfo = 
			rdf_cb.rdf_info_blk = rdf_tree_cb.rdf_info_blk;
		if (DB_FAILURE_MACRO(temp_status))
		{
		    (VOID) psf_rdf_error(RDF_UNFIX, &rdf_tree_cb.rdf_error,
			err_blk);
		    status = temp_status;
		}
	    }
	    if (DB_FAILURE_MACRO(status))
		goto exit;		/* close integrity file */

	    /*
	    ** Make the result variable for the integrity the same as the result
	    ** variable for the user query.  
	    ** I AM NOT SURE THE FOLLOWING COMMENT APPLIES SO I AM MERGING
	    ** THE RANGE VAR FOR APPENDS.
	    ** This is not done for append because
	    ** append doesn't have a result range variable.
	    */
	    for (i = 0; i < PST_NUMVARS; i++)
		map[i] = i;
	    i = inttup->dbi_resvar;

	    map[i] = resvar->pss_rgno;
	    status = psy_mapvars(p, map, err_blk);
	    if (DB_FAILURE_MACRO(status))
		goto exit;

	    /* add to integrity qual */

	    if (iqual == NULL)
	    {
		status = pst_node(sess_cb, mstream, p, (PST_QNODE *) NULL,
		    PST_AND, (PTR) NULL, sizeof(PST_OP_NODE), DB_NODT, (i2) 0,
		    (i4) 0, (DB_ANYTYPE *) NULL, &iqual, err_blk, (i4) 0);

		if (DB_FAILURE_MACRO(status))
		    goto exit;		/* close integrity file */

		/*
		** Note that tmp1 will contain address of the pst_right ptr
		** of the bottom AND node in the tree constructed from the
		** qualifications found in the integrities
		*/

		tmp1 = &iqual->pst_right;
	    }
	    else
	    {
		PST_QNODE	*newnode;
		
		status = pst_node(sess_cb, mstream, p, iqual, PST_AND,
		    (PTR) NULL, sizeof(PST_OP_NODE), DB_NODT, (i2) 0, (i4) 0,
		    (DB_ANYTYPE *) NULL, &newnode, err_blk, (i4) 0);

		if (DB_FAILURE_MACRO(status))
		    goto exit;		/* close integrity file */

		iqual = newnode;
	    }
	  }
	}
	if (rdf_cb.rdf_rb.rdr_rec_access_id != NULL)
	{
	    DB_STATUS	temp_status;
	    /* unfix integrity tuples */
	    rdf_cb.rdf_rb.rdr_update_op = RDR_CLOSE;
	    temp_status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);
	    resvar->pss_rdrinfo =
		rdf_tree_cb.rdf_info_blk = rdf_cb.rdf_info_blk;
	    if (DB_FAILURE_MACRO(temp_status))
	    {
		(VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
		status = temp_status;
	    }
	}
    }

    if (qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
    {
        /*
        ** On a "define cursor", keep a copy of the integrity qualification
        ** for later use.
        */
        if (iqual == NULL)
        {
	    curblk->psc_integ = (PST_QNODE *) NULL;
        }
        else
        {
	    status = pst_trdup(curblk->psc_stream, iqual,
		&curblk->psc_integ, &sess_cb->pss_memleft, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		return (status);
	    }
	}
    }
    else
    {   
	/*
	**  Clean up the integrity qualification so that it will merge
	**  nicely into the tree, and then append it to the user's
	**  qualification.
	*/

	if (iqual != NULL)
	{
	    /* replace VAR nodes by corresponding user afcn from user's
	    ** query. For example, if the integ says r.a > 1 and the user's
	    ** query says resdom 4 = 4 + 3, the integ is modified to 4 + 3 > 1.
	    **
	    ** The following paragraph refers to the case where an integrity
	    ** refers to a variable not updated in the target list.
	    **
	    ** If there is no resdom for r.a, if the user didn't specify r.a
	    ** in the target list of the query, and the query is an append,
	    ** r.a > 1 is replaced with 'default val for col a' > 1. If the
	    ** query is a replace statement, we do nothing because r.a will
	    ** be retrieved but is not in the targ list. We will be verifying
	    ** what we know should already hold (ie the integrity constraint).
	    ** If we have replace cursor, we wan't to replace r.a with a value
	    ** so a retrieve can be avoided on the replace command. We can't
	    ** have col a = 5 where r.a > 1. We do, however, have the value r.a
	    ** in the row to be updated. QEF has this value. We replace r.a with
	    ** a curval node that refers to the value r.a.
	    */

	    /* will pass dup_rb, but first null out pss_tree and pss_dup */
	    dup_rb.pss_tree = (PST_QNODE *) NULL;
	    dup_rb.pss_dup  = (PST_QNODE **) NULL;
	    
	    status = psy_subsvars(sess_cb, &iqual, resvar, r->pst_left, qmode,
		(PST_QNODE *) NULL, resvar, (i4) 0, qmode,
		&curblk->psc_blkid, &found, &dup_rb);

	    if (DB_FAILURE_MACRO(status))
	    {
		return (status);
	    }

	    /*
	    ** for REPLACE CURSOR, we need to traverse down the pst_right's
	    ** until we encounter NULL in place of which we will append the
	    ** qualification from the ROOT
	    ** Note that iqual is guaranteed to be an AND node with BOP for a
	    ** left child and either AND or NULL for the right child.
	    */

	    if (qmode == PSQ_REPCURS)
	    {
		for (tmp1 = &iqual->pst_right; (*tmp1) != (PST_QNODE *) NULL;
		     tmp1 = &(*tmp1)->pst_right)
		;
	    }
	    
	    /*
	    ** append qualification from the tree to the integrities and
	    ** make the result the new qualification for the tree
	    */
	    (*tmp1) = r->pst_right;
	    r->pst_right = iqual; 
	}
    }

exit:
    if ((qmode != PSQ_REPCURS)
	&&
	(rdf_cb.rdf_rb.rdr_rec_access_id != NULL))
    {
	DB_STATUS	temp_status;
	/* unfix integrity tuples */
	rdf_cb.rdf_rb.rdr_update_op = RDR_CLOSE;
	temp_status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);
	resvar->pss_rdrinfo =
	    rdf_tree_cb.rdf_info_blk = rdf_cb.rdf_info_blk;
	if (DB_FAILURE_MACRO(temp_status))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	    status = temp_status;
	}
    }
    if (status == E_DB_OK)
    {
	*result = r;
    }
    return (status);
}
