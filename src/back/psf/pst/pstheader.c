/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <cm.h>
#include    <me.h>
#include    <qu.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSTHEADER.C - Functions for creating query tree headers.
**
**  Description:
**      This file contains the functions for creating query tree headers.
**	Every query tree contains a header.  The header contains the query
**	mode, the range table for the query tree,
**	a flag telling whether it's a cursor definition "for update", a pointer
**	to the query tree, the sort list for the query tree (if any), the
**	number of repeat query parameters in the tree, the name of the repeat
**	query (if it is such), and the table id (or table name and owner) of
**	the result table, if any.  Other things may be added to the header
**	in the future.
**
**          pst_header - Allocate and fill in a query tree header.
**	    pst_modhdr - Update existing header.
**	    pst_cdb_cat- Query involves catalogs on CDB only
**
**  History:
**      01-apr-86 (jeff)    
**          written
**	18-mar-87 (stec)
**	    Added pst_resjour handling.
**	27-aug-87 (stec)
**	    Added pst_modhdr procedure.
**	02-oct-87 (stec)
**	    Cleaned up code in connection with PST_RESTAB.
**	29-feb-88 (stec)
**	    Check for VLUPs in the target list - temporary fix.
**	22-apr-88 (stec)
**	    pst_modhdr should update the mode in header.
**	24-oct-88 (stec)
**	    Change to code executed when dealing with cb->pss_resrng.
**	27-oct-88 (stec)
**	    Change initialization of pst_updmap in the header.
**      24-jul-89 (jennifer)
**          Added a new field to statement header pst_audit to point 
**          to security audit information required by QEF. 
**	13-sep-90 (teresa)
**	    changed the following booleans to bitflags: pss_agintree and
**	    pss_subintree
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**	    For a declared global temporary table, the "owner" of the table
**	    is the special internally-generated session schema, not the user.
**	11-jun-92 (barbara)
**	    Merged Star code for Sybil.  Star history:
**	    15-aug-91 (kirke)
**		Removed MEfill() declaration - it's in me.h.
**	    16-mar-92 (barbara)
**		Allocate PST_QRYHDR_INFO structure.  This structure is used
**		to pass table id info to OPC where it is stored as part of the
**		QEP.  PSF uses this part of the QEP to decide if a Star repeat
**		query is shareable.  The method is taken from QUEL repeat
**		queries.
**	09-Nov-92 (rblumer)
**	    Change pst_header to initialize PST_STATEMENT nodes to all zero's
**	    (thus initializing new field pst_statementEndRules).
**	22-dec-92 (rblumer)
**	    Attach statement-level rules to statement from global list.
**	    Initialize new pst_flags field in PST_PROCEDURE nodes.
**	06-apr-93 (smc)
**	    Commented out text following endif.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	05-may-94 (davebf)
**	    Add routine pst_singlesite (called from pst_header) 
**	    to set PST_1SITE for OPF
**      12-apr-95 (rudtr01)
**          Fixed bug 68036.  In pst_cdb_cat(), changed all calls to
**          CMcmpcase() to CMcmpnocase().  Needed for FIPS installations
**          where identifiers are coerced to upper case.  Also tidied
**          the code to conform to the One True Standard.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions regarding ifnull and
**	    aggregates. Making the decision in opf/opa, as is now done, can
**	    cause problems when we detect a "do not flatten" situation but
**	    have already begun the process of query flattening.
**	    Do not flatten query if we have:
**	    - more than one IFNULL with an aggregate operand
**		OR
**	    - one IFNULL with an aggregate operand and another aggregate
**	      operand in the query
**	2-feb-06 (dougi)
**	    Replace pst_rngtree integer flag with PST_RNGTREE single bit.
**	12-Jun-2006 (kschendel)
**	    Fix a null/0 mixup causing compiler warnings.
**	13-May-2009 (kschendel) b122041
**	    Another of the same.
**/

/* Forward references */
VOID
pst_singlesite(
	PSS_RNGTAB	*rng_tab,
	i4             *stype,
	DD_LDB_DESC	**save_ldb_desc);

static DB_STATUS
pst_proc_rngentry(
		  PSS_SESBLK	*sess_cb,
		  PSQ_CB	*psq_cb,
		  PST_QTREE	*header,
		  PSS_RNGTAB	*usrrange,
		  bool		*is_iidd);

static void
pst_chk_xtab_upd_const_only( PST_QTREE  *header);

static bool walk(PST_QNODE *qn1);

/*{
** Name: pst_header	- Allocate and fill in a query tree header
**
** Description:
**      This function allocates a query tree header and fills it in.  See the
**	description given above for what's in a query tree header.  In addition,
**	this function will do miscellaneous things to the query tree, like add
**	on the PST_TREE and PST_QLEND nodes, and copy the user's range table
**	from the session control block to the query tree's range table.
**
**	In addition, this function sets the QSF root of the query tree to point
**	to the query tree header.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**	  pss_ostream			  For memory allocation
**	  pss_usrrange			  User's range table copied to query
**					  tree's range table
**	  pss_auxrng			  Auxiliary range table copied to
**					  query tree's range table for
**					  statements that have gone through
**					  qrymod.
**	  pss_resrng			  Result range variable info copied to
**					  range table.
**	  pss_restab			  Result structure info copied to header
**	  pss_highparm			  Largest parameter number for this tree
**	  pss_crsr			  Ptr to current cursor cb, if any
**	  pss_stmt_flags	    	  word containing various bitmasks:
**	    PSS_AGINTREE		  set if aggregates were found in tree
**	    PSS_SUBINTREE		  set if subselect was found in tree
**	  pss_flattening_flags		bits of info that help us determine 
**					whether a given DML query should be 
**					flattened or, if processing a view 
**					definition, which bits describing 
**					flattening-related characteristics of a
**					view will be set in pst_mask1
**      psq_cb                          Pointer to the user's control block
**	  psq_error			  Place to put error info it error
**					  happens
**	  psq_mode			  Query mode is copied to header.
**      forupdate                       Type of update allowed in cursor
**					    PST_DIRECT - Direct update
**					    PST_DEFER - Deferred update
**					    PST_READONLY - No update allowed
**      sortlist                        Pointer to the sort list, if any
**	qtree				Pointer to query tree to store in header
**	tree				Place to put pointer to header
**	mask				Miscellaneous node info:
**					    PST_1INSERT_TID -  pass to OPF
**					    PST_0FULL_HEADER - stmt/proc node
**	xlated_qry			ptr to structure used to administer
**					translated query text.
**
** Outputs:
**	sess_cb
**	  .pss_ostream			  Memory allocated
**      psq_cb
**	  .psq_error			  Filled in with error information,
**					  if any
**	qtree				Qualification put in conjunctive normal
**					form.  PST_TREE and PST_QLEND nodes
**					attached.
**	tree				Filled in with pointer to header
**	pnode				Filled in with ptr to procedure node
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**	    Sets root of query tree in QSF
**
** History:
**	01-apr-86 (jeff)
**          written
**	02-oct-87 (stec)
**	    Cleaned up code in connection with PST_RESTAB.
**	29-feb-88 (stec)
**	    Check for VLUPs in the target list - temporary fix
**	    for FEs (to be removed).
**	24-oct-88 (stec)
**	    Change to code executed when cb->pss_resrng defined.
**	27-oct-88 (stec)
**	    Change initialization of pst_updmap in the header.
**	26-jan-89 (neil)
**	    Add in rule processing tree to the "after" statement list.
**	    This tree was built in qrymod, by psy_rules.
**	14-apr-89 (neil)
**	    Updated old comment.
**	18-apr-89 (andre)
**	    pst_res2 renamed to pst_mask1.
**      24-jul-89 (jennifer)
**          Added a new filed to statement header pst_audit to point 
**          to security audit information required by QEF. 
**	15-nov-89 (andre)
**	    removed loop checking for illegal use of VLUPs as it is now done in
**	    SQL/QUEL grammar where we can make a more intelligent decision as
**	    the context is known.  It was done to fix bug 8638 where the
**	    abovementioned loop caused errors when VLUPs were found in the
**	    target list of subselect in INSERT...subselect.
**	13-sep-90 (teresa)
**	    changed the following booleans to bitflags: pss_agintree and
**	    pss_subintree
**	25-jan-91 (andre)
**	    PST_QTREE.pst_res1 has been renamed to pst_info and will be used to
**	    contain an address of PST_QRYHDR_INFO structure.  pst_header() will
**	    be responsible for allocating this structure and initializing it.
**	    pst_header()'s caller may choose to store some meaningful
**	    information in *PST_QTREE.pst_info (e.g. for QUEL shareable repeat
**	    queries) or to leave them alone.
**	    If a tree is being built for an SQL query, set PST_SQL_QUERY bit in
**	    PST_QTREE.pst_mask1
**	08-Nov-91 (rblumer)
**	  merged from 6.4:  25-feb-91 (andre)
**	    make changes related to changes in PST_QTREE structure.
**	  merged from 6.4:  01-mar-91 (andre)
**	    for PSQ_DELCURS use pss_auxrng since pst_header() for DELETE CURSOR
**	    can be called either from PSLSGRAM.YI or from psq_dlcsrrow() where
**	    result table entry (if any) will also be entered into pss_auxrng.
**	2-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**	    For a declared global temporary table, the "owner" of the table
**	    is the special internally-generated session schema, not the user.
**	11-jun-92 (barbara)
**	    Merged Star code for Sybil.  Star history:
**	    07-oct-88 (andre)
**		added new arg - xlated_qry - for processing of distributed
**		queries
**	    01-nov-88 (andre)
**		Added code to send RANGE statement when translating
**		single-site QUEL queries in STAR.
**	    10-feb-89 (andre)
**		replaced hdrtype which was used to indicate whether or not
**		a full header was required with a mask field for greater
**		flexibility.
**	    6-feb-89 (andre)
**		QED_PSF_RANGE-->QED_1PSF_RANGE
**	    16-mar-89 (andre)
**		Save ptr to the last packet in the chain in 
**		qsite_info->pst_last_pkt
**	    05-apr-89 (andre)
**		OR pst_mask1 with PST_CDB_IIDD_TABLES if the query
**		references ONLY iidd_* tables located on CDB with exception
**		of iidd_stats and iidd_histograms.  Also OR pst_mask1 with
**		PST_CATUPD_OK if the user has catalog update privilege.
**	    20-apr-89 (andre)
**		queries referencing iidd_dbconstants will not be going over
**		privileged association.
**	    Because Star's single site query buffering is no longer done
**	    by PSF, some actions related to single-site queries have been
**	    eliminated for Sybil.  Star actions are to set special RDF flag
**	    if query is single-site against CDB catalogs; also to put
**	    distributed-only info into query tree for OPF.
**	05-oct-92 (barbara)
**	    If PSF has not saved up text, set pst_qtext to NULL for OPF.
**	09-Nov-92 (rblumer)
**	    Change code to initialize PST_STATEMENT nodes to all zero's
**	    (thus initializing new field pst_statementEndRules).
**	22-dec-92 (rblumer)
**	    Initialize new pst_flags field in PST_PROCEDURE nodes.
**	04-mar-93 (andre)
**	    when REPLACE CURSOR in QUEL, we will obtain description of the
**	    table/view on which cursor was defined and then let psy_qrymod()
**	    loose on it; description of the cursor's underlying base table will
**	    be placed into the query tree header
**	08-apr-93 (andre)
**	    sess_cb maintains four distinct lists of rules: user-defined
**	    row-level, system-generated row-level, user-defined statement-level,
**	    and system-generated statement-level.  System-generated row-level
**	    and statement-level rules need to be appended to user-defined
**	    row-level and statement-level rules, respectively.  The resulting
**	    two lists will be pointed to by pst_after_stmt and
**	    pst_statementEndRules.  Following this (unless we are processing
**	    PREPARE), we will voalesce row-level and statement-level rules.
**	    This used to be done in psy_rules() but with introduction of rules
**	    on views, it no longer seemed like a very good place to do it.
**	16-sep-93 (robf)
**         Set PST_UPD_ROW_SECLABEL in flags mask if so intructed by caller
**	   Leave pss_audit untouched until assigned to a statement (when
**	   full header is built). This prevents audit info for dbproc
**	   statements getting "lost".
**	17-nov-93 (andre)
**	    now that all the information needed to determine whether a query 
**	    should be flattened is available in sess_cb->pss_flattening_flags, 
**	    we can move code responsible for setting PST_NO_FLATTEN and 
**	    PST_CORR_AGGR bits from various places in PSLCTBL.C, PSLCVIEW.C, and
**	    PSLSGRAM.C into pst_header().  
**
**	    For DML queries (SELECT, DELETE, INSERT, UPDATE, CREATE TABLE AS 
**	    SELECT, DGTT AS SELECT) we will set (if appropriate) PST_NO_FLATTEN
**	    and PST_CORR_AGGR bits.  
**
**	    For PSQ_VIEW, we will translate various bits in pss_flattening_flags
**	    into corresponding bits in qtree->pst_mask1.  
**
**	    For DECLARE CURSOR (psq_cb->psq_mode == PSQ_DEFCURS) we will do 
**	    nothing here - pst_modhdr() will handle setting (if appropriate) 
**	    PST_NO_FLATTEN and PST_CORR_AGGR bits.
** 
**	    For remaining queries, bits in pss_flattening_flags will not even 
**	    be examined - this may need to change if we decide to add support 
**	    for rules that allow subselects (but then lots of other things may 
**	    need to change as well.)
**	6-dec-93 (robf)
**         Set need query syscat flag.
**	21-mar-94 (andre)
**	    fix for bug 58965:
**	    when populating a range entry describing the result range variable 
**	    for INSERT, we need to copy pss_[inner|outer]_rel into 
**	    pst_[inner|outer]_rel
**	05-may-94 (davebf)
**	    Add call to pst_singlesite to set PST_1SITE for OPF
**      23-june-1997(angusm)
**	    1. flag up cross-table updates
**          2. Validate tree for cross-table update query to allow
**          elimination of 'duplicate' rows where update only
**          involves constant values.
**	    (bugs 77040, 79623, 80750)
**	17-jul-2000 (hayke02)
**	    Set PST_INNER_OJ in pst_mask1 if PSS_INNER_OJ has been set in
**	    pss_stmt_flags.
**	26-Mar-2002 (thaju02)
**	    For dgtt as select involving a IN/OR subselect, set 
**	    PST_NO_FLATTEN bit. (B103567)
**	29-nov-2004 (hayke02)
**	    Init (pst_)stype with PST_1SITE rather than PST_NSITE so that STAR
**	    singlesite queries are now unflattened - see opa_1pass(). This
**	    change fixes problem INGSRV 3045, bug 113436.
**	 3-mar-2005 (hayke02)
**	    Set PST_MULT_CORR_ATTRS in pst_mask1 for select/retrieve as well
**	    as for views. We then test for PST_MULT_CORR_ATTRS in opa_1pass()
**	    when deciding whether or not to flatten single site STAR queries.
**	    This change fixes problem INGSTR 70, bug 114009.
**	16-jan-2006 (hayke02)
**	    Set PST_SUBSEL_IN_OR_TREE in pst_mask1 for select/retrieve as well
**	    as for views. We then test for PST_SUBSEL_IN_OR_TREE in opa_1pass()
**	    when deciding whether or not to flatten single site STAR queries.
**	    This change fixes bug 113941.
**	3-feb-06 (dougi)
**	    Stuff hint mask.
**	9-may-06 (dougi)
**	    Add support for trace point ps151 (enables 6.4 ambiguous update).
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	17-jan-2007 (dougi)
**	    Set PST_SCROLL flag for scrollable cursors.
**	12-feb-2007 (dougi)
**	    ... and PST_KEYSET.
**      03-jun-2005 (gnagn01)
**          Set PST_NO_FLATTEN bit for a select with case operator and 
**          aggregate.Bug 113574,INGSRV3073.
**	04-oct-2007 (toumi01) BUG 119250
**	    Fix setting of PST_CDB_IIDD_TABLES to avoid clobbering pst_mask1.
**	    Doing this broke createdb for Star.
**      10-dec-2007 (smeke01) b118405
**          Add AGHEAD_MULTI, IFNULL_AGHEAD and IFNULL_AGHEAD_MULTI handling 
**          to PSQ_VIEW, PSQ_DEFCURS & PSQ_DGTT_AS_SELECT cases. Restructured 
**          PSQ_RETRIEVE etc case so that the setting of pst_mask1 flags 
**          relevant to views is done in one place.
*/
DB_STATUS
pst_header(
	PSS_SESBLK      *sess_cb,
	PSQ_CB		*psq_cb,
	i4		forupdate,
	PST_QNODE	*sortlist,
	PST_QNODE	*qtree,
	PST_QTREE	**tree,
	PST_PROCEDURE   **pnode,
	i4		mask,
	PSS_Q_XLATE	*xlated_qry)
{
    DB_STATUS           status;
    PST_QTREE		*header;
    register PST_QNODE	*qnode;
    register PSS_RNGTAB	*usrrange;
    register i4	i;
    bool		is_iidd = FALSE;
    i4			stype;
    DD_LDB_DESC		*ldb_desc = (DD_LDB_DESC *)NULL;
    extern PSF_SERVBLK  *Psf_srvblk;
    i4		val1;
    i4		val2;

    *tree = (PST_QTREE *) NULL;
    *pnode = (PST_PROCEDURE *) NULL;

    /* set initial value of stype.  PST_1SITE may be over-ridden */
    stype = PST_1SITE;
    /*stype = PST_NSITE;*/
    /* Note: preceeding line turns "off" distributed singlesite unflattening.
    ** When it is desired to turn it on and finish debugging it (b58068),
    ** replace with    stype = PST_1SITE */

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_QTREE),
	(PTR *) &header, &psq_cb->psq_error);
    if (status != E_DB_OK)
    {
	return (status);
    }

    MEfill(sizeof (PST_QTREE), 0, (PTR) header);

    /* pst_rgtype field in the header range table needs to initialized
    ** to PST_UNUSED value, but the fact that MEfill initializes it
    ** to NULL and PST_UNUSED is 0, allow us to take no specific action.
    */

    /*
    ** Fill in query tree header.
    */

    /* The query mode */
    header->pst_mode = psq_cb->psq_mode;

    /* The version */
    header->pst_vsn = PST_CUR_VSN;

    /* Indicate if any aggregates in tree */
    header->pst_agintree = ((sess_cb->pss_stmt_flags & PSS_AGINTREE) != 0);
    /* indicate subselects */
    header->pst_subintree= ((sess_cb->pss_stmt_flags & PSS_SUBINTREE) != 0);

    header->pst_hintmask = sess_cb->pss_hint_mask;
    sess_cb->pss_hint_mask = 0;
    header->pst_hintcount = sess_cb->pss_hintcount;
    if (header->pst_hintcount == 0)
	header->pst_tabhint = (PST_HINTENT *) NULL;
    else
    {
	/* Allocate space for hint array and copy the whole thing. */
	i = header->pst_hintcount;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
		i * sizeof(PST_HINTENT), (PTR)&header->pst_tabhint,
		&psq_cb->psq_error);
	if (status != E_DB_OK)
	{
	    return (status);
	}
	MEcopy((char *)&sess_cb->pss_tabhint, i * sizeof(PST_HINTENT),
		(char *)header->pst_tabhint);
    }

    /*
    ** Copy range table to query tree.  If the statement has gone through
    ** qrymod, use the auxiliary range table, otherwise use the user's
    ** range table.
    */
    if (psq_cb->psq_qlang == DB_SQL
	||
	psq_cb->psq_mode == PSQ_RETRIEVE || psq_cb->psq_mode == PSQ_RETINTO ||
	psq_cb->psq_mode == PSQ_APPEND   || psq_cb->psq_mode == PSQ_REPLACE ||
	psq_cb->psq_mode == PSQ_DELETE   || psq_cb->psq_mode == PSQ_DEFCURS ||
	psq_cb->psq_mode == PSQ_DELCURS  || psq_cb->psq_mode == PSQ_REPCURS ||
	psq_cb->psq_mode == PSQ_DGTT_AS_SELECT)
    {
	usrrange = sess_cb->pss_auxrng.pss_rngtab;

	/* remember number of entries in the range table */
	header->pst_rngvar_count = sess_cb->pss_auxrng.pss_maxrng + 1;
    }
    else
    {
	usrrange = sess_cb->pss_usrrange.pss_rngtab;

	/* remember number of entries in the range table */
	header->pst_rngvar_count = sess_cb->pss_usrrange.pss_maxrng + 1;
    }

    /*
    ** if this is OPEN CURSOR statement or PREPAREd SELECT/RETRIEVE, we don't
    ** really know how many entries there are in the range table (since qrymod
    ** will be performed later), so in such cases we will allocate the maximum
    ** number of pointers, since otherwise we may have problems later on in
    ** pst_modhdr().
    */
    if(psq_cb->psq_mode == PSQ_DEFCURS ||
       (psq_cb->psq_mode == PSQ_RETRIEVE && sess_cb->pss_defqry == PSQ_PREPARE))
    {
	i = PST_NUMVARS;
    }
    else
    {
	i = header->pst_rngvar_count;
    }

    /* for some queries there may be no entries to allocate */
    if (i == 0)
    {
	header->pst_rangetab = (PST_RNGENTRY **) NULL;
    }
    else
    {
	/*
	** allocate and initilize an array of pointers to range entry structure
	*/ 
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, i * sizeof(PST_RNGENTRY *),
	    (PTR *) &header->pst_rangetab, &psq_cb->psq_error);
	if (status != E_DB_OK)
	{
	    return (status);
	}

	for (i--; i >= 0; i--)
	{
	    header->pst_rangetab[i] = (PST_RNGENTRY *) NULL;
	}

	for (i = 0; i < PST_NUMVARS; i++, usrrange++)
	{
	    /*
	    ** Only build the range table for those range variables that have
	    ** had range variable numbers assigned to them for this query.
	    */
	    if (usrrange->pss_rgno != -1 && usrrange->pss_used)
	    {
		status = pst_proc_rngentry(sess_cb, psq_cb, header, 
		    usrrange, &is_iidd);
		if (status != E_DB_OK)
		{
		    return(status);
		}

		if (sess_cb->pss_distrib & DB_3_DDB_SESS)
		{

		    /* set stype for use below */
		    pst_singlesite(usrrange, &stype, &ldb_desc);

		}
	    }
	}
    }

    /* Update mode indicator */
    header->pst_updtmode = forupdate;

    /* The query tree */
    header->pst_qtree = qtree;

    /*
    ** this field will be used (starting with 6.5) to store ID of a table whose
    ** timestamp must be changed to force invalidation of QEPs which may have
    ** become obsolete as a result of destruction of a view or a permit; it will
    ** also be used to force invalidation of QEPs of database procedures when a
    ** database procedure or a permit on a database procedure gets dropped
    */
    header->pst_basetab_id.db_tab_base =
	header->pst_basetab_id.db_tab_index = (i4) 0;
    
    /* is this a repeat query */
    if (sess_cb->pss_defqry == PSQ_50DEFQRY ||
	sess_cb->pss_defqry == PSQ_DEFQRY ||
	sess_cb->pss_defqry == PSQ_DEFCURS)
    {
	header->pst_mask1 |= PST_RPTQRY;
    }

    /* The sort list */
    if (sortlist != (PST_QNODE *) NULL)
    {
	header->pst_stflag = TRUE;
    }
    else
    {
	header->pst_stflag = FALSE;
    }
    header->pst_stlist = sortlist;

    /* The number of repeat query parameters (add 1 since they start at 0) */
    header->pst_numparm = sess_cb->pss_highparm + 1;

    /*
    ** I don't think this need to be copied for cases other than RETRIEVE
    ** INTO/CREATE AS SELECT, but I'll let it be for the time being
    */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_restab, header->pst_restab);

    /* The result range variable, if any */
    if (sess_cb->pss_resrng != (PSS_RNGTAB *) NULL)
    {
	/* copy some data from pss_resrng */
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
	    header->pst_restab.pst_restabid);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabname,
	    header->pst_restab.pst_resname);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_ownname,
	    header->pst_restab.pst_resown);
	header->pst_restab.pst_resvno = sess_cb->pss_resrng->pss_rgno;

	/*
	** unlike pst_modhdr(), here we cannot assume that the range entry
	** information has been copied:
	**   for INSERT/APPEND cb->pss_resrng will be pointing at
	**   pss_usrrange->pss_rsrng or pss_auxrange->pss_rsrng (for QUEL and
	**   SQL, respectively), and the above loop does not try to copy these
	**   entries
	*/

	/*
	** allocate a range variable entry and populate it if it has not been
	** allocated (and populated) yet
	*/
	if (header->pst_rangetab[header->pst_restab.pst_resvno] ==
	    (PST_RNGENTRY *) NULL)
	{
	    status = pst_proc_rngentry(sess_cb, psq_cb, header, 
	        sess_cb->pss_resrng, &is_iidd);
	    if (status != E_DB_OK)
	    {
	        return(status);
	    }
	}
	else if (sess_cb->pss_distrib & DB_3_DDB_SESS && !is_iidd)
	{

	    /*
	    ** check if the table is on CDB and has a name starting
	    ** with "iidd_". Queries against these tables will use
	    ** the privileged association.
	    */ 
	    is_iidd = pst_cdb_cat(sess_cb->pss_resrng, psq_cb); 
	}
    }

    /*
    ** On a retrieve into, the table name and owner are in a different place.
    */
    if (header->pst_mode == PSQ_RETINTO)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_user,
				header->pst_restab.pst_resown);
    }
    else if (header->pst_mode == PSQ_DGTT_AS_SELECT)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_sess_owner,
				header->pst_restab.pst_resown);
    }

    /*
    ** Copy the cursor id, if any, to the header.
    ** Copy the update map, if any, to the header.
    */
    if (sess_cb->pss_crsr != (PSC_CURBLK *) NULL)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_crsr->psc_blkid, header->pst_cursid);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_crsr->psc_iupdmap, header->pst_updmap);
    } else if ((sess_cb->pss_defqry == PSQ_DEFCURS) ||
	       (sess_cb->pss_defqry == PSQ_50DEFQRY) ||
	       (sess_cb->pss_defqry == PSQ_DEFQRY))
    {
	STRUCT_ASSIGN_MACRO(psq_cb->psq_cursid, header->pst_cursid);
    }

    /*
    ** If replace statment, set the updmap to contain the updated attributes
    ** All elements in the target list are updateable.
    */
    if (psq_cb->psq_mode == PSQ_REPLACE)
    {
	for (qnode = qtree->pst_left; qnode != (PST_QNODE*) NULL 
			    && qnode->pst_sym.pst_type == PST_RESDOM;
	     qnode = qnode->pst_left)
	{
	    BTset((i4) qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno, 
		(char *) &header->pst_updmap);
	}
    }
    /*
    ** Assume no delete permission given for now, unless this is a delete
    ** command.
    */
    header->pst_delete = psq_cb->psq_mode == PSQ_DELETE;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_QRYHDR_INFO),
	(PTR *) &header->pst_info, &psq_cb->psq_error);
    if (status != E_DB_OK)
    {
	return (status);
    }

    header->pst_info->pst_1_usage = 0L;	    /*
					    ** this field may get reset later,
					    ** if there is a need to use the
					    ** structure
					    */ 

    if (is_iidd == TRUE)
	header->pst_mask1 |= PST_CDB_IIDD_TABLES;

    if (sess_cb->pss_stmt_flags & PSS_SCROLL)
    {
	header->pst_mask1 |= PST_SCROLL;	/* scrollable cursor */
	if (sess_cb->pss_stmt_flags & PSS_KEYSET)
	    header->pst_mask1 |= PST_KEYSET;	/* keyset scrollable cursor */
    }

    /* remember language of the query */
    if (sess_cb->pss_lang == DB_SQL)
    {
	header->pst_mask1 |= PST_SQL_QUERY;
    }

    if (mask & PST_1INSERT_TID)
    {
	header->pst_mask1 |= PST_TIDREQUIRED;
    }

    /*
    ** Set qry_syscat flag. Note: This should really only be set
    ** when the query needs it (see qrymod), not just when the user
    ** has it.
    */
    if(psy_ckdbpr(psq_cb, DBPR_QUERY_SYSCAT)==E_DB_OK)
    {
	header->pst_mask1 |= PST_NEED_QRY_SYSCAT;
    }

    /* if user has catalog update privilege, set a bit for OPF */
    if (sess_cb->pss_ses_flag & PSS_CATUPD)
    {
	header->pst_mask1 |= PST_CATUPD_OK;
    }

    /*
    ** translate bits in sess_cb->pss_flattening_flags according to the mode of 
    ** the query
    */
    switch (psq_cb->psq_mode)
    {
	case PSQ_RETRIEVE:
	{
	    if (sess_cb->pss_defqry == PSQ_PREPARE)
	    {
		/* 
		** if preparing a SELECT, psy_qrymod() has not been called yet,
		** we will leave it up to pst_modhdr() (which gets called as a 
		** part of opening a cursor) to set (if appropriate) 
		** PST_NO_FLATTEN and PST_CORR_AGGR
		*/
		break;
	    }
	}
	case PSQ_RETINTO:
	case PSQ_DELETE:
	case PSQ_REPLACE:
	case PSQ_APPEND:
	case PSQ_DEFCURS:
	case PSQ_DGTT_AS_SELECT:
	{
	    /*
	    ** do not flatten trees which contain SUBSEL node s.t. it is a
	    ** descenedant of OR node;
	    ** ... the same applies to trees involving ALL ... SELECT
	    ** ... the same applies to trees containing multiple correlated
	    **     references in a tree rooted in EXIST which is a descendant 
	    **     of NOT or a SUBSEL with count() as one of the elements of 
	    **	   the target list
	    ** ... the same applies to queries involving singleton subselect(s)
	    **	   if we were told at server startup that such queires should 
	    **	   not be flattened
            ** ... the same applies to queries with case operator and aggregate
	    */
	    if (   (sess_cb->pss_flattening_flags &
			(  PSS_SUBSEL_IN_OR_TREE | PSS_ALL_IN_TREE 
			 | PSS_MULT_CORR_ATTRS))
		|| (   Psf_srvblk->psf_flags & PSF_NOFLATTEN_SINGLETON_SUBSEL
		    && sess_cb->pss_flattening_flags & PSS_SINGLETON_SUBSELECT)
                || (sess_cb->pss_flattening_flags & PSS_CASE_AGHEAD )
	       )
	    {
		header->pst_mask1 |= PST_NO_FLATTEN;
	    }

            /* b110577, INGSRV2413
            ** ... the same applies to queries with more than one IFNULL
            **     with an aggregate operand OR one IFNULL with an aggregate
            **     operand and another aggregate in the query
            */
            if (( sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD_MULTI )
                || ( sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD
                    && sess_cb->pss_flattening_flags & PSS_AGHEAD_MULTI )
               )
            {
                header->pst_mask1 |= PST_AGHEAD_NO_FLATTEN;
            }
             
	    if (sess_cb->pss_stmt_flags & PSS_XTABLE_UPDATE)
	    {
		header->pst_mask1 |= PST_XTABLE_UPDATE;
	    }
	    else
	    {
		header->pst_mask1 &= ~PST_XTABLE_UPDATE;
	    }

	    /* 
	    ** This drops through to PSQ_VIEW intentionally so as to keep the 
	    ** pst_mask1 code for PSQ_VIEW in line with PSQ_RETRIEVE/PSQ_RETINTO.
	    ** This is so that we make sure that exactly the same flags are 
	    ** stored (and subsequently restored when the view is used) for a 
	    ** view as used for a retrieve. 
	    ** 
	    ** We don't store PST_NO_FLATTEN or PST_AGHEAD_NO_FLATTEN as this is 
	    ** determined from the other flags that we do store. We also don't 
	    ** want to store PST_XTABLE_UPDATE as this is not a flag that could 
	    ** occur on a view.
	    */

	}

	case PSQ_VIEW:
	{
	    if (sess_cb->pss_flattening_flags & PSS_CORR_AGGR)
	    {
		header->pst_mask1 |= PST_CORR_AGGR;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_MULT_CORR_ATTRS)
	    {
		header->pst_mask1 |= PST_MULT_CORR_ATTRS;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_SINGLETON_SUBSELECT)
	    {
		header->pst_mask1 |= PST_SINGLETON_SUBSEL;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_SUBSEL_IN_OR_TREE)
	    {
		header->pst_mask1 |= PST_SUBSEL_IN_OR_TREE;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_ALL_IN_TREE)
	    {
		header->pst_mask1 |= PST_ALL_IN_TREE;
	    }

	    if (sess_cb->pss_stmt_flags & PSS_INNER_OJ)
	    {
		header->pst_mask1 |= PST_INNER_OJ;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_AGHEAD_MULTI)
	    {
		header->pst_mask1 |= PST_AGHEAD_MULTI; 
	    }

	    if (sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD)
	    {
		header->pst_mask1 |= PST_IFNULL_AGHEAD;
	    }

	    if (sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD_MULTI)
	    {
		header->pst_mask1 |= PST_IFNULL_AGHEAD_MULTI;
	    }

	    break;
	}

	default:
	    /* 
	    ** no translation of "flattening" bits needed for remaining types
	    ** of queries (at least for now)
	    */
	    break;
    }

    /*
    ** Do final processing of query tree.  Set variable bit maps.  Count
    ** variables.
    */
    status = pst_trfix(sess_cb, &sess_cb->pss_ostream, qtree, &psq_cb->psq_error);

    /*
    ** Make output pointer point to new header if we got through everything
    ** with no errors.
    */
    if (status == E_DB_OK)
	*tree = header;

    /*
    ** If hdrtype is TRUE a full header has been requested, i.e.,
    ** PROCEDURE and STATEMENT nodes should be generated.
    */
    if (mask & PST_0FULL_HEADER)
    {
	PST_STATEMENT	*snode;
	PST_PROCEDURE   *prnode;

	/* Allocate statement node. */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_STATEMENT),
	    (PTR *) &snode, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}

	MEfill(sizeof(PST_STATEMENT), 0, (PTR) snode);

	snode->pst_mode = psq_cb->psq_mode;
	snode->pst_type = PST_QT_TYPE;
	snode->pst_specific.pst_tree = header;

	/* Assign row-level and statement-level rules from saved rule lists 
	 */

	/*
	** make pst_after_stmt point at user-defined row-level rules and then
	** append system-generated row-level rules to the end of the list
	*/
	snode->pst_after_stmt        = sess_cb->pss_row_lvl_usr_rules;
	psy_rl_coalesce(&snode->pst_after_stmt, sess_cb->pss_row_lvl_sys_rules);

	/*
	** make pst_statementEndRules point at user-defined statement-level
	** rules and then append system-generated statement-level rules to the
	** end of the list
	*/
	snode->pst_statementEndRules = sess_cb->pss_stmt_lvl_usr_rules;
	psy_rl_coalesce(&snode->pst_statementEndRules,
	    sess_cb->pss_stmt_lvl_sys_rules);

	/*
	** same for "before" rules.
	*/
	snode->pst_before_stmt        = sess_cb->pss_row_lvl_usr_before_rules;
	psy_rl_coalesce(&snode->pst_before_stmt, 
				sess_cb->pss_row_lvl_sys_before_rules);

	snode->pst_before_statementEndRules = 
				sess_cb->pss_stmt_lvl_usr_before_rules;
	psy_rl_coalesce(&snode->pst_before_statementEndRules,
				sess_cb->pss_stmt_lvl_sys_before_rules);

	/*
	** finally, unless we are processing PREPARE, append statement-level
	** rules to the end of the list of row-level rules (after and before)
	*/
	if (sess_cb->pss_defqry != PSQ_PREPARE)
	{
	    psy_rl_coalesce(&snode->pst_after_stmt,
		snode->pst_statementEndRules);
	    psy_rl_coalesce(&snode->pst_before_stmt,
		snode->pst_before_statementEndRules);
	}
	
	sess_cb->pss_row_lvl_usr_rules  =
	sess_cb->pss_row_lvl_sys_rules  =
	sess_cb->pss_stmt_lvl_usr_rules =
	sess_cb->pss_stmt_lvl_sys_rules =
	sess_cb->pss_row_lvl_usr_before_rules  =
	sess_cb->pss_row_lvl_sys_before_rules  =
	sess_cb->pss_stmt_lvl_usr_before_rules =
	sess_cb->pss_stmt_lvl_sys_before_rules = (PST_STATEMENT *) NULL;

	/* Assign the audit information from session pointer. */
	snode->pst_audit = sess_cb->pss_audit;

        sess_cb->pss_audit = (PTR)NULL;

	snode->pst_lineno = sess_cb->pss_lineno;

	/* Allocate procedure node. */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_PROCEDURE),
	    (PTR *) &prnode, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}

	prnode->pst_mode = (i2) 0;
	prnode->pst_vsn = (i2) PST_CUR_VSN;
	prnode->pst_isdbp = FALSE;
	prnode->pst_flags = 0;
	prnode->pst_stmts = snode;
	prnode->pst_parms = (PST_DECVAR *) NULL;
	STRUCT_ASSIGN_MACRO(header->pst_cursid, prnode->pst_dbpid);

	*pnode = prnode;
    }

    if (ult_check_macro(&sess_cb->pss_trace, 16, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree:\n\n\n");
	status = pst_prmdump(qtree, header, &psq_cb->psq_error, DB_PSF_ID);
    }
    if (ult_check_macro(&sess_cb->pss_trace, 17, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree:\n\n\n");
	status = pst_1prmdump(qtree, header, &psq_cb->psq_error);
    }

    /*
    ** for distributed INGRES we need to indicate type (by site) of the query
    ** as well as provide location and translated text for single-site queries.
    */

    if (sess_cb->pss_distrib & DB_3_DDB_SESS) 
    {
	PST_DSTATE	*qsite_info;
	
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_DSTATE),
	                    (PTR *) &qsite_info, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}

	qsite_info->pst_stype    = stype;
	qsite_info->pst_ldb_desc = (DD_LDB_DESC *) NULL;  /* obsolete? */
	qsite_info->pst_qtext = (DD_PACKET *) NULL;

	/*
	** In some cases we may have something to pass to OPC when
	** processing multi-site queries, e.g. when dealing with
	** unknown options in the WITH clause in
	** CREATE TABLE...AS SELECT;  this information (if any) will be
	** pointed to by xlated_qry->pss_q_list.pss_head
	*/
	if (xlated_qry != (PSS_Q_XLATE *) NULL)
	{
	    qsite_info->pst_qtext = xlated_qry->pss_q_list.pss_head;
	}
		
	header->pst_distr = qsite_info;
    } /* if DISTRIBUTED */

/*
** 6.4 compat mode: duplicate elimination AND loose ambig replace
** checking
*/
    if (header->pst_mask1 & PST_XTABLE_UPDATE)
    {

	if (Psf_srvblk->psf_flags & PSF_AMBREP_64COMPAT ||
	    ult_check_macro(&sess_cb->pss_trace, PSS_AMBREP_64COMPAT,
				&val1, &val2))
	{
	    header->pst_mask1 &= ~PST_XTABLE_UPDATE;
	    header->pst_mask1 |= PST_UPD_CONST;
	}
	else
    	    pst_chk_xtab_upd_const_only(header);

    }
    return (status);
}

/*{
** Name: pst_modhdr - Update an existing tree header.
**
** Description:
**      This function modifies an existing tree header. In SQL "open cursor"
**	statement creates header before psy_qrymod is run. This is sort of
**	necessary, because of the way things are set up in terms of productions.
**	The header needs to be updated after running qrymod algorithm. So to
**	localize the changes this module was written.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**	  pss_auxrng			  Auxiliary range table copied to
**					  query tree's range table for
**					  statements that have gone through
**					  qrymod.
**	  pss_resrng			  Result range variable info copied to
**					  range table.
**	  pss_restab			  Result object info copied to header
**	  pss_highparm			  Largest parameter number for this tree
**	  pss_flag			  flag word containing various bitflags:
**	    PSS_AGINTREE		  set if aggregates were found in tree
**	    PSS_SUBINTREE		  set if subselect was found in tree
**	  pss_flattening_flags		bits of info that help us determine 
**					whether a given DML query should be 
**					flattened or, if processing a view 
**					definition, which bits describing 
**					flattening-related characteristics of a
**					view will be set in pst_mask1
**      psq_cb                          Pointer to the user's control block
**	  psq_mode			  Query mode is copied to header.
**	  psq_error			  Place to put error info it error
**					  happens
**      forupdate                       Type of update allowed in cursor
**					    PST_DIRECT - Direct update
**					    PST_DEFER - Deferred update
**					    PST_READONLY - No update allowed
**	tree				Pointer to existing header
**
** Outputs:
**      psq_cb
**	  .psq_error			Filled in with error info, if any.
**	tree				Updated header.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	28-aug-86 (stec)
**          written.
**	22-apr-88 (stec)
**	    pst_modhdr should update the mode in header.
**	24-oct-88 (stec)
**	    Change to code executed for cb->pss_resrng.
**	27-oct-88 (stec)
**	    Change initialization of pst_updmap in the header.
**	13-sep-90 (teresa)
**	    changed the following booleans to bitflags: pss_agintree and
**	    pss_subintree
**	08-nov-91 (rblumer)
**	  merged from 6.4:  25-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as an array of
**	    PST_RNGENTRY pointers instead of an array of structures.  It is
**	    possible, that space was not allocated for one or more entries, so
**	    it will be allocated here
**	  merged from 6.4:  11-mar-91 (andre)
**	    Overwrite information in the tree header with that from the PSF
**	    range table without checking if the entries (in the header and PSF
**	    range tables) contain the same information.  Originally, I was
**	    comparing table ids and making sure that the entry does not
**	    represent a non-mergeable view.  As it turns out, all other things
**	    being the same, the timestamp may have changed.  Rather than add one
**	    more check, we will just blindly overwrite the information.
**	16-aug-93 (andre)
**	    copy the inner and the outer relation maps from the pss_auxrng
**	    entries into the header range table entries.  Forgetting to do it
**	    resulted in errors trying to open cursors defined on views with
**	    outer joins
**	18-nov-93 (andre)
**	    added code to set (if appropriate) PST_NO_FLATTEN and PST_CORR_AGGR 
**	    bits in header->pst_mask1; if processing PREPARE SELECT or 
**	    DECLARE CURSOR, we didn't even try to set these bits in pst_header()
**	    since at that point we were yet to pump the tree through qrymod 
**	    which may discover new information which may affect the decision 
**	    whether these bits need to be set
**	jun-26-96 (inkdo01)
**	    Fixed nasty bug which resulted in overlaying other nice folks'
**	    memory with range table entries.
**	1-june-2007 (dougi)
**	    Set PST_KEYSET for scrollable cursors with implicit update. 
*/
DB_STATUS
pst_modhdr(
	PSS_SESBLK         *sess_cb,
	PSQ_CB		   *psq_cb,
	i4		   forupdate,
	PST_QTREE	   *header)
{
    DB_STATUS           status = E_DB_OK;
    register PSS_RNGTAB	*usrrange;
    PST_RNGENTRY 	*headrng;
    i4			oldcount;
    PST_RNGENTRY	**oldrangetab;
    DB_TAB_TIMESTAMP	*timestamp;
    register i4	i;
    extern PSF_SERVBLK  *Psf_srvblk;
#ifdef    xDEBUG
    i4		val1;
    i4		val2;
#endif

    /* The query mode */
    header->pst_mode = psq_cb->psq_mode;

    /* Indicate if any aggregates in tree */
    header->pst_agintree = ((sess_cb->pss_stmt_flags & PSS_AGINTREE) != 0);
    /* Indicate if any subselects in tree */
    header->pst_subintree= ((sess_cb->pss_stmt_flags & PSS_SUBINTREE) != 0);

    /*
    ** do not flatten trees which contain SUBSEL node s.t. it is a
    ** descenedant of OR node;
    ** ... the same applies to trees involving ALL ... SELECT
    ** ... the same applies to trees containing multiple correlated
    **     references in a tree rooted in EXIST which is a descendant 
    **     of NOT or a SUBSEL with count() as one of the elements of 
    **	   the target list 
    ** ... the same applies to queries involving singleton subselect(s)
    **	   if we were told at server startup that such queires should 
    **	   not be flattened
    */
    if (   (sess_cb->pss_flattening_flags &
		(PSS_SUBSEL_IN_OR_TREE | PSS_ALL_IN_TREE | PSS_MULT_CORR_ATTRS))
	|| (   Psf_srvblk->psf_flags & PSF_NOFLATTEN_SINGLETON_SUBSEL
	    && sess_cb->pss_flattening_flags & PSS_SINGLETON_SUBSELECT)
       )
    {
	header->pst_mask1 |= PST_NO_FLATTEN;
    }

    /*
    ** if sess_cb->pss_flattening_flags indicates that the query tree 
    ** contains (subselects involving aggregates in their target lists 
    ** or NOT EXISTS, NOT IN, !=ALL) and correlated attribute 
    ** references OR <expr involving an attrib> !=ALL (<subsel>), set 
    ** PST_CORR_AGGR in pst_mask1
    */
    if (sess_cb->pss_flattening_flags & PSS_CORR_AGGR)
    {
	header->pst_mask1 |= PST_CORR_AGGR;
    }

    /* Implicit update scrollable cursors need to be flagged as KEYSET. */
    if (sess_cb->pss_stmt_flags & PSS_KEYSET)
	header->pst_mask1 |= PST_KEYSET;	/* keyset scrollable cursor */

    /*
    ** We only get here after psy_qrymod() has returned, so use the auxiliary
    ** range table.
    */
    usrrange = &sess_cb->pss_auxrng.pss_rngtab[0];

    /* remember number of entries in the range table */
    oldcount = header->pst_rngvar_count;	/* save old count to avoid
						** overlaying other stge */
    oldrangetab = header->pst_rangetab;		/* & old ptr array addr */
    header->pst_rngvar_count = sess_cb->pss_auxrng.pss_maxrng + 1;
    if (oldcount < header->pst_rngvar_count)
    {					/* allocate/init new ptr array */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_RNGENTRY *)*
	    header->pst_rngvar_count, (PTR *)&header->pst_rangetab,
	    &psq_cb->psq_error);
	if (oldcount > 0)
	  MEcopy((PTR)oldrangetab, oldcount*sizeof(PST_RNGENTRY *), 
	    (PTR)header->pst_rangetab);
					/* copy old bits over */
	for (i = oldcount; i < header->pst_rngvar_count; i++)
	    header->pst_rangetab[i] = (PST_RNGENTRY *)NULL;
					/* & clear the rest */
    }

    for (i = 0, usrrange = sess_cb->pss_auxrng.pss_rngtab;
	 i < PST_NUMVARS;
	 i++, usrrange++)
    {
	/*
	** Only build the range table for those range variables that have
	** had range variable numbers assigned to them for this query.
	*/
	if (usrrange->pss_rgno != -1 && usrrange->pss_used)
	{
	    /*
	    ** allocate a range variable entry if it has not been allocated yet
	    */
	    if (header->pst_rangetab[usrrange->pss_rgno] ==
		(PST_RNGENTRY *) NULL)
	    {
		status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_RNGENTRY),
		    (PTR *) header->pst_rangetab + usrrange->pss_rgno,
		    &psq_cb->psq_error);
		if (status != E_DB_OK)
		{
		    return (status);
		}
	    }

	    headrng = header->pst_rangetab[usrrange->pss_rgno];

	    STRUCT_ASSIGN_MACRO(usrrange->pss_tabid, headrng->pst_rngvar);
	    timestamp = &usrrange->pss_tabdesc->tbl_date_modified;
	    STRUCT_ASSIGN_MACRO(*timestamp, headrng->pst_timestamp);
	    headrng->pst_rgtype = usrrange->pss_rgtype;
	    headrng->pst_rgparent = usrrange->pss_rgparent;
	    headrng->pst_rgroot = usrrange->pss_qtree;
	    if (headrng->pst_rgtype == PST_RTREE)
		header->pst_mask1 |= PST_RNGTREE;

	    /*
	    ** copy the range variable name
	    */
	    MEcopy((PTR) usrrange->pss_rgname, DB_MAXNAME,
		   (PTR) &headrng->pst_corr_name);

	    /* copy the inner and the outer relation maps */
	    STRUCT_ASSIGN_MACRO(usrrange->pss_outer_rel,headrng->pst_outer_rel);
	    STRUCT_ASSIGN_MACRO(usrrange->pss_inner_rel,headrng->pst_inner_rel);
	}
    }

    /* Update mode indicator */
    header->pst_updtmode = forupdate;

    /*
    ** no need to copy sess_cb->pss_restab since it is only used by CREATE TABLE
    ** AS SELECT and RETRIEVE INTO, and you cannot open a cursor for either
    */

    /* The result range variable, if any */
    if (sess_cb->pss_resrng == (PSS_RNGTAB *) NULL)
    {
	header->pst_restab.pst_resvno = -1;
    }
    else
    {
	/* copy some data from pss_resrng */
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
	    header->pst_restab.pst_restabid);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabname,
	    header->pst_restab.pst_resname);
	STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_ownname,
	    header->pst_restab.pst_resown);
	header->pst_restab.pst_resvno = sess_cb->pss_resrng->pss_rgno;

	/*
	** all the relevant information has already been copied from the PSF
	** range table entry (in the FOR loop above)
	*/
    }

    /* Copy the update map, if any, to the header. */
    if (sess_cb->pss_crsr != (PSC_CURBLK *) NULL)
    {
	STRUCT_ASSIGN_MACRO(sess_cb->pss_crsr->psc_iupdmap, header->pst_updmap);
    }

#ifdef	xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 16, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree after modification:\n\n\n");
	status = pst_prmdump(header->pst_qtree, header,
	    &psq_cb->psq_error, DB_PSF_ID);
    }
    if (ult_check_macro(&sess_cb->pss_trace, 17, &val1, &val2))
    {   
	/*
	** Print the final query tree.
	*/
	TRdisplay("Final query tree after modification:\n\n\n");
	status = pst_1prmdump(header->pst_qtree, header, &psq_cb->psq_error);
    }
#endif

    return (status);
}


/*{
** Name: pst_cdb_cat - is table an "iidd_*" table on the CDB?
**
** Description:
**      Check if the LDB is the CDB and the local table is prefixed by
**	"iidd_".  If so, return true, otherwise false with the following
**	exception:  there are three CDB "iidd_" tables that return FALSE
**	and these are
**	    iidd_stats, iidd_histograms and iidd_dbconstants
**
** Inputs:
**	rng_tab				Pointer to range table element
**      psq_cb                          Pointer to the user's control block
**	  psq_dfltldb			ptr to CDB descriptor
**
** Outputs:
**
**	Returns:
**	    TRUE		 	CDB table with "iidd_"prefix
**	    FAIL			non-CDB or non-"iidd_" table
**
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	11-jun-92 (barbara)
**	    Extracted out of pst_header for Sybil.
**      12-apr-95 (rudtr01)
**          Fixed bug 68036.  Changed all calls to CMcmpcase() to
**          CMcmpnocase().  Needed for FIPS installations where
**          identifiers are coerced to upper case.  Also tidied
**          the code to conform to the One True Standard.
*/
bool
pst_cdb_cat(
	PSS_RNGTAB	*rng_tab,
	PSQ_CB		*psq_cb)
{

    DD_2LDB_TAB_INFO	*ldb_tab_info =
		  	&rng_tab->pss_rdrinfo->rdr_obj_desc->dd_o9_tab_info;
    DD_LDB_DESC		*ldb_desc;
    char		*c1 = ldb_tab_info->dd_t1_tab_name;
    char		*c2 = "iidd_";

    for (; (*c2 != EOS && CMcmpnocase(c1,c2) == 0); CMnext(c1), CMnext(c2))
	    ;
	     
    if (*c2 != EOS)
    {
	return (FALSE);
    }

    if (CMcmpnocase(c1, "s") == 0)
	c2 = "stats";
    else if (CMcmpnocase(c1, "h") == 0)
	c2 = "histograms";
    else if (CMcmpnocase(c1, "d") == 0)
	c2 = "dbconstants";
    else
	c2 = (char *) NULL;

    /* if there is a reason to check */
    if (c2 != (char *) NULL)
    {
	for (; (*c2 != EOS && CMcmpnocase(c1,c2) == 0); CMnext(c1), CMnext(c2))
	    ;

	/*
	** if the search was terminated because the search
	** string was over AND there are no more non-blank
	** chars in the name we are checking, this must be
	** either iidd_histograms or iidd_stats or
	** iidd_dbconstants , so return FALSE.
	*/
	if (*c2 == EOS
	    && (c2 - ldb_tab_info->dd_t1_tab_name == sizeof(DD_NAME)
		|| CMspace(c1)
		)
	    )
	{
	    return (FALSE);
	}
    }

    /* Check site */
    ldb_desc = &ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc;

    return (psq_same_site(ldb_desc, psq_cb->psq_dfltldb));
}

/*
** Name: pst_proc_rngentry - copy contents of a range entry in PSF session CB 
**			     into a PST_RNGENTRY
**
** Description:
**	This function will copy contents of a range entry in PSF session CB 
**	into a PST_RNGENTRY and, if appropriate, update various fields in the 
**	query tree header structure
**
** Input:
**	sess_cb			PSF session CB
**	psq_cb			PSF request block
**	header			query tree header
**	usrrange		PSF range table entry information from which is 
**				to be copied into the query tree header
**	is_iidd			if processing a STAR query, this arg will 
**				contain an address of a variable used to keep 
**				track of whether the query involved a link to a
**				local table with a name starting with iidd_ and
**				which resides on CDB
**
** Output:
**	header
**	    pst_rangetab[usrrange->pss_rgno]
**				will be populated
**	*is_iidd		may be reset to TRUE
**	psq_cb.psq_errror	will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**	21-mar-94 (andre)
**	    extracted from pst_header() in the course of fixing bug 58965
*/
static DB_STATUS
pst_proc_rngentry(
		  PSS_SESBLK	*sess_cb,
		  PSQ_CB	*psq_cb,
		  PST_QTREE	*header,
		  PSS_RNGTAB	*usrrange,
		  bool		*is_iidd)
{
    PST_RNGENTRY	*headrng;
    DB_STATUS		status;

    /* allocate a range variable entry */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_RNGENTRY),
        (PTR *) header->pst_rangetab + usrrange->pss_rgno, &psq_cb->psq_error);
    if (status != E_DB_OK)
    {
        return(status);
    }

    headrng = header->pst_rangetab[usrrange->pss_rgno];
    STRUCT_ASSIGN_MACRO(usrrange->pss_tabid, headrng->pst_rngvar);

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
        headrng->pst_dd_obj_type =
    	    usrrange->pss_rdrinfo->rdr_obj_desc->dd_o6_objtype;

        if (!*is_iidd)
        {
            /*
            ** if we have not yet found a reference to a link to a iidd_* table 
            ** residing on CDB, check if this range entry describes one of these
            ** special tables.  Queries against such tables will use the 
            ** privileged association.
            */ 
            *is_iidd = pst_cdb_cat(usrrange, psq_cb); 
        }

    }

    STRUCT_ASSIGN_MACRO(usrrange->pss_tabdesc->tbl_date_modified, 
			headrng->pst_timestamp);
    headrng->pst_rgtype = usrrange->pss_rgtype;
    headrng->pst_rgparent = usrrange->pss_rgparent;
    headrng->pst_rgroot = usrrange->pss_qtree;
    if (headrng->pst_rgtype == PST_RTREE)
	header->pst_mask1 |= PST_RNGTREE;

    /*
    ** copy the range variable name
    */
    MEcopy((PTR) usrrange->pss_rgname, DB_MAXNAME,
           (PTR) &headrng->pst_corr_name);

    /* copy the inner and the outer relation maps */
    STRUCT_ASSIGN_MACRO(usrrange->pss_outer_rel, headrng->pst_outer_rel);
    STRUCT_ASSIGN_MACRO(usrrange->pss_inner_rel, headrng->pst_inner_rel);

    return(E_DB_OK);
}

/*{
** Name: pst_singlesite - compare dd_ldb_descs and set pst_stype
**
** Description:
**      Compare this descriptor to others seen and set pst_stype to
**	PST_NSITE if any mismatch.
**
** Inputs:
**	rng_tab				Pointer to range table element
**      stype                           Pointer to i4  holding pst_stype
**	save_ldb_desc			Pointer to ptr to DD_LDB_DESC
**
** Outputs:
**
**	May modify *stype and/or *save_ldb_desc
**
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	19-apr-94 (davebf)
**	    Created.
**	20-jun-94 (davebf)
**	    Enabled gateways
*/
VOID
pst_singlesite(
	PSS_RNGTAB	*rng_tab,
	i4             *stype,
	DD_LDB_DESC	**save_ldb_desc)
{

    DD_2LDB_TAB_INFO	*ldb_tab_info =
		  	&rng_tab->pss_rdrinfo->rdr_obj_desc->dd_o9_tab_info;
    DD_LDB_DESC		*ldb_desc = &ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc;

    if(ldb_desc == (DD_LDB_DESC *)NULL)
	return;

    if(*save_ldb_desc == NULL)
    {
	*save_ldb_desc = ldb_desc;	/* save pointer to first */
	return;
    }

    if(psq_same_site(ldb_desc, *save_ldb_desc))
	return;				/* if equal, leave stype alone */
    else
    {
	*stype = PST_NSITE;		/* else, change to NSITE */
	return;
    }

}
/*{
** Name: pst_chk_xtab_upd_const_only 
**
** Description:
**     	Walk completed query tree, and check if (cross-table
**	update) only applies constant values.
**
**	The Result domain (RSD) nodes start on the LH
**	branch from the ROOT of the tree: at each node,
**	the LH branch will be the next RSD (with a special
**	terminating type at end), and the RH branch will
**	be a node or further subtree indicating the value.
**
**	In order to determine if the xtable update only
**	involves constant values, we must walk down to the
**	leaves of all the value subtrees and check that they
**	are all PST_CONST.
**
**	If we find that the updates only apply constant values
**	to the target, unset the 'cross-table update' flag - 
**	there is no point to doing the checking lower down
**	in DMF. Also, set another flag to indicate to OPF
**	[in opn_halloween()] that a top-level duplicate
**	elimination can be done, reinstating previous
**	behaviour.
**	
** Inputs:
**	header	-	query tree header
** Outputs:
**	none
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	3-july-1997(angusm)
**		created.
*/
static 
void
pst_chk_xtab_upd_const_only( PST_QTREE	*header)
{
    register PST_QNODE	*qtree = header->pst_qtree;
    register PST_QNODE	*qnode;
    bool     notconst = FALSE;

/* PST_UPD_CONST */
    /* 
    outer loop: walk RSD nodes,
    but don't take leftmost
    */

    for (qnode = qtree->pst_left; qnode != (PST_QNODE*) NULL 
	    && qnode->pst_sym.pst_type == PST_RESDOM
	    && qnode->pst_left->pst_sym.pst_type != PST_TREE;
     qnode = qnode->pst_left)
    {
	/* inner loop: check leaves of RH tree at RSD node */
	if (qnode->pst_right != (PST_QNODE  *)NULL)
	{
	    notconst = walk(qnode->pst_right);
	    if (notconst == TRUE)
		break;
	}
    }
/* double negative: notconst FALSE. i.e. update only uses constant values */
    if (notconst == FALSE)
    {
	header->pst_mask1 &= ~PST_XTABLE_UPDATE;
	header->pst_mask1 |= PST_UPD_CONST;
    }
}
/*
**  recursive tree-walk
*/
static
bool
walk(PST_QNODE  *qn1)
{
	bool notconst;

	/* if we've reached leaf, terminate */
	if ((qn1->pst_left == (PST_QNODE*) NULL ) &&
	   (qn1->pst_right == (PST_QNODE*) NULL ) )
	{
		if (qn1->pst_sym.pst_type == PST_VAR)
		    return(TRUE);
		else
		    return(FALSE);
	}
	/* otherwise recurse down the branches */
	if (qn1->pst_left != NULL)
	{
	    notconst = walk(qn1->pst_left);
	    if (notconst == TRUE)
	    	return(notconst);
	}
	if (qn1->pst_right != NULL)
	{
	    notconst = walk(qn1->pst_right);
	    if (notconst == TRUE)
	    	return(notconst);
	}
	return notconst;
}
