/*
** Copyright (c) 1991, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <bt.h>
#include    <ci.h>
#include    <cv.h>
#include    <qu.h>
#include    <st.h>
#include    <cm.h>
#include    <er.h>
#include    <tm.h>
#include    <clfloat.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <copy.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <sxf.h>
#include    <qeuqcb.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psftrmwh.h>
#include    <psldef.h>
#include    <psyaudit.h>
#include    <uld.h>
#include    <spatial.h>

/*
**  NO_OPTIM=dgi_us5 int_lnx int_rpl i64_aix
*/


/*
** Name: PSLCTBL.C:	this file contains functions used in semantic actions
**			for CREATE (QUEL) and CREATE TABLE (SQL) statement
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_CT<number>[S|Q]_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
**	if the function will be called only by the SQL grammar, <number> must be
**	followed by 'S';
**	
**	if the function will be called only by the QUEL grammar, <number> must
**	be followed by 'Q';
**
** Description:
**	this file contains functions used by both SQL and QUEL grammar in
**	parsing of the CREATE [TABLE] statement
**
**	Routines:
**	    psl_ct1_create_table	- actions for create_table: (simple
**					  CREATE TABLE) and create:
**	    psl_crtas_fixup_columns	- Column fixup after create table as
**					  select, before WITH-options
**	    psl_ct2s_crt_tbl_as_select	- actions for create_table:
**					  (CREATE TABLE ... AS SELECT)
**	    psl_ct3_crwith		- actions following the parsing of
**					  a WITH clause (tbl_with_clause) 
**					  (both QUEL and SQL)
**	    psl_ct9_new_loc_name	- actions for the new_loc_name
**					  production
**	    psl_ct10_crt_tbl_kwd	- actions for the crt_tbl_kwd: and
**					  create: production
**	    psl_ct11_tname_name_name	- actions for the tname:NAME NAME
**					  production
**	    psl_ct12_crname		- actions for the crname production
**	    psl_ct13_newcolname		- actions for the newcolname production
**	    psl_ct14_typedesc		- actions for the typedesc production
**	    psl_ct15_distr_with		- distr-only actions for the NAME=NAME
**					  production (cr_nm_eq_nm) and
**					  NAME=SCONST production (cr_nm_eq_str)
**	    psl_ct16_distr_create	- Semantic actions (distr-only) 
**				  	  create_table: production (SQL) 
**	    psl_ct17_distr_specs	- Semantic actions (distr-only)
**					  specs: production (SQL)
**	    psl_ct18s_type_qual  	- check for duplicate & 
**					  conflicting qualifications
**	    psl_ct19s_constraint 	- store away info for constraints
**	    psl_ct20s_cons_col  	- store away column info for constraints
**	    psl_ct21s_cons_name  	- store constraint name (if specified)
**					  and update the constraint list
**	    psl_ct22s_cons_allowed   	- make sure constraints are allowed 
**					  in this statement/context
**
** History:
**	5-mar-1991 (bryanp)
**	    Created as part of the Btree index compression project.
**	13-nov-91 (andre)
**	    merged 02-jul-91 (andre) change:
**	      Made changes to fix bug 37706:
**		when processing a WITH clause associated with CREATE TABLE
**		statement, we will not report a syntax error upon seeing options
**		which are not specifically recognized by INGRES but which appear
**		to be directed at one of the gateways (the general format of
**		such options is db_id followed by '_' followed by a string where
**		db_id must be 3 chars long with the first char being
**		alphabetical, and the other two being alphanumeric.)
**		According to the OPEN SQL manual such options may be encountered
**		in a WITH-clauses of format
**		    "with_name = with_value | (with_value_list)"
**	    
**	        The changes involved psl_nm_eq_nm() (CREATE TABLE AS SELECT case
**		only), psl_nm_eq_no(), psl_lst_prefix(), psl_ct8_cr_lst_elem().
**	12-dec-91 (stevet)
**	    Delete redundant ADF error message from psl_ct14_typedesc.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	16-jan-1992 (bryanp)
**	    Add support for lightweight session tables.
**	28-feb-1992 (bryanp)
**	    Allow WITH ALLOCATION and WITH EXTEND for ordinary CREATE TABLE.
**      20-may-92 (schang)
**          GW integration.
**	15-jun-92 (barbara)
**	    Sybil merge.  Added distributed-only actions for parsing create
**	    table.  Wrote two new functions: psl_ct15_distr_with() for handling
**	    distributed-only WITH clauses (e.g. node, database, etc.); also
**	    psl_ct16_distr_create() to process the distributed-only parts
**	    of the top level production of CREATE TABLE.
**	31-aug-92 (barbara)
**	    Various bug fixes mainly for distributed code.  Added
**	    psl_ct17_distr_specs().
**	28-aug-92 (rblumer)
**	    call pst_crt_tbl_stmt to create a PST_CREATE_TABLE node for simple 
**          creates (in psl_ct10_crt_tbl_kwd);
**	    added functions psl_ct18s-ct22s for constraint productions;
**	    created psl_command_string to centralize command text for errors.
**      15-oct-92 (rblumer)
**          added code to set new defaultID columns in column descriptor,
**          but only for canonical defaults
**	16-nov-1992 (bryanp)
**	    Fix test for NOT NULL NOT DEFAULT in register table.
**	12-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    Modified interface to psl_ct19s_constraint.
**      20-nov-92 (rblumer)
**          moved code for defaults from psl_ct14_type_desc to new function
**	    psl_col_default; added support for user-specified defaults
**	10-dec-92 (rblumer)
**	    added new WITH UNIQUE_SCOPE clause for CREATE INDEX
**      20-jan-93 (rblumer)    
**          added parameter to psq_tmulti_out since it's prototype has changed
**      10-feb-93 (rblumer)    
**          change psl_ct1_create_table to use a new query mode and
**          reset the QSF root for Star CREATE TABLE.
**	09-mar-93 (walt)
**	    Removed external declaration of MEmove().  It's declared in
**	    me.h and this declaration causes a conflict if it's prototyped
**	    in me.h and not here.
**	25-feb-93 (rblumer)
**	    moved psl_col_default to psldefau.c; added default_text parameter
**	    to psl_ct14_typedesc definition and to psl_col_default calls;
**	    make system_maintained imply not null and with default.
**	    Change psl_ct2s_crt_tbl_as_select to record default ID info
**	    so OPC can use it to set up the attribute descriptors for the table.
**	05-mar-93 (ralph)
**	    CREATE SCHEMA:
**	    allow constraints when CREATE SCHEMA in psl_ct22s_cons_allowed
**	10-mar-93 (barbara)
**	    Delimited id support for Star.  Remove ldb_flags argument
**	    from psl_nm_eq_no, psl_ct6_cr_single_kwd, psl_lst_prefix,
**	    psl_ct15_distr_with, psl_ct16_distr_create.  Also changed
**	    interface to psl_ct9_new_loc_name and psl_ct13_newcolname.
**	06-apr-93 (rblumer)
**	    made psl_nm_eq_nm handle the MODIFY command, too.
**	30-mar-1993 (rmuth)
**	    - Add support for "modify xxx to add_extend", only option allowed
**	      is the "with extend=n". Check no other option passed in.
**	    - Disallow WITH ALLOCATION and WITH EXTEND for ordinary create
**	      table.
**	19-apr-93 (rblumer)
**	    Change default ID for calculated columns to the SQL 'default'
**	    default, which is 'unspecified'.
**	    changed psl_col_default to psl_1col_default (per coding standard).
**	    added trace point for printing out create table DDL tree.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use appropriate case for "tid".
**	    Use case insensitive "ii" when checking for catalog names.
**	    Use pss_cat_owner rather than DB_INGRES_NAME when assigning
**	    owner name for catalogs.
**	1-jun-93 (robf)
**	    Secure 2.0: Add support for security row attributes (label/auditing
**          and adding default values if necessary)
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	02-jul-93 (rblumer)
**	    in psl_ct18s_type_qual,
**	    to make error messages more helpful, added column name to them;
**	    added sess_cb and dbpinfo as parameters in order to get the name.
**	    Also added psl_find_18colname function for returning the name.
**	    in psl_ct12_crname, only change owner name to $ingres for commands
**	    that have a dmu_cb allocated; otherwise will get a segv.
**      12-Jul-1993 (fred)
**          Disallow mucking with extension tables.  Treat the same as
**          system catalogs.  Also, added code to make sure that the
**          type in question is allowed in a database (check the AD_INDB bit).
**	26-july-1993 (rmuth)
**	    Make psl_nm_eq_no() handle the COPY statement.
**	    Include <copy.h> and <qefcopy.h>
**	10-aug-93 (rblumer)
**          in psl_ct1_create_table, return error if no columns were specified;
**          the SQL syntax no longer prevents this, due to addition of
**          ANSI constraints.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	    removed declaration of qsf_call()
**	23-aug-93 (stephenb)
**	    fix calls to psy_secaudit() so that CREATE SYNONYM is audited as
**	    such, and not as create table.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	15-sep-93 (tam)
**	    Misc. patching for star code:
**	    (1) append extra space to local query buffer in 
**		psl_ct6_cr_single_kwd, psl_ct8_cr_lst_elem, and 
**		psl_ct16_distr_create
**	    (2) In psl_ct9_new_loc_name, remove "(" that was added in the 
**		local query buffer, which causes bug b53952.  Instead the "(" 
**		is added to the query buffer when the first column name is 
**		processed in psl_ct13_newcolname
**	    (3) In psl_ct6_cr_single_kwd and psl_lst_prefix, append " with " 
**		to the query buffer for CREATE TABLE AS SELECT (qmode == 
**		PSQ_RETINTO), in addition to CREATE TABLE.
**	16-aug-93 (rblumer)
**	    in psl_ct2s_crt_tbl_as_select,
**	    add code to generate PST_CREATE_INTEGRITY nodes for each NOT NULL 
**	    column, as we want to have a dummy constraint created for each 
**	    NOT NULL column.
**      13-sep-93 (rblumer)
**          Initialize attr_flags_mask to 0 in psl_ct14_typedesc, instead of
**          in psl_1col_default.  Otherwise we can overwrite the
**          known-not-nullable bit.
**	23-sep-93 (rblumer)
**	    in psl_ct12_crname,
**	    changed error message E_PS0BD3 to use qry for the command name,
**	    rather than hard-coding it to CREATE TABLE (could be SYNONYM, etc).
**	24-sep-93 (rblumer)
**	    in psl_command_string, added PSQ_CRESETDBP and PSQ_VAR as
**	    additional cases of CREATE PROCEDURE.
**	22-oct-93 (rblumer)
**	    in psl_18colname, pss_setparmq is now in dbpinfo, not in sess_cb.
**	28-oct-93 (stephenb)
**	    Updated call to psy_secuadit() in psl_ct12_crname() so that
**	    we do not audit creation of duplicate objects.
**	16-nov-93 (robf)
**          Keep row security label visible if requested
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping
**	15-jan-94 (rblumer)
**	    B58783: In psl_ct19s_constraint, fix ERlookup error in error 2715
**	    by passing the address of a i4 variable to psf_error instead
**	    of passing a constant.
**	21-jan-94 (rblumer)
**	    B58442: For CREATE TABLE AS SELECT, disabled generation of dummy
**	    NOT NULL constraints for 6.5; added trace point PS176, which
**	    enables generation if desired. 
**	18-apr-94 (rblumer)
**	    interface of psl_verify_cons() changed to accept PSS_RNGTAB/DMU_CB
**	    pair instead of PTR.
**      25-apr-1995 (rudtr01)
**          Bug #68167.  Buffer the WITH clause properly when query type
**          is PSQ_RETINTO (CREATE TABLE AS SELECT) in Star sessions.
**          See psl_nm_eq_no().
**	01-may-1995 (shero03)
**	    Support RTREE
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to this file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch for c89
**      16-aug-1995 (sarjo01) Bug 70424
**	    psl_ct8_cr_lst_elem(): check nxtchar for start of next element
**	    or "  ", which is what it actually gets set to.
**	18-Jan-1996 (allmi01)
**	    Added NO_OPTIM for dgi_us5 to correct misc. SQL92 (fips) problems 
**	    like segv when createdb iidbdb -u'$ingres' -S is issued during 
**	    ingbuild setup.
**	05-Feb-1996 (allmi01)
**	    Added NO_OPTIM for dgi_us5 (Intel based DG/UX) to correct SQL92
**	    related problems like SIGSEGV when creating iidbdb with SQL92 on.
**	19-mar-1996 (pchang)
**	    Fixed bug 70204.  Incorrect test on the next symbol location for
**	    byte alignment in psl_ct11_tname_name_name() prevented a new symbol
**	    block to be allocated when there are exactly 2 bytes left on the
**	    current symbol table, and subsequent alignment operation on the
**	    next symbol pointer caused it to advance beyond the current symbol
**	    block and thus, corrupting adjacent object when the next symbol was
**	    assigned.  SEGVIO followed when the trashed object was later
**	    referenced.
**	05-Feb-1996 (allmi01)
**	    Added NO_OPTIM for dgi_us5 (Intel based DG/UX) to correct SQL92
**	    related problems like SIGSEGV when creating iidbdb with SQL92 on.
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for WITH PAGE_SIZE= clause.
**	19-mar-1996 (pchang)
**	    Fixed bug 70204.  Incorrect test on the next symbol location for
**	    byte alignment in psl_ct11_tname_name_name() prevented a new symbol
**	    block to be allocated when there are exactly 2 bytes left on the
**	    current symbol table, and subsequent alignment operation on the
**	    next symbol pointer caused it to advance beyond the current symbol
**	    block and thus, corrupting adjacent object when the next symbol was
**	    assigned.  SEGVIO followed when the trashed object was later
**	    referenced.
**      19-apr-96 (stial01)
**          Validate page_size is one of 2k,4k,8k,16k,32k,64k
**	14-may-1996 (shero03)
**	    Add LZW as HI compression.  The default compression is the
**	    string-trim compression
**	23-jul-1996 (ramra01 for bryanp)
**	    Added PSQ_ATBL_ADD_COLUMN/DROP_COLUMN for alter table
**	    add/drop column support.
**	17-sep-96 (nick)
**	    Correct comment and tidy history entries whilst here.
**	10-oct-1996 (canor01)
**	    Use SystemCatPrefix for system prefix.
**	06-nov-1996 (canor01)
**	    Enable cr_nm_eq_no for quel to allow page_size parameter to
**	    CREATE TABLE.
**	07-nov-1996 (canor01)
**	    Move error checking for previous change to cover all exceptions.
**	21-May-1997 (jenjo02)
**	    Table priority project:
**	    Handle PRIORITY = <table_priority> in psl_nm_eq_no().
**	19-jan-1999 (muhpa01)
**	    Removed NO_OPTIM for hp8_us5.
**	26-jan-1999 (toumi01)
**	    Added NO_OPTIM for lnx_us5 to circumvent segv during install
**	    doing "createdb -S iidbdb" with the SQL-92 option.
**      21-mar-1999 (nanpr01)
**          Support raw location.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      30-Oct-2000 (zhahu02)
**         Added code to avoid table corruption with query
**         modify table_name to table_option=3 (bug 103064)
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout(), 
**	    pst_ldbtab_desc().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	01-Mar-2001 (jenjo02)
**	    Remove references to obsolete DB_RAWEXT_NAME.
**	20-may-2001 (somsa01)
**	    Changed 'longnat' to 'i4' from last cross-integration.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      02-jan-2004 (stial01)
**          Changes for new with clause, WITH [NO]BLOBJOURNALING
**          Changes for new ADD_EXTEND with clause, BLOBEXTEND
**          Changes to expand number of WITH CLAUSE options
**      14-jan-2004 (stial01)
**          Cleaned up 02-jan-2004 changes
**	25-Jan-2004 (schka24)
**	    Partition option clause support.
**	    WITH clause productions consolidated, reflect here.
**	    While I'm in here, clean up error handling a little.
**	    Update comments re CREATE SCHEMA mode.
**      13-may-2004 (stial01)
**          Removed support for SET [NO]BLOBJOURNALING ON table.
**	16-Jun-2004 (gupsh01)
**	    Fix alter table alter column, return error if user 
**	    specifies default value other than blank.
**	13-Jul-2004 (gupsh01)
**	    Put back stuff for B1 security.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**      20-oct-2006 (stial01)
**          Disable CLUSTERED index build changes for ingres2006r2
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	23-May-2007 (gupsh01)
**	    Added support for column level UNICODE_FRENCH collation. 
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	28-May-2009 (kschendel) b122118
**	    Minor code/comment cleanup.
**	17-Nov-2009 (kschendel) SIR 122890
**	    resjour values expanded so that we can do suitable things
**	    with CTAS in a non-journaled DB.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      15-apr-2010 (stial01)
**          Fixed length used when copying name
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	13-Oct-2010 (kschendel) SIR 124544
**	    Massive changes all over for centralized WITH-option handling:
**	    move the WITH-option scanners to pslwithopt.c;
**	    replace dmu char array with DMU_CHARACTERISTICS;
**	    teach quel to use pslctbl routines even for RETINTO;
**	    teach gateway register to use pslctbl close-out routines.
**	21-Oct-2010 (kiria01) b124629
**	    Use the macro symbol with ult_check_macro instead of literal.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 psl_ct1_create_table(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_CONS *cons_list);
i4 psl_crtas_fixup_columns(
	PSS_SESBLK *sess_cb,
	PST_QNODE *newcolspec,
	PST_QNODE *query_expr,
	PSQ_CB *psq_cb);
i4 psl_ct2s_crt_tbl_as_select(
	PSS_SESBLK *sess_cb,
	PST_QNODE *newcolspec,
	PST_QNODE *query_expr,
	PSQ_CB *psq_cb,
	PST_J_ID *join_id,
	PSS_Q_XLATE *xlated_qry,
	PST_QNODE *sort_list,
	PST_QNODE *first_n,
	PST_QNODE *offset_n);
i4 psl_ct3_crwith(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_ct9_new_loc_name(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	char *loc_name,
	char *table_name);
i4 psl_ct10_crt_tbl_kwd(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_ct11_tname_name_name(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	char *name1,
	char *name2,
	char *name3,
	char **value);
i4 psl_ct12_crname(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_OBJ_NAME *tbl_spec);
i4 psl_ct13_newcolname(
	PSS_SESBLK *sess_cb,
	char *column_name,
	PSQ_CB *psq_cb,
	PSS_Q_XLATE *xlated_qry,
	PTR *scanbuf_ptr);
i4 psl_ct14_typedesc(
	PSS_SESBLK *sess_cb,
	char *type_name,
	i4 num_len_prec_vals,
	i4 *len_prec,
	i4 null_def,
	PST_QNODE *default_node,
	DB_TEXT_STRING *default_text,
	DB_IISEQUENCE *idseqp,
	PSQ_CB *psq_cb,
	DB_COLL_ID collationID,
	i4 encrypt_spec);
i4 psl_ct15_distr_with(
	PSS_SESBLK *sess_cb,
	char *name,
	char *value,
	PSS_Q_XLATE *xlated_qry,
	bool quoted_val,
	PSQ_CB *psq_cb);
i4 psl_ct16_distr_create(
	PSS_SESBLK *sess_cb,
	PSS_Q_XLATE *xlated_qry,
	bool simple_create,
	PSQ_CB *psq_cb);
i4 psl_ct17_distr_specs(
	PSS_SESBLK *sess_cb,
	PTR scanbuf_ptr,
	PSS_Q_XLATE *xlated_qry,
	DB_ERROR *err_blk);
static void psl_find_18colname(
	i4 qmode,
	PSS_SESBLK *sess_cb,
	PSS_DBPINFO *dbpinfo,
	char **colname);
i4 psl_ct18s_type_qual(
	i4 qmode,
	i4 qual1,
	i4 qual2,
	PSS_SESBLK *sess_cb,
	PSS_DBPINFO *dbpinfo,
	DB_ERROR *err_blk);
i4 psl_ct19s_constraint(
	PSS_SESBLK *sess_cb,
	i4 type,
	PSY_COL *cons_cols,
	PSS_OBJ_NAME *ref_tabname,
	PSY_COL *ref_cols,
	PSS_TREEINFO *check_cond,
	PSS_CONS **consp,
	DB_ERROR *err_blk);
i4 psl_ct20s_cons_col(
	PSS_SESBLK *sess_cb,
	PSY_COL **colp,
	char *newcolname,
	DB_ERROR *err_blk);
i4 psl_ct21s_cons_name(
	char *cons_name,
	PSS_CONS *cons,
	PSS_CONS **cons_listp);
i4 psl_ct22s_cons_allowed(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp);
void psl_command_string(
	i4 qmode,
	PSS_SESBLK *sess_cb,
	char *command,
	i4 *length);
static char *psl_seckey_attr_name(
	PSS_SESBLK *sess_cb);

/*
** Name: psl_ct1_create_table	- Semantic actions for create_table: (SQL) and
**				  create: (QUEL) productions
**
** Description:
**	This routine performs the semantic action for create_table: (SQL) and
**	create: (QUEL) productions.  For SQL, this function is used when parsing
**	a "simple" CREATE TABLE (i.e. not CREATE TABLE AS SELECT)
**
**	create_table:	    crt_tbl_kwd new_loc_name LPAREN specs RPAREN crwith
**	create:		    crestmnt crlocationname LPAREN specs RPAREN crwith
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    PSF request CB
**	    psq_mode	    query mode
**	cons_list	    ptr to a list of constraint info blocks
**
** Outputs:
**	psq_cb		    PSF request CB
**         psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	08-nov-91 (andre)
**	    Largely copied from the SQL grammar
**	01-sep-92 (rblumer)
**	    added cons_list and psq_cb parameters for FIPS constraints.
**      10-feb-93 (rblumer)    
**          use a new query mode and reset the QSF root for Star CREATE TABLE.
**          Star OPF cannot handle the new create_table statement nodes.
**	07-may-93 (andre)
**	    interface of psl_verify_cons() changed to accept psq_cb instead of
**	    psq_mode and &psq_error
**	1-jun-93 (robf)
**	    Add support for default security row attributes.
**      10-aug-93 (rblumer)    
**          return error if no columns were specified; SQL syntax no longer
**          prevents this, due to addition of ANSI constraints.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	18-apr-94 (rblumer)
**	    interface of psl_verify_cons() changed to accept PSS_RNGTAB/DMU_CB
**	    pair instead of PTR.
**	06-mar-96 (nanpr01)
**	    defer the check until compile time of the DDL query to make 
**	    sure that user has specified page_size clause to accommodate
**	    larger tuple size.
**      19-apr-96 (stial01)
**          Validate page_size is one of 2k,4k,8k,16k,32k,64k
**	17-sep-96 (nick)
**	    Alter incorrect comment.
**	16-may-2007 (dougi)
**	    Add support for autostruct with options.
**	04-may-2010 (miket) SIR 122403
**	    Check for anatomically incorrect encryption specifications.
**	20-Jul-2010 (kschendel) SIR 124104
**	    Pass in with-clauses, check for compression, use default.
**	28-Jul-2010 (kschendel) SIR 124104
**	    Also set pst_compress in the create table header so that it
**	    can be passed along to auto-structure.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Replace dmu-char-array with dmu characteristics.
**	9-Nov-2010 (kschendel)
**	    Change psl-verify-cons call again.
*/
DB_STATUS
psl_ct1_create_table(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_CONS	*cons_list)
{
    DB_ERROR		*err_blk = &psq_cb->psq_error;
    i4			err_code;
    i4			colno, num_cols;
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    DMF_ATTR_ENTRY	**attrs;
    bool		logical_key_specified = FALSE;
    bool		encrypted_columns = FALSE;
    PSS_YYVARS		*yyvars;
    PST_STATEMENT	*stmt_list;
    DB_STATUS		status;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    yyvars = sess_cb->pss_yyvars;

    /* return an error if no columns were specified;
    ** SQL syntax no longer prevents this, due to addition of ANSI constraints.
    */
    num_cols = dmu_cb->dmu_attr_array.ptr_in_count;

    if (num_cols == 0)
    {
	(void) psf_error(E_PS04A5_NO_COLUMNS, 0L, PSF_USERERR,
			 &err_code, err_blk, 1,
			 psf_trmwhite(DB_TAB_MAXNAME,
				      dmu_cb->dmu_table_name.db_tab_name),
			 dmu_cb->dmu_table_name.db_tab_name);
	return(E_DB_ERROR);
    }

    /* Make a pass through the attributes checking things like attribute
    ** level flag settings, saving info we will need later in this function.
    */
    for (colno = 0,
	 attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;
	 (colno < num_cols);
	 colno++, attrs++)
    {
	if (!logical_key_specified &&
	    (*attrs)->attr_flags_mask & DMU_F_SYS_MAINTAINED)
	{
	    logical_key_specified = TRUE;
	}
	if ((*attrs)->attr_encflags & ATT_ENCRYPT)
	    encrypted_columns = TRUE;
    }

    if (logical_key_specified)
    {
	BTset(DMU_SYS_MAINTAINED, dmu_cb->dmu_chars.dmu_indicators);
    }

    /* process list of constraints, if any
     */
    if (cons_list != (PSS_CONS *) NULL)
    {
	/* verify that the constraints are correct
	** and convert them into CREATE INTEGRITY statements
	*/
	status = psl_verify_cons(sess_cb, psq_cb,
				 dmu_cb, (PSS_RNGTAB *) NULL,
				 cons_list, &stmt_list);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/* attach stmt_list to end of CREATE_TABLE statement tree
	 */
	status = pst_attach_to_tree(sess_cb, stmt_list, err_blk);
    
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    
    /* check if doing a Distributed CREATE;
    ** we must reset the root of the query object to point to the QEU_CB
    ** structure (like for 6.4 local CREATE TABLEs), instead of to the
    ** PST_CREATE_STATEMENT node (like for 6.5 local CREATE TABLEs).
    **
    ** We also change the query mode to a new Distributed CREATE mode,
    ** so that SCF knows to send this query to QEU like in 6.4 
    ** and not to OPF like 6.5 local CREATE TABLEs.
    */
    if ((psq_cb->psq_mode == PSQ_CREATE)
	&& (sess_cb->pss_distrib & DB_3_DDB_SESS))
    {
	status = psf_mroot(sess_cb, &sess_cb->pss_ostream,
			   sess_cb->pss_object, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	psq_cb->psq_mode = PSQ_DISTCREATE;
    }

    /* sanity check encryption */
    /* encrypted columns but no record-level encryption specified */
    if (encrypted_columns  &&
	! BTtest(PSS_WC_ENCRYPT, dmu_cb->dmu_chars.dmu_indicators))
    {
	(void) psf_error(E_PS0C86_ENCRYPT_NOTABLVL, 0L, PSF_USERERR,
		&err_code, err_blk, 1,
		psf_trmwhite(DB_TAB_MAXNAME,
			dmu_cb->dmu_table_name.db_tab_name),
		dmu_cb->dmu_table_name.db_tab_name);
	return(E_DB_ERROR);
    }
    /* record-level encryption specified but no encrypted columns */
    if (!encrypted_columns &&
	BTtest(PSS_WC_ENCRYPT, dmu_cb->dmu_chars.dmu_indicators))
    {
	(void) psf_error(E_PS0C87_ENCRYPT_NOCOLS, 0L, PSF_USERERR,
		&err_code, err_blk, 1,
		psf_trmwhite(DB_TAB_MAXNAME,
			dmu_cb->dmu_table_name.db_tab_name),
		dmu_cb->dmu_table_name.db_tab_name);
	return(E_DB_ERROR);
    }

#ifdef	xDEBUG
    {
	i4		val1;
	i4		val2;
	QSF_RCB		qsf_rb;
	PST_PROCEDURE    *proc_node;

	/* if PRINT TREE trace point is set, print out DDL query tree
	 */
	if ((psq_cb->psq_mode == PSQ_CREATE)
	    && (ult_check_macro(&sess_cb->pss_trace, PSS_DBPROC_TREE_BY_NEXT_TRACE,
			&val1, &val2)))
	{
	    /* get head of statement tree from QSF (it's root of object);
	    ** object is still exclusively locked from when we created it.
	    */
	    qsf_rb.qsf_type = QSFRB_CB;
	    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
	    qsf_rb.qsf_length = sizeof(qsf_rb);
	    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
	    qsf_rb.qsf_sid = sess_cb->pss_sessid;
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_ostream.psf_mstream, 
				qsf_rb.qsf_obj_id);
	    status = qsf_call(QSO_INFO, &qsf_rb);

	    if (DB_FAILURE_MACRO(status))
	    {
		(VOID) psf_error(E_PS0D19_QSF_INFO, 
				 qsf_rb.qsf_error.err_code,
				 PSF_INTERR, &err_code, err_blk, 0);
		return (status);
	    }
	    proc_node = (PST_PROCEDURE *) qsf_rb.qsf_root;

	    (VOID) pst_dbpdump(proc_node, 0);
	}
    }
#endif /* xDEBUG */

    return(E_DB_OK);
}

/* Name: psl_crtas_fixup_columns - Fix up columns for create-as-select.
**
** Description:
**	After parsing the SELECT query of a create table as select (or
**	declare global temporary table as select), this routine is called
**	to do some fixups on the select result list resdoms.  This is
**	done before the WITH clause is parsed, because the partition=
**	definition in the WITH clause wants to know certain things about
**	the resulting table columns and it would be nice to give it
**	correct information!
**
**	The create table as select (and the declare analog) generally takes
**	the new table's column names from the SELECT result list.
**	If the user specified a column-name list on the create, though,
**	we want to substitute those names into the select list.
**
**	If there is a discrepancy between the column-name-list and the
**	SELECT query, an error will be issued.  If there are any
**	duplicate column names, or "tid" as a column name, it's an
**	error.
**
**	We'll go through and decide on defaultability, and fix zero
**	length columns.  A SELECT query can result in a column like '',
**	which is perfectly legal, except that zero length columns shouldn't
**	be part of tables.  We will search for zero length columns, and make
**	them length one.  (Because union queries are represented with multiple
**	resdoms, one for each select-expr, we have to do some fancy stuff
**	to get it all right for unions.)
**
**	After doing all of these fixups to the resdom list (ie SELECT
**	result list), we'll generate a standard or garden variety
**	DMF_ATTR_ENTRY list for the DMU_CB being built.  Please note
**	that non-printing resdoms don't go on the DMU attribute list,
**	because they don't appear in the result table.  (Non-printing
**	resdoms are generated only to do grouping or sorting, and aren't
**	really part of the output.)  Don't confuse non-printing resdoms
**	and hidden attributes / resdoms!  A hidden attribute is a security
**	audit key, and it IS part of the result table.  It's hidden
**	only in the sense that select * and tm help doesn't show it.
**
**	Attention:  The generated dmu-attr-array cannot be
**	relied on for exact length or nullability information!
**	In particular, when the SELECT query involves outer joins,
**	the parser can get the gross type correct but it can't
**	figure out nullability for sure, because the optimizer
**	OJ analysis can change it!  What we generate here is a
**	good start, and OPF can fix it up quickly enough;  but
**	it's not necessarily the exact end product.
**
** Inputs:
**	sess_cb		Parser session control block
**	newcolspec	PST_QNODE points to column name list
**	query_expr	PST_QNODE points to query-expr ROOT node
**	psq_cb		Query parse control block
**
** Outputs:
**	Returns OK or error status
**
** History:
**	28-Jan-2004 (schka24)
**	    Split off from create-table action so that I can get at valid
**	    column names and lengths for partition definition.
**	12-Feb-2009 (kiria01) b121515
**	    Refer the typeless NULL check to the RESDOM node type and
**	    not the CONST node as only the former will have been fixed up
**	    by pst_union_resolve.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Move most of the post-CTAS resdom processing here, so that
**	    we can generate a DMU_CB attribute list.  Someone has to do
**	    it eventually, and being able to do it here helps out the
**	    WITH-clause parser and everyone downstream.
**	    Fix up loops to handle (ie skip) nonprinting resdoms.
*/

DB_STATUS
psl_crtas_fixup_columns(
	PSS_SESBLK	*sess_cb,
	PST_QNODE	*newcolspec,
	PST_QNODE	*query_expr,
	PSQ_CB		*psq_cb)
{
    bool	unions, rsdm_len_reset = FALSE;
    char	tid[3+1];		/* "tid" or "TID" */
    char	*name;
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    DB_STATUS	status;
    DMF_ATTR_ENTRY *attr;
    DMF_ATTR_ENTRY **ptrs;
    DMT_ATT_ENTRY *attribute;
    DMU_CB	*dmucb;
    i4		attr_count;
    i4		ntype, sstype;
    i4		rsdm_cnt;
    i4		toss_err;
    PSS_RNGTAB	*rngtabp;
    PST_QNODE	*node, *ssnode, *right;
    PSY_ATTMAP	rsdm_map;
    QEU_CB	*qeucb;

    /*
    ** Update RESDOMs in subselect with info from 'newcolspec'.
    ** newcolspec points to list of resdom's, query_expr resdoms are
    ** on the left of query_expr.  A TREE node ends the resdoms.
    */
    if (newcolspec != (PST_QNODE *) NULL)
    {
	node = newcolspec;
	ssnode = query_expr->pst_left;
	for (;;)
	{
	    ntype  = node->pst_sym.pst_type;
	    sstype = ssnode->pst_sym.pst_type;

	    if (ntype == PST_TREE && sstype == PST_TREE)
	    {
		/* equal # of nodes */
		break;
	    }
	    if (sstype == PST_RESDOM
	      && (ssnode->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT) == 0)
	    {
		/* Ignore non-printing resdoms entirely */
		ssnode = ssnode->pst_left;
		continue;
	    }

	    if (ntype != PST_RESDOM || sstype != PST_RESDOM)
	    {   /* not equal # of nodes */
		(VOID) psf_error(2001L, 0L, PSF_USERERR, &toss_err,
		    err_blk, 0);
		return (E_DB_ERROR);
	    }

	    /* copy column name */
	    MEcopy((char *) node->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		sizeof(DB_ATT_NAME),
		(char *) ssnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname);

	    node = node->pst_left;
	    ssnode = ssnode->pst_left;
	}
    }

    /*
    ** fix for bugs 60778, 60975:
    ** if the subselect in CREATE TABLE AS SELECT involves UNION(s) and we 
    ** end up having to reset length of a some RESDOM as a part of fix for bug 
    ** 8931, we need to remember to reset length of corresponding RESDOMs in 
    ** the target lists of remaining subselects.  First we will check whether 
    ** the SELECT involved UNIONs and if so, initialize the map of RESDOMs 
    ** whose lengths have changed
    */
    if ((unions = 
           (query_expr->pst_sym.pst_value.pst_s_root.pst_union.pst_next) != 
		(PST_QNODE *) NULL))
    {
	psy_fill_attmap(rsdm_map.map, (i4) 0);

	rsdm_cnt = 0;
    }

    /* Normalize the `tid' attribute name */
    STcopy(((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"), tid);

    /* Big resdom loop to:
    ** - check that no result-column is named "tid"
    ** - check for duplicate result column names
    ** - fix USER resdoms to ATTNO
    ** - set up target defaultability
    ** - check for LONG TEXT columns
    ** - check for zero-length columns and make them length 1
    */
    for (ssnode = query_expr->pst_left, sess_cb->pss_rsdmno=0;
	 ssnode->pst_sym.pst_type != PST_TREE;
	 ssnode = ssnode->pst_left, sess_cb->pss_rsdmno++
	)
    {
	/*
	** fix for bugs 60778, 60975:
	** if the SELECT involved UNIONS, increment rsdm_cnt so it can be used 
	** as ofset into rsdm_map
	*/
	if (unions)
	    rsdm_cnt++;

	name = ssnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname;

	/* Is the name equal to "tid" */
	if (cui_compare(DB_ATT_MAXNAME, name, STlength(tid), tid) == 0)
	{
	    (VOID) psf_error(2012L, 0L, PSF_USERERR, &toss_err,
		err_blk, 1, psf_trmwhite(DB_ATT_MAXNAME, name), name);
	    return (E_DB_ERROR);
	}

	/* Check for duplicates */
	for (node = ssnode->pst_left;
	     node->pst_sym.pst_type != PST_TREE;
	     node = node->pst_left
	    )
	{
	    if (MEcmp(node->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		name, DB_ATT_MAXNAME) == 0
	       )
	    {
		(VOID) psf_error(2013L, 0L, PSF_USERERR, &toss_err,
		    err_blk, 1, psf_trmwhite(DB_ATT_MAXNAME, name), name);
		return (E_DB_ERROR);
	    }
	}

	/* Convert resdoms with pst_ttargtype PST_USER so that they are
	** PST_ATTNO instead.  (The resdom is going to an attribute in
	** the result relation, not to the user.)
	*/
	if (ssnode->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype == PST_USER)
	{
	    ssnode->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO;
	}

	/* To avoid creation of columns of LONG TEXT type in case of
	** create table as select NULL following check is performed.
	** NOTE that it will have been the RESDOM that had been type
	** adjusted in pst_union_resolve to counter typeless NULL
	** columns and not the CONST data itself.
	*/
	right = ssnode->pst_right;
	if ((right->pst_sym.pst_type == PST_CONST) &&
	    (abs(ssnode->pst_sym.pst_dataval.db_datatype)
						    == DB_LTXT_TYPE))
	{
	    (VOID) psf_error(2016L, 0L, PSF_USERERR,
		&toss_err, err_blk, 0);
	    return (E_DB_ERROR);
	}

	/*
	** Set up defaultability.
	**
	** If the resdom is based directly on a column (i.e. is a PST_VAR),
	** we copy that column's default value;
	** otherwise the resdom must be based on a calculated column or
	** expression, so set up the 'default' SQL default.
	*/
	if (right->pst_sym.pst_type != PST_VAR)
	{
	    DB_DATA_VALUE *dbdata;

	    dbdata = &right->pst_sym.pst_dataval;

	    /* Resdom based on expression.  The SQL default is
	    ** "not specified";  the QUEL default is "not specified" for
	    ** nullable, or ingres default for not nullable.
	    */
	    if (sess_cb->pss_lang == DB_SQL || dbdata->db_datatype < 0)
	    {
		SET_CANON_DEF_ID(ssnode->pst_sym.pst_value.pst_s_rsdm.pst_defid,
			     DB_DEF_NOT_SPECIFIED);
	    }
	    else
	    {
		DMF_ATTR_ENTRY attr;

		/* datatype is non-nullable;
		** set up attribute descriptor and get ingres default
		** id for this tuple
		*/
		MEfill(sizeof(attr), 0, (PTR) &attr);

		attr.attr_type      = dbdata->db_datatype;
		attr.attr_size      = dbdata->db_length;
		attr.attr_precision = dbdata->db_prec;

		status = psl_2col_ingres_default(sess_cb, &attr, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		COPY_DEFAULT_ID(attr.attr_defaultID,
			ssnode->pst_sym.pst_value.pst_s_rsdm.pst_defid);
	    }
	}
	else	/* resdom is based on a column of underlying table */
	{
	    /* find range table entry
	     */
	    status = pst_rgfind(
			sess_cb->pss_lang == DB_SQL
			    ? &sess_cb->pss_auxrng : &sess_cb->pss_usrrange,
			&rngtabp,
			right->pst_sym.pst_value.pst_s_var.pst_vno, err_blk);
	    if (status != E_DB_OK)
	    {
		(VOID) psf_error(E_PS0909_NO_RANGE_ENTRY, 0L, PSF_INTERR,
		    &toss_err, err_blk, 1,
		    psf_trmwhite(DB_ATT_MAXNAME, name), name);
		return (E_DB_ERROR);
	    }

	    /* Look up the attribute
	     */
	    attribute = pst_coldesc(rngtabp,
		right->pst_sym.pst_value.pst_s_var.pst_atname.db_att_name,
		DB_ATT_MAXNAME);

	    /* Check for attribute not found
	     */
	    if (attribute == (DMT_ATT_ENTRY *) NULL)
	    {
		(VOID) psf_error(E_PS090A_NO_ATTR_ENTRY, 0L, PSF_INTERR,
		    &toss_err, err_blk, 1,
		    psf_trmwhite(DB_ATT_MAXNAME, name), name);
		return (E_DB_ERROR);
	    }
	    if (attribute->att_flags & (DMU_F_IDENTITY_ALWAYS |
			DMU_F_IDENTITY_BY_DEFAULT))
	    {
		/* Skip identities - they're not part of CTAS. */
		SET_CANON_DEF_ID(ssnode->pst_sym.pst_value.pst_s_rsdm.
			pst_defid, DB_DEF_NOT_SPECIFIED);
	    }
	    else
	    {
		COPY_DEFAULT_ID(attribute->att_defaultID,
			    ssnode->pst_sym.pst_value.pst_s_rsdm.pst_defid);
	    }

	}  /* end else resdom is based on a column of underlying table */
	/*
	** fix bug 8931: if user tried to create a table with a 0-length
	** TEXT or VARCHAR column, change it to 1.
	*/
	if ((ssnode->pst_sym.pst_dataval.db_datatype == DB_TXT_TYPE ||
	     ssnode->pst_sym.pst_dataval.db_datatype == DB_VCH_TYPE)
	    &&
	    ssnode->pst_sym.pst_dataval.db_length == DB_CNTSIZE)
	{
	    ssnode->pst_sym.pst_dataval.db_length++;

	    /*
	    ** fix for bugs 60778, 60975:
	    ** if the query tree involved UNIONs, set a corresponding bit in
	    ** rsdm_map to remind us to reset length(s) of corresponding RESDOMs
	    ** in subsequent subselect(s)
	    */
	    if (unions)
	    {
		BTset(rsdm_cnt, (char *) rsdm_map.map);
		rsdm_len_reset = TRUE;
	    }
	}

    }  /* end for ssnode->pst_sym.pst_type != PST_TREE */

    /*
    ** fix for bugs 60778, 60975:
    ** if the SUBSELECT involved UNIONs and we had to reset length of RESDOM(s)
    ** in the first subselect, now we will propagate these changes to subsequent
    ** subselect(s)
    */
    if (unions && rsdm_len_reset)
    {
	i4	cur_rsdm, next_rsdm;

	/*
	** for the second and subsequent subselects...
	*/
	for (ssnode = node = 
		 query_expr->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
	     node;
	     ssnode = node = 
		 node->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	{
	    /*
	    ** reset length of RESDOM corresponding to the bits set in rsdm_map
	    */
	    for (cur_rsdm = 0;
		 (next_rsdm = 
		     BTnext(cur_rsdm, (char *) rsdm_map.map, BITS_IN(rsdm_map)))
		  != -1; )
	    {
		for (; 
		     cur_rsdm < next_rsdm; 
		     cur_rsdm++, ssnode = ssnode->pst_left)
		;

		/* 
		** ssnode now points at a RESDOM node whose length must be 
		** bumped
		*/
		ssnode->pst_sym.pst_dataval.db_length++;
	    }
	}
    }

    /* Now we'll get serious about generating that attribute array for
    ** the DMU_CB.  First, count how many we need (ignoring non-printing's),
    ** Then, allocate the entry array, and fill in pointers and entries
    ** from the resdom tree.
    ** (The pointer array was allocated at DB_MAX_COLS when the statement
    ** started, in psl-ct10-crt-tbl-kwd, but the CTAS flavor never adds
    ** array entries, nor does it count anything in the DMU_CB.)
    ** Also note that the DMU attr array, unlike some, counts from zero
    ** and not from 1.
    */
    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;

    attr_count = 0;
    for (ssnode = query_expr->pst_left;
	 ssnode->pst_sym.pst_type != PST_TREE;
	 ssnode = ssnode->pst_left
	)
    {
	if (ssnode->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT)
	    ++ attr_count;
    }
    dmucb->dmu_attr_array.ptr_in_count = attr_count;
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		attr_count * sizeof(DMF_ATTR_ENTRY),
		&attr, err_blk);
    if (status != E_DB_OK)
	return (status);

    /* We actually see the resdom list in reverse, so fill in backwards */
    attr = attr + (attr_count - 1);
    ptrs = (DMF_ATTR_ENTRY **) dmucb->dmu_attr_array.ptr_address;
    ptrs = ptrs + (attr_count - 1);
    for (ssnode = query_expr->pst_left;
	 ssnode->pst_sym.pst_type != PST_TREE;
	 ssnode = ssnode->pst_left
	)
    {
	/* Ignore non-printing's entirely */
	if ((ssnode->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT) == 0)
	    continue;

	*ptrs-- = attr;
	MEcopy(ssnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		sizeof(attr->attr_name.db_att_name),
		attr->attr_name.db_att_name);
	attr->attr_type = ssnode->pst_sym.pst_dataval.db_datatype;
	attr->attr_size = ssnode->pst_sym.pst_dataval.db_length;
	attr->attr_precision = ssnode->pst_sym.pst_dataval.db_prec;
	attr->attr_collID = ssnode->pst_sym.pst_dataval.db_collID;
	attr->attr_flags_mask = 0;
	attr->attr_defaultTuple = NULL;
	attr->attr_seqTuple = NULL;
	attr->attr_encflags = 0;
	attr->attr_encwid = 0;
	attr->attr_geomtype = -1;
	attr->attr_srid = 0;

	/* copy default id from resdom
	 */
	COPY_DEFAULT_ID(ssnode->pst_sym.pst_value.pst_s_rsdm.pst_defid,
			attr->attr_defaultID);

	if (EQUAL_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_NOT_DEFAULT))
	    attr->attr_flags_mask |= DMU_F_NDEFAULT;

	/* Don't set "known not nullable" until OPF is done with things */

	/*
	** Copy over any special flags for the resdom, mainly
	** for hidden or system maintained
	*/
	if (ssnode->pst_sym.pst_value.pst_s_rsdm.pst_dmuflags != 0)
	{
	    /*
	    ** The dmuflags should match exactly with the DMF attribute
	    ** flags required. 
	    ** The DMU_F_LEGAL_ATTR_BITS mask is a hack to remove
	    ** excess bits, really code should be cleaned up to
	    ** ensure only legal bits get set
	    */
	    attr->attr_flags_mask |= 
		    (ssnode->pst_sym.pst_value.pst_s_rsdm.pst_dmuflags &
			    DMU_F_LEGAL_ATTR_BITS);
	}
	if ((attr->attr_size == ADE_LEN_UNKNOWN) ||
	    (attr->attr_size == ADE_LEN_LONG))
	{
	    DB_STATUS	status;
	    i4		bits;
	    DB_DATA_VALUE pdv;
	    DB_DATA_VALUE rdv;

	    status = adi_dtinfo(sess_cb->pss_adfcb, attr->attr_type,
				&bits);
	    if (bits & AD_PERIPHERAL)
	    {
		pdv.db_datatype = attr->attr_type;
		pdv.db_length = 0;
		pdv.db_prec = 0;
		STRUCT_ASSIGN_MACRO(pdv, rdv);

		status = adc_lenchk(sess_cb->pss_adfcb,
				    TRUE, &pdv, &rdv);
		attr->attr_size = rdv.db_length;
		attr->attr_precision = rdv.db_prec;
		attr->attr_flags_mask |= DMU_F_PERIPHERAL;
	    }
	}

	-- attr;
    }

    return (E_DB_OK);
} /* psl_crtas_fixup_columns */

/*
** Name: psl_ct2s_crt_tbl_as_select - Semantic actions for create_table:
**				      (CREATE TABLE ... AS SELECT ...)
**
** Description:
**	This routine performs the semantic action for create_table:
**	(CREATE TABLE ... AS SELECT ...) production
**
**	    crt_tbl_kwd new_loc_name newcolspec AS query_expr crwith
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	    pss_flattening_flags
**			    will be used (in pst_header()) to determine whether
**			    to tell OPF that the query should not be flattened 
**			    (set PST_NO_FLATTEN in tree->pst_mask1) and/or 
**	  		    whether the query contained (subselects involving 
**			    aggregates in their target lists or NOT EXISTS, 
**			    NOT IN, !=ALL) and correlated attribute references 
**			    or <expr involving an attrib> !=ALL (<subsel>) 
**			    (set PST_CORR_AGGR in tree->pst_mask1)
**	newcolspec	    Ptr to list of RESDOM nodes representing the columns
**			    for the table being created
**	query_expr	    Roor of the tree for the query expression.
**	psq_cb		    PSF request CB
**	join_id		    the highest join group id encounetred so far
**	first_n		    number of rows from select to insert into table
*8	offset_n	    offset into select result set to start saving rows
**
** Outputs:
**	psq_cb
**	    psq_error	    will be filled in if an error occurs
**	join_id		    may be updated if some view(s) in the query_expr
**			    involved outer joins
**	xlated_qry	    Star structure to pass to pst_header
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	08-nov-91 (andre)
**	    plagiairized from 6.5 SQL grammar
**	25-aug-92 (barbara)
**	    For Sybil merge, pass in xlated_qry structure to pass to pst_header.
**	08-oct-92 (barbara)
**	    Keep track of number of columns on CREATED TABLE.  Star QEF
**	    requires count.  Pass xlated_qry to pst_header.  For non-distrib,
**	    xlated_qry is null.
**	08-dec-92 (barbara)
**	    address of QEU_DDL_INFO will be stored in QEU_CB.qeu_ddl_info
**	    instead of overloading qeu_qso.
**	25-feb-93 (rblumer)
**	    Record default ID info in the resdom nodes, so OPC can
**	    use it to set up the attribute descriptors for the table.
**	19-apr-93 (rblumer)
**	    Change default ID for calculated columns to the SQL 'default'
**	    default, which is 'unspecified'.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	1-jun-93 (robf)
**	    Removed ORANGE code, reworked row audit auditing/labelling as
**	    seperate items (since row auditing is C2 but row labelling is B1)
**	16-aug-93 (rblumer)
**	    add code to generate PST_CREATE_INTEGRITY nodes for each NOT NULL 
**	    column, as we want to have a dummy constraint created for each 
**	    NOT NULL column.
**    	24-sep-93 (andre)
**	    (fix for bug 54500)
**	    Added qry_mask to the interface
**	08-oct-93 (rblumer/barbara)
**	    Fixed bug 55512 (Star CREATE AS SELECT of non null columns caused
**	    a/v).  Fix is to make Star bypass code that builds constraints.
**	09-nov-93 (barbara)
**	    Fixed bug 56660.  For Star, instead of having QEF populate
**	    iidd_columns directly from the LDB's iicolumns, we must now
**	    populate it with the user-specified column names in order to
**	    preserve case.  Therefore, this function saves away column
**	    names in the QED_DDL_INFO.
**	15-nov-93 (andre)
**	    PSS_SINGLETON_SUBSELECT got moved from pss_stmt_flags to
**	    pss_flattening_flags
**
**	    PSS_SUBSEL_IN_OR_TREE, PSS_ALL_IN_TREE, PSS_MULT_CORR_ATTRS, and
**	    PSS_CORR_AGGR got moved from PSS_YYVARS.qry_mask to 
**	    cb->pss_flattening_flags
**
**	    moved code setting PST_NO_FLATTEN and PST_CORR_AGGR bits to
**	    pst_header()
**
**	    removed qry_mask from the arg list - all bits that were of interest
**	    to us are in sess_cb->pss_flattening_flags
**	21-jan-94 (rblumer)
**	    B58442: Disabled generation of dummy NOT NULL constraints for 6.5;
**	    added trace point PS176, which enables generation if desired.
**	25-jan-94 (rblumer)
**	    B59122: For calculated columns, look at datatype to tell whether to
**	    generate dummy NOT NULL constraint (instead of ALWAYS creating one).
**	18-apr-94 (rblumer)
**	    interface of psl_verify_cons() changed to accept PSS_RNGTAB/DMU_CB
**	    pair instead of PTR.
**	21-apr-94 (robf)
**	    The visibility of _ii_sec_label should be the same
**	    whether its automatically or explicitly created. Check
**	    the visibility flag for the explicitly created case and
**	    set the hidden attribute appropriately.
**	29-apr-94 (robf)
**	    The row audit attribute _ii_sec_tabkey is always invisible, make
**	    this happen consistently. Also in CREATE TABLE AS SELECT the
**	    WITH SECURITY_AUDIT_KEY clause was failing. corrected (removed
**	    extra dereference of the pointer)
**	02-may-94 (andre)
**	    fix for bugs 60778, 60975
**	    	fix for bug 8931 failed to adequately deal with cases when a 
**	    	subselect involves UNION(s).  In cases when it does, lengths
**		of corresponding RESDOMs may be different (i.e. length of RESDOM
**		in the first subselect may be reset to 3 while in subsequent 
**		ones it may remain set to 2).  
**		The problem is fixed by resetting length of RESDOMs in 2-nd and
**		subsequent elements of UNIONs which correspond to RESDOMs in the
**		first subselect whose length were reset to fix bug 8931.
**	 2-may-01 (hayke02)
**	    Modified the logic for the checking of valid key attributes so
**	    that the found boolean is reset for each attribute in the
**	    key = (<columnname>{, <columnname>}) clause. This means we now
**	    get an error of 2024 (FE error E_US07E8) rather than 2179 (FE
**	    error E_US0883) when an attribute, other than the first in the key
**	    list, does not exist. This was found during the testing of bug
**	    104477.
**	30-apr-01 (inkdo01)
**	    Added "first_n" parm to allow "first n" in select.
**	28-Jan-2004 (schka24)
**	    Split off column name fixup.  Attach qeu-cb to the query tree,
**	    so that later phases can find the partition def if any.
**	18-dec-2007 (dougi)
**	    Adjust first "n" support & add offset "n".
**	09-Sep-2008 (jonj)
**	    Correct type of "err_blk" in psf_error().
**	30-april-2009 (dougi) bug 122013
**	    Prevent ctas from copying identity attributes - they don't go
**	    with column.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Delete unused decades-old code to do "with not null" as a
**	    check constraint.  Move a bunch of stuff into crtas-fixup
**	    so that it happens soon enough that we can generate a
**	    dmucb attr array before WITH-options happen.
**	9-Nov-2010 (kschendel)
**	    Change psl-verify-cons call again.
*/
DB_STATUS
psl_ct2s_crt_tbl_as_select(
	PSS_SESBLK	*sess_cb,
	PST_QNODE	*newcolspec,
	PST_QNODE	*query_expr,
	PSQ_CB		*psq_cb,
	PST_J_ID	*join_id,
	PSS_Q_XLATE	*xlated_qry,
	PST_QNODE	*sort_list,
	PST_QNODE	*first_n,
	PST_QNODE	*offset_n)
{
    DB_STATUS	    status;
    PST_QTREE	    *tree;
    i4         qrymod_resp_mask;
    QEU_CB	    *qeu_cb;
    DMU_CB	    *dmu_cb;
    PST_PROCEDURE   *pnode;
    PSS_CONS	    *cons_list = (PSS_CONS *) NULL;
    PST_STATEMENT   *stmt_list = (PST_STATEMENT *) NULL;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /* Neither encryption nor system-maintained columns are possible
    ** with CTAS, only CREATE.  So, we don't need a checking pass like
    ** ordinary create table does.  (exception:  a system maintained security
    ** audit key is possible with CTAS, but that's a special case, handled
    ** in psl-ct3-crwith.)
    */

    if(sess_cb->pss_stmt_flags&PSS_NEED_ROW_SEC_KEY)
    {
	/*
	** Need to add a row security audit key attribute.
	** We already checked for existing or duplicate attribute.
	** psl-ct3-crwith already added the attribute to the dmu-attr-array,
	** and bumped up the attr count and pss_rsdmno,
	** but it also has to become part of the query result.
	*/
	PST_CNST_NODE		cconst;
	PST_RSDM_NODE		tresdom;
	PST_QNODE		*newnode;
	DB_DATA_VALUE		db_data;
	DB_TAB_LOGKEY_INTERNAL  logkey;

	    db_data.db_datatype  = DB_TABKEY_TYPE;
	    db_data.db_prec	     = 0;
	    db_data.db_length    = DB_LEN_TAB_LOGKEY;
	    db_data.db_data	     = (PTR) &logkey;
	    status = adc_getempty(sess_cb->pss_adfcb, &db_data);
	    if(status)
	    	return status;
	    cconst.pst_tparmtype = PST_USER;
	    cconst.pst_parm_no   = 0;
	    cconst.pst_pmspec    = PST_PMNOTUSED;
	    cconst.pst_cqlang    = DB_SQL;
	    cconst.pst_origtxt   = (char *) NULL;
	    status = pst_node(sess_cb, &sess_cb->pss_ostream, (PST_QNODE *) NULL,
	        (PST_QNODE *) NULL, PST_CONST, (char *) &cconst, sizeof(cconst),
	        DB_TABKEY_TYPE, (i2) 0, (i4) DB_LEN_TAB_LOGKEY,
	        (DB_ANYTYPE*)&logkey, &newnode, &psq_cb->psq_error, (i4) 0);
	    if (status)
	        return (status);
	    STmove(psl_seckey_attr_name(sess_cb), 
	    	' ', sizeof(tresdom.pst_rsname),
	        	(char *) tresdom.pst_rsname);
	    tresdom.pst_rsno = tresdom.pst_ntargno = sess_cb->pss_rsdmno;
	    tresdom.pst_rsupdt = FALSE;
	    tresdom.pst_rsflags = PST_RS_PRINT;
	    tresdom.pst_ttargtype=PST_USER;
	    tresdom.pst_dmuflags= (
	    	DMU_F_SYS_MAINTAINED|
	    	DMU_F_SEC_KEY|
	    	DMU_F_WORM|
		DMU_F_HIDDEN|
	    	DMU_F_KNOWN_NOT_NULLABLE);


	    if ((status = pst_node(sess_cb, &sess_cb->pss_ostream,
	        (PST_QNODE *) query_expr->pst_left, newnode, PST_RESDOM,
	        (PTR) &tresdom, sizeof(tresdom), DB_TABKEY_TYPE,
	        (i2) 0, (i4) DB_LEN_TAB_LOGKEY, (DB_ANYTYPE *) NULL,
	        &query_expr->pst_left, &psq_cb->psq_error, (i4) 0)) != E_DB_OK)
	    {
	        return (status);
	    }
    }


    /*
    ** Change object type for the memory stream. For CREATE TABLE stmnt
    ** object type is QSO_QP_OBJ initially, for the subselect version of
    ** the statement we need QSO_TREE_OBJ.
    */
    status = psf_mchtyp(sess_cb, &sess_cb->pss_ostream, QSO_QTREE_OBJ,
	&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    /*
    ** Convert the root node of the subselect into the root node
    ** for the query.
    */
    query_expr->pst_sym.pst_type = PST_ROOT;
    query_expr->pst_sym.pst_value.pst_s_root.pst_rtuser = TRUE;

    /* No result range variable on a CREATE TABLE AS */
    sess_cb->pss_resrng = (PSS_RNGTAB *) NULL;

    status = psy_qrymod(query_expr, sess_cb, psq_cb, join_id,
	&qrymod_resp_mask);
    if (status != E_DB_OK)
	return (status);

    /*
    ** Make and fill in the query tree header
    ** NOTE: if we ever start allowing to create tables inside dbprocs, an
    **       indicator will have to be passed into psl_ct2s_crt_tbl_as_select();
    **	     the indicator will be used in the call to pst_header(), i.e. TRUE
    **	     will be replaced with !indicator
    */
    status = pst_header(sess_cb, psq_cb, PST_UNSPECIFIED,
	sort_list, query_expr, &tree, &pnode, PST_0FULL_HEADER, 
	xlated_qry);
    if (status != E_DB_OK)
    {
	return (status);
    }

    /* set pst_numjoins.  *join_id contains the highest join id */
    tree->pst_numjoins = *join_id;

    /* set pst_offsetn, pst_firstn. */
    tree->pst_offsetn = offset_n;
    tree->pst_firstn = first_n;

    tree->pst_qeucb = qeu_cb;

    /*
    ** For distributed CREATE TABLE, we provide ddl_info for QEF
    ** so it will be able to register the table once the table is
    ** created.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
        QED_DDL_INFO    *ddl_info; 
    	PST_QNODE	*node;
	i4		count; 

	ddl_info = (QED_DDL_INFO *)qeu_cb->qeu_ddl_info;
        ddl_info->qed_d5_qry_info_p = (QED_QUERY_INFO *) NULL;

	/* 
	** Save DDB column names for QEF to insert into iidd_columns.
	** As the list of PST_QNODEs represent columns in the
	** reverse order, count them first and then insert in the
	** list in backwards order.  Decrement count to discount
	** final PST_TREE node.
	*/
    	if (newcolspec != (PST_QNODE *) NULL)
    	{
	    for (node = newcolspec,count=0; node != (PST_QNODE *) NULL;
	     	node = node->pst_left, count++);

	    ddl_info->qed_d3_col_count = --count;

	    for (node = newcolspec; node != (PST_QNODE *) NULL;
	     	node = node->pst_left)
	    {
		/* allocate new column descriptor */
		status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
				sizeof(DD_COLUMN_DESC),
			(PTR *) &ddl_info->qed_d4_ddb_cols_pp[count],
			&psq_cb->psq_error);

		if (status != E_DB_OK)
	    	    return (status);

		STmove((char *) node->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			' ', sizeof(DD_ATT_NAME),
			ddl_info->qed_d4_ddb_cols_pp[count--]->dd_c1_col_name);
	    }
	}
	else
	{
	    ddl_info->qed_d4_ddb_cols_pp = (DD_COLUMN_DESC **)NULL;
	}

        /* give OPF location of TABLE being CREATEd */
        tree->pst_distr->pst_ldb_desc = psq_cb->psq_ldbdesc;

        /* ddl_info will be given to QEF to CREATE LINK */
        tree->pst_distr->pst_ddl_info = ddl_info;
    }

    /* Fix the root in QSF */
    if (pnode != (PST_PROCEDURE *) NULL)
    {
	status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR) pnode,
	    &psq_cb->psq_error);
	if (status != E_DB_OK)
	{
	    return (status);
	}
    }

    /* process list of constraints, if any
    ** Note that the constraints processed here only exist if the
    ** tracepoint to generate NOT NULL check constraints is turned on.
    ** The constraints on cons_list are generated up above.
     */
    if (cons_list != (PSS_CONS *) NULL)
    {
	/* convert constraints into CREATE INTEGRITY statements
	 */
	status = psl_verify_cons(sess_cb, psq_cb,
				 dmu_cb, (PSS_RNGTAB *) NULL,
				 cons_list, &stmt_list);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/* attach stmt_list to end of CREATE_TABLE AS SELECT statement tree
	 */
	status = pst_attach_to_tree(sess_cb, stmt_list, &psq_cb->psq_error);
    
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    
    return(E_DB_OK);
}

/*
** Name: psl_ct3_crwith	- Semantic actions for the tbloptionalwith production
**
** Description:
**	This routine performs the semantic actions for the crwith (QUEL)
**	and tbl_with_clause (SQL) productions.  The WITH clause, if any,
**	has been parsed to completion and post-processed.  This routine
**	does additional create-table-like semantic checking.
**	(We get here whether there was actually a WITH clause or not,
**	so this is a good place to do generic defaulting.)
**
**	It's desirable to put actions here that are a) specific to
**	create-table-like statements, including REGISTER, but not index or
**	modify;  and b) common to CREATE and CTAS.  Both CREATE and
**	CTAS (RETINTO) come here, but they have different close-out
**	semantic routines.
**
**	This semantic action is bypassed during CREATE SCHEMA.
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_lang		    query language
**	    pss_restab		    characteristics of the new table
**	psq_cb			    Query parse control block
**	  .qmode		    query mode
**	    PSQ_CREATE		    processing CREATE TABLE
**	    PSQ_RETINTO		    processing CREATE AS SELECT
**	    PSQ_DGTT		    processing DECLARE GLOBAL TEMPORARY TABLE
**	    PSQ_DGTT_AS_SELECT	    processing DECLARE GLOBAL TEMPORARY TABLE
**				    AS SELECT.
**	    PSQ_MODIFY		    processing MODIFY
**	    PSQ_INDEX		    processing CREATE INDEX
**	    PSQ_CONS		    processing constraint-WITH
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	24-jan-1992 (bryanp)
**	    Added support for WITH NORECOVERY for temporary tables.
**	26-may-92 (andre)
**	    if creating a DGTT, make sure that WITH NORECOVERY was specified.
**	1-jun-93 (robf)
**	    Secure 2.0: Process default security row options.
**	21-apr-94 (robf)
**	    The visibility of _ii_sec_label should be the same
**	    whether its automatically or explicitly created. Check
**	    the visibility flag for the explicitly created case and
**	    set the hidden attribute appropriately.
**	8-jun-94 (robf)
**          Regression, setting WITH SECURITY_AUDIT_KEY=(col) would make
**	    col hidden, which is not correct since its a user attribute
**	    so should continue to be visible.
**	15-jul-94 (robf)
**          Support with NOSECURITY_AUDIT_KEY, allowing for
** 	    case where audit key may be added automatically due to PM
**	    but on a per-table basis we want to not have a row audit key
**	25-Jan-2004 (schka24)
**	    Consolidate all the table-like with clauses.
**	23-Nov-2005 (schka24)
**	    Interpret allocation= per partition for validation.
**	07-apr-2010 (thaju02) Bug 123518
**	    Initialize attr->attr_seqTuple to 0 for system_maintained keys 
**	    table attribute "_ii_sec_tabkey".
**	5-Oct-2010 (kschendel) SIR 124554
**	    Centralize most create-table (including dgtt) and register-as-
**	    import specific checking here.  Grammar has changed to call
**	    this after a with-clause from both sql and quel.
*/
DB_STATUS
psl_ct3_crwith(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    char		*dmu_indicators;
    i4			err_code;
    DB_ERROR		*err_blk;
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    DMU_KEY_ENTRY	**keyptrs;
    DMU_KEY_ENTRY	*keyentry;
    PSS_YYVARS		*yyvars;
    DB_STATUS	        status;
    char		command[PSL_MAX_COMM_STRING];
    i4			qmode;
    i4			length;
    bool		need_audit_key;

    yyvars = sess_cb->pss_yyvars;
    qmode = psq_cb->psq_mode;
    err_blk = &psq_cb->psq_error;

    /* If this is MODIFY, CREATE INDEX, or a constraint WITH, there's
    ** no post-WITH semantics to do here.  These only get here by an
    ** accident of the grammar.  (Indeed, at present, only create-index
    ** and create-table-like things can get here, but be safe and
    ** spit out anything not create-table-like.)
    */
    if (qmode == PSQ_MODIFY || qmode == PSQ_INDEX || qmode == PSQ_CONS
      || qmode == PSQ_COPY || qmode == PSQ_X100_CRINDEX)
	return (E_DB_OK);

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    dmu_indicators = dmu_cb->dmu_chars.dmu_indicators;
    psl_command_string(qmode, sess_cb, command, &length);

    /* REGISTER?  turn on gateway now.  Also, default structure to heap,
    ** and UPDATE to NOUPDATE.
    */
    if (qmode == PSQ_REG_IMPORT)
    {
	BTset(DMU_GATEWAY, dmu_indicators);
	if (! BTtest(DMU_STRUCTURE, dmu_indicators))
	{
	    BTset(DMU_STRUCTURE, dmu_indicators);
	    dmu_cb->dmu_chars.dmu_struct = DB_HEAP_STORE;
	}
	if (! BTtest(DMU_GW_UPDT, dmu_indicators))
	    BTset(DMU_GW_UPDT, dmu_indicators);  /* leaving flag off */
    }

    /* Make sure there is a location */
    if (!dmu_cb->dmu_location.data_in_size)
    {
	/* Default location for 'normal' & 'subselect' versions */
	STmove("$default", ' ', sizeof(DB_LOC_NAME),
	    (char*) dmu_cb->dmu_location.data_address);
	dmu_cb->dmu_location.data_in_size = sizeof(DB_LOC_NAME);
    }

    /*
    ** If user has not specified [NO]JOURNALING clause session default should be
    ** used, unless this is DGTT or REGISTER, in which case journaling
    ** can't be done.
    */
    if (!BTtest(DMU_JOURNALED, dmu_indicators)
	&& (qmode == PSQ_CREATE || qmode == PSQ_RETINTO)
	&& sess_cb->pss_ses_flag & PSS_JOURNALING)
    {
	BTset(DMU_JOURNALED, dmu_indicators);
	if (sess_cb->pss_ses_flag & PSS_JOURNALED_DB)
	    dmu_cb->dmu_chars.dmu_journaled = DMU_JOURNAL_ON;
	else
	    dmu_cb->dmu_chars.dmu_journaled = DMU_JOURNAL_LATER;
    }

    /*
    ** If user has not specified WITH SECURITY_AUDIT=(ROW|NOROW) clause
    ** session  default should be used, unless this is DGTT or distributed,
    ** in which case security row auditing isn't done.
    */
    if ((qmode == PSQ_CREATE || qmode == PSQ_RETINTO)
      && (sess_cb->pss_distrib & DB_3_DDB_SESS) == 0)
    {
	need_audit_key = FALSE;
	if ((!BTtest(PSS_WC_ROW_SEC_AUDIT, dmu_indicators)
		&& (sess_cb->pss_ses_flag & PSS_ROW_SEC_AUDIT) )
	   || ( BTtest(PSS_WC_ROW_SEC_AUDIT, dmu_indicators) &&
		yyvars->secaudit) )
	{
	    need_audit_key=TRUE;
	    /*
	    ** Mark the table has having row security auditing
	    */
	    BTset(DMU_ROW_SEC_AUDIT, dmu_indicators);
	}

	/* The combination of explicit NOSECURITY_AUDIT_KEY and explicit
	** or session security_audit=(row) is invalid, error.
	*/
	if (need_audit_key
	  && BTtest(PSS_WC_ROW_SEC_KEY, dmu_indicators)
	  && yyvars->secaudkey == NULL)
	{
	    /*
	    ** We need a row audit key but the user explictly stated that
	    ** there should be no row audit key, so generate an error
	    */
	    (VOID) psf_error(6662, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);    /* non-zero return means error */
	}

	/*
	** If user has not specified SECURITY_AUDIT_KEY= clause session
	** default should be used.  Note that the session default may be
	** to create an audit key even when row auditing is disabled, to allow
	** for auditing to be enabled later on.
	** First check for explicit NOSECURITY_AUDIT_KEY.
	*/
	if(! (BTtest(PSS_WC_ROW_SEC_KEY, dmu_indicators)
		&& yyvars->secaudkey == NULL)
	  &&
	    (  (!BTtest(PSS_WC_ROW_SEC_KEY, dmu_indicators) &&
		 (sess_cb->pss_ses_flag & PSS_ROW_SEC_KEY)
	       )
	       ||  BTtest(PSS_WC_ROW_SEC_KEY, dmu_indicators)
	       ||  need_audit_key
	   ))
	{
	    i4	colno;
	    i4  len=0;
	    i4	attr_count;
	    DMF_ATTR_ENTRY	**attrs;
	    DMF_ATTR_ENTRY	*cur_attr;
	    i4		err_num;
	    char	*seckeyname;
	    /*
	    ** Search to see if the user already added a column called
	    ** _ii_sec_tabkey, or a requested key column
	    */
	    attrs=(DMF_ATTR_ENTRY**) dmu_cb->dmu_attr_array.ptr_address;
	    attr_count = dmu_cb->dmu_attr_array.ptr_in_count;
	    if (BTtest(PSS_WC_ROW_SEC_KEY, dmu_indicators))
	    {
		seckeyname= yyvars->secaudkey->db_att_name;
		len=sizeof(DB_ATT_NAME);
	    }
	    else
	    {
		seckeyname=psl_seckey_attr_name(sess_cb);
		len=STlength(seckeyname);
	    }
	    for(colno=0;colno<attr_count; colno++)
	    {
		cur_attr=attrs[colno];
		if(MEcmp(seckeyname,
			(PTR)&cur_attr->attr_name,
			len)==0)
		{
			break;
		}
	    }
	    if ((BTtest(PSS_WC_ROW_SEC_KEY, dmu_indicators))  &&
		colno == attr_count)
	    {
		/*
		** Error, invalid column specified
		*/
		(VOID) psf_error(2024L, 0L, PSF_USERERR, &err_code,
		    err_blk, 1,
		    psf_trmwhite(sizeof(DB_ATT_NAME),
			(char *) seckeyname),
		    (char *) seckeyname);
		return (E_DB_ERROR);

	    }
	    if(colno == attr_count)
	    {
		/* We have to add an audit key column */
		/* If CTAS, remember to add the resdom too (later) */
		if(qmode==PSQ_RETINTO)
		    sess_cb->pss_stmt_flags|= PSS_NEED_ROW_SEC_KEY;
		/*
		** Set system_maintained keys table attribute
		*/
		BTset(DMU_SYS_MAINTAINED, dmu_indicators);

		/*
		** The default attribute is a hidden system_maintained
		** table_key with name "_ii_sec_tabkey"
		*/
		colno = attr_count;

		if( ++attr_count > DB_MAX_COLS)
		{

			if (sess_cb->pss_lang == DB_QUEL)
			    err_num = 5107L;
			else
			    err_num = 2011L;

			(VOID) psf_error(err_num, 0L, PSF_USERERR, &err_code, err_blk, 0);
			return (E_DB_ERROR);
		}
		attrs=(DMF_ATTR_ENTRY**) dmu_cb->dmu_attr_array.ptr_address;

		status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMF_ATTR_ENTRY),
			(PTR*) &attrs[colno], err_blk);

		if(status!=E_DB_OK)
			return status;

		++ dmu_cb->dmu_attr_array.ptr_in_count;
		++ sess_cb->pss_rsdmno;
		cur_attr=attrs[colno];
		STmove(psl_seckey_attr_name(sess_cb)
			, ' ',sizeof(DB_ATT_NAME),
			(char*) &(cur_attr->attr_name));
		cur_attr->attr_defaultTuple=0;
		cur_attr->attr_seqTuple=0;
		cur_attr->attr_precision=0;
		cur_attr->attr_collID=0;
		cur_attr->attr_size=DB_LEN_TAB_LOGKEY;
		cur_attr->attr_type=DB_TABKEY_TYPE;
		cur_attr->attr_flags_mask=(
			DMU_F_SYS_MAINTAINED|
			DMU_F_SEC_KEY|
			DMU_F_WORM|
			DMU_F_HIDDEN|
			DMU_F_KNOWN_NOT_NULLABLE);

		status=psl_2col_ingres_default(sess_cb, cur_attr, err_blk);
		if(status!=E_DB_OK)
			return status;
		cur_attr->attr_encflags = 0;
		cur_attr->attr_encwid = 0;
	    }
	    else
	    {
		/*
		** Attribute already defined, so mark as row audit key.
		** Note: this is regular user attribute so do NOT override
		** the default visibility/hidden attribute here.
		*/
		cur_attr->attr_flags_mask|= DMU_F_SEC_KEY;
	    }
	}
    } /* security audit checks */

    if (! BTtest(DMU_DUPLICATES, dmu_indicators))
    {
	/* The DUPLICATES default depends on query and query language.
	** The SQL default is DUPLICATES.
	** The QUEL default is NODUPLICATES, and if the target storage
	** structure is [c]heapsort, do something so that the query plan
	** removes duplicates too.
	** The REGISTER default is duplicates, unless the storage structure
	** is nonheap and unique, in which case it's NODUPLICATES.
	** (I don't know if it really matters, but that's what the
	** original code did.)
	*/
	BTset(DMU_DUPLICATES, dmu_indicators);
	/* Set flag if DUPLICATES, do nothing if NODUPLICATES */
	if (qmode == PSQ_REG_IMPORT)
	{
	    if (! BTtest(DMU_UNIQUE, dmu_indicators)
	      || dmu_cb->dmu_chars.dmu_struct == DB_HEAP_STORE)
		dmu_cb->dmu_chars.dmu_flags |= DMU_FLAG_DUPS;
	}
	else if (sess_cb->pss_lang == DB_SQL)
	    dmu_cb->dmu_chars.dmu_flags |= DMU_FLAG_DUPS;
	else if (dmu_cb->dmu_chars.dmu_struct == DB_HEAP_STORE
	  && sess_cb->pss_restab.pst_heapsort)
	    yyvars->with_dups = PST_NODUPS;
    }

    /* Transform KEY= list into a dmu_key_array.  I suppose we could work
    ** with DMU key entries directly, but this way we know how big the
    ** array needs to be.
    ** If there is no KEY= list, and it's CTAS, and we have a keyed
    ** storage structure or HEAPSORT, use the first column as the key.
    */
    if (yyvars->withkey_count > 0)
    {
	PST_RESKEY *key;

	/* Allocate pointers, entries */
	dmu_cb->dmu_key_array.ptr_in_count = yyvars->withkey_count;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		yyvars->withkey_count * (sizeof(DMU_KEY_ENTRY *) + sizeof(DMU_KEY_ENTRY)),
		&dmu_cb->dmu_key_array.ptr_address, err_blk);
	if (status != E_DB_OK)
	    return (status);
	dmu_cb->dmu_key_array.ptr_size = sizeof(DMU_KEY_ENTRY);
	keyptrs = (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address;
	keyentry = (DMU_KEY_ENTRY *) (keyptrs + yyvars->withkey_count);
	key = yyvars->withkey;
	do
	{
	    MEcopy(key->pst_attname.db_att_name,
		sizeof(keyentry->key_attr_name.db_att_name),
		keyentry->key_attr_name.db_att_name);
	    keyentry->key_order = DMU_ASCENDING;
	    if (key->pst_descending)
		keyentry->key_order = DMU_DESCENDING;
	    *keyptrs++ = keyentry;
	    ++keyentry;
	    key = key->pst_nxtkey;
	} while (key != NULL);
    }
    else if ((qmode == PSQ_RETINTO || qmode == PSQ_DGTT_AS_SELECT)
      && ((dmu_cb->dmu_chars.dmu_struct != DB_HEAP_STORE
		&& dmu_cb->dmu_chars.dmu_struct <= DB_STDING_STORE_MAX)
	  || (dmu_cb->dmu_chars.dmu_struct == DB_HEAP_STORE
		&& sess_cb->pss_restab.pst_heapsort) ) )
    {
	DMF_ATTR_ENTRY *attr;

	attr = ((DMF_ATTR_ENTRY **)(dmu_cb->dmu_attr_array.ptr_address))[0];
	status = psl_check_key(sess_cb, err_blk, attr->attr_type);
	if (status != E_DB_OK)
	{
	    (VOID) psf_error(2179L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 2,
		sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
		psf_trmwhite(sizeof(DB_ATT_NAME), attr->attr_name.db_att_name),
		attr->attr_name);
	    return (E_DB_ERROR);
	}
	dmu_cb->dmu_key_array.ptr_in_count = 1;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		sizeof(DMU_KEY_ENTRY *) + sizeof(DMU_KEY_ENTRY),
		&dmu_cb->dmu_key_array.ptr_address, err_blk);
	if (status != E_DB_OK)
	    return (status);
	dmu_cb->dmu_key_array.ptr_size = sizeof(DMU_KEY_ENTRY);
	keyptrs = (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address;
	keyentry = (DMU_KEY_ENTRY *) (keyptrs + 1);
	MEcopy(attr->attr_name.db_att_name, DB_ATT_MAXNAME,
		keyentry->key_attr_name.db_att_name);
	keyentry->key_order = DMU_ASCENDING;
	*keyptrs = keyentry;
    }

    return(E_DB_OK);

} /* psl_ct3_crwith */

/*
** Name: psl_ct9_new_loc_name - Semantic actions for the new_loc_name production
**
** Description:
**	This routine performs the semantic actions for 	new_loc_name (SQL) or
**	crlocationname (QUEL) productions. These are the old-style productions
**	which accept [locationName:]table_name
**
**	new_loc_name:       [NAME COLON] crname	    (SQL)
**	crlocationname:	    [NAME COLON] crname	    (QUEL)
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	loc_name	    Location name (NULL if none was specified)
**	table_name	    Table name
**
** Outputs:
**	dmu_cb indicators and location updated
**			    specified
**	xlated_qry	    List of packets will contain qry text
**	psq_cb
**	    psq_error	    Filled in if an error occurs
**
** Returns:
**	none
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	12-nov-91 (andre)
**	    renamed to psl_ct9_new_loc_name;
**	    added code to set PSS_WC_LOCATION in with_clauses if
**	    location:tabname was specified;
**	    cleaned up the interface to get rid of unneeded parameters
**	15-jun-92 (barbara)
**	    Added scanbuf_ptr for Star.
**	01-sep-92 (rblumer)
**	    FIPS constraints.
**	    changed to disallow names beginning with a dollar sign-
**	    -added psq_cb parameter and now returns status.
**	12-nov-92 (barbara)
**	    When marking position in scanner buffer for text generation,
**	    take into account whether or not there has been yacc lookahead.
**	30-nov-92 (rblumer)
**	    use new psl_reserved_ident function to check for dollar sign.
**	01-mar-93 (barbara)
**	    Now remove scanbuf_ptr parameter!  Star can no longer yank the
**	    column specs in one piece from the scanner buffer for generation
**	    of SQL for the LDB; instead it must create the list in pieces in
**	    order to *delimit* column names.  Added xlated_qry parameter.
**	30-mar-93 (rblumer)
**	    removed psl_reserved_ident call; it's now done in a few central
**	    places in grammar.  Added more commands to psl_command_string.
**	10-may-93 (barbara)
**	    For Star, set flag to tell psl_ct13_newcolname that column
**	    specification is first of list.  Need this for code generation.
**	10-sep-93 (tam)
**	    For star, remove the "(" that is added to the buffered text 
**	    for the local query.  This "(" is not used in CREATE TABLE AS 
**	    SELECT in star and causes b53952.  The "(" should instead be
**	    added to the text buffer in psl_ct13_newcolname when the first 
**	    column is being parsed.
**	    
*/
DB_STATUS
psl_ct9_new_loc_name(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*loc_name,
	char		*table_name)
{
    QEU_CB              *qeu_cb;
    DMU_CB		*dmu_cb;
    DB_STATUS		status = E_DB_OK;
    i4			err_code;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /* Give DMF location and table name */

    if (loc_name)
    {
	/* Check location name */
	if (cui_chk3_locname(loc_name) != OK)
	{
	    i4 max = DB_LOC_MAXNAME;

	    (VOID) psf_error(2733, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 3, 8, "LOCATION", 0, loc_name,
		sizeof(max), &max);
	    return E_DB_ERROR;
	}

	/*
	** remember that we have seen a location; user will be prevented from
	** specifying location in the WITH clause if the new table name was
	** prefixed with location name
	*/
	BTset(PSS_WC_LOCATION, dmu_cb->dmu_chars.dmu_indicators);

	STmove(loc_name, ' ', sizeof(DB_LOC_NAME),
	    (char *) dmu_cb->dmu_location.data_address);
	dmu_cb->dmu_location.data_in_size = sizeof(DB_LOC_NAME);
    }

    STmove(table_name, ' ', sizeof(DB_TAB_NAME),
	(char *) &dmu_cb->dmu_table_name);

    /* Set up table name for 'subselect' version' */
    STRUCT_ASSIGN_MACRO(dmu_cb->dmu_table_name,
			    sess_cb->pss_restab.pst_resname);

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	sess_cb->pss_distr_sflags |= PSS_1ST_ELEM;
    }
    return(status);
}

/*
** Name: psl_ct10_crt_tbl_kwd - Semantic actions for the crt_tbl_kwd (SQL) and
**				crestmnt: (QUEL) production
**
** Description:
**	This routine handles the semantic actions for parsing the
**	crt_tbl_kwd: (SQL) and crestmnt: (QUEL) production.
**
**	    crt_tbl_kwd:        CREATE TABLE
**	    crestmnt:		CREATE
**
**	We also handle the initial setup for declare global temporary table,
**	and register as import.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    PSF request CB
**	    psq_mode	    One of PSQ_CREATE, PSQ_DGTT, or PSQ_REG_IMPORT.
**			    (create -> retinto and dgtt -> dgtt_as_select
**			    happens later, if appropriate)
**
** Outputs:
**	psq_cb
**	    psq_error	    will be filled in if an error occurs
**	dmu_indicators	    Set to 0 to indicate no with clauses yet.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	11-nov-91 (andre)
**	    plagiarized from the SQL grammar
**	3-mar-1992 (bryanp)
**	    Added temporary argument for DECLARE GLOBAL TEMPORARY TABLE.
**      28-may-1992 (schang)
**          GW merge, modified to suit register table.
**	15-jun-92 (barbara)
**	    Added support for distributed thread for Sybil.  For distributed, 
**	    we access various psq_cb fields: therefore pass in psq_cb and
**	    remove err_blk and qmode parameters which can be accessed
**	    out of psq_cb.  The semantic action for Star is to allocate
**	    and initialize the structure QEF needs to CREATE the local
**	    table and REGISTER it.
**	28-aug-92 (rblumer)
**	    call pst_crt_tbl_stmt to create PST_CREATE_TABLE node for
**	    simple creates;  this node will be passed to OPF just like 
**	    DML queries (instead of going directly to QEF)
**	07-dec-92 (andre)
**	    address of QEU_DDL_INFO will be stored in QEU_CB.qeu_ddl_info
**	    instead of overloading qeu_qso.
**	30-nov-92 (rblumer)
**	    initialize dmu_cb to nulls, as OPF now copies this structure
**	    and will barf if things are not initialized
**	30-mar-93 (barbara)
**	    Allocate an array of DD_COLUMN_DESC pointers. psl_ct13_newcolname
**	    will allocate a DD_COLUMN_DESC as each column is parsed.  The
**	    list will be used by QEF for inserting into iidd_columns.
**	04-aug-93 (andre)
**	    if processing DECLARE GLOBAL TEMPORARY TABLE, set QEU_TBP_TBL bit in
**	    qeu_cb->qeu_flag to let qeu_dbu() know that it is handling creation
**	    of a temp table
**	07-jan-02 (toumi01)
**	    Replace DD_300_MAXCOLUMN with DD_MAXCOLUMN (1024).
**	28-May-2009 (kschendel) b122118
**	    temporary is boolean, pass it as such.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Don't pass temporary.  Let REGISTER do options and keys
**	    the same as everyone else.
*/
DB_STATUS
psl_ct10_crt_tbl_kwd(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    DB_STATUS		    status;
    QEU_CB		    *qeu_cb;
    DMU_CB		    *dmu_cb;
    DB_ERROR		    *err_blk = &psq_cb->psq_error;

    /* Start out with column count of zero */
    sess_cb->pss_rsdmno = 0;

    /* Allocate QEU_CB for CREATE/REGISTER [TABLE] and initialize its header;
    ** 
    ** if a vanilla CREATE (not a temp table or register),
    ** then allocate a create_table query node and use the qeu_cb from there;
    ** otherwise just allocate a qeu_cb.
    ** Eventually, all creates will be handled by query nodes and trees,
    ** but for now we are only doing vanilla creates.
    **
    ** (create gets a query node, but dgtt goes thru the old qeu_dbu path!
    ** I guess "eventually" means "Not in our lifetimes...")
    */
    if (psq_cb->psq_mode == PSQ_CREATE)
	status = pst_crt_tbl_stmt(sess_cb, psq_cb->psq_mode,
				  DMU_CREATE_TABLE, err_blk);
    else
	status = psl_qeucb(sess_cb, DMU_CREATE_TABLE, err_blk);

    if (status != E_DB_OK)
	return (status);

    qeu_cb = (QEU_CB *) sess_cb->pss_object;

    /* Allocate a DMU control block */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_CB),
	(PTR *) &dmu_cb, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    qeu_cb->qeu_d_cb = dmu_cb;

    /* Make sure DMU_CB and embedded DMU_CHARACTERISTICS is clear */
    MEfill(sizeof(DMU_CB), 0, (PTR) dmu_cb);

    /* Fill in the DMU control block header */
    dmu_cb->type = DMU_UTILITY_CB;
    dmu_cb->length = sizeof(DMU_CB);
    dmu_cb->dmu_flags_mask = 0;
    dmu_cb->dmu_db_id = sess_cb->pss_dbid;
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dmu_cb->dmu_owner);
    dmu_cb->dmu_gw_id = DMGW_NONE;

    if (psq_cb->psq_mode == PSQ_DGTT)
    {
	/* remember that we are creating a temp table.
	** As noted above, this includes dgtt-as-select.
	*/
	qeu_cb->qeu_flag |= QEU_TMP_TBL;
	/* Do it in the DMU characteristics too */
	BTset(DMU_TEMP_TABLE, dmu_cb->dmu_chars.dmu_indicators);
    }

    /*
    ** Allocate the attribute entries.  Assume DB_MAX_COLS, although it's
    ** probably fewer.  This is because we don't know how many columns
    ** we have at this point, and it would be a big pain to allocate
    ** less and then run into problems.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	DB_MAX_COLS * sizeof(DMF_ATTR_ENTRY *),
	(PTR *) &dmu_cb->dmu_attr_array.ptr_address, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    dmu_cb->dmu_attr_array.ptr_in_count = 0;    /* Start with 0 attributes*/
    dmu_cb->dmu_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);

    if (psq_cb->psq_mode == PSQ_REG_IMPORT)
    {
	/*
	** Allocate the OLOCATION entries to save the from clause and the path
	** clause for gateway
	*/

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_FROM_PATH_ENTRY),
	    (PTR *) &dmu_cb->dmu_olocation.data_address, err_blk);
	if (status != E_DB_OK)
	    return (status);
        dmu_cb->dmu_olocation.data_address[0] = '\0';
	dmu_cb->dmu_olocation.data_in_size = sizeof(DMU_FROM_PATH_ENTRY);

	/*
	** Initialize the gateway rowcount to be DMGW_NO_ROW_ENTRY (so we can
	** default it to 1000 if none is given).
	*/
	dmu_cb->dmu_gwrowcount = DMGW_NO_ROW_ENTRY;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    DB_MAX_COLS * sizeof(DMU_GWATTR_ENTRY *),
	    (PTR *) &dmu_cb->dmu_gwattr_array.ptr_address, err_blk);
	if (status != E_DB_OK)
	    return (status);

	dmu_cb->dmu_gwattr_array.ptr_in_count = 0;  /* Start with 0 attributes*/
	dmu_cb->dmu_gwattr_array.ptr_size = sizeof(DMU_GWATTR_ENTRY);

    }

    /*
    ** Allocate the location entries.  Assume DM_LOC_MAX, although it's
    ** probably fewer.  This is because we don't know how many locations
    ** we have at this point, and it would be a big pain to allocate
    ** less and then run into problems.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, DM_LOC_MAX * sizeof(DB_LOC_NAME),
	(PTR *) &dmu_cb->dmu_location.data_address, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    dmu_cb->dmu_location.data_in_size = 0;    /* Start with 0 locations */

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	QED_DDL_INFO		*ddl_info;
	DD_2LDB_TAB_INFO	*ldb_tab_info;
	
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QED_DDL_INFO),
			    (PTR *) &ddl_info, err_blk);

	if (status != E_DB_OK)
	    return (status);

	MEcopy((char *)sess_cb->pss_user.db_own_name, sizeof(DD_OWN_NAME),
	       (char *) ddl_info->qed_d2_obj_owner);

	/*
	** Allocate the column descriptor pointer array.  Will be
	** used by QEF when promoting column names of locally created
	** table into Star catalogs.  Allocate DD_MAXCOLUMN pointers.
	*/
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		(DD_MAXCOLUMN+1) * sizeof(DD_COLUMN_DESC *),
		(PTR *) &ddl_info->qed_d4_ddb_cols_pp, err_blk);

	if (status != E_DB_OK)
	    return (status);

	ddl_info->qed_d3_col_count = 0;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QED_QUERY_INFO),
			    (PTR *) &ddl_info->qed_d5_qry_info_p, err_blk);

	if (status != E_DB_OK)
	    return (status);

	ddl_info->qed_d5_qry_info_p->qed_q1_timeout = 0;
	ddl_info->qed_d5_qry_info_p->qed_q2_lang    = DB_SQL;
	ddl_info->qed_d5_qry_info_p->qed_q3_qmode   = DD_2MODE_UPDATE;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QED_QUERY_INFO),
			    (PTR *) &ddl_info->qed_d9_reg_info_p, err_blk);

	if (status != E_DB_OK)
	    return (status);

	ddl_info->qed_d9_reg_info_p->qed_q1_timeout = 0;
	ddl_info->qed_d9_reg_info_p->qed_q2_lang    = DB_SQL;
	ddl_info->qed_d9_reg_info_p->qed_q3_qmode   = DD_2MODE_UPDATE;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_2LDB_TAB_INFO),
			    (PTR *) &ddl_info->qed_d6_tab_info_p, err_blk);

	if (status != E_DB_OK)
	    return (status);

	ldb_tab_info = ddl_info->qed_d6_tab_info_p;
	
	MEfill(sizeof(DD_OWN_NAME), ' ', (PTR) ldb_tab_info->dd_t2_tab_owner);
	ldb_tab_info->dd_t3_tab_type =
				  ddl_info->qed_d8_obj_type = DD_2OBJ_TABLE;
	ldb_tab_info->dd_t6_mapped_b = FALSE;
	ldb_tab_info->dd_t8_cols_pp  = (DD_COLUMN_DESC **) NULL;

	/* allocate space for LDB descriptor */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_1LDB_INFO),
			    (PTR *) &ldb_tab_info->dd_t9_ldb_p, err_blk);

	if (status != E_DB_OK)
	    return (status);

	/*
	** save address of LDB descriptor so it will be filled in when
	** processing WITH clause
	*/

	psq_cb->psq_ldbdesc = &ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc;
    
	/* copy default LDB description so it may be modified */
	STRUCT_ASSIGN_MACRO(*psq_cb->psq_dfltldb, *psq_cb->psq_ldbdesc);

	/* this flag is set to FALSE for user queries */
	psq_cb->psq_ldbdesc->dd_l1_ingres_b = FALSE;

	/* we may not assume that the LDB id is the same as that of CDB */
	psq_cb->psq_ldbdesc->dd_l5_ldb_id = DD_0_IS_UNASSIGNED;

	qeu_cb->qeu_ddl_info = (PTR) ddl_info;
    }

    return(E_DB_OK);
}

/*
** Name: psl_ct11_tname_name_name - Semantic actions for the tname:NAME NAME
**				    production
**
** Description:
**	This routine performs the semantic actions for tname: NAME NAME (two
**	word typename) or NAME NAME NAME (three word typename) production
**
**	tname:	    NAME NAME
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    Ptr to the PSQ_CB
**	name1		    First name in a two/three-name type name
**	name2		    Second name in a two/threee-name type name
**	name3		    Optional third name in a three-name type name
**
** Outputs:
**	value		    Set to point to the concatenated result typename
**	psq_cb->psq_error   Set if an error is detected.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	19-mar-1996 (pchang)
**	    Fixed bug 70204.  Incorrect test on the next symbol location for
**	    byte alignment prevented a new symbol block to be allocated when
**	    there are exactly 2 bytes left on the current symbol table, and
**	    subsequent alignment operation on the next symbol pointer caused
**	    it to advance beyond the current symbol block and thus, corrupting
**	    adjacent object when the next symbol was assigned.  SEGVIO followed
**	    when the trashed object was later referenced.
**	4-dec-98 (inkdo01)
**	    Added three name types (character/binary large object).
*/
DB_STATUS
psl_ct11_tname_name_name(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*name1,
	char		*name2,
	char		*name3,
	char		**value)
{
    DB_STATUS	    status;
    i4		    dv_size;
# ifdef BYTE_ALIGN
    i4		    left;
# endif

    /* concat the two names */
    /* determine the length of the result */

    dv_size = STlength(name1) + STlength(name2) + sizeof(" ");
    if (name3) dv_size += STlength(name3) + sizeof(" ");
    
    /* get room for the new string */

# ifdef BYTE_ALIGN
    left = &sess_cb->pss_symblk->pss_symdata[PSS_SBSIZE] - sess_cb->pss_symnext;
    /* 
    ** If not aligned and it is not possible to align or
    ** it doesn't make sense to align, allocate symbol memory block
    */
    if ((((PSS_SBSIZE - DB_CNTSIZE - left) % DB_ALIGN_SZ) != 0) &&
	(left <= DB_ALIGN_SZ)
       )
    {
	status = psf_symall(sess_cb, psq_cb, PSS_SBSIZE);
	if (DB_FAILURE_MACRO(status))
	{
	    return(E_DB_ERROR);	/* error */
	}
    }

    /* always start with aligned values */
    sess_cb->pss_symnext =
	(u_char *) ME_ALIGN_MACRO(sess_cb->pss_symnext, DB_ALIGN_SZ);
# endif /* BYTE_ALIGN */

    /*
    ** Make sure there's room for piece pointer; add null terminator in case
    ** conv to string
    */

    if ((((char*) sess_cb->pss_symnext) + dv_size + 1) >=
	(char *) &sess_cb->pss_symblk->pss_symdata[PSS_SBSIZE - 1])
    {
	status = psf_symall(sess_cb, psq_cb, PSS_SBSIZE);
	if (DB_FAILURE_MACRO(status))
	{
	    return(E_DB_ERROR);	/* error */
	}
    }

    (*value) = (PTR) sess_cb->pss_symnext;

    sess_cb->pss_symnext = ((u_char *) sess_cb->pss_symnext) + dv_size +  1;
    
    STpolycat(3, name1, " ", name2, (*value));

    if (name3) STpolycat(3, (*value), " ", name3, (*value));

    return (E_DB_OK);
}

/*
** Name: psl_ct12_crname - Semantic actions for the crname production
**
** Description:
**	This routine performs the semantic actions for crname production.
**
**	crname:	    obj_spec	    (SQL)
**	crname:	    NAME	    (QUEL)
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	psq_cb		    Ptr to the PSQ_CB
**	tbl_spec 	    Table name (possibly with owner name)
**
** Outputs:
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**	12-nov-91 (andre)
**	    6.4->6.5 merge
**	22-jan-1992 (bryanp)
**	    Temporary table names are qualified with SESSION., and must be
**	    created using DECLARE GLOBAL TEMPORARY TABLE.
**	27-may-92 (andre)
**	    having parsed the new object name, reset PSS_NEW_OBJ_NAME in
**	    pss_stmt_flags
**      28-may-1992 (schang)
**          GW merge, using psq_cb->psq_mode to tell if this is for Gateway
**	15-jun-92 (barbara)
**	    Added support for distributed thread for Sybil.  For "ii"
**	    tables look up as $ingres only.  Set mask to check for table
**	    existence only.  Both are to economize on queries to CDB.
**	    Don't check for CREATE TABLE permission because Star has no
**	    permissions.  Don't call qeu_audit because Star has no
**	    auditing.
**	25-sep-92 (pholman)
**	    For C2, modified qeu_audit to become a call to the SXF, adding
**	    <sxf.h> and <tm.h> as required.
**	28-sep-92 (rblumer)
**	    moved code generating statement name to psl_command_string
**	16-dec-92 (wolf)
**	    Add last argument, 'lookup_mask', to psl0_orngent call
**	18-nov-92 (barbara)
**	    New interface to psl0_orngent to pass in Star-specific flags
**	    (same now as psl0_rngent).
**	04-dec-1992 (pholman)
**	    C2: instead of directly calling SXF, go through psy_secaudit and
**	    provide owner information.
**	22-apr-92 (barbara)
**	    Do case insenstive comparison for tables prefixed with "ii".
**	30-mar-1993 (rblumer)
**	    move clearing/resetting of PSS_NEW_OBJ_NAME flag into grammar.
**	09-apr-1993 (ralph)
**	    DELIM_IDENT:
**	    Use case insensitive "ii" when checking for catalog names.
**	    Use pss_cat_owner rather than DB_INGRES_NAME when assigning
**	    owner name for catalogs.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	5-jul-93 (robf)
**	    Pass security label to psy_secaudit()
**	07-jul-93 (rblumer)
**	    only change owner name to $ingres for commands that have a dmu_cb
**	    allocated; otherwise will get a segv.
**      12-Jul-1993 (fred)
**          Disallow mucking with extension tables.  Treat the same as
**          system catalogs.
**	16-jul-93 (rblumer)
**	    only allow "ii" names for synonyms if user is $ingres.
**	30-jul-93 (andre)
**	    subtract 1 from the length of constant parameters to error message
**	    E_PS0459 to account for the '\0' byte
**	17-aug-93 (andre)
**	    PSS_OBJ_NAME.pss_qualified has been replaced with 
**	    PSS_OBJSPEC_EXPL_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	    PSS_OBJ_NAME.pss_sess_table has been replaced with
**	    PSS_OBJSPEC_SESS_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	23-aug-93 (stephenb)
**	    fix calls to psy_secaudit() so that CREATE SYNONYM is audited as
**	    such, and not as create table.
**	23-sep-93 (rblumer)
**	    changed error message E_PS0BD3 to use qry for the command name,
**	    rather than hard-coding it to CREATE TABLE (could be SYNONYM, etc).
**	28-oct-93 (stephenb)
**	    Updated if contition tested before calling psy_secaudit() so that
**	    we do not audit failure to create object because of duplicate object
**	    name. Also updated audit message list to include create view which 
**	    is also handled in this routine. Also tidy up audit event types.
**	31-mar-94 (rblumer)
**	    added error check for "ii" table names to disallow mixed-case
**	    catalog names.
**	23-jun-97 (nanpr01)
**	    Fixup the error parameters.
**	31-aug-06 (toumi01)
**	    Allow GTT declaration without "session." and, if we parse it,
**	    allow "shortcut" references to the table.
**	    For table creation treat GTTs and user tables as part of the same
**	    name space if the GTT shortcut is enabled, to avoid confusion and
**	    accidental actions against the wrong table.
**	22-sep-06 (toumi01)
**	    Check for mixed GTT syntax modes (with and without "session."),
**	    and forbid this mixing as too confusing in its side effects.
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl0_rngent for WITH support.
**	30-jul-10 (gupsh01) bug 124011
**	    CVupper may cause stack corruption, as it expects null terminated
**	    string.
*/
DB_STATUS
psl_ct12_crname(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_OBJ_NAME	*tbl_spec)
{
    i4         err_code;
    DB_STATUS	    status, local_status = E_DB_OK;
    PSS_RNGTAB	    *resrange;
    char	    *ch1, *ch2;
    char	    tempstr[DB_TAB_MAXNAME + 1];
    char	    qry[PSL_MAX_COMM_STRING];
    i4	    qry_len;
    i4		    rngvar_info;
    i4		    lookup_mask = 0;
    i4		    tbls_to_lookup = PSS_USRTBL;
    QEU_CB	    *qeu_cb;
    DMU_CB	    *dmu_cb;

    /* get command string in case we find an error 
     */
    psl_command_string(psq_cb->psq_mode, sess_cb, qry, &qry_len);

    /*
    ** If SESSION.t, create a lightweight session table.
    */
    if (tbl_spec->pss_objspec_flags & PSS_OBJSPEC_SESS_SCHEMA)
    {
	/* make sure DECLARE GLOBAL TEMPORARY TABLE was used */
	if (psq_cb->psq_mode != PSQ_DGTT && psq_cb->psq_mode != PSQ_DGTT_AS_SELECT)
	{
	    (VOID) psf_error(E_PS0BD3_ONLY_ON_TEMP, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 2,
			qry_len, qry,
			sizeof("session scope")-1, "session scope");
	    return (E_DB_ERROR);
	}

	/*
	** specifying GTT with "session." so require that all GTT tables
	** be specified in this mode
	*/
	if (sess_cb->pss_ses_flag & PSS_GTT_SYNTAX_SHORTCUT)
	{
	    (VOID) psf_error(2129L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, psf_trmwhite(sizeof(DB_TAB_NAME),
		(char*)tbl_spec->pss_obj_name.db_tab_name),
		tbl_spec->pss_obj_name.db_tab_name);
	    return(E_DB_ERROR);
	}
	if (sess_cb->pss_lang == DB_SQL)
	    sess_cb->pss_ses_flag |= PSS_GTT_SYNTAX_NOSHORTCUT;

	qeu_cb = (QEU_CB *) sess_cb->pss_object;
	dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

	MEcopy((PTR)&sess_cb->pss_sess_owner, sizeof(dmu_cb->dmu_owner),
		(PTR)&dmu_cb->dmu_owner);
    }
    else
    {
	/*
	** If name was qualified, it'd better be the same as the user, unless
	** it was qualified by SESSION, meaning it's a temporary table.
	*/
	if (   tbl_spec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA
	    && MEcmp((PTR) &tbl_spec->pss_owner, (PTR) &sess_cb->pss_user,
	           sizeof(DB_OWN_NAME)))
	{
	    (VOID) psf_error(E_PS0421_CRTOBJ_NOT_OBJ_OWNER, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 2,
		qry_len, qry,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &tbl_spec->pss_owner),
		&tbl_spec->pss_owner);
	    return(E_DB_ERROR);
	}

	/*
	** SIR 116264
	** if DECLARE GLOBAL TEMPORARY TABLE was used and "session." was
	** NOT specified, create the table AND turn on the session flag
	** that allows "session." to be implied for table references
	*/
	if (psq_cb->psq_mode == PSQ_DGTT || psq_cb->psq_mode == PSQ_DGTT_AS_SELECT)
	{
	    if (sess_cb->pss_ses_flag & PSS_GTT_SYNTAX_NOSHORTCUT)
	    {
		(VOID) psf_error(2129L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 1, psf_trmwhite(sizeof(DB_TAB_NAME),
		    (char*)tbl_spec->pss_obj_name.db_tab_name),
		    tbl_spec->pss_obj_name.db_tab_name);
		return(E_DB_ERROR);
	    }
	    if (sess_cb->pss_lang == DB_SQL)
		sess_cb->pss_ses_flag |= PSS_GTT_SYNTAX_SHORTCUT;

	    qeu_cb = (QEU_CB *) sess_cb->pss_object;
	    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

	    MEcopy((PTR)&sess_cb->pss_sess_owner, sizeof(dmu_cb->dmu_owner),
		(PTR)&dmu_cb->dmu_owner);
	}
    }

    if (   ~sess_cb->pss_distrib & DB_3_DDB_SESS
	&& (   psq_cb->psq_mode == PSQ_CREATE
	    || psq_cb->psq_mode == PSQ_VIEW
	    || psq_cb->psq_mode == PSQ_RETINTO
	    || psq_cb->psq_mode == PSQ_REG_IMPORT
	   )
       )
    {
	/*
	** Verify the user has CREATE_TABLE permission, unless they're
	** declaring a temporary table (which doesn't require any privs).
	** There are no permissions in Star.
	*/
	status = psy_ckdbpr(psq_cb, (u_i4) DBPR_TAB_CREATE);
	if (status)
	{
	    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	    {
		i4	msg_id;
		i4	accessmask = SXF_A_FAIL;
		SXF_EVENT	auditevent;
		DB_ERROR	e_error;

		/*
		** We need to distinguish between tables, synonyms and views
		** for auditing.
		*/
		if (psq_cb->psq_mode == PSQ_CSYNONYM) /* create synonym */
		{
		    msg_id = I_SX272A_TBL_SYNONYM_CREATE;
		    accessmask |= SXF_A_CONTROL;
		    auditevent = SXF_E_TABLE;
		}
		else if (psq_cb->psq_mode == PSQ_VIEW) /* create view */
		{
		    msg_id = I_SX2014_VIEW_CREATE;
		    accessmask |= SXF_A_CREATE;
		    auditevent = SXF_E_VIEW;
		}
		else
		{
		    msg_id = I_SX201E_TABLE_CREATE;
		    accessmask |= SXF_A_CREATE;
		    auditevent = SXF_E_TABLE;
		}
		local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)tbl_spec->pss_obj_name.db_tab_name,
			    &sess_cb->pss_user,
			    sizeof(DB_TAB_NAME), auditevent,
			    msg_id, accessmask,
			    &e_error);

		if (DB_FAILURE_MACRO(local_status) && local_status > status)
		    status = local_status;
	    }

	    (VOID) psf_error((psq_cb->psq_mode == PSQ_REG_IMPORT)
		? E_PS110A_TBL_REGFAIL : 6245L,
		0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 0);
	    return(status);
	}
    }

    /*
    ** In non-distributed caee, we really don't care about tables or
    ** synonyms owned by the DBA, we will be checking only for tables or
    ** synonyms owned by the user and by $ingres.
    ** NOTE: we have been discussing preventing users from crearting tables on
    **       behalf of other users - an offshot of that will be that only
    **	     $ingres will be able to create tables starting with II - so once we
    **	     actually do it, we will not have to lookup tables owned by the user
    **	     AND $ingres, but only those owned by the user
    **
    ** In distributed case, to save on extra queries to CDB, distributed checks
    ** only for tables owned by user.
    */
    if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
        tbls_to_lookup |= PSS_INGTBL;
    }
    /*
    ** If the GTT shortcut syntax is in effect "session." is optional and
    ** so we should treat GTTs and regular user tables as if they lived
    ** in the same name space.
    */
    if (sess_cb->pss_ses_flag & PSS_GTT_SYNTAX_SHORTCUT)
    {
        tbls_to_lookup |= PSS_SESTBL;
    }

    ch1 = tbl_spec->pss_orig_obj_name;
    ch2 = ch1 + CMbytecnt(ch1);

    if (!CMcmpnocase(ch1, &SystemCatPrefix[0]) && 
        !CMcmpnocase(ch2, &SystemCatPrefix[1]))
    {
	/* If CREATE SYNONYM, then user must be "$ingres";
	** (this makes synonyms behave similarly to rules, procedures,
	** and events)
	*/
	if (psq_cb->psq_mode == PSQ_CSYNONYM)
	{
	    if (MEcmp((PTR)&sess_cb->pss_user, (PTR)sess_cb->pss_cat_owner,
		      sizeof(DB_OWN_NAME)) != 0)
	    {
		(void) psf_error(E_PS0459_ILLEGAL_II_NAME, 0L, PSF_USERERR,
				 &err_code, &psq_cb->psq_error, 4,
				 sizeof(ERx("CREATE SYNONYM")) - 1, 
				 ERx("CREATE SYNONYM"),
				 sizeof(ERx("synonym")) - 1, ERx("synonym"),
				 STlength(tbl_spec->pss_orig_obj_name),
				 tbl_spec->pss_orig_obj_name,
				 STlength(SystemCatPrefix),SystemCatPrefix);
		return (E_DB_ERROR);
	    }
	}
	else
	{
    	    /* For CREATE TABLE/VIEW,
	    ** can only create tables beginning with 'II' if running with
	    ** system catalog update privilege.
	    */
	    if (~sess_cb->pss_ses_flag & PSS_CATUPD)
	    {
		(VOID) psf_error(5116L, 0L, PSF_USERERR, &err_code,
				 &psq_cb->psq_error, 3, qry_len, qry,
				 STlength(tbl_spec->pss_orig_obj_name),
				 tbl_spec->pss_orig_obj_name,
				 STlength(SystemCatPrefix), SystemCatPrefix);
		return (E_DB_ERROR);
	    }
	}
	
	/* For catalog names, we don't want to allow catalog names that differ
	** only by case.  An easy way to prevent that is to ONLY allow catalog
	** names that are expressible as regular ids, e.g. all upper-case
	** letters in a FIPS database.  Note that if delimited ids are not
	** MIXED case, then they must be translated the same as regular ids,
	** so in that case this extra check is unnecessary.
	** So, if delimited id's are mixed, we convert the string to
	** appropriate regular-id case, and make sure original name is the same
	*/
	if (*(sess_cb->pss_dbxlate) & CUI_ID_DLM_M)
	{
	    MEcopy(tbl_spec->pss_obj_name.db_tab_name,
		   sizeof(tempstr) - 1, tempstr);

	    /* CVupper and CVlower expects null terminated string */
	    tempstr[DB_TAB_MAXNAME] = EOS;

	    if (*(sess_cb->pss_dbxlate) & CUI_ID_REG_U)
		CVupper(tempstr);
	    else
		CVlower(tempstr);

	    if (MEcmp(tbl_spec->pss_obj_name.db_tab_name, 
		      tempstr, sizeof(tempstr) - 1) != 0)
	    {
		(void) psf_error(E_PS045F_WRONG_CASE, 0L, PSF_USERERR,
			     &err_code, &psq_cb->psq_error, 3,
			     qry_len, qry,
			     psf_trmwhite(DB_TAB_MAXNAME,
					  tbl_spec->pss_obj_name.db_tab_name),
			     tbl_spec->pss_obj_name.db_tab_name,
			     STlength(SystemCatPrefix), SystemCatPrefix);
		return (E_DB_ERROR);
	    }
	}	

	/*
	** For distributed thread lookup "ii" tables as $ingres only.
	*/
	if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    tbls_to_lookup = PSS_INGTBL;
	}

	/*
	** change owner name to be $ingres, as this will be a catalog.
	** 
	** We change the owner name here rather than just waiting until DMF
	** changes it because code for ANSI constraints in QEF
	** looks up the schema id based on the owner name.
	** 
	** NOTE: we only do this for CREATE TABLE, CREATE VIEW,
	**     gateway registers, and distributed CREATE TABLE.
	**     We don't do it for CREATE SYNONYM,
	**     as no dmu_cb is allocated for it.  We also don't do it for
	**     temp tables, as their 'owner name' is a session-specific
	**     identifier.
	**     Note that Star REGISTERs don't go through this code.
	*/
	if (   (psq_cb->psq_mode == PSQ_CREATE)
	    || (psq_cb->psq_mode == PSQ_RETINTO)
	    || (psq_cb->psq_mode == PSQ_VIEW)
	    || (psq_cb->psq_mode == PSQ_REG_IMPORT)
	    || (psq_cb->psq_mode == PSQ_DISTCREATE)
	    )
	{
	    if (psq_cb->psq_mode == PSQ_VIEW)
	    {
		QEUQ_CB	        *qeuq_cb;
		PST_CREATE_VIEW *crt_view;
		PST_STATEMENT	*snode;

		snode    = (PST_STATEMENT *) sess_cb->pss_object;
		crt_view = &snode->pst_specific.pst_create_view;
		qeuq_cb  = (QEUQ_CB *) crt_view->pst_qeuqcb;
		dmu_cb   = (DMU_CB *)  qeuq_cb->qeuq_dmf_cb;
	    }
	    else
	    {
		qeu_cb = (QEU_CB *) sess_cb->pss_object;
		dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
	    }
	    
	    MEcopy((PTR)sess_cb->pss_cat_owner, DB_OWN_MAXNAME,
		   (PTR)dmu_cb->dmu_owner.db_own_name);
	}
    }

    /*
    ** For distributed thread set mask for RDF to check only existence of
    ** table.  This saves on queries to the CDB.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	lookup_mask = PST_CHECK_EXIST;

    if (tbl_spec->pss_objspec_flags & PSS_OBJSPEC_SESS_SCHEMA)
    {
	/*
	** Session tables are always qualified by the session name
	*/
	status = psl0_orngent(&sess_cb->pss_auxrng, -1, "",
			    &tbl_spec->pss_owner, &tbl_spec->pss_obj_name,
			    sess_cb, TRUE, &resrange, psq_cb->psq_mode,
			    &psq_cb->psq_error, &rngvar_info, lookup_mask);
	if (status == E_DB_OK && resrange != 0)
	    rngvar_info = PSS_USR_OBJ;
    }
    else
    {
	status = psl0_rngent(&sess_cb->pss_auxrng, -1, "",
			    &tbl_spec->pss_obj_name, sess_cb, TRUE, &resrange,
			    psq_cb->psq_mode, &psq_cb->psq_error,
	    		    tbls_to_lookup, &rngvar_info, lookup_mask, NULL);
    }
    if (DB_FAILURE_MACRO(status))
	return (status);

    if (resrange)
    {
	i4	    err_num = 0L;

	/*
	** Make sure base table isn't a system table
	** This is only important if the name specified by the caller was
	** not resolved to a synonym.
	*/
	if (   (resrange->pss_tabdesc->tbl_status_mask &
	           (DMT_CATALOG | DMT_EXTENDED_CAT)
		|| resrange->pss_tabdesc->tbl_2_status_mask &
		    DMT_TEXTENSION)
	    && ~rngvar_info & PSS_BY_SYNONYM)
	{
	    err_num = (sess_cb->pss_lang == DB_SQL) ? 2009L : 5103L;
	}
	/*
	** or owned by user
	** Note that tables, views, and synonyms are in the same name space,
	** hence if there is already a table/view/synonym by that name
	** owned by the present user, this query definitely must be rejected
	** 
	** In QUEL in case a synonym with the same name is already owned by the
	** current user, we will say so explicitly, since for all other
	** queries, synonyms will be "overlooked", and it is useful to
	** notify the user that the object with duplicate name is, in fact,
	** a synonym.
	*/
	else if (rngvar_info & PSS_USR_OBJ)
	{
	    if (sess_cb->pss_lang == DB_QUEL)
	    {
		err_num = (rngvar_info & PSS_BY_SYNONYM)
					    ? E_PS0453_SYN_ALREADY_EXISTS
					    : 5102L;
	    }
	    else
	    {
		err_num = 2010L;
	    }
	}

	if (err_num)
	{
	    status = E_DB_ERROR;

	    /*
	    ** We do not need to audit if failure is caused by an attempt to
	    ** create an object with a duplicate name
	    */
	    if (~sess_cb->pss_distrib & DB_3_DDB_SESS && 
		err_num != E_PS0453_SYN_ALREADY_EXISTS &&
		err_num != 5102L && err_num != 2010L)
	    {
		if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
		{
		    i4		msg_id;
		    i4		accessmask = SXF_A_FAIL;
		    SXF_EVENT	auditevent;
		    DB_ERROR	e_error;
		    /*
		    ** We need to distinguish between tables, views and synonyms 
		    ** for auditing
		    */
		    if (psq_cb->psq_mode == PSQ_CSYNONYM) /* create synonym */
		    {
			accessmask |= SXF_A_CONTROL;
			msg_id = I_SX272A_TBL_SYNONYM_CREATE;
			auditevent = SXF_E_TABLE;
		    }
		    else if (psq_cb->psq_mode == PSQ_VIEW) /* create view */
		    {
			msg_id = I_SX2014_VIEW_CREATE;
			accessmask |= SXF_A_CREATE;
			auditevent = SXF_E_VIEW;
		    }
		    else
		    {
			accessmask |= SXF_A_CREATE;
			msg_id = I_SX201E_TABLE_CREATE;
			auditevent = SXF_E_TABLE;
		    }
		    local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)tbl_spec->pss_obj_name.db_tab_name,
			    &sess_cb->pss_user,
			    sizeof(DB_TAB_NAME), auditevent,
			    msg_id, accessmask,
			    &e_error);

		    if (DB_FAILURE_MACRO(local_status) && local_status > status)
			status = local_status;
		}
	    }

	    if (sess_cb->pss_lang == DB_SQL)
	    {
  		(VOID) psf_error(err_num, 0L, PSF_USERERR, &err_code, 
		    &psq_cb->psq_error, 1,
		    STlength(tbl_spec->pss_orig_obj_name),
		    tbl_spec->pss_orig_obj_name);
	    }
	    else
	    {
		(VOID) psf_error(err_num, 0L, PSF_USERERR,&err_code, 
		    &psq_cb->psq_error, 2, qry_len, qry,
		    STlength(tbl_spec->pss_orig_obj_name),
		    tbl_spec->pss_orig_obj_name);
	    }

	    return(status);
	}

	if (sess_cb->pss_lang == DB_SQL)
	{
	    /*
	    ** We need to reset pss_maxrng counter here, because
	    ** there is an object (which is not a catalog) or a synonym by this
	    ** name owned by $ingres and looking it up has incremented
	    ** pss_maxrange (pst_sent). This implies that 1st slot (index of 0)
	    ** in the range table in the tree header will be left empty when
	    ** filled in by pst_header and RDF is going to issue a consistency
	    ** check (it compresses tree data before writing it out and it does
	    ** not like the fact that there is an empty slot). So first slot
	    ** used by the lookup is not available unless we reset the counter;
	    ** the actual tbl description is pointed to by pss_rsrng (because of
	    ** the query mode).
	    */

	    /* same value as used in pst_clrrng */
	    sess_cb->pss_auxrng.pss_maxrng = -1;
	}
    }

    return (E_DB_OK);
}

/*
** Name: psl_ct13_newcolname - Semantic actions for the newcolname production
**
** Description:
**	This routine performs the semantic actions for newcolname production.
**
**	newcolname:	NAME
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	column_name	    column name
**	psq_cb
**	    psq_mode	    mode of the query
**	xlated_qry	    For query text buffering 
**
** Outputs:
**	psq_cb
**	    err_blk	    filled in if an error was encountered
**	xlated_qry	    List of packets pointed to will contain qry text
**	scanbuf_ptr	    will point to column's type description
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	5-mar-1991 (bryanp)
**	    Created.
**      28-may-1992 (schang)
**          GW merge.
**	01-mar-93 (barbara)
**	    Star delimited id support.  Added xlated_qry and scanbuf_ptr
**	    parameters.  Save column name for entry into iidd_columns (same
**	    as register statement)
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	10-may-93 (barbara)
**	    When setting scanbuf_ptr, use a flag instead of comparing
**	    prv_tok to column_name.  The old method doesn't work if we
**	    have performed case translation on a name.
**	10-sep-93 (tam)
**	    If first column in star, add "(" to query buffer.
**	09-nov-93 (barbara)
**	    Check PSS_DELIM_COLNAME flag in order to decide whether or
**	    not to delimit column name.
*/
DB_STATUS
psl_ct13_newcolname(
	PSS_SESBLK	*sess_cb,
	char		*column_name,
	PSQ_CB          *psq_cb,
	PSS_Q_XLATE	*xlated_qry,
	PTR		*scanbuf_ptr)
{
    DB_STATUS		status = E_DB_OK;
    i4		err_code;
    i4			colno;
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    DMF_ATTR_ENTRY	**attrs;
    i4			i;
    DB_ERROR		*err_blk = &psq_cb->psq_error;
    DMU_GWATTR_ENTRY	**gwattrs;
    u_char		unorm_col[DB_ATT_MAXNAME *2 +2];
    u_i4		unorm_len = DB_ATT_MAXNAME *2 +2;

    colno = sess_cb->pss_rsdmno;
    /* Count columns, error if too many */
    if (++sess_cb->pss_rsdmno > DB_MAX_COLS)
    {
	i4	    err_num;

	if (sess_cb->pss_lang == DB_QUEL)
	    err_num = 5107L;
	else if (psq_cb->psq_mode == PSQ_REG_IMPORT)
	    err_num = E_PS1107_TBL_MAXCOL;
	else
	    err_num = 2011L;

	(VOID) psf_error(err_num, 0L, PSF_USERERR, &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /* can't create an attribute named "tid" */
    if (STcompare(column_name,
		  ((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"))
	== 0)
    {
	if (sess_cb->pss_lang == DB_QUEL)
	{
	    (VOID) psf_error(5104L, 0L, PSF_USERERR, &err_code,
		err_blk, 2,
		sizeof(dmu_cb->dmu_table_name),
		(char *) &dmu_cb->dmu_table_name,
		STlength(column_name), column_name);
	}
	else
	{
	    (VOID) psf_error((psq_cb->psq_mode == PSQ_REG_IMPORT)
		? E_PS1108_TBL_COLINVAL : 2012L,
		0L, PSF_USERERR, &err_code, err_blk, 1,
		STlength(column_name), column_name);
	}
	return (E_DB_ERROR);
    }

    /* Fill in column name */
    attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMF_ATTR_ENTRY),
	(PTR *) &attrs[colno], err_blk);
    if (status != E_DB_OK)
	return (status);

    STmove(column_name, ' ', sizeof(DB_ATT_NAME),
	(char *) &(attrs[colno])->attr_name);

    /* schang : GW specific */
    if (psq_cb->psq_mode == PSQ_REG_IMPORT)
    {
	gwattrs = (DMU_GWATTR_ENTRY **) dmu_cb->dmu_gwattr_array.ptr_address;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_GWATTR_ENTRY),
	    (PTR *) &gwattrs[colno], err_blk);
	if (status != E_DB_OK)
	    return (status);
    }

    /* check for duplicate column name */
    for (i = 0; i < colno; i++)
    {
	if (!MEcmp((PTR) &(attrs[colno])->attr_name,
		(PTR) &(attrs[i])->attr_name, 
		sizeof((attrs[colno])->attr_name)))
	{
	    if (sess_cb->pss_lang == DB_QUEL)
	    {
		(VOID) psf_error(5105L, 0L, PSF_USERERR, &err_code,
			    err_blk, 2, 
			    sizeof(dmu_cb->dmu_table_name),
			    (char*) &dmu_cb->dmu_table_name,
			    sizeof((attrs[colno])->attr_name),
			    (char*) &(attrs[colno])->attr_name);
	    }
	    else
	    {
		(VOID) psf_error((psq_cb->psq_mode == PSQ_REG_IMPORT)
		    ? E_PS1109_TBL_COLDUP : 2013L,
		    0L, PSF_USERERR, &err_code, err_blk, 1, 
		    psf_trmwhite(sizeof((attrs[colno])->attr_name),
			(char*) &(attrs[colno])->attr_name),
		    (char*) &(attrs[colno])->attr_name);
	    }
	    return (E_DB_ERROR);
	    
	}
    }

    /* Count length of attribute array */
    dmu_cb->dmu_attr_array.ptr_in_count++;

   /* schang : GW specific */
    if (psq_cb->psq_mode == PSQ_REG_IMPORT)
    {
	/* Also, since each attribute has a corresponding gwattr entry... */
	dmu_cb->dmu_gwattr_array.ptr_in_count++;
    }

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	QED_DDL_INFO	*ddl_info;
	i4		*count;

	/*
	** If first column, add "(" in front of buffered text
	*/
    	if (sess_cb->pss_distr_sflags & PSS_1ST_ELEM)
    	{
	    status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
		       &xlated_qry->pss_q_list, (i4) -1,
		       (char *) NULL, (char *) NULL, "(", err_blk);
	    if (DB_FAILURE_MACRO(status))
	        return(status);
    	}
    	/*
    	** Delimit column names on CREATE stataements issued to the LDB
	** if user delimits.
    	*/
	if (sess_cb->pss_distr_sflags & PSS_DELIM_COLNAME)
	{
	    status = cui_idunorm((u_char *)column_name, (u_i4 *)0, unorm_col,
				&unorm_len, CUI_ID_DLM, err_blk);

	    if (DB_FAILURE_MACRO(status))
	        return(status);

	    status = psq_x_add(sess_cb, (char *)unorm_col, &sess_cb->pss_ostream,
			   xlated_qry->pss_buf_size,
			   &xlated_qry->pss_q_list, (i4) unorm_len,
			   " ", " ", (char *) NULL, err_blk);
	}
	else
	{
	    status = psq_x_add(sess_cb, column_name, &sess_cb->pss_ostream,
			    xlated_qry->pss_buf_size,			
			    &xlated_qry->pss_q_list, STlength(column_name),
			    " ", " ", (char *) NULL, err_blk);
	}

        if (DB_FAILURE_MACRO(status))				
	    return(status);
	
	/*
	** Save column name.  When QEF promotes column names from
	** iicolumns to iidd_columns, we will substitute the
	** distributed name.
	*/
	ddl_info =
	    (QED_DDL_INFO *) ((QEU_CB *)sess_cb->pss_object)->qeu_ddl_info;
	count = &ddl_info->qed_d3_col_count;

	/* allocate new column descriptor */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_COLUMN_DESC),
		(PTR *) &ddl_info->qed_d4_ddb_cols_pp[++*count],
		&psq_cb->psq_error);

	if (status != E_DB_OK)
	    return (status);

	STmove(column_name, ' ', sizeof(DD_ATT_NAME),
		ddl_info->qed_d4_ddb_cols_pp[*count]->dd_c1_col_name);

	/*
	** Save pointer to column type specification to be yanked into
	** buffer for query to LDB.  Assumption: word after column name
	** may not have already been scanned (depends if this is first
	** column in list).  psl_ct9_new_loc_name has set flag; grammar
	** unsets flag when it reads comma in list.
	*/

	if (sess_cb->pss_distr_sflags & PSS_1ST_ELEM)
	{
	    *scanbuf_ptr = (PTR) sess_cb->pss_prvtok;
	    sess_cb->pss_distr_sflags &= ~PSS_1ST_ELEM;
	}
	else
	{
	    *scanbuf_ptr = (PTR) sess_cb->pss_nxtchar;
	}

    }
    return (E_DB_OK);
}

/*
** Name:    psl_ct14_typedesc - semantic action for typedesc production (both
**				SQL and QUEL)
**
** Description:
**
**	Perform the semantic action associated with typedesc production.
**	Primarily, this involves encoding column specification for use of DMF.
**
**	typedesc:	tname null_default
**		|	NAME LPAREN intconst_e RPAREN null_default
**		|	NAME LPAREN intconst_e COMMA intconst_e RPAREN
**			null_default					(SQL)
**
**	typedesc:	tname null_default
**		|	NAME LPAREN intconst RPAREN null_default
**		|	NAME LPAREN intconst COMMA intconst RPAREN null_default
**									(QUEL)
**
** Input parameters:
**	sess_cb		    PSF session CB
**	type_name	    attribute type as specified by the user
**	num_len_prec_vals   indicator of whether length and/or precision have
**			    been specified
**	    0		    neither length nor precision have been specified
**	    1		    length but no precision has been specified
**	    2		    both length and precision have been specified
**	len_prec	    array containing length and precision, if they were
**			    specified (num_len_prec_vals contains the size of
**			    this array)
**	null_def	    indicator of whether the attribute has a default, is
**			    nullable and/or system_maintained
**	    PSS_TYPE_NULL	    nullable
**	    PSS_TYPE_NDEFAULT	    not defaultable
**	    PSS_TYPE_SYS_MAINTAINED system maintained (SQL only)
**                    plus the indicators below (for SQL92 functionality)
**	    PSS_TYPE_DEFAULT	    any default--INGRES or user
**	    PSS_TYPE_USER_DEFAULT   user-defined default
**	    PSS_TYPE_NOT_NULL	    non-nullable
**	    PSS_TYPE_GENALWAYS	    ALWAYS identity
**	    PSS_TYPE_GENDEFAULT	    BY DEFAULT identity
**	psq_cb		    PSF request CB
**	collID		    integer collation ID (for string columns)
**
** Output
**	psq_cb
**	    psq_error	    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	none
**
** History:
**
**	04-nov-91 (andre)
**	    plagiarized from typedesc: production
**	12-dec-91 (stevet)
**	    Remove the redundant ADF error message.
**      28-may-1992 (schang)
**          GW merge.
**      15-oct-92 (rblumer)
**          added code to set new defaultID columns in column descriptor,
**          but only for canonical defaults
**      20-nov-92 (rblumer)
**	    added default_tree parameter for user-defined defaults;
**          moved code for defaults from to new function psl_col_default.
**	25-feb-93 (rblumer)
**	    moved psl_col_default to psldefau.c; added default_text parameter
**	    to psl_ct14_typedesc definition and to psl_col_default calls;
**	    make system_maintained imply not null and with default.
**	20-apr-93 (rblumer)
**	    changed psl_col_default to psl_1col_default (per coding standard).
**      12-Jul-1993 (fred)
**          Added code to make sure that the type in question is
**          allowed in a database (check the AD_INDB bit).
**      13-sep-93 (rblumer)
**          Initialize attr_flags_mask to 0 here, not in psl_1col_default.
**          Otherwise we can overwrite the known-not-nullable bit.
**	17-dec-96 (inkdo01)
**	    Added consistency check for "alter table add column" column
**	    attributes (used to be in grammar - but didn't cover all bases).
**	    Also tightened it up while I was there - DEFAULT is also not 
**	    allowed for added columns.
**	6-aug-99 (inkdo01)
**	    Added support for "add column ... not null with default". Note, no
**	    explicit default values are allowed, and not null & with default must be 
**	    used together.
**	21-jan-00 (inkdo01)
**	    Alteration to "add column ... not null with default" validation code
**	    to permit all data types.
**	26-jan-00 (inkdo01)
**	    Grrrr! Minor change to above to require BOTH not null AND with default.
**	18-apr-01 (inkdo01)
**	    Check for nchar support before allowing nchar types.
**	20-apr-2004 (gupsh01)
**	    Add support for alter table alter columns.
**	18-jun-2004 (gupsh01)
**	    Fixed error messages reported for alter table alter column.
**	11-nov-2004 (gupsh01)
**	    Fixed the error messge for not null and defaults combination for
**	    alter table alter column.
**	13-dec-04 (inkdo01)
**	    Added support for collationID in iiattribute descriptor.
**	13-Sep-2005 (thaju02)
**	    B115209: disallow "create table...not null with default null".
**	    Also extend "add column ... not null with default" validation 
**	    code to alter table alter column.
**	08-Nov-2005 (thaju02) (B115762)
**	    In extending "add column...not null with default" validation
**	    code to alter table alter column, only report if we are 
**	    attempting to alter the column's nullability/default value. 
**      10-Jul-2006 (kibro01) (B116347)
**          Split checks for add column and alter column to remove
**          error (E_US0F11) when trying to add a column with not null
**          and default.
**	18-Sep-2006 (gupsh01)
**	    Added handing of E_AD5065_DATE_ALIAS_NOTSET error. 
**	9-Oct-2005 (kibro01) Bug 116822
**          Moved above the alter-column checks which try to copy the
**          default value, so we check if it's an invalid one first.
**	14-Feb-2007 (kibro01) b117682
**	    Check if there was a default value before - if not we can't
**	    change to a user-defined default now on an alter column.
**	23-May-2007 (gupsh01)
**	    Added support for column level UNICODE_FRENCH collation. 
**	    This is applicable for unicode types but can also be used for 
**	    char/varchar columns as well for UTF8 character set.
**	25-Jul-2007 (kibro01) b118799
**	    Allow change of column from "not null" to "with null".
**	23-Oct-2007 (kibro01) b118918
**	    Reenable some changes which were possible before and are valid.
**	    These will now be caught within dm2u_atable because they depend
**	    on the relversion being 0 to be valid.
**      13-Nov-2007 (gupsh01) b119464 
**	    Re-enable column level collation validation. unicode collaion
**	    should only be valid for unicode types or for UTF8 character
**	    set only.
**	19-Dec-2007 (kibro01) b119624
**	    Check to see if column is known in alter table alter column.
**      27-May-2008 (coomi01) Bug 120413
**          Flag qeu to call createDefaultTuples if new default detected.
**	6-june-2008 (dougi)
**	    Add identity column support.
**	06-Oct-2006 (gupsh01)
**	    Return error to the user if the length specified for the column
**	    datatype is outside of the valid range for column datatype.
**	10-Oct-2006 (gupsh01)
**	    Fix the above change and error E_US082C only if there 
**	    was an actual length value specified for column datatype.
**	31-oct-2008 (dougi)
**	    Forgot to set "system generated" flag for identity sequences.
**	21-May-2009 (kiria01) b122051
**	    Tidy collID handling. Move collation check code out to use the
**	    psl_collation_check()
**      02-Nov-2009 (smeke01) b122751
**          Flag qeu to call createDefaultTuples for 'alter table alter 
**          column' from no default to default.
**	04-Dec-2009 (troal01)
**	    Should now set the proper geometry type code.
**	21-Jan-2010 (troal01)
**	    If SRID has been specified, it is put in *len_prec with
**	    num_len_prec_vals set to zero. If it's a geospatial type
**	    SRID is retrieved from the pointer and assigned to the
**	    DMF_ATTR_ENTRY else, it defaults to -1.
**      16-Feb-2010 (hanal04) Bug 123292
**          Initialise attr_defaultTuple to avoid spurious values being
**          picked up.
**	22-Mar-2010 (toumi01) SIR 122403
**	    Save encryption settings.
*/
DB_STATUS
psl_ct14_typedesc(
	PSS_SESBLK	*sess_cb,
	char		*type_name,
	i4		num_len_prec_vals,
	i4		*len_prec,
	i4		null_def,
	PST_QNODE	*default_node,
	DB_TEXT_STRING	*default_text,
	DB_IISEQUENCE	*idseqp,
	PSQ_CB		*psq_cb,
	DB_COLL_ID	collationID,
	i4		encrypt_spec)
{
    ADF_CB		*adf_scb = (ADF_CB*) sess_cb->pss_adfcb;
    DB_STATUS	        status;
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    DMF_ATTR_ENTRY	**attrs;
    DMF_ATTR_ENTRY	*cur_attr;
    DB_DATA_VALUE	dt_dv;
    DB_DT_ID		dtype;
    i4			adi_flags = 0;
    i4		err_code;
    i4		        colno;
    i4                  bits;
    i4			i = 0;
    i4			enc_modulo;
    bool		nulldefaultproblem = FALSE;
    bool		identity = FALSE;

    /* Moved consistency check for added columns from grammar to here.
    ** "system_maintained" is checked here, not null/default stuff is
    ** checked later. */
    
    if ( (psq_cb->psq_mode == PSQ_ATBL_ADD_COLUMN) &&
	  (null_def & PSS_TYPE_SYS_MAINTAINED))
    {
	i4	err_code;

	_VOID_ psf_error(3857L, 0L, PSF_USERERR,
	     &err_code, &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }
    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;
    colno = sess_cb->pss_rsdmno - 1;
    cur_attr = attrs[colno];

    /* For backward compatibility with 6.4,
    ** just SYS_MAINTAINED => not null with default with system_maintained,
    ** so be sure those bits are turned on
    */
    if (null_def & PSS_TYPE_SYS_MAINTAINED)
    {
	null_def |= (PSS_TYPE_NOT_NULL | PSS_TYPE_DEFAULT);
    }
    
    /* set up nullability;
    ** either "with null" or nothing specified means that the column will be
    ** nullable, so set null bit as long as NOT NULL was NOT specified
    */
    if (~null_def & PSS_TYPE_NOT_NULL)
    {
	adi_flags |= ADI_F1_WITH_NULL;
    }


    status = adi_encode_colspec(adf_scb, type_name, num_len_prec_vals,
	len_prec, adi_flags, &dt_dv);

    if (status == E_DB_OK)
    {
	status = adi_dtinfo(adf_scb, dt_dv.db_datatype, &bits);
	if (status == E_DB_OK)
	{
	    if ((bits & AD_INDB) == 0)
		status = E_DB_ERROR;
	}
    }

    if (status != E_DB_OK)
    {
	if (adf_scb->adf_errcb.ad_errcode == E_AD5065_DATE_ALIAS_NOTSET)
    	{
	 /* send the text message to users */
         (VOID) psf_error(4320,
                         adf_scb->adf_errcb.ad_errcode, PSF_USERERR, &err_code,
                         &psq_cb->psq_error, 0);
          return (E_DB_ERROR);
    	}
	else if ((len_prec) && 
		 (num_len_prec_vals == 1) && 
		 (adf_scb->adf_errcb.ad_errcode == E_AD2006_BAD_DTUSRLEN))
	{

	 i4 userlen = *len_prec;

	 /* send the text message to users */
         (VOID) psf_error(2092, 0L,
			 PSF_USERERR, &err_code,
                         &psq_cb->psq_error, 4, 
			 sizeof(i4), &userlen,
	    		 psf_trmwhite(DB_ATT_MAXNAME, 
					(char *) &cur_attr->attr_name),
	    		 (char *) &cur_attr->attr_name,
	    		 STtrmwhite(type_name), type_name,
	    		 STtrmwhite(type_name), type_name);
          return (E_DB_ERROR);
    	}
	else
	  (VOID) psf_error((psq_cb->psq_mode == PSQ_REG_IMPORT)
	    ? E_PS110B_TBL_COLFORMAT : 2014L,
	    0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 2,
	    STtrmwhite(type_name), type_name,
	    psf_trmwhite(DB_ATT_MAXNAME, (char *) &cur_attr->attr_name),
	    (char *) &cur_attr->attr_name);

	return(status);
    }

    dtype = abs(dt_dv.db_datatype);
    if ((dtype == DB_NCHR_TYPE || dtype == DB_NVCHR_TYPE ||
	dtype == DB_LNVCHR_TYPE) && !adf_scb->adf_ucollation)
    {
	/* Defined nchar column without an nchar database. */
	(VOID) psf_error(2048L, 0L, PSF_USERERR, 
		&err_code, &psq_cb->psq_error, 0);
	return(E_DB_ERROR);
    }

    /* Check for identity column. */
    if (null_def & (PSS_TYPE_GENALWAYS | PSS_TYPE_GENDEFAULT))
    {
	identity = TRUE;
	if (dtype != DB_INT_TYPE && (dtype != DB_DEC_TYPE ||
			DB_S_DECODE_MACRO(dt_dv.db_prec) != 0))
	{
	    /* Identity column must be exact numeric scale 0. */
	    _VOID_ psf_error(E_PS04B7_BAD_IDENTITY_TYPE, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		psf_trmwhite(DB_ATT_MAXNAME, (char *) &cur_attr->attr_name),
		&cur_attr->attr_name);
	    return (E_DB_ERROR);
	}

	if (null_def & (PSS_TYPE_DEFAULT | PSS_TYPE_USER_DEFAULT))
	{
	    /* Can't mix identity with other default clause. */
	    _VOID_ psf_error(E_PS04B8_IDENTITY_DEFAULT, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		psf_trmwhite(DB_ATT_MAXNAME, (char *) &cur_attr->attr_name),
		&cur_attr->attr_name);
	    return (E_DB_ERROR);
	}

	null_def |= PSS_TYPE_DEFAULT | PSS_TYPE_USER_DEFAULT;

	/* Fill more into sequence definition - including type info. */
	idseqp->dbs_type = dtype;
	idseqp->dbs_length = (dt_dv.db_datatype < 0) ? dt_dv.db_length -1 :
		dt_dv.db_length;
	if (dtype == DB_INT_TYPE && idseqp->dbs_length < 4)
	    idseqp->dbs_length = 4;
	idseqp->dbs_prec = DB_P_DECODE_MACRO(dt_dv.db_prec);
	idseqp->dbs_flag |= DBS_SYSTEM_GENERATED;

	psl_csequence(sess_cb, idseqp, &psq_cb->psq_error);

	/* Allocate memory for sequence tuple and copy it over. */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(DB_IISEQUENCE), (PTR *)&cur_attr->attr_seqTuple,
			&psq_cb->psq_error);
	STRUCT_ASSIGN_MACRO(*idseqp, *cur_attr->attr_seqTuple);
    }
    else cur_attr->attr_seqTuple = (DB_IISEQUENCE *) NULL;

    /* Do collation checks - first assure that collation is
    ** applicable for data type, then assure that column level
    ** collation is supported for defined collation. */
    if (collationID > DB_NOCOLLATION)
    {
	status = psl_collation_check(psq_cb, sess_cb, &dt_dv, collationID);
	if (status != E_DB_OK)
	    return status;
    }

    cur_attr->attr_size = dt_dv.db_length;
    cur_attr->attr_type = dt_dv.db_datatype;
    cur_attr->attr_precision  = dt_dv.db_prec;
    cur_attr->attr_flags_mask = 0;
    cur_attr->attr_defaultTuple = (DB_IIDEFAULT *)NULL;
    cur_attr->attr_collID = collationID;

    /*
     * Initialize attr_geomtype and attr_srid
     */
   	while(geom_type_mapping[i].geom_type != GEOM_TYPE_UNDEFINED)
   	{
   		if(geom_type_mapping[i].db_type == abs(dt_dv.db_datatype))
   		{
   			break;
   		}
   		i++;
   	}
    cur_attr->attr_geomtype = geom_type_mapping[i].geom_type;
    if(cur_attr->attr_geomtype != GEOM_TYPE_UNDEFINED && len_prec != NULL)
    {
        cur_attr->attr_srid = *len_prec;
    }
    else
    {
    	cur_attr->attr_srid = SRID_UNDEFINED;
    }
    /* store attribute encryption flags and length
    */
    cur_attr->attr_encflags = 0;
    cur_attr->attr_encwid = 0;
    if (encrypt_spec & PSS_ENCRYPT)
    {
	/* peripheral data types are not valid for encryption */
	if ((bits & AD_PERIPHERAL) != 0)
	{
	    _VOID_ psf_error(E_PS0C85_ENCRYPT_INVALID, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		psf_trmwhite(DB_ATT_MAXNAME, (char *) &cur_attr->attr_name),
		&cur_attr->attr_name);
	    return (E_DB_ERROR);
	}

	cur_attr->attr_encflags |= ATT_ENCRYPT;
	cur_attr->attr_encwid = cur_attr->attr_size;
	if (encrypt_spec & PSS_ENCRYPT_SALT)
	{
	    cur_attr->attr_encflags |= ATT_ENCRYPT_SALT;
	    cur_attr->attr_encwid += AES_BLOCK;
	}
	cur_attr->attr_encflags |= ATT_ENCRYPT_CRC;
	cur_attr->attr_encwid += CRC_BYTES;
	if ((enc_modulo = cur_attr->attr_encwid % AES_BLOCK) != 0)
	    cur_attr->attr_encwid += AES_BLOCK - enc_modulo;
    }

    /* record that column is "known not nullable" (a SQL92 notion);
    ** will be used for future SQL92 INFO_SCHEMA catalogs
    */
    if (null_def & PSS_TYPE_NOT_NULL)
    {
	cur_attr->attr_flags_mask |= DMU_F_KNOWN_NOT_NULLABLE;
    }

    /* If identity column, set flags appropriately. */
    if (null_def & PSS_TYPE_GENALWAYS)
	cur_attr->attr_flags_mask |= DMU_F_IDENTITY_ALWAYS;
    else if (null_def & PSS_TYPE_GENDEFAULT)
	cur_attr->attr_flags_mask |= DMU_F_IDENTITY_BY_DEFAULT;

    /* set up defaultibility and system maintained 
     */
    status = psl_1col_default(sess_cb, null_def, cur_attr, 
			      default_text, default_node, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))				
	return(status);

    /* NOT NULL without DEFAULT is invalid
    ** no NOT NULL with DEFAULT is invalid
    ** Any specified value for the default is invalid (USER DEFAULT) */
    if (((null_def & PSS_TYPE_NOT_NULL) && !(null_def & PSS_TYPE_DEFAULT)) ||
	 (!(null_def & PSS_TYPE_NOT_NULL) && (null_def & PSS_TYPE_DEFAULT)))
		nulldefaultproblem = TRUE;

    if ((psq_cb->psq_mode == PSQ_ATBL_ADD_COLUMN) &&
	(nulldefaultproblem || (null_def & PSS_TYPE_USER_DEFAULT) ) )
    {
	_VOID_ psf_error(3857L, 0L, PSF_USERERR,
     		&err_code, &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

    /* Bug 116822 - Moved above the alter-column checks which try to
    ** copy the default value, so we check if it's an invalid one first
    */
    if ((null_def & PSS_TYPE_NOT_NULL) &&
	(null_def & PSS_TYPE_USER_DEFAULT) &&
	(cur_attr->attr_defaultID.db_tab_base == DB_DEF_ID_NULL) &&
	(cur_attr->attr_defaultID.db_tab_index == 0))
    {
	_VOID_ psf_error(5559L, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		psf_trmwhite(DB_ATT_MAXNAME, (char *) &cur_attr->attr_name),
		&cur_attr->attr_name);
	return (E_DB_ERROR);
    }

    if (psq_cb->psq_mode == PSQ_ATBL_ALTER_COLUMN)
    {
	i4	err_code;
	DMT_ATT_ENTRY	*alt_attr;

	alt_attr = pst_coldesc(sess_cb->pss_resrng,
                        cur_attr->attr_name.db_att_name, DB_ATT_MAXNAME);

	/* Check to see if column is known (kibro01) b119624 */
	if (alt_attr == NULL)
	{
	    _VOID_ psf_error(5560L, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		psf_trmwhite(DB_ATT_MAXNAME, (char *) &cur_attr->attr_name),
		&cur_attr->attr_name);
	    return (E_DB_ERROR);
	}

	/* While changing WITH NULL to NOT NULL is invalid, it is possible
	** to change NOT NULL to WITH NULL (kibro01) b118799
	*/
	if ( ((cur_attr->attr_type < 0) && (alt_attr->att_type > 0)) &&
	      !(null_def & PSS_TYPE_USER_DEFAULT) &&
	      (null_def & PSS_TYPE_DEFAULT) )
	{
	    /* If switching from NOT NULL to WITH NULL, and if we have not
	    ** specified NO DEFAULT, then automatically add the 
	    ** "with default" default value (kibro01) b118799 
	    */
	    null_def = PSS_TYPE_DEFAULT | PSS_TYPE_NULL;
	    cur_attr->attr_flags_mask |= DMU_F_DEFAULT;
	    cur_attr->attr_flags_mask &= ~DMU_F_NDEFAULT;
	    status = psl_1col_default(sess_cb, null_def, cur_attr, 
			      default_text, default_node, &psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))				
		return(status);
	}

	if ((null_def & PSS_TYPE_USER_DEFAULT) && (alt_attr->att_defaultID.db_tab_base))
	{
	    /*
	    ** alter column specifies default value, 
	    ** orig column has a default value
	    */

	    RDF_CB		rdf_cb;
	    RDR_INFO	rdrinfo;

	    /* 
	    ** get orig column's default value 
	    ** compare it against the specified def val
	    */
	    MEfill(sizeof(RDR_INFO), 0, (PTR)&rdrinfo);
	    rdrinfo.rdr_attr = &alt_attr;
	    rdrinfo.rdr_no_attr = 1;

	    /*
	    ** rdf_call() -> rdf_gdesc() -> rdu_default_bld()
	    ** rdr_attnametot is not used by rdu_default_bld()
	    ** resrng nametot:sess_cb->pss_resrng->pss_tabdesc->tbl_attr_nametot
	    ** alt_attr nametot: alt_attr->att_nmlen + 1
	    */
	    rdrinfo.rdr_attnametot = alt_attr->att_nmlen + 1;
	    pst_rdfcb_init(&rdf_cb, sess_cb);
	    rdf_cb.rdf_rb.rdr_types_mask  = RDR_ATTRIBUTES;
	    rdf_cb.rdf_rb.rdr_2types_mask = RDR2_DEFAULT;
	    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;

	    status = rdf_call(RDF_GETDESC, (PTR) &rdf_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
		switch (rdf_cb.rdf_error.err_code)
		{
		    case E_RD001D_NO_SUCH_DEFAULT:
			(void) psf_error(E_PS049C_NO_DEFAULT_FROM_RDF, 
			    0L, PSF_INTERR, &err_code, &psq_cb->psq_error, 
			    1, psf_trmwhite(sizeof(DB_TAB_NAME),
			    sess_cb->pss_resrng->pss_tabname.db_tab_name),
			    sess_cb->pss_resrng->pss_tabname.db_tab_name);
			break;
		    case E_RD001C_CANT_READ_COL_DEF:
			(void) psf_error(E_PS049D_RDF_CANT_READ_DEF, 
			    0L, PSF_INTERR, &err_code, &psq_cb->psq_error, 
			    1, psf_trmwhite(sizeof(DB_TAB_NAME),
			    sess_cb->pss_resrng->pss_tabname.db_tab_name),
			    sess_cb->pss_resrng->pss_tabname.db_tab_name);
			break;
		    default:
			(void) psf_rdf_error(RDF_GETDESC, &rdf_cb.rdf_error, 
			   &psq_cb->psq_error);
		}
		return(status);
	    }
	    /* If the default has the same value, use the same ID */
	    if (cur_attr->attr_defaultTuple != NULL)
	    {
		if (STcompare(cur_attr->attr_defaultTuple->dbd_def_value,
			      ((RDD_DEFAULT *)(alt_attr->att_default))->rdf_default) == 0)
		{
		    COPY_DEFAULT_ID(alt_attr->att_defaultID, 
				    cur_attr->attr_defaultID);
		}
		else
		{
		    /* (coomi01) Bug 120413
		    ** We need to flag qeu to call createDefaultTuples
		    ** at the appropriate time to create a new tuple in iidefaults.
		    */
		    qeu_cb->qeu_flag |= QEU_DFLT_VALUE_PRESENT;
		}
	    }

	    status = rdf_call(RDF_UNFIX, &rdf_cb);
	    if (DB_FAILURE_MACRO(status))
		(void) psf_rdf_error(RDF_UNFIX, &rdf_cb.rdf_error, 
		    &psq_cb->psq_error);
	}
	else if (null_def & PSS_TYPE_USER_DEFAULT) 
	{
	    /*
	    ** alter column specifies default value, 
	    ** orig column has no default value
	    */

	    /* Flag qeu to call createDefaultTuples */
	    qeu_cb->qeu_flag |= QEU_DFLT_VALUE_PRESENT;
	}


	/* While changing WITH NULL to NOT NULL is invalid, it is possible
	** to change NOT NULL to WITH NULL (kibro01) b118799
	*/
	/* Going from WITH NULL to NOT NULL is never allowed */
	if ((cur_attr->attr_type > 0) && (alt_attr->att_type < 0))
        {
	    _VOID_ psf_error(3859L, 0L, PSF_USERERR,
	        &err_code, &psq_cb->psq_error, 0);
	    return (E_DB_ERROR);
	}
	/* All other combinations will be picked up in dmf now, since it
	** is legitimate to amend the defaults and suchlike when this is
	** version 0 of the file 
	*/
    }

    if (null_def & PSS_TYPE_SYS_MAINTAINED)
    {
	/* system_maintained values must have defaults, so that
	** the rest of the system will compile in code to build
	** a correctly sized tuple which will be reset once it
	** gets down to dmf.
	*/

	/* system_maintained */
	cur_attr->attr_flags_mask |= DMU_F_SYS_MAINTAINED;

	if (    (abs(dt_dv.db_datatype) == DB_LOGKEY_TYPE)
	    ||  (abs(dt_dv.db_datatype) == DB_TABKEY_TYPE)
	   )
	{
	    cur_attr->attr_flags_mask |= DMU_F_WORM;
	}
	else
	{
	    (VOID) psf_error(E_US1901_6401_BAD_SYSMNT, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 2,
		STtrmwhite(type_name), type_name,
		psf_trmwhite(DB_ATT_MAXNAME, (char *) &cur_attr->attr_name),
		&cur_attr->attr_name);
	    return (E_DB_ERROR);
	}
    }

    return(E_DB_OK);

}  /* end psl_ct14_typedesc */

/*
** Name: psl_ct15_distr_with	- Semantic actions for handling the
**				  distributed-only with-clause (SQL)
**
** Description:
**	This routine performs the distributed-only semantic actions for
**	a with-clause.  It is called from the following productions:
**	    cr_nm_eq_nm	
**	    cr_nm_eq_str
**	These productions are used on the CREATE[AS SELECT] and, as
**	descendents of the ldb_spec_item, on the REGISTER, CREATE LINK,
**	DIRECT CONNECT, DIRECT DISCONNECT and DIRECT EXECUTE IMMEDIATE
**	statements.
**
**	DIRECT DISCONNECT allows no with-clause options, but uses the
**	WITH-clause production in order to give a meaningful error.
**	All other statements allow the options:
**	    node
**	    database
**	    dbms
**	The CREATE LINK and CREATE[AS SELECT] also allow the "table" option.
**	In addition, the CREATE[AS SELECT] statements allow a whole range
**	of other WITH-clause options, which are shipped on to the LDB
**	without examination.  Error checking for the non-distributed
**	options is therefore deferred to the local site.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_object	    ptr to QED_DDL_INFO
**	    pss_ostream	    ptr to mem stream for generation of text
**	    pss_distr_sflags  WITH-clause information
**	xlated_qry	    For query text buffering 
**	quoted_val	    TRUE of 'value' quoted
**	psq_cb		    PSF request CB
**	    qmode	    Mode of statement
**
** Outputs:
**     psq_cb
**         psq_error	    will be filled in if an error occurs
**	   ldbdesc	    will contain node, database and dbms info
**	xlated_qry	    List of packets pointed to will contain qry text
**	sess_cb
**	    pss_distr_sflags Updated to indicate WITH has been buffered.
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	15-jun-92 (barbara)
**	    Extracted from the 6.4 Star SQL grammar for Sybil.
**	23-oct-92 (barbara)
**	    Fixed problem on psq_x_add re generation of unknown with-clause
**	    options.
**	08-dec-92 (barbara)
**	    address of QEU_DDL_INFO will be stored in QEU_CB.qeu_ddl_info
**	    instead of overloading qeu_qso.
**	01-jun-93 (barbara)
**	    Do not initialize flag to indicate whether table name has been
**	    quoted if we are processing a REGISTER statement.  For a
**	    REGISTER statement we want to preserve the information about
**	    whether or not the table in the FROM clause was quoted.
*/
DB_STATUS
psl_ct15_distr_with(
	PSS_SESBLK	*sess_cb,
	char		*name,
	char		*value,
	PSS_Q_XLATE	*xlated_qry,
	bool		quoted_val,
	PSQ_CB		*psq_cb)
{
    i4	err_code;
    i4		qmode = psq_cb->psq_mode;
    DD_LDB_DESC	*ldb_desc;

    /* Shouldn't happen */
    if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
	return(E_DB_OK);

    /*
    ** NOTE: default LDB description has been stored in
    ** *psq_cb->psq_ldbdesc in yyvarsinit.h
    */
    ldb_desc = psq_cb->psq_ldbdesc;

    if (qmode != PSQ_REG_LINK)
    	sess_cb->pss_distr_sflags &= ~PSS_QUOTED_TBLNAME;	

    /* direct disconnect doesn't take a with-clause */
    if (qmode == PSQ_DIRDISCON)
    {
	(VOID) psf_error(2083L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

    if (STcompare(name, "node") == 0)
    {
	sess_cb->pss_distr_sflags |= PSS_NODE;
	STmove(value, ' ', sizeof(DD_NODE_NAME), ldb_desc->dd_l2_node_name);
	return (E_DB_OK);
    }

    else if (STcompare(name, "database") == 0)
    {
	sess_cb->pss_distr_sflags |= PSS_DB;
	STmove(value, ' ', sizeof(DD_256C), ldb_desc->dd_l3_ldb_name);
	return (E_DB_OK);
    }

    else if (STcompare(name, "dbms") == 0)
    {
	sess_cb->pss_distr_sflags |= PSS_DBMS;
	if (!quoted_val)
	    (VOID) CVupper(value);
	STmove(value, ' ', sizeof(ldb_desc->dd_l4_dbms_name), 
		ldb_desc->dd_l4_dbms_name);
	return (E_DB_OK);
    }

    if (qmode == PSQ_DIRCON || qmode == PSQ_DIREXEC)
    {
	(VOID) psf_error(2079L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, STlength(name), name);
	return (E_DB_ERROR);
    }

    if (qmode == PSQ_REG_LINK)
    {
	(VOID) psf_error(2080L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, STlength(name), name);
	return (E_DB_ERROR);
    }

    if (STcompare(name, "table") == 0)
    {
	DD_2LDB_TAB_INFO	*ldb_tab_info;

	if (qmode == PSQ_0_CRT_LINK)
	{
	    ldb_tab_info =
		((QED_DDL_INFO *) sess_cb->pss_object)->qed_d6_tab_info_p;
	}
	else if (qmode == PSQ_CREATE || qmode == PSQ_RETINTO)
	{
	    ldb_tab_info =
((QED_DDL_INFO *)((QEU_CB *)sess_cb->pss_object)->qeu_ddl_info)->qed_d6_tab_info_p;
	}
	else
	{
	    (VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR, &err_code,
			&psq_cb->psq_error, 0);
	    return (E_DB_ERROR);
	}

	sess_cb->pss_distr_sflags |= PSS_LDB_TABLE;

	if (quoted_val)
	    sess_cb->pss_distr_sflags |= PSS_QUOTED_TBLNAME;

	STmove(value, ' ', sizeof(DD_TAB_NAME), ldb_tab_info->dd_t1_tab_name);
    }

    else if (qmode == PSQ_0_CRT_LINK)
    {
	(VOID) psf_error(2087L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, STlength(name), name);
	return (E_DB_ERROR);
    }
    else
    {
	/*
	** Non-distributed-only parameter on CREATE [AS SELECT].  It
	** is buffered for passing (unverified) on to the LDB.
	** LDB will report error if any.  Parameter should not be quoted.
	*/
	char		*str;
	DB_STATUS	status;

	if (quoted_val)
	{
	    (VOID) psf_error(E_PS03A7_QUOTED_WITH_CLAUSE, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 0);
	    return (E_DB_ERROR);
	}

	if (sess_cb->pss_distr_sflags & PSS_PRINT_WITH)
	{
	    str = " with ";
	    sess_cb->pss_distr_sflags &= ~PSS_PRINT_WITH;	
	}
	else
	{
	    str = ", ";
	}

	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			    &xlated_qry->pss_q_list, (i4) -1, (char *) NULL,
			    (char *) NULL, str, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))				
	    return(status);

	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			    &xlated_qry->pss_q_list, (i4) -1, (char *) NULL,
			    (char *) NULL, name, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	    return(status);

	status = psq_x_add(sess_cb, value, &sess_cb->pss_ostream,
			    xlated_qry->pss_buf_size,
			    &xlated_qry->pss_q_list, STlength(value), "=", 
			    " ", (char *) NULL, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    return (E_DB_OK);
}

/*
** Name: psl_ct16_distr_create	- Semantic actions for the distributed-only
**				  parts of the create_table: (SQL) 
**				  production.
**
** Description:
**	This routine performs the distributed-only semantic actions for
**	create_table: (SQL). This function is used when parsing a "simple" 
**	CREATE and a CREATE AS SELECT.  It is called after the call to 
**	psl_ct1_create_table().
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	  pss_lang	    query language
**	  pss_object	    ptr to QEU_CB
**	  pss_distr_sflags   WITH clause flags
**	xlated_qry	    For query text buffering 
**	simple_create	    TRUE if not CREATE AS SELECT
**	psq_cb		    parser control block
**	  psq_ldbdesc	    ptr to LDB description from WITH clause
**	  psq_dfltldb	    ptr to CDB description
**
** Outputs:
**	psq_cb
**	  psq_error	    will be filled in if an error occurs
**	sess_cb
**	  pss_object	    query text will be hooked into QED_DDL_INFO
**			    nested inside QEU_CB
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	15-jun-92 (barbara)
**	    Extracted from the 6.4 Star SQL grammar for Sybil.
**	08-dec-92 (barbara)
**	    address of QEU_DDL_INFO will be stored in QEU_CB.qeu_ddl_info
**	    instead of overloading qeu_qso.
**	01-mar-93 (barbara)
**	    Star must generate an LDB table name on the CREATE statement
**	    that conforms case-wise to the way the table was looked
**	    up in the LDB's catalogs.
**	02-sep-93 (barbara)
**	    1. No case translation required for table name on CREATE TABLE text.
**	       RDF has already done in-place case translation for us.
**	    2. Star delimited ids: To decide whether or not to delimit, Star now
**	       relies on the presence of the LDB's DB_DELIMITED_CASE capability
**	       instead of its OPEN/SQL_LEVEL.
**	10-sep-93 (tam)
**	    remove & reference in err_blk which caused E_SC0206 (b54416).
**	    Padded an extra space after the location field for local query 
**	    buffer in star.
**	13-oct-93 (barbara)
**	    Registrations text for tables created through Star should
**	    state registration "as native" as oposed to "as link".  Set
**	    PSS_REG_AS_NATIVE flag before calling psl_rg4_regtext.
**	28-jan-94 (peters)
**	    B56184: Closing ")" is missing from query to local server.
**	    Changed arguments in second call to psq_x_add after
**	    'if (ldb_flags & PSS_LOCATION)':  ") " is now passed as 8th argument
**	    rather than 7th argument to psq_x_add() to make sure ")" is always
**	    put in.
*/
DB_STATUS
psl_ct16_distr_create(
	PSS_SESBLK	*sess_cb,
	PSS_Q_XLATE	*xlated_qry,
	bool		simple_create,
	PSQ_CB		*psq_cb)
{
    QEU_CB		*qeu_cb = (QEU_CB *) sess_cb->pss_object;
    DMU_CB		*dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    QED_DDL_INFO	*ddl_info = (QED_DDL_INFO *) qeu_cb->qeu_ddl_info;
    DD_2LDB_TAB_INFO	*ldb_tab_info = ddl_info->qed_d6_tab_info_p;
    char		*ch2;
    bool		ldb_not_cdb;
    i4		err_code;
    DB_STATUS		status;
    DB_ERROR		*err_blk = &psq_cb->psq_error;
				/*
				** we may end up clearing this bit if
				** the new LDB object will be owned by
				** the LDB system catalog owner
				*/
    i4			mask = PSS_DUP_TBL;
    i4		ldb_flags = sess_cb->pss_distr_sflags;

    if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    /*
    ** make sure that none or both of NODE and DATABASE clauses were found
    */
    if (   (ldb_flags & PSS_NODE || ldb_flags & PSS_DB)
    	&& (~ldb_flags & PSS_NODE || ~ldb_flags & PSS_DB)  )
    {
	(VOID) psf_error(2031L, 0L, PSF_USERERR, &err_code,
			 err_blk, 1, STlength("CREATE TABLE"),
			 "CREATE TABLE");
	return(E_DB_ERROR);
    }

    /* store LINK name for QEF */
    MEcopy((PTR) &dmu_cb->dmu_table_name, sizeof(DB_TAB_NAME),
	   (PTR) ddl_info->qed_d1_obj_name);

    /*
    ** if the name of the distributed object starts with II, it will be
    ** owned by $ingres rather than the current user (assuming current
    ** user has catalog update privileges).
    */
    if (!CMcmpnocase(ddl_info->qed_d1_obj_name, &SystemCatPrefix[0]))
    {
	ch2 = ddl_info->qed_d1_obj_name +
	      CMbytecnt(ddl_info->qed_d1_obj_name);
	if (!CMcmpnocase(ch2, &SystemCatPrefix[1]))
	{
	    /*@FIX_ME@ Should we use sess_cb->pss_cat_owner here? */
	    MEmove(STlength(SystemSyscatOwner), (PTR) SystemSyscatOwner, ' ',
		sizeof(ddl_info->qed_d2_obj_owner),
		(PTR) ddl_info->qed_d2_obj_owner);
	}
    }

    /* if TABLE was not specified, default to link_name */
    if (~ldb_flags & PSS_LDB_TABLE)
    {
	MEcopy((PTR) ddl_info->qed_d1_obj_name, sizeof(DD_TAB_NAME),
	       (PTR) ldb_tab_info->dd_t1_tab_name);
    }

    if (ldb_flags & PSS_DELIM_TBLNAME)
    {
	mask |= PSS_DELIM_TBL;
    }
    if (~ldb_flags & PSS_QUOTED_TBLNAME)
    {
	/* if the LDB table name was not specified or was not quoted,
	** let RDF uppercase it if IIDBCAPABILITIES indicates that table
	** names must be uppercased
	*/
	mask |= PSS_GET_TBLCASE;
    }
    if (*sess_cb->pss_dbxlate & CUI_ID_DLM_M)
    {
	mask |= PSS_DDB_MIXED;
    }
    ldb_not_cdb =
	    (ldb_flags & (PSS_DBMS | PSS_NDE_AND_DB) &&
	    !psq_same_site(psq_cb->psq_dfltldb, psq_cb->psq_ldbdesc));

    /*
    ** if the name of the LDB object starts with II, and it will be
    ** located on the CDB, it will be owned by the local system catalog
    ** owner rather than the current user
    */
    if (!ldb_not_cdb && 
        !CMcmpnocase(ldb_tab_info->dd_t1_tab_name, &SystemCatPrefix[0]))
    {
	ch2 = ldb_tab_info->dd_t1_tab_name +
	      CMbytecnt(ldb_tab_info->dd_t1_tab_name);
	if (!CMcmpnocase(ch2, &SystemCatPrefix[1]))
	{
	    mask &= ~PSS_DUP_TBL;
	    mask |= PSS_DUP_CAT;
	}
    }

    /* check if the LDB table owned by this user already exists */
    status = pst_ldbtab_desc(sess_cb, ddl_info, &sess_cb->pss_ostream, mask, err_blk);

    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    ddl_info->qed_d3_col_count =
	ldb_tab_info->dd_t7_col_cnt = sess_cb->pss_rsdmno;

    /* 
    **check if CREATE TABLE LOC:NAME was specified
    */
    if (ldb_flags & PSS_LOCATION)
    {
	char		*c1 = (ldb_flags & PSS_PRINT_WITH) ? "with" : ",";
	char		*bl;
	i4		len;

	status = psq_x_add(sess_cb, c1, &sess_cb->pss_ostream,
			   xlated_qry->pss_buf_size,
			   &xlated_qry->pss_q_list, (i4) -1,
			   (char *) NULL, (char *) NULL,
			   " location = (", err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
	
	bl = STindex(dmu_cb->dmu_location.data_address, " ",
			     sizeof(DB_LOC_NAME));

	len = (bl == (char *) NULL) ? (i4)sizeof(DB_LOC_NAME) : -1;

	status = psq_x_add(sess_cb, dmu_cb->dmu_location.data_address,
			   &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
			   &xlated_qry->pss_q_list, len,
			   " ", " ", ") ", err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
    }

    if (simple_create == TRUE)
    {
	/* preappend CREATE TABLE table_name to the translated text */

	DD_PACKET	*new_pkt;
	u_i2		len = (u_i2) STlength("create table ");
	i4		pkt_len;
	char		*c1 = " ";
    	u_char		unorm_tab[DB_TAB_MAXNAME *2 +2];
	char		*bufp;
    	u_i4		unorm_len=DB_TAB_MAXNAME *2 +2;
	u_i4		name_len;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_PACKET),
				(PTR *) &new_pkt, err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

	/*
	** Table name may require delimited identifiers for sending to
 	** the LDB.  Case translation is not required because RDF did
    	** in-place case translation as appropriate for the LDB when
    	** it checked the table's existence.
	*/

	name_len = (u_i4) psf_trmwhite(sizeof(DD_TAB_NAME),
						ldb_tab_info->dd_t1_tab_name);
	if (ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.
				dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
	{
	    status = cui_idunorm((u_char *)ldb_tab_info->dd_t1_tab_name,
				&name_len, unorm_tab, &unorm_len, CUI_ID_DLM,
				err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
	    	return(status);
	    }
	    bufp = (char *)unorm_tab;
	    name_len = unorm_len;
	}
	else
	{
	    bufp = ldb_tab_info->dd_t1_tab_name;
	}

	pkt_len = len + name_len + CMbytecnt(" ");
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, pkt_len,
			    (PTR *) &new_pkt->dd_p2_pkt_p, err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

	MEcopy((PTR) "create table ", len, (PTR) new_pkt->dd_p2_pkt_p);
	MEcopy((PTR) bufp, name_len, 
		(PTR) (new_pkt->dd_p2_pkt_p + len));
	CMcpychar((u_char *)c1, (u_char *) (new_pkt->dd_p2_pkt_p + len + name_len));

	new_pkt->dd_p1_len = pkt_len;
	new_pkt->dd_p3_nxt_p = xlated_qry->pss_q_list.pss_head;
	xlated_qry->pss_q_list.pss_head = new_pkt;


	/* we need to pad query with 1 blank */
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
		       &xlated_qry->pss_q_list, (i4) -1,
		       (char *) NULL, (char *) NULL, " ", err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

	ddl_info->qed_d5_qry_info_p->qed_q4_pkt_p =
					   xlated_qry->pss_q_list.pss_head;

    } /* if simple_create */

    /*
    ** Finally, we will generate text of query to be stored in
    ** IIREGISTRATIONS.
    */

    sess_cb->pss_stmt_flags |= PSS_REG_AS_NATIVE;

    status = psl_rg4_regtext(sess_cb, DD_2OBJ_TABLE, xlated_qry, psq_cb);

    if (DB_FAILURE_MACRO(status))
	return (status);

    ddl_info->qed_d9_reg_info_p->qed_q4_pkt_p =
				       xlated_qry->pss_q_list.pss_head;
    return(status);
}

/*
** Name: psl_ct17_distr_specs	- Semantic actions for the distributed-only
**				  actions of the tbl_elem: (SQL) production
**
** Description:
**	This routine performs the distributed-only semantic actions for
**	tbl_elem: (SQL).   The type specification is copied from the
**	scanner's buffer into the DD_PACKET list.  (A pointer to the
**	beginning of the column specifications (the left paren) was saved
**	in psl_ct13_newcolname()). pss_nxtchar marks the current place in
**	the scanner's buffer, i.e. the comma or the right paren.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	  pss_nxtchar	    ptr into current place in scanner buffer
**	xlated_qry	    For query text buffering 
**	scanbuf_ptr	    Pointer to type specification text
**
** Outputs:
**	err_blk 	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	21-aug-92 (barbara)
**	    Written as part of sybil merge.
**	01-mar-93 (barbara)
**	    Delimited id support for Star.  The buffering of CREATE TABLE text
**	    is now done in pieces in order to delimit each column name.  This
**	    function is changed to buffer a single column type specification
**	    instead of the whole column list.
**	06-mar-98 (nicph02)
**	    Bug 88490: The xlated_quey is getting the wrong delim (',' instead
**	    of ')') because only space characters were jumped but not CR ones.
*/
DB_STATUS
psl_ct17_distr_specs(
      PSS_SESBLK      *sess_cb,
      PTR	      scanbuf_ptr,
      PSS_Q_XLATE     *xlated_qry,
      DB_ERROR        *err_blk)
{
    u_char *c1 = sess_cb->pss_prvtok;
    char	*delim;
    DB_STATUS	status = E_DB_OK;

    while (CMspace(c1) || !CMcmpcase(c1, "\n"))
	CMnext(c1);
    if (!CMcmpcase(c1, ")"))
    {
	delim = ")";
    }
    else
    {
	delim = ",";
    }
    status = psq_x_add(sess_cb, scanbuf_ptr, &sess_cb->pss_ostream,
        xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
        (i4) ((char *) sess_cb->pss_prvtok - (char *) scanbuf_ptr),
        " ", delim, (char *) NULL, err_blk);
    return (status);
}

/*
** Name: psl_find_18colname  -	find column name/parameter name;
** 				used for error message in psl_ct18s_type_qual.
**
** Description:
** 	    Finds the current column name or parameter name, and passes it back
** 	    in the supplied pointer variable.
** 	    
**          For table operations, the column name is in a dmu_cb, which is
**          hanging off the qeu_cb attached to the session control block.
**          For create procedure, the parameter name is in a queue of variable
**          names; this is usually kept in the dbpinfo block, except for
**          set-input parameters, which are stored off of the sess_cb.
**
** Inputs:
**	qmode		    query mode/statement type
**	    PSQ_CREATE,etc	    CREATE TABLE-check for duplicates/conflicts
**	    PSQ_REG_IMPORT	    gateway REGISTER--extra error checks
**	    PSQ_CREDBP		    CREATE PROCEDURE
**	sess_cb		    session control block; 
**			        used to get column name for table errors
**	dbpinfo		    database procedure info block;
**			        used to get parameter name for procedure errors
**
** Outputs:
**	colname		    pointer to column name/parameter name
**	    	
**
** Returns:
**	none
**
** Side effects:
**	none
**
** History:
**
**	07-jul-93 (rblumer)
**	    written
**	22-oct-93 (rblumer)
**	    pss_setparmq is now in dbpinfo, not in sess_cb.
*/
static
void
psl_find_18colname(
		   i4		qmode,
		   PSS_SESBLK	*sess_cb,
		   PSS_DBPINFO	*dbpinfo,
		   char 	**colname)
{
    if (qmode == PSQ_CREDBP)
    {
	/* for create procedure, parameter name is in one of
	** two possible 'varq' variable queues; 
	** which one depends on whether this is a set-parameter or not
	*/
	QUEUE		*varq;

	if (sess_cb->pss_dbp_flags & PSS_PARSING_SET_PARAM)
	    varq = &(dbpinfo->pss_setparmq);
	else
	    varq = &(dbpinfo->pss_varq);

	*colname = ((PSS_DECVAR *)varq->q_next)->pss_varname.db_parm_name;
    }
    else 
    {
	/* must be a table operation (create/register/etc);
	** column name is in the dmu_cb attribute array
	*/
	QEU_CB		*qeu_cb;
	DMU_CB		*dmu_cb;
	DMF_ATTR_ENTRY	**attrs;
	i4 		colno;

	qeu_cb = (QEU_CB *) sess_cb->pss_object;
	dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
	attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;
	colno = sess_cb->pss_rsdmno - 1;

	*colname = attrs[colno]->attr_name.db_att_name;
    }

} /* end psl_find_18colname */

    
/*
** Name: psl_ct18s_type_qual  - check for duplicate & conflicting qualifications
**
** Description:	    
**          Check for duplicate and conflicting column qualifications.
**	    Now that the productions allow the different qualificatons in
**	    any order, they do not automatically prevent duplicate or
**	    conflicting specifications of the same type (i.e. defaults,
**	    nulls, or sys_maintained). Thus we have to check
**	    them explicitly here .
**
**          Will return an error for any duplicates, or for any conflicting
**	    default, null, or sys_maintained specifications.
**
** Inputs:
**	qmode		    query mode/statement type
**	    PSQ_CREATE,etc	    CREATE TABLE-check for duplicates/conflicts
**	    PSQ_REG_IMPORT	    gateway REGISTER--extra error checks
**	    PSQ_CREDBP		    CREATE PROCEDURE
**	qual1		    bitmap of existing qualifications
**	qual2		    bitmap of new (just-parsed) qualification
**	sess_cb		    session control block; 
**			        used to get column name for table errors
**	dbpinfo		    database procedure info block;
**			        used to get parameter name for procedure errors
**
** Outputs:
**	err_blk		    filled in if an error occurs
**	    	
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	none
**
** History:
**
**	01-sep-92 (rblumer)
**	    written
**	16-nov-1992 (bryanp)
**	    Fix test for NOT NULL NOT DEFAULT in register table.
**	02-jul-93 (rblumer)
**	    to make error messages more helpful, added column name to them;
**	    added sess_cb and dbpinfo as parameters in order to get the column
**	    name, plus added call to psl_find_18colname to get the name.
**	12-jan-2007 (dougi)
**	    Skip duplicate attr declaration for NOT NULL.
*/
DB_STATUS
psl_ct18s_type_qual(
		    i4		qmode,
		    i4	qual1,
		    i4	qual2,
		    PSS_SESBLK	*sess_cb,
		    PSS_DBPINFO	*dbpinfo,
		    DB_ERROR	*err_blk)
{
    register i4 temp;
    i4  err_code;
    char     command[PSL_MAX_COMM_STRING];
    i4  length;
    char     *colname;
    
    /* for backward compatibility with RMS gateway,
    ** check for illegal combinations BEFORE checking 
    ** for duplicates or conflicts
    */
    if (qmode == PSQ_REG_IMPORT)
    {
	temp = qual1 | qual2;
	
	/* schang: GW error
	**     RMS gateway only allows
	**        "nothing"   - implies NOT NULL WITH DEFAULT
        **	  NOT  NULL
        **	  WITH NULL
        **	  NOT  NULL WITH DEFAULT
        **	  NOT  NULL NOT  DEFAULT
	**
	** so check for the following illegal combinations (rblumer)
	**        DEFAULT <value>
	**        WITH NULL WITH DEFAULT
	**        WITH NULL NOT  DEFAULT
	*/
	if (   (temp & PSS_TYPE_USER_DEFAULT)
	    || ((temp & PSS_TYPE_NULL) && (   (temp & PSS_TYPE_DEFAULT)
					   || (temp & PSS_TYPE_NDEFAULT))))
	{
	    (VOID) psf_error(8003L, 0L, PSF_USERERR, &err_code,
			     err_blk, 1,
			     STlength(ERx("default")), ERx("default"));
	    return (E_DB_ERROR);
	}
    }  /* end if qmode == PSQ_REG_IMPORT */

    /* check for duplicate qualification, but not NOT NULL since that
    ** may happen as result of UNIQUE/PRIMARY KEY constraint. */
    if ((qual1 & qual2) && !(qual1 & PSS_TYPE_NOT_NULL))
    {
	psl_find_18colname(qmode, sess_cb, dbpinfo, &colname);
	psl_command_string(qmode, sess_cb, command, &length);
    
	_VOID_ psf_error(E_PS0479_DUP_COL_QUAL, 0L, PSF_USERERR,
			 &err_code, err_blk, 2,
			 length, command,
			 psf_trmwhite(DB_ATT_MAXNAME, colname), colname);
	return (E_DB_ERROR);
    }
    
    temp = qual1 | qual2;
    
    /* check for conflicting qualifications;
    **
    ** note that the only SYS_MAINTAINED options allowed are:
    **     WITH SYSTEM_MAINTAINED
    **     NOT NULL WITH SYSTEM_MAINTAINED
    **     NOT NULL WITH DEFAULT WITH SYSTEM_MAINTAINED
    */
    if (   ((temp & PSS_TYPE_NULL)     && (temp & PSS_TYPE_NOT_NULL))
	|| ((temp & PSS_TYPE_NDEFAULT) && (temp & PSS_TYPE_DEFAULT))
	|| ((temp & PSS_TYPE_SYS_MAINTAINED) 
	                               && (temp & PSS_TYPE_NOT_SYS_MAINTAINED))
	|| ((temp & PSS_TYPE_NULL)     && (temp & PSS_TYPE_SYS_MAINTAINED))
	|| ((temp & PSS_TYPE_NDEFAULT) && (temp & PSS_TYPE_SYS_MAINTAINED))
	|| ((temp & PSS_TYPE_USER_DEFAULT) && (temp & PSS_TYPE_SYS_MAINTAINED)))
    {
	psl_find_18colname(qmode, sess_cb, dbpinfo, &colname);
	psl_command_string(qmode, sess_cb, command, &length);

	_VOID_ psf_error(E_PS047A_CONFLICT_COL_QUAL, 0L, PSF_USERERR, 
			 &err_code, err_blk, 2,
			 length, command,
			 psf_trmwhite(DB_ATT_MAXNAME, colname), colname);
	return (E_DB_ERROR);
    }
    
    return (E_DB_OK);

}  /* end psl_ct18s_type_qual */


/*
** Name: psl_ct19s_constraint  - store away info for constraints
**
** Description:	    
**          Allocate a PSS_CONS structure and store info for constraints
**          into it.  This function does not actually attach the
**          constraint to the list of constraints stored in the YYVARS
**          structure; that is done by psl_ct21s_cons_name.
**      
**          Handles both col_constraint and tbl_constraint productions for
**          CREATE TABLE.
**
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_object	    ptr to QEU_CB (which contains DMU_CB)
**	    pss_ostream	    ptr to mem stream for allocating PSS_CONS structure
**	    pss_tchain	    text chain (with text of CHECK constraint)
**	    pss_tchain2	    text chain (with modified text of CHECK constraint)
**	type		    bitmap holding type of constraint
**                             one of the following MUST be set
**                              	PSS_CONS_UNIQUE
**					PSS_CONS_CHECK
**					PSS_CONS_REF
**			       and one or more of the following may be set:
**					  PSS_CONS_COL (column constraint)
**			                  PSS_CONS_PRIMARY
**			                  PSS_CONS_NOT_NULL
**	cons_cols	    list of columns in unique or ref constraint
**	ref_tabname         referenced table name   for ref constraint
**	ref_cols	    referenced column names for ref constraint
**	check_cond	    check condition (query tree) for check constraint
**
** Outputs:
**	consp		    will point to newly allocated PSS_CONS structure
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**	may set sess_cb->pss_tchain and sess_cb->pss_tchain2 to NULL
**
** History:
**
**	01-sep-92 (rblumer)
**	    written
**	12-dec-92 (ralph)
**	    CREATE_SCHEMA:
**	    For column constraints (PSS_COLS_COL), cols_cols points to single
**	    PSY_COL node in YYVARS that contains the constraint column.
**	    This is done to avoid allocating all of the DMU
**	    structures when they are not needed for CREATE SCHEMA.
**	20-nov-92 (rblumer)
**	    added code to get text for CHECK constraints from TXTemit chain
**	16-aug-93 (rblumer)
**	    moved most of the CHECK constraint code inside IF statement,
**	    since it does not need to be checked for NOT NULL constraints.
**	    This allows us to call it during CREATE TABLE AS SELECT processing.
**	27-oct-93 (rblumer)
**	    return an error if the schema/owner name of the referenced table in
**	    referential constraint was specified with a single-quoted username.
**	15-jan-94 (rblumer)
**	    B58783: fix ERlookup error in error 2715 by passing the address of
**	    a i4 variable to psf_error instead of passing a constant.
*/
DB_STATUS
psl_ct19s_constraint(
		     PSS_SESBLK	  *sess_cb,
		     i4	  type,
		     PSY_COL	  *cons_cols,
		     PSS_OBJ_NAME *ref_tabname,
		     PSY_COL	  *ref_cols,
		     PSS_TREEINFO *check_cond,
		     PSS_CONS	  **consp,
		     DB_ERROR	  *err_blk)
{
    DB_STATUS	status;
    PSS_CONS   	*cons;
    i4	err_code;
    PSY_COL	*temp_cols;
    DB_TEXT_STRING  *txt;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PSS_CONS),
			(PTR *) consp, err_blk);
    if (status != E_DB_OK)
	return (status);

    cons = *consp;

    /* initialize the structure
    ** which includes the embedded DMU_CHARACTERISTICS, etc
     */
    MEfill(sizeof(PSS_CONS), (u_char) 0, (PTR) cons);

    cons->pss_cons_type = type;
    (VOID) QUinit((QUEUE *) &cons->pss_cons_colq);
    (VOID) QUinit((QUEUE *) &cons->pss_ref_colq);
    
    /* if this is a column constraint, 
    ** get the column name so we can store it below 
    ** (note: the column name is saved in psl_ct13_newcolname() routine)
    */
    if (type & PSS_CONS_COL)
    {
	temp_cols = cons_cols;

	/* cons_cols will point to single static node for column constraints,
	** so can overwrite it here (and let it get stored in switch stmt)
	*/
	
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PSY_COL),
			    (PTR *) &cons_cols, err_blk);
	if (status != E_DB_OK)
	    return (status);

	MEfill(sizeof(PSY_COL), (u_char) 0, (PTR) cons_cols);
	QUinit((QUEUE *) cons_cols);

	STRUCT_ASSIGN_MACRO(temp_cols->psy_colnm, cons_cols->psy_colnm);
    }    

    switch (type & (PSS_CONS_UNIQUE | PSS_CONS_REF | PSS_CONS_CHECK))
    {
    case PSS_CONS_UNIQUE:
	/* make column queue point to the EXISTING queue cons_cols
	** by "inserting" it before the first column in the existing queue
	*/
	QUinsert((QUEUE *) &cons->pss_cons_colq, 
	    (QUEUE *) cons_cols->queue.q_prev);
	break;
	
    case PSS_CONS_REF:
	/* make column queues point to the EXISTING queues
	** by "inserting" them before the first column in the existing queues
	*/
	QUinsert((QUEUE *) &cons->pss_cons_colq, 
	    (QUEUE *) cons_cols->queue.q_prev);

	/* ref_cols can be null if user didn't specify them;
	** in that case we will look for a primary key in psl_verify_cons
	*/
	if (ref_cols != (PSY_COL *) NULL)
	{
	    QUinsert((QUEUE *) &cons->pss_ref_colq,  
		(QUEUE *)  ref_cols->queue.q_prev);
 	}

	/* single-quoted usernames are not allowed in referential constraints;
	** users should use delimited (or regular) identifiers instead
	*/
	if (ref_tabname->pss_schema_id_type == PSS_ID_SCONST)
	{
	    i4 lineno = 1L;
	    
	    (void) psf_error(2715L, 0L, PSF_USERERR, &err_code,
			     err_blk, 2, (i4) sizeof(lineno), &lineno,
			     psf_trmwhite(DB_OWN_MAXNAME,
					  ref_tabname->pss_owner.db_own_name),
			     ref_tabname->pss_owner.db_own_name);
	    return(E_DB_ERROR);
	}

	STRUCT_ASSIGN_MACRO(*ref_tabname, cons->pss_ref_tabname);
	break;

    case PSS_CONS_CHECK:
	/* for check constraints, cons_cols is only used 
	** for column-level constraints; 
	** in that case, need to keep column name around so can verify 
	** that the check condition only uses that column
	*/
	if (type & PSS_CONS_COL)
	{
	    QUinsert((QUEUE *) &cons->pss_cons_colq, 
		(QUEUE *) cons_cols->queue.q_prev);
	}

	/* make some semantic checks about the condition,
	** then put constraint text into contiguous block of QSF memory
	** and store ptr in constraint info block
	** (but don't do this for NOT NULL constraints, as their text 
	**  must show up as 'CHECK (c is not null)' and so is generated 
	**  by the system later)
	*/
	if (~cons->pss_cons_type & PSS_CONS_NOT_NULL)
	{
	    /* make sure CHECK condition doesn't contain any subselects 
	    ** or aggregates.
	    ** This is for Entry Level SQL92; Full Level SQL92 WILL allow them.
	    */
	    if (   (check_cond == (PSS_TREEINFO *) NULL)
		|| (check_cond->pss_mask & PSS_SUBSEL_IN_TREE)
		|| (sess_cb->pss_stmt_flags & PSS_AGINTREE))
	    {
		_VOID_ psf_error(E_PS0471_CHECK_CONSTRAINT, 0L, PSF_USERERR,
				 &err_code, err_blk, 1,
				 sizeof(ERx("CREATE/ALTER TABLE"))-1,
				 ERx("CREATE/ALTER TABLE"));
		return (E_DB_ERROR);
	    }
	    
	    /* reset PSS_AGINTREE in case have more constraints or other stmts
	    ** (this is unnecessary now because of the error check above, 
	    **  but when we allow aggregates (for Full Level SQL92),
	    **  clearing the flag will become necessary.)
	    */
	    sess_cb->pss_stmt_flags &= ~PSS_AGINTREE;
	    
	    /* store check condition in constraint info block
	     */
	    cons->pss_check_cond = check_cond;

	    /* now put constraint text into contiguous block of QSF memory
	    ** and store ptr in constraint info block
	    */
	    status = psq_tmulti_out(sess_cb,
				    (PSS_USRRANGE *) NULL, 
				    sess_cb->pss_tchain,
				    &sess_cb->pss_ostream, &txt, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
		  
	    cons->pss_check_text = txt;

	    /* de-allocate memory for text chain,
	    ** and clear its pointer so we don't try to free it again 
	    */
	    status = psq_tclose(sess_cb->pss_tchain, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    sess_cb->pss_tchain = (PTR) NULL;


	    /* also need to do same things for 2nd text chain
	    ** (used to create rule for enforcing CHECK constraints)
	    */
	    status = psq_tmulti_out(sess_cb,
				    (PSS_USRRANGE *) NULL, 
				    sess_cb->pss_tchain2,
				    &sess_cb->pss_ostream, &txt, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
		  
	    cons->pss_check_rule_text = txt;

	    /* de-allocate memory for text chain,
	    ** and clear its pointer so we don't try to free it again 
	    */
	    status = psq_tclose(sess_cb->pss_tchain2, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    sess_cb->pss_tchain2 = (PTR) NULL;
	}
	
	break;
	
    default:
	/* should never happen */
	(VOID) psf_error(E_PS0486_INVALID_CONS_TYPE, 0L, 
			 PSF_INTERR, &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }
    
    return (E_DB_OK);

}  /* end psl_ct19s_constraint */


/*
** Name: psl_ct20s_cons_col  - store away column info for constraints
**
** Description:	    
**          Allocate a PSY_COL structure and store the new column name
**          in it, then point it at itself.  Does not add it to the list 
**          of other columns names (if any), or attach the list to the 
**          constraint info block; that is done later by psl_ct19s_constraint.
**      
** Inputs:
**	sess_cb		    PSF session CB
**	    pss_ostream	    ptr to mem stream for allocating PSS_COL structure
**	newcolname	    newly found (parsed) column name
**
** Outputs:
**	colp	            will point to newly allocated column entry
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	01-sep-92 (rblumer)
**	    written
*/
DB_STATUS
psl_ct20s_cons_col(
		   PSS_SESBLK	*sess_cb,
		   PSY_COL	**colp,
		   char	  	*newcolname,
		   DB_ERROR	*err_blk)
{
    DB_STATUS	status;
    PSY_COL   	*newcol;
    
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(PSY_COL), (PTR *) colp, err_blk);
    if (status != E_DB_OK)
	return (status);

    newcol = *colp;

    /* initialize the structure
     */
    MEfill(sizeof(PSY_COL), (u_char) 0, (PTR) newcol);

    STmove(newcolname, ' ', 
	   sizeof(DB_ATT_NAME), newcol->psy_colnm.db_att_name);
    
    /* have newcol point to itself
    ** and return a pointer to it
    */
    QUinit((QUEUE *) newcol);

    *colp = newcol;
    
    return (E_DB_OK);

}  /* end psl_ct20s_cons_col */


/*
** Name: psl_ct21s_cons_name  - store constraint name & update constraint list
**
** Description:	    
**          Store the constraint name (if any) into the new constraint, 
**          and add constraint info block to the constraint list.
**          (Does not preserve the order of the constraints as they 
**           appeared in the CREATE TABLE statement).
**      
** Inputs:
**	cons_name	    constraint name, if any
**	cons		    constraint info block for this constraint
**
** Outputs:
**	cons_listp	    ptr to list of constraint info blocks;
**			      will point to list with cons inserted into it
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	none
**
** History:
**
**	01-sep-92 (rblumer)
**	    written
*/
DB_STATUS
psl_ct21s_cons_name(
		    char	  *cons_name,
		    PSS_CONS	  *cons,
		    PSS_CONS	  **cons_listp)
{
    /* save name in PSS_CONS structure, if any 
     */
    if (cons_name != (char *) NULL)
    {
	/* copy constraint name into constraint info block
	** and record that name was specified
	*/
	STmove(cons_name, ' ', DB_CONS_MAXNAME,
	    cons->pss_cons_name.db_constraint_name);
	
	cons->pss_cons_type |= PSS_CONS_NAME_SPEC;
    }
    
    /* attach new constraint to the list
     */
    cons->pss_next_cons = *cons_listp;
    *cons_listp = cons;

    return (E_DB_OK);

}  /* end psl_ct21s_cons_name */


/*
** Name: psl_ct22s_cons_allowed   - make sure constraints are allowed 
** 				    on this statement
**
** Description:	    
**          Constraints aren't allowed in STAR (distributed INGRES),
**          or on temporary tables, procedures, or REGISTER TABLE.
**
**	    They are only allowed in CREATE TABLE and ALTER TABLE.
**          This function checks for these conditions and returns an error
**          if any of them are found.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_distrib	    flag telling whether distributed or not
**	psq_cb
**	  psq_mode	    query mode
**	yyvarsp		    ptr to yyvars
**	    submode	    query mode when psq_mode is PSQ_CREATE_SCHEMA
**
** Outputs:
**	psq_cb
**	  psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	none
**
** History:
**
**	01-sep-92 (rblumer)
**	    written
**	28-dec-92 (rblumer)
**	    added ALTER TABLE as an allowed query mode, as we have changed the
**	    place in the grammar where this function is called
**	05-mar-93 (ralph)
**	    CREATE SCHEMA:
**	    allow constraints when CREATE SCHEMA in psl_ct22s_cons_allowed
*/
DB_STATUS
psl_ct22s_cons_allowed(
		       PSS_SESBLK *sess_cb,
		       PSQ_CB     *psq_cb,
		       PSS_YYVARS *yyvarsp)
{
    i4 err_code, 
            err;
    char    command[PSL_MAX_COMM_STRING];
    i4 length;
	
    /*
    ** constraints are allowed when CREATE TABLE
    ** is being parsed as part of CREATE SCHEMA
    */
    if (psq_cb->psq_mode == PSQ_CREATE_SCHEMA &&
	yyvarsp->submode == PSQ_CREATE)
	return(E_DB_OK);

    /*
    ** When not CREATE SCHEMA
    ** constraints only allowed for CREATE TABLE and ALTER TABLE
    ** in local databases; in all other cases, return error.
    ** 
    ** (i.e. if parsing TEMP table, REGISTER TABLE, or CREATE PROC, 
    **  or if in distributed server (STAR), return error)
    */
    if (   ((psq_cb->psq_mode == PSQ_CREATE) 
	    || (psq_cb->psq_mode == PSQ_ALTERTABLE)
	    || (psq_cb->psq_mode == PSQ_ATBL_ADD_COLUMN))
	&& (~sess_cb->pss_distrib & DB_3_DDB_SESS))
    {
        return (E_DB_OK);
    }
    else
    {
	psl_command_string(psq_cb->psq_mode,
			   sess_cb, command, &length);

	if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	    err = E_PS0488_DDB_CONSTRAINT_NOT_ALLOWED;
	else
	    err = E_PS0475_CONSTRAINT_NOT_ALLOWED;

	_VOID_ psf_error(err, 0L, PSF_USERERR, 
			 &err_code, &psq_cb->psq_error, 1,
			 length, command);
	return (E_DB_ERROR);
    }

}  /* end psl_ct22s_cons_allowed */


/*
** Name: psl_command_string  - return a string representing the current command
**
** Description:	    
**          Based on the query mode, return the first word or two representing 
**          a command--enough to give to the user in an error message.
**      
** Inputs:
**	qmode	    	    query mode
**	sess_cb		    PSS_SESBLK parser session cb
**
** Outputs:
**      length              length of the string being returned
**	command		    string to return the command text in
**                              -should be at least PSL_MAX_COMM_STRING bytes 
** Returns:
**	none
**
** Side effects:
**	none
**    NOTE: if you add a command to this function that is longer than 
**          PSL_MAX_COMM_STRING bytes, you must change PSL_MAX_COMM_STRING.
**
** History:
**
**	28-sep-92 (rblumer)
**	    written
**	30-mar-93 (rblumer)
**	    added SQL CREATE commands for EVENT, GROUP, INDEX, ROLE, RULE,
**	    SCHEMA, and USER, plus Star DISTCREATE and REGISTER INDEX.
**	24-sep-93 (rblumer)
**	    added PSQ_CRESETDBP and PSQ_VAR as additional cases of CREATE PROC.
**	03-nov-93 (andre)
**	    added PSQ_GRANT, PSQ_REVOKE, PSQ_CALARM, PSQ_EXEDBP, PSQ_CALLPROC,
**	    PSQ_EVREGISTER, PSQ_EVDEREG, PSQ_EVRAISE
**	27-Jan-2004 (schka24)
**	    Use utility lookup instead of switch.
**	15-Oct-2010 (kschendel) SIR 124544
**	    Spiff up the MODIFY string with the modify action, e.g.
**	    MODIFY (to table_recovery_disallowed).  Needs sess_cb.
*/
VOID
psl_command_string(
	i4	qmode,
	PSS_SESBLK *sess_cb,
	char	*command,
	i4	*length)
{
    
    if (sess_cb->pss_lang == DB_SQL)
    {
	STcopy(uld_psq_modestr(qmode), command);
	*length = STlength(command);
    }  /* end language == DB_SQL */
    else
    {  /* language is QUEL */
	switch (qmode)
	{
	    case PSQ_RETINTO:
		STcopy(ERx("RETRIEVE INTO"), command);
		*length = sizeof(ERx("RETRIEVE [INTO]")) - 1;
		break;
	    case PSQ_VIEW:
		STcopy(ERx("DEFINE VIEW"), command);
		*length = sizeof(ERx("DEFINE VIEW")) - 1;
		break;
	    case PSQ_CREATE:
		STcopy(ERx("CREATE"), command);
		*length = sizeof(ERx("CREATE")) - 1;
		break;
	    case PSQ_MODIFY:
		STcopy(ERx("MODIFY"), command);
		*length = sizeof(ERx("MODIFY")) - 1;
		break;
	    case PSQ_INDEX:
		STcopy(ERx("INDEX"), command);
		*length = sizeof(ERx("INDEX")) - 1;
		break;
	    default:
		/* this had better not happen */
		command[0] = NULLCHAR;
		*length = 0;
		break;
	}
    }  /* end else language is QUEL */
    if (qmode == PSQ_MODIFY)
	psl_md_action_string(sess_cb, command, length);

}  /* end psl_command_string */


/*
** Name: psl_seckey_attr_name
**
** Description:
**	Returns the name to be used for the default security key
** 	attribute, doing case translation as required. This is a pretty
**	simple function, but is called from a bunch of places
**
** Inputs:
**	sess_cb 	Session CB
**
** Outputs:
**	none
**
** Returns:
**	Pointer to name
**
** History:
**	12-jul-93 (robf)
**	    Created
*/
static char *
psl_seckey_attr_name (
	PSS_SESBLK	*sess_cb
)
{
	return (
	  (*sess_cb->pss_dbxlate & CUI_ID_REG_U) 
		? "_II_SEC_TABKEY" 
		: "_ii_sec_tabkey"
	);
}
