/*
**Copyright (c) 1986, 2005, 2009 Ingres Corporation
**
*/
#ifndef __PSHPARSE_H_INC
#define __PSHPARSE_H_INC

#include <ulm.h>		/* left out of too many psf .c's, do here */

/**
** Name: PSHPARSE.H - Names and types local to parser facility
**
** Description:
**      This file contains the names and types local to the parser facility
**      as a whole.
**
** History: 
**      02-jan-86 (jeff)
**          combined from a bunch of other headers
**     18-mar-87 (stec)
**          Added pss_resjour to PSS_SESBLK.
**     23-apr-87 (stec)
**          Added definitions of PSY_USR, PSY_TBL, PSY_COL, PSY_SEQ.
**     04-may-87 (stec)
**          Changed PSY_SEQ to PSY_TSEQ, PSY_CSEQ.
**     11-may-87 (stec)
**          Added pss_udbid to PSS_SESBLK.
**	21-may-87 (stec)
**	    Move YYAGG_NODE_PTR from yyvars.h
**	28-may-87 (puree)
**	    Add fields in PST_PROTO for GRANT statements.
**	14-aug-87 (stec)
**          Deleted/modified definitions of PSY_USR, PSY_TBL, PSY_COL, PSY_SEQ.
**	04-sep-87 (stec)
**	    Added pss_crsid to PSS_SESBLK.
**	10-nov-87 (puree)
**	    Added pst_qrange to save the range varable table of the 
**	    prepared query in PST_PROTO.
**	14-nov_87 (puree)
**	    Added definitions for COPY.
**	17-nov-87 (puree)
**	    Removed range table from DSQL query prototype.  It is not needed
**	    since the qrymod is run at query parsing time.
**	30-dec-87 (stec)
**	    Added PST_VARMAP definition.
**	10-feb-88 (stec)
**	    Add PSF_MAX_TEXT define.
**	17-feb-88 (stec)
**	    Converted PST_VARMAP to PST_VRMAP (lint).
**      13-jul-88 (andre)
**	    Added PSS_EXLIST typedef.
**      27-july-88 (andre)
**	    Modified PSS_DBPINFO for DB proc vsn of GRANT/REVOKE.
**	03-aug-88 (stec)
**	    Modified PSS_DBPINFO.
**	23-aug-88 (stec)
**	    Fix LINT warnings.
**	22-sep-88 (stec)
**	    Save update flag in PST_PROTO.
**	24-oct-88 (stec)
**	    Add fields to cursor control block.
**	09-nov-88 (stec)
**	    Define new datatype in connection with cursors.
**	10-nov-88 (stec)
**	    Changed pss_udbid from PTR to I4
**	08-mar-89 (andre)
**	    added pss_dba_drop_all to PSS_SESBLK.
**	14-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added pss_group to PSS-SESBLK for session group identifier;
**	    Added pss_aplid to PSS_SESBLK for session application id.
**	17-mar-89 (neil)
**	    Rules: Added pss_image to PSS_RNGTAB.
**	18-apr-89 (neil)
**	    Rules: Added PSC_RULE_TYPE and modified PSC_CURBLK for cursors.
**	11-may-89 (neil)
**	    Added pst_cursid to Dynamic SQL PST_PROTO structure.
**	30-may-89 (neil)
**	    Commented/updated use of PSS_DBPPARMMAX.
**	06-jun-89 (ralph)
**	    Changed pss_group and pss_aplid to DB_TAB OWN to eliminate
**	    unix compile problems.
**      20-jul-89 (jennifer)
**          Added server capability field to server control block.
**	27-jul-89 (jrb)
**	    Added pss_num_literals to PSS_SESBLK so PSF will know which set
**	    of numeric literal scanning rules to use.
**	11-oct-89 (ralph)
**	    Add pss_rgroup, pss_rgset, pss_raplid, pss_raset to store
**	    group/role state when compiling database procedures.
**	16-oct-89 (ralph)
**	    Changed pss_rgroup and pss_raplid to DB_TAB_OWN
**	27-oct-89 (ralph)
**	    Add pss_ustat to PSS_SESBLK.
**	28-dec-89 (andre)
**	    Added pss_fips_mode to PSS_SESBLK.
**	11-sep-90 (teresa)
**	    changed several bools/nat type of flags to become bitmasks in
**	    pss_flag or pss_ses_flag and pss_ses_flag in PSS_SESBLK; fixed
**	    faulty use of pss_retry; removed obsolete pss_embedded
**	12-sep-90 (sandyh)
**	    Added minimum session value and psf_sess_mem field.
**	28-nov-90 (andre)
**	    moved flags defined over qry_mask from yyvars.h since they may be
**	    used outside the semantic actions.
**	17-may-91 (andre)
**	    Added a new field to PSS_SESBLK to contain a NULL-terminated DBA
**	    name.  It will be used when processing $dba.
**	30-may-91 (andre)
**	    Added pst_ret_flag to PST_PROTO for handling dynamically prepared
**	    SET USER/GROUP/ROLE.
**	12-nov-91 (rblumer)
**        merged from 6.4:  6-mar-1991 (bryanp)
**	    Added PSS_COMP_CLAUSE to the list_clause definitions and also added
**	    definitions for the with_clauses field of the YYVARS.H struct.
**	    These changes are to support Btree Index Compression; as part of
**	    that project we are moving code out of the grammar files proper
**	    into separate 'semantic actions' files such as pslmdfy.c, pslindx.c,
**	    and pslctbl.c
**        merged from 6.4:  02-jul-91 (andre)
**	    Added PSS_UNSUPPORTED_CRTTBL_WITHOPT to the list_clause definitions.
**	    This new constant will be used if in the course of parsing
**	    CREATE TABLE WITH-clause "option = (value_list)", we come across an
**	    option which we don't support.
**        merged from 6.4:  25-jul-91 (andre)
**	    added PSS_STRIP_NL_IN_STRCONST as a flag for pss_stmt_flags; 
**	    this is required to fix bug 38098
**	26-feb-92 (andre)
**	    define PSS_REPAIR_SYSCAT over pss_ses_flag
**	30-mar-1992 (bryanp)
**	    Added pss_sess_owner to PSS_SESBLK for temp table enhancements.
**	    Added pss_sess_table to the PSS_OBJ_NAME.
**	    Increased max_modify_chars for DMU_TEMP_TABLE characteristic.
**	    Added PSS_WC_RECOVERY for WITH [NO]RECOVERY clause.
**	15-jun-92 (barbara)
**	    Merge of rplus and Star code for Sybil project.  Added Star
**	    masks, PSS_Q_XLATE and PSS_Q_PACKET_LIST stuctures for buffering
**	    query text; also PSS_LTBL_INFO for local table info in DROP
**	    table.  Added pss_sess_flag for gateway.  Added Star comments:
**	    12-oct-88 (andre)
**	    	Added PS_NUM_CONST, PSS_STR_CONST, PSS_DATA_VALUE,
**	    	increased PSS_SBSIZE.
**	    12-oct-88 (andre)
**	    	Added PST_CHECK_EXIST.
**	    21-oct-88 (andre)
**	    	placed pss_ctxt ahead of pss_text_const
**	    17-nov-88 (andre)
**	    	changed PST_CHECK_EXIST to PST_DLINK.
**	    28-feb-89  (andre)
**	    	Added definitions for the lengths of the longest keywords in
**		QUEL and SQL - PSS_0QUEL_LONGEST_KWRD and PSS_1SQL_LONGEST_KWRD.
**	    28-apr-89 (andre)
**	    	Changed PST_DLINK back to PST_CHECK_EXIST as this better
**		describes the meaning of the mask.
**	    13-oct-89 (stec)
**	    	Added PST_REMOVE define to provide additional info to RDF on 
**	    	DROP LINK, REMOVE commands.
**	    06-jun-91 (fpang)
**	    	Added /psf=mem support.
**	    29-apr-92 (barbara)
**		Substitute PSS_GET_OWNCASE and PSS_GET_TBLCASE in place of
**		PSS_GETCASE.  (Bug 42981 regarding incorrect case of owner name
**		in catalog queries from RDF.)
**	25-sep-92 (andre)
**	    defined PSS_COL_REF - this structure will describe how a column was
**	    referenced (i.e. whether a table/correlation name was used and
**	    whether a schema name was used.)
**	26-sep-92 (andre)
**	    got rid of PSS_VAR_NAMES and defined PSS_TBL_REF
**    30-oct-92 (rblumer)
**        FIPS CONSTRAINTS:  changed 'nat psr_column' to
**           'DB_COLUMN_BITMAP psr_columns' as now users can specify
**           any number of columns in the rule's update action;
**	  added PSS_CONS structure;  
**	  added numerous error messages for constraints;
**	  defined PSS_PARSING_CHECK_CONS over pss_stmt_flags;
**	  add PSS_REF_TYPERES for pst_node calls; added new PSS_TYPE_*'s;
**	  added prototypes for psl_ct18_* to psl_ct22_* functions;
**	  added parameters to prototypes for several functions.
**	12-oct-92 (ralph)
**	    defined constants PSS_ID_{NAME,DELIM,SCONST,DBA,INGRES}
**	    for YYVARS.H element id_type in support of delimited identifiers.
**	18-nov-92 (barbara)
**	    Extended interface to psl0_orngent.
**	    Changed interface of psl_lm5_setlockmode and psl_send_setqry
**	    to pass in PSQ_CB pointer.
**	23-nov-92 (ralph)
**	    CREATE SCHEMA:  Added new fields to PSS_YYVARS
**	    Added bypass_actions to support %nobypass in YACC and grammars.
**	    Added submode to indicate substatement type
**	    Added deplist to track view dependencies
**	    Added stmtstart to track beginning of substatement
**	03-dec-92 (tam)
**	    Added prototype declarations for psq_dbpreg
**	14-dec-92 (tam)
**	    Added PST_REGPROC and PST_FULLSEARCH to table description info.
**	04-jan-93 (tam)
**	    Added prototype for psy_kregproc().
**	10-dec-92 (rblumer)
**	    Added with-clause (WC_) options for UNIQUE_SCOPE, UNIQUE, and 
**	    PERSISTENCE options; added 5 to max INDEX & MODIFY CHAR entry's.
**	    For statement-level rules, added pss_stmt_rules to PSS_SESBLK
**	    and added psc_stmt_rules to PSC_CURBLK.
**	    Added pss_dbprng to PSS_SESBLK for holding for set-input parameter;
**	    added new fields to PSS_DBPINFO for set-input parameters.
**	05-mar-93 (ralph)
**	    CREATE SCHEMA: Add/change prototypes
**	    DELIM_IDENT: Added pss_dbxlate to PSS_SESBLK to contain
**	    case translation masks.
**	10-mar-93 (barbara)
**	    Star support for delimited ids.
**	    . Changed prototypes for psl_nm_eq_no, psl_ct6_cr_single_kwd,
**	      psl_lst_prefix, psl_ct16_distr_with, psl_rg1_reg_distr_tv,
**	      psl_rg2_reg_distr_idx, psl_rg5_ldb_spec, psl_ct9_new_loc_name
**	      and psl_ct13_newcolname.
**	    . Moved YYVARS variable "ldb_flags" into PSS_SESSBLK 
**	      ("pss_distr_sflags").
**	30-mar-93 (rblumer)
**	    removed prototype for psl_reserved_ident; it is now static.
**	    added extern for psy_secaudit to get rid of compile warnings.
**	02-apr-93 (ralph)
**	    DELIM_IDENT: Added pss_cat_owner to PSS_SESBLK to contain the
**	    name of the catalog owner for the database.
**	    Changed pss_dbxlate to be a pointer to a u_i4.
**	15-may-1993 (rmuth)
**	    Add support for concurrent index builds, add 
**	    PSS_WC_CONCURRENT_ACCESS.
**	27-may-93 (rblumer)
**	    added prototype for psl_fill_proc_params.
**	25-may-1993 (rog)
**	    Added ret_val argument to the psq_cbreturn() prototype.
**	1-jun-1993 (robf)
**	   Secure 2.0: Added PSS_ROW_SEC_LABEL, PSS_ROW_SEC_AUDIT and
**	   PSS_ROW_SEC_KEY for session row security attributes. 
**	   Add PST_2UPD_ROW_SECLABEL to pass info to pst_header()
**	10-jul-93 (andre)
**	    prototypes for externally visible PSF functions have been added to
**	    PSFPARSE.H so they need to be removed from this file
**	26-jul-1993 (rmuth)
**	    Add with_clauses argument to psl_cp1_copy, psl_cp2_copstmt
**	    and psl_cp11_nm_eq_no.
**	12-aug-93 (andre)
**	    removed definitions of TABLES_TO_CHECK, DBPROCS_TO_CHECK, and 
**	    DBEVENTS_TO_CHECK macros.  Our original (and very old) assumption 
**	    that INGRES's 3-tier name space will not be used when
**	    running in FIPS mode turned out to be incorrect, thus these
**	    macros which take into account whether we are running in FIPS mode 
**	    in determining whether to look for objects owned by the DBA and 
**	    $ingres are no longer needed
**	13-aug-93 (rblumer)
**	    Added PSS_CALL_ADF_EXCEPTION flag for signaling when to call the
**	    ADF exception handler from inside PSF (i.e. only when doing math
**	    calculations on behalf of the user).
**	17-aug-93 (andre)
**	    defined a typedef for identifier type - PSS_ID_TYPE
**	08-sep-93 (swm)
**	    Changed sessid parameter to psq_siddmp() function to CS_SID
**	    to reflect recent CL interface revision.
**	08-oct-93 (rblumer)
**	    Added trace point PS251, which disables single-quoted ids.
**	    Also changed trace point vector to have the last 20 trace points
**	    (ps236-ps255) allow values, with at most 5 set at one time;
**	    see PSS_TVALS and PSS_TVAO.
**	18-oct-93 (rogerk)
**	    Added PSF_NO_JNL_DEFAULT psf_flag which specifies that tables
**	    should be created with nojournaling by default.
**	22-oct-93 (rblumer)
**	    In PSS_SESBLK, changed pss_setparmlist to pss_procparmlist, as now
**	    it will be used for both set-input procedures AND normal procedures;
**	    moved rest of pss_setparm* parameters to PSS_DBPINFO, 
**	01-nov-93 (anitap)
**	    Added PSS_INGRES_PRIV over pss_ses_flag in PSS_SESBLK.
**	16-nov-93 (robf)
**          Add PSS_ROW_LABEL_VISIBLE to support display of row labels.
**	18-nov-93 (stephenb)
**	    Remove declaration for psy_secaudit() from here and place it in
**	    a new file psyaudit.h so that we can prototype it correctly without
**	    worrying about who includes sxf.h.
**	29-nov-93 (rblumer)
**	    Added prototype for psf_munlock routine.
**	04-jan-94 (rblumer)
**	    Added trace point 176 for enabling dummy NOT NULL constraints.
**	17-dec-93 (rblumer)
**	    removed PSS_FIPS_MODE flag from PSS_SESBLK.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Made psf_display() portable. The old psf_display() took a variable
**	    number of variable-sized arguments. It could not be re-written as
**	    a varargs function as this practice is outlawed in main line code.
**	    The problem was solved by invoking a varargs function, TRformat,
**	    directly.
**	    The psf_display() function has now been deleted, but for flexibilty
**	    psf_display has been #defined to TRformat to accomodate future
**	    change.
**	    All calls to psf_display() changed to pass additional arguments
**	    required by TRformat:
**	        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, ...)
**	    this emulates the behaviour of the old psf_display() except that
**	    the trbuf must be supplied.
**	    Note that the size of the buffer passed to TRformat has 1
**	    subtracted from it to fool TRformat into NOT discarding a
**	    trailing '\n'.
**	    Added psf_relay() to cater for an instance of adu_2prvalue() which
**	    requires a function pointer to an output routine.
**	17-mar-94 (andre)
**	    added prototype declaration for psq_open_rep_cursor()
**	18-apr-94 (rblumer)
**	    added trace point PSS_PRINT_PSS_CONS for printing out constraint
**	    info blocks generated by the parser.  Changed a few prototypes.
**      07-oct-94 (mikem)
**	    Added the PSS_ENABLE_NOERR_ON_DUPS traceflag, see "TRACE FLAGS" doc
**	    for more info.
**	15-nov--94 (rudtr01)
**		added DD_LDB_DESC struct to PST_PROTO to make COPY TABLE
**		work properly in STAR (bug 47329).
**      20-Jan-95  (liibi01)
**          Added function prototype for get_default_user
**	21-Mar-95  (ramra01)
**	    Added additional flag PSS_CREATE_STMT to pss_stmt_flags
**	    This is persistent for a statement execution (B67519)
**      21-Mar-95 (liibi01)
**          Due to the grammar change, now the alter user statement
**          doesn't support default user any more. So we don't need
**          function prototype get_default_user.
**	4-mar-1996 (angusm)
**	    Add flag:
**	    PSS_HAS_DGTT for PSS_SESBLK.pss_stmt_flags
**	    so that presence of global temp table in repeated query,
**	    can be propagated up from PSS_OBJ_NAME structure and query
**	    id adjusted to preserve uniqueness. (bug 74863)
**	11-jun-1996 (angusm)
**	    Add flag PSS_CREATE_DGTT for PSS_SESBLK.pss_stmt_flags,
**	    since DGTT needs to parse creation of decimal columns
**	    if II_DECIMAL = ",". Needs to be separate from PSS_HAS_DGTT
**	    in case has side-effects for complex queries which include
**	    a DGTT. (bug 76902).
**	29-oct-1996 (prida01)
**	    Add pss_viewrng to store first set of range variables after a 
**	    prepare if views were included. We need to use this set for each 
**	    subsequent open or we get invalid results.
**	06-mar-1996 (stial01 for bryanp)
**	    Add PSS_WC_PAGE_SIZE to track the WITH PAGE_SIZE clause.
**	01-may-1995 (shero03)
**	    Added support for RTREE in create_index.
**	22-jul-1996 (ramra01)
**	    Alter Table add/drop project:
**	    Function psl_alt_tbl_col_drop, psl_alt_tbl_col_add,
**	    and psl_alt_tbl_col added declaration.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added PSS_WC_PRIORITY
**	24-june-1997(angusm)
**	Add PSF_AMBREP_64COMPAT for redo of ambiguous replace.
**          PSS_XTABLE_UPDATE
**	27-aug-97 (i4jo01)
**	Add define for parsing of insert stmt. Used for skipping tuple
**	width check during insert...select.	
**	29-sep-97 (sarjo01)
**	    added new PSS_RNGTAB **rngtable arg to psl_p_telem()
**	30-sep-97 (sarjo01)
**	    Oops. Forgot comma in above change. 
**	19-may-1998 (nanpr01)
**	    Added new fields for Parallel Index Creation.
**      29-jun-98 (sarjo01) from 17-apr-98 (hayke02) change 435297
**          Added PSS_INNER_RNGVAR, which will be set (in psl_set_jrel()) if
**          this entry is the inner in a left, right or full join. Also added
**          a new argument to psl_set_jrel(), join_type (the outer join type),
**          with type DB_JNTYPE. The new constant and argument are used when
**          setting the nullability of resdoms in prepared outer join
**          statements. This change fixes bug 90099.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	    Change type of psf_sem from PTR to CS_SEMAPHORE, embedding
**	    it in Psf_srvblk.
**      05-Nov-98 (hanal04)
**          Added elements to PST_PROTO and PSS_SESBLK to allow
**          pss_viewrng.maxrng values to be held in a structure
**          with a one to one mapping to the prepared statement.
**          b90168.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**      28-jun-1999 (hanch04)
**	    Change i4  to i4
**	16-Jun-1999 (johco01)
**	    Cross integrate change 439089 (gygro01)
**	    Took out previous change (438631) because it introduced bug 94175.
**	    Change 437081 (inkdo01) solves bugs 90168 and 94175.
**	09-Sep-1999 (andbr08)
**	    Added one to the length of PST_PROTO structures pst_sname array.
**	    Allows a 32 character name and a terminator to fit.  (bug 43805).
**	25-oct-99 (inkdo01)
**	    Dropped psl_fillGroupList prototype, added a parm to psl_hcheck
**	    and added psl_hcheckvar and psl_constx for group by expression changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-nov-2000 (abbjo03) bug 102580
**	    Add PSS_DEFAULT_VALUE to deal with parsing of numeric values when
**	    II_DECIMAL is set to comma.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**         Added PSS_RULE_UPD_PREFETCH. If set we sill use the prefetch
**         solution to ensure consitent behaviour in updating rules fired
**         by updates.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SESBLK parm to prototypes for psf_mopen(), psf_mlock(),
**	    psf_munlock(), psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp()
**	    psl_rptqry_tblids(), psq_tout(), psq_tmulti_out(), psl_gen_alter_text(),
**	    pst_add_1indepobj(), pst_1dbtab_desc().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK*.
**	    Embedded QSF_RCB in PSS_SESBLK (pss_qsf_rcb).
**	    Added GLOBALREF to *Psf_srvblk, GET_PSS_SESBLK macro.
**	    Added psf_psscb_offset to PSF_SERVBLK.
**	17-Jan-2001 (jenjo02)
**	    Added *PSS_SESBLK to prototype for psq_prt_tabname().
**	30-apr-01 (inkdo01)
**	    Added "first_n" parm to psl_ct2s_crt_tbl_as_select.
**	25-nov-02 (inkdo01)
**	    Various changes required for enlarged range table.
**	01-May-2003 (hanch04)
**	    Added PSS_DBPROC_LHSVAR for bug 110041.  If II_DECIMAL=','
**	    then variable assignments should be allow to have ','
**	    instead of '.' in CREATE PROCEDURE statements.
**	16-jul-03 (hayke02)
**	    Added PSS_VARCHAR_PRECEDENCE for new trace point ps202 and
**	    new config parameter psf_vch_prec. This change fixes bug 109134.
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions.
**      24-jul-2003 (stial01)
**          Added PSQ_STMT_INFO to PST_PROTO for b110620
**      04-08-13 (wanfr01)
**          Bug 112828, INGSRV2799
**          Added targparm to PSS_SESBLK to track if any dynamic parameters
**          are used in the select list.
**      02-jan-2004 (stial01)
**          Changes for SET [NO]BLOBJOURNALING, SET [NO]BLOBLOGGING
**          Changes to expand number of WITH CLAUSE options
**      08-jan-2004 (stial01)
**          Fixed PSS_WC_ANY_MACRO
**      14-jan-2004 (stial01)
**          Removed PSS_BLOBJOURNALING, ([no]blobjournaling requires tablename)
**	25-Jan-2004 (schka24)
**	    Swipe WITH option CLUSTERED (never used) for PARTITION=.
**	    Add partitioning option definitions.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**	26-Apr-2004 (schka24)
**	    Prototype changes for partitioned COPY.
**      13-may-2004 (stial01)
**          Removed support for SET [NO]BLOBJOURNALING ON table.
**	03-jun-2004 (thaju02)
**	    Added PSS_WC_CONCURRENT_UPDATES for online modify.
**	13-jul-2004 (gupsh01)
**	    Put back PSS_NEED_ROW_SEC_KEY. 
**	22-Jul-2004 (schka24)
**	    Delete ifdef'ed and obsolete normalization prototypes.
**	18-Oct-2004 (shaha03)
**	    SIR 112918, added PSF_DFLT_READONLY_CRSR for configurable default 
**	    cursor mode support.
**      04-nov-2004 (thaju02)
**          Change psf_sess_mem, memleft to SIZE_TYPE.
**	13-dec-04 (inkdo01)
**	    Added collationID parm to psl_ct14_typedesc() for collation support.
**      22-Mar-2005 (thaju02)
**         Added prototype for psl_find_iobj_cons.
**	19-jul-2005 (toumi01)
**	    Added prototype for psl_us2_with_nonkw_eq_hexconst to
**	    support CREATE/ALTER USER/ROLE WITH PASSWORD=X'<encrypted>'.
**	19-Oct-2005 (schka24)
**	    Remove unused alter-timestamp prototype.
**	04-nov-05 (toumi01)
**	    Added flag to tell pst_node about avg(x) to sum(x)/count(x)
**	    transformation so that it can adjust result precision/scale.
**	    Added i4 xform_avg boolean for communicating avg(x)
**	    transformation through recursive psl_p_telem calls.
**	23-Nov-2005 (kschendel)
**	    Update validate-options prototype.
**	9-may-06 (dougi)
**	    Add trace point ps151 to enable 6.4 ambiguous replace semantics
**	    (it's already a CBF parm - this just provides finer granularity
**	    control).
**	12-Jun-2006 (kschendel)
**	    Make many ps144 dumper routines static, remove from here.
**	22-jun-06 (toumi01)
**	    Do not error schema-less references to tables where there exists
**	    both a session table and a permanent table for the give name.
**	23-Jun-2006 (kschendel)
**	    Add range table unfix routine.
**	30-aug-06 (thaju02)
**	    Add PSS_RULE_DEL_PREFETCH. (B116355)
**	31-aug-06 (toumi01)
**	    Delete ps152 trace point for controlling GTT syntax shortcut;
**	    symantics are now based on method of table declaration.
**	13-sep-2006 (dougi)
**	    Added pst_convlit() prototype to support insert/update
**	    coercions between strings & numerics.
**      9-Nov-2006 (kschendel)
**          Take some pslsgram functions static -- probably more could
**          be done but enough for today.
**      21-Feb-2007 (hanal04) Bug 117736
**          Added trace point ps203 to dump PSF's ULM memory usage to
**          the DBMS log.
**    25-Oct-2007 (smeke01) b117218
**        CASE statement with subselect has not been implemented. Added
**        error message to say this is not supported.
**	2-nov-2007 (dougi)
**	    Add trace point ps006 as server tp to force dynamic select's to 
**	    be cached.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Rename psf_error() to psfErrorFcn() and prototype.
**	    Define psf_error as a macro calling psfErrorFcn().
**    22-sep-2008 (stial01)
**          Added trace point ps147 for psl scanner utf8/unicode unorm diags
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	03-Nov-2008 (kiria01) SIR121012
**	    Add definition for pst_is_const_bool.
**	20-Nov-2008 (wanfr01) b121252
**	    Add PSS_PARSING_GRANT
**	13-Jan-2009 (kiria01) SIR121012
**	    Change prototype for pst_prmsub to allow tree to be collapsed
**	    in-situ.
**	05-Feb-2009 (kiria01) b121607
**	    Changed prototypes for psl_cp2_copstmnt() and getfacil()
**	    to reflect the change from i4 to PSQ_MODE used in the call.
**      10-mar-2009 (stial01) 
**          Added prototype for psl_unorm_error (b121769)
**      21-May-2009 (kiria01) b122051
**          Added prototype for psl_collation_check()
**      15-Jul-2009 (frima01) b122051
**          Use new type DB_COLL_ID in psl_ct14_typedesc declaration.
**      October 2009 (stephenb)
**          Add defines and fiedls for batch procesing
**      26-Oct-2009 (coomi01) b122714
**          Move psq_store_text() declarator from psqtxtmod.c to here and made public. 
**	05-Nov-2009 (kiria01) b122841
**	    Add prototype for psl_mk_const_similar
**	05-Nov-2009 (kiria01) SIR 121883
**	    Scalar Sub-queries changes.
**	12-Nov-2009 (kiria01) b122841
**	    Corrected psl_mk_const_similar parameters with explicit
**	    mstream.
**	10-Feb-2010 (toumi01) SIR 122403
**	    New #defines and encrypt_spec for tracking encryption parsing.
**	    New function psl_nm_eq_hexconst for AESKEY= parsing.
**	25-Mar-2010 (kiria01) b123535
**	    Clarified PST_DESCEND_MARK and exported psl_ss_flatten.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	06-Apr-2010 (gupsh01) SIR 123444
**	    Added support for ALTER TABLE ..RENAME TABLE/COLUMN.
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl0_rngent and psl_rngent for WITH
**	    support. Corrected prototype for psl_set_jrel & dropped
**	    pst_swelem.
**	14-Jul-2010 (kschendel) b123104
**	    Add pst-not-bool proto.
**	20-Jul-2010 (kschendel) SIR 124104
**	    Pass with-clauses to psl-ct1-create-table.
*/

/*
**  TRACE FLAGS:   
**	Session:
**	    -only last 20 (#236 through 255) allow values to be specified
**
**  xDEBUG
**    Y     0 (ps128)	-   print tracing info as YACC is shifting/reducing its
**			    way through the query text
**    N	    1 (ps129)	-   set printqry - print query buffer before
**			    parsing the
**			    query
**    N	    3 (ps131)	-   print tracing info of the algorithm determining if
**			    a user can grant permit on a given table or view
**    N	    4 (ps132)	-   print tracing info of the algorithm determining
**			    whether a given repeat query is shareable and
**			    whether it will be allowed to use an existing QEP
**			    (if one is found)
**    N	    5 (ps133)	-   print tracing info of the algorithm determining if
**			    a user can grant permit on a given database
**			    procedure, can execute a given dbproc owned by him,
**			    or can create a specified dbproc (i.e. whether a
**			    dbproc is grantable or active)
**    Y	    13 (ps141)	-   print info about a cursor
**    Y	    14 (ps142)	-   print dbproc query tree by following pst_link ptrs
**    Y	    15 (ps143)	-   print dbproc query tree by following pst_next ptrs
**    Y	    16 (ps144)	-   print query tree (long version)
**    Y	    17 (ps145)	-   print query tree (short version)
**    Y	    18 (ps146)	-   print a subtree rooted in the newly allocated node
**			    (pst_node)
**    N     19 (ps147)  -   print psl scanner UNORM diagnostics
**  ****    20 (ps148)	-   will never fire; used to print qualification tree as
**			    it was being normalized (pst_norml)
**    Y	    21 (ps149)	-   print number of children of opnode for which type
**			    resolution is being performed; use SET TRACE
**			    TERMINAL to get output on the screen (pst_resolve)
**    N     22 (ps150)  -   enable DROP SCHEMA; in 6.5, we will not officially
**			    support DROP SCHEMA, but if someone wants it real
**			    bad, they can
**				set trace point ps150
**			    y voila!
**    N	    23 (ps151)	-   enable 6.4 ambiguous replace semantics
**    N	    47 (ps175)	-   print out info in PSS_CONS list of constraints
**    N	    48 (ps176)	-   causes "dummy" NOT NULL constraints to be generated
**    			    for not null columns (used for testing this feature
**    			    in 6.5; the feature has been disabled until 6.6 or
**    			    whenever we need to implement SQL92 catalog support)
**    N	    49 (ps177)	-   allow set-input procedures and statement-level rules
**    			    to be created by the user (instead of only by QEF)
**    Y	    50 (ps178)	-   print tracing info as the protection algorithm gets
**			    applied to a query tree (psy_permit)
**    Y	    51 (ps179)	-   print protection qualification trees as the permitsd
**			    get applied to a query tree (psy_permit)
**  ****    71 (ps199)	-   will never fire; used to print segments of output as
**			    they are being stored in the buffer to be eventually
**			    displayed as output of HELP PERMIT|INTEGRITY_VIEW
**			    which are no longer supported in the BE (psy_print)
**    Y	    72 (ps200)	-   disable validation of integrity (psy_dinteg)
**    N     73 (ps201)  -   in SQL disable error return when a duplicate key
**			    or duplicate row error would normally be returned.
**			    Instead just return 0 row count as is done in QUEL.
**			    This has the benefit that in 1.1 open cursors will
**			    not be closed as a result of the error.
**    N	    74 (ps202)
**    Y     75 (ps203)  -   Call ulm_mappool() and ulm_print_pool() to dump
**                          PSF memory in ULM.
**
**    	NOTE: trace points 236-255 allow values to be assigned,
**    	      e.g. "set trace point ps251 2"
**
**    N    123 (ps251)  -   disable single-quoted identifiers in the SQL grammar
**			    (in the user_ident productions).  We can use this
**			    to test our FE apps to make sure they remove uses of
**			    single-quoted identifiers before the 6.5+1 release.
**			    Users can use this, too.
**
**  -	    -	    -	    -	    -	    -	    -	    -	    -	    -
**
**  xDEBUG == Y	    <==> will fire only if xDEBUG wa defined
**  xDEBUG == N	    <==> will always fire
**
** -----------------------------------------------------------------------------
*/
#define	    PSS_YACC_SHIFT_REDUCE_TRACE	    0	/* PS128 */
#define	    PSS_PRINT_QRY_TRACE		    1
#define	    PSS_TBL_VIEW_GRANT_TRACE	    3
#define	    PSS_REPEAT_QRY_SHARE_TRACE	    4
#define	    PSS_DBPROC_GRNTBL_ACTIVE_TRACE  5
#define	    PSS_CURSOR_INFO_TRACE	    13	/* PS141 */
#define	    PSS_DBPROC_TREE_BY_LINK_TRACE   14
#define     PSS_DBPROC_TREE_BY_NEXT_TRACE   15
#define	    PSS_LONG_QRY_TREE_TRACE	    16
#define     PSS_SHORT_QRY_TREE_TRACE        17	/* PS145 */
#define	    PSS_NEW_NODE_SUBTREE_TRACE	    18
#define     PSS_SCANNER_UNORM               19  /* PS147 */
#define	    PSS_OPNODE_CHILDREN_TRACE	    21	/* PS149 */
#define	    PSS_ENABLE_DROP_SCHEMA	    22
#define	    PSS_AMBREP_64COMPAT		    23  /* PS151 */
#define	    PSS_PRINT_PSS_CONS		    47	/* PS175 */
#define	    PSS_GENERATE_NOT_NULL_CONS	    48
#define	    PSS_ALLOW_SET_PROC_STMT_RULE    49	/* PS177 */
#define	    PSS_PRIVILEGE_CHECK_TRACE	    50
#define	    PSS_PRINT_PERM_QUAL_TREE_TRACE  51
#define	    PSS_NO_INTEG_VALIDATION_TRACE   72	/* PS200 */
#define	    PSS_ENABLE_NOERR_ON_DUPS	    73	/* PS201 */
#define	    PSS_VARCHAR_PRECEDENCE	    74	/* PS202 */
#define	    PSS_ULM_DUMP_POOL               75  /* PS203 */
#define	    PSS_NO_SINGLE_QUOTED_IDS       123	/* PS251 */

/*
**  Definitions for COPY
*/
#define	    CPY_NAME	1
#define	    CPY_SCONST	2
#define	    CPY_INTEGER	3
#define	    CPY_QDATA	4

/*
**  Memory allocation stream definitions.
*/

/*
** Amount of memory to allocate in memory pool per possible session.
*/

#define                 PSF_SESMEM      102400L

/*
** Minimum amount of memory to allocate in memory pool per possible session.
*/

#define                 PSF_SESMEM_MIN      25000L

/*
** Name: PSF_MSTREAM - Memory stream.
**
** Description:
**      This structure defines a memory stream.  Three operations are possible
**      on memory streams: open, allocate, and close.  Each stream can allocate
**	one of three types of QSF objects: query text, query tree, or query
**	plan.
**
** History:
**     30-dec-85 (jeff)
**          written
**     30-may-86 (jeff)
**	    re-worked for use with QSF
*/
typedef struct _PSF_MSTREAM
{
    QSO_OBID	    psf_mstream;	/* The QSF object id */
    i4		    psf_mlock;		/* The QSF object lock id */
}   PSF_MSTREAM;

/*
** Name: PSS_ID_TYPE - vars of this type should be used to describe 
**		       identifier type
**
** Description:
**	vars of this type can be used to contain type of identifier
**
** History:
**     	17-aug-93 (andre)
**	    defined
*/
typedef		i4	PSS_ID_TYPE;

					/* regular identifier */
#define	PSS_ID_REG		1	

					/* delimited identifier */
#define	PSS_ID_DELIM		2	

					/* 
					** identifier expressed as a quoted 
					** string 
					*/
#define	PSS_ID_SCONST		3	

					/*
					** special identifier $DBA - used to
					** represent DBA's default schema
					*/
#define	PSS_ID_DBA		4

					/*
					** special identifier $INGRES - used
					** to represent default schema owned by
					** the catalog owner 
					*/
#define	PSS_ID_INGRES		5

					/*
					** regular identifier having special 
					** meaning in the context of parsing 
					** the current statement
					*/
#define	PSS_ID_NONKEY		6

# ifdef NO
/*
** Name: PSF_CURNAME - Cursor name
**
** Description:
**	This defines a cursor name as seen by the front end.
**
** History:
**      31-dec-85 (jeff)
**          written
*/
typedef char PSF_CURNAME[DB_CURSOR_MAXNAME]; /* cursor name */
# endif

/*
** Name: PSC_RESCOL - Description of result column
**
** Description:
**      This structure describes a result column for a cursor.  Not everything
**      about the column is stored here; some of it is stored in bit maps in the
**      main part of the cursor control block.
**
** History:
**     08-oct-85 (jeff)
**          written
*/
typedef struct _PSC_RESCOL
{
    DB_DT_ID          psc_type;           /* Datatype id */
    i4                psc_len;            /* Length of data value */
    i2		      psc_prec;		  /* Precision of data value */
    DB_ATT_ID         psc_attid;          /* Column id */
    DB_ATT_NAME       psc_attname;        /* Column name */
    struct _PSC_RESCOL *psc_colnext;	  /* Pointer to next column in bucket */
} PSC_RESCOL;

/*
** Name: PSC_RESTAB - Information about a result column for a cursor.
**
** Description:
**	This structure defines the information that is kept about the set of
**	result columns for a cursor.  This is useful when doing updates; one
**	needs to know the datatypes in order to resolve expressions, and one
**	needs to know whether the column is updateable (e.g. is it a column in
**	a base table or is it the result of an expression?).
**
**	The information is kept in a hash table.  The size of the hash table
**	varies with the number of columns in the result.  The number of slots
**	in the hash table will always be at least twice as many as the number
**	of columns in the result.  Each slot is a linked list of column
**	descriptions.
**
** History:
**     08-oct-85 (jeff)
**          written
*/
typedef struct
{
    i4              psc_tabsize;        /* Number of slots in hash table */
    PSC_RESCOL      **psc_coltab;	/* Hash table of column descrs. */
} PSC_RESTAB;

/*
** Name: PSC_TBL_DESCR - element of a list of descriptions of table and views
**			 over which a cursor was defined
**
** Description:
**	Since now rules can be defined on view as well as tables, the old method
**	of building a single list of rules becomes impractical.  Instead, while
**	processing DECLARE CURSOR for an updatable cursor, we will build a
**	PSC_TBL_DESCR structure for every relation over which a cursor has been
**	defined (i.e. if a cursor has been defined over view V defined over
**	table T, both V and T will be entered into that list.)  Initially, the
**	list will be used to keep track of rules which are applicable to cursor
**	UPDATE/DELETE, but its uses may expand in future.)  During processing of
**	the first DELETE/UPDATE WHERE CURRENT OF <cursor>, we will attach rules
**	to appropriate elements of the list.
**
**	To determine which rules are to be attached to the query tree of
**	UPDATE/REPLACE CURSOR, we will traverse list of rules hanging of the
**	PSC_TBL_DESCR structure corresponding to the table or view whose
**	description is pointed at by sess_cb->pss_resrng.
**
**	When processing DELETE CURSOR, we will simply loop through all elements
**	of the list and attach all rules which fire after DELETE - thus saving
**	the expense of going through the full-blown qrymod.
**
** History:
**	05-apr-93 (andre)
**	    defined
**	15-june-06 (dougi)
**	    Add before versions of all the rules.
*/
typedef struct PSC_TBL_DESCR_
{
    QUEUE		psc_queue;		/* next/prev headers */
    DB_TAB_ID		psc_tabid;		/* id of the table */
    i4		psc_tbl_mask;		/* relstat of the table */
    PST_STATEMENT	*psc_row_lvl_usr_rules;
    PST_STATEMENT	*psc_row_lvl_sys_rules;
    PST_STATEMENT	*psc_stmt_lvl_usr_rules;
    PST_STATEMENT	*psc_stmt_lvl_sys_rules;
    PST_STATEMENT	*psc_row_lvl_usr_before_rules;
    PST_STATEMENT	*psc_row_lvl_sys_before_rules;
    PST_STATEMENT	*psc_stmt_lvl_usr_before_rules;
    PST_STATEMENT	*psc_stmt_lvl_sys_before_rules;
    i4			psc_flags;

#define	    PSC_RULES_CHECKED		0x01
} PSC_TBL_DESCR;
/*
** Name: PSC_CURBLK - Cursor control block.
**
** Description:
**      This structure defines a cursor control block.  One is created every
**      time a cursor is opened or a repeat cursor is defined.  Each parser
**      session stores its cursor control blocks in a hash table, where each
**      bucket of the hash table is a linked list of cursor control blocks.
**
** History:
**	08-oct-85 (jeff)
**	    Written
**	18-apr-89 (neil)
**	    Rules: Added psc_lang, psc_tbl_mask, psc_rchecked and psc_rules
**	    to support rules for cursors.
**	22-dec-92 (rblumer)
**	    Added psc_stmt_rules for statement-level rules (CONSTRAINTS).
**	10-mar-93 (andre)
**	    added psc_expmap
**	31-mar-93 (andre)
**	    (fix for bugs 50823 & 50825)
**	    added psc_flags and defined PSC_MAY_RETR_ALL_COLUMNS
**	05-apr-93 (andre)
**	    removed psc_rules and psc_stmt_rules - instead of a single list we
**	    will have separate lists corresponding to various tables/views over
**	    which the cursor is defined
**
**	    removed psc_tabid - it was never used; 6.4 fix for bugs 50825 and
**	    50899 uses that field during DELETE CURSOR processing because in 6.4
**	    we can rely on the fact that rules will be defined only on base
**	    tables - it is no longer the case in 6.5
**
**	    migrated psc_tbl_mask into PSC_TBL_DESCR
**
**	    deleted psc_rchecked - its function will be performed by
**	    PSC_RULES_CHECKED defined over PSC_TBL_DESCR.psc_flags
**
**	    added psc_tbl_descr_queue
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _PSC_CURBLK
{
    struct _PSC_CURBLK *psc_next;	/* Each hash bucket is que of cursors */
    struct _PSC_CURBLK *psc_prev;	/* backward pointer */
    SIZE_TYPE	    psc_length;		/* size of this structure */
    i2		    psc_type;		/* type code for this structure */
#define                 PSC_CRBID      0   /* 0 until real code assigned */
    i2		    psc_s_reserved;	/* possibly used by memory mgmt. */
    PTR		    psc_l_reserved;
    PTR		    psc_owner;		/* owner for internal memory mgmt. */
    i4		    psc_ascii_id;	/* so as to see in a dump */
#define                 PSCCUR_ID       CV_C_CONST_MACRO('P', 'S', 'C', 'B')
    DB_CURSOR_ID    psc_blkid;          /* DBMS Cursor id */
    DB_CURSOR_ID    psc_curid;		/* Cursor id as seen by front end */
#ifdef NO
    PSF_CURNAME     psc_curname;        /* Cursor name as seen by front end */
#endif
    i4		    psc_used;		/* TRUE iff this cb is being used */
    PTR		    psc_stream;		/* Mem stream used to alloc this CB */
    DB_LANG	    psc_lang;		/* Query language of original
					** definition.  Save this as DELETE/
					** FETCH/CLOSE cursor do not have a
					** query language indicator in SCF.
					*/
    PSC_COLMAP      psc_updmap;         /* Map of cols in "for update" clause */
    i4              psc_delall;         /* TRUE iff delete permit given */
    i4              psc_forupd;         /* TRUE iff "for update" clause given */
    i4              psc_readonly;       /* TRUE iff "read only" clause given */
    i4		    psc_rdonly;		/* TRUE iff "read only" clause or if 
					** joins or sorts exist in query.
					*/
    PSC_RESTAB      psc_restab;         /* Result column descriptions */
    i4              psc_repeat;         /* TRUE iff this is a repeat cursor */
    i4              psc_open;           /* TRUE iff this cursor is open */
    PST_QNODE	    *psc_integ;		/* Integrity qualification */
    DB_TAB_NAME	    psc_tabnm;		/* Table name for deletes, updates */
    DB_OWN_NAME	    psc_ownnm;		/* name of the table owner */
    PSC_COLMAP      psc_iupdmap;        /* Internal map of cols in "for update"
					** clause (after QRYMOD).
					*/
    PSC_COLMAP	    psc_expmap;		/*
					** a map of attributes which are based
					** on expressions.  This is only
					** relevant when a user declared a
					** cursor on a view without specifying
					** FOR UPDATE, ORDER BY, or FOR READONLY
					** and the <query expression> contained
					** in the <cursor declaration> is
					** updatable and the view itself is
					** updatable according to INGRES rules
					** (i.e. no UNIONs, DISTINCTs,
					** aggregates, GROUP BY, or HAVING, but
					** some columns may be based on an
					** expression and, therefore, cannot be
					** updated.)  We will allow updating of
					** attributes of the view which were not
					** based on expressions and for which
					** the user possesses UPDATE privilege.
					** psc_updmap will tell us whether a
					** user possesses UPDATE on a given
					** attribute while psc_expmap will tell
					** us whether a given attribute is based
					** on an expression (and, thus, cannot
					** be updated.)
					*/
    QUEUE	    psc_tbl_descr_queue; /*
					** ptrs to begginning/end of a
					** list of rules headers of type
					** PSC_TBL_DESCR describing rules that
					** apply to every view and the base
					** table over which the cursor is
					** defined
					*/
    i4		    psc_flags;		/* useful bits of information */

					/*
					** this bit will be set if a cursor was
					** declared in QUEL and we have
					** determined that the user possesses
					** unconditional RETRIEVE privilege on
					** every column in the table; knowing
					** this will save us the expense of
					** running qrymod over REPLACE CURSOR
					** query tree
					*/
#define	PSC_MAY_RETR_ALL_COLUMNS    0x0001
} PSC_CURBLK;

/*
** Name:  - Cursor update mode mask.
**
** Description:
**      This type is used in cursor related code ande defines
**	the updateability type for a cursor.
**
** History:
**     09-nov-88 (stec)
**          written
*/
typedef i4  PSC_UPDM;
#define		PST_DL	    0x01    /* deleteable */
#define		PST_UP	    0x02    /* updateable */

/*
** Query tree allocation block size.
*/
#define                 PST_QBSIZE      4096

/*
**  Session control block definition.
*/

#define                 PSS_CURTABSIZE  127	/* Size of cursor hash table */
#define                 PSS_YYMAXDEPTH  150	/* Depth of yacc stack */
#define                 PSS_COLTABSIZE  31	/* Size of column hash table */
#define			PSS_MAXCURS	64	/* Maximum number of cursors
						** we want defined at a time
						*/

/*
**  Table and attribute description information.
*/

#define			PST_SHWNAME	0x01 /* Show table by name */
#define			PST_SHWID	0x02 /* Show table by tab. id. */
#define			PST_CHECK_EXIST 0x04 /* check if exists only */
#define			PST_DTBL	0x08 /* Table will be dropped */
#define			PST_REMOVE	0x10 /* Remove link w/o checking */
					     /*
					     ** looking for a DDB object which,
					     **	if it exists, is a catalog
					     */
#define			PST_IS_CATALOG	0x20
#define			PST_REGPROC	0x40 /* look up procedure name space 
						only */
#define			PST_FULLSEARCH	0x80 /* look up both table and procedure
						name space. If this is set,
						PST_REGPROC should not be set */

/*
** Name: PSS_RNGTAB - Element in user range table LRU queue.
**
** Description:
**      This structure defines an element in the user range table's LRU queue.
**
** History:
**     09-oct-85 (jeff)
**          written
**     04-aug-86 (jeff)
**	    upgraded to new RDF interface (pointer to array of pointers)
**	07-mar-89 (neil)
**	    Rules: Added pss_image.
**	27-sep-89 (andre)
**	    Added pss_outer_rel and pss_inner_rel to keep track of inner and
**	    outer relations of outer joins.
**	12-dec-89 (andre)
**	    added pss_var_mask
**	16-sep-91 (andre)
**	    defined PSS_IN_INDEP_OBJ_LIST
**	26-sep-92 (andre)
**	    defined PSS_EXPLICIT_CORR_NAME over pss_var_mask
**	18-dec-92 (andre)
**	    defined PSS_RULE_RNG_VAR
**	04-jan-93 (andre)
**	    defined PSS_CHECK_RULES
**	11-feb-93 (andre)
**	    defined PSS_TID_REFERENCE
**	29-apr-93 (markg)
**	    defined PSS_DOAUDIT
**	26-jun-93 (andre)
**	    defined PSS_ATTACHED_CASC_CHECK_OPT_RULE over pss_var_mask
**	02-jul-93 (rblumer)
**	    added sess_cb & dbpinfo parameters to psl_ct18s_type_qual prototype.
**	16-jul-93 (rblumer)
**	    added missing prototype for pst_crt_schema function.
**	21-aug-93 (andre)
**	    defined PSS_NO_UBT_ID over pss_var_mask
**	21-oct-93 (andre)
**	    defined PSS_REFERENCED_THRU_SYN_IN_DBP and added pss_syn_id   
**	5-jan-2007 (dougi)
**	    Add with clause element type and flag.
*/
typedef struct _PSS_RNGTAB
{
    QUEUE           pss_rngque;         /* Queue pointers */
    i4              pss_used;           /* TRUE iff this slot is used */
    i4		    pss_rgno;		/* Range variable number */
    i4		    pss_permit;		/* used to mark permission to use */
    char            pss_rgname[DB_TAB_MAXNAME]; /* Name of the range variable */
    i4		    pss_rgtype;		/* type of range var */
	/*   uses the following definitions from PST_RNGENTRY in psfparse.h
	**	PST_TABLE 1 	-- table. 
	**	PST_RTREE 2	-- QTREE. the table id and rdrinfo block
	**			   is not valid. pss_qtree has the
	**			   qnode tree that materializes this
	**			   table.
	**	PST_SETINPUT 3	-- temporary table, used in set-input dbprocs;
	**			   tab_id will be filled in, but will only be
	**			   valid when RECREATEing the procedure
	**	PST_DRTREE 4	-- derived table (subselect in FROM clause)
	**	PST_WETREE 5	-- WITH list element (a.k.a. common table expr)
	**	PST_TPROC  6	-- table procedure entry. pss_qtree anchors
	**			   RESDOM list of parameter specifications
	*/
    i4		    pss_rgparent;	/* normally set to -1, this field has
					** the range var number of the view
					** table that caused the addition of
					** this entry.
					*/
    PST_QNODE	    *pss_qtree;		/* query that produces this table */
    DB_TAB_ID       pss_tabid;          /* Table id of the range variable */
    DB_TAB_NAME	    pss_tabname;	/* Name of corresponding table */
    DB_OWN_NAME	    pss_ownname;	/* Owner of table */
    RDD_ATTHSH	    *pss_colhsh;	/* Hash table of column descriptions */
    DMT_TBL_ENTRY   *pss_tabdesc;	/* Description of table */
    DMT_ATT_ENTRY   **pss_attdesc;	/* Decsriptions of columns */
    RDR_INFO	    *pss_rdrinfo;	/* Pointer to RDF information block */
    i4		    pss_image;		/* Range var references table image data
					** before/after an update.  This is used
					** by the CREATE RULE statement to fill
					** the PST_RL_NODE (RULEVAR) nodes for
					** OLD and NEW range variables. Values
					** are from RULVAR nodes:
					**	PST_BEFORE	1
					**	PST_AFTER	2
					*/
    PST_J_MASK	    pss_outer_rel;	/*
					** if n-th bit is ON, this relation is
					** an outer relation in the join with ID
					** equal to n
					*/
    PST_J_MASK	    pss_inner_rel;	/*
					** if n-th bit is ON, this relation is
					** an inner relation in the join with ID
					** equal to n
					*/
    i4		    pss_var_mask;
				    /*
				    ** table name was explicitly qualified
				    */
#define	    PSS_EXPLICIT_QUAL	    ((i4) 0x01)

				    /*
				    ** this range var or a table in whose
				    ** definition it was used has been added to
				    ** the list of independent objects
				    */
#define	    PSS_IN_INDEP_OBJ_LIST   ((i4) 0x02)

				    /* correlation name was specified */
#define	    PSS_EXPLICIT_CORR_NAME  ((i4) 0x04)

				    /*
				    ** rule is being defined on this variable or
				    ** this variable represents the underlying
				    ** table of a view on which a rule is being
				    ** defined
				    */
#define	    PSS_RULE_RNG_VAR	    ((i4) 0x08)
				    /*
				    ** check whether any rules apply to this
				    ** table/view
				    */
#define	    PSS_CHECK_RULES	    ((i4) 0x10)
				    /*
				    ** the range entry represents a view V and
				    ** a query contained reference to V.TID; in
				    ** psy_view(), if this mask is set, we will
				    ** verify that a view is updatable and if
				    ** not, issue an error
				    */
#define	    PSS_TID_REFERENCE	    ((i4) 0x20)
				    /*
				    ** table entry needs to be audited.
				    */
#define     PSS_DOAUDIT		    ((i4) 0x40)

				    /*
				    ** will be set if a rule enforcing CASCADED
				    ** CHECK OPTION on a view represented by
				    ** this range variable or a view defined
				    ** over the view represented by this range
				    ** variable has been attached to the query
				    ** tree
				    **
				    ** The rationale here is that as long as we
				    ** ensure that CASCADED CHECK OPTION has not
				    ** been violated on the highest level view
				    ** of those defined with CASCADED CHECK
				    ** OPTION, we can claim that it has not been
				    ** violated on any of its underlying views.
				    */
#define	    PSS_ATTACHED_CASC_CHECK_OPT_RULE	((i4) 0x80)

				    /*
				    ** will be set if the range entry describes
				    ** a view and we checked and determined that
				    ** base tables (if any) over which it was 
				    ** defined were all core catalogs (i.e. we 
				    ** cannot invalidate QEPs invoilving a view
				    ** which is being dropped or privileges on 
				    ** which are being revoked by changing 
				    ** timestamp of a bse table (if any) over 
				    ** which this view was defined
				    */
#define	    PSS_NO_UBT_ID			((i4) 0x0100)

				    /*
				    ** will be set if a table/view/index 
				    ** represented by this entry was referenced
				    ** by user through a synonym; 
				    ** NOTE: since getting synonym id represents
				    ** an additional expense and, at least for 
				    ** now, we are interested in knowing synonym
				    ** ids only for describing dependence of a 
				    ** dbproc on synonyms used in its 
				    ** definition, this flag will NOT be set 
				    ** unless we are processing definition of 
				    ** a dbproc
				    */
#define	    PSS_REFERENCED_THRU_SYN_IN_DBP	((i4) 0x0200)

                                    /*
                                    ** will be set if this entry is the inner
                                    ** in a left, right or full join. Used when
                                    ** setting the nullability of resdoms in
                                    ** prepared OJ selects.
                                    */
#define     PSS_INNER_RNGVAR                    ((i4) 0x0400)
				    /*
				    ** with element has been referenced 
				    ** somewhere else in the query - subsequent
				    ** references require it to be copied.
				    */
#define	    PSS_WE_REFED			((i4) 0x800)

    DB_TAB_ID       pss_syn_id;     /*
                                    ** if PSS_REFERENCED_THRU_SYN_IN_DBP is set,
                                    ** this field will contain synonym's id
                                    ** otherwise, its value is undefined
                                    */
} PSS_RNGTAB;

/*
** Name: PSS_USRRANGE - User range table
**
** Description:
**      This structure defines the user range table.  It contains the
**      names and table ids of the range variables declared by the user.
**	Range variables are replaced on an LRU basis.
**
** History:
**     09-oct-85 (jeff)
**          written
*/
typedef struct _PSS_USRRANGE
{
    QUEUE	    pss_qhead;		/* Head of LRU queue of range vars */
    PSS_RNGTAB	    pss_rngtab[PST_NUMVARS]; /* Elements in queue */
    PSS_RNGTAB	    pss_rsrng;		/* Result range variable */
    i4		    pss_maxrng;		/* Maximum range var # used so far */
} PSS_USRRANGE;

/*
** Name: PSS_HINTENT - Element in table/index hint list.
**
** Description:
**      This structure defines an entry in the list of table oriented hints.
**
** History:
**	17-mar-06 (dougi)
**	    Written for optimizer hints feature.
*/
typedef struct _PSS_HINTENT
{
    DB_TAB_NAME	    pss_tabname1;	/* table name */
    DB_TAB_NAME	    pss_tabname2;	/* table name (if join hint) or
					** index name (if index hint) */
    i4		    pss_hintcode;	/* hint type */
} PSS_HINTENT;

/*
** Name: PSS_CURSTAB - Hash table of cursor control blocks.
**
** Description:
**      This structure defines the hash table of cursor control blocks that
**      exists for each session.  Since cursors are continually created and
**      destroyed, it uses hash buckets instead of open addressing.  Each hash
**      bucket consists of a QUEUE of cursor control blocks, which can be
**      inserted or removed using QUinsert and QUremove.
**
** History:
**     09-oct-85 (jeff)
**          written
**	08-oct-87 (stec)
**	    Changed size of PSS_SBSIZE.
*/
typedef struct _PSS_CURSTAB
{
    PSC_CURBLK      *pss_curque[PSS_CURTABSIZE]; /* Table of hash buckets */
} PSS_CURSTAB;

/*
** Number of bytes of data in a symbol table block.
*/
#define                 PSS_SBSIZE      DB_MAXSTRING + DB_CNTSIZE

/*
** Name: PSS_SYMBLK - Symbol table block.
**
** Description:
**      This structure defines a symbol table block.  Symbol table blocks
**      are connected in linked lists.  The scanner puts semantic values in
**	the symbol table, which is composed of these blocks.  When the current
**	block runs out, it goes on to the next one in the list, or allocates a
**	new one if there are no more.  The symbol table won't shrink once it's
**	been expanded.
**
** History:
**     31-jan-86 (jeff)
**          written
*/
typedef struct _PSS_SYMBLK
{
    struct _PSS_SYMBLK    *pss_sbnext;  /* Pointer to the next block in chain */
    u_char          pss_symdata[PSS_SBSIZE]; /* Symbol table data */
}   PSS_SYMBLK;

/*
** Name: PST_PROTO - Prototype Tree Header for Dynamic SQL.
**
** Description:
[@comment_line@]...
**
** History:
**      28-may-87 (puree)
**	    add pst_tblq, pst_usrq, pst_colq to save the values of
**	    the end-of-queue for table, user, column queues.
**	10-nov-87 (puree)
**	    Added pst_qrange to save the range varable table of the 
**	    prepared query.  This range table is needed for qrymod.
**	23-aug-88 (stec)
**	    Fix LINT warnings.
**	22-sep-88 (stec)
**	    Save update flag.
**	31-oct-88 (stec)
**	    Save update column list (tree).
**	11-may-89 (neil)
**	    Added pst_cursid for Dynamic SQL cursor support.
**	17-apr-91 (andre)
**	    Added pst_num_parmptrs to keep track of the number of pointers
**	    pointed to by pst_plist.  As a consequence of the fix for bug 33598,
**	    number this number can be different from pst_maxparm+1
**	30-may-91 (andre)
**	    Added pst_ret_flag for handling dynamically prepared
**	    SET USER/GROUP/ROLE.
**	    Added pst_user, pst_group, and pst_aplid.  These new fields will
**	    contain the user, group, and role id which were in effect at the
**	    time the statement was prepared.
**	12-nov-91 (rblumer)
**        merged from 6.4:  23-aug-91 (bonobo)
**	    Fixed typo causing su4 compiler warnings.
**	02-dec-92 (andre)
**	    rename pst_user to pst_eff_user, remove pst_group and pst_aplid, add
**	    pst_user (pst_user used to contain effective user id at PREPARE time
**	    - now pst_eff_user will contain effective user id at PREPARE time,
**	    and pst_user will contain a copy of PSQ_CB.psq_user)
**	11-mar-93 (andre)
**	    defined pst_stmt_flags
**	15-nov-93 (andre)
**	    defined pst_flattening_flags
**      05-Nov-98 (hanal04)
**          Added new element pst_view_maxrng as this structure has a
**          one to one mapping with a statement. The sess_cb may
**          execute many prepared statements and is not therefore
**          suitable for long term storage of this value. b90168.
**	16-Jun-1999 (johco01)
**	    Cross integrate change 439089 (gygro01)
**	    Took out previous change 438631 because it introduced bug 94175.
**	    Change 437081 (inkdo01) solves bugs 90168 and 94175.
**	16-oct-2007 (dougi)
**	    Add pst_checksum for repeat dynamic select.
*/
typedef struct _PST_PROTO
{
    struct _PST_PROTO	*pst_next;	    /* next header block */
    PSF_MSTREAM		pst_ostream;	    /* stream for prototype object */
    PSF_MSTREAM		pst_cbstream;	    /* stream for prototype CB */
    PSF_MSTREAM		pst_tstream;	    /* stream for prototype text */
    i4			pst_mode;	    /* query mode */
    i4			pst_clsall;	    /* close all cursors */
    i4			pst_alldelupd;	    /* */
    i4			pst_maxparm;	    /* # of parm markers */
    i4			pst_num_parmptrs;   /* # of pointers pointed to by
					    ** pst_plist
					    */
    DB_DATA_VALUE	**pst_plist;	    /* ptr to array of parm pointers */
    QSO_OBID		pst_result;	    /* QSF result of parsing query */
    DB_TAB_ID		pst_table;	    /* Table id for deleting rng vars */
    char		pst_sname[DB_MAXNAME+1];  /* SQL statement name string */
    bool		pst_nupdt;	    /* copy of nonupdt from yyvars.h */
    bool		pst_repeat;	    /* TRUE - repeat dynamic select */
    PST_QNODE		*pst_updcollst;	    /* ptr to update col subtree */
    DB_CURSOR_ID	pst_cursid;	    /* Dynamic SQL cursor support */
    i4		pst_ret_flag;	    /* will contain a copy of
					    ** PSQ_CB.psq_ret_flag - flags to be
					    ** returned to the caller of
					    ** psq_call()
					    */
    DB_OWN_NAME		pst_eff_user;	    /* user id which was in effect when
					    ** the statement was prepared.
					    */
    DB_TAB_OWN		pst_user;	    /* copy of PSQ_CB.psq_user */
    i4		pst_stmt_flags;	    /* copy of selected bits from 
					    ** sess_cb->pss_stmt_flags
					    */
					    /*
					    ** copy of 
					    ** sess_cb->pss_flattening_flags
					    */
    i4  		pst_flattening_flags;
    DD_LDB_DESC         pst_ldbdesc;        /*
					    ** contains node, database
			 		    ** for COPY TABLE
					    */
    PSQ_STMT_INFO	pst_stmt_info;      /* init for prepared inserts */
    i4			pst_checksum;	    /* checksum for repeat select or 0 */
    char		*pst_syntax;	    /* ptr to actual syntax */
    i4			pst_syntax_length;
} PST_PROTO;

typedef struct _PSS_DECVAR PSS_DECVAR;  /* forward declaration */
/*
** Name: PSS_SESBLK - Session control block
**
** Description:
**      This structure defines a session control block for the parser.  It
**	contains everything about the session as a whole.  One of these is
**      allocated every time a parser session is started.  Each session control
**      block can point to one or more cursor control blocks.  It is possible to
**	parse an "alternate" query; that is, one can parse a query while another
**	go block is being parsed.  There are several alternate fields in this
**	structure to allow this to happen without interfering with the go block
**	in progress.
**
** History:
**     09-oct-85 (jeff)
**          written
**     18-mar-87 (stec)
**          Added pss_resjour to PSS_SESBLK.
**     11-may-87 (stec)
**          Added pss_udbid to PSS_SESBLK.
**     04-sep-87 (stec)
**	    Added pss_crsid to PSS_SESBLK.
**     02-oct-87 (stec)
**	    Grouped result table info in PST_RESTAB structure.
**	23-aug-88 (stec)
**	    Fix LINT warning pss_ruser -> pss_rusr.
**	10-nov-88 (stec)
**	    Changed pss_udbid from PTR to I4.
**	26-jan-89 (neil)
**	    Added pss_rules node for rules tree processed in query mod.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added pss_group for session's group id;
**	    Added pss_aplid for session's application id.
**	08-mar-89 (andre)
**	    added pss_dba_drop_all to PSS_SESBLK.
**	25-apr-89 (andre)
**	    Added PSS_IPROC.
**	06-jun-89 (ralph)
**	    Changed pss_group and pss_aplid to DB_TAB OWN to eliminate
**	    unix compile problems.
**      24-jul-89 (jennifer)
**          Added pss_audit to point to audit information for 
**          a statement.
**	27-jul-89 (jrb)
**	    Added pss_num_literals so PSF will know which set of numeric literal
**	    scanning rules to use.
**	11-oct-89 (ralph)
**	    Add pss_rgroup, pss_rgset, pss_raplid , pss_raset to store
**	    group/role state when compiling database procedures.
**	16-oct-89 (ralph)
**	    Changed pss_rgroup and pss_raplid to DB_TAB_OWN
**	27-oct-89 (ralph)
**	    Add pss_ustat to PSS_SESBLK.
**	28-dec-89 (andre)
**	    Added pss_fips_mode to PSS_SESBLK.
**	1-feb-90 (andre)
**	    Added PSS_DBPGRANT_OK.
**	2-mar-90 (andre)
**	    Defined PSS_VGRANT_OK over pss_mask.
**	07-jun-90 (andre)
**	    defined PSS_1DBP_MUSTBE_GRANTABLE.
**	11-sep-90 (teresa)
**	    changed several bools/nat type of flags to become bitmasks in 
**	    pss_flag or pss_ses_flag in PSS_SESBLK and fixed faulty definition 
**	    of pss_retry (which is really a tri-state flag, not a boolean), 
**	    and removed obsolete pss_embedded.
**	1-nov-90 (andre)
**	    replace pss_flag with pss_stmt_flags and pst_dbp_flags.
**	    pss_stmt_flags will persist throughout parsing of one statement.
**	    It will be initialized to 0 before calling the grammar and after
**	    every DML statement parsed as a part of a database procedure (the
**	    only exception that I can think of is PSS_TXTEMIT when parsing a new
**	    dbproc).
**	    pss_dbp_flags will persist throughout parsing of a dbproc.  It will
**	    be initialized to 0 before calling the garmmar and will be
**	    initialized appropriately if trying to recreate a dbproc.  While
**	    inside the grammar, this flag will most likely not be reinitialized
**	    wholesale, but, instead, certain bits may be turned ON or OFF as
**	    appropriate.
**	21-jan-91 (andre)
**	    defined PSS_QUAL_IN_PERMS over pss_stmt_flags; this flag will be set
**	    to indicate that permit(s) with non-NULL qualification was/were used
**	    to enable user to execute this query.  This is one of the changes
**	    made to support QUEL shareable repeat queries.
**	24-jan-91 (andre)
**	    defined PSS_QUEL_RPTQRY_TEXT over pss_stmt_flags; this flag is used
**	    to indicate that we are collecting text of a QUEL repeat query which
**	    so far appears to be shareable
**	19-feb-91 (andre)
**	    defined PSS_DISREGARD_GROUP_ROLE_PERMS over pss_stmt_flags;
**	    this is done as a part of fix for bug #32457
**	20-feb-91 (andre)
**	    Undefined pss_raset and pss_rgset; removed pss_raplid and pss_rgroup
**	    since group and role permits will no longer be considered when
**	    parsing definitions of views and database procedures, and the
**	    above-mentioned fields and constants were defined to avoid use of a
**	    current group/role identifiers when recreating database procedures.
**	    This was done as a part of fix for bug #32457
**	17-may-91 (andre)
**	    Added a new field, pss_dbaname, to contain a NULL-terminated DBA
**	    name.  It will be used when processing $dba.
**	12-jun-91 (andre)
**	    Defined PSS_PARSING_PRIVS over pss_stmt_flags.  This flag will be
**	    set when we are parsing a list of privileges found in GRANT
**	    statement.  When this flag is set, SQL scanner will interpret
**	    single keywords REGISTER and RAISE as keywords, while the rest
**	    of the time they will be treated as simple identifiers unless
**	    they occur as a part of a double keyword.
**	08-aug-91 (andre)
**	    Undefined PSS_VGRANT_OK since psy_dview() now obtains this
**	    information directly from psy_view() (via psy_qrymod()).
**	11-sep-91 (andre)
**	    Added pss_indep_objs to PSS_SESBLK.
**	27-sep-91 (andre)
**	    Added pss_dependencies_stream.  This is a pointer to the stream
**	    which will be used to build a list of independent objects when
**	    parsing a definition of a view or a database procedure or when
**	    checking if a user can grant a permit on his database procedure or a
**	    view.
**
**	    When a definition of a view or a database procedure is parsed in the
**	    course of creation (or for dbprocs, possibly recreation) of the
**	    object, pss_dependencies_stream will contain the address of the
**	    stream used to allocate a tree representing a view ir a dbproc (i.e.
**	    pss_ostream).  When reparsing definitions of dbprocs to determine if
**	    a user can grant access to his dbproc, pss_dependencies_stream will
**	    contain address of a stream which will be open until, we are done
**	    processing the GRANT statement (unlike pss_ostream which will be
**	    closed upon successfully parsing each dbproc.)
**
**	    It is my intention that pss_dependencies_stream be initialized by
**	    the callers of psq_parseqry() (to &sess_cb->pss_ostream by
**	    psq_call() and to the address of some other stream in psy_dgrant())
**	    and be used in psy_view() and dopro(), but the responsibility for
**	    explicitly unlocking or closing the stream (as opposed to the case
**	    this will happen implicitly when
**	    pss_dependencies_stream==&sess_cb->pss_ostream) will lie with the
**	    function that initialized it (i.e. psy_dgrant()) or with the
**	    exception handler.
**	03-oct-91 (andre)
**	    added pss_dbpgrant_name.  This will contain address of the name of
**	    the dbproc being reparsed to determine if it is grantable.
**	12-nov-91 (rblumer)
**        merged from 6.4:  25-jul-91 (andre)
**	    added PSS_STRIP_NL_IN_STRCONST as a flag for pss_stmt_flags in 
**	    PSS_SESBLK; if set, PSF will strip NLs found inside string const's;
**	    this will be required if the session is with an older FE;
**	    this is required to fix bug 38098
**	26-feb-92 (andre)
**	    define PSS_REPAIR_SYSCAT over pss_ses_flag
**	20-mar-92 (Andre)
**	    Added PSS_MISSING_PRIV over PSS_SESBLK.pss_dbp_flags
**	06-apr-92 (andre)
**	    Added PSS_MISSING_OBJ over PSS_SESBLK.pss_dbp_flags
**	04-may-1992 (bryanp)
**	    Added pss_sess_owner to PSS_SESBLK for temp table enhancements. This
**	    field contains a session unique identifier which is used as the
**	    owner of session-specific temporary tables.
**	27-may-92 (andre)
**	    defined PSS_NEW_OBJ_NAME over pss_stmt_flags
**	03-aug-92 (barbara)
**	    Changed pss_retry to a bitfield.  The value that used to be
**	    known as PSS_RETRY is now PSS_ERR_ENCOUNTERED and indicates that
**	    PSF should retry the query.  A new value, PSS_REFRESH_CACHE,
**	    indicates that RDF must get fresh copies of requested objects.
**	    These changes are part of the new RDF cache flushing scheme.
**	29-sep-92 (andre)
**	    defined PSS_SINGLETON_SUBSELECT over pss_stmt_flags
**	28-oct-92 (andre)
**	    defined PSS_DSQL_QUERY over pss_stmt_flags
**	29-oct-92 (rblumer)
**	    defined PSS_PARSING_CHECK_CONS over pss_stmt_flags
**	07-nov-92 (andre)
**	    undefined PSS_1DBP_MUSTBE_GRANTABLE; since we claim that existsnce
**	    of a permit on a dbproc implies that it is grantable, this flag is
**	    no longer useful
**	20-nov-92 (andre)
**	    defined PSS_VALIDATING_CHECK_OPTION
**	24-nov-92 (ralph)
**	    CREATE SCHEMA:
**	    added pss_prvgoval to locate beginning of previous substatement
**	13-jan-93 (andre)
**	    defined PSS_RUNNING_UPGRADEDB
**	22-dec-92 (rblumer)
**	    Added pss_stmt_rules for statement-level rules (CONSTRAINTS);
**	    added pss_tchain2 and TXTEMIT2 for CHECK constraints.
**	    Added pss_dbprng for holding for set-input parameter.
**	05-jan-93 (rblumer)
**	    added fields for set-input parameters: setparmname, setparmq, &
**	    setparmno; added flags for set-input, system-generated and
**	    not-droppable procedures to pss_dbp_flag.
**	22-feb-93 (rblumer)
**	    added setparmlist field for passing set-input parameters to RDF.
**	05-mar-93 (ralph)
**	    DELIM_IDENT: Added pss_dbxlate to PSS_SESBLK to contain
**	    case translation masks.
**	11-mar-93 (andre)
**	    defined PSS_IMPL_COL_LIST_IN_DECL_CURS
**	02-apr-93 (ralph)
**	    DELIM_IDENT: Added pss_cat_owner to PSS_SESBLK to contain the
**	    name of the catalog owner for the database.  This will be
**	    "$ingres" in databases where regular identifiers are
**	    translated to lower case, and "$INGRES" in databases where
**	    regular identifiers are translated to upper case.
**	    Changed pss_dbxlate to be a pointer to a u_i4.
**	07-apr-93 (andre)
**	    until now we've been keeping two lists of rules: row level and
**	    statement level (with user-created rules preceding system-generated
**	    rules in both cases).  Given the fact that in the course if qrymod
**	    we may encounter rules (both user- and system-generated) defined on
**	    views, it would be more efficient to have four separate lists:
**	    row-level user-created, row-level system-generated, statement-level
**	    user-created (not in 6.5, but may happen later), abd statement-level
**	    system-generated.  To acomplish this, we will replace pss_rules and
**	    pss_stmt_rules with pss_row_lvl_usr_rules, pss_row_lvl_sys_rules,
**	    pss_stmt_lvl_usr_rules and pss_stmt_lvl_sys_rules.
**	07-may-93 (andre)
**	    having processed a statement containing constraint definitions (i.e.
**	    CREATE TABLE or ALTER TABLE ADD), we allocate a PST_STATEMENT
**	    structure for every constraint and call an appropriate function to
**	    translate information acumulated in PSS_CONS structure.  In many
**	    cases, it is convenient to be able to access the PST_STATEMENT
**	    structure allocated for the current constraint without having to
**	    pass around its pointer through several levels of functions.  Which
**	    has brought about pss_cur_cons_stmt..
**	    Also defined PSS_RESOLVING_CHECK_CONS
**	10-may-93 (barbara)
**	    Added another flag for distr_sflags field (PSS_1ST_ELEM) to
**	    help with Star code generation of CREATE TABLE statement.
**	13-aug-93 (rblumer)
**	    Added PSS_CALL_ADF_EXCEPTION flag for signaling when to call the
**	    ADF exception handler from inside PSF (i.e. only when doing math
**	    calculations on behalf of the user).
**	01-sep-93 (andre)
**	    added pss_dbp_ubt_id
**	08-sep-93 (swm)
**	    Changed session id type from i4 to CS_SID to match recent CL
**	    interface revision.
**	29-sep-93 (andre)
**	    reverted pss_psessid back to i4 - it is not used as a session 
**	    id in the same sense as pss_sessid
**	08-oct-93 (rblumer)
**	    Changed trace point vector to allow last 20 trace points
**	    (ps236-255) to allow values, with at most 5 set at one time;
**	    see PSS_TVALS and PSS_TVAO.
**	02-nov-93 (andre)
**	    added pss_flattening_flags to store bits of information used to 
**	    determine whether a given query is to be flattened.  
**	    PSS_SUBSEL_IN_OR_TREE, PSS_ALL_IN_TREE, and PSS_MULT_CORR_ATTRS, 
**	    which used to be defined over PSS_YYVARS.qry_mask will be defined 
**	    over pss_flattening_flags as will PSS_SINGLETON_SUBSELECT which 
**	    used to be defined over PSS_SESBLK.pss_stmt_flags
**	15-nov-93 (andre)
**	    PSS_CORR_AGGR will also be defined over pss_flattening_flags
**	22-oct-93 (rblumer)
**	    Changed pss_setparmlist to pss_procparmlist, as now it will be used
**	    for both set-input procedures AND normal procedures; moved rest of
**	    pss_setparm* parameters from here to PSS_DBPINFO structure.
**      01-nov-93 (anitap)
**          Added PSS_INGRES_PRIV over pss_ses_flag. This flag will be used by
**	    verifydb so that $ingres in verifydb can "alter table drop 
**	    constriant" on another user's table.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	17-dec-93 (rblumer)
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).  So I removed the PSS_FIPS_MODE flag.
**	02-jan-94 (andre)
**	    defined PSS_JOURNALED_DB over pss_ses_flag
**      14-jan-94 (andre)
**          updated a comment describing meaning of PSS_MULT_CORR_ATTRS
**	15-mar-94 (andre)
**	    removed definition of PSS_DSQL_QUERY as we no longer map 
**	    42000-series SQLSTATEs into 37000-series SQLSTATEs when processing 
**	    DSQL.
**	17-mar-94 (robf)
**          add PSS_SELECT_ALL.
**  4-mar-1996(angusm)
**          add PSS_HAS_DGTT.
**	11-june-1996 (angusm)
**	    add PSS_CREATE_DGTT
**	27-june-1997(angusm)
**	    add  PSS_XTABLE_UPDATE
**	27-mar-98 (inkdo01)
**	    Added pss_curcons.
**      05-Nov-1998 (hanal04)
**          Added new element proto which is used to update a statement
**          specific structure with the value of pss_viewrng.pss_maxrng
**          b90168.
**	16-Jun-1999 (johco01)
**	   Cross integrate change 439089 (gygro01)
**	   Took out previous change 438631 because it introduced bug 94175.
**	   Change 437081 (inkdo01) solves bugs 90168 and 94175.
**	17-jul-2000 (hayke02)
**	   Added PSS_INNER_OJ.
**	10-Jan-2001 (jenjo02)
**	    Added pss_qsf_rcb.
**	27-jan-2004 (schka24)
**	    Define parsing-partition-definition flag so that recursive
**	    WITH productions know what to do.  Define partition def
**	    memory stream.
**	3-Feb-2005 (schka24)
**	    Rename pss_num_literals to pss_parser_compat;  kill duplicate
**	    PSS_xxx flag defs for same, we'll use the PSQ_xxx flag defns.
**	14-sep-05 (inkdo01)
**	    Added PSS_OUTER_OJ to ease recognition of OJs in Star.
**	3-feb-03 (dougi)
**	    Added pss_hint_mask to hold hints waiting for PST_QTREE.
**	17-mar-06 (inkdo01)
**	    Added pss_tabhint, pss_hintcount for table hints (join/index).
**	29-may-06 (dougi)
**	    Added pst_stmtno for procedure statement number tracking.
**	15-june-06 (dougi)
**	    Added "before" versions of all the rule list anchors.
**	19-june-06 (dougi)
**	    Added PSS_DATA_CHANGE flag for BEFORE trigger validation.
**	18-july-06 (dougi)
**	    Added PSS_DERTAB_INQ to identify queries with derived tables.
**	30-aug-06 (thaju02)
**	    Add PSS_RULE_DEL_PREFETCH. (B116355)
**	31-aug-06 (toumi01)
**	    Add PSS_GTT_SYNTAX_SHORTCUT for optional "session." syntax.
**	22-sep-06 (toumi01)
**	    Add PSS_GTT_SYNTAX_NOSHORTCUT for "session." syntax so that
**	    we can detect invalid mixing of syntax flavors.
**	29-dec-2006 (dougi)
**	    Add PSS_WITHELEM_INQ to indicate presence of with clause 
**	    elements in range table.
**	17-jan-2007 (dougi)
**	    Added PSS_SCROLL to identify scrollable cursors.
**	12-feb-2007 (dougi)
**	    ... and PSS_KEYSET.
**      03-jun-2005 (gnagn01)
**          Added PSS_CASE_AGHEAD flag to add some flattening decisions.
**          Bug 113574,INGSRV3073.
**	16-may-2007 (dougi)
**	    Added pss_crt_tbl_stmt to help with AUTOSTRUCT options.
**	3-march-2008 (dougi)
**	    Added PSS_[NO_]CACHEDYN session flags to override cached dynamic.
**	27-march-2008 (dougi)
**	    Added PSS_ROW_PROC to pss_dbp_flags.
**	10-oct-2008 (dougi) Bug 120998
**	    Added PSS_CACHEDYN_NORETRY flag to pss_retry.
**	25-Jun-2008 (kibro01)
**	    Add PSS_SESSION_STARTED for logging purposes to state that the
**	    creation of this session has been logged.
**	03-Dec-2009 (kiria01) SIR 121883
**	    Added .pss_stk_freelist to session block to aid stack memory reuse. 
**	04-may-2010 (miket) SIR 122403
**	    Add 32 more session-level flags: pss_stmt_flags2.
**	    Add flags PSS_2_ENCRYPTION and PSS_2_PASSPHRASE.
**	29-apr-2010 (stephenb)
**	    Add batch_copy_optim and associated defines.
**	21-Jul-2010 (kschendel) SIR 124104
**	    Add default compression.
**	30-Jul-2010 (kschendel) b124164
**	    Drop unused pss-yastr found while checking for uninited stuff.
**	2-Aug-2010 (kschendel) b124170
**	    Add a pointer to the parser state (PSS_YYVARS) to the session
**	    block.  This is long overdue;  we've passed around the yyvars
**	    pointer all this time, at least partly because yacc always
**	    generated the yyvars as a local and never hooked things up.
**	    The parser has been using its own yacc for decades now, and
**	    that yacc can bloody well do what it's told.
**	    Access to the parser state is necessary in some low level
**	    contexts, such as "make constant similar".
*/
typedef struct _PSS_SESBLK
{
    struct _PSS_SESBLK *pss_next;	/* forward pointer */
    struct _PSS_SESBLK *pss_prev;	/* backward pointer */
    SIZE_TYPE	    pss_length;		/* length of this structure */
    i2		    pss_type;		/* type code for this structure */
#define                 PSS_SBID      0   /* 0 until real code assigned */
    i2		    pss_s_reserved;	/* possibly used by memory management */
    PTR		    pss_l_reserved;
    PTR		    pss_owner;		/* owner for internal memory mgmt. */
    i4		    pss_ascii_id;	/* so as to see in a dump */
#define                 PSSSES_ID       CV_C_CONST_MACRO('P', 'S', 'S', 'B')
    CS_SID	    pss_sessid;         /* Session id */
    i4	    pss_psessid;	/* parser session id. used for cursor
					** ids.
					*/
    PSC_CURBLK      pss_firstcursor;    /* Avoid alloc. for 1st cursor/sess. */
    i4		    pss_numcursors;	/* Number of allocated cursors */
    DB_LANG         pss_lang;           /* Query language id */
    i4		    pss_decimal;        /* Character to use as decimal point */
    DB_DISTRIB      pss_distrib;        /* TRUE iff distributed */
    u_char          *pss_qbuf;          /* Query buffer */
    u_char          *pss_nxtchar;       /* Next character to scan in qbuf */
    u_char          *pss_prvtok;        /* Pointer to start of prev. token */
    u_char          *pss_prvgoval;      /* Pointer to start of PSL_GOVAL tok */
    u_char	    *pss_bgnstmt;	/* Ptr to start of last stmt parsed */
    u_char          *pss_endbuf;        /* Pointer to last char in query buf */
    DB_DATA_VALUE   **pss_qrydata;	/* array of data values in query */
    i4		    pss_dval;		/* current data value number */
    i4		    pss_dmax;		/* max number of data values */
    i4		    pss_lineno;		/* Current line no. in query buffer */
    i4		    pss_stmtno;		/* Current statement number in query
					** buffer - chosen to emphasize this is
					** NOT based on line feeds. */
    i4		    pss_qualdepth;	/* Current depth of where clauses */
    PTR		    pss_symstr;		/* Symbol table memory stream */
    PSS_SYMBLK	    *pss_symtab;	/* Beginning of symbol table */
    u_char	    *pss_symnext;	/* Next spot in symbol table */
    PSS_SYMBLK	    *pss_symblk;	/* Current symbol table block pointer */
    PSS_USRRANGE    pss_usrrange;       /* User range table */
    PSS_RNGTAB	    *pss_resrng;	/* Pointer to result range variable */
    PSS_RNGTAB	    *pss_dbprng;	/* Pointer to dummy range variable
					** holding info about the set-input
					** parameter for a dbproc 
					*/ 
    PST_RESTAB	    pss_restab;		/* structure holding result tab info */
    PSS_CURSTAB     pss_curstab;        /* Hash table of cursor ctrl blocks */
    struct _YACC_CB *pss_yacc;		/* Ptr to ctl blk for re-entrant yacc */
    struct PSS_YYVARS_ *pss_yyvars;	/* Ptr to current parser state */
    i4		    (*pss_parser)();	/* ptr to yacc finite state machine */

/* Trace vector for parser sessions 
 */
#define                 PSS_TBITS       128	/* max number of trace points */
#define                 PSS_TVALS       20	/* number allowing values */
#define                 PSS_TVAO        5	/* values allowed at one time */
#define			PSS_YTRACE	0	/* Flag number of yacc flag */
ULT_VECTOR_MACRO(PSS_TBITS, PSS_TVAO) pss_trace; 

    i4		    pss_defqry;		/* 0		    - regular query
					** PSQ_DEFCURS	    - repeat query
					** PSQ_DEFQRY	    - repeat query
					** PSQ_PREPARE	    - prepare query
					** PSQ_50DEFQRY	    - 5.0 repeat query
					*/
    i2		    pss_rsdmno;		/* result domain number */
    i2		    pss_create_compression;  /* Default table compression */
    PSF_MSTREAM	    pss_ostream;	/* Mem. stream to alloc output object */
    PST_QNODE	    *pss_tlist;		/* Pointer to current target list */
    PTR		    pss_dbid;		/* Database id for this session */
    PSC_CURBLK	    *pss_crsr;		/* Current cursor */
    i4		    pss_highparm;	/* Highest parameter number found */
    i4              pss_targparm;       /* Parameter count in select list */
    SIZE_TYPE	    pss_memleft;	/* Memory left for this session */
    DB_OWN_NAME	    pss_user;		/* User name of current user */
    DB_OWN_NAME	    pss_sess_owner;	/* Session identifier for temp tables */
    DB_TAB_OWN	    pss_dba;		/* User name of dba */
    DB_TAB_OWN	    pss_group;		/* Group id of current user */
    DB_TAB_OWN	    pss_aplid;		/* Application id of current user */
    PTR		    pss_cstream;	/* Stream id of cursor for this stmt. */
    PSS_USRRANGE    pss_auxrng;		/* Auxiliary range table for qrymod */
    PTR		    pss_tchain;		/* Pointer to query text chain */
    PTR		    pss_tchain2;	/* Pointer to 2nd query text chain;
					** used for CHECK constraints  */
    void	    *pss_object;	/* Pointer to output object */
    PSF_MSTREAM	    pss_tstream;	/* Memory stream for text allocation */
    PSF_MSTREAM	    pss_cbstream;	/* Memory stream for cb allocation */
    struct _ADF_CB  *pss_adfcb;		/* ADF session control block */
    i4		    pss_idxstruct;	/* Structure for new indexes */
    i4		    pss_udbid;		/* Unique database id */
    PST_PROTO	    *pss_proto;		/* ptr to list of prototype objects */
    PTR		    pss_pstream;	/* memory stream for prototypes */
    i4		    pss_crsid;		/* Cursor id within session */
    DB_OWN_NAME	    pss_rusr;		/* User name of real user (in case of DB
					** procedures pss_user may be temporarily
					** changed; this field holds the original
					** value. */
    i4	    pss_ses_flag;	/* contains bitflags that remain through
					** the session rather than the query
					** specific bitflags contained in
					** pss_stmt_flags.  This is initialized
					** at session startup
					*/

					/*
					** set ==> user has catalog update
					** privilege
					*/
#define     PSS_CATUPD			0x0001L

					/* set ==> warnings may be issued */
#define     PSS_WARNINGS		0x0002L

					/*
					** set ==> project should occur before
					** agg
					*/
#define     PSS_PROJECT			0x0004L

					/*
					** set ==> tables will be created with
					** journaling
					*/
#define     PSS_JOURNALING		0x0008L

					/*
					** set ==> DBA may DROP everyone's
					** tables; otherwise is FALSE
					*/
#define     PSS_DBA_DROP_ALL		0x0010L
					
					/*
					** set implies don't check WHERE clause
					** flattened subselect cardinality
					*/
#define	    PSS_NOCHK_SINGLETON_CARD	0x0020L

					/*
					** set ==> scanner will strip NLs inside
					** string constants
					*/
#define	    PSS_STRIP_NL_IN_STRCONST	0x0040L

					/*
					** set ==> allow UPDATE/INSERT/DELETE on
					** indexes that are marked as catalogs
					** (other than extended catalogs)
					*/
#define	    PSS_REPAIR_SYSCAT		0x0080L

					/*
					** set if we are running UPGRADEDB - for
					** the time being it means that we will
					** allow destruction of IIDEVICES if one
					** is running UPGRADEDB with catalog
					** update privilege
					*/
#define	    PSS_RUNNING_UPGRADEDB	0x0100L

					/* 
					** If this flag is set, $ingres in 
					** verifydb will be able to alter tables
					** to drop constraint on tables owned
					** by other users.
					*/
#define	    PSS_INGRES_PRIV             0x0200L

					/*
					** this session was started with a 
					** database which is actively journaled
					*/
#define	    PSS_JOURNALED_DB		0x0400L

					/*
					** session level enabling of
					** cached dynamic selects
					*/
#define	    PSS_CACHEDYN		0x0800L

					/*
					** session level suppression of
					** cached dynamic selects
					*/
#define	    PSS_NO_CACHEDYN		0x1000L

					/* 
					** Set if session adds a row audit key
					** by default
					*/
				        /*
                                        ** Set if session enables row auditing
                                        ** by default
                                        */
#define     PSS_ROW_SEC_AUDIT     	0x2000L
#define     PSS_ROW_SEC_KEY             0x4000L
					/* 
					** User Passwords not allowed
					*/
#define	    PSS_PASSWORD_NONE		0x10000L
					/*
					** Roles not allowed
					*/
#define	    PSS_ROLE_NONE		0x20000L
					/*
					** Roles need passwords
					*/
#define	    PSS_ROLE_NEED_PW		0x40000L
					/* 
					** Row security labels are visible
					*/
#define	    PSS_DERTAB_INQ		0x80000L
					/*
					** query has at least one derived table
					*/
#define	    PSS_SELECT_ALL		0x100000L
                                        /*
                                        ** Prefetch rows for rule triggered
                                        ** by update.
                                        */
#define     PSS_RULE_UPD_PREFETCH       0x200000L

#define	    PSS_PARTDEF_STREAM_OPEN	0x00400000L	/* partdef_stream
					** has been opened, needs closed
					*/
					/*
					** prefetch rows for rule
					** triggered by delete.
					*/

#define	    PSS_RULE_DEL_PREFETCH	0x00800000L
					/*
					** "session." is optional
					*/
#define	    PSS_GTT_SYNTAX_SHORTCUT	0x01000000L
					/*
					** "session." is NOT optional
					*/
#define	    PSS_GTT_SYNTAX_NOSHORTCUT	0x02000000L
					/*
					** query has at least one with clause
					** element.
					*/
#define	    PSS_WITHELEM_INQ		0x04000000L
					/*
					** query has at least one table
					** procedure invocation
					*/
#define	    PSS_TPROC_INQ		0x08000000L

#define	    PSS_SESSION_STARTED		0x10000000L
					/* For tracing only, the START of this
					** session has now been logged
					*/

    i4	pss_stmt_flags;		/*
					** this field will be used for flags
					** which need to persist throughout
					** parsing of one statement.  It must be
					** set to 0L before calling the grammar
					** and after every DML statement parsed
					** as a part of a database procedure.
					**
					** Exceptions:
					** - PSS_TXTEMIT
					**   must be retained while parsing a
					**   brand new database procedure
					** - PSS_DISREGARD_GROUP_ROLE_PERMS
					**   must be retained when parsing a new
					**   database procedure or recreating
					**   an existing one
					*/

					/* set ==> found aggr in the tree */
#define     PSS_AGINTREE		0x0002L

					/* set ==> found subsel in the tree */
#define     PSS_SUBINTREE		0x0004L

					/* set ==> collect query text */
#define     PSS_TXTEMIT			0x0008L

					/*
					** set if permits with non-NULL
					** qualifications were used to enable
					** the user to execute this query.  This
					** flag is defined as a part of changes
					** to support QUEL shareable repeat
					** queries.
					*/
#define	    PSS_QUAL_IN_PERMS		0x0010L

					/*
					** set if we are saving the text of a
					** QUEL repeat query to determine if it
					** is shareable
					*/
#define	    PSS_QUEL_RPTQRY_TEXT	0x0020L

					/*
					** set if the protection algorithm is
					** supposed to disregard permits granted
					** to the current role and group
					** identifiers (will be set when
					** parsing DEFINE/CREATE VIEW and
					** CREATE PROCEDURE)
					*/
#define	    PSS_DISREGARD_GROUP_ROLE_PERMS  0x0040L

					/*
					** set if we are parsing SET LOCKMODE
					** SESSION.  For the distributed thread,
					** TIMEOUT is the only option allowed
					** with SESSION.  It requires a special
					** call to QEF because it is handled
					** at the Star level (RQF) rather than
					** at the LDB level.
					*/ 
#define	    PSS_SET_LOCKMODE_SESS	0x0080L

					/*
					** set if distrib register statement
					** specifies "AS NATIVE" clause.  Note
					** that this clause is not documented.
					** One distinction between registering
					** "AS LINK" and "AS NATIVE" is that
					** there can be no column name mapping
					** in a NATIVE registration.
					*/
#define	    PSS_REG_AS_NATIVE		0x0100L

					/*
					** set on COPY statement if a dummy
					** format encountered.  If not set,
					** domain name will be looked up as
					** valid column and, if distrib, will
					** be mapped to LDB column name.
					** NOTE: this flag can be used by
					** distrib AND local.  In addition,
					** local can use CPY_DUMMY_TYPE in
					** the QEU_CPDOMD struct; distrib
					** cannot use the latter flag because
					** it does not allocate a QEU_CPDOMD.
					*/

#define	    PSS_CP_DUMMY_COL		0x0200L

					/* PSS_GATEWAY_SESS indicates that
					** this is an RMS gateway session.
					** The bit is automatically set if the
					** server is a gateway server.
					*/
#define	    PSS_GATEWAY_SESS		0x0400L
					/*
					** set ==> parsing a name of a new
					** object (i.e. the object the user is
					** trying to create)
					*/
#define	    PSS_NEW_OBJ_NAME		0x0800L

					/*
					** set if parsing privileges in GRANT
					** statement
					*/
#define	    PSS_PARSING_PRIVS		0x1000L

					/*
					** set this bit when you are doing math
					** calculations on behalf of the user
					** (e.g when checking validity of
					** default values in CREATE TABLE);
					** this bit causes the PSF exception
					** handler to call ADF to handle
					** math exceptions
					*/
#define	    PSS_CALL_ADF_EXCEPTION	0x2000L

					/*
					** set if parsing a check constraint
					** in CREATE or ALTER TABLE statements
					*/
#define	    PSS_PARSING_CHECK_CONS	0x4000L

					/* set ==> collect query text
					**         and put into 2nd text chain
					*/
#define     PSS_TXTEMIT2		0x8000L

					/*
					** set if we are validating
					** specification of WITH CHECK OPTION
					** in a view definition; will become
					** more relevant when we get around to
					** allowing users to define rules on
					** views
					*/
#define	    PSS_VALIDATING_CHECK_OPTION	0x010000L

					/*
					** cursor has been declared FOR UPDATE
					** without the column list
					*/
#define	    PSS_IMPL_COL_LIST_IN_DECL_CURS  0x020000L

					/*
					** set to indicate that we are
					** revisiting the tree which was built
					** to represent <search condition> of a
					** CHECK constraint specified inside
					** CREATE TABLE in order to resolve
					** references to attributes (now that we
					** know what they are) and to perform
					** type resolution on op-nodes 
					*/
#define	    PSS_RESOLVING_CHECK_CONS	0x040000L

					/*
					** bits of information which will be 
					** used to determine whether a query 
					** should be flattened
					*/
#define     PSS_CREATE_STMT		0x080000L

					/*
					** To resolve decimal type being scanned
					** as a COMMA or a DECIMAL
					**
					*/
#define     PSS_HAS_DGTT    0x100000L
					/*
					** to flag presence of global temp table
					** in repeated query
					*/
#define		PSS_CREATE_DGTT		0x200000L
					/*
					** to flag creation of global temp table
					** to allow correct parsing of decimal columns
					** if II_DECIMAL=","
					*/
#define	 PSS_XTABLE_UPDATE		0x400000L
					/*
					** flag up xtable update
					*/
#define  PSS_PARSING_INSERT_SUBSEL	0x800000L
					/*
					** keep track of insert+subsel stmt
					*/
#define	PSS_INNER_OJ			0x01000000L
					/*
					** query contains an inner outer join
					** which has been converted to a
					** normal inner join by the fix for SIR
					** 94906
					*/
# define	PSS_DEFAULT_VALUE	0x02000000L
					/*
					** to indicate we are definining a
					** DEFAULT value. This needs special
					** consideration because of the
					** ambiguity surrounding II_DECIMAL.
					*/
# define	PSS_DBPROC_LHSVAR	0x04000000L
					/*
					** To resolve decimal type being scanned
					** as a COMMA or a DECIMAL
					** This needs special
					** consideration because of the
					** ambiguity surrounding II_DECIMAL.
					*/
#define PSS_PARSING_TARGET_LIST         0x08000000L
					/* pst_node needs to know if it is
					** parsing a select list to handle
					** outer join parameters correctly
					*/
#define		PSS_PARTITION_DEF	0x10000000L
					/* Set when parsing a PARTITION=
					** WITH clause option.  Partition
					** definitions have WITH clauses
					** themselves, and we need to keep
					** it all straight!  The psq_mode is
					** unchanged, so that error messages
					** can reference the statement name.
					*/
#define		PSS_OUTER_OJ		0x20000000L
					/* An outer join has been coded.
					** This is used to detect OJ syntax
					** in Star queries
					*/
#define		PSS_SCROLL		0x40000000L
					/* This is a scrollable cursor
					*/
#define		PSS_KEYSET		0x80000000L
					/* This is a keyset scrollable cursor
					*/
    i4	pss_stmt_flags2;		/* yet more statement-level flags */
#define		PSS_2_ENCRYPTION	0x0001L
					/* table-level ENCRYPTION= parsed */
#define		PSS_2_PASSPHRASE	0x0002L
					/* table-level PASSPHRASE= parsed */
i4		    pss_flattening_flags;

					/*
					** set if a query tree involved a
					** singleton subselect
					** 
					** if PSF_NOFLATTEN_SINGLETON_SUBSEL bit
					** is set in Psf_srvblk->psf_flags (e.g.
					** if we are running in FIPS-compliant
					** mode) and the query (SELECT or a 
					** subselect inside DELETE, INSERT, 
					** UPDATE, CREATE TABLE AS SELECT or 
					** DGTT AS SELECT) contains a singleton 
					** subselect, that query should not be 
					** flattened
					** 
					** if this bit is set when processing 
					** CREATE VIEW, we will set
					** PST_SINGLETON_SUBSEL in pst_mask1 of
					** the query tree header
					*/
#define	    PSS_SINGLETON_SUBSELECT	0x01


					/*
					** query contains a SUBSEL node which 
					** is a descendant of OR.  If this is a
					** SELECT, DELETE, UPDATE, or INSERT, 
					** we will tell OPF to not flatten this
					** query
					*/
#define	    PSS_SUBSEL_IN_OR_TREE	0x02

					/* 
					** query contains ALL.  If this is a 
					** SELECT, DELETE, UPDATE, or INSERT, 
					** we will tell OPF to not flatten this
					** query
					*/
#define     PSS_ALL_IN_TREE		0x04

					/*
					** Let
					**   A <==> SUBSEL node is a child of 
					**          NOT EXISTS, NOT IN, or
					**          != ALL
					**   B <==> SUBSEL node represents a 
					**	    nested subselect with target
					**	    list containing of count() 
					**	    or count(*)
					** This mask will be set if:
					** (A || B) and the tree representing 
					** the SUBSEL node contains corelated 
					** references to at least 2 different 
					** tables.
					**
					** NOTE that when processing a NOT IN or
					** 	!= ALL, instead of simply 
					**	counting variables involved in
					**	correlated references in the
					**	subselect, we will count the
					** 	number of distinct variables
					**	referenced in the left subtree
					**	of NOT IN/!=ALL and those 
					**	involved in correlated 
					**	references in the subselect
					*/
#define     PSS_MULT_CORR_ATTRS		0x08

					/*
					** this mask will be set if a subselect
					** has one or more aggregates in the 
					** target list and involves at least one
					** correlated reference (sometimes this
					** is referred to as "correlated
					** aggregates")
					*/
#define	    PSS_CORR_AGGR	    	0x10

					/*
					** for query flattening we care if
					** we have one or more than one of:
					** - aggregate
					** - ifnull with aggregate operand
					** these four flags are effectively
					** two two-bit counters to indicate
					** that in parsing we have found
					** none, one, or more than one of
					** the above
					*/

#define	    PSS_AGHEAD			0x0100
#define	    PSS_AGHEAD_MULTI		0x0200
#define	    PSS_IFNULL_AGHEAD		0x0400
#define	    PSS_IFNULL_AGHEAD_MULTI	0x0800
                                         
                                        /* set to indicate the existence of case
                                        ** operator and agregate in a 
                                        ** select.such queries should not be 
                                        ** flattened.
                                        */  
#define     PSS_CASE_AGHEAD             0x10000

                                        /*
                                        ** Set to indicate we need to add
                                        ** a ROW SECURITY LABEL when doing
                                        ** CREATE TABLE AS SELECT
                                        */
#define     PSS_NEED_ROW_SEC_KEY        0x0800000L

    i4	    pss_dbp_flags;	/*
                                        ** this field will be used for flags
					** which need to persist across the
					** various statements comprising a
					** database procedure.  This flag must
					** be set to 0L before calling
					** psq_parseqry from psq_call()and it
					** must be set appropriately before
					** calling psq_recreate() from
					** psq_call(). In all likelihood, it
					** shall not be reinitialized (other
					** than by turning ON or OFF certain
					** bits) while parsing a database
					** procedure.
                                        */

					/* set ==> recreating a dbproc */
#define	    PSS_RECREATE		0x0001L

					/* set ==> parsing an internal dbproc */
#define	    PSS_IPROC			0x0002L

					/* set ==> parsing a dbproc */
#define	    PSS_DBPROC			0x0004L	

					/*
					** set ==> as far as we know, OK to
					** grant permit on the dbproc;
					** "as far as we know" covers cases when
					** a dbproc invokes other dbprocs owned
					** by its owner and one or more of
					** invoked dbprocs is not marked as
					** grantable
					*/
#define	    PSS_DBPGRANT_OK		0x0008L	

					/*
					** set ==> we are parsing dbproc to
					** determine if it is grantable
					*/
#define	    PSS_0DBPGRANT_CHECK		0x0010L	

					/* set ==> pss_ruser holds a name */
#define     PSS_RUSET			0x0040L	

					/*
					** when creating a new procedure or
					** recreating an existing one for use by
					** its owner, we must verify that they
					** are, at least, "active".  This flag
					** will be set if we are reparsing a
					** dbproc invoked (directly or
					** indirectly) by the dbproc being
					** [re]created
					*/
#define	    PSS_CHECK_IF_ACTIVE		0x0080L

					/*
					** set if in the process of [re]parsing
					** a dbproc is has been determined that
					** the user lacks a required privilege.
					*/
#define	    PSS_MISSING_PRIV		0x0100L

					/*
					** set if in the process of [re]parsing
					** a dbproc is has been determined that
					** some object used in the dbproc does
					** not exist
					*/
#define     PSS_MISSING_OBJ		0x0200L
					/*
					** set when parsing a set-input 
					** parameter in CREATE PROCEDURE
					*/
#define	    PSS_PARSING_SET_PARAM	0x0400L
					/*
					** set if the current procedure has a
					** set-input parameter 
					*/
#define     PSS_SET_INPUT_PARAM		0x0800L
					/*
					** set if the procedure was generated
					** by the system (via EXEC IMMED)
					*/
#define     PSS_SYSTEM_GENERATED	0x1000L
					/*
					** set if the procedure cannot be
					** dropped by the user
					*/
#define     PSS_NOT_DROPPABLE		0x2000L
					/*
					** set if the procedure is implementing
					** a constraint; need to bypass
					** permission checking for this proc
					*/
#define     PSS_SUPPORTS_CONS		0x4000L
					/*
					** set if in midst of parsing 
					** dbproc RESULT ROW clause
					*/
#define	    PSS_PARSING_RESULT_ROW	0x8000L
					/*
					** Set if procedure needs query_syscat
					** to execute
					*/
#define	    PSS_NEED_QRY_SYSCAT		0x10000L
					/*
					** Set if proc contains INSERT, UPDATE
					** or DELETE (for BEFORE trigger
					** validation)
					*/
#define	    PSS_DATA_CHANGE		0x20000L
					/*
					** Set if procedure has at least one
					** OUT or INOUT parm
					*/
#define	    PSS_OUT_PARMS		0x40000L
					/*
					** Set if this is a row producing 
					** procedure
					*/
#define	    PSS_ROW_PROC		0x80000L
					/*
					** Set if COMMIT or ROLLBACK in proc
					*/
#define	    PSS_TX_STMT			0x100000L
					/*
					** Set if parsing a procedure for grant
					** privileges
					*/
#define	    PSS_PARSING_GRANT		0x200000L
    i4		    pss_retry;		/* Flag field to instruct on whether
					** or not to suppress user errors &
					** whether or not to retry if error is
					** encountered. */
					/* suppress reporting of error messages
                                        ** to the user.  (Still go ahead an log
                                        ** them.)  The suppression is usually 
                                        ** only used because PSF intends to
                                        ** retry if it gets an error */
#define     PSS_SUPPRESS_MSGS   ((i4) 0x01)
					/* print any error msgs PSF encounters
                                        ** to user via SCC_ERROR */
#define     PSS_REPORT_MSGS     ((i4) 0x02)
					/* continue suppressing msgs to user,
                                        ** however, indicate that the caller
                                        ** should retry this operation. */
#define     PSS_ERR_ENCOUNTERED	((i4) 0x04)
					/* indicates that statement is in redo
					** mode and that RDF should be
					** instructed to get fresh copy of
					** any cached object. */
#define	    PSS_REFRESH_CACHE	((i4) 0x08)
					/* indicates this is a cached dynamic
					** query and cannot be retried */
#define	    PSS_CACHEDYN_NORETRY ((i4) 0X10)

    PST_STATEMENT   *pss_row_lvl_usr_rules;
					/*
					** will point at the list of row-level
					** user-defined rules associated with
					** the statement. System-generated
					** row-level rules will be attached to
					** the end of the list of user-generated
					** row-level rules and the result will
					** be pointed at by pst_after_stmt
					*/
    PST_STATEMENT   *pss_row_lvl_sys_rules;
					/*
					** will point at the list of row-level
					** system-generated rules associated
					** with the statement.
					*/
    PST_STATEMENT   *pss_stmt_lvl_usr_rules;
					/*
					** will point at the list of
					** statement-level user-defined rules
					** associated with the statement.
					** System-generated statement-level
					** rules will be attached to the end of
					** the list of user-generated 
					** statement-level rules and the result
					** will be pointed at by
					** pst_statementEndRules
					*/
    PST_STATEMENT   *pss_stmt_lvl_sys_rules;
					/*
					** will point at the list of
					** statement-level system-generated
					** rules associated with the statement.
					*/
    PST_STATEMENT   *pss_row_lvl_usr_before_rules;	/* "before" versions 
					** of all the above */
    PST_STATEMENT   *pss_row_lvl_sys_before_rules;
    PST_STATEMENT   *pss_stmt_lvl_usr_before_rules;
    PST_STATEMENT   *pss_stmt_lvl_sys_before_rules;

    PTR		    pss_audit;		/* This field is filled by query mod
					** when processing protections associated with
					** the current statement.  The audit
					** information pointer 
					** is moved from here into the
					** pst_audit field of the parent
					** statement when the head node is
					** built.
					*/
    i4		    pss_parser_compat;	/* Session copy of parser compatability
					** flags, set at session-begin time.
					** See psq_parser_compat in PSQ_CB
					** for the possible values.
					*/
    i4		    pss_ustat;		/* User status flags from SCS_ICS */

					/* NULL-terminated DBA name */
    char	    pss_dbaname[DB_OWN_MAXNAME+1];
    PSQ_INDEP_OBJECTS	pss_indep_objs;
    PSF_MSTREAM		*pss_dependencies_stream;
    DB_DBP_NAME		*pss_dbpgrant_name;
    QEF_DATA	    *pss_procparmlist;	/* procedure parameter list, passed to
					** RDF during CREATE PROCEDURE
					*/
    u_i4	    *pss_dbxlate;	/* Case translation semantics flags;
					** See <cuid.h> for masks.
					*/
    i4	    pss_distr_sflags;	/* statement flags for distrib - mainly
					** for interacting with LDB on DDL */
#define     PSS_DEFAULT			0x0000
#define     PSS_NODE			0x0001
#define     PSS_DB			0x0002
#define     PSS_NDE_AND_DB	(PSS_NODE | PSS_DB)
#define     PSS_DBMS			0x0004
#define     PSS_LDB_TABLE		0x0008
#define     PSS_QUOTED_NM		0x0010 /* can be removed */
#define     PSS_REFRESH			0x0020
#define     PSS_PRINT_WITH		0x0040
#define     PSS_QUOTED_TBLNAME		0x0080
#define     PSS_QUOTED_OWNNAME		0x0100
#define     PSS_LOCATION		0x0200
#define     PSS_REGISTER_PROC   	0x0400
#define     PSS_DELIM_TBLNAME		0x0800
#define     PSS_DELIM_OWNNAME		0x1000
#define	    PSS_DELIM_COLNAME		0x2000
#define	    PSS_1ST_ELEM		0x4000
    DB_OWN_NAME	    *pss_cat_owner;	/* Catalog owner (e.g. "$ingres") */

    PST_STATEMENT   *pss_cur_cons_stmt;	/*
					** PST_STATEMENT allocated for
					** the constraint being processed
					*/

    PST_STATEMENT   *pss_crt_tbl_stmt;	/*
					** PST_STATEMENT allocated for 
					** CREATE TABLE being processed
					*/

    DB_TAB_ID	    pss_dbp_ubt_id;	/*
					** if parsing definition of a dbproc,
					** this element will be used to contain
					** id of some base table (other than a 
					** core catalog) used (directly or 
					** through a view) in the dbproc
					*/
    PSS_USRRANGE   pss_viewrng;		/* Used  to store actual views we need to 
					** recover before we do substitutions 
					*/
    struct _PSS_CONS *pss_curcons;	/*
					** constraint definition currently
					** under analysis
					*/
    PTR		pss_save_qeucb;		/* parallel index */
    QSF_RCB	pss_qsf_rcb;		/* Used by psfmem.c functions */
    ULM_RCB	pss_partdef_stream;	/* Partition def memory stream */
    /* FIXME too many streams running around, how about one "utility"
    ** query-parse stream?  or use the symbol stream and close/reopen that
    ** one each query instead of holding it open?
    */
    PTR		pss_funarg_stream;	/* Function-arg stack stream */
    i4		pss_hint_mask;		/* to hold hint flags destined
					** for PST_QTREE - same as PST_HINT_xxx
					** values */
    PSS_HINTENT	pss_tabhint[PST_NUMVARS]; /* array of table hints */
    i4		pss_hintcount;		/* number of table hints */
    char	pss_last_sname[DB_MAXNAME+1]; /* last dynamic statement. This
					 ** name will be re-set at the end of each
					 ** group (batch) or when a new statement
					 ** is executed (dynamic or not). It can
					 ** only be used to determine the name of
					 ** the last dynamic statement used in
					 ** the current batch immediately 
					 ** before the current dynamic statement. Its
					 ** primary purpose is to tell the dynamic
					 ** insert optimization whether the same
					 ** dynamic statement is being re-executed */
    PST_QTREE	*pss_qtree;		/* query tree, used for dynamic insert
					** optimization.*/
    u_i1	batch_copy_optim;	/* whether to use batch copy optimization
					** determined by "set [no]batch_copy_optim"
					*/
#define		PSS_BATCH_OPTIM_UNDEF	0 /* use server default */
#define		PSS_BATCH_OPTIM_SET	1 /* use optimization (if possible ) */
#define		PSS_BATCH_OPTIM_UNSET	2 /* do not use optimization */
    struct PST_STK1 *pss_stk_freelist;
} PSS_SESBLK;	

/*
**  Server control block definition.
*/

/*
**  Version number of parser facility.
*/

#define                 PSF_VERSNO      0

/*
** Name: PSF_SERVBLK - Server control block for parser.
**
** Description:
**      This structure defines the server control block for the parser.  There
**      will be one of these for each database server.
**
** History:
**     09-oct-85 (jeff)
**          written
**      20-jul-89 (jennifer)
**          Added server capability field to server control block.
**     12-sep-90 (SandyH)
**	    Added psf_sess_mem field to handle calculated session memory.
**     22-jan-91 (andre)
**	    Added a server-wide flag field - psf_flags and defined a new mask
**	    over it - PSF_NO_ZERO_TIMEOUT.
**     08-mar-93 (andre)
**	    defined PSF_DEFAULT_DIRECT_CURSOR and PSF_NOFLATTEN_SINGLETON_SUBSEL
**     18-oct-93 (rogerk)
**	    Added PSF_NO_JNL_DEFAULT psf_flag which specifies that tables
**	    should be created with nojournaling by default.  This flag is
**	    set in the psq_startup call when the DEFAULT_TABLE_JOURNALING
**	    server startup flag specifies nojournaling.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	    Change type of psf_sem from PTR to CS_SEMAPHORE, embedding
**	    it in Psf_srvblk.
**	10-Jan-2001 (jenjo02)
**	    Added psf_psscb_offset.
**	24-aug-06 (toumi01)
**	    Add PSF_GTT_SYNTAX_SHORTCUT for optional "session." SIR.
**	31-aug-06 (toumi01)
**	    Delete PSF_GTT_SYNTAX_SHORTCUT.
**	3-march-2008 (dougi)
**	    Add PSF_CACHEDYN server wide flag.
**      15-Feb-2010 (maspa05) SIR 123293
**          Add psf_server_class so SC930 can output server_class
**	20-Jul-2010 (kschendel) SIR 124104
**	    Add default create-table compression.
*/
typedef struct _PSF_SERVBLK
{
    struct _PSF_SERVBLK *psf_next;	/* forward pointer */
    struct _PSF_SERVBLK *psf_prev;	/* backward pointer */
    SIZE_TYPE	    psf_length;		/* length of this structure */
    i2		    psf_type;		/* type code for this structure */
#define                 PSF_SRVBID      0   /* 0 until real code assigned */
    i2		    psf_s_reserved;	/* possibly used by memory management */
    PTR		    psf_l_reserved;
    PTR		    psf_owner;		/* owner for internal memory mgmt. */
    i4		    psf_ascii_id;	/* so as to see in a dump */
#define                 PSFSRV_ID       CV_C_CONST_MACRO('P', 'S', 'F', 'B')
    i4		    psf_srvinit;	/* TRUE iff server is initialized */
    i4		    psf_nmsess;		/* Count of active sessions */
    i4		    psf_mxsess;		/* Maximum number of sessions */
    i4              psf_version;        /* Parser version number */
    SIZE_TYPE	    psf_memleft;
    PTR		    psf_poolid;		/* Memory pool id */
    DB_LANG	    psf_lang_allowed;	/* Qry languages allowed */
#define                 PSF_TBITS       128
#define			PSF_TNVAO	20
#define			PSF_TPAIRS	64
 ULT_VECTOR_MACRO(PSF_TBITS, PSF_TNVAO) psf_trace; /* Trace vector for server */
    PTR		    psf_rdfid;		/* RDF control structure id */
    i4	    psf_sess_num;	/* PSF session number */
    SIZE_TYPE	    psf_sess_mem;	/* PSF session memory, if passed in.*/
    CS_SEMAPHORE    psf_sem;		/* PSF global semaphore */
    i4         psf_capabilities;   /* Server capabilities. */
#define                 PSF_C_C2SECURE  0x001
    i4	    psf_psscb_offset;		/* Offset from SCF's SCB to 
					** any session's PSS_SESBLK
					*/
    i4	    psf_flags;		/* server-wide flags */
					/*
					** PSF will disallow SET LOCKMODE ON
					** TABLE WHERE TIMEOUT=0 on SCONCUR
					** catalogs, iistatistics, and
					** iihistogram; this will be done if OPF
					** memory management can cause server
					** deadlock
					*/
#define	    PSF_NO_ZERO_TIMEOUT		    0x0001L
					/*
					** if set then updatable cursors will be
					** opened in DIRECT unless the user has
					** explicitly specified FOR DEFERRED
					** UPDATE;
					** otherwise updatable cursors will be
					** opened in DEFERRED unless the user
					** has explicitly specified FOR DIRECT
					** UPDATE; 
					*/
#define	    PSF_DEFAULT_DIRECT_CURSOR	    0x0002L
					/*
					** if set then queries involving
					** singleton subselects (be it in the
					** query itself or in a view used inside
					** the query) will not be flattened
					*/
#define	    PSF_NOFLATTEN_SINGLETON_SUBSEL  0x0004L
					/*
					** default for this server is that
					** new tables should be created with
					** nojournaling unless journaling is
					** actually requested by the user.
					*/
#define	    PSF_NO_JNL_DEFAULT		    0x0008L
					/*
					** 6.4 behaviour on ambiguous
					** replace in SQL cross-table
					** updates (i.e lax)
					*/
#define     PSF_AMBREP_64COMPAT	    	   0x0010L	
#define	    PSF_DFLT_READONLY_CRSR	   0x0020L
					/*
					** CBF or "set cache_dynamic" says 
					** cache dynamic cursor query plans
					*/
#define	    PSF_CACHEDYN		   0x0040L
					/*
					** if set then queries involving
					** singleton subselects in the where
					** clause will not raise cardinality
					** errors.
					*/
#define     PSF_NOCHK_SINGLETON_CARD	   0x0080L

    i2		    psf_create_compression;  /* DMU_C_xxx default create table
					** compression from config.
					*/
    bool	    psf_vch_prec;	/* varchar precedence */
    char           *psf_server_class;   /* server_class of server */
} PSF_SERVBLK;

/* Macro to get pointer to session's PSS_SESBLK directly from SCF's SCB */
GLOBALREF	PSF_SERVBLK	*Psf_srvblk;

#define	GET_PSS_SESBLK(sid) \
	*(PSS_SESBLK**)((char *)sid + Psf_srvblk->psf_psscb_offset)

/*
** Name: PSQ_CBHEAD - Control block header
**
** Description:
**      This structure duplicates what appears at the beginning of every
**      control block.  It would be possible to include this structure in
**      the control blocks; however, this would not conform to implementation
**      conventions.
**
** History:
**     10-jan-86 (jeff)
**          written
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _PSQ_CBHEAD
{
    struct _PSQ_CBHEAD    *psq_cbnext;  /* forward pointer */
    struct _PSQ_CBHEAD	  *psq_cbprev;	/* backward pointer */
    SIZE_TYPE		  psq_cblength;	/* length of this structure */
    i2			  psq_cbtype;	/* type code for this structure */
    i2			  psq_cb_s_reserved; /* possibly used by memory mgmt. */
    PTR			  psq_cb_l_reserved;
    PTR			  psq_cb_owner;	/* owner for memory mgmt. */
    i4			  psq_cb_ascii_id; /* so as to see in dump */
}   PSQ_CBHEAD;

/*
** Name: PSL_COL - Structure holding info about GRANT update column entry.
**
** Description:
**      This structure defines an element of a linked list created by
**	the GRANT stmnt to hold info about an update column on which grant
**	has been issued.
**
** History:
**     23-apr-87 (stec)
**          written
**	14-jun-91 (andre)
**	    get rid of psy_off2 since we can store column names directly into
**	    the IIQRYTEXT template
**	17-aug-93 (andre)
**	    added psy_col_flags and defined PSY_REGID_COLSPEC
*/
typedef struct _PSY_COL
{
    QUEUE		queue;		/* pointers	*/
    DB_ATT_NAME		psy_colnm;	/* column name	*/
    i4			psy_col_flags;	/* bits of useful information */

					/* 
					** set if column name was expressed 
					** using a regular identifier
					*/	
#define	PSY_REGID_COLSPEC	0x01	
}   PSY_COL;

/*
** Name: PSY_TBL - Structure holding info about GRANT table entry.
**
** Description:
**      This structure defines an element of a linked list created by
**	the GRANT stmnt to hold info about an object for which grant
**	has been issued.
**
** History:
**	23-apr-87 (stec)
**          written
**	28-oct-89 (andre)
**	    Added psy_mask.
**	18-dec-89 (andre)
**	    defined PSY_ALL_TO_ALL and PSY_RETR_TO_ALL
**	14-jun-91 (andre)
**	    offsets for object names (which are the same for all names) will be
**	    stored in PSY_CB.
**	27-jun-91 (andre)
**	    added psy_owner,
**	    undefined PSY_ALL_TO_ALL and PSY_RETR_TO_ALL,
**	    defined PSY_OBJ_IS_TABLE, PSY_OBJ_IS_INDEX, and PSY_OBJ_IS_VIEW
**	10-oct-91 (andre)
**	    added PSY_OBJ_IS_DBPROC and PSY_DBPROC_IS_GRANTABLE
**	17-aug-93 (andre)
**	    defined PSY_REGID_SCHEMA_NAME and PSY_REGID_OBJ_NAME
*/
typedef struct _PSY_TBL
{
    QUEUE		queue;		/* pointers	*/
    DB_TAB_ID		psy_tabid;	/* table id	*/
    DB_TAB_NAME		psy_tabnm;	/* table name	*/
    i4			psy_mask;
#define	    PSY_OBJ_IS_TABLE	((i4) 0x01)	/* object is of type table */
#define	    PSY_OBJ_IS_VIEW	((i4) 0x02)	/* object is of type view  */
#define	    PSY_OBJ_IS_INDEX	((i4) 0x04)	/* object is of type index */
#define     PSY_OBJ_IS_DBPROC   ((i4) 0x08)    /* object is of type dbproc */
						/*
						** set if dbproc is known to be
						** grantable
						*/
#define     PSY_DBPROC_IS_GRANTABLE ((i4) 0x10)
						/*
						** set if schema name was 
						** expressed using a regular 
						** identifier in GRANT/REVOKE 
						** statement
						*/
#define	    PSY_REGID_SCHEMA_NAME   ((i4) 0x20)
						/*
						** set if object name was 
						** expressed using a regular
						** identifier in GRANT/REVOKE
						** statement
						*/
#define     PSY_REGID_OBJ_NAME	     ((i4) 0x40)
    DB_OWN_NAME		psy_owner;	/* 
					** object owner (in 6.5 world this is 
					** the same as "schema name") 
					*/

    /* NOTE: this must be the LAST item in the structure!!! */
    i4		psy_colatno[1];	/* for update columns		    */
					/* this array will actually have    */
					/* as many elements as there are    */
					/* update columns		    */
}   PSY_TBL;

/*
** Name: PSY_USR - Structure holding info about GRANT grantee.
**
** Description:
**      This structure defines an element of a linked list created by
**	the GRANT stmnt to hold info about a user named in a GRANT command
**
** History:
**     23-apr-87 (stec)
**          written
**	14-jun-91 (andre)
**	    we do need to save the offsets for grantee names since it can be
**	    deduced based on the offset for object name and the grantee type.
**      17-aug-93 (andre)
**          added psy_usr_flags and defined PSY_REGID_USRSPEC   
*/
typedef struct _PSY_USR
{
    QUEUE		queue;		/* pointers	*/
    DB_OWN_NAME		psy_usrnm;	/* user name	*/
    i4			psy_usr_flags;	/* bits of useful information */

					/* 
					** set if user name was expressed 
					** using a regular identifier
					*/	
#define	PSY_REGID_USRSPEC	0x01	
}   PSY_USR;

/*
** Name: PSY_OBJ - Structure holding info about general object.
**
** Description:
**      This structure defines an element of a linked list used to
**	hold general object information.
**
** History:
**     7-dec-93 (robf)
**          written for security alarm enhancements.
*/
typedef struct _PSY_OBJ
{
    QUEUE		queue;		/* pointers	*/
    DB_NAME		psy_objnm;	/* object name	*/
    i4			psy_obj_flags;	/* bits of useful information */
}   PSY_OBJ;

/*
** These values are used to introduce values from host-language variables to
** the scanner.  For example, PSQ_HVI2 would appear directly before an i2
** sent from a front-end to the server in the middle of a query.  These numbers
** are non-contiguous for historical reasons.  To change them would mean to
** lose communications compatibility with existing front-ends.
*/

#define                 PSQ_HVI2        '\1'	/* Means an i2 follows */
#define			PSQ_HVSTR	'\3'	/* Means a string follows */
#define			PSQ_HVF8	'\4'	/* Means an f8 follows */
#define			PSQ_HVI4	'\6'	/* Means an i4 follows */

#define			PSQ_HV		'~'	/* means a data value follows */
#define			PSQ_HVD		'V'	/* single data value  */
#define			PSQ_HVQ		'Q'     /* cursor id (3 db_data_values)
						*/
/*
**  Moved from yyvars.h by Alex Stec on 21-may-87.
*/
typedef struct _yyagg_node_ptr
{
    struct _yyagg_node_ptr	    *agg_next;
    PST_QNODE			    *agg_node;
} YYAGG_NODE_PTR;

/*
** Name: PST_VRMAP - Bit map of variables used.
**
** Description:
**      This type definition is used for bitmaps representing
**	range table variables used. Its number of bits must be
**	greater than or equal to the number of variables in the
**	range table.
**
** History:
**     30-dec-87 (stec)
**          Written.
**	27-nov-02 (inkdo01)
**	    Range table expansion (change i4 to PST_J_MASK).
*/
typedef struct _PST_VRMAP
{
    PST_J_MASK    pst_vrmap;	    /* bitmap of range table variables. */
} PST_VRMAP;

/* Maximum length of message to be sent to FE. */
#define PSF_MAX_TEXT	80

/*
** Name: PSS_EXLIST - select expression list element.
**
** 
** Description:
**      This type definition defines an element of an expression list.
**	Each element holds a pointer to an expression tree.
**      Nodes of this type will be used when processing
**	"... where select_expr [not] in (sel_expr_list)"
**
** History:
**     13-jul-88 (andre)
**          Written.
*/
typedef struct _PSS_EXLIST
{
    struct _PSS_EXLIST	    *pss_next;
    PST_QNODE	            *pss_expr;	/* Ptr to select expression
					** tree. The select expression
					** is an element of sel_expr_list.
					*/
} PSS_EXLIST;

/*
** Name: PSS_FIXSTMT - Structure ptr to statement to be fixed.
**
** Description:
**      This structure defines an element of a linked list that
**	holds info about statement pointer which has to be fixed.
**	Depending on a statement, the pointer to be fixed could be pst_next,
**	pst_true, or pst_false.
**	(this comment was added as a part of a fix for bug #35659)
**
** History:
**	30-may-88 (stec)
**          written
**	11-feb-91 (andre)
**	    add a flag field + define a mask over it;
**	    this change was made as a part of fix for bug 35659
*/
typedef struct _PSS_FIXSTMT
{
    QUEUE		pss_queue;		/* pointers	*/
    PST_STATEMENT	**pss_stmt_ptr;		/* address of the location in
						** PST_STATEMENT to be fixed */
    i4			pss_flags;
						/*
						** if this flag is set,
						** *pss_stmt_ptr must be fixed
						** before beginning to process
						** the body of the loop 
						** statement
						*/
#define	PSS_FIX_EARLY		((i4) 0x01)
}   PSS_FIXSTMT;

/*
** Name: PSS_LOOP - Structure holding label info for a WHILE/FOR/REPEAT stmt.
**
** Description:
**      This structure defines an element of a linked list created by
**	a database procedure loop stmnt to hold the label value and
**	related info.
**
** History:
**     30-may-88 (stec)
**          written
**	17-aug-98 (inkdo01)
**	    Changed WHILE names to "loop" to reflect more general loop
**	    constructs of WHILE/FOR/REPEAT.
*/
typedef struct _PSS_LOOP
{
    QUEUE		pss_queue;		/* pointers	*/
    char		pss_label[DB_MAXNAME];  /* loop label or spaces */
    PST_STATEMENT	*pss_stmt;		/* ptr to related IF stmt node */
    QUEUE		pss_fixq;		/* queue of stmts to fix */
    i4			pss_out;		/* Out of scope flag */
}   PSS_LOOP;

/*}
** Name: PSS_DECVAR - Procedure parameter or local variable definition
**
** Description:
**	This holds the definition of procedure parameter or local variable
**	definitions as the parser internally holds it.
**
**	
** History:
**	18-may-88 (stec)
**	    Created.
**	30-may-89 (neil)
**	    Commented/updated use of PSS_DBPPARMMAX.
**	7-june-06 (dougi)
**	    Add flag field to support IN, OUT, INOUT parms.
*/
struct _PSS_DECVAR
{
    QUEUE		pss_queue;		    /* pointers	*/
    struct _PSS_DECVAR	*pss_next;		    /* next var of same type */
    DB_DATA_VALUE	pss_dbdata;
    i4			pss_default;		    /* Defaultibility indicator. */
    i4			pss_no;	
    /*
    ** The max number of parameters is 2*DB_MAXCOLS to allow old/new
    ** parameters for rules + (internally) the built in variables.
    ** The max number of result row "columns" is plain ol' DB_MAXCOLS.
    */
#define	    PSS_DBPUSRMAX	2*DB_MAX_COLS
#define	    PSS_DBPPARMMAX	PSS_DBPUSRMAX + PST_DBPBUILTIN
#define	    PSS_DBPROWMAX	DB_MAX_COLS
    DB_PARM_NAME	pss_varname;		    /* Name of local var, parm */
    i4			pss_scope;		    /* Scope level no. */
    i4			pss_out;		    /* Out of scope flag */
    i4			pss_flags;		    /* flags for parm type */
#define	PSS_PPIN	0x01			/* IN parm */
#define	PSS_PPOUT	0x02			/* OUT parm */
#define	PSS_PPINOUT	0x04			/* INOUT parm */
};

/*
** Name: PSS_IFSTMT - Element of the IF queue holding address of an IF stmt.
**
** Description:
**      This structure defines an element of a linked list that
**	holds info about an IF statement. This queue reflects nesting
**	of IF stmts (i.e., grows and shrinks).
**
** History:
**     30-may-88 (stec)
**          written
*/
typedef struct _PSS_IFSTMT
{
    QUEUE		pss_queue;		/* pointers	*/
    PST_STATEMENT	*pss_stmt;		/* address of the IF stmt */
}   PSS_IFSTMT;

/*
** Name: PSS_DBPINFO - Structure holding database procedure info.
**
** Description:
**      This structure holds general info related to an existing
**	database procedure.
**
** History:
**     18-may-88 (stec)
**          written
**      27-july-88 (andre)
**	    Modified PSS_DBPINFO for DB proc vsn of GRANT/REVOKE.
**	03-aug-88 (stec)
**	    Added pss_into_clause, pss_1sub_seen, pss_lvar_seen.
**	11-feb-91 (andre)
**	    defined a new flag field; replaced several boolean fields with masks
**	    over the flag field; added a new field which will be used to detect
**	    unreachable statements and statement blocks
**	    (these changes were made as a part of fix for bug 35659)
**	20-mar-91 (andre)
**	    added pss_last_stmt_stmt which will ALWAYS point at the last
**	    allocated statement block, regardless of whether or not it is
**	    reachable
**	22-oct-93 (rblumer)
**	    moved pss_setparm* parameters from PSS_SESBLK to here.
**	    Added pss_procparmno to be used for all procedures.
**	17-aug-98 (inkdo01)
**	    just changed inwhile/whileq to inloop/loopq for broadened meaning.
**	28-Oct-2008 (kiria01) SIR121012
**	    For manipulating the pss_list it is necessary to have a list
**	    head: .pss_head_stmt is added for this purpose.
*/
typedef struct _PSS_DBPINFO
{
    QUEUE	    pss_varq;		/* parameter, local var queue. */
    PSS_DECVAR	    *pss_curvar;	/* ptr to current parm, lvar */
    i4		    pss_varno;		/* Variable, parameter no. */
    i4		    pss_bescope;	/* BEGIN - END scope level */
    i4		    pss_inloop;		/* Level of WHILE/FOR/REPEAT loop
					** nesting */
    QUEUE	    pss_loopq;		/* WHILE/FOR/REPEAT label queue. */
    PST_STATEMENT   *pss_link;		/* last allocated REACHABLE stmt
					**		!!!!
					** VERY IMPORTANT: pss_link will point
					** at the last statement block which was
					** allocated WHILE
					** pss_unreachable_stmt_lvl == 0; once
					** pss_unreachable_stmt_lvl is
					** incremented, we will stop adding new
					** statements to the list until
					** pss_unreachable_stmt_lvl returns to
					** 0; therefore, one must not expect
					** pss_link to point at the last
					** allocated statement unless
					** pss_unreachable_stmt_lvl == 0.
					**		!!!!
					** (HELPFUL NOTE:) pss_last_stmt, on the
					** other hand, will ALWAYS point at the
					** last allocated statement block
					*/
    PST_STATEMENT   **pss_patch;	/* Address of location to be
					** patched in case ENDLOOP is
					** encountered.
					*/
    QUEUE	    pss_ifq;		/* if stmt queue */
    PST_QNODE	    *pss_into_clause;	/* ptr to tree holding target list
					** associated with the into clause
					*/
					/*
					** initially this will be set to 0;  if
					** ENDLOOP or RETURN is hit and
					** pss_unreachable_stmt_lvl == 0, it
					** will be reset to 1; for every
					** statement block entered while
					** pss_unreachable_stmt_lvl != 0, it
					** will be incremented by 1 and foir
					** each statement block completed while
					** pss_unreachable_stmt_lvl != 0, it
					** will be decremented by 1;  all
					** statements parsed while
					** pss_unreachable_stmt_lvl != 0, are
					** unreachable, so they will not be
					** linked to any of the statement lists
					*/
    i4		    pss_unreachable_stmt_lvl;

    i4		    pss_flags;		/* flag field */
					/* set ==> processing DBP condition */
#define	PSS_INCOND		((i4) 0x01)
					/*
					** set ==> first qry term has been seen
					*/
#define	PSS_1SUB_SEEN		((i4) 0x02)
					/*
					** set ==> local variable has been
					** referenced in the target list of this
					** query
					*/
#define	PSS_LVAR_SEEN		((i4) 0x04)
					/* set ==> processing DBP FOR loop */
#define PSS_INFOR		((i4) 0x08)
					/* set ==> processing select statement
					** of DBP FOR loop */
#define PSS_INFORQ		((i4) 0x10)

    DB_PROCEDURE    pss_ptuple;		/* IIPROCEDURE row for this procedure */
    PST_STATEMENT   *pss_head_stmt;	/* This will point to the first allocated
					** statement block; This is used so that
					** should we need to, we can identify 
					** a blocks predecessor. */
    PST_STATEMENT   *pss_last_stmt;	/* this will point at the last allocated
					** statement block; in most cases, its
					** value will be the same as pss_link,
					** however, if the last allocated
					** statement block was for an
					** unreachable statement, pss_link will
					** remain pointing at the last reachable
					** statement, whereas pss_last_stmt will
					** point at the new block
					*/
    PSS_DECVAR	    *pss_setparmname;	/* holds name of set parameter
					** (for 6.5, only one set is allowed)
					*/
    i4		    pss_setparmno;	/* number of columns in set parameter*/
    QUEUE	    pss_setparmq;	/* queue of set parameter columns */
    i4		    pss_procparmno;	/* number of parameters in procedure,
					** valid for both types of procedures,
					** but is only set after all parameters
					** have been parsed
					*/
    QUEUE	    pss_resrowq;	/* queue of result row "column"
					** descriptors */
    i4		    pss_resrowno;	/* number of "columns" in result row */
    PSS_DECVAR	    *pss_savevar;	/* save currvar across result row 
					** parse */
}   PSS_DBPINFO;

/*
** Name: PSC_RULE_TYPE	- Rule descriptor attached to an open cursor.
** 
** Description:
**	When a cursor is opened no rules are attached.  When the first
**	DELETE or UPDATE statement is invoked on the cursor, rules that
**	apply to the parent table of the cursor are attached to the cursor
**	block and from there are reused.  Because they are saved in the final
**	statement form a small rule descriptor is saved which describes the
**	type of the rule.
**
**	This descriptor is saved in the pst_opf field, which is now a general
**	purpose internal field.
**
** History:
**	18-apr-89 (neil)
**	    Written
**	30-jul-92 (rblumer)
**	    FIPS CONSTRAINTS:  changed 'nat psr_column' to 
**             'DB_COLUMN_BITMAP psr_columns' as now users can specify 
**	       any number of columns in the rule's update action
*/
typedef struct _PSC_RULE_TYPE
{
    i4	     psr_statement; /* Same as dbr_statement
				    **   - currently only uses:
				    **     DBR_S_DELETE and DBR_S_UPDATE.
				    */
    DB_COLUMN_BITMAP psr_columns;   /* Same as dbr_columns (if DBR_S_UPDATE) */
} PSC_RULE_TYPE;

/*
** The following masks are used in tree-scanning routines
*/
#define		PSS_1SAW_AGG		0x01

/*
** Name: PSS_OBJ_NAME - structure describing (possibly qualified) object name
**
** Description:
**	This structure is used to describe an object name (which may or may not
**	be qualified with the name of its owner) as it was enetered by the user.
**
**	If pss_sess_table is non-zero, then this object is a temporary table
**	which has been qualified by entering MODULE.t as the table name.
**
**  History:
**	20-nov-89 (andre)
**	    Written
**	22-jan-1992 (bryanp)
**	    Added pss_sess_table for temporary table support.
**	17-aug-93 (andre)
**	    defined pss_objspec_flags, pss_schema_id_type, pss_obj_id_type
**
**	    replaced pss_qualified with PSS_OBJSPEC_EXPL_SCHEMA
**	    replaced pss_sess_table with PSS_OBJSPEC_SESS_SCHEMA
**	    defined PSS_OBJSPEC_REGID_SCHEMA and PSS_OBJSPEC_REGID_OBJNAME
*/
typedef struct _PSS_OBJ_NAME
{
    DB_OWN_NAME	    pss_owner;
    DB_TAB_NAME	    pss_obj_name;
    char	    *pss_orig_obj_name;	/* ptr to NULL-terminated object name */
    i4		    pss_objspec_flags;	/* useful bits of information */

					/* set if schema was specified */
#define PSS_OBJSPEC_EXPL_SCHEMA		((i4) 0x01)

					/*
					** set if the object belongs (or will
					** belong) to the schema used for 
					** temporary objects visible only 
					** within this session; for the time 
					** being only DGTTs live in this schema
					*/
#define	PSS_OBJSPEC_SESS_SCHEMA		((i4) 0x02)
    
    					/* 
					** type of identifier used to represent 
					** schema name - valid if 
					** PSS_OBJSPEC_EXPL_SCHEMA is set and 
					** PSS_OBJSPEC_SESS_SCHEMA is not set
					*/
    PSS_ID_TYPE	    pss_schema_id_type;	
					/*
					** type of identifier used to represent
					** object name
					*/
    PSS_ID_TYPE	    pss_obj_id_type;
} PSS_OBJ_NAME;

/*
** Name: PSS_COL_REF - structure describing a column reference
**
** Description:
**	This structure is used to describe a column reference (consisting of
**	column name possibly qualified with table/correlation name and possibly
**	qualified with schema name).
**
**  History:
**	25-sep-92 (andre)
**	    Written
*/
typedef struct _PSS_COL_REF
{
    DB_OWN_NAME	    pss_schema_name;	/*
					** this cannot be a ptr because if
					** SESSION was specified, there is no
					** NULL-terminated string to point at
					*/
    char	    *pss_tab_name;
    char	    *pss_col_name;
    i4		    pss_flags;
					/*
					** table name or correlation name was
					** specified (implies that schema name
					** was not specified)
					*/
#define	    PSS_TBL_OR_CORR_NAME_SPECIFIED	0x01
					/*
					** table name was specified (implies
					** that schema name was specified)
					*/
#define	    PSS_TBL_SPECIFIED			0x02
					/* schema name was specified */
#define	    PSS_SCHEMA_SPECIFIED		0x04
					/* [schema.]tbl.* was specified */
#define	    PSS_ALL_COLUMNS			0x08
} PSS_COL_REF;

/*
** defines for control flags which are used when calling pst_dbpshow() and
** psy_gproc()
*/
					/*
					** look for dbproc owned by the current
					** user
					*/
#define		PSS_USRDBP		((i4) 0x01)
					/*
					** look for dbproc owned by the DBA
					** PSS_DBADBP ==> PSS_USRDBP
					*/
#define		PSS_DBADBP		((i4) 0x02)
					/*
					** look for dbproc owned by a specific
					** owner
					** NOTE:
					** PSS_USRDBP <==> !PSS_DBP_BY_OWNER 
					*/
#define		PSS_DBP_BY_OWNER	((i4) 0x04)
					/*
					** dbproc is not expected to exist
					** (used when trying to create a new
					** dbproc.)
					** IMPORTANT: this mask is for use with
					**	      pst_dbpshow() only.
					*/
#define		PSS_NEWDBP		((i4) 0x08)
					/*
					** look for dbprocs owned by $INGRES
					** PSS_INGDBP ==> PSS_USRDBP
					*/
#define		PSS_INGDBP		((i4) 0x10)
					/*
					** upon finding the dbproc, check if the
					** user posesses privileges required for
					** the operation
					** IMPORTANT: this mask is for use with
					**	      pst_dbpshow() only.
					*/
#define		PSS_DBP_PERM_AND_AUDIT	((i4) 0x20)
					/*
					** obtain dbproc info by dbproc id; when
					** this bit is set, none of PSS_USRDBP,
					** PSS_DBADBP, PSS_DBP_BY_OWNER,
					** PSS_NEWDBP, PSS_INGDBP should be set
					*/
#define		PSS_DBP_BY_ID		((i4) 0x40)


/*
** defines for flags that can be returned by pst_dbpshow() and psy_gproc()
** using these flags will enable the caller to distinguish between expected
** (flag is set) and unexpected (status != E_DB_OK) conditions
*/
					/*
					** user lacks required privileges -
					** pst_dbpshow() only
					*/
#define		PSS_INSUF_DBP_PRIVS	((i4) 0x01)
					/*
					** user tried to create a dbproc even
					** though he already owns a dbproc with
					** the specified name - pst_dbpshow()
					** only
					*/
#define		PSS_DUPL_DBPNAME	((i4) 0x02)
					/* dbproc could not be found */
#define		PSS_MISSING_DBPROC	((i4) 0x04)

/*
** defines used when calling psl0_rngfcn
*/
#define		PSS_USRTBL	((i4) 0x01)	/* look at user's tables */
#define		PSS_DBATBL	((i4) 0x02)    /* look at dba's tables */
#define		PSS_INGTBL	((i4) 0x04)    /* look at $ingres' tables */
#define		PSS_SESTBL	((i4) 0x08)    /* look at "session." tables */

/*
** defines which will be used when calling psy_gevent()
*/
					/* 
					** look for dbevent owned by the current
					** user
					*/
#define		PSS_USREVENT	        ((i4) 0x01)

					/* look for dbevent owned by the DBA */
#define		PSS_DBAEVENT	        ((i4) 0x02)

					/* look for dbevent owned by $ingres */
#define		PSS_INGEVENT	        ((i4) 0x04)

					/* 
					** look for dbevent owned by a specific
					** owner
					*/
#define		PSS_EV_BY_OWNER	        ((i4) 0x08)
					/*
					** obtain dbevent info by dbevent id;
					** when this bit is set, none of
					** PSS_USREVENT, PSS_DBAEVENT,
					** PSS_INGEVENT, or PSS_EV_BY_OWNER
					** should be set
					*/
#define		PSS_EV_BY_ID		((i4) 0x10)

/*
** defines for flags that can be returned by psy_gevent() using these flags will
** enable the caller to distinguish between expected (flag is set) and 
** unexpected (status != E_DB_OK) conditions
*/
					/* 
					** user lacks some required privilege on
					** dbevent
					*/
#define		PSS_INSUF_EV_PRIVS  	((i4) 0x01)

					/* dbevent could not be found */
#define		PSS_MISSING_EVENT	((i4) 0x02)

/*
** defines which will be used when calling psy_gsequence()
*/
					/* 
					** look for sequence owned by the current
					** user
					*/
#define		PSS_USRSEQ	        ((i4) 0x01)

					/* look for sequence owned by the DBA */
#define		PSS_DBASEQ	        ((i4) 0x02)

					/* look for sequence owned by $ingres */
#define		PSS_INGSEQ	        ((i4) 0x04)

					/* 
					** look for sequence owned by a specific
					** owner
					*/
#define		PSS_SEQ_BY_OWNER        ((i4) 0x08)
					/*
					** obtain sequence info by sequence name
					** when this bit is set, none of
					** PSS_USRSEQ, PSS_DBASEQ,
					** PSS_INGSEQ, or PSS_SEQ_BY_OWNER
					** should be set
					*/
#define		PSS_SEQ_BY_ID		((i4) 0x10)

/*
** defines for flags that can be returned by psy_gsequence() using these flags will
** enable the caller to distinguish between expected (flag is set) and 
** unexpected (status != E_DB_OK) conditions
*/
					/* 
					** user lacks some required privilege on
					** sequence
					*/
#define		PSS_INSUF_SEQ_PRIVS  	((i4) 0x01)

					/* sequence could not be found */
#define		PSS_MISSING_SEQUENCE	((i4) 0x02)

/*
** psl[0]_[o]rngent() will now return info to its caller.  Below are defines for
** such info.
**
**  History:
**	09-apr-90 (andre)
**	    defined.
**	29-dec-2006 (dougi)
**	    Added PSS_WITH_ELEM.
*/
				    /*
				    ** name supplied by the caller was that of a
				    ** synonym
				    */
#define	    PSS_BY_SYNONYM	((i4) 0x02)
				    /*
				    ** retrieved info on an object or synonym
				    ** owned by the present user
				    */
#define	    PSS_USR_OBJ		((i4) 0x04)
				    /*
				    ** retrieved info on an object or synonym
				    ** owned by the DBA
				    */
#define	    PSS_DBA_OBJ		((i4) 0x08)
				    /*
				    ** retrieved info on an object or synonym
				    ** owned by the system ($INGRES)
				    */
#define	    PSS_SYS_OBJ		((i4) 0x10)
				    /*
				    ** retrieved info on a with list element
				    */
#define	    PSS_WITH_ELEM	((i4) 0x20)

/*
** Name: PSS_TBL_REF -	Structure of an element of a list of descriptions of
**			<table reference>s found in <column reference>s in the
**			target list
**
** Description:
**      This structure will hold a description of a <table reference> found in a
**	<column reference> in the target list of a SELECT.  We need to save them
**	because from_list will be processed after the target_list and we will
**	not have the necessary info in the range table at the time when the
**	target list is being processed.
**
** History:
**     26-sep-92 (andre)
**	    written as a part of adding support for <schema>.<table>.<column>
*/
typedef struct _PSS_TBL_REF
{
    struct _PSS_TBL_REF		*pss_next;
    DB_OWN_NAME			*pss_schema_name;   /*
						    ** NULL if schema name was
						    ** not specified
						    */
    DB_TAB_NAME			pss_tab_name;
}   PSS_TBL_REF;

/*
** Name: PSS_1JOIN -- this structure contains description of a join.
**
** Description:
**	This structure will contain description of a join, i.e. join type and
**	join id.
**
** History:
**	28-aug-89 (andre)
**	    written
**	25-nov-02 (inkdo01)
**	    Support for larger range tables.
*/
typedef struct _PSS_1JOIN
{
    PST_J_MASK	left_rels;
    PST_J_MASK	right_rels;
    DB_JNTYPE	join_type;
    PST_J_ID	join_id;
} PSS_1JOIN;

/*
** Name: PSS_JOIN_INFO -- this structure will contain info about joins
**				 involved in the FROM list.  
**
**  Description: this structure will contain info about joins involved in the
**		 FROM list.  We need this info so that we can correctly process
**		 the join search condition.  We'll keep track of the relations
**		 involved in the join as well as the join type.  Join depth will
**		 be used to determine which element of the arrays of PSS_1JOINs
**		 has the appropriate information.
**
**  History:
**	    28-aug-89 (andre)
**		written
**	25-nov-02 (inkdo01)
**	    Support for larger range tables.
*/
typedef struct _PSS_JOIN_INFO
{
    i4		depth;
    PSS_1JOIN	pss_join[PST_NUMVARS];
} PSS_JOIN_INFO;

/*
** Name: PSS_J_QUAL -- this structure will be used to keep track of join search
**		       quals.
**
** Description: for each subselect a new PSS_J_QUAL will be allocated with
**              pss_qual pointing to the qualification(s) found in the join
**		search condition and pss_next pointing to the list built so far
**
** History:
**	    31-aug-89 (andre)
**		defined
*/
typedef struct _PSS_J_QUAL
{
    struct _PSS_J_QUAL	    *pss_next;
    PST_QNODE		    *pss_qual;
} PSS_J_QUAL;

/*
** Name: PSS_DUPRB -- this structure will be used to communicate request to
**		      pss_treedup()
**
** Description: This structure will contain a mask to indicate requested
**		services (if any) and information which must be communicated to
**		pst_treedup() by its caller.  For the time being, we'll pass an
**		address of a mask used to communicate all sorts of useful info
**		to the caller + number of joins in the tree found so far +
**		number of RESDOMs found so far in the GROUP_BY list (counting
**		from the bottom).
**		If PSS_3DUP_RENUMBER_BYLIST is set, RESDOMs in the tree rooted
**		in the BYHEAD will be renumbered starting with 1 bottom up.  If
**		PSS_4DUP_COUNT_RSDMS is set, pss_numrsdms will be incremented
**		and the result will be stored in pst_rsno every time a tree
**		rooted in a RESDOM is processed.
**		If an appropriate mask is not set, value supplied by the caller
**		will not be guaranteed to be meaningful.  In addition to that we
**		supply ptr to the memory stream, ptr to the original tree, addr
**		of the ptr to point at the tree copy, and ptr to an error block.
**
** History:
**	    11-sep-89 (andre)
**		defined
**	    19-mar-90 (andre)
**		define PSS_5DUP_ATTACH_AGHEAD + add a PTR to be able to pass
**		a node ptr or some other very useful stuff.
**	    10-jun-90 (andre)
**		get rid of pss_rsdm_no and PSS_4DUP_COUNT_RSDMS.  Originally,
**		pss_rsdm_no was used to count number of resdoms in the
**		BY-list.  PSS_4DUP_COUNT_RSDMS was set if
**		PSS_3DUP_RENUMBER_BYLIST was set and we just hit a BYHEAD.  Upon
**		completion of processing of BYHEAD, PSS_4DUP_COUNT_RSDMS would
**		be reset.  This strategy relied on the assumption that there can
**		be no BYHEAD nodes in the tree rooted in BYHEAD.  As it happens
**		retrieve (sum((t1.i) by avg((t1.i) by t1.i))) would result in
**		precisely this situation.
**		pst_dup_viewtree() will be changed to deal with
**		PSS_3DUP_RENUMBER_BYLIST.
**	    30-sep-92 (andre)
**		until now, the only bit that could be set in pss_tree_info was
**		PST_3GROUP_BY.  This is inconvenient since pst_treedup() may be
**		called upon to determine all sorts of other interesting facts
**		about a query tree being copied (e.g. whether it contains
**		singleton subselects.)  Accordingly, we will define constants
**		over pss_tree_info rathewr than depend on those defined over
**		PST_RT_NODE.pst_mask1
**	    1-oct-92 (andre)
**		defined PSS_4SEEK_SINGLETON_SUBSEL over pss_op_mask
**	    31-dec-92 (andre)
**		undefined PSS_4SEEK_SINGLETON_SUBSEL and
**		PSS_SINGLETON_SUBSEL_IN_TREE.  Instead of rediscovering this
**		info every time we copy a view, we will save it inside the view
**		tree header
**	    04-dec-93 (andre)
**		defined PSS_MAP_VAR_TO_RULEVAR; will be used when copying
**		subtrees of the view's target list in the course of replacing
**		references to the attributes of a view with references to the
**		attributes of its underlying base table in a rule tree
**	    14-Apr-2008 (kiria01) b119410
**		Added PSS_JOINID_PRESET to allow presetting of pst_join_id.
**	    05-Dec-2008 (kiria01) b121333
**		Added PSS_JOINID_PRESET  to allow presetting of pst_
**	3-Aug-2010 (kschendel) b124170
**	    Add PSS_JOINID_STD telling pst-node to set the joinid in the
**	    "standard" manner, using the parser state.  See pst-node for
**	    the exact definition of "standard manner".
*/
typedef struct _PSS_DUPRB
{
    i4		    pss_op_mask;    /* request mask */
#define		PSS_1DUP_SEEK_AGHEAD	    0x01
#define		PSS_2DUP_RESET_JOINID	    0x02

				    /* renumber RESDOMs in the bylist */
#define		PSS_3DUP_RENUMBER_BYLIST    0x04

				    /*
				    ** having encountered a PST_VAR node, make
				    ** a copy of a PST_RULEVAR node pointed at
				    ** by pss_1ptr and overwrite pst_rltno with
				    ** PST_VAR_NODE.pst_vno
				    */
#define		PSS_MAP_VAR_TO_RULEVAR	    0x08
				    /*
				    ** After having successfully duplicated a
				    ** PST_AGHEAD node, attach it to the end of
				    ** the list passed by the caller
				    */
#define		PSS_5DUP_ATTACH_AGHEAD	    0x10

    i4		    pss_num_joins;  /* number of joins if the tree so far */

    i4		    *pss_tree_info; /*
				    ** ptr to a mask containing all sorts of
				    ** useful info
				    */

				    /*
				    ** came across an AGHEAD whose left child is
				    ** a BYHEAD
				    */
#define		PSS_GROUP_BY_IN_TREE	    ((i4) 0x01)

    PSF_MSTREAM	    *pss_mstream;
    PST_QNODE	    *pss_tree;
    PST_QNODE	    **pss_dup;
    PTR		    pss_1ptr;
    DB_ERROR	    *pss_err_blk;
} PSS_DUPRB;

/* A crude way of indicating that user has explicitly specified resdom's name */
#define			PSS_2RESDOM	207

/* Flags to be passed to be passed to pst_node() */
/*
** PSS_NORES and PSS_TYPERES_ONLY are mutually exclusive.
** They only may be set for operator nodes.
** PSS_NOALLOC may only be set when type of the node to be produced is PST_VAR
** or PST_CONST
*/
				    /* No type resolution to be performed */
#define		PSS_NORES	    0x01    /* PST_1[UAB]OP */

				    /*
				    ** Node has already been allocated with the
				    ** possible exception of space for data
				    ** values, but fields still need to be set
				    */
#define		PSS_NOALLOC	    0x02    /* PST_2CONST, PST_2VAR */

				    /* Only perform type resolution */
#define		PSS_TYPERES_ONLY    0x04    /* used to be PST_2[UBA]OP */
				    /*
				    ** Do not report type resolution errors to
				    ** the user
				    */
#define		PSS_NOTYPERES_ERR   0x08
#define		PSS_REF_TYPERES     0x10   /* if error, report special error */
				    /*
				    ** transforming avg(x) to sum(x)/count(x)
				    ** so adjust precision
				    */
#define		PSS_XFORM_AVG       0x20
				    /*
				    ** Passed opnode pst_joinid field is already
				    ** set. Don't override.
				    */
#define		PSS_JOINID_PRESET   0x40
				    /*
				    ** Passed opnode pst_flags field is already
				    ** set. Don't override.
				    */
#define		PSS_FLAGS_PRESET    0x80

				    /* Set this to tell pst_node that it is to
				    ** set the joinid (for an operator node)
				    ** in the "standard" manner using parser
				    ** state.  See pst-node for the exact
				    ** definition of "standard manner".
				    */
#define		PSS_JOINID_STD	    0x100

/*
** PSS_DBPALIAS	   a typedef for dbproc id to be used throughout PSF
**
** Description:	   presently, dbproc alias is identified by a DB_CURSOR_ID
**		   (2 i4's + dbpname (DB_DBP_MAXNAME)), owner (DB_OWN_MAXNAME),
**		   and sess_udbid (i4).  The second i4 in DB_CURSOR_ID may be
**		   1 or 0  to represent private and public alias,
**		   respectively.  All dbproc QEPs will have private alias so
**		   that they can always be executed by the dbproc owner.  To
**		   warrant creation of public alias, dbproc must be "grantable".
**
** History:
**	14-jun-90 (andre)
**	    defined.
*/
typedef char PSS_DBPALIAS[sizeof(DB_CURSOR_ID) + DB_OWN_MAXNAME + sizeof(i4)];

/*
** Name: PSS_TREEINFO -- this structure will be used to return a pointer to the
**			 tree generated by the production along with some useful
**			 facts about the tree.
**
** Description: This structure will contain a pointer to the tree generated by
**		the production along with some useful facts about the tree.
**
** History:
**	    26-apr-90 (andre)
**		defined
**	    17-nov-93 (andre)
**	        defined PSS_CORR_ATTR_IN_EXIST
*/
typedef struct _PSS_TREEINFO
{
    PST_QNODE	    *pss_tree;
    i4		    pss_mask;
					/* tree contains a SUBSEL node */
#define	    PSS_SUBSEL_IN_TREE		    ((i4) 0x01)

					/*
					** tree rooted in EXISTS contained
					** corelated references to attributes in
					** at least 2 different relations
					*/
#define	    PSS_0MULT_CORR_ATTR_IN_EXIST    ((i4) 0x02)

					/*
					** tree rooted in EXISTS contained a
					** correlated attribute reference
					*/
#define	    PSS_CORR_ATTR_IN_EXIST	    ((i4) 0x04)
} PSS_TREEINFO;

/*
** Name:    PSY_MAP_SIZE    - numbers of elements of type TYPE required to
**			      accomodate map of length LEN
*/
#define	    PSY_MAP_SIZE(len, type)  ((len - 1) / BITS_IN(type) + 1)

/*
** Name:    PSY_ATTMAP	    - map of attributes big enough to accomodate
**			      (DB_MAX_COLS + 1) columns (1 for TID).  I made it
**			      a structure to make it faster to copy it
**
** History:
**	06-aug-91 (andre)
**	    changed map from array of i2's to array of i4's.
*/
typedef	struct _PSY_ATTMAP
{
    i4	    map[DB_COL_WORDS];
} PSY_ATTMAP;

/*
** Name:	    PSY_VIEWINFO
**
** Description:	    this structure will contain useful information about views
**		    as the qrymod algorithm (to be more precise, the view and
**		    permit portions of it).  psy_attr_mask will be used in
**		    conjunction with non-mergeable views s.t. we will have to
**		    check permissions on one or more tables used in their
**		    definition (bug# 33038).
**
** History:
**	12-sep-90 (andre)
**	    written
*/
typedef struct _PSY_VIEWINFO
{
    i4		psy_flags;	    /* all sorts of useful info */

				    /*
				    ** range variable entries may be reused;
				    ** entry previously used by a view which has
				    ** since been expanded away may now be used
				    ** for a base table, but we keep the
				    ** structure around
				    */
#define	    PSY_NOT_A_VIEW	((i4) 0x01)

#define	    PSY_SQL_VIEW	((i4) 0x02)	/* SQL view */

    PSY_ATTMAP	*psy_attr_map;	    /*
				    ** attribute map ptr; the map will be used
				    ** (at least, initially) as follows:
				    ** if V is a non-mergeable view and there is
				    ** at least one relation used in the
				    ** definition of V s.t. we need to check if
				    ** the user has required permisions to
				    ** access that relation, this map will
				    ** indicate which attributes of V are used
				    ** in the main query tree.
				    */
} PSY_VIEWINFO;

/*
** Miscellaneous constants used by the grammars
**
** History:
**  XX-XX-91 (andre)
**	moved constants used by ASC_DESC type productions
**  6-mar-1991 (bryanp)
**	Also, moved the 'TYPE_NULL' defines from pslsgram.yi here so that
**	pslctbl.c could see them, and renamed them to PSS_TYPE_* in the
**	process for better namespace usage.
**  10-dec-92 (rblumer)
**	Added 5 to max INDEX_CHARS to account for unique_scope, persistence,
**	system-generated, not-user-droppable, and supports-constraint
**	char_entry's; added 2 to max MODIFY_CHARS for unique_scope and
**	persistence char_entry's.
**	18-Sep-2007 (kschendel) SIR 122890
**	    Add "empty" choice for asc/desc production
*/

/* constants used by ASC_DESC type productions */
#define	PSS_DESCENDING		    0
#define PSS_ASCENDING		    1
#define PSS_ASCDESC_EMPTY	    2

/* used to keep track of "with null", "not default", "with system_maintained" */

#define PSS_TYPE_NULL		    0x01
#define PSS_TYPE_NDEFAULT	    0x02
#define PSS_TYPE_SYS_MAINTAINED	    0x04

/* additional types used to denote "not null", "with default", etc.
**
** These are necessary because the syntax productions now allow nulls and
** defaults in any order and we must prevent conflicting ones from being
** defined on the same column.
*/
#define PSS_TYPE_NOT_NULL	    0x08    /* "not null"              */
#define PSS_TYPE_NOT_SYS_MAINTAINED 0x10    /* "not system_maintained" */
#define PSS_TYPE_DEFAULT	    0x20    /* ANY default--INGRES or user */
#define PSS_TYPE_USER_DEFAULT	    0x40    /* user-defined default    */

/* used for identity columns */

#define	PSS_TYPE_GENALWAYS	    0x80    /* "generate always as identity" */
#define	PSS_TYPE_GENDEFAULT	    0x100   /* "generate by default as identity" */
#define	PSS_TYPE_NAMED_IDENT	    0x200   /* explicitly named identity seq */

/* used for encryption */
#define PSS_ENCRYPT		    0x001
#define PSS_ENCRYPT_SALT	    0x002
#define PSS_ENCRYPT_CRC		    0x004

/*
** size of the characteristic array for [CREATE] INDEX
*/
#define	PSS_MAX_INDEX_CHARS	    16
/*
** size of the characteristic arrsy for MODIFY
*/
#define	PSS_MAX_MODIFY_CHARS	    16
/*
** maximum number of concurrent indexes that can be built 
*/
#define	PSS_MAX_CONCURRENT_IDX	    32

/*
** Name: PSY_COL_PRIVS - specify the column-specific privileges and the map(s)
**			 of attributes on which they are requested
**
** Description:
**	This structure consists of a map of column-specific privileges
**	described therein and map(s) of attributes for each of the privileges
**
** History:
**	30-jul-91 (andre)
**	    defined
**	07-dec-92 (andre)
**	    defined PSY_REFERENCES_ATTRMAP and increased PSY_NUM_PRIV_ATTRMAPS
**	    to 3
*/
typedef struct _PSY_COL_PRIVS
{
    i4		    psy_col_privs;

    /*
    ** number of maps will increase as we allow more column-specific privileges
    */
#define	    PSY_INSERT_ATTRMAP	    0	/* index of attribute map for INSERT */
#define	    PSY_UPDATE_ATTRMAP	    1	/* index of attribute map for UPDATE */
					/*
					** index of attribute map for REFERENCES
					*/
#define	    PSY_REFERENCES_ATTRMAP  2 
#define	    PSY_NUM_PRIV_ATTRMAPS   3	/* number of attribute maps	     */

    PSY_ATTMAP	    psy_attmap[PSY_NUM_PRIV_ATTRMAPS];
} PSY_COL_PRIVS;

/*
** masks used for flag field of psy_check_privs()
*/
#define PSY_PRIVS_GRANT_ALL		((i4) 0x01)
#define PSY_PRIVS_NO_GRANT_OPTION	((i4) 0x02)
#define PSY_PRIVS_DONT_PRINT_ERROR	((i4) 0x04)
#define	PSY_GRANT_ISSUED_IN_CRT_SCHEMA	((i4) 0x08)

/*
** masks defined over the mask returned by psy_qrymod() and psy_view()
*/
    /*
    ** will be set if a view being created appears to be grantable
    */
#define	    PSY_VIEW_IS_GRANTABLE		0x01L

    /*
    ** will be set if user attempted to specify WITH CHECK OPTION and an
    ** underlying view involved UNION
    */
#define	    PSY_UNION_IN_UNDERLYING_VIEW	0x02L

    /*
    ** will be set if user attempted to specify WITH CHECK OPTION and an
    ** underlying view involved DISTINCT
    */
#define	    PSY_DISTINCT_IN_UNDERLYING_VIEW 	0x04L

    /*
    ** will be set if user attempted to specify WITH CHECK OPTION and an
    ** underlying view was a multi-variable view
    */
#define	    PSY_MULT_VAR_IN_UNDERLYING_VIEW 	0x08L

    /*
    ** will be set if user attempted to specify WITH CHECK OPTION and an
    ** underlying view involved GROUP BY in the outermost subselect
    */
#define	    PSY_GROUP_BY_IN_UNDERLYING_VIEW     0x10L

    /*
    ** will be set if user attempted to specify WITH CHECK OPTION and an
    ** underlying view involved HAVING in the outermost subselect
    */
#define	    PSY_HAVING_IN_UNDERLYING_VIEW 	0x20L

    /*
    ** will be set if user attempted to specify WITH CHECK OPTION and an
    ** underlying view involved a set function (e.g. sum()) in the outermost
    ** subselect
    */
#define	    PSY_SET_FUNC_IN_UNDERLYING_VIEW     0x40L

    /*
    ** will be set if user attempted to specify WITH CHECK OPTION and an
    ** underlying view contained no updatable columns
    */
#define	    PSY_NO_UPDT_COLS_IN_UNDRLNG_VIEW	0x80L

/*
** Name: PSS_DBPLIST_HEADER	- structure used in an internal list of dbprocs
**				  owned by the curent user whose (dbprocs' that
**				  is) grantability must be determined; each
**				  header will be asociated with exactly one
**				  dbproc at any given time
**
** History:
**
**	10-oct-91 (andre)
**	    written
**	04-mar-92 (andre)
**	    changed the structure in conjunction with change in the algorithm
**	    used in psy_dgrant().
**	25-mar-92 (andre)
**	    added pss_dbp_mask;
**	15-may-92 (andre)
**	    replaced pss_rc_root with pss_flags and defined PSS_RC_ROOT,
**	    PSS_RC_MEMBER, and PSS_PROCESSED over it.
*/
typedef struct _PSS_DBPLIST_HEADER PSS_DBPLIST_HEADER;

struct _PSS_DBPLIST_HEADER
{
					/*
					** list header associated with a dbproc
					** which invoked the dbproc with which
					** this header is associated.  If P1 and
					** P2 call each other recursively,
					** dbproc processed first will be
					** treated as "parent"
					*/
    PSS_DBPLIST_HEADER      *pss_parent;
					/*
					** list header associated with a dbproc
					** which was processed first among
					** dbprocs invoked by the dbproc with
					** which this header is associated
					*/
    PSS_DBPLIST_HEADER      *pss_child;
					/*
					** list header associated with a dbproc
					** having the same *pss_parent which was
					** processed following the dbproc
					** associated with this header.
					*/
    PSS_DBPLIST_HEADER      *pss_sibling;
					/*
					** used to link all headers together for
					** easy reuse
					*/
    PSS_DBPLIST_HEADER	    *pss_next;

    /*
    ** next two fields will help us deal with chain recursion involving dbprocs
    ** owned by the current user:
    **      In the following discussion P1 will be called "parent" of P2 and P2
    **	will be called "child" of P1 if P1 contains invocation of P2 AND P1 was
    **	processed before P2 (this is important because P2 may itself contain
    **	invocation of P1); "descendent" and "ancestor" will be defined in
    **	traditional manner based on "parent" and "child" relationships; a dbproc
    **	P will be deemed grantable unless P or any of the dbprocs owned by P's
    **	owner and invoked by it depends on a privilege on an object owned by
    **	another user such that the P's owner lacks ability to grant that
    **	privilege.
    **	    If P1 calls P2 and P2 calls P1 then at what point can we claim that
    **	P1 and P2 are grantable?  Suppose P1 is a parent of P2.  I claim that
    **	by the time we are done processing all of P1's descendents (including
    **	P2), we will be able to mark both P1 and P2 as grantable.  Indeed, if
    **	all descendents of P1 (except for P2) are known to be grantable, then
    **	all descendents of P2 (except for P1) must also be grantable
    **	({children of P2} is a proper subset of {children of P1}).  By the above
    **	definition both P1 and P2 are grantable.
    **	    Suppose P1 calls P2, P2 calls P3, P3 calls P1, and both P2 and P3
    **	are descendents of P1.  By a simple extension of the argument in the
    **	previous paragraph, I claim that by the time we are done processing all
    **	of P1's descendents (including P2 and P3), we will be able to mark P1,
    **	P2, and P3 as grantable.
    **	    Thus a one-sentence description of the algorithm dealing with
    **	grantability of dbprocs owned by the current user involved in chain
    **	recursion is as follows: dbprocs involved in a chain of recursive calls
    **	are grantable if by the time we process all descendents of a dbproc
    **	which is the ancestor of all other dbprocs involved in the chain, we
    **	have not found a single ungrantable dbproc.
    */
					    /*
					    ** next element in the list of
					    ** dbprocs involved in a chain of
					    ** recursive calls
					    */
    PSS_DBPLIST_HEADER	    *pss_rec_chain;

					    /*
					    ** flags describing this header
					    */
    i4			    pss_flags;
    
					    /*
                                            ** this element is a root of a
					    ** recursive call chain, i.e. this
					    ** descriptor is associated with the
					    ** dbproc which is the ancestor of
					    ** all other dbprocs involved in the
					    ** chain of recursive calls
                                            */
#define			PSS_RC_ROOT	    ((i4) 0x01)

					    /*
					    ** this element is a member of a
					    ** recursive call chain
					    ** PSS_RC_ROOT --> PSS_RC_MEMBER
					    */
#define			PSS_RC_MEMBER	    ((i4) 0x02)

					    /*
					    ** subtree rooted in this element
					    ** has been processed
					    */
#define			PSS_PROCESSED	    ((i4) 0x04)

					    /*
					    ** begining of the list of
					    ** independent objects on which the
					    ** dbproc associated with this
					    ** header depends
					    */
    PSQ_OBJ		    *pss_obj_list;
					    /*
					    ** DB_DBPGRANT_OK bit will be set if
					    ** the dbproc appears to be
					    ** grantable;
					    ** DB_ACTIVE_DBP bit will be set if
					    ** the dbproc appears to be active
					    */
    i4			    pss_dbp_mask;
};

/*
** Name: PSS_EVINFO	- information about a dbevent
**
** History
**	
**	08-feb-92 (andre)
**	    written for use with psy_gevent()
**	6-dec-93 (robf)
**          Add pss_evsecid for security label.
*/
typedef struct _PSS_EVINFO
{
    DB_ALERT_NAME	pss_alert_name;
    DB_TAB_ID		pss_ev_id;
} PSS_EVINFO;

/*
** Name: PSS_SEQINFO	- information about a sequence.
**
** History
**	18-mar-02 (inkdo01)
**	    Written for sequence support.
*/
typedef struct _PSS_SEQINFO
{
    DB_OWN_NAME		pss_seqown;
    DB_NAME		pss_seqname;
    DB_TAB_ID		pss_seqid;
    DB_DT_ID		pss_dt;
    i2			pss_length;
    i2			pss_prec;
} PSS_SEQINFO;

/*
** constants describing <drop_behaviour>
*/
#define	    PSS_CASCADING_DROP	    ((i4) 0x01)
#define	    PSS_RESTRICTED_DROP	    ((i4) 0x02)

/*}
** Name: PSS_Q_PACKET_LIST - used to store queries into a linked list of
**			     packets.
**
** Description:
**      This structure will contain ptrs to the first and the last DD_PACKETs
**	along with number of bytes stored so far, ptr to the next available
**	position and to the last position.
**
** History:
**    29-SEP-88 (andre)
**	written.
**    15-jun-92 (barbara)
**	Merged Star's PSS_Q_PACKET_LIST into rplus for Sybil.
*/
typedef struct _PSS_Q_PACKET_LIST{
    DD_PACKET	    *pss_head;
    DD_PACKET	    *pss_tail;
    char	    *pss_next_ch;
    i4		    pss_skip;	/*
				** num of bytes from the beginning to skip
				** Most of the time, it will be 0, but for
				** CREATE LINK and REGISTER, it will be 2
				*/
} PSS_Q_PACKET_LIST;

#define	    PSS_0_COMMA_LEN	CMbytecnt(",")

/*}
** Name: PSS_Q_XLATE - used to store translated single site queries for distrib.
**
** Description:
**      This structure used to contain a pointer to the LDB descriptor of a
**	site for single-site queries along with the buffered text for the
**	query.  Now that single-site query buffering is no longer done by
**	PSF, this structure is reduced to containing one list of packets
**	of buffered text.  Text is buffered on those statements that are
**	handled by SCF or directly by QEF, e.g. COPY, CREATE TABLE and SET.
**
**	BB_FIX_ME: Get rid of this structure altogether.  Use a field
**	in the session control block.  Need to change interfaces of
**	functions in psqxlate.c plus others.
**
** History:
**    29-SEP-88 (andre)
**	written.
**    31-oct-88 (andre)
**	Added a list of PSF-generated aliases to the structure.
**    15-nov-88 (andre)
**	Added a ptr to the packet list being presently used.
**    20-dec-88 (andre)
**	Added pss_next and pss_num.
**    14-feb-89 (andre)
**	Added pss_ldb_site_info.
**    15-jun-92 (barbara)
**	Merged Star's PSS_Q_XLATE into rplus for Sybil.  Eliminated
**	pss_ldb_desc, pss_tmp_list, pss_ldb_site_info, pss_name, pss_qtype,
**	pss_next and pss_num.  The need for these fields goes away with
**	the removal of the single_site buffering code for DML statements.
**	For DDL, COPY and SET (and a few other statements), code is still
**	buffered in pss_q_list.
*/
typedef struct _PSS_Q_XLATE{
    PSS_Q_PACKET_LIST   pss_q_list;
    i4			pss_buf_size;
				      /* buffer size for CREATE LINK/REGISTER */
#define     PSS_CRLINK_BSIZE	    242
					/* max buffer size (other than above) */
#define	    PSS_MAXBSIZE	    128

					    /*
					    ** when approximating size of   
					    ** packet, multiply query	    
					    ** buffer size by this number
					    */
#define	    PSS_BSIZE_MULT	    1.25
} PSS_Q_XLATE;

/*
** The following flags will be used when calling pst_ldbtab_desc to check for
** existence or some specific properties of LDB tables.
*/
				    /*
				    ** check for duplicate LDB table owned by
				    ** the user
				    */
#define	PSS_DUP_TBL 	    0x01

#define PSS_GET_TBLCASE	    0x02    /* change table name case if necessary */

#define PSS_GET_OWNCASE	    0x04    /* change owner name case if necessary */

				    /*
				    ** check for duplicate LDB table (catalog)
				    ** owned by the LDB system catalog owner
				    */
#define PSS_DUP_CAT	    0x08
#define PSS_REGPROC	    0x010   /* this is a registered procedure object */
#define PSS_DELIM_TBL	    0x020   /* Delimited table name used */
#define PSS_DELIM_OWN	    0x040   /* Delimited owner name used */
#define PSS_DDB_MIXED	    0x080   /* DDB supports mixed case delimiters */

/*}
** Name: PSS_LTBL_INFO - used to store LDB table info needed to drop an LDB table
**
** Description:
**      This structure will contain name of the LDB table to be dropped along
**	with description of the LDB where it resides.
**
** History:
**    11-nov-88 (andre)
**	written.
**  07-may-91 (andre)
**	added pss_obj_type; this new field will only be used for SQL queries
**	simce in QUEL we can only say DESTROY, whereas in SQL we need to
**	specify DROP TABLE|VIEW|INDEX; this change was made as a part of fix for
**	bug 37414
**  15-jun-92 (barbara)
**	Merged Star's PSS_LTBL_INFO into rplus for Sybil.
**  31-may-93 (barbara)
**	Added pss_delim_tbl to indicate whether table name should be
**	delimited.
*/
typedef struct _PSS_LTBL_INFO {
    struct _PSS_LTBL_INFO   *pss_next;
    i4			    pss_obj_type;
#define		PSS_LTBL_IS_BASETABLE	1
#define		PSS_LTBL_IS_VIEW	2
#define		PSS_LTBL_IS_INDEX	3
    DD_TAB_NAME		    pss_tbl_name;
    i4			    pss_delim_tbl;
    DD_LDB_DESC		    pss_ldb_desc;
} PSS_LTBL_INFO;

/*
** The following flags is used to pass information to pst_header() 
*/
#define	    PST_0FULL_HEADER		0x01
#define	    PST_1INSERT_TID		0x02

/*
** definitions for masks indicating which portions of the site spec are to be
** manually appended by PSF
**
** History:
**	12-may-89 (andre)
**	    Created
**	12-jun-92 (barbara)
**	    Merged into rplus line for Sybil.
**      04-dec-92 (barbara)
**          Changed interface of psl_lm5_setlockmode and psl_send_setqry
**          to pass in PSQ_CB pointer.
*/

#define	    PSS_0DBMS_TYPE	    0x01
#define     PSS_1NODE_AND_DB_NAME   0x02



/*}
** Name: PSS_CONS - used to store info about SQL92 constraints until we
**    		    have enough info to validate them and convert them to tuples
**
** Description:
**      This structure will contain column names for unique constraints,
**      column names, referenced table name, and referenced column names 
**      for referential constraints, and the search condition for check 
**      constraints.
**
**      The column names must be stored because the constraints can be 
**      defined before the user defines the columns names and types.
**
** History:
**    25-aug-92 (rblumer)
**	written.
**    20-nov-92 (rblumer)
**	added pss_check_text, as need to save text as user typed it in and
**	added pss_check_rule_text, as QEF cannot add in correlation names.
**    06-may-93 (andre)
**	added pss_check_cons_cols.  If processing CHECK constraint specified in
**	ALTER TABLE statement, we will resolve column references immediately.
**	Numbers of attributes involved in a constraint will be stored in
**	$Ycheck_cons_cols.  Once we allocate a PSS_CONS structure describing the
**	constraint, we will allocate additional memory for DB_COLUMN_BITMAP
**	structure, set pss_check_cons_cols to point to it and copy contents of
**	$Ycheck_cons_cols into it.
**    08-jan-94 (andre)
**	added pss_cons_flags and defined PSS_NO_JOURNAL_CHECK over it
**	30-mar-98 (inkdo01)
**	    added pss_restab to contain index definition with clause options.
**	25-may-98 (inkdo01)
**	    Added flags to define CASCADE/SET NULL/RESTRICT referential actions.
**	20-oct-98 (inkdo01)
**	    Added flags for RESTRICT (for real, this time)/NO ACTION refacts.
**      3-Sep-2007 (kibro01) b119050
**          It should not be possible to specify NOINDEX or INDEX = BASE TABLE
**          STRUCTURE with any other options.

*/
typedef struct _PSS_CONS {
    struct _PSS_CONS   *pss_next_cons;
    i4	               pss_cons_type;
#define PSS_CONS_UNIQUE	    0x01    /* unique constraint (incl. primary key) */
#define PSS_CONS_CHECK	    0x02    /* check constraint */
#define PSS_CONS_REF	    0x04    /* referential constraint */
                                /* at least ONE of the above three MUST be set;
                                ** the ones below hold extra info
				*/
#define PSS_CONS_PRIMARY    0x08    /* primary key specified */
#define PSS_CONS_NOT_NULL   0x10    /* NOT NULL column constraint */
#define PSS_CONS_NAME_SPEC  0x20    /* constraint name specified by user */
#define PSS_CONS_COL	    0x40    /* column constraint */

    DB_CONSTRAINT_NAME pss_cons_name;    /* constraint name, if any */
    PSF_QUEUE	       pss_cons_colq;    /* list of column names  
                                            (unique or ref constraint) */
    PSS_OBJ_NAME       pss_ref_tabname;  /* referenced table name 
                                            (ref constraint only)*/
    PSF_QUEUE	       pss_ref_colq;     /* list of referenced columns */
    PSS_TREEINFO       *pss_check_cond;  /* condition for CHECK constraint */
    DB_TEXT_STRING     *pss_check_text;  /* query text for CHECK constraint */
    DB_TEXT_STRING     *pss_check_rule_text;  /* text for creating rule that
					      ** implements CHECK constraint
					      */
					 /*
					 ** map of columns involved in a CHECK
					 ** constraint specified using
					 ** ALTER TABLE statement
					 */
    DB_COLUMN_BITMAP   *pss_check_cons_cols;

    i4		       pss_cons_flags;
#define	PSS_NO_JOURNAL_CHECK	0x01	/* WITH NOJOURNAL_CHECK was specified */
#define PSS_CONS_INDEX_OPTS	0x02	/* constraint has index defn opts */
#define PSS_CONS_IXNAME		0x04	/* "index = <name>" was coded */
#define PSS_CONS_BASETAB	0x08	/* "index = base table structure " */
#define PSS_CONS_NOINDEX	0x10	/* "with no index" for ref const only */
#define PSS_CONS_OTHER_OPTS	0x20	/* index defn opts other than BASETAB */
#define PSS_CONS_DEL_CASCADE	0x100	/* ON DELETE CASCADE */
#define PSS_CONS_DEL_SETNULL	0x200	/* ON DELETE SET NULL */
#define PSS_CONS_DEL_RESTRICT	0x400	/* ON DELETE RESTRICT */ 
#define PSS_CONS_DEL_NOACT	0x800	/* ON DELETE NO ACTION */
#define PSS_CONS_UPD_CASCADE	0x1000	/* ON UPDATE CASCADE */
#define PSS_CONS_UPD_SETNULL	0x2000	/* ON UPDATE SET NULL */
#define PSS_CONS_UPD_RESTRICT	0x4000	/* ON UPDATE RESTRICT */
#define PSS_CONS_UPD_NOACT	0x8000	/* ON UPDATE NO ACTION */
    PST_RESTAB	       pss_restab;	/* contains constraint index options */
} PSS_CONS;

#define PSS_WC_MAX_OPTIONS		35

#ifdef xxx
typedef struct _PSS_WITH_CLAUSE
{
    char	pss_wc[((PSS_WC_MAX_OPTIONS+1)/BITS_IN(char)) + 1];
} PSS_WITH_CLAUSE;
#define PSS_WC_SET_MACRO(option, wc) BTset((char *)option, wc);
#define PSS_WC_CLR_MACRO(option, wc) BTclear((char *)option, wc);
#define PSS_WC_TST_MACRO(option, wc) BTtest((char *)option, wc);
#define PSS_WC_ANY_MACRO(wc)\
    (BTcount((char *)wc, sizeof(PSS_WITH_CLAUSE)) != 0 ? TRUE : FALSE)
#endif

typedef struct _PSS_WITH_CLAUSE
{
    char	pss_wc[PSS_WC_MAX_OPTIONS + 1]; /* element 0 not used */
} PSS_WITH_CLAUSE;

#define PSS_WC_SET_MACRO(option, wc) *((char *)wc + option) = 1;
#define PSS_WC_CLR_MACRO(option, wc) *((char *)wc + option) = 0;
#define PSS_WC_TST_MACRO(option, wc) (*((char *)wc + option) ? TRUE : FALSE)
#define PSS_WC_ANY_MACRO(wc)\
(BTcount((char *)wc, sizeof(PSS_WITH_CLAUSE)*BITS_IN(char)) != 0 ? TRUE : FALSE)

/* size used for string to pass into psl_command_string()
 */
#define  PSL_MAX_COMM_STRING	40

/*
**  THE FOLLOWING STRUCTURE HAS BEEN MOVED INTO PSHPARSE.H FROM YYVARS.H (RIP)
*/

/**
** Name: PSS_YYVARS - structure holding data to be shared by YACC actions.
**
** Description:
**  The typedef defines a structure holding various fields, whose values need to
**  be shared by grammar actions. They are "global" to the grammar. Reentrancy
**  is ensured because the variable will be allocated on xxxparse stack.
**  Definition of the actual variable, as well as initialization of its
**  various fileds, if required, is to be done from within yyvarsinit.h.
**
** History:
**  13-nov-87 (stec)
**	Converted from old yyvars.h.
**  19-jan-88 (stec)
**	Added list_clause.
**  27-oct-88 (stec)
**	Add updcollst.
**  09-mar-89 (neil)
**	Added in_rule.
**  03-aug-89 (andre)
**	Declared in_groupby_clause.  Previously, in_target_clause was being set
**	to TRUE when processing list of columns in the order_by clause to make
**	sure that correlated vars do not get used.  Since now there is
**	difference between handling of element of target list and group_by list,
**	it makes sense to define a new flag.
**	Also declared var_names.  This structure will be used to keep track of
**	names of range vars used in the target list.  We must remember this
**	names since the from list will be yet to be processed by the time we see
**	the target list.
**  17-aug-89 (andre)
**	declare join_groupid, join_cond, join_type
**  06-sep-89 (neil)
**	Added tmp_stmt as a temp PST_STATEMENT.
**  13-sep-89 (andre)
**	declare dup_rb
**  1-oct-89 (andre)
**	declared bylist_elem_no.
**  14-nov-89 (andre)
**	declared check_for_vlups
**  21-nov-89 (andre)
**	Added obj_spec.
**  15-may-90 (andre)
**	Added illegal_agg_relmask.
**	Background: until now use of correlated aggregates was prohibited +
**		    use of aggregates in WHERE clause outside of SUBSELECT
**		    was also prohibited;
**		    effective now this restriction will be modified as
**		    follows:
**			given
**			    SELECT ... FROM from_list WHERE where_clause ...,
**			use of aggregate(x.col_name) where x is a part of
**			the from_list will be prohibited.
**	This field will be used as follows: a bit in illegal_aggr_relmask will
**	be turned on for every relation x (as in the above example).  Upon
**	encountering an aggregate, we will check for use of any relations marked
**	in illegal_agg_relmask, and if a match is found, an error will be
**	reported.
**  17-may-90 (andre)
**	got rid of yyagg_nodes; renamed stk_yyagg_nodes to agg_list_stack;
**	agg_list_stack[cb->pss_qualdepth] will replace yyagg_nodes.
**  13-jun-90 (andre)
**	Added rngvar_info.  This field will be used to communicate info about
**	the range var between from_item production and other productions (such
**	as integtbl) involving from_item.
**
**	Also added qry_mask to contain all sorts of useful info about a query.
**	It would be desirable that bit masks rather than nats be used whenever
**	possible to save some memory.
**  28-nov-90 (andre)
**	moved flags defined over qry_mask into pshparse.h since they may be used
**	outside the semantic actions.
**  11-jan-91 (andre)
**	Added corr_aggr structure to YYVARS.  This structure will be used when
**	checking for presense of correlated aggregates in a query.
**  12-feb-91 (andre)
**	moved definitions of flags defined over list_clause into PSHPARSE.H
**  05-mar-91 (andre)
**	Defined PSS_NONREPEAT_PARAM and PSS_PM_IN_REPEAT_PARAM over qry_mask
**  6-mar-1991 (bryanp)
**	Added PSS_COMP_CLAUSE to the list_clause definitions and also added
**	definitions for the with_clauses field of the YYVARS.H struct.
**	These changes are to support Btree Index Compression; as part of
**	that project we are moving code out of the grammar files proper
**	into separate 'semantic actions' files such as pslmdfy.c, pslindx.c,
**	and pslctbl.c
**  20-mar-91 (andre)
**	defined PSS_ASCENDING and PSS_DESCENDING;
**  02-jul-91 (andre)
**	Added PSS_UNSUPPORTED_CRTTBL_WITHOPT to the list_clause definitions.
**	This new constant will be used if in the course of parsing
**	CREATE TABLE WITH-clause "option = (value_list)", we come across an
**	option which we don't support.
**  12-nov-91 (rblumer)
**    merged from 6.4:  25-mar-1991 (bryanp)
**	Added support for Btree index compression:
**	Added 'with_clauses' field to YYVARS to keep track of which WITH
**	clauses have been seen by a particular statement. Added bitflag
**	definitions for the with_clauses field in <pshparse.h>. Also added
**	PSS_COMP_CLAUSE to record when the WITH parser is in the state
**	"now parsing a WITH COMPRESSION clause" (also in <pshparse.h>).
**  15-jun-92 (barbara)
**	Sybil merge.  Added four fields for distributed processing:
**	    scanbuf_ptr - for statements where text must be accessed out
**		of scanner's buffer
**	    shr_qryinfo - Star REPEAT queries require a comparison of LDB range
**		tables to guard against reregistered tables.
**	    ldb_flags - flags related to WITH-clause parsing and other
**		LDB table info.
**	    xlated_qry - used to buffer statement text for queries that
**		don't go through OPC, e.g. COPY, CREATE TABLE, SET statements.
**  20-jul-92 (andre)
**	added priv - when processing a list of columns to which a privilege
**	specified in GRANT/REVOKE statement applies, this field will contain a
**	mask describing privilege being processed;
**	added priv_colq
**  20-jul-92 (andre)
**	defined PSS_EXCLUDE_COLUMNS
**  25-sep-92 (andre)
**	declared col_ref - this structure will be used to describe column
**	references
**  26-sep-92 (andre)
**	replaced var_names with tbl_refs (PSS_TBL_REF *) since instead of array
**	of table names we well be constructing a linked list of
**	<table reference> descriptions
**  25-oct-92 (rblumer)
**	Improvements for FIPS CONSTRAINTS.
**	Added cons_list (PSS_CONS *) for holding constraint info.
**  12-oct-92 (ralph)
**	Added id_type field in support of delimited identifiers.
**	See constants PSS_ID_* in pshparse.h for definitions of values
**  15-nov-92 (andre)
**	moved definition of YYVARS from YYVARS.H;
**
**	changed qry_mask from i4  to i4 and updated masks accordingly
**
**	added text_chain_bypass, underlying_rel
**  16-nov-92 (andre)
**	added comment to nonupdt indicating that it also used in CREATE VIEW
**	processing
**
**	defined nonupdt_reason - this field will be used to indicate WHY a given
**	query expression was deemed non-updateable
**  10-dec-92 (andre)
**	defined priv_colq
**  23-nov-92 (ralph)
**      CREATE SCHEMA:  Added new fields to PSS_YYVARS
**      Added bypass_actions to support %nobypass in YACC and grammars.
**      Added submode to indicate substatement type
**      Added deplist to track view dependencies
**      Added stmtstart to track beginning of substatement
**  03-jan-93 (andre)
**	defined rule_view_tree
**  10-dec-92 (rblumer)
**	Added with-clause (WC_) options for UNIQUE_SCOPE, UNIQUE,
**	and PERSISTENCE options; added default_tree for storing PST_QNODE
**	holding the user-defined default for a column.
**  01-mar-93 (barbara)
**	Changes in support of delimited ids in Star.  Moved YYVARS
**	variable "ldb_flags" into PSS_SESSBLK ("pss_distr_sflags").
**  06-may-93 (andre)
**	added check_cons_cols.  This structure will be used to record numbers of
**	attributes involved in a CHECK constraint specified using ALTER TABLE
**	statement
**  15-may-1993 (rmuth)
**	Add support for concurrent index builds, add 
**	PSS_WC_CONCURRENT_ACCESS.
**  26-jul-1993 (rmuth)
**	Add the "with_clause" parameter to psl_cp11_nm_eq_no and 
**	psl_cp2_copstmnt.
**   1-jun-1993 (robf)
**	Add PSS_WC_ROW_SEC_LABEL and PSS_WC_ROW_SEC_AUDIT
**  17-aug-93 (andre)
**	added schema_id_type.  will be used to store id_type for schema_spec
**   20-sep-93 (robf)
**      Add prototype for psl_chk_upd_row_seclabel().
**   28-oct-93 (robf)
**      Change psy_cpyperm prototype (perms_required not i4, not i2)
**  02-nov-93 (andre)
**	PSS_SUBSEL_IN_OR_TREE, PSS_ALL_IN_TREE, and PSS_MULT_CORR_ATTRS will be
**	defined over PSS_SESBLK.pss_flattening_flags instead of
**	PSS_YYVARS.qry_mask
**  15-nov-93 (andre)
**	PSS_CORR_AGGR will be defined over PSS_SESBLK.pss_flattening_flags 
**	instead of PSS_YYVARS.qry_mask
**  15-jul-94 (robf)
**      Added PSS_WC_ROW_NO_SEC_KEY
**	06-mar-1996 (stial01 for braynp)
**		Add PSS_WCPAGE_SIZE to track the WITH PAGE_SIZE clause
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added PSS_WC_PRIORITY.
**	21-jul-97 (inkdo01)
**	    Added PSS_CRTAB_AS_SELECT to aid in permitting parm markers in
**	    PREPAREd cr table ... as select
**	18-nov-97 (inkdo01)
**	    Added $Yinhaving to track nested having refs for "count(*)"-in-
**	    where-clause detection.
**	27-mar-98 (inkdo01)
**	    Added $Ysavemode to save psq_mode during constraint with clause
**	    handling.
**	14-oct-98 (inkdo01)
**	    Added $Yinconstraint to track whether constraint "with" clause
**	    applies to a constraint.
**	27-nov-98 (inkdo01)
**	    Added in_orderby & ordby_qd to support non-select list 
**	    order by entries (and pst_sqlsort());
**	3-mar-00 (inkdo01)
**	    Added first_n for "select first n ..." feature.
**	11-mar-02 (inkdo01)
**	    Added seq_ops boolean to indicate use of next/currval.
**	1-may-02 (toumi01)
**	    issue 11873547 bug 107626 dbproc order by unselected cols
**	    Added pst_sqlsort parameter PST_QNODE *intolist so that
**	    we can add ORDER BY RESDOMs to procedure INTO parse tree.
**	15-may-02 (inkdo01)
**	    Added PSS_SESBLK parm to psl_hcheck call to enable adi_op_info call.
**	26-nov-02 (inkdo01)
**	    Range table expansion (change i4 masks to PST_J_MASK)
**	27-Jan-2004 (schka24)
**	    Define partition WITH option bit, clause list number.
**	    Define lots of partition parsing state.
**	19-jan-06 (dougi)
**	    Added parent PSS_YYVARS ptr and in_from_clause flag to help 
**	    with FROM clause subselects.
**	15-Mar-2006 (kschendel)
**	    Define holding area for funcall expr list
**	04-May-2006 (jenjo02)
**	    Add PSS_WC_CLUSTERED
**	15-mar-2007 (dougi)
**	    Add interval_frac for interval literals.
**	9-may-2007 (dougi)
**	    Add loc_count for FREE LOCATOR.
**	19-june-2007 (dougi)
**	    Add in_update_set_clause for unsupported scalar subselect detection.
**	18-july-2007 (dougi)
**	    Add offset_n for result offset clause.
**	26-oct-2007 (dougi)
**	    Add dynqp_comp for cached dynamic query plans.
**	8-feb-2008 (dougi)
**	    Add save_psq_mode for derived table processing.
**	19-march-2008 (dougi)
**	    Add named_parm for positional parameter validation.
**	6-june-2008 (dougi)
**	    Add flag values, done_identity and sequence for identity columns.
**	10-oct-2008 (dougi) Bug 120998
**	    Add rdonly flag for cached dynamic query processing.
**	7-May-2009 (kibro01) b121738
**	    Add union_head setting so that ORDER BY can determine if a column
**	    is located on both sides of a union or not.
**	28-May-2009 (kschendel) b122118
**	    Kill unused index-struct stuff.
**	20-Aug-2009 (kschendel) b122510
**	    Add holder for cast length in e.g. 20 in cast(x as varchar(20)).
**	    Reverse-engineering the cast length from the db_length is
**	    possible but annoying, this is easier.
**	14-Nov-2009 (kiria01) SIR 122894
**	    Add support for generic pseudo-polyadic functions for GREATER
**	    and LEAST (and CONCAT).
**	18-Mar-2010 (kiria01) b123438
**	    Scalar sub-query support changes. Renamed qual_type bit vector
**	    to more meaningful in_join_search and added similar bitvector
**	    for SINGLETON support.
**	23-Mar-2010 (kiria01) b123438
**	    Added first_n_ok_depth
**      02-Jun-2010 (coomi01) b123821
**          Add save_seq_ops[] of bool to PSS_YYVARS.
**	10-Aug-2010 (kschendel) b124222
**	    cast_length is used as i2 when a const node is created from it;
**	    it has to be declared i2 here (or, a real DB_ANYTYPE union used),
**	    or we pick up the wrong half on big-endian machines.
**	    Causes cast(thing as varchar(30)) to be wrong.
**/

/* For passing opflags to arg_stack users */
typedef union _PSS_ARG_FL {
    PST_QNODE	*arg;
    u_i4	opflags;
#define PSS_ARG_FL_EMPTY	    0x0000
#define PSS_ARG_FL_CHAIN_FN	    0x0001
} PSS_ARG_FL;

typedef struct PSS_YYVARS_ {

    struct PSS_YYVARS_	*prev_yyvarsp;	/* ptr to "parent" PSS_YYVARS when
				    ** processing subselect in FROM clause */
#define	    MAX_NESTING	    100
    YYAGG_NODE_PTR  *agg_list_stack[MAX_NESTING]; /* same depth as from list */
    i4	    with_journaling;
    i4	    with_location;
    i2	    with_dups;		    /* pst_alldups,pst_nodups,pst_dntcaredups */
    i4	    is_heapsort;
    i4	    in_from_clause;
    i4	    in_where_clause;
    i4	    in_target_clause;
    i4	    target_clause_depth;    /* qual depth of outer target list active */
    i4	    in_groupby_clause;
    i4	    in_update_set_clause;
    i4	    groupby_quantifier;	    /* new GROUP BY can have ALL/DISTINCT */
    i4	    in_rule;		    /* in rule statement */
    i4            in_case_function;

    PST_J_MASK flists[MAX_NESTING];    /* assume no more that a nesting
				    ** of 100 subselects.
				    */
    PST_QNODE	    *sort_list;
    DB_DATA_VALUE   db_pdata;
    DB_ANYTYPE	    db_vdata;
    PST_J_MASK	    from_list;
    PST_QNODE	    *resdmhd;

    /* Function call parsing "func(expr,...)" is recursive.  The args
    ** for the current function begin at index arg_base+1 and the next
    ** arg to be parsed goes into index arg_ix.  The location arg_base+0
    ** points to a flags word set in func:. At close-paren time
    ** there are arg_ix - arg_base - 1 arguments.
    ** To handle nesting, when an open-paren is encountered, we stack
    ** arg_base and set arg_base = arg_ix.  At close paren we set
    ** arg_ix = arg_base and pop arg_base, thus pruning the closed
    ** function's args off the arg-stack.
    ** The argument pointers are in func_args.  Rather than make it large
    ** enough for maximum nesting (which would be something like
    ** ADI_MAX_OPERANDS * PSS_YYMAXDEPTH which might be large if max
    ** operands has been raised), make it large enough for one function
    ** plus a little.  If we fill the area, a new larger area is
    ** allocated from the pss_funarg_stream memory stream.
    */

#define PSS_INITIAL_ARGS (ADI_MAX_OPERANDS + 10)
    PSS_ARG_FL	    initial_args[PSS_INITIAL_ARGS];  /* usually suffices */
    PSS_ARG_FL	    *func_args;	/* sel-expr-or-null's */
    i4		    max_args;		/* Number of func_args max */
    i4		    arg_base;		/* Index of this func's first arg */
    i4		    arg_ix;		/* Where next func arg goes */
    i4		    arg_stack[PSS_YYMAXDEPTH]; /* Stacked arg_base's */
    i4		    arg_stack_ix;	/* arg_stack pointer, ix++ to push,
					** --ix to pop
					*/

    bool	    nonupdt;	/* TRUE if the 'query' production has one
				** of the following (at the outmost level):
				** DISTINCT, UNION, aggregate function,
				** GROUP BY, HAVING, ORDER BY or more than
				** one variable. This indicator is used by
				** the cursor code to check whether FOR
				** UPDATE clause is acceptable for a given
				** stmnt.
				**
				** is is also used by CREATE VIEW code to
				** determine whether a view may be defined
				** WITH CHECK OPTION
				*/
    bool	    dynqp_comp;	/* TRUE if open of dynamic query has found
				** a query plan already compiled */

    i2  	    inhaving;	/* non-zero if we're nested inside a having
				** clause (and count(*) is legitimate) */
    i4		    upobjtype;

				/* the reason why nonupdt was set to TRUE */
    i4	    nonupdt_reason;
#define	PSS_ORDER_BY_IN_OUTERMOST_SUBSEL	0x01
#define	PSS_UNION_IN_OUTERMOST_SUBSEL		0x02
#define PSS_DISTINCT_IN_OUTERMOST_SUBSEL	0x04
#define	PSS_MULT_VAR_IN_OUTERMOST_SUBSEL	0x08
#define PSS_GROUP_BY_IN_OUTERMOST_SUBSEL        0x10
#define	PSS_HAVING_IN_OUTERMOST_SUBSEL		0x20
#define PSS_SET_FUNC_IN_OUTERMOST_SUBSEL        0x40

    i4	    dsql_maxparm;	/* max parm found in the current statement */
    DB_DATA_VALUE **dsql_plist;	/* address of array of ptrs to parm values */
    i4	    sort_by;		/* TRUE if sort by clause is used */
    i4	    agg_func;		/* TRUE if aggregate with BY list or GROUP BY
				** is used.
				*/
    i4	    aggr_allowed;	
#define PSS_AGG_ALLOWED 1	/* If bit 0 is on, aggregates may be
				** specified in the current clause. Unless
				** further restrictions applied.
				*/
#define PSS_STMT_AGG_ALLOWED 2	/* If bit 1 is on, aggregates are allowed in the
				** current statement.
				*/

#define PSS_AGG_SS_GT_0 4	/* If bit 2 is on, aggregates are allowed in the
				** current statement but only if in subselect and
				** this will be indicated by depth > 0 (insert values).
				*/
#define PSS_AGG_SS_GT_1 8	/* If bit 3 is on, aggregates are allowed in the
				** current statement but only if in subselect and
				** this will be indicated by depth > 1 (update set).
				*/

    i4	    list_clause;	/* if non zero, then a WITH KEY,LOCATION, or
				** COMPRESSION clause is being processed.
				*/

#define	PSS_KEY_CLAUSE			1
#define	PSS_NLOC_CLAUSE			2
#define	PSS_OLOC_CLAUSE			3
#define PSS_COMP_CLAUSE			4
#define PSS_UNSUPPORTED_CRTTBL_WITHOPT	5
#define PSS_SECAUDIT_CLAUSE		6
#define PSS_SECAUDIT_KEY_CLAUSE		7
#define	PSS_RANGE_CLAUSE		9
#define PSS_PARTITION_CLAUSE		10

   PSS_WITH_CLAUSE	    with_clauses;
				/*
				** with_clauses keeps track of which WITH
				** clauses have been seen by a particular
				** statement. When a particular clause is
				** parsed, the appropriate bit is set in the
				** field. If the bit is already set, then
				** this WITH clause has been given twice and
				** the statement is rejected.
				*/

#define PSS_WC_STRUCTURE		1
#define PSS_WC_FILLFACTOR   		2
#define PSS_WC_MINPAGES	    		3
#define PSS_WC_MAXPAGES	    		4
#define PSS_WC_LEAFFILL	    		5
#define PSS_WC_NONLEAFFILL  		6
#define PSS_WC_KEY	    		7
#define PSS_WC_LOCATION	    		8
#define PSS_WC_COMPRESSION  		9
#define PSS_WC_NEWLOCATION  		10
#define PSS_WC_OLDLOCATION  		11
#define PSS_WC_DUPLICATES   		12
#define PSS_WC_JOURNALING   		13
#define	PSS_WC_ALLOCATION   		14
#define PSS_WC_EXTEND	    		15
#define PSS_WC_TABLE_OPTION 		16
#define	PSS_WC_MAXINDEXFILL 		17
#define	PSS_WC_RECOVERY	    		18
#define	PSS_WC_UNIQUE_SCOPE 		19
#define	PSS_WC_UNIQUE	    		20
#define	PSS_WC_PERSISTENCE  		21
#define	PSS_WC_CONCURRENT_ACCESS	22
#define	PSS_WC_PAGE_SIZE		23
#define	PSS_WC_PRIORITY			24
#define	PSS_WC_CLUSTERED		25
#define PSS_WC_ROW_SEC_AUDIT		26
#define PSS_WC_ROW_SEC_KEY		27
#define PSS_WC_ROW_NO_SEC_KEY		28
#define PSS_WC_PARTITION		29
#define	PSS_WC_DIMENSION		30
#define PSS_WC_RANGE			31
#define PSS_WC_CONS_INDEX		32
#define PSS_WC_BLOBEXTEND		33
#define PSS_WC_CONCURRENT_UPDATES	34
#define	PSS_WC_AUTOSTRUCT		35
#define PSS_WC_MAX			35
#if PSS_WC_MAX > PSS_WC_MAX_OPTIONS
blow chunks now!
#endif

    PSS_EXLIST	*exprlist;	/* points to the head of an expression list
				** used in "... where select_expr (not) in
				** (sel_expr_list)
				*/

    DB_CURSOR_ID    fe_cursor_id;	/* Saved FE cursor id for shared QP's.*/
    bool	    qp_shareable;	/* TRUE if QP shareable, else FALSE   */
    bool	    repeat_dyn;		/* TRUE if repeat dynamic select      */
    bool	    named_parm;		/* TRUE if at least one named parm has*/
					/* been processed for proc invocation */
    bool	    in_from_tproc;	/* TRUE if were parsing tproc */
    i4		    isdbp;		/* TRUE for CREATE PROCEDURE stmt.    */
    PSS_DBPINFO	    *dbpinfo;		/* holds info about current dbproc    */
    PST_QNODE	    *updcollst;		/* root of update column list	      */

    
    char 	    correlation_mask[MAX_NESTING / BITS_IN(char) + 1];
    PSS_TBL_REF	    *tbl_refs;		/*
					** list of <table references> mentioned
					** in a target list
					*/
    PSS_JOIN_INFO   pss_join_info;	/*
					** info about joins in the FROM list(s)
					*/
    PST_J_ID	    join_id;		/* join group id		*/
    PST_J_MASK	    join_tbls;		/*
					** tables appearing in this mask may not
					** be used in correlated queries.  Such
					** situation may occur when we have a
					** join_search_condition involving a
					** subselect
					*/
    i4		    qual_depth;		/*
					** will be used to offset into in_join_search
					*/
    u_char	    in_join_search[MAX_NESTING / BITS_IN(u_char) + 1];
					/*
					** will be used to tell whether the
					** qualification being processed is a
					** part of a join_search condition; a
					** bit will be set to 1 if we are
					** processing a partof the join_search
					** clause, and 0 otherwise.
					*/
    i4		    singleton_depth;	/* offset into singleton */
    u_char	    singleton[MAX_NESTING / BITS_IN(u_char) + 1];
					/* Set if the contained (SUB)SELECT neeeds
					** to be a singleton */
    PSS_J_QUAL	    first_qual;
    PSS_J_QUAL	    *j_qual;
    PSS_DUPRB	    dup_rb;
    PSS_OBJ_NAME    obj_spec;
    PSS_COL_REF	    col_ref;
    PSS_RNGTAB	    *rng_vars[PST_NUMVARS];
    PST_STATEMENT   *tmp_stmt;	/* Useful temp statement ptr */
    i4		    bylist_elem_no;
    i4		    check_for_vlups;
    i4	    qry_mask;	/*
				** this mask will be used to store all sorts of
				** useful info about a query.  It would make
				** sense to have bit masks instead of NATs
				*/

/*	notused		0x00000001 */

		/* For the (unusual) syntax KEY=(colname DESC), only
		** used by RMS gateway register-as-import, indicate that
		** the key is DESC (as opposed to empty or ASC).
		*/
#define PSS_QMASK_REGKEY_DESC	0x00000002

/*	notused		0x00000004 */

				/*
				** this flag will be used in conjunction
				** with queries which require that query
				** text be stored in IIQRYTEXT:
				** CREATE VIEW/PERMIT/INTEGRITY.
				** 
				** (NOTE: this doesn't apply to dbprocs)
				** 
				** this flag is set to indicate to
				** from_item production that if a name
				** specified was that of a synonym,
				** synonym name will be replaced with
				** the actual object name as follows:
				**
				** form_item	    change to
				** ---------	    ---------
				** syn_name	    tbl_name syn_name
				** syn_name corr    tbl_name corr
				**
				*/
#define	    PSS_REPL_SYN_WITH_ACTUAL 0x00000008L

				/*
				** this flag will be used in conjunction
				** with queries which require that query
				** text be stored in IIQRYTEXT:
				** CREATE VIEW/PERMIT/INTEGRITY.
				** 
				** (NOTE: this doesn't apply to dbprocs)
				**
				** If object name was not prefixed with
				** the owner name, name of the owner of
				** the actual table will be inserted in
				** front of the object name.
				** If, on the other name, object was a
				** synonym and was prefixed with the
				** owner name, owner name will be
				** replaced with that of the owner of
				** the actual table
				*/
#define	    PSS_PREFIX_WITH_OWNER    0x00000010L

				/*
				** This flag will be set when processing CHECK
				** constraint defined for a column of a table
				** beiong created
				*/
#define	    PSS_COL_CHECK_CONSTRAINT 0x00000020L

				/*
				** This flag will be set when processing CHECK
				** constraint defined for a table beiong created
				*/
#define	    PSS_TBL_CHECK_CONSTRAINT 0x00000040L

				/*
				** set if a QUEL repeat query contained
				** non-repeat parameter(s)
				*/
#define	PSS_NONREPEAT_PARAM	    0x00000100L

				/*
				** set if repeat parameter(s) in a QUEL repeat
				** query contained pattern matching character(s)
				*/
#define PSS_PM_IN_REPEAT_PARAM	    0x00000200L

				/*
				** set if user specified EXCLUDING (<column
				** list>) in the GRANT statement
				*/
#define	PSS_EXCLUDE_COLUMNS	    0x00000400L

				/* 
				** set if parsing CREATE TABLE ... AS SELECT.
				** Aids in permitting parm markers (?) in 
				** WHERE, but not elsewhere in table def
				** (e.g. in check constraint defs)
				*/
#define PSS_CRTAB_AS_SELECT	    0x00000800L

    i4		    priv;
    PSF_QUEUE       *priv_colq;
    PSF_QUEUE	    excluded_colq;
    PSY_COL	    dcol_list;	/* list of override column names */
    i4		    rngvar_info;
    PST_J_MASK	    illegal_agg_relmask;
    struct
    {
	i4	depth;
	i4	rgno;
	bool	found;
    } mult_corr_attrs;

    struct
    {
	i4	depth;
	bool	found;
    } corr_aggr;

    PTR		    scanbuf_ptr;/*
				** keep track of text in scanner's buffer
				*/

    DB_SHR_RPTQRY_INFO	*shr_qryinfo;	/* table info for shareable rpt qry */


    PSS_Q_XLATE	    xlated_qry; /*
				** used where parser must save query text
				** for sending to LDB, e.g. COPY, SET,
				** CREATE TABLE
				*/

    PSS_CONS	    *cons_list; /* linked list of constraints 
				** and their column info
				*/

    PST_QNODE	    *default_node; /* node containing user-defined default */
    DB_TEXT_STRING  *default_text; /* text of user-defined default, including
				   ** surrounding quotes (if a string constant)
				   */

				/*
				** description of identiifer representing schema
				** name; set/used when it is desired to know
				** useful facts about schema identifier after
				** contents of id_type get overwritten with
				** description of subsequent identifier
				** legal values are those defined over id_type
				*/
    PSS_ID_TYPE	    schema_id_type;

    PSS_ID_TYPE	    id_type;	/*
				** Used to indicate the identifier type
				** of the last recognized identifier.
				** NOTE: This field does not pertain
				** to an entire list of identifiers,
				** but only to the last identifier
				** accepted by the grammar.
				*/

				/*
				** address of the range variable representing
				** the view's simply underlying table
				*/
    PSS_RNGTAB		    *underlying_rel;
    bool            bypass_actions;
                                /*
                                ** Bypass subsequent semantic actions
                                ** in this statement, unless otherwise
                                ** directed in the grammar.
                                */
    bool	    done_identity;
				/*
				** identity column defined (used to detect
				** duplicate identity columns)
				*/
    bool	    ident_bydef; /* OVERRIDING USER VALUE clause found in
				** INSERT syntax */
    bool	    ident_always; /* OVERRIDING SYSTEM VALUE clause found in
				** INSERT syntax */
    i4		    ident_attno; /* attno of identity column in INSERT */
    DB_IISEQUENCE   sequence;	/*
				** DB_IISEQUENCE to fill in for identities
				*/
    DB_IISEQUENCE   *seqp;	/*
				** ptr to DB_IISEQUENCE tuple for sequence,
				** identity column operations
				*/
    i4              submode;    /*
                                ** Query mode of substatement within
                                ** CREATE SCHEMA
                                */
    PST_OBJDEP      *deplist;   /*
                                ** linked list of object dependencies
                                ** used to order CREATE VIEW statements
                                ** within the CREATE SCHEMA wrapper
                                */
    char            *stmtstart; /*
                                ** Pointer to start of substatement
                                ** used for CREATE SCHEMA.
                                */
    PSY_COL         lastcol;    /*
                                ** Last column name specified,
                                ** for CREATE SCHEMA reconstruction
                                ** of constraints.
                                */
				/*
				** if a rule is being defined on a view, we pump
				** a copy of the view tree through qrymod to
				** determine that the view is updatable + to
				** obtain subtrees corresponding to view's
				** attributes which [the subtrees] will be used
				** when we need to replace references to the
				** attributes of the view with references to the
				** attributes of the view's underlying base
				** table;
				** rule_view_tree will point at that tree
				*/
    PST_QNODE	    *rule_view_tree;
				/*
				** map of attributes involved in a CHEDCK
				** constraint specified using ALTER TABLE
				** statement
				*/
    DB_COLUMN_BITMAP	check_cons_cols;
    i4		    savemode;	/*
				** saved copy of psq_mode while performing 
				** constraint with clause processing
				*/
    bool	   unique;	/* parallel index project */
    bool	    inconstraint; /* TRUE - if compiling constraint def */
    bool	    in_orderby; /* TRUE - compiling order by clause */
    bool	    seq_ops;	/* TRUE - next/currval sequence operators have been issued */
    bool	    create_with_key; /* TRUE: KEY=() WITH option in a
				** CREATE TABLE-type context ??TEMP??
				*/
    bool	    groupby_cube_rollup; /* TRUE - if we've already got 
				** one of these */
    bool	    first_n_ok_depth; /* Trigger depth for first n */
    i2		    interval_frac; /* fractions of seconds in interval lit */
    i4		    ordby_qd;   /* to save scope across order by analysis */
    PST_QNODE	    *first_n;   /* count in "select first n ..." syntax */
    PST_QNODE	    *offset_n;	/* count in result offset clause */

/* The next few members are for parsing a PARTITION= definition.
** There are a goodly number of things to keep track of.
*/
    DB_PART_DEF	    *part_def;	/* Pointer to partition definition being built
				** This prototype is allocated with max
				** dimensions (DBDS_MAX_LEVELS)
				*/
    i2		    *part_cols;	/* Pointer to array of ON column numbers.
				** We simply allocate the maximum possible.
				*/
    bool	    rdonly;		/* TRUE if cursor opened FOR READONLY */
    char	    *part_col_names; /* Array of ON column names, for errors.
				** These names are white-trimmed and null
				** terminated.  Allocated as the maximum
				** number possible.
				*/
    DB_NAME	    *part_name_next; /* Where next partition name goes.o					** The base of the name holding area is
				** in the db-part-dim dimension descriptor.
				*/
    i4		    names_size;	/* Size (bytes) of the name holding area */
    u_i2	    names_left;	/* Number of slots left in holding area */
    u_i2	    names_cur;	/* Number of names seen so far in the current
				** name list being scanned
				*/
    u_i2	    name_gen;	/* A counter for generating names if needed */
    u_i2	    partseq_first;  /* First partition sequence for this
				** partition definition being parsed
				*/
    u_i2	    partseq_last;  /* Last partition sequence for this def.
				** The first, last sequence things are not
				** to be used until the "partition spec" is
				** parsed, ie right before the partition-WITH.
				*/
    DB_TEXT_STRING  **part_textptr_next;  /* Pointer to array of value-text
				** pointers.  Each piece of a partitioning
				** value is a text string.  There are "ncols"
				** text strings making up a partitioning
				** value, i.e. a breaks table entry.
				** We don't need the base, this is where the
				** next textptr goes.
				*/
    i2		    textptrs_left;  /* Slots left in the value-text-pointer
				** array.  If it fills up we just start a
				** new one, textptrs don't need to be
				** contiguous.
				*/
    i2		    textptrs_cur;  /* Number of value strings we've seen for
				** the current breaks table entry.
				** Used to verify that the user gave ncols
				** text values, and for DEFAULT.
				*/
    DB_TEXT_STRING  *part_text_next;  /* Start of actual value text, stored
				** as DB_TEXT_STRING's pointed to by the
				** textptr array.
				** We don't need the base, this is where the
				** next text string goes.
				*/
    i2		    text_left;	/* Bytes left in the value text holding
				** area.  If it fills up we just start a
				** new one, doesn't have to be contiguous.
				*/
    i2		    value_oper;	/* DBDS_OP_xxx operator that applies to the
				** partitioning value being parsed.
				*/
    u_char	    *value_start;  /* Pointer to start of a column constant */
    DB_PART_BREAKS  *part_breaks_next;  /* Where next breaks entry goes
				** (The base of the breaks array is in the
				** db-part-dim dimension area.)
				*/
    i4		    breaks_left;  /* Entries left in the breaks table */
    i4		    breaks_size;  /* Size (bytes) of the breaks table */
    bool	    first_break;  /* First break entry of partition clause.
				** (NOT first break of the dimension!)
				*/
    i4		    save_list_clause;  /* Saved $Ylist_clause during partition
				** WITH (recursive WITH).
				*/
    PSS_WITH_CLAUSE save_with_clauses;  /* Saved $Ywith_clauses during
				** partition WITH parsing, so that we don't
				** confuse the partition WITH and the outer
				** WITH.
				*/
    DM_DATA	    part_locs;	/* Holding tank for a partition WITH-clause.
				** When the with-clause is done, this gets
				** copied to a qeu-phypart-char entry and
				** re-used.  Holds space for DM_LOC_MAX
				** DB_LOC_NAME entries.
				*/
    /* The partition definition semantic routines like giving useful
    ** error messages, and it's a real pain in the patoot doing the
    ** "get query name" thing everywhere.  Get it once, at the start of
    ** the definition, and store it here.
    ** NOTE: this may or may not be generalizable to other actions.
    ** The query "name" can be altered at times, e.g. to parse ANSI
    ** constraints, so you have to be a little careful.  For now (Jan '04)
    ** only the partition definition stuff expects anything here.
    */
    char	    qry_name[PSL_MAX_COMM_STRING];  /* Current query name */
    i4		    qry_len;	/* length of the above string */

    /* "create table as select" has the result column names in the select's
    ** query tree.  The tree isn't hooked on to anything until the statement
    ** is all done.  This doesn't help partition definition much, because
    ** it's part of the WITH clause, which is part of the statement!  So
    ** just prior to diving into the WITH clause, create-table-as-select will
    ** jam a copy of the select query-tree pointer here, so that partition
    ** definition can find column names.
    */
    PST_QNODE	    *part_crtas_qtree;  /* RETINTO select query expr */

    /* The MODIFY statement has a variant for partitioned tables:
    ** modify master-table PARTITION logpart.logpart...
    ** The logical partitions must occur in dimension order, although it's
    ** legal to skip dimensions.  Define some stuff to keep track.  Since
    ** all we ultimately need to track is the logical partition number,
    ** just allocate one per possible dimension as part of the yyvars block.
    */
    i4		    md_part_lastdim;	/* Last dimension we saw, -1 if none */
    i4		    md_part_logpart[DBDS_MAX_LEVELS];  /* Logical partitions
					** for the MODIFY, -1 if not specified
					*/
    bool	    md_reconstruct;	/* TRUE if modify to reconstruct */
    i2		    save_pss_rsdmno[MAX_NESTING];	/* across derived table processing */
    i2		    cast_length;	/* N in cast(x to varchar(N)) */
    i4		    save_psq_mode;	/* ditto */
    i4		    loc_count;		/* count of locators in psq_locator */

    PST_QNODE	    *union_head;	/* Head of UNION tree if applicable */
    PST_QNODE	    *tlist_stack[MAX_NESTING]; /* same depth as from list */

    bool            save_seq_ops[MAX_NESTING]; /* set of flags used for seq_op indicators */
}   PSS_YYVARS;



/*
** values that can be returned by psy_view_is_updatable() if it determines that
** a view whose tree was passed to it is not updatable
*/
#define	    PSY_UNION_IN_OUTERMOST_SUBSEL	0x01
#define	    PSY_DISTINCT_IN_OUTERMOST_SUBSEL	0x02
#define	    PSY_MULT_VAR_IN_OUTERMOST_SUBSEL	0x04
#define	    PSY_GROUP_BY_IN_OUTERMOST_SUBSEL	0x08
#define	    PSY_AGGR_IN_OUTERMOST_SUBSEL	0x10
#define	    PSY_NO_UPDT_COLS			0x20

/*
** Prototype declarations of PSF functions
*/
typedef DB_STATUS (*PSS_CONS_QUAL_FUNC)(
                                        PTR             *qual_args,
                                        DB_INTEGRITY    *integ,
                                        i4             *satisfies);
FUNC_EXTERN STATUS
psf_scctrace(
	PTR	    arg1,
	i4	    msg_length,
	char        *msg_buffer);

#define psf_display	TRformat

FUNC_EXTERN VOID	psf_relay(
	char	*msg_buffer);

FUNC_EXTERN DB_STATUS	psfErrorFcn(
	i4            errorno,
	i4            detail,
	i4            err_type,
	i4            *err_code,
	DB_ERROR      *err_blk,
	PTR	      FileName,
	i4	      LineNumber,
	i4	      num_parms,
	...);

#ifdef HAS_VARIADIC_MACROS

#define psf_error(errorno,detail,err_type,err_code,err_blk,...) \
	    psfErrorFcn( errorno,detail,err_type,err_code,err_blk, \
	    		__FILE__,__LINE__,__VA_ARGS__)
#else

/* Variadic macros not supported */
#define psf_error psf_errorNV

FUNC_EXTERN DB_STATUS	psf_errorNV(
	i4            errorno,
	i4            detail,
	i4            err_type,
	i4            *err_code,
	DB_ERROR      *err_blk,
	i4	      num_parms,
	...);

#endif /* HAS_VARIADIC_MACROS */

FUNC_EXTERN VOID
psf_adf_error(
	ADF_ERROR   *adf_errcb,
	DB_ERROR    *err_blk,
	PSS_SESBLK  *sess_cb);
FUNC_EXTERN VOID
psf_rdf_error(
	i4	    rdf_opcode,
	DB_ERROR    *rdf_errblk,
	DB_ERROR    *err_blk);
FUNC_EXTERN VOID
psf_qef_error(
	i4	    qef_opcode,
	DB_ERROR    *qef_errblk,
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
psf_mopen(
	PSS_SESBLK  	   *sess_cb,
	i4		   objtype,
	PSF_MSTREAM	   *mstream,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psf_malloc(
	PSS_SESBLK  	   *sess_cb,
	PSF_MSTREAM	   *stream,
	i4                size,
	void		   *result,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psf_mclose(
	PSS_SESBLK  	   *sess_cb,
	PSF_MSTREAM        *stream,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
psf_mroot(
	PSS_SESBLK  	   *sess_cb,
	PSF_MSTREAM        *mstream,
	PTR		   root,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psf_mchtyp(
	PSS_SESBLK  	*sess_cb,
	PSF_MSTREAM     *mstream,
	i4		objtype,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psf_mlock(
	PSS_SESBLK  	*sess_cb,
	PSF_MSTREAM     *mstream,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psf_munlock(
	PSS_SESBLK  	*sess_cb,
	PSF_MSTREAM     *mstream,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psf_symall(
	PSS_SESBLK  *pss_cb,
	PSQ_CB	    *psq_cb,
	i4	    size);
FUNC_EXTERN DB_STATUS
psf_umalloc(
	PSS_SESBLK  	*sess_cb,
	PTR		mstream,
	i4	    	msize,
	PTR		*mresult,
	DB_ERROR	*err_blk);
FUNC_EXTERN PSS_SESBLK*
psf_sesscb();
FUNC_EXTERN DB_STATUS
psl_cp1_copy(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_Q_XLATE	*xlated_qry,
	PTR		scanbuf_ptr,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp2_copstmnt(
	PSS_SESBLK	*sess_cb,
	PSQ_MODE	*qmode,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp3_copytable(
	PSS_SESBLK	*sess_cb,
	PSS_RNGTAB	*resrange,
	DB_TAB_NAME	*tabname,
	PSS_Q_XLATE	*xlated_qry,
	DD_LDB_DESC	*ldbdesc,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp4_coparam(
	PSS_SESBLK	*sess_cb,
	PSS_Q_XLATE	*xlated_qry,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp5_cospecs(
	PSS_SESBLK	*sess_cb,
	PSS_Q_XLATE	*xlated_qry,
	char		*domname,
	PTR		scanbuf_ptr,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp6_entname(
	PSS_SESBLK	*sess_cb,
	char		*entname,
	DB_ERROR	*err_blk,
	PTR		*scanbuf_ptr);
FUNC_EXTERN DB_STATUS
psl_cp7_fmtspec(
	PSS_SESBLK	*sess_cb,
	DB_ERROR	*err_blk,
	bool		w_nullval);
FUNC_EXTERN DB_STATUS
psl_cp8_coent(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	bool		parse_coent,
	char		*type_name,
	i4		len_prec_num,
	i4		*type_lenprec,
	char		*type_delim);
FUNC_EXTERN DB_STATUS
psl_cp9_nm_eq_nm(
	PSS_SESBLK	*sess_cb,
	char		*name,
	char		*value,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp10_nm_eq_str(
	PSS_SESBLK	*sess_cb,
	char		*name,
	char		*value,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp11_nm_eq_no(
	PSS_SESBLK	*sess_cb,
	char		*name,
	i4		value,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp12_nm_eq_qdata(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*name,
	DB_DATA_VALUE	*value,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cp13_nm_single(
	PSS_SESBLK	*sess_cb,
	char		*name,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_copydelim(
	char               *delimname,
	char		   **delimchar);
FUNC_EXTERN DB_STATUS
psl_ct1_create_table(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_WITH_CLAUSE *with_clauses,
	PSS_CONS	*cons_list);

FUNC_EXTERN DB_STATUS
psl_crtas_fixup_columns(
	PSS_SESBLK	*sess_cb,
	PST_QNODE	*newcolspec,
	PST_QNODE	*query_expr,
	PSQ_CB		*psq_cb);

FUNC_EXTERN DB_STATUS
psl_ct2s_crt_tbl_as_select(
	PSS_SESBLK	*sess_cb,
	PST_QNODE	*newcolspec,
	PST_QNODE	*query_expr,
	PSQ_CB		*psq_cb,
	PST_J_ID	*join_id,
	PSS_Q_XLATE	*xlated_qry,
	PST_QNODE	*sort_list,
	PST_QNODE	*first_n,
	PST_QNODE	*offset_n);
FUNC_EXTERN DB_STATUS
psl_ct3_crwith(
	PSS_SESBLK	*sess_cb,
	i4		qmode,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_nm_eq_nm(
	PSS_SESBLK	*sess_cb,
	char		*name,
	char		*value,
	PSS_WITH_CLAUSE *with_clauses,
	i4		qmode,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_nm_eq_hexconst(
	PSS_SESBLK	*sess_cb,
	char		*name,
	u_i2		hexlen,
	char		*hexvalue,
	PSS_WITH_CLAUSE *with_clauses,
	i4		qmode,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_nm_eq_no(
	PSS_SESBLK	*sess_cb,
	char		*name,
	i4		value,
	PSS_WITH_CLAUSE *with_clauses,
	i4		qmode,
	DB_ERROR	*err_blk,
	PSS_Q_XLATE	*xlated_qry);
FUNC_EXTERN DB_STATUS
psl_nm_eq_dec(
	PSS_SESBLK	*sess_cb,
	char		*name,
	f8		*value,
	PSS_WITH_CLAUSE *with_clauses,
	i4		qmode,
	DB_ERROR	*err_blk,
	PSS_Q_XLATE	*xlated_qry);
FUNC_EXTERN DB_STATUS
psl_ct6_cr_single_kwd(
	PSS_SESBLK	*sess_cb,
	char		*keyword,
	PSS_WITH_CLAUSE	*with_clauses,
	i4		qmode,
	DB_ERROR	*err_blk,
	PSS_Q_XLATE	*xlated_qry);
FUNC_EXTERN DB_STATUS
psl_lst_prefix(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*prefix,
	PSS_YYVARS	*yyvarsp);
FUNC_EXTERN DB_STATUS
psl_ct8_cr_lst_elem(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_YYVARS	*yyvarsp,
	char		*element,
	PST_RESKEY	**reskey,
	PSS_Q_XLATE	*xlated_qry);
FUNC_EXTERN DB_STATUS
psl_ct9_new_loc_name(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*loc_name,
	char		*table_name,
	PSS_WITH_CLAUSE *with_clauses,
	PSS_Q_XLATE     *xlated_qry);
FUNC_EXTERN DB_STATUS
psl_ct10_crt_tbl_kwd(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_WITH_CLAUSE *with_clauses,
	bool		temporary);
FUNC_EXTERN DB_STATUS
psl_ct11_tname_name_name(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*name1,
	char		*name2,
	char		*name3,
	char		**value);
FUNC_EXTERN DB_STATUS
psl_ct12_crname(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_OBJ_NAME	*tbl_spec);
FUNC_EXTERN DB_STATUS
psl_ct13_newcolname(
	PSS_SESBLK	*sess_cb,
	char		*column_name,
	PSQ_CB          *psq_cb,
	PSS_Q_XLATE     *xlated_qry,
        PTR             *scanbuf_ptr);
FUNC_EXTERN DB_STATUS
psl_ct14_typedesc(
	PSS_SESBLK	*sess_cb,
	char		*type_name,
	i4		num_len_prec_vals,
	i4		*len_prec,
	i4		null_def,
	PST_QNODE	*default_node,
	DB_TEXT_STRING	*default_text,
	DB_IISEQUENCE	*identity_seq,
	PSQ_CB		*psq_cb,
	DB_COLL_ID	collationID,
	i4		encrypt_spec);
FUNC_EXTERN DB_STATUS
psl_ct15_distr_with(
	PSS_SESBLK	*sess_cb,
	char		*name,
	char		*value,
	PSS_Q_XLATE	*xlated_qry,
	bool		quoted_val,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_ct16_distr_create(
	PSS_SESBLK	*sess_cb,
	PSS_Q_XLATE	*xlated_qry,
	bool		simple_create,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_ct17_distr_specs(
	PSS_SESBLK	*sess_cb,
	PTR		scanbuf_ptr,
	PSS_Q_XLATE	*xlated_qry,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_ct18s_type_qual(
		    i4		qmode,
		    i4	qual1,
		    i4	qual2,
		    PSS_SESBLK	*sess_cb,
		    PSS_DBPINFO	*dbpinfo,
		    DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_ct19s_constraint(
		     PSS_SESBLK	  *sess_cb,
		     i4	  type,
		     PSY_COL	  *cons_cols,
		     PSS_OBJ_NAME *ref_tabname,
		     PSY_COL	  *ref_cols,
		     PSS_TREEINFO *check_cond,
		     PSS_CONS	  **consp,
		     DB_ERROR	  *err_blk);
FUNC_EXTERN DB_STATUS
psl_ct20s_cons_col(
		   PSS_SESBLK	*sess_cb,
		   PSY_COL	**colp,
		   char	  	*newcolname,
		   DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_ct21s_cons_name(
		    char	  *cons_name,
		    PSS_CONS	  *cons,
		    PSS_CONS	  **cons_listp);

FUNC_EXTERN DB_STATUS
psl_ct22s_cons_allowed(
		       PSS_SESBLK *sess_cb,
		       PSQ_CB     *psq_cb,
		       PSS_YYVARS *yyvarsp);
FUNC_EXTERN DB_STATUS
psl_alter_table(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_RNGTAB	*rngvar,
		PSS_CONS	*cons_list);
FUNC_EXTERN DB_STATUS
psl_alt_tbl(
	    PSS_SESBLK 	  *sess_cb,
	    PSQ_CB	  *psq_cb,
	    PSS_OBJ_NAME  *tbl_spec,
	    PSS_RNGTAB	  **rngvarp);
FUNC_EXTERN DB_STATUS
psl_alt_tbl_col(
	        PSS_SESBLK 	  *sess_cb,
	        PSQ_CB	  	  *psq_cb);
FUNC_EXTERN DB_STATUS
psl_alt_tbl_col_drop(
	    	     PSS_SESBLK 	  *sess_cb,
	    	     PSQ_CB	  	  *psq_cb,
	             DB_ATT_NAME 	  attname,
	             bool		  cascade); 
FUNC_EXTERN DB_STATUS
psl_alt_tbl_col_add(
	    	    PSS_SESBLK 	  *sess_cb,
	    	    PSQ_CB  	  *psq_cb,
	            DB_ATT_NAME	  attname,
	            PSS_CONS  	  *cons_list,
		    i4		  altopt);
FUNC_EXTERN DB_STATUS
psl_alt_tbl_col_rename(
                    PSS_SESBLK    *sess_cb,
                    PSQ_CB        *psq_cb,
                    DB_ATT_NAME   attname,
                    DB_ATT_NAME   *newname);
FUNC_EXTERN DB_STATUS
psl_alt_tbl_rename(
                     PSS_SESBLK    *sess_cb,
                     PSQ_CB        *psq_cb,
                     PSS_OBJ_NAME  *newname);
FUNC_EXTERN DB_STATUS
psl_verify_cons(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		i4		newtbl,
		struct _DMU_CB 	*dmu_cb,
		PSS_RNGTAB   	*rngvar,
		PSS_CONS   	*cons_list,
		PST_STATEMENT	**stmt_listp);
FUNC_EXTERN DB_STATUS
psl_gen_alter_text(
		   PSS_SESBLK	   *sess_cb,
		   PSF_MSTREAM	   *mstream,
		   SIZE_TYPE	   *memleft,
		   DB_OWN_NAME	   *ownname,
		   DB_TAB_NAME	   *tabname,
		   PSS_CONS	   *cons,
		   DB_TEXT_STRING  **query_textp,
		   DB_ERROR	   *err_blk);
FUNC_EXTERN VOID 
psl_command_string(
	i4  	    qmode, 
	DB_LANG     language,
	char 	    *command,
	i4     *length);
FUNC_EXTERN DB_STATUS
psl_rg1_reg_distr_tv(
	PSS_SESBLK	*sess_cb,
	PSS_OBJ_NAME	*reg_name,
	i4		ldb_obj_type,
	PSS_Q_XLATE	*xlated_qry,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_rg2_reg_distr_idx(
	PSS_SESBLK	*sess_cb,
	char		*reg_name,
	PSS_Q_XLATE	*xlated_qry,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_rg3_reg_tvi(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_rg4_regtext(
	PSS_SESBLK	*sess_cb,
	i4		ldb_obj_type,
	PSS_Q_XLATE	*xlated_qry,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_rg5_ldb_spec(
	PSS_SESBLK	*sess_cb,
	char		*name,
	i4		qmode,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_rg6_link_col_list(
	char		*colname,
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_ds1_dircon(
	PSQ_CB		*psq_cb,
	char		*name1,
	char		*name2,
	bool		is_distrib);
FUNC_EXTERN DB_STATUS
psl_ds2_dir_exec_immed(
	char		*name,
	bool		is_distrib,
	char		*exec_arg,
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_proc_func(
	PSS_SESBLK	*sess_cb,
	PSF_MSTREAM	*stream,
	i4		op_code,
	PST_QNODE	*arg,
	PST_QNODE	**newnode,
	DB_ERROR	*err_blk);
FUNC_EXTERN i4
getfacil(
	char               *code,
	PSQ_MODE	   *facility,
	i4                 *flagno);
FUNC_EXTERN char *
sconvert(
	DB_TEXT_STRING     *string);
FUNC_EXTERN DB_STATUS
qdata_cvt(
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb,
	DB_DATA_VALUE	*fval,
	DB_DT_ID	totype,
	PTR		*value);
FUNC_EXTERN VOID
psl_quel_shareable(
	PSS_SESBLK	*sess_cb,
	i4		qry_mask,
	bool		*shareable);
FUNC_EXTERN DB_STATUS
psl_rptqry_tblids(
	PSS_SESBLK	*sess_cb,
	PSS_USRRANGE	*rngtab,
	PSF_MSTREAM     *mstream,
	i4		qry_mode,
	i4		*num_ids,
	DB_TAB_ID	**id_list,
	DB_ERROR        *err_blk);
FUNC_EXTERN DB_STATUS
psl_ci1_create_index(
	PSS_SESBLK	*sess_cb,
	PSS_WITH_CLAUSE *with_clauses,
	i4		unique,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_ci2_index_prefix(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_WITH_CLAUSE *with_clauses,
	i4		unique);
FUNC_EXTERN DB_STATUS
psl_ci3_indexrel(
	PSS_SESBLK	*sess_cb,
	PSS_OBJ_NAME	*tbl_spec,
	DB_ERROR	*err_blk,
	i4		qmode);
FUNC_EXTERN DB_STATUS
psl_ci4_indexcol(
	PSS_SESBLK	*sess_cb,
	char		*colname,
	DB_ERROR	*err_blk,
	bool		cond_add);
FUNC_EXTERN DB_STATUS
psl_ci5_indexlocname(
	PSS_SESBLK	*sess_cb,
	char		*loc_name,
	PSS_OBJ_NAME	*index_spec,
	PSS_WITH_CLAUSE *with_clauses,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_lst_elem(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	char		*element,
	i4		qmode,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_lst_relem(
	PSS_SESBLK	*sess_cb,
	i4		list_clause,
	f8		*element,
	i4		qmode,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_ci7_index_woword(
	PSS_SESBLK	*sess_cb,
	char		*word,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cv1_create_view(
	PSS_SESBLK          *sess_cb,
	PSQ_CB              *psq_cb,
	PST_QNODE           *new_col_list,
	PST_QNODE           *query_expr,
	i4                 check_option,
	PSS_YYVARS          *yyvarsp);
FUNC_EXTERN DB_STATUS
psl_cv2_viewstmnt(
	PSS_SESBLK      *sess_cb,
	PSQ_CB          *psq_cb,
	PSS_YYVARS      *yyvarsp);
FUNC_EXTERN DB_STATUS
psl_validate_options(
	PSS_SESBLK	*sess_cb,
	i4		qmode,
	PSS_WITH_CLAUSE	*options,
	i4		sstruct,
	i4		minp,
	i4		maxp,
	i4		dcomp,
	i4		icomp,
	i4		sliced_alloc,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_lm1_setlockstmnt(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_lm2_setlockkey(
	PSS_SESBLK  *sess_cb,
	char	    *char_name,
	i4	    *char_type,
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
psl_lm3_setlockparm_name(
	i4	    char_type,
	char	    *char_val,
	PSS_SESBLK  *sess_cb,
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
psl_lm4_setlockparm_num(
	i4	    char_type,
	i4	    char_val,
	PSS_SESBLK  *sess_cb,
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
psl_lm5_setlockmode_distr(
	PSS_SESBLK  	*sess_cb,
	char		*scanbuf_ptr,
	PSS_Q_XLATE     *xlated_qry,
	PSQ_CB    	*psq_cb);
FUNC_EXTERN DB_STATUS
psl_lm6_setlockscope_distr(
	PSS_SESBLK  	*sess_cb,
	PSS_RNGTAB	*rngvar,
	PSS_Q_XLATE     *xlated_qry,
	DB_ERROR    	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md1_modify(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md2_modstmnt(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_YYVARS	*yyvarsp);
FUNC_EXTERN DB_STATUS
psl_md3_modstorage(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md4_modstorname(
	PSS_SESBLK	*sess_cb,
	char		*storname,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md5_modkeys(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md6_modbasekey(
	PSS_SESBLK	*sess_cb,
	i4		ascending,
	i4		heapsort,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md7_modkeyname(
	PSS_SESBLK	*sess_cb,
	char		*keyname,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md8_modtable(
	PSS_SESBLK	*sess_cb,
	PSS_OBJ_NAME	*tblspec,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_md9_modopt_word(
	PSS_SESBLK	*sess_cb,
	char		*word,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk);

FUNC_EXTERN DB_STATUS
psl_md_logpartname(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_YYVARS	*yyvarsp,
	char		*logname);

FUNC_EXTERN DB_STATUS
psl_md_modpart(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_YYVARS	*yyvarsp);

FUNC_EXTERN DB_STATUS
psl_rngent(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *varname,
	DB_TAB_NAME	   *tabname,
	PSS_SESBLK	   *cb,
	bool		   tabonly,
	PSS_RNGTAB	   **rngvar,
	i4		   query_mode,
	DB_ERROR	   *err_blk,
	i4                *caller_info,
	PST_J_ID	   *pjoin_id);
FUNC_EXTERN DB_STATUS
psl_drngent(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *varname,
	PSS_SESBLK	   *cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	i4		    type,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psl_tprngent(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *corrname,
	i4		    dbpid,
	PSS_SESBLK	   *cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psl0_rngent(
	PSS_USRRANGE    *rngtable,
	i4		scope,
	char		*varname,
	DB_TAB_NAME	*tabname,
	PSS_SESBLK	*cb,
	bool		tabonly,
	PSS_RNGTAB	**rngvar,
	i4		query_mode,
	DB_ERROR	*err_blk,
	i4		tbls_to_lookup,
	i4		*caller_info,
	i4		lookup_mask,
	PST_J_ID	*pjoin_id);
FUNC_EXTERN DB_STATUS
psl_orngent(
	PSS_USRRANGE    *rngtable,
	i4		scope,
	char		*varname,
	DB_OWN_NAME	*ownname,
	DB_TAB_NAME	*tabname,
	PSS_SESBLK	*cb,
	bool		tabonly,
	PSS_RNGTAB	**rngvar,
	i4		query_mode,
	DB_ERROR	*err_blk,
	i4             *caller_info);
FUNC_EXTERN DB_STATUS
psl0_orngent(
	PSS_USRRANGE    *rngtable,
	i4		scope,
	char		*varname,
	DB_OWN_NAME	*ownname,
	DB_TAB_NAME	*tabname,
	PSS_SESBLK	*cb,
	bool		tabonly,
	PSS_RNGTAB	**rngvar,
	i4		query_mode,
	DB_ERROR	*err_blk,
	i4             *caller_info,
	i4		lookup_mask);
FUNC_EXTERN DB_STATUS
psl_csequence(
	PSS_SESBLK 	*cb,
	DB_IISEQUENCE	*seqp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_asequence(
	PSS_SESBLK 	*cb,
	DB_IISEQUENCE	*oldseqp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_seq_parmcheck(
	register PSS_SESBLK *cb,
	register PSQ_CB	    *psq_cb,
	DB_DATA_VALUE	    *pdv,
	DB_IISEQUENCE	    *seqp,
	i4		    flag,
	bool		    minus);
FUNC_EXTERN i4
psl_scan(
	register PSS_SESBLK *pss_cb,
	register PSQ_CB	    *psq_cb);
FUNC_EXTERN DB_STATUS
psl_subsel(
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb,
	PST_QNODE	*subnode);
FUNC_EXTERN PST_QNODE *
psl_find_node(
	PST_QNODE   *node,
	PST_QNODE   **cursor,
	PST_TYPE    type);
FUNC_EXTERN PST_QNODE **
psl_find_nodep(
	PST_QNODE   **node,
	PST_QNODE   ***cursor,
	PST_TYPE    type);
FUNC_EXTERN DB_STATUS
psl_hcheck(
	PSS_SESBLK	*cb,
	PST_QNODE	*having,
	PST_QNODE	*group,
	PST_J_MASK	*fromlist);
FUNC_EXTERN DB_STATUS
psl_hcheckvar(
	PST_QNODE	*having,
	PST_QNODE	*group,
	PST_J_MASK	*fromlist);
FUNC_EXTERN DB_STATUS
psl_curval(
	PSS_SESBLK  *cb,
	PSQ_CB	    *psq_cb,
	PSC_CURBLK  *cursor,
	PST_QNODE   **tree);
FUNC_EXTERN DB_STATUS
psl_shareable(
	PSQ_CB	   		*psq_cb,
	PSS_SESBLK 		*cb,
	bool	    		*shareable,
	DB_SHR_RPTQRY_INFO	**qry_info);
FUNC_EXTERN VOID
psl_backpatch(
	PST_STATEMENT   *stmt,
	PST_STATEMENT   *patch);
FUNC_EXTERN bool
psl_agfcn(
	PST_QNODE *node);
FUNC_EXTERN VOID
psl_up(
	PSS_SESBLK		*cb,
	PSS_USRRANGE		*rngtab,
	register PST_QTREE	*hdr,
	bool			*flag,
	i4			*reason);
FUNC_EXTERN DB_STATUS
psl_crsopen(
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb,
	PSC_CURBLK	*crblk,
	PST_QTREE	*tree,
	bool		for_rdonly,
	PST_QNODE	*updcollst,
	bool		nonupdt,
	PST_J_ID	num_joins,
	bool		dynqp_comp);
FUNC_EXTERN DB_STATUS
psl_do_insert(
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb,
	PST_QNODE	*root_node,
	bool		*shareable_qp,
	PST_QTREE	**tree,
	i4		isdbp,
	PST_J_ID	num_joins,
	PSS_Q_XLATE	*xlated_qry,
	DB_SHR_RPTQRY_INFO	**qry_info,
	PST_QNODE	*sort_list,
	PST_QNODE	*first_n,
	PST_QNODE	*offset_n);
FUNC_EXTERN DB_STATUS
psl_p_tlist(
	PST_QNODE	**tlist,
	PSS_YYVARS	*yyvarsp,
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb);

typedef struct _PSL_P_TELEM_LINK {
	struct _PSL_P_TELEM_LINK *link;
	PST_QNODE       *rt_node;
} PSL_P_TELEM_LINK;
typedef struct _PSL_P_TELEM_CTX {
	PSS_SESBLK	*cb;
	PSQ_CB		*psq_cb;
	PSS_YYVARS	*yyvarsp;
	PSS_RNGTAB      **rngtable;
	PST_QNODE	*resolved_fwdvar;
	PST_QNODE	*unresolved_fwdvar;
	PSL_P_TELEM_LINK*link;
	i4		qualdepth;
	i4		xform_avg;
} PSL_P_TELEM_CTX;
FUNC_EXTERN DB_STATUS
psl_ss_flatten(
	PST_QNODE	**nodep,
	PSS_YYVARS	*yyvarsp,
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_p_telem(
	PSL_P_TELEM_CTX *ctx,
	PST_QNODE	**node,
	PST_QNODE	*father);
FUNC_EXTERN VOID
psl_set_jrel(
	PST_J_MASK		*inner_rels,
	PST_J_MASK		*outer_rels,
	PST_J_ID		join_id,
	register PSS_RNGTAB	**rng_vars,
        DB_JNTYPE               join_type);
FUNC_EXTERN DB_STATUS
psl_check_key(
	PSS_SESBLK	    *cb,
	DB_ERROR	    *err_blk,
	DB_DT_ID	    att_type);
FUNC_EXTERN VOID
psl_syn_info_msg(
	PSS_SESBLK	*sess_cb,
	PSS_RNGTAB	*rngvar,
	PSS_OBJ_NAME	*obj_spec,
	i4		owned_by,
	i4		qry_len,
	char		*qry,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_comment_col(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_COL_REF	*col_ref);
FUNC_EXTERN VOID
psl_insert_into_agg_list(
	PST_QNODE	*var_node,
	YYAGG_NODE_PTR	*agglist_elem,
	YYAGG_NODE_PTR	**agg_list_stack,
	i4		cur_scope,
	PST_J_MASK	*rel_list);
FUNC_EXTERN DB_STATUS
psl_for_cond(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_DBPINFO	*dbpinfo,
	PST_QNODE	**condition);
FUNC_EXTERN DB_STATUS
psl_ifkwd(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_DBPINFO	*dbpinfo,
	PST_STATEMENT	**if_stmt_node);
FUNC_EXTERN VOID
psl_thenkwd(
	PSS_DBPINFO     *dbpinfo);
FUNC_EXTERN VOID
psl_elsekwd(
	PSS_DBPINFO     *dbpinfo);
FUNC_EXTERN VOID
psl_ifstmt(
	PST_STATEMENT	    *if_node,
	PST_QNODE	    *condition,
	PST_STATEMENT	    *if_action,
	PST_STATEMENT	    *else_action,
	PSS_DBPINFO	    *dbpinfo);
FUNC_EXTERN DB_STATUS
psl_repeat_qry_id(
	PSQ_CB			*psq_cb,
	PSS_SESBLK		*sess_cb,
	DB_CURSOR_ID		*fe_id,
	PST_QTREE		*header,
	DB_SHR_RPTQRY_INFO	*qry_info);
FUNC_EXTERN DB_STATUS
psl_qeucb(
	PSS_SESBLK	*sess_cb,
	i4		operation,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_init_dbp_stmt(
	PSS_SESBLK	*sess_cb,
	PSS_DBPINFO	*dbpinfo,
	i4		qmode,
	i4		type,
	PST_STATEMENT	**stmt,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_cdbp_typedesc(
	PSS_SESBLK	*sess_cb,
	PSS_DBPINFO	*dbpinfo,
	char		*type_name,
	i4		num_len_prec_vals,
	i4		*len_prec,
	i4		null_def,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
psl_send_setqry(
	PSS_SESBLK	*sess_cb,
	char		*str_2_send,
	PSQ_CB		*psq_cb);
FUNC_EXTERN i4
psl_sscan(
	register PSS_SESBLK *pss_cb,
	register PSQ_CB	    *psq_cb);
FUNC_EXTERN VOID
psl_yinit(
	PSS_SESBLK	*sess_cb);
FUNC_EXTERN VOID
psl_yerror(
	i4                errtype,
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb);

FUNC_EXTERN VOID
psl_yerr_buf(
	PSS_SESBLK	*sess_cb,
	char		*buf,
	i4		buf_len);

FUNC_EXTERN VOID
psl_sx_error(
	i4	    error,
	PSS_SESBLK  *sess_cb,
	PSQ_CB	    *psq_cb);
FUNC_EXTERN DB_STATUS
psl_cdbp_build_setrng(
		      PSS_SESBLK    *sess_cb,
		      PSQ_CB	    *psq_cb,
		      PSS_DBPINFO   *dbpinfo, 
		      PSS_RNGTAB    **dbprngp);
FUNC_EXTERN DB_STATUS
psl_fill_proc_params(
		     PSS_SESBLK *sess_cb, 
		     i4	num_params,
		     QUEUE	*parmq,
		     i4 num_rescols,
		     QUEUE	*rescolq,
		     i4	set_input_proc,
		     DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_check_defaults(
		   PSS_SESBLK	*sess_cb,
		   PSS_RNGTAB	*resrng,
		   PST_QNODE 	*root_node,
		   i4		insert_into_view,
		   DB_TAB_NAME	*view_name,
		   DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_make_default_node(
		      PSS_SESBLK    *sess_cb,
		      PSF_MSTREAM   *mstream,
		      PSS_RNGTAB    *rngvar,
		      i2	    attno,
		      PST_QNODE	    **newnode,
		      DB_ERROR	    *err_blk);

FUNC_EXTERN DB_STATUS
psl_mk_const_similar(
			PSS_SESBLK	*sess_cb,
			PSF_MSTREAM	*mstream,
			DB_DATA_VALUE	*templ,   
			PST_QNODE	**cst,
			DB_ERROR	*err_blk,
			bool		*handled);
/* Partition definition utilities in pslpart.c */
FUNC_EXTERN DB_STATUS psl_partdef_new_dim(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp,
		i4		distrule);

FUNC_EXTERN DB_STATUS psl_partdef_nonval(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp,
		i4		nparts);

FUNC_EXTERN DB_STATUS psl_partdef_oncol(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp,
		char		*colname);

FUNC_EXTERN DB_STATUS psl_partdef_partlist(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp);

FUNC_EXTERN DB_STATUS psl_partdef_pname(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp,
		char		*partname,
		bool		allow_ii);

FUNC_EXTERN DB_STATUS psl_partdef_start(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp);

FUNC_EXTERN DB_STATUS psl_partdef_value(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp,
		i4		sign_flag,
		DB_DATA_VALUE	*value);

FUNC_EXTERN DB_STATUS psl_partdef_value_check(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp);

FUNC_EXTERN DB_STATUS ppd_qsfmalloc(
		PSQ_CB		*psq_cb,
		i4		psize,
		void		*pptr);

FUNC_EXTERN DB_STATUS
psq_alter(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psq_bgn_session(
	register PSQ_CB     *psq_cb,
	register PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psq_clscurs(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psq_crdump(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN u_i4
psq_crhsh(
	DB_CURSOR_ID       *cursid);
FUNC_EXTERN DB_STATUS
psq_crfind(
	PSS_SESBLK         *sess_cb,
	DB_CURSOR_ID	   *cursor_id,
	PSC_CURBLK         **cursor,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_crffind(
	PSS_SESBLK         *sess_cb,
	DB_CURSOR_ID	   *cursor_id,
	PSC_CURBLK         **cursor,
	DB_ERROR	   *err_blk);
FUNC_EXTERN u_i4
psq_clhsh(
	i4                tabsize,
	DB_ATT_NAME        *name);
FUNC_EXTERN DB_STATUS
psq_clent(
	i4		   colno,
	DB_ATT_NAME	   *colname,
	DB_DT_ID	   type,
	i4		   length,
	i2		   precision,
	PSC_CURBLK	   *cursor,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk);
FUNC_EXTERN PSC_RESCOL *
psq_ccol(
	PSC_CURBLK         *curblk,
	DB_ATT_NAME        *colname);
FUNC_EXTERN VOID
psq_crent(
	PSC_CURBLK         *curblk,
	PSS_CURSTAB        *curtab);
FUNC_EXTERN DB_STATUS
psq_crcreate(
	PSS_SESBLK         *sess_cb,
	DB_CURSOR_ID	   *cursid,
	i4		   qmode,
	PSC_CURBLK	   **curblk,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_victim(
	PSS_SESBLK         *sess_cb,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_delcursor(
	PSC_CURBLK         *curblk,
	PSS_CURSTAB	   *curtab,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_cltab(
	PSC_CURBLK         *cursor,
	i4		   numcols,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_crclose(
	PSC_CURBLK         *cursor,
	PSS_CURSTAB	   *curstab,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk);
FUNC_EXTERN i4
psq_cvtdow(
	char               *sdow);
FUNC_EXTERN DB_STATUS
psq_dlcsrrow(
	PSQ_CB		*psq_cb,
	PSS_SESBLK	*sess_cb);
FUNC_EXTERN DB_STATUS
psq_delrng(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psq_clrcache(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psq_end_session(
	register PSQ_CB    *psq_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psq_killses(
	PSS_SESBLK         *sess_cb,
	i4		   force,
	DB_ERROR   	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_month(
	char               *monthname,
	i4		   *monthnum,
	DB_ERROR	   *err_blk);
FUNC_EXTERN i4
psq_monsize(
	i4     month,
	i4	year);
FUNC_EXTERN DB_STATUS
psq_headdmp(
	PSQ_CBHEAD         *cb_head);
FUNC_EXTERN DB_STATUS
psq_siddmp(
	CS_SID             *sessid);
FUNC_EXTERN DB_STATUS
psq_lngdmp(
	DB_LANG            qlang);
FUNC_EXTERN DB_STATUS
psq_decdmp(
	DB_DECIMAL         *decimal);
FUNC_EXTERN DB_STATUS
psq_dstdmp(
	DB_DISTRIB         distrib);
FUNC_EXTERN DB_STATUS
psq_booldmp(
	i4                boolval);
FUNC_EXTERN DB_STATUS
psq_modedmp(
	i2	mode);
FUNC_EXTERN DB_STATUS
psq_tbiddmp(
	DB_TAB_ID          *tabid);
FUNC_EXTERN DB_STATUS
psq_ciddmp(
	DB_CURSOR_ID	*cursid);
FUNC_EXTERN DB_STATUS
psq_errdmp(
	DB_ERROR         *errblk);
FUNC_EXTERN DB_STATUS
psq_dtdump(
	DB_DT_ID           dt_id);
FUNC_EXTERN DB_STATUS
psq_tmdmp(
	DB_TAB_TIMESTAMP   *timestamp);
FUNC_EXTERN DB_STATUS
psq_jrel_dmp(
	PST_J_MASK	*inner_rel,
	PST_J_MASK	*outer_rel);
FUNC_EXTERN DB_STATUS
psq_upddmp(
	i4                updtmode);
FUNC_EXTERN DB_STATUS
psq_str_dmp(
        u_char      *str,
        i4          len);
FUNC_EXTERN DB_STATUS
psq_parseqry(
	register PSQ_CB     *psq_cb,
	register PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psq_cbinit(
	PSQ_CB     *psq_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psq_cbreturn(
	PSQ_CB     *psq_cb,
	PSS_SESBLK *sess_cb,
	DB_STATUS   ret_val);
FUNC_EXTERN DB_STATUS
psq_prmdump(
	register PSQ_CB     *psq_cb);
FUNC_EXTERN DB_STATUS
psq_recreate(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *sess_cb);
FUNC_EXTERN DB_STATUS
psq_open_rep_cursor(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *sess_cb);
FUNC_EXTERN DB_STATUS
psq_dbpreg(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *sess_cb);
FUNC_EXTERN DB_STATUS
psq_sesdump(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN VOID
psq_rngdmp(
	PSS_RNGTAB         *rv);
FUNC_EXTERN DB_STATUS
psq_shutdown(
	PSQ_CB             *psq_cb);
FUNC_EXTERN DB_STATUS
psq_srvdump(
	register PSQ_CB    *psq_cb);
FUNC_EXTERN DB_STATUS
psq_startup(
	PSQ_CB             *psq_cb);
FUNC_EXTERN DB_STATUS
psq_topen(
	PTR                *header,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_tadd(
	PTR                header,
	u_char		   *piece,
	i4		   size,
	PTR		   *result,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_rptqry_text(
	PTR                header,
	u_char		   *piece,
	i4		   size,
	PTR		   *result,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_tclose(
	PTR		   header,
	DB_ERROR           *err_blk);
FUNC_EXTERN DB_STATUS
psq_tsubs(
	PTR                header,
	PTR		   piece,
	u_char		   *newtext,
	i4		   size,
	PTR		   *newpiece,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_tout(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE	   *rngtab,
	PTR                header,
	PSF_MSTREAM	   *mstream,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_tmulti_out(
	PSS_SESBLK	*sess_cb,
	PSS_USRRANGE    *rngtab,
	PTR             header,
	PSF_MSTREAM	*mstream,
	DB_TEXT_STRING	**txtp,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psq_store_text(
	PSS_SESBLK	    *sess_cb,
	PSS_USRRANGE	    *rngtab,
	PTR		    header,
	PSF_MSTREAM	    *mstream,
	PTR		    *result,
	bool		    return_db_text_string,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
psq_1rptqry_out(
	PSS_SESBLK	*sess_cb,
	PTR             header,
	PSF_MSTREAM	*mstream,
	i4		*txt_len,
	u_char		**txt,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psq_tqrylen(
	PTR                header,
	i4		   *length);
FUNC_EXTERN DB_STATUS
psq_tcnvt(
	PSS_SESBLK	   *sess_cb,
	PTR                header,
	DB_DATA_VALUE	   *dbval,
	PTR		   *result,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psq_tinsert(
	PTR                header,
	u_char		   *piece,
	i4		   size,
	PTR		   *result,
	PTR		   oldpiece,
	DB_ERROR	   *err_blk);
FUNC_EXTERN PTR
psq_tbacktrack(
	PTR         piece,
	i4	    n);
FUNC_EXTERN PTR
psq_tbackfromlast(
	PTR         piece,
	i4	    n);
FUNC_EXTERN void
psq_tdelete(
	PTR	    header,
	PTR         piece);
FUNC_EXTERN DB_STATUS
psq_version(
	PSQ_CB             *psq_cb);
FUNC_EXTERN DB_STATUS
psq_x_add(
	PSS_SESBLK	    *sess_cb,
	char		    *str_2_store,
	PSF_MSTREAM	    *mem_stream,
	i4		     buf_size,
	PSS_Q_PACKET_LIST   *pkt_list,
	i4		     str_len,
	char		    *open_delim,
	char		    *close_delim,
	char		    *separator,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
psq_x_new(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	    *mem_stream,
	i4		     buf_size,
	PSS_Q_PACKET_LIST   *pkt_list,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
psq_x_backup(
	PSS_Q_PACKET_LIST   *packet_list,
	char		    *str);
FUNC_EXTERN bool
psq_same_site(
	DD_LDB_DESC	    *ldb_desc1,
	DD_LDB_DESC	    *ldb_desc2);
FUNC_EXTERN DB_STATUS
psq_prt_tabname(
	PSS_SESBLK	    *sess_cb,
	PSS_Q_XLATE	    *xlated_qry,
	PSF_MSTREAM	    *mem_stream,
	PSS_RNGTAB	    *rng_var,
	i4		    qmode,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
pst_adparm(
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb,
	PSF_MSTREAM        *stream,
	i2		   parmno,
	char		   *format,
	PST_QNODE	   **newnode,
	i4		   *highparm);
FUNC_EXTERN DB_STATUS
pst_2adparm(
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb,
	PSF_MSTREAM        *stream,
	i2		   parmno,
	DB_DATA_VALUE	   *format,
	PST_QNODE	   **newnode,
	i4		   *highparm);
FUNC_EXTERN DB_STATUS
pst_adresdom(
	char               *attname,
	PST_QNODE	   *left,
	PST_QNODE	   *right,
	PSS_SESBLK	   *cb,
	PSQ_CB		   *psq_cb,
	PST_QNODE	   **newnode);
FUNC_EXTERN DB_STATUS
pst_rsdm_dt_resolve(
	PST_QNODE	*right,
	DMT_ATT_ENTRY	*coldesc,
	PSS_SESBLK	*cb,
	PSQ_CB		*psq_cb);
FUNC_EXTERN DB_STATUS
pst_clrrng(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE       *rngtab,
	DB_ERROR           *err_blk);
FUNC_EXTERN DMT_ATT_ENTRY *
pst_coldesc(
	PSS_RNGTAB         *rngentry,
	DB_ATT_NAME        *colname);
FUNC_EXTERN DB_STATUS
pst_prepare(
	PSQ_CB             *psq_cb,
	PSS_SESBLK         *sess_cb,
	char		   *sname,
	bool		   nonupdt,
	bool		    repeat,
	PTR		    stmt_offset,
	PST_QNODE	   *updcollst);
FUNC_EXTERN DB_STATUS
pst_execute(
	PSQ_CB             *psq_cb,
	PSS_SESBLK         *sess_cb,
	char		   *sname,
	PST_PROCEDURE	   **pnode,
	i4		   *maxparm,
	PTR		   *parmlist,
	bool		   *nonupdt,
	bool		   *qpcomp,
	PST_QNODE	   **updcollst,
	bool		    rdonly);
FUNC_EXTERN DB_STATUS
pst_prmsub(
	PSQ_CB		    *psq_cb,
	PSS_SESBLK	    *sess_cb,
	PST_QNODE	    **node,
	DB_DATA_VALUE	    *plist[],
	bool		    repeat_dyn);
FUNC_EXTERN DB_STATUS
pst_describe(
	PSQ_CB		    *psq_cb,
	PSS_SESBLK	    *sess_cb,
	char		    *sname);
FUNC_EXTERN DB_STATUS
pst_cpdata(
	PSS_SESBLK 	    *sess_cb, 
	PSQ_CB		    *psq_cb, 
	PST_QNODE 	    *tree,
	bool		    use_qsf);
FUNC_EXTERN DB_STATUS
pst_commit_dsql(
	PSQ_CB		    *psq_cb,
	PSS_SESBLK	    *sess_cb);
FUNC_EXTERN DB_STATUS
pst_header(
	PSS_SESBLK      *sess_cb,
	PSQ_CB		*psq_cb,
	i4		forupdate,
	PST_QNODE	*sortlist,
	PST_QNODE	*qtree,
	PST_QTREE	**tree,
	PST_PROCEDURE   **pnode,
	i4		mask,
	PSS_Q_XLATE	*xlated_qry);
FUNC_EXTERN DB_STATUS
pst_modhdr(
	PSS_SESBLK         *sess_cb,
	PSQ_CB		   *psq_cb,
	i4		   forupdate,
	PST_QTREE	   *header);
FUNC_EXTERN bool
pst_cdb_cat(
	PSS_RNGTAB	*rng_tab,
	PSQ_CB		*psq_cb);
FUNC_EXTERN i4
pst_hshname(
	DB_ATT_NAME        *colname);
FUNC_EXTERN DB_STATUS
pst_node(
	PSS_SESBLK	   *cb,
	PSF_MSTREAM        *stream,
	PST_QNODE	   *left,
	PST_QNODE	   *right,
	i4		   type,
	char		   *value,
	i4		   vallen,
	DB_DT_ID	   datatype,
	i2		   dataprec,
	i4		   datalen,
	DB_ANYTYPE	   *datavalue,
	PST_QNODE	   **newnode,
	DB_ERROR	   *err_blk,
	i4		    flags);
FUNC_EXTERN bool
pst_is_const_bool(
	PST_QNODE	   *node,
	bool		   *bval);
FUNC_EXTERN void pst_not_bool(
	PST_QNODE	   *node);

FUNC_EXTERN VOID
pst_negate(
	register DB_DATA_VALUE *dataval);
FUNC_EXTERN VOID
pst_map(
	PST_QNODE   *tree,
	PST_J_MASK  *map);
FUNC_EXTERN DB_STATUS
pst_node_size(
	PST_SYMBOL	    *sym,
	i4		    *symsize,
	i4		    *datasize,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
pst_treedup(
	PSS_SESBLK	   *sess_cb,
	PSS_DUPRB	   *dup_rb);
FUNC_EXTERN DB_STATUS
pst_dmpres(
	PST_RESTAB  *restab);
FUNC_EXTERN STATUS
pst_scctrace(
	PTR	    arg1,
	i4	    msg_length,
	char        *msg_buffer);

FUNC_EXTERN VOID	pst_display();

/*
FUNC_EXTERN VOID
pst_display(
	char               *format,
	i4		    p1,
	i4		    p2,
	i4		    p3,
	i4		    p4,
	i4		    p5,
	i4		    p6,
	i4		    p7,
	i4		    p8,
	i4		    p9,
	i4		    p10);
*/
FUNC_EXTERN VOID
pst_dbpdump(
	PST_PROCEDURE	*pnode,
	i4		whichlink);
FUNC_EXTERN DB_STATUS
pst_rginit(
	PSS_USRRANGE       *rangetab);
FUNC_EXTERN DB_STATUS
pst_rglook(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE       *rngtable,
	i4		   scope,
	char               *name,
	bool		   getdesc,
	PSS_RNGTAB	   **result,
	i4		   query_mode,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_rgent(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE	   *rngtable,
	i4		    scope,
	char               *varname,
	i4		   showtype,
	DB_TAB_NAME	   *tabname,
	DB_TAB_OWN	   *tabown,
	DB_TAB_ID	   *tabid,
	bool               tabonly,
	PSS_RNGTAB	   **rngvar,
	i4		   query_mode,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_rgdel(
	PSS_SESBLK	   *sess_cb,
	DB_TAB_NAME        *tabname,
	PSS_USRRANGE       *rngtable,
	DB_ERROR           *err_blk);
FUNC_EXTERN DB_STATUS
pst_rgrent(
	PSS_SESBLK	   *sess_cb,
	PSS_RNGTAB         *rngvar,
	DB_TAB_ID	   *tabid,
	i4		   query_mode,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_rgcopy(
	PSS_USRRANGE	    *fromtab,
	PSS_USRRANGE	    *totab,
	PSS_RNGTAB	    **resrng,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
pst_snscope(
	PSS_USRRANGE       *rngtable,
	PST_J_MASK	   *from_list,
	i4		    offset);
FUNC_EXTERN DB_STATUS
pst_rescope(
	PSS_USRRANGE       *rngtable,
	PST_J_MASK	   *from_list,
	i4		    offset);
FUNC_EXTERN DB_STATUS
pst_sent(
	PSS_SESBLK	   *sess_cb,
	PSS_USRRANGE	   *rngtable,
	i4		    scope,
	char               *varname,
	i4		   showtype,
	DB_TAB_NAME	   *tabname,
	DB_TAB_OWN	   *tabown,
	DB_TAB_ID	   *tabid,
	bool               tabonly,
	PSS_RNGTAB	   **rngvar,
	i4		   query_mode,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_sdent(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *varname,
	PSS_SESBLK	   *cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	i4		    type,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_slook(
	PSS_USRRANGE       *rngtable,
	PSS_SESBLK	   *cb,
	DB_OWN_NAME        *schema_name,
	char               *name,
	PSS_RNGTAB	   **result,
	DB_ERROR	   *err_blk,
	bool		   scope_flag);
FUNC_EXTERN DB_STATUS
pst_stproc(
	PSS_USRRANGE       *rngtable,
	i4		    scope,
	char		   *corrname,
	i4		    dbpid,
	PSS_SESBLK	   *cb,
	PSS_RNGTAB	   **rngvar,
	PST_QNODE	   *root,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_rgfind(
	PSS_USRRANGE       *rngtable,
	PSS_RNGTAB	   **result,
	i4		   vno,
	DB_ERROR	   *err_blk);

FUNC_EXTERN DB_STATUS pst_rng_unfix(
	PSS_SESBLK *sess_cb,
	PSS_RNGTAB *rng_ptr,
        DB_ERROR *errblk);

FUNC_EXTERN DB_STATUS
pst_rserror(
	i4		lineno,
	PST_QNODE       *opnode,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
pst_union_resolve(
	PSS_SESBLK  *sess_cb,
	PST_QNODE   *rootnode,
	DB_ERROR    *error);
FUNC_EXTERN DB_STATUS
pst_parm_resolve(
	PSS_SESBLK  *sess_cb,
	PSQ_CB	    *psq_cb,
	PST_QNODE   *resdom);
FUNC_EXTERN DB_STATUS
pst_resolve_table(
	DB_TEXT_STRING	*obj_owner,
	DB_TEXT_STRING  *obj_name,
	DB_TEXT_STRING	*out);
FUNC_EXTERN bool
pst_convlit(
	PSS_SESBLK  	*sess_cb,
	PSF_MSTREAM     *stream,
	DB_DATA_VALUE	*targdv,
	PST_QNODE	*srcenode);
FUNC_EXTERN DB_STATUS
pst_ruledup(
	PSS_SESBLK  	*sess_cb,
	PSF_MSTREAM     *mstream,
	i4		filter,
	char		*col_mask,
	PST_STATEMENT	*stmt_tree,
	PST_STATEMENT	**stmt_dup,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
pst_showtab(
	PSS_SESBLK	    *sess_cb,
	i4		    showtype,
	DB_TAB_NAME	    *tabname,
	DB_TAB_OWN	    *tabown,
	DB_TAB_ID	    *tabid,
	bool		    tabonly,
	PSS_RNGTAB	    *rngentry,
	i4		    query_mode,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
pst_dbpshow(
	PSS_SESBLK	*sess_cb,
	DB_DBP_NAME	*dbpname,
	PSS_DBPINFO	**dbpinfop,
	DB_OWN_NAME	*dbp_owner,
	DB_TAB_ID	*dbp_id,
	i4		dbp_mask,
	PSQ_CB		*psq_cb,
	i4		*ret_flags);
FUNC_EXTERN DB_STATUS
pst_add_1indepobj(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*obj_id,
	i4		obj_type,
	DB_DBP_NAME     *dbpname,
	PSQ_OBJ		**obj_list,
	PSF_MSTREAM	*mstream,
	DB_ERROR	*err_blk);
FUNC_EXTERN VOID
pst_rdfcb_init(
	RDF_CB	    *rdf_cb,
	PSS_SESBLK  *sess_cb);
FUNC_EXTERN DB_STATUS
pst_ldbtab_desc(
	PSS_SESBLK		*sess_cb,
	QED_DDL_INFO		*ddl_info,
	PSF_MSTREAM		*mem_stream,
	i4			flag,
	DB_ERROR		*err_blk);
FUNC_EXTERN DB_STATUS
pst_sdir(
	PST_QNODE          *node,
	char		   *direction,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_expsort(
        PSS_SESBLK          *sess_cb,
        PSF_MSTREAM         *stream,
        PST_QNODE           *tlist,
        PST_QNODE           **sort_list,
        char                *expname,
        PST_QNODE           **newnode,
        PSQ_CB              *psq_cb);
FUNC_EXTERN DB_STATUS
pst_sqlsort(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	    *stream,
	PST_QNODE	    *tlist,
	PST_QNODE	    *intolist,
	PST_QNODE	    **sort_list,
	PST_QNODE	    *expnode,
	PST_QNODE	    **newnode,
	PSQ_CB		    *psq_cb,
	PSS_RNGTAB	    **rng_vars
);
FUNC_EXTERN DB_STATUS
pst_varsort(
	PSS_SESBLK		*sess_cb,
	PSF_MSTREAM		*stream,
	PST_QNODE		*tlist,
	PST_QNODE		**sort_list,
	register PSS_RNGTAB	*rngvar,
	char			*colname,
	PST_QNODE		**newnode,
	PSQ_CB			*psq_cb);
FUNC_EXTERN DB_STATUS
pst_numsort(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	    *stream,
	PST_QNODE	    *tlist,
	PST_QNODE	    **sort_list,
	i2		    *expnum,
	PST_QNODE	    **newnode,
	PSQ_CB		    *psq_cb);
FUNC_EXTERN PST_QNODE *
pst_tlprpnd(
	PST_QNODE          *from,
	PST_QNODE          *onto);
FUNC_EXTERN DB_STATUS
pst_trdup(
	PTR                stream,
	PST_QNODE	   *tree,
	PST_QNODE	   **dup,
	SIZE_TYPE	   *memleft,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
pst_trfix(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM	   *mstream,
	PST_QNODE          *qtree,
	DB_ERROR	   *err_blk);
FUNC_EXTERN VOID
pst_windup(
	PST_QNODE          *reslist);
FUNC_EXTERN DB_STATUS
pst_crt_tbl_stmt(
		 PSS_SESBLK *sess_cb, 
		 i4  qmode,
		 i4 operation, 
		 DB_ERROR *err_blk);
FUNC_EXTERN DB_STATUS
pst_rename_stmt(
		 PSS_SESBLK *sess_cb, 
		 i4  qmode,
		 i4 operation, 
		 DB_ERROR *err_blk);
FUNC_EXTERN DB_STATUS
pst_ddl_header(
	       PSS_SESBLK    *sess_cb,
	       PST_STATEMENT *stmt,
	       DB_ERROR      *err_blk);
FUNC_EXTERN DB_STATUS
pst_attach_to_tree(
		   PSS_SESBLK    *sess_cb, 
		   PST_STATEMENT *stmt_list, 
		   DB_ERROR      *err_blk);
FUNC_EXTERN DB_STATUS
pst_crt_schema(PSS_SESBLK *sess_cb, 
	       i4	  qmode,
	       DB_ERROR   *err_blk);
FUNC_EXTERN DB_STATUS
psy_alarm(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_aplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_asequence(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_caplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_csequence(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_aaplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_kaplid(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_dsequence(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_gsequence(
	PSS_SESBLK	   *sess_cb,
	DB_OWN_NAME	   *seq_own,
	DB_NAME		   *seq_name,
	i4		   seq_mask,
	PSS_SEQINFO	   *seq_info,
	DB_IISEQUENCE	   *seqp,
	i4		   *ret_flags,
	i4		   *privs,
	i4		   qmode,
	i4		   grant_all,
	DB_ERROR	   *err_code);
FUNC_EXTERN DB_STATUS
psy_seqperm(
	RDF_CB		*rdf_cb,
	i4		*privs,
	PSS_SESBLK      *sess_cb,
	PSS_SEQINFO	*seq_info,
	i4		qmode,
	i4		grant_all,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_audit(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_comment(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psy_dbpriv(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_rolepriv(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_ckdbpr(
	PSQ_CB		    *psq_cb,
	u_i4	    dbprmask);
FUNC_EXTERN DB_STATUS
psy_cproc(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psy_kproc(
	PSY_CB		*psy_cb,
	PSS_SESBLK	*sess_cb);
FUNC_EXTERN DB_STATUS
psy_gproc(
	PSQ_CB		*psq_cb,
	PSS_SESBLK	*sess_cb,
	QSF_RCB		*qsf_rb,
	RDF_CB		*rdf_cb,
	DB_OWN_NAME	**alt_user,
	DB_OWN_NAME	*dbp_owner,
	i4		gproc_mask,
	i4		*ret_flags);
FUNC_EXTERN DB_STATUS
psy_dgrant(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN bool
psy_permit_ok(
	i4		obj_mask,
	PSS_SESBLK	*cb,
	DB_OWN_NAME	*obj_owner);
FUNC_EXTERN VOID
psy_prvmap_to_str(
	i4	    privs,
	char	    *priv_str,
	DB_LANG	    lang);
FUNC_EXTERN VOID
psy_init_rel_map(
	PSQ_REL_MAP	*map);
FUNC_EXTERN VOID
psy_attmap_to_str(
	DMT_ATT_ENTRY	    **attr_descr,
	PSY_ATTMAP	    *attr_map,
	register i4	    *offset,
	char		    *str,
	i4		    str_len);
FUNC_EXTERN DB_STATUS
psy_dbp_status(
	PSY_TBL		*dbp_descr,
	PSS_SESBLK	*sess_cb,
	PSF_QUEUE	*grant_dbprocs,
	i4		qmode,
	i4		*dbp_mask,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_dbp_priv_check(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	DB_DBP_NAME 	*dbpname,
	DB_TAB_ID	*dbpid,
	bool            *non_existant_dbp,
	bool		*missing_obj,
	i4		*dbp_mask,
	i4		*privs,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_tbl_grant_check(
	PSS_SESBLK	*sess_cb,
	i4		qmode,
	DB_TAB_ID	*grant_obj_id,
	i4		*tbl_wide_privs,
	PSY_COL_PRIVS   *col_specific_privs,
	DB_TAB_ID	*indep_id,
	i4		*indep_tbl_wide_privs,
	PSY_COL_PRIVS   *indep_col_specific_privs,
	i4		psy_flags,
	bool		*insuf_privs,
	bool		*quel_view,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_check_privs(
	PSS_RNGTAB	    *rngvar,
	i4		    *privs_to_find,
	i4		    indep_tbl_wide_privs,
	PSY_COL_PRIVS	    *indep_col_specific_privs,
	PSS_SESBLK	    *sess_cb,
	bool		    ps131,
	DB_TAB_ID	    *grant_obj_id,
	i4		    flags,
	i4                  *fully_satisfied,
	DB_ERROR	    *err_blk);
FUNC_EXTERN DB_STATUS
psy_dpermit(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_sqlview(
	PSS_RNGTAB	    *rngvar,
	PSS_SESBLK	    *sess_cb,
	DB_ERROR	    *err_blk,
	i4		    *issql);
FUNC_EXTERN DB_STATUS
psy_devent(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psy_kevent(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_gevent(
	PSS_SESBLK      *sess_cb,
	DB_OWN_NAME	*event_own,
	DB_TAB_NAME	*event_name,
	DB_TAB_ID	*ev_id,
	i4		ev_mask,
	PSS_EVINFO	*ev_info,
	i4		*ret_flags,
	i4		*privs,
	i4		qmode,
	i4		grant_all,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_evperm(
	RDF_CB		*rdf_cb,
	i4		*privs,
	PSS_SESBLK      *sess_cb,
	PSS_EVINFO	*ev_info,
	i4		qmode,
	i4		grant_all,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_group(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_cgroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_agroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_dgroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_kgroup(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_integ(
	PSF_MSTREAM	*mstream,
	PST_QNODE	*root,
	PSS_USRRANGE	*rngtab,
	PSS_RNGTAB	*resvar,
	i4		qmode,
	PSS_SESBLK	*sess_cb,
	PST_QNODE	**result,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_kinteg(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_kpermit(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_basetbl(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*view_id,
	DB_TAB_ID	*tbl_id,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_kregproc(
        PSY_CB          *psy_cb,
        PSS_SESBLK      *sess_cb,
        DB_TAB_NAME     *procname);
FUNC_EXTERN DB_STATUS
psy_loc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_cloc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_aloc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_kloc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_mapvars(
	PST_QNODE   *tree,
	i4	    map[],
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
psy_maxquery(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_protect(
	PSS_SESBLK	    *sess_cb,
	PSS_USRRANGE	    *rngtab,
	PST_QNODE	    *tree,
	i4		    qmode,
	DB_ERROR	    *err_blk,
	PSF_MSTREAM	    *mstream,
	i4	    	    *num_joins,
	PSY_VIEWINFO	    *viewinfo[]);
FUNC_EXTERN DB_STATUS
psy_dbpperm(
	PSS_SESBLK	*sess_cb,
	RDF_CB		*rdf_cb,
	PSQ_CB		*psq_cb,
	i4		*required_privs);
FUNC_EXTERN DB_STATUS
psy_cpyperm(
	PSS_SESBLK  *cb,
	i4	    perms_required,
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
proappl(
	register  DB_PROTECTION *protup,
	bool			*result,
	PSS_SESBLK		*sess_cb,
	DB_ERROR		*err_blk);
FUNC_EXTERN i4
prochk(
	i4			privs,
	i4			*domset,
	register DB_PROTECTION	*protup,
	PSS_SESBLK		*sess_cb);
FUNC_EXTERN VOID
psy_fill_attmap(
	register i4	*map,
	register i4	val);
FUNC_EXTERN DB_STATUS
psy_grantee_ok(
	PSS_SESBLK	*sess_cb,
	DB_PROTECTION	*protup,
	bool		*applies,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_insert_objpriv(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*objid,
	i4		objtype,
	i4		privmap,
	PSF_MSTREAM	*mstream,
	PSQ_OBJPRIV	**priv_list,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_insert_colpriv(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*tabid,
	i4		objtype,
	register i4	*attrmap,
	i4		privmap,
	PSF_MSTREAM	*mstream,
	PSQ_COLPRIV	**priv_list,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_check_objprivs(
	PSS_SESBLK	*sess_cb,
	i4		*required_privs,
	PSQ_OBJPRIV	**priv_descriptor,
	PSQ_OBJPRIV     **priv_list,
	bool		*missing,
	DB_TAB_ID	*id,
	PSF_MSTREAM	*mstream,
	i4		obj_type,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_print(
	PSF_MSTREAM	   *mstream,
	i4		    map[],
	PSY_QTEXT	   **block,
	u_char             *text,
	i4		   length,
	PSS_USRRANGE	   *rngtab,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psy_put(
	PSF_MSTREAM        *mstream,
	register char	   *inbuf,
	register i4	   len,
	PSY_QTEXT	   **block,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psy_txtalloc(
	PSF_MSTREAM        *mstream,
	PSY_QTEXT	   **newblock,
	DB_ERROR	   *err_blk);
FUNC_EXTERN DB_STATUS
psy_qrymod(
	PST_QNODE          *qtree,
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb,
	PST_J_ID	   *num_joins,
	i4		   *resp_mask);
FUNC_EXTERN DB_STATUS
psy_qminit(
	PSS_SESBLK	    *sess_cb,
	PSQ_CB		    *psq_cb,
	PSF_MSTREAM	    *mstream);
FUNC_EXTERN DB_STATUS
psy_revoke(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_v2b_col_xlate(
	DB_TAB_ID	*view_id,
	i4		*view_attrmap,
	DB_TAB_ID	*base_id,
	i4		*base_attrmap);
FUNC_EXTERN DB_STATUS
psy_b2v_col_xlate(
	DB_TAB_ID	*view_id,
	i4		*view_attrmap,
	DB_TAB_ID	*base_id,
	i4		*base_attrmap);
FUNC_EXTERN DB_STATUS
psy_drule(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psy_krule(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_rules(
	PSS_SESBLK      *sess_cb,
	i4		qmode,
	PST_QNODE	*root,
	DB_ERROR        *err_blk);
FUNC_EXTERN DB_STATUS
psy_create_synonym(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psy_drop_synonym(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psy_dschema(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb);
FUNC_EXTERN DB_STATUS
psy_vfind(
	u_i2	    vn,
	PST_QNODE   *vtree,
	PST_QNODE   **result,
	DB_ERROR    *err_blk);
FUNC_EXTERN PST_QNODE *
psy_qscan(
	PST_QNODE   *root,
	i4	    vn,
	u_i2	    an);
FUNC_EXTERN VOID
psy_varset(
	PST_QNODE	*root,
	PST_VRMAP	*bitmap);
FUNC_EXTERN DB_STATUS
psy_subsvars(
	PSS_SESBLK	*cb,
	PST_QNODE	**proot,
	PSS_RNGTAB	*rngvar,
	PST_QNODE	*transtree,
	i4		vmode,
	PST_QNODE	*vqual,
	PSS_RNGTAB	*resvar,
	PST_J_MASK	*from_list,
	i4		qmode,
	DB_CURSOR_ID	*cursid,
	i4		*mask,
	PSS_DUPRB	*dup_rb);
FUNC_EXTERN VOID
psy_vcount(
	PST_QNODE	*tree,
	PST_VRMAP	*bitmap);
FUNC_EXTERN DB_STATUS
psy_user(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_cuser(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_auser(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN DB_STATUS
psy_kuser(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb);
FUNC_EXTERN PST_QNODE *
psy_trmqlnd(
	PST_QNODE *qual);
FUNC_EXTERN DB_STATUS
psy_apql(
	PSS_SESBLK  *cb,
	PSF_MSTREAM *mstream,
	PST_QNODE   *qual,
	PST_QNODE   *root,
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
psy_mrgvar(
	PSS_USRRANGE	*rngtab,
	register i4	a,
	register i4	b,
	PST_QNODE	*root,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psy_mkempty(
	PSS_SESBLK	    *sess_cb,
	PSF_MSTREAM	*mstream,
	DB_DATA_VALUE	*typeandlen,
	PST_QNODE	**newnode,
	DB_ERROR	*err_blk);

FUNC_EXTERN DB_STATUS	psy_error();
/*
FUNC_EXTERN DB_STATUS
psy_error(
	i4            errorno,
	i4		   qmode,
	PSS_RNGTAB	   *rngvar,
	DB_ERROR	   *err_blk,
	i4		   p1,
	PTR		   p2,
	i4		   p3,
	PTR		   p4,
	i4		   p5,
	PTR		   p6);
*/
FUNC_EXTERN DB_STATUS
psy_view(
	PSF_MSTREAM	*mstream,
	PST_QNODE	*root,
	PSS_USRRANGE	*rngtab,
	i4		qmode,
	PSS_SESBLK	*sess_cb,
	DB_ERROR	*err_blk,
	PST_J_ID	*num_joins,
	i4		*resp_mask);
FUNC_EXTERN DB_STATUS
psy_vrscan(
	i4	    with_check,
	PST_QNODE   *root,
	PST_QNODE   *vtree,
	i4	    qmode,
	PSS_RNGTAB  *resvar,
	i4	    *rgno,
	DB_ERROR    *err_blk);
FUNC_EXTERN DB_STATUS
psy_translate_nmv_map(
	PSS_RNGTAB	*parent,
	char		*parent_attrmap,
	i4		rel_rgno,
	char		*rel_attrmap,
	VOID		(*treewalker)(),
	DB_ERROR	*err_blk);
					/*
					** build a map of attrs to which a
					** privilege will or will not apply
					*/
FUNC_EXTERN VOID
psy_prv_att_map(			
	char		*attmap,
	bool		exclude_cols,
	i4		*attr_nums,
	i4		num_cols);
					/*
					** walk a tree and build a map of
					** attributes of a given relation found
					** in the tree
					*/
FUNC_EXTERN VOID
psy_att_map(
	PST_QNODE	*t,
	i4		rgno,
	char		*attrmap);
	
FUNC_EXTERN bool
psy_view_is_updatable(
	PST_QTREE	*tree_header,
	i4		qmode,
	i4		*reason);

FUNC_EXTERN void
psy_rl_coalesce(
		  PST_STATEMENT **list1p,
		  PST_STATEMENT *list2);

FUNC_EXTERN DB_STATUS
psl_cs01s_create_schema(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
        char            *authid);
FUNC_EXTERN DB_STATUS
psl_reorder_schema(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
        PST_PROCEDURE   *prnode);
FUNC_EXTERN DB_STATUS
psl_cs02s_create_schema_key(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb);
FUNC_EXTERN DB_STATUS
psl_alloc_exec_imm(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
        char            *stmtstart,
        i4              stmtlen,
        PST_OBJDEP      *deplist,
        i4              qmode,
        i4              basemode);
FUNC_EXTERN DB_STATUS
psl_cs03s_create_table(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
        char            *stmtstart,
        PST_OBJDEP      *deplist,
        PSS_CONS        *cons_list);
FUNC_EXTERN DB_STATUS
psl_cs04s_create_view(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
        char            *stmtstart,
        PST_OBJDEP      *deplist);
FUNC_EXTERN DB_STATUS
psl_cs05s_grant(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
        char            *stmtstart,
        PST_OBJDEP      *deplist);
FUNC_EXTERN DB_STATUS
psl_cs06s_obj_spec(
        PSS_SESBLK      *sess_cb,
        PSQ_CB          *psq_cb,
        PST_OBJDEP      **deplistp,
        PSS_OBJ_NAME    *obj_spec,
        u_i4       obj_use);
FUNC_EXTERN DB_STATUS
psl_d_cons(
    PSS_SESBLK	    *sess_cb,
    PSQ_CB	    *psq_cb,
    PSS_RNGTAB	    *rngvar,
    char	    *cons_name,
    bool            drop_cascade);

FUNC_EXTERN i4
psl_find_column_number(
	struct _DMU_CB  *dmu_cb,
	DB_ATT_NAME     *col_name);
FUNC_EXTERN DB_STATUS
psl_must_be_string(
	PSS_SESBLK	*sess_cb,
	DB_DATA_VALUE	*in_val,
	DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psl_norm_id_2_delim_id(
		u_char		**ident,
		i4		*ident_len,
		u_char		*delim_id_buf,
		DB_ERROR	*err_blk);
FUNC_EXTERN DB_STATUS
psq_destr_dbp_qep(
		PSS_SESBLK	*sess_cb,
		PTR		handle,
		DB_ERROR	*err_blk);

FUNC_EXTERN DB_STATUS
psy_ubtid(
	PSS_RNGTAB	    *rngvar,
	PSS_SESBLK	    *sess_cb,
	DB_ERROR	    *err_blk,
	DB_TAB_ID	    *ubt_id);

FUNC_EXTERN DB_STATUS
psl_us1_with_nonkeyword(
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char 	    *word
);
FUNC_EXTERN DB_STATUS
psl_us2_with_nonkw_eq_nmsconst(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *word2
);
FUNC_EXTERN DB_STATUS
psl_us2_with_nonkw_eq_hexconst(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	u_i2	    length,
	char	    *word2
);
FUNC_EXTERN DB_STATUS
psl_us3_usrpriv(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	char	    *word,
	i4	    *privptr
);
FUNC_EXTERN DB_STATUS
psl_us4_usr_priv_or_nonkw(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word,
	i4	    privs
);
FUNC_EXTERN DB_STATUS
psl_us5_usr_priv_def(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	i4	    mode,
	i4	    privs
);
FUNC_EXTERN DB_STATUS
psl_as1_with_nonkeyword(
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char 	    *word
);
FUNC_EXTERN DB_STATUS
psl_as2_with_nonkw_eq_nmsconst(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *word2
);
FUNC_EXTERN DB_STATUS
psl_as3_alter_secaud(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb
);
FUNC_EXTERN DB_STATUS
psl_us12_set_nonkw_roll_nonkw(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *svpt
);
FUNC_EXTERN DB_STATUS
psl_us11_set_nonkw_roll_svpt(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *svpt
);
FUNC_EXTERN DB_STATUS
psl_us10_set_priv(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	i4	    mode,
	i4	    privs
);
FUNC_EXTERN DB_STATUS
psl_us9_set_nonkw_eq_nonkw(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *word2,
	bool	    isnkw
);
FUNC_EXTERN DB_STATUS
psl_us14_set_nonkw_eq_int(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	i4	    word2
);
FUNC_EXTERN DB_STATUS
psl_us15_set_nonkw(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word
);

FUNC_EXTERN DB_STATUS
psy_do_alarm_fail(
	PSS_SESBLK	   *sess_cb,
	PSS_RNGTAB	   *rngvar,
	i4		   qmode,
	DB_ERROR	   *err_blk)
;
FUNC_EXTERN DB_STATUS
psy_calarm(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
;
FUNC_EXTERN DB_STATUS
psy_kalarm(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
;
FUNC_EXTERN DB_STATUS
psy_evraise(
	PSS_SESBLK *sess_cb,
	DB_EVENT_NAME *evname,
	DB_OWN_NAME *evowner,
	char	    *evtext,
	i4	    ev_l_text,
        DB_ERROR    *err_blk
);
FUNC_EXTERN DB_STATUS
psy_evget_by_id(
	PSS_SESBLK *sess_cb,
	DB_TAB_ID  *ev_id,
	DB_IIEVENT *evtuple,
        DB_ERROR    *err_blk
);
FUNC_EXTERN  VOID
psl_grantee_type_to_str(
	i4	grantee_type,
	char	**str,
	i4	*str_len,
	bool	by_word);

FUNC_EXTERN DB_STATUS
psl_al4_drop_alarm(
	PSS_SESBLK  *cb,
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	i4	    object_type
);
/* can't add prototype for psy_secaudit, because not all users
** include SXF_EVENT definition; but extern it for at least its return type
*/
/*FUNC_EXTERN DB_STATUS psy_secaudit(); */
/*
** Moved above declaration to psyaudit.h so that we can prototype it without
** worrying about SXF_EVENT, we'll just include psyaudit.h when we call 
** psy_secaudit(), when we will also have to include sxf.h which contains
** SXF_EVENT
*/

FUNC_EXTERN VOID
psq_dbp_qp_ids(
               PSS_SESBLK       *sess_cb,
               char     	*fe_id,
               DB_CURSOR_ID     *be_id,
               DB_DBP_NAME      *dbp_name,
               DB_OWN_NAME      *dbp_own_name);

FUNC_EXTERN DB_STATUS
psl_find_cons(
	      PSS_SESBLK                *sess_cb,
	      PSS_RNGTAB                *rngvar,
	      PSS_CONS_QUAL_FUNC        qual_func,
	      PTR                       *qual_args,
	      DB_INTEGRITY              *integ,
	      i4                        *found,
	      DB_ERROR                  *err_blk);
FUNC_EXTERN DB_STATUS
psl_qual_ref_cons(
                  PTR           *qual_args,
                  DB_INTEGRITY  *integ,
                  i4            *satisfies);

FUNC_EXTERN bool
psf_retry(
	  PSS_SESBLK    *cb,
	  PSQ_CB        *psq_cb,
	  DB_STATUS     ret_val);

FUNC_EXTERN bool
psf_in_retry(
	     PSS_SESBLK         *cb,
	     PSQ_CB        	*psq_cb);

FUNC_EXTERN DB_STATUS
psl_dp1_dbp_nonkeyword(
PSS_SESBLK  *cb,
PSY_CB      *psy_cb,
PSQ_CB      *psq_cb,
char        *nonkw);

FUNC_EXTERN DB_STATUS
psl_find_iobj_cons(
PSS_SESBLK      *sess_cb,
PSS_RNGTAB      *rngvar,
i4              *found,
DB_ERROR        *err_blk);

DB_STATUS
psl_unorm_error(
PSS_SESBLK *pss_cb,
PSQ_CB      *psq_cb,
DB_DATA_VALUE *rdv,
DB_DATA_VALUE *dv1,
DB_STATUS err_status);

DB_STATUS
psl_collation_check(
	PSQ_CB		*psq_cb,
	PSS_SESBLK	*cb,
	DB_DATA_VALUE	*dv,
	DB_COLL_ID	collID);

/*
** PST_STK - stack structure to support flattening of heavily recursive
** functions. Function pst_push and pst_pop allow for the retention of the
** backtracking points. Function pst_pop_all allows for any cleanup of the
** stack should the traversal be terminated early.
** The stack is initially located on the runtime stack but extensions
** will be placed on the heap - hence the potential for cleanup needed.
** The size of 50 is unlikely to be tripped by any but the most complex
** queries and in tests optimiser memory ran out first.
** History:
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created to support reducing recursion
*/
typedef struct _PST_STK
{
    PSS_SESBLK *cb;
    struct PST_STK1 {
	struct PST_STK1 *link;	/* Stack block link. */
	i4 sp;			/* 'sp' index into list[] at next free slot */
#define PST_N_STK 50
	PTR list[PST_N_STK];	/* Stack list */
    } stk;
} PST_STK;

#define PST_STK_INIT(_s,_cb) \
    {(_s).cb = _cb; (_s).stk.link = NULL; (_s).stk.sp = 0;}

#define PST_DESCEND_MARK ((PST_QNODE**)TRUE)

STATUS 
pst_push_item(PST_STK *, PTR);

PTR
pst_pop_item(PST_STK *);

VOID
pst_pop_all(PST_STK *);

PST_QNODE *
pst_parent_node(PST_STK *base, PST_QNODE *child);

PST_QNODE *
pst_antecedant_by_1type(PST_STK *base, PST_QNODE *child,
	PST_TYPE t1);
PST_QNODE *
pst_antecedant_by_2types(PST_STK *base, PST_QNODE *child,
	PST_TYPE t1,PST_TYPE t2);
PST_QNODE *
pst_antecedant_by_3types(PST_STK *base, PST_QNODE *child,
	PST_TYPE t1, PST_TYPE t2, PST_TYPE t3);
PST_QNODE *
pst_antecedant_by_4types(PST_STK *base, PST_QNODE *child,
	PST_TYPE t1, PST_TYPE t2, PST_TYPE t3, PST_TYPE t4);

/* PST_STK_CMP - context for calling pst_qnode_compare.*/
typedef struct _PST_STK_CMP
{
    PST_STK stk;
    PSS_RNGTAB	**rngtabs;	/* Range table vector */
    u_i4 n_covno;		/* Number of valid VAR vno mapping */
    struct {			/* VAR vno mapping pairs */
	i4 a, b;
    } co_vnos[PST_NUMVARS];
    bool same:1;
    bool reversed:1;
    bool inconsistant:1;
} PST_STK_CMP;
#define PST_STK_CMP_INIT(_c,_cb,_rt) \
    {PST_STK_INIT((_c).stk, _cb) \
    (_c).rngtabs=_rt;(_c).n_covno=0;}

i4
pst_qtree_compare(
    PST_STK_CMP	*ctx,
    PST_QNODE	**a,
    PST_QNODE	**b,
    bool	fixup);

i4
pst_qtree_compare_norm(
    PST_STK_CMP	*ctx,
    PST_QNODE	**a,
    PST_QNODE	**b);

PST_QNODE *
pst_non_const_core(
    PST_QNODE *node);

DB_STATUS
pst_qtree_norm(
    PST_STK	*stk,
    PST_QNODE	**nodep,
    PSQ_CB	*psq_cb);

u_i4
pst_qtree_size(
    PST_STK	*stk,
    PST_QNODE	*node);

#endif /*__PSHPARSE_H_INC*/

