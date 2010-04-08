SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _OP
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	22-oct-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	07-dec-1993 (dianeh)
**	    Continue refining text destined for messages.txt file.
**	15-mar-94 (andre)
**	    messages which used to be mapped to 2A000-series SQLSTATEs need to
**	    be remapped to corresponding 42000-series SQLSTATEs because 2A000- 
**	    and 37000-series SQLSTATEs are going away
**	04-may-95 (nick)
**	    added E_OP009B_STACK_OVERFLOW
**	19-jun-1996 (sweeney)
**	    Fix typos.
**	21-Aug-1996 (prida01)
**	    Add missing error message for E_OP0A98_OPCTIDEQC
**	10-oct-1996 (canor01)
**	    Make messages more generic by removing some hard-coded
**	    references to 'ingres'.
**	15-sep-1998 (sarjo01)
**	    Added E_OP0018_TIMEOUTABORT
**	1-nov-99 (inkdo01)
**	    Added E_OP0985 for histogram creation on non-histo columns.
**	20-jul-00 (inkdo01)
**	    Added W_OP0992 for statdump of histogram whose row count differs 
**	    significantly from the current row count.
**	15-jun-2001 (somsa01)
**	    Re-worked E_OP0200 to be more meaningful.
**	16-july-01 (inkdo01)
**	    Added E_OP0993 in support of new optimizedb/statdump 
**	    "-xr" command line option.
**	14-jan-03 (inkdo01)
**	    Added E_OP04C0 to note failures of the new enumeration technique.
**	28-Feb-2005 (schka24)
**	    Add E_OP009D for misc unexpected internal errors.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	22-sep-2006 (dougi)
**	    Add OP04C1 - consistency check in greedy enumeration.
**	4-apr-2007 (dougi)
**	    Add E_OP088B - consistency check compiling scrollable cursor.
**	2-Apr-2008 (kibro01) b120056
**	    Add W_OP0976 warning message
**	18-april-2008 (dougi)
**	    Add E_OP028D error for table procedure parameter dependency cycle.
**	24-jun-08 (hayke02)
**	    Modify E_OP04C1 so that it reports that non-greedy enumeration
**	    will be performed.
**	18-Aug-2008 (kibro01) b115336
**	    Added W_OP096B_NO_HISTOGRAM
**	01-Oct-2009 (smeke01) b122210
**	    Changed wording and layout of E_OP000C because it broke over 3 
**	    lines in a messy way, and the word 'apparently' does not seem  
**	    appropriate in the context.
**      01-Feb-2010 (maspa05) b123140
**          Added E_OP0994_DUP_RELS for the case where there are multiple
**          -r flags for the same table with optimizedb or statdump
*/


#define _OP_CLASS 4


E_OP0000_OK:SS00000_SUCCESS
"successful completion"
E_OP0001_USER_ERROR:SS50000_MISC_ERRORS
"user error has been reported by optimizer"
E_OP0002_NOMEMORY:SS50003_EXHAUSTED_RESOURCE
"optimizer ran out of
\n    memory before generating execution plan"
E_OP0003_ASYNCABORT:SS50000_MISC_ERRORS
"asynchronous abort was detected "
E_OP0004_RDF_GETDESC:SS50000_MISC_ERRORS
"request for relation
\n    information failed - possibly because table was modified or deleted"
E_OP0005_GRANGETABLE:SS50002_LIMIT_EXCEEDED
"global range table overflow
\n    - query too complex because too many tables or subselects specified"
E_OP0006_TIMEOUT:SS50000_MISC_ERRORS
"optimizer timeout
\n    warning - suboptimal solution found"
E_OP0007_INVALID_OPCODE:SS50000_MISC_ERRORS
"opf_call entry point
\n    received an invalid operation code"
E_OP0008_NOEXECUTE:SS50000_MISC_ERRORS
"query optimized
\n   - trace flag indicates no execution should take place "
E_OP0009_TUPLE_SIZE:SS50002_LIMIT_EXCEEDED
"query cannot be processed,
\n   - size of tuple intermediate node is too large for all cases"
E_OP000A_RETRY:SS50000_MISC_ERRORS
"internal retry being attempted"
E_OP0018_TIMEOUTABORT:SS50000_MISC_ERRORS
"optimizer timeout abort
\n    - query optimization/execution aborted"
/*%
** Parameters:
**	None.
**
** Explanation:
**      This is an internal protocol message and should not be
**      returned to the user.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP000B_EXCEPTION:SS50000_MISC_ERRORS
"Consistency check - Floating point or integer exception occurred during
\n    - optimization phase and a query plan could not be found"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unrecoverable floating point exception occurred for this query.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP000C_DBPROC_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"Request for database procedure information failed.\n
The database procedure is not accessible or does not exist."
/*%
** Parameters:
**
** Explanation:
**	An attempt to obtain a description of a database procedure referenced
**	in a statement (or fired by a rule) resulted in an error because that
**	database procedure no longer exists.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP000D_RETRY:SS50000_MISC_ERRORS
"Consistency check - Internal retry will be attempted since a possible
\n    - deadlock situation has been detected"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error should not be reported, since it is only used for
**	internal code paths.
**
** System Status:
**      The query will be retried.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP000E_AGGTUP:SS50002_LIMIT_EXCEEDED
"Expressions in aggregate result list are longer than maximum tuple width"
/*%
** Parameters:
**	None.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Place a less complicated expression in the aggregate.
**
*/
E_OP000F_TEMP_SIZE:SS50000_MISC_ERRORS
"Temporary file size limits exceeded for all query plans due to SORTMAX limit"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The temporary file size limits exceeded for all query plans due to
**      SORTMAX server startup limits.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Increase the SORTMAX limit.
*/
E_OP0010_RDF_DEADLOCK:SS50000_MISC_ERRORS
"Resource deadlock encountered."
/*%
** Parameters:
**	None.
**
** Explanation:
**      A deadlock was encountered when the optimizer was reading system catalog
**      information about the tables being used in this query.
**
** System Status:
**      The transaction will be aborted.
**
** Recommendation:
**      Retry the transaction later.
*/
E_OP0011_TREE_PACK:SS50000_MISC_ERRORS
"Request to pack query tree into IITREE tuples failed -
\n possibly because the tree was inconsistent."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A unexpected error occurred when OPF called RDF to convert the query
**	tree into IITREE tuple format.  This is likely to be caused by the
**	query tree being inconsistent.
**
** System Status:
**      The statement will be aborted.
**
** Recommendation:
**      Contact Ingres Corporation Technical Support.
*/
E_OP0012_TEXT_PACK:SS50000_MISC_ERRORS
"Request to pack query text into IIQRYTEXT tuples failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A unexpected error occurred when OPF called RDF to convert query text 
**	into IIQRYTEXT tuple format.
**
** System Status:
**      The statement will be aborted.
**
** Recommendation:
**      Contact Ingres Corporation Technical Support.
*/

E_OP0013_READ_TUPLES:SS50000_MISC_ERRORS
"Request to read tuples from a system catalog failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A unexpected error occurred when OPF called RDF to read tuples from a
**	DBMS catalog.
**
** System Status:
**      The statement will be aborted.
**
** Recommendation:
**      Check the error log for an RDF error message.  Take the
**	corrective action suggested by the RDF error message.
*/

E_OP0080_ULM_CLOSESTREAM:SS50000_MISC_ERRORS
"consistency check
\n    - error deallocating a memory stream "
E_OP0081_NOSTATUS:SS50000_MISC_ERRORS
"consistency check
\n    - no status provided by error handling mechanism "
E_OP0082_UNEXPECTED_EX:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected exception occurred
\n Associated error messages which provide more detailed information about the
problem can be found in the error log (errlog.log)"
E_OP0083_UNEXPECTED_EXEX:SS50000_MISC_ERRORS
"consistency check
\n    - really bad, got an unexpected exception trying to recover
\n    from an exception"
E_OP0084_DEALLOCATION:SS50000_MISC_ERRORS
"consistency check
\n    - resource deallocation error "
E_OP0085_QSO_LOCK:SS50000_MISC_ERRORS
"consistency check
\n    - query tree could not be retrieved from QSF "
E_OP0086_ULM_STARTUP:SS50000_MISC_ERRORS
"consistency check
\n    - server startup error - cannot startup ULM"
E_OP0087_RDF_INITIALIZE:SS50000_MISC_ERRORS
"consistency check
\n    - server startup error - cannot startup RDF"
E_OP0088_ULM_SHUTDOWN:SS50000_MISC_ERRORS
"consistency check
\n    - server shutdown error - cannot shutdown ULM"
E_OP0089_ALTER:SS50000_MISC_ERRORS
"consistency check
\n    - invalid alter opcode"
E_OP008A_SCF_SERVER_MEM:SS50003_EXHAUSTED_RESOURCE
"consistency check
\n    - error obtaining memory for OPF server control block"
E_OP008B_SCF_SERVER_MEM:SS50000_MISC_ERRORS
"consistency check
\n    - error releasing memory for OPF server control block"
E_OP008C_SEMAPHORE:SS50000_MISC_ERRORS
"consistency check
\n    - could not initialize optimizer server semaphore"
E_OP008D_RDF_UNFIX:SS50000_MISC_ERRORS
"consistency check
\n    - could not unfix an RDF table"
E_OP008E_RDF_INVALIDATE:SS50000_MISC_ERRORS
"consistency check
\n    - could not invalidate an RDF table"
E_OP008F_RDF_MISMATCH:SS50000_MISC_ERRORS
"The RDF cache
\n    could not be synchronized with parser time stamp, SCF should reparse
\n    query,  this message should not be reported by SCF"
E_OP0090_PAINE:SS50000_MISC_ERRORS
"consistency check
\n    - PAINE handling routine could not be initialized by OPF"
E_OP0091_AIC:SS50000_MISC_ERRORS
"consistency check
\n    - AIC handling routine could not be initialized by OPF"
E_OP0092_SEMWAIT:SS50000_MISC_ERRORS
"consistency check
\n    - semaphore wait error for OPF server control block"
E_OP0093_ULM_ERROR:SS50000_MISC_ERRORS
"consistency check
\n    - memory manager error"
E_OP0094_RDF_SHUTDOWN:SS50000_MISC_ERRORS
"consistency check
\n    - error shutting down RDF facility"
E_OP0095_PARSE_TREE:SS50000_MISC_ERRORS
"consistency check
\n    - invalid parse tree from PSF"
E_OP0096_INSF_OPFMEM:SS50000_MISC_ERRORS
"insufficient opf memory
\n    - must be at least 2 * number_sessions * 50K"
E_OP0097_CONDITION:SS50000_MISC_ERRORS
"Consistency check
\n    - unexpected concurrency error with OPF ACTIVE server support"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal error dealing with limits of active threads in the
**      optimizer occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0098_CONDITION_INIT:SS50000_MISC_ERRORS
"Consistency check
\n    - unexpected concurrency error initializing OPF ACTIVE server support"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal initialization error dealing with limits of active threads
**	in the optimizer occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0099_CONDITION_FREE:SS50000_MISC_ERRORS
"Consistency check
\n    - unexpected concurrency error with OPF ACTIVE server support"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal shutdown error dealing with limits of active threads in the
**      optimizer occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP009A_INCONSISTENT_TAB:SS50000_MISC_ERRORS
"Table/index %0c explicitly referenced by statement (or contained 
\nin explicitly referenced view) is logically or physically inconsistent"
/*%
** Parameters:
**	%0c - table/index name.
**
** Explanation:
**      Likely a partial backup/restore has left a table inconsistent with its 
**      indexes and one of the indexes was explicitly referenced in the 
**      statement. 
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Rebuild index(es) to be consistent with underlying table and rerun 
**      statement.
*/
E_OP009B_STACK_OVERFLOW:SS50002_LIMIT_EXCEEDED
"Stack overflow detected
\n    - query aborted"
/*%
** Parameters:
**
** System Status:
**      The query will be terminated.  Ongoing transactions are not aborted.
**
** User Action:
**      Please report the problem to Technical Support.
**
*/
E_OP009C_RDF_COPY:SS50000_MISC_ERRORS
"Partition definition copy function failed - query aborted."
/*%
** Parameters:
**
** System Status:
**	The query will be terminated; the session is otherwise unaffected.
**
** Recommendation:
**	Examine the error log for other, related messages.  If there
**	is a memory shortage, increase the indicated memory pool.
*/
E_OP009D_MISC_INTERNAL_ERROR:SS50000_MISC_ERRORS
"Internal optimizer error: %0c"
/*%
** Parameters:
**	%0c - A short description of the error
**
** Explanation:
**	The optimizer facility encountered an unexpected internal
**	error.  An internal consistency check or assertion failed.
**
** System Status:
**	The query is aborted, but ongoing transactions are not aborted.
**
** Recommendation:
**	This message always indicates some kind of internal optimizer
**	error.  Please report it to Ingres Corporation Technical Support.
*/
E_OP0200_TUPLEOVERFLOW:SS50002_LIMIT_EXCEEDED
"Query result tuple
\n    length is larger than the configured maximum tuple length"
E_OP0201_ATTROVERFLOW:SS50002_LIMIT_EXCEEDED
"too many columns
\n    in aggregate temporary relation"
E_OP0202_VAR_OVERFLOW:SS50002_LIMIT_EXCEEDED
"query too complicated
\n    too many tables referenced, subselects or aggregates specified"
E_OP0280_SCOPE:SS50000_MISC_ERRORS
"consistency check
\n    - scoping rules for variables used in query were violated"
E_OP0281_SUBQUERY:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected subquery type in optimizer"
E_OP0282_FAGG:SS50000_MISC_ERRORS
"consistency check
\n    - function aggregate structure inconsistent"
E_OP0283_PROJECT:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected aggregate projections request"
E_OP0284_RESDOM:SS50000_MISC_ERRORS
"consistency check
\n    - resdom list not monotonically decreasing"
E_OP0285_UNIONAGG:SS50000_MISC_ERRORS
"consistency check
\n    - union or aggregate unexpected in this context"
E_OP0286_UNEXPECT_VAR:SS50000_MISC_ERRORS
"Consistency check
\n    - several alias for the same table were used and a mapping
\n    problem was discovered"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An error in handling multiple aliases for the same table was detected.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0287_OJVARIABLE:SS50000_MISC_ERRORS
"Consistency check
\n    - outer join relation placement was incorrect"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal error dealing with limits of active threads in the
**      optimizer occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0288_OJAGG:SS50000_MISC_ERRORS
"Consistency check
\n    - outer join definition of aggregate is inconsistent"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal error dealing with aggregates used with outer joins 
**      has occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP028A_MISSING_CORELATION:SS50000_MISC_ERRORS
"Consistency check - missing correlation attribute, expecting equi-join attribute"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal error dealing will flattening and variable removal
**      optimization occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP028B_VAR_ELIMINATION:SS50000_MISC_ERRORS
"Consistency check - expecting variable to be eliminated after substitution"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal error dealing will flattening and variable removal
**      optimization occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP028C_SELFJOIN:SS50000_MISC_ERRORS
"Consistency check - unexpected self join aggregate discovered"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal error dealing will aggregate variable elimination
**      has occurred.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP028D_PARMCYCLE:SS50000_MISC_ERRORS
"Table procedure parameter dependency cycle detected between\n
 procedures %0c and %1c."
/*%
** Parameters:
**	None.
**
** Explanation:
**      Table procedure parameter expressions may not form a dependency
**	cycle with each other (e.g. proc A parmlist references proc B
**	result column and proc B parmlist references proc A result column).
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Rewrite query to avoid the faulty table procedure parameters.
*/
E_OP0300_ATTRIBUTE_OVERFLOW:SS50002_LIMIT_EXCEEDED
"the joinop attributes
\n    array overflowed - query too complex"
E_OP0301_EQCLS_OVERFLOW:SS50002_LIMIT_EXCEEDED
"equivalence class
\n    array overflowed - query too complex"
E_OP0302_BOOLFACT_OVERFLOW:SS50002_LIMIT_EXCEEDED
"too many boolean
\n    factors defined in query - query too complex "
E_OP0303_RTOVERFLOW:SS50002_LIMIT_EXCEEDED
"joinop range table
\n    does not have sufficient room for variables - query is too complex"
E_OP0304_FUNCATTR_OVERFLOW:SS50002_LIMIT_EXCEEDED
"function attribute
\n    table overflowed - query too complex"
E_OP0305_NOCONVERSION:SS50000_MISC_ERRORS
"cannot convert
\n    constant to key - warning, keyed access cannot be used"
E_OP0380_JNCLAUS:SS50000_MISC_ERRORS
"Consistency check
\n    - left and right part of conditional are incompatible"
E_OP0381_FUNCATTR:SS50000_MISC_ERRORS
"consistency check
\n    - inconsistent typing error in function attributes "
E_OP0382_FATTR_CREATE:SS50000_MISC_ERRORS
"consistency check
\n    - function attribute should have been created "
E_OP0383_TYPEMISMATCH:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected type mismatch in equivalence class"
E_OP0384_NOATTS:SS50000_MISC_ERRORS
"consistency check
\n    - no attribute in equivalence class "
E_OP0385_EQCLS_MERGE:SS50000_MISC_ERRORS
"consistency check
\n    - more than one equivalence class in merge "
E_OP0386_BFCREATE:SS50000_MISC_ERRORS
"consistency check
\n    - assumption about ordering of boolean factor list failed "
E_OP0387_VARNO:SS50000_MISC_ERRORS
"consistency check
\n    - varno from parser range table out of range"
E_OP0388_VARBITMAP:SS50000_MISC_ERRORS
"consistency check
\n    - var bit map is invalid"
E_OP0389_EQUIJOIN:SS50000_MISC_ERRORS
"consistency check
\n    - equi join , unexpected types for operands"
E_OP038A_REPEAT:SS50000_MISC_ERRORS
"consistency check
\n    - repeat query parameter not available when expected"
E_OP038B_NO_SUBSELECT:SS50000_MISC_ERRORS
"consistency check
\n    - cannot find subselect for correlated variable"
E_OP038C_UNION:SS50000_MISC_ERRORS
"consistency check
\n    - union subquery expected"
E_OP038D_CORRELATED:SS50000_MISC_ERRORS
"consistency check
\n    - expecting correlated aggregate"
E_OP038E_MAXOUTERJOIN:SS50000_MISC_ERRORS
"     - maximum number of outer joins per subquery has been exceeded"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system limit for an outer join process has been exceeded.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Reduce the number of outer joins specified in the query.
*/
E_OP038F_OUTERJOINSCOPE:SS50000_MISC_ERRORS
"consistency check
\n    - outer join scoping rules were violated"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an internal outer join
**      specification which was inconsistent.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0390_ATTRIBUTE:SS50000_MISC_ERRORS
"consistency check
\n    - an attribute was inconsistently specified"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an attribute.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0391_HISTLEN:SS50000_MISC_ERRORS
"consistency check
\n    - a histogram length was out of expected range "
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of a histogram.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support. 
**	It may be caused by running optimizedb over a network between machines 
**	with incompatible architectures.  A workaround would be to delete 
**	statistics on this particular column.
*/
E_OP0392_HISTINCREASING:SS50000_MISC_ERRORS
"consistency check
\n    - histogram on attribute is not monotonically increasing"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of a histogram, in which the cell boundaries are not
**      increasing.  This error should only be reported for Star.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support, or 
**	check statdump output of the attribute to ensure the data is increasing.
*/
E_OP0393_HISTOUTOFRANGE:SS50000_MISC_ERRORS
"consistency check
\n    - the selectivity of a histogram was out of range 0.0 <= value <= 1.0"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP0394_NOATTR_DEFINED:SS50000_MISC_ERRORS
"consistency check
\n    - an attribute should be defined for the outer join at this point"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP0395_PSF_JOINID:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected outer join ID found"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0396_DUPLICATE_OJID:SS50000_MISC_ERRORS
"consistency check
\n    - attempt to evaluate 2 joinid's at the same node "
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP0397_MISPLACED_OJ:SS50000_MISC_ERRORS
"consistency check
\n    - attempt to evaluate outer join at unsupported node "
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP0398_OJ_SCOPE:SS50000_MISC_ERRORS
"consistency check
\n    -  cannot find outer join associated with view"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP0399_FJ_VARSETS:SS50000_MISC_ERRORS
"consistency check
\n    - full join does not create 2 variable sets "
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP039A_OJ_TYPE:SS50000_MISC_ERRORS
"consistency check
\n    -  unexpected outer join type"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP039B_OJ_LEFTRIGHT:SS50000_MISC_ERRORS
"consistency check
\n    - cannot resolve between left or right outer join types "
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP039C_OJ_HIST:SS50000_MISC_ERRORS
"consistency check
\n    -  mismatched histograms in outer join "
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP039D_OJ_COVAR:SS50000_MISC_ERRORS
"consistency check
\n    -  unexpected correlated variable expression"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**      The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP039E_EXPECTING_VAR:SS50000_MISC_ERRORS
"consistency check
\n    -  there must be at least one inner for an outer join"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP039F_BOOLFACT:SS50000_MISC_ERRORS
"consistency check
\n    - all elements of boolean factor do not have same join id"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP03A0_OJBITMAP:SS50000_MISC_ERRORS
"consistency check
\n    -  sum of maps must equal parent"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of an outer join data structure.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP03A1_OJMAP:SS50000_MISC_ERRORS
"consistency check
\n    -  outer join variable map is not a subset of parent"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of a histogram selectivity.  All selectivities should
**      be between 0.0 and 1.0
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute causing the problem.
*/
E_OP03A2_HISTINCREASING:SS50000_MISC_ERRORS
"consistency check
\n    - histogram on %0c attribute %1c is not monotonically increasing"
/*%
** Parameters:
**	%0c - table name
**	%1c - attribute name
**
** Explanation:
**      The system has found, during optimization, an inconsistent internal
**      specification of a histogram, in which the cell boundaries are not
**      increasing.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support, or 
**      check statdump output of the attribute to ensure the data is increasing.
*/
E_OP03A5_KEYINCELL:SS50000_MISC_ERRORS
"consistency check
\n    - key value falls in histogram cell but is not equal to either boundary"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, that a key value specified
**	in the query falls inside a histogram cell but does not equal either
**	boundary value of the cell. This should not happen because the
**	cell-splitting in oph_catalog() should have created a new boundary
**	value equal to the key value.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**	This is an xDEBUG-only consistency check. Please report this error to
**	the optimizer development group.
*/
E_OP03A6_OUTER_JOIN:SS50000_MISC_ERRORS
"consistency check
\n    - internal processing error related to outer joins "
/*%
** Parameters:
**	None.
**
** Explanation:
**	An inconsistent internal system state was detected related
**	to outer join processing
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**	Please report this error to technical support.
*/
E_OP03A7_OJ_SUBSELECT:SS50000_MISC_ERRORS
"consistency check
\n    - internal error, multiple subselects not outer join compatible "
/*%
** Parameters:
**	None.
**
** Explanation:
**	An inconsistent internal system state was detected related
**	to outer join processing and subselect handling
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**	Please report this error to technical support.
*/
E_OP0400_MEMORY:SS50003_EXHAUSTED_RESOURCE
"ran out of
\n    enumeration memory, optimizer needs more memory"
E_OP0480_MISSING_TEMP_HISTO:SS50000_MISC_ERRORS
"consistency check
\n    - histogram from temporary relation expected"
E_OP0481_KEYINFO:SS50000_MISC_ERRORS
"Consistency check
\n    - cannot find OPB_BFKEYINFO pointer when expected"
E_OP0482_HISTOGRAM:SS50000_MISC_ERRORS
"Consistency check
\n    - cannot find OPH_HISTOGRAM structure for equivalence class"
E_OP0483_HIST_BUFFER:SS50002_LIMIT_EXCEEDED
"Consistency check
\n    - overflow of temporary histogram buffer "
E_OP0484_HISTTYPE:SS50000_MISC_ERRORS
"Consistency check
\n    - unexpected histogram type "
E_OP0485_HISTAND:SS50000_MISC_ERRORS
"consistency check
\n    - attempt to AND two incompatible histograms "
E_OP0486_NOKEY:SS50000_MISC_ERRORS
"consistency check
\n    - no key found when one expected "
E_OP0487_NOEQCLS:SS50000_MISC_ERRORS
"consistency check
\n    - no joining equivalence class found when expected "
E_OP0488_JOINNOEQCLS:SS50000_MISC_ERRORS
"consistency check
\n    - no joining equivalence class found when expected "
E_OP0489_NOFUNCHIST:SS50000_MISC_ERRORS
"consistency check
\n    - no histogram found for function attribute when expected "
E_OP048A_NOCARTPROD:SS50000_MISC_ERRORS
"consistency check
\n    - no joining equivalence class and there is not a cartesian product "
E_OP048B_COST:SS50000_MISC_ERRORS
"consistency check
\n    - cpu or disk i/o cost is negative "
E_OP048C_SORTCOST:SS50000_MISC_ERRORS
"consistency check
\n    - sort cost routine failed"
E_OP048D_QUERYTYPE:SS50000_MISC_ERRORS
"consistency check
\n    - unknown subquery type"
E_OP048E_COTYPE:SS50000_MISC_ERRORS
"consistency check
\n    - unknown CO node type"
E_OP048F_NULL:SS50000_MISC_ERRORS
"consistency check
\n    - expecting IS NULL or IS NOT NULL operator id"
E_OP0490_CSSTATISTICS:SS50000_MISC_ERRORS
"consistency check
\n    - CSstatistics called failed, so enumeration phase cannot
\n    be bounded"
E_OP0491_NO_QEP:SS50000_MISC_ERRORS
"consistency check
\n    - no query plan was found:
\n Check values of OPF memory parameters for server
\n are at least defaults"
E_OP0492_ORIG_NODE:SS50000_MISC_ERRORS
"consistency check
\n    - orig node expected on end of CO list"
E_OP0493_MULTIATTR_SORT:SS50000_MISC_ERRORS
"consistency check
\n    - invalid multi-attribute sort type"
E_OP0494_ORDERING:SS50000_MISC_ERRORS
"consistency check
\n   - unexpected ordering present"
E_OP0495_CO_TYPE:SS50000_MISC_ERRORS
"consistency check
\n   - unexpected CO node type"
E_OP0496_EXACT:SS50000_MISC_ERRORS
"consistency check
\n   - exact multi-attribute ordering expected"
E_OP0497_INCOMPATIBLE:SS50000_MISC_ERRORS
"consistency check
\n   - incompatible orderings found in CO node"
E_OP0498_NO_EQCLS_FOUND:SS50000_MISC_ERRORS
"consistency check
\n   - equivalence class not found when expected"
E_OP0499_NO_CO:SS50000_MISC_ERRORS
"consistency check
\n   - expected CO memory to be available "
E_OP049A_CSALTR_SESSION:SS50000_MISC_ERRORS
"consistency check
\n   - could not turn on/off cpu accounting via CSaltr_session"
E_OP049B_SORT:SS50000_MISC_ERRORS
"consistency check
\n   - useless sort  node found in query plan"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, that a query plan has been
**      generated which has an unnecessary sort node. This type of query plan
**	should not be generated.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP049C_INEXACT:SS50000_MISC_ERRORS
"consistency check - inexact ordering expected"
E_OP049D_ON:SS50000_MISC_ERRORS
"consistency check - outer join ON clause estimate error found"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected cost estimate result in ON clause histogramming has
**	occurred.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP049E_FLOAT_UNDERFLOW:SS50000_MISC_ERRORS
"Floating point underflow occurred during the optimization phase"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An unrecoverable floating point underflow occurred for this query.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP049F_FLOAT_DIVIDE:SS50000_MISC_ERRORS
"Floating point divide by zero occurred during the optimization phase"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An unrecoverable floating point divide-by-zero occurred for this query.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A0_FLOAT_OVERFLOW:SS50000_MISC_ERRORS
"Floating point overflow occurred during the optimization phase"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An unrecoverable floating point overflow occurred for this query.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A1_INT_OVERFLOW:SS50000_MISC_ERRORS
"integer overflow occurred during the optimization phase"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An unrecoverable integer overflow occurred for this query.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A2_INT_DIVIDE:SS50000_MISC_ERRORS
"integer divide by zero occurred during the optimization phase"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An unrecoverable integer divide-by-zero occurred for this query.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A3_ATTR_FOUND:SS50000_MISC_ERRORS
"consistency check -
\n     - Histogram processing for project-restrict could not find expected attribute"
/*%
** Parameters:
**	None.
**
** Explanation:
**      A histogram estimation problem has occurred with this query.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A4_ONLY_ONE:SS50000_MISC_ERRORS
"consistency check -
\n     - only one histogram expected"
/*%
** Parameters:
**	None.
**
** Explanation:
**      A histogram estimation problem has occurred with this query.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A5_SHAPE:SS50000_MISC_ERRORS
"consistency check -
\n     - Unexpected join tree shape encountered"
/*%
** Parameters:
**	None.
**
** Explanation:
**      A problem occurred in the tree-generating algorithms of the optimizer.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A6_SECINDEX:SS50000_MISC_ERRORS
"consistency check -
\n     - Secondary index expected but not found in optimizer placement strategy"
/*%
** Parameters:
**	None.
**
** Explanation:
**      A problem occurred with the secondary index placement routines of the
**	 optimizer.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A7_CONODE:SS50000_MISC_ERRORS
"consistency check -
\n     - no cost ordering node found as expected"
/*%
** Parameters:
**	None.
**
** Explanation:
**      A problem occurred with the optimizer memory management routines.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A8_USAGEOVFL:SS50000_MISC_ERRORS
"consistency check -
\n     - internal data structure usage count overflow"
/*%
** Parameters:
**	None.
**
** Explanation:
**      A problem occurred with the optimizer memory management routines.
**      A workaround may be to use different OPF memory server startup
**	parameters.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04A9_OJEST:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected tuple estimate in outer join"
/*%
** Parameters:
**	None.
**
** Explanation:
**      A problem occurred with the optimizer cost model dealing with outer
**	joins.  A workaround may be to turn off statistics.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP04AA_HISTOMOD:SS50000_MISC_ERRORS
"Histogram modification failed. Original histogram will be used."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A problem occurred while adding predicate keys to the histogram.
**	The original histogram will be used; cost estimates will be inaccurate
**	unless predicate keys fall on original histogram boundaries.
**
** System Status:
**	Optimization continues; cost estimates may be inaccurate.
**
** Recommendation:
**	Try re-running optimizedb on tables referenced in the query.
**	If that does not work, report the problem to Ingres Corporation 
**	Technical Support.
*/
E_OP04AB_NOPRIMARY:SS50000_MISC_ERRORS
"consistency check -
\n     - no primary (base) relation variable available when expected"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04AC_JOINID:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected outer join type "
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04AD_CORRELATED:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected outer join attribute used in correlation"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04AE_OBSOLETE:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error in histogram processing"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04AF_JOINEQC:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error in processing joining attributes"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04B0_NESTEDOUTERJOIN:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error in nested outer joins"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04B1_AGGUV:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error in union view used in an aggregate"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04B2_TUPLEID:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error creating tuple id for outer join processing"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04B3_HISTOUTERJOIN:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error processing outer join histograms"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04B4_PLACEOUTERJOIN:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error placing outer join in query plan"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04B5_FILTEREOUTERJOIN:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error placing filters for outer joins"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04B6_SPECIALOUTERJOIN:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected internal error placing outer join special attributes"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP04C0_NEWENUMFAILURE:SS50000_MISC_ERRORS
"Enhanced enumeration technique failed to find valid query plan. Reverting 
to old technique"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The enhanced enumeration technique of OPF failed to find a valid
**	query plan. 
**
** System Status:
**	OPF will retry the compilation using its existing (slower)
**	enumeration algorithm.
**
** Recommendation:
**      This is an informational message. No action need be taken.
**
*/
E_OP04C1_GREEDYCONSISTENCY:SS50000_MISC_ERRORS
"Greedy enumeration produced a plan that wouldn't solve the query. Reverting
to old technique"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The enhanced enumeration technique of OPF found a plan that failed
**	to cover all the tables of the query. This shouldn't happen.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP0680_TARGETLIST:SS50000_MISC_ERRORS
"consistency check
\n    - target list has non PST_RESDOM nodes in it"
E_OP0681_UNKNOWN_NODE:SS50000_MISC_ERRORS
"consistency check
\n    - unknown query tree node type found"
E_OP0682_UNEXPECTED_NODE:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected query tree node - PST_CURVAL"
E_OP0683_DUMPQUERYTREE:SS50000_MISC_ERRORS
"consistency check
\n    - error dumping query tree "
E_OP0684_DUMPQUERYNODE:SS50000_MISC_ERRORS
"consistency check
\n    - error dumping query tree node "
E_OP0685_RESOLVE:SS50000_MISC_ERRORS
"consistency check
\n    - error resolving query tree node "
E_OP0686_CQMODE:SS50000_MISC_ERRORS
"consistency check
\n    - Illegal query mode for compilation."
E_OP0687_NOQTREE:SS50000_MISC_ERRORS
"consistency check
\n    - No query tree when expected."
E_OP0688_INEXACT:SS50000_MISC_ERRORS
"consistency check
\n    - expecting inexact ordering nesting in an exact ordering"
E_OP0689_BAD_TREE:SS50000_MISC_ERRORS
"consistency check
\n    - bad query tree defined, applying de Morgan's laws"
E_OP068A_BAD_TREE:SS50000_MISC_ERRORS
"consistency check
\n    - error converting query tree to text"
E_OP068B_BAD_VAR_NODE:SS50000_MISC_ERRORS
"consistency check
\n    - error converting query tree to text; var node"
E_OP068C_BAD_TARGET_LIST:SS50000_MISC_ERRORS
"A bad select (or subselect) target list has been found"
E_OP068D_QTREE:SS50000_MISC_ERRORS
"error copying query tree nodes"
E_OP068E_NODE_TYPE:SS50000_MISC_ERRORS
"consistency check -
\n     - unexpected query node type"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP068F_MISSING_RESDOM:SS50000_MISC_ERRORS
"consistency check -
\n     - could not find temp table attribute number"
/*%
** Parameters:
**	None.
**
** Explanation:
**      An internal system problem was detected.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
**
*/
E_OP0700_ADC_CVINTO:SS50000_MISC_ERRORS
"cannot build key
\n    because conversion does not exist"
E_OP0701_ADF_EXCEPTION:SS50000_MISC_ERRORS
"ADF exception
\n    - see error block for info, ADF exception warning"
E_OP0702_ADF_EXCEPTION:SS50000_MISC_ERRORS
"ADF exception
\n    - user error number between 120 and 16F "
E_OP0780_ADF_HISTOGRAM:SS50000_MISC_ERRORS
"consistency check
\n    - the ADF returned an unexpected error - adc_hg_dtln"
E_OP0781_ADI_FIDESC:SS50000_MISC_ERRORS
"consistency check
\n    - error calling adi_fidesc "
E_OP0782_ADI_CALCLEN:SS50000_MISC_ERRORS
"consistency check
\n    - error calling adi_calclen "
E_OP0783_ADI_FICOERCE:SS50000_MISC_ERRORS
"consistency check
\n    - error calling adi_ficoerce"
E_OP0784_ADC_KEYBLD:SS50000_MISC_ERRORS
"consistency check
\n    - error calling adc_keybld"
E_OP0785_ADC_HELEM:SS50000_MISC_ERRORS
"consistency check
\n    - error calling adc_helem"
E_OP0786_ADC_DHMAX:SS50000_MISC_ERRORS
"consistency check
\n    - default upper bound for uniform histogram error from ADF "
E_OP0787_ADC_DHMIN:SS50000_MISC_ERRORS
"consistency check
\n    - default lower bound for uniform histogram error from ADF "
E_OP0788_ADC_CVINTO:SS50000_MISC_ERRORS
"consistency check
\n    - conversion of histogram element unexpectedly failed"
E_OP0789_ADC_HMIN:SS50000_MISC_ERRORS
"consistency check
\n    - error calling min histogram value function routine"
E_OP078A_ADC_HMAX:SS50000_MISC_ERRORS
"consistency check
\n    - error calling max histogram value function routine"
E_OP078B_ADC_HDEC:SS50000_MISC_ERRORS
"consistency check
\n    - error calling histogram decrement routine"
E_OP078C_ADC_COMPARE:SS50000_MISC_ERRORS
"consistency check
\n    - ADF compare routine failed "
E_OP078D_ADI_OPID:SS50000_MISC_ERRORS
"consistency check
\n    - ADF operator ID routine failed in startup"
E_OP078E_ADE_CXSPACE:SS50000_MISC_ERRORS
"consistency check
\n    - Error estimating the size of a CX."
E_OP078F_ADE_BGNCOMP:SS50000_MISC_ERRORS
"consistency check
\n    - Error beginning a compilation."
E_OP0790_ADE_CONSTGEN:SS50000_MISC_ERRORS
"consistency check
\n    - Error generating a constant."
E_OP0791_ADE_INSTRGEN:SS50000_MISC_ERRORS
"consistency check
\n    - Error generating an instruction."
E_OP0792_ADE_INFORMSP:SS50000_MISC_ERRORS
"consistency check
\n    - Error informing ADF of a larger CX."
E_OP0793_ADE_ENDCOMP:SS50000_MISC_ERRORS
"consistency check
\n    - Error ending a compilation."
E_OP0794_ADI_OPNAME:SS50000_MISC_ERRORS
"consistency check
\n    - cannot get operator name from operator ID"
E_OP0795_ADF_EXCEPTION:SS50000_MISC_ERRORS
"ADF exception - non-user error,
\n    ADF error return code returned in error block"
E_OP0796_NO_COMPLEMENT:SS50000_MISC_ERRORS
"consistency check
\n    - no complement exists for function when one was expected"
E_OP0797_OP_USE:SS50000_MISC_ERRORS
"consistency check
\n      - unexpected operator construction PREFIX, POSTFIX, or INFIX mismatch"
E_OP0798_OP_TYPE:SS50000_MISC_ERRORS
"consistency check
\n      - unexpected operator type"
E_OP0799_LENGTH:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected length from intrinsic type"
/*%
** Parameters:
**      None.
**
** Explanation:
**      Internal processing detected an inconsistent length from
**      float, int, char, or varchar.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP079A_OPID:SS50000_MISC_ERRORS
"consistency check
\n    - unexpected binary operator"
/*%
** Parameters:
**      None.
**
** Explanation:
**      Internal processing detected an unexpected binary operator:
**      only <, >, <=, or >= expected.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP079B_ADI_RESOLVE:SS50000_MISC_ERRORS
"consistency check
\n    - could not resolve datatypes"
/*%
** Parameters:
**      None.
**
** Explanation:
**	Could not resolve two datatypes to a common datatype.
**
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP0800_NODEFAULT:SS50000_MISC_ERRORS
"The user has not
\n    specified all of the required columns for insertion."
E_OP0880_NOT_READY:SS50000_MISC_ERRORS
"This feature or
\n    section of code is not yet implemented."
E_OP0881_DMUFUNC:SS50000_MISC_ERRORS
"Illegal DMU function
\n    type for compilation."
E_OP0882_QSF_CREATE:SS50003_EXHAUSTED_RESOURCE
"Error calling QSF
\n    to create a new object."
E_OP0883_QSF_SETROOT:SS50000_MISC_ERRORS
"Error calling QSF
\n    to set the root of an object."
E_OP0884_RELEQC:SS50000_MISC_ERRORS
"Consistency error:
\n    Orig node returns an eqc that is not found in the originated relation."
E_OP0885_COTYPE:SS50000_MISC_ERRORS
"Consistency error:
\n    Illegal CO node type."
E_OP0886_QSF_PALLOC:SS50003_EXHAUSTED_RESOURCE
"Error calling QSF
\n     to allocate a piece of memory."
E_OP0887_CONSTTYPE:SS50000_MISC_ERRORS
"Illegal datatype
\n    or length for a constant in a query tree."
E_OP0888_ATT_NOT_HERE:SS50000_MISC_ERRORS
"Attribute is not
\n    available at a CO node."
E_OP0889_EQ_NOT_HERE:SS50000_MISC_ERRORS
"Eqc is not available
\n    at a CO node."
E_OP088A_EST_TUPLES:SS50000_MISC_ERRORS
"consistency check
\n    - estimated number of tuples returned from the orig node is too large."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is raised only in the debugging version of the code.
**	It will be raised when the estimated tuple count for an orig node
**	exceeds the number that can be represented in a four-byte long integer.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Development.
*/
E_OP088B_BAD_NODE:SS50000_MISC_ERRORS
"Consistency Check: PST_VAR node expected but not found in scrollable cursor."
E_OP0890_MAX_BASE:SS50000_MISC_ERRORS
"Too many bases
\n    added to a QEN_ADF struct."
E_OP0891_MAX_ROWS:SS50000_MISC_ERRORS
"Too large a row
\n    number used to add a base to a QEN_ADF struct."
E_OP0892_MAX_REPEATS:SS50000_MISC_ERRORS
"Too large a repeat
\n    query number used to add a base to a QEN_ADF struct."
E_OP0893_NV_REPEAT:SS50000_MISC_ERRORS
"A repeat query
\n    number that is not valid is being added to a QEN_BASE."
E_OP0894_EXCESSATTS:SS50000_MISC_ERRORS
"Too many attributes
\n    are being returned from a subselect"
E_OP0895_NO_SEJQTREE:SS50000_MISC_ERRORS
"Consistency Check:
\n    There is no query tree for a subselect join."
E_OP0896_MODIFY_ATTR:SS50000_MISC_ERRORS
"Consistency Check: the default attribute number 1 to modify does not exist."
E_OP0897_BAD_CONST_RESDOM:SS50000_MISC_ERRORS
"Consistency Check: a query tree constant or resdom node uses a
\n    source/destination type."
E_OP0898_ILL_LVARNO:SS50000_MISC_ERRORS
"Consistency Check: An out of range local variable number was found in a
\n    query tree for a DB procedure."
E_OP0899_BAD_TARGNO:SS50000_MISC_ERRORS
"A target number was found (in a query tree or otherwise) that is illegal."
E_OP089A_DIFF_BOOL_FACT:SS50000_MISC_ERRORS
"Consistency check: new and old boolean factor maps are different."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is raised only in the debugging version of the code.
**	It will be raised when the boolean factor bitmap received from
**	OPF is different from the one obtained using the old version of
**	the code.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Development.
*/
E_OP089B_PJOIN_FOUND:SS50000_MISC_ERRORS
"Consistency check: PJOIN implied."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is raised when the query plan implies a PJOIN.
**      PJOINs are not supported by this version of the code.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP089C_BAD_VALID_LIST:SS50000_MISC_ERRORS
"Consistency check: Invalid valid list."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is raised when internal DBMS structures contain
**	invalid data.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP089D_QSF_INFO:SS50000_MISC_ERRORS
"Error calling QSF to obtain object info."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is raised when an unsuccessful call to obtain QSF
**	object information has been executed.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP089E_QSF_UNLOCK:SS50000_MISC_ERRORS
"Error calling QSF to unlock an object."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is raised when an unsuccessful call to unlock a QSF object
**	has been executed.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP089F_QSF_FAILCREATE:SS50000_MISC_ERRORS
"Error occurred when creating a QSF object.
\nquery type : %0c
\nobject type: %1c
\nobject name: %2c
\nquery text for the corresponding query:
\n%3c"
/*%
** Parameters:
**	%0c - query type
**	%1c - object type
**	%2c - object name
**	%3c - query text
**
** Explanation:
**      This error is raised when an unsuccessful call to create a QSF object
**	has been executed.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP08A0_SD_COMPILE_FAIL:SS50000_MISC_ERRORS
"Unexpected error in call of SDcompile for table: %0c. Result: %1d"
/*%
** Parameters:
**	%0c - table name
**	%1d - return status
**
** Explanation:
**      This error is raised when a Smart Disk compile request has failed.
*/
E_OP08A1_TOO_SHORT:SS50000_MISC_ERRORS
"Internal error occurred when materializing data into a reusable buffer."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This internal error is raised when generating instructions moving
**      data into a reusable buffer, and the buffer length turns out
**      to be too short.
**
**      This error should never occur.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP08A2_EXCEPTION:SS50000_MISC_ERRORS
"Unexpected exception occurred within query compilation"
/*%
** Parameters:
**      None.
**
** Explanation:
**      This internal exception occurred while converting the query
**      plan into an executable form.
**
** System Status:
**      Execution of the query will be terminated.
**
** Recommendation:
**      Report the problem to Ingres Corporation Technical Support.
*/
E_OP08A3_UNKNOWN_STATEMENT:SS50000_MISC_ERRORS
"Unknown statement type.  Query compilation aborted."
/*%
** Parameters:
**	None.
**
** Explanation:
**      The compiler does not understand a statement type generated by the
**      parser.
**
** System Status:
**      The query will be terminated.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08A4_DELIMIT_FAIL:SS50000_MISC_ERRORS
"Error generating an unnormalized delimited identifier in SQL text."
/*%
** Parameters:
**	None.
**
** Explanation:
**	There has been an error during the SQL code-generation phase of
**	a distributed query.
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08A5_DATATYPE_MISMATCH:SS50000_MISC_ERRORS
"Two attributes couldn't be joined because their datatypes disagreed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During query compilation, two attributes were supposed to have the
**	same datatype but didn't.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08A6_NO_ATTRIBUTE:SS50000_MISC_ERRORS
"An equivalence class was not represented by an attribute."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During query compilation, we encountered an equivalence class which
**	had no attribute currently representing it.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08A7_BARREN:SS50000_MISC_ERRORS
"A parent node, which should have had children, didn't."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During query compilation, we encountered a parent node, which we
**	expected to have children--but it didn't.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08A8_MISSING_TID:SS50000_MISC_ERRORS
"No TID attribute found when expected."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During query compilation, we expected to find a TID attribute but
**	didn't.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08A9_BAD_RELATION:SS50000_MISC_ERRORS
"An unknown relation appeared during query compilation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During query compilation, we stumbled on an unknown relation number.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08AA_EMPTY_NODE:SS50000_MISC_ERRORS
"A node returned no equivalence classes."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During query compilation, we stumbled on a node which didn't return
**	any equivalence classes.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08AB_BAD_JOIN_TYPE:SS50000_MISC_ERRORS
"Unsupported join strategy requested."
/*%
** Parameters:
**	None.
**
** Explanation:
**	During query compilation, we were asked to compile an unknown
**	join strategy.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08AC_UNKNOWN_OPERATOR:SS50000_MISC_ERRORS
"An unrecognized operator occurred in an expression."
/*%
** Parameters:
**	None.
**
** Explanation:
**	While compiling an expression, we encountered an unknown operator.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08AD_SPATIAL_ERROR:SS50000_MISC_ERRORS
"An internal error was encountered while generating a spatial index query plan."
/*%
** Parameters:
**	None.
**
** Explanation:
**	While compiling an expression, we encountered an unknown operator.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Please report the problem to Ingres Corporation Technical Support.
*/
E_OP08AE_NOTSETI_PROC:SS50000_MISC_ERRORS
"Procedure: %0c has no SET OF parameter, but was called from statement rule 
or with global temporary table parameter."
/*%
** Parameters:
**	%0c - procedure name.
**
** Explanation:
**	A procedure was invoked with a temporary table parameter, either from
**	the triggering of a statement level rule or the issuance of an
**	"execute procedure" statement with a global temporary table actual
**	parameter. However, the procedure did not have a corresponding "set of" 
**	formal parameter. Procedures called with temporary table parameters must 
**	have a corresponding "set of" formal parameter whose definition matches 
**	that of the temporary table.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Recode procedure with an appropriate "set of" formal parameter.
*/
E_OP08AF_GTTPARM_MISMATCH:SS50000_MISC_ERRORS
"Global temporary table parameter: %0c, was passed to procedure: %1c, whose
 SET OF parameter doesn't exactly match the table definition."
/*%
** Parameters:
**	%0c - global temporary table name
**	%1c - procedure name.
**
** Explanation:
**	An EXECUTE PROCEDURE statement attempted to pass a global temporary
**	table parameter to a procedure whose "set of" formal parameter didn't
**	exactly match the table definition. The "set of" parameter must
**	match the table definition in the number of columns and the name,
**	type, length and nullability of each of the columns.
**
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Assure that the global temporary table definition matches that of the
**	procedure "set of" parameter, and retry the statement.
**	
*/
E_OP08B0_NODEFAULT:SS50000_MISC_ERRORS
"Entry: %0c in SET OF element list is defined NOT NULL and NOT DEFAULT, but 
has no corresponding actual parameter."
/*%
** Parameters:
**	%0c - element name in procedure SET OF list
**
** Explanation:
**	A procedure with a "set of" formal parameter was called as the result
**	of a triggered rule. An element was defined in the "set of" list
**	which is not null with no default, yet has no corresponding actual
**	parameter in the "execute procedure" clause of the rule definition. If
**	a "set of" element is defined not null with no default, there MUST be
**	a matching element in the "execute procedure" clause.
**
** System Status:
**	The query will be terminated.
**
** Recommendation:
**	Fix either the procedure definition or rule definition and retry the
**	triggering statement.
**
*/
E_OP08B1_PARAM_INVALID:SS42000_SYN_OR_ACCESS_ERR
"The database procedure parameter '%0c' is invalid.
The parameter name does not exist in the procedure referenced by an
EXECUTE PROCEDURE statement or database rule invocation."
/*%
** Parameters:
**      %0c - database procedure parameter
**
** Explanation:
**      This error will occur when executing a procedure via the "EXECUTE
**      PROCEDURE" statement or via the firing of a rule. This message is
**	analgous to QE0123 (detected at runtime).
**
** System Status:
**      If this is an "EXECUTE PROCEDURE" statement, the statement is aborted.
**      If this is a rule invocation, the original query triggering the rule
**      is aborted with a fatal error.
**
** Recommendation:
**      Correct the parameter name in the "EXECUTE PROCEDURE" statement or
**      "EXECUTE PROCEDURE" clause in the "CREATE RULE" statement.
*/
E_OP08B2_DESC_ERROR:SS50000_MISC_ERRORS
"Consistency check - descending sort without top sort node
has been chosen incorrectly."
I_OP0900_INTERRUPT:SS01000_WARNING
"\nExecution aborted due to user interrupt."
E_OP0901_UNKNOWN_EXCEPTION:SS50000_MISC_ERRORS
"\nUnknown exception occurred in optimizer utility. Value is %0d."
E_OP0902_ADFSTARTUP:SS50000_MISC_ERRORS
"%0c: internal error occurred when starting up Abstract Data Type Facility."
E_OP0903_ADFSESSION:SS50000_MISC_ERRORS
"%0c: internal error occurred when starting up Abstract Data Type Facility session."
E_OP0904_OPENFILE:SS42505_OBJ_NOT_FOUND
"%0c: cannot open input file '%1c', OS status:%2d.\n%3c"
E_OP0905_ARGUMENTS:SS50002_LIMIT_EXCEEDED
"%0c: more than %1d arguments."
E_OP0906_STACK:SS50003_EXHAUSTED_RESOURCE
"%0c: ran out of stack space."
E_OP0907_UNIQUECELLS:SS50000_MISC_ERRORS
"%0c: bad unique cells value: The value %3d specified in the -zu# flag
\nwas not a number, or not in the range greater than %1d or less than %2d."
E_OP0908_HISTOCELLS:SS5000B_INTERNAL_ERROR
"%0c: bad histogram cell count, -zr# range allowed %1d<#<%2d, but got :%3d."
E_OP0909_INGRESFLAGS:SS50002_LIMIT_EXCEEDED
"%0c: more than %1d %2c flags, or unrecognizable flags."
E_OP090A_DATABASE:SS5000B_INTERNAL_ERROR
"%0c: unexpected parameter:'%1c'."
E_OP090B_DBLENGTH:SS50002_LIMIT_EXCEEDED
"%0c: database name length must be <= %1d :'%2c'."
E_OP090C_RELLENGTH:SS50002_LIMIT_EXCEEDED
"%0c: relation name length must be <= %1d :'%2c'."
E_OP090D_ATTLENGTH:SS50002_LIMIT_EXCEEDED
"%0c: attribute name length must be <= %1d :'%2c'."
E_OP090E_PARAMETER:SS5000B_INTERNAL_ERROR
"%0c: unrecognized parameter: '%1c'."
E_OP090F_TABLES:SS50006_FATAL_ERROR
"%0c: more than 100 new tables created or non-existent tables
\n referenced, initial guess of %1d tables for database  '%2c' is too low.
\n run utility again later when less concurrent activity exists"
/*%
** Parameters:
**      %0c - utility name
**      %1c - estimated number of tables from iirelation
**      %2c - database name
**
** System Status:
**      An initial guess for space allocation is made by a count on
**      iirelation for the maximum number of tables which can be optimized.
**      This error occurs if iirelation is grossly out of date, either
**      100 new relations have been created, or some of the relations in the
**      utility parameter list reference relations which do not exist.
**
** Recommendation:
**      If a parameter list references relations directly with the "-r" option,
**      make sure all relations do exist.  If new or temp table creation rate
**	is high, run the utility using only the "-r" option or run the utility
**	at a time of less concurrent table creation activity.
**
**      If this does not correct problem, contact Ingres Corporation 
**      Technical Support.
*/
E_OP0910_DUPTABID:SS50000_MISC_ERRORS
"%0c: duplicate table IDs in database '%1c', table '%2c'."
E_OP0911_NOTABLE:SS5000B_INTERNAL_ERROR
"%0c: database '%1c', table '%2c' cannot be found or is not owned by you."
E_OP0912_VIEW:SS5000B_INTERNAL_ERROR
"%0c: database '%1c', table '%2c' is a view and will be ignored."
E_OP0913_ATTRIBUTES:SS50006_FATAL_ERROR
"%0c: database '%1c', number of columns in table '%2c' exceeds maximum of %3d."
E_OP0914_ATTRTUPLES:SS50000_MISC_ERRORS
"%0c: database '%1c', table '%2c', column %3c inconsistent system catalog,
\n    more than one attribute tuple for attribute ID."
E_OP0915_NOATTR:SS5000B_INTERNAL_ERROR
"optimizedb: database '%0c', table '%1c', column '%2c' not found."
E_OP0916_TYPE:SS50000_MISC_ERRORS
"%0c: bad datatype id =%1d in iiattribute.attfrmt, column name '%2c'."
E_OP0917_HTYPE:SS50000_MISC_ERRORS
"%0c: histogram type=%1d prec=%2d len=%3d in iiattribute, column name %4c,
\n   error->%5c"
E_OP0918_NOCOLUMNS:SS5000B_INTERNAL_ERROR
"optimizedb: there are no columns for database '%0c', table '%1c'."
E_OP0919_READ_ERR:SS50000_MISC_ERRORS
"Error %0d occurred when reading input file.\n%1c"
/*%
** Parameters:
**      %0d - error number
**      %1c - error message, if any
**
** Explanation:
**      A problem occurred when reading the contents of an input file.
**
** Recommendation:
**      Try again; if the problem persists, contact Ingres Corporation 
**      Technical Support.
*/
E_OP091A_STACK:SS50003_EXHAUSTED_RESOURCE
"%0c: ran out of dynamic memory to allocate (status %1d, size requested %2d)."
/*%
** Parameters:
**      %0d - utility name
**      %1d - error status
**      %2d - size requested
**
** Explanation:
**      A problem occurred when allocating dynamic memory.
**
** System Status:
**	Execution of the utility will be terminated.
**
** Recommendation:
**      Check the values following the command-line flags, "-za" and "-zt",
**      then try again. If the problem persists, contact Ingres Corporation 
**      Technical Support.
*/
E_OP091B_TUPLECOUNT:SS50000_MISC_ERRORS
"optimizedb: bad retrieval, database '%0c', table '%1c', i:%2d."
I_OP091C_NOROWS:SS01000_WARNING
"optimizedb: table '%0c' in database '%1c' contains no rows."
E_OP091D_HISTOGRAM:SS50000_MISC_ERRORS
"optimizedb: histogram conversion failed, column '%0c', code = %1d."
E_OP091E_BAD_VALUE_LENGTH:SS5000B_INTERNAL_ERROR
"Wrong input value '%0d' for statistics on table '%1c', column '%2c';
\nInvalid value length; does not match length of column or < 1."
E_OP091F_DATATYPE:SS50000_MISC_ERRORS
"%0c: error displaying type %1d, prec %2d, length %3d"
E_OP0920_DATATYPE:SS50000_MISC_ERRORS
"optimizedb: unsupported histogram datatype; type %0d, prec %1d, length %2d"
E_OP0921_ERROR:SS50002_LIMIT_EXCEEDED
"consistency check - parameter count too large for optimizer utility."
E_OP0922_BAD_ERLOOKUP:SS50000_MISC_ERRORS
"Cannot read error message %0d (%1x) for optimizer utility."
E_OP0923_DMF_BIT:SS50000_MISC_ERRORS
"%0c: statistics exist; this should be reflected in system catalogs, but is not."
E_OP0924_NON_BASE:SS50000_MISC_ERRORS
"%0c: non-base relation has statistics '%1c', data base '%2c'."
W_OP0925_NOSTATS:SS01000_WARNING
"statdump: database '%0c', table '%1c', column '%2c' - statistics not found."
E_OP0926_DUPLICATES:SS50000_MISC_ERRORS
"\nstatdump: database '%0c', table '%1c', column '%2c' - inconsistent iistats
catalog; %3d rows found."
E_OP0927_DUPLICATES:SS50000_MISC_ERRORS
"\nstatdump: database '%0c', table '%1c', column '%2c' - inconsistent
iihistograms catalog; %3d rows with sequence %4d found."
E_OP0928_BAD_COMPLETE:SS50000_MISC_ERRORS
"Invalid complete flag value `%0d' specified for column `%1c' in
table `%2c'.\nExpected values are `0' or `1'."
/*%
** Parameters:
**      %0d - complete flag value
**      %1c - attribute name
**      %2c - table name
**
** Explanation:
**      The user attempted to read in statistics from a file and an
**      invalid complete flag value was specified.
**
** Recommendation:
**	Correct the error in the file; ensure that the value is either
**	"0" or "1".
*/
E_OP0929_ADC_HDEC:SS50000_MISC_ERRORS
"optimizedb: An ADF error %0d occurred when creating a histogram for
column `%1c' in table `%2c'."
/*%
** Parameters:
**      %0d - error code
**      %1c - attribute name
**      %2c - table name
**
** System Status:
**      An error occurred when calling the adc_hdec() function in ADF.
**
** Recommendation:
**      This is an internal error. Please report the problem to
**      Technical Support.
*/
I_OP092A_STATSDELETED:SS50000_MISC_ERRORS
"statdump: statistics for database '%0c', table '%1c', column '%2c' deleted."
E_OP092B_COMPARE:SS50000_MISC_ERRORS
"optimizedb: data comparison failed, column '%0c', code = %1d."
I_OP092C_NULL:SS01000_WARNING
"optimizedb: Statistics for table '%0c', column '%1c' not created - NULL values found."
I_OP092D_REL_NOT_FOUND:SS01000_WARNING
"optimizedb: database '%0c' table '%1c' cannot be found."
/*%
** Parameters:
**      %0c - database name
**      %1c - table name
**
** Explanation:
**      The table for which statistics is being updated does not exist.
**
** Recommendation:
**      This is a user error; define the table before reading in statistics.
*/
E_OP092E_CVSN_CERR:SS50000_MISC_ERRORS
"Error %0d occurred when converting STANDARD_CATALOG_LEVEL value of '%1c'."
E_OP092F_STD_CAT:SS50000_MISC_ERRORS
"%0c version does not match the standard catalog level in database '%1c'."
E_OP0930_NOT_BASE:SS5000B_INTERNAL_ERROR
"%0c: database '%1c', table '%2c' is not a base table and will be ignored."
E_OP0931_NO_FILE:SS5000B_INTERNAL_ERROR
"-i/-o flag must be followed by a file name."
E_OP0932_BAD_PREC:SS5000B_INTERNAL_ERROR
"Precision for floating point numbers should be in the [0,30] range."
E_OP0933_BAD_USER:SS5000B_INTERNAL_ERROR
"-u flag must be followed by a user name."
E_OP0934_BAD_SCAT_TYPE:SS50000_MISC_ERRORS
"Table_type in iitables for 'iistats', 'iihistograms', 'iitables' or 'iiphysical_tables'
has an unexpected value of '%0c'."
E_OP0935_LVSN_CERR:SS50000_MISC_ERRORS
"Error %0d occurred when converting SQL_LEVEL value of '%1c'."
E_OP0936_ADI_TYID:SS50000_MISC_ERRORS
"An ADF error %0d occurred when converting data type '%1c'."
E_OP0937_OPEN_ERROR:SS5000B_INTERNAL_ERROR
"%0c: Error %2d when opening file '%1c'.\n%3c"
E_OP0938_CLOSE_ERROR:SS50000_MISC_ERRORS
"%0c: Error %2d when closing file '%1c'.\n%3c"
E_OP0939_TOO_MANY_COLS:SS50002_LIMIT_EXCEEDED
"Internal query generated to retrieve MINMAX statistics data for
'%1c' table in '%0c' database' has too many columns in the target list.
Rerun optimizedb for the table and specify columns using -a flag."
E_OP093A_ADC_LENCHK:SS50000_MISC_ERRORS
"An ADF error %0d occurred when computing result length for column '%1c'."
E_OP093B_TOO_MANY_ROWS:SS50000_MISC_ERRORS
"More rows returned than expected."
E_OP093C_INP_FAIL:SS5000B_INTERNAL_ERROR
"%0c: Failed to read in statistics from '%1c' file."
E_OP093D_MINMAX_NOT_ALLOWED:SS5000B_INTERNAL_ERROR
"Requests to create minmax statistics and to read statistics from a file
are incompatible."
I_OP093E_NO_STATS:SS50000_MISC_ERRORS
"%0c: Database '%1c', table '%2c' owned by '%3c' - no statistics found."
E_OP093F_BAD_COUNT:SS5000B_INTERNAL_ERROR
"Actual number of cells retrieved (%0d) differs from the number of cells
declared (%1d)"
/*%
** Parameters:
**      %0d - actual number of cells retrieved
**      %1d - declared number of cells
**
** System Status:
**      Creation of statistics for the given column will be aborted.
**
** Recommendation:
**      Please make sure that the declared number of cells following
**      the words "histogram cells:" agrees with the actual number of
**      cell entries. Check that cell numbers are numeric and in ascending
**      order.
*/
E_OP0940_READ_ERR:SS5000B_INTERNAL_ERROR
"Error %0d occurred when reading in statistical information from a file.\n%1c"
E_OP0941_TEXT_NOTFOUND:SS5000B_INTERNAL_ERROR
"Text line described by '%0c' not found in the input file when expected."
E_OP0942_ADI_PM_ENCODE:SS50000_MISC_ERRORS
"An ADF error %0d occurred when executing adi_pm_encode()."
E_OP0943_ADI_FICOERCE:SS50000_MISC_ERRORS
"An ADF error %0d occurred when executing adi_ficoerce()."
E_OP0944_ADF_FUNC:SS50000_MISC_ERRORS
"An ADF error %0d occurred when executing adf_func()."
E_OP0945_HISTVAL_NOT_INC:SS5000B_INTERNAL_ERROR
"Histogram values for table '%0c', column '%1c' are not in ascending order.
\nProblem occurs around cell number %2d."
E_OP0946_CELL_NO:SS5000B_INTERNAL_ERROR
"Wrong cell number found; should start at 0 and should increase."
E_OP0947_WRONG_COUNT:SS5000B_INTERNAL_ERROR
"Cell count must be in the [0.0,1.0] range."
E_OP0948_CELL_TOO_FEW:SS5000B_INTERNAL_ERROR
"There must be at least %0d cells in a histogram."
E_OP0949_CELL_TOO_MANY:SS50002_LIMIT_EXCEEDED
"Maximum number of cells exceeded."
E_OP094A_TRUNC:SS50000_MISC_ERRORS
"Truncation occurred when assigning a character string to host language
variable."
E_OP094B_COL_NO_MISMATCH:SS5000B_INTERNAL_ERROR
"Mismatching number of result columns and result host language variables
in a FETCH or SELECT statement."
E_OP094C_ERR_CRE_SAMPLE:SS5000B_INTERNAL_ERROR
"Could not create table for sample data (sqlcode: %0d, sqlerrd[0]: %1d).\n%2c"
E_OP094D_CANT_SAMPLE:SS5000B_INTERNAL_ERROR
"Sampling feature can be used only on %0c tables."
E_OP094E_INV_SAMPLE:SS5000B_INTERNAL_ERROR
"'-zs' flag must be followed by a number in the (0.0, 100.0) range
expressing percentage of table rows to be sampled."
E_OP094F_SAMPLE_NOT_ALLOWED:SS5000B_INTERNAL_ERROR
"Requests to create statistics based on sample data and to read statistics
from a file are incompatible."
W_OP0950_NOSAMPLING:SS01000_WARNING
"Sampling of the non-%0c table '%1c' is not possible; no statistics will be created."
E_OP0951_BAD_NO_UNIQUE:SS5000B_INTERNAL_ERROR
"Wrong input value '%0f' for statistics on table '%1c', column '%2c';
\nnumber of unique values can never be less than 1."
E_OP0952_BAD_NO_ROWS:SS5000B_INTERNAL_ERROR
"Wrong input value '%0d' for statistics on table '%1c', column '%2c';
\nnumber of rows can never be less than 1."
E_OP0953_BAD_NO_PAGES:SS5000B_INTERNAL_ERROR
"Wrong input value '%0d' for statistics on table '%1c', column '%2c';
\nnumber of pages can never be less than 1."
E_OP0954_BAD_NO_OVFLOW:SS5000B_INTERNAL_ERROR
"Wrong input value '%0d' for statistics on table '%1c', column '%2c';
\nnumber of overflow pages can never be less than 0."
E_OP0955_BAD_NULL_COUNT:SS5000B_INTERNAL_ERROR
"Wrong input value '%0f' for statistics on table '%1c', column '%2c';
\nnumber of null values can never be less than 0."
E_OP0956_TOTAL_COUNT_OFF:SS5000B_INTERNAL_ERROR
"The sum total of normalized cell counts for table '%1c', column '%2c' is '%0f';
expected value is 1.0 +/- %3f."
E_OP0957_NO_CAPABILITY:SS5000B_INTERNAL_ERROR
"Capability '%0c' is missing in iidbcapabilities catalog."
I_OP0958_COUNTING_ROWS:SS01000_WARNING
"Row count for table '%0c' not available from catalogs. Counting rows."
E_OP0959_CAP_ERR:SS50000_MISC_ERRORS
"IIdbcapabilities catalog is inconsistent, DBMS_TYPE indicates STAR, but DISTRIBUTED
value is missing or its value is not equal to 'Y'."
E_OP095A_TUPLE_OVFLOW:SS50002_LIMIT_EXCEEDED
"Error when creating minmax statistics on '%0c' table: resulting row is too wide.
Try creating statistics using -a flag, one column at a time."
E_OP095B_SYSHWEXC:SS50000_MISC_ERRORS
"System or hardware exception occurred in optimizer utility.\n%0c"
/*%
** Parameters:
**      %0c - string pointer
**
** System Status:
**      Execution of the utility will be aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP095C_FCONV_ERR:SS50000_MISC_ERRORS
"Internal error ocurred when converting character string to
a floating point number."
/*%
** Parameters:
**      None.
**
** Explanation:
**      A problem occurred when converting a character string to
**      a floating point number. This should never happen.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
W_OP095D_UDT:SS01000_WARNING
"Cannot create statistics on column '%0c' of table '%1c'.
\nDatatype for the column is a User Defined Datatype (UDT)."
/*%
** Parameters:
**      %0c - attribute name
**      %1c - table name
**
** Explanation:
**      The user attempted to create statistics on a column with
**      a User Defined Datatype (UDT).
**
** System Status:
**	The request will be ignored and optimizedb will continue creating
**	statistics on the remaining columns.
**
** Recommendation:
**	None.
*/
E_OP095E_BAD_UNIQUE:SS50000_MISC_ERRORS
"Invalid unique flag value `%0c' specified for column `%1c' in
table `%2c'.\nExpected values are `Y' or `N'."
/*%
** Parameters:
**      %0c - unique flag value
**      %1c - attribute name
**      %2c - table name
**
** Explanation:
**      The user attempted to read in statistics from a file and an
**      invalid unique flag value was specified.
**
** Recommendation:
**	Correct the error in the file; ensure that the value is either
**	"Y" or "N".
*/
W_OP095F_WARN_UNIQUE:SS01000_WARNING
"The unique flag value `%0c' specified for column `%1c' in table `%2c'
does not conform to optimizer rules."
/*%
** Parameters:
**      %0c - unique flag value
**      %1c - attribute name
**      %2c - table name
**
** Explanation:
**      When reading in statistics from a file, optimizedb discovered that
**	the unique flag value does not conform with the optimizer rules
**	(see below).
**
** System Status:
**	The value and statistics will be stored as requested.
**
** Recommendation:
**      The user should consider changing the unique flag value according
**      to the optimizer rule below:
**
**        if the number_of_unique_values >= 0.9 * row_count
**            set value to `Y'
**        else
**            set value to `N'
**
**      where number_of_unique_values is the number of unique values in the
**      relation (labeled "unique values:") in the statistics file, and
**      row_count is the number of rows in the relation (labeled "rows:")
**      in the statistics file.
*/
E_OP0960_BAD_LENGTH:SS50000_MISC_ERRORS
"%0c: The value '%1c' specified in the -length=# flag
\nis not a number, or is not in the range %2d <= # <= %3d."
W_OP0961_SQLWARN1:SS01000_WARNING
"Truncation occurred when assigning a character string to host language
variable."
/*%
** Parameters:
**      None.
**
** Explanation:
**	Data retrieved from DBMS has been truncated.
**
** System Status:
**      The query will continue.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
W_OP0962_SQLWARN2:SS01000_WARNING
"Null values eliminated from aggregates."
/*%
** Parameters:
**      None.
**
** Explanation:
**	NULL values have been eliminated from aggregates.
**
** System Status:
**      The query will continue.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
W_OP0963_SQLWARN3:SS01000_WARNING
"Mismatching number of result columns and result host language variables
in a FETCH or SELECT statement."
/*%
** Parameters:
**      None.
**
** Explanation:
**	The number of host language variables receiving is different
**	from result columns.
**
** System Status:
**      The query will continue.
**
** Recommendation:
**      Please report the problem to your product vendor.
*/
W_OP0964_SQLWARN4:SS01000_WARNING
"UPDATE or DELETE statement does not have the WHERE clause."
/*%
** Parameters:
**      None.
**
** System Status:
**      The query will continue.
**
** Recommendation:
**      None.
*/
W_OP0965_SQLWARN5:SS01000_WARNING
"Unexpected warning."
/*%
** Parameters:
**      None.
**
** System Status:
**      The query will continue.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
W_OP0966_SQLWARN6:SS01000_WARNING
"Abnormal termination of an active transaction."
/*%
** Parameters:
**      None.
**
** Explanation:
**      An active transaction was abnormally terminated.  Possible cause
**	is a serialization error (e.g., deadlock, log full).
**
** Recommendation:
**      Re-execute your request.
*/
W_OP0967_SQLWARN7:SS01000_WARNING
"Unexpected warning."
/*%
** Parameters:
**      None.
**
** System Status:
**      The query will continue.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0968_OPTDB_IANDO:SS50000_MISC_ERRORS
"Optimizedb cannot specify both -i and -o options."
E_OP0969_OPTIX_NOATTS:SS50000_MISC_ERRORS
"Optimizedb: database %0c, composite histogram on %1c requested with -a parameters."
/*%
** Parameters:
**      %0c - database name
**      %1c - table/index name
**
** Explanation:
**      An attempt was made to create statistics on individual attributes
**	of a secondary index or a base table with the -zcpk option. When an
**	index (in the -r option) or -zcpk is specified, a single composite
**	histogram is built on all key columns and -a parameters are not 
**	permitted.
**
** Recommendation:
**	Remove the -a parameters accompanying the -r specification and
**	rerun optimizedb.
*/
W_OP096A_OPTIX_1ATT:SS01000_WARNING
"Optimizedb: database %0c, composite histogram requested on %1c, but it is
\neither a heap table or has only 1 key column."
W_OP096B_NO_HISTOGRAM:SS01000_WARNING
"Optimizedb: histogram has no cells - if \"statdump -zq\" was used this\n
is expected.  Otherwise it represents an error."
/*%
** Parameters:
**      %0c - database name
**      %1c - table/index name
**
** Explanation:
**      An attempt was made to create statistics on an index or a base table
**	with the -zcpk option. This operation will build a composite
**	histogram on the concatenation of the key columns. However, either
**	the table uses the heap structure or its key structure contains only 
**	1 attribute making this a useless operation, since the resulting 
**	histogram will not be used in query optimization. The statistics 
**	should be run on the corresponding attribute in the base table.
**
** Recommendation:
**	Run statistics on the corresponding attribute in the base table.
*/
W_OP0970:SS01000_WARNING
"Transaction has been automatically restarted due to serialization error."
/*%
** Parameters:
**      None.
**
** Explanation:
**	A serialization error was returned by the DBMS, and the transaction
**      has been automatically restarted.
**
** System Status:
**      Execution will continue.
**
** Recommendation:
**      None required. However, if this happens often, the DBA should
**      consider increasing the size of the log file.
*/
E_OP0971:SS40001_SERIALIZATION_FAIL
"Transaction has been aborted due to serialization error."
/*%
** Parameters:
**      None.
**
** Explanation:
**	The transaction was automatically restarted and it failed again.
**
** System Status:
**      Execution of optimizedb/statdump will be terminated.
**
** Recommendation:
**      Please report the problem to your DBA. Consider increasing
**      the size of the log file.
*/
E_OP0972:SS50000_MISC_ERRORS
"Internal error occurred when scaling number of unique values.
\nSample row count:%0d, Unique values in sample:%1f, Total row count:%2d."
/*%
** Parameters:
**      %0d - number of rows in table holding sample data
**      %1f - number of unique values in table holding sample data
**      %2d - number of rows in table on which statistics are created
**
** Explanation:
**      When scaling the number of unique values for sampled statistics,
**      an internal error occurred.
**
** Recommendation:
**      Please report the problem and displayed data to Ingres Corporation 
**	Technical Support.
*/
W_OP0973_TOTAL_COUNT_OFF:SS01000_WARNING
"Statistics for table '%1c', column '%2c': the sum total of normalized
cell counts ('%0c') is expected to be in the 1.0 +/- %3c range."
/*%
** Parameters:
**      %0c - sum total of normalized cell counts
**      %1c - table name
**      %2c - column name
**      %3c - expected error value
**
** Explanation:
**      All normalized cell counts are expected to add up to 1.0.
**      When reading statistics in, the cell counts are added up
**      and a check is made whether the result is within error
**      margin (1 +/- delta). The delta value takes into account the
**      number of cells added. If the result is outside the error
**      margin, a warning is issued. Statistics will be stored in catalogs.
**
** Recommendation:
**      Check the cell count values.
*/
E_OP0974_BAD_MAX_COLS:SS5000B_INTERNAL_ERROR
"Error %0d occurred when converting iidbcapabilities value '%1c' of
MAX_COLUMNS to integer format."
/*%
** Parameters:
**      %0d - error code passed by conversion routine
**      %1c - value of MAX_COLUMNS (from iidbcapabilities)
**
** Explanation:
**      An error occurred when converting an ASCII number to an integer
**      format.
**
** Recommendation:
**      Check the MAX_COLUMNS value in the iidbcapabilities catalog.
*/
E_OP0975_STAT_COUNT:SS5000B_INTERNAL_ERROR
"Actual number of character statistics retrieved (%0d) differs
\nfrom the value length (%1d)"
/*%
** Parameters:
**      %0d - number of char stats (nunique or density) retrieved
**      %1d - histogram value length
**
** System Status:
**      Creation of statistics for the given column will be aborted.
**
** Recommendation:
**      Please make sure that the number of character-set statistics
**      following the words "unique chars:" and "char set densities:"
**	agrees with value length.
*/
W_OP0976_PREC_WARNING:SS01000_WARNING
"WARNING: Value for rows %0d and %1d have the same value (%2c).
\nThis will cause E_OP0945 when statistics are reimported.
\nConsider using -zn to increase precision."
/*%
** Parameters:
**      %0d - lower cell
**      %1d - upper cell
**      %2c - histogram value
**
** Recommendation:
**      Increase precision of statistical output (-zn)
*/
E_OP0980_ADI_PM_ENCODE:SS50000_MISC_ERRORS
"optimizedb: an ADF error %0d occurred when processing column '%1c' of table '%2c'."
/*%
** Parameters:
**      %0d - error code passed by ADF facility
**      %1c - column name
**      %2c - table name
**
** System Status:
**      An ADF error occurred when executing the adi_pm_encode() function.
**
** Recommendation:
**      If other messages are not present and the cause of the error cannot be
**      identified, please contact Ingres Corporation Technical Support.
*/
E_OP0981_ADI_FICOERCE:SS50000_MISC_ERRORS
"optimizedb: an ADF error %0d occurred when processing column '%1c' of table '%2c'."
/*%
** Parameters:
**      %0d - error code passed by ADF facility
**      %1c - column name
**      %2c - table name
**
** System Status:
**      An ADF error occurred when executing the adi_ficoerce() function.
**
** Recommendation:
**      If other messages are not present and the cause of the error cannot be
**      identified, please contact Ingres Corporation Technical Support.
*/
E_OP0982_ADF_FUNC:SS50000_MISC_ERRORS
"optimizedb: an ADF error %0d occurred when processing column '%1c' of table '%2c'."
/*%
** Parameters:
**      %0d - error code passed by ADF facility
**      %1c - column name
**      %2c - table name
**
** Explanation:
**      An ADF error occurred when executing the adf_func() function.
**
** Recommendation:
**      If other messages are not present and the cause of the error cannot be
**      identified, please contact Ingres Corporation Technical Support.
*/
E_OP0983_ADC_COMPARE:SS50000_MISC_ERRORS
"optimizedb: an ADF error %0d occurred when processing column '%1c' of table '%2c'."
/*%
** Parameters:
**      %0d - error code passed by ADF facility
**      %1c - column name
**      %2c - table name
**
** Explanation:
**      An ADF error occurred when executing the adc_compare() function.
**
** Recommendation:
**      If other messages are not present and the cause of the error cannot be
**      identified, please contact Ingres Corporation Technical Support.
*/
E_OP0984_NO_PRIV_TABLE_STATS:SS50000_MISC_ERRORS
"You are not authorized to generate or view table statistics."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The optimizedb or statdump utility was invoked by a user without the
**	necessary privileges.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Ensure you have appropriate privileges before running these utilities.
*/
E_OP0985_NOHIST:SS50000_MISC_ERRORS
"optimizedb: column %0c is of datatype %1c which does not support histograms."
/*%
** Parameters:
**	%0c - column name
**	%1c - data type
**
** Explanation:
**	Optimizedb cannot be run on columns whose type does not support 
**	histograms (e.g. all the BLOB types).
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Ensure all columns explicitly identified in "-a" parameters support
**	the creation of histograms.
*/
E_OP0990_BAD_RPTFACTOR:SS50000_MISC_ERRORS
"Wrong input value '%0f' for statistics column '%1c' in table '%2c';
\nrepetition factor can never be less than 1.0 and greater than number of rows %3d."
/*%
** Parameters:
**      %0f - repetition factor value
**      %1c - column name
**      %2c - table name
**      %3d - row count
**
** Explanation:
**      When reading statistics from a file, optimizedb found a repetition
**	factor value which is outside of the expected range. Repetition factor
**	should be 1.0 or greater, but never greater than the number of rows
**	in the relation for which it is specified.
**
** Recommendation:
**      Correct the repetition factor value and reexecute.
*/
W_OP0991_INV_RPTFACTOR:SS01000_WARNING
"The repetition factor value `%0f' specified for column `%1c' in table `%2c'
is not sufficiently close to tuple count divided by number of unique values."
/*%
** Parameters:
**      %0f - repeat factor value
**      %1c - column name
**      %2c - table name
**
** Explanation:
**      When reading statistics from a file, optimizedb found a repetition
**	factor value which is outside of the expected range. Repetition factor
**	multiplied by the number of unique values is expected to be close to
**	row count.
**
** Recommendation:
**      Consider correcting the repetition factor value.
*/
W_OP0992_ROWCNT_INCONS:SS01000_WARNING
"The current row count is significantly different than when optimizedb was run
for column '%0c' in table %1c'. Histogram may not reflect current value
distribution."
/*%
** Parameters:
**      %0c - column name
**      %1c - table name
**
** Explanation:
**      Statdump encountered a histogram which was built when the table row 
**	count was significantly different (+/- 20%) than it is now. The value
**	distribution in the histogram may not reflect the current state of
**	the column.
**
** Recommendation:
**      Consider running optimizedb to rebuild the histogram.
*/
E_OP0993_XRBADMIX:SS50000_MISC_ERRORS
"%0c: Invalid mixture of %1c command line parameters."
/*%
** Parameters:
**      %0c - utility name (Optimizedb or Statdump)
**      %1c - optional text ("'xr' and 'r'" or "'xr' and 'a'")
**
** Explanation:
**      Use of the "-xr" command line options precludes the use of either the
**	"-r" or "-a" options.
**
** Recommendation:
**      Recode the command and retry.
*/
E_OP0994_DUP_RELS:SS50000_MISC_ERRORS
"%0c: multiple \"-r\" flags for table %1c, only one is permitted."
/*%
** Parameters:
**      %0c - utility name (Optimizedb or Statdump)
**      %1c - table name
**
** Explanation:
**      Only one "-rtablename" flag is permitted per table. Thus the following
**      is invalid:
**
**         optimizedb mydb -rtbl1 -acol1 -rtbl1 -acol2
**
**      similarly for statdump.
**
** Recommendation:
**      Instead of the example above use the following syntax:
** 
**         optimizedb mydb -rtbl1 -acol1 -acol2
*/
E_OP0A00_MAXSITE:SS50002_LIMIT_EXCEEDED
"distributed user error - max number of sites in query exceeded"
E_OP0A01_USELESS_PLAN:SS50000_MISC_ERRORS
"distributed plan expected to be useful but it was not"
E_OP0A80_UNSUPPORTED:SS50000_MISC_ERRORS
"distributed user error - this type of distributed query not supported"
E_OP0A81_SITE:SS50000_MISC_ERRORS
"site has not been selected for query plan"
E_OP0A82_RESULT_VAR:SS50000_MISC_ERRORS
"unexpected result variable number for update statement"
E_OP0A83_CONSTRAINT:SS50000_MISC_ERRORS
"consistency check - unexpected state for correlated subselect constraint"
E_OP0A84_TEMP_REQ:SS50000_MISC_ERRORS
"consistency check - temp table required but not a data transfer"
E_OP0A85_UPDATE_VAR:SS50000_MISC_ERRORS
"consistency check - expecting update variable to be defined"
E_OP0A86_EQCLASS:SS50000_MISC_ERRORS
"consistency check - cannot add attribute to equivalence class for update"
E_OP0A87_UPDT_MATCH:SS50000_MISC_ERRORS
"consistency check - new range variable for target relation expected to be
the last item in base relation list"
E_OP0A88_REPEAT:SS50000_MISC_ERRORS
"consistency check - repeat query parameter is out of order"
E_OP0A89_SAGG:SS50000_MISC_ERRORS
"consistency check - simple aggregate constant count is inconsistent"
E_OP0A8A_OPCSAGG:SS50000_MISC_ERRORS
"consistency check - unexpected tree structure found for simple aggregate"
E_OP0A8B_OPCPARM:SS50000_MISC_ERRORS
"consistency check - parameter nodes out of order for simple aggregate"
E_OP0A8C_COST:SS50000_MISC_ERRORS
"consistency check - cost resolution for final query plan cannot be found"
E_OP0A8D_VARIABLE:SS50000_MISC_ERRORS
"consistency check - OPF variable list is not ordered"
E_OP0A8E_SUB_TYPE:SS50000_MISC_ERRORS
"consistency check - unexpected subquery type"
E_OP0A8F_UPDATE_MODE:SS50000_MISC_ERRORS
"consistency check - unknown update mode"
E_OP0A90_NO_ATTRIBUTES:SS50000_MISC_ERRORS
"consistency check - no attributes in the for update list of the cursor"
E_OP0A91_CORELATED:SS50000_MISC_ERRORS
"consistency check
\n      - query tree correlation inconsistent"
E_OP0A92_BAD_LANGUAGE:SS50000_MISC_ERRORS
"consistency check
\n      - bad language ID found for query tree to text conversion"
E_OP0A93_BAD_QUERYMODE:SS50000_MISC_ERRORS
"consistency check
\n      - unexpected query mode"
E_OP0A94_OPCFPROJ:SS50000_MISC_ERRORS
"consistency check
\n      - projection/function aggregate error"
E_OP0A95_OPCBYTARGET:SS50000_MISC_ERRORS
"consistency check
\n      - function aggregate has no by/target list"
E_OP0A96_OPCSUBQTYPE:SS50000_MISC_ERRORS
"consistency check
\n      - unrecognized subquery type"
E_OP0A97_OPCSORTTYPE:SS50000_MISC_ERRORS
"Consistency Check
\n      - Sort requires keying on unsortable datatype."
E_OP0A98_OPCTIDEQC:SS50000_MISC_ERRORS
"consistency check
\n	- no tid equivalence class found when tid join required."
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, that a query plan has been
**      generated which depends upon the sort order of an unsortable datatype.
**      This type of query plan should not be generated.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/
E_OP0AA0_BAD_COMP_VALUE:SS50000_MISC_ERRORS
"Internal error: compression options were %0x. No compression performed."
/*%
** Parameters:
**      %0x - compression options received (in hex)
**
** Explanation:
**      This message is issued when opc_ahd_build() examines the pst_compress
**      options in the PST_RESTAB structure, which is supposed to contain the
**      compression options for the table which is being selected or retrieved
**      into, and discovers that the wrong bits are set.
**
**      Certain of the bits in this flag word are for the internal use of the
**      SQL parser only, and should have been cleared before the query plan was
**      passed to the optimizer.
**
**      To recover from this situation, the optimizer logs this message and
**      ignores all the compression options.
**
** System Status:
**      The table which is created does not use compression.
**
** Recommendation:
**      Please report this error message to Ingres Corporation Technical 
**      Support. If the compression characteristics of the table are important, 
**      it can be modified to the desired storage structure, and the 
**      compression options can be re-specified at that time.
*/
E_OP0B80_UNKNOWN_STATEMENT:SS50000_MISC_ERRORS
"internal DB procedure error- unknown statement type"
E_OP0B81_VARIABLE_DEC:SS50000_MISC_ERRORS
"internal DB procedure error- expecting variable declaration at beginning of procedure"
/*%
** Parameters:
**	None.
**
** Explanation:
**      The system has found, during optimization, that the internal structure
**      of a database procedure has a missing declaration statement.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to your product vendor.
*/
E_OP0B82_MISSING_PARAMETER:SS50000_MISC_ERRORS
"internal DB procedure error- internally generated variables for a DB procedure
\n      are not in order or are missing "
/*%
** Parameters:
**
** Explanation:
**      The system has found, during optimization, that the internal structure
**      of a database procedure has local variables that are not in the
**	expected order.
**
** System Status:
**	The query will be terminated; ongoing transactions are not aborted.
**
** Recommendation:
**      Please report the problem to Ingres Corporation Technical Support.
*/

NO_SQLSTATE_MAPPING
