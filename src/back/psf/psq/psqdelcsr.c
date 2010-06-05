/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <me.h>
#include    <qu.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
** Name: psqdelcsr.c - PSF routine to delete current row of cursor.
**
** Description:
**	This file contains routines for supporting the cursor DELETE statement.
**	This support is required for processing qrymod requests associated
**	with a cursor modification (DELETE/UPDATE). 
**
**	This should not be confused with the routine named psq_delcursor
**	which deletes a cursor control block.
**
** Defines:
**          psq_dlcsrrow - Duplicate a rule tree.
**
** History:
**	21-apr-89 (neil)
**	    Written.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	05-may-93 (ralph)
**	    DELIM_IDENT:
**	    Don't lower case owner or table name in psq_dlcsrrow().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids().
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/*{
** Name: psq_dlcsrrow		Delete current row from a cursor.
**
** External Interface:		status = psq_call(PSQ_DELCURS, psq_cb, pss_cb);
**
** Description:
**	This routine processes the the cursor DELETE statement.  Since this
**	statement does not arrive to the server via query text a special
**	entry point is provided.  This routine was written to allow rules
**	application to be done for the cursor DELETE statement too.
**
**	This routine follows the exact same steps taken when normally
**	parsing a statement.  First the cursor is found in the session's list
**	of open cursors and set in the session CB.  If not found an error is
**	returned.  Next the session's language is set to the language of the
**	original cursor definition.  If the language is SQL then the table
**	name is compared against the original cursor table name.  If they are
**	not the same an error is issued.   If there is no DELETE permission
**	for the cursor then an error is given.  
**
**	If the saved table status mask indicates that there are rules (later
**	this can be done for other qrymod status) then a range table entry
**	is made so that later qrymod processing will work.
**	
**	From here an empty root node is constructed (there's a NULL target list
**	and qualification).  Then qrymod processing is applied - currently
**	only rules processing will actually be applied - and modifications
**	and made to the query tree (statement tree).  The query tree
**	header is attached and the tree is rooted in QSF.  Finally the
**	query mode is set.
**
** Restrictions:
**	As noted only rules processing is checked for and applied.  This can
**	be extended to include other qrymod processing if they are processed
**	on a cursor DELETE.
**
** Inputs:
**	psq_cb			Call control block.
**	    .psq_cursid		Cursor id of the current row being deleted.
**	    .psq_als_owner	Owner name, if it was specified.
**	    .psq_tabname	Table name of the SQL cursor DELETE 
**				statement (ignored if QUEL).
**	sess_cb			Session control block with various fields
**				used by this routine.
**	    .pss_curstab	Table of open cursors.
**
** Outputs:
**	psq_cb
**	    .psq_mode		Query mode = PSQ_DELCURS.
**	    .psq_result		QSF id of cursor DELETE query tree.
**	sess_cb
**	    .pss_lang		Query language of cursor definition.
**	    .pss_crsr		Set to current cursor.
**	    .pss_resrng		Range table (if required) for rules.
**
**	err_blk.err_code		Filled in if an error happened:
**		2205			  Cursor not open.
**		2211			  No DELETE permission.
**		2227			  (SQL only) Table and/or owner name
**					  is wrong.
**		Errors from other PSF utilities
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	1. Constructs a query tree.
**	2. Qrymod processing may set various unused fields of the cursor CB.
**
** History:
**	21-apr-89 (neil)
**	    Written.
**	23-mar-90 (andre)
**	    Cursor block will include name of the table owner and it will be
**	    checked against that specified by the caller.
**	18-apr-90 (andre)
**	    Allow user specify synonym name in DELETE CURSOR.
**	12-sep-90 (teresa)
**	    init pss_retry to PSS_REPORT_MSGS instead of false.  This is part
**	    of fixing faulty pss_retry logic.
**	30-oct-92 (barbara)
**	    Set range variable information for OPF.  Required for Star.
**	08-apr-93 (andre)
**	    several fields (inlcuding psc_tbl_mask) have migrate from PSC_CURBLK
**	    into PSC_TBL_DESCR a list of which will be attached to PSC_CURBLK.
**	    To determine whether some rules have been defined on the table/view
**	    on which cursor was declared we need to examine psc_tbl_mask of the
**	    first element of the list
**	05-may-93 (ralph)
**	    DELIM_IDENT:
**	    Don't lower case owner or table name in psq_dlcsrrow().
**	25-may-1993 (rog)
**	    Added status argument to psq_cbreturn() call so specify a good exit
**	    or a bad exit.
**	27-nov-02 (inkdo01)
**	    Range table expansion (i4 changed to PST_J_MASK).
**	14-jun-06 (toumi01)
**	    Allow session temporary tables to be referenced without the
**	    "session." in DML. This eliminates the need for this Ingres-
**	    specific extension, which is a hinderance to app portability.
**	    If psl_rngent returns E_DB_INFO it found a session temporary
**	    table, which is perfectly okay in this context.
*/

DB_STATUS
psq_dlcsrrow(
	PSQ_CB		*psq_cb,
	PSS_SESBLK	*sess_cb)
{
    PSC_CURBLK		*csr;		/* Current cursor to delete */
    PST_QNODE		*rootnode;	/* Root node of query tree */
    PST_RT_NODE		root;
    PST_QTREE		*tree;		/* Query tree stuff */
    PST_PROCEDURE	*procnode;
    PSS_RNGTAB		*resrange = (PSS_RNGTAB *) NULL;
    char		*blank = " ";
    DB_STATUS		status, loc_status;
    PSC_TBL_DESCR	*first_descr;
    i4		err_code;

    status = psq_cbinit(psq_cb, sess_cb);
    if (status != E_DB_OK)
	return(status);

    psq_cb->psq_mode = PSQ_DELCURS;	/* Set query mode */
    sess_cb->pss_retry = PSS_REPORT_MSGS;
    sess_cb->pss_lineno = 1;		/* All error messages require this */

    /* Find the cursor by name - if not found return an error */
    status = psq_crfind(sess_cb, &psq_cb->psq_cursid, &csr, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return(status);
    if (csr == NULL || csr->psc_open == FALSE)
    {
	(VOID) psf_error(2205L, 0L, PSF_USERERR, &err_code, 
	    &psq_cb->psq_error, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    psf_trmwhite(DB_CURSOR_MAXNAME, psq_cb->psq_cursid.db_cur_name),
	    psq_cb->psq_cursid.db_cur_name);
	return (E_DB_ERROR);
    }
    sess_cb->pss_crsr = csr;

    /* Set language as some cursor statements arrive w/o a language value */
    psq_cb->psq_qlang = sess_cb->pss_lang = csr->psc_lang;

    if (csr->psc_delall == FALSE)
    {
	(VOID) psf_error(2211L, 0L, PSF_USERERR, &err_code, 
	    &psq_cb->psq_error, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    psf_trmwhite(DB_CURSOR_MAXNAME, psq_cb->psq_cursid.db_cur_name),
	    psq_cb->psq_cursid.db_cur_name);
	return (E_DB_ERROR);
    }

    /*
    ** Open memory stream for query tree.  Any errors after this point must
    ** clean up allocated memory.  The memory gets cleaned up by calling 
    ** psq_cbreturn() with a non-OK status.
    */
    status = psf_mopen(sess_cb, QSO_QTREE_OBJ, &sess_cb->pss_ostream,
		       &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    /*
    ** first descriptor contains information about the table/view over which the
    ** cursor has been defined
    */
    first_descr = (PSC_TBL_DESCR *) csr->psc_tbl_descr_queue.q_next;

    /*
    ** If SQL compare table names (ignore case) - if different return an error.
    ** Note that the user may have supplied synonym name (possibly qualified by
    ** the name of the synonym's owner), so here we will determine it the name
    ** of the underlying object and its owner.
    ** QUEL DELETE does not have a table name.
    */
    if (csr->psc_lang == DB_SQL)
    {
	register char	*cp, *last;
	i4		rngvar_info;

	cp = psq_cb->psq_als_owner.db_own_name;

	if (!CMcmpcase(cp, blank))
	{
	    /*
	    ** if user has not specified name of the table owner, we need to
	    ** determine it; besides, name supplied by the user may turn out to
	    ** be a synonym, in which case we will overwrite synonym name with
	    ** that of the underlying object.
	    ** Since this info may also have to be obtained if we need to check
	    ** for any rules defined on this table, we will obtain info needed
	    ** for processing of the rules here as well (if it looks like there
	    ** rules defined on the table.)
	    */

	    status = psl_rngent(&sess_cb->pss_auxrng, -1, "",
		&psq_cb->psq_tabname, sess_cb,
		    /* for rules we need to obtain attribute info */
		((first_descr->psc_tbl_mask & DMT_RULE) == 0L),
		&resrange, psq_cb->psq_mode, &psq_cb->psq_error,
		&rngvar_info);
	    if (status == E_DB_INFO)
		status = E_DB_OK;	/* session temp tables are okay */
	}
	else
	{
	    /*
	    ** user supplied owner.obj_name; however, this may refer to a
	    ** synonym, in which case we have to overwwrite names of the synonym
	    ** and its owner with those of the underlying table and its owner
	    */
		
	    status = psl_orngent(&sess_cb->pss_auxrng, -1, "",
		 &psq_cb->psq_als_owner, &psq_cb->psq_tabname, sess_cb,
		    /* for rules we need to obtain attribute info */
		 ((first_descr->psc_tbl_mask & DMT_RULE) == 0L),
		 &resrange, psq_cb->psq_mode, &psq_cb->psq_error,
		 &rngvar_info);
	}

	if (status != E_DB_OK)
	    goto exit;

	if (rngvar_info & PSS_BY_SYNONYM)
	{
	    /*
	    ** if user supplied a synonym name, overwrite name of the
	    ** synonym and its owner with those of the underlying object
	    ** and its owner
	    */

	    STRUCT_ASSIGN_MACRO(resrange->pss_ownname, psq_cb->psq_als_owner);
	    STRUCT_ASSIGN_MACRO(resrange->pss_tabname, psq_cb->psq_tabname);
	}
	else if (!CMcmpcase(psq_cb->psq_als_owner.db_own_name, blank))
	{
	    /*
	    ** if user supplied table name but not the owner name, copy it
	    ** into psq_cb where the next check expects to find it
	    */

	    STRUCT_ASSIGN_MACRO(resrange->pss_ownname, psq_cb->psq_als_owner);

	}

	if (MEcmp((PTR) &psq_cb->psq_tabname, (PTR) &csr->psc_tabnm, DB_TAB_MAXNAME)
	    ||
	    MEcmp((PTR) &psq_cb->psq_als_owner, (PTR) &csr->psc_ownnm,
		  DB_OWN_MAXNAME))
	{
	    (VOID) psf_error(2227L, 0L, PSF_USERERR, &err_code, 
		&psq_cb->psq_error, 4,
		psf_trmwhite(DB_TAB_MAXNAME, (char *)&csr->psc_tabnm),
		&csr->psc_tabnm,
		psf_trmwhite(DB_OWN_MAXNAME, (char *)&csr->psc_ownnm),
		&csr->psc_ownnm,
		psf_trmwhite(DB_TAB_MAXNAME, (char *)&psq_cb->psq_tabname),
		&psq_cb->psq_tabname,
		psf_trmwhite(DB_OWN_MAXNAME, (char *)&psq_cb->psq_als_owner),
		&psq_cb->psq_als_owner);
	    status = E_DB_ERROR;
	    goto exit;
	}
    } /* If checking owner/table name */

    sess_cb->pss_resrng = resrange;

    /* Make the root node */
    root.pst_tvrc = 0;
    MEfill(sizeof(PST_J_MASK), 0, (char *)&root.pst_tvrm);
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	/* OPF requires range variable info for Star delete cursors */
	BTset(resrange->pss_rgno, (char *) &root.pst_tvrm);
	root.pst_tvrc = BTcount((char *)&root.pst_tvrm, BITS_IN(root.pst_tvrm));
    }
    root.pst_lvrc = 0;
    MEfill(sizeof(PST_J_MASK), 0, (char *)&root.pst_lvrm);
    root.pst_mask1 = 0;
    MEfill(sizeof(PST_J_MASK), 0, (char *)&root.pst_lvrm);
    root.pst_qlang = csr->psc_lang;
    root.pst_rtuser = TRUE;
    root.pst_union.pst_next  = NULL;
    root.pst_dups = PST_ALLDUPS;
    status = pst_node(sess_cb, &sess_cb->pss_ostream, 
		      (PST_QNODE *)NULL, (PST_QNODE *)NULL,
		      PST_ROOT, (PTR)&root, sizeof(PST_RT_NODE),
		      DB_NODT, (i2)0, (i4)0, (DB_ANYTYPE *)NULL,
		      &rootnode, &psq_cb->psq_error, (i4) 0);
    if (status != E_DB_OK)
	goto exit;

    /*
    ** Apply qrymod (currently only rules).
    */
    /*
    ** if table has not been looked up before (i.e. language is NOT SQL),
    ** do it now.  Since QUEL DELETE doesn't involve table name,
    ** we will use table name and owner name stored in the cursor CB.
    */
    if (resrange == (PSS_RNGTAB *) NULL)
    {
	i4		rngvar_info;

	status = psl_orngent(&sess_cb->pss_auxrng, -1, "",
	     &csr->psc_ownnm, &csr->psc_tabnm, sess_cb, FALSE, &resrange,
	     psq_cb->psq_mode, &psq_cb->psq_error, &rngvar_info);

	if (status != E_DB_OK)
	    goto exit;
    }

    sess_cb->pss_resrng = resrange;
    status = psy_rules(sess_cb, psq_cb->psq_mode, rootnode,
		       &psq_cb->psq_error);
    if (status != E_DB_OK)
	goto exit;

    /* Make the header */
    status = pst_header(sess_cb, psq_cb, PST_UNSPECIFIED, (PST_QNODE *)NULL,
			rootnode, &tree, &procnode, PST_0FULL_HEADER, 
			(PSS_Q_XLATE *) NULL);
    if (status != E_DB_OK)
	goto exit;

    /* Fix the root in QSF */
    if (procnode != (PST_PROCEDURE *) NULL)
    {
	status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR)procnode,
			   &psq_cb->psq_error);
	if (status != E_DB_OK)
	    goto exit;
    }

exit:

    loc_status = psq_cbreturn(psq_cb, sess_cb, status);
    if (loc_status > status)
	status = loc_status;
    return (status);
} /* psq_dlcsrrow */
