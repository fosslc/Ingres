/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <qefkon.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <uld.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <qefmain.h>
#include    <qefscb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <qefqp.h>
#include    <dudbms.h>
#include    <qefdsh.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <ex.h>
#include    <tm.h>
#include    <ade.h>
#include    <sxf.h>
#include    <qefprotos.h>	
#include    <usererror.h>
#include    <stdarg.h>
/**
**
**  Name: QEFERROR.C - Error formatting and reporting functions.
**
**  Description:
**      This file contains the error formatting and reporting functions for 
**      QEF.
**
**          qef_error	    - Format and report a QEF error.
**	    qef_adf_error   - Report an ADF error that occured in QEF
**
**
**  History:
**	28-may-86 (daved)
**	    written
**	08-sep-87 (puree)
**	    modified qef_error to use the error mapping table instead of
**	    case statements.
**	06-nov-87 (puree)
**	    change error code for ADF internal error from QE001D to QE000B.
**	01-dec-87 (puree)
**	    translate E_DM0110 to user code 5527 instead of 5541.
**	01-dec-87 (puree)
**	    If a transaction is forced to abort, send message to user even if
**	    the err_eflag is internal.
**	04-dec-87 (puree)
**	    Added translation of E_DM0100_DB_INCONSISTENT to 
**	    E_QE0099_DB_INCONSISTENT and force the transaction to be aborted.
**	07-dec-87 (puree)
**	    Log original error only if it's different from the mapped code.
**	07-dec-87 (puree)
**	    Initialize qef_code and user_code to 0L.
**	08-dec-87 (puree)
**	    Allow original facility codes to be passed to user/log file.
**	    Add E_DM0050_TABLE_NOT_LOADABLE in QEF error table.
**	22-dec-87 (puree)
**	    Added error codes from ULH
**	15-jan-88 (puree)
**	    Added error codes for multi-location table.
**	02-feb-88 (puree)
**	    Added E_DM001F_LOCATION_LIST_EXIST in QEF error table.
**	23-feb-88 (puree)
**	    Added E_QE0110_NO_ROWS_QUALIFIED mapping.
**	26-may-88 (puree)
**	    Added error codes for DB procedure.
**	02-aug-88 (puree)
**	    Added E_DM007B_CANT_CREATE_TABLEID and mapped it to E_QE0116
**	    CANT_ALLOCATE_DBP_ID.
**	03-aug-88 (puree)
**	    Modify the error mapping table to send message to user for any
**	    error that is to be logged in the error log file.
**	08-aug-88 (puree)
**	    Added E_DM0114_FILE_NOT_FOUND to the error map.
**	17-aug-88 (puree)
**	    Fix qef_error to set error code in the caller buffer for
**	    E_QE000B, E_QE0015, and E_QE0025.
**	17-aug-88 (puree)
**	    Added E_QE0117_NONEXISTENT_DBP.
**	29-sep-88 (puree)
**	    Modified qef_error for database procedure built-in variables.
**	29-nov-88 (puree)
**	    Change E_QE0002_INTERNAL_ERROR for most errors, including when
**	    an illegal error code is received, to E_QE001D_FAIL since this
**	    is more accurate.
**	09-nov-88 (puree)
**	    Set log flag for force-abort and deadlock
**	03-jan-89 (puree)
**	    Modified qef_error for E_QE001D to report the original error.
**	21-mar-89 (neil)
**	    Get message to user whatever happens.
**	1-apr-89 (paul)
**	    Add error message map entries for rule and resource control errors.
**	11-apr-89 (neil)
**	    Added E_QE209 for rule statements.
**	03-Mar-89 (ac)
**	    Added 2PC error mapping support.
**	10-may-89 (ralph)
**	    GRANT Enhancements, Phase 2: 
**	    Added error numbers E_QE0210 thru E_QE0227
**	23-may-89 (jrb)
**	    Renamed scf_enumber to scf_local_error and filled in
**	    scf_generic_error with generic error code obtained from ule_format.
**	31-may-89 (neil)
**	    Added E_QE123-4 for DBP execution.
**	13-jun-89 (fred)
**	    Added recognition of ADF_EXTERNAL_ERROR class.  These are passed
**	    back to the FE as formatted messages to avoid aesthetic
**	    interference.
**	07-aug-89 (neil)
**	    Added E_QE0105.
**	11-aug-89 (teg)
**	    add E_DM009B_ERROR_CHK_PATCH_TABLE to error lookup table.
**	07-sep-89 (neil)
**	    Alerters: Add new errors and new parms.
**	19-sep-89 (nancy)
**	    Fix bug 7623 where an incorrectly stated order by list produces 
**	    internal error 201.  Make QE0201 map to user error.
**	08-oct-89 (ralph)
**	    Translate error numbers E_QE0230 thru E_QE0233
**	09-nov-89 (sandyh)
**	    Added handling of E_DM0020_DMU_TOO_MANY.
**	29-dec-89 (neil)
**	    Error E_QE0199, E_QE0200.
**	06-jun-90 (teg)
**	    added handling of E_DM0130_DMMLIST_ERROR and 
**	    E_DM0140_EMPTY_DIRECTORY
**	11-jun-1990 (bryanp)
**	    Handle E_DM0141_GWF_DATA_CVT_ERROR: Error has already been reported
**	    to user by GWF. QEF should abort query, but no further error msg
**	    is needed. To handle this, I over-loaded the 'qef_code' in the
**	    error map: if qef_code is E_QE0122_ALREADY_REPORTED, then no QEF
**	    or user message is logged or sent to the user. It is assumed that
**	    sufficient messages have already been handled. This is used by
**	    gateway processing, where the GW reports the error itself but then
**	    causes DMF to return with an error code. Note that we don't actually
**	    return QE0122 in this case; we return QE0025, because that has
**	    the desired special-case handling elsewhere in QEF and SCF (e.g.,
**	    when SCF sees QE0025, it does NOT generate E_SC0206...). God, this
**	    is a mess...
**	    E_DM0142_GWF_ALREADY_REPORTED is handled the same way. Hopefully
**	    these are the only two that require this handling.
**	14-nov-90 (ralph)
**	    Translate E_QE0237_DROP_DBA to E_US18C3_6339
**	10-dec-90 (neil)
**	    Alerters: Add EVENT errors & new error parms (for longer errors).
**	14-dec-90 (davebf)
**	    On deadlock, insure that user gets message, even if caller of
**	    QEF sets errflag = QEF_INTERNAL.
**	31-dec-90 (ralph)
**	    Translate E_QE0229_DBPRIV_NOT_GRANTED to E_US189B_6299.
**	    Translate E_QE0233_ALLDBS_NOT_AUTH to E_US18C6_6342.
**	15-jan-91 (ralph)
**          Translate E_QE0229_DBPRIV_NOT_GRANTED to E_US189B_6299.
**	25-jan-1991 (teresa)
**	    REMAPPED the following that were previously mapped and are now
**	    somehow missing from the file:
**		E_QE0130 to E_QE0133 (QEF Errors)
**		DMF error E_DM0130 -> E_QE0134
**		DMF error E_DM0131 -> E_QE0135
**		DMF error E_DM0072 -> itself
**	    ALSO mapped the following new:
**		DMF error E_DM0144 -> E_QE0136
**		DMF error E_DM0145 -> E_QE0137
**      02-feb-91 (jennifer)
**          Fix bug 34966 that requires iierrornumber to be set when
**          a user error is return via qef_adf_error.
**	04-feb-91 (neil)
**	    Added E_QE019A.
**	03-mar-91 (ralph)
**	    Identify E_QE0300_USER_MSG as an "internal" message
**	25-feb-91 (rogerk)
**	    Added E_DM0146_SETLG_XCT_ABORT - return as user error.
**	08-may-91 (davebf)
**	    Avoid FE/BE loop on range failure in qef_adf_error()
**	04-jun-91 (wolf) 
**	    Remove temporary defines for E_DM0140, 141 and 142, all of which
**	    can be found in dmf.h
**      16-jul-91 (stevet)
**          Added E_QE0238_NO_MSTRAN, which translated to E_US08B4_2228.
**	01-aug-91 (markg)
**	    Translate E_DM0103 to user code 2045 instead of 5527. Bug 38959.
**	14-aug-91 (markg)
**	    Added handling of E_QE011B_PARAM_MISSING - this new message
**	    replaces E_QE0114_PARAM_MISSING which is now obselete.
**	19-aug-91 (rogerk)
**	    Added E_DM0149_SETLG_SVPT_ABORT - this error is not reported
**	    to the user.
**	12-feb-92 (nancy)
**	    Log error for E_QE0022_QUERY_ABORTED to provide diagnostics for
**	    query aborts not issued by a user interrupt.
**      08-may-91 (davebf)
**          Avoid FE/BE loop on range failure in qef_adf_error().
**      26-feb-92 (anitap)
**          Backed out the following change for bug 40557:
**             08-may-91 (davebf)
**                  Avoid FE/BE loop on range failure in qef_adf_error().
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	08-jun-92 (nancy)
**	    complete fix for 38565, add new error E_QE0301_TBL_SIZE_CHANGE
**	    to error table for completeness.  
**	24-sep-92 (robf)
**	    Added E_QE0400_GATEWAY_ERROR to map errors filtering up 
**	    from GWF (usually via DMF E_DM0136) so user doesn't see an 
**	    unmapped E_SC0206_INTERNAL_ERROR. (Note, no guarantee GWF
**	    has issued a diagnostic by this time or not)
**	    Added E_QE0401_GATEWAY_ACCESS_ERROR so user sees sensible message
**	    after illegal operation (e.g. MODIFY)
**	18-sep-1992 (fpang)
**	    Merged distributed errors E_QE0500 .. E_QE0990 into Qef_err_map[].
**	03-nov-92 (jrb)
**	    Added error mappings to table for new multi-loc sorts errors.
**	08-dec-92 (anitap)
**	    Added #include of psfparse.h and changed the order of 
**	    #include of qsf.h.
**	21-dec-92 (anitap)
**	    Prototyped qef_adf_error().
**	29-dec-92 (robf)
**	    Added E_QE0192_SEC_WRITE_ERROR handling when writing a
**	    Security Audit record fails.
**	03-feb-93 (anitap)
**	    Added E_QE0302_SCHEMA_EXISTS error when the user specifies CREATE
**	    SCHEMA statement more than once.
**	21-mar-93 (andre)
**	    added messages E_QE0166_NO_CONSTRAINT_IN_SCHEMA,
**	    E_QE0167_DROP_CONS_WRONG_TABLE, E_QE0168_ABANDONED_CONSTRAINT, and
**	    E_QE0169_UNKNOWN_OBJTYPE_IN_DROP to the table
**	28-mar-93 (andre)
**	    E_QE0169_UNKNOWN_OBJTYPE_IN_DROP needs to be mapped into
**	    E_QE0002_INTERNAL_ERROR outside of QEF_ERR_MAP since
**	    E_QE0002_INTERNAL_ERROR does not fit into a i4
**
**	    a comma was missing following E_QE0168_ABANDONED_CONSTRAINT
**	20-apr-93 (jhahn)
**	    Added E_QE0258_ILLEGAL_XACT_STMT for reporting when transaction
**	    statements not should have been allowed.
**	15-may-1993 (rmuth)
**	    Add error message for READONLY tables and concurrent index 
**	    operations. E_DM0067_READONLY_TABLE_ERR,  
**	    E_DM0068_MODIFY_READONLY_ERR and E_DM0069_CONCURRENT_IDX.
**	28-may-93 (robf)
**	    Add handling for E_DM201E_SECURITY_DML_NOPRIV.
**	    also E_DM201F_ROW_TABLE_LABEL, E_QE0280_MAC_ERROR,
**	    E_QE0281_DEF_LBL_ERROR
**	    Add direct reporting of E_SX002B_AUDIT_NOT_ACTIVE, returned 
**	    when security auditing has been disabled. 
**	    Report DM201E/DM201F directly as user errors, not QE2033
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	28-jul-93 (rblumer)
**	    Add error E_QE016A_NOT_DROPPABLE_CONS to error map.
**	26-jul-1993 (rmuth)
**	    Include <usererror.h>.
**	    Map E_DM0067_READONLY_TABLE_ERR to a user error. 
**	    Trap the user error E_US14E4_5348_NON_FAST_COPY and mark it
**	    as an EXTERNAL error.
**	3-aug-93 (rickh)
**	    Added error E_QE0138_CANT_CREATE_UNIQUE_CONSTRAINT.
**	11-aug-93 (rickh)
**	    Added E_QE0139_CANT_ADD_REF_CONSTRAINT and
**	    E_QE0140_CANT_EXLOCK_REF_TABLE.
**	23-aug-1993 (rmuth)
**	    Map E_DM0069_CONCURRENT_IDX to a user error.
**	9-sep-93 (rickh)
**	    Added E_QE0144_CANT_ADD_CHECK_CONSTRAINT and
**	    E_QE0145_CANT_EXLOCK_CHECK_TABLE.
**	07-oct-93 (davebf)
**	    Don't log RQ0014.
**	22-oct-93 (andre)
**	    Added E_QE011C_NONEXISTENT_SYNONYM
**	22-oct-93 (robf)
**	    Add mapping for E_DM0129_SECURITY_CANT_UPDATE
**	16-nov-93 (robf)
**          Update handling for E_DM0125_DDL_SECURITY_ERROR, we need
**	    to report as  external error.
**	7-jan-94 (robf)
**          All I_QE2033_SEC_VIOLATION errors get returned as external
**	    errors, using the error code defined in user_error.
**	19-jan-94 (jhahn)
**	    Mapped E_DM0150_DUPLICATE_KEY_STMT and E_DM015_SIDUPLICATE_KEY_STMT
**	    to user errors.
**	27-jan-94 (rickh)
**	    Map E_AD2005_BAD_DTLEN.
**	5-jul-94 (robf)
**          Added more SXF errors
**      29-nov-94 (harpa06)
**          added error message from INGRES 6.4 -- bug #62708:
**               case E_QE005A_DIS_DB_UNKNOWN 
**          in qef_error()
**      09-jan-95 (nanpr06)
**	    Added error message : E_DM0160_MODIFY_ALTER_STATUS_ERR
**	    Added error message : E_DM0161_LOG_INCONSISTENT_TABLE_ERR
**	    Added error message : E_DM0162_LOG_INCONSISTENT_TABLE_ERR
**      17-may-94 (dilma04)
**          Add error message E_DM0170_READONLY_TABLE_INDEX_ERR and map it
**          to QEF error and user error E_US14EB_5355.
**          All I_QE2034_READONLY_TABLE_ERR errors get returned as external
**          errors, using the error code defined in user_error.
**      21-aug-95 (tchdi01)
**          Added error messages
**             E_QE7612_PROD_MODE_ERR
**             E_QE7610_SPROD_UPD_IIDB
**          for the production mode project
**      13-sep-95 (chech02) b71218
**          map message E_DM011C_ERROR_DB_DESTROY and E_DM011A_ERROR_DIR to
**          E_QE0079_ERROR_DELETING_DB for dmm_destroy().
**      19-apr-96 (stial01)
**          Added error message E_QE0309_NO_BMCACHE_BUFFERS
**      22-jul-96 (ramra01)
**          Added error message E_QE009D_ALTER_TABLE_SUPP
**          Added error message E_QE016B_COLUMN_HAS_OBJECTS
**	23-aug-96 (nick)
**	    Add E_DM0168_INCONSISTENT_INDEX_ERR. #77429
**	23-sep-1996 (canor01)
**	    Move global data definitions to qefdata.c.
**	19-dec-1996 (chech02)
**	    Removed READONLY attrib from GLOBALREFs.
**	01-apr-1998 (hanch04)
**	    Calling SCF every time has horendous performance implications,
**	    re-wrote routine to call CS directly with qef_session.
**	22-apr-1998 (hanch04)
**	    Back at previous change.  Call for SCU_INFORMATION is needed.
**	11-Aug-1998 (hanch04)
**	    Removed ule_format declaration.
**          Make direct call to ule_doformat because to output is sent to II_DBMS_LOG
**      28-jul-98 (devjo01)
**          B89048: Prevent stack trashing when qef_error() is called
**          with zero parameters passed (hence no param1 or param2) on
**          stack, and qef_code for error map is E_QE001D_FAIL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Replace qef_session() with GET_QEF_CB macro, supply
**	    session's id to SCF/QSF when known.
**      17-Apr-2001 (hweho01)
**          Changed qef_error() to a variable-arg list function.
**          The mismatch of parameter numbers between the calling routines
**          and qef_error() caused stack corruptions during runtime.
**          According to ANSI standard, variable argument list should 
**          be used when the numbers of parameter passed by the callers  
**          are not fixed.   
**/

typedef struct	_QEF_ERR_MAP
{
    i4	err_no;		/* error code from other facilities */
    i4	qef_code;	/* mapped QEF error code
				** if E_QE0122_ALREADY_REPORTED, no additional
				** messages will be reported or logged. */
    i4		user_code;	/* mapped user error message number
				**  0 - the error is considered internal 
				** -1 - user code is the same as qef code */
    i4		log_flg;	/* if 1, log message in system error log */
} QEF_ERR_MAP;



GLOBALREF QEF_ERR_MAP Qef_err_map[];

/*{
** Name: qef_error	- Format and report a QEF error.
**
** Description:
**	Qef_error uses the error mapping table to interpret errors
**	from various routines.  Based on the input error code, error type,
**	qef_eflag, and the action specified in the table, qef_error may 
**	send message to the user, or log it in the error log file, or
**	both according to the following algorithm:
**
**	Qef_error locates an entry in the error mapping table based on the
**	input error code.  If the corresponding user code from the table
**	entry is not zero, qef_error considers it a user error.  If the
**	qef_eflag is external, the message text is sent to the user and the
**	returned error code is set to E_QE0025_USER_ERROR.  If the qef_eflag
**	is internal, no message is sent to the user.  In this case, the 
**	returned error code is set to the mapped qef code.
**
**	If the log_flg in the table entry is set, the qef message is logged.
**	However, if the error type is E_DB_SEVERE or E_DB_FATAL, qef_error
**	always logs the error message.
**
**	Errors not found in the error mapping table is considered an QEF 
**	internal error.  It will also be logged.
**  
** Inputs:
**      errorno                         Error number
**      detail				detail of error
**      err_type                        Type of error
**					    E_DB_ERROR
**					    E_DB_WARN
**					    E_DB_SEVERE
**					    E_DB_FATAL
**	err_code			Ptr to error code if qef_error cannot
**					complete its function.
**	err_blk				Pointer to output error block
**	FileName			Filename of function caller
**	LineNumber			Line number within that file
**	num_parms			Number of parameter pairs to follow.
** NOTE: The following parameters are optional, and come in pairs.  The first
**  of each pair gives the length of a value to be inserted in the error
**  message.  The second of each pair is a pointer to the value to be inserted.
**      param1                          First error parameter, if any
**      param2                          Second error parameter, if any
**      param3                          Third error parameter, if any
**      param4                          Fourth error parameter, if any
**      param5                          Fifth error parameter, if any
**      param6                          Sixth error parameter, if any
**	param7				Seventh error parameter, if any
**	param8				Eighth error parameter, if any
**	param9				Ninth error parameter, if any
**	param10				Tenth error parameter, if any
**
** Outputs:
**      err_code                        Filled in with error code if qef_error
**					cannot complete its function.
**	err_blk				Filled in with mapped error code.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Error by caller
**	    E_DB_FATAL			Couldn't format or report error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause message to be logged or sent to user.
**
** **	30-dec-85 (jeff)
**          written
**	28-may-86 (daved)
**	    modified for use by QEF
**	31-aug-87 (puree)
**	    added more mapping for DMF error
**	08-sep-87 (puree)
**	    modified qef_error to use the error mapping table instead of
**	    case statements.
**	06-nov-87 (puree)
**	    change error code QE001D from ADF to QE000B and error
**	    from qeq_escalate to QE0005.
**	01-dec-87 (puree)
**	    translate E_DM0110 to user code 5527 instead of 5541.
**	01-dec-87 (puree)
**	    If a transaction is forced to abort, send message to user even if
**	    the err_eflag is internal.
**	04-dec-87 (puree)
**	    Added translation of E_DM0100_DB_INCONSISTENT to 
**	    E_QE0099_DB_INCONSISTENT and force the transaction to be aborted.
**	07-dec-87 (puree)
**	    Log original error only if it's different from the mapped code.
**	07-dec-87 (puree)
**	    Initialize qef_code and user_code to 0L.
**	08-dec-87 (puree)
**	    Allow original facility codes to be passed to user/log file.
**	    Add E_DM0050_TABLE_NOT_LOADABLE in QEF error table.
**	02-feb-88 (puree)
**	    Added E_DM001F_LOCATION_LIST_EXIST in QEF error table.
**	23-feb-88 (puree)
**	    Added E_QE0110_NO_ROWS_QUALIFIED mapping.
**	17-aug-88 (puree)
**	    Fix qef_error to set error code in the caller buffer for
**	    E_QE000B, E_QE0015, and E_QE0025.
**	29-nov-88 (puree)
**	    Change E_QE0002_INTERNAL_ERROR for most errors, including when
**	    an illegal error code is received, to E_QE001D_FAIL since this
**	    is more accurate.
**	28-jan-89 (jrb)
**	    Added num_parms to qef_error.
**	09-mar-89 (jrb)
**	    Now passes num_parms thru to ule_format.
**	21-mar-89 (neil)
**	    Get message to user whatever happens plus pass correct num_parms.
**	21-may-89 (jrb)
**	    Renamed scf_enumber to scf_local_error and filled in
**	    scf_generic_error with generic error code obtained from ule_format.
**	11-jun-90 (bryanp)
**	    If qef_code is E_QE0122_ALREADY_REPORTED, then this message need
**	    not be logged nor sent to the user. This qef_code should be used
**	    ONLY for errors which are guaranteed to have already been reported
**	    to the user and/or error log. It gets a special map back to QE0025.
**	10-dec-90 (neil)
**	    Alerters: Allow full 10 parameters to error message.
**	14-dec-90 (davebf)
**	    On deadlock, insure that user gets message, even if caller of
**	    QEF sets errflag = QEF_INTERNAL.
**	25-jan-91 (teresa)
**	    REMAPPED the following that were previously mapped and are now
**	    somehow missing from the file:
**		E_QE0130 to E_QE0133 (QEF Errors)
**		DMF error E_DM0130 -> E_QE0134
**		DMF error E_DM0131 -> E_QE0135
**		DMF error E_DM0072 -> itself
**	    ALSO mapped the following new:
**		DMF error E_DM0144 -> E_QE0136
**		DMF error E_DM0145 -> E_QE0137
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	24-oct-92 (andre)
**	    ule_format() now expects a (DB_SQLSTATE *) (sqlstate) instead of
**	    (i4 *) (generic_error) +
**	    in SCF_CB, scf_generic_error has been replaced with scf_sqlstate
**      29-nov-94 (harpa06)
**          added error message from INGRES 6.4 -- bug #62708:
**               case E_QE005A_DIS_DB_UNKNOWN 
**      31-Aug-98 (wanya01)
**          Bug 77435: IPM session detail display shows "Performing Force
**          Abort Processing" as activity for session which is executing a
**          normal rollback. Take out change 415103 which fix bug 62550 in
**          dmxe.c. Call scf_call and set correct activity message when
**          deadlock is dectected.
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**          Added I_QE2037 to list of messages that must generate front
**          end errors.
**	22-mar-2006 (toumi01)
**	    Add I_QE2038 akin to I_QE2037 to allow for different message
**	    for MODIFY (vs. DROP) giving user advice re NODEPENDENCY_CHECK.
**	25-may-06 (dougi)
**	    Add display of procedure name/line number for errors in 
**	    procedure execution, and list of open tables if tp qe130 is on.
**	7-Jun-2006 (kschendel)
**	    Make sure we're in an action before calling procerr, might
**	    be a parameter setup error and there's no action yet.
**	28-Jul-2006 (jonj)
**	    Preserve originating errorno and resultant qef_err in
**	    the QEF_CB, useful to the sequencer for XA transactions.
**	16-Apr-2008 (kschendel)
**	    Add the DMF "already issued" error to the fastpath list.
**	11-Sep-2008 (jonj)
**	    SIR 120874: Define MAX_PARMS to shield running off the stack.
**	    Rename to qefErrorFcn, invoked by qef_error, qefError macros,
**	    first parm is (optional) DB_ERROR*.
**	    Use CLRDBERR, SETDBERR to manage DB_ERROR structures.
**	    Use new form of uleFormat().
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Shush a compiler warning.
*/

VOID
qefErrorFcn(
DB_ERROR	*dberror,
i4 		errorno, 
i4  		detail,
i4 		err_type, 
i4 		*err_code,
DB_ERROR 	*err_blk, 
PTR		FileName,
i4		LineNumber,
i4 		num_parms,
		...)
{
    QEF_ERR_MAP		*err_map;
    i4             uletype;
    DB_STATUS		status;
    QEF_CB		*qef_cb;
    CS_SID		sid;
    QEE_DSH		*dsh;
    i4		ulecode;
    i4		msglen;
    DB_SQLSTATE		sqlstate;
    i4			eflag;
    SCF_CB		scf_cb;
    char		errbuf[DB_ERR_SIZE];
    char                buffer[80];
    DB_ERROR	localDBerror, *DBerror;
    DB_ERROR	qefError, userError;

#define MAX_PARMS	5
    i4        param_size[MAX_PARMS];
    PTR       param_value[MAX_PARMS];
    i4	      NumParms;
    i4        i; 
    va_list ap;

    va_start( ap , num_parms );

    for( i = 0;  i < num_parms && i < MAX_PARMS; i++ )  
    {
        param_size[i] = (i4) va_arg( ap, i4 );
        param_value[i] = (PTR) va_arg( ap, PTR );
    }
    NumParms = i;
    va_end( ap );

    /*
    ** If no incoming dberror or there's an overriding errorno,
    ** use caller's source information.
    */
    if ( !dberror || errorno )
    {
        DBerror = &localDBerror;
	DBerror->err_file = FileName;
	DBerror->err_line = LineNumber;
	DBerror->err_code = errorno;
	DBerror->err_data = detail;

	/* Fill caller's dberror with that used */
	if ( dberror )
	    *dberror = *DBerror;
    }
    else
        DBerror = dberror;

    CLRDBERR(&qefError);
    CLRDBERR(&userError);
    *err_code = 0;

    for (;;)	/* Not a loop, just gives a place to break off */
    {
	/* Fast path for most common errors.
	** if user error or fail error, then return because
	** the error has already been processed by qef_adf_error
	*/

	qef_cb = (QEF_CB*)NULL;

	if (DBerror->err_code == E_DM0023_USER_ERROR)
	    DBerror->err_code = E_QE0025_USER_ERROR;

    	if (DBerror->err_code == E_QE0015_NO_MORE_ROWS
	    || DBerror->err_code == E_QE000B_ADF_INTERNAL_FAILURE
	    || DBerror->err_code == E_QE0025_USER_ERROR)
	{
	    *err_blk = *DBerror;
	    break;
	}

	/* Check if we're in a db procedure and display QE00C0
	** with procname and line number, if so. */
	CSget_sid(&sid);
	qef_cb = GET_QEF_CB(sid);
	if (qef_cb)
	{
	    QEE_DSH	*currdsh;

	    currdsh = (QEE_DSH *)qef_cb->qef_dsh;
	    if (currdsh && currdsh->dsh_qp_ptr &&
		currdsh->dsh_qp_ptr->qp_status & QEQP_ISDBP)
		qef_procerr(qef_cb, currdsh);
	}

	if (DBerror->err_code == 0L)
	    DBerror->err_code = E_QE0018_BAD_PARAM_IN_CB;

	/* Map all errors to QEF and user's errors */

	err_map = (QEF_ERR_MAP *)&Qef_err_map[0];
	if ((DBerror->err_code >> 16) == 0)	/* if user error code */
	{
	    userError = *DBerror;		/* set user code directly */
	    SETDBERR(&qefError, 0, E_QE0025_USER_ERROR);
	    uletype = 0L;			/* do not log this error */
	}
	
	/* find user error code if a QEF error */
	else if ((DBerror->err_code >> 16) == DB_QEF_ID )	/* if QEF error code */
	{
	    qefError = *DBerror;		/* Set QEF code directly */
	    while (err_map->qef_code != 0L && err_map->qef_code != DBerror->err_code)
		++err_map;			/* search user code */

	    if (err_map->qef_code == DBerror->err_code)	/* if found */
	    {
		if (err_map->user_code == -1L)	/* set user's code */
		    SETDBERR(&userError, 0, qefError.err_code);
		else
		    SETDBERR(&userError, 0, (i4)(err_map->user_code));

		if (err_map->log_flg)		/* set log flag */
		    uletype = ULE_LOG;
		else
		    uletype = 0L;
	    }
	    else				/* if not found */
	    {
		/* no user message */
		uletype = ULE_LOG;		/* must log message */
	    }
	}
	
	else if ((DBerror->err_code >> 16) > DB_MAX_FACILITY)  /* if bad error code */
	{
	    SETDBERR(&qefError, 0, E_QE0018_BAD_PARAM_IN_CB);
	    uletype = ULE_LOG;
	}
	else					/* error from other facility */
	{
	    while (err_map->qef_code != 0L && err_map->err_no != DBerror->err_code)
		++err_map;			/* search for QEF code */

	    if (err_map->err_no == DBerror->err_code)	/* if found */
	    {
		if (err_map->qef_code == -1L)	/* set qef_code */
		    qefError = *DBerror;
		else
		{
		    qefError = *DBerror;
		    qefError.err_code = err_map->qef_code;
		}

		if (err_map->user_code == -1L)	/* set user's code */
		{
		    if (qefError.err_code != E_QE0122_ALREADY_REPORTED)
		        userError = qefError;
		}
		else
		{
		    userError = qefError;
		    userError.err_code = err_map->user_code;
		}

		if (err_map->log_flg)		/* set log flag */
		    uletype = ULE_LOG;
		else
		    uletype = 0L;
	    }
	    else
	    {
		/* unexpected error */
		SETDBERR(&qefError, 0, E_QE009C_UNKNOWN_ERROR);
		/* no user message */
		uletype = ULE_LOG;   /* force log */
	    }
	}

	/*
	** set return value. E_QE0122 is used by the GWF facility when GWF has
	** reported the error as desired but DMF returned with a failure code.
	** Turning this into E_QE0025_USER_ERROR causes no more error messages
	** to be issued.
	*/
	*err_blk = *DBerror;
	if (qefError.err_code == E_QE0122_ALREADY_REPORTED)
	    err_blk->err_code = E_QE0025_USER_ERROR;
	else
	    err_blk->err_code = qefError.err_code;

	/* We must log severe errors */

	if (err_type == E_DB_FATAL || err_type == E_DB_SEVERE)
	    uletype = ULE_LOG;

	/* If we don't have to log error message and there is no user's
	** message, just return now */

	if (userError.err_code == 0L && uletype == 0L)
	{	
	    break;		/* returns */
	}

	scf_cb.scf_facility = DB_QEF_ID;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
        scf_cb.scf_ascii_id = SCF_ASCII_ID;

	/* Get the qef_cb for the current session to determine
	** the setting of err_eflag */
	eflag = QEF_INTERNAL;

	if ( qef_cb )
	{
	    ERR_CB		*err_cb;
	
	    scf_cb.scf_session = qef_cb->qef_ses_id;
	    dsh = (QEE_DSH *)qef_cb->qef_dsh;
	    err_cb = (ERR_CB *)qef_cb->qef2_rcb;
	    if (err_cb) 
		eflag = err_cb->err_eflag;
	}
	else
	{
	    scf_cb.scf_session = DB_NOSESSION;
	    dsh = (QEE_DSH *)NULL;
	    SETDBERR(&qefError, 0, E_QE0019_NON_INTERNAL_FAILURE);
	    *err_code = E_QE0019_NON_INTERNAL_FAILURE;
	    uletype = ULE_LOG;
	}


	/* Handle special cases */

	switch (qefError.err_code)
	{
	    /* the transaction must be aborted */
	    case E_QE0024_TRANSACTION_ABORTED:
	    case E_QE002A_DEADLOCK:
	    case E_QE0034_LOCK_QUOTA_EXCEEDED:
	    case E_QE0099_DB_INCONSISTENT:
		qef_cb->qef_abort = TRUE;   /* force abort */
		uletype = ULE_LOG;	    /* force error logging */
		eflag = QEF_EXTERNAL;	    /* force user code to fe */
                if (qefError.err_code == E_QE002A_DEADLOCK)
                {
                  scf_cb.scf_ptr_union.scf_buffer = (PTR) buffer;
                  scf_cb.scf_nbr_union.scf_atype = SCS_A_MAJOR;
                  STprintf(buffer, "Aborting on behalf of a deadlock");
                  scf_cb.scf_len_union.scf_blength = STlength(buffer);
                  (VOID) scf_call(SCS_SET_ACTIVITY, &scf_cb);
                }
		break;

            /* DDL security or READONLY table access violation */
	    case I_QE2033_SEC_VIOLATION:
            case I_QE2034_READONLY_TABLE_ERR:
	    case I_QE2037_CONSTRAINT_TAB_IDX_ERR:
	    case I_QE2038_CONSTRAINT_TAB_IDX_ERR:
            case E_QE7612_PROD_MODE_ERR:
	    case E_QE0309_NO_BMCACHE_BUFFERS:
	        eflag = QEF_EXTERNAL;
		break;

	    /* modify user_code */
	    case E_QE0004_NO_TRANSACTION:
		if (qef_cb->qef_operation == QET_ABORT)
		    SETDBERR(&userError, 0, 2171L);
		else if (qef_cb->qef_operation == QET_COMMIT)
		    SETDBERR(&userError, 0, 2173L);
		else if (qef_cb->qef_operation == QET_SAVEPOINT)
		    SETDBERR(&userError, 0, 2174L);
		break;

	    case E_QE0016_INVALID_SAVEPOINT:
	    case E_QE0032_BAD_SAVEPOINT_NAME:
		if (qef_cb->qef_operation == QET_ABORT ||
		    qef_cb->qef_operation == QET_ROLLBACK)
		{
		    SETDBERR(&userError, 0, 2170L);
		}
		break;

	    case E_QE001D_FAIL:
		param_size[0] = sizeof (i4);
		param_value[0] = (PTR)&DBerror->err_code;
		NumParms = 1;
		uletype = ULE_LOG;
		break;

	    case E_QE0054_DIS_TRAN_UNKNOWN:
	    case E_QE0055_DIS_TRAN_OWNER:

/* added error message from INGRES 6.4 -- bug #62708 (harpa06) */

	    case E_QE005A_DIS_DB_UNKNOWN:
		/* 
		** Don't send the user error now. Let SCF send the message
		** back.
		*/
		CLRDBERR(&userError);
		break;

	    case E_QE0022_QUERY_ABORTED:
	        qef_cb->qef_user_interrupt_handled = TRUE;
		break;

	    case E_QE0169_UNKNOWN_OBJTYPE_IN_DROP:
		/*
		** could not map inside the map structure because
		** E_QE0002_INTERNAL_ERROR is a i4
		*/
		SETDBERR(&userError, 0, E_QE0002_INTERNAL_ERROR);
		break;

	    case E_QE0025_USER_ERROR:
		if ( userError.err_code == E_US14E4_5348_NON_FAST_COPY )
		{
		    /*
		    ** COPY currently enforces all its errors to be marked
		    ** internal. This error we want to return to the user
		    ** and treat as an external messgae. So override here.
		    */
		    eflag = QEF_EXTERNAL;   /* force user code to fe */
		}
		break;

	    default:
		break;
	}

	if (eflag == QEF_INTERNAL)	/* do not issue user's message if */
	    CLRDBERR(&userError);	/* internal error */

	/*
	** Log QEF message in the error log file.
	*/
	if (uletype == ULE_LOG)
	{
	    /* If we are logging a QEF error that was mapped, also log the
	    ** the original error too.
	    */
	    if (DBerror->err_code != qefError.err_code)
	    {
		status = uleFormat(DBerror, 0, NULL, uletype, (DB_SQLSTATE *) NULL,
		    errbuf, (i4) sizeof(errbuf), &msglen, &ulecode,
		    NumParms, param_size[0], param_value[0], 
                               param_size[1], param_value[1], 
	                       param_size[2], param_value[2],
	                       param_size[3], param_value[3],
	                       param_size[4], param_value[4]);
	    }
	    
	    status = uleFormat(&qefError, 0, NULL, uletype, &sqlstate, errbuf,
		(i4) sizeof(errbuf), &msglen, &ulecode, NumParms, 
                               param_size[0], param_value[0],
                               param_size[1], param_value[1],
                               param_size[2], param_value[2],
                               param_size[3], param_value[3],
                               param_size[4], param_value[4]);


	    /*
	    ** If ule_format failed, we probably can't report any error.
	    ** Instead, just propagate the error up to the caller.
	    */
	    if (status != E_DB_OK)
	    {
		*err_code = E_QE0002_INTERNAL_ERROR;
		break;
	    }
	}

	/* send user's message to the front end and, if a procedure,
	** set iierrornumber variables */

	if (userError.err_code)
	{
	    if (dsh != (QEE_DSH *)NULL)
	    {
	        QEF_QP_CB	    *qp;
		i4		    *iierrorno;

		qp = dsh->dsh_qp_ptr;
		if (qp->qp_oerrorno_offset != -1)
		{
		    iierrorno = (i4 *)((char *)
				(dsh->dsh_row[qp->qp_rerrorno_row])
				 + qp->qp_oerrorno_offset);
		    *iierrorno = (i4) userError.err_code;
		}
	    }

	    /* get the message text */
	    status = uleFormat(&userError, 0, NULL, 0L, &sqlstate, errbuf,
		    (i4) sizeof(errbuf), &msglen, &ulecode, NumParms, 
                               param_size[0], param_value[0],
                               param_size[1], param_value[1],
                               param_size[2], param_value[2],
                               param_size[3], param_value[3],
                               param_size[4], param_value[4]);

	    /* send the message to the user even if "not found" */
	    scf_cb.scf_ptr_union.scf_buffer = errbuf;
	    scf_cb.scf_len_union.scf_blength = msglen;
	    scf_cb.scf_nbr_union.scf_local_error = userError.err_code;
	    STRUCT_ASSIGN_MACRO(sqlstate, 
				scf_cb.scf_aux_union.scf_sqlstate);
	    (VOID) scf_call(SCC_ERROR, &scf_cb);
	    SETDBERR(err_blk, 0, E_QE0025_USER_ERROR);

	    if (status != E_DB_OK)
	    {
		*err_code = E_QE0002_INTERNAL_ERROR;
		break;
	    }
	}
	*err_code = 0L;
	break;
    }

    /* 
    ** Before returning, save the originating DBerror->err_code and resultant 
    ** qef_code in the QEF_CB. The sequencer needs this for XA transactions 
    ** to determine the  right XA return code, and what gets returned to 
    ** the qef_error() caller in err_blk->err_code = E_QE0025_USER_ERROR 
    ** is pretty much useless for figuring out what error started the mess.
    */
    if ( qef_cb )
    {
	qef_cb->qef_error_errorno = DBerror->err_code;
	qef_cb->qef_error_qef_code = qefError.err_code;
    }

    return;
}

/* Non-variadic function forms, insert __FILE__, __LINE__ manually */
VOID
qefErrorNV(
DB_ERROR	*dberror,
i4 		errorno, 
i4  		detail,
i4 		err_type, 
i4 		*err_code,
DB_ERROR 	*err_blk, 
i4 		num_parms,
		...)
{
    i4	      ps[MAX_PARMS];
    PTR	      pv[MAX_PARMS];
    i4        i; 
    va_list ap;

    va_start( ap , num_parms );

    for( i = 0;  i < num_parms && i < MAX_PARMS; i++ )  
    {
        ps[i] = (i4) va_arg( ap, i4 );
        pv[i] = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    qefErrorFcn( dberror,
		 errorno,
		 detail,
		 err_type,
		 err_code,
		 err_blk,
		 __FILE__,
		 __LINE__,
		 i,
		 ps[0], pv[0],
		 ps[1], pv[1],
		 ps[2], pv[2],
		 ps[3], pv[3],
		 ps[4], pv[4] );
}

VOID
qef_errorNV(
i4 		errorno, 
i4  		detail,
i4 		err_type, 
i4 		*err_code,
DB_ERROR 	*err_blk, 
i4 		num_parms,
		...)
{
    i4	      ps[MAX_PARMS];
    PTR	      pv[MAX_PARMS];
    i4        i; 
    va_list ap;

    va_start( ap , num_parms );

    for( i = 0;  i < num_parms && i < MAX_PARMS; i++ )  
    {
        ps[i] = (i4) va_arg( ap, i4 );
        pv[i] = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    qefErrorFcn( NULL,
		 errorno,
		 detail,
		 err_type,
		 err_code,
		 err_blk,
		 __FILE__,
		 __LINE__,
		 i,
		 ps[0], pv[0],
		 ps[1], pv[1],
		 ps[2], pv[2],
		 ps[3], pv[3],
		 ps[4], pv[4] );
}


/*
** {
** Name: QEF_ADF_ERROR	- format, log, display ADF errors
**
** Description:
**      This routine takes an ADF error block and processes it properly for
**  QEF. If the message is for a user, the formatted error is returned.
**  If the error signals that something suggesting a bug has occurred, a
**  message is logged. 
**
** Inputs:
**      adf_errcb                       the adf error block
**	status				the adf return code
**
** Outputs:
**      qef_rcb				qef request block
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE001D_FAIL
**					E_QE0023_INVALID_QUERY
**					E_QE0025_USER_ERROR
**	Returns:
**	    E_DB_OK			continue processing
**	    E_DB_ERROR			stop processing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jul-86 (daved)
**          written
**	08-sep-87 (puree)
**	    convert E_AD550A to E_QE0023_INVALID_QUERY and set return
**	    status to E_DB_ERROR.
**	06-nov-87 (puree)
**	    change ADF internal error code from QE001D to
**	    QE000B_ADF_INTERNAL_ERROR.
**	03-jan-89 (puree)
**	    Modified E_QE001D to report the original error.
**	13-jun-89 (fred)
**	    Added recognition of ADF_EXTERNAL_ERROR class.  These are passed
**	    back to the FE as formatted messages to avoid aesthetic
**	    interference.
**      02-feb-91 (jennifer)
**          Fix bug 34966 that requires iierrornumber to be set when
**          a user error is return via qef_adf_error.
**	08-may-91 (davebf)
**	    Avoid FE/BE loop on range failure   (b36295/b42291)
**	26-feb-92 (anitap)
**	    Backed out davebf's change. Fix for bug 42291/36295. 
**	24-oct-92 (andre)
**	    ule_format() now expects a (DB_SQLSTATE *) (sqlstate) instead of
**	    (i4 *) (generic_error) +
**	    in both SCF_CB and ADF_ERROR generic_error has been replaced with
**	    sqlstate
**	21-dec-92 (anitap)
**	    Prototyped qef_adf_error().
**	27-jan-94 (rickh)
**	    Map E_AD2005_BAD_DTLEN.
**      13-Apr-1994 (fred)
**          Added mapping of E_AD7010_ADP_USER_INTR.  This is the
**          peripheral equivalent of a dmf interrupt.  If it occurs,
**          it should be treated just like a dmf interrupt.  So we map
**          it to the same thing.
**	24-feb-04 (inkdo01)
**	    Added errptr parm to isolate qef_adf_error() from location of
**	    error fields (for || query execution thread safety).
**	7-Jul-2005 (schka24)
**	    Trace the ADF error code for "other" errors, since ADF may
**	    have forgotten to set up any error message.
**	19-Dec-2005 (kschendel)
**	    Addition of errptr param exposed the fact that what we really
**	    want passed is the session CB, not the request CB.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass adf error source info in ad_dberror to uleFormat.
*/
DB_STATUS
qef_adf_error(
	ADF_ERROR          *adf_errcb,
	DB_STATUS          status,
	QEF_CB             *qefcb,
	DB_ERROR	   *errptr)
{
    DB_STATUS		ret_status;
    i4             error;
    i4		temp;
    QEE_DSH		*dsh;
    SCF_CB		scf_cb;

    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_session = qefcb->qef_ses_id;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;

    if (adf_errcb->ad_errclass == ADF_INTERNAL_ERROR)
    {
	if ( (adf_errcb->ad_errcode == E_AD550A_RANGE_FAILURE) ||
	     (adf_errcb->ad_errcode == E_AD2005_BAD_DTLEN ) )
	{
	    if (adf_errcb->ad_errcode == E_AD2005_BAD_DTLEN)
	        SETDBERR(errptr, 0, E_AD2005_BAD_DTLEN);
	    else
	        SETDBERR(errptr, 0, E_QE0023_INVALID_QUERY);

	    if ((qefcb != (QEF_CB *)NULL) &&
		((dsh = (QEE_DSH *)qefcb->qef_dsh) 
		!= (QEE_DSH *)NULL))
	    {
		dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
	    }
	    ret_status = E_DB_ERROR;
	}
	else if (adf_errcb->ad_errcode == E_AD7010_ADP_USER_INTR)
	{
	    SETDBERR(errptr, 0, E_QE0022_QUERY_ABORTED);
	    ret_status = E_DB_ERROR;
	}
	else
	{
	    if (adf_errcb->ad_emsglen == 0)
		TRdisplay("%@ qef_adf_error: ADF returned %d (%x) with no message\n",
				adf_errcb->ad_errcode, adf_errcb->ad_errcode);
	    uleFormat(&adf_errcb->ad_dberror, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL,
		adf_errcb->ad_errmsgp, adf_errcb->ad_emsglen, &temp, &error, 0);
	    SETDBERR(errptr, 0, E_QE000B_ADF_INTERNAL_FAILURE);
	    ret_status = E_DB_ERROR;
	}
    }
    else
    {
	if (status == E_DB_WARN)
	    ret_status = E_DB_OK;
	else
	    ret_status = E_DB_ERROR;
	scf_cb.scf_ptr_union.scf_buffer  = adf_errcb->ad_errmsgp;
	scf_cb.scf_len_union.scf_blength = adf_errcb->ad_emsglen;
	scf_cb.scf_nbr_union.scf_local_error = adf_errcb->ad_usererr;
	STRUCT_ASSIGN_MACRO(adf_errcb->ad_sqlstate,
			    scf_cb.scf_aux_union.scf_sqlstate);
	if (adf_errcb->ad_errclass == ADF_USER_ERROR)
	{
	    (VOID) scf_call(SCC_ERROR, &scf_cb);
            if ((qefcb != (QEF_CB *)NULL) &&
                ((dsh = (QEE_DSH *)qefcb->qef_dsh)
                != (QEE_DSH *)NULL))
            {

                QEF_QP_CB           *qp;
                i4                  *iierrorno;

                qp = dsh->dsh_qp_ptr;
                if (qp->qp_oerrorno_offset != -1)
                {
                    iierrorno = (i4 *)((char *)
                                (dsh->dsh_row[qp->qp_rerrorno_row])
                                 + qp->qp_oerrorno_offset);
                    *iierrorno = (i4) adf_errcb->ad_usererr;
                }
            }
   
	}
	else
	{
	    (VOID) scf_call(SCC_RSERROR, &scf_cb);
	}
	SETDBERR(errptr, 0, E_QE0025_USER_ERROR);
    }

    return (ret_status);
}


/*
** Name: QEF_PROCERR	- produces QE00C0 message for errors in procs
**
** Description:
**  This function displays a message with procedure name and statement 
**  number to accompany errors in database procedures.
**
** Inputs:
**      qef_cb				ptr to session QEF_CB
**	currdsh				ptr to current DSH
**
** Outputs:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-may-06 (dougi)
**	    Written to improve database procedure error diagnostics.
**	26-july-06 (dougi)
**	    Only do it if tp qe131 is on (server wide) and allow (nil) 
**	    act_ptr.
*/
void
qef_procerr(
	QEF_CB		*qef_cb,
	QEE_DSH		*currdsh)

{
    QEF_AHD     *currahd;
    SCF_CB	scf_cb;
    DB_SQLSTATE sqlstate;
    DB_STATUS	status;

    char	errbuf[DB_ERR_SIZE];
    char	*actnptr;
    i4		ulecode, msglen, stmtno, dummy;


    /* An error has occurred in a database procedure. This function 
    ** displays a QE00C0 message with the procedure name and the number
    ** of the statement corresponding to the current action header to aid
    ** the user in finding the problem. */

    if (ult_check_macro(&Qef_s_cb->qef_trace, QEF_T_PROCERR - QEF_SSNB,
					&dummy, &dummy))
    {
	currahd = currdsh->dsh_act_ptr;
	if (currahd)
	{
	    if (currahd->ahd_atype == QEA_IPROC)
		return;			/* don't display for internal procs */
	    actnptr = uld_qef_actstr(currahd->ahd_atype);
	    stmtno = currahd->ahd_stmtno;
	}
	else 
	{
	    actnptr = "not available";
	    stmtno = -1;
	}

	status = uleFormat(NULL, E_QE00C0_PROC_STMTNO, NULL, ULE_LOG,
	    &sqlstate, errbuf, (i4)sizeof(errbuf),
	    &msglen, &ulecode, 3,
	    sizeof(stmtno), (PTR)&stmtno,
	    STlength(actnptr), (PTR)actnptr,
	    qec_trimwhite(DB_MAXNAME, currdsh->dsh_qp_ptr->
                                qp_id.db_cur_name),
	    (PTR)&currdsh->dsh_qp_ptr->qp_id.db_cur_name);

	scf_cb.scf_facility = DB_QEF_ID;
	scf_cb.scf_session = qef_cb->qef_ses_id;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;

	scf_cb.scf_ptr_union.scf_buffer  = errbuf;
	scf_cb.scf_len_union.scf_blength = msglen;
	scf_cb.scf_nbr_union.scf_local_error = 0;
	STRUCT_ASSIGN_MACRO(sqlstate,
		    scf_cb.scf_aux_union.scf_sqlstate);
			    
	(VOID) scf_call(SCC_ERROR, &scf_cb);
    }
}
