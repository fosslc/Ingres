/*
** Copyright (c) 1986, 2010 Ingres Corporation
**
*/

/**
** Name: PSFPARSE.H - Global parser definitions.
**
** Description:
**      This file contains the types and names defined by the parser, and
**      used by other facilities.
**
** History: 
**      02-jan-86 (jeff)
**	    Combined from several other parser headers.
**	10-jul-86 (jeff)
**	    Changed psq_result in PSQ_CB to a QSO_OBID
**	07-jul-86 (jeff)
**	    Incorporated qrymod.
**	14-jul-86 (jeff)
**	    Added psy_istree to PSY_CB
**	14-jul-86 (jeff)
**	    Added psy_tabname to PSY_CB
**	15-jul-86 (jeff)
**	    Added psy_textout to PSY_CB
**	16-jul-86 (jeff)
**	    Changed psy_tabname to an array of names
**	28-jul-86 (jeff)
**	    Added psq_catupd, psq_idxstruct, psq_mnyfmt, psq_dtefmt to PSQ_CB.
**	30-jul-86 (jeff)
**	    Added psq_qryname to PSQ_CB
**	30-jul-86 (jeff)
**	    Added psq_warnings to PSQ_CB
**	05-aug-86 (jeff)
**	    Add pst_delete to PST_QTREE
**	06-aug-86 (jeff)
**	    Got rid of most user errors; now hard-wired constants
**	11-aug-86 (jeff)
**	    Added pst_unique flag to PST_QTREE.  Got rid of PSQ_RETUNIQUE.
**      13-sep-86 (seputis)
**          Added ptr to ADF_FIDESC descriptor
**	14-oct-86 (daved)
**	    added update column map to qtree header.
**	10-nov-86 (daved)
**	    made psq_qryid in the psq_cb a DB_CURSOR_ID. deleted the
**	    psq_qryname field because it is not used. Created the
**	    PSQ_QDESC for query input.
**	01-dec-86 (daved)
**	    removed psq_qryid from the psq_cb. use psq_cursid for all 
**	    query/cursor name specification.
**	30-jan-87 (daved)
**	    SQL changes
**	18-mar-87 (stec)
**	    Added E_PS0A07_CANTCHANGE definition.
**	    Added pst_resjour to PST_QTREE.
**	31-mar-87 (stec)
**	    Added E_PS0D1F_OPF_ERROR definition.
**	03-apr-87 (puree)
**	    Added E_PSA08_CANTLOCK and error codes related to dynamic
**	    SQL.
**	23-apr-87 (stec)
**	    Changed PSY_CB for GRANT command.
**     11-may-87 (stec)
**	    added psq_udbid to PSQ_CB.
**     24-sep-87 (stec)
**	    added PS0D22 error.
**     14-oct-87 (stec)
**	    added PS0D23 error.
**     20-oct-87 (stec)
**	    Made changes in PST_RT_NODE to preserve old size.
**     21-oct-87 (stec)
**	    Made changes in PST_QTREE to introduce version no.
**	    and renamed old PST_QTREE (RPLUS vsn) to PST_0QTREE.
**	5-feb-88 (stec)
**	    Added PST_UNSPECIFIED to PST_QTREE.
**	17-feb-88 (stec)
**	    Added PST_UNUSED to pst_rgtype defn.
**	17-feb-88 (stec)
**	    Added PST_1_VSN define.
**	22-feb-88 (stec)
**	    Changed PST_CUR_VSN to 2.
**	24-mar-88 (stec)
**	    Added E_PS0008.
**	25-apr-88 (stec)
**	    Make db procedure related changes.
**	    Changed PST_CUR_VSN to 3.
**	28-jul-88 (stec)
**	    Add defines for psy_grant in PSY_CB.
**	29-jul-88 (stec)
**	    Added PSQ_ASSIGN query mode.
**	04-aug-88 (stec)
**	    Change the define for E_PS0904.
**	24-aug-88 (stec)
**	    Add changes for built-ins.
**	02-nov-88 (stec)
**	    Added a define for E_PS0E03_RGENT_ERR.
**	10-nov-88 (stec)
**	    Changed defn of psq_udbid from PTR to I4.
**	11-nov-88 (stec)
**	    Correct bug in PSC_WORDSOVER macro.
**	14-nov-88 (stec)
**	    Add a define for a PSF error.
**	25-jan-89 (neil)
**	    Add PST_CALLPROC, PST_RL_NODE and associated type tags.
**	    Modified PST_STATEMENT for rules processing.
**	08-mar-89 (andre)
**	    Add psq_dba_drop_all to PSQ_CB.
**	10-Mar-89 (ac)
**	    Add PSQ_SECURE for 2PC support.
**	14-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Defined new statement modes, PSQ_[CADK]GROUP and PSQ_[CAK]APLID;
**	    Added psq_group, psq_aplid fields to PSQ_CB;
**	    Defined new qrymod opcodes,  PSY_GROUP and PSY_APLID;
**	    Added psy_grpid, psy_aplid, psy_apass, psy_gtype,
**		  psy_aplflag, psy_grpflag fields to PSY_CB;
**	    Defined grantee types, PSY_[UGA]GRANT
**	    Defined new group operation flags, PSY_[CADK]GROUP;
**	    Defined new applid operation flags, PSY_[CAK]APLID.
**	16-mar-89 (neil)
**          Rules: Added rule operation codes and modified PSQ_Cb and PSY_CB
**	    structures to further support rules processing.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Defined new statement modes, PSQ_[GR]DBPRIV, PSQ_REVOKE;
**	    Defined new qrymod opcode,  PSY_DBPRIV
**	    Added the following fields to the PSY_CB:
**		psy_fl_dbpriv, psy_qrow_limit, psy_qdio_limit,
**		psy_qcpu_limit, psy_qpage_limit, psy_qcost_limit;
**	    Defined new values for psy_grant: PSY_DGRANT, PSY_DREVOKE;
**	    Loaded psy_usrq to be a list of grantees;
**	    Loaded psy_tblq to be a list of objects, including databases.
**	24-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2a:
**	    Redefined PSY_DGRANT;
**	    Added psy_ctl_dbpriv.
**	28-apr-89 (neil)
**	    Rules: Added "table name" field in PSQ_CB for cursor rule
**	    processing (psq_tabname), added comment about reuse of pst_opf.
**	09-may-89 (neil)
**	    Rules: Added pst_rulename to PST_CPROCSTMT for tracing.
**	22-may-89 (ralph)
**	    remove psy_aplid, psy_grpid from PSY_CB.
**	30-may-89 (neil)
**	    Rules: Defined PST_DBPBUILTIN for maximum param# of built-ins.
**	22-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added statement mode PSQ_MXQUERY.
**	    Added psy opcode PSY_MXQUERY.
**	    Added E_PS0D44_IDEF_ERROR.
**	26-jun-89 (neil - my birthday)
**	    Nested Proc: Defined PSQ_CALLPROC for nested proc mode.
**	27-jul-89 (jrb)
**	    Added psq_num_literals to PSQ_CB so PSF can be told which set
**	    of numeric literal scanning rules to use.  Added four new errors
**	    for decimal conversion function.
**	17-aug-89 (andre)
**	    Added pst_groupid to PST_OP_NODE.
**	07-sep-89 (neil)
**	    Alerters: Added new statements modes (replaced old ones),
**	    PSY modes, and PST_STATEMENT structures.
**	08-sep-89 (andre)
**	    Added pst_numjoins to PST_QTREE.
**	2-Oct-89 (teg)
**	    Added PST_3_VSN constant.
**          bumped PST_CUR_VSN to 4 because of Outer Join changes.
**	09-oct-89 (ralph)
**	    Added new statement modes for B1:
**		PSQ_CUSER, PSQ_AUSER, PSQ_KUSER,
**		PSQ_CLOC,  PSQ_ALOC,  PSQ_KLOC,
**		PSQ_CALARM, PSQ_KALARM,
**		PSQ_ENAUDIT, PSQ_DISAUDIT.
**	    Added new PSY opcodes for B1:
**		PSY_USER, PSY_LOC, PSY_AUDIT, PSY_ALARM.
**	    Added fields for B1:
**		psy_usflag, psy_loflag, psy_auflag, psy_alflag, psy_usprivs,
**		psy_lousage, psy_usgroup, psy_aulevel.
**	    Added several constants for above flag fields.
**	27-oct-89 (ralph)
**	    Added psq_ustat to PSQ_CB.
**	    Added PSY_SDROP to psy_grant field to signifiy security_alarms
**	28-dec-89 (andre)
**	    Added psq_fips_mode flag to PSQ_CB.
**	16-jan-90 (ralph)
**	    Added PSY_USRPASS flag to PSY_CB to indicate password specified
**	    on the CREATE/ALTER USER statement.
**	25_apr_90 (fourt)
**	    Changed names PSQ_0_CRT_LINK to PSQ_REG_LINK, PSQ_REGISTER to
**	    PSQ_REG_IMPORT, to reflect change of statements CREATE LINK
**	    and REGISTER to REGISTER...AS LINK and REGISTER...AS IMPORT.
**	23-may-90 (linda)
**	    Changed PSQ_REG_LINK, etc. so we can distinguish all 3 statements:
**		PSQ_0_CRT_LINK	(100)	"create...as link"
**	       *PSQ_REG_LINK	(101)	"register...as link"
**		PSQ_REG_IMPORT	(103)	"register...as import"
**	    *NOTE NAME CHANGE WHEN INTEGRATING STAR CODE.  It was PSQ_REGISTER,
**	     but it always represented "register...as link".
**	08-aug-90 (ralph)
**          Add new statement modes PSQ_[GR]OKPRIV to allow GRANT ON DATABASE
**          with only UPDATESYSCAT and ACCESS privileges in vanilla environs.
**	    Added PSY_LGRANT, PSY_LDROP to psy_cb.psy_grant flags for
**	    granting and dropping permits on locations.
**	    Changed psy_cb.psy_opmap to u_i4.  Added psy_cb.psy_opctl.
**	    Added psy_cb.psy_bpass, etc., to support ALTER USER OLDPASSWORD.
**	11-sep-90 (teresa)
**	    change some bools/nats in PSQ_CB to become bitmasks in
**	    psq_instr status word.
**	12-sep-90 (sandyh)
**	    Added E_PS0704_INSF_MEM to prevent starting psf with session
**	    memory value less than PSF_SESMEM_MIN.
**	9-jan-91 (seputis)
**	    Added support for OPF memory management
**	04-feb-91 (andre)
**	    The following changes were made to support QUEL shared repeat
**	    queries:
**		- defined a new structure - PST_QRYHDR_INFO;
**		- previously reserved field in PST_QTREE, pst_res1, was renamed
**	 	  to pst_info and will be used to contain address
**		  PST_QRYHDR_INFO structure, if one was allocated;
**		- defined 2 new masks over PST_QTREE.pst_mask1;
**		- defined 2 new message numbers;
**	06-feb-91 (andre)
**	    Added a new statement type - ENDLOOP and a new query mode -
**	    PSQ_ENDLOOP;
**	    (this was done in conjunction with a fix fir bug #35659)
**	29-apr-1991 (Derek)
**	    Added new fields to PST_RESTAB for allocation project.
**	17-may-91 (andre)
**	    Defined 3 new query modes: PSQ_SETUSER, PSQ_SETROLE, and
**	    PSQ_SETGROUP.
**	    Added a new field, psq_ret_flag to PSQ_CB.  This field will be used
**	    by PSF to communicate useful info to its callers
**	20-may-91 (andre)
**	    defined E_PS0353_NAME_STRING_TOO_LONG and E_PS0354_SETID_INSUF_PRIV.
**	30-may-91 (andre)
**	    defined messages E_PS0355_DIFF_EFF_USER, E_PS0356_DIFF_EFF_GROUP,
**	    and E_PS0357_DIFF_EFF_ROLE.
**	15-jul-91 (andre)
**	    defined E_PS0358_UNSUPPORTED_IN_B1
**	23-aug-91 (andre)
**	    defined PSQ_REL_MAP
**	11-sep-91 (andre)
**	    Defined PSQ_INDEP_OBJECTS, PSQ_OBJ, PSQ_OBJPRIV, and PSQ_COLPRIV
**	07-nov-91 (andre)
**	    merge 4-feb-91 (rogerk) change:
**		Added PSQ_SLOGGING query mode.
**	07-nov-91 (andre)
**	    merge 25-feb-91 (rogerk) change:
**		Moved PSQ_SLOGGING query mode from 125->142 so as not to clash
**		with new query modes being created in 6.5.
**		Added new PSQ_SON_ERROR query mode for Set Session statement.
**		Added new error message range for Set errors (E_PS_SETERR),
**		along with new error messages for Set Nologging and Set Session.
**	07-nov-91 (andre)
**	    merge 18-dec-90 (kwatts) change:
**		Smart Disk integration: Declared pstflags in PST_OP_NODE and
**		defined one bit for Smart Disk.
**	07-nov-91 (andre)
**	    merge 25-feb-91 (andre) change:
**		- PST_RNGTAB was renamed to PST_RNGENTRY;
**		- a new field, pst_corr_name was added to PST_RNGENTRY to
**		  improve readability of QEPs
**		- PST_QTREE structure has been changed as follows:
**		    - replace PST_RNGTAB (a fixed-size array of structures
**		      representing range table entries) with a variable length
**		      array of pointers to PST_RNGENTRY structures and renamed
**		      it from pst_qrange to pst_rangetab
**		    - add a new field - pst_rngvar_count - which will contain
**		      the number of pointers in the array mentioned above;
**		    - add a new field - pst_basetab_id - which will be used in
**		      6.5 to facilitate invalidation of QEPs which could be
**		      rendered obsolete by destruction of a view, permit, or
**		      database procedure, but is added here in order to reduce
**		      number of new tree version numbers
**	07-nov-91 (andre)
**	    merge 27-feb-91 (ralph) change:
**		Add PST_MSGSTMT.pst_msgdest to support the DESTINATION clause of
**		the MESSAGE and RAISE ERROR statements
**	07-nov-91 (andre)
**	    merge 7-mar-1991 (bryanp) change:
**		Added support for Btree index compression: result tables can now
**		have two forms of compression: data compression and index (also
**		called key, somewhat inappropriately) compression. Thus the
**		pst_compress field in the PST_RESTAB structure is now a flag
**		word rather than a simple boolean, and can have various bit
**		values.
**	07-nov-91 (andre)
**	    merge 25-jul-91 (andre) change:
**		define PSQ_STRIP_NL_IN_STRCONST over PSQ_CB.psq_flag; when set
**		it indicates that the scanners should strip NLs found inside
**		string constants; (bug 38098)
**	12-dec-91 (barbara)
**	    Merge of rplus and Star code for Sybil project.  Added Star error 
**	    codes, modes for PSQ_CB, node descriptors, PST_DSTATE descriptor,
**	    etc. Also added Star comments:
**	    19-aug-88 (andre)
**		added PSQ-DIRCON, PSQ_DIRDISCON, PSQ_DIRSEND, and PSQ_DIREXEC
**	    21-sep-88 (andre)
**		added PSQ_0_CREATE_LINK
**	    22-sep-88 (andre)
**		remove psq_ldbflag from _PSQ_CB and add psq_dfltldb instead.
**	    23-sep-88 (andre)
**		Added E_PS090D_NO_LDB_TABLE
**	    26-sep-88 (andre)
**		Added E_PS090E_COL_MISMATCH, E_PS090F_BAD_COL_NAME
**	    10-Mar-89 (andre)
**		Add psq_dv and psq_dv_num to PSQ_CB to be used for passing
**		parameters and the # of parameters to SCF.
**	    13-mar-89 (andre)
**		Add psq_qid_offset to help SCF find the beginning of the 
**		query ID.
**	    05-jun-91 (fpang)
**		Added psq_mserver to support passing of startup value from scf.
**	    08-jul-91 (rudyw)
**		Comment out text following #endif
**	26-feb-92 (andre)
**	    define PSQ_REPAIR_SYSCAT over PSQ_CB.psq_flag
**	30-apr-1992 (bryanp)
**	    Added E_PS0009_DEADLOCK definition.
**	04-may-1992 (bryanp)
**	    Temporary table enhancements: added pst_temporary field to the
**	    PST_RESTAB to handle lightweight session tables.  Added
**	    PSY_IS_TMPTBL flagmask to psy_obj_mask definitions.  Added
**	    pst_recovery to the PST_RESTAB to handle the WITH NORECOVERY and
**	    clauses of the lightweight session table's CREATE TABLE statement.
**	    Add definitions for new error messages.
**	    Add new query modes PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**      27-may-92 (schang)
**          add PSQ_REG_INDEX and PSQ_REG_REMOVE
**          new syntax errors relates to GW starting with E_PS_GW
**      26-jun-92 (anitap)
**          Added new PSQ_UPD_ROWCNT query mode for Set Update_Rowcount
**          statement in PSQ_CB.
**	14-sep-92 (robf)
**	     add PSQ_GWFTRACE for C2 Security Audit/GWF tracing
**	05-oct-92 (barbara)
**	    added two new error definitions for GW REGISTER syntax errors.
**	26-oct-92 (jhahn)
**	    Added PST_IS_BYREF to PST_SYMBOL
**	     and pst_return_status to PST_CPROCSTMT for DB procedures..
**	26-oct-92 (robf)
**	    added two new error definitions for handling Gateway errors
**	    coming back through QEF.
**	02-nov-92 (jrb)
**	    Added PSQ_SWORKLOC for "set work locations..." command
**	    (multi-location sorts project).
**	26-oct-92 (ralph)
**	    Added E_PS0420_DROPOBJ_NOT_OBJ_OWNER.
**	    Added PSL_ID_* constants for translating identifiers.
**	    Added E_PS_IDERR and related message constants.
**	09-nov-92 (rickh)
**	    Improvements for FIPS CONSTRAINTS.  DDL support.
**	    Added errors for CONSTRAINTS and multi-column rules.   (rblumer)
**	    Added PSQ_CONS, modified PST_CREATE_TABLE and PST_CREATE_INTEGRITY.
**	19-nov-92 (rblumer)
**	    Changed QSO_OBIDs to text string and qtree pointers in
**	    PST_CREATE_INTEGRITY, PST_CREATE_RULE, and PST_CREATE_PROCEDURE.
**	19-nov-92 (anitap)
**	    Added statements PST_CREATE_SCHEMA_TYPE and PST_EXEC_IMM_TYPE.
**	    Added PST_CREATE_SCHEMA, PST_EXEC_IMM and PST_INFO for CREATE 
**	    SCHEMA project.
**	25-nov-92 (ralph)
**	    CREATE SCHEMA:
**	    Modified PST_INFO to contain additional info required
**	    by the parser.
**	    Introduced PST_OBJDEP structure to contain object dependencies
**	    for creating views within the CREATE SCHEMA wrapper.
**	    Changed PST_EXEC_IMM.pst_info to be an embedded structure
**	    rather than a pointer.
**	01-dec-92 (andre)
**	    SET USER AUTHORIZATION will change into SET SESSION AUTHORIZATION +
**	    we are getting rid of SET GROUP/ROLE.
**	    I will
**		- change PSQ_SETUSER to PSQ_SET_SESS_AUTH_ID,
**		- change PSQ_USE_SESSION_ID to PSQ_USE_SESSION_USER,
**		- add PSQ_USE_INITIAL_USER,
**		- get rid of PSQ_SETGROUP, PSQ_SETROLE, PSQ_CLEAR_ID,
**		  PSQ_RESET_EFF_GROUP_ID, PSQ_RESET_EFF_ROLE_ID
**	02-dec-92 (andre)
**	    undefined E_PS0356_DIFF_EFF_GROUP and E_PS0357_DIFF_EFF_ROLE because
**	    we are unsupporting SET GROUP and SET ROLE
**
**	    defined E_PS0359_NOT_A_VALID_AUTH_ID
**	17-dec-92 (ralph)
**	    CREATE SCHEMA:
**	    Added pst_objname to PST_INFO for reordering
**	    Added pst_depstmt to PST_OBJDEP for reordering
**	12-jan-92 (pholman)
**	    Add E_PS1005_SEC_WRITE_ERROR for failed sxf_call audit calls.
**      13-jan-1993 (stevet)
**          Added E_PS0C90_MUSTBE_CONST_INT which is the message id for
**          invalid value for the second argument of string functions
**	12-jan-93 (ralph)
**	    CREATE SCHEMA:
**	    Added PST_OBJDEP.pst_depuse for reordering statements
**      05-jan-93 (rblumer)
**          Added to PSQ_CB: set_proc_id and set_input_tabid for set-input
**          dbprocs, new query mode for creating set-input procs: PSQ_CRESETDBP.
**      10-feb-93 (rblumer)
**          Added new query mode for distributed CREATE TABLE, as SCF must
**          treat it like 6.4 CREATE TABLE and not like the new 6.5 processing.
**	    Added new error for Teresa: E_PS0D2A_RDF_READTUPLES.
**          Added several error messages for CONSTRAINTS and UNIQUE_SCOPE.
**      18-feb-93 (rblumer)
**          Shortened 2 error-number names that were longer than 32 characters.
**	    Added pst_defid to PST_RSDM_NODE structure.
**	18-feb-93 (rickh)
**	    Added constraint schema name to the CREATE_INTEGRITY structure.
**	05-mar-93 (ralph)
**	    CREATE SCHEMA:
**	    Added the following internal error codes:
**	    E_PS_SCHEMA (root for CREATE SCHEMA internal errors),
**	    E_PS04A0_BAD_SCHEMA_TREE, E_PS04A1_BAD_SCHEMA_NODE, and
**	    E_PS04A2_BAD_SCHEMA_OBJECT.
**	    DELIM_IDENT:
**	    Added psq_dbxlate to PSQ_CB for case translation semantics.
**	10-mar-93 (barbara)
**	    Added Star error message E_PS091E_INCOMPAT_CASE_MAPPING.
**	15-mar-93 (ralph)
**	    Move E_PS04Ax to E_PS04Cx to make way for more constraints msgs
**	17-mar-93 (rblumer)
**	    Move DEFAULT_TOO_LONG error message to number 4A0,
**	    add E_PS04A1_NO_REF_PRIV error message.
**	30-mar-93 (rblumer)
**	    in PSQ_CB, swapped PSQ_CREATE and PSQ_DISTCREATE query modes (for
**	    easier error handling in pslyerror.c) and added PSQ_NO_CHECK_PERMS,
**	    so that don't check permissions on internal queries used to verify
**	    constraints hold during alter table ADD constraint.
**	    added E_PS04A2_NO_REF_VIEW error message.
**	    added E_PS0BB5_NO_PERSIST_TABLE & E_PS04A3_NO_GRANT error messages.
**	05-apr-93 (anitap)
**	    in PSY_CB, added a new define for psy_flags for CREATE SCHEMA. 
**	02-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added psq_cat_owner to PSQ_CB to define the catalog owner.
**	    Changed psq_dbxlate to be a pointer to a u_i4.
**	12-apr-93 (tam)
**	    Added PSQ_OWNER_SET to PSQ_CB for owner.procedure project.
**	24-may-93 (ralph)
**	    Added PST_INFO.pst_srctext to point to query text in the query
**	    buffer.  This is used to correct the problem with GRANT WGO
**	    and CREATE VIEW WCO.
**	02-jun-93 (rblumer)
**	    Changed PSQ_NO_CHECK_PERMS so as not to conflict with PSQ_OWNER_SET
**	09-jul-93 (andre)
**	    added prototype declarations for externally used PSF functions
**	16-jul-93 (rblumer)
**	    Added error E_PS0459_ILLEGAL_II_NAME for synonyms and events;
**	    changed PST_EMPTY_REFERENCING_TABLE to PST_EMPTY_TABLE in PST_INFO.
**	26-jul-93 (anitap)
**	    Added error message for "SET UPDATE_ROWCOUNT" project.
**	1-jun-1993 (robf)
**	    Secure 2.0: Add PSQ_ROW_SEC_LABEL, PSQ_ROW_SEC_AUDIT and
**	    PSQ_ROW_SEC_KEY, PSQ_ROW_SEC_LBLID
**	    Add PSQ_SXFTRACE for SXF tracing
**	16-jul-93 (robf)
**	    Secure 2.0: Add more options for PSY_CB:
**	    psy_alflag, psy_auflag
**	10-aug-93 (rblumer)
**	    Added errors E_PS04A5_NO_COLUMNS and E_PS04A6_UDT_NO_LONGTXT.
**	27-aug-93 (robf)
**	    Secure 2.0: Add pst_seckey info to PST_RESTAB
**	11-oct-93 (rblumer)
**	    Removed errors E_PS0497_CONS_NOT_HOLD and E_PS049F_VALIDATE_CONS; 
**	    the code using them has been moved to QEF (i.e. pslatbl.c code).
**	13-oct-93 (andre)
**	    defined E_PS0BE0_TOO_MANY_COLS and E_PS0BE1_TUPLE_TOO_WIDE
**	18-oct-93 (rogerk)
**	    Added PSQ_NO_JNL_DEFAULT psq_flag which specifies that tables
**	    should be created with nojournaling by default.
**	26-oct-93 (robf)
**	    Add PSQ_MXSESSION for session limits 
**	01-nov-93 (anitap)
**	    defined PSQ_INGRES_PRIV over PSQ_CB.psq_flag for verifydb so that
**	    $ingres can drop and create constraints on tables owned by other
**  	    users. Removed duplicate definitions of PSQ_DFLT_DIRECT_CRSR and 
**	    PSQ_NOFLATTEN_SINGLETON_SUBSEL in PSQ_CB.
**	17-dec-93 (rblumer)
**	    removed PSQ_FIPS_MODE flag from PSQ_CB.
**	20-jan-94 (rblumer)
**	    added E_PS049f_NO_PERIPH_DEFAULT error.
**	31-mar-94 (rblumer)
**	    added E_PS045F_II_WRONG_CASE error;
**	    remove E_PS_ID errors, as these are now E_US errors (used in CUI).
**	18-apr-94 (rblumer)
**	    cleanup from Constraints Code Review:
**	      -to achieve more type-correctness, changed a few PTRs to struct
**	       <abc> * (which will still compile even if that structure
**	       definition is not included by all includers of THIS header file).
**	      -changed name of pst_integrityTuple2 to be more descriptive.
**      07-oct-94 (mikem)
**	    Added the the PST_4IGNORE_DUPERR bit to be used in pst_mask1 if 
**	    the ps201 tracepoint has been executed in this session.  This 
**	    traceflag will be used by the CA-MASTERPIECE product so that 
**	    duplicate key errors will not shut down all open cursors (whenever 
**	    the "keep cursors open on errors" project is integrated this change
**          will no longer be necessary).
**      31-oct-94 (tutto01)
**          defined E_PS03A9_FOREIGN_COL_EXCEEDED to check if maximum
**          number of columns is exceeded in foreign key
**	29-dec-94 (wilri01)
**	    Added E_PS03AA_MISSING_JQUAL:SS42000_SYN_OR_ACCESS_ERR
**	    to replace E_PS03A2_MISSING_JQUAL:SS42000_SYN_OR_ACCESS_ERR
**	    because natural join is not supported.
**      04-apr-1995 (dilma04)
**          Added PSQ_STRANSACTION querymode.
**      28-nov-1995 (stial01)
**          Added PSQ_XA_ROLLBACK, PSQ_XA_COMMIT, PSQ_XA_PREPARE
**          Added psq_distranid, PSQ_SET_XA_XID
**	17-jan-96 (pchang)
**	    Changed E_PS03A9_FOREIGN_COL_EXCEEDED to 
**	    E_PS03A9_CONSTRAINT_COL_EXCEEDED since its use is generic to all
**	    table constraint types (ref B71935).
**	29-april-1997(angusm)
**	    Add PST_XTABLE_UPDATE, PSQ_AMBREP_64COMPAT (bug 79623)
**	06-mar-1996 (stial01 for bryanp)
**	    Add pst_page_size to PST_RESTAB.
**	20-jun-1996 (shero03)
**	    Added PS0BCA-PS0BCE for RTree project
**      22-jul-1996 (ramra01 for bryanp)
**          Added PSQ_ATBL_ADD_COLUMN and PSQ_ATBL_DROP_COLUMN query modes.
**	10-oct-1996 (canor01)
**	    Change E_PS045F_II_WRONG_CASE to E_PS045F_WRONG_CASE.
**      02-dec-1996 (nanpr01)
**          Added message E_PS0010_TIMEOUT if lock timer expires instead
**          of giving E_PS0001_USER_ERROR.
**      24-jan-97 (dilma04)
**          Add E_PS03B0_ILLEGAL_ROW_LOCKING if row locking was specified
**          for a system catalog or a table with 2k page size.
**	27-Feb-1997 (jenjo02)
**	    Extended <set session> statement to support <session access mode>
**	    (READ ONLY | READ WRITE).
**	    Added PSQ_SET_READ_ONLY, PSQ_SET_READ_WRITE.
**	29-sep-1997 (shero03)
**	    B85904: added PS0BCF.
**      14-oct-1997 (stial01)
**          Added <set session isolation level>
**          Added PSQ_SET_ISOLATION, psq_isolation
**	19-Jan-1999 (shero03)
**	    Add set random_seed
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	22-feb-1999 (somsa01)
**	    Removed second definition of psq_random_seed due to previous
**	    cross-integration.
**	26-apr-1999 (shero03)
**	    Added PST_OPERAND to support Multivariate Functions.
**	 7-jul-1999 (hayke02)
**	    Added PST_HAVING to pst_mask1 to indicate a having clause. This
**	    change fixes bug 95906.
**	26-nov-1999 (hayke02)
**	    Replace PST_HAVING with PST_5HAVING - this is a flag for
**	    the pst_mask1 in PST_RT_NODE not in PST_QNODE.
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          Enhanced OPC to support compilation of the non-cnf case
**          in opc_crupcurs().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Made additions to PSQ_CB and PST_CPROCSTMT in order to
**          be able to flag the prefetch stategy required to ensure
**          consitent behaviour in updating rules fired by updates.
**	10-Jan-2001 (jenjo02)
**	    Added (PTR) psq_adfcb to PSQ_CB, filled in by scsinit so that 
**	    psq_bgn_session doesn't have to call back to SCF right away to get it.
**	    Added psq_psscb_offset to PSQ_CB, offset from CS_SCB to 
**	    session's PSS_SESBLK.
**	02-Apr-2001 (jenjo02)
**	    Added psy_loc_rawblocks.
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**          Added E_PS0BB6_PERSISTENT_CONSTRAINT.
**	11-May-2001 (jenjo02)
**	    Changed psy_loc_rawblocks to psy_loc_rawpct.
**	14-jun-2002 (hayke02)
**	    Added E_PS03AB_AGGR_IN_GROUP_BY. This change fixes bug 107026.
**	25-nov-02 (inkdo01)
**	    Added definitions for increased range variable limit - analogous 
**	    to column maps.
**	24-apr-03 (inkdo01)
**	    Added PS0D3D & PS0427 for sequence privilege messages.
**      24-jul-03 (stial01)
**          Added PSQ_GET_STMT_INFO and PSQ_STMT_INFO for b110620
**      03-oct-03 (chash01) Add register table/index error where
**          multi-column key have data type other than char or varchar.
**          #define E_PS1110_REG_TAB_IDX_MULTI_COL	(E_PS_GW + 0x10L)
**	17-nov-2003 (xu$we01)
**	    Addd E_PS059D_WCO_NONUPDATEABLE_VIEW for bug 110708.
**      02-jan-2004 (stial01)
**          Changes for SET [NO]BLOBJOURNALING, SET [NO]BLOBLOGGING
**	29-Jan-2004 (schka24)
**	    Added messages for partition definition parsing.
**	    Added qeu-cb pointer to qtree header.
**	09-feb-2004 (somsa01)
**	    Backed out original fix for bug 110708 so that it can be properly
**	    fixed later.
**	19-feb-2004 (gupsh01)
**	    Changes for SET [NO]UNICODE_SUBSTITUTION.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**       4-May-2004 (hanal04) Bug 112265 INGSRV2814
**          XF_PST_NUMVARS has been added to xf.qsh for copydb and unloaddb
**          to prevent the generation of parallel index creation statements
**          that will blow PST_NUMVARS. Added comment to this header file
**          to ensure any future change to PST_NUMVARS is reflected in
**          XF_PST_NUMVARS.
**      27-aug-2004 (stial01)
**          Removed SET NOBLOBLOGGING statement
**	18-Oct-2004 (shaha03)
**	    SIR # 112918 Added PSQ_DFLT_READONLY_CRSR for configurable
**	    default cursor open mode .
**	28-oct-04 (inkdo01)
**	    Undo the damage of 471455 and reset PST_NUMVARS as per expanded
**	    range table changes.
**	7-oct-2004 (thaju02)
**          Change psq_mserver to SIZE_TYPE.
**	3-Feb-2005 (schka24)
**	    Add 4-byte-tid backward compatability flag, rename
**	    psq_num_literals to psq_parser_compat.
**	19-jul-2005 (toumi01)
**	    Support CREATE/ALTER USER/ROLE WITH PASSWORD=X'<encrypted>'.
**	22-Jul-2005 (thaju02)
**	    Add PSQ_ON_USRERR_NOROLL, PSQ_ON_USRERR_ROLLBACK.
**      22-feb-2006 (stial01)
**          Added pst_pmask flag PST_CPSYS (b115765)
**	3-Jun-2006 (kschendel)
**	    Spiff up "symbol" node variants doc some.
**	30-aug-2006 (thaju02) (B116355)
**	    Add PSQ_RULE_DEL_PREFETCH and PST_DEL_PREFETCH.
**      15-aug-2006 (huazh01)
**          Add PST_6HAVING for query contains two aggregate in its
**          having clause. This fixes b116202.
**    25-oct-2007 (smeke01) b117218
**        Add error message for CASE syntax not supported with subselect.
**	10-dec-2007 (smeke01) b118405
**	    Add defines PST_AGHEAD_MULTI, PST_IFNULL_AGHEAD, 
**	    PST_IFNULL_AGHEAD_MULTI.
**	 2-jun-2008 (hayke02)
**	    Add PST_7HAVING for a "...having [not] <agg>..." style query. This
**	    change fixes bug 120553.
**	07-aug-2008 (gupsh01)
**	    Added error message E_PS04B4_DEFAULT_ANSIDT_IN_DBPROC, to
**	    inform the user that we don't support default for
**	    declaring ANSI date/time/timestamp type parameters/
**	    variables in database procedures.
**      22-sep-2008 (stial01)
**          Added msgs for unicode/utf8 normalization
**	11-May-2009 (kibro01) b121739
**	    Added E_PS03AE_COLREF_IN_WRONG_UNION_PART.
**	18-Aug-2009 (drivi01)
**	    Undefine PST_UNSPECIFIED before the define if it was
**          already defined. This is done to remove the warning
**          on Windows with Visual Studio 2008 compiler:
**	    'PST_UNSPECIFIED' : macro redefinition
**	    C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\winbase.h(547)
**	    Potentially this define should be renamed to something
**	    else to avoid the clash.  This is a temporary solution.
**	16-Jun-2009 (kiria01) SIR 121883
**	    Scalar Sub-queries changes.
**	5-nov-2009 (stephenb)
**	    various defines for batch execution poroject
**	11-Nov-2009 (kiria01) SIR 121883
**	    With PST_INLIST extend meaning so that it qualifies the operator 
**	    and servies to distinguish simple = from IN and <> from NOT IN.
**	    Thus the may be PST_SUBSEL on RHS as well as a compressed list
**	    PST_CONST.
**	03-Dec-2009 (kiria01) SIR 121883
**	    Added E_PS03AF_SUBSEL_ORDERBY_WITH_UNION
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type psq_call to use the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	12-Mar-2010 (thaju02) Bug 123440
**	    Added defines PSQ_GET_SESS_INFO, PSQ_SESS_CACHEDYN, 
**	    PSQ_SESS_NOCACHEDYN.
**	21-apr-2010 (toumi01) SIR 122403
**	    Add E_PS0C85_ENCRYPT_INVALID.
**	04-may-2010 (miket) SIR 122403
**	    Add for more thorough syntax error reporting:
**	    E_PS0C86_ENCRYPT_NOTABLVL, E_PS0C87_ENCRYPT_NOCOLS,
**	    E_PS0C88_ENCRYPT_NOPASS
**	24-jun-2010 (stephenb)
**	    Add E_PS03B4_NO_COPY_CACHE
**      11-Aug-2010 (hanal04) Bug 124180
**          Added PSQ_MONEY_COMPAT. Set if money_compat true in config.dat.
**/

/*
**  Error definitions.
*/

/*
**  Types of errors.
*/

#define                 PSF_USERERR     1   /* Error by user */
#define			PSF_INTERR	2   /* Error internal to PSF */
#define			PSF_CALLERR	3   /* Error by caller */

/*
**	Base value for parser facility errors.
*/
#define			E_PS_ERRORS	    0x00050000L

/*
**	General errors.
*/

/* Success */
#define			E_PS0000_OK	    0L
/* Error by user */
#define			E_PS0001_USER_ERROR (E_PS_ERRORS + 0x1L)
/* Undefined internal error */
#define			E_PS0002_INTERNAL_ERROR (E_PS_ERRORS + 0x2L)
/* Operation was interrupted */
#define			E_PS0003_INTERRUPTED  (E_PS_ERRORS + 0x3L)
/* Unexpected exception */
#define			E_PS0004_EXCEPTION  (E_PS_ERRORS + 0x4L)
/* User must abort transaction */
#define			E_PS0005_USER_MUST_ABORT (E_PS_ERRORS + 0x5L)
/* Unknown operation */
#define			E_PS0006_UNKNOWN_OPERATION (E_PS_ERRORS + 0x6L)
/* Internal error in other facility */
#define			E_PS0007_INT_OTHER_FAC_ERR (E_PS_ERRORS + 0x7L)
/* Retry this query */
#define			E_PS0008_RETRY (E_PS_ERRORS + 0x8L)
/* Resource deadlock encountered: */
#define			E_PS0009_DEADLOCK (E_PS_ERRORS + 0x9L)
/* Resource timeout encountered: */
#define                 E_PS0010_TIMEOUT  (E_PS_ERRORS + 0x10L)

/*
**	Errors having to do with error handling.
*/
#define			E_PS_ER		    (E_PS_ERRORS + 0x100L)
#define E_PS0101_BADERRTYPE   (E_PS_ER + 0x1L)     /* Bad error type */
#define E_PS0102_TOO_MANY_ERRS (E_PS_ER + 0x2L)    /* Errs won't fit in cb */
#define E_PS0103_ERROR_NOT_FOUND (E_PS_ER + 0X3L)  
					    /* Erro message not in er file */
/*
**	Errors having to do with starting and finding sessions.
*/
#define			E_PS_SES	    (E_PS_ERRORS + 0x200L)
#define E_PS0201_BAD_QLANG    (E_PS_SES + 0x1L)    /* Unknown query lang. */
#define E_PS0202_QLANG_NOT_ALLOWED (E_PS_SES + 0x2L) /* Unallowed qry lang */
#define E_PS0203_NO_DECIMAL   (E_PS_SES + 0x3L)  /* Decimal mark not spec */
#define E_PS0204_BAD_DISTRIB  (E_PS_SES + 0x4L)  /* Bad distrib spec */
#define E_PS0205_SRV_NOT_INIT (E_PS_SES + 0x5L)  /* Server not initialized */
#define E_PS0206_SES_NOT_FOUND (E_PS_SES + 0x6L) /* Can't find session cb */
#define E_PS0207_YACC_ALLOC   (E_PS_SES + 0x7L)  /* Can't alloc yacc cb */
#define E_PS0208_TOO_MANY_SESS (E_PS_SES + 0x8L) /* Too many sessions */
						 /*
						 ** failed to initialize a
						 ** semaphore at server startup
						 */
#define E_PS0209_INIT_SEM_FAILURE (E_PS_SES + 0x9L)
						 /*
						 ** failed to get a semaphore at
						 ** session startup
						 */
#define E_PS020A_BGNSES_GETSEM_FAILURE (E_PS_SES + 0xAL)
						 /*
						 ** failed to release a
						 ** semaphore at session startup
						 */
#define E_PS020B_BGNSES_RELSEM_FAILURE (E_PS_SES + 0xBL)
						 /*
						 ** failed to get a semaphore at
						 ** session shutdown
						 */
#define E_PS020C_ENDSES_GETSEM_FAILURE (E_PS_SES + 0xCL)
						 /*
						 ** failed to release a
						 ** semaphore at session
						 ** shutdown
						 */
#define E_PS020D_ENDSES_RELSEM_FAILURE (E_PS_SES + 0xDL)

						/*
						** unexpected error encountered
						** when calling DMC_SHOW to
						** determine whether a DB is
						** being journaled
						*/
#define	E_PS020E_CANT_GET_DB_JOUR_STATUS (E_PS_SES + 0xEL)

/*
**	Errors having to do with parsing QUEL or SQL statements.
*/
#define                 E_PS_PRS        (E_PS_ERRORS + 0x300L)
#define E_PS0309_ALLOC_SYMTAB	(E_PS_PRS + 0x9L)   /* Can't alloc symbol tab */
#define E_PS030A_MULT_STMT_ALT	(E_PS_PRS + 0xaL)   /* Mult stmts in alt qry */
#define E_PS030B_RETRY_AND_ALT (E_PS_PRS + 0xbL)   /* Simul. alt & reparse */
#define	E_PS030C_NO_PREV_STMT	(E_PS_PRS + 0xcL)   /* No prv stmt on reparse */
#define E_PS030D_NO_MORE_QRYS	(E_PS_PRS + 0xdL)   /* No more queries */
#define E_PS030F_BACKUP	(E_PS_PRS + 0xfL)   /* Error backing up a tok */
#define E_PS0311_YACC_UNKNOWN	(E_PS_PRS + 0x11L)  /* Unknown yacc error */
    /* illegal char in a delimited identifier */
#define E_PS0322_BAD_CHAR_IN_DELIM_ID   (E_PS_PRS + 0x22L)
    /* delimited identifier is too long */
#define E_PS0323_DELIM_ID_TOO_LONG      (E_PS_PRS + 0x23L)
    /* delimited identifier is too sshort */
#define E_PS0324_DELIM_ID_TOO_SHORT     (E_PS_PRS + 0x24L)
    /* reached end of query buffer without seeing the closing double quote */
#define E_PS0325_MISSING_DELIM_ID_END   (E_PS_PRS + 0x25L)
/* unknown parameter in "set lockmode" command -- internal error */
#define E_PS0351_UNKNOWN_LOCKPARM (E_PS_PRS + 0x51L)
/*
** SET LOCKMODE ON <table> where TIMEOUT=0 specified for one of the SCONCUR
** catalogs, iistatistics, or iihistogram while the PSF startup flag indicated
** that this should be disallowed due to potential deadlocks in OPF
*/
#define	E_PS0352_ILLEGAL_0_TIMEOUT  (E_PS_PRS + 0x52L)
					    /*
					    ** string literal used to express a
					    ** user/role/group name is too long
					    */ 
#define	E_PS0353_NAME_STRING_TOO_LONG	(E_PS_PRS + 0x53L)
					    /*
					    ** user attempting to change
					    ** session's user, group, or role id
					    ** lack necessary privileges
					    */
#define	E_PS0354_SETID_INSUF_PRIV	(E_PS_PRS + 0x54L)
					    /*
					    ** a dynamically prepared statement
					    ** cannot be executed because the
					    ** current effective user name is
					    ** different from that in effect
					    ** when the query was prepared.
					    */
#define	E_PS0355_DIFF_EFF_USER		(E_PS_PRS + 0x55L)
					    /*
					    ** certain statements (e.g.
					    ** SET SESSION AUTHORIZATION) are
					    ** not supported in B1 environment
					    */
#define	E_PS0358_UNSUPPORTED_IN_B1      (E_PS_PRS + 0x58L)
					    /*
					    ** value specified with SET SESSION
					    ** AUTHORIZATION statement did not
					    ** constitute a valid authorization
					    ** identifier
					    */
#define	E_PS0359_NOT_A_VALID_AUTH_ID	(E_PS_PRS + 0x59L)
					    /*
					    ** User tried to query a system
					    ** catalog without appropriate 
					    ** privileges
					    */
#define	E_PS035A_CANT_QUERY_SYSCAT	(E_PS_PRS + 0x5AL)
					    /*
					    ** User tries to access table
					    ** statistics
					    */
#define	E_PS035B_CANT_ACCESS_TBL_STATS	(E_PS_PRS + 0x5BL)

/* Bad decimal specifier */
#define E_PS035E_BAD_DECIMAL   (E_PS_PRS + 0x5EL)
/* Bad function instance id in query tree operator node */
#define E_PS0365_BAD_FUNC_INST (E_PS_PRS + 0x65L)
/* No complement for comparison operator instance */
#define E_PS0366_NO_COMPLEMENT (E_PS_PRS + 0x66L)
/* Error opening memory stream for text chain */
#define E_PS0370_OPEN_TEXT_CHAIN (E_PS_PRS + 0x70L)
/* Error allocating piece from text chain */
#define E_PS0371_ALLOC_TEXT_CHAIN (E_PS_PRS + 0x71L)
/* Error closing text chain */
#define E_PS0372_CLOSE_TEXT_CHAIN (E_PS_PRS + 0x72L)
/* Error opening query text in QSF */
#define E_PS0373_OPEN_QSF_TEXT (E_PS_PRS + 0x73L)
/* Error allocating query text in QSF */
#define E_PS0374_ALLOC_QSF_TEXT (E_PS_PRS + 0x74L)
/* Error unlocking query text in QSF */
#define E_PS0375_UNLOCK_QSF_TEXT (E_PS_PRS + 0x75L)
/* Error allocating mem in scanner, largest block too small */
#define E_PS0376_INSUF_MEM (E_PS_PRS + 0x76L)
/* Error calling ADF adi_pm_encode function */
#define E_PS0377_ADI_PM_ERR (E_PS_PRS + 0x77L)
/* Error calling SCF getting term name */
#define E_PS0378_SCF_TERM_ERR (E_PS_PRS + 0x78L)
/* Error calling QSF to translate or define */
#define E_PS0379_QSF_T_OR_D_ERR (E_PS_PRS + 0x79L)
/* Error calling QSF to translate or define */
#define E_PS037A_QSF_TRANS_ERR (E_PS_PRS + 0x7AL)
/* subselects are not allowed in join qualifications */
#define E_PS037B_UNORM (E_PS_PRS + 0x7BL)
#define E_PS037C_UNORM_DIAG (E_PS_PRS + 0x7CL)
#define E_PS03A0_SUBSEL_IN_JQUAL (E_PS_PRS + 0xA0L)
/* One may not specify join qualification for NATURAL joins */
#define E_PS03A1_JQUAL_IN_NATJOIN (E_PS_PRS + 0xA1L)
/* qualifications is required for joins other than NATURAL */
#define E_PS03A2_MISSING_JQUAL (E_PS_PRS + 0xA2)
/*
** join_search condition referenced a column not found in right/left relation(s)
*/
#define E_PS03A3_COL_NOT_IN_JREL (E_PS_PRS + 0xA3)
/*
** Relation referenced in the join_search condition is not involved in the join
*/
#define E_PS03A4_NOT_A_JREL	(E_PS_PRS + 0xA4)
/*
** Left or right relations of a NATURAL join had a like-named column which
** occurred more than once in one of the sides.
*/
#define E_PS03A5_MULT_COMMON_COLS (E_PS_PRS + 0xA5)
/* It is illegal to use aggregates in join_search condition */
#define E_PS03A6_AGGR_IN_JOIN_COND (E_PS_PRS + 0xA6)
/*
** Right-hand side of with-clause option may not be quoted in
** non-distributed thread.
*/
#define E_PS03A7_QUOTED_WITH_CLAUSE (E_PS_PRS + 0xA7)
/*
** Number of columns in table constraint cannot exceed DB_MAXKEYS 
*/
#define E_PS03A9_CONSTRAINT_COL_EXCEEDED (E_PS_PRS + 0xA9)
/*
**	Obsolete PRIVILEGES specified for user authorization
*/
#define W_PS03A8_OBSOLETE_PRIV		(E_PS_PRS + 0xA8)
/*
**	Aggregates are not allowed in order by or group by clauses
*/
#define E_PS03AA_AGGR_IN_ORDER_BY (E_PS_PRS + 0xAA)
#define E_PS03AB_AGGR_IN_GROUP_BY (E_PS_PRS + 0xAB)
/* Named columns join columns must be in both sides of join. */
#define E_PS03AC_MISSING_COLUMN (E_PS_PRS + 0xAC)
/* Certain forms of the CASE syntax are not currently supported */
#define E_PS03AD_CASE_NOT_SUPPORTED_WITH_SUBSELECT (E_PS_PRS + 0xAD)
/* If table name is specified, it must be from the first segment of union */
#define E_PS03AE_COLREF_IN_WRONG_UNION_PART (E_PS_PRS + 0xAE)
/* Sub-select disallowed in orderby of unioned select */
#define E_PS03AF_SUBSEL_ORDERBY_WITH_UNION (E_PS_PRS + 0xAF)

/*
** SET LOCKMODE ON <table> where LEVEL=ROW specified for a system catalog, 
** or for a table with 2K page size. This is not allowed.
*/
#define E_PS03B0_ILLEGAL_ROW_LOCKING    (E_PS_PRS + 0xB0L)
#define E_PS03B1_NULL_RCB		(E_PS_PRS + 0xB1L)
#define E_PS03B2_INVALID_CPLENGTH	(E_PS_PRS + 0xB2L)
#define	E_PS03B3_NULL_QUERY_TREE	(E_PS_PRS + 0xB3L)
#define E_PS03B4_NO_COPY_CACHE		(E_PS_PRS + 0xB4L)

/*
**	Errors having to do with hashing and finding cursor control blocks +
**	other miscellaneous cursor-related errors
*/
#define			E_PS_CUR	    (E_PS_ERRORS + 0x400L)

    /* Cursor CB not found */
#define E_PS0401_CUR_NOT_FOUND		(E_PS_CUR + 0x1L)
    /* attempt to update a non-updatable column using a cursor */
#define	E_PS0402_NONUPDATABLE_COLUMN	(E_PS_CUR + 0x2L)
    /*
    ** attempt to update through a cursor a column for which a user
    ** lacks UPDATE privilege
    */ 
#define	E_PS0403_CURS_UPDT_NO_PRIV	(E_PS_CUR + 0x3L)
    /* 
    ** repeat cursor opened not FOR READONLY 
    */
#define	E_PS0404_REP_CURS_NOT_FOR_RDONLY (E_PS_CUR + 0x4L)

/*
** Errors having to do with inappropriate use of owner.table construct in DDL
** statements.  
**
** 18-oct-90 (andre)
**	error messages were reworked to reduce number of messages. Corresponding
**	changes were also made in the code.  Usage of messages was changed as
**	follows:
**
**	if previously used		    will use
**	------------------		    --------
**	E_PS0421_CRTIND_NOT_IND_OWNER	    E_PS0421_CRTOBJ_NOT_OBJ_OWNER
**	E_PS0422_CRTIND_NOT_TBL_OWNER	    E_PS0424_ILLEGAL_TBL_REF
**	E_PS0424_CRTTBL_NOT_OBJ_OWNER	    E_PS0421_CRTOBJ_NOT_OBJ_OWNER
**	E_PS0425_CRTVIEW_NOT_OBJ_OWNER	    E_PS0421_CRTOBJ_NOT_OBJ_OWNER
**	E_PS0426_ILLEGAL_TBL_REF	    E_PS0424_ILLEGAL_TBL_REF
**	E_PS0427_ILLEGAL_DBP_REF	    E_PS0425_ILLEGAL_DBP_REF
**	E_PS0428_CRTDBP_NOT_DBP_OWNER	    E_PS0421_CRTOBJ_NOT_OBJ_OWNER
**	E_PS0429_DROPDBP_NOT_DBP_OWNER	    E_PS0425_ILLEGAL_DBP_REF
**	E_PS042B_GRANTTAB_NOT_FOUND	    E_PS0422_GRANTTAB_NOT_FOUND
*/
#define			E_PS_NOT_OWNER	    (E_PS_ERRORS + 0x420L)
#define	E_PS0420_DROPOBJ_NOT_OBJ_OWNER	    (E_PS_NOT_OWNER + 0x0L)
#define	E_PS0421_CRTOBJ_NOT_OBJ_OWNER	    (E_PS_NOT_OWNER + 0x1L)
#define E_PS0422_GRANTTAB_NOT_FOUND	    (E_PS_NOT_OWNER + 0x2L)
#define E_PS0423_CRTIND_INSUF_PRIV	    (E_PS_NOT_OWNER + 0x3L)
#define E_PS0424_ILLEGAL_TBL_REF	    (E_PS_NOT_OWNER + 0x4L)
#define E_PS0425_ILLEGAL_DBP_REF	    (E_PS_NOT_OWNER + 0x5L)
#define E_PS0426_ILLEGAL_EVENT_REF	    (E_PS_NOT_OWNER + 0x6L)
#define	E_PS0427_ALTOBJ_NOT_OBJ_OWNER	    (E_PS_NOT_OWNER + 0x7L)
#define E_PS042A_CANT_GRANT_VIEW_PERM 	    (E_PS_NOT_OWNER + 0xAL)
#define E_PS042C_CANT_GRANT_DBP_PERM	    (E_PS_NOT_OWNER + 0xCL)
#define E_PS042E_DROPSYN_NOT_OBJ_OWNER	    (E_PS_NOT_OWNER + 0xEL)
#define E_PS042F_UNGRANTABLE_DBPROC	    (E_PS_NOT_OWNER + 0xFL)

/*
** errors having to do with synonyms
*/
#define			E_PS_SYN	    (E_PS_ERRORS + 0x450L)
#define	I_PS0451_SYN_WAS_SPECIFIED 	    (E_PS_SYN + 0x1L)
#define	E_PS0452_CANT_DROP_SYN		    (E_PS_SYN + 0x2L)
#define E_PS0453_SYN_ALREADY_EXISTS	    (E_PS_SYN + 0x3L)
#define E_PS0454_CREATE_SYN_ERROR	    (E_PS_SYN + 0x4L)
#define E_PS0455_CRT_SYN_ON_SYN		    (E_PS_SYN + 0x5L)
#define E_PS0456_DROP_SYN_ERROR             (E_PS_SYN + 0x6L)
#define E_PS0457_DROPSYN_NOT_SYN	    (E_PS_SYN + 0x7L)
#define E_PS0458_ULH_ALIAS_ERROR	    (E_PS_SYN + 0x8L)
#define E_PS0459_ILLEGAL_II_NAME	    (E_PS_SYN + 0x9L)


#define E_PS045F_WRONG_CASE		    (E_PS_SYN + 0xFL)

        /* a rule had a duplicate column in its UPDATE column list */
#define E_PS0460_DUP_UPD_COL                (E_PS_ERRORS + 0x460)

	/*
	** a <column reference> contained a qualifier which did not correspond
	** to any <table reference> within whose scope the <column reference>
	** was found
	*/
#define	E_PS0461_BAD_COL_REF_QUAL	    (E_PS_ERRORS + 0x461L)

/*
** errors introduced by FIPS 127-2 CREATE TABLE extensions (Entry Level SQL92)
**          (and some also used by ALTER TABLE, too (Intermediate Level SQL92))
*/
#define		    E_PS_FIPS127_2_CRTTBL   (E_PS_ERRORS + 0x470L)
	/* user tried to grant REFERENCES on a view */
#define	E_PS0470_REFERENCES_ON_VIEW	    (E_PS_FIPS127_2_CRTTBL + 0x00L)
	/*
	** A subselect or an aggregate function was encountered inside a CHECK
	** constrint of CREATE TABLE statement.
	*/
#define E_PS0471_CHECK_CONSTRAINT	    (E_PS_FIPS127_2_CRTTBL + 0x01L)
	/* Column CHECK constraint was referncing other column(s) */
#define E_PS0472_COL_CHECK_CONSTRAINT	    (E_PS_FIPS127_2_CRTTBL + 0x02L)
	/* Table CHECK constraint was referencing a nonexistent attribute */
#define E_PS0473_TBL_CHECK_CONSTRAINT       (E_PS_FIPS127_2_CRTTBL + 0x03L)
	/*
	** Table CHECK constraint was referencing an attribute of a table other
	** than the one being created inside a CHECK constraint
	*/
#define E_PS0474_CHECK_CONSTRAINT           (E_PS_FIPS127_2_CRTTBL + 0x04L)

#define E_PS0475_CONSTRAINT_NOT_ALLOWED	    (E_PS_FIPS127_2_CRTTBL + 0x05L)
#define E_PS0476_USER_DEFAULT_NOTALLOWED    (E_PS_FIPS127_2_CRTTBL + 0x06L)
#define E_PS0477_BAD_DEFAULTSYNTAX_SQL92    (E_PS_FIPS127_2_CRTTBL + 0x07L)
#define E_PS0478_BAD_DEFAULT_SYNTAX         (E_PS_FIPS127_2_CRTTBL + 0x08L)
#define E_PS0479_DUP_COL_QUAL	 	    (E_PS_FIPS127_2_CRTTBL + 0x09L)
#define E_PS047A_CONFLICT_COL_QUAL	    (E_PS_FIPS127_2_CRTTBL + 0x0AL)
#define E_PS047B_RESERVED_IDENT		    (E_PS_FIPS127_2_CRTTBL + 0x0BL)
#define E_PS047C_DUP_CONS_NAME		    (E_PS_FIPS127_2_CRTTBL + 0x0CL)
#define E_PS047D_2UNIQUE_CONS_CONFLICT	    (E_PS_FIPS127_2_CRTTBL + 0x0DL)
#define E_PS047E_1UNIQUE_CONS_CONFLICT	    (E_PS_FIPS127_2_CRTTBL + 0x0EL)
#define E_PS047F_0UNIQUE_CONS_CONFLICT	    (E_PS_FIPS127_2_CRTTBL + 0x0FL)
#define E_PS0480_UNIQUE_NOT_NULL	    (E_PS_FIPS127_2_CRTTBL + 0x10L)
#define E_PS0481_CONS_DUP_COL		    (E_PS_FIPS127_2_CRTTBL + 0x11L)
#define E_PS0482_2CONS_DUP_COL		    (E_PS_FIPS127_2_CRTTBL + 0x12L)
#define E_PS0483_CONS_NO_COL		    (E_PS_FIPS127_2_CRTTBL + 0x13L)
#define E_PS0484_REF_NUM_COL		    (E_PS_FIPS127_2_CRTTBL + 0x14L)
#define E_PS0485_REF_TYPE_RES		    (E_PS_FIPS127_2_CRTTBL + 0x15L)
#define E_PS0486_INVALID_CONS_TYPE	    (E_PS_FIPS127_2_CRTTBL + 0x16L)
#define E_PS0487_INVALID_BITMAP		    (E_PS_FIPS127_2_CRTTBL + 0x17L)
#define E_PS0488_DDB_CONSTRAINT_NOT_ALLOWED (E_PS_FIPS127_2_CRTTBL + 0x18L)
#define E_PS0489_DDB_USER_DEFAULT_NOT_ALLOWED (E_PS_FIPS127_2_CRTTBL + 0x19L)
#define	E_PS048A_NO_SUCH_CONSTRAINT	    (E_PS_FIPS127_2_CRTTBL + 0x1AL)
#define	E_PS048B_NULDEF_SPEC_IN_DBPPARM     (E_PS_FIPS127_2_CRTTBL + 0x1BL)
#define E_PS048C_NULDEF_SPEC_IN_DBPVAR      (E_PS_FIPS127_2_CRTTBL + 0x1CL)
#define E_PS048D_CONS_BADINDEX		    (E_PS_FIPS127_2_CRTTBL + 0x1DL)
#define E_PS048E_CONS_NOINDEX		    (E_PS_FIPS127_2_CRTTBL + 0x1EL)
#define E_PS048F_CONS_NOBASE		    (E_PS_FIPS127_2_CRTTBL + 0x1EF)
#define E_PS0490_NO_UNIQUE_CONSTRAINT	    (E_PS_FIPS127_2_CRTTBL + 0x20L)
#define E_PS0491_NO_PRIMARY_KEY		    (E_PS_FIPS127_2_CRTTBL + 0x21L)
#define E_PS0492_CHECK_CONS_TYPE_ERROR	    (E_PS_FIPS127_2_CRTTBL + 0x22L)
#define E_PS0493_DUPL_PRIMARY_KEY	    (E_PS_FIPS127_2_CRTTBL + 0x23L)
#define E_PS0494_CONS_NAME		    (E_PS_FIPS127_2_CRTTBL + 0x24L)
#define E_PS0495_DUPL_CONS_NAME		    (E_PS_FIPS127_2_CRTTBL + 0x25L)
#define E_PS0496_CANT_ALTER_VIEW	    (E_PS_FIPS127_2_CRTTBL + 0x26L)
#define E_PS0497_CONS_IXOPTS		    (E_PS_FIPS127_2_CRTTBL + 0x27L)
#define E_PS0498_DEFAULT_USER_SIZE	    (E_PS_FIPS127_2_CRTTBL + 0x28L)
#define E_PS0499_BAD_DEFAULT_VALUE	    (E_PS_FIPS127_2_CRTTBL + 0x29L)
#define E_PS049A_BAD_DEFAULT_SIZE	    (E_PS_FIPS127_2_CRTTBL + 0x2AL)
#define E_PS049B_BAD_DEFAULT_FUNC	    (E_PS_FIPS127_2_CRTTBL + 0x2BL)
#define E_PS049C_NO_DEFAULT_FROM_RDF	    (E_PS_FIPS127_2_CRTTBL + 0x2CL)
#define E_PS049D_RDF_CANT_READ_DEF	    (E_PS_FIPS127_2_CRTTBL + 0x2DL)
#define E_PS049E_CANT_EXEC_SYSGEN_PROC	    (E_PS_FIPS127_2_CRTTBL + 0x2EL)
#define E_PS049F_NO_PERIPH_DEFAULT	    (E_PS_FIPS127_2_CRTTBL + 0x2FL)
#define E_PS04A0_DEFAULT_TOO_LONG	    (E_PS_FIPS127_2_CRTTBL + 0x30L)
#define E_PS04A1_NO_REF_PRIV		    (E_PS_FIPS127_2_CRTTBL + 0x31L)
#define E_PS04A2_NO_REF_VIEW		    (E_PS_FIPS127_2_CRTTBL + 0x32L)
#define E_PS04A3_NO_GRANT		    (E_PS_FIPS127_2_CRTTBL + 0x33L)
#define E_PS04A4_INT_ERROR		    (E_PS_FIPS127_2_CRTTBL + 0x34L)
#define E_PS04A5_NO_COLUMNS		    (E_PS_FIPS127_2_CRTTBL + 0x35L)
#define E_PS04A6_UDT_NO_LONGTXT		    (E_PS_FIPS127_2_CRTTBL + 0x36L)
#define E_PS04A7_UDT_NO_GETEMPTY	    (E_PS_FIPS127_2_CRTTBL + 0x37L)
#define E_PS04A8_UDT_LTXT_CONVERSION	    (E_PS_FIPS127_2_CRTTBL + 0x38L)
#define E_PS04A9_NON_JOUR_REFING_JOUR_DB    (E_PS_FIPS127_2_CRTTBL + 0x39L)
#define E_PS04AA_NON_JOUR_REFD_JOUR_DB	    (E_PS_FIPS127_2_CRTTBL + 0x3AL)
#define	E_PS04AB_REF_CONS_JOUR_MISMATCH	    (E_PS_FIPS127_2_CRTTBL + 0x3BL)
#define	E_PS04AC_CONS_NON_JOR_TBL_JOR_DB    (E_PS_FIPS127_2_CRTTBL + 0x3CL)
#define	W_PS04AD_NON_JOUR_REFING_JOUR_DB    (E_PS_FIPS127_2_CRTTBL + 0x3DL)
#define W_PS04AE_NON_JOUR_REFD_JOUR_DB      (E_PS_FIPS127_2_CRTTBL + 0x3EL)
#define W_PS04AF_REF_CONS_JOUR_MISMATCH     (E_PS_FIPS127_2_CRTTBL + 0x3FL)
#define	W_PS04B0_CONS_NON_JOR_TBL_JOR_DB    (E_PS_FIPS127_2_CRTTBL + 0x40L)
#define	E_PS04B1_SET_JOUR_ON_REF_TBL	    (E_PS_FIPS127_2_CRTTBL + 0x41L)
#define	E_PS04B2_CONS_REFOPTS		    (E_PS_FIPS127_2_CRTTBL + 0x42L)
#define	E_PS04B3_BAD_SEQOP_DEFAULT	    (E_PS_ERRORS + 0x04B3L)
#define	E_PS04B4_DEFAULT_ANSIDT_IN_DBPROC   (E_PS_ERRORS + 0x04B4L)
#define	E_PS04B5_ONE_IDENTITY_COL	    (E_PS_ERRORS + 0x04B5L)
#define	E_PS04B6_ADD_IDENTITY_COL	    (E_PS_ERRORS + 0x04B6L)
#define	E_PS04B7_BAD_IDENTITY_TYPE	    (E_PS_ERRORS + 0x04B7L)
#define	E_PS04B8_IDENTITY_DEFAULT	    (E_PS_ERRORS + 0x04B8L)
#define	E_PS04B9_ALTER_IDENTITY		    (E_PS_ERRORS + 0x04B9L)
#define	E_PS04BA_BAD_OVERRIDING		    (E_PS_ERRORS + 0x04BAL)
#define	E_PS04BB_OVERRIDING_ALWAYS	    (E_PS_ERRORS + 0x04BBL)
#define	E_PS04BC_OVERRIDING_BYDEF	    (E_PS_ERRORS + 0x04BCL)

/*
**	Errors with CREATE SCHEMA
*/
#define			E_PS_SCHEMA	    (E_PS_ERRORS + 0x4C0L)

#define	E_PS04C0_BAD_SCHEMA_TREE	    (E_PS_SCHEMA + 0x00L)
#define	E_PS04C1_BAD_SCHEMA_NODE	    (E_PS_SCHEMA + 0x01L)
#define	E_PS04C2_BAD_SCHEMA_OBJECT	    (E_PS_SCHEMA + 0x03L)


#define E_PS04D0_GRANT_COPY_ON_VIEW         (E_PS_ERRORS + 0x4D0)
#define E_PS04D1_ALARM_INVALID_OBJECT       (E_PS_ERRORS + 0x4D1)
#define E_PS04D2_ALARM_NO_OBJECT_NAMES      (E_PS_ERRORS + 0x4D2)
#define E_PS04D3_ALARM_TOO_MANY_OBJS        (E_PS_ERRORS + 0x4D3)
#define E_PS04D4_ALARM_TOO_MANY_SUBJS       (E_PS_ERRORS + 0x4D4)
#define E_PS04D5_ALARM_OBJ_OPER_MISMATCH    (E_PS_ERRORS + 0x4D5)
/*
**	Errors having to do with shutting down the server.
*/
#define			E_PS_SHT	    (E_PS_ERRORS + 0x500L)
#define E_PS0501_SESSION_OPEN (E_PS_SHT + 0x1L) /* Session was open */
#define E_PS0502_LEFTOVER_MEM (E_PS_SHT + 0x2L) /* Unacccounted alloc. mem. */

/* errors having to do with processing of shareable QUEL repeat queries */
#define			E_PS_QLSHR     (E_PS_ERRORS + 0x520L)
				    /*
				    ** error occurred while building an ORDERED
				    ** list of table ids for a QUEL shared
				    ** repeat  query; this message will be
				    ** produced if while scanning first N
				    ** entries in the LRU range variable list,
				    ** we encounter an entry that was not used
				    ** (which would contradict one of the basic
				    ** assumptions behind the algorithm for
				    ** determining if a given QUEL repeat query
				    ** is shareable)
				    */
#define	E_PS0521_BAD_TBLID_LIST   (E_PS_QLSHR + 0x01L)
				    /*
				    ** conflict was detected while trying to use
				    ** PST_QRYHDR_INFO structure pointed to by
				    ** PST_QTREE.pst_info.
				    */
#define E_PS0522_QRYHDR_INFO_CONFLICT (E_PS_QLSHR + 0x02L)

/*
** errors introduced by FIPS 127-1 GRANT/REVOKE extensions
*/
#define		    E_PS_FIPS127_1_GRANT_REVOKE	    (E_PS_ERRORS + 0x550L)
    /* grantee was of type group or role and WITH GRANT OPTION was specified */
#define E_PS0550_GRANTEE_TYPE_FOR_WGO	(E_PS_FIPS127_1_GRANT_REVOKE + 0x00L)
    /*
    ** user cannot GRANT/CRERATE PERMIT on an object owned by another user
    ** because he lacks applicable privileges WGO on that object
    */
#define E_PS0551_INSUF_PRIV_GRANT_OBJ	(E_PS_FIPS127_1_GRANT_REVOKE + 0x01L)
    /*
    ** user cannot GRANT/CRERATE PERMIT on his view because he lacks SELECT WGO
    ** on another user's object used in the definition of the view
    */
#define E_PS0552_INSUF_PRIV_INDEP_OBJ	(E_PS_FIPS127_1_GRANT_REVOKE + 0x02L)
    /*
    ** user tried to grant INSERT, DELETE, or UPDATE on a view which is
    ** considered non-updateable because it is a multi-variable view or its
    ** definition involves aggregates, UNION or DISTINCT
    */
#define E_PS0553_NONUPDATEABLE_VIEW	(E_PS_FIPS127_1_GRANT_REVOKE + 0x03L)
    /*
    ** user tried to grant INSERT, DELETE, or UPDATE on a view which is defined
    ** over a non-updateable view
    */
#define E_PS0554_NONUPDT_VIEW_IN_DEFN	(E_PS_FIPS127_1_GRANT_REVOKE + 0x04L)
    /*
    ** User attempted to grant EXECUTE on his database procedure which is
    ** ungrantable because he lacks some privileges WITH GRANT OPTION on some
    ** table or view used in the definition of that database procedure or some
    ** other database procedure invoked by it.
    */
#define	E_PS0555_DBPGRANT_LACK_TBLPRIV	(E_PS_FIPS127_1_GRANT_REVOKE + 0x05L)
    /*
    ** User attempted to grant EXECUTE on his database procedure which is
    ** ungrantable because he lacks EXECUTE WITH GRANT OPTION on some database
    ** procedure used in the definition of that database procedure or some other
    ** database procedure invoked by the database procedure mentioned in the
    ** GRANT statement.
    */
#define	E_PS0556_DBPGRANT_LACK_DBPPRIV	(E_PS_FIPS127_1_GRANT_REVOKE + 0x06L)
    /*
    ** User attempted to grant EXECUTE on his database procedure which is
    ** ungrantable because he lacks some privileges WITH GRANT OPTION on some
    ** event used in the definition of that database procedure or some other
    ** database procedure invoked by it.
    */
#define E_PS0557_DBPGRANT_LACK_EVPRIV	(E_PS_FIPS127_1_GRANT_REVOKE + 0x07L)
    /*
    ** Attempt to reparse the text of a database procedure in order to determine
    ** if it is grantable resulted in an unexpected error
    */
#define E_PS0558_GRANT_CHECK_ERROR	(E_PS_FIPS127_1_GRANT_REVOKE + 0x08L)
    /*
    ** Attempt to reparse the text of a database procedure used in the
    ** definition of the database procedure on which the user wished to grant
    ** EXECUTE privilege resulted in an unexpected error
    */
#define	E_PS0559_CANT_GRANT_DUE_TO_ERR	(E_PS_FIPS127_1_GRANT_REVOKE + 0x09L)
    /*
    ** User attempted to grant EXECUTE on his database procedure which is
    ** ungrantable because it uses another database procedure owned by the
    ** current user which is ungrantable.
    */
#define E_PS055A_UNGRANTABLE_DBP	(E_PS_FIPS127_1_GRANT_REVOKE + 0x0AL)
    /*
    ** user attempted to create a database procedure which invokes (directly or
    ** through other procedure(s)) a dormant database procedure.
    */
#define E_PS055B_CREATE_DORMANT_DBP	(E_PS_FIPS127_1_GRANT_REVOKE + 0x0BL)
    /*
    ** While trying to recreate a database procedure on behalf of the user, we
    ** determined that it is indeed dormant.
    */
#define E_PS055C_RECREATE_DORMANT_DBP	(E_PS_FIPS127_1_GRANT_REVOKE + 0x0CL)
    /*
    ** Attempt to determine whether a database procedure used in the
    ** definition of the database procedure being created is dormant resulted in
    ** an unexpected error.
    */
#define E_PS055D_ERR_IN_CREATE_DBP	(E_PS_FIPS127_1_GRANT_REVOKE + 0x0DL)
    /*
    ** Attempt to determine whether a database procedure used in the definition
    ** of the database procedure being recreated is dormant resulted in an
    ** unexpected error.
    */
#define E_PS055E_ERR_IN_RECREATE_DBP	(E_PS_FIPS127_1_GRANT_REVOKE + 0x0EL)
    /*
    ** attempt to look up a dbproc by id failed 
    */
#define	E_PS055F_DBP_NOT_FOUND		(E_PS_FIPS127_1_GRANT_REVOKE + 0x0FL)
    /*
    ** user attempted to GRANT ALL [PRIVILEGES] on another user's table or view
    ** X but he possesses no grantable privileges on X
    */
#define	E_PS0560_NOPRIV_ON_GRANT_OBJ	(E_PS_FIPS127_1_GRANT_REVOKE + 0x10L)
    /*
    ** User attempted to grant ALL [PRIVILEGES] on his view V, but he possesses
    ** no grantable privileges on another user's object X which is (in SQL92
    ** parlance) a leaf underlying object of the <query expression> of V
    */
#define	E_PS0561_NOPRIV_UNDERLYING_OBJ	(E_PS_FIPS127_1_GRANT_REVOKE + 0x11L)
    /*
    ** User attempted to grant ALL [PRIVILEGES] on his view V, but he lacks a
    ** grantable SELECT privilege on another user's object X which is used in
    ** V's definition.  This could happen if V is defined on top of X and the
    ** current user possesses no grantable privileges on X or if the current
    ** user possesses only grantable SELECT on the object on which V is defined
    ** but does not possess grantable SELECT on X which is used in V's
    ** definition
    */
#define	E_PS0562_NOPRIV_LACKING_SELECT	(E_PS_FIPS127_1_GRANT_REVOKE + 0x12L)
    /*
    ** User attempted to grant ALL [PRIVILEGES] on another user's dbevent X even
    ** though the user possesses no grantable privileges on X
    */
#define	E_PS0563_NOPRIV_ON_GRANT_EV	(E_PS_FIPS127_1_GRANT_REVOKE + 0x13L)
    /*
    ** user attempted to drop all permits on his object, but there exists some
    ** permit or object that depends on privilege(s) being destroyed.
    */
#define	E_PS0564_ALLPROT_ABANDONED_OBJ	(E_PS_FIPS127_1_GRANT_REVOKE + 0x14L)
    /*
    ** user attempted to drop a permit on his object, but there exists some
    ** permit or object that depends on privilege being destroyed.
    */
#define	E_PS0565_PROT_ABANDONED_OBJ	(E_PS_FIPS127_1_GRANT_REVOKE + 0x15L)
    /*
    ** user attempted to revoke privilege(s) from PUBLIC in RESTRICTed mode, but
    ** this would render some objects and/or privileges abandoned and will be
    ** disallowed
    */
#define	E_PS0566_REVOKE_FROM_PUBLIC_ERR	(E_PS_FIPS127_1_GRANT_REVOKE + 0x16L)
    /*
    ** user attempted to revoke privilege(s) from another user in RESTRICTed
    ** mode, but this would render some objects and/or privileges abandoned and
    ** will be disallowed
    */
#define	E_PS0567_REVOKE_FROM_USER_ERR	(E_PS_FIPS127_1_GRANT_REVOKE + 0x17L)

/*
** Errors having to do with processing of DROP SCHEMA statement
*/
#define		    E_PS_DROP_SCHEMA		    (E_PS_ERRORS + 0x570L)
    /*
    ** user attempted to drop a non-existent schema
    */
#define	E_PS0570_NO_SCHEMA_TO_DROP	(E_PS_DROP_SCHEMA + 0x00L)
    /*
    ** user attempted a restricted destruction of schema that contains at least
    ** one table, view, dbevent, or dbproc
    */
#define	E_PS0571_NONEMPTY_SCHEMA	(E_PS_DROP_SCHEMA + 0x01L)
    /*
    ** user attempted to drop a schema "$ingres" owned by $ingres which is
    ** illegal 
    */
#define E_PS0572_CANT_DROP_SYSCAT_SCHEMA    (E_PS_DROP_SCHEMA + 0x02L)
    /*
    ** user attempted to drop a schema owned by a different user
    */
#define E_PS0573_CANT_DROP_OTHERS_SCHEMA    (E_PS_DROP_SCHEMA + 0x03L)

/*
** errors and warnings introduced by FIPS 127-2 WITH CHECK OPTION project
*/
#define		    E_PS_FIPS127_2_CHECK_OPTION	    (E_PS_ERRORS + 0x580L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving UNION 
    */
#define W_PS0580_WCO_AND_UNION		(E_PS_FIPS127_2_CHECK_OPTION + 0x00L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving DISTINCT
    ** in the outermost subselect
    */
#define W_PS0581_WCO_AND_DISTINCT   	(E_PS_FIPS127_2_CHECK_OPTION + 0x01L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a multi-variable view
    */
#define W_PS0582_WCO_AND_MULT_VARS	(E_PS_FIPS127_2_CHECK_OPTION + 0x02L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving GROUP BY
    ** in the outermost subselect
    */
#define W_PS0583_WCO_AND_GROUP_BY	(E_PS_FIPS127_2_CHECK_OPTION + 0x03L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving HAVING in
    ** the outermost subselect
    */
#define W_PS0584_WCO_AND_HAVING		(E_PS_FIPS127_2_CHECK_OPTION + 0x04L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving set
    ** function (count, avg, sum, etc.) in the outermost subselect
    */
#define W_PS0585_WCO_AND_SET_FUNCS	(E_PS_FIPS127_2_CHECK_OPTION + 0x05L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view whose underlying
    ** view involved UNION 
    */
#define W_PS0586_WCO_AND_UNION_IN_UV	(E_PS_FIPS127_2_CHECK_OPTION + 0x06L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view whose underlying
    ** view involved DISTINCT in the outermost subselect
    */
#define W_PS0587_WCO_AND_DISTINCT_IN_UV	(E_PS_FIPS127_2_CHECK_OPTION + 0x07L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a multi-variable view
    */
#define W_PS0588_WCO_AND_MULT_VARS_IN_UV (E_PS_FIPS127_2_CHECK_OPTION + 0x08L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving GROUP BY
    ** in the outermost subselect
    */
#define W_PS0589_WCO_AND_GROUP_BY_IN_UV	(E_PS_FIPS127_2_CHECK_OPTION + 0x09L)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving HAVING in
    ** the outermost subselect
    */
#define W_PS058A_WCO_AND_HAVING_IN_UV	(E_PS_FIPS127_2_CHECK_OPTION + 0x0AL)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view involving set
    ** function (count, avg, sum, etc.) in the outermost subselect
    */
#define W_PS058B_WCO_AND_SET_FUNCS_IN_UV (E_PS_FIPS127_2_CHECK_OPTION + 0x0BL)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view which is updateable
    ** (according to INGRES/SQL but not SQL92) whose leaf generally underlying
    ** table also is involved in the view's WHERE-clause
    */
#define	W_PS058C_WCO_AND_LUT_IN_WHERE_CLAUSE (E_PS_FIPS127_2_CHECK_OPTION + 0x0CL)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view which is not
    ** updateable because it contains no updateable columns
    */
#define W_PS058D_WCO_AND_NO_UPDATABLE_COLS (E_PS_FIPS127_2_CHECK_OPTION + 0x0DL)
    /*
    ** user attempted to specify WITH CHECK OPTION on a view deemed
    ** non-updateable for none of the above reasons - ideally, this will not be
    ** seen by a user
    */
#define W_PS058F_WCO_AND_UNSPEC_REASON	(E_PS_FIPS127_2_CHECK_OPTION + 0x0FL)
    /*
    ** user attempted to create a rule on a view whose definition involved UNION
    */
#define E_PS0590_UNION_IN_RULE_VIEW	(E_PS_FIPS127_2_CHECK_OPTION + 0x10L)
    /*
    ** user attempted to create a rule on a view whose definition involved
    ** DISTINCT in the outermost subselect
    */
#define E_PS0591_DISTINCT_IN_RULE_VIEW   (E_PS_FIPS127_2_CHECK_OPTION + 0x11L)
    /*
    ** user attempted to create a rule on a multi-variable view
    */
#define E_PS0592_MULT_VARS_IN_RULE_VIEW	(E_PS_FIPS127_2_CHECK_OPTION + 0x12L)
    /*
    ** user attempted to create a rule on a view whose definition involved
    ** GROUP BY in the outermost subselect
    */
#define E_PS0593_GROUP_BY_IN_RULE_VIEW	(E_PS_FIPS127_2_CHECK_OPTION + 0x13L)
    /*
    ** user attempted to create a rule on a view whose definition involved
    ** a set function (count, avg, sum, etc.) in the outermost subselect
    */
#define E_PS0594_AGGR_IN_RULE_VIEW	(E_PS_FIPS127_2_CHECK_OPTION + 0x14L)
    /*
    ** user attempted to create a rule on a view having whose attributes
    ** contained no references to the attributes of the underlying table (i.e.
    ** DELETE/INSERT/UPDATE cannot be specified for such view)
    */
#define	E_PS0595_CANNOT_UPDT_RULE_VIEW	(E_PS_FIPS127_2_CHECK_OPTION + 0x15L)
    /*
    ** user attempted to create a rule on a view whose underlying view's
    ** definition involved UNION
    */
#define E_PS0596_CRT_RULE_UNION_IN_UV	(E_PS_FIPS127_2_CHECK_OPTION + 0x16L)
    /*
    ** user attempted to create a rule on a view whose underlying view's
    ** definition involved DISTINCT in the outermost subselect
    */
#define E_PS0597_CRT_RULE_DISTINCT_IN_UV   (E_PS_FIPS127_2_CHECK_OPTION + 0x17L)
    /*
    ** user attempted to create a rule on a view based on a multi-variable view
    */
#define E_PS0598_CRT_RULE_MULT_VARS_IN_UV  (E_PS_FIPS127_2_CHECK_OPTION + 0x18L)
    /*
    ** user attempted to create a rule on a view whose underlying view's
    ** definition involved GROUP BY in the outermost subselect
    */
#define E_PS0599_CRT_RULE_GROUP_BY_IN_UV   (E_PS_FIPS127_2_CHECK_OPTION + 0x19L)
    /*
    ** user attempted to create a rule on a view whose underlying view's
    ** definition involved a set function (count, avg, sum, etc.) in the
    ** outermost subselect
    */
#define E_PS059A_CRT_RULE_AGGR_IN_UV	(E_PS_FIPS127_2_CHECK_OPTION + 0x1AL)
    /*
    ** user attempted to create a rule on a view whose underlying view's
    ** attributes contained no references to the attributes of the underlying
    ** table (i.e. DELETE/INSERT/UPDATE cannot be specified for such view)
    */
#define	E_PS059B_CRT_RULE_CANNOT_UPDT_UV   (E_PS_FIPS127_2_CHECK_OPTION + 0x1BL)
    /*
    ** a query involved a reference to the TID attribute of a non-updatable
    ** view; for updatable views, such references get translated into references
    ** to the view's underlying base table, but they are illegal for
    ** non-updatable views.
    */
#define	E_PS059C_TID_REF_IN_NONUPDT_VIEW   (E_PS_FIPS127_2_CHECK_OPTION + 0x1CL)
/*
**	Errors having to do with ending a session.
*/
#define			E_PS_END	    (E_PS_ERRORS + 0x600L)
#define E_PS0601_CURSOR_OPEN	(E_PS_END + 0x1L) /* Cursor was open */
#define E_PS0602_CORRUPT_MEM 	(E_PS_END + 0x2L) /* Memory is corrupted */

/*
**	Errors having to do with starting up the server.
*/
#define			E_PS_STR	    (E_PS_ERRORS + 0x700L)
#define E_PS0701_SRV_ALREADY_UP (E_PS_STR + 0x1L) /* Server already started */
#define E_PS0702_NO_MEM         (E_PS_STR + 0x2L) /* Can't allocate pool */
#define E_PS0703_RDF_ERROR      (E_PS_STR + 0x3L) /* RDF failure */
#define E_PS0704_INSF_MEM       (E_PS_STR + 0x4L) /* Insufficient sess memory */

/*
**	It is possible to get an error by having an inappropriate word following
**	a good statement.  There is one of these errors for each statement type.
**	The error number is formed by adding the query mode to the base error
**	number for all of these statements.
*/
#define			E_PS_WRD	    (E_PS_ERRORS + 0x800L)
#define E_PS0800_UNAPT_WORD (E_PS_WRD + 0x0L)

/*
**	Errors having to do with getting table descriptions.
*/
#define                 E_PS_TBL        (E_PS_ERRORS + 0x900L)
#define E_PS0901_REDESC_RNG (E_PS_TBL + 0x1L)/* Attempt to re-descr. rng var.*/
#define E_PS0902_BAD_DESCTYPE (E_PS_TBL + 0x2L) /* Bad description type req. */
#define E_PS0903_TAB_NOTFOUND (E_PS_TBL + 0x3L) /* Table not found */
#define E_PS0904_BAD_RDF_GETDESC (E_PS_TBL + 0x4L) /* Bad RDF getdesc operation */
#define E_PS0905_DBP_NOTFOUND (E_PS_TBL + 0x5L) /* dbproc not found */
#define E_PS0906_TOO_MANY_COLS (E_PS_TBL + 0x6L) /* Too many result cols. */
#define E_PS0907_CANT_FREE_DESC (E_PS_TBL + 0x7L) /* Can't free descriptor */
#define	E_PS0908_RANGE_VAR_MISSING (E_PS_TBL + 0x8L) /* Can't find rng var */
#define	E_PS0909_NO_RANGE_ENTRY (E_PS_TBL + 0x9L) /* Can't find rng var */
#define	E_PS090A_NO_ATTR_ENTRY (E_PS_TBL + 0xAL) /* Can't find attr desc */
#define	E_PS090B_TABLE_EXISTS (E_PS_TBL + 0xBL) /* Table already exists */
#define	E_PS090C_NONEXISTENT_TABLE (E_PS_TBL + 0xCL) /* Table doesn't exist */
#define E_PS090D_NO_LDB_TABLE (E_PS_TBL + 0xDL) /* LDB Table doesn't exist */
						/*
						** # cols in LDB table !=
						** # cols in LINK object
						*/
#define E_PS090E_COL_MISMATCH (E_PS_TBL + 0xEL)
						/*
						** If user doesn't specify
						** LINK's column names, we try
						** to use those of the LDB
						** table.  If they are found to
						** be invalid INGRES names, this
						** msg will be produced.
						*/
#define E_PS090F_BAD_COL_NAME (E_PS_TBL + 0xFL)
#define E_PS0910_DUP_LDB_TABLE (E_PS_TBL + 0x10L)
#define E_PS0911_BAD_LDB_USER (E_PS_TBL + 0x11L)
#define E_PS0912_MISSING_LDB_TBL (E_PS_TBL + 0x12L)
#define E_PS0913_SCHEMA_MISMATCH (E_PS_TBL + 0x13L)
#define W_PS0914_MISSING_LDB_TBL (E_PS_TBL + 0x14L)
#define E_PS0915_MISSING_LDB_TBL (E_PS_TBL + 0x15L)
#define E_PS0916_USER_NOT_OWNER  (E_PS_TBL + 0x16L)
#define E_PS0917_CANNOT_CONNECT  (E_PS_TBL + 0x17L)
#define E_PS0918_MULTI_SITE_DROP (E_PS_TBL + 0x18L)
#define E_PS0919_BAD_CATALOG_ENTRY (E_PS_TBL + 0x19L)
#define E_PS091A_CANNOT_CONNECT    (E_PS_TBL + 0x1AL)
#define E_PS091B_NO_CONNECTION   (E_PS_TBL + 0x1BL)
#define E_PS091C_NOPARAMS_ALLOWED (E_PS_TBL + 0x1CL)
#define E_PS091D_PROC_MAPPED_COLS (E_PS_TBL + 0x1DL)
#define E_PS091E_INCOMPAT_CASE_MAPPING (E_PS_TBL + 0x1EL)
#define E_PS091F_STAR_NO_FIRSTN	(E_PS_TBL + 0x1FL)

/*
**      Errors having to do with memory allocation.
*/
#define			E_PS_MEMORY	    (E_PS_ERRORS + 0xA00L)
#define E_PS0A01_BADBLKSIZE   (E_PS_MEMORY + 0x1L) /* Bad block size */
#define E_PS0A02_BADALLOC     (E_PS_MEMORY + 0x2L) /* Couldn't alloc memory */
#define E_PS0A03_BADMEMTYPE   (E_PS_MEMORY + 0x3L) /* Bad memory type */
#define E_PS0A04_REQ_GT_BLOCK (E_PS_MEMORY + 0x4L) /* Request > block size */
#define E_PS0A05_BADMEMREQ    (E_PS_MEMORY + 0x5L) /* Bad memory request */
#define E_PS0A06_CANTFREE     (E_PS_MEMORY + 0x6L) /* Can't free memory */
#define E_PS0A07_CANTCHANGE   (E_PS_MEMORY + 0x7L) /* Can't change obj.type */
#define E_PS0A08_CANTLOCK     (E_PS_MEMORY + 0x8L) /* Can't lock object */
#define E_PS0A09_CANTDESTROY  (E_PS_MEMORY + 0x9L) /* Can't destroy object */
#define E_PS0A0A_CANTGETINFO  (E_PS_MEMORY + 0xAL) /* Can't get object info */
#define E_PS0A0B_CANTGETHNDLE (E_PS_MEMORY + 0xBL) /* Can't get object handle */
						   /*
						   ** can't create a QP alias
						   */
#define E_PS0A0C_CANT_CREATE_ALIAS (E_PS_MEMORY + 0xCL) 
#define E_PS0A0D_ULM_ERROR	(E_PS_MEMORY + 0xDL) /* Non-nomem ULM error */

/*
**	Errors having to do with the parse operation.
*/
#define			E_PS_PARSE	    (E_PS_ERRORS + 0xB00L)
#define E_PS0B04_CANT_GET_TEXT (E_PS_PARSE + 0x4L) /* Can't get query text */
#define E_PS0B05_CANT_UNLOCK   (E_PS_PARSE + 0x5L) /* Can't unlock QSF obj. */
					    /* Can't find corresponding WHILE */
#define E_PS0B06_NOWHILE       (E_PS_PARSE + 0x6L) 

/* errors discovered while parsing COMMENT ON */
#define			E_PS_COMMENT	    (E_PS_PARSE + 0xA0L)
#define E_PS0BA0_SHORT_REMARK_TOO_LONG	    (E_PS_COMMENT + 0x00L)
#define E_PS0BA1_LONG_REMARK_TOO_LONG	    (E_PS_COMMENT + 0x01L)
#define E_PS0BA2_NO_REMARK_SPECIFIED	    (E_PS_COMMENT + 0x02L)
#define E_PS0BA3_NONEXISTENT_COLUMN	    (E_PS_COMMENT + 0x03L)
#define	E_PS0BA4_TABLE_COMMENT_ERROR	    (E_PS_COMMENT + 0x04L)
#define E_PS0BA5_COLUMN_COMMENT_ERROR	    (E_PS_COMMENT + 0x05L)

#define E_PS0BAF_SHORT_REM_UNSUPPORTED	    (E_PS_COMMENT + 0x0FL)

/*
**	Errors having to do with table/index WITH options
*/
#define			E_PS_WITH_OPTIONS	(E_PS_ERRORS + 0xBB0L)
#define E_PS0BB0_UNIQ_SCOPE_NOT_ALLOWED		(E_PS_WITH_OPTIONS + 0x00L)
#define E_PS0BB1_UNIQ_SCOPE_SQL_ONLY		(E_PS_WITH_OPTIONS + 0x01L)
#define E_PS0BB2_UNIQUE_REQUIRED		(E_PS_WITH_OPTIONS + 0x02L)
#define E_PS0BB3_INVALID_UNIQUE_SCOPE		(E_PS_WITH_OPTIONS + 0x03L)
#define E_PS0BB4_CANT_CHANGE_UNIQUE_CONS	(E_PS_WITH_OPTIONS + 0x04L)
#define E_PS0BB5_NO_PERSIST_TABLE		(E_PS_WITH_OPTIONS + 0x05L)
#define E_PS0BB6_PERSISTENT_CONSTRAINT		(E_PS_WITH_OPTIONS + 0x06L)

/* errors having to do with Btree compression */
#define			E_PS_BTREE_COMPRESS (E_PS_PARSE + 0xC0L)
#define E_PS0BC0_BTREE_INDEX_NO_DCOMP		(E_PS_BTREE_COMPRESS + 0x00L)
#define E_PS0BC1_KCOMP_BTREE_ONLY		(E_PS_BTREE_COMPRESS + 0x01L)
#define E_PS0BC2_COMP_NOT_ALLOWED		(E_PS_BTREE_COMPRESS + 0x02L)
#define E_PS0BC3_COMP_NOT_IN_RELOC		(E_PS_BTREE_COMPRESS + 0x03L)
#define E_PS0BC4_COMPRESSION_TWICE		(E_PS_BTREE_COMPRESS + 0x04L)
#define E_PS0BC5_KEY_OR_DATA_ONLY		(E_PS_BTREE_COMPRESS + 0x05L)
#define E_PS0BC6_BAD_COMP_CLAUSE		(E_PS_BTREE_COMPRESS + 0x06L)
#define E_PS0BC7_NO_STRUCTURE			(E_PS_BTREE_COMPRESS + 0x07L)
#define E_PS0BC8_COMP_NO_STRUCTURE		(E_PS_BTREE_COMPRESS + 0x08L)
#define E_PS0BC9_DUPLICATE_WITH_CLAUSE		(E_PS_BTREE_COMPRESS + 0x09L)
#define E_PS0BCA_RTREE_INDEX_NO_COMP		(E_PS_BTREE_COMPRESS + 0x0AL)
#define E_PS0BCB_RTREE_INDEX_NO_UNIQUE		(E_PS_BTREE_COMPRESS + 0x0BL)
#define E_PS0BCC_RTREE_INDEX_ONE_KEY		(E_PS_BTREE_COMPRESS + 0x0CL)
#define E_PS0BCD_RTREE_INDEX_NEED_RANGE		(E_PS_BTREE_COMPRESS + 0x0DL)
#define E_PS0BCE_RTREE_INDEX_BAD_KEY		(E_PS_BTREE_COMPRESS + 0x0EL)
#define E_PS0BCF_RTREE_INDEX_BAD_RANGE		(E_PS_BTREE_COMPRESS + 0x0FL)

/*
**	Errors having to do with temporary tables:
*/
#define			E_PS_TEMP_TABLES	(E_PS_ERRORS + 0xBD0L)
#define	E_PS0BD0_MUST_USE_NORECOV	    	(E_PS_TEMP_TABLES   + 0x00L)
#define	E_PS0BD1_MUST_USE_SESSION	    	(E_PS_TEMP_TABLES   + 0x01L)
#define	E_PS0BD2_NOT_SUPP_ON_TEMP		(E_PS_TEMP_TABLES   + 0x02L)
#define	E_PS0BD3_ONLY_ON_TEMP			(E_PS_TEMP_TABLES   + 0x03L)
#define	E_PS0BD4_TTBL_NOT_IN_VIEW		(E_PS_TEMP_TABLES   + 0x04L)
#define	E_PS0BD5_TTBL_NOT_IN_DBP		(E_PS_TEMP_TABLES   + 0x05L)
#define	E_PS0BD6_MUST_USE_ON_COMMIT		(E_PS_TEMP_TABLES   + 0x06L)
#define E_PS0BD7_SESSION_EV_ILLEGAL		(E_PS_TEMP_TABLES   + 0x07L)
#define E_PS0BD8_SESSION_INDEX_ILLEGAL		(E_PS_TEMP_TABLES   + 0x08L)
#define E_PS0BD9_SESSION_DBP_ILLEGAL		(E_PS_TEMP_TABLES   + 0x09L)
#define E_PS0BDA_SESSION_SEQ_ILLEGAL		(E_PS_TEMP_TABLES   + 0x0AL)

/*
** Errors having to do with number of columns in and width of tables/views
*/
#define			E_PS_TABLE_VIEW_LIMITS	(E_PS_ERRORS + 0xBE0L)
#define	E_PS0BE0_TOO_MANY_COLS			(E_PS_TABLE_VIEW_LIMITS + 0x00L)
#define E_PS0BE1_TUPLE_TOO_WIDE			(E_PS_TABLE_VIEW_LIMITS + 0x01L)

/*
**	Errors having to do with datatype resolution.
*/
#define			E_PS_DT		    (E_PS_ERRORS + 0xC00L)
#define E_PS0C02_NULL_PTR      (E_PS_DT + 0x2L) /* NULL pointer as argument */
#define E_PS0C03_BAD_NODE_TYPE (E_PS_DT + 0x3L) /* Node is not operator node */
#define E_PS0C04_BAD_TREE      (E_PS_DT + 0x4L) /* Bad query tree */
#define E_PS0C05_BAD_ADF_STATUS (E_PS_DT + 0x5L) /* Error calling ADF */
#define E_PS0C06_BAD_STMT      (E_PS_DT + 0x6L) /* Invalid node in stmt list */

/*
**	Errors having to do with parsing decimal() conversion function.
*/
#define	E_PS0C80_NO_DEFLT_ON_DT	(E_PS_DT + 0x80L)  /* can't dflt prec/scale */
#define	E_PS0C81_MUST_BE_CONST	(E_PS_DT + 0x81L)  /* parm must be constant */
#define	E_PS0C82_BAD_PRECISION	(E_PS_DT + 0x82L)  /* prec is out of range  */
#define	E_PS0C83_BAD_SCALE	(E_PS_DT + 0x83L)  /* scale is out of range */
						/*
						** three parms passed to a
						** function other than decimal()
						*/
#define E_PS0C84_TERNARY_FUNC  (E_PS_DT + 0x84L)
#define E_PS0C85_ENCRYPT_INVALID  (E_PS_DT + 0x85L)
#define E_PS0C86_ENCRYPT_NOTABLVL (E_PS_DT + 0x86L)
#define E_PS0C87_ENCRYPT_NOCOLS   (E_PS_DT + 0x87L)
#define E_PS0C88_ENCRYPT_NOPASS   (E_PS_DT + 0x88L)

/*
**     Errors having to do with parsing string functions 
*/     
#define E_PS0C90_MUSTBE_CONST_INT  (E_PS_DT + 0x90L)
#define E_PS0C91_MUSTBE_NUMERIC  (E_PS_DT + 0x91L)

/*
**	Errors having to do with qrymod operations.
*/
#define                 E_PS_QYM        (E_PS_ERRORS + 0xD00L)
#define E_PS0D01_BAD_TREE_IN_VFIND (E_PS_QYM + 0x1L) /* Bad tree in psy_vfind */
#define E_PS0D02_BAD_TID_NODE (E_PS_QYM + 0x2L) /* Bad TID node found */
#define E_PS0D03_ATT_NOT_FOUND (E_PS_QYM + 0x3L) /* Att not found in view tree*/
#define E_PS0D05_NULL_TREE (E_PS_QYM + 0x5L) /* Null tree found in psy_apql */
#define E_PS0D06_QUAL_NOT_NORML (E_PS_QYM + 0x6L) /* Un-normalized qual found */
#define E_PS0D07_VARNO_OUT_OF_RANGE (E_PS_QYM + 0x7L) /* Bad varno found */
#define E_PS0D08_RNGVAR_UNUSED (E_PS_QYM + 0x8L) /*Attempt to use undef rngvar*/
#define E_PS0D09_MRG_DIFF_TABS (E_PS_QYM + 0x9L) /* Merge different tables */
#define E_PS0D0A_CANT_GET_EMPTY (E_PS_QYM + 0x0AL) /* Error from adc_getempty */
#define E_PS0D0B_RESDOM_EXPECTED (E_PS_QYM + 0x0BL) /* Non-resdom node found */
#define E_PS0D0C_NOT_RESDOM (E_PS_QYM + 0x0CL) /* Non-resdom node found */
#define E_PS0D0D_NULL_LHS (E_PS_QYM + 0x0DL) /* Null target list */
#define E_PS0D0E_BAD_QMODE (E_PS_QYM + 0x0EL) /* Bad query mode */
#define E_PS0D0F_BAD_AGGREGATE (E_PS_QYM + 0xFL) /* Bad node in aggregate */
#define E_PS0D10_BAD_QUALIFICATION (E_PS_QYM + 0x10L) /* Bad node in qual */
#define E_PS0D11_NULL_TREE (E_PS_QYM + 0x11L) /* Unexpected NULL query tree */
#define E_PS0D12_BYHEAD_OR_AOP (E_PS_QYM + 0x12L) /* Unexpected AGHEAD or AOP */
#define E_PS0D13_SCU_INFO_ERR (E_PS_QYM + 0x13L) /* Error on SCU_INFORMATION */
#define E_PS0D14_VIEW_CLEANUP (E_PS_QYM + 0x14L) /* Err cleaning up view def. */
#define E_PS0D15_RDF_GETINFO (E_PS_QYM + 0x15L) /* Err from RDF GETINFO */
#define E_PS0D16_RDF_UPDATE (E_PS_QYM + 0x16L) /* Err from RDF UPDATE */
#define E_PS0D17_RDF_UNFIX (E_PS_QYM + 0x17L) /* Error from RDF UNFIX */
#define E_PS0D18_QSF_LOCK (E_PS_QYM + 0x18L) /* Error getting QSF lock */
#define E_PS0D19_QSF_INFO (E_PS_QYM + 0x19L) /* Error getting QSF info */
#define E_PS0D1A_QSF_DESTROY (E_PS_QYM + 0x1AL) /* Error destroying QSF object*/
#define E_PS0D1B_PERMIT_CLEANUP (E_PS_QYM + 0x1BL) /* Err cleaning up pmt. def*/
#define E_PS0D1C_QSF_UNLOCK (E_PS_QYM + 0x1CL) /* Error unlocking QSF object */
#define E_PS0D1D_BAD_FUNC_NAME (E_PS_QYM + 0x1DL) /* Bad aggregate name */
#define E_PS0D1E_QEF_ERROR (E_PS_QYM + 0x1EL) /* QEF error while defining intg*/
#define E_PS0D1F_OPF_ERROR (E_PS_QYM + 0x1FL) /* OPF error while defining intg*/
#define E_PS0D20_QEF_ERROR (E_PS_QYM + 0x20L) /* QEF error destroying view */
#define E_PS0D21_QEF_ERROR (E_PS_QYM + 0x21L) /* QEF error destroying table */
#define E_PS0D22_QEF_ERROR (E_PS_QYM + 0x22L) /* QEF error tran. related*/
#define E_PS0D23_QEF_ERROR (E_PS_QYM + 0x23L) /* QEF error resource related*/
#define E_PS0D24_BGN_TX_ERR (E_PS_QYM + 0x24L) /* QEF error beginning tx */
#define E_PS0D25_CMT_TX_ERR (E_PS_QYM + 0x25L) /* QEF error commiting tx */
#define E_PS0D26_ABT_TX_ERR (E_PS_QYM + 0x26L) /* QEF error aborting tx */
#define E_PS0D27_RDF_INITIALIZE (E_PS_QYM + 0x27L) /* RDF error initializing */
#define E_PS0D28_RDF_TERMINATE (E_PS_QYM + 0x28L) /* RDF error terminating */
#define E_PS0D29_RDF_INVALIDATE (E_PS_QYM + 0x29L) /* RDF error invalidating */
#define E_PS0D2A_RDF_READTUPLES (E_PS_QYM + 0x2AL) /* RDF error reading tuples*/
#define E_PS0D30_UNEXPECTED_NODE (E_PS_QYM + 0x30L) /* Unexpected node */
#define E_PS0D31_NO_TUPLES	 (E_PS_QYM + 0x31L) /* RDF returned no tuples */
#define E_PS0D32_RDF_INV_NODE	 (E_PS_QYM + 0x32L) /* RDF returned invalid
						    ** pst_len value.
						    */
#define E_PS0D33_RULE_PARAM   (E_PS_QYM + 0x33L) /* Bad param list in rule */

#define	E_PS0D34_EVENT_ABSENT_PRIV (E_PS_QYM + 0x34L) /* No such event */
#define	E_PS0D35_EVENT_GRANT_REVOKE (E_PS_QYM + 0x35L) /* Invalid event perm */
#define	E_PS0D36_EVENT_TEXT_QUOTE (E_PS_QYM + 0x36L) /* RAISE EV syntax err */
#define	E_PS0D37_EVENT_TEXT_TYPE (E_PS_QYM + 0x37L) /* RAISE EV syntax err */
#define	E_PS0D38_EVENT_WITH (E_PS_QYM + 0x38L) /* RAISE EV syntax err */
#define	E_PS0D39_EVENT_GET (E_PS_QYM + 0x39L) /* GET EV internal error */
#define E_PS0D3B_DBEVENT_NOT_FOUND (E_PS_QYM + 0x3BL) /* dbevent not found */
#define E_PS0D3C_MISSING_DBEVENT_PRIV (E_PS_QYM + 0x3CL) 
					/* insufficiend dbevent privileges */
#define E_PS0D3D_MISSING_SEQUENCE_PRIV (E_PS_QYM + 0x3DL) 
					/* insufficiend sequence privileges */

#define E_PS0D40_INVALID_GRPID_OP (E_PS_QYM + 0x40L) /* Invalid group op */
#define E_PS0D41_INVALID_APLID_OP (E_PS_QYM + 0x41L) /* Invalid aplid op */
#define E_PS0D42_INVALID_GRANTEE_TYPE (E_PS_QYM + 0x42L) /* Invalid grantee */
#define E_PS0D43_XENCODE_ERROR	  (E_PS_QYM + 0x43L) /* SCU_XENCODE error */
#define E_PS0D44_SCU_IDEF_ERR     (E_PS_QYM + 0x44L) /* Error on SCU_IDEFINE */
#define E_PS0D45_INVALID_GRANT_OP (E_PS_QYM + 0x45L) /* Invalid psy_grant op */
#define E_PS0D46_INVALID_USER_OP  (E_PS_QYM + 0x46L) /* Invalid psy_usflag op */
#define E_PS0D47_INVALID_LOC_OP   (E_PS_QYM + 0x47L) /* Invalid psy_loflag op */
#define E_PS0D48_INVALID_AUDIT_OP (E_PS_QYM + 0x48L) /* Invalid psy_auflag op */
#define E_PS0D49_INVALID_ALARM_OP (E_PS_QYM + 0x49L) /* Invalid psy_alflag op */
#define E_PS0D50_INVALID_GRANTEE_TYPE (E_PS_QYM + 0x50L) /* Invalid grantee */

/*
**  Errors associated with Dynamic SQL.
*/
#define	E_PS_DSQL   (E_PS_ERRORS + 0x0E00L)
/* statement name not found */
#define	E_PS0E01_STMT_NFND	(E_PS_DSQL + 0x01L)
/* a select statement not allowed for EXECUTE */
#define E_PS0E02_NOEXEC		(E_PS_DSQL + 0x02L)
/* Error calling pst_rgent */
#define	E_PS0E03_RGENT_ERR	(E_PS_DSQL + 0x03L)

/*
**  Errors associated with general resource limitations. Upon
**  issue, the scheduler should suspend session and try later.
*/
#define E_PS_RSRC			(E_PS_ERRORS + 0x0F00L)
/* RDF cache full. All cache elements are locked (in use). */
#define E_PS0F01_CACHE_FULL		(E_PS_RSRC + 0x01L)
/* Memory errors. Need to wait until others finish or PSF is allocated
** more memory. 
*/
#define E_PS0F02_MEMORY_FULL		(E_PS_RSRC + 0x02L)
/* Transaction aborted, probably due to deadlock */
#define E_PS0F03_TRANSACTION_ABORTED	(E_PS_RSRC + 0x03L)
/* General error from RDF informing about lack of resources */
#define E_PS0F04_RDF_RESOURCE_ERR	(E_PS_RSRC + 0x04L)
/* If psf_malloc() can't get memory, show memory size */
#define E_PS0F05_BAD_MEM_SIZE		(E_PS_RSRC + 0x05L)

/*
**  Errors with requests to alter session characteristics through SET commands.
*/
#define E_PS_SETERR			(E_PS_ERRORS + 0x0F80L)
#define E_PS0F81_SETLG_PRIV_ERR		(E_PS_SETERR + 0x01L)
#define E_PS0F82_SET_SESSION_STMT	(E_PS_SETERR + 0x02L)
#define E_PS0F83_SET_ONERROR_STMT	(E_PS_SETERR + 0x03L)
#define E_PS0F84_SET_ONERROR_STMT	(E_PS_SETERR + 0x04L)
#define E_PS0F85_SET_UPDATE_ROWCOUNT_STMT (E_PS_SETERR + 0x05L)
#define E_PS0F86_SET_ROLE_WITH            (E_PS_SETERR + 0x06L)
#define E_PS0F87_SET_ON_USER_ERROR	(E_PS_SETERR + 0x07L)

/* Miscellaneous errors */
#define E_PS_MISC			(E_PS_ERRORS + 0x1000L)

/* Error when calling SCF to get info */
#define E_PS1001_ERR_SCU_INFO		(E_PS_MISC + 0x01L)

/*
** An CREATE TABLE WITH-clause option not supported by this DBMS was assumed to
** be a gateway-specific option and ignored
*/
#define I_PS1002_CRTTBL_WITHOPT_IGNRD	(E_PS_MISC + 0x02L)

/* Error when auditing security event */
#define E_PS1003_ERR_AUDITING		(E_PS_MISC + 0x03L)

/* Bad object type */
#define E_PS1004_BAD_OBJ_TYPE		(E_PS_MISC + 0x04L)

/* Attempt to write security audit message failed */
# define E_PS1005_SEC_WRITE_ERROR	(E_PS_MISC + 0x05L)

/* Unable to get default security label */
# define E_PS1006_DEF_LABEL_ERR		(E_PS_MISC + 0x06L)

/* Error  doing MAC access on object */
# define E_PS1007_MAC_ERROR		(E_PS_MISC + 0x07L)

/* Error altering  security audit  state  */
# define E_PS1008_ALTER_SECAUDIT_ERR	(E_PS_MISC + 0x08L)

#define E_PS_GW				(E_PS_ERRORS + 0x1100L)
#define E_PS1101_REG_NOLOC              (E_PS_GW + 0x01L)
#define E_PS1102_REG_NODIST             (E_PS_GW + 0x02L)
#define E_PS1103_IDX_TOOMANYOPT         (E_PS_GW + 0x03L)
#define E_PS1104_IDX_BADSTRUCT          (E_PS_GW + 0x04L)
#define E_PS1105_IDX_WITHINVAL          (E_PS_GW + 0x05L)
#define E_PS1106_IDX_NOPTBL             (E_PS_GW + 0x06L)
#define E_PS1107_TBL_MAXCOL             (E_PS_GW + 0x07L)
#define E_PS1108_TBL_COLINVAL           (E_PS_GW + 0x08L)
#define E_PS1109_TBL_COLDUP             (E_PS_GW + 0x09L)
#define E_PS110A_TBL_REGFAIL            (E_PS_GW + 0x0aL)
#define E_PS110B_TBL_COLFORMAT          (E_PS_GW + 0x0bL)
#define E_PS110C_TBL_COLLIST		(E_PS_GW + 0x0cL)
#define E_PS110D_TBL_NOCOL		(E_PS_GW + 0x0dL)
#define E_PS110E_GATEWAY_ERROR          (E_PS_GW + 0x0eL)
#define E_PS110F_GW_REM_ERR             (E_PS_GW + 0x0fL)
/*
** 03-oct-03 (chash01) Add register table/index warning where
**   multi-column key have data type other than char or varchar.
*/
#define W_PS1110_REG_TAB_IDX_MULTI_COL	(E_PS_GW + 0x10L)

/* Define error messages for registered procedure in star */
#define E_PS_REGPROC			(E_PS_ERRORS + 0x1200L)

#define E_PS1201_LDB_PROC_LVL_UNSUP	(E_PS_REGPROC + 0x01L)
#define E_PS1202_REG_WRONGTYPE		(E_PS_REGPROC + 0x02L)
#define E_PS1203_NO_LDB_PROC		(E_PS_REGPROC + 0x03L)
#define E_PS1204_RMV_WRONGTYPE		(E_PS_REGPROC + 0x04L)
#define E_PS1205_DROP_WRONGTYPE		(E_PS_REGPROC + 0x05L)
#define E_PS1206_REGISTER		(E_PS_REGPROC + 0x06L)
#define E_PS1207_REMOVE			(E_PS_REGPROC + 0x07L)



/*
**  Cursor control block definition.
*/

#define                 PSC_I4SIZE      (sizeof (i4) * BITSPERBYTE)
#define                 PSC_NUMWORDS    (DB_MAX_COLS / PSC_I4SIZE)
#define                 PSC_BITSOVER    (DB_MAX_COLS % PSC_I4SIZE)
#define                 PSC_WORDSOVER   ((PSC_BITSOVER + PSC_I4SIZE - 1) \
					 / PSC_I4SIZE)
#define                 PSC_MAPSIZE     (PSC_NUMWORDS + PSC_WORDSOVER)

#define                 PSV_NUMWORDS    (DB_MAXVARS / PSC_I4SIZE)
#define                 PSV_BITSOVER    (DB_MAXVARS % PSC_I4SIZE)
#define                 PSV_WORDSOVER   ((PSV_BITSOVER + PSC_I4SIZE - 1) \
					 / PSC_I4SIZE)
#define                 PSV_MAPSIZE     (PSV_NUMWORDS + PSV_WORDSOVER)

/*
**  Definition of control block for general-purpose and query-related
**  operations.
*/

/*
**  Opcodes used with PSQ_CB.
**
**  History:
**	18-apr-89 (neil)
**	    Added comment about overloaded opcodes.
**	17-may-91 (andre)
**	    Defined PSQ_RESET_EFF_USER_ID, PSQ_RESET_EFF_ROLE_ID, and
**	    PSQ_RESET_EFF_GROUP_ID.
**	01-dec-92 (andre)
**	    get rid of PSQ_RESET_EFF_ROLE_ID and PSQ_RESET_EFF_GROUP_ID
**	17-mar-94 (andre)
**	    defined PSQ_OPEN_REP_CURS
**	6-jul-95 (stephenb)
**	    add PSQ_RESOLVE (resolve user access to tables/views).
**	5-oct-95 (thaju02)
**	    Added PSQ_SESSOWN (obtain temporary table session owner) 
**	2-nov-1998 (stial01)
**	    Added for Rd0013
**	24-May-2006 (kschendel)
**	    Add code for DESCRIBE INPUT, remove obsolete SET statements.
**      20-Aug-2009 (coomi01) b122237
**          Define PSQ_OPRESLV to resolve op nodes. 
**	5-nov-2009 (stephenb)
**	    Add PSQ_COPYBUF
*/

#define		PSQ_ALTER	    1	/* Alter session parameters */
#define		PSQ_BGN_SESSION	    2   /* Begin a parser session */
#define		PSQ_CLOSEALL	    3   /* Close all cursors in a session */
#define		PSQ_CURDUMP	    4   /* Dump a cursor control block */
#define		PSQ_DELRNG	    5   /* Delete a range variable */
#define		PSQ_END_SESSION	    6   /* End a parser session */
#define		PSQ_PARSEQRY	    7   /* Parser a query */
#define		PSQ_PRMDUMP	    8   /* Dump a PSQ_CB */
#define		PSQ_SESDUMP	    9   /* Dump a parser session ctl blk */
#define		PSQ_SHUTDOWN	    10  /* Shut down the parser facility */
#define		PSQ_SRVDUMP	    11  /* Dump the server control block */
#define		PSQ_STARTUP	    12  /* Start up the parser facility */

					/*
					** Request to recreate a database
					** procedure.
					*/
#define		PSQ_RECREATE	    13  

#define		PSQ_RESET_EFF_USER_ID	14  /* reset the effective user id */

#define		PSQ_OPEN_REP_CURS   15	/* open a repeated cursor */
#define		PSQ_RESOLVE	    16  /* resolve user acceess to
					** tables, views and synonyms */
#define         PSQ_SESSOWN         17  /* obtain temp table session owner */
#define         PSQ_ULMCONCHK       18
#define		PSQ_GET_STMT_INFO   19
#define         PSQ_OPRESLV         20  /* Resolve op nodes */
#define		PSQ_COPYBUF	    22  /* fill out copy buffer for 
					** insert to copy optimization*/
#define		PSQ_GET_SESS_INFO   23

/*
** These are also used as PSF opcodes, but are really from the list of statement
** modes in the PSQ_CB.
** 
** #define	PSQ_CLSCURS	21
** #define	PSQ_DELCURS	72
** #define	PSQ_COMMIT	74
*/

/*}
** Name: PSQ_TRPARMS - Parameters from "set trace point" command
**
** Description:
**      This structure defines the parameters that can be passed to the parser
**      by the "set trace point" command.
**
** History:
**     14-oct-85 (jeff)
**          written
*/
typedef struct _PSQ_TRPARMS
{
    i4              psq_trswitch;       /* Indicator for on, off, or nothing */
#define                 PSQ_T_NOCHANGE 0   /* Don't change the trace vector */
#define                 PSQ_T_ON       1   /* Turn on the trace vector posn */
#define                 PSQ_T_OFF      2   /* Turn off the trace vector posn */
    i4              psq_trflag;         /* Which flag in vector to change */
    i4		    psq_trnmparms;	/* Number of optional parms below */
    i4              psq_tr1value;       /* Optional 1st value */
    i4              psq_tr2value;       /* Optional 2nd value */
} PSQ_TRPARMS;

/*}
** Name: PSQ_STMT_INFO - Information about prepared insert statement
**
** Description:
**      This structure defines information about a prepared insert statement
**      that is useful in the sequencer - scs_blob_input
**
** History:
**     24-jul-2003 (stial01)
**          Added for b110620
**	15-Apr-2010 (kschendel) SIR 123485
**	    Include table ID, is easier on dmpe than table name/owner.
*/
typedef struct _PSQ_STMT_INFO
{
    char		psq_stmt_tabname[DB_TAB_MAXNAME+1];
    char		psq_stmt_ownname[DB_OWN_MAXNAME+1];
    DB_TAB_ID		psq_stmt_tabid;
    i4			psq_stmt_blob_cnt;
    i4			psq_stmt_blob_colno;
} PSQ_STMT_INFO;


/*}
** Name: PSY_QID - Permanent query id for iiqrytext relation.
**
** Description:
**	A PSY_QID uniquely identifies query text stored in the iiqrytext system
**	relation.  It is returned by the parser facility whenever it stores
**	the text of a view, permit, or integrity definition in this relation.
**
** History:
**      01-oct-85 (jeff)
**          written
*/
typedef DB_TAB_TIMESTAMP PSY_QID;

/*}
** Name: PST_UNION - Structure holding union info.
**
** Description:
**      This structure groups info pertaining to unions.
**
** History:
**     07-oct-87 (stec)
**          Created.
**	20-sep-05 (inkdo01)
**	    Merged INTERSECT/EXCEPT changes from defunct ing30sol codeline.
*/
typedef struct _PST_UNION
{
    	/* pst_dups defines whether or not duplicates should be removed from
	** the results of the union with respect to this subtree.
	** pst_dups is filled in only when pst_next is not null.
	*/
    i2			    pst_dups; /* PST_ALLDUPS or PST_NODUPS */
	/* pst_setop defines the type of set operator. Currently recognized
	** are PST_UNION_OP and PST_EXCEPT_OP. Later, "intersect" will
	** also be supported - but requires set operation nesting because 
	** of precedence. 
	*/
    i1			    pst_setop;
#define	PST_UNION_OP	1
#define	PST_EXCEPT_OP	2
#define PST_INTERSECT_OP 3

	/* pst_nest defines the nesting level of the set operation represented 
	** by this node. It is used to support precedence ordering of
	** INTERSECT and of parenthesized operators. */
    i1			    pst_nest;

	/* Pst_next forms a linked list of root nodes that should be
	** unioned together. This means that the types and lengths of all
	** the target lists should be the same for each resdom number.
	*/
    struct _PST_QNODE	    *pst_next;
} PST_UNION;

/*}
** Name: PST_INFO - Info needed by the parser while parsing Execute Immedate
**                      actions.
** Description:
**      This holds the info needed by the parser for reporting accurate error
**      messages while parsing the Execute Immediate actions. It also has
**      some execution flags to enable the parser to ignore the REFERENCES
**      part of the CREATE TABLE statement while parsing the Execute Immediate
**      action for the CREATE TABLE statement.
**
** History:
**      19-nov-92 (anitap)
**          Written for CREATE SCHEMA project.
**	25-nov-92 (ralph)
**	    Modified PST_INFO to contain additional info required
**	    by the parser.
**	    Introduced PST_OBJDEP structure to contain object dependencies
**	    for creating views within the CREATE SCHEMA wrapper.
**	17-dec-92 (ralph)
**	    Added pst_objname to PST_INFO for reordering
**	28-dec-92 (rblumer)
**	    Added PST_SYSTEM_GENERATED, PST_NOT_DROPPABLE, and
**	    PST_SUPPORTS_CONSTRAINT flags.
**	02-may-93 (andre)
**	    defined PST_EMPTY_REFERENCING_TABLE over pst_execflags.  This bit
**	    will be set when generating EXECUTE IMMEDIATE node for
**	    self-referencing REFERENCES constraint declared inside CREATE TABLE
**	    statement.  This will save us the trouble of checking whether there
**	    are any rows violating the constraint since, clearly, there won't be
**	    any (as the table is brand new and very empty.)  If/when we start
**	    supporting specification of constraints in CREATE TABLE AS SELECT
**	    statement, this flag should NOT be used there, since the table will
**	    no longer be guaranteed to be empty.
**	24-may-93 (ralph)
**	    Added PST_INFO.pst_srctext to point to query text in the query
**	    buffer.  This is used to correct the problem with GRANT WGO
**	    and CREATE VIEW WCO.
**	26-jun-93 (andre)
**	    defined PST_CASCADED_CHECK_OPTION_RULE over pst_execflags
**	16-jul-93 (rblumer)
**	    changed PST_EMPTY_REFERENCING_TABLE to PST_EMPTY_TABLE,
**	    since it could be used by any constraint 
**	    (though currently is only used for referential constraints).
**      17-Apr-2001 (horda03) Bug 104402
**          Added PST_NOT_UNIQUE.
*/
typedef struct _PST_INFO
{
  i4    pst_baseline;          /* original line number of the statement. Needed
                               ** for ALTER TABLE ADD CONSTRAINT part of CREATE
                               ** TABLE statement.
                               */
  i4    pst_basemode;          /* original query mode of the statement. Needed
                               ** for the parser when parsing the Execute
                               ** Immediate action for the referential
                               ** integrity. Needed for accurate error message
                               ** reporting.
                               */
  u_i4     pst_execflags; /* Execution Flags.*/

#define PST_SCHEMA_MODE		0x01L  	/* Needed for the parser so that while
					** parsing the CREATE TABLE statement
					** in the CREATE SCHEMA wrapper, the
					** parser ignores the referential
					** integrity part of the statement. 
					*/
#define PST_SYSTEM_GENERATED	0x02L	/* this is a system-generated
					** statement, used to create rules,
					** procs, and indexes for SQL92
					** constraints and for WITH CHECK OPTION
					*/
#define PST_NOT_DROPPABLE	0x04L	/* this object being created cannot be
					** dropped by the user, as it is used
					** to implement constraints or WITH
					** CHECK OPTION 
					*/
#define PST_SUPPORTS_CONSTRAINT	0x08L	/* the index being created will be used
					** to implement a UNIQUE constraint,
					** and certain characteristics cannot
					** be changed
					*/
#define	PST_EMPTY_TABLE 0x10L		/*
					** set when we are executing an ALTER
					** TABLE statement inside of an execute
					** immediate statement, and we know
					** that the table will be empty (e.g.
					** for self-referential CREATE TABLEs
					** or for CREATE SCHEMA [since we don't
					** allow create table as select yet])
					** since the new table is empty, we can
					** forego checking whether existing
					** data satisfies the constraint
					*/

#define	PST_CASCADED_CHECK_OPTION_RULE	0x20L
					/*
					** this bit will be set if PSF has been
					** called to create a rule enforcing
					** CASCADED CHECK OPTION.  PSF will
					** translate this bit into
					** DBR_CASCADED_CHECK_OPTION_RULE bit
					** over DB_IIRULE.dbr_flags
					*/

#define PST_SHARED_CONSTRAINT 0x40L
					/*
					** index being created is to be
					** shared amongst several constraints.
					** This flag was invented to make
					** "copydb" build index defs for
					** them (rather than defining
					** PST_SYSTEM_GENERATED). */
#define PST_NOT_UNIQUE 0x80L
					/* index being created is for
					** a non-unique constraint.
					*/

  DB_TAB_NAME	*pst_objname;	/* Pointer to object name, if applicable.
				** Used only for tables and views.
				** Not valid during execution phase.
				*/
  struct _PST_OBJDEP  *pst_deplist;
				/* Pointer to first object dependency in list.
				** NULL if no dependencies.
				** Not valid during execution phase.
				*/
  char		*pst_srctext;
				/* Pointer to location in the query buffer
				** where the substatement starts.  This is
				** used to allocate query text buffers for
				** each user-specified statement after the
				** entire CREATE SCHEMA statement is read.
				** Not valid during execution phase.
				*/
} PST_INFO;

/*}
** Name: PST_OBJDEP - Element in object dependency list.
**
** Description:
**      This structure contains information about an object upon which
**	the current object being created depends.  For example, when
**	a view is being created, it references other tables and views.
**	These referenced tables and views are objects upon which the
**	view being created depends.  
**
**	This structure is used when parsing CREATE SCHEMA to track
**	objects upon which a view depends, so that later the parser
**	can reorder the CREATE VIEW statements to eliminate forward
**	references.
**
** History:
**	25-nov-92 (ralph)
**          Written for CREATE SCHEMA project.
**	17-dec-92 (ralph)
**	    Added pst_depstmt to PST_OBJDEP for reordering
**	17-dec-92 (anitap)
**	    Added struct to PST_STATEMENT structure to get rid of compilation
**	    errors.
**	12-jan-93 (ralph)
**	    CREATE SCHEMA:
**	    Added PST_OBJDEP.pst_depuse for reordering statements
*/
typedef struct _PST_OBJDEP
{
    struct _PST_OBJDEP  *pst_nextdep;	/* forward pointer */
    struct _PST_OBJDEP  *pst_prevdep;	/* backward pointer */
    struct _PST_STATEMENT *pst_depstmt;	/* PTR to corresponding PST_STATEMENT
					** that creates the dependency.
					*/
    u_i4		pst_depuse;	/* How dependency is used */
#define PST_DEP_NOORDER	0x01L		/* Object not used in reordering */
    DB_OWN_NAME	    	pst_depown;	/* owner of object */
    DB_TAB_NAME	    	pst_depname;	/* name of object */
} PST_OBJDEP;

/*}
** Name: PSQ_CB - Parser control block for general-purpose and query operations
**
** Description:
**      This structure defines the control block for with the Parser Facility
**      for query-related operations and general-purpose operations.  The result
**	of parsing a query is a pointer to a control block; this structure uses
**	a pointer to a character instead of the union of the control block types
**	in order to avoid header dependencies.
**
** History:
**     26-sep-85 (jeff)
**          written
**     10-jul-86 (jeff)
**	    changed psq_result from an object handle (PTR) to a QSO_OBID
**     28-jul-86 (jeff)
**	    added psq_catupd, psq_idxstruct, psq_mnyfmt, psq_dtefmt
**     30-jul-86 (jeff)
**	    added psq_qryname
**     30-jul-86 (jeff)
**	    added psq_warnings
**     11-may-87 (stec)
**	    added psq_udbid
**     25-jan-88 (stec)
**	    Added PSQ_QRYDEFED mode.
**	25-apr-88 (stec)
**	    Make db procedure related changes (new modes).
**	29-jul-88 (stec)
**	    Added PSQ_ASSIGN query mode.
**	10-nov-88 (stec)
**	    Changed defn of psq_udbid from PTR to I4.
**	08-mar-89 (andre)
**	    Add psq_dba_drop_all to PSQ_CB.
**	10-Mar-89 (ac)
**	    Add PSQ_SECURE for 2PC support.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Defined new statement modes, PSQ_[CADK]GROUP and PSQ_[CAK]APLID;
**	    Added psq_group, psq_aplid fields to PSQ_CB.
**	16-mar-89 (neil)
**          Rules: Added PSQ_RULE, PSQ_DSTRULE and PSQ_RS_ERROR opcodes.
**	    Added psq_als_owner to CB.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Defined new statement modes, PSQ_[GR]DBPRIV, PSQ_REVOKE;
**      21-apr-89 (eric)
**          added the define for PSQ_IPROC, for internal procedure execution.
**	28-apr-89 (neil)
**	    Rules: Added "table name" field in PSQ_CB for cursor rule
**	    processing (psq_tabname).
**	22-jun-89 (neil)
**	    Defined PSQ_CALLPROC for nested procs - replaced unused PSQ_HINTEG.
**	20-jul-89 (jennifer)
**	    Added ability to pass PSF server capability information
**	    by usering pss_version to convey this information
**	    for a PSQ_STARTUP operation.
**	27-jul-89 (jrb)
**	    Added psq_num_literals so PSF can be told which set of numeric
**	    literal scanning rules to use.
**	07-sep-89 (neil)
**	    Alerters: Replaced some old opcodes with new ones:
**		PSQ_EXCLNODE 	-> PSQ_EVENT
**		PSQ_HPERM 	-> PSQ_EVDROP
**		PSQ_HVIEW 	-> PSQ_EVREGISTER
**		PSQ_INCLNODE 	-> PSQ_EVDEREG
**		PSQ_RENREF 	-> PSQ_EVRAISE
**	09-oct-89 (ralph)
**	    Added new statement modes for nB1:
**		PSQ_CUSER, PSQ_AUSER, PSQ_KUSER,
**		PSQ_CLOC,  PSQ_ALOC,  PSQ_KLOC,
**		PSQ_CALARM, PSQ_KALARM,
**		PSQ_ENAUDIT, PSQ_DISAUDIT.
**	27-oct-89 (ralph)
**	    Added psq_ustat to PSQ_CB.
**	28-dec-89 (andre)
**	    Added psq_fips_mode flag to PSQ_CB.
**	20-feb-90 (andre)
**	    Added PSQ_COMMENT
**	17-apr-90 (andre)
**	    define PSQ_CSYNONYM, PSQ_DSYNONYM
**	08-aug-90 (ralph)
**          Add new statement modes PSQ_[GR]OKPRIV to allow GRANT ON DATABASE
**          with only UPDATESYSCAT and ACCESS privileges in vanilla environs.
**	11-sep-90 (teresa)
**	    change some bools/nats in PSQ_CB to become bitmasks in
**	    psq_instr status word.
**	12-sep-90 (sandyh)
**	    Added psq_mserver to support passing of startup value from
**	    scd_initiate().
**	9-jan-91 (seputis)
**	    Defined PSQ_NOTIMEOUTERROR over psq_flag as a part of the project to
**	    support OPF memory management 
**	06-feb-91 (andre)
**	    Added PSQ_ENDLOOP query mode.
**	    (this was done in conjunction with a fix for bug #35659)
**	17-may-91 (andre)
**	    The following changes were made in conjunction with implementing
**	    SET USER/ROLE/GROUP:
**	      - added 3 new query modes: PSQ_SETUSER, PSQ_SETROLE, and
**	        PSQ_SETGROUP,
**	      - added a new field, psq_ret_flag, which will be used by PSF to
**	        communicate useful info to its callers + defined 4 masks over
**		it: PSQ_USE_SESSION_ID, PSQ_USE_SYSTEM_USER,
**		PSQ_USE_CURRENT_USER, and PSQ_CLEAR_ID.
**	07-nov-91 (andre)
**	    merge 4-feb-91 (rogerk) change:
**		Added PSQ_SLOGGING query mode.
**	07-nov-91 (andre)
**	    merge 25-feb-91 (rogerk) change:
**		Moved PSQ_SLOGGING query mode from 125->142 so as not to clash
**		with new query modes being created in 6.5.
**		Added new PSQ_SON_ERROR query mode for Set Session statement.
**	07-nov-91 (andre)
**	    merge 25-jul-91 (andre) change:
**		define PSQ_STRIP_NL_IN_STRCONST over PSQ_CB.psq_flag; when set
**		it indicates that the scanners should strip NLs found inside
**		string constants; (bug 38098)
**	12-dec-91 (barbara)
**	    Merge of rplus and Star code for Sybil.  Added Star mode values for
**	    psq_mode, psq_flag value PSQ_DIRCONNECT_MODE, pointers to 
**	    DD_LDB_DESCs for target and default nodes.  Also added Star
**	    comments:
**	    12-sep-88 (andre)
**		added psq_ldbflag, psq_ldbdesc, psq_dircon.
**	    22-sep-88 (andre)
**		removed psq_ldbflag and replaced it with psq_dfltldb.
**	    10-Mar-89 (andre)
**		Add psq_dv and psq_dv_num to be used for passing parameters and
**		the # of parameters to SCF.
**	    13-mar-89 (andre)
**		Add psq_qid_offset to help SCF find the beginning of the 
**		query ID.
**	    02-may-89 (andre)
**		Add psq_tdesc and psq_res1.
**	26-feb-92 (andre)
**	    define PSQ_REPAIR_SYSCAT over PSQ_CB.psq_flag
**	6-apr-1992 (bryanp)
**	    Add new query modes PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**      27-may-92 (schang)
**          define PSQ_REG_INDEX and PSQ_REG_REMOVE for GW REGISTER INDEX 
**          and REMOVE [TABLE/INDEX].
**      26-jun-92 (anitap)
**          Added PSQ_UPD_ROWCNT query mode for the backward compatibility
**          update rowcount project.
**	03-aug-92 (barbara)
**	    define PSQ_REDO mask for psq_flag.  Flag is set by SCF to
**	    indicate server retry.
**	02-nov-92 (jrb)
**	    Added PSQ_SWORKLOC for "set work locations..." command
**	    (multi-location sorts project).
**	01-dec-92 (andre)
**	    SET USER AUTHORIZATION is becoming SET SESSION AUTHORIZATION and
**	    SET GROUP/ROLE is going away:
**		- change PSQ_SETUSER to PSQ_SET_SESS_AUTH_ID,
**		- get rid of PSQ_SETGROUP, PSQ_SETROLE, and PSQ_CLEAR_ID
**      13-oct-92 (anitap)
**          Added PSQ_CREATE_SCHEMA query modes for the CREATE SCHEMA project.
**          Also added pointer to PST_INFO structure. SCF will copy over the
**          pointer to PST_INFO structure from QEF_RCB to pass on to PSF.
**	14-jan-1993 (andre)
**	    define PSQ_RUNNING_UPGRADEDB
**      05-jan-93 (rblumer)
**          Added set_proc_id and set_input_tabid for set-input dbprocs;
**          Added new query mode for creating set-input procs: PSQ_CRESETDBP.
**	22-jan-93 (andre)
**	    defined PSQ_DFLT_DIRECT_CRSR and PSQ_NOFLATTEN_SINGLETON_SUBSEL
**      10-feb-93 (rblumer)
**          Added new query mode for distributed CREATE TABLE, as SCF must
**          treat it like 6.4 CREATE TABLE and not like the new 6.5 processing.
**	19-feb-93 (andre)
**	    defined PSQ_DROP_SCHEMA
**	05-mar-93 (ralph)
**	    DELIM_IDENT:
**	    Added psq_dbxlate to PSQ_CB for case translation semantics.
**	30-mar-93 (rblumer)
**	    swapped PSQ_CREATE and PSQ_DISTCREATE query modes (for easier error
**	    handling in pslyerror.c); added PSQ_NO_CHECK_PERMS, so that don't
**	    check permissions on internal queries used to verify constraints
**	    hold during alter table ADD constraint.
**	02-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added psq_cat_owner to PSQ_CB to define the catalog owner.
**	    This will point to a DB_OWN_NAME containing "$ingres" if
**	    regular identifiers are translated to lower case, or "$INGRES" if
**	    regular identifiers are translated to upper case.
**	    Changed psq_dbxlate to be a pointer to a u_i4.
**	12-apr-93 (tam)
**	    Added PSQ_OWNER_SET for owner.procedure; psq_als_owner will be
**	    reused to stored the owner field.
**	02-jun-93 (rblumer)
**	    Changed PSQ_NO_CHECK_PERMS so as not to conflict with PSQ_OWNER_SET
**	21-jun-93 (robf)
**	    Add PSQ_SXF_TRACE and PSQ_SET_SESSION
**	29-sep-93 (andre)
**	    looks like swm missed psq_sessid - change it from PTR to CS_SID
**	18-oct-93 (rogerk)
**	    Added PSQ_NO_JNL_DEFAULT psq_flag which specifies that tables
**	    should be created with nojournaling by default.  This flag is
**	    passed in the psq_startup call when the DEFAULT_TABLE_JOURNALING
**	    server startup flag specifies nojournaling.
**	01-nov-93 (anitap)
**	    defined PSQ_INGRES_PRIV for verifydb so that $ingres can drop and
**	    create constraints on tables owned by other users. 
**	    Removed duplicate definitions of PSQ_DFLT_DIRECT_CRSR and 
**	    PSQ_NOFLATTEN_SINGLETON_SUBSEL
**	17-dec-93 (rblumer)
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).  So I removed the PSQ_FIPS_MODE flag.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	16-nov-93 (robf)
**          Add PSQ_SETROWLABEL
**	17-mar-94 (robf)
**	    Add PSQ_SELECT_ALL
**      28-nov-95 (stial01)
**          Added PSQ_XA_ROLLBACK, PSQ_XA_COMMIT, PSQ_XA_PREPARE
**          Added psq_distranid, PSQ_SET_XA_XID
**	30-april-1997(angusm)
**	    add PSQ_AMBREP_64COMPAT (bug 79623).
**      22-jul-1996 (ramra01 for bryanp)
**          Added PSQ_ATBL_ADD_COLUMN and PSQ_ATBL_DROP_COLUMN query modes.
**	21-jul-00 (inkdo01)
**	    Added psq_maxmemf to support OPF-style memory mgmt in PSF.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Added psq_flag2 to hold additional assorted bitflag instructions
**          similar to psq_flag which is fully utilised.
**          Added PSQ_RULE_UPD_PREFETCH to indicate that we should set
**          PSS_RULE_UPD_PREFETCH is the session control block when we
**          call psq_bgn_session().
**	10-Jan-2001 (jenjo02)
**	    Added (PTR) psq_adfcb to PSQ_CB, filled in by scsinit so that 
**	    psq_bgn_session doesn't have to call back to SCF right away to get it.
**	    Added psq_psscb_offset to PSQ_CB, offset from CS_SCB to 
**	    session's PSS_SESBLK.
**	06-jun-2002 (toumi01)
**	    Added PSQ_PARSING_ORDER_BY flag so that we can test grammar
**	    context for the FIRST keyword in the scanner.
**	16-jul-03 (hayke02)
**	    Added psq_vch_prec for config parameter psf_vch_prec. This change
**	    fixes bug 109134.
**	19-feb-2004 (gupsh01)
**	    Added PSQ_SETUNICODESUB, psq_usub_stat and  psq_usub_char to 
**	    support set [no]unicode_substitution. 
**	20-Apr-2004 (jenjo02)
**	    Added SET SESSION ... ON_LOGFULL
**	03-mar-2004 (gupsh01)
**	    Added PSQ_ATBL_ALTER_COLUMN for alter table alter column support.
**	31-Aug-2004 (sheco02)
**	    X-integrating change 466442 to main.
**	01-Sep-2004 (bonro01)
**	    Fix syntax error caused by previous X-integration
**	13-Jul-2005 (thaju02)
**	    Add PSQ_ON_USRERR_NOROLL, PSQ_ON_USRERR_ROLLBACK.
**	26-Jun-2006 (jonj)
**	    Add XA integration stuff:
**	    PSQ_GCA_XA_START, PSQ_GCA_XA_END, PSQ_GCA_XA_PREPARE,
**	    PSQ_GCA_XA_COMMIT, PSQ_GCA_XA_ROLLBACK.
**	24-aug-06 (toumi01)
**	    Add PSQ_GTT_SYNTAX_SHORTCUT for optional "session." SIR.
**	31-aug-06 (toumi01)
**	    Delete PSQ_GTT_SYNTAX_SHORTCUT.
**	9-may-2007 (dougi)
**	    Add PSQ_FREELOCATOR for FREE LOCATOR.
**      17-may-2007 (stial01)
**          Changed psq_locator to DB_DATA_VALUE type
**	3-jan-2008 (dougi)
**	    Add PSQ_SCACHEDYN for "set [no]cache_dynamic" statement, 
**	    PSQ_CACHEDYN as corresponding flag and PSQ_SSCACHEDYN for 
**	    "set session [no]cache_dynamic".
**	15-Sep-2008 (kibro01) b120571
**	    Add PSQ_ALLOW_TRACEFILE_CHANGE.
**	05-Feb-2009 (kiria01) b121607
**	    Define psq_mode as enum with table macro to keep things
**	    in sync with little effort.
**	10-march-2009 (dougi) bug 121773
**	    Added PSQ_LOCATOR flag when GCA_LOCATOR_MASK is on.
**	11-Mar-2009 (frima01) b121607
**	    Moved psq_mode definition outside of PSQ_CB definition
**	    to avoid portability problems. 
**      20-Aug-2009 (coomi01) b122237
**          Add fields psq_qsfcb and psq_node for use in psq_call(PSQ_OPRESLV)
**          interface.
**	5-nov-2009 (stephenb)
**	    Add fields and flags for batch processing optimizations
**      15-feb-2010 (maspa05) b123293
**          Added psq_server_class so SC930 can output server class
**	18-Mar-2010 (gupsh01, dougi) SIR 123444
**	    Add defines for Alter table rename table/column.
**	29-apr-2010 (stephenb)
**	    Move batch flags from psq_flags to psq_flags2 to to avoid
**	    conflicts. Add PSQ_SETBATCHCOPYOPTIM.
**	20-Jul-2010 (kschendel) SIR 124104
**	    Add a place to pass in create-compression.
**	12-aug-2010 (stephenb)
**	    Resolve flag conflict in psq_flags2
*/
typedef enum psq_mode_enum {
#define PSQ_MODES_MACRO \
_DEFINE(PSQ_RETRIEVE,            1,"RETRIEVE")\
_DEFINE(PSQ_RETINTO,             2,"CREATE TABLE")\
_DEFINE(PSQ_APPEND,              3,"APPEND")\
_DEFINE(PSQ_REPLACE,             4,"REPLACE")\
_DEFINE(PSQ_DELETE,              5,"DELETE")\
_DEFINE(PSQ_COPY,                6,"COPY")\
_DEFINE(PSQ_DISTCREATE,          7,"DISTRIBUTED CREATE")\
_DEFINE(PSQ_DESTROY,             8,"DESTROY")\
_DEFINE(PSQ_HELP,                9,"HELP")\
_DEFINE(PSQ_INDEX,              10,"CREATE INDEX" /*MUST MATCH DB_INDEX value in <dbdbms.h>*/ )\
_DEFINE(PSQ_MODIFY,             11,"MODIFY")\
_DEFINE(PSQ_PRINT,              12,"PRINT")\
_DEFINE(PSQ_RANGE,              13,"RANGE")\
_DEFINE(PSQ_SAVE,               14,"SAVE")\
_DEFINE(PSQ_DEFCURS,            15,"DEFINE CURSOR")\
_DEFINE(PSQ_QRYDEFED,           16,"DEFINE CURSOR")\
_DEFINE(PSQ_VIEW,               17,"CREATE VIEW" /*MUST MATCH DB_VIEW value in <dbdbms.h>*/ )\
_DEFINE(PSQ_ABSAVE,             18,"ABORT TO SAVEPOINT")\
_DEFINE(PSQ_PROT,               19,"CREATE PERMIT" /*MUST MATCH DB_PROT value in <dbdbms.h>*/ )\
_DEFINE(PSQ_INTEG,              20,"CREATE INTEGRITY" /*MUST MATCH DB_INTG value in <dbdbms.h>*/ )\
_DEFINE(PSQ_CLSCURS,            21,"CLOSE CURSOR")\
_DEFINE(PSQ_RELOCATE,           22,"RELOCATE")\
_DEFINE(PSQ_DEFQRY,             23,"DEFINE QUERY")\
_DEFINE(PSQ_EXECQRY,            24,"EXECUTE QUERY")\
_DEFINE(PSQ_DEFREF,             25,"DEFINE REFERENCE")\
_DEFINE(PSQ_BGNTRANS,           26,"BEGIN TRANSACTION")\
_DEFINE(PSQ_ENDTRANS,           27,"END TRANSACTION")\
_DEFINE(PSQ_SVEPOINT,           28,"SAVEPOINT")\
_DEFINE(PSQ_ABORT,              29,"ABORT")\
_DEFINE(PSQ_DSTINTEG,           30,"DROP INTEGRITY")\
_DEFINE(PSQ_DSTPERM,            31,"DROP PERMIT")\
_DEFINE(PSQ_DSTREF,             32,"DESTROY REFERENCE")\
_DEFINE(PSQ_EVENT,              33,"CREATE DBEVENT" /*MUST MATCH DB_EVENT value in <dbdbms.h>*/ )\
_DEFINE(PSQ_EXCURS,             34,"EXECUTE CURSOR")\
_DEFINE(PSQ_CALLPROC,           35,"EXECUTE PROCEDURE")\
_DEFINE(PSQ_EVDROP,             36,"DROP DBEVENT")\
_DEFINE(PSQ_EVREGISTER,         37,"REGISTER DBEVENT")\
_DEFINE(PSQ_EVDEREG,            38,"REMOVE DBEVENT")\
_DEFINE(PSQ_EVRAISE,            39,"RAISE DBEVENT")\
_DEFINE(PSQ_REPCURS,            40,"REPLACE CURSOR")\
_DEFINE(PSQ_RETCURS,            41,"RETRIEVE CURSOR")\
_DEFINE(PSQ_DESCINPUT,          42,"DESCRIBE INPUT")\
_DEFINE(PSQ_REPDYN,             43,"PSQ_REPDYN")\
_DEFINE(PSQ_SCACHEDYN,          44,"PSQ_SCACHEDYN")\
_DEFINE(PSQ_SNOCACHEDYN,        45,"PSQ_SNOCACHEDYN")\
_DEFINE(PSQ_SAGGR,              46,"SET AGGREGATE")\
_DEFINE(PSQ_SCPUFACT,           47,"SET CPUFACTOR")\
_DEFINE(PSQ_SDATEFMT,           48,"SET DATE_FORMAT")\
_DEFINE(PSQ_SSCACHEDYN,         49,"PSQ_SSCACHEDYN")\
_DEFINE(PSQ_SDECIMAL,           50,"SET DECIMAL")\
_DEFINE(PSQ_SWORKLOC,           51,"SET WORK LOCATIONS")\
_DEFINE(PSQ_52_unused,          52,"PSQ_52_unused")\
_DEFINE(PSQ_SJOURNAL,           53,"SET JOURNALING")\
_DEFINE(PSQ_SMNFMT,             54,"SET MONEY FORMAT")\
_DEFINE(PSQ_SMNPREC,            55,"SET MONEY PRECISION")\
_DEFINE(PSQ_SCARDCHK,           56,"SET CARDINALITY_CHECK")\
_DEFINE(PSQ_SNOCARDCHK,         57,"SET NOCARDINALITY_CHECK")\
_DEFINE(PSQ_SRINTO,             58,"SET RETRIEVE_INTO")\
_DEFINE(PSQ_59_unused,          59,"PSQ_59_unused")\
_DEFINE(PSQ_SSQL,               60,"SET SQL")\
_DEFINE(PSQ_SSTATS,             61,"SET STATISTICS")\
_DEFINE(PSQ_SLOCKMODE,          62,"SET LOCKMODE")\
_DEFINE(PSQ_DMFTRACE,           63,"SET TRACE POINT DMXXXX")\
_DEFINE(PSQ_PSFTRACE,           64,"SET TRACE POINT PSXXXX")\
_DEFINE(PSQ_SCFTRACE,           65,"SET TRACE POINT SCXXXX")\
_DEFINE(PSQ_RDFTRACE,           66,"SET TRACE POINT RDXXXX")\
_DEFINE(PSQ_QEFTRACE,           67,"SET TRACE POINT QEXXXX")\
_DEFINE(PSQ_OPFTRACE,           68,"SET TRACE POINT OPXXXX")\
_DEFINE(PSQ_QSFTRACE,           69,"SET TRACE POINT QSXXXX")\
_DEFINE(PSQ_ADFTRACE,           70,"SET TRACE POINT ADXXXX")\
_DEFINE(PSQ_STRACE,             71,"SET TRACE")\
_DEFINE(PSQ_DELCURS,            72,"DELETE CURSOR")\
_DEFINE(PSQ_SQEP,               73,"SET [NO]QEP")\
_DEFINE(PSQ_COMMIT,             74,"COMMIT")\
_DEFINE(PSQ_DESCRIBE,           75,"DESCRIBE (output)")\
_DEFINE(PSQ_PREPARE,            76,"PREPARE")\
_DEFINE(PSQ_INPREPARE,          77,"PREPARE INTO")\
_DEFINE(PSQ_ROLLBACK,           78,"ROLLBACK")\
_DEFINE(PSQ_AUTOCOMMIT,         79,"AUTOCOMMIT")\
_DEFINE(PSQ_RLSAVE,             80,"ROLLBACK TO SAVEPOINT")\
_DEFINE(PSQ_OBSOLETE,           81,"OBSOLETE")\
_DEFINE(PSQ_JTIMEOUT,           82,"SET JOINOP (timeout)")\
_DEFINE(PSQ_DEFLOC,             83,"DEFINE LOCATION")\
_DEFINE(PSQ_GRANT,              84,"GRANT")\
_DEFINE(PSQ_ALLJOURNAL,         85,"SET JOURNALING")\
_DEFINE(PSQ_50DEFQRY,           86,"5.0 REPEAT QUERY")\
_DEFINE(PSQ_CREDBP,             87,"CREATE PROCEDURE" /*MUST MATCH DB_DBP value in <dbdbms.h>*/ )\
_DEFINE(PSQ_DRODBP,             88,"DROP DB PROCEDURE")\
_DEFINE(PSQ_EXEDBP,             89,"EXECUTE PROCEDURE")\
_DEFINE(PSQ_IF,                 90,"IF/THEN/ELSE")\
_DEFINE(PSQ_RETURN,             91,"RETURN")\
_DEFINE(PSQ_MESSAGE,            92,"MESSAGE")\
_DEFINE(PSQ_VAR,                93,"CREATE PROCEDURE (declaration)")\
_DEFINE(PSQ_WHILE,              94,"WHILE")\
_DEFINE(PSQ_ASSIGN,             95,"ASSIGNMENT")\
_DEFINE(PSQ_DIRCON,             96,"DIRECT CONNECT")\
_DEFINE(PSQ_DIRDISCON,          97,"DIRECT DISCONNECT")\
_DEFINE(PSQ_DIREXEC,            98,"DIRECT EXECUTE IMMEDIATE")\
_DEFINE(PSQ_REMOVE,             99,"REMOVE [TABLE|VIEW]")\
_DEFINE(PSQ_0_CRT_LINK,        100,"CREATE LINK" /*MUST MATCH DB_CRT_LINK value in <dbdbms.h>*/ )\
_DEFINE(PSQ_REG_LINK,          101,"REGISTER...AS LINK" /*MUST MATCH DB_REG_LINK value in <dbdbms.h>*/ )\
_DEFINE(PSQ_SECURE,            102,"PREPARE TO COMMIT")\
_DEFINE(PSQ_REG_IMPORT,        103,"REGISTER...AS IMPORT" /*MUST MATCH DB_REG_IMPORT value in <dbdbms.h>*/ )\
_DEFINE(PSQ_REG_INDEX,         104,"REGISTER INDEX")\
_DEFINE(PSQ_REG_REMOVE,        105,"REMOVE [TABLE/INDEX]")\
_DEFINE(PSQ_DDCOPY,            106,"COPY IN (Star)")\
_DEFINE(PSQ_REREGISTER,        107,"REGISTER ... AS REFRESH")\
_DEFINE(PSQ_DDLCONCUR,         108,"SET DDL_CONCURRENCY")\
_DEFINE(PSQ_109_unused,        109,"PSQ_109_unused")\
_DEFINE(PSQ_RULE,              110,"CREATE RULE" /*MUST MATCH DB_RULE value in <dbdbms.h>*/ )\
_DEFINE(PSQ_DSTRULE,           111,"DROP RULE")\
_DEFINE(PSQ_RS_ERROR,          112,"RAISE ERROR")\
_DEFINE(PSQ_CGROUP,            113,"CREATE GROUP")\
_DEFINE(PSQ_AGROUP,            114,"ALTER GROUP...ADD")\
_DEFINE(PSQ_DGROUP,            115,"ALTER GROUP...DROP")\
_DEFINE(PSQ_KGROUP,            116,"DROP GROUP")\
_DEFINE(PSQ_CAPLID,            117,"CREATE ROLE")\
_DEFINE(PSQ_AAPLID,            118,"ALTER ROLE")\
_DEFINE(PSQ_KAPLID,            119,"DROP ROLE")\
_DEFINE(PSQ_REVOKE,            120,"REVOKE")\
_DEFINE(PSQ_GDBPRIV,           121,"GRANT ON DATABASE")\
_DEFINE(PSQ_RDBPRIV,           122,"REVOKE ON DATABASE")\
_DEFINE(PSQ_IPROC,             123,"EXECUTE INTERNAL")\
_DEFINE(PSQ_MXQUERY,           124,"SET [NO]MAX...")\
_DEFINE(PSQ_CUSER,             125,"CREATE USER")\
_DEFINE(PSQ_AUSER,             126,"ALTER USER")\
_DEFINE(PSQ_KUSER,             127,"DROP USER")\
_DEFINE(PSQ_CLOC,              128,"CREATE LOCATION")\
_DEFINE(PSQ_ALOC,              129,"ALTER LOCATION")\
_DEFINE(PSQ_KLOC,              130,"DROP LOCATION")\
_DEFINE(PSQ_CALARM,            131,"CREATE SECURITY_ALARM" /*MUST MATCH DB_ALM value in <dbdbms.h>*/ )\
_DEFINE(PSQ_KALARM,            132,"DROP SECURITY_ALARM")\
_DEFINE(PSQ_ENAUDIT,           133,"ENABLE SECURITY_AUDIT")\
_DEFINE(PSQ_DISAUDIT,          134,"DISABLE SECURITY_AUDIT")\
_DEFINE(PSQ_COMMENT,           135,"COMMENT ON")\
_DEFINE(PSQ_GOKPRIV,           136,"GRANT ACCESS,UPDATE_SYSCAT")\
_DEFINE(PSQ_ROKPRIV,           137,"REVOKE ACCESS,UPDATE_SYSCAT")\
_DEFINE(PSQ_CSYNONYM,          138,"CREATE SYNONYM" /*MUST MATCH DB_SYNONYM value in <dbdbms.h>*/ )\
_DEFINE(PSQ_DSYNONYM,          139,"DROP SYNONYM")\
_DEFINE(PSQ_SET_SESS_AUTH_ID,  140,"SET SESSION AUTHORIZATION")\
_DEFINE(PSQ_ENDLOOP,           141,"ENDLOOP")\
_DEFINE(PSQ_SLOGGING,          142,"SET [NO]LOGGING")\
_DEFINE(PSQ_SON_ERROR,         143,"SET SESSION ON_ERROR")\
_DEFINE(PSQ_UPD_ROWCNT,        144,"SET UPDATE_ROWCOUNT")\
_DEFINE(PSQ_145_unused,        145,"PSQ_145_unused")\
_DEFINE(PSQ_DGTT,              146,"DECLARE GLOBAL TEMPORARY TABLE")\
_DEFINE(PSQ_DGTT_AS_SELECT,    147,"DECLARE GLOBAL TEMPORARY TABLE")\
_DEFINE(PSQ_GWFTRACE,          148,"SET TRACE POINT GWXXX")\
_DEFINE(PSQ_CONS,              149,"ANSI CONSTRAINT declaration" /*MUST MATCH DB_CONS value in <dbdbms.h>*/ )\
_DEFINE(PSQ_ALTERTABLE,        150,"ALTER TABLE")\
_DEFINE(PSQ_CREATE_SCHEMA,     151,"CREATE SCHEMA")\
_DEFINE(PSQ_REGPROC,           152,"EXECUTE REGISTERED DBP")\
_DEFINE(PSQ_DDEXECPROC,        153,"EXECUTE PROCEDURE (Star)")\
_DEFINE(PSQ_CRESETDBP,         154,"CREATE PROCEDURE (set-of)")\
_DEFINE(PSQ_CREATE,            155,"CREATE TABLE")\
_DEFINE(PSQ_DROP_SCHEMA,       156,"DROP SCHEMA")\
_DEFINE(PSQ_SXFTRACE,          157,"SET TRACE POINT SXXXX")\
_DEFINE(PSQ_SET_SESSION,       158,"SET SESSION ...")\
_DEFINE(PSQ_ALTAUDIT,          159,"ALTER SECURITY_AUDIT")\
_DEFINE(PSQ_CPROFILE,          160,"CREATE PROFILE")\
_DEFINE(PSQ_DPROFILE,          161,"DROP PROFILE")\
_DEFINE(PSQ_APROFILE,          162,"ALTER PROFILE")\
_DEFINE(PSQ_MXSESSION,         163,"SET [NO]MAX SESSION")\
_DEFINE(PSQ_164_unused,        164,"PSQ_164_unused")\
_DEFINE(PSQ_SETROLE,           165,"SET ROLE")\
_DEFINE(PSQ_RGRANT,            166,"GRANT ROLE")\
_DEFINE(PSQ_RREVOKE,           167,"REVOKE ROLE")\
_DEFINE(PSQ_STRANSACTION,      168,"SET TRANSACTION")\
_DEFINE(PSQ_XA_ROLLBACK,       169,"ROLLBACK (distributed)")\
_DEFINE(PSQ_XA_COMMIT,         170,"COMMIT (distributed)")\
_DEFINE(PSQ_XA_PREPARE,        171,"PREPARE TO COMMIT (distributed)")\
_DEFINE(PSQ_ATBL_ADD_COLUMN,   172,"ALTER TABLE (add column)")\
_DEFINE(PSQ_ATBL_DROP_COLUMN,  173,"ALTER TABLE (drop column)")\
_DEFINE(PSQ_REPEAT,            174,"REPEAT")\
_DEFINE(PSQ_FOR,               175,"FOR")\
_DEFINE(PSQ_RETROW,            176,"RETURN ROW")\
_DEFINE(PSQ_SETRANDOMSEED,     177,"SET RANDOM_SEED")\
_DEFINE(PSQ_ALTERDB,           178,"ALTER DATABASE")\
_DEFINE(PSQ_CSEQUENCE,         179,"CREATE SEQUENCE" /*MUST MATCH DB_SEQUENCE value in <dbdbms.h>*/ )\
_DEFINE(PSQ_ASEQUENCE,         180,"ALTER SEQUENCE")\
_DEFINE(PSQ_DSEQUENCE,         181,"DROP SEQUENCE")\
_DEFINE(PSQ_ATBL_ALTER_COLUMN, 182,"ALTER TABLE (alter column)")\
_DEFINE(PSQ_SETUNICODESUB,     183,"SET [no]UNICODE_SUBSTITUTION")\
_DEFINE(PSQ_GCA_XA_START,      184,"PSQ_GCA_XA_START")\
_DEFINE(PSQ_GCA_XA_END,        185,"PSQ_GCA_XA_END")\
_DEFINE(PSQ_GCA_XA_PREPARE,    186,"PSQ_GCA_XA_PREPARE")\
_DEFINE(PSQ_GCA_XA_COMMIT,     187,"PSQ_GCA_XA_COMMIT")\
_DEFINE(PSQ_GCA_XA_ROLLBACK,   188,"PSQ_GCA_XA_ROLLBACK")\
_DEFINE(PSQ_FREELOCATOR,       189,"FREE LOCATOR")\
_DEFINE(PSQ_ATBL_RENAME_COLUMN,190,"PSQ_ATBL_RENAME_COLUMN")\
_DEFINE(PSQ_ATBL_RENAME_TABLE, 191,"PSQ_ATBL_RENAME_TABLE")\
_DEFINE(PSQ_SETBATCHCOPYOPTIM, 192, "SET BATCH_COPY_OPTIM")\
_DEFINE(PSQ_CREATECOMP,        193,"SET CREATE_COMPRESSION")\
_ENDDEFINE
#define _DEFINE(n,v,x) n=v,
#define _ENDDEFINE PSQ_MODE_MAX
	PSQ_MODES_MACRO
#undef _DEFINE
#undef _ENDDEFINE
} PSQ_MODE;

typedef struct _PSQ_CB
{
    struct _PSQ_CB  *psq_next;		/* forward pointer */
    struct _PSQ_CB  *psq_prev;		/* backward pointer */
    SIZE_TYPE	    psq_length;		/* length of this structure */
    i2		    psq_type;		/* type code for this structure */
#define                 PSQCB_CB        0   /* 0 until real code is assigned */
    i2		    psq_s_reserved;	/* possibly used by mem mgmt */
    PTR		    psq_l_reserved;
    PTR		    psq_owner;		/* owner for internal memory mgmt */
    i4		    psq_ascii_id;	/* so as to see in dump */
#define                 PSQCB_ASCII_ID  CV_C_CONST_MACRO('P', 'S', 'Q', 'B')
    i4		    psq_sesize;		/* Session control block size */
    DB_LANG         psq_qlang;          /* Query language id */
    DB_DECIMAL      psq_decimal;        /* Character to use as decimal point */
    DB_DISTRIB      psq_distrib;        /* TRUE iff distributed backend */
    i4              psq_version;        /*
					** For PSQ_STARTUP operation this is
					** the capabilities of the server.
					** else the version number of parser
					*/
                                        /* Server is at least C2 secure. */
#define                 PSQ_C_C2SECURE  0x0001

    PSQ_MODE	    psq_mode;           /* Query/statement mode */
    PTR		    psq_qid;            /* Id of query stored in QSF */
    PSY_QID	    psq_txtid;		/* Id for getting text from iiqrytext */
    DB_TAB_ID       psq_table;          /* Table id for deleting rng vars, eg */
    DB_CURSOR_ID    psq_cursid;		/* Cursor id */
    DB_OWN_NAME	    psq_als_owner;	/* Set to procedure owner if the
					** previous field is TRUE.  Together
					** with psq_cursid it provides a full
					** procedure alias.
					*/
    DB_TAB_NAME	    psq_tabname;	/* A generic table name.  Currently
					** set by SCF when passing cursor
					** DELETE data to PSF.
					*/
    PTR		    psq_dbid;		/* Database id */
    QSO_OBID	    psq_result;		/* result of parsing query (in QSF) */
    i4		    psq_mxsess;		/* Max. sessions at one time */
    DB_ERROR	    psq_error;		/* Info on errors */
    CS_SID	    psq_sessid;		/* Session id; */
    PTR		    psq_server;		/* address of server control block */
    PTR		    psq_adfcb;		/* Ptr to session's ADF_CB */
    i4		    psq_psscb_offset;	/* Offset to any session's PSS_SESBLK
					** from SCF's SCB
					*/
    DB_TAB_OWN	    psq_user;		/* User name of current user */
    DB_TAB_OWN	    psq_dba;		/* User name of dba */
    DB_TAB_OWN	    psq_group;		/* Group id of current user */
    DB_TAB_OWN	    psq_aplid;	        /* Application id of current user */
    i4		    psq_idxstruct;	/* Structure to use for indexes */
    DB_MONY_FMT	    psq_mnyfmt;		/* Money format from set command */
    DB_DATE_FMT	    psq_dtefmt;		/* Date format from set command */
    i4		    psq_udbid;		/* Unique database id */
    i4		    psq_ustat;		/* User status flags from SCS_ICS */
    i4		    psq_random_seed;	/* Random seed value 	*/
    float	    psq_maxmemf;	/* max proportion of PSF mem available to
					** any one session */
    QSO_OBID	    psq_txtout;		/* QSF handle for PSF text output, meant
					** for psy_call, used for CREATE PROC stmt.
					*/
    PTR             psq_pnode;          /*
					** Pointer to current procedure
					** header and thus statement header
					*/
    PST_INFO        *psq_info;    	/* info needed by the parser while 
                               		** parsing query text of QEA_EXEC_IMM
                                	** action header. See PST_EXEC_IMM.
                               		*/
    PTR		    psq_cp_qefrcb;	/* stashed QEF_RCB for insert to copy 
					** optimization, see comment in scs.h */
    i4		    psq_qso_lkid;	/* lock ID for QSF */
    i2		    psq_parser_compat;	/* Parser compatability flags.
					** General flags to make the parser
					** do "stuff" to be compatible with
					** older clients, unusual situations,
					** etc.
					*/
	/* Normally, decimal literals are DECIMAL.  If PSQ_FLOAT_LITS is
	** set, all numeric literals are ints or floats;  no packed decimal.
	*/
#define	    PSQ_FLOAT_LITS	0x0001

	/* Tids are treated as i8's from r3 on.  Prior to r3, tids were
	** i4's.  If PSQ_I4_TIDS is set, a tid column in a select result
	** list is typed as i4, not i8.  (Only applies to outermost
	** selects, not DB proc selects, create/select, insert/select,
	** or subquery selects.)
	*/
#define	    PSQ_I4_TIDS		0x0002


    i2		    psq_create_compression; /* Place to pass create-compression
					** startup config option: passed
					** to parser facility CB.
					*/
    u_i4	    psq_flag;		/*
					** holds assorted bitflag instructions,
					** defined below:
					*/

					/*
					** set ==> disregard error condition
					** when performing requested action
					*/
#define	    PSQ_FORCE			    0x0001L

					/*
					** set ==> means get query from
					** iiqrytext;
					** ATTN: this flag appears to be unused
					*/
#define	    PSQ_IIQRYTEXT		    0x0002L

					/*
					** set ==> psq_als_owner is set and
					** procedure recreation should
					** (1) use the "alias owner" and
					** (2) not validate permissions
					** associated with the procedure
					** name/owner.
					** These 2 fields are filled by SCS
					** and checked by psq_recreate().
					*/
#define	    PSQ_ALIAS_SET		    0x0004L

					/* set ==> close all cursors */
#define	    PSQ_CLSALL			    0x0008L

					/* set ==> allow catalog updates */
#define	    PSQ_CATUPD			    0x0010L

					/* set ==> issue warnings */
#define	    PSQ_WARNINGS		    0x0020L

					/*
					** set ==> DELETE or UPDATE had no
					** qualification and SQL stmnt
					*/
#define	    PSQ_ALLDELUPD		    0x0040L

					/*
					** set ==> DBA may DROP anybody's
					** tables
					*/
#define	    PSQ_DBA_DROP_ALL		    0x0080L

					/* 6.4 compatiblility mode */
					/* for handling ambig replace */

#define		PSQ_AMBREP_64COMPAT	0x0100L

					/*
					** set if OPF memory management can
					** cause server deadlock, should cause 
					** PSF to disallow notimeout on SCONCUR
					** catalogs, iistatistics, iihistogram -
					** required only for server startup
					*/
#define		PSQ_NOTIMEOUTERROR	    0x0200L

					/*
					** set if the new session is with an
					** older FE which requires that PSF
					** strip NLs inside string constants
					*/
#define         PSQ_STRIP_NL_IN_STRCONST    0x0400L

					/*
					** if set, PSF will allow
					** UPDATE/DELETE/INSERT on indexes
					** marked as system (not extended)
					** catalogs
					*/
#define		PSQ_REPAIR_SYSCAT	    0x0800L

					/*
					** set ==> Star thread is in "direct
					** connect" mode
					*/
#define		PSQ_DIRCONNECT_MODE	    0x1000L
					/*
					** indicate that server is in a retry
					** mode, so PSF should assure that all
					** RDF returned information is "fresh"
					** by setting RDR2_REFRESH.
					*/
#define		PSQ_REDO		    0x2000L
					/*
					** running UPGRADEDB - "-A6" flag
					*/
#define		PSQ_RUNNING_UPGRADEDB	    0x4000L

					/*
					** default mode of updatable cursor is
					** DIRECT - this will be specified at
					** server startup
					*/
#define		PSQ_DFLT_DIRECT_CRSR	    0x8000L

					/*
					** do not flatten queries involving
					** singleton subselects - this will be
					** specified at server startup
					*/
#define		PSQ_NOFLATTEN_SINGLETON_SUBSEL	0x010000L

					/*
					** owner.procedure, owner is stored in
					** psq_als_owner.  Note psq_als_owner
					** is a reused field.
					*/
#define		PSQ_OWNER_SET		    0x020000L

					/*
					** do not check permissions for this 
					** query--used for system-generated
					** queries 
					*/
#define		PSQ_NO_CHECK_PERMS	    0x040000L

					/*
					** create tables without journaling
					** unless the user has requested it.
					** Default server behaviour is to
					** create tables with journaling unless
					** the user has requested not to.
					*/
#define		PSQ_NO_JNL_DEFAULT	    0x080000L

					/*
					** CBF says cache dynamic is ON
					*/
#define		PSQ_CACHEDYN		0x0100000L

					/*
					** Enable row security audit by
					** default
					*/
#define		PSQ_ROW_SEC_AUDIT	0x0200000L

					/*
					** Provide a row security key by default
					*/
#define		PSQ_ROW_SEC_KEY		0x0400000L

					/*
					** Don't raise cardinality error with
					** flattened singleton selects in
					** WHERE clause. (This to cover legacy
					** behaviour)
					*/
#define		PSQ_NOCHK_SINGLETON_CARD 0x0800000L

					/* verifydb will enable $ingres to
					** drop/add constraint on tables owned
					** by other users.
					*/

#define		PSQ_INGRES_PRIV	        0x1000000L

#define		PSQ_RULE_DEL_PREFETCH	0x2000000L
					/* prefetch strategy on delete */

/* User is allowed to run "set trace record" */
/* NOTE: psq_flag has space in Ingres 2.6 - move to psq_flag2 in main */
#define		PSQ_ALLOW_TRACEFILE_CHANGE	0x4000000L

					/* No user passwords */

#define		PSQ_PASSWORD_NONE	0x010000000L

					/* No roles */

#define		PSQ_ROLE_NONE		0x020000000L

					/* Roles  need  passwords */

#define		PSQ_ROLE_NEED_PW	0x040000000L

					/* Session can select all */

#define		PSQ_SELECT_ALL		0x080000000L

					
    SIZE_TYPE	psq_mserver;	/* PSF memory pool parameter */

    i4	    psq_ret_flag;	/* return useful info to the caller */

					/*
					** set effective user id to the
					** value of SESSION_USER - essentially
					** a NOOP; can be used when
					** PSQ_CB.psq_mode is set to
					** PSQ_SET_SESS_AUTH_ID
					*/
#define		PSQ_USE_SESSION_USER	0x0001L

					/*
					** set effective user id to the "real"
					** user; can be used when
					** PSQ_CB.psq_mode is set to
					** PSQ_SET_SESS_AUTH_ID
					*/
#define		PSQ_USE_SYSTEM_USER	0x0002L

					/*
					** set effective user id to the value
					** that was in effect at session startup
					** (i.e. the value of INITIAL_USER);
					** can be used when PSQ_CB.psq_mode is
					** set to PSQ_SET_SESS_AUTH_ID
					*/
#define		PSQ_USE_INITIAL_USER	0x0004L
					
					/*
					** set session authorization id to the
					** value of CURRENT_USER - until we
					** start supporting MODULEs with module
					** identifiers this will also be a NOOP;
					** can be used when PSQ_CB.psq_mode is
					** set to PSQ_SET_SESS_AUTH_ID
					*/
#define		PSQ_USE_CURRENT_USER	0x0008L
					/*
					** flags related to conversion of 
					** repeated execution of dynamic insert
					** in batch mode to copy.
					** continue copy if current statement
					** is same as last. End copy if it is not.
					*/
#define		PSQ_CONTINUE_COPY		0x0010L
#define		PSQ_FINISH_COPY			0x0020L

#define		PSQ_SESS_CACHEDYN	0x0040L
#define		PSQ_SESS_NOCACHEDYN	0x0080L

#define		PSQ_ON_USRERR_NOROLL		0x0100L
					/*
					** Setting ON_ERROR status
					*/
#define		PSQ_SET_ON_ERROR		0x0200L
#define		PSQ_ON_USRERR_ROLLBACK		0x0400L
					/*
					** Adding privileges 
					*/
#define		PSQ_SET_APRIV			0x0800L
					/*
					** Dropping privileges 
					*/
#define		PSQ_SET_DPRIV			0x1000L
					/*
					** Use privileges
					*/
#define		PSQ_SET_PRIV			0x2000L
					/*
					** Use default privileges 
					*/
#define		PSQ_SET_DEFPRIV			0x4000L
					/*
					** Use all privileges
					*/
#define		PSQ_SET_ALLPRIV			0x8000L
					/*
					** No privileges
					*/
#define		PSQ_SET_NOPRIV			0x10000L
					/*
					** Session Priority
					*/
#define		PSQ_SET_PRIORITY		0x20000L
					/*
					** Session Description
					*/
#define		PSQ_SET_DESCRIPTION		0x40000L
					/*
					** Initial priority
					*/
#define		PSQ_SET_PRIO_INITIAL		0x80000L
					/*
					** Maximum priority
					*/
#define		PSQ_SET_PRIO_MAX		0x100000L
					/*
					** Minimum priority
					*/
#define		PSQ_SET_PRIO_MIN		0x200000L

					/* No Role */
#define		PSQ_SET_NOROLE			0x400000L

					/* Role password */
#define		PSQ_SET_ROLE_PASSWORD		0x800000L

					/* Distributed transaction id */
#define         PSQ_SET_XA_XID                 0x1000000L

					/* Session access mode */
#define         PSQ_SET_ACCESSMODE             0x2000000L

					/* Session isolation level set */
#define         PSQ_SET_ISOLATION              0x4000000L

					/* Parsing ORDER BY (need for FIRST) */
#define		PSQ_PARSING_ORDER_BY		0x08000000L
					/* Setting ON_LOGFULL abort */
#define		PSQ_SET_ON_LOGFULL_ABORT	0x10000000L
					/* Setting ON_LOGFULL commit */
#define		PSQ_SET_ON_LOGFULL_COMMIT	0x20000000L
					/* Setting ON_LOGFULL notify */
#define		PSQ_SET_ON_LOGFULL_NOTIFY	0x40000000L
					/* insert converted to copy */
#define		PSQ_INSERT_TO_COPY		0x80000000L


    DD_LDB_DESC	    *psq_ldbdesc;	/*
					** will point to structure to contain
					** node, database, and dbms to which
					** one has requested a direct connection
					** or where a table (in CREATE/DEFINE
					** LINK) is located.
					*/
    DD_LDB_DESC	    *psq_dfltldb;	/*
					** will point to structure to contain
					** default node, database, and dbms.
					*/
    DB_TAB_ID	    psq_set_procid;	/* passed by QEF when it calls PSF to
					** recreate a query plan for a
					** set-input proc; PSF must verify that
					** the ID matches so that a proc with
					** the same name but different inputs
					** is not found by accident.
					*/
    DB_TAB_ID	    psq_set_input_tabid;
					/* passed by QEF when it calls PSF to
					** recreate a query plan for a

					** set-input proc; PSF puts this temp
					** table id in the range entry for the
					** set so that OPF can have the id
					** of the current set-input temp table.
					*/
    u_i4	    *psq_dbxlate;	/*
					** Case translation semantics flags
					** See <cuid.h> for masks.
					*/
    DB_OWN_NAME	    *psq_cat_owner;	/*
					** Catalog owner name.  Will be
					** "$ingres" if regular ids are lower;
					** "$INGRES" if regular ids are upper.
					*/
    i4	    psq_privs;		/* Privilege mask */
    char	    *psq_desc;		/* Session description */
    char            *psq_distranid;     /* Session distributed transaction id */
    i4	    psq_priority;	/* Session priority */
     i4         psq_isolation;      /* Session isolation level */
    i4         psq_accessmode;     /* Session access mode */
    DB_OWN_NAME     psq_rolename;	/* Role name for SET ROLE */
    DB_PASSWORD     psq_password;       /* Password, e.g for SET ROLE */

    i4	    psq_alter_op;	/* Flags for psq_alter */

					/* Session role is being changed */

#define		PSQ_CHANGE_ROLE		0x000000001L

					/* Reset priv/status mask
					*/
#define		PSQ_CHANGE_PRIVS	0x000000002L
    PSQ_STMT_INFO	*psq_stmt_info; /* return prepared statement info */

    u_i4            psq_flag2;          /*
                                        ** holds assorted bitflag instructions,
                                        ** defined below:
                                        ** Impossible to add additional values
                                        ** to psq_flag.
					*/

					/*
					** set ==> disregard error condition
					** when performing requested action
					*/
#define	    PSQ_RULE_UPD_PREFETCH	0x0001L
#define     PSQ_DFLT_READONLY_CRSR	0x0002L
#define	    PSQ_BATCH			0x0004L /* query is part of a batch */
#define	    PSQ_LAST_BATCH		0x0008L /* end of batch */
#define	    PSQ_COPY_OPTIM		0x0010L /* use copy optim */
					 /*
					 ** set ==> set if the default cursor 
					 ** mode is readonly 
					 */
#define	    PSQ_LOCATOR			0x00020L
					/* 
					** set ==> GCA_LOCATOR_MASK is on
					*/
#define     PSQ_MONEY_COMPAT		0x0008L
					/*
                                        ** Set if constant strings are to be
                                        ** parsed for money.
                                        */
    bool	    psq_vch_prec;	/* varchar precedence */
 
                                        /*
                                        ** set ==> disregard error condition
                                        ** when performing requested action
                                        */
#define     PSQ_RULE_UPD_PREFETCH       0x0001L
    i2                  psq_usub_stat;  /* Status of unicode substitution */
    char                psq_usub_char[4];  /* Unicode substitution character */
    DB_DATA_VALUE	psq_locator[10];   /* Array of locators for FREE LOCATOR */

    /*
    ** These two details appended to enable QSF to call PSF w.r.t. pst_resolve
    */
    void               *psq_qsfcb;      /* QSF control block */
    void               *psq_qnode;      /* QSF node details for */

    char	       *psq_server_class; /* server_class */
} PSQ_CB;

/*
**  Definition of query tree.
*/

#define                 PST_NUMVARS     DB_MAXVARS-2
					/* Number of range vars. per query
					** (1 saved for result, 1 saved for
					** something else???) */

/*
** Name: PST_J_MASK - this mask will be used to indicate joins for which a given
**		      range var is an outer/inner relation
**
** Description: If n'th (1<=n<=DB_MAXVARS; 0-th bit will not be used since join id
** of 0 was defined to indicate NO JOIN) bit is set in a range table entry, this
** entry was an outer/inner relation in the join.
**
** History:
** 
**  31-aug-89 (andre)
**	defined
**	25-nov-02 (inkdo01)
**	    Changed to support larger range table.
*/
typedef struct _PST_J_MASK
{
    i4  pst_j_mask[PSV_MAPSIZE]; 	/* enough bits for the max. range table */
} PST_J_MASK;

/*}
** Name: PST_VAR_NODE - VAR node
**
** Description:
**      This structure defines a VAR node for a query tree.  VAR nodes
**      represent columns in tables (e.g. tab.col).  Each VAR node contains
**      a variable number, which stands for the number of the range variable
**      in the range table of the query tree it is a part of.  There is also
**      a column number, corresponding to the column number in the attribute
**      system table.  The pointer to the actual value is contained in the
**	PST_SYMBOL structure, of which this is a part.
**
**      The optimizer will modify the query tree, so that the elements of
**      this structure reference a "joinop variable number" and a
**      "joinop attribute number".  The difference is that there is one
**      level of indirection introduced by a "joinop number".  The optimizer
**      will place all attributes of all tables used in a query into a 
**      flat structure, each element of which references the original
**      (pst_vno, pst_atno) pairs.  The joinop attribute number is an index
**      into this table and will be placed in pst_atno.  The joinop
**      variable number is placed in pst_vno to reference an internally created
**      joinop range table, which is larger than the parser range table since
**      implicitly referenced indexes are added to it.
**
**	VAR nodes are usually (always?) terminal nodes, with no children.
**
** History:
**     25-sep-85 (jeff)
**          written
*/
typedef struct _PST_VAR_NODE
{
    i4              pst_vno;            /* variable number */
    DB_ATT_ID	    pst_atno;		/* attribute number */
    DB_ATT_NAME	    pst_atname;		/* attribute name */
} PST_VAR_NODE;
/*}
** Name: PST_FWDVAR_NODE - FWDVAR node
**
** Description:
**      This structure defines a FWDVAR node for a query tree.  FWDVAR nodes
**      are essentially the same as PST_VAR nodes except that they retain
**	correlation or forward reference information that PST_VAR nodes have
**	no need of.
**
**	Note:	A PST_VAR node must always have its nodes initialised and is
**	always resolved. On the otherhand a PST_FWDVAR will start off potentially
**	unknown either as a forward referenced correllated variable, (pst_fwdref
**	set) or as an implied forward table reference (pst_fwdref and pst_atname
**	set). Thus a FWDVAR node will at some post-processing phase be updated
**	with a valid pst_rngvar and then it will also be updated with valid vno,
**	atno & atname - at which point it could be transformed into a PST_VAR.
**
** History:
**	27-Oct-2009 (kiria01) SIR 121883
**          Added for scalar sub-queries.
*/
typedef struct _PST_FWDVAR_NODE
{
    /* MATCH PST_VAR_NODE */
    i4              pst_vno;            /* variable number */
    DB_ATT_ID	    pst_atno;		/* attribute number */
    DB_ATT_NAME	    pst_atname;		/* attribute name */
    /* ENDMATCH PST_VAR_NODE */

    struct _PSS_TBL_REF	*pst_fwdref;	/* Fwd ref pending rngvar resolve */
    struct _PSS_RNGTAB	*pst_rngvar;	/* Address of rngvar */
    i4		    pst_qualdepth_ref;	/* pss_qualdepth at which reference made */
    bool	    pst_star_seen;	/* C.* or just * seen */
} PST_FWDVAR_NODE;

/*}
** Name: PST_CRVAL_NODE - Current value node
**
** Description:
**	This structure defines a CURVAL node for a query tree.  CURVAL
**	nodes represent the current values for cursors (e.g. the value of
**	column Y in cursor X).  Each CURVAL node contains a cursor id and
**	a column number.  The cursor ID is just window dressing, all
**	we really need is the column number.  (because CURVAL's only
**	show up in UPDATE WHERE CURRENT OF statements, and there's
**	only one possibility for the cursor ID in such a statement.)
**
**	CURVAL nodes are really just VAR nodes that use a different
**	mechanism to identify the column.  (kschendel:  in fact,
**	I really don't know why we bother with them;  VAR's in the SET
**	expressions of the update are converted to CURVAL's when they
**	refer to the cursor table, and then OPF has to have a whole
**	separate cqual routine to handle them.)
**
** History:
**     07-oct-85 (jeff)
**          written
*/
typedef struct _PST_CRVAL_NODE
{
    DB_CURSOR_ID    pst_cursor;		/* cursor id */
    DB_ATT_ID	    pst_curcol;		/* cursor attribute number */
} PST_CRVAL_NODE;

/*
** Name: PST_J_ID - join id
**
** Description: Will be used to store join id.
**
** History:
** 
**  31-aug-89 (andre)
**	defined
*/
typedef i2	PST_J_ID;
#define		PST_NOJOIN	(PST_J_ID) 0

/*}
** Name: PST_RT_NODE - Structure for query tree root nodes
**
** Description:
**	This structure defines ROOT, AGHEAD, or SUBSEL nodes for query trees.
**	ROOT nodes appear at the head of every query tree; AGHEAD nodes
**	appear at the head of every aggregate and aggregate function.
**	SUBSEL nodes describe subselects in SQL statements.
**
**	The range-variable information in this node is filled in by the
**	optimizer, not the parser.
**
**	The pst_project field is used only in AGHEAD nodes.  When TRUE, it
**	means that projection should be applied to the aggregate before
**	qualification. When FALSE, it means that projection should be applied
**	after the qualification.  FALSE corresponds to the "only where" syntax
**	in aggregate functions.  (Note that SQL aggregates when expressed
**	in QUEL-ish parse-tree notation are "only where" aggs.)
**
**	The left of a ROOT points to a resdom list, and the right
**	points to the WHERE qualification if any.  For UPDATE, the
**	resdom list corresponds to the SET clause (see RESDOM).
**	For INSERT, the resdom list corresponds to the column values
**	being explicitly assigned by the insert.
**
**	SUBSEL nodes are exactly like ROOTs with a different type code
**	to indicate that they aren't the outermost select.
**
**	The left of an AGHEAD points to either an AOP or a BYHEAD.
**	If it's an AOP, there's no BY-grouping.  (See resdoms for the
**	BYHEAD.)  The right of an AGHEAD is the WHERE qualification for
**	that aggregate.  This representation is suited to QUEL, where
**	each aggregate might have its own BY-list.  SQL aggregates are
**	exploded out to look like QUEL (i.e. the group-by list and
**	where clause is duplicated for each aggregate).
**
**	If there is no WHERE for a ROOT or AGHEAD, the right hand isn't
**	NULL, as one might think;  instead it points to a QLEND.
**
**	In a sense, ROOT/AGHEAD/SUBSEL nodes root table-returning subtrees.
**	A ROOT or SUBSEL heads a query tree that generates a result table and
**	sends it somewhere.  (In the case of INSERT, UPDATE, or DELETE,
**	"somewhere" might be into an existing table, or into nothingness.)
**	An AGHEAD heads a query tree that generates an aggregate result
**	table consisting of one aggregate (AOP) and the BY-list values
**	(resdoms under the BYHEAD).  This is conceptual, of course,
**	we don't expect to actually materialize things this way.
**
**	Note that the above description for AGHEAD refers to what comes
**	out of the parser.  OPA will piss with the tree to combine
**	aggregates (with the same by-list) that can be evaluated
**	together, and things end up cross-linked in strange ways.
**
**	By the way, you might think that the very very top of a query
**	tree is a ROOT.  You would be wrong.  See PST_PROCEDURE,
**	PST_STATEMENT, and PST_QTREE;  those are at the very top.
**	They don't really count as parse tree nodes, though;  they are
**	header structures rather than nodes.
**
** History:
**     25-sep-85 (jeff)
**          created
**     05-aug-86 (jeff)
**	    Added pst_project field.
**     16-mar-89 (andre)
**	    Renamed pst_rvrc (a filed that was never used) to pst_mask1
**	    (which will, hopefully, be used)
**     07-oct-94 (mikem)
**	    Added the the PST_4IGNORE_DUPERR bit to be used in pst_mask1 if 
**	    the ps201 tracepoint has been executed in this session.  This 
**	    traceflag will be used by the CA-MASTERPIECE product so that 
**	    duplicate key errors will not shut down all open cursors (whenever 
**	    the "keep cursors open on errors" project is integrated this change
**          will no longer be necessary).
**	26-nov-02 (inkdo01)
**	    Expansion of range table - pst_tvrm/lvrm are now PST_J_MASK.
**      15-aug-2006 (huazh01)
**          Add PST_6HAVING for query contains two aggregate in its
**          having clause. This fixes b116202.
**	18-Mar-2010 (kiria01) b123438
**	    Added PST_9SINGLETON for singleton aggregate support.
*/
typedef struct _PST_RT_NODE
{
	/*     For SQL pst_tvrc and pst_tvrm, are derived directly from the
        ** tables defined in the "FROM list", note that a table does not
        ** need to be referenced in a var node, to be in the from list map.
        ** Thus, it is not necessarily true that pst_tvrm== pst_lvrm || pst_rvrm
        ** For QUEL pst_tvrc and pst_tvrm give the count and map of all the 
        ** range variables that are directly used in this tree. By 
        ** 'directly used' I mean that there aren't any root, aghead, 
        ** or subquery nodes between all the var nodes and this node.  
	**     It is important to note that the range table in the
	** query header in NOT equivilent to the 'from' list, since it will
	** hold the range vars for all the subqueries.
	*/
    i4              pst_tvrc;
    PST_J_MASK	    pst_tvrm;	    /* SQL from list */

	/* The # and bitmap of range vars referenced directly in the left
	** tree (target list).
	*/
    i4              pst_lvrc;
    PST_J_MASK	    pst_lvrm;

    i4              pst_mask1;
	/* In order to avoid a new query tree version for corelated subselects
	** in views stored in the catalogs, this flag must be set for the
	** PST_2CORR_VARS_FOUND to be valid */
#define		PST_1CORR_VAR_FLAG_VALID    0x01
	/* This flag indicates that the subselect or aggregate associated with
	** this root node has corelated variables */
#define		PST_2CORR_VARS_FOUND	    0x02
	/*
        ** this flag indicates that this ROOT/SUBSEL node is the nearest
	** such node to AGHEAD node whose left child is a BYHEAD
	*/
#define         PST_3GROUP_BY               0x04
	/* 
	** this flag indicates that this (sub)tree should ignore the duplicate
	** key/row insert error of dmf and just return "0" rows as the result
	** with no error.  This is the QUEL semantic already in placed 
	** "triggered" by pst_qlang.
	*/
#define		PST_4IGNORE_DUPERR	    0x08
	/*
	* this flag indicates a "...having [not] exists..." style query
	*/
#define		PST_5HAVING		    0x10
        /* 
        ** this flag indicates there are two aggregates in query's
        ** having clause.
        */
#define         PST_6HAVING                 0x20
        /* 
        ** this flag indicates a "...having [not] <agg>..." style query
        */
#define         PST_7HAVING                 0x40
        /* 
        ** this flag indicates a SUBSEL being elided.
        */
#define		PST_8FLATN_SUBSEL	    0x80
        /* 
        ** this flag indicates a singleton SUBSEL.
        */
#define		PST_9SINGLETON		    0x100

	/* The bitmap of range vars referenced directly in the right
	** tree (where clause).
	*/
    PST_J_MASK	    pst_rvrm;		/* used in OPF to track range var
					** refs on RHS of tree */

    DB_LANG	    pst_qlang;		/* query language for this subtree. */

    i4              pst_rtuser;         /* TRUE if root of user-generated qry */
    i4		    pst_project;	/* TRUE means project aggregates */
    	/* Pst_dups defines whether or not duplicates should be removed from
	** the results of this (sub)tree, or if it matters.
	*/
    i2		    pst_dups;
#define	PST_ALLDUPS		1
#define	PST_NODUPS		2   /* ie UNIQUE or DISTINCT */
#define	PST_DNTCAREDUPS	3

    PST_J_ID	    pst_ss_joinid;	/* Subselect joinid identifying this
					** subselect to its outer */

    PST_UNION	    pst_union;		/* groups union related info */
} PST_RT_NODE;

/*}
** Name: PST_OP_NODE - Operator Node
**
** Description:
**      This structure defines AND, OR, and various xOP nodes.  Each of
**	these corresponds to an operator in the query language.  We have
**	COP, UOP, BOP, and MOP for zero, one, two, and many-operand
**	operators.  There's also AOP for a one or two operand aggregate
**	function.  Each operator has an operator number corresponding to the
**	name of the operator, a function instance id telling which abstract
**	datatype function to call to run this function, and function instance
**	ids telling which ADF functions to call to convert the left and right
**	operands to the right types to be used by this operator.
**
**	Normally the first operand is on the left and the second (if any)
**	is on the right.  MOP's are special; the first MOP arg is on the RHS,
**	and the LHS is a chain of OPERAND nodes;  each operand's RHS points
**	to the next actual argument.  Note the difference in operand-1 position
**	as opposed to UOP and BOP (see below).  An AOP looks like a UOP
**	or a BOP, as appropriate.  (AOP distinct-ness is a flag, not an
**	operator operand.)
**
**	The DECIMAL(exp,p,s) operator is goofy;  it looks like a three
**	operand function, and one might think that it becomes a MOP, but
**	in fact the p,s bits are squished into one argument and it
**	compiles to a BOP.
**
**	AND and OR nodes are special cases binary operators.  They don't have
**	function instance ids, and their operands must be comparison operators.
**
**	The pst_opparm field is used only by aggregates, and is filled in by the
**	optimizer.  It contains the aggregate parameter number, which is in the
**	same space as the parameter number in the constant node.
**
**	Summary of UOP a1, a1 BOP a2, and MOP(a1,a2,...aN)  operators:
**
**	  UOP			BOP				MOP
**	 /		       /  \			       /  \
**	a1		     a1    a2			    OPND   a1
**							   /   \
**							OPND   a2
**							...
**
**	Occasionally the right side of a BOP is tricked out to be
**	something special, such as an IN-list or a subquery.  IN-lists
**	look like a list of constants linked down the constant left
**	side (normally constants have NULL left and right children).
**
** History:
**     25-sep-85 (jeff)
**          written
**     05-aug-86 (jeff)
**	    Added pst_opparm.
**     10-sep-86 (seputis)
**          Added pst_fdesc for query compilation
**     03-jun-88 (jrb)
**	    Added possible values for pst_isescape
**     17-aug-89 (andre)
**	    Added pst_groupid to be used when dealing with
**	    outer joins.
**	07-nov-91 (andre)
**	    merge 18-dec-90 (kwatts) change:
**		Smart Disk integration: Declared pstflags in PST_OP_NODE and
**		defined one bit for Smart Disk.
**	10-jun-92 (seputis)
**	    added PST_IMPLICIT for outer join support
**	31-jan-94 (ed)
**	    added PST_OJJOIN for outer join support
**	 5-dec-00 (hayke02)
**	    Added PST_IFNULL_AOP to indicate that this AOP node is inside
**	    an ifnull(). This change fixes bug 103255.
**	12-dec-03 (inkdo01)
**	    Added PST_INLIST flag for expedited in-list processing.
**	6-Nov-2006 (kschendel) c125.dif toumi01 integrated 18-may-2009
**	    Added PST_CAST flag so that type resolution will leave the
**	    decimal p/s stuff alone.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	    Made pst_opmeta into u_i2 and widened pst_escape.
**	20-Oct-2008 (kiria01) b121098
**	    Removed comments that are now not true regarding pst_oplcnvrtid
**	    and pst_oprcnvrtid.
**	03-Nov-2009 (kiria01) b122822
**	    Added qualifier PST_INLISTSORT for PST_INLIST processing
**	13-Nov-2009 (kiria01) 121883
**	    Distinguish between LHS and RHS card checks for 01 adding
**	    PST_CARD_01L to reflect PST_CARD_01R.
*/
typedef struct _PST_OP_NODE
{
    ADI_OP_ID       pst_opno;           /* operator number */
    ADI_FI_ID       pst_opinst;         /* ADF function instance id */
    i4		    pst_distinct;	/* set to TRUE if distinct agg */
#define	PST_DISTINCT	1		/* distinct aggregate */
#define PST_NDISTINCT	0		/* regular, non distinct, aggregate */
#define PST_DONTCARE	-1		/* min, max - dups do not affect 
					** semantics
					*/
#define PST_BDISTINCT	-2		/* no ALL/DISTINCT spec at all */
    i4		    pst_retnull;	/* set to TRUE if agg should return
					** null value on empty set.
					*/
    ADI_FI_ID       pst_oplcnvrtid;     /* ADF func. inst. id to cnvrt left */
    ADI_FI_ID       pst_oprcnvrtid;     /* ADF func. inst. id to cnvrt right */
    i4		    pst_opparm;		/* Parameter number for aggregates */
    ADI_FI_DESC     *pst_fdesc;         /* ptr to struct describing operation */
	/* Pst_opmeta defines the way an operator to work on the operator
	** pst_opno. Currently this is needed only when the
	** right side of this node is a subselect (subselects can only
	** be on the right side). Instead of adding this field to the
	** op node, some new node type could have been created and
	** placed between this op node and the select. The problem with
	** this is that these modifiers effect both sides of this op node,
	** not just the select.
	*/
    u_i2	    pst_opmeta;

    /* PST_NOMODIFIER defines no additional semantics. */
#define	    PST_NOMETA		0

    /* PST_CARD_01R defines that only zero or one value can be returned
    ** from the subselect on the right side. If two or more values are returned
    ** then an error should be returned to the user.
    */
#define	    PST_CARD_01R	1

    /* PST_CARD_01L defines that only zero or one value can be returned
    ** from the subselect on the left side. If two or more values are returned
    ** then an error should be returned to the user.
    */
#define	    PST_CARD_01L	4

    /* PST_CARD_ANY defines that this operator is TRUE for each value
    ** on the left side if the operator defined by pst_opno is TRUE for
    ** any one or more of the values from the subselect on the right side.
    ** Otherwise, this comparison is FALSE or UNKNOWN.
    */
#define	    PST_CARD_ANY	2

    /* PST_CARD_ALL defines that this operator is TRUE for each value
    ** on the left side if the operator defined by pst_opno is TRUE for
    ** all of the values from the subselect on the right side. If the
    ** right side contains no values, then this operator is TRUE for each 
    ** value on the left side.
    */
#define	    PST_CARD_ALL	3

	/* Pst_escape is the escape char that the user has specified for the
	** LIKE familiy of operators. Pst_escape has valid info in it *only* if
	** pst_pat_flags has AD_PAT_HAS_ESCAPE bit set. If this bit is clear,
	** then this is a LIKE operator, but without an escape char.  If only
	** pst_pat_flags bit AD_PAT_DOESNT_APPLY is set then this is not a LIKE op.
	** pst_pat_flags shares values with adc_pat_flags except that only the
	** lower 16 bits are handled as the rest a reserved to the pattern
	** code.
	*/
    UCS2	pst_escape;
    u_i2	pst_pat_flags;		/* For flag values see AD_PAT_* flags */

    PST_J_ID	pst_joinid;

    i2		pst_flags;
#define		PST_OPMETA_DONE	1	/* If this is a PST_BOP node comparison
					** then this flag is used to track
					** whether pst_opmeta has been applied
					** into the CO node, used by OPF only */
#define		PST_IMPLICIT    2	/* used by OPF only, TRUE if
					** this was a join node introduce
					** by an implicit join operation 
					** e.g. secondary index */
#define		PST_OJJOIN	4       /* used by OPF only, TRUE if this
					** is an equi-join node which is
					** part of an outer join */
#define		PST_IFNULL_AOP	8	/* used to indicate that this PST_AOP
					** node is inside an ifnull() */
#define		PST_INLIST	16	/* Indicates that this "="/"<>" node 
					** anchors a list of "in-list" values
					** as part of in-list optimization if
					** rhs is a PST_CONST and marks that
					** IN SUBSEL implies DISTINCT */
#define		PST_XFORM_AVG	32	/* BOP of '/' that anchors a 
					** transformed avg(x) */
#define		PST_AVG_AOP	64	/* sum/count AOPs from transformed avg */
#define		PST_CAST	128	/* operator was produced by the CAST()
					** pseudo-function */
#define		PST_INLISTSORT	256	/* Indicates that this "=" node is also
					** in sorted order. This is just a
					** qualifier, PST_INLIST must also be set */ 
} PST_OP_NODE;

/*}
** Name: PST_RSDM_NODE - RESDOM and BYHEAD nodes
**
** Description:
**      This structure defines the RESDOM and BYHEAD query tree nodes.
**      RESDOM nodes stand for result domains; that is, result columns in
**      queries representing actual columns in tables or views.  BYHEAD
**	nodes appear under AGHEAD nodes in aggregate functions; their
**	left descendants are resdoms that point to the BY-expressions,
**	and their right descendants are AOP nodes representing the aggregate
**	function to be performed.  The result domains in every query are
**	numbered; this structure contains the result domain number along with
**	other miscellaneous stuff.
**
**	A resdom under a BYHEAD is sometimes called a BYDOM, since it's
**	indicating a BY-expression rather than a result-expression.
**	As output by the parser, a BYHEAD will have just one AOP on its
**	right.  (Thus, each aggregate function reference generates its
**	own little mini-tree with AGHEAD, optional BYHEAD, and AOP.)
**	The optimizer will screw around mightily with this to
**	coalesce aggs that can be produced simultaneously.
**
**	Resdoms are linked such that the leftmost entry in the result-list
**	is at the leftmost end of the RESDOM list;  the rightmost resdom
**	is at the top.  The leftmost resdom doesn't point to NULL, as one
**	might expect;  it points to a PST_TREE terminator.
**
**	Resdoms are used in the (obvious) context of a select or retrieve
**	target-list, but they are also used in the (less obvious) contexts
**	of UPDATE and INSERT.  For an update SET-list entry like
**	"col = expr", the resdom target type is ATTNO indicating that
**	the result goes into a column rather than out to the user;  the
**	resdom node itself contains the column number;  and the right
**	child is the replacement expression.  INSERTs work similarly,
**	with a resdom for each inserted column that is explicitly assigned
**	a value.  By the way, the parse tree makes no distinction between
**	INSERT VALUES and INSERT SELECT;  the former simply has constant
**	expressions under the resdoms, while the latter can have VAR
**	expressions.  (INSERT SELECT will also have differences in the
**	QTREE node at the very top, e.g. a range table.)
**
** History:
**     25-sep-85 (jeff)
**          written
**     21-feb-87 (seputis)
**          added pst_print, removed pst_bydomorig
**	27-apr-88 (stec)
**	    Modified for DB procedures.
**	25-jan-89 (neil)
**	    Added PST_RSDNO and PST_DBPARAMNO for rules.
**	26-oct-92 (jhahn)
**	    Added PST_BYREF_DBPARAMNO for DB procedures..
**	22-feb-93 (rblumer)
**	    Added pst_defid for use in CREATE TABLE AS SELECT & RETRIEVE INTO.
**	18-jun-93 (robf)
**	    Added pst_dmuflags for use in CREATE TABLE AS SELECT. 
**	23-oct-98 (inkdo01)
**	    Added PST_RESROW_COL for row producing procedures.
**	30-june-05 (inkdo01)
**	    Added PST_SUBEX_RSLT for CX optimization.
**	22-june-06 (dougi)
**	    Added PST_OUTPUT_DBPARAMNO for extended parameter mode support.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	16-mar-2007 (dougi)
**	    Added PST_RS_KEYSET flag for keyset scrollable cursors.
*/
typedef struct _PST_RSDM_NODE
{
    /* result domain number, from 1 to the number of resdoms
    */
    i4              pst_rsno;           /* result domain number */
    /* Pst_ntargno and pst_ttargtype tell where the results are supposed to
    ** go. The options are: an attribute in the result relation, a
    ** procedure local variable, a repeat query parameter number, or to
    ** the user (application). Pst_ntargno gives the result att number,
    ** the local variable number, or the repeat query parameter number.
    ** if pst_ttargtype is PST_USER then pst_ntargno is the same as
    ** pst_rsno. Pst_ttargtype value is immaterial when pst_print is
    ** not TRUE.
    */
    i4  	    pst_ntargno;
    i4  	    pst_ttargtype;
#define     PST_ATTNO		10
#define     PST_LOCALVARNO	11
#define     PST_RQPARAMNO	12
#define     PST_USER		13
#define	    PST_DBPARAMNO	14	/* Used by PST_CPROCSTMT - This
					** indicates the the target element
					** is a parameter to the DBP.
					** Pst_ntargno represents the parameter
					** number in the actual parameter list.
					** Pst_rsname is the name of the formal
					** parameter.
					*/
#define	    PST_RSDNO		15	/* Used by PST_RL_NODE */
					/*
					** Used by OPC_DMUAHD to mark
					** special B1 attributes for special
					** processing.
					*/
#define	    PST_HIDDEN		16
#define	    PST_BYREF_DBPARAMNO	17	/* Used by PST_CPROCSTMT - This
					** indicates the the target element
					** is a BYREF parameter to the DBP.
					** Pst_ntargno represents the parameter
					** number in the actual parameter list.
					** Pst_rsname is the name of the formal
					** parameter.
					*/
#define	    PST_TTAB_DBPARM     18      /* Used by PST_CPROCSTMT - Thus
					** indicates a temporary table 
					** procedure parameter. Pst_right 
					** addresses a VAR node with only the 
					** table name in pst_atname. 
					*/
#define	    PST_RESROW_COL	19	/* Used by PST_RETROW - indicates 
					** "columns" of dbproc result row
					** referenced in return row stmt.
					*/
#define	    PST_SUBEX_RSLT	20	/* Used in OPF code generation to
					** identify results of common
					** subexpressions optimized out of
					** ADF CXs 
					*/
#define	    PST_OUTPUT_DBPARAMNO 21	/* Used by PST_CPROCSTMT - same as
					** BYREF but for OUTPUT parms added 
					** for parameter modes. */

    i4		    pst_rsupdt;		/* TRUE iff cursor column for update */
    i4		    pst_rsflags;	/* Was pst_print - b116230 */
#define     PST_RS_PRINT     (0x00000001) /* SET iff printing resdom. else
					** user should not see result of
					** resdom.
					*/
#define     PST_RS_DEFAULTED (0x00000002) /* SET iff the column was not specified
					** by the user but added as part of
					** default processing. Used in INSERT
					** DISTINCT sort key choice - b116230
					*/
#define	    PST_RS_KEYSET    (0x00000004) /* SET iff the column is a keyset
					** column in a scrollable keyset
					** cursor (includes base table TID)
					*/
#define	    PST_RS_USRCONST  (0x00000008) /* set iff user provided a constant
					** for this resdom. The matching const
					** node will contain the value.
					** Set for prepared statements only.
					** Usable after pst_prmsub re-sets
					** pst_parm_no in the const node
					** either one will be set, but not
					** both
					*/
    char	    pst_rsname[DB_ATT_MAXNAME]; /* name of result column */
    DB_DEF_ID	    pst_defid;		/* used during RETRIEVE INTO and CREATE
					** TABLE AS SELECT, in order to preserve
					** default values in the new table
					*/
    i4	    pst_dmuflags;	/* DMF/DMU flags for the attribute
					** typically used when adding system
					** created attributes which are 
					** system_maintained or used for
					** security_audit/labelling etc
					*/
} PST_RSDM_NODE;

/*}
** Name: PST_SRT_NODE - SORT node
**
** Description:
**      This structure defines the SORT query tree node.  SORT nodes represent
**      the attributes in a retrieve query on which we will sort.
**
**	Sort nodes are hung off a separate list in the qtree header,
**	they do not appear under the query ROOT.
**
** History:
**     25-sep-85 (jeff)
**          written
*/
typedef struct _PST_SRT_NODE
{
    i4              pst_srvno;          /* number of attribute to sort on */
    i4              pst_srasc;          /* TRUE if ascending sort */
} PST_SRT_NODE;

/*}
** Name: PST_TAB_NODE - TABLE node
**
** Description:
**      This structure defines the TABLE query tree node.  TABLE nodes
**	represent (at least initially) explicit temporary table references 
**	in execute procedure parameters.
**
** History:
**     26-jul-96 (inkdo01)
**          written
*/
typedef struct _PST_TAB_NODE
{
    DB_OWN_NAME	    pst_owner;		/* owner of table (session for gtt's) */
    DB_TAB_NAME	    pst_tabname;	/* table name */
} PST_TAB_NODE;

/*}
** Name: PST_PMSPEC - Pattern-matching specifier
**
** Description:
**	This structure is used for specifying whether pattern-matching
**	characters are used in a constant.  There are three choices: yes, no,
**	and maybe.
**
** History:
**      20-may-86 (jeff)
**          written
*/
typedef i4  PST_PMSPEC;
#define                 PST_PMNOTUSED   (PST_PMSPEC) 1	/* PM chars not used */
#define			PST_PMUSED	(PST_PMSPEC) 2	/* PM chars used */
#define			PST_PMMAYBE	(PST_PMSPEC) 3	/* maybe they are used*/

/*}
** Name: PST_CNST_NODE - Constants and parameters
**
** Description:
**
**	The "constant" node points to a non-column scalar value.
**	It might be:
**	- a real constant constant (type USER)
**	- a prepared query parameter marker (also type USER, but with a
**	  nonzero pst_parm_no)
**	- a DB procedure scalar-param or local variable (type LOCALVARNO)
**	- a REPEATED query parameter (type RQPARAM)
**
**	Obviously some of these are constants only in a restricted sense.
**	E.g. a repeated query parameter is constant for a given
**	invocation of the repeated query with given actual-parameters.
**
**	Each parameter in a repeat query has a number assigned to it.  The
**	parameter can show up in more than one place in the query tree.
**	For example, the qrymod process can copy parameters to different parts
**	of the query tree.  Whenever a parameter shows up in more than one
**	place, the nodes representing the parameter are guaranteed to be
**	identical.
**
**	Constants that don't represent parameters have a parameter number of
**	zero.
**
**	The db_data field in the pst_dataval field of the PST_SYMBOL structure
**	will point to the actual constant for non-parameter constants.  This
**	value allocated in memory contiguous with the rest of the node.  The
**	db_data field will be NULL for parameter constants.
**
**	The optimizer can produce its own parameter nodes when it processes
**	aggregates.  It will use the same parameter number for aggregate
**	results as is used for repeat query parameters.  The pst_numparm field
**	in the PST_QTREE structure is used to distinguish where the repeat
**	query parameters end and the aggregate result parameters begin.
**
**	Normally the left and right children of a constant are NULL.
**	In special situations (an IN-list, for instance), multiple
**	constants can be linked through the left pointer.
**
** History:
**     14-apr-86 (jeff)
**          written
**	27-apr-88 (stec)
**	    Modified for DB procedures.
*/
typedef struct _PST_CNST_NODE
{
	/* Pst_tparmtype tells where to get the constant from. It will hold
	** the same values as PST_RSDM_NODE.pst_ttargtype will, except for
	** PST_ATTNO (attribute numbers are in var nodes). If pst_tparmtype
	** is PST_RQPARAMNO then pst_parm_no refers to a repeat query parameter
	** number. If it is PST_LOCALVARNO then pst_parm_no refers to a
	** procedure local variable number. Finally, if it is PST_USER then
	** the const was user (application) supplied and is in the
	** PST_SYMBOL.pst_dataval field.
	*/
    i4  	    pst_tparmtype;
    i4              pst_parm_no;        /* param number in saved query plan */
    PST_PMSPEC	    pst_pmspec;		/* pattern-matching specifier */
    DB_LANG	    pst_cqlang;		/* Query language id for this const */
    char	    *pst_origtxt;	/* If not NULL, a pointer to the original
					**  text for the constant.
					*/
}   PST_CNST_NODE;

/*}
** Name: PST_RL_NODE - Target node to be used in rule tree.
**
** Description:
**	This node is similar to the CURVAL and VAR nodes currently available.
**	Extra information is required that makes both the others unsuitable.
**	The only place that these nodes should be referenced is in the trees
**	applied to rules processing.  These trees are currently the condition
**	and parameter trees.
**
**	Pst_rltargtype will hold the same values as a RESDOM node targtype.
**	If the target type is a PST_ATTNO then pst_rltargno references an
**	attribute number in the table being modified.  If the target type
**	is PST_RSDNO then the attribute number references a RESDOM node
**      in the parent statement target list (may even be a non-printing
**      RESDOM) from which the rest of the data can be collected.
**
**	Currently, rules are only applied to update statements and, thus,
**	PST_ATTNO is the only kind of target type. When rules can be fired
**	off a SELECT statement or a WHERE qualification, then PST_RSDNO can
**	indicate which column in which referenced table is to be used.
**	
** History:
**	25-jan-89 (neil)
**	    Created for rules processing.
*/
typedef struct
{
    i4	    pst_rltype;		/*
				** Node type - before/after changes to row.
				** This node type indicates when the node image
				** data should be collected (it may be used
				** as a procedure parameter or in a condition).
				** The node value is always filled after the
				** statement is executed, though the data can
				** be the original data (before) or the new data
				** (after).
				*/
#define	       PST_BEFORE	    1
#define	       PST_AFTER	    2

    i4	    pst_rltargtype;	/* Same value as pst_ttargtype - see above  */
    i4	    pst_rltno;		/* # usage determined by  pst_rltargtype  */
} PST_RL_NODE;

/*}
** Name: PST_CASE_NODE - Node to be used in "case" function
**
** Description:
**	This node simply contains the resolved data type (as DB_DATA_VALUE) 
**	of the source/comparand expressions of a simple case function. This
**	field is required, since the pst_dataval of the CASE node is used for
**	the result type of the case function. 
**
**			CASE
**			/  \
**		   WHLIST  [expr]
**		   /   \
**	       WHLIST   WHOP
**	      ...       / \
**		   [pred]  valexpr
**
**	The RHS of the case node is used for "simple" case, i.e.
**	CASE expr WHEN val THEN val WHEN val THEN val ...
**	it holds the "expr" to test against.  Note that the expr is
**	duplicated under the WHOP's, the copy under the CASE node is
**	just for reference.  The "searched" CASE, CASE WHEN pred...,
**	has a null right-child link.
**
**	The LHS of the case node points to a list of WHLIST's, linked
**	down the left in the obvious manner(*).  The WHLIST's are just
**	list placeholders;  the real action is in the WHOP nodes.  The
**	left of the WHOP node is a predicate;  if it evaluates to TRUE,
**	the value of the case is the valexpr on the right of the WHOP.
**	A null WHOP-left is the ELSE case at the bottom.  If no explicit
**	ELSE is given, an "ELSE NULL" is compiled in.
**
**	(*) "Obvious" in that the first WHEN-clause is at the top, unlike
**	resdoms where the first/leftmost resdom is at the bottom (left).
**	
** History:
**	8-sep-99 (inkdo01)
**	    Written for the case function.
**	10-feb-00 (inkdo01)
**	    Unbundled the DB_DATA_VALUE structure to ditch the pointer.
*/
typedef struct _PST_CASE_NODE
{
    i4		pst_caselen;	/* resolved length */
    DB_DT_ID	pst_casedt;	/* resolved type */
    i2		pst_caseprec;	/* resolved precision */
    i4		pst_casetype;	/* Code to indicate simple or searched case 
				*/
#define		PST_SIMPLE_CASE	1
#define 	PST_SEARCHED_CASE 2
} PST_CASE_NODE;

/*}
** Name: PST_SEQOP_NODE - Node used in sequence nextval/currval operations.
**
** Description:
**	This node contains the sequence name and qualifier, as well as the
**	sequence data type (in pst_dataval).  This is usually (always?)
**	a terminal node with no children.
**	
** History:
**	11-mar-02 (inkdo01)
**	    Written for sequence support.
*/
typedef struct _PST_SEQOP_NODE
{
    DB_OWN_NAME	    pst_owner;		/* owner of sequence */
    DB_NAME	    pst_seqname;	/* sequence name */
    DB_TAB_ID	    pst_seqid;		/* internal sequence ID (filled in later) */
    i2		    pst_seqflag;	/* operator indicator */
#define	PST_SEQOP_NEXT	0x01		/* next value for (nextval) */
#define	PST_SEQOP_CURR	0x02		/* current value for (currval) */
} PST_SEQOP_NODE;

/*}
** Name: PST_GROUP_NODE - Node to be used in new grouping operations.
**
** Description:
**	This node demarks CUBE/ROLLUP groups.
**	
** History:
**	30-mar-06 (dougi)
**	    Written for "group by" enhancements.
*/
typedef struct _PST_GROUP_NODE
{
    i4		    pst_groupmask;	/* mask field for grouping */
#define	PST_GROUP_ROLLUP 0x01		/* this is a ROLLUP */
#define	PST_GROUP_CUBE	0x02		/* this is a CUBE */
} PST_GROUP_NODE;

/*}
** Name: PST_SYMVALUE - Union of all node types.
**
** Description:
**      This union defines the part of a query tree that varies depending
**      on the type of node.
**
** History:
**     26-sep-85 (jeff)
**          created
**	25-jan-89 (neil)
**	    Added PST_RL_NODE for rules processing.
**	30-mar-06 (dougi)
**	    Added pst_s_group for "group by" enhancements.
*/
typedef union
{
    PST_VAR_NODE    pst_s_var;
    PST_FWDVAR_NODE pst_s_fwdvar;
    PST_CRVAL_NODE  pst_s_curval;
    PST_RT_NODE     pst_s_root;
    PST_OP_NODE     pst_s_op;
    PST_RSDM_NODE   pst_s_rsdm;
    PST_SRT_NODE    pst_s_sort;
    PST_TAB_NODE    pst_s_tab; 
    PST_CNST_NODE   pst_s_cnst;
    PST_RL_NODE	    pst_s_rule;	/* Tagged by PST_RULEVAR in PST_SYMBOL */
    PST_CASE_NODE   pst_s_case;
    PST_SEQOP_NODE  pst_s_seqop;
    PST_GROUP_NODE  pst_s_group;
} PST_SYMVALUE;

/*}
** Name: PST_SYMBOL - Symbol structure: data part of query tree node.
**
** Description:
**      This structure defines the contents of a symbol.  A symbol is the
**      data part of a query tree node; that is, everything except the pointers
**      to the children.
**
**	For those brought up on ordinary compilers, the term "symbol" is
**	rather misleading, as there's no symbol table in the normal
**	sense.  A PST_SYMBOL is rarely if ever seen outside of a query
**	tree node, and probably could have been made part of the
**	PST_QNODE itself.
**
**	Some notes on query node types that haven't already been discussed:
**	- PST_QLEND: a dummy terminator node marking the end of a WHERE
**	  clause, or place-holding an empty WHERE clause.
**	- PST_TREE: not the root, as one might imagine, but another
**	  terminator marking the left-end of a resdom list.
**	- PST_OPERAND: a list placeholder node used under a MOP.  The
**	  operand list runs through the left links, with the right ones
**	  pointing to actual-operand expression subtrees.
**	- PST_WHLIST and PST_WHOP: WHEN-list and WHEN-op.  See CASE.
**
**	The TREE and QLEND terminators seem kind of pointless, and I
**	suspect that if we really tried, we could eliminate them entirely
**	in favor of NULL terminator links (kschendel).
**
** History:
**     26-sep-85 (jeff)
**          written
**	25-jan-89 (neil)
**	    added PST_RULEVAR to tag rule nodes in symbol.
**	2-jun-98 (inkdo01)
**	    added PST_MOP for multi-variate operations (> 2 operands).
**	2-sep-99 (inkdo01)
**	    Added PST_CASEOP, PST_WHLIST and PST_WHOP for case function.
**	11-mar-02 (inkdo01)
**	    Added PST_SEQOP for sequence operators (nextval, currval).
**	30-mar-06 (dougi)
**	    Added PST_GSET, PST_GBCR, PST_GCL for "group by" enhancements.
**	05-Feb-2009 (kiria01) b121607
**	    Define pst_type as enum with table macro to keep things
**	    in sync with little effort and allow debugger to symbolically
**	    display the parse tree node tags.
**	11-Mar-2009 (frima01) b121607
**	    Moved psq_type definition outside of PST_SYMBOL definition
**	    to avoid portability problems. 
*/
typedef    enum pst_type_enum {   /* code identifying node type */
/*  PST_xxxx   Short  N     Node class*/
#define PST_TYPES_MACRO \
_DEFINE(AND,    AND,  0 /* PST_OP_NODE */)\
_DEFINE(OR,     _OR,  1 /* PST_OP_NODE */)\
_DEFINE(UOP,    UOP,  2 /* PST_OP_NODE */)\
_DEFINE(BOP,    BOP,  3 /* PST_OP_NODE */)\
_DEFINE(AOP,    AOP,  4 /* PST_OP_NODE */)\
_DEFINE(COP,    COP,  5 /* PST_OP_NODE */)\
_DEFINE(CONST,  CST,  6 /* PST_CNST_NODE */)\
_DEFINE(RESDOM, RSD,  7 /* PST_RSDM_NODE */)\
_DEFINE(VAR,    VAR,  8 /* PST_VAR_NODE */)\
_DEFINE(ROOT,   RUT,  9 /* PST_RT_NODE */)\
_DEFINE(QLEND,  QLE, 10 /* nothing */)\
_DEFINE(TREE,   TRE, 11 /* nothing */)\
_DEFINE(BYHEAD, BYH, 12 /* PST_RSDM_NODE */)\
_DEFINE(AGHEAD, AGH, 13 /* PST_RT_NODE */)\
_DEFINE(SORT,   SRT, 14 /* PST_SRT_NODE */)\
_DEFINE(CURVAL, CVL, 15 /* PST_CRVAL_NODE */)\
_DEFINE(NOT,    NOT, 16 /* nothing */)\
_DEFINE(SUBSEL, SUB, 17 /* PST_RT_NODE */)\
_DEFINE(RULEVAR,RLV, 18 /* PST_RL_NODE */)\
_DEFINE(TAB,    TAB, 19 /* PST_TAB_NODE */)\
_DEFINE(MOP,    MOP, 20 /* PST_OP_NODE */)\
_DEFINE(OPERAND,OPN, 21 /* nothing */)\
_DEFINE(CASEOP, CAS, 22 /* PST_CASE_NODE */)\
_DEFINE(WHLIST, WHL, 23 /* nothing */)\
_DEFINE(WHOP,   WHP, 24 /* nothing */)\
_DEFINE(SEQOP,  SEQ, 25 /* PST_SEQOP_NODE */)\
_DEFINE(GSET,   GSE, 26 /* nothing */)\
_DEFINE(GBCR,   GBC, 27 /* PST_GROUP_NODE */)\
_DEFINE(GCL,    GCL, 28 /* nothing */)\
_DEFINE(FWDVAR, FWD, 29 /* PST_FWDVAR_NODE */)\
_DEFINEEND
#define _DEFINE(n,s,v) PST_##n=v,
#define _DEFINEEND
        PST_TYPES_MACRO PST_MAX_ENUM
#undef _DEFINE
#undef _DEFINEEND
    }               PST_TYPE;
typedef struct
{
    PST_TYPE	    pst_type;		/* code identifying node type */
    DB_DATA_VALUE   pst_dataval;	/* Type, len, precision, value
                                        ** - defined for PST_UOP, PST_BOP,
                                        ** PST_AOP, PST_COP, PST_CONST,
                                        ** PST_VAR, PST_RESDOM, PST_BYHEAD,
					** PST_RULEVAR.
					*/
    i4              pst_len;            /* length in bytes of pst_value field */
    PST_SYMVALUE    pst_value;          /* value(s) of this symbol */
} PST_SYMBOL;

/*}
** Name: PST_QNODE - Query tree node
**
** Description:
**      This structure defines a query tree node.  Each node has a left and
**	a right descendant.  If the node is a leaf the left and right pointers
**	will be NULL.  Depending on the "pst_type" field of the PST_SYMBOL
**	structure, there may be more additional information.
**
** History:
**     26-sep-85 (jeff)
**          written
*/
typedef struct _PST_QNODE
{
    struct _PST_QNODE    *pst_left;     /* pointer to left child */
    struct _PST_QNODE	 *pst_right;	/* pointer to right child */
    PST_SYMBOL      pst_sym;            /* symbol for this node */
} PST_QNODE;

/*}
** Name: PST_RNGENTRY - single element of a range table
**
** Description:
**      Describes a single element of the PST_RNGTAB structure.  Parser produces
**	a variable array of pointers to such entries for the rest of the system.
**	It is considered part of a query tree.
**
**	The pst_lvrm and pst_rvrm fields in the PST_RT_NODE structure tell which
**	slots used and which aren't.
[@comment_line@]...
**
** History:
**      15-sep-89 (seputis)
**          initial creation
**	27-sep-89 (andre)
**	    Added pst_outer_rel and pst_inner_rel to keep track of outer and
**	    inner relations of outer joins.
**	07-nov-91 (andre)
**	    merge 25-feb-91 (andre) change:
**		- instead of a fixed array of range table elements, PST_QTREE
**		  will contain a variable-size array of (PST_RNGENTRY *);
**		- a new field, pst_corr_name was added to PST_RNGENTRY to
**		  improve readability of QEPs
**	12-dec-91 (barbara)
**	    Merge of rplus and Star code for Sybil.  Added Star field
**	    pst_dd_objtype used to describe type of Star object.
**      05-jan-93 (rblumer)
**          Added new rgtype PST_SETINPUT for set-input proc parameters.
**	24-jan-06 (dougi)
**	    Added PST_DRTREE for derived tables (subselect in FROM clause).
**	28-dec-2006 (dougi)
**	    Added PST_WETREE for with clause elements. Note, their presence in 
**	    the range table doesn't necessarily mean that they are referenced 
**	    by the main query or its subselects.
**	18-march-2008 (dougi)
**	    Added PST_TPROC for table procedures.
**	21-nov-2008 (dougi) Bug 121265
**	    Added PST_TPROC_NOPARMS to solve RDF woes.
*/
typedef struct _PST_RNGENTRY
{
	/* Pst_rgtype tells whether this range variable represents a
	** phsyical relation or a relation that must be materialized from a
	** qtree. 
	*/
    i4			pst_rgtype;
#define	    PST_UNUSED	0
#define	    PST_TABLE	1
#define	    PST_RTREE	2
#define	    PST_SETINPUT 3	/* temporary table, used in set-input dbprocs;
				** tab_id will be filled in, but will only be
				** valid when RECREATEing the procedure
				*/
#define	    PST_DRTREE	4	/* derived table (subselect) in from clause 
				*/
#define	    PST_WETREE	5	/* with clause element "query name" */
#define	    PST_TPROC	6	/* table procedure */
#define	    PST_TPROC_NOPARMS 7	/* table proc with no parm list - used 
				** internally in RDF */

    PST_QNODE		*pst_rgroot;	/* root node of qtree */

	/* Pst_rgparent is a number of the range var that uses this range var
	** and caused it to be added. If this range var has no parent then
	** pst_rgparent will be set to -1. Parent range vars will always have
	** their pst_rgtype set to PST_QTREE.
	*/
    i4			pst_rgparent;
    DB_TAB_ID		pst_rngvar;     /* Rng tab = array of table ids. */
    DB_TAB_TIMESTAMP    pst_timestamp;  /* For checking whether up to date */
    PST_J_MASK		pst_outer_rel;  /*
					** if n-th bit is ON, this relation is
					** an outer relation in the join with ID
					** equal to n
					*/
    PST_J_MASK		pst_inner_rel;  /*
					** if n-th bit is ON, this relation is
					** an inner relation in the join with ID
					** equal to n
					*/
					/*
					** will contain correlation name if one
					** was supplied by the user; otherwise
					** will contain table name
					*/
    DB_TAB_NAME		pst_corr_name;	
    DD_OBJ_TYPE		pst_dd_obj_type;
} PST_RNGENTRY;

/*}
** Name: PSC_COLMAP - Bit map of columns.
**
** Description:
**      This structure defines a bit map of columns.  It is used to describe
**      binary properties about the columns; for instance, is a column
**      updateable or not?  There are enough bits here to describe any relation.
**
** History:
**     08-oct-85 (jeff)
**          written
*/
typedef struct _PSC_COLMAP
{
    i4              psc_colmap[PSC_MAPSIZE]; /* Enough bits for 128 cols */
} PSC_COLMAP;

/*}
** Name: PST_HINTENT - entry in table hint array.
**
** Description:
**      This structure defines an entry in the array of table level hints
**	used to describe index and join hints to the query optimizer.
**
** History:
**	17-mar-06 (dougi)
**	    Written for the optimizer hints feature.
*/
typedef struct _PST_HINTENT
{
    DB_TAB_NAME		pst_dbname1;	/* table name */
    DB_TAB_NAME		pst_dbname2;	/* index name (for index hint) or
					** table name (for set hint) */
    i4			pst_hintcode;	/* type of hint */

#define	PST_THINT_INDEX	0x0001		/* index (table, index) */
#define	PST_THINT_FSMJ	0x0002		/* fsmj (table1, table2) */
#define	PST_THINT_HASHJ	0x0004		/* hashj (table1, table2) */
#define	PST_THINT_KEYJ	0x0008		/* keyj (table1, table2) */
#define	PST_THINT_PSMJ	0x0010		/* psmj (table1, table2) */
} PST_HINTENT;

/*}
** Name: PST_RESKEY - Result table key info.
**
** Description:
**      This structure holds info about a key column of the result table.
**
** History:
**     01-oct-87 (stec)
**          written
*/
typedef struct _PST_RSKEY
{
    struct _PST_RSKEY  *pst_nxtkey;	/* pointer */
    DB_ATT_NAME		pst_attname;	/* attribute name */
} PST_RESKEY;

/*}
** Name: PST_RESTAB - Result table info.
**
** Description:
**      This structure groups information about result table.
**
** History:
**    01-oct-87 (stec)
**	written.
**    19-jan-88 (stec)
**	Pst_resloc field is changed to be of DM_DATA type.
**	Offset of the next field (pst_resjour) must not change, because
**	this struct is used in query tree headers stored in catalogs.
**    29-feb-88 (stec)
**	Change name of pst_fill1 to pst_flr1 (was causing Lint msg).
**    29-apr-1991 (Derek)
**	Added new field pst_alloc,pst_extend for allocation project.
**    07-nov-91 (andre)
**	merge 7-mar-1991 (bryanp) change:
**	    Added support for Btree index compression: result tables can now
**	    have two forms of compression: data compression and index (also
**	    called key, somewhat inappropriately) compression. Thus the
**	    pst_compress field in the PST_RESTAB structure is now a flag word
**	    rather than a simple boolean, and can have various bit values.
**	13-nov-91 (andre)
**	    changed pst_minpgs, pst_maxpgs, pst_alloc, pst_extend to i4 to
**	    ensure that it can handle the largest legal value -
**	    DB_MAX_PAGENO (2**23 - 1)
**	15-jan-1992 (bryanp)
**	    Temporary table enhancements: added pst_temporary field to the
**	    PST_RESTAB to handle CREATE TABLE AS SELECT ... for session tables.
**	    Added pst_recovery to the PST_RESTAB to handle the WITH NORECOVERY
**	    clause of the lightweight session table's CREATE TABLE statement.
**    18-jun-93 (robf)
**	 Secure 2.0:
**	- Add pst_flags to hold various flags as a bitmap
**	- Add pst_secaudkey to hold user-specified audit key columns,
**	  based on same idea as regular key clause
**	06-mar-1996 (stial01 for bryanp)
**	    Add pst_page_size to PST_RESTAB.
**	05-May-2006 (jenjo02)
**	    Add pst_clustered for Clustered Btree.
**	28-May-2009 (kschendel) b122118
**	    Make things that are boolean, actually bool.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Expand result-journaled to indicate NOW or AT NEXT CHECKPOINT.
*/
typedef struct _PST_RESTAB
{
    DB_TAB_ID	    pst_restabid;	/* result table id for this qry */
    DB_TAB_NAME	    pst_resname;	/* nm of res.tab.,if doesn't exist yet*/
    DB_OWN_NAME	    pst_resown;		/* owner of result table */
    DM_DATA	    pst_resloc;		/* location descriptor */
    char	    pst_flr1[sizeof(DB_LOC_NAME) - sizeof(DM_DATA)];/*filler*/
    i4		    pst_resvno;		/* number of result range variable */
    i4		    pst_fillfac;	/* fillfactor value */
    i4		    pst_leaff;		/* leaffill (indexfill) value */
    i4		    pst_nonleaff;	/* nonleaffill value */
    i4	    pst_page_size;	/* requested page size for table */
    i4	    pst_minpgs;		/* minpages value */
    i4	    pst_maxpgs;		/* maxpages values */
    i4	    pst_alloc;		/* allocation value. */
    i4	    pst_extend;		/* extend value. */
    i4	    pst_struct;		/* structure type */
    i2	    pst_resjour;	/* Result rel. journaled? */
#define	PST_RESJOUR_OFF		0	/* Not journaled */
#define	PST_RESJOUR_ON		1	/* Journaled now (db is journaled) */
#define	PST_RESJOUR_LATER	2	/* Journaled later; db not journaled */

    i2	    pst_compress;	/* compression options, as follows: */
					/* Data compression should be used. */
#define			PST_DATA_COMP	    0x0001L

					/* Data compression should NOT be used.
					** This bit indicates that COMPRESSION=
					** (NODATA) has been explicitly
					** specified, and is thus used
					** differently by the parser. Logically,
					** this bit and PST_DATA_COMP are
					** mutually exclusive, and are never
					** both set, and setting this bit is
					** equivalent to not setting the
					** PST_DATA_COMP bit.
					*/
#define			PST_NO_DATA_COMP    0x0002L

					/* Key compression should be used. */
#define			PST_INDEX_COMP	    0x0004L

					/* Key compression should NOT be used.
					** This bit indicates that COMPRESSION=
					** (NOKEY) has been explicitly
					** specified, and is thus used
					** differently by the parser. Logically,
					** this bit and PST_INDEX_COMP are
					** mutually exclusive, and are never
					** both set, and setting this bit is
					** equivalent to not setting the
					** PST_INDEX_COMP bit.
					*/
#define			PST_NO_INDEX_COMP   0x0008L

					/* Used only by the parser. This bit
					** indicates that the compression
					** semantics are currently deferred
					** until the storage structure is
					** known.
					*/
#define			PST_COMP_DEFERRED   0x0010L

					/* Used only by the parser. This bit
					** indicates that WITH NOCOMPRESSION
					** was specified. At the completion
					** of parsing the CREATE TABLE ...
					** AS SELECT statement, the parser
					** will clear this bit, and so OPF does
					** not ever see it.
					*/
#define			PST_NO_COMPRESSION  0x0020L

					/* HI Data compression should be used */
#define			PST_HI_DATA_COMP    0x0040L

    bool	    pst_resdup;		/* TRUE iff duplicates requested */
    bool	    pst_clustered;	/* TRUE if Clustered Btree */
    bool	    pst_temporary;	/* zero if table is permanent, non-zero
					** if table is temporary.
					*/
    char	    pst_autostruct;	/* flag for auto structure options */
#define			PST_AUTOSTRUCT	    0x01
					/* autostruct is on */
#define			PST_NO_AUTOSTRUCT   0x02
					/* autostruct is off */
    bool	    pst_recovery;	/* zero if temporary table was declared
					** WITH NORECOVERY, non-zero otherwise.
					** (always FALSE at present)
					*/
    i4	    pst_secaudit;	/* Security audit flags */
#define			PST_SECAUDIT_ROW	0x01
#define			PST_SECAUDIT_NOROW	0x02
    i4 	    pst_flags;		/* General flags */
					/* 
					** Table is system_maintained
					*/
# define		PST_SYS_MAINTAINED	0x01						/*
					** Table has row security auditing
					*/
# define		PST_ROW_SEC_AUDIT	0x04

    PST_RESKEY	    *pst_reskey;	/* NULL terminated linked list
					** of key entries */
    DB_ATT_NAME	    *pst_secaudkey;	/* Security audit key */
} PST_RESTAB;

/*
** Name:    PST_QRYHDR_INFO - this structure will be used to communicate all
**			      sorts of useful information inside PST_QTREE.
**
** Description: this structure will be used to communicate all sorts of useful
**		information inside PST_QTREE.  For now it will be used to pass
**		data about QUEL repeat queries which will be used to determine
**		if some other QUEL query can share the existing query plan
**
** History:
**
**	28-jan-91 (andre)
**	    defined
*/
typedef struct _PST_QRYHDR_INFO
{
    i4	    pst_1_usage;    /*
				    ** describe information contained in 
				    ** pst_1_info union
				    */
    /*
    ** masks defined over pst_1_usage
    */

				    /*
				    ** set if pst_1_info.pst_shr_rptqry points
				    ** at a structure containing information
				    ** about a shareable QUEL repeat query which
				    ** will enable us to determine if the same
				    ** query issued from the application on
				    ** behalf of another session can reuse the
				    ** existing query plan
				    */
#define	PST_SHR_RPTQRY	    0x0001L 


    /*
    ** all possible uses for pst_1_info
    */
#define PST_1INFO_USES	    (PST_SHR_RPTQRY)

/* macro to determine if pst_1_info is already being used */
#define PST_1_INFO_USED(header)	((header)->pst_info->pst_1_usage & PST_1INFO_USES)

    union
    {
	DB_SHR_RPTQRY_INFO	*pst_shr_rptqry;
    } pst_1_info;
} PST_QRYHDR_INFO;

/*}
** Name: PST_DSTATE - Will contain type of distributed query by site (and
**		      for single-site queries) the translated query text).
**
** Description:
**	This structure will contain indicator of the type of distributed query
**	(by site).  For single-site queries, text of translated queries will be
**	stored in a buffer list.
**
** History:
**	07-oct-88 (andre)
**	    written
**	08-nov-88 (andre)
**	    added pst_ddl_info
**	16-mar-89 (andre)
**	    Added pst_last_pkt.  This was done for dynamic OPEN CURSOR.
**	22-mar-89 (andre)
**	    Added #define for PST_TRUE_NSITE
**	12-dec-91 (barbara)
**	    Merged PST_DSTATE into rplus for Sybil.	
*/
typedef struct _PST_DSTATE {
    DD_LDB_DESC	    *pst_ldb_desc;
    DD_PACKET	    *pst_qtext;
    i4		    pst_stype;
#define	    PST_0SITE		0x00
#define	    PST_1SITE		0x01
#define	    PST_NSITE		0x02
#define	    PST_TRUE_NSITE	0x04
    QED_DDL_INFO    *pst_ddl_info;
    DD_PACKET	    *pst_last_pkt;
}   PST_DSTATE;

/*}
** Name: PST_QTREE - Query tree.
**
** Description:
**      This structure defines a query tree.  A query tree consists of a
**      range table and some query tree nodes linked together, where the
**      top node is a ROOT node.  In previous versions of INGRES, the result
**	table was given it's own range variable; now it is identified in this
**	structure either by a table id (if the table already exists, as for an
**	append) or the name and owner of the table (if the table doesn't yet
**	exist, as for a retrieve into).
**
**	RDF uses PST_VSN to determine how to unpack the tree.  Great care
**	must be taken to keep separate but current lines (6.3/04, STAR and
**	6.5 when this was written, but is bound to change over time) knowing
**	about all possible versions, even if this version is NOT used in
**	this line.
**      Having three separate lines lends itself to some problems. One of these
**      problems is tree version.  Tree version tells RDF how to unpack iitree 
**	tuples. This becomes an issue as soon as the format of iitree tuples is 
**	changed between different lines.  Example:  (Feb, 1991)
**	STAR LINE:
**	----------
**        PST_CUR_VSN = 3
**        A change is required to the RDF_QTREE structure to align pst_mask1 on
**        a 4-byte boundary. This is required for UNIX portability and also to
**        keep STAR compatible with mainline for the antisipated future merge
**        of local and star RDF.
**	6.3/04 Line:
**	------------
**        PST_CUR_VSN = 3
**        Two changes are required to the header because:
**                1) PST_RNGTAB is changed from an array of structs to a ptr to
**                   an array of structs.
**                2) a new field is added to the header.
**        These changes will eventually need to migrate to 6.5, and even more new
**        fields will need to be added to the header for 6.5 for ANDRE.
**	6.5 line (pre-SYBIL):
**	---------------------
**        PST_CUR_VSN = 4.  This is due to some Outer Join work that has already
**        been accomplished and was integrated a long time ago.
**	  Then PST_CUR_VSN = 7 cuz all 6.4 and version 5 (star) work was 
**	  integrated into 6.5 line.
**	
**	6.5 line (SYBIL):
**	-----------------
**	PST_CUR_VSN = 8.  This was due to merge of Star and RPLUS psfparse.h.
**	The effect on local was:
**	    add pst_dd_obj_name to PST_RNGENTRY 
**	    add pst_distr to PST_QTREE
**	The effect for STAR was:
**		PST_OP_NODE: pst_subquery renamed to pst_opparm;
**			     pst_joinid, pst_flags added;
**		PST_RNGTAB replaced with RNGENTRY; pst_corr_name added to struct
**		PST_RESTAB:  pst_alloc,pst_extend fields added;
**		PST_QTREE:   pst_rngvar_count, pst_rangetab, pst_numjoins and
**			      pst_basetab_id fields added;
**			    pst_qrange structure removed
**
**  SO, when you need to up PST_CUR_VSN, you must do the following:
**	1) update the PST_x_VSN list in ALL current lines (6.3, 6.5 and STAR for
**	   example for feb 1991.)  Be sure to manually update PST_MAX_VSN to the
**	   highest version in all current lines of psfparse.h.  [RDF will use
**	   that to distinguish between a tree with an illegal version and an
**	   improperly formatted tree.
**	2) update rdftree.h with the old structs that you are changing in
**	    psfparse.h.  This need to be propagated to ALL current lines.  Also
**	    update RDF_QTREE if you are changing PST_QTREE.
**	3) update rdfgetdesc.c of the given line to modify RDF to translate older 
**	    versions of trees to the current version.
**	4) update rdfupdate.c to correctly store the new tree.
**
** History:
**     26-sep-85 (jeff)
**          written
**     05-aug-86 (jeff)
**	    pst_delete added
**     11-aug-86 (jeff)
**	    added pst_unique
**     11-nov-86 (daved)
**	    added pointer to cursor control block so that cursor control
**	    blocks would be associated with its query tree.
**	18-mar-87 (stec)
**	    Added pst_resjour.
**	01-oct-87 (stec)
**	    Added structure for "create table as ..."; cleaned up.
**	21-oct-87 (stec)
**	    Added version no.
**	05-feb-88 (stec)
**	    Added PST_UNSPECIFIED.
**	17-feb-88 (stec)
**	    Added PST_1_VSN define.
**	22-feb-88 (stec)
**	    Changed PST_CUR_VSN to 2.
**	10-MAY-88 (stec)
**	    Moved PST_CUR_VSN to PST_PROCEDURE.
**	02-Feb-89 (andre)
**	    rename pst_res2 to pst_mask1 + define PST_TIDREQUERED
**	21-mar-89 (andre)
**	    defined PST_CDB_IIDD_TABLES - set if STAR SELECT/RETRIEVE queries
**	    reference objects with underlying tables' names starting with
**	    "iidd" and all located on CDB.
**	2-Oct-89 (teg)
**	    Added PST_3_VSN constant.
**	26-apr-90 (andre)
**	    define PST_NO_FLATTEN
**	04-jan-91 (andre)
**	    defined PST_CORR_AGGR - this flag will be turned on if a subselect
**	    has at least one aggregate in the target list and involves at least
**	    one correlated reference
**	04-feb-91 (andre)
**	    pst_res1 was renamed to pst_info and will be used to contain address
**	    of PST_QRYHDR_INFO structure, if one was allocated;
**	    defined 2 new masks over pst_mask1:
**	     - PST_SHAREABLE_QRY is set if the tree is for a shareable DML
**	       query;
**	     - PST_SQL_QUERY is set if the tree is for an SQL query
**	21-feb-91 (teresa)
**	    Updated pst_vsn to show all possible versions among current lines.
**	    also upped PST_CUR_VSN for star.
**	07-nov-91 (andre)
**	    merge 25-feb-91 (andre) change:
**		- replace PST_RNGTAB (a fixed-size array of structures
**		  representing range table entries) with a variable length array
**		  of pointers to PST_RNGENTRY structures and rename it from
**		  pst_qrange to pst_rangetab
**		- add a new field - pst_rngvar_count - which will contain the
**		  number of pointers in the array mentioned above;
**		- add a new field - pst_basetab_id - which will be used in 6.5
**		  to facilitate invalidation of QEPs which could be rendered
**		  obsolete by destruction of a view, permit, or database
**		  procedure, but is added here in order to reduce number of new
**		  tree version numbers
**	12-dec-91 (barbara)
**	    Merge of rplus and Star code for Sybil.  Added pointer to 
**	    PST_DSTATE (info on distributed parse) and Star PST_CATUPD_OK
**	    mask.  Also added Star comments:
**	    04-apr-89 (andre)
**		Define PST_CATUPD_OK.
**	27-jan-92 (teresa)
**	    SYBIL:  upped pst_cur_vsn to 8 because of SYBIL work, which added
**	    pst_dd_obj_name to PST_RNGENTRY and pst_distr to PST_QTREE.
**	12-mar-92 (andre)
**	    added pst_natjoins - a mask indicating which of the joins in the
**	    query tree were NATURAL
**	30-dec-92 (andre)
**	    defined PST_AGG_IN_OUTERMOST_SUBSEL
**	31-dec-92 (andre)
**	    defined PST_SINGLETON_SUBSEL
**	10-mar-93 (andre)
**	    defined PST_IMPLICIT_CURSOR_UPDATE
**	16-sep-93 (robf)
**          defined  PST_UPD_ROW_SECLABEL
**	02-nov-93 (andre)
**	    defined PST_SUBSEL_IN_OR_TREE, PST_ALL_IN_TREE, and 
**	    PST_MULT_CORR_ATTRS
**	6-dec-93 (robf)
**	    define PST_NEED_QRY_SYSCAT
**	9-jul-97(angusm)
**	    define PST_XTABLE_UPDATE
**	3-mar-00 (inkdo01)
**	    Replaced pst_natjoins (deprecated) with pst_firstn.
**	17-jul-00 (hayke02)
**	    Added PST_INNER_OJ.
**	6-dec-02 (inkdo01)
**	    Added PST_9_VSN to define range table expansion changes.
**	9-Feb-2004 (schka24)
**	    Added qeucb pointer for partitioned create table as select.
**	14-dec-04 (inkdo01)
**	    Added PST_10_VSN to include db_collID in DB_DATA_VALUE.
**	3-feb-06 (dougi)
**	    Change pst_rngtree integer flag to single bit flag and 
**	    use integer as source of bit flags to implement query-level
**	    hints.
**	17-mar-06 (dougi)
**	    Added pst_hintcount and pst_tabhint - ptr to array of table
**	    hints for optimizer hints project.
**	17-jan-2007 (dougi)
**	    PST_SCROLL flag for scrollable cursors.
**	5-feb-2007 (dougi)
**	    Add PST_KEYSET flag for keyset scrollable cursors.
**	18-july-2007 (dougi)
**	    Turn pst_rptqry to PST_RPTQRY flag and use integer for pst_offsetn.
**	10-dec-2007 (dougi)
**	    Added PST_PARM_FIRSTN/OFFSETN flags for parameterized first "n"
**	    and offset "n" values.
*/
typedef struct _PST_QTREE
{
    i2		    pst_mode;		/* Query mode */
    i2		    pst_vsn;		/* Header/tree vsn no.: 0, 1, ... */
#define	    PST_0_VSN	    0	    /* version 0 - illegal, no longer used */

#define	    PST_1_VSN	    1	    /* version 1 - illegal, no longer used */

				    /*
				    ** version 2 - for old style OP node, CNST
				    ** node and RSDM node.  Read into
                                    ** PST_2OP_NODE, PST_2CNST_NODE and
                                    ** PST_2RSDM_NODE, respectively.
				    */
#define	    PST_2_VSN	    2	    

				    /*
				    ** version 3, predates split of 6.5 and
				    ** star from 6.3. This is the last vsn
                                    ** common between those three lines.
				    */
#define	    PST_3_VSN	    3	    

				    /*
				    ** outer joins -- current version for
                                    ** 6.5 only.  Illegal for 6.3 and star
                                    ** FOR 6.5: OP node and rangetab changed,
				    ** read into PST_3OP_NODE and PST_3RNGTAB;
				    */
#define	    PST_4_VSN	    4	    

				    /*
				    ** STAR 6.4 version -- FIX STAR
				    ** RDF_QTREE alignment bug.  Illegal for
                                    ** 6.3 and pre-sybil 6.5. 
				    ** FOR STAR: RDF_QTREE changed:
				    ** read into RDF_V2_QTREE
				    */
#define	    PST_5_VSN	    5	    

				    /*
				    ** for 6.3/04.  Modify pst_hdr to make
				    ** range table a ptr to a array of struct
				    ** and to add some new fields.
				    */
#define	    PST_6_VSN	    6	    

				    /*
				    ** combines changes from version 3 to make
				    ** versions 4, 5, and 6 (Pre-SYBIL)
				    */
#define	    PST_7_VSN	    7	    

				    /* SYBIL:
				    ** Merged Local and STAR trees, which had
				    ** the effect of adding pst_dd_obj_name to
				    ** PST_RNGENTRY and pst_distr to PST_QTREE
				    ** for local.  For STAR, had the effect of
				    ** adding the following:
				    ** PST_OP_NODE: pst_subquery->pst_opparm;
				    **		    pst_joinid, pst_flags added;
				    ** PST_RNGTAB->RNGENTRY; pst_corr_name added
				    ** PST_RESTAB: pst_alloc,pst_extend added;
				    ** PST_QTREE: pst_rngvar_count, pst_rangetab
				    **		  pst_numjoins, pst_basetab_id
				    **		  added; pst_qrange removed
				    */
#define	    PST_8_VSN	    8

				    /* Ingres II, 2.65:
				    ** Range table expanded to 128 entries.
				    ** This increases the size of pst_xrvm
				    ** fields in root (subsel, aghead) nodes
				    ** and pst_inner/outer_rel in 
				    ** PST_RNGENTRY
				    */
#define	    PST_9_VSN	    9

				    /* r3.x
				    ** Added db_collID (2 bytes) to
				    ** DB_DATA_VALUE, thus screwing up
				    ** all parse tree nodes
				    */
#define	    PST_10_VSN	    10

				    /*
				    ** if you change this, see description
				    ** section for instructions
				    */
#define	    PST_CUR_VSN	    10

#define	    PST_MAX_VSN	    10

    i4		    pst_rngvar_count;	/* number of entries in the array of
					** pointers pointed to by pst_qrange;
					** 0 if no tables were used */
    PST_RNGENTRY    **pst_rangetab;	/* range table for this query tree;
					** NULL if no tables were used */
    PST_QNODE       *pst_qtree;         /* pointer to the actual tree */
    i4		    pst_agintree;	/* TRUE iff aghead exists in tree */
    i4		    pst_subintree;	/* TRUE iff subselect exists in tree */
    i4		    pst_hintmask;	/* contains hint flags for OPF */
#define	PST_HINT_FLATTEN	0x10
#define	PST_HINT_NOFLATTEN	0x20
#define	PST_HINT_OJFLATTEN	0x40
#define	PST_HINT_NOOJFLATTEN	0x80
#define	PST_HINT_GREEDY		0x0100
#define	PST_HINT_NOGREEDY	0x0200
#define	PST_HINT_HASH		0x0400 
#define	PST_HINT_NOHASH		0x0800
#define	PST_HINT_KEYJ		0x1000
#define	PST_HINT_NOKEYJ		0x2000
#define	PST_HINT_QEP		0x4000 
#define	PST_HINT_NOQEP		0x8000
#define	PST_HINT_PARALLEL	0x010000
#define	PST_HINT_NOPARALLEL	0x020000
#define	PST_HINT_ORDER		0x040000

    i4		    pst_wchk;		/* TRUE iff 'with check' view. */
    PST_QNODE	    *pst_offsetn;	/* value of result offset clause */
    i4		    pst_numparm;	/* number of user parms in query */
    i4              pst_stflag;         /* TRUE iff retrieve has sort clause */
    PST_QNODE       *pst_stlist;	/* sort list for retrieve */
    PST_RESTAB	    pst_restab;		/* structure holding all info about
					** result table */
    DB_CURSOR_ID    pst_cursid;		/* Cursor id for def. cursor stmt. 
					** query id for defined queries.  */
    i4		    pst_delete;		/* TRUE means delete permission given */
    i4		    pst_updtmode;	/* Type of updatability of cursor */
#define                 PST_DIRECT      1   /* Direct update */
#define			PST_DEFER	2   /* Deferred update */
#define			PST_READONLY	3   /* Not updateable */
#ifdef PST_UNSPECIFIED
#undef PST_UNSPECIFIED
#endif
#define			PST_UNSPECIFIED	4   /* Immaterial (not specified) */
    PSC_COLMAP	    pst_updmap;		/* bitmap of updated columns (columns in
					** 'for update' clause. Valid for
					** cursor commands. This map is
					** also filled in for all attributes
					** in a replace command. */
    PST_QRYHDR_INFO  *pst_info;		/*
					** ptr to a structure containing useful
					** information;
					*/
    PST_DSTATE	    *pst_distr;		/*
					** non NULL if a distributed parse
					** has been performed
					*/

    i4		    pst_mask1;		/* flag field */
    
#define	    PST_TIDREQUIRED	((i4) 0x01)	/* Tell OPF to make TID node */

#define	    PST_CDB_IIDD_TABLES	((i4) 0x02)	/* Query against CDB catalog */

#define     PST_NO_FLATTEN      ((i4) 0x04)	/* do not flatten this query */

						/*
						** query contained correlated
						** aggregates
						** will also be set in view 
						** trees if the tree involved 
						** (what OPF perceives as) 
						** correlated aggregates
						*/
#define	    PST_CORR_AGGR	((i4) 0x08)

#define	    PST_SHAREABLE_QRY	((i4) 0x10)	/* tree for a shareable query */

#define	    PST_SQL_QUERY	((i4) 0x20)	/* an SQL query */

#define	    PST_CATUPD_OK	((i4) 0x40)
						/*
						** will be set in view trees if
						** the outermost subselect of
						** an SQL view definition
						** involves an aggregate;
						** for QUEL views it will be set
						** if a view definition involves
						** an aggregate outside of 
						** where-clause
						*/
#define	    PST_AGG_IN_OUTERMOST_SUBSEL		((i4) 0x80)

						/*
						** will be set in view trees if
						** the outermost subselect of an
						** SQL view definition involves
						** GROUP BY
						*/
#define	    PST_GROUP_BY_IN_OUTERMOST_SUBSEL	((i4) 0x0100)

						/*
						** will be set in view trees if
						** an SQL view definition
						** involves singleton subselects
						*/
#define	    PST_SINGLETON_SUBSEL		((i4) 0x0200)

						/*
						** user has not specified ORDER
						** BY, FOR UPDATE, or FOR
						** READONLY in a <cursor
						** declaration>.  Since the
						** <query expression> is
						** updatable, user will be able
						** to delete/update through the
						** cursor.  Hoping that he never
						** does, we will will try to
						** acquire shared locks which
						** will be escalated should he
						** actually try to UPDATE or
						** DELETE
						*/
#define	    PST_IMPLICIT_CURSOR_UPDATE		((i4) 0x0400)

						/*
						** will be set in view trees if
						** query tree contains a SUBSEL
						** node which is a descendant 
						** of OR (any query involving 
						** such view will not be 
						** flattened)
						*/
#define	    PST_SUBSEL_IN_OR_TREE		((i4) 0x0800)

						/*
						** will be set in view trees if
						** query tree contains ALL 
						** (any query involving such 
						** view will not be flattened)
						*/
#define	    PST_ALL_IN_TREE			((i4) 0x1000)

						/*
						** will be set in view trees if
						** query tree contains a SUBSEL
						** node which is a child of NOT
						** EXISTS or a SUBSEL node 
						** representing a nested 
						** subselect with target list 
						** containing count() or 
						** count(*) and the tree 
						** representing the SUBSEL node
						** contains corelated references
						** to at least 2 different 
						** range variables
						** (any query involving such
						** view will not be flattened)
						*/
#define	    PST_MULT_CORR_ATTRS			((i4) 0x2000)

						/*
						** Query needs query_syscat
						** privilege (ne select_syscat)
						*/
#define	    PST_RNGTREE				((i4) 0x4000)
						/* range table holds a parse
						** tree (view or derived table) 
						** Used to be pst_rngtree i4
						*/
#define	    PST_RPTQRY				((i4) 0x8000)
						/* this is repeat query
						** (used to be pst_rptqry i4)
						*/
/* 0x010000 unused */
#define	    PST_NEED_QRY_SYSCAT			((i4)0x020000)
	/*
	** set for cross-table update so we can
	** check properly for ambiguous replace
	*/
#define		PST_XTABLE_UPDATE	        ((i4)0x040000)
	/*
	** distinguish cross-table update
	** where update only invovles constant
	** values applied to target
	*/
#define		PST_UPD_CONST		        ((i4)0x080000)
#define		PST_INNER_OJ			((i4)0x100000)
	/*
	** query contains an inner outer join
	** which has been converted to a
	** normal inner join by the fix for SIR
	** 94906
	*/
#define         PST_AGHEAD_NO_FLATTEN           ((i4)0x200000)
        /*
        ** do not flatten query(s) that contains: 
        ** a) more than one ifnull() w/ aggregate operand. 
        ** b) one ifnull() w/ an aggregate and another aggregate
        ** in the query. 
        */ 
#define		PST_SCROLL			((i4)0x400000)
	/*
	** cursor has been declared scrollable. OPF must generate
	** appropriate query plan.
	*/
#define		PST_KEYSET			((i4)0x800000)
	/*
	** cursor has been declared keyset scrollable. OPF must generate
	** appropriate query plan.
	*/
#define		PST_AGHEAD_MULTI		((i4)0x01000000)
	/*
	** query contains more than one aggregate operand.
	*/
#define		PST_IFNULL_AGHEAD		((i4)0x02000000)
	/*
	** query contains an ifnull() with an aggregate operand.
	*/
#define		PST_IFNULL_AGHEAD_MULTI		((i4)0x04000000)
	/*
	** query contains one or more ifnull() with one or more aggregate operands.
	*/
#define		PST_PARM_FIRSTN			((i4)0x08000000)
	/*
	** pst_firstn contains number of parm that will contain the
	** actual "n" value for repeat query/dbproc.
	*/
#define		PST_PARM_OFFSETN		((i4)0x10000000)
	/*
	** pst_offsetn contains number of parm that will contain the
	** actual "n" value for repeat query/dbproc.
	*/
    PST_J_ID	    pst_numjoins;	/* Number of joins in this tree */
    DB_TAB_ID	    pst_basetab_id;	/* this field will be used to facilitate
					** destruction of QEPs rendered
					** abandoned by destruction of views,
					** permits, or database procedures
					*/
    PST_QNODE	    *pst_firstn;		/*
					** count of rows to be returned from 
					** query (regardless of size of result
					** set). Used in "select first "n" ..."
					** syntax.
					*/
    struct _QEU_CB *pst_qeucb;		/* Pointer to QEU_CB for CREATE TABLE
					** AS SELECT only (and dgtt as select).
					** For now (Feb '04) the only use is
					** to get at the partition definition.
					** Ultimately it would be nice to
					** eliminate the pst_restab and pass
					** the result table info around like
					** modify does it.
					*/
    i4		    pst_hintcount;	/* count of entries in table hint
					** array */
    PST_HINTENT	    *pst_tabhint;	/* ptr to array of table hints */
} PST_QTREE;

/*
**  Definition of control block for qrymod operations.
*/

/* Max. chars one can store in a PSY_QTEXT block (2048 - overhead) */
#define                 PSY_QTSIZE	(2048 - sizeof(i4) - sizeof(PTR))

/*}
** Name: PSY_QTEXT - Query text decoded from iiqrytext relation
**
** Description:
**      The PSY_HVIEW, PSY_HPERM, and PSY_HINTG operations all read query
**	text out of the iiqrytext relation, reformat it, and put the results
**	in a linked list of these structures.  The linked list will be stored
**	in QSF, as query text.
**
** History:
**      15-jul-86 (jeff)
**          written
*/
typedef struct _PSY_QTEXT
{
    struct _PSY_QTEXT    *psy_qnext;    /* Pointer to next block */
    i4              psy_qcount;         /* Bytes used in this block */
    char	    psy_qtext[PSY_QTSIZE];  /* Query text to send to user */
}   PSY_QTEXT;

/**
** This is a copy of qu.h. Even though QUEUE typedef should have been
** used instead of PSF_QUEUE, we have decided not to do this, because
** every C module referencing psfparse.h would also have to include qu.h
** and that is unacceptable at this point in time (too much work).
**
** Description:
**      The file contains the type used by QU and the definition of the
**      QU functions.  These are used for manipulating queues.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**      30-apr-87 (stec)
**          Copied qu.h
*/
struct _PSF_QUEUE
{
	struct	_PSF_QUEUE		*q_next;
	struct	_PSF_QUEUE		*q_prev;
};

typedef struct _PSF_QUEUE PSF_QUEUE;

/*
**  Opcodes for use with PSY_CB.
**
**  History:
**	06-mar-89 (neil)
**          Rules: Added PSY_DRULE and PSY_KRULE.
**	22-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added psy opcode PSY_MXQUERY.
**	07-sep-89 (neil)
**	    Alerters: Added PSY_DEVENT & PSY_KEVENT.
**	09-oct-89 (ralph)
**	    Added new PSY opcodes for B1:
**		PSY_USER, PSY_LOC, PSY_AUDIT, PSY_ALARM.
**	20-feb-90 (andre)
**	    Added PSY_COMMENT
**	17-apr-90 (andre)
**	    define PSY_CSYNONYM, PSY_DSYNONYM
**	12-dec-91 (barbara)
**	    Merge of rplus and Star code for Sybil.  Added PSY_REREGISTER
**	    opcode for Star.
**	22-dec-92 (andre)
**	    undefined PSY_DVIEW since processing CREATE/DEFINE VIEW will no
**	    longer involve calling psy_dview()
**	19-feb-93 (andre)
**	    defined PSY_DSCHEMA for DROP SCHEMA processing
*/

#define                 PSY_DINTEG      1   /* Define an integrity */
#define			PSY_DPERMIT	2   /* CREATE/DEFINE PERMIT */
#define			PSY_HINTEG	4   /* Get help on integrities */
#define			PSY_HPERMIT	5   /* Get help on permits */
#define			PSY_HVIEW	6   /* Get help on a view */
#define			PSY_KINTEG	7   /* Destroy an integrity */
#define			PSY_KPERMIT	8   /* Destroy an integrity */
#define			PSY_KVIEW	9   /* Destroy a view */
#define			PSY_PRMDUMP	10  /* Dump a PSY_CB */
#define			PSY_CREDBP	11  /* Create a database procedure */
#define			PSY_DRODBP	12  /* Drop a database procedure */
#define			PSY_GROUP	13  /* CREATE/ALTER/DROP GROUP */
#define			PSY_APLID	14  /* CREATE/ALTER/DROP APLID */
#define			PSY_DBPRIV	15  /* GRANT/REVOKE ON DATABASE */
#define			PSY_DRULE	16  /* Define a rule */
#define			PSY_KRULE	17  /* Kill a rule */
#define			PSY_MXQUERY	18  /* SET [NO]MAX... */
#define			PSY_USER	19  /* CREATE/ALTER/DROP USER */
#define			PSY_LOC 	20  /* CREATE/ALTER/DROP LOCATION */
#define			PSY_AUDIT	21  /* ENABLE/DISABLE SECURITY_AUDIT */
#define			PSY_ALARM	22  /* CREATE/DROP SECURITY_ALARM */
#define			PSY_DEVENT	23  /* Define an event */
#define			PSY_KEVENT	24  /* Kill an event */
#define			PSY_COMMENT	25  /* Add comment on table/column */
#define			PSY_CSYNONYM	26  /* CREATE SYNONYM */
#define			PSY_DSYNONYM    27  /* DROP SYNONYM */
					    /*
					    ** GRANT ON
					    **   [TABLE]|PROCEDURE|DBEVENT
					    */
#define			PSY_GRANT	28
					    /*
					    ** REVOKE ON
					    **	 [TABLE]|PROCEDURE|DBEVENT
					    */
#define			PSY_REVOKE	29
#define			PSY_REREGISTER	30  /* REGISTER star_obj AS REFRESH */
#define			PSY_DSCHEMA	31  /* DROP SCHEMA */

#define			PSY_RPRIV	32  /* GRANT/REVOKE ROLE */
#define			PSY_ALTDB	33  /* ALTER DATABASE */
#define			PSY_CSEQUENCE	34  /* CREATE SEQUENCE */
#define			PSY_ASEQUENCE	35  /* ALTER SEQUENCE */
#define			PSY_DSEQUENCE	36  /* DROP SEQUENCE */


/*
** Maximum number of tables for PSY_CB.
*/

#define                 PSY_MAXTABS     20

/*}
** Name: PSY_CB - Control block for QRYMOD operations.
**
** Description:
**      This structure defines the control block for use with the Parser
**      Facility for QRYMOD-related operations.
**
** History:
**     30-sep-85 (jeff)
**          written
**     14-jul-86 (jeff)
**	    Added psy_istree and psy_tabname
**     15-jul-86 (jeff)
**	    Added psy_textout
**     16-jul-86 (jeff)
**	    Changed psy_tabname to an array of names
**     28-jul-86 (jeff)
**	    Changed psy_dmucb from a QSF object id to a PTR (easier to manage)
**     2-sep-86 (seputis)
**          Changed psy_intrans to psy_trans for lint
**     23-apr-87 (stec)
**          Added psy_usrq, psy_tblq, psy_colq.
**	28-jul-88 (stec)
**	    Add defines for psy_grant in PSY_CB.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Defined new qrymod opcodes,  PSY_GROUP and PSY_APLID;
**	    Added psy_grpid, psy_aplid, psy_apass, psy_gtype,
**		  psy_aplflag, psy_grpflag fields to PSY_CB;
**	    Defined grantee types, PSY_[UGA]GRANT
**	    Defined new group operation flags, PSY_[CADK]GROUP;
**	    Defined new applid operation flags, PSY_[CAK]APLID.
**	16-mar-89 (neil)
**          Rules: Added psy_rule field for rule processing.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Defined new qrymod opcode,  PSY_DBPRIV
**	    Added the following fields to the PSY_CB:
**		psy_fl_dbpriv, psy_qrow_limit, psy_qdio_limit,
**		psy_qcpu_limit, psy_qpage_limit, psy_qcost_limit;
**	    Defined new values for psy_grant: PSY_DGRANT, PSY_DREVOKE;
**	    Loaded psy_usrq to be a list of grantees;
**	    Loaded psy_tblq to be a list of objects, including databases.
**	24-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2a:
**	    Redefined PSY_DGRANT;
**	    Added psy_ctl_dbpriv.
**	22-may-89 (ralph)
**	    remove psy_aplid, psy_grpid from PSY_CB.
**	07-sep-89 (neil)
**	    Alerters: Move psy_rule into union, now shared with psy_event.
**	    Added psy_grant values (PSY_EVGRANT, PSY_EVREVOKE).
**	08-oct-89 (ralph)
**	    Added fields for B1:
**		psy_usflag, psy_loflag, psy_auflag, psy_alflag, psy_usprivs,
**		psy_lousage, psy_usgroup, psy_aulevel.
**	    Added several constants for above flag fields.
**	28-oct-89 (ralph)
**	    Added PSY_SDROP to psy_grant field to signifiy security_alarms
**	16-jan-90 (ralph)
**	    Added PSY_USRPASS flag to PSY_CB to indicate password specified
**	    on the CREATE/ALTER USER statement.
**	08-aug-90 (ralph)
**	    Added PSY_LGRANT, PSY_LDROP to psy_cb.psy_grant flags for
**	    granting and dropping permits on locations.
**	    Changed psy_cb.psy_opmap to u_i4.  Added psy_cb.psy_opctl.
**	    Added psy_cb.psy_bpass, etc., to support ALTER USER OLDPASSWORD.
**	14-jun-91 (andre)
**	    psy_off1 and psy_off2 will be moved from PSY_TBL into PSY_CB since
**	    the offset is the same for all objects mentioned in the grant
**	    statement.
**	17-jun-91 (andre)
**	    Add psy_u1off and psy_u2off.  psy_u1off will to contain the offset
**	    of the user name (in CREATE SECURITY ALARM template) or grantee
**	    name (in "non-column-specific" GRANT template).
**	    psy_u2off will contain the offset of the grantee name in
**	    "UPDATE(col_list)" GRANT template.
**	25-jun-91 (andre)
**	    Add psy_priv_off.  This field will contain the offset of privilege
**	    name from the end of the text of DEFINE PERMIT statement.  I chose
**	    to save the offset from the end of the statement not because of my
**	    excentricity, but because psq_tout() adds one or more RANGE
**	    statements in front of DEFINE PERMIT statement thus changing the
**	    offset of the privilege name from the beginning of the text stored
**	    in QSF.
**	15-jan-1992 (bryanp)
**	    Temporary table enhancements:
**	    Added PSY_IS_TMPTBL flagmask to psy_obj_mask definitions.
**	14-feb-92 (andre)
**	    got rid off psy_priv_off.  I was never very happy with the idea and
**	    have finally decided to store privilege name for DEFINE PERMIT that
**	    specifies exactly one privilege, so psy_priv_off is no longer
**	    necessary.
**	14-jul-92 (andre)
**	    added psy_flags and defined PSY_ALL_PRIVS over it
**	19-jul-92 (andre)
**	    defined PSY_PUBLIC
**	20-jul-92 (andre)
**	    defined PSY_EXCLUDE_COLUMNS over PSY_CB.psy_flags to indicate that
**	    user supplied a list of columns to be excluded from a table-wide
**	    privilege; this flag will be used when processing CREATE/DEFINE
**	    PERMIT
**
**	    defined PSY_EXCLUDE_UPDATE_COLUMNS, PSY_EXCLUDE_INSERT_COLUMNS, and
**	    PSY_EXCLUDE_REFERENCES_COLUMNS; this flags will be used to indicate
**	    whether a column map corresponding to a given privilege contains a
**	    list of columns to which the privilege applies or a list of columns
**	    to which the privilege DOES NOT apply; these flags will be used in
**	    processing of GRANT/REVOKE statements
**	21-jul-92 (andre)
**	    Added psy_u_numcols, psy_u_col_offset, psy_i_numcols,
**	    psy_i_col_offset, psy_r_numcols, and psy_r_col_offset.
**	    psy_%_numcols will indicate number of columns in the list associated
**	    with uPDATE, iNSERT, or rEFERENCES.
**	    psy_%_col_offset will indicate for each entry in psy_tblq where the
**	    list of attribute ids associated with uPDATE, iNSERT, or rEFERENCES
**	    starts (PSY_TBL contains a variable-size array of column ids which,
**	    once we add support for column-specific INSERT and REFERENCES may
**	    have to accomodate up to 3 different column lists.)
**	02-aug-92 (andre)
**	    defined PSY_EVDROP (to take place of PSY_EVREVOKE to indicate that
**	    we are processing DROP PERMIT ON DBEVENT); changed PSY_EVREVOKE to
**	    mean that we are processing REVOKE ON DBEVENT; defined PSY_TREVOKE
**	    and PSY_PREVOKE for REVOKE ON [TABLE] and REVOKE ON PROCEDURE
**
**	    defined PSY_DROP_CASCADE over psy_flags
**	09-nov-92 (ed)
**	    changed password members to DB_PASSWORD for DB_MAXNAME project
**	06-dec-92 (andre)
**	    added psy_u_colq and psy_r_colq for columns on which UPDATE and 
**	    REFERENCES, respectively, are being granted/revoked
**
**	    added psy_2qlen to contain length of the template (if any) built 
** 	    for GRANT UPDATE [EXCLUDING] (<column list>)
**
**	    added psy_off3 and psy_u3off to contain offsets to the beginning of
** 	    the object name and grantee name in the third template.
**	11-dec-92 (andre)
**	    to make code more readable renamed fields used when processing GRANT
**	    statement as follows:
**
**		psy_qlen	-->	psy_noncol_qlen
**		psy_2qlen	-->	psy_updt_qlen
**		psy_off1	-->	psy_noncol_obj_name_off
**		psy_off2	-->	psy_updt_obj_name_off
**		psy_off3	-->	psy_ref_obj_name_off
**		psy_u1off	-->	psy_noncol_grantee_off
**		psy_u2off	-->	psy_updt_grantee_off
**		psy_u3off       -->     psy_ref_grantee_off
**
**	    added psy_ref_qlen
**	30-dec-92 (tam)
**	    defined PSY_REMOVE_PROC over psy_flags
**	03-apr-93 (anitap)
**	    defined PSY_SCHEMA_MODE over psy_flags. This is needed when a user
**	    having insufficient privileges issues GRANT on more than one table
**	    within CREATE SCHEMA.
**	25-jun-93 (robf)
**	  Secure 2.0: 
**	  - Added  psy_usdefprivs, psy_seclabel, psy_date plus new
**	    status flags PSY_USRDEFPRIV, PSYUSREXPDATE, PSYUSRLIMLABEL.
**	    psy_usflag is extended from i4  to i4.
**	    Added psy_aufile for audit file, plus new PSY_AU options.
**	    Added psy_usprofile for profile name
**          Added psy_idle_limit/connect_limit/priority_limit
**	    Added psy_alarm for alarm tuple storage.
**	    Added psy_objq for general object storage queue.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	28-apr-94 (andre)
**	    (part of fix for bug 62890)
**	    defined PSY_SYSTEM_GENERATED over psy_flags
**	11-May-2001 (jenjo02)
**	    Changed psy_loc_rawblocks to psy_loc_rawpct.
**	7-mar-02 (inkdo01)
**	    Changes for cr/alt/drop sequence
**      28-July-2003 (gupsh01)
**          Added PSY_UNOPRIVS and PSY_UNODEFPRIV to indicate that user
**          has supplied, noprivileges and nodefault_privileges options.
**	17-nov-2008 (dougi)
**	    Added psy_idseq_defp[] for identity columns.
**	27-Jun-2010 (kschendel) b123986
**	    There isn't any psy-cb for create table, move psy-seqflag
**	    to a temporary flag in the iisequence tuple.  (The alternative
**	    of moving it to the YYVARS area was going to be a lot more work!)
**	    The only flag we needed in psy_seqflag was "decimal seen" anyway.
*/
typedef struct _PSY_CB
{
    struct _PSY_CB  *psy_next;		/* forward pointer */
    struct _PSY_CB  *psy_prev;		/* backward pointer */
    SIZE_TYPE	    psy_length;		/* length of this data structure */
    i2		    psy_type;		/* type code for this structure */
#define                 PSYCB_CB        0   /* 0 until real code is assigned */
    i2		    psy_s_reserved;	/* possibly used by mem mgmt */
    PTR		    psy_l_reserved;
    PTR		    psy_owner;		/* owner for internal mem mgmt */
    i4		    psy_ascii_id;	/* so as to see in dump */
#define                 PSYCB_ASCII_ID  CV_C_CONST_MACRO('P', 'S', 'Y', 'B')
    PTR		    psy_dbid;		/* Database id */
    i4              psy_timbgn;         /* Beginning time of day;min from 0:0 */
    i4              psy_timend;         /* Ending time of day; min from 0:0 */
    i4              psy_daybgn;         /* Beginning day of week; 0 = Sun. */
    i4              psy_dayend;         /* Ending day of week; 0 = Sun. */
    u_i4	    psy_opctl;          /* Defined map of permitted ops */
    u_i4	    psy_opmap;          /* Valued  map of permitted ops */
#define		    PSY_RETOK   0x0001L	/* Retrieve permitted */
#define             PSY_REPOK   0x0002L	/* Replace permitted */
#define             PSY_DELOK   0x0004L	/* Delete permitted */
#define             PSY_APPOK   0x0008L	/* Append permitted */
    i4              psy_numbs[PSY_MAXTABS]; /* List of permit/integrity	nums.
					** On DROP statement will contain
					** QEF opcodes to distinguish REMOVE
					** from DROP, etc. (distrib only
					*/
    i4		    psy_numnms;		/* Number of numbers listed above */
    DB_TAB_ID       psy_tables[PSY_MAXTABS]; /* List of table ids */
    i4		    psy_numtabs;	/* Number of tables listed above */
    i4		    psy_tbno;		/* Current table number */
    DB_ATT_ID	    psy_cols[DB_MAX_COLS]; /* Cols on which permission given */
    i4		    psy_numcols;	/* Num. of cols given above (0 = all) */
    QSO_OBID	    psy_qrytext;        /* Query id of qrymod definition text */
    QSO_OBID	    psy_intree;         /* Input query tree */
    QSO_OBID	    psy_outtree;        /* Output query tree */
    PTR		    psy_dmucb;		/* DMU control block pointer */
    DB_OWN_NAME     psy_user;           /* Name of user to get permission */
    DB_PASSWORD     psy_apass;          /* User/role password */
    DB_PASSWORD     psy_bpass;          /* Old user password */
    DB_DATE	    psy_date;		/* General date */

    DB_OWN_NAME     psy_usgroup;        /* Default group in CREATE/ALTER USER */
    DB_OWN_NAME     psy_usprofile;	/* User profile name */
    DB_TERM_NAME    psy_terminal;       /* Terminal at which permission OK */
    DB_AREANAME	    psy_area;		/* Area name for CREATE LOCATION */
    DB_ERROR	    psy_error;		/* Info on errors */
    i4		    psy_istree;		/* TRUE iff there is an input tree */
    DB_TAB_NAME	    psy_tabname[PSY_MAXTABS]; /* Names of table to work on */
    PST_QNODE	    *psy_idseq_defp[PSY_MAXTABS]; /* IIDEFAULT ids for identity
					** sequences during DROP */
    QSO_OBID	    psy_textout;	/* QSF id for formatted query text */
    i4		    psy_tabprinted;	/* TRUE means tabname has been printed*/
    PTR		    psy_tupptr;		/*
					** Current integ or permit tuple
					** When dealing with COMMENTs and
					** SYNONYMs, will point at formatted
					** II_1_DBCOMMENT and II_DBSYNONYM
					** tuple, respectively
					*/
    PTR		    psy_rdfinfo;	/* RDF info block pointer */
    i4		    psy_nomore;		/* TRUE means no more query text, or
					** none wanted.
					*/
    i4		    psy_trans;		/* TRUE means we are in a transaction */
    PTR		    psy_recid;		/* Record access id */
    i4		    psy_intno;		/* current integrity number */
    PSF_QUEUE	    psy_usrq;		/* Pointer to 1st grantee in chain */
    PSF_QUEUE	    psy_tblq;		/* Pointer to 1st object in chain */
    PSF_QUEUE	    psy_objq;		/* Pointer to general object chain */
    PSF_QUEUE	    psy_colq;		/*
					** Pointer to 1st column in list of
					** columns to which a given statement 
					** applies - is NO LONGER used for 
					** GRANT/REVOKE processing
					*/
    PSF_QUEUE	    psy_u_colq;		/*
					** Pointer to 1st column in list of
					** columns associated with UPDATE
					** privilege being granted or revoked
					*/
    PSF_QUEUE	    psy_r_colq;		/*
					** Pointer to 1st column in list of
					** columns associated with REFERENCES
					** privilege being granted or revoked
					*/
    i4		    psy_u_numcols;
    i4		    psy_u_col_offset;
    i4              psy_i_numcols;
    i4              psy_i_col_offset;
    i4              psy_r_numcols;
    i4              psy_r_col_offset;
    i4		    psy_noncol_qlen;	/* Length of columnless iiqrytext */
    i4		    psy_updt_qlen;	/* length of UPDATE(cols) query text */
    i4              psy_ref_qlen;       /*
					** length of REFERENCES(cols) query text
					*/
    i4		    psy_loc_rawpct;	/* Raw percent in CREATE LOCATION */
    i4		    psy_grant;		/* A flag used in conjunction with
					** permission stmts.
					*/
#define			PSY_CPERM	0x00	/* Create permit stmt */
#define			PSY_TGRANT	0x01	/* Table version of GRANT */
#define			PSY_PGRANT	0x02	/* Procedure version of GRANT */
#define			PSY_TDROP	0x03	/* Table version of DROP */
#define			PSY_PDROP	0x04	/* Procedure version of DROP */
#define			PSY_DGRANT	0x05	/* Database version of GRANT */
#define			PSY_DREVOKE	0x06	/* Database version of REVOKE */
#define			PSY_EVGRANT	0x07	/* GRANT privilege on EVENT */
#define			PSY_EVDROP 	0x08	/* Dbevent version of DROP */
#define			PSY_SDROP	0x09	/* Security_alarm vers of DROP*/
#define                 PSY_LGRANT      0x0A    /* Location version of GRANT */
#define                 PSY_LDROP       0x0B    /* Location version of DROP */
#define			PSY_TREVOKE	0x0C	/* table version of REVOKE */
#define			PSY_PREVOKE	0x0D	/* dbproc version of REVOKE */
#define			PSY_EVREVOKE	0x0E	/* dbevent version of REVOKE */
#define		        PSY_RGRANT      0x0F    /* GRANT ROLE */
#define 	        PSY_RREVOKE     0x10    /* REVOKE ROLE */
#define			PSY_SQGRANT	0x11	/* GRANT privilege on SEQUENCE */
#define			PSY_SQDROP	0x12	/* Sequence version of DROP */
#define			PSY_SQREVOKE	0x13	/* sequence version of REVOKE */
    i4		    psy_gtype;		/* Grantee type:
					**  See DB_PROTECTION for values
					*/
    i4		    psy_aplflag;	/* A flag used in conjunction with
   					** CREATE/ALTER/DROP APPLICATION_ID
   					*/
#define			PSY_CAPLID	1	/* CREATE APPLICATION_ID */
#define			PSY_AAPLID	2	/* ALTER  APPLICATION_ID */
#define			PSY_KAPLID	3	/* DROP   APPLICATION_ID */
    i4		    psy_grpflag;	/* A flag used in conjunction with
   					** CREATE/ALTER/DROP GROUP
   					*/
#define			PSY_CGROUP	1	/* CREATE GROUP */
#define			PSY_AGROUP	2	/* ALTER  GROUP...ADD */
#define			PSY_DGROUP	3	/* ALTER  GROUP...DROP */
#define			PSY_KGROUP	4	/* DROP   GROUP */
    i4		    psy_usflag;		
					/* A flag used in conjuunction with
					** CREATE/ALTER/DROP USER/PROFILE/ROLE
					*/
#define			PSY_CUSER	0x01	/* CREATE USER */
#define			PSY_AUSER	0x02	/* ALTER  USER */
#define			PSY_KUSER	0x04	/* DROP   USER */
#define			PSY_UNOPRIVS	0x08	/* 'noprivileges' given */
#define			PSY_USRDEFGRP	0x010	/* Default group specified */
#define			PSY_USRPRIVS	0x020	/* Privileges    specified */
#define			PSY_USRPASS	0x040	/* Password	 specified */
#define			PSY_USROLDPASS	0x080	/* Oldpassword	 specified */
#define			PSY_UNODEFPRIV	0x100	/* 'nodefault_privileges'given*/
#define			PSY_HEXPASS	0x200	/* HEX password  specified */
#define			PSY_USRNAME	0x1000	/* User name	 specified */
#define		        PSY_USRAPRIVS   0x2000  /* ADD PRIVILEGES. specified */
#define		        PSY_USRDPRIVS   0x4000  /* DROP PRIVILEGES. specified */
#define			PSY_USROPASSCTL 0x8000  /* Oldpassword being processed*/
#define			PSY_USRDEFPRIV  0x10000 /* Default privs specified */
#define			PSY_USREXPDATE  0x20000 /* Expiration date specified */
#define			PSY_USRSECAUDIT 0x80000 /* Security_audit specified */
#define			PSY_USRPROFILE  0x100000/* User profile specified */
#define			PSY_CPROFILE	0x200000/* CREATE PROFILE */
#define			PSY_APROFILE	0x400000/* ALTER PROFILE */
#define			PSY_DPROFILE	0x800000/* DROP PROFILE */
#define			PSY_USRCASCADE  0x1000000  /* CASCADE specified */
#define			PSY_USRRESTRICT 0x2000000  /* RESTRICT specified */
#define			PSY_USRDEFALL   0x4000000  /* DEFAULT_PRIV=ALL */
#define			PSY_USREXTPASS  0x8000000  /* EXTERNAL_PASSWORD */
    i4		    psy_loflag;		/* A flag used in conjuunction with
					** CREATE/ALTER/DROP LOCATION
					*/
#define			PSY_CLOC	0x01	/* CREATE LOCATION */
#define			PSY_ALOC	0x02	/* ALTER  LOCATION */
#define			PSY_KLOC	0x04	/* DROP   LOCATION */
#define			PSY_LOCAREA	0x0100	/* Area name was specified */
#define			PSY_LOCUSAGE	0x0200	/* Usage     was specified */
#define			PSY_LOCUSED	0x0400	/* ALL or NONE   specified */
    i4		    psy_alflag;		/* A flag used in conjuunction with
					** CREATE/DROP SECURITY_ALARM
					*/
#define			PSY_CALARM	0x01	/* CREATE SECURITY_ALARM */
#define			PSY_KALARM	0x04	/* DROP   SECURITY_ALARM */
#define			PSY_ALMSUCCESS	0x0100	/* IF SUCCESS specified	 */
#define			PSY_ALMFAILURE	0x0200	/* IF FAILURE specified	 */
#define			PSY_ALMCONNECT  0x0400  /* WHEN CONNECT */
#define			PSY_ALMDISCONNECT  0x0400  /* WHEN DISCONNECT */
#define			PSY_ALMEVENT  	0x0800   /* WITH DBEVENT */
#define			PSY_ALMEVTEXT  	0x01000  /* WITH DBEVENT TEXT */
    i4		    psy_auflag;		/* A flag used in conjuunction with
					** ENABLE/DISABLE SECURITY_AUDIT
					*/
#define			PSY_ENAUDIT	1	/* ENABLE  SECURITY_AUDIT */
#define			PSY_DISAUDIT	2	/* DISABLE SECURITY_AUDIT */
#define			PSY_ALTAUDIT	0x04	/* ALTER SECURITY_AUDIT */
#define			PSY_AUSTOP	0x08	/* ALTER ... STOP */
#define			PSY_AURESTART	0x010	/* ALTER ... RESTART */
#define			PSY_AUSETLOG	0x020	/* ALTER WITH LOG= */
#define			PSY_AUSUSPEND	0x040	/* ALTER ... SUSPEND */
#define			PSY_AURESUME	0x080	/* ALTER ... RESUME */
    char	    *psy_aufile;	/* New audit file name */

    i4	    psy_autype;		/*
					** Audit type specified on the
					** ENABLE/DISABLE SECURITY_AUDIT
					** statements.
					** See the du_id field of the
					** DU_SECSTATE definition
					** for possible values.
					*/
    u_i4	    psy_lousage;	/*
					** Location usage specified on the
					** CREATE/ALTER LOCATION statements.
					** See the DU_LOCATIONS definition
					** for possible values.
					*/
    u_i4	    psy_usprivs;	/*
					** Privileges specified on the
					** CREATE/ALTER USER statements.
					** See the DU_USER definition
					** for possible values.
					*/
    u_i4	    psy_usdefprivs;	/*
					** Default privileges specified on the
					** CREATE/ALTER USER statements.
					*/
    u_i4	    psy_ussecaudit;	/* Security audit options specified on 
					** the CREATE/ALTER USER statements
					*/
# define	PSY_USAU_QRYTEXT	0x01	/* QUERY TEXT auditing */
# define	PSY_USAU_ALL_EVENTS	0x04	/* All events auditing */
# define	PSY_USAU_DEF_EVENTS	0x08	/* Default events auditing */
    u_i4	    psy_ctl_dbpriv;	/* A flag used in conjunction with
					** GRANT/REVOKE ON DATABASE.
					** This flag identified privs being
					** granted or revoked.
					** See the DB_PRIVILEGES definition
					** for possible values
					*/
    u_i4	    psy_fl_dbpriv;	/* A flag used in conjunction with
					** GRANT/REVOKE ON DATABASE.
					** This flag specifies the value
					** for granted binary privs.
					** See the DB_PRIVILEGES definition
					** for possible values
					*/
    i4	    psy_qrow_limit;	/* Maximum estimated query row  limit */
    i4	    psy_qdio_limit;	/* Maximum estimated query I/O  limit */
    i4	    psy_qcpu_limit;	/* Maximum estimated query cpu  limit */
    i4	    psy_qpage_limit;	/* Maximum estimated query page limit */
    i4	    psy_qcost_limit;	/* Maximum estimated query cost limit */
    i4	    psy_idle_limit;	/* Maximum idle time limit */
    i4	    psy_connect_limit;	/* Maximum connect time limit */
    i4	    psy_priority_limit;	/* Maximum priority limit */
    union
    {
	DB_IIRULE	psy_rule;	/* CREATE/DROP RULE tuple storage */
	DB_IIEVENT	psy_event;	/* CREATE/DROP EVENT tuple storage */
	DB_IISCHEMA	psy_schema;
	DB_SECALARM     psy_alarm;	/* CREATE/DROP SECURITY_ALARM storage*/
	DB_IISEQUENCE	psy_sequence;	/* CREATE/ALTER/DROP SEQUENCE tuple */
    } psy_tuple;
    i4		    psy_obj_mask[PSY_MAXTABS];	/*
						** will be used to communicate
						** info about objects listed in
						** PSY_CB list
						*/

					
#define	    PSY_IS_VIEW		((i4) 0x01)	/* object is a view */
#define	    PSY_IS_TMPTBL	((i4) 0x02)	/* dropping a temp table */

    i4	    psy_noncol_obj_name_off;	/*
					** offset of an object name in a
					** "columnless" GRANT statement template
					*/
    i4	    psy_updt_obj_name_off;	/*
					** offset of an object name in a
					** template for a GRANT UPDATE statement
					** involving columns
					*/
    i4	    psy_ref_obj_name_off;	/*
					** offset of an object name in a
					** template for a GRANT REFERENCES 
					** statement involving columns
					*/
    i4	    psy_noncol_grantee_off;	/*
					** offset to the beginning of a user
					** name "non-column-specific" template
					*/
    i4	    psy_updt_grantee_off;	/*
					** offset to the beginning of a user
					** name in "GRANT UPDATE(col_list)"
					** template
					*/
    i4	    psy_ref_grantee_off;	/*
					** offset to the beginning of a user
					** name in "GRANT REFERENCES(col_list)"
					** template
					*/
    i4			psy_flags;	/* useful info */

					/*
					** user specified ALL [PRIVILEGES] in a
					** GRANT or REVOKE statement
					*/
#define	    PSY_ALL_PRIVS	((i4) 0x01)

					/*
					** user authorization identifier list
					** included PUBLIC
					*/
#define	    PSY_PUBLIC		((i4) 0x02)

					/*
					** user specified columns to be excluded
					** from table-wide privilege(s)
					*/
#define	    PSY_EXCLUDE_COLUMNS	((i4) 0x04)

					/*
					** column list corresponding to UPDATE
					** privilege contains names of columns
					** to which the UPDATE privilege will
					** NOT apply
					*/
#define	    PSY_EXCLUDE_UPDATE_COLUMNS	    ((i4) 0x08)

					/*
					** column list corresponding to INSERT
					** privilege contains names of columns
					** to which the INSERT privilege will
					** NOT apply
					*/
#define	    PSY_EXCLUDE_INSERT_COLUMNS	    ((i4) 0x10)

					/*
					** column list corresponding to
					** REFERENCES privilege contains names
					** of columns to which the REFERENCES
					** privilege will NOT apply
					*/
#define	    PSY_EXCLUDE_REFERENCES_COLUMNS  ((i4) 0x20)

					/*
					** cascading destruction was specified
					*/
#define	    PSY_DROP_CASCADE		    ((i4) 0x40)

					/*
					** remove regproc in star; tell
					** psy_kview to destroy QP
					*/
#define	    PSY_REMOVE_PROC		    ((i4) 0x80)

					 /*
					 ** GRANT statement has been specified 
					 ** within CREATE SCHEMA statement.
					 */
#define	    PSY_SCHEMA_MODE		    ((i4) 0x100)	 

					 /*
					 ** statement represented by this 
					 ** control block was generated 
					 ** internally
					 */
#define	    PSY_SYSTEM_GENERATED	    ((i4) 0x200)
					/*
					** Object should be delimited
					*/
#define	    PSY_DELIM_OBJ		   ((i4)0x1000)
					/*
					** Got grant/revoke dbrivs names
					*/
#define	    PSY_GR_REV_DBPRIV		((i4)0x4000)
					/*
					** Got grant/revoke role names
					*/
#define	    PSY_GR_REV_ROLE		((i4)0x8000)


} PSY_CB;

/*}
** Name: PSQ_QDESC - qry input 
**
** Description:
**      This structure contains the query text and data appropriate for
**  the query statement.
**
** History:
**      10-nov-86 (daved)
**          written
[@history_line@]...
[@history_template@]...
*/
typedef struct _PSQ_QDESC
{
    i4              psq_qrysize;        /* length of query text */
    i4              psq_datasize;       /* total size of DB_DATA_VALUES
					** and the data referenced
					*/
    i4		    psq_dnum;		/* number of db data values */
    char            *psq_qrytext;       /* text portion of query */
    DB_DATA_VALUE   **psq_qrydata;      /* data portion of query */
}   PSQ_QDESC;
#ifdef NOT_NEEDED

/*}
** Name: PSQ_PLIST - parameters for 5.0 execute query statement
**
** Description:
**      The structure defines the parameter list for a
**  5.0 repeat query execute statement.
**
** History:
**      10-nov-86 (daved)
**          written
*/
typedef struct _PSQ_PLIST
{
    i4              psq_pnum;           /* number of parameters */
    PTR             *psq_params;        /* pointers to the data values */
}   PSQ_PLIST;
#endif

/*}
** Name: PST_DECVAR - Procedure parameter or local variable definition
**
** Description:
**	This holds the definition of procedure parameter or local variable
**	definitions.
**
** History:
**	27-apr-88 (stec)
**	    Created.
**	9-june-06 (dougi)
**	    Added pst_parmmode for IN, OUT, INOUT procedure parms.
*/
typedef struct _PST_DECVAR
{
	/* Pst_nvars gives the number of local variables or DBP parameters
	** that are being defined.
	*/
    i4  		pst_nvars;

	/* Pst_first_varno gives the number of the first local var, procedure
	** parameter. The rest of the vars are numbered consequtively.
	*/
    i4  		pst_first_varno;

	/* Pst_vardef is a pointer to an array of DB_DATA_VALUEs that describe
	** the type, length, and precision of each variable or parameter.
	*/
    DB_DATA_VALUE	*pst_vardef;

	/* Pst_varname is a pointer to an array of DB_PARM_NAMEs that hold
	** parameter names.
	*/
    DB_PARM_NAME	*pst_varname;

	/* pst_parmmode is pointer to an array of parameter mode values.
	*/
    i4			*pst_parmmode;
#define	PST_PMIN	0x01		/* IN */
#define	PST_PMOUT	0x02		/* OUT */
#define	PST_PMINOUT	0x04		/* INOUT */
}   PST_DECVAR;

/*  History:
**	24-aug-88 (stec)
**	    Defines for built-ins.
**	30-may-89 (neil)
**	    Rules: Defined PST_DBPBUILTIN for maximum param# of built-ins.
*/
#define	    PST_RCNT	    0	    /* iirowcount    */
#define	    PST_ERNO	    1	    /* iierrornumber */

#define	    PST_DBPBUILTIN	1   /* increase this as more are added */


/*}
** Name: PST_IFSTMT - If statement specific info.
**
** Description:
**	This holds the condition and statements required for an if statement.
**
** History:
**	27-apr-88 (stec)
**	    Created.
*/
typedef struct _PST_IFSTMT
{
	/* Pst_condition points to a qualification like qnode tree that will
	** consist only of const and op (including AND and OR) nodes.
	*/
    PST_QNODE	    *pst_condition;

	/* Pst_true and pst_false are pointers to statements that should be
	** executed if pst_condition is true of false respectively.
	*/
    struct _PST_STATEMENT   *pst_true;
    struct _PST_STATEMENT   *pst_false;
}   PST_IFSTMT;

/*}
** Name: PST_FORSTMT - For statement specific info.
**
** Description:
**	This holds info to allow linkage of header nodes of FOR-loop.
**
** History:
**	25-sep-98 (inkdo01)
**	    Created.
*/
typedef struct _PST_FORSTMT
{
	/* Pst_forhead addresses the QTREE node of the select statement
	*/
    struct _PST_STATEMENT   *pst_forhead;
}   PST_FORSTMT;

/*}
** Name: PST_MSGSTMT - Message statement specific info.
**
** Description:
**	This holds the message text and number required for a MESSAGE
**	and RAISE ERROR statements.
**
** History:
**	27-apr-88 (stec)
**	    Created.
**	06-mar-89 (neil)
**          Rules: Now also used for RAISE ERROR statement.
**	07-nov-91 (andre)
**	    merge 27-feb-91 (ralph) change:
**		Add PST_MSGSTMT.pst_msgdest to support the DESTINATION clause of
**		the MESSAGE and RAISE ERROR statements
*/
typedef struct _PST_MSGSTMT
{
    PST_QNODE	*pst_msgnumber;
    PST_QNODE	*pst_msgtext;
    i4		pst_msgdest;
}   PST_MSGSTMT;

/*}
** Name: PST_RTNSTMT - Return statement specific info.
**
** Description:
**	This holds the return value that is optional for the return stmt.
**	If pst_rtn value is null, there is no value to be returned, else
**	it points to a constant node that identifies a parameter or local
**	variable or a constant.
**
** History:
**	27-apr-88 (stec)
**	    Created.
*/
typedef struct _PST_RTNSTMT
{
    PST_QNODE	    *pst_rtn_value;
}   PST_RTNSTMT;

/*}
** Name: PST_CMTSTMT - Commit statement.
**
** Description:
**	There is no specific info associated with the commit statement.
**	A char filed has been defined to make the length non-zero.
**
** History:
**	27-apr-88 (stec)
**	    Created.
*/
typedef struct _PST_CMTSTMT
{
    char		pst_unused;
}   PST_CMTSTMT;

/*}
** Name: PST_RBKSTMT - Rollback statement specific info.
**
** Description:
**	There is no specific info associated with the rollback statement.
**	A char filed has been defined to make the length non-zero.
**
** History:
**	27-apr-88 (stec)
**	    Created.
*/
typedef struct _PST_RBKSTMT
{
    char		pst_unused;
}   PST_RBKSTMT;

/*}
** Name: PST_RSPSTMT - Rollback to savepoint statement specific info.
**
** Description:
**	This holds the savepoint value for the rollback to savepoint 
**	statement. The value is tored in a constant node that identifies
**	a parameter or local variable or a constant.
**
** History:
**	27-apr-88 (stec)
**	    Created.
*/
typedef struct _PST_RSPSTMT
{
    PST_QNODE		*pst_savept;
}   PST_RSPSTMT;

/*}
** Name: PST_CPROCSTMT - "Call a procedure" statement info.
**
** Description:
**	This structure defines the information used for the nested procedure
**	calls implied when processing rules on other statements.  This structure
**	is not yet available for user-issued nested procedure calls, through
**	it should be sufficient.  Different semantics may be applied to the
**	argument list when user-issued by using the pst_ptype field.
**
**	The procedure name is stored in a QSO alias (QSO_NAME) which is not
**	necessarily a "validated" procedure (ie, only the external names -
**	procedure and owner are required).
**
**	If the procedure is invoked via a rule then the rule name is saved
**	for debugging and rule tracing.
**
**	The parameters to the procedure (of type PST_CPRULE) are stored as
**	pairs of RESDOM nodes (with PST_DBPARAMNO types) and either RULEVAR or
**	CONST nodes with the source values (if no parameters a single
**	PST_TREE node).
**	
** History:
**	25-jan-89 (neil)
**	    Created for rules processing.
**	09-may-89 (neil)
**	    Added pst_rulename to for tracing rule firing.
**	26-oct-92 (jhahn)
**	    Added pst_return_status for DB procedures..
**	26-oct-92 (jhahn)
**	    Added pst_tuple for processing set input procedures.
**	17-nov-00 (inkdo01)
**	    Changed pst_ptype to pst_pmask to store flags, including new
**	    indication to use prefetch strategy for update triggering rule.
*/
typedef struct _PST_CPROCSTMT
{
    DB_NAME		pst_rulename;	/* If this field is not empty then 
					** when the called procedure is invoked
					** and tracing is on this rule name
					** should be displayed.  If this does
					** not originate from a rule (ie, a
					** nested procedure call) then this
					** field is cleared.
					*/
    DB_OWN_NAME		pst_ruleowner;	/* Owner of rule for security audit. */
    QSO_NAME		pst_procname;
    i4			pst_pmask;	/* Tag differentiates between
					** usages of this node:	
					*/
#define	  PST_CPRULE	    0x01	/* Invoked by rule processing */
#define	  PST_CPUSER	    0x02	/* Explicit user invocation - not yet */
#define	  PST_UPD_PREFETCH  0x04	/* TRUE - triggering update statements
					** must use prefetch strategies */
#define   PST_CPSYS	    0x08	/* System generated rule */
#define   PST_DEL_PREFETCH  0x10	/* TRUE - triggering delete stmts
					** must use prefetch strategies */

    i4			pst_argcnt;	/* # of actuals in pst_arglist */
    PST_QNODE		*pst_arglist;	/* Actual args as spec'd in rule */
    PST_RSDM_NODE	*pst_return_status;
					/* NULL or should point to where return
					** status should be stored. */
    DB_PROCEDURE	*pst_ptuple;	/* pointer to tuple for this procedure*/
}   PST_CPROCSTMT;

/*}
** Name: PST_IPROCSTMT - This causes a DBMS internal procedure to be executed.
**
** Description:
**      This statement causes a DBMS internal database procedure to be executed.
**      Note that because of the way that this statement works, it can be the
**      *only* statement in a procedure. It another procedure wishes to use 
**      this statement, then it must do so through the callproc statement.
**       
**      This structure only holds the name and owner of the procedure to
**      be executed because the DBP is in reality "hard coded" in QEF.
**      The name and owner allow QEF to call the correct routine to do the
**      job. 
[@comment_line@]...
**
** History:
**      21-apr-89 (eric)
**	    defined
[@history_template@]...
*/
typedef struct _PST_IPROCSTMT
{
    DB_DBP_NAME	    pst_procname;	/* The name of the DBP */
    DB_OWN_NAME	    pst_ownname;	/* The owner of the DBP */
}   PST_IPROCSTMT;

/*}
** Name: PST_REGPROC_STMT - This causes a Registered procedure to be executed.
**
** Description:
**      This statement causes a registered database procedure to be executed.
**      Note that because of the way that this statement works, it can be the
**      *only* statement in a procedure. 
**
**	Note: The registered procedure does not look at all like a procedure
**	      in the catalogs. A registered procedure does not resemble a
**	      database procedure, it merely indicates which LDBMS to pass the
**	      run procedure request to.
**       
**      This structure holds the name and owner of the procedure to
**      be executed and a ptr to the LDB descriptor.  That is because star
**	passes this to local and Local does the work.
[@comment_line@]...
**
** History:
**      07-dec-92 (teresa)
**	    Initial creation
[@history_template@]...
*/
typedef struct _PST_REGPROC_STMT
{
    DB_DBP_NAME	    pst_procname;	/* The name of the DBP */
    DB_OWN_NAME	    pst_ownname;	/* The owner of the DBP */
    DD_REGPROC_DESC *pst_rproc_desc;	/* registered procedure descriptor */
}   PST_REGPROC_STMT;

/*}
** Name: PST_EVENTSTMT - RAISE/REGISTER/REMOVE EVENT statement information.
**
** Description:
**	This structure defines the information used for all event processing
**	in the parser.  For the event statements the event name and owner are
**	extracted from RDF and deposited into the respective fields.  The
**	current database name is extracted from SCF.
**
**	The 'flags' and 'value' fields are only used by the RAISE statement
**	and should be nulled otherwise.
**
**	This structure is passed in from PSF to OPF where it is compiled into
**	a QEF_EVENT action.
**
** History:
**	22-aug-89 (neil)
**	    Written for Terminator II/alerters.
**	6-dec-93 (robf)
**          Add event security label.
*/
typedef struct _PST_EVENTSTMT
{
    DB_ALERT_NAME	pst_event;
    i4		pst_evflags;	/* Modifiers per statement */
# define		  PST_EVNOSHARE		0x01	/* RAISE with NOSHARE */
    PST_QNODE		*pst_evvalue;	/* Optional text for RAISE statement */
} PST_EVENTSTMT;

/*}
** Name: PST_COL_ID - structure for linked list of column ids
**
** Description:
**	This structure is used to store a linked list of column
**      ids.  Currently used for keeping track of the order of unique and
**      referenced columns when creating constraints.
**
** History:
**	15-oct-92 (rblumer)
**	    Created for use in PST_CREATE_INTEGRITY.
*/
typedef	struct	_PST_COL_ID
{
    DB_ATT_ID		col_id;
    struct _PST_COL_ID	*next;

}	PST_COL_ID;


/*}
** Name: PST_CREATE_TABLE - Descriptor for "Create Table" statement
**
** Description:
**	This structure defines the information used for creating a table.
**	The bulk of this information lives in a DMU_CB, with some in a QEU_CB.
**	In the old fashion way of doing business, PSF used to store the
**	DMU_CB in a QEU_CB and hand that QEU_CB directly to QEF.
**	Well... PSF still does that.  However, with the FIPS CONSTRAINTS
**	project, PSF can now also pass a query tree with a CREATE TABLE
**	node to OPF, mimicking the flow of control for DML statements.
**	Our object all sublime is to have ALL statements (DML and DDL)
**	follow the DML flow of control eventually.
**      NOTE: the extra info in the QEU_CB (vs the DMU_CB) is mainly for
**	distributed case, which we aren't handling yet, and to hold the
**	opcode for the different flavors of CREATE.
** 
** History:
**	17-aug-92 (rickh)
**	    Created for FIPS CONSTRAINTS.
**	28-aug-92 (rblumer)
**	    Changed the DMU_CB to a QEU_CB, added #define for CRT_TABLE
**	18-apr-94 (rblumer)
**	    changed PTR to struct _QEU_CB *, since that is more type-correct
**	    and yet will still compile even if that structure definition
**	    is not included by all includers of THIS header file.
**	22-jul-96 (ramra01)
**	    Alter table Add/Drop column create table flags.
**	15-oct-2009 (gupsh01)
**	    Fix define values. 
**	28-Jul-2010 (kschendel) SIR 124104
**	    Add compression so that auto structure can maintain compression.
*/
typedef	struct	_PST_CREATE_TABLE
{
    i4	pst_createTableFlags;
#define	PST_CRT_TABLE   	1L	 /* creating a base table */
#define	PST_CRT_INDEX		2L	 /* creating a secondary index */
#define	PST_ATBL_ADD_COLUMN 	3L 	 /* alter table add column */
#define	PST_ATBL_DROP_COLUMN  	4L  	 /* alter table drop column */
#define       PST_ATBL_ALTER_COLUMN   5L     /* alter table alter column */
                                  /* may eventually allow temp tables,etc */

    i2	pst_autostruct;		/* flag field for auto structure options */
				/* Flags defined in PST_RESTAB */
    i2	pst_compress;		/* Compression flags, same as PST_RESTAB */
				/* OPF doesn't care much about this, it depends
				** on the pst_restab or DMU char stuff, but
				** it does pass this along to the create
				** integrity action for use by auto-structure
				*/
    struct _QEU_CB	*pst_createTableQEUCB;	 
				 /* the DMU_CB in this QEUCB describes table */
}	PST_CREATE_TABLE;

/*
** Name: PST_RENAME
**
**	This is a descriptor for ALTER TABLE RENAME .... type of statements. 
** History:
**	18-Mar-2010 (gupsh01) SIR 123444
**	    Created.
*/
typedef	struct	_PST_RENAME
{
    struct _QEU_CB	*pst_rnm_QEUCB;	 
				 /* the DMU_CB in this QEUCB describes table */
    PTR                 pst_rnm_qeuqCB; /* QEUQ_CB describing DDL statement */
    DB_TAB_ID           pst_rnm_tab_id;     /* Table id for table being renamed */
    DB_OWN_NAME         pst_rnm_owner;      /* owner of tab constraint applies to */
    i4			pst_rnm_type;
#define	PST_ATBL_RENAME_TABLE	0x01  	 /* alter table rename table */
#define	PST_ATBL_RENAME_COLUMN  0x02  	 /* alter table rename column */
    DB_TAB_NAME         pst_rnm_old_tabname;  /* Original table name renamed */
    DB_TAB_NAME         pst_rnm_new_tabname;  /* new table name, if table rename op */
    DB_ATT_ID           pst_rnm_col_id;       /* Column id,   if column rename */
    DB_ATT_NAME         pst_rnm_old_colname;  /* Column name, if column rename*/
    DB_ATT_NAME         pst_rnm_new_colname;  /* New column name, if column rename */
}	PST_RENAME;

/*
**  Name: PST_TEXT_STORAGE
**
**        Similar to DB_TEXT_STRING which may only store upto 64k
**        of data and is externally visible. This struct may store
**        upto 4Gb, and is for internal use.
**
**  History:
**        26-Oct-2009 (coomi01) b122714
**            Created.
*/  
typedef struct {
    u_i4            pst_t_count;         /* The number of chars in the string */
    u_char          pst_t_text[1];       /* The actual string */
} PST_TEXT_STORAGE; 

/*
** Name: PST_CREATE_VIEW - descriptor for CREATE VIEW statement
**
** Description:
**	this structure will contain information used for creating a view
**
** History:
**	23-nov-92 (andre)
**	    created for WITH CHECK OPTION project
**	28-may-93 (andre)
**	    changed type of pst_qeuqcb from PTR to (struct _QEUQ_CB *)
**      13-Jun-08 (coomi01) Bug 110708
**          Add an extra flag, PST_SUPPRESS_CHECK_OPT, to pst_flags 
**      26-Oct-2009 (coomi01) b122714
**          Use PST_TEXT_STORAGE to allow for long (>64k) view text.
**          Changed name to pst_view_text_storage to distinguish new
**          from old.
*/
typedef struct PST_CREATE_VIEW_
{
    struct _QEUQ_CB	*pst_qeuqcb;
    DB_TAB_NAME		pst_view_name;
    PST_TEXT_STORAGE	*pst_view_text_storage;
    PST_QTREE		*pst_view_tree;
    i4			pst_flags;

						/*
						** CHECK OPTION will need to be
						** enforced dynamically after
						** INSERT into the view
						*/
#define	    PST_CHECK_OPT_DYNAMIC_INSRT	    0x01

						/*
						** CHECK OPTION will need to be
						** enforced dynamically after
						** UPDATE; depending on whether
						** PST_VERIFY_UPDATE_OF_ALL_COLS
						** is set, enforcement will have
						** to take place after any
						** UPDATE of the view or after
						** UPDATE involving columns
						** described by pst_updt_cols
						*/
#define	    PST_CHECK_OPT_DYNAMIC_UPDT	    0x02

						/*
						** will be set if UPDATE of
						** every updatable column of the
						** view has a potential to
						** result in violation of CHECK
						** OPTION
						*/
#define	    PST_VERIFY_UPDATE_OF_ALL_COLS   0x04

						/*
						** creating a STAR view
						*/
#define	    PST_STAR_VIEW		    0x08
						/*
						** status word for view creation
						*/
#define	    PST_SUPPRESS_CHECK_OPT	    0x10
						/*
						** status word for view creation
						*/
    i4			pst_status;		
    

    DB_COLUMN_BITMAP	pst_updt_cols;
} PST_CREATE_VIEW;

/*}
** Name: PST_MODIFY_TABLE - Descriptor for "Modify Table " statement
**
** Description:
**	This structure defines the information used for modifying a table.
**
** History:
**	17-aug-92 (rickh)
**	    Created for FIPS CONSTRAINTS.
**	18-apr-94 (rblumer)
**	    changed PTR to struct _DMU_CB *, since that is more type-correct
**	    and yet will still compile even if that structure definition
**	    is not included by all includers of THIS header file.
*/
typedef	struct	_PST_MODIFY_TABLE
{
    struct _DMU_CB	*pst_modifyTableDMUCB;	
				/* DMU_CB control block describes table */
}	PST_MODIFY_TABLE;

/*}
** Name: PST_CREATE_RULE - Descriptor for "Create Rule" statement
**
** Description:
**	This structure defines the information used for creating a rule.
**
** History:
**	17-aug-92 (rickh)
**	    Created for FIPS CONSTRAINTS.
**	19-nov-92 (rblumer)
**	    Changed QSO_OBIDs to text string and qtree pointers.
*/
typedef	struct	_PST_CREATE_RULE
{
    i4		pst_createRuleFlags;
#define	PST_RULE_TREE_EXISTS	0x00000001L
					/* true if there's a tree representing
					** the paramater list and/or rule
					** qualification.
					*/
    DB_IIRULE	*pst_ruleTuple;
		    /*  Rule tuple to insert.  All user-specified fields of this
		    **  parameter are filled in by the grammar.  
		    **  The owner of the rule must be filled in from pss_user.
		    **  Internal fields (tree and text ids) must be filled in 
		    **  just before storage in QEU when the ids are constructed.
		    */
    DB_TEXT_STRING *pst_ruleQueryText;	/* Id of query text as stored in QSF. */
    PST_QTREE      *pst_ruleTree;	/* Query tree representing parameter
					** list and/or qualification of rule if
					** PST_RULE_TREE_EXISTS is set.
					*/
}	PST_CREATE_RULE;

/*}
** Name: PST_CREATE_PROCEDURE - Descriptor for "Create Procedure" statement
**
** Description:
**	This structure defines the information used for creating a procedure.
**
** History:
**	17-aug-92 (rickh)
**	    Created for FIPS CONSTRAINTS.
**	19-nov-92 (rblumer)
**	    Changed QSO_OBID to text string.
*/
typedef	struct	_PST_CREATE_PROCEDURE
{
    DB_PROCEDURE	*pst_procedureTuple;
		/* Procedure tuple to insert.   Fill in fields in grammar.
		** The owner of the rule must be filled in from pss_user.
		** Internal fields (dbproc and text ids) must be filled in just
		** before storage in QEU when the ids are constructed. */
    DB_TEXT_STRING	*pst_procedureQueryText;
		/* Id of query text as stored in QSF. */
}	PST_CREATE_PROCEDURE;

/*}
** Name: PST_CREATE_INTEGRITY - Descriptor for "Create Integrity" statement
**
** Description:
**	This structure defines the information used for creating an integrity.
**
** History:
**	17-aug-92 (rickh)
**	    Created for FIPS CONSTRAINTS.
**	19-nov-92 (rblumer)
**	    Changed QSO_OBIDs to text string and qtree pointers;
**	    added pst_cons_ownname field.
**	18-feb-93 (rickh)
**	    Added constraint schema name to the CREATE_INTEGRITY structure.
**	26-mar-93 (andre)
**	    added a ptr to PSQ_INDEP_OBJECTS structure - pst_indep.
**	    This structure will be allocated for REFERENTIAL constraints only
**	    and will be used to describe the UNIQUE/PRIMARY KEY constraint and,
**	    if the owner of the referenced table is different from that of the
**	    referencing table, REFERENCES privilege on which the new REFERENCES
**	    constraint will depends.
**	    In the interests of avoiding having to #include PSFINDEP.H  all over
**	    creation, this field will be declared as a PTR and those who want to
**	    access it will have to cast it to (PSQ_INDEP_OBJECTS *)
**	10-sep-93 (rblumer)
**	    Added PST_VERIFY_CONS_HOLDS bit to pst_createIntegrityFlags,
**	    for use in ALTER TABLE.  Tells QEF to verify that constraint holds
**	    on existing data before creating a constraint on the table.
**	18-apr-94 (rblumer)
**	    changed pst_integrityTuple2 to the more descriptive name
**	    pst_knownNotNullableTuple. 
**	30-mar-98 (inkdo01)
**	    Added PST_RESTAB to house constraint index options.
**	26-may-98 (inkdo01)
**	    Added flags for delete/update cascade/set null referential actions.
**	20-oct-98 (inkdo01)
**	    Added flags for delete/update restrict refacts (to distinguish
**	    from "no action").
**	3-may-2007 (dougi)
**	    Added flag to force base table structure to btree on constrained
**	    columns.
**	28-Jul-2010 (kschendel) SIR 124104
**	    Add compression so that auto structure can maintain compression.
*/
typedef	struct	_PST_CREATE_INTEGRITY
{
    i4			pst_createIntegrityFlags;
#define	PST_CONSTRAINT	       0x00000001
	/* set if this is an ANSI-style constraint 
	 */
#define	PST_SUPPORT_SUPPLIED   0x00000002
	/* no need to create an index to support this constraint.  the
	** user linked this constraint to the user supplied index or base
	** table stored in pst_suppliedSupport. NOT SUPPORTED - superceded
	** by index option with clause.
	*/
#define PST_CONS_NAME_SPEC     0x00000004  /* name has been specified by user */
#define PST_VERIFY_CONS_HOLDS  0x00000008
        /*
        ** This flag is set when we're adding a constraint to a pre-existing
        ** table.  This flag instructs QEF to cook up a SELECT query to verify
        ** that the constraint holds on any existing data in the table
	** BEFORE adding the constraint to the table.
        */
#define PST_CONS_WITH_OPT      0x00000010
	/* Index options for this constraint explicitly specified through
	** with clause and contained in pst_restab.
	*/
#define PST_CONS_NEWIX	       0x00000020
	/* New named index specified in with clause.
	*/
#define PST_CONS_OLDIX	       0x00000040
	/* Existing index specified in with clause.
	*/
#define PST_CONS_NOIX	       0x00000080
	/* No index to be defined for this (referential) constraint.
	*/
#define PST_CONS_BASEIX	       0x00000100
	/* This constraint uses the existing base table structure.
	*/
#define PST_CONS_SHAREDIX      0x00000200
	/* The index created for this constraint is shared amongst
	** others. */
#define PST_CONS_DEL_CASCADE   0x00001000
	/* This referential constraint has DELETE CASCADE action.
	*/
#define PST_CONS_DEL_SETNULL   0x00002000
	/* This referential constraint has DELETE SET NULL action.
	*/
#define PST_CONS_DEL_RESTRICT  0x00004000
	/* This referential constraint has DELETE RESTRICT action.
	*/
#define PST_CONS_UPD_CASCADE   0x00008000
	/* This referential constraint has UPDATE CASCADE action.
	*/
#define PST_CONS_UPD_SETNULL   0x00010000
	/* This referential constraint has UPDATE SET NULL action.
	*/
#define PST_CONS_UPD_RESTRICT  0x00020000
	/* This referential constraint has UPDATE RESTRICT action.
	*/
#define PST_CONS_TABLE_STRUCT	0x00040000
	/* Modify table to btree on constraint keys.
	*/

    DB_TAB_ID	pst_suppliedSupport;	/* if PST_SUPPORT_SUPPLIED is
					** set, then this is the id of the
					** index (or base table) whose
					** clustering scheme supports this
					** constraint
					*/

    DB_INTEGRITY	*pst_integrityTuple;
		/* tuple to insert into iiintegrity.  PSF fills in the
		** constraint name, schema id, constraint flags, and
		** the bit map of restricted columns.
		*/

    DB_INTEGRITY	*pst_knownNotNullableTuple;
		/* 2nd tuple to insert into iiintegrity. Only used for
	        ** CHECK constraints that cause a column to become "known
		** "known not nullable."  PSF fills in the constraint
		** name, schema id, constraint flags, and the bit map of
		** "known not nullable" columns.
		*/
    DB_TAB_NAME	      pst_cons_tabname;  /* table name constraint applies to */
    DB_OWN_NAME	      pst_cons_ownname;  /* schema name of table */
    PST_COL_ID	      *pst_cons_collist; /* in-order linked list of unique
					 ** or referencing column ids        
					 */
    DB_TAB_NAME	      pst_ref_tabname;   /* referenced table name (if any)   */
    DB_OWN_NAME	      pst_ref_ownname;   /* schema name for referenced table */
    PST_COL_ID	      *pst_ref_collist;	 /* in-order linked list of 
					 ** referenced column ids            
					 */
    DB_CONSTRAINT_ID  pst_ref_depend_consid; 
		/* if a referential constraint, the unique constraint 
		** (on the referenced table) upon which this referential 
		** constraint depends 
		*/
    DB_TEXT_STRING    *pst_integrityQueryText;
		/* query text of Constraint clause
		 */
    DB_TEXT_STRING    *pst_checkRuleText;
		/* text to be used as for create rule stmt for check constraint;
		** this text is for the WHERE clause of the rule.
		** It has column names qualified with the correlation name NEW.
		*/
    PST_QNODE         *pst_integrityTree;
		/* Query tree (boolean expression) representing the integrity;
		** only used for INGRES integrities (in the future, that is)
		*/
    DB_SCHEMA_NAME	pst_integritySchemaName;
		/* name of the schema that the integrity is defined in */

    PTR			pst_indep;
		/* description of object(s) and/or privilege(s) on which a
		** constraint depends
		*/
    PST_COL_ID	      *pst_key_collist;
		/* linked list of column ids to be turned into index
		*/
    i2		      pst_key_count;
		/* count of columns in key column list */
    i2		      pst_compress;
		/* Compression indicator, same as PST_RESTAB.  The path to
		** get here is sess_cb->pss_restab.pst_compress is copied
		** to pst_createTable.pst_compress is copied (by opc) to
		** pst_createIntegrity.pst_compress.  Goofy, but it's the
		** simplest way to do it.
		** Note that the compression indicator is only set when
		** auto-structure is asked for.
		*/
    PST_RESTAB	      pst_indexopts;
		/* constraint index options */

}	PST_CREATE_INTEGRITY;

/*}
** Name: PST_DROP_INTEGRITY - Descriptor for "Drop Integrity" statement
**
** Description:
**	This structure defines the information used for dropping an integrity.
**
** History:
**	17-aug-92 (rickh)
**	    Created for FIPS CONSTRAINTS.
**	13-mar-93 (andre)
**	    rearrange the structure to support destruction of integrities and
**	    constraints
*/
typedef	struct	PST_DROP_INTEGRITY_
{
    i4			pst_flags;
					    /*
					    ** set if dropping an "old-style"
					    ** integrity; 
					    */
#define	PST_DROP_INGRES_INTEGRITY	0x01
					    /*
					    ** set if dropping a (UNIQUE, CHECK,
					    ** or REFERENCES) cosntraint;
					    */
#define	PST_DROP_ANSI_CONSTRAINT	0x02
					    /*
					    ** set if RESTRICT was NOT specified
					    ** on ALTER TABLE DROP CONSTRAINT
					    ** (CASCADE is the default behavior)
					    */
#define	PST_DROP_CONS_CASCADE		0x04
					    /*
					    ** once we start using this
					    ** structure to process DROP
					    ** INTEGRITY, this bit will be set
					    ** if a user requested that all
					    ** integrities be dropped
					    */
#define	PST_DROP_ALL_INTEGRITIES	0x08

    union
    {
	/*
	** if dropping "old style" integrity(ies), the description will be
	** stored here
	*/
	struct PST_DROP_INTEG_DESCR_
	{
					    /*
					    ** integrity number if a single
					    ** integrity is being dropped;
					    ** if PST_DROP_ALL_INTEGRITIES is
					    ** set, contents of pst_integ_number
					    ** are undefined
					    */
	    i4		    pst_integ_number;
					    /*
					    ** id of the table integrity(ies) on
					    ** which is/are being dropped
					    */
	    DB_TAB_ID	    pst_integ_tab_id;
	} pst_drop_integ_descr;

	/*
	** if dropping a (CHECK, UNIQUE, or REFERENCES) constraint, its
	** description will go here
	*/
	struct PST_DROP_CONS_DESCR_
	{
					    /*
					    ** name of the table a constraint
					    ** on which is to be dropped
					    */
	    DB_TAB_NAME		pst_cons_tab_name;

					    /*
					    ** schema to which the constraint
					    ** belongs
					    */
	    DB_OWN_NAME		pst_cons_schema;

					    /*
					    ** id of the table a constraint on
					    ** which is to be dropped
					    ** this field will enable us to
					    ** verify that we are dropping a
					    ** constraint on the correct table
					    */
	    DB_TAB_ID		pst_cons_tab_id;
	    
					    /*
					    ** name of the constraint to be
					    ** dropped
					    */
	    DB_CONSTRAINT_NAME	pst_cons_name;

					    /*
					    ** id of the schema to which the
					    ** constraint being dropped belongs
					    */
	    DB_SCHEMA_ID	pst_cons_schema_id;
	} pst_drop_cons_descr;
    } pst_descr;

} PST_DROP_INTEGRITY;

/*}
** Name: PST_EXEC_IMM - Execute Immediate statement definition.
**
** Desciption:
**      This holds the definition of an Execute Immediate statement.
**
** History:
**      27-oct-92 (anitap)
**          Written for CREATE SCHEMA project.
**	12-dec-92 (ralph)
**	    Changed PST_EXEC_IMM.pst_info to be an embedded structure
**	    rather than a pointer.
*/
typedef struct _PST_EXEC_IMM
{
  DB_TEXT_STRING *pst_qrytext;    /* query text of CREATE VIEW or GRANT
                                  ** statement. This query text will be given
                                  ** to PSF by to process the statement.
                                  */
  PST_INFO      pst_info;      /* info needed by the parser while parsing
                               ** Execute Immediate actions.
                               */
} PST_EXEC_IMM;

/*}
** Name: PST_CREATE_SCHEMA - Schema definition
**
** Description:
**      This holds the definition of a schema.
**
** History:
**      13-oct-92 (anitap)
**          Written for CREATE SCHEMA project.
**	04-mar-93 (anitap)
**	    Changed PST_CREATE_SCHEMA to _PST_CREATE_SCHEMA.
*/
typedef struct _PST_CREATE_SCHEMA
{
   DB_SCHEMA_NAME       pst_schname;     /* schema name */
   DB_OWN_NAME          pst_owner;       /* schema owner. Schema name &
                                        ** owner take same value for
                                        ** 6.5. Post 6.5 will support
                                        ** a single user having more
                                        ** than one schema/database
                                        */
} PST_CREATE_SCHEMA;

/*}
** Name: PST_STATEMENT - Procedure statement definition
**
** Description:
**	This defines each statement that can be included in a procedure.
**	Note that if the PST_LOCALVAR statement, which declares local
**	variables, is used then it must be the first statement in
**	the procedure.
**
**	Currently all parsed statements are under this structure, causing
**	single statements to be degenerate cases of a DBP (with a single
**	pst_tree).
**
**	Rules fired after the processing of a statement (whether in a DBP
**	or alone) are themselves a list of statements, built in qrymod.
**	
** History:
**	27-apr-88 (stec)
**	    Created.
**	25-jan-89 (neil)
**	    Added pst_after_stmt for "after statement rules processing".
**	    Added PST_CP_TYPE to extend for PST_CPROCSTMT.
**	20-apr-89 (andre)
**	    Get rid of pst_bgnstmt, pst_endstmt and remove #ifdef, #endif from
**	    around pst_lineno.
**	21-apr-89 (neil)
**	   Commented overloaded use of pst_opf for rules.
**	24-jul-89 (jennifer)
**	    added pst_audit for passing audit information to QEF via QP.
**	07-sep-89 (neil)
**	   Alerters: Added new pst_types and corresponding pst_eventstmt.
**	06-feb-91 (andre)
**	    Added a new statement type - ENDLOOP;
**	    (this was done in conjunction with a fix fir bug #35659)
**	17-aug-92 (rickh)
**	    Improvements for FIPS CONSTRAINTS.  DDL support.  Added DDL
**	    statement types, pst_statementEndRules.
**      13-oct-92 (anitap)
**          Added new statement types for EXECUTE IMMEDIATE and CREATE
**          SCHEMA statements.
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          Added pst_stmt_mask to flag IF statements that are in non-cnf
**          form.
**	15-june-06 (dougi)
**	    Added pst_before_stmt and pst_before_statementEndRules for 
**	    before triggers.
*/

typedef enum _PST_STTYPE
{
    PST_DV_TYPE =	1,	/* PST_DECVAR	*/
    PST_QT_TYPE =	2,	/* PST_QTREE	*/
    PST_IF_TYPE =	3,	/* PST_IFSTMT	*/
    PST_MSG_TYPE =	4,	/* PST_MSGSTMT	*/
    PST_RTN_TYPE =	5,	/* PST_RTNSTMT	*/
    PST_CMT_TYPE =	6,	/* PST_CMTSTMT  */
    PST_RBK_TYPE =	7,	/* PST_RBKSTMT  */
    PST_RSP_TYPE =	8,	/* PST_RSPSTMT  */
    PST_WH_TYPE =	9,	/* Reserved for PSF use at this time */
    PST_CP_TYPE =	10,	/* PST_CPROCSTMT - EXECUTE/CALL PROCEDURE */
    PST_EMSG_TYPE =	11,	/* PST_MSGSTMT - same as MESSAGE */
    PST_IP_TYPE =	12,	/* PST_IPROCSTMT - execute an internal DBP */
    PST_EVREG_TYPE =	13,	/* REGISTER EVENT - data is PST_EVENTSTMT */
    PST_EVDEREG_TYPE =	14,	/* REMOVE EVENT - data is PST_EVENTSTMT */
    PST_EVRAISE_TYPE =	15,	/* RAISE EVENT - data is PST_EVENTSTMT */
    PST_ENDLOOP_TYPE =	16,	/* ENDLOOP - will not be visible outside PSF -
				** the only truly useful field is pst_next! */
    PST_EXEC_IMM_TYPE =	17,     /* implies EXECUTE IMMEDIATE type */
    PST_REGPROC_TYPE =	18,	/* PST_REGPROC_STMT - execute a registered DBP*/
    PST_RP_TYPE =	19,	/* Reserved for PSF use at this time */
    PST_FOR_TYPE =	20,	/* FOR loop */
    PST_RETROW_TYPE =	21,	/* RETURN ROW */
} PST_STTYPE;

typedef struct _PST_STATEMENT
{

	/* Pst_stmttype gives the type of statement that is to be
	** executed. The allowable values ate the same as for
	** the psq_mode in PSQ_CB.  This means that statment/query type/modes
	** will be listed in three places, PST_QTREE.pst_mode,
	** PSQ_CB.psq_mode, and PST_STATEMENT.pst_stmttype.
	*/
    i4			pst_mode;

	/* Pst_next points to the statement that should be executed
	** after the current statement is executed. The linked list
	** of statements pointed to by pst_true or pst_false of the
	** PST_IFSTMT is not NULL terminated, they point to the next
	** statement to be executed after the IF.
	** (QEF follows pst_next to execute statements in order.)
	*/
    struct _PST_STATEMENT *pst_next;

	/*						
	** pst_after_stmt points at a statement list that includes actions
	** to take when processing a rule associated with the current statement.
	** Multiple statements (multiple rules) may apply to the same current
	** statement.  The tail node of this "after" tree should NOT point
	** at the next statement of the parent statement.  This tree is filled
	** in by qrymod in psy_rule.
	*/
    struct _PST_STATEMENT *pst_after_stmt;

	/* pst_statementEndRules points at a list of rules to be 
	** fired after this statement has finished with ALL its tuples.
	** Multiple statements (multiple rules) may apply to the same current
	** statement.  The tail node of this "after" tree should NOT point
	** at the next statement of the parent statement. 
	*/
    struct _PST_STATEMENT *pst_statementEndRules;

	/* pst_before_stmt anchors a statement list representing the before 
	** triggers to execute for the current statement. */
    struct _PST_STATEMENT *pst_before_stmt;

	/* pst_before_statementEndRules anchors the statement list 
	** representing the "for each statement" before triggers to execute
	** for the current statement. "for each statement" before rules
	** aren't yet supported, though. */
    struct _PST_STATEMENT *pst_before_statementEndRules;

	/* Pst_opf is for internal use by OPF. It is defined as PTR
	** so that OPF structures do not have to be included by all sources
	** refering to this header.
	**
	** This field is also used by PSF internally during qrymod processing.
	** When saving rule statement lists for cursors this field tags the
	** statement type of the rule (with a PSC_RULE_TYPE) so that different
	** rules can later be filtered out when actually applied.
	*/
    PTR			pst_opf;

	/* Indicates which variant of the union */
    PST_STTYPE		pst_type;

	/* statement types from 4000-7999 are reserved for DDL statements */

	/*
	** direct comparisons to the following two symbols should not be
	** made.  to figure out whether a statement is a DDL type, use
	** the PST_IS_A_DDL_STATEMENT macro.
	*/

#define	PST_FIRST_DDL_STATEMENT_TYPE	4000
#define	PST_LAST_DDL_STATEMENT_TYPE	7999

#define	PST_IS_A_DDL_STATEMENT( pstType ) 			\
	(    ( pstType >= PST_FIRST_DDL_STATEMENT_TYPE )	\
	  && ( pstType <=  PST_LAST_DDL_STATEMENT_TYPE ))

#define	PST_CREATE_TABLE_TYPE	( PST_FIRST_DDL_STATEMENT_TYPE )
	/* implies PST_CREATE_TABLE structure */
#define	PST_CREATE_INDEX_TYPE	( PST_FIRST_DDL_STATEMENT_TYPE + 1 )
	/* implies PST_CREATE_TABLE_STATEMENT structure */
#define	PST_CREATE_RULE_TYPE	( PST_FIRST_DDL_STATEMENT_TYPE + 2 )
	/* implies PST_CREATE_RULE structure */
#define	PST_CREATE_PROCEDURE_TYPE ( PST_FIRST_DDL_STATEMENT_TYPE + 3 )
	/* implies PST_CREATE_PROCEDURE structure */
#define	PST_CREATE_INTEGRITY_TYPE ( PST_FIRST_DDL_STATEMENT_TYPE + 4 )
	/* implies PST_CREATE_INTEGRITY structure */
#define	PST_MODIFY_TABLE_TYPE	( PST_FIRST_DDL_STATEMENT_TYPE + 5 )
	/* implies PST_MODIFY_TABLE structure */
#define	PST_DROP_INTEGRITY_TYPE	( PST_FIRST_DDL_STATEMENT_TYPE + 6 )
	/* implies PST_DROP_INTEGRITY structure */
#define PST_CREATE_SCHEMA_TYPE  (PST_FIRST_DDL_STATEMENT_TYPE + 7)
       /* implies PST_CREATE_SCHEMA structure */
#define PST_CREATE_VIEW_TYPE    ( PST_FIRST_DDL_STATEMENT_TYPE + 8 )
	/* implies PST_CREATE_VIEW structure */
#define PST_RENAME_TYPE    ( PST_FIRST_DDL_STATEMENT_TYPE + 9 )
	/* implies rename processing */

	/* Pst_specific holds the statement specific information */
    union
    {
	PST_DECVAR	*pst_dbpvar;
	PST_QTREE	*pst_tree;
	PST_IFSTMT	pst_if;
	PST_FORSTMT	pst_for;
	PST_MSGSTMT	pst_msg;
	PST_RTNSTMT	pst_rtn;
	PST_CMTSTMT	pst_commit;
	PST_RBKSTMT	pst_rollback;
	PST_RSPSTMT	pst_rbsp;
	PST_CPROCSTMT	pst_callproc;
	PST_IPROCSTMT	pst_iproc;
	PST_REGPROC_STMT pst_regproc;
	PST_EVENTSTMT	pst_eventstmt;
	PST_CREATE_TABLE	pst_createTable;
	PST_CREATE_RULE		pst_createRule;
	PST_CREATE_PROCEDURE	pst_createProcedure;
	PST_CREATE_INTEGRITY	pst_createIntegrity;
	PST_MODIFY_TABLE	pst_modifyTable;
	PST_DROP_INTEGRITY	pst_dropIntegrity;
	PST_EXEC_IMM            pst_execImm;
        PST_CREATE_SCHEMA       pst_createSchema;
	PST_CREATE_VIEW         pst_create_view;
	PST_RENAME	        pst_rename;
    } pst_specific;

    i4		pst_lineno;	/* Line number of the statement */

	/* Pst_link points to the next statement in the procedure.
	** It is simply a linked list of the procedure statements.
	** They are not guaranteed to be linked in any particular
	** order (i.e., the logical order is not preserved).
	** (OPF follows pst_link to guarantee that it compiles ALL 
	**  statements in a list.)
	*/
    struct _PST_STATEMENT *pst_link;
    PTR                   pst_audit;	/*
					** Pointer to audit information needed
					** by QEF for C2 security auditing.
					*/
    i4                    pst_stmt_mask;
#define PST_NONCNF        0x0001
}   PST_STATEMENT;

/*}
** Name: PST_PROCEDURE - The top structure for a procedure execution plan.
**
** Description:
**	This structure, and those below it, define a procedure.
**
** History:
**	27-apr-88 (stec)
**	    Created.
**	2-Oct-89 (teg)
**	    bumped PST_CUR_VSN to 4 because of Outer Join changes.
**	28-feb-91 (teresa)
**	    moved PST_CUR_VSN constant to PST_QTREE structure, where it
**	    belongs.
**	18-oct-2007 (dougi)
**	    Changes for repeat dynamic queries.
*/
typedef struct _PST_PROCEDURE
{
	/* Pst_mode nad pst_vsn have been inserted so that the first 4 bytes
	** are identical to old tree haeders. This is necessary for RDF to
	** be able to read trees from the system catalogs.
	*/
    i2		    pst_mode;		/* Unused */
    i2		    pst_vsn;		/* Header/tree vsn no.: 0, 1, ... 
					** see pst_vsn in struct PST_QTREE for
					** a list of possible values and their
					** descriptions */
	/* Pst_isproc is TRUE if this represents a database procedure. If
	** it is FALSE, then this represents a non-procedure (ie. dml) query
	** that equel, esql, the TM, abf, etc has sent us.
	*/
    bool 	    pst_isdbp;

    i4	    pst_flags;
#define	PST_SET_INPUT_PARAM	0x0001 /* procedure has a set-input parameter */
#define	PST_REPEAT_DYNAMIC	0x0002 /* parse tree for repeat dynamic query */

	/* The list of statements to execute. */
    PST_STATEMENT   *pst_stmts;

	/* The definition of the procedure parameters. If this isn't a real
	** procedure then this will be NULL.
	*/
    PST_DECVAR	    *pst_parms;

	/* Pst_dbpid is the full id given to this procedure.
	** OPF use this id as the object id for storing the procedure
	** execution plan in QSF memory.  We will return this id to
	** the FE so that they can use it to refer to this procedure in
	** the future.
	**
	** If this is a repeat dynamic query, pst_dbpid contains the syntax
	** checksum as object identifier.
    	*/
    DB_CURSOR_ID   pst_dbpid;
}   PST_PROCEDURE;

/*
** Name: PSQ_REL_MAP - map of relations used in a query
**
** Description: this structure will be used to contain a map of relations used
**		in a query.  At present (8/91) we are using i4 to represent
**		relation maps, but this should change some time soon, or
**		increasing PST_NUMVAR could become a very expensive proposition
**
**		The time has come to increase PST_NUMVARS and it is more 
**		tedious than expensive (this is just a parser, after all).
**
** History:
**	23-aug-91 (andre)
**	    written
**	25-nov-02 (inkdo01)
**	    Changed to support much bigger range table. In fact, nothing is 
**	    done here because this structure is only used as a parameter to 
**	    a function (psy_init_rel_map) that is never called.
*/
typedef struct _PSQ_REL_MAP
{
    i4	rel_map;
} PSQ_REL_MAP;

/*
** Mnemonics used in psl_idxlate() to control identifier translation
**
** History:
**	26-oct-92 (ralph)
**	    written
**	05-mar-93 (ralph)
**	    @FIX_ME@ THESE SHOULD BE REMOVED WHEN PSLID.C IS REMOVED
*/
# define	PSL_ID_DLM      0x0001	/* Treat the identifier as delimited */
# define        PSL_ID_DLM_M    0x0002	/* Don't xlate case of delimited ids */
# define        PSL_ID_DLM_U    0x0008	/* xlate delimited ids to upper case */
# define        PSL_ID_QUO      0x0010	/* Treat the identifier as quoted */
# define        PSL_ID_QUO_T    0x0040	/* Translate  case of quoted ids */
# define        PSL_ID_QUO_U    0x0080	/* xlate quoted ids to upper case */
# define        PSL_ID_REG      0x0100	/* Treat the identifier as regular */
# define        PSL_ID_REG_M    0x0200	/* Don't xlate case of regular ids */
# define        PSL_ID_REG_U    0x0800	/* xlate regular ids to upper case */
# define        PSL_ID_ANSI     0x1000	/* Check for special ANSI conditions */
# define        PSL_ID_NORM     0x2000	/* Treat identifier as if normalized */
# define        PSL_ID_STRIP    0x4000	/* Ignore trailing white space */
# define        PSL_ID_FIPS     (PSL_ID_DLM_M | PSL_ID_REG_U) /* FIPS   dflt */
# define        PSL_ID_INGRES   (PSL_ID_DLM_L | PSL_ID_REG_L) /* INGRES dflt */

/*
** prototype declarations for externally used PSF functions
*/

/*
** some of the functions expect PSS_SESBLK * but they do not include pshparse.h
** here we will define a typedef for struct _PSS_SESBLK * to avoid 
** "dubious tag declaration" messages from ACC
*/
typedef struct _PSS_SESBLK *PSF_SESSCB_PTR;

/*
** entry points
*/
FUNC_EXTERN DB_STATUS
psq_call(
	i4		   opcode,
	PSQ_CB		   *psq_cb,
	PSF_SESSCB_PTR	   sess_cb);

FUNC_EXTERN DB_STATUS
psy_call(
	i4		   opcode,
	PSY_CB		   *psy_cb,
	PSF_SESSCB_PTR	   sess_cb);

FUNC_EXTERN DB_STATUS
psf_debug(
	DB_DEBUG_CB	   *debug_cb);

/*
** these functions are not the "normal" entry points but are nonetheless used
** by OPF 
*/
FUNC_EXTERN DB_STATUS
pst_resolve(
	PSF_SESSCB_PTR     sess_cb,
	ADF_CB		   *adf_scb,
	PST_QNODE	   *opnode,
	DB_LANG		   lang,
	DB_ERROR	   *error);

FUNC_EXTERN VOID
pst_1ftype(
	PST_QNODE          *qnode,
	char               *buf);

FUNC_EXTERN VOID
pst_1fidop(
	PST_QNODE          *qnode,
	char               *buf);

FUNC_EXTERN DB_STATUS
pst_1prmdump(
	PST_QNODE          *tree,
	PST_QTREE	   *header,
	DB_ERROR	    *error);

FUNC_EXTERN DB_STATUS
pst_prmdump(
	PST_QNODE           *tree,
	PST_QTREE	    *header,
	DB_ERROR	    *error,
	i4		    facility);
