/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

/*
NO_OPTIM=nc4_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cm.h>
#include    <cs.h>
#include    <cv.h>
#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <tmtz.h>
#include    <qu.h>
#include    <tr.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <usererror.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <ulm.h>
#include    <usererror.h>
#include    <scf.h>
#include    <sxf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <qefmain.h>
#include    <psftrmwh.h>
#include    <psyaudit.h>
#include    <usererror.h>

/**
**
**  Name: PSYPERMIT.C - Functions for applying permits
**
**  Description:
**	This module performs the INGRES protection algorithm, as
**	presented in Stonebraker & Rubinstein, "The INGRES Protection
**	System", with a few modifications.
**
**	The basic algorithm is as follows:
**
**	The algorithm is applied once to each variable used in the
**	query.  Each variable has an initial check performed to
**	determine applicability -- if the current user owns the
**	relation, or if the relation is specially marked as being
**	"all access to everyone", then the algorithm is skipped,
**	thereby having effectively no restriction.
**
**	The set of all such variables is computed in 'protect',
**	and then 'dopro' is called to process each of those.  This
**	is so the protection algorithm does not get applied recursively
**	to constraints which define more than one variable.  Notice
**	that this could in itself be a protection violation, if it
**	were acceptable to reference a relation you do not own in a
**	PERMIT statement.
**
**	The effective query mode for this variable is then computed.
**	This is the same as the query mode of the whole query if
**	the variable in question is the result variable, otherwise
**	it is "retrieve" mode.
**
**	The next step is to scan the query tree and create sets of
**	domains referenced.  Four sets are created:
**		uset -- the set of domains updated (actually,
**			referenced in the target list -- on a
**			retrieve, this will be the set of domains
**			retrieved to the user).
**		rset -- the set of domains retrieved in some
**			context other than the left hand side of
**			an equal sign.
**		aset -- the set of domains aggregated.  This only
**			includes domains aggregated with a simple
**			aggregate (not an aggregate function) with
**			no qualification, since it may be possible
**			to come up with too much information other-
**			wise.
**		qset -- the set of domains retrieved for use in
**			a qualification, but never stored.  This
**			includes domains in a qualification of an
**			aggregate or aggregate function.
**	For more details of domains in each of these sets, look at
**	the routine 'makedset'.
**
**	If we had a retrieve operation in the first place, we will
**	then merge 'uset' into 'rset' and clear 'uset', so that
**	now 'uset' only contains domains which are actually updated,
**	and 'rset' contains all domains which are retrieved.
**
**	Now that we know what is referenced, we can scan the protection
**	catalog.  We scan the entire catalog once for each variable
**	mentioned in the query (except as already taken care of as
**	described above).
**
**	We must create a set of all operations on this variable which
**	are not yet resolved, that is, for which no PERMIT statements
**	which qualify have been issued.  We store this set in the
**	variable "noperm".  As PERMIT statements are found, bits will
**	be cleared.  If the variable is not zero by the end of the
**	scan of the protection catalog, then we reject the query,
**	saying that we don't have permission -- giving us default
**	to deny.
**
**	For each tuple in the protection catalog for this relation,
**	we call "proappl" to see if it applies to this query.  This
**	routine checks the user, terminal, time of day, and so forth
**	(in fact, everything which is query-independent) and tells
**	whether this tuple might apply.
**
**	If the tuple passes this initial check, we then do the query-
**	dependent checking.  This amounts to calling "prochk" once
**	for each operation (and domain set) in the query.  What we
**	get back is a set of operations which this tuple applies to.
**	If zero, the tuple doesn't apply at all; otherwise, it
**	applies to at least one operation.  If it applies to some-
**	thing, we call it "interesting".
**
**	For "interesting" tuples, we now get the corresponding
**	qualification (if one exists), and build up the bit-maps
**	(qmaps) for each operation indicating which qualifications
**	are applicable. Also, we mark
**	any operations we found as having been done, by clearing
**	bits in "noperm".
**
**	When we have completed scanning the protection catalog,
**	we check "noperm".  If it is non-zero, then we have some
**	operation for which a PERMIT statement has not been issued,
**	and we issue an error message.  
**
**	Now, using the qmaps, construct the appropriate Boolean
**	combination of quals (disjoin within a single qmap set,
**	and conjoin between qmap sets), minimizing the Boolean
**	expression where possible to avoid unnecessary redundancy.
**	The result, if non-null, is conjoined to the query tree
**	which is re-normalized.
**
**	Then we go on and try the next variable.
**
**	Finally, we return the root of the modified tree.  This
**	tree is guaranteed to have no authorization violations,
**	and may be run as a regular query.
**
**          psy_protect - Apply the protection algorithm
**	    dopro	- Do the protection algorithm recursively
**	    applypq	- Make minimized Boolean expression of quals
**	    gorform	- Form disjunction of all pquals in map
**	    pickqmap	- Select qual map for elimination
**	    makedset	- Make domain sets
**	    proappl	- Test whether protection tuple applicable
**	    prochk	- Query-dependent protection tuple check
**	    pr_set	- Print column set for debugging
**	    pr_qmap	- Print qualification map
**	    btmask	- Clear bits according to mask
**	    btdumpx	- Format a bit map for printing
**
**	    psy_dbpperm - check is a user has permission to execute a given
**			  dbproc (this function is not a part of the above
**			  mentioned algorithm and was included in this module
**			  as it performs a related function
**	    
**          psy_cpyperm - check if a user has permission to copy a table.
**                        This function is not a part of the above
**                        mentioned algorithm and was included in this module
**                        as it performs a related function
**
**	psy_grantee_ok  - check name and type of grantee to determine if the
**			  IIPROTECT tuple is applicable
**
**  psy_check_permits   - having determined the required privileges and the
**			  attributes on which the privileges are required, scan
**			  the permit tuples for this table or view and
**			  applicable to this session
**	    
**  History:
**      23-jun-86 (jeff)    
**          Adapted from version 4.0 protect.c
**      03-sep-86 (seputis)
**          Fixed SCU_INFORMATION call to set array length
**	03-dec-87 (stec)
**	    Change psy_apql interface.
**	04-jan-88 (stec)
**	    General cleanup.
**	23-feb-88 (stec)
**	    When merging permit vars (if qual present), handle
**	    gracefully missing table case.
**	11-may-88 (stec)
**	    Modify for db procs.
**	03-aug-88 (stec)
**	    Fix "append to somebody's table always successful" bug.
**	04-aug-88 (andre)
**	    Added new procedure - psy_dbpperm - to check if a user has
**	    permission to execute a database procedure.
**	19-aug-1988 (andre)
**	    fixed the bug that allowed one to bypass any checking when
**	    qmode==PSQ_RETINTO.  It was definitely wrong, as only the tables
**	    from which data was retrieved are ever placed into rngvar, and
**	    the processing should be identical to one done when qmode is set to
**	    PSQ_RETRIEVE.
**	28-sep-88 (stec)
**	    Correct a typo in psy_dbpperm.
**	04-nov-88 (stec)
**	    For extended catalogs explicit privilege required
**	    (catalog update priv insufficient).
**	    Undo the change above - otherwise utilities like
**	    CREATEDB are likely to have problems.
**	11-nov-88 (stec)
**	    Change defn of time zone (for Unix).
**	30-nov-88 (stec)
**	    Improve error recovery in psy_dbpperm.
**	02-dec-88 (stec)
**	    Change psy_dbpperm to process rdf_call status differently.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    psy_dbpperm() checks for permits defined for group/application id's
**	    proappl()     checks for permits defined for group/application id's
**	    dopro()	  passes proappl() the session's group/application id's
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Use DBGR_USER, DBGR_GROUP, and DBGR_APLID constants for dbp_gtype.
**	03-aug-89 (andre)
**	    If a  user has no permission to retrieve from an index, check
**	    permissions on the underlying table instead.
**      24-jul-89 (jennifer)
**          Added auditing of event if protections cause failure.
**      24-jul-89 (jennifer)
**          Added new routine doaudit to build an audit range table and 
**          and audit alarm permit list if protections pass.
**          This will be attached to statement header and onto the QP.
**	14-sep-89 (andre)
**	    Made changes in conjunction with outer join development (see comment
**	    below.)
**	21-mar-90 (andre)
**	    Replace %o in TRdisplay calls with 0x%x
**      08-aug-90 (ralph)
**          Don't interpret security alarms as permits.
**          Fix apr queueing in doaudit().
**	23-aug-90 (ralph)
**	    Added psy_cpyperm to check permits for COPY.
**	    Audit dbproc execution in psy_dbpperm.
**	    Change interface to psy_dbpperm to include psq_cb and pss_sesblk.
**	10-sep-90 (andre)
**	    fixed bugs # 33040, 33041, and 33042
**		This bug had to do with checking permits on tables/views used in
**	    definition of non-mergeable view s.t. its (i.e. the nonmergeable
**	    view's) permissions were not checked (note that to qualify for such
**	    treatment, the view must have been owned by the current user or must
**	    have been a QUEL view.)
**		The problem is that nonmergeable views are not merged into the
**	    main query tree (surprise, surprise...), and yet we try to look for
**	    attributes of the tables/views used in their definition in the main
**	    query tree.  Clearly, our search always fails, and so we allow users
**	    to retrieve information through nonmergeable views with no regard
**	    for existing permits.
**		The solution is as follows:
**		    1) let R be the range variable used in definition of a
**		       nonmergeable view V, and during this pass psy_protect()
**		       is expected to check if the user has appropriate
**		       permissions on R (note that since nonmergeable views are
**		       not updateable, we only need to determine if the user has
**		       RETRIEVE privilege on some subset of attributes of R)
**		    2) for each table/view which has to be checked for
**		       approrpiate permissions AND which was entered into the
**		       range table because it was used in definition of a
**		       nonmergeable view (note that the only reason we would be
**		       checking permissions of a table/view used in the
**		       definition of V, if V was owned by the current user or
**		       was a QUEL view), an attribute map will be built as
**		       follows:
**			    a) if V was not itself used in the definition of a
**			       nonmergeable view, first the main query tree will
**			       be searched for attributes of V (this will be
**			       done by the function calling psy_protect()),
**			    b) the definition of these attributes of V AND the
**			       qualification tree of V will be searched for
**			       attributes of R;
**		       NOTE: if V itself was used in the definition of some
**			     nonmergeable view V', first a map of attributes of
**			     V used in the main query tree or in the
**			     qualification of V' will be built, and then we will
**			     generate a map of attributes of R used in the main
**			     query tree or in the qualification of V; one can
**			     easily extend this algorithm.
**	14-sep-90 (teresa)
**	    pss_catupd becomes bitflag rather than boolean.
**	16-dec-90 (ralph)
**	    Trigger security_alarm for failed attempts (b34903)
**	    This involves a minor interface change to doaudit().
**	20-feb-91 (andre)
**	    Changed interface for psy_dbpperm(): add a pointer to session CB
**	    and remove pointers to user, group, and role identifiers since they
**	    can all be obtained from the session CB.
**	21-feb-91 (andre)
**	    - changed interface for proappl(): there is no need to supply user,
**	      role, and group identifiers separately since they can be found in
**	      sess_cb;
**	    - defined psy_grantee_ok(); is will be used by psy_dbpperm(),
**	      psy_cpyperm(), and proappl().
**	4-may-1992 (bryanp)
**	    Added support for PSQ_DGTT and PSQ_DGTT_AS_SELECT
**  	24-sep-92 (stevet)
**  	    Implemented the new timezone adjustment method by
**  	    replacing TMzone() with TMtz_search() routine.
**	04-dec-1992 (pholman)
**	    C2: replace obsolete qeu_audit with psy_secaudit.
**	14-dec-92 (walt)
**	    Replaced \T with \t in a TRdisplay format in psy_check_permits to
**	    silence a compiler warning about illegal display formats on Alpha.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	23-mar-93 (smc)
**	    Cast STskipblank function parameter to match prototype. Commented
**	    string following endif.
**	3-apr-1993 (markg)
**	    Moved the doaudit() routine from this file to psyview.c so that
**	    the audit data structures that are built contain the correct
**	    data. Added a new routine do_alarm() which in C2 servers checks
**	    for security alarms which need to be fired for failing queries. 
**	29-jun-93 (andre)
**	    #included CV.H
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	10-nov-93 (anitap)
**	    If pss_ses_flag = PSS_INGRES_PRIV, then do not check the privileges.
**	    $ingres in verifydb should be able to "alter table drop constraint"
**	    on another user's table. Changes in psy_protect().
**	11-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	17-nov-93 (rblumer)
**	    Fix regression in WGO test suite due to misplaced parentheses
**	    around check for PSS_INGRES_PRIV bit.
**	    Also fixed some compiler warnings by removing unused variables,etc.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in prochk().
**	24-jan-1997 (kch)
**	    In the function psy_protect(), we now check for
**	    viewinfo[rngvar->pss_rgno] == NULL during the scan of the range
**	    table. In psy_view(), the view info structure will not be allocated
**	    if
**	    - this view (or a view defined on top of it) is a target of
**	      UPDATE, INSERT, or DELETE, and
**	    - there are rules defined on it, and
**	    - the rules have not been checked to see if any of them apply.
**	    This change fixes bug 80279.
**	23-May-1997 (shero03)
**	    Save the rdr_info_blk after an UNFIX
**	27-oct-1998 (matbe01)
**	    Added NO_OPTIM for NCR (nc4_us5).
**	14-May-1999 (carsu07)
**	    The function psy_check_permits scans through 20 permits at a time
**	    unless the WGO was set. Part of checking this permission involves 
**	    checking if the definition is of a dbproc which appears to be 
**	    grantable so far has WGO.  However if you're not using dbprocs, then
**   	    the if statement fails and privs_wgo is set.
**	    This in turn means no futher permissions are checked. (Bug 80064)
**	8-nov-1999 (nanpr01)
**	    No need to check terminal/time if the permits are granted 
**	    through SQL because these can not be specified.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    pst_treedup(), psy_check_objprivs(), psy_insert_objpriv(),
**	    psy_check_colpriv(), psy_insert_colpriv().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	20-Jul-2004 (schka24)
**	    Fix bug that stopped looking for unconditional permits when
**	    the required privs were satisfied, but only with conditional
**	    permits.
**	21-Nov-2004 (kodse01)
**	    BUG 113504
**	    In function psy_cpyperm, removed the condition where COPY_INTO 
**	    permission is granted if DMT_RETRIEVE_PRO bit is off.
**	28-nov-2007 (dougi)
**	    Added PSQ_REPDYN to all PSQ_DEFCURS tests for cached dynamic.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Defines of other constants.
*/

/*
**      Sizes and numbers of qualifications and qualification maps for use
**	by the process which simplifies qualifications that are appended by
**	the permit process.
*/

#define                 PSY_MQMAP       4   /* Number of qual. maps to keep */
#define			PSY_MPQUAL	128 /* Number of quals. to keep */

/*
**      Macro for allocating bit maps.
*/

#define		    PSYBITMAPSIZE_MACRO(nbits) ((nbits - 1) / BITSPERBYTE + 1)

/*}
** Name: PQMAP - Qualification bit map
**
** Description:
**	This structure gives a bit map of qualifications, for simplifying
**	quals added to a query by the permit process.
**
** History:
**      23-jun-86 (jeff)    
**          Adapted from version 4.0 PQMAP structure
*/
typedef i2          PQMAP[PSYBITMAPSIZE_MACRO(PSY_MPQUAL) / 2 + 1];

/*}
** Name: PSY_QMAP - Qualification map
**
** Description:
**      This structure tells which qualifications have been used for which
**	types of operations.
**
**	There are four entries in the qmap array, for update, retrieve,
**	aggregate, and test respectively.  ("update" meaning update,
**	delete, append, etc;  only one kind of update priv can be required
**	for any one statement, I think, and in any case they are all
**	considered together.)
**
**	q_map has bit N set if permit number N in the psy_pqual array
**		contributes to this privilege.
**	q_applies is used in the check-permits loop to record whether
**		the permit we're examining confers this privilege.
**	q_usemap says whether the privilege is conditional or not.
**		Unconditional privs don't look at the psy_pqual trees.
**	q_unqual also says that we've found an unconditional privilege.
**		(We need it to distinguish "known unconditional" from
**		"we haven't started yet".)
**
** History:
**     23-jun-86 (jeff)
**          Adapted from QMAP structure in 4.0
**     26-jul-91 (andre)
**	    changed q_mode to q_applies
**	20-Jul-2004 (schka24)
**	    rename q_active to q_usemap, use bool.
*/
typedef struct _PSY_QMAP
{
    PQMAP           q_map;              /* Bit map of qualifications */
    bool	    q_applies;		/*
					** TRUE if current tuple applies to the
					** operation whose qualifications are
					** tracked through this structure
					*/
    bool	    q_usemap;		/* TRUE means that this privilege is
					** conditional, or at least we think
					** so, so far;  q_map indicates which
					** permit trees go with this priv.
					** FALSE means that the privilege is
					** granted unconditionally.
					*/

    bool	    q_unqual;		/* TRUE means there is an unqualified
					** permit that applies
					*/
} PSY_QMAP;

/*}
** Name: PSY_PQUAL - Permit qualification
**
** Description:
**      This structure gives a permit qualification, along with a flag
**	telling whether this qualification has been used.
**
** History:
**     23-jun-86 (jeff)
**	    Adapted from PQUAL structure in 4.0
*/
typedef struct _PSY_PQUAL
{
    PST_QNODE	    *p_qual;            /* The qualification */
    i4              p_used;             /* TRUE means this qual was used */
} PSY_PQUAL;

/*
**  Definition of static variables and forward static functions.
*/

					/*
					** try to use the information found in
					** relstat to determine if the user may
					** access a given table
					*/
static bool
psy_passed_prelim_check(
	PSS_SESBLK	    *cb,
	PSS_RNGTAB	    *rngvar,
	i4		    qmode);

					/* Do the protection algorithm */
static DB_STATUS
dopro(
	PSS_RNGTAB	*rngvar,
	PST_QNODE	*root,
	i4		query_mode,
	i4		qmode,
	PSY_ATTMAP	*byset,
	bool		retrtoall,
	PSS_SESBLK	*sess_cb,
	PSS_USRRANGE	*rngtab,
	RDF_CB		*rdf_cb,
	RDF_CB		*rdf_tree_cb,
	PSS_DUPRB	*dup_rb,
	PSS_RNGTAB	*err_rngvar,
	PSY_ATTMAP      *attrmap);

					/* Apply permit qualifications */
static DB_STATUS
applypq(
	PSS_SESBLK  *sess_cb,
	i4	    size,
	PSY_QMAP    qmap[],
	PSY_PQUAL   pqual[PSY_MPQUAL],
	i4	    checkany,
	PST_QNODE   **outtree,
	PSS_DUPRB   *dup_rb);

					/* Form disjunct of all pquals
					** referenced in maps.
					*/
static DB_STATUS
gorform(
	PSS_SESBLK	    *sess_cb,
	i4		   size,
	PQMAP		   map,
	PSY_PQUAL	   pqual[],
	PST_QNODE	   **qual,
	PSS_DUPRB	    *dup_rb);

					/* Select a qmap for elimination from
					** the pqual bit matrix.
					*/
static i4
pickqmap(
	i4	    size,
	PSY_QMAP    qmap[]);

					/* Make domain reference sets */
static DB_STATUS
makedset(
	PSS_RNGTAB	    *var,
	PST_QNODE	    *tree,
	i4		    query_mode,
	i4		    qmode,
	i4		    retrtoall,
	PSS_SESBLK	    *sess_cb,
	PSS_USRRANGE	    *rngtab,
	i4		    *uset,
	i4		    *rset,
	i4		    *aset,
	i4		    *qset,
	RDF_CB		    *rdf_cb,
	RDF_CB		    *rdf_tree_cb,
	PSS_DUPRB	    *dup_rb);

					/* Clear bits according to mask */
static VOID
btmask(
	i4     size,
	char	*mask,
	char	*result);

					/*
					** in the course of parsing a dbproc, 
					** check the existing list of 
					** column-specific privileges to
					** determine if the current user 
					** possesses (or does not possess) a 
					** required column-specific privilege
					*/
static DB_STATUS
psy_check_colprivs(
	PSS_SESBLK	*sess_cb,
	register i4	*required_priv,
	PSQ_COLPRIV	**priv_descriptor,
	PSQ_COLPRIV     **priv_list,
	bool		*missing,
	DB_TAB_ID	*id,
	i4		*attrmap,
	PSF_MSTREAM	*mstream,
	DB_ERROR	*err_blk);

					/*
					** check if there are permits to satisfy
					** the required privileges
					*/
static DB_STATUS
psy_check_permits(
	PSS_SESBLK	*sess_cb,
	PSS_RNGTAB      *rngvar,
	PSY_ATTMAP	*uset,
	PSY_ATTMAP	*rset,
	PSY_ATTMAP	*aset,
	PSY_ATTMAP	*qset,
	i4		*privs,
	i4		query_mode,
	i4		qmode,
	PSS_DUPRB	*dup_rb,
	PSY_QMAP	*qmap,
	PSY_PQUAL	*pqual,
	RDF_CB		*rdf_cb,
	RDF_CB		*rdf_tree_cb,
	PSS_USRRANGE	*rngtab,
	i4		*pndx);

#ifdef    xDEBUG
					/* print attribute map */
static VOID
pr_set(
	i4     *xset,
	char   *labl);

					/* Formatted printout of qmaps */
static VOID
pr_qmap(
	i4	    size,
	PSY_QMAP    qmap[]);

					/* Print bits in vector */
static VOID
btdumpx(
	char               *vec,
	i4		   size,
	char		   *buf);
#endif

static  const i4  namemap[] =	/* Maps permit bits to query modes */
    {
	PSQ_RETRIEVE,	/* DB_RETRIEVE */
	PSQ_REPLACE,	/* DB_REPLACE */
	PSQ_DELETE,	/* DB_DELETE */
	PSQ_APPEND,	/* DB_APPEND */
	PSQ_RETRIEVE,	/* DB_AGGREGATE */
	PSQ_RETRIEVE	/* DB_TEST */
    };

#ifdef    xDEBUG
static  const char opstring[] = "URAQ"; /* Operation indicators for debug */
#endif


/*{
** Name: psy_protect	- Apply the protection algorithm
**
** Description:
**      This function applies the protection algorithm as described in the
**	file-level comments above.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**	tree				Query tree to receive permission
**	qmode				User query mode
**	err_blk				Filled in if an error happens
**	num_joins			ptr to a var containing number of joins
**					found in the query so far
**	viewinfo			array of structures providing additional
**					information about views appearing in the
**					range table
**
** Outputs:
**      tree                            Permit qualifications may be applied
**	err_blk				Filled in if an error happened
**	*num_joins			Will be incremented if we decide to use
**					any permits with joins in their
**					qualification.
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
**	23-jun-86 (jeff)
**          Adapted from protect() in 4.0
**      04-sep-86 (seputis)
**          get result range var from aux table instead of sess_cb
**	03-aug-88 (stec)
**	    Fix "append to somebody's table always successful" bug.
**	04-nov-88 (stec)
**	    For extended catalogs explicit privilege required
**	    (catalog update priv insufficient).
**	22-jun-89 (andre)
**	    If the object belongs to the current user, set delall to TRUE iff
**	    the object is a table.  This will fix a following class of bugs:
**	    DBA grants user U a select permit on table T;
**	    U create view V "as select * from T";
**	    U declares a cursor on V and proceeds to delete all entries.
**	03-aug-89 (andre)
**	    If a user has no permissions to retrieve from an index I, check
**	    permissions on the underlying table T, and if an appropriate
**	    permit has been found, allow user to retrieve from U.  The change
**	    will only effect SQL queries.  Similar (but somewhat more
**	    complicated) solution for QUEL will follow.
**      24-jul-89 (jennifer)
**          Added auditing of event if protections cause failure.
**      24-jul-89 (jennifer)
**          Added new routine doaudit to build an audit range table and 
**          and audit alarm permit list if protections pass.
**          This will be attached to statement header and onto the QP.
**	14-sep-89 (andre)
**	    Accept ptr to a var containign number of joins found in the query so
**	    far.  If we intend to use a permit with qualification which
**	    involves joins (this may only occur when processing a QUEL query and
**	    using a permit created via CREATE PERMIT in SQL), we need to
**	    increment group_id field in all operator nodes which have a valid
**	    group_id by the number of joins found so far.  We will also need to
**	    reset outer_rel masks in the range table entry(ies) for the table(s)
**	    used in the qualification of the permit.
**	    Also, interface to pst_treedup() has changed.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	14-sep-90 (teresa)
**	    pss_catupd becomes bitflag rather than boolean.
**	17-sep-90 (andre)
**	    added a new argument - viewinfo.  this was done as a part of
**	    the fix for bugs 33040, 33041, and 33042.  viewinfo is an array of
**	    pointers to structures providing additional information about views.
**	    They are guaranteed to indicate if a given view was created in SQL
**	    or QUEL.  More importantly, for any non-mergeable view V s.t. we
**	    need to check permissions on some relation R used in its definition,
**	    a map (in reverse) of attributes of V which are used in the main
**	    query will be available.  By examining view tree of V, we can build
**	    a map of attributes of R for which SELECT permission must be
**	    checked.  Note that at present in SQL we do not recognize
**	    permissions on individual columns, so an easy was to speed things up
**	    for SQL is to avoid walking V's tree altogether, and look for
**	    "GRANT-compatible" SELECT permission on R
**	29-nov-90 (andre)
**	    A fix made by me on 8/3/89 had a problem: user trying to RETRIEVE
**	    from an index owned by someone else would invariably get a
**	    "unexpected internal error" message.  This was caused by the fact
**	    that dopro() would be instructed to return error message number
**	    inside the error block rather than to print it, but psy_protect()
**	    would not call dopro() again to check for permissions on the
**	    underlying base table (the latter part was intentional - the use of
**	    permissions on underlying table was intended only for SQL queries).
**	    The solution implemented back then can be substantially simplified:
**		1) IF the current object is an index and
**		      the query language is SQL and
**		      the query is NOT trying to update the current object
**		   THEN
**		      save the range var representing the index (in ind_rngvar)
**		      obtain the description of the underlying table and apply
**		      preliminary checking (i.e. check for ALL/RETRIEVE_TO_ALL,
**		      etc.)
**		      if we still need to invoke dopro, remember to pass
**		      ind_rgvar (for error reporting) and a 0'd out attribute
**		      map to dopro()
**		   ENDIF
**
**	    Note that the above solution relies on the newly introduced
**	    mechanism whereby we can provide dopro() with map of attributes
**	    which must be accessed.  Furthermore, since for SQL we only
**	    recognize GRANT-compatible permits, setting the attribute mask to
**	    all 0's will cause dopro() to reset it to all 1's and check for
**	    SELECT permission on all columns of the underlying base table.
**	    NOTE that this also requires a change in dopro() interface (see the
**	    comment in dopro() prolog)
**	16-dec-90 (ralph)
**	    Trigger security_alarm for failed attempts (b34903).
**	    This involves a minor interface change to doaudit().
**	01-apr-91 (andre)
**	    the above-mentioned fix for bugs 33040-33042 had one little,
**	    but quite unpleasant problem:
**		nmv_map gets populated with offsets into rngtab of non-mergeable
**		views, unfortunately, they can get overwritten with -1 since
**		instead of initializing the entries to -1 before entering the
**		loop, I chose to do it inside the loop; the recipe for disaster:
**		any script which causes a non-mergeable view with range number N
**		be placed in entry M of rngtab where M<N.
**	07-aug-91 (andre)
**	    qmode may be set to PSQ_VIEW to indicate that the function is being
**	    called in the course of determining whether the view being created
**	    is not abandoned
**	25-sep-91 (andre)
**	    When parsing definitions of views and dbprocs we need to build a map
**	    of independent objects (including privileges.)  Using ALL_TO_ALL and
**	    RETR_TO_ALL to avoid a "full-fledged protection algorithm" makes it
**	    impossible for dopro() to build a complete map of independent
**	    privileges.
**	    Accordingly, we will postpone use of relstat bits to satisfy
**	    privileges until dopro() if we are processing a definition of a view
**	    or a dbproc.
**	01-nov-91 (andre)
**	    at least for now there seems to be no reason to call doaudit() for
**	    database procedures: security alarms are not looked up when
**	    processing dbprocs and table information which does get built is
**	    disregarded later on.
**	4-may-1992 (bryanp)
**	    Added support for PSQ_DGTT and PSQ_DGTT_AS_SELECT
**	15-jun-92 (barbara)
**	    Change interface to pst_rgent().
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	03-apr-93 (markg)
**	    Removed calls to doaudit() as this functionality is now being
**	    performed in psyview.c. Added calls to the do_alarm() routine
**	    for failing queries in C2 servers.
**	04-apr-93 (andre)
**	    psy_protect() may now get called in the course of processing cursor
**	    UPDATE/REPLACE if a cursor was declared using QUEL and at DECLARE
**	    CURSOR time we could not ascertain that the user possesses
**	    unconditional RETRIEVE privilege on every column of the table/view
**	    on which the cursor is defined
**	20-may-93 (andre)
**	    fix for bug 51965:
**		before calling dopro(), we perform special processing if the
**		relation was referenced in a non-mergeable view or if a
**		relation turns out to be an index.  However, the order in which
**		we performed our checks was incorrect.  Instead of checking if
**		an index is being used in SQL query with effective query mode of
**		RETRIEVE only if it was not used in a non-mergeable view, we
**		need to check whether it was an index first.  This will ensure
**		that if a non-mergeable view was based on an index, we will
**		check permissions on the index' base table instead of trying to
**		look for permissions on the index which is very unlikely to be
**		there.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	5-jul-93 (robf)
**	    changed interface of psy_secaudit() to accept security label
**	10-nov-93 (anitap)
**	    If pss_ses_flag = PSS_INGRES_PRIV, then do not check the privileges.
**	    $ingres in verifydb should be able to "alter table drop constraint"
**	    on another user's table.
**	17-nov-93 (rblumer)
**	    Fix regression in WGO test suite due to misplaced parentheses
**	    around check for PSS_INGRES_PRIV bit.
**	22-nov-93 (robf)
**          Make do_alarm public, now called psy_do_alarm_fail()
**	17-mar-94 (robf)
**          Use distinct flag for select_all rather than piggy-backing
**	    from the drop_all
**	24-jan-1997 (kch)
**	    We now check for viewinfo[rngvar->pss_rgno] == NULL during the
**	    scan of the range table. In psy_view(), the view info structure
**	    will not be allocated if
**	    - this view (or a view defined on top of it) is a target of
**	      UPDATE, INSERT, or DELETE, and
**	    - there are rules defined on it, and
**	    - the rules have not been checked to see if any of them apply.
**	    This change fixes bug 80279.
**	3-dec-2008 (dougi) BUG 121319
**	    Don't do the check when "table" is actually a table procedure.
**      18-Mar-2009 (hanal04) Bug 121807
**          No need to check permissions for derrived table range entries.
**          The underlying tables and views will be checked separately and
**          the simulated RDF structures generated by pst_sdent() will
**          cause psy_checkPermits() to SEGV.
**      15-Oct-2010 (hanal04) Bug 124600
**          Security auditing was failing to log illegal access to a
**          view because we were telling the security autiting system that
**          the event was a table access. Set the eventtype to SXF_E_VIEW
**          if the rngvar is a view.
*/
DB_STATUS
psy_protect(
	PSS_SESBLK	    *sess_cb,
	PSS_USRRANGE	    *rngtab,
	PST_QNODE	    *tree,
	i4		    qmode,
	DB_ERROR	    *err_blk,
	PSF_MSTREAM	    *mstream,
	i4	    	    *num_joins,
	PSY_VIEWINFO	    *viewinfo[])
{
    register PST_QNODE	*r;
    register i4	vn;
    i4			vmode;	/* Query mode of range variable in question */
    PST_VRMAP		varset;
    i4			nmv_map[PST_NUMVARS];	/* map of non-mergeable views */

				/*
				** attribute map; used for relations which were
				** used in definition of non-mergeable views;
				** also used when we check permissions on the
				** underlying base table instead of checking
				** permissions on an index (we don't grant
				** permits on indices)
				*/
    PSY_ATTMAP		attrmap;

				    /*
				    ** this will be set to a non-NULL value if
				    ** we plan to check permissions on the
				    ** underlying base table instead of checking
				    ** permissions on an index
				    */
    PSS_RNGTAB		*index_rngvar = (PSS_RNGTAB *) NULL;

    register PSS_RNGTAB	*rngvar;
    
    DB_STATUS		status = E_DB_OK, stat;
    RDF_CB		rdf_cb;		/* For permit tuples */
    RDF_CB		rdf_tree_cb;	/* For query trees */
    PSS_DUPRB		dup_rb;		/* It will be partially initialized here
					** and passed to dopro().  I prefer this
					** appraoch since dopro() may call
					** itself recursively, and I see no
					** reason to waste stack space
					*/
    RDR_RB		*rdf_rb;
    RDR_RB		*rdf_t_rb;
    i4		mask;
    PSC_CURBLK		*cursor = (PSC_CURBLK *) NULL;
#ifdef    xDEBUG
    i4		val1, val2;
#endif

    /*
    ** VERIFYDB magic, if special flag set then we let the DBA access
    ** any table directly without having to do -u. 
    */
    if(sess_cb->pss_ses_flag & PSS_SELECT_ALL)
	return E_DB_OK;

    /*
    ** Don't do anything on an attempt to apply a permit where it doesn't
    ** belong.
    */
    switch (qmode)
    {
	case PSQ_REPCURS:
	case PSQ_DEFCURS:
	case PSQ_REPDYN:
	    cursor = sess_cb->pss_crsr;
	    break;
	    
	case PSQ_DELETE:
	case PSQ_RETRIEVE:
	case PSQ_APPEND:
	case PSQ_REPLACE:
			    /*
			    ** check the tables from which data is being
			    ** retrieved
			    */
	case PSQ_RETINTO:   
	case PSQ_DGTT_AS_SELECT:
			    /*
			    ** check the tables over which the view is being
			    ** defined
			    */
	case PSQ_VIEW:
	{
	    break;
	}
	default:
	{
	    return (E_DB_OK);
	}
    }

    /* initialize some fields in dup_rb */
    {
	/* pss_op_mask may be changed if pss_num_joins != PST_NOJOIN */
	dup_rb.pss_op_mask   = PSS_3DUP_RENUMBER_BYLIST;
	
	dup_rb.pss_tree_info = (i4 *) NULL;
	dup_rb.pss_mstream   = mstream;
	dup_rb.pss_err_blk   = err_blk;
	dup_rb.pss_1ptr      = NULL;

	/*
	** .pss_num_joins may change later on, and when we are done with permit
	** processing we will copy pss_num_joins into *num_joins
	*/
	dup_rb.pss_num_joins = *num_joins;  
    }
    
    rdf_rb = &rdf_cb.rdf_rb;
    rdf_t_rb = &rdf_tree_cb.rdf_rb;

    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(rdf_cb, rdf_tree_cb);
    
    rdf_rb->rdr_types_mask = RDR_PROTECT;
    rdf_t_rb->rdr_types_mask = RDR_PROTECT | RDR_QTREE;

    r = tree;

#ifdef xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	TRdisplay("\nApplying Protections\n\n");
#endif

    MEfill(sizeof(varset), (u_char) 0, (PTR) &varset);

    /* initialize entries in nmv_map to -1 */
    for (vn = 0; vn < PST_NUMVARS; vn++)
    {
	nmv_map[vn] = -1;
    }

    /*
    **  Scan the range table and create a set of all variables
    **  which are 'interesting', that is, on which the protection
    **  algorithm should be performed.
    */

    for (vn = 0; vn <= PST_NUMVARS; vn++)
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
	/* else terminate the `for' loop */
	else
	{
	    break;
	}

	if (
		/* Is range variable being used? */
	    !rngvar->pss_used || rngvar->pss_rgno == -1
	    ||
		/* only check those range vars needing checking */
	    !rngvar->pss_permit || rngvar->pss_rgtype == PST_TPROC
            ||
                /* range variable is a derrived table */
            rngvar->pss_rgtype == PST_DRTREE
	   )
	{
	    continue;
	}

	/* Is it a table? */
	if (rngvar->pss_rgtype == PST_RTREE)
	{
	    /*
	    ** note that we will definitely not need the information in nmv_map
	    ** if !rngvar->pss_permit
	    */
	    nmv_map[rngvar->pss_rgno] = vn;

	    continue;
	}

	mask = rngvar->pss_tabdesc->tbl_status_mask;

	/* if owner, accept any query */
	if (!MEcmp((PTR) &rngvar->pss_tabdesc->tbl_owner,
	    (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME)))
	{
	    /*
	    ** if define cursor, and the object is not a view (is a table), and
	    ** this IS the object for which a cursor is being defined,
	    ** mark delete permission
	    */
	    if ((qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN) &&
	        ~mask & DMT_VIEW     &&
		rngvar == sess_cb->pss_resrng)
	    {
		cursor->psc_delall = TRUE;

		/*
		** if defining a QUEL cursor, remember that the user may
		** retrieve every column of the table/view over which the cursor
		** is being defined
		*/
		if (sess_cb->pss_lang == DB_QUEL)
		    cursor->psc_flags |= PSC_MAY_RETR_ALL_COLUMNS;
	    }

	    continue;
	}

	/* After this point we are looking at tables/views that
	** are not owned by the current user.
	*/

	/* If the view created in SQL, check permits now, else wait until
	** after variable substitution in the view processor.
	*/
	if (mask & DMT_VIEW)
	{
	    /*
	    ** we no longer need to call psy_sqlview() here to determine if this
	    ** is an SQL view; caller must furnish this information inside
	    ** viewinfo array;
	    */

	    /* Bug 80279 */
	    if (viewinfo[rngvar->pss_rgno] != NULL)
	    {
		if (~viewinfo[rngvar->pss_rgno]->psy_flags & PSY_SQL_VIEW)
		{
		    continue;
		}
	    }
	}

	/* Mark the range entry as having been examined. */
	rngvar->pss_permit = FALSE;

	/*
	** check if we can avoid invoking a full-blown protection algorithm by
	** using information available in the relstat bit (i.e. is it the case
	** that it is a catalog and the user has catalog update privilege?,
	** was there a ALL/RETRIEVE-TO-ALL granted on it?, etc.)
	**
	** If parsing a definition of a view or a dbproc, postpone calling
	** psy_passed_prelim_check() until after dopro() has built a map of
	** independent privileges
        */
	if (   (   qmode != PSQ_VIEW
		&& ~sess_cb->pss_dbp_flags & PSS_DBPROC
		&& psy_passed_prelim_check(sess_cb, rngvar, qmode))
	    || (sess_cb->pss_ses_flag & PSS_INGRES_PRIV))
	{
	    continue;
	}
	else
	{
	    BTset(vn, (char *) &varset);
	}
    }

    /* For each variable specified in varset do the real algorithm.
    ** The check should be for PST_NUMVARS+1 bits, because the range
    ** variable table (PST_NUMVARS entries) is followed by an entry for
    ** PSQ_APPEND case.
    */
    for (vn = -1;
         (vn = BTnext(vn, (char *) &varset, (i4) (PST_NUMVARS+1))) >= 0;
	)
    {
	/*
	** if during the previous iteration we were checking permissions on the
	** base table to determine if the user may retrieve from an index, mark
	** the base table range variable as unused and place it at the head of
	** the queue
	*/
	if (index_rngvar != (PSS_RNGTAB *) NULL)
	{
	    rngvar->pss_used = FALSE;
	    (VOID) QUremove((QUEUE *) rngvar);
	    (VOID) QUinsert((QUEUE *) rngvar, rngtab->pss_qhead.q_prev);
	    index_rngvar = (PSS_RNGTAB *) NULL;
	}

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
	/* else terminate the `for' loop */
	else
	{
	    break;
	}

#ifdef	    xDEBUG
	if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	    TRdisplay("\nvarno = %d, relname = %#s\n", rngvar->pss_rgno,
		sizeof (rngvar->pss_tabname.db_tab_name),
		rngvar->pss_tabname.db_tab_name);
#endif

	mask = rngvar->pss_tabdesc->tbl_status_mask;

	/*
	**  Determine the query mode for this variable.  This
	**  is not the query mode of the original query,
	**  unless the variable is the result variable.
	**  We handle the possible replace and delete cursor later.
	*/

	if (   (sess_cb->pss_resrng &&
	        rngvar->pss_rgno != sess_cb->pss_resrng->pss_rgno)
	    || qmode == PSQ_RETINTO || qmode == PSQ_DEFCURS || qmode == PSQ_VIEW
	    || qmode == PSQ_DGTT_AS_SELECT || qmode == PSQ_REPDYN
	    || (rngvar->pss_rgparent > -1 && sess_cb->pss_lang == DB_SQL)
	   )
	{
	    vmode = PSQ_RETRIEVE;
	}
	else
	{
	    vmode = qmode;
	}

	/*
	** if the current variable is an index, there will be no permits defined
	** on it (since we disallow creating permits on indices).  We will check
	** permissions on the underlying table to determine if the user can
	** access the index if
	**	- query language is SQL  and
	**	- index will be used for retrieval only
	**	  
	*/
	if (   sess_cb->pss_lang == DB_SQL
	    && mask & DMT_IDX
	    && vmode == PSQ_RETRIEVE)
	{
	    DB_TAB_ID	    basetbl_id;
	    PSS_RNGTAB	    *base_tbl;

	    /* convert index id to base table id */
	    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, basetbl_id);
	    basetbl_id.db_tab_index = 0;

	    /* remember the range variable representing index */
	    index_rngvar = rngvar;

	    /* obtain description of the underlying base table */
	    status = pst_rgent(sess_cb, rngtab, -1, "!", PST_SHWID,
		(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, &basetbl_id,
		TRUE, &base_tbl, qmode, err_blk);
	    
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }

	    rngvar = base_tbl;

	    /* reset the mask to that of the base table */
	    mask = rngvar->pss_tabdesc->tbl_status_mask;

	    /*
	    ** If parsing a definition of a view or a dbproc, postpone calling
	    ** psy_passed_prelim_check() until after dopro() has built a map of
	    ** independent privileges
	    */
	    if (   qmode != PSQ_VIEW
		&& ~sess_cb->pss_dbp_flags & PSS_DBPROC
		&& psy_passed_prelim_check(sess_cb, base_tbl, qmode))
	    {
		/*
		** user may retrieve from the base table; we are done with this
		** entry; base table range entry will be freed at the top of the
		** loop
		*/
		continue;
	    }

	    /* need to have GRANT-compatible SELECT permission */
	    psy_fill_attmap(attrmap.map, ~((i4) 0));
	}
	else if (rngvar->pss_rgparent != -1)
	{
	    PSS_RNGTAB		*parent = rngtab->pss_rngtab +
					  nmv_map[rngvar->pss_rgparent];

	    psy_fill_attmap(attrmap.map, (i4) 0);

	    /*
	    ** before we do the "interesting" part of the algorithm, we need to
	    ** get a map of attributes of of the current relation which are used
	    ** in the query.  Unfortunately, the algorithm in dopro() will not
	    ** handle this situation appropriately since (quite intentionally)
	    ** THIS RELATION HAS NOT BEEN MERGED INTO THE MAIN QUERY.  Instead,
	    ** we will use the map of attributes of this relations's "parent",
	    ** translate them to attributes of the current relation, and pass
	    ** them to dopro().  dopro() will skip over the code used to scan
	    ** the main query tree but will trust our map instead.
	    */

	    status = psy_translate_nmv_map(parent,
		(char *) viewinfo[parent->pss_rgno]->psy_attr_map,
		rngvar->pss_rgno, (char *) &attrmap, psy_att_map, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }
	}

	/* do the interesting part of the algorithm */
	status = dopro(rngvar, r, qmode, vmode, (PSY_ATTMAP *) NULL,
	    !(mask & DMT_RETRIEVE_PRO),
	    sess_cb, rngtab, &rdf_cb, &rdf_tree_cb, &dup_rb,
	    index_rngvar,
	    ((rngvar->pss_rgparent != -1) || index_rngvar)
		? &attrmap : (PSY_ATTMAP *) NULL);

	if (DB_FAILURE_MACRO(status))
	{
	    goto exit;
	}

	/*
	** note that at this point if index_rngvar!=NULL, rngvar is pointing at
	** a range entry for the base table on which index was defined.
	** Fortunately, we do not need to be concerned about it, since we are
	** almost at the bottom of the loop, and the conditions required for
	** setting index_rngvar guarantee that it if it was set, the next test
	** is bound to fail.
	*/

	/*
	** If this is a "define cursor" for update, do the same thing, treating
	** the updateable columns like the replace set.
	*/
	if ((qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN) &&
	    cursor->psc_forupd &&
	    rngvar == sess_cb->pss_resrng)
        {
#ifdef    xDEBUG
	    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	    {
		TRdisplay("\nNow doing update portion of define cursor\n");
		TRdisplay("Table name is %#s\n",
		    sizeof (rngvar->pss_tabname.db_tab_name),
		    rngvar->pss_tabname.db_tab_name);
	    }
#endif

	    /* Do the interesting part of the algorithm */
	    status = dopro(rngvar, r, PSQ_DEFCURS, PSQ_DEFCURS,
		(PSY_ATTMAP *) NULL, !(mask & DMT_RETRIEVE_PRO),
		sess_cb, rngtab, &rdf_cb, &rdf_tree_cb, &dup_rb,
		(PSS_RNGTAB *) NULL, (PSY_ATTMAP *) NULL);
					    /*
					    ** index_rngvar is set to NULL if we
					    ** got here;
					    ** attrmap only needs to be passed
					    ** when the relation was used in the
					    ** definition of a non-mergeable
					    ** view
					    */

	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }
	}
    }

    /*
    ** if during the last iteration we were checking permissions on the base
    ** table to determine if the user may retrieve from an index, mark the base
    ** table range variable as unused and place it at the head of the queue
    */
    if (index_rngvar != (PSS_RNGTAB *) NULL)
    {
	rngvar->pss_used = FALSE;
	(VOID) QUremove((QUEUE *) rngvar);
	(VOID) QUinsert((QUEUE *) rngvar, rngtab->pss_qhead.q_prev);
	index_rngvar = (PSS_RNGTAB *) NULL;
    }

exit:
    /* If the rdf_cb has been used for permit tuples, free them */
    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	rdf_rb->rdr_update_op = RDR_CLOSE;
	stat = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(stat))
	{
	    if (stat > status)
	    {
		DB_ERROR err_blk;

		(VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, &err_blk);
		status = stat;	
	    }
	}
    }

    /* return the (authorized) tree */
#ifdef    xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
    {
	TRdisplay("Protection algorithm finished\n");
    }

    if (ult_check_macro(&sess_cb->pss_trace, 51, &val1, &val2))
    {
	TRdisplay("Query tree after protection algorithm:\n");
	pst_prmdump(r, (PST_QTREE *) NULL, err_blk, DB_PSF_ID);
    }
#endif

    /* 
    ** If an error occurred, write a security audit 
    ** of the failure.
    */
    if (DB_FAILURE_MACRO(status) && err_blk->err_code == E_PS0001_USER_ERROR)
    {
	DB_ERROR	    e_error;
	DB_STATUS	    local_status;
	i4		    access = 0;
	i4                  eventtype;

	/* Get rest of access from query mode. */
	if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	{
	    switch (qmode)
	    {
		case PSQ_RETRIEVE:
		case PSQ_DEFCURS:
		case PSQ_REPDYN:
		case PSQ_RETINTO:
		case PSQ_DGTT_AS_SELECT:
		case PSQ_VIEW:
		    access = SXF_A_FAIL | SXF_A_SELECT;
		    break;
		case PSQ_APPEND:
		    access = SXF_A_FAIL | SXF_A_INSERT;
		    break;
		case PSQ_DELETE:
		    access = SXF_A_FAIL | SXF_A_DELETE;
		    break;
		case PSQ_REPLACE:
		    access = SXF_A_FAIL | SXF_A_UPDATE;
		    break;
	    }

            if(rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
            {
                eventtype = SXF_E_VIEW;
            }
            else
            {
                /* Default is table access */
                eventtype = SXF_E_TABLE;
            }
	    local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)&rngvar->pss_tabname,
			    &rngvar->pss_ownname,
			    sizeof(DB_TAB_NAME), eventtype,
			    I_SX2020_PROT_REJECT, access, 
			    &e_error);

	    if (local_status > status)
		status = local_status;
	}

	/* process security_alarms */
	local_status = psy_do_alarm_fail(sess_cb, rngvar, qmode, &e_error);
	if (local_status > status)
	    status = local_status;
    }

    /* Update number of joins in the tree */
    *num_joins = dup_rb.pss_num_joins;
    
    return (status);
}

/*{
** Name: psy_dbpperm	- Check if the user has permission to execute dbproc
**
** Description:
**      This function fetches all protection tuples applicable to a given
**	dbproc until there are no more left (failure) or one is found
**	that indicates that the user has permission to execute (success).
**	IMPORTANT: this function expects to be called when processing a
**		   dbproc invocation (as a part of CREATE RULE,
**		   EXECUTE PROCEDURE or recreation of a dbproc) or when
**		   processing GRANT ON PROCEDURE (if the user does not own the
**		   dbproc we want to make sure that he has EXECUTE WGO on the
**		   dbproc).
**	
** Inputs:
**	sess_cb			    PSF session CB
**	rdf_cb			    ptr to RDF CB which was used to retrieve
**				    IIPROCEDURE tuple describing the dbproc
**	    rdf_info_blk->rdr_dbp   IIPROCEDURE tuple
**      psq_cb			    ptr to psq_cb
**	    psq_mode		    mode of the statement (must be one of
**				    PSQ_EXEDBP, PSQ_CALLPROC, PSQ_RULE, or
**				    PSQ_GRANT)
**	dbp_id			    dbproc id
**	dbp_owner		    dbproc owner
**	dbp_name		    dbproc name
**	required_privs		    addr of a map of privileges required by the
**				    statement being processed
**
** Outputs:
**	psq_cb->
**		psq_error	    Filled in if an error happened
**	required_privs		    0 if all required privileges have been
**				    satisfied; map of privileges which the user
**				    lacks otherwise; should be consulted only if
**				    !DB_FAILURE_MACRO(status)
**
** Returns:
**	E_DB_OK			    Success - need to check *required_privs to
**				    determine if the user indeed poseses
**				    sufficient privileges
**	E_DB_ERROR		    Failure
**
** Exceptions:
**	none
**
** Side Effects:
**	May do catalog I/O
**
** History:
**	04-aug-88 (andre)
**          Created.
**	28-sep-88 (stec)
**	    Correct a typo (&rdf_cb => rdf_cb).
**	    Check 1st char of user name in the protection
**	    tuple for space (which indicates PUBLIC). 
**	30-nov-88 (stec)
**	    Improve error recovery in for RDR_CLOSE call
**	    (use err_blk passed to procedure).
**	02-dec-88 (stec)
**	    Change psy_dbpperm to process rdf_call status differently.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Check for permits to group/application identifiers.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Use DBGR_USER, DBGR_GROUP, and DBGR_APLID constants for dbp_gtype.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**      08-aug-90 (ralph)
**          Don't interpret security alarms as permits.
**	27-sep-90 (ralph)
**	    Audit dbproc execution in psy_dbpperm.
**	    Change interface to psy_dbpperm to include psq_cb and pss_sesblk.
**	20-feb-91 (andre)
**	    Changed interface for psy_dbpperm(): add a pointer to session CB
**	    and remove pointers to user, group, and role identifiers since they
**	    can all be obtained from the session CB.
**	21-feb-91 (andre)
**	    call newly defined psy_grantee_ok() to determine if, based on
**	    grantee name and type, the permit applies to this session;
**	20-sep-91 (andre)
**	    psy_dbpperm() can be called to determine if a given user can grant a
**	    permit on a dbproc; note that in case of GRANT only failure to
**	    create a permit due to insufficient privileges will be audited
**	23-sep-91 (andre)   (SEE 01-oct-91 (andre))
**	    when processing GRANT ON PROCEDURE use PSS_INSUF_DBP_PRIVS (over
**	    sess_cb->pss_stmt_flags) to indicate that the user may not grant the
**	    desired privilege on the dbproc.  Lack of required privileges in
**	    this case will not result in status being set to E_DB_ERROR, thus
**	    allowing user to distinguish between lack of privileges and other
**	    errors.
**	26-sep-91 (andre)
**	    if we are checking if the current user can invoke a dbproc P' not
**	    owned by him in the course of parsing dbproc P, check the list of
**	    independent privileges for occurrence of P' and if none was found,
**	    add it to the list
**	30-sep-91 (andre)
**	    elements of the list of independent objects will be allocated from
**	    the stream pointed to by sess_cb->pss_dependencies_stream.
**	01-oct-91 (andre)
**	    added insuf_privs to psy_dbpperm()'s interface; as a result the
**	    (23-sep-91) is no longer valid - regardless of the query mode,
**	    if the user lacks some of the required privileges, *insuf_privs will
**	    be set to TRUE, but status will not be set to E_DB_ERROR.
**	03-oct-91 (andre)
**	    if we are reparsing a dbproc definition to determine if the dbproc
**	    is grantable, we MUST find EXECUTE WGO.  While in other cases, we
**	    are satisfied with resetting PSS_DBPGRANT_OK bit in pss_dbp_flags,
**	    in this case, we will report lack of EXECUTE WGO as an error.
**	04-oct-91 (andre)
**	    When searching for an independent object privilege descriptor
**	    pertaining to this dbproc, if one is found in the sublist associated
**	    with the dbproc being parsed, we are done.  If one is found outside
**	    the current sublist, copy it into the current sublist and we are
**	    done.  Only if such descriptor could not be found, we will create a
**	    new descriptor, insert it into the sublist associated with the
**	    dbproc being parsed and then proceed to determine if it is
**	    grantable.
**	    NOTE: an independent privilege entry may indicate that a dbproc is
**	    ungrantable which would make the dbproc being parsed ungrantable as
**	    well.
**	12-feb-92 (andre)
**	    Caller will no longer pass addr of a flag which will
**	    be used to indicate that the user lacked some required privilege(s),
**	    instead, caller will pass addr of a map of a privilege required for
**	    the statement, and psy_dbpperm() will reset it to 0 if the user
**	    posesses all required privileges.  In some cases, when, in fact,
**	    the privilege must be grantable, psy_dbpperm() may choose to set
**	    DB_GRANT_OPTION bit in *required_privs
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	13-apr-93 (andre)
**	    do not add privileges on dbprocs to the independent privilege list
**	    if parsing a system-generated dbproc
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	25-oct-93 (robf)
**          Audit security label for procedure 
**      02-nov-93 (andre)
**          if the user attempting to grant a privilege on a dbproc X possesses
**          no privileges whatsoever on X, rather than issuing a message saying
**          that he cannot grant some privilege(s) on X, we will issue a message
**          saying that EITHER X does not exist OR the user may not access it.
**	20-dec-93 (stephenb)
**	    correct call to psy_secaudit(), we were oring two access types
**	    and a condition, it should only be a condition and one access type.
**	09-Sep-2008 (jonj)
**	    2nd param to psf_error() is i4, not DB_ERROR.
*/
DB_STATUS
psy_dbpperm(
	PSS_SESBLK	*sess_cb,
	RDF_CB		*rdf_cb,
	PSQ_CB		*psq_cb,
	i4		*required_privs)
{
    QEF_DATA	    *qp;
    DB_PROTECTION   *protup;
    RDR_RB	    *rdf_rb = &rdf_cb->rdf_rb;
    DB_TAB_ID	    *dbp_id = &rdf_cb->rdf_info_blk->rdr_dbp->db_procid;
    DB_OWN_NAME	    *dbp_owner = &rdf_cb->rdf_info_blk->rdr_dbp->db_owner;
    DB_DBP_NAME	    *dbp_name = &rdf_cb->rdf_info_blk->rdr_dbp->db_dbpname;
    DB_STATUS	    status = E_DB_OK, stat;
    i4	    err_code;
    i4		    tupcount;

				/* 
				** will be used to remember whether the 
				** <auth id> attempting to grant privilege(s)
				** him/herself possesses any privilege on the 
				** object; 
				** will start off pessimistic, but may be reset 
				** to TRUE if we find at least one permit 
				** granting a privilege to <auth id> 
				** attempting to grant the privilege or to 
				** PUBLIC
				*/
    bool	    cant_access = TRUE;

    bool	    must_audit;
    i4		    save_privs, privs_wgo = (i4) 0;
    PSQ_OBJPRIV	    *dbppriv_element = (PSQ_OBJPRIV *) NULL;
    DB_ERROR	    *err_blk = &psq_cb->psq_error;

    /* we were called but the are no privileges to verify */
    if (!*required_privs)
	return(E_DB_OK);

    /*
    ** determine if we may need to audit the operation; if an unexpected error
    ** occurs, we will definitely not audit
    ** for GRANT, we will only audit the failure due to insufficient privileges
    ** do not audit for CREATE RULE
    */
    must_audit = !(psq_cb->psq_flag & PSQ_ALIAS_SET ||
		   psq_cb->psq_mode == PSQ_RULE);

    /*
    ** If this is a dbproc fired by a rule and we are trying to recreate it, we
    ** really need to check if the creator of the rule (which may be different
    ** from the current user) can execute it; until we have the necessary
    ** interface to pass to PSF the identity of the rule owner, we shall skip
    ** checking permissions on this dbproc, but we will definitely check if the
    ** dbproc owner can execute the various dbprocs used in its definition
    */

    if ((psq_cb->psq_flag & PSQ_ALIAS_SET && psq_cb->psq_mode == PSQ_EXEDBP) ||
        !MEcmp((PTR) dbp_owner, (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME)) 
       )
    {
	*required_privs = 0;	    /* Session has implicit permission */
	goto audit_outcome;
    }

    /*
    ** we set rdr_rec_access_id here since under some circumstances we may fall
    ** through to saw_the_perms and/or dbpperm_exit and code under dbpperm_exit
    ** relies on rdr_rec_access_id being set to NULL as indication that the
    ** tuple stream has not been opened
    */
    rdf_rb->rdr_rec_access_id = NULL;

    /*
    ** if processing CREATE PROCEDURE, before scanning IIPROTECT to check if the
    ** user has the required privileges on the dbproc, check if we already had
    ** to look it up
    */
    if (sess_cb->pss_dbp_flags & PSS_DBPROC && psq_cb->psq_mode == PSQ_CALLPROC)
    {
	bool                    missing;

	status = psy_check_objprivs(sess_cb, required_privs, &dbppriv_element,
	    &sess_cb->pss_indep_objs.psq_objprivs, &missing, dbp_id,
	    sess_cb->pss_dependencies_stream, PSQ_OBJTYPE_IS_DBPROC, err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    /* unexpected error - do not audit */
	    must_audit = FALSE;
	    goto dbpperm_exit;
	}
	else if (missing)
	{
	    if (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	    {
		/*
		** if we were parsing a dbproc to determine its grantability and
		** determined that the user lacks a required privilege by
		** scanning IIPROTECT, code under saw_the_perms would see
		** *required_privs with DB_GRANT_OPTION bit set; we are doing it
		** here to ensure that saw_the_perms: code is not presented with
		** different value depending on how we arrive at deciding that
		** the user lacks required privilege
		*/
		save_privs = *required_privs;
		*required_privs |= DB_GRANT_OPTION;
	    }

	    goto saw_the_perms;
	}
	else if (!*required_privs)
	{
	    /*
	    ** we were able to determine that the user posesses required
	    ** privilege(s) based on information contained in independent
	    ** privilege lists
	    */
	    goto audit_outcome;
	}
    }

    /*
    ** for invocation in the dbproc being parsed to determine its
    ** grantability, we need grantable privilege(s), so we set DB_GRANT_OPTION
    ** in *required_privs; otherwise we leave *required_privs alone
    */
    if (sess_cb->pss_dbp_flags & PSS_DBPROC && psq_cb->psq_mode == PSQ_CALLPROC)
    {
	save_privs = *required_privs;

	if (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	{
	    *required_privs |= DB_GRANT_OPTION;
	}
	else if (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
	{
	    /*
	    ** if we are processing a definition of a dbproc which appears to be
	    ** grantable so far, we will check for existence of required
	    ** privilege WGO to ensure that it is, indeed, grantable.
	    */
	    privs_wgo = *required_privs;  
	}
    }

    /* get permit tuples from RDF that apply to this dbproc */
    STRUCT_ASSIGN_MACRO((*dbp_id), rdf_rb->rdr_tabid);
			 
    rdf_rb->rdr_types_mask     = RDR_PROTECT | RDR_PROCEDURE;

    rdf_rb->rdr_2types_mask    = (RDF_TYPES) 0;
    rdf_rb->rdr_instr          = RDF_NO_INSTR;

    rdf_rb->rdr_update_op      = RDR_OPEN;
    rdf_rb->rdr_qrymod_id      = 0;	/* get all tuples */
    rdf_rb->rdr_qtuple_count   = 20;	/* get 20 at a time */
    rdf_cb->rdf_error.err_code = 0;

    /*
    ** search IIPROTECT until some error occurs or until we find the required
    ** privilege and, possibly, the required privilege WGO
    */

    /* For each group of 20 permits */
    while (rdf_cb->rdf_error.err_code == 0 && (*required_privs || privs_wgo))
    {
	status = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
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
		    _VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error,
			err_blk);
			
		    must_audit = FALSE;	  /* unexpected error - do not audit */

		    /* close the tuple stream and return */
		    goto dbpperm_exit;
	    }	    /* switch */
	}	/* if status != E_DB_OK */

	/* For each permit tuple */
	for 
	(
	    qp = rdf_cb->rdf_info_blk->rdr_ptuples->qrym_data,
	    tupcount = 0;
	    tupcount < rdf_cb->rdf_info_blk->rdr_ptuples->qrym_cnt;
	    qp = qp->dt_next,
	    tupcount++
	)
	{
	    bool	    applies;
	    i4		    privs_found;

	    /* Set protup pointing to current permit tuple */
	    protup = (DB_PROTECTION*) qp->dt_data;

	    /*
	    ** check if the tuple applies to this session
	    */
	    status = psy_grantee_ok(sess_cb, protup, &applies, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		must_audit = FALSE;	/* unexpected error - do not audit */

		/* invalid grantee type - close tuple stream and return */
		goto dbpperm_exit;
	    }
	    else if (!applies)
	    {
		continue;
	    }

	    /*
	    ** remember that the <auth id> attempting to grant a privilege
	    ** possesses some privilege on the dbproc
	    */
	    if (psq_cb->psq_mode == PSQ_GRANT && cant_access)
		cant_access = FALSE;

	    /*
	    ** if the required privileges has not been satisfied yet, we will
	    ** check for privileges in *required_privs; otherwise we will check
	    ** for privileges in (privs_wgo|DB_GRANT_OPTION)
	    */
	    privs_found = prochk((*required_privs) ? *required_privs
					 : (privs_wgo | DB_GRANT_OPTION),
		(i4 *) NULL, protup, sess_cb);

	    if (!privs_found)
		continue;

	    /* mark the operations as having been handled */
	    *required_privs &= ~privs_found;

	    /*
	    ** if the newly found privileges are WGO and we are looking for
	    ** all/some of them WGO, update the map of privileges WGO being
	    ** sought
	    */
	    if (protup->dbp_popset & DB_GRANT_OPTION && privs_found & privs_wgo)
	    {
		privs_wgo &= ~privs_found;
	    }

	    /*
	    ** check if all required privileges have been satisfied;
	    ** note that DB_GRANT_OPTION bit does not get turned off when we
	    ** find a tuple matching the required privileges
	    */
	    if (*required_privs && !(*required_privs & ~DB_GRANT_OPTION))
	    {
		*required_privs = (i4) 0;
	    }

	    if (!*required_privs && !privs_wgo)
	    {
		/*
		** all the required privileges have been satisfied and the
		** required privileges WGO (if any) have been satisfied as
		** well -  we don't need to examine any more tuples
		*/
		break;
	    }
	}	/* for */
    }	    /* while */

saw_the_perms:

    if (   sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK
	&& (privs_wgo || *required_privs & DB_GRANT_OPTION))
    {
	/*
	** we are processing a dbproc definition; until now the dbproc appeared
	** to be grantable, but the user lacks the required privileges WGO on
	** this dbproc - mark the dbproc as non-grantable
	*/
	sess_cb->pss_dbp_flags &= ~PSS_DBPGRANT_OK;
    }

    /*
    ** if processing a dbproc, we will record the privileges which the current
    ** user posesses if
    **	    - user posesses all the required privileges or
    **	    - we were parsing the dbproc to determine if it is grantable (in
    **	      which case the current dbproc will be marked as ungrantable, but
    **	      the privilege information may come in handy when processing the
    **	      next dbproc mentioned in the same GRANT statement.)
    **
    ** NOTE: we do not build independent privilege list for system-generated
    **	     dbprocs
    */
    if (   sess_cb->pss_dbp_flags & PSS_DBPROC
	&& ~sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED
	&& (!*required_privs || sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK))
    {
	if (save_privs & ~*required_privs)
	{
	    if (dbppriv_element)
	    {
		dbppriv_element->psq_privmap |= (save_privs & ~*required_privs);
	    }
	    else
	    {
		/* create a new descriptor */

		status = psy_insert_objpriv(sess_cb, dbp_id, PSQ_OBJTYPE_IS_DBPROC, 
		    save_privs & ~*required_privs, 
		    sess_cb->pss_dependencies_stream,
		    &sess_cb->pss_indep_objs.psq_objprivs, err_blk);

		if (DB_FAILURE_MACRO(status))
		{
		    must_audit = FALSE;	/* unexpected error - do not audit */

		    /* invalid grantee type - close tuple stream and return */
		    goto dbpperm_exit;
		}
	    }
	}
    }

    if (*required_privs)
    {
	/* required privilege(s) do not exist */

	/*
	** note that we are not setting status to E_DB_ERROR, thus making it
	** easier for the caller to distinguish this situation from other
	** errors
	*/

	if (sess_cb->pss_dbp_flags & PSS_DBPROC)
	    sess_cb->pss_dbp_flags |= PSS_MISSING_PRIV;

	if (psq_cb->psq_mode == PSQ_GRANT)
	{
	    if (cant_access)
	    {
		_VOID_ psf_error(E_US0890_2192_INACCESSIBLE_DBP, 0L, 
		    PSF_USERERR, &err_code, err_blk, 3,
		    sizeof("GRANT") - 1, "GRANT",
		    psf_trmwhite(sizeof(DB_OWN_NAME), (char *) dbp_owner),
		    dbp_owner,
		    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbp_name),
		    dbp_name);
	    }
	    else
	    {
	        _VOID_ psf_error(E_PS0551_INSUF_PRIV_GRANT_OBJ, 0L, PSF_USERERR,
		    &err_code, err_blk, 3, 
		    sizeof("EXECUTE") - 1, "EXECUTE",
		    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbp_name), 
		    dbp_name,
		    psf_trmwhite(sizeof(DB_OWN_NAME), (char *) dbp_owner), 
		    dbp_owner);
	    }
	}
	else if (   psq_cb->psq_mode == PSQ_CALLPROC
		 && sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	{
	    /* we were reparsing a dbproc to determine its grantability */
	    _VOID_ psf_error(E_PS0556_DBPGRANT_LACK_DBPPRIV, 0L, PSF_USERERR,
		&err_code, err_blk, 3,
		psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbp_name), dbp_name,
		psf_trmwhite(sizeof(DB_OWN_NAME), (char *) dbp_owner), 
		dbp_owner,
		psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) sess_cb->pss_dbpgrant_name),
		sess_cb->pss_dbpgrant_name);

	    /*
	    ** if we were processing a dbproc definition to determine if it is
	    ** grantable, record those of required privileges which the current
	    ** user does not posess
	    */
	    status = psy_insert_objpriv(sess_cb, dbp_id, 
		PSQ_OBJTYPE_IS_DBPROC | PSQ_MISSING_PRIVILEGE,
		save_privs & *required_privs, sess_cb->pss_dependencies_stream,
		&sess_cb->pss_indep_objs.psq_objprivs, err_blk);

	    if (DB_FAILURE_MACRO(status))
	    {
		must_audit = FALSE;	/* unexpected error - do not audit */

		/* invalid grantee type - close tuple stream and return */
		goto dbpperm_exit;
	    }
	}
	else	/* psq_cb->psq_mode in (PSQ_EXEDBP, PSQ_CALLPROC, PSQ_RULE) */
	{
	    _VOID_ psf_error(3382L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbp_name), dbp_name);
	}
    }
    else if (psq_cb->psq_mode == PSQ_GRANT)
    {
	/* for GRANT we only audit failure */
	must_audit = FALSE;
    }

dbpperm_exit:

    /* close the iiprotect system catalog if it has been successfully opened */

    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	rdf_rb->rdr_update_op = RDR_CLOSE;
	stat = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error, err_blk);
	    if (stat > status)
		status = stat;
	}
    }

audit_outcome:

    /* Audit result if executing procedure, but not executing a rule */

    if ( must_audit && Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
    {
	DB_ERROR    e_error;

	if (psq_cb->psq_mode == PSQ_EXEDBP || psq_cb->psq_mode == PSQ_CALLPROC)
	{
	    i4          access;

	    access = SXF_A_EXECUTE |
		((!*required_privs) ? SXF_A_SUCCESS : SXF_A_FAIL);

	    stat = psy_secaudit(FALSE, sess_cb,
	    		(char *)dbp_name, dbp_owner,
	    		sizeof(DB_DBP_NAME), SXF_E_PROCEDURE,
	      		I_SX201D_DBPROC_EXECUTE, access,
	      		&e_error);
	}
	else
	{
	    /* for GRANT we only audit failures */
	    stat = psy_secaudit(FALSE, sess_cb,
	    		(char *)dbp_name, dbp_owner,
	    		sizeof(DB_DBP_NAME), SXF_E_PROCEDURE,
	      		I_SX2017_PROT_DBP_CREATE,
			SXF_A_FAIL | SXF_A_CONTROL,
	      		&e_error);
	}

        if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS1003_ERR_AUDITING, e_error.err_code, PSF_INTERR,
		&err_code, err_blk, 0);
	    status = (status > stat) ? status : stat;
	}
    }

    return(status);
}

/*{
** Name: psy_cpyperm	- Check if the user has permission to copy a table
**
** Description:
**      This function fetches all protection tuples applicable to a given
**	table until there are no more left (failure) or one is found
**	that indicates that the user has permission to copy into or from.
**	
** Inputs:
**	cb				PSF session control block
**	    pss_user			current user identifier
**	perms_required			Permissions required to perform COPY:
**					    DB_RETRIEVE if COPY INTO
**					    DB_APPEND   if COPY FROM
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
**	    Does catalog I/O
**
** History:
**	19-jul-90 (ralph)
**	    Written.
**	23-jan-91 (andre)
**	    set rdr_2types_mask to 0 and rdr_instr to RDF_NO_INSTR
**	21-feb-91 (andre)
**	    call newly defined psy_grantee_ok() to determine if, based on
**	    grantee name and type, the permit applies to this session;
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	20-oct-93 (robf)
**          changed for new  COPY_INTO/FROM privileges
**	28-oct-93 (robf)
**          pass perms_required as i4 to avoid truncation
*/
DB_STATUS
psy_cpyperm(
	PSS_SESBLK  *cb,
	i4	    perms_required,
	DB_ERROR    *err_blk)
{

    RDF_CB	    rdf_cb;
    RDR_RB	    *rdf_rb = &rdf_cb.rdf_rb;
    QEF_DATA	    *qp;
    DB_PROTECTION   *protup;
    i4	    err_code;
    DB_STATUS	    status = E_DB_OK;
    DB_STATUS	    stat;
    i4		    tupcount;
    bool	    perm_found = FALSE;
    bool	    must_audit = FALSE;	    /* assume that we will succeed */
    i4	    tab_mask;
    i4		    access = 0;

    /*
    ** Fast path check for permission to COPY the table.
    ** Approve request if one of the following is met:
    **
    **      1) table is owned by this user
    **      2) table has all permission bit off
    **      3) table is syscat and COPY INTO or user has update syscat priv
    */

    tab_mask = cb->pss_resrng->pss_tabdesc->tbl_status_mask;

    if ((MEcmp((PTR) &cb->pss_resrng->pss_tabdesc->tbl_owner,
	       (PTR) &cb->pss_user, sizeof(DB_OWN_NAME)) == 0)
	||
	(!(tab_mask & DMT_ALL_PROT))
	||


	((tab_mask & (DMT_CATALOG|DMT_EXTENDED_CAT)) &&
	 ((perms_required == DB_COPY_INTO) ||
	  (cb->pss_ses_flag & PSS_CATUPD)))
       )

	return(E_DB_OK);			/* Session has permission */

    /*
    ** We need to check iiprotect tuples for required permissions.
    */

    /* get permit tuples that apply to this table from RDF */

    /* zero out RDF_CB and init common elements */
    pst_rdfcb_init(&rdf_cb, cb);

    rdf_cb.rdf_info_blk		= cb->pss_resrng->pss_rdrinfo;
    rdf_rb->rdr_types_mask	= RDR_PROTECT;

    STRUCT_ASSIGN_MACRO(cb->pss_resrng->pss_tabid, rdf_rb->rdr_tabid);

    rdf_rb->rdr_update_op	= RDR_OPEN;
    rdf_rb->rdr_qrymod_id	= 0;	/* get all tuples */
    rdf_rb->rdr_qtuple_count	= 20;	/* get 20 at a time */

    /* For each group of 20 permits */
    while ((rdf_cb.rdf_error.err_code == 0) &&
           (!perm_found))
    {
	status = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);

        /*
        ** RDF may choose to allocate a new info block and return its address in
        ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
        ** leak and other assorted unpleasantries
        */
	cb->pss_resrng->pss_rdrinfo = rdf_cb.rdf_info_blk;

	rdf_rb->rdr_update_op = RDR_GETNEXT;

	/* Must not use DB_FAILURE_MACRO because E_RD0011 returns
	** E_DB_WARN that would be missed.
	*/
	if (status != E_DB_OK)
	{
	    switch(rdf_cb.rdf_error.err_code)
	    {
		case E_RD0011_NO_MORE_ROWS:
		    status = E_DB_OK;
		    break;

		case E_RD0013_NO_TUPLE_FOUND:
		    status = E_DB_OK;
		    continue;

		default:
		    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error,
			err_blk);

		    /* unexpected error - skip auditing */

		    /* close the tuple stream and return */
		    goto cpyperm_exit;
	    }	/* switch */
	}	/* if status != E_DB_OK */

	/* For each permit tuple */
	for 
	(
	    qp = rdf_cb.rdf_info_blk->rdr_ptuples->qrym_data,
	    tupcount = 0;
	    tupcount < rdf_cb.rdf_info_blk->rdr_ptuples->qrym_cnt;
	    qp = qp->dt_next,
	    tupcount++
	)
	{
	    /* Set protup pointing to current permit tuple */
	    protup = (DB_PROTECTION*) qp->dt_data;

	    /*
	    ** See if tuple describes the required permission and
	    ** lists one of the session's authorization identifiers.
	    */
	    if (    (	/*
			** See if the required privileges are represented
			** in this tuple
			*/
			(protup->dbp_popset & perms_required) == perms_required
		    )
		    &&
		    (
			/* 
			** See if the permissions in this tuple are
			** GRANT compatible
			*/
			(   /* Check for qualification */
			    (protup->dbp_treeid.db_tre_high_time == 0) &&
			    (protup->dbp_treeid.db_tre_low_time == 0)
			)
			&&
			(   /* Check for DOW restriction */
			    (protup->dbp_pwbgn == 0) &&
			    (protup->dbp_pwend == 6)
			)
			&&
			(   /* Check for TOD retriction */
			    (protup->dbp_pdbgn == 0) &&
			    (protup->dbp_pdend == 1440)
			)
			&&
			(
			    /* Check terminal id */
			    STskipblank((char *)&protup->dbp_term,
					sizeof(protup->dbp_term))
				== NULL
			)
			&&
			(
			    /* Check for column-specific privileges */
			    protup->dbp_domset[0] == -1
			)
			&&
			(
			    /* Ensure this is not a security alarm */
			    !(protup->dbp_popset & DB_ALARM)
			)
			/*
			** @FIX_ME@ -- For TERMINATOR II
			** Do we want to honor column-specific insert privs?
			*/
		     )
	       )
	    {
		/*
		** everything looks OK; now check if the permit applies to
		** PUBLIC or to any of the session's identifiers (user, group,
		** or role)
		*/
		status = psy_grantee_ok(cb, protup, &perm_found, err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    /* unexpected error - skip auditing */
		    
		    /* bad grantee type - close catalog and return */
		    goto cpyperm_exit;
		}

		if (perm_found)
		{
		    break;
		}
	    }
	}	/* for */
    }	    /* while */

    if (!perm_found)	    
    {			/* user has no permission to COPY INTO/FROM */
	status = E_DB_ERROR;
	(VOID) psf_error((perms_required == DB_COPY_INTO) ? 5822L : 5813L, 0L,
	    PSF_USERERR, &err_code, err_blk, 1,
	    sizeof(DB_TAB_NAME), &cb->pss_resrng->pss_tabdesc->tbl_name);

	must_audit = TRUE;  /* user does not have permission - will audit */
    }

cpyperm_exit:

    /* close the iiprotect system catalog if it has been opened */

    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	rdf_rb->rdr_update_op = RDR_CLOSE;
	stat = rdf_call(RDF_READTUPLES, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb.rdf_error, err_blk);
	    if (stat > status)
		status = stat;
	}
    }

    if ( must_audit && Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
    {
	/* Audit the COPY failure */
	DB_ERROR e_error;

	access = (perms_required == DB_COPY_INTO) ? (SXF_A_COPYINTO|SXF_A_FAIL)
						 : (SXF_A_COPYFROM|SXF_A_FAIL);

	stat = psy_secaudit(FALSE, cb,
	    		(char *)&cb->pss_resrng->pss_tabdesc->tbl_name,
			&cb->pss_resrng->pss_tabdesc->tbl_owner,
	    		sizeof(DB_OWN_NAME), SXF_E_TABLE,
	      		I_SX2026_TABLE_ACCESS, access, 
			&e_error);

	if (stat > status)
	    status = stat;
    }

    return(status);
}

/*
** Name: psy_passed_prelim_check - check if the user has adequate permissions
**				   based on information available from
**				   IIRELATION (relstat).
**
** Input:
**	cb	    PSF session CB
**	rngvar	    range variable of interest
**	qmode	    query mode
**
** Output:
**	none
**
** Returns:
**	TRUE if user appears to posess the necessary permissions; FALSE
**	otherwise
**
** Side effects:
**	none
**
** History:
**	29-nov-90 (andre)
**	    written.
**	01-apr-93 (andre)
**	    if processing declaration of a QUEL cursor and
**	    this is the table/view over which the cursor is being defined and
**	    public possesses ALL or RETRIEVE on this table, remember that the
**	    user possesses RETRIEVE on every column of the table
*/
static bool
psy_passed_prelim_check(
	PSS_SESBLK	    *cb,
	PSS_RNGTAB	    *rngvar,
	i4		    qmode)
{
    i4	mask = rngvar->pss_tabdesc->tbl_status_mask;

    /*
    ** Check passed if "no restriction" bit asserted (= clear) or
    ** a catalog (extended cats included) and catalog update privilege.
    */
    if (~mask & DMT_ALL_PROT
	||
	(mask & DMT_CATALOG && (cb->pss_ses_flag & PSS_CATUPD))
       )
    {
	/*
	** if define cursor, mark delete permission ONLY if this is a table
	** for which cursaor is being declared (i.e. avoid marking cursor as
	** deleteable based on information about a table found in one of
	** the nested subselects)
	*/
	if ((qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
					&& rngvar == cb->pss_resrng)
	{
	    cb->pss_crsr->psc_delall = TRUE;

	    /*
	    ** if this is a QUEL cursor, remember that the user possesses
	    ** unconditional RETRIEVE on every column
	    */
	    if (cb->pss_lang == DB_QUEL)
		cb->pss_crsr->psc_flags |= PSC_MAY_RETR_ALL_COLUMNS;
	}

	return(TRUE);
    }

    /*
    ** Check for "retrieve all to all" bit asserted (= clear) or for
    ** catalogs.  This is important not only when doing a retrieve,
    ** but also when there is an implied retrieve in an update statement.
    ** All catalogs have retrieve all to all.
    */
    if (~mask & DMT_RETRIEVE_PRO || mask & DMT_CATALOG)
    {
	if (cb->pss_resrng != rngvar)
	{
	    return(TRUE);
	}
	else if ((qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
					&& cb->pss_lang == DB_QUEL)
	{
	    /*
	    ** user possesses unconditional RETRIEVE privilege on every column
	    ** of a table/view over which QUEL cursor is being defined
	    */
	    cb->pss_crsr->psc_flags |= PSC_MAY_RETR_ALL_COLUMNS;
	}
    }

    return(FALSE);
}

/*
** Name: dopro	- Do the protection algorithm
**
** Description:
**      This is the guts of the protection algorithm, broken off because it
**	must be called recursively on aggregates.  The algorithm is as
**	discussed in the module header.
**
** Inputs:
**      rngvar                          Pointer to range variable slot to get
**					permission on
**      root                            Root of query tree to get permission on
**      qmode                           Query mode of range variable, i.e. how
**					it's being used
**	query_mode			mode of the query
**	byset				Ptr to column set of `by' domains;
**					NULL if none
**	retrtoall			TRUE iff retrieve to all permission
**					given on this range variable
**	sess_cb				Pointer to session control block
**	rngtab				Pointer to current range table
**	rdf_cb				Pointer to RDF_CB to use for permit
**					tuples
**	rdf_tree_cb			Pointer to RDF_CB to use for query trees
**	dup_rb->			Ptr to dup. request block.
**	    pss_op_mask			indicator of requested services
**	    pss_num_joins		number of joins found in the tree so far
**	    pss_rsdm_no			resdom number which may be used to
**					renumber RESDOM nodes when copying a
**					tree
**	    pss_tree_info		ptr to mask to contain all sorts of
**					useful info about a tree beign
**					duplicated
**	    pss_mstream			memory stream to be used
**	    pss_err_blk			error block used when reporting errors
**	err_rngvar			range variable to pass to psy_error() if
**					adequate permissions were not found;
**					this will be NULL unless an SQL user
**					tries to (explicitly or implicitly)
**					retrieve from an index owned by another
**					user
**	attrmap				if the relation was used in definition
**					of a non-mergeable view
**					(rngvar->pss_rgparent != -1), attrmap
**					will contain map of attributes of the
**					relation, so we will not walk the tree,
**					but skip to the code that calls RDF to
**					read DB_PROTECT tuples; otherwise
**					attrmap will be set to NULL;
**					this will also be non-null if err_rngvar
**					is non-NULL (i.e. if we are checking
**					permissions on a table different from
**					the one specified by the user as would
**					occur when an SQL user tries to retrieve
**					from an index owned by another user)
**
** Outputs:
**      root                            Query tree may be modified
**	byset				Column set of by domains
**      rngtab                          New range vars may be entered
**	dup_rb->
**	    pss_err_blk			Filled in if an error happened
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
**	23-jun-86 (jeff)
**          Adapted from dopro() in 4.0
**	03-dec-87 (stec)
**	    Change psy_apql interface.
**	23-feb-88 (stec)
**	    When merging permit vars (if qual present), handle
**	    gracefully missing table case.
**	03-sep-88 (andre)
**	    Modify call to pst_rgent to pass 0 as a query mode since it is
**	    clearly not PSQ_DESTROY.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Pass session's group/application id's to proappl().
**	22-jun-89 (andre)
**	    dopro() will receive a real query mode along with effective query
**	    mode which have been passed up 'till now.
**	10-jul-89 (andre)
**	    Added new argument - mask - to communicate all sorts of useful info.
**	14-sep-89 (andre)
**	    Will accept a new parameter - dup_rb.  It will contain ptr to a
**	    memory stream and error block, in addition to being preset for
**	    passing to pst_treedup()
**      08-aug-90 (ralph)
**          Don't interpret security alarms as permits.
**	18-sep-90 (andre)
**	    added a new attribute - attrmap; this attribute will be used when
**	    the relation permissions on which need to be checked was used in the
**	    definition of a non-mergeable view, in which case the current
**	    algorithm will fail to find even the trace of the relation in the
**	    main query tree
**	25-oct-90 (andre)
**	    pndx should not be reset for each group of 20 permits as it
**	    effectively wipes out qualifications found on permits which were
**	    found prior to the last batch of 20 or less permits (bug 33971)
**	30-nov-90 (andre)
**	    changed interface to accept a pointer to an "error rngvar" instead
**	    of the "mask".  error rngvar will be non-NULL if weare checking
**	    permissions on a table other than the one from which the user was
**	    trying to retrieve data (this would occur if an SQL user tried to
**	    retrieve from an index owned by another user, and psy_protect()
**	    decided that we should use permissions granted on the index's base
**	    table to determine if the retrieval is to be permitted)
**	01-21-91 (andre)
**	    if any of the permits used involved a qualification, set
**	    PSS_QUAL_IN_PERMS in sess_cb->pss_stmt_flags.
**	12-sep-91 (andre)
**	    If processing CREATE/DEFINE VIEW, add the privileges required in
**	    order to create a view to the appropriate list in
**	    sess_cb->pss_indep_objs.
**	24-sep-91 (andre)
**	    If processing CREATE PROCEDURE, add the privileges required in
**	    order to create a dbproc to the appropriate list in
**	    sess_cb->pss_indep_objs.
**	30-sep-91 (andre)
**	    elements of the list of independent objects will be allocated from
**	    the stream pointed to by sess_cb->pss_dependencies_stream.
**	08-oct-91 (andre)
**	    When reparsing dbprocs to determine if they are grantable, the lists
**	    of independent object and column-specific privileges will be broken
**	    into sublists by the means of list header elements.
**
**	    A sublist can  be terminated with a list_header element or with NULL
**	    (the last sublist only).  A sublist currently under construction
**	    will have no header element.  If the dbproc was reparsed
**	    successfully, the sublist will be prefixed with a PSQ_DBPLIST_HEADER
**	    element which will also contain the id of the dbproc.  If parsing a
**	    dbproc resulted in an error, a partially completed sublist will be
**	    prefixed with a PSQ_INFOLIST_HEADER element.
**
**	    While the information collected in this latter case will not be used
**	    for creating a list of independent objects in IIPRIV, it may help us
**	    to avoid scans of IIPROTECT to collect information which has already
**	    been obtained.  Besides, the memory stream used for allocation of
**	    independent object list elements will remain open until the GRANT
**	    statement is parsed, so deleting entries of the sublist from the
**	    list will serve no purpose other than losing potentially useful
**	    info.
**	    
**	    Of special interest is the fact that sublists prefixed with
**	    PSQ_INFOLIST_HEADER elements will contain not only information about
**	    privileges which the current user posesses, but also the privileges
**	    which the current user does NOT posess, thus enabling us to advise
**	    the user that he lacks the necessary privileges without consulting
**	    IIPROTECT.
**
**	    I believe that being able to determine if the user posesses the
**	    required privileges without scanning IIPROTECT will help offset the
**	    extra expense involved in building the lists of independent objects
**	    ("aha", you may say, "wouldn't it be even cheaper to not build the
**	    lists at all?".  The answer is that we will avoid adding elements to
**	    the independent object lists whenever possible (i.e. if IIPROCEDURE
**	    indicates that a dbproc is grantable or if a list in IIPRIV already
**	    exists; unfortunately, neither of these is currently implemented, so
**	    for the time being we always take the more expensive route)
**	    but in some cases we need to determine if the dbproc is grantable
**	    and insert the dependency information into IIPRIV to facilitate the
**	    process of determining if permit(s) on a dbproc has become abandoned
**	    (coinsidently having this information will spare us the expense of 
**	    going through the reparsing process when a permit on one of the
**	    dbprocs which were reparsed is created.)
**	10-mar-93 (andre)
**	    if we discover that a user lacks UPDATE on some attributes in the
**	    FOR UPDATE clause of SQL DECLARE CURSOR, then rather than report an
**	    error, we will reset pst_rsupdt in the RESDOM nodes corresponding to
**	    these attributes which will let psl_crsopen() know that a user lacks
**	    UPDATE privilege on these attributes so that any future attempt to
**	    UPDATE any of them through the cursor would be thwarted
**	02-apr-93 (andre)
**	    if qmode is PSQ_RETRIEVE or PSQ_REPCURS, we are not interested in
**	    checking permits on attributes corresponding to resdom numbers (in
**	    the former case, because they are just an ordered list and do not
**	    necessarily correspond to attribute numbers and in the latter -
**	    because we checked for UPDATE privilege on columns in the FOR UPDATE
**	    list at DECLARE CURSOR time and there is no good reason to do it all
**	    over again)
**	27-nov-02 (inkdo01)
**	    Range table expansion.
*/
static DB_STATUS
dopro(
	PSS_RNGTAB	*rngvar,
	PST_QNODE	*root,
	i4		query_mode,
	i4		qmode,
	PSY_ATTMAP	*byset,
	bool		retrtoall,
	PSS_SESBLK	*sess_cb,
	PSS_USRRANGE	*rngtab,
	RDF_CB		*rdf_cb,
	RDF_CB		*rdf_tree_cb,
	PSS_DUPRB	*dup_rb,
	PSS_RNGTAB	*err_rngvar,
	PSY_ATTMAP      *attrmap)
{
    PSY_ATTMAP		qset, uset, aset, rset;
    i4			*uset_map;
    PST_QNODE		*p;
    i4			pndx = 0;
    i4			noperm;
    register PSS_RNGTAB	*var;
    register PST_QNODE  *t;
    PSY_QMAP		qmap[PSY_MQMAP];
    PSY_PQUAL		pqual[PSY_MPQUAL];
    i4		err_code;
    DB_STATUS		status;
    bool		rootnode = FALSE;
    i4			opmask;
    bool		skipped_check_permits = FALSE;

    /*
    ** the following 4 vars will be used when we are reparsing a dbproc as a
    ** part of processing GRANT ON PROCEDURE
    */
    i4			tblwide_privs = 0, colspecific_privs = 0;
    PSQ_OBJPRIV		*tblpriv_element = (PSQ_OBJPRIV *) NULL;
    PSQ_COLPRIV		*colpriv_element = (PSQ_COLPRIV *) NULL;
#ifdef    xDEBUG
    i4		val1, val2;
#endif

    t = root;
    var = rngvar;

    /* create domain usage sets */
    psy_fill_attmap(uset.map, (i4) 0);
    STRUCT_ASSIGN_MACRO(uset, rset);
    STRUCT_ASSIGN_MACRO(uset, aset);
    STRUCT_ASSIGN_MACRO(uset, qset);

    /* If `byset' to be returned, initialize it first. */
    if (byset != (PSY_ATTMAP *) NULL)
    {
	STRUCT_ASSIGN_MACRO(uset, (*byset));
    }

    /*
    ** if the current relation was used in the definition of a non-mergeable
    ** view, we already have the attribute map to check, so copy it to rset, and
    ** skip the regular tree-walking part of the algorithm;
    ** the same applies if err_rngvar is non-NULL (if an SQL user tried to
    ** retrieve from an index owned by another user, we will check if he is
    ** allowed to retrieve from the index's base table instead; if he lacks
    ** necessary permissions, err_rngvar will be passed to psy_error() to
    ** complain about the lack of necessary permissions)
    */
    if (var->pss_rgparent != -1 || err_rngvar)
    {
	STRUCT_ASSIGN_MACRO((*attrmap), rset);
	goto check_permits;
    }

    /*
    ** if qmode is PSQ_RETRIEVE or PSQ_REPCURS, we are not interested in
    ** info that may be collected in user by makedset() (see 2-apr-93 (andre)
    ** comment)
    */
    uset_map = (qmode == PSQ_RETRIEVE || qmode == PSQ_REPCURS) ? (i4 *) NULL
							       : uset.map;
    
    /*
    **  Create domain usage set for target list side.  There are
    **  two general cases: this is the root of the tree, or this
    **  is the head of an aggregate.
    */

    switch (t->pst_sym.pst_type)
    {
    case PST_AGHEAD:
	/*
	**  An aggregate head falls into two classes: simple
	**  aggregate and aggregate function.  In an aggregate
	**  function, care must be taken to bind the variables
	**  in the by-list outside of the aggregate.  We use
	**  'rset' as a temporary here.
	*/

	if (t->pst_left->pst_sym.pst_type == PST_BYHEAD)
	{
	    /* make by-list set */
	    status = makedset(var, t->pst_left->pst_left, query_mode, qmode,
	            retrtoall, sess_cb, rngtab, (i4 *) NULL, rset.map,
		    aset.map, qset.map, rdf_cb, rdf_tree_cb, dup_rb);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    /* merge by-list set into qualification set */
	    if (byset != (PSY_ATTMAP *) NULL)
	    {
		BTor((i4) BITS_IN(rset), (char *) &rset, (char *) byset);
	    }

	    BTor((i4) BITS_IN(rset), (char *) &rset, (char *) &qset);
	    psy_fill_attmap(rset.map, (i4) 0);

	    /* make aggregate list set */
	    status = makedset(var, t->pst_left->pst_right->pst_left, query_mode,
	        qmode, retrtoall, sess_cb, rngtab, (i4 *) NULL,
		rset.map, aset.map, qset.map, rdf_cb, rdf_tree_cb, dup_rb);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	else
	{
	    /* simple aggregate */
#	    ifdef xDEV_TEST
	    if (t->pst_left->pst_sym.pst_type != PST_AOP)
	    {
		(VOID) psf_error(E_PS0D0F_BAD_AGGREGATE, 0L, PSF_INTERR,
		    &err_code, dup_rb->pss_err_blk, 0);
		return (E_DB_ERROR);
	    }
#	    endif
			
	    /* check for qualification */
	    if (t->pst_right->pst_sym.pst_type == PST_QLEND)
	    {
		/* simple, unqualified aggregate */
		status = makedset(var, t->pst_left->pst_left, query_mode, qmode,
		    retrtoall, sess_cb, rngtab, (i4 *) NULL, aset.map,
		    aset.map, qset.map, rdf_cb, rdf_tree_cb, dup_rb);
		if (DB_FAILURE_MACRO(status))
		    return (status);
	    }
	    else
	    {
		status = makedset(var, t->pst_left->pst_left, query_mode, qmode,
		    retrtoall, sess_cb, rngtab, (i4 *) NULL, rset.map,
		    aset.map, qset.map, rdf_cb, rdf_tree_cb, dup_rb);
		if (status)
		    return (status);
	    }
	}
	break;
	
    case PST_ROOT:
	if (t->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	{
	    /* do protection on other members of the union (if any) */
	    status = dopro(var, 
		t->pst_sym.pst_value.pst_s_root.pst_union.pst_next, query_mode,
		qmode, (PSY_ATTMAP *) NULL, retrtoall, sess_cb, rngtab, rdf_cb,
		rdf_tree_cb, dup_rb, (PSS_RNGTAB *) NULL, (PSY_ATTMAP *) NULL);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	/* Fall through */

    case PST_SUBSEL:
	status = makedset(var, t->pst_left, query_mode, qmode, retrtoall,
	    sess_cb, rngtab, uset_map, rset.map, aset.map, qset.map, rdf_cb,
	    rdf_tree_cb, dup_rb);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	rootnode = TRUE;
	break;

    default:
	{
	    (VOID) psf_error(E_PS0D30_UNEXPECTED_NODE, 0L, PSF_INTERR,
		&err_code, dup_rb->pss_err_blk, 0);
	    return (E_DB_SEVERE);
	    break;
	}
    }

    /* scan qualification */
    status = makedset(var, t->pst_right, query_mode, qmode, retrtoall, sess_cb,
	rngtab, (i4 *) NULL, qset.map, aset.map, qset.map, rdf_cb,
	rdf_tree_cb, dup_rb);
    if (DB_FAILURE_MACRO(status))
	return (status);

    if (qmode == PSQ_DELETE)	   /* if delete, you need DELETE on the table */
    {
	psy_fill_attmap(uset.map, ~((i4) 0));
    }

	    /*
	    ** if we are trying to check permissions on a table which was used
	    ** in a definition of a non-mergeable view, we were given the
	    ** attribute map, so we didn't have to apply the usual algorithm
	    ** (and a good thing too, since it would not find any attributes, as
	    ** it has not been merged into the main query tree.)
	    ** In this case we simply skip to the following label
	    */
check_permits:

    /*
    ** If SQL, then, except for updates (replace), if you need one attribute,
    ** you need them all. We go through the problem of makedset so we
    ** can append qualifications properly.
    */
    if (sess_cb->pss_lang == DB_SQL)
    {
	i4 afound, rfound, qfound, ufound;

	ufound = (BTnext(-1, (char *) &uset, DB_MAX_COLS + 1) >= 0);
	rfound = (BTnext(-1, (char *) &rset, DB_MAX_COLS + 1) >= 0);
	afound = (BTnext(-1, (char *) &aset, DB_MAX_COLS + 1) >= 0);
	qfound = (BTnext(-1, (char *) &qset, DB_MAX_COLS + 1) >= 0);

	/*
	** We need to take care of the case when a relation has
	** been specified in the from clause, but its attributes
	** are not used in any expression (`cartesian product' case).
	** So, when no domains have been specified and we have a root type
	** node, and current range var is in the bitmap of the root node
	** set the `rset' bits on.
	*/
	if (!ufound && !afound && !qfound && !rfound)
	{
	    if (var->pss_rgparent != -1
		||
		(rootnode &&
		 BTtest(var->pss_rgno,
		    (char *) &t->pst_sym.pst_value.pst_s_root.pst_tvrm))
	       )
	    {
		psy_fill_attmap(rset.map, ~((i4) 0));
	    }
	    else
	    {
		/*
		** Can't see how this would ever happen,
		** so we'll leave it blank.  Take it back, this could happen if
		** we are processing a target list of a query and the list
		** contains no attributes (e.g. select count(*) from x)
		*/
	    }
	}
	else
	{
	    if (qmode == PSQ_APPEND)
		if (ufound)
		{
		    psy_fill_attmap(uset.map, ~((i4) 0));
		}

	    /*
	    ** in SQL, there is no reason at all to distinguish between
	    ** DB_RETRIEVE, DB_TEST, and DB_AGGREGATE - they all require a
	    ** table-wide SELECT privilege.  To make matters simpler (and a tiny
	    ** bit faster) any combination of the three privileghes will be
	    ** represented by DB_RETRIEVE
	    */
	    if (afound)
	    {
		rfound = TRUE;
		psy_fill_attmap(aset.map, ((i4) 0));
	    }

	    if (qfound)
	    {
		rfound = TRUE;
		psy_fill_attmap(qset.map, ((i4) 0));
	    }

	    if (rfound)
	    {
		psy_fill_attmap(rset.map, ~((i4) 0));
	    }
	}
    }

#ifdef xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
    {
	TRdisplay("In dopro:\n");
	TRdisplay("qmode %d\n", qmode);
	pr_set(uset.map, "uset");
	pr_set(rset.map, "rset");
	pr_set(aset.map, "aset");
	pr_set(qset.map, "qset");
    }
#endif

    /* Get the bit mask and bit position for the current operation */
    switch (qmode)
    {
	case PSQ_RETRIEVE:
	{
	    opmask = DB_RETRIEVE;
	    break;
	}
	case PSQ_APPEND:
	{
	    opmask = DB_APPEND;
	    break;
	}
	case PSQ_REPLACE:
	case PSQ_DEFCURS:
	case PSQ_REPDYN:
	{
	    opmask = DB_REPLACE;
	    break;
	}
	case PSQ_DELETE:
	{
	    opmask = DB_DELETE;
	    break;
	}
	default:
	{
	    (VOID) psf_error(E_PS0D0E_BAD_QMODE, 0L, PSF_INTERR, &err_code,
		dup_rb->pss_err_blk, 0);
	    return (E_DB_ERROR);
	}
    }

    /* create a bit map of all referenced operations */
    noperm = (i4) 0;
    if (BTnext(-1, (char *) &uset, DB_MAX_COLS + 1) >= 0)
	noperm |= opmask;

    /*
    ** if parsing a definition of a view or a dbproc, postpone using retrtoall
    ** until after we are done processing maps of independent privileges
    */
    if (   query_mode == PSQ_VIEW
        || sess_cb->pss_dbp_flags & PSS_DBPROC
	|| !retrtoall)
    {
	/*
	** Note that if this is an update operation and retrieve to all
	** permission has been given, the retrieve, aggregate, and qualification
	** set automatically succeeds
	*/

	if (BTnext(-1, (char *) &rset, DB_MAX_COLS + 1) >= 0)
	    noperm |= DB_RETRIEVE;
	if (BTnext(-1, (char *) &aset, DB_MAX_COLS + 1) >= 0)
	    noperm |= DB_AGGREGATE;
	if (BTnext(-1, (char *) &qset, DB_MAX_COLS + 1) >= 0)
	    noperm |= DB_TEST;
    }

    /*
    ** if we are processing CREATE/DEFINE VIEW or CREATE PROCEDURE, before
    ** calling psy_check_permits() to check if the user has the required
    ** privileges on the object, check if some of the privileges have already
    ** been checked.  Note that checking performed if we are processing
    ** CREATE PROCEDURE will be different from the CREATE VIEW case because
    ** required privileges for the former are not limited to DB_RETRIEVE.
    **
    ** Privileges which are still to be checked will be entered into the list
    ** of independent privileges.
    **
    ** If all privileges have already been checked, we do not need to scan
    ** IIPROTECT for this range variable.
    */
    if (!noperm)
    {
	/*
	** no permits to check - as I mentioned above, this could indeed happen
	*/
    }
    else if (query_mode == PSQ_VIEW)
    {
	if (sess_cb->pss_lang == DB_SQL)
	{
	    /*
	    ** for CREATE VIEW the only privilege of interest here is a
	    ** table-wide SELECT, so we simply scan down the list of table-wide
	    ** privilege descriptors looking for an element with matching table
	    ** id.
	    */
	    PSQ_OBJPRIV	    *tbl_priv;

	    for (tbl_priv = sess_cb->pss_indep_objs.psq_objprivs;
		 tbl_priv;
		 tbl_priv = tbl_priv->psq_next
		)
	    {
		if (tbl_priv->psq_objid.db_tab_base ==
		    var->pss_tabid.db_tab_base
		    &&
		    tbl_priv->psq_objid.db_tab_index ==
		    var->pss_tabid.db_tab_index
		   )
		{
		    /*
		    ** we will not need to check if the user can select from
		    ** this table - set noperm to 0 to avoid calling
		    ** psy_check_permits()
		    */
		    noperm = 0;
		    break;
		}
	    }

	    if (noperm)
	    {
		/*
		** need to add a privilege descriptor to the list of table-wide
		** independent privileges
		*/
		status = psy_insert_objpriv(sess_cb, &var->pss_tabid,
		    PSQ_OBJTYPE_IS_TABLE, (i4) DB_RETRIEVE,
		    sess_cb->pss_dependencies_stream,
		    &sess_cb->pss_indep_objs.psq_objprivs, dup_rb->pss_err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}
	    }
	}
	else
	{
	    /*
	    ** for DEFINE VIEW, we need to look at the list of column-specific
	    ** privileges;
	    ** unlike CREATE VIEW case we actually need to find a privilege
	    ** descriptor describing a column-wide privilege on a superset of
	    ** columns in the privilege which we are expected to satisfy
	    */
	    PSQ_COLPRIV	    *col_priv;

	    for (col_priv = sess_cb->pss_indep_objs.psq_colprivs;
		 noperm && col_priv;
		 col_priv = col_priv->psq_next
		)
	    {
		if (col_priv->psq_tabid.db_tab_base ==
		    var->pss_tabid.db_tab_base
		    &&
		    col_priv->psq_tabid.db_tab_index ==
		    var->pss_tabid.db_tab_index
		   )
		{
		    if (noperm & DB_RETRIEVE &&
			BTsubset((char *) rset.map,
			    (char *) col_priv->psq_attrmap, DB_MAX_COLS + 1))
		    {
			noperm &= ~DB_RETRIEVE;
		    }

		    if (noperm & DB_AGGREGATE &&
			BTsubset((char *) aset.map,
			    (char *) col_priv->psq_attrmap, DB_MAX_COLS + 1))
		    {
			noperm &= ~DB_AGGREGATE;
		    }

		    if (noperm & DB_TEST &&
			BTsubset((char *) qset.map,
			    (char *) col_priv->psq_attrmap, DB_MAX_COLS + 1))
		    {
			noperm &= ~DB_TEST;
		    }
		}
	    }

	    if (noperm)
	    {
		/*
		** need to add privilege(s) descriptor to the list of
		** column-specific independent privileges
		*/

		if (noperm & DB_RETRIEVE)
		{
		    status = psy_insert_colpriv(sess_cb, &var->pss_tabid,
			PSQ_OBJTYPE_IS_TABLE, rset.map, (i4) DB_RETRIEVE,
			sess_cb->pss_dependencies_stream,
			&sess_cb->pss_indep_objs.psq_colprivs,
			dup_rb->pss_err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(status);
		    }
		}

		if (noperm & DB_AGGREGATE)
		{
		    status = psy_insert_colpriv(sess_cb, &var->pss_tabid,
			PSQ_OBJTYPE_IS_TABLE, aset.map, (i4) DB_RETRIEVE,
			sess_cb->pss_dependencies_stream,
			&sess_cb->pss_indep_objs.psq_colprivs,
			dup_rb->pss_err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(status);
		    }
		}

		if (noperm & DB_TEST)
		{
		    status = psy_insert_colpriv(sess_cb, &var->pss_tabid,
			PSQ_OBJTYPE_IS_TABLE, qset.map, (i4) DB_RETRIEVE,
			sess_cb->pss_dependencies_stream,
			&sess_cb->pss_indep_objs.psq_colprivs,
			dup_rb->pss_err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(status);
		    }
		}
	    }
	}
    }
    else if (sess_cb->pss_dbp_flags & PSS_DBPROC)
    {
	bool			missing;
	i4			priv;

	if (priv = (noperm & (DB_DELETE | DB_APPEND | DB_RETRIEVE)))
	{
	    status = psy_check_objprivs(sess_cb, &priv, &tblpriv_element,
		&sess_cb->pss_indep_objs.psq_objprivs, &missing,
		&var->pss_tabid, sess_cb->pss_dependencies_stream,
		PSQ_OBJTYPE_IS_TABLE, dup_rb->pss_err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		return(status);
	    }
	    else if (missing)
	    {
		if (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
		{
		    /* dbproc is clearly ungrantable now */
		    sess_cb->pss_dbp_flags &= ~PSS_DBPGRANT_OK;
		}

		if (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
		{
		    /*
		    ** code under saw_the_perms expects DB_GRANT_OPTION bit to
		    ** be set if we are reparsing a dbproc to determine if it is
		    ** grantable
		    **
		    ** code under saw_the_perms: expects tblwide_privs to
		    ** contain privileges for which it had to be determined
		    ** whether the current user posesses them
		    */
		    noperm = (tblwide_privs = priv) | DB_GRANT_OPTION;
		}

		goto saw_the_perms;
	    }
	    else
	    {
		/*
		** priv contains map of table-wide privileges (if any) yet to be
		** satisfied; merge it into noperm
		*/
		noperm &= (~(DB_DELETE | DB_APPEND | DB_RETRIEVE) | priv);
	    }
	}

	if (noperm & DB_REPLACE)
	{
	    priv = DB_REPLACE;

	    status = psy_check_colprivs(sess_cb, &priv, &colpriv_element,
		&sess_cb->pss_indep_objs.psq_colprivs, &missing,
		&var->pss_tabid, uset.map, sess_cb->pss_dependencies_stream,
		dup_rb->pss_err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		return(status);
	    }
	    else if (missing)
	    {
		if (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
		{
		    /* dbproc is clearly ungrantable now */
		    sess_cb->pss_dbp_flags &= ~PSS_DBPGRANT_OK;
		}

		/*
		** code under saw_the_perms expects DB_GRANT_OPTION bit to
		** be set if we are reparsing a dbproc to determine if it is
		** grantable
		*/
		if (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
		{
		    /*
		    ** code under saw_the_perms: expects colspecific_privs to
		    ** contain privileges for which it had to be determined
		    ** whether the current user posesses them
		    */
		    noperm = DB_REPLACE | DB_GRANT_OPTION;
		    colspecific_privs = DB_REPLACE;
		}
		else
		{
		    noperm = DB_REPLACE;
		}

		goto saw_the_perms;
	    }
	    else if (!priv)
	    {
		noperm &= ~DB_REPLACE;
	    }
	}
    }

    if (!noperm)
	return(E_DB_OK);

    /*
    ** if we are parsing a dbproc to determine if its owner can grant permit on
    ** it, we definitely want all of the privileges WGO
    */

    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
    {
	if (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	{
	    noperm |= DB_GRANT_OPTION;
	}

	/*
	** remember which table-wide and column-specific (for now it can only be
	** DB_REPLACE) privileges we still need
	*/
	tblwide_privs = noperm & (DB_APPEND | DB_DELETE | DB_RETRIEVE);
	colspecific_privs = noperm & DB_REPLACE;
    }

    /*
    ** if processing a definition of a view or a dbproc, we have avoided using
    ** relstat bits (all/retrieve-to-all) to determine if the user has the
    ** necessary privileges; now that we have processed the maps of independent
    ** privileges, we are ready to look at these bits...
    ** UNLESS we are parsing a dbproc and it appears to be grantable (this also
    ** includes the case when the dbproc MUST be grantable, i.e.
    ** (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK) != 0).  The catch
    ** here is that relstat will indicate whether the public is allowed to
    ** perform an operation, and not whether the current user may grant to other
    ** users a privilege to perform it.  
    */
    if (query_mode == PSQ_VIEW 
	|| (sess_cb->pss_dbp_flags & PSS_DBPROC
	    && ~sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK))
    {
	if (psy_passed_prelim_check(sess_cb, var, query_mode))
	{
	    noperm = 0;
	}
	else if (retrtoall)
	{
	    noperm &= ~(DB_RETRIEVE | DB_AGGREGATE | DB_TEST);
	}
    }

    /*
    ** if no operation, something is wrong, or, in the case of CREATE/DEFINE
    ** VIEW or CREATE PROCEDURE, we have already located privilege descriptors
    ** satisfying this request;
    ** if we are NOT processing a dbproc definition and there are no privileges
    ** left to check, we are done; for dbprocs, we still need to add
    ** descriptions of privileges on which they depended, so we'll arrange to
    ** skip the call to psy_check_permits() and fall into the code that builds
    ** descriptors for privileges which the user possesses
    */

    if (noperm)
    {
	status = psy_check_permits(sess_cb, var, &uset, &rset, &aset, &qset,
	    &noperm, query_mode, qmode, dup_rb, qmap, pqual, rdf_cb,
	    rdf_tree_cb, rngtab, &pndx);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}

#ifdef xDEBUG
	if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	{
	    TRdisplay("Qmaps before applypq:\n");
	    pr_qmap(pndx, qmap);
	    TRdisplay("No perm on 0x%x\n", noperm);
	}
#endif
    }
    else if (~sess_cb->pss_dbp_flags & PSS_DBPROC)
    {
	return(E_DB_OK);
    }
    else
    {
	skipped_check_permits = TRUE;
    }
    
saw_the_perms:

    /*
    ** if processing a dbproc, we will record the privileges which the current
    ** user posesses if
    **	    - user posesses all the required privileges or
    **	    - we were parsing the dbproc to determine if it is grantable (in
    **	      which case the current dbproc will be marked as ungrantable, but
    **	      the privilege information may come in handy when processing the
    **	      next dbproc mentioned in the same GRANT statement.)
    */
    if (   sess_cb->pss_dbp_flags & PSS_DBPROC
	&& (!noperm || sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK))
    {
	/*
	** if some table-wide privilege(s) have been satisfied, store the
	** information in the privilege list
	*/
	if (tblwide_privs & ~noperm)
	{
	    if (tblpriv_element)
	    {
		tblpriv_element->psq_privmap |= (tblwide_privs & ~noperm);
	    }
	    else
	    {
		status = psy_insert_objpriv(sess_cb, &var->pss_tabid,
		    PSQ_OBJTYPE_IS_TABLE, tblwide_privs & ~noperm,
		    sess_cb->pss_dependencies_stream,
		    &sess_cb->pss_indep_objs.psq_objprivs, dup_rb->pss_err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}
	    }
	}

	/*
	** if the column-specific privilege has been satisfied, store the
	** information in the privilege list
	*/
	if (colspecific_privs & ~noperm)
	{
	    if (colpriv_element)
	    {
		/*
		** merge the list of attributes for which the privilege was
		** required into an existing descriptor
		*/
		BTor(DB_MAX_COLS + 1, (char *) uset.map,
		         (char *) colpriv_element->psq_attrmap);
	    }
	    else
	    {
		/* create a new descriptor */
		status = psy_insert_colpriv(sess_cb, &var->pss_tabid,
		    PSQ_OBJTYPE_IS_TABLE, uset.map,
		    colspecific_privs & ~noperm,
		    sess_cb->pss_dependencies_stream,
		    &sess_cb->pss_indep_objs.psq_colprivs,
		    dup_rb->pss_err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}
	    }
	}

	/*
	** if we determined that the user possesses sufficient privileges based
	** on relstat alone, we want to return now since some of the following
	** code assumes that you've been through psy_check_permits() and blows
	** up if you have not
	*/
	if (skipped_check_permits)
	    return(E_DB_OK);
    }

    /* see if no tuples applied for some operation */
    if (noperm != (i4) 0)
    {
	/*
	** if processing DECLARE CURSOR and determined that the user lacks
	** UPDATE on some of the attributes in the FOR UPDATE list, rather than
	** report an error, reset pst_rsupdt in the corresponding RESDOM nodes
	** to indicate to psl_crsopen() that even though these attributes
	** appeared in the FOR UPDATE list, any attempt to update them through
	** the cursor should result in an error
	**
	** for all other cases, we will continue returning error
	*/
	if ((qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
					&& noperm == DB_REPLACE)
	{
	    PST_QNODE	    *tree_node;
	    
	    /*
	    ** uset contains a map of attributes on which the user lacks UPDATE;
	    ** we will scan the list of RESDOM nodes which have pst_rsupdt set
	    ** and reset it for those corresponding to attribute(s) on which the
	    ** user lacks UPDATE privilege
	    */

	    for (tree_node = root; tree_node; tree_node = tree_node->pst_left)
	    {
		if (   tree_node->pst_sym.pst_type == PST_RESDOM
		    && tree_node->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt
		    && tree_node->pst_right->pst_sym.pst_type == PST_VAR
		    && BTtest(
			(i4) tree_node->pst_right->pst_sym.pst_value.pst_s_var.
			    pst_atno.db_att_id, (char *) &uset)
		   )
		{
		    tree_node->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt = FALSE;
		}
	    }
	    
	    noperm = 0;
	}
	else
	{
	    char	    buf[128];  /* buffer for missing privilege string */

	    psy_prvmap_to_str(noperm, buf, sess_cb->pss_lang);

	    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
		sess_cb->pss_dbp_flags |= PSS_MISSING_PRIV;

	    if (noperm & DB_GRANT_OPTION)
	    {
		psy_error(E_PS0555_DBPGRANT_LACK_TBLPRIV, -1,
		    (err_rngvar != (PSS_RNGTAB *) NULL) ? err_rngvar : var,
		    dup_rb->pss_err_blk,
		    psf_trmwhite(sizeof(DB_DBP_NAME), 
			(char *) sess_cb->pss_dbpgrant_name),
		    sess_cb->pss_dbpgrant_name, STlength(buf), buf);
	    }
	    else
	    {
		psy_error((sess_cb->pss_lang == DB_SQL ? 3502L : 3500L), -1,
		    (err_rngvar != (PSS_RNGTAB *) NULL) ? err_rngvar : var,
		    dup_rb->pss_err_blk, STlength(buf), buf);
	    }

	    if (   sess_cb->pss_dbp_flags & PSS_DBPROC
		&& sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	    {
		/*
		** if some table-wide privilege(s) have not been satisfied,
		** store the information in the privilege list
		*/
		if (tblwide_privs & noperm)
		{
		    status = psy_insert_objpriv(sess_cb, &var->pss_tabid,
			PSQ_OBJTYPE_IS_TABLE | PSQ_MISSING_PRIVILEGE,
			tblwide_privs & noperm,
			sess_cb->pss_dependencies_stream, 
			&sess_cb->pss_indep_objs.psq_objprivs,
			dup_rb->pss_err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(status);
		    }
		}

		if (colspecific_privs & noperm)
		{
		    /*
		    ** user lacks the column-specific privilege for some of the
		    ** attributes;  if this was determined by actually scanning
		    ** IIPROTECT, uset will contain a map of attributes on which
		    ** the user lacks the column-specific privilege; if, on the
		    ** other hand, this was determined by scanning previously
		    ** accumulated descriptors, uset will contain a map of
		    ** required attributes which were found in the first
		    ** "missing privilege" descriptor which indicated that the
		    ** userr lacks privilege on at least one of required
		    ** attributes.
		    ** The descriptor will be used as follows: if by scanning a
		    ** privilege list we determine that some dbproc owned by the
		    ** current user in nongrantable, we can scan the sublist
		    ** which was created when the dbproc was reparsed and
		    ** provide the user with information as to which privileges
		    ** he lacks.
		    */
		    status = psy_insert_colpriv(sess_cb, &var->pss_tabid,
			PSQ_OBJTYPE_IS_TABLE | PSQ_MISSING_PRIVILEGE, uset.map,
			colspecific_privs & noperm,
			sess_cb->pss_dependencies_stream,
			&sess_cb->pss_indep_objs.psq_colprivs,
			dup_rb->pss_err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(status);
		    }
		}
	    }

	    return (E_DB_ERROR);
	}
    }
	
    /*
    ** ATTENTION:
    ** Here we reset pss_op_mask to 0, since we do not want to modify group_id
    ** field inside opnodes if we have to make multiple copies of qualifications
    ** in gorform().  The necessary adjustment was made above when we called
    ** pst_treedup() to duplicate tree given to us by RDF.
    */
    dup_rb->pss_op_mask = 0;
    
    status = applypq(sess_cb, pndx, qmap, pqual, FALSE, &p, dup_rb);
    if (DB_FAILURE_MACRO(status))
	return (status);

    if (p != (PST_QNODE *) NULL)
    {
	sess_cb->pss_stmt_flags |= PSS_QUAL_IN_PERMS;
    }

#ifdef    xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 51, &val1, &val2))
    {
	TRdisplay("Protect qual for var %d\n", var->pss_rgno);
	(VOID) pst_prmdump(p, (PST_QTREE *) NULL, dup_rb->pss_err_blk,
			   DB_PSF_ID);
    }
#endif

    /* get the collective from lists for the qualifications added */
    {
	PST_J_MASK    from_list;

	/* Since quals can only come from QUEL, and quel has no subselect,
	** the from list is the same as the map of the vars.
	*/
	MEfill(sizeof(PST_J_MASK), 0, (char *)&from_list);
	pst_map(p, &from_list);
	BTor(PST_NUMVARS, (char *)&from_list, 
		(char *)&t->pst_sym.pst_value.pst_s_root.pst_tvrm);

	/* Append the qualification to the user's query */
	status = psy_apql(sess_cb, dup_rb->pss_mstream, p, t,
			  dup_rb->pss_err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }
    return (E_DB_OK);
}

/*{
** Name: applypq	- Make minimized Boolean expr. of protection quals
**
** Description:
**	Qmap contains the bit-maps indicating which quals apply toward each
**	operation (update, retrieve, qualify, or aggregate) active in the query.
**	Pqual contains the protection qualifications.
**
**	The basic algorithm is:
**		1. For each active operation for which there is a non-null
**		   qmap, form the disjunction of all referenced pquals, being
**		   careful to create duplicates of any pquals referenced more
**		   than once.
**		2. Form the conjunction of all such qmap qualifications.
**		3. Conjoin this result to the original query tree.
**	
**	However, this can produce considerable redundancy in the resultant
**	Boolean expression.  For example, let A, B, C, ... represent pquals.
**	Given the following bit-map:
**
**			    pquals
**			    A   B   C   D
**		qmap[0] U   1   1   1
**		qmap[1] R   1   1       1
**		qmap[2] A
**		qmap[3] Q   1   1   1 
**
**	the simple algorithm would produce:
**		(A or B or C) and (A or B or D) and (A or B or C)
**	which could be simplified to:
**		A or B or (C and D)
**
**	Therefore we modify the algorithm to produce a minimized expression as
**	follows:
**	    (Algorithm PQ)
**		1. Form bit-wise AND of all (non-zero) qmaps into gormap
**		("global-OR-map").  Each bit in gormap represents a pqual that
**		can be factored out and OR'd globally with the remaining
**		resultant conjunctive expression.  If there are any active
**		all-zero qmaps we can return (NULL) immediately since we can
**		show that the remaining qualification must be logically
**		redundant (exercise for the reader).  (This check need not be
**		done first time called, as all active qmaps will be non-zero.)
**		2. If gormap is non-zero, disjoin all gormap pquals to form
**		QUAL, checking for pqual duplication.
**		3. Mask out all gormap bits from all qmaps.
**		4. Recursively generate remaining protection qual, and if
**		non-null DISJOIN it to QUAL.
**		5. Else pick one of the qmaps to eliminate, call it Z.  (We
**		can use some algorithm to pick the "best" one -- e.g., the one
**		such that the gormap of the remaining qmaps would contain the
**		most pquals.)
**		6. Disjoin all pquals in Z, forming QUAL, checking for pqual
**		duplication, and marking Z as inactive.
**		7. Recursively generate remaining protection qual based on
**		rest of qmaps and if non-null CONJOIN it to Z.
**		8. Return QUAL.
**
** Inputs:
**	size				Size of qualification map
**	qmap				The qualification map
**	pqual				Qualification list
**	checkany			TRUE means check for any all-zero maps
**	outtree				Pointer to place to put pointer to
**					qualification tree
**	dup_rb				ptr to request block used by pst_treedup
**					conveniently, it contains memory stream
**					and error block ptrs (among other
**					things). Other fields have been preset
**					to be passed to gorform(), and, if
**					necesasary, to pst_treedup().
**	    pss_mstream			memory stream
**	    pss_err_blk			error block.
**
** Outputs:
**      outtree                         Filled in with pointer to qualification
**					tree
**	dup_rb->
**	    pss_err_blk			Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	24-jun-86 (jeff)
**	    Adapted from applypq() in 4.0
**	14-sep-89 (andre)
**	    changed interface (slightly.)  ptrs to memory stream and error block
**	    are passed inside the request block to be passed to gorform() for
**	    use by pst_treedup().
*/
static DB_STATUS
applypq(
	PSS_SESBLK  *sess_cb,
	i4	    size,
	PSY_QMAP    qmap[],
	PSY_PQUAL   pqual[PSY_MPQUAL],
	i4	    checkany,
	PST_QNODE   **outtree,
	PSS_DUPRB   *dup_rb)
{
    PQMAP	gormap;
    PST_QNODE	*qual = NULL,
		*q;
    i4		i,
		z;
    bool	inited = FALSE;
    DB_STATUS	status;

    /* (1) Bit-wise AND all active, non-zero qmaps into gormap */
    for (i = 0; i < PSY_MQMAP; i++)
    {
	if (qmap[i].q_usemap)
	{
	    if (checkany && BTnext((i4) -1, (char *) qmap[i].q_map, size) < 0)
	    {
		qmap[i].q_usemap = FALSE;
		*outtree = NULL;
		return(E_DB_OK);
	    }
	    else if (inited)
	    {
		BTand(size, (char *) qmap[i].q_map, (char *) gormap);
	    }
	    else
	    {
		MEcopy((PTR) qmap[i].q_map, sizeof(PQMAP), (PTR) gormap);
		inited = TRUE;
	    }
	}
    }

    /* (2) If gormap is non-zero, disjoin pqual for each bit */
    if (inited && BTnext((i4) -1, (char *) gormap, size) >= 0)
    {
	status = gorform(sess_cb, size, gormap, pqual, &qual, dup_rb);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* (3) Mask out gormap from all active qmaps */
	for (i = 0; i < PSY_MQMAP; i++)
	{
	    if (qmap[i].q_usemap)
		btmask((i4)size, (char *) gormap, (char *) qmap[i].q_map);
	}

	/*
	** (4) Recursively generate remaining qualification and if non-null
	** DISJOIN to qual
	*/
	status = applypq(sess_cb, size, qmap, pqual, TRUE, &q, dup_rb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	if (q != (PST_QNODE *) NULL)
	{
	    status = pst_node(sess_cb, dup_rb->pss_mstream, qual, q, PST_OR, 
		(PTR) NULL, sizeof(PST_OP_NODE), DB_NODT, (i2) 0, (i4) 0,
		(DB_ANYTYPE *) NULL, &qual, dup_rb->pss_err_blk, (i4) 0);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
    }

    /*
    ** (5) Pick a qmap to eliminate (if there are no more, we must be done)
    */
    else if ((z = pickqmap(size, qmap)) >= 0)
    {
	/* (6) Disjoin all pquals in selected qmap and mark qmap inactive */
	status = gorform(sess_cb, size, qmap[z].q_map, pqual, &qual, dup_rb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	qmap[z].q_usemap = FALSE;

	/*
	** (7) Recursively generate remaining qualification, and if non-null
	** CONJOIN to Z
	*/
	status = applypq(sess_cb, size, qmap, pqual, TRUE, &q, dup_rb);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	if (q != NULL)
	{
	    status = pst_node(sess_cb, dup_rb->pss_mstream, qual, q, PST_AND, 
		(PTR) NULL, sizeof(PST_OP_NODE), DB_NODT, (i2) 0, (i4) 0,
		(DB_ANYTYPE *) NULL, &qual, dup_rb->pss_err_blk, (i4) 0);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
    }

    /* (8) Whew! */
    *outtree = qual;
    return(E_DB_OK);
}

/*{
** Name: gorform	- Form disjunction of all pquals in map
**
** Description:
**      This function forms the disjunction of all the qualifications in
**	a given map, by linking them together with OR nodes.
**
** Inputs:
**	size				Size of bit map
**	map				The bit map
**	pqual				Array of qualifications
**	qual				Place to put pointer to final qual
**	dup_rb				ptr to request block used by pst_treedup
**					conveniently, it contains memory stream
**					and error block ptrs (among other
**					things). Other fields have been preset
**					to be passed, if necesasary, to
**					pst_treedup().
**	    pss_op_mask			set to 0
**	    pss_mstream			memory stream
**	    pss_err_blk			error block.
**
** Outputs:
**      qual                            Filled in with pointer to final qual
**	dup_rb->
**	    pss_err_blk			Filled in if an error happened
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
**      07-jul-86 (jeff)
**	    Adapted from gorform() in 4.0
**	14-sep-89 (andre)
**	    changed interface (slightly.)  ptrs to memory stream and error block
**	    are passed inside the request block to be passed to pst_treedup().
*/
static DB_STATUS
gorform(
	PSS_SESBLK	    *sess_cb,
	i4		   size,
	PQMAP		   map,
	PSY_PQUAL	   pqual[],
	PST_QNODE	   **qual,
	PSS_DUPRB	    *dup_rb)
{
    i4                  i;
    PST_QNODE		*q;
    DB_STATUS		status;

    i  = BTnext(-1, (char *) map, size);
    /*
    ** The first time a qual is used, don't make a copy.
    ** Every subsequent time, do make a copy.
    */
    if (pqual[i].p_used)
    {
	dup_rb->pss_tree = pqual[i].p_qual;
	dup_rb->pss_dup  = qual;
        status = pst_treedup(sess_cb, dup_rb);
	
        if (DB_FAILURE_MACRO(status))
	    return (status);
    }
    else
    {
        pqual[i].p_used = TRUE;
        *qual = pqual[i].p_qual;
    }

    while ((i = BTnext(i, (char *) map, size)) >= 0)
    {
	/*
	** The first time a qual is used, don't make a copy.
	** Every subsequent time, do make a copy.
	*/
	if (pqual[i].p_used)
	{
	    dup_rb->pss_tree = pqual[i].p_qual;
	    dup_rb->pss_dup  = &q;
	    status = pst_treedup(sess_cb, dup_rb);

	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	else
	{
	    pqual[i].p_used = TRUE;
	    q = pqual[i].p_qual;
	}

	/* Now disjoin the new qualification with the existing set */
	status = pst_node(sess_cb, dup_rb->pss_mstream, *qual, q, PST_OR,
	    (PTR) NULL, sizeof(PST_OP_NODE), DB_NODT, (i2) 0, (i4) 0,
	    (DB_ANYTYPE *) NULL, qual, dup_rb->pss_err_blk, (i4) 0);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }

    return (E_DB_OK);
}

/*{
** Name: pickqmap   - Select a qmap for elimination from the pqual bit matrix
**
** Description:
**	All active qmaps are guaranteed to be non-zero at this point.
**	Pick one such that the gormap of the remaining qmaps will have the
**	most bits set.
**
** Inputs:
**      size                            Number of qualification maps
**	qmap				The qualification maps
**
** Outputs:
**	Returns:
**	    The position of the qmap to eliminate
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-jul-86 (jeff)
**	    Adapted from pickqmap() in 4.0
*/
static i4
pickqmap(
	i4	    size,
	PSY_QMAP    qmap[])
{
    i4		score;
    i4		maxscore = -1;
    i4		whichqmap = -1;
    i4		i, j;
    PQMAP	gormap;
    bool	inited = FALSE;

    for (i = 0; i < PSY_MQMAP; i++)
    {
	if ( !qmap[i].q_usemap)
	    continue;

	for (j = 0; j < PSY_MQMAP; j++)
	{
	    if (j == i || !qmap[j].q_usemap)
		continue;

	    if (inited)
	    {
		BTand(size, (char *) qmap[i].q_map, (char *) gormap);
	    }
	    else
	    {
		MEcopy((PTR) qmap[i].q_map, sizeof(PQMAP), (PTR) gormap);
		inited = TRUE;
	    }
	}

	score = inited ? BTcount((char *) gormap, size): 0;
	if (score > maxscore)
	{
	    maxscore = score;
	    whichqmap = i;
	}
    }

    return(whichqmap);
}

/*{
** Name: makedset	- Make domain reference sets
**
** Description:
**	This routine creates some sets which reflect the usage of
**	domains for a particular variable.
**
**	The interesting nodes are 'case' labels in the large
**	switch statement which comprises most of the code.  To
**	describe briefly:
**
**	VAR nodes are easy: if they are for the current variable,
**		set the bit corresponding to the domain in the
**		'retrieval' set.  They can have no descendents,
**		so just return.
**	RESDOM nodes are also easy: they can be handled the same,
**		but the bit is set in the 'update' set instead.
**	AGHEAD nodes signal the beginning of an aggregate or
**		aggregate function.  In this case, we scan the
**		qualification first (noting that RESDOM and VAR
**		nodes are processed as 'qualification' sets
**		instead of 'retrieval' or 'update' sets).  Then,
**		if the aggregate has a WHERE clause or a BY list,
**		we treat it as a retrieve; otherwise, we call our-
**		selves recursively treating VAR nodes as 'aggregate'
**		types rather than 'retrieve' types.
**	BYHEAD nodes signal the beginning of a BY list.  The left
**		subtree (the actual BY-list) is processed with
**		RESDOM nodes ignored (since they are pseudo-domains
**		anyhow) and VAR nodes mapped into the 'qualification'
**		set.  Then we check the right subtree (which better
**		begin with an AOP node!) and continue processing.
**	AOP nodes must have a null left subtree, so we just drop
**		to the right subtree and iterate.  Notice that we
**		do NOT map VAR nodes into the 'aggregate' set for
**		this node, since this has already been done by the
**		AGHEAD node; also, this aggregate might be counted
**		as a retrieve operation instead of an aggregate
**		operation (as far as the protection system is con-
**		cerned) -- this has been handled by the AGHEAD
**		node.
**	All other nodes are processed recursively along both edges.
**
** Inputs:
**      var                             The range variable we are currently
**					interested in.
**	tree				The root of the tree to scan.  Notice
**					that this will in general only be one
**					half of the tree -- makedset will be
**					called once for the target list and 
**					once for the qualification, with
**					different sets for the following
**					parameters
**	query_mode			real query mode
**	qmode				Query mode we are getting permission
**					for
**	retrtoall			TRUE means "retrieve to all" permission
**					was given
**	sess_cb				Pointer to session control block
**	rngtab				Pointer to range table
**	uset				Current update set
**	rset				Current retrieve set
**	aset				Current aggregate set
**	qset				Current qualification set
**	rdf_cb				to read protection tuples
**	rdf_tree_cb			to read tree tuples
**	dup_rb				ptr to a request block for use by
**					pst_treedup().  It is passed here so
**					that it can be used in invoking dopro()
**					+ it contains ptr to memory stream and
**					error block.
**	
**
** Outputs:
**	uset				Adjusted to be the set of all domains
**					updated
**	rset				Adjusted to be the set of all domains
**					retrieved implicitly, that is, on the
**					right-hand-side of an assignment
**					operator
**	aset				Adjusted to be the set of all domains
**					aggregated.  Notice that this set is
**					not adjusted explicitly, but rather is
**					passed to recursive incarnations of this
**					routine as 'rset'
**	qset				Adjusted to be the set of domains
**					retrieved implicitly in a qualification.
**					Like 'aset', that is, passed as 'rset'
**					to recursive incarnations.
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
**      07-jul-86 (jeff)
**          Adapted from makedset() in 4.0
**	22-jun-89 (andre)
**	    makedset() will accept a true query mode in addition to the
**	    effective query mode that was being passed up till now.  We need it
**	    here to pass it to dopro().
**	10-jul-89 (andre)
**	    accept mask as a parameter to be passed to dopro()
**	14-sep-89 (andre)
**	    accept ptr to a request block for pst_treedup().  dup_rb also
**	    contains ptr to the memory stream and error block.
**	30-nov-90 (andre)
**	    mask is no longer used in dorpo(), so it is no longer passed here
**	10-mar-93 (andre)
**	    when building a map of attributes in the FOR UPDATE clause of
**	    DECLARE CURSOR, it is incorrect to use resdom number since it is
**	    always set to 1.  Instead, we must use the attribute number
**	    contained in the VAR node attached to the RESDOM node.  If a node
**	    other than VAR node is atached to the RESDOM node, we will simply
**	    skip the node, since that column will most definitely not be
**	    updatable
*/
static DB_STATUS
makedset(
	PSS_RNGTAB	    *var,
	PST_QNODE	    *tree,
	i4		    query_mode,
	i4		    qmode,
	i4		    retrtoall,
	PSS_SESBLK	    *sess_cb,
	PSS_USRRANGE	    *rngtab,
	i4		    *uset,
	i4		    *rset,
	i4		    *aset,
	i4		    *qset,
	RDF_CB		    *rdf_cb,
	RDF_CB		    *rdf_tree_cb,
	PSS_DUPRB	    *dup_rb)
{
    register i4	vn;
    register PST_QNODE  *t;
    PSY_ATTMAP		byset;
    PST_VRMAP		varmap;
    DB_STATUS		status;
    i4		err_code;
#ifdef    xDEBUG
    i4		val1;
    i4		val2;
#endif

    t = tree;
    vn = var->pss_rgno;

#ifdef    xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
    {
    	TRdisplay("->makedset\n");
	pr_set(uset, "uset");
	pr_set(rset, "rset");
	pr_set(aset, "aset");
	pr_set(qset, "qset");
    }
#endif

    while (t != NULL)
    {
	switch (t->pst_sym.pst_type)
	{
	case PST_VAR:
	    if (t->pst_sym.pst_value.pst_s_var.pst_vno == vn)
		BTset((i4) t->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
		    (char *) rset);
	    break;

	case PST_AGHEAD:
	    (VOID)psy_varset(t, &varmap);
	    if (!BTtest(vn, (char *) &varmap))
		break;

	    /* do protection on qualification */
	    status = dopro(var, t, query_mode, PSQ_RETRIEVE, &byset, retrtoall,
	        sess_cb, rngtab, rdf_cb, rdf_tree_cb, dup_rb,
		(PSS_RNGTAB *) NULL, (PSY_ATTMAP *) NULL);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    /* merge by-list set into qualification set */
	    BTor((i4) BITS_IN(PSY_ATTMAP), (char *) byset.map, (char *) qset);

	    break;

	case PST_BYHEAD:
	case PST_AOP:
	    /* These nodes are illegal here */
	    (VOID) psf_error(E_PS0D12_BYHEAD_OR_AOP, 0L, PSF_INTERR, &err_code,
		dup_rb->pss_err_blk, 0);
	    return (E_DB_SEVERE);

	case PST_RESDOM:
	    if (t->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 0)
	    {
		/* tid -- ignore right subtree (and this node) */
		t = t->pst_left;
		continue;
	    }

	    if (uset != (i4 *) NULL)
	    {
		/*
		** On a "define cursor", only include the updateable columns
		** in the update set.  On all other update commands, include
		** every column.
		*/
		if (qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
		{
		    i4	    att_no;
		    
		    /*
		    ** instead of using the RESDOM number, we need to use the
		    ** attribute number contained in the VAR node (if any)
		    ** attached to the RESDOM node.  This is so because resdom
		    ** numbers corresponding to column in the FOR UPDATE list
		    ** are set to 1 and are never reset so they do NOT represent
		    ** the number of the attribute to be updated.
		    ** Care should be exercised to avoid adding the TID
		    ** attribute to the list of those on which the user must
		    ** possess UPDATE, though
		    */
		    if (   t->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt
			&& t->pst_right->pst_sym.pst_type == PST_VAR
			&& (att_no =
			        t->pst_right->pst_sym.pst_value.pst_s_var.
				    pst_atno.db_att_id) != 0
		       )
		    {
			BTset(att_no, (char *) uset);
		    }
		}
		else
		{
		    BTset((i4) t->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
			(char *) uset);
		}
	    }
	    /* explicit fall-through to "default" case */
	default:
	    /* handle left subtree (recursively) */
	    status = makedset(var, t->pst_left, query_mode, qmode, retrtoall,
	        sess_cb, rngtab, uset, rset, aset, qset, rdf_cb, rdf_tree_cb,
		dup_rb);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    /* handle right subtree (iteratively) */
	    t = t->pst_right;
	    continue;

	case PST_SUBSEL:
	    /* do protection on qualification */
	    status = dopro(var, t, query_mode, PSQ_RETRIEVE,
		(PSY_ATTMAP *) NULL, retrtoall, sess_cb, rngtab, rdf_cb,
		rdf_tree_cb, dup_rb, (PSS_RNGTAB *) NULL, (PSY_ATTMAP *) NULL);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    break;

	}

	break;
    }

#ifdef    xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
    {
    	TRdisplay("makedset->\n");
	pr_set(uset, "uset");
	pr_set(rset, "rset");
	pr_set(aset, "aset");
	pr_set(qset, "qset");
    }
#endif

    return (E_DB_OK);
}

/*{
** Name: proappl	- Check for protection tuple applicable
**
** Description:
**      A given protection catalog tuple is checked in a query-independent
**	way for applicability.
**
**	This routine checks such environmental constraints as the user,
**	the terminal, and the time of day.  The code is fairly straightforward.
**
**	One note: The user and terminal codes contained in the iiprotect
**	catalog are blank to mean "any value" of the corresponding field.
**
** Inputs:
**      protup                          Pointer to protection tuple
**	sess_cb				PSF session CB
**
** Outputs:
**	result				Filled in with TRUE if tuple applies,
**					FALSE if not.
**	err_blk				Filled in if an error occurred
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
**      07-jul-86 (jeff)
**          written
**	11-nov-88 (stec)
**	    Change defn of time zone from i2 to nat.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Check for permits to session's group and application identifier
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Use DBGR_USER, DBGR_GROUP, and DBGR_APLID constants for dbp_gtype.
**	21-feb-91 (andre)
**	    user, group, and role identifiers do not need to be passed
**	    separately since they can be found in sess_cb
**  	24-sep-92 (stevet)
**  	    Implemented the new timezone adjustment method by
**  	    replacing TMzone() with TMtz_search() routine.
*/
DB_STATUS
proappl(
	register  DB_PROTECTION *protup,
	bool			*result,
	PSS_SESBLK		*sess_cb,
	DB_ERROR		*err_blk)
{
    i4                  secs_since_jan_1_1970;
    register i4	mtime;
    register i4	wday;
    i4			seconds;
    DB_TERM_NAME	termname;
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[2];	/* For getting time, terminal */
    DB_STATUS		status;
    i4		err_code;
    ADF_CB		*adf_scb = (ADF_CB*)sess_cb->pss_adfcb;
#ifdef    xDEBUG
    i4			time_zone;
    i4		val1;
    i4		val2;
#endif

#define             PSY_SPMIN           60  /* Seconds per minute */
#define		    PSY_MPHR		60  /* Minutes per hour */
#define		    PSY_SPHR		(PSY_SPMIN * PSY_MPHR) /* Secs per hr */
#define		    PSY_HPDAY		24  /* Hours per day */
#define		    PSY_SPDAY		(PSY_SPHR * PSY_HPDAY) /* Secs per dy */
#define		    PSY_DYPWK		7   /* Days per week */
#define		    PSY_JAN_1_1970	4   /* Thursday */

    /*
    ** check if the permit applies to any (user, group, or role) identifiers for
    ** this session, or if the permit has been granted to PUBLIC
    */
    status = psy_grantee_ok(sess_cb, protup, result, err_blk);
    if (DB_FAILURE_MACRO(status) || !*result)
    {
	/*
	** if bad grantee type was discovered, or if the tuple does not apply,
	** we are done
	*/
	return(status);
    }

    /* If permit is granted through SQL, no need to check this */
    if (protup->dbp_flags & DBP_GRANT_PERM) 
	return(status);

    /* Get the terminal name and time of day */
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_PSF_ID;
    scf_cb.scf_session = sess_cb->pss_sessid;
    scf_cb.scf_len_union.scf_ilength = 2;  /* Requesting 2 items */
    /* expect lint error message */
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) sci_list;
    sci_list[0].sci_code = SCI_NOW; /* Get time of day */
    sci_list[0].sci_length = sizeof(secs_since_jan_1_1970);
    sci_list[0].sci_aresult = (PTR) &secs_since_jan_1_1970;
    sci_list[0].sci_rlength = NULL; /* Not interested in result length */
    sci_list[1].sci_code = SCI_UTTY;	/* Get user terminal id */
    sci_list[1].sci_length = sizeof(termname);
    sci_list[1].sci_aresult = (PTR) &termname;
    sci_list[1].sci_rlength = NULL; /* Not interested in result length */
    status = scf_call(SCU_INFORMATION, &scf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0D13_SCU_INFO_ERR, scf_cb.scf_error.err_code,
	    PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    /* Check for correct terminal.  All blanks means any terminal OK */
    if (STskipblank((char *) &protup->dbp_term, sizeof(protup->dbp_term))
	&&
	MEcmp((PTR) &protup->dbp_term, (PTR) &termname, sizeof(termname))
       )
    {
	*result = FALSE;
	return (E_DB_OK);
    }

    /* Check for correct time of day and week by adjusting the timezone */
    seconds = secs_since_jan_1_1970 + 
	      TMtz_search(adf_scb->adf_tzcb, TM_TIMETYPE_GMT,
    	    	    	  secs_since_jan_1_1970);
    /*
    ** Calculate today's time in minutes (minutes + hours * 60).
    **
    ** mtime is the number of minutes in the current day.
    **
    ** mtime = (seconds in the current day) / (seconds/min)
    */
    mtime = ((seconds % PSY_SPDAY) / PSY_SPMIN);

    /*
    ** Calculate day of week.
    **
    ** wday is the number of days in the current week (0 - 6)
    ** where Sunday is 0 and Saturday is 6.
    */
    wday = ((seconds / PSY_SPDAY) + PSY_JAN_1_1970) % PSY_DYPWK;

#ifdef    xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
    {
	time_zone = TMtz_search(adf_scb->adf_tzcb, TM_TIMETYPE_GMT,
    	    	    	  secs_since_jan_1_1970);
	TRdisplay("In proappl:\n");
     TRdisplay("secs_since_jan_1_1970 =%%x %x; secs_since_jan_1_1970=%%d %d;\n",
	    secs_since_jan_1_1970,  secs_since_jan_1_1970);
	TRdisplay("timezone     =%%d %d;seconds      =%%x %x;\n",
	    time_zone, seconds);
	TRdisplay("mtime        =%%x %x;mtime        =%%d %d;\n",
	    mtime, mtime);
	TRdisplay("wday         =%%x %x;wday         =%%d %d;\n",
	    wday, wday);
	TRdisplay("protup->dbp_pdbgn=%%x %x;protup->dbp_pdbgn=%%d %d;\n",
	    protup->dbp_pdbgn, protup->dbp_pdbgn);
	TRdisplay("protup->dbp_pdend =%%x %x;protup->dbp_pdend =%%d %d;\n",
	    protup->dbp_pdend, protup->dbp_pdend);
	TRdisplay("protup->dbp_pwbgn =%%x %x;protup->dbp_pwbgn =%%d %d;\n",
	    protup->dbp_pwbgn, protup->dbp_pwbgn);
	TRdisplay("protup->dbp_pwend =%%x %x;protup->dbp_pwend =%%d %d;\n",
	    protup->dbp_pwend, protup->dbp_pwend);
    }
#endif

    /* Check tuple versus time of day */
    if (protup->dbp_pdbgn > mtime || protup->dbp_pdend < mtime)
    {
	*result = FALSE;
	return (E_DB_OK);
    }

    /* Check tuple versus day of week */
    if (protup->dbp_pwbgn > wday || protup->dbp_pwend < wday)
    {
	*result = FALSE;
	return (E_DB_OK);
    }

    /* Everything checks out */
    *result = TRUE;
    return (E_DB_OK);
}

/*{
** Name: prochk	- Query-dependent protection tuple check
**
** Description:
**	This routine does the query-dependent part of checking
**	the validity of a protection tuple.  Unlike proappl,
**	which looked at aspects of the environment but not the
**	query being run, this routine assumes that the environ-
**	ment is ok, and checks that if it applies to this tuple.
**
**	Two things are checked.  The first is if this tuple applies
**	to the operation in question (passed as privs).  The
**	second is if the set of domains in the tuple contains the
**	set of domains in the query (applies only when dealing with table
**	privileges, i.e. SELECT, INSERT, DELETE, UPDATE.)  If either of these
**	fail, the return is zero.  Otherwise the return is the operation
**	bit.  Note that privs contained DB_GRANT_OPTION bit, we will ensure that
**	the tuple specifies WGO, but will only return the actual operation
**
**	If checking for table privileges, the domain set is checked for all
**	zero.  If so, no domains have been referenced for this 
**	operation at all, and we return zero.  In other words, this
**	tuple might apply to this operation, but since we don't
**	use the operation anyhow we will ignore it.  It is important
**	to handle things in this way so that the qualification for
**	this tuple doesn't get appended if the variable is not
**	used in a particular context.
**
** Inputs:
**      privs				Map representing required privilege(s).
**					More than one operation may be checked
**					for (may happen with GRANT).
**					DB_GRANT_OPTION bit, if set, indicates
**					that the privilege(s) must be grantable.
**
**	domset				The set of domains actually referenced
**					in this query for the operation
**					described by "privs".  When checking for
**					privileges other than SELECT, INSERT,
**					DELETE, or UPDATE, this will be set to
**					NULL
**					
**	protup				The tuple in question.
**
** Outputs:
**	Returns:
**	    The operation (if any) to which this tuple applies.
**	Exceptions:
**	    none
**
**	domset				For REPLACE (update) or REFERENCES
**					priv, domset is updated to clear the
**					bits for all columns that this tuple
**					permits.  QUEL does not support column
**					specific update permits, so for QUEL
**					all columns in domset are cleared.
**
** Side Effects:
**	    none
**
** History:
**      07-jul-86 (jeff)
**          Adapted from prochk() in 4.0
**	21-sep-90 (andre)
**	    domain set passed by the caller has a size =  sizeof(PSY_ATTMAP)
**	    which is the minimum number of i2's required to represent
**	    DB_MAX_COLS+1 columns.  Domain set as found in IIPROTECT tuple
**	    consists of a minimum number of i4's required to represent
**	    DB_MAX_COLS+1 columns.  As a result the size of the former is
**	    guaranteed to be <= the size of the latter, so it makes sense to use
**	    the size of the former when determining if we are done scanning the
**	    domain maps.
**	26-jul-91 (andre)
**	    prochk may be given a mask specifying more than one operation;
**	    the mask will be of type i4, to match dbp_popset.  If the
**	    DB_GRANT_OPTION bit is set, we need to ascertain that the permit
**	    tuple specified GRANT OPTION along with at least one of desired
**	    operations.
**	06-aug-91 (andre)
**	    PSY_ATTMAP now consists of DB_COL_WORDS i4's so the (21-sep-90)
**	    comment about difference in sizes is no longer relevant
**	29-jan-92 (andre)
**	    when  processing SQL queries, UPDATE(c1), ..., UPDATE(cn) is
**	    equivalent to UPDATE(c1, ..., cn).  Therefore, we will no longer
**	    insist that the tuple's attribute map be a superset of domset;
**	    instead we will replace domset with a set difference of domset and
**	    the tuple's attribute map.  If domset becomes empty, we will notify
**	    caller that the user posesses UPDATE; otherwise domset will
**	    represent attributes on which UPDATE privilege must still be
**	    satisfied
**	10-feb-92 (andre)
**	    adapt prochk() for checking permits for objects other than tables
**	    (e.g. dbevents).  Mainly this involves skipping all attribute map
**	    checks when dealing with non-table objects
**	09-jun-92 (andre)
**	    DB_TEST and DB_AGGREGATE may have attribute maps associated with
**	    them
**	10-dec-92 (andre)
**	    as a part of adding support for REFERENCES privilege, we will be
**	    handling permits specifying REFERENCES in the same manner as permits
**	    specifying UPDATE, i.e. when  processing GRANT, REFERENCES(c1), ...,
**	    REFERENCES(cn) is equivalent to REFERENCES(c1, ..., cn).  Therefore,
**	    we will not insist that the tuple's attribute map be a superset of
**	    domset; instead we will replace domset with a set difference of
**	    domset and the tuple's attribute map.  If domset becomes empty, we
**	    will notify caller that the user posesses REFERENCES on the desired
**	    set of attributes; otherwise domset will represent attributes on
**	    which REFERENCES privilege must still be satisfied
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	4-Aug-2004 (schka24)
**	    Update the REPLACE or REFERENCES bitmap for quel as well as sql.
**	    This change was to have been made with my 20-Jul changes to
**	    check-permits, but I forgot.  Duh.
*/
i4
prochk(
	i4			privs,
	i4			*domset,
	register DB_PROTECTION	*protup,
	PSS_SESBLK		*sess_cb)
{
    register i4         *d;
    register i4	i;
    register i4		*dset;
    i4			valid_attr_map;
#ifdef	  xDEBUG
    i4		val1;
    i4		val2;
#endif

    d = domset;
    dset = protup->dbp_domset;

    valid_attr_map = (privs &
	(DB_RETRIEVE | DB_REPLACE | DB_DELETE | DB_APPEND | DB_TEST |
	    DB_AGGREGATE | DB_REFERENCES)) != 0;

#ifdef    xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
    {
	TRdisplay("->prochk, privs=0x%x, dbp_popset=0x%p\n", privs, dset);
	if (valid_attr_map)
	{
	    pr_set(d, "domset");
	    pr_set(dset, "prodomset");
	}
    }
#endif

    /*
    ** see if this tuple applies to this operation:
    **	- operation priv map and tuple priv map must have a "real" privilege
    **	  (DB_GRANT_OPTION is not a "real" privilege) in common, and
    **	- if operation priv map has DB_GRANT_OPTION set, then so must the tuple
    **	  priv map
    */
    if (   !(privs & protup->dbp_popset & ~((i4) DB_GRANT_OPTION))
        || (privs & ~protup->dbp_popset & (i4) DB_GRANT_OPTION)
       )
    {
#ifdef    xDEBUG
	if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	    TRdisplay("prochk-> no op\n");
#endif
	return (0);
    }

    /* check for null domain set, if so return zero */
    if (valid_attr_map && BTnext((i4) -1, (char *) d, DB_MAX_COLS + 1) == -1)
    {
#ifdef    xDEBUG
	if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	    TRdisplay("prochk-> null set\n");
#endif
	return (0);
    }

    /*
    ** recall that we will return a privilege bit, if the permit satisfies some
    ** privilege, but the callers expect that DB_GRANT_OPTION bit will not be
    ** set
    ** in the event that the tuple does satisfy some privilege, compute that
    ** privilege now
    */
    privs &= (protup->dbp_popset & ~((i4) DB_GRANT_OPTION));

    /*
    ** we will check attribute maps only when they contain meaningful
    ** information, i.e. we are dealing with a privilege on a table or a view
    */
    if (valid_attr_map)
    {
	/*
	** For REPLACE or REFERENCES, indicate to the caller which columns
	** are permitted by this protup, by clearing the corresponding
	** bits in domset.  If any bits are still lit, some column of interest
	** to the caller is not permitted yet, so do not return the priv.
	** This used to be done only for SQL, but now I do it for QUEL also,
	** to support fixes in psy_check_permit.
	*/
	if (privs == DB_REPLACE || privs == DB_REFERENCES)
	{
	    /* Check if domains are a subset, and indicate which columns
	    ** are permitted so far.
	    */
	    for (i = 0; i < DB_COL_WORDS; i++)
	    {
		if ((d[i] &= ~dset[i]) != 0)
		    privs = 0;	/* Still some unpermitted attrs */
	    }
	}
	else
	{
	    /* check if domains are a subset */
	    for (i = 0; i < DB_COL_WORDS; i++)
	    {
		if ((d[i] & ~dset[i]) != 0)
		{
		    /* failure */
#ifdef    xDEBUG
		    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
			TRdisplay("prochk-> not subset\n");
#endif
		    return (0);
		}
	    }
	}
    }
    /* This is an "interesting" tuple */
#ifdef    xDEBUG
    if (privs && ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	TRdisplay("prochk-> %d\n", privs);
#endif

    return (privs);
}

#ifdef    xDEBUG
/*{
** Name: pr_set	- Print a set for debugging
**
** Description:
**      This function prints a 128-bit set for debugging.
**	Make that DB_MAX_COLS+1, not 128
**
** Inputs:
**      xset                            The set to print
**	labl				A label to print before the set
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-jul-86 (jeff)
**          Adapted from 4.0 pr_set()
**	18-sep-90 (andre)
**	    enable the function to print the entire attribute map, not just the
**	    first 128 bits.
*/
static VOID
pr_set(
	i4     *xset,
	char   *labl)
{
    register i4         *x;
    register i4	i;
    char		str[25];

    TRdisplay("\t%s: ", labl);

    if (xset == (i4 *) NULL)
    {
	TRdisplay("<NULL>\n");
    }
    else
    {
	for (i = DB_COL_WORDS, x = xset + DB_COL_WORDS - 1; i > 0; i--, x--)
	    TRdisplay("0x%x/", *x);

	TRdisplay(" <> ");

	for (i = 0, x = xset; i < DB_COL_WORDS; i++, x++)
	{
	    CVla(*x, str);
	    TRdisplay("/%s", str);
	}

	TRdisplay("\n");
    }
}

/*{
** Name: pr_qmap	- Formatted printout of qmaps
**
** Description:
**      This function formats and prints a qualification map
**
** Inputs:
**      size                            Number of elements in the qmap set
**      qmap                            The array of qmaps
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-jul-86 (jeff)
**          Adapted from 4.0 pr_qmap()
*/
static VOID
pr_qmap(
	i4	    size,
	PSY_QMAP    qmap[])
{
    register i4     i;
    char	    buf[PSY_MPQUAL + (PSY_MPQUAL - 1) / 4 + 1];

    for (i = 0; i < PSY_MQMAP; i++)
    {
	TRdisplay(" [%c] ", opstring[i]);
	if (!qmap[i].q_usemap)
	{
	    TRdisplay("<inactive>");
	}
	else if (qmap[i].q_unqual)
	{
	    TRdisplay("<unqualified>");
	}
	else
	{
	    btdumpx((char *) qmap[i].q_map, size, buf);
	    TRdisplay("%s", buf);
	}

	TRdisplay("\n");
    }
}
# endif

/*{
** Name: btmask	- Clear bits in result according to mask
**
** Description:
**      Effectively this will do:
**
**	    result &= ~mask;
** 
**	for an arbitrary sized bit vector.
**
** Inputs:
**      size                            The number of bits in the vectors
**	mask				Vector of bits to mask out
**	result				Vector in which to do masking
**
** Outputs:
**      result                          Bits that are on in mask are masked out
**	Returns:
**	    NONE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-jul-86 (jeff)
**          Adapted from 4.0 btmask()
*/
static VOID
btmask(
	i4     size,
	char	*mask,
	char	*result)
{
    register i4    i;

    for (i = (size - 1) / BITSPERBYTE + 1; i--; )
	*result++ &= ~(*mask++);
}

# ifdef xDEBUG
/*{
** Name: btdumpx	- Formatted printout of bits in a bit vector
**
** Description:
**      This function prints out the bits in a bit vector in human-readable
**	form.
**
** Inputs:
**      vec                             The bit vector to print
**	size				Number of bytes in the vector
**	buf				Place to put formatted result
**
** Outputs:
**      buf                             Filled with formatted bit vector
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-jul-86 (jeff)
**          Adapted from 4.0 btdumpx()
*/
static VOID
btdumpx(
	char               *vec,
	i4		   size,
	char		   *buf)
{
    i4                  i;
    i4			nchars = size + (size - 1) / 4 + 1;
    char		*b = buf;

    while (b < &buf[nchars])
    {
	/* Fill up buf with sets of 4 zeros interspersed with blanks */
	if ((b - buf) % 5 == 4)
	    *b++ = ' ';
	else
	    *b++ = '0';
    }

    /* Null terminate buf and make b point to last printing character in it */
    *(--b) = '\0';
    --b;

    /* Put a '1' in every position corresponding to a 1 bit in vec */
    for (i = -1; (i = BTnext(i, vec, size)) >= 0; )
	b[(-i) - i / 4] = '1';
}

# endif

/*
** Name: psy_do_alarm_fail - Do the C2 security alarm handling.
**
** Description:
**	This routine is responsible for triggering security alarms for
**	failing queries. It scans through the iiprotect catalog for
**	security alarm entries which match the user, table and query mode
**	criteria. For each alarm entry that is found a call is made to 
**	psy_secaudit()
**
** Inputs:
**	sess_cb 			Session control block pointer.
**      rngvar                          Pointer to range variable.
**      qmode                           Query mode of range variable, i.e. how
**					it's being used
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
**	03-apr-93 (markg)
**	    Created from the old doaudit() routine.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	16-nov-93 (stephenb)
**	    Parenthesise test in "if (!protup->dbp_popset & DB_ALMFAILURE)"
**	    because not (!) takes precedence over logical and (&), and we were
**	    therefore testing the wrong thing.
**	22-nov-93 (robf)
**	    Make function public.
**          Check for any of user/group/role/public for alarm owners,
**          Rewrite to use security alarms rather than protect code
**	15-feb-94 (robf)
**          STAR doesn't have alarms currently, so don't look them up.
**	02-Jun-1997 (shero03)
**	    Update the pss_rdrinfo pointer after calling rdf
*/
DB_STATUS
psy_do_alarm_fail(
	PSS_SESBLK	   *sess_cb,
	PSS_RNGTAB	   *rngvar,
	i4		   qmode,
	DB_ERROR	   *err_blk)
{
    DB_STATUS		local_status;
    DB_ERROR		local_error;
    RDF_CB		rdfcb;
    RDF_CB		*rdf_cb = &rdfcb;	/* For permit tuples */
    DB_STATUS           status;
    QEF_DATA	    *qp;
    DB_SECALARM     *almtup;
    RDR_RB	    *rdf_rb = &rdf_cb->rdf_rb;
    DB_STATUS	     stat;
    i4		     tupcount;
    i4               mode;
    bool	     applies=FALSE;
    char	     *grantee;
    char	     *access_str;
    i4	     err_code;

    /*
    ** If not reporting messages then quietly return.
    ** This prevents double events/alarms being sent due to PSF retry 
    ** logic.
    */
    if( ~sess_cb->pss_retry & PSS_REPORT_MSGS)
	return E_DB_OK;

    /*
    ** STAR currently doesn't have security alarms, so don't
    ** try to look them up
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return E_DB_OK;


    /* remember the access mode for audit records */
    switch (qmode)
    {
        case PSQ_DEFCURS:
        case PSQ_REPDYN:
        case PSQ_RETINTO:
        case PSQ_DGTT_AS_SELECT:
        case PSQ_RETRIEVE:
            mode = SXF_A_SELECT;
	    access_str="select";
            break;

        case PSQ_APPEND:
            mode = SXF_A_INSERT;
	    access_str="insert";
            break;

        case PSQ_DELETE:
            mode = SXF_A_DELETE;
	    access_str="delete";
            break;

        case PSQ_REPLACE:
            mode = SXF_A_UPDATE;
	    access_str="update";
            break;
	default:
	    mode = SXF_A_SELECT;
	    access_str="access";
    }
 
    /*	Initialize RDF request block. */

    pst_rdfcb_init(rdf_cb, sess_cb);


    /* Get permit tuples from RDF that apply to this table */

    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_rb->rdr_tabid);
    rdf_rb->rdr_types_mask     = RDR_SECALM;
    rdf_cb->rdf_info_blk       = rngvar->pss_rdrinfo;
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
			goto do_alarm_wrapup;
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
	    /* Set almup pointing to current permit tuple */

	    almtup = (DB_SECALARM*) qp->dt_data;

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
			if (~almtup->dba_popset & DB_DELETE)
			    continue;
			break;

		case PSQ_REPLACE:
			if (~almtup->dba_popset & DB_REPLACE)
			    continue;
			break;
	    }

	    /* Skip this alarm if FAILURE not specified */
	    if (!(almtup->dba_popset & DB_ALMFAILURE))
		continue;

	    /* Skip this alarm if it does not apply to this 
	    ** user/group/role
	    */
	    applies=FALSE;
    	    switch (almtup->dba_subjtype)
    	    {
		    case DBGR_PUBLIC:
		    {
	    	    	applies = TRUE;
	    	    	break;
		    }
		    case DBGR_USER:
		    {
	    	   	grantee = (char *) &sess_cb->pss_user;
			if (!MEcmp((PTR) &almtup->dba_subjname, grantee, 
					sizeof(almtup->dba_subjname)))
				applies=TRUE;
	    	    	break;
		    }
		    case DBGR_GROUP:	   /* Check group id */
		    {
			grantee = (char *) &sess_cb->pss_group;
			if (!MEcmp((PTR) &almtup->dba_subjname, grantee, 
					sizeof(almtup->dba_subjname)))
				applies=TRUE;
	    		break;
		    }
		    case DBGR_APLID:	   /* Check application id */
	    	    {
			 grantee = (char *) &sess_cb->pss_aplid;
			if (!MEcmp((PTR) &almtup->dba_subjname, grantee, 
					sizeof(almtup->dba_subjname)))
				applies=TRUE;
	    		break;
	            }
		    default:	    /* this should never happen */
		    {
	    		i4	err_code;

	    		(VOID) psf_error(E_US2478_9336_BAD_ALARM_GRANTEE, 0L, PSF_INTERR,
			&err_code, err_blk, 2,
			sizeof(almtup->dba_subjtype), &almtup->dba_subjtype,
			sizeof(almtup->dba_alarmname), &almtup->dba_alarmname);
			status=E_DB_ERROR;
			break;
		     }
	    }
	    if(!applies)
	    {
		continue;
	    }

	    /*
	    ** Write audit record
	    */
	    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
		local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)&rngvar->pss_tabname,
			    &rngvar->pss_ownname,
			    sizeof(DB_TAB_NAME), SXF_E_ALARM,
			    I_SX2028_ALARM_EVENT, SXF_A_FAIL | mode, 
			    &local_error);
	    /*
	    ** Raise dbevent if needed
	    */
	    if(almtup->dba_flags & DBA_DBEVENT)
	    {
		char evtxt[DB_EVDATA_MAX+1];
		char *evtxtptr;
		i4  evtxtlen;
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
		/*
		** Set up call to SCF to raise the event
		*/
		if(!STskipblank((char*)&almtup->dba_eventtext, 
				sizeof(almtup->dba_eventtext)))
		{
		    /*
		    ** No event text so build default
		    */
		    STprintf(evtxt,ERx("Security alarm fired on table %.*s.%.*s on failure when %s by %.*s"),
		    	psf_trmwhite(sizeof(DB_OWN_NAME), 
				(char *) &rngvar->pss_ownname),
			    &rngvar->pss_ownname,
		    	psf_trmwhite(sizeof(DB_TAB_NAME), 
				(char *) &rngvar->pss_tabname),
			    &rngvar->pss_tabname,
			    access_str,
		    	psf_trmwhite(sizeof(DB_OWN_NAME), 
				(char *) &sess_cb->pss_user),
			    &sess_cb->pss_user
			    );
		    evtxtlen=STlength(evtxt);
		    evtxtptr=evtxt;
		}
		else
		{
		    evtxtptr= (char*)&almtup->dba_eventtext;
		    evtxtlen=sizeof(almtup->dba_eventtext);
		}
		psy_evraise(sess_cb, &evtuple.dbe_name, &evtuple.dbe_owner,
				  evtxtptr, evtxtlen, err_blk);
	    }
	}
    } /* end while loop */
	
    /* close the iisecalarm system catalog */
do_alarm_wrapup:

    rdf_rb->rdr_update_op = RDR_CLOSE;
    stat = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
    rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;
    if (DB_FAILURE_MACRO(stat))
    {
	if (stat > status)
	{
	    _VOID_ psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error, err_blk);
	    status = stat;	
	}
    }

    return(status);
}

/*
** Name:    psy_att_map() - build a map of attributes of a relation found
**			     in a given tree;
**
** Description:	build a map of attributes of a relation used in the tree; 
**
** Input:
**	    t		- tree to be searched
**	    rgno	- number of the range variable corresponding to the
**			  relation of interest
**	    attrmap	- attribute map which has been properly initialized
**
** Output
**	    attrmap	- map of attributes of the relation will be filled in
**
** Returns:
**	none
**
** History:
**	18-sep-90 (andre)
**	    written
**	17-nov-92 (andre)
**	    changed function to be externally visible
*/
VOID
psy_att_map(
	PST_QNODE	*t,
	i4		rgno,
	char		*attrmap)
{
    if (t != (PST_QNODE *) NULL)
    {
	if (t->pst_sym.pst_type == PST_VAR)
	{
	    if (t->pst_sym.pst_value.pst_s_var.pst_vno == rgno)
	    {
		BTset(t->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
		      attrmap);
	    }
	}
	else
	{
	    if (t->pst_left != (PST_QNODE *) NULL)
	    {
		psy_att_map(t->pst_left, rgno, attrmap);
	    }
	    
	    if (t->pst_right != (PST_QNODE *) NULL)
	    {
		psy_att_map(t->pst_right, rgno, attrmap);
	    }
	}
    }
    
    return;
}

/*
** Name:	psy_fill_attmap()
**
** Description:	    fill attribute map with supplied value
**
** Input:
**	map	    ptr to the attribute map
**	val	    value to be stored
**
** Output:
**	none
**
** Returns:
**	none
**
** History:
**	18-sep-90 (andre)
**	    written
*/
VOID
psy_fill_attmap(
	register i4	*map,
	register i4	val)
{
    register i4		*last = map + DB_COL_WORDS;

    for (; map < last; map++)
    {
	*map = val;
    }
}

/*
** Name psy_grantee_ok:	determine if the permit tuple applies to this session
**			based on the name and type of grantee found in IIPROTECT
**			tuple
**
** Description:
**	Examine name and type of grantee in the IIPROTECT tuple to determine if
**	this tuple may apply to the current session.  Depending on the grantee
**	type, grantee name may be compared with user or group or role identifier
**
** Input:
**	sess_cb				PSF session CB
**	    pss_user			current user identifier
**	    pss_group			current group identifier
**	    pss_aplid			current role identifier
**	    pss_stmt_flags
**		PSS_DISREGARD_GROUP_ROLE_PERMS	    group/role permits are to be
**						    disregarded
**	protup				protection tuple
**	    dbp_gtype			grantee type
**		DBGR_PUBLIC		granted to ALL/PUBLIC
**		DBGR_USER		granted to a user
**					NOTE: due to an oversight, permits
**					CREATEd/DEFINEd TO ALL had type set to
**					DBGR_USER.  In 6.3/04 we will treat 
**					permits tuples with grantee name
**					starting with a blank and grantee type
**					set to DBGR_USER as if they were granted
**					to PUBLIC/ALL (this is not the cleanest
**					way to do it, but it is supposed to
**					cause the least harm)
**		DBGR_GROUP		granted to a group identifier
**		DBGR_APLID		granted to a role identifier
**	    dbp_owner			grantee name
**		
** Output:
**	applies				TRUE if the tuple seems to apply to the
**					current session; FALSE otherwise
**	err_blk				filled in if invalid grantee type is
**					discovered
**
** Returns:
**	E_DB_ERROR if invalid grantee type is discovered, E_DB_OK otherwise
**
** Side effects:
**	none
**
** History:
**	21-feb-91 (andre)
**	    written
**	    Note that we will disregard group/role permits when
**	    (sess_cb->pss_stmt_flags & PSS_DISREGARD_GROUP_ROLE_PERMS).  This
**	    may happen if we are parsing a rule, as view, or a database
**	    procedure definition.
**	    (this was done as a part of fix for bug #32457)
*/
DB_STATUS
psy_grantee_ok(
	PSS_SESBLK	*sess_cb,
	DB_PROTECTION	*protup,
	bool		*applies,
	DB_ERROR	*err_blk)
{
    char	*grantee = (char *) NULL;

    switch (protup->dbp_gtype)
    {
	case DBGR_PUBLIC:
	{
	    *applies = TRUE;
	    break;
	}
	case DBGR_USER:
	{
	    grantee = (char *) &sess_cb->pss_user;
	    break;
	}
	case DBGR_GROUP:			    /* Check group id */
	{
	    if (sess_cb->pss_stmt_flags & PSS_DISREGARD_GROUP_ROLE_PERMS)
	    {
		*applies = FALSE;
	    }
	    else
	    {
		grantee = (char *) &sess_cb->pss_group;
	    }
	    break;
	}
	case DBGR_APLID:			    /* Check application id */
	{
	    if (sess_cb->pss_stmt_flags & PSS_DISREGARD_GROUP_ROLE_PERMS)
	    {
		*applies = FALSE;
	    }
	    else
	    {
		grantee = (char *) &sess_cb->pss_aplid;
	    }
	    break;
	}
	default:	    /* this should never happen */
	{
	    i4	err_code;

	    (VOID) psf_error(E_PS0D50_INVALID_GRANTEE_TYPE, 0L, PSF_INTERR,
		&err_code, err_blk, 4,
		sizeof(protup->dbp_gtype), &protup->dbp_gtype,
		sizeof(protup->dbp_permno), &protup->dbp_permno,
		sizeof(protup->dbp_tabid.db_tab_base),
		    &protup->dbp_tabid.db_tab_base,
		sizeof(protup->dbp_tabid.db_tab_index),
		    &protup->dbp_tabid.db_tab_index);
	    *applies = FALSE;
	    return (E_DB_ERROR);
	}
    }

    if (grantee != (char *) NULL)
    {
	if (MEcmp((PTR) &protup->dbp_owner, grantee, sizeof(protup->dbp_owner)))
	{
	    *applies = FALSE;
	}
	else
	{
	    *applies = TRUE;
	}
    }

    return(E_DB_OK);
}

/*
** Name: psy_check_permits - check if there are permits satisfying the privilege
**			     requirement
**			     
** Inputs:
**	sess_cb				Pointer to session control block
**      rngvar				The range variable we are currently
**					interested in.
**	uset				set of all domains being updated
**	rset				set of all domains being retrieved
**					implicitly, that is, on the
**					right-hand-side of an assignment
**					operator
**	aset				set of all domains being aggregated.
**	qset				set of domains being retrieved
**					implicitly in a qualification.
**	privs				pointer to the map of required
**					privileges (may include DB_GRANT_OPTION
**					if the privileges MUST be specified WGO)
**	query_mode			real query mode
**	qmode				Query mode we are getting permission
**					for
**	dup_rb				ptr to a request block for use by
**					pst_treedup().  It is passed here so
**					that it can be used in invoking dopro()
**					+ it contains ptr to memory stream and
**					error block.
**	uset				Current update set
**	rset				Current retrieve set
**	aset				Current aggregate set
**	qset				Current qualification set
**	rdf_cb				to read protection tuples
**	rdf_tree_cb			to read tree tuples
**	rngtab				Pointer to range table
**
** Outputs:
**	privs 				contains map of operations which were
**					not satisfied by the existing permits
**	qmap				map of qualifications used with various
**					applucable permits
**	pqual				list of qualifications
**	pndx				offset of the last qualification added
**					to pqual list
**
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
**      24-jul-91 (andre)
**	    extracted from dopro()
**	26-jul-91 (andre)
**	    privs can involve more than one of "update" operations
**	    (UPDATE, DELETE, INSERT).
**
**	    NOTE: this will work correctly in SQL only.  If there is a chance
**	    that we are looking for more than one type of "update" privilege, we
**	    would have to define separate qual maps for each of
**	    "update operations" since otherwise we may incorrectly stop looking
**	    for unqualified permits.  I do not expect that this (looking for
**	    multiple "update" privileges in QUEL) will ever happen, though.
**
**	    "update" privs and DB_RETRIEVE may be requested WGO
**	25-sep-91 (andre)
**	    if parsing a dbproc and it still appears to be grantable, check if
**	    the required privileges are available WGO - if not, the dbproc being
**	    parsed is non-grantable
**	11-nov-91 (rblumer)
**	  merged from 6.4:  26-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as an array of
**	    pointers to PST_RNGENTRY structure.
**	18-mar-92 (andre)
**	    forgot to copy join maps from permit tree range entry to the new
**	    range table entry when merging permit tree into the query tree
**	03-aug-92 (barbara)
**	    Set rdr_tabid and rdr_sequence when requesting permit tree from
**	    RDF.
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	14-dec-92 (walt)
**	    Replaced \T with \t in a TRdisplay format to silence a compiler
**	    warning about illegal display formats on Alpha.
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	01-apr-93 (andre)
**	    (fix for bugs 50823 & 50825)
**	    if processing declaration of a QUEL cursor on the current table (or
**	    a view defined over the current table), we will try to determine
**	    whether the user possesses unconditional permits on all of its
**	    attributes.  Knowing this fact will enable us to avoid going through
**	    view/permit processing if cursor was defined over a table and permit
**	    processing if it was defined over a view
**	02-apr-93 (andre)
**	    (fix for bugs 50823 & 50825)
**	    we could be called in the course of processing REPLACE CURSOR to
**	    verify that the user possesses RETRIEVE on all columns appearing
**	    on the RHS of the assignments; in that case we are not interested
**	    in conditional permits either.
**	23-May-1997 (shero03)
**	    Save the rdf_info_blk after an UNFIX
**	02-Jun-1997 (shero03)
**	    Update the pss_rdrinfo pointer after calling rdf
**      14-May-1999 (carsu07)
**          The function psy_check_permits scans through 20 permits at a time
**          unless the WGO was set. Part of checking this permission involves
**          checking if the definition is of a dbproc which appears to be
**          grantable so far has WGO.  However if you're not using dbprocs, then
**          the if statement fails and privs_wgo is set.
**          This in turn means no futher permissions are checked. (Bug 80064)
**	18-Mar-2004 (schka24)
**	    Random odd results caused by failure to initialize privs_wgo,
**	    which in turn was probably caused by bad indentation from last
**	    change.  Move the init, fix the indentation.
**	20-Jul-2004 (schka24)
**	    When a privilege is satisfied, but with a conditional permit,
**	    keep looking in case we discover an unconditional permit.
**	    This was the intent all along, but the code did not keep track
**	    properly.  The result was that a retrieval might or might not
**	    have (quel) permit conditions applied, depending on the order
**	    in which we happened to see the permits.
**	    Also, the update bitmap stuff was handled in an awkward manner,
**	    clean it up.
**	4-Aug-2004 (schka24)
**	    Above change messed up quel update permits, fix.
**	    Fix another bug that terminated early before satisfying requested
**	    cursor checks.  Not sure, but I think this bug predates my 20-jul
**	    fixes as well;  could cause random spurious E_US08A3.
*/
static DB_STATUS
psy_check_permits(
	PSS_SESBLK	*sess_cb,
	PSS_RNGTAB      *rngvar,
	PSY_ATTMAP	*uset,
	PSY_ATTMAP	*rset,
	PSY_ATTMAP	*aset,
	PSY_ATTMAP	*qset,
	i4		*privs,
	i4		query_mode,
	i4		qmode,
	PSS_DUPRB	*dup_rb,
	PSY_QMAP	*qmap,
	PSY_PQUAL	*pqual,
	RDF_CB		*rdf_cb,
	RDF_CB		*rdf_tree_cb,
	PSS_USRRANGE	*rngtab,
	i4		*pndx)
{
    i4			tupcount;
    i4		err_code;
    PSY_ATTMAP		quel_cursor_retr_map;
    PSY_ATTMAP		delset;
    bool		check_retr_for_quel_cursor = FALSE;
    bool		check_delall = FALSE;
    bool		need_wgo;	/* Caller requires grant option */
    PSC_CURBLK		*cursor = (PSC_CURBLK *) NULL;
    DB_STATUS		status;
    QEF_DATA		*qp;
    DB_PROTECTION	*protup;
    PST_QTREE		*ptree;
    PST_PROCEDURE	*pnode;
    PSS_RNGTAB		*newrngvar;
#ifdef    xDEBUG
    i4		val1, val2;
#endif
    bool		apply;
    register i4	i, j;
    i4			privs_left, privs_left_wgo;	/* What we still need */
    i4			uncond_privs_left, uncond_privs_left_wgo;
    i4			privs_found, privs_to_find;
    i4			privs_found_wgo;
    i4			tuple_privs;
    i4			insdel_privs, repref_privs;
    PSY_ATTMAP		upd_cond_map;	/* update: Attrs left unpermitted */
    PSY_ATTMAP		upd_uncond_map;	/* update: Attrs left, unconditional */
    PSY_ATTMAP		upd_cond_map_wgo;
    PSY_ATTMAP		upd_uncond_map_wgo;
    PSY_ATTMAP		ones_mask;
    PSY_ATTMAP		*upd_map;	/* Either upd_cond_map or _wgo */

    /* initialize qmaps */
    for (i = 0; i < PSY_MQMAP; i++)
    {
	MEfill(sizeof(PQMAP), (u_char) 0, (PTR) qmap[i].q_map);
	qmap[i].q_usemap = qmap[i].q_unqual = FALSE;
    }

    if (   (qmode == PSQ_DEFCURS || query_mode == PSQ_DEFCURS ||
		qmode == PSQ_REPDYN || query_mode == PSQ_REPDYN)
	&& rngvar == sess_cb->pss_resrng)
    {
	cursor = sess_cb->pss_crsr;

	/*
	** If checking update permission on a "define cursor" and haven't
	** already determined that the user may DELETE from the cursor's
	** underlying table, it is necessary to check for delete permission.
	** In that case we set up a special domain set and operation map for
	** delete.  These will be checked versus each permit tuple that applies,
	** using prochk().
	*/
	if (!cursor->psc_delall)
	{
	    check_delall = TRUE;
	    psy_fill_attmap(delset.map, ~((i4) 0));
	}

	/*
	** if this is a QUEL cursor and psy_passed_prelim_check() has not
	** determined that the user possesses unconditional RETRIEVE on every
	** column in the table/view, remember that we would like to determine
	** whether the user possesses unconditional RETRIEVE on every column in
	** the table/view + initialize quel_cursor_retr_map
	*/
	if (   sess_cb->pss_lang == DB_QUEL
	    && ~cursor->psc_flags & PSC_MAY_RETR_ALL_COLUMNS)
	{
	    check_retr_for_quel_cursor = TRUE;
	    psy_fill_attmap(quel_cursor_retr_map.map, ~((i4) 0));
	}
    }

    /* Privs-left are privileges we don't have at all and still need.
    ** uncond-privs-left are privileges that we don't yet have unconditionally
    ** (ie we may have the privilege, but if so, it's conditional and we
    ** should keep looking).
    ** The DB_GRANT_OPTION flag is not set in either, it's carried in a
    ** separate flag.
    ** See below for the _wgo stuff.
    */

    privs_left = *privs & ~DB_GRANT_OPTION;
    uncond_privs_left = privs_left;
    privs_left_wgo = 0;
    uncond_privs_left_wgo = 0;
    need_wgo = (*privs & DB_GRANT_OPTION) != 0;

    /* If we'll be messing about with update privs, make a couple copies
    ** of the update column map.  Update permits can be column specific,
    ** and we'll incrementally clear map bits until they are all off.
    ** Do this in some working copies.
    */
    if (privs_left & (DB_REPLACE | DB_REFERENCES))
    {
	STRUCT_ASSIGN_MACRO(*uset, upd_cond_map);
	STRUCT_ASSIGN_MACRO(upd_cond_map, upd_uncond_map);
    }

    /*
    ** if *privs does not include DB_GRANT_OPTION and we are processing a
    ** definition of a dbproc which appears to be grantable so far, we will
    ** check for existence of all required privileges WGO to ensure that it is,
    ** indeed, grantable.
    ** This is where privs_left_wgo and uncond_privs_left_wgo come into
    ** play.  The privilege check success or failure depends on what permits
    ** we see regardless of their WGO-ness;  but in addition we want to
    ** record (separately) any applicable permits that are in fact WGO.
    ** At the end, the priv check may succeed, but if we don't find all
    ** necessary privs WGO, the DB proc is not grantable.
    */

    if (   sess_cb->pss_dbp_flags & PSS_DBPROC   
	&& sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK
	&& !need_wgo)
    {
	privs_left_wgo = uncond_privs_left_wgo = privs_left;

	/*
	** if we were interested in finding out if a given dbproc is grantable,
	** until this point we have been avoiding using information found in
	** relstat to determine if some of the required privileges have been
	** satisfied; this was done because we cannot determine if the dbproc is
	** grantable without ascertaining that the current user posesses all of
	** the required privileges WGO.
	** Now that we have copied the map of required privileges into the map
	** of privileges WGO, we can check relstat.
	** Note that relstat information will not be used if the required
	** privileges were themselves WGO, in which case the information which
	** we need cannot be found in relstat
	*/
	if (psy_passed_prelim_check(sess_cb, rngvar, query_mode))
	{
	    privs_left = uncond_privs_left = (i4) 0;
	}
    }

    /* If we are tracking WGO separately, and need UPDATE or REFERENCES,
    ** make a WGO copy of the column maps
    */
    if (privs_left_wgo & (DB_REPLACE | DB_REFERENCES))
    {
	STRUCT_ASSIGN_MACRO(upd_cond_map, upd_cond_map_wgo);
	STRUCT_ASSIGN_MACRO(upd_uncond_map, upd_uncond_map_wgo);
    }

    /* Double check that caller asked for something */
    if (privs_left == 0 && privs_left_wgo == 0)
    {
	*privs = 0;
	return (E_DB_OK);
    }

    /* Easier to test for non-retrieval privs, there are a boatload
    ** of update-ish privs.  Exclude actual UPDATE, it's handled specially.
    */
    insdel_privs = privs_left & ~(DB_RETRIEVE | DB_TEST | DB_AGGREGATE
					| DB_REPLACE | DB_REFERENCES);
    if (insdel_privs == 0)
	insdel_privs = privs_left_wgo & ~(DB_RETRIEVE | DB_TEST | DB_AGGREGATE
					| DB_REPLACE | DB_REFERENCES);

    /* Similarly for actual UPDATE or REFERENCES privs, these are special
    ** because of the cumulative column map business.  (ie you can grant
    ** update and references privileges on individual columns.)
    */
    repref_privs = privs_left & (DB_REPLACE | DB_REFERENCES);
    if (repref_privs == 0)
	repref_privs = privs_left_wgo & (DB_REPLACE | DB_REFERENCES);


    /* check the protection catalog */

    /* Get permit tuples from RDF that apply to this table */
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_tree_cb->rdf_rb.rdr_tabid);
    rdf_tree_cb->rdf_info_blk	    = rngvar->pss_rdrinfo;
    rdf_tree_cb->rdf_rb.rdr_update_op    = 0;
    rdf_tree_cb->rdf_rb.rdr_qtuple_count = 0;

    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_cb->rdf_rb.rdr_tabid);
    rdf_cb->rdf_info_blk	    = rngvar->pss_rdrinfo;
    rdf_cb->rdf_rb.rdr_qrymod_id    = 0;	/* Get all permit tuples */
    rdf_cb->rdf_rb.rdr_update_op    = RDR_OPEN;
    rdf_cb->rdf_rb.rdr_rec_access_id = NULL;
    rdf_cb->rdf_rb.rdr_qtuple_count = 20;   /* Get 20 permits at a time */
    rdf_cb->rdf_error.err_code	    = 0;

    /*
    ** search IIPROTECT until some error occurs or until we find the required
    ** privileges and, possibly, the required privileges WGO
    */
    
    /* For each group of 20 permits */
    do
    {
	status = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);

	/*
	** RDF may choose to allocate a new info block and return its address in
	** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
	** leak and other assorted unpleasantries
	*/
	if (rdf_cb->rdf_info_blk != rngvar->pss_rdrinfo)
	{
	    rngvar->pss_rdrinfo = rdf_tree_cb->rdf_info_blk = rdf_cb->rdf_info_blk;
	}

	rdf_cb->rdf_rb.rdr_update_op = RDR_GETNEXT;
	if (DB_FAILURE_MACRO(status))
	{
	    if (rdf_cb->rdf_error.err_code == E_RD0011_NO_MORE_ROWS
		|| rdf_cb->rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	    {
		/* Not errors, just eof */
		status = E_DB_OK;
	    }
	    else
	    {
		if (rdf_cb->rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		{
		    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
			    &err_code, dup_rb->pss_err_blk, 1,
			    psf_trmwhite(sizeof(DB_TAB_NAME), 
			    (char *) &rngvar->pss_tabname),
			    &rngvar->pss_tabname);
		}
		else
		{
		    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error,
				     dup_rb->pss_err_blk);
		}
	    }
	    /*
	    ** break out of WHILE loop; tuple stream will get closed before
	    ** exiting the function
	    */
	    break;	
	}

	/* For each permit tuple */
	for (
	     qp = rdf_cb->rdf_info_blk->rdr_ptuples->qrym_data,
		 tupcount = 0;
	     tupcount < rdf_cb->rdf_info_blk->rdr_ptuples->qrym_cnt;
	     qp = qp->dt_next, tupcount++
	)
	{
	    /* Set protup pointing to current permit tuple */
	    protup = (DB_PROTECTION*) qp->dt_data;
#ifdef xDEBUG
	    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	    {
		TRdisplay("Current protection tuple:\n");
		TRdisplay("\tdbp_tabid: %d,%d\n", protup->dbp_tabid.db_tab_base,
		    protup->dbp_tabid.db_tab_index);
		TRdisplay("\tdbp_permno: %d\n", protup->dbp_permno);
		TRdisplay("\tdbp_popset: %d\n", protup->dbp_popset);
		TRdisplay("\tdbp_pdbgn: %d\n", protup->dbp_pdbgn);
		TRdisplay("\tdbp_pdend: %d\n", protup->dbp_pdend);
		TRdisplay("\tdbp_pwbgn: %d\n", protup->dbp_pwbgn);
		TRdisplay("\tdbp_pwend: %d\n", protup->dbp_pwend);
		pr_set((i4 *) protup->dbp_domset, "dbp_domset");
		TRdisplay("\tdbp_txtid: %d,%d\n",
		    protup->dbp_txtid.db_qry_high_time,
		    protup->dbp_txtid.db_qry_low_time);
		TRdisplay("\tdbp_owner: %#s\n", sizeof (protup->dbp_owner),
			&protup->dbp_owner);
		TRdisplay("\tdbp_term: %#s\n", sizeof (protup->dbp_term),
			&protup->dbp_term);
		TRdisplay("\n");
	    }
#endif

	    /*
	    ** if
	    **  - this is a security alarm OR
	    **  - the permit was defined with a qualification and
	    **	- we are processing an SQL query (SQL permits ignore
	    **	  qualifications) OR
	    **	- we are processing REPLACE CURSOR (we are only interested in
	    **	  unconditional RETRIEVE privilege on attributes on the RHS of
	    **	  assignment)
	    **  
	    ** then
	    **  skip it
	    */
	    if (   protup->dbp_popset & DB_ALARM
		|| (   (   protup->dbp_treeid.db_tre_high_time != 0
			|| protup->dbp_treeid.db_tre_low_time != 0)
		    && 
		       (   sess_cb->pss_lang == DB_SQL
			|| qmode == PSQ_REPCURS)
		   )
	       )
		continue;

	    /* check if this is the correct user, terminal, etc */
	    status = proappl(protup, &apply, sess_cb, dup_rb->pss_err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    if (!apply)
		continue;

#ifdef xDEBUG
	    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	    {
		TRdisplay("In dopro: proappl returned TRUE\n");
	    }
#endif

	    /* all right, let's check the operation */

	    privs_found = privs_found_wgo = 0;
	    qmap[0].q_applies = FALSE;
	    qmap[1].q_applies = FALSE;
	    qmap[2].q_applies = FALSE;
	    qmap[3].q_applies = FALSE;
	    psy_fill_attmap(&ones_mask.map[0], (i4) ~0);
	    upd_map = NULL;

	    /* If we still need a priv in uncond_privs_left, check that
	    ** priv (need_wgo says whether to ask for grant option too).
	    ** Otherwise, if we still need a priv in uncond_privs_left_wgo,
	    ** check for that priv with WGO.  The distinction is subtle
	    ** but if we are doing a DBP definition, we may ask about
	    ** non-WGO privs and yet record their WGO-ness.
	    */

	    /* See if we need update priv and tuple permits it.
	    ** Update has to be handled separately from the other update-type
	    ** privs because multiple permits may apply to different
	    ** columns.  We'll let prochk update a work mask and then
	    ** we'll mask out the permitted columns from the appropriate
	    ** column mask.  (Unfortunately prochk doesn't tell us when
	    ** some but not all columns were permitted, vs no permit.)
	    **
	    ** This check MUST BE FIRST because it assigns the privs-found
	    ** variables instead of and'ing or or'ing them, for convenience.
	    */

	    privs_to_find = uncond_privs_left & repref_privs;
	    if (privs_to_find != 0)
	    {
		if (need_wgo) privs_to_find |= DB_GRANT_OPTION;
		upd_map = &upd_cond_map;
	    }
	    else
	    {
		privs_to_find = uncond_privs_left_wgo & repref_privs;
		if (privs_to_find != 0)
		{
		    privs_to_find |= DB_GRANT_OPTION;
		    upd_map = &upd_cond_map_wgo;
		}
	    }
	    if (privs_to_find != 0)
	    {
		/* We only care about the column map change, if any.
		** prochk will clear any bits for permitted columns.
		*/
		(void) prochk(privs_to_find, &ones_mask.map[0],
					protup, sess_cb);
		privs_found = repref_privs;
		if (sess_cb->pss_lang == DB_QUEL)
		{
		    /* QUEL does not allow update permits to be cumulative.
		    ** A permit has to include all columns in the replace,
		    ** or it's ignored.
		    ** See if we have all needed columns in this permit.
		    */
		    for (i = 0; i < DB_COL_WORDS; ++i)
		    {
			if ((upd_map->map[i] & ones_mask.map[i]) != 0)
			{
			    privs_found = 0;
			    /* Claim that nothing happened... */
			    psy_fill_attmap(&ones_mask.map[0], (i4) ~0);
			    break;
			}
		    }
		    /* Don't need the wgo stuff in quel */
		}
		else
		{
		    /* SQL allows cumulative update permits.
		    ** Clear any column permits this tuple had, see if we have
		    ** them all yet
		    */
		    for (i = 0; i < DB_COL_WORDS; ++i)
			if ( (upd_map->map[i] &= ones_mask.map[i]) != 0)
			    privs_found = 0;
		    /* If we have a GRANT OPTION permit tuple, record the
		    ** columns it permits in our wgo map as well (if we care)
		    */
		    if (protup->dbp_popset & DB_GRANT_OPTION
		      && upd_map == &upd_cond_map
		      && uncond_privs_left_wgo & repref_privs)
		    {
			privs_found_wgo = repref_privs;
			for (i = 0; i < DB_COL_WORDS; ++i)
			    if ( (upd_cond_map_wgo.map[i] &= ones_mask.map[i]) != 0)
				privs_found_wgo = 0;
		    }
		}
		/* Only the permit that fully and/or finally satisfies UPDATE
		** "applies".  We get away with this for SQL and its cumulative
		** permits only because SQL grants don't have where-trees;
		** so it doesn't matter that we don't "apply" partial UPDATE
		** permits in sql.
		*/
		if (privs_found != 0)
		    qmap[0].q_applies = TRUE;
	    }

	    /* If we are looking for other kinds of updatey privs,
	    ** check the permit tuple to see if it supplies them.
	    */

	    privs_to_find = uncond_privs_left & insdel_privs;
	    if (privs_to_find != 0)
	    {
		if (need_wgo) privs_to_find |= DB_GRANT_OPTION;
	    }
	    else
	    {
		privs_to_find = uncond_privs_left_wgo & insdel_privs;
		if (privs_to_find != 0) privs_to_find |= DB_GRANT_OPTION;
	    }
	    if (privs_to_find != 0)
	    {
		tuple_privs = prochk(privs_to_find, uset->map,
					protup, sess_cb);
		if (tuple_privs != 0)
		{
		    privs_found |= tuple_privs;
		    if (protup->dbp_popset & DB_GRANT_OPTION)
			privs_found_wgo |= tuple_privs;
		    qmap[0].q_applies = TRUE;
		}
	    }

	    /* If we are looking for retrieval privileges,
	    ** check the permit tuple to see if it supplies them.
	    */

	    privs_to_find = uncond_privs_left & DB_RETRIEVE;
	    if (privs_to_find != 0)
	    {
		if (need_wgo) privs_to_find |= DB_GRANT_OPTION;
	    }
	    else
	    {
		privs_to_find = uncond_privs_left_wgo & DB_RETRIEVE;
		if (privs_to_find != 0) privs_to_find |= DB_GRANT_OPTION;
	    }
	    if (privs_to_find != 0)
	    {
		tuple_privs = prochk(privs_to_find, rset->map,
					protup, sess_cb);
		if (tuple_privs != 0)
		{
		    privs_found |= tuple_privs;
		    if (protup->dbp_popset & DB_GRANT_OPTION)
			privs_found_wgo |= tuple_privs;
		    qmap[1].q_applies = TRUE;
		}
	    }

	    /* If we are looking for aggregation privileges,
	    ** check the permit tuple to see if it supplies them.
	    */

	    privs_to_find = uncond_privs_left & DB_AGGREGATE;
	    if (privs_to_find != 0)
	    {
		if (need_wgo) privs_to_find |= DB_GRANT_OPTION;
	    }
	    else
	    {
		privs_to_find = uncond_privs_left_wgo & DB_AGGREGATE;
		if (privs_to_find != 0) privs_to_find |= DB_GRANT_OPTION;
	    }
	    if (privs_to_find != 0)
	    {
		tuple_privs = prochk(privs_to_find, aset->map,
					protup, sess_cb);
		if (tuple_privs != 0)
		{
		    privs_found |= tuple_privs;
		    if (protup->dbp_popset & DB_GRANT_OPTION)
			privs_found_wgo |= tuple_privs;
		    qmap[2].q_applies = TRUE;
		}
	    }

	    /* If we are looking for test (where clause) privileges,
	    ** check the permit tuple to see if it supplies them.
	    */

	    privs_to_find = uncond_privs_left & DB_TEST;
	    if (privs_to_find != 0)
	    {
		if (need_wgo) privs_to_find |= DB_GRANT_OPTION;
	    }
	    else
	    {
		privs_to_find = uncond_privs_left_wgo & DB_TEST;
		if (privs_to_find != 0) privs_to_find |= DB_GRANT_OPTION;
	    }
	    if (privs_to_find != 0)
	    {
		tuple_privs = prochk(privs_to_find, qset->map,
					protup, sess_cb);
		if (tuple_privs != 0)
		{
		    privs_found |= tuple_privs;
		    if (protup->dbp_popset & DB_GRANT_OPTION)
			privs_found_wgo |= tuple_privs;
		    qmap[3].q_applies = TRUE;
		}
	    }

#ifdef xDEBUG
	    if (ult_check_macro(&sess_cb->pss_trace, 50, &val1, &val2))
	    {
		TRdisplay("Protection tuple permits 0x%x (0x%x WGO)\n",
		    privs_found, privs_found_wgo);
	    }
#endif

    /*
    ** If we are checking update permission for a cursor and have not yet
    ** established that the user may delete through the cursor, we need to
    ** check whether this permit conveys the necessary privilege.
    ** 
    ** If we are creating a QUEL cursor, we would like to know whether the
    ** user possesses unconditional RETRIEVE on every column of the
    ** table/view over which the cursor is being defined.  If we haven't
    ** established that so far, check whether this permit conveys the
    ** necessary privilege.
    ** 
    ** In both cases, we will insist that the privilege granted by the
    ** permit is unconditional (i.e. no trees.)
    */
	    if (   protup->dbp_treeid.db_tre_high_time == 0
		&& protup->dbp_treeid.db_tre_low_time  == 0)
	    {
		if (   check_delall
		    && prochk(DB_DELETE, delset.map, protup, sess_cb) == DB_DELETE)
		{
		    check_delall = FALSE;
		    cursor->psc_delall = TRUE;
		}

		if (   check_retr_for_quel_cursor
		    && prochk(DB_RETRIEVE, quel_cursor_retr_map.map, protup,
			   sess_cb) == DB_RETRIEVE
		   )
		{
		    cursor->psc_flags |= PSC_MAY_RETR_ALL_COLUMNS;
		    check_retr_for_quel_cursor = FALSE;
		}

		/* For an unconditional permit, always update the
		** appropriate UPDATE column map.  If the permit did not
		** confer any update or references column privs, nothing
		** will happen.  We have to do this here because if we
		** don't yet have all the column privs we need, we'll
		** decide that this tuple is "uninteresting" and toss it.
		*/
		if (upd_map != NULL)
		{
		    if (upd_map == &upd_cond_map)
		    {
			tuple_privs = repref_privs;
			for (i = 0; i < DB_COL_WORDS; ++i)
			    if ( (upd_uncond_map.map[i] &= ones_mask.map[i]) != 0)
				tuple_privs = 0;
			uncond_privs_left &= ~tuple_privs;
		    }
		    if (protup->dbp_popset & DB_GRANT_OPTION
		      && uncond_privs_left_wgo & repref_privs)
		    {
			/* This permit is WGO, record it as WGO as well */
			tuple_privs = repref_privs;
			for (i = 0; i < DB_COL_WORDS; ++i)
			    if ( (upd_uncond_map_wgo.map[i] &= ones_mask.map[i]) != 0)
				tuple_privs = 0;
			uncond_privs_left_wgo &= ~tuple_privs;
		    }
		}
	    }

	    /* see if this tuple is "interesting" */
	    if (privs_found == 0 && privs_found_wgo == 0)
		continue;
		    
	    /* Get the query tree for this permit from RDF (if any) */
	    ptree = (PST_QTREE *) NULL;
	    rdf_tree_cb->rdf_rb.rdr_protect = NULL;

	    /* Zero tree id means no qualification */
	    if (protup->dbp_treeid.db_tre_high_time != 0 ||
		protup->dbp_treeid.db_tre_low_time != 0)
	    {
	/*
	** If checking update permission on a cursor, and we find a permit
	** with a tree, give an error and return.  This prevents having to
	** run a separate query for each "replace cursor" command, because
	** the where clause for "replace cursor" won't contain any range
	** variables from permits.
	**
	** This has been approved by PMC/LRC committees, since we do not
	** want to have different data sets when opening cursors for readonly
	** and for update (this is possible if there is an update permit with
	** a qualification). We could (theoreticaly) append such update 
	** permits to the tree on replace cursor request but QEF is not 
	** set up to do this, so we just disallow cases with update
	** permits with quals.
	*/
		if (qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN)
		{
		    (VOID) psf_error(3380L, 0L, PSF_USERERR, &err_code,
			dup_rb->pss_err_blk, 1,
			psf_trmwhite(sizeof(DB_TAB_NAME), 
			    (char *) &rngvar->pss_tabname),
			&rngvar->pss_tabname);
		    return (E_DB_ERROR);
		}

		/* Look up the tree with this permit number */
		rdf_tree_cb->rdf_rb.rdr_rec_access_id = NULL;
		rdf_tree_cb->rdf_error.err_code	    = 0;
		rdf_tree_cb->rdf_rb.rdr_qrymod_id = protup->dbp_permno;
		STRUCT_ASSIGN_MACRO(protup->dbp_treeid,
		    rdf_tree_cb->rdf_rb.rdr_tree_id);
		rdf_tree_cb->rdf_rb.rdr_sequence = protup->dbp_permno;
		STRUCT_ASSIGN_MACRO(protup->dbp_tabid,
				    rdf_tree_cb->rdf_rb.rdr_tabid);
		status = rdf_call(RDF_GETINFO, (PTR) rdf_tree_cb);

		/*
		** RDF may choose to allocate a new info block and return its
		** address in rdf_info_blk - we need to copy it over to
		** pss_rdrinfo to avoid memory leak and other assorted
		** unpleasantries
		*/
		if (rdf_tree_cb->rdf_info_blk != rngvar->pss_rdrinfo)
		{
		    rngvar->pss_rdrinfo =
			rdf_cb->rdf_info_blk = rdf_tree_cb->rdf_info_blk;
		}
		
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_tree_cb->rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		    {
			(VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
			    &err_code, dup_rb->pss_err_blk, 1,
			    psf_trmwhite(sizeof(DB_TAB_NAME), 
				(char *) &rngvar->pss_tabname),
			    &rngvar->pss_tabname);
		    }
		    else
		    {
			(VOID) psf_rdf_error(RDF_GETINFO, &rdf_tree_cb->rdf_error,
			    dup_rb->pss_err_blk);
		    }

		    return (E_DB_ERROR);
		}

		pnode = 
		(PST_PROCEDURE *) rdf_tree_cb->rdf_rb.rdr_protect->qry_root_node;
		ptree = pnode->pst_stmts->pst_specific.pst_tree;
	    }

	    /* The tuple is "interesting".  Process the qualification (if any).
	    ** The qualification is not yet merged into the query, but we
	    ** will duplicate the qual tree, and preprocess its range vars
	    ** (adjusting their numbers so that each qual tree fragment will
	    ** not conflict with any others, or with the user query).
	    ** Keep in mind that we may end up not using the qual tree for
	    ** various reasons;  we may find an unconditional permit later on,
	    ** or a qual may be eliminated by applypq.
	    */
	    if (ptree != (PST_QTREE *) NULL)
	    {
		bool	    qualused = FALSE;
		i4		    map[PST_NUMVARS];
		PST_VRMAP	    permmap;
		PST_QNODE	    *temp_node, *p;

		/*
		** Enter the range variables from the permit tree into
		** the user range table.  Whenever a range variable is
		** entered, change the range variable numbers in the permit
		** tree to match the range variable it got entered as.
		** If we are doing a replace, delete, or replace, merge the
		** result range variable in the permit tree with the current
		** range variable we are getting permission on.  If we are
		** doing an append, enter the result range variable as a
		** separate variable (where clauses on append permits create
		** disjoint queries).
		*/

		/* Set up a range variable map that maps everything to itself */
		for (i = 0; i < PST_NUMVARS; i++)
		    map[i] = i;

		/* Find out what range variables exist in the permit tree */
		(VOID)psy_varset(ptree->pst_qtree, &permmap);

		/* Now enter each one in the range table and re-map */
		for (i = -1; (i = BTnext(i, (char *) &permmap, PST_NUMVARS)) >= 0;)
		{
		    char            *old_mask;
		    PST_J_MASK      tmp_mask;
		    
		    /*
		    ** If we are doing an append, or we are not doing the
		    ** result range variable, just enter it.
		    */
		    if (qmode == PSQ_APPEND || i != ptree->pst_restab.pst_resvno)
		    {
			/*
			** Range variable name "!" means don't look for and try
			** to replace a range variable of the same name.
			*/
			status = pst_rgent(sess_cb, rngtab, -1, "!", PST_SHWID, 
			    (DB_TAB_NAME*) NULL, (DB_TAB_OWN*) NULL,
			    &ptree->pst_rangetab[i]->pst_rngvar, FALSE,
			    &newrngvar, (i4) 0, dup_rb->pss_err_blk);
			if (DB_FAILURE_MACRO(status))
			{
			    /* Case of missing tables needs to be handled
			    ** gracefully.
			    */
			    if (dup_rb->pss_err_blk->err_code ==
							  E_PS0903_TAB_NOTFOUND)
			    {
				(VOID) psf_error(3381L, 0L, PSF_USERERR,
				    &err_code, dup_rb->pss_err_blk, 0);
			    }
			    return (status);
			}

			/*
			** copy inner and outer join maps into the new range
			** table entry
			*/
			MEcopy((char *)&ptree->pst_rangetab[i]->pst_inner_rel,
			    sizeof(PST_J_MASK), (char *)&newrngvar->pss_inner_rel);
			MEcopy((char *)&ptree->pst_rangetab[i]->pst_outer_rel,
			    sizeof(PST_J_MASK), (char *)&newrngvar->pss_outer_rel);

			/* don't check permits on these */
			newrngvar->pss_permit = FALSE;

			/*
			** reconstruct range variable name from the permit
			** definition; this may be later copied into
			** PST_RNGENTRY structure and used by OPF when
			** displaying QEP-related info
			*/
			MEcopy((PTR) &ptree->pst_rangetab[i]->pst_corr_name,
			    DB_TAB_MAXNAME, (PTR) newrngvar->pss_rgname);
		    }
		    else
		    {
			/*
			** If doing the result range variable for a non-append,
			** just re-map without entering the new variable.
			*/
			newrngvar = rngvar;
		    }

		    /* Set up the range variable for re-mapping */
		    map[i] = newrngvar->pss_rgno;

		    /*
		    ** we may need to update the mask indicating in which (if
		    ** any) joins this range var is an inner or an outer
		    ** relation
		    */
		    if (dup_rb->pss_num_joins != PST_NOJOIN)
		    {
			if ((j = BTnext((i4) -1,
				    (old_mask = (char *) &newrngvar->pss_inner_rel),
				    sizeof(PST_J_MASK) * BITSPERBYTE)) != -1
			   )
			{
			    MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &tmp_mask);
			    for (;
				 j != -1;
				 j = BTnext(j, old_mask,
					    sizeof(PST_J_MASK) * BITSPERBYTE))
			    {
				BTset(j + (i4) dup_rb->pss_num_joins,
				      (char *) &tmp_mask);
			    }

			    /* Copy the new mask in place of the old mask */
			    MEcopy((char *)&tmp_mask, sizeof(PST_J_MASK),
				    (char *)&newrngvar->pss_inner_rel);
			}
			
			if ((j = BTnext((i4) -1,
				    (old_mask = (char *) &newrngvar->pss_outer_rel),
				    sizeof(PST_J_MASK) * BITSPERBYTE)) != -1
			   )
			{
			    MEfill(sizeof(PST_J_MASK), (u_char) 0, (PTR) &tmp_mask);
			    for (;
				 j != -1;
				 j = BTnext(j, old_mask,
					    sizeof(PST_J_MASK) * BITSPERBYTE))
			    {
				BTset(j + (i4) dup_rb->pss_num_joins,
				      (char *) &tmp_mask);
			    }

			    /* Copy the new mask in place of the old mask */
			    MEcopy((char *)&tmp_mask, sizeof(PST_J_MASK),
				    (char *)&newrngvar->pss_outer_rel);
			}
		    }
		}

		/* Now re-map */
		/* copy the query tree since it currently lives in RDF
		** memory, and we'll unfix it below
		*/
		
		/* No need to reset join_id if no joins've been found so far */
		if (dup_rb->pss_num_joins != PST_NOJOIN)
		{
		    dup_rb->pss_op_mask |= PSS_2DUP_RESET_JOINID;
		}

		dup_rb->pss_tree = ptree->pst_qtree;
		dup_rb->pss_dup  = &temp_node;
		status = pst_treedup(sess_cb, dup_rb);

		if (DB_FAILURE_MACRO(status))
		    return (status);

		/*
		** Now that we are done with processing range vars involved in
		** the permit qualification, increase dup_rb.pss_num_joins by
		** the number of joins found in the qualification
		*/
		dup_rb->pss_num_joins += ptree->pst_numjoins;

		status = psy_mapvars(temp_node, map, dup_rb->pss_err_blk);
		
		if (DB_FAILURE_MACRO(status))
		    return (status);

		/*
		** Now get rid of the QLEND node from the permit tree,
		** so it can be appended to the user's query.
		*/
		p = psy_trmqlnd(temp_node->pst_right);

#ifdef xDEBUG
		if (ult_check_macro(&sess_cb->pss_trace, 51, &val1, &val2))
		{
		    TRdisplay("Protection Clause:\n");
		    (VOID) pst_prmdump(p, (PST_QTREE *) NULL, dup_rb->pss_err_blk,
			DB_PSF_ID);
		}
#endif

#ifdef xDEV_TEST
		/* check for a non-null qualification */
		if (p == NULL)
		{
		    (VOID) psf_error(E_PS0D11_NULL_TREE, 0L, PSF_INTERR,
			&err_code, dup_rb->pss_err_blk, 0);
		    return (E_DB_SEVERE);
		}
#endif

		/*
		** Permit qual may need to be attached more than once if it
		** applies to more than one operation in query (e.g. REPLACE
		** and TEST).  We also need to optimize this to avoid
		** redundancies like replace x(...) where Q and P and P.
		*/

		/* mark which operations this qual applies to */
		for (j = 0; j < PSY_MQMAP; j++)
		{
		    if (!qmap[j].q_applies || qmap[j].q_unqual)
		    {
			continue;
		    }
		    else
		    {
			BTset(*pndx, (char *) qmap[j].q_map);
			qmap[j].q_usemap = TRUE;
			qualused = TRUE;
		    }
		}

		if (!qualused)
		    continue;

		if (*pndx >= PSY_MPQUAL)
		{
		    i4	    errparm = PSY_MPQUAL;

		    /*
		    ** Number of protection qualifications on this var in this
		    ** query exceeds maximum of PSY_MPQUAL
		    */
		    psy_error(3501L, -1, rngvar, dup_rb->pss_err_blk,
			sizeof(errparm), &errparm);
		}
		pqual[*pndx].p_used = FALSE;
		/* we have made a copy of p, assign it here */
		pqual[(*pndx)++].p_qual = p;
		if (DB_FAILURE_MACRO(status))
		{
		    return (status);
		}
	    }
	    else
	    {
		/*
		** Permission for operation(s) is UNqualified
		** (i.e. needs no qual).  This permission therefore
		** supercedes any qualified permissions
		*/
		for (j=0; j < PSY_MQMAP; j++)
		{
		    if (qmap[j].q_applies)
		    {
			qmap[j].q_unqual = TRUE;

			/*
			** Eliminate qual checking for those operations
			** permitted unconditionally.
			*/
			qmap[j].q_usemap = FALSE;
		    }

		}
		/* Record that we've seen this permit unconditionally,
		** and there is no point in asking for it again.
		** (Note: unconditional update or references priv is
		** recorded earlier, above, based on the column map.)
		*/
		uncond_privs_left &= ~privs_found;
		uncond_privs_left_wgo &= ~privs_found_wgo;
	    }

	    /* mark this operation as having been handled */

	    privs_left &= ~privs_found;
	    privs_left_wgo &= ~privs_found_wgo;

	    if (rdf_tree_cb->rdf_rb.rdr_protect)
	    {   /* Unfix the query tree from the RDF cache */
		DB_STATUS	temp_status;
		temp_status = rdf_call(RDF_UNFIX, (PTR) rdf_tree_cb);
		rngvar->pss_rdrinfo = 
		    rdf_cb->rdf_info_blk = rdf_tree_cb->rdf_info_blk;
		rdf_tree_cb->rdf_rb.rdr_protect = NULL;
		if (DB_FAILURE_MACRO(temp_status))
		{
		    (VOID) psf_rdf_error(RDF_UNFIX, &rdf_cb->rdf_error,
			dup_rb->pss_err_blk);
		    status = temp_status;
		}
	    }

	    /* Keep looking if we're interested in goofy cursor stuff,
	    ** even if we've satisfied the requested privs.  E.g. cursor
	    ** might need to know if it can delete.
	    */
	    if (uncond_privs_left == 0 && uncond_privs_left_wgo == 0
	      && ! check_delall  && ! check_retr_for_quel_cursor)
	    {
		/*
		** all the required privs have been satisfied unconditionally
		** and the required privs WGO (if any) have been satisfied as
		** well -  we don't need to examine any more tuples
		*/
		break;
	    }
	} /* for each permit tuple */
    } while (rdf_cb->rdf_error.err_code == 0);

    if (rdf_cb->rdf_rb.rdr_rec_access_id != NULL)
    {
	DB_STATUS	temp_status;
	rdf_cb->rdf_rb.rdr_update_op = RDR_CLOSE;
	temp_status = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
	rngvar->pss_rdrinfo = rdf_tree_cb->rdf_info_blk = rdf_cb->rdf_info_blk;
	if (DB_FAILURE_MACRO(temp_status))
	{
	    (VOID) psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error,
		dup_rb->pss_err_blk);
	    status = temp_status;
	}
    }

    /* Tell caller which privs are still outstanding, if any */
    *privs = privs_left;

    /* If UPDATE/REFERENCES privs were not satisfied, tell caller what
    ** columns were not satisfied.
    */
    if (privs_left & repref_privs)
    {
	STRUCT_ASSIGN_MACRO(upd_cond_map, *uset);
    }

    if (DB_SUCCESS_MACRO(status) && privs_left_wgo != 0)
    {
	/*
	** we are processing a dbproc definition; until now the dbproc appeared
	** to be grantable, but the user lacks the required privileges WGO on
	** this table - mark the dbproc as non-grantable
	*/
	sess_cb->pss_dbp_flags &= ~PSS_DBPGRANT_OK;
    }

    return(status);
}

/*
** Name:    psy_insert_objpriv - insert a new element into a list of object
**				 privilege descriptors
**
** Description:
**	Given object id and type and privilege map, construct a new privilege
**	descriptor and insert it at the head of the privilege descriptor list
**
** Input:
**	objid				object id ptr ptr (NULL if creating an
**					INFOLIST_HEADER)
**	objtype				object type (i.e. table or dbproc)
**	    PSQ_OBJTYPE_IS_TABLE	element will contain a description of a
**					table-wide privilege
**	    PSQ_DBPLIST_HEADER		header of the list of privileges
**					associated with a dbproc
**	    PSQ_INFOLIST_HEADER		header of the list of privileges
**					associated with a failed attempt to
**					reparse a dbproc while processing
**					GRANT ON PROCEDURE
**	privmap				privilege map
**	mstream				memory stream from which to allocate
**					memory for a new element
**	priv_list			address of the pointer to the first
**					element of the privilege descriptor list
**
** Output:
**	*priv_list			will point at the new descriptor
**	err_blk				will be filled in if an error is
**					encountered
**
** Exceptions:
**	none
**
** Side effects:
**	will allocate memory
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	13-sep-91 (andre)
**	    written
**	07-oct-91 (andre)	<---	see next comment
**	    will copy object id only if the objtype is TABLE, DBPROC or
**	    DBPLIST_HEADER (i.e. if it is not INFOLIST_HEADER.)
**	11-oct-91 (andre)
**	    On second thought, there may be some utility in knowing that the
**	    following sublist may contain a PSQ_MISSING_PRIVILEGE descriptor
**	    which would explain why the dbproc is ungrantable;
**	    so the (07-oct-91 (andre)) should be disregarded, i.e. object id
**	    will be copied regardless of objtype
*/
DB_STATUS
psy_insert_objpriv(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*objid,
	i4		objtype,
	i4		privmap,
	PSF_MSTREAM	*mstream,
	PSQ_OBJPRIV	**priv_list,
	DB_ERROR	*err_blk)
{
    PSQ_OBJPRIV	    *priv_descr;
    DB_STATUS	    status;

    /* allocate the new descriptor */
    status = psf_malloc(sess_cb, mstream, sizeof(PSQ_OBJPRIV), (PTR *) &priv_descr, 
	err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    /* populate it */
    
    priv_descr->psq_privmap = privmap;
    priv_descr->psq_objtype = objtype;
    priv_descr->psq_objid.db_tab_base = objid->db_tab_base;
    priv_descr->psq_objid.db_tab_index = objid->db_tab_index;

    /* and insert it at the head of the privilege descriptor list */
    priv_descr->psq_next = *priv_list;
    *priv_list = priv_descr;

    return(E_DB_OK);
}

/*
** Name:    psy_insert_colpriv - insert a new element into a list of column
**				 privilege descriptors
**
** Description:
**	Given object (i.e. table or view) id, privilege map, and a map of
**	attributes on which privilege(s) are being sought, construct a new
**	privilege descriptor and insert it at the head of the column privilege
**	descriptor list.
**
** Input:
**	tabid				table id ptr (NULL if creating an
**					INFOLIST_HEADER)
**	objtype				object type:
**	    PSQ_OBJTYPE_IS_TABLE	element will contain a description of a
**					column-specific privilege
**	    PSQ_DBPLIST_HEADER		header of the list of privileges
**					associated with a dbproc
**	    PSQ_INFOLIST_HEADER		header of the list of privileges
**					associated with a failed attempt to
**					reparse a dbproc while processing
**					GRANT ON PROCEDURE
**	attrmap				map of attributes on which privilege is
**					required (NULL unless objtype is
**					PSQ_OBJTYPE_IS_TABLE)
**	privmap				privilege map
**	mstream				memory stream from which to allocate
**					memory for a new element
**	priv_list			address of the pointer to the first
**					element of the privilege descriptor list
**
** Output:
**	*priv_list			will point at the new descriptor
**	err_blk				will be filled in if an error is
**					encountered
**
** Exceptions:
**	none
**
** Side effects:
**	will allocate memory
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	13-sep-91 (andre)
**	    written
**	07-oct-91 (andre)
**	    added object type (objtype) to the interface;
**	    will copy attribute map only if the object type is
**	    PSQ_OBJTYPE_IS_TABLE;
**	07-oct-91 (andre)	<---    see next comment
**	    will copy object id only if the objtype is TABLE, DBPROC or
**	    DBPLIST_HEADER (i.e. if it is not INFOLIST_HEADER.)
**	11-oct-91 (andre)
**	    On second thought, there may be some utility in knowing that the
**	    following sublist may contain a PSQ_MISSING_PRIVILEGE descriptor
**	    which would explain why the dbproc is ungrantable;
**	    so the (07-oct-91 (andre)) should be disregarded, i.e. object id
**	    will be copied regardless of objtype
**	31-jan-92 (andre)
**	    attribute map will be stored for any PSQ_OBJTYPE_IS_TABLE element,
**	    even if it is also marked with PSQ_MISSING_PRIVILEGE
*/
DB_STATUS
psy_insert_colpriv(
	PSS_SESBLK	*sess_cb,
	DB_TAB_ID	*tabid,
	i4		objtype,
	register i4	*attrmap,
	i4		privmap,
	PSF_MSTREAM	*mstream,
	PSQ_COLPRIV	**priv_list,
	DB_ERROR	*err_blk)
{
    PSQ_COLPRIV	    *priv_descr;
    register i4	    *descr_attrmap;
    DB_STATUS	    status;

    /* allocate the new descriptor */
    status = psf_malloc(sess_cb, mstream, sizeof(PSQ_COLPRIV), (PTR *) &priv_descr, 
	err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    /* populate it */

    priv_descr->psq_privmap = privmap;
    priv_descr->psq_objtype = objtype;
    priv_descr->psq_tabid.db_tab_base = tabid->db_tab_base;
    priv_descr->psq_tabid.db_tab_index = tabid->db_tab_index;

    if (objtype & PSQ_OBJTYPE_IS_TABLE)
    {
	register i4     i;

	for (i = 0, descr_attrmap = priv_descr->psq_attrmap;
	     i < DB_COL_WORDS;
	     i++, descr_attrmap++, attrmap++
	    )
	{
	    *descr_attrmap = *attrmap;
	}
    }

    /* and insert it at the head of the privilege descriptor list */
    priv_descr->psq_next = *priv_list;
    *priv_list = priv_descr;

    return(E_DB_OK);
}

/*
** Name: psy_check_objprivs - in the course of parsing a dbproc, check the
**			      existing list of table-wide privileges to
**			      determine if the current user posesses (or does
**			      not posess) all/some of the required privileges
**
** Description:
**	While (re)parsing database procedures, we maintain a list of independent
**	privileges which the current user is known to posess and, if dbprocs are
**	being reparsed in the context of GRANT ON PROCEDURE, the list may also
**	contain privileges which the user is known to LACK.  This function is
**	responsible for scanning the list of privileges and determining which,
**	if any, privileges the current user posesses or does not posess
**
** Input:
**	required_privs		    addr of a map of required object (as opposed
**				    to column-specific) privileges
**	priv_list		    addr of the list of privileges which the
**				    current user is known to posess or lack
**	id			    id of a table privileges on which are being
**				    checked
**	mstream			    memory stream to be used in allocating a new
**				    privilege descriptor (if one must be
**				    allocated)
**	obj_type		    type of object
**	    PSQ_OBJTYPE_IS_TABLE    checking for table privileges
**	    PSQ_OBJTYPE_IS_DBEVENT  checking for dbevent privileges
**	    PSQ_OBJTYPE_IS_DBPROC   checking for dbproc privileges
**
** Output:
**	required_privs		if (!missing), map of privileges that still need
**				to be satisfied; otherwise, map of privileges
**				which the current user lacks
**	priv_descriptor		if (!missing) and some privileges are yet to be
**				satisfied and EITHER the current sublist
**				contained an entry for this table OR previous
**				sublist(s) contained an entry indicating that
**				the user posesses some of the required
**				privileges, this will contain ptr to the
**				descriptor which will be modified once we
**				determine that the user posesses the remaining
**				required privileges
**	priv_list		if a new descriptor was added, this will point
**				at it since new elements are added at the front
**				the list
**	missing			TRUE if the user is known to lack some of the
**				required privileges; FALSE otherwise
**	err_blk			filled in if an error occurs
**
** Returns
**	E_DB_{OK, ERROR}
**
** Side effects
**	may allocate memory
**
** History:
**	09-oct-91 (andre)
**	    written
**	29-jan-92 (andre)
**	    the function was violating the claim that each sublist will contain
**	    at most one descriptor per object if the the existing descriptor in
**	    the current sublist did not satisfy any of the required privileges;
**	    this has been corrected by setting priv_descriptor to point at such
**	    descriptor as long as it does not satisfy ALL of the required
**	    table-wide privileges
**	12-feb-92 (andre)
**	    rename psy_check_tblprivs() to psy_check_objprivs() and adapt it for
**	    checking privileges on objects besides tables/views
*/
DB_STATUS
psy_check_objprivs(
	PSS_SESBLK	*sess_cb,
	i4		*required_privs,
	PSQ_OBJPRIV	**priv_descriptor,
	PSQ_OBJPRIV     **priv_list,
	bool		*missing,
	DB_TAB_ID	*id,
	PSF_MSTREAM	*mstream,
	i4		obj_type,
	DB_ERROR	*err_blk)
{
    DB_STATUS	    status;
    i4		    privs_to_find;
    PSQ_OBJPRIV	    *priv_elem;
    i4		    end_of_sublist = PSQ_DBPLIST_HEADER | PSQ_INFOLIST_HEADER;

    /*
    ** first check if any object-wide privileges are required
    */
    if (!*required_privs)
    {
	return(E_DB_OK);
    }

    *missing = FALSE;
    *priv_descriptor = (PSQ_OBJPRIV *) NULL;

    /*
    ** NOTE: since dbprocs are only supported in SQL, we are guaranteed that
    ** (DB_TEST | DB_AGGREGATE | DB_RETRIEVE) is represented by DB_RETRIEVE in
    ** required_privs
    */

    /*
    ** scan the independent object-wide privilege sublist being constructed
    ** looking for an existing entry for this object
    ** Note that we guarantee that the current sublist will have AT MOST
    ** one object privilege entry for a given object
    */

    for (priv_elem = *priv_list;
	 (priv_elem && !(priv_elem->psq_objtype & end_of_sublist));
	 priv_elem = priv_elem->psq_next
	)
    {
	if (   priv_elem->psq_objtype & obj_type
	    && priv_elem->psq_objid.db_tab_base == id->db_tab_base
	    && priv_elem->psq_objid.db_tab_index == id->db_tab_index
	   )
	{
	    /*
	    ** first turn off the bits in required_privs which represent
	    ** privileges (if any) which have already been satisfied
	    */
	    *required_privs &= ~priv_elem->psq_privmap;

	    /*
	    ** if some privileges are yet to be satisfied, remember address of
	    ** the privilege list element which may be updated once it is known
	    ** whether the user posesses the remaining privileges
	    */
	    if (*required_privs)
		*priv_descriptor = priv_elem;

	    /*
	    ** skip past the remaining elements of the sublist since
	    ** we know that in the sublist corresponding to the
	    ** dbproc being reparsed there is at most one entry for
	    ** any given object
	    */
	    for (;
		 (priv_elem &&
		  !(priv_elem->psq_objtype & end_of_sublist));
		 priv_elem = priv_elem->psq_next
		)
	    ;

	    break;
	}
    }

    /*
    ** if we could not determine that the user posesses some of the
    ** privileges based on information found in the sublist being
    ** constructed and there are more sublists, check them for these
    ** privileges
    ** privs_to_find will hold a map of privileges which were yet to be
    ** satisfied before we scan through the previously constructed sublists; if
    ** at the end of the scan it is different from *required_privs, some of the
    ** privileges were satisfied based on informagtion found during the scan
    */
    if ((privs_to_find = *required_privs) && priv_elem)
    {
	i4	    priv;

	/*
	** at this point priv_elem points at the sublist header immediately
	** following the sublist being constructed
	*/
	for (; priv_elem; priv_elem = priv_elem->psq_next)
	{
	    if (   priv_elem->psq_objtype & obj_type
		&& priv_elem->psq_objid.db_tab_base == id->db_tab_base
		&& priv_elem->psq_objid.db_tab_index == id->db_tab_index
		&& (priv = priv_elem->psq_privmap & *required_privs)
	       )
	    {
		/*
		** if this is a map of privileges which the current user
		** does not posess, we will skip any further checking
		** and report error;
		** otherwise we will turn off bits in required_privs to indicate
		** that the privileges found in this element have been
		** satisfied
		*/
		if (priv_elem->psq_objtype & PSQ_MISSING_PRIVILEGE)
		{
		    *required_privs = priv;
		    *missing = TRUE;
		    return(E_DB_OK);
		}
		else
		{
		    *required_privs &= ~priv;

		    if (!(privs_to_find & *required_privs))
		    {
			/* user posesses all the required privileges */
			break;
		    }
		}
	    }

	    /*
	    ** if while scaning the remaining sublists we have discovered
	    ** that the user posesses some more of the required privileges,
	    ** we need to record this information
	    */
	    if (priv = privs_to_find & ~*required_privs)
	    {
		/*
		** at this point *required_privs contains privileges which are
		** yet to be satisfied and priv contains privileges which have
		** been determined to be satisfied based on descriptors found in
		** some other sublist(s)
		*/

		if (*priv_descriptor)
		{
		    /*
		    ** if the privilege sublist associated with a dbproc
		    ** being parsed already has an entry for this object,
		    ** modify it to include the required privileges
		    ** which we discovered to be posessed by the current
		    ** user
		    */
		    (*priv_descriptor)->psq_privmap |= priv;

		    if (!*required_privs)
		    {
			/*
			** strictly speaking, this is unnecessary, but we do
			** promise the caller priv_descriptor will be set ONLY
			** IF there are more object-wide privileges to be
			** satisfied
			*/
			priv_descriptor = (PSQ_OBJPRIV **) NULL;
		    }
		}
		else
		{
		    /*
		    ** now we will create a new list element which will
		    ** contain the map of privileges which the current owner
		    ** is known to posess
		    */

		    status = psy_insert_objpriv(sess_cb, id, obj_type, priv, mstream,
			priv_list, err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(status);
		    }

		    /*
		    ** privs_to_find will contain privileges which will be
		    ** searched for in IIPROTECT
		    */
		    if (*required_privs)
		    {
			/*
			** if we may discover that some other object-wide
			** privileges are posessed by the current user,
			** remember the entry which must be modified
			*/
			*priv_descriptor = *priv_list;
		    }
		}
	    }
	}
    }

    return(E_DB_OK);
}

/*
** Name: psy_check_colprivs - in the course of parsing a dbproc, check the
**			      existing list of column-specific privileges to
**			      determine if the current user posesses (or does
**			      not posess) a required column-specific privilege
**
** Description:
**	While (re)parsing database procedures, we maintain a list of independent
**	privileges which the current user is known to posess.  This function is
**	responsible for scanning the list of column-specific privileges and
**	determining if the current user posesses (or does not posess) a
**	specified privilege on a specified set of attributes
**
** Input:
**	required_priv		a required column-specific privilege
**	priv_list		addr of the list of privileges which the current
**				user is known to posess or lack
**	id			id of a table privileges on which are being
**				checked
**	attrmap			map of attributes on which the required_priv is
**				required
**	mstream			memory stream to be used in allocating a new
**				privilege descriptor (if one must be allocated)
**
** Output:
**	required_priv		0 if the required privilege on the specified set
**				of attributes was satisfied;
**	attrmap			if !*missing - map of attributes (if any) for
**				which we still need to determine if the user
**				posesses the required_priv; otherwise - a map of
**				attributes on which the user is known to LACK
**				required privilege
**	priv_descriptor		if !*mising and required_priv is non-zero, this
**				will contain ptr to the descriptor which will be
**				modified once we determine that the user
**				posesses the required_priv on the remaining
**				attributes
**	priv_list		if a new descriptor was added, this will point
**				at it since new elements are added at the front
**				the list
**	missing			TRUE if the user is known to lack the
**				required_priv on some of the attributes which
**				were passed in inside attrmap; FALSE otherwise
**	err_blk			filled in if an error occurs
**
** Returns
**	E_DB_{OK, ERROR}
**
** Side effects
**	may allocate memory
**
** History:
**	09-oct-91 (andre)
**	    written
**	31-jan-92 (andre)
**	    if a descriptor being scanned indicates that the user lacks the
**	    privilege on one or more of required attributes, we will AND the map
**	    found in the descriptor into the map passed by the caller and return
**	    it along with an indicator that the user lacks privileges on some of
**	    the attributes.
*/
static DB_STATUS
psy_check_colprivs(
	PSS_SESBLK	*sess_cb,
	register i4	*required_priv,
	PSQ_COLPRIV	**priv_descriptor,
	PSQ_COLPRIV     **priv_list,
	bool		*missing,
	DB_TAB_ID	*id,
	i4		*attrmap,
	PSF_MSTREAM	*mstream,
	DB_ERROR	*err_blk)
{
    register i4	i;

    *missing = FALSE;
    *priv_descriptor = (PSQ_COLPRIV *) NULL;

    /*
    ** One of the FIPS requirements is to recognize that
    **	UPDATE(col1, ..., coln)
    ** is equivalent to
    **	UPDATE(col1), ..., UPDATE(coln).
    ** (Once we start supporting column-specific INSERT, the same will apply to
    ** INSERT privilege).
    ** This algorithm incorporates this change (but more changes are
    ** required (in prochk() mostly) to fully implement this.)
    ** 
    ** In particular
    **
    **  if there exists an entry E in the sublist corresponding to the
    **  dbproc being parsed corresponding to this object and having
    **  *required_priv in psq_privmap
    **  {
    **	    if (attrmap is a subset of E->psq_attrmap)
    **	    {
    **		*required_priv = 0;
    **	    }
    **	    else
    **	    {
    **		attrmap -= E->psq_attrmap;
    **		** once we determine that the user posesses *required_priv on
    **		** the remaining columns in attrmap, attrmap will be merged into
    **		** E->psq_attrmap
    **	    }
    **  }
    */
    if (*required_priv)
    {
	PSQ_COLPRIV	    *col_priv;
	i4		    end_of_sublist =
				PSQ_DBPLIST_HEADER | PSQ_INFOLIST_HEADER;

	for (col_priv = *priv_list;
	     (col_priv && !(col_priv->psq_objtype & end_of_sublist));
	     col_priv = col_priv->psq_next
	    )
	{
	    if (   col_priv->psq_tabid.db_tab_base == id->db_tab_base
		&& col_priv->psq_tabid.db_tab_index == id->db_tab_index
		&& col_priv->psq_privmap == *required_priv
	       )
	    {
		if (BTsubset((char *) attrmap,
			(char *) col_priv->psq_attrmap, DB_MAX_COLS + 1))
		{
		    *required_priv = 0;
		}
		else
		{
		    /*
		    ** remember the element which will have to be modified
		    ** if it is discovered that the current user posesses
		    ** DB_REPLACE on the remaining columns
		    */
		    *priv_descriptor = col_priv;
		    
		    for (i = 0; i < DB_COL_WORDS; i++)
		    {
			attrmap[i] &= ~col_priv->psq_attrmap[i];
		    }

		    /* skip to the end of the current sublist */
		    for (;
			 (col_priv &&
			  !(col_priv->psq_objtype & end_of_sublist));
			 col_priv = col_priv->psq_next
			)
		    ;
		}

		break;
	    }
	}

	if (col_priv && *required_priv)
	{
	    PSY_ATTMAP	    savemap;
	    i4		    *map_ptr;
	    bool	    more_attrs = FALSE;

	    /*
	    ** if a privilege descriptor has been found in the current list,
	    ** newly found attributes will be merged into it; otherwise, we will
	    ** construct a map which will be used in constructing a new
	    ** descriptor
	    */
	    if (*priv_descriptor)
	    {
		map_ptr = (*priv_descriptor)->psq_attrmap;
	    }
	    else
	    {
		psy_fill_attmap(map_ptr = savemap.map, ((i4) 0));
	    }

	    /*
	    ** now walk down the rest of the list trying to determine if the
	    ** user posesses the required privileges
	    */
	    for (; col_priv; col_priv = col_priv->psq_next)
	    {
		if (   col_priv->psq_objtype & PSQ_OBJTYPE_IS_TABLE
		    && col_priv->psq_tabid.db_tab_base == id->db_tab_base
		    && col_priv->psq_tabid.db_tab_index == id->db_tab_index
		    && col_priv->psq_privmap == *required_priv
		   )
		{
		    /*
		    ** if the descriptor represents privilege which
		    ** the user does not posess and any of the
		    ** attributes on which the privilege is required
		    ** appear in the descriptor, we need not look
		    ** any further.
		    */
		    if (col_priv->psq_objtype & PSQ_MISSING_PRIVILEGE)
		    {
			for (i = 0; i < DB_COL_WORDS; i++)
			{
			    /*
			    ** if we determine that the user lacks privilege on
			    ** at least one of the attributes, we will finish
			    ** processing the current descriptor and return to
			    ** the caller a map of attributes for which he is
			    ** known to lack the required privilege inside
			    ** attrmap
			    */
			    if (attrmap[i] & col_priv->psq_attrmap[i])
			    {
				if (!*missing)
				{
				    *missing = TRUE;
				    psy_fill_attmap(savemap.map, ((i4) 0));
				}

				savemap.map[i] =
				    attrmap[i] & col_priv->psq_attrmap[i];
			    }
			}

			if (*missing)
			{
			    for (i = 0; i < DB_COL_WORDS; i++)
				attrmap[i] = savemap.map[i];

			    return(E_DB_OK);
			}
		    }
		    else
		    {
			for (i = 0; i < DB_COL_WORDS; i++)
			{
			    if (attrmap[i] & col_priv->psq_attrmap[i])
			    {
				/*
				** remember that we discovered some additional
				** attributes for which the user posesses the
				** colspecific_priv
				*/
				more_attrs = TRUE;

				map_ptr[i] |=
				    (attrmap[i] & col_priv->psq_attrmap[i]);

				attrmap[i] &= ~map_ptr[i];
			    }
			}
		    }
		}
	    }

	    /*
	    ** if while scaning the remaining sublists we have discovered
	    ** that the user posesses DB_REPLACE on some additional columns,
	    ** we need to record this information
	    **
	    ** At the same time try to determine if by now we have
	    ** determined that the user posesses all the required privileges
	    */
	    if (more_attrs)
	    {
		if (!*priv_descriptor)
		{
		    /*
		    ** at this point savemap contains a subset (possibly
		    ** improper) of required attributes for which the user
		    ** posesses colspecific_priv.
		    **
		    ** now we will create a new list element which will
		    ** contain the map of attributes for which the user
		    ** posesses colspecific_priv
		    */
		    DB_STATUS			status;

		    status = psy_insert_colpriv(sess_cb, id, PSQ_OBJTYPE_IS_TABLE,
			savemap.map, *required_priv, mstream, priv_list,
			err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			return(status);
		    }
		}

		/*
		** check if we have determined that the user posesses all
		** required privileges
		*/
		for (i = 0; (i < DB_COL_WORDS && !attrmap[i]); i++)
		;

		/*
		** if the are no more attributes for which we need to
		** determine if the user posesses DB_REPLACE privilege, we
		** no longer need to check for that privilege
		*/
		if (i == DB_COL_WORDS)
		{
		    *required_priv = 0;
		}
		else if (!*priv_descriptor)
		{
		    /*
		    ** if we are yet to discover that the current user posesses
		    ** the required_priv on some remaining attributes, the entry
		    ** which we have just created may need to be modified - so
		    ** we will return its address to the caller
		    */
		    *priv_descriptor = *priv_list;
		}
	    }
	}
    }
    return(E_DB_OK);
}
