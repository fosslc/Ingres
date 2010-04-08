SQLSTATE_MAPPING_REQUIRED

/*
**    Message file for Facility code: _PS
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	11-oct-93 (rblumer)
**	    Added table name to error messages PS0480 and PS0484;
**	    removed errors PS0497 and PS049F--they are no longer used.
**	22-oct-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	19-nov-1993 (rblumer)
**	    Removed linefeed from message 0354.
**	30-nov-93 (robf)
**	    Add E_PS04D0_GRANT_COPY_ON_VIEW,
**	        E_PS04D1_ALARM_INVALID_OBJECT
**	07-dec-1993 (dianeh)
**	    Refine text destined for messages.txt file added since last time.
**	02-jan-94 (andre)
**	    defined E_PS020E_CANT_GET_DB_JOUR_STATUS
**	07-jan-94 (andre)
**	    defined msgs E_PS04A9-E_PS04AC and W_PS04AD-W_PS04B0
**	09-jan-94 (andre)
**	    defined msg E_PS04B1
**	10-jan-94 (andre)
**	    defined msg E_PS048B
**	20-jan-94 (rblumer)
**	    defined msg E_PS049F_NO_PERIPH_DEFAULT
**	22-feb-94 (robf)
**          define E_PS0F86_SET_ROLE_WITH
**	31-mar-94 (rblumer)
**          define E_PS045F_II_WRONG_CASE
**      14-nov-1994 (tutto01)
**          Added E_PS03A9_FOREIGN_COL_EXCEEDED msg
**	29-dec-94 (wilri01)
**	    Changed error text for E_PS03A2_MISSING_JQUAL:
**	    SS42000_SYN_OR_ACCESS_ERR by removing reference to NATURAL join, 
**	    which was not implemented.
**	27-jan-95 (stephenb)
**	    modify error E_PS0F82 to add new SET options.
**      31-Jan-95 (liibi01)
**          modify the error message for E_PS04D1.
**	17-jan-96 (pchang)
**	    Changed error E_PS03A9_FOREIGN_COL_EXCEEDED to
**	    E_PS03A9_CONSTRAINT_COL_EXCEEDED and altered its message accordingly
**	    since its use is generic to all table constraint types (ref B71935).
**	19-jun-1996 (sweeney)
**	    Fix typos.
**	20-jun-1996 (shero03)
**	    Added Rtree messages, BCA, BCB, and BCC
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**	02-dec-1996 (nanpr01)
**	    Added message E_PS0010_TIMEOUT if lock timer expires instead
**	    of giving E_PS0001_USER_ERROR.
**	10-jan-1997 (nanpr01)
**	    Take out the short remark from the message since this is not
**	    supported.
**      24-jan-97 (dilma04)
**          Add message E_PS03B0_ILLEGAL_ROW_LOCKING if row locking was 
**          specified for a system catalog or a table with 2k page size.
**	26-sep-1997 (shero03)
**	    Added Rtree message BCF.
**	21-apr-1999 (shero03)
**	    Added Substring message C91.
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**          New message E_PS0BB6_PERSISTENT_CONSTRAINT if a MODIFY was
**          specified for an index that supports a non-unique
**          constraint.
**	4-sep-01 (inkdo01)
**	    PS091F "temporarily" disables "first n" syntax from Star queries.
**	19-mar-02 (toumi01)
**	    Changed message for DB_MAX_COLS from 300 to 1024.
**	14-jun-2002 (hayke02)
**	    Added E_PS03AB_AGGR_IN_GROUP_BY. This change fixes bug 107026.
**	5-nov-02 (inkdo01)
**	    Added E_PS03AC_MISSING_COLUMN as part of named columns join impl.
**	24-apr-03 (inkdo01)
**	    Added E_PS0D3D_MISSING_SEQUENCE_PRIV and E_PS0427_ALTOBJ_NOT_OBJ_OWNER 
**	    for sequence support.
**      08-oct-03 (chash01) 
**          Add E_PS1110_REG_TAB_IDX_NULTI_COL:SS50005_GW_ERROR for RMS gateway
**          multicolumn index error if any component is not of type character.
**	17-nov-2003 (xu$we01)
**	    Added E_PS059D_WCO_NONUPDATEABLE_VIEW for bug 110708.
**	09-feb-2004 (somsa01)
**	    Backed out original change for bug 110708 so that it can be
**	    properly fixed later.
**	18-May-2004 (jenjo02)
**	    Cleaned up E_PS0F82, adding additional "WITH" clauses.
**	18-jun-2004 (gupsh01)
**	    Fixed error message E_PS0475_CONSTRAINT_NOT_ALLOWED, to 
**	    alter table alter column statements also.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	22-Jul-2005 (thaju02)
**	    Add E_PS0F87_SET_ON_USER_ERROR.
**	24-Aug-2005 (thaju02)
**	    Modified E_PS0F82.
**	31-May-2006 (kschendel)
**	    Change PS03B0 message, not restricting catalogs.
**	13-Jun-2006 (kschendel)
**	    Invalid sequence default msg.
**      10-Nov-2006 (kschendel)
**          Fix up PS0D32 message to be contentful.
**	08-Feb-2007 (gupsh01)
**	    Modify the error message E_PS0BC6_BAD_COMP_CLAUSE, to state
**	    that NODEPENDENCY_CHECK is valid only on modify.
**	25-oct-07 (smeke01) b117218
**	    Add error message for CASE syntax not supported with subselect.
**	4-june-2008 (dougi)
**	    Added errors PS04B4, PS04B5, PS04B6 and PS04B7 for identity columns.
**	11-Aug-2008 (gupsh01)
**	    Added error message E_PS04B4_DEFAULT_ANSIDT_IN_DBPROC,
**	    informing the user that we don't support defaults for
**	    ANSI date/time/timestamp type parameters/variables
**	    in database procedures.
**      22-aug-2008 (stial01)
**          Added error messages for unicode/utf8 normalization
**	12-Oct-2008 (kiria01) SIR121012
**	    Adjust messages for E_PS0C81, E_PS0C82 and E_PS0C83
**	    reflecting the fact that constant expressions are
**	    supported if constant folding is active.
**	11-May-2009 (kibro01) b121739
**	    Added E_PS03AE_COLREF_IN_WRONG_UNION_PART
**	03-Dec-2009 (kiria01) SIR 121883
**	    Added E_PS03AF_SUBSEL_ORDERBY_WITH_UNION
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Modified PS03B0 to include attempted
**	    lock level.
**	February 2010 (stephenb)
**		Add messages E_PS03B1_NULL_RCB, E_PS03B3_NULL_QUERY_TREE, and
**		E_PS0402_NONUPDATABLE_COLUMN for batch processing.
*/


#define _PS_CLASS 5


E_PS0000_OK:SS00000_SUCCESS
"PSF successful return status"
E_PS0001_USER_ERROR:SS5000B_INTERNAL_ERROR
"User error in query"
E_PS0002_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"Internal error in PSF"
E_PS0003_INTERRUPTED:SS50000_MISC_ERRORS
"Query aborted due to user request."
E_PS0004_EXCEPTION:SS22000_DATA_EXCEPTION
"Exception occurred in PSF"
E_PS0005_USER_MUST_ABORT:SS5000B_INTERNAL_ERROR
"An event has occurred which requires the user to abort the current
transaction."
E_PS0006_UNKNOWN_OPERATION:SS5000B_INTERNAL_ERROR
"The requested operation is unknown to PSF."
E_PS0007_INT_OTHER_FAC_ERR:SS5000B_INTERNAL_ERROR
"PSF detected an internal error when calling other facility."
E_PS0008_RETRY:SS5000B_INTERNAL_ERROR
"Operation should be retried."
E_PS0009_DEADLOCK:SS40001_SERIALIZATION_FAIL
"Resource deadlock encountered."
E_PS0010_TIMEOUT:SS5000B_INTERNAL_ERROR
" Timeout has occurred on a system catalog."
E_PS0101_BADERRTYPE:SS5000B_INTERNAL_ERROR
"psf_error: bad error type"
E_PS0102_TOO_MANY_ERRS:SS5000B_INTERNAL_ERROR
"PSF has lost track of some of the errors that have occurred in this
request."
E_PS0103_ERROR_NOT_FOUND:SS5000B_INTERNAL_ERROR
"Error message corresponding to error, %0d, not found."
E_PS0201_BAD_QLANG:SS0A500_INVALID_QRY_LANG
"psq_bgn_session: bad query language id"
E_PS0202_QLANG_NOT_ALLOWED:SS0A500_INVALID_QRY_LANG
"query language not allowed"
E_PS0203_NO_DECIMAL:SS50004_SYS_CONFIG_ERROR
"psq_bgn_session: decimal marker not specified"
E_PS0204_BAD_DISTRIB:SS50004_SYS_CONFIG_ERROR
"psq_bgn_session: bad distributed/non-distributed id"
E_PS0205_SRV_NOT_INIT:SS5000B_INTERNAL_ERROR
"psq_bgn_session: parser facility has not been started up"
E_PS0206_SES_NOT_FOUND:SS50003_EXHAUSTED_RESOURCE
"no such session in the parser facility"
E_PS0207_YACC_ALLOC:SS50003_EXHAUSTED_RESOURCE
"Could not allocate the YACC control block."
E_PS0208_TOO_MANY_SESS:SS50002_LIMIT_EXCEEDED
"Maximum number of sessions exceeded for parser facility"
E_PS0209_INIT_SEM_FAILURE:SS50006_FATAL_ERROR
"Failed to initialize a semaphore at server startup."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error was encountered while trying to initialize
**	the PSF server block semaphore at server startup.
**
** System Status:
**	The server will be brought down.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS020A_BGNSES_GETSEM_FAILURE:SS50006_FATAL_ERROR
"Failed to acquire a semaphore at session startup."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error was encountered while trying to get the
**	PSF server block semaphore at session startup.
**
** System Status:
**	The session will be aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS020B_BGNSES_RELSEM_FAILURE:SS50006_FATAL_ERROR
"Failed to release a semaphore at session startup."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error was encountered while trying to release the
**	PSF server block semaphore at session startup.
**
** System Status:
**	The server will be brought down.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS020C_ENDSES_GETSEM_FAILURE:SS50006_FATAL_ERROR
"Failed to acquire a semaphore at session shutdown."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error was encountered while trying to get the
**	PSF server block semaphore at session shutdown.
**
** System Status:
**	The server will be brought down.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS020D_ENDSES_RELSEM_FAILURE:SS50006_FATAL_ERROR
"Failed to release a semaphore at session shutdown."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error was encountered while trying to release the
**	PSF server block semaphore at session shutdown.
**
** System Status:
**	The server will be brought down.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS020E_CANT_GET_DB_JOUR_STATUS:SS50000_MISC_ERRORS
"Error encountered at session startup when trying to obtain journaling 
\nstatus of the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error was encountered while trying to determine whether
**	the database is being journaled
**
** System Status:
**	The session will not start
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS0309_ALLOC_SYMTAB:SS50003_EXHAUSTED_RESOURCE
"Could not allocate the symbol table."
E_PS030A_MULT_STMT_ALT:SS5000B_INTERNAL_ERROR
"Attempt to parse second statement in alternate query"
E_PS030B_RETRY_AND_ALT:SS5000B_INTERNAL_ERROR
"Attempt to retry an alternate query"
E_PS030C_NO_PREV_STMT:SS5000B_INTERNAL_ERROR
"Attempt to parse the second statement of a query without doing the first"
E_PS030D_NO_MORE_QRYS:SS5000B_INTERNAL_ERROR
"No more queries in this go block"
E_PS030F_BACKUP:SS50000_MISC_ERRORS
"Error while trying to back up a token"
E_PS0311_YACC_UNKNOWN:SS50000_MISC_ERRORS
"Unknown error from yacc"
E_PS0321_BAD_CHAR1_IN_DELIM_ID:SS42506_INVALID_IDENTIFIER
"Body of a delimited identifier may not start with character '%0c'."
/*%
** Parameters:
**	%0c - character following an opening double-quote (")
**
** Explanation:
**	A character immediately following a double-quote and perceived as the
**	first character of the body of a delimited identifier is illegal.
**	Characters comprising the body of a delimited identifier must follow
**	the rules for characters comprising valid identifiers.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the text of the query and reenter.
*/
E_PS0322_BAD_CHAR_IN_DELIM_ID:SS42506_INVALID_IDENTIFIER
"Body of a delimited identifier may not contain character '%0c'."
/*%
** Parameters:
**	%0c - character found in the body of a delimited identifier
**
** Explanation:
**	A character found in the body of a delimited identifier is illegal.
**	Characters comprising the body of a delimited identifier must follow
**	the rules for characters comprising valid identifiers.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the text of the query and reenter.
*/
E_PS0323_DELIM_ID_TOO_LONG:SS42506_INVALID_IDENTIFIER
"Delimited identifier \"%0c\" is too long.
\nMaximum allowed length is %1d."
/*%
** Parameters:
**	%0c - characters comprising the body of the delimited identifier,
**	      including the first character that exceeded the maximum legal size
**	%1d - maximum allowed length for a body of a delimited identifier
**
** Explanation:
**	The body of a delimited identifier is too long.  This could happen
**	if the closing double-quote was inadvertently omitted.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the text of the query and reenter.
*/
E_PS0324_DELIM_ID_TOO_SHORT:SS42506_INVALID_IDENTIFIER
"Delimited identifier must have a non-empty body."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A delimited identifier with a body of length 0 was specified.  It is
**	possible that the user was attempting to escape a double-quote (") with
**	another double-quote, but in 6.4, the body of a delimited identifier
**	must satisfy the rules for a valid identifier.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the text of the query and reenter.
*/
E_PS0325_MISSING_DELIM_ID_END:SS42506_INVALID_IDENTIFIER
"Non-terminated delimited identifier was encountered."
/*%
** Parameters:
**	None.
**
** Explanation:
**	End-of-query was encountered while parsing the body of a delimited
**	identifier.  Apparently, the closing double-quote is missing.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the text of the query and reenter.
*/
E_PS0326_KEYWORD_IN_DELIM_IDENT:SS42506_INVALID_IDENTIFIER
"Body of a delimited identifier may not consist of a keyword '%0c'."
/*%
** Parameters:
**	%0c - keyword found in the delimited identifier body
**
** Explanation:
**	The body of a delimited identifier was found to consist of a
**	system or SQL keyword, which is illegal in release 6.4.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the text of the query and reenter.
*/
E_PS0351_UNKNOWN_LOCKPARM:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Unknown internal parameter '%1d' for the SET LOCKMODE command.
          This is an internal error.  Please report it to your vendor."
E_PS0352_ILLEGAL_0_TIMEOUT:SS5000A_QUERY_ERROR
"SET LOCKMODE: cannot set TIMEOUT to 0 for table '%0c'
\nsince this may cause server to enter possible undetected
\ndeadlocks when accessing this table."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The server can enter possible undetected deadlocks when accessing
**	core catalogs iistatistics and iihistograms.  Users may not set
**	timeout to 0 on any such tables, unless corrective action is
**	taken to avoid deadlocks.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Specify a different timeout value, or define the "OPF ACTIVE"
**	server startup flag to be equal to the "CONNECTED_SESSIONS" flag.
*/
E_PS0353_NAME_STRING_TOO_LONG:SS42506_INVALID_IDENTIFIER
"Length of a string literal '%0c'
\nrepresenting a user, group, or role name exceeds %1d bytes -
\nthe maximum length of a valid identifier."
/*%
** Parameters:
**	%0c - string literal supplied by the user
**	%1d - maximum length of a valid identifier
**
** Explanation:
**	The length of a literal string supplied by the user to represent a
**	name of a user, group, or role exceeds the maximum length of a valid
**	identifier. Therefore, the string cannot possibly represent a
**	valid user, group, or role name.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0354_SETID_INSUF_PRIV:SS42503_INSUF_PRIV
"SET SESSION AUTHORIZATION: You lack the privileges necessary to execute
this query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to change the session's effective user identifier
**	but lacks the permissions necessary to execute the query.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Only a user with the necessary privileges is allowed to change the
**	session's effective user identifier.
*/
E_PS0355_DIFF_EFF_USER:SS07500_CONTEXT_MISMATCH
"You may not execute dynamically prepared statement '%0c'.
\nCurrent effective user name ('%1c') is different from that
\nwhich was in effect when the statement was prepared ('%2c')."
/*%
** Parameters:
**	%0c - name of the prepared query supplied by the user
**	%1c - current effective user name
**	%2c - user name in effect when the query was prepared
**
** Explanation:
**	The user attempted to execute a prepared query, but the current
**	effective user identifier is different from that in effect when the
**	query was prepared.  Since the query was prepared in a context which
**	was different from the current one, we will take a conservative approach
**	and prevent the user from executing the query.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	The user can either change the effective identifier to match that
**	which was in effect when the query was prepared or prepare that query
**	in the current context.
*/
E_PS0356_DIFF_EFF_GROUP:SS07500_CONTEXT_MISMATCH
"You may not execute a dynamically prepared statement '%0c'.
\nCurrent effective group name ('%1c') is different from that
\nwhich was in effect when the statement was prepared ('%2c')."
/*%
** Parameters:
**	%0c - name of the prepared query supplied by the user
**	%1c - current effective group name
**	%2c - group name in effect when the query was prepared
**
** Explanation:
**	The user attempted to execute a prepared query, but the current
**	effective group identifier is different from that in effect when the
**	query was prepared.  Since the query was prepared in a context which
**	was different from the current one, we will take a conservative approach
**	and prevent the user from executing the query.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	The user can either change the effective identifier to match that
**	which was in effect when the query was prepared or prepare that query
**	in the current context.
*/
E_PS0357_DIFF_EFF_ROLE:SS07500_CONTEXT_MISMATCH
"You may not execute a dynamically prepared statement '%0c'.
\nCurrent effective role name ('%1c') is different from that
\nwhich was in effect when the statement was prepared ('%2c')."
/*%
** Parameters:
**	%0c - name of the prepared query supplied by the user
**	%1c - current effective role name
**	%2c - role name in effect when the query was prepared
**
** Explanation:
**	The user attempted to execute a prepared query, but the current
**	effective role identifier is different from that in effect when the
**	query was prepared.  Since the query was prepared in a context which
**	was different from the current one, we will take a conservative approach
**	and prevent the user from executing the query.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	The user can either change the effective identifier to match that
**	which was in effect when the query was prepared or prepare that query
**	in the current context.
*/
E_PS0358_UNSUPPORTED_IN_B1:SS50008_UNSUPPORTED_STMT
"%0c: this statement is not supported
\nin a B1 Secure system."
/*%
** Parameters:
**	%0c - offending statement
**
** Explanation:
**	The user attempted to execute a query which is not supported in the
**	B1 environment (for example, "SET SESSION AUTHORIZATION").
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	For some statements, there may be workarounds, while the functionality
**	of others may simply be unavailable in B1.  Please consult your 
**	Reference Manual or a Ingres Corporation Technical Support 
**	representative.
*/
E_PS0359_NOT_A_VALID_AUTH_ID:SS28000_INV_AUTH_SPEC
"SET SESSION AUTHORIZATION: '%0c' is not a valid authorization identifier."
/*%
** Parameters:
**	%0c - string found inside a string literal, language variable, or a
**	      dynamic parameter in "SET SESSION AUTHORIZATION" statement
**
** Explanation:
**	The result of trimming leading and trailing blanks from the contents
**	of a string literal, ESQL variable, or dynamic parameter specified for
**	"SET SESSION AUTHORIZATION" statement did not represent a valid
**	authorization identifier.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS035A_CANT_QUERY_SYSCAT:SS42000_SYN_OR_ACCESS_ERR
"%0c is a system catalog. 
\nYou do not have appropriate privileges to directly
query system catalogs"
/*%
** Parameters:
**	%0c - catalog name
**
** Explanation:
**	The indicated table is a system catalog, which you 
** 	are not authorized to query.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.  Try using the standard catalog
**	interface instead.
*/
E_PS035B_CANT_ACCESS_TBL_STATS:SS42000_SYN_OR_ACCESS_ERR
"You are not allowed to directly access table statistics."
/*%
** Parameters:
**	None.
**
** Explanation:
**	You are not allowed to directly access table statistics.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and retry.
*/
E_PS035E_BAD_DECIMAL:SS5000A_QUERY_ERROR
"An illegal decimal specifier was requested."
E_PS0365_BAD_FUNC_INST:SS5000B_INTERNAL_ERROR
"pstnorml.c: Bad function instance %0d."
E_PS0366_NO_COMPLEMENT:SS5000B_INTERNAL_ERROR
"pstnorml.c: Comparison operator '%1c' with no complement."
E_PS0370_OPEN_TEXT_CHAIN:SS50003_EXHAUSTED_RESOURCE
"Could not open a memory stream for text chain."
E_PS0371_ALLOC_TEXT_CHAIN:SS50003_EXHAUSTED_RESOURCE
"Could not allocate a new block from the text chain."
E_PS0372_CLOSE_TEXT_CHAIN:SS5000B_INTERNAL_ERROR
"Could not close the text chain."
E_PS0373_OPEN_QSF_TEXT:SS50003_EXHAUSTED_RESOURCE
"Could not open a QSF stream for storing query text information."
E_PS0374_ALLOC_QSF_TXT:SS50003_EXHAUSTED_RESOURCE
"Could not allocate a block in QSF for query text information."
E_PS0375_UNLOCK_QSF_TEXT:SS5000B_INTERNAL_ERROR
"Could not unlock the QSF object containing query text information."
E_PS0376_INSUF_MEM:SS50003_EXHAUSTED_RESOURCE
"Newly allocated scanner symbol block too small to accommodate data."
E_PS0377_ADI_PM_ERR:SS5000B_INTERNAL_ERROR
"Unexpected error executing adi_pm_encode call."
E_PS0378_SCF_TERM_ERR:SS5000B_INTERNAL_ERROR
"Unexpected error calling SCF to obtain terminal info."
E_PS0379_QSF_T_OR_D_ERR:SS5000B_INTERNAL_ERROR
"Error calling QSF to translate/define."
E_PS037A_QSF_TRANS_ERR:SS5000B_INTERNAL_ERROR
"Error calling QSF to translate."
E_PS037B_UNORM:SS5000B_INTERNAL_ERROR
"Error normalizing data value (%0d,%1d)-> (%2d,%3d)"
E_PS037C_UNORM_DIAG:SS5000B_INTERNAL_ERROR
"Normalization changed data value (%0d,%1d)-> (%2d,%3d)"
E_PS03A0_SUBSEL_IN_JQUAL:SS50008_UNSUPPORTED_STMT
"Subselects are not supported inside join qualifications."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When parsing a join qualification, a subselect is encountered.
**	This is not currently supported.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Reformulate this query so that there are no subselects inside the join
**	qualification.
*/
E_PS03A1_JQUAL_IN_NATJOIN:SS42000_SYN_OR_ACCESS_ERR
"You may not provide join_search qualifications for NATURAL joins."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A qualification was specified with a NATURAL join, which is not
**	supported.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Reformulate the query so it conforms to the correct syntax.
*/
E_PS03A2_MISSING_JQUAL:SS42000_SYN_OR_ACCESS_ERR
"Join_search qualification is mandatory for joins."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A qualification was not supplied, and the join type is not NATURAL.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Reformulate the query so it conforms to the correct syntax.
*/
E_PS03A3_COL_NOT_IN_JREL:SS42000_SYN_OR_ACCESS_ERR
"Line %0d: Column '%1c' appearing in join_search condition is
\nnot an attribute of any relation involved in the join."
/*%
** Parameters:
**	%0d - line number
**	%1c - column name
**
** Explanation:
**	A join_search condition referenced a column from a relation not
**	involved in the join.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS03A4_NOT_A_JREL:SS42000_SYN_OR_ACCESS_ERR
"Line %0d: Relation '%1c' appearing in join_search condition is not
\ninvolved in the join."
/*%
** Parameters:
**	%0d - line number
**	%1c - relation name
**
** Explanation:
**	A join_search condition referenced a relation not involved in the join.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS03A5_MULT_COMMON_COLS:SS42000_SYN_OR_ACCESS_ERR
"Line %0d: Column '%1c' is a common column that appears more than 
\nonce on left or right side of NATURAL join."
/*%
** Parameters:
**	%0d - line number
**	%1c - column name
**
** Explanation:
**	If like-named columns appear on the left and right side of a natural
**	join, there may only be one such column on each side. Two or more 
**	columns of the same name may not appear on one side of the join.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS03A6_AGGR_IN_JOIN_COND:SS50008_UNSUPPORTED_STMT
"Use of aggregates inside the join_search condition is illegal."
/*%
** Parameters:
**	None.
**
** Explanation:
**	It is illegal to have aggregates in the join_search condition of outer
**	joins. This restriction only applies to use of aggregates in the
**	join_search condition outside of subselects (which are presently also
**	disallowed in the join_search condition, but this may change.)
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS03A7_QUOTED_WITH_CLAUSE:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE statement WITH-clause options may not be quoted except for
\nthe NODE, DATABASE, DBMS and TABLE options."
/*%
** Parameters:
**	None.
**
** Explanation:
**	In general, it is illegal to quote the right-hand side of a WITH-clause
**	item.  WITH-clause items may be of the form:
**
**	        <name> = <name>
**	        <name> = <integer>
**	        <name>
**	        <name> = (<name> {,<name>})
**
**	The exception to this rule applies to the NODE, DATABASE, DBMS, and
**	TABLE options used in distributed servers on the "CREATE TABLE" and 
**	other distributed-only statements.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the syntax and reenter.
*/
W_PS03A8_OBSOLETE_PRIV:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER USER: The '%0c' PRIVILEGE is obsolete and should be replaced
by current syntax. 
\nSupport for this privilege will be discontinued in a later release"
/*%
** Parameters:
**	%0c - privilege name
**
** Explanation:
**	The "CREATE USER"/"ALTER USER" statement was issued with a PRIVILEGES
**	clause which specified an obsolete privilege. This privilege has
** 	been replaced by different syntax and should not be used. A future
**	release will remove support for this privilege.
**
** System Status:
**	The message is a warning only; the current statement continues.
**
** Recommendation:
**	Correct the syntax for future use.
*/
E_PS03A9_CONSTRAINT_COL_EXCEEDED:SS42000_SYN_OR_ACCESS_ERR
"Too many columns specified in table constraint.  Reduce the number of columns."
/*%
** Explanation:
**    There is a maximum number of columns which can be specified in
**    a foreign key.  This number is defined in RDF_TUPLE_RETRIEVE.
**
** System Status:
**    The parsing comes to a stop.
**
** Recommendation:
**    Reduce the number of columns specified in the foreign key.
*/
E_PS03AA_AGGR_IN_ORDER_BY:SS50008_UNSUPPORTED_STMT
"Use of aggregates inside the order by clause is illegal."
/*%
** Parameters:
**	None.
**
** Explanation:
**	It is illegal to have aggregates in the order by clause.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS03AB_AGGR_IN_GROUP_BY:SS50008_UNSUPPORTED_STMT
"Use of aggregates inside the group by clause is illegal."
/*%
** Parameters:
**	None.
**
** Explanation:
**	It is illegal to have aggregates in the group by clause.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS03AC_MISSING_COLUMN:SS42000_SYN_OR_ACCESS_ERR
"Line %0d: Column '%1c' appearing in a named columns join does not
\nappear in both tables of the join."
/*%
** Parameters:
**	%0d - line number
**	%1c - column name
**
** Explanation:
**	All columns in the list of a named columns join must appear in both
**	tables of the join.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS03AD_CASE_NOT_SUPPORTED_WITH_SUBSELECT:SS50008_UNSUPPORTED_STMT
"The CASE construct is not supported with a subselect."
/*%
** Parameters:
**    None.
**
** Explanation:
**    The user entered a statement containing a form of the
**    CASE construct that is not currently supported.
**
** System Status:
**    The statement is aborted.
**
** Recommendation:
**    Recast the statement without using the unsupported syntax
**    and reenter.
*/
E_PS03AE_COLREF_IN_WRONG_UNION_PART:SS5000A_QUERY_ERROR
"Line %0d: Column '%1c.%2c' not from first segment of UNION."
E_PS03AF_SUBSEL_ORDERBY_WITH_UNION:SS50008_UNSUPPORTED_STMT
"The sub-select is not supported in the ORDER BY clause of a UNION"
/*%
** Parameters:
**    None.
**
** Explanation:
**    The user entered a statement containing a sub-select
**    within the ORDER BY clause of a UNION.
**
** System Status:
**    The statement is aborted.
**
** Recommendation:
**    Recast the statement without using the unsupported syntax
**    and reenter.
*/
E_PS03B0_ILLEGAL_ROW_LOCKING:SS5000A_QUERY_ERROR
"SET LOCKMODE: cannot set LEVEL to %0c for table '%1c'
\nsince it is a table with 2k page size." 
/*%
** Parameters:
**      %0c - attempted lock level, "row" or "mvcc"
**      %1c - table name
**
** Explanation:
**      Row and MVCC locking are not allowed for tables that use the 2k
**      page size.
**
** System Status:
**      The program continues; the current statement is aborted.
**
** Recommendation:
**      Change the page size of the table to greater than 2k to
**      use row or MVCC locking.  Use sysmod -page_size=nnnn to change
**	catalog page sizes.
*/
E_PS03B1_NULL_RCB:SS5000B_INTERNAL_ERROR
"A null RCB has been passed to function pst_cpdata."
E_PS03B2_INVALID_CPLENGTH:SS5000B_INTERNAL_ERROR
"A zero copy field length was found in the attribute array when
processing an insert-to-copy optimization."
E_PS03B3_NULL_QUERY_TREE:SS5000B_INTERNAL_ERROR
"A NULL query tree was passed to the function pst_cpdata.
Query processing cannot continue."
E_PS0401_CUR_NOT_FOUND:SS42504_UNKNOWN_CURSOR
"no such cursor in the parser facility"
E_PS0402_NONUPDATABLE_COLUMN:SS42000_SYN_OR_ACCESS_ERR
"UPDATE: column '%0c' of a view '%1c' may not be updated
\nusing cursor '%2c' because it is based on an expression."
/*%
** Parameters:
**	%0c - column name
**	%1c - view name
**	%2c - cursor name
**
** Explanation:
**	The user has defined a cursor on a view without specifying "FOR UPDATE"
**	or "FOR READONLY" so they can update all columns of the view which are
**	updatable and for which the user possesses UPDATE privilege.  In this
**	query, the user attempted to update a column which is not updatable,
**	because it is based on an expression (as opposed to a simple column).
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0403_CURS_UPDT_NO_PRIV:SS42503_INSUF_PRIV
"UPDATE: column '%0c' of '%1c' may not be updated
\nusing cursor '%2c' because you lack privileges required
\nto update it."
/*%
** Parameters:
**	%0c - column name
**	%1c - table or view name
**	%2c - cursor name
**
** Explanation:
**	The user has defined a cursor on a table or view, but they lack
**	privileges required to update some of its attributes.
**
**	We no longer prevent a user from explicitly or implicitly specifying a
**	"FOR UPDATE" clause containing the name of a column which they may not
**	UPDATE (this behavior is compliant with ANSI SQL92), but we still
**	prevent them from updating this column.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0404_REP_CURS_NOT_FOR_RDONLY:SS42000_SYN_OR_ACCESS_ERR
"OPEN CURSOR: cursor '%0c' is a repeat cursor
\nand may only be opened FOR READONLY."
/*
** Parameters:
**	%0c	name of a cursor
**	
** Explanation:
**	User attempted to open a repeat cursor without specifying FOR READONLY
**	which is illegal since at this stage we support only read-only shared
**	repeat cursors.
**	
** System Status:
**	Program continues; current statement is aborted.
**	
** User Action:
**	Open the cursor FOR READONLY or, if it is necessary to perform
**	UPDATEs/DELETEs using this cursor, remove REPEAT(ED) from its
**	declaration
*/
E_PS0420_DROPOBJ_NOT_OBJ_OWNER:SS42503_INSUF_PRIV
"%0c: You may not drop an object owned by '%1c'."
/*%
** Parameters:
**	%0c - user name
**	%1c - object owner as specified by the user
**
** Explanation:
**	The user attempted to drop an object owned by another user through
**	the use of the owner.table construct, and the owner is different
**	from the current user.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0421_CRTOBJ_NOT_OBJ_OWNER:SS42503_INSUF_PRIV
"%0c: You may not create an object owned by '%1c'."
/*%
** Parameters:
**	%0c - user name
**	%1c - object owner as specified by the user
**
** Explanation:
**	The user attempted to create an object owned by another user through
**	the use of the owner.table construct, and the owner is different from
**	the current user.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0422_GRANTTAB_NOT_FOUND:SS42500_TBL_NOT_FOUND
"GRANT: failed to obtain table description
\nof an underlying table (%0d,%1d)
\nfor view '%2c' owned by '%3c'
\non which a permit is being granted.
\nPlease contact your Ingres Corporation
\ntechnical representative."
/*%
** Parameters:
**	%0d,%1d - table ID
**	%2c     - view name
**	%3c     - view owner
**
** Explanation:
**	This message is not expected unless you are running a database
**	conversion program.  It is likely to signify a database consistency
**	problem.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS0423_CRTIND_INSUF_PRIV:SS42503_INSUF_PRIV
"Insufficient privilege to CREATE INDEX on catalog '%0c'."
/*%
** Parameters:
**	%0c - catalog name
**
** Explanation:
**	Either the user attempted to create an index on a core catalog or the
**	user didn't have catalog update privileges and tried to create an index
**	on a catalog which is not an extended catalog.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0424_ILLEGAL_TBL_REF:SS42503_INSUF_PRIV
"%0c: You may not run a DDL query against
\n table '%1c' owned by '%2c'."
/*
** Parameters:
**	%0c - offending query
**	%1c - table name
**	%2c - object owner as specified by the user
**
** Explanation:
**	The user attempted to execute a DDL query explicitly referencing
**	a table which may not be referenced by this user.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0425_ILLEGAL_DBP_REF:SS42503_INSUF_PRIV
"%0c: You may not run this query against
\n database procedure '%1c' owned by '%2c'."
/*%
** Parameters:
**	%0c - offending query
**	%1c - database procedure name
**	%2c - database procedure owner as specified by the user
**
** Explanation:
**	The user attempted to execute a query explicitly referencing a database
**	procedure which may not be referenced by this user.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0426_ILLEGAL_EVENT_REF:SS42503_INSUF_PRIV
"%0c: You may not execute this statement against
\n dbevent '%1c' owned by '%2c'."
/*%
** Parameters:
**	%0c - offending query
**	%1c - dbevent name
**	%2c - dbevent owner as specified by the user
**
** Explanation:
**	The user attempted to execute a query explicitly referencing a dbevent
**	which may not be referenced by this user.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0427_ALTOBJ_NOT_OBJ_OWNER:SS42503_INSUF_PRIV
"%0c: You may not alter an object owned by '%1c'."
/*%
** Parameters:
**	%0c - user name
**	%1c - object owner as specified by the user
**
** Explanation:
**	The user attempted to alter an object owned by another user through
**	the use of the owner.table construct, and the owner is different
**	from the current user.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS042A_CANT_GRANT_VIEW_PERM:SS42503_INSUF_PRIV
"You may not CREATE or GRANT permit on view '%0c'.
\nYou do not own one or more of the underlying tables."
/*%
** Parameters:
**	%0c - view name as specified by the user
**
** Explanation:
**	The user attempted to GRANT a permit on a view, but at least one of the
**	underlying relations is not owned by them.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS042C_CANT_GRANT_DBP_PERM:SS42503_INSUF_PRIV
"GRANT: You may not grant permit on database procedure '%0c'.
\nSome object used in definition of '%0c' does not exist or
\nyou are not allowed to grant required privilege(s) on some object
\nused in '%0c'."
/*%
** Parameters:
**	%0c - database procedure name as specified by the user
**
** Explanation:
**	The user attempted to GRANT a permit on a database procedure, but it was
**	determined that the user may not grant permits on it because some object
**	(i.e., table, view, database procedure, or dbevent) used in its
**	definition did not exist; or, the user may not grant required
**	privilege(s) on some object used in its definition.
**
** System Status:
**	Permit on this database procedure will not be created; other objects
**	(if any) mentioned in the statement will be processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS042E_DROPSYN_NOT_OBJ_OWNER:SS42503_INSUF_PRIV
"You may not drop a synonym owned by '%0c'."
/*%
** Parameters:
**	%0c - synonym owner as specified by the user
**
** Explanation:
**	The user entered "DROP SYNONYM owner.synonym_name...", and the owner
**	is different from the current user.
**
** System Status:
**	Synonym owned by another user will not be dropped.
**
**	If any other synonyms were listed in this statement, they will be
**	processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS042F_UNGRANTABLE_DBPROC:SS42503_INSUF_PRIV
"You may not execute database procedure '%0c' owned by '%1c'.
\nPermits on database procedure '%0c' owned by '%1c' will
\nnot be honored: '%1c' may not grant permit on one or
\nmore tables and/or database procedures used in '%0c'."
/*%
** Parameters:
**	%0c - database procedure name as specified by the user
**	%1c - database procedure owner
**
** Explanation:
**	While trying to recreate a database procedure P1 which is not owned by
**	the current user, it was determined that the permits on P1 shall not
**	be honored.  This can occur if P1 uses at least one table or database
**	procedure which is not owned by the owner of P1.  Since we disallow
**	users to grant access to objects not owned by them, we will similarly
**	disregard any permits which may have been issued on P1.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact the owner of the database procedure or change the query which
**	caused the error to not use the database procedure in question.
*/
I_PS0451_SYN_WAS_SPECIFIED:SS5000F_TEXTUAL_INFO
"%0c: '%1c' owned by '%2c' is a synonym for
\n'%3c' owned by '%4c'."
/*%
** Parameters:
**	%0c - query
**	%0c - synonym name
**	%0c - synonym owner
**	%0c - object for which the synonym is defined
**	%0c - object name
**
** Explanation:
**	This is an informational message which will be displayed in conjunction
**	with various error messages if the object specified by the user was a
**	synonym.
**
** System Status:
**	No change is system status; informational only.
**
** Recommendation:
**	None.
*/
E_PS0452_CANT_DROP_SYN:SS42000_SYN_OR_ACCESS_ERR
"DROP: '%0c' is a synonym.
\nUse 'DROP SYNONYM' to drop a synonym or
\n'DROP [TABLE|VIEW|INDEX]' to drop an object on which a synonym
\nhas been defined."
/*%
** Parameters:
**	%0c - query
**	%1c - synonym name
**
** Explanation:
**	The user entered a "DROP synonym_name".  One must use "DROP SYNONYM"
**	to drop the synonym or "DROP [TABLE|VIEW|INDEX]" to drop the object
**	for which the synonym was defined.
**
** System Status:
**	The synonym will not be dropped.  If there were other objects listed in
**	the DROP statement, they will be processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0453_SYN_ALREADY_EXISTS:SS42502_DUPL_OBJECT
"%0c: object cannot be created.
\nYou already own synonym '%1c'."
/*%
** Parameters:
**	%0c - query
**	%0c - object name as specified by the user
**
** Explanation:
**	The user tried to create a new object, but they already own a
**	synonym by the same name.  This message is intended to be used
**	in conjunction with QUEL queries: CREATE/DEFINE VIEW/RETRIEVE INTO.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	The user can give the new object a different name or use "DROP SYNONYM"
**	(SQL) to destroy an existing synonym.
*/
E_PS0454_CREATE_SYN_ERROR:SS50000_MISC_ERRORS
"CREATE SYNONYM: Failed to create synonym '%0c'.
\nPlease contact system administrator."
/*%
** Parameters:
**	%0c - synonym name
**
** Explanation:
**	A "CREATE SYNONYM" query failed due to some internal error.
**
** System Status:
**	Current statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_PS0455_CRT_SYN_ON_SYN:SS42000_SYN_OR_ACCESS_ERR
"CREATE SYNONYM: '%0c' is a synonym.
\nSynonyms may not be created on other synonyms."
/*%
** Parameters:
**	%0c - synonym name on which user tried to create a synonym
**
** Explanation:
**	The user attempted to create a synonym on another synonym.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Correct the error and continue.
*/
E_PS0456_DROP_SYN_ERROR:SS50000_MISC_ERRORS
"DROP SYNONYM: Failed to drop synonym '%0c'.
\nPlease contact system administrator."
/*%
** Parameters:
**	%0c - synonym name
**
** Explanation:
**	A "DROP SYNONYM" query failed due to some internal error.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_PS0457_DROPSYN_NOT_SYN:SS42000_SYN_OR_ACCESS_ERR
"DROP SYNONYM: '%0c' is a %1c.
\nUse '%2c'."
/*%
** Parameters:
**	%0c - object name as specified by the user
**	%1c - object type
**	%2c - query to be used to drop this object
**
** Explanation:
**	The user entered "DROP SYNONYM obj_name", but "obj_name" does not
**	refer to a synonym.
**
** System Status:
**	If the user specified other synonyms to be dropped, they will be
**	processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0458_ULH_ALIAS_ERROR:SS50003_EXHAUSTED_RESOURCE
"A ULH error occurred due to concurrent activity and/or
\nimproperly chosen RDF startup parameters."
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF called ulh_define_alias(), and ULH returned E_UL0119_ALIAS_MEM.
**	This situation would occur if there is too much concurrent activity
**	and/or if the RDF startup parameters were poorly chosen.
**
**	To correct this problem, one could rerun the query, anticipating
**	that some of the concurrent activity has died down.  However, a more
**	permanent solution would be to stop the server and restart it with a
**	larger maximum number of tables and larger number of synonyms per table
**	which can be specified in iirundbms.com_<node> using:
**
**	    /rdf=(max_tbls=N1, tbl_synonyms=N2).
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Modify iirundbms.com_<node> RDF startup parameters, specifying larger
**	numbers, then restart the server and try again.
*/
E_PS0459_ILLEGAL_II_NAME:SS42507_RESERVED_IDENTIFIER
"%0c: Invalid %1c name '%2c'.
\n Names starting with '%3c' are reserved for system use."
/*%
** Parameters:
**	%0c - command name (CREATE SYNONYM or DBEVENT)
**	%1c - object type (synonym or dbevent)
**	%2c - invalid name
**	%3c - the reserved system prefix
**
** Explanation:
**	All objects beginning with the system prefix are reserved.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Use a different name, and retry the statement.
*/
E_PS045F_WRONG_CASE:SS5000A_QUERY_ERROR
"%0c: Invalid name '%1c'.
\n Names starting with '%2c' must be expressible as regular identifiers,
\nand cannot have mixed case in their names."
/*%
** Parameters:
**	%0c - command name (CREATE TABLE or SYNONYM)
**	%1c - invalid name
**	%2c - the reserved system prefix
**
** Explanation:
**	All objects beginning with the system prefix are reserved.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Use a name that is all upper-case (for FIPS database)
**	or all lower-case (for lower-mixed database).
*/
E_PS0460_DUP_UPD_COL:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: duplicate column '%0c' in UPDATE column list."
/*%
** Parameters:
**	%0c - column name
**
** Explanation:
**	In a "CREATE RULE" statement, the same column can be listed in the
**	UPDATE(col_list) only once.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the statement to list the column only once and reenter.
*/
E_PS0461_BAD_COL_REF_QUAL:SS42000_SYN_OR_ACCESS_ERR
"A <column reference> contained a <qualifier> '%0c'
\nwhich did not correspond to any of the <table reference>s
\nwithin whose scope the <column reference> appeared."
/*%
** Parameters:
**	%0c - <qualifier> found in a <column reference>;
**	      may be of format <table name>, <correlation name>, or
**	      <schema name>.<table name>
**
** Explanation:
**	A <qualifier> found in a <column reference> did not identify any of the
**	<table reference>s within whose scope the <column reference> appeared.
**	
**	A <column reference> found in a <target list> or <group by clause> of a
**	query may only reference tables appearing in the <from list> (or
**	equivalent) at the same scope.
**
**	A <column reference> found outside of a <target list> or <group by
**	clause> of a query may reference tables appearing in the <from list>
**	(or equivalent) at the same scope or in outer subselects.
**
**	A <column reference> in CHECK constraint found outside of subselects,
**	must reference the table on which CHECK constraint is being defined.
**	
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the query and reenter.
*/
E_PS0470_REFERENCES_ON_VIEW:SS42000_SYN_OR_ACCESS_ERR
"GRANT/REVOKE: '%0c' is a view.
\n'REFERENCES' privilege on views may not be granted or revoked."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to grant or revoke REFERENCES privilege on a view, 
**	which is illegal.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the query and reenter.  Either REFERENCES must be removed from
**	the privilege list or the offending view must be removed from the
**	object list.
*/
E_PS0471_CHECK_CONSTRAINT:SS42000_SYN_OR_ACCESS_ERR
"%0c: aggregates and subselects cannot
be used inside a CHECK constraint."
/*%
** Parameters:
**	%0c - executed statement ("CREATE TABLE" or "ALTER TABLE")
**
** Explanation:
**	A subselect or an aggregate function was encountered inside a CHECK
**	constraint of a "CREATE TABLE" statement.  These constructs are
**	explicitly disallowed inside a CHECK constraint.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0472_COL_CHECK_CONSTRAINT:SS42000_SYN_OR_ACCESS_ERR
"%0c: CHECK constraint for column '%1c' of table
'%2c' cannot reference any other columns.
\nUse a table-level constraint to check more than one column
at the same time."
/*%
** Parameters:
**	%0c - executed statement ("CREATE TABLE" or "ALTER TABLE")
**	%1c - column name
**	%2c - table name
**	
** Explanation:
**	A column CHECK constraint cannot reference any other columns.
**	A reference to a column name different from the one for which
**	the CHECK constraint is being defined has been found.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0473_TBL_CHECK_CONSTRAINT:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c constraint for table '%2c' references
nonexistent attribute '%3c'."
/*%
** Parameters:
**	%0c - executed statement ("CREATE TABLE" or "ALTER TABLE")
**	%1c - constraint type (CHECK, UNIQUE, or REFERENTIAL)
**	%2c - table name
**	%3c - nonexistent attribute name
**
** Explanation:
**	A column used in a constraint definition does not exist in the
**	table being created or altered.
**
**	Note that, although CHECK constraints allow a boolean expression,
**	in Entry Level SQL92 (and r6.5), they cannot access attributes of
**	any other table.  This will be relaxed later for Intermediate SQL92.
**
**	NOTE: UNIQUE also includes PRIMARY KEY constraints, so the
**	      constraint could be a PRIMARY KEY constraint.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0474_CHECK_CONSTRAINT:SS42000_SYN_OR_ACCESS_ERR
"%0c: CHECK constraint for table '%1c' may
\nnot reference attributes of table '%2c'."
/*%
** Parameters:
**	%0c - executed statement ("CREATE TABLE" or "ALTER TABLE")
**	%1c - table name
**	%2c - table attribute
**
** Explanation:
**	A CHECK constraint may reference only the attributes of the table
**	being created.  The user attempted to use an attribute of a table
**	other than the one being created inside a CHECK constraint.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0475_CONSTRAINT_NOT_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"%0c: Constraints are not allowed on columns or parameters
in temporary tables, CREATE PROCEDURE, REGISTER TABLE or
\nALTER TABLE ... ALTER COLUMN statements."
/*%
** Parameters:
**	%0c - executed statement (one of "CREATE PROCEDURE",
**	      "DECLARE GLOBAL TEMPORARY TABLE", or "REGISTER TABLE")
**
** Explanation:
**	SQL92-style constraints are only allowed in the "CREATE TABLE"
**	or "ALTER TABLE" statements.  They cannot be used for temporary
**	tables, database procedures (parameters or local variables), or
**	"REGISTER TABLE" or ALTER TABLE ..ALTER COLUMN statements.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the constraint clauses, and reenter the statement.
*/
E_PS0476_USER_DEFAULT_NOTALLOWED:SS42000_SYN_OR_ACCESS_ERR
"%0c: User-defined defaults are not allowed
in temporary tables, CREATE PROCEDURE, or REGISTER TABLE.
\nTry WITH DEFAULT instead."
/*%
** Parameters:
**	%0c - executed statement executed (one of:
**	      "DECLARE GLOBAL TEMPORARY TABLE" or "REGISTER TABLE")
** Explanation:
**	User-defined defaults (i.e., [WITH] DEFAULT <values>) are only allowed
**	in "CREATE TABLE", "ALTER TABLE", and "CREATE PROCEDURE" statements.
**	They cannot be used for temporary tables, or "REGISTER TABLE"
**	statements.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the user-defined default clauses, and reenter the statement.
*/
E_PS0477_BAD_DEFAULTSYNTAX_SQL92:SS42000_SYN_OR_ACCESS_ERR
"%0c: Invalid DEFAULT syntax.  
\n\tTry either 'DEFAULT <value>' or 'WITH DEFAULT'."
/*%
** Parameters:
**	%0c - executed statement (one of "CREATE TABLE" or "CREATE PROCEDURE")
**
** Explanation:
**	The user specified DEFAULT without the WITH keyword and without a
**	value.  The WITH keyword is only optional if a value is specified.
**	Thus, one or the other must be specified.
**
**	Valid default syntax is one of:
**	       WITH DEFAULT
**	       [WITH] DEFAULT <value>
**	       [WITH] DEFAULT USER
**	       [WITH] DEFAULT NULL
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error, and reenter the statement.
*/
E_PS0478_BAD_DEFAULT_SYNTAX:SS42000_SYN_OR_ACCESS_ERR
"%0c: Invalid DEFAULT syntax.  Try WITH DEFAULT."
/*%
** Parameters:
**	%0c - executed statement (such as "REGISTER TABLE",
**	      "DECLARE GLOBAL TEMPORARY TABLE", or "QUEL CREATE")
**
** Explanation:
**	The user specified DEFAULT without the WITH keyword and without a
**	value.  The WITH keyword is only optional if a value is specified.
**	Thus, one or the other must be specified.
**
**	Valid default syntax is one of
**	       WITH DEFAULT
**	       [WITH] DEFAULT <value>
**	       [WITH] DEFAULT USER
**	       [WITH] DEFAULT NULL
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error, and reenter the statement.
*/
E_PS0479_DUP_COL_QUAL:SS42000_SYN_OR_ACCESS_ERR
"%0c: A column qualification was listed more than once for column '%1c'.
\nThe NULL, DEFAULT, and SYSTEM_MAINTAINED clauses can each only
be used once per column."
/*%
** Parameters:
**	%0c - executed statement executed (one of "CREATE TABLE",
**	      "CREATE PROCEDURE", "DECLARE GLOBAL TEMPORARY TABLE",
**	      or "REGISTER TABLE")
**	%1c - column name (parameter name if "CREATE PROCEDURE" statement)
**
** Explanation:
**	Since column qualifications can be in any order now (as of r6.5),
**	the syntax rules don't automatically catch conflicts.  This error
**	was explicitly added to catch them instead. It occurs when a user
**	specifies a statement such as:
**
**	       C int NOT NULL UNIQUE NOT NULL
**	or
**	       C int WITH DEFAULT WITH NULL WITH DEFAULT 100
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the duplicate clause, and reenter the statement.
*/
E_PS047A_CONFLICT_COL_QUAL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Conflicting column qualifications were specified for column '%1c'."
/*%
** Parameters:
**	%0c - executed statement (one of "CREATE TABLE", "CREATE PROCEDURE",
**	      "DECLARE GLOBAL TEMPORARY TABLE", or "REGISTER TABLE")
**	%1c - column name (parameter name if "CREATE PROCEDURE" statement)
**
** Explanation:
**	Since column qualifications can be in any order now (as of r6.5),
**	the syntax rules don't automatically catch conflicts.  This error
**	was explicitly added to catch them instead. It occurs when a user
**	specifies a statement such as:
**
**	       C int NOT NULL UNIQUE WITH NULL
**	or
**	       C int WITH DEFAULT WITH NULL NOT DEFAULT
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the conflicting clause, and reenter the statement.
*/
E_PS047B_RESERVED_IDENT:SS42506_INVALID_IDENTIFIER
"%0c: Invalid object name '%1c'.
\nCannot create an object name beginning with '$'.  Such object names are
reserved for internally generated names and other internal use."
/*%
** Parameters:
**	%0c - command name
**	%1c - object name
**
** Explanation:
**	The DBMS must generate names for CONSTRAINTS and other objects.
**	To make sure that these names do not conflict with user names,
**	we start these names with a dollar sign ("$") and reserve all
**	names starting with a dollar sign for internal use.
**	Thus, users cannot create an object name starting with a dollar
**	sign, even within delimited identifiers.
**
**	Users can, however, use names starting with "$" in non-create
**	commands, such as "DROP INDEX" or "ALTER TABLE DROP CONSTRAINT".
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error, and reenter the statement.
*/
E_PS047C_DUP_CONS_NAME:SS42000_SYN_OR_ACCESS_ERR
"%0c: Constraint name '%1c' already exists or has been
specified more than once."
/*%
** Parameters:
**	%0c - executed statement (either "CREATE TABLE" or "ALTER TABLE")
**	%1c - constraint name
**
** Explanation:
**	Constraint names must be unique within a user's entire schema,
**	not just within a single table.
**
**	This error is returned if a constraint of the same name already
**	exists in the database, or if the user specified the same name for
**	two constraints in a single "CREATE TABLE" statement.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the duplicate clause, and reenter the statement.
*/
E_PS047D_2UNIQUE_CONS_CONFLICT:SS42000_SYN_OR_ACCESS_ERR
"%0c: UNIQUE constraints '%1c' and '%2c' have conflicting sets of columns.
\nThe set of columns of one UNIQUE constraint cannot be the same as
the set of columns of another UNIQUE constraint on the same table."
/*%
** Parameters:
**	%0c - statement executed (either "CREATE TABLE" or "ALTER TABLE")
**	%1c - first UNIQUE constraint name
**	%2c - second UNIQUE constraint name
**
** Explanation:
**	The set of columns in a UNIQUE constraint must be distinct from the
**	set of columns in every other UNIQUE constraint. That is, the set
**	of columns in one constraint cannot be exactly the same as the set of
**	columns in the other constraint.
**
**	NOTE: UNIQUE also includes PRIMARY KEY constraints, so one of the
**	      constraints could be a PRIMARY KEY constraint.
**
**	This error is returned if both of the constraints have names.
**	If neither or only one has a name, following errors are used.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove one of the constraint clauses or change the set of columns,
**	and reenter the statement.
*/
E_PS047E_1UNIQUE_CONS_CONFLICT:SS42000_SYN_OR_ACCESS_ERR
"%0c: A UNIQUE constraint has a set of columns that conflicts with UNIQUE
constraint '%1c'.
\nThe set of columns of one UNIQUE constraint cannot be the same as
the set of columns of another UNIQUE constraint on the same table."
/*%
** Parameters:
**	%0c - executed statement (either "CREATE TABLE" or "ALTER TABLE")
**	%1c - UNIQUE constraint name
**
** Explanation:
**	The set of columns in a UNIQUE constraint must be distinct from the
**	set of columns in every other UNIQUE constraint. That is, the set of
**	columns in one constraint cannot be exactly the same as the set of
**	columns in the other constraint.
**
**	NOTE: UNIQUE also includes PRIMARY KEY constraints, so one of the
**	      constraints could be a PRIMARY KEY constraint.
**
**	This error is returned if only one of the constraints has a name.
**	If neither or both have a name, following errors are used.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove one of the constraint clauses or change the set of columns,
**	and reenter the statement.
*/
E_PS047F_0UNIQUE_CONS_CONFLICT:SS42000_SYN_OR_ACCESS_ERR
"%0c: UNIQUE constraints have conflicting sets of columns.
\nThe set of columns of one UNIQUE constraint cannot be the same as
the set of columns of another UNIQUE constraint on the same table."
/*%
** Parameters:
**	%0c - executed statement (either "CREATE TABLE" or "ALTER TABLE")
**
** Explanation:
**	The set of columns in a UNIQUE constraint must be distinct from the
**	set of columns in every other UNIQUE constraint. That is, the set of
**	columns in one constraint cannot be exactly the same as the set of
**	columns in the other constraint.
**
**	NOTE: UNIQUE also includes PRIMARY KEY constraints, so one of the
**	      constraints could be a PRIMARY KEY constraint.
**
**	This error is returned if neither of the constraints has a name.
**	If one or both have a name, above errors are used.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove one of the constraint clauses or change the set of columns,
**	and reenter the statement.
*/
E_PS0480_UNIQUE_NOT_NULL:SS42000_SYN_OR_ACCESS_ERR
"%0c: A column in a UNIQUE constraint has been
\ndefined as WITH NULL (on table '%1c').
\nAll columns in a UNIQUE constraint MUST be created as NOT NULL."
/*%
** Parameters:
**	%0c - executed statement (either "CREATE TABLE" or "ALTER TABLE")
**	%1c - table name
**
** Explanation:
**	For Entry Level SQL92, all columns in a UNIQUE or PRIMARY KEY
**	constraint must specify "NOT NULL" in the column definition.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	If a "CREATE TABLE" statement was used, add a "NOT NULL" specification
**	to the column definition of all columns in the UNIQUE constraint, and
**	reenter the statement.
**
**	If an "ALTER TABLE" statement was used, it cannot use "WITH NULL"
**	columns in a UNIQUE constraint; the table must be dropped and
**	recreated with those columns "NOT NULL".
*/
E_PS0481_CONS_DUP_COL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Duplicate column name '%1c' in %2c constraint."
/*%
** Parameters:
**	%0c - executed statement ("CREATE TABLE" or "ALTER TABLE")
**	%1c - column name
**	%2c - constraint type (CHECK, UNIQUE, or REFERENTIAL)
**
** Explanation:
**	A column name was listed twice in the same constraint.  In a
**	REFERENTIAL constraint, the duplicate could be in either the
**	referencing column list or the referenced column list.
**
**	NOTE: UNIQUE also includes PRIMARY KEY constraints, so the
**	      constraint could be a PRIMARY KEY constraint.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the duplicate column name, and reenter the statement.
*/
E_PS0482_2CONS_DUP_COL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Duplicate column name '%1c' in %2c constraint '%3c'."
/*%
** Parameters:
**	%0c - executed statement (either "CREATE TABLE" or "ALTER TABLE")
**	%1c - column name
**	%2c - constraint type (CHECK,UNIQUE,REFERENTIAL)
**	%3c - constraint name
**
** Explanation:
**	A column name was listed twice in the same constraint.  In a
**	REFERENTIAL constraint, the duplicate could be in either the
**	referencing column list or the referenced column list.
**
**	NOTE: UNIQUE also includes PRIMARY KEY constraints, so the
**	      constraint could be a PRIMARY KEY constraint.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the duplicate column name, and reenter the statement.
*/
E_PS0483_CONS_NO_COL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Column '%1c' used in %2c constraint \ndoes not exist."
/*%
** Parameters:
**	%0c - executed statement (either "CREATE TABLE" or "ALTER TABLE")
**	%1c - column name
**	%2c - constraint type (CHECK, UNIQUE, or REFERENTIAL)
**
** Explanation:
**	A column name listed in a constraint does not exist. In a REFERENTIAL
**	constraint, the duplicate could be in either the referencing column
**	list or the referenced column list.
**
**	NOTE: UNIQUE also includes PRIMARY KEY constraints, so the
**	      constraint could be a PRIMARY KEY constraint.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the error, and reenter the statement.
*/
E_PS0484_REF_NUM_COL:SS42000_SYN_OR_ACCESS_ERR
"%0c: In table '%1c', the number of referenced columns in the referential
constraint does not match the number of referencing columns."
/*%
** Parameters:
**	%0c - executed statement (either "CREATE TABLE" or "ALTER TABLE")
**	%1c - table name
**
** Explanation:
**	The number of referencing columns has to be the same as the number
**	of referenced columns.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Reenter the statement with the correct list of column names.
*/
E_PS0485_REF_TYPE_RES:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: The datatype of referencing column '%0c' does not
match the datatype of the referenced column '%1c'."
/*%
** Parameters:
**	%0c - referencing column name
**	%1c - referenced column name
**
** Explanation:
**	The datatype of each referencing column must match the datatype of
**	its corresponding referenced column.
**
**	Since we have defined "match" to be the same as "joinable," and
**	the DBMS can join almost any type to any other type, most types will
**	match.  The main ones that don't are:
**
**	             date and money
**	             date and float
**	             date and integer
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	The user can reenter the statement with a different list of column
**	names, or they may have to drop and redefine the tables.
*/
E_PS0486_INVALID_CONS_TYPE:SS5000B_INTERNAL_ERROR
"PSF internal error: an error occurred while processing constraints.
\nFound an invalid constraint type."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error means an unexpected constraint type was found in
**	internal data structures.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Please report this error to Ingres Corporation Technical Support. 
**	Save the error log file for analysis by Technical Support personnel.
*/
E_PS0487_INVALID_BITMAP:SS5000B_INTERNAL_ERROR
"PSF internal error: an error occurred while processing constraints.
\nFound an empty column bitmap."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error means an empty column bitmap was found in
**	internal data structures.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Please report this error to Ingres Corporation Technical Support. 
**	Save the error log file for analysis by Technical Support personnel.
*/
E_PS0488_DDB_CONSTRAINT_NOT_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"%0c: Table constraints are not allowed in distributed server."
/*%
** Parameters:
**	%0c - executed statement
**
** Explanation:
**	SQL92-style constraints are not allowed in distributed servers.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the constraint clauses, and reenter the statement.
*/
E_PS0489_DDB_USER_DEFAULT_NOT_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"%0c: User-defined defaults are not allowed in a distributed server.
\nTry WITH DEFAULT instead."
/*%
** Parameters:
**	%0c - executed statement
**
** Explanation:
**	User-defined defaults (i.e., [WITH] DEFAULT <values>)
**	are not allowed in Distributed servers.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the default clauses, and reenter the statement.
*/
E_PS048A_NO_SUCH_CONSTRAINT:SS42505_OBJ_NOT_FOUND
"ALTER TABLE DROP CONSTRAINT: constraint '%0c' does not exist
\nin schema '%1c'."
/*%
** Parameters:
**	%0c - constraint name specified by the user
**	%1c - schema name
**
** Explanation:
**	A constraint with a name specified by the user could not be found in the
**	schema to which the table named in the "ALTER TABLE" statement belongs.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS048B_NULDEF_SPEC_IN_DBPPARM:SS42000_SYN_OR_ACCESS_ERR
"CREATE PROCEDURE: Invalid nullability/defaultability specification
\nwas supplied for a database procedure parameter '%0c'.
\nValid specifications are:
\n\tWITH NULL\t(default if no specification is provided)
\n\tNOT NULL
\n\tNOT NULL WITH DEFAULT
\n\tNOT NULL NOT DEFAULT."
/*%
** Parameters:
**	%0c - name of a database procedure parameter
**
** Explanation:
**	User provided an invalid nullability/defaultability specification for
**	a database procedure parameter.  The following specifications are 
**	allowed:
**
**		WITH NULL (default if no specification is provided)
**		NOT NULL
**		NOT NULL WITH DEFAULT
**		NOT NULL NOT DEFAULT
**
** System Status:
**      The current statement is aborted; the program continues.
**
** Recommendation:
**      Correct the statement and reenter.
*/
E_PS048C_NULDEF_SPEC_IN_DBPVAR:SS42000_SYN_OR_ACCESS_ERR
"CREATE PROCEDURE: Invalid nullability/defaultability specification
\nwas supplied for a database procedure variable '%0c'.
\nValid specifications are:
\n\tWITH NULL\t(default if no specification is provided)
\n\tNOT NULL
\n\tNOT NULL WITH DEFAULT
\n\tNOT NULL NOT DEFAULT."
/*%
** Parameters:
**	%0c - name of a database procedure variable
**
** Explanation:
**	User provided an invalid nullability/defaultability specification for
**	a database procedure variable.  The following specifications are 
**	allowed:
**
**		WITH NULL (default if no specification is provided)
**		NOT NULL
**		NOT NULL WITH DEFAULT
**		NOT NULL NOT DEFAULT
**
** System Status:
**      The current statement is aborted; the program continues.
**
** Recommendation:
**      Correct the statement and reenter.
*/
E_PS048D_CONS_BADINDEX:SS42000_SYN_OR_ACCESS_ERR
"Invalid constraint specification. An attempt was made to use an
\nexisting index or share an index between several constraints
\nand the index was not suitable ('%0c')."
/*%
** Parameters:
**	%0c - name of an index
**
** Explanation:
**	A constraint definition explicitly specified the name of an 
**	existing index, several constraints in the same table definition
**	specified the same index name or the constraint specified the
**	WITH INDEX = BASE TABLE STRUCTURE option, and the column lists 
**	of the constraints and the key structure of the corresponding 
**	index were incompatible. See documentation for more details. 
**
** System Status:
**      The current statement is aborted; the program continues.
**
** Recommendation:
**      Correct the statement and reenter.
*/
E_PS048E_CONS_NOINDEX:SS42000_SYN_OR_ACCESS_ERR
"Invalid constraint specification. The NO INDEX option was
\nspecified with a UNIQUE or PRIMARY KEY constraint. It is
\nonly permissible with referential constraints."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The definition of a UNIQUE or PRIMARY KEY constraint included
**	the WITH NO INDEX option. It is only permitted on referential
**	constraints.
**
** System Status:
**      The current statement is aborted; the program continues.
**
** Recommendation:
**      Correct the statement and reenter.
*/
E_PS048F_CONS_NOBASE:SS42000_SYN_OR_ACCESS_ERR
"Invalid constraint specification. The BASE TABLE STRUCTURE\n
option was specified and the base table has no structure defined."
/*%
** Parameters:
**	none.
**
** Explanation:
**	An ALTER TABLE ... ADD CONSTRAINT statement specified the 
**	WITH INDEX = BASE TABLE STRUCTURE option, but the table has
**	no defined structure. Before this option is permitted, the 
**	table must be modified to an appropriate structure on the
**	constrained columns.
**
** System Status:
**      The current statement is aborted; the program continues.
**
** Recommendation:
**      Correct the statement and reenter.
*/
E_PS0490_NO_UNIQUE_CONSTRAINT:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: The referenced columns in table '%0c' do not form
a unique constraint; a foreign key may only reference columns in a
unique or primary key constraint."
/*%
** Parameters:
**	%0c - table referenced in the FOREIGN KEY constraint.
**
** Explanation:
**	The columns specified in the REFERENCED clause of a FOREIGN
**	KEY constraint must form a unique constraint in the REFERENCED
**	table.  (Note that a primary key is a unique constraint). The
**	REFERENCED column list is the second list of columns in the
**	foreign key specification.
**
**	The DBMS looks for a unique constraint on the referenced table
**	containing exactly the list of columns specified in the
**	FOREIGN KEY constraint; if no matching constraint is found,
**	this error is returned.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Find the unique constraint on the referenced table desired
**	for the referential constraint, and specify the columns exactly.
**	If no unique constraint exists on the columns desired, it may be
**	possible to create one.
*/
E_PS0491_NO_PRIMARY_KEY:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: No primary key exists for 
\nreferenced table '%0c'; please specify the referenced 
\ncolumn list for the referential (FOREIGN KEY) constraint." 
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	If the user does not specify a referenced column list in a
**	foreign key constraint, SQL92 says we must look for a primary key
**	on the referenced table to use as the referenced columns. If no
**	primary key is found, this error is returned.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Find the unique constraint on the referenced table desired for the
**	referential constraint, and specify the columns explicitly.  If no
**	primary key constraint exists on the columns desired, it may be
**	possible to create one.
*/
E_PS0492_CHECK_CONS_TYPE_ERROR:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: Datatype mismatch in CHECK constraint on table '%0c'."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	This error occurs if an expression in a CHECK condition involves
**	conflicting datatypes.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the conflicting expression and try again.
*/
E_PS0493_DUPL_PRIMARY_KEY:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: Duplicate primary key on table '%0c';
there cannot be more than one primary key constraint on a table.
\nTry to create it as a unique constraint."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	SQL92 says there can be only one primary key on a table.
**	If the user tries to specify another primary key on a table
**	that already has one, this error is returned.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Remove the primary key constraint, or change it to a plain
**	unique constraint.
*/
E_PS0494_CONS_NAME:SS42000_SYN_OR_ACCESS_ERR
"The constraint name for the above error is: '%0c'."
/*%
** Parameters:
**	%0c - constraint name
**
** Explanation:
**	If a constraint that is being created contains a syntax or
**	other error, and that constraint was given a name by the user,
**	this message will print out the constraint name. This is extra
**	information for the user, not a second error.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	See error message E_PS0493.
*/
E_PS0495_DUPL_CONS_NAME:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: Duplicate constraint name;
\na constraint with the name '%0c' already exists in schema '%1c'."
/*%
** Parameters:
**	%0c - constraint name
**	%01 - schema name
**
** Explanation:
**	SQL92 says that constraint names must be unique within a schema.
**	This means that two tables in the same schema cannot have
**	constraints with the same name.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Choose a different name for the constraint and try again.
*/
E_PS0496_CANT_ALTER_VIEW:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: '%0c' is a view or index; you cannot alter a view or index."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	An "ALTER TABLE" statement can only be used on tables, not views. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Specify a different table name (one that is not a view).
*/
E_PS0497_CONS_IXOPTS:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: base table or index must have PERSISTENCE and/or \n
UNIQUE_SCOPE = STATEMENT to support constraint definition."
/*%
** Parameters:
**	none
**
** Explanation:
**	If an "ALTER TABLE ... ADD CONSTRAINT ..." uses the
**	WITH INDEX = <index name> or BASE TABLE STRUCTURE option,
**	the index or base table structure must have the PERSISTENCE and
**	UNIQUE_SCOPE = STATEMENT (for unique constraints, only) 
**	attributes.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	If the required options are acceptable, they may be added
**	with "MODIFY table/index WITH PERSISTENCE, UNIQUE_SCOPE =
**	STATEMENT", and the constraint definition can be re-executed.
*/
E_PS0498_DEFAULT_USER_SIZE:SS42000_SYN_OR_ACCESS_ERR
"Column '%0c' cannot hold a user function as a default.  
\nThe column is either too small or is not one of the character datatypes.
\nThe column must be at least %1d characters long in order
to use defaults such as USER, CURRENT_USER, SESSION_USER, or SYSTEM_USER."
/*%
** Parameters:
**	%0c - column name
**	%1d - minimum size required (DB_MAXNAME)
**
** Explanation:
**	A column that has a user function as a default must be
**	large enough to hold the maximum-sized user name. The
**	maximum size is DB_MAXNAME.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Try the query again, giving a larger size for the column.
*/
E_PS0499_BAD_DEFAULT_VALUE:SS42000_SYN_OR_ACCESS_ERR
"Default value doesn't match datatype for column '%0c'."
/*%
** Parameters:
**	%0c - column name
**
** Explanation:
**	The default value for a column must be of the same
**	datatype as the column itself.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Try the query again with a default value of the correct type.
*/
E_PS049A_BAD_DEFAULT_SIZE:SS42000_SYN_OR_ACCESS_ERR
"Default value for column '%0c' is out of range or
has invalid length or precision for the datatype."
/*%
** Parameters:
**	%0c - column name
**
** Explanation:
**	SQL92 says the default value for a column must fit into the
**	datatype the column is defined with. For example, "col1 i1 default 500"
**	is not valid because the range of an i1 is -128 to 127.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Try the query again with a default value within the correct
**	range, length, or precision of the datatype.
*/
E_PS049B_BAD_DEFAULT_FUNC:SS5000B_INTERNAL_ERROR
"Bad default value: invalid constant function %0d."
/*%
** Parameters:
**	%0c - column name
**
** Explanation:
**	SQL92 says the default value for a column must fit into the
**	datatype the column is defined with. For example, "col1 i1 default 500"
**	is not valid because the range of an i1 is -128 to 127.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Try the query again with a default value within the correct
**	range, length, or precision of the datatype.
*/
E_PS049C_NO_DEFAULT_FROM_RDF:SS5000B_INTERNAL_ERROR
"RDF could not find a default value for a column in table '%0c'."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	This is an internal error. This error occurs when the default ID for
**	a column in the iiattribute catalog cannot be found in the iidefault
**	catalog.
**	
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS049D_RDF_CANT_READ_DEF:SS5000B_INTERNAL_ERROR
"RDF could not read the default catalog for table '%0c'; there could be a
concurrency problem."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	This is an internal error. This error occurs when the default ID for
**	a column in the iiattribute catalog cannot be found in the iidefault
**	catalog.
**	
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS049E_CANT_EXEC_SYSGEN_PROC:SS5000B_INTERNAL_ERROR
"You cannot execute system-generated procedure '%0c'."
/*%
** Parameters:
**	%0c - system-generated procedure name
**
** Explanation:
**	System-generated procedures can only be executed by system-generated
**	rules.  Users cannot execute them.
**
**	These rules and procedure are used internally to implement complex
**	features such as SQL92 CHECK and REFERENTIAL constraints or
**	WITH CHECK OPTION for views.
**	
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Do not attempt to execute system-generated procedures.
*/
E_PS049F_NO_PERIPH_DEFAULT:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: cannot create a default on column '%0c'
of type '%1c', because it is a large object datatype.\n
Defaults are not allowed on large object datatypes."
/*%
** Parameters:
**	%0c - column name
**	%1c - datatype name
**
** Explanation:
**	Defaults are not allowed on large objects (e.g. blobs).
**	The user must either specify NOT DEFAULT or specify nothing (which
**	gives an implicit NULL default).
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the DEFAULT specification to NOT DEFAULT,
**	or remove it entirely (to get a NULL default).
*/
E_PS04A0_DEFAULT_TOO_LONG:SS42000_SYN_OR_ACCESS_ERR
"Default value for column '%0c' too long.
\n The maximum allowed default size is %1d bytes."
/*%
** Parameters:
**	%0c - column name of column in error
**	%1c - maximum size of a default (1500 bytes in 6.5)
**
** Explanation:
**	The system catalog for storing default values
**	has a fixed maximum size.  A default that is longer than
**	that size cannot be declared, as it cannot be stored.
**
**	Note that character-string defaults must be stored with
**	surrounding single quotes, so that the maximum character
**	default is actually two bytes less than the maximum
**	non-character default (i.e., 1498 bytes in 6.5).
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Pick a default of a shorter length, or leave the default
**	value off the "CREATE TABLE" statement.
*/
E_PS04A1_NO_REF_PRIV:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: You cannot create a REFERENTIAL constraint
\nthat refers to table '%0c' in schema '%1c' because 
\nit does not exist or you lack REFERENCES privilege on 
\none or more of the <referenced columns>."
/*%
** Parameters:
**	%0c - table name
**	%1c - schema name
**
** Explanation:
**	In order to create a referential constraint in which the referenced
**	table (sometimes called the primary key table) is owned by another
**	user, that table must exist and the current user must have REFERENCES 
**	privilege on the columns used in the referential constraint.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Have the owner of the table grant you REFERENCES privilege,
**	if possible; otherwise, remove the referential constraint clause
**	from the statement (if the statement was a "CREATE TABLE").
*/
E_PS04A2_NO_REF_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: '%0c' is a view.
\nYou cannot create a REFERENTIAL constraint that refers to a view."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	REFERENTIAL constraints can only refer to base tables (or synonyms),
**	not to views. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Replace the name of the view with the name of a base table.
*/
E_PS04A3_NO_GRANT:SS42000_SYN_OR_ACCESS_ERR
"GRANT/REVOKE: '%0c' is a system-generated procedure;
\nYou cannot grant or revoke EXECUTE privilege on it."
/*%
** Parameters:
**	%0c - procedure name
**
** Explanation:
**	SQL92 constraints and views WITH CHECK OPTION create internal
**	procedures on behalf of the user.  These procedures are only
**	executed when called by RULES, and then all users can execute them.
**	But users cannot GRANT or REVOKE execute on these procedures.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Do not attempt to GRANT or REVOKE privileges on this procedure.
*/
E_PS04A4_INT_ERROR:SS42000_SYN_OR_ACCESS_ERR
"%0c: Internal error processing default for column '%1c';
\nerror converting decimal to float."
/*%
** Parameters:
**	%0c - statement name
**	%1c - column name
**
** Explanation:
**	A DECIMAL default value is converted to a FLOAT to check
**	whether it is a zero default.  If this conversion fails,
**	this error is returned.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Try the query again; look in the error log for other errors that
**	may have caused this error.
*/
E_PS04A5_NO_COLUMNS:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: No columns were defined for table '%0c'.
All tables must contain at least one column."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The user did not specify any columns when defining a table.
**	This used to cause a syntax error.
**	
**	Since we have added ANSI-style constraints, "CREATE TABLE" follows
**	ANSI-style syntax.  This syntax no longer prevents a user from
**	specifying a table definition without any column definitions.
**	So now we have to explicitly check this and return this error,
**	instead of relying on the grammar to automatically return a
**	syntax error. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Include columns in the table definition, and resubmit the query.
*/
E_PS04A6_UDT_NO_LONGTXT:SS42000_SYN_OR_ACCESS_ERR
"%0c: No II_LONGTEXT conversion function exists for datatype '%1c'
(used in column '%2c').
\nIn order to use defaults on a UDT column, the UDT must have
conversion functions to and from II_LONGTEXT." 
/*%
** Parameters:
**	%0c - statement name
**	%1c - user-defined datatype (UDT)
**	%2c - UDT column name
**
** Explanation:
**	In 6.5, defaults are manipulated internally in longtext format,
**	which is in the same format as varchar, but with different semantics.
**	In order to use defaults on a UDT column, there must be a way
**	to convert the UDT value to and from longtext.  Users building a
**	UDT are required to supply longtext conversion functions, but this
**	is not strictly enforced, so it is possible to have a working UDT
**	that does not have a longtext conversion function.
**
**	In order for defaults and UDTs to work together in 6.5, the user
**	must define and supply conversion functions for UDT -> longtext
**	and longtext -> UDT.
**	
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Add required conversion functions to the UDT definition, recompile it
**	and relink it into the server, then retry the command.
*/
E_PS04A7_UDT_NO_GETEMPTY:SS42000_SYN_OR_ACCESS_ERR
"%0c: The 'getempty' function for datatype '%1c' failed
(used in column '%2c').
\nIn order to use WITH DEFAULT on a UDT column, the UDT must have
a working getempty function defined." 
/*%
** Parameters:
**	%0c - statement name
**	%1c - user-defined datatype (UDT)
**	%2c - UDT column name
**
** Explanation:
**	When a column is defined using WITH DEFAULT, the DBMS puts an
**	"empty" value into that column if the user doesn't specify a value
**	on an INSERT statement. In order to use "WITH DEFAULT" on a UDT column,
**	there must be a "getempty" function defined for that UDT, which the
**	DBMS uses to get the value to put into that column for its default.
**
**	This error occurs when either the "getempty" routine for a UDT
**	wasn't found by ADF, or if the routine itself has a coding error.
**	
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Add the required "getempty" function to the UDT definition, recompile
**	it and relink it into the server, then retry the command.
*/
E_PS04A8_UDT_LTXT_CONVERSION:SS42000_SYN_OR_ACCESS_ERR
"%0c: The II_LONGTEXT conversion failed for datatype '%1c'
(used in column '%2c').
\nIn order to use defaults on a UDT column, the UDT must have
working conversion functions to and from II_LONGTEXT." 
/*%
** Parameters:
**	%0c - statement name
**	%1c - user-defined datatype (UDT)
**	%2c - UDT column name
**
** Explanation:
**	In 6.5, defaults are manipulated internally in longtext format,
**	which is in the same format as varchar, but with different semantics.
**	In order to use defaults on a UDT column, there must be a way
**	to convert the UDT value to and from longtext.  Users building a
**	UDT are required to supply longtext conversion functions, but this
**	is not strictly enforced, so it is possible to have a working UDT
**	that does not have a longtext conversion function.
**
**	In order for defaults and UDTs to work together in 6.5, the user
**	must define and supply conversion functions for UDT -> longtext
**	and longtext -> UDT.
**	
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Debug UDT conversion functions, recompile the UDT definition and
**	relink it into the server, then retry the command.
**	
**	If no problems can be found in the conversion functions, contact 
**	Ingres Corporation Technical Support. Save the error log file for
**	analysis by Technical Support personnel.
*/
E_PS04A9_NON_JOUR_REFING_JOUR_DB:SS50000_MISC_ERRORS
"CREATE/ALTER TABLE: You cannot create a REFERENTIAL constraint
\non table '%0c' in schema '%1c' 
\nbecause the database to which it belongs is being journaled,
\nbut the data in the table itself is not being journaled."
/*%
** Parameters:
**	%0c - name of referencing table
**	%1c - schema to which the referencing table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  referential constraints defined 
**	on it, we must prevent user from creating a referential constraint on a 
**	table whose data is not being journaled in a database which is being 
**	journaled.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	If the table exists and has journaling disabled, you need to 
**	SET JOURNALING on that table.  You also need to checkpoint the database
**	to enable journaling on that table.  Only then you may be allowed to
**	define a referential constraint on this table.
** 	If this is a new table, remove NOJOURNALING from its WITH-clause and 
**	reissue the statement.
*/
E_PS04AA_NON_JOUR_REFD_JOUR_DB:SS50000_MISC_ERRORS
"CREATE/ALTER TABLE: You cannot create a REFERENTIAL constraint
\non table '%0c' in schema '%1c'
\nreferencing table '%2c' in schema '%3c' 
\nbecause the database to which the referenced table belongs is 
\nbeing journaled, but the data in the table itself is not being 
\njournaled."
/*%
** Parameters:
**	%0c - name of referencing table
**	%1c - schema to which the referencing table belongs
**	%2c - name of referenced table
**	%3c - schema to which the referenced table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  referential constraints defined 
**	on it, we must prevent user from creating a referential constraint 
**	referencing a table whose data is not being journaled in a database 
**	which is being journaled.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	If the table exists and has journaling disabled, you need to 
**	SET JOURNALING on that table.  You also need to checkpoint the database
**	to enable journaling on that table.  Only then you may be allowed to
**	define a referential constraint on this table.
*/
E_PS04AB_REF_CONS_JOUR_MISMATCH:SS50000_MISC_ERRORS
"CREATE/ALTER TABLE: You cannot create a REFERENTIAL constraint
\non table '%0c' in schema '%1c' 
\nreferencing table '%2c' in schema '%3c'
\nbecause the database to which they belong is not being journaled,
\nand journaling is disabled in one of these tables 
\nbut is set to be enabled after the next checkpoint in the other."
/*%
** Parameters:
**	%0c - name of referencing table
**	%1c - schema to which the referencing table belongs
**	%2c - name of referenced table
**	%3c - schema to which the referenced table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  referential constraints defined 
**	on it, we must prevent user from creating a referential constraint 
**	in a database which is not being journaled between two tables one of 
**	which has journaling disabled while the other may have it enabled after
**	the next checkpoint 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Ensure that both referencing and referenced tables have the same 
**	journaling status.  Journaling status of existing tables can be altered
**	by issuing SET [NO]JOURNALING ON <table>.  If the referencing table is 
**	being created, its journaling status can be influenced by specifying 
**	WITH [NO]JOURNALING.
*/
E_PS04AC_CONS_NON_JOR_TBL_JOR_DB:SS50000_MISC_ERRORS
"ALTER TABLE: You cannot create a %0c constraint
\non table '%1c' in schema '%2c' 
\nbecause the database to which it belongs is being journaled,
\nbut the data in the table itself is not being journaled."
/*%
** Parameters:
**	%0c - type of constraint; one of CHECK, UNIQUE, PRIMARY KEY
**	%1c - name of referencing table
**	%2c - schema to which the referencing table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  CHECK, UNIQUE, or PRIMARY KEY
**	constraints defined on it, we must prevent user from creating a CHECK, 
**	UNIQUE, or PRIMARY KEY constraint on a table whose data is not being 
**	journaled in a database which is being journaled.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	You need to SET JOURNALING on the table and to checkpoint the database
**	to enable journaling on that table.  Only then you may be allowed to
**	define a constraint on it.
*/
W_PS04AD_NON_JOUR_REFING_JOUR_DB:SS01000_WARNING
"ALTER TABLE: Specifying WITH NOJOURNAL_CHECK will enable creation 
\nof a REFERENTIAL constraint on 
\ntable '%0c' in schema '%1c' 
\neven though the database to which it belongs is being journaled,
\nbut the data in the table itself is not being journaled."
/*%
** Parameters:
**	%0c - name of referencing table
**	%1c - schema to which the referencing table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  referential constraints defined 
**	on it, we must prevent user from creating a referential constraint on a 
**	table whose data is not being journaled in a database which is being 
**	journaled.  
**
**	User has overridden this restriction by specifying WITH NOJOURNAL_CHECK.
**	User must be aware that if ROLLFORWARDDB is used on this database before
**	the next checkpoint is taken, it may end up in a state where data in the
**	referencing table will not satisfy the constraint added by this 
**	statement.
**
** System Status:
**	This is just a warning which will be sent to the error log file; 
**	the program continues.
**
** Recommendation:
**	We strongly discourage users from overriding restrictions linking 
**	ability to define constraints on tables with journaling status of the 
**	database and the table(s) involved in the constraint.  Users resorting 
**	to specifying WITH NOJOURNAL_CHECK should try to checkpoint the database
**	as soon as possible after adding this constraint in order to avoid a 
**	situation where use of ROLLFORWARDDB may render data in the table no 
**	longer satisfying the constraint defined by this statement.
*/
W_PS04AE_NON_JOUR_REFD_JOUR_DB:SS01000_WARNING
"ALTER TABLE: Specifying WITH NOJOURNAL_CHECK will enable creation
\nof a REFERENTIAL constraint 
\non table '%0c' in schema '%1c'
\nreferencing table '%2c' in schema '%3c' 
\neven though the database to which the referenced table belongs 
\nis being journaled, but the data in the table itself is not 
\nbeing journaled."
/*%
** Parameters:
**	%0c - name of referencing table
**	%1c - schema to which the referencing table belongs
**	%2c - name of referenced table
**	%3c - schema to which the referenced table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  referential constraints defined 
**	on it, we must prevent user from creating a referential constraint 
**	referencing a table whose data is not being journaled in a database 
**	which is being journaled.
**
**	User has overridden this restriction by specifying WITH NOJOURNAL_CHECK.
**	User must be aware that if ROLLFORWARDDB is used on this database before
**	the next checkpoint is taken, it may end up in a state where data in the
**	referencing table will not satisfy the constraint added by this 
**	statement.
**
** System Status:
**	This is just a warning which will be sent to the error log file; 
**	the program continues.
**
** Recommendation:
**	We strongly discourage users from overriding restrictions linking 
**	ability to define constraints on tables with journaling status of the 
**	database and the table(s) involved in the constraint.  Users resorting 
**	to specifying WITH NOJOURNAL_CHECK should try to checkpoint the database
**	as soon as possible after adding this constraint in order to avoid a 
**	situation where use of ROLLFORWARDDB may render data in the table no 
**	longer satisfying the constraint defined by this statement.
*/
W_PS04AF_REF_CONS_JOUR_MISMATCH:SS01000_WARNING
"ALTER TABLE: Specifying WITH NOJOURNAL_CHECK will enable creation
\nof a REFERENTIAL constraint
\non table '%0c' in schema '%1c' 
\nreferencing table '%2c' in schema '%3c'
\neven though the database to which they belong is not being journaled,
\nand journaling is disabled in one of these tables 
\nbut is set to be enabled after the next checkpoint in the other."
/*%
** Parameters:
**	%0c - name of referencing table
**	%1c - schema to which the referencing table belongs
**	%2c - name of referenced table
**	%3c - schema to which the referenced table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  referential constraints defined 
**	on it, we must prevent user from creating a referential constraint 
**	in a database which is not being journaled between two tables one of 
**	which has journaling disabled while the other may have it enabled after
**	the next checkpoint 
**
**	User has overridden this restriction by specifying WITH NOJOURNAL_CHECK.
**	User must be aware that if ROLLFORWARDDB is used on this database before
**	the next checkpoint is taken, it may end up in a state where data in the
**	referencing table will not satisfy the constraint added by this 
**	statement.
**
** System Status:
**	This is just a warning which will be sent to the error log file; 
**	the program continues.
**
** Recommendation:
**	We strongly discourage users from overriding restrictions linking 
**	ability to define constraints on tables with journaling status of the 
**	database and the table(s) involved in the constraint.  Users resorting 
**	to specifying WITH NOJOURNAL_CHECK should try to checkpoint the database
**	as soon as possible after adding this constraint in order to avoid a 
**	situation where use of ROLLFORWARDDB may render data in the table no 
**	longer satisfying the constraint defined by this statement.
*/
W_PS04B0_CONS_NON_JOR_TBL_JOR_DB:SS01000_WARNING
"ALTER TABLE: Specifying WITH NOJOURNAL_CHECK will enable creation
\nof a %0c constraint
\non table '%1c' in schema '%2c' 
\neven though the database to which it belongs is being journaled,
\nbut the data in the table itself is not being journaled."
/*%
** Parameters:
**	%0c - type of constraint; one of CHECK, UNIQUE, PRIMARY KEY
**	%1c - name of referencing table
**	%2c - schema to which the referencing table belongs
**
** Explanation:
**	In order to ensure that ROLLFORWARDDB processing does not result in
**	data in a table no longer satisfying  CHECK, UNIQUE, or PRIMARY KEY
**	constraints defined on it, we must prevent user from creating a CHECK, 
**	UNIQUE, or PRIMARY KEY constraint on a table whose data is not being 
**	journaled in a database which is being journaled.
**
**	User has overridden this restriction by specifying WITH NOJOURNAL_CHECK.
**	User must be aware that if ROLLFORWARDDB is used on this database before
**	the next checkpoint is taken, it may end up in a state where data in the
**	referencing table will not satisfy the constraint added by this 
**	statement.
**
** System Status:
**	This is just a warning which will be sent to the error log file; 
**	the program continues.
**
** Recommendation:
**	We strongly discourage users from overriding restrictions linking 
**	ability to define constraints on a table with journaling status of the 
**	database and the table involved in the constraint.  Users resorting 
**	to specifying WITH NOJOURNAL_CHECK should try to checkpoint the database
**	as soon as possible after adding this constraint in order to avoid a 
**	situation where use of ROLLFORWARDDB may render data in the table no 
**	longer satisfying the constraint defined by this statement.
*/
E_PS04B1_SET_JOUR_ON_REF_TBL:SS50000_MISC_ERRORS
"SET [NO]JOURNALING: You may not alter journaling status 
\nof table '%0c' in schema '%1c'
\nbecause it is involved in a REFERENCES constraint."
/*%
** Parameters:
**	%0c -	name of a table
**	%1c -	name of the schema to which the table belongs
**
** Explanation:
**	User attempted to issue SET [NO]JOURNALING statement against a table
**	which is involved in a REFERENCES constraint.  Alteration of journaling 
**	status of tables involved in REFERENCES constraint(s) is expressly 
**	proscribed since subsequent use of ROLLFORWARDDB may result in data in 
**	the table violating REFERENCES constraint(s) defined on it
**
** System Status:
**      The program continues; the current statement is aborted.
**
** Recommendation:
**	Avoid trying to alter journaling status of tables involved in 
**	REFERENCES constraint(s).
*/
E_PS04B2_CONS_REFOPTS:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER TABLE: only one ON DELETE and one ON UPDATE referential\n
action may be specified for each referential constraint definition."
/*%
** Parameters:
**	none
**
** Explanation:
**	If a CREATE TABLE or ALTER TABLE statement defines a 
**	referential constraint which includes referential actions
**	(ON DELETE, ON UPDATE clauses), at most one of each of
**	ON DELETE and ON UPDATE may be specified. This statement 
**	included two or more ON UPDATE and/or ON DELETE clauses for
**	the same referential relationship definition.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Select the proper combination of referential actions and
**	retry the statement.
*/
E_PS04B3_BAD_SEQOP_DEFAULT:SS5000B_INTERNAL_ERROR
"An invalid sequence default was retrieved from the system catalogs.\n
The invalid value was: %0c."
/*%
** Parameters:
**	%0c - The invalid default string
**
** Explanation:
**	The default value retrieved from the system catalogs started out
**	to be a sequence default ("next value for ..."), but the remainder
**	was malformed.  This could be caused by an internal error, or by
**	an administrator manually changing the defaults catalog.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Reverse the manual change, if any;  else report this error
**	to Technical Support.
*/
E_PS04B4_DEFAULT_ANSIDT_IN_DBPROC:SS42000_SYN_OR_ACCESS_ERR
"CREATE PROCEDURE: You have declared the parameter/variable
\n '%0c' as 'not null with default'. In database procdures,
\n defaults are not supported for parameters or variables of type
\n ANSIDATE/TIME/TIMESTAMP. Please specify 'not null not default'
\n or 'with null' in the declaration of this paramter/variable."
/*%
** Parameters:
**      %0c - The name of the parameter with invalid default string
**
** Explanation:
**      The database procedure procedure or variable is of type ANSIDATE 
**	or one of the ANSI time/timestamp types and has been declared
**	as not null with default. Defaults are not supported for ANSI 
**	date/time/timestamp types in procedure parameters/variables.
**      You must either use not null not default or with null for this
**	parameter/variable.
**
** System Status:
**      The query is aborted.
**
** Recommendation:
**      Modify the procedure definition and rerun the query.
*/
E_PS04B5_ONE_IDENTITY_COL:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE : Table may only have one identity column."
/*%
** Parameters:
**
** Explanation:
**	A table definition contained at least 2 identity column definitions.
**	This is not permitted.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Select only one identity column and run the query again.
*/
E_PS04B6_ADD_IDENTITY_COL:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : An identity column may not be added with ALTER TABLE."
/*%
** Parameters:
**
** Explanation:
**	An identity column may only be defined with CREATE TABLE. Use of 
**	ALTER TABLE ... ADD ... to add an identity column to an existing
**	table is prohibited.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the statement and run it again.
*/
E_PS04B7_BAD_IDENTITY_TYPE:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE : Identity column %0c must be exact numeric (integer or
\ndecimal with scale 0)."
/*%
** Parameters:
**	%0c - Column name
**
** Explanation:
**	An identity column must be of type exact numeric (integer or decimal
**	with scale 0).
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the statement and run it again.
*/
E_PS04B8_IDENTITY_DEFAULT:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE : Identity column %0c cannot specify any other default."
/*%
** Parameters:
**	%0c - Column name
**
** Explanation:
**	An identity column definition must not include any other default
**	specification.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the statement and run it again.
*/
E_PS04B9_ALTER_IDENTITY:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Identity status for column %0c cannot be altered \n
when it is not already defined as identity."
/*%
** Parameters:
**	%0c - Column name
**
** Explanation:
**	ALTER TABLE ALTER COLUMN SET ... can only be done on a column 
**	that is already defined as identity.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the statement and run it again.
*/
E_PS04BA_BAD_OVERRIDING:SS42000_SYN_OR_ACCESS_ERR
"INSERT : OVERRIDING clause must not be used
\nif there is no identity column."
/*%
** Parameters:
**
** Explanation:
**	Any INSERT statement with an OVERRIDING clause must include an
**	identity column in its column list.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the statement and run it again.
*/
E_PS04BB_OVERRIDING_ALWAYS:SS42000_SYN_OR_ACCESS_ERR
"INSERT : OVERRIDING SYSTEM VALUE clause must accompany
\nINSERT into GENERATED ALWAYS AS IDENTITY column %0c."
/*%
** Parameters:
**	%0c - Column name
**
** Explanation:
**	INSERT statement includes GENERATED ALWAYS identity column without
**	an accompanying OVERRIDING SYSTEM VALUE clause.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Add the required clause and run again.
*/
E_PS04BC_OVERRIDING_BYDEF:SS42000_SYN_OR_ACCESS_ERR
"INSERT : OVERRIDING USER VALUE clause (if any) must accompany
\nINSERT into GENERATED BY DEFAULT AS IDENTITY column %0c."
/*%
** Parameters:
**	%0c - Column name
**
** Explanation:
**	INSERT statement includes GENERATED BY DEFAULT identity column with
**	an accompanying OVERRIDING SYSTEM VALUE clause. If the OVERRIDING
**	clause is included at all, it must be OVERRIDING USER VALUE.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Change OVERRIDING clause to USER VALUE or remove it altogether
**	and run again.
*/
E_PS04C0_BAD_SCHEMA_TREE:SS5000B_INTERNAL_ERROR
"PSF internal error: First node on CREATE SCHEMA tree not valid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	While preparing to reorder nodes on a CREATE SCHEMA tree, 
**	the DBMS detected that the first node was not valid for
**	such a tree.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS04C1_BAD_SCHEMA_NODE:SS5000B_INTERNAL_ERROR
"PSF internal error: Invalid node encountered on CREATE SCHEMA tree."
/*%
** Parameters:
**	None.
**
** Explanation:
**	While reordering nodes on a CREATE SCHEMA tree, the DBMS detected
**	an invalid node on the tree.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS04C2_BAD_SCHEMA_OBJECT:SS5000B_INTERNAL_ERROR
"PSF internal error: Unable to determine object name
\n for node encountered on CREATE SCHEMA tree."
/*%
** Parameters:
**	None.
**
** Explanation:
**	While reordering nodes on a CREATE SCHEMA tree, the DBMS could not
**	determine the object name for a node representing a "CREATE TABLE"
**	or "CREATE VIEW".
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
**	Program continues; current statement is aborted.
** User Action:
**	Contact your Ingres Corporation technical representative.
*/
E_PS04D0_GRANT_COPY_ON_VIEW:SS42000_SYN_OR_ACCESS_ERR
"GRANT: '%0c' is a view.
\n'COPY_INTO' and 'COPY_FROM' privilege may not be granted on views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to grant COPY_INTO or COPY_FROM privilege
**	on a view, which is illegal.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the query and reenter. Either COPY_INTO, COPY_FROM must be
**	removed from the privilege list, or the offending view must be
**	removed from the object list.
*/
E_PS04D1_ALARM_INVALID_OBJECT:SS42000_SYN_OR_ACCESS_ERR
"'%0c' is %1c. Security alarms may not be created/dropped on
%2c."
/*%
** Parameters:
**	%0c - object name
**	%1c - object type
**	%2c - object type
**
** Description:
**	An attempt was made to create a security alarm on an object that
**	does not have security alarms; for example, a view or index.
** 
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the query and reenter.  
*/
E_PS04D2_ALARM_NO_OBJECT_NAMES:SS42000_SYN_OR_ACCESS_ERR
"No objects of type '%0c' were specified when creating a security alarm."
/*%
** Parameters:
**	%0c - object type
**
** Explanation:
**	An attempt was made to create a security alarm on, but no objects of
**	the appropriate type were specified.
** 
** System status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the query and reenter.  
*/
E_PS04D3_ALARM_TOO_MANY_OBJS:SS42000_SYN_OR_ACCESS_ERR
"Too many objects of type '%0c' were specified when creating a security alarm.
The maximum allowed is %1d."
/*%
** Parameters:
**	%0c - object type
**	%1d - maximum objects
**
** Description:
**	An attempt was made to create a security alarm on, but too many
**	objects of the indicated type were specified.
** 
** System status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the query and reenter.  
*/
E_PS04D4_ALARM_TOO_MANY_SUBJS:SS42000_SYN_OR_ACCESS_ERR
"Too many subjects were specified when creating a security alarm.
The maximum allowed is %0d."
/*%
** Parameters:
**	%0d - maximum subjects
**
** Explanation:
**	An attempt was made to create a security alarm on, but too many
**	subjects were specified. 
** 
** System status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Change the query and reenter.  
*/
E_PS04D5_ALARM_OBJ_OPER_MISMATCH:SS42000_SYN_OR_ACCESS_ERR
"CREATE SECURITY_ALARM: operation '%0c' may not be specified for an
object of type '%1c'."
/*%
** Parameters:
**	%0c - operation name
**	%1c - object type
**
** Explanation:
**	An attempt was made to create a security alarm, but an illegal
**	operation was specified for the type of object. 
** 
** System status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Make sure the operation (SELECT, CONNECT, etc.) is appropriate
**	for the type of object (TABLE, DATABASE, etc.), then rerun the query.
*/
E_PS0501_SESSION_OPEN:SS5000B_INTERNAL_ERROR
"There were open sessions when trying to shut down the parser facility."
E_PS0502_LEFTOVER_MEM:SS5000B_INTERNAL_ERROR
"psq_shutdown: After shutting down all sessions, there was still allocated
memory in the parser facility."
E_PS0519_QSF_INFO:SS5000B_INTERNAL_ERROR
"Error getting QSF information on object."
E_PS0521_BAD_TBLID_LIST:SS5000B_INTERNAL_ERROR
"PSF internal error: an error occurred while building an ORDERED
\nlist of table ids for a QUEL shared repeat query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is produced if, while scanning the first N entries
**	in the LRU range variable list, we encounter an entry that was
**	not used (which would contradict one of the basic assumptions
**	behind the algorithm for determining if a given QUEL repeat query
**	is shareable.)
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS0522_QRYHDR_INFO_CONFLICT:SS5000B_INTERNAL_ERROR
"PSF internal error: a conflict was detected while trying to
\nuse an info structure inside the query tree header."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A conflict was detected while trying to use an info structure inside
**	the query tree header.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS0550_GRANTEE_TYPE_FOR_WGO:SS42000_SYN_OR_ACCESS_ERR
"GRANT: 'WITH GRANT OPTION' may not be specified if the
\ngrantee is of type 'group' or 'role'."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A "GRANT" statement naming grantees of type "group" or "role"
**	may not specify "WITH GRANT OPTION".
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0551_INSUF_PRIV_GRANT_OBJ:SS01007_PRIV_NOT_GRANTED
"GRANT: '%0c' on '%1c' owned by '%2c'
\nmust have been granted to you WITH GRANT OPTION in order
\nfor you to grant these privilege(s) to other users."
/*%
** Parameters:
**	%0c - privileges list
**	%1c - object name
**	%2c - object owner
**
** Explanation:
**	The user attempted to grant some privilege(s) on another user's object
**	even though these privilege(s) have not been granted to the current
**	user with the "WITH GRANT OPTION" clause.
**
** System Status:
**	The permit on this object will not be created; other objects (if any)
**	mentioned in the statement will be processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0552_INSUF_PRIV_INDEP_OBJ:SS01007_PRIV_NOT_GRANTED
"CREATE PERMIT/GRANT: You may not grant '%4c'
\non your view '%0c' because you lack '%3c WITH GRANT OPTION'
\non '%1c' owned by '%2c'
\nwhich is used in the definition of '%0c'."
/*%
** Parameters:
**	%0c - view name
**	%1c - object name
**	%2c - object owner
**	%3c - privileges without WITH GRANT OPTION
**	%4c - privileges that could not be granted on the view
**
** Explanation:
**	The user attempted to grant access to a view whose definition involves
**	an object owned by another user.  The current user lacks some
**	privileges with the WITH GRANT OPTION that are necessary in order
**	to grant the desired access on the view.
**
** System Status:
**	The permit on this object will not be created; other objects (if any)
**	mentioned in the statement will be processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0553_NONUPDATEABLE_VIEW:SS42503_INSUF_PRIV
"GRANT/CREATE PERMIT: View '%0c' is not updateable
\nbecause it is not a single-variable view or its definition involves
\naggregates, set operator(s) (UNION) or DISTINCT.
\n'%1c' cannot be granted on non-updateable views"
/*%
** Parameters:
**	%0c - view name
**	%1c - privileges list
**
** Explanation:
**	The user attempted to grant one or more of INSERT, DELETE, UPDATE on
**	this view, which is not updateable because its definition involves
**	UNION, DISTINCT, or aggregates or it is a multi-variable view.
**
** System Status:
**	The permit on this object will not be created; other objects (if any)
**	mentioned in the statement will be processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0554_NONUPDT_VIEW_IN_DEFN:SS42503_INSUF_PRIV
"GRANT/CREATE PERMIT: View '%0c' is not updateable
\nbecause it is defined over a non-updateable view
\n'%1c' owned by '%2c'.
\n'%3c' cannot be granted on non-updateable views"
/*%
** Parameters:
**	%0c - view name
**	%1c - non-updateable view
**	%2c - view owner
**	%3c - privileges list
**
** Explanation:
**	The user attempted to grant one or more of INSERT, DELETE, UPDATE on
**	this view, which is not updateable because it is defined over a
**	non-updateable view.
**
** System Status:
**	The permit on this object will not be created; other objects (if any)
**	mentioned in the statement will be processed in turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0555_DBPGRANT_LACK_TBLPRIV:SS42503_INSUF_PRIV
"GRANT ON PROCEDURE: You may not grant privileges on your
\ndatabase procedure '%2c' because you lack
\n'%3c' on table '%0c' owned by '%1c'
\nwhich is used in '%2c'."
/*%
** Parameters:
**	%0c - table or view name
**	%1c - table or view owner
**	%2c - database procedure
**	%3c - privileges list
**
** Explanation:
**	The user attempted to grant EXECUTE on this database procedure, which is
**	ungrantable because they lack some WITH GRANT OPTION privileges on a
**	table or view used in the definition of that database procedure or some
**	other database procedure invoked by it.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0556_DBPGRANT_LACK_DBPPRIV:SS42503_INSUF_PRIV
"GRANT ON PROCEDURE: You may not grant privileges on your
\ndatabase procedure '%2c' because you lack
\n'EXECUTE WITH GRANT OPTION' on database procedure '%0c'
\nowned by '%1c' which is used in '%2c'."
/*%
** Parameters:
**	%0c - database procedure without "EXECUTE WITH GRANT OPTION"
**	%1c - database procedure owner
**	%2c - database procedure in which %0c occurs
**
** Explanation:
**	The user attempted to grant EXECUTE on their database procedure,
**	which is ungrantable because they lack "EXECUTE WITH GRANT OPTION" on
**	some database procedure used in the definition of that database
**	procedure or some other database procedure invoked by the database
**	procedure mentioned in the "GRANT" statement.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0557_DBPGRANT_LACK_EVPRIV:SS42503_INSUF_PRIV
"GRANT ON PROCEDURE: You may not grant privileges on your
\ndatabase procedure '%2c' because you lack '%3c'
\non dbevent '%0c' owned by '%1c'
\nwhich is used in '%2c'."
/*%
** Parameters:
**	%0c - dbevent name
**	%1c - dbevent owner
**	%2c - database procedure name
**	%3c - privileges list
**
** Explanation:
**	The user attempted to grant EXECUTE on their database procedure, which
**	is ungrantable because they lack some privileges with the "WITH GRANT
**	OPTION" on a dbevent used in the definition of that database procedure
**	or some other database procedure invoked by it.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0558_GRANT_CHECK_ERROR:SS50000_MISC_ERRORS
"GRANT ON PROCEDURE: You may not grant EXECUTE on database procedure
\n'%0c' owned by you.  Unexpected error was encountered while
\nattempting to determine if '%0c' is grantable."
/*%
** Parameters:
**	%0c - database procedure name
**
** Explanation:
**	An attempt to reparse the text of a database procedure in order to
**	determine if it is grantable resulted in an unexpected error
**	(the only expected error is lack of required privileges).
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Examine the error log for message(s) describing the error.
**	Correct the error and reenter.
*/
E_PS0559_CANT_GRANT_DUE_TO_ERR:SS50000_MISC_ERRORS
"GRANT ON PROCEDURE: You may not grant EXECUTE on database procedure
\n'%0c' owned by you.  Unexpected error was encountered while
\nattempting to determine if your database procedure '%1c'
\nused in '%0c' is grantable."
/*%
** Parameters:
**	%0c - database procedure on which user attempted to grant
**	      EXECUTE privilege
**	%1c - database procedure used in %0c
**
** Explanation:
**	An attempt to reparse the text of a database procedure used in the
**	definition of the database procedure on which the user wanted to grant
**	EXECUTE privilege resulted in an unexpected error (the only expected
**	error is lack of required privileges).
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Examine the error log for message(s) describing the error.
**	Correct the error and reenter.
*/
E_PS055A_UNGRANTABLE_DBP:SS42503_INSUF_PRIV
"GRANT ON PROCEDURE: you may not grant EXECUTE on your database procedure
\n'%0c' because you may not grant privileges
\non your database procedure '%1c' which is used in '%0c'."
/*%
** Parameters:
**	%0c - database procedure owned by the current user
**	%1c - ungrantable database procedure
**
** Explanation:
**	The user attempted to grant EXECUTE on their database procedure, which
**	is ungrantable because it uses another database procedure owned by the
**	current user that is ungrantable.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS055B_CREATE_DORMANT_DBP:SS42503_INSUF_PRIV
"CREATE PROCEDURE: You may not create database procedure '%0c'
\nbecause database procedure '%1c' invoked by it is dormant.
\nAny database procedure attempting to invoke a dormant
\ndatabase procedure is itself dormant, and it is illegal
\nto create dormant database procedures."
/*%
** Parameters:
**	%0c - database procedure the user was attempting to create
**	%1c - dormant database procedure
**
** Explanation:
**	The user attempted to create a database procedure which invokes
**	(either directly or through other procedure(s)) a dormant database
**	procedure.  Creating database procedures which are known to be dormant
**	at creation time is illegal.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Ensure that the new database procedure does not invoke any dormant
**	database procedures.
*/
E_PS055C_RECREATE_DORMANT_DBP:SS42503_INSUF_PRIV
"You may not execute your database procedure '%0c'
\nbecause database procedure '%1c' invoked by it is dormant.
\nAny database procedure attempting to invoke a dormant
\ndatabase procedure is itself dormant, and it is illegal
\nto execute dormant database procedures."
/*%
** Parameters:
**	%0c - database procedure owned by the current user
**	%1c - dormant database procedure
**
** Explanation:
**	While trying to recreate a database procedure, it was determined
**	that it the procedure is dormant.  Executing dormant database
**	procedures is illegal.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Ensure that the database procedure does not invoke any dormant
**	database procedures.
*/
E_PS055D_ERR_IN_CREATE_DBP:SS50000_MISC_ERRORS
"CREATE PROCEDURE: You may not create database procedure '%0c'.
\nUnexpected error was encountered while attempting to verify
\nthat your database procedure '%1c'
\nused in '%0c' is not dormant."
/*%
** Parameters:
**	%0c - database procedure which the user was attempting to create
**	%1c - database procedure used in %0c
**
** Explanation:
**	An attempt to determine whether a database procedure used in the
**	definition of the database procedure being created is dormant resulted
**	in an unexpected error.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Examine the error log for message(s) describing the error.
**	Correct the error and reenter.
*/
E_PS055E_ERR_IN_RECREATE_DBP:SS50000_MISC_ERRORS
"You may not execute your database procedure '%0c'.
\nUnexpected error was encountered while attempting to verify
\nthat your database procedure '%1c'
\nused in '%0c' is not dormant."
/*%
** Parameters:
**	%0c - database procedure being recreated
**	%1c - database procedure used in %0c
**
** Explanation:
**	An attempt to determine whether a database procedure used in the
**	definition of the database procedure being recreated is dormant
**	resulted in an unexpected error.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Examine the error log for message(s) describing the error.
**	Correct the error and reenter.
*/
E_PS055F_DBP_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"Database procedure with id (%0d, %1d) could not be found."
/*%
** Parameters:
**	%0d - db_tab_base of the dbproc ID
**	%1d - db_tab_index of the dbproc ID
**
** Explanation:
**	An attempt to find a database procedure by its ID failed.  This is
**	likely to happen when the DBMS is trying to verify that a database
**	procedure is active or grantable, using the independent object and
**	privilege lists stored in iidbdepends and iipriv system catalogs.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation Technical Support representative.
*/
E_PS0560_NOPRIV_ON_GRANT_OBJ:SS01007_PRIV_NOT_GRANTED
"GRANT ALL [PRIVILEGES]: You may not grant any privileges on
\ntable/view '%0c' owned by '%1c' because
\nyou possess no grantable privileges on it."
/*%
** Parameters:
**	%0c - object name
**	%1c - object owner
**
** Explanation:
**	The user attempted to grant ALL [PRIVILEGES] on another user's object,
**	even though the user possesses no grantable privileges on that object.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0561_NOPRIV_UNDERLYING_OBJ:SS01007_PRIV_NOT_GRANTED
"GRANT ALL [PRIVILEGES]: You may not grant any privileges
\non your view '%0c' because you possess no grantable privilege
\non '%1c' owned by '%2c' on which your view is defined."
/*%
** Parameters:
**	%0c - view name
**	%1c - object name
**	%2c - object owner of the above object
**
** Explanation:
**	The user attempted to grant ALL [PRIVILEGES] on view V, but they
**	possess no grantable privileges on another user's object X, which is
**	(in SQL92) a leaf underlying object of the <query expression> of V.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0562_NOPRIV_LACKING_SELECT:SS01007_PRIV_NOT_GRANTED
"GRANT ALL [PRIVILEGES]: You may not grant any privileges
\non your view '%0c' because you lack grantable SELECT privilege
\non '%1c' owned by '%2c' which is used
\nin the definition of your view."
/*%
** Parameters:
**	%0c - view name
**	%1c - object name
**	%2c - object owner
**
** Explanation:
**	The user attempted to grant ALL [PRIVILEGES] on view V, but they lack a
**	grantable SELECT privilege on another user's object X, which is used in
**	V's definition.  This could happen if V is defined on top of X, and
**	the current user possesses no grantable privileges on X, or if the
**	current user possesses only grantable SELECT on the object on which V
**	is defined but does not possess grantable SELECT on X, which is used
**	in V's definition.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0563_NOPRIV_ON_GRANT_EV:SS01007_PRIV_NOT_GRANTED
"GRANT ALL [PRIVILEGES]: You may not grant any privileges on
\ndbevent '%0c' owned by '%1c' because
\nyou possess no grantable privileges on it."
/*%
** Parameters:
**	%0c - dbevent name
**	%1c - dbevent owner
**
** Explanation:
**	The user attempted to grant ALL [PRIVILEGES] on another user's dbevent,
**	even though the user possesses no grantable privileges on that dbevent.
**
** System Status:
**	The permit on this database procedure will not be created; other
**	objects (if any) mentioned in the statement will be processed in
**	turn.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0564_ALLPROT_ABANDONED_OBJ:SS2B000_DEP_PRIV_EXISTS
"DROP PERMIT ALL: you may not drop all permits on %0c '%1c'
\nbecause it would cause some permit or object to become abandoned."
/*%
** Parameters:
**	%0c - object type (table, dbproc, or dbevent)
**	%1c - object name
**
** Explanation:
**	The user attempted to drop all permits on object X, but there is some
**	object or permit on another object Y, which depend on privilege(s) on
**	X.  Since <drop behavior> of the "DROP PERMIT" statement is RESTRICT,
**	destruction of permits on X using "DROP PERMIT ALL" will be disallowed.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	To revoke privileges, use the "REVOKE CASCADE" statement, which will
**	also destroy all objects dependent on specified privileges.
*/
E_PS0565_PROT_ABANDONED_OBJ:SS2B000_DEP_PRIV_EXISTS
"DROP PERMIT: you may not drop permit %0d on %1c '%2c'
\nbecause it would cause some permit or object to become abandoned."
/*%
** Parameters:
**	%0d - permit number
**	%1c - object type (table, dbproc, or dbevent)
**	%2c - object name
**
** Explanation:
**	The user attempted to drop a permit on object X, but there is some
**	object or permit on another object Y, which depends on privileges on
**	X conveyed by that permit. Since <drop behavior> of the "DROP PERMIT"
**	statement is RESTRICT, destruction of this permit on X using "DROP
**	PERMIT ALL" will be disallowed.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	To revoke privileges, use the "REVOKE CASCADE" statement, which will
**	also destroy all objects dependent on specified privileges.
*/
E_PS0566_REVOKE_FROM_PUBLIC_ERR:SS2B000_DEP_PRIV_EXISTS
"REVOKE...RESTRICT: you may not revoke specified privileges
\n on %0c '%1c'.'%2c' from PUBLIC
\nbecause it would cause some permit or object to become abandoned."
/*%
** Parameters:
**	%0c - object type (table, dbproc, or dbevent)
**	%1c - schema name
**	%2c - object name
**
** Explanation:
**	The user issued a "REVOKE ... FROM PUBLIC RESTRICT" statement, but
**	some privilege(s) specified in the statement cannot be revoked, because
**	it would render some object(s) or permit(s) abandoned.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	To revoke privileges, use the "REVOKE CASCADE" statement, which will
**	also destroy all objects dependent on specified privileges.
*/
E_PS0567_REVOKE_FROM_USER_ERR:SS2B000_DEP_PRIV_EXISTS
"REVOKE...RESTRICT: you may not revoke specified privileges
\n on %0c '%1c'.'%2c' from user '%3c'
\nbecause it would cause some permit or object to become abandoned."
/*%
** Parameters:
**	%0c - object type (table, dbproc, or dbevent)
**	%1c - schema name
**	%2c - object name
**	%3c - user name
**
** Explanation:
**	The user issued a "REVOKE ... FROM [USER] <auth id> RESTRICT"
**	statement, but some privilege(s) specified in the statement cannot
**	be revoked, because it would render some object(s) or permit(s)
**	abandoned.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	To revoke privileges, use the "REVOKE CASCADE" statement, which will
**	also destroy all objects dependent on specified privileges.
*/
E_PS0570_NO_SCHEMA_TO_DROP:SS42505_OBJ_NOT_FOUND
"DROP SCHEMA: schema '%0c' owned by '%1c' does not exist."
/*%
** Parameters:
**	%0c - schema name
**	%1c - schema owner
**
** Explanation:
**	The user attempted to drop a non-existent schema.  
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	If the specified name was incorrect, correct the query and reenter.
*/
E_PS0571_NONEMPTY_SCHEMA:SS42000_SYN_OR_ACCESS_ERR
"DROP SCHEMA ... RESTRICT: Restricted destruction of schema '%0c'
\nowned by '%1c' could not be performed because that schema
\ncontains a table, synonym, database procedure, or dbevent."
/*%
** Parameters:
**	%0c - schema name
**	%1c - schema owner
**
** Explanation:
**	The user attempted restricted destruction of a schema that contains
**	at least one table, view, synonym, dbevent, or database procedure.
**	Restricted destruction of a schema may proceed only if it does not
**	contain any tables, views, synonyms, database procedures, or dbevents.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Consult ther error log for a type and name of an object found in the
**	specified schema.
**
**	If the user wants to destroy the schema along with any objects
**	contained therein, "DROP SCHEMA <schema name> CASCADE" must be
**	used.
*/
E_PS0572_CANT_DROP_SYSCAT_SCHEMA:SS42503_INSUF_PRIV
"DROP SCHEMA: Schema \"%0c\" owned by \"%0c\" cannot be dropped."
/*%
** Parameters:
**	%0c - the name of the system catalog owner
**
** Explanation:
**	The user attempted to drop the system catalog owner's schema.  This
**	schema cannot be dropped, since it will result in the destruction of all
**	system catalogs and would render the database totally useless.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0573_CANT_DROP_OTHERS_SCHEMA:SS42503_INSUF_PRIV
"DROP SCHEMA: you may not drop schema '%0c' owned by '%1c'."
/*%
** Parameters:
**	%0c -schema name
**	%1c - schema owner
**
** Explanation:
**	The user attempted to drop a schema owned by another user.  This is
**	illegal.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
W_PS0580_WCO_AND_UNION:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because its <query expression> involves UNION, and
\nWITH CHECK OPTION should not be specified for non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the view involves a UNION.  Such views are not
**	updatable, and allowing the "WITH CHECK OPTION" clause for them
**	violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0581_WCO_AND_DISTINCT:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because DISTINCT is specified in the outermost subselect
\nof its definition, and WITH CHECK OPTION should not be specified for
\nnon-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the view involves DISTINCT in the outermost
**	subselect of its definition. Such views are not updatable, and
**	allowing the "WITH CHECK OPTION" clause for them violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0582_WCO_AND_MULT_VARS:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the FROM-list of the 
\noutermost subselect of its definition contains more than one
\n<table reference>, and WITH CHECK OPTION should not be specified for
\nnon-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the view is a multi-variable view (i.e., the
**	FROM-list in the outermost subselect of its definition contains
**	more than one <table reference>.) Such views are not updatable,
**	and allowing the "WITH CHECK OPTION" clause for them violates
**	ANSI SQL92.
**
** System Status:
**	The view will be created withe the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0583_WCO_AND_GROUP_BY:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the outermost subselect of its definition
\ncontains a <group by clause>, and WITH CHECK OPTION should not be specified
\nfor non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the outermost subselect of the view's definition
**	contains a <group by clause>.  Such views are not updatable, and
**	allowing the "WITH CHECK OPTION" clause for them violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0584_WCO_AND_HAVING:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the outermost subselect of its definition
\ncontains a <having clause>, and WITH CHECK OPTION should not be specified
\nfor non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause even though the outermost subselect of the view's definition
**	contains a <having clause>.  Such views are not updatable, and
**	allowing the "WITH CHECK OPTION" clause for them violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0585_WCO_AND_SET_FUNCS:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the outermost subselect of its definition
\ninvolves a <set function>, and WITH CHECK OPTION should not be specified
\nfor non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the outermost subselect of the view's definition
**	contains a <set function> (e.g., COUNT, SUM, etc.). Such views are
**	not updatable, and allowing the "WITH CHECK OPTION" clause for them
**	violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0586_WCO_AND_UNION_IN_UV:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because <query expression> of its underlying view
\ninvolves UNION, and WITH CHECK OPTION should not be specified for
\nnon-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view withe the "WITH CHECK OPTION"
**	clause, even though the view's underlying view involves a UNION.
**	Such views are not updatable, and allowing the "WITH CHECK OPTION"
**	clause for them violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0587_WCO_AND_DISTINCT_IN_UV:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because DISTINCT is specified in the outermost
\nsubselect of the definition of its underlying view, and WITH CHECK OPTION
\nshould not be specified for non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the view's underlying view involves a DISTINCT
**	in the outermost subselect of its definition.  Such views are not
**	updatable, and allowing the "WITH CHECK OPTION" clause for them
**	violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0588_WCO_AND_MULT_VARS_IN_UV:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the FROM-list of the outermost subselect
\nof the definition of its underlying view contains more than one
\n<table reference>, and WITH CHECK OPTION should not be specified for
\nnon-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION" clause,
**	even though the view's underlying view is a multi-variable view
**	(i.e., the FROM-list in the outermost subselect of its definition
**	consists of more than one <table reference>.) Such views are not
**	updatable, and allowing the "WITH CHECK OPTION" clause for them
**	violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS0589_WCO_AND_GROUP_BY_IN_UV:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the outermost subselect of the definition
\nof its underlying view contains a <group by clause>, and WITH CHECK OPTION
\nshould not be specified for non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the outermost subselect of the underlying view
**	contains a <group by clause>.  Such views are not updatable, and
**	allowing the "WITH CHECK OPTION" clause for them violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS058A_WCO_AND_HAVING_IN_UV:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the outermost subselect of the definition
\nof its underlying view contains a <having clause>, and WITH CHECK OPTION
\nshould not be specified for non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the outermost subselect of the definition of
**	its underlying view contains a <having clause>.  Such views are not
**	updatable, and allowing the "WITH CHECK OPTION" clause for them
**	violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS058B_WCO_AND_SET_FUNCS_IN_UV:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nIt is non-updatable because the outermost subselect of definition
\nof its underlying view involves a <set function>, and WITH CHECK OPTION
\nshould not be specified for non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the outermost subselect of the definition of its
**	underlying view contains a <set function> (e.g., COUNT, SUM, etc.).
**	Such views are not updatable, and allowing the "WITH CHECK OPTION"
**	clause for them violates ANSI SQL92.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS058C_WCO_AND_LUT_IN_WHERE_CLAUSE:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nThe base table on which it is defined is also involved in a subquery
\ninside its qualification, and WITH CHECK OPTION should not be specified for
\nsuch views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the base table on which a view is defined is
**	also involved in the WHERE-clause of the view definition. In SQL92,
**	this is the case where the <table expression> immediately contained
**	in the <query specification> (QS) of <view definition> contains a
**	<where clause> (WC), and the leaf generally underlying table of the
**	QS is a generally underlying table of some <query expression>
**	contained in the WC.
**
**	Such views are not updatable under SQL92 rules, but are updatable under
**	extended SQL rules. However, the "WITH CHECK OPTION" clause should not
**	be specified for such views.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	N/A
*/
W_PS058D_WCO_AND_NO_UPDATABLE_COLS:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION.
\nNone of its columns are updatable, and WITH CHECK OPTION should not be
\nspecified for such views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though none of its columns are updatable.  The extended SQL
**	definition of what constitutes an updatable view is much looser than
**	the SQL92 definition, but we need to set the limit somewhere.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	None.
*/
W_PS058F_WCO_AND_UNSPEC_REASON:SS01000_WARNING
"CREATE VIEW: View '%0c' will be created without CHECK OPTION
\nbecause it is non-updatable."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a view with the "WITH CHECK OPTION"
**	clause, even though the view is non-updatable.  This message will
**	be produced only if a new criterion has been added to the list of
**	those that would render a view non-updatable, but the DBMS code
**	responsible for producing the message has not been appropriately
**	modified.
**
** System Status:
**	The view will be created with the "WITHOUT CHECK OPTION" clause.
**
** Recommendation:
**	Notify your Ingres Corporation Technical Support representative.
*/
E_PS0590_UNION_IN_RULE_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because its <query expression> involves
\nUNION, and rules may not be created on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though it involves
**	a UNION. Such views are not updatable, and rules cannot be specified
**	for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0591_DISTINCT_IN_RULE_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because DISTINCT is specified in the
\noutermost subselect of its definition, and rules may not
\nbe created on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though it involves
**	a DISTINCT in the outermost subselect of its definition. Such views are
**	not updatable, and rules cannot be specified for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0592_MULT_VARS_IN_RULE_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because the FROM-list of the 
\noutermost subselect of its definition contains more than
\none <table reference>, and rules may not be created on
\nnon-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though it
**	is a multi-variable view (i.e., the FROM-list in the outermost
**	subselect of its definition contains more than one <table reference>.).
**	Such views are not updatable, and rules cannot be specified for
**	non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0593_GROUP_BY_IN_RULE_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because the outermost subselect of its
\ndefinition contains a <group by clause>, and rules may not
\nbe created on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though the
**	outermost subselect of the view's definition contains a <group by
**	clause>.  Such views are not updatable, and rules cannot be specified
**	for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0594_AGGR_IN_RULE_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because the outermost subselect of its
\ndefinition involves a <set function>, and rules may not be
\ncreated on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though the
**	outermost subselect of the its definition contains a <set function>
**	(e.g., COUNT, SUM, etc.).  Such views are not updatable, and rules
**	cannot be specified for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter
*/
E_PS0595_CANNOT_UPDT_RULE_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because none of its attributes are based
\non attributes of its underlying table, and rules may not be
\ncreated on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though none of
**	its columns are based on attributes of its underlying table.
**	DELETE/INSERT/UPDATE cannot be specified for such view and neither can
**	rules.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0596_CRT_RULE_UNION_IN_UV:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because the <query expression> of its
\nunderlying view involves UNION, and rules may not be created
\non non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though its
**	underlying view involves a UNION.  Such views are not updatable,
**	and rules cannot be specified for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0597_CRT_RULE_DISTINCT_IN_UV:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because DISTINCT is specified in the
\noutermost subselect of the definition of its underlying view,
\nand rules may not be created on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though its
**	underlying view involves a DISTINCT in the outermost subselect
**	of its definition. Such views are not updatable, and rules cannot
**	be specified for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0598_CRT_RULE_MULT_VARS_IN_UV:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because the FROM-list of the 
\noutermost subselect of the definition of its underlying
\nview contains more than one <table reference>, and rules
\nmay not be created on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though its
**	underlying view is a multi-variable view (i.e., the FROM-list in
**	the outermost subselect of its definition contains more than one
**	<table reference>.) Such views are not updatable, and rules cannot
**	be specified for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0599_CRT_RULE_GROUP_BY_IN_UV:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because the outermost subselect of the 
\ndefinition of its underlying view contains a <group by clause>,
\nand rules may not be created on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though the
**	outermost subselect of the definition of its underlying view contains a
**	<group by clause>. Such views are not updatable, and rules cannot be
**	specified for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS059A_CRT_RULE_AGGR_IN_UV:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because the outermost subselect of the
\ndefinition of its underlying view involves a <set function>,
\nand rules may not be created on non-updatable views."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though the
**	outermost subselect of the definition of its underlying view contains
**	a <set function> (e.g., COUNT, SUM, etc).  Such views are not
**	updatable, and rules cannot be specified for non-updatable views.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS059B_CRT_RULE_CANNOT_UPDT_UV:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Rule cannot be created on view '%0c'.
\nIt is non-updatable because none of the attributes of its
\nunderlying view are based on attributes of that view's
\nunderlying table, and rules may not be created on non-updatable
\nviews."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	The user attempted to create a rule on a view, even though none of
**	the columns of its underlying view are based on attributes of that
**	view's underlying table.  DELETE/INSERT/UPDATE cannot be specified
**	for such view and neither can rules.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS059C_TID_REF_IN_NONUPDT_VIEW:SS42000_SYN_OR_ACCESS_ERR
"A query contained a reference to TID attribute of non-updatable view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A query contained a reference to the TID attribute of a non-updatable
**	view.  For updatable views, such references get translated into
**	references to the view's underlying base table, but they are illegal
**	for non-updatable views.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Correct the query and reenter.
*/
E_PS0601_CURSOR_OPEN:SS5000B_INTERNAL_ERROR
"There were open cursors when ending a session."
E_PS0602_CORRUPT_MEM:SS5000B_INTERNAL_ERROR
"PSF memory was discovered corrupted when ending a session."
E_PS0701_SRV_ALREADY_UP:SS5000B_INTERNAL_ERROR
"The parser facility was already initialized for the server when SCF asked to
initialize it again."
E_PS0702_NO_MEM:SS50003_EXHAUSTED_RESOURCE
"Error allocating the PSF memory pool."
E_PS0703_RDF_ERROR:SS5000B_INTERNAL_ERROR
"Error occurred while calling RDF."
E_PS0704_INSF_MEM:SS5000B_INTERNAL_ERROR
"Insufficient memory allocated per parser session."
E_PS0901_REDESC_RNG:SS5000B_INTERNAL_ERROR
"An attempt was made to re-describe a range variable."
E_PS0902_BAD_DESCTYPE:SS5000B_INTERNAL_ERROR
"An illegal description request was specified in a range table operation."
E_PS0903_TAB_NOTFOUND:SS42500_TBL_NOT_FOUND
"Requested table '%0c' was not found in the database."
E_PS0904_BAD_RDF_GETDESC:SS5000B_INTERNAL_ERROR
"RDF error occurred when getting description for an object."
E_PS0905_DBP_NOTFOUND:SS42505_OBJ_NOT_FOUND
"Requested procedure '%0c' was not found in the database."
E_PS0906_TOO_MANY_COLS:SS50002_LIMIT_EXCEEDED
"Result table has too many columns."
E_PS0907_CANT_FREE_DESC:SS5000B_INTERNAL_ERROR
"pstclrrng.c: Can't free a table description."
E_PS0908_RANGE_VAR_MISSING:SS5000B_INTERNAL_ERROR
"line %0d, Can't find range variable number %1d."
E_PS0909_NO_RANGE_ENTRY:SS5000B_INTERNAL_ERROR
"CREATE VIEW: Could not find range table entry for attribute associated
\nwith resdom '%0c'."
E_PS090A_NO_ATTR_ENTRY:SS5000B_INTERNAL_ERROR
"CREATE VIEW: Could not find attribute description for attribute associated
\nwith resdom '%0c'."
E_PS090B_TABLE_EXISTS:SS42502_DUPL_OBJECT
"Table '%0c' already exists in the database."
E_PS090C_NONEXISTENT_TABLE:SS42500_TBL_NOT_FOUND
"Table '%0c' does not exist in the database."
E_PS090D_NO_LDB_TABLE:SS42500_TBL_NOT_FOUND
"LDB table '%0c' does not exist
\n on node '%1c', database '%2c', dbms '%3c'."
E_PS090E_COL_MISMATCH:SS5000A_QUERY_ERROR
"Number of columns in LDB table is different from that specified for the LINK\n
object."
E_PS090F_BAD_COL_NAME:SS5000A_QUERY_ERROR
"LDB table column name '%0c' is not a valid identifier and can not be used as\n
a LINK object column name."
E_PS0910_DUP_LDB_TABLE:SS42502_DUPL_OBJECT
"LDB table '%0c' owned by you already exists on NODE '%1c', DATABASE '%2c'."
E_PS0911_BAD_LDB_USER:SS42503_INSUF_PRIV
"You are not an authorized user on NODE '%0c', DATABASE '%1c'."
E_PS0912_MISSING_LDB_TBL:SS42500_TBL_NOT_FOUND
"There is no LDB table corresponding to STAR object '%0c'."
E_PS0913_SCHEMA_MISMATCH:SS5000I_UNEXP_LDB_SCHEMA_CHNG
"Structure of LDB table corresponding to STAR object '%0c'
\nhas been changed since the time '%0c' was created."
W_PS0914_MISSING_LDB_TBL:SS01500_LDB_TBL_NOT_DROPPED
"DROP/DESTROY: LDB table has not been dropped.\n
There is no LDB table corresponding to STAR object '%0c'."
E_PS0915_MISSING_LDB_TBL:SS42500_TBL_NOT_FOUND
"There is no LDB table corresponding to STAR object
\nwith base id '%0d', index id '%1d'."
/*%
** Parameters:
**	%0d - base ID of a Star object
**	%1d - index ID of a Star object
**
** Explanation:
**	This message will be produced instead of PS0912, if a failure occurred
**	while trying to obtain a table description using its ID.
*/
E_PS0916_USER_NOT_OWNER:SS5000A_QUERY_ERROR
"DROP/DESTROY: You do not own LDB object corresponding
\nto STAR object '%0c'.
\nYou may use REMOVE to remove the registration."
E_PS0917_CANNOT_CONNECT:SS08500_LDB_UNAVAILABLE
"Can not connect
\nto node '%0c', database '%1c', dbms '%2c'."
E_PS0918_MULTI_SITE_DROP:SS5000A_QUERY_ERROR
"Table '%0c' was not dropped.
\n    Dropping it would violate single-site update requirement."
E_PS0919_BAD_CATALOG_ENTRY:SS50000_MISC_ERRORS
"Catalog entry for DDB object '%0c' is inconsistent.
\n Try 'REGISTER %0c AS LINK WITH REFRESH' or
\n REMOVE and then REGISTER %0c."
E_PS091A_CANNOT_CONNECT:SS5000A_QUERY_ERROR
"DROP/DESTROY: Cannot establish connection with the site
\nof the LDB table corresponding to STAR object '%0c'.
\nYou may use REMOVE to remove the registration."
E_PS091B_NO_CONNECTION:SS08500_LDB_UNAVAILABLE
"LDB is temporarily inaccessible or it does not exist."
E_PS091C_NOPARAMS_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"Parameters may not be specified with the register procedure statement"
E_PS091D_PROC_MAPPED_COLS:SS42000_SYN_OR_ACCESS_ERR
"It is illegal to map column names for registered procedures.\n
This occurred for procedure '%0c' (registered from NODE '%1c', DATABASE '%2c')."
E_PS091E_INCOMPAT_CASE_MAPPING:SS42000_SYN_OR_ACCESS_ERR
"Do not use a delimited identifier to specify the local object '%0c' on the
\nREGISTER statement.  The case translation semantics of delimited identifiers 
\nfor the Star database are incompatible with those for the local database."
E_PS091F_STAR_NO_FIRSTN:SS42000_SYN_OR_ACCESS_ERR
"The 'first n' syntax is NOT supported in Star queries."
E_PS0A01_BADBLKSIZE:SS5000B_INTERNAL_ERROR
"psf_mopen: bad block size"
E_PS0A02_BADALLOC:SS50003_EXHAUSTED_RESOURCE
"psf memory function couldn't allocate memory"
E_PS0A03_BADMEMTYPE:SS5000B_INTERNAL_ERROR
"psf_mopen: unknown memory type"
E_PS0A04_REQ_GT_BLOCK:SS5000B_INTERNAL_ERROR
"psf_malloc: request for memory greater than block size for stream"
E_PS0A05_BADMEMREQ:SS5000B_INTERNAL_ERROR
"PSF: bad memory request"
E_PS0A06_CANTFREE:SS5000B_INTERNAL_ERROR
"psf_mclose: can't free memory"
E_PS0A07_CANTCHANGE:SS5000B_INTERNAL_ERROR
"Can't change object type in QSF memory."
E_PS0A08_CANTLOCK:SS5000B_INTERNAL_ERROR
"Can't lock QSF object."
E_PS0A09_CANTDESTROY:SS5000B_INTERNAL_ERROR
"Can't destroy QSF object."
E_PS0A0A_CANTGETINFO:SS5000B_INTERNAL_ERROR
"Can't get info about a QSF object."
E_PS0A0B_CANTGETHNDLE:SS5000B_INTERNAL_ERROR
"Can't obtain QSF object handle."
E_PS0A0C_CANT_CREATE_ALIAS:SS5000B_INTERNAL_ERROR
"Can't create an alias for a QSF QP object."
E_PS0A0D_ULM_ERROR:SS5000B_INTERNAL_ERROR
"Memory allocation error (%0d from ULM) while parsing %1c."
E_PS0B04_CANT_GET_TEXT:SS5000B_INTERNAL_ERROR
"Failed attempt to get the query text from SCF."
E_PS0B05_CANT_UNLOCK:SS5000B_INTERNAL_ERROR
"Can't unlock the QSF object."
E_PS0B06_NOWHILE:SS5000A_QUERY_ERROR
"Internal PSF error: corresponding while node not found."
E_PS0BA0_SHORT_REMARK_TOO_LONG:SS42000_SYN_OR_ACCESS_ERR
"COMMENT: length of short remark may not exceed %0d characters."
/*%
** Parameters:
**	%0d - maximum length of short remark in COMMENT statement
**
** Explanation:
**	The length of the text of a short remark exceeds the maximum
**	legal length.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reenter the query with a shorter short remark.
*/
E_PS0BA1_LONG_REMARK_TOO_LONG:SS42000_SYN_OR_ACCESS_ERR
"COMMENT: length of long remark may not exceed %0d characters."
/*%
** Parameters:
**	%0d - maximum length of long remark in COMMENT statement
**
** Explanation:
**	The length of the text of a long remark exceeds the maximum
**	legal length.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reenter the query with a shorter long remark.
*/
E_PS0BA2_NO_REMARK_SPECIFIED:SS42000_SYN_OR_ACCESS_ERR
"COMMENT: Long remark must be specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The COMMENT query specified neither a long nor a short remark.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reenter the query with either a long remark, short remark, or both.
*/
E_PS0BA3_NONEXISTENT_COLUMN:SS42501_COL_NOT_FOUND
"COMMENT ON COLUMN: Table '%0c' does not contain column '%1c'."
/*%
** Parameters:
**	%0c - table name
**	%1c - column name
**
** Explanation:
**	The user attempted to create a comment on a non-existent column.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reenter the query with the name of an existent column.
*/
E_PS0BA4_TABLE_COMMENT_ERROR:SS50000_MISC_ERRORS
"COMMENT ON TABLE: Failed to create/modify comment on table '%0c'.
\n Please contact system administrator."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The COMMENT query failed due to some internal error.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Contact your Ingres Corporation system administrator.
*/
E_PS0BA5_COLUMN_COMMENT_ERROR:SS50000_MISC_ERRORS
"COMMENT ON COLUMN: Failed to create/modify comment
\non column '%0c' of table '%1c'.
\nPlease contact system administrator."
/*%
** Parameters:
**	%0c - column name
**	%1c - table name
**
** Explanation:
**	The COMMENT query failed due to some internal error.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Contact your system administrator.
*/
E_PS0BAF_SHORT_REM_UNSUPPORTED:SS50008_UNSUPPORTED_STMT
"COMMENT: Use of short remark is not supported in this release."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to specify a short remark that is not supported
**	in this release.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reenter the query, specifying only the long remark.
*/
E_PS0BB0_UNIQ_SCOPE_NOT_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"%0c: UNIQUE_SCOPE is not a valid WITH option for this statement.
It is only valid for CREATE INDEX or MODIFY."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	UNIQUE_SCOPE is only valid with a UNIQUE index or table order,
**	and UNIQUE can only be specified via the "CREATE INDEX" or
**	"MODIFY" commands.  Commands such as "CREATE TABLE", "DECLARE TEMP
**	TABLE", and "CREATE TABLE .. AS SELECT" will return this error.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement without the UNIQUE_SCOPE option.
*/
E_PS0BB1_UNIQ_SCOPE_SQL_ONLY:SS42000_SYN_OR_ACCESS_ERR
"%0c: UNIQUE_SCOPE is only allowed in SQL."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	UNIQUE_SCOPE is only allowed in SQL, not QUEL.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement without the UNIQUE_SCOPE option,
**	or re-issue the statement using SQL "CREATE INDEX".
*/
E_PS0BB2_UNIQUE_REQUIRED:SS42000_SYN_OR_ACCESS_ERR
"%0c: UNIQUE_SCOPE cannot be specified unless UNIQUE is also
specified."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	UNIQUE_SCOPE is only valid on a unique index or storage
**	structure.  The statement must specify the UNIQUE
**	option in order to specify the UNIQUE_SCOPE option.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement, adding the UNIQUE option,
**	or re-issue the statement without the UNIQUE_SCOPE option.
*/
E_PS0BB3_INVALID_UNIQUE_SCOPE:SS42000_SYN_OR_ACCESS_ERR
"%0c: Invalid value '%1c' for UNIQUE_SCOPE option.
Use either 'statement' or 'row.'"
/*%
** Parameters:
**	%0c - entered statement
**	%1c - valid value
**
** Explanation:
**	UNIQUE_SCOPE has only two valid values: "statement" and "row".
**	The user specified some other string value instead.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement, specifying either "statement" or "row".
*/
E_PS0BB4_CANT_CHANGE_UNIQUE_CONS:SS42000_SYN_OR_ACCESS_ERR
"%0c: You must specify 'UNIQUE' and WITH options
\n'UNIQUE_SCOPE = statement' and 'PERSISTENCE' for this %1c.
\nThe %1c '%2c' supports a unique constraint,
so these 3 properties cannot be changed."
/*%
** Parameters:
**	%0c - entered statement
**	%1c - "index" or "table"
**	%2c - table or index name
**
** Explanation:
**	Some UNIQUE indexes are created by the system to implement SQL92
**	UNIQUE and PRIMARY KEY constraints. Users can modify these indexes,
**	as necessary, to change their characteristics (e.g., from Btree to
**	ISAM), or to reorganize the index.  However, users cannot change the
**	UNIQUEness attributes or the PERSISTENCE of the index, as that could
**	cause the constraint to be violated.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Add the missing keywords and WITH options, and re-issue the statement.
*/
E_PS0BB5_NO_PERSIST_TABLE:SS42000_SYN_OR_ACCESS_ERR
"%0c: WITH [NO]PERSISTENCE can only be specified for indexes,
and '%1c' is a table."
/*%
** Parameters:
**	%0c - entered statement (MODIFY)
**	%1c - table name
**
** Explanation:
**	PERSISTENCE means that an index still exists after its base
**	table has been modified.  The index is automatically recreated
**	after the MODIFY finishes. Thus, this concept does not make sense
**	for base tables, and the [NO]PERSISTENCE option is not allowed for
**	base tables, only for indexes.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Remove the [NO]PERSISTENCE clause from the statement and reenter.
*/
E_PS0BB6_PERSISTENT_CONSTRAINT:SS42000_SYN_OR_ACCESS_ERR
"%0c: You must specify WITH option 'PERSISTENCE' for this %1c.
\nThe %1c '%2c' supports a constraint,
so this property cannot be changed."
/*%
** Parameters:
**	%0c - entered statement
**	%1c - "index" or "table"
**	%2c - table or index name
**
** Explanation:
**	Some indexes are created by the system to implement SQL92
**	FOREIGN KEY constraints. Users can modify these indexes,
**	as necessary, to change their characteristics (e.g., from Btree to
**	ISAM), or to reorganize the index.  However, users cannot change the
**	PERSISTENCE of the index, as that could cause the constraint to be 
**      violated.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
*/
E_PS0BC0_BTREE_INDEX_NO_DCOMP:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c: Btree secondary indices do not support data compression."
/*%
** Parameters:
**	%0c - statement executed (either MODIFY or CREATE INDEX)
**	%1c - table name
**
** Explanation:
**	This message is issued when a "MODIFY" or "CREATE INDEX" statement
**	requests that a secondary index is to be modified to a Btree
**	storage structure and also requests that data compression is
**	to be used. This is illegal, since Btree secondary indexes
**	do not support data compression.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the statement (either do not request data compression or choose
**	another storage structure) and re-issue the statement.
*/
E_PS0BC1_KCOMP_BTREE_ONLY:SS42000_SYN_OR_ACCESS_ERR
"The %1c option specified KEY compression for table/index %0c,
\nbut KEY compression is only available for the Btree storage structure."
/*%
** Parameters:
**	%0c - table name
**	%1c - compression option name
**
** Explanation:
**	This message is issued when a "CREATE TABLE", "MODIFY", or "CREATE
**	INDEX" statement requests that KEY compression be used, but the storage
**	structure in question is not a Btree storage structure.
**
**	Currently, only the Btree storage structure supports KEY compression.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the statement (either do not request KEY compression or specify
**	the Btree storage structure) and re-issue the statement.
*/
E_PS0BC2_COMP_NOT_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"The WITH COMPRESSION clause is not allowed in the MODIFY TO MERGE,
\nMODIFY TO TRUNCATED, or MODIFY TO REORGANIZE forms of the MODIFY statement."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued when a "MODIFY TO MERGE", "MODIFY TO TRUNCATED",
**	or "MODIFY TO REORGANIZE" statement specifies the "WITH COMPRESSION"
**	clause. This clause is not valid on these special-purpose forms of the
**	"MODIFY" statement.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Remove the "WITH COMPRESSION" clause, and re-issue the statement.
*/
E_PS0BC3_COMP_NOT_IN_RELOC:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Line %0d: the WITH COMPRESSION clause is not allowed in the
\nMODIFY TO RELOCATE form of the MODIFY statement."
/*%
** Parameters:
**	%0d - line number
**
** Explanation:
**	This message is issued when a "MODIFY TO RELOCATE" statement
**	specifies the "WITH COMPRESSION" clause. This clause is not valid
**	on this special-purpose form of the "MODIFY" statement.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Remove the "WITH COMPRESSION" clause, and re-issue the statement.
*/
E_PS0BC4_COMPRESSION_TWICE:SS42000_SYN_OR_ACCESS_ERR
"%0c: Line %1d: compression options may not be specified more than once."
/*%
** Parameters:
**	%0c - statement name ("MODIFY", "CREATE INDEX", etc.)
**	%1d - line number
**
** Explanation:
**	This message is issued when a "MODIFY", "CREATE INDEX", or
**	"CREATE TABLE ... AS SELECT" statement specifies compression
**	options more than once. Compression options may be specified
**	using the "WITH COMPRESSION" clause, or they may be specified
**	using a "C" in front of the storage structure name (e.g., "CISAM"),
**	but the compression options may only be specified *once in the
**	statement.
**
**	You may not provide the "WITH COMPRESSION" clause more than once in a
**	given statement, nor may you use the "WITH COMPRESSION" clause if you
**	have also used a "C" in front of the storage structure name.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Remove the extra compression options, and re-issue the statement.
*/
E_PS0BC5_KEY_OR_DATA_ONLY:SS42000_SYN_OR_ACCESS_ERR
"%0c: Line %1d: COMPRESSION= clause must have only
\nKEY, NOKEY, DATA, or NODATA, not %2c."
/*%
** Parameters:
**	%0c - statement name ("MODIFY", "CREATE INDEX", etc.)
**	%1d - line number
**	%2c - invalid value
**
** Explanation:
**	This message is issued when a "MODIFY", "CREATE INDEX", or
**	"CREATE TABLE ... AS SELECT" statement specifies an invalid
**	keyword in the "WITH COMPRESSION=" clause.
**
**	The only valid words in the "COMPRESSION=" clause are "KEY", "NOKEY",
**	"DATA", or "NODATA".
**
**	NODEPENDENCY_CHECK is only valid for a MODIFY statement.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct any typographical errors and remove any invalid keywords,
**	then re-issue the statement.
*/
E_PS0BC6_BAD_COMP_CLAUSE:SS42000_SYN_OR_ACCESS_ERR
"%0c: Line %1d, %2c is not a valid option for WITH clause. Valid single-keyword WITH options in a create statement are COMPRESSION, NOCOMPRESSION, NOPARTITION, PERSISTENCE or NOPERSISTENCE. The valid options for modify statement are all the above and NODEPENDENCY_CHECK."
/*%
** Parameters:
**	%0c - statement name ("MODIFY", "CREATE INDEX", etc.)
**	%1d - line number
**	%2c - invalid value
**
** Explanation:
**	This message is issued when a "MODIFY", "CREATE INDEX", or
**	"CREATE TABLE ... AS SELECT" statement specifies an invalid
**	single-keyword option.  The WITH clause must specify one of
**	[NO]COMPRESSION, NOPARTITION, or [NO]PERSISTENCE, instead of
**	the value printed in the message.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the WITH clause, and re-issue the statement.
*/
E_PS0BC7_NO_STRUCTURE:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Line %0d: Internal error -- storage structure not known when
\nCOMPRESSION clause encountered."
/*%
** Parameters:
**	%0d - line number
**
** Explanation:
**	This message is logged when an internal error is encountered in the
**	parser. The error is that parsing of the "WITH COMPRESSION" clause was
**	halted, because no storage structure could be found for the table or
**	index being modified. The storage structure must be known in order for
**	the parsing to be successfully completed, so the statement is rejected.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Please report this error to Ingres Corporation Technical Support. 
**	Save the error log file for analysis by Technical Support personnel. If
**	possible, determine the "MODIFY" statement which caused the error --
**	this information will be valuable for diagnosing the problem.
*/
E_PS0BC8_COMP_NO_STRUCTURE:SS42000_SYN_OR_ACCESS_ERR
"%0c: Line %1d: specification of COMPRESSION options requires
\nspecification of STRUCTURE as well."
/*%
** Parameters:
**	%0c - statement name ("CREATE INDEX" or "CREATE TABLE")
**	%1d - line number
**
** Explanation:
**	The "WITH COMPRESSION" clause on the "CREATE INDEX" and
**	"CREATE TABLE ... AS SELECT" statements can only be provided
**	if the "STRUCTURE=" clause is also provided.
**
**	This restriction exists because the meaning of the COMPRESSION clause
**	depends on the storage structure of the table or index. Some storage
**	structures, such as HASH and HEAP, do not support KEY compression.
**	Other storage structures, such as a Btree secondary index, do not
**	support DATA compression. Therefore, the desired storage structure
**	must be specified in order to clarify the meaning of the COMPRESSION
**	options.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Add the "STRUCTURE=" clause to the statement and re-issue.
*/
E_PS0BC9_DUPLICATE_WITH_CLAUSE:SS42000_SYN_OR_ACCESS_ERR
"%0c: Line %1d: %2c options may not be specified more than once."
/*%
** Parameters:
**	%0c - statement name ("MODIFY", "CREATE INDEX", etc.)
**	%1d - line number
**	%2c - WITH clause that is being rejected
**
** Explanation:
**	This message is issued when a "MODIFY", "CREATE INDEX", or
**	"CREATE TABLE ... AS SELECT" statement specifies a particular
**	WITH clause more than once.
**
**	Each WITH clause in the statement should be specified at most once.
**
**	Compression options may be specified using the "WITH COMPRESSION"
**	clause, or they may be specified using a "C" in front of the storage
**	structure name (e.g., "CISAM"), but the compression options may only
**	be specified once in the statement.
**
**	You may not provide the "WITH COMPRESSION" clause more than once in a
**	given statement, nor may you use the "WITH COMPRESSION" clause if you
**	have also used a "C" in front of the storage structure name.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Remove the extra WITH clause options, and re-issue the statement.
*/
E_PS0BCA_RTREE_INDEX_NO_COMP:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c: Rtree secondary indices do not support compression."
/*%
** Parameters:
**	%0c - statement executed (CREATE INDEX)
**	%1c - table name
**
** Explanation:
**	This message is issued when a "CREATE INDEX" statement
**	requests that a secondary index is a Rtree storage structure
**	and also requests that data compression is to be used.
**	This is illegal, since Rtree indexes do not support compression.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the statement (either do not request data compression or choose
**	another storage structure) and re-issue the statement.
*/
E_PS0BCB_RTREE_INDEX_NO_UNIQUE:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c: Rtree secondary indices do not support unique keys."
/*%
** Parameters:
**	%0c - statement executed (CREATE INDEX)
**	%1c - table name
**
** Explanation:
**	This message is issued when a "CREATE INDEX" statement
**	requests that a secondary index is a Rtree storage structure
**	and also requests that UNIQUE keys are to be used.
**	This is illegal, since Rtree indexes do not support unique keys.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the statement (either do not request unique keys or choose
**	another storage structure) and re-issue the statement.
*/
E_PS0BCC_RTREE_INDEX_ONE_KEY:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c: Rtree secondary indices only support one column as a key."
/*%
** Parameters:
**	%0c - statement executed (CREATE INDEX)
**	%1c - table name
**
** Explanation:
**	This message is issued when a "CREATE INDEX" statement
**	requests that a secondary index is a Rtree storage structure
**	and also requests that the key is to be created from more than
**	one column.  This is illegal, since Rtree indexes
**	only support a single column as key.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the statement (either do not request multiple columns or choose
**	another storage structure) and re-issue the statement.
*/
E_PS0BCD_RTREE_INDEX_NEED_RANGE:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c: Rtree secondary indices require a range specification."
/*%
** Parameters:
**	%0c - statement executed (CREATE INDEX)
**	%1c - table name
**
** Explanation:
**	This message is issued when a "CREATE INDEX" statement
**	requests that a secondary index is a Rtree storage structure
**	and has omitted the range specification.  An Rtree index
**	requires a range specification.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the statement by adding the range of the key field
**	and re-issue the statement.
*/
E_PS0BCE_RTREE_INDEX_BAD_KEY:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c: Rtree secondary key must be a spatial data type."
/*%
** Parameters:
**	%0c - statement executed (CREATE INDEX)
**	%1c - table name
**
** Explanation:
**	This message is issued when a "CREATE INDEX" statement
**	requests that a secondary index is a Rtree storage structure
**	and specifies a key that is not supported by the spatial function
**	"nbr".  The column must be supported by the "nbr" function for
**      an Rtree secondary index.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**      Pick a spatial datatype for the rtree secondary index key
**	and re-issue the statement.
*/
E_PS0BCF_RTREE_INDEX_BAD_RANGE:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c: Rtree range is specificed incorrectly."
/*%
** Parameters:
**	%0c - statement executed (CREATE INDEX)
**	%1c - table name
**
** Explanation:
**	This message is issued when a "CREATE INDEX" statement
**	requests that a secondary index is a Rtree storage structure
**	and the range is specified incorrectly.  There must be 2 *
**	dimension(base type) entries.  The first set of coordinates
**	must be less than the second.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Correct the statement by adjusting the range values
**	and re-issue the statement.
*/
E_PS0BD0_MUST_USE_NORECOV:SS42000_SYN_OR_ACCESS_ERR
"%0c: To create a lightweight session table, the WITH NORECOVERY clause
must be specified. Re-issue the '%0c' statement and append the WITH NORECOVERY
clause."
/*%
** Parameters:
**	%0c - "DECLARE GLOBAL TEMPORARY TABLE"
**
** Explanation:
**	The "WITH NORECOVERY" clause is required when creating a lightweight
**	session table. This clause may not be omitted.
**
** System Status:
**	The "DECLARE GLOBAL TEMPORARY TABLE" statement is rejected.
**
** Recommendation:
**	Re-issue the "DECLARE GLOBAL TEMPORARY TABLE" statement, and append
**	the "WITH NORECOVERY" clause.
*/
E_PS0BD1_MUST_USE_SESSION:SS42000_SYN_OR_ACCESS_ERR
"%0c: The lightweight session table name must be qualified with the SESSION
keyword. You must specify the SESSION schema name in order to use the '%0c'
statement. To create a lightweight session table, re-issue the '%0c'
statement and prefix the table name with the SESSION schema name."
/*%
** Parameters:
**	%0c - "DECLARE GLOBAL TEMPORARY TABLE"
**
** Explanation:
**	All references to lightweight session tables must be prefixed
**	with the "SESSION" keyword. This includes the "DECLARE GLOBAL
**	TEMPORARY TABLE" statement itself. To create a lightweight session
**	table named "temp", for example, the statement should be specified as:
**
**	    DECLARE GLOBAL TEMPORARY TABLE SESSION.temp ( ... )
**	        ON COMMIT PRESERVE ROWS WITH NORECOVERY...
**
**	If you want to create a permanent table, rather than a temporary
**	table, then you must issue the "CREATE TABLE" statement.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the "DECLARE GLOBAL TEMPORARY TABLE" statement, specifying the
**	"SESSION" keyword as a prefix.
*/
E_PS0BD2_NOT_SUPP_ON_TEMP:SS42000_SYN_OR_ACCESS_ERR
"%0c: This operation is not supported against lightweight session tables."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	Not all operations are valid against lightweight session tables. Some
**	operations cannot be performed. For example, since a lightweight
**	session table is visible only to the session that created it, the
**	"GRANT" command cannot be issued against the table.
**
**	This message indicates that you have attempted an operation that is
**	not supported for use with a lightweight session table.
**
** System Status
**	The statement is rejected.
**
** Recommendation:
**	You cannot issue this statement against a lightweight session table.
*/
E_PS0BD3_ONLY_ON_TEMP:SS42000_SYN_OR_ACCESS_ERR
"%0c: '%1c' is only supported for lightweight session tables."
/*%
** Parameters:
**	%0c - entered statement
**	%1c - invalid part of the statement
**
** Explanation:
**	Some operations are only valid against lightweight session tables. For
**	example, the "WITH NORECOVERY" clause is not valid on a permanent table.
**
**	This message indicates that you have attempted an operation which is
**	not supported for use with a permanent table.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	You cannot issue this statement against a permanent table.
*/
E_PS0BD4_TTBL_NOT_IN_VIEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE VIEW: lightweight session tables may not be referenced within the query
expression of a view. The table '%0c' is a lightweight session table, and hence
it cannot be used in this view."
/*%
** Parameters:
**	%0c - lightweight session table name
**
** Explanation:
**	Currently, lightweight session tables may not be used in views. That
**	is, you may not define a view whose query expression references a
**	lightweight session table.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the "CREATE VIEW" statement, referencing only permanent
**	tables.
*/
E_PS0BD5_TTBL_NOT_IN_DBP:SS42000_SYN_OR_ACCESS_ERR
"CREATE PROCEDURE: lightweight session tables may not be referenced within a
database procedure. The table '%0c' is a lightweight session table, and hence it
cannot be used in this database procedure."
/*%
** Parameters:
**	%0c - lightweight session table name
**
** Explanation:
**	Currently, lightweight session tables may not be used in database
**	procedures.
**
**	The body of a database procedure may not contain any references to
**	lightweight session tables, and lightweight session tables may not be
**	created within a database procedure.
**
** System Status:
**	The database procedure definition is rejected.
**
** Recommendation:
**	Re-issue the "CREATE PROCEDURE" statement, referencing only permanent
**	tables.
*/
E_PS0BD6_MUST_USE_ON_COMMIT:SS42000_SYN_OR_ACCESS_ERR
"%0c: The ON COMMIT PRESERVE ROWS clause must be provided.
To create a lightweight session table, re-issue the '%0c'
statement and provide the ON COMMIT PRESERVE ROWS clause."
/*%
** Parameters:
**	%0c - "DECLARE GLOBAL TEMPORARY TABLE"
**
** Explanation:
**	All references to lightweight session tables must be specified with
**	the "ON COMMIT PRESERVE ROWS" clause.
**
**	To create a lightweight session table named "temp", for example,
**	the statement should be specified as:
**
**	    DECLARE GLOBAL TEMPORARY TABLE SESSION.temp ( ... )
**	        ON COMMIT PRESERVE ROWS WITH NORECOVERY...
**
**	If you wanted to create a permanent table, rather than a temporary
**	table, you must issue the "CREATE TABLE" statement.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the "DECLARE GLOBAL TEMPORARY TABLE" statement, specifying the
**	"ON COMMIT PRESERVE ROWS" clause.
*/
E_PS0BD7_SESSION_EV_ILLEGAL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Dbevent name may not be prefixed with keyword \"SESSION\"."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	The name of a dbevent in the statement was prefixed with "SESSION".
**	"SESSION" must be used to prefix names of declared global temporary
**	tables, but may not be used with other types of DBMS objects.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement, taking care to not prefix a dbevent name with
**	the "SESSION" keyword.
*/
E_PS0BD8_SESSION_INDEX_ILLEGAL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Index name may not be prefixed with keyword \"SESSION\"."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	The name of an index in the statement was prefixed with "SESSION".
**	"SESSION" must be used to prefix names of declared global temporary
**	tables, but may not be used with other types of DBMS objects.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement, taking care to not prefix an index name with
**	the "SESSION" keyword.
*/
E_PS0BD9_SESSION_DBP_ILLEGAL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Database procedure name may not be prefixed with keyword \"SESSION\"."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	The name of a database procedure in the statement was prefixed with
**	"SESSION".  "SESSION" must be used to prefix names of declared global
**	temporary tables, but may not be used with other types of DBMS objects.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement, taking care to not prefix a database procedure
**	name with the "SESSION" keyword.
*/
E_PS0BDA_SESSION_SEQ_ILLEGAL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Sequence name may not be prefixed with keyword \"SESSION\"."
/*%
** Parameters:
**	%0c - entered statement
**
** Explanation:
**	The name of a sequence in the statement was prefixed with
**	"SESSION".  "SESSION" must be used to prefix names of declared global
**	temporary tables, but may not be used with other types of DBMS objects.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Re-issue the statement, taking care to not prefix a sequence
**	name with the "SESSION" keyword.
*/
E_PS0BE0_TOO_MANY_COLS:SS50002_LIMIT_EXCEEDED
"%0c: Specified number of columns (%1d) exceeds the maximum limit of %2d."
/*%
** Parameters:
**	%0c - entered statement (one of: "CREATE TABLE AS SELECT",
**	      "CREATE VIEW", "DGTT AS SELECT")
**	%1d - number of columns specified in the statement
**	%2d - maximum legal number of columns 
**
** Explanation:
**	The user attempted to create a table, a temporary table, or a view
**	with more columns than is permitted.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0BE1_TUPLE_TOO_WIDE:SS50002_LIMIT_EXCEEDED
"%0c: Width of tuple (%1d) exceeds the maximum limit of %2d."
/*%
** Parameters:
**	%0c - entered statement (one of: "SELECT", "CREATE TABLE AS SELECT",
**	      "CREATE VIEW", "DGTT AS SELECT")
**	%1d - width of tuple specified in the statement
**	%2d - maximum legal width of a tuple
**
** Explanation:
**	The width of a tuple represented by the target list of the outermost
**	subselect exceeds the maximum legal width.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_PS0C02_NULL_PTR:SS5000B_INTERNAL_ERROR
"Unexpected NULL pointer in query tree."
E_PS0C03_BAD_NODE_TYPE:SS5000B_INTERNAL_ERROR
"Unknown node type in query tree."
E_PS0C04_BAD_TREE:SS5000B_INTERNAL_ERROR
"Bad query tree."
E_PS0C05_BAD_ADF_STATUS:SS5000B_INTERNAL_ERROR
"Unexpected status from ADF."
E_PS0C06_BAD_STMT:SS5000B_INTERNAL_ERROR
"Invalid PST_STATEMENT node - '%0c' node corrupt or missing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When reconstructing the rule statement tree from internal data
**	structures, an invalid relationship between statement nodes was found.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	If this is a Dynamic SQL statement, try to re-PREPARE the statement;
**	otherwise, if this statement has a rule associated with it, your rule
**	definition may have been corrupted.  Contact your system
**	administrator and request that they save the current "rule row" in
**	iirule, together with the associated objects in iitree and iiqrytext.
**	Recreate the rule and try again.
*/
E_PS0C80_NO_DEFLT_ON_DT:SS42000_SYN_OR_ACCESS_ERR
"No default values are provided for the decimal() conversion function when a\
 value with datatype `%0c' is given as the first parameter.  Explicitly\
 specify precision and scale."
/*%
** Parameters:
**	%0c - datatype of value passed to decimal() conversion function
**
** Explanation:
**	Currently, only float, integer, decimal, and money datatypes are
**	assigned default precision and scale values when using the decimal()
**	conversion function.  Receiving this error means that a value with
**	some other datatype was passed as the first parameter of the decimal()
**	function with the precision or scale omitted.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the usage of the decimal() conversion function by either
**	changing the datatype of the first parameter to a type that provides
**	default values for precision and scale, or explicitly provide precision
**	and scale values in your decimal() function call.
*/
E_PS0C81_MUST_BE_CONST:SS42000_SYN_OR_ACCESS_ERR
"The second and third arguments to the decimal() conversion function must be\
 constants.  Column names or host variables, etc. are not allowed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The second and third arguments to the decimal() conversion function
**	must be constants.  You are not allowed to use a column name from a
**	table, a host variable (regardless of its storage class in the host
**	language), or repeat query parameters.  For example,
**	decimal(123, 10, 2) is legal while decimal(123, :prec, 2) is illegal.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the usage of the decimal() conversion function by using only
**	constants for the precision and scale arguments to the function.
*/
E_PS0C82_BAD_PRECISION:SS50002_LIMIT_EXCEEDED
"Precision specified in decimal() conversion function was invalid.  Precision\
 must be a constant between 1 and 39, inclusive."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The precision specified as the second parameter to the decimal()
**	conversion function was either not an integer literal, or was outside
**	the legal range (1 <= precision <= 39).  You may not specify any type
**	other than integer; decimal, and float literals are illegal.
**	For example, decimal(123, 10, 2) is legal, but decimal(123, 10.0, 2)
**	is illegal.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Change the usage of the decimal() conversion function to conform to
**	the rules stated above.
*/
E_PS0C83_BAD_SCALE:SS50002_LIMIT_EXCEEDED
"Scale specified in decimal() conversion function was invalid.  Scale\
 must be a constant between 0 and `precision' (in this case\
 %0d), inclusive."
/*%
** Parameters:
**	%0d - precision given in this decimal() function invocation
**
** Explanation:
**	The scale specified as the third parameter to the decimal()
**	conversion function was either not an integer literal, or was
**	outside the legal range (1 <= scale <= precision).  You may not
**	specify any type other than integer; decimal, and float
**	literals are illegal. For example, decimal(123, 10, 2) is legal,
**	but decimal(123, 10, 2.0) is illegal.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Change the usage of the decimal() conversion function to conform to
**	the rules stated above.
*/
E_PS0C84_TERNARY_FUNC:SS42000_SYN_OR_ACCESS_ERR
"Only decimal() conversion function may be passed 3 arguments."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user passed three arguments to a function other than decimal().
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Either change the function name to decimal or get rid of an extra
**	parameter.
*/
E_PS0C90_MUSTBE_CONST_INT:SS42000_SYN_OR_ACCESS_ERR
"The second argument to the char(), varchar(), ascii() and text() conversion\
 function must be constant integer.  Expressions, column names, host\
 variables, etc. are not allowed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Currently, only an integer constant is allowed for the second
**	argument to the varchar(), char(), text() and ascii() functions.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the usage of the function.
*/
E_PS0C91_MUSTBE_NUMERIC:SS42000_SYN_OR_ACCESS_ERR
"Function %0c requires argument %0d to be numeric."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Substring From and For must be numeric value expression.
**	Other function also require numeric value expressions.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Correct the usage of the function.
*/
E_PS0D01_BAD_TREE_IN_VFIND:SS5000B_INTERNAL_ERROR
"A bad query tree was detected in psy_vfind."
E_PS0D02_BAD_TID_NODE:SS5000B_INTERNAL_ERROR
"An illegal TID node was discovered."
E_PS0D03_ATT_NOT_FOUND:SS5000B_INTERNAL_ERROR
"The view attribute was not found in the view tree."
E_PS0D05_NULL_TREE:SS5000B_INTERNAL_ERROR
"A NULL tree was found in psy_apql."
E_PS0D06_QUAL_NOT_NORML:SS5000B_INTERNAL_ERROR
"An un-normalized qualification was discovered."
E_PS0D07_VARNO_OUT_OF_RANGE:SS5000B_INTERNAL_ERROR
"An out of sequence variable number was found."
E_PS0D08_RNGVAR_UNUSED:SS5000B_INTERNAL_ERROR
"An attempt was made to use an undefined range variable."
E_PS0D09_MRG_DIFF_TABS:SS5000B_INTERNAL_ERROR
"An attempt was made to merge two different tables."
E_PS0D0A_CANT_GET_EMPTY:SS5000B_INTERNAL_ERROR
"Failed call to get an empty datatype from ADF."
E_PS0D0B_RESDOM_EXPECTED:SS5000B_INTERNAL_ERROR
"A non-resdom node was found where a resdom node was expected."
E_PS0D0C_NOT_RESDOM:SS5000B_INTERNAL_ERROR
"A non-resdom node was found."
E_PS0D0D_NULL_LHS:SS5000B_INTERNAL_ERROR
"A NULL target list was found where one was not expected."
E_PS0D0E_BAD_QMODE:SS5000B_INTERNAL_ERROR
"An illegal query mode was discovered."
E_PS0D0F_BAD_AGGREGATE:SS5000B_INTERNAL_ERROR
"An illegal node was discovered in an aggregate tree fragment."
E_PS0D10_BAD_QUALIFICATION:SS5000B_INTERNAL_ERROR
"An illegal node was discovered in the qualification."
E_PS0D11_NULL_TREE:SS5000B_INTERNAL_ERROR
"An unexpected NULL query tree was found."
E_PS0D12_BYHEAD_OR_AOP:SS5000B_INTERNAL_ERROR
"An unexpected AGHEAD or AOP node was found in query tree."
E_PS0D13_SCU_INFO_ERR:SS5000B_INTERNAL_ERROR
"Failed attempt to get information from SCU."
E_PS0D14_VIEW_CLEANUP:SS5000B_INTERNAL_ERROR
"An error occurred while cleaning up a view definition."
E_PS0D15_RDF_GETINFO:SS5000B_INTERNAL_ERROR
"RDF error occurred when getting information on an object."
E_PS0D16_RDF_UPDATE:SS5000B_INTERNAL_ERROR
"Error updating object in RDF."
E_PS0D17_RDF_UNFIX:SS5000B_INTERNAL_ERROR
"Error unfixing object in RDF."
E_PS0D18_QSF_LOCK:SS5000B_INTERNAL_ERROR
"Error getting QSF lock on object."
E_PS0D19_QSF_INFO:SS5000B_INTERNAL_ERROR
"Error getting QSF information."
E_PS0D1A_QSF_DESTROY:SS5000B_INTERNAL_ERROR
"Error destroying QSF object."
E_PS0D1B_PERMIT_CLEANUP:SS5000B_INTERNAL_ERROR
"An error occurred cleaning up permit definition."
E_PS0D1C_QSF_UNLOCK:SS5000B_INTERNAL_ERROR
"Error unlocking QSF object."
E_PS0D1D_BAD_FUNC_NAME:SS42000_SYN_OR_ACCESS_ERR
"An illegal aggregate function name was specified."
E_PS0D1E_QEF_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred calling QEF while defining integrity."
E_PS0D1F_OPF_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred calling OPF while defining integrity."
E_PS0D20_QEF_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred calling QEF while destroying view or table."
E_PS0D21_QEF_ERROR:SS40001_SERIALIZATION_FAIL
"Access conflict occurred while dropping table - your previous activity
resulted in acquiring a lock that is still being held."
E_PS0D22_QEF_ERROR:SS5000B_INTERNAL_ERROR
"Internal transaction error occurred while executing QRYMOD."
E_PS0D23_QEF_ERROR:SS50003_EXHAUSTED_RESOURCE
"Resource related error occurred while executing QRYMOD. Try again later,
if problem persists consult your system administrator."
E_PS0D24_BGN_TX_ERR:SS50000_MISC_ERRORS
"Error beginning transaction during QRYMOD operation."
E_PS0D25_CMT_TX_ERR:SS50000_MISC_ERRORS
"Error committing transaction during QRYMOD operation."
E_PS0D26_ABT_TX_ERR:SS50000_MISC_ERRORS
"Error aborting transaction during QRYMOD operation."
E_PS0D27_RDF_INITIALIZE:SS5000B_INTERNAL_ERROR
"Error occurred while initializing RDF facility."
E_PS0D28_RDF_TERMINATE:SS5000B_INTERNAL_ERROR
"Error occurred while terminating RDF facility."
E_PS0D29_RDF_INVALIDATE:SS5000B_INTERNAL_ERROR
"Error occurred while invalidating a descriptor in the RDF cache."
E_PS0D2A_RDF_READTUPLES:SS5000B_INTERNAL_ERROR
"Error occurred while RDF was reading tuples from the data dictionary."
/*%
** Parameters:
**	None.
**
** Explanation:
**	PSF called RDF to read tuples from one of the DBMS catalogs, and RDF
**	received an error.  RDf will have logged its error in the 
**	error log.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Look in the error log to determine the RDF error.  Take the
**	appropriate corrective action based on the RDF error.
*/
E_PS0D30_UNEXPECTED_NODE:SS5000B_INTERNAL_ERROR
"Unexpected node found by protection checking routine."
E_PS0D31_NO_TUPLES:SS50000_MISC_ERRORS
"No tree tuples retrieved for a QUERYMOD object due to concurrent
activity. Try again later."
E_PS0D32_RDF_INV_NODE:SS5000B_INTERNAL_ERROR
"A parse tree node being duplicated has an invalid length;
\nExpected length: %0d, node length: %1d;  node type: %2d."
/*%
** Parameters:
**	%0d - computed node length
**	%1d - actual length in pst_len
**	%2d - Node type code
**
** Explanation:
**	An invalid parse tree node length was encountered.
**	If the query involves query-tree objects such as views or
**	rules, this may indicate a problem with RDF.  In any
**	case, this is an internal error and should be reported
**	to Tech Support.
**
** System Status:
**	The query is aborted;  the session continues.
**
** Recommendation:
**	Report this to Tech Support.
**	If the query involves views or rules, try dropping and recreating
**	them and retrying the query.
*/
E_PS0D33_RULE_PARAM:SS5000A_QUERY_ERROR
"Procedure argument counter/argument list mismatch in rule tuple/tree."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When reconstructing the rule statement tree from internal RDF tuple
**	and tree definitions, an inconsistency was found between the number
**	of parameters in the master rule tuple and the first node of the
**	parameter list tree.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Contact your system administrator and request that they save the
**	current "rule row" in iirule, together with the associated objects in
**	iitree and iiqrytext.  Recreate the rule and try again.
*/
E_PS0D34_EVENT_ABSENT_PRIV:SS42505_OBJ_NOT_FOUND
"%0c DBEVENT: Dbevent '%1c' does not exist or you do not
\nhave permission to execute this statement."
/*%
** Parameters:
**	%0 - "REGISTER", "REMOVE", or "RAISE DBEVENT" statement.
**	%1 - dbevent name
**
** Explanation:
**	The named dbevent does not exist or the current user does not have
**	permission to execute the statement. This could result from the dbevent
**	being dropped (explicitly or by a transaction rollback), from
**	insufficient privileges, or from the revoking of previous privileges.
**	Note that when you REGISTER or RAISE from within a database procedure
**	you are assigned the privileges of the owner of the procedure.
**
** System Status:
**	The current statement is aborted.  If this statement is issued from
**	within a database procedure, the database procedure compilation is
**	aborted.
**
** Recommendation:
**	Create the dbevent or, if it is already created, request that the
**	correct execution privileges be granted to you.
*/
E_PS0D35_EVENT_GRANT_REVOKE:SS42000_SYN_OR_ACCESS_ERR
"GRANT/REVOKE: Only REGISTER and RAISE privileges may be associated with
\na DBEVENT.  These privileges may not be associated with any other objects."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The only privileges that may be granted on or revoked from a dbevent
**	are REGISTER and RAISE.  Additionally, those privileges may only apply
**	to dbevents and not to tables, procedures, databases, or other objects.
**
** System Status:
**	The current GRANT or REVOKE statement is aborted.
**
** Recommendation:
**	Correct the statement to fit the syntax.
*/
E_PS0D36_EVENT_TEXT_QUOTE:SS42000_SYN_OR_ACCESS_ERR
"RAISE DBEVENT text identified by unquoted name '%0c'
\noutside of database procedure.  The text value must be a quoted string
\nor database procedure variable."
/*%
** Parameters:
**	%0c - name used
**
** Explanation:
**	Outside of a database procedure, you may only use a quoted string.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Correct the statement syntax.
*/
E_PS0D37_EVENT_TEXT_TYPE:SS42000_SYN_OR_ACCESS_ERR
"RAISE DBEVENT text parameter '%0c' must be of a non-nullable character type."
/*%
** Parameters:
**	%0c - procedure variable used for text
**
** Explanation:
**	The text field for the "RAISE DBEVENT" statement must be a
**	non-nullable or varying-length character value.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "RAISE DBEVENT" statement with the correct data type.
*/
E_PS0D38_EVENT_WITH:SS42000_SYN_OR_ACCESS_ERR
"RAISE DBEVENT: Invalid word '%0c' in WITH clause
\nof statement.  Only [NO]SHARE is allowed."
/*%
** Parameters:
**	%0c - invalid word
**
** Explanation:
**	The WITH clause syntax is incorrect.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Correct the syntax.
*/
E_PS0D39_EVENT_GET:SS5000B_INTERNAL_ERROR
"Invalid request to retrieve dbevent data.  The PSF routine '%0c' has
\nbeen called with statement mode (psq_mode) %1d."
/*%
** Parameters:
**	%0c - internal name of parser routine
**	%1c - invalid query mode
**
** Explanation:
**	A call has been made to retrieve a dbevent tuple in the wrong context.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Try the statement again, and submit an error report.
*/
E_PS0D3A_ILLEGAL_EVENT_REF:SS42503_INSUF_PRIV
"%0c: You may not apply this query to
\n dbevent '%1c' owned by '%2c'."
/*%
** Parameters:
**	%0c - entered query
**	%1c - dbevent name
**	%2c - dbevent owner
**
** Explanation:
**	The user attempted to execute a query explicitly referencing a dbevent
**	that may not be referenced by this user.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_PS0D3B_DBEVENT_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"%0c: Dbevent '%1c' does not exist."
/*%
** Parameters:
**	%0c - "REGISTER", "REMOVE", "RAISE DBEVENT", or "GRANT ON DBEVENT"
**	%1c - dbevent name
**
** Explanation:
**	The named dbevent does not exist.  If ev_schema.ev_name was used
**	to reference the dbevent, then it will be displayed; otherwise,
**	an unqualified dbevent name will be displayed.
**
** System Status:
**	The current statement is aborted.  If this statement is issued from
**	within a database procedure, the database procedure compilation is
**	aborted. If the statement was "GRANT ON DBEVENT", we will proceed to
**	the next dbevent named in the statement.
**
** Recommendation:
**	Ensure that the dbevent exists.
*/
E_PS0D3C_MISSING_DBEVENT_PRIV:SS42503_INSUF_PRIV
"%0c: you lack '%1c' on dbevent '%2c' owned by '%3c'."
/*%
** Parameters:
**	%0c - dbevent operation
**	%1c - privilege(s) that the user lacks on the dbevent
**	%2c - dbevent name
**	%3c - dbevent owner
**
** Explanation:
**	The user attempted to perform an operation involving a dbevent owned by
**	another user while lacking privilege(s) required for the operation.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Ensure that the user (or the owner of the database procedure where the
**	dbevent is used) possesses privileges required for the operation.
*/
E_PS0D3D_MISSING_SEQUENCE_PRIV:SS42503_INSUF_PRIV
"%0c: you lack '%1c' on sequence '%2c' owned by '%3c'."
/*%
** Parameters:
**	%0c - sequence operation
**	%1c - privilege(s) that the user lacks on the sequence
**	%2c - sequence name
**	%3c - sequence owner
**
** Explanation:
**	The user attempted to perform an operation involving a sequence owned by
**	another user while lacking privilege(s) required for the operation.
**
** System Status:
**	The statement will be aborted.
**
** Recommendation:
**	Ensure that the user (or the owner of the database procedure where the
**	sequence is used) possesses privileges required for the operation.
*/
E_PS0D40_INVALID_GRPID_OP:SS5000B_INTERNAL_ERROR
"Invalid PSF operation for iiusergroup encountered."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_group() function encountered an invalid operation in
**	psy_grpflag.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D41_INVALID_APLID_OP:SS5000B_INTERNAL_ERROR
"Invalid PSF operation for iirole encountered."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_aplid() function encountered an invalid operation in
**	psy_aplflag.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D42_INVALID_GRANTEE_TYPE:SS5000B_INTERNAL_ERROR
"Invalid grantee type encountered in iiprotect tuple"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_permit_ok() found an invalid grantee type in dbp_gtype
**	of an iiprotect catalog record.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Delete the permit associated with this error.  Valid grantee types
**	are: 0 (user); 1 (group); 2 (aplid); and 3 (public).
*/
E_PS0D43_XENCODE_ERROR:SS5000B_INTERNAL_ERROR
"An error was encountered during encryption"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_[ca]aplid() function got an error back from scu_xencode()
**	when attempting to encrypt a role password.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D44_SCU_IDEF_ERR:SS5000B_INTERNAL_ERROR
"Failed attempt to store information into SCU."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The scu_idefine() function returned an error.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D45_INVALID_GRANT_OP:SS5000B_INTERNAL_ERROR
"Invalid grant operation encountered"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_dbpriv() function detected an invalid operation in psy_grant.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D46_INVALID_USER_OP:SS5000B_INTERNAL_ERROR
"Invalid user operation encountered"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_user() function detected an invalid operation in psy_usflag.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D47_INVALID_LOC_OP:SS5000B_INTERNAL_ERROR
"Invalid location operation encountered"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_loc() function detected an invalid operation in psy_loflag.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D48_INVALID_AUDIT_OP:SS5000B_INTERNAL_ERROR
"Invalid audit operation encountered"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_audit() function detected an invalid operation in psy_auflag.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D49_INVALID_ALARM_OP:SS5000B_INTERNAL_ERROR
"Invalid alarm operation encountered"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The psy_alarm() function detected an invalid operation in psy_alflag.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_PS0D50_INVALID_GRANTEE_TYPE:SS5000B_INTERNAL_ERROR
"Invalid grantee type (%0d) encountered in IIPROTECT tuple for
\npermit number %1d for table (%2d,%3d)."
/*%
** Parameters:
**	%0d     - grantee type
**	%1d     - permit number
**	%2d,%3d - table ID
**
**
** Explanation:
**	An invalid grantee type was discovered while trying to determine if a
**	permit tuple applies to the current session.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Delete the permit associated with this error.  Valid grantee types
**	are: 0 (user); 1 (group); 2 (aplid); and 3 (public).
*/
E_PS0E01_STMT_NFND:SS26000_INV_SQL_STMT_NAME
"Dynamic SQL statement was not found."
E_PS0E02_NOEXEC:SS07003_CAN_EXEC_CURS_SPEC
"The select statement is not allowed to executed."
E_PS0E03_RGENT_ERR:SS5000B_INTERNAL_ERROR
"Error %0d occurred when reentering table info into the range table."
E_PS0F01_CACHE_FULL:SS50003_EXHAUSTED_RESOURCE
"The descriptor cache is currently full. Try again later."
E_PS0F02_MEMORY_FULL:SS50003_EXHAUSTED_RESOURCE
"There is no more available memory. Try again later."
E_PS0F03_TRANSACTION_ABORTED:SS40001_SERIALIZATION_FAIL
"Transaction had to be aborted. Try again later."
E_PS0F04_RDF_RESOURCE_ERR:SS50003_EXHAUSTED_RESOURCE
"Resource error occurred while retrieving a table description.
Try again or consult your system administrator."
E_PS0F05_BAD_MEM_SIZE:SS5000B_INTERNAL_ERROR
"Possibly bad memory size: %0d."
E_PS0F81_SETLG_PRIV_ERR:SS42503_INSUF_PRIV
"Only the DBA can issue the SET [NO]LOGGING statement."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "SET [NO]LOGGING" statement allows transactions to run with logging
**	disabled, thus bypassing recovery processing. If a NOLOGGING transaction
**	fails, the database will be left in an unknown state.
**
**	Only the DBA of a database may run in this state.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**	Only the DBA can use this session state.
*/
E_PS0F82_SET_SESSION_STMT:SS42000_SYN_OR_ACCESS_ERR
"SET SESSION: invalid or incomplete WITH option '%0c'. The correct syntax is:
\n
\n     SET SESSION 
\n     [WITH [SECURITY_LABEL = 'label' | INITIAL_LABEL]
\n	     [,ON_ERROR = ROLLBACK TRANSACTION|STATEMENT]
\n	     [,ON_USER_ERROR = NOROLLBACK | ROLLBACK TRANSACTION]
\n	     [,ON_LOGFULL = ABORT|COMMIT|NOTIFY]
\n	     [,[NO]PRIVILEGES [= ( priv {,priv} )| ALL | DEFAULT]]
\n           [,[NO]PRIORITY [= nnnn | INITIAL | MAXIMUM | MINIMUM]]
\n           [,[NO]DESCRIPTION [= description]]
\n     ]"
/*%
** Parameters:
**	%0 - option specified
**
** Explanation:
**	The user entered a "SET SESSION" statement, and the WITH clause
**	specified an unknown or incomplete option.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_PS0F83_SET_ONERROR_STMT:SS42000_SYN_OR_ACCESS_ERR
"SET SESSION: invalid on_error option '%0c'.  The correct syntax is:
\n SET SESSION
\n     WITH ON_ERROR = ROLLBACK STATEMENT |
\n                     ROLLBACK TRANSACTION"
/*%
** Parameters:
**	%0  - rollback option specified
**
** Explanation:
**	The user entered a "SET SESSION WITH ON_ERROR" statement and the
**	ON_ERROR clause specified "ROLLBACK <str>", where <str> was not an
**	accepted option.  The only currently accepted ON_ERROR options are
**	"ROLLBACK STATEMENT" and "ROLLBACK TRANSACTION".
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_PS0F84_SET_ONERROR_STMT:SS42000_SYN_OR_ACCESS_ERR
"SET SESSION: invalid on_error option '%0c'.  The correct syntax is:
\n SET SESSION
\n     WITH ON_ERROR = ROLLBACK STATEMENT |
\n                     ROLLBACK TRANSACTION"
/*%
** Parameters:
**	%0  - rollback option specified
**
** Explanation:
**	The user entered a "SET SESSION WITH ON_ERROR" statement, and the
**	ON_ERROR clause specified "ROLLBACK <str>", where <str> was not an
**	accepted option.  The only currently accepted ON_ERROR options are
**	"ROLLBACK STATEMENT" and "ROLLBACK TRANSACTION".
**
**	This is the QUEL version of E_PS0F83_SET_ONERROR_STMT.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_PS0F85_SET_UPDATE_ROWCOUNT_STMT:SS42000_SYN_OR_ACCESS_ERR
"SET UPDATE_ROWCOUNT: invalid update_rowcount option '%0c'.  The correct
\n syntax is:
\n      SET UPDATE_ROWCOUNT
\n                   CHANGED |
\n                   QUALIFIED"
/*%
** Parameters:
**	%0 - update_rowcount option specified
**
** Explanation:
**	The user entered a "SET UPDATE_ROWCOUNT" statement, and the
**	UPDATE_ROWCOUNT clause specified <str>, where <str> was not
**	an accepted option. The only currently accepted options are
**	"CHANGED" and "QUALIFIED".
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_PS0F86_SET_ROLE_WITH:SS42000_SYN_OR_ACCESS_ERR
"SET ROLE: invalid WITH-clause option '%0c'.  The correct
\n syntax is:
\n      SET ROLE role_name | NONE
\n	[WITH PASSWORD  = 'password]';
"
/*%
** Parameters:
**	%0 - Invalid WITH-clause specified
**
** Explanation:
**	The user entered a "SET ROLE" statement, and the
**	ROLE clause specified <str>, where <str> was not
**	an accepted option. The only currently accepted option is
**	PASSWORD
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_PS0F87_SET_ON_USER_ERROR:SS42000_SYN_OR_ACCESS_ERR
"SET SESSION: invalid on_user_error option '%0c'.  The correct syntax is:
\n SET SESSION
\n     WITH ON_USER_ERROR = ROLLBACK TRANSACTION |
\n                     NOROLLBACK"
/*%
** Parameters:
**      %0  - rollback option specified
**
** Explanation:
**      The user entered a "SET SESSION WITH ON_USER_ERROR" statement and the
**      ON_ERROR clause specified "ROLLBACK <str>", where <str> was not an
**      accepted option.  The only currently accepted ON_USER_ERROR options are
**      "NOROLLBACK" and "ROLLBACK TRANSACTION".
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Correct the syntax error and reenter.
*/
E_PS1001_ERR_SCU_INFO:SS5000B_INTERNAL_ERROR
"Error occurred on an SCU INFORMATION call."
I_PS1002_CRTTBL_WITHOPT_IGNRD:SS00000_SUCCESS
"CREATE TABLE: WITH-clause option '%0c' is not supported
\nby this DBMS.  It was assumed to be a gateway-specific option and ignored."
/*%
** Parameters:
**	%0  - unsupported WITH-clause option
**
** Explanation:
**	The "CREATE TABLE" statement contains a WITH-clause option that is not
**	supported by this DBMS.  Based on its format, it was assumed to be a
**	gateway-specific option and ignored (as stipulated in the OPEN SQL
**	Reference Manual.)  This message does not imply that the statement has
**	been aborted; it is intended as an informational message only.
**
** System Status:
**	This is an informational message; no change in system status.
**
** Recommendation:
**	Ensure that the WITH-clause option in question was entered correctly.
*/
E_PS1003_ERR_AUDITING:SS5000B_INTERNAL_ERROR
"Error occurred while auditing a security event."
/*
** General invalid object type, e.g when REGISTER
*/
E_PS1004_BAD_OBJ_TYPE:SS5000B_INTERNAL_ERROR
"E_PS1004_BAD_OBJ_TYPE: Invalid type '%0d' detected when '%1c'"
E_PS1005_SEC_WRITE_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to write a Security Audit Record.
Check the error log (errlog.log) for more information why this occurred."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error was detected while attempting to write a security audit record.
**	Additional details about the precise error should already have been
**	logged in the error log file; these additional error messages
**	describe the actual problem encountered.
**
** System Status:
**	The operation is aborted. The DBMS may stop auditing or shutdown,
**	depending on the action selected by the security audit configuration.
**
** Recommendation:
**	The error log should contain additional error messages, possibly
**	beginning with the prefix "E_SXxxxx", which will describe the actual
**	problem that was encountered when creating the database. Using the
**	information in these messages, resolve the error condition and reissue
**	the createdb command. If you are unable to determine the reason for
**	the error, contact Ingres Corporation Technical Support.
*/
E_PS1006_DEF_LABEL_ERR:SS42000_SYN_OR_ACCESS_ERR
"Error getting the default security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to create the default security
**	label.
**
** System Status:
**	The operation is aborted. 
**
** Recommendation:
**	The error log should contain additional error messages, possibly
**	beginning with the prefix "E_SXxxxx", which will describe the actual
**	problem that was encountered when creating the database. Using the
**	information in these messages, resolve the error condition and reenter
**	the operation.
*/
E_PS1007_MAC_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred while performing MAC arbitration on a PSF object.\n
The object type is '%0c' with name '%1c'.\n
Please check the error log (errlog.log) for more information why
this occurred."
/*%
** Parameters:
**	%0c - object type
**	%1c - object name
**
** Explanation:
**	An error occurred while attempting to perform MAC access on an object.
**
** System Status:
**	The operation is aborted. 
**
** Recommendation:
**	The error log should contain additional error messages, possibly
**	beginning with the prefix "E_SXxxxx", which will describe the actual
**	problem that was encountered when creating the database. Using the
**	information in these messages, resolve the error condition and reenter
**	the operation.
*/
E_PS1008_ALTER_SECAUDIT_ERR:SS5000B_INTERNAL_ERROR
"The security audit state could not be altered to the requested configuration.
\nPlease check the error log for more detail why the change could not 
\nbe made."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error was returned while attempting to alter the security audit
**	state.
** 
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	The error log should contain additional error messages, possibly
**	beginning with the prefix "E_SXxxxx", which will describe the actual
**	problem that was encountered when creating the database. Using the
**	information in these messages, resolve the error condition and reenter
**	the operation.
*/
/*
** schang :
** gateway specific error, modeled after Ingres error
*/
E_PS1101_REG_NOLOC:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE/INDEX: Can not specify location in register table/index syntax."
E_PS1102_REG_NODIST:SS42000_SYN_OR_ACCESS_ERR
"REGISTER/REMOVE TABLE/INDEX statement not allowed in a distributed server."
E_PS1103_IDX_TOOMANYOPT:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: Too many options in the WITH clause."
E_PS1104_IDX_BADSTRUCT:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: Bad storage structure '%0c'.
The valid storage structure names are keyed, sorkey, fullkey."
E_PS1105_IDX_WITHINVAL:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: '%0c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nSTRUCTURE is the only valid keyword for this type of subclause."
E_PS1106_IDX_NOPTBL:SS42500_TBL_NOT_FOUND
"REGISTER INDEX: cannot find primary table '%0c'."
E_PS1107_TBL_MAXCOL:SS50002_LIMIT_EXCEEDED
"REGISTER TABLE: the number of columns must not exceed 1024."
E_PS1108_TBL_COLINVAL:SS42506_INVALID_IDENTIFIER
"REGISTER TABLE: invalid column name '%0c'."
E_PS1109_TBL_COLDUP:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: duplicate column name '%0c'."
E_PS110A_TBL_REGFAIL:SS42503_INSUF_PRIV
"You are not allowed to register tables in this database.\n
Contact your Database Administrator for further information."
E_PS110B_TBL_COLFORMAT:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: invalid column format '%0c' on column '%1c'."
E_PS110C_TBL_COLLIST:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: A datatype must be specified for each column
\nin the registered table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	On the "REGISTER" statement, when used in a non-distributed session,
**	the datatype must follow each column name.  This describes
**	the column type to which the underlying object's column
**	must be mapped.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Supply a type for each column on the "REGISTER" statement.
*/
E_PS110D_TBL_NOCOL:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: A list of the column names and their datatypes
\nmust be specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "REGISTER" statement, when used in a non-distributed session,
**	requires a list of the registered table's columns and datatypes.
**	The syntax is:
**	    REGISTER TABLE tablename
**		(col_name format [IS 'ext_format']
**		{, col_name format [IS 'ext_format']})
**	    AS IMPORT ...
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Supply a list of columns and their types on the "REGISTER" statement.
*/
E_PS110E_GATEWAY_ERROR:SS50005_GW_ERROR
"An error was returned by a Gateway.
\nCheck the error log (errlog.log) for more information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A gateway returned an error that could not otherwise be
**	mapped.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**	Check the error log for more information on the exact cause
**	of the error.
*/
E_PS110F_GW_REM_ERR:SS50005_GW_ERROR
"A Gateway error occurred performing a REMOVE/DROP  operation on '%0c'.
\nCheck the error log (errlog.log) for more specific information."
W_PS1110_REG_TAB_IDX_MULTI_COL:SS50005_GW_ERROR
"WARNING!! In the RMS Gateway, components of a Multi-column key must
\n have one of the four character data types: CHAR, VARCHAR, C, or
\n TEXT. The column '%0c' in the key list does not have 
\none of the four character data types.  If you do not use one of
\n the character data types, the wrong number of rows may be returned
\n to queries using this table/index."
/*
** REGISTER PROCEDURE errors, some of these errors are modelled after REGISTER
** TABLE errors in erusf.msg
*/
E_PS1201_LDB_PROC_LVL_UNSUP:SS42000_SYN_OR_ACCESS_ERR
"REGISTER PROCEDURE: Procedure '%0c' is in a database at Release 6.4 or below.
\n You may not register this procedure."
E_PS1202_REG_WRONGTYPE:SS5000A_QUERY_ERROR
"REGISTER %0c: '%1c' is of type '%2c'.
\n You must use 'REGISTER %2c'"
E_PS1203_NO_LDB_PROC:SS42505_OBJ_NOT_FOUND
"LDB procedure '%0c' does not exist
\n on node '%1c', database '%2c', dbms '%3c'."
E_PS1204_RMV_WRONGTYPE:SS5000A_QUERY_ERROR
"REMOVE %0c:  STAR object '%1c' corresponds to an LDB\n
object of type procedure.  Use REMOVE PROCEDURE."
E_PS1205_DROP_WRONGTYPE:SS5000A_QUERY_ERROR
"DROP/DESTROY %0c:  STAR object '%1c' was used
\n to REGISTER an LDB object of type procedure.
\n You cannot DROP/DESTROY a registered procedure."
E_PS1206_REGISTER:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REGISTER [TABLE|VIEW] objname [(col_name {, col_name})]
\n      AS LINK [FROM source] [with_clause]
\n
\n REGISTER objname AS LINK WITH REFRESH
\n
\n REGISTER PROCEDURE objname AS LINK [FROM source] [with_clause]"
E_PS1207_REMOVE:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REMOVE [TABLE|VIEW] object_name {, object_name}
\n
\n REMOVE PROCEDURE object_name {, object_name}"
NO_SQLSTATE_MAPPING
