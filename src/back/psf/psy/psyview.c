/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <me.h>
#include    <tm.h>
#include    <qu.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <usererror.h>

/**
**
**  Name: PSYVIEW.C - View processing functions
**
**  Description:
**	This module does the view processing.  Basically, it operates
**	by detecting all references to views and replacing them by
**	references to real relations.  There are a number of cases
**	when it cannot do this, to whit:
**
**	Syntactic problems:  the view may have a domain defined as
**	a non-simple value (that is, not a simple attribute), which
**	is then required to take on a value.  For example, if the
**	view is defined as
**		range of x is baserel
**		define v (d = x.a / 3)
**	and then referenced as
**		append to v (d = 7)
**	would result after query modification as
**		range of x is baserel
**		append to baserel (a / 3 = 7)
**	which is not acceptable.  Of course, there is a range of cases
**	where this can be fixed, but (for the time being) we will just
**	throw them all out.
**
**	Disappearing tuple anomaly:  the implicit qualification on
**	a view allows tuples to disappear even when not a duplicate.
**	For example, take a view defined as:
**		range of x is baserel
**		define v (d = x.a) where x.a = 4
**	and issue the query
**		append to v (d = 5)
**	The tuple will be inserted into the base relation, but will not
**	be included in the view.  To solve that problem, we disallow
**	updates to domains included in the qualification of the query.
**	Note that this includes implicit updates, that is, an append
**	with the domain missing (which implicitly appends a zero or
**	blank domain).
**
**	Cross product problem:  a view which is defined as a cross
**	product of two relations has several update anomalies.  For
**	example, take R1 and R2 as:
**              R1 | a | b      R2 | b | c
**              ---|---|---     ---|---|---
**                 | 7 | 0         | 0 | 3
**                 | 8 | 0         | 0 | 4
**	and issue the view definition
**		range of m is R1
**		range of n is R2
**		define v (m.a, m.b, n.c) where m.b = n.b
**	which will define a view which looks like
**              view | a | b | c
**              -----|---|---|---
**                   | 7 | 0 | 3
**                   | 7 | 0 | 4
**                   | 8 | 0 | 3
**                   | 8 | 0 | 4
**	Now try issuing
**		range of v is v
**		delete v where v.a = 8 and v.c = 4
**	which will try to give a view which looks like:
**              view | a | b | c
**              -----|---|---|---
**                   | 7 | 0 | 3
**                   | 7 | 0 | 4
**                   | 8 | 0 | 3
**	which is of course unexpressible in R1 and R2.
**
**	Multiple query problem:  certain updates will require gener-
**	ating multiple queries to satisfy the update on the view.
**	Although this can be made to work, it won't now.  Cases are
**	replaces where the target list contains more than one
**	relation, and appends to a view over more than one relation.
**
**	To solve these problems, we dissallow the following cases:
**
**	I.  In a REPLACE or APPEND statement, if a 'v.d' appears
**		on the LHS in the target list of the query and
**		the a-fcn for 'v.d' is not a simple attribute.
**	II.  In REPLACE or APPEND statements, if a 'v.d' appears
**		on the LHS in a target list of the query and in
**		the qualification of the view.
**	III.  In a DELETE or APPEND statement, if the view ranges
**		over more than one relation.
**
** III above should include REPLACE, I think (vrscan already checks this
** anyhow). I do not understand what IV is for.  It makes the query
** replace tmp(att=someview.att) illegal. So we will take IV out.
**
**	IV.  In a REPLACE statement, if the query resulting after
**		modification of the tree, but before appending the
**		view qualification Qv, has more than one variable.
**	NOTE: RESTRICTION NUMBER IV HAS BEEN REMOVED.
**	V.  In any update, if an aggregate or aggregate function
**		appears anywhere in the target list of the view.
**
**	Note the assumption that the definition of a consistent update
**	is:
**		"An update is consistent if the result of
**		 performing the update on the view and then
**		 materializing that view is the same as the
**		 result of materializing the view and then
**		 performing the update."
**
**          psy_view	- Apply view definition
**	    psy_vrscan	- Scan query tree and replace RESDOM nodes
**
**
**  History:
**      19-jun-86 (jeff)    
**          written
**	29-oct-87 (stec)
**	    Fix bit maps by executing psy_fixmap; add psy_fixmap.
**	11-may-88 (stec)
**	    Modify for db procs.
**	22-july-88 (andre)
**	    reset emode to PSQ_RETRIEVE for non-target range vars in
**          DELETE/INSERT/UPDATE staatements
**	30-aug-88 (stec)
**	    Translate E_PS903 into user class error.
**	31-aug-88 (stec)
**	    The fix above had a bug.
**	06-sep-88 (stec)
**	    Change view_rngvar to view_rgvar (lint).
**	    Call psy_subsvars with view_rgvar and not its address.
**	07-oct-88 (stec)
**	    Fix bugs resulting in wrong number of rows returned.
**	04-nov-88 (stec)
**	    Fix processing for case where cb->pss_resrng turns out
**	    to represent no real variables.
**	23-nov-88 (stec)
**	    Change psy_vrscan to modify pst_ntargno.
**	05-apr-89 (andre)
**	    Since psy_subsvars() now takes care of ALL maps, we remove the call
**	    to psy_fixmap() as well as the procedure itself.  As it were,
**	    psy_fixmap() was called with a wrong parameter.  It was supposed to
**	    be called with pst_tvrm of the root node of the view tree so that
**	    every reference to the view would be replaced with references to the
**	    tables used in the definition of the view (from_list only);
**	    however, passing viewmask resluted in every reference to the view
**	    being replaced with reference to ALL tables used in definition of
**	    the view (even the tables being used in the qualification of the
**	    view.)
**	16-aug-89 (nancy)
**	    Fix bug #7461 where error results if result var for append is view
**	    and target of subselect is also a view.  Since emode is reset,
**	    use query_mode when checking for special result range var.
**	14-sep-90 (teresa)
**	    the following booleans have changed to bit flags: PSS_AGINTREE and 
**	    PSS_CATUPD
**	20-jan-92 (andre)
**	    defined psy_add_indep_obj()
**	4-may-1992 (bryanp)
**	    Added support for PSQ_DGTT_AS_SELECT for temporary tables.
**	3-apr-93 (markg)
**	    Moved the doaudit() routine from psypermit.c to this file and
**	    modifed psy_view() to call doaudit() while unravelling view
**	    definitions in the range table. This means that in C2 servers 
**	    the auditing data structures are built as each view is unravelled,
**	    and will now contain the correct data.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-sep-93 (tam)
**	    In star, if local schema of the underlying tables has changed, as 
**	    detected by pst_showtab, change psy_view() to report error (b54663).
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	21-oct-93 (andre)
**	    added obj_type to psy_add_indep_obj() interface since this function
** 	    will now also be used to accumulate ids of synonyms used in dbproc's
** 	    definition
**	21-oct-93 (stephenb)
**	    INSERT statement is not being audited, altered code so that a range
**	    table will be built for this instance.
**	16-nov-93 (stephenb)
**	    Altered routines so that UPDATE WHERE CURRENT OF CURSOR is audited.
**	2-dec-93 (robf)
**          Renamed doaudit() to psy_build_audit_info()
**	1-feb-94 (stephenb)
**	    Backed out INSERT fixes for 21-oct, and made 16-nov fix for
**	    update cursor more efficient.
**      31-jul-97 (stial01)
**          psy_view() Copy vtree->pst_rangetab[rgno]->pst_rngvar to pass
**          to pst_rgrent(). RDF_UNFIX in pst_rgrent frees this memory.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    pst_treedup().
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_view(
	PSF_MSTREAM *mstream,
	PST_QNODE *root,
	PSS_USRRANGE *rngtab,
	i4 qmode,
	PSS_SESBLK *sess_cb,
	DB_ERROR *err_blk,
	PST_J_ID *num_joins,
	i4 *resp_mask);
i4 psy_vrscan(
	i4 with_check,
	PST_QNODE *root,
	PST_QNODE *vtree,
	i4 qmode,
	PSS_RNGTAB *resvar,
	i4 *rgno,
	DB_ERROR *err_blk);
static i4 psy_newvar(
	PSS_SESBLK *sess_cb,
	PSS_RNGTAB *rngvar,
	PSY_VIEWINFO **cur_viewinfo,
	bool *check_perms,
	PSF_MSTREAM *mstream,
	i4 qmode,
	DB_ERROR *err_blk);
static void psy_nmv_map(
	PST_QNODE *t,
	i4 rgno,
	char *nmv_map);
i4 psy_translate_nmv_map(
	PSS_RNGTAB *parent,
	char *parent_attrmap,
	i4 rel_rgno,
	char *rel_attrmap,
	void (*treewalker)(PST_QNODE*, i4, char*),
	DB_ERROR *err_blk);
static void psy_add_indep_obj(
	PSS_SESBLK *sess_cb,
	PSS_RNGTAB *rngvar,
	DB_TAB_ID *indep_id_list,
	i4 obj_type,
	i4 *list_size);
static PST_QNODE *psy_rgvar_in_subtree(
	register PST_QNODE *node,
	register i4 rgno);
bool psy_view_is_updatable(
	PST_QTREE *tree_header,
	i4 qmode,
	i4 *reason);
static i4 psy_build_audit_info(
	PSS_SESBLK *sess_cb,
	PSS_USRRANGE *rngtab,
	i4 qmode,
	PSF_MSTREAM *mstream,
	DB_ERROR *err_blk);
i4 psy_ubtid(
	PSS_RNGTAB *rngvar,
	PSS_SESBLK *sess_cb,
	DB_ERROR *err_blk,
	DB_TAB_ID *ubt_id);

/*{
** Name: psy_view	- Driver for view processing
**
** Description:
**	This routine does the view processing portion of qrymod.
**	Since the 'tree' catalog can contain relations which are
**	themselves views, it iterates over itself until no views
**	are found.  Presumably this cannot result in an infinite
**	loop.
**
**	For each range variable declared, it is checked whether
**	that variable is a view.  If not, it is ignored.
**	Then the tree which defines
**	this view is fetched from the "tree" catalog by psy_gettree().
**	which also defines any variables required by this tree
**	and adjusts the tree so that the varno's contained in the
**	tree correspond to the varno's in the range table.
**
**	'psy_subsvars' and 'psy_vrscan' really do it.  Given the root of the
**	tree to be modified, the variable number to be eliminated, and the
**	target list for a replacement tree, they actually do the
**	tacking of 'new tree' onto 'old tree'.  After it is done,
**	there should be no references to the old variable at all
**	in the tree.
**	'psy_vrscan' scans the left hand branch of the tree (the RESDOM
**	nodes) and substitutes them.
**	'psy_subsvars' scans for VAR nodes (which are
**	retrieve-only, and hence are always alright);
**	it also determines which root nodes (ROOTs and AGHEADs)
**	to which the view qualification will have to be appended;
**	psy_subsvars calls 'psy_apql' to actually append the view
**	qualification (if any) where appropriate.  Finally, the variable for
**	the view (which had all references to it eliminated by 'subsvars') is
**	un-defined, so that that slot in the range table can be re-
**	used by later scans.
**
** Inputs:
**	mstream				psf memory stream control block
**      root                            root node of query tree
**	rngtab				The range table
**	qmode				query mode --  Valid expected modes are:
** 					    PSQ_RETRIEVE, PSQ_RETINTO, 
**					    PSQ_APPEND, PSQ_DELETE, PSQ_REPLACE,
**					    PSQ_DEFCURS, PSQ_VIEW, PSQ_RULE, and
**					    PSQ_DGTT_AS_SELECT.
** 					All other modes cause silent return 
**					(no error)
**	sess_cb				PSF session control block
**	num_joins			Ptr to a var containing number of joins
**					found in the tree so far
**	resp_mask			pointer to a preinitiaslized (0L) mask
**					which may be populated with useful info
**					discovered by psy_view()
**	
** Outputs:
**      root                            Can be modified by view definition
**	err_blk				Filled in if an error happened
**	*num_joins			Will be incremented by the number of
**					joins found in the views referenced
**					by the query
**	*resp_mask			may contain useful information for the
**					caller
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    The range table is updated to delete any views and add any base
**	    relations needed to support them.
**	    Catalog I/O to get query trees.
**
** History:
**	19-jun-86 (jeff)
**          written
**	1-oct-86 (daved)
**	    modified to handle special result range var the way all range
**	    vars are handled. Reduce code space by combining two almost
**	    identical sections. Fix what appear to be certain bugs related
**	    to special result range vars and other potential bugs found while
**	    reading the code.
**	9-oct-86 (daved)
**	    don't request qrytext information when processing the view from
**	    RDF.
**	19-feb-87 (daved)
**	    save result range var copy for error handling in psy_vrscan
**	2-apr-87 (daved)
**	    set a pss_permit flag in the range table for each used entry.
**	    psy_permit will clear this flag. Psy_view will set this flag
**	    in all range entries that are derived from a table that still has
**	    this flag set, else it will clear this flag for new entries.
**	    We will then call psy_permit again, to catch all the new entries.
**	27-aug-87 (stec)
**	    Remember if PSQ_DEFCURS mode for psy_vrscan.
**	30-dec-87 (stec)
**	    Validate mode with a switch statement rather than an if stmnt.
**	    Change code to reflect  changes in psy_vcount i/f.
**	    Define and use PST_VRMAP variable rather than a i4  for
**	    variable bitmap operations. 
**	11-may-88 (stec)
**	    Modify for db procs.
**	22-july-88 (andre)
**	    Reset emode to PSQ_RETRIEVE for non-target range vars in
**          DELETE/INSERT/UPDATE staatements.  This is necessary because
**	    we should not insist that a view which appears in a subselect
**	    of a DELETE/UPDATE/INSERT statement is updatable.
**	03-aug-88 (andre)
**	    Instead of checking value of emode (which may have been reset to
**	    PSQ_RETRIEVE while processing the previous view), the above fix
**	    must involve checking query_mode, which would contain a true
**	    query mode.
**	30-aug-88 (stec)
**	    Translate E_PS903 into user class error on pst_rgent call to fix
**	    2644.
**	31-aug-88 (stec)
**	    The fix above had a bug.
**	06-sep-88 (stec)
**	    Change view_rngvar to view_rgvar (lint).
**	    Call psy_subsvars with view_rgvar and not its address.
**	03-sep-88 (andre)
**	    Modify call to pst_rgent and pst_rgrent to pass 0 as a query mode
**	    since it is clearly not PSQ_DESTROY.
**	23-may-89 (andre)
**	        Consider the following situation:DBA owns table T and a view V
**	    defined on it.  DBA grants user Andre retrieve eprmit on V, but not
**	    T.  Under present model, psy_protect() is called by psy_qrymod()
**	    twice: before psy_view() and after psy_integ().
**	        We will modify this model as follows:  check_perms will be
**	    used to indicate if there are any entries in the range table which
**	    must be presented to psy_protect() before we try to qrymod them.
**	    check_perms will be set to FALSE in the beginning of each scan
**	    through the range table.
**	        Every potential candidate for qrymod will be checked to make
**	    sure that it doesn't have to be presented to psy_protect() first;
**	    if so, qrymod will be aplied to it during the next scan.
**	        At the end of the scan, if check_perms is TRUE, psy_protect()
**	    will be invoked.
**	30-may-89 (andre)
**	    until now, psy_view was given both ptr to the root of the tree
**	    (root) and address of the ptr to the root of the query tree
**	    (result).  At the end, if everything went OK, *result was set to r.
**	    Unfortunately, depending on whether or not the last view considered
**	    by psy_view() was a parent of a non-mergeable view, r could be
**	    pointing to the tree representing the non-mergeable view, rather
**	    than the query tree.  As a result of this, psy_integ() and
**	    psy_permit() would be modifying a wrong tree (consider the following
**	    example: create table t1 (i i);
**	             create view vv1 as select * from t1;
**		     create view v1 as select distinct * from vv1;
**		     insert into t1 values (5);
**		     create integrity on t1 is i>0;
**		     update t1 set i=-1; * (0 rows) *
**		     update t1 from v1 set i=-1; * (1 rows) *
**	    ) The reason for this is that psy_integ() would apply qualification
**	    (i>0) to the tree fro v1 as opposed to the query tree.
**	    The solution was to remove assignment to *result, and, since that
**	    was the only place where result was referenced, to not pass address
**	    of the ptr to the root as an argument to psy_view().
**	02-jun-89 (andre)
**	    Use pst_dup_viewtree() to duplicate view tree + check for AGHEAD
**	    nodes with BYHEAD for a left child.  If such combination is found in
**	    the outermost SELECT of the view definition, view will be considered
**	    non-mergeable.
**	16-jun-89 (andre)
**	    Rather than call psy_sqlview(), use the info that we obtain anyway
**	    to tell if this is a SQL view.
**	16-aug-89 (nancy)
**	    Fix bug #7461 where error results if result var for append is view
**	    and target of subselect is also a view.  Since emode is reset,
**	    use query_mode when checking for special result range var.
**	08-sep-89 (andre)
**	    Accept ptr to a var containign number of joins found in the query so
**	    far.  If we find a view which involves joins, we need to increment
**	    group_id field in all operator nodes which have a valid group_id by
**	    the number of joins found so far.  We will also need to reset
**	    outer_rel masks in the range table entry(ies) for the table(s) used
**	    in the definition of the view.
**	13-sep-89 (andre)
**	    Since pst_dup_viewtree() has been collapsed into all new and
**	    improved pst_treedup(), we will be using pst_treedup() again.
**	    Note: pst_treedup()'s interface has changed.
**	09-nov-89 (andre)
**	    Multivariable view will be treated as unmergeable.  This is done to
**	    fix bug 8160.  OPF will handle it better, and the
**	    "duplicate semantics" will not be lost.
**	02-feb-90 (andre)
**	    ALL distinct views will be deemed unmergeable.  Until now distinct
**	    views woould be declared unmergeable if they were used in a
**	    non-distinct query.  This is done to fix bug 9520.
**	    Views involved in outer join will be marked as unmergeable.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	12-sep-90 (andre)
**	    as a part of fix for bug# (forgot the number), we will declare 
**	    array of pointers to structures used to collect additional 
**    	    info about views.  To avoid fetching view tree more than 
**	    once for the sole purpose of determining if the view is an 
**	    SQL view , new structure will include a bit which will allow 
**	    us to save this info.  The primary reason for this change, 
**	    however, is to fix bug (forgot the number):
**		for non-mergerable view V s.t. for relations R1, R2, ... Rn used
**		in definition of V we need to check if the current user may
**		access them, we will build a map of the attributes of the
**		view which are used in the main query; this info will be used by
**		psy_protect().
**	    NOTE: as a result of one of the changes introduced with this fix,
**	    caller of psy_view() has no reason to call psy_protect() after
**	    psy_view(), since psy_view() will call psy_protect() if there are
**	    any tables (not just views) whose permissions need to be checked.
**	14-sep-90 (teresa)
**	    the following booleans have changed to bit flags: PSS_AGINTREE and 
**	    PSS_CATUPD
**	19-mar-91 (andre)
**	    pst_rgrent() unfixes the RDF cache entry, so, once we have replaced
**	    the description of the result view with that of the relation on
**	    which it is defined, we may not access the view tree.  Therefore
**	    we need to save all information from the view tree which
**	    [information, that is] we need after calling pst_rgrent().  To
**	    reduce the amount of information which must be saved, we will
**	    replace the description of the result view with that of the relation
**	    on which it was defined after we have entered into PSF range table
**	    description of all other relations used in the definition of the
**	    view.
**	07-aug-91 (andre)
**	    psy_view() can be called by psy_dview() (via psy_qrymod()) to
**	    ascertain that the new view is not abandoned.  psy_qrymod() used to
**	    translate PSQ_VIEW into PSQ_RETRIEVE before calling psy_view(), but
**	    now psy_view() may be expected to perform special actions if the
**	    tree is that of a view, so the translation will no longer take
**	    place, i.e. psy_view() will now "know" how to deal when qmode is set
**	    to PSQ_VIEW
**
**	    Added resp_mask to the argument list.  This new field will be used
**	    to communicate all sorts of useful info to the caller of the
**	    function.  Initially, it will be used when the function is called in
**	    the course of processing CREATE/DEFINE VIEW to notify caller whether
**	    the view is grantable and/or updateable.
**	08-aug-91 (andre)
**	    if psy_view() was called to determine if the view being created is
**	    abandoned, and the view definition is updateable (as far as we can
**	    tell from the definition alone), sess_cb->pss_resrng will point at
**	    the relation in the outermost subselect of the view definition
**	11-sep-91 (andre)
**	    If processing CREATE/DEFINE VIEW, build a list of independent
**	    objects; this list will consist of tables and views owned by the
**	    current user and QUEL views owned by other users which appear in the
**	    definition of the view
**
**	    A user is guaranteed to be able to retrieve data from tables and
**	    views owned by him (I am sort of jumping the gun since we do not
**	    enforce destruction of abandoned objects, YET), so when processing
**	    CREATE/DEFINE VIEW we can avoid expanding views owned by the current
**	    user.  Furthermore, once it is established that a user can access a
**	    given view, we do not need to expand it any further since the
**	    purpose of calling psy_view() when creating a new view is to
**	    establish that a user can select data from it.
**	12-sep-91 (andre)
**	    (RE: 07-aug-91 and 08-aug-91 (andre))
**	    psy_view() will not be responsible for determining if a view is
**	    updateable.  We decided that we should not be recording if a view is
**	    updateable since the criteria for determining it may change and
**	    recording it now would result in a big conversion effort later.
**	24-sep-91 (andre)
**	    If processing CREATE PROCEDURE, build a list of independent
**	    objects; this list will consist of tables and views owned by the
**	    current user and QUEL views owned by other users which appear in the
**	    definition of the procedure; note that unlike CREATE/DEFINE VIEW
**	    case, we will not assume that views owned by the current user can be
**	    accesed in the prescribed manner (i.e. we will not reset pss_permit
**	    for that range variable.)
**	30-sep-91 (andre)
**	    use sess_cb->pss_dependencies_stream to allocate elements of the
**	    independent object lists
**	04-oct-91 (andre)
**	    when building the list of independent objects in the course of
**	    parsing a dbproc, look for duplicates only in the current sublist
**	    (i.e. until hit a PSQ_DBPLIST_HEADER or a PSQ_INFOLIST_HEADER or
**	    NULL)
**	17-oct-91 (andre)
**	    if processing CREATE/DEFINE VIEW we will avoid expanding views owned
**	    by the current user, however, before deciding to ignore such views,
**	    we must check their grantability status as it may affect
**	    grantability of the view being created.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  26-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as an array of
**	    pointers to PST_RNGENTRY structure.
**	20-jan-92 (andre)
**	    defined a new function - psy_add_indep_obj();  it will be
**	    responsible for checking if a given object needs to be inserted into
**	    an independent object list;
**
**	    while at it, I fixed a (maybe not so minor) bug: when processing
**	    dbprocs, new id could have been added to a wrong list which implied
**	    that we could/would be trashing memory
**	18-mar-92 (andre)
**	    forgot to copy join maps from view tree range entry to the new range
**	    table entry when unraveling view definitions.
**	4-may-1992 (bryanp)
**	    Added support for PSQ_DGTT_AS_SELECT for temporary tables.
**	02-jun-92 (andre)
**	    Until now if in the course of processing CREATE/DEFINE VIEW we came
**	    across a view owned by the current user, we would automatically
**	    decide to not expand it further being satisfied that the view being
**	    created/defined will not be abandoned since the underlying view
**	    would only exist if it were not abandoned.
**	    
**	    Unfortunately, this assumption introduced one real problem + it has
**	    a potential to complicate processing of certain cases of REVOKE
**	    GRANT OPTION FOR SELECT on some of the view's underlying tables
**	    owned by other users and GRANT SELECT on the view itself:
**	    
**          consider the following scenario:
**
**		U1: create T (i=i)
**		    define permit retrieve on T(i) to U2
**		U2: define view QV (T.all)
**		    create view SV as select * from QV
**		    
**	      - SV above would be created even though it is abandoned (i.e. its
**	        owner would not be able to select from it); this is possible
**		because the privileges required to select from the new view will
**		constitute a proper superset of those required to retrieve from
**		the underlying QUEL view;
**	      - when processing REVOKE GRANT OPTION FOR SELECT ON U1.T from U2,
**	        we would be forced to repeatedly scan IIDBDEPENDS to find all
**		SQL views owned by U2 which depend on SELECT privilege on U1.T
**		on which SELECT PRIVILEGE may have been granted
**	      - when processing GRANT SELECT ON U2.SV we would have to unravel
**	        the entire view tree (for privileges other than SELECT we need
**		to unravel only view trees associated with underlying views as
**		opposed to all views involved in the view's <query expression>)
**
**	    Consequently, I am going to change the algorithm as follows:
**
**	      - if in the course of processing DEFINE VIEW we encounter an
**	        object (table or view) owned by the current user, we will not
**		process this object any further; this will not cause problems
**		described above because privileges required for a new QUEL view
**		to not be abandoned are guaranteed to be a subset of those
**		required for its underlying (QUEL or SQL) views to not be
**		abandoned and one may not grant privileges on QUEL views;
**	      - if in the course of processing CREATE VIEW we encounter a table
**	        or an always-grantable view owned by the current user, we will
**		not process this object any further; this will not cause
**		problems because no one can revoke current user's privileges on
**		that table or view
**	15-jun-92 (barbara)
**	    For Sybil change interface to pst_rgent and pst_rgrent.  Set
**	    rdr_r1_distrib.
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	30-sep-92 (andre)
**	    pst_treedup() will return PSS_GROUP_BY_IN_TREE instead of
**	    PST_3GROUP_BY in dup_rb.pss_tree_info if the outermost subselect
**	    immediately contained an AGGHEAD with a BYHEAD for a left child
**
**	    if a view tree contained a singleton subselect, pst_treedup() would
**	    set PSS_SINGLETON_SUBSEL_IN_TREE in dup_rb.pss_tree_info in which
**	    case we will set PSS_SINGLETON_SUBSELECT in sess_cb->pss_stmt_flags
**	25-nov-92 (andre)
**	    psy_view() will do some special processing when handling trees for
**	    SQL views being defined WITH CHECK OPTION (HINT: this processing
**	    will be similar to processing for rules defined on views when/if we
**	    get around to supporing them.)  In particular, if an underlying view
**	    of the new view is not updatable, we will specify the reason using
**	    resp_mask and return error.
**
**	    Another change that was made in passing but will probably have a
**	    more significant impact is that rather than relying on pst_agintree
**	    as an indication that a view is not updatable, we will instruct
**	    pst_treedup() to look for aggregates in the target list of the view
**	    whose tree is being copied, and only if one is found, will we claim
**	    that a view is not updatable
**	29-dec-92 (andre)
**	    (this overrides the last sentense of the first paragraph under
**	     25-nov-92 (andre))
**	     
**	    if processing a view definition and we were asked to determine
**	    whether WITH CHECK OPTION may be specified for the view and it turns
**	    out that the view is not updatable, continue processing and return
**	    an indicator of why the view is not updatable to the caller.  Rather
**	    than treating it as an error, we will return a warning to the user
**	    and proceed to create a view WITHOUT CHECK OPTION
**	30-dec-92 (andre)
**	    pst_treedup() will not have to look for aggregates in the gtarget
**	    list of the outermost subselect of the view's definition.  Instead
**	    this information will be available inside PST_QTREE.pst_mask1
**
**	    psy_view() will learn to process a view tree as a part of
**	    CREATE RULE ON view processing
**	31-dec-92 (andre)
**	    we will no longer expect pst_treedup() to look for singleton
**	    subselects inside view trees.  Instead, if a view involved a
**	    singleton subselect, PST_SINGLETON_SUBSEL will be set inside
**	    pst_mask1 of the view tree header
**	04-jan-93 (andre)
**	    if a view which is a target of DELETE, INSERT, or UPDATE has rules
**	    defined on it, check if any of them apply before unravelling the
**	    view
**
**	    if a table which is a target of DELETE, INSERT, or UPDATE has rules
**	    defined on it, check if any of them apply before leaving the
**	    function
**	11-feb-93 (andre)
**	    if a query contained a reference(s) to TID of a base table T through
**	    a view V, V must be updatable; if not, user will be treated to an
**	    error message
**	31-mar-93 (andre)
**	    fix for bugs 50823 & 50825
**
**	    Briefly, these bugs were caused by the fact that
**	    UPDATE WHERE CURRENT OF were not going through qrymod a consequence
**	    of which was that
**		1) references to view's columns were not translated into
**		   references to the columns of the underlying base table which
**		   meant that RESDOM numbers were not changed and wrong columns
**		   were updated, and
**		2) rules defined on base tables were never attached to the query
**		   tree
**		   
**	    To fix this, we will implement the following changes:
**
**	      - during processing of DEFINE/DECLARE CURSOR, psy_view() will
**	        build PSC_TBL_DESCR structure for every view or table over which
**		the cursor is defined; these elements will be used to build
**		lists of rules pertaining to each of the cursor's underlying
**		table/views
**
**	      - psy_view() will NOT ALWAYS return E_DB_OK upon being
**	        presented with PSQ_REPCURS
**		
**	      - if (qmode == PSQ_REPCURS)
**		{
**		  if (   cursor was defined in SQL
**		      || at DECLARE CURSOR time we determined that the user
**		         possesses unconditional RETRIEVE privilege on every
**			 attribute of the table or view over which the QUEL
**			 cursor is defined
**		     )
**		  {
**		    if (   cursor was defined over a view
**			or some rules were defined over the table/view over
**			   which the cursor was defined)
**		    {
**		      we need to unravel the view referenced in the UPDATE
**		      statement, but we should skip permit checking since it was
**		      done at DECLARE CURSOR time; we may also have to invoke
**		      psy_rules() to establish whether any rules defined on the
**		      cursor's underlying table/views apply to this query
**		    }
**		    else
**		    {
**		      skip qrymod - there are no views to unravel, privileges to
**		      check, or rules to attach
**		    }
**		  }
**		  else
**		  {
**		    // cursor was defined in QUEL over another user's table,
**		    // and at DECLARE CURSOR time we were unable to verify that
**		    // the user possesses unconditional RETRIEVE on every
**		    // column of the table or view on which this cursor was
**		    // defined
**		    go through the usual algorithm - psy_protect() will
**		    verify that the user may RETRIEVE columns which appear on
**		    the RHS of assignment; psy_rules() will determine whether
**		    any rules defined on the cursor's underlying table/views
**		    apply to this query
**		  }
**		}
**
**	      - having obtained a view tree, we will immediately get rid of the
**		view's qualification since we are not at all interested in
**		dealing with it and any range variables that may be found in it
**
**	    NOTE: bug 50825 also applies to DELETE CURSOR.  However, because
**		  with DELETE CURSOR there is no need to translate references to
**		  view attributes, the fix will be made in psq_dlcsrrow()
**	12-apr-93 (andre)
**	    do NOT add elements to the independent object list if parsing a
**	    system-generated dbproc.  System-generated dbprocs are created to
**	    enforce constraints or CHECK OPTION and QEF will record their
**	    dependence on a constraint or a view, respectively
**	31-mar-93 (andre)
**	    (PSF part of fix for bug 49513)
**	    Sometime ago we changed things to avoid merging multi-variable views
**	    into the query tree.  Now we go one step further, and stop merging
**	    into the query tree 0-variable (i.e. constant) views
**	3-apr-93 (markg)
**	    Moodified this routine to call doaudit() for range table
**	    entries as views are being unravelled in C2 servers.
**	19-may-93 (andre)
**	    when unravelling a view, contents of parent's rngvar->pss_var_mask
**	    should not be automatically passed on to the child's
**	    rngvar->pss_var_mask.
**	    In particular, it makes little sense to copy PSS_EXPLICIT_QUAL and
**	    PSS_EXPLICIT_CORR_NAME.
**	    PSS_RULE_RNG_VAR and PSS_TID_REFERENCE should only be copied if the
**	    child is the parents underlying table/view. This means that only
**	    PSS_IN_INDEP_OBJ_LIST should be copied from the parent's
**	    rngvar->pss_var_mask to that of all of its children.
**	10-aug-93 (andre)
**	    fixed a cause of a compiler warning
**	14-aug-93 (andre)
**	    move code block incorrectly placed in the middle of existing 
**	    IF...ELSE
**
**	    in addition to recording ids of QUEL views owned by other users
**	    when processing definitions of views and dbprocs, we must also
**	    record ids of indices owned by other users which are directly 
**	    referenced in the view definition; this will ensure that destruction
**	    of the index will result in destruction of the view or in dbproc
**	    being marked dormant
**	20-aug-93 (andre)
**	    When processing definition of a view, we need to remember id of some
**	    base table (other than a core catalog) used in its definition.  It
**	    will be used to ensure that if the view is destroyed, QEPs for 
**	    queries and dbprocs involving this view are invalidated.  
**	    
**	    The underlying base table id (ubt_id) for the new view V will be 
**	    determined as follows:
**		- during regular qrymod processing when considering 
**		  some object X:
**		  if (    ubt_id has not yet been determined
**		      and X is a table or index
**		      and X is NOT a core catalog)
**		  {
**		      set V's ubt_id to id of X
**		  }
**		- during regular qrymod processing, upon retrieving query tree
**		  for view X
**		  if (    ubt_id has not yet been determined
**		      and underlying base table id has been determined when X 
**	   	          was first created)
**		  {
**		      set V's ubt_id to ubt_id of X
**		  }
**		- if ubt_id could not be determined during the regular qrymod
**		  processing then we will scan the range table one last time
**		  looking for an entry describing a view whose ubt_id has 
**		  been determined when it was created; if such view V1 is found,
**		  V's ubt_id will be set to that of V1
**	01-sep-93 (andre)
**	    if we are parsing a dbproc definition, and 
**	    sess_cb->pss_dbp_ubt_id.db_tab_base is 0, we are still looking for
**	    an id of a base table on which a dbproc being parsed depends, so we
**	    will set ubt_id to &sess_cb->pss_dbp_ubt_id and utilize code 
**	    previously added for determining ubt_id of a view to determine 
**	    ubt_id of a dbproc
**	10-sep-93 (tam)
**	    In star, if local schema of the underlying tables has changed,
**	    as detected by pst_showtab, report error (b54663).
**	21-oct-93 (andre)
**	    When processing definition of a dbproc, we need to store ids of 
**	    synonyms used in the dbproc definition
**	21-oct-93 (stephenb)
**	    Make sure we set build_audit TRUE even though rngtab->pss_rgno = -1
**	    because we need to audit INSERT.
**	15-nov-93 (andre)
**	    PSS_SINGLETON_SUBSELECT got moved from pss_stmt_flags to
**	    pss_flattening_flags
**	16-nov-93 (andre)
**	    Avoid recording id of any catalog (although we absolutely need to 
**	    do it for S_CONCUR catalogs and IIPROTECT, IIPROCEDURE, IISYNONYM, 
**	    IICOMMENT, and IIRULE) catalogs as the UBT_ID.  This is done in 
**	    order to avoid a situation where we alter a timestamp of a catalog 
**	    (which causes it to be opened in exclusive mode) and later try to 
**	    update that catalog which results in an unfriendly message from DMF.
**	    I feel that the number of cases where we would fail to force 
**	    invalidation of timestamps as a result of these change will be very
**	    small whereas having to deal with an irate customer unable to 
**	    destroy his object or privilege can be much more traumatic
**	16-nov-93 (stephenb)
**	    Altered so that we do continue on PSQ_REPCURS, because we need to
**	    audit this.
**	17-nov-93 (andre)
**	    if PST_CORR_AGGR/PST_SUBSEL_IN_OR_TREE/PST_ALL_IN_TREE/
**	    PST_MULT_CORR_ATTRS bits are set in vtree->pst_mask1, set
**	    PSS_CORR_AGGR/PSS_SUBSEL_IN_OR_TREE/PSS_ALL_IN_TREE/
**	    PSS_MULT_CORR_ATTRS in sess_cb->pss_flattening_flags
**	1-feb-94 (stephenb)
**	    Backed out INSERT fixes for 21-oct, and made 16-nov fix for
**	    update cursor more efficient.
**	23-may-97 (inkdo01)
**	    Don't use global temp tables for UBT of dbproc.
**      31-jul-97 (stial01)
**          psy_view() Copy vtree->pst_rangetab[rgno]->pst_rngvar to pass
**          to pst_rgrent(). RDF_UNFIX in pst_rgrent frees this memory.
**	17-jul-00 (hayke02)
**	    Set PSS_INNER_OJ in pss_stmt_flags if PST_INNER_OJ is set in
**	    pst_mask1.
**	27-nov-02 (inkdo01)
**	    Range table expansion.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	18-july-06 (dougi)
**	    Changes to handle expansion of views with derived table refs and
**	    processing of derived tables with view refs.
**	12-dec-2006 (dougi)
**	    Changes to the above to assure all derived tables are handled.
**	10-dec-2007 (smeke01) b118405
**	    Added code to reconstitute sess_cb pss_flattening_flags for 
**	    PSS_IFNULL_AGHEAD_MULTI, PSS_IFNULL_AGHEAD and PSS_AGHEAD_MULTI.
**	13-Dec-2008 (kiria01)
**	    Initialise build_indep_obj_list lest it mislead.
**      9-jun-2010 (huazh01)
**          Search for possible nested derived table after adding derived 
**          tables into a view's range table. (b123615)
*/
DB_STATUS
psy_view(
	PSF_MSTREAM	*mstream,
	PST_QNODE	*root,
	PSS_USRRANGE	*rngtab,
	i4		qmode,
	PSS_SESBLK	*sess_cb,
	DB_ERROR	*err_blk,
	PST_J_ID	*num_joins,
	i4		*resp_mask)
{
    register i4	    vn;
    register PST_QTREE	    *vtree;
    PST_PROCEDURE	    *pnode;
    i4			    viewfound, found, query_mode;
    PST_QNODE		    *r, *tree_copy, *der_tab[PST_NUMVARS];
    DB_STATUS		    status = E_DB_OK;
    PST_VRMAP		    viewmap;
    i4			    i, j;
    i4			    map[PST_NUMVARS];
    i4			    vn_map[PST_NUMVARS];
    i4		    err_code;
    RDF_CB		    rdfcb, *rdf_cb = &rdfcb;
    RDR_RB		    *rdf_rb = &rdf_cb->rdf_rb;
    PSS_DUPRB		    dup_rb, subsvar_dup_rb;
    PSS_RNGTAB		    *newrngvar;
    PSS_RNGTAB		    *rngvar;	/* the current range var */
    i4			    nonupdt_reason;
    i4			    mergeable;	/* can view substitution be done */

				/*
				** TRUE <==> view being expanded is
				** non-mergeable, and we will need to check
				** permissions on at least one of underlying
				** relations for which we need to build a map of
				** attributes of the non-mergeable view which
				** are used in the main query
				** (this was added to fix bug (forgot the 
				** number))
				*/
    bool 		    build_nmv_attmap;

    i4			    rgno;	/* result variable */
				/*
				** rgno gets set if the effective mode for the
				** variable is PSQ_APPEND, PSQ_REPLACE,
				** PSQ_DELETE, PSQ_REPCURS, and PSQ_DEFCURS.
				** However, when processing CREATE RULE on a
				** view, CREATE VIEW WITH CHECK OPTION, or if a
				** query involved a reference to TID of a view,
				** we need to determine range number of the
				** view's underlying table/view to correctly
				** propagate contents of pss_var_mask
				*/
    i4			    uv_rgno;
    PSS_RNGTAB		    view_rng,	/* the view range var */
			    *view_rgvar;/* the pointer to the view range var */
    i4			    emode;	/* effective mode */
    bool 		    check_perms; /* FALSE <==> no permits to check */
    bool		    update_op =
	(   qmode == PSQ_DELETE || qmode == PSQ_APPEND || qmode == PSQ_REPLACE
	 || qmode == PSQ_REPCURS);
    i4			    tree_info_mask;
    PSY_VIEWINFO	    *viewinfo[PST_NUMVARS];
    bool		    build_indep_obj_list = FALSE;
    i4			    indep_obj_list_size = 0;
    DB_TAB_ID		    indep_ids[PST_NUMVARS];
    i4                      indep_syn_list_size = 0;
    DB_TAB_ID               indep_syn_ids[PST_NUMVARS];
    PST_QNODE		    *qlend_node;
    PSC_CURBLK		    *cursor = (PSC_CURBLK *) NULL;
    DB_TAB_ID		    *ubt_id = (DB_TAB_ID *) NULL;
    bool		    validate_check_option =
		(sess_cb->pss_stmt_flags & PSS_VALIDATING_CHECK_OPTION) != 0;
    bool		    build_audit;
    DB_TAB_ID               tabid;
    GLOBALREF PSF_SERVBLK   *Psf_srvblk;

    /* Check the query mode */
    switch (qmode)
    {
	case PSQ_VIEW:
	{
	    PST_STATEMENT	*snode = (PST_STATEMENT *) sess_cb->pss_object;

	    /* until known otherwise, assume that the new view is grantable */
	    *resp_mask |= PSY_VIEW_IS_GRANTABLE;

	    /*
	    ** remember to build the list of independent objects on which this
	    ** new view depends for existence
	    */
	    build_indep_obj_list = TRUE;

	    /* 
	    ** we need to determine id of some base table on which this view 
	    ** depends - set ubt_id to the address of pst_basetab_id inside 
	    ** the query tree header of the view tree
	    */
	    ubt_id = &snode->pst_specific.pst_create_view.pst_view_tree->
		         pst_basetab_id;

	    break;
	}
	case PSQ_APPEND:
	case PSQ_DELETE:
	case PSQ_REPLACE:
	case PSQ_RETRIEVE:
	{
	    /*
	    ** if parsing CREATE PROCEDURE, build a list of independent objects;
	    ** eventually, we will check if such list already exists to avoid
	    ** unnecessary work, but until then we will always build the
	    ** independent object list
	    **
	    ** 12-apr-93 (andre)
	    **	    do not build independent object list for system-generated
	    **	    dbprocs
	    */
	    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
	    {
		build_indep_obj_list = 
		    (sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED) == 0;
		
		if (!sess_cb->pss_dbp_ubt_id.db_tab_base)
		{
		    ubt_id = &sess_cb->pss_dbp_ubt_id;
		}

	        if (build_indep_obj_list)
	        {
		    /* 
		    ** if the dbproc definition involved any synonyms, now is as
		    ** good time as any to store their ids for appending 
		    ** them to the dbproc's independenet object list
		    */

	            /* scan all variables in range table */
	            for (vn = 0; ; vn++)
	            {
	                /* if regular range var */
	                if (vn < PST_NUMVARS)
	                {
		            rngvar = rngtab->pss_rngtab + vn;
	                }
	                /* else if special range var */
	                else if (vn == PST_NUMVARS && qmode == PSQ_APPEND)
	                {
		            rngvar = &rngtab->pss_rsrng;
	                }
	                else
	                {
		            break;
	                }
    
	                /*
	                ** skip over entries in range table which are either 
			** empty or do not represent tables/views/indices which
			** were referenced using synonyms
	    	        */
	    	        if (   !rngvar->pss_used 
			    || rngvar->pss_rgno == -1
			    || ~rngvar->pss_var_mask & 
			           PSS_REFERENCED_THRU_SYN_IN_DBP)
	    	        {
		            continue;
	    	        }

		        /*
		        ** object described by this range entry was referenced 
		        ** through a synonym - save away synonym's id
		        */
		        psy_add_indep_obj(sess_cb, rngvar, indep_syn_ids,
			    PSQ_OBJTYPE_IS_SYNONYM, &indep_syn_list_size);
		    }
	        }
	    }

	    break;
	}
	case PSQ_DEFCURS:
	case PSQ_REPDYN:
	{
	    /* save address of the cursor CB */
	    cursor = sess_cb->pss_crsr;

	    break;
	}
	case PSQ_RETINTO:
	case PSQ_DGTT_AS_SELECT:
	case PSQ_RULE:
	{
	    break;
	}
	case PSQ_REPCURS:
	{
	    /*
	    ** see 31-mar-93 (andre) comment for details of what I am
	    ** doing and why
	    */

	    /* save address of the cursor CB */
	    cursor = sess_cb->pss_crsr;

	    if (   cursor->psc_lang == DB_SQL
		|| cursor->psc_flags & PSC_MAY_RETR_ALL_COLUMNS)
	    {
		/* perform qrymod WITHOUT permit checking */
		sess_cb->pss_resrng->pss_permit = FALSE;
	    }
	    
	    if (sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_VIEW)
	    {
		/*
		** processing cursor UPDATE/REPLACE on a view - in all
		** likelyhood, we will need to replace qualification in the copy
		** of the view tree with a PST_QLEND node.  What better time to
		** do it then now!
		*/
		status = pst_node(sess_cb, mstream, (PST_QNODE *) NULL,
		    (PST_QNODE *) NULL, PST_QLEND, (PTR) NULL, 0, DB_NODT,
		    (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL, &qlend_node,
		    err_blk, 0);
		if (DB_FAILURE_MACRO(status))
		    goto exit;
	    }
	    
	    break;
	}
	default:
	/* On unexpected query mode just return, tree unchanged */
	{
	    return (E_DB_OK);
	}
    }

    /*
    ** `define cursor' should be processed as a retrieve statement;
    ** for the most part, so will "create/define view"
    ** unravelling a view tree in the course of processing CREATE RULE on a view
    ** will follow pretty much the same path as SELECT/RETRIEVE
    */
    if (qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN || qmode == PSQ_VIEW 
			|| qmode == PSQ_RULE)
	emode = PSQ_RETRIEVE;
    else
	emode = qmode;

    r = root;
    query_mode = emode;

    /*
    ** initialize fields in dup_rb - this is request block to call tree
    ** duplication routine pst_treedup()
    */
    dup_rb.pss_op_mask = PSS_1DUP_SEEK_AGHEAD | PSS_3DUP_RENUMBER_BYLIST;
    
    dup_rb.pss_num_joins = *num_joins;

    dup_rb.pss_tree_info = &tree_info_mask;
    subsvar_dup_rb.pss_mstream = dup_rb.pss_mstream = mstream;
    subsvar_dup_rb.pss_err_blk = dup_rb.pss_err_blk = err_blk;
    subsvar_dup_rb.pss_1ptr    = dup_rb.pss_1ptr    = NULL;
    subsvar_dup_rb.pss_op_mask = 0;
    subsvar_dup_rb.pss_num_joins = PST_NOJOIN;
    subsvar_dup_rb.pss_tree_info = (i4 *) NULL;
    
    /*
    ** Initialize the map of non-mergeable views + the array of view information
    ** structures
    */
    for (i = 0; i < PST_NUMVARS; i++)
    {
	vn_map[i] = -1;
	viewinfo[i] = (PSY_VIEWINFO *) NULL;
    }

    /* initialize static fields in rdf_cb used to retrieve view trees */
    pst_rdfcb_init(rdf_cb, sess_cb);

    rdf_rb->rdr_types_mask = RDR_VIEW | RDR_QTREE;

    /* Keep scanning the range table until there are no views. */
    for (viewfound = TRUE; viewfound; )
    {
	/* scan range table for views */

	/* no views found yet */
	viewfound = FALSE;

	/* Initially, there are no permits to be checked */
	check_perms = FALSE;
	build_audit = FALSE;

	/* scan all variables in range table */
	for (vn = 0; ; vn++)
	{
	    PSY_VIEWINFO	*cur_viewinfo;
	    bool		permflag;
	    bool		replace_res_view;

	    /* if regular range var */
	    if (vn < PST_NUMVARS)
	    {
		rngvar = rngtab->pss_rngtab + vn;
	    }
	    /* else if special range var */
	    else if (vn == PST_NUMVARS && query_mode == PSQ_APPEND)
	    {
		rngvar = &rngtab->pss_rsrng;
		/*
		** Do this here since rgno is -1 and we  still need an
		** audit table built for the insert
		*/
		if (rngvar->pss_var_mask & PSS_DOAUDIT)
		{
		    build_audit = TRUE;
		}
	    }
	    /* terminate the inner for loop */
	    else
	    {
		break;
	    }

	    /*
	    ** skip over empty entries in range table, or entries representing
	    ** a non-mergeable view.
	    */
	    if (   !rngvar->pss_used
		|| rngvar->pss_rgno == -1
		|| rngvar->pss_rgtype == PST_RTREE)
	    {
		continue;
	    }

	    /*
	    ** if processing CREATE/DEFINE VIEW, skip over entries which are
	    ** accessible for the current user, since the purpose of calling
	    ** psy_view() in this case is to establish that a new view is not
	    ** abandoned, and the user can access the table or view represented
	    ** by this entry in the desired manner (i.e. select/retrieve)
	    ** This is not the case if we were asked to validate WITH CHECK
	    ** OPTION of a view definition since we need to verify that a view
	    ** being created is updatable + that the leaf underlying table of
	    ** the view being created does not occur in the qualification of the
	    ** view
	    */
	    if (   qmode == PSQ_VIEW
		&& !rngvar->pss_permit
		&& !validate_check_option)
	    {
		continue;
	    }

	    /*
	    ** if (    we need to determine underlying base table id (ubt_id) of
	    **         the view or dbproc whose definition is being processed, 
	    **     and we haven't determined it yet, 
	    **	   and the current object is a base table other than a catalog)
	    ** {
	    **     store id of the current object in *ubt_id
	    ** }
	    */
	    if (   ubt_id
		&& !ubt_id->db_tab_base
		&& !rngvar->pss_tabdesc->tbl_temporary
		&& ~rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW
		&& ~rngvar->pss_tabdesc->tbl_status_mask & DMT_CATALOG
		&& rngvar->pss_rgtype != PST_TPROC)
	    {
		ubt_id->db_tab_base  = rngvar->pss_tabid.db_tab_base;
		ubt_id->db_tab_index = rngvar->pss_tabid.db_tab_index;
	    }

	    /*
	    ** if processing CREATE/DEFINE VIEW or CREATE PROCEDURE and this
	    ** object is owned by the current user, we may need to add it to
	    ** the list of independent objects for this view or dbproc
	    */

	    if ((qmode == PSQ_VIEW || sess_cb->pss_dbp_flags & PSS_DBPROC)
		&&
		!MEcmp((PTR) &rngvar->pss_tabdesc->tbl_owner,
		    (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME)))
	    {
		/*
		** if we are building the list of independent objects, consider
		** adding this var to the list only if a view in whose
		** definition it was used has not already been added to the list
		*/
		if (build_indep_obj_list &&
		    ~rngvar->pss_var_mask & PSS_IN_INDEP_OBJ_LIST)
		{
		    psy_add_indep_obj(sess_cb, rngvar, indep_ids,
			PSQ_OBJTYPE_IS_TABLE, &indep_obj_list_size);
		}

		/*
		** if processing CREATE/DEFINE VIEW, skip over entries which
		** represent tables and views owned by the current user, since
		** the purpose of calling psy_view() in this case is to
		** establish that a new view is not abandoned and the user is
		** guaranteed to be able to retrieve/select from his own tables
		** and views;
		**
		**  17-oct-91 (andre)
		**	one minor correction: if the view being created so far
		**	appears to be grantable and the current object is a
		**	view, before proceeding to the next entry determine if
		**	this view is grantable - if it is not grantable, then
		**	neither will be the view being created
		**  02-jun-92 (andre)
		**	and another (not so minor) correction - please see
		**	(02-jun-92 (andre)) above
		**  25-nov-92 (andre)
		**	and yet another correction: in conjunction with adding
		**	support for SQL92-compliant enforcement of WITH CHECK
		**	OPTION, we will continue unravelling view tree
		**	regardless of whether a given view is owned by the
		**	current user since we need to verify that WITH CHECK
		**	OPTION may be speciifed for a given view
		**
		** 
		** for CREATE PROCEDURE, continue processing the entry as usual
		*/
		if (qmode == PSQ_VIEW && !validate_check_option)
		{
		    if (sess_cb->pss_lang == DB_SQL)
		    {
			/*
			** when processing CREATE VIEW, do not need to further
			** process entries representing tables and
			** "always grantable" views owned by the current user
			*/
			if (   ~rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW
			    || rngvar->pss_tabdesc->tbl_status_mask &
			           DMT_VGRANT_OK
			   )
			{
			    rngvar->pss_permit = FALSE;
			    continue;
			}
		    }
		    else	/* processing DEFINE VIEW */
		    {
			/*
			** when processing DEFINE VIEW, do not need to further
			** process entries representing tables and views owned
			** by the current user
			*/
			if (   *resp_mask & PSY_VIEW_IS_GRANTABLE
			    && rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW
			    && ~rngvar->pss_tabdesc->tbl_status_mask &
				   DMT_VGRANT_OK
			   )
			{
			    *resp_mask &= ~PSY_VIEW_IS_GRANTABLE;
			}

			rngvar->pss_permit = FALSE;
			continue;
		    }
		}
	    }

	    /*
	    ** NOTE: if (   mode == PSQ_VIEW
	    **		 && we were NOT asked to verify that WITH CHECK OPTION
	    **		    may be specified for this SQL view),
	    **		at this point we will only encounter tables and views
	    **		owned by other users and, in case of CREATE VIEW, views
	    **		owned by the current user which are not marked as
	    **		"always grantable" (i.e. their definition involves
	    **		object(s) owned by other user(s))
	    */

	    /*
	    ** if this is not a view, skip over it, but first determine if
	    ** we need to check if the user has the required permissions
	    ** (i.e. if the table is not owned by the current user, and
	    ** we have not determined that the user may access the table)
	    */
	    if (!(rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW))
	    {
		/*
		** if base table is owned by the user, and query mode is
		** PSQ_DEFCURS (DECLARE CURSOR), mark DELETE permission
		*/
		if (!MEcmp((PTR) &rngvar->pss_tabdesc->tbl_owner,
			   (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME)))
		{
		    /*
		    ** Note: DECLARE CURSOR may involve more than one table
		    ** (e.g. in the WHERE clause); it is WRONG to use tables
		    ** other than the one for which the cursor has been declared
		    ** when determining if user is to be allowed to delete rows
		    ** using this cursor.
		    */
		    if ((qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
					&& rngvar == sess_cb->pss_resrng)
		    {
			cursor->psc_delall = TRUE;

			/*
			** if defining a QUEL cursor, remember that the user may
			** retrieve every column of the table/view over which
			** the cursor is being defined
			*/
			if (sess_cb->pss_lang == DB_QUEL)
			    cursor->psc_flags |= PSC_MAY_RETR_ALL_COLUMNS;
		    }
		}
		else 
		{
		    /* base table or index is owned by another user */

		    /* remember that we need to check permissions */
		    if (rngvar->pss_permit)
		    {
		        check_perms = TRUE;
		    }

		    /*
		    ** if we are building the list of independent objects, 
		    ** index owned by another user will be added to the list, 
		    ** unless it (or a view in whose definition it was used) 
		    ** has already been added to the list
		    **
		    ** this is done to ensure that if this index is dropped,
		    ** views dependent on it will also be dropped and dbprocs 
		    ** dependent on it will be marked dormant
		    **
		    ** we are inserting id of the index into the independent 
		    ** object list instead of inserting a description of SELECT
		    ** privilege on index into the independent privilege list 
		    ** because IIPRIV tuples are bigger than IIDBDEPENDS and 
		    ** creating an IIPRIV tuple describing dependence of a view
		    ** or dbproc on SELECT privilege on the index instead of 
		    ** IIDBDEPENDS tuple describing dependence on the index 
		    ** itself does not buy us anything except for an additional
		    ** headache of remembering to disregard such entries when 
		    ** using independent privilege list to determine whether 
		    ** a user may grant SELECT on his view (and eventually, 
		    ** EXECUTE on his dbproc)
		    */
		    if (   (   qmode == PSQ_VIEW 
			    || sess_cb->pss_dbp_flags & PSS_DBPROC)
			&& build_indep_obj_list
			&& rngvar->pss_tabdesc->tbl_status_mask & DMT_IDX
			&& ~rngvar->pss_var_mask & PSS_IN_INDEP_OBJ_LIST)
		    {
			psy_add_indep_obj(sess_cb, rngvar, indep_ids,
			    PSQ_OBJTYPE_IS_TABLE, &indep_obj_list_size);
		    }
		}

		/*
		** check to see if we need to audit this table.
		*/
		if (rngvar->pss_var_mask & PSS_DOAUDIT)
		{
		    build_audit = TRUE;
		}

		continue;
	    }

	    /* WE HAVE FOUND A RANGE TABLE ENTRY OF INTEREST */
	    viewfound = TRUE;

	    /*
	    ** if
	    **	- this view (or a view defined on top of it) is a target of
	    **    UPDATE, INSERT, or DELETE, and
	    **	- there are rules defined on it, and
	    **	- we haven't checked if any of them apply
	    ** then
	    **  skip this entry for now
	    */
	    if (   update_op
		&& rngvar == sess_cb->pss_resrng
		&& rngvar->pss_var_mask & PSS_CHECK_RULES
	       )
	    {
		continue;
	    }

	    cur_viewinfo = viewinfo[rngvar->pss_rgno];	/* note that at this
							** point it may be
							** set to NULL
							*/
	    /*
	    ** if this is a view which was entered into the table as a result of
	    ** expanding some other view, we may need to skip expanding it until
	    ** permissions are checked if it is an SQL view owned by someone
	    ** other than the current user, and we have not determined that he
	    ** may access the it)
	    */
	    if (cur_viewinfo == (PSY_VIEWINFO *) NULL)
	    {
		/*
		** view info structure has not been allocated yet; this would
		** happen only the first time through the loop
		*/
		status = psf_malloc(sess_cb, mstream, (i4) sizeof(PSY_VIEWINFO), 
		    (PTR *) (viewinfo + rngvar->pss_rgno), err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    goto exit;
		}

		cur_viewinfo = viewinfo[rngvar->pss_rgno];
		cur_viewinfo->psy_flags = 0;
		cur_viewinfo->psy_attr_map = (PSY_ATTMAP *) NULL;
	    }
	    else if (rngvar->pss_permit				        &&
		     cur_viewinfo->psy_flags & PSY_SQL_VIEW		&&
		     MEcmp((PTR) &rngvar->pss_tabdesc->tbl_owner,
		           (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME))
	            )
	    {
		check_perms = TRUE;
		if (rngvar->pss_var_mask & PSS_DOAUDIT)
		    build_audit = TRUE;
		continue;
	    }

	    /* Retrieve view tree */

	    /* first initialize "dynamic" fields in rdf_cb */
	    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_rb->rdr_tabid);
	    rdf_rb->rdr_rec_access_id = NULL;
	    rdf_cb->rdf_info_blk = rngvar->pss_rdrinfo;

	    rdf_rb->rdr_qtuple_count = 1;	    /* I don't think that this
						    ** would change, but Teresa
						    ** was unsure
						    */

	    status = rdf_call(RDF_GETINFO, (PTR) rdf_cb);

	    /*
	    ** RDF may choose to allocate a new info block and return its
	    ** address in rdf_info_blk - we need to copy it over to pss_rdrinfo
	    ** to avoid memory leak and other assorted unpleasantries
	    */
	    rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;
	    
	    if (DB_FAILURE_MACRO(status))
	    {
		if (rdf_cb->rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		{
		    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 
			rdf_cb->rdf_error.err_code, PSF_INTERR,
			&err_code, err_blk, 1,
			psf_trmwhite(sizeof(DB_TAB_NAME), 
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname);
		}
		else
		{
		    (VOID)psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error,
			err_blk);
		}
		goto exit;
	    }

	    /* Make vtree point to the view tree retrieved */
	    pnode =
		(PST_PROCEDURE *) rdf_cb->rdf_info_blk->rdr_view->qry_root_node;
	    vtree = pnode->pst_stmts->pst_specific.pst_tree;

	    /*
	    ** if (    we need to determine underlying base table id (ubt_id) of
	    **         the view or dbproc whose definition is being processed, 
	    **     and we haven't determined it yet, 
	    **	   and the current view was defined over some base table other 
	    **	       than a catalog)
	    ** {
	    **     store ubt_id of the current view in *ubt_id
	    ** }
	    */
	    if (   ubt_id
		&& !ubt_id->db_tab_base
		&& ~rngvar->pss_var_mask & PSS_NO_UBT_ID)
	    {
		if (vtree->pst_basetab_id.db_tab_base > DM_SCATALOG_MAX)
		{
		    ubt_id->db_tab_base  = vtree->pst_basetab_id.db_tab_base;
		    ubt_id->db_tab_index = vtree->pst_basetab_id.db_tab_index;
		}
		else
		{
		    rngvar->pss_var_mask |= PSS_NO_UBT_ID;
		}
	    }

	    if (vtree->pst_qtree->pst_sym.pst_value.pst_s_root.pst_qlang ==
		DB_SQL)
	    {
		cur_viewinfo->psy_flags |= PSY_SQL_VIEW;

		/*
		** see if need to check permissions before unraveling the view;
		** need to check if
		** 1) haven't checked before AND
		** 2) object (view) is not a QUEL view (Note that to get to this
		**    point, it HAS to be an SQL view) AND
		** 3) object not owned by the present user
		*/
		
		if (rngvar->pss_permit
		    &&
		    MEcmp((PTR) &rngvar->pss_tabdesc->tbl_owner,
			  (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME))
		   )
		{
		    /*
		    ** note that we will only end up here if the current range
		    ** var is one of the range vars created while parsing the
		    ** query (hence we do not expect to get here except during
		    ** the first scan of the range table)
		    */
		    check_perms = TRUE;
		    if (rngvar->pss_var_mask & PSS_DOAUDIT)
			build_audit = TRUE;
		    continue;
		}
	    }

	    /*
	    ** if we are building the list of independent objects, QUEL view
	    ** owned by another user will be added to the list, unless it
	    ** (or a view in whose definition there were used) has already been
	    ** added to the list
	    **
	    ** for DEFINE VIEW we know that this view is not owned by the
	    ** current user (I even put in a comment to that effect above), but
	    ** this is not necessarily true for CREATE VIEW (could happen if the
	    ** view definition involves a current user's view which is not
	    ** marked as "always grantable") and CREATE PROCEDURE where we do
	    ** not stop processing an object as soon as we discover that it is
	    ** owned by the current user.
	    */
	    else if (   (qmode == PSQ_VIEW ||
			 sess_cb->pss_dbp_flags & PSS_DBPROC)
		     && build_indep_obj_list
		     && ~rngvar->pss_var_mask & PSS_IN_INDEP_OBJ_LIST)
	    {
		psy_add_indep_obj(sess_cb, rngvar, indep_ids,
		    PSQ_OBJTYPE_IS_TABLE, &indep_obj_list_size);
	    }

	    /*
	    ** if auditing is required on this view it must be done now.
	    */
	    if (rngvar->pss_var_mask & PSS_DOAUDIT)
	    {
		build_audit = TRUE;
		continue;
	    }

	    rgno = -1;
	    
	    /*
	    ** Does the range entry represent a view used in a definition of a
	    ** non-mergeable view?
	    */
	    if (rngvar->pss_rgparent > -1)
	    {
		/* find the range entry containing the view definition */
		r = rngtab->pss_rngtab[vn_map[rngvar->pss_rgparent]].pss_qtree;
		emode = PSQ_RETRIEVE;
	    }
	    else
	    {
		r = root;

		/*							    
    		** if the view being processed is not the one being	    
		** updated, change mode to PSQ_RETRIEVE to avoid
		** a problem which may arise when the view other than the one
		** being updated is not updatable.
		*/
	        if (((query_mode == PSQ_DELETE) ||
		     (query_mode == PSQ_APPEND) ||
		     (query_mode == PSQ_REPLACE))    &&
		    (sess_cb->pss_resrng != rngvar)
		   )
		{
		    emode = PSQ_RETRIEVE;
		}
		else
		{ 
		    emode = query_mode;
		}
	    }

	    if (vtree->pst_agintree)
		sess_cb->pss_stmt_flags |= PSS_AGINTREE;

	    /*
	    ** under certain circumstances, we must establish that the current
	    ** view is updatable:
	    **	- a rule is being created on this view or on some view defined
	    **    over this view, or
	    **	- WITH CHECK OPTION has is being specified on a view defined
	    **	  over this view, or
	    **	- a query contained a reference to TID of the view's underlying
	    **	  base table through the view, or
	    **	- user issued UPDATE/DELETE/INSERT on this view or on some other
	    **	  view defined over this view
	    **
	    ** psy_view_is_updatable() will verify that a view is updatable,
	    ** and provide us with an indication of why it is not updatable
	    ** otherwise
	    ** 
	    ** NOTE that when processing CURSOR UPDATE/REPLACE, we are
	    **      skipping some of the checks performed for UPDATE - the
	    **	    rationale being that it was already done at DECLARE
	    **	    CURSOR time
	    */
	    if ((   emode == PSQ_DELETE
		 || emode == PSQ_APPEND
		 || emode == PSQ_REPLACE
		 || rngvar->pss_var_mask &
			(PSS_RULE_RNG_VAR | PSS_TID_REFERENCE)
		)
		&& !psy_view_is_updatable(vtree, qmode, &nonupdt_reason))
	    {
		if (rngvar->pss_var_mask & PSS_RULE_RNG_VAR)
		{
		    /*
		    ** translate contents of nonupdt_reason into bits in
		    ** *resp_mask with which the caller will deal as
		    ** appropriate: 
		    ** - if processing definition of a view containing
		    **   WITH CHECK OPTION, a warning will be issued and a view
		    **	 will be created WITHOUT CHECK OPTION,
		    ** - if processing RULE on a view, we will return an error
		    **   here, and the caller will issue an appropriate error
		    **	 message,
		    */
		    if (nonupdt_reason & PSY_UNION_IN_OUTERMOST_SUBSEL)
		    {
			*resp_mask |= PSY_UNION_IN_UNDERLYING_VIEW;
		    }
		    else if (nonupdt_reason & PSY_DISTINCT_IN_OUTERMOST_SUBSEL)
		    {
			*resp_mask |= PSY_DISTINCT_IN_UNDERLYING_VIEW;
		    }
		    else if (nonupdt_reason & PSY_AGGR_IN_OUTERMOST_SUBSEL)
		    {
			*resp_mask |= PSY_SET_FUNC_IN_UNDERLYING_VIEW;
		    }
		    else if (nonupdt_reason & PSY_GROUP_BY_IN_OUTERMOST_SUBSEL)
		    {
			*resp_mask |= PSY_GROUP_BY_IN_UNDERLYING_VIEW;
		    }
		    else if (nonupdt_reason & PSY_MULT_VAR_IN_OUTERMOST_SUBSEL)
		    {
			*resp_mask |= PSY_MULT_VAR_IN_UNDERLYING_VIEW;
		    }
		    else if (nonupdt_reason & PSY_NO_UPDT_COLS)
		    {
			*resp_mask |= PSY_NO_UPDT_COLS_IN_UNDRLNG_VIEW;
		    }

		    if (!validate_check_option)
		    {
			/*
			** we were called as a part of processing a rule on a
			** view - since the view is not updatable, we will
			** return an error; the actual error message will be
			** produced by the caller who knows the name of the view
			** on which a user was attempting to create a rule 
			*/
			status = E_DB_ERROR;
			goto exit;
		    }
		}
		else if (rngvar->pss_var_mask & PSS_TID_REFERENCE)
		{
		    /*
		    ** if processing a query containing a reference to TID
		    ** attribute of a view (which needs to be translated into
		    ** a TID attribute of the view's underlying base table),
		    ** report an error to the user here;  we are doing it here
		    ** instead of passing the desxription of the cause to the
		    ** caller since caller cannot provide any additional
		    ** information to the user (unlike CREATE RULE case above
		    ** where the caller "knows" the name of the view on which a
		    ** user tried to create a rule
		    */
		    (void) psf_error(E_PS059C_TID_REF_IN_NONUPDT_VIEW, 0L,
			PSF_USERERR, &err_code, err_blk, 0);
		    status = E_DB_ERROR;
		    goto exit;
		}
		else
		{
		    i4	error_number = 0L;
		    
		    /*
		    ** user atempted to issue UPDATE/INSERT/DELETE against a
		    ** non-updatable view - issue appropriate error and return
		    */
		    if (nonupdt_reason & PSY_AGGR_IN_OUTERMOST_SUBSEL)
		    {
			error_number = 3350L;
		    }
		    else if (nonupdt_reason & PSY_MULT_VAR_IN_OUTERMOST_SUBSEL)
		    {
			error_number = 3330L;
		    }
		    else
		    {
			/*
			** nonupdt_reason has PSY_UNION_IN_OUTERMOST_SUBSEL,
			** PSY_DISTINCT_IN_OUTERMOST_SUBSEL,
			** PSY_GROUP_BY_IN_OUTERMOST_SUBSEL, or PSY_NO_UPDT_COLS
			** set
			*/
			error_number = 3361L;
		    }

		    psy_error(error_number, emode, rngvar, err_blk);
		    status = E_DB_ERROR;
		    goto exit;
		}
	    }
	    
	    /*
	    ** when processing INSERT/APPEND into a STAR SQL view which was
	    ** created WITH CHECK OPTION or a QUEL view (which was created
	    ** WITH CHECK OPTION by default), use old rules for enforcing
	    ** CHECK OPTION: if the view tree involved a qualification,
	    ** treat it as an error
	    */
	    if (   emode == PSQ_APPEND
		&& vtree->pst_wchk
		&& vtree->pst_qtree->pst_right->pst_sym.pst_type !=
		       PST_QLEND
		&& (   sess_cb->pss_distrib & DB_3_DDB_SESS
		    || ~cur_viewinfo->psy_flags & PSY_SQL_VIEW)
	       )
	    {
		psy_error(3321L, emode, rngvar, err_blk);
		status = E_DB_ERROR;
		goto exit;
	    }

	    /*
	    ** if a view tree contained a singleton subselect, set
	    ** PSS_SINGLETON_SUBSELECT in sess_cb->pss_flattening_flags
	    */
	    if (vtree->pst_mask1 & PST_SINGLETON_SUBSEL)
		sess_cb->pss_flattening_flags |= PSS_SINGLETON_SUBSELECT;
	
	    /*
	    ** if PST_CORR_AGGR/PST_SUBSEL_IN_OR_TREE/PST_ALL_IN_TREE/
	    ** PST_MULT_CORR_ATTRS bits are set in vtree->pst_mask1, set
	    ** PSS_CORR_AGGR/PSS_SUBSEL_IN_OR_TREE/PSS_ALL_IN_TREE/
	    ** PSS_MULT_CORR_ATTRS in sess_cb->pss_flattening_flags
	    */
	    if (vtree->pst_mask1 & PST_CORR_AGGR)
		sess_cb->pss_flattening_flags |= PSS_CORR_AGGR;
	    if (vtree->pst_mask1 & PST_SUBSEL_IN_OR_TREE)
		sess_cb->pss_flattening_flags |= PSS_SUBSEL_IN_OR_TREE;
	    if (vtree->pst_mask1 & PST_ALL_IN_TREE)
		sess_cb->pss_flattening_flags |= PSS_ALL_IN_TREE;
	    if (vtree->pst_mask1 & PST_MULT_CORR_ATTRS)
		sess_cb->pss_flattening_flags |= PSS_MULT_CORR_ATTRS;
	    if (vtree->pst_mask1 & PST_IFNULL_AGHEAD_MULTI)
		sess_cb->pss_flattening_flags |= PSS_IFNULL_AGHEAD_MULTI;
	    if (vtree->pst_mask1 & PST_IFNULL_AGHEAD)
		sess_cb->pss_flattening_flags |= PSS_IFNULL_AGHEAD;
	    if (vtree->pst_mask1 & PST_AGHEAD_MULTI)
		sess_cb->pss_flattening_flags |= PSS_AGHEAD_MULTI;
	    if (vtree->pst_mask1 & PST_INNER_OJ)
		sess_cb->pss_stmt_flags |= PSS_INNER_OJ;


	    /* Determine if the view can be merged into the main query; 
	    ** View can be merged if it does not represent a union and
	    ** duplicate semantics are preserved as depicted below.
	    **
	    **	VIEW	QUERY	MERGEABLE
	    **	=========================
	    **	 ND	 ND	 Y  !!! NO LONGER TRUE  SEE BELOW
	    **   ND	 D	 N
	    **	 D	 D	 Y
	    **	 D	 ND	 Y
	    **
	    ** where D	    - duplicates allowed
	    **	     ND	    - no duplicates allowed
	    **	     Y, N   - yes, no
	    **
	    ** 02-feb-90 (andre)
	    **	to fix bug 9520, we will treat all distinct views (views
	    **	disallowing duplicates) as non-mergeable; so using notation
	    **	from the above table (mergeable == Y) <==> (view = D)
	    **	
	    ** View may also be declared unmergeable if there is an AGHEAD node
	    ** whose left child is a BYHEAD in a subtree s.t. the closest ROOT
	    ** parent is the root of the query or one of the roots of the union
	    ** query.
	    ** One more type of views that will be treated as unmergeable:
	    ** multi-variable view.  This is done to remove the cause of bug
	    ** 8160.
	    ** And yet another restriction on when a view would be declared
	    ** unmergeable: views involved in outer join would be declared
	    ** ummergeable.
	    ** And yet another type of view bites the dust: 0-variable (i.e.
	    ** constant) view will no longer be merged into the query tree. This
	    ** is done as a part of fix for bug 49513
	    */
	    {
		PST_RT_NODE *rn;

		/*
		** Copy the view tree from RDF while checking for
		** AGHEAD/BYHEAD combination that would make this view
		** unmergeable and resetting join_id inside operator nodes, if
		** needed.  Elements of BY lists will be renumbered as in 7.0
		** (also known as 6.3) elements of BY lists were not assigned
		** numbers.
		*/

		/* No need to reset join_id if no joins've been found so far */
		if (dup_rb.pss_num_joins != PST_NOJOIN)
		{
		    dup_rb.pss_op_mask |= PSS_2DUP_RESET_JOINID;
		}

		dup_rb.pss_tree = vtree->pst_qtree;
		dup_rb.pss_dup  = &tree_copy;
		tree_info_mask = 0;
		status = pst_treedup(sess_cb, &dup_rb);

		if (DB_FAILURE_MACRO(status))
		{
		    goto exit;
		}

		rn = &vtree->pst_qtree->pst_sym.pst_value.pst_s_root;
		if (rn->pst_union.pst_next == (PST_QNODE *) NULL	&&
		    ~*dup_rb.pss_tree_info & PSS_GROUP_BY_IN_TREE	&&
		    rn->pst_tvrc == 1	/* not a multi/zero-variable view */ &&
		    rn->pst_dups == PST_ALLDUPS				&&
		    BTnext((i4) -1, (char *) &rngvar->pss_inner_rel,
				BITS_IN(PST_J_MASK)) == -1		&&
		    BTnext((i4) -1, (char *) &rngvar->pss_outer_rel,
				BITS_IN(PST_J_MASK)) == -1		&&
		    !(sess_cb->pss_ses_flag & PSS_DERTAB_INQ)
		    
		   )
		{
		    mergeable = TRUE;
		}
		else
		{
		    mergeable = FALSE;

		    /*
		    ** mark view as non-mergeable and save address of the view
		    ** tree
		    */
		    rngvar->pss_qtree = tree_copy;
		    rngvar->pss_rgtype = PST_RTREE;

		    /* add current view to the map of non-mergeable views */
		    vn_map[rngvar->pss_rgno] = vn;
		}
	    }
		
	    /*
	    ** Enter range variables from view into the range table,
	    ** and remap the range variable numbers in the view tree.
	    */

	    /* Set up a range variable map that maps everything to itself.
	    ** It will be used to map variables in the original view tree
	    ** to variables reentered into the range table (slot nos. will
	    ** change).
	    */
	    for (i = 0; i < PST_NUMVARS; i++)
		map[i] = i;

	    /*
	    ** In case of UPDATE, INSERT, and CURSOR UPDATE/REPLACE, resno
	    ** field in a resdom must not just be a sequential number, but an
	    ** attribute number in the underlying base relation.
	    */
	    if (rngvar == sess_cb->pss_resrng && mergeable)
	    {
		switch (qmode)
		{
		    case PSQ_REPLACE:
		    case PSQ_APPEND:
		    case PSQ_REPCURS:
		    {
			i4	    enforce_check_option =
			    (   vtree->pst_wchk
			     && (   sess_cb->pss_distrib & DB_3_DDB_SESS
				 || ~cur_viewinfo->psy_flags & PSY_SQL_VIEW)
			    );
			/*
			** the old reactionary semantics of WITH CHECK OPTION
			** will be enforced for QUEL views and in STAR;
			** for local SQL views WITH CHECK OPTION will be
			** enforced using rules
			*/
			status = psy_vrscan(enforce_check_option, r, tree_copy,
			    qmode, rngvar, &rgno, err_blk);
			if (DB_FAILURE_MACRO(status))
			{
			    goto exit;
			}
			break;
		    }
		    case PSQ_DELETE:
		    case PSQ_DEFCURS:
		    case PSQ_REPDYN:
		    {
			if (cur_viewinfo->psy_flags & PSY_SQL_VIEW)
			{
			    rgno = BTnext((i4) -1,
				    (char *) &vtree->pst_qtree->
				    pst_sym.pst_value.pst_s_root.pst_tvrm,
				    PST_NUMVARS);
			}
			else
			{
			    PST_VRMAP	    varmap;

			    /*
			    ** build a map of relations whose attributes appear
			    ** in the left subtree of the view definition
			    */
			    psy_vcount(vtree->pst_qtree->pst_left, &varmap);

			    rgno = BTnext((i4) -1, (char *) &varmap,
				PST_NUMVARS);
			}

			break;  
		    }
		    default:
		    {
			break;
		    }
		}
	    }

	    /*
	    ** if
	    **       a rule is being created on this view or a view for which
	    **	     the current view is an underlying view
	    **	  or the current view is an underlying view of a new SQL view
	    **	     being created WITH CHECK OPTION
	    **	  or the query contained a reference to the TID attribute of
	    **	     this view's underlying base table through this view or some
	    **	     other view for which the current view is an underlying view
	    ** {
	    **	   we need to determine range number of the current view's
	    **	   underlying table or view;
	    **	   note that in case of references to TID attribute reference,
	    **	   this number may have been determined if the view is a result
	    **	   variable of a DELETE, INSERT, UPDATE, REPLACE CURSOR, or
	    **	   DEFINE CURSOR; otherwise we'll have to compute it here
	    ** }
	    */
	    if (rngvar->pss_var_mask & (PSS_RULE_RNG_VAR | PSS_TID_REFERENCE))
	    {
		if (rgno != -1)
		{
		    uv_rgno = rgno;
		}
		else
		{
		    if (cur_viewinfo->psy_flags & PSY_SQL_VIEW)
		    {
			uv_rgno = BTnext((i4) -1,
			    (char *) &vtree->pst_qtree->
				pst_sym.pst_value.pst_s_root.pst_tvrm,
			    PST_NUMVARS);
		    }
		    else
		    {
			PST_VRMAP	    varmap;

			/*
			** build a map of relations whose attributes appear
			** in the left subtree of the view definition
			*/
			psy_vcount(vtree->pst_qtree->pst_left, &varmap);

			uv_rgno = BTnext((i4) -1, (char *) &varmap,
			    PST_NUMVARS);
		    }
		}
	    }
	    else
	    {
		/* better safe than sorry */
		uv_rgno = -1;
	    }
	    
	    /*
	    ** MERGE VIEW INTO THE MAIN QUERY TREE.
	    ** At this point, we will only have read-only non-mergeable views
	    ** or mergeable views for update. We can, therefore, add range vars
	    ** without worrying about the case of a result relation on
	    ** a non-mergeable view.
	    */

	    /*
	    ** Create a bitmap of variables used in the view
	    **
	    ** Note:
	    **	    difference betwen psy_varset() and psy_vcount() is that the
	    **	    former looks at pst_tvrm in ROOT-type nodes whereas the
	    **	    latter looks at pst_vno found in VAR nodes; results produced
	    **	    by them may be different as in the case of
	    **		"select t1.a from t1,t2"
	    */

	    /*
	    ** NOTE: for UPDATE/REPLACE CURSOR (qmode == PSQ_REPCURS), we
	    **       will get rid of the view's qualification and replace it
	    **	     with a PST_QLEND node to make it appear that no
	    **	     qualification was present; we are doing this because we are
	    **	     trying to fast-track through qrymod and are not at all
	    **	     interested in the qualification and/or any relations that
	    **	     may be referenced in it;
	    **	    
	    **	     we will also clear out pst_tvrm in the root node and
	    **	     then set the bit corresponding to the view's underlying
	    **	     table or view (which was determined by psy_vrscan())
	    **
	    */
	    if (qmode == PSQ_REPCURS)
	    {
		/* replace view's qualification with a PST_QLEND node */
		tree_copy->pst_right = qlend_node;

		/*
		** make sure that the only bit set in the root's pst_tvrm is
		** that corresponding to the relation over which the view is
		** defined
		*/
		MEfill(sizeof(PST_J_MASK), 0,
		    (char *)&tree_copy->pst_sym.pst_value.pst_s_root.pst_tvrm);
		BTset(rgno,
		    (char *) &tree_copy->pst_sym.pst_value.pst_s_root.pst_tvrm);
		tree_copy->pst_sym.pst_value.pst_s_root.pst_tvrc = 1;
	    }

	    (VOID)psy_varset(tree_copy, &viewmap);

	    /* In case result var defined, we need to make sure that
	    ** the view refers to some variable(s). Views defined as
	    ** SELECT constant (or equivalent) are not updateable
	    ** and we need to complain.
	    */

	    if ((BTcount((char *)&viewmap, PST_NUMVARS) == 0) && 
                sess_cb->pss_resrng == rngvar)
	    {
		/* Looks like somebody has an update in mind, but
		** there is no real relation to update.
		*/
		(VOID) psf_error(2225L, 0L, PSF_USERERR,
		    &err_code, err_blk, 2, 
		    psf_trmwhite(DB_CURSOR_MAXNAME, cursor->psc_blkid.db_cur_name),
		    cursor->psc_blkid.db_cur_name,
		    psf_trmwhite(DB_TAB_MAXNAME,
			(char *) &sess_cb->pss_resrng->pss_tabname),
		    &sess_cb->pss_resrng->pss_tabname);
		status = E_DB_ERROR;
		goto exit;
	    }

	    /* Look for derived tables and WITH list elements in this view's 
	    ** range table and add their tables to the viewmap. */
	    for (i = -1; (i = BTnext(i, (char *)&viewmap, PST_NUMVARS)) != -1;)
	    {
		PST_VRMAP	dtmap;

		if (vtree->pst_rangetab[i]->pst_rgtype != PST_DRTREE &&
		    vtree->pst_rangetab[i]->pst_rgtype != PST_WETREE)
		    continue;		/* only for dertabs & WITH elems */

		(VOID)psy_varset(vtree->pst_rangetab[i]->pst_rgroot, &dtmap);

                /* b123615:
                ** search for nested derived table after adding dtmap into
                ** viewmap.
                */
                if (BTcount((char *)&dtmap, PST_NUMVARS) &&
                    !BTsubset((char *)&dtmap, (char *)&viewmap, PST_NUMVARS) &&
                    (j = BTnext(-1, (char *)&dtmap, PST_NUMVARS)) < i)
                    i = j - 1; 

                BTor(BITS_IN(dtmap), (char *)&dtmap, (char *)&viewmap);
	    }

	    /* In a separate loop (to assure the viewmap is complete), 
	    ** duplicate the parse trees of the derived tables and WITH
	    ** list elements. */
	    for (i = BTnext(-1, (char *) &viewmap, PST_NUMVARS);
		i >= 0; i = BTnext(i, (char *)&viewmap, PST_NUMVARS))
	    {
		if (vtree->pst_rangetab[i]->pst_rgtype != PST_DRTREE &&
		    vtree->pst_rangetab[i]->pst_rgtype != PST_WETREE)
		    continue;		/* only for derived tables */
		dup_rb.pss_tree = vtree->pst_rangetab[i]->pst_rgroot;
		dup_rb.pss_dup = &der_tab[i];
		status = pst_treedup(sess_cb, &dup_rb);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }

	    build_nmv_attmap = FALSE;

	    /*
	    ** note that i has already been set to range number of the first
	    ** underlying relation (-1 if none were found)
	    */

	    /* Let's merge the range variables of the view */
	    for (i = -1, replace_res_view = FALSE;
		 (i = BTnext(i, (char *) &viewmap, PST_NUMVARS)) != -1;
		)
	    {
		i4		type;
		char		*old_mask;
		PST_J_MASK	tmp_mask;
		
		if (i == rgno)
		{
		    /*
		    ** result view description, if any, will be replaced with
		    ** that of the relation on which it was defined AFTER we are
		    ** done inserting descriptions of all other relations used
		    ** in definition of the view
		    */
		    replace_res_view = TRUE;
		}
		else
		{
		    /*
		    ** Range variable name "!" means don't look for and
		    ** try to replace a range variable of the same name.
		    */
		    if ((type = vtree->pst_rangetab[i]->pst_rgtype) == 
				PST_DRTREE || type == PST_WETREE)
			status = pst_sdent(rngtab, -1, "!", sess_cb,
			  &newrngvar, der_tab[i], type, err_blk);
		    else if (type == PST_TPROC)
			status = pst_stproc(rngtab, -1, "!", 
			  vtree->pst_rangetab[i]->pst_rngvar.db_tab_base,
			  sess_cb, &newrngvar, vtree->pst_rangetab[i]->
			  pst_rgroot, err_blk);
		    else status = pst_rgent(sess_cb, rngtab, -1, "!", 
			  PST_SHWID, (DB_TAB_NAME*) NULL, (DB_TAB_OWN*) NULL,
			  &vtree->pst_rangetab[i]->pst_rngvar, FALSE, 
			  &newrngvar, (i4) 0, err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			/* If table could not be found return a user error
			** so that things can be retried.
			*/
			if (err_blk->err_code == E_PS0903_TAB_NOTFOUND)
			{
			    (VOID) psf_error(2120L, 0L, PSF_USERERR,
				&err_code, err_blk, 0);
			}
			/*
			** In star if local schema has changed, report user error
			** (b54663).
			*/
			if (err_blk->err_code == E_PS0913_SCHEMA_MISMATCH)
			{
			    (VOID) psf_error(E_PS0913_SCHEMA_MISMATCH, 0L, 
				PSF_USERERR, &err_code, err_blk, 1, 
				psf_trmwhite(DB_TAB_MAXNAME,
		                        (char *) rngvar->pss_rgname),
        		        rngvar->pss_rgname);
			}
			goto exit;
		    }

		    /*
		    ** copy inner and outer join maps into the new range table
		    ** entry
		    */
		    MEcopy((char *)&vtree->pst_rangetab[i]->pst_inner_rel,
			sizeof(PST_J_MASK), (char *)&newrngvar->pss_inner_rel);
		    MEcopy((char *)&vtree->pst_rangetab[i]->pst_outer_rel,
			sizeof(PST_J_MASK), (char *)&newrngvar->pss_outer_rel);

		    /*
		    ** remember whether we still need to check permits on this
		    ** table
		    */
		    newrngvar->pss_permit = rngvar->pss_permit;

		    /*
		    ** only PSS_IN_INDEP_OBJ_LIST gets automatically copied from
		    ** parent's pss_var_mask to its children's pss_var_mask
		    */
		    newrngvar->pss_var_mask =
			rngvar->pss_var_mask & PSS_IN_INDEP_OBJ_LIST;

		    /*
		    ** if
		    **	      this view (or a view for which this view is an
		    **        underlying view) is a target of CREATE RULE,
		    **	   or it is an underlying view of an SQL view being
		    **	      created WITH CHECK OPTION
		    **	   or a query contained a reference to the TID attribute
		    **	      of this view (or a view for which this view is an
		    **	      underlying view)
		    ** {
		    **	   copy PSS_RULE_RNG_VAR and/or PSS_TID_REFERENCE bits
		    **	   from parent's pss_var_mask into child's pss_var_mask
		    ** }
		    */
		    if (   rngvar->pss_var_mask &
			       (PSS_RULE_RNG_VAR | PSS_TID_REFERENCE)
			&& i == uv_rgno
		       )
		    {
			newrngvar->pss_var_mask |=
			    rngvar->pss_var_mask &
				(PSS_RULE_RNG_VAR | PSS_TID_REFERENCE);
		    }

		    /*
		    ** If this is a C2 server then this new
		    ** range variable should be audited.
		    */
		    if (Psf_srvblk->psf_capabilities & PSF_C_C2SECURE)
			newrngvar->pss_var_mask |= PSS_DOAUDIT;

		    /*
		    ** if (    we are trying to determine the id of the 
		    **	       underlying base table of the view or dbproc whose
		    **	       definition is being parsed, 
		    **	   and the current object is a view
		    **	   and the view in whose definition it appeared was not
		    **	       created over any base tables which were not core
		    **	       catalogs
		    **    )
		    ** {
		    **     remember that the tree associated with the current 
		    **	   object will not contain an ID of a base table whose 
		    **	   timestamp should be changed when the view or dbproc
		    **	   being created is dropped or some privilege defined 
		    **	   on it is revoked
		    ** }
		    */
		    if (   ubt_id
			&& !ubt_id->db_tab_base
			&& rngvar->pss_var_mask & PSS_NO_UBT_ID
			&& newrngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW
		       )
		    {
			newrngvar->pss_var_mask |= PSS_NO_UBT_ID;
		    }

		    status = psy_newvar(sess_cb, newrngvar,
			viewinfo + newrngvar->pss_rgno, &permflag, mstream,
			qmode, err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			goto exit;
		    }

		    check_perms = (check_perms || permflag);

		    /*
		    ** note that pss_rgparent may be set to a number != -1 if
		    ** the current view is non-mergeable or if it itself was
		    ** used in the definition of some other non-mergeable view
		    */
		    newrngvar->pss_rgparent = (mergeable) ? rngvar->pss_rgparent
							  : rngvar->pss_rgno;

		    /*
		    ** if (
		    **	    the new relation was used (directly or through its
		    **	    use in another view) in the definition of a
		    **	    non-mergeable view,
		    **	    AND
		    **	    we need to check if the current user can access it
		    **	  )
		    **     remember to build a map of attributes of its "parent"
		    **	   used in the query tree (again, directly, or through
		    **	   their use in other view(s))
		    */
		    if (permflag && newrngvar->pss_rgparent > -1)
		    {
			build_nmv_attmap = TRUE;
		    }

		    /* Set up for later re-mapping */
		    map[i] = newrngvar->pss_rgno;

		    /*
		    ** reconstruct range variable name from the view
		    ** definition; this may be later copied into PST_RNGENTRY
		    ** structure and used by OPF when displaying QEP-related
		    ** info
		    */
		    MEcopy((PTR) &vtree->pst_rangetab[i]->pst_corr_name,
			DB_TAB_MAXNAME, (PTR) newrngvar->pss_rgname);

		    /*
		    ** we may need to update the mask indicating in which (if
		    ** any) joins this range var is an outer or an inner
		    ** relation.  We won't need to do it for the relation on
		    ** which the result view (if any) was defined since result
		    ** view may NOT be a multivariable view.
		    */
		    if (dup_rb.pss_num_joins != PST_NOJOIN)
		    {
			old_mask = (char *) &newrngvar->pss_inner_rel;

			if ((j = BTnext((i4) -1, old_mask,
				    sizeof(PST_J_MASK) * BITSPERBYTE)) != -1
			   )
			{
			    MEfill(sizeof(PST_J_MASK), (u_char) 0,
				(PTR) &tmp_mask);
			    for (;
				 j != -1;
				 j = BTnext(j, old_mask,
					    sizeof(PST_J_MASK) * BITSPERBYTE))
			    {
				BTset(j + (i4) dup_rb.pss_num_joins,
				      (char *) &tmp_mask);
			    }

			    /* Copy the new mask in place of the old mask */
			    MEcopy((char *)&tmp_mask, sizeof(PST_J_MASK),
					(char *)&newrngvar->pss_inner_rel);
			}

			old_mask = (char *) &newrngvar->pss_outer_rel;

			if ((j = BTnext((i4) -1, old_mask,
				    sizeof(PST_J_MASK) * BITSPERBYTE)) != -1
			   )
			{
			    MEfill(sizeof(PST_J_MASK), (u_char) 0,
				(PTR) &tmp_mask);
			    for (;
				 j != -1;
				 j = BTnext(j, old_mask,
					    sizeof(PST_J_MASK) * BITSPERBYTE))
			    {
				BTset(j + (i4) dup_rb.pss_num_joins,
				      (char *) &tmp_mask);
			    }

			    /* Copy the new mask in place of the old mask */
			    MEcopy((char *)&tmp_mask, sizeof(PST_J_MASK),
					(char *)&newrngvar->pss_outer_rel);
			}
		    }
		}
	    } /* End of `for' loop merging view variables */

	    /*
	    ** check if we need to replace the description of the result view
	    ** with that of the relation on which it was defined; we HAD TO
	    ** postpone it until now since pst_rgrent() unfixes the view
	    ** description
	    ** IMPORTANT: once pst_rgrent() unfixes the description of rngvar,
	    **		  we may no longer use vtree as it is not guaranteed to
	    **		  point at the view tree anymore
	    */
	    if (replace_res_view)
	    {
		DB_TAB_NAME	corr_name;
		i4		save_var_mask = rngvar->pss_var_mask;

		/* This is the result relation; reuse the current range 
		** table slot for it. Only one slot is needed because 
		** updates can only occur in views defined on a single
		** variable.
		*/

		/* Remember the range variable describing merged view,
		** make view_rgvar point to the copy.
		*/
		STRUCT_ASSIGN_MACRO(*rngvar, view_rng);
		view_rgvar = &view_rng;

		/*
		** remember correlation name of the relation on which the result
		** view has been defined as we will need it after pst_rgrent()
		** has freed up the view description
		*/

		STRUCT_ASSIGN_MACRO(vtree->pst_rangetab[rgno]->pst_corr_name,
				    corr_name);

		/*
		** IMPORTANT: once pst_rgrent() unfixes the description of
		**            rngvar, we may no longer use vtree as it is
		**            not guaranteed to point at the view tree anymore
		** 
		** Copy vtree->pst_rangetab[rgno]->pst_rngvar to pass
		** to pst_rgrent.
		**
		*/
		STRUCT_ASSIGN_MACRO(vtree->pst_rangetab[rgno]->pst_rngvar,
			tabid);

		/*
		** recall that rgno contains the offset into the view range
		** table
		*/
		status = pst_rgrent(sess_cb, rngvar, &tabid, (i4) 0, err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    /* If table could not be found return a user error
		    ** so that things can be retried.
		    */
		    if (err_blk->err_code == E_PS0903_TAB_NOTFOUND)
		    {
			(VOID) psf_error(2120L, 0L, PSF_USERERR,
			    &err_code, err_blk, 0);
		    }
		    /*
		    ** In star if local schema has changed, report user error
		    ** (b54663).
		    */
		    if (err_blk->err_code == E_PS0913_SCHEMA_MISMATCH)
		    {
			(VOID) psf_error(E_PS0913_SCHEMA_MISMATCH, 0L, 
			    PSF_USERERR, &err_code, err_blk, 1, 
			    psf_trmwhite(DB_TAB_MAXNAME, (char *) rngvar->pss_rgname), 
				rngvar->pss_rgname);
		    }
		    goto exit;
		}

		/* don't update system catalogs through a view */
		if (   (emode != PSQ_RETRIEVE)
		    && (emode != PSQ_RETINTO)
		    && (emode != PSQ_DGTT_AS_SELECT)
		    && (rngvar->pss_tabdesc->tbl_status_mask &
			(DMT_CATALOG | DMT_IDX)
		       )
		    && !(sess_cb->pss_ses_flag & PSS_CATUPD)
		   )
		{
		    psy_error(3380L, -1 , rngvar, err_blk);
		    status = E_DB_ERROR;
		    goto exit;
		}

		/* 
		** restore the pss_var_mask - it was cleared by pst_showtab();
		** we do not need to preserve the PSS_REFERENCED_THRU_SYN_IN_DBP
		** bit since the current entry was CLEARLY not referenced by the
		** user through a synonym
		*/
		rngvar->pss_var_mask = 
		    save_var_mask & ~PSS_REFERENCED_THRU_SYN_IN_DBP;

		/*
		** if the view defined on top of this table or view is a target
		** of an UPDATE, INSERT, or DELETE, and there are rules defined
		** on it, remember that we have to check them
		*/
		if (   update_op
		    && rngvar->pss_tabdesc->tbl_status_mask & DMT_RULE)
		{
		    rngvar->pss_var_mask |= PSS_CHECK_RULES;
		}

		map[rgno] = rngvar->pss_rgno;

		/*
		** note that pst_rgrent() reuses the slot and does NOT
		** change the range variable number.  in particular it means
		** that the PSY_VIEWINFO structure corresponding to the
		** underlying relation has already been allocated.
		*/

		status = psy_newvar(sess_cb, rngvar, &cur_viewinfo,
		    &permflag, mstream, qmode, err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    goto exit;
		}

		check_perms = (check_perms || permflag);

		/*
		** reconstruct range variable name from the view
		** definition; this may be later copied into PST_RNGENTRY
		** structure and used by OPF when displaying QEP-related
		** info
		*/

		MEcopy((PTR) &corr_name, DB_TAB_MAXNAME,
		       (PTR) rngvar->pss_rgname);

		/*
		** if processing DECLARE CURSOR, this table/view will be an
		** underlying table/view of a cursor; we need to build a
		** PSC_TBL_DESCR for it and attach it to the cursor CB
		*/
		if (qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
		{
		    PSC_TBL_DESCR       *crs_tbl_descr;
		    
		    status = psf_umalloc(sess_cb,
			cursor->psc_stream, sizeof(*crs_tbl_descr),
			(PTR *) &crs_tbl_descr, err_blk);
		    if (DB_FAILURE_MACRO(status))
			goto exit;

		    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid,
					crs_tbl_descr->psc_tabid);
		    crs_tbl_descr->psc_tbl_mask =
			rngvar->pss_tabdesc->tbl_status_mask;

		    crs_tbl_descr->psc_row_lvl_usr_rules =
		    crs_tbl_descr->psc_row_lvl_sys_rules =
		    crs_tbl_descr->psc_stmt_lvl_usr_rules = 
		    crs_tbl_descr->psc_stmt_lvl_sys_rules =
		    crs_tbl_descr->psc_row_lvl_usr_before_rules =
		    crs_tbl_descr->psc_row_lvl_sys_before_rules =
		    crs_tbl_descr->psc_stmt_lvl_usr_before_rules = 
		    crs_tbl_descr->psc_stmt_lvl_sys_before_rules =
			(PST_STATEMENT *) NULL;

		    crs_tbl_descr->psc_flags = 0;
		    QUinsert(&crs_tbl_descr->psc_queue,
			cursor->psc_tbl_descr_queue.q_prev);
		}
	    }
	    else
	    {
		/* this wasn't the result view */
		view_rgvar = (PSS_RNGTAB *) NULL;   
	    }

	    /*
	    ** Now that we are done with processing range vars involved in the
	    ** view definition, increase dup_rb.pss_num_joins by the number of
	    ** joins found in the view definition
	    */
	    dup_rb.pss_num_joins += vtree->pst_numjoins;
	    
	    /* Now re-map all range variables in view tree */
	    status = psy_mapvars(tree_copy, map, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }

	    /* And do the same for parse trees of contained derived tables. */
	    for (i = -1; (i = BTnext(i, (char *)&viewmap, PST_NUMVARS)) >= 0; )
	    {
		if (vtree->pst_rangetab[i]->pst_rgtype != PST_DRTREE)
		    continue;		/* only for derived tables */
		status = psy_mapvars(der_tab[i], map, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }


	    /* the following code was added to fix bug (forgot the number) */
	    if (build_nmv_attmap)
	    {
		register PSS_RNGTAB	    *cur_rgvar;
		i4			    non_mergeable_views[PST_NUMVARS];
		i4			    nmv_count = 0;

		/*
		** build a map of attributes of the non-mergeable view V
		** (if the view which we just finished processing was itself a
		** non-mergeable view, V refers to that view; otherwise, V
		** refers to a non-mergeable view in definition of which the
		** view which we just finished processing was used, directly or
		** indirectly) which are used in the main query;
		** (note that this non-mergeable view V may itself be used
		** in definition of another non-mergeable view V' in which
		** case we first need to get the map of attributes of V'
		** which are used in the main query, and then translate them
		** to attributes of V)
		*/

		/*
		** first we build a list of "interesting" non-mergeable
		** views as follows:
		**	    Let V1 refer to the view which we just finished
		**	    processing
		**
		**	    0) if V1 is non-mergeable, mark it as CURRENT;
		**	       otherwise mark the non-mergeable view in
		**	       definition of which V1 was used (hint: its range
		**	       number is in rngvar->pss_rgparent) as CURRENT
		**	    1) IF (CURRENT is NULL   OR
		**	           a map of attributes of CURRENT which are used
		**		   (explicitly or implicitly) in the main query
		**		   has been allocated
		**		  )
		**	       THEN
		**		    we are done
		**	       ENDIF
		**	    2) add CURRENT view to the list, allocate space
		**	       for the map of its attributes which are used
		**	       (explicitly or implicitly) in the main query,
		**	       and initialize it
		**	    3) IF (parent of CURRENT view is itself a
		**	           non-mergeable view
		**		  )
		**	       THEN
		**		    label parent of the CURRENT view as CURRENT;
		**	       ELSE
		**		    set CURRENT to NULL
		**	       ENDIF
		**	       goto (1)
		*/
		
		for (
		     /* STEP 0 */
		     cur_rgvar = (mergeable)
		         ? rngtab->pss_rngtab + vn_map[rngvar->pss_rgparent]
		         : rngvar,
		     i = cur_rgvar->pss_rgno;

		     /* STEP 1 */
		     (cur_rgvar != (PSS_RNGTAB *) NULL &&
		      viewinfo[i]->psy_attr_map == (PSY_ATTMAP *) NULL
		     )
		     ;
		     /* STEP 3 */
		     cur_rgvar =
			((i = cur_rgvar->pss_rgparent) != -1)
			    ? rngtab->pss_rngtab + vn_map[i]
			    : (PSS_RNGTAB *) NULL
		    )
		{
		    /* STEP 2 */
		    status = psf_malloc(sess_cb, mstream, (i4) sizeof(PSY_ATTMAP),
			(PTR *) &viewinfo[cur_rgvar->pss_rgno]->psy_attr_map,
			err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			goto exit;
		    }

		    /* zero out the new mask */
		    psy_fill_attmap(
			viewinfo[cur_rgvar->pss_rgno]->psy_attr_map->map,
			(i4) 0);

		    /*
		    ** add this view to our list of "interesting" 
		    ** non-mergeable views
		    */
		    non_mergeable_views[nmv_count++] = cur_rgvar->pss_rgno;
		}

		/*
		** next we process the list of "interesting" non-mergeable
		** views; note that we process it starting with the last
		** element added to the list.
		**
		**	    FOR each element V of the list from LAST to FIRST
		**	    DO
		**		label V as CURRENT
		**		IF (CURRENT was not itself used in another
		**	            non-mergeable view [i.e. pss_rgparent == -1]
		**		   )
		**	       THEN
		**		   scan the main query tree and build a map of
		**		   attributes of CURRENT used in the main query;
		**	       ELSE
		**		   let PARENT be the view which has
		**		   pss_rgno==CURRENT->pss_rgparent
		**		   FOR (each attribute A of PARENT used in the
		**		        main query
		**			[as specified by PARENT's psy_attr_map]
		**		       )
		**		   DO
		**		       add attributes of CURRENT used in the
		**		       qualification of PARENT to the map
		**		       add attributes of CURRENT used in
		**		       definition of A to a map
		**		   ENDDO
		**	       ENDIF
		**	    ENDDO
		*/

		for (; nmv_count-- != 0; )
		{
		    cur_rgvar = rngtab->pss_rngtab +
			vn_map[non_mergeable_views[nmv_count]];

		    if (cur_rgvar->pss_rgparent == -1)
		    {
			/*
			** scan the main query tree for attributes of
			** CURRENT view
			*/
			psy_nmv_map(root, cur_rgvar->pss_rgno,
			    (char *)
			      viewinfo[cur_rgvar->pss_rgno]->psy_attr_map);
		    }
		    else
		    {
			PSS_RNGTAB	    *parent =
					    rngtab->pss_rngtab +
					    vn_map[cur_rgvar->pss_rgparent];

			status = psy_translate_nmv_map(parent,
			    (char *)
				viewinfo[parent->pss_rgno]->psy_attr_map,
			    cur_rgvar->pss_rgno,
			    (char *)
			       viewinfo[cur_rgvar->pss_rgno]->psy_attr_map,
			       psy_nmv_map, err_blk);

			if (DB_FAILURE_MACRO(status))
			{
			    goto exit;
			}
		    }
		}	/* process the list of "interesting" views */
	    }   /* if (build_nmv_attmap) */

	    /*
	    ** If the view was not mergeable, we have already copied the view
	    ** definition into QSF and marked the range entry appropriately.
	    ** It will be left to OPF to perform the variable substitution
	    */
	    if (!mergeable)
	    {
		continue;
	    }

	    /* Scan view replacing VAR nodes, pass the true query mode */
	    status = psy_subsvars(sess_cb, &r, rngvar,
		tree_copy->pst_left, PSQ_VIEW, tree_copy->pst_right, 
		view_rgvar, &tree_copy->pst_sym.pst_value.pst_s_root.pst_tvrm,
		qmode, (DB_CURSOR_ID *) NULL, &found, &subsvar_dup_rb);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }
	
	    /* replace the range variable with its view definition
	    ** by removing the range variable from the range table.
	    ** we have already added the view definition which replaces
	    ** the range variable. mark the range variable as unused.
	    */
	    if (!sess_cb->pss_resrng || 
		sess_cb->pss_resrng->pss_rgno != rngvar->pss_rgno)
	    {
		rngvar->pss_used = FALSE;
		QUremove((QUEUE*) rngvar);
		QUinsert((QUEUE*) rngvar, rngtab->pss_qhead.q_prev);
	    }

	} /* for (vn = 0; ; vn++) */

	/*
	** If there are some views for which we have to check permissions
	** before continuing to unravel them, do it now
	*/
	if (check_perms)
	{
	    status = psy_protect(sess_cb, rngtab, root, qmode, err_blk,
		mstream, &dup_rb.pss_num_joins, viewinfo);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }

	    /*
	    ** if we were processing a definition of a new view
	    ** (qmode == PSQ_VIEW), the view is clearly not grantable
	    */
	    if (qmode == PSQ_VIEW && *resp_mask & PSY_VIEW_IS_GRANTABLE)
	    {
		*resp_mask &= ~PSY_VIEW_IS_GRANTABLE;
	    }
	}

	/*
	** If there are some views which need to be audited, do it now
	** before unraveling them any further.
	*/
	if (build_audit)
	{
	    status = psy_build_audit_info(sess_cb, 
			rngtab, qmode, mstream, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }
	}

	/*
	** if the table or view described by sess_cb->pss_resrng (or a view
	** defined on top it) is a target of DELETE, INSERT, or UPDATE, and
	** there are rules defined on it, and we haven't already checked if any
	** of them apply, do it now 
	*/
	if (update_op && sess_cb->pss_resrng->pss_var_mask & PSS_CHECK_RULES)
	{
	    status = psy_rules(sess_cb, qmode, root, err_blk);
	    if (DB_FAILURE_MACRO(status))
		goto exit;

	    /*
	    ** remember that we have checked whether any of the rules apply to
	    ** the table or view being updated - we do not want to do it more
	    ** than once
	    */
	    sess_cb->pss_resrng->pss_var_mask &= ~PSS_CHECK_RULES;
	}

	/*
	** only tables and views which appear in the view definition should be
	** considered for insertion into the list of independent objects - after
	** the first pass through the range variable list, we should not try to
	** add any more elements to the independent object list
	*/
	if (build_indep_obj_list)
	{
	    build_indep_obj_list = FALSE;
	}
    } /* for (viewfound = TRUE; viewfound; ) */

    /*
    ** if we are processing a definition of a view or a dbproc for which we 
    ** need to find the underlying base table id (ubt_id), and none has been 
    ** found so far, we will scan the range table for the last time looking for
    ** entries representing views for which we have not determined that their 
    ** trees would not contain ids of their underlying base tables; for each 
    ** such view, we will invoke psy_ubtid() to determine whether its tree 
    ** contains a ubt_id
    ** if we haven't determined ubt_id yet, a given range entry describing a 
    ** view will not have PSS_NO_UBT_ID bit set iff it describes a 
    ** (always-grantable if creating an SQL view) view owned by the current user
    */
    for (vn = 0, rngvar = rngtab->pss_rngtab;
	 ubt_id && !ubt_id->db_tab_base && vn < PST_NUMVARS;
	 vn++, rngvar++)
    {
	if (   rngvar->pss_used
	    && rngvar->pss_rgno != -1
	    && rngvar->pss_rgtype == PST_TABLE
	    && rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW
	    && ~rngvar->pss_var_mask & PSS_NO_UBT_ID)
	{
	    status = psy_ubtid(rngvar, sess_cb, err_blk, ubt_id);
	    if (DB_FAILURE_MACRO(status))
		goto exit;
	}
    }

exit:
    /*
    ** if the list of independent objects is non-empty and everything looks
    ** good, copy the list into sess_cb->pss_indep_objs
    */
    if (   (indep_obj_list_size || indep_syn_list_size) 
	&& DB_SUCCESS_MACRO(status))
    {
	PSQ_OBJ		*objlist = sess_cb->pss_indep_objs.psq_objs;

	if (indep_obj_list_size)
	{
	    /*
	    ** remember that PSQ_OBJ.psq_dbp_name is only used when the object
	    ** descriptor represents a dbproc, so we can reduce the amount of 
	    ** memory being allocated by sizeof(DB_DBP_NAME); similarly, 
	    ** PSQ_OBJ.psq_2id is used when the object descriptor represents a 
	    ** constraint, so we can also reduce the amount of memory being 
	    ** allocated by sizeof(u_i4)
	    */
	    status = psf_malloc(sess_cb, sess_cb->pss_dependencies_stream,
	        (i4) (sizeof(PSQ_OBJ) + 
		       sizeof(DB_TAB_ID) * indep_obj_list_size -
		       sizeof(DB_TAB_ID) - sizeof(DB_DBP_NAME) - sizeof(u_i4)),
	        (PTR *) &sess_cb->pss_indep_objs.psq_objs, err_blk);
    
	    if (DB_SUCCESS_MACRO(status))
	    {
	        DB_TAB_ID	    *idfrom, *idto;
    
	        /* append the rest of the list to the new entry */
	        sess_cb->pss_indep_objs.psq_objs->psq_next = objlist;
    
	        /* populate the new entry */
	        objlist = sess_cb->pss_indep_objs.psq_objs;
	        objlist->psq_objtype = PSQ_OBJTYPE_IS_TABLE;
	        objlist->psq_num_ids = indep_obj_list_size;
    
	        for (i = 0, idfrom = indep_ids, idto = objlist->psq_objid;
		     i < indep_obj_list_size;
		     i++, idfrom++, idto++
		    )
	        {
		    idto->db_tab_base = idfrom->db_tab_base;
		    idto->db_tab_index = idfrom->db_tab_index;
	        }
	    }
	}

	if (indep_syn_list_size)
	{
	    /*
	    ** remember that PSQ_OBJ.psq_dbp_name is only used when the object
	    ** descriptor represents a dbproc, so we can reduce the amount of 
	    ** memory being allocated by sizeof(DB_DBP_NAME); similarly, 
	    ** PSQ_OBJ.psq_2id is used when the object descriptor represents a 
	    ** constraint, so we can also reduce the amount of memory being 
	    ** allocated by sizeof(u_i4)
	    */
	    status = psf_malloc(sess_cb, sess_cb->pss_dependencies_stream,
	        (i4) (sizeof(PSQ_OBJ) + 
		       sizeof(DB_TAB_ID) * indep_syn_list_size -
		       sizeof(DB_TAB_ID) - sizeof(DB_DBP_NAME) - sizeof(u_i4)),
	        (PTR *) &sess_cb->pss_indep_objs.psq_objs, err_blk);
    
	    if (DB_SUCCESS_MACRO(status))
	    {
	        DB_TAB_ID	    *idfrom, *idto;
    
	        /* append the rest of the list to the new entry */
	        sess_cb->pss_indep_objs.psq_objs->psq_next = objlist;
    
	        /* populate the new entry */
	        objlist = sess_cb->pss_indep_objs.psq_objs;
	        objlist->psq_objtype = PSQ_OBJTYPE_IS_SYNONYM;
	        objlist->psq_num_ids = indep_syn_list_size;
    
	        for (i = 0, idfrom = indep_syn_ids, idto = objlist->psq_objid;
		     i < indep_syn_list_size;
		     i++, idfrom++, idto++
		    )
	        {
		    idto->db_tab_base = idfrom->db_tab_base;
		    idto->db_tab_index = idfrom->db_tab_index;
	        }
	    }
	}
    }

    *num_joins = dup_rb.pss_num_joins;
    return (status);
}

/*{
** Name: psy_vrscan	- scan query tree and replace RESDOM nodes
**
** Description:
**	The query tree issued is scanned and RESDOM nodes are
**	converted to conform to the underlying base relations.
**	There are many checks in here, and things can fail
**	easily.
**
**	For all queries except DELETE and OPEN CURSOR, the target list 
**	of the query submitted is scanned down the left hand side (the 
**	RESDOM list). For each RESDOM, that variable is looked up in 
**	the view definition.  If the definition of it is not a simple
**	attribute, the query is aborted.
**
**	Then, if the variable appears anywhere in the qualification
**	of the view, the query is aborted.
**
**	Finally, we make sure that the result range variable doesn't change
**	into two range variables (e.g. the user is trying to update a two-
**	variable view).
**
**	And as the last step, we actually change the 'pst_resno' and
**	'pst_ntargno' for this RESDOM.
**
**	Notice that there are a number of overly restrictive
**	conditions on runability.  Notably, there are large classes
**	of queries which can run consistently but which violate
**	either the not-in-qualification condition or the aggregate-
**	free condition.
**
** Inputs:
**	with_check			If TRUE, don't allow updates which
**					may become invisible.
**      root                            Root of the tree to be updated
**	vtree				Root of the tree which defines the view
**	qmode				Query mode of user's query
**	resvar				Result range variable entry
**	rgno				Result range var. no.
**	err_blk				Filled in if an error happens
**
** Outputs:
**      root                            RESDOMS modified to conform to view
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-jun-86 (jeff)
**          written
**	27-aug-87 (stec)
**          Determine result relation for OPEN/DEFINE CURSOR and exit.
**	04-jan-88 (stec)
**          Handle result range var for OPEN CURSOR in PSY_VIEW.
**	23-nov-88 (stec)
**	    Change psy_vrscan to modify pst_ntargno.
*/

DB_STATUS
psy_vrscan(
	i4	    with_check,
	PST_QNODE   *root,
	PST_QNODE   *vtree,
	i4	    qmode,
	PSS_RNGTAB  *resvar,
	i4	    *rgno,
	DB_ERROR    *err_blk)
{
    register PST_QNODE		*t;
    register PST_QNODE		*v;
    i4			i;
    PST_QNODE			*p;
    i4			err_code;
    DB_STATUS			status;

    t = root;
    v = vtree;

    /* scan target list of query */
    i = -1;
    while ((t = t->pst_left) && t->pst_sym.pst_type != PST_TREE)
    {
	if (t->pst_sym.pst_type != PST_RESDOM)
	{
	    (VOID) psf_error(E_PS0D0B_RESDOM_EXPECTED, 0L, PSF_INTERR,
		&err_code, err_blk, 0);
	    return (E_DB_SEVERE);
	}

	/* check for 'tid' attribute */
	if (t->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 0)
	    continue;

	/* find definition for this domain in the view */
	status = psy_vfind((u_i2) t->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
	    v->pst_left, &p, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	*rgno = p->pst_sym.pst_value.pst_s_var.pst_vno;

	/* check for simple attribute */
	if (p->pst_sym.pst_type != PST_VAR || 
	    p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == 0)
	{
	    /* Attempt to update expression */
	    psy_error(3310L, qmode, resvar, err_blk);
	    return (E_DB_ERROR);
	}

	/* scan qualification of view for this attribute */
	if (with_check && psy_qscan(v->pst_right, 
	    (i4) p->pst_sym.pst_value.pst_s_var.pst_vno,
	    (u_i2) p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id) !=
		(PST_QNODE *) NULL)
	{
	    /* Attempt to update column in qualification */
	    psy_error(3320L, qmode, resvar, err_blk);
	    return (E_DB_ERROR);
	}

	/* check for trying to do update on two relations again */
	/* this test should only be true for REPLACE commands */
	if (i < 0)
	{
	    i = p->pst_sym.pst_value.pst_s_var.pst_vno;
	}
	else if (i != p->pst_sym.pst_value.pst_s_var.pst_vno)
	{
	    /* Update of multi-variable view */
	    psy_error(3330L, qmode, resvar, err_blk);
	    return (E_DB_ERROR);
	}

	/* finally, do the substitution of resno's and pst_ntargno's */
	{
	    register PST_RSDM_NODE  *rsdm;

	    rsdm = &t->pst_sym.pst_value.pst_s_rsdm;

    	    rsdm->pst_rsno =
		p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;

	    if (rsdm->pst_rsflags&PST_RS_PRINT &&
		(rsdm->pst_ttargtype == PST_USER
		 ||
		 rsdm->pst_ttargtype == PST_ATTNO)
	       )
	    {
		rsdm->pst_ntargno = rsdm->pst_rsno;
	    }
	}
    }

    return (E_DB_OK);
}

/*
** Name: psy_newvar - if the new range variable introduced as a result of
**		      expanding a view is itself a view, we may need to allocate
**		      the PSY_VIEWINFO structure and initialize it as apprpriate
**
** Input:
**	    sess_cb		session CB
**	    rngvar		new reange variable
**	    cur_viewinfo	address of a ptr to a view info structure
**				corresponding to the new range variable; if the
**				new range var represents a view, we may need to
**				allocate and initialize it.
**	    mstream		stream from which to allocate memory
**	    qmode		actual query mode
** Output:
**	    check_perms		set to TRUE if we have to determine if the user
**				has required permissions form this relation;
**				FALSE otherwise
**	    err_blk		Filled in if an error happened
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** History:
**	14-sep-90 (andre)
**	    written (in conjunction with fix for bug (forgot the number))
*/
static DB_STATUS
psy_newvar(
	PSS_SESBLK	*sess_cb,
	PSS_RNGTAB	*rngvar,
	PSY_VIEWINFO	**cur_viewinfo,
	bool		*check_perms,
	PSF_MSTREAM	*mstream,
	i4		qmode,
	DB_ERROR	*err_blk)
{
    DB_STATUS	    status;
    bool	    is_owner;	/*
				** TRUE <==> table is owned by the current user
				*/

    is_owner = (MEcmp((PTR) &rngvar->pss_tabdesc->tbl_owner,
		      (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME)) == 0);

    if (rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
    {
	i4		issql = 0;

	/* if a view descriptor structure has not been allocated, do it now */
	if (*cur_viewinfo == (PSY_VIEWINFO *) NULL)
	{
	    status = psf_malloc(sess_cb, mstream, (i4) sizeof(PSY_VIEWINFO),
		(PTR *) cur_viewinfo, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		return(status);
	    }

	    (*cur_viewinfo)->psy_attr_map = (PSY_ATTMAP *) NULL;
	}

	/* is this an sql view ? */
	status = psy_sqlview(rngvar, sess_cb, err_blk, &issql);
	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}

	(*cur_viewinfo)->psy_flags = 0;
	if (issql)
	{
	    (*cur_viewinfo)->psy_flags |= PSY_SQL_VIEW;
	}
    }
    else
    {
	if (*cur_viewinfo != (PSY_VIEWINFO *) NULL)
	{
	    /* range var with number rngvar->pss_rgno is not a view */
	    (*cur_viewinfo)->psy_flags = PSY_NOT_A_VIEW;
	}

	/*
	** if
	**	- this is a base table owned by the current user and
	**	- pasrsing DECLARE CURSOR and
	**	- this is the relation over which the cursor is being
	**	  declared/defined
	** then
	**	user will be allowed to delete tuples using this cursor
	** endif
	*/
	if (is_owner && (qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
			&& rngvar == sess_cb->pss_resrng)
	{
	    sess_cb->pss_crsr->psc_delall = TRUE;

	    /*
	    ** if defining a QUEL cursor, remember that the user may retrieve
	    ** every column of the table/view over which the cursor is being
	    ** defined
	    */
	    if (sess_cb->pss_lang == DB_QUEL)
		sess_cb->pss_crsr->psc_flags |= PSC_MAY_RETR_ALL_COLUMNS;
	}
    }

    /*
    ** check if we need to check if the user has the required
    ** permissions
    ** (i.e. if ((this is an SQL view OR a base table) AND
    **           it is not owned by the current user,  AND
    **		 we are yet to determine if the user may access
    **		 it
    **		)
    ** )
    */
    *check_perms =
		    (rngvar->pss_permit
		     &&
		     (
		      *cur_viewinfo == (PSY_VIEWINFO *) NULL	    ||
		      (*cur_viewinfo)->psy_flags == PSY_NOT_A_VIEW  ||
		      (*cur_viewinfo)->psy_flags & PSY_SQL_VIEW
		     )
		     &&
		     !is_owner
		    );

    return(E_DB_OK);
}

/*
** Name:    psy_nmv_map() - build a reversed map of attributes of a
**			    non-mergeable view found in a given tree;
**
** Description:	build a map of attributes of a non-mergeable view used in the
**		tree; this function produces a map s.t.
**		i-th bit is turned on <==> (DB_MAX_COL-i)-th attribute was found
**		reversing the map makes the process of searching the tree of the
**		non-mergeable view for attributes of a given relation more
**		efficient.
**
** Input:
**	    t		- tree to be searched
**	    rgno	- number of the range variable corresponding to the
**			  non-mergeable view
**	    nmv_map	- attribute map which has been properly initialized
**
** Output
**	    nmv_map	- map of attributes of non-mergeable view will be filled
**			  in as described above
**
** Returns:
**	none
**
** History:
**	17-sep-90 (andre)
**	    written
*/
static VOID
psy_nmv_map(
	PST_QNODE	*t,
	i4		rgno,
	char		*nmv_map)
{
    if (t != (PST_QNODE *) NULL)
    {
	if (t->pst_sym.pst_type == PST_VAR)
	{
	    if (t->pst_sym.pst_value.pst_s_var.pst_vno == rgno)
	    {
		BTset((DB_MAX_COLS -
		       t->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id),
		      nmv_map);
	    }
	}
	else
	{
	    if (t->pst_left != (PST_QNODE *) NULL)
	    {
		psy_nmv_map(t->pst_left, rgno, nmv_map);
	    }
	    
	    if (t->pst_right != (PST_QNODE *) NULL)
	    {
		psy_nmv_map(t->pst_right, rgno, nmv_map);
	    }
	}
    }
    
    return;
}

/*
** Name:    psy_translate_nmv_map - translate the map of attributes of a
**				    non-mewrgeable view into a map of attributes
**				    of a given relation used in definition of
**				    the view + scan qualification of the view
**				    for more attributes of the relation in
**				    question
** Description:
**	given a range table entry for a non-mergeable view V and a relation R
**	used in the definition of V, this function will translate a map of
**	attributes of V used in the main query into a map of attributes of R by
**	walking the qualification tree of V and the subtrees corresponding to
**	the attributes of V mentioned in the map
**
** Input:
**	parent		range table entry for the non-mergeable view
**	parent_attrmap	map of attributes of non-mergeable view used in the
**			main query
**	rel_rgno	range number of the relation for which a map of
**			attributes has to be built
**	rel_attrmap	preinitialized map of attributes of relation R
**	treewalker	function to be used to walk trees insearch of attributes
**			of a given relation; we pass the function address
**			because we may want to build a reversed or a usual
**			attribute map (the former is done when building a map of
**			attributes of a non-mergeable view, the latter - when
**			building a map of attributes for a relation used in the
**			definition of a non-mergeable view in conjunction with
**			checking if the user has the required permissions to
**			access a given relation.
** Output:
**	rel_attrmap	map of attributes of relation R (see above) used in the
**			main query
**	err_blk		will be filled in if an error is encountered
**
** Returns:
**	    E_DB_OK, E_DB_SEVERE
**
** History:
**	17-sep-90 (andre)
**	    written
**	14-apr-92 (andre)
**	    fix bug 43640: at the time I wrote this function I have mistakenly
**	    assumed that ROOT's pst_tvrm reflects all range variables used in
**	    the query tree.  while this is true in QUEL, in SQL it is patently
**	    false.  Instead of looking at ROOT's pst_tvrm, we will scan the tree
**	    until we find a ROOT or PST_SUBSEL node with pst_tvrm indicating
**	    that a given range var was used in the tree rooted in the node
**	11-aug-92 (andre)
**	    one good bug deserves another - fix for bug 43640 introduced another
**	    bug - will I ever learn?..
**	    If a relation is used in a subselect inside of view's
**	    qualification, we should search that subselect's tree as a 
**	    whole as opposed to searching its (i.e. subselect tree's)
**	    qualification and then trying to map its target list which quite
**	    likely has nothing to do with the target list of the view tree
**	11-jun-93 (andre)
**	    OK, let's see if I get it right this time: until now this function
**	    failed to adequately address cases when a view is itself (or is
**	    based on) a QUEL view with aggregates.  AGGHEAD in a QUEL view has
**	    its own pst_tvrm and it is different from pst_tvrm in the root node.
**	    To fix this problem we must give up the assumption that pst_tvrm's
**	    are to be found only in PST_ROOT, root-level PST_SUBSELs and
**	    PST_SUBSELs inside the qualification
**	    To fix this problem, we will get rid of some shortcuts that relied
**	    on usage of pst_tvrm in SQL views and start looking for pst_tvrm's
**	    all over the view tree
**
**	    The resulting algorithm will be as follows:
**		1) check pst_tvrm of the ROOT node and, if the parent view is a
**		   UNION view, of other root-level PST_SUBSEL nodes; if a bit
**		   corresponding to our range var is set in one of those maps,
**		   we will scan the qualification of the tree + subtrees
**		   corresponding to the attributes of the parent view which are
**		   used in the query looking for attributes of the range
**		   variable whose range number was given to us;
**		2) if the first step failed to locate a reference to the range
**		   var whose range number was passed by the caller, we will call
**		   psy_rgvar_in_subtree() to look for PST_ROOT, PST_SUBSEL, or
**		   PST_AGGHEAD node whose pst_tvrm includes out range var; if
**		   such node is found, we will scan its subtrees looking for
**		   references to attributes of the range var whose range number
**		   was given to us
**		3) finally, if the above two steps failed (which, I think, can
**		   happen if the parent is a QUEL view or some of its underlying
**		   views are QUEL views), we will call psy_rgvar_in_subtree() to
**		   examine subtrees of attributes of the parent view looking for
**		   PST_AGHEAD node whose pst_tvrm includes our range var;
**		   once such node is found, if the attribute in whose subtree it
**		   was found was referenced in the query, we will scan its
**		   subtrees looking for references to attributes of the range
**		   var whose range number was given to us
**
**		We will try to optimize our search for pst_tvrm mentioning the
**		range var whose number was given to us for views which do not
**		involve QUEL attributes in their target lists (which I hope
**		represents the majority of views.)  Accordingly, rather than
**		check pst_tvrm of PST_ROOT, then search the qualification, then
**		search the target list for every root-level PST_SUBSEL node, we
**		will perform step (1) for PST_ROOT and all root-level PST_SUBSEL
**		nodes, then step (2) and step (3).  
**	10-aug-93 (andre)
**	    fixed the cause of a compiler warning
*/
DB_STATUS
psy_translate_nmv_map(
	PSS_RNGTAB	*parent,
	char		*parent_attrmap,
	i4		rel_rgno,
	char		*rel_attrmap,
	VOID		(*treewalker)(PST_QNODE*, i4, char*),
	DB_ERROR	*err_blk)
{
    register PST_QNODE	    *qry_term;
    PST_QNODE		    *search_tree = (PST_QNODE *) NULL;

    /* step (1) */

    /* if the view is a UNION view, we must check all terms */
    for (qry_term = parent->pss_qtree;
	 qry_term != (PST_QNODE *) NULL;
	 qry_term = qry_term->pst_sym.pst_value.pst_s_root.pst_union.pst_next
	)
    {
	/*
	** check if R is used in this subtree (outside of any subselect or
	** QUEL-style aggregate)
	*/
	if (BTtest(rel_rgno,
		  (char *) &qry_term->pst_sym.pst_value.pst_s_root.pst_tvrm))
	{
	    register i4	    cur_attr, next_attr;

	    /*
	    ** we are guaranteed to have the attribute map for PARENT; 
	    ** first we will walk the PARENT's qualification, then we will walk
	    ** the subtrees representing attributes appearing in the PARENT's
	    ** mask;
	    ** Note that in the PARENT view tree, subtrees representing
	    ** attributes appear in the backward order (i.e.
	    ** root->pst_left->pst_right is the subtree representing the last
	    ** attribute).  Fortunately psy_nmv_map() builds a reversed map of
	    ** attributes (i.e. bit n is set iff attribute DB_MAX_COLS-n was
	    ** encountered), so we will avoid O(n2) complexity algorithm.
	    */

	    search_tree = qry_term;

	    /*
	    ** first scan PARENT's qualification subtree for attributes of R
	    */
	    (*treewalker) (search_tree->pst_right, rel_rgno, rel_attrmap);

	    /*
	    ** next, examine definitions of attributes of PARENT used in the
	    ** main query tree for attributes of R.  Recall that parent_attrmap
	    ** was built in reverse
	    */

	    cur_attr = DB_MAX_COLS - parent->pss_tabdesc->tbl_attr_count - 1;
	    
	    for (next_attr = BTnext(cur_attr, parent_attrmap, DB_MAX_COLS);
	         (next_attr != -1 && next_attr != DB_MAX_COLS);
		 next_attr = BTnext(next_attr, parent_attrmap, DB_MAX_COLS)
		)
	    {
		/*
		** get ptr to the subtree describing the "next" attribute of
		** PARENT;
		*/
		for (; cur_attr < next_attr; cur_attr++)
		{
		    search_tree = search_tree->pst_left;
		}
		
		/*
		** now search the attribute subtree for attributes of CURRENT
		*/
		(*treewalker) (search_tree->pst_right, rel_rgno, rel_attrmap);
	    }

	    return(E_DB_OK);
	}
    }

    /* step (2) */

    /* if the view is a UNION view, we must check all terms */
    for (qry_term = parent->pss_qtree;
	 qry_term != (PST_QNODE *) NULL;
	 qry_term = qry_term->pst_sym.pst_value.pst_s_root.pst_union.pst_next
	)
    {
	/*
	** check if R is used in a subselect or QUEL-style aggregate inside the
	** qualification
	*/
	search_tree = psy_rgvar_in_subtree(qry_term->pst_right, rel_rgno);
	
	if (search_tree)
	{
	    /*
	    ** search the subselect tree for attributes of CURRENT
	    ** (part of fix for bug 45011)
	    */
	    
	    (*treewalker) (search_tree, rel_rgno, rel_attrmap);
	    return(E_DB_OK);
	}
    }

    /* step (3) */

    /* if the view is a UNION view, we must check all terms */
    for (qry_term = parent->pss_qtree;
	 qry_term != (PST_QNODE *) NULL;
	 qry_term = qry_term->pst_sym.pst_value.pst_s_root.pst_union.pst_next
	)
    {
	PST_QNODE	*rsdm;
	i4		cur_attr;
	
	/*
	** check if R is used in a QUEL aggregate in the target list
	** if we find it in a subtree describing the attribute of the parent
	** view which is used in the main query, we will scan aggregate's
	** subtree looking for attributes of R; otherwise we simply return
	*/

	for (rsdm = qry_term->pst_left,
	     cur_attr = parent->pss_tabdesc->tbl_attr_count;

	     cur_attr > 0;

	     rsdm = rsdm->pst_left, cur_attr--
	   )
	{
	    if (search_tree = psy_rgvar_in_subtree(rsdm->pst_right, rel_rgno))
	    {
		if (BTtest(DB_MAX_COLS - cur_attr, parent_attrmap))
		{
		    (*treewalker) (search_tree, rel_rgno, rel_attrmap);
		}

		return(E_DB_OK);
	    }
	}
    }

    /*
    ** we get here only if we found no references to the relation and that
    ** should never happen -- R would never be added to the range table if it
    ** was not mentioned in the query tree of its parent
    */
    {
	i4		err_code;

	/*
	** this should never happen -- R would never be added to the range
	** table if it was not mentioned in the query tree of its parent
	*/
	(VOID) psf_error(E_PS0C04_BAD_TREE, 0L, PSF_INTERR, &err_code,
	    err_blk, 0);
	return (E_DB_SEVERE);
    }
}

/*
** Name: psy_add_indep_obj()	- add a table/view id to the list of ids unless
**				  it has already been added to that list
**
** Description:
**	Look for the object id in the list passed by the caller; if none is
**	found and we are processing a dbproc, also check elements of independent
**	objects sublist constructed for this dbproc.  If none was found, a new
**	entry will be added.
**
** Input:
**	sess_cb				PSF session CB
**	    pss_dbp_flags		PSS_DBPROC is set if processing a dbproc
**	    pss_indep_objs		structure holding descriptors of
**					independent objects and privileges when
**					processing a dbproc
**					
**	rngvar				range variable for the table/view being
**					considered
**	    pss_tabid			id to check
**	    
**	indep_id_list			list of object ids
**	obj_type			type of object (one of
**					PSQ_OBJTYPE_IS_TABLE or
**					PSQ_OBJTYPE_IS_SYNONYM)
**	list_size			number of elements in indep_id_list
**
** Output:
**	rngvar
**	    pss_var_mask		if new id has been added to the list,
**					PSS_IN_INDEP_OBJ_LIST will be set
**					
**	indep_id_list			new id may have been added
**	
**	list_size			may be incremented if new id has been
**					added
**
** Returns:
**	none
**
** Side effects:
**	none
**
** History:
**
**	20-jan-92 (andre)
**	    written
**	08-jun-92 (andre)
**	    remember to set PSS_IN_INDEP_OBJ_LIST in rngvar->pss_var_mask even
**	    if this independent object is already in the list associated with
**	    this statement or (in the case of dbproc) in a list associated with
**	    one of the statements of the dbproc.  Otherwise, we may end up
**	    adding objects on which this object is defined to the independent
**	    object list, which is undesirable.
**	21-oct-93 (andre)
**	    this function will also be used to accumulate ids of synonyms used 
**	    in dbproc definition - th is required that I add a new parameter, 
**	    obj_type, to the function's interface
*/
static VOID
psy_add_indep_obj(
	PSS_SESBLK		*sess_cb,
	PSS_RNGTAB		*rngvar,
	DB_TAB_ID		*indep_id_list,
	i4			obj_type,
	i4			*list_size)
{
    register DB_TAB_ID	    *id;
    register DB_TAB_ID      *new_id;
    register i4	    i;

    /*
    ** unless we are adding id of a synonym (as opposed to that of a table, 
    ** view, or index), by the time we leave this function the object 
    ** represented by this entry will either have been added to the independent
    ** object list or will have been found to have been already added to the 
    ** list, it is safe to mark it as having been added to the independent 
    ** object list
    */
    if (obj_type == PSQ_OBJTYPE_IS_SYNONYM)
    {
	new_id = &rngvar->pss_syn_id;
    }
    else
    {
        rngvar->pss_var_mask |= PSS_IN_INDEP_OBJ_LIST;
	new_id = &rngvar->pss_tabid;
    }

    /*
    ** first check if the object is in the list being constructed; if so, we are
    ** done
    */
    for (i = *list_size, id = indep_id_list; i > 0; i--, id++)
    {
	if (   id->db_tab_base == new_id->db_tab_base
	    && id->db_tab_index == new_id->db_tab_index)
	{
	    /*
	    ** id didn't have to be added to the list being constructed - it is
	    ** already there - we are done
	    */
	    return;
	}
    }

    /*
    ** if processing CREATE PROCEDURE, look for this object in the blocks
    ** (if any) which were generated for the previous statement(s).
    ** 
    ** Terminate the search if encountered end of list or an end-of-sublist
    ** marker.  The latter can only occur if we are reparsing a dbproc as a part
    ** of processing GRANT ON PROCEDURE statement (a new sublist is built for
    ** each dbproc being parsed)
    */
    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
    {
	i4	    end_of_sublist = PSQ_DBPLIST_HEADER | PSQ_INFOLIST_HEADER;
	PSQ_OBJ	    *objlist;

	for (objlist = sess_cb->pss_indep_objs.psq_objs;
	     (objlist && !(objlist->psq_objtype & end_of_sublist));
	     objlist = objlist->psq_next
	    )
	{
	    if (objlist->psq_objtype != obj_type)
	    {
		/* 
		** only need to consider elements containing ids of objects of 
		** the same type (either tables or synonyms); while it is true 
		** that no table will have the same id as a synonym, this saves
		** us from traversing a list where we have no hope of ever 
		** finding a matching id
		*/
		continue;
	    }

	    for (i = objlist->psq_num_ids, id = objlist->psq_objid;
		 i > 0;
		 i--, id++
		)
	    {
		if (id->db_tab_base == new_id->db_tab_base
		    &&
		    id->db_tab_index == new_id->db_tab_index)
		{
		    /*
		    ** found id in a list built for a previously processed
		    ** dbproc statement - we are done
		    */
		    return;
		}
	    }
	}
    }
    
    id = indep_id_list + (*list_size)++;
    id->db_tab_base = new_id->db_tab_base;
    id->db_tab_index = new_id->db_tab_index;

    return;
}

/*
** Name:    psy_rgvar_in_subtree - locate SUBSEL node in whose subtree a range
**				   var with specified range numbr is used
**
** Description:
**	Traverse a tree searching for a PST_ROOT or PST_SUBSEL node
**	with pst_tvrm indicating a specified range variable is used in the
**	subtree of which it is the root.  If such node is found, its address
**	will be returned; otherwise NULL will be returned.
**	
** Input:
**	node	    root of the subtree to examine
**	rgno	    range number of the var in which we are interested
**	
** Output:
**	None
**
** Returns:
**	NULL	    node with rgno bit set in pst_tvrm has not been found 
**	otherwise   desired node has been found
**
** Side effects:
**	none
**
** Exceptions:
**	none
**
** History:
**
**	14-apr-92 (andre)
**	    written as a part of fix for bug 43640
**	11-jun-93 (andre)
**	    QUEL-style aggregates have range map of their very own.  It is not
**	    sufficient to examine pst_tvrm only in PST_ROOT and PST_SUBSEL - we
**	    must also look in PST_AGHEAD nodes.
**	    At the same time, the fact that QUEL-style aggregates carry their
**	    own pst_tvrm means that in addition to scanning qualification
**	    subtree of PST_ROOT or PST_SUBSEL, we must also scan their target lists
**	    
*/
static PST_QNODE *
psy_rgvar_in_subtree(
	register PST_QNODE	*node,
	register i4		rgno)
{
    PST_QNODE	*res = (PST_QNODE *) NULL;

    if (!node)
	return((PST_QNODE *) NULL);
	
    if (   (   node->pst_sym.pst_type == PST_ROOT
	    || node->pst_sym.pst_type == PST_SUBSEL
	    || node->pst_sym.pst_type == PST_AGHEAD)
	&& BTtest(rgno, (char *) &node->pst_sym.pst_value.pst_s_root.pst_tvrm)
       )
    {
	res = node;
    }
    else
    {
	if (node->pst_right)
	    res = psy_rgvar_in_subtree(node->pst_right, rgno);

	if (!res && node->pst_left)
	    res = psy_rgvar_in_subtree(node->pst_left, rgno);
    }

    return(res);
}

/*
** Name: psy_view_is_updatable - determine whether a view is updatable
**
** Description:
**	Examine view's header and tree to determine whether it is updatable.
**	If it appears non-updatable, an caller will be notified of the reason
**	why the view was deemed non-updatable
**
** Input:
**	tree_header		view's query tree header
**	qmode			query mode;
**
** Output
**	reason			if a view is non-updatable, this field will
**				contain an indicator of the reason;
**				can be one of:
**				    PSY_UNION_IN_OUTERMOST_SUBSEL,
**				    PSY_DISTINCT_IN_OUTERMOST_SUBSEL,
**				    PSY_MULT_VAR_IN_OUTERMOST_SUBSEL,
**				    PSY_GROUP_BY_IN_OUTERMOST_SUBSEL,
**				    PSY_AGGR_IN_OUTERMOST_SUBSEL,
**				    PSY_NO_UPDT_COLS
**				    
** Returns:
**	TRUE if a view is updatable; FALSE otherwise
**
** Side effects:
**	none
**
** History:
**	30-dec-92 (andre)
**	    written
*/
bool
psy_view_is_updatable(
	PST_QTREE	*tree_header,
	i4		qmode,
	i4		*reason)
{
    PST_RT_NODE		*root_node =
			  &tree_header->pst_qtree->pst_sym.pst_value.pst_s_root;
    PST_VRMAP		varmap;

    /* check whether a view involves UNION */
    if (root_node->pst_union.pst_next)
    {
	*reason = PSY_UNION_IN_OUTERMOST_SUBSEL;
	return((bool) FALSE);
    }

    /* check whether a view involves an aggregate outside of qualification */
    if (tree_header->pst_mask1 & PST_AGG_IN_OUTERMOST_SUBSEL)
    {
	*reason = PSY_AGGR_IN_OUTERMOST_SUBSEL;
	return((bool) FALSE);
    }
    
    /* check if a view involves GROUP BY */
    if (tree_header->pst_mask1 & PST_GROUP_BY_IN_OUTERMOST_SUBSEL)
    {
	*reason = PSY_GROUP_BY_IN_OUTERMOST_SUBSEL;
	return((bool) FALSE);
    }
    
    /* check whether a view involves DISTINCT */
    if (root_node->pst_dups != PST_ALLDUPS)
    {
	*reason = PSY_DISTINCT_IN_OUTERMOST_SUBSEL;
	return((bool) FALSE);
    }

    /*
    ** check whether the target list of the view's definition involves any of
    ** the underlying table's attributes - if not, then clearly none of the
    ** columns is updatable
    */
    psy_vcount(tree_header->pst_qtree->pst_left, &varmap);
    if (BTcount((char *) &varmap, (i4)BITS_IN(varmap)) == 0)
    {
	*reason = PSY_NO_UPDT_COLS;
	return((bool) FALSE);
    }
    
    /* check whether this is a multi-variable view */
    MEcopy((char *)&root_node->pst_tvrm, sizeof(PST_J_MASK), 
				(char *)&varmap.pst_vrmap);
    if (BTcount((char *) &varmap, BITS_IN(varmap)) != 1)
    {
	*reason = PSY_MULT_VAR_IN_OUTERMOST_SUBSEL;
	return((bool) FALSE);
    }

    /*
    ** check whether any of the view's columns are updatable - do it only for
    ** WITH CHECK OPTION processing since for other queries significance of a
    ** given column being updatable is based on the query itself; 
    */
    if (qmode == PSQ_VIEW)
    {
	PST_QNODE	*resdom;
	bool		updatable_attr = FALSE;

	for (
	     resdom = tree_header->pst_qtree->pst_left;
	     
	     (   resdom != (PST_QNODE *) NULL
	      && resdom->pst_sym.pst_type == PST_RESDOM);
	      
	     resdom = resdom->pst_left
	    )
	{
	    if (resdom->pst_right->pst_sym.pst_type == PST_VAR)
	    {
		updatable_attr = TRUE;
		break;
	    }
	}

	if (!updatable_attr)
	{
	    *reason = PSY_NO_UPDT_COLS;
	    return((bool) FALSE);
	}
    }

    /* this view is updatable according to INGRES/SQL */
    *reason = 0;
    return((bool) TRUE);
}

/*
** Name: psy_build_audit_info - Build C2 audit info into query plan
**
** Description:
**      This routine is responsible for building the two structures needed
**      by QEF for auditing access to tables and views each time the 
**      query is executed and for listing the security alarms which 
**      will have to be applied each time a query is executed.
**
** Inputs:
**	sess_cb 			Session control block pointer.
**      rngtab                          Pointer to array of range variables.
**      qmode                           Query mode of range variable, i.e. how
**					it's being used
**      mstream                         Memory stream to allocate memory.
**
** Outputs:
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**	    Does catalog I/O
**
** History:
**	20-jul-89 (jennifer)
**          Created.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**      08-aug-90 (ralph)
**          Fix apr queueing.
**	16-dec-90 (ralph)
**	    Trigger security_alarm for failed attempts (b34903).
**	    This involves a minor interface change to doaudit().
**	01-nov-91 (andre)
**	    cleanup some minor bugs
**	4-may-1992 (bryanp)
**	    Added support for PSQ_DGTT_AS_SELECT
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	3-apr-93 (markg)
**	    Moved this routine from psypermit.c and changed it to allow it
**	    to be called multiple times. This enables the auditing data
**	    structures to be built up as the views in a query are being
**	    unravelled.
**	5-jul-93 (robf)
**	    When building the art use QEF definitions, not DMF, and 
**	    also note if its a system catalog
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	15-sep-93 (robf)
**           Don't audit SETINPUT temporary tables. 
**	21-oct-93 (stephenb)
**	    Altered so that we audit when statement is INSERT....VALUES. Also
**	    made sure we do not audit SETINPUT temporary tables.
**	16-nov-93 (stephenb)
**	    Added functionality for PSQ_REPCURS, becuase we now audit update
**	    where current of cursor.
**	24-nov-93 (robf)
**           Filter out FAILURE alarms, and assign flags.
**	2-dec-93 (robf)
**           Renamed psy_build_audit_info() since doaudit() is a tad vague
**	13-dec-93 (robf)
**           Build subject type into qef_gtype, not qef_type
**	12-jan-94 (robf)
**           Add modes PSQ_REPCURS, PSQ_DELCURS
**	1-feb-94 (stephenb)
**	    Backed out INSERT fixes for 21-oct.
**	3-feb-94 (robf)
**           Check for session temp tables.
**	1-mar-94 (robf)
**	     Initialize status.
**	29-mar-94 (robf)
**            Remove check for session temp tables, not fully available in
**	      DMF and may not be set at this point.
**	6-may-94 (robf)
**	     Add space between =& to quieten VMS compiler
**	12-jul-94 (robf)
**           Only look up alarms if table is marked as having them.
**      30-Oct-2008 (hanal04) Bug 120702
**           Completely initialise aud variable to prevent later SEGVs.
*/
static DB_STATUS
psy_build_audit_info(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE	   *rngtab,
	i4		   qmode,
	PSF_MSTREAM	   *mstream,
	DB_ERROR	   *err_blk)
{
    RDF_CB		rdfcb;
    RDF_CB		*rdf_cb = &rdfcb;	/* For alarm tuples */
    register i4	vn;
    register PSS_RNGTAB	*rngvar = NULL;
    i4                  count = 0;
    i4                  acount = 0;
    QEF_ART             *art = 0, *cur_art;
    QEF_APR             *apr = 0, *cur_apr;
    QEF_AUD             *aud = 0;
    DB_STATUS           status=E_DB_OK;
    i4		err_code;
    QEF_DATA	    *qp;
    DB_SECALARM     *almtup;
    RDR_RB	    *rdf_rb = &rdf_cb->rdf_rb;
    DB_STATUS	     stat;
    i4		     tupcount;
    i4               mode;

    /*	Initialize RDF request block. */
    pst_rdfcb_init(rdf_cb, sess_cb);
    rdf_rb->rdr_types_mask = RDR_SECALM;

    cur_art = art;

    /* remember the access mode for audit records */
    switch (qmode)
    {
	case PSQ_DEFCURS:
	case PSQ_REPDYN:
	case PSQ_RETINTO:
	case PSQ_DGTT_AS_SELECT:
	case PSQ_RETRIEVE:
	    mode = SXF_A_SELECT;
	    break;

	case PSQ_APPEND:
	    mode = SXF_A_INSERT;
	    break;
	
	case PSQ_DELETE:
	case PSQ_DELCURS:
	    mode = SXF_A_DELETE;
	    break;

	case PSQ_REPLACE:
	case PSQ_REPCURS: /* update where current of cursor */
	    mode = SXF_A_UPDATE;
	    break;

	default:
	    /* Should this be a more explicit check? */
	    mode = SXF_A_SELECT;
	    break;
    }
	    
    /* Build audit range table. */
    for (vn = 0; vn <= PST_NUMVARS; vn++)
    {
	if (vn < PST_NUMVARS)
	{
	    rngvar = rngtab->pss_rngtab + vn;
	}
	else if (qmode == PSQ_APPEND)
	{
	   /*
	   ** Result range table
	   */
	   rngvar = &rngtab->pss_rsrng;
	}
	else
	{
	    break;
	}

	/* Is range variable being used? Is it a table? */
	if ((!rngvar->pss_used) || (rngvar->pss_rgno == -1 && 
		(qmode!=PSQ_APPEND || rngvar!= &rngtab->pss_rsrng)) || 
	    rngvar->pss_rgtype == PST_RTREE ||
	    rngvar->pss_rgtype == PST_SETINPUT ||
	    !(rngvar->pss_var_mask & PSS_DOAUDIT)) 
	    continue;

        status = psf_malloc(sess_cb, mstream, sizeof(QEF_ART), (PTR *)&cur_art, err_blk);
	if (status != E_DB_OK)
	    return(status);

	if (art != NULL)
	    cur_art->qef_next = art;
	else
	    cur_art->qef_next = NULL;
	art = cur_art;
	count++;

        cur_art->qef_tabtype = 0;
	if (rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
	    cur_art->qef_tabtype |= QEFART_VIEW;
	else
	    cur_art->qef_tabtype |= QEFART_TABLE;
	
	if (rngvar->pss_tabdesc->tbl_status_mask & 
		(DMT_CATALOG|DMT_EXTENDED_CAT) )
	    cur_art->qef_tabtype|= QEFART_SYSCAT;

	if (rngvar->pss_tabdesc->tbl_status_mask & DMT_SECURE)
	    cur_art->qef_tabtype |= QEFART_SECURE;
	STRUCT_ASSIGN_MACRO(rngvar->pss_tabname, cur_art->qef_tabname);
	STRUCT_ASSIGN_MACRO(rngvar->pss_ownname, cur_art->qef_ownname);
	STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, cur_art->qef_tabid);
	cur_art->qef_mode=mode;


	/* Get alarm tuples from RDF that apply to this table */
	/*
	** Only get alarms for tables marked as having alarms.
	** Note that DMT_ALARM was added for ES 1.1 when alarms
	** were stored in iisecalarm seperately from permits.
	*/

	if(!(rngvar->pss_tabdesc->tbl_2_status_mask & DMT_ALARM))
		goto end_alarm;

	STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_rb->rdr_tabid);
	rdf_cb->rdf_info_blk	    = rngvar->pss_rdrinfo;
	rdf_rb->rdr_rec_access_id  = NULL;    
	rdf_rb->rdr_update_op      = RDR_OPEN;
	rdf_rb->rdr_qrymod_id      = 0;	/* get all tuples */
	rdf_rb->rdr_qtuple_count   = 20;	/* get 20 at a time */
	rdf_cb->rdf_error.err_code = 0;

	/* For each group of 20 alarms */
	while (rdf_cb->rdf_error.err_code == 0)
	{
	    status = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);

	    /*
	    ** RDF may choose to allocate a new info block and return its
	    ** address in rdf_info_blk - we need to copy it over to pss_rdrinfo
	    ** to avoid memory leak and other assorted unpleasantries
	    */
	    rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;

	    rdf_rb->rdr_update_op = RDR_GETNEXT;

	    /* Must not use DB_FAILURE_MACRO because E_RD0011 returns
	    ** E_DB_WARN that would be missed.
	    */
	    if (status != E_DB_OK)
	    {
		switch(rdf_cb->rdf_error.err_code)
		{
		    case E_RD0011_NO_MORE_ROWS:
			status = E_DB_OK;
			break;

		    case E_RD0013_NO_TUPLE_FOUND:
			status = E_DB_OK;
			continue;

		    default:
			(VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error,
			    err_blk);
			goto wrapup;
		}	    /* switch */

	    }	/* if status != E_DB_OK */

	    /* For each alarm tuple */
	    for 
	    (
		qp = rdf_cb->rdf_info_blk->rdr_atuples->qrym_data,
		tupcount = 0;
		tupcount < rdf_cb->rdf_info_blk->rdr_atuples->qrym_cnt;
		qp = qp->dt_next,
		tupcount++
	    )
	    {
		/* Set almtup pointing to current alarm tuple */

		almtup = (DB_SECALARM*) qp->dt_data;

		/* Ignore not-success */
	        if (!(almtup->dba_popset & DB_ALMSUCCESS))
			continue;

		switch (qmode)
		{
		    case PSQ_DEFCURS:
		    case PSQ_REPDYN:
		    case PSQ_RETINTO:
		    case PSQ_DGTT_AS_SELECT:
		    case PSQ_RETRIEVE:
			if (~almtup->dba_popset & DB_RETRIEVE)
			    continue;
			break;

		    case PSQ_APPEND:
			if (~almtup->dba_popset & DB_APPEND)
			    continue;
			break;

		    case PSQ_DELETE:
		    case PSQ_DELCURS:
			if (~almtup->dba_popset & DB_DELETE)
			    continue;
			break;

		    case PSQ_REPLACE:
		    case PSQ_REPCURS:
			if (~almtup->dba_popset & DB_REPLACE)
			    continue;
			break;
		}

		/* 
		** queue the alarm to be triggered later by qef.
		*/
		status = psf_malloc(sess_cb, mstream, sizeof(QEF_APR), 
				(PTR *)&cur_apr, err_blk);
		if (status != E_DB_OK)
		    goto wrapup;

		STRUCT_ASSIGN_MACRO (almtup->dba_objid, cur_apr->qef_tabid);
		STRUCT_ASSIGN_MACRO (almtup->dba_subjname, cur_apr->qef_user);
		cur_apr->qef_flags=0;
		/*
		** Copy of alarm info if needed
		*/
		if(almtup->dba_flags&DBA_DBEVENT)
		{
		    i4  len;
		    DB_IIEVENT evtuple;

		    status=psy_evget_by_id(sess_cb, &almtup->dba_eventid,
					&evtuple, err_blk);
		    if(status!=E_DB_OK)
		    {
			/*
			** Couldn't find event!
			*/
	    		(VOID) psf_error(E_US247A_9338_MISSING_ALM_EVENT, 
					0L, PSF_INTERR,
					&err_code, err_blk, 3,
			sizeof(almtup->dba_eventid.db_tab_base), &almtup->dba_eventid.db_tab_base,
			sizeof(almtup->dba_eventid.db_tab_index), &almtup->dba_eventid.db_tab_index,
			sizeof(almtup->dba_alarmname), &almtup->dba_alarmname);
			status=E_DB_SEVERE;
			break;
		    }
		    STRUCT_ASSIGN_MACRO (evtuple.dbe_name, 
				cur_apr->qef_evname);
		    STRUCT_ASSIGN_MACRO (evtuple.dbe_owner, 
					cur_apr->qef_evowner);
		    cur_apr->qef_flags|=QEFA_F_EVENT;
		    len=psf_trmwhite(sizeof(almtup->dba_eventtext), 
			    (char *) &almtup->dba_eventtext);
		    MEcopy((PTR)almtup->dba_eventtext, 
				len, 
			(PTR)cur_apr->qef_evtext);
		    cur_apr->qef_ev_l_text=len;
		}
		cur_apr->qef_popset = almtup->dba_popset;
		cur_apr->qef_gtype = almtup->dba_subjtype;
		cur_apr->qef_mode = mode;
		if (apr != NULL)
		    cur_apr->qef_next = apr;
		else
		    cur_apr->qef_next = 0;
		apr = cur_apr;
		acount++;
	    }
	} /* end while loop */
	
	/* close the iisecalarm system catalog */

	rdf_rb->rdr_update_op = RDR_CLOSE;
	stat = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
	rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;
	rdf_rb->rdr_rec_access_id  = NULL;
	if (DB_FAILURE_MACRO(stat))
	{
	    if (stat > status)
	    {
		(VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error, err_blk);
		status = stat;	
	    }
	}
end_alarm:
	if (status != E_DB_OK)
	    break;

	rngvar->pss_var_mask &= ~PSS_DOAUDIT;
    }	    /* for all interesting range table entries */

wrapup:

    if (rdf_rb->rdr_rec_access_id != NULL)
    {   /* close RDF tuple stream, to release lock on resource */
	rdf_rb->rdr_update_op = RDR_CLOSE;
	stat = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
	rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;
	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error, err_blk);
	    status = stat;
	}
    }

    if (status != E_DB_OK)
	return(status);

    /* 
    **  Attach audit range table and audit alarm list to tree. 
    **  Set count for audit range table.
    */
    if (sess_cb->pss_audit == NULL && (count || acount))
    {
	status = psf_malloc(sess_cb, mstream, sizeof(QEF_AUD), (PTR *)&aud, err_blk);
	if (status != E_DB_OK)
	    return(status);
	/* Initialize audit control block */
        MEfill(sizeof(QEF_AUD), 0, (PTR)aud);
	aud->qef_type=QEFAUD_CB;
	aud->qef_ascii_id=QEFAUD_ASCII_ID;
	sess_cb->pss_audit = (PTR)aud;
    }
    else
    {
	aud = (QEF_AUD *) sess_cb->pss_audit;
    }

    if (count)
    {
	if (aud->qef_art != NULL)
	{
	    for (cur_art = art; 
		 cur_art->qef_next; 
		 cur_art = cur_art->qef_next)
	    {
	    }
	    cur_art->qef_next = aud->qef_art;
	}
	aud->qef_art = art;
	aud->qef_cart += count;
    }

    if (acount)
    {
	if (aud->qef_apr != NULL)
	{
	    while (cur_apr->qef_next)
		cur_apr = cur_apr->qef_next;
	    cur_apr->qef_next = aud->qef_apr;
	}
	aud->qef_apr = apr;
	aud->qef_capr += acount;
    }

    return (E_DB_OK);
}

/*  
** Name: psy_ubtid	- determine whether the view tree contains id of its 
**			  underlying base table (ubt_id)
**
** Description:
** 	This function will obtain a view tree and if it contains id of that 
**	view's underlying base table, make a copy of it for the caller
**
** Input:
**	rngvar		range table entry describing a view whose tree must be 
**			fetched
**	sess_cb		PSF session CB
**
** Output:
**	err_blk		filled in if an error occurs
**	ubt_id		filled in with id of the view's underlying base table if
**			the view tree contains one; 0'd out otherwise
**
** History:
**	21-aug-93 (andre)
**	    written
**	16-nov-93 (andre)
**	    this function will not return ids of catalogs because we do not want
**	    to alter their timestamps as means of forcing QEP invalidation.
*/
DB_STATUS
psy_ubtid(
	PSS_RNGTAB	    *rngvar,
	PSS_SESBLK	    *sess_cb,
	DB_ERROR	    *err_blk,
	DB_TAB_ID	    *ubt_id)
{
    DB_STATUS	    status = E_DB_OK;
    RDF_CB	    rdf_cb;
    PST_PROCEDURE   *pnode;
    PST_QTREE	    *vtree;
    i4	    err_code;

    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_rb.rdr_types_mask = RDR_VIEW | RDR_QTREE ;
    rdf_cb.rdf_rb.rdr_qtuple_count = 1;
    rdf_cb.rdf_info_blk = rngvar->pss_rdrinfo;
    status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);

    /*
    ** RDF may choose to allocate a new info block and return its address in
    ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
    ** leak and other assorted unpleasantries
    */
    rngvar->pss_rdrinfo = rdf_cb.rdf_info_blk;
    
    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	{
	    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, rdf_cb.rdf_error.err_code,
		PSF_INTERR, &err_code, err_blk, 1,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &rngvar->pss_tabname),
		&rngvar->pss_tabname);
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb.rdf_error, err_blk);
	}
	return (status);
    }
    pnode = (PST_PROCEDURE *) rdf_cb.rdf_info_blk->rdr_view->qry_root_node;
    vtree = pnode->pst_stmts->pst_specific.pst_tree;
    if (vtree->pst_basetab_id.db_tab_base > DM_SCATALOG_MAX)
    {
	ubt_id->db_tab_base  = vtree->pst_basetab_id.db_tab_base;
	ubt_id->db_tab_index = vtree->pst_basetab_id.db_tab_index;
    }
    else
    {
	ubt_id->db_tab_base = ubt_id->db_tab_index = (i4) 0;
    }

    return (E_DB_OK);
}
