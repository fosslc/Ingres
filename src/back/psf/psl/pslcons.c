/*
**Copyright (c) 2004 Ingres Corporation
**
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <er.h>
#include    <me.h>
#include    <si.h>
#include    <st.h>
#include    <qu.h>
#include    <iicommon.h>
#include    <cui.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psftrmwh.h>
#include    <uld.h>

/*
**
**  Name: PSLCONS.C - Functions for processing constraint info 
**                    during constraint creation
**
**  Description:
**      This file contains functions for processing SQL92-style constraints.
**
**          psl_verify_cons - verify that constraints for a CREATE TABLE 
**                            statement are correct and do not conflict 
**			      with each other.
**
**
**  History:
**      10-sep-92 (rblumer)    
**          created.
**	09-dec-92 (anitap)
**	    Changed the order of #include <qsf.h> for CREATE SCHEMA.
**      20-nov-92 (rblumer)    
**          changed psl_cons_text to handle pre-generated check constraint text,
**          and to allocate all strings from pss_ostream and pass back the text
**	    string instead of using a new text stream/QSF object per constraint;
**	    added psl_compare* functions; added synonym checks to ver_ref_cons;
**	    made changes so most functions can be used by ALTER TABLE, too.
**      20-jan-93 (rblumer)    
**          added parameter to psq_tmulti_out since it's prototype has changed
**	10-feb-93 (teresa)
**	    changed interface to use RDF_READTUPLES instead of RDF_GETINFO.
**	22-feb-93 (rblumer)
**	    added code for find_primary_cons & compare_agst_schema functions
**	    now set up separate schema_name for the constraint;
**	    added specific error messages instead of generic ones;
**	    changed psl_cons_text to handle case with no ref columns.
**	05-mar-93 (ralph)
**	    CREATE SCHEMA: Added text_chain parameter to psl_cons_text()
**	    to use an existing text chain.
**	    Made psl_cons_text() available to external routines.
**	    DELIM_IDENT: Unnormalize object names to make them delimited id's.
**	16-mar-93 (rblumer)
**	    set CONS_KNOWN_NOT_NULLABLE_TUPLE bit in 2nd tuple created for
**	    not null columns;  
**	    Move code for generating ALTER TABLE text from pslschma.c
**	    into psl_gen_alter_text function in this file, and make
**	    psl_cons_text static again. 
**	    Add support for self-referential constraints in CREATE TABLE, by
**	    creating an execute-immediate ALTER TABLE node for the constraint.
**	    Code psl_ver_ref_priv to check that user has references privilege.
**	26-mar-93 (rblumer)
**	    remove ps131 trace point lookup from psl_ver_ref_priv;
**	    change status checks after rdf_call to look for both E_DB_WARN and
**	    E_DB_ERROR & change temp_status checks to return most severe error.
**	    Add check for ref constraint on view in psl_ver_ref_cons.
**	21-apr-93 (rblumer)
**	    add space between column name and IS NOT NULL in psl_cons_text.
**	22-apr-93 (rblumer)
**	    in psl_gen_alter_text, don't add schema name for CREATE SCHEMA.
**	07-may-93 (andre)
**	    got rid of psl_ver_cond_tree().  psl_p_telem() already knows how to
**	    handle prototype trees and with a minor modification will handle
**	    prototype trees for CHECK constraints as well
**	26-may-93 (rblumer)
**	    made the 'constraint type' string be either UNIQUE or PRIMARY KEY,
**	    as appropriate, instead of always UNIQUE (in psl_ver_unique_cons).
**	15-jun-93 (rblumer)
**	    in get_primary_key_cols, initialize key_count variable to 0.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	16-jul-93 (rblumer)
**	    changed PST_EMPTY_REFERENCING_TABLE to PST_EMPTY_TABLE;
**	    in psl_verify_cons, if processing an execute immediate call,
**	    use its base query mode instead of the current one.
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	16-aug-93 (rblumer)
**	    in psl_ver_check_cons,
**	    for implicit CONS_NOT_NULL constraints created during CREATE TABLE
**	    AS SELECT processing, use the previously set-up bitmap, instead of
**	    generating one from the column name.
**	23-sep-93 (rblumer)
**	    in psl_ver_cons_columns and psl_ver_check_cons,
**	    return an error if we find a TID column specified in a constraint.
**	22-sep-93 (rblumer)
**	    change psl_cons_text to NOT delimit columnnames that were specified
**	    using regular identifiers; this way we only delimit columns
**	    that were delimited by the user or added to the query by the
**	    system. More work is needed to do this for other identifiers.
**	    Also use DB_MAX_DELIMID instead of DB_MAXNAME*2 +2 and changed
**	    "unnormal_id" to more mnemonic "delim_id".
**	    Moved common 'column list' code to new psl_add_col_list function
**	    & common 'schema.table' code to new psl_add_schema_table function.
**	    Other miscellaneous cleanup.
**	09-jan-94 (andre)
**	    rewrote functions scanning IIINTEGRITY looking for a tuple 
**	    representing a particular constraint to use a new function - 
**	    psl_find_cons() - which will accept a qualifying function which 
**	    will be responsible for determining whether a given tuple is the 
**	    one we are looking for
**	18-apr-94 (rblumer)
**	    General clean-up from code review:
**	       in several functions, split tblinfo into 2 parameters so that it
**	       can be the correct type(s) instead of a PTR;
**	       allocate array of col_id's instead of allocating them one-by-one
**	       (in get_primary_key_cols);
**	       put print-out function into a trace point instead of xDEBUG;
**	       removed get_primary_cons function after moving its body into 
**	       psl_find_primary_cons, since get_* function has become so simple;
**	       add comments to function headers for psl_compare_* functions.
**	26-apr-94 (rblumer)
**	    fixed integration problem for UNIX; I forgot to remove xDEBUG from
**	    around the prototypes of the print-out functions.
**	17-jan-96 (pchang)
**	    Bumped up the number of columns allowed in a table constraint to
**	    DB_MAXKEYS in psl_ver_cons_columns() (B71935).  Modified error 
**	    E_PS03A9_FOREIGN_COL_EXCEEDED to E_PS03A9_CONSTRAINT_COL_EXCEEDED 
**	    since its use is generic to all table constraint types.
**	24-sep-97 (sarjo01)
**	    Bugs 84044, 85255: added argument to psl_p_telem().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psl_tmulti_out(), psq_1rptqry_out(), psq_tout().
**	24-Jul-2002 (inifa01) 
**	    Bug 106821.  Set CONS_BASEIX bit in psl_ver_unique_cons() flagging 
**	    'index = base table' for constraint.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE for memory pool > 2Gig.
**	22-Mar-2005 (thaju02)
**	    Added psl_find_iobj_cons(). (B114000)
**	18-jan_2007 (dougi)
**	    Added parm to psl_ver_cons_columns() because sometimes the caller
**	    expects DMT_ATT_ENTRYs to be returned and sometimes DMU_ATTR_ENTRYs
**	    and mixing them up causes grief.
**     16-Jun-2009 (kiria01) SIR 121883
**	    Cater for updated interface for psl_p_telem in support of
**	    Scalar Sub-queries
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	28-Jul-2010 (kschendel) SIR 124104
**	    Key count in statement node changed to i2, fix here.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
*/

/* function prototypes for static functions
 */
static DB_STATUS    psl_ver_cons_columns(
					 PSS_SESBLK       *sess_cb,
					 char 	    	  *cons_str,
					 i4		  qmode,
					 PSF_QUEUE	  *colq,
					 DMU_CB	     	  *dmu_cb,
					 PSS_RNGTAB    	  *rngvar,
					 i4	     	  newtbl,
					 bool		  dmtatt,
					 DB_COLUMN_BITMAP *col_map,
					 DMT_ATT_ENTRY    **att_array,
					 PST_COL_ID       **colid_listp,
					 i2		  *numcols,
					 DB_ERROR  	  *err_blk);
static DB_STATUS    psl_ver_unique_cons(
					PSS_SESBLK    *sess_cb, 
					i4           qmode, 
					PSS_CONS      *cons,
					i4           newtbl,
					DMU_CB	      *dmu_cb,
					PSS_RNGTAB    *rngvar,
					PST_CREATE_INTEGRITY *cr_integ,
					DB_ERROR      *err_blk);
static DB_STATUS    psl_ver_ref_cons(
				     PSS_SESBLK       *sess_cb, 
				     i4  	      qmode, 
				     PSS_CONS         *cons,
				     i4	      newtbl,
				     DMU_CB    	      *dmu_cb,
				     PSS_RNGTAB	      *refing_rngvar,
				     PST_STATEMENT    *stmt,
				     DB_ERROR         *err_blk);
static DB_STATUS    psl_ver_check_cons(
				       PSS_SESBLK   *sess_cb,
				       PSQ_CB	    *psq_cb,
				       PSS_CONS     *cons,
				       i4           newtbl,
				       DMU_CB  	    *dmu_cb,
				       PSS_RNGTAB   *rngvar,
				       PST_CREATE_INTEGRITY *cr_integ);
static DB_STATUS    psl_cons_text(
				  PSS_SESBLK      *sess_cb,
				  PSS_CONS	  *cons, 
				  SIZE_TYPE	  *memleft,
				  PSF_MSTREAM	  *mstream,
				  DB_TEXT_STRING  **txtp,
				  PTR	  	  text_chain,
				  DB_ERROR	  *err_blk);
static DB_STATUS    psl_add_col_list(
				     PSF_QUEUE	*colq,
				     PTR	text_chain,
				     DB_ERROR	*err_blk);
static DB_STATUS    psl_add_schema_table(
					 DB_OWN_NAME	*ownname,
					 i4		ownname_is_regid,
					 DB_TAB_NAME	*tabname,
					 i4		tabname_is_regid,
					 PTR		text_chain,
					 DB_ERROR	*err_blk);
static DB_STATUS    psl_ixopts_text(
				PSS_CONS	*cons, 
				PTR		text_chain, 
				DB_ERROR	*err_blk, 
				bool		firstwith);
static DB_STATUS    psl_ver_ref_priv(
				     PSS_SESBLK		*sess_cb,
				     PSS_RNGTAB		*rngvar,
				     DB_COLUMN_BITMAP	*ref_map,
				     PST_CREATE_INTEGRITY *cr_integ,
				     DB_ERROR		*err_blk);
static DB_STATUS    psl_ver_ref_types(
				      PSS_SESBLK     *sess_cb, 
				      i4  	     qmode, 
				      i4             newtbl,
				      i4             num_cols,
				      DMF_ATTR_ENTRY **cons_att_array, 
				      DMT_ATT_ENTRY  **ref_att_array,
				      DB_ERROR       *err_blk);
static DB_STATUS    psl_find_primary_cons(
					  PSS_SESBLK	    *sess_cb,
					  PSS_RNGTAB	    *rngvar,
					  DB_COLUMN_BITMAP  *col_map,
					  DB_CONSTRAINT_ID  *cons_id,
					  u_i4		    *integ_no,
					  DMT_ATT_ENTRY	    **att_array,
					  PST_COL_ID	    **collistp,
					  DB_ERROR	    *err_blk);
static DB_STATUS psl_qual_primary_cons(
				      PTR		*qual_args,
				      DB_INTEGRITY      *integ,
				      i4		*satisfies,
				      PSS_SESBLK	*sess_cb);
static DB_STATUS    psl_find_unique_cons(
					 PSS_SESBLK 	  *sess_cb,
					 DB_COLUMN_BITMAP *col_map,
					 PSS_RNGTAB 	  *rngvar,
					 DB_CONSTRAINT_ID *cons_id,
					 u_i4		  *integ_no,
					 DB_ERROR	  *err_blk);
static DB_STATUS   psl_qual_unique_cons(
					PTR		*qual_args,
					DB_INTEGRITY    *integ,
					i4		*satisfies,
					PSS_SESBLK	*sess_cb);
static DB_STATUS    psl_handle_self_ref(
					PSS_SESBLK	*sess_cb,
					PST_STATEMENT	*stmt,
					PSS_OBJ_NAME	*obj_name,
					PSS_CONS	*cons,
					DB_ERROR	*err_blk);
static DB_STATUS    psl_check_unique_set(
					 DB_INTEGRITY	*integ_tup1,
					 DB_INTEGRITY	*integ_tup2,
					 i4		name1_specified,
					 i4		name2_specified,
					 i4		qmode,
					 PSS_SESBLK	*sess_cb,
					 DB_ERROR	*err_blk);
static DB_STATUS psl_compare_multi_cons(
					PSS_SESBLK	*sess_cb,
					PST_STATEMENT	**stmt_list,
					DMU_CB		*dmu_cb,
					i4		qmode,
					DB_ERROR	*err_blk);
static DB_STATUS psl_qual_dup_cons(
				   PTR		*qual_args,
				   DB_INTEGRITY	*integ,
				   i4		*dummy,
				   PSS_SESBLK	*sess_cb);
static DB_STATUS psl_compare_agst_table(
					PSS_SESBLK	*sess_cb,
					PST_STATEMENT	*stmt,
					PSS_RNGTAB	*rngvar,
					i4		qmode,
					DB_ERROR	*err_blk);
static DB_STATUS psl_compare_agst_schema(
					 PSS_SESBLK	*sess_cb,
					 PST_STATEMENT	*stmt_list,
					 i4		qmode,
					 DB_ERROR	*err_blk);

static bool psl_verify_new_index(
		PSS_SESBLK		*sess_cb,
		PST_CREATE_INTEGRITY	*cinteg,
		PST_CREATE_INTEGRITY	*xinteg);

static bool psl_verify_existing_index(
		PST_CREATE_INTEGRITY	*cinteg,
		i4			keycount,
		i4			*keyarray);

static VOID    psl_print_conslist(PSS_CONS *cons_list, i4  print);
static VOID    psl_print_cons(PSS_CONS *cons);
static VOID    psl_print_colq(PSF_QUEUE *q);


/*
** Name: psl_verify_cons  - verify that constraints for a CREATE TABLE statement
**                          are correct and do not conflict with each other
**
** Description:
** 	    Used for both CREATE TABLE and ALTER TABLE.
** 	    
**          Verify that columns (and tables) used in constraints really
**          exist, and build a PST_CREATE_INTEGRITY statement node for each
**          constraint (this includes a DB_INTEGRITY tuple).  Then check the
**          set of constraints to make sure they don't conflict with each
**          other.
**              If a self-referential REFERENCE constraint is being created
**          during a CREATE TABLE, it is changed from a create-integrity node
**          into an execute-immediate node that does an ALTER TABLE ...
**          ADD <constraint>.  This node must execute _after_ all unique
**          constraints are created (since they must depend on one of these
**          unique constraints), so all execute-immediate nodes are put at the
**          end of the list of statements returned by this function.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_object	      ptr to QEU_CB (which contains DMU_CB)
**	    pss_ostream	      ptr to mem stream for allocating statement nodes
**	psq_cb		    PSF request CB
**	    psq_mode	      query mode 
**	cons_list	    list of constraint info blocks for this table
**      newtbl		    TRUE implies table is being created, 
**                              and table info is in 'dmu_cb';
**               	    FALSE implies table already exists, 
**                              and table info is in 'rngvar'.
**     			(ONLY ONE OF BELOW (dmu_cb/rngvar) SHOULD BE USED!!)
**	dmu_cb		    info block for table if CREATE TABLE, else NULL
**	rngvar	    	    info block for table if ALTER TABLE,  else NULL
**	     
**
** Outputs:
**	stmt_listp	    returned list of CREATE CONSTRAINT statement nodes
**	psq_cb
**	    psq_error 	      filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	10-sep-92 (rblumer)
**	    written
**      20-nov-92 (rblumer)    
**          pass pss_ostream and text string to psl_cons_text so all memory
**	    is allocated from one stream instead of a separate text stream 
**	    for each constraint.
**	    Add support for self-referential constraints in CREATE TABLE, by
**	    creating an execute-immediate ALTER TABLE node for the constraint.
**	05-mar-93 (ralph)
**	    CREATE SCHEMA: Changed call to psl_cons_text()
**	07-may-93 (andre)
**	    having allocated PST_STATEMENT for the constraint about to be
**	    processed, store its address in sess_cb->pss_cur_cons_stmt
**
**	    changed interface to accept (PSQ_CB *) instead of qmode and err_blk
**	    because we need to pass PSQ_CB to psl_p_telem() via
**	    psl_ver_check_cons()
**	16-jul-93 (rblumer)
**	    if processing an execute immediate call,
**	    use its base query mode instead of the current one.
**	18-apr-94 (rblumer)
**	    General clean-up from code review:
**	       split tblinfo into 2 parameters so that it can be the
**	       correct type(s) instead of a PTR; put print-out function into a
**	       trace point instead of xDEBUG; 
**	31-mar-98 (inkdo01)
**	    Added support for constraint index options.
**	20-oct-98 (inkdo01)
**	    Added support of RESTRICT referential actions (SET NULL/CASCADE
**	    were already in place).
*/
DB_STATUS
psl_verify_cons(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		i4		newtbl,
		DMU_CB	   	*dmu_cb,
		PSS_RNGTAB   	*rngvar,
		PSS_CONS   	*cons_list,
		PST_STATEMENT	**stmt_listp)
{
    DB_STATUS		    status;
    i4			    qmode;
    i4		    err_code;
    PSS_CONS		    *curcons;
    PST_STATEMENT	    *newstmt;
    PST_STATEMENT	    *exec_immed_list;
    PST_CREATE_INTEGRITY    *cr_integ;
    DB_INTEGRITY	    *integ_tup;
    DB_ERROR		    *err_blk = &psq_cb->psq_error;
    i4		    val1, val2;

    /* if trace point is set, print out info
     */
    if (ult_check_macro(&sess_cb->pss_trace, PSS_PRINT_PSS_CONS, &val1, &val2))
	psl_print_conslist(cons_list, TRUE);

    *stmt_listp = (PST_STATEMENT *) NULL;
    exec_immed_list = (PST_STATEMENT *) NULL;

    if (cons_list == (PSS_CONS *) NULL)
        return (E_DB_OK);
    
    /* if this is an internal EXECUTE IMMEDIATE statement created on behalf of
    ** another statement (like CREATE SCHEMA or CREATE TABLE),
    ** set the query mode to the base query mode.
    ** This is so that error messages print out the correct command name.
    ** 
    ** Note: this query mode will only be different for ALTER TABLEs done on
    ** the behalf of CREATE TABLE (inside or outside of CREATE SCHEMA);
    ** CREATE TABLEs inside of CREATE SCHEMA have a base-mode of CREATE TABLE.
    */
    if (psq_cb->psq_info != (PST_INFO *) NULL)
	qmode = psq_cb->psq_info->pst_basemode;
    else
	qmode = psq_cb->psq_mode;

    /* process each constraint, 
    ** converting each into a PST_CREATE_INTEGRITY statment
    */
    for (curcons = cons_list; 
	 curcons != (PSS_CONS *) NULL;
	 curcons = curcons->pss_next_cons)
    {
	/* allocate a statement node
	** and initialize the structure
	*/
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			    sizeof(PST_STATEMENT), (PTR *) &newstmt, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* save address of the statement node for the current constraint */
	sess_cb->pss_cur_cons_stmt = newstmt;
	
	MEfill(sizeof(PST_STATEMENT), (u_char) 0, (PTR) newstmt);

	newstmt->pst_mode = qmode;
	newstmt->pst_type = PST_CREATE_INTEGRITY_TYPE;

#ifdef xDEBUG
	newstmt->pst_lineno = sess_cb->pss_lineno;
#endif

	/* allocate an integrity tuple, initialize it,
	** and stick it into the statement node
	*/
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			    sizeof(DB_INTEGRITY), (PTR *) &integ_tup, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	
	MEfill(sizeof(DB_INTEGRITY), (u_char) 0, (PTR) integ_tup);

	cr_integ = &(newstmt->pst_specific.pst_createIntegrity);
	cr_integ->pst_integrityTuple = integ_tup;

	/* fill in specific integrity statement and tuple info
	**
	** (NOTE: dbi_tree, dbi_resvar, and dbi_number are not used 
	**  in constraints, and dbi_tabid, dbi_txtid, and dbi_cons_id
	**  are filled in by QEF)
	*/
	cr_integ->pst_createIntegrityFlags |= PST_CONSTRAINT;
	
	/* store table name and schema name
	** (note that schema name is the same for both constraint and table)
	*/
	if (newtbl)
	{
	    STRUCT_ASSIGN_MACRO(dmu_cb->dmu_table_name, 
				cr_integ->pst_cons_tabname);
	    STRUCT_ASSIGN_MACRO(dmu_cb->dmu_owner, 
				cr_integ->pst_cons_ownname);
	    MEcopy(dmu_cb->dmu_owner.db_own_name,
		   sizeof(DB_SCHEMA_NAME),
		   cr_integ->pst_integritySchemaName.db_schema_name);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(rngvar->pss_tabname,
				cr_integ->pst_cons_tabname);
	    STRUCT_ASSIGN_MACRO(rngvar->pss_ownname,
				cr_integ->pst_cons_ownname);
	    MEcopy(rngvar->pss_ownname.db_own_name,
		   sizeof(DB_SCHEMA_NAME),
		   cr_integ->pst_integritySchemaName.db_schema_name);
	}
    
	if (curcons->pss_cons_type & PSS_CONS_NAME_SPEC)
	{
	    cr_integ->pst_createIntegrityFlags |= PST_CONS_NAME_SPEC;

	    MEcopy((PTR) &curcons->pss_cons_name,
		   sizeof(DB_CONSTRAINT_NAME),
		   (PTR) &integ_tup->dbi_consname);
	}

	integ_tup->dbi_seq = 0L;
	
	/* schema_id (dbi_consschema) will be filled in by QEF 
	** before inserting the tuple into the iiintegrity catalog
	*/
	SET_SCHEMA_ID( integ_tup->dbi_consschema, 0L, 0L );

	switch (curcons->pss_cons_type 
		& (PSS_CONS_UNIQUE | PSS_CONS_REF | PSS_CONS_CHECK))
	{
	case PSS_CONS_UNIQUE:
	    status = psl_ver_unique_cons(sess_cb, qmode, curcons, newtbl,
					 dmu_cb, rngvar, cr_integ, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    break;
	    
	case PSS_CONS_REF:
	    status = psl_ver_ref_cons(sess_cb, qmode, curcons, newtbl,
				      dmu_cb, rngvar, newstmt, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    break;
	    
	case PSS_CONS_CHECK:
	    status = psl_ver_check_cons(sess_cb, psq_cb, curcons, newtbl,
					dmu_cb, rngvar, cr_integ);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    break;
	    
	default:
	    /* should never happen */
	    (VOID) psf_error(E_PS0486_INVALID_CONS_TYPE, 0L, 
			     PSF_INTERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	} /* end switch (type) */
	

	/* if the statement is still a create integrity statement, 
	** attach it to the list, and generate text for the catalogs;
	** otherwise it has been changed to execute-immediate statement
	** (which happens for self-referential constraints in CREATE TABLE),
	** so attach it to the exec-immediate list
	*/
	if (newstmt->pst_type == PST_CREATE_INTEGRITY_TYPE)
	{
	    /* attach this CREATE INTEGRITY statement node to beginning of list
	    ** (order is not important)
	    */
	    newstmt->pst_next = *stmt_listp;
	    newstmt->pst_link = *stmt_listp;
	    *stmt_listp = newstmt;

	    /* build the query text, store it in QSF, 
	    ** and store the returned ID in CREATE_INTEGRITY statement node
	    */
	    status = psl_cons_text(sess_cb, curcons, &sess_cb->pss_memleft, 
				   &sess_cb->pss_ostream, 
				   &cr_integ->pst_integrityQueryText,
				   (PTR)NULL, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    /* Check for index options to be copied, too. */
	    if (curcons->pss_cons_flags & PSS_CONS_INDEX_OPTS)
	    {
		STRUCT_ASSIGN_MACRO(curcons->pss_restab,
				cr_integ->pst_indexres);
		STRUCT_ASSIGN_MACRO(curcons->pss_dmu_chars,
				cr_integ->pst_indexopts);
		cr_integ->pst_createIntegrityFlags |= PST_CONS_WITH_OPT;
	    }
	    if (curcons->pss_cons_flags & PSS_CONS_IXNAME)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_NEWIX;
	    if (curcons->pss_cons_flags & PSS_CONS_BASETAB)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_BASEIX;

	    /* Copy referential actions, too. */
	    if (curcons->pss_cons_flags & PSS_CONS_DEL_CASCADE)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_DEL_CASCADE;
	    if (curcons->pss_cons_flags & PSS_CONS_DEL_SETNULL)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_DEL_SETNULL;
	    if (curcons->pss_cons_flags & PSS_CONS_DEL_RESTRICT)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_DEL_RESTRICT;
	    if (curcons->pss_cons_flags & PSS_CONS_UPD_CASCADE)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_UPD_CASCADE;
	    if (curcons->pss_cons_flags & PSS_CONS_UPD_SETNULL)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_UPD_SETNULL;
	    if (curcons->pss_cons_flags & PSS_CONS_UPD_RESTRICT)
		cr_integ->pst_createIntegrityFlags |= PST_CONS_UPD_RESTRICT;
	}
	else  /* have exec-immed stmt */
	{
	    /* attach this EXECUTE IMMEDIATE statement node to beginning
	    ** of exec_immed list (order is not important within this list).
	    ** This statement list will eventually be added to the END
	    ** of the CREATE INTEGRITY list (so that any UNIQUE constraints
	    ** will already exist when this stmt is executed).
	    */
	    newstmt->pst_next = exec_immed_list;
	    newstmt->pst_link = exec_immed_list;
	    exec_immed_list = newstmt;
	}
	
    } /* end for (curcons = cons_list) */
    

    if ( newtbl || (qmode == PSQ_ATBL_ADD_COLUMN) )
    {
	/* have a CREATE TABLE with possibly more than one constraint,
	** so do specific checks between the different constraints
	** (look for duplicate constraint names and unique constraints
	**  on columns that are not declared NOT NULL)
	*/
	status = psl_compare_multi_cons(sess_cb, stmt_listp, dmu_cb, 
				qmode, err_blk);
    }
    else
    {
	/* have an ALTER TABLE (with only one constraint),
	** so do checks specific to an existing table
	** (check unique constraint against existing constraints on table
	**  and check that all unique columns are NOT NULL)
	*/
	status = psl_compare_agst_table(sess_cb, *stmt_listp,
					rngvar, qmode, err_blk);
    }
    if (DB_FAILURE_MACRO(status))
	return (status);


    /* for both CREATE and ALTER TABLE,
    ** check constraint name against existing constraints in this schema
    ** (constraint names have to be unique within a schema, 
    **  not just within a table)
    */
    status = psl_compare_agst_schema(sess_cb, *stmt_listp, qmode, err_blk);

    if (DB_FAILURE_MACRO(status))
	return (status);

    /* attach any execute immediate statements to the end of the stmt list
     */
    if (exec_immed_list != (PST_STATEMENT *) NULL)
    {
	if (*stmt_listp == (PST_STATEMENT *) NULL)
	{
	    *stmt_listp = exec_immed_list;
	}
	else
	{
	    /* find end of list
	     */
	    for (newstmt = *stmt_listp; 
		 newstmt->pst_next != NULL; 
		 newstmt = newstmt->pst_next)
		;
	    
	    newstmt->pst_next = exec_immed_list;
	    newstmt->pst_link = exec_immed_list;
	}
    } /* end if exec_immed != NULL */

    return (E_DB_OK);
    
}  /* end psl_verify_cons */


/*
** Name: psl_ver_cons_columns  - Check that column names actually exist
**                               and also check for duplicate column names
**
** Description:	    
**          Check that column names actually exist in a table and check that
**          the column queue does not have any duplicate column names.
**          Returns a bitmap of the columns in the queue, plus possibly an
**          array of attribute info.  Also returns a linked list of column
**          id's that are in the same order as the queue of column names
**          passed in.
**
**          (Note that, since colq has a dummy header, this function
**           will be a no-op if called with an empty list.)
**
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	      ptr to mem stream for allocating col_id nodes
**      cons_str	    string telling which type of constraint is being
**                              processed (one of UNIQUE,CHECK,REFERENTIAL);
**                              used in error messages
**	qmode		    query mode  (used for error messages)
**      colq		    column queue to be processed
**      newtbl		    TRUE implies table is being created, 
**                              and table info is in 'dmu_cb';
**               	    FALSE implies table already exists, 
**                              and table info is in 'rngvar'.
**	dmtatt		    TRUE if att_array is of DMT_ATT_ENTRYs, requiring
**			    mapping from DMU_ATTR_ENTRYs, else FALSE
**	dmu_cb		    info about table (if newtbl is TRUE), else NULL
**	rngvar		    info about table (if newtbl is FALSE), else NULL
**	
** Outputs:
**	col_map		    bitmap of columns contained in colq
**	att_array	    array of pointers to column info on columns in colq
**			       if NULL, it is ignored
**			       if non-NULL, filled in with attribute descriptors
**                             (DMT_ATT_ENTRY  if newtbl is TRUE,
**                              DMF_ATTR_ENTRY if newtbl is FALSE)
**      colid_listp	    in-order list of column id's in colq
**				(needs to be in-order for OPC to use)
**	num_cols	    count of constraint columns
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
**	10-sep-92 (rblumer)
**	    written
**	23-sep-93 (rblumer)
**	    return an error if we find a TID column specified in a constraint
**	18-apr-94 (rblumer)
**	    split tblinfo into 2 parameters (dmu_cb & rngvar) so that it can be
**	    correctly typed (instead of being a PTR that is one or the other);
**	    changed malloc'ing of PST_COL_ID nodes to be done all at once in
**	    one block, rather than allocating them one-by-one.
**      31-oct-94 (tutto01)
**          added check to see if max number of columns is exceeded
**	17-jan-96 (pchang)
**	    Bumped up the number of columns allowed in a table constraint to
**	    DB_MAXKEYS (B71935).  Modified error E_PS03A9_FOREIGN_COL_EXCEEDED 
**	    to E_PS03A9_CONSTRAINT_COL_EXCEEDED since its use is generic to all
**	    table constraint types.
**	18-jan-2007 (dougi)
**	    Fix mixed structure mapping caused by calls from psl_ver_ref_cons()
**	    which may pass either DMT_ATT_ENTRY or DMU_ATTR_ENTRY arrays.
**	29-Sept-2009 (troal01)
**		Added geospatial support.
*/
static DB_STATUS
psl_ver_cons_columns(
		     PSS_SESBLK	      *sess_cb,
		     char 	      *cons_str,
		     i4	      qmode,
		     PSF_QUEUE	      *colq,
		     DMU_CB	      *dmu_cb,
		     PSS_RNGTAB	      *rngvar,
		     i4	      newtbl,
		     bool	      dmtatt,
/*returned*/	     DB_COLUMN_BITMAP *col_map,
/*returned*/	     DMT_ATT_ENTRY    **att_array,
/*returned*/	     PST_COL_ID       **colid_listp,
/*returned*/	     i2	      *num_cols,
/*returned*/	     DB_ERROR  	      *err_blk)
{
    PSY_COL 	    *curcol;
    PSY_COL 	    *col2;
    i4	      	    colno, attcolno;
    i4	    err_code;
    PST_COL_ID	    *tail, *colnode;
    DMT_ATT_ENTRY   *att;
    DMF_ATTR_ENTRY  **attrs;
    DB_STATUS	    status;
    char	    command[PSL_MAX_COMM_STRING];
    i4	    length;
    i4		    n;
    char	    *nextname;

    *colid_listp = (PST_COL_ID *) NULL;

    if (  newtbl || (qmode == PSQ_ATBL_ADD_COLUMN)  )
    {
	/* set up attrs pointer for use inside of loop
	 */
	attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;
    }

    /* first let's count the number of columns
    ** and allocate enough col_id's for the whole list
    */
    *num_cols = 0;
    for (curcol =  (PSY_COL *) colq->q_next;
	 curcol != (PSY_COL *) colq; 
	 curcol =  (PSY_COL *) curcol->queue.q_next)
	(*num_cols)++;

    /* Has the number of columns exceeded the max? */
    if (*num_cols > DB_MAXKEYS) {
        psf_error(E_PS03A9_CONSTRAINT_COL_EXCEEDED,0L,PSF_USERERR,
                  &err_code,err_blk,0);
        return (E_DB_ERROR);
       }

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			(*num_cols) * sizeof(PST_COL_ID),
			(PTR *) &colnode, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* point returned list to what will be the first node
     */
    *colid_listp = colnode;


    /* for each column in the constraint,
    ** check that column name actually exists in table,
    ** and also check for duplicate column name in constraint
    ** (since there are dummy headers, this works even for empty lists)
    */
    for (curcol =  (PSY_COL *) colq->q_next, attcolno = 0;
	 curcol != (PSY_COL *) colq; 
	 curcol =  (PSY_COL *) curcol->queue.q_next, attcolno++)
    {
	/* check for duplicate names 
	 */
	for (col2 = (PSY_COL *) curcol->queue.q_next;
	     col2 !=(PSY_COL *) colq; 
	     col2 = (PSY_COL *) col2->queue.q_next)
	{
	    if (!MEcmp((PTR) &curcol->psy_colnm.db_att_name,
		       (PTR) &col2->psy_colnm.db_att_name,
		       sizeof(DB_ATT_NAME)))
	    {
		/* get command string to pass to error routine
		 */
		psl_command_string(qmode, sess_cb, command, &length);

		_VOID_ psf_error(E_PS0481_CONS_DUP_COL, 0L, PSF_USERERR,
				 &err_code, err_blk, 3,
				 length, command,
				 psf_trmwhite(sizeof(DB_ATT_NAME),
					      curcol->psy_colnm.db_att_name),
				 curcol->psy_colnm.db_att_name,
				 STlength(cons_str), cons_str);
		return (E_DB_ERROR);
	    }
	} /* end for (col2 = curcol->queue.q_next) */
	

	/* check that column actually exists
	 */

	colno = -1;

	if (newtbl || (qmode == PSQ_ATBL_ADD_COLUMN) )
	{
	    /* get info from DMU control block of table being created
	     */
	    colno = psl_find_column_number(dmu_cb, &curcol->psy_colnm);

	    if ((att_array != (DMT_ATT_ENTRY **) NULL) && 
						(colno != -1))
	    {
	     if (dmtatt)
	     {
		/* Compute blank stripped length of attribute name */
		n = cui_trmwhite(DB_ATT_MAXNAME, 
			(attrs[colno-1])->attr_name.db_att_name);

		/* Since att_array is (DMT_ATTR_ENTRY **) and attrs is
		** (DMU_ATTR_ENTRY **), allocate a DMT_ATTR_ENTRY and 
		** copy field by field. */
		status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			sizeof(DMT_ATT_ENTRY) + (n + 1), 
			(PTR *) &att_array[attcolno], err_blk);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		MEfill(sizeof(DMT_ATT_ENTRY), (u_char)0, 
					(PTR)att_array[attcolno]);
		nextname = (char *)(&att_array[attcolno]) 
			+ sizeof(DMT_ATT_ENTRY);

		(att_array[attcolno])->att_nmlen = n;
		(att_array[attcolno])->att_nmstr = nextname;
		cui_move(n, (attrs[colno-1])->attr_name.db_att_name, '\0',
				n + 1, nextname);

		(att_array[attcolno])->att_type = (attrs[colno-1])->attr_type;
		(att_array[attcolno])->att_width = (attrs[colno-1])->attr_size;
		(att_array[attcolno])->att_prec = 
					(attrs[colno-1])->attr_precision;
		(att_array[attcolno])->att_collID =
					(attrs[colno-1])->attr_collID;
		(att_array[attcolno])->att_geomtype = (attrs[colno-1])->attr_geomtype;
		(att_array[attcolno])->att_srid = (attrs[colno-1])->attr_srid;
	     }
	     else att_array[attcolno] = (DMT_ATT_ENTRY *)attrs[colno-1];
	    }
	}

	if  ( (!newtbl) && (colno == -1) )
	      
	{
	    /* existing table; get info from RDF */
	    att = pst_coldesc(rngvar,
		curcol->psy_colnm.db_att_name, DB_ATT_MAXNAME);
	    
	    if (att != (DMT_ATT_ENTRY *) NULL)
		colno = att->att_number;	
	    else
		colno = -1;		/* column not found */

	    /* save pointer to this column entry (in order)
	     */
	    if (att_array != (DMT_ATT_ENTRY **) NULL)
		att_array[attcolno] = att;
	    
	}  /* end else existing table */

	if (colno == -1)
	{
	    /* get command string to pass to error routine */
	    psl_command_string(qmode, sess_cb, command, &length);

	    _VOID_ psf_error(E_PS0483_CONS_NO_COL, 0L, PSF_USERERR,
			     &err_code, err_blk, 3,
			     length, command,
			     psf_trmwhite(sizeof(DB_ATT_NAME),
					  curcol->psy_colnm.db_att_name),
			     curcol->psy_colnm.db_att_name,
			     STlength(cons_str), cons_str);
	    return (E_DB_ERROR);
	}

	/* we don't allow TID columns in constraints
	 */
	if (colno == DB_IMTID)
	{
	    (void) psf_error(3496L, 0L, PSF_USERERR, 
			     &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}

	/* set bit in bit map corresponding to this column,
	 */
	BTset(colno, (char *) col_map->db_domset);

	/* add column id to list of PST_COL_ID's
	 */
	colnode->col_id.db_att_id = colno;

	/* add current node to end of list (i.e. to the previous node)
	** except if this is the first node in list.
	*/
	if (attcolno != 0)
	    tail->next = colnode;

	tail = colnode;
	colnode++;

    }  /* end for (curcol = colq.q_next) */

    /* set the last col_id node's pointer to NULL to end the list
     */
    tail->next  = (PST_COL_ID *) NULL;

    return (E_DB_OK);

} /* end psl_ver_cons_columns */



/*
** Name: psl_cons_text  - generate text for the constraint
**
** Description:	    
**          Generates the constraint text that is to be stored in the system
**          catalogs, stores it in QSF, and returns its QSF id.
**
**          NOTE: this stores the text as it is supposed to be displayed in the
**          ANSI INFO_SCHEMA catalogs for UNIQUE and REF constraints;
**          for CHECK constraints, it leaves the CHECK keyword on the text,
**          so that will have to be stripped off for the INFO_SCHEMA catalog.
**      
** Inputs:
**	cons		    constraint info block
**	memleft		    memory left for this PSF session 
**	text_chain	    pointer to text chain
**			    if NULL, a new chain will be used
**			    (i.e. sess_cb->pss_memleft)
** Outputs:
**	mstream		    QSF memory stream containing text
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
**	10-sep-92 (rblumer)
**	    written
**	20-nov-92 (rblumer)
**	    add txtp parameter to pass back DB_TEXT_STRING and call
**	    psq_tmulti_out to fill in txtp using memory from pre-opened stream
**	    (instead of psq_tout which allocates new QSF object for each call);
**          handle pre-generated check constraint text for table constraints.
**	21-jan-93 (ralph)
**	    CREATE SCHEMA: Added text_chain parameter to psl_cons_text()
**	    to use an existing text chain.
**	    Made psl_cons_text() available to external routines.
**	    DELIM_IDENT: Unnormalize object names to make them delimited id's.
**	21-apr-93 (rblumer)
**	    add space between column name and IS NOT NULL in text generation.
**	22-sep-93 (rblumer)
**	    change to only delimit identifiers that were delimited by the user
**	    (or added to the query by the system).  Also use DB_MAX_DELIMID
**	    instead of DB_MAXNAME*2 +2 and changed "unnormal_id" to more
**	    mnemonic "delim_id".
**	    Moved common 'column list' code to new psl_add_col_list function.
**	    Other miscellaneous cleanup.
**	8-apr-98 (inkdo01)
**	    Support for constraint index with clause - text added. This is
**	    contrary to ANSI information schema, but the syntax which 
**	    triggers it is also non-ANSI.
**	7-may-98 (inkdo01)
**	    Finish up text for constraint with options.
*/
static
DB_STATUS
psl_cons_text(
	      PSS_SESBLK  *sess_cb,
	      PSS_CONS    *cons, 
	      SIZE_TYPE	  *memleft,
	      PSF_MSTREAM *mstream,
	      DB_TEXT_STRING **txtp,
	      PTR	  text_chain,
	      DB_ERROR	  *err_blk)
{
    i4	err_code;
    PSY_COL	*col;
    DB_STATUS	status;
    i4		col_nm_len;
    u_char	*col_name;
    u_char	delim_id[DB_MAX_DELIMID];
    PTR		result;		/* returned from psq_tadd, but unused */
    bool	firstwith = TRUE;

    /* open a text chain for this constraint
     */
    if (text_chain == NULL)
    {
	status = psq_topen(&text_chain, memleft, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }

    switch (cons->pss_cons_type 
	    & (PSS_CONS_UNIQUE | PSS_CONS_REF | PSS_CONS_CHECK))
    {
    case PSS_CONS_UNIQUE:
	if (cons->pss_cons_type & PSS_CONS_PRIMARY)
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(" PRIMARY KEY("),
			      sizeof(ERx(" PRIMARY KEY(")) - 1, 
			      &result, err_blk);
	}
	else
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(" UNIQUE("),
			      sizeof(ERx(" UNIQUE(")) - 1, &result, err_blk);
	}
	if (DB_FAILURE_MACRO(status))
	    return (status);
	
	/* add the columns to the text chain
	 */
	status = psl_add_col_list(&cons->pss_cons_colq,
				  text_chain, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* put closing parentheses on chain
	 */
	status = psq_tadd(text_chain, (u_char *) ERx(")"),
			  sizeof(ERx(")")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	if (cons->pss_cons_flags & 
		(PSS_CONS_BASETAB | PSS_CONS_IXNAME | PSS_CONS_INDEX_OPTS))
	{
	    /* Add "with (" followed by the various unique constraint 
	    ** index options. */
	    status = psq_tadd(text_chain, (u_char *) ERx(" WITH ("),
				sizeof(ERx(" WITH (")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    if (cons->pss_cons_flags & (PSS_CONS_BASETAB | PSS_CONS_IXNAME))
	    {
		/* Add "index = ", followed by "<index name>" or
		** "base table structure". */
		status = psq_tadd(text_chain, (u_char *) ERx("INDEX = "),
				sizeof(ERx("INDEX = ")) - 1,
				&result, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		if (cons->pss_cons_flags & PSS_CONS_BASETAB)
		    status = psq_tadd(text_chain, 
		    (u_char *) ERx("BASE TABLE STRUCTURE"),
		    sizeof(ERx("BASE TABLE STRUCTURE")) - 1,
		    &result, err_blk);
		else 
		{
		    /* Build text from index name and add to text string. */
		    col_name = (u_char *)&cons->pss_restab.
						pst_resname.db_tab_name;
		    col_nm_len = psf_trmwhite(DB_TAB_MAXNAME,
			cons->pss_restab.pst_resname.db_tab_name);
		    status = psl_norm_id_2_delim_id(&col_name, &col_nm_len,
				delim_id, err_blk);
		    if (DB_FAILURE_MACRO(status)) return(status);
		    status = psq_tadd(text_chain, col_name, col_nm_len, 
					&result, err_blk);
		}
		firstwith = FALSE;
	    }
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    if (cons->pss_cons_flags & PSS_CONS_INDEX_OPTS)
		status = psl_ixopts_text(cons, text_chain, err_blk, firstwith);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    /* And add the ")". */
	    status = psq_tadd(text_chain, (u_char *) ERx(")"),
				sizeof(ERx(")")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	break;
	    
	
    case PSS_CONS_REF:
	status = psq_tadd(text_chain, (u_char *) ERx(" FOREIGN KEY ("),
			  sizeof(ERx(" FOREIGN KEY (")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/*
	** add the REFERENCING (foreign key) columns to the text chain
	*/
	status = psl_add_col_list(&cons->pss_cons_colq,
				  text_chain, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* put closing parentheses on chain
	 */
	status = psq_tadd(text_chain, (u_char *) ERx(")"),
			  sizeof(ERx(")")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	status = psq_tadd(text_chain, (u_char *) ERx(" REFERENCES "),
			  sizeof(ERx(" REFERENCES ")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* put referenced owner and table name into chain
	 */ 
	status = psl_add_schema_table(
		    &cons->pss_ref_tabname.pss_owner,
		    (cons->pss_ref_tabname.pss_schema_id_type == PSS_ID_REG),
		    &cons->pss_ref_tabname.pss_obj_name,
		    (cons->pss_ref_tabname.pss_obj_id_type == PSS_ID_REG),
		    text_chain, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* if we have user-specified columns, add them to the text;
	** otherwise user implicitly used the primary key,
	** so we can't generate the column names from the ref_colq 
	*/
	if (cons->pss_ref_colq.q_next != &cons->pss_ref_colq)
	{
	    /* put opening parentheses onto text chain
	     */
	    status = psq_tadd(text_chain, (u_char *) ERx("("),
			      sizeof(ERx("(")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    /*
	    ** add the REFERENCED (unique/primary key) columns to the chain
	    */
	    status = psl_add_col_list(&cons->pss_ref_colq,
				      text_chain, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    /* put closing parentheses on chain
	     */
	    status = psq_tadd(text_chain, (u_char *) ERx(")"),
			      sizeof(ERx(")")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}

	if (cons->pss_cons_flags & PSS_CONS_DEL_CASCADE)
	{
	    /* Add referential action syntax (ON DELETE CASCADE). */
	    status = psq_tadd(text_chain, (u_char *) ERx(" ON DELETE CASCADE"),
				sizeof(ERx(" ON DELETE CASCADE")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}

	if (cons->pss_cons_flags & PSS_CONS_DEL_SETNULL)
	{
	    /* Add referential action syntax (ON DELETE SET NULL). */ 
	    status = psq_tadd(text_chain, (u_char *) ERx(" ON DELETE SET NULL"),
				sizeof(ERx(" ON DELETE SET NULL")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}

	if (cons->pss_cons_flags & PSS_CONS_UPD_CASCADE)
	{
	    /* Add referential action syntax (ON UPDATE CASCADE). */ 
	    status = psq_tadd(text_chain, (u_char *) ERx(" ON UPDATE CASCADE"),
				sizeof(ERx(" ON UPDATE CASCADE")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}

	if (cons->pss_cons_flags & PSS_CONS_UPD_SETNULL)
	{
	    /* Add referential action syntax (ON UPDATE SET NULL). */ 
	    status = psq_tadd(text_chain, (u_char *) ERx(" ON UPDATE SET NULL"),
				sizeof(ERx(" ON UPDATE SET NULL")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}

	if (cons->pss_cons_flags & 
		(PSS_CONS_BASETAB | PSS_CONS_IXNAME | PSS_CONS_INDEX_OPTS))
	{
	    /* Add "with (" followed by the various unique constraint 
	    ** index options. */
	    status = psq_tadd(text_chain, (u_char *) ERx(" WITH ("),
				sizeof(ERx(" WITH (")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    if (cons->pss_cons_flags & (PSS_CONS_BASETAB | PSS_CONS_IXNAME))
	    {
		/* Add "index = ", followed by "<index name>" or
		** "base table structure". */
		status = psq_tadd(text_chain, (u_char *) ERx("INDEX = "),
				sizeof(ERx("INDEX = ")) - 1,
				&result, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		if (cons->pss_cons_flags & PSS_CONS_BASETAB)
		   status = psq_tadd(text_chain, 
		    (u_char *) ERx("BASE TABLE STRUCTURE"),
		    sizeof(ERx("BASE TABLE STRUCTURE")) - 1,
		    &result, err_blk);
		else 
		{
		    /* Build text from index name and add to text string. */
		    col_name = (u_char *)&cons->pss_restab.
						pst_resname.db_tab_name;
		    col_nm_len = psf_trmwhite(DB_TAB_MAXNAME,
			cons->pss_restab.pst_resname.db_tab_name);
		    status = psl_norm_id_2_delim_id(&col_name, &col_nm_len,
				delim_id, err_blk);
		    if (DB_FAILURE_MACRO(status)) return(status);
		    status = psq_tadd(text_chain, col_name, col_nm_len, 
					&result, err_blk);
		}
		firstwith = FALSE;
	    }
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    if (cons->pss_cons_flags & PSS_CONS_INDEX_OPTS)
		status = psl_ixopts_text(cons, text_chain, err_blk, firstwith);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    /* And add the ")". */
	    status = psq_tadd(text_chain, (u_char *) ERx(")"),
				sizeof(ERx(")")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}	/* end of index options */
	else if (cons->pss_cons_flags & PSS_CONS_NOINDEX)
	{
	    /* Add "with no index". */
	    status = psq_tadd(text_chain, (u_char *) ERx(" WITH NO INDEX"),
				sizeof(ERx(" WITH NO INDEX")) - 1,
				&result, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}

	break;


    case PSS_CONS_CHECK:
	if (~cons->pss_cons_type & PSS_CONS_NOT_NULL)
	{
	    /* text for check constraints has already been collected
	    ** (so that we can store it as the user typed it in
	    **  instead of as the parser breaks it up
	    **  e.g. as IN or BETWEEN instead of converted into several ORs),
	    ** so just return previously emitted text from info block.
	    */
	    *txtp = cons->pss_check_text;

	    /* de-allocate memory for text chain
	     */
	    status = psq_tclose(text_chain, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    
	    return(E_DB_OK);
	}
	/* ELSE */

	/* do special case for system-generated NOT NULL constraints
	**
	** (these have to show up in SQL92 catalogs as 
	**  "equivalent to a 'CHECK (c is not null)' constraint")
	*/
	status = psq_tadd(text_chain, (u_char *) ERx(" CHECK ("),
			  sizeof(ERx(" CHECK (")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	
	/* put column name into constraint (there can only be one)
	 */
	col = (PSY_COL *) cons->pss_cons_colq.q_next;

	col_name   = (u_char *) col->psy_colnm.db_att_name;
	col_nm_len = psf_trmwhite(DB_ATT_MAXNAME, col->psy_colnm.db_att_name);

	status = psl_norm_id_2_delim_id(&col_name, &col_nm_len, 
					delim_id, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	status = psq_tadd(text_chain, col_name, col_nm_len, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	status = psq_tadd(text_chain, (u_char *) ERx(" IS NOT NULL)"),
			  sizeof(ERx(" IS NOT NULL)")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	break;
	
	
    default:
	/* should never happen */
	(VOID) psf_error(E_PS0486_INVALID_CONS_TYPE, 0L, 
			 PSF_INTERR, &err_code, err_blk, 0);

	return (E_DB_ERROR);
    } /* end switch (type) */

    status = psq_tmulti_out(sess_cb, (PSS_USRRANGE *) NULL,
			    text_chain, mstream, txtp, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* de-allocate memory for text chain
     */
    status = psq_tclose(text_chain, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);
    
    return (E_DB_OK);

} /* end psl_cons_text */


/*
** Name: psl_add_col_list  - generate text for a list of column names,
** 			     delimiting those names if necessary
**
** Description:	    
**          Generates text consisting of a list of comma-separated column
**          names, based on the column names in the queue <colq>.
**      
** Inputs:
**	colq		    pointer to list of columns
**				(in a queue of PSY_COL structures)
**	text_chain	    pointer to text chain, to which 
**				the column names will be added
** Outputs:
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
**	22-sep-93 (rblumer)
**	    written from common code pulled out from psl_cons_text.
*/
static
DB_STATUS
psl_add_col_list(
		 PSF_QUEUE  *colq,
		 PTR	    text_chain,
		 DB_ERROR   *err_blk)
{
    PSY_COL	*col;
    DB_STATUS	status = E_DB_OK;
    i4		col_nm_len;
    u_char	*col_name;
    u_char	delim_id[DB_MAX_DELIMID];
    PTR		result;		/* returned from psq_tadd, but unused */

    /* put first column name into the text chain.
    ** 
    ** If it's not a regular identifier, put delimiters around it.
    ** This will allow us to only delimit identifiers that
    ** either were delimited by the user or were added by the parser.
    */
    col = (PSY_COL *) colq->q_next;

    col_name   = (u_char *) col->psy_colnm.db_att_name;
    col_nm_len = psf_trmwhite(DB_ATT_MAXNAME, col->psy_colnm.db_att_name);

    if (~col->psy_col_flags & PSY_REGID_COLSPEC)
    {
	status = psl_norm_id_2_delim_id(&col_name, &col_nm_len, 
					delim_id, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    status = psq_tadd(text_chain, col_name, col_nm_len, &result, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* add any more column names to the constraint
     */
    for (col = (PSY_COL *) col->queue.q_next; 
	 col !=(PSY_COL *) colq;
	 col = (PSY_COL *) col->queue.q_next)
    {
	status = psq_tadd(text_chain, (u_char *) ERx(", "),
			  sizeof(ERx(", ")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	col_name   = (u_char *) col->psy_colnm.db_att_name;
	col_nm_len = psf_trmwhite(DB_ATT_MAXNAME, col->psy_colnm.db_att_name);
	    
	/* If it's not a regular ident, unnormalize it
	** (i.e. put delimiters around it).
	*/
	if (~col->psy_col_flags & PSY_REGID_COLSPEC)
	{
	    status = psl_norm_id_2_delim_id(&col_name, &col_nm_len, 
					    delim_id, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	status = psq_tadd(text_chain, col_name, col_nm_len, 
			  &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

    }  /* end for (col != colq) */
	
    return (status);

}  /* end psl_add_col_list */


/*
** Name: psl_add_schema_table - generate text for <schemaname>.<tablename>
** 			        delimiting those names if necessary
**
** Description:	    
**          Generates text <schema>.<tablename>, given the schema and table
**          names passed in.  If the schema name is all blanks, it only
**          generates text for the table name.
**      
** Inputs:
**	ownname		    owner name (schema name) for the table
**	ownname_is_regid    TRUE if schema name is a regular identifier
**	tabname		    name of table
**	tabname_is_regid    TRUE if table name is a regular identifier
**	text_chain	    pointer to text chain, to which 
**				the schema/table names will be added
** Outputs:
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
**	22-sep-93 (rblumer)
**	    written from common code pulled out from psl_cons_text
**	    and psl_gen_alter_text.
*/
static
DB_STATUS
psl_add_schema_table(
		    DB_OWN_NAME	*ownname,
		    i4		ownname_is_regid,
		    DB_TAB_NAME	*tabname,
		    i4		tabname_is_regid,
		    PTR		text_chain,
		    DB_ERROR	*err_blk)
{
    i4		name_len;
    u_char	*name;
    u_char	delim_id[DB_MAX_DELIMID];
    DB_STATUS	status = E_DB_OK;
    PTR		result;		/* returned from psq_tadd, but unused */

    /* Insert the schema name
    ** --but only if it is non-blank.
    ** (it will be blank in the case of CREATE SCHEMA, though this
    **  may have to change when we allow multiple schemas per user.)
    */
    if (STskipblank(ownname->db_own_name,
		    sizeof(ownname->db_own_name)) != NULL)
    {
	name     = (u_char *) ownname->db_own_name;
	name_len = psf_trmwhite(DB_OWN_MAXNAME, ownname->db_own_name);

	if (!ownname_is_regid)
	{
	    /* delimit the name
	     */
	    status = psl_norm_id_2_delim_id(&name, &name_len, 
					    delim_id, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
	status = psq_tadd(text_chain, name, name_len, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* Insert separator 
	 */
	status = psq_tadd(text_chain, (u_char *) ERx("."),
			  sizeof(ERx("."))-1, &result, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

    }  /* end if own_name non-blank */
    
    /* Insert the table name,
    ** delimiting it if necessary
    */
    name     = (u_char *) tabname->db_tab_name;
    name_len = psf_trmwhite(DB_TAB_MAXNAME, tabname->db_tab_name);
    
    if (!tabname_is_regid)
    {
	status = psl_norm_id_2_delim_id(&name, &name_len, 
					delim_id, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    status = psq_tadd(text_chain, name, name_len, &result, err_blk);
    
    return (status);
    
}  /* end psl_add_schema_table */


/*
** Name: psl_ixopts_text  - generate text for constraint index with options
**
** Description:	    
**          Adds text for accompanying index with clause options to 
**	    constraint text already built. Simply uses values in the
**	    DMU_CHARACTERISTICS and PST_RESTAB in the PSS_CONS
**	    to trigger appropriate text.
**      
** Inputs:
**	cons		    constraint info block
**	text_chain	    pointer to text chain. Applicable text is
**			    appended using psq_tadd.
**	firstwith	    flag indicating whether a with option has
**			    already been coded (and whether a leading 
**			    comma is needed for the rest)
** Outputs:
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	none
**
** History:
**
**	8-may-1998 (inkdo01)
**	    Written as part of constraint index with option enhancement.
**      8-Oct-2010 (hanal04) Bug 124561
**          Add handling of compression settings for the index.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Work from new dmu characteristics structure.
*/
static
DB_STATUS
psl_ixopts_text(
	      PSS_CONS    *cons, 
	      PTR	  text_chain,
	      DB_ERROR	  *err_blk,
	      bool	  firstwith)
{
    DB_STATUS	status;
    char	tbuf[128];
    PTR		result;		/* returned from psq_tadd, but unused */

    /* Text string has already been started. Simply examine all possible
    ** index options and add appropriate text (comma-separated) for each. */

    if (BTtest(DMU_STRUCTURE, cons->pss_dmu_chars.dmu_indicators))
    {
	char	*structname;

	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	structname = uld_struct_name(cons->pss_dmu_chars.dmu_struct);
	STprintf(tbuf, ERx("STRUCTURE = %s"), structname);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf), 
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "structure = ... " */

    if (BTtest(DMU_DATAFILL, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("FILLFACTOR = %d"),
					cons->pss_dmu_chars.dmu_fillfac);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "fillfactor = n" */

    if (BTtest(DMU_MINPAGES, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("MINPAGES = %d"),
					cons->pss_dmu_chars.dmu_minpgs);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "minpages = n" */

    if (BTtest(DMU_MAXPAGES, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("MAXPAGES = %d"),
					cons->pss_dmu_chars.dmu_maxpgs);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "maxpages = n" */

    if (BTtest(DMU_LEAFFILL, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("LEAFFILL = %d"),
					cons->pss_dmu_chars.dmu_leaff);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "leaffill = n" */

    if (BTtest(DMU_IFILL, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("NONLEAFFILL = %d"),
					cons->pss_dmu_chars.dmu_nonleaff);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "nonleaffill = n" */

    if (BTtest(DMU_ALLOCATION, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("ALLOCATION = %d"),
					cons->pss_dmu_chars.dmu_alloc);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "allocation = n" */

    if (BTtest(DMU_EXTEND, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("EXTEND = %d"),
					cons->pss_dmu_chars.dmu_extend);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "extend = n" */

    if (BTtest(DMU_PAGE_SIZE, cons->pss_dmu_chars.dmu_indicators))
    {
	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	STprintf(tbuf, ERx("PAGE_SIZE = %d"),
					cons->pss_dmu_chars.dmu_page_size);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "page_size = n" */

    if (BTtest(DMU_KCOMPRESSION, cons->pss_dmu_chars.dmu_indicators)
      || BTtest(DMU_DCOMPRESSION, cons->pss_dmu_chars.dmu_indicators))
    {
	char compression[20];	/* Longest is "nokey,nodata" */

	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}

	compression[0] = '\0';
	if (BTtest(DMU_KCOMPRESSION, cons->pss_dmu_chars.dmu_indicators))
	{
	    if (cons->pss_dmu_chars.dmu_kcompress == DMU_COMP_ON)
		STcat(compression, "KEY");
	    else
		STcat(compression, "NOKEY");
	}
	if (BTtest(DMU_DCOMPRESSION, cons->pss_dmu_chars.dmu_indicators))
	{
	    if (compression[0] != '\0')
		STcat(compression, ",");
	    if (cons->pss_dmu_chars.dmu_dcompress == DMU_COMP_OFF)
		STcat(compression, "NODATA");
	    else if (cons->pss_dmu_chars.dmu_dcompress == DMU_COMP_ON)
		STcat(compression, "DATA");
	    else
		STcat(compression, "HIDATA");	/* Should be impossible */
	}

	STprintf(tbuf, ERx("COMPRESSION = (%s)"), compression);
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf),
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;

    }

    /* Stupid ol' location needs doing, too. */
    if (cons->pss_restab.pst_resloc.data_address != 0)	/* "location = (...)" */
    {
        char    *locaddr = cons->pss_restab.pst_resloc.data_address;
	char	*tbufaddr = &tbuf[0];
        i4      locleft = cons->pss_restab.pst_resloc.data_in_size;
        i4      locsize;

	if (!firstwith)		/* prepend with ", ", if necessary */
	{
	    status = psq_tadd(text_chain, (u_char *) ERx(", "),
		sizeof(ERx(", ")) - 1, &result, err_blk);
	    if (DB_FAILURE_MACRO(status)) return(status);
	}
	STprintf(tbuf, ERx("LOCATION = ("));
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf), 
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);

	/* Loop over location string, copying one location at a time. */
        while (locleft)
        {
            for (locsize = (sizeof(DB_LOC_NAME) < locleft) ?
                sizeof(DB_LOC_NAME) : locleft; locsize; locsize--)
             if (locaddr[locsize-1] != ' ') break;
                                /* wee loop to strip trailing blanks */
	    MEcopy((char *)locaddr, locsize, (char *)tbufaddr);
	    tbufaddr = &tbufaddr[locsize+2];
	    tbufaddr[-2] = ',';
	    tbufaddr[-1] = ' ';
            locleft -= sizeof(DB_LOC_NAME);
            locaddr = &locaddr[sizeof(DB_LOC_NAME)];
	}
	tbufaddr[-2] = 0;	/* terminate the list */
	status = psq_tadd(text_chain, (u_char *)tbuf, STlength(tbuf), 
	    &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);

	/* Close off the location list. */
	status = psq_tadd(text_chain, (u_char *)ERx(")"),
	    sizeof(ERx(")")) - 1, &result, err_blk);
	if (DB_FAILURE_MACRO(status)) return(status);
	firstwith = FALSE;
    }	/* end of "location = (...)" */

    return (E_DB_OK);
}

/*
** Name: psl_gen_alter_text  - generate text for an ALTER TABLE statement that
** 			       adds a referential constraint to a table
**
** Description:	    
**          Open a text chain and build up the ALTER TABLE ADD <constraint>
**          statement piece-by-piece, using general PSF memory.
**          Then allocate a contiguous piece of memory from a given memory
**          stream and copy the whole statement text into it.
**          This memory is passed back as a DB_TEXT_STRING.
**
** Inputs:
**	mstream		    memory stream for allocating query text
**	memleft		    memory left in general PSF memory
**      ownname		    schema/owner name of table being altered
**      tabname		    table name of table being altered
**      cons		    constraint info block
**
** Outputs:
**	query_textp	    filled in with pointer to text string holding
**				ALTER TABLE statement
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**	16-mar-93 (rblumer)
**	    written.
**	    Pulled out largely as-is from psl_cs03s_create_table in pslschma.c;
**	    added schema name to output.
**	22-apr-93 (rblumer)
**	    don't add schema name for CREATE SCHEMA, else get errors.
*/
DB_STATUS
psl_gen_alter_text(
		   PSS_SESBLK	   *sess_cb,
		   PSF_MSTREAM	   *mstream,
		   SIZE_TYPE	   *memleft,
		   DB_OWN_NAME	   *ownname,
		   DB_TAB_NAME	   *tabname,
		   PSS_CONS	   *cons,
		   DB_TEXT_STRING  **query_textp,
		   DB_ERROR	   *err_blk)
{
    DB_STATUS	status;
    PTR		text_chain;
    PTR		result;		/* returned from psq_tadd, but unused */
    u_char	delim_id[DB_MAX_DELIMID];
    u_i4	delim_len;
    u_i4	normal_len;
    i4	err_code;

    /* Open a text string for this constraint */
    status = psq_topen(&text_chain, memleft, err_blk);
    if (DB_FAILURE_MACRO(status))
        return (status);

    /* Insert statement prolog 
     */
    status = psq_tadd(text_chain, (u_char *) ERx("ALTER TABLE "),
		      sizeof(ERx("ALTER TABLE "))-1, &result, err_blk);
    if (DB_FAILURE_MACRO(status))
        return (status);

    /* put the schema name and table name into chain
     */ 
    status = psl_add_schema_table(ownname, FALSE, tabname, FALSE,
				  text_chain, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Insert the "ADD" noise 
     */
    status = psq_tadd(text_chain, (u_char *) ERx(" ADD "),
    		  sizeof(ERx(" ADD "))-1, &result, err_blk);
    if (DB_FAILURE_MACRO(status))
        return (status);

    /* If a constraint name was specified, insert it 
     */
    if (cons->pss_cons_type & PSS_CONS_NAME_SPEC)
    {
        status = psq_tadd(text_chain, (u_char *) ERx("CONSTRAINT "),
    		      sizeof(ERx("CONSTRAINT "))-1, &result, err_blk);
        if (DB_FAILURE_MACRO(status))
        	return (status);
        delim_len = sizeof(delim_id);
        normal_len = sizeof(cons->pss_cons_name.db_constraint_name);
        status = cui_idunorm(
    	     (u_char *)&cons->pss_cons_name.db_constraint_name,
    	     &normal_len, delim_id, &delim_len,
    	     (u_i4)(CUI_ID_DLM | CUI_ID_STRIP), err_blk);
        if (DB_FAILURE_MACRO(status))
        {
    	psf_error(err_blk->err_code, 0L, PSF_INTERR, 
    		  &err_code, err_blk, 2, 
    		  sizeof(ERx("Normalized identifier")),
    		  ERx("Normalized identifier"),
    		  normal_len,
    		  &cons->pss_cons_name.db_constraint_name);
    	return(status);
        }
        status = psq_tadd(text_chain,delim_id,delim_len,
    			&result, err_blk);
        if (DB_FAILURE_MACRO(status))
        	return (status);
        status = psq_tadd(text_chain, (u_char *) ERx(" "),
    		      sizeof(ERx(" "))-1, &result, err_blk);
        if (DB_FAILURE_MACRO(status))
        	return (status);
    }

    /* Generate the text of the constraint
     */
    status = psl_cons_text(sess_cb, cons, memleft,
			   mstream, query_textp, text_chain,
			   err_blk);
    if (DB_FAILURE_MACRO(status))
        return (status);

    return (E_DB_OK);

}  /* end psl_gen_alter_text */


/*
** Name: psl_ver_unique_cons  - Verify that columns in UNIQUE constraint are
**                              correct and fill in appropriate pieces of
**                              PST_CREATE_INTEGRITY node
**
** Description:	    
**          Check that UNIQUE column names actually exist in a table
**          and check that the column queue does not have any duplicate
**          column names.  Fills in the bitmap of the columns in the queue,
**          the table name, and the column-id list.
**
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	qmode		    query mode  (used for error messages)
**      cons		    constraint info block
**      newtbl		    TRUE implies table is being created, 
**                              and table info is in 'dmu_cb';
**               	    FALSE implies table already exists, 
**                              and table info is in 'rngvar'.
**	dmu_cb		    info about table (if newtbl is TRUE), else NULL
**	rngvar		    info about table (if newtbl is FALSE), else NULL
**
** Outputs:
**	cr_integ	    statement node; following fields are filled in:
**	    pst_cons_collist	in-order list of column ids
**	    pst_integrityTuple
**		dbi_consflags	UNIQUE bit, possibly PRIMARY_KEY bit
**		dbi_columns	bitmap of unique columns
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
**	10-sep-92 (rblumer)
**	    written
**	26-may-93 (rblumer)
**	    made the 'constraint type' string be either UNIQUE or PRIMARY KEY,
**	    as appropriate, instead of always UNIQUE.
**	07-jan-94 (andre)
**	    we will prevent a user from adding a UNIQUE/PRIMARY KEY constraint 
**	    on an existing table if the database is being journaled but the 
**	    table is not
**	18-apr-94 (rblumer)
**	    split tblinfo into 2 parameters (dmu_cb & rngvar) so that it can be
**	    correctly typed (instead of being a PTR that is one or the other).
**	2-apr-98 (inkdo01)
**	    Changes for constraint index options feature.
**      24-Jul-2002 (inifa01)
**	    Bug 106821.  Set CONS_BASEIX bit in psl_ver_unique_cons() flagging
**	    'index = base table' for constraint.
**	01-Oct-2003 (inifa01)
**	    Made changes to above fix to bug 106821.  Not part of any specific
**	    bug.  Was checking for PSS_CONS_BASEIX bit in 
**	    cons->pss_cons_type, bit is set only for cons->pss_cons_flags. fix
**	    worked b/cos for PRIMARY KEY PSS_CONS_BASEIX == PSS_CONS_PRIMARY
*/
static DB_STATUS
psl_ver_unique_cons(
		    PSS_SESBLK    *sess_cb, 
		    i4            qmode, 
		    PSS_CONS      *cons,
		    i4            newtbl, /* 1 =>CREATE TABLE, 0 =>ALTER TABLE*/
		    DMU_CB        *dmu_cb,
		    PSS_RNGTAB    *rngvar,
		    PST_CREATE_INTEGRITY *cr_integ, /* modified */
		    DB_ERROR      *err_blk)
{
    DB_INTEGRITY    *integ_tup;
    PST_COL_ID	    *collist;
    char	    *cons_str;
    DB_STATUS	    status;
    i4	    err_code;

    /* Before anything else, check that he hasn't opted out of the index. */
    if (cons->pss_cons_flags & PSS_CONS_NOINDEX)
    {	/* this isn't allowed for unique/pkey constraints */
	(void) psf_error(E_PS048E_CONS_NOINDEX, 0L, PSF_USERERR,
			&err_code, err_blk, 0);
	return(E_DB_ERROR);
    }

    /* set bit flagging this as a unique constraint;
    ** then add primary key flag, if that is set in cons info block
    */
    integ_tup = cr_integ->pst_integrityTuple;
    integ_tup->dbi_consflags |= CONS_UNIQUE;

    cons_str = ERx("UNIQUE");		/* set 'constraint type' string */
    
    if (cons->pss_cons_type & PSS_CONS_PRIMARY)
    {
	integ_tup->dbi_consflags |= CONS_PRIMARY;
	cons_str = ERx("PRIMARY KEY");
    }
    
    /* Set CONS_BASEIX bit for 'index = base table structure' with option */ 
    if (cons->pss_cons_flags & PSS_CONS_BASETAB)
    {
        integ_tup->dbi_consflags |= CONS_BASEIX;
    }
    
    /*
    ** if adding a UNIQUE/PRIMARY KEY constraint on an existing table in a 
    ** database which is being journaled, we need to verify that journaling is 
    ** enabled on the table itself - if not, we want to prevent user from 
    ** creating this constraint because if it should become necessary to employ
    ** ROLLFORWARDDB before the next checkpoint is taken, database may be left 
    ** in the state where the data in the table no longer satisfies the 
    ** constraint being created
    **
    ** user may choose to override this restriction by specifying 
    ** WITH NOJOURNAL_CHECK in which case we will send a warning 
    ** message to the error log
    */
    if (!newtbl && (sess_cb->pss_ses_flag & PSS_JOURNALED_DB))
    {
	if (~rngvar->pss_tabdesc->tbl_status_mask & DMT_JNL)
	{
	    if (~cons->pss_cons_flags & PSS_NO_JOURNAL_CHECK)
	    {
                (void) psf_error(E_PS04AC_CONS_NON_JOR_TBL_JOR_DB, 0L,
				 PSF_USERERR, &err_code, err_blk, 3,
				 STlength(cons_str), (PTR) cons_str,
				 psf_trmwhite(sizeof(rngvar->pss_tabname),
					      (char *) &rngvar->pss_tabname),
				 (PTR) &rngvar->pss_tabname,
				 psf_trmwhite(sizeof(rngvar->pss_ownname),
					      (char *) &rngvar->pss_ownname),
				 (PTR) &rngvar->pss_ownname);
                return(E_DB_ERROR);
	    }
	    else
	    {
		/*
		** we want the error to be sent only to the error log - this 
		** requires that we set error type to "internal error", but we 
		** do not want to return "internal error" to the caller, so we 
		** must remember to reset err_blk->err_code
		*/
                (VOID) psf_error(W_PS04B0_CONS_NON_JOR_TBL_JOR_DB, 0L,
                    PSF_INTERR, &err_code, err_blk, 3,
		    STlength(cons_str), (PTR) cons_str,
                    psf_trmwhite(sizeof(rngvar->pss_tabname),
                        (char *) &rngvar->pss_tabname),
                    (PTR) &rngvar->pss_tabname,
                    psf_trmwhite(sizeof(rngvar->pss_ownname),
                        (char *) &rngvar->pss_ownname),
                    (PTR) &rngvar->pss_ownname);

		err_blk->err_code = E_PS0000_OK;
	    }
	}
    }

    /* check that column names actually exist in table,
    ** and also check for duplicate column names
    */
    status = psl_ver_cons_columns(sess_cb, cons_str, qmode,
				  &cons->pss_cons_colq, dmu_cb, rngvar, 
				  newtbl, TRUE, &integ_tup->dbi_columns,
				  (DMT_ATT_ENTRY **) NULL, &collist, 
				  &cr_integ->pst_key_count, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    cr_integ->pst_cons_collist = collist;
    cr_integ->pst_key_collist = collist;
    
    return(E_DB_OK);
    
}  /* end psl_ver_unique_cons */


/*
** Name: psl_ver_ref_cons     - Verify that columns in REFERENTIAL constraint
**                              exist and fill in appropriate pieces of
**                              PST_CREATE_INTEGRITY node
**
** Description:	    
**          Check that referencing column names actually exist in this table,
**          and check that referenced table and columns exist.
**          Check that unique constraint exists on referenced columns.
**          Fills in the bitmap of the referenced columns, the referenced
**          and referencing table names, the referenced and referencing
**          column-id lists, and the id referenced UNIQUE constraint. 
**
**          (NOTE: the referencing columns are the foreign key, and the
**          referenced columns are the primary key/unique key in another table).
**
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	qmode		    query mode  (used for error messages)
**      cons		    constraint info block
**      newtbl		    TRUE implies table is being created, 
**                              and table info is in 'dmu_cb';
**               	    FALSE implies table already exists, 
**                              and table info is in 'rngvar'.
**	dmu_cb		    info about table (if newtbl is TRUE), else NULL
**	rngvar		    info about table (if newtbl is FALSE), else NULL
**
** Outputs:
**	cr_integ	    statement node; following fields are filled in:
**	    pst_cons_collist	    in-order list of referencing column ids
**	    pst_integrityTuple
**		dbi_consflags	    REFERENTIAL bit
**		dbi_columns	    bitmap of unique columns
**	    pst_ref_tabname	    referenced table name
**	    pst_ref_collist	    in-order list of referenced column ids
**	    pst_ref_depend_consid   id of referenced UNIQUE constraint
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
**	10-sep-92 (rblumer)
**	    written
**	16-mar-93 (rblumer)
**	    Add support for self-referential constraints in CREATE TABLE.
**	26-mar-93 (andre)
**	    REFERENCES constraint depends on a UNIQUE/PRIMARY KEY constraint on
**	    referenced table and, if the owner of the referenced table is
**	    different from that of the referencing table, on REFERENCES
**	    privilege on the referenced columns.  Description of object(s) and
**	    privilege(s) on which a constraint depends will be stored in a
**	    PSQ_INDEP_OBJECTS structure pointed to by
**	    PST_CREATE_INTEGRITY.pst_indep
**	01-apr-93 (rblumer)
**	    Add check for ref constraint on view.
**	04-05-93 (andre)
**	    indep_cons->psq_next was left unitialized - this sometimes resulted
**	    in AV in qeu_enter_dependencies() where we would try to traverse
**	    pointer chain leading into the great blue yonder
**	07-apr-93 (rblumer)
**	    change error message in psl_ver_ref_priv to use rngvar table and
**	    owner names, since names in cr_integ haven't been filled in yet.
**	16-jul-93 (rblumer)
**	    when hit an error, print out constraint name, if it was specified.
**	17-aug-93 (andre)
**	    PSS_OBJ_NAME.pss_qualified has been replaced with 
**	    PSS_OBJSPEC_EXPL_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	    PSS_OBJ_NAME.pss_sess_table has been replaced with
**	    PSS_OBJSPEC_SESS_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	07-jan-94 (andre)
**	    ability to define a referential constraint depends on journaling 
**	    status of the database and of the referencing/referenced tables as 
**	    follows:
**		- if the database is being journaled (sess_cb->pss_ses_flag & 
**		  PSS_JOURNALED_DB), both tables must be actively journaled 
**		  (i.e. referenced table must have DMT_JNL bit set and the 
**		  referencing table must either be in the process of being 
**		  created WITH JOURNALING or must exist and have DMT_JNL bit 
**		  set)
**		- if the database is not being journaled, both referencing and 
**		  referenced tables must have journaling disabled or set to be 
**		  enabled at next checkpoint (i.e. if referenced table has 
**		  neither DMT_JNL nor DMT_JON set, then the referencing table 
**		  must either be in the process of being created 
**		  WITH NOJOURNALING or must exist and have neither DMT_JNL nor 
**		  DMT_JON set; otherwise (if referenced table has DMT_JON set),
**		  the referencing table must either be in the process of being 
**		  created WITHJOURNALING or must exist and have DMT_JON set) 
**	11-jan-94 (rblumer)
**	    B58746:
**	    must delimit table/owner names that are used to replace a synonym.
**	01-feb-94 (andre)
**	    cleanup post-integration confusion (mask was no longer defined but
**	    was still used)
**	18-apr-94 (rblumer)
**	    split tblinfo into 2 parameters (dmu_cb & rngvar) so that it can be
**	    correctly typed (instead of being a PTR that is one or the other);
**	    also, count the columns and allocate that many ATTR pointers,
**	    instead of always allocating the max (300).
**	    Fixed bug where we could go off the end of the char_entry array if
**	    there was no entry for the DMU_JOURNAL option.
**	3-apr-98 (inkdo01)
**	    Changes for constraint index with options.
**	14-jun-06 (toumi01)
**	    Allow session temporary tables to be referenced without the
**	    "session." in DML. This eliminates the need for this Ingres-
**	    specific extension, which is a hinderance to app portability.
**	    If psl_rngent returns E_DB_INFO it found a session temporary
**	    table, which is invalid in this context, so ignore such tables
**	    (pslsgram will catch this anyway, or at least should!).
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl_rngent for WITH support.
*/
static DB_STATUS
psl_ver_ref_cons(
		 PSS_SESBLK    *sess_cb, 
		 i4  	       qmode, 
		 PSS_CONS      *cons,
		 i4            newtbl,
		 DMU_CB	       *dmu_cb,
		 PSS_RNGTAB    *refing_rngvar,
		 PST_STATEMENT *stmt,
		 DB_ERROR      *err_blk)
{	
    PSS_OBJ_NAME     *ref_obj_name;
    PSS_RNGTAB	     *rngvar;
    i4		     rngvar_info;
    i4		     num_cols;
    i2		     dummy;
    PST_COL_ID	     *cons_collist;
    PST_COL_ID	     *ref_collist;
    PSY_COL	     *curcol;
    DMF_ATTR_ENTRY   **cons_att_array;
    DMT_ATT_ENTRY    **ref_att_array;
    DB_COLUMN_BITMAP ref_map;
    DB_CONSTRAINT_ID ref_cons_id;
    DB_INTEGRITY     *integ_tup;
    DB_STATUS	     status;
    i4	     err_code;
    char	     command[PSL_MAX_COMM_STRING];
    i4	     length;
    PST_CREATE_INTEGRITY *cr_integ;
    PSQ_INDEP_OBJECTS	*indep_objs;
    PSQ_OBJ		*indep_cons;
				/* 
				** since the referencing table may be in the 
				** process of being created, we will record its
				** journaling status in "refing_jour" to 
				** simplify code determining whether the 
				** constraint may be created
				*/
    i4		refing_jour = 0L;

    /* set bit flagging this as a referential constraint;
     */
    cr_integ = &(stmt->pst_specific.pst_createIntegrity);
    integ_tup = cr_integ->pst_integrityTuple;
    integ_tup->dbi_consflags |= CONS_REF;

    /* count the number of columns
    ** and allocate enough ATTR pointers for all of them
    */
    num_cols = 0;
    for (curcol =  (PSY_COL *) cons->pss_cons_colq.q_next;
	 curcol != (PSY_COL *) &cons->pss_cons_colq;
	 curcol =  (PSY_COL *) curcol->queue.q_next)
	num_cols++;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			num_cols * sizeof(DMF_ATTR_ENTRY *), 
			(PTR *) &cons_att_array, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(num_cols * sizeof(DMF_ATTR_ENTRY *),
	   (u_char) 0, (PTR) cons_att_array);

    /* check that columns actually exist in table,
    ** and check for duplicate column names in the constraint;
    ** also, fill in cons_att_array and cons_collist with column info
    **
    ** (note: if newtbl is FALSE, cons_att_array will be filled with
    **  DMT_ATT_ENTRYs instead of DMF_ATTR_ENTRY.  But that's OK because
    **  the size of (<struct> *) is the same for both)
    */
    status = psl_ver_cons_columns(sess_cb, ERx("REFERENTIAL"), qmode,
				  &cons->pss_cons_colq,
				  dmu_cb, refing_rngvar, newtbl, FALSE,
				  &integ_tup->dbi_columns,
				  (DMT_ATT_ENTRY **) cons_att_array,
				  &cons_collist, &dummy, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    /* now look at the referenced table
     */
    ref_obj_name = &cons->pss_ref_tabname;

    /* make sure user did not specify SESSION as the 'schema';
    ** constraints are not allowed on temporary tables (yet)
    */
    if (ref_obj_name->pss_objspec_flags & PSS_OBJSPEC_SESS_SCHEMA)
    {
	(VOID) psf_error(E_PS0BD2_NOT_SUPP_ON_TEMP, 0L, PSF_USERERR,
			 &err_code, err_blk, 1,
			 sizeof("REFERENTIAL CONSTRAINT")-1,
			 "REFERENTIAL CONSTRAINT");
	return(E_DB_ERROR);
    }

    /* if creating a table,
    ** determine journaling status of the referencing table and decide whether
    ** the constraint may be created (depending on the journaling status of the 
    ** database + 
    ** determine whether this is a self-referential constraint.
    ** 
    ** If this is a self-referencing constraint, we handle it differently:
    **   We generate an execute-immediate node containing an ALTER TABLE
    ** ADD <constraint> statement.  This constraint will then be re-executed
    ** after the table and all its unique constraints have been created and
    ** added to the appropriate catalogs.
    ** This allows us to use the same set of code to verify self-referential
    ** constraint and vanilla referential constraints, rather than having a
    ** significant amount of special-case code for self-referential.
    */
    if (newtbl)
    {
    	bool		     	self_ref;

	/*
	** determine journaling status of the referencing table; if the 
	** database is being journaled, we will prevent user from creating a 
	** REF constraint if the table is being created WITH NOJOURNALING;
	** if this is not a self-referencing constraint, later on we will
	** condider the journaling status of the referencing and referenced 
	** tables in the context of journaling status of the database to 
	** determine whether the constraint may be created
	*/

	if (BTtest(DMU_JOURNALED, dmu_cb->dmu_chars.dmu_indicators)
	  && dmu_cb->dmu_chars.dmu_journaled != DMU_JOURNAL_OFF)
	{
	    /* 
	    ** if the database is being journaled, this table will be created 
	    ** as journaled (TCB_JOURNAL set in relstat); otherwise it will be
	    ** created "set journaled" (i.e. TCB_JON bit set in relstat).
	    */
	    refing_jour = (sess_cb->pss_ses_flag & PSS_JOURNALED_DB)
		? DMT_JNL : DMT_JON;
	}
        else if (sess_cb->pss_ses_flag & PSS_JOURNALED_DB)
        {
            /* 
            ** user attempted to create a REF constraint on a non-journaled 
	    ** table in a journaled database - this is illegal; 
	    **
	    ** user may choose to override this restriction by specifying 
	    ** WITH NOJOURNAL_CHECK in which case we will send a warning 
	    ** message to the error log
            */
	    if (~cons->pss_cons_flags & PSS_NO_JOURNAL_CHECK)
	    {
	        (void) psf_error(E_PS04A9_NON_JOUR_REFING_JOUR_DB, 0L, 
				 PSF_USERERR, &err_code, err_blk, 2,
				 psf_trmwhite(sizeof(dmu_cb->dmu_table_name), 
					      (char *) &dmu_cb->dmu_table_name),
				 (PTR) &dmu_cb->dmu_table_name,
				 psf_trmwhite(sizeof(dmu_cb->dmu_owner),
					      (char *) &dmu_cb->dmu_owner),
				 (PTR) &dmu_cb->dmu_owner);
                return(E_DB_ERROR);
	    }
	    else
	    {
		/*
		** we want the error to be sent only to the error log - this 
		** requires that we set error type to "internal error", but we 
		** do not want to return "internal error" to the caller, so we 
		** must remember to reset err_blk->err_code
		*/
	        (VOID) psf_error(W_PS04AD_NON_JOUR_REFING_JOUR_DB, 0L, 
				 PSF_INTERR, &err_code, err_blk, 2,
				 psf_trmwhite(sizeof(dmu_cb->dmu_table_name), 
					      (char *) &dmu_cb->dmu_table_name),
				 (PTR) &dmu_cb->dmu_table_name,
				 psf_trmwhite(sizeof(dmu_cb->dmu_owner),
					      (char *) &dmu_cb->dmu_owner),
				 (PTR) &dmu_cb->dmu_owner);

		err_blk->err_code = E_PS0000_OK;
	    }
        }

	self_ref = FALSE;
	
	if (ref_obj_name->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA)
	{
	    if ((MEcmp(ref_obj_name->pss_obj_name.db_tab_name,
		       dmu_cb->dmu_table_name.db_tab_name, DB_TAB_MAXNAME) == 0)
		&& (MEcmp(ref_obj_name->pss_owner.db_own_name,
			  dmu_cb->dmu_owner.db_own_name, DB_OWN_MAXNAME) == 0))
	    {
		self_ref = TRUE;
	    }
	}
	else if (MEcmp(ref_obj_name->pss_obj_name.db_tab_name,
		       dmu_cb->dmu_table_name.db_tab_name, DB_TAB_MAXNAME) == 0)
	{
	    self_ref = TRUE;
	    MEcopy((PTR) &dmu_cb->dmu_owner, 
		   sizeof(DB_OWN_NAME), 
		   (PTR) &ref_obj_name->pss_owner);
	}
	if (self_ref)
	{
	    status = psl_handle_self_ref(sess_cb, stmt, 
					 ref_obj_name, cons, err_blk);
	    return(status);
	}
    }  /* end if (newtbl) */
    else
    {
	refing_jour = refing_rngvar->pss_tabdesc->tbl_status_mask & 
			  (DMT_JNL | DMT_JON);

	if (   (~refing_jour & DMT_JNL)
	    && (sess_cb->pss_ses_flag & PSS_JOURNALED_DB))
        {
            /* 
            ** user attempted to create a REF constraint on a non-journaled 
	    ** table in a journaled database - this is illegal
	    **
	    ** user may choose to override this restriction by specifying 
	    ** WITH NOJOURNAL_CHECK in which case we will send a warning 
	    ** message to the error log
            */
	    if (~cons->pss_cons_flags & PSS_NO_JOURNAL_CHECK)
	    {
	        (void) psf_error(E_PS04A9_NON_JOUR_REFING_JOUR_DB, 0L, 
			       PSF_USERERR, &err_code, err_blk, 2,
			       psf_trmwhite(sizeof(refing_rngvar->pss_tabname),
				      (char *) &refing_rngvar->pss_tabname),
			       (PTR) &refing_rngvar->pss_tabname,
			       psf_trmwhite(sizeof(refing_rngvar->pss_ownname),
				      (char *) &refing_rngvar->pss_ownname),
			       (PTR) &refing_rngvar->pss_ownname);
                return(E_DB_ERROR);
	    }
	    else
	    {
		/*
		** we want the error to be sent only to the error log - this 
		** requires that we set error type to "internal error", but we 
		** do not want to return "internal error" to the caller, so we 
		** must remember to reset err_blk->err_code
		*/
	        (VOID) psf_error(W_PS04AD_NON_JOUR_REFING_JOUR_DB, 0L, 
	            PSF_INTERR, &err_code, err_blk, 2,
	            psf_trmwhite(sizeof(refing_rngvar->pss_tabname), 
		        (char *) &refing_rngvar->pss_tabname),
	            (PTR) &refing_rngvar->pss_tabname,
	            psf_trmwhite(sizeof(refing_rngvar->pss_ownname),
		        (char *) &refing_rngvar->pss_ownname),
	            (PTR) &refing_rngvar->pss_ownname);

		err_blk->err_code = E_PS0000_OK;
	    }
        }
    }

    /* get info on referenced table and put it into rngvar.
    ** 
    ** the call to psl_[o]rngent will fix the table info in RDF
    ** and store it in the pss_rsrng part of the range table; 
    ** it will be unfixed automatically when another referential constraint
    ** is processed OR during normal cleanup when the query is finished
    */
    if (ref_obj_name->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA)
    {
	status = psl_orngent(&sess_cb->pss_auxrng, -1, "", 
			     &ref_obj_name->pss_owner,
			     &ref_obj_name->pss_obj_name, sess_cb, FALSE, 
			     &rngvar, qmode, err_blk, &rngvar_info);
    }
    else
    {
	status = psl_rngent(&sess_cb->pss_auxrng, -1, "", 
			    &ref_obj_name->pss_obj_name, sess_cb, FALSE, 
			    &rngvar, qmode, err_blk, &rngvar_info, NULL);
	if (status == E_DB_INFO)	/* oops, we found a session.table */
	{
	    rngvar = (PSS_RNGTAB *) NULL;
	    status = E_DB_ERROR;	/* just in case pretend we didn't */
	}

	/* if table was found, copy owner_name (used later in psl_cons_text)
	 */
	if (rngvar != (PSS_RNGTAB *) NULL)
	    MEcopy((PTR) &rngvar->pss_ownname, 
		   sizeof(DB_OWN_NAME), (PTR) &ref_obj_name->pss_owner);
    }
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* can't have a referential constraint referencing a view
     */
    if (rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
    {
	/*
	** let user know if name supplied by the user was resolved to a
	** synonym
	*/
	if (rngvar_info & PSS_BY_SYNONYM)
	{
	    psl_syn_info_msg(sess_cb, rngvar, ref_obj_name, rngvar_info,
			     sizeof(ERx("ALTER TABLE"))-1,
			     newtbl ? ERx("CREATE TABLE") : ERx("ALTER TABLE"),
			     err_blk);
	}				      

	(VOID) psf_error(E_PS04A2_NO_REF_VIEW, 0L, PSF_USERERR,
			 &err_code, err_blk, 1,
			 psf_trmwhite(sizeof(DB_TAB_NAME), 
				      (char *) &rngvar->pss_tabname),
			 (char *) &rngvar->pss_tabname);
	return (E_DB_ERROR);
    }

    /*
    ** determine whether, based on journaling status of the referencing and 
    ** referenced tables and of the database itself, we should prevent user 
    ** from creating this constraint
    */

    if (sess_cb->pss_ses_flag & PSS_JOURNALED_DB)
    {
        /* 
        ** if the database if being journaled, we have already validated the 
        ** journaling status of the referencing table - now it's time to check 
        ** the journaling status of the referenced table
        */
	if (~rngvar->pss_tabdesc->tbl_status_mask & DMT_JNL)
        {
	    DB_TAB_NAME		*refing_name;
	    DB_OWN_NAME		*refing_schema;

            /* 
            ** user attempted to create a REF constraint referencing a 
	    ** non-journaled table in a journaled database - this is illegal
	    **
	    ** user may choose to override this restriction by specifying 
	    ** WITH NOJOURNAL_CHECK in which case we will send a warning 
	    ** message to the error log
            */

	    if (newtbl)
	    {
		refing_name = &dmu_cb->dmu_table_name;
		refing_schema = &dmu_cb->dmu_owner;
	    }
	    else
	    {
		refing_name = &refing_rngvar->pss_tabname;
		refing_schema = &refing_rngvar->pss_ownname;
	    }

	    if (~cons->pss_cons_flags & PSS_NO_JOURNAL_CHECK)
	    {
	        (void) psf_error(E_PS04AA_NON_JOUR_REFD_JOUR_DB, 0L, 
				 PSF_USERERR, &err_code, err_blk, 4,
				 psf_trmwhite(sizeof(*refing_name), 
					      (char *) refing_name),
				 (PTR) refing_name,
				 psf_trmwhite(sizeof(*refing_schema), 
					      (char *) refing_schema),
				 (PTR) refing_schema,
				 psf_trmwhite(sizeof(rngvar->pss_tabname), 
					      (char *) &rngvar->pss_tabname),
				 (PTR) &rngvar->pss_tabname,
				 psf_trmwhite(sizeof(rngvar->pss_ownname),
					      (char *) &rngvar->pss_ownname),
				 (PTR) &rngvar->pss_ownname);
                return(E_DB_ERROR);
	    }
	    else
	    {
		/*
		** we want the error to be sent only to the error log - this 
		** requires that we set error type to "internal error", but we 
		** do not want to return "internal error" to the caller, so we 
		** must remember to reset err_blk->err_code
		*/
	        (VOID) psf_error(W_PS04AE_NON_JOUR_REFD_JOUR_DB, 0L, 
	            PSF_INTERR, &err_code, err_blk, 4,
		    psf_trmwhite(sizeof(*refing_name), (char *) refing_name),
		    (PTR) refing_name,
		    psf_trmwhite(sizeof(*refing_schema), 
			(char *) refing_schema),
		    (PTR) refing_schema,
	            psf_trmwhite(sizeof(rngvar->pss_tabname), 
		        (char *) &rngvar->pss_tabname),
	            (PTR) &rngvar->pss_tabname,
	            psf_trmwhite(sizeof(rngvar->pss_ownname),
		        (char *) &rngvar->pss_ownname),
	            (PTR) &rngvar->pss_ownname);

		err_blk->err_code = E_PS0000_OK;
	    }
        }
    }
    else
    {
	/*
	** if the database is not being journaled, we must verify that both the
	** referencing and the referenced table have the same journaling status
	** (i.e. both have journaling disabled (DMT_JON not set in the relstat 
	** of either) or both may have it enabled during the next checkpoint
	** (DMT_JON set in the relstat of both))
	**
	** user may choose to override this restriction by specifying 
	** WITH NOJOURNAL_CHECK in which case we will send a warning 
	** message to the error log
	*/
	if (  (refing_jour & DMT_JON)
	    ^ (rngvar->pss_tabdesc->tbl_status_mask & DMT_JON))
	{
	    DB_TAB_NAME		*refing_name;
	    DB_OWN_NAME		*refing_schema;

	    if (newtbl)
	    {
		refing_name = &dmu_cb->dmu_table_name;
		refing_schema = &dmu_cb->dmu_owner;
	    }
	    else
	    {
		refing_name = &refing_rngvar->pss_tabname;
		refing_schema = &refing_rngvar->pss_ownname;
	    }

	    if (~cons->pss_cons_flags & PSS_NO_JOURNAL_CHECK)
	    {
	        (VOID) psf_error(E_PS04AB_REF_CONS_JOUR_MISMATCH, 0L, 
				 PSF_USERERR, &err_code, err_blk, 4,
				 psf_trmwhite(sizeof(*refing_name), 
					      (char *) refing_name),
				 (PTR) refing_name,
				 psf_trmwhite(sizeof(*refing_schema), 
					      (char *) refing_schema),
				 (PTR) refing_schema,
				 psf_trmwhite(sizeof(rngvar->pss_tabname), 
					      (char *) &rngvar->pss_tabname),
				 (PTR) &rngvar->pss_tabname,
				 psf_trmwhite(sizeof(rngvar->pss_ownname),
					      (char *) &rngvar->pss_ownname),
				 (PTR) &rngvar->pss_ownname);
                return(E_DB_ERROR);
	    }
	    else
	    {
		/*
		** we want the error to be sent only to the error log - this 
		** requires that we set error type to "internal error", but we 
		** do not want to return "internal error" to the caller, so we 
		** must remember to reset err_blk->err_code
		*/
	        (VOID) psf_error(W_PS04AF_REF_CONS_JOUR_MISMATCH, 0L, 
	            PSF_INTERR, &err_code, err_blk, 4,
		    psf_trmwhite(sizeof(*refing_name), (char *) refing_name),
		    (PTR) refing_name,
		    psf_trmwhite(sizeof(*refing_schema), 
			(char *) refing_schema),
		    (PTR) refing_schema,
	            psf_trmwhite(sizeof(rngvar->pss_tabname), 
		        (char *) &rngvar->pss_tabname),
	            (PTR) &rngvar->pss_tabname,
	            psf_trmwhite(sizeof(rngvar->pss_ownname),
		        (char *) &rngvar->pss_ownname),
	            (PTR) &rngvar->pss_ownname);

		err_blk->err_code = E_PS0000_OK;
	    }
	}
    }

    /* if name was a synonym, copy actual table and owner names,
    ** as they will be put into the constraint text later
    */
    if (rngvar_info & PSS_BY_SYNONYM)
    {
	MEcopy((PTR) &rngvar->pss_tabname, 
	       sizeof(DB_TAB_NAME), (PTR) &ref_obj_name->pss_obj_name);
	MEcopy((PTR) &rngvar->pss_ownname, 
	       sizeof(DB_OWN_NAME), (PTR) &ref_obj_name->pss_owner);

	/* don't know if above names are valid regular identifiers or not,
	** so have to always delimit them (this is used by cons text generator)
	*/
	ref_obj_name->pss_schema_id_type = PSS_ID_DELIM;
	ref_obj_name->pss_obj_id_type    = PSS_ID_DELIM;
    }

    /* initialize indep_objs and indep_cons for calls below..
    ** 
    ** we need to record id of the UNIQUE/PRIMARY KEY constraint and, if the
    ** owner of the <referenced table> is different from that of the
    ** <referencing table>, the (REFERENCES) privilege on <referenced columns>
    ** on which this REFERENCES constraint will depend
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PSQ_INDEP_OBJECTS),
	(PTR *) &indep_objs, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(sizeof(PSQ_INDEP_OBJECTS), (u_char) 0, (PTR) indep_objs);

    /*
    ** attach the structure describing independent objects/privileges to
    ** PST_CREATE_INTEGRITY statement
    */
    cr_integ->pst_indep = (PTR) indep_objs;
    
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PSQ_OBJ),
	(PTR *) &indep_cons, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    indep_objs->psq_objs = indep_cons;

    indep_cons->psq_next = (PSQ_OBJ *) NULL;
    indep_cons->psq_objtype = PSQ_OBJTYPE_IS_CONSTRAINT;
    indep_cons->psq_num_ids = 1;    /* will depend on exactly one constraint */

    /*
    ** store id of the <referenced table>, psl_find_primary_cons() or
    ** psl_find_unique_cons() will fill in the integrity number
    */
    indep_cons->psq_objid->db_tab_base  = rngvar->pss_tabid.db_tab_base;
    indep_cons->psq_objid->db_tab_index = rngvar->pss_tabid.db_tab_index;

    MEfill(sizeof(indep_cons->psq_dbp_name), (u_char) ' ',
	(PTR) &indep_cons->psq_dbp_name);

    /* count the number of columns in refq,
    ** then allocate enough ATTR pointers for all of them
    */
    num_cols = 0;
    for (curcol =  (PSY_COL *) cons->pss_ref_colq.q_next;
	 curcol != (PSY_COL *) &cons->pss_ref_colq;
	 curcol =  (PSY_COL *) curcol->queue.q_next)
	num_cols++;

    /* ref_colq can be empty if the user did not specify any columns;
    ** in that case, we will look up the primary key automatically,
    ** so allocate enough attributes for the max allowable columns in
    ** a primary key (which is the same as max keys/columns in an index
    ** since primary key & unique constraints are implemented via indexes).
    */
    if (num_cols == 0)
	num_cols = DB_MAXKEYS;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			num_cols * sizeof(DMT_ATT_ENTRY *), 
			(PTR *) &ref_att_array, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
	
    MEfill(num_cols * sizeof(DMT_ATT_ENTRY *),
	   (u_char) 0, (PTR) ref_att_array);

    /* initialize map of referenced columns
     */
    MEfill(sizeof(DB_COLUMN_BITMAP), (u_char) 0, (PTR) &ref_map);


    /* if referenced columns not specified, find primary key;
    ** otherwise, verify that referenced columns exist and form a unique key
    */
    if (cons->pss_ref_colq.q_next == &cons->pss_ref_colq)
    {
	status = psl_find_primary_cons(sess_cb, rngvar,
				       &ref_map, &ref_cons_id,
				       &indep_cons->psq_2id,
				       ref_att_array, &ref_collist,
				       err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }
    else  /* else user supplied list of referenced columns */
    {
	/* check that columns actually exist in referenced table,
	** and check for duplicate column names in the constraint;
	** also, fill in att_array with description of referenced columns
	*/
	status = psl_ver_cons_columns(sess_cb, ERx("REFERENTIAL"), qmode,
				      &cons->pss_ref_colq, 
				      dmu_cb, rngvar, FALSE, TRUE,
				      &ref_map, ref_att_array, 
				      &ref_collist, &dummy, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* check that a unique constraint exists on referenced columns
	 */
	status = psl_find_unique_cons(sess_cb, &ref_map, 
				      rngvar,  &ref_cons_id,
				      &indep_cons->psq_2id, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

    }  /* end else user supplied list of referenced columns */
	
    /* check that number of columns match in cons_cols and ref_cols
     */
    num_cols = BTcount((char *)&integ_tup->dbi_columns, DB_COL_BITS);

    if (BTcount((char *) &ref_map, DB_COL_BITS) != num_cols)
    {
	char *tabname = cr_integ->pst_cons_tabname.db_tab_name;

	psl_command_string(qmode, sess_cb, command, &length);

	(void) psf_error(E_PS0484_REF_NUM_COL, 0L, PSF_USERERR, 
			 &err_code, err_blk, 2,
			 length, command,
			 psf_trmwhite(DB_TAB_MAXNAME, tabname), tabname);

	/* print out constraint name, if user specified one
	 */
	if (cr_integ->pst_createIntegrityFlags & PST_CONS_NAME_SPEC)
	{
	    (void) psf_error(E_PS0494_CONS_NAME, 0L, PSF_USERERR, 
			     &err_code, err_blk, 1,
			     psf_trmwhite(DB_CONS_MAXNAME,
				    integ_tup->dbi_consname.db_constraint_name),
			     integ_tup->dbi_consname.db_constraint_name);
	}
	return (E_DB_ERROR);
    }
    
    /* verify that user has REFERENCES privilege on the columns
     */
    status = psl_ver_ref_priv(sess_cb, rngvar, 
			      &ref_map, cr_integ, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    /* check that types of columns match in cons_cols and ref_cols
     */
    status = psl_ver_ref_types(sess_cb, qmode, newtbl, num_cols, 
			       cons_att_array, ref_att_array, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* finish building CREATE_INTEGRITY tuple
     */
    cr_integ->pst_cons_collist = cons_collist;
    cr_integ->pst_ref_collist  = ref_collist;
    cr_integ->pst_key_collist = cons_collist;
    cr_integ->pst_key_count = num_cols;
    if (cons->pss_cons_flags & PSS_CONS_NOINDEX)
	cr_integ->pst_createIntegrityFlags |= PST_CONS_NOIX;

    STRUCT_ASSIGN_MACRO(ref_cons_id,         cr_integ->pst_ref_depend_consid);	
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabname, cr_integ->pst_ref_tabname);
    STRUCT_ASSIGN_MACRO(rngvar->pss_ownname, cr_integ->pst_ref_ownname);
    
    return (E_DB_OK);

}   /* end psl_ver_ref_cons */	



/*
** Name: psl_ver_ref_types  - verifies that the types of the corresponding
**                            columns in a referential constraint are comparable
**
** Description:	    
**          Check that types of corresponding columns in a referential
**          constraint match each other.
**          To do this, we build a dummy EQUAL node with children that have
**          the types of the corresponding columns, then call pst_node to do
**          the type resolution.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB
**	    pss_ostream	      ptr to mem stream for allocating memory
**	num_cols	    number of columns in the constraint
**	cons_att_array	    array of pointers to attribute info for foreign key
**	ref_att_array	    array of pointers to attribute info for unique key
**			    
** Outputs:
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
**	10-sep-92 (rblumer)
**	    written
**	19-Nov-96 (nanpr01)
**	    Alter table bug : 79064. Referential constraint does not
**	    work.
*/
static DB_STATUS
psl_ver_ref_types(
		  PSS_SESBLK     *sess_cb, 
		  i4		 qmode,
		  i4             newtbl,
		  i4             num_cols,
		  DMF_ATTR_ENTRY **cons_att_array, 
		  DMT_ATT_ENTRY  **ref_att_array,
		  DB_ERROR       *err_blk)
{
    DMF_ATTR_ENTRY *cur_col;        /* used if newtbl == FALSE */
    DMT_ATT_ENTRY  *cur_col_dmt;    /* used if newtbl == TRUE  */
    DMT_ATT_ENTRY  *cur_ref_col;
    PST_VAR_NODE   varnode;
    PST_OP_NODE    opnode;
    PST_QNODE      *node1, *node2, *eqnode;
    DB_STATUS	   status;
    i4	   err_code;
    i4		   colno;

	/* to check that each pair of column types are comparable 
	** (in the same way that 2 join columns are comparable),
	** we will build a dummy EQUAL node with dummy child nodes
	** containing the types of the two columns, and then
	** see if that gets an error from pst_resolve (called by pst_node)
	*/

    /* to save space, allocate the three nodes first using PSS_NORES
    ** (rather than allocating new nodes for each iteration of the loop)
    ** and then use PSS_NOALLOC within the loop.
    */
    status = pst_node(sess_cb, &sess_cb->pss_ostream, 
		      (PST_QNODE *) NULL, (PST_QNODE *) NULL, 
		      PST_VAR, (char *) &varnode, sizeof(PST_VAR_NODE), 
		      DB_NODT, (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL, 
		      &node1, err_blk, PSS_NORES);
    if (DB_FAILURE_MACRO(status))
	return (status);

    status = pst_node(sess_cb, &sess_cb->pss_ostream, 
		      (PST_QNODE *) NULL, (PST_QNODE *) NULL, 
		      PST_VAR, (char *) &varnode, sizeof(PST_VAR_NODE), 
		      DB_NODT, (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL, 
		      &node2, err_blk, PSS_NORES);
    if (DB_FAILURE_MACRO(status))
	return (status);
    
    opnode.pst_opno = (ADI_OP_ID) ADI_EQ_OP;
    opnode.pst_opmeta = PST_NOMETA;
    
    status = pst_node(sess_cb, &sess_cb->pss_ostream, 
		      node1, node2,
		      PST_BOP, (char *) &opnode, sizeof(PST_OP_NODE),
		      DB_NODT, (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL, 
		      &eqnode, err_blk, PSS_NORES);
    if (DB_FAILURE_MACRO(status))
	return (status);
    

    /* now loop through the column lists,
    ** filling in type info and testing each pair of columns
    */
    for (colno = 0; colno < num_cols; colno++)
    {
	cur_col     = cons_att_array[colno];
	cur_ref_col = ref_att_array[colno];

	if ((cur_col == (DMF_ATTR_ENTRY *) NULL) 
	    || (cur_ref_col == (DMT_ATT_ENTRY *) NULL))
	{
	    /* should never happen */
	    (VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, 
			     PSF_INTERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}	    

	/* fill in column info for the two columns
	 */
	cui_move(cur_ref_col->att_nmlen, cur_ref_col->att_nmstr, ' ',
		DB_ATT_MAXNAME, varnode.pst_atname.db_att_name);

	status = pst_node(sess_cb, &sess_cb->pss_ostream, 
			  (PST_QNODE *) NULL, (PST_QNODE *) NULL, 
			  PST_VAR, (char *) &varnode, sizeof(PST_VAR_NODE), 
			  cur_ref_col->att_type, cur_ref_col->att_prec,
			  cur_ref_col->att_width, (DB_ANYTYPE *) NULL, 
			  &node2, err_blk, PSS_NOALLOC);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* depending on attr info passed in, setup info for 2nd column
	 */
	if ((newtbl) || (qmode == PSQ_ATBL_ADD_COLUMN))
	{
	    /* have new table and DMF_ATTR_ENTRY structure 
	     */
	    STRUCT_ASSIGN_MACRO(cur_col->attr_name, varnode.pst_atname);
	
	    status = pst_node(sess_cb, &sess_cb->pss_ostream, 
			      (PST_QNODE *) NULL, (PST_QNODE *) NULL, 
			      PST_VAR, (char *) &varnode, sizeof(PST_VAR_NODE), 
			      cur_col->attr_type, cur_col->attr_precision,
			      cur_col->attr_size, (DB_ANYTYPE *) NULL, 
			      &node1, err_blk, PSS_NOALLOC);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	else 
	{
	    /* have existing table, so use DMT_ATT_ENTRY structure 
	     */
	    cur_col_dmt = (DMT_ATT_ENTRY *) cons_att_array[colno];

	    cui_move(cur_col_dmt->att_nmlen, cur_col_dmt->att_nmstr, ' ',
		DB_ATT_MAXNAME, varnode.pst_atname.db_att_name);

	    status = pst_node(sess_cb, &sess_cb->pss_ostream, 
			      (PST_QNODE *) NULL, (PST_QNODE *) NULL, 
			      PST_VAR, (char *) &varnode, sizeof(PST_VAR_NODE), 
			      cur_col_dmt->att_type, cur_col_dmt->att_prec,
			      cur_col_dmt->att_width, (DB_ANYTYPE *) NULL, 
			      &node1, err_blk, PSS_NOALLOC);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	
	/* now perform the type resolution
	 */
	status = pst_node(sess_cb, &sess_cb->pss_ostream, 
			  node1, node2,
			  PST_BOP, (char *) &opnode, sizeof(PST_OP_NODE),
			  DB_NODT, (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL, 
			  &eqnode, err_blk, PSS_TYPERES_ONLY | PSS_REF_TYPERES);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	
    }  /* end for colno = 0 */
    
    /* if no error was returned by the 3rd pst_node call,
    ** then no column type incompatibilities were found
    */
    return (E_DB_OK);

}  /* end psl_ver_ref_types */




/*
** Name: psl_handle_self_ref  - generate an execute immediate node for the
** 				ALTER TABLE statement to add a self-referential
** 				constraint
**
** Description:	    
**          Initialize fields in a statement node for an EXECUTE IMMEDIATE
**          statement.  Generate text for an ALTER TABLE ADD constraint
**          command to create the self-referential constraint.
**          
**          This function should only be called for self-referential
**          constraints defined within a CREATE TABLE; otherwise we will get
**          into an infinite loop of ALTER TABLE exec-immediate statements.
**          The statement node returned should be placed AFTER the CREATE TABLE
**          statement and any CREATE_INTEGRITY statements in the query tree.
**
**          The purpose of this function is to simplify handling of
**          self-referential constraints, by postponing the actual creation of
**          the constraint until AFTER the table and all its unique constraints
**          have been created.  Then system catalogs can be queried using RDF
**          (just like for normal referential constraints), instead of having
**          special-case code for looking up info on a table and constraints
**          that haven't been created yet.
**
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	    pss_ostream	      memory stream for allocating query text
**	    pss_memleft	      amount of memory left (used by psq_topen)
**	stmt		    already allocated & initialized PST_STATEMENT node
**      obj_name	    contains name and schema of table being created
**      cons		    constraint info block
**
** Outputs:
**	stmt		    EXECUTE IMMEDIATE statement node;
**			    following fields are initialized/filled in:
**	    pst_specific.pst_execImm	execute immediate structure
**		pst_info	    specific info about current statement
**		pst_qrytext	    text of statement to be executed
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**	16-mar-93 (rblumer)
**	    written.
**	02-may-93 (andre)
**	    set PST_EMPTY_REFERENCING_TABLE in pst_execflags.  This will tell 
**	    us to skip checking whether the constraint initially holds
**
**	    instead of passing address of query_text to psl_gen_alter_text()
**	    and then copying the text string into yet another chunk of memory
**	    pointed to by exec_node->pst_qrytext, we will pass address of
**	    exec_node->pst_qrytext instead.
**
**	    Appending EOS to the end of text string was wrong, since psl_sscan()
**	    barfs upon seeing EOS in the query text.  SCF will pad the query
**	    text with a blank, so the text string returned by
**	    psl_gen_alter_text() will be quite adequate
**	16-jul-93 (rblumer)
**	    changed PST_EMPTY_REFERENCING_TABLE to PST_EMPTY_TABLE.
*/
static DB_STATUS 
psl_handle_self_ref(
		    PSS_SESBLK	  *sess_cb,
		    PST_STATEMENT *stmt,
		    PSS_OBJ_NAME  *obj_name,
		    PSS_CONS	  *cons,
		    DB_ERROR	  *err_blk)
{
    DB_STATUS	    status;
    PST_EXEC_IMM    *exec_node;

    /* reset the statement type
     */
    stmt->pst_type = PST_EXEC_IMM_TYPE;
 
    /* clear the execute immediate part of the statement node
     */
    exec_node = &(stmt->pst_specific.pst_execImm);
    MEfill(sizeof(PST_EXEC_IMM), (u_char) 0, (PTR) exec_node);

    /* initialize the PST_INFO portion of the PST_EXEC_IMM area
    */
    exec_node->pst_info.pst_baseline  = stmt->pst_lineno;
    exec_node->pst_info.pst_basemode  = stmt->pst_mode;
    exec_node->pst_info.pst_execflags = PST_EMPTY_TABLE;

    /* generate the ALTER TABLE query text
     */
    status = psl_gen_alter_text(sess_cb,
				&sess_cb->pss_ostream,
				&sess_cb->pss_memleft,
				&obj_name->pss_owner,
				&obj_name->pss_obj_name,
				cons, &exec_node->pst_qrytext, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);

    return (E_DB_OK);

} /* end psl_handle_self_ref */


/*
** Name: psl_ver_ref_priv     - verify that user has references privilege
** 				on the referenced table
**
** Description:	    
**          Check to make sure that the user is allowed to create referential
**          constraints on the set of referenced columns in the referenced
**          table.
**
**          If the user owns the referenced table, there is no need to
**          explicitly check, as users automatically have all privileges on
**          their own tables.
**          Otherwise, set up the col_privs and call psy_check_privs
**          to see if the user has the correct privileges.  Note that
**          WITH GRANT OPTION is not needed for referential constraints. 
**
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	    pss_user	      name of current user
**	rngvar		    holds description of referenced table
**			      passed to psy_check_privs
**      ref_map		    bitmap of referenced columns
**      cr_integ	    create integrity statement node
**	    pss_ref_tabname   used in error messages
**	    pss_ref_ownname   used in error messages
**	    pst_integTuple
**	        dbi_consname    used in error messages
**	    pst_indep	    descriptor of object(s) and/or privilege(s) on which
**			    a REFERENCES constraint will depend
**
** Outputs:
**	pst_indep
**	    psq_colprivs    if the owner of the <referenced table> is different
**			    from that of the <referencing table>, psq_colprivs
**			    will point at a description of a privilege on which
**			    the new constraint will depend
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	looks up IIPROTECT tuples
**
** History:
**	19-mar-93 (rblumer)
**	    written.
**	26-mar-93 (andre)
**	    if the owner of the <referenced table> is different from that of the
**	    <referencing table>, we will allocate a PSQ_COLPRIV structure,
**	    populate it with a description of a privilege on which the new
**	    REFERENCES constraint will depend, and attach it to
**	    cr_integ->pst_indep
**	26-mar-93 (rblumer)
**	    remove ps131 trace point lookup, and replace with FALSE; ps131 has
**	    to do with GRANT processing, so we don't ever want to use it here. 
**	07-apr-93 (rblumer)
**	    change error message to use rngvar table and owner names,
**	    since names in cr_integ haven't been filled in yet.
*/
static DB_STATUS 
psl_ver_ref_priv(
		 PSS_SESBLK	     *sess_cb,
		 PSS_RNGTAB	     *rngvar,
		 DB_COLUMN_BITMAP    *ref_map,
		 PST_CREATE_INTEGRITY *cr_integ,
		 DB_ERROR	     *err_blk)
{
    DB_STATUS	  status;
    i4		  privs_to_find;
    PSY_COL_PRIVS ref_colprivs;
    i4	  err_code;
    DB_TAB_ID	  junk_id;
    i4 		  junk_flag;
    
    /* if current user is owner of the referenced table,
    ** we don't need to look up privileges
    */
    if (MEcmp(rngvar->pss_ownname.db_own_name,
	      sess_cb->pss_user.db_own_name, DB_OWN_MAXNAME) == 0)
	return (E_DB_OK);
    
    /* set up parameters to function for checking privileges.
    ** Set up references privilege and copy the column bitmap
    ** into the references bitmap.
    ** Also, tell function not to look for the with-grant-option,
    ** and not to print out any GRANT-specific errors.
     */
    privs_to_find = DB_REFERENCES;

    ref_colprivs.psy_col_privs = DB_REFERENCES;
    MEcopy((PTR) ref_map,
	   sizeof(PSY_ATTMAP),
	   (PTR) &ref_colprivs.psy_attmap[PSY_REFERENCES_ATTRMAP]);
    
    status = psy_check_privs(rngvar, &privs_to_find, (i4) 0, 
			     &ref_colprivs, sess_cb, (bool) FALSE,
			     &junk_id, 
			 PSY_PRIVS_NO_GRANT_OPTION | PSY_PRIVS_DONT_PRINT_ERROR,
			     &junk_flag, err_blk);

    if (DB_FAILURE_MACRO(status))
	return(status);
    
    /* if user had correct privileges, then privs_to_find is cleared,
    ** so if it iss non-zero, return an error.
    */
    if (privs_to_find)
    {
	(void) psf_error(E_PS04A1_NO_REF_PRIV, 0L, PSF_USERERR,
			 &err_code, err_blk, 2,
			 psf_trmwhite(DB_TAB_MAXNAME,
				      rngvar->pss_tabname.db_tab_name),
			 rngvar->pss_tabname.db_tab_name,
			 psf_trmwhite(DB_OWN_MAXNAME,
				      rngvar->pss_ownname.db_own_name),
			 rngvar->pss_ownname.db_own_name);

	/* print out constraint name, if user specified one
	 */
	if (cr_integ->pst_createIntegrityFlags & PST_CONS_NAME_SPEC)
	{
	    DB_INTEGRITY  *integ_tup = cr_integ->pst_integrityTuple;

	    (void) psf_error(E_PS0494_CONS_NAME, 0L, PSF_USERERR, 
			     &err_code, err_blk, 1,
			     psf_trmwhite(DB_CONS_MAXNAME,
				    integ_tup->dbi_consname.db_constraint_name),
			     integ_tup->dbi_consname.db_constraint_name);
	}
	return(E_DB_ERROR);
    }

    /*
    ** session possesses REFERENCES privilege on <referenced columns> - build a
    ** description of this privilege so it can be passed to QEF via OPC
    */
    {
	PSQ_INDEP_OBJECTS	*indep_objs;
	PSQ_COLPRIV		*indep_col_priv;
	DB_OWN_NAME		*grantee;
	
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PSQ_COLPRIV),
	    (PTR *) &indep_col_priv, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	indep_col_priv->psq_next = (PSQ_COLPRIV *) NULL;
	indep_col_priv->psq_objtype = PSQ_OBJTYPE_IS_TABLE;
	indep_col_priv->psq_privmap = DB_REFERENCES;
	indep_col_priv->psq_tabid.db_tab_base  = rngvar->pss_tabid.db_tab_base;
	indep_col_priv->psq_tabid.db_tab_index = rngvar->pss_tabid.db_tab_index;
	MEcopy((PTR) ref_map, sizeof(indep_col_priv->psq_attrmap),
	       (PTR) indep_col_priv->psq_attrmap);
	
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DB_OWN_NAME),
	    (PTR *) &grantee, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	STRUCT_ASSIGN_MACRO(sess_cb->pss_user, (*grantee));

	/*
	** finally attach description of the privilege on which the constraint
	** will depend and a name of the grantee of that privilege to the
	** PSQ_INDEP_OBJS structure associated with this constraint
	*/
	indep_objs = (PSQ_INDEP_OBJECTS *) cr_integ->pst_indep;
	indep_objs->psq_colprivs = indep_col_priv;
	indep_objs->psq_grantee = grantee;
    }

    return (E_DB_OK);

} /* end psl_ver_ref_priv */




/*
** Name: psl_qual_primary_cons  - determine whether a tuple represents a 
**				  PRIMARY KEY constraint
**
** Description:	    
**          Address of this function will be passed to pss_find_cons() by 
**	    callers interested in a tuple representing a PRIMARY KEY constraint
**      
** Inputs:
**	integ		IIINTEGRITY tuple to be considered
**
** Outputs:
**	satisfies	will be set to TRUE if the tuple represents a 
**			PRIMARY KEY constraint
**
** Returns:
**	E_DB_OK
**
** Side effects:
**	none
**
** History:
**	09-jan-94 (andre)
**	    written
*/
static DB_STATUS
psl_qual_primary_cons(
		     PTR		*qual_args,
		     DB_INTEGRITY       *integ,
		     i4		*satisfies,
		     PSS_SESBLK	*sess_cb)
{
    if (integ->dbi_consflags & CONS_PRIMARY)
	*satisfies = TRUE;

    return(E_DB_OK);
}

/*
** Name: psl_qual_ref_cons  - determine whether a tuple represents a REFERENCES
**		  	      constraint
**
** Description:	    
**          Address of this function will be passed to pss_find_cons() by 
**	    callers interested in a tuple representing a REFERENCES constraint
**      
** Inputs:
**	integ		IIINTEGRITY tuple to be considered
**
** Outputs:
**	satisfies	will be set to TRUE if the tuple represents a 
**			REFERENCES constraint
**
** Returns:
**	E_DB_OK
**
** Side effects:
**	none
**
** History:
**	09-jan-94 (andre)
**	    written
*/
DB_STATUS
psl_qual_ref_cons(
		  PTR		*qual_args,
		  DB_INTEGRITY  *integ,
		  i4		*satisfies,
		  PSS_SESBLK	*sess_cb)
{
    if (integ->dbi_consflags & CONS_REF)
	*satisfies = TRUE;

    return(E_DB_OK);
}

/*
** Name: psl_find_cons  - find IIINTEGRITY tuple representing a specified type 
**			  of constraint
**
** Description:	    
**          This function will scan IIINTEGRITY tuples associated with a given 
**	    table and pass them (one at a time) to the qualification function 
**	    suppled by the caller to determine whether this tuple represents a 
**	    type of constraint in which the caller is interested.  
**	    NOTE: this function should be used only if at most one tuple may be
**	    of interest to the caller
**      
** Inputs:
**	sess_cb		PSF session CB
**	rngvar		range variable describing the table whose IIINTEGRITY 
**			tuples we will be perusing
**	qual_func	qualification function used to determine whether a 
**			given tuple represents the constraint of interest to 
**			the caller
**	qual_args	array (possibly empty) of arguments to be fed to the 
**			qualification function
**
** Output:
**	integ		location to store a copy of a tuple (if one is found) 
**			representing a constraint which is of interest to the 
**			caller - may be set to NULL if the caller is only 
**			interested in existence of a tuple
**	found		TRUE if a desired tuple was found; FALSE otherwise
**	err_blk		filled in if an error occurred
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	fixes and unfixes IIINTEGRITY tuples in RDF cache
**
** History:
**	09-jan-94 (andre)
**	    written
*/
DB_STATUS
psl_find_cons(
	      PSS_SESBLK        	*sess_cb,
	      PSS_RNGTAB        	*rngvar,
	      PSS_CONS_QUAL_FUNC	qual_func,
	      PTR			*qual_args,
	      DB_INTEGRITY		*integ,
	      i4			*found,
	      DB_ERROR          	*err_blk)
{
    DB_STATUS     status = E_DB_OK;
    DB_STATUS     temp_status;
    DB_INTEGRITY  *cur_integ;
    RDF_CB        rdf_cb;
    QEF_DATA      *qp;
    i4            tupcount;
    i4       err_code;

    /* Set up constant part of rdf control block
    ** for getting ALL integrity tuples 
    ** (the following code was taken from psy_integ and modified slightly)
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_rb.rdr_types_mask    = RDR_INTEGRITIES;
    rdf_cb.rdf_rb.rdr_update_op     = RDR_OPEN;
    rdf_cb.rdf_rb.rdr_rec_access_id = NULL;
    rdf_cb.rdf_rb.rdr_qtuple_count  = RDF_MAX_QTCNT; 
    rdf_cb.rdf_info_blk = rngvar->pss_rdrinfo;

    *found = FALSE;
    
    /* For each group of integrities 
     */
    while (rdf_cb.rdf_error.err_code == 0 && status == E_DB_OK && !*found)
    {
	status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

	/*
	** RDF may choose to allocate a new info block and return its address
	** in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
	** memory leak and other assorted unpleasantries
	*/
	rngvar->pss_rdrinfo = rdf_cb.rdf_info_blk;
	  
	rdf_cb.rdf_rb.rdr_update_op = RDR_GETNEXT;

	/* 
	** can't use DB_FAILURE_MACRO because RDF returns a warning (E_DB_WARN)
	** for errors such as RD0011, and we want to reset the status in those
	** cases, too.
	*/
	if (status != E_DB_OK)
	{
	    switch (rdf_cb.rdf_error.err_code)
	    {
	        case E_RD0002_UNKNOWN_TBL:
		{   
		    (VOID) psf_error(2117L, 0L, PSF_USERERR,
		        &err_code, err_blk, 1,
			psf_trmwhite(sizeof(DB_TAB_NAME), 
			    rngvar->pss_tabname.db_tab_name),
			 rngvar->pss_tabname.db_tab_name);
		    status = E_DB_ERROR;
		    continue;
		}

	        case E_RD0011_NO_MORE_ROWS:
		{
		    status = E_DB_OK;
		    break;
		}

	    	case E_RD0013_NO_TUPLE_FOUND:
		{
		    status = E_DB_OK;
		    continue;
		}
	
	    	default:
		{
		    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, 
			err_blk);
		    status = E_DB_ERROR;
		    continue;
		}
	    } /* end switch rdf_cb.err_code */
	} /* end if DB_FAILURE_MACRO */
    
	/* FOR EACH INTEGRITY 
	 */
	for (qp = rdf_cb.rdf_info_blk->rdr_ituples->qrym_data,
	     tupcount = 0;
	     tupcount < rdf_cb.rdf_info_blk->rdr_ituples->qrym_cnt;
	     qp = qp->dt_next,
	     tupcount++)
	    {
		cur_integ = (DB_INTEGRITY*) qp->dt_data;

		status = qual_func(qual_args, cur_integ, found, sess_cb);
		if (DB_FAILURE_MACRO(status))
		    break;

		if (*found)
		{
		    /* 
		    ** found a tuple representing a desired constraint - if the 
		    ** needs to examine it, copy it into caller's memory
		    */
		    if (integ)
		    {
		        STRUCT_ASSIGN_MACRO(*cur_integ, *integ);
		    }

		    break;
		}
	    }  /* end FOR EACH INTEGRITY */
    }  /* end while rdf_cb.err_code = 0 && status == E_DB_OK && !*found*/
	
    /* unfix integrity tuples
     */
    if (rdf_cb.rdf_rb.rdr_rec_access_id != NULL)
    {
	rdf_cb.rdf_rb.rdr_update_op = RDR_CLOSE;

	/* use temp_status so that we don't overwrite 
	** a possible error status from above
	*/
	temp_status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

	if (DB_FAILURE_MACRO(temp_status))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	    return(temp_status > status ? temp_status : status );
	}
    }

    return(status);

}  /* end psl_find_cons */

/*
** Name: get_primary_key_cols  - retrieves key columns for a constraint from 
** 				 IIKEY, and returns an ordered list of columns
**
** Description:	    
**          Given a constraint id and a rngvar (which contains table id and RDF
**          info), calls RDF to get the columns contained in the key and their
**          order in the key.
**          Returns an linked list of column ids, sorted in the order they
**          appear in the primary key, plus returns an array of attribute
**          descriptors, sorted in the order they appear in key.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB, used to initialize RDF_CB
**	    pss_ostream	      ptr to mem stream for allocating memory
**	rngvar		    table for which primary key is needed
**	    pss_rdrinfo	      already initialized RDF table info block
**	        rdr_attr	array of attribute descriptors, in column order
**	cons_id		    ID of primary key constraint
**	att_array	    pre-allocated space for column attributes
**	colid_listp	    pointer to a colid pointer 
**
** Outputs:
**	att_array	    filled in with key columns, in order
**	colid_listp	    points to head of linked list of key columns,
**				in key order
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
**	22-feb-93 (rblumer)
**	    written
**	15-jun-93 (rblumer)
**	    initialize key_count variable to 0.
**	10-aug-93 (andre)
**	    fixed cause of compiler warnings
**	18-apr-94 (rblumer)
**	    changed malloc'ing of PST_COL_ID nodes to be done all at once in
**	    one block, rather than allocating them one-by-one.
*/
static
DB_STATUS
get_primary_key_cols(
		     PSS_SESBLK	    *sess_cb, 
		     PSS_RNGTAB	    *rngvar, 
		     DB_CONSTRAINT_ID *cons_id,
		     DMT_ATT_ENTRY  **att_array, 
		     PST_COL_ID	    **colid_listp, 
		     DB_ERROR	    *err_blk)
{
    DB_STATUS	status;
    DB_STATUS	temp_status;
    DB_IIKEY	*cur_key;
    DB_IIKEY	*key_array;
    DMT_ATT_ENTRY **rdr_attr = rngvar->pss_rdrinfo->rdr_attr;
    RDF_CB	rdf_cb;
    i4		i, tup_count;
    i4		key_count = 0;
    i2		cur_att_id;
    PST_COL_ID	*tail, *colnode;

    /* set return pointer to NULL
     */ 
    *colid_listp = (PST_COL_ID *) NULL;

    /*
    ** call RDF to get the key tuples;
    ** be sure to put tuples in the array in key-position order.
    */

    /* allocate space for key tuples
     */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			RDF_MAX_QTCNT * sizeof(DB_IIKEY),
			(PTR *) &key_array, err_blk);
    if (DB_FAILURE_MACRO(status))
	return (status);
	
    MEfill(RDF_MAX_QTCNT * sizeof(DB_IIKEY),
	   (u_char) 0, (PTR) key_array);

    /* Set up constant part of rdf control block
    ** for getting a block of iikey tuples 
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(*cons_id, rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_rb.rdr_2types_mask    = RDR2_KEYS;
    rdf_cb.rdf_rb.rdr_update_op     = RDR_OPEN;
    rdf_cb.rdf_rb.rdr_rec_access_id = NULL;
    rdf_cb.rdf_rb.rdr_qtuple_count  = RDF_MAX_QTCNT; 
    rdf_cb.rdf_rb.rdr_qrytuple      = (PTR) key_array;

    /* For each group of keys 
     */
    while (rdf_cb.rdf_error.err_code == 0)
    {
	status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

	/* all calls except the first OPEN  will be a getnext
	 */ 
	rdf_cb.rdf_rb.rdr_update_op = RDR_GETNEXT;

	/* can't use DB_FAILURE_MACRO because RDF returns a warning (E_DB_WARN)
	** for errors such as RD0011, and we want to reset the status in those
	** cases, too.
	*/
	if (status != E_DB_OK)
	{
	    switch (rdf_cb.rdf_error.err_code)
	    {
	    case E_RD0011_NO_MORE_ROWS:
		status = E_DB_OK;
		break;

	    case E_RD0013_NO_TUPLE_FOUND:
		status = E_DB_OK;
		continue;
	
	    default:
		(VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
		status = E_DB_ERROR;
		break;

	    } /* end switch rdf_cb.err_code */

	    /* if a real error, break out of loop and do cleanup
	     */
	    if (DB_FAILURE_MACRO(status))
		break;
	    
	} /* end if DB_FAILURE_MACRO */
    
	/* FOR EACH KEY,
	** get its description from the attribute array
	** and store it (in order) in the returned attribute array;
	*/
	for (tup_count = 0;
	     tup_count < rdf_cb.rdf_rb.rdr_qtuple_count;
	     tup_count++)
	{
	    cur_key    = &key_array[tup_count];
	    cur_att_id = cur_key->dbk_column_id.db_att_id;
	    key_count++;

	    /* offset position by one, as they start at 1
	    ** and C arrays start at 0
	    */
	    att_array[cur_key->dbk_position -1] = rdr_attr[cur_att_id];
	    
	}  /* end FOR EACH KEY */

    }  /* end while rdf_cb.err_code = 0 */
	
    /* close iikey table
     */
    if (rdf_cb.rdf_rb.rdr_rec_access_id != NULL)
    {
	rdf_cb.rdf_rb.rdr_update_op = RDR_CLOSE;

	/* use temp_status so that we don't overwrite 
	** a possible error status from above
	*/
	temp_status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

	if (DB_FAILURE_MACRO(temp_status))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	    return(temp_status > status ? temp_status : status );
	}
    }

    if (DB_FAILURE_MACRO(status))
	return(status);


    /*  now allocate the col_id's,
    **  and build a column_id list using the ordered attribute array
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			key_count * sizeof(PST_COL_ID),
			(PTR *) &colnode, err_blk); 
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* we will return the head of the col_id list
     */
    *colid_listp = colnode;

    /* build the column_id list using the ordered attribute array
     */
    for (i = 0; i < key_count; i++)
    {
	colnode->col_id.db_att_id = att_array[i]->att_number;	
	    
	/* add current node to end of list (i.e. to the previous node)
	** except if this is the first node in list.
	*/
	if (i != 0)
	    tail->next = colnode;

	tail = colnode;
	colnode++;

    }  /* end for i < key_count */

    /* set the last col_id node's pointer to NULL to end the list
     */
    tail->next  = (PST_COL_ID *) NULL;

    return(E_DB_OK);
    
}  /* end get_primary_key_cols */




/*
** Name: psl_find_primary_cons  - find primary key constraint, and 
** 				  build an ordered list of its columns
**
** Description:	    
**          Given a rngvar (which contains table id and RDF info),
**          find the primary key on the table represented by it.
**          If there isn't a primary key, print and return an error;
**          if there is one, get the order of the key columns,
**          and return a linked list AND an ordered array of the key columns.
**	    
**          Returns an linked list of column ids, sorted in the order they
**          appear in the primary key, plus returns an array of attribute
**          descriptors, sorted in the order they appear in key.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB, used to initialize RDF_CB
**	    pss_ostream	      ptr to mem stream for allocating memory
**	rngvar		    table for which primary key is needed
**	    pss_tabid	      id of table
**	    pss_rdrinfo	      already initialized RDF table info block
**
** Outputs:
**	col_map		    filled in with column bitmap for constraint
**	cons_id		    ID of primary key constraint
**	integ_no	    integrity number of the primary key constraint
**			    constraint id is not very useful when trying to find
**			    IIINTEGRITY tuple for a given constraint, which is
**			    why I decided to use a combination of
**			    (tbl_id, tbl_idx, integ_no) to describe a PRIMARY
**			    KEY constraint
**	att_array	    filled in with key columns, in order
**	colid_listp	    filled in with linked list of key columns, in order
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
**	22-feb-93 (rblumer)
**	    written
**	26-mar-93 (andre)
**	    added integ_no to the interface.  At this point, I am not sure
**	    whether constraint id needs to be returned to the caller, but I will
**	    leave it here until we decide on its fate
**	18-apr-94 (rblumer)
**	    merged get_primary_cons function into this function,
**	    since get_primary_cons has become so simple.
*/
static DB_STATUS
psl_find_primary_cons(
		      PSS_SESBLK	*sess_cb,
		      PSS_RNGTAB	*rngvar,
		      DB_COLUMN_BITMAP	*col_map,  /* returned */
		      DB_CONSTRAINT_ID	*cons_id,  /* returned */
		      u_i4		*integ_no, /* returned */
		      DMT_ATT_ENTRY	**att_array,/* returned */
		      PST_COL_ID	**colid_listp,/* returned */
		      DB_ERROR		*err_blk)
{
    DB_STATUS	status;
    i4		found;
    i4	err_code;
    DB_INTEGRITY  integ;
    
    /* Look for a primary key on the table in 'rngvar',
    ** by scanning integrity catalog for a PRIMARY KEY constraint
    */
    status = psl_find_cons(sess_cb, rngvar, psl_qual_primary_cons, (PTR *) NULL,
			   &integ, &found, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* if we did not find a primary key, return an error
     */
    if (!found)
    {
	(void) psf_error(E_PS0491_NO_PRIMARY_KEY, 0L, PSF_USERERR, 
			 &err_code, err_blk, 1,
			 psf_trmwhite(sizeof(DB_TAB_NAME), 
				      rngvar->pss_tabname.db_tab_name),
			 rngvar->pss_tabname.db_tab_name);
	return(E_DB_ERROR);
    }
    /* ELSE we have found a tuple;
    ** process it below
    */
    
    /* if the tuple describing PRIMARY KEY constraint was found,
    ** copy constraint id, integrity number, and column map into return fields
    */
    STRUCT_ASSIGN_MACRO(integ.dbi_cons_id, *cons_id);
    *integ_no = integ.dbi_number;
    STRUCT_ASSIGN_MACRO(integ.dbi_columns, *col_map);
    
    /* find order of columns in key from IIKEY, 
    ** and build ref_attr_array and ref_collist
    */
    status = get_primary_key_cols(sess_cb, rngvar, cons_id,
				  att_array, colid_listp, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);
    
    return(E_DB_OK);
    
}  /* end psl_find_primary_cons */



/*
** Name: psl_find_unique_cons  - find a unique constraint on a table
** 				 that is defined on a given set of columns
**
** Description:	    
**          Given a rngvar (which contains table id and RDF info) and a column
**          bitmap, find a unique constraint defined on the same set of
**          columns.
**          If there isn't a matching constraint, print and return an error;
**          if there is one, return the integrity number for that constraint.
**      
** Inputs:
**	sess_cb		    ptr to PSF session CB, used to initialize RDF_CB
**	col_map		    bitmap representing columns we are looking for
**	rngvar		    table for which primary key is needed
**	    pss_tabid	      id of table
**	    pss_rdrinfo	      already initialized RDF table info block
**
** Outputs:
**	cons_id		    ID of unique constraint (not used anymore)
**	integ_no	    integrity number of the primary key constraint
**			 (constraint id is not very useful when trying to find
**			  IIINTEGRITY tuple for a given constraint, which is
**			  why we decided to use a combination of
**			  (tbl_id, tbl_idx, integ_no) to describe a constraint)
**	err_blk		    filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	calls psl_find_cons, which allocates memory
**
** History:
**	16-mar-93 (rblumer)
**	    written
**	10-aug-93 (andre)
**	    fixed cause of compiler warnings
*/
static DB_STATUS 
psl_find_unique_cons(
		     PSS_SESBLK 	*sess_cb,
		     DB_COLUMN_BITMAP 	*col_map,
		     PSS_RNGTAB 	*rngvar,
		     DB_CONSTRAINT_ID 	*cons_id,
		     u_i4		*integ_no,  /* returned */
		     DB_ERROR		*err_blk)   /* returned */
{
    DB_STATUS	  status = E_DB_OK;
    DB_INTEGRITY  integ;
    i4		  found = FALSE;
    PTR		  qual_args[1];
    i4	  err_code;
    
    qual_args[0] = (PTR) col_map;

    /* 
    ** scan IIINTEGRITY catalog looking for UNIQUE/PRIMARY KEY constraint on a 
    ** specified set of attributes
    */
    status = psl_find_cons(sess_cb, rngvar, psl_qual_unique_cons, qual_args,
	&integ, &found, err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (found)
    {
	STRUCT_ASSIGN_MACRO(integ.dbi_cons_id, *cons_id);
	*integ_no = integ.dbi_number;
    }
    else
    {
        MEfill(sizeof(DB_CONSTRAINT_ID), NULLCHAR, (PTR) cons_id);

	(void) psf_error(E_PS0490_NO_UNIQUE_CONSTRAINT, 0L, PSF_USERERR, 
			 &err_code, err_blk, 1,
			 psf_trmwhite(sizeof(DB_TAB_NAME), 
				      rngvar->pss_tabname.db_tab_name),
			 rngvar->pss_tabname.db_tab_name);
	status = E_DB_ERROR;
    }
    
    return(status);

}  /* end psl_find_unique_cons */

/*
** Name: psl_qual_unique_cons  - determine whether a tuple represents a 
**				 UNIQUE constraint on a specified set of columns
**
** Description:	    
**          Address of this function will be passed to pss_find_cons() by 
**	    callers interested in a tuple representing a UNIQUE constraint
**	    on a specified set of columns
**      
** Inputs:
**	qual_args	args to this function
**	    [0]		map of attributes on which the desired UNIQUE 
**			constraint must be defined
**	integ		IIINTEGRITY tuple to be considered
**
** Outputs:
**	satisfies	will be set to TRUE if the tuple represents a 
**			UNIQUE constraint on a specified set of columns
**
** Returns:
**	E_DB_OK
**
** Side effects:
**	none
**
** History:
**	09-jan-94 (andre)
**	    written
*/
static DB_STATUS
psl_qual_unique_cons(
		     PTR		*qual_args,
		     DB_INTEGRITY       *integ,
		     i4		*satisfies,
		     PSS_SESBLK	*sess_cb)
{
    if (   integ->dbi_consflags & CONS_UNIQUE
        && !MEcmp((PTR) &integ->dbi_columns, qual_args[0],
	       sizeof(DB_COLUMN_BITMAP)))
    {
	/* tuple represents a UNIQUE constraint on a desired set of columns */
	*satisfies = TRUE;
    }

    return(E_DB_OK);
}

/*
** Name: psl_ver_check_cons  - Verify that columns in CHECK constraint exist
**                             and are of the correct type
**
** Description:	    
**          Walks the boolean expression in the check-condition tree
**          and checks that columns in the tree exist and are of the correct
**          type for the expressions in the tree.
**          Fills in the bitmap of the columns in the condition and 
**          the table name.  If a column in the table is made
**          known-not-nullable by the constraint, a 2nd integrity tuple is
**          allocated and its bit map is filled in.
**
** Inputs:
**	sess_cb		    ptr to PSF session CB (passed to other routines)
**	    pss_ostream	      ptr to mem stream for allocating 2nd tuple
**	psq_cb		    PSF request CB
**	    psq_mode	      query mode  (used for error messages)
**      cons		    constraint info block
**      newtbl		    TRUE implies table is being created, 
**                              and table info is in 'dmu_cb';
**               	    FALSE implies table already exists, 
**                              and table info is in 'rngvar'.
**	dmu_cb		    info about table (if newtbl is TRUE), else NULL
**	rngvar		    info about table (if newtbl is FALSE), else NULL
**
** Outputs:
**	cr_integ	    statement node; following fields are filled in:
**	    pst_integrityTuple
**		dbi_consflags	CHECK bit, possibly KNOWN_NOT_NULLABLE bit
**		dbi_columns	bitmap of columns in the check condition
**	    pst_knownNotNullableTuple
**		dbi_columns	bitmap of columns made known-not-nullable
**	    pst_integrityTree	search condition for check constraint
**	psq_cb
**	    psq_error		filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	allocates memory
**
** History:
**
**	10-sep-92 (rblumer)
**	    written
**	16-mar-93 (rblumer)
**	    set sequence number to 2 and set CONS_KNOWN_NOT_NULLABLE_TUPLE bit
**	    in 2nd tuple created for not null columns;
**	06-may-93 (andre)
**	    if creating a CHECK constraint using ALTER TABLE, all required type
**	    resolution has taken place and cons->pss_check_cons_cols points at a
**	    map of attributes referenced in the <search condition>.
**	    All that's left to do in that case is to copy
**	    *cons->pss_check_cons_cols into integ_tup->dbi_columns, and generate
**	    a list of PST_COL_ID's describing attributes referenced in the
**	    <search condition>
**	07-may-93 (andre)
**	    if called to process description of a CHECK constraint specified
**	    inside CREATE TABLE statement, set PSS_RESOLVING_CHECK_CONS in
**	    sess_cb->pss_stmt_flags and pass the prototype tree representing the
**	    <search condition> to psl_p_telem() to resolve references to
**	    attributes as well as perform type resolution for op-nodes.
**
**	    changed interface to accept psq_cb instead of qmode and err_blk
**	    because we need to pass (PSQ_CB *) to psl_p_telem()
**	10-may-93 (andre)
**	    used col_id instead of cons_col_id when generating message
**	    E_PS0472_COL_CHECK_CONSTRAINT
**	16-aug-93 (rblumer)
**	    for implicit CONS_NOT_NULL constraints created during CREATE TABLE
**	    AS SELECT processing, use the previously set-up bitmap, instead of
**	    generating one from the column name.
**	23-sep-93 (rblumer)
**	    return an error if we find a TID column specified in a constraint
**	08-jan-94 (andre)
**	    we will prevent a user from adding a CHECK constraint on an existing
**	    table if the database is being journaled but the table is not
**	18-apr-94 (rblumer)
**	    split tblinfo into 2 parameters (dmu_cb & rngvar) so that it can be
**	    correctly typed (instead of being a PTR that is one or the other);
**	    changed malloc'ing of PST_COL_ID nodes to be done all at once in
**	    one block, rather than allocating them one-by-one.
**	27-nov-02 (inkdo01)
**	    Range table expansion changes a cast in psl_p_telem call.
**	4-Feb-2005 (schka24)
**	    Change params for psl_p_telem call again.
**	04-nov-05 (toumi01)
**	    Add xform_avg parameter to psl_p_telem call.
*/
static DB_STATUS
psl_ver_check_cons(
		   PSS_SESBLK	*sess_cb,
		   PSQ_CB	*psq_cb,
		   PSS_CONS     *cons,
		   i4		newtbl,
		   DMU_CB	*dmu_cb,
		   PSS_RNGTAB	*rngvar,
		   PST_CREATE_INTEGRITY *cr_integ)
{
    i4		   qmode = psq_cb->psq_mode;
    DB_INTEGRITY   *integ_tup;
    DB_INTEGRITY   *integ_tup2;
    DB_STATUS	   status;
    DB_COLUMN_BITMAP known_not_nullable_map;
    i4	   err_code;
    DB_ERROR	   *err_blk = &psq_cb->psq_error;
    i2		   dummy;

    /* set bit flagging this as a check constraint
     */
    integ_tup  = cr_integ->pst_integrityTuple;
    integ_tup->dbi_consflags |= CONS_CHECK;
    
    /* zero out bit map for known-not-nullable columns
     */
    MEfill(sizeof(DB_COLUMN_BITMAP), 
	   (u_char) 0, (PTR) &known_not_nullable_map);


    if (cons->pss_cons_type & PSS_CONS_NOT_NULL)
    {
	/* have a NOT NULL column, which is an IMPLICIT constraint
	** because it is really implemented via a non-nullable datatype in DMF
	*/
	integ_tup->dbi_consflags |= (CONS_NON_NULLABLE_DATATYPE
				                | CONS_KNOWN_NOT_NULLABLE);

	if (cons->pss_check_cons_cols == (DB_COLUMN_BITMAP *) NULL)
	{
	    /* find the column number for this column,
	    ** and set the bit for that column number in the bitmap
	    */
	    status = psl_ver_cons_columns(sess_cb, ERx("CHECK"), qmode,
					  &cons->pss_cons_colq,
					  dmu_cb, rngvar, TRUE, TRUE,
					  &known_not_nullable_map,
					  (DMT_ATT_ENTRY**) NULL, 
					  &cr_integ->pst_cons_collist, 
					  &dummy, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	else
	{
	    /* the bitmap was set up in the CREATE TABLE AS SELECT routine
	    ** (since don't have column names stored in the dmu_cb for CTAS,
	    **  we can't use psl_ver_cons_columns to find the column number);
	    ** just copy the bitmap here
	    */
	    STRUCT_ASSIGN_MACRO(*cons->pss_check_cons_cols,
				known_not_nullable_map);
	}

	/* set the same bit in the 1st tuple
	 */
	STRUCT_ASSIGN_MACRO(known_not_nullable_map, integ_tup->dbi_columns);
	
    }  /* end if PSS_CONS_NOT_NULL */
    else 
    {
	/*
	** we have an EXPLICIT constraint
	*/
	
	if (newtbl || (qmode == PSQ_ATBL_ADD_COLUMN) )
	{
	    PSL_P_TELEM_CTX ctx;
	    /*
	    ** constraint was specified inside CREATE TABLE statement.
	    */

	    /* While parsing the <search condition> we postponed looking up
	    ** attribute descriptions and performing type resolution until such
	    ** time as the information on all attributes of the new table is
	    ** available.  Now's the time, so process the prototype condition
	    ** tree looking up attribute definitions in DMU_CB and performing
	    ** type resolution as appropriate.
	    */
	    if (cons->pss_check_cond == (PSS_TREEINFO *) NULL)
	    {
		/* should never happen */
		(VOID) psf_error(E_PS0486_INVALID_CONS_TYPE, 0L, 
				 PSF_INTERR, &err_code, err_blk, 0);

		return (E_DB_ERROR);
	    }

	    /*
	    ** call psl_p_telem() to walk the prototype tree created to
	    ** represent the <search condition> and resolve attribute references
	    ** and perform type resolution on op-nodes.  Tell psl_p_telem() that
	    ** it is being given a tree representing <search condition> of a
	    ** CHECK constraint so that it will "know" to process PST_VAR nodes
	    ** differently from PST_VAR nodes found in a prototype tree created
	    ** for <target list> of a subselect
	    */
	    sess_cb->pss_stmt_flags |= PSS_RESOLVING_CHECK_CONS;

	    ctx.cb = sess_cb;
	    ctx.psq_cb = psq_cb;
	    ctx.yyvarsp = NULL;
	    ctx.rngtable = NULL;
	    ctx.link = NULL;
	    ctx.resolved_fwdvar = NULL;
	    ctx.unresolved_fwdvar = NULL;
	    ctx.xform_avg = 0;
	    status = psl_p_telem(&ctx, &cons->pss_check_cond->pss_tree, NULL);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    if (ctx.unresolved_fwdvar)
	    {
		(void) psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR,
				 &err_code, &psq_cb->psq_error, 0);
	    }

	    /* reset PSS_RESOLVING_CHECK_CONS bit to avoid possible confusion */
	    sess_cb->pss_stmt_flags &= ~PSS_RESOLVING_CHECK_CONS;
	}
	else
	{
	    /*
	    ** we have a CHECK constraint specified using ALTER TABLE
	    */

	    /* return error if TID column was used in this constraint
	     */
	    if (BTtest(DB_IMTID, (char *) cons->pss_check_cons_cols))
	    {
		(void) psf_error(3496L, 0L, PSF_USERERR, 
				 &err_code, err_blk, 0);
		return (E_DB_ERROR);
	    }

	    /*
	    ** if adding a CHECK constraint on an existing table in a database 
	    ** which is being journaled, we need to verify that journaling is 
	    ** enabled on the table itself - if not, we want to prevent user
	    ** from creating this constraint because if it should become 
	    ** necessary to employ ROLLFORWARDDB before the next checkpoint is 
	    ** taken, database may be left in the state where the data in the 
	    ** table no longer satisfies the constraint being created
	    **
	    ** user may choose to override this restriction by specifying 
	    ** WITH NOJOURNAL_CHECK in which case we will send a warning 
	    ** message to the error log
	    */
	    if (sess_cb->pss_ses_flag & PSS_JOURNALED_DB)
	    {
		if (~rngvar->pss_tabdesc->tbl_status_mask & DMT_JNL)
		{
		   char		*cons_str = ERx("CHECK");

	    	    if (~cons->pss_cons_flags & PSS_NO_JOURNAL_CHECK)
		    {
		        (void) psf_error(E_PS04AC_CONS_NON_JOR_TBL_JOR_DB, 0L,
				      PSF_USERERR, &err_code, err_blk, 3,
				      STlength(cons_str), (PTR) cons_str,
				      psf_trmwhite(sizeof(rngvar->pss_tabname),
				    	      (char *) &rngvar->pss_tabname),
				      (PTR) &rngvar->pss_tabname,
				      psf_trmwhite(sizeof(rngvar->pss_ownname),
				    	      (char *) &rngvar->pss_ownname),
				      (PTR) &rngvar->pss_ownname);
		        return(E_DB_ERROR);
		    }
		    else
		    {
			/*
			** we want the error to be sent only to the error log -
			** this requires that we set error type to 
			** "internal error", but we do not want to return 
			** "internal error" to the caller, so we must remember 
			** to reset err_blk->err_code
			*/
		        (void) psf_error(W_PS04B0_CONS_NON_JOR_TBL_JOR_DB, 0L,
				      PSF_INTERR, &err_code, err_blk, 3,
				      STlength(cons_str), (PTR) cons_str,
				      psf_trmwhite(sizeof(rngvar->pss_tabname),
				    	      (char *) &rngvar->pss_tabname),
				      (PTR) &rngvar->pss_tabname,
				      psf_trmwhite(sizeof(rngvar->pss_ownname),
				    	      (char *) &rngvar->pss_ownname),
				      (PTR) &rngvar->pss_ownname);

			err_blk->err_code = E_PS0000_OK;
		    }
		}
	    }

	    /* copy map of attributes referenced in the <search condition> */
	    STRUCT_ASSIGN_MACRO((*cons->pss_check_cons_cols),
				integ_tup->dbi_columns);
	}

	for (;;)  /* give us something to break out of */
	{
	    PST_COL_ID	    *col;
	    i4		    cons_col_id, col_id;
	    i4		    num_cols;

	    /* initialize the list of PST_COL_ID's.
	    ** We will generate the list in the 'for' loop below
	    */
	    cr_integ->pst_cons_collist = (PST_COL_ID *) NULL;

	    /* count the number of columns set in the bitmap,
	    ** and allocate that many col_id structures.
	    ** They will be filled in by the loop below.
	    **
	    ** Note: it is possible (though silly) for a CHECK condition to
	    ** have no columns in it; for example CHECK ('now' > '01011994').
	    ** We handle that case by leaving the col_id list NULL,
	    ** and skipping the list building code below.
	    */
	    num_cols = BTcount((char *) &integ_tup->dbi_columns, DB_COL_BITS);
	    if (num_cols == 0)
		break;
	    
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
				num_cols * sizeof(PST_COL_ID),
				(PTR *) &col, err_blk);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    /* if a column constraint, find the id of the column it was
	    ** defined on, as the condition can only contain that column and
	    ** make sure that no other attributes of the table have been
	    ** referenced in the <search condition>
	    */
	    if (cons->pss_cons_type & PSS_CONS_COL)
	    {
		PSY_COL	    *col_descr;
		
		col_descr = (PSY_COL *) cons->pss_cons_colq.q_next;

		cons_col_id = psl_find_column_number(dmu_cb, 
						     &col_descr->psy_colnm);
	    }
	    
	    /*
	    ** generate a list of PST_COL_ID's describing attributes referenced
	    ** inside the <search condition>; if processing a column constraint,
	    ** make sure that only the attribute for which the constraint was
	    ** defined is referenced in the <search condition>
	    */
	    for (col_id = -1;
		 (col_id = BTnext(col_id, (char *) &integ_tup->dbi_columns,
				  DB_MAX_COLS + 1)) != -1;
		)
	    {
		if (   (cons->pss_cons_type & PSS_CONS_COL)
		    && (col_id != cons_col_id))
		{
		    char	    *tabname;
		    char	    command[PSL_MAX_COMM_STRING];
		    i4	    length;
		    DMF_ATTR_ENTRY  **attrs =
			(DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;

		    /*
		    ** column-level constraint can only be specified using
		    ** CREATE TABLE statement, so look in dmu_cb for info.
		    */

		    tabname = dmu_cb->dmu_table_name.db_tab_name;
		    psl_command_string(psq_cb->psq_mode, sess_cb, command,
			&length);

		    _VOID_ psf_error(E_PS0472_COL_CHECK_CONSTRAINT, 0L,
			PSF_USERERR, &err_code, err_blk, 3,
			length, command,
			psf_trmwhite(sizeof(DB_ATT_NAME),
			    attrs[cons_col_id-1]->attr_name.db_att_name),
			attrs[cons_col_id-1]->attr_name.db_att_name,
			psf_trmwhite(sizeof(DB_TAB_NAME), tabname), tabname);
		    return(E_DB_ERROR);
		}
		
		col->col_id.db_att_id = col_id;
		col->next             = cr_integ->pst_cons_collist;
		cr_integ->pst_cons_collist = col;
		col++;

	    }  /* end of for (col_id) */

	    break;

	}  /* end of for (;;) */

	/* store text to be used to create WHERE clause
	** of rule implementing constraint
	*/
	cr_integ->pst_checkRuleText = cons->pss_check_rule_text;
	

	/* check if any columns are made known-not-nullable
	 */
	/***** This check has been postponed to a future release;
	****** we do not have to support known-not-nullable until we
	****** support the SQL92 INFO_SCHEMA.      rjb  28-dec-1992
	*****/

    }  /* end else EXPLICIT constraint */


    if (integ_tup->dbi_consflags & CONS_KNOWN_NOT_NULLABLE)
    {
	/* have at least one known-not-nullable column,
	** so need a 2nd tuple for this constraint (which will be 
	** identical to the 1st except for the map and sequence)
	** Malloc it, copy the first tuple, and set
	** the map and sequence to new values.
	*/
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			    sizeof(DB_INTEGRITY), (PTR *) &integ_tup2, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	
	MEcopy((PTR) integ_tup, sizeof(DB_INTEGRITY), (PTR) integ_tup2);

	cr_integ->pst_knownNotNullableTuple = integ_tup2;

	integ_tup2->dbi_consflags |= CONS_KNOWN_NOT_NULLABLE_TUPLE;
	integ_tup2->dbi_seq = 1L;
	STRUCT_ASSIGN_MACRO(known_not_nullable_map, integ_tup2->dbi_columns);
    }
    
    return (E_DB_OK);

}  /* end psl_ver_check_cons */


static DB_STATUS
psl_check_unique_set(
		     DB_INTEGRITY *integ_tup1,
		     DB_INTEGRITY *integ_tup2,
		     i4	  name1_specified,
		     i4	  name2_specified,
		     i4	  qmode,
		     PSS_SESBLK *sess_cb,
		     DB_ERROR     *err_blk)
{
    char	  command[PSL_MAX_COMM_STRING];
    i4	  length;
    i4	  err_code;

    /* if both constraints are UNIQUE constraints, 
    ** make sure set of columns are distinct (i.e. not identical)
    */
    if (MEcmp((PTR) &integ_tup1->dbi_columns,
	      (PTR) &integ_tup2->dbi_columns, sizeof(DB_COLUMN_BITMAP))  == 0)
    {
	psl_command_string(qmode, sess_cb, command, &length);

	if ((name1_specified) && (name2_specified))
	{
	    _VOID_ psf_error(E_PS047D_2UNIQUE_CONS_CONFLICT, 0L,
			     PSF_USERERR, &err_code, err_blk, 3,
			     length, command,
			     psf_trmwhite(sizeof(DB_CONSTRAINT_NAME),
				   integ_tup1->dbi_consname.db_constraint_name),
			     integ_tup1->dbi_consname.db_constraint_name,
			     psf_trmwhite(sizeof(DB_CONSTRAINT_NAME),
				   integ_tup2->dbi_consname.db_constraint_name),
			     integ_tup2->dbi_consname.db_constraint_name);
	}
	else if (name1_specified)
	{
	    _VOID_ psf_error(E_PS047E_1UNIQUE_CONS_CONFLICT, 0L, 
			     PSF_USERERR, &err_code, err_blk, 2,
			     length, command,
			     psf_trmwhite(sizeof(DB_CONSTRAINT_NAME),
				   integ_tup1->dbi_consname.db_constraint_name),
			     integ_tup1->dbi_consname.db_constraint_name);
	}
	else if (name2_specified)
	{
	    _VOID_ psf_error(E_PS047E_1UNIQUE_CONS_CONFLICT, 0L, PSF_USERERR,
			     &err_code, err_blk, 2,
			     length, command,
			     psf_trmwhite(sizeof(DB_CONSTRAINT_NAME),
				   integ_tup2->dbi_consname.db_constraint_name),
			     integ_tup2->dbi_consname.db_constraint_name);
	}
	else
	{
	    _VOID_ psf_error(E_PS047F_0UNIQUE_CONS_CONFLICT, 0L, PSF_USERERR,
			     &err_code, err_blk, 1,
			     length, command);
	}
	return (E_DB_ERROR);

    }  /* end if (MEcmp == 0) */

    return(E_DB_OK);
   
}  /* end psl_check_unique_set */
	    

/*
** Name: psl_compare_multi_cons  - check for conflicts between constraints
**				   in a CREATE TABLE statement
**
** Description:	    
**          Since a single CREATE TABLE can contain many constraints,
**          we need to check that none of them conflict with each other.
**          This routine makes some checks, and returns an error if
**              a) two constraints have the same name;
**              b) two unique constraints are defined on the
**                 exact same set of columns; or
**              c) if a unique constraint is defined on a column that is
**                 declared as nullable  (per Entry Level SQL92).
**      
** Inputs:
**	stmt_list	ptr to list of CREATE CONSTRAINT statement nodes
**	dmu_cb		information about table (i.e. name and attribute info)
**	qmode		query mode (used for error messages)
**
** Outputs:
**	err_blk		filled in if an error occurs
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** Side effects:
**	none
**
** History:
**	20-nov-92 (rblumer)
**	    written
**	18-apr-94 (rblumer)
**	    collapsed separate list traversals into 1 loop,
**	    so now check for nullable columns in same loop
**	    in which other checks are done.
**	2-apr-98 (inkdo01)
**	    Changes to support constraint definition index options.
**	7-june-00 (inkdo01)
**	    Changes to get shared index name right (when one of the constraints 
**	    is UNIQUE).
*/
static DB_STATUS
psl_compare_multi_cons(
		      PSS_SESBLK	*sess_cb,
		      PST_STATEMENT	**stmt_list,
		      DMU_CB		*dmu_cb,
		      i4		qmode,
		      DB_ERROR		*err_blk)
{
    DB_STATUS 	   status;
    i4	   err_code;
    PST_STATEMENT  *stmt1, 
                   *stmt2,
		   **stmtx;
    PST_CREATE_INTEGRITY *cr_integ1, 
                         *cr_integ2;
    DB_INTEGRITY   *integ_tup1, 
                   *integ_tup2;
    i4             name1_spec,     /* was integ1's name specified? */
                   name2_spec;     /* was integ2's name specified? */
    i4             num_cols, 
                   colno;
    DMF_ATTR_ENTRY **attrs;
    char           command[PSL_MAX_COMM_STRING];
    i4        length;
    i4		   init_null_cols = FALSE;
    bool	   longer_ref;
    DB_COLUMN_BITMAP null_cols;
    DB_COLUMN_BITMAP empty_cols;
    DB_COLUMN_BITMAP tst_cols;

    
    /* check that no two UNIQUE constraints use the same columns
    ** AND that no two constraints have the same constraint name
    */
    for (stmt1 = *stmt_list;
	 stmt1 != (PST_STATEMENT *) NULL;
	 stmt1 = stmt1->pst_next)
    {
	cr_integ1  = &stmt1->pst_specific.pst_createIntegrity;
	integ_tup1 = cr_integ1->pst_integrityTuple;
	name1_spec = cr_integ1->pst_createIntegrityFlags & PST_CONS_NAME_SPEC;
	longer_ref = FALSE;

	/* First see if an explicit index name was supplied. If so, 
	** a check must be made for other constraints using the same index
	** and such a circumstance must be validated.
	*/

	if (cr_integ1->pst_createIntegrityFlags & PST_CONS_NEWIX)
	{
	    PST_CREATE_INTEGRITY	*xinteg;
	    bool	firstun = TRUE, firstref = TRUE;

	    /* Loop over preceding constraints, looking for match. */
	    for (stmt2 = *stmt_list; stmt2 != stmt1;
					stmt2 = stmt2->pst_next)
	    {
		xinteg = &stmt2->pst_specific.pst_createIntegrity;
		if (!(MEcmp((PTR)&xinteg->pst_indexres.pst_resname,
			(PTR)&cr_integ1->pst_indexres.pst_resname,
			sizeof(DB_TAB_NAME)) == 0)) continue;

		if (psl_verify_new_index(sess_cb, cr_integ1, xinteg))
		{
		    cr_integ1->pst_createIntegrityFlags &= ~PST_CONS_NEWIX;
		    cr_integ1->pst_createIntegrityFlags |= PST_CONS_OLDIX;
		    xinteg->pst_createIntegrityFlags |= PST_CONS_SHAREDIX;

		    /* The next bit of code assures that UNIQUE/PRIMARY KEY
		    ** constraints in a shared index group are processed first.
		    ** This is required for the "unique" attribute to be
		    ** assigned to the index.
		    **
		    ** If all constraints in the group are referential, we
		    ** must also assure that the one with the most keys
		    ** (which effectively defines the index) is the first
		    ** to be processed. 
		    **
		    ** In both cases (unique & referential) the flags are
		    ** reset here and the positions are swapped at the end 
		    ** of the big loop. 
		    */
		    if (xinteg->pst_integrityTuple->dbi_consflags & CONS_UNIQUE)
							firstun = FALSE;
		    else if (firstun && 
			cr_integ1->pst_integrityTuple->dbi_consflags & CONS_UNIQUE)
		    {
			firstun = FALSE;
			cr_integ1->pst_createIntegrityFlags &= ~PST_CONS_OLDIX;
			cr_integ1->pst_createIntegrityFlags |= 
					(PST_CONS_NEWIX | PST_CONS_SHAREDIX);
			xinteg->pst_createIntegrityFlags &= 
					~(PST_CONS_NEWIX | PST_CONS_SHAREDIX);
			xinteg->pst_createIntegrityFlags |= PST_CONS_OLDIX;
		    }
		    if (firstun && firstref && 
			cr_integ1->pst_integrityTuple->dbi_consflags & CONS_REF)
		    {
			firstref = FALSE;
			if (cr_integ1->pst_key_count > xinteg->pst_key_count)
			{
			    cr_integ1->pst_createIntegrityFlags &= ~PST_CONS_OLDIX;
			    cr_integ1->pst_createIntegrityFlags |= 
					(PST_CONS_NEWIX | PST_CONS_SHAREDIX);
			    xinteg->pst_createIntegrityFlags &= 
					~(PST_CONS_NEWIX | PST_CONS_SHAREDIX);
			    xinteg->pst_createIntegrityFlags |= PST_CONS_OLDIX;
			    longer_ref = TRUE;
			}
		    }
		}
		else
		{
		    (void) psf_error(E_PS048D_CONS_BADINDEX, 0L, PSF_USERERR,
				&err_code, err_blk, 1,
				psf_trmwhite(sizeof(DB_TAB_NAME),
				  (PTR)&cr_integ1->pst_indexres.pst_resname),
				&cr_integ1->pst_indexres.pst_resname);
		    return(E_DB_ERROR);
		}
	    }
	}	/* end of index name check */

	/* if this is not a unique constraint or a named constraint,
	** no need to check (so continue)
	*/
	if ((~integ_tup1->dbi_consflags & CONS_UNIQUE) && (!name1_spec) &&
		(!longer_ref)) continue;

	for (stmt2 = stmt1->pst_next;
	     stmt2 != (PST_STATEMENT *) NULL;
	     stmt2 = stmt2->pst_next)
	{
	    cr_integ2  = &stmt2->pst_specific.pst_createIntegrity;
	    integ_tup2 = cr_integ2->pst_integrityTuple;
	    name2_spec = 
		cr_integ2->pst_createIntegrityFlags & PST_CONS_NAME_SPEC;


	    /* if constraint name specified in both constraints, compare them
	     */
	    if (name1_spec && name2_spec)
	    {
		if (!MEcmp(integ_tup1->dbi_consname.db_constraint_name,
			   integ_tup2->dbi_consname.db_constraint_name,
			   sizeof(DB_CONSTRAINT_NAME)))
		{
		    psl_command_string(qmode, sess_cb, command, &length);

		    _VOID_ psf_error(E_PS047C_DUP_CONS_NAME, 0L, PSF_USERERR,
				 &err_code, err_blk, 2,
				 length, command,
				 psf_trmwhite(sizeof(DB_CONSTRAINT_NAME),
				   integ_tup2->dbi_consname.db_constraint_name),
				 integ_tup2->dbi_consname.db_constraint_name);
		    return (E_DB_ERROR);
		}
	    }  /* end if (name1_spec && name2_spec) */
	

	    /* if both constraints are UNIQUE constraints, 
	    ** make sure columns are distinct (i.e. not identical)
	    ** and that they are not both primary keys
	    */
	    if (   (integ_tup1->dbi_consflags & CONS_UNIQUE)
		&& (integ_tup2->dbi_consflags & CONS_UNIQUE))
	    {
		status = psl_check_unique_set(integ_tup1, integ_tup2,
					      name1_spec, name2_spec,
					      qmode, sess_cb, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);

		/* can't have two primary key constraints on same table
		 */
		if (   (integ_tup1->dbi_consflags & CONS_PRIMARY)
		    && (integ_tup2->dbi_consflags & CONS_PRIMARY))
		{
		    (void) psf_error(E_PS0493_DUPL_PRIMARY_KEY, 0L, PSF_USERERR,
				     &err_code, err_blk, 1,
				     psf_trmwhite(sizeof(DB_TAB_NAME),
					   dmu_cb->dmu_table_name.db_tab_name),
				     dmu_cb->dmu_table_name.db_tab_name);
		    return (E_DB_ERROR);
		}
		
	    } /* end if both CONS_UNIQUE */

	}  /* end for stmt2 */

	/* now check to make sure that unique constraints
	** don't have any nullable columns in them.
	**   NOTE: "CHECK (c is not null)" is not good enough
	**   (per Entry Level SQL92; but remove this check for Intermed. Level)
	*/
	if (longer_ref) goto stmt_swap;
	if (~integ_tup1->dbi_consflags & CONS_UNIQUE)
	    continue;

	/* init null_cols data structure, if necessary
	 */
	if (!init_null_cols)
	{
	    /* zero out null_cols and empty_cols bitmaps
	     */
	    MEfill(sizeof(DB_COLUMN_BITMAP), 
		   (u_char) 0, (PTR) &null_cols);
	    MEfill(sizeof(DB_COLUMN_BITMAP), 
		   (u_char) 0, (PTR) &empty_cols);

	    attrs    = (DMF_ATTR_ENTRY**) dmu_cb->dmu_attr_array.ptr_address;
	    num_cols = dmu_cb->dmu_attr_array.ptr_in_count;
	
	    /* make a bitmap of the nullable columns in the table,
	    ** to be used for checking all the unique constraints
	    ** (note we have to adjust the colno, as the dmu_cb starts at 0,
	    **  but bitmaps start counting columns at 1)
	    */
	    for (colno = 0; colno < num_cols ; colno++)
	    {
		if (attrs[colno]->attr_type < 0)
		    BTset(colno+1, (char *) &null_cols);
	    
	    }  /* end for (colno < num_cols) */

	    init_null_cols = TRUE;
	}
	
	/* make copy of column bitmap in the unique constraint; 
	** will be used to verify that all columns are declared NOT NULL
	*/
	MEcopy((PTR) &integ_tup1->dbi_columns,
	       sizeof(tst_cols), (PTR) &tst_cols);

	/* Do a logical AND of the null_cols and unique columns;
	** the result should be zero (no matching columns)
	*/
	BTand(DB_COL_BITS, (char *) &null_cols, (char *) &tst_cols);

	if (MEcmp((PTR) &tst_cols, (PTR) &empty_cols, sizeof(tst_cols)))
	{
	    char *tabname = stmt1->pst_specific.pst_createIntegrity
					.pst_cons_tabname.db_tab_name;

	    psl_command_string(qmode, sess_cb, command, &length);

	    (void) psf_error(E_PS0480_UNIQUE_NOT_NULL, 0L, PSF_USERERR,
			     &err_code, err_blk, 2,
			     length, command,
			     psf_trmwhite(DB_TAB_MAXNAME, tabname), tabname);

	    if (name1_spec)
	    {
		(void) psf_error(E_PS0494_CONS_NAME, 0L, PSF_USERERR, 
				 &err_code, err_blk, 1,
				 psf_trmwhite(DB_CONS_MAXNAME,
				   integ_tup1->dbi_consname.db_constraint_name),
				 integ_tup1->dbi_consname.db_constraint_name);
	    }
	    return (E_DB_ERROR);

	}  /* end if MEcmp */

stmt_swap: 
	/* One last check - of this is unique constraint with explicit index
	** name (in a shared index group) or a referential constraint with 
	** more keys than all others (so far) in a shared index group,
	** stick it at head of constraint statement list to assure
	** proper index definition. Flags in the swapped constraints must
	** also be changed, but that was done earlier in the loop when the
	** need to swap was first detected. */
	if ((longer_ref || integ_tup1->dbi_consflags & CONS_UNIQUE) &&
	    cr_integ1->pst_createIntegrityFlags & PST_CONS_SHAREDIX)
	{
	    for (stmtx = stmt_list; (*stmtx)->pst_next != stmt1;
				stmtx = &(*stmtx)->pst_next);

	    {
		(*stmtx)->pst_next = (*stmtx)->pst_link = stmt1->pst_next;
		stmt1->pst_next = stmt1->pst_link = *stmt_list;
		*stmt_list = stmt1;
		stmt1 = *stmtx;		/* reset stmt1 for big loop */
	    }
	}

    }      /* end for stmt1 */

    return(E_DB_OK);

} /* end psl_compare_multi_cons */


/*
** Name: psl_compare_agst_table  - check for conflicts between a new constraint
**				   and existing constraints on a table
**
** Description:	    
**          Compares a constraint against existing constraints on an
**          EXISTING table; only comparisons currently needed are:
**          	-columns in a unique constraint are not-null datatypes
**          	-unique constraint does not apply to the exact same set of
**          	 columns as another unique constraint on the same table
**
**          (NOTE: constraint names will be checked in psl_compare_agst_schema)
**      
**          Used for ALTER TABLE (which can contain only 1 constraint
**          definition).
**
** Inputs:
**	sess_cb		ptr to PSF session CB (passed to other routines)
**	stmt		info about constraint (CREATE CONSTRAINT statement node)
**	rngvar		information about table (i.e. name and attribute info)
**	qmode		query mode (used for error messages)
**
** Outputs:
**	err_blk		filled in if an error occurs
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** Side effects:
**	none
**
** History:
**	20-nov-92 (rblumer)
**	    written
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	8-apr-98 (inkdo01)
**	    Changes to support "with index" option of add constraint.
*/
static DB_STATUS
psl_compare_agst_table(
		       PSS_SESBLK	*sess_cb,
		       PST_STATEMENT	*stmt,
		       PSS_RNGTAB	*rngvar,
		       i4		qmode,
		       DB_ERROR		*err_blk)
{
    DB_COLUMN_BITMAP     tst_cols;
    DMT_ATT_ENTRY        **attrs;
    PST_CREATE_INTEGRITY *cr_integ;
    DB_INTEGRITY  *integ_tup;
    i4		  name_specified;
    i4		  colno,
                  num_cols;
    i4	  err_code;
    DB_STATUS	  status = E_DB_OK;
    char          command[PSL_MAX_COMM_STRING];
    i4       length;
    i4		  dummy;
    PTR		  qual_args[5];
    
    cr_integ  = &stmt->pst_specific.pst_createIntegrity;
    integ_tup = cr_integ->pst_integrityTuple;

    /* If constraint contained "with index = " option, must
    ** check for existing index or base table structure and
    ** verify key structure to be suitable for the constraint. */

    if (cr_integ->pst_createIntegrityFlags &
		(PST_CONS_NEWIX | PST_CONS_BASEIX))
    {
	i4	keycount;
	i4	*keyarray;
	DB_TAB_NAME *err_name;
	RDR_INFO *rdrinfo = sess_cb->pss_resrng->pss_rdrinfo;
	bool	checkcols = FALSE;
	bool	badixopts = FALSE;

	if (cr_integ->pst_createIntegrityFlags &
					PST_CONS_BASEIX)
	{
	    if (rdrinfo->rdr_keys == NULL)
	    {
		/* Incompatible with "index = base table" */
		(void) psf_error(E_PS048F_CONS_NOBASE, 0L, 
			PSF_USERERR, &err_code, err_blk, 0);
		return(E_DB_ERROR);
	    }

	    checkcols = TRUE;
	    if (integ_tup->dbi_consflags & CONS_UNIQUE &&
		!(rdrinfo->rdr_rel->tbl_2_status_mask &
		DMT_STATEMENT_LEVEL_UNIQUE)) badixopts = TRUE;
	    keycount = rdrinfo->rdr_keys->key_count;
	    keyarray = rdrinfo->rdr_keys->key_array;
	    err_name = &rdrinfo->rdr_rel->tbl_name;
	}
	else	/* must be secondary index */
	{
	    i4	i;

	    /* Loop over indexes looking for match. If not
	    ** found, named index will be created specifically
	    ** for constraint. */
	    for (i = 0; i < rdrinfo->rdr_no_index; i++)
	     if (MEcmp((PTR)&cr_integ->pst_indexres.pst_resname,
		(PTR)&rdrinfo->rdr_indx[i]->idx_name,
		sizeof(DB_TAB_NAME)) == 0)
	    {
		checkcols = TRUE;
		break;
	    }
	    if (checkcols)
	    {
		if (integ_tup->dbi_consflags & CONS_UNIQUE &&
		    !(rdrinfo->rdr_indx[i]->idx_status &
		    DMT_I_STATEMENT_LEVEL_UNIQUE)) badixopts = TRUE;
		if (!(rdrinfo->rdr_indx[i]->idx_status &
		    DMT_I_PERSISTS_OVER_MODIFIES)) badixopts = TRUE;
		keycount = rdrinfo->rdr_indx[i]->idx_key_count;
		keyarray = &rdrinfo->rdr_indx[i]->idx_attr_id[0];
		err_name = &cr_integ->pst_indexres.pst_resname;
	    }
	}

	if (checkcols)
	 if (badixopts)
	{
	    /* Index/base table doesn't have needed "persistence, 
	    ** unique_scope = statement" combination. */
	    (void) psf_error(E_PS0497_CONS_IXOPTS, 0L, 
			PSF_USERERR, &err_code, err_blk, 0);
	    return(E_DB_ERROR);
	}
	 else if (integ_tup->dbi_consflags & CONS_UNIQUE &&
		keycount > cr_integ->pst_key_count ||
	 	!psl_verify_existing_index(cr_integ, keycount,
			keyarray))
	{
	    /* Found index/structure to verify, and it didn't. */
	    (void) psf_error(E_PS048D_CONS_BADINDEX, 0L, PSF_USERERR,
			&err_code, err_blk, 1,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			  (PTR)err_name), err_name);
	    return(E_DB_ERROR);
	}
	 else if (cr_integ->pst_createIntegrityFlags & PST_CONS_NEWIX)
	{
	    /* Selected index is ok. Reset flag. */
	    cr_integ->pst_createIntegrityFlags &= ~PST_CONS_NEWIX;
	    cr_integ->pst_createIntegrityFlags |= PST_CONS_OLDIX;
	}
    }

    /* from here, only need to do checks for UNIQUE constraints;
    ** if not a UNIQUE constraint, return immediately
    */
    if (~integ_tup->dbi_consflags & CONS_UNIQUE)
	return(E_DB_OK);
    
    name_specified = cr_integ->pst_createIntegrityFlags & PST_CONS_NAME_SPEC;
    attrs    = rngvar->pss_attdesc;
    num_cols = rngvar->pss_tabdesc->tbl_attr_count;
    
    /* make copy of column bitmap in the unique constraint; 
    ** will be used to verify that all columns are declared NOT NULL
    */
    MEcopy((PTR) &integ_tup->dbi_columns,
	   sizeof(tst_cols), (PTR) &tst_cols);
    
    /* Check that all unique columns have not null datatype.
    ** for each not-null column, clear its bit in the unique column bitmap
    **  (note: RDF uses attrs[0] for the "tid" column, so we don't look at it)
    */
    for (colno = 1; colno <= num_cols ; colno++)
    {
	if (attrs[colno]->att_type > 0)
	    BTclear(colno, (char *) &tst_cols);
    }
    
    /* if all columns in the unique constraint had NOT NULL set,
    ** then all bits in the map should have been turned off
    */
    if (BTcount((char *) &tst_cols, DB_COL_BITS) != 0)
    {
	char *tabname = cr_integ->pst_cons_tabname.db_tab_name;

	psl_command_string(qmode, sess_cb, command, &length);

	(void) psf_error(E_PS0480_UNIQUE_NOT_NULL, 0L, PSF_USERERR,
			 &err_code, err_blk, 2,
			 length, command,
			 psf_trmwhite(DB_TAB_MAXNAME, tabname), tabname);

	if (name_specified) 
	{
	    (void) psf_error(E_PS0494_CONS_NAME, 0L, PSF_USERERR, 
			     &err_code, err_blk, 1,
			     psf_trmwhite(DB_CONS_MAXNAME,
				   integ_tup->dbi_consname.db_constraint_name),
			     integ_tup->dbi_consname.db_constraint_name);
	}
	return (E_DB_ERROR);
    }

    /*
    **  Scan integrity catalog for other unique constraints on this table,
    **  and make sure their set of columns is not identical to this constraint
    */
    qual_args[0] = (PTR) integ_tup;
    qual_args[1] = (PTR) rngvar;
    qual_args[2] = (PTR) &name_specified;
    qual_args[3] = (PTR) &qmode;
    qual_args[4] = (PTR) err_blk;

    status = psl_find_cons(sess_cb, rngvar, psl_qual_dup_cons, qual_args,
	(DB_INTEGRITY *) NULL, &dummy, err_blk);

    return(status);

}  /* end psl_compare_agst_table */

/*
** Name: psl_qual_dup_cons  - determine whether a tuple represents a 
**		    	      UNIQUE or PRIMARY KEY constraints which conflicts
**			      with a constraint being created
**
** Description:	    
**          Address of this function will be passed to pss_find_cons() by 
**	    callers interested in whether there exists a constraint which would
**	    conflict with a given constraint.  If the tuple represents a 
**	    conflicting constraint, function will return E_DB_ERROR, otherwise 
**	    it will return E_DB_OK.  *dummy will not be changed
**      
** Inputs:
**	qual_args	args to this function
**	    [0]		integrity tuple describing a constraint for which we
**			want to ascertain that there are no existing constraints
**		 	which would conflict with it
**	    [1]		range table entry describing a table on which 
**			constraints in question are defined
**	    [2]		address of indicator of whether the integrity tuple 
**			pointed to by qual_args[0] contains integrity name
**	    [3]		address of a var containing mode of the query
**	integ		IIINTEGRITY tuple to be considered
**
** Outputs:
**	qual_args       args to this function
**	    [4]		address of error block - will be filled in if a 
**			duplicate constraint is encountered
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	none
**
** History:
**	09-jan-94 (andre)
**	    written
**	15-Oct-2010 (kschendel) SIR 124544
**	    Update psl-command-string call.
*/
static DB_STATUS
psl_qual_dup_cons(
		  PTR		*qual_args,
		  DB_INTEGRITY	*integ,
		  i4		*dummy,
		  PSS_SESBLK	*sess_cb)
{
    i4		err_code;
    DB_STATUS		status = E_DB_OK;
    DB_INTEGRITY	*new_cons	= (DB_INTEGRITY *) qual_args[0];
    PSS_RNGTAB		*rngvar		= (PSS_RNGTAB *)   qual_args[1];
    i4			name_specified	= *((i4 *)   	   qual_args[2]);
    i4			qmode		= *((i4 *)   	   qual_args[3]);
    DB_ERROR		*err_blk	= (DB_ERROR *)     qual_args[4];

    /* 
    ** only look at UNIQUE constraints
    */
    if (integ->dbi_consflags & CONS_UNIQUE)
    {
        /* can't have two primary key constraints on same table
	 */
	if (new_cons->dbi_consflags & integ->dbi_consflags & CONS_PRIMARY)
	{
	    (void) psf_error(E_PS0493_DUPL_PRIMARY_KEY, 0L, PSF_USERERR,
			     &err_code, err_blk, 1,
			     psf_trmwhite(sizeof(DB_TAB_NAME), 
				      rngvar->pss_tabname.db_tab_name),
			     rngvar->pss_tabname.db_tab_name);
	    status = E_DB_ERROR;
	}
	else
	{
	    /* 
	    ** compare the set of unique columns, and return an error if they 
	    ** are identical
	    */
	    status = psl_check_unique_set(new_cons, integ, name_specified, TRUE,
		qmode, sess_cb, err_blk);
	}
    }

    return(status);
}


/*
** Name: psl_compare_agst_schema  - check for name conflicts between 
**				    constraints in the same schema
** 
** Description:	    
**          Compares a list of constraints for names that conflict with other
**          constraints in the same schema.  Returns an error if the name of a
**          constraint in the list is the same as an existing constraint.
**          
**          Used for CREATE TABLE and ALTER TABLE.
**
** Inputs:
**	sess_cb		ptr to PSF session CB (passed to other routines)
**	stmt_list	list of CREATE CONSTRAINT statement nodes
**	qmode		query mode (used for error messages)
**
** Outputs:
**	err_blk		filled in if an error occurs
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** Side effects:
**	none
**
** History:
**	20-nov-92 (rblumer)
**	    written
**	10-aug-93 (andre)
**	    fixed cause of compiler warnings
*/
static DB_STATUS
psl_compare_agst_schema(
			PSS_SESBLK	*sess_cb,
			PST_STATEMENT	*stmt_list,
			i4		qmode,
			DB_ERROR	*err_blk)
{
    DB_STATUS	   status;
    i4 	   err_code;
    RDF_CB	   rdf_cb;
    PST_STATEMENT  *stmt;
    PST_CREATE_INTEGRITY *cr_integ;
    DB_INTEGRITYIDX	 integidx;

    /* set up constant part of RDF control block
     */ 
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_cb.rdf_rb.rdr_2types_mask = RDR2_CNS_INTEG;
    rdf_cb.rdf_rb.rdr_qrytuple    = (PTR) &integidx;

    /* for each named constraint,
    ** check that the name is unique within the current schema
    */
    for (stmt = stmt_list;
	 stmt != (PST_STATEMENT *) NULL;
	 stmt = stmt->pst_next)
    {
	cr_integ = &stmt->pst_specific.pst_createIntegrity;

	/* if this is not a named constraint,
	** no need to check (so continue)
	*/
	if (~cr_integ->pst_createIntegrityFlags & PST_CONS_NAME_SPEC)
	    continue;

	/* call RDF to see if a constraint in the current schema
	** with the same name as the new constraints
	*/
	STRUCT_ASSIGN_MACRO(cr_integ->pst_integrityTuple->dbi_consname,
			    rdf_cb.rdf_rb.rdr_name.rdr_cnstrname);
	MEcopy(cr_integ->pst_integritySchemaName.db_schema_name,
	       DB_OWN_MAXNAME, rdf_cb.rdf_rb.rdr_owner.db_own_name);

	status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);

	/* can't use DB_FAILURE_MACRO because RDF returns a warning (E_DB_WARN)
	** for errors such as RD0011, and we want to reset the status in those
	** cases, too.
	*/
	if (status == E_DB_OK)
	{
	    /* if we found a constraint with the same name,
	    ** so return an error
	    */
	    (void) psf_error(E_PS0495_DUPL_CONS_NAME, 0L, PSF_USERERR, 
			     &err_code, err_blk, 2,
			     psf_trmwhite(DB_CONS_MAXNAME,
					  cr_integ->pst_integrityTuple->
			      		      dbi_consname.db_constraint_name),
			     cr_integ->pst_integrityTuple->
			         dbi_consname.db_constraint_name,
			     psf_trmwhite(DB_SCHEMA_MAXNAME,
			      cr_integ->pst_integritySchemaName.db_schema_name),
			     cr_integ->pst_integritySchemaName.db_schema_name);
	    return(E_DB_ERROR);
	}
	else 
	{
	    /* we got an RDF error;
	    ** check to see if it's the expected "not found" error
	    */
	    switch (rdf_cb.rdf_error.err_code)
	    {
	    case E_RD0011_NO_MORE_ROWS:
	    case E_RD0013_NO_TUPLE_FOUND:
		continue;
	
	    default:
		(VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb.rdf_error, err_blk);
		return(E_DB_ERROR);

	    } /* end switch rdf_cb.err_code */

	} /* end else got an error */
    
    }  /* end for each stmt */
    
    return(E_DB_OK);

} /* end psl_compare_agst_schema */


/*
** Name: psl_find_column_number  - finds column number given a column name
**
** Description:
** 	    Looks up the column name in the attribute array of the dmu_cb
** 	    passed in.
** 	    
**	    Returns the attribute ID of the column "col_name", if found;
**	    returns -1, if column "col_name" is not found.
**
**	    NOTE: column numbers start at 1, not 0
**      
** Inputs:
**	dmu_cb		DMU control block for CREATE TABLE
**	col_name	name of column
** 
** Outputs:    none
**	       
** Returns:
**	number of the column corresponding to 'col_name'.
**
**
** History:
**	22-feb-93 (rblumer)
**	    written
**	19-apr-94 (rblumer)
**	    changed dmu_cb to be a DMU_CB pointer instead of a PTR.
*/
i4
psl_find_column_number(
		   DMU_CB	*dmu_cb,
		   DB_ATT_NAME	*col_name)
{
    DMF_ATTR_ENTRY  **attrs;
    i4		    num_cols;
    i4		    colno;

    num_cols = dmu_cb->dmu_attr_array.ptr_in_count;
    attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;
	    
    for (colno = 0;
	 colno < num_cols;
	 colno++, attrs++)
    {
	if (!MEcmp((PTR) col_name,
		   (PTR) &((*attrs)->attr_name.db_att_name),
		   sizeof(DB_ATT_NAME)))
	{
	    return(colno+1);
	}
    }  /* end for colno = 0 */

    return(-1);

}  /* end psl_find_column_number */


/*
** Name: psl_verify_existing_index - confirms suitability of key structure
**	    to support constraint.
**
** Description:
** 	    Compares key list of supplied constraint with key list
**	    in parameter list (from some existing secondary index, or the
**	    base table structure itself) to determine their mutual 
**	    compatibility.
**      
** Inputs:
**	cinteg		PST_CREATE_INTEGRITY of current constraint
**	keycount	Count of attributes in key structure
**	keyarray	Array of IDs (column numbers) of columns in 
**			key structure
** 
** Outputs:    none
**	       
** Returns:
**	TRUE  - if key structure is satisfactory
**	FALSE - if key structure is NOT satisfactory
**
**
** History:
**	8-apr-98 (inkdo01)
**	    Written for constraint index options feature.
*/
static bool
psl_verify_existing_index(
	PST_CREATE_INTEGRITY	*cinteg,
	i4			keycount,
	i4			*keyarray)

{
    i4		i, j;
    PST_COL_ID	*colp;
    

    /* The process is not difficult. The requirements for a suitable
    ** key structure are as follows:
    **  - must have at least as many columns as the constraint
    **  - all constraint columns must appear in the key structure
    **  - if there are "n" constraint columns, no constraint 
    **  column may appear beyond position "n" in the key structure
    **  (i.e., the constraint columns must be amongst the high order
    **  key columns).
    **
    ** A simple loop is all that is needed to perform the tests.
    */

    if (cinteg->pst_key_count > keycount) return(FALSE);

    for (colp = cinteg->pst_cons_collist, i = 0;
		colp != NULL; colp = colp->next, i++)
    {
	bool	found;
	for (j = 0, found = FALSE; 
	    j < cinteg->pst_key_count && !found; j++)
	 if (keyarray[j] == colp->col_id.db_att_id) found = TRUE;
	if (!found) return(FALSE);	/* didn't find one */
    }

    return(TRUE);		/* survived the test */
}

/*
** Name: psl_verify_new_index - confirms suitability of index to be shared 
**	    amongst multiple constraints.
**
** Description:
** 	    Compares key lists of current constraint with first constraint
**	    with same index name to determine their mutual compatibility.
**      
** Inputs:
**	cinteg		PST_CREATE_INTEGRITY of current constraint
**	xinteg		PST_CREATE_INTEGRITY of 1st constraint with same
**			index name
** 
** Outputs:    none
**	       
** Returns:
**	TRUE  - if constraints are compatible (index-wise)
**	FALSE - if constraints aren't compatible
**
**
** History:
**	2-apr-98 (inkdo01)
**	    Written for constraint index options feature.
**	7-june-00 (inkdo01)
**	    Changes to add checks for UNIQUE constraint sharing index.
*/
static bool
psl_verify_new_index(
		PSS_SESBLK		*sess_cb,
		PST_CREATE_INTEGRITY	*cinteg,
		PST_CREATE_INTEGRITY	*xinteg)

{
    PST_COL_ID	*colp, *colp1, *colp2, *prevcolp;
    i4		i, j;
    DB_STATUS	status;
    DB_ERROR	err_blk;
    bool	copy_xinteg = FALSE;


    /* Three acceptible conditions can prevail:
    **  1. new constraint's columns are simply a permutation
    **    of the current index; i.e. the same columns but possibly
    **    in a different order.
    **
    **  2. new constraint's columns are a proper subset of the
    **    current index, but are a permutation of the high order
    **    columns of the index.
    **
    **  3. new constraint's columns are a superset of the current
    **    index, but all the current index keys are covered by the
    **    new constraint's columns. The new constraint's extra
    **    columns can be added to end of the current index key list
    **    without disrupting any previously specified constraint.
    **
    ** All other conditions are unacceptible.
    */


    /* For each column in current constraint, check index to assure
    ** high order columns of new constraint match high order columns
    ** of index (so far). */
    for (colp1 = cinteg->pst_cons_collist, i = 0;
		colp1 != NULL && i < xinteg->pst_key_count; 
		colp1 = colp1->next, i++)
    {
	bool	found;

	for (colp2 = xinteg->pst_key_collist, j = 0, found = FALSE; 
		colp2 != NULL && !found && j < cinteg->pst_key_count;
		colp2 = colp2->next, j++)
	 if (colp1->col_id.db_att_id == colp2->col_id.db_att_id)
	  found = TRUE;

	if (!found) return(FALSE);	/* high order col not matched */
    }

    /* Constraint is compatible with current index. If it has same or
    ** fewer columns, nothing more needs to be done. If it has more 
    ** columns, the extra columns must be added to end of current index
    ** key list. Also, if one constraint is UNIQUE/PRIMARY KEY, it had
    ** better have at least as many columns as the other (can't have
    ** unique defined on part of the index).
    */

    if ((cinteg->pst_key_count > xinteg->pst_key_count &&
	(xinteg->pst_integrityTuple->dbi_consflags & CONS_UNIQUE)) ||
	(cinteg->pst_key_count < xinteg->pst_key_count &&
	(cinteg->pst_integrityTuple->dbi_consflags & CONS_UNIQUE)))
							return(FALSE);

    if (cinteg->pst_key_count <= xinteg->pst_key_count) return(TRUE);

    i = cinteg->pst_key_count - xinteg->pst_key_count;
    if (xinteg->pst_key_collist == xinteg->pst_cons_collist)
    {
	i = cinteg->pst_key_count;	/* copy xinteg list, too */
	copy_xinteg = TRUE;
    }
    else for (colp2 = xinteg->pst_key_collist; colp2->next != NULL;
		colp2 = colp2->next);	/* addr end of key list */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, i * sizeof(PST_COL_ID),
	(PTR *)&colp, &err_blk);
    if (status != E_DB_OK) return(FALSE);

    /* If this is first alteration of xinteg's keylist, copy the whole
    ** list (so we don't interfere with pst_cons_collist). Otherwise,
    ** just copy the added cols from current constraint to the end. */

    if (copy_xinteg)
    {
	xinteg->pst_key_collist = colp;
	for (colp2 = xinteg->pst_cons_collist; colp2 != NULL; 
		colp2 = colp2->next) 
	{
	    STRUCT_ASSIGN_MACRO(colp2->col_id, colp->col_id);
					/* copy the column ID */
	    prevcolp = colp++;
	    prevcolp->next = colp;
	}
    }
    else colp2->next = colp;		/* link new ones to old chain */

    for (; colp1 != NULL; colp1 = colp1->next)
    {
	STRUCT_ASSIGN_MACRO(colp1->col_id, colp->col_id);
					/* copy the column ID */
	prevcolp = colp++;
	if (colp1->next != NULL) prevcolp->next = colp;
	else prevcolp->next = NULL;
    }
    return(TRUE);

}


/*
** Name: psl_print_conslist  - print out info in list of constraints
**
** Description:	    
**          Cycle through list of PSS_CONS nodes, printing out info for each
**      
** Inputs:
**	cons_list	    list of constraint info blocks
**
** Outputs:
**	none
**
** Returns:
**	none
**
** Side effects:
**	prints out debug info
**
** History:
**
**	01-sep-92 (rblumer)
**	    written
*/
static VOID
psl_print_conslist(PSS_CONS *cons_list, i4  print)
{
    PSS_CONS   	*curcons;
    
    if (print == FALSE)
	return;
    
    if (cons_list == (PSS_CONS *) NULL) 
	return;

    SIprintf("\nThe CONSTRAINT LIST contains:\n");

    curcons = cons_list;
    
    while (curcons != NULL)
    {
	psl_print_cons(curcons);

	curcons = curcons->pss_next_cons;
    }

}  /* end psl_print_conslist */


static VOID
psl_print_colq(PSF_QUEUE *q)
{
    PSY_COL   	*col;
    
    /* print out column names, if any
     */
    for (col = (PSY_COL *) q->q_next; 
	 col !=(PSY_COL *) q;
	 col = (PSY_COL *) col->queue.q_next)
    {
	SIprintf("\t\t col name = '%*s'\n", DB_ATT_MAXNAME, 
		                            col->psy_colnm.db_att_name);
    }
}  /* end psl_print_colq */


static VOID
psl_print_cons(PSS_CONS *cons)
{
    SIprintf("cons address = 0x%lx\n", cons);
    SIprintf("\t cons type = 0x%lx\n", cons->pss_cons_type);

    if (cons->pss_cons_type & PSS_CONS_NAME_SPEC)
	SIprintf("\t cons name = '%*s'\n", DB_CONS_MAXNAME, 
		     cons->pss_cons_name.db_constraint_name);
    else
	SIprintf("\t cons name = <none>\n");

    if (cons->pss_cons_type & PSS_CONS_UNIQUE)
	SIprintf("\t constraint is a UNIQUE constraint\n");

    if (cons->pss_cons_type & PSS_CONS_CHECK)
	SIprintf("\t constraint is a CHECK constraint\n");

    if (cons->pss_cons_type & PSS_CONS_REF)
	SIprintf("\t constraint is a REFERENTIAL constraint\n");
    
    if (cons->pss_cons_type & PSS_CONS_PRIMARY)
	SIprintf("\t constraint is a PRIMARY KEY constraint\n");

    if (cons->pss_cons_type & PSS_CONS_NOT_NULL)
	SIprintf("\t constraint is a NOT NULL constraint\n");

    if (cons->pss_cons_type & PSS_CONS_COL)
	SIprintf("\t constraint is a COLUMN-LEVEL constraint\n");


    if (cons->pss_cons_colq.q_next == &cons->pss_cons_colq)
	SIprintf("\t No constraint columns\n");
    else
	SIprintf("\t Constraint column(s) are:\n");

    /* print out column names, if any
     */
    psl_print_colq(&cons->pss_cons_colq);
    

    if (cons->pss_cons_type & PSS_CONS_REF)
    {
	SIprintf("\t Referenced table name = '%*s'\n", DB_TAB_MAXNAME,
		 cons->pss_ref_tabname.pss_obj_name.db_tab_name);
	SIprintf("\t\t Owner name = '%*s'\n", DB_OWN_MAXNAME,
		 cons->pss_ref_tabname.pss_owner.db_own_name);
	SIprintf("\t\t Original table name = '%s'\n", 
		 cons->pss_ref_tabname.pss_orig_obj_name);
    }
    else
	SIprintf("\t No referenced table name\n");


    if (cons->pss_ref_colq.q_next == &cons->pss_ref_colq)
	SIprintf("\t No referenced columns\n");
    else
	SIprintf("\t Referenced column(s) are:\n");

    /* print out column names, if any
     */
    psl_print_colq(&cons->pss_ref_colq);

    
    if (cons->pss_check_cond == (PSS_TREEINFO *) NULL)
	SIprintf("\t No check condition\n");
    else
	SIprintf("\t Check condition exists\n");   	

}  /* end psl_print_cons */


/*
** Name: psl_find_iobj_cons - find IIDBDEPENDS tuples for references constraint
**
** Description:
**          This function will scan IIDBDEPEND tuples associated with a given
**          independent object, locating those that are for references constraint.
**
** Inputs:
**	sess_cb         PSF session CB
**	rngvar          range variable describing the table whose
**                      tuples we will be perusing
**
** Output:
**	found           TRUE if a desired tuple was found; FALSE otherwise
**	err_blk         filled in if an error occurred
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	fixes and unfixes IIDBDEPENDS tuples in RDF cache
**
** History:
**	22-Mar-2005 (thaju02)
**	    Created. (B114000)
*/
DB_STATUS
psl_find_iobj_cons(
PSS_SESBLK                *sess_cb,
PSS_RNGTAB                *rngvar,
i4                        *found,
DB_ERROR                  *err_blk)
{
    DB_STATUS           status = E_DB_OK;
    DB_STATUS           temp_status;
    DB_IIDBDEPENDS      dbdep[RDF_MAX_QTCNT];
    DB_IIDBDEPENDS      *cur_dbdep;
    RDF_CB              rdf_cb;
    i4                  tupcount;
    i4                  err_code;

    MEfill(RDF_MAX_QTCNT * sizeof(DB_IIDBDEPENDS),
		(u_char)0, (PTR)&dbdep[0]);

    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_rb.rdr_2types_mask    = RDR2_DBDEPENDS;
    rdf_cb.rdf_rb.rdr_update_op     = RDR_OPEN;
    rdf_cb.rdf_rb.rdr_rec_access_id = NULL;
    rdf_cb.rdf_rb.rdr_qtuple_count  = RDF_MAX_QTCNT;
    rdf_cb.rdf_rb.rdr_qrytuple = (PTR)&dbdep[0];

    *found = FALSE;

    while (rdf_cb.rdf_error.err_code == 0 && !*found)
    {
	status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

	rdf_cb.rdf_rb.rdr_update_op = RDR_GETNEXT;

	/*
	** can't use DB_FAILURE_MACRO because RDF returns a warning (E_DB_WARN)
	** for errors such as RD0011, and we want to reset the status in those
	** cases, too.
	*/
	if (status != E_DB_OK)
	{
	    switch (rdf_cb.rdf_error.err_code)
	    {
		case E_RD0002_UNKNOWN_TBL:
		{
		    (VOID) psf_error(2117L, 0L, PSF_USERERR,
			&err_code, err_blk, 1,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    rngvar->pss_tabname.db_tab_name),
			rngvar->pss_tabname.db_tab_name);
		    status = E_DB_ERROR;
		    continue;
		}

		case E_RD0011_NO_MORE_ROWS:
		{
		    status = E_DB_OK;
		    break;
		}

		case E_RD0013_NO_TUPLE_FOUND:
		{
		    status = E_DB_OK;
		    continue;
		}

		default:
		{
		    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error,
			err_blk);
		    status = E_DB_ERROR;
		    continue;
		}
	    }
	}

	/* FOR EACH DBDEPENDS */
	for ( tupcount = 0;
	     tupcount < rdf_cb.rdf_rb.rdr_qtuple_count;
	     tupcount++)
	{
	    cur_dbdep = (DB_IIDBDEPENDS *)&dbdep[tupcount];

	    if (cur_dbdep->dbv_dtype == DB_CONS)
	    {
		*found = TRUE;
		break;
	    }
	}
    }

    /* unfix iidbdepends, if not already done so by rdf */
    if (rdf_cb.rdf_rb.rdr_rec_access_id != NULL)
    {
	rdf_cb.rdf_rb.rdr_update_op = RDR_CLOSE;

	/* use temp_status so that we don't overwrite
	** a possible error status from above
        */
	temp_status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

	if (DB_FAILURE_MACRO(temp_status))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	    if (temp_status > status)
		status = temp_status;
	}
    }
    return(status);
}
