/*
** Copyright 2004, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <bt.h>
#include    <ci.h>
#include    <er.h>
#include    <qu.h>
#include    <st.h>
#include    <tm.h>
#include    <cm.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <adfops.h>
#include    <adudate.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <usererror.h>

/*
** Name: pslpart.c - Parser utilities for constructing a partition definition
**
** Description:
**	This module contains routines for constructing a partition definition
**	as it's being parsed.  The goal is to build two main data
**	structures:
**	- a DB_PART_DEF complete with all partition names and raw value
**	  text strings, reflecting the new logical partitioning.
**	  (But see the note below.)
**	- a list of QEU_LOGPART_CHAR structures, each of which describes
**	  the physical characteristics of one or more logical partitions.
**
**	Roughly, the QEU_LOGPART_CHAR entries correspond with partition WITH-
**	clauses, and the DB_PART_DEF corresponds to the rest of the partition
**	definition.
**
**	The partition definition that the parser spits out is almost
**	complete, but it may be unusable in one important way:  the
**	partition column descriptors (the DB_PART_LIST array(s)) will not
**	be completely filled in if the query is CREATE (as opposed to
**	MODIFY).  The reason is that there's lots of "stuff" that goes
**	on with columns, length fudging and security labels, and what-not.
**	The actual for-real you-betcha column offsets are not really
**	finalized until DMF gets around to creating the table!
**	Therefore, the row_offset member will not be set for CREATE.
**	Fortunately, this is easy enough for the eventual partition-def
**	users to be aware of and fix.
**
**	The problems posed by building a DB_PART_DEF from the parse are
**	similar to, but not identical to, the problems faced by RDF in
**	reading the partitioning catalogs.  There are some similarities,
**	but the main differences are that the parse sees things in
**	definition order (by definition!), but has no idea how much stuff
**	is coming.  RDF has a better idea of how much data there is,
**	but can't count on reading catalog rows in any sort of order.
**
**	Much of the partition definition is built up using a PSF temporary
**	memory stream.  When the definition is complete, it is copied to
**	the QSF object where the rest of the query is parsed into.
**	The stream is closed after query parsing is done.
**
**	Functions defined here (in parse order):
**
**	psl_partdef_start -- Start up a partition definition
**	psl_partdef_new_dim -- Start a new dimension definition
**	psl_partdef_oncol -- An ON column name has been parsed
**	psl_partdef_partlist -- An ON column name list is done
**	psl_partdef_pname -- A partition name has been parsed
**	psl_partdef_value -- A partitioning constant has been parsed
**	psl_partdef_value_check -- A composite constant has been parsed
**	psl_partdef_nonval -- One non-value PARTITION clause is done,
**			except for the WITH clause
**	psl_partdef_with -- a partition WITH clause is done
**	psl_partdef_end -- Finish the partition definition, validate
**			everything not yet validated, copy to QSF, etc.
**
**	Be aware that none of this happens when parsing a create table
**	inside of a CREATE SCHEMA.  During CREATE SCHEMA parsing, all
**	(nearly all) semantic actions are turned off, including all of
**	the actions that call this module.
**
**	For ease of finding things, the routines in this module are arranged
**	with global routines first, local routines second, alphabetically
**	within each section.  Please maintain this organization as you add
**	new routines.
**
** History:
**	28-Jan-2004 (schka24)
**	    Create for the partitioned tables project.
**	26-Apr-2004 (schka24)
**	    Extern-ize ppd_qsfmalloc.
**	6-Jul-2006 (kschendel)
**	    Fill in unique dbid for RDF, just in case.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
*/

/* Local routine prototypes */

/* Lookup a column name, return column info */
static i2 ppd_lookup_column(
	PSS_SESBLK *sess_cb,
	i4 qmode,
	PST_QNODE *qry_tree,
	char *colname,
	i2 *col_typep);

/* Allocate memory from the partition def temporary stream */
static DB_STATUS ppd_malloc(
	i4 psize,
	void *pptr,
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);

/* Start a new breaks table entry */
static DB_STATUS ppd_new_break(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp,
	DB_PART_DIM *dim_ptr);

/* Issue memory allocation message, returns E_DB_ERROR */
static DB_STATUS ppd_ulm_errmsg(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);


/* Local definitions */
/* Various temporary memory areas are sized with a guess.  Define the
** initial size guesses here.
** The partition names and breaks table for each dimension have to be
** contiguous, so they are sized somewhat generously.  If either one
** fills up, a new larger area is allocated, and the old one thrown away.
** The text-pointer and text areas don't have to be contiguous, so
** we simply pick a reasonable size for each area.  If one fills up,
** we just create another.
*/

#define BREAKS_MEM_GUESS	100	/* Initial slots for breaks table */
#define	NAMES_MEM_GUESS		150	/* Initial slots for partition names */
#define	TEXTP_MEM_ENTRIES	200	/* Entries in a text-pointer area */
#define TEXT_MEM_SIZE		2048	/* Size of a text holding area */

/* Any local storage/tables here */


/* Name: psl_partdef_end -- End a partition definition
**
** Description:
**	After the entire partition definition is parsed, this routine
**	is called to do final validation checks and "finish" the
**	definition.
**
**	If there are any LIST or RANGE dimensions, the value breaks
**	tables involved need to be checked, and the partition values
**	need to be converted into the canonical internal form for
**	doing row comparisions.  All of this work is done by RDF when
**	it reads a partition definition from the catalogs anyway.
**	So, we'll simply give the partition definition to RDF and ask
**	it to check it out.  (RDF may allocate more memory from our
**	temporary memory stream, so it's a good idea to do this first.)
**
**	When the partition definition checks out OK, we need to copy
**	the whole thing to the same QSF object that holds the other
**	data structures being built for the query (QEU_CB, etc).
**	Again, RDF will help out, as it has a nice little partition
**	definition copier.  After the definition is copied, we just
**	need to hook it up to the QEU_CB for the query, and we're done.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query-parse control block
**	yyvarsp		Parser state variables
**
** Outputs:
**	Returns OK or error
**
** History:
**	2-Feb-2004 (schka24)
**	    Written.
**	23-Jun-2006 (kschendel)
**	    Check for too many partitions lest we cause havoc.
**	3-Dec-2007 (kschendel)
**	    Verify that each dimension has at least 2 partitions.
**	    Single-partition tables or dimensions can cause havoc in
**	    QEF, and they don't make any sense anyway.
*/

DB_STATUS
psl_partdef_end(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, PSS_YYVARS *yyvarsp)
{

    char errbuf[DB_ERR_SIZE+1];		/* Where to place error strings */
    DB_PART_DEF *part_def;		/* Partition definition address */
    DB_PART_DIM *dim_ptr;		/* Pointer to one dimension */
    DB_STATUS status;			/* Called routine status */
    DMU_CB *dmucb;			/* DMF utility info block */
    i4 i;
    i4 stride;				/* Partition stride */
    i4 toss_err;
    QEU_CB *qeucb;			/* Query QEU info block */
    RDF_CB rdfcb;			/* RDF call control block */
    RDR_INFO rdf_info;			/* Dummy RDF info block */

    part_def = yyvarsp->part_def;

    /* Start by computing a couple things in the definition that haven't
    ** been done yet - dimension stride, total number of partitions.
    ** Stride computation has to run from the inside out.
    */

    stride = 1;
    i = part_def->ndims;
    for (dim_ptr = &part_def->dimension[part_def->ndims - 1];
	 dim_ptr >= &part_def->dimension[0];
	 -- dim_ptr)
    {
	if (dim_ptr->nparts == 1)
	{
	    (void) psf_error(E_US1951_6481_PART_ONEP, 0, PSF_USERERR,
			&toss_err, &psq_cb->psq_error, 2,
			yyvarsp->qry_len, yyvarsp->qry_name,
			sizeof(i), &i);
	    return (E_DB_ERROR);
	}
	dim_ptr->stride = stride;
	stride = stride * dim_ptr->nparts;
	--i;
    }
    part_def->nphys_parts = stride;

    /* FIXME:  this should be USHRT_MAX (MAXUI2 but there ain't no such
    ** thing).  Problem is, we've defined nparts as i2 instead of u_i2
    ** in a couple key places.  Until they are all definitively found and
    ** fixed, use MAXI2 as the limit instead.
    */
    if (stride > MAXI2)
    {
	i = MAXI2;
	(void) psf_error(E_US1949_6467_PART_TOOMANYP, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 3,
		yyvarsp->qry_len, yyvarsp->qry_name,
		sizeof(stride), &stride,
		sizeof(i), &i);
	return (E_DB_ERROR);
    }

    /* Ask RDF to finish up.  This is only strictly necessary (at present)
    ** if there are LIST or RANGE dimensions, but go ahead and do the
    ** call regardless.
    */

    MEfill(sizeof(rdfcb), 0, &rdfcb);	/* Don't confuse RDF with junk */
    rdfcb.rdf_rb.rdr_db_id = sess_cb->pss_dbid;
    rdfcb.rdf_rb.rdr_unique_dbid = sess_cb->pss_udbid;
    rdfcb.rdf_rb.rdr_fcb = Psf_srvblk->psf_rdfid;
    rdfcb.rdf_rb.rdr_session_id = sess_cb->pss_sessid;
    rdfcb.rdf_rb.rdr_newpart = part_def;
    rdfcb.rdf_rb.rdf_fac_ulmrcb = (PTR) &sess_cb->pss_partdef_stream;
    rdfcb.rdf_rb.rdr_instr = RDF_USE_CALLER_ULM_RCB;
    rdfcb.rdf_rb.rdr_errbuf = &errbuf[0];
    status = rdf_call(RDF_PART_FINISH, &rdfcb);
    if (DB_FAILURE_MACRO(status))
    {
	(void) psf_error(E_US1940_6458_PART_INVALID, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 2,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, errbuf);
	return (E_DB_ERROR);
    }

    /* Everything looks good, copy a finished product into QSF */
    MEfill(sizeof(rdfcb), 0, &rdfcb);	/* Don't confuse RDF with junk */
    MEfill(sizeof(rdf_info), 0, &rdf_info);
    rdfcb.rdf_info_blk = &rdf_info;
    rdfcb.rdf_rb.rdr_db_id = sess_cb->pss_dbid;
    rdfcb.rdf_rb.rdr_unique_dbid = sess_cb->pss_udbid;
    rdfcb.rdf_rb.rdr_fcb = Psf_srvblk->psf_rdfid;
    rdfcb.rdf_rb.rdr_session_id = sess_cb->pss_sessid;
    rdfcb.rdf_rb.rdr_partcopy_mem = ppd_qsfmalloc;
    rdfcb.rdf_rb.rdr_partcopy_cb = psq_cb;	/* Can get at everything from this */
    rdfcb.rdf_rb.rdr_instr = RDF_PCOPY_NAMES | RDF_PCOPY_TEXTS;
    /* This time the original partition def is in the rdf info block,
    ** and the new copy comes back in rdfrb.
    */
    rdf_info.rdr_types = RDR_BLD_PHYS;	/* Copy function wants to see this */
    rdf_info.rdr_parts = part_def;
    status = rdf_call(RDF_PART_COPY, &rdfcb);
    if (DB_FAILURE_MACRO(status))
    {
	return (status);
    }
    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    dmucb->dmu_part_def = rdfcb.rdf_rb.rdr_newpart;
    dmucb->dmu_partdef_size = rdfcb.rdf_rb.rdr_newpartsize;

    /* Flush the temporary partition-definition memory */
    status = ulm_closestream(&sess_cb->pss_partdef_stream);
    if (DB_FAILURE_MACRO(status))
	return (status);
    sess_cb->pss_ses_flag &= ~PSS_PARTDEF_STREAM_OPEN;

    /* We are no longer doing a partition definition. */
    sess_cb->pss_stmt_flags &= ~PSS_PARTITION_DEF;

    return (E_DB_OK);
} /* psl_partdef_end */

/*
** Name: psl_partdef_new_dim -- Start a new partition dimension
**
** Description:
**	After the leading distribution-rule keyword of a new partitioning
**	dimension is parsed, this routine is called to get the new
**	dimension moving.  Mostly we just need to initialize a bunch of
**	state variables saying that we haven't seen anything in this
**	dimension yet.
**
**	A partition names array for this dimension is allocated.
**	For LIST and RANGE dimensions, a breaks table for this dimension
**	is created, with initial size BREAKS_MEM_GUESS entries.
**	The number of dimensions in the partition definition is counted
**	up, and the dimension info area initialized.
**
** Inputs:
**	sess_cb		Parser session control block
**	psq_cb		Query-parse control block
**	yyvarsp		Parser state variables
**	distrule	Distribution rule just parsed (DBDS_RULE_xxx)
**
** Outputs:
**	Returns OK or error status
**
** History:
**	29-Jan-2004 (schka24)
**	    Written.
*/

DB_STATUS
psl_partdef_new_dim(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp, i4 distrule)
{
    DB_PART_DEF *part_def;		/* Current partition definition */
    DB_PART_DIM *dim_ptr;		/* Dimension we're starting */
    DB_STATUS status;			/* Routine call status */
    i4 i;
    i4 toss_err;

    part_def = yyvarsp->part_def;

    /* Check for max dimensions */
    if (part_def->ndims == DBDS_MAX_LEVELS)
    {
	i = DBDS_MAX_LEVELS;
	(void) psf_error(E_US1933_6451_PART_TOOMANY, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 2,
		yyvarsp->qry_len, yyvarsp->qry_name,
		sizeof(i), &i);
	return (E_DB_ERROR);
    }
    dim_ptr = &part_def->dimension[part_def->ndims];
    ++ part_def->ndims;

    yyvarsp->names_cur = 0;		/* No names seen yet */
    yyvarsp->textptrs_cur = 0;		/* No break values started yet */
    yyvarsp->partseq_first = 0;		/* No logical partitions yet */

    /* Sequence numbers are 1-origined, so setting the "last one used"
    ** to zero does the trick -- the next partseq will be 1.
    */
    yyvarsp->partseq_last = 0;		/* "last" partseq was zero */

    /* Init some dimension stuff */
    dim_ptr->distrule = distrule;
    dim_ptr->nparts = 0;
    dim_ptr->ncols = 0;
    dim_ptr->nbreaks = 0;
    dim_ptr->value_size = 0;
    dim_ptr->text_total_size = 0;
    dim_ptr->part_list = NULL;

    /* Make a holding area for this dimension's names.  All the names
    ** for one dimension must be contiguous.
    ** (The names area for the prior dimension is probably not full, but
    ** so what...)
    */
    status = ppd_malloc(NAMES_MEM_GUESS * sizeof(DB_NAME),
			&dim_ptr->partnames,
			sess_cb, psq_cb);
    if (DB_FAILURE_MACRO(status))
	return (status);
    yyvarsp->part_name_next = dim_ptr->partnames;
    yyvarsp->names_size = NAMES_MEM_GUESS * sizeof(DB_NAME);
    yyvarsp->names_left = NAMES_MEM_GUESS;

    /* For LIST or RANGE, make a holding area for this dimension's
    ** breaks table.  All breaks for a dimension must be contiguous.
    ** As with the names, there might be space in the last dimension's
    ** breaks holding area, but so what.
    */
    if (distrule == DBDS_RULE_AUTOMATIC || distrule == DBDS_RULE_HASH)
	dim_ptr->part_breaks = NULL;
    else
    {
	status = ppd_malloc(BREAKS_MEM_GUESS * sizeof(DB_PART_BREAKS),
			&dim_ptr->part_breaks,
			sess_cb, psq_cb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	yyvarsp->part_breaks_next = dim_ptr->part_breaks;
	yyvarsp->breaks_size = BREAKS_MEM_GUESS * sizeof(DB_PART_BREAKS);
	yyvarsp->breaks_left = BREAKS_MEM_GUESS;
    }

    /* We should be ready for partition definitions */

    return (E_DB_OK);
} /* psl_partdef_new_dim */

/*
** Name: psl_partdef_nonval -- End a "non-value" partition spec
**
** Description:
**	The AUTOMATIC and HASH distribution types don't use explicit
**	partitioning values to decide where things go.  Therefore there's
**	not much to defining partitions:  all we need is how many,
**	and what they're called.  This routine is called after one
**	of those "non-value" partition specs has been parsed, up to but
**	not including the partition WITH clause:
**
**	  PARTITION foo
**	or
**	  128 PARTITIONS (foo1, foo2, foo3, ...)
**
**	This is not necessarily the end of the dimension, as the user is
**	allowed to use multiple partition specs.  (Perhaps some are in a
**	different location, or have other differences in the WITH-options.)
**
**	After either checking the user provided names, or generating
**	some, we'll set up the starting and ending partition sequence
**	numbers (1 origin) for this batch of logical partitions.  If
**	there is a WITH-clause, it will apply to each logical partition
**	in this partition-spec.  The name counter is reset for the next
**	batch, if there is one.
**
** Inputs:
**	sess_cb		Parser session state block
**	psq_cb		Query-parse control block
**	yyvarsp		Parser state variables
**	nparts		Number of logical partitions to define
**
** Outputs:
**	The usual ok/error status is returned.
**
** History:
**	30-Jan-2004 (schka24)
**	    New for partitioned tables.
*/

DB_STATUS
psl_partdef_nonval(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp, i4 nparts)
{
    DB_PART_DIM *dim_ptr;		/* Address of partition dimension */
    DB_STATUS status;			/* Routine call status */
    i4 i;
    i4 toss_err;

    dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];
    if (yyvarsp->names_cur > 0)
    {
	/* Names were given, numbers have to match */
	/* FIXME ***** perhaps if the user list is short, we generate
	** names?  although that sounds bogus, and I'm not doing it now.
	*/
	if (nparts != yyvarsp->names_cur)
	{
	    (void) psf_error(E_US1936_6454_PART_NNAMES, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 3,
		yyvarsp->qry_len, yyvarsp->qry_name,
		sizeof(nparts), &nparts,
		sizeof(yyvarsp->names_cur), &yyvarsp->names_cur);
	    return (E_DB_ERROR);
	}
    }
    else
    {
	char generated_name[DB_MAXNAME + 1];

	/* No names at all, make some up.
	** Names are generated with the form iipartNN, where NN is an
	** arbitrary number that has no meaning at all.
	** Pretend the name was parsed in the usual manner, so that
	** we don't have to deal with growing the array, etc.
	*/
	for (i = 0; i < nparts; ++i)
	{
	    STprintf(generated_name,"iipart%d",yyvarsp->name_gen);
	    ++ yyvarsp->name_gen;
	    status = psl_partdef_pname(sess_cb, psq_cb, yyvarsp,
			generated_name, TRUE);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
    }
    yyvarsp->names_cur = 0;		/* reset for next clause */

    /* The last partition clause stopped at partition sequence
    ** partseq_last.  Define the range of partition sequence numbers
    ** created by this clause, in case there's a WITH coming.
    */
    yyvarsp->partseq_first = yyvarsp->partseq_last + 1;
    yyvarsp->partseq_last = yyvarsp->partseq_first + nparts - 1;
    dim_ptr->nparts = dim_ptr->nparts + nparts;

    return (E_DB_OK);
} /* psl_partdef_nonval */

/*
** Name: psl_partdef_oncol -- Add an ON column to the current dimension
**
** Description:
**	All of the value-based partitioning methods (everything but
**	AUTOMATIC) require a column name list.  This routine is called
**	after each column name is scanned.  We need to verify that the
**	column name is OK, and add its number to the column list
**	being built.  (The column numbers are attribute numbers, 1-origin.)
**	So far we're only building a column list, it will get turned into
**	a DB_PART_LIST once we have all the columns.
**
**	The things to check are:
**	- It's really a valid column name.  (This can be fairly simple,
**	  or quite exciting, depending on the statement being parsed!)
**	- It's not a duplicate (in this dimension).
**	- It has to be usable as a key (this excludes non-keyable or
**	  non-sortable things like long varchar).
**	- For LIST and RANGE, it can't be a logical key type or other
**	  "magic cookie" types.
**
**	Finding a column name depends on what query the partition definition
**	appears in.  Briefly:
**
**	CREATE INDEX: look in the DMU_CB being built for the query, the
**		index column names are in dmu_key_array.  (We don't
**		allow partitioned indexes, yet, so this info is for
**		future reference.)  For data type information, we can
**		use ordinary column-lookup (pst_coldesc) which will run
**		against the base table.
**
**	CREATE TABLE:  (the ordinary kind)  look in the DMU_CB being built
**		for the query, the column definitions are in dmu_attr_array.
**
**	CREATE TABLE AS SELECT: (i.e. PSQ_RETINTO) This one is fun.
**		You have to look into the select query-expr to get the
**		names.  The "finish" actions for the CREATE haven't run
**		yet, so the select query-expr isn't attached to anything!
**		Fortunately, the grammar actions are kind enough to jam
**		the query-expr pointer into the parser state (yyvarsp),
**		into part-crtas-qtree.
**
**	MODIFY: perhaps the easiest, as the table already exists.  By
**		the time we get to the partition definition, the
**		table name has been parsed, and so there's RDF definition
**		stuff around.  We can simply use the "lookup column"
**		utility pst_coldesc to get at the column definition.
**
**	For the two CREATE TABLE variations, additional WITH- or CREATE-
**	post-processing actions may result in more columns being added
**	to the table!  (Security labels.)  Fortunately they are always
**	added at the end, and do not upset our attribute numbers here.
**
**	By the way, the table name can always be found in the DMU_CB,
**	no matter what statement is being executed.  (For CREATE INDEX,
**	the table name is the index name, which is what we want for
**	error messages.)
**		
** Inputs:
**	sess_cb		Parser session state block
**	psq_cb		Query-parse control block
**	yyvarsp		Parser state variables
**	colname		Textual column name
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	29-Jan-2004 (schka24)
**	    Written for partitioned tables.
*/

DB_STATUS
psl_partdef_oncol(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp, char *colname)
{

    DB_PART_DIM *dim_ptr;		/* Address of partition dimension */
    DB_STATUS status;			/* Called routine status */
    DMU_CB *dmucb;			/* DMF dmu instructions */
    i2 att_number;			/* Column ID number */
    i2 col_type;			/* Column datatype */
    i2 *colarray_ptr;			/* Column number-array pointer */
    i4 toss_err;
    QEU_CB *qeucb;			/* QEF qeu instructions */

    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];

    /* Find the column based on the name */
    att_number = ppd_lookup_column(sess_cb, psq_cb->psq_mode,
		yyvarsp->part_crtas_qtree, colname,
		&col_type);
    if (att_number == 0)
    {
	/* Didn't work, column must not be in the table */
	(void) psf_error(E_US1934_6452_PART_BADCOL, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 5,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, "Column",
		STlength(colname), colname,
		0, "not found",
		psf_trmwhite(sizeof(DB_TAB_NAME), dmucb->dmu_table_name.db_tab_name),
		dmucb->dmu_table_name.db_tab_name);
	return (E_DB_ERROR);
    }

    /* See if we know it already */
    for (colarray_ptr = &yyvarsp->part_cols[dim_ptr->ncols - 1];
	 colarray_ptr >= &yyvarsp->part_cols[0];
	 -- colarray_ptr)
    {
	if (att_number == *colarray_ptr)
	{
	    (void) psf_error(E_US1934_6452_PART_BADCOL, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 5,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, "Column",
		STlength(colname), colname,
		0, "duplicated",
		psf_trmwhite(sizeof(DB_TAB_NAME), dmucb->dmu_table_name.db_tab_name),
		dmucb->dmu_table_name.db_tab_name);
	    return (E_DB_ERROR);
	}
    }

    /* Check out the data type */
    status = psl_check_key(sess_cb, &psq_cb->psq_error, (DB_DT_ID) col_type);
    if (DB_SUCCESS_MACRO(status))
    {
	/* For list or range, disallow logical keys and table keys,
	** because there's no rational way to specify constants or expect
	** legal values.  Disallow bit only because I don't have time or
	** inclination to fool with it.
	*/
	col_type = abs(col_type);
	if ((dim_ptr->distrule == DBDS_RULE_LIST || dim_ptr->distrule == DBDS_RULE_RANGE)
	  && (col_type == DB_LOGKEY_TYPE
		|| col_type == DB_TABKEY_TYPE
		|| col_type == DB_BIT_TYPE
		|| col_type == DB_VBIT_TYPE))
	    status = E_DB_ERROR;
    }
    if (DB_FAILURE_MACRO(status))
    {
	(void) psf_error(E_US1935_6453_PART_BADCOLTYPE, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 2,
		yyvarsp->qry_len, yyvarsp->qry_name,
		STlength(colname), colname);
	return (E_DB_ERROR);
    }

    /* Looks OK, add this column to the list */
    yyvarsp->part_cols[dim_ptr->ncols] = att_number;
    STcopy(colname,&yyvarsp->part_col_names[dim_ptr->ncols * (DB_MAXNAME+1)]);
    ++ dim_ptr->ncols;

    return (E_DB_OK);
} /* psl_partdef_oncol */

/*
** Name: psl_partdef_partlist -- Generate DB_PART_LIST for dimension
**
** Description:
**	After an ON column list is finished, we can generate the column
**	descriptors for the partitioning dimension - the DB_PART LIST.
**	This little array will have one entry per ON column, with type
**	and offset details.  The array will get ppd-malloc'ed and
**	attached to the new dimension.
**
**	Important note!  One thing we aren't attempting to compute in
**	the CREATE TABLE case is the row_offset, the position of the
**	column in a row image.  For a canonical DB_PART_LIST (as opposed
**	to one that has been amended by OPC or QEF), the row_offset
**	is supposed to be position of the column in a row image as
**	supplied by DMF.  The thing is, if the table hasn't been created
**	yet, we don't know it!  There's lots of processing to be done
**	still on table create, and the column row offsets aren't assigned
**	until DMF goes to actually create the table.
**
**	For the MODIFY case, since the table does exist and we know the
**	row offsets, we'll go ahead and compute them.
**
**	See ppd_lookup_column for some of the details about where column
**	data is stored for the various statement types.  Fortunately
**	we already know the column number, which makes life a little
**	simpler in most cases.
**
**	The parser only calls this after an ON clause, no need to check
**	the distribution rule.
**
**
** Inputs:
**	sess_cb		Parser session state block
**	psq_cb		Query-parse control block
**	yyvarsp		Parser state variables
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	30-Jan-2004 (schka24)
**	    Written for partitioned tables.
*/

DB_STATUS
psl_partdef_partlist(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, PSS_YYVARS *yyvarsp)
{

    DB_PART_DIM *dim_ptr;		/* Pointer to current dimension */
    DB_PART_LIST *part_list_ptr;	/* Pointer to a list entry */
    DB_STATUS status;			/* The usual status thing */
    DMT_ATT_ENTRY *dmt_att;		/* DMT/RDF style attribute descriptor */
    DMF_ATTR_ENTRY *dmu_att;		/* DMU style attribute descriptor */
    DMU_CB *dmucb;			/* DMF dmu instructions */
    i2 att_number;			/* Column number, 1-origin */
    i2 col_prec;			/* Column precision if applicable */
    i2 col_type;			/* Column datatype */
    i2 col_width;			/* Column width in bytes */
    i2 val_offset;			/* Internal form value offset */
    i4 ncols;				/* Number of ON columns */
    i2 *colarray_ptr;			/* Pointer to column number list */
    PST_QNODE *qry_node;		/* Scan select query tree */
    QEU_CB *qeucb;			/* QEF qeu instructions */

    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];
    ncols = dim_ptr->ncols;

    /* Allocate memory for the column list */

    status = ppd_malloc(ncols * sizeof(DB_PART_LIST), &part_list_ptr,
		sess_cb, psq_cb);
    if (DB_FAILURE_MACRO(status))
	return (status);
    dim_ptr->part_list = part_list_ptr;

    /* Go through the list, fill in the entries */
    colarray_ptr = yyvarsp->part_cols;
    val_offset = 0;
    while (--ncols >= 0)
    {
	att_number = *colarray_ptr++;	/* Get the next column number */
	part_list_ptr->row_offset = 0;	/* Assume unknown */

	/* Play games to get at the proper column data */
	switch (psq_cb->psq_mode)
	{
	case PSQ_CREATE:
	    /* CREATE TABLE (normal, not as-select) */
	    /* Find the column in the DMU CB - note that it doesn't have
	    ** the zero throw-away slot that pss-attdesc does.
	    */
	    dmu_att = *( (DMF_ATTR_ENTRY **) dmucb->dmu_attr_array.ptr_address + att_number - 1);
	    col_prec = dmu_att->attr_precision;
	    col_type = dmu_att->attr_type;
	    col_width = dmu_att->attr_size;
	    break;

	case PSQ_INDEX:
	    /* Create index doesn't get here */
	    /* Notimp, see ppd_lookup_column for how to do it when we need it */
	    break;

	case PSQ_MODIFY:
	    /* Modify */
	    /* Find the column in the usual RDF/DMT descriptor */
	    dmt_att = sess_cb->pss_resrng->pss_attdesc[att_number];
	    col_prec = dmt_att->att_prec;
	    col_type = dmt_att->att_type;
	    col_width = dmt_att->att_width;
	    /* For MODIFY we know the row offset */
	    part_list_ptr->row_offset = dmt_att->att_offset;
	    break;

	case PSQ_RETINTO:
	    /* Create table as select */
	    /* This one is completely different.  Walk the left-links in the
	    ** SELECT query tree;  as long as RESDOMs are seen, those are the
	    ** result columns.  We could count off columns, backwards, but
	    ** the column number is in the resdom entry, use that.
	    ** The data type is in the node's dataval.
	    */
	    qry_node = yyvarsp->part_crtas_qtree->pst_left;
	    while (qry_node != NULL && qry_node->pst_sym.pst_type == PST_RESDOM)
	    {
		/* Looking at a resdom, see if it's the one we want */
		if (qry_node->pst_sym.pst_value.pst_s_rsdm.pst_rsno == att_number)
		{
		    /* Found it */
		    col_prec = qry_node->pst_sym.pst_dataval.db_prec;
		    col_type = qry_node->pst_sym.pst_dataval.db_datatype;
		    col_width = qry_node->pst_sym.pst_dataval.db_length;
		    break;
		}
		/* Not yet, try the one to the left */
		qry_node = qry_node->pst_left;
	    }
	    break;
	} /* switch */

	/* Fill in the partition column list */
	part_list_ptr->att_number = att_number;
	part_list_ptr->type = col_type;
	part_list_ptr->precision = col_prec;
	part_list_ptr->length = col_width;
	part_list_ptr->val_offset = val_offset;

	/* Allow for the column in a partitioning constant and align
	** where the next column will go
	*/
	val_offset = val_offset + col_width;
	val_offset = (val_offset + (DB_ALIGN_SZ-1)) & ( ~(DB_ALIGN_SZ-1) );

	++ part_list_ptr;
    } /* while */

    /* Store the partitioning constant value size and we're done */
    dim_ptr->value_size = val_offset;

    return (E_DB_OK);
} /* psl_partdef_partlist */

/*
** Name: psl_partdef_pname -- Store a partition name
**
** Description:
**	After a partition name is parsed, we are called to store it
**	into the dimension's partition name table.   If there's not
**	enough room, a new larger name area is allocated, and the old
**	one is thrun away.
**
**	There's not much validation to do on names.  The caller has
**	already checked for "reserved" names, in the sense of no funny
**	leading dollar-signs.  We need to check for an ii-name and
**	disallow it.  (Partition names don't mean much as a syntax
**	element;  but they are optional.  Auto-generated names start with
**	an ii prefix.)  We also need to check for duplicates.
**
** Inputs:
**	sess_cb		Parser session control block
**	psq_cb		Query-parse control block
**	yyvarsp		Parser state variables
**	pname		The partition name string
**	allow_ii	TRUE to allow ii-names, FALSE otherwise
**			(It's easiest to generate names by stuffing them
**			thru this routine, hence the flag.)
**
** Outputs:
**	Returns OK or error
**
** History:
**	29-Jan-2004 (schka24)
**	    Written
**	5-Jan-2005 (schka24)
**	    It might not be a bad idea to check for memory expansion
**	    needed before moving the new name in, rather than after.
**	25-Apr-2007 (kschendel) SIR 122890
**	    Fix dumb bug that allowed duplicate partition names.  Code
**	    was confused about what names_cur was counting (only current
**	    [nonval-]partition's names, not names in the dimension).
*/

DB_STATUS
psl_partdef_pname(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp, char *pname, bool allow_ii)
{

    bool is_ii_name;			/* TRUE if an ii-name */
    char *p1;				/* Next-char in name */
    DB_NAME *names_ptr;			/* Name array to dup-check */
    DB_NAME *new_names;			/* Relocated/bigger name array */
    DB_NAME temp_name;			/* Holder for space filled name */
    DB_PART_DIM *dim_ptr;		/* Pointer to dimension info */
    DB_STATUS status;			/* Called routine status */
    DMU_CB *dmucb;			/* DMU info area with table name */
    i4 nnames;				/* Number of names to dup-check */
    i4 toss_err;
    QEU_CB *qeucb;			/* QEF qeu info block */

    /* Find the DMU CB being built, might need it */

    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;

    p1 = pname;
    CMnext(p1);
    is_ii_name = FALSE;
    if (CMcmpnocase(pname,&SystemCatPrefix[0]) == 0
	  && CMcmpnocase(p1,&SystemCatPrefix[1]) == 0)
	is_ii_name = TRUE;

    if ( (! allow_ii) && is_ii_name)
    {
	(void) psf_error(E_PS0459_ILLEGAL_II_NAME, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 4,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, "partition",
		STlength(pname), pname,
		STlength(SystemCatPrefix), SystemCatPrefix);
	return (E_DB_ERROR);
    }

    /* Space-fill names */
    STmove(pname, ' ', sizeof(DB_NAME), &temp_name.db_name[0]);

    /* Check for a duplicate name, against all of the names we have in
    ** all dimensions.  Only the current dimension's names are in the
    ** current holding area, so some slight whirling is needed.
    ** No need to check generated ii-names.
    */
    if ( ! is_ii_name)
    {
	/* First trip thru, include names listed so far for current
	** partition declaration (e.g. partition (a,b,c,a) 4 partitions,
	** if we're checking the second "a", the first 3 names aren't
	** counted in nparts yet).
	*/
	dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];
	nnames = yyvarsp->names_cur + dim_ptr->nparts;
	for (;;) {
	    names_ptr = dim_ptr->partnames;
	    while (--nnames >= 0)
	    {
		if (MEcmp(temp_name.db_name,
				names_ptr->db_name, sizeof(DB_NAME)) == 0)
		{
		    (void) psf_error(E_US1934_6452_PART_BADCOL,0,PSF_USERERR,
				&toss_err, &psq_cb->psq_error, 5,
				yyvarsp->qry_len, yyvarsp->qry_name,
				0, "Partition name",
				STlength(pname), pname,
				0, "duplicated",
				psf_trmwhite(sizeof(DB_TAB_NAME), dmucb->dmu_table_name.db_tab_name),
				dmucb->dmu_table_name.db_tab_name);
		    return (E_DB_ERROR);
		}
		++ names_ptr;
	    }
	    /* This dimension checks out OK, try prior one */
	    if (dim_ptr == &yyvarsp->part_def->dimension[0])
		break;
	    -- dim_ptr;
	    nnames = dim_ptr->nparts;
	} /* for */
    }

    if (yyvarsp->names_left == 0)
    {
	/* Oops.  The names area filled up.  Make it bigger. */
	dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];
	status = ppd_malloc(yyvarsp->names_size + NAMES_MEM_GUESS * sizeof(DB_NAME),
			&new_names,
			sess_cb, psq_cb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	MEcopy(dim_ptr->partnames, yyvarsp->names_size, new_names);
	yyvarsp->part_name_next = (DB_NAME *) ((PTR) new_names
			+ ((PTR) yyvarsp->part_name_next - (PTR) dim_ptr->partnames));
	dim_ptr->partnames = new_names;
	yyvarsp->names_left = NAMES_MEM_GUESS;
	yyvarsp->names_size = yyvarsp->names_size + NAMES_MEM_GUESS * sizeof(DB_NAME);
    }
    /* Ok, move the new name in */
    MEcopy(temp_name.db_name, sizeof(DB_NAME), yyvarsp->part_name_next->db_name);
    -- yyvarsp->names_left;
    ++ yyvarsp->part_name_next;
    ++ yyvarsp->names_cur;
    return (E_DB_OK);
} /* psl_partdef_pname */

/*
** Name: psl_partdef_start -- Start a new partition definition
**
** Description:
**	This routine is called when a PARTITION= WITH option list prefix
**	is seen.  A partition definition is built up, slowly and
**	laboriously, in a bunch of temporary memory areas, using
**	numerous state variables in the YYVARS structure.  We need
**	to allocate a bunch of holding tanks, and init the state
**	variables.  Specifically, we open a temp (PSF) memory stream
**	and allocate:
**	- a partition definition (DB_PART_DEF) with the maximum allowed
**	  number of dimensions;
**	- a partitioning value pointer area,  size TEXTP_MEM_ENTRIES
**	- a partitioning value text area, size TEXT_MEM_SIZE
**	- a column number holding array, one i2 per possible column
**	- a column name holding array (mostly for error messages), one
**	  DB_MAXNAME+1 (asciz) per possible column
**	- a location name area, allowing DM_LOC_MAX DB_LOC_NAME entries
**
**	A partition name area is needed, but each dimension needs a
**	fresh one, so that's done in the new-dimension routine.
**	There's also a breaks table area to allocate, but that's done
**	when a LIST or RANGE dimension is seen.  They are the only
**	ones that use breaks tables, and each dimension needs a fresh
**	one anyway.
**
**	For the outside world, the PSS_PARTITION_DEF flag is set in the
**	PSS_SESBLK pss_stmt_flags showing that we're in a partition
**	definition.
**
** Inputs:
**	sess_cb		The session's PSS_SESBLK control block
**	psq_cb		The query-parse control block
**	yyvarsp		The parser state structure
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	29-Jan-2004 (schka24)
**	    Another outburst of typing for the partitioned tables project.
*/

DB_STATUS
psl_partdef_start(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, PSS_YYVARS *yyvarsp)
{

    DB_STATUS status;			/* The usual return status */

    /* Set up a memory stream for holding all the definitions as
    ** we go.  There should not be a stream open already;  if there
    ** is, someone screwed up, but just close it and start a fresh one.
    */
    if (sess_cb->pss_ses_flag & PSS_PARTDEF_STREAM_OPEN)
    {
	/* Perhaps exception recovery forgot to close it? */
	(void) ulm_closestream(&sess_cb->pss_partdef_stream);
    }
    sess_cb->pss_partdef_stream.ulm_facility = DB_PSF_ID;
    sess_cb->pss_partdef_stream.ulm_poolid = Psf_srvblk->psf_poolid;
    sess_cb->pss_partdef_stream.ulm_blocksize = 0;
    sess_cb->pss_partdef_stream.ulm_memleft = &sess_cb->pss_memleft;
    sess_cb->pss_partdef_stream.ulm_streamid_p = &sess_cb->pss_partdef_stream.ulm_streamid;
    /* One dimension is built into the db-part-def */
    sess_cb->pss_partdef_stream.ulm_psize = sizeof(DB_PART_DEF)
			+ (DBDS_MAX_LEVELS-1) * sizeof(DB_PART_DIM);
    /* No other thread will see this stream, make it private */
    sess_cb->pss_partdef_stream.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    status = ulm_openstream(&sess_cb->pss_partdef_stream);
    if (DB_FAILURE_MACRO(status))
	return (ppd_ulm_errmsg(sess_cb, psq_cb));
    yyvarsp->part_def = (DB_PART_DEF *) sess_cb->pss_partdef_stream.ulm_pptr;
    sess_cb->pss_ses_flag |= PSS_PARTDEF_STREAM_OPEN;
    yyvarsp->part_def->nphys_parts = 0;
    yyvarsp->part_def->ndims = 0;
    yyvarsp->part_def->part_flags = 0;

    /* Column array, used to build up a dimension's ON-columns.  Since
    ** all that's in here is an i2, just allocate it to the max number
    ** of columns.
    */
    status = ppd_malloc(DB_MAX_COLS * sizeof(i2), &yyvarsp->part_cols,
			sess_cb, psq_cb);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Do the same for column names, which are a bit larger, but oh well. */
    status = ppd_malloc(DB_MAX_COLS * (DB_MAXNAME+1), &yyvarsp->part_col_names,
			sess_cb, psq_cb);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Text-pointer holding area.  There's one value-text pointer in a
    ** breaks table entry, but the "value" might be a composite if there
    ** are multiple partitioning columns.  So the breaks table points to
    ** a little array of N-columns pointers to each of the components
    ** of the constant value.  We're allocating the place to put those
    ** text pointers.
    */
    status = ppd_malloc(TEXTP_MEM_ENTRIES * sizeof(DB_TEXT_STRING *),
			&yyvarsp->part_textptr_next,
			sess_cb, psq_cb);
    if (DB_FAILURE_MACRO(status))
	return (status);
    yyvarsp->textptrs_left = TEXTP_MEM_ENTRIES;

    /* Partition value text holding area.  Partitioning values are stored
    ** as raw text, rather than as internal ints, etc.  (At least initially.)
    ** So we save up the actual text of the tokens making up a partitioning
    ** value;  that's what is being allocated here.
    */
    status = ppd_malloc(TEXT_MEM_SIZE, &yyvarsp->part_text_next,
			sess_cb, psq_cb);
    if (DB_FAILURE_MACRO(status))
	return (status);
    yyvarsp->text_left = TEXT_MEM_SIZE;

    /* Individual partitions, or groups of partitions, can have physical
    ** storage structure characteristics.  (In fact, conceptually, any
    ** physical storage definition above the lowest physical partition
    ** level is just a template;  the real partition is ultimately what
    ** gets structured.)  We need someplace to put the info defined by a
    ** WITH clause appearing on an individual partition definition.
    ** For now, due partly to system catalog limitations, the only
    ** WITH option allowed on a partition is LOCATION=.  Allocate an
    ** area to hold the location names as they are parsed.  After
    ** each WITH-clause, the location names are moved elsewhere, so
    ** we only need room for one set.
    */
    status = ppd_malloc(DM_LOC_MAX * sizeof(DB_LOC_NAME),
			&yyvarsp->part_locs.data_address,
			sess_cb, psq_cb);
    if (DB_FAILURE_MACRO(status))
	return (status);
    yyvarsp->part_locs.data_in_size = 0;

    /* Lots of errmsgs want the query name for context, get it once: */
    psl_command_string(psq_cb->psq_mode, sess_cb->pss_lang,
			&yyvarsp->qry_name[0], &yyvarsp->qry_len);

    /* In case we need to generate names, start up a counter */
    yyvarsp->name_gen = 1;

    /* Ready to rock... */
    sess_cb->pss_stmt_flags |= PSS_PARTITION_DEF;

    return (E_DB_OK);
} /* psl_partdef_start */

/*
** Name: psl_partdef_value -- Store a component of a partitioning value
**
** Description:
**	RANGE and LIST distributions use user-specified constant values
**	to define the partitioning scheme.  The exact manner in which
**	they work is irrelevant for the moment, because our job is to
**	deal with the constant values.
**
**	A partition constant may look like: thing (if there is just one
**	column in the ON list), or: (thing, thing, ...) if there are many
**	columns in the ON list.  This routine gets called for a single
**	"thing", ie one column component of a multicolumn constant.
**
**	The (composite) constant is pointed to by a breaks table entry.
**	There's one breaks entry for every constant.  So, unless one
**	has been started already, we have to set up a breaks table entry.
**	Each breaks table entry has an operation code as well, which the
**	grammar actions have kindly stashed in yyvars for us.
**
**	Ultimately we want a textual version of the constant, because
**	that's how partitioning values are stored in the iidistval
**	catalog.  (And it stores text because it's easier to look at,
**	and perhaps more portable.  And because I perhaps mistakenly
**	thought to mimic the way defaults work.)  User default values
**	work this way by asking the scanner to save a copy of the
**	input text in a text chain.  Text chains are very inefficient
**	for short bits of text, though;  and without some significant
**	work on the grammar, it's hard if not impossible to get the
**	text-emit flag turned on in time!  (Because of lookahead.)
**
**	So, we'll use grammar productions that result in db-data-values,
**	and deal with them.  Signed numbers are a minor difficulty, as
**	the scanner returns them as a - token and a positive number,
**	but with a little cleverness (modeled after what Doug did for
**	sequences) the sign can be handled easily.  Another possible
**	"value" is the keyword NULL, which is passed as a null data
**	value pointer.  Eventually it would be nice to handle program
**	variables (QDATA), but for now it's not supported (for no special
**	reason other than lack of time to figure it out).
**
**	We have to make sure that the constant is appropriate for the
**	column datatype, which we do fairly simplistically.  Then, the
**	constant is string-ized and placed into the value text strings
**	that we're gathering up.  One special case occurs here with DATE.
**	Date literals are strings, and we could store what the user entered,
**	except that dates are way too sensitive to the current date
**	format context.  It would be a bad thing to have rows sent to
**	different partitions, depending on what date format a user
**	happened to be using!  So, we'll string-ize a DATE by first
**	coercing the string to a true date (which we need to do anyway,
**	to make sure it's valid), and then coercing it back using the
**	date_gmt function.  Date_gmt style dates (yyyy_mm_dd) are always
**	interpreted the same way regardless of date format context.
**	There's more trickiness involving the time part, but see in-line.
**
**	There's one more special case to handle, and that's DEFAULT.
**	DEFAULT is stored as an operator, not as a text string.  But,
**	just to be consistent, we'll store empty text strings for
**	DEFAULT.  Also, in a multi-column context, we only see one
**	DEFAULT keyword.  I.e. it's DEFAULT, not (DEFAULT, DEFAULT, ...).
**	So DEFAULT has to generate "ncols" empty text strings.
**
**	Storing partitioning values as text does make the catalogs more
**	portable, and avoids some kinds of float/decimal rounding and
**	printing issues;  but it sure can be a PITA sometimes!
**
** Inputs:
**	sess_cb		The session's PSS_SESBLK control block
**	psq_cb		The query-parse control block
**	yyvarsp		The parser state structure
**	sign_flag	Zero, or ADI_MINUS_OP, or ADI_PLUS_OP
**	value		Pointer to DB_DATA_VALUE, or null for NULL
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	30-jan-2004 (schka24)
**	    Written.
**	18-Mar-2004 (schka24)
**	    Add extra room to date_gmt var to allow for alignment.
**	22-Mar-2004 (schka24)
**	    I sorta forgot to update text-left, fix.
**	10-may-04 (inkdo01)
**	    Align tempbuf for validating money, float, dec values.
**	22-Jun-2004 (schka24)
**	    Dates without a time part need to elide the time from the
**	    date_gmt result.
**	18-Feb-2005 (schka24)
**	    Pass type-names more properly to printf, might have caused
**	    segv under Winders.
**      20-Dec-2006 (kschendel)
**          Allow NULL for non-nullable if CTAS, since an outer join can
**          make a result column nullable and we don't know until opf is
**          done with it.
**	12-oct-2007 (dougi)
**	    Add support for new ANSI date/time types.
*/

DB_STATUS
psl_partdef_value(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp, i4 sign_flag, DB_DATA_VALUE *value)
{

    ADF_CB *adfcb;			/* Parser ADF control block */
    ADI_DT_NAME col_dtname, val_dtname;	/* Data type names for msgs */
    char buf[DB_MAX_DIST_VAL_LENGTH+1];  /* Useful largeish multipurpose area */
    char *col_name;			/* Column name string */
    char date_gmt[25+4+1];		/* date_gmt version of date */
    char *what;				/* LIST or RANGE for errmsgs */
    DB_DATA_VALUE gmt_val;		/* Date_gmt-ized date string */
    DB_DATA_VALUE result_val;		/* Native form constant value */
    DB_PART_BREAKS *breaks_ptr;		/* Breaks table entry pointer */
    DB_PART_LIST *part_entry_ptr;	/* Partition column entry pointer */
    DB_PART_DIM *dim_ptr;		/* This dimension definition */
    DB_STATUS status;			/* Called routine status */
    DB_TEXT_STRING *text_str;		/* Where stored text goes */
    i2 col_len;				/* Column length (omitting null) */
    i2 col_type;			/* Column type (not-null) */
    i2 val_len;				/* Value length (omitting null) */
    i2 val_type;			/* Value type (not-null) */
    i4 i;
    i4 toss_err;
    i8 int_val;				/* Integer value holder */
    u_i2 aligned_len;			/* Length of db-text-string plus null */
    u_i2 copy_len;			/* Length of db-text-string */
    u_i2 text_len;			/* Textual length of constant */

    dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];

    /* If this is the first component of a value, we'll need another
    ** breaks table entry for it.  If not, we already started one.
    */
    if (yyvarsp->textptrs_cur == 0)
    {
	/* Make new breaks entry, check holding tanks. */
	status = ppd_new_break(sess_cb, psq_cb, yyvarsp, dim_ptr);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }
    breaks_ptr = yyvarsp->part_breaks_next - 1;

    /* If this is the DEFAULT operator, special case -- set all text
    ** values for this break to an empty string. 
    */
    if (breaks_ptr->oper == DBDS_OP_DFLT)
    {
	DB_TEXT_STRING **textp_putter;
	u_i2 total_len;

	if (yyvarsp->textptrs_cur != 0)
	{
	    (void) psf_error(E_US1930_6448_PARTITION_EXPECTS, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 3,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, "a LIST or RANGE value",
		0, "DEFAULT");
	    return (E_DB_ERROR);
	}
	aligned_len = sizeof(u_i2) + 1;
	aligned_len = (aligned_len + sizeof(u_i2)-1) & (~(sizeof(u_i2)-1));
	total_len = aligned_len * dim_ptr->ncols;
	/* If there's not enough room, make another holding area */
	if (total_len > yyvarsp->text_left)
	{
	    status = ppd_malloc(TEXT_MEM_SIZE, &yyvarsp->part_text_next,
			sess_cb, psq_cb);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    yyvarsp->text_left = TEXT_MEM_SIZE;
	}
	/* Store an empty DB_TEXT_STRING with a zero null indicator */
	text_str = yyvarsp->part_text_next;
	yyvarsp->part_text_next = (DB_TEXT_STRING *) ((PTR) yyvarsp->part_text_next + total_len);
	yyvarsp->text_left -= total_len;
	dim_ptr->text_total_size = dim_ptr->text_total_size + total_len;
	textp_putter = breaks_ptr->val_text;
	for (i=0; i < dim_ptr->ncols; ++i)
	{
	    text_str->db_t_count = 0;
	    text_str->db_t_text[0] = '\0';
	    *textp_putter++ = text_str;
	    text_str = (DB_TEXT_STRING *) ((PTR) text_str + aligned_len);
	}
	yyvarsp->textptrs_cur = 0;
	return (E_DB_OK);
    }

    /* Make sure there aren't too many components of the value */
    if (yyvarsp->textptrs_cur == dim_ptr->ncols)
    {
	/* We're going to have too many.  Make it look bad for the value-
	** check routine, and let it issue the message.
	*/
	++ yyvarsp->textptrs_cur;
	(void) psl_partdef_value_check(sess_cb, psq_cb, yyvarsp);
	return (E_DB_ERROR);
    }

    what = "LIST";
    if (dim_ptr->distrule == DBDS_RULE_RANGE)
	what = "RANGE";
    adfcb = sess_cb->pss_adfcb;

    part_entry_ptr = &dim_ptr->part_list[yyvarsp->textptrs_cur];
    col_name = &yyvarsp->part_col_names[yyvarsp->textptrs_cur * (DB_MAXNAME+1)];

    /* Check for NULL first, and handle it specially. */
    if (value == NULL)
    {
	if (part_entry_ptr->type > 0 && psq_cb->psq_mode != PSQ_RETINTO)
	{
	    CL_ERR_DESC toss_cl;

	    /* Put together a decent message */
	    status = ERslookup(E_AD1012_NULL_TO_NONNULL, NULL, ER_TEXTONLY,
		NULL, buf, DB_MAX_DIST_VAL_LENGTH, 1, &i, &toss_cl, 0, NULL);
	    if (DB_SUCCESS_MACRO(status))
		buf[i] = '\0';
	    else
		STcopy("NULL is not allowed with a non-NULL column", buf);
	    (void) psf_error(E_US1938_6456_PART_VALTYPE, 0, PSF_USERERR,
			&toss_err, &psq_cb->psq_error, 4,
			yyvarsp->qry_len, yyvarsp->qry_name,
			0, what, 0, col_name, 0, buf);
	    return (E_DB_ERROR);
	}
	else if (part_entry_ptr->type > 0)
	{
	    /* (implying that psq-mode is PSQ_RETINTO)
	    ** We allow NULL for a non-nullable column if this is create table
	    ** as select.  The reason is that a result column might be
	    ** derived from an outer join's inner, making it nullable,
	    ** and there's no way to know until the optimizer is done with
	    ** its outer join analysis.
	    ** So -- for this case make the column nullable in the partition
	    ** def, and if that ends up being wrong the optimizer will
	    ** have to either detect it or let it crap out at first usage.
	    */
	    part_entry_ptr->type = -part_entry_ptr->type;
	    ++ part_entry_ptr->length;
	    /* part-offsets for CTAS are bogus until opc, don't try to fix */
	}
	aligned_len = sizeof(u_i2) + 1;
	aligned_len = (aligned_len + sizeof(u_i2)-1) & (~(sizeof(u_i2)-1));
	/* If there's not enough room, make another holding area */
	if (aligned_len > yyvarsp->text_left)
	{
	    status = ppd_malloc(TEXT_MEM_SIZE, &yyvarsp->part_text_next,
			sess_cb, psq_cb);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    yyvarsp->text_left = TEXT_MEM_SIZE;
	}
	/* Store an empty DB_TEXT_STRING with a null indicator */
	text_str = yyvarsp->part_text_next;
	yyvarsp->part_text_next = (DB_TEXT_STRING *) ((PTR) yyvarsp->part_text_next + aligned_len);
	yyvarsp->text_left -= aligned_len;
	dim_ptr->text_total_size = dim_ptr->text_total_size + aligned_len;
	*(breaks_ptr->val_text + yyvarsp->textptrs_cur) = text_str;
	text_str->db_t_count = 0;
	text_str->db_t_text[0] = (char) 1;
	++ yyvarsp->textptrs_cur;
	return (E_DB_OK);
    }

    /* Get data types and length, ignoring nullability. */
    col_len = part_entry_ptr->length;
    col_type = part_entry_ptr->type;
    if (col_type < 0)
    {
	col_type = -col_type;
	-- col_len;
    }
    val_len = value->db_length;
    val_type = value->db_datatype;
    if (val_type < 0)
    {
	val_type = -val_type;
	-- val_len;
    }
    if (val_type == DB_LTXT_TYPE)
	val_len = val_len - sizeof(u_i2);  /* Take off count i2 */

    /* Check value length if string-y for toolong;  this is fairly important
    ** because partition columns can be much larger than what fits in the
    ** iidistval catalog.
    */
    if (val_type == DB_LTXT_TYPE && val_len > DB_MAX_DIST_VAL_LENGTH)
    {
	MEcopy(value->db_data, 17, buf);
	STcopy("...", &buf[17]);
	text_len = DB_MAX_DIST_VAL_LENGTH;
	(void) psf_error(E_US1937_6455_PART_VALTOOLONG, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 5,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, what,
		0, buf,
		0, col_name,
		sizeof(text_len), &text_len);
	return (E_DB_ERROR);
    }

    /* Do some type checking.  At present, the grammar only allows
    ** numbers and strings to appear in the constant.
    */

    switch (col_type)
    {
    case DB_INT_TYPE:
	/* Int column requires an int constant */
	if (val_type != DB_INT_TYPE)
	    goto type_error;
	/* Can't just check lengths if column is i1, because the scanner
	** returns i2 or i4 or i8 constants.  We'll check i1 later.
	*/
	if (col_len < val_len && col_len != 1)
	    goto type_error;
	break;

    case DB_DEC_TYPE:
    case DB_FLT_TYPE:
    case DB_MNY_TYPE:
	/* These can be pretty much any number-ish type.  Make sure that
	** the input isn't a string constant, ADF might possibly convert
	** it but we don't want to allow it.
	*/
	if (val_type == DB_LTXT_TYPE)
	    goto type_error;
	/* Work up a db-data-value for the column result and stuff it
	** through an ADF type-checker.
	*/
	result_val.db_datatype = col_type;
	result_val.db_length = col_len;
	result_val.db_prec = part_entry_ptr->precision;
	result_val.db_data = (PTR) ME_ALIGN_MACRO(&buf[0], sizeof(i8));
	status = adc_defchk(adfcb, value, &result_val);
	/* FIXME really should look at what error we got in the adfcb */
	if (DB_FAILURE_MACRO(status))
	    goto type_error;
	break;

    case DB_BYTE_TYPE:
    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_TXT_TYPE:
    case DB_VBYTE_TYPE:
    case DB_VCH_TYPE:
	/* ***FIXME not sure about the unicode types?  do they need stuff
	** done to them??
	*/
	/* String-ish types just need the lengths checked. */
	/* Adjust column length for the length count if it's a counted
	** string type.  We did this for val-len so need to be consistent.
	*/
	if (col_type == DB_NVCHR_TYPE || col_type == DB_TXT_TYPE
	  || col_type == DB_VBYTE_TYPE || col_type == DB_VCH_TYPE)
	    col_len = col_len - DB_CNTSIZE;
	if (val_type != DB_LTXT_TYPE || val_len > col_len)
	    goto type_error;
	break;

    case DB_DTE_TYPE:
	/* Dates, convert the input to a date to make sure it's OK,
	** then convert the date back to a normalized form.
	** Use "internal" date variable to avoid alignment problems!
	*/
	{
	    AD_DATENTRNL date_val;		/* Converted date constant */

	    if (val_type != DB_LTXT_TYPE)
		goto type_error;
	    result_val.db_datatype = DB_DTE_TYPE;
	    result_val.db_length = DB_DTE_LEN;
	    result_val.db_prec = 0;
	    result_val.db_data = (PTR) &date_val;
	    status = adc_cvinto(adfcb, value, &result_val);
	    if (DB_FAILURE_MACRO(status))
	    {
		/* Make sure null-terminated string... */
		if (adfcb->adf_errcb.ad_emsglen <= 0)
		    STcopy("unknown ADF error",buf);
		else
		    adfcb->adf_errcb.ad_errmsgp[adfcb->adf_errcb.ad_emsglen] = '\0';
		(void) psf_error(E_US1938_6456_PART_VALTYPE, 0, PSF_USERERR,
			&toss_err, &psq_cb->psq_error, 4,
			yyvarsp->qry_len, yyvarsp->qry_name,
			0, what, 0, col_name,
			0, adfcb->adf_errcb.ad_errmsgp);
		return (E_DB_ERROR);
	    }
	    /* Now gmt-ize it.  It turns out that you don't get a choice of
	    ** result type from adu_dgmt, the result is a char(25).
	    ** We'll make it look like a longtext.
	    */
	    text_str = (DB_TEXT_STRING *) ME_ALIGN_MACRO((&date_gmt[0]), sizeof(u_i2));
	    gmt_val.db_datatype = DB_CHA_TYPE;
	    gmt_val.db_length = 25;	/* Hardwired into adu_dgmt */
	    gmt_val.db_data = (PTR) &text_str->db_t_text[0];
	    status = adu_dgmt(adfcb, &result_val, &gmt_val);
	    if (DB_FAILURE_MACRO(status))
	    {
		/* Make sure null-terminated string... */
		if (adfcb->adf_errcb.ad_emsglen <= 0)
		    STcopy("unknown ADF error",buf);
		else
		    adfcb->adf_errcb.ad_errmsgp[adfcb->adf_errcb.ad_emsglen] = '\0';
		(void) psf_error(E_US1938_6456_PART_VALTYPE, 0, PSF_USERERR,
			&toss_err, &psq_cb->psq_error, 4,
			yyvarsp->qry_len, yyvarsp->qry_name,
			0, what, 0, col_name,
			0, adfcb->adf_errcb.ad_errmsgp);
		return (E_DB_ERROR);
	    }
	    /* Here's yet more special casing.  date_gmt always gives you
	    ** a time part, whether the original date had one or not.
	    ** I've always thought this was wrong, but it's what it does.
	    ** It's best to trim away this time part if the original date
	    ** didn't have one.
	    ** Consider a range like <= '31-Jan-2004'.  If we don't trim
	    ** away the time part, it turns into <= '2004_01_31 00:00:00 GMT',
	    ** and now there are potential issues with timezones, etc.
	    */
	    text_str->db_t_text[25] = '\0';
	    if ((date_val.dn_status & AD_DN_TIMESPEC) == 0)
		text_str->db_t_text[10] = '\0';
	    text_str->db_t_count = STtrmwhite((char *) text_str->db_t_text);
	    /* Now it's a longtext like the rest... */
	    gmt_val.db_datatype = DB_LTXT_TYPE;
	    gmt_val.db_data = (PTR) text_str;
	    value = &gmt_val;
	}
	break;

    case DB_ADTE_TYPE:
    case DB_TMWO_TYPE:
    case DB_TMW_TYPE:
    case DB_TME_TYPE:
    case DB_TSWO_TYPE:
    case DB_TSW_TYPE:
    case DB_TSTMP_TYPE:
    case DB_INYM_TYPE:
    case DB_INDS_TYPE:
	/* ANSI dates, convert the input to a date to make sure it's OK,
	** then convert the date back to a normalized form.
	*/
	{
	    union {
		AD_ADATE	date_val;
		AD_TIME		time_val;
		AD_TIMESTAMP	timestamp_val;
		AD_INTYM	intym_val;
		AD_INTDS	intds_val;
		} dtval;

	    /* Assign various lengths by type. */
	    switch (col_type) {
	      case DB_ADTE_TYPE:
		result_val.db_length = sizeof(dtval.date_val);
		gmt_val.db_length = AD_2ADTE_OUTLENGTH;
		break;
	      case DB_TMWO_TYPE:
		result_val.db_length = sizeof(dtval.time_val);
		gmt_val.db_length = AD_3TMWO_OUTLENGTH;
		break;
	      case DB_TMW_TYPE:
		result_val.db_length = sizeof(dtval.time_val);
		gmt_val.db_length = AD_4TMW_OUTLENGTH;
		break;
	      case DB_TME_TYPE:
		result_val.db_length = sizeof(dtval.time_val);
		gmt_val.db_length = AD_5TME_OUTLENGTH;
		break;
	      case DB_TSWO_TYPE:
		result_val.db_length = sizeof(dtval.timestamp_val);
		gmt_val.db_length = AD_6TSWO_OUTLENGTH;
		break;
	      case DB_TSW_TYPE:
		result_val.db_length = sizeof(dtval.timestamp_val);
		gmt_val.db_length = AD_7TSW_OUTLENGTH;
		break;
	      case DB_TSTMP_TYPE:
		result_val.db_length = sizeof(dtval.timestamp_val);
		gmt_val.db_length = AD_8TSTMP_OUTLENGTH;
		break;
	      case DB_INYM_TYPE:
		result_val.db_length = sizeof(dtval.intym_val);
		gmt_val.db_length = AD_9INYM_OUTLENGTH;
		break;
	      case DB_INDS_TYPE:
		result_val.db_length = sizeof(dtval.intds_val);
		gmt_val.db_length = AD_10INDS_OUTLENGTH;
		break;
	    }
	    result_val.db_datatype = col_type;
	    result_val.db_prec = 0;

	    if (val_type != col_type)
	    {
		if (val_type != DB_LTXT_TYPE)
		    goto type_error;
		result_val.db_data = (PTR) &dtval;
		status = adc_cvinto(adfcb, value, &result_val);
		if (DB_FAILURE_MACRO(status))
		{
		    /* Make sure null-terminated string... */
		    if (adfcb->adf_errcb.ad_emsglen <= 0)
			STcopy("unknown ADF error",buf);
		    else
			adfcb->adf_errcb.ad_errmsgp[adfcb->adf_errcb.ad_emsglen] = '\0';
		    (void) psf_error(E_US1938_6456_PART_VALTYPE, 0, PSF_USERERR,
			&toss_err, &psq_cb->psq_error, 4,
			yyvarsp->qry_len, yyvarsp->qry_name,
			0, what, 0, col_name,
			0, adfcb->adf_errcb.ad_errmsgp);
		    return (E_DB_ERROR);
		}
	    }
	    else result_val.db_data = value->db_data;

	    /* Now return it to normalized string type.
	    ** We'll make it look like a longtext (cuz Karl wants it that way).
	    */
	    text_str = (DB_TEXT_STRING *) ME_ALIGN_MACRO((&date_gmt[0]), sizeof(u_i2));
	    gmt_val.db_datatype = DB_CHA_TYPE;
	    gmt_val.db_data = (PTR) &text_str->db_t_text[0];
	    status = adc_cvinto(adfcb, &result_val, &gmt_val);
	    if (DB_FAILURE_MACRO(status))
	    {
		/* Make sure null-terminated string... */
		if (adfcb->adf_errcb.ad_emsglen <= 0)
		    STcopy("unknown ADF error",buf);
		else
		    adfcb->adf_errcb.ad_errmsgp[adfcb->adf_errcb.ad_emsglen] = '\0';
		(void) psf_error(E_US1938_6456_PART_VALTYPE, 0, PSF_USERERR,
			&toss_err, &psq_cb->psq_error, 4,
			yyvarsp->qry_len, yyvarsp->qry_name,
			0, what, 0, col_name,
			0, adfcb->adf_errcb.ad_errmsgp);
		return (E_DB_ERROR);
	    }
	    text_str->db_t_count = gmt_val.db_length;
	    /* Now it's a longtext like the rest... */
	    gmt_val.db_datatype = DB_LTXT_TYPE;
	    gmt_val.db_data = (PTR) text_str;
	    value = &gmt_val;
	}
	break;

    default:
	goto type_error;

    } /* switch */

    /* If the input value was a number, string-ize it and take care
    ** of any minus sign.
    */
    text_str = (DB_TEXT_STRING *) ME_ALIGN_MACRO((&buf[0]), sizeof(u_i2));
    switch (val_type)
    {
    case DB_INT_TYPE:
	if (val_len == 1)
	    int_val = *(i1 *) value->db_data;
	else if (val_len == 2)
	    int_val = *(i2 *) value->db_data;
	else if (val_len == 4)
	    int_val = *(i4 *) value->db_data;
	else
	    int_val = *(i8 *) value->db_data;
	if (sign_flag == ADI_MINUS_OP)
	    int_val = -int_val;
	if (col_len == 1)
	{
	    /* Continue the type check we didn't finish for i1 columns */
	    if (int_val < MINI1 || int_val > MAXI1)
		goto type_error;
	}
	CVla8(int_val,text_str->db_t_text);
	text_str->db_t_count = STlength((char *) text_str->db_t_text);
	value->db_data = (PTR) text_str;
	break;

    case DB_DEC_TYPE:
    case DB_FLT_TYPE:
    case DB_MNY_TYPE:
	result_val.db_data = (PTR) &text_str->db_t_text[0];
	result_val.db_length = DB_MAX_DIST_VAL_LENGTH-2;
	result_val.db_datatype = DB_LTXT_TYPE;
	status = adc_cvinto(adfcb, value, &result_val);
	if (DB_FAILURE_MACRO(status))
	{
	    /* Not really supposed to happen... */
	    /* Make sure null-terminated string... */
	    if (adfcb->adf_errcb.ad_emsglen <= 0)
		STcopy("unknown ADF error",buf);
	    else
		adfcb->adf_errcb.ad_errmsgp[adfcb->adf_errcb.ad_emsglen] = '\0';
	    (void) psf_error(E_US1938_6456_PART_VALTYPE, 0, PSF_USERERR,
		    &toss_err, &psq_cb->psq_error, 4,
		    yyvarsp->qry_len, yyvarsp->qry_name,
		    0, what, 0, col_name,
		    0, adfcb->adf_errcb.ad_errmsgp);
	    return (E_DB_ERROR);
	}
	/* If a minus was wanted, simply insert it by hand.  This avoids
	** weird machinations with decimals, at the cost of a little loop.
	*/
	if (sign_flag == ADI_MINUS_OP)
	{
	    u_char *p1, *p2;

	    p2 = &text_str->db_t_text[text_str->db_t_count];
	    p1 = p2 - 1;
	    while (p2 > &text_str->db_t_text[0])
		*p2-- = *p1--;
	    text_str->db_t_text[0] = '-';
	}
	value = &result_val;
	break;

    /* Others are already ltext in value */
    } /* switch */

    /* Ok, the dataval of the stringified constant is now pointed to by
    ** value.  Value->db_data points to a DB_TEXT_STRING with a length
    ** in it.  (value->db_length is not necessarily meaningful.)
    */

    /* Stash the text into the holding area, making a new one if
    ** need be.  We need space for the actual bytes, plus the DB_TEXT_STRING
    ** length counter, plus one for a null indicator in case the constant
    ** happens to be NULL.
    */
    text_len = ((DB_TEXT_STRING *) (value->db_data))->db_t_count;
    copy_len = text_len + sizeof(u_i2);

    aligned_len = copy_len + 1;		/* Allow for null indicator */
    /* i2-align the length for the next string so that text always starts
    ** at an i2 boundary.
    */
    aligned_len = (aligned_len + sizeof(u_i2)-1) & (~(sizeof(u_i2)-1));
    /* If there's not enough room, make another holding area */
    if (aligned_len > yyvarsp->text_left)
    {
	status = ppd_malloc(TEXT_MEM_SIZE, &yyvarsp->part_text_next,
			sess_cb, psq_cb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	yyvarsp->text_left = TEXT_MEM_SIZE;
    }
    /* Make a DB_TEXT_STRING out of the constant text */
    text_str = yyvarsp->part_text_next;
    yyvarsp->part_text_next = (DB_TEXT_STRING *) ((PTR) yyvarsp->part_text_next + aligned_len);
    yyvarsp->text_left -= aligned_len;
    dim_ptr->text_total_size = dim_ptr->text_total_size + aligned_len;
    MEcopy(value->db_data, copy_len, text_str);
    text_str->db_t_text[text_len] = 0; /* Null byte, if nullable */

    /* Set the pointer to this text string and we're finally done! */

    *(breaks_ptr->val_text + yyvarsp->textptrs_cur) = text_str;
    ++ yyvarsp->textptrs_cur;

    return (E_DB_OK);


/* Here for data type errors, done out-of-line for clarity */
type_error:
    adi_tyname(adfcb, col_type, &col_dtname.adi_dtname[0]);
    adi_tyname(adfcb, val_type, &val_dtname.adi_dtname[0]);
    /* FIXME ideally would ERslookup a string with substitution */
    STprintf(buf,"Column: %s length %d;  Value: %s length %d",
		&col_dtname.adi_dtname[0], col_len, &val_dtname.adi_dtname[0], val_len);
    (void) psf_error(E_US1938_6456_PART_VALTYPE, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 4,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, what, 0, col_name,
		0, buf);
    return (E_DB_ERROR);

} /* psl_partdef_value */

/* Name: psl_partdef_value_check -- Check composite value
**
** Description:
**	The number of individual value tokens in a composite partitioning
**	value must match the number of ON-columns for the dimension.
**	This action is called when the parser thinks that it's seen
**	the end of one value.  All we need to do at this point is
**	verify that the correct number of value bits were seen.
**	It's also called when the value token action sees that too
**	many values are on the way.
**
**	Since missing value errors might possibly be hard to find, a
**	modicum of effort has been put into the error message.
**
** Inputs:
**	sess_cb		The session's PSS_SESBLK control block
**	psq_cb		The query-parse control block
**	yyvarsp		The parser state structure
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	30-jan-2004 (schka24)
**	    Written.
*/

DB_STATUS
psl_partdef_value_check(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp)
{

    char buf[80+1];			/* Arbitrary length for msg */
    DB_PART_DIM *dim_ptr;		/* This dimension definition */
    i4 toss_err;

    dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];

    if (yyvarsp->textptrs_cur == dim_ptr->ncols)
    {
	yyvarsp->textptrs_cur = 0;	/* Reset for next one */
	return (E_DB_OK);		/* All is well */
    }

    /* Oops, wrong number of components in the value, issue a message. */

    psl_yerr_buf(sess_cb, buf, sizeof(buf) - 10);
    (void) psf_error(E_US1939_6457_PART_VALCOUNT, 0, PSF_USERERR,
	&toss_err, &psq_cb->psq_error, 4,
	yyvarsp->qry_len, yyvarsp->qry_name,
	sizeof(dim_ptr->ncols), &dim_ptr->ncols,
	sizeof(yyvarsp->textptrs_cur), &yyvarsp->textptrs_cur,
	0, buf);
    return (E_DB_ERROR);

} /* psl_partdef_value_check */

/* Name: psl_partdef_with -- Process partition WITH clause
**
** Description:
**	After a partition WITH clause is parsed, we need to save the
**	structure options that were specified.  This is done by creating
**	a QEU_LOGPART_INFO block and copying all the info that the
**	user gave, into that block.  At this point in the partition definition
**	parsing, all of the necessary info (dimension, partition sequence)
**	is ready in the partition definition.
**
**	For now, the only option allowed by the option scanners during
**	a partition definition is LOCATION=.  The location list will
**	be pointed to by the part_locs descriptor.
**
** Inputs:
**	sess_cb			PSS_SESBLK parser session control block
**	psq_cb			Query-parse control block
**	yyvarsp			Parser state variables
**
** Outputs:
**	Returns OK or error status
**
** History:
**	2-Feb-2004 (schka24)
**	    Written.  (Almost done with partition definition parsing...)
*/

DB_STATUS
psl_partdef_with(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, PSS_YYVARS *yyvarsp)
{

    DB_PART_DIM *dim_ptr;		/* Partition dimension info */
    DB_STATUS status;			/* The usual routine status */
    QEU_CB *qeucb;			/* Query QEU info block */
    QEU_LOGPART_CHAR *qeu_char;		/* Pointer to new QEU block */

    qeucb = (QEU_CB *) sess_cb->pss_object;
    dim_ptr = &yyvarsp->part_def->dimension[yyvarsp->part_def->ndims - 1];

    /* If no options, return, but the grammar should not allow this */
    if (! PSS_WC_ANY_MACRO(&yyvarsp->with_clauses))
	return (E_DB_OK);

    /* Allocate an info block to tell QEF what to do */
    status = ppd_qsfmalloc(psq_cb, sizeof(QEU_LOGPART_CHAR), &qeu_char);
    if (DB_FAILURE_MACRO(status))
	return (status);

    qeu_char->dim = yyvarsp->part_def->ndims - 1;
    qeu_char->partseq = yyvarsp->partseq_first;
    qeu_char->nparts = yyvarsp->partseq_last - yyvarsp->partseq_first + 1;
    /* Zero what we don't use (yet) */
    MEfill(sizeof(DM_DATA), 0, &qeu_char->char_array);
    MEfill(sizeof(DM_PTR), 0, &qeu_char->key_array);

    /* See if LOCATION= given */

    if (PSS_WC_TST_MACRO(PSS_WC_LOCATION, &yyvarsp->with_clauses))
    {
	/* take care of LOCATION= specification; copy the location name
	** array into QSF.
	*/
	status = ppd_qsfmalloc(psq_cb,
		yyvarsp->part_locs.data_in_size * sizeof(DB_LOC_NAME),
		&qeu_char->loc_array.data_address);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	MEcopy(yyvarsp->part_locs.data_address,
		yyvarsp->part_locs.data_in_size * sizeof(DB_LOC_NAME),
		qeu_char->loc_array.data_address);
	qeu_char->loc_array.data_in_size = yyvarsp->part_locs.data_in_size;
	qeu_char->loc_array.data_out_size = 0;
	/* Now that the data is safe, reset for next time */
	yyvarsp->part_locs.data_in_size = 0;
    }

    /* Other structural characteristics, eventually ... */

    /* All done, link the block into the QEU chain */
    qeu_char->next = qeucb->qeu_logpart_list;
    qeucb->qeu_logpart_list = qeu_char;

    /* Reset the WITH parsing state to the outer WITH's state */
    yyvarsp->list_clause = yyvarsp->save_list_clause;
    MEcopy(&yyvarsp->save_with_clauses, sizeof(PSS_WITH_CLAUSE),
		&yyvarsp->with_clauses);

    return (E_DB_OK);
} /* psl_partdef_with */

/* Name: ppd_lookup_column -- Find a column by name
**
** Description:
**	This routine is given a column name and the current query mode,
**	and returns the column number and its type.
**
**	Finding a column name depends on what query the partition definition
**	appears in.  Briefly:
**
**	CREATE INDEX: look in the DMU_CB being built for the query, the
**		index column names are in dmu_key_array.  (We don't
**		allow partitioned indexes, yet, so this info is for
**		future reference.)  For data type information, we can
**		use ordinary column-lookup (pst_coldesc) which will run
**		against the base table.
**
**	CREATE TABLE:  (the ordinary kind)  look in the DMU_CB being built
**		for the query, the column definitions are in dmu_attr_array.
**
**	CREATE TABLE AS SELECT: (i.e. PSQ_RETINTO) This one is fun.
**		You have to look into the select query-expr to get the
**		names.  The "finish" actions for the CREATE haven't run
**		yet, so the select query-expr isn't attached to anything!
**		Fortunately, the grammar actions are kind enough to jam
**		the query-expr pointer into the parser state (yyvarsp),
**		into part-crtas-qtree.
**
**	MODIFY: perhaps the easiest, as the table already exists.  By
**		the time we get to the partition definition, the
**		table name has been parsed, and so there's RDF definition
**		stuff around.  We can simply use the "lookup column"
**		utility pst_coldesc to get at the column definition.
**
** Inputs:
**	sess_cb		Parser session control block
**	qmode		PSQ_xxx query mode
**	qry_tree	SELECT query tree (only relevant for CREATE AS)
**	colname		The column name string
**	col_typep	An output
**
** Outputs:
**	Returns column number (1-origin), or zero if not found
**	col_typep	Filled in with column datatype
**
** History:
**	29-Jan-2004 (schka24)
**	    Written.
*/

static i2
ppd_lookup_column(PSS_SESBLK *sess_cb, i4 qmode, PST_QNODE *qry_tree,
	char *colname, i2 *col_typep)
{

    DB_ATT_NAME fullname;		/* Blank-extended name */
    DMT_ATT_ENTRY *dmt_att_ptr;		/* Attr entry in standard RDF info */
    DMT_ATT_ENTRY **dmt_attrs;		/* Ptr to DMT-style attr-ptr array */
    DMF_ATTR_ENTRY **attrs;		/* Pointer to DMU attr-pointer array */
    DMU_CB *dmucb;			/* DMF dmu instructions */
    i4 i;
    i4 ncols;
    PST_QNODE *qry_node;		/* Scan select query tree */
    QEU_CB *qeucb;			/* QEF qeu instructions */

    /* Find the DMU CB being built, might need it */

    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;

    /* Column names are typically space-padded out */
    STmove(colname, ' ', sizeof(DB_ATT_NAME), fullname.db_att_name);

    /* Do the right thing depending on what query we have */
    switch (qmode)
    {
    case PSQ_CREATE:
	/* CREATE TABLE (normal, not as-select) */
	/* Find the column in the DMU CB */
	attrs = (DMF_ATTR_ENTRY **) dmucb->dmu_attr_array.ptr_address;
	ncols = dmucb->dmu_attr_array.ptr_in_count;
	for (i = 0; i < ncols; ++i, ++attrs)
	{
	    if (MEcmp(fullname.db_att_name, (*attrs)->attr_name.db_att_name,
			sizeof(DB_ATT_NAME)) == 0)
	    {
		/* Found it */
		*col_typep = (*attrs)->attr_type;
		return (i + 1);		/* One origin please */
	    }
	}
	return (0);			/* Not found */

    case PSQ_INDEX:
	/* Create index */
	/* Notimp, see intro for how to do it when we need it */
	return (0);

    case PSQ_MODIFY:
	/* Modify */
	/* Pretty easy, ask column looker-upper for the column */
	dmt_att_ptr = pst_coldesc(sess_cb->pss_resrng, &fullname);
	if (dmt_att_ptr == NULL)
	    return (0);			/* Not found */
	*col_typep = dmt_att_ptr->att_type;
	/* Well, it was almost easy.  We need the column number!  It's
	** still worth doing the coldesc hash thing, since it's a lot
	** quicker to find a pointer than it is comparing column names.
	** Note: "show" attr numbers and indexes run from 1 to N, so
	** there's a junk attr slot to skip.
	*/
	ncols = sess_cb->pss_resrng->pss_tabdesc->tbl_attr_count;
	dmt_attrs = sess_cb->pss_resrng->pss_attdesc + 1;
	for (i=0; i < ncols; ++i, ++dmt_attrs)
	    if (dmt_att_ptr == *dmt_attrs)
		return (i+1);		/* One origin please */
	/* Huh?? */
	return (0);

    case PSQ_RETINTO:
	/* Create table as select */
	/* This one is completely different.  Walk the left-links in the
	** SELECT query tree;  as long as RESDOMs are seen, those are the
	** result columns.  The walk is actually from right to left in the
	** select result list, but the column number is in the resdom
	** entry, so we don't have to do the counting.
	** The data type is in the node's dataval.
	*/
	qry_node = qry_tree->pst_left;	/* Move off the root */
	while (qry_node != NULL && qry_node->pst_sym.pst_type == PST_RESDOM)
	{
	    /* Looking at a resdom, see if it's the one we want */
	    if (MEcmp(fullname.db_att_name,
			qry_node->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			sizeof(DB_ATT_NAME)) == 0)
	    {
		/* Found it */
		*col_typep = qry_node->pst_sym.pst_dataval.db_datatype;
		return (qry_node->pst_sym.pst_value.pst_s_rsdm.pst_rsno);
	    }
	    /* Not yet, try the one to the left */
	    qry_node = qry_node->pst_left;
	}
	return (0);
    }

    /* Shouldn't get here... */
    return (0);
} /* ppd_lookup_column */

/* Name: ppd_malloc -- Memory allocation utility
**
** Description:
**	This is just a convenience utility to allocate memory from the
**	partition definition temporary stream.  If the allocation
**	fails, an error message is issued or set up.
**
**	Everything in the partition definition that will ultimately
**	be copied with the partition-def-copy function should be
**	allocated with ppd_malloc.  There's no point in trying to be
**	cute, even if the ultimate size of the object is known for sure,
**	because copy-partdef is going to make a new one anyway.
**	The only things in this module that aren't allocated out of the
**	temporary stream are the WITH-option blocks, since they aren't
**	considered part of a partition definition.
**
** Inputs:
**	psize		Size in bytes needed
**	pptr		an output
**	sess_cb		Session control block
**	psq_cb		Query-parse control block, needed for errors
**
** Outputs:
**	Returns E_DB_OK or error
**	pptr		Where the resulting area address should be stored
**
** History:
**	29-Jan-2004 (schka24)
**	    Written.
*/

static DB_STATUS
ppd_malloc(i4 psize, void *pptr, PSS_SESBLK *sess_cb, PSQ_CB *psq_cb)
{

    sess_cb->pss_partdef_stream.ulm_psize = psize;
    if (ulm_palloc(&sess_cb->pss_partdef_stream) != E_DB_OK)
	return (ppd_ulm_errmsg(sess_cb, psq_cb));
    *(void **) pptr = sess_cb->pss_partdef_stream.ulm_pptr;

    return (E_DB_OK);
} /* ppd_malloc */

/*
** Name: ppd_new_break -- Start a new breaks table entry
**
** Description:
**	When a new breaks table entry is started, there's lots of work to
**	do, so it's been broken out into a separate routine.
**
**	We have to make sure that there is space left in the breaks
**	holding array, and the text pointer array.  The operator
**	specified by the grammar actions is stored.  If we're working
**	on the first break entry for this particular clause, we
**	need to see if a partition name was given;  if not, we need
**	to make one.  (The grammar prevents more than one name, so
**	we don't have to check that.)
**
** Inputs:
**	sess_cb		The session's PSS_SESBLK control block
**	psq_cb		The query-parse control block
**	yyvarsp		The parser state structure
**	dim_ptr		Pointer to the dimension we're parsing
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	30-Jan-2004 (schka24)
**	    Written.
*/

static DB_STATUS
ppd_new_break(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp, DB_PART_DIM *dim_ptr)
{

    char generated_name[DB_MAXNAME + 1]; /* Place for partition name */
    DB_PART_BREAKS *breaks_ptr;		/* Ptr to new breaks table entry */
    DB_PART_BREAKS *new_breaks;		/* Relocated/bigger breaks area */
    DB_STATUS status;			/* Called routine status */


    if (yyvarsp->breaks_left == 0)
    {
	/* Oops.  The breaks table area filled up.  Make it bigger. */
	status = ppd_malloc(yyvarsp->breaks_size + BREAKS_MEM_GUESS * sizeof(DB_PART_BREAKS),
			&new_breaks,
			sess_cb, psq_cb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	MEcopy(dim_ptr->part_breaks, yyvarsp->breaks_size, new_breaks);
	yyvarsp->part_breaks_next = (DB_PART_BREAKS *) ((PTR) new_breaks
		+ ((PTR) yyvarsp->part_breaks_next - (PTR) dim_ptr->part_breaks));
	dim_ptr->part_breaks = new_breaks;
	yyvarsp->breaks_left = BREAKS_MEM_GUESS;
	yyvarsp->breaks_size = yyvarsp->breaks_size + BREAKS_MEM_GUESS * sizeof(DB_PART_BREAKS);
    }
    breaks_ptr = yyvarsp->part_breaks_next;
    ++ yyvarsp->part_breaks_next;
    -- yyvarsp->breaks_left;
    ++ dim_ptr->nbreaks;
    breaks_ptr->val_base = NULL;

    /* The next thing to worry about is the text pointer holding area.
    ** We're going to need one pointer per ON column, so we might as
    ** well make sure we have enough now.  If not, just grab another
    ** holding area -- unlike the breaks table, the text pointers don't
    ** have to be contiguous (except those for one break entry).
    */
    if (yyvarsp->textptrs_left < dim_ptr->ncols)
    {
	status = ppd_malloc(TEXTP_MEM_ENTRIES * sizeof(DB_TEXT_STRING *),
			&yyvarsp->part_textptr_next,
			sess_cb, psq_cb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	yyvarsp->textptrs_left = TEXTP_MEM_ENTRIES;
    }
    /* Go ahead and grab all ncols pointer slots now */
    breaks_ptr->val_text = yyvarsp->part_textptr_next;
    yyvarsp->part_textptr_next = yyvarsp->part_textptr_next + dim_ptr->ncols;
    yyvarsp->textptrs_left = yyvarsp->textptrs_left - dim_ptr->ncols;

    /* Copy the operator to the breaks table entry */
    breaks_ptr->oper = yyvarsp->value_oper;

    /* If this is the first break value of the clause, check out the
    ** partition name.  We can assign the partition sequence number here
    ** too;  that way there's no need for an "end of value list" action.
    */
    if (yyvarsp->first_break)
    {
	yyvarsp->first_break = FALSE;	/* Not first any more! */
	if (yyvarsp->names_cur == 0)
	{
	    /* No user name given, generate a name for this partition */
	    STprintf(generated_name,"iipart%d",yyvarsp->name_gen);
	    ++ yyvarsp->name_gen;
	    status = psl_partdef_pname(sess_cb, psq_cb, yyvarsp,
			generated_name, TRUE);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	yyvarsp->names_cur = 0;		/* Reset for next clause */
	/* LIST/RANGE does one partition at a time */
	++ yyvarsp->partseq_first;
	++ yyvarsp->partseq_last;
	++ dim_ptr->nparts;
    }
    breaks_ptr->partseq = yyvarsp->partseq_first;
    return (E_DB_OK);
} /* ppd_new_break */

/*
** Name: ppd_qsfmalloc -- Allocate memory from QSF object
**
** Description:
**	When the partition definition is all done, we want to copy it to
**	the QSF object that holds all the other query stuff.  Copying
**	a partition definition, complete with text, name, etc, is a
**	nontrivial operation, so an RDF utility is called to do it.
**	This routine is handed to RDF for doing the memory allocation.
**
**	RDF allows one control block address for context, so we pass
**	the query-parse control block (PSQ_CB).  From the psq-cb we
**	can figure out the parser session control block, which holds all
**	of the QSF info that we need.
**
** Inputs:
**	psq_cb			Query-parse control block
**	psize			Size of block needed in bytes
**	pptr			An output
**
** Outputs:
**	Returns E_DB_OK or error status
**	pptr			Pointer to allocated block returned here
**
** History:
**	2-Feb-2004 (schka24)
**	    written.
**	26-Apr-2004 (schak24)
**	    Make extern for copy's use.
*/

DB_STATUS
ppd_qsfmalloc(PSQ_CB *psq_cb, i4 psize, void *pptr)
{

    PSS_SESBLK *sess_cb;		/* Parser session control block */

    sess_cb = GET_PSS_SESBLK(psq_cb->psq_sessid);
    return (psf_malloc(sess_cb, &sess_cb->pss_ostream, psize, pptr,
		&psq_cb->psq_error));

} /* ppd_qsfmalloc */

/*
** Name: ppd_ulm_errmsg -- Issue error for ULM memory allocation failure
**
** Description:
**	If ULM chokes, it doesn't say anything to the user, it just
**	returns error status.  In order to get something back to the
**	user we have to arrange for the error status to be translated
**	into the error block in the query CB.  A simple out-of-memory
**	condition turns into a "PSF out of memory" message, anything
**	else turns into a more general message.
**
**	IMPORTANT:  This routine is only to be called when an allocation
**	from the partdef temporary memory stream fails.  If you're
**	allocating something to the "real" query object in QSF,
**	you should be using psf-malloc, which does its own error
**	message stuff.
**
** Inputs:
**	sess_cb		Parser session control block.
**	psq_cb		Query-parse control block
**
** Outputs:
**	Always returns E_DB_ERROR.
**
** History:
**	29-Jan-2004 (schka24)
**	    Written.
*/

static DB_STATUS
ppd_ulm_errmsg(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb)
{
    char qry[PSL_MAX_COMM_STRING + 40];	/* Description of what's going on */
    i4 qry_len;				/* Length of qry */
    i4 toss_err;			/* Not-used error code */


    if (sess_cb->pss_partdef_stream.ulm_error.err_code == E_UL0005_NOMEM)
	(void) psf_error(E_PS0F02_MEMORY_FULL, 0, PSF_CALLERR,
			&toss_err, &psq_cb->psq_error, 0);
    else
    {
	/* Try to give some input as to what's going on */
	psl_command_string(psq_cb->psq_mode, sess_cb->pss_lang, qry, &qry_len);
	STcat(qry,":partition definition");
	/* Use interr so that the message is logged */
	(void) psf_error(E_PS0A0D_ULM_ERROR, 0, PSF_INTERR,
			&toss_err, &psq_cb->psq_error, 2,
			sizeof(sess_cb->pss_partdef_stream.ulm_error.err_code),
			&sess_cb->pss_partdef_stream.ulm_error.err_code,
			STlength(qry), qry);
    }

    return (E_DB_ERROR);
} /* ppd_ulm_errmsg */
