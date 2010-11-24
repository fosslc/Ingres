/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <me.h>
#include    <st.h>
#include    <qu.h>
#include    <bt.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <usererror.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psyaudit.h>
#include    <cui.h>

/**
**
**  Name: PSYDGRANT.C - Function used to define a grant of a privilege
**			on a database object.
**
**  Description:
**      This file contains functions necessary to define a grant of
**	a privilege on a table.
**
**          psy_dgrant		-   Grant a privilege.
**	    psy_permit_ok	-   Check if a user may grant permit on
**				    a table
**	    psy_dbp_priv_check	-   Reparse a dbproc owned by the current user
**				    to determine if he may grant permit on it
**	    psy_tbl_grant_check -   Determine if the user may grant specified
**				    privileges on a given table or view
**	    psy_prvmap_to_str	-   Convert a privilege map into a string
**	    psy_init_rel_map	-   Initialize a structure holding a map of
**				    relations
**	    psy_attmap_to_str	-   conmvert a map of attribute into an
**				    attribute list
**
**	This file also contains the following static functions:
**
**	    psy_tbl_to_col_priv -   translate table-wide privilege on a view
**				    into column-specific privilege on the
**				    underlying table
**	    psy_colpriv_xlate	-   translate column-specific privilege on a
**				    view into column-specific privilege on the
**				    underlying table
**	    psy_find_rgvar_ref	-   find reference to a given range variable
**				    in a tree
**	    psy_dbp_ev_seq_perm	-   complete the actions required to insert
**				    permits granting specified privilege on a
**				    dbevent or a dbproc to all grantees
**	    psy_mark_sublist	-   mark beginning of sublists of independent
**				    objects/privileges by inserting sublist
**				    headers into the lists
**	    psy_recursive_chain -   determine if a dbproc is being invoked
**				    recursively; this is required to avoid
**				    getting trapped in an infinite loop when
**				    processing dbprocs involved in recursive
**				    call chains + to enable us to correctly
**				    determine whether dbprocs involved in
**				    recursive call chains are grantable
**	    psy_dupl_dbp	-   determine if a descriptor for a given dbproc
**				    has already been created and if so, massage
**				    the tree to ensure that it never contains
**				    more than one descriptor for a given dbproc
**	    psy_prvtxt		-   build query text for a permit conveying a
**                        	    column-specific privilege
**	    psy_pr_cs_privs	-   print description of a column-specific
**                                  privilege
**
**  History:
**      30-apr-87 (stec)
**          written
**	05-aug-88 (andre)
**	    modified to handle granting permit on a database procedure
**	06-feb-89 (ralph)
**	    modified to use DB_COL_WORDS when resetting all dbp_domset bits
**	    modified to use DB_COL_BITS  when   setting all dbp_domset bits
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Copy grantee type from psy_gtype into dbp_gtype.
**	08-may-89 (ralph)
**	    Initialize reserved field to blanks (was \0)
**	04-jun-89 (ralph)
**	    Initialize dbp_fill1 to zero
**	    Corrected unix portability problems
**	01-nov-89 (ralph)
**	    Correct problem with CREATE SECURITY_ALARM:  If all/select
**	    and public specified, this was mistaken here as grant all/select
**	    to public.
**	02-nov-89 (neil)
**	    Alerters: Allow privileges for events.
**	06-feb-90 (andre)
**	    Added psy_permit_ok(), psy_dbp_grant_check(), and
**	    psy_view_grant_check().
**	18-jun-90 (andre)
**	    Unfix RDF cache entries in psy_view_grant_check().
**	08-aug-90 (ralph)
**	    Initialize new fields in iiprotect tuple
**	14-sep-90 (teresa)
**	    pss_catupd becomes bitflag in pss_ses_flag instead of a boolean.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	15-jun-92 (barbara)
**	    Changed interface to pst_sent; set rdr_r1_distrib flag.
**	28-oct-92 (robf)
**	    When a SECURITY ALARM is created on a user, handling (non)
**	    quoting of user name, also make sure when BY PUBLIC is
**          specified any IF/WHEN clauses are preserved appropriately.
**	06-nov-1992 (rog)
**	    Include <ulf.h> before <qsf.h>.
**	16-nov-1992 (pholman)
**	    C2: replace obsolete calls to qeu_audit with psy_secaudit 
**	    and include <sxf.h>.
**	18-mar-93 (rblumer)
**	    Made psy_check_privs non-static, changed grant_all parameter into
**	    a flags field, and added no-grant-option flag & no-error flag.
**	    All this is to allow the function to be used for checking
**	    REFERENCES privilege for referential constraints.
**	05-apr-93 (anitap)
**	    Added a check for CREATE SCHEMA. If an user issues a GRANT 
**	    statement for issuing permits on a bunch of tables and if he 
**	    doesn't have sufficient privilege on one of the tables, psy_dgrant()
**	    continues processing the permits for the next table in the list.
**	    For CREATE SCHEMA, we want to return an error and abort the 
**	    statement.  
**	03-apr-1993 (ralph)
**	    DELIM_IDENT: Use pss_cat_owner instead of "$ingres"
**	17-jun-93 (andre)
**	    undo Anita's change from (05-apr-93).  If a user cannot grant some
**	    privilege, we return a warning - not an error - thus CREATE SCHEMA
**	    should not be rolled back if one or more GRANTs inside of it fail
**	    because the usser lacks needed privileges
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
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
**	11-feb-94 (swm)
**	    Bug #59614
**	    My previous integration (bug #56448) erroneously introduced a
**	    block of code during integrate merging. This had the side-effect
**	    of failing the WGO runall test because US088E is issued instead
**	    of PS055[12] when a user specifies some privileges which they
**	    may not grant.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout(), psy_mark_sublist().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      13-Mar-2003 (hanal04) Bug 109786 INGSRV 2122
**          Prevent invalid privileges being set on VIEWs when a GRANT ALL
**          is issued.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	21-Oct-2010 (kiria01) b124629
**	    Use the macro symbol with ult_check_macro instead of literal.
[@history_template@]...
**/

/*
**  Definition of static variables and forward functions.
*/

static DB_STATUS
psy_tbl_to_col_priv(
	i4		    *tbl_wide_privs,
	i4		    *col_priv_map,
	PSY_ATTMAP	    *colpriv_attr_map,
	i4		    priv,
	PST_QNODE	    *tree,
	DB_ERROR	    *err_blk);
static DB_STATUS
psy_colpriv_xlate(
	PST_QNODE	    *tree,
	i4		    *col_priv_map,
	PSY_ATTMAP	    *colpriv_attr_map,
	i4		    priv,
	i4		    view_attr_count,
	DB_ERROR	    *err_blk);
static bool
psy_find_rgvar_ref(
	PST_QNODE       *t,
	i4             rgno);
static DB_STATUS
psy_mark_sublist(
	PSS_SESBLK		*sess_cb,
	PSQ_INDEP_OBJECTS	*indep_objs,
	PSF_MSTREAM		*mstream,
	i4			hdrtype,
	DB_TAB_ID		*dbpid,
	DB_DBP_NAME		*dbpname,
	DB_ERROR		*err_blk);
static DB_STATUS
psy_dbp_ev_seq_perm(
	RDF_CB		*rdf_cb,
	PSY_CB		*psy_cb,
	PSY_TBL		*psy_tbl,
	DB_PROTECTION	*protup,
	char		*buf,
	i4		buf_len);
static bool
psy_recursive_chain(
	PSS_DBPLIST_HEADER      *cur_node,
	register DB_TAB_ID	*dbpid);
static bool
psy_dupl_dbp(
	PSS_DBPLIST_HEADER  *dbp_list,
	PSQ_OBJ		    *obj,
	PSS_DBPLIST_HEADER  *cur_dbp,
	bool		    ps133);
static DB_STATUS
psy_prvtxt(
	    PSS_SESBLK		*sess_cb,
	    i4		priv,
	    PSY_ATTMAP		*attrib_map,
	    PTR			txt_chain,
	    DMT_ATT_ENTRY	**attdesc,
	    PSY_TBL		*psy_tbl,
	    i4			*grntee_type_offset,
	    i4			*grantee_offset,
	    char		*buf,
	    i4			buf_len,
	    bool		grant_option,
	    DB_TEXT_STRING	**txt,
	    PSF_MSTREAM		*mstream,
	    DB_ERROR		*err_blk);
static VOID
psy_pr_cs_privs(
		i4		priv,
		PSY_ATTMAP	*col_priv_attmap,
		DMT_ATT_ENTRY	**att_list);

/*{
** Name: psy_dgrant	- Execute GRANT.
**
**  INTERNAL PSF call format: status psy_dgrant(&psy_cb, sess_cb);
**
** Description:
**	Given all of the parameters necessary to define a grant on a data-
**	base object (table, dbevent, or a database procedure), psy_dgrant()
**	will store the permission in the system tables.
**	This will include storing the query tree in the tree table,
**	storing the text of the query in the iiqrytext table (really done by
**	the QSM), storing a row in the protect table, and issuing an "alter
**	table" operation to DMF to indicate that there are permissions on the
**	given object.
**	Note the processing for dbprocs & events is different from that needed
**	for tables, so each is done as an autonomous block inside psy_dgrant().
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_qrytext		Id of query text as stored in QSM.
**          .psy_opctl			Bit map of defined   operations
**          .psy_opmap                  Bit map of permitted operations
**          .psy_terminal               Terminal at which permission is given
**					(blank if none specified)
**          .psy_timbgn                 Time of day at which the permission
**					begins (minutes since 00:00)
**          .psy_timend                 Time of day at which the permission ends
**					(minutes since 00:00)
**          .psy_daybgn                 Day of week at which the permission
**					begins (0 = Sunday)
**          .psy_dayend                 Day of week at which the permission ends
**					(0 = Sunday)
**	    .psy_grant			PSY_TGRANT if GRANT on a table,
**					PSY_PGRANT if grant on a dbproc,
**					PSY_SQGRANT if grant on a sequence,
**					PSY_EVGRANT if grant on an event,
**					PSY_LGRANT if grant on a location (code
**					is yet to be written)
**	    .psy_gtype			Grantee type:
**					DBGR_USER if users (and/or PUBLIC) were
**					specified
**					DBGR_GROUP if groups were specified
**					DBGR_APLID if application ids were
**					specified
**	    .psy_tblq			head of table queue
**	    .psy_colq			head of column queue
**	    .psy_usrq			head of user (or applid/group) queue
**	    .psy_u_numcols		number of elements in the list of
**					attribute ids of columns to which UPDATE
**					privilege will/will not apply
**	    .psy_u_col_offset		offset into the list of attribute ids
**					associated with a given PSY_TBL entry of
**					the first element associated with UPDATE
**					privilege
**	    .psy_i_numcols		number of elements in the list of
**					attribute ids of columns to which INSERT
**					privilege will/will not apply
**	    .psy_i_col_offset		offset into the list of attribute ids
**					associated with a given PSY_TBL entry of
**					the first element associated with INSERT
**					privilege
**	    .psy_r_numcols		number of elements in the list of
**					attribute ids of columns to which
**					REFERENCES privilege will/will not apply
**	    .psy_r_col_offset		offset into the list of attribute ids
**					associated with a given PSY_TBL entry of
**					the first element associated with
**					REFERENCES privilege
**	    .psy_noncol_qlen		length of first iiqrytext
**	    .psy_flags
**	  PSY_EXCLUDE_UPDATE_COLUMNS	attribute ids associated with UPDATE
**					privilege indicate to which columns
**					UPDATE privilege will not apply
**	  PSY_EXCLUDE_INSERT_COLUMNS	attribute ids associated with INSERT
**					privilege indicate to which columns
**					INSERT privilege will not apply
**	PSY_EXCLUDE_REFERENCES_COLUMNS	attribute ids associated with REFERENCES
**					privilege indicate to which columns
**					REFERENCES privilege will not apply
**	PSY_PUBLIC			privilege(s) being granted to PUBLIC
**					(possibly, in addition to other users)
**	PSY_ALL_PRIVS			GRANT ALL [PRIVILEGES] was specified,
**					i.e. privileges being granted were not
**					named explicitly
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**          .psy_txtid                  Id of query text as stored in the
**					iiqrytext system relation.
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores text of query in iiqrytext relation, query tree in tree
**	    relation, row in protect relation identifying the permit.  Does
**	    an alter table DMF operation to indicate that there are permissions
**	    on the object.
**
** History:
**	29-apr-87 (stec)
**          written
**	05-aug-88 (andre)
**	    modified to handle granting permit on a database procedure.
**	06-feb-89 (ralph)
**	    modified to use DB_COL_WORDS when resetting all dbp_domset bits
**	    modified to use DB_COL_BITS  when   setting all dbp_domset bits
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Copy grantee type from psy_gtype into dbp_gtype.
**	08-may-89 (ralph)
**	    Initialize reserved field to blanks (was \0)
**	04-jun-89 (ralph)
**	    Initialize dbp_fill1 to zero
**	    Corrected unix portability problems
**	01-nov-89 (ralph)
**	    Correct problem with CREATE SECURITY_ALARM:  If all/select
**	    and public specified, this was mistaken here as grant all/select
**	    to public.
**	02-nov-89 (neil)
**	    Alerters: Allow privileges for events.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	08-aug-90 (ralph)
**	    Initialize new fields in iiprotect tuple
**	17-jun-91 (andre)
**	    A number of changes were made to beautify the
**	    GRANT/CREATE SECURITY_ALARM query text stored in IIQRYTEXT.  In
**	    particular, we will do away with blank padding for names of users,
**	    objects and columns.
**	24-jun-91 (andre)
**	    IIPROTECT tuples for table permits will contain exactly one
**	    privilege.  IIQRYTEXT template built for table-wide privileges
**	    contains a placeholder for a privilege name which will be filled in
**	    with each of the table-wide privileges being granted, one at a time.
**	    PSY_CB.psy_opmap will be set to correspond with privilege name
**	    stored in the IIQRYTEXT permit.
**	25-jun-91 (andre)
**	    we will start storing IIPROTECT and IIQRYTEXT tuples for
**	    ALL/RETRIEVE_TO_ALL permits on tables.
**	    ALL_TO_ALL will result in four tuples while RETRIEVE_TO_ALL will
**	    result in one tuple.  In both cases, we will continue setting the
**	    ALL_TO_ALL and RETRIEVE_TO_ALL bits in IIRELATION.
**	17-jul-91 (andre)
**	    If a permit specifies more than one privilege (possibly WGO), set
**	    RDF_SPLIT_PERM in rdr_instr to instruct qeu_cprot() to create
**	    multiple permit tuples.
**	06-aug-91 (andre)
**	    The following now applies to GRANT on tables and views:
**		unless the object on which a permit is being granted is a base
**		table owned by the current user (can grant any permit) or this
**		is an object to which a kludge described below applies, we will
**		verify that the user can, indeed, grant privileges on the given
**		object.
**		KLUDGE: for the time being we allow a user with CATUPD to grant
**			permits on catalogs and a user who is a DBA to grant
**			permits on extended catalogs.
**	25-sep-91 (andre)
**	    when processing a grant on a dbproc owned by another user, we need
**	    to open a memory stream since pst_dbpshow() will try to allocate a
**	    PSS_DBPINFO structure.
**	30-sep-91 (andre)
**	    when processing a grant on a dbproc owned by the current user, we
**	    need to verify that the dbproc is grantable.  Depending on the
**	    information found in IIPRIV and IIPROCEDURE this may be
**	    accomplished as follows:
**		- (once (if) we start storing in IIPROCEDURE an indicator of
**		  whether the dbproc is grantable,) if IIPROCEDURE indicates
**		  that the dbproc is grantable, we are done
**		- else (once we start storing a list of independent objects for
**		  a dbproc in IIPRIV) if IIPRIV contains a list of independent
**		  objects for this dbproc, if the user can grant access on all
**		  of dbproc's independent objects, we are done
**		else reparse the dbproc to determine if it is grantable.
**	    As of now (9/30/91) the only option open to us is to reparse the
**	    dbproc, but this is likely (no, no, it IS GOING) to change soon.
**	07-oct-91 (andre)
**	    if a dbproc has been parsed successfully, insert a
**	    PSQ_DBPLIST_HEADER elements into the lists of independent objects
**	    and privileges; otherwise insert a PSQ_INFOLIST_HEADER element to
**	    indicate that the following sublist does not describe independent
**	    objects for a dbproc but may contain useful information.
**	10-oct-91 (andre)
**	    Two more changes for processing of GRANT ON PROCEDURE:
**		- until now we have not been checking if the dbprocs owned by
**		  the current user which were used inside the dbprocs which were
**		  reparsed were themselves grantable;  descriptions of these
**		  dbprocs will be removed from the independent object list and
**		  moved (temporarily) into the internal list of dbprocs
**		  grantability of which must be verified;
**		- once it is known that a dbproc is (or is not) grantable, an
**		  entry will be placed into the privilege list to avoid trying
**		  to rediscover this information again; if the dbproc was
**		  mentioned in the GRANT statement, a corresponding privilege
**		  entry will be placed into the special INFOLIST at the end of
**		  the privilege list, while for dbprocs whose descriptors were
**		  removed from the independent object list for some dbproc P,
**		  will be inserted into the independent privilege list for P
**		  (I am going to think more about this last point, since if we
**		  decide to store the dependency information immediately after
**		  successful reparsing of a dbproc, nothing will be gained by
**		  storing the privilege descriptor into the corresponding
**		  privilege sublist, instead we may choose to store it into the
**		  INFOLIST sublist mentioned above.)
**	28-feb-92 (andre)
**	    permits being created on dbevents will be handled separately from
**	    those being created on dbprocs because
**		- we will be splitting dbevent perms specifying more than one
**		  privilege and
**		- we will verify here that the user may, indeed, grant the
**		  specified privilege(s) (we no longer insist that only the
**		  owner of the event may grant privileges on it.)
**	07-jul-92 (andre)
**	    if granting one or more of DELETE/INSERT/UPDATE on a view and
**	    psy_tbl_grant_check() indicates that privileges being granted depend
**	    on privileges on an underlying table or view, populate structures
**	    representing independent privilege list and pass them along to
**	    qeu_cperm().
**	07-jul-92 (andre)
**	    DB_PROTECTION tuple will contain an indicator of how the permit was
**	    created, i.e. whether it was created using SQL or QUEL and if the
**	    former, then whether it was created using GRANT statement.  Having
**	    this information will facilitate merging similar and identical
**	    permit tuples.
**	14-jul-92 (andre)
**	    semantics of GRANT ALL [PRIVILEGES] is different from that of
**	    CREATE PERMIT ALL in that the former (as dictated by SQL92) means
**	    "grant all privileges which the current auth id posesses WGO"
**	    whereas the latter (as is presently interpreted) means "grant all
**	    privileges that can be defined on the object" which in case of
**	    tables and views means SELECT, INSERT, DELETE, UPDATE.
**	    psy_tbl_grant_check() (function responsible for determining whether
**	    a user may grant specified privilege on a specified table or view)
**	    will have to be notified whether we are processing GRANT ALL.  Its
**	    behaviour will change as follows:
**	      - if processing GRANT ALL and psy_tbl_grant_check() determines
**	        that the user does not possess some (but not all) of the
**		privileges passed to it by the caller it will not treat it as an
**		error, but will instead inform the caller of privileges that the
**		user does not posess,
**	      - if processing GRANT ALL and psy_tbl_grant_check() determines
**	        that the user does not possess any of the privileges passed to
**		it by the caller it will treat it as an error
**	      - if processing a statement other than GRANT ALL and
**	        psy_tbl_grant_check() determines that the user does not possess
**		some of the privileges passed to it by the caller it will treat
**		it as an error
**	15-jul-92 (andre)
**	    if user specified GRANT ALL [PRIVILEGES], psy_tbl_grant_check() may
**	    reduce the set of privileges being granted.  we will pass 
**	    psy_tbl_grant_check() the address of tbl_wide_privs instead of
**	    psy_tbl_grant_check itself and use the value returned to us as the
**	    map of all privileges which we will grant, i.e. under some
**	    circumstances psy_opmap may contain more privileges than we are
**	    going to grant on a given table
**	18-jul-92 (andre)
**	    we will no longer tell qeu_cprot() (through rdf_update()) that a
**	    user is creating ALL/SELECT TO PUBLIC.  qeu_scan_perms() (a fucntion
**	    invoked by qeu_cprot()) will determine whether the combination of
**	    existing permits and the permits being created give PUBLIC a SELECT
**	    (DMT_RETRIEVE_PRO) or SELECT, INSERT, DELETE, UPDATE (DMT_ALL_PROT).
**	19-jul-92 (andre)
**	    if user specified PUBLIC as a grantee (or in CREATE SECURITY_ALARM
**	    statement), psy_gtype will no longer be set to DBGR_PUBLIC.
**	    Instead, PSY_CB.psy_flags will have PSY_PUBLIC set
**	21-jul-92 (andre)
**	    make changes to support EXCLUDING clause in GRANT statement
**	13-aug-92 (andre)
**	    a further refinement of GRANT ALL [PRIVILEGES] processing: if a user
**	    can only GRANT UPDATE on some columns of specified table or view,
**	    he will be allowed to do so.
**	15-aug-92 (barbara)
**	    For table grants, invalidate base object's infoblk from the RDF
**	    cache.
**	16-sep-92 (andre)
**	    privilege map gets built using bitwise operators, so care should be
**	    exercised when accessing privilege maps using BTnext(), BTset(),
**	    BTclear() and the like.  The safest bet is probably to NOT DO IT.
**	28-oct-92 (robf)
**	    When a SECURITY ALARM is created on a user, handle (non)quoting
**	    of user name 
**	07-dec-92 (andre)
**	    added support for GRANT REFERENCES
**	11-dec-92 (andre)
**	    A number of PSY_CB elements have been renamed to improve readability
**	    of code.
**	05-apr-93 (anitap)
**	    Added a check for CREATE SCHEMA. If an user issues a GRANT 
**	    statement for issuing permits on a bunch of tables and if he 
**	    doesn't have sufficient privilege on one of the tables, psy_dgrant()
**	    continues processing the permits for the next table in the list.
**	    For CREATE SCHEMA, we want to return an error and abort the 
**	    statement.  
**	17-jun-93 (andre)
**	    undo Anita's change from (05-apr-93).  If a user cannot grant some
**	    privilege, we return a warning - not an error - thus CREATE SCHEMA
**	    should not be rolled back if one or more GRANTs inside of it fail
**	    because the usser lacks needed privileges
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	5-jul-93 (robf)
**	    Pass security label to psy_secaudit
**	17-aug-93 (andre)
**	    identifier placeholders are now of length DB_MAX_DELIMID.  
**	    Names of schemas, objects and grantees will be expressed using 
**	    regular identifier if that's how they were expressed by the user - 
** 	    otherwise they will be expressed using delimited identifiers
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
**	24-nov-93 (robf)
**         Removed xORANGE block, now handled consistently in B1/C2 modes
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	28-jan-94 (andre)
**	    set object type (T or V) in IIPROTECT tuples describing 
**	    security_alarms
**	10-may-02 (inkdo01)
**	    Add code for sequence privileges.
**	24-feb-03 (inkdo01)
**	    Add mask parm to psy_gsequence call.
**      13-Mar-2003 (hanal04) Bug 109786 INGSRV 2122
**          A GRANT ALL will set DB_COPY_INTO and DB_COPY_OUT privileges
**          but these are not valid for a VIEW. Ensure these flags are 
**          turn off in  tbl_wide_privs if the object is a VIEW.
**	11-nov-2004 (gupsh01)
**	    If grant all privilages is added on a view then fixed the flags
**	    added to remove the COPY_INTO and COPY_FROM privileges on a view.
**	16-nov-2004 (gupsh01)
**	    Rollback previous change to remove COPY_INTO and COPY_FROM.
*/
DB_STATUS
psy_dgrant(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    RDF_CB		rdf_inv_cb;	/* Separate RDF_CB for invalidate */
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    QSF_RCB		qsf_rb1, *txt_qsf_rb_p = &qsf_rb1;
    DB_STATUS		status;
    DB_STATUS		stat;
    DB_PROTECTION	ptuple;
    PSQ_INDEP_OBJECTS   indep_objs;
    register DB_PROTECTION *protup = &ptuple;
    i4			*domset	= ptuple.dbp_domset;
    register i4	i;
    i4		err_code;
    i4			textlen;
    i4			text_lock = 0;
    PSY_USR		*psy_usr;
    PSY_TBL		*psy_tbl;
    bool		updtcols, insrtcols, refcols;
    i4			colpriv_map;
    bool		noncol;
    DB_TIME_ID          timeid;
    char		buf[80];
    i4			buf_text_len;
    bool		grant_to_public;
    char		*grntee_type_start;
    bool		split_perm;
    char		*ch;
	/*
	** this var will contain a map of table/view privileges to grant; if
	** GRANT ALL [PRIVILEGES] was specified, it may be a subset of
	** psy_cb->psy_opmap; otherwise it will be identical to
	** psy_cb->psy_opmap
	*/
    i4			privs_to_grant;
    i4			grant_all = psy_cb->psy_flags & PSY_ALL_PRIVS;
    char		trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    /* This code is called for SQL only */

    /*
    ** Initialize the QSF control block used to obtain the IIQRYTEXT template(s)
    ** This code must be executed before the first branch to exit since
    ** otherwise qsf_call() will get quite frazzled by being presented with an
    ** uninitialized QSF_RCB
    */
    txt_qsf_rb_p->qsf_type	= QSFRB_CB;
    txt_qsf_rb_p->qsf_ascii_id	= QSFRB_ASCII_ID;
    txt_qsf_rb_p->qsf_length	= sizeof(QSF_RCB);
    txt_qsf_rb_p->qsf_owner	= (PTR)DB_PSF_ID;
    txt_qsf_rb_p->qsf_sid	= sess_cb->pss_sessid;

    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, txt_qsf_rb_p->qsf_obj_id);

    /* 
    ** if processing GRANT ALL on tables, we may need to build temporary 
    ** templates - initilaize the text chain and the text memory stream 
    ** structure to indicate that they have not been used
    */
    if (grant_all && psy_cb->psy_grant == PSY_TGRANT)
    {
	sess_cb->pss_tchain = (PTR) NULL;
	sess_cb->pss_tstream.psf_mstream.qso_handle = (PTR) NULL;
    }

    /* get a lock on the QSF object containing IIQRYTEXT template(s) */
    txt_qsf_rb_p->qsf_lk_state = QSO_EXLOCK;
    status = qsf_call(QSO_LOCK, txt_qsf_rb_p);
    if (DB_FAILURE_MACRO(status))
    {
	_VOID_ psf_error(E_PS0D19_QSF_INFO, txt_qsf_rb_p->qsf_error.err_code,
	    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	goto exit;
    }

    /*
    ** remember that we have a lock on the QSF object containing IIQRYTEXT
    ** template(s)
    */
    text_lock = txt_qsf_rb_p->qsf_lk_id;

    /* Initialize the template tuple */

    MEfill(sizeof(ptuple), (u_char) 0, (PTR) protup);
    protup->dbp_gtype = psy_cb->psy_gtype;	  /* Copy in grantee type */
    MEfill(sizeof(protup->dbp_reserve),  /* Init reserve  */
		 (u_char)' ',
		 (PTR)protup->dbp_reserve);
    protup->dbp_pdbgn = psy_cb->psy_timbgn;   /* Begin, end times of day */
    protup->dbp_pdend = psy_cb->psy_timend;
    protup->dbp_pwbgn = psy_cb->psy_daybgn;   /* Begin, end days of week */
    protup->dbp_pwend = psy_cb->psy_dayend;
    STRUCT_ASSIGN_MACRO(psy_cb->psy_terminal, protup->dbp_term);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, protup->dbp_grantor);
    TMnow((SYSTIME *) &timeid);
    protup->dbp_timestamp.db_tim_high_time = timeid.db_tim_high_time;
    protup->dbp_timestamp.db_tim_low_time  = timeid.db_tim_low_time;

    /*
    ** store masks indicating that the permit is being created using SQL
    ** GRANT statement
    */
    protup->dbp_flags = DBP_SQL_PERM | DBP_GRANT_PERM | DBP_65_PLUS_PERM;

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    pst_rdfcb_init(&rdf_inv_cb, sess_cb);

    rdf_rb->rdr_update_op   = RDR_APPEND;		/* permit definition */
    rdf_rb->rdr_status	    = DB_SQL;

    /* initialize independent object structure */
    indep_objs.psq_objs	= (PSQ_OBJ *) NULL;
    indep_objs.psq_objprivs = (PSQ_OBJPRIV *) NULL;
    indep_objs.psq_colprivs = (PSQ_COLPRIV *) NULL;
    indep_objs.psq_grantee  = &sess_cb->pss_user;

    rdf_rb->rdr_indep	    = (PTR) &indep_objs;
    rdf_rb->rdr_qrytuple    = (PTR) protup;

    /*
    ** set some fields common to processing of privileges on dbprocs
    ** dbevents and sequences.
    */

    if (psy_cb->psy_grant == PSY_EVGRANT || psy_cb->psy_grant == PSY_PGRANT ||
	psy_cb->psy_grant == PSY_SQGRANT)
    {
	protup->dbp_obstat = ' ';		    /*@FIX_ME@ 
						    ** Where do I get this?
						    */

	/*
	** Give RDF a pointer to the query text to be stored in iiqrytext.
	*/
	MEcopy((char *) txt_qsf_rb_p->qsf_root, sizeof(i4),
	    (char *) &textlen);

	rdf_rb->rdr_querytext = ((char *) txt_qsf_rb_p->qsf_root) + sizeof(i4);
	protup->dbp_popset = psy_cb->psy_opmap;
	protup->dbp_popctl = psy_cb->psy_opctl;

	/*
	** store text which must appear between the end of object name and
	** beginning of grantee name (if any) into the local buffer as it is
	** likely to be overwritten (we must account for schema.object)
	*/
	buf_text_len = psy_cb->psy_noncol_grantee_off -
	    psy_cb->psy_noncol_obj_name_off - 
		2 * DB_MAX_DELIMID - CMbytecnt(".");

	MEcopy(
	    (char *) rdf_rb->rdr_querytext + psy_cb->psy_noncol_obj_name_off + 
		2 * DB_MAX_DELIMID + CMbytecnt("."),
	    buf_text_len, buf);

    }

    /* granting privileges on sequence(s)? */
    if (psy_cb->psy_grant == PSY_SQGRANT)
    {
	i4	    seq_privs;

	rdf_rb->rdr_types_mask = RDR_PROTECT;
	rdf_rb->rdr_2types_mask = RDR2_SEQUENCE;
	protup->dbp_obtype = DBOB_SEQUENCE;

	/*
	** IIPROTECT tuples representing permits will contain exactly one
	** privilege.  If permit specified more than one sequence privilege,
	** notify QEF that it will have to split the permit into multiple
	** IIPROTECT tuples
	*/
	seq_privs = psy_cb->psy_opmap & ~DB_GRANT_OPTION;

	if (BTcount((char *) &seq_privs, BITS_IN(seq_privs)) > 1)
	{
	    rdf_rb->rdr_instr |= RDF_SPLIT_PERM;
	}
	
	/*
	** Call RDF to store GRANTed privileges for each sequence.
	*/
	for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	     psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	     psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	    )
	{
	    /*
	    ** verify that the user may grant specified privileges on this
	    ** sequence, i.e. if he posesses these privileges WGO
	    ** 
	    ** owner of the sequence implicitly posesses all required privileges
	    */
	    if (MEcmp((PTR) &sess_cb->pss_user, (PTR) &psy_tbl->psy_owner,
		    sizeof(sess_cb->pss_user)))
	    {
		PSS_SEQINFO	    seq_info;
		i4		    gseq_flags;

		seq_privs = psy_cb->psy_opmap | DB_GRANT_OPTION;

		status = psy_gsequence(sess_cb, &psy_tbl->psy_owner,
		    (DB_NAME *)&psy_tbl->psy_tabnm, PSS_SEQ_BY_OWNER,
		    &seq_info, (DB_IISEQUENCE *) NULL, &gseq_flags, 
		    &seq_privs, (i4)PSQ_GRANT, grant_all, &psy_cb->psy_error);

		if (DB_FAILURE_MACRO(status))
		    goto exit;
		else if (gseq_flags &
		         (PSS_MISSING_EVENT | PSS_INSUF_EV_PRIVS))
		{
		    /*
		    ** sequence could not be found or the user may not grant
		    ** specified priivlege(s) on it - proceed to the next one
		    */
		    continue;
		}

		/*
		** if user specified GRANT ALL, it may be that he/she may grant
		** some of the legal privileges, so we will need to set
		** protup->dbp_popset and protup->dbp_popctl accordingly
		*/
		if (grant_all)
		{
		    protup->dbp_popset = psy_cb->psy_opmap & seq_privs;
		    protup->dbp_popctl =
			psy_cb->psy_opctl & ~(psy_cb->psy_opmap ^ seq_privs);
		}
	    }

	    status = psy_dbp_ev_seq_perm(&rdf_cb, psy_cb, psy_tbl, protup, buf,
		buf_text_len);
	    if (DB_FAILURE_MACRO(status))
		goto exit;
	}	/* for all sequences */

	goto exit;    /* skip the part dealing with tables */
    }

    /* granting privileges on dbevent(s)? */
    if (psy_cb->psy_grant == PSY_EVGRANT)
    {
	i4	    ev_privs;

	rdf_rb->rdr_types_mask = RDR_PROTECT | RDR_EVENT;
	protup->dbp_obtype = DBOB_EVENT;

	/*
	** IIPROTECT tuples representing permits will contain exactly one
	** privilege.  If permit specified more than one dbevent privilege,
	** notify QEF that it will have to split the permit into multiple
	** IIPROTECT tuples
	*/
	ev_privs = psy_cb->psy_opmap & ~DB_GRANT_OPTION;

	if (BTcount((char *) &ev_privs, BITS_IN(ev_privs)) > 1)
	{
	    rdf_rb->rdr_instr |= RDF_SPLIT_PERM;
	}
	
	/*
	** Call RDF to store GRANTed privileges for each dbevent.
	*/
	for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	     psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	     psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	    )
	{
	    /*
	    ** verify that the user may grant specified privileges on this
	    ** dbevent, i.e. if he posesses these privileges WGO
	    ** 
	    ** owner of the dbevent implicitly posesses all required privileges
	    */
	    if (MEcmp((PTR) &sess_cb->pss_user, (PTR) &psy_tbl->psy_owner,
		    sizeof(sess_cb->pss_user)))
	    {
		PSS_EVINFO	    ev_info;
		i4		    gevent_flags;

		ev_privs = psy_cb->psy_opmap | DB_GRANT_OPTION;

		status = psy_gevent(sess_cb, &psy_tbl->psy_owner,
		    &psy_tbl->psy_tabnm, (DB_TAB_ID *) NULL, PSS_EV_BY_OWNER,
		    &ev_info, &gevent_flags, &ev_privs, (i4) PSQ_GRANT,
		    grant_all, &psy_cb->psy_error);

		if (DB_FAILURE_MACRO(status))
		    goto exit;
		else if (gevent_flags &
		         (PSS_MISSING_EVENT | PSS_INSUF_EV_PRIVS))
		{
		    /*
		    ** dbevent could not be found or the user may not grant
		    ** specified priivlege(s) on it - proceed to the next one
		    */
		    continue;
		}

		/*
		** if user specified GRANT ALL, it may be that he/she may grant
		** some of the legal privileges, so we will need to set
		** protup->dbp_popset and protup->dbp_popctl accordingly
		*/
		if (grant_all)
		{
		    protup->dbp_popset = psy_cb->psy_opmap & ev_privs;
		    protup->dbp_popctl =
			psy_cb->psy_opctl & ~(psy_cb->psy_opmap ^ ev_privs);
		}
	    }

	    status = psy_dbp_ev_seq_perm(&rdf_cb, psy_cb, psy_tbl, protup, buf,
		buf_text_len);
	    if (DB_FAILURE_MACRO(status))
		goto exit;
	}	/* for all dbevents */

	goto exit;    /* skip processing of permits/secirity_alarms on tables */
    }

    /* granting privileges on dbprocs? */
    else if (psy_cb->psy_grant == PSY_PGRANT)
    {
	/* will be set to TRUE if tracepoint ps133 has been set */
	bool		ps133;	    
	/*
	** an error in reparsing a dbproc usually results in cache getting
	** flushed and then we try to reparse the dbproc in hope that the error
	** was caused by some stale cache entries;
	** if processing GRANT ON PROCEDURE, we may end up reparsing multiple
	** dbprocs, so we will try to avoid flushing the cache more than once
	*/
	char	    buff[sizeof("EXECUTE WITH GRANT OPTION")];

	{
	    i4		val1, val2;

	    /* determine if trace point ps133 has been set */
	    ps133 = (bool) ult_check_macro(&sess_cb->pss_trace,
			PSS_DBPROC_GRNTBL_ACTIVE_TRACE, &val1, &val2);
	    if (ps133)
	    {
		/* get a string representing privileges being granted */
		psy_prvmap_to_str(psy_cb->psy_opmap, buff, DB_SQL);
	    }
	}

	rdf_rb->rdr_types_mask = RDR_PROTECT | RDR_PROCEDURE;
	protup->dbp_obtype = DBOB_DBPROC;

	sess_cb->pss_ostream.psf_mstream.qso_handle =
	    sess_cb->pss_tstream.psf_mstream.qso_handle =
		sess_cb->pss_cbstream.psf_mstream.qso_handle = (PTR) NULL;

	/*
	** Call RDF to store GRANTed privileges for each dbproc.
	*/
	for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	     psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	     psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	    )
	{
	    if (ps133)
	    {
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, "<<<\n\
Attempting to grant %t on database procedure \n'%t' owned by '%t'\n>>>\n---",
		    STlength(buff), buff,
		    psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &psy_tbl->psy_tabnm), &psy_tbl->psy_tabnm,
		    psf_trmwhite(sizeof(DB_OWN_NAME),
			(char *) &psy_tbl->psy_owner), &psy_tbl->psy_owner);
	    }

	    if (MEcmp((PTR) &sess_cb->pss_user,
		(PTR) &psy_tbl->psy_owner, sizeof(sess_cb->pss_user)))
	    {
		PSS_DBPINFO	*dbpinfop;
		i4		dbpshow_flags;
		PSQ_CB		psq_cb;

		MEfill(sizeof(PSQ_CB), (u_char) '\0', (PTR) &psq_cb);

		psq_cb.psq_qlang = DB_SQL;
		psq_cb.psq_mode = PSQ_GRANT;
		psq_cb.psq_error.err_code = E_PS0000_OK;

		sess_cb->pss_dbp_flags = 0L;

		if (ps133)
		{
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"<<<\n\
Current user must posess EXECUTE WITH GRANT OPTION on database procedure\n\
'%t' owned by '%t'\n>>>\n---",
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    (char *) &psy_tbl->psy_tabnm), &psy_tbl->psy_tabnm,
			psf_trmwhite(sizeof(DB_OWN_NAME),
			    (char *) &psy_tbl->psy_owner), &psy_tbl->psy_owner);
		}

		/*
		** first we must open sess_cb->pss_ostream since pst_dbpshow()
		** will try to allocate memory from it for a dbproc descriptor
		** stream will be closed upon return from pst_dbpshow() since we
		** already have all the dbpinfo we need
		*/

		status = psf_mopen(sess_cb, QSO_QTREE_OBJ, &sess_cb->pss_ostream,
		    &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(status))
		    goto exit;

		status = pst_dbpshow(sess_cb,
		    (DB_DBP_NAME *) &psy_tbl->psy_tabnm, &dbpinfop,
		    &psy_tbl->psy_owner, (DB_TAB_ID *) NULL,
		    PSS_DBP_BY_OWNER | PSS_DBP_PERM_AND_AUDIT,
		    &psq_cb, &dbpshow_flags);

		if (DB_FAILURE_MACRO(status))
		{
		    STRUCT_ASSIGN_MACRO(psq_cb.psq_error, psy_cb->psy_error);
		}

		stat = psf_mclose(sess_cb, &sess_cb->pss_ostream, &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(stat) && stat > status)
		    status = stat;

		if (DB_FAILURE_MACRO(status))
		{
		    goto exit;
		}
		else if (dbpshow_flags &
			 (PSS_INSUF_DBP_PRIVS | PSS_MISSING_DBPROC))
		{
		    /*
		    ** dbproc could not be found (very strange, but could happen
		    ** with prepared GRANT) or the user lacks the required
		    ** privileges on the dbproc, copy the error block and
		    ** proceed on to the next entry
		    */

		    STRUCT_ASSIGN_MACRO(psq_cb.psq_error, psy_cb->psy_error);

		    continue;
		}
		else
		{
		    psy_tbl->psy_mask |= PSY_DBPROC_IS_GRANTABLE;
		}
	    }
	    else
	    {
		i4			    dbp_mask[2];

		status = psy_dbp_status(psy_tbl, sess_cb, &psy_cb->psy_tblq,
		    (i4) PSQ_GRANT, dbp_mask, &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(status))
		{
		    goto exit;
		}
		else if (~dbp_mask[0] & DB_DBPGRANT_OK)
		{
		    /* dbproc is not grantable - skip to the enxt entry */
		    continue;
		}
	    }	    /* granting permit on the current user's dbproc */

	    status = psy_dbp_ev_seq_perm(&rdf_cb, psy_cb, psy_tbl, protup, buf,
		buf_text_len);
	    if (DB_FAILURE_MACRO(status))
		goto exit;
	} /* for all dbprocs */

	goto exit;	/* skip the part dealing with tables */

    } /* if psy_grant == PSY_PGRANT */

    /*
    ** Let's establish if there are column-specific and table-wide privileges.
    */
    colpriv_map = 0;
    if (updtcols = psy_cb->psy_u_numcols != 0)
	colpriv_map |= (i4) DB_REPLACE;
	
    if (insrtcols = psy_cb->psy_i_numcols != 0)
	colpriv_map |= (i4) DB_APPEND;

    if (refcols = psy_cb->psy_r_numcols != 0)
	colpriv_map |= (i4) DB_REFERENCES;

    noncol = (psy_cb->psy_noncol_qlen != 0);

    /*
    ** IIPROTECT tuples representing permits will contain exactly one privilege.
    ** if permit specified more than one table-wide privilege, notify QEF that
    ** it will have to split the permit into multiple IIPROTECT tuples; if a
    ** permit involves any column-specific privileges, they will be factored
    ** into separate tuple(s)
    ** Tuples repersenting alarms do not fall under this requirement
    */
    if (psy_cb->psy_opmap & DB_ALARM || !noncol)
    {
	split_perm = FALSE;
    }
    else
    {
	/*
	** do not consider column-specific privileges in determining if the
	** table-wide permits will need to be split;
	*/

	u_i4	tbl_opmap =
	    psy_cb->psy_opmap & ~((i4) DB_GRANT_OPTION | colpriv_map);

	split_perm = (BTcount((char *) &tbl_opmap, BITS_IN(tbl_opmap)) > 1);
    }

    rdf_rb->rdr_types_mask = RDR_PROTECT;

    /*
    ** Give RDF a pointer to the query text to be stored in iiqrytext.
    ** Remember that the text segment retrieved from QSF may contain up to three
    ** templates (for table-wide text, UPDATE(<column list>) and 
    ** REFERENCES(<column list>)).  The length of the first (if present) is 
    ** given in psy_noncol_qlen and that of the second - in psy_updt_qlen, and
    ** of the third - in psy_ref_qlen.  
    */

    MEcopy((char *) txt_qsf_rb_p->qsf_root, sizeof(i4), (char *) &textlen);

    /*
    ** store text which must appear between the end of object name and
    ** beginning of grantee name (if any) into the local buffer as it is
    ** likely to be overwritten.  For CREATE SECURITY_ALARM we need to account
    ** for object name, for GRANT - for schema.object
    */
    buf_text_len =
	psy_cb->psy_noncol_grantee_off - psy_cb->psy_noncol_obj_name_off -
	((psy_cb->psy_opmap & DB_ALARM)
	    ? DB_MAX_DELIMID 
	    : 2 * DB_MAX_DELIMID + CMbytecnt("."));

    if (noncol)
    {
	MEcopy((char *) txt_qsf_rb_p->qsf_root +
	  sizeof(i4) + psy_cb->psy_noncol_obj_name_off +
	      ((psy_cb->psy_opmap & DB_ALARM)
	    ? DB_MAX_DELIMID 
	    : 2 * DB_MAX_DELIMID + CMbytecnt(".")),
	   buf_text_len, buf);
    }
    else if (updtcols)
    {
	MEcopy((char *) txt_qsf_rb_p->qsf_root +
		sizeof(i4) + psy_cb->psy_noncol_qlen +
		    psy_cb->psy_updt_obj_name_off +
		    2 * DB_MAX_DELIMID + CMbytecnt("."),
	    buf_text_len, buf);
    }
    else if (refcols)
    {
	MEcopy((char *) txt_qsf_rb_p->qsf_root +
		sizeof(i4) + psy_cb->psy_noncol_qlen + psy_cb->psy_updt_qlen + 
		psy_cb->psy_ref_obj_name_off + 2 * DB_MAX_DELIMID +
		CMbytecnt("."),
	    buf_text_len, buf);
    }

    /*
    ** if qeu_cprot() will not be splitting the permit into multiple
    ** tuples and RETRIEVE is one of the privileges mentioned in it, set
    ** the two bits associated with DB_RETRIEVE
    */
    if ((psy_cb->psy_opmap & DB_ALARM || !split_perm)
	&&
	psy_cb->psy_opmap & DB_RETRIEVE
       )
    {
	psy_cb->psy_opmap |= DB_TEST | DB_AGGREGATE;
	psy_cb->psy_opctl |= DB_TEST | DB_AGGREGATE;
    }

    if (split_perm)
    {
	rdf_rb->rdr_instr |= RDF_SPLIT_PERM;
    }

    /*
    ** if privileges were specified explicitly (and NOT as ALL [PRIVILEGES]) and
    ** only table-wide privileges were specified, we can copy the privilege map
    ** into IIPROTECT tuple now since it will not have to change
    */
    if (!colpriv_map && !grant_all)
    {
	protup->dbp_popset = psy_cb->psy_opmap;
	protup->dbp_popctl = psy_cb->psy_opctl;
    }

    /*
    ** if the user specified GRANT ALL [PRIVILEGES], privs_to_grant may be reset
    ** to be a subset of psy_cb->psy_opmap; otherwise they will be identical
    */
    privs_to_grant = psy_cb->psy_opmap;

    /*
    ** Call RDF to store GRANTed privileges for each table.
    */
    for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	)
    {
	i4		    text_len_without_grantee;
	char		    *grantee_start;
	PSY_COL_PRIVS       col_specific_privs;
	i4		    u_grntee_type_offset, r_grntee_type_offset;
	i4		    u_grantee_offset, r_grantee_offset;

        i4		    obj_name_len, grantee_name_len, schema_name_len;
	u_char              delim_schema_name[DB_MAX_DELIMID];
	u_char              delim_obj_name[DB_MAX_DELIMID];
	u_char              delim_grantee_name[DB_MAX_DELIMID];
	u_char              *schema_name, *obj_name, *grantee_name;

	DB_TEXT_STRING	    *u_txt, *r_txt;
	PSQ_OBJPRIV	    obj_priv;	    /* space for independent DELETE */
	PSQ_COLPRIV	    col_privs[2];   /*
					    ** space for independent INSERT and
					    ** UPDATE
					    ** Note that we will never have 
					    ** independent REFERENCES here 
					    ** because REFERENCES may only be 
					    ** specified on base tables
					    */

        /* 
        ** determine whether the object name will be represented by a regular or
        ** delimited identifier, compute that identifier and its length 
        **
        ** object name will be represented by a regular identifier if the user 
	** has specified it using a regular identifier - otherwise it will be 
        ** represented by a delimited identifier
        */
        obj_name = (u_char *) &psy_tbl->psy_tabnm;
        obj_name_len = 
	    (i4) psf_trmwhite(sizeof(psy_tbl->psy_tabnm), (char *) obj_name);
        if (~psy_tbl->psy_mask & PSY_REGID_OBJ_NAME)
        {
	    status = psl_norm_id_2_delim_id(&obj_name, &obj_name_len,
	        delim_obj_name, &psy_cb->psy_error);
	    if (DB_FAILURE_MACRO(status))
	        goto exit;
        }

	/*
	** Call psy_tbl_grant_check() to determine if the user may grant
	** specified permit on the given table or view.  psy_tbl_grant_check()
	** will be called if the object is owned by the current user or if the
	** kludge (described below) does not apply
	** KLUDGE:
	**	In order to support the existing (but hopefully, moribund)
	**	kludge, we will forego calling psy_tbl_grant_check() if the user
	**	has CATUPD and the object is a catalog or if the user is the DBA
	**	and the object is an extended catalog
	*/

	if (~psy_cb->psy_opmap & DB_ALARM)
	{
	    PSS_RNGTAB	*rngvar;

	    /*
	    ** if we are processing GRANT ALL, we may need to be able to
	    ** look up column names by attribute number
	    */
	    status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "", PST_SHWID,
		(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, &psy_tbl->psy_tabid,
		!grant_all, &rngvar, (i4) 0, &psy_cb->psy_error);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }

	    /* 
	    ** for GRANT text determine whether the schema name will be 
	    ** represented by a regular or delimited identifier, compute that 
	    ** identifier and its length 
	    **
	    ** schema name will be represented by a regular identifier if the 
	    ** user has specified it using a regular identifier - otherwise it 
	    ** will be represented by a delimited identifier
	    */
	    schema_name = (u_char *) &psy_tbl->psy_owner;
	    schema_name_len = 
		(i4) psf_trmwhite(sizeof(psy_tbl->psy_owner), (char *) schema_name);
	    if (~psy_tbl->psy_mask & PSY_REGID_SCHEMA_NAME)
	    {
		status = psl_norm_id_2_delim_id(&schema_name, &schema_name_len,
		    delim_schema_name, &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(status))
		    goto exit;
	    }

	    /*
	    ** if the object is a view owned by the current user or if the
	    ** abovementioned kludge does not apply, check if the user may grant
	    ** permit on this object
	    */
	    if (
		(
		 !MEcmp((PTR) &rngvar->pss_ownname, (PTR) &sess_cb->pss_user,
		     sizeof(sess_cb->pss_user))
		 &&
		 rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW
		)
		||
		!psy_permit_ok(rngvar->pss_tabdesc->tbl_status_mask, sess_cb,
		    &rngvar->pss_ownname)
	       )
	    {
		i4		tbl_wide_privs;
		PSY_COL_PRIVS	*csp, indep_col_specific_privs;
		DB_TAB_ID	indep_id;
		i4		indep_tbl_wide_privs;
		bool		insuf_privs, quel_view;

		/*
		** build maps of table-wide and column-specific privileges for
		** psy_tbl_grant_check();
		** 
		** if processing GRANT ALL [PRIIVLEGES] it is possible that the
		** user may be able to grant UPDATE only on some of the columns
		** of the table or view or REFERENCES only on some of the 
		** columns of the table - to handle such case without further
		** complicating the code, we will psy_tbl_grant_check(); a
		** pointer to the col_specific_privs and it will fill it in if
		** it turns out that the user can grant UPDATE or REFERENCES
		** (and soon - INSERT) on a subset of columns of the table
		** or view named in the GRANT statement
		*/

		/*
		** turn off AGGREGATE, TEST and GRANT_OPTION bits in case they
		** are set; the first two are represented by DB_SELECT, while
		** the last one is implied
		** For views also turn off COPY_INTO and COPY_FROM
		*/

		if(rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
		{
		    tbl_wide_privs = psy_cb->psy_opmap &
			~(DB_TEST | DB_AGGREGATE | DB_GRANT_OPTION |
			  DB_COPY_INTO | DB_COPY_FROM);
		}
		else
		{
		    tbl_wide_privs = psy_cb->psy_opmap &
		        ~(DB_TEST | DB_AGGREGATE | DB_GRANT_OPTION);
		}

		col_specific_privs.psy_col_privs = 0;

		if (updtcols)
		{
		    csp = &col_specific_privs;
		    csp->psy_col_privs |= DB_REPLACE;
		    tbl_wide_privs &= ~DB_REPLACE;

		    /* build the attribute map for UPDATE privilege */
		    psy_prv_att_map(
			(char *) csp->psy_attmap[PSY_UPDATE_ATTRMAP].map,
		        (bool) ((psy_cb->psy_flags & PSY_EXCLUDE_UPDATE_COLUMNS)
				!= 0),
		        psy_tbl->psy_colatno + psy_cb->psy_u_col_offset, 
			psy_cb->psy_u_numcols);
		}

		if (refcols)
		{
		    csp = &col_specific_privs;
		    csp->psy_col_privs |= DB_REFERENCES;
		    tbl_wide_privs &= ~DB_REFERENCES;

		    /* build the attribute map for REFERENCES privilege */
		    psy_prv_att_map(
			(char *) csp->psy_attmap[PSY_REFERENCES_ATTRMAP].map,
		        (bool) ((psy_cb->psy_flags & 
				     PSY_EXCLUDE_REFERENCES_COLUMNS) != 0),
		        psy_tbl->psy_colatno + psy_cb->psy_r_col_offset, 
			psy_cb->psy_r_numcols);
		}

		if (grant_all)
		{
		    csp = &col_specific_privs;
		    csp->psy_col_privs = 0;

		    /*
		    ** if processing GRANT ALL and the current object is a view,
		    ** remove REFERENCES from the list of table-wide privileges 
		    ** that may be granted
		    */
		    if (rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
		    {
		        tbl_wide_privs &= ~DB_REFERENCES;
		    }
		}

		/*
		** if neither column-specific privilege(s) nor ALL was 
		** specified, we will not pass a column-specific privilege
		** descriptor to psy_tbl_grant_check()
		**
		*/
		if (!colpriv_map && !grant_all)
		{
		    csp = (PSY_COL_PRIVS *) NULL;
		}

		status = psy_tbl_grant_check(sess_cb, (i4) PSQ_GRANT,
		    &rngvar->pss_tabid, &tbl_wide_privs, csp, &indep_id,
		    &indep_tbl_wide_privs, &indep_col_specific_privs,
		    psy_cb->psy_flags, &insuf_privs, &quel_view,
		    &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(status))
		{
		    goto exit;
		}

		if (insuf_privs)
		{
		    /*
		    ** current user may not grant specified permit on this
		    ** table, audit the failure and proceed to the next entry
		    */
		    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
		    {
			DB_ERROR	e_error;

			/* Must audit CREATE PERMIT failure. */
			status = psy_secaudit(FALSE, sess_cb,
			    (char *)&rngvar->pss_tabdesc->tbl_name,
			    &rngvar->pss_tabdesc->tbl_owner,
			    sizeof(DB_TAB_NAME), SXF_E_TABLE,
			    I_SX2016_PROT_TAB_CREATE, SXF_A_FAIL | SXF_A_CREATE,
			    &e_error);

			if (DB_FAILURE_MACRO(status))
			{
			    goto exit;
			}
		    }

		    continue;
		}

		if (quel_view)
		    continue;

		if (grant_all)
		{
		    /*
		    ** if it was determined that the user can grant some
		    ** table-wide privileges, store them into privs_to_grant;
		    ** otherwise reset noncol
		    */
		    if (tbl_wide_privs)
		    {
			/*
			** psy_tbl_grant_check() may have determined that a user
			** who specified ALL [PRIVILEGES] may not grant some of
			** them - here we set privs_to_grant to the set of
			** privileges that a user may actually grant
			*/
			privs_to_grant = psy_cb->psy_opmap &
			    (tbl_wide_privs | DB_GRANT_OPTION);
		    }
		    else
		    {
			noncol = FALSE;
		    }

		    updtcols = ((csp->psy_col_privs & DB_REPLACE) != 0);
		    refcols  = ((csp->psy_col_privs & DB_REFERENCES) != 0);

		    if (updtcols || refcols)
		    {
			/*
			** psy_tbl_grant_check() determined that a user may
			** grant UPDATE and/or REFERENCES only on some columns 
			** of the table or view - we need to build temporary 
			** templates for column-specific UPDATE and/or 
			** REFERENCES on the table or view + prepare a few other
			** things that would otherwise be predetermined
			*/

			/* 
			** open the memory stream where the query text for 
			** GRANT UPDATE and/or REFERENCES will be stored
			*/
			status = psf_mopen(sess_cb, QSO_QTEXT_OBJ, 
			    &sess_cb->pss_tstream, &psy_cb->psy_error);
			if (DB_FAILURE_MACRO(status))
			    goto exit;

		        if (updtcols)
		        {
			    status = psq_topen(&sess_cb->pss_tchain, 
			        &sess_cb->pss_memleft, &psy_cb->psy_error);
			    if (DB_FAILURE_MACRO(status))
			        goto exit;

			    status = psy_prvtxt(sess_cb, DB_REPLACE,
			        csp->psy_attmap + PSY_UPDATE_ATTRMAP, 
			        sess_cb->pss_tchain, rngvar->pss_attdesc, 
				psy_tbl, &u_grntee_type_offset, 
				&u_grantee_offset, buf, buf_text_len, 
			        (bool) ((psy_cb->psy_opmap & DB_GRANT_OPTION) 
					!= 0),
			        &u_txt, &sess_cb->pss_tstream,
			        &psy_cb->psy_error);
			    if (DB_FAILURE_MACRO(status))
			        goto exit;

			    /* now close the text chain */
			    status = psq_tclose(sess_cb->pss_tchain, 
			        &psy_cb->psy_error);
    
			    /* 
			    ** set text chain NULL to avoid trying to close it 
			    ** again
			    */
			    sess_cb->pss_tchain = (PTR) NULL;
    
			    if (DB_FAILURE_MACRO(status))
			        goto exit;
		        }

		        if (refcols)
		        {
			    status = psq_topen(&sess_cb->pss_tchain, 
			        &sess_cb->pss_memleft, &psy_cb->psy_error);
			    if (DB_FAILURE_MACRO(status))
			        goto exit;

			    status = psy_prvtxt(sess_cb, DB_REFERENCES,
			        csp->psy_attmap + PSY_REFERENCES_ATTRMAP, 
			        sess_cb->pss_tchain, rngvar->pss_attdesc, 
				psy_tbl, &r_grntee_type_offset, 
				&r_grantee_offset, buf, buf_text_len, 
			        (bool) ((psy_cb->psy_opmap & DB_GRANT_OPTION) 
					!= 0),
			        &r_txt, &sess_cb->pss_tstream,
			        &psy_cb->psy_error);
			    if (DB_FAILURE_MACRO(status))
			        goto exit;

			    /* now close the text chain */
			    status = psq_tclose(sess_cb->pss_tchain, 
			        &psy_cb->psy_error);
    
			    /* 
			    ** set text chain NULL to avoid trying to close it 
			    ** again
			    */
			    sess_cb->pss_tchain = (PTR) NULL;
    
			    if (DB_FAILURE_MACRO(status))
			        goto exit;
		        }
		    }
		}

		/*
		** If user is trying to grant one or more of
		** INSERT/DELETE/UPDATE on his/her view whose underlying table
		** or view is owned by another user, psy_tbl_grant_check() will
		** return id of the underlying object along with map of
		** privileges.  We will convert maps of independent privileges
		** into elements of independent privilege list and pass them
		** along to QEF
		*/
		if (   indep_id.db_tab_base != (i4) 0
		    && (   indep_id.db_tab_base != rngvar->pss_tabid.db_tab_base
		        || indep_id.db_tab_index !=
			       rngvar->pss_tabid.db_tab_index
		       )
		   )
		{
		    if (indep_tbl_wide_privs & DB_DELETE)
		    {
			/*
			** the only expected independent table-wide privilege
			** is DELETE
			*/
			obj_priv.psq_next		= (PSQ_OBJPRIV *) NULL;
			obj_priv.psq_objtype		= PSQ_OBJTYPE_IS_TABLE;
			obj_priv.psq_privmap		= (i4) DB_DELETE;
			obj_priv.psq_objid.db_tab_base	= indep_id.db_tab_base;
			obj_priv.psq_objid.db_tab_index = indep_id.db_tab_index;
			indep_objs.psq_objprivs		= &obj_priv;
		    }

		    if (indep_col_specific_privs.psy_col_privs)
		    {
			i4		i, j;
			PSQ_COLPRIV	*csp;
			i4		*att_map, *p;
			i4		priv_map = 0;

			/*
			** privilege map is built using bitwise operators, but
			** here using BTnext() makes code much more palatable,
			** so convert a privilege map
			*/
			if (indep_col_specific_privs.psy_col_privs & DB_APPEND)
			    BTset(DB_APPP, (char *) &priv_map);
			if (indep_col_specific_privs.psy_col_privs & DB_REPLACE)
			    BTset(DB_REPP, (char *) &priv_map);

			for (i = -1, csp = col_privs;
			     (i = BTnext(i, (char *) &priv_map,
				BITS_IN(priv_map))) != -1;
			      csp++
			    )
			{
			    csp->psq_next = indep_objs.psq_colprivs;
			    indep_objs.psq_colprivs = csp;
			    csp->psq_objtype = PSQ_OBJTYPE_IS_TABLE;
			    csp->psq_tabid.db_tab_base = indep_id.db_tab_base;
			    csp->psq_tabid.db_tab_index = indep_id.db_tab_index;
			    switch (i)
			    {
				case DB_APPP:	    /* INSERT privilege */
				{
				    csp->psq_privmap = (i4) DB_APPEND;
				    att_map = indep_col_specific_privs.
					psy_attmap[PSY_INSERT_ATTRMAP].map;
				    break;
				}
				case DB_REPP:
				{
				    csp->psq_privmap = (i4) DB_REPLACE;
				    att_map = indep_col_specific_privs.
					psy_attmap[PSY_UPDATE_ATTRMAP].map;
				    break;
				}
			    }

			    for (p = csp->psq_attrmap, j = 0;
				 j < DB_COL_WORDS;
				 j++)
			    {
				*p++ = *att_map++;
			    }
			}
		    }
		}
	    }
	    else if (rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
	    {
		/*
		** we have foregone checking if the current user can grant the
		** specified permit; if the current object is a view, we still
		** need to verify that it is an SQL view
		*/
		i4	    issql = 0;

		status = psy_sqlview(rngvar, sess_cb, &psy_cb->psy_error,
		    &issql);
		if (DB_FAILURE_MACRO(status))
		{
		    goto exit;
		}

		if (!issql)
		{
		    /* can only have permits on SQL views */
		    psf_error(3598L, 0L, PSF_USERERR, &err_code,
			&psy_cb->psy_error, 1, 
			psf_trmwhite(sizeof(rngvar->pss_tabname),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname);

		    /* skip to the next entry */
		    continue;
		}
		
		/*
		** if user specified GRANT ALL, set privs_to_grant to 
		** psy_cb->psy_opmap except for DB_REFERENCES since it may have
		** been reset for one of preceeding objects and REFERENCES on 
		** views is not allowed; 
		*/
		if (grant_all)
		{
		    privs_to_grant = psy_cb->psy_opmap & ~DB_REFERENCES;
		}
	    }
	    else if (grant_all)
	    {
		/*
		** GRANTing ALL PRIVILEGES on the user's base table - set
		** privs_to_grant to psy_cb->psy_opmap.  This the user could
		** grant only a subset of psy_cb->psy_opmap's privileges on the
		** previous object, this will not result in only that subset
		** being granted on this object as well
		*/
		privs_to_grant = psy_cb->psy_opmap;
	    }
	}

	/* The table which is receiving the permit */
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_rb->rdr_tabid);
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, protup->dbp_tabid);

	/* For invalidating the table info from the RDF cache */
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_inv_cb.rdf_rb.rdr_tabid);

	/* Initialize object name/type/status fields in the iiprotect tuple */
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm, protup->dbp_obname);
	protup->dbp_obstat = ' ';		    /*@FIX_ME@ 
						    ** Where do I get this?
						    */
	/* store the name of the object owner */
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, protup->dbp_obown);

	switch(psy_cb->psy_grant)		    /* Set dbp_obtype */
	{
	    default:
		if (~psy_cb->psy_opmap & DB_ALARM)
		{
		    protup->dbp_obtype = ' ';
		    break;
		}

		/* 
		** for security_alarms, fall through to the PSY_TGRANT case and 
		** set the object type 
		*/

	    case(PSY_TGRANT):
		protup->dbp_obtype = (psy_tbl->psy_mask & PSY_OBJ_IS_TABLE)
		    ? DBOB_TABLE : DBOB_VIEW;    
		break;
	    case(PSY_LGRANT):
		protup->dbp_obtype = DBOB_LOC;
		break;
	}

	/*
	** If we have update columns, privileges have to be granted in 3 stages:
	**
	**	    1 - grant table-wide privileges (if any)
	**	    2 - grant column-specific UPDATE privilege (if any)
	**	    3 - grant column-specific REFERENCES privilege (if any)
	*/

	/* Stage 1 */
	if (noncol)
	{
	    /*
	    ** Code below shall be executed for all GRANT privileges but 
	    ** UPDATE(cols) and REFERENCES(cols). If there are update/references
	    ** columns, the UPDATE/REFERENCES privileges need to be turned off,
	    ** because a separate piece of code shall define UPDATE/REFERENCES 
	    ** privilege and specify columns to which it is restricted.
	    */

	    rdf_rb->rdr_querytext = ((char *) txt_qsf_rb_p->qsf_root) +
		sizeof(i4);

	    /*
	    ** Reinitialize the operation map in the prot.tuple, if necessary.
	    ** When updtcols is TRUE, operation map in the protection tuple
	    ** will have to be modified. This shall happen later.
	    */

	    if (colpriv_map)
	    {
		/*
		** if column-specific privileges are being defined in addition
		** to table-wide ones, turn off bits representing
		** column-specific privileges in the protection tuple 
		**
		** Note that if a user specified ALL [PRIVILEGES],
		** colpriv_map would not be set (it will change once we start
		** processing GRANT ALL in a true ANSI-compliant manner)
		*/
		protup->dbp_popset = psy_cb->psy_opmap & ~colpriv_map;
		protup->dbp_popctl = psy_cb->psy_opctl & ~colpriv_map;
	    }
	    else if (grant_all)
	    {
		/*
		** if user specified ALL [PRIVILEGES], it may be that
		** he/she may grant some but not all of the legal
		** privileges, so we will need to set
		** protup->dbp_popset and protup->dbp_popctl accordingly
		*/
		protup->dbp_popset = privs_to_grant;
		protup->dbp_popctl = psy_cb->psy_opctl &
		    ~(psy_cb->psy_opmap ^ privs_to_grant);
	    }

	    if (split_perm && (colpriv_map || grant_all))
	    {
		/*
		** processing of column-specific privileges requires that we
		** reset RDF_SPLIT_PERM - restore it before processing
		** table-wide permits which must be split;
		** if processing GRANT ALL, we may end up granting
		** column-specific privileges, so set RDF_SPLIT_PERM here
		** just in case
		*/
		rdf_rb->rdr_instr |= RDF_SPLIT_PERM;
	    }

	    /* Give permission on all columns */
	    psy_fill_attmap(domset, ~((i4) 0));

	    /*
	    ** Do the name substitution for schema.table for GRANT or table for
	    ** CREATE SECURITY_ALARM in the first iiqrytext.  Note that we will
	    ** trim any trailing blanks found in the names.
	    */
	    ch = (char *) rdf_rb->rdr_querytext +
		    psy_cb->psy_noncol_obj_name_off;
	    if (~psy_cb->psy_opmap & DB_ALARM)
	    {
		/* insert schema. only for GRANT */
		MEcopy((PTR) schema_name, schema_name_len, (PTR) ch);
		ch += schema_name_len;
		CMcpychar(".", ch);
		CMnext(ch);
	    }

	    MEcopy((PTR) obj_name, obj_name_len, (PTR) ch);
	    grntee_type_start = ch + obj_name_len;

	    /*
	    ** remember whether user has explicitly specified TO PUBLIC (in
	    ** GRANT) or BY PUBLIC (in CREATE SECURITY_ALARM)
	    */
	    grant_to_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    if (!grant_to_public)
	    {
		/*
		** if PUBLIC was not specified, text which must appear between
		** the object name and the grantee name will not change so copy
		** it here; note that in CREATE SECURITY_ALARM statement, if the
		** user did not specify any auth. ids, default is BY PUBLIC, but
		** we don't want to append the string "by public" unless the
		** user said so.
		*/
		MEcopy((PTR) buf, buf_text_len, (PTR) grntee_type_start);
	    }
	    /*
	    **	If PUBLIC and CREATE SECURITY_ALARM remember  to copy
	    **  any text (IF, WHEN clauses) between the object and 
	    **  BY PUBLIC otherwise they would get lost
	    */
	    if (grant_to_public  && (psy_cb->psy_opmap & DB_ALARM))
	    {
		MEcopy((PTR) buf, buf_text_len, (PTR) grntee_type_start);
		grntee_type_start += buf_text_len;
	    }

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    /*
	    ** if user specified auth. id(s) other than PUBLIC, compute length
	    ** of query text w/o grantee name + compute where grantee name will
	    ** start
	    */
	    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
	    {
		/*
		** the template contains a grantee name placeholder, a schema
		** name placeholder (only for GRANT statement), and an object 
		** name placeholder, each of length DB_MAX_DELIMID.  
		** Since the actual schema name (only for GRANT statement), 
		** object name and grantee name are likely to be of different 
		** length, adjust length of IIQRYTEXT tuple to reflect lengths 
		** of the actual schema name (only for GRANT statement) and 
		** object name.  We will account for the length of grantee name
		** once we start cycling through the grantee list.
		*/
		text_len_without_grantee = (psy_cb->psy_opmap & DB_ALARM)
		    ? psy_cb->psy_noncol_qlen - 2 * DB_MAX_DELIMID + 
		      obj_name_len 
		    : psy_cb->psy_noncol_qlen - 3 * DB_MAX_DELIMID +
		      schema_name_len + obj_name_len;

		/*
		** Compute where the grantee name will be inserted into the
		** query text.  The address of the beginning of grantee name
		** needs to be adjusted to take into account the length of the
		** actual schema name (only for GRANT statement) and object name
		*/

		grantee_start = (psy_cb->psy_opmap & DB_ALARM)
		    ? (char *) rdf_rb->rdr_querytext +
		      psy_cb->psy_noncol_grantee_off - DB_MAX_DELIMID + 
		      obj_name_len
		    : (char *) rdf_rb->rdr_querytext +
		      psy_cb->psy_noncol_grantee_off -
		      2 * DB_MAX_DELIMID + schema_name_len + obj_name_len;
	    }

	    do
	    {
		/*
		** Get name of user getting privileges
		**
		** recall that psy_cb->psy_gtype would be set to DBGR_PUBLIC
		** only if we are processing a CREATE SECURITY_ALARM and user
		** did not specify any authorization ids
		*/
		
		if (!grant_to_public && psy_cb->psy_gtype != DBGR_PUBLIC)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);

		    /* 
		    ** determine whether the grantee name will be represented by
		    ** a regular or delimited identifier, compute that 
		    ** identifier and its length 
		    **
		    ** grantee name will be represented by a regular identifier
		    ** if the user has specified it using a regular identifier -
		    ** otherwise it will be represented by a delimited 
		    ** identifier
		    */
		    grantee_name = (u_char *) &psy_usr->psy_usrnm;
		    grantee_name_len = 
			(i4) psf_trmwhite(sizeof(psy_usr->psy_usrnm), 
			    (char *) grantee_name);
		    if (~psy_usr->psy_usr_flags & PSY_REGID_USRSPEC)
		    {
			status = psl_norm_id_2_delim_id(&grantee_name, 
			    &grantee_name_len, delim_grantee_name, 
			    &psy_cb->psy_error);
			if (DB_FAILURE_MACRO(status))
			    goto exit;
		    }

		    /*
		    ** text_len_without_grantee contains length of query text
		    ** without grantee name.  Once we know the grantee name
		    ** length, we can compute the query text length
		    */

		    rdf_rb->rdr_l_querytext =
			text_len_without_grantee + grantee_name_len;

		    /* 
		    ** Do the user name substitution in the iiqrytext.
		    ** SECURITY ALARMS don't have quotes for the user.
		    */
		    ch = grantee_start;
		    MEcopy((PTR) grantee_name, grantee_name_len, (PTR) ch);
		    ch += grantee_name_len;
		}
		else
		{
		    char	    *str;
		    i4		    str_len;

		    /*
		    ** No grantee name was specified, use default.
		    */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);

		    if (grant_to_public)
		    {
			/* user has explicitly specified TO/BY PUBLIC */
			
			protup->dbp_gtype = DBGR_PUBLIC;
 
			/* append " to/by public" to the template now
			*/
			if (psy_cb->psy_opmap & DB_ALARM)
			{
			    str = " by public";
			    str_len = sizeof(" by public") - 1;
			}
			else
			{
			    str = " to public";
			    str_len = sizeof(" to public") - 1;
			}
		        MEcopy((PTR) str, str_len, (PTR) grntee_type_start);
			
		    }

		    /*
		    ** if processing GRANT statement, the template contains a
		    ** schema name placeholder, an object name placeholder, and
		    ** a grantee name placeholder (all of length DB_MAX_DELIMID)
		    ** 
		    ** if processing CREATE SECURITY_ALARM, the template
		    ** contains an object name placeholder of length 
		    ** DB_MAX_DELIMID and, if the user explicitly specified 
		    ** "BY PUBLIC", an authorization id placeholder of length
		    ** DB_MAX_DELIMID 
		    **
		    ** And to make matters a tad more complicated, we chose to
		    ** overwrite the grantee type string specified by the user
		    ** (" to " or " to user " with " to " for GRANT and
		    **  " by " or " by user " with " by " for CREATE
		    **	SECURITY_ALARM.)
		    **	
		    ** Since the actual names are likely to be of different
		    ** length, adjust length of IIQRYTEXT tuple accordingly.
		    */
		    if (psy_cb->psy_opmap & DB_ALARM)
		    {
			if (grant_to_public)
			{
			    rdf_rb->rdr_l_querytext = psy_cb->psy_noncol_qlen -
				DB_MAX_DELIMID + obj_name_len + str_len;
			}
			else
			{
			    rdf_rb->rdr_l_querytext = psy_cb->psy_noncol_qlen -
				DB_MAX_DELIMID + obj_name_len;
			}
		    }
		    else
		    {
			rdf_rb->rdr_l_querytext = psy_cb->psy_noncol_qlen -
			    3 * DB_MAX_DELIMID + schema_name_len + 
			    obj_name_len - buf_text_len + str_len;
		    }
		}

		if (psy_cb->psy_opmap & DB_GRANT_OPTION)
		{
		    /*
		    ** if granting to PUBLIC, ch points to the beginning of
		    ** table name; otherwise, it already points to a character
		    ** immediately following the grantee name
		    */
		    if (grant_to_public)
		    {
			ch += obj_name_len + sizeof(" to public") - 1;
		    }

		    MEcopy((PTR) " with grant option ",
			sizeof(" with grant option ") - 1, (PTR) ch);
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		    {
			_VOID_ psf_error(E_PS0903_TAB_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				(char *) &psy_tbl->psy_tabnm),
			    &psy_tbl->psy_tabnm);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			    &psy_cb->psy_error);
		    }
		    goto exit;
		}

		/*
		** if the user has explicitly specified TO/BY PUBLIC and we have
		** just created a permit or security_alarm for PUBLIC auth. id,
		** dbp_gtype in IIPROCEDURE tuple and the IIQRYTEXT template
		** needs to be modified if some "real" grantees were named
		*/
		if (grant_to_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			grant_to_public = FALSE;

			/*
			** now copy the text which must appear between the
			** object name and the user name
			*/
			MEcopy((PTR) buf, buf_text_len, (PTR) grntee_type_start);
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);
	}
	else if (grant_all)
	{
	    /*
	    ** when processing GRANT ALL, if user can grant no table-wide 
	    ** privileges on a table/view we would set noncol to FALSE - reset 
	    ** it here for the next object;
	    */
	    noncol = TRUE;
	}

	/*
	** Stage 2.
	** Now call RDF to store GRANT UPDATE (col(s)) privilege.
	*/
	if (updtcols)
	{
	    /*
	    ** turn off the "split permit" bit since for column-specific
	    ** permit we supply the query text
	    */
	    if (split_perm)
	    {
		rdf_rb->rdr_instr &= ~RDF_SPLIT_PERM;
	    }

	    /*
	    ** now insert UPDATE [EXCLUDING] (<column list>) [WGO] permit tuple
	    */ 
	    protup->dbp_popset = protup->dbp_popctl =
		psy_cb->psy_opmap & (DB_REPLACE | DB_GRANT_OPTION);

	    if (grant_all)
	    {
		/* 
		** if we are processing GRANT ALL and it turned out that
		** the user can GRANT UPDATE only on some columns of the
		** view, we have already prepared a bit map describing
		** attributes on which the permit can be granted + we built
		** the query text template including names of the object
		** owner and of the object itself
		*/
		i4   *p = col_specific_privs.psy_attmap[PSY_UPDATE_ATTRMAP].map;

		for (i = 0; i < DB_COL_WORDS; i++)
		    domset[i] = p[i];

	        rdf_rb->rdr_querytext = (PTR) u_txt->db_t_text;
		grntee_type_start = 
		    (char *) rdf_rb->rdr_querytext + u_grntee_type_offset;
	    }
	    else
	    {
		psy_prv_att_map(
		    (char *) domset,
		    (bool) ((psy_cb->psy_flags & PSY_EXCLUDE_UPDATE_COLUMNS)
			    != 0),
		    psy_tbl->psy_colatno + psy_cb->psy_u_col_offset, 
		    psy_cb->psy_u_numcols);

	        /*
	        ** Set iiqrytext information.
	        */
	        rdf_rb->rdr_querytext = ((char *) txt_qsf_rb_p->qsf_root) +
		    sizeof(i4) + psy_cb->psy_noncol_qlen;

	        /*
	        ** Do the name substitution for schema.table name in iiqrytext.
	        ** Note that we will trim any trailing blanks found in the 
		** object name.
	        */
	        ch = (char *) rdf_rb->rdr_querytext +
		    psy_cb->psy_updt_obj_name_off;
	        MEcopy((PTR) schema_name, schema_name_len, (PTR) ch);
	        ch += schema_name_len;
	        CMcpychar(".", ch);
	        CMnext(ch);
	        MEcopy((PTR) obj_name, obj_name_len, (PTR) ch);
	        grntee_type_start = ch + obj_name_len;
	    }

	    /*
	    ** remember whether user has explicitly specified TO PUBLIC (in
	    ** GRANT) or BY PUBLIC (in CREATE SECURITY_ALARM)
	    */
	    grant_to_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    if (!grant_to_public)
	    {
		/*
		** if PUBLIC was not specified, text which must appear between
		** the object name and the grantee name will not change so copy
		** it here
		*/
		MEcopy((PTR) buf, buf_text_len, (PTR) grntee_type_start);
	    }

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    /*
	    ** if user specified auth. id(s) other than PUBLIC, compute length
	    ** of query text w/o grantee name + compute where grantee name will
	    ** start
	    */
	    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
	    {
		/*
		** if we are processing GRANT ALL, the template was
		** built for a specific object, so text_len_without_grantee
		** and grantee_start need to be computed differently
		*/
		if (grant_all)
		{
		    text_len_without_grantee = 
			u_txt->db_t_count - DB_MAX_DELIMID;
		    grantee_start = 
			(char *) rdf_rb->rdr_querytext + u_grantee_offset;
		}
		else
		{
		    /*
		    ** the template contains a grantee name placeholder, a
		    ** schema name placeholder, and an object name placeholder,
		    ** each of length DB_MAX_DELIMID.  Since the actual
		    ** schema name, object name and grantee name are
		    ** likely to be of different length, adjust length of
		    ** IIQRYTEXT tuple to reflect lengths of the actual object
		    ** owner name and object name.  We will account for the 
		    ** length of grantee name once we start cycling through 
		    ** the grantee list.
		    */
    
		    text_len_without_grantee = psy_cb->psy_updt_qlen -
		        3 * DB_MAX_DELIMID + schema_name_len + obj_name_len;

		    /*
		    ** Compute where the grantee name will be inserted into the
		    ** query text.  The address of the beginning of grantee name
		    ** needs to be adjusted to take into account the length of 
		    ** the actual object owner name and object name
		    */

		    grantee_start =
		        (char *) rdf_rb->rdr_querytext +
			    psy_cb->psy_updt_grantee_off -
			    2 * DB_MAX_DELIMID + schema_name_len + obj_name_len;
		}
	    }

	    do
	    {
		/* Get name of user getting privileges */
		if (!grant_to_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);

		    /* 
		    ** determine whether the grantee name will be represented by
		    ** a regular or delimited identifier, compute that 
		    ** identifier and its length 
		    **
		    ** grantee name will be represented by a regular identifier
		    ** if the  user has specified it using a regular 
		    ** identifier - otherwise it will be represented by a 
		    ** delimited identifier
		    */
		    grantee_name = (u_char *) &psy_usr->psy_usrnm;
		    grantee_name_len = 
			(i4) psf_trmwhite(sizeof(psy_usr->psy_usrnm), 
			    (char *) grantee_name);
		    if (~psy_usr->psy_usr_flags & PSY_REGID_USRSPEC)
		    {
			status = psl_norm_id_2_delim_id(&grantee_name, 
			    &grantee_name_len, delim_grantee_name, 
			    &psy_cb->psy_error);
			if (DB_FAILURE_MACRO(status))
			    goto exit;
		    }

		    /*
		    ** text_len_without_grantee contains length of query text
		    ** without grantee name.  Now that we know the grantee name
		    ** length, we can compute the query text length
		    */

		    rdf_rb->rdr_l_querytext =
			text_len_without_grantee + grantee_name_len;

		    /* Do the user name substitution in the iiqrytext. */
		    ch = grantee_start;
		    MEcopy((PTR) grantee_name, grantee_name_len, (PTR) ch);
		    ch += grantee_name_len;
		}
		else
		{
		    /*
		    ** No grantee names were specified, use default.
		    */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);

		    /*
		    ** reset dbp_gtype since this permit will specify
		    ** privileges which will be granted to PUBLIC
		    */
		    protup->dbp_gtype = DBGR_PUBLIC;
		    
		    MEcopy((PTR) " to public",
			(sizeof(" to public") - 1),
			(PTR) grntee_type_start);

		    if (grant_all)
		    {
		        /*
		        ** template built for GRANT ALL differs from
		        ** that built for regular GRANT UPDATE() in that it
		        ** is built for a specific object, thus requiring a
		        ** different formula for computing the length of
		        ** this query text
		        */
			rdf_rb->rdr_l_querytext = u_txt->db_t_count - 
			    DB_MAX_DELIMID - buf_text_len + 
			    sizeof(" to public") - 1;
		    }
		    else
		    {
		        /*
		        ** the template contains a schema name placeholder,
			** an object name placeholder, and a grantee name 
			** placeholder (all of length DB_MAX_DELIMID
		        ** 
		        ** Since the actual schema name and object 
			** name are likely to be of different length + we 
			** have replaced grantee name along with " to " or 
			** " to user " suppled by user with " to public", 
			** we need to adjust length of IIQRYTEXT tuple 
		        */
		        rdf_rb->rdr_l_querytext = psy_cb->psy_updt_qlen -
			    3 * DB_MAX_DELIMID + schema_name_len + 
			    obj_name_len - buf_text_len + 
			    sizeof(" to public") - 1;
		    }
		}

		if (psy_cb->psy_opmap & DB_GRANT_OPTION)
		{
		    /*
		    ** if granting to PUBLIC, set ch using
		    ** grntee_type_start; otherwise, it already points at a 
		    ** character immediately folowing the grantee name
		    */
		    if (grant_to_public)
		    {
		        ch = grntee_type_start + sizeof(" to public") - 1;
		    }

		    MEcopy((PTR) " with grant option ",
			sizeof(" with grant option ") - 1, (PTR) ch);
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		    {
			_VOID_ psf_error(E_PS0903_TAB_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				(char *) &psy_tbl->psy_tabnm),
			    &psy_tbl->psy_tabnm);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			    &psy_cb->psy_error);
		    }
		    goto exit;
		}

		/*
		** if the user has explicitly specified TO/BY PUBLIC and we have
		** just created a permit for PUBLIC auth. id, dbp_gtype in 
		** IIPROTECT tuple and the IIQRYTEXT template needs to be 
		** modified if some "real" grantees were named
		*/
		if (grant_to_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			grant_to_public = FALSE;

			/*
			** now copy the text which must appear between the
			** object name and the user name
			*/
			MEcopy((PTR) buf, buf_text_len,
			    (PTR) grntee_type_start);
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);	

	} /* if (updtcols == TRUE) */

	/*
	** Stage 3.
	** Now call RDF to store GRANT REFERENCES (col(s)) privilege.
	*/
	if (refcols)
	{
	    /*
	    ** turn off the "split permit" bit since for column-specific
	    ** permit we supply the query text
	    */
	    if (split_perm)
	    {
		rdf_rb->rdr_instr &= ~RDF_SPLIT_PERM;
	    }

	    /*
	    ** now insert REFERENCES [EXCLUDING] (<column list>) [WGO] 
	    ** permit tuple
	    */ 
	    protup->dbp_popset = protup->dbp_popctl =
		psy_cb->psy_opmap & (DB_REFERENCES | DB_GRANT_OPTION);

	    if (grant_all)
	    {
		/* 
		** if we are processing GRANT ALL and it turned out that
		** the user can GRANT REFERENCES only on some columns of the
		** view, we have already prepared a bit map describing
		** attributes on which the permit can be granted + we built
		** the query text template including names of the object
		** owner and of the object itself
		*/
		i4   *p = 
		      col_specific_privs.psy_attmap[PSY_REFERENCES_ATTRMAP].map;

		for (i = 0; i < DB_COL_WORDS; i++)
		    domset[i] = p[i];

	        rdf_rb->rdr_querytext = (PTR) r_txt->db_t_text;
		grntee_type_start = 
		    (char *) rdf_rb->rdr_querytext + r_grntee_type_offset;
	    }
	    else
	    {
		psy_prv_att_map(
		    (char *) domset,
		    (bool) ((psy_cb->psy_flags & PSY_EXCLUDE_REFERENCES_COLUMNS)
			    != 0),
		    psy_tbl->psy_colatno + psy_cb->psy_r_col_offset, 
		    psy_cb->psy_r_numcols);

	        /*
	        ** Set iiqrytext information. we must account for a "noncol"
		** template and the column-specific UPDATE template
	        */
	        rdf_rb->rdr_querytext = ((char *) txt_qsf_rb_p->qsf_root) +
		    sizeof(i4) + psy_cb->psy_noncol_qlen +
		    psy_cb->psy_updt_qlen;

	        /*
	        ** Do the name substitution for schema.table name in iiqrytext.
	        ** Note that we will trim any trailing blanks found in the 
		** object name.
	        */
	        ch = (char *) rdf_rb->rdr_querytext +
		    psy_cb->psy_ref_obj_name_off;
	        MEcopy((PTR) schema_name, schema_name_len, (PTR) ch);
	        ch += schema_name_len;
	        CMcpychar(".", ch);
	        CMnext(ch);
	        MEcopy((PTR) obj_name, obj_name_len, (PTR) ch);
	        grntee_type_start = ch + obj_name_len;
	    }

	    /*
	    ** remember whether user has explicitly specified TO PUBLIC 
	    */
	    grant_to_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    if (!grant_to_public)
	    {
		/*
		** if PUBLIC was not specified, text which must appear between
		** the object name and the grantee name will not change so copy
		** it here
		*/
		MEcopy((PTR) buf, buf_text_len, (PTR) grntee_type_start);
	    }

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    /*
	    ** if user specified auth. id(s) other than PUBLIC, compute length
	    ** of query text w/o grantee name + compute where grantee name will
	    ** start
	    */
	    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
	    {
		/*
		** if we are processing GRANT ALL, the template was
		** built for a specific object, so text_len_without_grantee
		** and grantee_start need to be computed differently
		*/
		if (grant_all)
		{
		    text_len_without_grantee = 
			r_txt->db_t_count - DB_MAX_DELIMID;
		    grantee_start = 
			(char *) rdf_rb->rdr_querytext + r_grantee_offset;
		}
		else
		{
		    /*
		    ** the template contains a grantee name placeholder, a
		    ** schema name placeholder, and an object name
		    ** placeholder, each of length DB_MAX_DELIMID.  Since the 
		    ** actual schema name, object name and grantee name are
		    ** likely to be of different length, adjust length of
		    ** IIQRYTEXT tuple to reflect lengths of the actual 
		    ** schema name and object name.  We will account for the 
		    ** length of grantee name once we start cycling through 
		    ** the grantee list.
		    */
    
		    text_len_without_grantee = psy_cb->psy_ref_qlen -
			3 * DB_MAX_DELIMID + schema_name_len + obj_name_len;

		    /*
		    ** Compute where the grantee name will be inserted into the
		    ** query text.  The address of the beginning of grantee name
		    ** needs to be adjusted to take into account the length of 
		    ** the actual schema name and object name
		    */

		    grantee_start =
		        (char *) rdf_rb->rdr_querytext +
			    psy_cb->psy_ref_grantee_off -
			    2 * DB_MAX_DELIMID + schema_name_len + obj_name_len;
		}
	    }

	    do
	    {
		/* Get name of user getting privileges */
		if (!grant_to_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);


		    /* 
		    ** determine whether the grantee name will be represented by
		    ** a regular or delimited identifier, compute that 
		    ** identifier and its length 
		    **
		    ** grantee name will be represented by a regular identifier
		    ** if the user has specified it using a regular identifier -
		    ** otherwise it will be represented by a delimited 
		    ** identifier
		    */
		    grantee_name = (u_char *) &psy_usr->psy_usrnm;
		    grantee_name_len = 
			(i4) psf_trmwhite(sizeof(psy_usr->psy_usrnm), 
			    (char *) grantee_name);
		    if (~psy_usr->psy_usr_flags & PSY_REGID_USRSPEC)
		    {
			status = psl_norm_id_2_delim_id(&grantee_name, 
			    &grantee_name_len, delim_grantee_name, 
			    &psy_cb->psy_error);
			if (DB_FAILURE_MACRO(status))
			    goto exit;
		    }

		    /*
		    ** text_len_without_grantee contains length of query text
		    ** without grantee name.  Now that we know the grantee name
		    ** length, we can compute the query text length
		    */

		    rdf_rb->rdr_l_querytext =
			text_len_without_grantee + grantee_name_len;

		    /* Do the user name substitution in the iiqrytext. */
		    ch = grantee_start;
		    MEcopy((PTR) grantee_name, grantee_name_len, (PTR) ch);
		    ch += grantee_name_len;
		}
		else
		{
		    /*
		    ** No grantee names were specified, use default.
		    */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);

		    /*
		    ** reset dbp_gtype since this permit will specify
		    ** privileges which will be granted to PUBLIC
		    */
		    protup->dbp_gtype = DBGR_PUBLIC;
		    
		    MEcopy((PTR) " to public",
			(sizeof(" to public") - 1),
			(PTR) grntee_type_start);

		    if (grant_all)
		    {
		        /*
		        ** template built for GRANT ALL differs from
		        ** that built for regular GRANT UPDATE() in that it
		        ** is built for a specific object, thus requiring a
		        ** different formula for computing the lenth of
		        ** this query text
		        */
			rdf_rb->rdr_l_querytext = r_txt->db_t_count - 
			    DB_MAX_DELIMID - buf_text_len + 
			    sizeof(" to public") - 1;
		    }
		    else
		    {
		        /*
		        ** the template contains a schema name placeholder ,
			** an object name placeholder, and a grantee name 
			** placeholder (all of length DB_MAX_DELIMID)
		        ** 
		        ** Since the actual schema name and object 
			** name are likely to be of different length + we 
			** have replaced grantee name along with " to " or 
			** " to user " suppled by user with " to public", 
			** we need to adjust length of IIQRYTEXT tuple 
		        */
		        rdf_rb->rdr_l_querytext = psy_cb->psy_ref_qlen -
			    3 * DB_MAX_DELIMID + schema_name_len + 
			    obj_name_len - buf_text_len + 
			    sizeof(" to public") - 1;
		    }
		}

		if (psy_cb->psy_opmap & DB_GRANT_OPTION)
		{
		    /*
		    ** if granting to PUBLIC, set ch using
		    ** grntee_type_start; otherwise, ch already points to the 
		    ** character immediately following grantee name
		    */
		    if (grant_to_public)
		    {
			ch = grntee_type_start + sizeof(" to public") - 1;
		    }

		    MEcopy((PTR) " with grant option ",
			sizeof(" with grant option ") - 1, (PTR) ch);
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		    {
			_VOID_ psf_error(E_PS0903_TAB_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				(char *) &psy_tbl->psy_tabnm),
			    &psy_tbl->psy_tabnm);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			    &psy_cb->psy_error);
		    }
		    goto exit;
		}

		/*
		** if the user has explicitly specified TO/BY PUBLIC and we have
		** just created a permit for PUBLIC auth. id, dbp_gtype in 
		** IIPROTECT tuple and the IIQRYTEXT template needs to be 
		** modified if some "real" grantees were named
		*/
		if (grant_to_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			grant_to_public = FALSE;

			/*
			** now copy the text which must appear between the
			** object name and the user name
			*/
			MEcopy((PTR) buf, buf_text_len, (PTR) grntee_type_start);
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);	

	} /* if (refcols == TRUE) */

	if (grant_all && (updtcols || refcols || insrtcols))
	{
	    /* 
	    ** user specified ALL [PRIVILEGES but was able to grant UPDATE 
	    ** and/or REFERENCES (and in the future) and/or INSERT only on a
	    ** subset of columns - we must remember to close the memory stream
	    ** that was used to build the generated  on-the-fly query text
	    ** templates for these permits
	    **
	    ** we also reset updtcols, refcols and insrtcols to FALSE since we
	    ** cannot  predict which privileges a user may grant on the next
	    ** object in the list
	    */
	    updtcols = refcols = insrtcols = FALSE;
	    status = psf_mclose(sess_cb, &sess_cb->pss_tstream, &psy_cb->psy_error);
	    sess_cb->pss_tstream.psf_mstream.qso_handle = (PTR) NULL;
	    if (DB_FAILURE_MACRO(status))
	        goto exit;
	}

	/*
	** make independent privilege lists empty since there may be no
	** independent privileges for the next table/view
	*/
	indep_objs.psq_objprivs = (PSQ_OBJPRIV *) NULL;
	indep_objs.psq_colprivs = (PSQ_COLPRIV *) NULL;

	/*
	** Invalidate base object's infoblk from RDF cache: rdr_tabid already
	** contains base table id; rdr_2types_mask has been initialized.
	*/
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
	    break;
	}
    } /* for each table */

exit:
    /* Destroy query text */
    
    if ((txt_qsf_rb_p->qsf_lk_id = text_lock) == 0)
    {
	stat = qsf_call(QSO_LOCK, txt_qsf_rb_p);
	if (DB_FAILURE_MACRO(stat))
	{
	    _VOID_ psf_error(E_PS0D18_QSF_LOCK,
		txt_qsf_rb_p->qsf_error.err_code, PSF_INTERR, &err_code,
		&psy_cb->psy_error, 0);
	    if (stat > status)
		status = stat;
	}
	else
	{
	    text_lock = txt_qsf_rb_p->qsf_lk_id;
	}
    }

    if (text_lock)
    {
	/*
	** don't try to destroy the QSF object without first having successfully
	** locked it
	*/
	stat = qsf_call(QSO_DESTROY, txt_qsf_rb_p);
	if (DB_FAILURE_MACRO(stat))
	{
	    _VOID_ psf_error(E_PS0D1A_QSF_DESTROY,
		txt_qsf_rb_p->qsf_error.err_code, PSF_INTERR, &err_code,
		&psy_cb->psy_error, 0);
	    if (stat > status)
		status = stat;
	}
    }

    if (grant_all && psy_cb->psy_grant == PSY_TGRANT)
    {
	/* 
	** when processing GRANT ALL on tables and views, we may have
	** needed to create a temporary query text template - if either the
	** text chain or the text stream have not been closed, do it now
	*/
	if (sess_cb->pss_tchain)
	{
	    stat = psq_tclose(sess_cb->pss_tchain, &psy_cb->psy_error);
	    if (stat > status)
		status = stat;
	    sess_cb->pss_tchain = (PTR) NULL;
	}

	if (sess_cb->pss_tstream.psf_mstream.qso_handle)
	{
	    stat = psf_mclose(sess_cb, &sess_cb->pss_tstream, &psy_cb->psy_error);
	    if (stat > status)
		status = stat;
	}
    }

    return    (status);
} 

/*
** Name: psy_permit_ok - determine if the user may GRANT/CREATE/DEFINE a permit
**			 on a table/view
**
** Description:
**	This function will return TRUE if present user may GRANT/CREATE/DEFINE a
**	permit and FALSE otherwise.  As described below, a permit may be
**	granted/created if at least one of the following holds:
**	    1) Object is catalog or an extended catalog, and user has catalog
**	       update priviledge;
**	    2) Object is an extended catalog, and user is a DBA
**	    3) Object is owned by the user.
**	For DEFINE PERMIT (3) in the above restriction is changed to
**	    3) Object is owned by the user WHO IS THE DBA
**
** Input:
**	obj_mask		mask describing the object
**	    DMT_CATALOG		object is a catalog
**	    DMT_EXTENDED_CAT	object is an extended catalog
**	    otherwise		object is a regular table/view
**
**	cb			PSF session CB
**	    pss_lang		language of the query
**	    pss_ses_flag	flag describing session attributes
**		PSS_CATUPD	session posesses catalog update privilege
**	    pss_user		current DBMS session user identifier
**	    pss_dba		dba identifier
**
**	obj_owner		identifier of the owner of the object
**
** Output:
**	none
**
** Returns:
**	TRUE if the user may GRANT/CREATE/DEFINE a permit on the object; FALSE
**	otherwise
**
** Side effects:
**	none
**	
** History:
**
**	29-jul-91 (andre)
**	    this function will no longer be called for GRANT.  Eventually, it
**	    will be phased out since all DDL statements (except for GRANT) will
**	    be disallowed on objects not owned by the current user.
**	20-feb-92 (andre)
**	    this function can now be called during processing of DEFINE PERMIT
**	    the difference between CREATE PERMIT and DEFINE PERMIT is that
**	    unless the object is a catalog and the session posesses CATUPD, the
**	    user MUST be the DBA.
*/
bool
psy_permit_ok(
	i4		obj_mask,
	PSS_SESBLK	*cb,
	DB_OWN_NAME	*obj_owner)
{
    /*
    ** We will allow mere mortals GRANT/DROP permits on their tables
    ** too.  We will relax conditions which must be satisfied in
    ** order to let one GRANT/DROP a permit as follows:
    ** Let
    **	    E = Extended cat.
    **	    C = not_an_extended catalog
    **	    T = Table (not a catalog)
    **	    D = user is a DBA
    **	    I = user is $ingres
    **	    O = user is owner
    **	    CU= user has catalog update privilege
    **	    + = LOGICAL OR
    **	    * = LOGICAL AND
    **	Presently, in order issue GRANT the following must hold:
    **	    (E+C) * CU + E * (D+I) + (T+C) * O * (D+I)
    **	We will change it making sure that semantics of
    **	accessing catalogs is not altered, i.e.
    **	(E+C) * CU + E * (D+I) + C * O * (D+I) + T * O
    **	Note that we eased the restriction on non-catalogs.
    **	Further note that C * O * (D+I) <==> C * O since
    **	catalogues are owned by $ingres.  Finally,
    **	E * (D+I) <==> E * (O+D) since extended catalogues are
    **	owned by $ingres.
    **	So the final restriction is:
    **	(E+C) * CU + E * (O+D) + C * O + T * O =
    **	(E+C) * CU + E * D + O * (C+E+T) =
    **	(E+C) * CU + E * D + O
    **	since all objects are either E or C or T,
    **	i.e. C+E+T == TRUE.
    **
    **	if called when processing a QUEL query (DEFINE PERMIT), the restriction
    **	is slightly modified since in QUEL only the DBA may define a permit
    **	unless the object is a catalog and the session posesses CATUPD,
    **	hence for QUEL the restriction is:
    **	    (E+C) * CU + D * (E * D + O) = (E+C) * CU + D * (E + O)
    */

    if (cb->pss_lang == DB_SQL)
    {
	return((   obj_mask & (DMT_CATALOG | DMT_EXTENDED_CAT)
		&& cb->pss_ses_flag & PSS_CATUPD
	       )
	       ||
	       (   obj_mask & DMT_EXTENDED_CAT
	        && !MEcmp((PTR) &cb->pss_user, (PTR) &cb->pss_dba,
			  sizeof(DB_OWN_NAME))
	       )
	       ||
	       !MEcmp((PTR) &cb->pss_user, (PTR) obj_owner,
		      sizeof(DB_OWN_NAME))
	      );
    }
    else
    {
	return((   obj_mask & (DMT_CATALOG | DMT_EXTENDED_CAT)
	        && cb->pss_ses_flag & PSS_CATUPD
	       )
	       ||
	       (   !MEcmp((PTR) &cb->pss_user, (PTR) &cb->pss_dba,
		          sizeof(DB_OWN_NAME))
		&& (   obj_mask & DMT_EXTENDED_CAT
		    || !MEcmp((PTR) &cb->pss_user, (PTR) obj_owner,
			      sizeof(DB_OWN_NAME))
		   )
	       )
	      );
    }
}

/*
** Name: psy_prvmap_to_str - convert a privilege map into a string of SQL
**			     privilege names
**
** Description:
**	Given a map of privileges, build a string consisting of comma-separated
**	SQL privilege names.  If DB_GRANT_OPTION bit is set, "with grant option"
**	will be added at the end of the string
**
** Input:
**	privs		map of privileges
**	priv_str	pointer to a buffer big enough to contain all privilege
**			names
**	lang		language of the query
** Output:
**	priv_str	string filled with comma-separated privilege names
**
** Returns:
**	none
**
** Exceptions:
**	none
**
** Side effects:
**	none
**
** History:
**	02-aug-91 (andre)
**	    written
**	03-oct-91 (andre)
**	    added lang to the interface
**	13-feb-92 (andre)
**	    added support for dbproc privileges (EXECUTE) and dbevent privileges
**	    (REGISTER and RAISE)
**	16-sep-92 (andre)
**	    privilege map is build using bitwise operators, so instead of
**	    accessing it using BTnext we will use bitwise AND
**	08-dec-92 (andre)
**	    DB_REFERENCES will be set in privs if REFERENCES is being granted
**	20-oct-93 (robf)
**          Add COPY_INTO, COPY_FROM privileges
*/  
VOID
psy_prvmap_to_str(
	i4	    privs,
	char	    *priv_str,
	DB_LANG	    lang)
{
    char	*s;

    *priv_str = EOS;
    
    if (privs & (DB_RETRIEVE | DB_TEST | DB_AGGREGATE))
    {
	if (*priv_str)
	    s = (lang == DB_SQL) ? ", SELECT" : ", RETRIEVE";
	else
	    s = (lang == DB_SQL) ? "SELECT" : "RETRIEVE";
	    
	privs &= ~((i4) (DB_RETRIEVE | DB_TEST | DB_AGGREGATE));
	STcat(priv_str, s);
    }

    if (privs & DB_REPLACE)
    {
	if (*priv_str)
	    s = (lang == DB_SQL) ? ", UPDATE" : ", REPLACE";
	else
	    s = (lang == DB_SQL) ? "UPDATE" : "REPLACE";

	privs &= ~((i4) DB_REPLACE);
	STcat(priv_str, s);
    }

    if (privs & DB_DELETE)
    {
	s = (*priv_str) ? ", DELETE" : "DELETE";
	privs &= ~((i4) DB_DELETE);
	STcat(priv_str, s);
    }

    if (privs & DB_APPEND)
    {
	if (*priv_str)
	    s = (lang == DB_SQL) ? ", INSERT" : ", APPEND";
	else
	    s = (lang == DB_SQL) ? "INSERT" : "APPEND";

	privs &= ~((i4) DB_APPEND);
	STcat(priv_str, s);
    }

    if (privs & DB_REFERENCES)
    {
	s = (*priv_str) ? ", REFERENCES" : "REFERENCES";
	privs &= ~((i4) DB_REFERENCES);
	STcat(priv_str, s);
    }

    if (privs & DB_EXECUTE)
    {
	s = (*priv_str) ? ", EXECUTE" : "EXECUTE";
	privs &= ~((i4) DB_EXECUTE);
	STcat(priv_str, s);
    }

    if (privs & DB_EVREGISTER)
    {
	s = (*priv_str) ? ", REGISTER" : "REGISTER";
	privs &= ~((i4) DB_EVREGISTER);
	STcat(priv_str, s);
    }

    if (privs & DB_EVRAISE)
    {
	s = (*priv_str) ? ", RAISE" : "RAISE";
	privs &= ~((i4) DB_EVRAISE);
	STcat(priv_str, s);
    }

    if (privs & DB_NEXT)
    {
	s = (*priv_str) ? ", NEXT" : "NEXT";
	privs &= ~((i4) DB_NEXT);
	STcat(priv_str, s);
    }

    if (privs & DB_COPY_INTO)
    {
	s = (*priv_str) ? ", COPY_INTO" : "COPY_INTO";
	privs &= ~((i4) DB_COPY_INTO);
	STcat(priv_str, s);
    }

    if (privs & DB_COPY_FROM)
    {
	s = (*priv_str) ? ", COPY_FROM" : "COPY_FROM";
	privs &= ~((i4) DB_COPY_FROM);
	STcat(priv_str, s);
    }

    /* All valid privileges should be checked BEFORE we get here */
    
    if (!*priv_str || privs & ~((i4) DB_GRANT_OPTION))
    {
	/* unknown privilege or no privilege at all was specified */
	s = (*priv_str) ? ", UNKNOWN" : "UNKNOWN";
	STcat(priv_str, s);
    }

    if (privs & DB_GRANT_OPTION)
    {
	STcat(priv_str, " WITH GRANT OPTION");
    }

    return;
}

/*
** Name: psy_init_rel_map - initialize a structure holding a map of relations.
**
** Description:
**	Initialize map of relations used in a query.  For now, the map consists
**	of a i4, but this may change
**
** Input:
**	map	pointer to a relation map to be initialized
**
** Output:
**	map	initialized relation map
**
** Side effects:
**	none
**
** Exceptions:
**	none
**
** Returns:
**	none
**
** History
**
**	23-aug-91 (andre)
**	    written
*/
VOID
psy_init_rel_map(
	PSQ_REL_MAP	*map)
{
    map->rel_map = 0L;
    return;
}

/*
** Name: psy_tbl_to_col_priv - translate table-wide privilege on a view into
**			       column-specific privilege on the underlying table
**
** Description:
**	Translate a table-wide privilege (UPDATE or INSERT) on a view into
**	column-specific privilege on the underlying table (function will have to
**	change when we add support for updateable multi-variable views)
**
** Input:
**	tbl_wide_privs		map of table-wide privileges
**	col_priv_map		map of column-specific privileges
**	colpriv_attr_map	attribute map to be filled in
**	priv			privilege being translated (DB_REPLACE or
**				DB_APPEND)
**	tree			root of the <query term> tree which may contain
**				references to columns of an underlying table;
**				in case of UNION view, this will be a query tree
**				for one of UNION'd <query term>s; otherwise,
**				this will be the query tree (this assumes that
**				some time in the future we will support updating
**				of UNION views)
**
** Output:
**	err_blk			will be filled in if an error is encountered
**
** Side effects:
**	none
**
** Exceptions:
**	none
**
** Returns:
**	E_DB_{OK, SEVERE}
**
** History:
**	27-aug-91 (andre)
**	    written
**	05-may-92 (andre)
**	    instead of receiving the view tree header, this function will
**	    receive the root of a tree representing a query term in which a
**	    specified table is used
*/
static DB_STATUS
psy_tbl_to_col_priv(
	i4		    *tbl_wide_privs,
	i4		    *col_priv_map,
	PSY_ATTMAP	    *colpriv_attr_map,
	i4		    priv,
	PST_QNODE	    *tree,
	DB_ERROR	    *err_blk)
{
    register PST_QNODE	    *node;
    i4		    err_code;

    /*
    ** first move privilege bit from table-wide privilege map into the
    ** column-specific privilege map
    */
    *tbl_wide_privs &= ~priv;
    *col_priv_map |= priv;

    psy_fill_attmap(colpriv_attr_map->map, ((i4) 0));

    /*
    ** now walk down the target list of the query term and record all updateable
    ** attributes of the relation over which the view is defined
    */
    for (node = tree->pst_left;
	 (node && node->pst_sym.pst_type != PST_TREE);
	 node = node->pst_left)
    {
	register PST_QNODE	*right_child;

	if (node->pst_sym.pst_type != PST_RESDOM)
	{
	    _VOID_ psf_error(E_PS0D0B_RESDOM_EXPECTED, 0L, PSF_INTERR,
		&err_code, err_blk, 0);
	    return(E_DB_SEVERE);
	}

	right_child = node->pst_right;

	/*
	** only need to record the updateable attributes
	** 6.4 contains an RDF fix which does not define
	** TID attribute for views, once it is in place
	** we can remove the second part of the below
	** test ("db_att_id != 0")
	*/
	
	if (right_child->pst_sym.pst_type == PST_VAR
	    &&
	    right_child->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id != 0)
	{
	    BTset(right_child->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
		(char *) colpriv_attr_map->map);
	}
    }

    /*
    ** if none of the view attributes is updateable, remove the privilege from
    ** the list of independent privileges which must be sought
    */
    if (BTnext(-1, (char *) colpriv_attr_map->map, DB_MAX_COLS + 1) < 0)
    {
	*col_priv_map &= ~priv;
    }

    return(E_DB_OK);
}

/*
** Name: psy_colpriv_xlate - translate column-specific privilege on a view into
**			     column-specific privilege on the underlying table
**
** Description:
**	Translate a column-specific privilege (UPDATE or INSERT) on a view into
**	a column-specific privilege on the underlying table (function will have
**	to change when we add support for updateable multi-variable views)
**
** Input:
**	tree			root of a tree representing a query term in
**				which a relation is being referenced
**	col_priv_map		map of column-specific privileges
**	colpriv_attr_map	attribute map to be filled in
**	priv			privilege being translated (DB_REPLACE or
**				DB_APPEND)
**	view_attr_count		number of attributes in the view
**
** Output:
**	err_blk			will be filled in if an error is encountered
**
** Side effects:
**	none
**
** Exceptions:
**	none
**
** Returns:
**	E_DB_{OK, SEVERE}
**
** History:
**	27-aug-91 (andre)
**	    written
**	21-jul-92 (andre)
**	    sometimes (if a user specified <privilege> EXCLUDING (<column
**	    list>)), bit 0 of the colpriv_attr_map could be set; since here we
**	    are translating privileges on column(s) of a view into privileges on
**	    column(s) of the underlying table, we should not be looking at the
**	    bit 0
*/
static DB_STATUS
psy_colpriv_xlate(
	PST_QNODE	    *tree,
	i4		    *col_priv_map,
	PSY_ATTMAP	    *colpriv_attr_map,
	i4		    priv,
	i4		    view_attr_count,
	DB_ERROR	    *err_blk)
{
    PSY_ATTMAP		    tmpmap;
    register i4	    cur_attr, next_attr;
    register PST_QNODE      *node;
    register char	    *map = (char *) colpriv_attr_map->map;

    psy_fill_attmap(tmpmap.map, ((i4) 0));

    /*
    ** to facilitate translation, we will reverse the
    ** attribute map (this will save us many unnecessary
    ** traversals of the view definition target list)
    **
    ** if the privilege was granted on a set of columns, only bits corresponding
    ** to those columns will be set;
    ** if, however, privilege was granted on a view EXCLUDING a set of columns,
    ** all bits in the attribute mnap except for those corresponding to the
    ** "excluded" columns will be set - since the view quite likely does not
    ** contain DB_MAX_COLS columns, this function may have hard time translating
    ** references to non-existent view attributes.  Accordingly, we will build a
    ** map of view attributes to which a privilege applies as a map of all
    ** attributes minus the set of "excluded" columns
    */
    if (BTtest(0, (char *) colpriv_attr_map->map))
    {
	/*
	** recall that bit 0 indicates that the privilege applies to the entire
	** table - we do not want to copy it
	*/
	for (cur_attr = 1; cur_attr <= view_attr_count; cur_attr++)
	{
	    if (BTtest(cur_attr, map))
	    {
		BTset(DB_MAX_COLS - cur_attr, (char *) tmpmap.map);
	    }
	}
    }
    else
    {
	for (cur_attr = 0;
	     (cur_attr = BTnext(cur_attr, map, DB_MAX_COLS + 1)) > -1;
	    )
	{
	    BTset(DB_MAX_COLS - cur_attr, (char *) tmpmap.map);
	}
    }

    /*
    ** now reinitialize the attribute map corresponding to the column-specific
    ** privilege
    */
    psy_fill_attmap(colpriv_attr_map->map, ((i4) 0));

    /*
    ** finally walk the query term tree and translate map of view attributes
    ** into a map of attributes of the underlying object used in the updateable
    ** attributes of the view
    */

    /*
    ** set cur_attr to the offset of the first possibly interesting bit
    */
    cur_attr = DB_MAX_COLS - view_attr_count - 1;
    
    for (node = tree;
	 (next_attr = BTnext(cur_attr, (char *) tmpmap.map, DB_MAX_COLS)) != -1;
	)
    {
	register PST_QNODE	*right_child;

	/*
	** get pointer to the subtree describing the
	** next attribute of the view
	*/
	for (; cur_attr < next_attr; cur_attr++)
	{
	    node = node->pst_left;

	    if (node->pst_sym.pst_type != PST_RESDOM)
	    {
		i4	    err_code;

		_VOID_ psf_error(E_PS0D0B_RESDOM_EXPECTED, 0L, PSF_INTERR,
		    &err_code, err_blk, 0);
		return(E_DB_SEVERE);
	    }
	}

	/*
	** only need to record the updateable attributes 6.4 contains an RDF fix
	** which does not define TID attribute foir views, once it is in place
	** we can remove the second part of the below test ("db_att_id != 0")
	*/

	right_child = node->pst_right;
	
	if (right_child->pst_sym.pst_type == PST_VAR &&
	    right_child->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id != 0)
	{
	    BTset(right_child->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
		(char *) colpriv_attr_map->map);
	}
    }

    /*
    ** if none of the view attributes is updateable,
    ** remove UPDATE from the list of independent
    ** privileges which must be located
    */
    if (BTnext(-1, (char *) colpriv_attr_map->map, DB_MAX_COLS + 1) < 0)
    {
	*col_priv_map &= ~priv;
    }

    return(E_DB_OK);
}

/*
** Name: psy_find_rgvar_ref - find reference to a given range variable in a tree
**
** Description:
**	Recursively (one day we'll do away with it) walk a tree looking for
**	references to attributes of a specified range variable
**
** Input:
**	t	    tree to be searched
**	rgno	    number of the range variable corresponding to the relation
**		    of interest
**
** Output:
**	none
**
** Returns:
**	TRUE	    if a reference was found
**	FALSE	    otherwise
**
** History:
**	27-aug-91 (andre)
**	    written
*/
static bool
psy_find_rgvar_ref(
	PST_QNODE       *t,
	i4             rgno)
{
    if (t != (PST_QNODE *) NULL)
    {
	if (t->pst_sym.pst_type == PST_VAR)
	{
	    if (t->pst_sym.pst_value.pst_s_var.pst_vno == rgno)
	    {
		return(TRUE);
	    }
	}
	else
	{
	    if (t->pst_left != (PST_QNODE *) NULL &&
		psy_find_rgvar_ref(t->pst_left, rgno))
	    {
		return(TRUE);
	    }

	    if (t->pst_right != (PST_QNODE *) NULL &&
		psy_find_rgvar_ref(t->pst_right, rgno))
	    {
		return(TRUE);
	    }
	}
    }

    return(FALSE);
}

/*
** Name: psy_attmap_to_str - produce a string of comma separated attribute names
**			     given a map of attributes and an array of attribute
**			     descriptors.
**
** Description:
**	Given array of attribute descriptors and an attribute map, produce a
**	comma-separated list of attribute names.  Caller will supply maximum
**	string length and a starting offset into the attribute map.  The latter
**	will be modified to point at the next bit in the attribute map enabling
**	us to communicate to the caller that there are no more attribute names
**	to print.
**
** Input:
**	attr_descr	array of pointers to attribute descriptors
**	attr_map	map of "interesting" attributes
**	offset		address of the offset into the attribute map of the
**			first interesting column
**	str		pointer to a string large enough to accomodate str_len
**			characters
**	str_len		maximum number of characters that can be stored in str
**
** Output:
**	offset		offset of the next "interesting" bit in the map if the
**			map has not been exhausted; -1 otherwise
**
** Exceptions:
**	none
**
** Side effects:
**	none
**
** Returns:
**	none
**
** History:
**	28-aug-91 (andre)
**	    written
*/
VOID
psy_attmap_to_str(
	DMT_ATT_ENTRY	    **attr_descr,
	PSY_ATTMAP	    *attr_map,
	register i4	    *offset,
	char		    *str,
	i4		    str_len)
{
    i4		len = 0;

    for (;;)
    {

	if (attr_descr[*offset]->att_nmlen + len + CMbytecnt(",") > str_len)
	{
	    /* no more names can be added to the string */
	    break;
	}

	cui_move(attr_descr[*offset]->att_nmlen,
	    attr_descr[*offset]->att_nmstr, ' ',
	    attr_descr[*offset]->att_nmlen, (PTR) (str + len));
	len += attr_descr[*offset]->att_nmlen;
	CMcpychar(",", (str + len));
	len += CMbytecnt(",");

	*offset = BTnext(*offset, (char *) attr_map->map, DB_MAX_COLS + 1);

	/*
	** we will follow a comma with a blank unless this is the last attribute
	** name in the list
	*/
	if (*offset != -1)
	{
	    if (len + CMbytecnt(" ") > str_len)
	    {
		break;
	    }

	    CMcpychar(" ", (str + len));
	    len += CMbytecnt(" ");
	}
	else
	{
	    /*
	    ** no more attributes, forget about the last comma and break out of
	    ** the loop
	    */
	    len -= CMbytecnt(",");
	    break;
	}
    }

    /* NULL terminate the string for printing */
    str[len] = EOS;

    return;
}

/*
** Name:    psy_mark_sublist - insert elements marking beginning of a sublist
**			       into the independent object lists
**
** Description:
**	Mark the beginning of independent object sublists by inserting a header
**	element of specified type
**
** Input:
**	indep_objs		    structure containing addresses of the
**				    independent object, object privilege, and
**				    column-specific privilege lists
**	mstream			    stream to be used for allocation of header
**				    elements
**	hdrtype			    type of header element
**	    PSQ_DBPLIST_HEADER	    header of a list of independent objects for
**				    a given dbproc
**	    PSQ_INFOLIST_HEADER	    header of a list of independent objects
**				    which was constructed during an unsuccessful
**				    attempt at reparsing a dbproc
**	dbpname			    dbproc name
**	dbpid			    id of a dbproc responsible for construction
**				    of a sublist
** Output:
**	err_blk			    filled in if an error is encountered
**
** Side effects:
**	allocates memory
**
** Exceptions:
**	none
**
** Returns
**	E_DB_{OK, ERROR}
**
** History
**
**	07-oct-91 (andre)
**	    written
**	10-oct-91 (andre)
**	    when allocating header elements, avoid allocating space for
**	    DB_DBP_NAME which will most certainly not be used.
**	15-oct-91 (andre)
**	    allow for creation of DBPLIST headers even if there are no elements
**	    in the list (this applies to the object list as well as to the
**	    privilege lists)
**	1-may-92 (andre)
**	    independent object list header will contain dbproc name along with
**	    ID - this will make it easier to produce useful output with trace
**	    point ps133
*/
static DB_STATUS
psy_mark_sublist(
	PSS_SESBLK		*sess_cb,
	PSQ_INDEP_OBJECTS	*indep_objs,
	PSF_MSTREAM		*mstream,
	i4			hdrtype,
	DB_TAB_ID		*dbpid,
	DB_DBP_NAME		*dbpname,
	DB_ERROR		*err_blk)
{
    i4			    sublist_hdr;
    DB_STATUS		    status;

    sublist_hdr = PSQ_DBPLIST_HEADER | PSQ_INFOLIST_HEADER;

    /*
    ** do not insert an INFOLIST header unless the first element in the list is
    ** an object descriptor
    */
    if (   hdrtype != PSQ_INFOLIST_HEADER
	|| (indep_objs->psq_objs
	    && !(indep_objs->psq_objs->psq_objtype & sublist_hdr))
       )
    {
	PSQ_OBJ	    *hdr;
	
	status = psf_malloc(sess_cb, mstream, (i4) sizeof(PSQ_OBJ), (PTR *) &hdr, 
	    err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	hdr->psq_objtype = hdrtype;
	hdr->psq_num_ids = 1;
	hdr->psq_objid->db_tab_base = dbpid->db_tab_base;
	hdr->psq_objid->db_tab_index = dbpid->db_tab_index;
	STRUCT_ASSIGN_MACRO((*dbpname), hdr->psq_dbp_name);

	hdr->psq_next = indep_objs->psq_objs;
	indep_objs->psq_objs = hdr;
    }

    /*
    ** do not insert an INFOLIST header unless the first element in the list is
    ** an object privilege descriptor
    */
    if (   hdrtype != PSQ_INFOLIST_HEADER
	|| (indep_objs->psq_objprivs
	    && !(indep_objs->psq_objprivs->psq_objtype & sublist_hdr))
       )
    {
	status = psy_insert_objpriv(sess_cb, dbpid, hdrtype, (i4) 0, mstream,
	    &indep_objs->psq_objprivs, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /*
    ** do not insert an INFOLIST header unless the first element in the list is
    ** a column-specific privilege descriptor
    */
    if (   hdrtype != PSQ_INFOLIST_HEADER
	|| (indep_objs->psq_colprivs
	    && !(indep_objs->psq_colprivs->psq_objtype & sublist_hdr))
       )
    {
	status = psy_insert_colpriv(sess_cb, dbpid, hdrtype, (i4 *) NULL, (i4) 0,
	    mstream, &indep_objs->psq_colprivs, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    return(E_DB_OK);
}

/*
** Name:    psy_dbp_ev_seq_perm - complete processing to insert tuples representing
**			      permits on the current database procedure or
**			      dbevent.
**
** Description:
**	This function will populate fields in RDF_CB and DB_PROTECTION that need
**	to be reinitialized for each object and/or grantee and will complete
**	building IIQRYTEXT text for this object and for each grantee named in
**	the statement.  That done, it will invoke RDF_UPDATE to actually insert
**	tuple(s) into appropriate catalogs
**
** Input:
**	rdf_cb			RDF control block
**	    rdf_rb		RDF request block
**		rdr_querytxt	beginning of IIQRYTEXT template
**	psy_cb			PSY control block
**	    psy_grant		type of object on which privileges are being
**				granted
**		PSY_PGRANT	granting dbproc privileges
**		PSY_EVGRANT	granting dbevent privileges
**	    psy_noncol_qlen	length of IIQRYTEXT template;
**				note that the text passed to QEU is likely to be
**				shorter since we trim trailing blanks off object
**				and user names
**	    psy_noncol_obj_name_off
**				offset into IIQRYTEXT template where
**				'owner'.obj_name is to be written
**	    psy_noncol_grantee_off
**				offset into the original template where
**				'grantee_name' is to be written
**	    psy_opmap		map of privileges being granted
**		DB_GRANT_OPTION	privileges are being granted WGO
**	    psy_user		if grant_to_public, default grantee name will be
**				found here
**	    psy_usrq		if granting privilege(s) to groups, roles, or
**				individual users (possibly in addition to
**				PUBLIC) this will point to a list of grantees
**	psy_tbl			entry describing the object permits on which are
**				being granted
**	    psy_tabnm		name of the dbproc/dbevent privilege(s) on
**				which are being granted
**	    psy_owner		owner of the dbproc/dbevent privilege(s) on
**				which are being granted
**	    psy_tabid		id of the dbproc/dbevent privilege(s) on which
**				are being granted
**	protup			DB_PROTECTION tuple to be completed and passed
**				to QEU
**	buf			text to be inserted between the name of the
**				object privilege(s) on which is/are being
**				granted and the grantee name
**	buf_len			length of text in buf
**
** Outputs:
**	psy_cb
**	    psy_error		filled in if an error occurs
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	one or more tuples will be inserted into IIPROTECT and IIQRYTEXT
**
** History:
**
**	03-mar-92 (andre)
**	    written (or, rather, plagiarized from psy_dgrant())
**	19-jul-92 (andre)
**	    PUBLIC can now be specified along with a list of user authorization
**	    identifiers, thus we will no longer be able to treat processing
**	    permits to PUBLIC and to auth ids as mutually exclusive
**	10-may-02 (inkdo01)
**	    Add support for sequence privileges.
*/
static DB_STATUS
psy_dbp_ev_seq_perm(
	RDF_CB		*rdf_cb,
	PSY_CB		*psy_cb,
	PSY_TBL		*psy_tbl,
	DB_PROTECTION	*protup,
	char		*buf,
	i4		buf_len)
{
    register RDR_RB	*rdf_rb = &rdf_cb->rdf_rb;
    i4			obj_name_len, grantee_name_len, schema_name_len;
    char		*ch;
    PSY_USR		*psy_usr;
    i4			text_len_without_grantee;
    char		*grantee_start;
    DB_STATUS		status;
    bool		grant_to_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;
    char		*grntee_type_start;
    u_char		delim_schema_name[DB_MAX_DELIMID];
    u_char		delim_obj_name[DB_MAX_DELIMID];
    u_char              delim_grantee_name[DB_MAX_DELIMID];
    u_char		*schema_name, *obj_name, *grantee_name;

    if (psy_cb->psy_grant == PSY_PGRANT)
    {
	DB_DBP_NAME	    *dbp_name = (DB_DBP_NAME *) &psy_tbl->psy_tabnm;

	STRUCT_ASSIGN_MACRO(*dbp_name, rdf_rb->rdr_name.rdr_prcname);
    }
    else if (psy_cb->psy_grant == PSY_EVGRANT)
    {
	DB_EVENT_NAME	*ev_name = (DB_EVENT_NAME *) &psy_tbl->psy_tabnm;

	STRUCT_ASSIGN_MACRO(*ev_name, rdf_rb->rdr_name.rdr_evname);
    }
    else if (psy_cb->psy_grant == PSY_SQGRANT)
    {
	DB_NAME		*seq_name = (DB_NAME *) &psy_tbl->psy_tabnm;

	STRUCT_ASSIGN_MACRO(*seq_name, rdf_rb->rdr_name.rdr_seqname);
    }
    else
    {
	/* this should never happen */
	psy_cb->psy_error.err_code = E_PS0002_INTERNAL_ERROR;
	return(E_DB_ERROR);
    }

    /*
    ** save owner name for RDF - note that it may be different from the
    ** name of the current user
    */
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, rdf_rb->rdr_owner);
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_rb->rdr_tabid);
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, protup->dbp_tabid);

    /* Initialize object name field in the iiprotect tuple */
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm, protup->dbp_obname);

    /* store the name of the object owner (schema to which an object belongs) */
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, protup->dbp_obown);

    /* 
    ** determine whether the schema name will be represented by a regular or 
    ** delimited identifier, compute that identifier and its length 
    **
    ** schema name will be represented by a regular identifier if the user has
    ** specified it using a regular identifier - otherwise it will be 
    ** represented by a delimited identifier
    */
    schema_name = (u_char *) &psy_tbl->psy_owner;
    schema_name_len = 
	(i4) psf_trmwhite(sizeof(psy_tbl->psy_owner), (char *) schema_name);
    if (~psy_tbl->psy_mask & PSY_REGID_SCHEMA_NAME)
    {
	status = psl_norm_id_2_delim_id(&schema_name, &schema_name_len,
	    delim_schema_name, &psy_cb->psy_error);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* 
    ** determine whether the object name will be represented by a regular or 
    ** delimited identifier, compute that identifier and its length 
    **
    ** object name will be represented by a regular identifier if the user has
    ** specified it using a regular identifier - otherwise it will be 
    ** represented by a delimited identifier
    */
    obj_name = (u_char *) &psy_tbl->psy_tabnm;
    obj_name_len = 
	(i4) psf_trmwhite(sizeof(psy_tbl->psy_tabnm), (char *) obj_name);
    if (~psy_tbl->psy_mask & PSY_REGID_OBJ_NAME)
    {
	status = psl_norm_id_2_delim_id(&obj_name, &obj_name_len,
	    delim_obj_name, &psy_cb->psy_error);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /*
    ** Do the name substitution for schema.dbproc/event/sequence in the 
    ** iiqrytext tuple template 
    */
    ch = (char *) rdf_rb->rdr_querytext + psy_cb->psy_noncol_obj_name_off;
    MEcopy((PTR) schema_name, schema_name_len, (PTR) ch);
    ch += schema_name_len;
    CMcpychar(".", ch);
    CMnext(ch);
    MEcopy((PTR) obj_name, obj_name_len, (PTR) ch);
    grntee_type_start = ch + obj_name_len;

    if (!grant_to_public)
    {
	/*
	** if PUBLIC was not specified, text which must appear between the
	** object name and the grantee name will not change so copy it here 
	*/
	MEcopy((PTR) buf, buf_len, (PTR) grntee_type_start);
    }

    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

    /*
    ** if user specified grantee(s) other than PUBLIC, compute length of query
    ** text w/o grantee name + compute where grantee name will start
    */
    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
    {
	/*
	** the template contains a grantee name placeholder, a schema name 
	** placeholder, and an object name placeholder, each of length 
	** DB_MAX_DELIMID.  Since the actual schema name, object name and 
	** grantee name are likely to be of different length, adjust
	** length of IIQRYTEXT tuple to reflect lengths of the actual schema
	** name and object name.  We will account for the length of
	** grantee name once we start cycling through the grantee list.
	*/
	text_len_without_grantee =
	    psy_cb->psy_noncol_qlen - 3 * DB_MAX_DELIMID + schema_name_len +
	    obj_name_len;

	/*
	** Compute where the grantee name will be inserted into the query text.
	** The address of the beginning of grantee name needs to be adjusted to
	** take into account the length of the actual schema name and object 
	** name
	*/

	grantee_start = (char *) rdf_rb->rdr_querytext +
	    psy_cb->psy_noncol_grantee_off - 2 * DB_MAX_DELIMID + 
	    schema_name_len + obj_name_len;
    }

    do
    {
	/* Get name of user getting privileges */
	if (!grant_to_public)
	{
	    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);

	    /* 
	    ** determine whether the grantee name will be represented by a 
	    ** regular or delimited identifier, compute that identifier and 
	    ** its length 
	    **
	    ** grantee name will be represented by a regular identifier if the 
	    ** user has specified it using a regular identifier - otherwise it 
	    ** will be represented by a delimited identifier
	    */
	    grantee_name = (u_char *) &psy_usr->psy_usrnm;
	    grantee_name_len = (i4) psf_trmwhite(sizeof(psy_usr->psy_usrnm), 
		(char *) grantee_name);
	    if (~psy_usr->psy_usr_flags & PSY_REGID_USRSPEC)
	    {
		status = psl_norm_id_2_delim_id(&grantee_name, 
		    &grantee_name_len, delim_grantee_name, &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }

	    /*
	    ** text_len_without_grantee contains length of query text
	    ** without grantee name.  Now that we know the grantee name
	    ** length, we can compute the query text length
	    */

	    rdf_rb->rdr_l_querytext =
		text_len_without_grantee + grantee_name_len;

	    /* Do the user name substitution in the iiqrytext. */
	    ch = grantee_start;
	    MEcopy((PTR) grantee_name, grantee_name_len, (PTR) ch);
	    ch += grantee_name_len;
	}
	else
	{
	    /* PUBLIC was named as a grantee */
	    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);

	    protup->dbp_gtype = DBGR_PUBLIC;
	    MEcopy((PTR) " to public", (sizeof(" to public") - 1),
		(PTR) grntee_type_start);
	    /*
	    ** the template contains a schema name placeholder, an object name 
	    ** placeholder, and a grantee name placeholder (all of length
	    ** DB_MAX_DELIMID).
	    **
	    ** Furthermore, it contains grantee type string specified by the
	    ** user (i.e. " to " or " to user "), which we replace with " to" .
	    ** Since the actual object names and object owner names are likely
	    ** to be of different length, and the grantee type string could be
	    ** different from the one we actualy used, adjust length of
	    ** IIQRYTEXT tuple to reflect length of the actual object owner name
	    ** and object name and a possible difference between the specified
	    ** grantee type string and the one that we used.
	    */

	    rdf_rb->rdr_l_querytext = psy_cb->psy_noncol_qlen -
		3 * DB_MAX_DELIMID + schema_name_len + obj_name_len -
		buf_len + sizeof(" to public") - 1;
	}

	if (psy_cb->psy_opmap & DB_GRANT_OPTION)
	{
	    /*
	    ** if granting to PUBLIC, ch points to the beginning of
	    ** table name; otherwise, it already points to a character
	    ** immediately following the grantee name
	    */
	    if (grant_to_public)
	    {
		ch += obj_name_len + sizeof(" to public") - 1;
	    }

	    MEcopy((PTR) " with grant option ",
		sizeof(" with grant option ") - 1, (PTR) ch);
	}

	status = rdf_call(RDF_UPDATE, (PTR) rdf_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    if (rdf_cb->rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
	    {
		i4		err_code;

		_VOID_ psf_error(E_PS0905_DBP_NOTFOUND, 0L,
		    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
		    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
			(char *) &psy_tbl->psy_tabnm),
		    &psy_tbl->psy_tabnm);
	    }
	    else
	    {
		/* Event user errors taken care of in QEF */
		_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb->rdf_error,
		    &psy_cb->psy_error);
	    }

	    return(status);
	}

	/*
	** if we just granted privilege(s) to PUBLIC, dbp_gtype in
	** IIPROCEDURE tuple and the IIQRYTEXT template needs to be modified
	** if some "real" grantees were named
	*/
	if (grant_to_public)
	{
	    protup->dbp_gtype = psy_cb->psy_gtype;

	    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
	    {
		/* some grantee(s) were named in addition to PUBLIC */
		grant_to_public = FALSE;

		/*
		** now copy the text which must appear between the object name
		** and the user name
		*/
		MEcopy((PTR) buf, buf_len, (PTR) grntee_type_start);
	    }
	}
	else
	{
	    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
	}

    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);

    return(E_DB_OK);
}

/*
** Name: psy_recursive_chain - determine if the dbproc invocation represents a
**			       recursive call and build/massage lists
**			       representing recursive call chains
**
** Description:
**	Starting at the current node in the list of dbproc headers ("cur_node"),
**	search its ancestors to determine if the dbproc being considered is
**	called recursively. If so, dbproc headers asociated with dbprocs
**	involved in the recursive call chain will be linked together (if they
**	are not already linked.)  This is done to facilitate determining when we
**	can claim that a dbproc involved in a recursive call chain is, in fact,
**	active or grantable.
**
**	Note that within a "family" of headers for the dbproc named in the GRANT
**	statement and all dbprocs invoked (directly or indirectly) by it, there
**	may be multiple recursive chains some of which may be disjoint while
**	others may intersect or contain each other.  All nodes (inclusively)
**	lying between the "cur_node" and the highest node (i.e. such node that
**	all other nodes in the recursive call chain are its descendants) H in
**	this recursive call chain must be linked together for the purposes of
**	determining whether dbprocs with which they are associated are
**	grantable.  A case when the newly discovered recursion chain is disjoint
**	from any other recursion chain discovered so far is uninteresting (we
**	will simply link together all nodes between "cur_node" and H.)  If two
**	chains are not disjoint, we will merge them to produce a single chain
**	with its root node being the highest of root nodes of chains being
**	merged.
**
** Input:
**	cur_node		dbproc header associated with a dbproc whoich
**				invokes a dbproc for which we are trying to
**				determine if is called recursively
**	dbpid			id of the dbproc for which we are trying to
**				determine if is called recursively
**
** Output:
**	none
**
** Side effects:
**	if the dbproc is being called recursively, will manipulate recurisive
**	call chain list(s)
**
** Returns:
**	TRUE if the dbproc is being called recursively; FALSE otherwise
** 
** History:
**
**	05-mar-92 (andre)
**	    written
*/
static bool
psy_recursive_chain(
	PSS_DBPLIST_HEADER      *cur_node,
	register DB_TAB_ID	*dbpid)
{
    register PSS_DBPLIST_HEADER     *h, *root;

    /*
    ** check if this dbproc is a part of a recursive call chain (e.g. P1 calls
    ** P2 calls P1 or, more generally, P1 calls P2 calls P3 ... calls P1).
    ** If that is the case, we would be asking for an infinite loop if we were
    ** to pass this dbproc to psy_dbp_priv_check();
    ** (there is additional discussion of what we do with recursive chains in
    ** the comments for PSS_DBPLIST_HEADER in pshparse.h)
    */

    for (h = cur_node; h; h = h->pss_parent)
    {
	if (   h->pss_obj_list->psq_objid->db_tab_base == dbpid->db_tab_base
	    && h->pss_obj_list->psq_objid->db_tab_index == dbpid->db_tab_index
	   )
	{
	    /*
	    ** dbproc with which "cur_node" is associated is involved in a
	    ** recursive call chain. 
	    */
	    break;
	}
    }
    
    if (!h)
	return(FALSE);

    root = h;	/* remember root of this recursive call chain */

    /*
    ** link together all elements of the newly discovered recursive call chain;
    ** if the chain intersects any existing chains, merge them
    */
    
    for (h = cur_node; h != root; h = h->pss_parent)
    {
	/*
	** if some node N's parent points at N (through pss_rec_chain), then
	** we may conclude that the chain being constructed intersects some
	** existing chain and we should proceeed to N's parent
	*/
	if (h->pss_parent->pss_rec_chain == h)
	{
	    continue;
	}

	/* remember that this node belongs to a recursive call chain */
	h->pss_flags |= PSS_RC_MEMBER;

	if (!h->pss_rec_chain)
	{
	    /*
	    ** node is not pointing to any other members of existing recursive
	    ** call chains (actually, we can conclude that it has not been a
	    ** part of a recursive chain until now since its parent is not
	    ** pointing at it and it does not point at any other node) - build a
	    ** link to it from its parent;
	    ** if the parent is already a part of a recursive chain, we will
	    ** simply insert this node into the parent's list
	    */
	    h->pss_rec_chain = h->pss_parent->pss_rec_chain;
	    h->pss_parent->pss_rec_chain = h;
	}
	else
	{
	    /*
	    ** this node is the highest node inserted into the recursive chain
	    ** so far, but the root of the new chain is an ancestor of this
	    ** node - make sure this node is not marked as a root
	    */
	    h->pss_flags &= ~PSS_RC_ROOT;

	    /*
	    ** if the parent of this node does not point to any members of
	    ** existing call chain
	    **   make parent point at this node
	    ** else if parent of this node points at a node other than this node
	    **   // i.e. if it is pointing at this node's sibling
	    **	 we need to merge two chains
	    ** endif
	    **	 
	    ** // ACHTUNG, VNIMANIYE (that's "attention" in Russian):
	    ** // Given any chain, the following must hold:
	    ** //   1) root of the chain must be the ancestor of all elements
	    ** //      of the chain and
	    ** //   2) Node N1 may be pointed at (through recursive chain
	    ** //      pointer) by a node N2 which is not N1's parent only if
	    ** //      we have finished processing the subtree rooted in N1.
	    ** // (1) must hold to enable us to know at which point we may
	    ** // claim that elements of a given recursive call chain are all
	    ** // active or grantable since we can claim to have processed all
	    ** // dbprocs in a subtree only when we finally reach the
	    ** // subtree's root.
	    ** 
	    ** // The argument for making (2) an invariant goes something like
	    ** // this:
	    ** //   Suppose N1's parent (P) is pointing at N2, which is N1's
	    ** //   sibling, and N2 is pointing at N1.  If some descendant
	    ** //   of N1 recursively invokes P or P's ancestor, establishing
	    ** //   that P and N1 belong to the same chain would require
	    ** //   scanning the entire chain since (1) is the only other
	    ** //   invariant that holds about chains.  Since I am trying to
	    ** //   make the process as efficient as I can, I prefer to keep
	    ** //   cost of determining whether a given node and its root
	    ** //   belong to the same chain constant (O(1).)
	    **	 
	    ** Note that as a result of merging two or more chains, we may have
	    ** lower level nodes in one subtree point at higher level nodes in
	    ** another subtree, but we will never have descendants point at
	    ** their ancestors.
	    */
	    
	    if (!h->pss_parent->pss_rec_chain)
	    {
		h->pss_parent->pss_rec_chain = h;
	    }
	    else
	    {
		/*
		** the node and its parent are members of different recursive
		** chains (see invariant (2) above).  To merge them, we will
		** append node pointed to by "cur_node" to the end of the chain
		** rooted in the parent
		*/
		register PSS_DBPLIST_HEADER     *tmp;

		for (tmp = h->pss_parent->pss_rec_chain;
		     tmp->pss_rec_chain != (PSS_DBPLIST_HEADER *) NULL;
		     tmp = tmp->pss_rec_chain)
		;

		/*
		** tmp now points at the last node in the chain to which
		** h->pss_parent belongs
		*/

		tmp->pss_rec_chain = cur_node->pss_rec_chain;

		/* make cur_node->pss_rec_chain point at the merged chain */
		cur_node->pss_rec_chain = h->pss_parent->pss_rec_chain;

		/* h's parent will now point at h */
		h->pss_parent->pss_rec_chain = h;
	    }
	}
    }

    /*
    ** if the root of the chain we just built has no parent or if parent of the
    ** root of the chain we just built ("root") is not itself a part of a chain
    ** of which "root" is an element, we will mark "root" as a root (and member)
    ** of the chain; otherwise some ancestor of it is a root of the combined
    ** chain (and it is already marked as such.) 
    */
    if (!root->pss_parent || root->pss_parent->pss_rec_chain != root)
    {
	root->pss_flags |= (PSS_RC_ROOT | PSS_RC_MEMBER);
    }

    return(TRUE);
}

/*
** Name:    psy_dbp_status  - given a list of objects on which the dbproc
**			      depends, determine if its owner posesses
**			      privileges required for the specified operation
**
** Description:
**	This function may be invoked to verify that the user may grant
**	privilege(s) on a given dbproc owned by him (i.e. verify that the dbproc
**	is grantable) or to verify that the user may execute a dbproc owned by
**	him or to create a new dbproc (i.e. verify that the dbproc is active.) 
**
**	If the user is attempting to GRANT privilege(s) on his own dbproc, we
**	will verify that he may grant privileges required to execute every
**	statement of the dbproc.  This is fairly straightorward for privileges
**	on tables, views, dbevents, and dbprocs owned by other users.  For
**	dbprocs owned by the current user which are invoked (directly or
**	indirectly) by the dbproc on which privilege(s) are being granted,
**	however, this means establishing that they are "grantable".  In the best
**	case, this can be determined by examining IIPROCEDURE.db_mask[0].
**	Failing that, we will be forced to either reparse the dbproc (if no
**	independent object/privilege list can be found in IIDBDEPENDS/IIPRIV),
**	or to verify that the user may grant every privilege found in the
**	dbproc's independent privilege list + that every dbproc owned by the
**	current user which was found in the dbproc's independent object list is
**	grantable.
**
**	If user is attempting to execute a dbproc owned by him which is not
**	marked as active or to create a new dbproc, we must establish that he
**	posesses privileges required to execute every statement in that dbproc.
**	This is fairly straightorward for privileges on tables, views, dbevents,
**	and dbprocs owned by other users.  For dbprocs owned by the current user
**	which are invoked (directly or indirectly) by the dbproc which user is
**	attempting to create or execute, however, this means establishing that
**	they are at least "active".  In the best case, this can be determined by
**	examining IIPROCEDURE.db_mask[0].  Failing that, we will be forced to
**	either reparse the dbproc (if no independent object/privilege list can
**	be found in IIDBDEPENDS/IIPRIV), or to verify that the user posesses
**	every privilege found in the dbproc's independent privilege list + that
**	every dbproc owned by the current user which was found in the dbproc's
**	independent object list is active
**
**	Information obtained in the course of processing dbproc definitions
**	(i.e. the independent object/privilege lists) will be inserted in
**	IIDBDEPENDS and IIPRIV to avoid having to rediscover this information in
**	the future.  Information obtained in the course of processing dbprocs'
**	independent object/privilege lists (i.e. whether the dbproc is active or
**	grantable) will be entered into IIPROCEDURE to avoid having to
**	rediscover this information in the future.
**	
**	Special care will be exercised in dealing with recursive dbproc
**	invocations (expecially recursive call chains) to avoid getting stuck in
**	infinite loop or to incorrectly declare a given dbproc active or
**	grantable.  Comments in the body of this function describe problems
**	posed by recursive call chains and how we will go about dealing with
**	them.
**
** Input:
**	dbp_descr		description of the dbproc for which we need to
**				determine whether it is active and/or grantable
**	    psy_tabid		dbproc id
**	    psy_tabnm		dbproc name
**	sess_cb			PSF session block
**	    pss_indep_objs
**		psq_objs	if processing [re]CREATE PROCEDURE, will point
**				to the list of independent objects for the
**				dbproc being [re]created; NULL otherwise
**		psq_objprivs	if processing [re]CREATE PROCEDURE, will point
**				to the list of independent object-wide
**				privileges for the dbproc being [re]created;
**				NULL otherwise
**		psq_colprivs	if processing [re]CREATE PROCEDURE, will point
**				to the list of independent column-specific
**				privileges for the dbproc being [re]created;
**				NULL otherwise
**	grant_dbprocs		if qmode==PSQ_GRANT, will point to the first
**				element in the list of dbprocs mentioned in
**				GRANT statement; NULL otherwise
**	qmode			mode of the query beinbg processed
**	    PSQ_GRANT		GRANT ON PROCEDURE - must establish that the
**				dbproc is grantable
**	    PSQ_CREDBP		CREATE PROCEDURE - must establish that the
**				dbproc is at least active
**	    PSQ_EXEDBP		recreate dbproc for execution by its owner -
**				must establish that the dbproc is at least
**				active
**
** Output:
**	dbp_mask		mask describing whether a dbproc is active or
**				grantable
**	    0			we cannot guarantee that the user posesses
**				privilege(s) required for the operation
**	    DB_ACTIVE_DBP	we are [re]creating a dbproc and it is ACTIVE
**	    DB_ACTIVE_DBP |
**	    DB_DBPGRANT_OK	we are processing GRANT and user posesses all
**				requried privileges
**
**	err_blk			filled in if an error occurs
**
** Return:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	allocates memory, then releases it before exiting
**
** History:
**
**	19-mar-92 (andre)
**	    segregated from psy_dgrant() to enable reuse of the code when
**	    processing dbproc [re]creation.
**	03-may-92 (andre)
**	    Added support for trace point ps133.
**	19-may-92 (andre)
**	    removed psy_cb from the parameter list, added grant_dbprocs and
**	    err_blk;
**	    PSQ_CB will be allocated on stack and initialized before calling 
**	    psy_dbp_priv_check().
**	21-may-92 (andre)
**	    if we are processing GRANT ON PROCEDURE and the current dbproc, P1,
**	    was not marked as grantable in IIPROCEDURE but we have determined
**	    that it is, in fact, grantable, we must set PSY_DBPROC_IS_GRANTABLE
**	    bit in PST_TBL.psy_mask describing P1.  This is necessary since the
**	    user may have specified multiple dbprocs in this GRANT statement,
**	    and some subsequent dbproc, say P2, may involve P1.  If we fail to
**	    set PSY_DBPROC_IS_GRANTABLE in PST_TBL.psy_mask describing P1, we
**	    will mistakenly claim that P2 is ungrantable (because we build a
**	    list of dbprocs "known to be grantable/ungrantable" based on
**	    PSY_TBL entries for dbprocs which have already been processed)
**	25-jun-92 (andre)
**	    if there is an independent object/privilege list for the dbproc
**	    bneing [re]created, turn on DB_DBP_INDEP_LIST in dbp_mask[0]
**	26-jun-92 (andre)
**	    If IIPROCEDURE.dbp_mask1 indicated that the user may lack some
**	    required privileges on the dbproc but we have established that
**	    he/she posesses them, call RDF_UPDATE to modify
**	    IIPROCEDURE.dbp_mask1
**	09-sep-92 (andre)
**	    removed cache_flushed from the interface - psy_dbp_priv_check()
**	    always gets "fresh" cache entries
**	29-sep-92 (andre)
**	    RDR_2* masks over rdr_2types_mask got renamed to RDR2_*
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
**	30-jul-96 (inkdo01)
**	    Changed to resurrect "setinput" bit for db_mask[0] which got lost
**	    in all this other turmoil - needed for temptable proc parms.
**	19-june-06 (dougi)
**	    Same deal for DBP_DATA_CHANGE, used to validate BEFORE triggers.
**	29-march-2008 (dougi)
**	    And once more for ROW_PROC flag.
*/
DB_STATUS
psy_dbp_status(
	PSY_TBL		*dbp_descr,
	PSS_SESBLK	*sess_cb,
	PSF_QUEUE	*grant_dbprocs,
	i4		qmode,
	i4		*dbp_mask,
	DB_ERROR	*err_blk)
{
	    /*
	    ** remember whether the memory stream used to allocate
	    ** independent object/privilege lists needs to be closed before
	    ** exiting the function
	    */
    bool			close_stream = FALSE;

	    /*
	    ** will be reset to FALSE if we cannot guarantee that the user
	    ** posesses required privilege(s); if FALSE, consult
	    ** non_existant_dbp, missing_obj, and insuf_priv to determine the
	    ** reason.
	    */
    bool			satisfied = TRUE;
	    /*
	    ** TRUE if we psy_dbp_priv_check() could not look up a database
	    ** procedure
	    */ 
    bool			non_existant_dbp = FALSE;

	    /*
	    ** TRUE if during reparsing text of a dbproc, some object (i.e.
	    ** table,view,dbevent, or dbproc) was not found - dbproc is dormant
	    ** (very, very dormant)
	    */
    bool			missing_obj = FALSE;
	    /*
	    ** TRUE if it has been determined that the user lacks some required
	    ** privilege 
	    */
    bool			insuf_priv = FALSE;

	    /*
	    ** TRUE if unexpected error occurred while calling RDF_UPDATE to
	    ** modify IIPROCEDURE.dbp_mask1 to indicate that a dbproc is
	    ** grantable or active
	    */
    bool			rdf_upd_err = FALSE;

    PSS_DBPLIST_HEADER		*root_dbp = (PSS_DBPLIST_HEADER *) NULL;
    PSS_DBPLIST_HEADER		*cur_node;
    PSQ_OBJ			*cur_obj;
    DB_STATUS			status;
    i4			err_code;
    i4				loc_dbp_mask[2];
    i4				privs;
    bool			ps133;
	    /*
	    ** save_head and save_tail will be used to remember addresses of the
	    ** first and last element of independent object/privilege lists
	    ** passed in by the caller when processesing [re]CREATE PROCEDURE
	    */
    PSQ_INDEP_OBJECTS		save_head, save_tail;
    PSQ_OBJPRIV			*infolist = (PSQ_OBJPRIV *) NULL;
    PSF_MSTREAM			mem_stream;

    PSQ_CB			loc_psq_cb, *psq_cb = &loc_psq_cb;
    RDF_CB			rdf_cb;
    register RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    DB_PROCEDURE		proctuple, *dbp_tuple = &proctuple;
    PSQ_INDEP_OBJECTS		empty_list;

    /*
    ** if processing GRANT and determine that the dbproc which was NOT marked as
    ** grantable in IIPROCEDURE is, in fact, grantable, we must remember to set
    ** PSY_DBPROC_IS_GRANTABLE in psy_tbl->psy_mask for that dbproc;
    ** cur_grant_entry will point at at the PSY_TBL entry for the dbproc being
    ** processed
    */
    PSY_TBL			*cur_grant_entry = (PSY_TBL *) NULL;

    char		trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    {
	i4		val1, val2;

	/* determine if trace point ps133 has been set */
	ps133 = (bool) ult_check_macro(&sess_cb->pss_trace,
		    PSS_DBPROC_GRNTBL_ACTIVE_TRACE, &val1, &val2);
    }

    dbp_mask[0] = dbp_mask[1] = (i4) 0;

    /*
    ** initialize local copy of PSQ_CB:
    **	    we must ensure that psq_qlang == DB_SQL and psq_flags == 0
    */
    MEfill(sizeof(PSQ_CB), (u_char) '\0', (PTR) psq_cb);
    psq_cb->psq_qlang = DB_SQL;
    psq_cb->psq_error.err_code = E_PS0000_OK;
    
    if (qmode == PSQ_CREDBP || qmode == PSQ_EXEDBP)
    {
	/*
	** caller has built independent privilege/object list for the dbproc
	** being [re]created.  we must remember beginning and end of the list so
	** that we can restore it before exiting this function.  In case of
	** CREATE PROCEDURE, caller will pass the list to QEF to have
	** independent object/privilege lists entered into IIDBDEPENDS/IIPRIV.
	** In case of recreate, this info will be entered into
	** IIDBDEPENDS/IIPRIV if IIPROCEDURE.db_mask[0] indicates that it isn't
	** already there.
	*/
	STRUCT_ASSIGN_MACRO(sess_cb->pss_indep_objs, save_head);

	if (sess_cb->pss_indep_objs.psq_objs == (PSQ_OBJ *) NULL)
	{
	    save_tail.psq_objs = (PSQ_OBJ *) NULL;
	}
	else
	{
	    PSQ_OBJ         *obj;

	    /* find end of independent object list and save its address */
	    for (obj = sess_cb->pss_indep_objs.psq_objs;
		 obj->psq_next != (PSQ_OBJ *) NULL;
		 obj = obj->psq_next)
	    ;

	    save_tail.psq_objs = obj;
	}

	if (sess_cb->pss_indep_objs.psq_objprivs == (PSQ_OBJPRIV *) NULL)
	{
	    save_tail.psq_objprivs = (PSQ_OBJPRIV *) NULL;
	}
	else
	{
	    PSQ_OBJPRIV	    *obj_priv;

	    /*
	    ** find end of independent object privilege list and save its
	    ** address
	    */
	    for (obj_priv = sess_cb->pss_indep_objs.psq_objprivs;
		 obj_priv->psq_next != (PSQ_OBJPRIV *) NULL;
		 obj_priv = obj_priv->psq_next)
	    ;
	    
	    save_tail.psq_objprivs = obj_priv;
	}

	if (sess_cb->pss_indep_objs.psq_colprivs == (PSQ_COLPRIV *) NULL)
	{
	    save_tail.psq_colprivs = (PSQ_COLPRIV *) NULL;
	}
	else
	{
	    PSQ_COLPRIV	    *col_priv;
	    
	    /*
	    ** find end of independent column-specific privilege list and save
	    ** its address
	    */
	    for (col_priv = sess_cb->pss_indep_objs.psq_colprivs;
		 col_priv->psq_next != (PSQ_COLPRIV *) NULL;
		 col_priv = col_priv->psq_next)
	    ;

	    save_tail.psq_colprivs = col_priv;
	}
    }
    else	/* qmode == PSQ_GRANT */
    {
	/*
	** when processing [re]CREATE PROCEDURE, pss_dependencies_stream gets
	** open in pslsgram.yi and is still open; for GRANT ON PROCEDURE we will
	** open it here and remember to close it before returning
	** 
	** the stream which will be used to allocate elements of the independent
	** object list for dbprocs owned by the current user;
	**
	** as a part of processing GRANT ON PROCEDURE, this list will also
	** contain elements describing whether the user may grant privilege on
	** dbprocs preceeding the current one in the <grant_obj_list>
	*/

	status = psf_mopen(sess_cb, QSO_QTEXT_OBJ, &mem_stream, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    goto dbp_stat_exit;
	}

	/* remember to close the stream before exiting */
	close_stream = TRUE;

	/*
	** far out: we will allocate a stream descriptor using the stream
	** itself;
	** sess_cb->pss_dependencies_stream will point at this new descriptor so
	** that if an exception occurs, the exception handler will be able to
	** release the memory associated with the stream.
	*/
	status = psf_malloc(sess_cb, &mem_stream, sizeof(PSF_MSTREAM),
	    (PTR *) &sess_cb->pss_dependencies_stream, err_blk);
	if (DB_FAILURE_MACRO(status))
	    goto dbp_stat_exit;

	STRUCT_ASSIGN_MACRO(mem_stream, (*sess_cb->pss_dependencies_stream));
    }

    {
	DB_TAB_ID	    dummy;

	/*
	** we will now create a special privilege sublist which will be used to
	** store privilege descriptors for dbprocs owned by the current user
	** which were placed into the independent object list by
	** psy_dbp_priv_check()
	*/

	dummy.db_tab_base = dummy.db_tab_index = 0;

	status = psy_insert_objpriv(sess_cb, &dummy, PSQ_INFOLIST_HEADER, (i4) 0,
	    sess_cb->pss_dependencies_stream, &infolist, err_blk);
	if (DB_FAILURE_MACRO(status))
	    goto dbp_stat_exit;

	if (qmode == PSQ_CREDBP || qmode == PSQ_EXEDBP)
	{
	    /*
	    ** if processing [re]CREATE PROCEDURE, we need to insert sublist
	    ** headers in front of independent object/privilege lists which were
	    ** accumulated during processing of the dbproc being [re]created;
	    */

	    /*
	    ** infolist header will follow the last element (if any) of the
	    ** independent object privilege list accumulated during processing
	    ** of the dbproc being [re]created - this will enable psy_dbpperm()
	    ** take advantage of information about lack/presence of required
	    ** privileges on dbprocs owned by the current user which have been
	    ** processed so far
	    ** if object privilege list is not empty, infolist will follow the
	    ** last element passed in by the caller; otherwise, insert it at the
	    ** head of the list
	    */
	    if (save_tail.psq_objprivs)
		save_tail.psq_objprivs->psq_next = infolist;
	    else
		sess_cb->pss_indep_objs.psq_objprivs = infolist;

	    status = psy_mark_sublist(sess_cb, &sess_cb->pss_indep_objs,
		sess_cb->pss_dependencies_stream, PSQ_DBPLIST_HEADER,
		&dbp_descr->psy_tabid, (DB_DBP_NAME *) &dbp_descr->psy_tabnm,
		err_blk);

	    if (DB_FAILURE_MACRO(status))
		goto dbp_stat_exit;
	}
	else
	{
	    PSY_TBL	*elem;
	    DB_TAB_ID   *id = &dbp_descr->psy_tabid;

	    /*
	    ** in case of GRANT ON PROCEDURE, we expect the independent
	    ** object/privilege lists to be empty
	    */
	
	    /*
	    ** make sure that information in infolist is available to
	    ** psy_dbpperm() which consults privilege list to avoid looking in
	    ** IIPROTECT
	    */
	    sess_cb->pss_indep_objs.psq_objprivs = infolist;

	    /*
	    ** insert privilege descriptors for the dbprocs already processed
	    ** (if any) into the newly created special privilege sublist
	    */
	    for (elem = (PSY_TBL *) grant_dbprocs->q_next;
		 (   elem->psy_tabid.db_tab_base != id->db_tab_base
		  || elem->psy_tabid.db_tab_index != id->db_tab_index
		 );
		 elem = (PSY_TBL *) elem->queue.q_next
		)
	    {
		status = psy_insert_objpriv(sess_cb, &elem->psy_tabid,
		    (elem->psy_mask & PSY_DBPROC_IS_GRANTABLE)
			? PSQ_OBJTYPE_IS_DBPROC
			: PSQ_OBJTYPE_IS_DBPROC | PSQ_MISSING_PRIVILEGE,
		    (i4) DB_EXECUTE, sess_cb->pss_dependencies_stream,
		    &infolist->psq_next, err_blk);

		if (DB_FAILURE_MACRO(status))
		    goto dbp_stat_exit;
	    }

	    /*
	    ** remember the PSY_TBL entry describing the dbproc permit on
	    ** which is being granted;  if we discover that it is grantable,
	    ** we will set PSY_DBPROC_IS_GRANTABLE in psy_mask field
	    ** (see 21-may-92 (andre)) above
	    */
	    cur_grant_entry = elem;

	    /*
	    ** as discussed in the history section (30-sep-91), if
	    ** the user is trying to grant privileges on his own
	    ** dbproc, there are 3 ways of determining if he can do
	    ** it:
	    **  - look at IIPROCEDURE which may contain an indicator of whether
	    **    the dbproc is grantable,
	    **  - look at IIPRIV (which is yet to be defined) and if
	    **    there is a list of independent objects associated
	    **    with the dbproc, ensure that the user can grant
	    **    all of the appropriate privileges WGO, or
	    **  - reparse the dbproc to determine if it is grantable
	    **
	    ** Initially, psy_dbp_priv_check() will always opt for the
	    ** last (and the least elegant) method.
	    **
	    **	03-mar-92 (andre)
	    **	    psy_dbp_priv_check() is about to become smarter:
	    **	      - if the dbproc is marked as grantable
	    **	        (DB_DBPGRANT_OK), it will return an indicator
	    **		that the user posesses 	privileges required to
	    **		grant EXECUTE on this dbproc
	    **	      - else if the dbproc is marked as active
	    **	        (DB_ACTIVE_DBP), but is not marked as grantable,
	    **		it will copy the independent object/privilege
	    **		list from IIPRIV rather than reparse the dbproc
	    **	      - else if all else fails, we will be forced to
	    **	        reparse the dbproc
	    */

	    /* dbproc must at least appear grantable */
	    privs = (i4) (DB_EXECUTE | DB_GRANT_OPTION);

	    status = psy_dbp_priv_check(sess_cb, psq_cb,
		(DB_DBP_NAME *) &dbp_descr->psy_tabnm, id, &non_existant_dbp,
		&missing_obj, loc_dbp_mask, &privs, err_blk);
	    if (DB_FAILURE_MACRO(status))
		goto dbp_stat_exit;
	    else if (non_existant_dbp || missing_obj)
	    {
		/*
		** remember that we have not verified that the user posesses
		** required privilege
		*/
		satisfied = FALSE;
		goto dbp_stat_exit;
	    }
	    else if (!privs)
	    {
		insuf_priv = TRUE;
		satisfied = FALSE;
		goto dbp_stat_exit;
	    }
	    else if ((dbp_mask[0] = loc_dbp_mask[0]) & DB_DBPGRANT_OK)
	    {
		/*
		** IIPROCEDURE.db_mask[0] has DB_DBPGRANT_OK bit set -
		** dbproc is guaranteed to be grantable.
		*/
		goto dbp_stat_exit;
	    }
	}
    }

    /*
    ** if the dbproc is owned by the current user and we had to reparse it, we
    ** may need to determine if the user may grant required privilege (EXECUTE
    ** for [re]CREATE PROCEDURE] or EXECUTE WGO for GRANT) on his/her dbproc(s)
    ** invoked by this dbproc
    */
    status = psf_malloc(sess_cb, sess_cb->pss_dependencies_stream,
	sizeof(PSS_DBPLIST_HEADER), (PTR *) &root_dbp, err_blk);
    if (DB_FAILURE_MACRO(status))
	goto dbp_stat_exit;

    /*
    ** initialize internal header:
    **	header for a dbproc passed by the caller will never have a parent or
    **	siblings;
    **	children are unknown at this point;
    **	it is not a part of a recursive chain (not yet, anyway)
    **	if processing GRANT,
    **	  assume that the dbproc is active and grantable (i.e. set
    **	  DB_DBPGRANT_OK and DB_ACTIVE_DBP bits in pss_dbp_mask) + use info
    **	  provided by psy_dbp_priv_check() (in loc_dbp_mask[0]) to determine
    **	  whether to set DB_DBP_INDEP_LIST in pss_dbp_mask; 
    **	else
    **	  if (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
    **	    assume that it is active and grantable (i.e. set DB_DBPGRANT_OK and
    **	    DB_ACTIVE_DBP bits in pss_dbp_mask);
    **	  else
    **	    assume that it is active (i.e. set DB_ACTIVE_DBP bit in
    **	    pss_dbp_mask);
    **	  endif
    **	  use info in save_tail to determine whether to set DB_DBP_INDEP_LIST
    **	  in pss_dbp_mask;
    **	endif
    **  // whether or not a dbproc is grantable will be mostly relevant when
    **	// processing [re]create, since to succeed with GRANT it must be both
    **	// active and grantable
    **	
    **	Note that if we are processing [re]create procedure, and current dbproc
    **	invokes some other dbproc which is not known to be active, we will only
    **	verify that it is active which will make it impossible to claim that the
    **	parent dbproc is grantable - eventually, I may start allowing mixing of
    **	grantable and non-grantable privileges in the independent privilege
    **	list.
    */
    root_dbp->pss_parent = root_dbp->pss_child =
	root_dbp->pss_sibling = root_dbp->pss_rec_chain =
	    (PSS_DBPLIST_HEADER *) NULL;
    root_dbp->pss_flags = 0;
    root_dbp->pss_obj_list = sess_cb->pss_indep_objs.psq_objs;
    root_dbp->pss_next = (PSS_DBPLIST_HEADER *) NULL;
    if (qmode == PSQ_GRANT)
    {
	root_dbp->pss_dbp_mask =
	    (i4) (DB_DBPGRANT_OK | DB_ACTIVE_DBP) | loc_dbp_mask[0];
    }
    else
    {
	root_dbp->pss_dbp_mask = (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
	    ? (DB_DBPGRANT_OK | DB_ACTIVE_DBP) : DB_ACTIVE_DBP;

	/*
	** if processing [re]CREATE PROCEDURE and there are some independent
	** objects/privileges for this dbproc, set DB_DBP_INDEP_LIST bit since
	** they have already been entered into IIPRIV/IIDBDEPENDS by psy_cproc()
	** or psq_recreate()
	*/
	if (   save_tail.psq_objs
	    || save_tail.psq_objprivs
	    || save_tail.psq_colprivs
	   )
	{
	    root_dbp->pss_dbp_mask |= DB_DBP_INDEP_LIST;
	}

	/* Also add in SETINPUT if necessary */
	if (sess_cb->pss_dbp_flags & PSS_SET_INPUT_PARAM) 
	 root_dbp->pss_dbp_mask |= DBP_SETINPUT;

	/* And DATA_CHANGE, too. */
	if (sess_cb->pss_dbp_flags & PSS_DATA_CHANGE)
	 root_dbp->pss_dbp_mask |= DBP_DATA_CHANGE;

	/* And OUT_PARMS, too. */
	if (sess_cb->pss_dbp_flags & PSS_OUT_PARMS)
	 root_dbp->pss_dbp_mask |= DBP_OUT_PARMS;

	/* And TX_STMT, too. */
	if (sess_cb->pss_dbp_flags & PSS_TX_STMT) 
	 root_dbp->pss_dbp_mask |= DBP_TX_STMT;

	/* And ROW_PROC, too. */
	if (sess_cb->pss_dbp_flags & PSS_ROW_PROC) 
	 root_dbp->pss_dbp_mask |= DBP_ROW_PROC;
    }

    /*
    ** Initialize RDF_CB which (we hope) will be used to update status of
    ** dbproc(s) being processed
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    rdf_rb->rdr_update_op   = RDR_REPLACE;
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE;
    rdf_rb->rdr_2types_mask = RDR2_DBP_STATUS;
    rdf_rb->rdr_status      = DB_SQL;
    rdf_rb->rdr_qrytuple    = (PTR) dbp_tuple;
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dbp_tuple->db_owner);
    empty_list.psq_objs     = (PSQ_OBJ *) NULL;
    empty_list.psq_objprivs = (PSQ_OBJPRIV *) NULL;
    empty_list.psq_colprivs = (PSQ_COLPRIV *) NULL;
    sess_cb->pss_indep_objs.psq_grantee =
	empty_list.psq_grantee  = &sess_cb->pss_user;
    rdf_rb->rdr_indep       = (PTR) &empty_list;
    
    for (cur_node = root_dbp; cur_node;)
    {
	i4	end_of_sublist = PSQ_INFOLIST_HEADER | PSQ_DBPLIST_HEADER;
	char	*active_or_grantable;

	if (ps133)
	{
	    active_or_grantable = (qmode == PSQ_GRANT) ? "grantable" : "active";
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\nWill verify that database procedures (if any) \
owned by the current user\ninvoked by the database procedure '%t' are %t\
\n>>>\n---",
	        psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) &cur_node->pss_obj_list->psq_dbp_name), 
		&cur_node->pss_obj_list->psq_dbp_name,
		STlength(active_or_grantable), active_or_grantable);
	}

	/*
	** find the next dbproc in the current sublist - first
	** object is a PSQ_DBPLIST_HEADER
	*/
	for (cur_obj = cur_node->pss_obj_list->psq_next;
	     (cur_obj && !(cur_obj->psq_objtype & end_of_sublist));
	     cur_obj = cur_obj->psq_next
	    )
	{
	    PSQ_OBJPRIV	    *p;

	    if (~cur_obj->psq_objtype & PSQ_OBJTYPE_IS_DBPROC)
	    {
		/* only consider entries describing dbprocs */
		continue;
	    }
	    else if (qmode == PSQ_GRANT)
	    {
		/*
		** if we are processing GRANT ON PROCEDURE, we are interested in
		** dbprocs which were not known to be grantable when they were
		** entered into the independent object list
		*/
		if (cur_obj->psq_objtype & PSQ_GRANTABLE_DBPROC)
		{
		    if (ps133)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nDatabase procedure %t invoked by %t \
was marked as grantable in IIPROCEDURE.\nWill skip checking this database \
procedure\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_obj->psq_dbp_name),
			    &cur_obj->psq_dbp_name,
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name);
		    }
		    continue;
		}
	    }
	    else
	    {
		/*
		** if we are [re]creating a dbproc, we are interested in dbprocs
		** which were not known to be active when they were entered into
		** the independent object list
		*/
		if (cur_obj->psq_objtype & PSQ_ACTIVE_DBPROC)
		{
		    if (ps133)
		    {
			char	*c =
			    (cur_obj->psq_objtype & PSQ_GRANTABLE_DBPROC)
				? "active and grantable" : "active";

			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nDatabase procedure %t invoked by %t\n\
was marked as %t in IIPROCEDURE.\nWill skip checking this database \
procedure\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_obj->psq_dbp_name),
			    &cur_obj->psq_dbp_name,
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name,
			    STlength(c), c);
		    }

		    if (~cur_obj->psq_objtype & PSQ_GRANTABLE_DBPROC)
		    {
#ifdef xDEBUG
			if (ps133)
			{
			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"<<<\nDatabase procedure %t invoked by \
%t was not marked as grantable.\n%t cannot possibly be marked as grantable.\
\n>>>\n---",			psf_trmwhite(sizeof(DB_DBP_NAME),
				    (char *) &cur_obj->psq_dbp_name),
			        &cur_obj->psq_dbp_name,
			        psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			        &cur_node->pss_obj_list->psq_dbp_name,
				psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			        &cur_node->pss_obj_list->psq_dbp_name);
		        }
#endif
			/*
			** if this dbproc is not guaranteed to be grantable,
			** then neither is the dbproc in whose independent
			** object list it appears
			*/
			cur_node->pss_dbp_mask &= ~DB_DBPGRANT_OK;
		    }
		    continue;
		}
	    }
		
	    /*
	    ** we found a descriptor of a dbproc owned by the current user which
	    ** is used in the dbproc passed by the caller and for which we
	    ** cannot guarantee that the user posesses adequate privileges
	    **
	    ** we will scan the auxiliary list (infolist) of privilege
	    ** descriptors to determine if we already know that the user
	    ** does/doesn't posess required privileges on this dbproc; 
	    ** if no such descriptor is found, we may have to employ other
	    ** mechanisms (discussed in the history section (30-sep-91)) to
	    ** determine if the user posesses required privilege on this dbproc;
	    ** note that we do not need to scan the entire independent privilege
	    ** list since descriptors pertaining to dbprocs owned by the current
	    ** user can only occur in the internal sublist which follows all
	    ** regular sublists
	    */

	    for (p = infolist; p; p = p->psq_next)
	    {
		if (   p->psq_objtype & PSQ_OBJTYPE_IS_DBPROC
		    && p->psq_objid.db_tab_base ==
		       cur_obj->psq_objid->db_tab_base
		    && p->psq_objid.db_tab_index ==
		       cur_obj->psq_objid->db_tab_index
		   )
		{
		    /* we are in luck */
		    break;
		}
	    }

	    if (p)
	    {
		if (p->psq_objtype & PSQ_MISSING_PRIVILEGE)
		{
		    /*
		    ** remember that the user lacks required privileges on the
		    ** dbproc associated with root_dbp;
		    ** remaining processing will occur when we exit the
		    ** "for all headers in the internal list" loop
		    */
		    insuf_priv = TRUE;
		    satisfied = FALSE;

		    if (ps133)
		    {
		        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nWe have previously determined that \
database procedure %t \ninvoked by %t is not %t thus making the calling \n\
database procedure not %t as well.\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
			        (char *) &cur_obj->psq_dbp_name),
			    &cur_obj->psq_dbp_name,
			    psf_trmwhite(sizeof(DB_DBP_NAME),
			        (char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name,
			    STlength(active_or_grantable), active_or_grantable,
			    STlength(active_or_grantable), active_or_grantable);
		    }

		    break;
		}	    /* user lacks required privilege */
		else
		{
		    /* user posesses required privikege - skip this entry */
		    if (ps133)
		    {
		        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nWe have previously determined that \
database procedure %t \ninvoked by %t is %t.  Skip checking this database \
procedure.\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
			        (char *) &cur_obj->psq_dbp_name),
			    &cur_obj->psq_dbp_name,
			    psf_trmwhite(sizeof(DB_DBP_NAME),
			        (char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name,
			    STlength(active_or_grantable), active_or_grantable);
		    }

		    continue;
		}
	    }	/* found a privilege descriptor */
	    else
	    {
		DB_TAB_ID		*id = cur_obj->psq_objid;

		if (psy_recursive_chain(cur_node, id))
		{
		    /*
		    ** dbproc is being called recursively - will not process
		    ** it - skip to the next independent object descriptor
		    */
#ifdef xDEBUG
		    if (ps133)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nDatabase procedure %t is called \
recursively - no further\nprocessing will be performed until we encounter the \
root of the recursive call chain.\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_obj->psq_dbp_name),
			    &cur_obj->psq_dbp_name);
		    }
#endif
		    continue;
		}

		/*
		** an interesting caveat:
		**	suppose U owns P1, P2, and P3, furthermore, let
		**	P1 = {exec P3; exec P2} and P2 = {exec P3}.
		**	If P1 was passed by the caller and IIPROCEDURE does not
		**	indicate that the user posesses required privileges on
		**	P1, P2, and P3, I would prefer to avoid having
		**	psy_dbp_priv_check() 	process P3 twice.
		**	After examining P1's list of independent objects,
		**	internal header list will look as follows:
		**
		**	    P1
		**	    |
		**	    |child
		**	    |
		**	    v sibling
		**	    P2-------->P3
		**
		**	In the course of processing P2's independent
		**	object list, we will come across P3 again.
		**
		**	In the interest of efficiency, I would like to ensure
		**	that no dbproc is processed more than once; of course,
		**	this may not come at the expense of loosing information.
		**	Below you may find a description of the algorithm which
		**	will be used to accomplish this task and an informal
		**	justification of why it will not break everything.
		**
		**	Let DBP be the dbproc for which we were asked to
		**	determine whether it is grantable/active (i.e. the
		**	dbproc pointed to by root_dbp).
		**	Let DBP1 and DBP2 be two different dbprocs in the tree
		**	rooted in DBP (note that it is possible that one of
		**	DBP1, DBP2 may be DBP.)
		**	Let DBP3 be a dbproc invoked by both DBP1 and DBP2
		**	Suppose DBP1 was encounterd before DBP2 in the course of
		**	traversing the tree rooted in DBP.
		**	
		**	If by the time we encounter DBP3 in the independent
		**	object list associated with DBP2 it has not been
		**	determined whether he posesses required privileges on
		**	DBP3, one of the following must be true:
		**	
		**	  (1) the first node representing DBP3 (i.e. the node
		**	      which is the child of DBP1) has not been
		**	      processed yet - this implies that DBP3's parent
		**	      (i.e. DBP1) is also DBP2's ancestor or
		**	  (2) DBP3 is involved in a recursive call chain
		**	      root of which is DBP2's ancestor (otherwise the
		**	      recursive call chain involving DBP3 would have
		**	      been processed by now) and is either DBP1 or
		**	      DBP1's ancestor
		**
		**	If (1) is true, I claim that we can safely move DBP3
		**	from DBP1's children list to DBP2's children list
		**	without introducing any errors into the algorithm used
		**	to determine when a given dbproc is grantable/active:
		**
		**	  - if DBP3 in not involved in any recursive call
		**	    chains, its ancestors will continue to depend on it
		**	    because they depend on DBP2; if it turns out that
		**	    DBP3 recursively invokes any of DBP2's ancestors
		**	    (possibly DBP1 itself), recursive call chain will
		**	    correctly represent dependencies existing between
		**	    the recursively called dbprocs
		**
		**	If (2) is true, I claim that we can simply build a
		**	recursive call chain from DBP2 to the root of the chain
		**	to which DBP3 belonged.  This will ensure that we will
		**	not try to make conclusions about DBP2's being
		**	grantable/active until all nodes in the recursive call
		**	chain in which DBP3 was involved are processed
		**
		**	(I hope you are convinced ... now, could you please
		**	convince me?)  
		**
		**	By the way, the case described in the beginning of this
		**	diatribe (archaic: prolonged discourse) falls under
		**	category (1), so the tree will be transformed to look as
		**	follows:
		**	
		**	    P1
		**	    |
		**	    |child
		**	    |
		**	    v 
		**	    P2
		**	    |
		**	    |child
		**	    |
		**	    v
		**	    P3
		*/

		/*
		** if a header associated with this dbproc already exists, merge
		** it into the tree as described above; otherwise, call
		** psy_dbp_priv_check()
		*/

		if (!psy_dupl_dbp(root_dbp, cur_obj, cur_node, ps133))
		{
		    /*
		    ** we have exhausted all the shortcuts - try to determine if
		    ** user posesses required privileges on the dbproc 
		    */

		    /*
		    ** if processing GRANT, dbproc must appear grantable;
		    ** otherwise it must appear active
		    */
		    privs = (qmode == PSQ_GRANT)
			? (i4) (DB_EXECUTE | DB_GRANT_OPTION) : (i4) DB_EXECUTE;
		    
#ifdef xDEBUG
		    if (ps133)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nWill call psy_dbp_priv_check() to \
determine whether the current user\nposesses required privileges on database \
procedure %t.\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_obj->psq_dbp_name),
			    &cur_obj->psq_dbp_name);
		    }
#endif
		    status = psy_dbp_priv_check(sess_cb, psq_cb,
			&cur_obj->psq_dbp_name, id, &non_existant_dbp,
			&missing_obj, loc_dbp_mask, &privs, err_blk);

		    if (DB_FAILURE_MACRO(status))
			goto dbp_stat_exit;
		    else if (non_existant_dbp)
		    {
			/*
			** dbproc could not be found - this is highly irregular
			** this could conceivably happen if we spend too much
			** time in this function and some object successfully
			** looked up for some dbproc no longer exists
			** 
			** To distinguish this from the case when the dbproc
			** passed by the caller could not be found, we will
			** reset non_existant_dbp and set missing_obj
			** 
			** remember that we have not verified that the user
			** posesses required privilege
			*/
			non_existant_dbp = FALSE;
			missing_obj = TRUE;
			satisfied = FALSE;
			break;
		    }
		    else if (missing_obj)
		    {
			/*
			** some object used by the dbproc does not exist - it is
			** clearly abandoned
			** remember that we have not verified that the user
			** posesses required privilege
			*/
			satisfied = FALSE;
			break;
		    }
		    else if (!privs)
		    {
			insuf_priv = TRUE;
			satisfied = FALSE;
			break;
		    }
		    else if (loc_dbp_mask[0] & DB_DBPGRANT_OK ||
			        (qmode != PSQ_GRANT
				 && loc_dbp_mask[0] & DB_ACTIVE_DBP))
		    {
			/*
			** either the dbproc is grantable and active (what more
			** can we ask for) or we needed to verify that the
			** dbproc is active and psy_dbp_priv_check() guarantees
			** that it is - record the fact in the infolist and skip
			** to the next entry
			*/
			status = psy_insert_objpriv(sess_cb, id, PSQ_OBJTYPE_IS_DBPROC,
			    (i4) DB_EXECUTE, sess_cb->pss_dependencies_stream,
			    &infolist->psq_next, err_blk);
			if (DB_FAILURE_MACRO(status))
			    goto dbp_stat_exit;

			continue;
		    }
		    else
		    {
			PSS_DBPLIST_HEADER	    *hdr;

			/*
			** we may need to determine if the current user's
			** dbproc(s) invoked by this dbproc are themselves
			** grantable, etc.
			*/
			status = psf_malloc(sess_cb, sess_cb->pss_dependencies_stream,
			    sizeof(PSS_DBPLIST_HEADER), (PTR *) &hdr,
			    err_blk);
			if (DB_FAILURE_MACRO(status))
			    goto dbp_stat_exit;

			/*
			** link this header into the list of headers created so
			** far - this will facilitate scanning the entire list
			*/
			hdr->pss_next = root_dbp->pss_next;
			root_dbp->pss_next = hdr;

			/*
			** initialize internal header:
			**  parent is pointed to by cur_node;
			**  sibling can be found by following
			**  cur_node->pss_child - for ease of use we will always
			**  make the newly discovered child the first among
			**  siblings;
			**  it is not a part of a recursive chain (not yet,
			**  anyway);
			**  unless known otherwise, we will assume that it is
			**  active and grantable - this is mostly relevant when
			**  processing [re]create, since to succeed with GRANT
			**  it must be both active and grantable
			*/
			hdr->pss_parent = cur_node;

			/*
			** link this node into the list of children
			** of cur_node
			*/
			hdr->pss_sibling = cur_node->pss_child;
			cur_node->pss_child = hdr;

			hdr->pss_child = hdr->pss_rec_chain =
			    (PSS_DBPLIST_HEADER *) NULL;

			hdr->pss_flags = 0;
			hdr->pss_obj_list = sess_cb->pss_indep_objs.psq_objs;
			/* remember whether the dbproc appeared grantable */
			hdr->pss_dbp_mask = (privs & DB_GRANT_OPTION)
			    ? DB_DBPGRANT_OK | DB_ACTIVE_DBP : DB_ACTIVE_DBP;

			/*
			** if there is an independent object/privilege list for
			** in IIDBDEPENDS/IIPRIV, for this dbproc,
			** loc_dbp_mask[0] will have DB_DBP_INDEP_LIST bit set -
			** we need to copy it into pss_dbp_mask
			*/
			hdr->pss_dbp_mask |= loc_dbp_mask[0];
		    }
		}
	    }
	}	    /* for all dbprocs in the sublist */

	if (!satisfied)
	{
	    /* dbproc is not grantable - no reason to continue */
	    break;
	}

	/*
	** in essence, we are doing a depth-first traversal of a
	** tree consisting of internal headers;
	** if "cur_node" has children,
	**   will process the first child next
	** else
	**   // starting at cur_node follow pss_parent list looking for a node
	**   // with siblings
	**   while cur_node != NULL
	**     if (qmode==PSQ_CREDBP || qmode==PSQ_EXEDBP)
	**       // it is not required that the user must posess GRANTABLE
	**	 // privilege
	**	 check *cur_node and list headers associated with its children
	**	 to determine whether the dbproc with which *cur_node is
	**	 associated appears to be active or grantable and update
	**	 cur_node->pss_dbp_mask accordingly
	**     else
	**	 // qmode==PSQ_GRANT --> dbproc asociated with *cur_node and
	**	 // dbprocs associated with cur_node's children must all have
	**	 // appeared grantable - cur_node->pss_dbp_mask is already set
	**	 // accordingly
	**     endif
	**
	**     if cur_node is not a member of any recursive chain,
	**	 // user posesses required privileges on the dbproc with
	**	 // which *cur_node is associated -
	**	 if (cur_node!=root_dbp || qmode!=PSQ_CREDBP)
	**	   // the dbproc exists
	**	   record in IIPROCEDURE whether the dbproc is active or
	**	   grantable (cur_node->pss_dbp_mask has the required info)
	**	 else
	**	   // info will be entered when the new dbproc is
	**	   // actually created
	**	 endif
	**     else if cur_node is a root of a recursive chain,
	**	 // user posesses required privileges on dbprocs associated with
	**	 // all members of the chain (including *cur_node)
	**	 if all headers in the chain indicate that the dbprocs
	**	    with which they are associated appear grantable,
	**         dbprocs associated with all headers in the chain are, indeed,
	**	   grantable;
	**	 else
	**	   // there is at least one member of the chain which is not
	**	   // grantable
	**	   dbprocs associated with all members of the chain are
	**	   ungrantable
	**	 endif
	**	 // the fact that a dbproc is active or grantable MUST be
	**	 // recorded in IIPROCEDURE
	**	 // (note that if (cur_node==root_dbp && qmode==PSQ_CREDBP), the
	**	 // dbproc does not exist, so the info will be entered AFTER it
	**	 // is created)
	**     endif
	**
	**     if cur_node has siblings
	**	 leave this loop - we will process cur_node->pss_sibling next
	**     else
	**	 cur_node = cur_node->pss_parent
	**     endif
	**   endwhile
	**
	**   if cur_node != NULL
	**     will process cur_node's sibling next
	**   else
	**     user posesses required privilege on the dbproc associated
	**     with root_dbp (i.e. the dbproc passed by the caller) -
	**     simply leave the loop
	**   endif
	** endif
	*/

	if (cur_node->pss_child)
	{
#ifdef xDEBUG
	    if (ps133)
	    {
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "<<<\nNext we will process independent object \
lists associated with\ndatabase procedures invoked by %t.\n>>>\n---",
		    psf_trmwhite(sizeof(DB_DBP_NAME),
			(char *) &cur_node->pss_obj_list->psq_dbp_name),
		    &cur_node->pss_obj_list->psq_dbp_name);
	    }
#endif

	    cur_node = cur_node->pss_child;
	}
	else
	{
	    while (1)
	    {
		if (   (qmode == PSQ_CREDBP || qmode == PSQ_EXEDBP)
		    && cur_node->pss_dbp_mask & DB_DBPGRANT_OK
		    && cur_node->pss_child)
		{
		    /*
		    ** dbproc associated with cur_node appeared to be
		    ** grantable - examine cur_node's children to verify that it
		    ** is
		    */
		    PSS_DBPLIST_HEADER	    *h;

#ifdef xDEBUG
		    if (ps133)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nWe have finished processing \
independent object lists associated\nwith database procedures invoked by %t.\n\
During processing of %t, it appeared to be grantable - now we will\n\
examine information collected for database procedures invoked by it to make \n\
sure that it is, indeed, grantable.\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name,
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name);
		    }
#endif
		    for (h = cur_node->pss_child; h; h = h->pss_sibling)
		    {
			if (~h->pss_dbp_mask & DB_DBPGRANT_OK)
			{
			    /*
			    ** dbproc associated with cur_node is obviously
			    ** ungrantable - no need to scan any further
			    */
			    cur_node->pss_dbp_mask &= ~DB_DBPGRANT_OK;
#ifdef xDEBUG
			    if (ps133)

			    {
				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "<<<\n%t invoked by %t is not \
grantable.\n>>>\n---",
				    psf_trmwhite(sizeof(DB_DBP_NAME),
				       (char *) &h->pss_obj_list->psq_dbp_name),
				    &h->pss_obj_list->psq_dbp_name,
				    psf_trmwhite(sizeof(DB_DBP_NAME),
					(char *) &cur_node->pss_obj_list->psq_dbp_name),
				    &cur_node->pss_obj_list->psq_dbp_name);
			    }
#endif
			    break;
			}
		    }
		}

		/*
		** if the dbproc is not a member of a recursive call chain, we
		** now know whether it is grantable and/or active
		*/
		if (~cur_node->pss_flags & PSS_RC_MEMBER)
		{
		    /*
		    ** user posesses required privileges on the dbproc with
		    ** which *cur_node is associated
		    */

		    if (ps133)
		    {
			char	*c = 
			    (cur_node->pss_dbp_mask & DB_DBPGRANT_OK)
				? "active and grantable" : "active";

			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\n%t will be marked as %t in \
IIPROCEDURE.\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name,
			    STlength(c), c);
		    }

		    if (cur_node != root_dbp)
		    {
			/*
			** remember that the user posesses required privileges
			** on this dbproc unless *cur_node represents the
			** dbproc passed by the caller in which case we are
			** about to exit and will never get a chance to use
			** this information
			*/
			status = psy_insert_objpriv(sess_cb,
			    cur_node->pss_obj_list->psq_objid,
			    PSQ_OBJTYPE_IS_DBPROC, (i4) DB_EXECUTE,
			    sess_cb->pss_dependencies_stream,
			    &infolist->psq_next, err_blk);
			if (DB_FAILURE_MACRO(status))
			    goto dbp_stat_exit;
		    }

		    /*
		    ** change db_mask to reflect newly discovered facts about
		    ** the dbproc (i.e. that it is active or grantable);
		    */

		    STRUCT_ASSIGN_MACRO((*cur_node->pss_obj_list->psq_objid),
			rdf_rb->rdr_tabid);

		    /*
		    ** need to pass dbproc name so that QEF can find the
		    ** tuple in IIPROCEDURE and the new value of dbp_mask1 so
		    ** QEF can correctly update it
		    */
		    STRUCT_ASSIGN_MACRO(cur_node->pss_obj_list->psq_dbp_name,
			dbp_tuple->db_dbpname);
		    dbp_tuple->db_mask[0] = cur_node->pss_dbp_mask;

		    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		    if (DB_FAILURE_MACRO(status))
		    {
			rdf_upd_err = TRUE;
			goto dbp_stat_exit;
		    }
		}
		else if (cur_node->pss_flags & PSS_RC_ROOT)
		{
		    /* cur_node is a root of a recursive chain */
		    
		    PSS_DBPLIST_HEADER	    *h;
		    i4			    mask = cur_node->pss_dbp_mask;

		    if (   (qmode == PSQ_CREDBP || qmode == PSQ_EXEDBP)
			&& mask & DB_DBPGRANT_OK)
		    {
			/*
			** dbproc associated with cur_node appeared to be
			** grantable - examine other members of the chain
			** to verify that they all are marked as grantable
			*/

#ifdef xDEBUG
			if (ps133)
			{
			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"<<<\n%t is a root of a recursive call \
chain.\nWhen it was processed, it appeared grantable.  Now we will examine \n\
other members of the call chain to determine if it is, indeed, grantable.\n\
>>>\n---",
			        psf_trmwhite(sizeof(DB_DBP_NAME),
				    (char *) &cur_node->pss_obj_list->psq_dbp_name),
				&cur_node->pss_obj_list->psq_dbp_name);
			}
#endif
			for (h = cur_node; h; h = h->pss_rec_chain)
			{
			    if (~h->pss_dbp_mask & DB_DBPGRANT_OK)
			    {
				cur_node->pss_dbp_mask =
				    (mask &= ~DB_DBPGRANT_OK);
#ifdef xDEBUG
				if (ps133)
				{
				    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
					"<<<\n%t which is a member of \
a recursive call chain \nrooted in %t is not grantable - therefore none of the \
database procedures \nbelonging to the call chain are grantable.\n>>>\n---",
					psf_trmwhite(sizeof(DB_DBP_NAME),
					    (char *) &h->pss_obj_list->psq_dbp_name),
					&h->pss_obj_list->psq_dbp_name,
					psf_trmwhite(sizeof(DB_DBP_NAME),
					    (char *) &cur_node->pss_obj_list->psq_dbp_name),
					&cur_node->pss_obj_list->psq_dbp_name);
				}
#endif
				break;
			    }
			}
		    }
		    
#ifdef xDEBUG
		    if (ps133)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<\nMarking member of a recursive call \
chain rooted in %t.\n>>>\n---",
			    psf_trmwhite(sizeof(DB_DBP_NAME),
				(char *) &cur_node->pss_obj_list->psq_dbp_name),
			    &cur_node->pss_obj_list->psq_dbp_name);
		    }
#endif
		    for (h = cur_node; h; h = h->pss_rec_chain)
		    {
			/*
			** mask indicates whether all dbprocs in the chain are
			** grantable or just active
			*/
			h->pss_dbp_mask = mask;

			if (ps133)
			{
			    char	*c = (mask & DB_DBPGRANT_OK)
				? "active and grantable" : "active";

			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"<<<\n%t will be marked as %t in \
IIPROCEDURE.\n>>>\n---",
				psf_trmwhite(sizeof(DB_DBP_NAME),
				    (char *) &h->pss_obj_list->psq_dbp_name),
				&h->pss_obj_list->psq_dbp_name,
				STlength(c), c);
			}

			/*
			** this (comparing cur_node instead of h) is not a
			** mistake - if the root of the recursive call chain
			** represents the dbproc passed by the caller, we are
			** almost done
			*/
			if (cur_node != root_dbp)
			{
			    /*
			    ** remember that the user posesses required
			    ** privileges on this dbproc unless *cur_node
			    ** represents the dbproc passed by the caller in
			    ** which case we are about to exit and will never
			    ** get a chance to use this information
			    */
			    status = psy_insert_objpriv(sess_cb,
				h->pss_obj_list->psq_objid,
				PSQ_OBJTYPE_IS_DBPROC, (i4) DB_EXECUTE,
				sess_cb->pss_dependencies_stream,
				&infolist->psq_next, err_blk);
			    if (DB_FAILURE_MACRO(status))
				goto dbp_stat_exit;
			}

			/*
			** change db_mask to reflect newly discovered facts
			** about the dbproc (i.e. that it is active or
			** grantable)
			*/

			STRUCT_ASSIGN_MACRO((*h->pss_obj_list->psq_objid),
			    rdf_rb->rdr_tabid);

			/*
			** need to pass dbproc name so that QEF can find the
			** tuple in IIPROCEDURE and the new value of
			** dbp_mask1 so QEF can correctly update it
			*/
			STRUCT_ASSIGN_MACRO(h->pss_obj_list->psq_dbp_name,
			    dbp_tuple->db_dbpname);
			dbp_tuple->db_mask[0] = h->pss_dbp_mask;

			status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
			if (DB_FAILURE_MACRO(status))
			{
			    rdf_upd_err = TRUE;
			    goto dbp_stat_exit;
			}
		    }
		}

		/*
		** remember that the subtree rooted in cur_node has been
		** processed
		*/
		cur_node->pss_flags |= PSS_PROCESSED;

		if (cur_node->pss_sibling)
		{
		    cur_node = cur_node->pss_sibling;
		    break;
		}
		else if (cur_node != root_dbp)
		{
		    cur_node = cur_node->pss_parent;
		}
		else
		{
		    /*
		    ** let the caller know whether the dbproc passed in was
		    ** determined to be active or grantable
		    */
		    dbp_mask[0] = cur_node->pss_dbp_mask;
		    cur_node = (PSS_DBPLIST_HEADER *) NULL;
		    break;
		}
	    }
	}
    }	/* for all headers in the internal list */

dbp_stat_exit:

    /* unexpected error */
    if (DB_FAILURE_MACRO(status))
    {
	/* if unexpected error has occurred, we will return the error status */

	if (rdf_upd_err)
	{
	    /* unexpected error occurred when calling RDF_UPDATE */
	    if (rdf_cb.rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
	    {
		_VOID_ psf_error(E_PS0905_DBP_NOTFOUND, 0L,
		    PSF_USERERR, &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(dbp_tuple->db_dbpname),
			(char *) &dbp_tuple->db_dbpname),
		    &dbp_tuple->db_dbpname);
	    }
	    else
	    {
		_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, err_blk);
	    }
	}
	else if (qmode == PSQ_GRANT)
	{
	    if (root_dbp)
	    {
		_VOID_ psf_error(E_PS0559_CANT_GRANT_DUE_TO_ERR, 0L,
		    PSF_USERERR, &err_code, err_blk, 2,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) &dbp_descr->psy_tabnm),
		    &dbp_descr->psy_tabnm,
		    psf_trmwhite(sizeof(DB_DBP_NAME), 
			(char *) &cur_obj->psq_dbp_name),
		    &cur_obj->psq_dbp_name);
	    }
	    else
	    {
		_VOID_ psf_error(E_PS0558_GRANT_CHECK_ERROR, 0L, PSF_USERERR,
		    &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) &dbp_descr->psy_tabnm),
		    &dbp_descr->psy_tabnm);
	    }
	}
	else if (qmode == PSQ_CREDBP && root_dbp)
	{
	    /*
	    ** cannot create dbproc due to unexpected error
	    ** we check root_dbp to make sure the error occurred after we
	    ** started processing dbprocs used in the dbproc being created 
	    */
	    _VOID_ psf_error(E_PS055D_ERR_IN_CREATE_DBP, 0L, PSF_USERERR,
		&err_code, err_blk, 2,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &dbp_descr->psy_tabnm),
		&dbp_descr->psy_tabnm,
		psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) &cur_obj->psq_dbp_name),
		&cur_obj->psq_dbp_name);
	}
	else if (qmode == PSQ_EXEDBP && root_dbp)
	{
	    /*
	    ** dbproc cannot be recreated due to some unexpected error
	    ** we check root_dbp to make sure the error occurred after we
	    ** started processing dbprocs used in the dbproc being created 
	    */
	    _VOID_ psf_error(E_PS055E_ERR_IN_RECREATE_DBP, 0L, PSF_USERERR,
		&err_code, err_blk, 2,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &dbp_descr->psy_tabnm),
		&dbp_descr->psy_tabnm,
		psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) &cur_obj->psq_dbp_name),
		&cur_obj->psq_dbp_name);
	}
    }	    /* some unexpected error occurred */
    
    /* user posesses required privileges on the dbproc */
    else if (satisfied)
    {
	if (qmode == PSQ_GRANT)
	{
	    /*
	    ** mark PSY_TBL entry for the current dbproc to indicate that it was
	    ** determined to be grantable
	    */
	    cur_grant_entry->psy_mask |= PSY_DBPROC_IS_GRANTABLE;
	}
    }
    
    /*
    ** user lacks some required privilege or an object used in the dbproc does
    ** not exist;
    ** 
    ** note that we intentionally are not checking for 'non_existant_dbp' which
    ** would be set only if the dbproc specified by the caller in the course of
    ** processing a GRANT statement did not exist; if that was the case, the
    ** message produced by pst_dbpshow() is all that is required.
    */
    else if (insuf_priv || missing_obj)
    {
	/*
	** we've been unable to verify that the user posesses required
	** privilege(s); this would happen if
	** - we have determined some object used in dbproc's definition
	**   (directly or via another dbproc called by it) does not exist or
	** - user lacks required privilege(s) on some object used in the dbproc
	**   (directly or via another dbproc called by it)
	*/
	if (qmode == PSQ_GRANT)
	{
	    /* will audit GRANT failure */
	    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	    {
		DB_STATUS   local_status;
		DB_ERROR	e_error;

		local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)&dbp_descr->psy_tabnm,
			    &sess_cb->pss_user,
			    sizeof(DB_TAB_NAME), SXF_E_PROCEDURE,
			    I_SX2017_PROT_DBP_CREATE, 
			    SXF_A_FAIL | SXF_A_CONTROL,
			    &e_error);
	    }

	    /*
	    ** if root_dbp is not NULL, the dbproc named in the GRANT
	    ** statement is ungrantable because some dbproc used in its
	    ** definition is ungrantable
	    */
	    if (root_dbp)
	    {
		_VOID_ psf_error(E_PS055A_UNGRANTABLE_DBP, 0L, PSF_USERERR,
		    &err_code, err_blk, 2,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) &dbp_descr->psy_tabnm),
		    &dbp_descr->psy_tabnm,
		    psf_trmwhite(sizeof(DB_DBP_NAME), 
			(char *) &cur_obj->psq_dbp_name),
		    &cur_obj->psq_dbp_name);
	    }
	    else
	    {
		_VOID_ psf_error(E_PS042C_CANT_GRANT_DBP_PERM, 0L,
		    PSF_USERERR, &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) &dbp_descr->psy_tabnm),
		    &dbp_descr->psy_tabnm);
	    }
	}
	else if (qmode == PSQ_CREDBP)
	{
	    /* dormant dbproc cannot be created */
	    _VOID_ psf_error(E_PS055B_CREATE_DORMANT_DBP, 0L, PSF_USERERR,
		&err_code, err_blk, 2,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &dbp_descr->psy_tabnm),
		&dbp_descr->psy_tabnm,
		psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) &cur_obj->psq_dbp_name),
		&cur_obj->psq_dbp_name);

	    status = E_DB_ERROR;
	}
	else if (qmode == PSQ_EXEDBP)
	{
	    /* user may not execute his dbproc - it is dormant */
	    _VOID_ psf_error(E_PS055C_RECREATE_DORMANT_DBP, 0L, PSF_USERERR,
		&err_code, err_blk, 2,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &dbp_descr->psy_tabnm),
		&dbp_descr->psy_tabnm,
		psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) &cur_obj->psq_dbp_name),
		&cur_obj->psq_dbp_name);

	    status = E_DB_ERROR;
	}
    }	    /*
	    ** user lacks required privilege(s) or some objects used in dbprocs
	    ** could not be found
	    */

    /*
    ** close memory stream used for allocating independent object/priivlege
    ** lists if it has been opened
    */
    if (close_stream)
    {
	DB_STATUS	stat;

	stat = psf_mclose(sess_cb, &mem_stream, err_blk);
	if (DB_FAILURE_MACRO(stat) && stat > status)
	{
	    /* remember the most severe return value */
	    status = stat;
	}

	/* ensure that the stream that is no longer valid does not get used */
	sess_cb->pss_dependencies_stream = (PSF_MSTREAM *) NULL;
    }

    /*
    ** Memory stream which was used to allocate independent privilege/object
    ** lists elements has been closed - make sure that the elements themselves
    ** are not reachable.
    ** We will restore the independent privilege list to the way it looked when
    ** this function was called.  In case of GRANT, this means that we simply
    ** reset all list pointers to NULL; for [re]CREATE PROCEDURE, we use
    ** save_tail and save_head.
    */
    if (qmode == PSQ_GRANT)
    {
	sess_cb->pss_indep_objs.psq_objs = (PSQ_OBJ *) NULL;
	sess_cb->pss_indep_objs.psq_objprivs = (PSQ_OBJPRIV *) NULL;
	sess_cb->pss_indep_objs.psq_colprivs = (PSQ_COLPRIV *) NULL;
    }
    else
    {
	if (sess_cb->pss_indep_objs.psq_objs = save_head.psq_objs)
	    save_tail.psq_objs->psq_next = (PSQ_OBJ *) NULL;
	if (sess_cb->pss_indep_objs.psq_objprivs = save_head.psq_objprivs)
	    save_tail.psq_objprivs->psq_next = (PSQ_OBJPRIV *) NULL;
	if (sess_cb->pss_indep_objs.psq_colprivs = save_head.psq_colprivs)
	    save_tail.psq_colprivs->psq_next = (PSQ_COLPRIV *) NULL;
    }

    return    (status);
}

/*
** Name:    psy_dbp_priv_check	-   determine if the user posesses (or appears
**				    to posess) specified privileges on his/her
**				    dbproc
**
** Description:
**	Obtain description of the dbproc and check if it indicates that the
**	user posesses required privilege.  If not, reparse the dbproc in attempt
**	to determine if the current user posesses (or appears to posess)
**	required privilege.
**
** Input:
**	sess_cb		    session CB
**	
**	psq_cb		    PSF request CB
**	
**	dbpname		    dbproc name
**	
**	dbpid		    dbproc id
**	
**	dbp_mask	    ptr to a 2-element array of i4's to be used to store
**			    IIPROCEDURE.db_mask
**			    
**	privs		    privileges required on the dbproc
**	    DB_EXECUTE	    dbproc must be at least active
**	    DB_EXECUTE|
**	    DB_GRANT_OPTION dbproc must be grantable
**
** Output:
**	*dbp_mask	    a copy of IIPROCEDURE.db_mask for the dbproc;
**			    this is guaranteed to be valid only if
**			    (!DB_FAILURE_MACRO(ret_stat))
**
**	*non_existant_dbp   TRUE if the dbproc could not be looked up; FALSE
**			    otherwise;
**			    this is guaranteed to be valid only if
**			    (!DB_FAILURE_MACRO(ret_stat))
**
**	*missing_obj	    TRUE if in the course of reparsing a dbproc text
**			    some object used in the definition of the dbproc was
**			    not found (i.e. the dbproc is dormant); FALSE
**			    otherwise
**			    this is guaranteed to be valid only if
**			    (!DB_FAILURE_MACRO(ret_stat) && !*non_existant_dbp);
**			    
**	*privs		    privileges that the user appears to posess on the
**			    dbproc; this is guaranteed to be valid only if
**			    (!DB_FAILURE_MACRO(ret_stat) && !*non_existant_dbp
**			     && !*missing_obj)
**			    NOTE: dbp_mask conveys information found in
**			    IIPROCEDURE whereas *privs may represent info
**			    obtained by reparsing a dbproc, thus making it
**			    possible for the two to be different
**	    0		    user does not posess required privilege;
**			    note that if the caller wanted to determine whether
**			    a dbproc is grantable, and it is not, we will not
**			    try to determine if it is at least active
**	    DB_EXECUTE	    caller requested to verify that the dbproc is ACTIVE
**			    and it appears to be active but NOT grantable
**	    DB_EXECUTE|
**	    DB_GRANT_OPTION caller requested to verify that the dbproc is ACTIVE
**			    or GRANTABLE, and it appears to be active AND
**			    grantable 
**
** Returns:
**	E_DB_OK		user posesses required privileges or one of "expected"
**			errors occurred
**	E_DB_ERROR	unexpected error occurred
**  
** History:
**
**	14-oct-91 (andre)
**	    written
**	04-mar-92 (andre)
**	    before unfixing RDF object check the II_PROCEDURE tuple to see
**	    whether the dbproc is marked as grantable and/or active:
**		if it is marked as grantable,
**		    we will set PSS_DBPGRANT_OK in sess_cb->pss_dbp_flags to
**		    indicate to psy_dgrant() that the dbproc is grantable and no
**		    further checking will be required
**		else if it is marked as active,
**		    we will build the independent object/privilege list based on
**		    info in IIDBDEPENDS and IIPRIV and verify that the user
**		    posesses all required privileges WGO
**		else
**		    we will reparse the dbproc to determ,ine whether itis
**		    grantable and insert the independent object/privilege
**		    information into IIDBDEPENDS and IIPRIV and mark the dbproc
**		    as ACTIVE and, if upon completing reparsing PSS_DBPGRANT_OK
**		    is set in sess_cb->pss_dbp_flags, as grantable
**	20-mar-92 (andre)
**	    changed function to enable its use to determine if a user may
**	    [re]CREATE PROCEDURE owned by him as well as GRANT ON PROCEDURE
**	    owned by him
**	21-may-92 (andre)
**	    must reset sess_cb->pss_retry to PSS_REPORT_MSGS after the first
**	    attempt at psq_parseqry() to ensure that subsequent error messages
**	    are reported to the user
**	26-jun-92 (andre)
**	    having assembled a list of independent objects and privileges, call
**	    RDF to have QEF insert the list into IIDBDEPENDS/IIPRIV and update
**	    IIPROCEDURE.dbp_mask1 to indicate that catalogs contain a list of
**	    independent objects and privileges for the dbproc
**	30-jun-92 (andre)
**	    if IIPROCEDURE.dbp_mask1 indicates that the dbproc is dormant but
**	    there is an independent object/privilege list associated with it,
**	    call RDF to get the privilege list and verify that the user posesses
**	    them (WGO, if necessary); then call RDF to obtain the dbprocs on
**	    which the current dbproc depends and enter them into independent
**	    object list which will be returned to the caller
**	10-aug-92 (andre)
**	    when passing independent object/privilege list to qeu_dbp_status(),
**	    make sure that only descriptors pertaining to this dbproc are
**	    passed, i.e. if save_tail indicates that there independent
**	    object/privilege lists contain elements in addition to those
**	    pertaining to the current dbproc, we will temporarily separate these
**	    other elements from the list
**	09-sep-92 (andre)
**	    (1) before calling psq_parseqry() we will ensure that
**	        PSS_REFRESH_CACHE is set in pss_retry, thus allowing us to
**		remove code that retries psq_parseqry() with fresh cache
**	    (2) remove cache_flushed from the interface since it is no longer
**	        useful (as a side-effect of (1))
**	    (3) before calling psq_parseqry() ensure that PSS_REPORT_MSGS is
**	        reset, so that errors (if any) which occur during reparsing of
**		the dbproc will be placed into the error log but not displayed
**		on the screen
**	01-sep-93 (andre)
**	    if after parsing a dbproc which was marked dormant but now appears 
**	    to be active we have a list of independent objects/privileges to be
**	    inserted into IIPRIV, we must also pass to qeu_dbp_status() id of 
**	    a base table (if any) on which the dbproc depends
**	15-nov-93 (andre)
**	    init sess_cb->pss_flattening_flags to 0 before calling 
**	    psq_parseqry() to reparse a dbproc definition
**	13-jan-94 (andre)
**	    psy_gproc() will no longer issue an error message if the dbproc is 
**	    not found - instead it will become the responsibility of the caller
**	    to decide whether and which message needs to be issued
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
**      20-Nov-08 (wanfr01)
**          Bug 121252
**          Use new PSS_PARSING_GRANT flag to indicate to the parser that
**          the 'RECREATE' has no input set because it is run via a grant
**          statement
**	04-may-2010 (miket) SIR 122403
**	    Init new sess_cb->pss_stmt_flags2.
*/
DB_STATUS
psy_dbp_priv_check(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	DB_DBP_NAME 	*dbpname,
	DB_TAB_ID	*dbpid,
	bool            *non_existant_dbp,
	bool		*missing_obj,
	i4		*dbp_mask,
	i4		*privs,
	DB_ERROR	*err_blk)
{
    QSF_RCB             qsf_rb2, *dbp_qsf_rb_p = &qsf_rb2;
    RDF_CB		dbp_rdf_cb, *rdf_cb_p = &dbp_rdf_cb;
    /*
    ** save_head will be used to remember the beginning of the list before we
    ** start prepending more elements to it as a part of reparsing the dbproc
    */
    PSQ_INDEP_OBJECTS	save_head;
    DB_OWN_NAME		*dummy;
    i4			gproc_flags;
    i4		err_code;
    DB_STATUS		status, stat;
    bool		ps133;
    char		*active_or_grantable;
    char		trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    {
	i4		val1, val2;

	/* determine if trace point ps133 has been set */
	ps133 = (bool) ult_check_macro(&sess_cb->pss_trace,
		    PSS_DBPROC_GRNTBL_ACTIVE_TRACE, &val1, &val2);
    }

    if (ps133)
    {
	active_or_grantable = (*privs == (i4) (DB_EXECUTE | DB_GRANT_OPTION))
	    ? "grantable": "active";

	psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
	    "<<<\nWill verify that the database procedure '%t' \n \
owned by the current user is %t\n>>>\n---",
	    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname), dbpname,
	    STlength(active_or_grantable), active_or_grantable);
    }

    *missing_obj = *non_existant_dbp = FALSE;
    dbp_mask[0] = dbp_mask[1] = (i4) 0;
    psq_cb->psq_mode = 0;
    psq_cb->psq_error.err_code = E_PS0000_OK;

    /* store dbproc name in PSQ_CB, where psy_gproc() expects to find it */
    MEcopy((PTR) dbpname, sizeof(DB_DBP_NAME),
	(PTR) psq_cb->psq_cursid.db_cur_name);

    status = psy_gproc(psq_cb, sess_cb, dbp_qsf_rb_p, rdf_cb_p, &dummy,
	&sess_cb->pss_user, PSS_DBP_BY_OWNER, &gproc_flags);

    if (DB_FAILURE_MACRO(status))
    {
#ifdef xDEBUG
	if (ps133)
	{
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\nUnexpected error occurred while trying to obtain \
description of \ndatabase procedure %t owned by the current user\n>>>\n---",
		psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname), dbpname);
	}
#endif
	STRUCT_ASSIGN_MACRO(psq_cb->psq_error, (*err_blk));
	return(status);
    }
    else if (gproc_flags & PSS_MISSING_DBPROC)
    {
        (VOID) psf_error(2405L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname), (PTR) dbpname);

#ifdef xDEBUG
	if (ps133)
	{
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\nDatabase procedure %t owned by the current\n\
user does not exist\n>>>\n---",
		psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname), dbpname);
	}
#endif

	*non_existant_dbp = TRUE;
	return(E_DB_OK);
    }

    /*
    ** NOTE: at this point, QSF object containing dbproc text is unlocked, so if
    **	     we need to destroy it, we must lock it first.
    */

    /*
    ** save IIPROCEDURE.db_mask - this will tell us whether the dbproc was
    ** marked as active and/or grantable and/or if the list of independent
    ** objects/privileges can be found in IIDBDEPENDS/IIPRIV
    */
    dbp_mask[0] = rdf_cb_p->rdf_info_blk->rdr_dbp->db_mask[0];
    dbp_mask[1] = rdf_cb_p->rdf_info_blk->rdr_dbp->db_mask[1];

    /* we no longer need to have the RDF object: try to release it now */
    status = rdf_call(RDF_UNFIX, (PTR) rdf_cb_p);

    if (DB_FAILURE_MACRO(status))
    {
	_VOID_ psf_rdf_error(RDF_UNFIX, &rdf_cb_p->rdf_error, err_blk);

	/* cleanup the QSF object containing the text of the dbproc */
	goto dbp_priv_cleanup;
    }

    if (   *privs == (i4) (DB_EXECUTE | DB_GRANT_OPTION)
	&& *dbp_mask & DB_DBPGRANT_OK)
    {
	/*
	** IIPROCEDURE indicates that the user posesses privilege required by
	** the caller - no further processing is required
	*/

	if (ps133)
	{
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\n\
IIPROCEDURE indicates that the database procedure '%t' \n \
owned by the current user is grantable - no further checking of this\n\
database procedure is required\n>>>\n---",
	    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname), dbpname);
	}

	/* cleanup the QSF object containing the text of the dbproc */
	goto dbp_priv_cleanup;
    }
    else if (*privs == (i4) DB_EXECUTE && *dbp_mask & DB_ACTIVE_DBP)
    {
	if (ps133)
	{
	    active_or_grantable = (*dbp_mask & DB_DBPGRANT_OK)
		? "active and grantable" : "active";

	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\n\
IIPROCEDURE indicates that the database procedure '%t' \n \
owned by the current user is %t - no further checking of this\n\
database procedure is required\n>>>\n---",
	    psf_trmwhite(sizeof(DB_TAB_NAME), (char *) dbpname), dbpname,
	    STlength(active_or_grantable), active_or_grantable);
	}

	/* if IIPROCEDURE indicates that the dbproc is grantable, modify *privs
	** since the caller expects that *privs will indicate which privileges
	** the user posesses or appears to posess on the dbproc
	*/
	if (*dbp_mask & DB_DBPGRANT_OK)
	    *privs |= DB_GRANT_OPTION;

	/*
	** IIPROCEDURE indicates that the user posesses privilege required by
	** the caller - no further processing is required
	*/

	/* cleanup the QSF object containing the text of the dbproc */
	goto dbp_priv_cleanup;
    }
#if 0
    else if (*dbp_mask & DB_DBP_INDEP_LIST)
    {
	/*
	** code yet to be written:
	**	1) call RDF for the independent privileges
	**	2) verify that the user may grant all the privileges returned by
	**	   RDF; if a user posesses a given privilege WGO, enter a new
	**	   element into the independent privilege list - otherwise,
	**	   follow the same procesing as would occur if we were to
	**	   discover this by reparsing the dbproc (san retry)
	**	3) call RDF for the list of independent objects
	**	4) for each dbproc which is not marked as grantable, enter the
	**	   dbproc into the independent object list - if no dbprocs were
	**	   entered into the list, dbproc being processed is grantable -
	**	   we will call QEF (through RDF?) to mark the dbproc as
	**	   grantable for all future references + set PSS_DBPGRANT_OK in
	**	   sess_cb->pss_dbp_flags to indicate to the caller that the
	**	   dbproc is grantable
	*/
    }
#endif
	
    if (*privs & DB_GRANT_OPTION)
    {
	/*
	** if caller indicated that the user must be able to grant EXECUTE WGO,
	** set the bit to ensure that for every privileges required to execute a
	** dbproc statement, we insist that the user must posess that privilege
	** WGO
	*/

	if (ps133)
	{
	    active_or_grantable = "grantable";
	}

	sess_cb->pss_dbp_flags = PSS_RECREATE | PSS_0DBPGRANT_CHECK;
    }
    else
    {
	/*
	** otherwise, we may still be interested in whether the dbproc appears
	** to be grantable, but we'll settle for discovering that it is active;
	** setting PSS_CHECK_IF_ACTIVE will ensure that the independent
	** object/privilege list will not get reinitialized in PSQPARSE.C
	*/

	if (ps133)
	{
	    active_or_grantable = "active";
	}
    
	sess_cb->pss_dbp_flags = PSS_RECREATE | PSS_CHECK_IF_ACTIVE;
    }

    if (ps133)
    {
	psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
	    "<<<\n\
Will reparse database procedure '%t' \n \
owned by the current user to determine whether it is %t\n>>>\n---",
	psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname), dbpname,
	STlength(active_or_grantable), active_or_grantable);
    }

    sess_cb->pss_stmt_flags = sess_cb->pss_stmt_flags2 = 0L;

    sess_cb->pss_flattening_flags = 0;

    psq_cb->psq_qid = dbp_qsf_rb_p->qsf_obj_id.qso_handle;

    /* store name of the dbproc being parsed in sess_cb */
    sess_cb->pss_dbpgrant_name = dbpname;

    /*
    ** before calling psq_parseqry() remember the values in
    ** sess_cb->pss_indep_objs which will enable us to more efficiently
    ** determine where the independent object/privilege lists generated for this
    ** dbproc end
    */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_indep_objs, save_head);

    {
	/*
	** when reparsing a dbproc to determine whether it is ACTIVE or
	** GRANTABLE, we want to get "fresh" cache entries
	** (pss_retry |= PSS_REFRESH_CACHE) + we don't want errors to be shown
	** to the user (pss_retry &= ~PSS_REPORT_MSGS); upon returning from
	** psq_parseqry() we will return these bits to their original setting
	*/
	i4	    save_retry;

	/*
	** remember whether PSS_REFRESH_CACHE and/or PSS_REPORT_MSGS was set so
	** we can reset pss_retry correctly upon return from pss_parseqry()
	*/
	save_retry = sess_cb->pss_retry & (PSS_REFRESH_CACHE | PSS_REPORT_MSGS);

	sess_cb->pss_dbp_flags |= PSS_PARSING_GRANT;

	sess_cb->pss_retry |= PSS_REFRESH_CACHE;
	sess_cb->pss_retry &= ~PSS_REPORT_MSGS;
	
	status = psq_parseqry(psq_cb, sess_cb);

	/*
	** return PSS_REFRESH_CACHE and PSS_REPORT_MSGS to their original
	** setting
	*/
	sess_cb->pss_dbp_flags &= ~PSS_PARSING_GRANT;
	sess_cb->pss_retry &= ~(PSS_REFRESH_CACHE | PSS_REPORT_MSGS);
	sess_cb->pss_retry |= save_retry;
    }
    
    if (DB_FAILURE_MACRO(status))
    {
	/*
	** overwrite status returned by psq_parseqry() unless it was E_DB_SEVERE
	** or E_DB_FATAL
	*/
	if (   status == E_DB_ERROR
	    && psq_cb->psq_error.err_code == E_PS0001_USER_ERROR)
	{
	    /* if one of "expected" errors occurred, reset return status */
	    
	    if (sess_cb->pss_dbp_flags & PSS_MISSING_OBJ)
	    {
		if (ps133)
		{
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"<<<\nSome object used in the definition of \n\
database procedure %t does not exist\n>>>\n---",
			psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname),
			dbpname);
		}

		/* some object used in the dbproc does not exist */
		*missing_obj = TRUE;
		status = E_DB_OK;
	    }
	    else if (sess_cb->pss_dbp_flags & PSS_MISSING_PRIV)
	    {
		if (ps133)
		{
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"<<<\nCurrent user lacks some required \
privilege on an object\nused in the definition of database procedure %t\
\n>>>\n---",
			psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname),
			dbpname);
		}

		/* user lacks required privilege(s) */
		*privs = (i4) 0;
		status = E_DB_OK;
	    }
	}

	if (DB_FAILURE_MACRO(status))
	{
	    /* copy the error code into the error block */
	    STRUCT_ASSIGN_MACRO(psq_cb->psq_error, (*err_blk));
	}
    }
    else
    {
	PSF_MSTREAM	    tmp_stream;

	/*
	** save_tail will be used to remember the last elements of the list
	** added in the course of reparsing the dbproc - we need it since before
	** calling RDF to enter the list into IIDBDEPENDS/IIPRIV we will
	** temporarity separate it from the preexisting list by setting psq_next
	** to NULL
	*/
	PSQ_INDEP_OBJECTS	save_tail;
	PSQ_INDEP_OBJECTS	new_list;

	/*
	** store pointers to last elements of independent object/privilege lists
	** in save_tail - if the list is empty, set appropriate pointer to NULL;
	** if the list WAS empty before we added entries form the current
	** dbproc, we will not need to cut or restore the list, so set
	** appropriate pointer to NULL as well; while at it, set up new_list -
	** if a dbproc depends on any objects and/or privileges, new_list will
	** be passed to QEF
	*/

	if (save_head.psq_objs == sess_cb->pss_indep_objs.psq_objs)
	{
	    new_list.psq_objs = save_tail.psq_objs = (PSQ_OBJ *) NULL;
	}
	else
	{
	    new_list.psq_objs = sess_cb->pss_indep_objs.psq_objs;
	    
	    if (!save_head.psq_objs)
	    {
		save_tail.psq_objs = (PSQ_OBJ *) NULL;
	    }
	    else
	    {
		PSQ_OBJ	    *obj, *one_beyond_last = save_head.psq_objs;

		for (obj = sess_cb->pss_indep_objs.psq_objs;
		     obj->psq_next != one_beyond_last;
		     obj = obj->psq_next
		    )
		;

		save_tail.psq_objs = obj;
	    }
	}

	if (save_head.psq_objprivs == sess_cb->pss_indep_objs.psq_objprivs)
	{
	    new_list.psq_objprivs =
		save_tail.psq_objprivs = (PSQ_OBJPRIV *) NULL;
	}
	else
	{
	    new_list.psq_objprivs = sess_cb->pss_indep_objs.psq_objprivs;

	    if (!save_head.psq_objprivs)
	    {
		save_tail.psq_objprivs = (PSQ_OBJPRIV *) NULL;
	    }
	    else
	    {
		PSQ_OBJPRIV	*obj_priv,
				*one_beyond_last = save_head.psq_objprivs;

		for (obj_priv = sess_cb->pss_indep_objs.psq_objprivs;
		     obj_priv->psq_next != one_beyond_last;
		     obj_priv = obj_priv->psq_next
		    )
		;

		save_tail.psq_objprivs = obj_priv;
	    }
	}

	if (save_head.psq_colprivs == sess_cb->pss_indep_objs.psq_colprivs)
	{
	    new_list.psq_colprivs =
		save_tail.psq_colprivs = (PSQ_COLPRIV *) NULL;
	}
	else
	{
	    new_list.psq_colprivs = sess_cb->pss_indep_objs.psq_colprivs;

	    if (!save_head.psq_colprivs)
	    {
		save_tail.psq_colprivs = (PSQ_COLPRIV *) NULL;
	    }
	    else
	    {
		PSQ_COLPRIV	    *col_priv,
				    *one_beyond_last = save_head.psq_colprivs;

		for (col_priv = sess_cb->pss_indep_objs.psq_colprivs;
		     col_priv->psq_next != one_beyond_last;
		     col_priv = col_priv->psq_next
		    )
		;

		save_tail.psq_colprivs = col_priv;
	    }
	}

	/*
	** enter independent object/privilege list into IIDBDEPENDS/IIPRIV and
	** change IIPROCEDURE.dbp_mask1 to indicate that the list is in catalogs
	** unless the list is empty
	*/
	if (new_list.psq_objs || new_list.psq_objprivs || new_list.psq_colprivs)
	{
	    register RDR_RB		*rdf_rb = &rdf_cb_p->rdf_rb;
	    DB_PROCEDURE		proctuple, *dbp_tuple = &proctuple;

	    /*
	    ** temporarily separate the elements of independent object/privilege
	    ** list which were added for this dbproc from the rest of the list
	    */
	    if (save_tail.psq_objs)
		save_tail.psq_objs->psq_next     = (PSQ_OBJ *) NULL;
	    if (save_tail.psq_objprivs)
		save_tail.psq_objprivs->psq_next = (PSQ_OBJPRIV *) NULL;
	    if (save_tail.psq_colprivs)
		save_tail.psq_colprivs->psq_next = (PSQ_COLPRIV *) NULL;

	    new_list.psq_grantee    = sess_cb->pss_indep_objs.psq_grantee;

	    /* initialize important fields in RDF_CB */
	    rdf_rb->rdr_update_op   = RDR_REPLACE;
	    rdf_rb->rdr_types_mask  = RDR_PROCEDURE;
	    rdf_rb->rdr_2types_mask = RDR2_DBP_STATUS;
	    rdf_rb->rdr_status      = DB_SQL;
	    rdf_rb->rdr_indep       = (PTR) &new_list;
	    STRUCT_ASSIGN_MACRO((*dbpid), rdf_rb->rdr_tabid);
	    rdf_rb->rdr_qrytuple    = (PTR) dbp_tuple;
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dbp_tuple->db_owner);
	    
	    /*
	    ** need to pass dbproc name so that QEF can find the
	    ** tuple in IIPROCEDURE and the new value of dbp_mask1 so
	    ** QEF can correctly update it
	    */
	    STRUCT_ASSIGN_MACRO((*dbpname), dbp_tuple->db_dbpname);
	    dbp_tuple->db_mask[0] = dbp_mask[0] | (i4) DB_DBP_INDEP_LIST;

	    /*
	    ** while parsing a dbproc, we were looking for id of a base table
	    ** on which the dbproc depends - now we need to pass it to 
	    ** qeu_dbp_status()
	    */
	    dbp_tuple->db_dbp_ubt_id.db_tab_base = 
		sess_cb->pss_dbp_ubt_id.db_tab_base;
	    dbp_tuple->db_dbp_ubt_id.db_tab_index =
		sess_cb->pss_dbp_ubt_id.db_tab_index;

	    status = rdf_call(RDF_UPDATE, (PTR) rdf_cb_p);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (rdf_cb_p->rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
		{
		    _VOID_ psf_error(E_PS0905_DBP_NOTFOUND, 0L,
			PSF_USERERR, &err_code, err_blk, 1,
			psf_trmwhite(sizeof(dbp_tuple->db_dbpname),
			    (char *) &dbp_tuple->db_dbpname),
			&dbp_tuple->db_dbpname);
		}
		else
		{
		    _VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb_p->rdf_error,
			err_blk);
		}

		goto dbp_priv_cleanup;
	    }

	    /*
	    ** restore the independent object/privilege list pointers which were
	    ** temporarily set to NULL
	    */
	    if (save_tail.psq_objs)
		save_tail.psq_objs->psq_next     = save_head.psq_objs;
	    if (save_tail.psq_objprivs)
		save_tail.psq_objprivs->psq_next = save_head.psq_objprivs;
	    if (save_tail.psq_colprivs)
		save_tail.psq_colprivs->psq_next = save_head.psq_colprivs;

	    /*
	    ** IIPROCEDURE.dbp_mask1 now has DB_DBP_INDEP_LIST bit set - set it
	    ** in dbp_mask[0] which the caller expects to accurately reflect
	    ** IIPROCEDURE.dbp_mask1
	    */
	    dbp_mask[0] = dbp_tuple->db_mask[0];
	}

	/*
	** free up memory which was allocated for reparsing
	** of the dbproc; psq_parseqry() sets handles of all
	** streams to NULL, but we know that parsing a
	** dbproc results in opening of only pss_ostream and
	** the QSF object id was stored in psq_cb->psq_result
	*/

	/*
	** ensure that object is locked before an attempt is made to destroy it
	*/
	tmp_stream.psf_mlock = 0;

	STRUCT_ASSIGN_MACRO(psq_cb->psq_result, tmp_stream.psf_mstream);

	status = psf_mclose(sess_cb, &tmp_stream, err_blk);
	if (DB_FAILURE_MACRO(status))
	    goto dbp_priv_cleanup;

	/*
	** place PSQ_DBPLIST_HEADER structures in front of the newly added
	** elements of the independent object lists;
	**
	** having this element at the start of a sublist indicates that the
	** elements of the sublist represent the objects/privileges on which
	** a dbproc depends
	*/

	status = psy_mark_sublist(sess_cb, &sess_cb->pss_indep_objs,
	    sess_cb->pss_dependencies_stream, PSQ_DBPLIST_HEADER, dbpid,
	    dbpname, err_blk);
	if (DB_FAILURE_MACRO(status))
	    goto dbp_priv_cleanup;

	if (   *privs == (i4) DB_EXECUTE
	    && sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
	{
	    /*
	    ** caller needs for the dbproc to be at least active, but it appears
	    ** to be grantable as well - let the caller know
	    */
	    *privs |= DB_GRANT_OPTION;
	}

	if (ps133)
	{
	    active_or_grantable = (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
		? "active and grantable" : "active";

	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\nDatabase procedure %t appears to be %t\n>>>\n---",
		psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbpname), dbpname,
		STlength(active_or_grantable), active_or_grantable);
	}
    }

    /*
    ** this code is responsible for destroying text of the dbproc which was
    ** obtained prior to reparsing the dbproc to determine its grantability
    */
dbp_priv_cleanup:
    
    /* Lock the object exclusively in order to destroy it. */
    dbp_qsf_rb_p->qsf_lk_state = QSO_EXLOCK;

    stat = qsf_call(QSO_LOCK, dbp_qsf_rb_p);

    if (DB_FAILURE_MACRO(stat))
    {
	_VOID_ psf_error(E_PS0A08_CANTLOCK, dbp_qsf_rb_p->qsf_error.err_code,
		PSF_INTERR, &err_code, err_blk, 0);
	if (stat > status)
	    status = stat;
    }
    else
    {
	/* Destroy the object, now that it's locked. */
	stat = qsf_call(QSO_DESTROY, dbp_qsf_rb_p);

	if (DB_FAILURE_MACRO(stat))
	{
	    _VOID_ psf_error(E_PS0A09_CANTDESTROY,
		dbp_qsf_rb_p->qsf_error.err_code, PSF_INTERR, &err_code,
		err_blk, 0);
	    if (stat > status)
		status = stat;
	}
    }

    return(status);
}

/*
** Name: psy_dupl_dbp	- determine if a descriptor for a given dbproc has
**			  already been built and massage the tree as described
**			  below
**
** Description:
**  Scan the list of descriptors looking for a specified id.  If none is found,
**  return FALSE.  If it was found, massage the tree as follows:
**  let DBP refer to the descriptor we found; we distinguish two cases:
**    - if the subtree rooted in DBP has been processed, but DBP in involved in
**	a recursive call chain whose root is an ancestor of a descriptor whose
**	independent object list we are processing and which has not been fully
**	processed yet, we will call psy_recursive_chain() to build a recursive
**	call chain from the descriptor whose independent object list we are
**	processing to the root of the chain to which DBP depends
**    - if we haven't gotten around to processing DBP, move it from its current
**      parent's children list to the children list of the descriptor whose
**	independent object list we are processing
**
** Input:
**	dbp_list	    first element in the list of dbproc descriptors
**	obj		    independent object list element being processed
**	    psq_objid	    id of the dbproc we are supposed to look up
**	    psq_dbp_name    name of the dbproc we are supposed to look up
**	cur_dbp		    descriptor whose independent object list we are
**			    processing
**	ps133		    TRUE if ps133 has been set; FALSE otherwise
**
** Output:
**	none
**	
** Side effects:
**	tree may be rearranged as described above
**
** Exceptions:
**	none
**
** Returns:
**	TRUE	    found a descriptor with specified id
**	FALSE	    otherwise
**
** History:
**
**	15-may-92 (andre)
**	    written
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
*/		
static bool
psy_dupl_dbp(
	PSS_DBPLIST_HEADER  *dbp_list,
	PSQ_OBJ		    *obj,
	PSS_DBPLIST_HEADER  *cur_dbp,
	bool		    ps133)
{
    PSS_DBPLIST_HEADER	    *descr;
    DB_TAB_ID		    *id = obj->psq_objid;
    char		    trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    /* try to find the descriptor with specified id */
    for (descr = dbp_list; descr; descr = descr->pss_next)
    {
	if (   descr->pss_obj_list->psq_objid->db_tab_base ==
		   id->db_tab_base
	    &&
	       descr->pss_obj_list->psq_objid->db_tab_index ==
		   id->db_tab_index)
	{
	    break;
	}
    }

    if (!descr)
	return(FALSE);

    if (descr->pss_flags & PSS_PROCESSED)
    {
	/*
	** subtree rooted in descr has been processed but the dbproc represented
	** by descr belongs to a recursive call chain such that its root's
	** subtree has not been processed yet - we will find the root and build
	** a recursive call chain from the descriptor whose independent object
	** list we are processing to the root of the existing chain
	*/

	/* find the root of the chain to which descr belongs */
	for (; ~descr->pss_flags & PSS_RC_ROOT; descr = descr->pss_parent)
	;

	if (ps133)
	{
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\n'%t' belongs to a recursive call chain\n\
rooted in '%t'; will build a new chain\n\
from the root of existing chain to '%t'.\n>>>\n---",
		psf_trmwhite(sizeof(DB_DBP_NAME), (char *) &obj->psq_dbp_name),
		&obj->psq_dbp_name,
		psf_trmwhite(sizeof(DB_DBP_NAME),
		    (char *) &descr->pss_obj_list->psq_dbp_name),
		&descr->pss_obj_list->psq_dbp_name,
		psf_trmwhite(sizeof(DB_DBP_NAME),
		    (char *) &cur_dbp->pss_obj_list->psq_dbp_name),
		&cur_dbp->pss_obj_list->psq_dbp_name);
	}

	/* build a chain from cur_dbp to the root of the chain */
	if (!psy_recursive_chain(cur_dbp, descr->pss_obj_list->psq_objid))
	{
#ifdef	xDEBUG
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"!!!\nFailed to build a recursive call chain\n\
from '%t' to '%t'\n!!!\n---",
		psf_trmwhite(sizeof(DB_DBP_NAME),
		    (char *) &descr->pss_obj_list->psq_dbp_name),
		&descr->pss_obj_list->psq_dbp_name,
		psf_trmwhite(sizeof(DB_DBP_NAME),
		    (char *) &cur_dbp->pss_obj_list->psq_dbp_name),
		&cur_dbp->pss_obj_list->psq_dbp_name);
#endif
	}
    }
    else
    {
	/*
	** independent object list associated with "descr" has not been
	** processed yet - move "descr" from its parent's children list to
	** cur_dbp's children list
	*/

	if (ps133)
	{
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"<<<\nWill move '%t' from the list of children of '%t'\
\nto the list of children of '%t'.\n>>>\n---",
		psf_trmwhite(sizeof(DB_DBP_NAME), (char *) &obj->psq_dbp_name),
		&obj->psq_dbp_name,
		psf_trmwhite(sizeof(DB_DBP_NAME),
		    (char *) &descr->pss_parent->pss_obj_list->psq_dbp_name),
		&descr->pss_parent->pss_obj_list->psq_dbp_name,
		psf_trmwhite(sizeof(DB_DBP_NAME),
		    (char *) &cur_dbp->pss_obj_list->psq_dbp_name),
		&cur_dbp->pss_obj_list->psq_dbp_name);
	}

	if (descr->pss_parent->pss_child == descr)
	{
	    /* descr is the first in the list of its parent's chidlren */
	    descr->pss_parent->pss_child = descr->pss_sibling;
	}
	else
	{
	    PSS_DBPLIST_HEADER	    *child;

	    /*
	    ** find the child immediately preceeding "descr" in its parent's
	    ** children list and unlink descr from the list
	    */

	    for (child = descr->pss_parent->pss_child;
	         child->pss_sibling != descr;
		 child = child->pss_sibling
		)
	    ;

	    child->pss_sibling = descr->pss_sibling;
	}

	descr->pss_sibling = cur_dbp->pss_child;
	cur_dbp->pss_child = descr;
	descr->pss_parent = cur_dbp;
    }
    
    return(TRUE);
}

/*
** Name: psy_tbl_grant_check()	- check if a user may grant a permit on a given
**				  table or view.
**
** Description:
**
**	This function will ascertain that a given user may grant specified
**	privilege(s) on a given object.  In particular, user U1 may grant permit
**	specifying <action> ACT on an object X if
**	 (1) - X is a base table owned by U1, or
**	 (2) - X is a view owned by U1 which was determined to be
**	       "always grantable" at creation time and ACT is SELECT, or
**	 (3) - X is a view owned by U1 which was determined to be
**	       "always grantable" and updateable (updateable is meant in a very
**	       narrow sence, i.e. single-relation view with no UNION,
**	       aggregates, or DISTINCT) at creation time (for the time being
**	       we will not store an indicator of whether a view is updateable,
**	       so this is a moot point) and ACT is INSERT, DELETE, or UPDATE, or
**	 (4) - X is a view owned by U1 and ACT is SELECT and U1 may grant SELECT
**	       privilege on every object used in the view definition, or
**	 (5) - X is a view owned by U1 and ACT is INSERT or UPDATE and for every
**	       updateable column named in the GRANT/CREATE PERMIT statement U1 
**	       may grant ACT on the underlying column of the underlying object
**	       Y, or
**	 (6) - X is a view owned by U1 and ACT is DELETE and U1 may grant
**	       DELETE on the underlying object of X, or
**	 (7) - X is an object (table or view) owned by another user U2 and U1
**	       was given privilege specifying <action> ACT on X WGO
**
**	For (1) and (2) no processing besides obtaining object description from
**	RDF will be required.
**	
**	For (3) we need to unravel the view definition to verify that it is,
**	indeed, updateable.
**
**	For (4) we will obtain a list of independent privileges for the view
**	(thay are stored in IIPRIV at view creation time) and verify that the
**	user may grant all of them
**
**	For (5) we will unravel view definition (only underlying views will be
**	unravelled, i.e. we will not pay any attention to the objects used
**	outside of the outermost from-list of the view definition) and verify
**	that the user may grant privileges on all specified columns which are
**	updateable.
**
**	For (6) we will unravel view definition (only underlying views will be
**	unravelled, i.e. we will not pay any attention to the objects used
**	outside of the outermost from-list of the view definition) and verify
**	that the user may GRANT DELETE on the view's underlying table.
**
**	For (7) we will verify that the user may grant specified privileges on
**	the specified object
**	
** Input:
**	sess_cb			    PSF session CB
**
**	qmode			    query mode of the statement being processed
**				    (PSQ_PROT or PSQ_GRANT)
**
**	grant_obj_id		    id of the object permit on which is being
**				    created/granted
**
**	tbl_wide_privs		    map of table-wide privileges which are being
**				    granted
**
**	col_specific_privs	    NULL if no column-specific privileges are
**				    requested; otherwise it describes
**				    column-specific privileges and maps of
**				    attributes on which these privileges are 
**				    required; if ps131 is set and we are
**				    processing CREATE PERMIT, the descriptor may
**				    have privilege map set to 0 - this will
**				    happen if CREATE PERMIT was used to create
**				    non GRANT-compatible column-specific permits
**				    and UPDATE was not one of them.  The
**				    attribute map will then be useful for
**				    displaying the type of privileges being
**				    created (part of display produced by ps131)
**				    and nothing else
**
**				    NOTE: no privilege shall appear in both
**				    tbl_wide_privs and col_specific_privs
**
**	psy_flags		    useful info about the privileges being
**				    granted
**	    PSY_ALL_PRIVS	    user specified GRANT ALL [PRIVILEGES]
**	    PSY_EXCLUDE_COLUMNS	    user specified DEFINE/CREATE PERMIT ...
**				    ON <table name> EXCLUDING (<column list>)
**	 PSY_EXCLUDE_UPDATE_COLUMNS user specified GRANT UPDATE EXCLUDING
**				    (<column list>)
**	 PSY_EXCLUDE_INSERT_COLUMNS user specified GRANT INSERT EXCLUDING
**				    (<column list>)
**  PSY_EXCLUDE_REFERENCES_COLUMNS  user specified GRANT REFERENCES EXCLUDING
**                                  (<column list>)
**  	    PSY_SCHEMA_MODE	    processing GRANT issued inside CREATE SCHEMA
**				    statement
**
** Output:
**	tbl_wide_privs		    if the user specified GRANT ALL [PRIVILEGES]
**				    this function will be called with
**				    DB_RETRIEVE, DB_DELETE, DB_INSERT, and
**				    DB_REPLACE bits set; however, if the user
**				    may not grant some of gthe privileges, we
**				    will reset this map to contain only the
**				    privileges that the user may grant.
**				    Otherwise it will be left intact.
**	indep_id
**				    if user is granting privileges on another
**				       user's object
**				    {
**					*indep_id == *grant_obj_id
**				    }
**				    else if the privileges being granted do not
**				            depend on any other privileges (with
**					    a possible exception of SELECT
**					    privilege on his view (so the new
**					    privilege will depend on the view's
**					    independent privilege list))
**				     {
**					 indep_id->db_tab_base == 0
**				     }
**				     else
**				     {
**				         
**				         // privileges are being granted on
**					 // user's view and include at least
**					 // one of INSERT/DELETE/UPDATE and the
**					 // user does not own the view's
**					 // underlying table
**					 *indep_id will contain ID of the object
**					 on which the user must posess
**					 privileges described in
**					 indep_tbl_wide_privs and 
**					 indep_col_specific_privs
**				     }
**					
**				    
**				    
**	indep_tbl_wide_privs	    map of independent table-wide privileges
**				    which are required for the object whose id
**				    is contained in indep_id
**
**	indep_col_specific_privs    description of independent column-specific
**				    privileges which are required for object
**				    whose id is contained in indep_id
**
**	insuf_privs		    TRUE if some required privs were not found
**
**	quel_view		    TRUE if the use attempted to grant 
**				    privileges on his own QUEL view
**
**	err_blk			    error block will be filled in if an error
**				    occurs
**
** Side effects:
**	none
**
** Exceptions:
**	none
**
** History:
**	03-jul-92 (andre)
**	    rewrote once more to take advantage of the view's independent
**	    privilege list stored in IIPRIV if a user is attempting to
**	    grant select on hiw view which is not marked "always grantable"
**	    (case (4) above) + made changes so that privileges required for
**	    INSERT/DELETE/UPDATE would agree with SQL92.
**	14-jul-92 (andre)
**	    if user specified ALL [PRIVILEGES], we will determine which
**	    of the privileges the user may grant and prevent him from granting
**	    the rest of the privileges; only if the user may not grant any of
**	    the privileges, we will generate an error.  This is how SQL92
**	    defines semantics of ALL PRIVILEGES, so if you don't like it, take
**	    it up with X3H2.  As a part of the change, tbl_wide_privs was
**	    changed from (i4) to (i4 *)
**	13-aug-92 (andre)
**	    if the user specified ALL [PRIVILEGES], UPDATE privilege may need
**	    to be trnasferred into the column-specific privilege map if it turns
**	    out that the user may GRANT UPDATE on a subset of columns
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	13-aug-93 (andre)
**	    instead of using vtree_pst_agintree which is TRUE if a view 
**	    definition involves an aggregate ANYWHERE in the definition, we will
**	    check whether PST_AGG_IN_OUTERMOST_SUBSEL is set in 
**	    vtree->pst_mask1.  As a result we will no longer prevent users from
**	    granting DELETE,INSERT,UPDATE on views involving aggregates OUTSIDE
**	    of outermost subselect of the view definition
** 	03-nov-93 (andre)
**	    if processing GRANT which was issued inside CREATE SCHEMA statement,
**	    pass that bit of information to psy_check_privs() which will then 
**	    return an error (rather than just issuing a message) if the user 
**	    issuing GRANT statement inside CREATE SCHEMA on a table or view 
**	    possesses no privileges whatsoever on that object.  This agrees 
**	    with our new and improved reading of SQL92 that considers such 
**	    condition an "Access Rule violation."  
**	
**	    My rationale for handling user errors while processing GRANT in 
**          different manner depending on whether GRANT statement was issued 
**          inside CREATE SCHEMA follows.
**
**	    If processing GRANT statement issued outside of CREATE SCHEMA, I 
**	    propose that we do NOT return an error:
**	      -	ability to grant privilege on multiple objects using a single 
**		GRANT statement is our extension; traditionally, if a user was 
**		unable to grant a privilege on some object, we would issue and 
**		error message and try to proceed on to the next one.  
**
**		Since ability to grant privileges on other users' objects was 
**	  	not available until 6.5, out actions are not, strictly speaking,
**		constrained by the ubiquitous backward compatibility, but I 
**		feel that there is no harm in keeping behaviour of GRANT 
**		statement consistent with previous releases insofar as 
**		contunuing processing even if privilege cannot be granted on 
**		some object is concerned.  
**
**		If a user used SQL92-compliant syntax (i.e. specifying 
**		privileges to be granted on exactly one object), then our 
**		behaviour will be perfectly compliant with SQL92 (i.e. SQLSTATE
**		will be set to 42000 and no privilege will be granted).  If, on 
**		the other hand, user took advantage of our extension and 
**		specified multiple objects on which privileges are to be 
**		granted, we will continue the current policy of granting 
**		privileges on those objects on which the current user may grant
**		them and issuing error messages for the rest.
**
**	    On the other hand, not aborting a CREATE SCHEMA statement containing
**	    GRANT statement(s) attempting to grant privileges on objects in 
**	    other schemas which either do not exist or on which the current 
**	    user possesses no privileges whatsoever, is clearly non-compliant 
**	    with SQL92 and may at some point result in a failure to pass a NIST
**	    test.  Therefore, user errors while processing GRANT statement 
**	    issued inside CREATE SCHEMA will result in an error being returned 
**	    to the caller which will lead to rollback of the entire CREATE 
**	    SCHEMA statement
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
*/
DB_STATUS
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
	DB_ERROR	*err_blk)
{
    RDF_CB		    rdf_vtree_cb, rdf_ipriv_cb;
    RDR_RB		    *rdf_v_rb = &rdf_vtree_cb.rdf_rb,
			    *rdf_i_rb = &rdf_ipriv_cb.rdf_rb;
    PSS_RNGTAB		    *rngvar;
    
    i4		    err_code;
    i4		    mask;
    register PST_QTREE      *vtree;
    DB_STATUS		    status;
    register i4	    i;
    PSY_ATTMAP		    *col_priv_attmap;

    i4			    privs_to_find;
    char		    buff[70];
    bool		    ps131;
    bool		    grant_all = (psy_flags & PSY_ALL_PRIVS) != 0;
    i4			    check_privs_flags = 0;
    char		    trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    {
	i4		val1, val2;

	/* determine if trace point ps131 has been set */
	ps131 = (bool) ult_check_macro(&sess_cb->pss_trace,
				PSS_TBL_VIEW_GRANT_TRACE, &val1, &val2);
    }

    /* 
    ** once and for all figure out which flags will be handed to 
    ** psy_check_privs()
    */
    if (grant_all)
	check_privs_flags |= PSY_PRIVS_GRANT_ALL;
    if (psy_flags & PSY_SCHEMA_MODE)
	check_privs_flags |= PSY_GRANT_ISSUED_IN_CRT_SCHEMA;

    *insuf_privs = *quel_view = FALSE;	    /* optimistic */

    /*
    ** copy map of requested table-wide privileges to the map of independent
    ** table-wide privileges; note that some privileges may have to be moved
    ** from the table-wide list to column-specific list; if a permit WGO is
    ** being granted, turn off WGO bit in *indep_tbl_wide_privs since it is
    ** understood that privileges WGO will be sought
    */
    *indep_tbl_wide_privs = *tbl_wide_privs & ~DB_GRANT_OPTION;

    if (col_specific_privs && col_specific_privs->psy_col_privs)
    {
	/* copy the map of requested column specific privileges */

	STRUCT_ASSIGN_MACRO(*col_specific_privs, *indep_col_specific_privs);

	/*
	** as was explained for the independent table-wide privileges above, we
	** do not want to have DB_GRANT_OPTION bit turned on in the map of
	** independent privileges
	*/
	indep_col_specific_privs->psy_col_privs &= ~DB_GRANT_OPTION;
    }
    else
    {
	/* initialize the map of independent column-specific privileges */

	indep_col_specific_privs->psy_col_privs = (i4) 0;

	psy_fill_attmap(indep_col_specific_privs->psy_attmap->map, ((i4) 0));
	for (i = 1, col_priv_attmap = indep_col_specific_privs->psy_attmap + 1;
	     i < PSY_NUM_PRIV_ATTRMAPS;
	     i++, col_priv_attmap++
	    )
	{
	    STRUCT_ASSIGN_MACRO(indep_col_specific_privs->psy_attmap[0],
				*col_priv_attmap);
	}
    }

    /*
    ** compute the list of privileges (both table-wide and column-specific)
    ** being sought.  If permit is being granted on a view owned by the current
    ** user, privileges in privs_to_find will apply to tables and views used
    ** (directly or indirectly) in the outermost subselect of that view's
    ** definition
    */
    privs_to_find =
	*indep_tbl_wide_privs | indep_col_specific_privs->psy_col_privs;

    /* initialize static fields in rdf_vtree_cb */
    pst_rdfcb_init(&rdf_vtree_cb, sess_cb);

    /* init custom fields in rdf_vtree_cb */
    rdf_v_rb->rdr_types_mask	= RDR_VIEW | RDR_QTREE;
    rdf_v_rb->rdr_qtuple_count	= 1;
    rdf_v_rb->rdr_rec_access_id = NULL;

    /*
    ** must initialize it here in the event that we reach tbl_grant_check_exit:
    ** without ever hitting the code initializing rdf_ipriv_cb
    */
    rdf_i_rb->rdr_rec_access_id = NULL;

    status = E_DB_OK;
    
    /*
    ** need to get the description of the object on which permit is being
    ** granted to get the algorithm going.
    **
    ** if ps131 is set and column-specific privileges are required, get the
    ** column description, otherwise we don't need it
    */
    status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "!", PST_SHWID,
	(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, grant_obj_id,
	!(ps131 && col_specific_privs != (PSY_COL_PRIVS *) NULL),
	&rngvar, qmode, err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    if (ps131)
    {
	i4	    offset;

	psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, "<<<");

	if (qmode == PSQ_GRANT)
	{
	    /* processing GRANT statement */

	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"Attempting to grant the following privileges on\n'%t' \
owned by '%t':\n\n",
		psf_trmwhite(sizeof(DB_TAB_NAME),
		    (char *) &rngvar->pss_tabname), &rngvar->pss_tabname,
		psf_trmwhite(sizeof(DB_OWN_NAME),
		    (char *) &rngvar->pss_ownname), &rngvar->pss_ownname);

	    if (grant_all)
	    {
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\t ALL PRIVILEGES\n");
	    }
	    else if (*indep_tbl_wide_privs)
	    {
		psy_prvmap_to_str(*indep_tbl_wide_privs, buff, DB_SQL);
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\ttable-wide %t;\n", STlength(buff), buff);
	    }

	    if (indep_col_specific_privs->psy_col_privs)
	    {
		char	    *str;
		i4	    str_len;
		PSY_ATTMAP  attmap;

		if (indep_col_specific_privs->psy_col_privs & DB_APPEND)
		{
		    col_priv_attmap =
			indep_col_specific_privs->psy_attmap +
			    PSY_INSERT_ATTRMAP;

		    if (psy_flags & PSY_EXCLUDE_INSERT_COLUMNS)
		    {
			str = "insert excluding";
			str_len = sizeof("insert excluding") - 1;

			/*
			** *col_priv_attmap has all bits turned on except for
			** those "excluded" from the privilege - but we only
			** want to display names of columns which are excluded,
			** so we will make a copy of the attribute map and
			** negate it
			*/
			STRUCT_ASSIGN_MACRO((*col_priv_attmap), attmap);
			BTnot(BITS_IN(attmap), (char *) &attmap);
			col_priv_attmap = &attmap;
		    }
		    else
		    {
			str = "insert";
			str_len = sizeof("insert") - 1;
		    }

		    offset = BTnext(-1, (char *) col_priv_attmap->map,
			DB_MAX_COLS + 1);

		    psy_attmap_to_str(rngvar->pss_attdesc, col_priv_attmap,
			&offset, buff, 60);

		    if (offset == -1)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "\t%t(%t)",
			    str_len, str, STlength(buff), buff);
		    }
		    else
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "\t%t(%t",
			    str_len, str, STlength(buff), buff);

			for (;;)
			{
			    psy_attmap_to_str(rngvar->pss_attdesc,
				col_priv_attmap, &offset, buff, 60);
			    if (offset != -1)
			    {
				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "\t\t%t", STlength(buff), buff);
			    }
			    else
			    {
				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "\t\t%t);", STlength(buff), buff);
				break;
			    }
			}
		    }
		}

		if (indep_col_specific_privs->psy_col_privs & DB_REPLACE)
		{
		    col_priv_attmap =
			indep_col_specific_privs->psy_attmap +
			    PSY_UPDATE_ATTRMAP;
		    
		    if (psy_flags & PSY_EXCLUDE_UPDATE_COLUMNS)
		    {
			str = "update excluding";
			str_len = sizeof("update excluding") - 1;

			/*
			** *col_priv_attmap has all bits turned on except for
			** those "excluded" from the privilege - but we only
			** want to display names of columns which are excluded,
			** so we will make a copy of the attribute map and
			** negate it
			*/
			STRUCT_ASSIGN_MACRO((*col_priv_attmap), attmap);
			BTnot(BITS_IN(attmap), (char *) &attmap);
			col_priv_attmap = &attmap;
		    }
		    else
		    {
			str = "update";
			str_len = sizeof("update") - 1;
		    }

		    offset = BTnext(-1, (char *) col_priv_attmap->map,
			DB_MAX_COLS + 1);

		    psy_attmap_to_str(rngvar->pss_attdesc, col_priv_attmap,
			&offset, buff, 60);

		    if (offset == -1)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "\t%t(%t)",
			    str_len, str, STlength(buff), buff);
		    }
		    else
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "\t%t(%t",
			    str_len, str, STlength(buff), buff);

			for (;;)
			{
			    psy_attmap_to_str(rngvar->pss_attdesc,
				col_priv_attmap, &offset, buff, 60);
			    if (offset != -1)
			    {
				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "\t\t%t", STlength(buff), buff);
			    }
			    else
			    {
				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "\t\t%t)", STlength(buff), buff);
				break;
			    }
			}
		    }
		}

		if (indep_col_specific_privs->psy_col_privs & DB_REFERENCES)
		{
		    col_priv_attmap =
			indep_col_specific_privs->psy_attmap +
			    PSY_REFERENCES_ATTRMAP;
		    
		    if (psy_flags & PSY_EXCLUDE_REFERENCES_COLUMNS)
		    {
			str = "references excluding";
			str_len = sizeof("references excluding") - 1;

			/*
			** *col_priv_attmap has all bits turned on except for
			** those "excluded" from the privilege - but we only
			** want to display names of columns which are excluded,
			** so we will make a copy of the attribute map and
			** negate it
			*/
			STRUCT_ASSIGN_MACRO((*col_priv_attmap), attmap);
			BTnot(BITS_IN(attmap), (char *) &attmap);
			col_priv_attmap = &attmap;
		    }
		    else
		    {
			str = "references";
			str_len = sizeof("references") - 1;
		    }

		    offset = BTnext(-1, (char *) col_priv_attmap->map,
			DB_MAX_COLS + 1);

		    psy_attmap_to_str(rngvar->pss_attdesc, col_priv_attmap,
			&offset, buff, 60);

		    if (offset == -1)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "\t%t(%t)",
			    str_len, str, STlength(buff), buff);
		    }
		    else
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "\t%t(%t",
			    str_len, str, STlength(buff), buff);

			for (;;)
			{
			    psy_attmap_to_str(rngvar->pss_attdesc,
				col_priv_attmap, &offset, buff, 60);
			    if (offset != -1)
			    {
				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "\t\t%t", STlength(buff), buff);
			    }
			    else
			    {
				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "\t\t%t)", STlength(buff), buff);
				break;
			    }
			}
		    }
		}
	    }
	}
	else
	{
	    if (col_specific_privs)
	    {
		char	    *str;
		i4	    str_len;
		PSY_ATTMAP  attmap;
		
		if (col_specific_privs->psy_col_privs & DB_REPLACE)
		{
		    col_priv_attmap =
			col_specific_privs->psy_attmap + PSY_UPDATE_ATTRMAP;
		}
		else
		{
		    col_priv_attmap = col_specific_privs->psy_attmap;
		}
		
		if (psy_flags & PSY_EXCLUDE_COLUMNS)
		{
		    str = " excluding";
		    str_len = sizeof(" excluding") - 1;

		    /*
		    ** *col_priv_attmap has all bits turned on except for
		    ** those "excluded" from the privilege - but we only
		    ** want to display names of columns which are excluded,
		    ** so we will make a copy of the attribute map and
		    ** negate it
		    */
		    STRUCT_ASSIGN_MACRO((*col_priv_attmap), attmap);
		    BTnot(BITS_IN(attmap), (char *) &attmap);
		    col_priv_attmap = &attmap;
		}
		else
		{
		    str = "";
		    str_len = 0;
		}

		offset = BTnext(-1, (char *) col_priv_attmap->map,
		    DB_MAX_COLS + 1);

		psy_attmap_to_str(rngvar->pss_attdesc, col_priv_attmap,
		    &offset, buff, 60);
		
		if (offset == -1)
		{
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"Attempting to create the following privileges \
on\n%t%t(%t):",
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname,
			str_len, str, STlength(buff), buff);
		}
		else
		{
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"Attempting to create the following privileges \
on\n%t%t(%t",
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname,
			str_len, str, STlength(buff), buff);

		    for (;;)
		    {
			psy_attmap_to_str(rngvar->pss_attdesc,
			    col_priv_attmap, &offset, buff, 60);
			if (offset != -1)
			{
			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"\t\t%t", STlength(buff), buff);
			}
			else
			{
			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"\t\t%t):", STlength(buff), buff);
			    break;
			}
		    }
		}
		psy_prvmap_to_str(
		  *indep_tbl_wide_privs |
		      indep_col_specific_privs->psy_col_privs,
		    buff, DB_SQL);
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\t%t\n", STlength(buff), buff);
	    }
	    else
	    {
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "Attempting to create the following privileges on\n\
%t:",
		    psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &rngvar->pss_tabname), &rngvar->pss_tabname);

		psy_prvmap_to_str(*indep_tbl_wide_privs, buff, DB_SQL);
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\ttable-wide %t\n", STlength(buff), buff);
	    }
	}
	psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, ">>>\n---");
    }

    indep_id->db_tab_base = indep_id->db_tab_index = (i4) 0;

    /*
    ** here we are relying on the fact that pst_rgent() is storing range
    ** variable entries on LRU basis.  This allows us to find the correct range
    ** entry by grabbing it from the front of the queue
    */
    for (; privs_to_find != (i4) 0;)
    {
	rngvar = (PSS_RNGTAB *) sess_cb->pss_auxrng.pss_qhead.q_next;
	
	mask = rngvar->pss_tabdesc->tbl_status_mask;

	/*
	**  if (the object is a view)
	**  {
	**	if (this is not the view on which the original
	**	    GRANT/CREATE PERMIT was issued
	**	    &&
	**	    object owner == current user && view is "always grantable"
	**	    &&
	**	    (view is updateable || the privileges sought on the
	**	     object include none of INSERT, DELETE, or UPDATE)
	**	   )
	**	{
	**	    // in the course of unraveling the outermost from-list of
	**	    // the view on which privileges are being granted, we came
	**	    // across a view on which we are guaranteed that the user
	**	    // is allowed to grant the desired privileges on it; we do
	**	    // not need to obtain the view tree nor do we need to
	**	    // unravel the view any further.  If the set of privileges
	**	    // being granted includes SELECT, we need to verify that the
	**	    // user possesses SELECT WGO on all objects appearing in the
	**	    // original view's independent privilege list; otherwise we
	**	    // are done
	**	    //
	**	    // until we have means of recording that a given view is
	**	    // updateable (need another relstat word)
	**	    // (view is updateable) will always evaluate to FALSE
	**
	**	    if (privs_to_find include DB_RETRIEVE)
	**		set privs_to_find to DB_RETRIEVE to ensure that we
	**		verify that the user posesses SELECT WGO on all objects
	**		in the original view's independent privilege list
	**	    else
	**	        notify caller that the user may grant desired
	**		privilege(s)
	**	}
	**	else
	**	{
	**	    obtain the view tree
	**
	**	    if (this is an object on which the original
	**		GRANT/CREATE PERMIT was issued)
	**	    {
	**		if this is not an SQL view,
	**		    issue error and notify caller that the user may not
	**		    grant any privileges on this object
	**	    }
	**
	**	    if (object owner == current user && view is grantable
	**	        &&
	**		(view is updateable || the privileges sought on the
	**		 object include none of INSERT, DELETE, or UPDATE)
	**	       )
	**	    {
	**		// this must be the view on which the privilege(s)
	**		// are being granted
	**		// we are guaranteed that the user is allowed to
	**		// grant the desired privileges on the object;
	**		//
	**		// until we have means of recording that a given
	**		// view is updateable (need another relstat word)
	**		// (view is updateable) will always evaluate to FALSE
	**
	**		notify caller that the user may grant desired
	**		privilege(s)
	**	    }
	**	}
	**  }
	**  else if (object owner == current user)
	**  {
	**	// If this is NOT the object on which the privileges are being
	**	// granted and the set of privileges being granted includes
	**	// SELECT, we need to verify that the user possesses SELECT WGO
	**	// on all objects appearing in the original view's independent
	**	// privilege list;
	**	// otherwise we know that the current user can grant all
	**	// desired privileges on the specified object 
	**
	**	if (   privs_to_find include DB_RETRIEVE
	**	    && this is not the object on which privileges are being
	**	       created)
	**	    set privs_to_find to DB_RETRIEVE to ensure that we
	**	    verify that the user posesses SELECT WGO on all objects
	**	    in the original view's independent privilege list
	**	else
	**	    notify caller that the user may grant desired privilege(s)
	**  }
	*/

	if (mask & DMT_VIEW)
	{
	    if (   (rngvar->pss_tabid.db_tab_base != grant_obj_id->db_tab_base
	            ||
	           rngvar->pss_tabid.db_tab_index != grant_obj_id->db_tab_index)
		&& mask & DMT_VGRANT_OK
		&& !MEcmp((PTR) &rngvar->pss_ownname,
			  (PTR) &sess_cb->pss_user, sizeof(sess_cb->pss_user))
		&& (/* view is UPDATEABLE || */
		    (privs_to_find & (DB_REPLACE | DB_DELETE | DB_APPEND)) ==
			((i4) 0)
		   )
	       )
	    {
		/*
		** user may grant required privilege(s) on the view's underlying
		** view - if the privileges to be granted include SELECT, we
		** need to verify that the user possesses SELECT WGO on all
		** objects in the view's independent privilege list
		*/

		if (privs_to_find &= DB_RETRIEVE)
		{
		    /* remember that we have not read the view's tree */
		    vtree = (PST_QTREE *) NULL;
		}
#ifdef xDEBUG
		if (ps131 && privs_to_find)
		{
		    DB_TAB_NAME	    *tab_name =
			&sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"<<<\nSkip permit checking on '%t'.",
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname);
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"Current user owns it and is guaranteed to be \
able to grant desired privileges on it.");
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"We will now verify that the user possesses \
SELECT WGO on all objects\nin the independent privilege list associated with \
'%t'.\n>>>\n---",
			psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tab_name),
			tab_name);
		}
#endif
	    }
	    else
	    {
		/* obtain the view tree */
		STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_v_rb->rdr_tabid);
		rdf_vtree_cb.rdf_info_blk = rngvar->pss_rdrinfo;

		status = rdf_call(RDF_GETINFO, (PTR) &rdf_vtree_cb);

	        /*
	        ** RDF may choose to allocate a new info block and return its
		** address in rdf_info_blk - we need to copy it over to
		** pss_rdrinfo to avoid memory leak and other assorted
		** unpleasantries
	        */
		rngvar->pss_rdrinfo = rdf_vtree_cb.rdf_info_blk;
		
		if (DB_FAILURE_MACRO(status))
		{
		    _VOID_ psf_rdf_error(RDF_GETINFO, &rdf_vtree_cb.rdf_error,
			err_blk);
		    break;
		}

		/* Make vtree point to the view tree retrieved */
		vtree = ((PST_PROCEDURE *) rdf_vtree_cb.rdf_info_blk->
		    rdr_view->qry_root_node)->pst_stmts->pst_specific.pst_tree;

		/*
		** if this is a view on which a permit is being created/granted,
		** make sure that it is an SQL view
		*/
		if (   rngvar->pss_tabid.db_tab_base ==
		       grant_obj_id->db_tab_base
		    && rngvar->pss_tabid.db_tab_index ==
		       grant_obj_id->db_tab_index
		    && vtree->pst_qtree->pst_sym.pst_value.pst_s_root.pst_qlang
			    != DB_SQL
		   )
		{
		    _VOID_ psf_error(3598L, 0L, PSF_USERERR, &err_code,
			err_blk, 1,
			psf_trmwhite(sizeof(rngvar->pss_tabname),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname);

		    /*
		    ** this is one of "expected" errors so we will not set
		    ** status to E_DB_ERROR but we will notify caller that the
		    ** user may not grant privileges on this object because it
		    ** is a QUEL view
		    */
		    *quel_view = TRUE;
		    goto tbl_grant_check_exit;
		}

		if (mask & DMT_VGRANT_OK
		    && !MEcmp((PTR) &rngvar->pss_ownname,
		           (PTR) &sess_cb->pss_user, sizeof(sess_cb->pss_user))
		    && (/* view is UPDATEABLE || */
		        (privs_to_find & (DB_REPLACE | DB_DELETE | DB_APPEND))
			    == ((i4) 0)
		       )
		   )
		{
		    /*
		    ** rngvar contains description of the object on which the
		    ** privilege(s) are being granted - user obviously possesses
		    ** all of required privileges WGO
		    */
		    privs_to_find = (i4) 0;
		}
	    }
	}
	else if (!MEcmp((PTR) &rngvar->pss_ownname,
			(PTR) &sess_cb->pss_user, sizeof(sess_cb->pss_user))
		)
	{
	    /* object is a base table owned by the current user */
	    
	    if (   rngvar->pss_tabid.db_tab_base != grant_obj_id->db_tab_base
		|| rngvar->pss_tabid.db_tab_index != grant_obj_id->db_tab_index)
	    {
		/*
		** underlying table of the view on which a privilege being
		** granted is a base table owned by the current user - he/she
		** definitely may any one of DELETE/INSERT/UPDATE on his view -
		** to determine whether he may grant SELECT, we will verify that
		** he/she posesses GRANT OPTION FOR select privilege on all
		** objects in the view's independent privilege list
		*/
		if (privs_to_find &= DB_RETRIEVE)
		{
		    /* remember that we have not read the view's tree */
		    vtree = (PST_QTREE *) NULL;
		}
#ifdef xDEBUG
		if (ps131 && privs_to_find)
		{
		    DB_TAB_NAME	    *tab_name =
			&sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"<<<\nSkip permit checking on '%t'.",
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname);
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"Current user owns it and is guaranteed to be \
able to grant desired privileges on it.");
		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"We will now verify that the user possesses \
SELECT WGO on all objects\nin the independent privilege list associated with \
'%t'.\n>>>\n---",
			psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tab_name),
			tab_name);
		}
#endif
	    }
	    else
	    {
		/* user may grant all privileges on his base table */
		privs_to_find = (i4) 0;
	    }
	}

	if (privs_to_find == (i4) 0)
	{
	    /*
	    ** user may grant desired privileges
	    */

#ifdef xDEBUG
	    if (ps131)
	    {
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "<<<\nSkip permit checking on '%t'.",
		    psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &rngvar->pss_tabname), &rngvar->pss_tabname);
		psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "Current user owns it and is guaranteed to be able \
to grant desired privileges on it\n>>>\n---");
	    }
#endif
	}
	/*
	** if the object is owned by a user different from the current one,
	** we will look for applicable permits only if it is a base table or
	** an SQL view.
	*/
	else if (MEcmp((PTR) &rngvar->pss_ownname, (PTR) &sess_cb->pss_user,
		     sizeof(sess_cb->pss_user))
	         &&
	         (
		     ~mask & DMT_VIEW
		     ||
		     vtree->pst_qtree->pst_sym.pst_value.pst_s_root.pst_qlang
		         == DB_SQL
	         )
	        )
	{
		/*
		** this will be filled with a list of privileges which were
		** "fully" satisfied, i.e. user was determined to possess all of
		** privileges required to grant privileges in fully_satisfied;
		** When processing GRANT ALL, it is possible that a privilege
		** (such as UPDATE) could be granted only on a subset of
		** columns - if that was the case, DB_REPLACE will be turned off
		** in privs_to_find, but will NOT be turned on in
		** fully_satisfied +
		** indep_col_specific_privs->psy_attmap[PSY_UPDATE_ATTRMAP] will
		** describe attributes on which the user possesses UPDATE WGO
		*/
	    i4          fully_satisfied;

	    /*
	    ** remember id of the object on which the user must posess specified
	    ** privilege(s) WGO
	    */
	    indep_id->db_tab_base = rngvar->pss_tabid.db_tab_base;
	    indep_id->db_tab_index = rngvar->pss_tabid.db_tab_index;
	    
	    /*
	    ** for objects not owned by the current user, we need to look
	    ** for applicable privileges WGO
	    */

	    status = psy_check_privs(rngvar, &privs_to_find,
		*indep_tbl_wide_privs, indep_col_specific_privs, sess_cb, ps131,
		grant_obj_id, check_privs_flags, &fully_satisfied, err_blk);

	    if (DB_FAILURE_MACRO(status))
		break;

	    if (privs_to_find)
	    {
		/*
		** if the user specified ALL [PRIVILEGES], but may grant only
		** some (but not none) of the valid table/view privileges, we
		** will simply remove the privileges for which he does not
		** possess GRANT OPTION from the list of privileges that he will
		** grant and the list of independent privileges; otherwise
		** generate an error
		*/
		if (grant_all && ps131)
		{
		    psy_prvmap_to_str(privs_to_find, buff, DB_SQL);

		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			"<<<");

		    if (   grant_obj_id->db_tab_base == 
			       rngvar->pss_tabid.db_tab_base
			&& grant_obj_id->db_tab_index ==
			       rngvar->pss_tabid.db_tab_index)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "User specified ALL [PRIVILEGES] on '%t' \
\nowned by '%t', but he/she lacks %t WGO on that object. \
\nThe privileges that will be generated will not include %t",
			    psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &rngvar->pss_tabname),
			    &rngvar->pss_tabname,
			    psf_trmwhite(sizeof(rngvar->pss_ownname),
				(char *) &rngvar->pss_ownname),
			    &rngvar->pss_ownname,
			    STlength(buff), buff,
			    STlength(buff), buff);
		    }
		    else
		    {
			DB_TAB_NAME	    *tab_name =
			    &sess_cb->pss_auxrng.pss_rsrng.pss_tabname;
			    
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "User specified ALL [PRIVILEGES] on \
view '%t' \
\nowned by him, but he/she lacks %t WGO on '%t' \
\nowned by '%t' on which the user's view is defined.\
\nThe privileges that will be generated will not include %t",
			    psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) tab_name),
			    tab_name,
			    STlength(buff), buff,
			    psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &rngvar->pss_tabname),
			    &rngvar->pss_tabname,
			    psf_trmwhite(sizeof(rngvar->pss_ownname),
				(char *) &rngvar->pss_ownname),
			    &rngvar->pss_ownname,
			    STlength(buff), buff);
		    }

		    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			">>>\n---");
		}

		if (   !grant_all
		    || privs_to_find ==
			(*indep_tbl_wide_privs | 
			    indep_col_specific_privs->psy_col_privs)
		   )
		{
		    if (grant_all && ps131)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<");

			if (   grant_obj_id->db_tab_base == 
				   rngvar->pss_tabid.db_tab_base
			    && grant_obj_id->db_tab_index ==
				   rngvar->pss_tabid.db_tab_index)
			{
			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"User specified ALL [PRIVILEGES] on \
'%t' \nowned by '%t', but he/she may not grant any privileges on that object",
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) &rngvar->pss_tabname),
				&rngvar->pss_tabname,
				psf_trmwhite(sizeof(rngvar->pss_ownname),
				    (char *) &rngvar->pss_ownname),
				&rngvar->pss_ownname);
			}
			else
			{
			    DB_TAB_NAME	    *tab_name =
				&sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"User specified ALL [PRIVILEGES] on \
view '%t' owned by the user, \
\nbut he/she may not grant any privileges on that view",
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) tab_name),
				tab_name);
			}

			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    ">>>\n---");
		    }

		    *insuf_privs = TRUE;
		    break;
		}
	    }

	    /*
	    ** if processing GRANT ALL and some of privileges were not
	    ** fully_satisfied, reset maps of table-wide privileges that will be
	    ** granted as well as the maps of independent table-wide and
	    ** column-specific privileges.
	    ** If user can grant UPDATE or REFERENCES (soon to be joined by 
	    ** INSERT) on a subset of columns, DB_REPLACE or DB_REFERENCES (soon
	    ** to be joined by DB_APPEND) will NOT be set in either 
	    ** fully_satisfied or privs_to_find
	    */
	    if (   grant_all
		&& fully_satisfied !=
		    (*indep_tbl_wide_privs |
			indep_col_specific_privs->psy_col_privs)
	       )
	    {
		/*
		** reset the map of privileges that the user may grant -
		** only those that were fully_satisfied should be left in
		** *tbl_wide_privs
		*/
		*tbl_wide_privs = fully_satisfied;

		/*
		** reset the map of applicable independent table-wide
		** privileges - only those that were in
		** *indep_tbl_wide_privs to begin with and were also
		** fully_satisfied must remain in *indep_tbl_wide_privs
		*/
		*indep_tbl_wide_privs &= fully_satisfied;

		/*
		** reset the map of applicable independent column-specific
		** privileges
		*/
		indep_col_specific_privs->psy_col_privs &= ~privs_to_find;

		/*
		** since SQL92 interprets ALL PRIIVLEGES as "all grantable
		** privileges which are possessed by the current auth id",
		** the user so far appears to possess some privileges (which
		** is what is required), so set privs_to_find to 0
		*/
		privs_to_find = 0;
	    }

	    /* 
	    ** if processing GRANT ALL and the user possesses UPDATE or 
	    ** REFERENCES WGO only on a subset of required columns, set 
	    ** DB_REPLACE or DB_REFERENCES in col_specific_privs->psy_col_privs;
	    **
	    ** if the object on which we were checking privileges was not the
	    ** object which was mentioned in the GRANT statement itself, we will
	    ** need to invoke psy_b2v_col_xlate() to determine on which of the
	    ** columns of the view named in the GRANT statement user may grant
	    ** UPDATE privilege; otherwise we simply copy the 
	    ** column map from indep_col_specific_privs[PSY_UPDATE_ATTRMAP].map
	    ** into col_specific_privs->psy_attmap[PSY_UPDATE_ATTRMAP].map
	    ** 
	    ** in case of REFERENCES, we copy the column map from
	    ** indep_col_specific_privs[PSY_REFERENCES_ATTRMAP].map into
	    ** col_specific_privs->psy_attmap[PSY_REFERENCES_ATTRMAP].map
	    */
	    if (grant_all)
	    {
		if(   indep_col_specific_privs->psy_col_privs & DB_REPLACE
		   && ~fully_satisfied & DB_REPLACE
	          )
	        {
		    col_specific_privs->psy_col_privs |= DB_REPLACE;

		    if (   grant_obj_id->db_tab_base !=
		               rngvar->pss_tabid.db_tab_base
		        || grant_obj_id->db_tab_index !=
			       rngvar->pss_tabid.db_tab_index
		       )
		    {
		        status = psy_b2v_col_xlate(grant_obj_id, 
			    col_specific_privs->psy_attmap[PSY_UPDATE_ATTRMAP].
									    map,
			    &rngvar->pss_tabid,
			    indep_col_specific_privs->
			        psy_attmap[PSY_UPDATE_ATTRMAP].map);
		        if (DB_FAILURE_MACRO(status))
		        {
			    err_blk->err_code = E_PS0002_INTERNAL_ERROR;
			    goto tbl_grant_check_exit;
		        }
		    }
		    else
		    {
		        i4		i;
		        i4		*map_to, *map_from;

		        map_to = col_specific_privs->
			    psy_attmap[PSY_UPDATE_ATTRMAP].map;
		        map_from = indep_col_specific_privs->
			    psy_attmap[PSY_UPDATE_ATTRMAP].map;

		        for (i = 0; i < DB_COL_WORDS; i++)
			    map_to[i] = map_from[i];
		    }
	        }

		if(   indep_col_specific_privs->psy_col_privs & DB_REFERENCES
		   && ~fully_satisfied & DB_REFERENCES
	          )
	        {
		    i4		i;
		    i4		*map_to, *map_from;

		    /* REFERENCES may only be granted on base tables */

		    col_specific_privs->psy_col_privs |= DB_REFERENCES;

		    map_to = col_specific_privs->
			psy_attmap[PSY_REFERENCES_ATTRMAP].map;
		    map_from = indep_col_specific_privs->
		        psy_attmap[PSY_REFERENCES_ATTRMAP].map;

		    for (i = 0; i < DB_COL_WORDS; i++)
		        map_to[i] = map_from[i];
	        }
	    }

	    /* 
	    ** if the user was trying to grant at least one of INSERT,
	    ** DELETE, UPDATE in addition to SELECT on his/her view, we
	    ** still need to verify that he may grant SELECT on objects
	    ** used in the view's qualification.  This is the case since
	    ** the code that unravels view definition only concerns
	    ** itself with objects used in the outermost  FROM-list of
	    ** the view definition
	    */

	    if (   *indep_tbl_wide_privs & DB_RETRIEVE
		&& (   grant_obj_id->db_tab_base !=
		           rngvar->pss_tabid.db_tab_base
		    || grant_obj_id->db_tab_index !=
			   rngvar->pss_tabid.db_tab_index
		   )
	       )
	    {
		/* 
		** if the independent privilege list included SELECT and
		** the table/view privileges on which we have just
		** established is NOT the object on which privilege(s) is/are
		** being granted (i.e. user was granting privileges on
		** his/her view), set privs_to_find to DB_RETRIEVE so that
		** the following test succeeds and we verify that the user
		** may grant SELECT on objects (if any) in the view's
		** qualification.  *indep_id contains id of the object which we
		** have just processed and we will avoid checking privileges on
		** it (since we have already done it)
		*/

		privs_to_find = DB_RETRIEVE;
	    }
	}

	/*
	** Case when a user is trying to grant SELECT on his view which is
	** not marked as "always grantable" will be handled separately:
	**	    we will fetch from IIPRIV the view's independent privilege
	**	    list and for every object X in that list verify that the
	**	    user posesses SELECT WGO on X
	*/
	if (privs_to_find == DB_RETRIEVE)
	{
#define	    PSY_IIPRIV_TUPLES	    5
	    DB_IIPRIV		iipriv_tups[PSY_IIPRIV_TUPLES];
	    DB_IIPRIV		*priv_tup;
	    i4			privtup_count;
	    
	    /* initialize static fields in rdf_ipriv_cb */
	    pst_rdfcb_init(&rdf_ipriv_cb, sess_cb);
		
	    /* 
	    ** it is important that we use grant_obj_id rather than
	    ** rngvar->pss_tabid since we could find ourselves here after
	    ** partially unravelling the view definition in which case
	    ** rngvar->pss_tabid will not contain id of the object 
	    ** privilege(s) on which is/are being granted
	    */
	    STRUCT_ASSIGN_MACRO((*grant_obj_id), rdf_i_rb->rdr_tabid);

	    rdf_i_rb->rdr_types_mask	= RDR_VIEW;
	    rdf_i_rb->rdr_2types_mask	= RDR2_INDEP_PRIVILEGE_LIST;
	    rdf_i_rb->rdr_qrytuple	= (PTR) iipriv_tups;
	    rdf_i_rb->rdr_update_op	= RDR_OPEN;

	    /*
	    ** we will fetch elements of the view's independent privilege list,
	    ** PSY_IIPRIV_TUPLES at a time and verify that the user may grant
	    ** SELECT on objects on which the view depends;
	    ** If we find one object on which the user may not grant SELECT,
	    ** search will be terminated and the caller will be advised that the
	    ** user lacks required privileges to grant specified privilege
	    */
	    while (rdf_ipriv_cb.rdf_error.err_code == 0)
	    {
		rdf_i_rb->rdr_qtuple_count = PSY_IIPRIV_TUPLES;
		
		status = rdf_call(RDF_READTUPLES, (PTR) &rdf_ipriv_cb);
		if (status != E_DB_OK)
		{
		    DB_ERROR	    *err = &rdf_ipriv_cb.rdf_error;
		    
		    if (err->err_code == E_RD0011_NO_MORE_ROWS)
		    {
			status = E_DB_OK;
		    }
		    else if (err->err_code == E_RD0013_NO_TUPLE_FOUND)
		    {
			/*
			** apparently the user possesses SELECT WGO on all
			** objects in the view's independent privilege list (if
			** any).  If the independent privilege list is
			** non-empty, privs_to_find would be set to 0 here, but
			** if it is empty, it would be set to DB_RETRIEVE and
			** needs to be reset to indicate that the user does not
			** lack any of required privileges
			*/
			privs_to_find = (i4) 0;

			status = E_DB_OK;

			break;	    /* no more tuples to process */
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_READTUPLES, err, err_blk);
			goto tbl_grant_check_exit;
		    }
		}

		rdf_i_rb->rdr_update_op = RDR_GETNEXT;

		for (privtup_count = rdf_i_rb->rdr_qtuple_count,
		     priv_tup = iipriv_tups;

		     privtup_count > 0;

		     priv_tup++, privtup_count--
		    )
		{
		    PSS_RNGTAB      *newrngvar;
		    i4		    fully_satisfied;
		    
		    /*
		    ** if we have already verified that the user
		    ** posesses all required privileges on this object,
		    ** skip it
		    */
		    if (   indep_id->db_tab_base ==
			       priv_tup->db_indep_obj_id.db_tab_base
			&& indep_id->db_tab_index ==
			       priv_tup->db_indep_obj_id.db_tab_index
		       )
		    {
			/*
			** reset privs_to_find to 0 to indicate that the user
			** posesses required privileges on this object
			*/
			privs_to_find = 0;
			continue;
		    }

		    /*
		    ** for each object listed in the view's independent
		    ** privilege list, determine whether the user posesses
		    ** SELECT WGO on that object
		    */
		    status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "!",
			PST_SHWID, (DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL,
			&priv_tup->db_indep_obj_id, TRUE,
			&newrngvar, qmode, err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			goto tbl_grant_check_exit;
		    }

		    /*
		    ** we are done with the entry pointed to by rngvar; now
		    ** we mark is as "unused" and stick it in the end of the
		    ** queue 
		    */

		    rngvar->pss_used = FALSE;
		    QUremove((QUEUE *) rngvar);
		    QUinsert((QUEUE *) rngvar, 
			sess_cb->pss_auxrng.pss_qhead.q_prev);

		    rngvar = newrngvar;

		    privs_to_find = DB_RETRIEVE;

		    status = psy_check_privs(rngvar, &privs_to_find,
			*indep_tbl_wide_privs, indep_col_specific_privs,
			sess_cb, ps131, grant_obj_id, check_privs_flags,
			&fully_satisfied, err_blk);

		    if (DB_FAILURE_MACRO(status))
			goto tbl_grant_check_exit;
		    else if (privs_to_find)
		    {
			/*
			** if the user specified GRANT ALL [PRIVILEGES],
			**   having privs_to_find set simply indicates that he
			**   will not be allowed to grant RETRIEVE.
			**   If so far it looks like he/she may grant at least
			**   one privilege,
			**     simply turn off DB_RETRIEVE bit and skip checking
			**     the rest of elements of independent privilege
			**     list
			**   else
			**     treat it as error
			**   endif
			** else
			**   treat it as error
			** endif
			*/

			if (grant_all && ps131)
			{
			    DB_TAB_NAME	    *tab_name =
				&sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"<<<");

			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"User specified ALL \
[PRIVILEGES] on view '%t' \nowned by him, but he/she lacks SELECT WGO on '%t'\n\
owned by '%t' which was used in the view's definition.  \nThe privileges that \
will be generated will not include SELECT\n",
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) tab_name),
				tab_name,
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) &rngvar->pss_tabname),
				&rngvar->pss_tabname,
				psf_trmwhite(sizeof(rngvar->pss_ownname),
				    (char *) &rngvar->pss_ownname),
				&rngvar->pss_ownname);

			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				">>>\n---");
			}

			if (   grant_all
			    && *tbl_wide_privs &
			           ~ (i4) (DB_RETRIEVE | DB_GRANT_OPTION)
			   )
			{
			    /*
			    ** reset DB_RETRIEVE in the map of privileges that
			    ** the user may grant
			    */
			    *tbl_wide_privs &= ~((i4) DB_RETRIEVE);

			    /*
			    ** reset DB_RETRIEVE in the map of applicable
			    ** independent table-wide privileges
			    */
			    *indep_tbl_wide_privs &= ~((i4) DB_RETRIEVE);

			    /*
			    ** reset DB_RETRIEVE in the map of applicable
			    ** independent column-specific privileges;
			    ** 
			    ** currently this bit will never be set in the
			    ** independent column-specific privilege map, but
			    ** some day, some day...
			    */
			    indep_col_specific_privs->psy_col_privs &=
				~((i4) DB_RETRIEVE);

			    /* don't have to look for SELECT privilege */
			    privs_to_find = 0;

			    /* no point in checking the rest of objects */
			    break;
			}
			else
			{
			    if (grant_all && ps131)
			    {
				DB_TAB_NAME	    *tab_name =
				    &sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "<<<");

				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    "User specified ALL \
[PRIVILEGES] on view '%t' \
\nowned by him, but he/she may not grant any privileges on that view",
				    psf_trmwhite(sizeof(DB_TAB_NAME),
					(char *) tab_name),
				    tab_name);

				psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				    ">>>\n---");
			    }

			    *insuf_privs = TRUE;
			    goto tbl_grant_check_exit;
			}
		    }
		}

		if (grant_all && ~*tbl_wide_privs & (i4) DB_RETRIEVE)
		{
		    /*
		    ** if processing GRANT ALL [PRIVILEGES], we may have
		    ** determined that the user may not grant SELECT on the object
		    ** but so far appears to be able to grant some other
		    ** privilege(s) - do not read any more IIPRIV tuples
		    */
		    break;
		}
	    }
	}
	/*
	** we could find ourselves here after verifying that the user
	** posesses the required privileges in which case we do not want
	** to do any more porcessing - that explains if (privs_to_find)
	** below
	*/
	else if (privs_to_find)
	{
	    /*
	    ** we get here only if (the object is a view owned by the current
	    ** user or if it is a QUEL view owned by another user)
	    ** AND the set of privileges being granted includes some
	    ** privilege other than SELECT 
	    */

	    PSS_RNGTAB	    *newrngvar;
	    bool	    get_attrs;
	    PST_RT_NODE	    *vtree_root = 
				&vtree->pst_qtree->pst_sym.pst_value.pst_s_root;

	    /*
	    ** if the privileges required for this view include at least one of
	    ** DELETE, INSERT, or UPDATE (which is TRUE since otherwise (as was
	    ** mentioned above) we would never find ourselves in this fragment
	    ** of code), check whether it is updateable, i.e. its definition
	    ** does not involve aggergates, set  functions (so far we only
	    ** support UNION), or DISTINCT and the outermost FROM-list consists
	    ** of exactly one relation
	    **
	    ** if the view is not updateable and the privileges were specified
	    ** explicitly (i.e. the user did not GRANT ALL [PRIVILEGES]), this
	    ** will be treated as an error; otherwise we will simply prevent the
	    ** user from granting INSERT, DELETE, UPDATE.
	    */

	    if (   vtree->pst_mask1 & PST_AGG_IN_OUTERMOST_SUBSEL
		|| vtree_root->pst_union.pst_next 
		|| vtree_root->pst_dups != PST_ALLDUPS 
		|| BTcount((char *) &vtree_root->pst_tvrm, PST_NUMVARS) != 1
	       )
	    {
		if (grant_all)
		{
		    i4	    updt_privs =
			(i4) (DB_DELETE | DB_APPEND | DB_REPLACE);

		    if (ps131)
		    {
			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    "<<<");

			if (   grant_obj_id->db_tab_base == 
				   rngvar->pss_tabid.db_tab_base
			    && grant_obj_id->db_tab_index ==
				   rngvar->pss_tabid.db_tab_index)
			{
			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"User specified ALL [PRIVILEGES] on \
view '%t' owned by him, \nbut that view is not updateable.  The privileges \
that will be generated \nwill not include DELETE, INSERT, or UPDATE",
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) &rngvar->pss_tabname),
				&rngvar->pss_tabname);
			}
			else
			{
			    DB_TAB_NAME	    *tab_name =
				&sess_cb->pss_auxrng.pss_rsrng.pss_tabname;
				
			    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
				"User specified ALL [PRIVILEGES] on \
view '%t' owned by him, \nbut that view is defined on top of view '%t' (also \
owned by him) \nwhich is not updateable .  The privileges that will be \
generated \nwill not include DELETE, INSERT, or UPDATE",
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) tab_name),
				tab_name,
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) &rngvar->pss_tabname),
				&rngvar->pss_tabname,
				psf_trmwhite(sizeof(DB_TAB_NAME),
				    (char *) tab_name),
				tab_name);
			}

			psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
			    ">>>\n---");
		    }

		    /* reset the map of privileges that the user may grant */
		    *tbl_wide_privs &= ~updt_privs;

		    /*
		    ** reset the map of applicable independent table-wide
		    ** privileges
		    */
		    *indep_tbl_wide_privs &= ~updt_privs;

		    /*
		    ** reset the map of applicable independent column-specific
		    ** privileges
		    */
		    indep_col_specific_privs->psy_col_privs &= ~updt_privs;

		    /*
		    ** skip to the end of the loop so we will look at this view
		    ** again, except that this time we will only aspire to grant
		    ** SELECT
		    */

		    /*
		    ** reset privs_to_find to indicate that the best we can
		    ** hope for is a SELECT privilege
		    */
		    privs_to_find = DB_RETRIEVE;
		    
		    continue;
		}
		else
		{
		    /*
		    ** user tried to explicitly grant at least one of DELETE,
		    ** INSERT, UPDATE on a non-updateable view
		    */

		    psy_prvmap_to_str(
			privs_to_find & (DB_APPEND | DB_DELETE | DB_REPLACE),
			buff, DB_SQL);

		    if (   grant_obj_id->db_tab_base == 
			       rngvar->pss_tabid.db_tab_base
			&& grant_obj_id->db_tab_index ==
			       rngvar->pss_tabid.db_tab_index)
		    {
			/*
			** the view was named in the GRANT or CREATE PERMIT
			*/
			_VOID_ psf_error(E_PS0553_NONUPDATEABLE_VIEW,
			    0L, PSF_USERERR, &err_code, err_blk, 2,
			    psf_trmwhite(sizeof(rngvar->pss_tabname),
				(char *) &rngvar->pss_tabname),
			    &rngvar->pss_tabname,
			    STlength(buff), buff);
		    }
		    else
		    {
			DB_TAB_NAME	*tab_name =
			    &sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

			_VOID_ psf_error(E_PS0554_NONUPDT_VIEW_IN_DEFN,
			    0L, PSF_USERERR, &err_code, err_blk, 4,
			    psf_trmwhite(sizeof(DB_TAB_NAME), 
				(char *) tab_name),
			    tab_name,
			    psf_trmwhite(sizeof(rngvar->pss_tabname),
				(char *) &rngvar->pss_tabname),
			    &rngvar->pss_tabname,
			    psf_trmwhite(sizeof(rngvar->pss_ownname),
				(char *) &rngvar->pss_ownname),
			    &rngvar->pss_ownname,
			    STlength(buff), buff);
		    }

		    *insuf_privs = TRUE;
		    break;
		}
	    }

	    /*
	    ** will enter the underlying table into the range variable
	    ** table
	    */
	    i = BTnext(-1, (char *) &vtree_root->pst_tvrm, PST_NUMVARS);

	    /*
	    ** if PS131 is set and we may end up looking for
	    ** column-specific privileges on this object, get the
	    ** description of its attributes 
	    */
	    if (ps131 && privs_to_find & (DB_REPLACE | DB_APPEND))
	    {
		get_attrs = TRUE;
	    }
	    else
	    {
		get_attrs = FALSE;
	    }

	    status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "!",
		PST_SHWID, (DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL,
		&vtree->pst_rangetab[i]->pst_rngvar, !get_attrs,
		&newrngvar, qmode, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto tbl_grant_check_exit;
	    }

	    /*
	    ** Table-wide or column-specific UPDATE and INSERT privilege 
	    ** on the view will be converted to column-specific privilege on the
	    ** relevant updateable columns of the underlying object 
	    ** (we are doing it for INSERT privileges in anticipation of 
	    ** introduction of column-specific GRANT-compatible INSERT 
	    ** privileges.)  More precisely, if we need table-wide UPDATE 
	    ** or INSERT privilege, we will find attributes of the underlying 
	    ** object used in the definition of the updateable attributes 
	    ** of the view and enter them into the attribute map associated 
	    ** with an appropriate column-specific privilege.
	    ** If a column-specific UPDATE or INSERT privilege on the view 
	    ** is required, we will translate references to updateable 
	    ** view attributes into references to the attributes of the 
	    ** object over which the view is defined
	    */

	    if (*indep_tbl_wide_privs & DB_REPLACE)
	    {
	        status = psy_tbl_to_col_priv(indep_tbl_wide_privs,
		    &indep_col_specific_privs->psy_col_privs,
		    indep_col_specific_privs->psy_attmap +
		        PSY_UPDATE_ATTRMAP,
		    (i4) DB_REPLACE, vtree->pst_qtree, err_blk);

		if (DB_FAILURE_MACRO(status))
		{
		    goto tbl_grant_check_exit;
		}
	    }
	    else if (indep_col_specific_privs->psy_col_privs & DB_REPLACE)
	    {
	        status = psy_colpriv_xlate(vtree->pst_qtree,
		    &indep_col_specific_privs->psy_col_privs,
		    indep_col_specific_privs->psy_attmap +
			PSY_UPDATE_ATTRMAP, (i4) DB_REPLACE,
		    rngvar->pss_tabdesc->tbl_attr_count, err_blk);

		if (DB_FAILURE_MACRO(status))
		{
		    goto tbl_grant_check_exit;
		}
	    }

	    if (*indep_tbl_wide_privs & DB_APPEND)
	    {
	        status = psy_tbl_to_col_priv(indep_tbl_wide_privs,
		    &indep_col_specific_privs->psy_col_privs,
		    indep_col_specific_privs->psy_attmap +
		        PSY_INSERT_ATTRMAP,
		    (i4) DB_APPEND, vtree->pst_qtree, err_blk);

		if (DB_FAILURE_MACRO(status))
		{
		    goto tbl_grant_check_exit;
		}
	    }
	    else if (indep_col_specific_privs->psy_col_privs & DB_APPEND)
	    {
		status = psy_colpriv_xlate(vtree->pst_qtree,
		    &indep_col_specific_privs->psy_col_privs,
		    indep_col_specific_privs->psy_attmap +
		        PSY_INSERT_ATTRMAP, (i4) DB_APPEND,
		    rngvar->pss_tabdesc->tbl_attr_count, err_blk);

		if (DB_FAILURE_MACRO(status))
		{
		    goto tbl_grant_check_exit;
		}
	    }
	}

	/*
	** we are done with the entry pointed to by rngvar; now we mark is
	** as "unused" and stick it in the end of the queue
	*/

	rngvar->pss_used = FALSE;
	QUremove((QUEUE *) rngvar);
	QUinsert((QUEUE *) rngvar, sess_cb->pss_auxrng.pss_qhead.q_prev);
    } 

tbl_grant_check_exit:

    /* make sure we close IIPRIV tuple stream */
    if (rdf_i_rb->rdr_rec_access_id)
    {
	DB_STATUS   local_status;

	rdf_i_rb->rdr_update_op = RDR_CLOSE;
	local_status = rdf_call(RDF_READTUPLES, (PTR) &rdf_ipriv_cb);
	if (DB_FAILURE_MACRO(local_status))
	{
	    _VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_ipriv_cb.rdf_error, err_blk);
	    if (local_status > status)
	    {
		status = local_status;
	    }
	}
    }

    /*
    ** If psy_tbl_grant_check() completes successfully, all entries in
    ** sess_cb->pss_auxrng queue will have been marked as "unused".  If, on the
    ** other hand, it was determined that the user lacks the necessary
    ** privileges to grant the requested privilege or the user may not
    ** grant any privileges on this object because it is a QUEL view, there will
    ** be some entries in sess_cb->pss_auxrng which will be marked as "used" and
    ** whose range number will be greater than -1.  If we don't clean up the 
    ** range table, and there are more tables to be processed for this 
    ** statement, next invocation of psy_tbl_grant_check() could come back with
    ** "too many entries in the range table" error.
    */
    if ((*insuf_privs || *quel_view) && DB_SUCCESS_MACRO(status))
    {
	/* mark the remaining entries in pss_auxrng as unused */
	for (;
	      (rngvar != (PSS_RNGTAB *) &sess_cb->pss_auxrng.pss_qhead &&
	       rngvar->pss_rgno != -1 && rngvar->pss_used)
	     ;
	      rngvar = (PSS_RNGTAB *) rngvar->pss_rngque.q_next
	    )
	{
	    rngvar->pss_used = FALSE;
	}
    }

    return(status);
}

/*
** Name: psy_check_privs - verify that the user posesses required
**			   privileges
** Description:
**    
**    Given a range table entry describing an object and a map of
**    table wide privileges required on the object and a structure
**    describing column-specific privileges and columns on which they
**    are required, this function will scan IIPROTECT to verify that
**    the user possesses the specified privileges WGO.
**
** Input:
**     rngvar			range table entry describing the
**     				table/view in question
**     privs_to_find		address of a map of required privileges
**     indep_tbl_wide_privs	map of required table-wide privileges
**     indep_col_specific_privs	structure descring required
**     				column-specific privileges
**     sess_cb			PSF session CB
**     ps131			an indicator of whether trace point
**     				ps131 has been set
**     grant_obj_id		id of the object named by user (e.g.
**     				the table/view on which a user wishes to 
**				grant privilege(s))
**     flags			one of:
**	    PSY_PRIVS_GRANT_ALL 
**				set if processing GRANT and the user specified 
**				ALL [PRIVILEGES];
**     				   
**     	    PSY_PRIVS_NO_GRANT_OPTION
**     			        set if WITH GRANT OPTION is not required
**     			        for these privileges (used for checking
**     			        REFERENCES priv for referential constraints)
**     				   
**	    PSY_PRIVS_DONT_PRINT_ERROR
**     			        don't print out grant-specific errors
**     			        (used when checking REFERENCES priv for
**     			        referential constraints) 
**
**	    PSY_GRANT_ISSUED_IN_CRT_SCHEMA
**				processing GRANT issued inside CREATE SCHEMA 
**				statement; if a user attempting to grant 
**				privilege(s) possesses no privileges on the 
**				object (which, in SQL92 vernacular, constitutes
**				"Access Rule violation"), status of E_DB_ERROR 
**				will be returned which will cause the 
**				CREATE SCHEMA statement to be rolled back
** Output:
**     
**     privs_to_find		0 if user posesses all required
**     				privileges; map of privileges which the 
**				user does not posess otherwise
**     fully_satisfied          map of privileges which were fully satisfied;
**				this is only relevant when we are processing 
**				GRANT ALL and want to find out which privileges 
**				were fully satisfied (privileges such as UPDATE
**				and soon - INSERT and REFERENCES could
**				be partially satisfied)
**     err_blk			filled in if an error occurs
**
** Side effects:
**     causes IIPROTECT to be opened, reads tuples, and closes it
** 
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**
**	04-jul-92 (andre)	
**	    written
**	15-jul-92 (andre)
**	    added grant_all to the interface
**	14-aug-92 (andre)
**	    added fully_satisfied to the interface
**
**	    if while processing GRANT ALL it turns out that a privilege such as
**	    UPDATE was only partially satisfied, we will store a map of
**	    attributes on which a user possesses UPDATE WGO in the
**	    indep_col_specific_privs and set DB_REPLACE in
**	    indep_col_specific_privs->psy_col_privs
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	08-dec-92 (andre)
**	    made changes associated with adding support for REFERENCES privilege
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	18-mar-93 (rblumer)
**	    Changed grant_all parameter into a flags field,
**	    and added no-grant-option flag & no-error flag.
**	02-nov-93 (andre)
**	    if the user attempting to grant a privilege on object X possesses 
**	    no privileges whatsoever on X, rather than issuing a message saying
**	    that he cannot grant some privilege(s) on X, we will issue a message
**	    saying that EITHER X does not exist OR the user may not access it.
**	03-nov-93 (andre)
**	    a new bit - PSY_GRANT_ISSUED_IN_CRT_SCHEMA - may be set in flags to
**	    notify us that the GRANT statement was issued inside CREATE SCHEMA 
**	    in which case if a user possesses no privileges whatsoever on the 
**	    object named in CREATE SCHEMA statement, in addition to issuing the
**	    E_US088E error message, we will return a status of E_DB_ERROR which
**	    will lead to rollback of CREATE SCHEMA statement (which is what 
**	    should happen since, according to SQL92, this constitutes an 
**	    "Access Rule violation".
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
**	11-feb-94 (swm)
**	    Bug #59614
**	    My previous integration (bug #56448) erroneously introduced a
**	    block of code during integrate merging. This had the side-effect
**	    of failing the WGO runall test because US088E is issued instead
**	    of PS055[12] when a user specifies some privileges which they
**	    may not grant.
**	02-Jun-1997 (shero03)
**	    Update the info_blk in the pss_rdrinfo after calling rdf_call
*/
DB_STATUS
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
	DB_ERROR	    *err_blk)
{
    DB_STATUS		status;
    RDF_CB		rdfcb, *rdf_prot_cb = &rdfcb;
    char		buff[70];
    PSY_ATTMAP		*col_priv_attmap;
    bool		apply;

				/* 
				** will be used to remember whether the 
				** <auth id> attempting to grant privilege(s)
				** him/herself possesses any privilege on the 
				** object; 
				** will start off pessimistic, but may be reset 
				** to TRUE if we find at least one permit 
				** granting ANY privilege to <auth id> 
				** attempting to grant the privilege or to 
				** PUBLIC
				*/
    bool		cant_access = TRUE;

    i4			grant_all;
    i4			grant_option;
    RDR_RB		*rdf_p_rb = &rdf_prot_cb->rdf_rb;

			/* map used to represent a "table-wide privilege" */
    PSY_ATTMAP		all_attrs;
    PSY_ATTMAP		updt_attmap, insrt_attmap, ref_attmap;
    PSY_ATTMAP		*attmap;
    i4			privs_requested = *privs_to_find;
    i4			all_privs =
	indep_tbl_wide_privs | indep_col_specific_privs->psy_col_privs;
    char		trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    *fully_satisfied = 0;

    grant_all    = (flags & PSY_PRIVS_GRANT_ALL) != 0;
    grant_option = (flags & PSY_PRIVS_NO_GRANT_OPTION) ? 0 : DB_GRANT_OPTION;

    if (ps131)
    {
	i4	privs;

	psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
	    "<<<\nCurrent user must have the following \
privileges WGO on\n'%t' owned by '%t':\n\n",
	    psf_trmwhite(sizeof(DB_TAB_NAME), (char *) &rngvar->pss_tabname),
	    &rngvar->pss_tabname,
	    psf_trmwhite(sizeof(DB_OWN_NAME), (char *) &rngvar->pss_ownname),
	    &rngvar->pss_ownname);

	if (privs = *privs_to_find & indep_tbl_wide_privs)
	{
	    psy_prvmap_to_str(privs, buff, DB_SQL);
	    psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		"\ttable-wide %t;\n", STlength(buff), buff);
	}

	if (privs = *privs_to_find & indep_col_specific_privs->psy_col_privs)
	{
	    if (privs & DB_APPEND)
	    {
	        psy_pr_cs_privs(DB_APPEND, 
		    indep_col_specific_privs->psy_attmap + PSY_INSERT_ATTRMAP,
		    rngvar->pss_attdesc);
	    }

	    if (privs & DB_REPLACE)
	    {
		psy_pr_cs_privs(DB_REPLACE,
		    indep_col_specific_privs->psy_attmap + PSY_UPDATE_ATTRMAP,
		    rngvar->pss_attdesc);
	    }

	    if (privs & DB_REFERENCES)
	    {
		psy_pr_cs_privs(DB_REFERENCES,
		    indep_col_specific_privs->psy_attmap + 
			PSY_REFERENCES_ATTRMAP,
		    rngvar->pss_attdesc);
	    }
	}
	psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, ">>>\n---");
    }

    /* initialize static fields in rdf_prot_cb */
    pst_rdfcb_init(rdf_prot_cb, sess_cb);

    /* init custom fields in rdf_prot_cb */
    rdf_p_rb->rdr_types_mask	= RDR_PROTECT;
    rdf_p_rb->rdr_qtuple_count	= 20;		/* Get 20 permits at a time */
    rdf_p_rb->rdr_rec_access_id = NULL;

    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_p_rb->rdr_tabid);
    rdf_prot_cb->rdf_info_blk = rngvar->pss_rdrinfo;
    rdf_p_rb->rdr_update_op = RDR_OPEN;
    rdf_prot_cb->rdf_error.err_code = 0;

    /* build a table-wide privilege attribute map */
    psy_fill_attmap(all_attrs.map, ~((i4) 0));

    /*
    ** when checking for UPDATE privilege, prochk() modifies the
    ** attribute map passed to it.  Since we will need this map later to
    ** build a list of independent privileges, if column-specific UPDATE
    ** WGO is required, we will make a copy of the map of attributes on
    ** which it is required and will pass it to prochk();
    ** on the other hand, if table-wide UPDATE WGO is required, we
    ** need to pass to prochk() an attribute map indicating that fact
    */
    if (*privs_to_find & DB_REPLACE)
    {
	attmap = (indep_tbl_wide_privs & DB_REPLACE)
	    ? &all_attrs
	    : indep_col_specific_privs->psy_attmap + PSY_UPDATE_ATTRMAP;

	STRUCT_ASSIGN_MACRO((*attmap), updt_attmap);
    }

    /*
    ** when checking for INSERT privilege, prochk() modifies the
    ** attribute map passed to it.  Since we will need this map later to
    ** build a list of independent privileges, if column-specific INSERT
    ** WGO is required, we will make a copy of the map of attributes on
    ** which it is required and will pass it to prochk();
    ** on the other hand, if table-wide INSERT WGO is required, we
    ** need to pass to prochk() an attribute map indicating that fact
    */
    if (*privs_to_find & DB_APPEND)
    {
	attmap = (indep_tbl_wide_privs & DB_APPEND)
	    ? &all_attrs
	    : indep_col_specific_privs->psy_attmap + PSY_INSERT_ATTRMAP;

	STRUCT_ASSIGN_MACRO((*attmap), insrt_attmap);
    }

    /*
    ** when checking for REFERENCES privilege, prochk() modifies the
    ** attribute map passed to it.  Since we will need this map later to
    ** build a list of independent privileges, if column-specific REFERENCES
    ** WGO is required, we will make a copy of the map of attributes on
    ** which it is required and will pass it to prochk();
    ** on the other hand, if table-wide REFERENCES WGO is required, we
    ** need to pass to prochk() an attribute map indicating that fact
    */
    if (*privs_to_find & DB_REFERENCES)
    {
	attmap = (indep_tbl_wide_privs & DB_REFERENCES)
	    ? &all_attrs
	    : indep_col_specific_privs->psy_attmap + PSY_REFERENCES_ATTRMAP;

	STRUCT_ASSIGN_MACRO((*attmap), ref_attmap);
    }

    while (rdf_prot_cb->rdf_error.err_code == 0 && *privs_to_find)
    {
	i4		tupcount;
	QEF_DATA	*qp;

	status = rdf_call(RDF_READTUPLES, (PTR) rdf_prot_cb);

        /*
        ** RDF may choose to allocate a new info block and return its address in
        ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
        ** leak and other assorted unpleasantries
        */
	rngvar->pss_rdrinfo = rdf_prot_cb->rdf_info_blk;

	/*
	** I am not using DB_FAILURE_MACRO() because rdf_getinfo() returns
	** E_DB_WARN if there are fewer than 20 tuples
	*/
	if (status != E_DB_OK)
	{
	    if (rdf_prot_cb->rdf_error.err_code == E_RD0011_NO_MORE_ROWS)
	    {
		status = E_DB_OK;
	    }
	    else if (rdf_prot_cb->rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	    {
		status = E_DB_OK;
		continue;
	    }
	    else
	    {
		_VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_prot_cb->rdf_error,
		    err_blk);
		goto priv_check_exit;
	    }
	}

	if (rdf_p_rb->rdr_update_op == RDR_OPEN)
	{
	    rdf_p_rb->rdr_update_op = RDR_GETNEXT;
	}

	/* for each permit tuple */
	for
	(
	 qp = rdf_prot_cb->rdf_info_blk->rdr_ptuples->qrym_data,
	 tupcount = 0;
	 tupcount < rdf_prot_cb->rdf_info_blk->rdr_ptuples->qrym_cnt;
	 qp = qp->dt_next, tupcount++
	)
	{
	    i4			privs_found;
	    i4			privs;
	    DB_PROTECTION       *protup;

	    protup = (DB_PROTECTION*) qp->dt_data;

	    /*
	    ** If this is a security alarm or has a tree associated
	    ** with it, skip it (recall that this function will be
	    ** called only for GRANT/CREATE PERMIT so we have to
	    ** disregard non-GRANT-compatible permits)
	    */
	    if (protup->dbp_popset & DB_ALARM ||
		protup->dbp_treeid.db_tre_high_time ||
		protup->dbp_treeid.db_tre_low_time)
	    {
		continue;
	    }

	    /* check if this is the correct user, terminal, etc */
	    status = proappl(protup, &apply, sess_cb, err_blk);
	    if (DB_FAILURE_MACRO(status))
		goto priv_check_exit;

	    if (!apply)
		continue;

	    /* 
	    ** remember that the <auth id> attempting to grant a privilege 
	    ** possesses some privilege on the table/view
	    */
	    if (cant_access)
		cant_access = FALSE;

	    /*
	    ** check if the tuple specifies any of the sought
	    ** privileges
	    */

	    /*
	    ** if any of the table-wide privileges (other than UPDATE or 
	    ** REFERENCES which must be checked using separate attribute maps) 
	    ** have not been satisfied, check if this tuple would satisfy them
	    */
	    if (privs = *privs_to_find & 
			  (indep_tbl_wide_privs & ~(DB_REPLACE | DB_REFERENCES))
	       )
	    {
		privs_found = prochk(privs | grant_option,
		    all_attrs.map, protup, sess_cb);
		*privs_to_find &= ~privs_found;
	    }

	    /* check for table-wide UPDATE WGO if necessary */
	    if (privs = *privs_to_find & indep_tbl_wide_privs & DB_REPLACE)
	    {
		privs_found = prochk(DB_REPLACE | grant_option,
		    updt_attmap.map, protup, sess_cb);
		*privs_to_find &= ~privs_found;
	    }
	    
	    /* check for table-wide REFERENCES WGO if necessary */
	    if (privs = *privs_to_find & indep_tbl_wide_privs & DB_REFERENCES)
	    {
		privs_found = prochk(DB_REFERENCES | grant_option,
		    ref_attmap.map, protup, sess_cb);
		*privs_to_find &= ~privs_found;
	    }
	    
	    /*
	    ** check if any of the column-specific privileges are being sought
	    */
	    if (privs =
		    *privs_to_find & indep_col_specific_privs->psy_col_privs)
	    {
		i4	i;
		i4	priv_map = 0;

		/*
		** convert privilege map built using bitwise ops into one that
		** can be accessed using BTnext()
		*/
		if (privs & DB_APPEND)
		    BTset(DB_APPP, (char *) &priv_map);
		if (privs & DB_REPLACE)
		    BTset(DB_REPP, (char *) &priv_map);
		if (privs & DB_REFERENCES)
		    BTset(DB_REFP, (char *) &priv_map);
		
		for (i = -1;
		     (i = BTnext(i, (char *) &priv_map, BITS_IN(priv_map)))
		         > -1;
		    )
		{
		    i4	    priv;

		    switch (i)
		    {
			case DB_APPP:
			{
			    priv = DB_APPEND;

			    /*
			    ** we will pass a copy of attribute map to
			    ** prochk() since it modifies the copy
			    ** passed to it
			    */
			    col_priv_attmap = &insrt_attmap;

			    break;
			}
			case DB_REPP:
			{
			    priv = DB_REPLACE;
			  
			    /*
			    ** we will pass a copy of attribute map to
			    ** prochk() since it modifies the copy
			    ** passed to it
			    */
			    col_priv_attmap = &updt_attmap;

			    break;
			}
			case DB_REFP:
			{
			    priv = DB_REFERENCES;
			  
			    /*
			    ** we will pass a copy of attribute map to
			    ** prochk() since it modifies the copy
			    ** passed to it
			    */
			    col_priv_attmap = &ref_attmap;

			    break;
			}
		    }
		    
		    privs_found = prochk(priv | grant_option,
			    col_priv_attmap->map, protup, sess_cb);

		    *privs_to_find &= ~privs_found;
		}
	    }

	    if (!*privs_to_find)
	    {
		/*
		** we have satisfied all privileges on this
		** object, no reason to continue
		*/
		
		rdf_p_rb->rdr_update_op = RDR_CLOSE;
		status = rdf_call(RDF_READTUPLES, (PTR) rdf_prot_cb);
	        rngvar->pss_rdrinfo = rdf_prot_cb->rdf_info_blk;
		if (DB_FAILURE_MACRO(status))
		{
		    _VOID_ psf_rdf_error(RDF_READTUPLES,
			&rdf_prot_cb->rdf_error, err_blk);
		    goto priv_check_exit;
		}

		/* remember that tuple stream was closed */
		rdf_p_rb->rdr_rec_access_id = NULL;
		break;
	    }
	}	    /* for each permit tuple */
    }	    /*
	    ** while there are more tuples and we haven't satisfied
	    ** the requested privileges
	    */

    /* remember which privileges were fully satisfied */
    *fully_satisfied = privs_requested & ~*privs_to_find;

    /*
    ** if processing GRANT ALL, before we try to determine whether all or
    ** any of the privileges have been satisfied, we need to check whether
    ** UPDATE or REFERENCES privilege was required and whether it was satisfied
    ** on at least one column which would enable us to claim that we found the 
    ** required UPDATE or REFERENCES privilege (with ALL [PRIVILEGES] if you 
    ** find a privilege on a subset of columns, you retroactively recompute the
    ** set of columns on which you wanted the privilege in the first place)
    */

    if (grant_all && privs_requested & *privs_to_find & DB_REPLACE)
    {
        /* 
        ** determine whether the user possessed UPDATE WGO on some
        ** of the required columns of the relation
        */
        i4		i;
        i4		*map_reqd, *map_not_found, *col_privs;
        i4		found = FALSE;

	if (indep_tbl_wide_privs & DB_REPLACE)
	{
	    map_reqd = all_attrs.map;
	}
	else
	{
	    map_reqd =
		indep_col_specific_privs->psy_attmap[PSY_UPDATE_ATTRMAP].map;
	}
	
    	map_not_found = updt_attmap.map;
	col_privs =
	    indep_col_specific_privs->psy_attmap[PSY_UPDATE_ATTRMAP].map;

        for (i = 0;
	     i < DB_COL_WORDS;
	     i++, map_reqd++, map_not_found++, col_privs++
	    )
        {
	    if (*map_not_found != *map_reqd)
	    {
	        found = TRUE;
		*col_privs = *map_reqd & ~*map_not_found;
	    }
	    else
	    {
	        *col_privs = 0;
	    }
        }

        if (found)
        {
	    /* 
	    ** user possesses UPDATE WGO on some (but not all) of the 
	    ** required columns -
	    ** turn on DB_REPLACE bit in indep_col_specific_privs->psy_col_privs
	    ** to indicate that the column-specific privilege which can be
	    ** granted by this user depends on privileges on attributes listed
	    ** in indep_col_specific_privs->psy_attmap[PSY_UPDATE_ATTRMAP]
	    */
	    indep_col_specific_privs->psy_col_privs |= DB_REPLACE;
	    *privs_to_find &= ~DB_REPLACE;
        }
    }

    if (grant_all && privs_requested & *privs_to_find & DB_REFERENCES)
    {
        /* 
        ** determine whether the user possessed REFERENCES WGO on some
        ** of the required columns of the relation
        */
        i4		i;
        i4		*map_reqd, *map_not_found, *col_privs;
        i4		found = FALSE;

	if (indep_tbl_wide_privs & DB_REFERENCES)
	{
	    map_reqd = all_attrs.map;
	}
	else
	{
	    map_reqd =
	       indep_col_specific_privs->psy_attmap[PSY_REFERENCES_ATTRMAP].map;
	}
	
    	map_not_found = ref_attmap.map;
	col_privs =
	    indep_col_specific_privs->psy_attmap[PSY_REFERENCES_ATTRMAP].map;

        for (i = 0;
	     i < DB_COL_WORDS;
	     i++, map_reqd++, map_not_found++, col_privs++
	    )
        {
	    if (*map_not_found != *map_reqd)
	    {
	        found = TRUE;
		*col_privs = *map_reqd & ~*map_not_found;
	    }
	    else
	    {
	        *col_privs = 0;
	    }
        }

        if (found)
        {
	    /* 
	    ** user possesses REFERENCES WGO on some (but not all) of the 
	    ** required columns -
	    ** turn on DB_REFERENCES bit in 
	    ** indep_col_specific_privs->psy_col_privs to indicate that the 
	    ** column-specific privilege which can be granted by this user 
	    ** depends on privileges on attributes listed in 
	    ** indep_col_specific_privs->psy_attmap[PSY_REFERENCES_ATTRMAP]
	    */
	    indep_col_specific_privs->psy_col_privs |= DB_REFERENCES;
	    *privs_to_find &= ~DB_REFERENCES;
        }
    }

    /* if any required privilege was not found,
    ** see if we need to print out an error message
    ** (unless caller asked us not to)
    */
    if ((*privs_to_find) && (~flags & PSY_PRIVS_DONT_PRINT_ERROR))
    {
	i4	    err_code;

	/*
	** loop was left because we ran out of tuples;
	**
	** if the user lacks a single privilege on the table/view, issue a 
	** message which would not let a user figure out whether the table/view 
	** on which he tried to grant privilege existed
	**
	** if the user explicitly specified privilege(s), report here that the
	** user does not posess privilege(s) required to grant the desired
	** privilege(s)
	**
	** If the user entered GRANT ALL [PRIVILEGES], and he possesses at least
	** some of required privileges simply return the map of unsatisfied
	** privileges to the caller; otherwise this is definitely an error
	*/

	if (cant_access)
	{
	    /*
	    ** unless I really screwed things up, this can only happen when we
	    ** check whether a user possesses GRANT OPTION FOR privileges which
	    ** he/she is attempting to grant on the object named in the GRANT 
	    ** statement
	    */
	    _VOID_ psf_error(E_US088E_2190_INACCESSIBLE_TBL, 0L, PSF_USERERR, 
		&err_code, err_blk, 3,
		sizeof("GRANT") - 1, "GRANT",
		psf_trmwhite(sizeof(rngvar->pss_ownname),
		    (char *) &rngvar->pss_ownname),
		&rngvar->pss_ownname,
		psf_trmwhite(sizeof(rngvar->pss_tabname),
		    (char *) &rngvar->pss_tabname),
		&rngvar->pss_tabname);

	    if (flags & PSY_GRANT_ISSUED_IN_CRT_SCHEMA)
	    {
		/* 
		** if GRANT statement was issued inside CREATE SCHEMA statement,
		** we need to return E_DB_ERROR to force rollback of CREATE 
		** SCHEMA statement
		*/
		status = E_DB_ERROR;
	    }
	}
	else if (!grant_all)
	{
	    psy_prvmap_to_str(*privs_to_find, buff, DB_SQL);

	    if (grant_obj_id->db_tab_base == rngvar->pss_tabid.db_tab_base
		&&
		grant_obj_id->db_tab_index == rngvar->pss_tabid.db_tab_index
	       )
	    {
		/*
		** user attempted to grant privileges on another user's object,
		** but appeared to lack GRANT OPTION FOR some of these
		** privileges
		*/
		_VOID_ psf_error(E_PS0551_INSUF_PRIV_GRANT_OBJ,
		    0L, PSF_USERERR, &err_code, err_blk, 3,
		    STlength(buff), buff,
		    psf_trmwhite(sizeof(rngvar->pss_tabname),
			(char *) &rngvar->pss_tabname),
		    &rngvar->pss_tabname,
		    psf_trmwhite(sizeof(rngvar->pss_ownname),
			(char *) &rngvar->pss_ownname),
		    &rngvar->pss_ownname);
	    }
	    else
	    {
		DB_TAB_NAME	*tab_name =
		    &sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

		_VOID_ psf_error(E_PS0552_INSUF_PRIV_INDEP_OBJ,
		    0L, PSF_USERERR, &err_code, err_blk, 5,
		    psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tab_name),
		    tab_name,
		    psf_trmwhite(sizeof(rngvar->pss_tabname),
			(char *) &rngvar->pss_tabname),
		    &rngvar->pss_tabname,
		    psf_trmwhite(sizeof(rngvar->pss_ownname),
			(char *) &rngvar->pss_ownname),
		    &rngvar->pss_ownname,
		    STlength(buff), buff,
		    STlength(buff), buff);
	    }
	}
	else if (all_privs == *privs_to_find)
	{
	    /*
	    ** user issued GRANT ALL PRIIVLEGES but he possess none of required
	    ** grantable privileges
	    */

	    if (grant_obj_id->db_tab_base == rngvar->pss_tabid.db_tab_base
		&&
		grant_obj_id->db_tab_index == rngvar->pss_tabid.db_tab_index
	       )
	    {
		/*
		** user attempted to grant ALL [PRIVILEGES] on another user's
		** object on which he possesses no grantable privileges
		*/
		_VOID_ psf_error(E_PS0560_NOPRIV_ON_GRANT_OBJ,
		    0L, PSF_USERERR, &err_code, err_blk, 2,
		    psf_trmwhite(sizeof(rngvar->pss_tabname),
			(char *) &rngvar->pss_tabname),
		    &rngvar->pss_tabname,
		    psf_trmwhite(sizeof(rngvar->pss_ownname),
			(char *) &rngvar->pss_ownname),
		    &rngvar->pss_ownname);
	    }
	    else
	    {
		DB_TAB_NAME	*tab_name =
		    &sess_cb->pss_auxrng.pss_rsrng.pss_tabname;

		if (all_privs == (i4) DB_RETRIEVE)
		{
		    /*
		    ** user specified GRANT ALL [PRIVILEGES], but we only look
		    ** for SELECT - this could happen if
		    **   - the user was found to possess only grantable SELECT
		    **	   on the view's underlying object and we are checking
		    **	   whether the user possesses grantable SELECT on the
		    **	   objects listed in the view's independent privilege
		    **	   list or
		    **	 - the view on which the user was trying to GRANT ALL
		    **	   turned out to be non-updateable and we are checking
		    **	   whether the user possesses grantable SELECT on the
		    **	   objects listed in the view's independent privilege
		    **	   list
		    */
		    _VOID_ psf_error(E_PS0562_NOPRIV_LACKING_SELECT,
			0L, PSF_USERERR, &err_code, err_blk, 3,
			psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tab_name),
			tab_name,
			psf_trmwhite(sizeof(rngvar->pss_tabname),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname,
			psf_trmwhite(sizeof(rngvar->pss_ownname),
			    (char *) &rngvar->pss_ownname),
			&rngvar->pss_ownname);
		}
		else
		{
		    /*
		    ** user possesses no grantable privilege on another user's
		    ** object on which the user's view is defined
		    */
		    _VOID_ psf_error(E_PS0561_NOPRIV_UNDERLYING_OBJ,
			0L, PSF_USERERR, &err_code, err_blk, 5,
			psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tab_name),
			tab_name,
			psf_trmwhite(sizeof(rngvar->pss_tabname),
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname,
			psf_trmwhite(sizeof(rngvar->pss_ownname),
			    (char *) &rngvar->pss_ownname),
			&rngvar->pss_ownname);
		}
	    }
	}
    }

priv_check_exit:
    /* make sure we close IIPROTECT tuple stream */
    if (rdf_p_rb->rdr_rec_access_id)
    {
	DB_STATUS   local_status;

	rdf_p_rb->rdr_update_op = RDR_CLOSE;
	local_status = rdf_call(RDF_READTUPLES, (PTR) rdf_prot_cb);
	rngvar->pss_rdrinfo = rdf_prot_cb->rdf_info_blk;
	if (DB_FAILURE_MACRO(local_status))
	{
	    _VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_prot_cb->rdf_error, err_blk);
	    if (local_status > status)
	    {
		status = local_status;
	    }
	}
    }

    return(status);
}

/*
** Name: psy_prv_att_map	- build a map of attributes on which a 
**				  privilege is being granted
**
** Description:
**	This function will build a map of attributes on which a privilege is 
**	being granted
**
** Input:
**	attmap		map of attributes to be filled in
**	exclude_cols	TRUE if we are asked to build a map of attributes to 
**			which a privilege will not apply
**	attr_nums	array of attribute numbers 
**	num_cols	number of attributes to which a privilege will/will not 
**			apply
**
** Output:
**	attmap		map describing to which attributes a privilege will or
**			will not apply
**
** Returns:
**	none
**
** Side effects:
**	none
**
** History:
**	7-dec-92 (andre)
**	    written
*/
VOID
psy_prv_att_map(
		char		*attmap,
		bool		exclude_cols,
		i4		*attr_nums,
		i4		num_cols)
{
    i4	*attrid;
    i4		i;

    /*
    ** build the attribute map for a given privilege P
    **
    ** note that the user could have specified column to which P shall apply 
    ** or columns to which it should not apply
    */
    
    if (exclude_cols)
    {
	/*
	** if user specified to which columns privilege should
	** not apply, it will apply to all other attributes as
	** well as to the table itself, i.e. if new columns are
	** added to the table, the privilege will apply to them
	*/
	psy_fill_attmap((i4 *) attmap, ~((i4) 0));

	for (i = 0, attrid = attr_nums; i < num_cols; i++, attrid++)
	{
	    BTclear((i4) *attrid, attmap);
	}
    }
    else
    {
	psy_fill_attmap((i4 *) attmap, ((i4) 0));

	for (i = 0, attrid = attr_nums; i < num_cols; i++, attrid++)
	{
	    BTset((i4) *attrid, attmap);
	}
    }

    return;
}

/*
** Name: psy_prvtxt	- build query text for a permit conveying a 
**			  column-specific privilege
**
** Description:
**	If a user was trying to GRANT ALL on a table, we may have determined 
**	that he/she may only GRANT UPDATE and/or REFERENCES only on a subset of
**	table's columns, in which case this function will be responsible for 
** 	putting together text which will be stored for a permit based on the 
** 	privilege being conveyed and a map of columns to which this privilege 
** 	will apply
**
** Input:
**	priv			privilege being granted
**	    DB_REPLACE		construct text for a column-specific UPDATE
**	    DB_REFERENCES	construct text for a column-specific REFERENCES
**	attrib_map		map of attributes to which the privilege will 
**				apply
**	txt_chain		address of a header of a (pre-opened) text chain
**				which will be used to build text for a permit
**	attdesc			addr of a list of table's attribute descriptions
**	psy_tbl			description of a table privilege on which is 
**				being granted
**	buf			text which must appear between the end of object
**				name and beginning of grantee name
**	buf_len			length of text pointed to by buf
**	grant_option		TRUE if creating a permit WITH GRANT OPTION
**	mstream			pre-opened memory stream which will be used to
**				store completed query text
**
** Output:
**	grntee_type_offset	offset of a grantee type string in the query 
**				text
**	grantee_offset		offset of the grantee name in the query text
**	txt			completed query text
**	err_blk			will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK,ERROR}
**
**
** Side effects:
**	will allocate memory
**
** History:
**	08-dec-92 (andre)
**	    written
**	17-aug-93 (andre)
**	    schema/object/grantee name placeholders are all DB_MAX_DELIMID bytes
**	    long
**
**	    since thecolumn names have not been specified by the user (so we
**	    don't know whether they can be represented by regular identifiers,
**	    they will all be represented by delimited identifiers
*/
static DB_STATUS
psy_prvtxt(
	    PSS_SESBLK		*sess_cb,
	    i4		priv,
	    PSY_ATTMAP		*attrib_map,
	    PTR			txt_chain,
	    DMT_ATT_ENTRY	**attdesc,
	    PSY_TBL		*psy_tbl,
	    i4			*grntee_type_offset,
	    i4			*grantee_offset,
	    char		*buf,
	    i4			buf_len,
	    bool		grant_option,
	    DB_TEXT_STRING	**txt,
	    PSF_MSTREAM		*mstream,
	    DB_ERROR		*err_blk)
{
    DB_STATUS   	status = E_DB_OK;
    i4	    		i;
    u_char      	ident_placeholder[DB_MAX_DELIMID + 1];
    char	    	*attmap;
    PSY_ATTMAP  	excl_map;
    char	    	*str;
    i4	    		str_len;
    PTR	    		piece;
    u_char          	*col_name, *schema_name, *obj_name;
    i4              	col_nm_len, schema_name_len, obj_name_len;
    u_char          	delim_col_name[DB_MAX_DELIMID];
    u_char              delim_schema_name[DB_MAX_DELIMID];
    u_char              delim_obj_name[DB_MAX_DELIMID];

    /* fill in an identifier placeholder */
    MEfill(DB_MAX_DELIMID, '?', (PTR) ident_placeholder);
    ident_placeholder[DB_MAX_DELIMID] = EOS;

    attmap = (char *) attrib_map->map;

    if (BTtest(0, attmap))
    {
        /* 
        ** if bit 0 is set, <privilege> can be granted on a table excluding a 
	** set of columns *attrib_map has all bits turned on except for 
        ** those "excluded" from the privilege - we need to build a negated map
	** using which we can easily derive names of columns excluded from the
        ** privilege
        */
        STRUCT_ASSIGN_MACRO((*attrib_map), excl_map);
        attmap = (char *) excl_map.map;
        BTnot(BITS_IN(excl_map), attmap);
        if (priv == DB_REPLACE)
        {
	    str = "grant update excluding (";
	    str_len = sizeof("grant update excluding (") - 1;
        }
        else
        {
	    /* must be REFERENCES */
	    str = "grant references excluding (";
	    str_len = sizeof("grant references excluding (") - 1;
        }
    }
    else
    {
        if (priv == DB_REPLACE)
        {
	    str = "grant update (";
	    str_len = sizeof("grant update (") - 1;
        }
        else
        {
	    /* must be REFERENCES */
	    str = "grant references (";
	    str_len = sizeof("grant references (") - 1;
        }
    }

    status = psq_tadd(txt_chain, (u_char *) str, str_len, &piece, err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);

    /* 
    ** now add the list of columns; add the first column 
    ** outside of loop to make the loop simpler
    */
    i = BTnext(0, attmap, DB_MAX_COLS + 1);

    col_name = attdesc[i]->att_nmstr;
    col_nm_len = attdesc[i]->att_nmlen;

    status = psl_norm_id_2_delim_id(&col_name, &col_nm_len, delim_col_name, 
	err_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);

    status = psq_tadd(txt_chain, col_name, col_nm_len, &piece, err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);

    for (; (i = BTnext(i, attmap, DB_MAX_COLS + 1)) != -1;)
    {
        status = psq_tadd(txt_chain, (u_char *) ", ", sizeof(", ") - 1, 
	    &piece, err_blk);
        if (DB_FAILURE_MACRO(status))
	    return(status);

        col_name = attdesc[i]->att_nmstr;
        col_nm_len = attdesc[i]->att_nmlen;

        status = psl_norm_id_2_delim_id(&col_name, &col_nm_len, delim_col_name, 
	    err_blk);
        if (DB_FAILURE_MACRO(status))
	    return(status);

        status = psq_tadd(txt_chain, col_name, col_nm_len, &piece, err_blk);
        if (DB_FAILURE_MACRO(status))
            return(status);
    }

    status = psq_tadd(txt_chain, (u_char *) ") on ", sizeof(") on ") - 1, 
        &piece, err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);

    /* 
    ** determine whether the schema name will be represented by a regular or 
    ** delimited identifier, compute that identifier and its length 
    **
    ** schema name will be represented by a regular identifier if the user has
    ** specified it using a regular identifier - otherwise it will be 
    ** represented by a delimited identifier
    */
    schema_name = (u_char *) &psy_tbl->psy_owner;
    schema_name_len = 
	(i4) psf_trmwhite(sizeof(psy_tbl->psy_owner), (char *) schema_name);
    if (~psy_tbl->psy_mask & PSY_REGID_SCHEMA_NAME)
    {
	status = psl_norm_id_2_delim_id(&schema_name, &schema_name_len,
	    delim_schema_name, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    status = psq_tadd(txt_chain, schema_name, schema_name_len, &piece, err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);

    status = psq_tadd(txt_chain, (u_char *) ".", sizeof(".") - 1, &piece, 
	err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);

    /* 
    ** determine whether the object name will be represented by a regular or 
    ** delimited identifier, compute that identifier and its length 
    **
    ** object name will be represented by a regular identifier if the user has
    ** specified it using a regular identifier - otherwise it will be 
    ** represented by a delimited identifier
    */
    obj_name = (u_char *) &psy_tbl->psy_tabnm;
    obj_name_len = 
	(i4) psf_trmwhite(sizeof(psy_tbl->psy_tabnm), (char *) obj_name);
    if (~psy_tbl->psy_mask & PSY_REGID_OBJ_NAME)
    {
	status = psl_norm_id_2_delim_id(&obj_name, &obj_name_len,
	    delim_obj_name, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    status = psq_tadd(txt_chain, obj_name, obj_name_len, &piece, err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);
	
    status = psq_tqrylen(txt_chain, grntee_type_offset);
    if (DB_FAILURE_MACRO(status))
        return(status);

    status = psq_tadd(txt_chain, (u_char *) buf, buf_len, &piece, err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);

    status = psq_tqrylen(txt_chain, grantee_offset);
    if (DB_FAILURE_MACRO(status))
        return(status);

    status = psq_tadd(txt_chain, ident_placeholder, DB_MAX_DELIMID, 
	&piece, err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);
	
    if (grant_option)
    {
        status = psq_tadd(txt_chain, (u_char *) " with grant option", 
	    sizeof(" with grant option") - 1, &piece, err_blk);
        if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* 
    ** the text is ready - collapse it into a contiguous buffer in QSF
    */
    status = psq_tmulti_out(sess_cb, (PSS_USRRANGE *) NULL, txt_chain, mstream, txt,
	err_blk);
    if (DB_FAILURE_MACRO(status))
        return(status);

    return(E_DB_OK);
}

/*
** Name:	psy_pr_cs_privs	- print description of a column-specific 
**				  privilege
**
** Description:
**	Given a privilege, a bit map of attributes to which a privilege applies
**	(or does not apply), and a list of attribute descriptors, this function 
**	will generate text describing the column specific privilege
**
** Input:
**	priv			privilege type (one of DB_APPEND, DB_REPLACE, or
**				DB_REFERENCES)
**	col_priv_attmap		map describing to which attributes a privilege 
**				denoted by priv must (or must not apply)
**	att_list		address of a list of attribute descriptors
**
** Output:
**	none
**
** Side effects:
**	none
**
** Returns:
**	none
**
** History:
**	09-dec-92 (andre)
**	    written
**	11-oct-93 (swm)
**	    Bug #56448
**	    Declared trbuf for psf_display() to pass to TRformat.
**	    TRformat removes `\n' chars, so to ensure that psf_scctrace()
**	    outputs a logical line (which it is supposed to do), we allocate
**	    a buffer with one extra char for NL and will hide it from TRformat
**	    by specifying length of 1 byte less. The NL char will be inserted
**	    at the end of the message by psf_scctrace().
*/
static VOID
psy_pr_cs_privs(
		i4		priv,
		PSY_ATTMAP	*col_priv_attmap,
		DMT_ATT_ENTRY	**att_list)
{
    PSY_ATTMAP  	attmap;
    char		*str;
    i4			str_len;
    i4			offset;
    char		buff[70];
    char		trbuf[PSF_MAX_TEXT + 1]; /* last char for `\n' */

    /*
    ** if bit 0 in the attribute map is set, user is trying to issue
    ** a privilege on another user's table EXCLUDING (<column list>)
    */

    if (BTtest(0, (char *) col_priv_attmap->map))
    {
	if (priv == DB_APPEND)
	{
            str = "insert excluding";
            str_len = sizeof("insert excluding") - 1;
	}
	else if (priv == DB_REPLACE)
	{
            str = "update excluding";
            str_len = sizeof("update excluding") - 1;
	}
	else	/* must be REFERENCES */
	{
            str = "references excluding";
            str_len = sizeof("references excluding") - 1;
	}

        /*
        ** *col_priv_attmap has all bits turned on except for
        ** those "excluded" from the privilege - but we only
        ** want to display names of columns which are excluded,
        ** so we will make a copy of the attribute map and
        ** negate it
        */
        STRUCT_ASSIGN_MACRO((*col_priv_attmap), attmap);
        BTnot(BITS_IN(attmap), (char *) &attmap);
        col_priv_attmap = &attmap;
    }
    else
    {
	if (priv == DB_APPEND)
	{
            str = "insert";
            str_len = sizeof("insert") - 1;
	}
	else if (priv == DB_REPLACE)
	{
            str = "update";
            str_len = sizeof("update") - 1;
	}
	else	/* must be REFERENCES */
	{
            str = "references";
            str_len = sizeof("references") - 1;
	}
    }

    offset = BTnext(0, (char *) col_priv_attmap->map, DB_MAX_COLS + 1);

    psy_attmap_to_str(att_list, col_priv_attmap, &offset, buff, 60);

    if (offset == -1)
    {
        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
	    "\t%t(%t)", str_len, str, STlength(buff), buff);
    }
    else
    {
        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
	    "\t%t(%t", str_len, str, STlength(buff), buff);

        for (;;)
        {
	    psy_attmap_to_str(att_list, col_priv_attmap, &offset, buff, 60);
	    if (offset != -1)
	    {
	        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\t\t%t", STlength(buff), buff);
	    }
	    else
	    {
	        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1,
		    "\t\t%t);", STlength(buff), buff);
	        break;
	    }
        }
    }

    return;
}
