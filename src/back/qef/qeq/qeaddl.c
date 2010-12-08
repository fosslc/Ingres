/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <usererror.h>
#include    <bt.h>
#include    <cm.h>
#include    <cv.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <ddb.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>	
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>
#include    <qefscb.h>
#include    <rdf.h>

/**
**
**  Name: QEADDL.C - perform a DDL action item
**
**  Description:
**      DDL action items are performed by the routines in this file. 
**
**          qea_createIntegrity - Create an integrity.
**	    qea_cview		- create a view
**
**
**  History:    $Log-for RCS$
**	03-dec-92 (rickh)
**	    Created.
**      24-feb-93 (rickh)
**          Made qea_reserveID external.
**	28-feb-93 (andre)
**	    Added qea_cview()
**      08-mar-93 (anitap)
**          Removed break in qea_cop_text() and qea_cor_text() in for loop.
**	14-mar-93 (andre)
**	    added qea_d_integ()
**	15-mar-93 (rickh)
**	    Rototill in delimited identifier support.
**	29-mar-93 (rickh)
**	    Strip semi-colons from the end of CREATE PROCEDURE text since the
**	    parser gags on them.
**	31-mar-93 (rickh)
**	    Turn off execute immediate processing in this module.  This will
**	    prevent later actions from thinking that they are processing inside
**	    an execute immediate context.
**	2-apr-93 (rickh)
**	    Use tableID/integrityNumber instead of constraintID in
**	    IIDBDEPENDS tuples.
**      06-apr-1993 (smc)
**          Hush the Alpha compiler.
**	15-apr-93 (rickh)
**	    Use user specified constraint name when reporting errors.  Treat
**	    internal constraint name as blank padded, not null terminated.
**	09-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use qef_cat_owner instead of "$ingres" in lookupSchemaID()
**	11-jun-93 (rickh)
**	    When calling qeu_cintg, make sure the returned error is
**	    correctly mapped into a qef_rcb.
**      07-jul-93 (anitap)
**          Added new argument err to qea_reservID() and changed qef_rcb
**          to qef_cb.
**	    Removed tmp_dsh from qea_scontext() in setupForExecuteImmediate().
**	09-jul-93 (rickh)
**	    Made some function calls agree with their prototypes.
**	22-jul-93 (anitap)
**	    Added a function argument to qea_chkerror() in qea_cview(), 
**	    qea_uniqueConstraint(), qea_checkConstraint(), 
**	    qea_referentialConstraint().
**	    Added explicit calls to qea_chkerror() in 
**	    qea_referentialConstraint() so that the E.I. query text objects can
**	    be destroyed.
**	    Added new argument to qea_cobj() in setupForExecuteImmediate().
**	22-jul-93 (rickh)
**	    Added a trace point to dump execute immediate text to the
**	    terminal.
**	28-jul-93 (rblumer)
**	    in qea_d_integ: added check for NOT NULL constraints implemented as
**	    non-nullable datatypes--we don't allow them to be dropped;
**	    also made sure to set status to E_DB_ERROR for some errors, as
**	    otherwise they are not reported to the user.
**	    Also removed many unused variables that the compiler pointed out.
**	2-aug-93 (rickh)
**	    Case translation semantics became a pointer in the qef_cb.
**	2-aug-93 (rickh)
**	    Use the session control block to remember when we're in the middle
**	    of creating a unique constraint.  This enables the dbu code to
**	    emit a different error message if a duplicate error occurs.
**      06-aug-93 (anitap)
**          Added includes of <qefprotos.h>, <rqf.h>, <tm.h>, <ex.h>, etc. which
**	    were missing and causing compiler errors.
**	    Also cast rdf_call() in qea_packTuples() to get rid of compiler
**	    warnings.
**	    Fixed compiler warning in dunpTextToScreen().
**	    Initialize qef_cb->qef_dbid in qea_createIntegrity().
**	9-aug-93 (rickh)
**	    Added a new state to the REFERENTIAL INTEGRITY state machine:
**	    cook up and execute a SELECT statement when adding a REF CONSTRAINT
**	    to a pre-existing table.  Added flags argument to qea_chkerror.
**	20-aug-93 (rickh)
**	    When creating a NOT NULL integrity, make sure you close the
**	    base table first.  This is because the table may have been
**	    created with a CREATE TABLE AS SELECT statement and the SELECT
**	    may have left the table open.  A DMF ALTER TABLE call downstream
**	    in qeu_cintg will hang if the table is open.
**	25-aug-93 (anitap)
**	    Set qef_callbackStatus for EXECUTE_SELECT_STATEMENT state in 
**	    qea_referentialConstraint().
**	9-aug-93 (rickh)
**	    Added new states to the CHECK CONSTRAINT state machine to cook up
**	    and execute a query for verifying the CHECK CONSTRAINT at ALTER
**	    TABLE ADD CHECK CONSTRAINT time.
**	6-sep-93 (stephenb)
**	    Creation and deletion of integrities (through ALTER TABLE) is an
**	    auditable action, added calls to qeu_secaudit() in appropriate 
**	    places.
**	13-sep-93 (rickh)
**	    Added WITH PERSISTENCE to the text of the index that's created
**	    on the foreign key in a referring table.
**	11-oct-93 (rickh)
**	    When opening IISCHEMA for scanning, ask for page rather than table
**	    locks.  The shared table level lock was locking out other schema
**	    creators.
**	27-oct-93 (rickh)
**	    Set QEF_EXECUTE_SELECT_STMT after we finish the parsing state
**	    of the SELECT that supports an ALTER TABLE ADD CONSTRAINT
**	    so that QEQ_QUERY will know not to reset the qef_output buffer.
**	    Also, successful completions of the security audit call were
**	    overwriting the status returned from qea_createIntegrity so I
**	    fixed this.
**	29-oct-93 (stephenb)
**	    Initialize qeuq_cb.qeuq_tabname in qea_writeIntegrityTuples() 
**	    before calling qeu_cintg(), we need it to write an audit record.
**	22-dec-93 (rickh)
**	    Fix the CONSTRAINT name generator to work on double-byte systems.
**	28-dec-93 (rickh)
**	    Fixed populateIIKEY to properly record and return errors.
**	7-jan-94 (robf)
**          Add new audit parameters to qeu_secaudit(). Still need
**	    to add security label info for table.
**	14-jan-94 (rickh)
**	    Changed REFERENTIAL CONSTRAINT error messages.
**	22-dec-95 (pchang)
**	    In the Left Outer Join Select statement cooked-up in textOfSelect()
**	    to verify that the foreign key referential constraint holds on a
**	    pre-existing table, do not attempt to match NULL value(s) in the
**	    column(s) of the Referencing table to other values in the
**	    corresponding column(s) of the Referenced table (B65876).
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	1-apr-98 (inkdo01)
**	    Added index option support to constraint definitions.
**	2-Oct-1998 (rigka01) bug #93612
**	    In populateIIKEY,
**	    set next pointer in last QEF_DATAarray not in non-element after
**	    next;  bug introduced ULM memory corruption when running
**	    alter table ... add constraint ... FOREIGN KEY ... REFERENCES
**	    which surfaced as GPF here or later in ULM code (e.g.
**	    ulm_closestream, ulm_free) 
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Jan-2001 (jenjo02)
**	    Fix memory leak in qea_d_integ().
**	    If RDF fails to get (QEF) memory, garbage collect, 
**	    then retry.
**	26-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	26-jan-2001 (hweho01)
**	    Changed  variable name restrict into iirestrict, avoid conflict  
**          with the compiler key/reserved word on axp_osf platform.
**      18-Apr-2001 (hweho01)
**          Removed the unnecessary qef_error() call after qeu_secaudit();
**          since the message would has been issued if an error occurred
**          in qeu_secaudit_detail(), and the e_error block is a mismatch 
**          contains no detail informations.
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constaints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**      09-jan-2002 (stial01)
**          Don't validate check constraints from upgradedb (SIR 106745)
**      17-dec-2002 (stial01)
**          Fix bypass of verify constraint from upgradedb
**	27-sep-2002 (gygro01) b108127 / ingsrv1817
**            For referential constraint with QCI_BASE_INDEX flag set, it is
**            not needed to lookup tabid in LinkRefObjects and update the
**            index reference in iidbdepends as there is none.
**      09-jan-2002 (stial01)
**          Removed QEF_RUNNING_UPGRADEDB bypass of verify check constraint, 
**          since upgradedb no longer drops and recreates check constraints
**          to fix associated system generated rule query trees. 
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	12-may-04 (inkdo01)
**	    Changed dbproc declarations of "row_tid i4" to "row_tid i8"
**	    For compatibility with r3 partitioned table changes.
**	16-dec-04 (inkdo01)
**	    Added a few collID's.
**	08-aug-2005 (toumi01)
**	    Fix totalLength++ for pass 0 for PREPEND_A_CLOSEPAREN so that
**	    we compute the correct db_t_count and avoid risk of string
**	    trunctation and inaedquate string allocation.
**	9-Dec-2005 (kschendel)
**	    Clean out obsolete and wrong i2 casts on mecopy.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	11-Apr-2008 (kschendel)
**	    new style dmf-level row qualification.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	19-Apr-2010 (gupsh01) SIR 123444
**	    Added support for rename table /columns.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning / prototype fixes.  CMcpychar is not to be used with
**	    single character constant strings, causes "out of array bounds"
**	    errors, and is unnecessary.
**/

/*	definitions needed by the static function declarations	*/

/*	these structures facilitate building strings from fragments */

typedef struct _EVOLVING_STRING	{

	i4		pass;	/* 2 passes now. pass 0 and pass 1 */
#define	TEXT_COMPILATION_PASSES		2

	u_i2		length;

	DB_TEXT_STRING	*textString;

}	EVOLVING_STRING;

/* for defining the CHECK and REFERENTIAL CONSTRAINT state machines */

struct	state_table_entry	{
	i4		type;
	DB_STATUS	( *textMaker )(
				QEF_AHD		*qea_act,
				QEE_DSH		*dsh,
				DB_TEXT_STRING	**text
			);
	i4		chkerrorFlags;	/* flags to pass to qea_chkerror */
	u_i4	executionFlags;	/* PST_INFO flags to PSF */
};

/* postfix types for stringInColumnList */

#define	NO_POSTFIX		0
#define	DATATYPES		1
#define	IS_NOT_NULL		2
#define	IS_NULL			3
#define SET_NULL		4

/*	user for unnormalizing object names	*/

#define	UNNORMALIZED_NAME_LENGTH	( ( 2 * DB_MAXNAME ) + 2 )

#define	ADD_UNNORMALIZED_NAME( evolvingString, name, nameLength, punctuation, singleQuote )\
	status = addUnnormalizedName( name, nameLength, unnormalizedName, \
				&unnormalizedNameLength, evolvingString, \
				punctuation, singleQuote, dsh );	\
	if ( status != E_DB_OK )	{  break; }

/*	for selecting between user specified and constructed constraint name */

#define	GET_CONSTRAINT_NAME( )	\
	( ( details->qci_flags & QCI_CONS_NAME_SPEC ) ? \
	  ( char * ) &details->qci_integrityTuple->dbi_consname :	\
	  internalConstraintName )


/*	unique character built into procedure and rule names	*/

#define	INSERT_REFING_PROCEDURE_ID	'1'
#define	UPDATE_REFING_PROCEDURE_ID	'2'
#define	DELETE_REFED_PROCEDURE_ID	'3'
#define	UPDATE_REFED_PROCEDURE_ID	'4'

#define	INSERT_REFING_RULE_ID		'1'
#define	UPDATE_REFING_RULE_ID		'2'
#define	DELETE_REFED_RULE_ID		'3'
#define	UPDATE_REFED_RULE_ID		'4'


/*	static functions	*/

static DB_STATUS
qea_constructConstraintName(
    DB_TAB_ID	*seed,
    char	*tableName,
    i4		constraintType,
    char	*constraintName,
    QEE_DSH	*dsh
);

static DB_STATUS
qea_co_obj_name(
    DB_TAB_ID            *view_id,
    DB_TAB_NAME          *view_name,
    char                 *obj_type,
    char                 *obj_name,
    QEE_DSH		 *dsh
);

static DB_STATUS
qea_cor_text(
	QEE_DSH		*dsh,
	QEF_CREATE_VIEW	*crt_view,
	DMT_TBL_ENTRY	*tbl_info,
	DB_NAME		*rule_name,
	DB_TEXT_STRING	**rule_text);

static DB_STATUS
qea_cop_text(
	QEE_DSH         *dsh,
	QEF_CREATE_VIEW *crt_view,
	DB_TEXT_STRING	**dbp_text);

static VOID
seedToDigits(
    DB_TAB_ID	*seed,
    char	*digitString
);

static DB_STATUS
qea_readAttributes(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DMT_TBL_ENTRY	*tableinfo,
    DMT_ATT_ENTRY	***attributeArray );

static DB_STATUS
qea_checkConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh );

static DB_STATUS
qea_referentialConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh );

static DB_STATUS
setupStateMachine( 
    QEF_AHD			*qea_act,
    QEE_DSH			*dsh,
    struct state_table_entry	*stateTable,
    i4				numberOfPossibleStates,
    struct state_table_entry	**returnedStateTableEntry
);

static	DB_STATUS
grabResources(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    i4			initializeOrCleanup
);

static	DB_STATUS
exclusiveLock(
    DB_TAB_ID		*tableID,
    QEE_DSH		*dsh
);

static DB_STATUS
qea_uniqueConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh );

static DB_STATUS
linkRefObjects(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh );

static DB_STATUS
qea_record_co_dbp_dependency(
    QEF_AHD             *qea_act,
    QEE_DSH		*dsh);

static DB_STATUS
initString(
    EVOLVING_STRING	*evolvingString,
    i4			pass,
    QEE_DSH		*dsh,
    ULM_RCB		*ulm );

static VOID
addString(
    EVOLVING_STRING	*evolvingString,
    char		*subString,
    i4			subStringLength,
    i4			punctuation,
    i4			addQuotes );

static VOID
endString(
    EVOLVING_STRING	*evolvingString,
    DB_TEXT_STRING	**resultString );

static DB_STATUS
textVerifyingCheckConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**ruleText );

static DB_STATUS
makeCheckConstraintRuleText(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**ruleText );

static DB_STATUS
stringInColumnList(
    QEE_DSH		*dsh,
    PST_COL_ID		*columnList,
    DMT_ATT_ENTRY	**attributeArray,
    EVOLVING_STRING	*evolvingString,
    ULM_RCB		*ulm,
    i4			postfix,
    i4			maxNumberOfColumns,
    char		*columnNameSeed,
    char		*prefix,
    i4			listSeparator );

static DB_STATUS
addDataType(
    QEE_DSH		*dsh,
    DMT_ATT_ENTRY	*currentAttribute,
    EVOLVING_STRING	*evolvingString,
    ADI_FI_DESC		**fi2ptr );

static DB_STATUS
compareColumnLists(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    PST_COL_ID		*leftTableColumnList,
    DMT_ATT_ENTRY	**leftTableAttributeArray,
    PST_COL_ID		*rightTableColumnList,
    DMT_ATT_ENTRY	**rightTableAttributeArray,
    EVOLVING_STRING	*evolvingString,
    char		*leftTableName,
    char		*rightTableName,
    char		*leftColumnNameSeed,
    char		*rightColumnNameSeed,
    i4			listSeparator,
    char		*comparisonOperator );

static DB_STATUS
qea_0lookupTableInfo(
    QEE_DSH		*dsh,
    bool		by_name_and_owner,
    DB_TAB_NAME		*tableName,
    DB_OWN_NAME		*tableOwner,
    DB_TAB_ID		*tableID,
    DMT_TBL_ENTRY	*tableInfo,
    bool		reportErrorIfNotFound );

static DB_STATUS
qea_lookupTableID(
    QEE_DSH	*dsh,
    DB_TAB_NAME	*tableName,
    DB_OWN_NAME	*tableOwner,
    DB_TAB_ID	*tableID,
    i4	*attributeCount,
    bool	reportErrorIfNotFound );

static DB_STATUS
qea_writeIntegrityTuples(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_INTEGRITY	*tuple1,
    DB_INTEGRITY	*tuple2 );

static DB_STATUS
qea_closeTable(
    QEE_DSH		*dsh,
    DB_TAB_ID		*tableID
);

static DB_STATUS
qea_packTuples(
    ULM_RCB		*ulm,
    QEE_DSH		*dsh,
    QEUQ_CB		*qeuq_cb,
    DB_TAB_ID		*tableID,
    PTR			queryText,
    i4			queryTextLength,
    PTR			queryTree,
    i4			viewStatus );

static DB_STATUS
makeIIDBDEPENDStuple(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DB_TAB_ID		*independentObjectID,
    i4			independentSecondaryID,
    i4			independentObjectType,
    DB_TAB_ID		*dependentObjectID,
    i4			dependentSecondaryID,
    i4			dependentObjectType,
    DB_IIDBDEPENDS	**returnedTuple
);

static DB_STATUS
writeIIDBDEPENDStuples(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DB_IIDBDEPENDS	**tuples,
    i4		tupleCount
);

static DB_STATUS
lookupProcedureIDs(
    QEE_DSH		*dsh,
    DB_OWN_NAME		*owner,
    char		**procedureNames,
    i4			count,
    DB_TAB_ID		*IDs );

static DB_STATUS
lookupRuleIDs(
    QEE_DSH		*dsh,
    DB_TAB_ID		*tableID,
    DB_NAME		**ruleNames,
    i4			count,
    DB_TAB_ID		*IDs );

static DB_STATUS
setupForExecuteImmediate(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    PST_INFO		*qea_info,
    DB_TEXT_STRING	*text,
    u_i4		executionFlags
);

static VOID
dumpTextToScreen(
    DB_TEXT_STRING	*text
);

static DB_STATUS
textOfIndexOnRefingTable(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
doesTableWithConsNameExist(
    QEE_DSH		*dsh,
    char		*constraintName,
    DB_OWN_NAME		*owner,
    char		*preferredIndexName,
    bool		*indexNameUsed );

static DB_STATUS
textOfSelect(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfInsertRefingProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfUpdateRefingProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textsOfModifyRefingProcedures(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text,
    char		*procedureName,
    char		procedureIdentifier,
    i4			errorNumber
);

static DB_STATUS
textOfDeleteRefedProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfDeleteCascadeRefedProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfDeleteSetNullRefedProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfUpdateRefedProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfUpdateCascadeRefedProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfUpdateSetNullRefedProcedure(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfInsertRefingRule(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfUpdateRefingRule(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfDeleteRefedRule(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfUpdateRefedRule(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static DB_STATUS
textOfUniqueIndex(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text );

static VOID
addWithopts(
    EVOLVING_STRING	*evolvingString,
    QEF_CREATE_INTEGRITY_STATEMENT	*details);

static DB_STATUS
populateIIKEY(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DB_TAB_ID		*constraintID,
    PST_COL_ID		*keyList );

static DB_STATUS
lookupSchemaID(
    QEE_DSH		*dsh,
    DB_SCHEMA_NAME	*schemaName,
    DB_SCHEMA_ID	*schemaID
);

static DB_STATUS
reportIntegrityViolation(
    EVOLVING_STRING	*evolvingString,
    char		*constraintType,
    char		*constraintName,
    char		*tableName,
    QEE_DSH		*dsh
);

static DB_STATUS
reportRefConsViolation(
    EVOLVING_STRING	*evolvingString,
    QEE_DSH		*dsh,
    i4			errorNumber,
    char		*activeTableName,
    char		*passiveTableName,
    char		*constraintName
);

static	DB_STATUS
addUnnormalizedName(
    char		*name,
    i4			nameLength,
    char		*unnormalizedName,
    i4			*unnormalizedNameLength,
    EVOLVING_STRING	*evolvingString,
    i4			punctuation,
    i4			singleQuote,
    QEE_DSH		*dsh
);

static DB_STATUS
qea_dep_cons(
	void		*dummy,
	QEU_QUAL_PARAMS	*qparams);

static VOID
makeObjectName(
    char	*nameSeed,
    char	postfixCharacter,
    char	*targetName
);

/* other structures */

/* for manipulating the CHECK and REFERENTIAL CONSTRAINT state machines */

#define	ADVANCE_TO_NEXT_STATE	{		\
	    stateTableEntry++;			\
	    *state = stateTableEntry->type;	}


/* useful boiler plate */

#define	QEF_ERROR( errorCode )	\
            error = errorCode;	\
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );

#define	BEGIN_CODE_BLOCK	for ( ; ; ) {
#define	EXIT_CODE_BLOCK		break;
#define	END_CODE_BLOCK		break; }


/*{
** Name: qea_constructConstraintName - construct a constraint name
**
** Description:
**	This routine constructs a constraint name based on a seed number and the
**	type of constraint.  The constraint name will be blank padded to
**	DB_MAXNAME positions.
**
**	The general format for names is:
**
**		$<tabname>_?<id>
**
**	where
**		tabname		is the first 5 chars of the table name
**		?		is a char (e.g., U for unique constraint)
**		id		is an ascii encoding of the seed number
**
** Inputs:
**	seed		number used to construct name
**	tableName	name of table constraint is on
**	constraintType	type of constraint to name
**
** Outputs:
**	constraintName		constructed name
**
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	15-apr-93 (rickh)
**	    Blank pad the name to DB_MAXNAME positions.
**	2-aug-93 (rickh)
**	    Case translation semantics became a pointer in the qef_cb.
**
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	22-dec-93 (rickh)
**	    Fix the CONSTRAINT name generator to work on double-byte systems.
*/



/*	these are indices into the constraintTypes array	*/

#define	UNIQUE_CONSTRAINT		0
#define	CHECK_CONSTRAINT		1
#define	REFERENTIAL_CONSTRAINT		2

static	char	constraintTypes[ ] =	{
	'U',			/* unique constraint */
	'C',			/* check constraint */
	'R'			/* referential constraint */
};

#define	TABLE_STUB_LENGTH	5

static DB_STATUS
qea_constructConstraintName(
    DB_TAB_ID	*seed,
    char	*tableName,
    i4		constraintType,
    char	*constraintName,
    QEE_DSH	*dsh
)
{
    DB_STATUS	status = E_DB_OK;
    i4		j;
    char	digitString[ 20 ], *digitStringPtr = digitString;
    u_i4	caseTranslationFlags =
		 ( * ( dsh->dsh_qefcb->qef_dbxlate ) ) | CUI_ID_DLM | CUI_ID_NORM;
    u_i4	translationReturnMode;
    u_char	untranslatedString[ DB_MAXNAME + 1 ];
    char	*p = ( char * ) untranslatedString;
    u_i4	untranslatedStringLength, translatedStringLength;
    char	*limit;

    *p++ = '$';
    for ( j = 0; j < TABLE_STUB_LENGTH; j++ )
    {	CMcpyinc( tableName, p ); }
    *p++ = '_';
    *p++ = constraintTypes[ constraintType ];

    /* turn CONSTRAINT ID into a string of hex digits */

    seedToDigits( seed, digitString );
    for ( ; *digitStringPtr != EOS; CMcpyinc( digitStringPtr, p ) ) { ; }

    /*
    ** now force name to the correct case for this session.
    ** cui_idxlate returns E_DB_WARN if translation occurred but the semantics
    ** were a little fuzzy.  that's OK (e.g., '$' isn't legal in ANSI names but
    ** is legal for INGRES system objects).
    */

    translatedStringLength =
    untranslatedStringLength = p - (char *) untranslatedString;

    status = cui_idxlate( untranslatedString,
			  &untranslatedStringLength,
			  ( u_char * ) constraintName,
			  &translatedStringLength,
			  caseTranslationFlags,
			  &translationReturnMode,
			  &dsh->dsh_error );
    if ( status == E_DB_ERROR ) { return( status ); }
    else { status = E_DB_OK; }

    /* blank pad */
    p = constraintName + translatedStringLength;
    limit = constraintName + DB_MAXNAME;
    while (p < limit)
	*p++ = ' ';

    return( status );
}	/* qea_constructConstraintName */

/*
** Name: qea_co_obj_name - construct name of an object used to enforce 
**			   CHECK OPTION on a view
**			   
** Description:
**	This function will use name and id of the view and a string describing 
**	type of an object to construct name of that object.  
**	Name will consist of a $ followed by first 5 chars of the view name
**	(blank padded if necessary), followed by _COR_ for a rule or _COP_ for a
**	dbproc, followed by the hex representation of the view id.  Before
**	returning to the caller, we will call cui_idxlate() to convert name to
**	the case appropriate for the database to which we are connected.
**
** Input:
**	view_id			id of the view
**	view_name		name of the view
**	obj_type		"P" if asked to build a dbproc name, "R" is 
**				asked to build a rule name
**
** Output
**	obj_name		object name constructued as described above
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects
**	none
**
** History:
**	01-mar-93 (andre)
**	    written
**	12-apr-93 (andre)
**	    use cui_idxlate() to ensure that the name is represented using a
**	    correct case for the current database
**	2-aug-93 (rickh)
**	    Case translation semantics became a pointer in the qef_cb.
*/
static DB_STATUS
qea_co_obj_name(
    DB_TAB_ID            *view_id,
    DB_TAB_NAME          *view_name,
    char                 *obj_type,
    char                 *obj_name,
    QEE_DSH		 *dsh
)
{
    DB_STATUS	status = E_DB_OK;
    u_i4	cui_flags =
		   ( * ( dsh->dsh_qefcb->qef_dbxlate ) ) | CUI_ID_DLM | CUI_ID_NORM;
    u_i4   ret_mode;
    u_i4	len_untrans, len_trans = DB_MAXNAME;
    u_char	untrans_str[DB_MAXNAME];
    i4		j;
    char	*vname = view_name->db_tab_name;
    char	*p = (char *) untrans_str;
    char	*limit;
    char	id_str[20], *id_p = id_str;

    *p++ = '$';
    for (j = 0; j < TABLE_STUB_LENGTH; j++)
	CMcpyinc(vname, p);
    *p++ = '_';
    *p++ = 'C';
    *p++ = '0';
    *p++ = *obj_type;
    *p++ = '_';

    /* convert view id into a hex string */
    seedToDigits(view_id, id_str);

    /* append id string to the name built so far */
    for (; *id_p != EOS; CMcpyinc(id_p, p))
    ;

    /* remember length of the name */
    len_untrans = p - (char *) untrans_str;

    status = cui_idxlate(untrans_str, &len_untrans, (u_char *) obj_name,
	&len_trans, cui_flags, &ret_mode, &dsh->dsh_error);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* blank pad */
    p = obj_name + len_trans;
    limit = obj_name + DB_MAXNAME;
    while (p < limit)
	*p++ = ' ';

    return(E_DB_OK);
}	/* qea_co_obj_name */

/*{
** Name: seedToDigits - turn a DB_TAB_ID into a hex number
**
** Description:
**	This routine converts a DB_TAB_ID into a hex number.  This is called
**	by the constraint name builder.
**
**
** Inputs:
**	seed		number used to construct name
**
** Outputs:
**	digitString	where to write the string of hex digits
**
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	19-mar-93 (rickh)
**	    Apply prevailing case translation semantics.
**
*/

#define	DIGITS_IN_I4	8
#define	STRING_LENGTH	( 2 * DIGITS_IN_I4 )

static VOID
seedToDigits(
    DB_TAB_ID	*seed,
    char	*digitString
)
{
    i4		leadingZeroes;
    i4		firstStringLength;
    i4		secondStringLength;
    char	firstString[ DIGITS_IN_I4 + 1 ];
    char	secondString[ DIGITS_IN_I4 + 1 ];

    CVlx( seed->db_tab_base, firstString );
    firstString[ DIGITS_IN_I4 ] = 0;	/* just in case */
    firstStringLength = STlength( firstString );

    CVlx( seed->db_tab_index, secondString );
    secondString[ DIGITS_IN_I4 ] = 0;	/* just in case */
    secondStringLength = STlength( secondString );

    leadingZeroes = DIGITS_IN_I4 - firstStringLength;
    STmove( "", '0', leadingZeroes, digitString );
    STcopy( firstString, &digitString[ leadingZeroes ] );

    leadingZeroes = DIGITS_IN_I4 - secondStringLength;
    STmove( "", '0', leadingZeroes, &digitString[ DIGITS_IN_I4 ] );
    STcopy( secondString, &digitString[ DIGITS_IN_I4 + leadingZeroes ] );
}

/*{
** Name: QEA_CREATEINTEGRITY - Create an Integrity.
**
** Description:
**	This routine will create an Integrity.  Currently, it only creates
**	Constraint Integrities.
**
**
**
**  qea_createIntegrity will log any appropriate errors. 
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE1996_BAD_ACTION
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	27-mar-93 (andre)
**	    when creating a REFERENCES constraint, pass a description of the
**	    UNIQUE or PRIMARY CONSTRAINT and, if the owner of the <referenced
**	    table> is different from that of the <referencing table>, of
**	    privilege on which the constraint depends to qeu_cintg() which will
**	    enter them into IIDBDEPENDS and IIPRIV
**	31-mar-93 (rickh)
**	    Turn off execute immediate processing in this module.  This will
**	    prevent later actions from thinking that they are processing inside
**	    an execute immediate context.
**	22-jul-93 (anitap)
**	    Got rid of commented out code (not needed).
**	19-aug-93 (anitap)
**	    Initialize qef_cb->qef_dbid.
**	9-aug-93 (rickh)
**	    Added initialState variable.
**	6-sep-93 (stephenb)
**	    Added call to qeu_secaudit() to audit successfull creation of 
**	    integrity.
**	9-sep-93 (rickh)
**	    Set initialState for ALTER TABLE ADD CHECK CONSTRAINT.
**	27-oct-93 (rickh)
**	    Successful completions of the security audit call were
**	    overwriting the status returned from qea_createIntegrity so I
**	    fixed this.
**	18-apr-94 (rblumer)
**	    changed qci_integrityTuple2 to qci_knownNotNullableTuple.
**	16-aug-99 (inkdo01)
**	    Encode referential actions into iiintegrity tuple.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
[@history_line@]...
*/

/*
**	Values that the state word can take:
*/

#define	INITIALIZE			0
#define	LINK_DEPENDENT_OBJECTS		1
#define	CLEANUP				2

/*	check constraint specific states	*/

#define	CONSTRUCT_CK_SELECT_STATEMENT	20
#define	EXECUTE_CK_SELECT_STATEMENT	21
#define	MAKE_CHECK_RULE			22

/*
**	Referential constraint specific states
*/

#define	CONSTRUCT_SELECT_STATEMENT	30
#define	EXECUTE_SELECT_STATEMENT	31
#define	MAKE_INDEX_ON_REFING		32
#define	MAKE_INSERT_REFING_PROCEDURE	33
#define	MAKE_UPDATE_REFING_PROCEDURE	34
#define	MAKE_DELETE_REFED_PROCEDURE	35
#define	MAKE_UPDATE_REFED_PROCEDURE	36
#define	MAKE_INSERT_REFING_RULE		37
#define	MAKE_UPDATE_REFING_RULE		38
#define	MAKE_DELETE_REFED_RULE		39
#define	MAKE_UPDATE_REFED_RULE		40


DB_STATUS
qea_createIntegrity(
    QEF_AHD		*qea_act,
    QEF_RCB		*qef_rcb,
    QEE_DSH		*dsh )
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		securityStatus = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    i4		error;
    QEF_CREATE_INTEGRITY_STATEMENT	*details =
			&(qea_act->qhd_obj.qhd_createIntegrity);
    DB_INTEGRITY	integrityTuple;
    DB_INTEGRITY	integrityTuple2;
    DMT_TBL_ENTRY	constrainedTableInfo;
    DMT_TBL_ENTRY	referredTableInfo;
    DB_TAB_ID		*constraintID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constraintID ];
    DMT_ATT_ENTRY       ***constrainedTableAttributeArray =
 ( DMT_ATT_ENTRY *** ) &( dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray]);
    DB_CONSTRAINT_NAME	constraintName;
    i4			constraintType;
    char		*internalConstraintName =
	    ( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DB_TAB_ID		*constrainedTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constrainedTableID ];
					 /* id of table constraint is on */
    DB_TAB_ID		*referredTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_referredTableID ];
					 /* id of foreign table */
    DMT_ATT_ENTRY	***referredTableAttributeArray =
( DMT_ATT_ENTRY ***) &(dsh->dsh_row[ details->qci_referredTableAttributeArray]);

    i4			*state = ( i4  * ) dsh->dsh_row[ details->qci_state ];
    i4			*initialState =
			( i4  * ) dsh->dsh_row[ details->qci_initialState ];

    dsh->dsh_error.err_code = E_QE0000_OK;

    switch ( details->qci_integrityTuple->dbi_consflags &
	     ( CONS_CHECK | CONS_UNIQUE | CONS_REF ) )
    {
	case CONS_CHECK:
	    constraintType = CHECK_CONSTRAINT;
	    break;

	case CONS_UNIQUE:
	    constraintType = UNIQUE_CONSTRAINT;
	    break;

	case CONS_REF:
	    constraintType = REFERENTIAL_CONSTRAINT;
	    break;

	default:
	    status = E_DB_ERROR;
	    error = E_QE1996_BAD_ACTION;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    return( status );

    }	/* end switch on dbi_consflags */

    for ( ; ; )		/* code block to break out of */
    {
        if ( !( qef_rcb->qef_intstate & QEF_EXEIMM_PROC ) )
        {
	    /* the QEF_EXEIMM_PROC bit will be turned on by the setup
	    ** for execute immediates */

            /* open a memory stream to which to write integrity tuples,
	    ** text strings and attribute descriptors */
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, *ulm);

            if ((status = qec_mopen( ulm )) != E_DB_OK)
            {
	        status = E_DB_ERROR;
	        error = E_QE001E_NO_MEM;
	        qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	        break;
            }

	    /* copy the compiled integrity tuple into a writeable data area
	    ** where we can modify it */

            MEcopy( ( PTR ) details->qci_integrityTuple,
	        sizeof( DB_INTEGRITY ), ( PTR ) &integrityTuple );

            /* currently, we only allow constraints */

            if ( !( details->qci_flags & QCI_CONSTRAINT ) )
            {
	        status = E_DB_ERROR;
	        error = E_QE1996_BAD_ACTION;
	        qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	        break;
            }

            /* fill in table info */
            status = qea_0lookupTableInfo( dsh, (bool)TRUE, 
		    &details->qci_cons_tabname, &details->qci_cons_ownname,
		    (DB_TAB_ID *)NULL, &constrainedTableInfo, (bool)TRUE);

            if ( DB_FAILURE_MACRO( status ) )
		break;
	    else
		*constrainedTableID = constrainedTableInfo.tbl_id;

            MEcopy( ( PTR ) constrainedTableID, sizeof( DB_TAB_ID ),
		    ( PTR ) &integrityTuple.dbi_tabid );

	    /* lookup schemaID if it wasn't supplied */

	    if ( ( integrityTuple.dbi_consschema.db_tab_base == 0 )
	      && ( integrityTuple.dbi_consschema.db_tab_index == 0 )
	       )
	    {
		status = lookupSchemaID( dsh,
			 &details->qci_integritySchemaName,
			 &integrityTuple.dbi_consschema );
		if ( DB_FAILURE_MACRO( status ) )	break;
	    }

            /* allocate constraint id and store it in the tuple */
	    qef_cb->qef_dbid = qef_rcb->qef_db_id;
            status = qea_reserveID(constraintID, qef_cb, &dsh->dsh_error);
            if ( status != E_DB_OK )	break;
	    MEcopy( ( PTR ) constraintID, sizeof( DB_CONSTRAINT_ID ),
		    ( PTR ) &integrityTuple.dbi_cons_id );

            /* construct a constraint name and copy it to tuple if necessary */

            status = qea_constructConstraintName( constraintID,
	        details->qci_cons_tabname.db_tab_name,
	        constraintType, constraintName.db_constraint_name, dsh );
            if ( status != E_DB_OK )	break;

            if ( !( details->qci_flags & QCI_CONS_NAME_SPEC ) )
            {
	        STRUCT_ASSIGN_MACRO( constraintName, integrityTuple.dbi_consname );
            }

	    /*
	    ** For referential constraints, save referential action values
	    ** in integrity tuple. 
	    */
	    if (constraintType == REFERENTIAL_CONSTRAINT)
	    {
		integrityTuple.dbi_delrule = DBI_RULE_NOACTION;
		if (details->qci_flags & QCI_DEL_RESTRICT)
			integrityTuple.dbi_delrule = DBI_RULE_RESTRICT;
		if (details->qci_flags & QCI_DEL_CASCADE)
			integrityTuple.dbi_delrule = DBI_RULE_CASCADE;
		if (details->qci_flags & QCI_DEL_SETNULL)
			integrityTuple.dbi_delrule = DBI_RULE_SETNULL;

		integrityTuple.dbi_updrule = DBI_RULE_NOACTION;
		if (details->qci_flags & QCI_UPD_RESTRICT)
			integrityTuple.dbi_updrule = DBI_RULE_RESTRICT;
		if (details->qci_flags & QCI_UPD_CASCADE)
			integrityTuple.dbi_updrule = DBI_RULE_CASCADE;
		if (details->qci_flags & QCI_UPD_SETNULL)
			integrityTuple.dbi_updrule = DBI_RULE_SETNULL;
	    }

            /*
            ** save the constraint name in the DSH so we can recall it to construct
            ** subsequent rule and procedure names.
            */

            MEcopy( ( PTR ) constraintName.db_constraint_name,
		    DB_MAXNAME, ( PTR ) internalConstraintName );

            /* if there's a second integrity tuple, make it agree with the first */

            if ( (details->qci_knownNotNullableTuple != (DB_INTEGRITY *) NULL)
		&& (integrityTuple.dbi_consflags & CONS_KNOWN_NOT_NULLABLE) )
            {
	        MEcopy( ( PTR ) details->qci_knownNotNullableTuple,
    	        	sizeof( DB_INTEGRITY ), ( PTR ) &integrityTuple2 );
	        MEcopy( ( PTR ) constraintID,
			sizeof( DB_CONSTRAINT_ID ),
		        ( PTR ) &integrityTuple2.dbi_cons_id );
	        STRUCT_ASSIGN_MACRO( integrityTuple.dbi_tabid,
		    integrityTuple2.dbi_tabid );
	        STRUCT_ASSIGN_MACRO( integrityTuple.dbi_consname,
		    integrityTuple2.dbi_consname );
		STRUCT_ASSIGN_MACRO( integrityTuple.dbi_consschema,
		    integrityTuple2.dbi_consschema );
            }

            /*
            ** package up constraint query text into tuples.  store them  and
            ** the integrity tuples on disk.
            */

            status = qea_writeIntegrityTuples( qea_act, dsh, &integrityTuple,
					&integrityTuple2 );
            if ( DB_FAILURE_MACRO( status ) )	break;

	    /* fill in attribute array for constrained table */

	    status = qea_readAttributes( dsh, ulm, &constrainedTableInfo,
			constrainedTableAttributeArray );
            if ( DB_FAILURE_MACRO( status ) )	break;

	    /*
	    ** for referential constraint, fill in attribute array for
	    ** the referred table.
	    */

	    if ( details->qci_integrityTuple->dbi_consflags & CONS_REF )
	    {
		status = qea_0lookupTableInfo( dsh, (bool)TRUE, 
		    &details->qci_ref_tabname, &details->qci_ref_ownname,
		    (DB_TAB_ID *)NULL, &referredTableInfo, (bool)TRUE);

                if ( DB_FAILURE_MACRO( status ) )
		    break;
		else
		    *referredTableID = referredTableInfo.tbl_id;

		status = qea_readAttributes( dsh, ulm, &referredTableInfo,
			referredTableAttributeArray );
		if ( DB_FAILURE_MACRO( status ) )	break;
	    }

	    /* initialize the state machine for this type of constraint */

            switch ( details->qci_integrityTuple->dbi_consflags &
	             ( CONS_CHECK | CONS_UNIQUE | CONS_REF ) )
            {
	        case CONS_CHECK:

		    /*
		    ** If we're adding a CHECK CONSTRAINT to a pre-existing
		    ** table, then we have to verify the CONSTRAINT with a
		    ** SELECT statement.
		    **
		    ** Don't validate check constraints from upgradedb:
		    ** jan-09-2003: removed QEF_RUNNING_UPGRADEDB bypass
		    ** of verify check constraint holds, since upgradedb
		    ** no longer drops and recreates check constraints
		    ** to fix query trees. (It upgrades the underlying
		    ** system generated rules instead)
		    ** 
		    */

		    if ((details->qci_flags & QCI_VERIFY_CONSTRAINT_HOLDS ))
			/*
		       && (qef_cb->qef_sess_flags & QEF_RUNNING_UPGRADEDB) == 0)
			*/
		    {	
			*state = CONSTRUCT_CK_SELECT_STATEMENT; 
		    }
		    else
		    {
		        *state = MAKE_CHECK_RULE; 
		    }

		    break;

	        case CONS_UNIQUE:
	            *state = INITIALIZE;
	            break;

	        case CONS_REF:

		    /*
		    ** If we're adding a REFERENTIAL CONSTRAINT to a
	 	    ** pre-existing table, then we have to cook up a SELECT
		    ** statement to verify it.  Otherwise, the first thing
		    ** we do is cook up an INDEX on the referring columns.
		    */

		    if ( details->qci_flags & QCI_VERIFY_CONSTRAINT_HOLDS )
		    {	*state = CONSTRUCT_SELECT_STATEMENT; }
		    else {   *state = MAKE_INDEX_ON_REFING; }

		    /*
		    ** The SELECT statement uses this flag to communicate
		    ** its success or failure to the subsequent REFERENTIAL
		    ** state.  Initialize the flag to a known condition.
		    */

		    qef_cb->qef_callbackStatus &= ~(QEF_SELECT_RETURNED_ROWS );

	            break;

	        default:
	            break;

            }	/* end switch on dbi_consflags */
	    *initialState = *state;

        }	/* end if first time through */

        /* constraint-type specific neuroses */

        switch ( details->qci_integrityTuple->dbi_consflags &
	         ( CONS_CHECK | CONS_UNIQUE | CONS_REF ) )
        {
	    case CONS_CHECK:
		status = qea_checkConstraint( qea_act, dsh );
	        break;

	    case CONS_REF:
	        status = qea_referentialConstraint( qea_act, dsh );
	        break;

	    case CONS_UNIQUE:
	        status = qea_uniqueConstraint( qea_act, dsh );
	        break;

	    default:
	        status = E_DB_ERROR;
	        error = E_QE1996_BAD_ACTION;
	        qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	        break;

        }	/* end switch on dbi_consflags */
	if ( status != E_DB_OK )	break;

	break;
    }	/* end of code block */

    if ( *state == CLEANUP || ( status != E_DB_OK ) )
    {
	/* flag that we are done with execute immediate processing */

	qef_rcb->qef_intstate &= ~QEF_EXEIMM_PROC;

        /* tear down the memory stream */

        ulm_closestream( ulm );

	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR	e_error;
	    /*
	    ** We need to audit successful creation of integrity
	    */
	    securityStatus = qeu_secaudit(FALSE, qef_rcb->qef_sess_id, 
		details->qci_cons_tabname.db_tab_name,
		&details->qci_cons_ownname, 
		sizeof(details->qci_cons_tabname.db_tab_name),
		SXF_E_TABLE, I_SX203A_TBL_CONSTRAINT_ADD,
		SXF_A_SUCCESS | SXF_A_ALTER, &e_error);
	    if ( securityStatus != E_DB_OK)
		status = securityStatus;
	}
    }

    return( status );
}

/*{
** Name: qea_cview() - handle creation of a view
**
** Description:
**	This function will handle creation of a view.  
**	In many cases (STAR and QUEL views, SQL views created without CHECK 
**	OPTION and those for which CHECK OPTION does not have to be enforced 
**	dynamically, this will simply involve calling an appropriate qeu 
**	function to populate catalogs describing the new view.  
**
**	For non-STAR views created WITH CHECK OPTION for which the CHECK OPTION
**	needs to be enforced dynamically, we will create (using EXECUTE 
**	IMMEDIATE) a statement-level rule and a set-input dbproc to handle 
**	enforcement of WITH CHECK OPTION.  qea_cview() will use a "state" 
**	variable in dsh to keep track of the action that need to be performed 
**	next. This is required to handle the special "magic" used by EXECUTE 
**	IMMEDIATE processing
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**	    qhd_obj.qhd_create_view	QEA_CREATE_VIEW action descriptor
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE1996_BAD_ACTION
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-feb-93 (andre)
**	    written
**	09-mar-93 (andre)
**	    when calling setupForExecuteImmediate(), we must tell it that the
**	    object is system-generated (PST_SYSTEM_GENERATED_ in addition to the
**	    fact that it is not user-droppable
**	31-mar-93 (rickh)
**	    Turn off execute immediate processing in this module.  This will
**	    prevent later actions from thinking that they are processing inside
**	    an execute immediate context.
**	12-mar-93 (andre)
**	    having create the rule and the dbproc to enforce CHECK OPTION, we
**	    need to insert a tuple into IIDBDEPENDS indicating that the dbproc
**	    enforcing CHECK OPTION depends on the view.  This is needed because
**	    PSF will not build independent object lists for system-generated
**	    objects, and without this tuple we would have no way to know that
**	    the dbproc needs to be dropped when the view itself is dropped
**	26-jun-93 (andre)
**	    when calling setupForExecuteImmediate() before calling PSF to create
**	    a rule which will enforce CASCADED CHECK OPTION on the new view,
**	    pass PST_CASCADED_CHECK_OPTION_RULE to setupForExecuteImmediate() to
**	    ensure that the new rule will get marked as enforcing CASCADED CHECK
**	    OPTION.
**	22-jul-93 (anitap)
**	    Added new argument to qea_chkerror().
**	25-aug-93 (rickh)
**	    Added another argument to qea_chkerror.
[@history_line@]...
*/
/*
**	Values that the state word can take:
*/


#define	QEA_CVIEW_CREATE_CO_DBPROC	1
#define	QEA_CVIEW_CREATE_CO_RULE	2
#define	QEA_RECORD_DBP_VIEW_DEPENDENCY	3

#define	QEA_CVIEW_CLEANUP		99

DB_STATUS
qea_cview(
    QEF_AHD		*qea_act,
    QEF_RCB		*qef_rcb,
    QEE_DSH		*dsh)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    i4		error;
    QEF_CREATE_VIEW     *crt_view = &qea_act->qhd_obj.qhd_create_view;
    ULM_RCB		*ulm = (ULM_RCB *) dsh->dsh_row[crt_view->ahd_ulm_rcb];
    DB_TAB_ID		*view_id =
	(DB_TAB_ID *) dsh->dsh_row[crt_view->ahd_view_id];
    DB_OWN_NAME		*view_owner = 
	(DB_OWN_NAME *) dsh->dsh_row[crt_view->ahd_view_owner];
    DB_NAME		rule_name;
    DB_TEXT_STRING	*rule_text;
    DB_DBP_NAME		*dbp_name = 
	(DB_DBP_NAME *) dsh->dsh_row[crt_view->ahd_check_option_dbp_name];
    DB_TEXT_STRING	*dbp_text;
    i4			*state = ( i4  * ) dsh->dsh_row[crt_view->ahd_state];
    PST_INFO		*pst_info = 
	(PST_INFO *) dsh->dsh_row[crt_view->ahd_pst_info];
    DMT_TBL_ENTRY	*tbl_info = 
	(DMT_TBL_ENTRY *) dsh->dsh_row[crt_view->ahd_tbl_entry];
    QSF_RCB		qsf_rb;

    dsh->dsh_error.err_code = E_QE0000_OK;

    for (;;)		/* something to break out of */
    {
        if (~qef_rcb->qef_intstate & QEF_EXEIMM_PROC)
        {
	    /* 
	    ** the QEF_EXEIMM_PROC bit will be turned on by the setup
	    ** for EXECUTE IMMEDIATEs
	    */

	    QEUQ_CB	    *qeuqcb = (QEUQ_CB *) crt_view->ahd_crt_view_qeuqcb;

	    /* 
	    ** before anything else happens, set ulm->ulm_streamid to NULL so 
	    ** that if an error occurs before (if ever) we open the memory 
	    ** stream, we will not try to close it 
	    */
	    ulm->ulm_streamid = (PTR)NULL;
	    ulm->ulm_streamid_p = &ulm->ulm_streamid;

	    if (crt_view->ahd_crt_view_flags & AHD_STAR_VIEW)
	    {
	        QEF_RCB	lock_rcb;

	        MEfill(sizeof(lock_rcb), '\0', (PTR) & lock_rcb);
	        lock_rcb.qef_cb = qef_cb;

	        /* lock to synchronize */
    
	        status = qel_u3_lock_objbase(&lock_rcb);
	        if (status)
	        {
		    STRUCT_ASSIGN_MACRO(lock_rcb.error, dsh->dsh_error);
		    break;
	        }
		    
	        status = qeu_d3_cre_view(qef_cb, qeuqcb);
	    }
	    else
	    {
		/* 
		** I've just trained qeu_cview() to provide me with id of the 
		** view; all we need to do is pass it the address..
		*/
		qeuqcb->qeuq_rtbl = view_id;

	        status = qeu_cview(qef_cb, qeuqcb);
	    }

	    if (DB_FAILURE_MACRO(status))
	    {
	        STRUCT_ASSIGN_MACRO(qeuqcb->error, dsh->dsh_error);
		break;
	    }

	    /* 
	    ** non-STAR view created WITH CHECK OPTION may require that we 
	    ** enforce CHECK OPTION dynamically, in which case we will have to 
	    ** create a rule and a dbproc to enforce CHECK OPTION; otherwise we 
	    ** are done
	    */
	    if (!(crt_view->ahd_crt_view_flags & 
		  (AHD_CHECK_OPT_DYNAMIC_INSRT | AHD_CHECK_OPT_DYNAMIC_UPDT)))
	    {
		break;
	    }

	    /*
	    ** open memory stream which will be used to build text of a rule and
	    ** a dbproc which will enforce CHECK OPTION
	    */

	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, (*ulm));
	    ulm->ulm_streamid = (PTR)NULL;
            if ((status = qec_mopen(ulm)) != E_DB_OK)
            {
	        status = E_DB_ERROR;
	        error = E_QE001E_NO_MEM;
	        qef_error(error, 0L, status, &error, &dsh->dsh_error, 0);
	        break;
            }

            /* get the view description */

            status = qea_0lookupTableInfo(dsh, (bool) FALSE,
		(DB_TAB_NAME *) NULL, (DB_OWN_NAME *) NULL, view_id, tbl_info,
		( bool ) TRUE );
            if (DB_FAILURE_MACRO(status))
		break;

	    /* 
	    ** copy the view owner name from tbl_info into DSH
	    */
	    STRUCT_ASSIGN_MACRO(tbl_info->tbl_owner, (*view_owner));

	    *state = QEA_CVIEW_CREATE_CO_DBPROC;
	}	/* if first time through */

	switch (*state)
	{
	    case QEA_CVIEW_CREATE_CO_DBPROC:
	    {
	        /* construct dbproc name */
	        status = qea_co_obj_name(view_id, &crt_view->ahd_view_name, "P",
		    dbp_name->db_dbp_name, dsh);
		if (DB_FAILURE_MACRO(status))
		    break;

	        /* construct dbproc text */
	        status = qea_cop_text(dsh, crt_view, &dbp_text);
	        if (DB_FAILURE_MACRO(status))
	    	    break;

		status = setupForExecuteImmediate(qea_act, dsh, 
		    pst_info, dbp_text,
		    ( u_i4 ) (PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED));
		if (DB_FAILURE_MACRO(status))
		    break;
		
		/* next we must create the rule */
		*state = QEA_CVIEW_CREATE_CO_RULE;

		break;
	    }
	    case QEA_CVIEW_CREATE_CO_RULE:
	    {
		/* check for errors from the preceding execute immediate */

		status = qea_chkerror(dsh, &qsf_rb,
			QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS );
		if (status != E_DB_OK)
		    break;

	        /* construct rule name */
	        status = qea_co_obj_name(view_id, &crt_view->ahd_view_name, "R",
		    rule_name.db_name, dsh );
		if (DB_FAILURE_MACRO(status))
		    break;

	        /* construct rule text */
	        status = qea_cor_text(dsh, crt_view, tbl_info, &rule_name,
		    &rule_text);
	        if (DB_FAILURE_MACRO(status))
		    break;

		status = setupForExecuteImmediate(qea_act, dsh, 
		    pst_info, rule_text,
		    ( u_i4 ) (PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED |
				   PST_CASCADED_CHECK_OPTION_RULE));
		if (DB_FAILURE_MACRO(status))
		    break;
		
		*state = QEA_RECORD_DBP_VIEW_DEPENDENCY;

		break;
	    }
	    case QEA_RECORD_DBP_VIEW_DEPENDENCY:
	    {
		/* check for errors from the preceding execute immediate */

		status = qea_chkerror(dsh, &qsf_rb,
			QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS );
		if (status != E_DB_OK)
		    break;

		/*
		** we need to construct an IIDBDEPENDS tuple linking the dbproc
		** responsible for enforcing CHECK OPTION to the view
		*/

		status = qea_record_co_dbp_dependency(qea_act, dsh);
		if (status != E_DB_OK)
		    break;

		*state = QEA_CVIEW_CLEANUP;
		
		/* fall through to perform cleanup */
	    }
	    case QEA_CVIEW_CLEANUP:
	    {
		/* terminate the execute immediate processing */

		qef_rcb->qef_intstate &= ~QEF_EXEIMM_PROC;

		/* we are done - let go of the memory */
		if (ulm->ulm_streamid != (PTR)NULL)
		{
		    ulm_closestream(ulm);
		}
		
		break;

	    }
        }	/* switch (*state) */

	if (DB_FAILURE_MACRO(status))
	    break;

	break;
    }	/* end of code block */

    /*
    ** if everything went OK, the code above will take care of releasing memory;
    ** if some error occurred, it has to happen here
    */
    if (status != E_DB_OK && ulm->ulm_streamid != (PTR) NULL)
    {
        /* release the allocated memory */

        ulm_closestream(ulm);
    }

    return( status );
}

/*
** Name: qea_d_integ - orchestrate destruction of an integrity constraint
**
** Description:
**	This function will handle destruction of a single ANSI-style constraint
**	Eventually, this function may assume responsibility for destroying
**	INGRES integrity(ies), but this will have to wait.
**
**	If asked to destroy a specific constraint which was defined on a table,
**	we will
**	    (1) verify that the specified constraint exists and that it was
**	        created on the specified table; if not, report the error to the
**		user
**	    (2) if the specified constraint is a UNIQUE or PRIMARY KEY
**		constraint and RESTRICTED destruction has been specified,
**		determine if any constraint depends on the constraint to be
**		dropped (we are only interested in constraints since currently
**		the only type of user-created objects that may depend on a
**		[UNIQUE/PRIMARY KEY] constraint is a [REFERENCES] constraint);
**		if one is found, report error to the user
**	    (3) if everything looks OK, call qeu_d_cascade() to handle 
**		cascading destruction
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**	    qhd_obj.qhd_drop_integrity	QEF_DROP_INTEGRITY action descriptor
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE1996_BAD_ACTION
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-mar-93 (andre)
**	    written
**	7-apr-93 (rickh)
**	    Initialize status to E_DB_OK as good hygiene.
**	13-apr-93 (andre)
**	    DB_INTEGRITYIDX did not correctly reflect the structure of
**	    IIINTEGRITYIDX. 
**	28-jul-93 (rblumer)
**	    added check for NOT NULL constraints implemented as non-nullable
**	    datatypes--we don't allow them to be dropped;
**	    also made sure to set status to E_DB_ERROR for some errors,
**	    as otherwise they are not reported to the user.
**	6-sep-93 (stephenb)
**	    Added call to qeu_secaudit() to write security audit record.
**	10-sep-93 (andre)
**	    set QEU_FORCE_QP_INVALIDATION in qeuq_cb->qeuq_flag_mask to 
**	    indicate to qeu_d_casacde() that QPs that may be affected by 
**	    destruction of the constraint need to be invalidated
**	08-oct-93 (andre)
**	    qeu_d_cascade() expects one more parameter - an address of a 
**	    DMT_TBL_ENTRY describing table/view/index being dropped; for all 
**	    other types of objects, NULL must be passed
**	24-Jan-2001 (jenjo02)
**	    If we break on error after all successful qec_malloc()-s,
**	    the stream will not get closed, leaking memory.
*/
DB_STATUS
qea_d_integ(
	QEF_AHD             *qea_act,
	QEF_RCB             *qef_rcb)
{
    QEF_CB		    *qef_cb = qef_rcb->qef_cb;
    QEE_DSH		*dsh = (QEE_DSH *) qef_cb->qef_dsh;
    DB_IIDBDEPENDS          *dep_tuple;
    DB_INTEGRITYIDX	    *idx_tuple;
    DB_INTEGRITY	    *int_tuple;
    DB_STATUS		    status = E_DB_OK;
    DB_STATUS		    local_status = E_DB_OK;
    i4		    error = E_QE0000_OK;
    bool		    transtarted;
    QEU_CB		    tranqeu;
    QEU_CB		    depqeu, *dep_qeu = (QEU_CB *) NULL;
    QEU_CB		    idxqeu, *idx_qeu = (QEU_CB *) NULL;
    QEU_CB		    intqeu, *int_qeu = (QEU_CB *) NULL;
    QEU_QUAL_PARAMS	    qparams;
    ULM_RCB		    ulm;
    QEF_DATA		    depqef_data;
    QEF_DATA		    idxqef_data;
    QEF_DATA		    intqef_data;
    DMR_ATTR_ENTRY	    depkey_array[4];
    DMR_ATTR_ENTRY	    *depkey_ptr_array[4];
    DMR_ATTR_ENTRY	    idxkey_array[3];
    DMR_ATTR_ENTRY	    *idxkey_ptr_array[3];
    QEUQ_CB		    *qeuq_cb;
    QEF_DROP_INTEGRITY	    *drop_integ = &qea_act->qhd_obj.qhd_drop_integrity;
    i4			    obj_type = DB_CONS;
    i4			    integ_num;

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_streamid = (PTR)NULL;

    /* check the contents of QEF_DROP_INTEGRITY action for consistency */

    /*
    ** the only flags which we recognize are
    **	- AHD_DROP_ANSI_CONSTRAINT - drop constraint(s) (must be set)
    **	- AHD_DROP_CONS_CASCADE    - perform cascading destruction
    */
    if (   drop_integ->ahd_flags &
	       ~(AHD_DROP_ANSI_CONSTRAINT | AHD_DROP_CONS_CASCADE)
	|| ~drop_integ->ahd_flags & AHD_DROP_ANSI_CONSTRAINT
       )
    {
	(VOID) qef_error(E_QE1996_BAD_ACTION, 0L, E_DB_ERROR, &error,
	    &dsh->dsh_error, 0);
	return(E_DB_ERROR);
    }

    for (;;)	    /* something to break out of */
    {
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qef_cb->qef_dbid;
	    tranqeu.qeu_d_id = qef_cb->qef_ses_id;
	    tranqeu.qeu_flag = 0;
	    tranqeu.qeu_mask = 0;
	    status = qeu_btran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {	
		error = tranqeu.error.err_code;
		break;	
	    }	    
	    transtarted = TRUE;
	}
	else
	{
	    transtarted = FALSE;
	}
	
	/* escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	if ( (status = qec_mopen(&ulm)) == E_DB_OK )
	{
	    /*
	    ** we will try to read IIDBDEPENDS only if RESTRICTED revocation
	    ** is specified
	    */
	    if (~drop_integ->ahd_flags & AHD_DROP_CONS_CASCADE)
	    {
		ulm.ulm_psize = sizeof(DB_IIDBDEPENDS);
		if ( (status = qec_malloc(&ulm)) == E_DB_OK )
		    dep_tuple = (DB_IIDBDEPENDS *) ulm.ulm_pptr;
	    }
	    else
		dep_tuple = (DB_IIDBDEPENDS *) NULL;
	
	    if ( status == E_DB_OK )
	    {
		ulm.ulm_psize = sizeof(DB_INTEGRITYIDX);
		if ( (status = qec_malloc(&ulm)) == E_DB_OK )
		{
		    idx_tuple = (DB_INTEGRITYIDX *) ulm.ulm_pptr;

		    ulm.ulm_psize = sizeof(DB_INTEGRITY);
		    if ( (status = qec_malloc(&ulm)) == E_DB_OK )
		    {
			int_tuple = (DB_INTEGRITY *) ulm.ulm_pptr;

			ulm.ulm_psize = sizeof(QEUQ_CB);
			if ( (status = qec_malloc(&ulm)) == E_DB_OK )
			    qeuq_cb = (QEUQ_CB *) ulm.ulm_pptr;
		    }
		}
	    }
	}

	if (status != E_DB_OK)
	{
	    error = E_QE001E_NO_MEM;
	    break;
	}

	/*
	** now we open tables which we will need to perform the requested
	** operation;
	*/
	intqeu.qeu_type = QEUCB_CB;
	intqeu.qeu_length = sizeof(QEUCB_CB);
	intqeu.qeu_db_id = qef_cb->qef_dbid;
	intqeu.qeu_lk_mode = DMT_IS;
	intqeu.qeu_flag = DMT_U_DIRECT;
	intqeu.qeu_access_mode = DMT_A_READ;
	intqeu.qeu_mask = 0;
	intqeu.qeu_qual = 0;
	intqeu.qeu_qarg = 0;
	intqeu.qeu_f_qual = 0;
	intqeu.qeu_f_qarg  = 0;

	STRUCT_ASSIGN_MACRO(intqeu, idxqeu);

	intqeu.qeu_tab_id.db_tab_base  = DM_B_INTEGRITY_TAB_ID;
	intqeu.qeu_tab_id.db_tab_index  = DM_I_INTEGRITY_TAB_ID;
	status = qeu_open(qef_cb, &intqeu);
	if (DB_FAILURE_MACRO(status))
	{
	    error = intqeu.error.err_code;
	    break;
	}

	int_qeu = &intqeu;

	idxqeu.qeu_tab_id.db_tab_base  = DM_B_INTEGRITYIDX_TAB_ID;
	idxqeu.qeu_tab_id.db_tab_index = DM_I_INTEGRITYIDX_TAB_ID;
	status = qeu_open(qef_cb, &idxqeu);
	if (DB_FAILURE_MACRO(status))
	{
	    error = idxqeu.error.err_code;
	    break;
	}

	idx_qeu = &idxqeu;

	/*
	** set up QEU_CB structures to read IIINTEGRITY and
	** IIINTEGRITYIDX tuples
	*/
	
	int_qeu->qeu_count = 1;
	int_qeu->qeu_tup_length = sizeof(DB_INTEGRITY);
	int_qeu->qeu_output = &intqef_data;
	intqef_data.dt_next = 0;
	intqef_data.dt_size = sizeof(DB_INTEGRITY);
	intqef_data.dt_data = (PTR) int_tuple;

	idx_qeu->qeu_count = 1;
	idx_qeu->qeu_tup_length = sizeof(DB_INTEGRITYIDX);
	idx_qeu->qeu_output = &idxqef_data;
	idxqef_data.dt_next = 0;
	idxqef_data.dt_size = sizeof(DB_INTEGRITYIDX);
	idxqef_data.dt_data = (PTR) idx_tuple;

	idx_qeu->qeu_getnext = QEU_REPO;
	idx_qeu->qeu_klen = 3;
	idx_qeu->qeu_key = idxkey_ptr_array;
	
	idxkey_ptr_array[0] = idxkey_array;
	idxkey_ptr_array[0]->attr_number = DM_1_INTEGRITYIDX_KEY;
	idxkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	idxkey_ptr_array[0]->attr_value =
	    (char *) &drop_integ->ahd_cons_schema_id.db_tab_base;

	idxkey_ptr_array[1] = idxkey_array + 1;
	idxkey_ptr_array[1]->attr_number = DM_2_INTEGRITYIDX_KEY;
	idxkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	idxkey_ptr_array[1]->attr_value =
	    (char *) &drop_integ->ahd_cons_schema_id.db_tab_index;

	idxkey_ptr_array[2] = idxkey_array + 2;
	idxkey_ptr_array[2]->attr_number = DM_3_INTEGRITYIDX_KEY;
	idxkey_ptr_array[2]->attr_operator = DMR_OP_EQ;
	idxkey_ptr_array[2]->attr_value = (char *) &drop_integ->ahd_cons_name;

	int_qeu->qeu_flag = QEU_BY_TID;
	int_qeu->qeu_getnext = QEU_NOREPO;
	int_qeu->qeu_klen = 0;
	int_qeu->qeu_key = (DMR_ATTR_ENTRY **) NULL;

	/* first, look up the IIINTEGRITYIDX tuple for this constraint */
	status = qeu_get(qef_cb, idx_qeu);
	if (DB_FAILURE_MACRO(status))
	{
	    if (idx_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/*
		** constraint with the specified name does not exist in the
		** specified schema; this is somewhat strange since PSF was
		** supposed to verify that the constraint exists
		*/
		error = E_QE0166_NO_CONSTRAINT_IN_SCHEMA;
	    }
	    else
	    {
		error = idx_qeu->error.err_code;
	    }

	    break;
	}

	/* save the tid so we can read the IIINTEGRITY tuple */
	int_qeu->qeu_tid = idx_tuple->dbii_tidp;

	/* read an IIINTEGRITY tuple */
	status = qeu_get(qef_cb, int_qeu);
	if (DB_FAILURE_MACRO(status))
	{
	    /*
	    ** Something is very wrong since we were trying to retrieve the
	    ** tuple using the TID read from IIINTEGRITYIDX tuple
	    */
	    error = idx_qeu->error.err_code;
	    break;
	}

	/*
	** make sure the id of the table in IIINTEGRITY tuple matches that in
	** QEF_DROP_INTEGRITY structure before proceeding with destruction
	*/
	if (   int_tuple->dbi_tabid.db_tab_base !=
		   drop_integ->ahd_integ_tab_id.db_tab_base
	    || int_tuple->dbi_tabid.db_tab_index !=
		    drop_integ->ahd_integ_tab_id.db_tab_index
	   )
	{
	    error = E_QE0167_DROP_CONS_WRONG_TABLE;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** make sure the tuple describes a constraint and not an "old style"
	** INGRES integrity
	*/
	if (int_tuple->dbi_consflags == CONS_NONE)
	{
	    error = E_QE0166_NO_CONSTRAINT_IN_SCHEMA;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** make sure the tuple does not describe a NOT NULL constraint,
	** as this is implemented in the datatype/table structure,
	** not as a droppable rule.
	** Such constraints cannot be dropped.
	*/
	if (int_tuple->dbi_consflags & CONS_NON_NULLABLE_DATATYPE)
	{
	    error = E_QE016A_NOT_DROPPABLE_CONS;
	    status = E_DB_ERROR;
	    break;
	}
	
	/*
	** if asked to perform a RESTRICTed destruction of a UNIQUE or PRIMARY
	** KEY constraint, verify that there is no REFERENCES constraint
	** dependent on it (actually, since we will only look in IIDBDEPENDS, we
	** will not verify that it is, indded, a REFERENCES constraint that
	** depends on the constraint being dropped - we just happen to know that
	** this is the only type of user-created object that can depend on a
	** constraint)
	*/

	if (   int_tuple->dbi_consflags & CONS_UNIQUE
	    && ~drop_integ->ahd_flags & AHD_DROP_CONS_CASCADE)
	{
	    STRUCT_ASSIGN_MACRO(intqeu, depqeu);

	    depqeu.qeu_tab_id.db_tab_base  = DM_B_DEPENDS_TAB_ID;
	    depqeu.qeu_tab_id.db_tab_index = DM_I_DEPENDS_TAB_ID;
	    status = qeu_open(qef_cb, &depqeu);
	    if (DB_FAILURE_MACRO(status))
	    {
		error = depqeu.error.err_code;
		break;
	    }

	    dep_qeu = &depqeu;

	    dep_qeu->qeu_count = 1;
	    dep_qeu->qeu_tup_length = sizeof(DB_IIDBDEPENDS);
	    dep_qeu->qeu_output = &depqef_data;
	    depqef_data.dt_next = 0;
	    depqef_data.dt_size = sizeof(DB_IIDBDEPENDS);
	    depqef_data.dt_data = (PTR) dep_tuple;

	    dep_qeu->qeu_getnext = QEU_REPO;
	    dep_qeu->qeu_klen = 4;
	    dep_qeu->qeu_key = depkey_ptr_array;
	    
	    depkey_ptr_array[0] = depkey_array;
	    depkey_ptr_array[0]->attr_number = DM_1_DEPENDS_KEY;
	    depkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    depkey_ptr_array[0]->attr_value =
		(char *) &drop_integ->ahd_integ_tab_id.db_tab_base;

	    depkey_ptr_array[1] = depkey_array + 1;
	    depkey_ptr_array[1]->attr_number = DM_2_DEPENDS_KEY;
	    depkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    depkey_ptr_array[1]->attr_value =
		(char *) &drop_integ->ahd_integ_tab_id.db_tab_index;

	    depkey_ptr_array[2] = depkey_array + 2;
	    depkey_ptr_array[2]->attr_number = DM_3_DEPENDS_KEY;
	    depkey_ptr_array[2]->attr_operator = DMR_OP_EQ;
	    depkey_ptr_array[2]->attr_value = (char *) &obj_type;

	    depkey_ptr_array[3] = depkey_array + 3;
	    depkey_ptr_array[3]->attr_number = DM_4_DEPENDS_KEY;
	    depkey_ptr_array[3]->attr_operator = DMR_OP_EQ;
	    depkey_ptr_array[3]->attr_value = (char *) &integ_num;

	    /*
	    ** we must use a copy of the integrity number found in the
	    ** IIINTEGRITY tuple because it is an i2, while
	    ** IIDBDEPENDS.dbv_i_qid is an i4
	    */
	    integ_num = int_tuple->dbi_number;

	    /*
	    ** specifying qea_dep_cons() as the qualification function will
	    ** ensure that we will not be given a tuple unless it describes
	    ** dependence of a constraint on the constraint being dropped
	    */
	    dep_qeu->qeu_qual = qea_dep_cons;
	    dep_qeu->qeu_qarg = &qparams;

	    /*
	    ** look for a constraint that depends on constraint named in
	    ** ALTER TABLE DROP CONSTRAINT statement
	    */
	    status = qeu_get(qef_cb, dep_qeu);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (dep_qeu->error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    /*
		    ** no dependent constraint was found - OK to destroy
		    ** specified constraint
		    */
		    status = E_DB_OK;
		}
		else
		{
		    error = dep_qeu->error.err_code;
		    break;
		}
	    }
	    else
	    {
		/*
		** restricted revocation cannot proceed - some REFERENCES
		** constraint depends on this constraint
		*/
		error = E_QE0168_ABANDONED_CONSTRAINT;
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** now we call qeu_d_cascade() to destroy the constraint and all objects
	** dependent on it; note that we can do it even if RESTRICTed revocation
	** has been specified because we have verified that there are no
	** user-specified objects dependent on it
	*/

	/*
	** first initialize QEUQ_CB
	*/
	MEfill(sizeof(QEUQ_CB), (u_char) 0, (PTR) qeuq_cb);
	qeuq_cb->qeuq_eflag  	 = QEF_EXTERNAL;
	qeuq_cb->qeuq_type   	 = QEUQCB_CB;
	qeuq_cb->qeuq_length 	 = sizeof(QEUQ_CB);
	qeuq_cb->qeuq_rtbl   	 = &drop_integ->ahd_integ_tab_id;
	qeuq_cb->qeuq_d_id   	 = qef_cb->qef_ses_id;
	qeuq_cb->qeuq_db_id  	 = qef_cb->qef_dbid;
	qeuq_cb->qeuq_ino    	 = int_tuple->dbi_number;
	qeuq_cb->qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;

	status = qeu_d_cascade(qef_cb, qeuq_cb, (DB_QMODE) DB_CONS, 
	    (DMT_TBL_ENTRY *) NULL);
	if (DB_FAILURE_MACRO(status))
	{
	    /* qeu_d_cascade() reports its own errors */
	    STRUCT_ASSIGN_MACRO(qeuq_cb->error, dsh->dsh_error);
	}
	/* successfull deletion of integrity, we need to audit */
	else if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR	e_error;

	    status = qeu_secaudit(FALSE, qef_rcb->qef_sess_id, 
		drop_integ->ahd_integ_tab_name.db_tab_name,
		&drop_integ->ahd_integ_schema, 
		sizeof(drop_integ->ahd_integ_tab_name.db_tab_name),
		SXF_E_TABLE, I_SX203B_TBL_CONSTRAINT_DROP,
		SXF_A_SUCCESS | SXF_A_ALTER, &e_error);
	    if (status != E_DB_OK)
		error = e_error.err_code;
	}

	break;
    }

    /*
    ** we get here if we are done or if some error was encountered - status
    ** will tell us which.  If error status was returned by qeu_d_cascade(), it
    ** will report its own error - we do not want to report it here
    */
    if (status != E_DB_OK && error != E_QE0000_OK)
    {
	/*
	** some of the errors that may be reported here require that parameters
	** be passed to qef_error()
	*/
	if (error == E_QE0166_NO_CONSTRAINT_IN_SCHEMA)
	{
	    (VOID) qef_error(E_QE0166_NO_CONSTRAINT_IN_SCHEMA, 0L, status,
		&error, &dsh->dsh_error, 2,
		qec_trimwhite(sizeof(drop_integ->ahd_cons_name),
		    (char *) &drop_integ->ahd_cons_name),
		(PTR) &drop_integ->ahd_cons_name,
		qec_trimwhite(sizeof(drop_integ->ahd_integ_schema),
		    (char *) &drop_integ->ahd_integ_schema),
		(PTR) &drop_integ->ahd_integ_schema);
	}
	else if (error == E_QE0167_DROP_CONS_WRONG_TABLE)
	{
	    (VOID) qef_error(E_QE0167_DROP_CONS_WRONG_TABLE, 0L, status,
		&error, &dsh->dsh_error, 3,
		qec_trimwhite(sizeof(drop_integ->ahd_cons_name),
		    (char *) &drop_integ->ahd_cons_name),
		(PTR) &drop_integ->ahd_cons_name,
		qec_trimwhite(sizeof(drop_integ->ahd_integ_schema),
		    (char *) &drop_integ->ahd_integ_schema),
		(PTR) &drop_integ->ahd_integ_schema,
		qec_trimwhite(sizeof(drop_integ->ahd_integ_tab_name),
		    (char *) &drop_integ->ahd_integ_tab_name),
		(PTR) &drop_integ->ahd_integ_tab_name);
	}
	else if (   (error == E_QE0168_ABANDONED_CONSTRAINT)
		 || (error == E_QE016A_NOT_DROPPABLE_CONS))
	{
	    (VOID) qef_error(error, 0L, status,
			     &error, &dsh->dsh_error, 1,
			     qec_trimwhite(sizeof(drop_integ->ahd_cons_name),
					   (char *) &drop_integ->ahd_cons_name),
			     (PTR) &drop_integ->ahd_cons_name);	
	}
	else
	{
	    (VOID) qef_error(error, 0L, status, &error, &dsh->dsh_error, 0);
	}
    }
    
    /* Close off all the tables that have been opened. */

    if (int_qeu)
    {
	local_status = qeu_close(qef_cb, int_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(int_qeu->error.err_code, 0L, local_status,
		&error, &dsh->dsh_error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (idx_qeu)
    {
	local_status = qeu_close(qef_cb, idx_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(idx_qeu->error.err_code, 0L, local_status,
		&error, &dsh->dsh_error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (dep_qeu)
    {
	local_status = qeu_close(qef_cb, dep_qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(dep_qeu->error.err_code, 0L, local_status,
		&error, &dsh->dsh_error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (transtarted)
    {
	local_status = qeu_atran(qef_cb, &tranqeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			&error, &dsh->dsh_error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    /* Close memory stream if opened */
    if ( ulm.ulm_streamid )
	ulm_closestream(&ulm);

    return(status);
}

/*{
** Name: qea_readAttributes - read in the attribute descriptors for a table
**
** Description:
**	This routine is called by the integrity interpreter.  This
**	routine reads in the attribute descriptors for a table.
**
**
**
** Inputs:
**      dsh			data segment header
**	    .dsh_qefcb			session control block
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
*/

static DB_STATUS
qea_readAttributes(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DMT_TBL_ENTRY	*tableinfo,
    DMT_ATT_ENTRY	***attributeArray )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb; /* session control block */
    DMT_SHW_CB          dmt_shw_cb;
    DMT_ATT_ENTRY	**att_pptr;
    DMT_ATT_ENTRY	*att;
    i4		error;
    i4			i;
    i4		realNumberOfAttributes = tableinfo->tbl_attr_count + 1;
    i4		nametot = tableinfo->tbl_attr_nametot;
    i4		dmt_mem_size;
    char	*nextname;

    dmt_shw_cb.length = sizeof(DMT_SHW_CB);
    dmt_shw_cb.type = DMT_SH_CB;
    dmt_shw_cb.dmt_db_id = qef_cb->qef_rcb->qef_db_id;
    dmt_shw_cb.dmt_session_id = qef_cb->qef_ses_id;
    dmt_shw_cb.dmt_flags_mask = DMT_M_ATTR;
    dmt_shw_cb.dmt_char_array.data_address = (PTR)NULL;
    dmt_shw_cb.dmt_char_array.data_in_size = 0;
    dmt_shw_cb.dmt_char_array.data_out_size  = 0;
    dmt_shw_cb.dmt_tab_id = tableinfo->tbl_id;

    /*
    ** Allocate memory for:
    ** array of pointers to DMT_ATT_ENTRY
    ** array of DMT_ATT_ENTRY
    ** array of null terminated names
    ** Remember that the number of elements in these arrays must be one greater
    ** than the attribute count (TID is treated as the 0'th attribute)
    */
    dmt_mem_size = (sizeof(DMT_ATT_ENTRY *) * realNumberOfAttributes)
	+ (sizeof(DMT_ATT_ENTRY ) * realNumberOfAttributes)
	+ nametot;

    ulm->ulm_psize = dmt_mem_size;
    if ((status = qec_malloc(ulm)) != E_DB_OK)
    {
        error = ulm->ulm_error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
        return (status);
    }

    *attributeArray = (DMT_ATT_ENTRY **) ulm->ulm_pptr;
    att_pptr = (DMT_ATT_ENTRY **) ulm->ulm_pptr;
    att = (DMT_ATT_ENTRY *) (att_pptr + realNumberOfAttributes);
    nextname = (char *)(att + realNumberOfAttributes);

    /* now fill in the array of pointers to them */
    for (i = 0; i < realNumberOfAttributes; i++, att++)
    {
        att_pptr[i]  = att;
    }
	     
    dmt_shw_cb.dmt_attr_array.ptr_address  = (PTR) att_pptr;
    dmt_shw_cb.dmt_attr_array.ptr_in_count = realNumberOfAttributes;
    dmt_shw_cb.dmt_attr_array.ptr_size     = sizeof(DMT_ATT_ENTRY);

    dmt_shw_cb.dmt_attr_names.ptr_address  = (PTR) nextname;
    dmt_shw_cb.dmt_attr_names.ptr_in_count = 1;
    dmt_shw_cb.dmt_attr_names.ptr_size     = nametot;

    /* call dmt_show() to get description of attributes */
    dmt_shw_cb.dmt_flags_mask = DMT_M_ATTR;
    if ((status = dmf_call(DMT_SHOW, &dmt_shw_cb)) != E_DB_OK)
    {
        error = dmt_shw_cb.error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
        return (status);
    }

    return( status );
}

/*{
** Name: QEA_CHECKCONSTRAINT - specifics for a check constraint
**
** Description:
**	This routine is called by the integrity interpreter.  This
**	routine performs actions specific to check constraints.  This
**	machine goes through the following states in the following order.
**
**	[CONSTRUCT_CK_SELECT_STATEMENT]	Compile a SELECT statement to verify
**					whether the constraint holds on
**					data currently in the table.
**					This only applies if we are adding
**					a CHECK CONSTRAINT to a
**					pre-existing table.
**
**	[EXECUTE_SELECT_STATEMENT]	Execute the above SELECT statement.
**					At the beginning of this state, the
**					SELECT statement has been parsed and
**					optimized into its own action.  This
**					state performs the voodoo necessary
**					to execute the SELECT.
**
**	MAKE_CHECK_RULE			make a statement end rule on the table
**
**	LINK_DEPENDENT_OBJECTS		stuff IIDBDEPENDS with tuples linking
**					the constraint to the rules we just
**					created.
**
**	CLEANUP				release tables, memory, etc.
**
**  qea_checkConstraint will log any appropriate errors. 
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	22-jul-93 (anitap)
**	    Added a new argument to qea_chkerror().
**	9-sep-93 (rickh)
**	    Added new states so we can cook up and execute a verification
**	    query for ALTER TABLE ADD CHECK CONSTRAINT.
**	27-oct-93 (rickh)
**	    Set QEF_EXECUTE_SELECT_STMT after we finish the parsing state
**	    of the SELECT that supports an ALTER TABLE ADD CONSTRAINT
**	    so that QEQ_QUERY will know not to reset the qef_output buffer.
[@history_line@]...
*/

/*
**	States are listed in execution order.
*/

static	struct	state_table_entry	checkStates[ ] =	{

	{
	    CONSTRUCT_CK_SELECT_STATEMENT,
	    textVerifyingCheckConstraint,
	    0,
	    ( 0 )
	},

	{
	    EXECUTE_CK_SELECT_STATEMENT,
	    0,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    ( 0 )
	},

	{
	    MAKE_CHECK_RULE,
	    makeCheckConstraintRuleText,
	    QEA_CHK_LOOK_FOR_ERRORS,
	    ( PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED )
	},

	{
	    LINK_DEPENDENT_OBJECTS,
	    0,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    0
	},

	{
	    CLEANUP,
	    0,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    0
	}
};

#define	NUMBER_OF_CHECK_STATES	\
	( sizeof( checkStates ) / sizeof( struct state_table_entry ) )

static DB_STATUS
qea_checkConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    DB_INTEGRITY	*integrityTuple = details->qci_integrityTuple;
    PST_INFO		*pstInfo =
		( PST_INFO * ) dsh->dsh_row[ details->qci_pstInfo ];
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DB_TAB_ID		*constrainedTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constrainedTableID ];
					 /* id of table constraint is on */
    i4			*integrityNumber =
			( i4  * ) dsh->dsh_row[ details->qci_integrityNumber ];
			/* integrity numberof this constraint */

    DB_TEXT_STRING	*execImmediateText;
    i4			*state = ( i4  * ) dsh->dsh_row[ details->qci_state ];
    DB_TAB_ID		ruleIDarray[ 1 ];
    DB_NAME		ruleName;
    DB_NAME		*ruleNameArray[ 1 ];
    DB_IIDBDEPENDS	*tuplePTR;
    DB_IIDBDEPENDS	*tuples[ 1 ];
    struct state_table_entry	*stateTableEntry;

    /*
    ** nothing else to do for check constraints generated by the
    ** "not null" tag on a column
    */

    if ( integrityTuple->dbi_consflags & CONS_NON_NULLABLE_DATATYPE )
    {
	*state = CLEANUP;
	return( status );
    }

    /*
    ** setup common to CHECK and REFERENTIAL CONSTRAINTS.  point
    ** stateTableEntry at the correct state descriptor
    */

    status = setupStateMachine( qea_act, dsh, checkStates,
		NUMBER_OF_CHECK_STATES, &stateTableEntry );

    /*
    ** we need to generate a statement end rule to enforce the
    ** check constraint.  we may also have to run a query to verify that
    ** the CHECK CONSTRAINT holds on existing data.
    */

    switch ( *state )
    {

	case CONSTRUCT_CK_SELECT_STATEMENT:
	    /*
	    ** We need to tell qeq_query() that the SELECT ANY(1) statement 
	    ** has to be parsed and executed.  This flags qeq_query to
	    ** not clobber the output buffer pointer which the sequencer
	    ** sets up for the SELECT ANY.
	    */
	    qef_cb->qef_callbackStatus |= QEF_EXECUTE_SELECT_STMT;

	    /* now fall through to create the SELECT ANY text */

	case MAKE_CHECK_RULE:

	    /* cook up the text that must be execute immediated */

	    status = ( *(stateTableEntry->textMaker) )
		     ( qea_act, dsh, &execImmediateText );
	    if ( status != E_DB_OK )	break;

	    /*
	    ** stuff the text where the execute immediate logic knows to
	    ** look for it.  stack the rcb.
	    */

	    status = setupForExecuteImmediate( qea_act, dsh, pstInfo,
			execImmediateText,
			( u_i4 ) stateTableEntry->executionFlags );
	    if ( status != E_DB_OK )	break;

	    /* now return.  the text we cooked up will be executed */

	    ADVANCE_TO_NEXT_STATE;
	    break;

	case EXECUTE_CK_SELECT_STATEMENT:

	    /*
	    ** The SELECT action is actually buried inside some EXECUTE
	    ** IMMEDIATE wrappers.  Dig up the SELECT action so it can be
	    ** executed.
	    */

	    status = qea_dsh( qef_cb->qef_rcb, dsh, qef_cb );
	    if ( status != E_DB_OK )	{ break; }

	    /*
	    ** We need to tell qeq_query() that the SELECT ANY(1) statement 
	    ** has to be executed.
	    */
	    qef_cb->qef_callbackStatus |= QEF_EXECUTE_SELECT_STMT;

	    ADVANCE_TO_NEXT_STATE;
	    break;

	case LINK_DEPENDENT_OBJECTS:

	    ADVANCE_TO_NEXT_STATE;	/* which is CLEANUP. bye-bye */

	    /* link the rule we just created to the constraint */

	    /* first get the rule id */

	    MEcopy( ( PTR ) internalConstraintName, DB_MAXNAME,
		    ( PTR ) &ruleName );
	    ruleNameArray[ 0 ] = &ruleName;
	    status = lookupRuleIDs( dsh, constrainedTableID,
			ruleNameArray, 1, ruleIDarray );
	    if ( status != E_DB_OK )	return( status );

	    /* now fill in an IIDBDEPENDS tuple to link the constraint and
	    ** rule */

	    status = makeIIDBDEPENDStuple( dsh, ulm, constrainedTableID,
			( i4 ) *integrityNumber, ( i4 ) DB_CONS, ruleIDarray,
			( i4 ) 0, ( i4 ) DB_RULE, &tuplePTR );
	    if ( status != E_DB_OK )	return( status );

	    /* finally, write the bloody tuple */

	    tuples[ 0 ] = tuplePTR;
	    status = writeIIDBDEPENDStuples( dsh, ulm, tuples, 1 );
	    if ( status != E_DB_OK )	return( status );

	    break;

	case CLEANUP:
	    break;

	default:
	    status = E_DB_ERROR;
	    error = E_QE0002_INTERNAL_ERROR;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    break;

    }	/* end switch on state */

    if ( *state == CLEANUP || ( status != E_DB_OK ) )
    {
	( VOID ) grabResources( qea_act, dsh, CLEANUP );
    }

    return( status );
}

/*{
** Name: qea_referentialConstraint - specifics for a referential constraint
**
** Description:
**	This routine is called by the integrity interpreter.  It
**	routine performs actions specific to referential constraints.  This
**	machine goes through the following states in the following order.
**	"referencingTable" and "referredTable" indicate the CONSTRAINed
**	relationship of the tables,
**	i.e., referencingTable REFERENCES referredTable:
**
**	[CONSTRUCT_SELECT_STATEMENT]	Compile a SELECT statement to verify
**					whether the constraint holds on
**					data currently in the two tables.
**					This only applies if we are adding
**					a REFERENTIAL CONSTRAINT to
**					pre-existing tables.
**
**	[EXECUTE_SELECT_STATEMENT]	Execute the above SELECT statement.
**					At the beginning of this state, the
**					SELECT statement has been parsed and
**					optimized into its own action.  This
**					state performs the voodoo necessary
**					to execute the SELECT.
**
**	MAKE_INDEX_ON_REFING		make an index on the referencing table
**
**	MAKE_INSERT_REFING_PROCEDURE	make a procedure to be called when
**					inserting into the referencing table
**
**	MAKE_UDPATE_REFING_PROCEDURE	make a procedure to be called when
**					updating referencing table
**
**	MAKE_DELETE_REFED_PROCEDURE	make a procedure to be called when
**					tuples are deleted from the referred
**					table
**
**	MAKE_UPDATE_REFED_PROCEDURE	make a procedure to be called when
**					tuples are updated in the referred
**					table
**
**	MAKE_INSERT_REFING_RULE		make a rule that fires when tuples
**					are inserted into the referencing
**					table.  this rule executes the first
**					procedure we made.
**
**	MAKE_UPDATE_REFING_RULE		make a rule that fires when tuples
**					are updated in the referencing table.
**					this rule also executes the first
**					procedure we made.
**
**	MAKE_DELETE_REFED_RULE		make a rule that fires when tuples
**					are deleted from the referred table.
**					this rule executes the second
**					procedure we created.
**
**	MAKE_UPDATE_REFED_RULE		make a rule that fires when tuples
**					are updated in the referred table.
**					this rule executes the last procedure
**					we created.
**
**	LINK_DEPENDENT_OBJECTS		stuff IIDBDEPENDS with tuples linking
**					the constraint to the rules and
**					procedures we just created.  at this
**					time we also populate IIKEYS with
**					tuples describing the constrained
**					columns in the referencing table.
**
**	CLEANUP				release tables, memory, etc.
**
**
**
** Inputs:
**	qea_ahd				action header
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	dsh
**
** Outputs:
**      dsh
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jan-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	22-jul-93 (anitap)
**	    Call qea_chkerror() after creating the E.I. object to check for
**	    any E.I. processing errors and to destroy the E.I. query text
**	    object.
**	9-aug-93 (rickh)
**	    Added two new states to the REFERENTIAL INTEGRITY state machine:
**	    cook up and execute a SELECT statement when adding a REF CONSTRAINT
**	    to a pre-existing table.
**	25-aug-93 (anitap)
**	    Set qef_callbackStatus for EXECUTE_SELECT_STATEMENT state in 
**	    qea_referentialConstraint().
**	27-oct-93 (rickh)
**	    Set QEF_EXECUTE_SELECT_STMT after we finish the parsing state
**	    of the SELECT that supports an ALTER TABLE ADD CONSTRAINT
**	    so that QEQ_QUERY will know not to reset the qef_output buffer.
**	09-feb-94 (andre)
**	    fix for bug 59595:
**		when sending to PSF text of a query used to determine whether 
**		the constraint initially holds, we need to tell PSF that this is
**		a system-generated query so that it would know to avoid permit 
**		checking when pumping the query tree through qrymod; this is 
**		accomplished by setting PST_SYSTEM_GENERATED bit in PST_INFO 
**		structure which will be handed to PSF as a part of processing 
**		the EXECUTE IMMEDIATE SELECT query.
**	28-feb-00 (inkdo01)
**	    Fix for bug 100625 to alter table add foreign key with no index.
**      17-Apr-2001 (horda03) Bug 104402
**          For foreign key constraints, the index cannot be dropped by the user
**          and the index must be PERSISTENCE (but it doesn't need to be UNIQUE).
[@history_line@]...
*/


/*
**	States are listed in execution order.
*/

static	struct	state_table_entry	refStates[ ] =	{

	{
	    CONSTRUCT_SELECT_STATEMENT,
	    textOfSelect,
	    0,
	    ( PST_SYSTEM_GENERATED )
	},

	{
	    EXECUTE_SELECT_STATEMENT,
	    0,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    ( 0 )
	},

	{
	    MAKE_INDEX_ON_REFING,
	    textOfIndexOnRefingTable,
	    QEA_CHK_LOOK_FOR_ERRORS,
	    ( PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT |
	      PST_NOT_UNIQUE )
	},

	{
	    MAKE_INSERT_REFING_PROCEDURE,
	    textOfInsertRefingProcedure,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    MAKE_UPDATE_REFING_PROCEDURE,
	    textOfUpdateRefingProcedure,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    MAKE_DELETE_REFED_PROCEDURE,
	    textOfDeleteRefedProcedure,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    MAKE_UPDATE_REFED_PROCEDURE,
	    textOfUpdateRefedProcedure,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    MAKE_INSERT_REFING_RULE,
	    textOfInsertRefingRule,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    MAKE_UPDATE_REFING_RULE,
	    textOfUpdateRefingRule,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    MAKE_DELETE_REFED_RULE,
	    textOfDeleteRefedRule,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    MAKE_UPDATE_REFED_RULE,
	    textOfUpdateRefedRule,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    PST_NOT_DROPPABLE | PST_SYSTEM_GENERATED | PST_SUPPORTS_CONSTRAINT
	},

	{
	    LINK_DEPENDENT_OBJECTS,
	    0,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    0
	},

	{
	    CLEANUP,
	    0,
	    QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS,
	    0
	}

};

#define	NUMBER_OF_REF_STATES	\
	( sizeof( refStates ) / sizeof( struct state_table_entry ) )

static DB_STATUS
qea_referentialConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    PST_INFO		*pstInfo =
		( PST_INFO * ) dsh->dsh_row[ details->qci_pstInfo ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DB_TAB_ID		*constraintID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constraintID ];
    DB_TEXT_STRING	*execImmediateText;
    i4			*state = ( i4  * ) dsh->dsh_row[ details->qci_state ];
    u_i4		modflags = 0;
    struct state_table_entry	*stateTableEntry;

    /*
    **setup common to CHECK and REFERENTIAL CONSTRAINTS.  point
    ** stateTableEntry at the correct state descriptor
    */

    status = setupStateMachine( qea_act, dsh, refStates,
		NUMBER_OF_REF_STATES, &stateTableEntry );

    /*
    ** We need to generate 1 index, 3 procedures, and 4 rules to support this
    ** referential integrity.  If we're adding a REFERENTIAL CONSTRAINT to a
    ** pre-existing table, we'll have to cook up a SELECT statement to verify
    ** the REFERENTIAL condition.
    */

    switch ( *state )
    {
	/*
	** These are the states that cook up EXECUTE IMMEDIATE text to
	** be pumped at the parser.  For all states except
	** CONSTRUCT_SELECT_STATEMENT, the parser then just calls RDF/QEF
	** to populate the catalogs.  However, for CONSTRUCT_SELECT_STATEMENT,
	** the statement is also pumped through OPF/OPC and turned into an
	** action.
	*/

	case CONSTRUCT_SELECT_STATEMENT:
	    /*
	    ** We need to tell qeq_query() that the SELECT ANY(1) statement 
	    ** has to be parsed and executed.  This flags qeq_query to
	    ** not clobber the output buffer pointer which the sequencer
	    ** sets up for the SELECT ANY.
	    */
	    qef_cb->qef_callbackStatus |= QEF_EXECUTE_SELECT_STMT;

	    /* now fall through to create the SELECT ANY text */

	case MAKE_INDEX_ON_REFING:
	    /* Check for constraint options which obviate need to
	    ** create the index. */
	    if (details->qci_flags & (QCI_OLD_NAMED_INDEX |
					QCI_BASE_INDEX |
					QCI_NO_INDEX) &&
		stateTableEntry->type != CONSTRUCT_SELECT_STATEMENT)
	     ADVANCE_TO_NEXT_STATE;	/* if so, just skip this step */
	    if (details->qci_flags & QCI_SHARED_INDEX)
		modflags = PST_SHARED_CONSTRAINT;
	case MAKE_INSERT_REFING_PROCEDURE:
	case MAKE_UPDATE_REFING_PROCEDURE:
	case MAKE_DELETE_REFED_PROCEDURE:
	case MAKE_UPDATE_REFED_PROCEDURE:
	case MAKE_INSERT_REFING_RULE:
	case MAKE_UPDATE_REFING_RULE:
	case MAKE_DELETE_REFED_RULE:
	case MAKE_UPDATE_REFED_RULE:

	    /* cook up the text that must be execute immediated */

	    status = ( *(stateTableEntry->textMaker) )
		     ( qea_act, dsh, &execImmediateText );
	    if ( status != E_DB_OK )	break;

	    /*
	    ** stuff the text where the execute immediate logic knows to
	    ** look for it.  stack the rcb.
	    */

	    status = setupForExecuteImmediate( qea_act, dsh, pstInfo,
			execImmediateText,
			( modflags ) ? modflags :
			( u_i4 ) stateTableEntry->executionFlags );
	    if ( status != E_DB_OK )	break;

	    /* now return.  the text we cooked up will be executed */

	    ADVANCE_TO_NEXT_STATE;
	    break;

	case EXECUTE_SELECT_STATEMENT:

	    /*
	    ** The SELECT action is actually buried inside some EXECUTE
	    ** IMMEDIATE wrappers.  Dig up the SELECT action so it can be
	    ** executed.
	    */

	    status = qea_dsh( qef_cb->qef_rcb, dsh, qef_cb );
	    if ( status != E_DB_OK )	{ break; }

	    /*
	    ** We need to tell qeq_query() that the SELECT ANY(1) statement 
	    ** has to be executed.
	    */
	    qef_cb->qef_callbackStatus |= QEF_EXECUTE_SELECT_STMT;

	    ADVANCE_TO_NEXT_STATE;
	    break;

	case LINK_DEPENDENT_OBJECTS:

	    ADVANCE_TO_NEXT_STATE;	/* which is CLEANUP. bye-bye */

	    /*
	    ** link the CONSTRAINT to all the indices, rules, and procedures
	    ** we just created.
	    */

	    status = linkRefObjects( qea_act, dsh );
	    if ( status != E_DB_OK )	break;

	    /* make iikey entries for the CONSTRAINT columns */

	    status = populateIIKEY( dsh, ulm, constraintID,
			( PST_COL_ID * ) details->qci_cons_collist );
	    if ( status != E_DB_OK )	break;

	    break;

	case CLEANUP:
	    break;

	default:
	    status = E_DB_ERROR;
	    error = E_QE0002_INTERNAL_ERROR;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    break;

    }	/* end switch on state */

    if ( *state == CLEANUP || ( status != E_DB_OK ) )
    {
	( VOID ) grabResources( qea_act, dsh, CLEANUP );
    }

    return( status );
}

/*{
** Name: qea_referentialConstraint - specifics for a referential constraint
**
** Description:
**	This routine performs setup common to the CHECK and REFERENTIAL
**	CONSTRAINT state machines.  This routine looks for errors from
**	previous execute immediate stages.  This routine gets exclusive
**	locks on the tables touched by the CONSTRAINT.  This routine
**	returns a pointer to the current state's descriptor.
**
**
** Inputs:
**	qea_ahd				action header
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	stateTable			table of states that this DDL
**					constructor cycles through
**	numberOfPossibleStates		number of states in the above table
**
** Outputs:
**      qef_rcb
**	    .error.err_code
**	returnedStateTableEntry		pointer to descriptor of current state.
**					this descriptor lives in stateTable
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	The state and initial state variables pointed to by
**	dsh->dsh_row[ details->qci_state ] and
**	dsh->dsh_row[ details->qci_initialState ] may be changed.
**
** History:
**	9-sep-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/
static DB_STATUS
setupStateMachine( 
    QEF_AHD			*qea_act,
    QEE_DSH			*dsh,
    struct state_table_entry	*stateTable,
    i4				numberOfPossibleStates,
    struct state_table_entry	**returnedStateTableEntry
)
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CREATE_INTEGRITY_STATEMENT	*details =
			&(qea_act->qhd_obj.qhd_createIntegrity);
    char		*constrainedTableName =
			( char * ) &details->qci_cons_tabname;
    char		*referredTableName =
			( char * ) &details->qci_ref_tabname;
    i4			*state = ( i4  * ) dsh->dsh_row[ details->qci_state ];
    i4			*initialState =
			( i4  * ) dsh->dsh_row[ details->qci_initialState ];
    i4			i;
    struct state_table_entry	*stateTableEntry;
    QSF_RCB		qsf_rb;
    i4			constraintType =
			details->qci_integrityTuple->dbi_consflags &
			( CONS_CHECK | CONS_UNIQUE | CONS_REF );

    /* find the instructions for this state */

    for ( i = 0, stateTableEntry = stateTable;
		  i < numberOfPossibleStates;
		  i++,  stateTableEntry++ )
    {
	if ( stateTableEntry->type == *state )	break;
    }
    if ( i >= numberOfPossibleStates ) 
    {
	status = E_DB_ERROR;
	error = E_QE0002_INTERNAL_ERROR;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	*state = CLEANUP;
    }
    *returnedStateTableEntry = stateTableEntry;

    /*
    ** If this is the first state, get exclusive locks on the relevant
    ** tables.
    */

    if ( *state == *initialState )
    {
	status = grabResources( qea_act, dsh, INITIALIZE );
	if ( status != E_DB_OK )	{ *state = CLEANUP; }
    }
    else	/* subsequent state.  not the first one. */
    {
        /*
	** We want to call qea_chkerror() for the subsequent times (but not
        ** the first time around). qea_chkerror() will also take care of
        ** destroying the query text objects.
        */	

	status = qea_chkerror( dsh, &qsf_rb,
			stateTableEntry->chkerrorFlags );
	if ( status != E_DB_OK )	{ *state = CLEANUP; }
    }
	    
    /*
    ** If we issued a SELECT statement to verify whether the CONSTRAINT holds,
    ** then we need to check whether it failed.  If it failed, we roll back
    ** this ADD CONSTRAINT DDL and don't bother creating objects to support
    ** the CONSTRAINT.
    */

    if ( qef_cb->qef_callbackStatus & QEF_SELECT_RETURNED_ROWS )
    {
	switch ( constraintType )
	{
	    case CONS_REF:
		(VOID) qef_error( E_QE0139_CANT_ADD_REF_CONSTRAINT, 0L, status,
		&error, &dsh->dsh_error, 2,
		qec_trimwhite( DB_MAXNAME, constrainedTableName ),
		(PTR) constrainedTableName,
		qec_trimwhite( DB_MAXNAME, referredTableName ),
		(PTR) referredTableName );

		break;

	    case CONS_CHECK:
	    default:
		(VOID) qef_error( E_QE0144_CANT_ADD_CHECK_CONSTRAINT, 0L, status,
		&error, &dsh->dsh_error, 1,
		qec_trimwhite( DB_MAXNAME, constrainedTableName ),
		(PTR) constrainedTableName );

		break;

	}	/* end switch */

	*state = CLEANUP;
	status = E_DB_ERROR;
    }

    /*
    ** Default the following bits to being off.  These bits flag information
    ** between this state machine and the actions it cooks up.  If an action
    ** is flagging the state machine, then we must check for the flag BEFORE
    ** the following code returns the bits to their default state.  Similarly,
    ** if a state needs to flag information to an action, the state must set
    ** the corresponding bit AFTER the following code defaults the flags.
    */

    qef_cb->qef_callbackStatus &=
	~( QEF_SELECT_RETURNED_ROWS | QEF_EXECUTE_SELECT_STMT );

    return( status );
}

/*{
** Name: grabResources - initialize or cleanup resources
**
** Description:
**	This routine initializes or releases resources for creating
**	a CHECK or REFERENTIAL CONSTRAINT.  Currently, the only resources 
**	managed here are the exclusive locks on the CONSTRAINed tables.
**
**	We want to hold exclusive table level locks on both tables
**	throughout this ordeal.  This is to prevent other users from
**	fouling the data in the tables before we have nailed down all
**	the constraint enforcements.
**
** Inputs:
**	qea_act			ALTER TABLE ADD CONSTRAINT action header
**	qef_rcb			qef request block
**	    .qef_cb			session control block
**	initializeOrCleanup	INITIALIZE (grab the resources) or
**				CLEANUP (release them)
**
** Outputs:
**      qef_rcb
**	    .error.err_code
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-aug-93 (rickh)
**	    Created.
**	9-sep-93 (rickh)
**	    Added CHECK CONSTRAINT case.
[@history_line@]...
*/

static	DB_STATUS
grabResources(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    i4			initializeOrCleanup
)
{
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CREATE_INTEGRITY_STATEMENT	*details =
			&(qea_act->qhd_obj.qhd_createIntegrity);
    char		*constrainedTableName =
			( char * ) &details->qci_cons_tabname;
    char		*referredTableName =
			( char * ) &details->qci_ref_tabname;
    DB_TAB_ID		*constrainedTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constrainedTableID ];
					 /* id of table constraint is on */
    DB_TAB_ID		*referredTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_referredTableID ];
			/* id of foreign table for REFERENTIAL CONSTRAINTs */
    i4			constraintType =
			details->qci_integrityTuple->dbi_consflags &
			( CONS_CHECK | CONS_UNIQUE | CONS_REF );

    switch( initializeOrCleanup )
    {
	case INITIALIZE:

	    if ( status = exclusiveLock( constrainedTableID, dsh ) )
	    {
		switch( constraintType )
		{
		    case CONS_REF:

			(VOID) qef_error( E_QE0140_CANT_EXLOCK_REF_TABLE, 0L,
			status, &error, &dsh->dsh_error, 1,
			qec_trimwhite( DB_MAXNAME, constrainedTableName ),
			(PTR) constrainedTableName );

			break;

		    case CONS_CHECK:
		    default:
			(VOID) qef_error( E_QE0145_CANT_EXLOCK_CHECK_TABLE, 0L,
			status, &error, &dsh->dsh_error, 1,
			qec_trimwhite( DB_MAXNAME, constrainedTableName ),
			(PTR) constrainedTableName );

			break;

		}	/* end switch */

		break;
	    }

	    if ( ( constraintType == CONS_REF ) &&
	         ( status = exclusiveLock( referredTableID, dsh ) ) )
	    {
		(VOID) qef_error( E_QE0140_CANT_EXLOCK_REF_TABLE, 0L, status,
		    &error, &dsh->dsh_error, 1,
		    qec_trimwhite( DB_MAXNAME, referredTableName ),
		    (PTR) referredTableName );
		break;
	    }

	    break;

	case CLEANUP:
	default:
	    /*
	    ** Locks are very sticky.  The only way to peel them off is
	    ** to commit the transaction.  So currently, there's nothing
	    ** to do at cleanup time.
	    */
	    break;

    }	/*	end switch	*/

    return( status );
}

/*{
** Name: exclusiveLock - get an exclusive lock on a table
**
** Description:
**	This routine gets an exclusive lock on a table.
**
**
** Inputs:
**	tableID			ID of table to lock
**	qef_rcb			qef request block
**	    .qef_cb			session control block
**
** Outputs:
**      qef_rcb
**	    .error.err_code
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-aug-93 (rickh)
**	    Created.
[@history_line@]...
*/

static	DB_STATUS
exclusiveLock(
    DB_TAB_ID		*tableID,
    QEE_DSH		*dsh
)
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEU_CB		qeu_cb;

    /*
    ** To get an exclusive lock on a table, you open it with exclusive
    ** table level access then close it immediately.  The lock will
    ** persist until the transaction ends.
    */

    /* open sesame! */

    qeu_cb.qeu_type = QEUCB_CB;
    qeu_cb.qeu_length = sizeof(QEUCB_CB);
    qeu_cb.qeu_db_id = qef_cb->qef_rcb->qef_db_id;
    qeu_cb.qeu_eflag = QEF_INTERNAL;
    qeu_cb.qeu_lk_mode = DMT_X;
    qeu_cb.qeu_flag = DMT_U_DIRECT;
    qeu_cb.qeu_access_mode = DMT_A_WRITE;
    qeu_cb.qeu_mask = 0;
    qeu_cb.qeu_qual = 0;
    qeu_cb.qeu_qarg = 0;
    qeu_cb.qeu_f_qual = 0;
    qeu_cb.qeu_f_qarg = 0;
    qeu_cb.qeu_klen = 0;
    qeu_cb.qeu_key = (DMR_ATTR_ENTRY **) NULL;

    qeu_cb.qeu_acc_id = ( PTR ) NULL;
    qeu_cb.qeu_tab_id.db_tab_base = tableID->db_tab_base;
    qeu_cb.qeu_tab_id.db_tab_index = tableID->db_tab_index;
    status = qeu_open(qef_cb, &qeu_cb);
    if (status != E_DB_OK)
    {
	error = qeu_cb.error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	return( status );
    }

    /* close sesame! */

    status = qeu_close( qef_cb, &qeu_cb );
    if (status != E_DB_OK)
    {
	error = qeu_cb.error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
    }

    return( status );
}

/*{
** Name: qea_uniqueConstraint - specifics for a unique constraint
**
** Description:
**	This routine is called by the integrity interpreter.  This
**	routine performs actions specific to unique constraints.
**
**
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	22-jul-93 (anitap)
**	    Added a new argument to qea_chkerror().
**	2-aug-93 (rickh)
**	    Use the session control block to remember when we're in the middle
**	    of creating a unique constraint.  This enables the dbu code to
**	    emit a different error message if a duplicate error occurs.
**	18-aug-99 (inkdo01)
**	    Slight change to store iikey row(s) for unique constraint using 
**	    base table structure (permits retrieval of referential constraint
**	    key mapping information from catalogs)
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**          When creating a constraint with index = base table structure
**          create the iidbdepends entries (as we do for named indexes) so 
**          that we can identify the dependencies in qeu_dbu().
**        
[@history_line@]...
*/

static DB_STATUS
qea_uniqueConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    PST_INFO		*pstInfo =
		( PST_INFO * ) dsh->dsh_row[ details->qci_pstInfo ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DB_TAB_ID		*constraintID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constraintID ];
    DB_TAB_ID		*constrainedTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constrainedTableID ];
					 /* id of table constraint is on */
    i4			*integrityNumber =
			( i4  * ) dsh->dsh_row[ details->qci_integrityNumber ];
			/* integrity numberof this constraint */

    i4			*state = ( i4  * ) dsh->dsh_row[ details->qci_state ];
    DB_TEXT_STRING	*createIndexText;
    DB_TAB_NAME		*indexName =
	    ( DB_TAB_NAME * ) dsh->dsh_row[ details->qci_nameOfIndex ];
    i4		indexAttributeCount;
    DB_TAB_ID		indexID;
    DB_IIDBDEPENDS	*tuples[ 1 ];
    QSF_RCB		qsf_rb;

    /*
    ** we need to generate a statement level unique index 
    ** to enforce the unique constraint.
    */

    switch ( *state )
    {

	case INITIALIZE:
	    if (details->qci_flags & (QCI_BASE_INDEX | QCI_TABLE_STRUCT))
	    {
		MEcopy ((PTR)&details->qci_cons_tabname.db_tab_name[0], 
		    sizeof(DB_TAB_NAME), (PTR)indexName);
		*state = CLEANUP;
		qef_cb->qef_callbackStatus &= ~( QEF_MAKE_UNIQUE_CONS_INDEX );
		if (details->qci_flags & QCI_BASE_INDEX)
		    goto unique_noindex_splice;
	    }
	    if (details->qci_flags & QCI_OLD_NAMED_INDEX)
	    {
		/* Used existing index, but still need to record dependency. */
		MEcopy ((PTR)&details->qci_idxname.db_tab_name[0], 
		    sizeof(DB_TAB_NAME), (PTR)indexName);
		*state = CLEANUP;
		qef_cb->qef_callbackStatus &= ~( QEF_MAKE_UNIQUE_CONS_INDEX );
		goto unique_noindex_splice;
	    }
	    status = textOfUniqueIndex( qea_act, dsh, &createIndexText );
	    if ( status != E_DB_OK )	return( status );

	    status = setupForExecuteImmediate( qea_act, dsh, pstInfo,
			createIndexText,
			( u_i4 ) ( PST_NOT_DROPPABLE |
					PST_SYSTEM_GENERATED |
		  			PST_SUPPORTS_CONSTRAINT ) );
	    if ( status != E_DB_OK )	return( status );

	    /*
	    ** For error processing, the dbu code will want to know whether
	    ** it is in the middle of creating an index for a unique
	    ** constraint.
	    */

	    qef_cb->qef_callbackStatus |= QEF_MAKE_UNIQUE_CONS_INDEX;

	    /* now return.  the text we cooked up will be executed */

	    *state = LINK_DEPENDENT_OBJECTS;	/* for next time through */
	    break;

	case LINK_DEPENDENT_OBJECTS:

	    *state = CLEANUP;	/* don't come back. */

	    qef_cb->qef_callbackStatus &= ~( QEF_MAKE_UNIQUE_CONS_INDEX );

	    /* check for errors from the preceding execute immediate */

	    status = qea_chkerror( dsh, &qsf_rb,
			QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS );
	    if ( status != E_DB_OK )
	    {	/* error in index creation */
		i4	error;

		error = E_QE013A_CANT_CREATE_CONSTRAINT;
		qef_error( error, 0L, status, &error, &dsh->dsh_error, 1,
		    qec_trimwhite( DB_MAXNAME,
				   (char *)&details->qci_cons_tabname),
		    (PTR) &details->qci_cons_tabname);
		return( status );
	    }

unique_noindex_splice:	/* for now, this is how index creation is bypassed */
	    /* link the index we just created to the constraint */

	    /* lookup the index on the unique key */

	    status = qea_lookupTableID( dsh, indexName,
				&details->qci_cons_ownname,
				&indexID,
				&indexAttributeCount, ( bool ) TRUE );
	    if ( status != E_DB_OK )	return( status );

	    status = makeIIDBDEPENDStuple( dsh, ulm, constrainedTableID,
		( i4 ) *integrityNumber, ( i4 ) DB_CONS, &indexID,
		( i4 ) 0, ( i4 ) DB_INDEX, &tuples[ 0 ] );
	    if ( status != E_DB_OK )	return( status );

	    /* write the bloody tuple and be done with it */

	    status = writeIIDBDEPENDStuples( dsh, ulm, tuples, 1 );
	    if ( status != E_DB_OK )	return( status );

	    /* make iikey entries */

	    status = populateIIKEY( dsh, ulm, constraintID,
			( PST_COL_ID * ) details->qci_cons_collist );
	    if ( status != E_DB_OK )	return( status );

	    break;

    }	/* end switch on state */

    return( status );
}

/*{
** Name: linkRefObjects - link a referential constraint to dependent objects
**
** Description:
**	This routine links a referential constraint to its dependent objects:
**	an index, 3 procedures, and 4 rules.
**
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jan-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	01-apr-93 (rickh)
**	    The latter two rules are on the REFERENCED table.
**	6-apr-93 (rickh)
**	    Made static.
**	27-sep-2002 (gygro01) b108127 / ingsrv1817
**            For referential constraint with QCI_BASE_INDEX flag set, it is
**            not needed to lookup tabid update the index reference in
**            iidbdepends as there is none. Index has already been validated
**            and verified in psl_compare_agst_table().
[@history_line@]...
*/

#define	NUMBER_OF_INDICES		1
#define	NUMBER_OF_PROCEDURES		4
#define	NUMBER_OF_REFERENCING_RULES	2
#define	NUMBER_OF_REFERRED_RULES	2
#define	NUMBER_OF_TUPLES	\
	( NUMBER_OF_REFERENCING_RULES + NUMBER_OF_REFERRED_RULES +	\
	  NUMBER_OF_PROCEDURES + NUMBER_OF_INDICES )

static DB_STATUS
linkRefObjects(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DB_TAB_ID		*constrainedTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_constrainedTableID ];
					 /* id of table constraint is on */
    i4			*integrityNumber =
			( i4  * ) dsh->dsh_row[ details->qci_integrityNumber ];
			/* integrity numberof this constraint */

    DB_TAB_ID		*referredTableID =
	    ( DB_TAB_ID * ) dsh->dsh_row[ details->qci_referredTableID ];
					 /* id of foreign table */
    i4		indexAttributeCount;
    DB_TAB_ID		indexID;
    DB_TAB_ID		procedureIDarray[ NUMBER_OF_PROCEDURES ];
    DB_TAB_ID		referencingRuleIDarray[ NUMBER_OF_REFERENCING_RULES ];
    DB_TAB_ID		referredRuleIDarray[ NUMBER_OF_REFERRED_RULES ];
    DB_NAME		*procedureNameArray[ NUMBER_OF_PROCEDURES ];
    DB_NAME		*referencingRuleNameArray[ NUMBER_OF_REFERENCING_RULES];
    DB_NAME		*referredRuleNameArray[ NUMBER_OF_REFERRED_RULES ];
    DB_IIDBDEPENDS	*tuples[ NUMBER_OF_TUPLES ];
    i4			i, j;
    DB_TAB_NAME		*indexName =
	    ( DB_TAB_NAME * ) dsh->dsh_row[ details->qci_nameOfIndex ];

    DB_NAME		*nameOfInsertRefingProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfInsertRefingProc ];
    DB_NAME		*nameOfUpdateRefingProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfUpdateRefingProc ];
    DB_NAME		*nameOfDeleteRefedProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfDeleteRefedProc ];
    DB_NAME		*nameOfUpdateRefedProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfUpdateRefedProc ];

    DB_NAME		*nameOfInsertRefingRule =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfInsertRefingRule ];
    DB_NAME		*nameOfUpdateRefingRule =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfUpdateRefingRule ];
    DB_NAME		*nameOfDeleteRefedRule =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfDeleteRefedRule ];
    DB_NAME		*nameOfUpdateRefedRule =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfUpdateRefedRule ];


    /* now lookup the IDs of the index, procs, and rules we created */

    /* first the index on the foreign key */

    if (details->qci_flags & QCI_OLD_NAMED_INDEX)
    {
	/* Used existing index, but still need to record dependency. */
	MEcopy ((PTR)&details->qci_idxname.db_tab_name[0], 
		sizeof(DB_TAB_NAME), (PTR)indexName);
    }
    if (!(details->qci_flags & QCI_NO_INDEX) &&
	!(details->qci_flags & QCI_BASE_INDEX))
	status = qea_lookupTableID( dsh, indexName,
				&details->qci_cons_ownname,
				&indexID,
				&indexAttributeCount, ( bool ) TRUE );
    if ( status != E_DB_OK )	return( status );

    /* get the procedure ids */

    procedureNameArray[ 0 ] = nameOfInsertRefingProcedure;
    procedureNameArray[ 1 ] = nameOfUpdateRefingProcedure;
    procedureNameArray[ 2 ] = nameOfDeleteRefedProcedure;
    procedureNameArray[ 3 ] = nameOfUpdateRefedProcedure;
    status = lookupProcedureIDs( dsh, &details->qci_cons_ownname,
			( char ** ) procedureNameArray, NUMBER_OF_PROCEDURES,
			procedureIDarray );
    if ( status != E_DB_OK )	return( status );

    /* now get the ids of rules on the referencing table */

    referencingRuleNameArray[ 0 ] = nameOfInsertRefingRule;
    referencingRuleNameArray[ 1 ] = nameOfUpdateRefingRule;
    status = lookupRuleIDs( dsh, constrainedTableID,
			referencingRuleNameArray, NUMBER_OF_REFERENCING_RULES,
			referencingRuleIDarray );
    if ( status != E_DB_OK )	return( status );

    /* now get the ids of rules on the foreign table */

    referredRuleNameArray[ 0 ] = nameOfDeleteRefedRule;
    referredRuleNameArray[ 1 ] = nameOfUpdateRefedRule;
    status = lookupRuleIDs( dsh, referredTableID,
			referredRuleNameArray, NUMBER_OF_REFERRED_RULES,
			referredRuleIDarray );
    if ( status != E_DB_OK )	return( status );

    /* 
    ** now link the IIDBDEPENDS tuples to link the constraint to
    ** its dependent objects.  that is, link the constraint to its
    ** supporting index, procedures, and rules
    */

    i = 0;
    if (!(details->qci_flags & QCI_NO_INDEX) &&
	!(details->qci_flags & QCI_BASE_INDEX))
	status = makeIIDBDEPENDStuple( dsh, ulm, constrainedTableID,
		( i4 ) *integrityNumber, ( i4 ) DB_CONS, &indexID,
		( i4 ) 0, ( i4 ) DB_INDEX, &tuples[ i++ ] );
    if ( status != E_DB_OK )	return( status );

    for ( j = 0; j < NUMBER_OF_PROCEDURES; j++ )
    {
	status = makeIIDBDEPENDStuple( dsh, ulm, constrainedTableID,
			( i4 ) *integrityNumber, ( i4 ) DB_CONS,
			&procedureIDarray[ j ], ( i4 ) 0, ( i4 ) DB_DBP,
			&tuples[ i++ ] );
	if ( status != E_DB_OK )	return( status );
    }

    for ( j = 0; j < NUMBER_OF_REFERENCING_RULES; j++ )
    {
	status = makeIIDBDEPENDStuple( dsh, ulm, constrainedTableID,
			( i4 ) *integrityNumber, ( i4 ) DB_CONS,
			&referencingRuleIDarray[ j ], ( i4 ) 0,
			( i4 ) DB_RULE, &tuples[ i++ ] );
	if ( status != E_DB_OK )	return( status );
    }

    for ( j = 0; j < NUMBER_OF_REFERRED_RULES; j++ )
    {
	status = makeIIDBDEPENDStuple( dsh, ulm, constrainedTableID,
			( i4 ) *integrityNumber, ( i4 ) DB_CONS,
			&referredRuleIDarray[ j ], ( i4 ) 0,
			( i4 ) DB_RULE, &tuples[ i++ ] );
	if ( status != E_DB_OK )	return( status );
    }

    /* write the bloody tuples and be done with it */

    status = writeIIDBDEPENDStuples( dsh, ulm, tuples, i );
    if ( status != E_DB_OK )	return( status );
    
    return( status );
}

/*
** Name:    qea_record_co_dbp_dependency    - record dependency of a dbproc
**					      charged with enforcing CHECK
**					      OPTION on the view
** Description:
**	This function will look up id of the dbproc charged with enforcing
**	CHECK OPTION on the newly created view and will insert an IIDBDEPENDS
**	tuple describing dependence between the dbproc and the view
**
** Input:
**	qea_act		        action header
**	qef_rcb			QEF request CB
**
** Output:
**	qef_rcb
**	    .error.err_code	filled in if an error occurs
**
** Returns:
**	E_DB_{OK,ERROR}
**
** Side effects:
**	Calls makeIIDBDEPENDStuple() which allocates memory for IIDBDEPENDS
**	tuple
**
** History:
**	12-apr-93 (andre)
**	    written
*/
static DB_STATUS
qea_record_co_dbp_dependency(
    QEF_AHD             *qea_act,
    QEE_DSH		*dsh)
{
    QEF_CREATE_VIEW     *crt_view = &qea_act->qhd_obj.qhd_create_view;
    ULM_RCB             *ulm = (ULM_RCB *) dsh->dsh_row[crt_view->ahd_ulm_rcb];
    
    DB_OWN_NAME		*dbp_owner =
	(DB_OWN_NAME *) dsh->dsh_row[crt_view->ahd_view_owner];
    char    		*dbp_name =
	(char *) dsh->dsh_row[crt_view->ahd_check_option_dbp_name];
    DB_TAB_ID		*view_id =
	(DB_TAB_ID *) dsh->dsh_row[crt_view->ahd_view_id];

    DB_STATUS		status = E_DB_OK;
    DB_TAB_ID		dbp_id;
    DB_IIDBDEPENDS	*dep_tuple;

    /* lookup ID of the dbproc charged with enforcing CHECK OPTION */

    status = lookupProcedureIDs(dsh, dbp_owner, &dbp_name, 1, &dbp_id);
    if (status != E_DB_OK)
    	return(status);

    /* 
    ** now construct an IIDBDEPENDS tuple to link the dbproc charged with
    ** enforcing CHECK OPTION on the view with that view
    */

    status = makeIIDBDEPENDStuple(dsh, ulm, view_id, (i4) 0, (i4) DB_TABLE,
	&dbp_id, ( i4 ) 0, ( i4 ) DB_DBP, &dep_tuple);
    if (status != E_DB_OK)
    	return(status);

    /* insert IIDBDEPENDS tuple */

    status = writeIIDBDEPENDStuples(dsh, ulm, &dep_tuple, 1);
    if (status != E_DB_OK)
    	return(status);
    
    return(status);
}

	/* routines to allocate space for for an evolving text string */

/*{
** Name: initString - initialize the evolving text string
**
** Description:
**	This routine initializes an evolving text string.  For the first
**	pass, we just count up how long the total string will be.
**	For the second pass, we allocate space for that total length
**	and string in the pieces.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/

static DB_STATUS
initString(
    EVOLVING_STRING	*evolvingString,
    i4			pass,
    QEE_DSH		*dsh,
    ULM_RCB		*ulm )
{
    DB_STATUS		status = E_DB_OK;
    i4		error;

    evolvingString->pass = pass;
    
    if ( pass > 0 )
    {
        /* allocate a chunk of memory to hold the concatenated fragments */

        ulm->ulm_psize = evolvingString->length + sizeof( DB_TEXT_STRING ) + 1;
        status = qec_malloc(ulm);
        if (status != E_DB_OK)
        {
            error = ulm->ulm_error.err_code;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
            return (status);
        }

	evolvingString->textString = ( DB_TEXT_STRING * ) ulm->ulm_pptr;
	evolvingString->textString->db_t_count = evolvingString->length;
    }

    evolvingString->length = 0;

    return( status );
}

/*{
** Name: addString - add a string to a list of string fragments
**
** Description:
**	This routine adds a substring to an evolving text string.
**	This is called by a two pass compiler.  On the first pass
**	the length of the substring is added to a running total.
**	On the second pass, after a DB_TEXT_STRING of sufficient size
**	has been allocated, the substring is actually copied into
**	the evolving DB_TEXT_STRING.
**
**
** Inputs:
**
** Outputs:
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/

#define	NO_PUNCTUATION		( 0 )
#define	LEAD_COMMA		( 1 )
#define	LEAD_PERIOD		( 2 )
#define	CLOSE_PARENTHESIS	( 4 )
#define	APPEND_OPEN_PARENTHESIS	( 8 )
#define	PREPEND_AN_AND		( 16 )
#define	PREPEND_AN_EQUALS_SIGN	( 32 )
#define PREPEND_AN_OR		( 64 )
#define	PREPEND_NOT_EQUALS	( 128 )
#define	PREPEND_A_SPACE		( 256 )
#define	PREPEND_A_CLOSEPAREN	( 512 )
#define	PREPEND_A_COMMA		( 1024 )

#define	NO_QUOTES	( 0 )
#define	SINGLE_QUOTE	( 1 )
#define	DOUBLE_QUOTE	( 2 )

static	char	andString[ ] = " AND ";
static	char	equalsString[ ] = " = ";
static	char	orString[ ] = " OR ";
static	char	notEqualsString[ ] = " != ";

static VOID
addString(
    EVOLVING_STRING	*evolvingString,
    char		*subString,
    i4			subStringLength,
    i4			punctuation,
    i4			addQuotes )
{
    i4			totalLength = 0;
    char		*string;

    /* for the first pass, we just add this string's length to the total */

    if ( evolvingString->pass == 0 )
    {
	totalLength += subStringLength;
	if ( punctuation & PREPEND_A_SPACE )	totalLength++;
	if ( punctuation & PREPEND_A_COMMA )	totalLength++;
	if ( punctuation & PREPEND_A_CLOSEPAREN )	totalLength++;
	if ( punctuation & LEAD_COMMA )	totalLength++;
	if ( punctuation & LEAD_PERIOD )	totalLength++;
	if ( punctuation & CLOSE_PARENTHESIS )	totalLength++;
	if ( punctuation & APPEND_OPEN_PARENTHESIS )	totalLength++;
	if ( punctuation & PREPEND_AN_AND )
		totalLength += STlength( andString );
	if ( punctuation & PREPEND_AN_OR )
		totalLength += STlength( orString );
	if ( punctuation & PREPEND_AN_EQUALS_SIGN )
		totalLength += STlength( equalsString );
	if ( punctuation & PREPEND_NOT_EQUALS )
		totalLength += STlength( notEqualsString );
	if ( addQuotes & DOUBLE_QUOTE ) totalLength += 2;
	if ( addQuotes & SINGLE_QUOTE ) totalLength += 2;
    }

    /*
    ** for the second pass, we actually copy the strings into the allocated
    ** space
    */

    else if ( evolvingString->pass == 1 )
    {
	string = ( char * ) &( evolvingString->textString
		->db_t_text[ evolvingString->length ] );

	if ( punctuation & PREPEND_AN_AND )
	{
	    STcopy( andString, &string[ totalLength ] );
	    totalLength += STlength( andString );
	}
	if ( punctuation & PREPEND_AN_OR )
	{
	    STcopy( orString, &string[ totalLength ] );
	    totalLength += STlength( orString );
	}
	if ( punctuation & PREPEND_AN_EQUALS_SIGN )
	{
	    STcopy( equalsString, &string[ totalLength ] );
	    totalLength += STlength( equalsString );
	}
	if ( punctuation & PREPEND_NOT_EQUALS )
	{
	    STcopy( notEqualsString, &string[ totalLength ] );
	    totalLength = STlength( notEqualsString );
	}
	if ( punctuation & PREPEND_A_SPACE )
		string[ totalLength++ ] = ' ';
	if ( punctuation & PREPEND_A_COMMA )
		string[ totalLength++ ] = ',';
	if ( punctuation & PREPEND_A_CLOSEPAREN )
		string[ totalLength++ ] = ')';
	if ( punctuation & LEAD_COMMA )
		string[ totalLength++ ] = ',';
	if ( punctuation & LEAD_PERIOD )
		string[ totalLength++ ] = '.';
	if ( addQuotes & SINGLE_QUOTE )
		string[ totalLength++ ] = ( char ) 0x27;
	if ( addQuotes & DOUBLE_QUOTE )
		string[ totalLength++ ] = '"';

	STncpy( &string[ totalLength ], subString,
	    subStringLength );
	string[ totalLength + subStringLength ] = '\0';
	totalLength += subStringLength;

	if ( addQuotes & DOUBLE_QUOTE )
		string[ totalLength++ ] = '"';
	if ( addQuotes & SINGLE_QUOTE )
		string[ totalLength++ ] = ( char ) 0x27;
	if ( punctuation & CLOSE_PARENTHESIS )
		string[ totalLength++ ] = ')';
	if ( punctuation & APPEND_OPEN_PARENTHESIS )
		string[ totalLength++ ] = '(';

        string[ totalLength ] = EOS;
    }

    evolvingString->length += totalLength;
}

/*{
** Name: endString - finish up an evolving text string
**
** Description:
**	This routine finishes up an evolving text string.  A pointer to
**	the filled in text string is returned.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/

static VOID
endString(
    EVOLVING_STRING	*evolvingString,
    DB_TEXT_STRING	**resultString )
{
    if ( evolvingString->pass == ( TEXT_COMPILATION_PASSES - 1 ) )
    {
        *resultString = evolvingString->textString;
    }
}

/*{
** Name: textVerifyingCheckConstraint - verifying query for check constraints
**
** Description:
**	This routine constructs the query which verifies that the tuples
**	already in a table satisfy the condition in a CHECK CONSTRAINT.
**	This query is executed at ALTER TABLE ADD CHECK CONSTRAINT time.
**
**	The query text looks like this:
**
**		SELECT ANY( 1 ) FROM "schemaName"."tableName"
**			WHERE NOT ( constraintCondition )
**
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	09-sep-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/

static	char	selectAnyFrom[ ] = "SELECT ANY( 1 ) FROM ";

/* schemaName.tableName */

static	char	whereNot[ ] = " WHERE NOT ";

/* constraintCondition */

#define	CHECK_CONDITION_OFFSET	5	/* after the keyword "check" */

static DB_STATUS
textVerifyingCheckConstraint(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**ruleText )
{
    DB_STATUS		status = E_DB_OK;
    EVOLVING_STRING	evolvingString;
    QEF_CREATE_INTEGRITY_STATEMENT	*details =
			&(qea_act->qhd_obj.qhd_createIntegrity);
    ULM_RCB	*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    char		*checkCondition =
			( char * ) details->qci_integrityQueryText->db_t_text +
				CHECK_CONDITION_OFFSET;
    i4			checkConditionLength =
			( i4  ) details->qci_integrityQueryText->db_t_count -
				CHECK_CONDITION_OFFSET;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* SELECT ANY( 1 ) FROM */

	addString( &evolvingString, selectAnyFrom, STlength( selectAnyFrom ),
		NO_PUNCTUATION, NO_QUOTES );

	/* "schemaName"."tablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* WHERE NOT */

	addString( &evolvingString, whereNot,
		   STlength( whereNot ), NO_PUNCTUATION, NO_QUOTES );

	/*
	** constraint condition begins 5 characters into the integrity text,
	** that is, after the CHECK keyword.
	*/

	addString( &evolvingString, checkCondition, checkConditionLength,
		   NO_PUNCTUATION, NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, ruleText );

    }	/* end of two pass compiler */

    return( status );
}

/*{
** Name: MAKECHECKCONSTRAINTRULETEXT - generate rule text for check constraints
**
** Description:
**	This routine constructs rule text for check constraints.
**
**	The rule text looks like this:
**
**		CREATE RULE constraintName AFTER UPDATE( columnList ),
**			INSERT INTO "tableName"
**			WHERE NOT ( constraintCondition )
**			EXECUTE PROCEDURE IIERROR( errono = errorNumber,
**						detail = 0,
**						p_count = 3,
**						p1 = 'Check',
**						p2 = 'constraintName',
**						p3 = '"tableName"' )
**
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	08-may-93 (andre)
**	    if the constraint does not involve any of the columns of the table,
**	    the rule will be created for UPDATE (no parens), INSERT
[@history_line@]...
*/

static	char	createRule[ ] = "CREATE RULE ";
					/*   constraintName */
static	char	afterUpdate[ ] = " AFTER UPDATE( ";
						/* column list */
static	char	insertInto[ ] = " ), INSERT INTO ";
						/*
						** OR, if no columns of the
						** table were involved in the
						** <search condition>:
						*/
static char	afterUpdateInsertInto[] = " AFTER UPDATE, INSERT INTO ";
						/* "schemaName"."tablename" */
static	char	justWhere[ ] = " WHERE ";

/* check condition */

/* EXECUTE PROCEDURE IIERROR(	errorno = errorNumber,
**				detail = 0,
**				p_count = 3,
**				p1 = 'Check',
**				p2 = 'constraintName',
**				p3 = 'tableName' )
*/

static	char	checkType[ ] = "'Check'";

#define	DECIMAL_DIGITS_IN_A_LONG	15	/* should be large enough */

static DB_STATUS
makeCheckConstraintRuleText(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**ruleText )
{
    DB_STATUS		status = E_DB_OK;
    EVOLVING_STRING	evolvingString;
    QEF_CREATE_INTEGRITY_STATEMENT	*details =
			&(qea_act->qhd_obj.qhd_createIntegrity);
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB	*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    DB_TEXT_STRING	*checkCondition = details->qci_checkRuleText;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;
    char		*constraintName = GET_CONSTRAINT_NAME( );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE RULE schemaName.constraintName AFTER UPDATE( */

	addString( &evolvingString, createRule,
		   STlength( createRule ), NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, internalConstraintName,
		DB_MAXNAME, LEAD_PERIOD, NO_QUOTES );

	/*
	** if there are columns, need to add
	**	AFTER UPDATE (<column list>), INSERT INTO
	** otherwise, add
	**	AFTER UPDATE, INSERT INTO
	*/
	if (details->qci_cons_collist)
	{
	    addString( &evolvingString, afterUpdate,
		       STlength( afterUpdate ), NO_PUNCTUATION, NO_QUOTES );

	    /* construct column list */

	    status = stringInColumnList( dsh,
			( PST_COL_ID * ) details->qci_cons_collist,
			constrainedTableAttributeArray, &evolvingString, ulm,
			NO_POSTFIX, DB_MAX_COLS, ( char * ) NULL, ( char * ) NULL,
			LEAD_COMMA );
	    if ( status != E_DB_OK )	break;

	    /* ), INSERT INTO */

	    addString( &evolvingString, insertInto,
		       STlength( insertInto ), NO_PUNCTUATION, NO_QUOTES );
	}
	else
	{
	    addString( &evolvingString, afterUpdateInsertInto,
		STlength(afterUpdateInsertInto), NO_PUNCTUATION, NO_QUOTES );
	}

	/* "schemaName"."tablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* WHERE */

	addString( &evolvingString, justWhere,
		   STlength( justWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* check condition is framed with parentheses and prefixed with NOT */

	addString( &evolvingString,
		   ( char * ) checkCondition->db_t_text,
		   ( i4  ) checkCondition->db_t_count, NO_PUNCTUATION,
		   NO_QUOTES );

	/*
	** EXECUTE PROCEDURE IIERROR(	errorno = errorNumber,
	**				detail = 0,
	**				p_count = 3,
	**				p1 = 'Check',
	**				p2 = 'constraintName',
	**				p3 = 'tableName' )
	*/

	status = reportIntegrityViolation( &evolvingString, checkType,
		   constraintName, tableName, dsh );
	if ( status != E_DB_OK )	break;

	/* now concatenate all the string fragments */

	endString( &evolvingString, ruleText );
    }

    return( status );
}

/*{
** Name: STRINGINCOLUMNLIST - Add a column list to an evolving string
**
** Description:
**	This routine adds a column list to an evolving string.  For each
**	attribute in the list, we copy in its double quoted name.  We may
**	also be called on the prefix each attribute name with a string
**	such as "new." or "old."  We may also be asked to follow each
**	attribute name with a predicate such as "is not null" or the
**	attribute's datatype.  And the
**	caller can specify whether we separate attributes with a comma
**	or an AND.
**
**
**
** Inputs:
**      qef_rcb                         qef request block
**	columnList			list of relevant columns
**	attributeArray			names of all columns in the table
**	ulm				memory stream
**
**	postfix				if not null, one of these constants:
**
**					IS_NOT_NULL	add the string
**							"is not null" after
**							each attribute
**
**					DATATYPES	add each attribute's
**							datatype
**					IS_NULL		add the string
**							"is null" after each
**							attribute
**					SET_NULL	add string "= NULL"
**							after each attribute
**
**	maxNumberOfColumns		maximum number of columns to consider
**
**	columnNameSeed			if not null, then we don't grab the
**					attributes' names out of the
**					attributeArray from the table. 
**					Instead, we construct an artificial
**					name for each attribute.  For instance,
**					the columnNameSeed could be the string
**					"new".  In that case, we construct
**					names of the form:  "new1", "new2",
**					..."newn".
**
**	prefix				string to tack onto every attribute
**					name.  this is a string with a built
**					in separator like "old." or "new.".
**
**	listSeparator			what to separate list elements with.
**					this is a constant indicating whether
**					to use a COMMA separator or the
**					string "is not null".
**
** Outputs:
**	evolvingString			where to write the list we compile
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	6-apr-93 (rickh)
**	    Pass pointer to fi2 to addDataType.
**	18-jun-93 (andre)
**	    added code to handle requests to insert IS NULL after every
**	    attribute
**	25-may-98 (inkdo01)
**	    Added support of SET_NULL to build set clause to implement
**	    "on delete/update set null" referential actions.
[@history_line@]...
*/

static char	isNotNullString[ ] = " IS NOT NULL ";
static char     isNullString[ ] = " IS NULL ";
static char     setNullString[ ] = " = NULL ";

static DB_STATUS
stringInColumnList(
    QEE_DSH		*dsh,
    PST_COL_ID		*columnList,
    DMT_ATT_ENTRY	**attributeArray,
    EVOLVING_STRING	*evolvingString,
    ULM_RCB		*ulm,
    i4			postfix,
    i4			maxNumberOfColumns,
    char		*columnNameSeed,
    char		*prefix,
    i4			listSeparator )
{
    DB_STATUS		status = E_DB_OK;
    i4			punctuation = NO_PUNCTUATION;
    i4			i;
    DMT_ATT_ENTRY	*currentAttribute;
    char		*columnName;
    PST_COL_ID		*columnID;
    ADI_FI_DESC		*fi2 = ( ADI_FI_DESC * ) NULL;
    char		columnNumber[ DECIMAL_DIGITS_IN_A_LONG ];
    i4			columnNameSeedLength;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    if ( columnNameSeed != ( char * ) NULL )
	columnNameSeedLength = STlength( columnNameSeed );

    /* get the attribute array for this table */

    /* for each attribute in the list, put its double-quoted name in the
    ** evolving string */

    for ( columnID = columnList, i = 0;
	  ( columnID != ( PST_COL_ID * ) NULL ) && ( i < maxNumberOfColumns );
          columnID = columnID->next, i++ )
    {
	currentAttribute = attributeArray[ ( i4) columnID->col_id.db_att_id ];

	/* prepend the prefix string if supplied */

	if ( prefix != ( char * )  NULL )
	{
	    addString( evolvingString, prefix, STlength( prefix ),
			punctuation, NO_QUOTES );

	    punctuation = NO_PUNCTUATION;
	}

	/*
	** If our caller supplied a column name seed, we will use it as
	** the seed for the column names.  That is, we won't use the column
	** names supplied in the attribute array.  Instead, we will construct
	** names of the form columnNameSeed1, columnNameSeed2, ...
	*/

	if ( columnNameSeed != ( char * ) NULL )
	{
	    addString( evolvingString, columnNameSeed,
			columnNameSeedLength, punctuation, NO_QUOTES );

	    CVna( i + 1, columnNumber );
	    addString( evolvingString, columnNumber,
		   STlength( columnNumber ), NO_PUNCTUATION, NO_QUOTES );
	}
	else
	{
	    columnName = currentAttribute->att_nmstr;
	    ADD_UNNORMALIZED_NAME( evolvingString, columnName,
			currentAttribute->att_nmlen, punctuation, NO_QUOTES );
	}

	switch ( postfix )
	{
	    case DATATYPES:
	        status = addDataType( dsh, currentAttribute, evolvingString,
				  &fi2 );
	        if ( status != E_DB_OK )	return( status );
		break;

	    case IS_NOT_NULL:
	        addString( evolvingString, isNotNullString,
		   STlength( isNotNullString ), NO_PUNCTUATION, NO_QUOTES );
		break;

	    case IS_NULL:
	        addString( evolvingString, isNullString,
		   STlength( isNullString ), NO_PUNCTUATION, NO_QUOTES );
		break;

	    case SET_NULL:
	        addString( evolvingString, setNullString,
		   STlength( setNullString ), NO_PUNCTUATION, NO_QUOTES );
		break;

	    case NO_POSTFIX:
	    default:
		break;
	}	/* end switch on postfix */

	punctuation = listSeparator;
    }	/* end for */

    return( status );
}

/*{
** Name: addDataType - Add a datatype description to an evolving string
**
** Description:
**	This routine adds text describing a datatype to an evolving string.
**	The text is supposed to be suitable as a column/parameter
**	specification as in, say, a CREATE PROCEDURE parameter specification.
**
**
**
** Inputs:
**      dsh                             data segment header
**	qea_ahd				action header
**
** Outputs:
**      dsh
**	    .dsh_error.err_code		one of the following
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	6-apr-93 (rickh)
**	    Last argument should be a pointer to a pointer.  Also, aligned
**	    the text buffer.
**      19-July-2005 (nansa02)
**          ii_dv_desc returns a 64-character string now to support 
**          some 64-bit platforms. 
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
[@history_line@]...
*/

static char	notNullString[ ] = " NOT NULL ";

static DB_STATUS
addDataType(
    QEE_DSH		*dsh,
    DMT_ATT_ENTRY	*currentAttribute,
    EVOLVING_STRING	*evolvingString,
    ADI_FI_DESC		**fi2ptr )
{
    ADF_CB		*adf_cb = dsh->dsh_adf_cb;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    ADI_OP_ID		op_dv_desc;
    ADI_OP_NAME         adi_oname;
    DB_DATA_VALUE       dv1;
    DB_DATA_VALUE       rdv;
    ADI_FI_TAB		fitab; 
    ADI_FI_DESC		*fi2 = *fi2ptr;
    ADF_FN_BLK		dv_desc_func;
    char		buff[ 64 + DB_CNTSIZE + DB_ALIGN_SZ + 1];
			/* function now returns 64 chars */
    bool		nullableDatatype;
    DB_TEXT_STRING	*textDescriptor = ( DB_TEXT_STRING * )
			ME_ALIGN_MACRO( ( PTR ) buff, DB_ALIGN_SZ );

    /* ADF setup if first time through.  get function id. */

    if ( fi2 == ( ADI_FI_DESC * ) NULL )
    {
        STcopy("ii_dv_desc", adi_oname.adi_opname);
    	/* Get operator ID base on operator name */
    	status = adi_opid( adf_cb, &adi_oname, &op_dv_desc );
    	/* Get function instance */
        status = adi_fitab( adf_cb, op_dv_desc, &fitab );
	if ( status != E_DB_OK )
	{
	    status = qef_adf_error(&adf_cb->adf_errcb, status,
			qef_cb, &dsh->dsh_error );
            if ( status != E_DB_OK )	return (status);
	}

    	/* Pick the first instance because this function is now DB_ALL_TYPE */
    	fi2 = *fi2ptr = fitab.adi_tab_fi;
    }	/* endif first time through */

    nullableDatatype = ( currentAttribute->att_type < 0 ? TRUE : FALSE );

    /* Set up input data */
    dv1.db_prec = currentAttribute->att_prec;
    dv1.db_datatype = currentAttribute->att_type;
    dv1.db_length = currentAttribute->att_width;
    dv1.db_collID = currentAttribute->att_collID;
    dv1.db_data = NULL;

    /*
    ** The ADF function will pass back a string describing the
    ** datatype, which we want to use as the datatype declaration
    ** for this attribute.  Unfortunately, if the datatype is
    ** nullable, ADF will prefix the datatype with the string
    ** "NULLABLE".  This is fine for a user to read but will
    ** choke the parser if included in the datatype declaration
    ** of, oh say, a procedure parameter.  So we always pass in
    ** the non-nullable version of the datatype.
    */

    if ( nullableDatatype == TRUE )
    {
	dv1.db_datatype = -dv1.db_datatype;
	dv1.db_length -= 1;	/* remove the null indicator */
    }

    /* Set up output buffer */
    rdv.db_datatype = fi2->adi_dtresult;
    rdv.db_length = fi2->adi_lenspec.adi_fixedsize;
    rdv.db_prec = 0;
    rdv.db_collID = -1;
    rdv.db_data = (PTR) textDescriptor;    

    dv_desc_func.adf_fi_id = fi2->adi_finstid;
    dv_desc_func.adf_fi_desc = NULL;
    dv_desc_func.adf_dv_n = 1;
    STRUCT_ASSIGN_MACRO(dv1, dv_desc_func.adf_1_dv);
    STRUCT_ASSIGN_MACRO(rdv, dv_desc_func.adf_r_dv);

    dv_desc_func.adf_pat_flags = AD_PAT_NO_ESCAPE;

    /* Execute ADF function */
    status = adf_func( adf_cb, &dv_desc_func );
    if ( status != E_DB_OK )
    {
	status = qef_adf_error( &adf_cb->adf_errcb, status,
			qef_cb, &dsh->dsh_error );
        if ( status != E_DB_OK )	return (status);
    }

    /* Now we have to copy the datatype description to persistent storage */

    addString( evolvingString, ( char * ) textDescriptor->db_t_text,
		   textDescriptor->db_t_count, PREPEND_A_SPACE, NO_QUOTES );

    /* for non-nullable datatypes, we need to postfix NOT NULL */

    if ( nullableDatatype == FALSE )
    {
        addString( evolvingString, notNullString,
		   STlength( notNullString ), NO_PUNCTUATION, NO_QUOTES );
    }

    return( status );
}

/*{
** Name: compareColumnLists - create text to compare two column lists
**
** Description:
**	This routine adds text to an evolving string.  The text is intended
**	to be an SQL qualification equijoining two column lists.  The output
**	of this routine is a text string (actually an evolving string)
**	like this:
**
**		firstTableName.1stColumnName =
**		    secondTableName.1stColumnName ...
**		AND firstTableName.LastColumnName =
**		    secondTableName.LastColumnName
**
**	If a seed is provided for the left column names, we ignore the
**	attribute names in the left attribute array.  Instead, we construct
**	column names of the form leftColumnNameSeed1, leftColumnNameSeed2,...
**	Similarly if a seed is provided for the right column names.
**
**
**
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action header
**
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/

static DB_STATUS
compareColumnLists(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    PST_COL_ID		*leftTableColumnList,
    DMT_ATT_ENTRY	**leftTableAttributeArray,
    PST_COL_ID		*rightTableColumnList,
    DMT_ATT_ENTRY	**rightTableAttributeArray,
    EVOLVING_STRING	*evolvingString,
    char		*leftTableName,
    char		*rightTableName,
    char		*leftColumnNameSeed,
    char		*rightColumnNameSeed,
    i4			listSeparator,
    char		*comparisonOperator )
{
    DB_STATUS		status = E_DB_OK;
    i4			punctuation = NO_PUNCTUATION;
    i4			leadPeriod;
    char		*leftColumnName;
    char		*rightColumnName;
    PST_COL_ID		*leftColumnID;
    PST_COL_ID		*rightColumnID;
    i4			leftTableNameLength = 0;
    i4			rightTableNameLength = 0;
    char		columnNumber[ DECIMAL_DIGITS_IN_A_LONG ];
    i4			leftColumnNameSeedLength = 0;
    i4			rightColumnNameSeedLength = 0;
    i4			i;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    if ( leftColumnNameSeed != ( char * ) NULL )
	leftColumnNameSeedLength = STlength( leftColumnNameSeed );

    if ( rightColumnNameSeed != ( char * ) NULL )
	rightColumnNameSeedLength = STlength( rightColumnNameSeed );

    if ( leftTableName != ( char * ) NULL )
	leftTableNameLength = STlength( leftTableName );

    if ( rightTableName != ( char * ) NULL )
	rightTableNameLength = STlength( rightTableName );

    /* for each attribute in the list, put its double-quoted name in the
    ** evolving string */

    for ( leftColumnID = leftTableColumnList,
		rightColumnID = rightTableColumnList, i = 0;
	  ( leftColumnID != ( PST_COL_ID * ) NULL ) &&
		( rightColumnID != ( PST_COL_ID * ) NULL );
          leftColumnID = leftColumnID->next,
		rightColumnID = rightColumnID->next, i++ )
    {
	/* AND leftTableName */

	if ( leftTableName != ( char * ) NULL )
	{
	    addString( evolvingString, leftTableName, leftTableNameLength,
			punctuation, NO_QUOTES );
	    leadPeriod = LEAD_PERIOD;
	}
	else
	{
	    leadPeriod = punctuation;
	}

	CVna( i + 1, columnNumber );

	/* .leftColumnName */

	if ( leftColumnNameSeedLength != 0 )
	{
	    addString( evolvingString, leftColumnNameSeed,
			leftColumnNameSeedLength, leadPeriod, NO_QUOTES );
	    addString( evolvingString, columnNumber,
		   STlength( columnNumber ), NO_PUNCTUATION, NO_QUOTES );
	}
	else
	{
	    leftColumnName =
	      (leftTableAttributeArray[ ( i4) leftColumnID->col_id.db_att_id])
			->att_nmstr;
	    ADD_UNNORMALIZED_NAME( evolvingString, leftColumnName,
	      (leftTableAttributeArray[ ( i4) leftColumnID->col_id.db_att_id])
			->att_nmlen, leadPeriod, NO_QUOTES );
	}

	/* = rightTableName */

	addString( evolvingString, comparisonOperator,
			STlength( comparisonOperator ), NO_PUNCTUATION,
			NO_QUOTES );

	if ( rightTableName != ( char * ) NULL )
	{
	    addString( evolvingString, rightTableName, rightTableNameLength,
			NO_PUNCTUATION, NO_QUOTES );
	    leadPeriod = LEAD_PERIOD;
	}
	else
	{
	    leadPeriod = NO_PUNCTUATION;
	}

	/* .rightColumnName */

	if ( rightColumnNameSeedLength != 0 )
	{
	    addString( evolvingString, rightColumnNameSeed,
			rightColumnNameSeedLength, leadPeriod, NO_QUOTES );
	    addString( evolvingString, columnNumber,
		   STlength( columnNumber ), NO_PUNCTUATION, NO_QUOTES );
	}
	else
	{
	    rightColumnName =
	      (rightTableAttributeArray[ (i4)rightColumnID->col_id.db_att_id])
			->att_nmstr;
	    ADD_UNNORMALIZED_NAME( evolvingString, rightColumnName,
	      (rightTableAttributeArray[ (i4)rightColumnID->col_id.db_att_id])
			->att_nmlen, leadPeriod, NO_QUOTES );
	}

	punctuation = listSeparator;
    }	/* end for */

    return( status );
}

/*{
** Name: qea_reserveID - reserve a DMF table id
**
** Description:
**	This routine reserves a DMF table id for use by its caller.
**
**
** Inputs:
**	qef_rcb			global request block
**
** Outputs:
**	tableID			filled in
**
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	07-jul-93 (anitap)
**          Modified the arguments so that qea_reserveID() can be called
**          from the QEU routines.
**      06-aug-93 (anitap)
**          Removed extra argument from qea_reserveID().
**	    Typecast error in qef_error() call.
*/

DB_STATUS
qea_reserveID(
    DB_TAB_ID	*tableID,
    QEF_CB	*qef_cb,
    DB_ERROR    *err)
{
    DMU_CB	dmu_cb;
    DB_STATUS	status = E_DB_OK;
    i4	error;

    /* Get a unique object id (table id) from DMF. */
	
    dmu_cb.type = DMU_UTILITY_CB;
    dmu_cb.length = sizeof(DMU_CB);
    dmu_cb.dmu_flags_mask = 0;
    dmu_cb.dmu_tran_id = qef_cb->qef_dmt_id;
    dmu_cb.dmu_db_id = qef_cb->qef_dbid;

    status = dmf_call(DMU_GET_TABID, &dmu_cb);
    if (status != E_DB_OK)
    {
	error = dmu_cb.error.err_code;
	qef_error(error, 0L, status, &error, err, 0 );
	return( status );
    }	
	
    tableID->db_tab_base = dmu_cb.dmu_tbl_id.db_tab_base;
    tableID->db_tab_index = dmu_cb.dmu_tbl_id.db_tab_index;

    return( status );
}	/* qea_reserveID */

/*{
** Name: qea_0lookupTableInfo - look up table info
**
** Description:
**	This routine looks up table info by name/owner or by id.  Caller must 
**	pass an address of a DMT_TBL_ENTRY structure which will be populated by 
**	a call to dmt_show()
**
**
** Inputs:
**	dsh
**	by_name_and_owner	TRUE if the table info is to be looked up by 
**				name and owner; FALSE if the table id is already
**				available and should be used to lookup the 
**				remainder of table info
**	tableName		if (by_name_and_owner) then name of the table
**	tableOwner		if (by_name_and_owner) then name of the table 
**				owner
**	tableID			if (!by_name_and_owner) then id of the table
**
** Outputs:
**	tableInfo		a DMT_TBL_ENTRY structure describing the 
**				specified table
**
**	Returns:
**	    E_DB_{OK,ERROR}
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	01-mar-93 (andre)
**	    renamed to qea_0lookupTableInfo() 
**
**	    changed the interface to allow requesting info by name/owner or by 
**	    id 
**
**	    changed the interface to return table info inside a DMT_TBL_INFO 
**	    structure instead of returning assorted bits of info
*/

static DB_STATUS
qea_0lookupTableInfo(
    QEE_DSH		*dsh,
    bool		by_name_and_owner,
    DB_TAB_NAME		*tableName,
    DB_OWN_NAME		*tableOwner,
    DB_TAB_ID		*tableID,
    DMT_TBL_ENTRY	*tableInfo,
    bool	reportErrorIfNotFound )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb; /* session control block */
    DMT_SHW_CB		dmt_shw_cb;
    i4		error;
    DB_STATUS		status = E_DB_OK;

    /* boiler plate */

    dmt_shw_cb.length = sizeof(DMT_SHW_CB);
    dmt_shw_cb.type = DMT_SH_CB;
    dmt_shw_cb.dmt_db_id = qef_cb->qef_rcb->qef_db_id;
    dmt_shw_cb.dmt_session_id = qef_cb->qef_ses_id;

    /* parameters specific to a "probe by name" request */
    if (by_name_and_owner)
    {
	STRUCT_ASSIGN_MACRO((*tableName), dmt_shw_cb.dmt_name);
	STRUCT_ASSIGN_MACRO((*tableOwner), dmt_shw_cb.dmt_owner);
	dmt_shw_cb.dmt_flags_mask = DMT_M_NAME | DMT_M_TABLE;
    }
    else
    {
	STRUCT_ASSIGN_MACRO((*tableID), dmt_shw_cb.dmt_tab_id);
	dmt_shw_cb.dmt_flags_mask = DMT_M_TABLE;
    }

    dmt_shw_cb.dmt_table.data_address = (char *) tableInfo;
    dmt_shw_cb.dmt_table.data_in_size = sizeof(*tableInfo);
    dmt_shw_cb.dmt_char_array.data_address = NULL;

    /* please, DMF, divulge your secrets */

    status = dmf_call(DMT_SHOW, &dmt_shw_cb);
    if ( status != E_DB_OK )
    {
	if ( ( ( dmt_shw_cb.error.err_code == E_DM0054_NONEXISTENT_TABLE ) ||
	       ( dmt_shw_cb.error.err_code == E_DM0114_FILE_NOT_FOUND )
	     ) &&
	     ( reportErrorIfNotFound == FALSE )
	   )
	{
	    status = E_DB_WARN;
	}
	else
	{
	    error = dmt_shw_cb.error.err_code;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    return( E_DB_ERROR );
	}
    }	
	
    return( status );
}	/* qea_0lookupTableInfo */

/*{
** Name: qea_lookupTableID - look up a DMF table id given a name and owner
**
** Description:
**	This routine looks up a DMF table id by name/owner.
**
**
** Inputs:
**	dsh			global request block
**
** Outputs:
**	tableID			filled in
**
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	03-mar-93 (andre)
**	    changed to call qea_0lookupTableInfo()
**	7-apr-93 (rickh)
**	    Initialize status = E_DB_OK.
*/

static DB_STATUS
qea_lookupTableID(
    QEE_DSH	*dsh,
    DB_TAB_NAME	*tableName,
    DB_OWN_NAME	*tableOwner,
    DB_TAB_ID	*tableID,
    i4	*attributeCount,
    bool	reportErrorIfNotFound )
{
    DMT_TBL_ENTRY       tableInfo;
    DB_STATUS		status = E_DB_OK;
    
    status = qea_0lookupTableInfo(dsh, (bool) TRUE,
	tableName, tableOwner, (DB_TAB_ID *) NULL, &tableInfo,
	reportErrorIfNotFound );

    /* all that for this */
    MEcopy( ( PTR ) &tableInfo.tbl_id, sizeof( DB_TAB_ID ), 
			(PTR) tableID );
    *attributeCount = tableInfo.tbl_attr_count;

    return( status );
}	/* qea_lookupTableID */

/*{
** Name: QEA_WRITEINTEGRITYTUPLES - Write querytext tree and integrity tuples.
**
** Description:
**	This routine writes all the tuples needed to support an integrity.
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	27-mar-93 (andre)
**	    when creating a REFERENCES constraint, pass a description of the
**	    UNIQUE or PRIMARY CONSTRAINT and, if the owner of the <referenced
**	    table> is different from that of the <referencing table>, of
**	    privilege on which the constraint depends to qeu_cintg() which will
**	    enter them into IIDBDEPENDS and IIPRIV
**	2-apr-93 (rickh)
**	    Use tableID/integrityNumber instead of constraintID in
**	    IIDBDEPENDS tuples.
**	11-jun-93 (rickh)
**	    When calling qeu_cintg, make sure the returned error is
**	    correctly mapped into a qef_rcb.
**	20-aug-93 (rickh)
**	    When creating a NOT NULL integrity, make sure you close the
**	    base table first.  This is because the table may have been
**	    created with a CREATE TABLE AS SELECT statement and the SELECT
**	    may have left the table open.  A DMF ALTER TABLE call downstream
**	    in qeu_cintg will hang if the table is open.
**	29-oct-93 (stephenb)
**	    Initialize qeuq_cb.qeuq_tabname before calling qeu_cintg(), we need
**	    it to write an audit record.
[@history_line@]...
*/
static DB_STATUS
qea_writeIntegrityTuples(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_INTEGRITY	*tuple1,
    DB_INTEGRITY	*tuple2 )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb; /* session control block */
    QEUQ_CB		qeuq_cb;
    QEF_DATA		firstIntegrityTuple;
    QEF_DATA		secondIntegrityTuple;
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details =
			&(qea_act->qhd_obj.qhd_createIntegrity);
    char		*queryText =
			( char * ) details->qci_integrityQueryText->db_t_text;
    i4			queryTextLength =
			details->qci_integrityQueryText->db_t_count;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    PTR			queryTree = details->qci_queryTree;
    DB_TAB_ID		*tableID = &tuple1->dbi_tabid;
    i4			*integrityNumber =
			( i4  * ) dsh->dsh_row[ details->qci_integrityNumber ];

    /* qeuq_cb boiler plate */

    MEfill ( sizeof( QEUQ_CB ), ( char ) 0, (PTR) &qeuq_cb );

    qeuq_cb.qeuq_eflag  = QEF_INTERNAL;
    qeuq_cb.qeuq_type   = QEUQCB_CB;
    qeuq_cb.qeuq_length = sizeof(QEUQ_CB);
    qeuq_cb.qeuq_rtbl   = &tuple1->dbi_tabid;
    qeuq_cb.qeuq_d_id   = qef_cb->qef_ses_id;
    qeuq_cb.qeuq_db_id  = qef_cb->qef_rcb->qef_db_id;
    qeuq_cb.qeuq_tabname= details->qci_cons_tabname;

    for ( ; ; )	/* code block to break out of */
    {

	/* pack the query text and tree into tuples */

	status = qea_packTuples( ulm, dsh, &qeuq_cb, tableID,
	    ( PTR ) queryText, queryTextLength, queryTree, ( i4 ) 0 );
	if ( DB_FAILURE_MACRO( status ) )	break;

	/* chain the integrity tuples into the request control block */

	qeuq_cb.qeuq_ci = 1;
	firstIntegrityTuple.dt_next = NULL;
	firstIntegrityTuple.dt_size = sizeof(DB_INTEGRITY);
	firstIntegrityTuple.dt_data = ( PTR ) tuple1;
	qeuq_cb.qeuq_int_tup = &firstIntegrityTuple;

	if ( tuple1->dbi_consflags & CONS_KNOWN_NOT_NULLABLE )
	{
	    qeuq_cb.qeuq_ci++;
	    secondIntegrityTuple.dt_next = NULL;
	    secondIntegrityTuple.dt_size = sizeof(DB_INTEGRITY);
	    secondIntegrityTuple.dt_data = ( PTR ) tuple2;
	    firstIntegrityTuple.dt_next = &secondIntegrityTuple;
	}

	/*
	** if the constraint depends on some object(s) and/or privilege(s). pass
	** their description to qeu_cintg() which will be responsible for
	** storing them in IIDBDEPENDS and/or IIPRIV
	*/
	qeuq_cb.qeuq_indep = details->qci_indep;
	
	/*
	** Before calling qeu_cintg, make sure the base table is closed.
	** If this is a NOT NULL CONSTRAINT chained in after a CREATE
	** TABLE AS SELECT, the SELECT will have opened the table.  This
	** will cause a DMF ALTER_TABLE in qeu_cintg to hang.  So we
	** close the table here.
	*/

	status = qea_closeTable( dsh, tableID );
	if ( DB_FAILURE_MACRO( status ) )	break;

	/* write the query text, tree, and integrity tuples */

	status = qeu_cintg( qef_cb, &qeuq_cb );
	STRUCT_ASSIGN_MACRO( qeuq_cb.error, dsh->dsh_error );
	if ( DB_FAILURE_MACRO( status ) )	break;

	/*
	** save the integrity number for later linking to other objects
	** in IIDBDEPENDS.
	*/

	*integrityNumber = qeuq_cb.qeuq_ino;

	break;
    }	/* end of code block */

    return( status );
}

/*{
** Name: qea_closeTable - Close a table in the DSH's list of open tables.
**
** Description:
**	This routine closes a table that the DSH shows as open.
**
**
** Inputs:
**
**	qef_rcb		QEF request control block.  Points at the DSH, which
**			in turn points at the list of open tables for this
**			query plan.
**
**	tableID		table to be closed and unlinked from the chain of
**			open tables for this query.
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-aug-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	4-Jun-2009 (kschendel) b122118
**	    Use close-and-unlink utility.
[@history_line@]...
*/
static DB_STATUS
qea_closeTable(
    QEE_DSH		*dsh,
    DB_TAB_ID		*tableID
)
{
    DB_STATUS		status = E_DB_OK;
    DMT_CB		*dmt_cb, *next_dmt;
    i4		err;

    dmt_cb = dsh->dsh_open_dmtcbs;
    while (dmt_cb != NULL)
    {
	/* Get now in case we close / unlink */
	next_dmt = (DMT_CB *) dmt_cb->q_next;

	/* but is this our table? */

	if ( ( dmt_cb->dmt_id.db_tab_base == tableID->db_tab_base ) &&
	     ( dmt_cb->dmt_id.db_tab_index == tableID->db_tab_index ) )
	{

	    /* close the table */
	    status = qen_closeAndUnlink(dmt_cb, dsh);
	    if (status)
	    {
		qef_error(dmt_cb->error.err_code, 0L, status, &err,
			&dsh->dsh_error, 0);
	    }
	    /* Continue in case multiple opens on the same table ID */
	}
	dmt_cb = next_dmt;
    }	/* end of loop through open tables */

    return( status );
}

/*{
** Name: QEA_PACKTUPLES - Package up querytext and tree tuples.
**
** Description:
**	This routine packs up query text and tree tuples for an action.
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	04-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	11-jan-92 (rickh)
**	    Rewrote to use Andre's logic.
**      06-aug-93 (anitap)
**          Cast rdf_call() to get rid of compiler warnings.
**	21-jun-1997 (nanpr01)
**	    Another segv problem which Roger's RDF change unveiled.
**	24-Jan-2001 (jenjo02)
**	    If RDF fails to get (QEF) memory, garbage collect, 
**	    then retry instead of immediately giving up.
[@history_line@]...
*/
static DB_STATUS
qea_packTuples(
    ULM_RCB		*ulm,
    QEE_DSH		*dsh,
    QEUQ_CB		*qeuq_cb,
    DB_TAB_ID		*tableID,
    PTR			queryText,
    i4			queryTextLength,
    PTR			queryTree,
    i4			viewStatus )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    RDF_CB		rdfcb;
    RDF_CB		*rdf_cb = &rdfcb;
    RDR_RB		*rdf_rb = &rdf_cb->rdf_rb;
    RDF_QT_PACK_RCB	pack_rcb;
    i4			error;

    qeuq_cb->qeuq_flag_mask = 0;
    qeuq_cb->qeuq_permit_mask = 0;

    /* Must set the info blk to NULL */
    rdf_cb->rdf_info_blk    = NULL;

    /* package the query text */

    rdf_rb->rdr_db_id = qef_cb->qef_rcb->qef_db_id;
    rdf_rb->rdr_session_id = qef_cb->qef_ses_id;

    rdf_rb->rdr_l_querytext = queryTextLength;
    rdf_rb->rdr_querytext   = queryText;
    rdf_rb->rdr_r1_distrib = ( DB_DISTRIB ) 0;
	
    rdf_rb->rdr_status	= viewStatus;
	
    rdf_rb->rdr_instr      = RDF_USE_CALLER_ULM_RCB;
	
    rdf_rb->rdf_fac_ulmrcb	= (PTR) ulm;
    rdf_rb->rdr_fcb  = (PTR) NULL;
	
    rdf_rb->rdr_qt_pack_rcb	    = &pack_rcb;
    pack_rcb.rdf_qt_tuples	    = &qeuq_cb->qeuq_qry_tup;
    pack_rcb.rdf_qt_tuple_count = &qeuq_cb->qeuq_cq;
    pack_rcb.rdf_qt_mode	    = DB_INTG;

    /* If RDF can't get (QEF) memory, garbage collect and retry */
    while ( status == E_DB_OK &&
	    (status = rdf_call(RDF_PACK_QTEXT, (PTR)rdf_cb)) &&
	     rdf_cb->rdf_error.err_code == E_UL0005_NOMEM )
    {
	status = qec_mfree(ulm);
    }

    if ( status )
	error = rdf_cb->rdf_error.err_code;
    else if ( queryTree != ( PTR ) NULL )
    {
	/* Get the tree tuples. */

	/* most of the fields in rdf_rb have been set for the previous call */

	rdf_rb->rdr_qry_root_node   = queryTree;
	
	pack_rcb.rdf_qt_tuples	    = &qeuq_cb->qeuq_tre_tup;
	pack_rcb.rdf_qt_tuple_count = &qeuq_cb->qeuq_ct;
	
	rdf_rb->rdr_tabid.db_tab_base = tableID->db_tab_base;
	rdf_rb->rdr_tabid.db_tab_index = tableID->db_tab_index;

	/* If RDF can't get (QEF) memory, garbage collect and retry */
	while ( status == E_DB_OK &&
		(status = rdf_call(RDF_PACK_QTREE, (PTR)rdf_cb)) &&
		 rdf_cb->rdf_error.err_code == E_UL0005_NOMEM )
	{
	    status = qec_mfree(ulm);
	}
	if ( status )
	    error = rdf_cb->rdf_error.err_code;
    }
    else
    {
	qeuq_cb->qeuq_tre_tup	= 0;
	qeuq_cb->qeuq_ct	= 0;
    }

    if ( status )
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );

    return( status );
}

/*{
** Name: makeIIDBDEPENDStuple - Create an IIDBDEPENDS tuple
**
** Description:
**	This routine looks up a dependent object, based on its name
**	and type, and links it to an independent object in an IIDBDEPENDS
**	tuple.
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	30-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/
static DB_STATUS
makeIIDBDEPENDStuple(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DB_TAB_ID		*independentObjectID,
    i4			independentSecondaryID,
    i4			independentObjectType,
    DB_TAB_ID		*dependentObjectID,
    i4			dependentSecondaryID,
    i4			dependentObjectType,
    DB_IIDBDEPENDS	**returnedTuple
)
{
    DB_STATUS		status = E_DB_OK;
    i4		error;
    DB_IIDBDEPENDS	*tuple;

    /* allocate space for the tuple */

    ulm->ulm_psize = sizeof( DB_IIDBDEPENDS );
    if ((status = qec_malloc(ulm)) != E_DB_OK)
    {
        error = ulm->ulm_error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
        return (status);
    }
    *returnedTuple = tuple = ( DB_IIDBDEPENDS * ) ulm->ulm_pptr;

    /* fill in the tuple */

    MEfill ( sizeof( DB_IIDBDEPENDS ), ( char ) 0, ( PTR ) tuple );	

    MEcopy( ( PTR ) independentObjectID, sizeof( DB_TAB_ID ),
	    ( PTR ) &tuple->dbv_indep );
    tuple->dbv_i_qid = independentSecondaryID;
    tuple->dbv_itype = independentObjectType;
    MEcopy( ( PTR ) dependentObjectID, sizeof( DB_TAB_ID ),
	    ( PTR ) &tuple->dbv_dep );
    tuple->dbv_qid = dependentSecondaryID;
    tuple->dbv_dtype = dependentObjectType;

    return( status );
}

/*{
** Name: writeIIDBDEPENDStuples - Write tuples to IIDBDEPENDS catalog
**
** Description:
**	This routine writes a string of tuples to the IIDBDEPENDS catalog.
**
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	30-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	12-apr-93 (andre)
**	    we were linking up one QEF_DATA structure too many
**	28-dec-93 (rickh)
*	    Stuff error code with correct error.
[@history_line@]...
*/
static DB_STATUS
writeIIDBDEPENDStuples(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DB_IIDBDEPENDS	**tuples,
    i4		tupleCount
)
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		closeStatus = E_DB_OK;
    i4		error;
    QEU_CB		qeucb, *qeu = &qeucb;
    QEF_DATA		*QEF_DATAarray;
    i4			i;

    /* allocate an array of QEF_DATAs to point at the tuples */

    ulm->ulm_psize = sizeof( QEF_DATA ) * tupleCount;
    if ((status = qec_malloc(ulm)) != E_DB_OK)
    {
        error = ulm->ulm_error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
        return (status);
    }
    QEF_DATAarray = ( QEF_DATA * ) ulm->ulm_pptr;

    /* point one QEF_DATA at each tuple */

    for (i = tupleCount - 1; i > 0; i--)
	QEF_DATAarray[i-1].dt_next = &QEF_DATAarray[i];

    QEF_DATAarray[tupleCount-1].dt_next = 0;
    
    for ( i = 0; i < tupleCount; i++ )
    {
	QEF_DATAarray[ i ].dt_size = sizeof(DB_IIDBDEPENDS);
	QEF_DATAarray[ i ].dt_data = (PTR) tuples[ i ];
    }

    qeu->qeu_type = QEUCB_CB;
    qeu->qeu_length = sizeof(QEUCB_CB);
    qeu->qeu_db_id = qef_cb->qef_rcb->qef_db_id;
    qeu->qeu_lk_mode = DMT_IX;
    qeu->qeu_flag = DMT_U_DIRECT;
    qeu->qeu_access_mode = DMT_A_WRITE;
    qeu->qeu_mask = 0;
    qeu->qeu_qual = 0;
    qeu->qeu_qarg = 0;
    qeu->qeu_f_qual = 0;
    qeu->qeu_f_qarg = 0;
    qeu->qeu_klen = 0;
    qeu->qeu_key = (DMR_ATTR_ENTRY **) NULL;

    qeu->qeu_tab_id.db_tab_base = DM_B_DEPENDS_TAB_ID;
    qeu->qeu_tab_id.db_tab_index = DM_I_DEPENDS_TAB_ID;	 
    status = qeu_open(qef_cb, qeu);
    if (status != E_DB_OK)
    {
	error = qeu->error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	return( status );
    }

    qeu->qeu_count = tupleCount;
    qeu->qeu_tup_length = sizeof(DB_IIDBDEPENDS);
    qeu->qeu_input = QEF_DATAarray;

    status = qeu_append(qef_cb, qeu);
    if (status != E_DB_OK)
    {
	error = qeu->error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );

	/* don't return. we have to close IIKEY first */
    }

    closeStatus = qeu_close(qef_cb, qeu);    
    if (closeStatus != E_DB_OK)
    {
	error = qeu->error.err_code;
	qef_error( error, 0L, closeStatus, &error, &dsh->dsh_error, 0 );
    }

    return( status );
}

/*{
** Name: lookupProcedureIDs - Lookup the DB_TAB_IDs of a list of procedures
**
** Description:
**	This routine is passed an array of procedure names.  It returns
**	an array of procedure IDs corresponding to those names.
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	30-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	28-dec-93 (rickh)
**	    Return error status correctly.
[@history_line@]...
*/

#define	DBP_KEYS	2

static DB_STATUS
lookupProcedureIDs(
    QEE_DSH		*dsh,
    DB_OWN_NAME		*owner,
    char		**procedureNames,
    i4			count,
    DB_TAB_ID		*IDs )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		closeStatus = E_DB_OK;
    i4		error;
    QEU_CB		qeu;
    QEF_DATA		qef_data;
    DB_PROCEDURE	dbptuple;
    DMR_ATTR_ENTRY	*key_ptr_array[ DBP_KEYS ];
    DMR_ATTR_ENTRY	key_array[ DBP_KEYS ];
    i4			i;
    DB_DBP_NAME		dbpName;

    for (i=0; i< DBP_KEYS; i++)
	key_ptr_array[i] = &key_array[i];

    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_db_id = qef_cb->qef_rcb->qef_db_id;
    qeu.qeu_lk_mode = DMT_IS;
    qeu.qeu_flag = DMT_U_DIRECT;
    qeu.qeu_access_mode = DMT_A_READ;
    qeu.qeu_tab_id.db_tab_base = DM_B_DBP_TAB_ID; 	
    qeu.qeu_tab_id.db_tab_index = DM_I_DBP_TAB_ID; 	
    qeu.qeu_mask = 0;

    status = qeu_open(qef_cb, &qeu);
    if (DB_FAILURE_MACRO(status))
    {
	error = qeu.error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	return( status );
    }

    qeu.qeu_count = 1;
    qeu.qeu_tup_length = sizeof(DB_PROCEDURE);
    qeu.qeu_output = &qef_data;
    qef_data.dt_next = 0;
    qef_data.dt_size = sizeof(DB_PROCEDURE);
    qef_data.dt_data = (PTR)&dbptuple;
    qeu.qeu_getnext = QEU_REPO;

    qeu.qeu_klen = DBP_KEYS;       
    qeu.qeu_key = key_ptr_array;
    key_ptr_array[0]->attr_number = DM_1_DBP_KEY;
    key_ptr_array[0]->attr_operator = DMR_OP_EQ;
    key_ptr_array[0]->attr_value = dbpName.db_dbp_name;
    key_ptr_array[1]->attr_number = DM_2_DBP_KEY;
    key_ptr_array[1]->attr_operator = DMR_OP_EQ;
    key_ptr_array[1]->attr_value = (char *) owner;
	
    qeu.qeu_qual = 0;
    qeu.qeu_qarg = 0;

    /* loop through the procedure names and get their ids */

    for ( i = 0; i < count; i++ )
    {
	STmove( procedureNames[ i ], ' ', DB_DBP_MAXNAME, dbpName.db_dbp_name );
     
	status = qeu_get(qef_cb, &qeu);
        if (status != E_DB_OK)
        {
	    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	        error = E_QE0117_NONEXISTENT_DBP;
	    else
	        error = qeu.error.err_code;

	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    break;
        }

	MEcopy( ( PTR ) &dbptuple.db_procid, sizeof( DB_TAB_ID ),
		( PTR ) &IDs[ i ] );
    }	/* end for */

    /* Close the iiprocedure */
    closeStatus = qeu_close(qef_cb, &qeu);    
    if (DB_FAILURE_MACRO(closeStatus))
    {
	error = qeu.error.err_code;
	qef_error( error, 0L, closeStatus, &error, &dsh->dsh_error, 0 );
    }

    return( status );
}

/*{
** Name: lookupRuleIDs - Lookup the DB_TAB_IDs of a list of rules
**
** Description:
**	This routine is passed an array of rule names.  It returns
**	an array of rule IDs corresponding to those names.
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	30-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	7-apr-93 (rickh)
**	    Initialize getStatus = E_DB_OK.
[@history_line@]...
*/

     
#define	RULE_KEYS	2

static DB_STATUS
lookupRuleIDs(
    QEE_DSH		*dsh,
    DB_TAB_ID		*tableID,
    DB_NAME		**ruleNames,
    i4			count,
    DB_TAB_ID		*IDs )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		getStatus = E_DB_OK;
    i4		error;
    QEU_CB		qeu;
    QEF_DATA		qef_data;
    DB_IIRULE		tuple;
    DMR_ATTR_ENTRY	*key_ptr_array[ RULE_KEYS ];
    DMR_ATTR_ENTRY	key_array[ RULE_KEYS ];
    i4			i;

    /* zero out all the rule IDs */

    for ( i = 0; i < count; i++ )
    {
	MEfill ( sizeof( DB_TAB_ID ), ( char ) 0, ( PTR ) &IDs[ i ] );
    }

    qeu.qeu_type = QEUCB_CB;
    qeu.qeu_length = sizeof(QEUCB_CB);
    qeu.qeu_db_id = qef_cb->qef_rcb->qef_db_id;
    qeu.qeu_lk_mode = DMT_IS;
    qeu.qeu_flag = DMT_U_DIRECT;
    qeu.qeu_access_mode = DMT_A_READ;
    qeu.qeu_tab_id.db_tab_base = DM_B_RULE_TAB_ID; 	
    qeu.qeu_tab_id.db_tab_index = DM_I_RULE_TAB_ID; 	
    qeu.qeu_mask = 0;

    status = qeu_open(qef_cb, &qeu);
    if (DB_FAILURE_MACRO(status))
    {
	error = qeu.error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	return( status );
    }

    /* we will leaf through the IIRULE catalog by primary key (table id) */

    qeu.qeu_count = 1;
    qeu.qeu_tup_length = sizeof(DB_IIRULE);
    qeu.qeu_output = &qef_data;
    qef_data.dt_next = NULL;
    qef_data.dt_size = sizeof(DB_IIRULE);
    qef_data.dt_data = (PTR)&tuple;
    qeu.qeu_getnext = QEU_REPO;

    /* fill in the key (the table id ) */

    for (i=0; i< RULE_KEYS; i++)
	key_ptr_array[i] = &key_array[i];

    qeu.qeu_klen = RULE_KEYS;       
    qeu.qeu_key = key_ptr_array;
    key_ptr_array[0]->attr_number = DM_1_RULE_KEY;
    key_ptr_array[0]->attr_operator = DMR_OP_EQ;
    key_ptr_array[0]->attr_value = ( char * ) &tableID->db_tab_base;
    key_ptr_array[1]->attr_number = DM_2_RULE_KEY;
    key_ptr_array[1]->attr_operator = DMR_OP_EQ;
    key_ptr_array[1]->attr_value = ( char * ) &tableID->db_tab_index;

    qeu.qeu_qual = 0;
    qeu.qeu_qarg = 0;

    /* loop through the RULE names and get their ids */

    for ( ; ; )
    {
	status = qeu_get(qef_cb, &qeu);
        if (status != E_DB_OK)
        {
	    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		status = E_DB_OK;
	    }
	    else
	    {
	        error = qeu.error.err_code;
		qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    }

	    break;
        }
	qeu.qeu_getnext = QEU_NOREPO;

	for ( i = 0; i < count; i++ )
	{
	    if	( STbcompare( ruleNames[ i ]->db_name,
		      DB_RULE_MAXNAME, tuple.dbr_name.db_name, DB_RULE_MAXNAME,
		      TRUE ) == 0
		)
	    {
		MEcopy( ( PTR ) &tuple.dbr_ruleID, sizeof( DB_TAB_ID ),
			( PTR ) &IDs[ i ] );
		break;
	    }
	}	/* end loop through rule names */

    }	/* end loop through RULEs on this table */
    getStatus = status;

    /* Close the iirule */
    status = qeu_close(qef_cb, &qeu);    
    if (DB_FAILURE_MACRO(status))
    {
	error = qeu.error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	return( status );
    }
    if ( getStatus != E_DB_OK )	return( status );

    /* see if any of the rule IDs weren't found */

    for ( i = 0; i < count; i++ )
    {
	if ( ( IDs[ i ].db_tab_base == 0 ) && ( IDs[ i ].db_tab_index == 0 ) )
	{
	    status = E_DB_ERROR;
	    error = E_QE0165_NONEXISTENT_RULE;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	}
    }

    return( status );
}

/*{
** Name: setupForExecuteImmediate - Stack DSH et. al. for execute immediate
**
** Description:
**	This routine sets up the PST_INFO, creates a QSF object from a
**	text string, and stacks the current DSH.  This is the setup
**	needed to return to PSF and have the text string parsed and executed.
**
**
**
**
** Inputs:
**	act			current action header
**	qef_rcb			QEF request control block
**	text			text to pass to PSF
**	executionFlags		flags to put in the status block
**
** Outputs:
**	qef_rcb			DSH stacked
**	qea_info		status block allocated
**
** Side Effects:
**	    none
**
** History:
**	30-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	3-mar-93 (rickh)
**	    Set pst_info bits in here, since they get copied by qea_cobj.
**	07-jul-93 (anitap)
**	    Got rid of tmp_dsh in qea_scontext(). Not used.
**	    Added new argument QSF_RCB to qea_cobj() so that the qea_dobj()
**	    can later destroy the E.I. query text.
**	22-jul-93 (rickh)
**	    Added a trace point to dump execute immediate text to the
**	    terminal.
[@history_line@]...
*/

static DB_STATUS
setupForExecuteImmediate(
    QEF_AHD		*act,
    QEE_DSH		*dsh,
    PST_INFO		*qea_info,
    DB_TEXT_STRING	*text,
    u_i4		executionFlags
)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    QSF_RCB		qsf_rb;
    i4		val1, val2;

    /*
    ** If requested to, dump the execute immediate text to the terminal.
    */

    if (ult_check_macro(&qef_cb->qef_trace, QEF_T_DUMP_EXEC_IMMEDIATE_TEXT,
	    &val1, &val2))
    {	dumpTextToScreen( text ); }

    /*
    ** Save the current DSH context.  This involves copying the qef_rcb
    ** into the current DSH.
    */

    qea_scontext(qef_cb->qef_rcb, dsh, act);

    /*
    ** PSF must be told that it's being called to parse execute
    ** immediate text.  The special handshake for this occurs in
    ** a PST_INFO structure.  You must make sure that your PST_INFO
    ** is allocated out of some memory that will persist across the
    ** PSF call (i.e., allocating a PST_INFO on the stack won't work.
    ** We assume the PST_INFO was allocated out of the DSH.  Let's
    ** zero it out.
    */

    MEfill ( sizeof( PST_INFO ), ( char ) 0, (PTR) qea_info );
    qea_info->pst_execflags = executionFlags;

    /*
    ** Load the text into a QSF object.  The PST_INFO handshake with
    ** PSF is initialized here.
    **
    ** In addition, this routine will set the QEF_EXEIMM_PROC bit.
    ** Next time, this will prevent us from going through the first
    ** time logic in our caller.
    */

    status = qea_cobj(qef_cb->qef_rcb, dsh, qea_info, text, &qsf_rb);
    if (status != E_DB_OK)	return( status );

    /*
    ** Now return.  Magically, the text we just cooked up will be
    ** parsed and executed.
    */

    return( status );
}

/*{
** Name: dumpTextToScreen - Dumps a DB_TEXT_STRING to the terminal
**
** Description:
**	This routine writes a DB_TEXT_STRING to the terminal, dividing
**	it up into 80 character lines.
**
**
**
** Inputs:
**	text			text to spew on the screen
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	22-jul-93 (rickh)
**	    Wrote.
**	09-aug-93 (anitap)
**	    Fixed compiler warning.
[@history_line@]...
*/

#define	LINE_LENGTH	80

static VOID
dumpTextToScreen(
    DB_TEXT_STRING	*text
)
{
    i4		remainingLength, bufferLength, lineLength;
    char	*source;
    char	buffer[ LINE_LENGTH + 3 ];
    SCF_CB      scf_cb;
    DB_STATUS   scf_status;
    bool	firstPrint = TRUE;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID;

    for( remainingLength = ( i4  ) text->db_t_count,
	 source = ( char * ) text->db_t_text;

	 remainingLength > 0;

	 source += LINE_LENGTH
	)
    {
	/* first time through, add a newline */

	bufferLength = 0;
	if ( firstPrint )
	{   buffer[ bufferLength++ ] = '\n'; firstPrint = FALSE; }

	/* copy in a line's worth of text */

	lineLength = (	remainingLength < LINE_LENGTH ) ?
			remainingLength : LINE_LENGTH;
	STncpy(&buffer[ bufferLength ], source, lineLength );
	buffer[ bufferLength + lineLength ] = '\0';

	/* end with a newline */

	bufferLength += lineLength;
	buffer[ bufferLength++ ] = '\n';

	/* this is how you get your printout across the network */

	scf_cb.scf_nbr_union.scf_local_error = 0; /* this is an ingres error number
					** returned to user, use 0 just in case 
					*/
	scf_cb.scf_len_union.scf_blength = ( u_i4 ) bufferLength;
	scf_cb.scf_ptr_union.scf_buffer = buffer;
	scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */
	scf_status = scf_call(SCC_TRACE, &scf_cb);
	if (scf_status != E_DB_OK)
	{
	    TRdisplay("SCF error %d displaying text to user\n",
	    scf_cb.scf_error.err_code);
	}

	/* adjust for next time through */

	remainingLength -= LINE_LENGTH;

    }	/* end loop through the text */

}	/* end dumpTextToScreen */

/*{
** Name: textOfIndexOnRefingTable - cook up the text for creating an index
**
** Description:
**	This routine cooks up the text for creating an index on a foreign
**	key.  That is an index on the REFERRING columns of a referential
**	constraint.
**
**	This is what the text looks like:
**
**		CREATE INDEX $constraintName ON tableName
**		( listOfReferringColumns )
**		WITH PERSISTENCE, STRUCTURE=BTREE;
**
**	If TABLE-STRUCT is requested, we instead generate a modify:
**		MODIFY tableName TO BTREE ON
**		( listOfUniqueColumns )
**		WITH [ COMPRESSION = ([HI]DATA) ] | [ FILLFACTOR = 100 ]
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	14-jun-93 (rickh)
**	    Prefix index name with schema name.
**	13-sep-93 (rickh)
**	    Added WITH PERSISTENCE clause.
**	1-apr-98 (inkdo01)
**	    Add constraint index options to syntax build.
**	3-may-2007 (dougi)
**	    Add support for optional modify of base table to btree on
**	    constrained columns (in lieu of secondary index).
**	28-Jul-2010 (kschendel) SIR 124104
**	    Add code to autostruct to preserve compression in case the table
**	    was created with compression.  Also, tweak autostruct so that
**	    it asks for 100% fillfactor ... lower fillfactors are pointless
**	    for uncompressed btrees (due to the associated data page business).
*/

static	char	createIndex[ ] = "CREATE INDEX ";
		/* $constraintName */
static	char	on[ ] = " ON ";
		/* table name ( referringColumnList */
static	char	withBTREE[ ] = " ) WITH PERSISTENCE, STRUCTURE=";

static	char	modifyTable1[ ] = "MODIFY ";

static	char	toBTREE[ ] = " TO BTREE ON ";

static	char	withFF100[ ] = " WITH FILLFACTOR=100 ";
static	char	withDataComp[ ] = " WITH COMPRESSION=(DATA) ";
static	char	withHidataComp[ ] = " WITH COMPRESSION=(HIDATA) ";

static DB_STATUS
textOfIndexOnRefingTable(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    DB_INTEGRITY	*integrityTuple = details->qci_integrityTuple;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    DB_TAB_NAME		*indexName =
	    ( DB_TAB_NAME * ) dsh->dsh_row[ details->qci_nameOfIndex ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    EVOLVING_STRING	evolvingString;
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    PTR			collistp;
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    bool		indexNameUsed;
    char		preferredIndexName[ DB_TAB_MAXNAME + 1 ];
    char		*actualIndexName = ( char * ) NULL;
    i4			unpaddedLength;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /*
    ** $constraintName
    **
    ** sigh.  Actually, it's not that easy.  IF the user specifed a
    ** constraint name AND there is no index named $userSuppliedName,
    ** THEN we can use that name.  Otherwise, we must
    ** use the internal constraint name constructed from the constraint
    ** ID. Before that, check if index name was explicitly coded, in which
    ** case, it is used.
    */

    if (details->qci_flags & QCI_NEW_NAMED_INDEX)
	actualIndexName = &details->qci_idxname.db_tab_name[0];
    else
    {
	actualIndexName = internalConstraintName;
	if ( details->qci_flags & QCI_CONS_NAME_SPEC )
	{
            status = doesTableWithConsNameExist( dsh,
		integrityTuple->dbi_consname.db_constraint_name,
		&details->qci_cons_ownname,
		preferredIndexName, &indexNameUsed );
            if ( status != E_DB_OK )	return( status );
            if ( indexNameUsed == FALSE )
		actualIndexName = preferredIndexName;
	}
    }

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	if (details->qci_flags & QCI_TABLE_STRUCT)
	{
	    /* MODIFY TABLE "schemaName"."tableName" */
                                         
	    addString( &evolvingString, modifyTable1,
		   STlength( modifyTable1 ), NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		    LEAD_PERIOD, NO_QUOTES );

	    /* TO BTREE ON (column list) */

	    addString( &evolvingString, toBTREE, 
		   STlength( toBTREE ), NO_PUNCTUATION, NO_QUOTES );

	    collistp = (details->qci_key_collist) ? details->qci_key_collist :
						details->qci_cons_collist;
				/* shared indexes use qci_key_collist */
	    status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) collistp,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	    if ( status != E_DB_OK )	break;

	    /* WITH COMPRESSION = (data) (optional).  If no compression,
	    ** add WITH FILLFACTOR = 100 which is best for uncompressed btree.
	    */
	    if (details->qci_autocompress == DMU_COMP_ON)
		addString(&evolvingString, withDataComp, STlength(withDataComp),
			NO_PUNCTUATION, NO_QUOTES);
	    else if (details->qci_autocompress == DMU_COMP_HI)
		addString(&evolvingString, withHidataComp, STlength(withHidataComp),
			NO_PUNCTUATION, NO_QUOTES);
	    else
		addString(&evolvingString, withFF100, STlength(withFF100),
			NO_PUNCTUATION, NO_QUOTES);

	    /* now concatenate all the string fragments */

	    endString( &evolvingString, text );
	}
	else
	{
	    /* CREATE INDEX */
                                         
	    addString( &evolvingString, createIndex,
		   STlength( createIndex ), NO_PUNCTUATION, NO_QUOTES );

	    /* "schemaName"."indexName" */

	    ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

 	    ADD_UNNORMALIZED_NAME( &evolvingString, actualIndexName,
		DB_TAB_MAXNAME, LEAD_PERIOD, NO_QUOTES );

	    /* ON "schemaName"."tableName" ( */

	    addString( &evolvingString, on,
		   STlength( on ), NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		    LEAD_PERIOD | APPEND_OPEN_PARENTHESIS, NO_QUOTES );

	    /* referringColumnList */

	    collistp = (details->qci_key_collist) ? details->qci_key_collist :
						details->qci_cons_collist;
				/* shared indexes use qci_key_collist */
	    status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) collistp,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	    if ( status != E_DB_OK )	break;

	    /* ) WITH STRUCTURE=BTREE */

	    addString( &evolvingString, withBTREE,
		   STlength( withBTREE ), NO_PUNCTUATION, NO_QUOTES );

	    /* Add the structure and any other possible index with options */

	    addWithopts( &evolvingString, details);

	    /* now concatenate all the string fragments */

	    endString( &evolvingString, text );
	}
    }

    /* store the index name in the DSH so we can link it to the constraint
    ** later */

    if (details->qci_flags & QCI_TABLE_STRUCT)
	MEcopy((char *)tableName, DB_TAB_MAXNAME, indexName);
    else
    {
	unpaddedLength = STlength( actualIndexName );
	MEmove( ( u_i2 ) unpaddedLength, ( PTR ) actualIndexName, ' ',
	    ( u_i2 ) DB_TAB_MAXNAME, ( PTR ) indexName );
    }

    return( status );
}

/*{
** Name: doesTableWithConsNameExist - does an existing table have this name
**
** Description:
**	This routine constructs the preferred name for an index supporting
**	a constraint.  If a table by that name already exist, this routine
**	returns true.  Otherwise, this routine returns false.
**
** Inputs:
**	dsh			data segment header
**	constraintName		user specified constraint name
**	owner			owner name
**
** Outputs:
**	preferredIndexName	user specified name with $ prepended to it
**	indexNameUsed 		TRUE if there is already a table with
**				the same name as preferredIndexName.  FALSE
**				otherwise.
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	15-apr-93 (rickh)
**	    Removed blank padding since we can assume it.
[@history_line@]...
*/

static DB_STATUS
doesTableWithConsNameExist(
    QEE_DSH		*dsh,
    char		*constraintName,
    DB_OWN_NAME		*owner,
    char		*preferredIndexName,
    bool		*indexNameUsed )
{
    DB_STATUS		status = E_DB_OK;
    DB_TAB_NAME		tableName;
    DB_TAB_ID		tableID;
    i4		attributeCount;

    /*
    ** the preferred index name consists of the user specified constraint
    ** name with a '$' prepended.
    */

    preferredIndexName[ 0 ] = '$';
    MEcopy( ( PTR ) constraintName, DB_TAB_MAXNAME - 1,
	    ( PTR ) &preferredIndexName[ 1 ] );

    MEcopy( ( PTR ) preferredIndexName, DB_TAB_MAXNAME,
	    ( PTR ) tableName.db_tab_name );

    /* OK.  does it exist? */

    status = qea_lookupTableID( dsh, &tableName, owner, &tableID,
				&attributeCount, ( bool ) FALSE );
    if ( status == E_DB_OK )	/* if a table by that name exists */
    {
	*indexNameUsed = TRUE;
    }
    else if ( status == E_DB_WARN )	/* if no table by that name */
    {
	*indexNameUsed = FALSE;
	status = E_DB_OK;
    }
    /* otherwise, we have a real error on our hands. ouch. */

    return( status );
}

/*{
** Name: textOfInsertRefingProcedures - cook up the text for creating a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint.  This routine is to create the procedure
**	fired at the end of inserts into the referring table.
**
** Inputs:
**	qea_act			action header
**	qef_rcb			request control block
**
** Outputs:
**	text			procedure text is written here
**	procedureName		procedure name is written here for later
**				reference
**
** Side Effects:
**	    none
**
** History:
**	17-jan-94 (rickh)
**	    Created for FIPS CONSTRAINTS (release 6.5).
[@history_line@]...
*/
static DB_STATUS
textOfInsertRefingProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    char		*nameOfInsertRefingProcedure =
	    ( char * ) dsh->dsh_row[ details->qci_nameOfInsertRefingProc ];


    status = textsOfModifyRefingProcedures( qea_act, dsh, text,
		nameOfInsertRefingProcedure, INSERT_REFING_PROCEDURE_ID,
		E_US1906_REFING_FK_INS_VIOLATION );

    return( status );
}

/*{
** Name: textOfUpdateRefingProcedures - cook up the text for creating a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint.  This routine is to create the procedure
**	fired at the end of updates of the referring table.
**
** Inputs:
**	qea_act			action header
**	qef_rcb			request control block
**
** Outputs:
**	text			procedure text is written here
**	procedureName		procedure name is written here for later
**				reference
**
** Side Effects:
**	    none
**
** History:
**	17-jan-94 (rickh)
**	    Created for FIPS CONSTRAINTS (release 6.5).
[@history_line@]...
*/
static DB_STATUS
textOfUpdateRefingProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    char		*nameOfUpdateRefingProcedure =
	    ( char * ) dsh->dsh_row[ details->qci_nameOfUpdateRefingProc ];


    status = textsOfModifyRefingProcedures( qea_act, dsh, text,
		nameOfUpdateRefingProcedure, UPDATE_REFING_PROCEDURE_ID,
		E_US1907_REFING_FK_UPD_VIOLATION );

    return( status );
}

/*{
** Name: textsOfModifyRefingProcedures - cook up the text for creating a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint.  This routine is called twice, once to
**	create the procedure fired at the end of inserts (into the
**	referring table) and then again to create the procedure fired at
**	the end of updates (into the same table).
**
**	This is what the text looks like:
**
**		CREATE PROCEDURE constraintName1
**		( "$A" SET OF 
**		    ( 1stRefingCol itsType, ... lastRefingCol itsType )
**		) AS
**		DECLARE
**		    COUNTER INTEGER;
**		BEGIN
**		    SELECT ANY( 1 ) INTO :COUNTER FROM
**		    ( "$A" "$REFING" LEFT JOIN referredTableName "$REFED"
**		      ON "$REFING".1stRefingCol =
**			 "$REFED".1stRefedCol ...
**		      AND "$REFING".lastRefingCol =
**			 "$REFED".lastRefedCol
**		    )
**		    WHERE "$REFED".1stRefedCol IS NULL;
**
**		    IF COUNTER != 0 THEN
**				EXECUTE PROCEDURE IIERROR(
**						errorno =errorNumber,
**						detail = 0,
**						p_count = 3,
**						p1 = '"refingTableName"',
**						p2 = '"refedTableName"',
**						p3 = 'constraintName' )
**		    ENDIF;
**		END
**
**
** Inputs:
**	qea_act			action header
**	qef_rcb			request control block
**	procedureIdentifier	unique character that goes into the procedure
**				name concocted by this routine
**	errorNumber		error that the procedure should return if
**				the INSERT or UPDATE failed
**
** Outputs:
**	text			procedure text is written here
**	procedureName		procedure name is written here for later
**				reference
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	15-apr-93 (rickh)
**	    Construct procedure name directly into the DSH.
**	17-jan-94 (rickh)
**	    Added extra arguments so this procedure can be called to create
**	    both INSERT and UPDATE driven procedures.
[@history_line@]...
*/

static char	createProcedure[ ] = "CREATE PROCEDURE ";
					/* constraintName1 ( */

static char	setName[ ] = "$A";

static char	setOf[ ] = " SET OF ( ";
					/* column list with types */
static char	asDeclare[ ] =
" ) ) AS DECLARE COUNTER INTEGER; BEGIN SELECT ANY( 1 ) INTO :COUNTER FROM ( ";
					/* "$A" */
static char	refingLeftJoin[ ] = " \"$REFING\" LEFT JOIN ";
					/* referredTableName */
static char	refedOn[ ] = " \"$REFED\" ON ";
					/* compare column lists */
static char	whereRefed[ ] = " ) WHERE \"$REFED\".";
					/* 1stRefedCol */
static char	isNull[ ] =
	" IS NULL;  IF COUNTER != 0 THEN ";

/* EXECUTE PROCEDURE IIERROR(
**				errono = errorNumber,
**				detail = 0,
**				p_count = 3,
**				p1 = '"refingTableName"',
**				p2 = '"refedTableName"',
**				p3 = 'constraintName' )
*/


static char	endProcedure[ ] = "; ENDIF; END";

static char	refing[ ] = "\"$REFING\"";
static char	refed[ ] = "\"$REFED\"";


static DB_STATUS
textsOfModifyRefingProcedures(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text,
    char		*procedureName,
    char		procedureIdentifier,
    i4			errorNumber
)
{
    DB_STATUS		status = E_DB_OK;
    EVOLVING_STRING	evolvingString;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*referredTableName =
			( char * ) &details->qci_ref_tabname;
    char		*referredSchemaName =
			( char * ) &details->qci_ref_ownname;
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*constraintSchemaName =
			( char * ) &details->qci_integritySchemaName;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;
    char		*constraintName = GET_CONSTRAINT_NAME( );

    /* create the procedure name */

    makeObjectName( internalConstraintName, procedureIdentifier,
			procedureName );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString( &evolvingString, createProcedure,
		   STlength( createProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.procedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString, constraintSchemaName, DB_SCHEMA_MAXNAME,
		    NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, procedureName,
	    DB_DBP_MAXNAME, LEAD_PERIOD | APPEND_OPEN_PARENTHESIS, NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* SET OF ( */

	addString( &evolvingString, setOf,
		   STlength( setOf ), NO_PUNCTUATION, NO_QUOTES );

	/* column list with types */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) ) as declare COUNTER INTEGER; BEGIN SELECT ANY( 1 ) INTO :COUNTER
	** FROM ( */

	addString( &evolvingString, asDeclare,
		   STlength( asDeclare ), NO_PUNCTUATION, NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* "$REFING" LEFT JOIN */

	addString( &evolvingString, refingLeftJoin,
		   STlength( refingLeftJoin ), NO_PUNCTUATION, NO_QUOTES );

	/* "referredSchemaName"."referredTablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, referredSchemaName,
		DB_SCHEMA_MAXNAME, NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, referredTableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFED" ON */

	addString( &evolvingString, refedOn,
		   STlength( refedOn ), NO_PUNCTUATION, NO_QUOTES );

	/* compare the column lists */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, refed, ( char * ) NULL,
		    ( char * ) NULL, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) WHERE "$REFED". */

	addString( &evolvingString, whereRefed,
		   STlength( whereRefed ), NO_PUNCTUATION, NO_QUOTES );

	/* 1stRefedColumn */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, 1, ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* IS NULL;  IF COUNTER != 0 THEN */

	addString( &evolvingString, isNull,
		   STlength( isNull ), NO_PUNCTUATION, NO_QUOTES );

	/* EXECUTE PROCEDURE IIERROR(
	**				errorno = errorNumber,
	**				detail = 0,
	**				p_count = 3,
	**				p1 = '"refingTableName"',
	**				p2 = '"refedTableName"',
	**				p3 = 'constraintName' )
	*/

	status = reportRefConsViolation( &evolvingString, dsh,
		errorNumber, tableName, referredTableName, constraintName );
	if ( status != E_DB_OK )	break;

	/* ; ENDIF; END */

	addString( &evolvingString, endProcedure,
		   STlength( endProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfSelect - cook up SELECT statement to verify ref constraint
**
** Description:
**	This routine cooks up a SELECT statement to verify that a
**	referential constraint holds.  This statement will be executed if
**	we're adding a referential constraint to a pre-existing table.  If
**	this SELECT returns 1, then the constraint doesn't hold and we won't
**	bother to cooking up objects to support the constraint.
**
**	This is what the text looks like:
**
**	SELECT ANY(1) FROM
**		    ( referringTable "$REFING" LEFT JOIN referredTable "$REFED" 
**		      ON "$REFING".1stRefingCol =
**			 "$REFED".1stRefedCol ...
**		      AND "$REFING".lastRefingCol =
**			 "$REFED".lastRefedCol
**		    )
**		    WHERE "$REFED".1stRefedCol IS NULL
**		    AND "$REFING".1stRefingCol IS NOT NULL ...
**		    AND "$REFING".lastRefingCol IS NOT NULL
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	9-aug-93 (rickh)
**	    Creation.
**	22-dec-95 (pchang)
**	    The foreign key referential constraint means that every NON_NULL
**	    value in the column(s) of the Referencing table must have an
**	    equivalent value in other column(s) of the Referenced table.
**	    As such, do not attempt to match any NULL value in the column(s)
**	    representing the foreign key, in the Left Outer Join Select
**	    statement constructed (B65876).  Updated description accordingly.
**
[@history_line@]...
*/

static char	selectAny[ ] = "SELECT ANY( 1 ) FROM ( ";
			     /* referringTableName "$REFING" LEFT JOIN        */
			     /* referredTableName  "$REFED"  ON               */
			     /* compare column lists                          */
			     /* ) WHERE "$REFED".1stRefedCol IS NULL          */
			     /*   AND   "$REFING".1stRefingCol IS NOT NULL... */
			     /*   AND   "$REFING".lastRefingCol IS NOT NULL   */
static char	refingPeriod[ ] = "\"$REFING\".";

static DB_STATUS
textOfSelect(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    EVOLVING_STRING	evolvingString;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*referringSchemaName =
			( char * ) &details->qci_cons_ownname;
    char		*referringTableName =
			( char * ) &details->qci_cons_tabname;
    char		*referredTableName =
			( char * ) &details->qci_ref_tabname;
    char		*referredSchemaName =
			( char * ) &details->qci_ref_ownname;
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* SELECT ANY( 1 ) FROM ( */

	addString( &evolvingString, selectAny, STlength( selectAny ),
		NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.referringTableName */

	ADD_UNNORMALIZED_NAME( &evolvingString, referringSchemaName, DB_SCHEMA_MAXNAME,
		    NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, referringTableName,
		DB_TAB_MAXNAME, LEAD_PERIOD , NO_QUOTES );

	/* "$REFING" LEFT JOIN */

	addString( &evolvingString, refingLeftJoin,
		   STlength( refingLeftJoin ), NO_PUNCTUATION, NO_QUOTES );

	/* "referredSchemaName"."referredTablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, referredSchemaName,
		DB_SCHEMA_MAXNAME, NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, referredTableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFED" ON */

	addString( &evolvingString, refedOn,
		   STlength( refedOn ), NO_PUNCTUATION, NO_QUOTES );

	/* compare the column lists */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, refed, ( char * ) NULL,
		    ( char * ) NULL, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) WHERE "$REFED". */

	addString( &evolvingString, whereRefed,
		   STlength( whereRefed ), NO_PUNCTUATION, NO_QUOTES );

	/* 1stRefedColumn */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, 1, ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* IS NULL */

	addString( &evolvingString, isNullString,
		   STlength( isNullString ), NO_PUNCTUATION, NO_QUOTES );

	/* AND "$REFING". */
 
	addString( &evolvingString, andString,
		   STlength( andString ), NO_PUNCTUATION, NO_QUOTES );
 
	/* allRefingColumns IS NOT NULL */
 
	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    IS_NOT_NULL, DB_MAX_COLS, ( char * ) NULL, refingPeriod,
		    PREPEND_AN_AND );
 
	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfDeleteRefedProcedure - cook up the text for creating a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint.  Before building the procedure which 
**	enforces the RESTRICT action for deletes of rows from the 
**	referenced table, it checks for CASCADE or SET NULL, and calls 
**	their respective text building routines instead.
**
**	This is what the RESTRICT text looks like:
**
**		CREATE PROCEDURE constraintName2
**		( "$A" SET OF 
**		    ( 1stRefedCol itsType, ... lastRefedCol itsType )
**		) AS
**		DECLARE
**		    COUNTER INTEGER;
**		BEGIN
**		    SELECT ANY( 1 ) INTO :COUNTER FROM
**		    ( "$A" "$REFED" LEFT JOIN referringTableName "$REFING"
**		      ON "$REFED".1stRefedCol =
**			 "$REFING".1stRefingCol ...
**		      AND "$REFED".lastRefedCol =
**			 "$REFING".lastRefingCol
**		    )
**		    WHERE "$REFING".1stRefingCol IS NOT NULL;
**
**		    IF COUNTER != 0 THEN
**			EXECUTE PROCEDURE IIERROR( errono = errorNumber,
**						detail = 0,
**						p_count = 3,
**						p1 = '"refedTableName"',
**						p2 = '"refingTableName"',
**						p3 = 'constraintName' )
**		    ENDIF;
**		END
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	15-apr-93 (rickh)
**	    Construct procedure name directly into DSH.
**	25-may-98 (inkdo01)
**	    Added support for ON DELETE CASCADE/SET NULL.
**	20-oct-98 (inkdo01)
**	    Added support for ON DELETE RESTRICT/NO ACTION.
[@history_line@]...
*/

/* CREATE PROCEDURE constraintName2 ( "$A" SET OF ( */
					/* column list with types */

/* ) ) AS DECLARE COUNTER INTEGER; BEGIN SELECT ANY( 1 ) INTO :COUNTER
** FROM ( "$A" */

static char	refedLeftJoin[ ] = " \"$REFED\" LEFT JOIN ";
					/* constrainedTableName */
static char	refingOn[ ] = " \"$REFING\" ON ";
					/* compare column lists */
static char	whereRefing[ ] = " ) WHERE \"$REFING\".";
					/* 1stRefingCol */
static char	isNotNull[ ] =
	" IS NOT NULL;  IF COUNTER != 0 THEN ";

/* EXECUTE PROCEDURE IIERROR(	errorno = errorNumber,
**				detail = 0,
**				p_count = 3,
**				p1 = '"refedTableName"',
**				p2 = '"refingTableName"',
**				p3 = 'constraintName' )
*/

/* ; ENDIF; END */

static DB_STATUS
textOfDeleteRefedProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    EVOLVING_STRING	evolvingString;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*refedTableName = ( char * ) &details->qci_ref_tabname;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*nameOfDeleteRefedProcedure =
	    ( char * ) dsh->dsh_row[ details->qci_nameOfDeleteRefedProc ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*constraintSchemaName =
			( char * ) &details->qci_integritySchemaName;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;
    char		*constraintName = GET_CONSTRAINT_NAME( );
    bool		iirestrict;

    /* Check for "ON DELETE CASCADE/SET NULL" first. */
    if (details->qci_flags & QCI_DEL_CASCADE)
	return (textOfDeleteCascadeRefedProcedure(qea_act, dsh, text));
    else if (details->qci_flags & QCI_DEL_SETNULL)
	return (textOfDeleteSetNullRefedProcedure(qea_act, dsh, text));
    else if (details->qci_flags & QCI_DEL_RESTRICT) iirestrict = TRUE;
    else iirestrict = FALSE;

    /* create the procedure name */

    makeObjectName( internalConstraintName, DELETE_REFED_PROCEDURE_ID,
			nameOfDeleteRefedProcedure );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString( &evolvingString, createProcedure,
		   STlength( createProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.procedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString, constraintSchemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfDeleteRefedProcedure,
		   DB_DBP_MAXNAME, LEAD_PERIOD | APPEND_OPEN_PARENTHESIS,
		   NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* SET OF ( */

	addString( &evolvingString, setOf,
		   STlength( setOf ), NO_PUNCTUATION, NO_QUOTES );

	/* column list with types */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) ) as declare COUNTER INTEGER; BEGIN SELECT ANY( 1 ) INTO :COUNTER
	** FROM ( */

	addString( &evolvingString, asDeclare,
		   STlength( asDeclare ), NO_PUNCTUATION, NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* "$REFED" LEFT JOIN */

	addString( &evolvingString, refedLeftJoin,
		   STlength( refedLeftJoin ), NO_PUNCTUATION, NO_QUOTES );

	/* "constrainedSchemaName"."constraineedTablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFING" ON */

	addString( &evolvingString, refingOn,
		   STlength( refingOn ), NO_PUNCTUATION, NO_QUOTES );

	/* compare the column lists */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, refed, ( char * ) NULL,
		    ( char * ) NULL, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) WHERE "$REFING". */

	addString( &evolvingString, whereRefing,
		   STlength( whereRefing ), NO_PUNCTUATION, NO_QUOTES );

	/* 1stRefingColumn */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, 1, ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* IS NOT NULL;  IF COUNTER != 0 THEN */

	addString( &evolvingString, isNotNull,
		   STlength( isNotNull ), NO_PUNCTUATION, NO_QUOTES );

	/*
	** EXECUTE PROCEDURE IIERROR(	errorno = errorNumber,
	**				detail = 0,
	**				p_count = 3,
	**				p1 = '"refedTableName"',
	**				p2 = '"refingTableName"',
	**				p3 = 'constraintName' )
	*/

	status = reportRefConsViolation( &evolvingString, dsh,
		(iirestrict) ? E_US1911_REFINT_RESTR_VIOLATION :
		E_US1908_REFED_PK_DEL_VIOLATION,
		refedTableName, tableName, constraintName );
	if ( status != E_DB_OK )	break;

	/* ; ENDIF; END */

	addString( &evolvingString, endProcedure,
		   STlength( endProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfDeleteCascadeRefedProcedure - cook up the text for 
**	creating a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint.  This procedure will be fired at the
**	end of statements that delete from the referenced table with
**	CASCADE option.
**
**	This is what the text looks like:
**
**		CREATE PROCEDURE constraintName2
**		( "$A" SET OF 
**		    ( 1stRefedCol itsType, ... lastRefedCol itsType )
**		) AS
**		BEGIN
**		    DELETE FROM referringTableName "$REFING"
**		    WHERE EXISTS (SELECT * FROM "$A" "$REFED"
**		      WHERE "$REFED".1stRefedCol =
**			 "$REFING".1stRefingCol ...
**		      AND "$REFED".lastRefedCol =
**			 "$REFING".lastRefingCol
**		    )
**		END
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	15-may-98 (inkdo01)
**	    Written to support DELETE CASCADE option.
[@history_line@]...
*/

/* CREATE PROCEDURE constraintName2 ( "$A" SET OF ( */
					/* column list with types */

static char	asDelete[ ] = ") ) AS BEGIN DELETE FROM ";

static char	refedWhere[ ] = " \"$REFED\" WHERE ";

static char	refingDelete[ ] = " \"$REFING\" WHERE EXISTS (SELECT * FROM ";

static char	endNoIfProcedure[ ] = "; END ";

static DB_STATUS
textOfDeleteCascadeRefedProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    EVOLVING_STRING	evolvingString;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*nameOfDeleteRefedProcedure =
	    ( char * ) dsh->dsh_row[ details->qci_nameOfDeleteRefedProc ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*constraintSchemaName =
			( char * ) &details->qci_integritySchemaName;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;


    /* create the procedure name */

    makeObjectName( internalConstraintName, DELETE_REFED_PROCEDURE_ID,
			nameOfDeleteRefedProcedure );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString( &evolvingString, createProcedure,
		   STlength( createProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.procedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString, constraintSchemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfDeleteRefedProcedure,
		   DB_DBP_MAXNAME, LEAD_PERIOD | APPEND_OPEN_PARENTHESIS,
		   NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* SET OF ( */

	addString( &evolvingString, setOf,
		   STlength( setOf ), NO_PUNCTUATION, NO_QUOTES );

	/* column list with types */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) ) as BEGIN; DELETE FROM */

	addString( &evolvingString, asDelete,
		   STlength( asDelete ), NO_PUNCTUATION, NO_QUOTES );

	/* "constrainedSchemaName"."constraineedTablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFING" WHERE EXISTS (SELECT * FROM " */

	addString( &evolvingString, refingDelete,
		   STlength( refingDelete ), NO_PUNCTUATION, NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* "$REFED" WHERE */

	addString( &evolvingString, refedWhere,
		   STlength( refedWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* compare the column lists */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, refed, ( char * ) NULL,
		    ( char * ) NULL, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* END */

	addString( &evolvingString, endNoIfProcedure,
		   STlength( endNoIfProcedure ), PREPEND_A_CLOSEPAREN, 
		   NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfDeleteSetNullRefedProcedure - cook up the text for 
**	creating a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint.  This procedure will be fired at the
**	end of statements that delete from the referenced table with
**	SET NULL option.
**
**	This is what the text looks like:
**
**		CREATE PROCEDURE constraintName2
**		( "$A" SET OF 
**		    ( 1stRefedCol itsType, ... lastRefedCol itsType )
**		) AS
**		BEGIN
**		    UPDATE referringTableName "$REFING"
**		    SET "$REFING".1stRefingCol = NULL, ...,
**		   	"$REFING".lastRefingCol = NULL
**		    WHERE EXISTS (SELECT * FROM "$A" "$REFED"
**		      WHERE "$REFED".1stRefedCol =
**			 "$REFING".1stRefingCol ...
**		      AND "$REFED".lastRefedCol =
**			 "$REFING".lastRefingCol
**		    )
**		END
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	25-may-98 (inkdo01)
**	    Written to support ON DELETE SET NULL option.
[@history_line@]...
*/

/* CREATE PROCEDURE constraintName2 ( "$A" SET OF ( */
					/* column list with types */

static char	asBeginUpd[ ] = " ) ) AS BEGIN UPDATE ";
					/* referringTable */

static char	refingSet[ ] = " \"$REFING\" SET ";

static char	refingWhere[ ] = "WHERE EXISTS (SELECT * FROM ";
/* static char	endNoIfProcedure[ ] = "; END"; */

static DB_STATUS
textOfDeleteSetNullRefedProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    EVOLVING_STRING	evolvingString;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*nameOfDeleteRefedProcedure =
	    ( char * ) dsh->dsh_row[ details->qci_nameOfDeleteRefedProc ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*constraintSchemaName =
			( char * ) &details->qci_integritySchemaName;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;


    /* create the procedure name */

    makeObjectName( internalConstraintName, DELETE_REFED_PROCEDURE_ID,
			nameOfDeleteRefedProcedure );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString( &evolvingString, createProcedure,
		   STlength( createProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.procedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString, constraintSchemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfDeleteRefedProcedure,
	       DB_DBP_MAXNAME, LEAD_PERIOD | APPEND_OPEN_PARENTHESIS,
	       NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* SET OF ( */

	addString( &evolvingString, setOf,
		   STlength( setOf ), NO_PUNCTUATION, NO_QUOTES );

	/* column list with types */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) ) as BEGIN; UPDATE */

	addString( &evolvingString, asBeginUpd,
		   STlength( asBeginUpd ), NO_PUNCTUATION, NO_QUOTES );

	/* "constrainedSchemaName"."constraineedTablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFING" SET */
	addString( &evolvingString, refingSet,
		   STlength( refingSet ), NO_PUNCTUATION, NO_QUOTES );


	/* assign new refing column list to NULL */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    SET_NULL, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* WHERE EXISTS (SELECT * FROM " */

	addString( &evolvingString, refingWhere,
		   STlength( refingWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* "$REFED" WHERE */

	addString( &evolvingString, refedWhere,
		   STlength( refedWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* compare the column lists */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, refed, ( char * ) NULL,
		    ( char * ) NULL, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* END */

	addString( &evolvingString, endNoIfProcedure,
		   STlength( endNoIfProcedure ), PREPEND_A_CLOSEPAREN, 
		   NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfUpdateRefedProcedure - cook up the text for creating a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint.  This procedure will be fired at the
**	end of statements that delete from the referred table.
**
**	This is what the text looks like:
**
**		CREATE PROCEDURE constraintName3
**		( "$A" SET OF 
**		    ( new1 1stRefedColType, ... newN lastRefedColType,
**		      old1 1stRefedColType, ... oldN lastRefedColType )
**		) AS
**		DECLARE
**		    COUNTER INTEGER;
**		BEGIN
**		    SELECT ANY( 1 ) INTO :COUNTER FROM referringTable "$REFING",
**		    ( "$A" "$DEL" LEFT JOIN
**		      "$A" "$INS"R
**		      ON "$DEL".old1 = "$INS".new1 ...
**		      AND "$DEL".oldN = "$INS".newN
**		    )
**		    WHERE "$REFING".1stRefingCol =
**			  "$DEL".old1 ...
**		    AND "$REFING".lastRefingCol =
**			  "$DEL".oldN
**		    AND "$INS".new1 IS NULL;
**
**		    IF COUNTER != 0 THEN
**			EXECUTE PROCEDURE IIERROR( errono = errorNumber,
**						detail = 0,
**						p_count = 3,
**						p1 = '"refedTableName"',
**						p2 = '"refingTableName"',
**						p3 = 'constraintName' )
**		    ENDIF;
**		END
**
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	15-apr-93 (rickh)
**	    Construct constraint name directly into DSH.
**	25-may-98 (inkdo01)
**	    Added support for ON UPDATE CASCADE/SET NULL.
**	20-oct-98 (inkdo01)
**	    Added dual support of ON UPDATE RESTRICT/NO ACTION.
[@history_line@]...
*/

/* CREATE PROCEDURE constraintName3 ( "$A" SET OF ( */
					/* refed column list with types,
					** new and old */
static char	asDeclareFrom[ ] =
" ) ) AS DECLARE COUNTER INTEGER; BEGIN SELECT ANY( 1 ) INTO :COUNTER FROM ";
					/* referringTable */
static char	refingParenthesis[ ] = " \"$REFING\", ( ";
					/* "$A" */
static char	delLeftJoin[ ] = " \"$DEL\" LEFT JOIN ";
					/* "$A" */
static char	insOn[ ] = " \"$INS\" ON ";
					/* compare old and new column lists */
static char	plainWhere[ ] = " ) WHERE ";
					/* compare refing and deleted columns */
static char	andIsNull[ ] =
" AND \"$INS\".new1 IS NULL;  IF COUNTER != 0 THEN ";

/* EXECUTE PROCEDURE IIERROR(	errorno = errorNumber,
**				detail = 0,
**				p_count = 3,
**				p1 = '"refedTableName"',
**				p2 = '"refingTableName"',
**				p3 = 'constraintName' )
*/

/* ; ENDIF; END */

static char	newString[ ] = "new";
static char	oldString[ ] = "old";
static char	commaString[ ] = " , ";

static char	delString[ ] = "\"$DEL\"";
static char	insString[ ] = "\"$INS\"";

static DB_STATUS
textOfUpdateRefedProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*refedTableName = ( char * ) &details->qci_ref_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    char		*nameOfUpdateRefedProcedure =
	  ( char * ) dsh->dsh_row[ details->qci_nameOfUpdateRefedProc ];
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*constraintSchemaName =
			( char * ) &details->qci_integritySchemaName;
    i4			pass;
    EVOLVING_STRING	evolvingString;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;
    char		*constraintName = GET_CONSTRAINT_NAME( );
    bool		iirestrict;

    /* Check for "ON UPDATE CASCADE/SET NULL" first. */
    if (details->qci_flags & QCI_UPD_CASCADE)
	return (textOfUpdateCascadeRefedProcedure(qea_act, dsh, text));
    else if (details->qci_flags & QCI_UPD_SETNULL)
	return (textOfUpdateSetNullRefedProcedure(qea_act, dsh, text));
    else if (details->qci_flags & QCI_UPD_RESTRICT)
	iirestrict = TRUE;
    else iirestrict = FALSE;

    /* create the procedure name */

    makeObjectName( internalConstraintName, UPDATE_REFED_PROCEDURE_ID,
			nameOfUpdateRefedProcedure );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString( &evolvingString, createProcedure,
		   STlength( createProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.procedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString, constraintSchemaName, DB_SCHEMA_MAXNAME,
		    NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfUpdateRefedProcedure,
	       DB_DBP_MAXNAME, LEAD_PERIOD | APPEND_OPEN_PARENTHESIS,
	       NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* SET OF ( */

	addString( &evolvingString, setOf,
		   STlength( setOf ), NO_PUNCTUATION, NO_QUOTES );

	/* refed column list with types, new and old */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS, newString, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	addString( &evolvingString, commaString,
		   STlength( commaString ), NO_PUNCTUATION, NO_QUOTES );

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS, oldString, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) ) as declare COUNTER INTEGER; BEGIN SELECT ANY( 1 ) INTO :COUNTER
	** FROM  */

	addString( &evolvingString, asDeclareFrom,
		   STlength( asDeclareFrom ), NO_PUNCTUATION, NO_QUOTES );

	/* "referringSchemaName"."referringTableName" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFING", ( */

	addString( &evolvingString, refingParenthesis,
		   STlength( refingParenthesis ), NO_PUNCTUATION, NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* "$DEL" LEFT JOIN */

	addString( &evolvingString, delLeftJoin,
		   STlength( delLeftJoin ), NO_PUNCTUATION, NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* "$INS" ON */

	addString( &evolvingString, insOn,
		   STlength( insOn ), NO_PUNCTUATION, NO_QUOTES );

	/* compare the old and new column lists */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, delString, insString, oldString,
		    newString, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) WHERE */

	addString( &evolvingString, plainWhere,
		   STlength( plainWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* compare refing and deleted columns */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, delString, ( char * ) NULL,
		    oldString, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* AND "$INS".new1 IS NULL;  IF COUNTER != 0 THEN */

	addString( &evolvingString, andIsNull,
		   STlength( andIsNull ), NO_PUNCTUATION, NO_QUOTES );

	/*
	** EXECUTE PROCEDURE IIERROR(	errorno = errorNumber,
	**				detail = 0,
	**				p_count = 3,
	**				p1 = '"refedTableName"',
	**				p2 = '"refingTableName"',
	**				p3 = 'constraintName' )
	*/

	status = reportRefConsViolation( &evolvingString, dsh,
		(iirestrict) ? E_US1911_REFINT_RESTR_VIOLATION :
		E_US1909_REFED_PK_UPD_VIOLATION,
		refedTableName, tableName, constraintName );
	if ( status != E_DB_OK )	break;

	/* ; ENDIF; END */

	addString( &evolvingString, endProcedure,
		   STlength( endProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfUpdateCascadeRefedProcedure - cook up the text for creating 
**	a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint with the "on update cascade" option.  
**	This procedure will be fired at the end of statements that update
**	the referenced columns of the referenced table.
**
**	This is what the text looks like:
**
**		CREATE PROCEDURE constraintName3
**		( "$A" SET OF 
**		    ( new1 1stRefedColType, ... newN lastRefedColType,
**		      old1 1stRefedColType, ... oldN lastRefedColType )
**		) AS
**		BEGIN
**
**		    UPDATE referringTable "$REFING" from "$A" "$REFED"
**			SET 1stRefingCol = "$REFED".new1, ...,
**		   	    lastRefingCol = "$REFED".newn
**			WHERE "$REFING".1stRefingCol = "$REFED".old1 ...
**			    "$REFING".lastRefingCol = "$REFED".oldn;
**
**		END
**
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-may-98 (inkdo01)
**	    Written to support CASCADE.
[@history_line@]...
*/

/* CREATE PROCEDURE constraintName3 ( "$A" SET OF ( */
					/* refed column list with types,
					** new and old */
/* static char	asBeginUpd[ ] = " ) ) AS BEGIN UPDATE "; */
					/* referringTable */
static char	refingFromSet[ ] = " \"$REFING\" FROM \"$A\" \"$REFED\"  SET ";
					/* assign new refed cols to refing */
static char	plainerWhere[ ] = " WHERE ";
					/* compare refing & old refed columns */

/* END */


static DB_STATUS
textOfUpdateCascadeRefedProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    char		*nameOfUpdateRefedProcedure =
	  ( char * ) dsh->dsh_row[ details->qci_nameOfUpdateRefedProc ];
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*constraintSchemaName =
			( char * ) &details->qci_integritySchemaName;
    i4			pass;
    EVOLVING_STRING	evolvingString;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /* create the procedure name */

    makeObjectName( internalConstraintName, UPDATE_REFED_PROCEDURE_ID,
			nameOfUpdateRefedProcedure );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString( &evolvingString, createProcedure,
		   STlength( createProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.procedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString, constraintSchemaName, DB_SCHEMA_MAXNAME,
		    NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfUpdateRefedProcedure,
		   DB_DBP_MAXNAME, LEAD_PERIOD | APPEND_OPEN_PARENTHESIS,
		   NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* SET OF ( */

	addString( &evolvingString, setOf,
		   STlength( setOf ), NO_PUNCTUATION, NO_QUOTES );

	/* refed column list with types, new and old */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS, newString, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	addString( &evolvingString, commaString,
		   STlength( commaString ), NO_PUNCTUATION, NO_QUOTES );

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS, oldString, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) ) as declare BEGIN UPDATE */

	addString( &evolvingString, asBeginUpd,
		   STlength( asBeginUpd ), NO_PUNCTUATION, NO_QUOTES );

	/* "referringSchemaName"."referringTableName" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFING" FROM "$A" "$REFED" SET  */

	addString( &evolvingString, refingFromSet,
		   STlength( refingFromSet ), NO_PUNCTUATION, NO_QUOTES );

	/* assign new refed column list to refing column list */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, ( char * ) NULL, refed, ( char * ) NULL,
		    newString, PREPEND_A_COMMA, equalsString );
	if ( status != E_DB_OK )	break;

	/* WHERE */

	addString( &evolvingString, plainerWhere,
		   STlength( plainerWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* compare refing and refed columns */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, refed, ( char * ) NULL,
		    oldString, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* ; END */

	addString( &evolvingString, endNoIfProcedure,
		   STlength( endNoIfProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfUpdateSetNullRefedProcedure - cook up the text for creating 
**	a proc
**
** Description:
**	This routine cooks up the text of a procedure that will enforce
**	a referential constraint with the "on update set null" option.  
**	This procedure will be fired at the end of statements that update
**	the referenced columns of the referenced table.
**
**	This is what the text looks like:
**
**		CREATE PROCEDURE constraintName3
**		( "$A" SET OF 
**		    ( new1 1stRefedColType, ... newN lastRefedColType,
**		      old1 1stRefedColType, ... oldN lastRefedColType )
**		) AS
**		BEGIN
**
**		    UPDATE referringTable "$REFING" from "$A" "$REFED"
**			SET "$REFING".1stRefingCol = NULL, ...,
**		   	    "$REFING".lastRefingCol = NULL
**			WHERE "$REFING".1stRefingCol = "$REFED".old1 ...
**			    "$REFING".lastRefingCol = "$REFED".oldn;
**
**		END
**
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	25-may-98 (inkdo01)
**	    Written to support SET NULL.
[@history_line@]...
*/

/* CREATE PROCEDURE constraintName3 ( "$A" SET OF ( */
					/* refed column list with types,
					** new and old */
/* static char	asBeginUpd[ ] = " ) ) AS BEGIN UPDATE "; */
					/* referringTable */
/* static char	refingFromSet[ ] = " \"$REFING\" FROM \"$A\" \"$REFED\"  SET "; */
					/* assign new refed cols to null */
/* static char	plainerWhere[ ] = " WHERE "; */
					/* compare refing & old refed columns */

/* END */


static DB_STATUS
textOfUpdateSetNullRefedProcedure(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    char		*nameOfUpdateRefedProcedure =
	  ( char * ) dsh->dsh_row[ details->qci_nameOfUpdateRefedProc ];
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*constraintSchemaName =
			( char * ) &details->qci_integritySchemaName;
    i4			pass;
    EVOLVING_STRING	evolvingString;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /* create the procedure name */

    makeObjectName( internalConstraintName, UPDATE_REFED_PROCEDURE_ID,
			nameOfUpdateRefedProcedure );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString( &evolvingString, createProcedure,
		   STlength( createProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.procedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString, constraintSchemaName, DB_SCHEMA_MAXNAME,
		    NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfUpdateRefedProcedure,
	       DB_DBP_MAXNAME, LEAD_PERIOD | APPEND_OPEN_PARENTHESIS,
	       NO_QUOTES );

	/* "$A" */

	addString( &evolvingString, setName, STlength( setName ),
		   NO_PUNCTUATION, DOUBLE_QUOTE );

	/* SET OF ( */

	addString( &evolvingString, setOf,
		   STlength( setOf ), NO_PUNCTUATION, NO_QUOTES );

	/* refed column list with types, new and old */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS, newString, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	addString( &evolvingString, commaString,
		   STlength( commaString ), NO_PUNCTUATION, NO_QUOTES );

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    DATATYPES, DB_MAX_COLS, oldString, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) ) as  BEGIN UPDATE " */

	addString( &evolvingString, asBeginUpd,
		   STlength( asBeginUpd ), NO_PUNCTUATION, NO_QUOTES );

	/* "referringSchemaName"."referringTableName" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* "$REFING" FROM "$A" "$REFED" SET  */

	addString( &evolvingString, refingFromSet,
		   STlength( refingFromSet ), NO_PUNCTUATION, NO_QUOTES );

	/* assign new refing column list to NULL */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    SET_NULL, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;


	/* WHERE */

	addString( &evolvingString, plainerWhere,
		   STlength( plainerWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* compare refing and deleted columns */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray,
		    &evolvingString, refing, refed, ( char * ) NULL,
		    oldString, PREPEND_AN_AND, equalsString );
	if ( status != E_DB_OK )	break;

	/* ; END */

	addString( &evolvingString, endNoIfProcedure,
		   STlength( endNoIfProcedure ), NO_PUNCTUATION, NO_QUOTES );

	/* now concatenate all the string fragments */

	endString( &evolvingString, text );
    }

    return( status );
}

/*{
** Name: textOfInsertRefingRule - cook up the text for creating a rule
**
** Description:
**	This routine cooks up the text of a rule that will enforce
**	a referential constraint.  This rule will fire at the
**	end of statements that insert into the referring table.
**
**	This is what the text looks like:
**
**	CREATE RULE constraintName1 AFTER INSERT INTO referringTableName
**		WHERE new.1stRefingCol IS NOT NULL ...
**	  	AND new.lastRefingCol IS NOT NULL
**	WITH RULE_SCOPE=STATEMENT
**	EXECUTE PROCEDURE modifyRefingProcedureName
**	( 1stRefingCol = new.1stRefingCol, ...
**	  lastRefingCol = new.lastRefingCol )
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	15-apr-93 (rickh)
**	    Construct rule name directly into DSH.
**	18-jul-96 (inkdo01)
**	    Replaced obsolete "with rule_scope = statement" generated syntax
**	    by "for each statement" in accordance with externalization of 
**	    statement level rules.
[@history_line@]...
*/

/* CREATE RULE constraintName1 */
static char	afterInsert[ ] = " AFTER INSERT INTO ";
	/* referringTableName WHERE new columns are not null */
static char	withRule_Scope[ ] =
" FOR EACH STATEMENT EXECUTE PROCEDURE ";
	/* modifyRefingProcedureName ( assign new.columns to parameters */
static char	nakedParenthesis[ ] = " ) ";

static char	newPeriod[ ] = "new.";

static char     oldPeriod[ ] = "old.";

static DB_STATUS
textOfInsertRefingRule(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    char		*internalConstraintName =
	    ( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    DB_NAME		*nameOfInsertRefingRule =
	  ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfInsertRefingRule ];
    DB_NAME		*nameOfInsertRefingProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfInsertRefingProc ];
    i4			pass;
    EVOLVING_STRING	evolvingString;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /* create the rule name */

    makeObjectName( internalConstraintName, INSERT_REFING_RULE_ID,
	(char *) nameOfInsertRefingRule );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE RULE */

	addString( &evolvingString, createRule,
		   STlength( createRule ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.ruleName */ 

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, (char *) nameOfInsertRefingRule,
		DB_RULE_MAXNAME, LEAD_PERIOD, NO_QUOTES );

	/* AFTER INSERT INTO */

	addString( &evolvingString, afterInsert,
		   STlength( afterInsert ), NO_PUNCTUATION, NO_QUOTES );

	/* "schemaName"."tablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* WHERE */

	addString( &evolvingString, justWhere,
		   STlength( justWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* new.referringColumns are not null */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    IS_NOT_NULL, DB_MAX_COLS , ( char * ) NULL, newPeriod,
		    PREPEND_AN_AND );
	if ( status != E_DB_OK )	break;

	/* WITH RULE_SCOPE=STATEMENT EXECUTE PROCEDURE */

	addString( &evolvingString, withRule_Scope,
		   STlength( withRule_Scope ), NO_PUNCTUATION, NO_QUOTES );

	/* modifyRefingProcedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString,
		   ( char * ) nameOfInsertRefingProcedure,
		   DB_DBP_MAXNAME, APPEND_OPEN_PARENTHESIS, NO_QUOTES );

	/* assign new.columns to parameters */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    &evolvingString, ( char * ) NULL, newString,
		    ( char * ) NULL,
		    ( char * ) NULL, LEAD_COMMA, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) */

	addString( &evolvingString, nakedParenthesis,
		   STlength( nakedParenthesis ), NO_PUNCTUATION, NO_QUOTES );

	/* that's all, folks */

	endString( &evolvingString, text );

    }	/* end of the 2 pass text compilation */

    return( status );
}

/*{
** Name: textOfUpdateRefingRule - cook up the text for creating a rule
**
** Description:
**	This routine cooks up the text of a rule that will enforce
**	a referential constraint.  This rule will fire at the
**	end of statements that update the referring table.
**
**	This is what the text looks like:
**
**	CREATE RULE constraintName2
**	AFTER UPDATE( 1stRefingCol, ..., lastRefingCol )
**	OF referringTableName
**		WHERE new.1stRefingCol IS NOT NULL ...
**	  	AND new.lastRefingCol IS NOT NULL
**		AND ( new.1stRefingCol != old.1stRefingCol ...
**		      OR new.lastRefingCol != old.lastRefingCol
**		    )
**	WITH RULE_SCOPE=STATEMENT
**	EXECUTE PROCEDURE modifyRefingProcedureName
**	( 1stRefingCol = new.1stRefingCol, ...
**	  lastRefingCol = new.lastRefingCol )
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	15-apr-93 (rickh)
**	    Construct rule name directly into DSH.
**	18-jun-93 (andre)
**	    need to modify text of the rule to adequately handle cases when
**	    value of a column changes from NULL to non-NULL.  (recall that a!=b
**	    will always fail if either a or b is NULL)
**	18-jul-96 (inkdo01)
**	    Replaced obsolete "with rule_scope = statement" generated syntax
**	    by "for each statement" in accordance with externalization of 
**	    statement level rules.
[@history_line@]...
*/

/* CREATE RULE constraintName2 AFTER UPDATE ( refingColumnList */
static char	parenthesisOf[ ] = " ) OF ";
/* referringTableName WHERE new.refingColumnList IS NOT NULL */
static char	andParenthesis[ ] = " AND ( ";
/* new.1stRefingCol != old.1stRefingCol ...
   OR new.lastRefingCol != old.lastRefingCol */
static char orByItself[] = " OR ";
/* old.1stRefingCol is NULL ...
   OR old.lastRefingCol is NULL */
static char	parenthesisWithRule[ ] =
" ) FOR EACH STATEMENT EXECUTE PROCEDURE ";
/* modifyRefingProcedureName ( assign parameters to new.columnList ) */

static DB_STATUS
textOfUpdateRefingRule(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*nameOfUpdateRefingRule =
	  ( char * ) dsh->dsh_row[ details->qci_nameOfUpdateRefingRule ];
    DB_NAME		*nameOfUpdateRefingProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfUpdateRefingProc ];
    i4			pass;
    EVOLVING_STRING	evolvingString;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /* create the rule name */

    makeObjectName( internalConstraintName, UPDATE_REFING_RULE_ID,
			nameOfUpdateRefingRule );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE RULE */

	addString( &evolvingString, createRule,
		   STlength( createRule ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.ruleName */ 

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfUpdateRefingRule,
		DB_RULE_MAXNAME, LEAD_PERIOD, NO_QUOTES );

	/* AFTER UPDATE( */

	addString( &evolvingString, afterUpdate,
		   STlength( afterUpdate ), NO_PUNCTUATION, NO_QUOTES );

	/* refingColumnList */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) OF */

	addString( &evolvingString, parenthesisOf,
		   STlength( parenthesisOf ), NO_PUNCTUATION, NO_QUOTES );

	/* "schemaName"."tablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_SCHEMA_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* WHERE */

	addString( &evolvingString, justWhere,
		   STlength( justWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* new.referringColumns are not null */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    IS_NOT_NULL, DB_MAX_COLS , ( char * ) NULL, newPeriod,
		    PREPEND_AN_AND );
	if ( status != E_DB_OK )	break;

	/* AND ( */

	addString( &evolvingString, andParenthesis,
		   STlength( andParenthesis ), NO_PUNCTUATION, NO_QUOTES );

	/*
	** new.1stRefingCol != old.1stRefingCol ... OR
	** new.lastRefingCol != old.lastRefingCol
	*/

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    &evolvingString, newString, oldString,
		    ( char * ) NULL,
		    ( char * ) NULL, PREPEND_AN_OR, notEqualsString );
	if ( status != E_DB_OK )	break;

	/* OR */

	addString( &evolvingString, orByItself,
	    STlength(orByItself), NO_PUNCTUATION, NO_QUOTES );

	/*
	** old.1stRefingCol is NULL
	** ...
	** OR old.lastRefingCol is NULL
	*/

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    IS_NULL, DB_MAX_COLS, ( char * ) NULL, oldPeriod,
		    PREPEND_AN_OR);
	if ( status != E_DB_OK )        break;

	/* ) WITH RULE_SCOPE=STATEMENT EXECUTE PROCEDURE */

	addString( &evolvingString, parenthesisWithRule,
		   STlength( parenthesisWithRule), NO_PUNCTUATION, NO_QUOTES );

	/* modifyRefingProcedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString,
		( char * ) nameOfUpdateRefingProcedure,
		DB_DBP_MAXNAME, APPEND_OPEN_PARENTHESIS, NO_QUOTES );

	/* assign new.columns to parameters */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, 
		    &evolvingString, ( char * ) NULL, newString,
		    ( char * ) NULL,
		    ( char * ) NULL, LEAD_COMMA, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) */

	addString( &evolvingString, nakedParenthesis,
		   STlength( nakedParenthesis ), NO_PUNCTUATION, NO_QUOTES );

	/* that's all, folks */

	endString( &evolvingString, text );

    }	/* end of the 2 pass text compilation */

    return( status );
}

/*{
** Name: textOfDeleteRefedRule - cook up the text for creating a rule
**
** Description:
**	This routine cooks up the text of a rule that will enforce
**	a referential constraint.  This rule will fire at the
**	end of statements that delete from the referenced table.
**
**	This is what the text looks like:
**
**	CREATE RULE constraintName3 AFTER DELETE FROM referredTableName
**		WHERE old.1stRefedCol IS NOT NULL ...
**	  	AND old.lastRefedCol IS NOT NULL
**	WITH RULE_SCOPE=STATEMENT
**	EXECUTE PROCEDURE deleteRefedProcedureName
**	( 1stRefedCol = old.1stRefedCol, ...
**	  lastRefedCol = old.lastRefedCol )
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	31-mar-93 (rickh)
**	    Use referred table not constrainted table.
**	15-apr-93 (rickh)
**	    Construct rule name directly into DSH.
[@history_line@]...
*/

/* CREATE RULE constraintName3 */
static char	afterDelete[ ] = " AFTER DELETE FROM ";
/* referredTableName WHERE old.refedColList IS NOT NULL */
/* WITH RULE_SCOPE=STATEMENT EXECUTE PROCEDURE deleteRefedProcedureName ( */
/* assign parameters to old.refedColumns ) */

static DB_STATUS
textOfDeleteRefedRule(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_ref_tabname;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    char		*schemaName = ( char * ) &details->qci_ref_ownname;
    char		*nameOfDeleteRefedRule =
	  ( char * ) dsh->dsh_row[ details->qci_nameOfDeleteRefedRule ];
    DB_NAME		*nameOfDeleteRefedProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfDeleteRefedProc ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*referredSchemaName =
			( char * ) &details->qci_ref_ownname;
    i4			pass;
    EVOLVING_STRING	evolvingString;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /* create the rule name */

    makeObjectName( internalConstraintName, DELETE_REFED_RULE_ID,
			nameOfDeleteRefedRule );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE RULE */

	addString( &evolvingString, createRule,
		   STlength( createRule ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.ruleName */ 

	ADD_UNNORMALIZED_NAME( &evolvingString, referredSchemaName, DB_SCHEMA_MAXNAME,
		    NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfDeleteRefedRule,
		DB_RULE_MAXNAME, LEAD_PERIOD, NO_QUOTES );

	/* AFTER DELETE FROM */

	addString( &evolvingString, afterDelete,
		   STlength( afterDelete ), NO_PUNCTUATION, NO_QUOTES );

	/* "schemaName"."tablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* WHERE */

	addString( &evolvingString, justWhere,
		   STlength( justWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* old.refedColumns are not null */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    IS_NOT_NULL, DB_MAX_COLS , ( char * ) NULL, oldPeriod,
		    PREPEND_AN_AND );
	if ( status != E_DB_OK )	break;

	/* WITH RULE_SCOPE=STATEMENT EXECUTE PROCEDURE */

	addString( &evolvingString, withRule_Scope,
		   STlength( withRule_Scope ), NO_PUNCTUATION, NO_QUOTES );

	/* deleteRefedProcedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString,
		( char * ) nameOfDeleteRefedProcedure,
		DB_DBP_MAXNAME, APPEND_OPEN_PARENTHESIS, NO_QUOTES );

	/* assign old.refedColumns to parameters */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    &evolvingString, ( char * ) NULL, oldString,
		    ( char * ) NULL,
		    ( char * ) NULL, LEAD_COMMA, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) */

	addString( &evolvingString, nakedParenthesis,
		   STlength( nakedParenthesis ), NO_PUNCTUATION, NO_QUOTES );

	/* that's all, folks */

	endString( &evolvingString, text );

    }	/* end of the 2 pass text compilation */

    return( status );
}

/*{
** Name: textOfUpdateRefedRule - cook up the text for creating a rule
**
** Description:
**	This routine cooks up the text of a rule that will enforce
**	a referential constraint.  This rule will fire at the
**	end of statements that update the referenced table.
**
**	This is what the text looks like:
**
**	CREATE RULE constraintName4
**	AFTER UPDATE( 1stRefedCol, ..., lastRefedCol )
**	OF referredTableName
**		WHERE old.1stRefedCol IS NOT NULL ...
**	  	AND old.lastRefedCol IS NOT NULL
**		AND (
**			new.1stRefedCol != old.1stRefedCol ...
**			OR new.lastRefedCol != old.lastRefedCol
**		    )
**	WITH RULE_SCOPE=STATEMENT
**	EXECUTE PROCEDURE updateRefedProcedureName
**	( new1 = new.1stRefedCol, ...
**	  newN = new.lastRefedCol,
**	  old1 = old.1stRefedCol,...
**	  oldN = old.lastRefedCol )
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	31-mar-93 (rickh)
**	    Use referred table not constrainted table.
**	15-apr-93 (rickh
**	    Construct rule name directly into DSH.
[@history_line@]...
*/

/* CREATE RULE constraintName4 AFTER UPDATE ( refedColumnList ) OF */
/* referredTableName WHERE old.refedColumns are not null */
/* AND ( new.refedColumns != old.refedColumns */
/* ) WITH RULE_SCOPE=STATEMENT EXECUTE PROCEDURE updateRefedProcedureName ( */
/* assign new.refedColumns and old.refedColumns to parameters ) */

static DB_STATUS
textOfUpdateRefedRule(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    char		*tableName = ( char * ) &details->qci_ref_tabname;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    char		*schemaName = ( char * ) &details->qci_ref_ownname;
    char		*nameOfUpdateRefedRule =
	  ( char * ) dsh->dsh_row[ details->qci_nameOfUpdateRefedRule ];
    DB_NAME		*nameOfUpdateRefedProcedure =
	    ( DB_NAME * ) dsh->dsh_row[ details->qci_nameOfUpdateRefedProc ];
    DMT_ATT_ENTRY	**referredTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_referredTableAttributeArray ];
    char		*referredSchemaName =
			( char * ) &details->qci_ref_ownname;
    i4			pass;
    EVOLVING_STRING	evolvingString;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    /* create the rule name */

    makeObjectName( internalConstraintName, UPDATE_REFED_RULE_ID,
			nameOfUpdateRefedRule );

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE RULE */

	addString( &evolvingString, createRule,
		   STlength( createRule ), NO_PUNCTUATION, NO_QUOTES );

	/* schemaName.ruleName */ 

	ADD_UNNORMALIZED_NAME( &evolvingString, referredSchemaName, DB_SCHEMA_MAXNAME,
		    NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, nameOfUpdateRefedRule,
		DB_RULE_MAXNAME, LEAD_PERIOD, NO_QUOTES );

	/* AFTER UPDATE( */

	addString( &evolvingString, afterUpdate,
		   STlength( afterUpdate ), NO_PUNCTUATION, NO_QUOTES );

	/* refedColumnList */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	if ( status != E_DB_OK )	break;

	/* ) OF */

	addString( &evolvingString, parenthesisOf,
		   STlength( parenthesisOf ), NO_PUNCTUATION, NO_QUOTES );

	/* "schemaName"."tablename" */

	ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		LEAD_PERIOD, NO_QUOTES );

	/* WHERE */

	addString( &evolvingString, justWhere,
		   STlength( justWhere ), NO_PUNCTUATION, NO_QUOTES );

	/* old.refedColumns are not null */

	status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, &evolvingString, ulm,
		    IS_NOT_NULL, DB_MAX_COLS , ( char * ) NULL, oldPeriod,
		    PREPEND_AN_AND );
	if ( status != E_DB_OK )	break;

	/* AND ( */

	addString( &evolvingString, andParenthesis,
		   STlength( andParenthesis ), NO_PUNCTUATION, NO_QUOTES );

	/*
	** new.1stRefedCol != old.1stRefedCol ... OR
	** new.lastRefedCol != old.lastRefedCol
	*/

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    &evolvingString, newString, oldString,
		    ( char * ) NULL,
		    ( char * ) NULL, PREPEND_AN_OR, notEqualsString );
	if ( status != E_DB_OK )	break;

	/* ) WITH RULE_SCOPE=STATEMENT EXECUTE PROCEDURE */

	addString( &evolvingString, parenthesisWithRule,
		   STlength( parenthesisWithRule), NO_PUNCTUATION, NO_QUOTES );

	/* updateRefedProcedureName ( */

	ADD_UNNORMALIZED_NAME( &evolvingString,
		( char * ) nameOfUpdateRefedProcedure,
		DB_DBP_MAXNAME, APPEND_OPEN_PARENTHESIS, NO_QUOTES );

	/* assign new.refedColumns and old.refedColumns to parameters */

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    &evolvingString, ( char * ) NULL, newString,
		    newString,
		    ( char * ) NULL, LEAD_COMMA, equalsString );
	if ( status != E_DB_OK )	break;

	addString( &evolvingString, commaString,
		   STlength( commaString ), NO_PUNCTUATION, NO_QUOTES );

	status = compareColumnLists( dsh, ulm,
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    ( PST_COL_ID * ) details->qci_ref_collist,
		    referredTableAttributeArray, 
		    &evolvingString, ( char * ) NULL, oldString,
		    oldString,
		    ( char * ) NULL, LEAD_COMMA, equalsString );
	if ( status != E_DB_OK )	break;

	/* ) */

	addString( &evolvingString, nakedParenthesis,
		   STlength( nakedParenthesis ), NO_PUNCTUATION, NO_QUOTES );

	/* that's all, folks */

	endString( &evolvingString, text );

    }	/* end of the 2 pass text compilation */

    return( status );
}

/*{
** Name: textOfUniqueIndex - cook up the text for creating an index
**
** Description:
**	This routine cooks up the text for creating a unique index on a
**	primary key.  That is, an index on the UNIQUE columns of a
**	UNIQUE constraint.
**
**	This is what the text looks like:
**
**		CREATE UNIQUE INDEX constraintName ON tableName
**		( listOfUniqueColumns )
**		WITH PERSISTENCE, UNIQUE_SCOPE = STATEMENT,
**		STRUCTURE = BTREE
**
**	If TABLE-STRUCT is requested, we instead generate a modify:
**		MODIFY tableName TO BTREE UNIQUE
**		UNIQUE_SCOPE = STATEMENT
**		( listOfUniqueColumns )
**		WITH [ COMPRESSION = ([HI]DATA) ] | [ FILLFACTOR = 100 ]
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	14-jun-93 (rickh)
**	    Construct index name from user supplied constraint name if
**	    possible.
**	1-apr-98 (inkdo01)
**	    Add constraint index options to syntax build.
**	3-may-2007 (dougi)
**	    Add support for optional modify of base table to btree on
**	    constrained columns (in lieu of secondary index).
**	28-Jul-2010 (kschendel) SIR 124104
**	    Add code to autostruct to preserve compression in case the table
**	    was created with compression.  Also, tweak autostruct so that
**	    it asks for 100% fillfactor ... lower fillfactors are pointless
**	    for uncompressed btrees (due to the associated data page business).
*/

static char	createUniqueIndex[ ] = "CREATE UNIQUE INDEX ";
/* constraintName ON tableName ( uniqueColumnList */
static char	withPersistence[ ] =
" ) WITH PERSISTENCE, UNIQUE_SCOPE=STATEMENT, STRUCTURE=";
static	char	modifyTable2[ ] = "MODIFY ";
static	char	toBTREEUnique[ ] = " TO BTREE UNIQUE UNIQUE_SCOPE = STATEMENT ON ";

static DB_STATUS
textOfUniqueIndex(
    QEF_AHD		*qea_act,
    QEE_DSH		*dsh,
    DB_TEXT_STRING	**text )
{
    DB_STATUS		status = E_DB_OK;
    QEF_CREATE_INTEGRITY_STATEMENT	*details = &qea_act->qhd_obj.
						qhd_createIntegrity;
    DB_INTEGRITY	*integrityTuple = details->qci_integrityTuple;
    char		*internalConstraintName =
		( char * ) dsh->dsh_row[ details->qci_internalConstraintName ];
    DB_TAB_NAME		*indexName =
	    ( DB_TAB_NAME * ) dsh->dsh_row[ details->qci_nameOfIndex ];
    ULM_RCB		*ulm =
	    ( ULM_RCB * ) dsh->dsh_row[ details->qci_ulm_rcb ];
    EVOLVING_STRING	evolvingString;
    DMT_ATT_ENTRY	**constrainedTableAttributeArray =
    ( DMT_ATT_ENTRY ** ) dsh->dsh_row[ details->qci_cnstrnedTabAttributeArray ];
    char		*tableName = ( char * ) &details->qci_cons_tabname;
    char		*schemaName = ( char * ) &details->qci_cons_ownname;
    i4			pass;
    i4			unpaddedLength;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;
    char		*actualIndexName = ( char * ) NULL;
    bool		indexNameUsed;
    char		preferredIndexName[ DB_TAB_MAXNAME + 1 ];

    /*
    ** $constraintName
    **
    ** sigh.  Actually, it's not that easy.  IF the user specifed a
    ** constraint name AND there is no index named $userSuppliedName,
    ** THEN we can use that name.  Otherwise, we must
    ** use the internal constraint name constructed from the constraint
    ** ID. Before that, check if index name was explicitly coded, in which
    ** case, it is used.
    */

    if (details->qci_flags & QCI_NEW_NAMED_INDEX)
	actualIndexName = &details->qci_idxname.db_tab_name[0];
    else
    {
	actualIndexName = internalConstraintName;
	if ( details->qci_flags & QCI_CONS_NAME_SPEC )
	{
            status = doesTableWithConsNameExist( dsh,
		integrityTuple->dbi_consname.db_constraint_name,
		&details->qci_cons_ownname,
		preferredIndexName, &indexNameUsed );
            if ( status != E_DB_OK )	return( status );
            if ( indexNameUsed == FALSE )
		actualIndexName = preferredIndexName;
	}
    }

    /*
    ** inside this loop, ulm should only be used for the text string storage.
    ** otherwise, you will break the compiler.
    */

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	if (details->qci_flags & QCI_TABLE_STRUCT)
	{
	    /* MODIFY TABLE "schemaName"."tableName" */
                                         
	    addString( &evolvingString, modifyTable2,
		   STlength( modifyTable2 ), NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		    LEAD_PERIOD, NO_QUOTES );

	    /* TO BTREE UNIQUE UNIQUE_SCOPE = STATEMENT ON column list */

	    addString( &evolvingString, toBTREEUnique,
		   STlength( toBTREEUnique ), NO_PUNCTUATION, NO_QUOTES );

	    status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	    if ( status != E_DB_OK )	break;

	    /* WITH COMPRESSION = (data) (optional).  If no compression,
	    ** add WITH FILLFACTOR = 100 which is best for uncompressed btree.
	    */
	    if (details->qci_autocompress == DMU_COMP_ON)
		addString(&evolvingString, withDataComp, STlength(withDataComp),
			NO_PUNCTUATION, NO_QUOTES);
	    else if (details->qci_autocompress == DMU_COMP_HI)
		addString(&evolvingString, withHidataComp, STlength(withHidataComp),
			NO_PUNCTUATION, NO_QUOTES);
	    else
		addString(&evolvingString, withFF100, STlength(withFF100),
			NO_PUNCTUATION, NO_QUOTES);

	    /* now concatenate all the string fragments */

	    endString( &evolvingString, text );
	}
	else
	{
	    /* CREATE UNIQUE INDEX */
                                         
	    addString( &evolvingString, createUniqueIndex,
		   STlength( createUniqueIndex ), NO_PUNCTUATION, NO_QUOTES );

	    /* "schemaName"."indexName" */

	    ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

 	    ADD_UNNORMALIZED_NAME( &evolvingString, actualIndexName,
		DB_TAB_MAXNAME, LEAD_PERIOD, NO_QUOTES );

	    /* ON "schemaName"."tableName" ( */

	    addString( &evolvingString, on,
		   STlength( on ), NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, schemaName, DB_SCHEMA_MAXNAME,
		NO_PUNCTUATION, NO_QUOTES );

	    ADD_UNNORMALIZED_NAME( &evolvingString, tableName, DB_TAB_MAXNAME,
		    LEAD_PERIOD | APPEND_OPEN_PARENTHESIS, NO_QUOTES );

	    /* uniqueColumnList */

	    status = stringInColumnList( dsh,
		    ( PST_COL_ID * ) details->qci_cons_collist,
		    constrainedTableAttributeArray, &evolvingString, ulm,
		    NO_POSTFIX, DB_MAX_COLS , ( char * ) NULL, ( char * ) NULL,
		    LEAD_COMMA );
	    if ( status != E_DB_OK )	break;

	    /* ) WITH PERSISTENCE, UNIQUE_SCOPE=STATEMENT, STRUCTURE=????? */

	    addString( &evolvingString, withPersistence,
		   STlength( withPersistence ), NO_PUNCTUATION, NO_QUOTES );

	    /* add structure and possible index with options */

	    addWithopts( &evolvingString, details);

	    /* now concatenate all the string fragments */

	    endString( &evolvingString, text );
	}
    }

    /* store the index name in the DSH so we can link it to the constraint
    ** later */

    if (details->qci_flags & QCI_TABLE_STRUCT)
	MEcopy((char *)tableName, DB_TAB_MAXNAME, indexName);
    else
    {
	unpaddedLength = STnlength( DB_TAB_MAXNAME, actualIndexName );
	MEmove( ( u_i2 ) unpaddedLength, ( PTR ) actualIndexName, ' ',
	    ( u_i2 ) DB_TAB_MAXNAME, ( PTR ) indexName );
    }

    return( status );
}

/*{
** Name: addWithopts - finish building refing/unique index text.
**
** Description:
**	This routine adds the structure type to the evolving "create index" 
**	string, then any other explicitly specified index "with" options.
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	1-apr-98 (inkdo01)
**	    Written as part of support for constraint index with options.
**      8-Oct-2010 (hanal04) Bug 124561
**          Add handling of compression settings.
**	18-Oct-2010 (kschendel) SIR 124544
**	    Replace individual qci_idx_xxx things with DMU characteristics.
*/

static char	withLocation[ ] = ", LOCATION=(";

static VOID
addWithopts(
    EVOLVING_STRING	*evolvingString,
    QEF_CREATE_INTEGRITY_STATEMENT	*details)
{
    char	*ixstruct = "BTREE";
    char	withstring[80];		/* space for assembling options */
    char	*withopt = &withstring[0];


    /* Add the structure type, if none it's btree */
    if (BTtest(DMU_STRUCTURE, details->qci_dmu_chars.dmu_indicators))
     switch (details->qci_dmu_chars.dmu_struct) {
      case DB_ISAM_STORE:
	ixstruct = "ISAM ";
	break;
      case DB_HASH_STORE:
	ixstruct = "HASH ";
	break;
     }

    addString( evolvingString, ixstruct, 5, NO_PUNCTUATION, NO_QUOTES);

    /* Now check for the various index with clause options */

    if (BTtest(DMU_KCOMPRESSION, details->qci_dmu_chars.dmu_indicators)
      || BTtest(DMU_DCOMPRESSION, details->qci_dmu_chars.dmu_indicators))
    {
	char compression[20];	/* Longest is "nokey,nodata" */

	compression[0] = '\0';
	if (BTtest(DMU_KCOMPRESSION, details->qci_dmu_chars.dmu_indicators))
	{
	    if (details->qci_dmu_chars.dmu_kcompress == DMU_COMP_ON)
		STcat(compression, "KEY");
	    else
		STcat(compression, "NOKEY");
	}
	if (BTtest(DMU_DCOMPRESSION, details->qci_dmu_chars.dmu_indicators))
	{
	    if (compression[0] != '\0')
		STcat(compression, ",");
	    if (details->qci_dmu_chars.dmu_dcompress == DMU_COMP_OFF)
		STcat(compression, "NODATA");
	    else if (details->qci_dmu_chars.dmu_dcompress == DMU_COMP_ON)
		STcat(compression, "DATA");
	    else
		STcat(compression, "HIDATA");	/* Should be impossible */
	}
        sprintf(withopt, ", COMPRESSION=(%s)", compression);
        addString( evolvingString, withopt, STlength(withopt),
            NO_PUNCTUATION, NO_QUOTES);
    }

    if (BTtest(DMU_DATAFILL,details->qci_dmu_chars.dmu_indicators))
    {
	sprintf(withopt, ", FILLFACTOR=%d", details->qci_dmu_chars.dmu_fillfac);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }
    if (BTtest(DMU_LEAFFILL,details->qci_dmu_chars.dmu_indicators))
    {
	sprintf( withopt, ", LEAFFILL=%d", details->qci_dmu_chars.dmu_leaff);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }
    if (BTtest(DMU_IFILL,details->qci_dmu_chars.dmu_indicators))
    {
	sprintf( withopt, ", NONLEAFFILL=%d", details->qci_dmu_chars.dmu_nonleaff);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }
    if (BTtest(DMU_PAGE_SIZE,details->qci_dmu_chars.dmu_indicators))
    {
	sprintf( withopt, ", PAGE_SIZE=%d", details->qci_dmu_chars.dmu_page_size);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }
    if (BTtest(DMU_MINPAGES,details->qci_dmu_chars.dmu_indicators))
    {
	sprintf( withopt, ", MINPAGES=%d", details->qci_dmu_chars.dmu_minpgs);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }
    if (BTtest(DMU_MAXPAGES,details->qci_dmu_chars.dmu_indicators))
    {
	sprintf( withopt, ", MAXPAGES=%d", details->qci_dmu_chars.dmu_maxpgs);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }
    if (BTtest(DMU_ALLOCATION,details->qci_dmu_chars.dmu_indicators))
    {
	sprintf( withopt, ", ALLOCATION=%d", details->qci_dmu_chars.dmu_alloc);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }
    if (BTtest(DMU_EXTEND, details->qci_dmu_chars.dmu_indicators))
    {
	sprintf( withopt, ", EXTEND=%d", details->qci_dmu_chars.dmu_extend);
	addString( evolvingString, withopt, STlength(withopt),
	    NO_PUNCTUATION, NO_QUOTES);
    }

    /* The last with option is location, which is a tad trickier. */
    if (details->qci_idxloc.data_in_size)
    {
	char	*locaddr = details->qci_idxloc.data_address;
	i4	puncflag = 0;
	i4	locleft = details->qci_idxloc.data_in_size;
	i4	locsize;

	addString( evolvingString, withLocation, 
	    STlength(withLocation), NO_PUNCTUATION, NO_QUOTES);
	while (locleft)
	{
	    if (locleft <= sizeof(DB_LOC_NAME)) 
		puncflag |= CLOSE_PARENTHESIS;
	    for (locsize = (sizeof(DB_LOC_NAME) < locleft) ?
		sizeof(DB_LOC_NAME) : locleft; locsize; locsize--)
	     if (locaddr[locsize-1] != ' ') break;
				/* wee loop to strip trailing blanks */
	    addString( evolvingString, locaddr, locsize,
			puncflag, NO_QUOTES);
				/* NOTE: use of addString, because 
				** location names can't be delimited */
	    puncflag |= LEAD_COMMA;
	    locleft -= sizeof(DB_LOC_NAME);
	    locaddr = &locaddr[sizeof(DB_LOC_NAME)];
	}
    }

}

/*{
** Name: populateIIKEY - stuff IIKEY with tuples describing key columns
**
** Description:
**	This routine cooks up IIKEY tuples describing the PRIMARY or
**	FOREIGN keys passed by its caller.  Then this routine writes those
**	tuples into the IIKEY catalog.
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	20-jan-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	30-jun-93 (rickh0
**	    Removed a spurious reference to variable "i" in the first
**	    for loop.  Lint choked on it.
**	28-dec-93 (rickh)
**	    Fixed populateIIKEY to properly record and return errors.
**	2-Oct-1998 (rigka01) bug #93612
**	    set next pointer in last QEF_DATAarray not in non-element after
**	    next;  bug introduced ULM memory corruption when running
**	    alter table ... add constraint ... FOREIGN KEY ... REFERENCES
**	    which surfaced as GPF here or later in ULM code (e.g.
**	    ulm_closestream, ulm_free) 
[@history_line@]...
*/

static DB_STATUS
populateIIKEY(
    QEE_DSH		*dsh,
    ULM_RCB		*ulm,
    DB_TAB_ID		*constraintID,
    PST_COL_ID		*keyList )
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		closeStatus = E_DB_OK;
    i4		error;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    i4			i;
    PST_COL_ID		*columnID;
    i4			numberOfKeys;
    QEU_CB		qeucb, *qeu = &qeucb;
    QEF_DATA		*QEF_DATAarray;
    DB_IIKEY		*tuples;

    /* count the keys */

    for ( columnID = keyList, numberOfKeys = 0;
	  columnID != ( PST_COL_ID * ) NULL;
          columnID = columnID->next )
    {
	numberOfKeys++;
    }

    /* allocate space for IIKEY tuples and QEF_DATAs */

    ulm->ulm_psize = sizeof( DB_IIKEY ) * numberOfKeys;
    if ((status = qec_malloc(ulm)) != E_DB_OK)
    {
        error = ulm->ulm_error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
        return (status);
    }
    tuples = ( DB_IIKEY * ) ulm->ulm_pptr;

    ulm->ulm_psize = sizeof( QEF_DATA ) * numberOfKeys;
    if ((status = qec_malloc(ulm)) != E_DB_OK)
    {
        error = ulm->ulm_error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
        return (status);
    }
    QEF_DATAarray = ( QEF_DATA * ) ulm->ulm_pptr;

    /* point the QEF_DATAs at the tuples */

    for ( i = 0; i < numberOfKeys; i++ )
    {
	QEF_DATAarray[ i ].dt_next = &QEF_DATAarray[ i + 1 ];
	QEF_DATAarray[ i ].dt_size = sizeof( DB_IIKEY );
	QEF_DATAarray[ i ].dt_data = (PTR) &tuples[ i ];
    }
    QEF_DATAarray[ numberOfKeys-1 ].dt_next = 0;

    /* fill up the tuples */

    for ( columnID = keyList, i = 0;
	  i < numberOfKeys;
	  columnID = columnID->next, i++ )
    {
	tuples[ i ].dbk_constraint_id.db_tab_base = constraintID->db_tab_base;
	tuples[ i ].dbk_constraint_id.db_tab_index = constraintID->db_tab_index;
	tuples[ i ].dbk_column_id = columnID->col_id;
	tuples[ i ].dbk_position = i + 1;	/* numbering starts at 1 */
    }

    /* write the tuples to IIKEY */

    qeu->qeu_type = QEUCB_CB;
    qeu->qeu_length = sizeof(QEUCB_CB);
    qeu->qeu_db_id = qef_cb->qef_rcb->qef_db_id;
    qeu->qeu_lk_mode = DMT_IX;
    qeu->qeu_flag = DMT_U_DIRECT;
    qeu->qeu_access_mode = DMT_A_WRITE;
    qeu->qeu_mask = 0;
    qeu->qeu_qual = 0;
    qeu->qeu_qarg = 0;
    qeu->qeu_f_qual = 0;
    qeu->qeu_f_qarg = 0;
    qeu->qeu_klen = 0;
    qeu->qeu_key = (DMR_ATTR_ENTRY **) NULL;

    qeu->qeu_tab_id.db_tab_base = DM_B_IIKEY_TAB_ID;
    qeu->qeu_tab_id.db_tab_index = DM_I_IIKEY_TAB_ID;	 
    status = qeu_open(qef_cb, qeu);
    if (status != E_DB_OK)
    {
	error = qeu->error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	return( status );
    }

    qeu->qeu_count = numberOfKeys;
    qeu->qeu_tup_length = sizeof( DB_IIKEY );
    qeu->qeu_input = QEF_DATAarray;

    status = qeu_append(qef_cb, qeu);
    if (status != E_DB_OK)
    {
	error = qeu->error.err_code;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );

	/* don't return. we have to close IIKEY first */
    }

    closeStatus = qeu_close(qef_cb, qeu);    
    if (closeStatus != E_DB_OK)
    {
	error = qeu->error.err_code;
	qef_error( error, 0L, closeStatus, &error, &dsh->dsh_error, 0 );
    }

    /* good night, Gracie */

    return( status );
}

/*{
** Name: lookupSchemaID - find a schema ID by schema name
**
** Description:
**	This routine is passed a schema name.  It looks up the schema ID
**	that corresponds to that name by key probing IISCHEMA.
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**	    none
**
** History:
**	18-feb-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
**	09-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use qef_cat_owner instead of "$ingres" in lookupSchemaID()
**	11-jun-93 (rickh)
**	    Change lock mode on IISCHEMA to shared read.  Remove hack for
**	    forcing schema id to 100:  implicit schema creation now works.
**	    Trim schema name in qef_error() call.
**	11-oct-93 (rickh)
**	    When opening IISCHEMA for scanning, ask for page rather than table
**	    locks.  The shared table level lock was locking out other schema
**	    creators.
[@history_line@]...
*/

#define	LSI_NUMBER_OF_KEYS	1	/* number of schema keys */

static DB_STATUS
lookupSchemaID(
    QEE_DSH		*dsh,
    DB_SCHEMA_NAME	*schemaName,
    DB_SCHEMA_ID	*schemaID
)
{
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    i4			i;
    DB_IISCHEMA		schemaTuple;
    bool		IISCHEMAopened = FALSE;
    QEF_DATA		qef_data;
    DMR_ATTR_ENTRY	key_array[ LSI_NUMBER_OF_KEYS ];
    DMR_ATTR_ENTRY      *key_ptr_array[ LSI_NUMBER_OF_KEYS ];
    QEU_CB		qeu_cb;

    /*
    ** if this is the $ingres schema, don't bother looking up the schema id.
    ** instead, just fill in the canonical schema id for $ingres.  why?
    ** because:  at createdb time, we will be asked to fill in the schema id
    ** for NOT NULL integrities before the iischema catalog has been created.
    ** psf will have been thoughtful enough to force the schema name to
    ** $ingres in these cases.  simple, elegant.
    */

    if ( MEcmp(	(PTR)schemaName, (PTR)qef_cb->qef_cat_owner,
		sizeof(DB_SCHEMA_NAME)) == 0)
    {
	schemaID->db_tab_base = DB_INGRES_SCHEMA_ID;
	schemaID->db_tab_index = 0;
	return( status );

    }	/* endif this is the $ingres schema */

    for (i=0; i < LSI_NUMBER_OF_KEYS; i++) { key_ptr_array[i] = &key_array[i]; }

    BEGIN_CODE_BLOCK	/* code block to break out of */

        /* open IISCHEMA */

	qeu_cb.qeu_type = QEUCB_CB;
	qeu_cb.qeu_length = sizeof( QEU_CB );

        qeu_cb.qeu_tab_id.db_tab_base  = DM_B_IISCHEMA_TAB_ID;
        qeu_cb.qeu_tab_id.db_tab_index  = DM_I_IISCHEMA_TAB_ID;
        qeu_cb.qeu_db_id = qef_cb->qef_rcb->qef_db_id;
        qeu_cb.qeu_eflag = QEF_INTERNAL;
        qeu_cb.qeu_flag = DMT_U_DIRECT;

        qeu_cb.qeu_lk_mode = DMT_IS;
        qeu_cb.qeu_access_mode = DMT_A_READ;
        qeu_cb.qeu_mask = 0;
    	
        if ((status = qeu_open(qef_cb, &qeu_cb)) != E_DB_OK)
        {   QEF_ERROR( qeu_cb.error.err_code ); EXIT_CODE_BLOCK; }

	IISCHEMAopened = TRUE;

	/* set up for reading IISCHEMA by key */

        qeu_cb.qeu_output = &qef_data;
        qef_data.dt_next = 0;
        qef_data.dt_size = sizeof( DB_IISCHEMA );
        qef_data.dt_data = (PTR) &schemaTuple; 

        qeu_cb.qeu_count = 1;
        qeu_cb.qeu_tup_length = sizeof( DB_IISCHEMA );
	qeu_cb.qeu_qual = 0;
        qeu_cb.qeu_qarg = 0;

        qeu_cb.qeu_klen =  LSI_NUMBER_OF_KEYS;       
        qeu_cb.qeu_key = key_ptr_array;

        key_ptr_array[0]->attr_number = DM_1_IISCHEMA_KEY;
        key_ptr_array[0]->attr_operator = DMR_OP_EQ;
        key_ptr_array[0]->attr_value = ( char * ) schemaName;

        qeu_cb.qeu_getnext = QEU_REPO;

	if ((status = qeu_get(qef_cb, &qeu_cb)) != E_DB_OK)
	{
	    if ( qeu_cb.error.err_code == E_QE0015_NO_MORE_ROWS )
	    {
		error = E_QE0304_SCHEMA_DOES_NOT_EXIST;
		(VOID) qef_error( E_QE0304_SCHEMA_DOES_NOT_EXIST, 0L,
				 status, &error, &dsh->dsh_error, 1,
				 qec_trimwhite( DB_SCHEMA_MAXNAME, 
					( char *)schemaName ), schemaName );
	    }
	    else
	    {
		error = qeu_cb.error.err_code;
	        qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	    }

	    EXIT_CODE_BLOCK;
	}	/* endif read from iischema fails */

	MEcopy( ( PTR ) &schemaTuple.db_schid, sizeof( DB_SCHEMA_ID ),
		( PTR ) schemaID );

    END_CODE_BLOCK

    /* close IISCHEMA */

    if ( IISCHEMAopened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &qeu_cb );    }

    /* good night, Gracie */

    return( status );
}

/*{
** Name: reportIntegrityViolation - execute iierror
**
** Description:
**	This routine cooks up the text to execute the iierror procedure
**	to report an integrity violation.  This routine is called by the
**	compilers that cook up text to be "execute immediate"ed.
**
**	The following text is cooked up:
**			EXECUTE PROCEDURE IIERROR( errono = errorNumber,
**						detail = 0,
**						p_count = 3,
**						p1 = 'constraintType',
**						p2 = 'constraintName',
**						p3 = '"tableName"' )
**
** Inputs:
**	constraintType		e.g., Check, Unique, Referential
**	constraintName
**	tableName
**
** Outputs:
**	evolvingString		where text is written to
**
** Side Effects:
**	    none
**
** History:
**	2-mar-93 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/

static	char	executeProcedure[ ] =
	" EXECUTE PROCEDURE IIERROR( errorno = ";

static	char	detailEquals[ ] = ", detail = 0, p_count = 3, p1 = ";
static	char	p2Equals[ ] = ", p2 = ";
static	char	p3Equals[ ] = ", p3 = ";

static DB_STATUS
reportIntegrityViolation(
    EVOLVING_STRING	*evolvingString,
    char		*constraintType,
    char		*constraintName,
    char		*tableName,
    QEE_DSH		*dsh
)
{
    DB_STATUS		status = E_DB_OK;
    char		errorNumber[ DECIMAL_DIGITS_IN_A_LONG ];
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    for ( ; ; )		/* something to break out of */
    {

        /* EXECUTE PROCEDURE IIERROR( errorno = */

        addString( evolvingString, executeProcedure,
		   STlength( executeProcedure), NO_PUNCTUATION, NO_QUOTES );

        /* error number */

        CVna( ( i4  ) E_US1905_INTEGRITY_VIOLATION, errorNumber );
        addString( evolvingString, errorNumber,
		   STlength( errorNumber ), NO_PUNCTUATION, NO_QUOTES );

        /* , detail = 0, p_count = 3, p1 =  */

        addString( evolvingString, detailEquals, STlength( detailEquals ),
		NO_PUNCTUATION, NO_QUOTES );

        /* 'constraintType' */

        addString( evolvingString, constraintType, STlength( constraintType ),
		   NO_PUNCTUATION, NO_QUOTES );

        /* , p2 = */

        addString( evolvingString, p2Equals, STlength( p2Equals), NO_PUNCTUATION,
		NO_QUOTES );

        /* constraintName */

        ADD_UNNORMALIZED_NAME( evolvingString, constraintName,
		DB_CONS_MAXNAME, NO_PUNCTUATION, SINGLE_QUOTE );

        /* , p3 = */

        addString( evolvingString, p3Equals, STlength( p3Equals), NO_PUNCTUATION,
		NO_QUOTES );

        /* tableName ) */

        ADD_UNNORMALIZED_NAME( evolvingString, tableName, DB_TAB_MAXNAME,
	    CLOSE_PARENTHESIS, SINGLE_QUOTE );

	break;
    }	/* end of code block */

    return( status );
}

/*{
** Name: reportRefConsViolation - report violation of referential constraint
**
** Description:
**	This routine cooks up the text to execute the iierror procedure
**	to report referential constraint violation. This routine is called 
**	by the compilers that cook up text to be "execute immediate"ed.
**	The possible errors that this routine can compile are:
**
**		E_US1906_REFING_FK_INS_VIOLATION
**		E_US1907_REFING_FK_UPD_VIOLATION
**		E_US1908_REFED_PK_DEL_VIOLATION
**		E_US1909_REFED_PK_UPD_VIOLATION
**
**	The following text is cooked up:
**			EXECUTE PROCEDURE IIERROR( errono = errorNumber,
**						detail = 0,
**						p_count = 3,
**						p1 = '"activeTableName"',
**						p2 = '"passiveTableName"',
**						p3 = 'constraintName' )
**
** Inputs:
**	qef_rcb			qef request control block
**	errorNumber		one of the above errors
**	activeTableName		table that the violation operation (e.g.,
**				DELETE, UPDATE, INSERT) was being performed on
**	passiveTableName	other table linked to the activeTable by this
**				referential constraint
**	constraintName
**
** Outputs:
**	evolvingString		where text is written to
**
** Side Effects:
**	    none
**
** History:
**	14-jan-94 (rickh)
**	    Created for FIPS CONSTRAINTS project (6.5).
[@history_line@]...
*/

/* EXECUTE PROCEDURE IIERROR( errorno = errorNumber */

/* , detail = 0, p_count = 3, p1 = activeTableName */

/* , p2 = passiveTableName */

/* , p3 = constraintName */

static DB_STATUS
reportRefConsViolation(
    EVOLVING_STRING	*evolvingString,
    QEE_DSH		*dsh,
    i4			errorNumber,
    char		*activeTableName,
    char		*passiveTableName,
    char		*constraintName
)
{
    DB_STATUS		status = E_DB_OK;
    char		errorString[ DECIMAL_DIGITS_IN_A_LONG ];
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;

    for ( ; ; )		/* something to break out of */
    {

        /* EXECUTE PROCEDURE IIERROR( errorno = */

        addString( evolvingString, executeProcedure,
		   STlength( executeProcedure), NO_PUNCTUATION, NO_QUOTES );

        /* error number */

        CVna( errorNumber, errorString );
        addString( evolvingString, errorString,
		   STlength( errorString ), NO_PUNCTUATION, NO_QUOTES );

        /* , detail = 0, p_count = 3, p1 =  */

        addString( evolvingString, detailEquals, STlength( detailEquals ),
		NO_PUNCTUATION, NO_QUOTES );

        /* activetableName */

        ADD_UNNORMALIZED_NAME( evolvingString, activeTableName, DB_TAB_MAXNAME,
	    NO_PUNCTUATION, SINGLE_QUOTE );

        /* , p2 = */

        addString( evolvingString, p2Equals, STlength( p2Equals), NO_PUNCTUATION,
		NO_QUOTES );

        /* passiveTableName */

        ADD_UNNORMALIZED_NAME( evolvingString, passiveTableName, DB_TAB_MAXNAME,
	    NO_PUNCTUATION, SINGLE_QUOTE );

        /* , p3 = */

        addString( evolvingString, p3Equals, STlength( p3Equals), NO_PUNCTUATION,
		NO_QUOTES );

        /* constraintName ) */

        ADD_UNNORMALIZED_NAME( evolvingString, constraintName,
		DB_CONS_MAXNAME, CLOSE_PARENTHESIS, SINGLE_QUOTE );

	break;
    }	/* end of code block */

    return( status );
}

/*
** Name: qea_cor_text - build text for a Rule which will enforce Check Option
**
** Description:
**	This function will construct text for a Check Option Rule.  It will use 
**	information found in the QEF_CREATE_VIEW action as well as the DSH in 
**	its task.
**
** Input:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	crt_view			create view action structure
**	tbl_info			table description
**	rule_name			name for the rule
**
** Outputs:
**	*rule_text			text for the rule
**      qef_rcb
**	    .error.err_code		filled in if an error occurs
**
** History:
**	sometime-in-march-93 (andre)
**	    written
**      08-mar-93 (anitap)
**          Removed break at the end of for lop. We were breaking out of
**          the loop after the first pass whereas the query text string is built
**          in two passes.
**	31-mar-93 (andre)
**	    optimize things a bit by not performing computations during the
**	    second pass (they will be performed during the first pass so that we
**	    can compute the total buffer size)
**
**	    qea_readAttributes returns address of an array of pointers -
**	    att_array needs to be (DMT_ATT_ENTRY **)
**	13-apr-93 (andre)
**	    will use cui_idunorm() to correctly convert names into delimited
**	    identifiers
**	20-apr-93 (andre)
**	    att_entry is (DMT_ATT_ENTRY *) and att_array is (DMT_ATT_ENTRY **),
**	    so att_entry = att_array + i is obviously incorrect
**	07-jun-93 (andre)
**	    we are introducing a new tracepoint to control the CHECK OPTION
**	    enforcement mechanism:
**		if QEF_T_CHECK_OPT_ROW_LVL_RULE is set,
**		    we will use a row level rule
**		else
**		    we will use a statement-level rule 
**	28-jan-94 (andre)
**	    QEF_T_CHECK_OPT_ROW_LVL_RULE is no more;
**	    one can set trace point QE10 (QEF_T_CHECK_OPT_STMT_LVL_RULE) to use
**	    statement level rule with dbproc NOT involving an outer join on TID
**	    or QE10 AND QE8 (QEF_T_CHECK_OPT_TIDOJ_SET_DBP) to use statement 
**	    level rule with dbproc involving an outer join on TID
**	18-jul-96 (inkdo01)
**	    Replaced obsolete "with rule_scope = statement" generated syntax
**	    by "for each statement" in accordance with externalization of 
**	    statement level rules.
*/
static DB_STATUS
qea_cor_text(
	QEE_DSH		*dsh,
	QEF_CREATE_VIEW	*crt_view,
	DMT_TBL_ENTRY	*tbl_info,
	DB_NAME		*rule_name,
	DB_TEXT_STRING	**rule_text)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    ULM_RCB		*ulm = (ULM_RCB *) dsh->dsh_row[crt_view->ahd_ulm_rcb];
    DB_TAB_NAME		*view_name = &crt_view->ahd_view_name;
    DB_OWN_NAME		*schema_name = 
	(DB_OWN_NAME *) dsh->dsh_row[crt_view->ahd_view_owner];
    DB_DBP_NAME		*dbp_name = 
	(DB_DBP_NAME *) dsh->dsh_row[crt_view->ahd_check_option_dbp_name];
    EVOLVING_STRING	evolvingString;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;
    DMT_ATT_ENTRY	**att_array;
    bool		bld_row_level_rule;
    i4		val1, val2;

    /*
    ** first determine how we will go about enforcing the CHECK OPTION
    */
    bld_row_level_rule =
	(ult_check_macro(&qef_cb->qef_trace, QEF_T_CHECK_OPT_STMT_LVL_RULE,
	    &val1, &val2) == 0);

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE RULE */

	addString(&evolvingString, "create rule ",
	    sizeof("create rule ") - 1, NO_PUNCTUATION, NO_QUOTES );

	/* add schema */
	status = addUnnormalizedName(schema_name->db_own_name, DB_OWN_MAXNAME,
	    unnormalizedName, &unnormalizedNameLength, &evolvingString,
	    NO_PUNCTUATION, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;
	
	/* add .rule_name */
	status = addUnnormalizedName(rule_name->db_name, DB_RULE_MAXNAME,
	    unnormalizedName, &unnormalizedNameLength, &evolvingString,
	    LEAD_PERIOD, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;

	/* " after " */
	addString(&evolvingString, " after ", sizeof(" after ") - 1,
		   NO_PUNCTUATION, NO_QUOTES );

	/* 
	** if CHECK OPTION needs to be enforced dynamically after inserting into
	** view, add keyword INSERT to the "evolving string"
	*/
	if (crt_view->ahd_crt_view_flags & AHD_CHECK_OPT_DYNAMIC_INSRT)
	{
	    char	*str;
	    i4		str_len;

	    if (crt_view->ahd_crt_view_flags & AHD_CHECK_OPT_DYNAMIC_UPDT)
	    {
		/* 
		** if CHECK OPTION needs to be enforced after both INSERT and 
		** UPDATE, plant a comma after INSERT
		*/
		str = "insert, ";
		str_len = sizeof("insert, ") - 1;
	    }
	    else
	    {
		str = "insert ";
		str_len = sizeof("insert ") - 1;
	    }

	    addString(&evolvingString, str, str_len, 
		       NO_PUNCTUATION, NO_QUOTES );
	}

	if (crt_view->ahd_crt_view_flags & AHD_CHECK_OPT_DYNAMIC_UPDT)
	{
	    char	*str;
	    i4		str_len;

	    if (crt_view->ahd_crt_view_flags & AHD_VERIFY_UPDATE_OF_ALL_COLS)
	    {
		str = "update ";
		str_len = sizeof("update ") - 1;
	    }
	    else
	    {
		str = "update(";
		str_len = sizeof("update(") - 1;
	    }

	    addString(&evolvingString, str, str_len, NO_PUNCTUATION, 
		NO_QUOTES );
	    
	    /* 
	    ** if need to enforce CHECK OPTION after update of some (but not 
	    ** all) columns of the view, add a list of columns now
	    */
	    if (~crt_view->ahd_crt_view_flags & AHD_VERIFY_UPDATE_OF_ALL_COLS)
	    {
		DMT_ATT_ENTRY       	*att_entry;
		i4			i;
		char			*attrmap = 
			(char *) crt_view->ahd_updt_cols.db_domset;

		/* 
		** get the description of the view's attributes during the first
		** pass - don't do it again during the second pass
		*/
		if (pass == 0)
		{
		    status = qea_readAttributes(dsh, ulm, tbl_info, &att_array);
		    if (DB_FAILURE_MACRO(status))
			break;
		}
		
		/* now we will walk the map of attributes of the view s.t.
		** CHECK OPTION needs to be enforced whenever one or more of 
		** them are updated and add the list of columns to our text
		*/

		/* 
		** add the first column outside of the loop since all the 
		** subsequent columns will have a leading comma; this will make
		** the loop a tiny bit more efficient
		*/
		i = BTnext(0, attrmap, DB_MAX_COLS + 1);
		if (i == -1)
		{
		    i4		error = E_QE0018_BAD_PARAM_IN_CB;

		    /* 
		    ** something was very wrong - at least one bit must be set 
		    */
		    qef_error(error, 0L, status, &error, &dsh->dsh_error, 0);
		    break;
		}

		att_entry = att_array[i];

		status = addUnnormalizedName(
		    att_entry->att_nmstr, att_entry->att_nmlen,
		    unnormalizedName, &unnormalizedNameLength,
		    &evolvingString, NO_PUNCTUATION, NO_QUOTES, dsh);
		if (DB_FAILURE_MACRO(status))
		    break;

		/* 
		** now add the remaining columns taking care to precede them 
		** with commas
		*/
		for (; (i = BTnext(i, attrmap, DB_MAX_COLS + 1)) != -1;)
		{
		    att_entry = att_array[i];

		    status = addUnnormalizedName(
			att_entry->att_nmstr, att_entry->att_nmlen,
			unnormalizedName, &unnormalizedNameLength,
			&evolvingString, LEAD_COMMA, NO_QUOTES, dsh);
		    if (DB_FAILURE_MACRO(status))
			break;
		}
		if (status != E_DB_OK)
		    break;

	 	/* add the closing paren */   
	        addString(&evolvingString, ") ", sizeof(") ") - 1,
		    NO_PUNCTUATION, NO_QUOTES );
	    }
	}

	/* add OF */
	addString(&evolvingString, "of ", sizeof("of ") - 1,
	    NO_PUNCTUATION, NO_QUOTES );
	
	/* add schema */
	status = addUnnormalizedName(schema_name->db_own_name,
	    DB_OWN_MAXNAME, unnormalizedName, &unnormalizedNameLength,
	    &evolvingString, NO_PUNCTUATION, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;

	/* add .view_name */
	status = addUnnormalizedName(view_name->db_tab_name,
	    DB_TAB_MAXNAME, unnormalizedName, &unnormalizedNameLength,
	    &evolvingString, LEAD_PERIOD, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;

	if (bld_row_level_rule)
	{
	    /* building a row level rule calling a "regular" dbproc */

	    /* add EXECUTE PROCEDURE */
	    addString(&evolvingString, 
		" execute procedure ", sizeof(" execute procedure ") - 1, 
		NO_PUNCTUATION, NO_QUOTES );
	}
	else
	{
	    /* building a statement-level rule calling a set input dbproc */
	    
	    /* add WITH RULE_SCOPE = STATEMENT EXECUTE PROCEDURE */
	    addString(&evolvingString, 
		" for each statement execute procedure ", 
		sizeof(" for each statement execute procedure ") - 1, 
		NO_PUNCTUATION, NO_QUOTES );
	}
	
	/* add schema */
	status = addUnnormalizedName(schema_name->db_own_name,
	    DB_OWN_MAXNAME, unnormalizedName, &unnormalizedNameLength,
	    &evolvingString, NO_PUNCTUATION, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;

	/* add .dbp_name */
	status = addUnnormalizedName(dbp_name->db_dbp_name,
	    DB_DBP_MAXNAME, unnormalizedName, &unnormalizedNameLength,
	    &evolvingString, LEAD_PERIOD, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;

	/* add (row_tid = new.tid) */
	addString(&evolvingString, " (row_tid = new.tid) ", 
	    sizeof(" (row_tid = new.tid) ") - 1, NO_PUNCTUATION, 
	    NO_QUOTES );
	
	/* now concatenate all the string fragments */

	endString(&evolvingString, rule_text );

    }

    return(status);
}

/*
** Name: qea_cop_text - build text for a Proc which will enforce Check Option
**
** Description:
**	This function will construct text for a Check Option dbProc.  It will 
**	use information found in the QEF_CREATE_VIEW action as well as the DSH 
**	in its task.
**
** Input:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	crt_view			create view action structure
**
** Outputs:
**	*dbp_text			dbproc text
**      qef_rcb
**	    .error.err_code		filled in if an error occurs
**
** History:
**	sometime-in-march-93 (andre)
**	    written
**      08-mar-93 (anitap)
**          Removed break at the end of for loop. The query text string is
**          built in two passes. But we were breaking out of the loop after the
**          first pass.
**	31-mar-93 (andre)
**	    optimize things a bit by not performing computations during the
**	    second pass (they will be performed during the first pass so that we
**	    can compute the total buffer size)
**	12-apr-93 (andre)
**	    will use cui_idunorm() to correctly convert names into delimited
**	    identifiers
**	07-jun-93 (andre)
**	    we are introducing 2 new tracepoints to control the CHECK OPTION
**	    enforcement mechanism:
**		if QEF_T_CHECK_OPT_ROW_LVL_RULE is set,
**		    we will use a row level rule
**		else if QEF_T_CHECK_OPT_TIDOJ_SET_DBP is set,
**		    we will use a statement-level rule with a set-input dbproc
**		    containing an outer tid join (this will allow me to remove
**		    #ifdef outer_tid_join_works)
**		else
**		    we will use a statement-level rule with a set-input dbproc
**		    containing a subselect 
**	28-jan-94 (andre)
**	    QEF_T_CHECK_OPT_ROW_LVL_RULE is no more;
**	    one can set trace point QE10 (QEF_T_CHECK_OPT_STMT_LVL_RULE) to use
**	    statement level rule with dbproc NOT involving an outer join on TID
**	    or QE10 AND QE8 (QEF_T_CHECK_OPT_TIDOJ_SET_DBP) to use statement 
**	    level rule with dbproc involving an outer join on TID
*/
static DB_STATUS
qea_cop_text(
	QEE_DSH		*dsh,
	QEF_CREATE_VIEW	*crt_view,
	DB_TEXT_STRING	**dbp_text)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    ULM_RCB		*ulm = (ULM_RCB *) dsh->dsh_row[crt_view->ahd_ulm_rcb];
    DB_TAB_NAME		*view_name = &crt_view->ahd_view_name;
    DB_OWN_NAME		*schema_name = 
	(DB_OWN_NAME *) dsh->dsh_row[crt_view->ahd_view_owner];
    DB_DBP_NAME		*dbp_name = 
	(DB_DBP_NAME *) dsh->dsh_row[crt_view->ahd_check_option_dbp_name];
    char		err_no_buf[DECIMAL_DIGITS_IN_A_LONG];
    EVOLVING_STRING	evolvingString;
    i4			pass;
    char		unnormalizedName[ UNNORMALIZED_NAME_LENGTH ];
    i4			unnormalizedNameLength;
    bool		bld_regular_dbproc;
    bool		bld_set_input_dbproc_with_tid_oj;
    i4		val1, val2;

    /*
    ** first determine how we will go about enforcing the CHECK OPTION
    */
    if (ult_check_macro(&qef_cb->qef_trace, QEF_T_CHECK_OPT_STMT_LVL_RULE,
	    &val1, &val2))
    {
	/* will concoct text for a set-input dbproc */
	bld_regular_dbproc = FALSE;

	/* 
	** determine wehther this will be a set-input dbproc involving outer 
	** join on TID
	*/
	bld_set_input_dbproc_with_tid_oj = 
	    ult_check_macro(&qef_cb->qef_trace, QEF_T_CHECK_OPT_TIDOJ_SET_DBP,
		&val1, &val2);
    }
    else
    {
	/*
	** will concoct text for a regular dbproc
	*/
	bld_regular_dbproc = TRUE;
	bld_set_input_dbproc_with_tid_oj = FALSE;
    }

    for ( pass = 0; pass < TEXT_COMPILATION_PASSES; pass++ )
    {
	status = initString( &evolvingString, pass, dsh, ulm );
	if ( status != E_DB_OK )	break;

	/* CREATE PROCEDURE */

	addString(&evolvingString, "create procedure ",
	    sizeof("create procedure ") - 1, NO_PUNCTUATION, NO_QUOTES );

	/* add schema */
	status = addUnnormalizedName(schema_name->db_own_name, DB_OWN_MAXNAME,
	    unnormalizedName, &unnormalizedNameLength, &evolvingString,
	    NO_PUNCTUATION, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;
	
	/* add .dbp_name( */
	status = addUnnormalizedName(dbp_name->db_dbp_name, DB_DBP_MAXNAME,
	    unnormalizedName, &unnormalizedNameLength, &evolvingString,
	    LEAD_PERIOD | APPEND_OPEN_PARENTHESIS, NO_QUOTES, dsh);
	if (DB_FAILURE_MACRO(status))
	    break;

	/*
	** argument declaration syntax depends on whether we are using a row
	** level or statement level rules to enforce CHECK OPTION
	*/
	if (bld_regular_dbproc)
	{
	    /* add "row_tid i8) as declare cnt integer; begin " */
	    addString(&evolvingString, 
		"row_tid i8) as declare cnt integer; begin ",
		sizeof("row_tid i8) as declare cnt integer; begin ") - 1,
		NO_PUNCTUATION, NO_QUOTES );

	    /* add "select any(v.tid) into :cnt from " */
	    addString(&evolvingString,
		"select any(v.tid) into :cnt from ", 
		sizeof("select any(v.tid) into :cnt from ") - 1,
		NO_PUNCTUATION, NO_QUOTES );

	    /* add schema */
	    status = addUnnormalizedName(schema_name->db_own_name, DB_OWN_MAXNAME,
		unnormalizedName, &unnormalizedNameLength, &evolvingString,
		NO_PUNCTUATION, NO_QUOTES, dsh);
	    if (DB_FAILURE_MACRO(status))
		break;
	    
	    /* add .view_name */
	    status = addUnnormalizedName(view_name->db_tab_name, DB_TAB_MAXNAME,
		unnormalizedName, &unnormalizedNameLength, &evolvingString,
		LEAD_PERIOD, NO_QUOTES, dsh);
	    if (DB_FAILURE_MACRO(status))
		break;
	    
	    /*
	    ** add " v where v.tid = :row_tid;" (v is a correlation name for the
	    ** view)
	    */
	    addString(&evolvingString,
		" v where v.tid = :row_tid;",
		sizeof(" v where v.tid = :row_tid;") - 1,
		NO_PUNCTUATION, NO_QUOTES );

		/* add "if cnt = 0 then execute procedure iierror(errorno = " */
		addString(&evolvingString,
		    "if cnt = 0 then execute procedure iierror(errorno = ",
	      sizeof("if cnt = 0 then execute procedure iierror(errorno =") - 1,
		    NO_PUNCTUATION, NO_QUOTES );
		
	}
	else
	{
	    /*
	    ** CHECK OPTION will be enforced using a statement-level rule
	    */
	    
	    /* add "set_name"; same as the procedure name */
	    status = addUnnormalizedName(dbp_name->db_dbp_name, DB_DBP_MAXNAME,
		unnormalizedName, &unnormalizedNameLength, &evolvingString,
		NO_PUNCTUATION, NO_QUOTES, dsh);
	    if (DB_FAILURE_MACRO(status))
		break;

	    /* add " set of (row_tid i8)) as declare cnt integer; begin " */
	    addString(&evolvingString, 
		" set of (row_tid i8)) as declare cnt integer; begin ",
		sizeof(" set of (row_tid i8)) as declare cnt integer; begin ")
		    - 1,
		NO_PUNCTUATION, NO_QUOTES );

	    /* add "select any(t.row_tid) into :cnt from " */
	    addString(&evolvingString,
		"select any(t.row_tid) into :cnt from ", 
		sizeof("select any(t.row_tid) into :cnt from ") - 1,
		NO_PUNCTUATION, NO_QUOTES );
	    
	    /* add <set name> */
	    status = addUnnormalizedName(dbp_name->db_dbp_name, DB_DBP_MAXNAME,
		unnormalizedName, &unnormalizedNameLength, &evolvingString,
		NO_PUNCTUATION, NO_QUOTES, dsh);
	    if (DB_FAILURE_MACRO(status))
		break;

	    if (bld_set_input_dbproc_with_tid_oj)
	    {
		/*
		** query used to check whether CHECK OPTION has been violated
		** will use a TID outer join
		*/
		
		/* add " t left join " (t is a correlation name for the set) */
		addString(&evolvingString,
		    " t left join ", sizeof(" t left join ") - 1,
		    NO_PUNCTUATION, NO_QUOTES );
		
		/* add schema */
		status = addUnnormalizedName(schema_name->db_own_name, DB_OWN_MAXNAME,
		    unnormalizedName, &unnormalizedNameLength, &evolvingString,
		    NO_PUNCTUATION, NO_QUOTES, dsh);
		if (DB_FAILURE_MACRO(status))
		    break;
		
		/* add .view_name */
		status = addUnnormalizedName(view_name->db_tab_name, DB_TAB_MAXNAME,
		    unnormalizedName, &unnormalizedNameLength, &evolvingString,
		    LEAD_PERIOD, NO_QUOTES, dsh);
		if (DB_FAILURE_MACRO(status))
		    break;
		
		/*
		** add " v on v.tid = t.row_tid where v.tid is null;" (v is a
		** correlation name for the view)
		*/
		addString(&evolvingString,
		    " v on v.tid = t.row_tid where v.tid is null;",
		    sizeof(" v on v.tid = t.row_tid where v.tid is null;") - 1,
		    NO_PUNCTUATION, NO_QUOTES );
	    }
	    else
	    {
		/*
		** query used to check whether CHECK OPTION has been violated
		** will use a subselect
		*/
		
		/* add " t where t.row_tid not in (select v.tid from " */
		addString(&evolvingString,
		    " t where t.row_tid not in (select v.tid from ",
		    sizeof(" t where t.row_tid not in (select v.tid from ") - 1,
		    NO_PUNCTUATION, NO_QUOTES);
		
		/* add schema */
		status = addUnnormalizedName(schema_name->db_own_name, DB_OWN_MAXNAME,
		    unnormalizedName, &unnormalizedNameLength, &evolvingString,
		    NO_PUNCTUATION, NO_QUOTES, dsh);
		if (DB_FAILURE_MACRO(status))
		    break;
		
		/* add .view_name */
		status = addUnnormalizedName(view_name->db_tab_name, DB_TAB_MAXNAME,
		    unnormalizedName, &unnormalizedNameLength, &evolvingString,
		    LEAD_PERIOD, NO_QUOTES, dsh);
		if (DB_FAILURE_MACRO(status))
		    break;
		
		/* add " v);" */
		addString(&evolvingString,
		    " v);", sizeof(" v);") - 1, NO_PUNCTUATION, NO_QUOTES);
	    }

	    /* add "if cnt > 0 then execute procedure iierror(errorno = " */
	    addString(&evolvingString,
		"if cnt > 0 then execute procedure iierror(errorno = ",
		sizeof("if cnt > 0 then execute procedure iierror(errorno = ")
		    - 1,
		NO_PUNCTUATION, NO_QUOTES );
	    
	}
	
	/* 
	** first parameter to IIERROR is the message number - we need to 
	** translate E_US1910_6416_CHECK_OPTION_ERR into an ASCII string
	*/

	if (pass == 0)
	{
	    /* do it during the first pass only - save a few picoseconds */
	    CVla(E_US1910_6416_CHECK_OPTION_ERR, err_no_buf);
	}

	/* add message number */
	addString(&evolvingString, err_no_buf, STlength(err_no_buf),
	    NO_PUNCTUATION, NO_QUOTES );

	/* add ", detail = 0, p_count = 2, p1 = " */
	addString(&evolvingString,
	    ", detail = 0, p_count = 2, p1 = ",
	    sizeof(", detail = 0, p_count = 2, p1 = ") - 1,
	    NO_PUNCTUATION, NO_QUOTES );

	/* add '<schema name>' */
	addString(&evolvingString, schema_name->db_own_name,
	    qec_trimwhite(sizeof(*schema_name), schema_name->db_own_name),
	    NO_PUNCTUATION, SINGLE_QUOTE );

	/* add ", p2 = " */
	addString(&evolvingString,
	    ", p2 = ", sizeof(", p2 = ") - 1,
	    NO_PUNCTUATION, NO_QUOTES );

	/* add '<view_name>') */
	addString(&evolvingString, view_name->db_tab_name, 
	    qec_trimwhite(sizeof(*view_name), view_name->db_tab_name),
	    CLOSE_PARENTHESIS, SINGLE_QUOTE );

	/* add "endif; return; end " */
	addString(&evolvingString,
	    "endif; return; end ", sizeof("endif; return; end ") - 1,
	    NO_PUNCTUATION, NO_QUOTES );

	/* now concatenate all the string fragments */

	endString(&evolvingString, dbp_text );

    }

    return(status);
}

/*
** Name: addUnnormalizedName - unnormalize an object name. add it to a string
**
** Description:
**	This function will call the CUF utility to unnormalize a name.
**	'Unnormalizing' means enclosing the name in double quotes
**	and replacing any embedded " with "".
**
**	After unnormalizing the name, this routine will add it to an evolving
**	string.  This routine is driven by the macro ADD_UNNORMALIZED_NAME.
**
** Input:
**	name				name to unnormalize
**	nameLength			length of that name
**	punctuation			punctuation to add to the string
**
** Outputs:
**	unnormalizedName		where to stick the unnormalized result
**	unnormalizedNameLength		and its length
**	evolvingString			where to string in the quoted name
**      dsh                             where to write errors.
** History:
**	18-mar-93 (rickh)
**	    Done wrote it, I did.
*/
static	DB_STATUS
addUnnormalizedName(
    char		*name,
    i4			nameLength,
    char		*unnormalizedName,
    i4			*unnormalizedNameLength,
    EVOLVING_STRING	*evolvingString,
    i4			punctuation,
    i4			singleQuote,
    QEE_DSH		*dsh
)
{
    DB_STATUS		status = E_DB_OK;
    i4		error;
    i4			untranslatedStringLength;

    *unnormalizedNameLength = UNNORMALIZED_NAME_LENGTH;

    untranslatedStringLength = cus_trmwhite( ( u_i4 ) nameLength, name );
    status = cui_idunorm( ( u_char * ) name,
			  ( u_i4 * ) &untranslatedStringLength,
			  ( u_char * ) unnormalizedName,
			  ( u_i4 * ) unnormalizedNameLength,
			  ( u_i4 ) CUI_ID_DLM,
			  &dsh->dsh_error );
    if ( status != E_DB_OK )
    {  QEF_ERROR( dsh->dsh_error.err_code );  return( status ); }

    addString(	evolvingString, unnormalizedName, *unnormalizedNameLength,
		punctuation, singleQuote );

    return( status );
}

/*
** Name: qea_dep_cons -	  examine an IIDBDEPENDS tuple to determine whether the
**			  dependent object is of type constraint
**				  
** Description:
**	This function will be passed to DMF to qualify  IIDBDEPENDS tuples.
**	When asked to perform RESTRICTed destruction of a UNIQUE or PRIMARY KEY
**	constraint, we need to verify that there are no REFERENCES constraints
**	dependent on it - this function will spare us the expense of looping
**	through all of the IIDBDEPENDS tuples in qea_d_integ()
**
** Input:
**	dummy			DMF wants to pass an argument, even though we
**				don't need one
**	qparams			QEU_QUAL_PARAMS with record address to qualify
**
** Output:
**	qparams.qeu_retval	Set ADE_TRUE if qualifies, else ADE_NOT_TRUE
**
** Returns:
**	E_DB_OK always
**
** History:
**	16-mar-93 (andre)
**	    written
**	11-Apr-2008 (kschendel)
**	    Update qualification function call sequence.
*/
static DB_STATUS
qea_dep_cons(
	void		*dummy,
	QEU_QUAL_PARAMS	*qparams)
{
    DB_IIDBDEPENDS *dep_tuple = (DB_IIDBDEPENDS *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;
    if (dep_tuple->dbv_dtype == DB_CONS)
	qparams->qeu_retval = ADE_TRUE;
    return (E_DB_OK);
}

/*
** Name: makeObjectName - make an object name
**
** Description:
**	This routine constructs an object name.  It takes the seed string
**	and tags on a postfix character where the trailing white space
**	begins.
**
**
** Input:
**	nameSeed		blank padded seed name
**	postfixCharacter	character to tag onto the end
**
** Outputs:
**	targetName		where to write the constructed name
**
** History:
**	15-apr-93 (rickh)
**	    Done wrote it.
*/
static VOID
makeObjectName(
    char	*nameSeed,
    char	postfixCharacter,
    char	*targetName
)
{
    u_i2	unpaddedLength;

    unpaddedLength = cus_trmwhite( ( u_i4 ) DB_MAXNAME, nameSeed );
    MEcopy( ( PTR ) nameSeed, ( u_i2 ) DB_MAXNAME, ( PTR ) targetName );
    targetName[ unpaddedLength ] = postfixCharacter;
}

/* Name: qea_renameExecute - Execute postprocessing for rename operation.
**
** Description:
**	This routine will execute the post-processing for the rename operation 
**	on the tables and columns. 
**	After fixing iirelation/iiattribute catalogs we need to fix the query text
**	for any supported grants. This routine will call the appropriate modules
**	to make this possible. 
**
** History:
**	19-Mar-2010 (gupsh01) SIR 123444
**	    Written.
**	27-May-2010 (gupsh01) Bug 123823
**	    Fix error handling code.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Alter action now in dmu_action.
*/
DB_STATUS
qea_renameExecute(
    QEF_AHD		*qea_act,
    QEF_RCB		*qef_rcb,
    QEE_DSH		*dsh )
{
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    QEF_RENAME_STATEMENT *details = &(qea_act->qhd_obj.qhd_rename);
    DB_STATUS		status = E_DB_OK;
    i4			error;
    dsh->dsh_error.err_code = E_QE0000_OK;

    /* Check if we have a good request */
    if ( (details->qrnm_type != QEF_RENAME_TABLE_TYPE) &&  
         (details->qrnm_type != QEF_RENAME_COLUMN_TYPE))
    {
	status = E_DB_ERROR;
	error = E_QE1996_BAD_ACTION;
	qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
    	return( status );
    }

    /* Get Database id */
    qef_cb->qef_dbid = qef_rcb->qef_db_id;

    for ( ; ; )		/* code block to break out of */
    {
        QEUQ_CB             qeuq_cb;
        DB_QMODE            obj_type;
	DMT_SHW_CB          dmt_show;
	DMU_CB		    dmu_cb, *dmu_ptr = &dmu_cb;
	DMT_TBL_ENTRY       dmt_tbl_entry;
	DB_TAB_NAME	    old_name;
	DB_TAB_NAME         new_name;
	QSF_RCB             qsf_rcb;
	bool                qsf_obj = FALSE;    /* TRUE if holding QSF object */
	bool                dmu_atbl_rename_col = FALSE;
	bool                dmu_atbl_rename_tab = FALSE;
	DB_TEXT_STRING      *out_txt = (DB_TEXT_STRING *) NULL;
    	DB_TEXT_STRING      **result;
	QEU_CB		    *qeu_cb =  ( QEU_CB * ) qea_act->qhd_obj.
							qhd_rename.qrnm_ahd_qeuCB;
	int		    opcode = qeu_cb->qeu_d_op;

	result = &out_txt;
	MEfill(sizeof(old_name), 0, (PTR)&old_name);
	MEfill(sizeof(new_name), 0, (PTR)&new_name);

 	/* get control block from QSF */
    	if (qeu_cb->qeu_d_cb == 0)
    	{
            qsf_rcb.qsf_next = 0;
            qsf_rcb.qsf_prev = 0;
            qsf_rcb.qsf_length = sizeof(QSF_RCB);
            qsf_rcb.qsf_type = QSFRB_CB;
            qsf_rcb.qsf_owner = (PTR)DB_QEF_ID;
            qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
            qsf_rcb.qsf_sid = qef_cb->qef_ses_id;
	    qsf_rcb.qsf_lk_state = QSO_SHLOCK;
	    if (qeu_cb->qeu_qso)
	    {
		qsf_rcb.qsf_obj_id.qso_handle = qeu_cb->qeu_qso;
            	/* access QSF by handle */
            	if (status = qsf_call(QSO_LOCK, &qsf_rcb))
            	{
		    qef_rcb->error.err_code = qsf_rcb.qsf_error.err_code;
		    return (status);
            	}
	    }
            else
            {
		/* get object by name */
		qsf_rcb.qsf_obj_id.qso_type = QSO_QP_OBJ;
		MEcopy((PTR)&qeu_cb->qeu_qp, sizeof (DB_CURSOR_ID),
                		(PTR) qsf_rcb.qsf_obj_id.qso_name);
            	if (status = qsf_call(QSO_GETHANDLE, &qsf_rcb))
            	{
		    qef_rcb->error.err_code = qsf_rcb.qsf_error.err_code;
		    return (status);
            	}
            }
            qsf_obj = TRUE; /* we have successfully claimed the object */
            /*
            ** QP is locked now
            ** we copy the structure because it is a shared object and
            ** we cannot write on it.
            */
            STRUCT_ASSIGN_MACRO(*((DMU_CB *) qsf_rcb.qsf_root), dmu_cb);
	}
    	else
    	{
	    /*
            ** We cannot write on the DMU_CB because it may be a read-only
            ** object to this routine.
            */
	    STRUCT_ASSIGN_MACRO(*((DMU_CB *) qeu_cb->qeu_d_cb), dmu_cb);
    	}

	/* get base table's name */
        dmt_show.type = DMT_SH_CB;
        dmt_show.length = sizeof(DMT_SHW_CB);
        dmt_show.dmt_session_id = qeu_cb->qeu_d_id;
        dmt_show.dmt_db_id = qeu_cb->qeu_db_id;
        dmt_show.dmt_char_array.data_in_size = 0;
        dmt_show.dmt_char_array.data_out_size  = 0;
        dmt_show.dmt_tab_id.db_tab_base = dmu_ptr->dmu_tbl_id.db_tab_base;
        dmt_show.dmt_tab_id.db_tab_index = 0;
        dmt_show.dmt_flags_mask = DMT_M_TABLE;
        dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
        dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
        dmt_show.dmt_char_array.data_address = (PTR)NULL;
        dmt_show.dmt_char_array.data_in_size = 0;
        dmt_show.dmt_char_array.data_out_size  = 0;

        status =  dmf_call(DMT_SHOW, &dmt_show);
        if (status != E_DB_OK)
	{
               error = dmt_show.error.err_code;
	       qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );   
	       return (status);
	}

        qeuq_cb.qeuq_next = qeuq_cb.qeuq_prev   = 0;
        qeuq_cb.qeuq_length = sizeof (qeuq_cb);
        qeuq_cb.qeuq_type           = QEUQCB_CB;
        qeuq_cb.qeuq_owner          = (PTR)DB_QEF_ID;
        qeuq_cb.qeuq_ascii_id       = QEUQCB_ASCII_ID;
        qeuq_cb.qeuq_db_id          = qeu_cb->qeu_db_id;
        qeuq_cb.qeuq_d_id           = qeu_cb->qeu_d_id;
        qeuq_cb.qeuq_eflag          = qeu_cb->qeu_eflag;
        qeuq_cb.qeuq_rtbl           = &dmu_ptr->dmu_tbl_id;
        qeuq_cb.qeuq_status_mask    = dmt_tbl_entry.tbl_status_mask;
        qeuq_cb.qeuq_qid.db_qry_high_time = qeuq_cb.qeuq_qid.db_qry_low_time = 0;
        qeuq_cb.qeuq_flag_mask	    = 0; 
	qeuq_cb.qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;
        qeuq_cb.qeuq_keydropped	    = FALSE;
        qeuq_cb.qeuq_keypos	    = 0;
        qeuq_cb.qeuq_keyattid	    = 0;
        qeuq_cb.qeuq_rtbl	    = &dmu_cb.dmu_tbl_id;

	/* Make sure this is a table */
        if ((dmt_tbl_entry.tbl_status_mask & DMT_VIEW)
            || (dmt_tbl_entry.tbl_status_mask & DMT_IDX))
	{
	    /* Error rename operation is not allowed on a view or a table.
	    ** If we receive a view or index then something is not right.
	    */
	    status = E_DB_ERROR;
	    error = E_QE1996_BAD_ACTION;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	}
        else
            obj_type = (DB_QMODE) DB_TABLE;

        /* Find out the mode for rename operation */
	if (opcode == DMU_ALTER_TABLE)
	{
	    if (dmu_ptr->dmu_action == DMU_ALT_TBL_RENAME)
		dmu_atbl_rename_tab = TRUE;
	    else if (dmu_ptr->dmu_action == DMU_ALT_COL_RENAME)
		dmu_atbl_rename_col = TRUE;
	}
	if (dmu_atbl_rename_tab)
       	{
            STmove(dmu_ptr->dmu_table_name.db_tab_name, '\0', 
				sizeof(old_name.db_tab_name), (PTR)&old_name.db_tab_name);
            STmove(dmu_ptr->dmu_newtab_name.db_tab_name, '\0', 
				sizeof(old_name.db_tab_name), (PTR)&new_name.db_tab_name);
	    qeuq_cb.qeuq_flag_mask |= QEU_RENAME_TABLE;
        }
	else if (dmu_atbl_rename_col)
	    qeuq_cb.qeuq_flag_mask |= QEU_RENAME_COLUMN;
	else
	{
	    /* We should be doing rename return with error */
	    status = E_DB_ERROR;
	    error = E_QE1996_BAD_ACTION;
	    qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
	}

	/* Propagate the new object name to the query text for grants */
	status = qeu_rename_grants (qef_rcb->qef_cb, &qeuq_cb, &new_name);

	/* delete the QSF object */
	if (qeu_cb->qeu_d_cb == 0 && qsf_obj)
    	{
            if (status = qsf_call(QSO_DESTROY, &qsf_rcb))
            {
		qef_rcb->error.err_code = qsf_rcb.qsf_error.err_code;
		return (status);
	    }
	}
	break;
    }	

    return( status );
}
